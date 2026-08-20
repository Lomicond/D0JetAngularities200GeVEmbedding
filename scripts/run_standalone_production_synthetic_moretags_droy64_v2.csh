#!/bin/tcsh -f
#
# run_standalone_production_synthetic_moretags_droy64_v2.csh
#
# Standalone D0 MC chain generating a job-specific synthetic MoreTags file:
#
#   1 PYTHIA 8.303
#   2 Synthetic MoreTags generation and validation
#   3 SKIPPED: DAQ chopper
#   4 STARSIM / GEANT3
#   5 standalone FZD -> MuDst reconstruction
#   6 MuDst -> PicoDst
#
# Default test:
#
#   tcsh scripts/run_standalone_production_synthetic_moretags_droy64_v2.csh
#
# Arguments:
#
#   argv[1] = number of events, default 5
#   argv[2] = start stage, default 1
#   argv[3] = force rerun, default 0
#   argv[4] = PYTHIA seed, default 1000011
#   argv[5] = synthetic-MoreTags seed, default 2000011
#   argv[6] = first synthetic event ID, default 1000011
#   argv[7] = D0-decayer seed, default 3000011
#   argv[8] = first GEANT seed, default 4000011
#   argv[9] = second GEANT seed, default 5000011
#
# Examples:
#
#   Fresh 5-event run (new output directories; resume if already present):
#       tcsh scripts/run_standalone_production_synthetic_moretags_droy64_v2.csh \
#           5 1 0 1000011 2000011 1000011 3000011 4000011 5000011
#
#   Force a complete fresh 5-event rerun:
#       tcsh scripts/run_standalone_production_synthetic_moretags_droy64_v2.csh \
#           5 1 1 1000011 2000011 1000011 3000011 4000011 5000011
#
#   Resume from STARSIM:
#       tcsh scripts/run_standalone_production_synthetic_moretags_droy64_v2.csh \
#           5 4 0 1000011 2000011 1000011 3000011 4000011 5000011
#
#   Force only PicoDst:
#       tcsh scripts/run_standalone_production_synthetic_moretags_droy64_v2.csh \
#           5 6 1 1000011 2000011 1000011 3000011 4000011 5000011
#

umask 002
set nonomatch

# ============================================================================
# Basic configuration
# ============================================================================

if ( ! $?D0WF ) then
    set SCRIPT_DIR = `dirname "$0"`
    set SCRIPT_DIR = `readlink -f "$SCRIPT_DIR"`
    setenv D0WF `dirname "$SCRIPT_DIR"`
else
    setenv D0WF `readlink -f "$D0WF"`
endif

set NEVENTS = 5
set START_STAGE = 1
set FORCE = 0
set PYTHIA_SEED = 1000011
set MORETAGS_SEED = 2000011
set FIRST_EVT_ID = 1000011
set D0_DECAYER_SEED = 3000011
set GEANT_SEED1 = 4000011
set GEANT_SEED2 = 5000011

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

