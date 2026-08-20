#!/bin/tcsh -f

# Build a catalog list of Run-2014 Au+Au MuDst files on HPSS.
# This script only creates text lists; it does not download any MuDst files.

set MIN_RUN  = 15107008
set MAX_RUN  = 15167014
set NSELECT  = 5000
set SEED     = 20140731
set OUTDIR   = lists

set RAWLIST  = "${OUTDIR}/Run2014_MuDst_HPSS_catalog_all.txt"
set CLEANLIST = "${OUTDIR}/Run2014_MuDst_HPSS_clean_all.txt"
set SELECTED = "${OUTDIR}/Run2014_MuDst_HPSS_random${NSELECT}.txt"
set REJECTED = "${OUTDIR}/Run2014_MuDst_HPSS_rejected.tsv"

set TMPBASE = "/tmp/${USER}_Run2014_MuDst_list_$$"
set BADRUNS = "${TMPBASE}.badruns"
set FILTER  = "${TMPBASE}.filter.awk"

onintr interrupted

if (! -d "${OUTDIR}") mkdir -p "${OUTDIR}"
if ($status != 0) then
    echo "ERROR: cannot create output directory: ${OUTDIR}"
    exit 1
endif

# Only bad runs at or above MIN_RUN are listed here. Earlier runs are rejected
# independently by the MIN_RUN requirement.
cat >! "${BADRUNS}" << \EOF_BADRUNS
15108018
15108019
15108020
15109040
15110032
15112049
15112050
15113001
15114058
15115086
15118063
15119025
15120011
15121062
15121076
15121077
15121078
15122003
15122004
15122006
15122008
15122010
15122011
15122042
15122043
15122044
15122045
15122049
15122062
15122063
15122064
15122065
15123001
15123002
15123003
15123006
15123009
15123010
15123011
15123019
15123020
15123021
15123022
15123023
15123024
15123025
15123026
15123027
15123028
15123035
15123036
15123037
15123050
15123051
15123053
15123054
15124001
15124002
15124003
15124004
15124006
15124008
15124010
15124028
15124031
15124032
15124033
15124034
15124035
15124040
15124041
15124042
15124043
15124044
15124056
15124057
15124058
15124060
15124061
15124062
15124063
15125001
15125002
15125003
15125007
15126009
15126010
15126011
15126012
15126013
15126015
15126016
15126017
15126018
15126019
15126021
15126022
15126023
15128031
15129006
15129011
15129013
15130001
15131040
15131042
15131044
15131045
15131046
15131047
15131048
15131049
15131050
15131051
15131052
15131053
15132008
15132009
15132010
15132017
15132018
15132019
15133043
15135016
15144018
15145021
15146003
15146004
15146049
15146050
15146051
15146052
15146054
15146055
15146057
15146058
15146059
15146060
15146061
15146062
15147001
15147002
15147003
15147004
15147005
15147006
15147007
15147008
15147009
15147010
15147011
15147012
15147013
15147014
15147015
15147027
15147028
15147029
15147030
15147031
15147032
15147033
15147041
15147042
15148003
15148004
15148005
15148006
15148007
15148008
15148009
15148010
15148011
15149012
15149013
15149015
15149016
15149017
15149071
15149073
15149074
15149076
15150001
15150004
15150027
15150030
15150031
15150062
15151041
15151042
15152004
15152016
15153050
15153055
15153056
15153057
15153058
15154001
15154002
15154003
15156008
15159036
15161022
15161051
15161066
15161067
15162047
15162053
15163022
15163054
15164048
15164067
15166014
15166015
15166016
15166017
EOF_BADRUNS

cat >! "${FILTER}" << \EOF_FILTER
BEGIN {
    while ((getline line < badfile) > 0) {
        if (line ~ /^[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$/)
            bad[line] = 1
    }
    close(badfile)
}
{
    path = $0
    npath = split(path, part, "/")
    nfield = split(part[npath], field, "_")
    run = ""

    # Expected forms include:
    # st_physics_15107008_raw_0000010.MuDst.root
    # st_physics_adc_15107008_raw_1000061.MuDst.root
    for (i = 2; i <= nfield; ++i) {
        if (field[i] == "raw")
            run = field[i - 1]
    }

    if (run !~ /^[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$/) {
        print "invalid_runID\t" path >> rejected
        next
    }
    if ((run + 0) < minrun || (run + 0) > maxrun) {
        print "outside_run_range\t" path >> rejected
        next
    }
    if (run in bad) {
        print "bad_run_" run "\t" path >> rejected
        next
    }

    print path
}
EOF_FILTER

echo "Querying the STAR catalog..."
get_file_list.pl \
    -keys path,filename \
    -delim '/' \
    -cond "production=P16id,trgsetupname=AuAu_200_production_mid_2014||AuAu_200_production_low_2014,filename~st_physics,filetype=daq_reco_MuDst,library=SL16d,runnumber[]${MIN_RUN}-${MAX_RUN},storage=hpss" \
    -limit 0 \
    -distinct >! "${RAWLIST}"

if ($status != 0 || ! -s "${RAWLIST}") then
    echo "ERROR: catalog query failed or returned an empty list."
    goto failed
endif

# Sorting makes duplicate removal and the seeded random sample reproducible
# even if the catalog returns the same records in a different order.
sort -u "${RAWLIST}" -o "${RAWLIST}"
rm -f "${REJECTED}"

awk \
    -v badfile="${BADRUNS}" \
    -v minrun="${MIN_RUN}" \
    -v maxrun="${MAX_RUN}" \
    -v rejected="${REJECTED}" \
    -f "${FILTER}" "${RAWLIST}" | sort -u >! "${CLEANLIST}"

if ($status != 0 || ! -s "${CLEANLIST}") then
    echo "ERROR: filtering failed or no acceptable MuDst files remain."
    goto failed
endif

set NRAW   = `wc -l < "${RAWLIST}"`
set NCLEAN = `wc -l < "${CLEANLIST}"`

if (${NCLEAN} <= ${NSELECT}) then
    cp "${CLEANLIST}" "${SELECTED}"
    set NFINAL = ${NCLEAN}
    set SAMPLE_NOTE = "all clean files (fewer than or equal to target)"
else
    perl -MList::Util=shuffle -e \
        '$seed = shift; $n = shift; srand($seed); @x = <>; @x = shuffle(@x); print @x[0 .. $n-1];' \
        "${SEED}" "${NSELECT}" "${CLEANLIST}" >! "${SELECTED}"
    if ($status != 0) then
        echo "ERROR: random selection failed."
        goto failed
    endif
    set NFINAL = `wc -l < "${SELECTED}"`
    set SAMPLE_NOTE = "seeded random sample"
endif

set NREJECT = 0
if (-e "${REJECTED}") set NREJECT = `wc -l < "${REJECTED}"`

rm -f "${BADRUNS}" "${FILTER}"

echo ""
echo "MuDst list creation completed."
echo "Catalog files       : ${NRAW}"
echo "Clean files         : ${NCLEAN}"
echo "Rejected records    : ${NREJECT}"
echo "Selected files      : ${NFINAL} (${SAMPLE_NOTE})"
echo "Random seed         : ${SEED}"
echo ""
echo "Complete clean list : ${CLEANLIST}"
echo "Selected list       : ${SELECTED}"
echo "Rejected records    : ${REJECTED}"
exit 0

interrupted:
echo ""
echo "Interrupted."
goto failed

failed:
rm -f "${BADRUNS}" "${FILTER}"
exit 1
