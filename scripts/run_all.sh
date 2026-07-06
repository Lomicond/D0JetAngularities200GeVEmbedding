#!/usr/bin/env bash
set -Eeuo pipefail

# STAR/RCF compatibility: this script intentionally avoids Bash 4.3+ features
# such as namerefs (`local -n`). Bash 4.0 or newer is required for arrays and
# mapfile.
if (( BASH_VERSINFO[0] < 4 )); then
    printf 'ERROR: run_all.sh requires Bash >= 4.0; found %s\n' "$BASH_VERSION" >&2
    exit 2
fi

# =============================================================================
# D0EmbeddingClean end-to-end runner - Stage 4 v3
#
# Stages:
#   1. Standalone Pythia8 generation (custom Pythia 8.317)
#   2. Run14 MoreTags event selection (STAR SL22c)
#   3. DAQ chopping to the first N selected events (STAR SL22c)
#   4. STARSIM / GEANT3 (STAR SL22c, bundled Pythia 8.303 decayer)
#   5. Mixer / reconstruction (STAR SL22c)
#   6. PicoDst production from embedding MuDst (STAR SL22c)
#
# The script is intended to live at:
#   scripts/run_all.sh
#
# Example:
#   ./scripts/run_all.sh \
#       --nevents 200 \
#       --daq-list config/daq.list \
#       --mudst-list config/mudst.list \
#       --pair-index 1
#
# Useful modes:
#   --quiet
#   --verbose
#   --resume
#   --from STAGE
#   --stop-after STAGE
#
# Stage names:
#   pythia, moretags, chop, starsim, mixer, pico
# =============================================================================

# -----------------------------------------------------------------------------
# Project location
# -----------------------------------------------------------------------------
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

# -----------------------------------------------------------------------------
# Defaults
# -----------------------------------------------------------------------------
NEVENTS=""
DAQ_LIST="${ROOT_DIR}/config/daq.list"
MUDST_LIST="${ROOT_DIR}/config/mudst.list"
PAIR_INDEX=1

PYTHIA_SEED=12345
DETROIT_CONFIG="${ROOT_DIR}/config/detroit.cmnd"
PYTHIA_HOME="${PYTHIA8_HOME:-/gpfs01/star/pwg/lomicond/Ondrej/Jets/Alma9Pythia8/pythia8317-install}"

STAR_LEVEL="SL22c"

# 0 means: scan all events in the selected MuDst file.
# This is deliberately different from NEVENTS: makeMuDstQA_Run14's nEvents
# argument counts events READ, not events SELECTED.  Scanning all events and
# later taking the first N selected events avoids an accidental shortfall.
MORETAGS_SCAN_EVENTS=0

WORK_DIR=""
QUIET=0
VERBOSE=0
RESUME=0
DRY_RUN=0

START_STAGE="pythia"
STOP_STAGE="pico"

SPINNER_ENABLED=1

