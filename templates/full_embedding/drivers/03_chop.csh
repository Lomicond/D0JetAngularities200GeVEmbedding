#!/bin/tcsh

# Full-embedding Stage 3: select real events and create a chopped DAQ file.
#
# Required substitutions:
#   @D0WF@     project root
#   @SIMJOB@   generated simulation/full-embedding job directory
#   @NEVENTS@  number of real events to retain
#   @DAQFILE@  original real-data DAQ file

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

which daqFileChopper
if ($status != 0) then
    echo "ERROR: daqFileChopper not found after starver SL22c"
    exit 2
endif

set chopper_txt = "@SIMJOB@/moretags/st_physics_15130045_raw_1000011.chopper.txt"
set chopped_dir = "@SIMJOB@/chopped"
set trimmed_chopper = "$chopped_dir/st_physics_15130045_raw_1000011.first_@NEVENTS@.chopper.txt"
set chopped_daq = "$chopped_dir/st_physics_15130045_raw_1000011.daq"

if (! -s "$chopper_txt") then
    echo "ERROR: MoreTags chopper list is missing or empty: $chopper_txt"
    exit 3
endif

set n_available = `wc -l "$chopper_txt" | awk '{print $1}'`
if ($n_available < @NEVENTS@) then
    echo "ERROR: chopper list contains only $n_available events; need @NEVENTS@"
    exit 4
endif

mkdir -p "$chopped_dir"
head -n @NEVENTS@ "$chopper_txt" >! "$trimmed_chopper"

set events = (`awk '{print $2}' "$trimmed_chopper"`)
if ($#events != @NEVENTS@) then
    echo "ERROR: expected @NEVENTS@ event numbers, got $#events"
    exit 5
endif

rm -f "$chopped_daq"
daqFileChopper "@DAQFILE@" "-eventnum" $events >! "$chopped_daq"
set rc = $status

if ($rc != 0) then
    echo "ERROR: daqFileChopper failed with status $rc"
    exit $rc
endif

if (! -s "$chopped_daq") then
    echo "ERROR: chopped DAQ output is missing or empty: $chopped_daq"
    exit 6
endif

echo "Stage 3 completed:"
ls -lh "$trimmed_chopper" "$chopped_daq"

exit 0
