#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TBranch.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMath.h"
#include "TString.h"
#include "TStyle.h"
#include "TROOT.h"

#include <cstdio>

// ============================================================================
// comparePicoDstSamples_v17.C
//
// Fast, MC-only subset of the v10 response QA.  It deliberately avoids the
// full v9/v10 event loop and reads only:
//   McTrack.mPx, McTrack.mPy, McTrack.mGePid (or McTrack.mGeantId).
//
// The four pages are identical in definition and binning to v10:
//   1) MC tracks per event (per-event normalization),
//   2) MC pT shape (unit area),
//   3) MC GEANT-PID fractions (unit area),
//   4) D0/anti-D0 pT shape, GEANT IDs 37/38 (unit area).
//
// Outputs for OUT:
//   OUT_mc_summary_v17.pdf
//   OUT_mc_summary_v17.root
//
// ROOT 5.34/CINT compatible.
// ============================================================================

namespace PicoQaV17 {

struct McSummarySampleV17 {
    TString prefix;
    TString label;
    Long64_t nEvents;
    Long64_t nMcTracks;
    Long64_t nD0;

    TH1D *hNMcTracks;
    TH1D *hMcPtShape;
    TH1D *hMcGePidFraction;
    TH1D *hD0PtShape;
};

TH1D *histV17(const TString &prefix,
              const char *key,
              Int_t nBins,
              Double_t minimum,
              Double_t maximum)
{
    TH1D *histogram = new TH1D(
        Form("%s_%s", prefix.Data(), key), "",
        nBins, minimum, maximum);
    histogram->SetDirectory(0);
    histogram->Sumw2();
    return histogram;
}

void initSampleV17(McSummarySampleV17 &sample,
                   const char *prefix,
                   const char *label)
{
    sample.prefix = prefix;
    sample.label = label;
    sample.nEvents = 0;
    sample.nMcTracks = 0;
    sample.nD0 = 0;

    // Keep exactly the v9/v10 binning.
    sample.hNMcTracks = histV17(
        sample.prefix, "n_mc_tracks_exact",
        1001, -0.5, 1000.5);
    sample.hMcPtShape = histV17(
        sample.prefix, "mc_pt_shape",
        100, 0.0, 10.0);
    sample.hMcGePidFraction = histV17(
        sample.prefix, "mc_gepid_fraction",
        60, -0.5, 59.5);
    sample.hD0PtShape = histV17(
        sample.prefix, "d0_pt_shape",
        100, 0.0, 25.0);
}

TLeaf *leafV17(TTree *tree,
               const char *name1,
               const char *name2 = 0)
{
    if (!tree) return 0;
    TLeaf *leaf = name1 ? tree->GetLeaf(name1) : 0;
    if (!leaf && name2) leaf = tree->GetLeaf(name2);
    return leaf;
}

Bool_t finiteV17(Double_t value)
{
    return value == value && TMath::Abs(value) < 1.0e30;
}

Double_t valueV17(TLeaf *leaf,
                  Int_t index,
                  Double_t fallback = 0.0)
{
    if (!leaf || index < 0 || index >= leaf->GetNdata()) return fallback;
    const Double_t value = leaf->GetValue(index);
    return finiteV17(value) ? value : fallback;
}

void activateBranchV17(TTree *tree, TLeaf *leaf)
{
    if (!tree || !leaf) return;

    TBranch *branch = leaf->GetBranch();
    if (branch) {
        tree->SetBranchStatus(branch->GetName(), 1);
        tree->AddBranchToCache(branch->GetName(), kFALSE);

        // Enable the collection-size branch (normally "McTrack") as well.
        TString branchName = branch->GetName();
        const Ssiz_t dot = branchName.Index(".");
        if (dot > 0) {
            TString counterName = branchName(0, dot);
            tree->SetBranchStatus(counterName.Data(), 1);
            tree->AddBranchToCache(counterName.Data(), kFALSE);
        }
    }

    TLeaf *countLeaf = leaf->GetLeafCount();
    if (countLeaf && countLeaf->GetBranch()) {
        tree->SetBranchStatus(countLeaf->GetBranch()->GetName(), 1);
        tree->AddBranchToCache(
            countLeaf->GetBranch()->GetName(), kFALSE);
    }
}

Bool_t fillSampleV17(TTree *tree, McSummarySampleV17 &sample)
{
    if (!tree) return kFALSE;

    TLeaf *mcPx = leafV17(tree, "McTrack.mPx");
    TLeaf *mcPy = leafV17(tree, "McTrack.mPy");
    TLeaf *mcGePid = leafV17(
        tree, "McTrack.mGePid", "McTrack.mGeantId");

    printf("V17LEAF sample=%-10s McTrack.mPx    : %s\n",
           sample.label.Data(), mcPx ? mcPx->GetName() : "MISSING");
    printf("V17LEAF sample=%-10s McTrack.mPy    : %s\n",
           sample.label.Data(), mcPy ? mcPy->GetName() : "MISSING");
    printf("V17LEAF sample=%-10s McTrack GEANT  : %s\n",
           sample.label.Data(), mcGePid ? mcGePid->GetName() : "MISSING");

    if (!mcPx || !mcPy || !mcGePid) {
        printf("V17ERROR sample=%s reason=missing_essential_mc_leaf\n",
               sample.label.Data());
        fflush(stdout);
        return kFALSE;
    }

    tree->SetBranchStatus("*", 0);
    tree->SetCacheSize(5 * 1024 * 1024);
    activateBranchV17(tree, mcPx);
    activateBranchV17(tree, mcPy);
    activateBranchV17(tree, mcGePid);

    sample.nEvents = tree->GetEntries();
    printf("V17IO sample=%s mode=mc_only active_leaves=3 cache_MB=5\n",
           sample.label.Data());
    printf("V17READ sample=%s events=%lld\n",
           sample.label.Data(), sample.nEvents);
    fflush(stdout);

    for (Long64_t iEvent = 0;
         iEvent < sample.nEvents; ++iEvent) {
        const Long64_t bytes = tree->GetEntry(iEvent);
        if (bytes < 0) {
            printf("V17ERROR sample=%s event=%lld reason=GetEntry_failed\n",
                   sample.label.Data(), iEvent);
            fflush(stdout);
            return kFALSE;
        }

        if (iEvent % 500 == 0) {
            printf("  V17 %s event %lld / %lld\n",
                   sample.label.Data(), iEvent, sample.nEvents);
            fflush(stdout);
        }

        const Int_t nMc = mcPx->GetNdata();
        sample.hNMcTracks->Fill(nMc);
        sample.nMcTracks += nMc;

        for (Int_t iMc = 0; iMc < nMc; ++iMc) {
            const Double_t px = valueV17(mcPx, iMc);
            const Double_t py = valueV17(mcPy, iMc);
            const Double_t pt = TMath::Sqrt(px * px + py * py);
            const Int_t gePid =
                (Int_t)(valueV17(mcGePid, iMc) + 0.5);

            if (finiteV17(pt)) sample.hMcPtShape->Fill(pt);
            sample.hMcGePidFraction->Fill(gePid);

            if (gePid == 37 || gePid == 38) {
                if (finiteV17(pt)) sample.hD0PtShape->Fill(pt);
                ++sample.nD0;
            }
        }
    }

    return kTRUE;
}

void scalePerEventV17(TH1D *histogram, Long64_t nEvents)
{
    if (histogram && nEvents > 0) {
        histogram->Scale(1.0 / (Double_t)nEvents);
    }
}

void unitAreaV17(TH1D *histogram)
{
    if (!histogram) return;
    const Double_t integral = histogram->Integral(
        0, histogram->GetNbinsX() + 1);
    if (integral > 0.0) histogram->Scale(1.0 / integral);
}

void deriveSampleV17(McSummarySampleV17 &sample)
{
    scalePerEventV17(sample.hNMcTracks, sample.nEvents);
    unitAreaV17(sample.hMcPtShape);
    unitAreaV17(sample.hMcGePidFraction);
    unitAreaV17(sample.hD0PtShape);
}

void printQaV17(const char *key, TH1D *newHistogram, TH1D *referenceHistogram)
{
    if (!newHistogram || !referenceHistogram) return;
    const Double_t newMean = newHistogram->GetMean();
    const Double_t referenceMean = referenceHistogram->GetMean();
    const Double_t meanRatio = referenceMean != 0.0 ?
        newMean / referenceMean : 0.0;
    const Double_t ks =
        newHistogram->Integral() > 0.0 &&
        referenceHistogram->Integral() > 0.0 ?
        newHistogram->KolmogorovTest(referenceHistogram) : -1.0;
    printf("V17QA %-24s mean_new=%11.6g mean_ref=%11.6g ratio=%9.5f KS=%10.4g\n",
           key, newMean, referenceMean, meanRatio, ks);
}

void printCanvasV17(TCanvas *canvas,
                    const char *pdf,
                    Bool_t first,
                    Bool_t last)
{
    TString printName = pdf;
    if (first) printName += "(";
    if (last) printName += ")";
    canvas->Print(printName.Data());
}

void drawPageV17(TH1D *newHistogram,
                 TH1D *referenceHistogram,
                 TFile *output,
                 const char *pdf,
                 const char *key,
                 const char *title,
                 const char *xTitle,
                 const char *yTitle,
                 Bool_t logY,
                 Double_t ratioMinimum,
                 Double_t ratioMaximum,
                 Int_t page,
                 Int_t totalPages)
{
    if (!newHistogram || !referenceHistogram || !output) return;

    printf("V17PDF page %d / %d: %s\n", page, totalPages, key);
    fflush(stdout);

    newHistogram->SetLineColor(kRed + 1);
    newHistogram->SetMarkerColor(kRed + 1);
    newHistogram->SetMarkerStyle(20);
    newHistogram->SetMarkerSize(0.55);
    referenceHistogram->SetLineColor(kBlack);
    referenceHistogram->SetMarkerColor(kBlack);
    referenceHistogram->SetMarkerStyle(24);
    referenceHistogram->SetMarkerSize(0.55);

    TH1D *ratio = (TH1D*)newHistogram->Clone(
        Form("ratio_v17_%s", key));
    ratio->SetDirectory(0);
    ratio->Divide(referenceHistogram);

    output->cd();
    newHistogram->Write(Form("new_%s", key));
    referenceHistogram->Write(Form("reference_%s", key));
    ratio->Write();

    TCanvas *canvas = new TCanvas(
        Form("canvas_v17_%s", key), title, 900, 800);
    TPad *upper = new TPad(
        Form("upper_v17_%s", key), "", 0.0, 0.30, 1.0, 1.0);
    TPad *lower = new TPad(
        Form("lower_v17_%s", key), "", 0.0, 0.0, 1.0, 0.30);
    upper->SetLeftMargin(0.13);
    upper->SetRightMargin(0.04);
    upper->SetBottomMargin(0.02);
    lower->SetLeftMargin(0.13);
    lower->SetRightMargin(0.04);
    lower->SetTopMargin(0.03);
    lower->SetBottomMargin(0.32);
    canvas->cd();
    upper->Draw();
    lower->Draw();

    upper->cd();
    if (logY) upper->SetLogy();
    referenceHistogram->SetTitle(title);
    referenceHistogram->GetXaxis()->SetLabelSize(0.0);
    referenceHistogram->GetYaxis()->SetTitle(yTitle);
    referenceHistogram->GetYaxis()->SetTitleOffset(1.35);
    const Double_t maximum = TMath::Max(
        newHistogram->GetMaximum(), referenceHistogram->GetMaximum());
    if (logY) {
        referenceHistogram->SetMinimum(
            maximum > 0.0 ? maximum * 1.0e-6 : 1.0e-10);
        referenceHistogram->SetMaximum(
            maximum > 0.0 ? maximum * 10.0 : 1.0);
    }
    else {
        referenceHistogram->SetMinimum(0.0);
        referenceHistogram->SetMaximum(
            maximum > 0.0 ? maximum * 1.30 : 1.0);
    }
    referenceHistogram->Draw("E1");
    newHistogram->Draw("E1 SAME");

    TLegend *legend = new TLegend(0.57, 0.76, 0.94, 0.90);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    legend->AddEntry(newHistogram, "new", "lep");
    legend->AddEntry(referenceHistogram, "reference", "lep");
    legend->Draw();

    lower->cd();
    ratio->SetTitle("");
    ratio->GetXaxis()->SetTitle(xTitle);
    ratio->GetXaxis()->SetTitleSize(0.12);
    ratio->GetXaxis()->SetTitleOffset(1.05);
    ratio->GetXaxis()->SetLabelSize(0.095);
    ratio->GetYaxis()->SetTitle("new / ref");
    ratio->GetYaxis()->SetTitleSize(0.10);
    ratio->GetYaxis()->SetTitleOffset(0.50);
    ratio->GetYaxis()->SetLabelSize(0.08);
    ratio->GetYaxis()->SetNdivisions(505);
    ratio->SetMinimum(ratioMinimum);
    ratio->SetMaximum(ratioMaximum);
    ratio->Draw("E1");
    TLine unity(
        ratio->GetXaxis()->GetXmin(), 1.0,
        ratio->GetXaxis()->GetXmax(), 1.0);
    unity.SetLineStyle(2);
    unity.Draw();

    printCanvasV17(
        canvas, pdf, page == 1, page == totalPages);
    printQaV17(key, newHistogram, referenceHistogram);
    delete canvas;
}

} // namespace PicoQaV17

