#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TRandom.h"
#include "TRandom3.h"
#include "TMath.h"
#include "TSystem.h"
#include "TString.h"

#include <cstdio>

void makeSyntheticMoreTags_v1(
    const char *pythiaFileName,
    const char *vertexFileName,
    const char *outputFileName,
    Int_t firstEvtId = 0,
    UInt_t randomSeed = 12345
)
{
    // Number of required MoreTags entries comes directly from PYTHIA.
    TFile *pythiaFile = TFile::Open(pythiaFileName, "READ");
    if (!pythiaFile || pythiaFile->IsZombie()) {
        fprintf(stderr, "ERROR: Cannot open PYTHIA file: %s\n",
                pythiaFileName);
        return;
    }

    TTree *d0Tree = (TTree*)pythiaFile->Get("D0Tree");
    if (!d0Tree) {
        fprintf(stderr, "ERROR: D0Tree not found in: %s\n",
                pythiaFileName);
        return;
    }

    const Long64_t nEvents = d0Tree->GetEntries();
    if (nEvents <= 0) {
        fprintf(stderr, "ERROR: D0Tree contains no entries.\n");
        return;
    }

    // Vertex distributions and fixed run metadata.
    TFile *vertexFile = TFile::Open(vertexFileName, "READ");
    if (!vertexFile || vertexFile->IsZombie()) {
        fprintf(stderr, "ERROR: Cannot open vertex file: %s\n",
                vertexFileName);
        return;
    }

    TH1D *hVtxZ = (TH1D*)vertexFile->Get("event/hVtxZ");
    TH2D *hVtxR = (TH2D*)vertexFile->Get("event/hVtxR");
    TTree *runContext =
        (TTree*)vertexFile->Get("metadata/RunContext");

    if (!hVtxZ || !hVtxR || !runContext) {
        fprintf(stderr,
                "ERROR: Missing event/hVtxZ, event/hVtxR, "
                "or metadata/RunContext.\n");
        return;
    }

    if (hVtxZ->Integral() <= 0.0 || hVtxR->Integral() <= 0.0) {
        fprintf(stderr, "ERROR: Vertex histogram has zero integral.\n");
        return;
    }

    const char *requiredMetadata[] = {
        "RunId",
        "SourceEvtId",
        "StarsimDateYYYYMMDD",
        "StarsimDbvYYYYMMDD",
        "MagFieldKGauss"
    };

    for (Int_t iMetaData = 0; iMetaData < 5; ++iMetaData) {
        if (!runContext->GetBranch(requiredMetadata[iMetaData])) {
            fprintf(stderr, "ERROR: Missing metadata branch: %s\n",
                    requiredMetadata[iMetaData]);
            return;
        }
    }

    Double_t runIdValue          = 0.0;
    Double_t sourceEvtIdValue    = 0.0;
    Double_t starsimDateValue    = 0.0;
    Double_t starsimDbvValue     = 0.0;
    Double_t magFieldValue       = 0.0;

    runContext->SetBranchAddress("RunId", &runIdValue);
    runContext->SetBranchAddress("SourceEvtId", &sourceEvtIdValue);
    runContext->SetBranchAddress("StarsimDateYYYYMMDD",
                                 &starsimDateValue);
    runContext->SetBranchAddress("StarsimDbvYYYYMMDD",
                                 &starsimDbvValue);
    runContext->SetBranchAddress("MagFieldKGauss",
                                 &magFieldValue);

    if (runContext->GetEntries() < 1 ||
        runContext->GetEntry(0) <= 0) {
        fprintf(stderr, "ERROR: Cannot read metadata/RunContext.\n");
        return;
    }

    Int_t runId = TMath::Nint(runIdValue);

    if (firstEvtId <= 0)
        firstEvtId = TMath::Nint(sourceEvtIdValue);

    const Long64_t lastEvtId64 =
        (Long64_t)firstEvtId + nEvents - 1;

    if (firstEvtId <= 0 || lastEvtId64 > 2147483647LL) {
        fprintf(stderr, "ERROR: EvtId range does not fit Int_t.\n");
        return;
    }

    Double_t evtTime =
    (Double_t)TMath::Nint(starsimDateValue);

    Double_t prodTime =
    (Double_t)TMath::Nint(starsimDbvValue);

    Double_t magField = magFieldValue;
    Double_t vx = 0.0;
    Double_t vy = 0.0;
    Double_t vz = 0.0;
    Int_t evtId = 0;

    TString outputDirectory = gSystem->DirName(outputFileName);
    if (outputDirectory.Length() > 0 &&
        outputDirectory != ".") {
        gSystem->mkdir(outputDirectory.Data(), kTRUE);
    }

    TFile *outputFile = TFile::Open(outputFileName, "RECREATE");
    if (!outputFile || outputFile->IsZombie()) {
        fprintf(stderr, "ERROR: Cannot create output: %s\n",
                outputFileName);
        return;
    }

    TTree *moreTags = new TTree("MoreTags", "MoreTags");

    moreTags->Branch("RunId",    &runId,    "RunId/I");
    moreTags->Branch("EvtId",    &evtId,    "EvtId/I");
    moreTags->Branch("EvtTime",  &evtTime,  "EvtTime/D");
    moreTags->Branch("ProdTime", &prodTime, "ProdTime/D");
    moreTags->Branch("magField", &magField, "magField/D");
    moreTags->Branch("VX",       &vx,       "VX/D");
    moreTags->Branch("VY",       &vy,       "VY/D");
    moreTags->Branch("VZ",       &vz,       "VZ/D");

    // TH1::GetRandom() and TH2::GetRandom2() use global gRandom.
    TRandom *previousRandom = gRandom;
    TRandom3 randomGenerator(randomSeed);
    gRandom = &randomGenerator;

    for (Long64_t iEvent = 0; iEvent < nEvents; ++iEvent) {
        evtId = firstEvtId + (Int_t)iEvent;

        // Joint sampling keeps the Vx–Vy correlation.
        hVtxR->GetRandom2(vx, vy);

        // Vz is intentionally sampled independently.
        vz = hVtxZ->GetRandom();

        moreTags->Fill();
    }

    gRandom = previousRandom;

    outputFile->cd();
    moreTags->Write();
    outputFile->Close();

    vertexFile->Close();
    pythiaFile->Close();

    printf("\nCreated synthetic MoreTags: %s\n", outputFileName);
    printf("Entries          = %lld\n", (Long64_t)nEvents);
    printf("RunId            = %d\n", runId);
    printf("EvtId range      = %d -- %lld\n",
           firstEvtId, lastEvtId64);
    printf("EvtTime          = %.15f\n", evtTime);
    printf("ProdTime         = %.15f\n", prodTime);
    printf("magField         = %.12f kG\n", magField);
    printf("randomSeed       = %u\n\n", randomSeed);
}
