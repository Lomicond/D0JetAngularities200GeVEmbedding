#include "comparePicoDstSamples_v15.C"

#include <cstdio>
#include <map>
#include <math.h>

// ============================================================================
// comparePicoDstSamples_v16.C
//
// Track-quality comparison for truth-matched reconstructed tracks.  Four
// unit-area distributions are written and plotted with a new/reference ratio:
//   * signed nHitsFit (the sign stores the reconstructed charge),
//   * |nHitsFit| (the actual number of fitted hits),
//   * nHitsMax,
//   * 3D DCA between Track.mOrigin and the event primary vertex.
//
// Preselection:
//   Track.mIdTruth > 0, Track.mQATruth >= 50, a corresponding MC-track ID,
//   reconstructed pT >= 0.2 GeV/c, and |eta| < 1.
//
// No nHitsFit, nHitsMax, hit-ratio, or DCA cut is applied: these are the
// quantities being diagnosed.
//
// Outputs for OUT:
//   OUT_track_quality_v16.pdf
//   OUT_track_quality_v16.root
//
// ROOT 5.34/CINT compatible; intended for STAR SL22c.
// ============================================================================

namespace PicoQaV16 {

struct TrackQualitySampleV16 {
    TString prefix;
    TString label;
    Long64_t nEvents;
    Long64_t nSelected;
    Long64_t nNegativeNHitsFit;
    Long64_t nZeroNHitsFit;
    Long64_t nDcaOverflow;