void comparePicoDstSamples_v17(
    const char *newFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_v17")
{
    using namespace PicoQaV17;

    gStyle->SetOptStat(0);

    TFile *newFile = TFile::Open(newFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!newFile || newFile->IsZombie() ||
        !referenceFile || referenceFile->IsZombie()) {
        printf("V17ERROR cannot open one or both input files\n");
        fflush(stdout);
        return;
    }

    TTree *newTree = (TTree*)newFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!newTree || !referenceTree) {
        printf("V17ERROR PicoDst tree is missing in one or both files\n");
        fflush(stdout);
        newFile->Close();
        referenceFile->Close();
        return;
    }

    gROOT->cd();
    McSummarySampleV17 newSample;
    McSummarySampleV17 referenceSample;
    initSampleV17(newSample, "newv17", "new");
    initSampleV17(referenceSample, "referencev17", "reference");

    if (!fillSampleV17(newTree, newSample) ||
        !fillSampleV17(referenceTree, referenceSample)) {
        printf("V17ERROR MC summary could not be filled\n");
        fflush(stdout);
        newFile->Close();
        referenceFile->Close();
        return;
    }

    deriveSampleV17(newSample);
    deriveSampleV17(referenceSample);

    printf("V17CONFIG events new=%lld reference=%lld\n",
           newSample.nEvents, referenceSample.nEvents);
    printf("V17CONFIG mc_tracks new=%lld reference=%lld\n",
           newSample.nMcTracks, referenceSample.nMcTracks);
    printf("V17CONFIG d0_and_antid0 new=%lld reference=%lld\n",
           newSample.nD0, referenceSample.nD0);
    fflush(stdout);

    const TString rootName = Form(
        "%s_mc_summary_v17.root", outputBase);
    const TString pdfName = Form(
        "%s_mc_summary_v17.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V17ERROR cannot create output ROOT file %s\n",
               rootName.Data());
        fflush(stdout);
        newFile->Close();
        referenceFile->Close();
        return;
    }

    const Int_t totalPages = 4;
    drawPageV17(
        newSample.hNMcTracks, referenceSample.hNMcTracks,
        output, pdfName.Data(),
        "n_mc_tracks_exact", "MC tracks per event (exact)",
        "N_{MC}", "events^{-1}",
        kTRUE, 0.0, 2.0, 1, totalPages);
    drawPageV17(
        newSample.hMcPtShape, referenceSample.hMcPtShape,
        output, pdfName.Data(),
        "mc_pt_shape", "MC p_{T} shape (unit area)",
        "p_{T}^{MC} (GeV/c)", "fraction",
        kTRUE, 0.0, 2.5, 2, totalPages);
    drawPageV17(
        newSample.hMcGePidFraction,
        referenceSample.hMcGePidFraction,
        output, pdfName.Data(),
        "mc_gepid_fraction", "MC GEANT-PID fractions",
        "GEANT PID", "fraction",
        kTRUE, 0.0, 3.0, 3, totalPages);
    drawPageV17(
        newSample.hD0PtShape, referenceSample.hD0PtShape,
        output, pdfName.Data(),
        "d0_pt_shape", "D^{0}/#bar{D}^{0} p_{T} shape (unit area)",
        "p_{T}^{D^{0}} (GeV/c)", "fraction",
        kTRUE, 0.0, 3.0, 4, totalPages);

    output->Write();
    output->Close();
    newFile->Close();
    referenceFile->Close();

    printf("V17DONE pdf=%s\n", pdfName.Data());
    printf("V17DONE root=%s\n", rootName.Data());
    fflush(stdout);
}
