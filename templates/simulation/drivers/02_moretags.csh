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

mkdir -p "@SIMJOB@/moretags"
rm -f \
    "@SIMJOB@/moretags/st_physics_15130045_raw_1000011.moretags.root" \
    "@SIMJOB@/moretags/st_physics_15130045_raw_1000011.chopper.txt"

root4star -l -b -q \
'@D0WF@/src/moretags/makeMuDstQA_Run14.C("@SIMJOB@/mudst.selected.list",1,0,"@SIMJOB@/moretags/")'
set rc = $status
exit $rc
