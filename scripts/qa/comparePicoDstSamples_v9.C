#include "comparePicoDstSamples_v8.C"

#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMath.h"
#include "TString.h"
#include "TStyle.h"

#include <cstdio>
#include <map>
#include <set>
#include <vector>

// ==========================================================================
// comparePicoDstSamples_v9.C
//
// v9 keeps the broad inclusive comparison from v8 and adds a second,
// response-oriented comparison.  The latter is designed to distinguish
// generator/tune differences from detector/reconstruction differences:
//
//   * collection sizes are obtained from array-leaf lengths, not Track_ etc.;
//   * MC spectra are also compared after unit-area normalization;
//   * truth-matched tracking efficiencies are compared versus pT, eta, phi,
//     and GEANT PID;
//   * pT, eta, and phi residuals are compared at fixed MC truth;
//   * D0 -> K pi decay completeness and D0-daughter efficiency are checked;
//   * BTOF cell occupancy and BEMC E/ADC response are compared;
//   * missing leaves are reported explicitly in the log.
//
// Outputs for OUT:
//   OUT_inclusive_v8.pdf/root  -- original broad v8 comparison
//   OUT_response_v9.pdf/root   -- response/tune-separation comparison
//
// ROOT5/CINT compatible; intended for STAR SL22c.
// ==========================================================================

namespace PicoQaV9 {

const Double_t kMinMcPtV9 = 0.2;
const Double_t kMaxMcEtaV9 = 1.0;

struct SampleV9 {
    TString prefix;
    TString label;
    Long64_t nEvents;

    TH1D *hNTracks;
    TH1D *hNMcTracks;
    TH1D *hNBTofHits;
    TH1D *hNBTowHits;

    TH1D *hMcPtShape;
    TH1D *hMcEtaShape;
    TH1D *hMcPhiShape;
    TH1D *hMcGePidFraction;

    TH1D *hEffAllDenPt;
    TH1D *hEffAllNumPt;
    TH1D *hEffAllDenEta;
    TH1D *hEffAllNumEta;
    TH1D *hEffAllDenPhi;
    TH1D *hEffAllNumPhi;
    TH1D *hEffAllDenGePid;
    TH1D *hEffAllNumGePid;

    TH1D *hEffPrimaryDenPt;
    TH1D *hEffPrimaryNumPt;

    TH1D *hEffD0DaughterDenPt;
    TH1D *hEffD0DaughterNumPt;

    TH1D *hEffAllPt;
    TH1D *hEffAllEta;
    TH1D *hEffAllPhi;
    TH1D *hEffAllGePid;
    TH1D *hEffPrimaryPt;
    TH1D *hEffD0DaughterPt;

    TH1D *hPtResolution;
    TH1D *hEtaResidual;
    TH1D *hPhiResidual;
    TH1D *hMatchedNHitsFit;
    TH1D *hMatchedQATruth;

    TH1D *hND0;
    TH1D *hD0Pt;
    TH1D *hD0DecayCompleteness;
    TH1D *hD0KPiMass;
    TH1D *hD0MomentumClosure;

    TH1D *hBTofCellId;
    TH1D *hBTowEnergyOverAdc;
    TH1D *hBTowEnergyPositiveFraction;

    TH1D *hTowerEnergyPositiveCount;
    TH1D *hTowerEnergyPositiveOccupancy;

