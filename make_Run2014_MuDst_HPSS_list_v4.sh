#!/usr/bin/env bash

# Create Run-2014 Au+Au MuDst file lists from the STAR catalog.
# This script only creates text lists; it does not download files from HPSS.

set -euo pipefail

MIN_RUN=15107008
MAX_RUN=15167014
NSELECT=5000
SEED=20140731
OUTDIR="lists"

RAWLIST="${OUTDIR}/Run2014_MuDst_HPSS_catalog_all.txt"
CLEANLIST="${OUTDIR}/Run2014_MuDst_HPSS_clean_all.txt"
SELECTED="${OUTDIR}/Run2014_MuDst_HPSS_random${NSELECT}.txt"
REJECTED="${OUTDIR}/Run2014_MuDst_HPSS_rejected.tsv"

mkdir -p "${OUTDIR}"

if ! command -v get_file_list.pl >/dev/null 2>&1; then
    echo "ERROR: get_file_list.pl is not available in PATH." >&2
    exit 1
fi

echo "Querying the STAR catalog..."

if ! get_file_list.pl \
    -keys path,filename \
    -delim '/' \
    -cond "production=P16id,trgsetupname=AuAu_200_production_mid_2014||AuAu_200_production_low_2014,filename~st_physics,filetype=daq_reco_MuDst,library=SL16d,runnumber[]${MIN_RUN}-${MAX_RUN},storage=hpss" \
    -limit 0 \
    -distinct > "${RAWLIST}"; then
    echo "ERROR: STAR catalog query failed." >&2
    exit 1
fi

if [[ ! -s "${RAWLIST}" ]]; then
    echo "ERROR: STAR catalog returned an empty list." >&2
    exit 1
fi

# Normalize the catalog output before filtering. This also removes duplicate
# paths and makes the seeded random selection independent of catalog ordering.
LC_ALL=C sort -u "${RAWLIST}" -o "${RAWLIST}"

python3 - \
    "${RAWLIST}" \
    "${CLEANLIST}" \
    "${SELECTED}" \
    "${REJECTED}" \
    "${MIN_RUN}" \
    "${MAX_RUN}" \
    "${NSELECT}" \
    "${SEED}" <<'PYTHON'
import random
import re
import sys
from pathlib import Path

(
    raw_name,
    clean_name,
    selected_name,
    rejected_name,
    min_run_text,
    max_run_text,
    nselect_text,
    seed_text,
) = sys.argv[1:]

min_run = int(min_run_text)
max_run = int(max_run_text)
nselect = int(nselect_text)
seed = int(seed_text)

bad_runs = {
    15108018,15108019,15108020,15109040,15110032,15112049,15112050,15113001,
    15114058,15115086,15118063,15119025,15120011,15121062,15121076,15121077,
    15121078,15122003,15122004,15122006,15122008,15122010,15122011,15122042,
    15122043,15122044,15122045,15122049,15122062,15122063,15122064,15122065,
    15123001,15123002,15123003,15123006,15123009,15123010,15123011,15123019,
    15123020,15123021,15123022,15123023,15123024,15123025,15123026,15123027,
    15123028,15123035,15123036,15123037,15123050,15123051,15123053,15123054,
    15124001,15124002,15124003,15124004,15124006,15124008,15124010,15124028,
    15124031,15124032,15124033,15124034,15124035,15124040,15124041,15124042,
    15124043,15124044,15124056,15124057,15124058,15124060,15124061,15124062,
    15124063,15125001,15125002,15125003,15125007,15126009,15126010,15126011,
    15126012,15126013,15126015,15126016,15126017,15126018,15126019,15126021,
    15126022,15126023,15128031,15129006,15129011,15129013,15130001,15131040,
    15131042,15131044,15131045,15131046,15131047,15131048,15131049,15131050,
    15131051,15131052,15131053,15132008,15132009,15132010,15132017,15132018,
    15132019,15133043,15135016,15144018,15145021,15146003,15146004,15146049,
    15146050,15146051,15146052,15146054,15146055,15146057,15146058,15146059,
    15146060,15146061,15146062,15147001,15147002,15147003,15147004,15147005,
    15147006,15147007,15147008,15147009,15147010,15147011,15147012,15147013,
    15147014,15147015,15147027,15147028,15147029,15147030,15147031,15147032,
    15147033,15147041,15147042,15148003,15148004,15148005,15148006,15148007,
    15148008,15148009,15148010,15148011,15149012,15149013,15149015,15149016,
    15149017,15149071,15149073,15149074,15149076,15150001,15150004,15150027,
    15150030,15150031,15150062,15151041,15151042,15152004,15152016,15153050,
    15153055,15153056,15153057,15153058,15154001,15154002,15154003,15156008,
    15159036,15161022,15161051,15161066,15161067,15162047,15162053,15163022,
    15163054,15164048,15164067,15166014,15166015,15166016,15166017,
}

run_pattern = re.compile(r"_([0-9]{8})_raw_")
raw_paths = [
    line.strip()
    for line in Path(raw_name).read_text(encoding="utf-8").splitlines()
    if line.strip()
]

clean_paths = []
rejected = []

for path in raw_paths:
    matches = run_pattern.findall(Path(path).name)
    if len(matches) != 1:
        rejected.append(("invalid_runID", path))
        continue

    run = int(matches[0])
    if run < min_run or run > max_run:
        rejected.append(("outside_run_range", path))
    elif run in bad_runs:
        rejected.append((f"bad_run_{run}", path))
    else:
        clean_paths.append(path)

clean_paths = sorted(set(clean_paths))

if not clean_paths:
    raise SystemExit("ERROR: no acceptable MuDst files remain after filtering.")

if len(clean_paths) <= nselect:
    selected_paths = clean_paths
    sample_note = "all clean files (at most the requested number)"
else:
    generator = random.Random(seed)
    selected_paths = sorted(generator.sample(clean_paths, nselect))
    sample_note = "seeded random sample"

Path(clean_name).write_text(
    "".join(f"{path}\n" for path in clean_paths),
    encoding="utf-8",
)
Path(selected_name).write_text(
    "".join(f"{path}\n" for path in selected_paths),
    encoding="utf-8",
)
Path(rejected_name).write_text(
    "".join(f"{reason}\t{path}\n" for reason, path in rejected),
    encoding="utf-8",
)

print()
print("MuDst list creation completed.")
print(f"Catalog files       : {len(raw_paths)}")
print(f"Clean files         : {len(clean_paths)}")
print(f"Rejected records    : {len(rejected)}")
print(f"Selected files      : {len(selected_paths)} ({sample_note})")
print(f"Random seed         : {seed}")
print()
print(f"Complete clean list : {clean_name}")
print(f"Selected list       : {selected_name}")
print(f"Rejected records    : {rejected_name}")
PYTHON
