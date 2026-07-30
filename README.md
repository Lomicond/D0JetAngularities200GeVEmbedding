# D0 Embedding Workflow with PYTHIA8 and STAR

This repository (`D0EmbeddingClean`) contains the workflow used to generate \(D^0\)-tagged events with a standalone PYTHIA8 installation and to propagate them through the STAR simulation/reconstruction chain.

The current workflow is organized approximately as

1. standalone PYTHIA8 event generation,
2. MoreTags / real-event selection,
3. DAQ chopping,
4. STARSIM / GEANT3 transport,
5. embedding mixer and reconstruction,
6. PicoDst production and QA.

> **Important:** the standalone generator is built and run with a custom PYTHIA **8.317** installation.  
> STAR steps are run in the STAR software environment (currently **SL22c**) and should **not inherit the custom PYTHIA8 library paths**.

---

## Repository layout

Relevant files include

```text
config/
└── detroit.cmnd

src/
├── pythia/
│   └── make_d0_pythia8.cc
├── moretags/
│   └── makeMuDstQA_Run14.C
├── starsim/
│   └── starsim.D0toy.C
├── mixer/
│   ├── bfcMixer_Hft_D0toy.C
│   └── runMixerD0toy_ZB.C
└── pico/

scripts/
└── run_all.sh

work/
├── test_stage1/
└── test_run_all/
```

The exact directory content may evolve, but the standalone generator entry point is

```text
src/pythia/make_d0_pythia8.cc
```

and the Detroit tune configuration is stored in

```text
config/detroit.cmnd
```

---

## Physics configuration of the standalone generator

The current generator setup uses

- PYTHIA8,
- \(pp\) collisions at \(\sqrt{s}=200\) GeV,
- Detroit tune,
- `HardQCD:all = on`,
- \(\hat{p}_{T,\min}=3\) GeV/\(c\),
- `421:mayDecay = off`.

The event selection requires at least one accepted \(D^0\) or \(\bar{D}^0\) candidate satisfying

- \(1 < p_T^{D^0} < 25\) GeV/\(c\),
- \(|y_{D^0}| < 1\),
- \(|\eta_{D^0}| < 3\).

Stored final particles are restricted to

- final-state particles,
- \(|\eta| < 3\),
- neutrinos excluded.

For the authoritative and up-to-date selection logic, always check

```text
src/pythia/make_d0_pythia8.cc
```

## Running the full workflow

To run the complete workflow:

```bash
chmod +x run_all.sh
./run_all.sh
```

---

# Building the standalone PYTHIA8 generator

## Requirements

The standalone generator requires

- a C++ compiler with C++11 support,
- ROOT with `root-config`,
- PYTHIA8 with `pythia8-config`.

This workflow has been developed with a custom PYTHIA **8.317** installation.

---

## Option A: use author's existing PYTHIA 8.317 installation

The installation used during development is

```text
/gpfs01/star/pwg/lomicond/Ondrej/Jets/Alma9Pythia8/pythia8317-install
```

A different user should point directly to this **absolute path**, provided the filesystem is mounted and readable on the machine being used.

Do **not** use a path beginning with `~` when referring to another user's installation, because `~` expands to the home directory of the user running the command.

For example:

```bash
export PYTHIA8=/gpfs01/star/pwg/lomicond/Ondrej/Jets/Alma9Pythia8/pythia8317-install

export PATH="$PYTHIA8/bin:$PATH"
export LD_LIBRARY_PATH="$PYTHIA8/lib:$PYTHIA8/lib64:${LD_LIBRARY_PATH:-}"
export PYTHIA8DATA="$PYTHIA8/share/Pythia8/xmldoc"

hash -r
```

### Verify the environment

```bash
which pythia8-config
pythia8-config --version

echo "$PYTHIA8"
echo "$PYTHIA8DATA"
```

The expected PYTHIA version is

```text
8.317
```

and `which pythia8-config` should resolve inside

```text
$PYTHIA8/bin/
```

A useful additional check is

```bash
test -d "$PYTHIA8DATA" && echo "PYTHIA8DATA OK"
```

---

## Option B: use another local PYTHIA8 installation

Point the same variables to the installation prefix:

```bash
export PYTHIA8=/path/to/your/pythia8-install

export PATH="$PYTHIA8/bin:$PATH"
export LD_LIBRARY_PATH="$PYTHIA8/lib:$PYTHIA8/lib64:${LD_LIBRARY_PATH:-}"
export PYTHIA8DATA="$PYTHIA8/share/Pythia8/xmldoc"

hash -r
```

Then verify

```bash
which pythia8-config
pythia8-config --version
```

The workflow is validated with PYTHIA 8.317. Other versions may require separate validation.

---

## Compile

Run from the repository root:

```bash
mkdir -p build/bin

g++ -std=c++11 -O2 \
    src/pythia/make_d0_pythia8.cc \
    -o build/bin/make_d0_pythia8_8317 \
    $(pythia8-config --cxxflags --libs) \
    $(root-config --cflags --libs)
```

Equivalent compact command:

```bash
g++ -std=c++11 -O2 src/pythia/make_d0_pythia8.cc \
    -o build/bin/make_d0_pythia8_8317 \
    `pythia8-config --cxxflags --libs` \
    `root-config --cflags --libs`
```