    TH1D *hNHitsFitSigned;
    TH1D *hNHitsFitAbs;
    TH1D *hNHitsMax;
    TH1D *hDca3D;
};

void initTrackQualitySampleV16(TrackQualitySampleV16 &s,
                               const char *prefix,
                               const char *label)
{
    using namespace PicoQaV11;

    s.prefix = prefix;
    s.label = label;
    s.nEvents = 0;
    s.nSelected = 0;
    s.nNegativeNHitsFit = 0;
    s.nZeroNHitsFit = 0;
    s.nDcaOverflow = 0;

    s.hNHitsFitSigned = hist1V11(
        s.prefix, "nhitsfit_signed", 111, -55.5, 55.5);
    s.hNHitsFitAbs = hist1V11(
        s.prefix, "nhitsfit_abs", 56, -0.5, 55.5);
    s.hNHitsMax = hist1V11(
        s.prefix, "nhitsmax", 56, -0.5, 55.5);
    s.hDca3D = hist1V11(
        s.prefix, "dca_3d_cm", 200, 0.0, 10.0);
}

void reportLeafV16(const char *sample,
                   const char *logicalName,
                   TLeaf *leaf)
{
    printf("V16LEAF sample=%-10s %-27s : %s\n",
           sample, logicalName, leaf ? leaf->GetName() : "MISSING");
}

Bool_t fillTrackQualitySampleV16(TTree *tree, TrackQualitySampleV16 &s)
{
    using namespace PicoQaV9;
    using namespace PicoQaV11;

    if (!tree) return kFALSE;

    TLeaf *eventVx = leafV9(tree, "Event.mPrimaryVertexX");
    TLeaf *eventVy = leafV9(tree, "Event.mPrimaryVertexY");
    TLeaf *eventVz = leafV9(tree, "Event.mPrimaryVertexZ");

    TLeaf *trackPx = leafV9(tree, "Track.mGMomentumX");
    TLeaf *trackPy = leafV9(tree, "Track.mGMomentumY");
    TLeaf *trackPz = leafV9(tree, "Track.mGMomentumZ");
    TLeaf *trackOriginX = leafV9(tree, "Track.mOriginX");
    TLeaf *trackOriginY = leafV9(tree, "Track.mOriginY");
    TLeaf *trackOriginZ = leafV9(tree, "Track.mOriginZ");
    TLeaf *trackNHitsFit = leafV9(tree, "Track.mNHitsFit");
    TLeaf *trackNHitsMax = leafV9(tree, "Track.mNHitsMax");
    TLeaf *trackIdTruth = leafV9(tree, "Track.mIdTruth");
    TLeaf *trackQATruth = leafV9(tree, "Track.mQATruth");

    TLeaf *mcId = leafV9(tree, "McTrack.mId");
    TLeaf *mcGePid = leafV9(tree,
                            "McTrack.mGePid", "McTrack.mGeantId");

    reportLeafV16(s.label.Data(), "Track momentum", trackPx);
    reportLeafV16(s.label.Data(), "Track signed nHitsFit", trackNHitsFit);
    reportLeafV16(s.label.Data(), "Track nHitsMax", trackNHitsMax);
    reportLeafV16(s.label.Data(), "Track origin", trackOriginX);
    reportLeafV16(s.label.Data(), "Event primary vertex", eventVx);
    reportLeafV16(s.label.Data(), "Track idTruth", trackIdTruth);
    reportLeafV16(s.label.Data(), "Track qaTruth", trackQATruth);
    reportLeafV16(s.label.Data(), "McTrack ID", mcId);
    reportLeafV16(s.label.Data(), "McTrack GEANT PID", mcGePid);

    if (!eventVx || !eventVy || !eventVz ||
        !trackPx || !trackPy || !trackPz ||
        !trackOriginX || !trackOriginY || !trackOriginZ ||
        !trackNHitsFit || !trackNHitsMax ||
        !trackIdTruth || !trackQATruth || !mcGePid) {
        printf("V16ERROR sample=%s reason=missing_essential_leaf\n",
               s.label.Data());
        return kFALSE;
    }

    s.nEvents = tree->GetEntries();
    printf("V16READ sample=%s events=%lld\n",
           s.label.Data(), s.nEvents);

    for (Long64_t iEvent = 0; iEvent < s.nEvents; ++iEvent) {
        tree->GetEntry(iEvent);
        if (iEvent % 100 == 0) {
            printf("  V16 %s event %lld / %lld\n",
                   s.label.Data(), iEvent, s.nEvents);
        }

        std::map<Int_t, Int_t> mcIndex;
        const Int_t nMc = mcGePid->GetNdata();
        for (Int_t iMc = 0; iMc < nMc; ++iMc) {
            const Int_t id = mcId ?
                (Int_t)(valueV9(mcId, iMc, iMc + 1) + 0.5) : iMc + 1;
            mcIndex[id] = iMc;
        }

        const Double_t vertexX = valueV9(eventVx, 0);
        const Double_t vertexY = valueV9(eventVy, 0);
        const Double_t vertexZ = valueV9(eventVz, 0);

        const Int_t nTracks = trackPx->GetNdata();
        for (Int_t iTrack = 0; iTrack < nTracks; ++iTrack) {
            const Int_t idTruth =
                (Int_t)(valueV9(trackIdTruth, iTrack) + 0.5);
            const Double_t qaTruth = valueV9(trackQATruth, iTrack);
            if (idTruth <= 0 || qaTruth < kMinQaTruthV11) continue;
            if (mcIndex.find(idTruth) == mcIndex.end()) continue;

            const Double_t px = valueV9(trackPx, iTrack);
            const Double_t py = valueV9(trackPy, iTrack);
            const Double_t pz = valueV9(trackPz, iTrack);
            const Double_t pt = sqrt(px * px + py * py);
            const Double_t eta = etaV9(px, py, pz);
            if (pt < kMinRecoPtV11 || !finiteV9(eta) ||
                fabs(eta) >= kMaxRecoEtaV11) continue;

            const Double_t signedNHitsFit =
                valueV9(trackNHitsFit, iTrack);
            const Double_t nHitsMax = valueV9(trackNHitsMax, iTrack);
            const Double_t dx =
                valueV9(trackOriginX, iTrack) - vertexX;
            const Double_t dy =
                valueV9(trackOriginY, iTrack) - vertexY;
            const Double_t dz =
                valueV9(trackOriginZ, iTrack) - vertexZ;
            const Double_t dca3D = sqrt(dx * dx + dy * dy + dz * dz);

            if (!finiteV9(signedNHitsFit) || !finiteV9(nHitsMax) ||
                !finiteV9(dca3D)) continue;

            s.hNHitsFitSigned->Fill(signedNHitsFit);
            s.hNHitsFitAbs->Fill(fabs(signedNHitsFit));
            s.hNHitsMax->Fill(nHitsMax);
            s.hDca3D->Fill(dca3D);
            ++s.nSelected;
            if (signedNHitsFit < 0.0) ++s.nNegativeNHitsFit;
            else if (signedNHitsFit == 0.0) ++s.nZeroNHitsFit;
            if (dca3D >= 10.0) ++s.nDcaOverflow;
        }
    }

    return kTRUE;
}

void deriveTrackQualitySampleV16(TrackQualitySampleV16 &s)
{
    using namespace PicoQaV9;
    unitAreaV9(s.hNHitsFitSigned);
    unitAreaV9(s.hNHitsFitAbs);
    unitAreaV9(s.hNHitsMax);
    unitAreaV9(s.hDca3D);
}

void printTrackQualitySampleV16(const TrackQualitySampleV16 &s)
{
    const Double_t negativeFraction = s.nSelected > 0 ?
        (Double_t)s.nNegativeNHitsFit / (Double_t)s.nSelected : 0.0;
    const Double_t dcaOverflowFraction = s.nSelected > 0 ?
        (Double_t)s.nDcaOverflow / (Double_t)s.nSelected : 0.0;
    printf("V16QA sample=%s selected=%lld negative_nHitsFit=%lld "
           "negative_fraction=%.6f zero_nHitsFit=%lld "
           "mean_abs_nHitsFit=%.6f mean_nHitsMax=%.6f mean_DCA3D_cm=%.6f "
           "DCA_ge_10cm=%lld DCA_ge_10cm_fraction=%.6g\n",
           s.label.Data(), s.nSelected, s.nNegativeNHitsFit,
           negativeFraction, s.nZeroNHitsFit,
           s.hNHitsFitAbs->GetMean(), s.hNHitsMax->GetMean(),
           s.hDca3D->GetMean(), s.nDcaOverflow, dcaOverflowFraction);
}

} // namespace PicoQaV16