    Bool_t hasMcId;
    Bool_t hasMcCharge;
    Bool_t hasMcStartVertex;
    Bool_t hasMcStopVertex;
    Bool_t hasBTofCellId;
};

TH1D *histV9(const TString &prefix,
             const char *key,
             Int_t nBins,
             Double_t minimum,
             Double_t maximum)
{
    TH1D *h = new TH1D(Form("%s_%s", prefix.Data(), key), "",
                       nBins, minimum, maximum);
    h->SetDirectory(0);
    h->Sumw2();
    return h;
}

void initV9(SampleV9 &s, const char *prefix, const char *label)
{
    s.prefix = prefix;
    s.label = label;
    s.nEvents = 0;

    // Fine binning fixes the misleading first-bin centres printed by v8.
    s.hNTracks   = histV9(s.prefix, "n_tracks_exact",   401, -0.5, 400.5);
    s.hNMcTracks = histV9(s.prefix, "n_mc_tracks_exact", 1001, -0.5, 1000.5);
    s.hNBTofHits = histV9(s.prefix, "n_btof_hits_exact", 401, -0.5, 400.5);
    s.hNBTowHits = histV9(s.prefix, "n_btow_hits_exact", 4801, -0.5, 4800.5);

    s.hMcPtShape       = histV9(s.prefix, "mc_pt_shape", 100, 0.0, 10.0);
    s.hMcEtaShape      = histV9(s.prefix, "mc_eta_shape", 80, -2.0, 2.0);
    s.hMcPhiShape      = histV9(s.prefix, "mc_phi_shape", 72, -TMath::Pi(), TMath::Pi());
    s.hMcGePidFraction = histV9(s.prefix, "mc_gepid_fraction", 60, -0.5, 59.5);

    s.hEffAllDenPt    = histV9(s.prefix, "eff_all_den_pt", 49, 0.2, 10.0);
    s.hEffAllNumPt    = histV9(s.prefix, "eff_all_num_pt", 49, 0.2, 10.0);
    s.hEffAllDenEta   = histV9(s.prefix, "eff_all_den_eta", 40, -1.0, 1.0);
    s.hEffAllNumEta   = histV9(s.prefix, "eff_all_num_eta", 40, -1.0, 1.0);
    s.hEffAllDenPhi   = histV9(s.prefix, "eff_all_den_phi", 36, -TMath::Pi(), TMath::Pi());
    s.hEffAllNumPhi   = histV9(s.prefix, "eff_all_num_phi", 36, -TMath::Pi(), TMath::Pi());
    s.hEffAllDenGePid = histV9(s.prefix, "eff_all_den_gepid", 60, -0.5, 59.5);
    s.hEffAllNumGePid = histV9(s.prefix, "eff_all_num_gepid", 60, -0.5, 59.5);

    s.hEffPrimaryDenPt = histV9(s.prefix, "eff_primary_den_pt", 49, 0.2, 10.0);
    s.hEffPrimaryNumPt = histV9(s.prefix, "eff_primary_num_pt", 49, 0.2, 10.0);

    s.hEffD0DaughterDenPt = histV9(s.prefix, "eff_d0daughter_den_pt", 49, 0.2, 10.0);
    s.hEffD0DaughterNumPt = histV9(s.prefix, "eff_d0daughter_num_pt", 49, 0.2, 10.0);

    s.hEffAllPt         = histV9(s.prefix, "eff_all_pt", 49, 0.2, 10.0);
    s.hEffAllEta        = histV9(s.prefix, "eff_all_eta", 40, -1.0, 1.0);
    s.hEffAllPhi        = histV9(s.prefix, "eff_all_phi", 36, -TMath::Pi(), TMath::Pi());
    s.hEffAllGePid      = histV9(s.prefix, "eff_all_gepid", 60, -0.5, 59.5);
    s.hEffPrimaryPt     = histV9(s.prefix, "eff_primary_pt", 49, 0.2, 10.0);
    s.hEffD0DaughterPt  = histV9(s.prefix, "eff_d0daughter_pt", 49, 0.2, 10.0);

    s.hPtResolution     = histV9(s.prefix, "matched_pt_resolution", 160, -0.8, 0.8);
    s.hEtaResidual      = histV9(s.prefix, "matched_eta_residual", 160, -0.08, 0.08);
    s.hPhiResidual      = histV9(s.prefix, "matched_phi_residual", 160, -0.08, 0.08);
    s.hMatchedNHitsFit  = histV9(s.prefix, "matched_nhitsfit_shape", 50, -0.5, 49.5);
    s.hMatchedQATruth   = histV9(s.prefix, "matched_qatruth_shape", 101, -0.5, 100.5);

    s.hND0                  = histV9(s.prefix, "n_d0", 11, -0.5, 10.5);
    s.hD0Pt                 = histV9(s.prefix, "d0_pt_shape", 100, 0.0, 25.0);
    s.hD0DecayCompleteness  = histV9(s.prefix, "d0_decay_completeness", 3, -0.5, 2.5);
    s.hD0DecayCompleteness->GetXaxis()->SetBinLabel(1, "missing both");
    s.hD0DecayCompleteness->GetXaxis()->SetBinLabel(2, "one K/#pi");
    s.hD0DecayCompleteness->GetXaxis()->SetBinLabel(3, "complete K#pi");
    s.hD0KPiMass = histV9(s.prefix, "d0_kpi_mass_shape", 120, 1.80, 1.92);
    s.hD0MomentumClosure = histV9(s.prefix, "d0_momentum_closure_shape", 200, 0.0, 0.02);

    s.hBTofCellId = histV9(s.prefix, "btof_cell_id_shape", 240, 0.5, 24000.5);
    s.hBTowEnergyOverAdc = histV9(s.prefix, "btow_energy_over_adc_shape", 200, 0.0, 0.02);
    s.hBTowEnergyPositiveFraction = histV9(s.prefix, "btow_energy_positive_fraction", 101, -0.005, 1.005);

    s.hTowerEnergyPositiveCount = histV9(s.prefix, "tower_energy_positive_count", 4800, 0.5, 4800.5);
    s.hTowerEnergyPositiveOccupancy = histV9(s.prefix, "tower_energy_positive_occupancy", 4800, 0.5, 4800.5);

    s.hasMcId = kFALSE;
    s.hasMcCharge = kFALSE;
    s.hasMcStartVertex = kFALSE;
    s.hasMcStopVertex = kFALSE;
    s.hasBTofCellId = kFALSE;
}

Bool_t finiteV9(Double_t x)
{
    return x == x && TMath::Abs(x) < 1.0e30;
}

Double_t valueV9(TLeaf *leaf, Int_t index, Double_t fallback = 0.0)
{
    if (!leaf || index < 0 || index >= leaf->GetNdata()) return fallback;
    const Double_t x = leaf->GetValue(index);
    return finiteV9(x) ? x : fallback;
}

TLeaf *leafV9(TTree *tree,
              const char *name1,
              const char *name2 = 0,
              const char *name3 = 0)
{
    if (!tree) return 0;
    TLeaf *leaf = name1 ? tree->GetLeaf(name1) : 0;
    if (!leaf && name2) leaf = tree->GetLeaf(name2);
    if (!leaf && name3) leaf = tree->GetLeaf(name3);
    return leaf;
}

Double_t etaV9(Double_t px, Double_t py, Double_t pz)
{
    const Double_t pt = TMath::Sqrt(px * px + py * py);
    const Double_t p = TMath::Sqrt(pt * pt + pz * pz);
    if (pt <= 1.0e-12 || p <= TMath::Abs(pz)) return 1.0e30;
    return 0.5 * TMath::Log((p + pz) / (p - pz));
}

Double_t deltaPhiV9(Double_t a, Double_t b)
{
    Double_t d = a - b;
    while (d > TMath::Pi()) d -= 2.0 * TMath::Pi();
    while (d <= -TMath::Pi()) d += 2.0 * TMath::Pi();
    return d;
}

Bool_t chargedGePidV9(Int_t gePid)
{
    switch (gePid) {
        case 2: case 3: case 5: case 6:
        case 8: case 9: case 11: case 12:
        case 14: case 15: case 19: case 21:
        case 23: case 24: case 27: case 29:
        case 31: case 32:
            return kTRUE;
        default:
            return kFALSE;
    }
}

Bool_t pionGePidV9(Int_t gePid)
{
    return gePid == 8 || gePid == 9;
}

Bool_t kaonGePidV9(Int_t gePid)
{
    return gePid == 11 || gePid == 12;
}

void reportLeafV9(const char *sample, const char *logicalName, TLeaf *leaf)
{
    printf("V9LEAF sample=%-10s %-24s : %s\n",
           sample, logicalName, leaf ? leaf->GetName() : "MISSING");
}

Bool_t fillV9(TTree *tree, SampleV9 &s)
{
    if (!tree) return kFALSE;

    TLeaf *trackPx = leafV9(tree, "Track.mGMomentumX");
    TLeaf *trackPy = leafV9(tree, "Track.mGMomentumY");
    TLeaf *trackPz = leafV9(tree, "Track.mGMomentumZ");
    TLeaf *trackNHitsFit = leafV9(tree, "Track.mNHitsFit");
    TLeaf *trackIdTruth = leafV9(tree, "Track.mIdTruth");
    TLeaf *trackQATruth = leafV9(tree, "Track.mQATruth");

    TLeaf *mcId = leafV9(tree, "McTrack.mId");
    TLeaf *mcGePid = leafV9(tree, "McTrack.mGePid", "McTrack.mGeantId");
    TLeaf *mcCharge = leafV9(tree, "McTrack.mCharge");
    TLeaf *mcPx = leafV9(tree, "McTrack.mPx");
    TLeaf *mcPy = leafV9(tree, "McTrack.mPy");
    TLeaf *mcPz = leafV9(tree, "McTrack.mPz");
    TLeaf *mcEnergy = leafV9(tree, "McTrack.mE", "McTrack.mEnergy");
    TLeaf *mcStartVtx = leafV9(tree, "McTrack.mIdVtxStart", "McTrack.mIdVx");
    TLeaf *mcStopVtx = leafV9(tree, "McTrack.mIdVtxStop", "McTrack.mIdVxEnd");

    TLeaf *btofCellId = leafV9(tree, "BTofHit.mCellId", "BTofHit.mId");
    TLeaf *btowId = leafV9(tree, "BTowHit.mId");
    TLeaf *btowAdc = leafV9(tree, "BTowHit.mAdc");
    TLeaf *btowEnergy = leafV9(tree, "BTowHit.mE");

    reportLeafV9(s.label.Data(), "Track momentum", trackPx);
    reportLeafV9(s.label.Data(), "Track idTruth", trackIdTruth);
    reportLeafV9(s.label.Data(), "Track qaTruth", trackQATruth);
    reportLeafV9(s.label.Data(), "McTrack id", mcId);
    reportLeafV9(s.label.Data(), "McTrack charge", mcCharge);
    reportLeafV9(s.label.Data(), "McTrack start vertex", mcStartVtx);
    reportLeafV9(s.label.Data(), "McTrack stop vertex", mcStopVtx);
    reportLeafV9(s.label.Data(), "McTrack energy", mcEnergy);
    reportLeafV9(s.label.Data(), "BTof cell id", btofCellId);
    reportLeafV9(s.label.Data(), "BTow ADC", btowAdc);
    reportLeafV9(s.label.Data(), "BTow energy", btowEnergy);

    if (!trackPx || !trackPy || !mcPx || !mcPy || !mcGePid) {
        printf("V9ERROR sample=%s essential Track/McTrack leaves are missing\n",
               s.label.Data());
        return kFALSE;
    }

    s.hasMcId = mcId != 0;
    s.hasMcCharge = mcCharge != 0;
    s.hasMcStartVertex = mcStartVtx != 0;
    s.hasMcStopVertex = mcStopVtx != 0;
    s.hasBTofCellId = btofCellId != 0;

    s.nEvents = tree->GetEntries();
    printf("V9READ sample=%s events=%lld\n", s.label.Data(), s.nEvents);

    for (Long64_t iEvent = 0; iEvent < s.nEvents; ++iEvent) {
        tree->GetEntry(iEvent);
        if (iEvent % 100 == 0) {
            printf("  V9 %s event %lld / %lld\n",
                   s.label.Data(), iEvent, s.nEvents);
        }

        const Int_t nTracks = trackPx->GetNdata();
        const Int_t nMc = mcPx->GetNdata();
        const Int_t nBTof = btofCellId ? btofCellId->GetNdata() : 0;
        const Int_t nBTow = btowAdc ? btowAdc->GetNdata() : 0;

        s.hNTracks->Fill(nTracks);
        s.hNMcTracks->Fill(nMc);
        s.hNBTofHits->Fill(nBTof);
        s.hNBTowHits->Fill(nBTow);

        std::map<Int_t, Int_t> mcIndex;
        std::set<Int_t> matchedGoodIds;

        for (Int_t iMc = 0; iMc < nMc; ++iMc) {
            const Int_t id = mcId ? (Int_t)(valueV9(mcId, iMc, iMc + 1) + 0.5) : iMc + 1;
            mcIndex[id] = iMc;

            const Double_t px = valueV9(mcPx, iMc);
            const Double_t py = valueV9(mcPy, iMc);
            const Double_t pz = valueV9(mcPz, iMc);
            const Double_t pt = TMath::Sqrt(px * px + py * py);
            const Double_t eta = etaV9(px, py, pz);
            const Double_t phi = TMath::ATan2(py, px);
            const Int_t gePid = (Int_t)(valueV9(mcGePid, iMc) + 0.5);

            if (finiteV9(pt)) s.hMcPtShape->Fill(pt);
            if (finiteV9(eta)) s.hMcEtaShape->Fill(eta);
            if (finiteV9(phi)) s.hMcPhiShape->Fill(phi);
            s.hMcGePidFraction->Fill(gePid);
        }

        for (Int_t iTrack = 0; iTrack < nTracks; ++iTrack) {
            if (!trackIdTruth) continue;
            const Int_t idTruth = (Int_t)(valueV9(trackIdTruth, iTrack) + 0.5);
            const Double_t qaTruth = trackQATruth ? valueV9(trackQATruth, iTrack) : 100.0;
            if (idTruth <= 0 || qaTruth < 50.0) continue;

            std::map<Int_t, Int_t>::const_iterator found = mcIndex.find(idTruth);
            if (found == mcIndex.end()) continue;
            matchedGoodIds.insert(idTruth);

            const Int_t iMc = found->second;
            const Double_t recoPx = valueV9(trackPx, iTrack);
            const Double_t recoPy = valueV9(trackPy, iTrack);
            const Double_t recoPz = valueV9(trackPz, iTrack);
            const Double_t mcPxV = valueV9(mcPx, iMc);
            const Double_t mcPyV = valueV9(mcPy, iMc);
            const Double_t mcPzV = valueV9(mcPz, iMc);

            const Double_t recoPt = TMath::Sqrt(recoPx * recoPx + recoPy * recoPy);
            const Double_t mcPtV = TMath::Sqrt(mcPxV * mcPxV + mcPyV * mcPyV);
            const Double_t recoEta = etaV9(recoPx, recoPy, recoPz);
            const Double_t mcEtaV = etaV9(mcPxV, mcPyV, mcPzV);
            const Double_t recoPhi = TMath::ATan2(recoPy, recoPx);
            const Double_t mcPhiV = TMath::ATan2(mcPyV, mcPxV);

            if (mcPtV > 1.0e-12) {
                s.hPtResolution->Fill((recoPt - mcPtV) / mcPtV);
            }
            if (finiteV9(recoEta) && finiteV9(mcEtaV)) {
                s.hEtaResidual->Fill(recoEta - mcEtaV);
            }
            s.hPhiResidual->Fill(deltaPhiV9(recoPhi, mcPhiV));
            if (trackNHitsFit) s.hMatchedNHitsFit->Fill(TMath::Abs(valueV9(trackNHitsFit, iTrack)));
            if (trackQATruth) s.hMatchedQATruth->Fill(qaTruth);
        }

        for (Int_t iMc = 0; iMc < nMc; ++iMc) {
            const Int_t id = mcId ? (Int_t)(valueV9(mcId, iMc, iMc + 1) + 0.5) : iMc + 1;
            const Int_t gePid = (Int_t)(valueV9(mcGePid, iMc) + 0.5);
            const Double_t charge = mcCharge ? valueV9(mcCharge, iMc) : (chargedGePidV9(gePid) ? 1.0 : 0.0);
            const Double_t px = valueV9(mcPx, iMc);
            const Double_t py = valueV9(mcPy, iMc);
            const Double_t pz = valueV9(mcPz, iMc);
            const Double_t pt = TMath::Sqrt(px * px + py * py);
            const Double_t eta = etaV9(px, py, pz);
            const Double_t phi = TMath::ATan2(py, px);
            const Bool_t matched = matchedGoodIds.find(id) != matchedGoodIds.end();

            const Bool_t eligible = TMath::Abs(charge) > 0.01 &&
                                    pt >= kMinMcPtV9 &&
                                    finiteV9(eta) && TMath::Abs(eta) < kMaxMcEtaV9;
            if (!eligible) continue;

            s.hEffAllDenPt->Fill(pt);
            s.hEffAllDenEta->Fill(eta);
            s.hEffAllDenPhi->Fill(phi);
            s.hEffAllDenGePid->Fill(gePid);
            if (matched) {
                s.hEffAllNumPt->Fill(pt);
                s.hEffAllNumEta->Fill(eta);
                s.hEffAllNumPhi->Fill(phi);
                s.hEffAllNumGePid->Fill(gePid);
            }

            const Int_t startVtx = mcStartVtx ? (Int_t)(valueV9(mcStartVtx, iMc) + 0.5) : 1;
            if (startVtx == 1) {
                s.hEffPrimaryDenPt->Fill(pt);
                if (matched) s.hEffPrimaryNumPt->Fill(pt);
            }
        }

        Int_t nD0 = 0;
        for (Int_t iD0 = 0; iD0 < nMc; ++iD0) {
            const Int_t d0GePid = (Int_t)(valueV9(mcGePid, iD0) + 0.5);
            if (d0GePid != 37 && d0GePid != 38) continue;
            ++nD0;

            const Double_t d0Px = valueV9(mcPx, iD0);
            const Double_t d0Py = valueV9(mcPy, iD0);
            s.hD0Pt->Fill(TMath::Sqrt(d0Px * d0Px + d0Py * d0Py));

            const Int_t decayVtx = mcStopVtx ? (Int_t)(valueV9(mcStopVtx, iD0) + 0.5) : -1;
            Int_t nAcceptedDaughters = 0;
            Bool_t hasPion = kFALSE;
            Bool_t hasKaon = kFALSE;
            Int_t pionIndex = -1;
            Int_t kaonIndex = -1;

            if (decayVtx > 0 && mcStartVtx) {
                for (Int_t iDaughter = 0; iDaughter < nMc; ++iDaughter) {
                    const Int_t startVtx = (Int_t)(valueV9(mcStartVtx, iDaughter) + 0.5);
                    if (startVtx != decayVtx) continue;
                    const Int_t gePid = (Int_t)(valueV9(mcGePid, iDaughter) + 0.5);
                    if (!pionGePidV9(gePid) && !kaonGePidV9(gePid)) continue;

                    if (pionGePidV9(gePid)) {
                        hasPion = kTRUE;
                        if (pionIndex < 0) pionIndex = iDaughter;
                    }
                    if (kaonGePidV9(gePid)) {
                        hasKaon = kTRUE;
                        if (kaonIndex < 0) kaonIndex = iDaughter;
                    }
                    ++nAcceptedDaughters;

                    const Double_t px = valueV9(mcPx, iDaughter);
                    const Double_t py = valueV9(mcPy, iDaughter);
                    const Double_t pz = valueV9(mcPz, iDaughter);
                    const Double_t pt = TMath::Sqrt(px * px + py * py);
                    const Double_t eta = etaV9(px, py, pz);
                    if (pt < kMinMcPtV9 || !finiteV9(eta) || TMath::Abs(eta) >= kMaxMcEtaV9) continue;

                    const Int_t id = mcId ? (Int_t)(valueV9(mcId, iDaughter, iDaughter + 1) + 0.5) : iDaughter + 1;
                    s.hEffD0DaughterDenPt->Fill(pt);
                    if (matchedGoodIds.find(id) != matchedGoodIds.end()) {
                        s.hEffD0DaughterNumPt->Fill(pt);
                    }
                }
            }

            Int_t completeness = 0;
            if (hasPion || hasKaon || nAcceptedDaughters == 1) completeness = 1;
            if (hasPion && hasKaon && nAcceptedDaughters >= 2) completeness = 2;
            s.hD0DecayCompleteness->Fill(completeness);

            // These quantities test the forced StarPythia8Decayer step itself.
            // They are independent of the pp tune used by the first PYTHIA.
            if (pionIndex >= 0 && kaonIndex >= 0 && mcEnergy) {
                const Double_t piPx = valueV9(mcPx, pionIndex);
                const Double_t piPy = valueV9(mcPy, pionIndex);
                const Double_t piPz = valueV9(mcPz, pionIndex);
                const Double_t piE  = valueV9(mcEnergy, pionIndex);
                const Double_t kPx = valueV9(mcPx, kaonIndex);
                const Double_t kPy = valueV9(mcPy, kaonIndex);
                const Double_t kPz = valueV9(mcPz, kaonIndex);
                const Double_t kE  = valueV9(mcEnergy, kaonIndex);
                const Double_t sumE = piE + kE;
                const Double_t sumPx = piPx + kPx;
                const Double_t sumPy = piPy + kPy;
                const Double_t sumPz = piPz + kPz;
                const Double_t mass2 = sumE * sumE -
                    sumPx * sumPx - sumPy * sumPy - sumPz * sumPz;
                if (mass2 >= 0.0) s.hD0KPiMass->Fill(TMath::Sqrt(mass2));

                const Double_t deltaPx = sumPx - d0Px;
                const Double_t deltaPy = sumPy - d0Py;
                const Double_t deltaPz = sumPz - valueV9(mcPz, iD0);
                const Double_t d0P = TMath::Sqrt(d0Px * d0Px +
                    d0Py * d0Py + valueV9(mcPz, iD0) * valueV9(mcPz, iD0));
                const Double_t closureDenominator = d0P > 1.0e-9 ? d0P : 1.0;
                const Double_t closure = TMath::Sqrt(deltaPx * deltaPx +
                    deltaPy * deltaPy + deltaPz * deltaPz) / closureDenominator;
                s.hD0MomentumClosure->Fill(closure);
            }
        }
        s.hND0->Fill(nD0);

        if (btofCellId) {
            for (Int_t iHit = 0; iHit < nBTof; ++iHit) {
                s.hBTofCellId->Fill(valueV9(btofCellId, iHit));
            }
        }

        Int_t nPositiveAdc = 0;
        Int_t nPositiveEnergy = 0;
        for (Int_t iTower = 0; iTower < nBTow; ++iTower) {
            const Double_t adc = valueV9(btowAdc, iTower);
            const Double_t energy = btowEnergy ? valueV9(btowEnergy, iTower) / 1000.0 : 0.0;
            const Int_t towerId = btowId ?
                towerIdV5(valueV9(btowId, iTower)) :
                iTower + 1;

            if (adc > 0.0) ++nPositiveAdc;
            if (energy > 0.0) {
                ++nPositiveEnergy;
                if (towerId >= 1 && towerId <= 4800) {
                    s.hTowerEnergyPositiveCount->Fill(towerId);
                }
            }
            if (adc > 0.0 && energy > 0.0) {
                s.hBTowEnergyOverAdc->Fill(energy / adc);
            }
        }
        if (nPositiveAdc > 0) {
            s.hBTowEnergyPositiveFraction->Fill((Double_t)nPositiveEnergy / (Double_t)nPositiveAdc);
        }
    }

    return kTRUE;
}

void scalePerEventV9(TH1D *h, Long64_t nEvents)
{
    if (h && nEvents > 0) h->Scale(1.0 / (Double_t)nEvents);
}

void unitAreaV9(TH1D *h)
{
    if (!h) return;
    const Double_t integral = h->Integral(0, h->GetNbinsX() + 1);
    if (integral > 0.0) h->Scale(1.0 / integral);
}

void efficiencyV9(TH1D *out, TH1D *num, TH1D *den)
{
    if (!out || !num || !den) return;
    out->Reset();
    out->Divide(num, den, 1.0, 1.0, "B");
}

void deriveV9(SampleV9 &s)
{
    efficiencyV9(s.hEffAllPt, s.hEffAllNumPt, s.hEffAllDenPt);
    efficiencyV9(s.hEffAllEta, s.hEffAllNumEta, s.hEffAllDenEta);
    efficiencyV9(s.hEffAllPhi, s.hEffAllNumPhi, s.hEffAllDenPhi);
    efficiencyV9(s.hEffAllGePid, s.hEffAllNumGePid, s.hEffAllDenGePid);
    efficiencyV9(s.hEffPrimaryPt, s.hEffPrimaryNumPt, s.hEffPrimaryDenPt);
    efficiencyV9(s.hEffD0DaughterPt, s.hEffD0DaughterNumPt, s.hEffD0DaughterDenPt);

    scalePerEventV9(s.hNTracks, s.nEvents);
    scalePerEventV9(s.hNMcTracks, s.nEvents);
    scalePerEventV9(s.hNBTofHits, s.nEvents);
    scalePerEventV9(s.hNBTowHits, s.nEvents);
    scalePerEventV9(s.hND0, s.nEvents);

    unitAreaV9(s.hMcPtShape);
    unitAreaV9(s.hMcEtaShape);
    unitAreaV9(s.hMcPhiShape);
    unitAreaV9(s.hMcGePidFraction);
    unitAreaV9(s.hPtResolution);
    unitAreaV9(s.hEtaResidual);
    unitAreaV9(s.hPhiResidual);
    unitAreaV9(s.hMatchedNHitsFit);
    unitAreaV9(s.hMatchedQATruth);
    unitAreaV9(s.hD0Pt);
    unitAreaV9(s.hD0DecayCompleteness);
    unitAreaV9(s.hD0KPiMass);
    unitAreaV9(s.hD0MomentumClosure);
    unitAreaV9(s.hBTofCellId);
    unitAreaV9(s.hBTowEnergyOverAdc);
    unitAreaV9(s.hBTowEnergyPositiveFraction);

    for (Int_t iBin = 1; iBin <= 4800; ++iBin) {
        const Double_t count = s.hTowerEnergyPositiveCount->GetBinContent(iBin);
        const Double_t occupancy = s.nEvents > 0 ? count / (Double_t)s.nEvents : 0.0;
        s.hTowerEnergyPositiveOccupancy->SetBinContent(iBin, occupancy);
        if (s.nEvents > 0 && occupancy >= 0.0 && occupancy <= 1.0) {
            s.hTowerEnergyPositiveOccupancy->SetBinError(
                iBin, TMath::Sqrt(occupancy * (1.0 - occupancy) / (Double_t)s.nEvents));
        }
    }
}

void printV9(const char *key, TH1D *a, TH1D *b)
{
    if (!a || !b) return;
    const Double_t meanA = a->GetMean();
    const Double_t meanB = b->GetMean();
    const Double_t ratio = meanB != 0.0 ? meanA / meanB : 0.0;
    const Double_t ks = (a->Integral() > 0.0 && b->Integral() > 0.0) ? a->KolmogorovTest(b) : -1.0;
    printf("V9QA %-30s mean new=%11.6g ref=%11.6g ratio=%9.5f KS=%10.4g\n",
           key, meanA, meanB, ratio, ks);
}

void drawV9(TH1D *a,
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
            Bool_t first,
            Bool_t last)
{
    if (!a || !b) return;
    a->SetLineColor(kRed + 1);
    a->SetMarkerColor(kRed + 1);
    a->SetMarkerStyle(20);
    a->SetMarkerSize(0.55);
    b->SetLineColor(kBlack);
    b->SetMarkerColor(kBlack);
    b->SetMarkerStyle(24);
    b->SetMarkerSize(0.55);

    TH1D *ratio = (TH1D*)a->Clone(Form("ratio_v9_%s", key));
    ratio->SetDirectory(0);
    ratio->Divide(b);

    output->cd();
    a->Write(Form("new_%s", key));
    b->Write(Form("reference_%s", key));
    ratio->Write();

    TCanvas *c = new TCanvas(Form("canvas_v9_%s", key), title, 900, 800);
    TPad *up = new TPad(Form("up_v9_%s", key), "", 0.0, 0.30, 1.0, 1.0);
    TPad *down = new TPad(Form("down_v9_%s", key), "", 0.0, 0.0, 1.0, 0.30);
    up->SetLeftMargin(0.13); up->SetRightMargin(0.04); up->SetBottomMargin(0.02);
    down->SetLeftMargin(0.13); down->SetRightMargin(0.04); down->SetTopMargin(0.03); down->SetBottomMargin(0.32);
    c->cd(); up->Draw(); down->Draw();

    up->cd();
    if (logY) up->SetLogy();
    b->SetTitle(title);
    b->GetXaxis()->SetLabelSize(0.0);
    b->GetYaxis()->SetTitle(yTitle);
    b->GetYaxis()->SetTitleOffset(1.35);
    const Double_t maximum = TMath::Max(a->GetMaximum(), b->GetMaximum());
    if (logY) {
        b->SetMinimum(maximum > 0.0 ? maximum * 1.0e-6 : 1.0e-10);
        b->SetMaximum(maximum > 0.0 ? maximum * 10.0 : 1.0);
    } else {
        b->SetMinimum(0.0);
        b->SetMaximum(maximum > 0.0 ? maximum * 1.30 : 1.0);
    }
    b->Draw("E1");
    a->Draw("E1 SAME");
    TLegend *legend = new TLegend(0.57, 0.76, 0.94, 0.90);
    legend->SetBorderSize(0); legend->SetFillStyle(0);
    legend->AddEntry(a, "new", "lep");
    legend->AddEntry(b, "reference", "lep");
    legend->Draw();

    down->cd();
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
    ratio->SetMinimum(ratioMin);
    ratio->SetMaximum(ratioMax);
    ratio->Draw("E1");
    TLine line(ratio->GetXaxis()->GetXmin(), 1.0,
               ratio->GetXaxis()->GetXmax(), 1.0);
    line.SetLineStyle(2); line.Draw();

    TString printName = pdf;
    if (first) printName += "(";
    if (last) printName += ")";
    c->Print(printName.Data());
    printV9(key, a, b);
    delete c;
}

} // namespace PicoQaV9

