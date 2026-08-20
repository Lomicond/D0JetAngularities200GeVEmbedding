#include "comparePicoDstSamples_v11.C"

#include "TROOT.h"

#include <cstdio>

// ==========================================================================
// comparePicoDstSamples_v12.C
//
// ROOT-5/CINT-safe driver for the PID and raw-ADC additions introduced in
// v11.  By default it does not rerun the already validated v10 response block.
// This also isolates the new histograms from global ROOT state left behind by
// the v10 PDF/ROOT output.
//
// The physics content is unchanged relative to v11:
//   * dE/dx versus p*sign(q);
//   * exclusive pion/kaon/proton TPC nSigma selections;
//   * MC-truth efficiency, purity, and migration matrices;
//   * raw BTowHit.mAdc checks with no use of BTowHit.mE.
//
// Output for OUT:
//   OUT_pid_adc_v12.pdf
//   OUT_pid_adc_v12.root
//
// If runResponseV10 is true, OUT_response_v10.pdf/root is also regenerated.
// ==========================================================================

namespace PicoQaV12 {

void checkpointV12(const char *sample, const char *stage)
{
    printf("V12DERIVE sample=%-10s stage=%s\n", sample, stage);
    fflush(stdout);
}

void deriveV12(PicoQaV11::SampleV11 &s)
{
    using namespace PicoQaV11;

    checkpointV12(s.label.Data(), "begin");

    deriveMeanDedxV11(s);
    checkpointV12(s.label.Data(), "mean_dedx_done");

    unitArea2DV11(s.hDedxVsSignedPShape);
    unitAreaV11(s.hNSigmaPionTruePion);
    unitAreaV11(s.hNSigmaKaonTrueKaon);
    unitAreaV11(s.hNSigmaProtonTrueProton);
    checkpointV12(s.label.Data(), "pid_shapes_done");

    unitAreaV11(s.hAdcAllShape);
    unitAreaV11(s.hAdcPositiveShape);
    unitAreaV11(s.hNPositiveAdcTowers);
    unitAreaV11(s.hSumPositiveAdc);
    checkpointV12(s.label.Data(), "adc_shapes_done");

    for (Int_t iPid = 1; iPid <= 4; ++iPid) {
        const Double_t truth = s.hPidTruthCounts->GetBinContent(iPid);
        const Double_t selected = s.hPidSelectedCounts->GetBinContent(iPid);
        const Double_t correct = s.hPidCorrectCounts->GetBinContent(iPid);
        binomialBinV11(s.hPidEfficiency, iPid, correct, truth);
        binomialBinV11(s.hPidPurity, iPid, correct, selected);

        for (Int_t iSelected = 1; iSelected <= 5; ++iSelected) {
            const Double_t count =
                s.hPidMatrixCounts->GetBinContent(iSelected, iPid);
            s.hPidMatrixFraction->SetBinContent(
                iSelected, iPid, truth > 0.0 ? count / truth : 0.0);
        }
    }
    checkpointV12(s.label.Data(), "pid_metrics_done");

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
        if (s.nEvents > 0 && occupancy >= 0.0 && occupancy <= 1.0) {
            s.hTowerPositiveAdcOccupancy->SetBinError(
                iTower,
                TMath::Sqrt(occupancy * (1.0 - occupancy) /
                            (Double_t)s.nEvents));
        }
    }
    checkpointV12(s.label.Data(), "tower_metrics_done");
    checkpointV12(s.label.Data(), "done");
}

} // namespace PicoQaV12

