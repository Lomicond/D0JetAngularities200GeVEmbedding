#!/bin/tcsh -f
#
# run_standalone_production_v3.csh
#
# Standalone D0 MC chain:
#
#   1 PYTHIA 8.303
#   2 Event tags / MoreTags
#   3 SKIPPED: DAQ chopper
#   4 STARSIM / GEANT3
#   5 standalone FZD -> MuDst reconstruction
#   6 MuDst -> PicoDst
#
# Default test:
#
#   tcsh scripts/run_standalone_production_v3.csh
#
# Arguments:
#
#   argv[1] = number of events, default 5
#   argv[2] = start stage, default 1
#   argv[3] = force rerun, default 0
#   argv[4] = PYTHIA seed, default 1000011
#
# Examples:
#
#   Fresh 5-event run:
#       tcsh scripts/run_standalone_production_v3.csh 5 1 0
#
#   Resume from STARSIM:
#       tcsh scripts/run_standalone_production_v3.csh 5 4 0
#
#   Force only PicoDst:
#       tcsh scripts/run_standalone_production_v3.csh 5 6 1
#

umask 002
set nonomatch

# ============================================================================
# Basic configuration
# ============================================================================

setenv D0WF \
"/gpfs/mnt/gpfs01/star/pwg/lomicond/Ondrej/Jets/PythiaD0JetGeant/D0EmbeddingClean"

set NEVENTS = 5
set START_STAGE = 1
set FORCE = 0
set PYTHIA_SEED = 1000011

