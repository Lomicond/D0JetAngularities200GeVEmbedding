// makePicoDstFromMuDst.C - Stage 6 v3
//
// Stage 6 of the D0EmbeddingClean workflow:
//   reconstructed embedding MuDst -> PicoDst
//
// This is a Run14-adapted wrapper based on STAR's public star-picoDst
// makePicoDst.C conversion macro.  It intentionally runs as a separate stage
// after the validated embedding mixer, rather than changing the mixer chain.
//
// ROOT5/CINT target: STAR SL22c.

class StMaker;
class StChain;
class StPicoDstMaker;
class StMuDstMaker;

void loadPicoDstLibraries_Run14()
{
    gSystem->Load("libTable");
    gSystem->Load("libPhysics");
    gSystem->Load("libSt_base");
    gSystem->Load("libStChain");
    gSystem->Load("libSt_Tables");
    gSystem->Load("libStUtilities");
    gSystem->Load("libStTreeMaker");
    gSystem->Load("libStIOMaker");
    gSystem->Load("libStarClassLibrary");
    gSystem->Load("libStTriggerDataMaker");
    gSystem->Load("libStBichsel");
    gSystem->Load("libStEvent");
    gSystem->Load("libStEventUtilities");
    gSystem->Load("libStDbLib");
    gSystem->Load("libStEmcUtil");
    gSystem->Load("libStTofUtil");
    gSystem->Load("libStPmdUtil");
    gSystem->Load("libStPreEclMaker");
    gSystem->Load("libStStrangeMuDstMaker");
    gSystem->Load("libStMuDSTMaker");
    gSystem->Load("libStarAgmlUtil");
    gSystem->Load("libStTpcDb");
    gSystem->Load("libStMcEvent");
    gSystem->Load("libStMcEventMaker");
    gSystem->Load("libStDaqLib");
    gSystem->Load("libgen_Tables");
    gSystem->Load("libsim_Tables");
    gSystem->Load("libglobal_Tables");

    gSystem->Load("libStEmcTriggerMaker");
    gSystem->Load("libStEmcRawMaker");
    gSystem->Load("libStEmcADCtoEMaker");
    gSystem->Load("libStEpcMaker");
    gSystem->Load("libStEmcSimulatorMaker");

    gSystem->Load("libStDbBroker");
    gSystem->Load("libStDetectorDbMaker");
    gSystem->Load("libStDbUtilities");
    gSystem->Load("libStEEmcUtil");
    gSystem->Load("libStEEmcDbMaker");
    gSystem->Load("libSt_db_Maker");

    gSystem->Load("libStTriggerUtilities");
    gSystem->Load("libStMagF");

    gSystem->Load("libStMtdUtil");
    gSystem->Load("libStMtdMatchMaker");
    gSystem->Load("libStMtdCalibMaker");

    gSystem->Load("libStPicoEvent");
    gSystem->Load("libStPicoDstMaker");
}

void loadPicoDstAgML_Run14(const char *geometryTag)
{
    gROOT->LoadMacro("bfc.C");
    bfc(0, "agml nodefault mysql");

    AgModule::SetStacker(new StarTGeoStacker());

    if (geometryTag && TString(geometryTag).Length() > 0) {
        StarGeometry::Construct(geometryTag);
    }
}

