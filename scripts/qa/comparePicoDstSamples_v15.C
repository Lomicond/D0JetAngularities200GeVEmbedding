#include "comparePicoDstSamples_v14.C"

#include "TBranch.h"
#include "TLegend.h"
#include "TLine.h"

#include <cstdio>
#include <map>
#include <set>
#include <math.h>

// ============================================================================
// comparePicoDstSamples_v15.C
//
// Truth-matched tracking efficiency versus generated pT, separated into:
//   * charged pions:       GEANT IDs 8 and 9 combined;
//   * charged kaons:       GEANT IDs 11 and 12 combined;
//   * protons:             GEANT ID 14 only;
//   * antiprotons:         GEANT ID 15 only.
//
// Denominator:
//   MC particles from vertex 1, pT(MC) >= 0.2 GeV/c and |eta(MC)| < 1.
//
// Two numerator definitions are produced:
//   1) Droy-like/basic match: at least one unique reconstructed track with
//      Track.mIdTruth equal to the MC ID and Track.mQATruth >= 50;
//   2) analysis-quality match: the same unique truth match, with the reco
//      track additionally satisfying pT >= 0.2 GeV/c, |eta| < 1,
//      |DCA| < 3 cm (when leaves exist), |nHitsFit| >= 15, and
//      |nHitsFit|/nHitsMax >= 0.52 (when nHitsMax exists).
//
// std::set<Int_t> is used for both numerators so multiple reconstructed tracks
// matched to the same MC particle can never make the efficiency exceed unity.
//
// Outputs for OUT:
//   OUT_tracking_efficiency_species_v15.pdf
//   OUT_tracking_efficiency_species_v15.root
//
// ROOT 5.34/CINT compatible; intended for STAR SL22c.
// ============================================================================

namespace PicoQaV15 {

void enableLeafBranchV15(TTree *tree, TLeaf *leaf)
{
    if (!tree || !leaf) return;

    TBranch *branch = leaf->GetBranch();
    if (branch) {
        tree->SetBranchStatus(branch->GetName(), 1);
        TString branchName = branch->GetName();
        const Ssiz_t dot = branchName.Index(".");
        if (dot > 0) {
            TString counterName = branchName(0, dot);
            tree->SetBranchStatus(counterName.Data(), 1);
        }
    }

    TLeaf *countLeaf = leaf->GetLeafCount();
    if (countLeaf && countLeaf->GetBranch()) {
        tree->SetBranchStatus(countLeaf->GetBranch()->GetName(), 1);
    }
}

void configureInputBranchesV15(TTree *tree,
                               const char *sample,
                               TLeaf **leaves,
                               Int_t nLeaves)
{
    if (!tree) return;

    tree->SetCacheSize(0);
    tree->SetBranchStatus("*", 0);

    Int_t nPresent = 0;
    for (Int_t i = 0; i < nLeaves; ++i) {
        if (leaves[i]) {
            ++nPresent;
            enableLeafBranchV15(tree, leaves[i]);
        }
    }

    printf("V15IO sample=%s mode=selected_branches requested=%d present=%d cache=off\n",
           sample, nLeaves, nPresent);
}

const Int_t kNSpeciesV15 = 4;
const Int_t kPionV15 = 0;
const Int_t kKaonV15 = 1;
const Int_t kProtonV15 = 2;
const Int_t kAntiProtonV15 = 3;
const Double_t kMinMcPtV15 = 0.2;
const Double_t kMaxMcEtaV15 = 1.0;
const Int_t kNPtBinsV15 = 120;
const Double_t kMinPtAxisV15 = 0.0;
const Double_t kMaxPtAxisV15 = 12.0;

Int_t efficiencySpeciesV15(Int_t gePid)
{
    if (gePid == 8 || gePid == 9) return kPionV15;
    if (gePid == 11 || gePid == 12) return kKaonV15;
    if (gePid == 14) return kProtonV15;
    if (gePid == 15) return kAntiProtonV15;
    return -1;
}

struct EfficiencySampleV15 {
    TString prefix;
    TString label;
    Long64_t nEvents;
    Long64_t nDenominator[kNSpeciesV15];
    Long64_t nBasicMatched[kNSpeciesV15];
    Long64_t nQualityMatched[kNSpeciesV15];

