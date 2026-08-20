#include "comparePicoDstSamples_v11.C"

#include "TROOT.h"

#include <cstdio>
#include <math.h>

// ============================================================================
// comparePicoDstSamples_v13.C
//
// ROOT-5/CINT-safe PID and raw-ADC driver.  It keeps the v11 physics content
// and the v12 checkpointed workflow, but avoids every post-fill TMath call.
// This fixes the ROOT-5 error
//
//   Symbol TMath is not defined ... comparePicoDstSamples_v11.C:516
//
// observed while calculating the exclusive-PID binomial uncertainties.
//
// Physics content (unchanged relative to v11/v12):
//   * dE/dx versus p*sign(q), with q from sign(Track.mNHitsFit);
//   * exclusive pion/kaon/proton TPC nSigma selections;
//   * MC-truth efficiency, purity, and migration matrices;
//   * raw BTowHit.mAdc checks without using BTowHit.mE.
//
// Default output for OUT:
//   OUT_pid_adc_v13.pdf
//   OUT_pid_adc_v13.root
//
// Set runResponseV10=true only if OUT_response_v10.pdf/root should also be
// regenerated.  The default false avoids repeating the validated response QA.
// ============================================================================

namespace PicoQaV13 {

void checkpointV13(const char *sample, const char *stage)
{
    printf("V13DERIVE sample=%-10s stage=%s\n", sample, stage);
    fflush(stdout);
}

void deriveMeanDedxV13(PicoQaV11::SampleV11 &s)
{
    for (Int_t iX = 1;
         iX <= s.hDedxVsSignedPCounts->GetNbinsX(); ++iX) {
        Double_t sum = 0.0;
        Double_t sum2 = 0.0;
        Double_t count = 0.0;
        for (Int_t iY = 1;
             iY <= s.hDedxVsSignedPCounts->GetNbinsY(); ++iY) {
            const Double_t weight =
                s.hDedxVsSignedPCounts->GetBinContent(iX, iY);
            const Double_t dedx =
                s.hDedxVsSignedPCounts->GetYaxis()->GetBinCenter(iY);
            sum += weight * dedx;
            sum2 += weight * dedx * dedx;
            count += weight;
        }
        if (count > 0.0) {
            const Double_t mean = sum / count;
            Double_t variance = sum2 / count - mean * mean;
            if (variance < 0.0) variance = 0.0;
            s.hMeanDedxVsSignedP->SetBinContent(iX, mean);
            s.hMeanDedxVsSignedP->SetBinError(
                iX, sqrt(variance / count));
        }
    }
}

void binomialBinV13(TH1D *out,
                    Int_t iBin,
                    Double_t numerator,
                    Double_t denominator)
{
    if (!out) return;

    const Double_t value =
        denominator > 0.0 ? numerator / denominator : 0.0;
    Double_t variance = 0.0;
    if (denominator > 0.0) {
        variance = value * (1.0 - value) / denominator;
        if (variance < 0.0) variance = 0.0;
    }

    out->SetBinContent(iBin, value);
    out->SetBinError(iBin, sqrt(variance));
}

void deriveV13(PicoQaV11::SampleV11 &s)
{
    using namespace PicoQaV11;

    checkpointV13(s.label.Data(), "begin");

    deriveMeanDedxV13(s);
    checkpointV13(s.label.Data(), "mean_dedx_done");

    unitArea2DV11(s.hDedxVsSignedPShape);
    unitAreaV11(s.hNSigmaPionTruePion);
    unitAreaV11(s.hNSigmaKaonTrueKaon);
    unitAreaV11(s.hNSigmaProtonTrueProton);
    checkpointV13(s.label.Data(), "pid_shapes_done");

    unitAreaV11(s.hAdcAllShape);
    unitAreaV11(s.hAdcPositiveShape);
    unitAreaV11(s.hNPositiveAdcTowers);
    unitAreaV11(s.hSumPositiveAdc);
    checkpointV13(s.label.Data(), "adc_shapes_done");

    for (Int_t iPid = 1; iPid <= 4; ++iPid) {
        const Double_t truth =
            s.hPidTruthCounts->GetBinContent(iPid);
        const Double_t selected =
            s.hPidSelectedCounts->GetBinContent(iPid);
        const Double_t correct =
            s.hPidCorrectCounts->GetBinContent(iPid);

        binomialBinV13(s.hPidEfficiency, iPid, correct, truth);
        binomialBinV13(s.hPidPurity, iPid, correct, selected);

        for (Int_t iSelected = 1; iSelected <= 5; ++iSelected) {
            const Double_t count =
                s.hPidMatrixCounts->GetBinContent(iSelected, iPid);
            s.hPidMatrixFraction->SetBinContent(
                iSelected, iPid, truth > 0.0 ? count / truth : 0.0);
        }
    }
    checkpointV13(s.label.Data(), "pid_metrics_done");

    for (Int_t iTower = 1; iTower <= 4800; ++iTower) {
        const Double_t adcCount =
            s.hTowerAdcCount->GetBinContent(iTower);
        const Double_t adcSum =
            s.hTowerAdcSum->GetBinContent(iTower);
        if (adcCount > 0.0) {
            s.hTowerMeanAdc->SetBinContent(iTower, adcSum / adcCount);
        }

        const Double_t positiveCount =
            s.hTowerPositiveAdcCount->GetBinContent(iTower);
        const Double_t occupancy = s.nEvents > 0 ?
            positiveCount / (Double_t)s.nEvents : 0.0;
        s.hTowerPositiveAdcOccupancy->SetBinContent(iTower, occupancy);

        Double_t variance = 0.0;
        if (s.nEvents > 0 && occupancy >= 0.0 && occupancy <= 1.0) {
            variance = occupancy * (1.0 - occupancy) /
                       (Double_t)s.nEvents;
            if (variance < 0.0) variance = 0.0;
        }
        s.hTowerPositiveAdcOccupancy->SetBinError(
            iTower, sqrt(variance));
    }
    checkpointV13(s.label.Data(), "tower_metrics_done");
    checkpointV13(s.label.Data(), "done");
}

void drawDedxMapsV13(PicoQaV11::SampleV11 &a,
                     PicoQaV11::SampleV11 &b,
                     TFile *output,
                     const char *pdf,
                     Int_t &page,
                     Int_t totalPages)
{
    using namespace PicoQaV11;

    ++page;
    printf("V13PDF page %d / %d: dedx_vs_signed_p\n",
           page, totalPages);

    output->cd();
    a.hDedxVsSignedPCounts->Write("new_dedx_vs_signed_p_counts");
    b.hDedxVsSignedPCounts->Write("reference_dedx_vs_signed_p_counts");
    a.hDedxVsSignedPShape->Write("new_dedx_vs_signed_p_shape");
    b.hDedxVsSignedPShape->Write("reference_dedx_vs_signed_p_shape");

    const Double_t maximumA = a.hDedxVsSignedPShape->GetMaximum();
    const Double_t maximumB = b.hDedxVsSignedPShape->GetMaximum();
    const Double_t maximum = maximumA > maximumB ? maximumA : maximumB;
    const Double_t minimum =
        maximum > 0.0 ? maximum * 1.0e-5 : 1.0e-12;

    TCanvas *canvas = new TCanvas(
        "canvas_v13_dedx_maps",
        "TPC dE/dx versus signed momentum", 1600, 720);
    canvas->Divide(2, 1);
    TH2D *maps[2] = {
        a.hDedxVsSignedPShape, b.hDedxVsSignedPShape};
    const char *titles[2] = {"new", "reference"};
    for (Int_t i = 0; i < 2; ++i) {
        TPad *pad = (TPad*)canvas->cd(i + 1);
        pad->SetLogz();
        pad->SetLeftMargin(0.11);
        pad->SetRightMargin(0.16);
        pad->SetBottomMargin(0.12);
        maps[i]->SetTitle(Form("%s: TPC dE/dx", titles[i]));
        maps[i]->GetXaxis()->SetTitle("p #times sign(q) (GeV/c)");
        maps[i]->GetYaxis()->SetTitle("dE/dx (keV/cm)");
        maps[i]->GetZaxis()->SetTitle("fraction");
        maps[i]->SetMinimum(minimum);
        maps[i]->SetMaximum(maximum > 0.0 ? maximum : 1.0);
        maps[i]->Draw("COLZ");
    }
    printCanvasV11(canvas, pdf, page, totalPages);
    delete canvas;
}

} // namespace PicoQaV13