### Check which PYTHIA library was linked

```bash
ldd build/bin/make_d0_pythia8_8317 | grep -i pythia
```

This is especially useful on systems where another PYTHIA installation is already available globally.

The resolved library should correspond to the intended 8.317 installation.

---

## Run

Run the executable from the repository root so that repository-relative configuration paths such as

```text
config/detroit.cmnd
```

remain resolvable:

```bash
./build/bin/make_d0_pythia8_8317
```

The current source code is the authoritative reference for the number of generated events, command-line handling, output naming, stored branches, and event-selection details.

---

# Recommended clean environment handling

The safest approach is to use the custom PYTHIA environment only for the standalone generation stage.

For example, start a dedicated shell:

```bash
bash
```

Inside that shell:

```bash
export PYTHIA8=/gpfs01/star/pwg/lomicond/Ondrej/Jets/Alma9Pythia8/pythia8317-install
export PATH="$PYTHIA8/bin:$PATH"
export LD_LIBRARY_PATH="$PYTHIA8/lib:$PYTHIA8/lib64:${LD_LIBRARY_PATH:-}"
export PYTHIA8DATA="$PYTHIA8/share/Pythia8/xmldoc"

hash -r

mkdir -p build/bin

g++ -std=c++11 -O2 \
    src/pythia/make_d0_pythia8.cc \
    -o build/bin/make_d0_pythia8_8317 \
    $(pythia8-config --cxxflags --libs) \
    $(root-config --cflags --libs)

./build/bin/make_d0_pythia8_8317
```

After generation, leave that shell:

```bash
exit
```

Then start the STAR workflow from a clean shell and load the required STAR environment, e.g. SL22c.

This avoids accidental mixing of

- the custom standalone PYTHIA 8.317 libraries, and
- the libraries expected by the STAR software stack.

---

# STAR simulation / embedding stages

After standalone event generation, the workflow continues through the STAR chain.

Relevant source files include

```text
src/moretags/makeMuDstQA_Run14.C
src/starsim/starsim.D0toy.C
src/mixer/bfcMixer_Hft_D0toy.C
src/mixer/runMixerD0toy_ZB.C
```

The high-level sequence is

```text
PYTHIA8 generation
        ↓
MoreTags / event selection
        ↓
DAQ chopping
        ↓
STARSIM / GEANT3
        ↓
Mixer / reconstruction
        ↓
PicoDst production and QA
```

The STAR stages should be executed in the corresponding STAR environment rather than in the custom standalone-PYTHIA shell.

Current development uses

```text
SL22c
```

for the STAR-side workflow.

---

# Troubleshooting

## `pythia8-config` points to the wrong installation

Check

```bash
which pythia8-config
pythia8-config --version
```

Then prepend the intended installation explicitly:

```bash
export PYTHIA8=/absolute/path/to/pythia8317-install
export PATH="$PYTHIA8/bin:$PATH"
hash -r
```

Check again:

```bash
which pythia8-config
pythia8-config --version
```

---

## Runtime error: PYTHIA shared library not found

Make sure the library directory is visible:

```bash
export LD_LIBRARY_PATH="$PYTHIA8/lib:$PYTHIA8/lib64:${LD_LIBRARY_PATH:-}"
```

Then inspect the executable:

```bash
ldd build/bin/make_d0_pythia8_8317 | grep -i pythia
```

---

## XML data directory not found

Set

```bash
export PYTHIA8DATA="$PYTHIA8/share/Pythia8/xmldoc"
```

and verify

```bash
ls "$PYTHIA8DATA"
```

---

## Generator works, but STAR macros behave strangely

Check that the STAR shell is not inheriting the custom standalone PYTHIA setup:

```bash
echo "$PYTHIA8"
echo "$PYTHIA8DATA"
which pythia8-config
echo "$LD_LIBRARY_PATH"
```

The recommended fix is to run the STAR stage from a new clean shell rather than manually trying to undo every modified environment variable.

---

## Wrong tune/configuration file path

Run the generator from the repository root and verify

```bash
ls config/detroit.cmnd
```

---

# Reproducibility notes

For reproducible production, record at least

- git commit hash,
- PYTHIA version,
- ROOT version,
- STAR library version,
- generator configuration,
- number of requested and accepted events,
- random seed,
- input real-data file list used for embedding.

Useful commands:

```bash
git rev-parse HEAD
pythia8-config --version
root-config --version
```

For STAR productions, also record the active STAR environment, for example SL22c.

---

# Status

This is an analysis/development workflow. Physics selections and production details may still evolve.

Before large-scale production, validate

- generator-level \(D^0\) spectra,
- accepted-event fraction,
- particle content,
- STARSIM transport,
- detector acceptance maps,
- reconstructed distributions,
- consistency of the full chain with the reference STAR production.


## Local runtime dependencies

The following runtime components are intentionally not stored in Git:

- `external/pythia8_303/`
- `.sl73_x8664_gcc485/`
- `local_SL16d_embed2_facade/`
- `local_sl16d_bin/`
-- `.sl73_x8664_gcc485/`
- `local_SL16d `sl16d2_D0decay_overlay/`

The standalone workflow was validated with:

```text
scripts/run_standalone_production_v3.csh
