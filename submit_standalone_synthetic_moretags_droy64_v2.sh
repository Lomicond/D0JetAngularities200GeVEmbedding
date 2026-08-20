#!/usr/bin/env bash

set -euo pipefail

PROJECT="/gpfs01/star/pwg/lomicond/Ondrej/Jets/PythiaD0JetGeant/D0EmbeddingClean"
TEMPLATE="${PROJECT}/JobStandaloneProduction_synthetic_moretags_droy64_v2.template.xml"
RUNTIME="${PROJECT}/scripts/run_standalone_production_synthetic_moretags_droy64_v2.csh"
MORETAGS_MACRO="${PROJECT}/src/moretags/makeSyntheticMoreTags_v1.C"
VERTEX_INPUT="${PROJECT}/src/moretags/Run14_AuAu200GeV_vertexDistributions.root"

usage()
{
    echo "Usage: $0 NEVENTS_PER_JOB NJOBS" >&2
    echo "Example: $0 1000 3" >&2
}

if [[ $# -ne 2 ]]; then
    usage
    exit 1
fi

NEVENTS="$1"
NJOBS="$2"

if [[ ! "${NEVENTS}" =~ ^[1-9][0-9]*$ ]]; then
    echo "ERROR: NEVENTS_PER_JOB must be a positive integer" >&2
    exit 2
fi

if [[ ! "${NJOBS}" =~ ^[1-9][0-9]*$ ]]; then
    echo "ERROR: NJOBS must be a positive integer" >&2
    exit 3
fi

if (( NJOBS > 100000 )); then
    echo "ERROR: NJOBS exceeds the safety limit of 100000" >&2
    exit 4
fi

for required in \
    "${TEMPLATE}" \
    "${RUNTIME}" \
    "${MORETAGS_MACRO}" \
    "${VERTEX_INPUT}"
do
    if [[ ! -s "${required}" ]]; then
        echo "ERROR: required production input is missing: ${required}" >&2
        exit 5
    fi
done

if ! command -v star-submit >/dev/null 2>&1; then
    echo "ERROR: star-submit is unavailable in the current environment" >&2
    exit 6
fi

RUN_TAG="$(date +%Y%m%d_%H%M%S)_$$"
ENTROPY_TEXT="${RUN_TAG}:$(date +%s%N):${NEVENTS}:${NJOBS}:${PROJECT}"
ENTROPY="$(printf '%s\n' "${ENTROPY_TEXT}" | cksum | awk '{print $1}')"

# Pythia accepts seeds up to 900000000.  The even/odd assignment in the XML
# gives two distinct seeds per scheduler job.
BASE_SEED=$((1000000 + ENTROPY % 700000000))
MAX_SEED=$((BASE_SEED + 2 * NJOBS + 1))

if (( MAX_SEED > 900000000 )); then
    echo "ERROR: generated seed range exceeds the PYTHIA limit" >&2
    exit 7
fi

# Each job receives a disjoint block of EvtId values.  Leave one extra job of
# headroom because this remains safe whether JOBINDEX is zero- or one-based.
EVENT_ID_BASE=$((100000000 + ENTROPY % 1000000000))
MAX_EVT_ID=$((EVENT_ID_BASE + (NJOBS + 1) * NEVENTS - 1))

if (( MAX_EVT_ID > 2147483647 )); then
    echo "ERROR: requested production does not fit the Int_t EvtId range" >&2
    echo "       EVENT_ID_BASE=${EVENT_ID_BASE}, maximum=${MAX_EVT_ID}" >&2
    exit 8
fi

SUBMISSION_DIR="${PROJECT}/submission/synthetic_moretags/${RUN_TAG}"
MARKER_DIR="${SUBMISSION_DIR}/job_markers"
JOB_LIST="${SUBMISSION_DIR}/job_markers.list"
RENDERED_XML="${SUBMISSION_DIR}/JobStandaloneProduction_synthetic_moretags_droy64_v2.xml"
MANIFEST="${SUBMISSION_DIR}/submission_manifest.txt"

mkdir -p \
    "${MARKER_DIR}" \
    "${PROJECT}/out" \
    "${PROJECT}/err" \
    "${PROJECT}/log" \
    "${PROJECT}/production/synthetic_moretags" \
    "${PROJECT}/production/synthetic_moretags/checkpoints" \
    "${PROJECT}/report" \
    "${PROJECT}/csh" \
    "${PROJECT}/list"

: > "${JOB_LIST}"

for ((i_job = 0; i_job < NJOBS; ++i_job)); do
    marker="${MARKER_DIR}/job_$(printf '%06d' "${i_job}").input"
    printf 'submission_tag=%s\nrequested_index=%d\n' \
        "${RUN_TAG}" "${i_job}" > "${marker}"
    printf '%s\n' "${marker}" >> "${JOB_LIST}"
done

sed \
    -e "s|@NEVENTS@|${NEVENTS}|g" \
    -e "s|@NJOBS@|${NJOBS}|g" \
    -e "s|@BASE_SEED@|${BASE_SEED}|g" \
    -e "s|@EVENT_ID_BASE@|${EVENT_ID_BASE}|g" \
    -e "s|@JOB_LIST@|${JOB_LIST}|g" \
    -e "s|@SUBMISSION_TAG@|${RUN_TAG}|g" \
    "${TEMPLATE}" > "${RENDERED_XML}"

if grep -Eq '@[A-Z_][A-Z_]*@' "${RENDERED_XML}"; then
    echo "ERROR: unresolved template placeholder in ${RENDERED_XML}" >&2
    grep -nE '@[A-Z_][A-Z_]*@' "${RENDERED_XML}" >&2
    exit 9
fi

if command -v xmllint >/dev/null 2>&1; then
    xmllint --noout "${RENDERED_XML}"
fi

TOTAL_EVENTS=$((NEVENTS * NJOBS))

{
    echo "submission_tag=${RUN_TAG}"
    echo "events_per_job=${NEVENTS}"
    echo "number_of_jobs=${NJOBS}"
    echo "total_requested_events=${TOTAL_EVENTS}"
    echo "base_seed=${BASE_SEED}"
    echo 'pythia_seed_formula=BASE_SEED+2*JOBINDEX'
    echo 'moretags_seed_formula=BASE_SEED+2*JOBINDEX+1'
    echo "event_id_base=${EVENT_ID_BASE}"
    echo 'first_evt_id_formula=EVENT_ID_BASE+JOBINDEX*NEVENTS'
    echo "job_list=${JOB_LIST}"
    echo "rendered_xml=${RENDERED_XML}"
} > "${MANIFEST}"

echo "============================================================"
echo "Synthetic-MoreTags standalone production submission"
echo "============================================================"
echo "Events per job : ${NEVENTS}"
echo "Number of jobs : ${NJOBS}"
echo "Total events   : ${TOTAL_EVENTS}"
echo "Base seed      : ${BASE_SEED}"
echo "EvtId base     : ${EVENT_ID_BASE}"
echo "Submission tag : ${RUN_TAG}"
echo "Rendered XML   : ${RENDERED_XML}"
echo "============================================================"

cd "${PROJECT}"
star-submit-beta "${RENDERED_XML}" | tee "${SUBMISSION_DIR}/star-submit.log"

echo
echo "Submission completed."
echo "Manifest: ${MANIFEST}"
echo "Log:      ${SUBMISSION_DIR}/star-submit.log"