void comparePicoDstSamples_v16(
    const char *newFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_new_vs_reference")
{
    using namespace PicoQaV9;
    using namespace PicoQaV16;

    gStyle->SetOptStat(0);

    TFile *newFile = TFile::Open(newFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!newFile || newFile->IsZombie() ||
        !referenceFile || referenceFile->IsZombie()) {
        printf("V16ERROR cannot open one or both input files\n");
        return;
    }

    TTree *newTree = (TTree*)newFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!newTree || !referenceTree) {
        printf("V16ERROR PicoDst tree is missing in one or both files\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    TrackQualitySampleV16 a, b;
    initTrackQualitySampleV16(a, "newv16", "new");
    initTrackQualitySampleV16(b, "refv16", "reference");
    if (!fillTrackQualitySampleV16(newTree, a) ||
        !fillTrackQualitySampleV16(referenceTree, b)) {
        printf("V16ERROR track-quality comparison could not be filled\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    deriveTrackQualitySampleV16(a);
    deriveTrackQualitySampleV16(b);

    const TString rootName = Form("%s_track_quality_v16.root", outputBase);
    const TString pdfName = Form("%s_track_quality_v16.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V16ERROR cannot create output ROOT file %s\n",
               rootName.Data());
        newFile->Close();
        referenceFile->Close();
        return;
    }

    printf("V16CONFIG selection: truth matched, idTruth>0, qaTruth>=%.0f, "
           "reco pT>=%.3f GeV/c, |eta|<%.3f\n",
           PicoQaV11::kMinQaTruthV11,
           PicoQaV11::kMinRecoPtV11,
           PicoQaV11::kMaxRecoEtaV11);
    printf("V16CONFIG diagnostic variables have no hit-count, hit-ratio, or DCA cuts\n");
    printf("V16CONFIG DCA3D=sqrt((originX-vx)^2+(originY-vy)^2+(originZ-vz)^2)\n");
    printTrackQualitySampleV16(a);
    printTrackQualitySampleV16(b);

    drawV9(a.hNHitsFitSigned, b.hNHitsFitSigned,
           output, pdfName.Data(), "nhitsfit_signed",
           "Truth-matched tracks: signed nHitsFit (sign stores charge)",
           "signed nHitsFit", "fraction", kTRUE, 0.0, 2.0,
           kTRUE, kFALSE);
    drawV9(a.hNHitsFitAbs, b.hNHitsFitAbs,
           output, pdfName.Data(), "nhitsfit_abs",
           "Truth-matched tracks: |nHitsFit|",
           "|nHitsFit|", "fraction", kTRUE, 0.0, 2.0,
           kFALSE, kFALSE);
    drawV9(a.hNHitsMax, b.hNHitsMax,
           output, pdfName.Data(), "nhitsmax",
           "Truth-matched tracks: nHitsMax",
           "nHitsMax", "fraction", kTRUE, 0.0, 2.0,
           kFALSE, kFALSE);
    drawV9(a.hDca3D, b.hDca3D,
           output, pdfName.Data(), "dca_3d_cm",
           "Truth-matched tracks: 3D DCA to primary vertex",
           "DCA_{3D} (cm)", "fraction", kTRUE, 0.0, 2.0,
           kFALSE, kTRUE);

    output->Write();
    output->Close();
    newFile->Close();
    referenceFile->Close();

    printf("V16OUTPUT pdf=%s\n", pdfName.Data());
    printf("V16OUTPUT root=%s\n", rootName.Data());
}
