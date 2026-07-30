# Remove personal-directory runtime dependency

This package installs:

- `src/standalone/runBfc.C`
  - copied from the already audited local snapshot
    `work/droy_audit/runBfc_droy_reference.C`
  - required MD5:
    `88c1f94daf6e7b4e20389943df27ce73`
- `scripts/setup_SL16d2_D0Embedding_hybrid_v2.csh`
- `templates/standalone/drivers/05_standalone_reco_v2.csh`
- `scripts/run_standalone_production_v2.csh`
  - generated from the existing v1 wrapper with the Stage 5 v2 template selected

The old v1 workflow is retained until the new smoke test succeeds.

Install from the project root with:

```csh
tcsh RemoveDroyDependency_v1/install_remove_droy_dependency_v1.csh "$cwd"
```