# -----------------------------------------------------------------------------
# Stage registry
# -----------------------------------------------------------------------------
STAGE_NAMES=(pythia moretags chop starsim mixer pico)
STAGE_LABELS=(
    "Pythia8 generation"
    "MoreTags selection"
    "DAQ chopping"
    "STARSIM / GEANT3"
    "Mixer / reconstruction"
    "PicoDst production"
)
TOTAL_STAGES=${#STAGE_NAMES[@]}

# -----------------------------------------------------------------------------
# Formatting
# -----------------------------------------------------------------------------
if [[ -t 1 ]]; then
    C_RESET=$'\033[0m'
    C_BOLD=$'\033[1m'
    C_GREEN=$'\033[32m'
    C_RED=$'\033[31m'
    C_YELLOW=$'\033[33m'
    C_CYAN=$'\033[36m'
else
    C_RESET=""
    C_BOLD=""
    C_GREEN=""
    C_RED=""
    C_YELLOW=""
    C_CYAN=""
    SPINNER_ENABLED=0
fi

info() {
    (( QUIET )) && return 0
    printf '%s\n' "$*"
}

warn() {
    (( QUIET )) && return 0
    printf '%sWARNING:%s %s\n' "${C_YELLOW}" "${C_RESET}" "$*" >&2
}

die() {
    printf '%sERROR:%s %s\n' "${C_RED}" "${C_RESET}" "$*" >&2
    exit 1
}

usage() {
    cat <<'USAGE'
Usage:
  ./scripts/run_all.sh --nevents N [options]

Required:
  --nevents N
      Number of aligned embedding events to produce.

Input pairing:
  --daq-list FILE
      DAQ list. Default: config/daq.list
  --mudst-list FILE
      MuDst list. Default: config/mudst.list
  --pair-index N
      1-based DAQ/MuDst pair index. Default: 1

Generation:
  --seed N
      Standalone Pythia seed. Default: 12345
  --detroit-config FILE
      Pythia command file. Default: config/detroit.cmnd
  --pythia-home DIR
      Custom Pythia 8.317 install prefix.
      Default:
      /gpfs01/star/pwg/lomicond/Ondrej/Jets/Alma9Pythia8/pythia8317-install

STAR:
  --star-level LEVEL
      STAR release. Default: SL22c
  --moretags-scan-events N
      Number of MuDst events READ by makeMuDstQA_Run14.
      0 means all events in the selected MuDst file. Default: 0

Workflow:
  --work-dir DIR
      Per-job working directory.
      Default: work/jobs/<DAQ stem>
  --from STAGE
      Start at: pythia, moretags, chop, starsim, mixer, pico
  --stop-after STAGE
      Stop after the named stage.
  --resume
      Skip a stage when its semantic validation already passes.
  --dry-run
      Print the resolved configuration without running stages.

Output:
  --quiet
      No progress output; errors are still shown.
  --verbose
      Stream full stage output and also save it to logs.

Other:
  -h, --help
USAGE
}

# -----------------------------------------------------------------------------
# Argument parsing
# -----------------------------------------------------------------------------
while (($#)); do
    case "$1" in
        --nevents)
            [[ $# -ge 2 ]] || die "--nevents requires a value"
            NEVENTS="$2"
            shift 2
            ;;
        --daq-list)
            [[ $# -ge 2 ]] || die "--daq-list requires a value"
            DAQ_LIST="$2"
            shift 2
            ;;
        --mudst-list)
            [[ $# -ge 2 ]] || die "--mudst-list requires a value"
            MUDST_LIST="$2"
            shift 2
            ;;
        --pair-index)
            [[ $# -ge 2 ]] || die "--pair-index requires a value"
            PAIR_INDEX="$2"
            shift 2
            ;;
        --seed)
            [[ $# -ge 2 ]] || die "--seed requires a value"
            PYTHIA_SEED="$2"
            shift 2
            ;;
        --detroit-config)
            [[ $# -ge 2 ]] || die "--detroit-config requires a value"
            DETROIT_CONFIG="$2"
            shift 2
            ;;
        --pythia-home)
            [[ $# -ge 2 ]] || die "--pythia-home requires a value"
            PYTHIA_HOME="$2"
            shift 2
            ;;
        --star-level)
            [[ $# -ge 2 ]] || die "--star-level requires a value"
            STAR_LEVEL="$2"
            shift 2
            ;;
        --moretags-scan-events)
            [[ $# -ge 2 ]] || die "--moretags-scan-events requires a value"
            MORETAGS_SCAN_EVENTS="$2"
            shift 2
            ;;
        --work-dir)
            [[ $# -ge 2 ]] || die "--work-dir requires a value"
            WORK_DIR="$2"
            shift 2
            ;;
        --from)
            [[ $# -ge 2 ]] || die "--from requires a stage name"
            START_STAGE="$2"
            shift 2
            ;;
        --stop-after)
            [[ $# -ge 2 ]] || die "--stop-after requires a stage name"
            STOP_STAGE="$2"
            shift 2
            ;;
        --resume)
            RESUME=1
            shift
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        --quiet)
            QUIET=1
            shift
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "Unknown option: $1"
            ;;
    esac
done

(( QUIET && VERBOSE )) && die "--quiet and --verbose are mutually exclusive"

[[ "$NEVENTS" =~ ^[1-9][0-9]*$ ]] || die "--nevents must be a positive integer"
[[ "$PAIR_INDEX" =~ ^[1-9][0-9]*$ ]] || die "--pair-index must be a positive integer"
[[ "$PYTHIA_SEED" =~ ^[0-9]+$ ]] || die "--seed must be a non-negative integer"
[[ "$MORETAGS_SCAN_EVENTS" =~ ^[0-9]+$ ]] || die "--moretags-scan-events must be a non-negative integer"

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------
stage_index() {
    local needle="$1"
    local i
    for i in "${!STAGE_NAMES[@]}"; do
        if [[ "${STAGE_NAMES[$i]}" == "$needle" ]]; then
            printf '%d\n' "$i"
            return 0
        fi
    done
    return 1
}

START_IDX="$(stage_index "$START_STAGE")" || die "Unknown --from stage: $START_STAGE"
STOP_IDX="$(stage_index "$STOP_STAGE")" || die "Unknown --stop-after stage: $STOP_STAGE"
(( START_IDX <= STOP_IDX )) || die "--from stage occurs after --stop-after stage"

resolve_existing_file() {
    local value="$1"
    local list_dir="${2:-$ROOT_DIR}"

    if [[ -f "$value" ]]; then
        readlink -f "$value"
        return 0
    fi
    if [[ -f "${ROOT_DIR}/${value}" ]]; then
        readlink -f "${ROOT_DIR}/${value}"
        return 0
    fi
    if [[ -f "${list_dir}/${value}" ]]; then
        readlink -f "${list_dir}/${value}"
        return 0
    fi
    return 1
}

resolve_existing_dir() {
    local value="$1"
    if [[ -d "$value" ]]; then
        readlink -f "$value"
        return 0
    fi
    if [[ -d "${ROOT_DIR}/${value}" ]]; then
        readlink -f "${ROOT_DIR}/${value}"
        return 0
    fi
    return 1
}

read_clean_list() {
    local file="$1"

    sed -e 's/\r$//' \
        -e 's/^[[:space:]]*//' \
        -e 's/[[:space:]]*$//' \
        "$file" \
    | grep -vE '^(#|$)'
}

daq_stem() {
    local base
    base="$(basename -- "$1")"
    [[ "$base" == *.daq ]] || return 1
    printf '%s\n' "${base%.daq}"
}

mudst_stem() {
    local base
    base="$(basename -- "$1")"
    [[ "$base" == *.MuDst.root ]] || return 1
    printf '%s\n' "${base%.MuDst.root}"
}

strip_path_component() {
    # Remove path components containing either pattern.  Used to prevent a
    # custom Pythia 8.317 environment from leaking into STAR stages.
    local value="${1:-}"
    printf '%s\n' "$value" \
        | tr ':' '\n' \
        | grep -v -E 'pythia8317|Alma9Pythia8' \
        | paste -sd: -
}

safe_root_string() {
    local value="$1"
    [[ "$value" != *'"'* ]] || die "Path contains a double quote and cannot be embedded safely in a ROOT macro: $value"
    [[ "$value" != *$'\n'* ]] || die "Path contains a newline: $value"
}

get_root_entries() {
    local file="$1"
    local tree="$2"
    local root_cmd=""

    if command -v root >/dev/null 2>&1; then
        root_cmd="root"
    elif command -v root4star >/dev/null 2>&1; then
        root_cmd="root4star"
    else
        return 2
    fi

    safe_root_string "$file"
    safe_root_string "$tree"

    "$root_cmd" -l -b 2>/dev/null <<EOF \
        | awk '/^__D0WF_ENTRIES__/ {print $2; exit}'
TFile f("$file");
TTree *t = (TTree*)f.Get("$tree");
cout << "__D0WF_ENTRIES__ " << (t ? t->GetEntries() : -1) << endl;
.q
EOF
}

format_seconds() {
    local total="$1"
    if (( total < 60 )); then
        printf '%ds' "$total"
    elif (( total < 3600 )); then
        printf '%dm%02ds' "$((total/60))" "$((total%60))"
    else
        printf '%dh%02dm%02ds' "$((total/3600))" "$(((total%3600)/60))" "$((total%60))"
    fi
}

show_failure() {
    local label="$1"
    local log="$2"
    local rc="$3"

    printf '\n%sFAILED:%s %s (status %s)\n' "${C_RED}" "${C_RESET}" "$label" "$rc" >&2
    printf 'Log: %s\n' "$log" >&2

    if [[ -f "$log" ]]; then
        printf '%s\n' '--- last 60 log lines ---' >&2
        tail -n 60 "$log" >&2 || true
        printf '%s\n' '-------------------------' >&2
    fi
}

run_stage() {
    local idx="$1"
    local label="$2"
    local log="$3"
    local stage_fn="$4"
    local validate_fn="$5"

    mkdir -p "$(dirname -- "$log")"

    if (( RESUME )) && "$validate_fn" >/dev/null 2>&1; then
        if (( ! QUIET )); then
            printf '[%d/%d] %-24s %sSKIPPED%s (already valid)\n' \
                "$((idx+1))" "$TOTAL_STAGES" "$label" "${C_YELLOW}" "${C_RESET}"
        fi
        return 0
    fi

    local start_time end_time elapsed rc=0
    start_time="$(date +%s)"

    if (( VERBOSE )); then
        printf '\n%s[%d/%d] %s%s\n' "${C_BOLD}" "$((idx+1))" "$TOTAL_STAGES" "$label" "${C_RESET}"
        set +e
        "$stage_fn" 2>&1 | tee "$log"
        rc=${PIPESTATUS[0]}
        set -e
    else
        set +e
        "$stage_fn" >"$log" 2>&1 &
        local pid=$!

        if (( ! QUIET )); then
            if (( SPINNER_ENABLED )); then
                local frames='|/-\'
                local n=0
                while kill -0 "$pid" 2>/dev/null; do
                    printf '\r[%d/%d] %-24s %s' \
                        "$((idx+1))" "$TOTAL_STAGES" "$label" "${frames:n%4:1}"
                    n=$((n+1))
                    sleep 0.15
                done
            else
                printf '[%d/%d] %s ...\n' "$((idx+1))" "$TOTAL_STAGES" "$label"
            fi
        fi

        wait "$pid"
        rc=$?
        set -e
    fi

    if (( rc == 0 )); then
        if ! "$validate_fn"; then
            rc=90
        fi
    fi

    end_time="$(date +%s)"
    elapsed=$((end_time-start_time))

    if (( rc != 0 )); then
        if (( SPINNER_ENABLED && ! VERBOSE && ! QUIET )); then
            printf '\r\033[K'
        fi
        show_failure "$label" "$log" "$rc"
        exit "$rc"
    fi

    if (( ! QUIET )); then
        if (( SPINNER_ENABLED && ! VERBOSE )); then
            printf '\r\033[K'
        fi
        printf '[%d/%d] %-24s %sOK%s (%s)\n' \
            "$((idx+1))" "$TOTAL_STAGES" "$label" "${C_GREEN}" "${C_RESET}" "$(format_seconds "$elapsed")"
    fi
}

# -----------------------------------------------------------------------------
# Resolve lists and selected pair
# -----------------------------------------------------------------------------
DAQ_LIST="$(resolve_existing_file "$DAQ_LIST" "$ROOT_DIR")" || die "DAQ list not found: $DAQ_LIST"
MUDST_LIST="$(resolve_existing_file "$MUDST_LIST" "$ROOT_DIR")" || die "MuDst list not found: $MUDST_LIST"
DETROIT_CONFIG="$(resolve_existing_file "$DETROIT_CONFIG" "$ROOT_DIR")" || die "Detroit config not found: $DETROIT_CONFIG"
PYTHIA_HOME="$(resolve_existing_dir "$PYTHIA_HOME")" || die "Custom Pythia directory not found: $PYTHIA_HOME"

declare -a DAQ_ITEMS MUDST_ITEMS

# Avoid Bash 4.3+ namerefs (`local -n`): STAR/RCF nodes may still provide
# an older Bash. `mapfile` is available in Bash 4.x and is sufficient here.
mapfile -t DAQ_ITEMS < <(read_clean_list "$DAQ_LIST")
mapfile -t MUDST_ITEMS < <(read_clean_list "$MUDST_LIST")

((${#DAQ_ITEMS[@]} > 0)) || die "DAQ list is empty: $DAQ_LIST"
((${#MUDST_ITEMS[@]} > 0)) || die "MuDst list is empty: $MUDST_LIST"

if ((${#DAQ_ITEMS[@]} != ${#MUDST_ITEMS[@]})); then
    warn "DAQ and MuDst lists have different lengths (${#DAQ_ITEMS[@]} vs ${#MUDST_ITEMS[@]}). The selected pair will still be validated."
fi

PAIR_ZERO=$((PAIR_INDEX-1))
(( PAIR_ZERO < ${#DAQ_ITEMS[@]} )) || die "--pair-index $PAIR_INDEX exceeds DAQ list length ${#DAQ_ITEMS[@]}"
(( PAIR_ZERO < ${#MUDST_ITEMS[@]} )) || die "--pair-index $PAIR_INDEX exceeds MuDst list length ${#MUDST_ITEMS[@]}"

DAQ_LIST_DIR="$(dirname -- "$DAQ_LIST")"
MUDST_LIST_DIR="$(dirname -- "$MUDST_LIST")"

DAQ_FILE="$(resolve_existing_file "${DAQ_ITEMS[$PAIR_ZERO]}" "$DAQ_LIST_DIR")" \
    || die "DAQ file not found for pair $PAIR_INDEX: ${DAQ_ITEMS[$PAIR_ZERO]}"
MUDST_FILE="$(resolve_existing_file "${MUDST_ITEMS[$PAIR_ZERO]}" "$MUDST_LIST_DIR")" \
    || die "MuDst file not found for pair $PAIR_INDEX: ${MUDST_ITEMS[$PAIR_ZERO]}"

DAQ_STEM="$(daq_stem "$DAQ_FILE")" || die "DAQ filename must end in .daq: $DAQ_FILE"
MUDST_STEM="$(mudst_stem "$MUDST_FILE")" || die "MuDst filename must end in .MuDst.root: $MUDST_FILE"

[[ "$DAQ_STEM" == "$MUDST_STEM" ]] || die \
    "DAQ/MuDst pair mismatch:
  DAQ stem:   $DAQ_STEM
  MuDst stem: $MUDST_STEM"

STEM="$DAQ_STEM"

if [[ -z "$WORK_DIR" ]]; then
    WORK_DIR="${ROOT_DIR}/work/jobs/${STEM}"
elif [[ "$WORK_DIR" != /* ]]; then
    WORK_DIR="${ROOT_DIR}/${WORK_DIR}"
fi
mkdir -p "$WORK_DIR"
WORK_DIR="$(readlink -f "$WORK_DIR")"

# -----------------------------------------------------------------------------
# Job paths
# -----------------------------------------------------------------------------
PYTHIA_DIR="${WORK_DIR}/pythia"
MORETAGS_DIR="${WORK_DIR}/moretags"
CHOP_DIR="${WORK_DIR}/chopped"
STARSIM_DIR="${WORK_DIR}/starsim"
MIXER_DIR="${WORK_DIR}/mixer"
PICO_DIR="${WORK_DIR}/pico"
LOG_DIR="${WORK_DIR}/logs"
DRIVER_DIR="${WORK_DIR}/drivers"

mkdir -p \
    "$PYTHIA_DIR" "$MORETAGS_DIR" "$CHOP_DIR" \
    "$STARSIM_DIR" "$MIXER_DIR" "$PICO_DIR" "$LOG_DIR" "$DRIVER_DIR"

PYTHIA_BIN="${ROOT_DIR}/work/bin/make_d0_pythia8"
PYTHIA_OUTPUT="${PYTHIA_DIR}/pythia8_D0_DetroitTune.root"

JOB_MUDST_LIST="${WORK_DIR}/mudst.selected.list"
printf '%s\n' "$MUDST_FILE" > "$JOB_MUDST_LIST"

MORETAGS_ROOT="${MORETAGS_DIR}/${STEM}.moretags.root"
CHOPPER_TXT="${MORETAGS_DIR}/${STEM}.chopper.txt"
TRIMMED_CHOPPER="${CHOP_DIR}/${STEM}.first_${NEVENTS}.chopper.txt"
CHOPPED_DAQ="${CHOP_DIR}/${STEM}.daq"

STARSIM_ROOT="${STARSIM_DIR}/D0toy.starsim.root"
STARSIM_FZD="${STARSIM_DIR}/D0toy.starsim.fzd"

MIXER_MUDST="${MIXER_DIR}/${STEM}.MuDst.root"
PICO_OUTPUT="${PICO_DIR}/${STEM}.picoDst.root"
PICO_MACRO="${ROOT_DIR}/src/pico/makePicoDstFromMuDst.C"

LOG_PYTHIA="${LOG_DIR}/01_pythia.log"
LOG_MORETAGS="${LOG_DIR}/02_moretags.log"
LOG_CHOP="${LOG_DIR}/03_chop.log"
LOG_STARSIM="${LOG_DIR}/04_starsim.log"
LOG_MIXER="${LOG_DIR}/05_mixer.log"
LOG_PICO="${LOG_DIR}/06_pico.log"

# Paths embedded into ROOT strings must be simple.
for p in \
    "$ROOT_DIR" "$PYTHIA_OUTPUT" "$JOB_MUDST_LIST" "$MORETAGS_DIR" \
    "$MORETAGS_ROOT" "$CHOPPED_DAQ" "$STARSIM_ROOT" "$STARSIM_FZD" \
    "$MIXER_DIR" "$MIXER_MUDST" "$PICO_DIR" "$PICO_MACRO"
do
    safe_root_string "$p"
done

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------
if (( ! QUIET )); then
    cat <<EOF
${C_BOLD}D0 embedding workflow${C_RESET}
  project          = ${ROOT_DIR}
  pair index       = ${PAIR_INDEX}
  events           = ${NEVENTS}
  DAQ              = ${DAQ_FILE}
  MuDst            = ${MUDST_FILE}
  stem             = ${STEM}
  work dir         = ${WORK_DIR}

  Pythia home      = ${PYTHIA_HOME}
  Pythia seed      = ${PYTHIA_SEED}
  Detroit config   = ${DETROIT_CONFIG}

  STAR level       = ${STAR_LEVEL}
  MoreTags scan    = ${MORETAGS_SCAN_EVENTS}  (0 = all MuDst events)
  stages           = ${START_STAGE} -> ${STOP_STAGE}
  resume           = ${RESUME}
EOF
fi

if (( DRY_RUN )); then
    exit 0
fi

# -----------------------------------------------------------------------------
# STAR tcsh prelude generator
# -----------------------------------------------------------------------------
write_star_prelude() {
    local target="$1"
    cat > "$target" <<EOF
#!/bin/tcsh

# Remove the standalone custom Pythia 8.317 environment before STAR.
unsetenv PYTHIA8
unsetenv PYTHIA8DATA

if (\$?PATH) then
    setenv PATH \`echo "\$PATH" | tr ':' '\\n' | grep -v -E 'pythia8317|Alma9Pythia8' | paste -sd: -\`
endif

if (\$?LD_LIBRARY_PATH) then
    setenv LD_LIBRARY_PATH \`echo "\$LD_LIBRARY_PATH" | tr ':' '\\n' | grep -v -E 'pythia8317|Alma9Pythia8' | paste -sd: -\`
endif

rehash
starver ${STAR_LEVEL}
set starver_rc = \$status
if (\$starver_rc != 0) then
    echo "ERROR: starver ${STAR_LEVEL} failed with status \$starver_rc"
    exit \$starver_rc
endif
rehash

echo "D0WF_STAR_LEVEL=\$STAR_LEVEL"
echo "D0WF_STAR=\$STAR"
EOF
    chmod +x "$target"
}

# -----------------------------------------------------------------------------
# Stage 1: Pythia
# -----------------------------------------------------------------------------
stage_pythia() {
    (
        local clean_path clean_ld
        clean_path="$(strip_path_component "${PATH:-}")"
        clean_ld="$(strip_path_component "${LD_LIBRARY_PATH:-}")"

        export PYTHIA8="$PYTHIA_HOME"
        export PYTHIA8DATA="${PYTHIA_HOME}/share/Pythia8/xmldoc"
        export PATH="${PYTHIA_HOME}/bin:${clean_path}"
        export LD_LIBRARY_PATH="${PYTHIA_HOME}/lib${clean_ld:+:${clean_ld}}"
        hash -r

        [[ -x "${PYTHIA_HOME}/bin/pythia8-config" ]] \
            || { echo "ERROR: pythia8-config not found under ${PYTHIA_HOME}/bin"; return 2; }
        [[ -d "$PYTHIA8DATA" ]] \
            || { echo "ERROR: PYTHIA8DATA not found: $PYTHIA8DATA"; return 2; }

        command -v g++ >/dev/null 2>&1 || { echo "ERROR: g++ not found"; return 2; }
        command -v root-config >/dev/null 2>&1 || { echo "ERROR: root-config not found"; return 2; }

        mkdir -p "$(dirname -- "$PYTHIA_BIN")"

        if [[ ! -x "$PYTHIA_BIN" || "${ROOT_DIR}/src/pythia/make_d0_pythia8.cc" -nt "$PYTHIA_BIN" ]]; then
            echo "Building standalone Pythia generator..."
            # shellcheck disable=SC2046
            g++ -O2 -std=c++11 \
                "${ROOT_DIR}/src/pythia/make_d0_pythia8.cc" \
                -o "$PYTHIA_BIN" \
                $(pythia8-config --cxxflags) \
                $(root-config --cflags) \
                $(pythia8-config --libs) \
                $(root-config --libs)
        else
            echo "Using existing generator binary: $PYTHIA_BIN"
        fi

        rm -f "$PYTHIA_OUTPUT"

        "$PYTHIA_BIN" \
            "$NEVENTS" \
            "$PYTHIA_SEED" \
            "$DETROIT_CONFIG" \
            "$PYTHIA_OUTPUT"
    )
}

validate_pythia() {
    [[ -s "$PYTHIA_OUTPUT" ]] || return 1

    local entries rc
    set +e
    entries="$(get_root_entries "$PYTHIA_OUTPUT" "D0Tree")"
    rc=$?
    set -e

    if (( rc == 0 )); then
        [[ "$entries" == "$NEVENTS" ]] || {
            echo "Validation error: D0Tree entries=$entries, expected=$NEVENTS"
            return 1
        }
    fi

    return 0
}

# -----------------------------------------------------------------------------
# Stage 2: MoreTags
# -----------------------------------------------------------------------------
stage_moretags() {
    local csh="${DRIVER_DIR}/02_moretags.csh"
    write_star_prelude "$csh"

    cat >> "$csh" <<EOF

mkdir -p "${MORETAGS_DIR}"
rm -f "${MORETAGS_ROOT}" "${CHOPPER_TXT}"

root4star -l -b -q '${ROOT_DIR}/src/moretags/makeMuDstQA_Run14.C("${JOB_MUDST_LIST}",1,${MORETAGS_SCAN_EVENTS},"${MORETAGS_DIR}/")'
set rc = \$status
exit \$rc
EOF

    tcsh "$csh"
}

validate_moretags() {
    [[ -s "$MORETAGS_ROOT" ]] || {
        echo "Validation error: missing MoreTags ROOT file: $MORETAGS_ROOT"
        return 1
    }
    [[ -s "$CHOPPER_TXT" ]] || {
        echo "Validation error: missing chopper list: $CHOPPER_TXT"
        return 1
    }

    local selected
    selected="$(wc -l < "$CHOPPER_TXT")"
    (( selected >= NEVENTS )) || {
        echo "Validation error: only $selected selected real events, need $NEVENTS."
        echo "Increase --moretags-scan-events, or use 0 to scan the full MuDst file."
        return 1
    }

    local entries rc
    set +e
    entries="$(get_root_entries "$MORETAGS_ROOT" "MoreTags")"
    rc=$?
    set -e

    if (( rc == 0 )); then
        (( entries >= NEVENTS )) || {
            echo "Validation error: MoreTags entries=$entries, need at least $NEVENTS"
            return 1
        }
        [[ "$entries" == "$selected" ]] || {
            echo "Validation error: MoreTags entries=$entries but chopper lines=$selected"
            return 1
        }
    fi

    return 0
}

# -----------------------------------------------------------------------------
# Stage 3: DAQ chopping
# -----------------------------------------------------------------------------
stage_chop() {
    local selected
    selected="$(wc -l < "$CHOPPER_TXT")"
    (( selected >= NEVENTS )) || {
        echo "ERROR: chopper list contains only $selected selected events; need $NEVENTS"
        return 2
    }

    head -n "$NEVENTS" "$CHOPPER_TXT" > "$TRIMMED_CHOPPER"

    local csh="${DRIVER_DIR}/03_chop.csh"
    write_star_prelude "$csh"

    cat >> "$csh" <<EOF

which daqFileChopper
if (\$status != 0) then
    echo "ERROR: daqFileChopper not found after starver ${STAR_LEVEL}"
    exit 2
endif

set events = (\`awk '{print \$2}' "${TRIMMED_CHOPPER}"\`)
if (\$#events != ${NEVENTS}) then
    echo "ERROR: expected ${NEVENTS} event numbers, got \$#events"
    exit 3
endif

rm -f "${CHOPPED_DAQ}"
daqFileChopper "${DAQ_FILE}" "-eventnum" \$events >! "${CHOPPED_DAQ}"
set rc = \$status
exit \$rc
EOF

    tcsh "$csh"
}

validate_chop() {
    [[ -s "$TRIMMED_CHOPPER" ]] || return 1
    [[ -s "$CHOPPED_DAQ" ]] || return 1

    local lines
    lines="$(wc -l < "$TRIMMED_CHOPPER")"
    [[ "$lines" == "$NEVENTS" ]] || {
        echo "Validation error: trimmed chopper lines=$lines, expected=$NEVENTS"
        return 1
    }

    return 0
}

# -----------------------------------------------------------------------------
# Stage 4: STARSIM / GEANT3
# -----------------------------------------------------------------------------
stage_starsim() {
    local csh="${DRIVER_DIR}/04_starsim.csh"
    write_star_prelude "$csh"

    cat >> "$csh" <<EOF

rm -f "${STARSIM_ROOT}" "${STARSIM_FZD}"

root4star -l -b << ROOTEOF
.include \$STAR/StRoot

.L ${ROOT_DIR}/src/starsim/starsim.D0toy.C

starsim(
    ${NEVENTS},
    "${PYTHIA_OUTPUT}",
    "${MORETAGS_ROOT}",
    "${STARSIM_ROOT}",
    "${STARSIM_FZD}"
);

.q
ROOTEOF

set rc = \$status
exit \$rc
EOF

    tcsh "$csh"
}

validate_starsim() {
    [[ -s "$STARSIM_ROOT" ]] || {
        echo "Validation error: missing STARSIM ROOT output"
        return 1
    }
    [[ -s "$STARSIM_FZD" ]] || {
        echo "Validation error: missing STARSIM FZD output"
        return 1
    }

    grep -q "STARSIM Stage 2C v2 finished." "$LOG_STARSIM" || {
        echo "Validation error: STARSIM completion marker not found"
        return 1
    }

    grep -Eq "NUMBER OF EVENTS PROCESSED[[:space:]]*=[[:space:]]*${NEVENTS}([[:space:]]|$)" "$LOG_STARSIM" || {
        echo "Validation error: STARSIM did not report exactly $NEVENTS processed events"
        return 1
    }

    grep -q "output dir  = restored" "$LOG_STARSIM" || {
        echo "Validation error: ROOT output directory finalization was not restored"
        return 1
    }

    return 0
}

# -----------------------------------------------------------------------------
# Stage 5: Mixer / reconstruction
# -----------------------------------------------------------------------------
stage_mixer() {
    # The current wrapper checks the canonical macro through the relative path
    # src/mixer/bfcMixer_Hft_D0toy.C.  Run inside the job mixer directory and
    # provide a job-local src symlink so outputs stay isolated here.
    ln -sfn "${ROOT_DIR}/src" "${MIXER_DIR}/src"

    local csh="${DRIVER_DIR}/05_mixer.csh"
    write_star_prelude "$csh"

    cat >> "$csh" <<EOF

cd "${MIXER_DIR}"

root4star -l -b << ROOTEOF
.include \$STAR/StRoot

.L src/mixer/bfcMixer_Hft_D0toy.C
.L src/mixer/runMixerD0toy_ZB.C

runMixerD0toy_ZB(
    ${NEVENTS},
    "${CHOPPED_DAQ}",
    "${STARSIM_FZD}"
);

.q
ROOTEOF

set rc = \$status
exit \$rc
EOF

    tcsh "$csh"
}

validate_mixer() {
    [[ -s "$LOG_MIXER" ]] || return 1

    grep -q "Mixer Stage 3 wrapper finished." "$LOG_MIXER" || {
        echo "Validation error: mixer wrapper completion marker not found"
        return 1
    }
    grep -q "Run completed" "$LOG_MIXER" || {
        echo "Validation error: STAR 'Run completed' marker not found"
        return 1
    }

    local summaries restored bad
    summaries="$(grep -c 'D0toyManualLoopSummary' "$LOG_MIXER" || true)"
    restored="$(grep -c 'D0toy: restored EvtHddr before chain3' "$LOG_MIXER" || true)"

    [[ "$summaries" == "$NEVENTS" ]] || {
        echo "Validation error: mixer summaries=$summaries, expected=$NEVENTS"
        return 1
    }
    [[ "$restored" == "$NEVENTS" ]] || {
        echo "Validation error: EvtHddr restores=$restored, expected=$NEVENTS"
        return 1
    }

    bad="$(
        grep 'D0toyManualLoopSummary' "$LOG_MIXER" \
        | awk '
            $0 !~ /iMake=0/ ||
            $0 !~ /chain1Return=0/ ||
            $0 !~ /chain2Return=0/ ||
            $0 !~ /chain3Return=0/ ||
            $0 !~ /inputReturn=0/ {n++}
            END {print n+0}
        '
    )"
    [[ "$bad" == "0" ]] || {
        echo "Validation error: $bad mixer event summaries contain a non-zero return code"
        return 1
    }

    [[ -s "$MIXER_MUDST" ]] || {
        echo "Validation error: expected MuDst output not found: $MIXER_MUDST"
        return 1
    }

    return 0
}

# -----------------------------------------------------------------------------
# Stage 6: PicoDst production from reconstructed embedding MuDst
# -----------------------------------------------------------------------------
stage_pico() {
    [[ -f "$PICO_MACRO" ]] || {
        echo "ERROR: PicoDst conversion macro not found: $PICO_MACRO"
        return 2
    }

    local csh="${DRIVER_DIR}/06_pico.csh"
    write_star_prelude "$csh"

    cat >> "$csh" <<EOF

mkdir -p "${PICO_DIR}"
rm -f "${PICO_OUTPUT}"

cd "${PICO_DIR}"

root4star -l -b << ROOTEOF
.include \$STAR/StRoot

.L ${PICO_MACRO}

makePicoDstFromMuDst(
    "${MIXER_MUDST}",
    ${NEVENTS},
    "y2014a"
);

.q
ROOTEOF

set rc = \$status
exit \$rc
EOF

    tcsh "$csh"
}

validate_pico() {
    [[ -s "$PICO_OUTPUT" ]] || {
        echo "Validation error: expected PicoDst output not found: $PICO_OUTPUT"
        return 1
    }

    [[ -s "$LOG_PICO" ]] || return 1

    grep -q "PicoDst Stage 6 finished." "$LOG_PICO" || {
        echo "Validation error: PicoDst completion marker not found"
        return 1
    }

    grep -Eq \
        "PicoDstStage6Summary requested=${NEVENTS} processed=${NEVENTS} written=[0-9]+ errors=0" \
        "$LOG_PICO" || {
        echo "Validation error: PicoDst summary is missing, incomplete, or contains errors"
        return 1
    }

    local entries rc
    set +e
    entries="$(get_root_entries "$PICO_OUTPUT" "PicoDst")"
    rc=$?
    set -e

    if (( rc == 0 )); then
        [[ "$entries" =~ ^[0-9]+$ ]] || {
            echo "Validation error: invalid PicoDst entry count: $entries"
            return 1
        }

        (( entries > 0 )) || {
            echo "Validation error: PicoDst tree contains zero events"
            return 1
        }

        (( entries <= NEVENTS )) || {
            echo "Validation error: PicoDst entries=$entries exceeds requested MuDst events=$NEVENTS"
            return 1
        }
    fi

    return 0
}

# -----------------------------------------------------------------------------
# Dependency checks for --from
# -----------------------------------------------------------------------------
VALIDATORS=(
    validate_pythia
    validate_moretags
    validate_chop
    validate_starsim
    validate_mixer
    validate_pico
)

if (( START_IDX > 0 )); then
    for ((i=0; i<START_IDX; i++)); do
        if ! "${VALIDATORS[$i]}" >/dev/null 2>&1; then
            die "Cannot start from '${START_STAGE}': prerequisite stage '${STAGE_NAMES[$i]}' is not valid in ${WORK_DIR}"
        fi
    done
fi

# -----------------------------------------------------------------------------
# Run selected range
# -----------------------------------------------------------------------------
STAGE_FUNCS=(
    stage_pythia
    stage_moretags
    stage_chop
    stage_starsim
    stage_mixer
    stage_pico
)

STAGE_LOGS=(
    "$LOG_PYTHIA"
    "$LOG_MORETAGS"
    "$LOG_CHOP"
    "$LOG_STARSIM"
    "$LOG_MIXER"
    "$LOG_PICO"
)

for ((i=START_IDX; i<=STOP_IDX; i++)); do
    run_stage \
        "$i" \
        "${STAGE_LABELS[$i]}" \
        "${STAGE_LOGS[$i]}" \
        "${STAGE_FUNCS[$i]}" \
        "${VALIDATORS[$i]}"
done

# -----------------------------------------------------------------------------
# Final summary
# -----------------------------------------------------------------------------
if (( ! QUIET )); then
    cat <<EOF

${C_GREEN}${C_BOLD}Workflow completed.${C_RESET}
  work dir       = ${WORK_DIR}
  Pythia ROOT    = ${PYTHIA_OUTPUT}
  MoreTags ROOT  = ${MORETAGS_ROOT}
  chopped DAQ    = ${CHOPPED_DAQ}
  STARSIM FZD    = ${STARSIM_FZD}
  mixer MuDst    = ${MIXER_MUDST}
  PicoDst         = ${PICO_OUTPUT}
  logs           = ${LOG_DIR}
EOF
fi
