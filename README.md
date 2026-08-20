# Standalone D0 simulation with synthetic MoreTags

This repository contains a STAR-specific production workflow for generating
standalone `D0`/`anti-D0` Monte Carlo events and propagating them through
PYTHIA 8.303, STARSIM/GEANT3, standalone reconstruction, and PicoDst
production.

The supported batch workflow does **not** consume a real MuDst or DAQ file.
Instead, every accepted PYTHIA event is paired entry by entry with a synthetic
`MoreTags` entry. The collision vertex is sampled from Run14 Au+Au vertex
histograms and the remaining run context is taken from a fixed metadata record.

The workflow was validated end to end with 100 jobs of 100 events (10,000
events total). Before starting another large production, run the smoke tests
described below, especially after changing paths, STAR libraries, or random-seed
handling.

That 10k validation predates the explicit job-specific seeds for
`StarPythia8Decayer` and GEANT added with this cleanup. The full chain is
validated, but the new five-stream seed plumbing must pass a small `5 2` smoke
test before it is used for another large production.

## Scope and portability

This is not a generic standalone package. It is intended for users with access
to the STAR software and filesystems at BNL. In particular, stage 5 currently
depends on the shared Droy SL16d_embed 64-bit installation:

```text
/gpfs01/star/pwg/droy1/STAR-Workspace/LocalSTAR/SL16d_embed_64b
```

