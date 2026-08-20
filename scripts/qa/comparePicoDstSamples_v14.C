#include "comparePicoDstSamples_v13.C"

#include "TLegend.h"
#include "TLine.h"

#include <cstdio>
#include <map>
#include <math.h>

// ============================================================================
// comparePicoDstSamples_v14.C
//
// Dedicated truth-separated TPC dE/dx comparison for the old Detroit-tune
// standalone sample and a reference PicoDst.  It adds dE/dx versus signed
// momentum p*q separately for true pions, kaons, and protons/antiprotons.
//
// Species are selected exclusively through Track.mIdTruth -> McTrack.mGePid:
//   pion        GEANT IDs 8, 9
//   kaon        GEANT IDs 11, 12
//   proton      GEANT IDs 14, 15
// No reconstructed nSigma cut is used for these plots, because such a cut
// could sculpt or hide the dE/dx displacement that this macro is testing.
//
// Track-quality selection is kept equal to the v11-v13 PID comparison:
//   idTruth > 0, qaTruth >= 50, pT >= 0.2 GeV/c, |eta| < 1,
//   |DCA| < 3 cm (when available), |nHitsFit| >= 15,
//   |nHitsFit|/nHitsMax >= 0.52 (when nHitsMax is available).
//
// Each species gets one four-panel PDF page:
//   1) Detroit dE/dx map; 2) reference dE/dx map;
//   3) mean dE/dx overlay;
//   4) Detroit-reference difference of the mean dE/dx.
// The fourth panel directly tests an additive constant displacement.
//
// ROOT 5.34/CINT compatible; intended for STAR SL22c.
// ============================================================================

namespace PicoQaV14 {

const Int_t kNSpeciesV14 = 3;
const Int_t kPionIndexV14 = 0;
const Int_t kKaonIndexV14 = 1;
const Int_t kProtonIndexV14 = 2;
const Double_t kMinimumDifferenceEntriesV14 = 5.0;

const char *speciesKeyV14(Int_t species)
{
    if (species == kPionIndexV14) return "pion";
    if (species == kKaonIndexV14) return "kaon";
    return "proton";
}

const char *speciesTitleV14(Int_t species)
{
    if (species == kPionIndexV14) return "true #pi^{#pm}";
    if (species == kKaonIndexV14) return "true K^{#pm}";
    return "true p and #bar{p}";
}

Int_t truthSpeciesV14(Int_t gePid)
{
    if (gePid == 8 || gePid == 9) return kPionIndexV14;
    if (gePid == 11 || gePid == 12) return kKaonIndexV14;
    if (gePid == 14 || gePid == 15) return kProtonIndexV14;
    return -1;
}

struct SpeciesDedxSampleV14 {
    TString prefix;
    TString label;
    Long64_t nEvents;
    Long64_t nSelected[kNSpeciesV14];
    TH2D *hCounts[kNSpeciesV14];
    TH2D *hShape[kNSpeciesV14];
    TH1D *hMean[kNSpeciesV14];
    TH1D *hEntriesVsSignedP[kNSpeciesV14];
    Bool_t appliesDcaCut;
    Bool_t appliesHitsRatioCut;
};

void initSpeciesDedxV14(SpeciesDedxSampleV14 &s,
                        const char *prefix,
                        const char *label)
{
    using namespace PicoQaV11;

    s.prefix = prefix;
    s.label = label;
    s.nEvents = 0;
    s.appliesDcaCut = kFALSE;
    s.appliesHitsRatioCut = kFALSE;

    for (Int_t iSpecies = 0;
        iSpecies < kNSpeciesV14; ++iSpecies) {
        s.nSelected[iSpecies] = 0;
        const char *key = speciesKeyV14(iSpecies);
        const TString countsKey =
            Form("dedx_vs_signed_p_%s_counts", key);
        const TString shapeKey =
            Form("dedx_vs_signed_p_%s_shape", key);
        const TString meanKey =
            Form("mean_dedx_vs_signed_p_%s", key);
        const TString entriesKey =
            Form("entries_vs_signed_p_%s", key);
        s.hCounts[iSpecies] = hist2V11(
            s.prefix, countsKey.Data(),
            120, -6.0, 6.0, 240, 0.0, 24.0);
        s.hShape[iSpecies] = hist2V11(
            s.prefix, shapeKey.Data(),
            120, -6.0, 6.0, 240, 0.0, 24.0);
        s.hMean[iSpecies] = hist1V11(
            s.prefix, meanKey.Data(),
            120, -6.0, 6.0);
        s.hEntriesVsSignedP[iSpecies] = hist1V11(
            s.prefix, entriesKey.Data(),
            120, -6.0, 6.0);
    }
}

void reportLeafV14(const char *sample,
                   const char *logicalName,
                   TLeaf *leaf)
{
    printf("V14LEAF sample=%-10s %-26s : %s\n",
           sample, logicalName, leaf ? leaf->GetName() : "MISSING");
}

Bool_t fillSpeciesDedxV14(TTree *tree, SpeciesDedxSampleV14 &s)
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
    TLeaf *trackDedx = leafV9(tree, "Track.mDedx", "Track.mDEdx");
    TLeaf *trackIdTruth = leafV9(tree, "Track.mIdTruth");
    TLeaf *trackQATruth = leafV9(tree, "Track.mQATruth");

