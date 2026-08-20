#include "comparePicoDstSamples_v10.C"

#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLatex.h"
#include "TMath.h"
#include "TString.h"
#include "TStyle.h"

#include <cstdio>
#include <map>

// ==========================================================================
// comparePicoDstSamples_v11.C
//
// v11 keeps the complete v10 response comparison and adds a dedicated TPC
// PID / raw-BEMC-ADC comparison.  The new PID block is intended to expose the
// known weakness of simulated STAR dE/dx and to test exactly the exclusive
// nSigma selections used by StHIOverlayAngularities:
//
//   pion   : |nSigmaPi| < 2, |nSigmaK| > 2, |nSigmaP| > 2
//   kaon   : |nSigmaPi| > 2, |nSigmaK| < 2, |nSigmaP| > 2
//   p/pbar : |nSigmaPi| > 2, |nSigmaK| > 2, |nSigmaP| < 2,
//             with p versus pbar determined from the reconstructed charge.
//
// The signed-momentum axis is p * sign(q), where sign(q) is read from the
// sign of Track.mNHitsFit, exactly as in StPicoTrack::charge().  Direct leaf
// values Track.mNSigma* are divided by 1000, matching the user's SL16d/SL22c
// standalone-analysis reader.
//
// Tracks entering the dE/dx and PID plots must satisfy:
//   idTruth > 0, qaTruth >= 50, pT >= 0.2 GeV/c, |eta| < 1,
//   |DCA| < 3 cm (when origin and event vertex leaves exist),
//   |nHitsFit| >= 15, and |nHitsFit|/nHitsMax >= 0.52.
//
// Raw ADC tests deliberately do not use BTowHit.mE:
//   * all-ADC and positive-ADC shapes;
//   * number of ADC-positive towers and positive-ADC sum per event;
//   * mean ADC and ADC-positive occupancy versus tower ID.
//
// Outputs for OUT:
//   OUT_inclusive_v8.pdf/root  -- optional broad v8 comparison
//   OUT_response_v10.pdf/root  -- complete response comparison inherited v10
//   OUT_pid_adc_v11.pdf/root   -- new dE/dx, exclusive PID, and raw ADC QA
//
// ROOT 5.34/CINT compatible; intended for STAR SL22c.
// ==========================================================================

namespace PicoQaV11 {

const Double_t kMinRecoPtV11 = 0.2;
const Double_t kMaxRecoEtaV11 = 1.0;
const Double_t kMaxDcaV11 = 3.0;
const Int_t kMinNHitsFitV11 = 15;
const Double_t kMinNHitsRatioV11 = 0.52;
const Double_t kMinQaTruthV11 = 50.0;
const Double_t kNSigmaScaleV11 = 1000.0;

enum PidCategoryV11 {
    kUnidentifiedV11 = 0,
    kPionV11 = 1,
    kKaonV11 = 2,
    kProtonV11 = 3,
    kAntiProtonV11 = 4
};

struct SampleV11 {
    TString prefix;
    TString label;
    Long64_t nEvents;
    Long64_t nPidTracks;
    Long64_t nDedxTracks;
    Double_t totalPositiveAdcTowers;
    Double_t totalPositiveAdc;

    TH2D *hDedxVsSignedPCounts;
    TH2D *hDedxVsSignedPShape;
    TH1D *hMeanDedxVsSignedP;
    TH1D *hNSigmaPionTruePion;
    TH1D *hNSigmaKaonTrueKaon;
    TH1D *hNSigmaProtonTrueProton;

    TH2D *hPidMatrixCounts;
    TH2D *hPidMatrixFraction;
    TH1D *hPidTruthCounts;
    TH1D *hPidSelectedCounts;
    TH1D *hPidCorrectCounts;
    TH1D *hPidEfficiency;
    TH1D *hPidPurity;

    TH1D *hAdcAllShape;
    TH1D *hAdcPositiveShape;
    TH1D *hNPositiveAdcTowers;
    TH1D *hSumPositiveAdc;
    TH1D *hTowerAdcSum;
    TH1D *hTowerAdcCount;
    TH1D *hTowerPositiveAdcCount;
    TH1D *hTowerMeanAdc;
    TH1D *hTowerPositiveAdcOccupancy;