The repository path itself is resolved automatically from the location of the
top-level submission script, so another user may clone the project elsewhere.
The shared Droy path and the project-local binary runtime assets listed under
[Requirements](#requirements) must still be available.

## Production chain

| Stage | Operation | Main implementation |
|---:|---|---|
| 1 | Generate accepted `D0` events with PYTHIA 8.303 | `templates/simulation/pythia/` |
| 2 | Generate and validate synthetic MoreTags | `src/moretags/makeSyntheticMoreTags_v1.C` |
| 3 | DAQ/chopper | skipped |
| 4 | Transport through STAR with STARSIM/GEANT3 | `src/starsim/starsim.D0toy.C` |
| 5 | Reconstruct FZD to standalone MuDst | `templates/standalone/drivers/05_standalone_reco_droy64_v1.csh` |
| 6 | Convert MuDst to PicoDst | `templates/standalone/drivers/06_pico.csh` |

The batch entrypoint is:

```text
submit_standalone_synthetic_moretags_droy64_v2.sh
```

It renders
`JobStandaloneProduction_synthetic_moretags_droy64_v2.template.xml`, creates
one marker input per requested process, and submits the jobs with
`star-submit-beta`.

## Physics configuration

### PYTHIA generation

The frozen stage-1 configuration uses:

- PYTHIA 8.303 compiled from `external/pythia8_303/`;
- `pp` collisions at `sqrt(s) = 200 GeV`;
- the Detroit tune;
- `HardQCD:all = on`;
- `PhaseSpace:pTHatMin = 3 GeV/c`;
- `421:mayDecay = off` during event generation.

An event is accepted when it contains at least one `D0` or `anti-D0` satisfying

- `1 < pT(D0) < 25 GeV/c`;
- `|y(D0)| < 1`.

Stored final-state particles are restricted to `|eta| < 3`, and neutrinos are
excluded. The frozen source and command file in
`templates/simulation/pythia/` are authoritative if this summary and the code
ever differ.

### Synthetic MoreTags

The vertex input is:

```text
src/moretags/Run14_AuAu200GeV_vertexDistributions.root
```

It must contain:

```text
event/hVtxZ
event/hVtxR
metadata/RunContext
```

For every accepted PYTHIA event, `makeSyntheticMoreTags_v1.C`:

- samples `(Vx,Vy)` jointly from `event/hVtxR`;
- samples `Vz` independently from `event/hVtxZ`;
- assigns a unique sequential `EvtId`;
- copies `RunId`, STAR timestamps, and magnetic field from
  `metadata/RunContext`.

The vertex histograms do not have to be normalized. ROOT's `TH1::GetRandom()`
and `TH2::GetRandom2()` use relative bin contents internally. Histogram
integrals must be positive.

### STARSIM and D0 decay

Stage 4 synchronizes PYTHIA and MoreTags strictly by entry index. It applies
the synthetic collision vertex through `StarPrimaryMaker` and uses the Run14
configuration:

```text
y2014x geant gstar usexgeom agml misalign newtpcalignment bigbig
```

`D0` and `anti-D0` decays are handled by `StarPythia8Decayer` with:

```text
421:onMode = 0
421:onIfMatch = 321 -211
```

This forces the kaon-pion channel. STARSIM writes both a ROOT bookkeeping file
and an FZD file used by stage 5.

### Reconstruction and PicoDst

Stage 5 uses Droy's validated 64-bit SL16d_embed tree and reconstructs the FZD
without a heavy-ion underlying event. Stage 6 switches to SL22c and loads the
isolated project-local `StPicoDstMaker` library before producing the PicoDst.

## Requirements

Run from a BNL environment in which the following are available:

- `bash`, `tcsh`, `g++`, ROOT, and `root4star`;
- `star-submit-beta`;
- the STAR SL7 container
  `/cvmfs/star.sdcc.bnl.gov/containers/rhic_sl7.sif`;
- the shared Droy tree shown above;
- a project-local PYTHIA 8.303 source tree in `external/pythia8_303/`;
- project-local SL16d compatibility/overlay assets included by the XML sandbox;
- `local_SL22c_pico_lib/libStPicoDstMaker.so` for stage 6.

The XML performs early checks for Droy's `root4star`,
`libStdEdxY2Maker.so`, and the isolated SL22c PicoDstMaker library. The
validated `libStdEdxY2Maker.so` checksum is also checked before an expensive
job begins.

The binary runtime directories are intentionally not stored in Git. A fresh
clone is therefore not sufficient by itself; another user must provide or
rebuild the required local assets.

Before submission, verify at least:

```bash
command -v star-submit-beta
test -x /gpfs01/star/pwg/droy1/STAR-Workspace/LocalSTAR/SL16d_embed_64b/.sl73_x8664_gcc485/BIN/root4star
test -s local_SL22c_pico_lib/libStPicoDstMaker.so
test -d external/pythia8_303/include/Pythia8
```

## Quick start

Run the submission script from the repository root:

```bash
bash submit_standalone_synthetic_moretags_droy64_v2.sh \
    NEVENTS_PER_JOB NJOBS
```

For example, the validated 10k layout is:

```bash
bash submit_standalone_synthetic_moretags_droy64_v2.sh 100 100
```

This requests 100 independent jobs with 100 events per job, or 10,000 events
in total. A smaller smoke test is:

```bash
bash submit_standalone_synthetic_moretags_droy64_v2.sh 5 1
```

Both arguments must be positive integers. The submission helper limits
`NJOBS` to 100,000 and checks the allowed seed and signed-`Int_t` event-ID
ranges before rendering the XML.

By default the project root is the directory containing the submission script.
An explicit override is available when needed:

```bash
D0WF_PROJECT=/absolute/path/to/D0EmbeddingClean \
    bash /absolute/path/to/D0EmbeddingClean/submit_standalone_synthetic_moretags_droy64_v2.sh 5 1
```

## Random seeds and event IDs

The submitter derives a submission-specific `BASE_SEED` from the timestamp,
process ID, requested layout, and project path. For scheduler job index `j`,
the random streams are:

| Stream | Seed |
|---|---:|
| PYTHIA event generation | `BASE_SEED + 5*j` |
| synthetic MoreTags vertex sampling | `BASE_SEED + 5*j + 1` |
| `StarPythia8Decayer` | `BASE_SEED + 5*j + 2` |
| GEANT seed 1 | `BASE_SEED + 5*j + 3` |
| GEANT seed 2 | `BASE_SEED + 5*j + 4` |

The GEANT seeds are passed to STAR through the `RNDM` command. All five
streams are distinct both within a job and across jobs in one submission.

Each job also receives a disjoint event-ID block:

```text
FIRST_EVT_ID = EVENT_ID_BASE + j * NEVENTS_PER_JOB
LAST_EVT_ID  = FIRST_EVT_ID + NEVENTS_PER_JOB - 1
```

The rendered XML, submission manifest, per-job log, and final metadata file all
record the assigned seeds and event-ID range.

## Output layout

Every submission gets a unique tag of the form:

```text
YYYYMMDD_HHMMSS_PID
```

Submission bookkeeping is stored under:

```text
submission/synthetic_moretags/<SUBMISSION_TAG>/
├── JobStandaloneProduction_synthetic_moretags_droy64_v2.xml
├── job_markers/
├── job_markers.list
├── submission_manifest.txt
└── star-submit.log
```

Scheduler output and consolidated job logs are written to:

```text
out/
err/
log/
report/
csh/
list/
```

Successful final products are copied from the scheduler scratch directory to:

```text
production/synthetic_moretags/
```

For each job this directory contains:

```text
D0StandaloneSyntheticMoreTagsDroy64V2_<TAG>_<N>evts_seed<SEED>_<JOBID>_<JOBINDEX>.MuDst.root
D0StandaloneSyntheticMoreTagsDroy64V2_<TAG>_<N>evts_seed<SEED>_<JOBID>_<JOBINDEX>.picoDst.root
D0StandaloneSyntheticMoreTagsDroy64V2_<TAG>_<N>evts_seed<SEED>_<JOBID>_<JOBINDEX>.metadata.txt
```

If reconstruction succeeds but the PicoDst stage fails, the MuDst is copied to
`production/synthetic_moretags/checkpoints/` before the job exits.

## Monitoring and completeness checks

The submission helper prints the tag and writes it to
`submission_manifest.txt`. For a known tag:

```bash
TAG=YYYYMMDD_HHMMSS_PID

find production/synthetic_moretags \
    -maxdepth 1 -name "*${TAG}*.picoDst.root" | wc -l

find production/synthetic_moretags \
    -maxdepth 1 -name "*${TAG}*.metadata.txt" | wc -l

ls -lh log/*"${TAG}"*
```

The expected PicoDst and metadata counts are both `NJOBS`. Inspect failures in
the corresponding consolidated log under `log/`, then in `out/` and `err/`.

## Manual execution and stage restart

Batch submission is recommended for production. The underlying runner can also
be called directly for testing:

```tcsh
tcsh scripts/run_standalone_production_synthetic_moretags_droy64_v2.csh \
    NEVENTS START_STAGE FORCE \
    PYTHIA_SEED MORETAGS_SEED FIRST_EVT_ID \
    D0_DECAYER_SEED GEANT_SEED1 GEANT_SEED2
```

Example:

```tcsh
tcsh scripts/run_standalone_production_synthetic_moretags_droy64_v2.csh \
    5 1 0 1000011 2000011 1000011 3000011 4000011 5000011
```

`START_STAGE` may be `1`, `2`, `4`, `5`, or `6`. Stage 3 does not exist in
this standalone workflow. With `FORCE=0`, a stage is skipped when its existing
output passes validation. With `FORCE=1`, stages from `START_STAGE` onward are
rerun.

Use exactly the same event count, seeds, and first event ID when resuming an
existing job; these values are part of its working-directory name and random
state definition.

## Merging PicoDst files

Merge only files belonging to one submission tag:

```bash
TAG=YYYYMMDD_HHMMSS_PID

hadd -f "Output_${TAG}_MC.root" \
    production/synthetic_moretags/*"${TAG}"*.picoDst.root
```

Before merging, verify that the number of input PicoDst files equals the
requested number of jobs and inspect the metadata files for unique seeds and
non-overlapping event-ID ranges.

## Fast MC QA

`scripts/qa/comparePicoDstSamples_v17.C` is a lightweight MC-only comparison.
It reads only:

```text
McTrack.mPx
McTrack.mPy
McTrack.mGePid (or McTrack.mGeantId)
```

It produces four new/reference comparisons:

1. MC tracks per event;
2. MC `pT` shape;
3. MC GEANT-PID fractions;
4. `D0`/`anti-D0` `pT` shape for GEANT IDs 37 and 38.

Example:

```bash
mkdir -p work/qa

root4star -l -b -q \
  'scripts/qa/comparePicoDstSamples_v17.C("Output_10k_MC.root","reference.picoDst.root","work/qa/PicoDstQA")'
```

The MC-track multiplicity histogram is divided by the number of events. The
three shape histograms are normalized to unit sum, including underflow and
overflow. They are not divided by bin width.

The outputs are:

```text
work/qa/PicoDstQA_mc_summary_v17.pdf
work/qa/PicoDstQA_mc_summary_v17.root
```

## Repository map

```text
submit_standalone_synthetic_moretags_droy64_v2.sh
JobStandaloneProduction_synthetic_moretags_droy64_v2.template.xml

scripts/
├── run_standalone_production_synthetic_moretags_droy64_v2.csh
├── setup_Droy_SL16d_embed_64b_v1.csh
└── qa/
    └── comparePicoDstSamples_v17.C

src/
├── moretags/
│   ├── makeSyntheticMoreTags_v1.C
│   └── Run14_AuAu200GeV_vertexDistributions.root
├── starsim/
│   └── starsim.D0toy.C
├── standalone/
└── pico/

templates/
├── simulation/
└── standalone/
    └── drivers/
        ├── 05_standalone_reco_droy64_v1.csh
        └── 06_pico.csh
```

## Cleaning generated output

`CleanOutput.csh` removes scheduler logs, reports, and everything currently
stored under `production/`. It asks for confirmation, but the operation is
destructive and the ROOT products are not recoverable from Git.

Run it only from the repository root and only after copying all required
production outputs elsewhere:

```tcsh
./CleanOutput.csh
```

The ignored `submission/` bookkeeping directories are intentionally not
removed by this helper.

## Reproducibility checklist

For every production, preserve:

- the Git commit hash (`git rev-parse HEAD`);
- `submission_manifest.txt` and the rendered XML;
- all per-job `.metadata.txt` files;
- the STAR, ROOT, and Droy environment snapshots stored by stage 5;
- the vertex-distribution ROOT file and its checksum;
- the final MuDst/PicoDst file list;
- the QA report.

Do not mix files from different submission tags merely because they have the
same number of events per job.

## Known limitations

- The current production depends on BNL/STAR infrastructure and Droy's shared
  SL16d_embed tree.
- Synthetic MoreTags reproduce the supplied vertex distributions and fixed run
  context; they do not reproduce all correlations of real Run14 events.
- `Vz` is sampled independently of `(Vx,Vy)` by construction.
- There is no real DAQ input and no heavy-ion underlying event in standalone
  reconstruction.
- Stage 1 compiles the frozen PYTHIA 8.303 source inside each job.
- Internal working filenames retain the historical
  `st_physics_15130045_raw_1000011` stem. Final filenames and metadata contain
  unique submission, seed, job, and event-ID information.

## Citation and contact

When using this workflow in an analysis, cite the relevant STAR software and
simulation documentation and record the exact repository commit. Project-level
physics choices should be confirmed with the analysis owner before production.