    TLeaf *mcId = leafV9(tree, "McTrack.mId");
    TLeaf *mcGePid = leafV9(tree, "McTrack.mGePid", "McTrack.mGeantId");

    reportLeafV14(s.label.Data(), "Track momentum", trackPx);
    reportLeafV14(s.label.Data(), "Track dE/dx", trackDedx);
    reportLeafV14(s.label.Data(), "Track signed nHitsFit", trackNHitsFit);
    reportLeafV14(s.label.Data(), "Track nHitsMax", trackNHitsMax);
    reportLeafV14(s.label.Data(), "Track origin", trackOriginX);
    reportLeafV14(s.label.Data(), "Event primary vertex", eventVx);
    reportLeafV14(s.label.Data(), "Track idTruth", trackIdTruth);
    reportLeafV14(s.label.Data(), "Track qaTruth", trackQATruth);
    reportLeafV14(s.label.Data(), "McTrack ID", mcId);
    reportLeafV14(s.label.Data(), "McTrack GEANT PID", mcGePid);

    const Bool_t hasEssentialLeaves =
        trackPx && trackPy && trackPz && trackNHitsFit &&
        trackDedx && trackIdTruth && mcGePid;
    if (!hasEssentialLeaves) {
        printf("V14ERROR sample=%s reason=missing_essential_leaf\n",
               s.label.Data());
        return kFALSE;
    }

    s.appliesDcaCut = eventVx && eventVy && eventVz &&
                      trackOriginX && trackOriginY && trackOriginZ;
    s.appliesHitsRatioCut = trackNHitsMax != 0;
    s.nEvents = tree->GetEntries();

    printf("V14READ sample=%s events=%lld dca_cut=%d hits_ratio_cut=%d\n",
           s.label.Data(), s.nEvents,
           (Int_t)s.appliesDcaCut,
           (Int_t)s.appliesHitsRatioCut);

