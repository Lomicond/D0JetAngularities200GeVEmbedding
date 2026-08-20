#include "comparePicoDstSamples_v9.C"

#include "TBranch.h"

// ==========================================================================
// comparePicoDstSamples_v10.C
//
// ROOT 5.34/CINT compatibility fix for v9.  The histogram definitions,
// event-loop logic, efficiencies, and detector-response observables are kept
// unchanged.  Only the plot dispatcher is replaced: v9 created a local
// struct and std::vector after opening the response ROOT file, which can make
// old CINT terminate before the first V9CONFIG/V9QA message.  v10 dispatches
// every plot directly and therefore does not instantiate that local vector.
//
// Outputs for OUT:
//   OUT_inclusive_v8.pdf/root  -- original broad v8 comparison
//   OUT_response_v10.pdf/root  -- response/tune-separation comparison
// ==========================================================================

namespace PicoQaV10 {

void enableLeafBranchV10(TTree *tree, TLeaf *leaf)
{
    if (!tree || !leaf) return;

    TBranch *branch = leaf->GetBranch();
    if (branch) {
        tree->SetBranchStatus(branch->GetName(), 1);

        // Split PicoDst collections have a separate counter branch such as
        // Track, McTrack, BTofHit, or BTowHit.  Enable it explicitly as well.
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

void configureResponseBranchesV10(TTree *tree, const char *sample)
{
    using namespace PicoQaV9;
    if (!tree) return;

    TLeaf *leaves[20];
    Int_t nLeaves = 0;
    leaves[nLeaves++] = leafV9(tree, "Track.mGMomentumX");
    leaves[nLeaves++] = leafV9(tree, "Track.mGMomentumY");
    leaves[nLeaves++] = leafV9(tree, "Track.mGMomentumZ");
    leaves[nLeaves++] = leafV9(tree, "Track.mNHitsFit");
    leaves[nLeaves++] = leafV9(tree, "Track.mIdTruth");
    leaves[nLeaves++] = leafV9(tree, "Track.mQATruth");
    leaves[nLeaves++] = leafV9(tree, "McTrack.mId");
    leaves[nLeaves++] = leafV9(tree,
                               "McTrack.mGePid", "McTrack.mGeantId");
    leaves[nLeaves++] = leafV9(tree, "McTrack.mCharge");
    leaves[nLeaves++] = leafV9(tree, "McTrack.mPx");
    leaves[nLeaves++] = leafV9(tree, "McTrack.mPy");
    leaves[nLeaves++] = leafV9(tree, "McTrack.mPz");
    leaves[nLeaves++] = leafV9(tree,
                               "McTrack.mE", "McTrack.mEnergy");
    leaves[nLeaves++] = leafV9(tree,
                               "McTrack.mIdVtxStart", "McTrack.mIdVx");
    leaves[nLeaves++] = leafV9(tree,
                               "McTrack.mIdVtxStop", "McTrack.mIdVxEnd");
    leaves[nLeaves++] = leafV9(tree,
                               "BTofHit.mCellId", "BTofHit.mId");
    leaves[nLeaves++] = leafV9(tree, "BTowHit.mId");
    leaves[nLeaves++] = leafV9(tree, "BTowHit.mAdc");
    leaves[nLeaves++] = leafV9(tree, "BTowHit.mE");

    tree->SetCacheSize(0);
    tree->SetBranchStatus("*", 0);

    Int_t nPresent = 0;
    for (Int_t i = 0; i < nLeaves; ++i) {
        if (leaves[i]) {
            ++nPresent;
            enableLeafBranchV10(tree, leaves[i]);
        }
    }

    printf("V10IO sample=%s mode=selected_branches requested=%d present=%d cache=off\n",
           sample, nLeaves, nPresent);
}

void drawPageV10(TH1D *a,
                 TH1D *b,
                 TFile *output,
                 const char *pdf,
                 const char *key,
                 const char *title,
                 const char *xTitle,
                 const char *yTitle,
                 Bool_t logY,
                 Double_t ratioMin,
                 Double_t ratioMax,
                 Int_t &page,
                 Int_t totalPages)
{
    ++page;
    printf("V10PDF page %d / %d: %s\n", page, totalPages, key);
    PicoQaV9::drawV9(a, b, output, pdf, key, title, xTitle, yTitle,
                     logY, ratioMin, ratioMax,
                     page == 1, page == totalPages);
}

} // namespace PicoQaV10

void comparePicoDstSamples_v10(
    const char *newFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_v10",
    Bool_t runInclusiveV8 = kTRUE)
{
    using namespace PicoQaV9;
    using namespace PicoQaV10;

    gStyle->SetOptStat(0);

    if (runInclusiveV8) {
        const TString inclusiveBase = Form("%s_inclusive_v8", outputBase);
        printf("V10STEP inclusive-v8 output=%s\n", inclusiveBase.Data());
        comparePicoDstSamples_v8(newFileName, referenceFileName,
                                 inclusiveBase.Data());
    }

    TFile *newFile = TFile::Open(newFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!newFile || newFile->IsZombie() ||
        !referenceFile || referenceFile->IsZombie()) {
        printf("V10ERROR cannot open one or both input files\n");
        return;
    }

    TTree *newTree = (TTree*)newFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!newTree || !referenceTree) {
        printf("V10ERROR PicoDst tree is missing in one or both files\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    configureResponseBranchesV10(newTree, "new");
    configureResponseBranchesV10(referenceTree, "reference");

    SampleV9 a, b;
    initV9(a, "newv10", "new");
    initV9(b, "refv10", "reference");
    if (!fillV9(newTree, a) || !fillV9(referenceTree, b)) {
        printf("V10ERROR response comparison could not be filled\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }
    deriveV9(a);
    deriveV9(b);

    const TString rootName = Form("%s_response_v10.root", outputBase);
    const TString pdfName = Form("%s_response_v10.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V10ERROR cannot create output ROOT file %s\n",
               rootName.Data());
        newFile->Close();
        referenceFile->Close();
        return;
    }

    const Bool_t drawBTof = a.hasBTofCellId && b.hasBTofCellId;
    const Int_t totalPages = drawBTof ? 28 : 27;
    Int_t page = 0;

    printf("V10CONFIG efficiency_acceptance: charged, pT>=%.3f GeV/c, |eta|<%.3f, qaTruth>=50\n",
           kMinMcPtV9, kMaxMcEtaV9);
    printf("V10CONFIG primary_definition: McTrack.mIdVtxStart == 1 (when leaf exists)\n");
    printf("V10CONFIG new_entries=%lld reference_entries=%lld\n",
           a.nEvents, b.nEvents);

    drawPageV10(a.hNTracks, b.hNTracks, output, pdfName.Data(),
        "n_tracks_exact", "Reconstructed tracks per event (exact)",
        "N_{tracks}", "events^{-1}", kTRUE, 0.0, 2.0, page, totalPages);
    drawPageV10(a.hNMcTracks, b.hNMcTracks, output, pdfName.Data(),
        "n_mc_tracks_exact", "MC tracks per event (exact)",
        "N_{MC}", "events^{-1}", kTRUE, 0.0, 2.0, page, totalPages);
    drawPageV10(a.hNBTofHits, b.hNBTofHits, output, pdfName.Data(),
        "n_btof_hits_exact", "BTOF hits per event (exact)",
        "N_{BTOF}", "events^{-1}", kTRUE, 0.0, 2.0, page, totalPages);
    drawPageV10(a.hNBTowHits, b.hNBTowHits, output, pdfName.Data(),
        "n_btow_hits_exact", "BEMC tower entries per event (exact)",
        "N_{BTow}", "events^{-1}", kTRUE, 0.8, 1.2, page, totalPages);

    drawPageV10(a.hMcPtShape, b.hMcPtShape, output, pdfName.Data(),
        "mc_pt_shape", "MC p_{T} shape (unit area)",
        "p_{T}^{MC} (GeV/c)", "fraction", kTRUE, 0.0, 2.5, page, totalPages);
    drawPageV10(a.hMcEtaShape, b.hMcEtaShape, output, pdfName.Data(),
        "mc_eta_shape", "MC #eta shape (unit area)",
        "#eta^{MC}", "fraction", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hMcPhiShape, b.hMcPhiShape, output, pdfName.Data(),
        "mc_phi_shape", "MC #phi shape (unit area)",
        "#phi^{MC}", "fraction", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hMcGePidFraction, b.hMcGePidFraction, output, pdfName.Data(),
        "mc_gepid_fraction", "MC GEANT-PID fractions",
        "GEANT PID", "fraction", kTRUE, 0.0, 3.0, page, totalPages);

    drawPageV10(a.hEffAllPt, b.hEffAllPt, output, pdfName.Data(),
        "eff_all_pt", "Truth-matched tracking efficiency: all charged",
        "p_{T}^{MC} (GeV/c)", "efficiency", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hEffAllEta, b.hEffAllEta, output, pdfName.Data(),
        "eff_all_eta", "Truth-matched tracking efficiency vs #eta",
        "#eta^{MC}", "efficiency", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hEffAllPhi, b.hEffAllPhi, output, pdfName.Data(),
        "eff_all_phi", "Truth-matched tracking efficiency vs #phi",
        "#phi^{MC}", "efficiency", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hEffAllGePid, b.hEffAllGePid, output, pdfName.Data(),
        "eff_all_gepid", "Truth-matched tracking efficiency by GEANT PID",
        "GEANT PID", "efficiency", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hEffPrimaryPt, b.hEffPrimaryPt, output, pdfName.Data(),
        "eff_primary_pt", "Tracking efficiency: charged particles from MC vertex 1",
        "p_{T}^{MC} (GeV/c)", "efficiency", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hEffD0DaughterPt, b.hEffD0DaughterPt, output, pdfName.Data(),
        "eff_d0daughter_pt", "Tracking efficiency: D^{0} K/#pi daughters",
        "p_{T}^{MC} (GeV/c)", "efficiency", kFALSE, 0.5, 1.5, page, totalPages);

    drawPageV10(a.hPtResolution, b.hPtResolution, output, pdfName.Data(),
        "matched_pt_resolution", "Matched-track p_{T} response (unit area)",
        "(p_{T}^{reco}-p_{T}^{MC})/p_{T}^{MC}", "fraction",
        kTRUE, 0.0, 2.0, page, totalPages);
    drawPageV10(a.hEtaResidual, b.hEtaResidual, output, pdfName.Data(),
        "matched_eta_residual", "Matched-track #eta residual (unit area)",
        "#eta^{reco}-#eta^{MC}", "fraction", kTRUE, 0.0, 2.0, page, totalPages);
    drawPageV10(a.hPhiResidual, b.hPhiResidual, output, pdfName.Data(),
        "matched_phi_residual", "Matched-track #phi residual (unit area)",
        "#phi^{reco}-#phi^{MC}", "fraction", kTRUE, 0.0, 2.0, page, totalPages);
    drawPageV10(a.hMatchedNHitsFit, b.hMatchedNHitsFit, output, pdfName.Data(),
        "matched_nhitsfit_shape", "Matched-track nHitsFit (unit area)",
        "|nHitsFit|", "fraction", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hMatchedQATruth, b.hMatchedQATruth, output, pdfName.Data(),
        "matched_qatruth_shape", "Matched-track qaTruth (unit area)",
        "qaTruth", "fraction", kTRUE, 0.5, 1.5, page, totalPages);

    drawPageV10(a.hND0, b.hND0, output, pdfName.Data(),
        "n_d0", "D^{0}/#bar{D}^{0} per written event",
        "N_{D^{0}}", "events^{-1}", kTRUE, 0.0, 2.0, page, totalPages);
    drawPageV10(a.hD0Pt, b.hD0Pt, output, pdfName.Data(),
        "d0_pt_shape", "D^{0} p_{T} shape (unit area)",
        "p_{T}^{D^{0}} (GeV/c)", "fraction", kTRUE, 0.0, 3.0, page, totalPages);
    drawPageV10(a.hD0DecayCompleteness, b.hD0DecayCompleteness, output, pdfName.Data(),
        "d0_decay_completeness", "D^{0} #rightarrow K#pi truth completeness",
        "category", "fraction", kFALSE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hD0KPiMass, b.hD0KPiMass, output, pdfName.Data(),
        "d0_kpi_mass_shape", "Forced-decay K#pi invariant mass",
        "m_{K#pi} (GeV/c^{2})", "fraction", kTRUE, 0.5, 1.5, page, totalPages);
    drawPageV10(a.hD0MomentumClosure, b.hD0MomentumClosure, output, pdfName.Data(),
        "d0_momentum_closure_shape", "Forced-decay momentum closure",
        "|#Sigma#vec{p}_{daughter}-#vec{p}_{D^{0}}|/|#vec{p}_{D^{0}}|",
        "fraction", kTRUE, 0.0, 2.0, page, totalPages);

    if (drawBTof) {
        drawPageV10(a.hBTofCellId, b.hBTofCellId, output, pdfName.Data(),
            "btof_cell_id_shape", "BTOF cell-ID occupancy (unit area)",
            "cell ID", "fraction", kTRUE, 0.0, 3.0, page, totalPages);
    } else {
        printf("V10SKIP btof_cell_id_shape reason=missing_BTofHit_cellId_leaf\n");
    }

    drawPageV10(a.hBTowEnergyOverAdc, b.hBTowEnergyOverAdc, output, pdfName.Data(),
        "btow_energy_over_adc_shape", "BEMC positive E/ADC response (unit area)",
        "E/ADC (GeV/count)", "fraction", kTRUE, 0.0, 3.0, page, totalPages);
    drawPageV10(a.hBTowEnergyPositiveFraction, b.hBTowEnergyPositiveFraction,
        output, pdfName.Data(), "btow_energy_positive_fraction",
        "Fraction of ADC>0 towers with E>0", "N(E>0)/N(ADC>0)",
        "fraction", kFALSE, 0.7, 1.3, page, totalPages);
    drawPageV10(a.hTowerEnergyPositiveOccupancy,
        b.hTowerEnergyPositiveOccupancy, output, pdfName.Data(),
        "tower_energy_positive_occupancy", "BEMC E>0 occupancy by tower",
        "tower ID", "events^{-1}", kFALSE, 0.7, 1.3, page, totalPages);

    output->Write();
    output->Close();
    newFile->Close();
    referenceFile->Close();

    if (runInclusiveV8) {
        printf("V10DONE inclusive_pdf=%s_inclusive_v8.pdf\n", outputBase);
        printf("V10DONE inclusive_root=%s_inclusive_v8.root\n", outputBase);
    } else {
        printf("V10SKIP inclusive_v8 reason=runInclusiveV8_false\n");
    }
    printf("V10DONE response_pdf=%s\n", pdfName.Data());
    printf("V10DONE response_root=%s\n", rootName.Data());
}
