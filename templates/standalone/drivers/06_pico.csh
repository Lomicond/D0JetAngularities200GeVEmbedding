#!/bin/tcsh

# Remove the standalone custom Pythia 8.317 environment before STAR.
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

# Prefer locally built STAR libraries after starver configured the release.
set local_star_lib = "@D0WF@/.$STAR_HOST_SYS/lib"

if ( -d "$local_star_lib" ) then
    if ( $?LD_LIBRARY_PATH ) then
        setenv LD_LIBRARY_PATH "${local_star_lib}:${LD_LIBRARY_PATH}"
    else
        setenv LD_LIBRARY_PATH "${local_star_lib}"
    endif
else
    echo "ERROR: Local STAR library directory not found: $local_star_lib"
    exit 2
endif

echo "D0WF_LOCAL_STAR_LIB=$local_star_lib"

mkdir -p "@STANDJOB@/pico"
rm -f "@STANDJOB@/pico/st_physics_15130045_raw_1000011.picoDst.root"

cd "@STANDJOB@/pico"

root4star -l -b << ROOTEOF
.include $STAR/StRoot

.L @D0WF@/src/pico/makePicoDstFromMuDst.C

makePicoDstFromMuDst(
    "@STANDJOB@/mixer/st_physics_15130045_raw_1000011.MuDst.root",
    @NEVENTS@,
    "y2014a"
);

.q
ROOTEOF

set rc = $status
exit $rc
