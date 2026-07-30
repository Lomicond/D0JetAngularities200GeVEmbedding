#!/bin/tcsh -f

set nonomatch

setenv D0WF @D0WF@

set SIMJOB = @SIMJOB@
set WORK   = @STANDJOB@
set NEVENTS = @NEVENTS@

set RUNBFC_SRC = "$D0WF/src/standalone/runBfc.C"
set LOCAL_SETUP = \
"$D0WF/scripts/setup_SL16d2_D0Embedding_hybrid_v3.csh"

set MIXDIR = "$WORK/mixer"
set LOGDIR = "$WORK/logs"
set SNAPDIR = "$WORK/snapshots"

mkdir -p "$MIXDIR"
mkdir -p "$LOGDIR"
mkdir -p "$SNAPDIR"

############################################################
# Validate local reconstruction assets
############################################################

if (! -f "$RUNBFC_SRC") then
    echo "ERROR: local standalone reconstruction macro is missing:"
    echo "       $RUNBFC_SRC"
    exit 2
endif

if (! -f "$LOCAL_SETUP") then
    echo "ERROR: local SL16d setup is missing:"
    echo "       $LOCAL_SETUP"
    exit 3
endif

if (! -d "$D0WF/local_SL16d_embed2_facade") then
    echo "ERROR: local SL16d facade is missing:"
    echo "       $D0WF/local_SL16d_embed2_facade"
    exit 4
endif

if (! -x "$D0WF/local_sl16d_bin/root4star") then
    echo "ERROR: local root4star wrapper is missing or not executable:"
    echo "       $D0WF/local_sl16d_bin/root4star"
    exit 5
endif

############################################################
# Locate STARSIM FZD
############################################################

set FZD = `find "$SIMJOB" \
    -type f \
    -name 'D0toy.starsim.fzd' | head -1`

if ("$FZD" == "") then
    set FZD = `find "$SIMJOB" \
        -type f \
        -name '*.fzd' \
        ! -name 'kinematics*' | head -1`
endif

if ("$FZD" == "") then
    echo "ERROR: no STARSIM FZD was found in:"
    echo "       $SIMJOB"
    exit 11
endif

if (! -e "$FZD") then
    echo "ERROR: FZD does not exist:"
    echo "       $FZD"
    exit 12
endif

echo "======================================================"
echo "Standalone reconstruction v3"
echo "Date       = `date`"
echo "Host       = `hostname`"
echo "FZD        = $FZD"
echo "NEVENTS    = $NEVENTS"
echo "WORK       = $WORK"
echo "RUNBFC_SRC = $RUNBFC_SRC"
echo "======================================================"

############################################################
# Clean and prepare output directory
############################################################

cd "$MIXDIR"

rm -f runBfc.C
rm -f st_physics_15130045_raw_1000011.fzd
rm -f *.MuDst.root
rm -f *.event.root
rm -f *.hist.root
rm -f *.geant.root

cp -p "$RUNBFC_SRC" runBfc.C
ln -s "$FZD" st_physics_15130045_raw_1000011.fzd

############################################################
# Enter the project-local standalone reconstruction environment
############################################################

if ($?ROOTSYS) unsetenv ROOTSYS
if ($?LD_LIBRARY_PATH) unsetenv LD_LIBRARY_PATH
if ($?ROOT_INCLUDE_PATH) unsetenv ROOT_INCLUDE_PATH
if ($?ROOT_PLUGIN_PATH) unsetenv ROOT_PLUGIN_PATH
if ($?ROOTMAP_PATH) unsetenv ROOTMAP_PATH
if ($?STAR) unsetenv STAR
if ($?STAR_LIB) unsetenv STAR_LIB
if ($?STAR_BIN) unsetenv STAR_BIN
if ($?QTROOTSYSDIR) unsetenv QTROOTSYSDIR

source "$LOCAL_SETUP"

if (! $?STAR_LEVEL) then
    echo "ERROR: STAR_LEVEL was not configured by:"
    echo "       $LOCAL_SETUP"
    exit 13
endif