void comparePicoDstSamples_v9(
    const char *newFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_v9",
    Bool_t runInclusiveV8 = kTRUE)
{
    using namespace PicoQaV9;

    gStyle->SetOptStat(0);

    if (runInclusiveV8) {
        const TString inclusiveBase = Form("%s_inclusive_v8", outputBase);
        printf("V9STEP inclusive-v8 output=%s\n", inclusiveBase.Data());
        comparePicoDstSamples_v8(newFileName, referenceFileName, inclusiveBase.Data());
    }

    TFile *newFile = TFile::Open(newFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!newFile || newFile->IsZombie() || !referenceFile || referenceFile->IsZombie()) {
        printf("V9ERROR cannot open one or both input files\n");
        return;
    }

    TTree *newTree = (TTree*)newFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!newTree || !referenceTree) {
        printf("V9ERROR PicoDst tree is missing in one or both files\n");
        newFile->Close(); referenceFile->Close();
        return;
    }

    SampleV9 a, b;
    initV9(a, "newv9", "new");
    initV9(b, "refv9", "reference");
    if (!fillV9(newTree, a) || !fillV9(referenceTree, b)) {
        printf("V9ERROR response comparison could not be filled\n");
        newFile->Close(); referenceFile->Close();
        return;
    }
    deriveV9(a);
    deriveV9(b);

    const TString rootName = Form("%s_response_v9.root", outputBase);
    const TString pdfName = Form("%s_response_v9.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V9ERROR cannot create output ROOT file %s\n", rootName.Data());
        newFile->Close(); referenceFile->Close();
        return;
    }

    struct PlotV9 {
        TH1D *a; TH1D *b;
        const char *key; const char *title; const char *x; const char *y;
        Bool_t logY; Double_t rMin; Double_t rMax;
    };

    std::vector<PlotV9> plots;
    PlotV9 p;