if ( $#argv >= 1 ) then
    set NEVENTS = "$argv[1]"
endif

if ( $#argv >= 2 ) then
    set START_STAGE = "$argv[2]"
endif

if ( $#argv >= 3 ) then
    set FORCE = "$argv[3]"
endif

if ( $#argv >= 4 ) then
    set PYTHIA_SEED = "$argv[4]"
endif

if ( $NEVENTS <= 0 ) then
    echo "ERROR: NEVENTS must be positive"
    exit 1
endif

if ( "$START_STAGE" != "1" && \
     "$START_STAGE" != "2" && \
     "$START_STAGE" != "4" && \
     "$START_STAGE" != "5" && \
     "$START_STAGE" != "6" ) then

    echo "ERROR: START_STAGE must be 1, 2, 4, 5, or 6"
    exit 2
endif

if ( "$FORCE" != "0" && \
     "$FORCE" != "1" ) then

    echo "ERROR: FORCE must be 0 or 1"
    exit 3
endif

if ( ! -d "$D0WF" ) then
    echo "ERROR: D0WF does not exist:"
    echo "       $D0WF"
    exit 4
endif

set SETUP_SCRIPT = \
"$D0WF/scripts/setup_SL16d2_D0Embedding_hybrid.csh"

set CHECK_MACRO = \
"$D0WF/scripts/checkRootTree_v1.C"

if ( ! -f "$SETUP_SCRIPT" ) then
    echo "ERROR: setup script is missing:"
    echo "       $SETUP_SCRIPT"
    exit 5
endif

if ( ! -f "$CHECK_MACRO" ) then
    echo "ERROR: ROOT validation macro is missing:"
    echo "       $CHECK_MACRO"
    exit 6
endif

# ============================================================================
# Immutable production templates
# ============================================================================

set SIM_TEMPLATE = \
"$D0WF/templates/simulation"

set STAND_TEMPLATE = \
"$D0WF/templates/standalone"

if ( ! -d "$SIM_TEMPLATE" ) then
    echo "ERROR: simulation template is missing:"
    echo "       $SIM_TEMPLATE"
    exit 7
endif

if ( ! -d "$STAND_TEMPLATE" ) then
    echo "ERROR: standalone template is missing:"
    echo "       $STAND_TEMPLATE"
    exit 8
endif

# ============================================================================
# New event-count-specific jobs
# ============================================================================

set SIMJOB = \
"$D0WF/work/jobs/st_physics_15130045_raw_1000011_${NEVENTS}evts_simulation_v1"

set STANDJOB = \
"$D0WF/work/jobs/st_physics_15130045_raw_1000011_${NEVENTS}evts_standalone_v1"

set SIMDRIVERS = "$SIMJOB/drivers"
set STANDDRIVERS = "$STANDJOB/drivers"

set PYTHIADIR = "$SIMJOB/pythia"
set MORETAGSDIR = "$SIMJOB/moretags"
set STARSIMDIR = "$SIMJOB/starsim"

set MIXDIR = "$STANDJOB/mixer"
set PICODIR = "$STANDJOB/pico"

mkdir -p "$SIMDRIVERS"
mkdir -p "$STANDDRIVERS"
mkdir -p "$PYTHIADIR"
mkdir -p "$MORETAGSDIR"
mkdir -p "$STARSIMDIR"
mkdir -p "$MIXDIR"
mkdir -p "$PICODIR"
mkdir -p "$STANDJOB/logs"
mkdir -p "$STANDJOB/snapshots"

set RUN_TAG = `date +%Y%m%d_%H%M%S`

set LOGDIR = \
"$STANDJOB/logs/run_standalone_production_v3_$RUN_TAG"

mkdir -p "$LOGDIR"

# ============================================================================
# Load the normal project environment in the wrapper process
# ============================================================================

echo "============================================================"
echo "Loading project environment"
echo "============================================================"
echo "D0WF = $D0WF"
echo

source "$SETUP_SCRIPT"

if ( $status != 0 ) then
    echo "ERROR: project environment setup failed"
    exit 9
endif

# All calls to checkRootTree_v1.C below use the macro name without
# its directory. Keep the wrapper in the directory containing it.
cd "$D0WF/scripts"

if ( $status != 0 ) then
    echo "ERROR: cannot enter scripts directory:"
    echo "       $D0WF/scripts"
    exit 9
endif

# ============================================================================
# Copy immutable inputs
# ============================================================================

set TEMPLATE_MUDST_LIST = \
"$SIM_TEMPLATE/mudst.selected.list"

set MUDST_LIST = \
"$SIMJOB/mudst.selected.list"

if ( ! -s "$TEMPLATE_MUDST_LIST" ) then
    echo "ERROR: template MuDst list is missing:"
    echo "       $TEMPLATE_MUDST_LIST"
    exit 10
endif

cp -p "$TEMPLATE_MUDST_LIST" "$MUDST_LIST"

set TEMPLATE_PYTHIA_SOURCE = \
"$SIM_TEMPLATE/pythia/make_d0_pythia8.cc"

set TEMPLATE_PYTHIA_CONFIG = \
"$SIM_TEMPLATE/pythia/pythia8_detroit_v1.cmnd"

set PYTHIA_SOURCE = \
"$PYTHIADIR/make_d0_pythia8.cc"

set PYTHIA_CONFIG = \
"$PYTHIADIR/pythia8_detroit_v1.cmnd"

if ( ! -s "$TEMPLATE_PYTHIA_SOURCE" ) then
    echo "ERROR: frozen PYTHIA source is missing:"
    echo "       $TEMPLATE_PYTHIA_SOURCE"
    exit 11
endif

if ( ! -s "$TEMPLATE_PYTHIA_CONFIG" ) then
    echo "ERROR: frozen PYTHIA configuration is missing:"
    echo "       $TEMPLATE_PYTHIA_CONFIG"
    exit 12
endif

cp -p "$TEMPLATE_PYTHIA_SOURCE" "$PYTHIA_SOURCE"
cp -p "$TEMPLATE_PYTHIA_CONFIG" "$PYTHIA_CONFIG"

# ============================================================================
# Build event-specific drivers from placeholder-based templates
# ============================================================================

set STAGE02_DRIVER = \
"$SIMDRIVERS/02_moretags.csh"

set STAGE04_DRIVER = \
"$SIMDRIVERS/04_starsim.csh"

set STAGE05_DRIVER = \
"$STANDDRIVERS/05_standalone_reco_v3.csh"

set STAGE06_DRIVER = \
"$STANDDRIVERS/06_pico.csh"

sed \
    -e "s|@D0WF@|$D0WF|g" \
    -e "s|@SIMJOB@|$SIMJOB|g" \
    "$SIM_TEMPLATE/drivers/02_moretags.csh" \
    >! "$STAGE02_DRIVER"

sed \
    -e "s|@D0WF@|$D0WF|g" \
    -e "s|@SIMJOB@|$SIMJOB|g" \
    -e "s|@NEVENTS@|$NEVENTS|g" \
    "$SIM_TEMPLATE/drivers/04_starsim.csh" \
    >! "$STAGE04_DRIVER"

sed \
    -e "s|@D0WF@|$D0WF|g" \
    -e "s|@SIMJOB@|$SIMJOB|g" \
    -e "s|@STANDJOB@|$STANDJOB|g" \
    -e "s|@NEVENTS@|$NEVENTS|g" \
    "$STAND_TEMPLATE/drivers/05_standalone_reco_v3.csh" \
    >! "$STAGE05_DRIVER"

sed \
    -e "s|@D0WF@|$D0WF|g" \
    -e "s|@STANDJOB@|$STANDJOB|g" \
    -e "s|@NEVENTS@|$NEVENTS|g" \
    "$STAND_TEMPLATE/drivers/06_pico.csh" \
    >! "$STAGE06_DRIVER"

chmod +x \
    "$STAGE02_DRIVER" \
    "$STAGE04_DRIVER" \
    "$STAGE05_DRIVER" \
    "$STAGE06_DRIVER"

# ============================================================================
# Validate generated driver contents before running anything destructive
# ============================================================================

foreach DRIVER ( \
    "$STAGE02_DRIVER" \
    "$STAGE04_DRIVER" \
    "$STAGE05_DRIVER" \
    "$STAGE06_DRIVER" )

    grep -q '@D0WF@\|@SIMJOB@\|@STANDJOB@\|@NEVENTS@' "$DRIVER"

    if ( $status == 0 ) then
        echo "ERROR: unresolved template placeholder in:"
        echo "       $DRIVER"
        exit 13
    endif
end

grep -q "$SIMJOB" "$STAGE02_DRIVER"
if ( $status != 0 ) then
    echo "ERROR: Stage 2 driver does not reference the simulation job"
    exit 14
endif

grep -q "$SIMJOB" "$STAGE04_DRIVER"
if ( $status != 0 ) then
    echo "ERROR: Stage 4 driver does not reference the simulation job"
    exit 15
endif

grep -q "$SIMJOB" "$STAGE05_DRIVER"
if ( $status != 0 ) then
    echo "ERROR: Stage 5 driver does not reference the simulation job"
    exit 16
endif

grep -q "$STANDJOB" "$STAGE05_DRIVER"
if ( $status != 0 ) then
    echo "ERROR: Stage 5 driver does not reference the standalone job"
    exit 17
endif

grep -q "set NEVENTS = $NEVENTS" "$STAGE05_DRIVER"
if ( $status != 0 ) then
    echo "ERROR: Stage 5 driver has the wrong event count"
    exit 18
endif

grep -q "$STANDJOB" "$STAGE06_DRIVER"
if ( $status != 0 ) then
    echo "ERROR: Stage 6 driver does not reference the standalone job"
    exit 19
endif

echo "Generated drivers validated:"
echo
grep -n -E \
'^set SIMJOB|^set WORK|^set NEVENTS' \
"$STAGE05_DRIVER"
echo

# ============================================================================
# Output names
# ============================================================================

set PYTHIA_EXE = \
"$PYTHIADIR/make_d0_pythia8"

set PYTHIA_OUTPUT = \
"$PYTHIADIR/pythia8_D0_events.root"

set MORETAGS_OUTPUT = \
"$MORETAGSDIR/st_physics_15130045_raw_1000011.moretags.root"

set CHOPPER_OUTPUT = \
"$MORETAGSDIR/st_physics_15130045_raw_1000011.chopper.txt"

set STARSIM_ROOT = \
"$STARSIMDIR/D0toy.starsim.root"

set STARSIM_FZD = \
"$STARSIMDIR/D0toy.starsim.fzd"

set MUDST_OUTPUT = \
"$MIXDIR/st_physics_15130045_raw_1000011.MuDst.root"

set PICO_OUTPUT = \
"$PICODIR/st_physics_15130045_raw_1000011.picoDst.root"

# ============================================================================
# Summary
# ============================================================================

echo "============================================================"
echo "Standalone production v3"
echo "============================================================"
echo "Events          : $NEVENTS"
echo "PYTHIA seed     : $PYTHIA_SEED"
echo "Start stage     : $START_STAGE"
echo "Force rerun     : $FORCE"
echo "Simulation job  : $SIMJOB"
echo "Standalone job  : $STANDJOB"
echo "Logs            : $LOGDIR"
echo
echo "Stage 1         : PYTHIA 8.303"
echo "Stage 2         : MoreTags"
echo "Stage 3         : SKIPPED"
echo "Stage 4         : STARSIM"
echo "Stage 5         : standalone reconstruction"
echo "Stage 6         : PicoDst"
echo "============================================================"
echo

# ============================================================================
# Stage 1: PYTHIA 8.303
# ============================================================================

if ( $START_STAGE <= 1 ) then

    echo "============================================================"
    echo "Stage 1: PYTHIA 8.303"
    echo "============================================================"

    set STAGE01_VALID = 0

    if ( -s "$PYTHIA_OUTPUT" && \
         $FORCE == 0 ) then

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$PYTHIA_OUTPUT"'","","",1)' \
        >&! "$LOGDIR/01_validate_existing_pythia.log"

        if ( $status == 0 ) then
            set STAGE01_VALID = 1
        endif
    endif

    if ( $STAGE01_VALID == 1 ) then
        echo "SKIP: valid PYTHIA output already exists:"
        echo "      $PYTHIA_OUTPUT"
    else
        set P8SRC = \
"$D0WF/external/pythia8_303"

        set P8XML = \
"$P8SRC/share/Pythia8/xmldoc"

        if ( ! -d "$P8SRC/include/Pythia8" ) then
            echo "ERROR: PYTHIA 8.303 headers are missing:"
            echo "       $P8SRC/include/Pythia8"
            exit 21
        endif

        if ( ! -d "$P8SRC/src" ) then
            echo "ERROR: PYTHIA 8.303 sources are missing:"
            echo "       $P8SRC/src"
            exit 22
        endif

        if ( ! -d "$P8XML" ) then
            echo "ERROR: PYTHIA XML directory is missing:"
            echo "       $P8XML"
            exit 23
        endif

        which root-config >& /dev/null

        if ( $status != 0 ) then
            echo "ERROR: root-config is unavailable"
            exit 24
        endif

        which g++ >& /dev/null

        if ( $status != 0 ) then
            echo "ERROR: g++ is unavailable"
            exit 25
        endif

        set ROOT_CONFIG = `which root-config`
        set CXX = `which g++`

        echo "Compiler       : $CXX"
        echo "root-config    : $ROOT_CONFIG"
        echo "PYTHIA source  : $P8SRC"
        echo "XML directory  : $P8XML"
        echo

        rm -f "$PYTHIA_EXE"

        $CXX \
            -O2 \
            -std=c++11 \
            -I"$P8SRC/include" \
            `$ROOT_CONFIG --cflags` \
            -DXMLDIR=\"${P8XML}\" \
            "$PYTHIA_SOURCE" \
            $P8SRC/src/*.cc \
            `$ROOT_CONFIG --libs` \
            -ldl \
            -o "$PYTHIA_EXE" \
            >&! "$LOGDIR/01_pythia_compile.log"

        set COMPILE_STATUS = $status

        if ( $COMPILE_STATUS != 0 ) then
            echo "ERROR: PYTHIA compilation failed"
            echo
            tail -100 "$LOGDIR/01_pythia_compile.log"
            exit 26
        endif

        if ( ! -x "$PYTHIA_EXE" ) then
            echo "ERROR: compiled PYTHIA executable is missing"
            exit 27
        endif

        rm -f "$PYTHIA_OUTPUT"

        # The project setup may leave the standalone PYTHIA 8.317
        # environment active.  The executable compiled above contains
        # PYTHIA 8.303 code and must use matching 8.303 XML data.
        if ( $?PYTHIA8 ) then
            echo "Previous PYTHIA8     = $PYTHIA8"
            unsetenv PYTHIA8
        endif

        if ( $?PYTHIA8DATA ) then
            echo "Previous PYTHIA8DATA = $PYTHIA8DATA"
            unsetenv PYTHIA8DATA
        endif

        if ( ! -f "$P8XML/Version.xml" ) then
            echo "ERROR: PYTHIA Version.xml is missing:"
            echo "       $P8XML/Version.xml"
            exit 28
        endif

        grep -q "8.303" "$P8XML/Version.xml"

        if ( $status != 0 ) then
            echo "ERROR: selected XML directory is not PYTHIA 8.303:"
            echo "       $P8XML"
            echo
            grep -n "versionNumber" "$P8XML/Version.xml"
            exit 28
        endif

        setenv PYTHIA8 "$P8SRC"
        setenv PYTHIA8DATA "$P8XML"

        echo "Runtime PYTHIA8     = $PYTHIA8"
        echo "Runtime PYTHIA8DATA = $PYTHIA8DATA"
        echo "Resolved XML path   = `readlink -f "$PYTHIA8DATA"`"
        echo

        "$PYTHIA_EXE" \
            "$NEVENTS" \
            "$PYTHIA_SEED" \
            "$PYTHIA_CONFIG" \
            "$PYTHIA_OUTPUT" \
            >&! "$LOGDIR/01_pythia_run.log"

        set PYTHIA_STATUS = $status

        if ( $PYTHIA_STATUS != 0 ) then
            echo "ERROR: PYTHIA run failed with status $PYTHIA_STATUS"
            echo
            tail -100 "$LOGDIR/01_pythia_run.log"
            exit 28
        endif

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$PYTHIA_OUTPUT"'","","",1)' \
        >&! "$LOGDIR/01_validate_pythia.log"

        if ( $status != 0 ) then
            echo "ERROR: PYTHIA output validation failed"
            cat "$LOGDIR/01_validate_pythia.log"
            exit 29
        endif

        echo "Stage 1 completed:"
        ls -lh "$PYTHIA_OUTPUT"
    endif

    echo
endif

# ============================================================================
# Validate PYTHIA input for subsequent stages
# ============================================================================

if ( ! -s "$PYTHIA_OUTPUT" ) then
    echo "ERROR: PYTHIA output is unavailable:"
    echo "       $PYTHIA_OUTPUT"
    exit 30
endif

# ============================================================================
# Stage 2: Event tags / MoreTags
# ============================================================================

if ( $START_STAGE <= 2 ) then

    echo "============================================================"
    echo "Stage 2: Event tags / MoreTags"
    echo "============================================================"

    set STAGE02_VALID = 0

    if ( -s "$MORETAGS_OUTPUT" && \
         -s "$CHOPPER_OUTPUT" && \
         $FORCE == 0 ) then

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$MORETAGS_OUTPUT"'","","",1)' \
        >&! "$LOGDIR/02_validate_existing_moretags.log"

        set ROOT_CHECK_STATUS = $status

        set CHOPPER_LINES = \
`wc -l "$CHOPPER_OUTPUT" | awk '{print $1}'`

        if ( $ROOT_CHECK_STATUS == 0 && \
             $CHOPPER_LINES >= $NEVENTS ) then

            set STAGE02_VALID = 1
        endif
    endif

    if ( $STAGE02_VALID == 1 ) then
        echo "SKIP: valid MoreTags outputs already exist"
        echo "      $MORETAGS_OUTPUT"
        echo "      $CHOPPER_OUTPUT"
    else
        tcsh "$STAGE02_DRIVER" \
            >&! "$LOGDIR/02_moretags.log"

        set STAGE02_STATUS = $status

        set STAGE02_OUTPUT_VALID = 0

        if ( -s "$MORETAGS_OUTPUT" && \
             -s "$CHOPPER_OUTPUT" ) then

            root4star -l -b -q \
            'checkRootTree_v1.C("'"$MORETAGS_OUTPUT"'","","",1)' \
            >&! "$LOGDIR/02_validate_moretags.log"

            set ROOT_CHECK_STATUS = $status

            set CHOPPER_LINES = \
`wc -l "$CHOPPER_OUTPUT" | awk '{print $1}'`

            if ( $ROOT_CHECK_STATUS == 0 && \
                 $CHOPPER_LINES >= $NEVENTS ) then

                set STAGE02_OUTPUT_VALID = 1
            endif
        endif

        if ( $STAGE02_OUTPUT_VALID == 0 ) then
            echo "ERROR: MoreTags outputs are missing or invalid"
            echo
            tail -120 "$LOGDIR/02_moretags.log"
            exit 31
        endif

        if ( $STAGE02_STATUS != 0 ) then

            grep -q \
                "This is the end of ROOT" \
                "$LOGDIR/02_moretags.log"

            set NORMAL_ROOT_END = $status

            if ( $NORMAL_ROOT_END == 0 ) then
                echo "WARNING: Event-tags process returned status"
                echo "         $STAGE02_STATUS after valid outputs"
                echo "         and normal ROOT completion."
                echo "         Accepted as known cleanup crash."
            else
                echo "ERROR: Event tags returned non-zero status"
                echo "       without a normal ROOT completion marker."
                echo
                tail -120 "$LOGDIR/02_moretags.log"
                exit 32
            endif
        else
            echo "Stage 2 completed normally."
        endif

        echo "MoreTags output:"
        ls -lh "$MORETAGS_OUTPUT"

        echo "Chopper entries:"
        wc -l "$CHOPPER_OUTPUT"
    endif

    echo
endif

# ============================================================================
# Validate MoreTags input for STARSIM
# ============================================================================

if ( ! -s "$MORETAGS_OUTPUT" ) then
    echo "ERROR: MoreTags file is unavailable:"
    echo "       $MORETAGS_OUTPUT"
    exit 33
endif

echo "============================================================"
echo "Stage 3: Event DAQ / Chopper"
echo "============================================================"
echo "SKIPPED in standalone production."
echo

# ============================================================================
# Stage 4: STARSIM / GEANT3
# ============================================================================

if ( $START_STAGE <= 4 ) then

    echo "============================================================"
    echo "Stage 4: STARSIM / GEANT3"
    echo "============================================================"

    set STAGE04_VALID = 0

    if ( -s "$STARSIM_ROOT" && \
         -s "$STARSIM_FZD" && \
         $FORCE == 0 ) then

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$STARSIM_ROOT"'","","",1)' \
        >&! "$LOGDIR/04_validate_existing_starsim.log"

        if ( $status == 0 ) then
            set STAGE04_VALID = 1
        endif
    endif

    if ( $STAGE04_VALID == 1 ) then
        echo "SKIP: valid STARSIM output already exists:"
        echo "      $STARSIM_FZD"
    else
        tcsh "$STAGE04_DRIVER" \
            >&! "$LOGDIR/04_starsim.log"

        set STAGE04_STATUS = $status

        if ( $STAGE04_STATUS != 0 ) then
            echo "ERROR: STARSIM failed with status $STAGE04_STATUS"
            echo
            tail -120 "$LOGDIR/04_starsim.log"
            exit 41
        endif

        if ( ! -s "$STARSIM_FZD" ) then
            echo "ERROR: STARSIM produced no valid FZD"
            echo
            tail -120 "$LOGDIR/04_starsim.log"
            exit 42
        endif

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$STARSIM_ROOT"'","","",1)' \
        >&! "$LOGDIR/04_validate_starsim.log"

        if ( $status != 0 ) then
            echo "ERROR: STARSIM ROOT output validation failed"
            cat "$LOGDIR/04_validate_starsim.log"
            exit 43
        endif

        echo "Stage 4 completed:"
        ls -lh "$STARSIM_ROOT" "$STARSIM_FZD"
    endif

    echo