if (! $?ROOTSYS) then
    echo "ERROR: ROOTSYS was not configured by:"
    echo "       $LOCAL_SETUP"
    exit 14
endif

set ROOT4STAR = `which root4star`

if ("$ROOT4STAR" == "") then
    echo "ERROR: root4star is unavailable after local setup"
    exit 15
endif

############################################################
# Save and audit environment snapshot
############################################################

set SNAPSHOT = \
"$SNAPDIR/05_standalone_environment_v3.txt"

echo "Date: `date`" >! "$SNAPSHOT"
echo "Host: `hostname`" >> "$SNAPSHOT"
echo "STAR: $STAR" >> "$SNAPSHOT"
echo "STAR_LEVEL: $STAR_LEVEL" >> "$SNAPSHOT"
echo "ROOT_LEVEL: $ROOT_LEVEL" >> "$SNAPSHOT"
echo "ROOTSYS: $ROOTSYS" >> "$SNAPSHOT"
echo "STAR_HOST_SYS: $STAR_HOST_SYS" >> "$SNAPSHOT"
echo "root4star: $ROOT4STAR" >> "$SNAPSHOT"
echo "FZD: $FZD" >> "$SNAPSHOT"
echo "RUNBFC_SRC: $RUNBFC_SRC" >> "$SNAPSHOT"
echo "LOCAL_SETUP: $LOCAL_SETUP" >> "$SNAPSHOT"
echo "D0WF_SL16D_LOCAL64: $D0WF_SL16D_LOCAL64" >> "$SNAPSHOT"
echo "D0WF_LOCAL_SL16D_LIB: $D0WF_LOCAL_SL16D_LIB" >> "$SNAPSHOT"
echo "D0WF_OVERLAY_LIB: $D0WF_OVERLAY_LIB" >> "$SNAPSHOT"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH" >> "$SNAPSHOT"

md5sum \
    "$RUNBFC_SRC" \
    "$FZD" \
    >> "$SNAPSHOT"

grep -Eqi \
'droy|/gpfs01/star/pwg/droy1|/star/u/droy1' \
"$SNAPSHOT"

if ($status == 0) then
    echo "ERROR: external personal-directory dependency detected"
    echo "       See $SNAPSHOT"
    exit 20
endif

echo ""
echo "Standalone environment:"
echo "STAR       = $STAR"
echo "STAR_LEVEL = $STAR_LEVEL"
echo "ROOT_LEVEL = $ROOT_LEVEL"
echo "ROOTSYS    = $ROOTSYS"
echo "root4star  = $ROOT4STAR"
echo ""

############################################################
# Run standalone FZD reconstruction
############################################################

root4star -l -b -q \
    'runBfc.C('"$NEVENTS"',"'st_physics_15130045_raw_1000011.fzd'")' \
    >&! "$LOGDIR/05_standalone_reco_v3.root4star.log"

set ROOT_STATUS = $status

echo "root4star status = $ROOT_STATUS"

if ($ROOT_STATUS != 0) then
    echo "ERROR: standalone reconstruction failed"
    exit 21
endif

############################################################
# Locate and normalize MuDst output name
############################################################

set EXPECTED = \
"$MIXDIR/st_physics_15130045_raw_1000011.MuDst.root"

if (! -e "$EXPECTED") then
    set MUDSTLIST = ( `find "$MIXDIR" \
        -maxdepth 1 \
        -type f \
        -name '*.MuDst.root' | sort` )

    if ($#MUDSTLIST == 0) then
        echo "ERROR: standalone reconstruction produced no MuDst"
        exit 22
    endif

    if ($#MUDSTLIST != 1) then
        echo "ERROR: expected one MuDst, found $#MUDSTLIST"
        ls -lh "$MIXDIR"/*.MuDst.root
        exit 23
    endif

    mv "$MUDSTLIST[1]" "$EXPECTED"
endif

echo ""
echo "Produced standalone MuDst:"
ls -lh "$EXPECTED"

echo ""
echo "Standalone Stage 5 v3 completed successfully."
echo "End: `date`"

exit 0