    TH1D *hDenominator[kNSpeciesV15];
    TH1D *hBasicNumerator[kNSpeciesV15];
    TH1D *hQualityNumerator[kNSpeciesV15];
    TH1D *hBasicEfficiency[kNSpeciesV15];
    TH1D *hQualityEfficiency[kNSpeciesV15];

    Bool_t hasMcStartVertex;
    Bool_t appliesDcaCut;
    Bool_t appliesHitsRatioCut;
};

void initEfficiencySampleV15(EfficiencySampleV15 &s,
                             const char *prefix,
                             const char *label)
{
    using namespace PicoQaV11;

    s.prefix = prefix;
    s.label = label;
    s.nEvents = 0;
    s.hasMcStartVertex = kFALSE;
    s.appliesDcaCut = kFALSE;
    s.appliesHitsRatioCut = kFALSE;

    for (Int_t species = 0; species < kNSpeciesV15; ++species) {
        TString key = "invalid";
        if (species == kPionV15) key = "pion";
        else if (species == kKaonV15) key = "kaon";
        else if (species == kProtonV15) key = "proton";
        else if (species == kAntiProtonV15) key = "antiproton";
        s.nDenominator[species] = 0;
        s.nBasicMatched[species] = 0;
        s.nQualityMatched[species] = 0;

        const TString denominatorKey =
            Form("%s_efficiency_denominator_pt", key.Data());
        const TString basicNumeratorKey =
            Form("%s_basic_numerator_pt", key.Data());
        const TString qualityNumeratorKey =
            Form("%s_quality_numerator_pt", key.Data());
        const TString basicEfficiencyKey =
            Form("%s_basic_efficiency_pt", key.Data());
        const TString qualityEfficiencyKey =
            Form("%s_quality_efficiency_pt", key.Data());

        s.hDenominator[species] = hist1V11(
            s.prefix, denominatorKey.Data(),
            kNPtBinsV15, kMinPtAxisV15, kMaxPtAxisV15);
        s.hBasicNumerator[species] = hist1V11(
            s.prefix, basicNumeratorKey.Data(),
            kNPtBinsV15, kMinPtAxisV15, kMaxPtAxisV15);
        s.hQualityNumerator[species] = hist1V11(
            s.prefix, qualityNumeratorKey.Data(),
            kNPtBinsV15, kMinPtAxisV15, kMaxPtAxisV15);
        s.hBasicEfficiency[species] = hist1V11(
            s.prefix, basicEfficiencyKey.Data(),
            kNPtBinsV15, kMinPtAxisV15, kMaxPtAxisV15);
        s.hQualityEfficiency[species] = hist1V11(
            s.prefix, qualityEfficiencyKey.Data(),
            kNPtBinsV15, kMinPtAxisV15, kMaxPtAxisV15);
    }
}

void reportLeafV15(const char *sample,
                   const char *logicalName,
                   TLeaf *leaf)
{
    printf("V15LEAF sample=%-10s %-27s : %s\n",
           sample, logicalName, leaf ? leaf->GetName() : "MISSING");
}

Bool_t fillEfficiencySampleV15(TTree *tree, EfficiencySampleV15 &s)
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
    TLeaf *mcPx = leafV9(tree, "McTrack.mPx");
    TLeaf *mcPy = leafV9(tree, "McTrack.mPy");
    TLeaf *mcPz = leafV9(tree, "McTrack.mPz");
    TLeaf *mcStartVtx = leafV9(tree,
                               "McTrack.mIdVtxStart", "McTrack.mIdVx");

    reportLeafV15(s.label.Data(), "Track momentum", trackPx);
    reportLeafV15(s.label.Data(), "Track signed nHitsFit", trackNHitsFit);
    reportLeafV15(s.label.Data(), "Track nHitsMax", trackNHitsMax);
    reportLeafV15(s.label.Data(), "Track origin", trackOriginX);
    reportLeafV15(s.label.Data(), "Event primary vertex", eventVx);
    reportLeafV15(s.label.Data(), "Track idTruth", trackIdTruth);
    reportLeafV15(s.label.Data(), "Track qaTruth", trackQATruth);
    reportLeafV15(s.label.Data(), "McTrack ID", mcId);
    reportLeafV15(s.label.Data(), "McTrack GEANT PID", mcGePid);
    reportLeafV15(s.label.Data(), "McTrack momentum", mcPx);
    reportLeafV15(s.label.Data(), "McTrack start vertex", mcStartVtx);