endif

# ============================================================================
# Validate FZD input for standalone reconstruction
# ============================================================================

if ( ! -s "$STARSIM_FZD" ) then
    echo "ERROR: STARSIM FZD is unavailable:"
    echo "       $STARSIM_FZD"
    exit 44
endif

# ============================================================================
# Stage 5: standalone FZD -> MuDst
# ============================================================================

if ( $START_STAGE <= 5 ) then

    echo "============================================================"
    echo "Stage 5: standalone FZD -> MuDst"
    echo "============================================================"

    set STAGE05_VALID = 0

    if ( -s "$MUDST_OUTPUT" && \
         $FORCE == 0 ) then

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$MUDST_OUTPUT"'","","",1)' \
        >&! "$LOGDIR/05_validate_existing_mudst.log"

        if ( $status == 0 ) then
            set STAGE05_VALID = 1
        endif
    endif

    if ( $STAGE05_VALID == 1 ) then
        echo "SKIP: valid standalone MuDst already exists:"
        echo "      $MUDST_OUTPUT"
    else
        tcsh "$STAGE05_DRIVER" \
            >&! "$LOGDIR/05_standalone_reco.log"

        set STAGE05_STATUS = $status

        if ( $STAGE05_STATUS != 0 ) then
            echo "ERROR: standalone reconstruction failed"
            echo "       status = $STAGE05_STATUS"
            echo
            tail -120 "$LOGDIR/05_standalone_reco.log"

            if ( -f "$STANDJOB/logs/05_standalone_reco_v3.root4star.log" ) then
                echo
                tail -120 \