#define ADDV9(A,B,K,T,X,Y,L,R0,R1) p.a=(A);p.b=(B);p.key=(K);p.title=(T);p.x=(X);p.y=(Y);p.logY=(L);p.rMin=(R0);p.rMax=(R1);plots.push_back(p)
    ADDV9(a.hNTracks,b.hNTracks,"n_tracks_exact","Reconstructed tracks per event (exact)","N_{tracks}","events^{-1}",kTRUE,0.0,2.0);
    ADDV9(a.hNMcTracks,b.hNMcTracks,"n_mc_tracks_exact","MC tracks per event (exact)","N_{MC}","events^{-1}",kTRUE,0.0,2.0);
    ADDV9(a.hNBTofHits,b.hNBTofHits,"n_btof_hits_exact","BTOF hits per event (exact)","N_{BTOF}","events^{-1}",kTRUE,0.0,2.0);
    ADDV9(a.hNBTowHits,b.hNBTowHits,"n_btow_hits_exact","BEMC tower entries per event (exact)","N_{BTow}","events^{-1}",kTRUE,0.8,1.2);
    ADDV9(a.hMcPtShape,b.hMcPtShape,"mc_pt_shape","MC p_{T} shape (unit area)","p_{T}^{MC} (GeV/c)","fraction",kTRUE,0.0,2.5);
    ADDV9(a.hMcEtaShape,b.hMcEtaShape,"mc_eta_shape","MC #eta shape (unit area)","#eta^{MC}","fraction",kFALSE,0.5,1.5);
    ADDV9(a.hMcPhiShape,b.hMcPhiShape,"mc_phi_shape","MC #phi shape (unit area)","#phi^{MC}","fraction",kFALSE,0.5,1.5);
    ADDV9(a.hMcGePidFraction,b.hMcGePidFraction,"mc_gepid_fraction","MC GEANT-PID fractions","GEANT PID","fraction",kTRUE,0.0,3.0);
    ADDV9(a.hEffAllPt,b.hEffAllPt,"eff_all_pt","Truth-matched tracking efficiency: all charged","p_{T}^{MC} (GeV/c)","efficiency",kFALSE,0.5,1.5);
    ADDV9(a.hEffAllEta,b.hEffAllEta,"eff_all_eta","Truth-matched tracking efficiency vs #eta","#eta^{MC}","efficiency",kFALSE,0.5,1.5);
    ADDV9(a.hEffAllPhi,b.hEffAllPhi,"eff_all_phi","Truth-matched tracking efficiency vs #phi","#phi^{MC}","efficiency",kFALSE,0.5,1.5);
    ADDV9(a.hEffAllGePid,b.hEffAllGePid,"eff_all_gepid","Truth-matched tracking efficiency by GEANT PID","GEANT PID","efficiency",kFALSE,0.5,1.5);
    ADDV9(a.hEffPrimaryPt,b.hEffPrimaryPt,"eff_primary_pt","Tracking efficiency: charged particles from MC vertex 1","p_{T}^{MC} (GeV/c)","efficiency",kFALSE,0.5,1.5);
    ADDV9(a.hEffD0DaughterPt,b.hEffD0DaughterPt,"eff_d0daughter_pt","Tracking efficiency: D^{0} K/#pi daughters","p_{T}^{MC} (GeV/c)","efficiency",kFALSE,0.5,1.5);
    ADDV9(a.hPtResolution,b.hPtResolution,"matched_pt_resolution","Matched-track p_{T} response (unit area)","(p_{T}^{reco}-p_{T}^{MC})/p_{T}^{MC}","fraction",kTRUE,0.0,2.0);
    ADDV9(a.hEtaResidual,b.hEtaResidual,"matched_eta_residual","Matched-track #eta residual (unit area)","#eta^{reco}-#eta^{MC}","fraction",kTRUE,0.0,2.0);
    ADDV9(a.hPhiResidual,b.hPhiResidual,"matched_phi_residual","Matched-track #phi residual (unit area)","#phi^{reco}-#phi^{MC}","fraction",kTRUE,0.0,2.0);
    ADDV9(a.hMatchedNHitsFit,b.hMatchedNHitsFit,"matched_nhitsfit_shape","Matched-track nHitsFit (unit area)","|nHitsFit|","fraction",kFALSE,0.5,1.5);
    ADDV9(a.hMatchedQATruth,b.hMatchedQATruth,"matched_qatruth_shape","Matched-track qaTruth (unit area)","qaTruth","fraction",kTRUE,0.5,1.5);
    ADDV9(a.hND0,b.hND0,"n_d0","D^{0}/#bar{D}^{0} per written event","N_{D^{0}}","events^{-1}",kTRUE,0.0,2.0);
    ADDV9(a.hD0Pt,b.hD0Pt,"d0_pt_shape","D^{0} p_{T} shape (unit area)","p_{T}^{D^{0}} (GeV/c)","fraction",kTRUE,0.0,3.0);
    ADDV9(a.hD0DecayCompleteness,b.hD0DecayCompleteness,"d0_decay_completeness","D^{0} #rightarrow K#pi truth completeness","category","fraction",kFALSE,0.5,1.5);
    ADDV9(a.hD0KPiMass,b.hD0KPiMass,"d0_kpi_mass_shape","Forced-decay K#pi invariant mass","m_{K#pi} (GeV/c^{2})","fraction",kTRUE,0.5,1.5);
    ADDV9(a.hD0MomentumClosure,b.hD0MomentumClosure,"d0_momentum_closure_shape","Forced-decay momentum closure","|#Sigma#vec{p}_{daughter}-#vec{p}_{D^{0}}|/|#vec{p}_{D^{0}}|","fraction",kTRUE,0.0,2.0);
    if (a.hasBTofCellId && b.hasBTofCellId) {
        ADDV9(a.hBTofCellId,b.hBTofCellId,"btof_cell_id_shape","BTOF cell-ID occupancy (unit area)","cell ID","fraction",kTRUE,0.0,3.0);
    } else {
        printf("V9SKIP btof_cell_id_shape reason=missing_BTofHit_cellId_leaf\n");
    }
    ADDV9(a.hBTowEnergyOverAdc,b.hBTowEnergyOverAdc,"btow_energy_over_adc_shape","BEMC positive E/ADC response (unit area)","E/ADC (GeV/count)","fraction",kTRUE,0.0,3.0);
    ADDV9(a.hBTowEnergyPositiveFraction,b.hBTowEnergyPositiveFraction,"btow_energy_positive_fraction","Fraction of ADC>0 towers with E>0","N(E>0)/N(ADC>0)","fraction",kFALSE,0.7,1.3);
    ADDV9(a.hTowerEnergyPositiveOccupancy,b.hTowerEnergyPositiveOccupancy,"tower_energy_positive_occupancy","BEMC E>0 occupancy by tower","tower ID","events^{-1}",kFALSE,0.7,1.3);
