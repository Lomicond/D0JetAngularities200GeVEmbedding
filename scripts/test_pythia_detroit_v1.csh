#!/bin/tcsh -f

set nonomatch

############################################################
# Project
############################################################

if ($?D0WF) then
    set PROJECT = "$D0WF"
else
    set PROJECT = "$cwd"
endif

set PYTHIA_HOME = "$PROJECT/external/pythia8_303"
set CONFIG      = "$PROJECT/config/pythia8_detroit_v1.cmnd"
set SOURCE      = "$PROJECT/src/pythia/make_d0_pythia8.cc"

set TESTDIR = "$PROJECT/work/pythia_detroit_validation_v1"
set BIN     = "$TESTDIR/make_d0_pythia8"
set OUTPUT  = "$TESTDIR/pythia8_D0_events.root"
set LOG     = "$TESTDIR/pythia8_D0_events.log"

set NEVENTS = 1
set SEED    = 1000011

############################################################
# Validate inputs
############################################################

if (! -d "$PYTHIA_HOME") then
    echo "ERROR: PYTHIA 8.303 installation is missing:"
    echo "       $PYTHIA_HOME"
    exit 2
endif

if (! -x "$PYTHIA_HOME/bin/pythia8-config") then
    echo "ERROR: pythia8-config is missing:"
    echo "       $PYTHIA_HOME/bin/pythia8-config"
    exit 3
endif

if (! -f "$CONFIG") then
    echo "ERROR: Detroit configuration is missing:"
    echo "       $CONFIG"
    exit 4
endif

if (! -f "$SOURCE") then
    echo "ERROR: generator source is missing:"
    echo "       $SOURCE"
    exit 5
endif

set ROOT_CONFIG = `which root-config`

if ("$ROOT_CONFIG" == "") then
    echo "ERROR: root-config was not found"
    exit 6
endif

############################################################
# Configure standalone PYTHIA 8.303
############################################################

unsetenv PYTHIA8
unsetenv PYTHIA8DATA

setenv PYTHIA8 "$PYTHIA_HOME"
setenv PYTHIA8DATA "$PYTHIA_HOME/share/Pythia8/xmldoc"

setenv PATH \
"${PYTHIA_HOME}/bin:${PATH}"

if ($?LD_LIBRARY_PATH) then
    setenv LD_LIBRARY_PATH \
"${PYTHIA_HOME}/lib:${LD_LIBRARY_PATH}"
else
    setenv LD_LIBRARY_PATH \
"${PYTHIA_HOME}/lib"
endif

rehash

set PYTHIA_CONFIG = "$PYTHIA_HOME/bin/pythia8-config"

############################################################
# Build
############################################################

mkdir -p "$TESTDIR"

echo "======================================================"
echo "PYTHIA Detroit validation"
echo "======================================================"
echo "Project       = $PROJECT"
echo "PYTHIA_HOME   = $PYTHIA_HOME"
echo "PYTHIA8DATA   = $PYTHIA8DATA"
echo "Config        = $CONFIG"
echo "Source        = $SOURCE"
echo "root-config   = $ROOT_CONFIG"
echo "pythia8-config = $PYTHIA_CONFIG"
echo "======================================================"

rm -f "$BIN"

g++ -O2 -std=c++11 \
    "$SOURCE" \
    -o "$BIN" \
    `$PYTHIA_CONFIG --cxxflags` \
    `$ROOT_CONFIG --cflags` \
    `$PYTHIA_CONFIG --libs` \
    `$ROOT_CONFIG --libs`

set BUILD_STATUS = $status

if ($BUILD_STATUS != 0) then
    echo "ERROR: generator compilation failed"
    exit 11
endif

############################################################
# Generate one accepted D0 event
############################################################

rm -f "$OUTPUT"
rm -f "$LOG"

"$BIN" \
    "$NEVENTS" \
    "$SEED" \
    "$CONFIG" \
    "$OUTPUT" \
    >&! "$LOG"

set RUN_STATUS = $status

if ($RUN_STATUS != 0) then
    echo "ERROR: PYTHIA generation failed"
    echo "       status = $RUN_STATUS"
    tail -100 "$LOG"
    exit 12
endif

if (! -s "$OUTPUT") then
    echo "ERROR: PYTHIA output is missing or empty:"
    echo "       $OUTPUT"
    exit 13
endif

############################################################
# Summary
############################################################

echo
echo "===== DETROIT SETTINGS FOUND IN LOG ====="

grep -E \
'Tune:ee|Tune:pp|PDF:pSet|MultipartonInteractions:bProfile|MultipartonInteractions:ecmRef|MultipartonInteractions:pT0Ref|MultipartonInteractions:ecmPow|MultipartonInteractions:coreRadius|MultipartonInteractions:coreFraction|ColourReconnection:range' \
"$LOG"

echo
echo "===== OUTPUT ====="
ls -lh "$OUTPUT"

echo
echo "===== LOG ====="
echo "$LOG"

echo
echo "PYTHIA Detroit validation completed successfully."

exit 0
