#!/bin/tcsh -f

set nonomatch

setenv D0WF @D0WF@

set SIMJOB = @SIMJOB@
set WORK    = @STANDJOB@

set REFERENCE_RECO_SRC = /gpfs01/star/pwg/droy1/STAR-Workspace/D0Analysis/PythiaScript
set REFERENCE_LOCALSTAR = /gpfs01/star/pwg/droy1/STAR-Workspace/LocalSTAR/SL16d_embed_64b

set NEVENTS = @NEVENTS@

set MIXDIR = $WORK/mixer
set LOGDIR = $WORK/logs

mkdir -p $MIXDIR
mkdir -p $LOGDIR
mkdir -p $WORK/snapshots

############################################################
# Locate STARSIM FZD
############################################################

set FZD = `find $SIMJOB \
    -type f \
    -name 'D0toy.starsim.fzd' | head -1`

if ( "$FZD" == "" ) then
    set FZD = `find $SIMJOB \
        -type f \
        -name '*.fzd' \
        ! -name 'kinematics*' | head -1`
endif

if ( "$FZD" == "" ) then
    echo "ERROR: no STARSIM FZD was found in:"
    echo "       $SIMJOB"
    exit 11
endif

if ( ! -e "$FZD" ) then
    echo "ERROR: FZD does not exist:"
    echo "       $FZD"
    exit 12
endif

echo "======================================================"
echo "Standalone reconstruction"
echo "Date       = `date`"
echo "Host       = `hostname`"
echo "FZD        = $FZD"
echo "NEVENTS    = $NEVENTS"
echo "WORK       = $WORK"
echo "======================================================"

############################################################
# Clean output directory
############################################################

cd $MIXDIR

rm -f runBfc.C
rm -f st_physics_15130045_raw_1000011.fzd
rm -f *.MuDst.root
rm -f *.event.root
rm -f *.hist.root
rm -f *.geant.root

cp -p $REFERENCE_RECO_SRC/runBfc.C .

ln -s $FZD st_physics_15130045_raw_1000011.fzd

############################################################
# Switch to the reference standalone reconstruction environment
############################################################

if ( $?ROOTSYS ) unsetenv ROOTSYS
if ( $?LD_LIBRARY_PATH ) unsetenv LD_LIBRARY_PATH
if ( $?ROOT_INCLUDE_PATH ) unsetenv ROOT_INCLUDE_PATH
if ( $?ROOT_PLUGIN_PATH ) unsetenv ROOT_PLUGIN_PATH
if ( $?ROOTMAP_PATH ) unsetenv ROOTMAP_PATH
if ( $?STAR ) unsetenv STAR
if ( $?STAR_LIB ) unsetenv STAR_LIB
if ( $?STAR_BIN ) unsetenv STAR_BIN
if ( $?QTROOTSYSDIR ) unsetenv QTROOTSYSDIR

source /star/u/droy1/.cshrc

starver SL16d_embed
setup 64b

setenv STAR $REFERENCE_LOCALSTAR
setenv STAR_LIB $STAR/.${STAR_HOST_SYS}/LIB
setenv STAR_BIN $STAR/.${STAR_HOST_SYS}/BIN
setenv QTROOTSYSDIR $STAR/.${STAR_HOST_SYS}

set path = ( $STAR_BIN $path )

if ( $?LD_LIBRARY_PATH ) then
    setenv LD_LIBRARY_PATH ${STAR_LIB}:${QTROOTSYSDIR}/lib:${ROOTSYS}/lib:${LD_LIBRARY_PATH}
else
    setenv LD_LIBRARY_PATH ${STAR_LIB}:${QTROOTSYSDIR}/lib:${ROOTSYS}/lib
endif

rehash

############################################################
# Save environment snapshot
############################################################

set SNAPSHOT = $WORK/snapshots/05_standalone_environment_v1.txt

echo "Date: `date`" >! $SNAPSHOT
echo "Host: `hostname`" >> $SNAPSHOT
echo "STAR: $STAR" >> $SNAPSHOT
echo "STAR_LEVEL: $STAR_LEVEL" >> $SNAPSHOT
echo "ROOT_LEVEL: $ROOT_LEVEL" >> $SNAPSHOT
echo "ROOTSYS: $ROOTSYS" >> $SNAPSHOT
echo "STAR_HOST_SYS: $STAR_HOST_SYS" >> $SNAPSHOT
echo "root4star: `which root4star`" >> $SNAPSHOT
echo "FZD: $FZD" >> $SNAPSHOT
echo "REFERENCE_RECO_SRC: $REFERENCE_RECO_SRC" >> $SNAPSHOT
echo "REFERENCE_LOCALSTAR: $REFERENCE_LOCALSTAR" >> $SNAPSHOT
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH" >> $SNAPSHOT

md5sum \
    $REFERENCE_RECO_SRC/runBfc.C \
    $FZD \
    >> $SNAPSHOT

echo ""
echo "Standalone environment:"
echo "STAR       = $STAR"
echo "STAR_LEVEL = $STAR_LEVEL"
echo "ROOT_LEVEL = $ROOT_LEVEL"
echo "ROOTSYS    = $ROOTSYS"
echo "root4star  = `which root4star`"
echo ""

############################################################
# Run standalone FZD reconstruction
############################################################

root4star -l -b -q \
    'runBfc.C('"$NEVENTS"',"'st_physics_15130045_raw_1000011.fzd'")' \
    >&! $LOGDIR/05_standalone_reco.root4star.log

set ROOT_STATUS = $status

echo "root4star status = $ROOT_STATUS"

if ( $ROOT_STATUS != 0 ) then
    echo "ERROR: standalone reconstruction failed"
    exit 21
endif

############################################################
# Locate and normalize MuDst output name
############################################################

set EXPECTED = $MIXDIR/st_physics_15130045_raw_1000011.MuDst.root

if ( ! -e "$EXPECTED" ) then

    set MUDSTLIST = ( `find $MIXDIR \
        -maxdepth 1 \
        -type f \
        -name '*.MuDst.root' | sort` )

    if ( $#MUDSTLIST == 0 ) then
        echo "ERROR: standalone reconstruction produced no MuDst"
        exit 22
    endif

    if ( $#MUDSTLIST != 1 ) then
        echo "ERROR: expected one MuDst, found $#MUDSTLIST"
        ls -lh $MIXDIR/*.MuDst.root
        exit 23
    endif

    mv $MUDSTLIST[1] $EXPECTED
endif

echo ""
echo "Produced standalone MuDst:"
ls -lh $EXPECTED

echo ""
echo "Standalone Stage 5 completed successfully."
echo "End: `date`"

exit 0