if ( $#argv >= 5 ) then
    set MORETAGS_SEED = "$argv[5]"
endif

if ( $#argv >= 6 ) then
    set FIRST_EVT_ID = "$argv[6]"
endif

if ( $#argv >= 7 ) then
    set D0_DECAYER_SEED = "$argv[7]"
endif

if ( $#argv >= 8 ) then
    set GEANT_SEED1 = "$argv[8]"
endif

if ( $#argv >= 9 ) then
    set GEANT_SEED2 = "$argv[9]"
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

if ( $PYTHIA_SEED <= 0 || $PYTHIA_SEED > 900000000 ) then
    echo "ERROR: PYTHIA_SEED must be in 1--900000000"
    exit 4
endif

if ( $MORETAGS_SEED <= 0 || $MORETAGS_SEED > 900000000 ) then
    echo "ERROR: MORETAGS_SEED must be in 1--900000000"
    exit 5
endif

if ( $PYTHIA_SEED == $MORETAGS_SEED ) then
    echo "ERROR: PYTHIA_SEED and MORETAGS_SEED must differ"
    exit 6
endif

if ( $FIRST_EVT_ID <= 0 ) then
    echo "ERROR: FIRST_EVT_ID must be positive"
    exit 7
endif

if ( $D0_DECAYER_SEED <= 0 || $D0_DECAYER_SEED > 900000000 ) then
    echo "ERROR: D0_DECAYER_SEED must be in 1--900000000"
    exit 8
endif

if ( $GEANT_SEED1 <= 0 || $GEANT_SEED1 > 900000000 || \
     $GEANT_SEED2 <= 0 || $GEANT_SEED2 > 900000000 ) then
    echo "ERROR: both GEANT seeds must be in 1--900000000"
    exit 8
endif

if ( $PYTHIA_SEED == $MORETAGS_SEED || \
     $PYTHIA_SEED == $D0_DECAYER_SEED || \
     $PYTHIA_SEED == $GEANT_SEED1 || \
     $PYTHIA_SEED == $GEANT_SEED2 || \
     $MORETAGS_SEED == $D0_DECAYER_SEED || \
     $MORETAGS_SEED == $GEANT_SEED1 || \
     $MORETAGS_SEED == $GEANT_SEED2 || \
     $D0_DECAYER_SEED == $GEANT_SEED1 || \
     $D0_DECAYER_SEED == $GEANT_SEED2 || \
     $GEANT_SEED1 == $GEANT_SEED2 ) then
    echo "ERROR: all production seeds must differ"
    exit 8
endif

if ( ! -d "$D0WF" ) then
    echo "ERROR: D0WF does not exist:"
    echo "       $D0WF"
    exit 8
endif

set SETUP_SCRIPT = \
"$D0WF/scripts/setup_SL16d2_D0Embedding_hybrid.csh"

set CHECK_MACRO = \
"$D0WF/scripts/checkRootTree_v1.C"

set SYNTHETIC_MORETAGS_MACRO = \
"$D0WF/src/moretags/makeSyntheticMoreTags_v1.C"

set VERTEX_DISTRIBUTIONS = \
"$D0WF/src/moretags/Run14_AuAu200GeV_vertexDistributions.root"

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

if ( ! -f "$SYNTHETIC_MORETAGS_MACRO" ) then
    echo "ERROR: synthetic MoreTags macro is missing:"
    echo "       $SYNTHETIC_MORETAGS_MACRO"
    exit 9
endif

if ( ! -s "$VERTEX_DISTRIBUTIONS" ) then
    echo "ERROR: vertex-distribution file is missing:"
    echo "       $VERTEX_DISTRIBUTIONS"
    exit 10
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
"$D0WF/work/jobs/st_physics_15130045_raw_${FIRST_EVT_ID}_${NEVENTS}evts_seed${PYTHIA_SEED}_simulation_synthetic_moretags_v2"

set STANDJOB = \
"$D0WF/work/jobs/st_physics_15130045_raw_${FIRST_EVT_ID}_${NEVENTS}evts_seed${PYTHIA_SEED}_standalone_synthetic_moretags_droy64_v2"

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
"$STANDJOB/logs/run_standalone_production_synthetic_moretags_droy64_v2_$RUN_TAG"

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

set STAGE04_DRIVER = \
"$SIMDRIVERS/04_starsim.csh"

set STAGE05_DRIVER = \
"$STANDDRIVERS/05_standalone_reco_droy64_v1.csh"

set STAGE06_DRIVER = \
"$STANDDRIVERS/06_pico.csh"

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
    "$STAND_TEMPLATE/drivers/05_standalone_reco_droy64_v1.csh" \
    >! "$STAGE05_DRIVER"

sed \
    -e "s|@D0WF@|$D0WF|g" \
    -e "s|@STANDJOB@|$STANDJOB|g" \
    -e "s|@NEVENTS@|$NEVENTS|g" \
    "$STAND_TEMPLATE/drivers/06_pico.csh" \
    >! "$STAGE06_DRIVER"

chmod +x \
    "$STAGE04_DRIVER" \
    "$STAGE05_DRIVER" \
    "$STAGE06_DRIVER"

# ============================================================================
# Validate generated driver contents before running anything destructive
# ============================================================================

foreach DRIVER ( \
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
echo "Standalone production with generated synthetic MoreTags v2"
echo "============================================================"
echo "Events          : $NEVENTS"
echo "PYTHIA seed     : $PYTHIA_SEED"
echo "MoreTags seed   : $MORETAGS_SEED"
echo "D0-decayer seed : $D0_DECAYER_SEED"
echo "GEANT seeds     : $GEANT_SEED1, $GEANT_SEED2"
echo "First event ID  : $FIRST_EVT_ID"
echo "Start stage     : $START_STAGE"
echo "Force rerun     : $FORCE"
echo "Vertex input    : $VERTEX_DISTRIBUTIONS"
echo "Simulation job  : $SIMJOB"
echo "Standalone job  : $STANDJOB"
echo "Logs            : $LOGDIR"
echo
echo "Stage 1         : PYTHIA 8.303"
echo "Stage 2         : generate synthetic MoreTags"
echo "Stage 3         : SKIPPED (DAQ/chopper)"
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
# Stage 2: Synthetic MoreTags generation and validation
# ============================================================================

if ( $START_STAGE <= 2 ) then

    echo "============================================================"
    echo "Stage 2: Generate synthetic MoreTags"
    echo "============================================================"

    set STAGE02_VALID = 0

    if ( -s "$MORETAGS_OUTPUT" && \
         $FORCE == 0 ) then

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$MORETAGS_OUTPUT"'","","",1)' \
        >&! "$LOGDIR/02_validate_existing_synthetic_moretags.log"

        if ( $status == 0 ) then
            set STAGE02_VALID = 1
        endif
    endif

    if ( $STAGE02_VALID == 1 ) then
        echo "SKIP: valid synthetic MoreTags output already exists:"
        echo "      $MORETAGS_OUTPUT"
    else
        rm -f "$MORETAGS_OUTPUT"

        set SYNTHETIC_MORETAGS_DRIVER = \
"$SIMDRIVERS/makeSyntheticMoreTags_job.C"

        # Keep nested ROOT/C++ quotes completely out of the tcsh command line.
        # The here-document expands the job-specific paths and seeds into a
        # small ROOT driver.  root4star then receives only the driver filename.
        cat >! "$SYNTHETIC_MORETAGS_DRIVER" << ROOTEOF
#include "$SYNTHETIC_MORETAGS_MACRO"

void makeSyntheticMoreTags_job()
{
    makeSyntheticMoreTags_v1(
        "$PYTHIA_OUTPUT",
        "$VERTEX_DISTRIBUTIONS",
        "$MORETAGS_OUTPUT",
        $FIRST_EVT_ID,
        $MORETAGS_SEED
    );
}
ROOTEOF

        if ( ! -s "$SYNTHETIC_MORETAGS_DRIVER" ) then
            echo "ERROR: failed to create synthetic MoreTags ROOT driver"
            echo "       $SYNTHETIC_MORETAGS_DRIVER"
            exit 31
        endif

        echo "Synthetic MoreTags driver:"
        echo "      $SYNTHETIC_MORETAGS_DRIVER"

        root4star -l -b -q "$SYNTHETIC_MORETAGS_DRIVER" \
            >&! "$LOGDIR/02_generate_synthetic_moretags.log"

        set STAGE02_STATUS = $status

        if ( $STAGE02_STATUS != 0 || ! -s "$MORETAGS_OUTPUT" ) then
            echo "ERROR: synthetic MoreTags generation failed"
            echo "       status: $STAGE02_STATUS"
            echo "       output: $MORETAGS_OUTPUT"
            echo
            tail -120 "$LOGDIR/02_generate_synthetic_moretags.log"
            exit 31
        endif

        root4star -l -b -q \
        'checkRootTree_v1.C("'"$MORETAGS_OUTPUT"'","","",1)' \
        >&! "$LOGDIR/02_validate_generated_synthetic_moretags.log"

        if ( $status != 0 ) then
            echo "ERROR: generated synthetic MoreTags failed validation:"
            echo "       $MORETAGS_OUTPUT"
            echo
            cat "$LOGDIR/02_validate_generated_synthetic_moretags.log"
            exit 32
        endif

        echo "Synthetic MoreTags generated:"
        echo "      output: $MORETAGS_OUTPUT"
        echo "      first EvtId: $FIRST_EVT_ID"
        echo "      random seed: $MORETAGS_SEED"
        ls -lh "$MORETAGS_OUTPUT"
    endif

    echo
endif

# ============================================================================
# Validate MoreTags input for STARSIM
# ============================================================================

if ( ! -s "$MORETAGS_OUTPUT" ) then
    echo "ERROR: MoreTags file is unavailable:"
    echo "       $MORETAGS_OUTPUT"
    exit 35
endif

echo "============================================================"
echo "Stage 3: Event DAQ / Chopper"
echo "============================================================"
echo "SKIPPED in standalone production."
echo

# ============================================================================
# Stage 4: STARSIM / GEANT3
# ============================================================================

setenv D0WF_D0_DECAYER_SEED "$D0_DECAYER_SEED"
setenv D0WF_GEANT_SEED1 "$GEANT_SEED1"
setenv D0WF_GEANT_SEED2 "$GEANT_SEED2"

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

            if ( -f "$STANDJOB/logs/05_standalone_reco_droy64_v1.root4star.log" ) then
                echo
                tail -120 \
"$STANDJOB/logs/05_standalone_reco_droy64_v1.root4star.log"
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
echo "Standalone production with generated synthetic MoreTags completed"
echo "============================================================"
echo "Events:"
echo "    $NEVENTS"
echo
echo "Seeds and event IDs:"
echo "    PYTHIA seed   = $PYTHIA_SEED"
echo "    MoreTags seed = $MORETAGS_SEED"
echo "    D0-decayer seed = $D0_DECAYER_SEED"
echo "    GEANT seeds     = $GEANT_SEED1, $GEANT_SEED2"
echo "    first EvtId   = $FIRST_EVT_ID"
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
