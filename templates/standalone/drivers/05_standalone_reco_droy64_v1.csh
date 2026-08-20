#!/bin/tcsh -f

set nonomatch

setenv D0WF @D0WF@

set SIMJOB = @SIMJOB@
set WORK   = @STANDJOB@
set NEVENTS = @NEVENTS@

set RUNBFC_SRC = "$D0WF/src/standalone/runBfc.C"
set LOCAL_SETUP = \
"$D0WF/scripts/setup_Droy_SL16d_embed_64b_v1.csh"

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

set DROY_STAR = \
"/gpfs01/star/pwg/droy1/STAR-Workspace/LocalSTAR/SL16d_embed_64b"

if (! -d "$DROY_STAR/.sl73_x8664_gcc485/LIB") then
    echo "ERROR: Droy 64-bit library tree is missing:"
    echo "       $DROY_STAR/.sl73_x8664_gcc485/LIB"
    exit 4
endif

if (! -x "$DROY_STAR/.sl73_x8664_gcc485/BIN/root4star") then
    echo "ERROR: Droy root4star is missing or not executable:"
    echo "       $DROY_STAR/.sl73_x8664_gcc485/BIN/root4star"
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
echo "Standalone reconstruction Droy64 v1"
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
"$SNAPDIR/05_standalone_environment_droy64_v1.txt"

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
    "$ROOT4STAR" \
    "$STAR_LIB/libStdEdxY2Maker.so" \
    >> "$SNAPSHOT"

set RESOLVED_STAR = `readlink -f "$STAR"`
set RESOLVED_EXPECTED_STAR = `readlink -f "$DROY_STAR"`

if ("$RESOLVED_STAR" != "$RESOLVED_EXPECTED_STAR") then
    echo "ERROR: wrong STAR tree is active"
    echo "       active   = $RESOLVED_STAR"
    echo "       expected = $RESOLVED_EXPECTED_STAR"
    exit 20
endif

set RESOLVED_ROOT4STAR = `readlink -f "$ROOT4STAR"`
set EXPECTED_ROOT4STAR = \
`readlink -f "$DROY_STAR/.sl73_x8664_gcc485/BIN/root4star"`

if ("$RESOLVED_ROOT4STAR" != "$EXPECTED_ROOT4STAR") then
    echo "ERROR: wrong root4star is active"
    echo "       active   = $RESOLVED_ROOT4STAR"
    echo "       expected = $EXPECTED_ROOT4STAR"
    exit 21
endif

echo "$LD_LIBRARY_PATH" | tr ':' '\n' | \
    grep -E 'SL16d_embed2|local_SL16d_embed2_facade|local_sl16d_bin'

if ($status == 0) then
    echo "ERROR: SL16d_embed2 contamination detected"
    echo "       See $SNAPSHOT"
    exit 22
endif

set DEDX_MD5 = \
`md5sum "$STAR_LIB/libStdEdxY2Maker.so" | awk '{print $1}'`

if ("$DEDX_MD5" != "eaed165ff1ed2b5e556b08d82f5fe139") then
    echo "ERROR: unexpected Droy libStdEdxY2Maker.so"
    echo "       MD5 = $DEDX_MD5"
    exit 23
endif

echo ""
echo "Standalone environment:"
echo "STAR       = $STAR"
echo "STAR_LEVEL = $STAR_LEVEL"
echo "ROOT_LEVEL = $ROOT_LEVEL"
echo "ROOTSYS    = $ROOTSYS"
echo "root4star  = $ROOT4STAR"
echo "STAR_LIB: $STAR_LIB" >> "$SNAPSHOT"
echo "STAR_BIN: $STAR_BIN" >> "$SNAPSHOT"
echo "D0WF_DROY_STAR: $D0WF_DROY_STAR" >> "$SNAPSHOT"
echo "D0WF_DROY_BUILD: $D0WF_DROY_BUILD" >> "$SNAPSHOT"
echo "D0WF_DROY_ROOT4STAR: $D0WF_DROY_ROOT4STAR" >> "$SNAPSHOT"

echo "Resolved root4star: `readlink -f "$ROOT4STAR"`" \
    >> "$SNAPSHOT"

echo "Resolved libStdEdxY2Maker: `readlink -f "$STAR_LIB/libStdEdxY2Maker.so"`" \
    >> "$SNAPSHOT"
echo ""

############################################################
# Run standalone FZD reconstruction
############################################################

root4star -l -b -q \
    'runBfc.C('"$NEVENTS"',"'st_physics_15130045_raw_1000011.fzd'")' \
    >&! "$LOGDIR/05_standalone_reco_droy64_v1.root4star.log"

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
echo "Standalone Stage 5 Droy64 v1 completed successfully."
echo "End: `date`"

exit 0
