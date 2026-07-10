// dumpMuDstBTofTruthQA_v1.C
//
// Diagnose BTOF truth information in a STAR MuDst.
// Intended for comparing:
//   (a) the original real-data MuDst
//   (b) the mixed/embedding MuDst used as input to PicoDst conversion
//
// The macro records StMuBTofHit::idTruth()/qaTruth() together with the
// associated global/primary track truth information, and prints event-level
// multiplicities for several candidate MC-only selections.
//
// Run in STAR SL22c, for example:
//   root4star -l -b -q \
//     'dumpMuDstBTofTruthQA_v1.C("input.MuDst.root","btof_truthQA.root",50)'

#include <iostream>
#include <algorithm>

#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TSystem.h"
#include "TROOT.h"

#ifndef __CINT__
#include "StChain.h"
#include "StMuDSTMaker/COMMON/StMuDstMaker.h"
#include "StMuDSTMaker/COMMON/StMuDst.h"
#include "StMuDSTMaker/COMMON/StMuEvent.h"
#include "StMuDSTMaker/COMMON/StMuBTofHit.h"
#include "StMuDSTMaker/COMMON/StMuTrack.h"
#endif

void dumpMuDstBTofTruthQA_v1(
    const char* inputFile,
    const char* outputFile = "btof_truthQA.root",
    Int_t maxEvents = 50
)
{
#ifdef __CINT__
    // Same ROOT5/CINT loading strategy used by the working MuDst QA macro.
    gROOT->Macro("loadMuDst.C");
#endif

    gSystem->Load("StMuDSTMaker");

    StChain* chain = new StChain("BTofTruthQAChain");

    StMuDstMaker* muDstMaker =
        new StMuDstMaker(
            0,          // read mode
            0,          // fixed-name mode
            "",         // directory
            inputFile,  // exact input file
            "",         // filter
            1000,       // max files
            "MuDst"
        );

    const Int_t initStatus = chain->Init();
    if (initStatus != kStOK) {
        std::cerr << "ERROR: chain->Init() failed with status "
                  << initStatus << std::endl;
        delete chain;
        return;
    }

    TChain* inputChain = muDstMaker->chain();
    if (!inputChain) {
        std::cerr << "ERROR: StMuDstMaker input TChain is null." << std::endl;
        delete chain;
        return;
    }

    const Long64_t nAvailable = inputChain->GetEntries();
    Long64_t nToRun = nAvailable;
    if (maxEvents >= 0) {
        nToRun = std::min<Long64_t>(nAvailable, maxEvents);
    }

    std::cout << "Input file: " << inputFile << std::endl;
    std::cout << "Available events: " << nAvailable << std::endl;
    std::cout << "Events to read: " << nToRun << std::endl;

    TFile* fout = TFile::Open(outputFile, "RECREATE");
    if (!fout || fout->IsZombie()) {
        std::cerr << "ERROR: cannot create output file "
                  << outputFile << std::endl;
        delete chain;
        return;
    }

    // ============================================================
    // Event-level tree
    // ============================================================
    TTree* tEvent = new TTree(
        "events",
        "Event-level BTOF truth diagnostics"
    );

    Int_t runId = 0;
    Int_t eventId = 0;

    UInt_t nHits = 0;
    UInt_t nPhysicalHits = 0;
    UInt_t nIdTruthPositive = 0;
    UInt_t nQaTruthPositive = 0;
    UInt_t nQaTruthEq1 = 0;
    UInt_t nQaTruthGe50 = 0;

    UInt_t nWithGlobalTrack = 0;
    UInt_t nGlobalTrackTruthPositive = 0;
    UInt_t nGlobalTrackQaGe50 = 0;
    UInt_t nSameTruthAsGlobal = 0;

    // Candidate selections to compare before modifying the Pico writer.
    UInt_t nCandHitIdPositive = 0;
    UInt_t nCandHitIdQaEq1 = 0;
    UInt_t nCandHitIdQaPositive = 0;
    UInt_t nCandGlobalMcTrack = 0;
    UInt_t nCandSameTruthGlobalMc = 0;
    UInt_t nCandStrict = 0;

    tEvent->Branch("runId", &runId, "runId/I");
    tEvent->Branch("eventId", &eventId, "eventId/I");
    tEvent->Branch("nHits", &nHits, "nHits/i");
    tEvent->Branch("nPhysicalHits", &nPhysicalHits, "nPhysicalHits/i");
    tEvent->Branch("nIdTruthPositive", &nIdTruthPositive, "nIdTruthPositive/i");
    tEvent->Branch("nQaTruthPositive", &nQaTruthPositive, "nQaTruthPositive/i");
    tEvent->Branch("nQaTruthEq1", &nQaTruthEq1, "nQaTruthEq1/i");
    tEvent->Branch("nQaTruthGe50", &nQaTruthGe50, "nQaTruthGe50/i");
    tEvent->Branch("nWithGlobalTrack", &nWithGlobalTrack, "nWithGlobalTrack/i");
    tEvent->Branch("nGlobalTrackTruthPositive", &nGlobalTrackTruthPositive, "nGlobalTrackTruthPositive/i");
    tEvent->Branch("nGlobalTrackQaGe50", &nGlobalTrackQaGe50, "nGlobalTrackQaGe50/i");
    tEvent->Branch("nSameTruthAsGlobal", &nSameTruthAsGlobal, "nSameTruthAsGlobal/i");

    tEvent->Branch("nCandHitIdPositive", &nCandHitIdPositive, "nCandHitIdPositive/i");
    tEvent->Branch("nCandHitIdQaEq1", &nCandHitIdQaEq1, "nCandHitIdQaEq1/i");
    tEvent->Branch("nCandHitIdQaPositive", &nCandHitIdQaPositive, "nCandHitIdQaPositive/i");
    tEvent->Branch("nCandGlobalMcTrack", &nCandGlobalMcTrack, "nCandGlobalMcTrack/i");
    tEvent->Branch("nCandSameTruthGlobalMc", &nCandSameTruthGlobalMc, "nCandSameTruthGlobalMc/i");
    tEvent->Branch("nCandStrict", &nCandStrict, "nCandStrict/i");

    // ============================================================
    // Hit-level tree
    // ============================================================
    TTree* tHit = new TTree(
        "btofHits",
        "BTOF hit truth and associated-track diagnostics"
    );

    Int_t hitIndex = -1;
    Int_t tray = -1;
    Int_t module = -1;
    Int_t cell = -1;

    Int_t hitIdTruth = 0;
    Int_t hitQaTruth = 0;

    Int_t associatedTrackId = -1;
    Int_t index2Global = -1;
    Int_t index2Primary = -1;

    Bool_t hasGlobalTrack = kFALSE;
    Int_t globalTrackId = -1;
    Int_t globalTrackIdTruth = 0;
    Int_t globalTrackQaTruth = 0;

    Bool_t hasPrimaryTrack = kFALSE;
    Int_t primaryTrackId = -1;
    Int_t primaryTrackIdTruth = 0;
    Int_t primaryTrackQaTruth = 0;

    Bool_t sameTruthAsGlobal = kFALSE;
    Bool_t sameTruthAsPrimary = kFALSE;

    Bool_t candHitIdPositive = kFALSE;
    Bool_t candHitIdQaEq1 = kFALSE;
    Bool_t candHitIdQaPositive = kFALSE;
    Bool_t candGlobalMcTrack = kFALSE;
    Bool_t candSameTruthGlobalMc = kFALSE;
    Bool_t candStrict = kFALSE;

    tHit->Branch("runId", &runId, "runId/I");
    tHit->Branch("eventId", &eventId, "eventId/I");
    tHit->Branch("hitIndex", &hitIndex, "hitIndex/I");
    tHit->Branch("tray", &tray, "tray/I");
    tHit->Branch("module", &module, "module/I");
    tHit->Branch("cell", &cell, "cell/I");
    tHit->Branch("hitIdTruth", &hitIdTruth, "hitIdTruth/I");
    tHit->Branch("hitQaTruth", &hitQaTruth, "hitQaTruth/I");
    tHit->Branch("associatedTrackId", &associatedTrackId, "associatedTrackId/I");
    tHit->Branch("index2Global", &index2Global, "index2Global/I");
    tHit->Branch("index2Primary", &index2Primary, "index2Primary/I");

    tHit->Branch("hasGlobalTrack", &hasGlobalTrack, "hasGlobalTrack/O");
    tHit->Branch("globalTrackId", &globalTrackId, "globalTrackId/I");
    tHit->Branch("globalTrackIdTruth", &globalTrackIdTruth, "globalTrackIdTruth/I");
    tHit->Branch("globalTrackQaTruth", &globalTrackQaTruth, "globalTrackQaTruth/I");

    tHit->Branch("hasPrimaryTrack", &hasPrimaryTrack, "hasPrimaryTrack/O");
    tHit->Branch("primaryTrackId", &primaryTrackId, "primaryTrackId/I");
    tHit->Branch("primaryTrackIdTruth", &primaryTrackIdTruth, "primaryTrackIdTruth/I");
    tHit->Branch("primaryTrackQaTruth", &primaryTrackQaTruth, "primaryTrackQaTruth/I");

    tHit->Branch("sameTruthAsGlobal", &sameTruthAsGlobal, "sameTruthAsGlobal/O");
    tHit->Branch("sameTruthAsPrimary", &sameTruthAsPrimary, "sameTruthAsPrimary/O");

    tHit->Branch("candHitIdPositive", &candHitIdPositive, "candHitIdPositive/O");
    tHit->Branch("candHitIdQaEq1", &candHitIdQaEq1, "candHitIdQaEq1/O");
    tHit->Branch("candHitIdQaPositive", &candHitIdQaPositive, "candHitIdQaPositive/O");
    tHit->Branch("candGlobalMcTrack", &candGlobalMcTrack, "candGlobalMcTrack/O");
    tHit->Branch("candSameTruthGlobalMc", &candSameTruthGlobalMc, "candSameTruthGlobalMc/O");
    tHit->Branch("candStrict", &candStrict, "candStrict/O");

    // Histograms are intentionally broad; exact values remain in the tree.
    TH1D* hHitIdTruth = new TH1D(
        "hHitIdTruth",
        "BTOF hit idTruth;idTruth;hits",
        500, -0.5, 499.5
    );

    TH1D* hHitQaTruth = new TH1D(
        "hHitQaTruth",
        "BTOF hit qaTruth;qaTruth;hits",
        202, -0.5, 201.5
    );

    TH2D* hHitQaVsIdTruth = new TH2D(
        "hHitQaVsIdTruth",
        "BTOF hit qaTruth vs idTruth;idTruth;qaTruth",
        200, -0.5, 199.5,
        102, -0.5, 101.5
    );

    // Aggregate counters.
    Long64_t totalHits = 0;
    Long64_t totalPhysicalHits = 0;
    Long64_t totalIdTruthPositive = 0;
    Long64_t totalQaTruthPositive = 0;
    Long64_t totalQaTruthEq1 = 0;
    Long64_t totalQaTruthGe50 = 0;
    Long64_t totalWithGlobalTrack = 0;
    Long64_t totalGlobalTrackTruthPositive = 0;
    Long64_t totalGlobalTrackQaGe50 = 0;
    Long64_t totalSameTruthAsGlobal = 0;
    Long64_t totalCandStrict = 0;

    Long64_t nProcessed = 0;

    for (Long64_t iEvent = 0; iEvent < nToRun; ++iEvent) {
        chain->Clear();

        const Int_t makeStatus = chain->Make(iEvent);
        if (makeStatus == kStEOF) break;
        if (makeStatus != kStOK) {
            std::cerr << "WARNING: chain->Make(" << iEvent
                      << ") returned " << makeStatus << std::endl;
            continue;
        }

        StMuDst* muDst = muDstMaker->muDst();
        if (!muDst) {
            std::cerr << "WARNING: null StMuDst at event "
                      << iEvent << std::endl;
            continue;
        }

        StMuEvent* muEvent = muDst->event();
        if (muEvent) {
            runId = muEvent->runId();
            eventId = muEvent->eventId();
        }
        else {
            runId = -1;
            eventId = -1;
        }

        nHits = muDst->numberOfBTofHit();
        nPhysicalHits = 0;
        nIdTruthPositive = 0;
        nQaTruthPositive = 0;
        nQaTruthEq1 = 0;
        nQaTruthGe50 = 0;
        nWithGlobalTrack = 0;
        nGlobalTrackTruthPositive = 0;
        nGlobalTrackQaGe50 = 0;
        nSameTruthAsGlobal = 0;

        nCandHitIdPositive = 0;
        nCandHitIdQaEq1 = 0;
        nCandHitIdQaPositive = 0;
        nCandGlobalMcTrack = 0;
        nCandSameTruthGlobalMc = 0;
        nCandStrict = 0;

        for (UInt_t iHit = 0; iHit < nHits; ++iHit) {
            StMuBTofHit* hit = muDst->btofHit(iHit);
            if (!hit) continue;

            hitIndex = static_cast<Int_t>(iHit);
            tray = hit->tray();
            module = hit->module();
            cell = hit->cell();

            hitIdTruth = hit->idTruth();
            hitQaTruth = hit->qaTruth();

            associatedTrackId = hit->associatedTrackId();
            index2Global = hit->index2Global();
            index2Primary = hit->index2Primary();

            hasGlobalTrack = kFALSE;
            globalTrackId = -1;
            globalTrackIdTruth = 0;
            globalTrackQaTruth = 0;

            StMuTrack* globalTrack = hit->globalTrack();
            if (globalTrack) {
                hasGlobalTrack = kTRUE;
                globalTrackId = globalTrack->id();
                globalTrackIdTruth = globalTrack->idTruth();
                globalTrackQaTruth = globalTrack->qaTruth();
            }

            hasPrimaryTrack = kFALSE;
            primaryTrackId = -1;
            primaryTrackIdTruth = 0;
            primaryTrackQaTruth = 0;

            StMuTrack* primaryTrack = hit->primaryTrack();
            if (primaryTrack) {
                hasPrimaryTrack = kTRUE;
                primaryTrackId = primaryTrack->id();
                primaryTrackIdTruth = primaryTrack->idTruth();
                primaryTrackQaTruth = primaryTrack->qaTruth();
            }

            sameTruthAsGlobal =
                hasGlobalTrack &&
                hitIdTruth > 0 &&
                globalTrackIdTruth == hitIdTruth;

            sameTruthAsPrimary =
                hasPrimaryTrack &&
                hitIdTruth > 0 &&
                primaryTrackIdTruth == hitIdTruth;

            // Candidate selections. These are diagnostics, not assumptions.
            candHitIdPositive = (hitIdTruth > 0);
            candHitIdQaEq1 = (hitIdTruth > 0 && hitQaTruth == 1);
            candHitIdQaPositive = (hitIdTruth > 0 && hitQaTruth > 0);
            candGlobalMcTrack =
                hasGlobalTrack &&
                globalTrackIdTruth > 0 &&
                globalTrackQaTruth >= 50;
            candSameTruthGlobalMc =
                candGlobalMcTrack &&
                sameTruthAsGlobal;
            candStrict =
                candSameTruthGlobalMc &&
                hitQaTruth == 1;

            const Bool_t physicalHit =
                tray >= 1 && tray <= 120 &&
                module >= 1 && module <= 32 &&
                cell >= 1 && cell <= 6;

            if (physicalHit) ++nPhysicalHits;
            if (hitIdTruth > 0) ++nIdTruthPositive;
            if (hitQaTruth > 0) ++nQaTruthPositive;
            if (hitQaTruth == 1) ++nQaTruthEq1;
            if (hitQaTruth >= 50) ++nQaTruthGe50;

            if (hasGlobalTrack) ++nWithGlobalTrack;
            if (hasGlobalTrack && globalTrackIdTruth > 0) {
                ++nGlobalTrackTruthPositive;
            }
            if (hasGlobalTrack && globalTrackQaTruth >= 50) {
                ++nGlobalTrackQaGe50;
            }
            if (sameTruthAsGlobal) ++nSameTruthAsGlobal;

            if (candHitIdPositive) ++nCandHitIdPositive;
            if (candHitIdQaEq1) ++nCandHitIdQaEq1;
            if (candHitIdQaPositive) ++nCandHitIdQaPositive;
            if (candGlobalMcTrack) ++nCandGlobalMcTrack;
            if (candSameTruthGlobalMc) ++nCandSameTruthGlobalMc;
            if (candStrict) ++nCandStrict;

            hHitIdTruth->Fill(hitIdTruth);
            hHitQaTruth->Fill(hitQaTruth);
            hHitQaVsIdTruth->Fill(hitIdTruth, hitQaTruth);

            tHit->Fill();
        }

        tEvent->Fill();

        totalHits += nHits;
        totalPhysicalHits += nPhysicalHits;
        totalIdTruthPositive += nIdTruthPositive;
        totalQaTruthPositive += nQaTruthPositive;
        totalQaTruthEq1 += nQaTruthEq1;
        totalQaTruthGe50 += nQaTruthGe50;
        totalWithGlobalTrack += nWithGlobalTrack;
        totalGlobalTrackTruthPositive += nGlobalTrackTruthPositive;
        totalGlobalTrackQaGe50 += nGlobalTrackQaGe50;
        totalSameTruthAsGlobal += nSameTruthAsGlobal;
        totalCandStrict += nCandStrict;

        std::cout
            << "BTofTruthQA event " << iEvent
            << " run=" << runId
            << " evt=" << eventId
            << " hits=" << nHits
            << " id>0=" << nIdTruthPositive
            << " qa>0=" << nQaTruthPositive
            << " qa==1=" << nQaTruthEq1
            << " qa>=50=" << nQaTruthGe50
            << " gTrk=" << nWithGlobalTrack
            << " gTrkMC=" << nGlobalTrackQaGe50
            << " sameTruth=" << nSameTruthAsGlobal
            << " strict=" << nCandStrict
            << std::endl;

        ++nProcessed;
    }

    fout->cd();
    tEvent->Write();
    tHit->Write();
    hHitIdTruth->Write();
    hHitQaTruth->Write();
    hHitQaVsIdTruth->Write();
    fout->Close();

    std::cout << std::endl;
    std::cout << "===== BTOF TRUTH QA SUMMARY =====" << std::endl;
    std::cout << "Processed events              = " << nProcessed << std::endl;
    std::cout << "Total BTOF hits               = " << totalHits << std::endl;
    std::cout << "Physical BTOF hits            = " << totalPhysicalHits << std::endl;
    std::cout << "Hit idTruth > 0               = " << totalIdTruthPositive << std::endl;
    std::cout << "Hit qaTruth > 0               = " << totalQaTruthPositive << std::endl;
    std::cout << "Hit qaTruth == 1              = " << totalQaTruthEq1 << std::endl;
    std::cout << "Hit qaTruth >= 50             = " << totalQaTruthGe50 << std::endl;
    std::cout << "Hits with global track        = " << totalWithGlobalTrack << std::endl;
    std::cout << "Global track idTruth > 0      = " << totalGlobalTrackTruthPositive << std::endl;
    std::cout << "Global track qaTruth >= 50    = " << totalGlobalTrackQaGe50 << std::endl;
    std::cout << "Hit/global same truth ID      = " << totalSameTruthAsGlobal << std::endl;
    std::cout << "Strict candidate              = " << totalCandStrict << std::endl;
    std::cout << "=================================" << std::endl;

    delete chain;
}