    for (Long64_t iEvent = 0; iEvent < s.nEvents; ++iEvent) {
        tree->GetEntry(iEvent);
        if (iEvent % 100 == 0) {
            printf("  V14 %s event %lld / %lld\n",
                   s.label.Data(), iEvent, s.nEvents);
        }

        std::map<Int_t, Int_t> mcIndex;
        const Int_t nMc = mcGePid->GetNdata();
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

            std::map<Int_t, Int_t>::const_iterator found =
                mcIndex.find(idTruth);
            if (found == mcIndex.end()) continue;

            const Double_t px = valueV9(trackPx, iTrack);
            const Double_t py = valueV9(trackPy, iTrack);
            const Double_t pz = valueV9(trackPz, iTrack);
            const Double_t pt = sqrt(px * px + py * py);
            const Double_t p = sqrt(pt * pt + pz * pz);
            const Double_t eta = etaV9(px, py, pz);
            const Double_t signedNHitsFit =
                valueV9(trackNHitsFit, iTrack);
            const Double_t nHitsFit = fabs(signedNHitsFit);
            const Int_t charge = signedNHitsFit > 0.0 ? 1 : -1;

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

            const Double_t dedx =
                valueV9(trackDedx, iTrack, -999.0);
            if (!finiteV9(p) || !finiteV9(dedx) || dedx <= 0.0) continue;

            const Int_t iMc = found->second;
            const Int_t gePid =
                (Int_t)(valueV9(mcGePid, iMc) + 0.5);
            const Int_t species = truthSpeciesV14(gePid);
            if (species < 0) continue;

            const Double_t signedP = p * (Double_t)charge;
            ++s.nSelected[species];
            s.hCounts[species]->Fill(signedP, dedx);
            s.hShape[species]->Fill(signedP, dedx);
            s.hEntriesVsSignedP[species]->Fill(signedP);
        }
    }

    for (Int_t iSpecies = 0;
         iSpecies < kNSpeciesV14; ++iSpecies) {
        printf("V14COUNT sample=%-10s species=%-8s tracks=%lld\n",
               s.label.Data(), speciesKeyV14(iSpecies),
               s.nSelected[iSpecies]);
    }
    return kTRUE;
}

void deriveMeanV14(TH2D *counts, TH1D *mean)
{
    if (!counts || !mean) return;

    for (Int_t iX = 1; iX <= counts->GetNbinsX(); ++iX) {
        Double_t sum = 0.0;
        Double_t sum2 = 0.0;
        Double_t entries = 0.0;
        for (Int_t iY = 1; iY <= counts->GetNbinsY(); ++iY) {
            const Double_t weight = counts->GetBinContent(iX, iY);
            const Double_t dedx = counts->GetYaxis()->GetBinCenter(iY);
            sum += weight * dedx;
            sum2 += weight * dedx * dedx;
            entries += weight;
        }
        if (entries > 0.0) {
            const Double_t average = sum / entries;
            Double_t variance = sum2 / entries - average * average;
            if (variance < 0.0) variance = 0.0;
            mean->SetBinContent(iX, average);
            mean->SetBinError(iX, sqrt(variance / entries));
        }
    }
}

void deriveSpeciesDedxV14(SpeciesDedxSampleV14 &s)
{
    using namespace PicoQaV11;
    for (Int_t iSpecies = 0;
         iSpecies < kNSpeciesV14; ++iSpecies) {
        deriveMeanV14(s.hCounts[iSpecies], s.hMean[iSpecies]);
        unitArea2DV11(s.hShape[iSpecies]);
    }
}