"$STANDJOB/logs/05_standalone_reco_v3.root4star.log"
            endif

            exit 51
        endif

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$MUDST_OUTPUT"'","","",1)' \
        >&! "$LOGDIR/05_validate_mudst.log"

        if ( $status != 0 ) then
            echo "ERROR: standalone MuDst validation failed"
            cat "$LOGDIR/05_validate_mudst.log"
            exit 52
        endif

        echo "Stage 5 completed:"
        ls -lh "$MUDST_OUTPUT"
    endif

    echo
endif

# ============================================================================
# Validate MuDst input for PicoDst
# ============================================================================

if ( ! -s "$MUDST_OUTPUT" ) then
    echo "ERROR: standalone MuDst is unavailable:"
    echo "       $MUDST_OUTPUT"
    exit 53
endif

# ============================================================================
# Stage 6: MuDst -> PicoDst
# ============================================================================

if ( $START_STAGE <= 6 ) then

    echo "============================================================"
    echo "Stage 6: MuDst -> PicoDst"
    echo "============================================================"

    set STAGE06_VALID = 0

    if ( -s "$PICO_OUTPUT" && \
         $FORCE == 0 ) then

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$PICO_OUTPUT"'","PicoDst","McTrack.mId",1)' \
        >&! "$LOGDIR/06_validate_existing_pico.log"

        if ( $status == 0 ) then
            set STAGE06_VALID = 1
        endif
    endif

    if ( $STAGE06_VALID == 1 ) then
        echo "SKIP: valid standalone PicoDst already exists:"
        echo "      $PICO_OUTPUT"
    else
        tcsh "$STAGE06_DRIVER" \
            >&! "$LOGDIR/06_pico.log"

        set STAGE06_STATUS = $status

        if ( $STAGE06_STATUS != 0 ) then
            echo "ERROR: PicoDst production failed"
            echo "       status = $STAGE06_STATUS"
            echo
            tail -150 "$LOGDIR/06_pico.log"
            exit 61
        endif

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$PICO_OUTPUT"'","PicoDst","McTrack.mId",1)' \
        >&! "$LOGDIR/06_validate_pico.log"

        if ( $status != 0 ) then
            echo "ERROR: PicoDst validation failed"
            cat "$LOGDIR/06_validate_pico.log"
            exit 62
        endif

        echo "Stage 6 completed:"
        ls -lh "$PICO_OUTPUT"
    endif

    echo
endif

# ============================================================================
# Final summary
# ============================================================================

echo "============================================================"
echo "Standalone production completed"
echo "============================================================"
echo "Events:"
echo "    $NEVENTS"
echo
echo "PYTHIA:"
ls -lh "$PYTHIA_OUTPUT"
echo
echo "MoreTags:"
ls -lh "$MORETAGS_OUTPUT"
echo
echo "STARSIM FZD:"
ls -lh "$STARSIM_FZD"
echo
echo "Standalone MuDst:"
ls -lh "$MUDST_OUTPUT"
echo
echo "Standalone PicoDst:"
ls -lh "$PICO_OUTPUT"
echo
echo "Logs:"
echo "    $LOGDIR"
echo
echo "Finished:"
date

exit 0
