#!/bin/tcsh

# Full-embedding Stage 6: convert the Mixer MuDst to PicoDst.
#
# Required substitutions:
#   @D0WF@     project root
#   @SIMJOB@   generated simulation/full-embedding job directory
#   @NEVENTS@  maximum number of MuDst events to process

# Remove any standalone Pythia environment before entering STAR.
unsetenv PYTHIA8
unsetenv PYTHIA8DATA

if ($?PATH) then
    setenv PATH `echo "$PATH" | tr ':' '\n' | grep -v -E 'pythia8317|Alma9Pythia8' | paste -sd: -`
endif

if ($?LD_LIBRARY_PATH) then
    setenv LD_LIBRARY_PATH `echo "$LD_LIBRARY_PATH" | tr ':' '\n' | grep -v -E 'pythia8317|Alma9Pythia8' | paste -sd: -`
endif

rehash
starver SL22c
set starver_rc = $status
if ($starver_rc != 0) then
    echo "ERROR: starver SL22c failed with status $starver_rc"
    exit $starver_rc
endif
rehash

echo "D0WF_STAR_LEVEL=$STAR_LEVEL"
echo "D0WF_STAR=$STAR"

# Load the locally built SL22c StPicoDstMaker without exposing it
# to the SL16d_embed2 reconstruction environment.
set local_pico_lib = "@D0WF@/local_SL22c_pico_lib"

if ( ! -s "$local_pico_lib/libStPicoDstMaker.so" ) then
    echo "ERROR: Isolated SL22c PicoDstMaker library not found:"
    echo "       $local_pico_lib/libStPicoDstMaker.so"
    exit 2
endif

if ( $?LD_LIBRARY_PATH ) then
    setenv LD_LIBRARY_PATH "${local_pico_lib}:${LD_LIBRARY_PATH}"
else
    setenv LD_LIBRARY_PATH "${local_pico_lib}"
endif

echo "D0WF_SL22C_PICO_LIB=$local_pico_lib"
ls -l "$local_pico_lib/StPicoDstMaker.so" \
      "$local_pico_lib/libStPicoDstMaker.so"

set pico_dir = "@SIMJOB@/pico"
set mixer_mudst = "@SIMJOB@/mixer/st_physics_15130045_raw_1000011.MuDst.root"
set pico_output = "$pico_dir/st_physics_15130045_raw_1000011.picoDst.root"

if (! -s "$mixer_mudst") then
    echo "ERROR: Mixer MuDst input is missing or empty: $mixer_mudst"
    exit 3
endif

mkdir -p "$pico_dir"
rm -f "$pico_output"

cd "$pico_dir"

root4star -l -b << ROOTEOF
.include $STAR/StRoot

.L @D0WF@/src/pico/makePicoDstFromMuDst.C

makePicoDstFromMuDst(
    "$mixer_mudst",
    @NEVENTS@,
    "y2014a"
);

.q
ROOTEOF

set rc = $status

if ($rc != 0) then
    echo "ERROR: PicoDst ROOT process failed with status $rc"
    exit $rc
endif

if (! -s "$pico_output") then
    echo "ERROR: PicoDst output is missing or empty: $pico_output"
    exit 4
endif

echo "Stage 6 completed:"
ls -lh "$pico_output"

exit 0