TH1D *makeDifferenceV14(const SpeciesDedxSampleV14 &a,
                        const SpeciesDedxSampleV14 &b,
                        Int_t species,
                        Double_t &constant,
                        Double_t &constantError,
                        Int_t &nBinsUsed,
                        Double_t &chi2,
                        Int_t &ndf)
{
    TH1D *difference = (TH1D*)a.hMean[species]->Clone(
        Form("difference_mean_dedx_%s", speciesKeyV14(species)));
    difference->Reset();
    difference->SetDirectory(0);

    Double_t weightedSum = 0.0;
    Double_t weightSum = 0.0;
    nBinsUsed = 0;

    for (Int_t iBin = 1;
         iBin <= difference->GetNbinsX(); ++iBin) {
        const Double_t entriesA =
            a.hEntriesVsSignedP[species]->GetBinContent(iBin);
        const Double_t entriesB =
            b.hEntriesVsSignedP[species]->GetBinContent(iBin);
        if (entriesA < kMinimumDifferenceEntriesV14 ||
            entriesB < kMinimumDifferenceEntriesV14) continue;

        const Double_t value =
            a.hMean[species]->GetBinContent(iBin) -
            b.hMean[species]->GetBinContent(iBin);
        const Double_t errorA = a.hMean[species]->GetBinError(iBin);
        const Double_t errorB = b.hMean[species]->GetBinError(iBin);
        const Double_t error = sqrt(errorA * errorA + errorB * errorB);
        difference->SetBinContent(iBin, value);
        difference->SetBinError(iBin, error);

        if (error > 0.0) {
            const Double_t weight = 1.0 / (error * error);
            weightedSum += weight * value;
            weightSum += weight;
            ++nBinsUsed;
        }
    }

    constant = weightSum > 0.0 ? weightedSum / weightSum : 0.0;
    constantError = weightSum > 0.0 ? sqrt(1.0 / weightSum) : 0.0;
    chi2 = 0.0;
    ndf = nBinsUsed > 0 ? nBinsUsed - 1 : 0;
    if (nBinsUsed > 0) {
        for (Int_t iBin = 1;
             iBin <= difference->GetNbinsX(); ++iBin) {
            const Double_t error = difference->GetBinError(iBin);
            if (error <= 0.0) continue;
            const Double_t pull =
                (difference->GetBinContent(iBin) - constant) / error;
            chi2 += pull * pull;
        }
    }
    return difference;
}

void styleMapV14(TH2D *histogram,
                 const char *title,
                 Double_t minimum,
                 Double_t maximum)
{
    histogram->SetTitle(title);
    histogram->GetXaxis()->SetTitle("p #times q (GeV/c)");
    histogram->GetYaxis()->SetTitle("dE/dx (keV/cm)");
    histogram->GetZaxis()->SetTitle("fraction");
    histogram->SetMinimum(minimum);
    histogram->SetMaximum(maximum > 0.0 ? maximum : 1.0);
}