void makePicoDstFromMuDst(
    const Char_t *inputFile,
    Int_t nEvents = 100000,
    const Char_t *geometryTag = "y2014a"
)
{
    cout << "PicoDst Stage 6 configuration:" << endl
         << "  input MuDst   = " << (inputFile ? inputFile : "<null>") << endl
         << "  events        = " << nEvents << endl
         << "  geometry      = " << (geometryTag ? geometryTag : "<null>") << endl
         << "  vertex mode   = PicoVtxDefault" << endl;

    if (!inputFile || TString(inputFile).Length() == 0) {
        cout << "ERROR: PicoDst input MuDst path is empty." << endl;
        cout << "PicoDstStage6Summary requested=" << nEvents
             << " processed=0 written=0 errors=1" << endl;
        return;
    }

    if (gSystem->AccessPathName(inputFile)) {
        cout << "ERROR: PicoDst input MuDst is not accessible:" << endl
             << "  " << inputFile << endl;
        cout << "PicoDstStage6Summary requested=" << nEvents
             << " processed=0 written=0 errors=1" << endl;
        return;
    }

    if (nEvents <= 0) {
        cout << "ERROR: nEvents must be > 0, got " << nEvents << endl;
        cout << "PicoDstStage6Summary requested=" << nEvents
             << " processed=0 written=0 errors=1" << endl;
        return;
    }

    loadPicoDstLibraries_Run14();

    StChain *chain = new StChain();

    StMuDstMaker *muDstMaker =
        new StMuDstMaker(0, 0, "", inputFile, "MuDst", 1);

    // Follow STAR's public makePicoDst.C branch selection.
    muDstMaker->SetStatus("*", 0);
    muDstMaker->SetStatus("MuEvent", 1);
    muDstMaker->SetStatus("PrimaryVertices", 1);
    muDstMaker->SetStatus("PrimaryTracks", 1);
    muDstMaker->SetStatus("GlobalTracks", 1);
    muDstMaker->SetStatus("CovGlobTrack", 1);
    muDstMaker->SetStatus("BTof*", 1);
    muDstMaker->SetStatus("Emc*", 1);
    muDstMaker->SetStatus("MTD*", 1);

    // Preserve embedding MC truth in the output PicoDst.
    //
    // Important for SL22c: enabling the individual names
    // "StMuMcVertex" / "StMuMcTrack" is not sufficient here.  The tested
    // StMuDstMaker group selector "MCAll" enables both MC arrays, which are
    // then consumed by StPicoDstMaker to fill PicoDst McVertex and McTrack.
    muDstMaker->SetStatus("MCAll", 1);

    // STAR DB and makers used by the public picoDst conversion macro.
    //
    // FMS/FPS/FPOST support is intentionally omitted in this D0-jet workflow:
    // those detectors are not used by the midrapidity analysis and their Run14
    // DB initialization produces large amounts of irrelevant error output.
    // Tracks, vertices, BTOF, BEMC/EEMC, MTD, MC truth and PicoDst writing
    // remain enabled.
    St_db_Maker *dbMk =
        new St_db_Maker("db", "StarDb", "MySQL:StarDb", "$STAR/StarDb");
    dbMk->SetFlavor("physics+ofl");
    cout << "  DB flavor    = physics+ofl" << endl;

    StEEmcDbMaker *eemcDb = new StEEmcDbMaker();

    StEmcADCtoEMaker *adc2e = new StEmcADCtoEMaker();
    adc2e->setPrint(false);
    adc2e->saveAllStEvent(true);

    StPreEclMaker *preEcl = new StPreEclMaker();
    preEcl->setPrint(kFALSE);

    StEpcMaker *epc = new StEpcMaker();
    epc->setPrint(kFALSE);

#if 0
    // D0WF disabled whole StTriggerSimuMaker block.
    // BEMC hit reconstruction above stays enabled.
    // This avoids StTriggerSimuMaker/EEMC DSM threshold crashes in SL16d_embed2 hybrid Pico stage.
    StTriggerSimuMaker *trigSimu = new StTriggerSimuMaker();
    trigSimu->setMC(false);
    trigSimu->useBemc();
    // D0WF disabled: trigSimu->useEemc();
    trigSimu->useOfflineDB();
    trigSimu->bemc->setConfig(StBemcTriggerSimu::kOffline);
#endif

    StMagFMaker *magFMaker = new StMagFMaker();

    StMtdMatchMaker *mtdMatchMaker = new StMtdMatchMaker();
    StMtdCalibMaker *mtdCalibMaker = new StMtdCalibMaker("mtdcalib");

    // StPicoDstMaker derives its output name from the string passed here.
    // Pass only the MuDst basename, not the absolute path: the maker searches
    // for the first "st_" substring internally, and a work-directory name such
    // as "test_run_all" would otherwise confuse the output-name derivation.
    TString picoNameSource = gSystem->BaseName(inputFile);

    StPicoDstMaker *picoMaker =
        new StPicoDstMaker(
            StPicoDstMaker::IoWrite,
            picoNameSource.Data(),
            "picoDst"
        );

    picoMaker->SetAttr("PicoVtxMode", "PicoVtxVpdOrDefault");
  picoMaker->SetAttr("PicoCovMtxMode", "PicoCovMtxWrite");
  picoMaker->PrintAttr();

    Int_t initStatus = chain->Init();
    if (initStatus != kStOK) {
        cout << "ERROR: PicoDst chain Init returned " << initStatus << endl;
        cout << "PicoDstStage6Summary requested=" << nEvents
             << " processed=0 written=0 errors=1" << endl;
        delete chain;
        return;
    }

    cout << "PicoDst Stage 6: chain initialized." << endl;

    // STAR's public conversion macro constructs AgML geometry after Init().
    // For this Run14 workflow use the production-level Run14 tag.
    loadPicoDstAgML_Run14(geometryTag);

    Int_t processed = 0;
    Int_t errors = 0;

    for (Int_t iEvent = 0; iEvent < nEvents; ++iEvent) {
        if (iEvent % 100 == 0) {
            cout << "PicoDst Stage 6: processing event " << iEvent
                 << " / " << nEvents << endl;
        }

        chain->Clear();
        const Int_t makeStatus = chain->Make(iEvent);

        if (makeStatus == kStEOF) {
            cout << "PicoDst Stage 6: reached EOF at requested event "
                 << iEvent << endl;
            break;
        }

        if (makeStatus != kStOK) {
            cout << "ERROR: PicoDst chain Make(" << iEvent
                 << ") returned " << makeStatus << endl;
            ++errors;
            break;
        }

        ++processed;
    }

    Long64_t written = -1;
    if (picoMaker->tree()) {
        written = picoMaker->tree()->GetEntries();
    }

    const Int_t finishStatus = chain->Finish();
    if (finishStatus != kStOK) {
        cout << "ERROR: PicoDst chain Finish returned "
             << finishStatus << endl;
        ++errors;
    }

    cout << "PicoDst Stage 6 finished." << endl
         << "  requested MuDst events = " << nEvents << endl
         << "  processed MuDst events = " << processed << endl
         << "  written PicoDst events = " << written << endl
         << "  errors                 = " << errors << endl;

    cout << "PicoDstStage6Summary"
         << " requested=" << nEvents
         << " processed=" << processed
         << " written=" << written
         << " errors=" << errors
         << endl;

    delete chain;
}
