# Optional full-embedding driver templates

These files preserve the optional complete workflow in addition to the default
standalone production:

1. PYTHIA
2. MoreTags
3. DAQ chopping — `drivers/03_chop.csh`
4. STARSIM/GEANT3
5. real-event Mixer/reconstruction — `drivers/05_mixer.csh`
6. PicoDst production — `drivers/06_pico.csh`

The default `scripts/run_standalone_production_v1.csh` does not use these
drivers.

## Provenance

- Stage 3 follows the previously generated `03_chop.csh`: STAR `SL22c`,
  event numbers from the second column of the chopper list, and
  `daqFileChopper ... -eventnum`.
- Stage 5 preserves the later working `SL16d_embed2` driver, including the
  local compatibility libraries and explicit ROOT library loads.
- Stage 6 preserves the working Pico conversion setup and reads the MuDst
  produced by the Mixer.

The historical full-embedding path itself has not been revalidated after the
repository cleanup. These are retained as restoration-ready templates.

## Required substitutions

Before execution, replace:

- `@D0WF@` — project root
- `@SIMJOB@` — generated full-embedding job directory
- `@NEVENTS@` — event count
- `@DAQFILE@` — original input DAQ file, Stage 3 only

The intended generated job layout is:

```text
@SIMJOB@/
├── moretags/
├── chopped/
├── starsim/
├── mixer/
└── pico/
```