void drawSpeciesPageV14(SpeciesDedxSampleV14 &a,
                        SpeciesDedxSampleV14 &b,
                        Int_t species,
                        TFile *output,
                        const char *pdf,
                        Int_t page,
                        Int_t totalPages)
{
    const char *key = speciesKeyV14(species);
    const char *title = speciesTitleV14(species);

    output->cd();
    a.hCounts[species]->Write(Form("detroit_%s_dedx_counts", key));
    b.hCounts[species]->Write(Form("reference_%s_dedx_counts", key));
    a.hShape[species]->Write(Form("detroit_%s_dedx_shape", key));
    b.hShape[species]->Write(Form("reference_%s_dedx_shape", key));
    a.hMean[species]->Write(Form("detroit_%s_mean_dedx", key));
    b.hMean[species]->Write(Form("reference_%s_mean_dedx", key));
    a.hEntriesVsSignedP[species]->Write(
        Form("detroit_%s_entries_vs_signed_p", key));
    b.hEntriesVsSignedP[species]->Write(
        Form("reference_%s_entries_vs_signed_p", key));

    Double_t constant = 0.0;
    Double_t constantError = 0.0;
    Int_t nBinsUsed = 0;
    Double_t chi2 = 0.0;
    Int_t ndf = 0;
    TH1D *difference = makeDifferenceV14(
        a, b, species, constant, constantError, nBinsUsed, chi2, ndf);
    difference->Write(Form("detroit_minus_reference_%s_mean_dedx", key));

    printf("V14SHIFT species=%-8s weighted_constant=%g error=%g bins=%d chi2=%g ndf=%d chi2_ndf=%g min_entries_per_sample=%.0f\n",
           key, constant, constantError, nBinsUsed, chi2, ndf,
           ndf > 0 ? chi2 / (Double_t)ndf : 0.0,
           kMinimumDifferenceEntriesV14);

    const Double_t maximumA = a.hShape[species]->GetMaximum();
    const Double_t maximumB = b.hShape[species]->GetMaximum();
    const Double_t maximum = maximumA > maximumB ? maximumA : maximumB;
    const Double_t minimum =
        maximum > 0.0 ? maximum * 1.0e-5 : 1.0e-12;

    TCanvas *canvas = new TCanvas(
        Form("canvas_v14_%s", key),
        Form("Truth-separated dE/dx: %s", title), 1600, 1250);
    canvas->Divide(2, 2);

    TPad *pad1 = (TPad*)canvas->cd(1);
    pad1->SetLogz();
    pad1->SetLeftMargin(0.11);
    pad1->SetRightMargin(0.16);
    pad1->SetBottomMargin(0.12);
    styleMapV14(a.hShape[species],
        Form("Detroit: %s", title), minimum, maximum);
    a.hShape[species]->Draw("COLZ");

    TPad *pad2 = (TPad*)canvas->cd(2);
    pad2->SetLogz();
    pad2->SetLeftMargin(0.11);
    pad2->SetRightMargin(0.16);
    pad2->SetBottomMargin(0.12);
    styleMapV14(b.hShape[species],
        Form("Reference: %s", title), minimum, maximum);
    b.hShape[species]->Draw("COLZ");

    TPad *pad3 = (TPad*)canvas->cd(3);
    pad3->SetLeftMargin(0.12);
    pad3->SetRightMargin(0.04);
    pad3->SetBottomMargin(0.13);
    a.hMean[species]->SetTitle(Form("Mean dE/dx: %s", title));
    a.hMean[species]->GetXaxis()->SetTitle("p #times q (GeV/c)");
    a.hMean[species]->GetYaxis()->SetTitle("#LTdE/dx#GT (keV/cm)");
    a.hMean[species]->SetLineColor(kRed + 1);
    a.hMean[species]->SetMarkerColor(kRed + 1);
    a.hMean[species]->SetMarkerStyle(20);
    b.hMean[species]->SetLineColor(kBlue + 1);
    b.hMean[species]->SetMarkerColor(kBlue + 1);
    b.hMean[species]->SetMarkerStyle(24);
    const Double_t meanMaximumA = a.hMean[species]->GetMaximum();
    const Double_t meanMaximumB = b.hMean[species]->GetMaximum();
    const Double_t meanMaximum =
        meanMaximumA > meanMaximumB ? meanMaximumA : meanMaximumB;
    a.hMean[species]->SetMinimum(0.0);
    a.hMean[species]->SetMaximum(
        meanMaximum > 0.0 ? 1.20 * meanMaximum : 1.0);
    a.hMean[species]->Draw("E1");
    b.hMean[species]->Draw("E1 SAME");
    TLegend *legend = new TLegend(0.66, 0.75, 0.92, 0.90);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    legend->AddEntry(a.hMean[species], "Detroit", "lp");
    legend->AddEntry(b.hMean[species], "reference", "lp");
    legend->Draw();

    TPad *pad4 = (TPad*)canvas->cd(4);
    pad4->SetLeftMargin(0.13);
    pad4->SetRightMargin(0.04);
    pad4->SetBottomMargin(0.13);
    difference->SetTitle(Form("Detroit - reference: %s", title));
    difference->GetXaxis()->SetTitle("p #times q (GeV/c)");
    difference->GetYaxis()->SetTitle(
        "#Delta#LTdE/dx#GT (keV/cm)");
    difference->SetLineColor(kBlack);
    difference->SetMarkerColor(kBlack);
    difference->SetMarkerStyle(20);

    Double_t differenceMaximum = 0.0;
    for (Int_t iBin = 1;
         iBin <= difference->GetNbinsX(); ++iBin) {
        const Double_t candidate =
            fabs(difference->GetBinContent(iBin)) +
            difference->GetBinError(iBin);
        if (candidate > differenceMaximum) differenceMaximum = candidate;
    }
    if (differenceMaximum <= 0.0) differenceMaximum = 1.0;
    difference->SetMinimum(-1.25 * differenceMaximum);
    difference->SetMaximum(1.25 * differenceMaximum);
    difference->Draw("E1");

    TLine *zeroLine = new TLine(-6.0, 0.0, 6.0, 0.0);
    zeroLine->SetLineStyle(2);
    zeroLine->Draw();
    if (nBinsUsed > 0) {
        TLine *constantLine =
            new TLine(-6.0, constant, 6.0, constant);
        constantLine->SetLineColor(kRed + 1);
        constantLine->SetLineStyle(7);
        constantLine->SetLineWidth(2);
        constantLine->Draw();

        TLatex *latex = new TLatex();
        latex->SetNDC();
        latex->SetTextSize(0.035);
        latex->DrawLatex(0.17, 0.88,
            Form("weighted constant = %.3f #pm %.3f keV/cm",
                 constant, constantError));
        if (ndf > 0) {
            latex->DrawLatex(0.17, 0.83,
                Form("#chi^{2}/ndf = %.1f/%d = %.2f",
                     chi2, ndf, chi2 / (Double_t)ndf));
        }
    }

    PicoQaV11::printCanvasV11(canvas, pdf, page, totalPages);

    delete canvas;
    delete difference;
}

} // namespace PicoQaV14