    if (!trackPx || !trackPy || !trackPz || !trackNHitsFit ||
        !trackIdTruth || !mcGePid || !mcPx || !mcPy || !mcPz ||
        !mcStartVtx) {
        printf("V15ERROR sample=%s reason=missing_essential_leaf\n",
               s.label.Data());
        return kFALSE;
    }

    TLeaf *inputLeaves[20];
    Int_t nInputLeaves = 0;
    inputLeaves[nInputLeaves++] = eventVx;
    inputLeaves[nInputLeaves++] = eventVy;
    inputLeaves[nInputLeaves++] = eventVz;
    inputLeaves[nInputLeaves++] = trackPx;
    inputLeaves[nInputLeaves++] = trackPy;
    inputLeaves[nInputLeaves++] = trackPz;
    inputLeaves[nInputLeaves++] = trackOriginX;
    inputLeaves[nInputLeaves++] = trackOriginY;
    inputLeaves[nInputLeaves++] = trackOriginZ;
    inputLeaves[nInputLeaves++] = trackNHitsFit;
    inputLeaves[nInputLeaves++] = trackNHitsMax;
    inputLeaves[nInputLeaves++] = trackIdTruth;
    inputLeaves[nInputLeaves++] = trackQATruth;
    inputLeaves[nInputLeaves++] = mcId;
    inputLeaves[nInputLeaves++] = mcGePid;
    inputLeaves[nInputLeaves++] = mcPx;
    inputLeaves[nInputLeaves++] = mcPy;
    inputLeaves[nInputLeaves++] = mcPz;
    inputLeaves[nInputLeaves++] = mcStartVtx;
    configureInputBranchesV15(tree, s.label.Data(),
                              inputLeaves, nInputLeaves);

    s.hasMcStartVertex = mcStartVtx != 0;
    s.appliesDcaCut = eventVx && eventVy && eventVz &&
                      trackOriginX && trackOriginY && trackOriginZ;
    s.appliesHitsRatioCut = trackNHitsMax != 0;
    s.nEvents = tree->GetEntries();

    printf("V15READ sample=%s events=%lld primary_leaf=%d dca_cut=%d hits_ratio_cut=%d\n",
           s.label.Data(), s.nEvents,
           (Int_t)s.hasMcStartVertex,
           (Int_t)s.appliesDcaCut,
           (Int_t)s.appliesHitsRatioCut);