void comparePicoDstSamples_v13(
    const char *newFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_v13",
    Bool_t runResponseV10 = kFALSE)
{
    using namespace PicoQaV11;
    using namespace PicoQaV13;

    gStyle->SetOptStat(0);
    gStyle->SetPaintTextFormat(".3f");

    printf("V13STEP response_v10 enabled=%d\n", (Int_t)runResponseV10);
    fflush(stdout);
    if (runResponseV10) {
        comparePicoDstSamples_v10(newFileName, referenceFileName,
                                  outputBase, kFALSE);
        printf("V13STEP response_v10_done\n");
        fflush(stdout);
    }
    else {
        printf("V13SKIP response_v10 reason=runResponseV10_false\n");
    }

    gROOT->cd();

    TFile *newFile = TFile::Open(newFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!newFile || newFile->IsZombie() ||
        !referenceFile || referenceFile->IsZombie()) {
        printf("V13ERROR cannot open one or both input files\n");
        return;
    }

    TTree *newTree = (TTree*)newFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!newTree || !referenceTree) {
        printf("V13ERROR PicoDst tree is missing in one or both files\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    gROOT->cd();
    SampleV11 a;
    SampleV11 b;
    initV11(a, "newv13", "new");
    initV11(b, "refv13", "reference");

    const Bool_t filledNew = fillV11(newTree, a);
    printf("V13STEP fill_done sample=new status=%d\n",
           (Int_t)filledNew);
    fflush(stdout);
    if (!filledNew) {
        printf("V13ERROR new PID/ADC sample could not be filled\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    const Bool_t filledReference = fillV11(referenceTree, b);
    printf("V13STEP fill_done sample=reference status=%d\n",
           (Int_t)filledReference);
    fflush(stdout);
    if (!filledReference) {
        printf("V13ERROR reference PID/ADC sample could not be filled\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    printf("V13CONFIG signed_momentum=p*sign(q), q_from=sign(Track.mNHitsFit)\n");
    printf("V13CONFIG nsigma_leaf_scale=%.0f exclusive_cut=2.0\n",
           kNSigmaScaleV11);
    printf("V13CONFIG track_selection=idTruth>0,qaTruth>=%.0f,pT>=%.3f,absEta<%.3f,DCA<%.2f,nHitsFit>=%d,hitsRatio>=%.2f\n",
           kMinQaTruthV11, kMinRecoPtV11, kMaxRecoEtaV11,
           kMaxDcaV11, kMinNHitsFitV11, kMinNHitsRatioV11);
    printf("V13CONFIG selected_pid_tracks new=%lld reference=%lld\n",
           a.nPidTracks, b.nPidTracks);
    printf("V13CONFIG selected_dedx_tracks new=%lld reference=%lld\n",
           a.nDedxTracks, b.nDedxTracks);
    fflush(stdout);

    deriveV13(a);
    deriveV13(b);

    const Bool_t drawPid = a.hasPid && b.hasPid;
    const Bool_t drawAdc = a.hasAdc && b.hasAdc;
    const Int_t pidPages = drawPid ? 9 : 0;
    const Int_t adcPages = drawAdc ? 6 : 0;
    const Int_t totalPages = pidPages + adcPages;
    printf("V13CONFIG pages pid=%d adc=%d total=%d\n",
           pidPages, adcPages, totalPages);
    fflush(stdout);

    if (totalPages <= 0) {
        printf("V13ERROR neither common PID nor common ADC leaves are available\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    const TString rootName = Form("%s_pid_adc_v13.root", outputBase);
    const TString pdfName = Form("%s_pid_adc_v13.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V13ERROR cannot create output ROOT file %s\n",
               rootName.Data());
        newFile->Close();
        referenceFile->Close();
        return;
    }

    Int_t page = 0;
    if (drawPid) {
        output->cd();
        a.hPidTruthCounts->Write("new_exclusive_pid_truth_counts");
        b.hPidTruthCounts->Write("reference_exclusive_pid_truth_counts");
        a.hPidSelectedCounts->Write("new_exclusive_pid_selected_counts");
        b.hPidSelectedCounts->Write("reference_exclusive_pid_selected_counts");
        a.hPidCorrectCounts->Write("new_exclusive_pid_correct_counts");
        b.hPidCorrectCounts->Write("reference_exclusive_pid_correct_counts");

        drawDedxMapsV13(a, b, output, pdfName.Data(), page, totalPages);
        drawDedxRatioV11(a, b, output, pdfName.Data(), page, totalPages);
        drawPage1DV11(a.hMeanDedxVsSignedP, b.hMeanDedxVsSignedP,
            output, pdfName.Data(), "mean_dedx_vs_signed_p",
            "Mean TPC dE/dx versus signed momentum",
            "p #times sign(q) (GeV/c)", "#LTdE/dx#GT (keV/cm)",
            kFALSE, 0.5, 1.5, page, totalPages);
        drawPage1DV11(a.hNSigmaPionTruePion, b.hNSigmaPionTruePion,
            output, pdfName.Data(), "nsigma_pion_true_pion",
            "TPC n#sigma_{#pi} for true pions",
            "n#sigma_{#pi}", "fraction", kFALSE, 0.0, 2.0,
            page, totalPages);
        drawPage1DV11(a.hNSigmaKaonTrueKaon, b.hNSigmaKaonTrueKaon,
            output, pdfName.Data(), "nsigma_kaon_true_kaon",
            "TPC n#sigma_{K} for true kaons",
            "n#sigma_{K}", "fraction", kFALSE, 0.0, 2.0,
            page, totalPages);
        drawPage1DV11(a.hNSigmaProtonTrueProton,
            b.hNSigmaProtonTrueProton,
            output, pdfName.Data(), "nsigma_proton_true_proton",
            "TPC n#sigma_{p} for true protons and antiprotons",
            "n#sigma_{p}", "fraction", kFALSE, 0.0, 2.0,
            page, totalPages);
        drawPage1DV11(a.hPidEfficiency, b.hPidEfficiency,
            output, pdfName.Data(), "exclusive_pid_efficiency",
            "Efficiency of the exclusive TPC PID selection",
            "MC truth species", "correctly selected / true",
            kFALSE, 0.0, 2.0, page, totalPages);
        drawPage1DV11(a.hPidPurity, b.hPidPurity,
            output, pdfName.Data(), "exclusive_pid_purity",
            "Purity of the exclusive TPC PID selection",
            "selected species", "correct truth / selected",
            kFALSE, 0.0, 2.0, page, totalPages);
        drawPidMatricesV11(a, b, output, pdfName.Data(), page, totalPages);
        printPidV11(a);
        printPidV11(b);
    }
    else {
        printf("V13SKIP PID pages reason=missing_common_PID_leaves\n");
    }

    if (drawAdc) {
        drawPage1DV11(a.hAdcAllShape, b.hAdcAllShape,
            output, pdfName.Data(), "btow_adc_all_shape",
            "Raw BEMC tower ADC: all towers (unit area)",
            "ADC", "fraction", kTRUE, 0.0, 2.0,
            page, totalPages);
        drawPage1DV11(a.hAdcPositiveShape, b.hAdcPositiveShape,
            output, pdfName.Data(), "btow_adc_positive_shape",
            "Raw BEMC tower ADC: ADC > 0 (unit area)",
            "ADC", "fraction", kTRUE, 0.0, 2.0,
            page, totalPages);
        drawPage1DV11(a.hNPositiveAdcTowers, b.hNPositiveAdcTowers,
            output, pdfName.Data(), "n_positive_adc_towers",
            "Number of BEMC towers with ADC > 0 per event",
            "N_{tower}(ADC>0)", "fraction of events",
            kFALSE, 0.0, 2.0, page, totalPages);
        drawPage1DV11(a.hSumPositiveAdc, b.hSumPositiveAdc,
            output, pdfName.Data(), "sum_positive_adc",
            "Positive BEMC ADC sum per event",
            "#Sigma ADC, ADC>0", "fraction of events",
            kFALSE, 0.0, 2.0, page, totalPages);
        drawPage1DV11(a.hTowerMeanAdc, b.hTowerMeanAdc,
            output, pdfName.Data(), "tower_mean_adc",
            "Mean raw ADC by BEMC tower ID",
            "BEMC tower ID", "#LTADC#GT",
            kFALSE, 0.5, 1.5, page, totalPages);
        drawPage1DV11(a.hTowerPositiveAdcOccupancy,
            b.hTowerPositiveAdcOccupancy,
            output, pdfName.Data(), "tower_positive_adc_occupancy",
            "BEMC ADC-positive occupancy by tower ID",
            "BEMC tower ID", "fraction of events with ADC>0",
            kFALSE, 0.5, 1.5, page, totalPages);

        const Double_t adcMeanA = a.hAdcPositiveShape->GetMean();
        const Double_t adcMeanB = b.hAdcPositiveShape->GetMean();
        const Double_t meanPositiveTowersA = a.nEvents > 0 ?
            a.totalPositiveAdcTowers / (Double_t)a.nEvents : 0.0;
        const Double_t meanPositiveTowersB = b.nEvents > 0 ?
            b.totalPositiveAdcTowers / (Double_t)b.nEvents : 0.0;
        const Double_t meanPositiveSumA = a.nEvents > 0 ?
            a.totalPositiveAdc / (Double_t)a.nEvents : 0.0;
        const Double_t meanPositiveSumB = b.nEvents > 0 ?
            b.totalPositiveAdc / (Double_t)b.nEvents : 0.0;

        printf("V13ADC adc_positive_mean new=%g reference=%g ratio=%g\n",
               adcMeanA, adcMeanB,
               adcMeanB != 0.0 ? adcMeanA / adcMeanB : 0.0);
        printf("V13ADC positive_towers_per_event_mean new=%g reference=%g ratio=%g\n",
               meanPositiveTowersA, meanPositiveTowersB,
               meanPositiveTowersB != 0.0 ?
               meanPositiveTowersA / meanPositiveTowersB : 0.0);
        printf("V13ADC positive_adc_sum_per_event_mean new=%g reference=%g ratio=%g\n",
               meanPositiveSumA, meanPositiveSumB,
               meanPositiveSumB != 0.0 ?
               meanPositiveSumA / meanPositiveSumB : 0.0);
    }
    else {
        printf("V13SKIP ADC pages reason=missing_common_BTowHit_mAdc\n");
    }

    output->Write();
    output->Close();
    newFile->Close();
    referenceFile->Close();

    printf("V13DONE pid_adc_pdf=%s\n", pdfName.Data());
    printf("V13DONE pid_adc_root=%s\n", rootName.Data());
}
