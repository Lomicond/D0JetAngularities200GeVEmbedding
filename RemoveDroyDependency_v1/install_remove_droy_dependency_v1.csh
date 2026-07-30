#!/bin/tcsh -f

set nonomatch

if ($#argv >= 1) then
    set PROJECT = "$argv[1]"
else
    set PROJECT = "$cwd"
endif

set PACKAGE_ROOT = `dirname "$0"`
set PACKAGE_ROOT = `cd "$PACKAGE_ROOT" && pwd`

if (! -d "$PROJECT/src") then
    echo "ERROR: project root does not contain src/:"
    echo "       $PROJECT"
    exit 2
endif

set AUDIT_RUNBFC = \
"$PROJECT/work/droy_audit/runBfc_droy_reference.C"

if (! -f "$AUDIT_RUNBFC") then
    echo "ERROR: audited runBfc copy was not found:"
    echo "       $AUDIT_RUNBFC"
    exit 3
endif

set RUNBFC_MD5 = `md5sum "$AUDIT_RUNBFC" | awk '{print $1}'`

if ("$RUNBFC_MD5" != "88c1f94daf6e7b4e20389943df27ce73") then
    echo "ERROR: unexpected runBfc.C checksum:"
    echo "       found    = $RUNBFC_MD5"
    echo "       expected = 88c1f94daf6e7b4e20389943df27ce73"
    exit 4
endif

mkdir -p "$PROJECT/src/standalone"
cp -p "$AUDIT_RUNBFC" \
    "$PROJECT/src/standalone/runBfc.C"

cp -p \
"$PACKAGE_ROOT/scripts/setup_SL16d2_D0Embedding_hybrid_v2.csh" \
"$PROJECT/scripts/setup_SL16d2_D0Embedding_hybrid_v2.csh"

cp -p \
"$PACKAGE_ROOT/templates/standalone/drivers/05_standalone_reco_v2.csh" \
"$PROJECT/templates/standalone/drivers/05_standalone_reco_v2.csh"

chmod +x \
"$PROJECT/scripts/setup_SL16d2_D0Embedding_hybrid_v2.csh" \
"$PROJECT/templates/standalone/drivers/05_standalone_reco_v2.csh"

set WRAPPER_V1 = \
"$PROJECT/scripts/run_standalone_production_v1.csh"

set WRAPPER_V2 = \
"$PROJECT/scripts/run_standalone_production_v2.csh"

if (! -f "$WRAPPER_V1") then
    echo "ERROR: source wrapper is missing:"
    echo "       $WRAPPER_V1"
    exit 5
endif

sed \
    -e 's|run_standalone_production_v1|run_standalone_production_v2|g' \
    -e 's|05_standalone_reco_v1\.root4star\.log|05_standalone_reco_v2.root4star.log|g' \
    -e 's|05_standalone_reco\.csh|05_standalone_reco_v2.csh|g' \
    "$WRAPPER_V1" \
    >! "$WRAPPER_V2"

chmod +x "$WRAPPER_V2"

echo "===== INSTALLED FILES ====="
ls -lh \
"$PROJECT/src/standalone/runBfc.C" \
"$PROJECT/scripts/setup_SL16d2_D0Embedding_hybrid_v2.csh" \
"$PROJECT/templates/standalone/drivers/05_standalone_reco_v2.csh" \
"$WRAPPER_V2"

echo
echo "===== CHECKSUM ====="
md5sum "$PROJECT/src/standalone/runBfc.C"

echo
echo "===== NEW RUNTIME REFERENCES ====="
grep -RInI -E \
'droy|Droy|/gpfs01/star/pwg/droy1|/star/u/droy1' \
"$PROJECT/src/standalone/runBfc.C" \
"$PROJECT/scripts/setup_SL16d2_D0Embedding_hybrid_v2.csh" \
"$PROJECT/templates/standalone/drivers/05_standalone_reco_v2.csh" \
"$WRAPPER_V2"

if ($status == 0) then
    echo "ERROR: a personal-directory reference remains in the new workflow"
    exit 6
endif

echo "No personal-directory references found in the new workflow."

echo
echo "===== SYNTAX CHECK ====="
tcsh -n "$PROJECT/scripts/setup_SL16d2_D0Embedding_hybrid_v2.csh"
set RC1 = $status

tcsh -n "$PROJECT/templates/standalone/drivers/05_standalone_reco_v2.csh"
set RC2 = $status

tcsh -n "$WRAPPER_V2"
set RC3 = $status

echo "setup v2 status  = $RC1"
echo "stage 5 v2 status = $RC2"
echo "wrapper v2 status = $RC3"

if ($RC1 != 0 || $RC2 != 0 || $RC3 != 0) then
    echo "ERROR: at least one syntax check failed"
    exit 7
endif

echo
echo "Installation completed."
echo "The v1 workflow was left untouched for comparison."

exit 0