    for (Long64_t iEvent = 0; iEvent < s.nEvents; ++iEvent) {
        tree->GetEntry(iEvent);
        if (iEvent % 100 == 0) {
            printf("  V15 %s event %lld / %lld\n",
                   s.label.Data(), iEvent, s.nEvents);
        }

        std::map<Int_t, Int_t> mcIndex;
        std::set<Int_t> basicMatchedIds;
        std::set<Int_t> qualityMatchedIds;

        const Int_t nMc = mcPx->GetNdata();
        for (Int_t iMc = 0; iMc < nMc; ++iMc) {
            const Int_t id = mcId ?
                (Int_t)(valueV9(mcId, iMc, iMc + 1) + 0.5) : iMc + 1;
            mcIndex[id] = iMc;
        }

        const Double_t vertexX = eventVx ? valueV9(eventVx, 0) : 0.0;
        const Double_t vertexY = eventVy ? valueV9(eventVy, 0) : 0.0;
        const Double_t vertexZ = eventVz ? valueV9(eventVz, 0) : 0.0;

        const Int_t nTracks = trackPx->GetNdata();
        for (Int_t iTrack = 0; iTrack < nTracks; ++iTrack) {
            const Int_t idTruth =
                (Int_t)(valueV9(trackIdTruth, iTrack) + 0.5);
            const Double_t qaTruth = trackQATruth ?
                valueV9(trackQATruth, iTrack) : 100.0;
            if (idTruth <= 0 || qaTruth < kMinQaTruthV11) continue;
            if (mcIndex.find(idTruth) == mcIndex.end()) continue;

            basicMatchedIds.insert(idTruth);

            const Double_t px = valueV9(trackPx, iTrack);
            const Double_t py = valueV9(trackPy, iTrack);
            const Double_t pz = valueV9(trackPz, iTrack);
            const Double_t pt = sqrt(px * px + py * py);
            const Double_t eta = etaV9(px, py, pz);
            const Double_t signedNHitsFit =
                valueV9(trackNHitsFit, iTrack);
            const Double_t nHitsFit = fabs(signedNHitsFit);

            if (pt < kMinRecoPtV11 || !finiteV9(eta) ||
                fabs(eta) >= kMaxRecoEtaV11) continue;
            if (nHitsFit < kMinNHitsFitV11) continue;

            if (s.appliesHitsRatioCut) {
                const Double_t nHitsMax =
                    valueV9(trackNHitsMax, iTrack);
                if (nHitsMax <= 0.0 ||
                    nHitsFit / nHitsMax < kMinNHitsRatioV11) continue;
            }

            if (s.appliesDcaCut) {
                const Double_t dx =
                    valueV9(trackOriginX, iTrack) - vertexX;
                const Double_t dy =
                    valueV9(trackOriginY, iTrack) - vertexY;
                const Double_t dz =
                    valueV9(trackOriginZ, iTrack) - vertexZ;
                const Double_t dca = sqrt(dx * dx + dy * dy + dz * dz);
                if (dca >= kMaxDcaV11) continue;
            }

            qualityMatchedIds.insert(idTruth);
        }

        for (Int_t iMc = 0; iMc < nMc; ++iMc) {
            const Int_t gePid =
                (Int_t)(valueV9(mcGePid, iMc) + 0.5);
            const Int_t species = efficiencySpeciesV15(gePid);
            if (species < 0) continue;

            const Int_t startVtx = mcStartVtx ?
                (Int_t)(valueV9(mcStartVtx, iMc) + 0.5) : 1;
            if (startVtx != 1) continue;

            const Double_t px = valueV9(mcPx, iMc);
            const Double_t py = valueV9(mcPy, iMc);
            const Double_t pz = valueV9(mcPz, iMc);
            const Double_t pt = sqrt(px * px + py * py);
            const Double_t eta = etaV9(px, py, pz);
            if (!finiteV9(pt) || pt < kMinMcPtV15 ||
                !finiteV9(eta) || fabs(eta) >= kMaxMcEtaV15) continue;

            const Int_t id = mcId ?
                (Int_t)(valueV9(mcId, iMc, iMc + 1) + 0.5) : iMc + 1;

            ++s.nDenominator[species];
            s.hDenominator[species]->Fill(pt);
            if (basicMatchedIds.find(id) != basicMatchedIds.end()) {
                ++s.nBasicMatched[species];
                s.hBasicNumerator[species]->Fill(pt);
            }
            if (qualityMatchedIds.find(id) != qualityMatchedIds.end()) {
                ++s.nQualityMatched[species];
                s.hQualityNumerator[species]->Fill(pt);
            }
        }
    }

    for (Int_t species = 0; species < kNSpeciesV15; ++species) {
        TString key = "invalid";
        if (species == kPionV15) key = "pion";
        else if (species == kKaonV15) key = "kaon";
        else if (species == kProtonV15) key = "proton";
        else if (species == kAntiProtonV15) key = "antiproton";
        const Double_t basicEfficiency = s.nDenominator[species] > 0 ?
            s.nBasicMatched[species] /
                (Double_t)s.nDenominator[species] : 0.0;
        const Double_t qualityEfficiency = s.nDenominator[species] > 0 ?
            s.nQualityMatched[species] /
                (Double_t)s.nDenominator[species] : 0.0;
        printf("V15COUNT sample=%-10s species=%-10s denominator=%lld basic=%lld basic_eff=%g quality=%lld quality_eff=%g\n",
               s.label.Data(), key.Data(), s.nDenominator[species],
               s.nBasicMatched[species], basicEfficiency,
               s.nQualityMatched[species], qualityEfficiency);
    }

    return kTRUE;
}

void deriveEfficiencySampleV15(EfficiencySampleV15 &s)
{
    using namespace PicoQaV9;
    for (Int_t species = 0; species < kNSpeciesV15; ++species) {
        efficiencyV9(s.hBasicEfficiency[species],
                     s.hBasicNumerator[species],
                     s.hDenominator[species]);
        efficiencyV9(s.hQualityEfficiency[species],
                     s.hQualityNumerator[species],
                     s.hDenominator[species]);
    }
}