#undef ADDV9

    printf("V9CONFIG efficiency_acceptance: charged, pT>=%.3f GeV/c, |eta|<%.3f, qaTruth>=50\n",
           kMinMcPtV9, kMaxMcEtaV9);
    printf("V9CONFIG primary_definition: McTrack.mIdVtxStart == 1 (when leaf exists)\n");
    printf("V9CONFIG new_entries=%lld reference_entries=%lld\n", a.nEvents, b.nEvents);

    for (size_t i = 0; i < plots.size(); ++i) {
        printf("V9PDF page %lu / %lu: %s\n",
               (unsigned long)(i + 1), (unsigned long)plots.size(), plots[i].key);
        drawV9(plots[i].a, plots[i].b, output, pdfName.Data(),
               plots[i].key, plots[i].title, plots[i].x, plots[i].y,
               plots[i].logY, plots[i].rMin, plots[i].rMax,
               i == 0, i + 1 == plots.size());
    }

    output->Write();
    output->Close();
    newFile->Close();
    referenceFile->Close();

    printf("V9DONE inclusive_pdf=%s_inclusive_v8.pdf\n", outputBase);
    printf("V9DONE inclusive_root=%s_inclusive_v8.root\n", outputBase);
    printf("V9DONE response_pdf=%s\n", pdfName.Data());
    printf("V9DONE response_root=%s\n", rootName.Data());
}