    Bool_t hasPid;
    Bool_t hasAdc;
    Bool_t appliesDcaCut;
    Bool_t appliesHitsRatioCut;
};

TH1D *hist1V11(const TString &prefix,
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

TH2D *hist2V11(const TString &prefix,
               const char *key,
               Int_t nBinsX,
               Double_t minimumX,
               Double_t maximumX,
               Int_t nBinsY,
               Double_t minimumY,
               Double_t maximumY)
{
    TH2D *h = new TH2D(Form("%s_%s", prefix.Data(), key), "",
                       nBinsX, minimumX, maximumX,
                       nBinsY, minimumY, maximumY);
    h->SetDirectory(0);
    h->Sumw2();
    return h;
}

void setPidLabelsV11(TH1D *h)
{
    if (!h) return;
    h->GetXaxis()->SetBinLabel(1, "#pi");
    h->GetXaxis()->SetBinLabel(2, "K");
    h->GetXaxis()->SetBinLabel(3, "p");
    h->GetXaxis()->SetBinLabel(4, "#bar{p}");
}

void setPidMatrixLabelsV11(TH2D *h)
{
    if (!h) return;
    h->GetXaxis()->SetBinLabel(1, "unID");
    h->GetXaxis()->SetBinLabel(2, "#pi");
    h->GetXaxis()->SetBinLabel(3, "K");
    h->GetXaxis()->SetBinLabel(4, "p");
    h->GetXaxis()->SetBinLabel(5, "#bar{p}");
    h->GetYaxis()->SetBinLabel(1, "true #pi");
    h->GetYaxis()->SetBinLabel(2, "true K");
    h->GetYaxis()->SetBinLabel(3, "true p");
    h->GetYaxis()->SetBinLabel(4, "true #bar{p}");
}

void initV11(SampleV11 &s, const char *prefix, const char *label)
{
    s.prefix = prefix;
    s.label = label;
    s.nEvents = 0;
    s.nPidTracks = 0;
    s.nDedxTracks = 0;
    s.totalPositiveAdcTowers = 0.0;
    s.totalPositiveAdc = 0.0;

    s.hDedxVsSignedPCounts = hist2V11(s.prefix, "dedx_vs_signed_p_counts",
                                      240, -6.0, 6.0, 240, 0.0, 24.0);
    s.hDedxVsSignedPShape = hist2V11(s.prefix, "dedx_vs_signed_p_shape",
                                     240, -6.0, 6.0, 240, 0.0, 24.0);
    s.hMeanDedxVsSignedP = hist1V11(s.prefix, "mean_dedx_vs_signed_p",
                                    120, -6.0, 6.0);
    s.hNSigmaPionTruePion = hist1V11(s.prefix, "nsigma_pion_true_pion",
                                     160, -8.0, 8.0);
    s.hNSigmaKaonTrueKaon = hist1V11(s.prefix, "nsigma_kaon_true_kaon",
                                     160, -8.0, 8.0);
    s.hNSigmaProtonTrueProton = hist1V11(s.prefix, "nsigma_proton_true_proton",
                                         160, -8.0, 8.0);

    s.hPidMatrixCounts = hist2V11(s.prefix, "exclusive_pid_matrix_counts",
                                  5, -0.5, 4.5, 4, 0.5, 4.5);
    s.hPidMatrixFraction = hist2V11(s.prefix, "exclusive_pid_matrix_fraction",
                                    5, -0.5, 4.5, 4, 0.5, 4.5);
    setPidMatrixLabelsV11(s.hPidMatrixCounts);
    setPidMatrixLabelsV11(s.hPidMatrixFraction);

    s.hPidTruthCounts = hist1V11(s.prefix, "exclusive_pid_truth_counts",
                                 4, 0.5, 4.5);
    s.hPidSelectedCounts = hist1V11(s.prefix, "exclusive_pid_selected_counts",
                                    4, 0.5, 4.5);
    s.hPidCorrectCounts = hist1V11(s.prefix, "exclusive_pid_correct_counts",
                                   4, 0.5, 4.5);
    s.hPidEfficiency = hist1V11(s.prefix, "exclusive_pid_efficiency",
                                4, 0.5, 4.5);
    s.hPidPurity = hist1V11(s.prefix, "exclusive_pid_purity",
                            4, 0.5, 4.5);
    setPidLabelsV11(s.hPidTruthCounts);
    setPidLabelsV11(s.hPidSelectedCounts);
    setPidLabelsV11(s.hPidCorrectCounts);
    setPidLabelsV11(s.hPidEfficiency);
    setPidLabelsV11(s.hPidPurity);

    s.hAdcAllShape = hist1V11(s.prefix, "btow_adc_all_shape",
                              200, -0.5, 4999.5);
    s.hAdcPositiveShape = hist1V11(s.prefix, "btow_adc_positive_shape",
                                   200, 0.0, 5000.0);
    s.hNPositiveAdcTowers = hist1V11(s.prefix, "n_positive_adc_towers",
                                     4801, -0.5, 4800.5);
    s.hSumPositiveAdc = hist1V11(s.prefix, "sum_positive_adc",
                                 200, 0.0, 500000.0);
    s.hTowerAdcSum = hist1V11(s.prefix, "tower_adc_sum",
                              4800, 0.5, 4800.5);
    s.hTowerAdcCount = hist1V11(s.prefix, "tower_adc_count",
                                4800, 0.5, 4800.5);
    s.hTowerPositiveAdcCount = hist1V11(s.prefix, "tower_positive_adc_count",
                                        4800, 0.5, 4800.5);
    s.hTowerMeanAdc = hist1V11(s.prefix, "tower_mean_adc",
                               4800, 0.5, 4800.5);
    s.hTowerPositiveAdcOccupancy = hist1V11(s.prefix, "tower_positive_adc_occupancy",
                                            4800, 0.5, 4800.5);

    s.hasPid = kFALSE;
    s.hasAdc = kFALSE;
    s.appliesDcaCut = kFALSE;
    s.appliesHitsRatioCut = kFALSE;
}

Int_t truthPidV11(Int_t gePid, Double_t mcCharge)
{
    if (gePid == 8 || gePid == 9) return kPionV11;
    if (gePid == 11 || gePid == 12) return kKaonV11;
    if (gePid == 14 || gePid == 15) {
        if (TMath::Abs(mcCharge) > 0.01) {
            return mcCharge > 0.0 ? kProtonV11 : kAntiProtonV11;
        }
        return gePid == 14 ? kProtonV11 : kAntiProtonV11;
    }
    return kUnidentifiedV11;
}

Int_t selectedPidV11(Double_t nSigmaPion,
                     Double_t nSigmaKaon,
                     Double_t nSigmaProton,
                     Int_t charge)
{
    if (TMath::Abs(nSigmaPion) < 2.0 &&
        TMath::Abs(nSigmaKaon) > 2.0 &&
        TMath::Abs(nSigmaProton) > 2.0) {
        return kPionV11;
    }
    if (TMath::Abs(nSigmaPion) > 2.0 &&
        TMath::Abs(nSigmaKaon) < 2.0 &&
        TMath::Abs(nSigmaProton) > 2.0) {
        return kKaonV11;
    }
    if (TMath::Abs(nSigmaPion) > 2.0 &&
        TMath::Abs(nSigmaKaon) > 2.0 &&
        TMath::Abs(nSigmaProton) < 2.0) {
        return charge > 0 ? kProtonV11 : kAntiProtonV11;
    }
    return kUnidentifiedV11;
}

void reportLeafV11(const char *sample, const char *logicalName, TLeaf *leaf)
{
    printf("V11LEAF sample=%-10s %-26s : %s\n",
           sample, logicalName, leaf ? leaf->GetName() : "MISSING");
}

Bool_t fillV11(TTree *tree, SampleV11 &s)
{
    using namespace PicoQaV9;

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
    TLeaf *trackNSigmaPion = leafV9(tree, "Track.mNSigmaPion");
    TLeaf *trackNSigmaKaon = leafV9(tree, "Track.mNSigmaKaon");
    TLeaf *trackNSigmaProton = leafV9(tree, "Track.mNSigmaProton");
    TLeaf *trackIdTruth = leafV9(tree, "Track.mIdTruth");
    TLeaf *trackQATruth = leafV9(tree, "Track.mQATruth");

    TLeaf *mcId = leafV9(tree, "McTrack.mId");
    TLeaf *mcGePid = leafV9(tree, "McTrack.mGePid", "McTrack.mGeantId");
    TLeaf *mcCharge = leafV9(tree, "McTrack.mCharge");

    TLeaf *btowId = leafV9(tree, "BTowHit.mId");
    TLeaf *btowAdc = leafV9(tree, "BTowHit.mAdc");

    reportLeafV11(s.label.Data(), "Track momentum", trackPx);
    reportLeafV11(s.label.Data(), "Track dE/dx", trackDedx);
    reportLeafV11(s.label.Data(), "Track nSigma pion", trackNSigmaPion);
    reportLeafV11(s.label.Data(), "Track nSigma kaon", trackNSigmaKaon);
    reportLeafV11(s.label.Data(), "Track nSigma proton", trackNSigmaProton);
    reportLeafV11(s.label.Data(), "Track signed nHitsFit", trackNHitsFit);
    reportLeafV11(s.label.Data(), "Track nHitsMax", trackNHitsMax);
    reportLeafV11(s.label.Data(), "Track origin", trackOriginX);
    reportLeafV11(s.label.Data(), "Event primary vertex", eventVx);
    reportLeafV11(s.label.Data(), "Track idTruth", trackIdTruth);
    reportLeafV11(s.label.Data(), "Track qaTruth", trackQATruth);
    reportLeafV11(s.label.Data(), "McTrack GEANT PID", mcGePid);
    reportLeafV11(s.label.Data(), "BTow ADC", btowAdc);

    s.hasPid = trackPx && trackPy && trackPz && trackNHitsFit &&
               trackDedx && trackNSigmaPion && trackNSigmaKaon &&
               trackNSigmaProton && trackIdTruth && mcGePid;
    s.hasAdc = btowAdc != 0;
    s.appliesDcaCut = eventVx && eventVy && eventVz &&
                      trackOriginX && trackOriginY && trackOriginZ;
    s.appliesHitsRatioCut = trackNHitsMax != 0;

    if (!s.hasPid) {
        printf("V11SKIP sample=%s PID reason=missing_essential_leaf\n",
               s.label.Data());
    }
    if (!s.hasAdc) {
        printf("V11SKIP sample=%s ADC reason=missing_BTowHit_mAdc\n",
               s.label.Data());
    }

    s.nEvents = tree->GetEntries();
    printf("V11READ sample=%s events=%lld dca_cut=%d hits_ratio_cut=%d\n",
           s.label.Data(), s.nEvents,
           (Int_t)s.appliesDcaCut, (Int_t)s.appliesHitsRatioCut);

    for (Long64_t iEvent = 0; iEvent < s.nEvents; ++iEvent) {
        tree->GetEntry(iEvent);
        if (iEvent % 100 == 0) {
            printf("  V11 %s event %lld / %lld\n",
                   s.label.Data(), iEvent, s.nEvents);
        }

        if (s.hasPid) {
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
                const Int_t idTruth = (Int_t)(valueV9(trackIdTruth, iTrack) + 0.5);
                const Double_t qaTruth = trackQATruth ?
                    valueV9(trackQATruth, iTrack) : 100.0;
                if (idTruth <= 0 || qaTruth < kMinQaTruthV11) continue;

                std::map<Int_t, Int_t>::const_iterator found = mcIndex.find(idTruth);
                if (found == mcIndex.end()) continue;

                const Double_t px = valueV9(trackPx, iTrack);
                const Double_t py = valueV9(trackPy, iTrack);
                const Double_t pz = valueV9(trackPz, iTrack);
                const Double_t pt = TMath::Sqrt(px * px + py * py);
                const Double_t p = TMath::Sqrt(pt * pt + pz * pz);
                const Double_t eta = etaV9(px, py, pz);
                const Double_t signedNHitsFit = valueV9(trackNHitsFit, iTrack);
                const Double_t nHitsFit = TMath::Abs(signedNHitsFit);
                const Int_t charge = signedNHitsFit > 0.0 ? 1 : -1;

                if (pt < kMinRecoPtV11 || !finiteV9(eta) ||
                    TMath::Abs(eta) >= kMaxRecoEtaV11) continue;
                if (nHitsFit < kMinNHitsFitV11) continue;

                if (s.appliesHitsRatioCut) {
                    const Double_t nHitsMax = valueV9(trackNHitsMax, iTrack);
                    if (nHitsMax <= 0.0 || nHitsFit / nHitsMax < kMinNHitsRatioV11) continue;
                }

                if (s.appliesDcaCut) {
                    const Double_t dx = valueV9(trackOriginX, iTrack) - vertexX;
                    const Double_t dy = valueV9(trackOriginY, iTrack) - vertexY;
                    const Double_t dz = valueV9(trackOriginZ, iTrack) - vertexZ;
                    const Double_t dca = TMath::Sqrt(dx * dx + dy * dy + dz * dz);
                    if (dca >= kMaxDcaV11) continue;
                }

                const Double_t dedx = valueV9(trackDedx, iTrack, -999.0);
                const Double_t nSigmaPion =
                    valueV9(trackNSigmaPion, iTrack, 999999.0) / kNSigmaScaleV11;
                const Double_t nSigmaKaon =
                    valueV9(trackNSigmaKaon, iTrack, 999999.0) / kNSigmaScaleV11;
                const Double_t nSigmaProton =
                    valueV9(trackNSigmaProton, iTrack, 999999.0) / kNSigmaScaleV11;

                if (!finiteV9(nSigmaPion) || !finiteV9(nSigmaKaon) ||
                    !finiteV9(nSigmaProton)) continue;
                if (!finiteV9(p)) continue;

                ++s.nPidTracks;
                if (finiteV9(dedx) && dedx > 0.0) {
                    ++s.nDedxTracks;
                    const Double_t signedP = p * (Double_t)charge;
                    s.hDedxVsSignedPCounts->Fill(signedP, dedx);
                    s.hDedxVsSignedPShape->Fill(signedP, dedx);
                }

                const Int_t iMc = found->second;
                const Int_t gePid = (Int_t)(valueV9(mcGePid, iMc) + 0.5);
                const Double_t truthCharge = mcCharge ?
                    valueV9(mcCharge, iMc) : 0.0;
                const Int_t truthPid = truthPidV11(gePid, truthCharge);
                const Int_t selectedPid = selectedPidV11(
                    nSigmaPion, nSigmaKaon, nSigmaProton, charge);

                if (truthPid == kPionV11) {
                    s.hNSigmaPionTruePion->Fill(nSigmaPion);
                }
                else if (truthPid == kKaonV11) {
                    s.hNSigmaKaonTrueKaon->Fill(nSigmaKaon);
                }
                else if (truthPid == kProtonV11 || truthPid == kAntiProtonV11) {
                    s.hNSigmaProtonTrueProton->Fill(nSigmaProton);
                }

                if (truthPid != kUnidentifiedV11) {
                    s.hPidTruthCounts->Fill(truthPid);
                    s.hPidMatrixCounts->Fill(selectedPid, truthPid);
                }
                if (selectedPid != kUnidentifiedV11) {
                    s.hPidSelectedCounts->Fill(selectedPid);
                }
                if (truthPid != kUnidentifiedV11 && selectedPid == truthPid) {
                    s.hPidCorrectCounts->Fill(truthPid);
                }
            }
        }

        if (s.hasAdc) {
            const Int_t nBTow = btowAdc->GetNdata();
            Int_t nPositiveAdc = 0;
            Double_t sumPositiveAdc = 0.0;

            for (Int_t iTower = 0; iTower < nBTow; ++iTower) {
                const Double_t adc = valueV9(btowAdc, iTower, -999999.0);
                if (!finiteV9(adc)) continue;

                const Int_t towerId = btowId ?
                    towerIdV5(valueV9(btowId, iTower)) : iTower + 1;
                s.hAdcAllShape->Fill(adc);

                if (towerId >= 1 && towerId <= 4800) {
                    s.hTowerAdcSum->Fill(towerId, adc);
                    s.hTowerAdcCount->Fill(towerId);
                }

                if (adc > 0.0) {
                    ++nPositiveAdc;
                    sumPositiveAdc += adc;
                    s.hAdcPositiveShape->Fill(adc);
                    if (towerId >= 1 && towerId <= 4800) {
                        s.hTowerPositiveAdcCount->Fill(towerId);
                    }
                }
            }

            s.hNPositiveAdcTowers->Fill(nPositiveAdc);
            s.hSumPositiveAdc->Fill(sumPositiveAdc);
            s.totalPositiveAdcTowers += nPositiveAdc;
            s.totalPositiveAdc += sumPositiveAdc;
        }
    }

    return kTRUE;
}

void unitAreaV11(TH1 *h)
{
    if (!h) return;
    const Double_t integral = h->Integral();
    if (integral > 0.0) h->Scale(1.0 / integral);
}

void unitArea2DV11(TH2D *h)
{
    if (!h) return;
    const Double_t integral = h->Integral();
    if (integral > 0.0) h->Scale(1.0 / integral);
}

void binomialBinV11(TH1D *out,
                    Int_t iBin,
                    Double_t numerator,
                    Double_t denominator)
{
    if (!out) return;
    const Double_t value = denominator > 0.0 ? numerator / denominator : 0.0;
    out->SetBinContent(iBin, value);
    out->SetBinError(iBin,
        denominator > 0.0 ?
        TMath::Sqrt(value * (1.0 - value) / denominator) : 0.0);
}

void deriveMeanDedxV11(SampleV11 &s)
{
    for (Int_t iX = 1; iX <= s.hDedxVsSignedPCounts->GetNbinsX(); ++iX) {
        Double_t sum = 0.0;
        Double_t sum2 = 0.0;
        Double_t count = 0.0;
        for (Int_t iY = 1; iY <= s.hDedxVsSignedPCounts->GetNbinsY(); ++iY) {
            const Double_t weight = s.hDedxVsSignedPCounts->GetBinContent(iX, iY);
            const Double_t dedx = s.hDedxVsSignedPCounts->GetYaxis()->GetBinCenter(iY);
            sum += weight * dedx;
            sum2 += weight * dedx * dedx;
            count += weight;
        }
        if (count > 0.0) {
            const Double_t mean = sum / count;
            Double_t variance = sum2 / count - mean * mean;
            if (variance < 0.0) variance = 0.0;
            s.hMeanDedxVsSignedP->SetBinContent(iX, mean);
            s.hMeanDedxVsSignedP->SetBinError(iX,
                TMath::Sqrt(variance / count));
        }
    }
}

void deriveV11(SampleV11 &s)
{
    deriveMeanDedxV11(s);
    unitArea2DV11(s.hDedxVsSignedPShape);
    unitAreaV11(s.hNSigmaPionTruePion);
    unitAreaV11(s.hNSigmaKaonTrueKaon);
    unitAreaV11(s.hNSigmaProtonTrueProton);
    unitAreaV11(s.hAdcAllShape);
    unitAreaV11(s.hAdcPositiveShape);
    unitAreaV11(s.hNPositiveAdcTowers);
    unitAreaV11(s.hSumPositiveAdc);

    for (Int_t iPid = 1; iPid <= 4; ++iPid) {
        const Double_t truth = s.hPidTruthCounts->GetBinContent(iPid);
        const Double_t selected = s.hPidSelectedCounts->GetBinContent(iPid);
        const Double_t correct = s.hPidCorrectCounts->GetBinContent(iPid);
        binomialBinV11(s.hPidEfficiency, iPid, correct, truth);
        binomialBinV11(s.hPidPurity, iPid, correct, selected);

        for (Int_t iSelected = 1; iSelected <= 5; ++iSelected) {
            const Double_t count = s.hPidMatrixCounts->GetBinContent(iSelected, iPid);
            s.hPidMatrixFraction->SetBinContent(
                iSelected, iPid, truth > 0.0 ? count / truth : 0.0);
        }
    }

    for (Int_t iTower = 1; iTower <= 4800; ++iTower) {
        const Double_t adcCount = s.hTowerAdcCount->GetBinContent(iTower);
        const Double_t adcSum = s.hTowerAdcSum->GetBinContent(iTower);
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
}

void printPidV11(const SampleV11 &s)
{
    const char *names[4] = {"pion", "kaon", "proton", "antiproton"};
    for (Int_t iPid = 1; iPid <= 4; ++iPid) {
        printf("V11PID sample=%-10s species=%-10s truth=%8.0f selected=%8.0f correct=%8.0f efficiency=%8.5f purity=%8.5f\n",
               s.label.Data(), names[iPid - 1],
               s.hPidTruthCounts->GetBinContent(iPid),
               s.hPidSelectedCounts->GetBinContent(iPid),
               s.hPidCorrectCounts->GetBinContent(iPid),
               s.hPidEfficiency->GetBinContent(iPid),
               s.hPidPurity->GetBinContent(iPid));
    }
}

void printCanvasV11(TCanvas *canvas,
                    const char *pdf,
                    Int_t page,
                    Int_t totalPages)
{
    TString printName = pdf;
    if (page == 1) printName += "(";
    if (page == totalPages) printName += ")";
    canvas->Print(printName.Data());
}

void drawPage1DV11(TH1D *a,
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
    printf("V11PDF page %d / %d: %s\n", page, totalPages, key);
    PicoQaV9::drawV9(a, b, output, pdf, key, title, xTitle, yTitle,
                     logY, ratioMin, ratioMax,
                     page == 1, page == totalPages);
}

void drawDedxMapsV11(SampleV11 &a,
                     SampleV11 &b,
                     TFile *output,
                     const char *pdf,
                     Int_t &page,
                     Int_t totalPages)
{
    ++page;
    printf("V11PDF page %d / %d: dedx_vs_signed_p\n", page, totalPages);

    output->cd();
    a.hDedxVsSignedPCounts->Write("new_dedx_vs_signed_p_counts");
    b.hDedxVsSignedPCounts->Write("reference_dedx_vs_signed_p_counts");
    a.hDedxVsSignedPShape->Write("new_dedx_vs_signed_p_shape");
    b.hDedxVsSignedPShape->Write("reference_dedx_vs_signed_p_shape");

    const Double_t maximum = TMath::Max(a.hDedxVsSignedPShape->GetMaximum(),
                                        b.hDedxVsSignedPShape->GetMaximum());
    const Double_t minimum = maximum > 0.0 ? maximum * 1.0e-5 : 1.0e-12;

    TCanvas *canvas = new TCanvas("canvas_v11_dedx_maps",
                                  "TPC dE/dx versus signed momentum", 1600, 720);
    canvas->Divide(2, 1);
    TH2D *maps[2] = {a.hDedxVsSignedPShape, b.hDedxVsSignedPShape};
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

void drawDedxRatioV11(SampleV11 &a,
                      SampleV11 &b,
                      TFile *output,
                      const char *pdf,
                      Int_t &page,
                      Int_t totalPages)
{
    ++page;
    printf("V11PDF page %d / %d: dedx_vs_signed_p_ratio\n", page, totalPages);

    TH2D *ratio = (TH2D*)a.hDedxVsSignedPShape->Clone(
        "ratio_v11_dedx_vs_signed_p_shape");
    ratio->SetDirectory(0);
    for (Int_t iX = 1; iX <= ratio->GetNbinsX(); ++iX) {
        for (Int_t iY = 1; iY <= ratio->GetNbinsY(); ++iY) {
            const Double_t numerator = a.hDedxVsSignedPShape->GetBinContent(iX, iY);
            const Double_t denominator = b.hDedxVsSignedPShape->GetBinContent(iX, iY);
            ratio->SetBinContent(iX, iY,
                denominator > 0.0 ? numerator / denominator : 0.0);
            ratio->SetBinError(iX, iY, 0.0);
        }
    }
    output->cd();
    ratio->Write();

    TCanvas *canvas = new TCanvas("canvas_v11_dedx_ratio",
                                  "TPC dE/dx ratio", 1000, 760);
    canvas->SetLeftMargin(0.11);
    canvas->SetRightMargin(0.15);
    canvas->SetBottomMargin(0.12);
    ratio->SetTitle("TPC dE/dx shape ratio: new / reference");
    ratio->GetXaxis()->SetTitle("p #times sign(q) (GeV/c)");
    ratio->GetYaxis()->SetTitle("dE/dx (keV/cm)");
    ratio->GetZaxis()->SetTitle("new / reference");
    ratio->SetMinimum(0.0);
    ratio->SetMaximum(2.0);
    ratio->Draw("COLZ");

    TLatex note;
    note.SetNDC();
    note.SetTextSize(0.028);
    note.DrawLatex(0.13, 0.94, "Only bins populated in the reference are divided; sparse bins should be ignored.");

    printCanvasV11(canvas, pdf, page, totalPages);
    delete canvas;
    delete ratio;
}

void drawPidMatricesV11(SampleV11 &a,
                        SampleV11 &b,
                        TFile *output,
                        const char *pdf,
                        Int_t &page,
                        Int_t totalPages)
{
    ++page;
    printf("V11PDF page %d / %d: exclusive_pid_truth_matrix\n",
           page, totalPages);

    output->cd();
    a.hPidMatrixCounts->Write("new_exclusive_pid_matrix_counts");
    b.hPidMatrixCounts->Write("reference_exclusive_pid_matrix_counts");
    a.hPidMatrixFraction->Write("new_exclusive_pid_matrix_fraction");
    b.hPidMatrixFraction->Write("reference_exclusive_pid_matrix_fraction");

    TCanvas *canvas = new TCanvas("canvas_v11_pid_matrices",
                                  "Exclusive TPC PID truth matrices", 1600, 720);
    canvas->Divide(2, 1);
    TH2D *matrices[2] = {a.hPidMatrixFraction, b.hPidMatrixFraction};
    const char *titles[2] = {"new", "reference"};
    for (Int_t i = 0; i < 2; ++i) {
        TPad *pad = (TPad*)canvas->cd(i + 1);
        pad->SetLeftMargin(0.15);
        pad->SetRightMargin(0.16);
        pad->SetBottomMargin(0.15);
        matrices[i]->SetTitle(Form("%s: exclusive PID result / true species", titles[i]));
        matrices[i]->GetXaxis()->SetTitle("exclusive TPC PID result");
        matrices[i]->GetYaxis()->SetTitle("MC truth");
        matrices[i]->GetZaxis()->SetTitle("row fraction");
        matrices[i]->SetMinimum(0.0);
        matrices[i]->SetMaximum(1.0);
        matrices[i]->SetMarkerSize(1.35);
        matrices[i]->Draw("COLZ TEXT");
    }
    printCanvasV11(canvas, pdf, page, totalPages);
    delete canvas;
}

} // namespace PicoQaV11

void comparePicoDstSamples_v11(
    const char *newFileName,
    const char *referenceFileName,
    const char *outputBase = "PicoDstQA_v11",
    Bool_t runInclusiveV8 = kTRUE)
{
    using namespace PicoQaV11;

    gStyle->SetOptStat(0);
    gStyle->SetPaintTextFormat(".3f");

    // Preserve every response test from v10.  runInclusiveV8 controls only
    // the broad v8 block, just as it did in v10.
    printf("V11STEP inherited_response_v10\n");
    comparePicoDstSamples_v10(newFileName, referenceFileName,
                              outputBase, runInclusiveV8);

    TFile *newFile = TFile::Open(newFileName, "READ");
    TFile *referenceFile = TFile::Open(referenceFileName, "READ");
    if (!newFile || newFile->IsZombie() ||
        !referenceFile || referenceFile->IsZombie()) {
        printf("V11ERROR cannot open one or both input files\n");
        return;
    }

    TTree *newTree = (TTree*)newFile->Get("PicoDst");
    TTree *referenceTree = (TTree*)referenceFile->Get("PicoDst");
    if (!newTree || !referenceTree) {
        printf("V11ERROR PicoDst tree is missing in one or both files\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    SampleV11 a, b;
    initV11(a, "newv11", "new");
    initV11(b, "refv11", "reference");
    if (!fillV11(newTree, a) || !fillV11(referenceTree, b)) {
        printf("V11ERROR PID/ADC comparison could not be filled\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }
    deriveV11(a);
    deriveV11(b);

    const Bool_t drawPid = a.hasPid && b.hasPid;
    const Bool_t drawAdc = a.hasAdc && b.hasAdc;
    const Int_t pidPages = drawPid ? 9 : 0;
    const Int_t adcPages = drawAdc ? 6 : 0;
    const Int_t totalPages = pidPages + adcPages;

    printf("V11CONFIG signed_momentum=p*sign(q), q_from=sign(Track.mNHitsFit)\n");
    printf("V11CONFIG nsigma_leaf_scale=%.0f exclusive_cut=2.0\n",
           kNSigmaScaleV11);
    printf("V11CONFIG track_selection=idTruth>0,qaTruth>=%.0f,pT>=%.3f,absEta<%.3f,DCA<%.2f,nHitsFit>=%d,hitsRatio>=%.2f\n",
           kMinQaTruthV11, kMinRecoPtV11, kMaxRecoEtaV11,
           kMaxDcaV11, kMinNHitsFitV11, kMinNHitsRatioV11);
    printf("V11CONFIG selected_pid_tracks new=%lld reference=%lld\n",
           a.nPidTracks, b.nPidTracks);
    printf("V11CONFIG selected_dedx_tracks new=%lld reference=%lld\n",
           a.nDedxTracks, b.nDedxTracks);
    printf("V11CONFIG pages pid=%d adc=%d total=%d\n",
           pidPages, adcPages, totalPages);

    if (totalPages <= 0) {
        printf("V11ERROR neither common PID nor common ADC leaves are available\n");
        newFile->Close();
        referenceFile->Close();
        return;
    }

    const TString rootName = Form("%s_pid_adc_v11.root", outputBase);
    const TString pdfName = Form("%s_pid_adc_v11.pdf", outputBase);
    TFile *output = TFile::Open(rootName.Data(), "RECREATE");
    if (!output || output->IsZombie()) {
        printf("V11ERROR cannot create output ROOT file %s\n",
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
        drawPage1DV11(a.hNSigmaProtonTrueProton, b.hNSigmaProtonTrueProton,
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
        printf("V11SKIP PID pages reason=missing_common_PID_leaves\n");
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

        printf("V11ADC adc_positive_mean new=%g reference=%g ratio=%g\n",
               a.hAdcPositiveShape->GetMean(),
               b.hAdcPositiveShape->GetMean(),
               b.hAdcPositiveShape->GetMean() != 0.0 ?
               a.hAdcPositiveShape->GetMean() /
               b.hAdcPositiveShape->GetMean() : 0.0);
        const Double_t meanPositiveTowersA = a.nEvents > 0 ?
            a.totalPositiveAdcTowers / (Double_t)a.nEvents : 0.0;
        const Double_t meanPositiveTowersB = b.nEvents > 0 ?
            b.totalPositiveAdcTowers / (Double_t)b.nEvents : 0.0;
        const Double_t meanPositiveSumA = a.nEvents > 0 ?
            a.totalPositiveAdc / (Double_t)a.nEvents : 0.0;
        const Double_t meanPositiveSumB = b.nEvents > 0 ?
            b.totalPositiveAdc / (Double_t)b.nEvents : 0.0;
        printf("V11ADC positive_towers_per_event_mean new=%g reference=%g ratio=%g\n",
               meanPositiveTowersA, meanPositiveTowersB,
               meanPositiveTowersB != 0.0 ?
               meanPositiveTowersA / meanPositiveTowersB : 0.0);
        printf("V11ADC positive_adc_sum_per_event_mean new=%g reference=%g ratio=%g\n",
               meanPositiveSumA, meanPositiveSumB,
               meanPositiveSumB != 0.0 ?
               meanPositiveSumA / meanPositiveSumB : 0.0);
    }
    else {
        printf("V11SKIP ADC pages reason=missing_common_BTowHit_mAdc\n");
    }

    output->Write();
    output->Close();
    newFile->Close();
    referenceFile->Close();

    printf("V11DONE response_pdf=%s_response_v10.pdf\n", outputBase);
    printf("V11DONE response_root=%s_response_v10.root\n", outputBase);
    printf("V11DONE pid_adc_pdf=%s\n", pdfName.Data());
    printf("V11DONE pid_adc_root=%s\n", rootName.Data());
}