void comparePicoDstSamples_v12(
    const char *newFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_v12",
    Bool_t runResponseV10 = kFALSE)
{
    using namespace PicoQaV11;
    using namespace PicoQaV12;

    gStyle->SetOptStat(0);
    gStyle->SetPaintTextFormat(".3f");

    printf("V12STEP response_v10 enabled=%d\n", (Int_t)runResponseV10);
    fflush(stdout);
    if (runResponseV10) {
        comparePicoDstSamples_v10(newFileName, referenceFileName,
                                  outputBase, kFALSE);
        printf("V12STEP response_v10_done\n");
        fflush(stdout);
    }
    else {
        printf("V12SKIP response_v10 reason=runResponseV10_false\n");
    }

    // Ensure that newly created histograms are not associated with an input
    // or with the already closed output file from the optional v10 block.
    gROOT->cd();

    TFile *newFile = TFile::Open(newFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!newFile || newFile->IsZombie() ||
        !referenceFile || referenceFile->IsZombie()) {
        printf("V12ERROR cannot open one or both input files\n");
        return;
    }

    TTree *newTree = (TTree*)newFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!newTree || !referenceTree) {
        printf("V12ERROR PicoDst tree is missing in one or both files\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    gROOT->cd();
    SampleV11 a;
    SampleV11 b;
    initV11(a, "newv12", "new");
    initV11(b, "refv12", "reference");

    const Bool_t filledNew = fillV11(newTree, a);
    printf("V12STEP fill_done sample=new status=%d\n", (Int_t)filledNew);
    fflush(stdout);
    if (!filledNew) {
        printf("V12ERROR new PID/ADC sample could not be filled\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    const Bool_t filledReference = fillV11(referenceTree, b);
    printf("V12STEP fill_done sample=reference status=%d\n",
           (Int_t)filledReference);
    fflush(stdout);
    if (!filledReference) {
        printf("V12ERROR reference PID/ADC sample could not be filled\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    printf("V12CONFIG signed_momentum=p*sign(q), q_from=sign(Track.mNHitsFit)\n");
    printf("V12CONFIG nsigma_leaf_scale=%.0f exclusive_cut=2.0\n",
           kNSigmaScaleV11);
    printf("V12CONFIG track_selection=idTruth>0,qaTruth>=%.0f,pT>=%.3f,absEta<%.3f,DCA<%.2f,nHitsFit>=%d,hitsRatio>=%.2f\n",
           kMinQaTruthV11, kMinRecoPtV11, kMaxRecoEtaV11,
           kMaxDcaV11, kMinNHitsFitV11, kMinNHitsRatioV11);
    printf("V12CONFIG selected_pid_tracks new=%lld reference=%lld\n",
           a.nPidTracks, b.nPidTracks);
    printf("V12CONFIG selected_dedx_tracks new=%lld reference=%lld\n",
           a.nDedxTracks, b.nDedxTracks);
    fflush(stdout);

    deriveV12(a);
    deriveV12(b);

    const Bool_t drawPid = a.hasPid && b.hasPid;
    const Bool_t drawAdc = a.hasAdc && b.hasAdc;
    const Int_t pidPages = drawPid ? 9 : 0;
    const Int_t adcPages = drawAdc ? 6 : 0;
    const Int_t totalPages = pidPages + adcPages;
    printf("V12CONFIG pages pid=%d adc=%d total=%d\n",
           pidPages, adcPages, totalPages);
    fflush(stdout);

    if (totalPages <= 0) {
        printf("V12ERROR neither common PID nor common ADC leaves are available\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    const TString rootName = Form("%s_pid_adc_v12.root", outputBase);
    const TString pdfName = Form("%s_pid_adc_v12.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V12ERROR cannot create output ROOT file %s\n",
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

        drawDedxMapsV11(a, b, output, pdfName.Data(), page, totalPages);
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
        printf("V12SKIP PID pages reason=missing_common_PID_leaves\n");
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

        printf("V12ADC adc_positive_mean new=%g reference=%g ratio=%g\n",
               adcMeanA, adcMeanB,
               adcMeanB != 0.0 ? adcMeanA / adcMeanB : 0.0);
        printf("V12ADC positive_towers_per_event_mean new=%g reference=%g ratio=%g\n",
               meanPositiveTowersA, meanPositiveTowersB,
               meanPositiveTowersB != 0.0 ?
               meanPositiveTowersA / meanPositiveTowersB : 0.0);
        printf("V12ADC positive_adc_sum_per_event_mean new=%g reference=%g ratio=%g\n",
               meanPositiveSumA, meanPositiveSumB,
               meanPositiveSumB != 0.0 ?
               meanPositiveSumA / meanPositiveSumB : 0.0);
    }
    else {
        printf("V12SKIP ADC pages reason=missing_common_BTowHit_mAdc\n");
    }

    output->Write();
    output->Close();
    newFile->Close();
    referenceFile->Close();

    printf("V12DONE pid_adc_pdf=%s\n", pdfName.Data());
    printf("V12DONE pid_adc_root=%s\n", rootName.Data());
}