void comparePicoDstSamples_v14(
    const char *detroitFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_Detroit_vs_reference",
    Bool_t rerunFullV13 = kFALSE,
    Bool_t runResponseV10 = kFALSE)
{
    using namespace PicoQaV14;

    gStyle->SetOptStat(0);

    // The old comparison can be regenerated from the same entry point, but
    // stays disabled by default because the present diagnostic only needs the
    // three truth-separated dE/dx pages.
    if (rerunFullV13) {
        comparePicoDstSamples_v13(
            detroitFileName, referenceFileName,
            outputBase, runResponseV10);
        gROOT->cd();
    }

    TFile *detroitFile = TFile::Open(detroitFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!detroitFile || detroitFile->IsZombie() ||
        !referenceFile || referenceFile->IsZombie()) {
        printf("V14ERROR cannot open one or both input files\n");
        return;
    }

    TTree *detroitTree = (TTree*)detroitFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!detroitTree || !referenceTree) {
        printf("V14ERROR PicoDst tree is missing in one or both files\n");
        detroitFile->Close();
        referenceFile->Close();
        return;
    }

    gROOT->cd();
    SpeciesDedxSampleV14 detroit;
    SpeciesDedxSampleV14 reference;
    initSpeciesDedxV14(detroit, "detroitv14", "Detroit");
    initSpeciesDedxV14(reference, "referencev14", "reference");

    if (!fillSpeciesDedxV14(detroitTree, detroit) ||
        !fillSpeciesDedxV14(referenceTree, reference)) {
        printf("V14ERROR truth-separated dE/dx comparison could not be filled\n");
        detroitFile->Close();
        referenceFile->Close();
        return;
    }

    deriveSpeciesDedxV14(detroit);
    deriveSpeciesDedxV14(reference);

    const TString rootName =
        Form("%s_dedx_species_v14.root", outputBase);
    const TString pdfName =
        Form("%s_dedx_species_v14.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V14ERROR cannot create output ROOT file %s\n",
               rootName.Data());
        detroitFile->Close();
        referenceFile->Close();
        return;
    }

    for (Int_t iSpecies = 0;
         iSpecies < kNSpeciesV14; ++iSpecies) {
        drawSpeciesPageV14(detroit, reference, iSpecies,
            output, pdfName.Data(), iSpecies + 1, kNSpeciesV14);
    }

    output->Write();
    output->Close();
    detroitFile->Close();
    referenceFile->Close();

    printf("V14DONE dedx_species_pdf=%s\n", pdfName.Data());
    printf("V14DONE dedx_species_root=%s\n", rootName.Data());
}