void writeEfficiencyHistogramsV15(EfficiencySampleV15 &detroit,
                                  EfficiencySampleV15 &reference,
                                  TFile *output)
{
    if (!output) return;
    output->cd();
    for (Int_t species = 0; species < kNSpeciesV15; ++species) {
        TString key = "invalid";
        if (species == kPionV15) key = "pion";
        else if (species == kKaonV15) key = "kaon";
        else if (species == kProtonV15) key = "proton";
        else if (species == kAntiProtonV15) key = "antiproton";

        detroit.hDenominator[species]->Write(
            Form("detroit_%s_denominator", key.Data()));
        detroit.hBasicNumerator[species]->Write(
            Form("detroit_%s_basic_numerator", key.Data()));
        detroit.hQualityNumerator[species]->Write(
            Form("detroit_%s_quality_numerator", key.Data()));
        detroit.hBasicEfficiency[species]->Write(
            Form("detroit_%s_basic_efficiency", key.Data()));
        detroit.hQualityEfficiency[species]->Write(
            Form("detroit_%s_quality_efficiency", key.Data()));

        reference.hDenominator[species]->Write(
            Form("reference_%s_denominator", key.Data()));
        reference.hBasicNumerator[species]->Write(
            Form("reference_%s_basic_numerator", key.Data()));
        reference.hQualityNumerator[species]->Write(
            Form("reference_%s_quality_numerator", key.Data()));
        reference.hBasicEfficiency[species]->Write(
            Form("reference_%s_basic_efficiency", key.Data()));
        reference.hQualityEfficiency[species]->Write(
            Form("reference_%s_quality_efficiency", key.Data()));
    }
}

void styleEfficiencyV15(TH1D *histogram,
                        const char *title,
                        Color_t color,
                        Style_t marker)
{
    histogram->SetTitle(title);
    histogram->GetXaxis()->SetTitle("p_{T}^{MC} (GeV/c)");
    histogram->GetYaxis()->SetTitle("reconstruction efficiency");
    histogram->SetMinimum(0.0);
    histogram->SetMaximum(1.2);
    histogram->SetLineColor(color);
    histogram->SetMarkerColor(color);
    histogram->SetMarkerStyle(marker);
    histogram->SetMarkerSize(0.75);
}

void drawEfficiencyPageV15(EfficiencySampleV15 &detroit,
                           EfficiencySampleV15 &reference,
                           Bool_t quality,
                           const char *pdf,
                           Int_t page,
                           Int_t totalPages)
{
    TString pageKind = quality ?
        "Analysis-quality truth match" :
        "Basic truth match (qaTruth #geq 50)";
    TCanvas *canvas = new TCanvas(
        quality ? "canvas_v15_quality_efficiency" :
                  "canvas_v15_basic_efficiency",
        pageKind.Data(), 1800, 1050);
    canvas->Divide(2, 2);

    for (Int_t species = 0; species < kNSpeciesV15; ++species) {
        TString key = "invalid";
        TString speciesTitle = "INVALID SPECIES";
        if (species == kPionV15) {
            key = "pion";
            speciesTitle = "#pi^{#pm} (GEANT ID 8 or 9)";
        }
        else if (species == kKaonV15) {
            key = "kaon";
            speciesTitle = "K^{#pm} (GEANT ID 11 or 12)";
        }
        else if (species == kProtonV15) {
            key = "proton";
            speciesTitle = "p (GEANT ID 14)";
        }
        else if (species == kAntiProtonV15) {
            key = "antiproton";
            speciesTitle = "#bar{p} (GEANT ID 15)";
        }
        TString panelTitle = pageKind;
        panelTitle += ": ";
        panelTitle += speciesTitle;

        TPad *pad = (TPad*)canvas->cd(species + 1);
        pad->SetLeftMargin(0.12);
        pad->SetRightMargin(0.04);
        pad->SetBottomMargin(0.13);
        pad->SetTopMargin(0.10);
        pad->SetGridy();

        TH1D *detroitHistogram = quality ?
            detroit.hQualityEfficiency[species] :
            detroit.hBasicEfficiency[species];
        TH1D *referenceHistogram = quality ?
            reference.hQualityEfficiency[species] :
            reference.hBasicEfficiency[species];

        styleEfficiencyV15(detroitHistogram, panelTitle.Data(),
                           kRed + 1, 20);
        styleEfficiencyV15(referenceHistogram, panelTitle.Data(),
                           kBlue + 1, 24);
        detroitHistogram->Draw("E1");
        referenceHistogram->Draw("E1 SAME");

        TLine *unity = new TLine(
            kMinPtAxisV15, 1.0, kMaxPtAxisV15, 1.0);
        unity->SetLineStyle(2);
        unity->SetLineColor(kGray + 2);
        unity->Draw();

        TLegend *legend = new TLegend(0.60, 0.18, 0.92, 0.34);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->AddEntry(detroitHistogram, "Detroit", "lp");
        legend->AddEntry(referenceHistogram, "Droy reference", "lp");
        legend->Draw();
    }

    canvas->cd();
    TLatex *pageLabel = new TLatex();
    pageLabel->SetNDC();
    pageLabel->SetTextAlign(22);
    pageLabel->SetTextFont(62);
    pageLabel->SetTextSize(0.025);
    pageLabel->DrawLatex(0.50, 0.995, pageKind.Data());

    PicoQaV11::printCanvasV11(canvas, pdf, page, totalPages);
    delete canvas;
}

} // namespace PicoQaV15

