#!/bin/tcsh

# Full-embedding Stage 5: mix the chopped real event with the simulated FZD
# and run the SL16d_embed2 reconstruction chain.
#
# Required substitutions:
#   @D0WF@     project root
#   @SIMJOB@   generated simulation/full-embedding job directory
#   @NEVENTS@  number of events to process

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
starver SL16d_embed2
set starver_rc = $status
if ($starver_rc != 0) then
    echo "ERROR: starver SL16d_embed2 failed with status $starver_rc"
    exit $starver_rc
endif
rehash

# Prefer the locally validated SL16d_embed2 64-bit compatibility libraries.
setenv D0WF_SL16D_LOCAL64 "@D0WF@/local_SL16d_embed2_facade"
set local_sl16d_lib = "@D0WF@/local_SL16d_embed2_facade/.$STAR_HOST_SYS/lib"

if (! -d "$local_sl16d_lib") then
    echo "ERROR: Local SL16d library directory not found: $local_sl16d_lib"
    exit 2
endif

# The official 64-bit release may not provide a usable root4star wrapper.
if (-d "@D0WF@/local_sl16d_bin") then
    setenv PATH "@D0WF@/local_sl16d_bin:${ROOTSYS}/bin:${PATH}"
else
    setenv PATH "${ROOTSYS}/bin:${PATH}"
endif

if ($?LD_LIBRARY_PATH) then
    setenv LD_LIBRARY_PATH "@D0WF@/sl16d2_D0decay_overlay/lib:${local_sl16d_lib}:${STAR}/.${STAR_HOST_SYS}/lib:${ROOTSYS}/lib:${LD_LIBRARY_PATH}"
else
    setenv LD_LIBRARY_PATH "@D0WF@/sl16d2_D0decay_overlay/lib:${local_sl16d_lib}:${STAR}/.${STAR_HOST_SYS}/lib:${ROOTSYS}/lib"
endif

rehash

echo "D0WF_STAR_LEVEL=$STAR_LEVEL"
echo "D0WF_STAR=$STAR"
echo "D0WF_SL16D_LOCAL64=$D0WF_SL16D_LOCAL64"
echo "D0WF_LOCAL_SL16D_LIB=$local_sl16d_lib"

setenv D0WF_MIN_LOADER_LIB "$local_sl16d_lib"
echo "D0WF_MIN_LOADER_LIB=$D0WF_MIN_LOADER_LIB"
which root4star

set mixer_dir = "@SIMJOB@/mixer"
set chopped_daq = "@SIMJOB@/chopped/st_physics_15130045_raw_1000011.daq"
set starsim_fzd = "@SIMJOB@/starsim/D0toy.starsim.fzd"
set mixer_mudst = "$mixer_dir/st_physics_15130045_raw_1000011.MuDst.root"

if (! -s "$chopped_daq") then
    echo "ERROR: chopped DAQ is missing or empty: $chopped_daq"
    exit 3
endif

if (! -s "$starsim_fzd") then
    echo "ERROR: STARSIM FZD is missing or empty: $starsim_fzd"
    exit 4
endif

mkdir -p "$mixer_dir"

# The Mixer wrapper loads src/mixer/... relative to the job mixer directory.
ln -sfn "@D0WF@/src" "$mixer_dir/src"

cd "$mixer_dir"

root4star -l -b << ROOTEOF
.include $STAR/StRoot

gSystem->Load("libTable");
gSystem->Load("libSt_base");
gSystem->Load("libStarRoot");
gSystem->Load("libStarClassLibrary");
gSystem->Load("libStEvent");
gSystem->Load("libStChain");
gSystem->Load("libStBFChain");
gSystem->Load("libStEmcRawMaker");
gSystem->Load("libStEmcMixerMaker");

.L src/mixer/bfcMixer_Hft_D0toy.C
.L src/mixer/runMixerD0toy_ZB.C

runMixerD0toy_ZB(
    @NEVENTS@,
    "$chopped_daq",
    "$starsim_fzd"
);

.q
ROOTEOF

set rc = $status

if ($rc != 0) then
    echo "ERROR: Mixer ROOT process failed with status $rc"
    exit $rc
endif

if (! -s "$mixer_mudst") then
    echo "ERROR: expected Mixer MuDst is missing or empty: $mixer_mudst"
    exit 5
endif

echo "Stage 5 completed:"
ls -lh "$mixer_mudst"

exit 0