void comparePicoDstSamples_v15(
    const char *detroitFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_Detroit_vs_reference",
    Bool_t rerunDedxV14 = kFALSE)
{
    using namespace PicoQaV15;

    gStyle->SetOptStat(0);

    if (rerunDedxV14) {
        comparePicoDstSamples_v14(
            detroitFileName, referenceFileName,
            outputBase, kFALSE, kFALSE);
        gROOT->cd();
    }

    TFile *detroitFile = TFile::Open(detroitFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!detroitFile || detroitFile->IsZombie() ||
        !referenceFile || referenceFile->IsZombie()) {
        printf("V15ERROR cannot open one or both input files\n");
        return;
    }

    TTree *detroitTree = (TTree*)detroitFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!detroitTree || !referenceTree) {
        printf("V15ERROR PicoDst tree is missing in one or both files\n");
        detroitFile->Close();
        referenceFile->Close();
        return;
    }

    gROOT->cd();
    EfficiencySampleV15 detroit;
    EfficiencySampleV15 reference;
    initEfficiencySampleV15(detroit, "detroitv15", "Detroit");
    initEfficiencySampleV15(reference, "referencev15", "reference");

    if (!fillEfficiencySampleV15(detroitTree, detroit) ||
        !fillEfficiencySampleV15(referenceTree, reference)) {
        printf("V15ERROR species-efficiency comparison could not be filled\n");
        detroitFile->Close();
        referenceFile->Close();
        return;
    }

    deriveEfficiencySampleV15(detroit);
    deriveEfficiencySampleV15(reference);

    printf("V15CONFIG denominator=MC_vertex_1,pT_MC>=%.3f,absEta_MC<%.3f\n",
           kMinMcPtV15, kMaxMcEtaV15);
    printf("V15CONFIG basic_numerator=unique_idTruth_match,qaTruth>=%.0f\n",
           PicoQaV11::kMinQaTruthV11);
    printf("V15CONFIG quality_numerator=basic_plus_pT_reco>=%.3f,absEta_reco<%.3f,DCA<%.2f,nHitsFit>=%d,hitsRatio>=%.2f\n",
           PicoQaV11::kMinRecoPtV11,
           PicoQaV11::kMaxRecoEtaV11,
           PicoQaV11::kMaxDcaV11,
           PicoQaV11::kMinNHitsFitV11,
           PicoQaV11::kMinNHitsRatioV11);

    const TString rootName = Form(
        "%s_tracking_efficiency_species_v15.root", outputBase);
    const TString pdfName = Form(
        "%s_tracking_efficiency_species_v15.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V15ERROR cannot create output ROOT file %s\n",
               rootName.Data());
        detroitFile->Close();
        referenceFile->Close();
        return;
    }

    writeEfficiencyHistogramsV15(detroit, reference, output);
    drawEfficiencyPageV15(
        detroit, reference, kFALSE,
        pdfName.Data(), 1, 2);
    drawEfficiencyPageV15(
        detroit, reference, kTRUE,
        pdfName.Data(), 2, 2);

    output->Write();
    output->Close();
    detroitFile->Close();
    referenceFile->Close();

    printf("V15DONE efficiency_pdf=%s\n", pdfName.Data());
    printf("V15DONE efficiency_root=%s\n", rootName.Data());
}
