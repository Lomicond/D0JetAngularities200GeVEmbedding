#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TClass.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMath.h"
#include "TString.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"

#include <cstdio>

Int_t gPdfPageV7 = 0;
const Int_t gTotalPdfPagesV7 = 26;

// ============================================================================
// Histogram container
// ============================================================================

struct SampleHistogramsV5
{
    TString prefix;
    TString label;

    Long64_t nEvents;

    TH1D *hVertexZ;

    TH1D *hNTracks;
    TH1D *hNMcTracks;
    TH1D *hNBTofHits;
    TH1D *hNBTowHits;

    TH1D *hNTruthTracks;
    TH1D *hNGoodTruthTracks;

    TH1D *hTrackPt;
    TH1D *hTrackEta;
    TH1D *hTrackNHitsFit;
    TH1D *hTrackQATruth;

    TH1D *hMcTrackPt;
    TH1D *hMcGePid;

    TH1D *hBTofIdTruth;
    TH1D *hBTofQATruth;

    TH1D *hBTowAdcAll;
    TH1D *hBTowAdcPositive;

    TH1D *hBTowEnergyAll;
    TH1D *hBTowEnergyPositive;

    TH1D *hNPositiveAdcTowers;
    TH1D *hNPositiveEnergyTowers;

    TH1D *hSumPositiveAdc;
    TH1D *hSumPositiveEnergy;

    // Internal tower accumulators.
    TH1D *hTowerAdcSum;
    TH1D *hTowerAdcCount;

    TH1D *hTowerPositiveEnergySum;
    TH1D *hTowerPositiveEnergyCount;

    TH1D *hTowerPositiveAdcCount;

    // Derived tower-by-tower quantities.
    TH1D *hTowerMeanAdc;
    TH1D *hTowerMeanPositiveEnergy;
    TH1D *hTowerPositiveAdcOccupancy;
};


// ============================================================================
// Basic helpers
// ============================================================================

TH1D *makeHistogramV5(const TString &prefix,
                      const char *key,
                      Int_t nBins,
                      Double_t minimum,
                      Double_t maximum)
{
    TString name = Form("%s_%s", prefix.Data(), key);

    TH1D *histogram =
        new TH1D(name.Data(),
                 "",
                 nBins,
                 minimum,
                 maximum);

    histogram->SetDirectory(0);
    histogram->Sumw2();

    return histogram;
}


// ----------------------------------------------------------------------------

void initializeSampleV5(SampleHistogramsV5 &sample,
                        const char *prefix,
                        const char *label)
{
    sample.prefix = prefix;
    sample.label = label;
    sample.nEvents = 0;

    sample.hVertexZ =
        makeHistogramV5(sample.prefix,
                        "vertex_z",
                        60, -6.0, 6.0);

    sample.hNTracks =
        makeHistogramV5(sample.prefix,
                        "n_tracks",
                        200, -0.5, 1999.5);

    sample.hNMcTracks =
        makeHistogramV5(sample.prefix,
                        "n_mc_tracks",
                        200, -0.5, 4999.5);

    sample.hNBTofHits =
        makeHistogramV5(sample.prefix,
                        "n_btof_hits",
                        100, -0.5, 499.5);

    sample.hNBTowHits =
        makeHistogramV5(sample.prefix,
                        "n_btow_hits",
                        97, -0.5, 4849.5);

    sample.hNTruthTracks =
        makeHistogramV5(sample.prefix,
                        "n_truth_tracks",
                        200, -0.5, 199.5);

    sample.hNGoodTruthTracks =
        makeHistogramV5(sample.prefix,
                        "n_good_truth_tracks",
                        200, -0.5, 199.5);

    sample.hTrackPt =
        makeHistogramV5(sample.prefix,
                        "track_pt",
                        100, 0.0, 20.0);

    sample.hTrackEta =
        makeHistogramV5(sample.prefix,
                        "track_eta",
                        60, -1.5, 1.5);

    sample.hTrackNHitsFit =
        makeHistogramV5(sample.prefix,
                        "track_nhitsfit",
                        50, -0.5, 49.5);

    sample.hTrackQATruth =
        makeHistogramV5(sample.prefix,
                        "track_qatruth",
                        101, -0.5, 100.5);

    sample.hMcTrackPt =
        makeHistogramV5(sample.prefix,
                        "mc_track_pt",
                        100, 0.0, 20.0);

    sample.hMcGePid =
        makeHistogramV5(sample.prefix,
                        "mc_gepid",
                        60, -0.5, 59.5);

    sample.hBTofIdTruth =
        makeHistogramV5(sample.prefix,
                        "btof_idtruth",
                        100, -0.5, 999.5);

    sample.hBTofQATruth =
        makeHistogramV5(sample.prefix,
                        "btof_qatruth",
                        11, -0.5, 10.5);

    sample.hBTowAdcAll =
        makeHistogramV5(sample.prefix,
                        "btow_adc_all",
                        200, -0.5, 4999.5);

    sample.hBTowAdcPositive =
        makeHistogramV5(sample.prefix,
                        "btow_adc_positive",
                        200, 0.0, 5000.0);

    sample.hBTowEnergyAll =
        makeHistogramV5(sample.prefix,
                        "btow_energy_all",
                        502, -0.1, 25.0);

    sample.hBTowEnergyPositive =
        makeHistogramV5(sample.prefix,
                        "btow_energy_positive",
                        500, 0.0, 25.0);

    sample.hNPositiveAdcTowers =
        makeHistogramV5(sample.prefix,
                        "n_positive_adc_towers",
                        96, -0.5, 4799.5);

    sample.hNPositiveEnergyTowers =
        makeHistogramV5(sample.prefix,
                        "n_positive_energy_towers",
                        96, -0.5, 4799.5);

    sample.hSumPositiveAdc =
        makeHistogramV5(sample.prefix,
                        "sum_positive_adc",
                        200, 0.0, 500000.0);

    sample.hSumPositiveEnergy =
        makeHistogramV5(sample.prefix,
                        "sum_positive_energy",
                        300, 0.0, 150.0);

    sample.hTowerAdcSum =
        makeHistogramV5(sample.prefix,
                        "tower_adc_sum",
                        4800, 0.5, 4800.5);

    sample.hTowerAdcCount =
        makeHistogramV5(sample.prefix,
                        "tower_adc_count",
                        4800, 0.5, 4800.5);

    sample.hTowerPositiveEnergySum =
        makeHistogramV5(sample.prefix,
                        "tower_positive_energy_sum",
                        4800, 0.5, 4800.5);

    sample.hTowerPositiveEnergyCount =
        makeHistogramV5(sample.prefix,
                        "tower_positive_energy_count",
                        4800, 0.5, 4800.5);

    sample.hTowerPositiveAdcCount =
        makeHistogramV5(sample.prefix,
                        "tower_positive_adc_count",
                        4800, 0.5, 4800.5);

    sample.hTowerMeanAdc =
        makeHistogramV5(sample.prefix,
                        "tower_mean_adc",
                        4800, 0.5, 4800.5);

    sample.hTowerMeanPositiveEnergy =
        makeHistogramV5(sample.prefix,
                        "tower_mean_positive_energy",
                        4800, 0.5, 4800.5);

    sample.hTowerPositiveAdcOccupancy =
        makeHistogramV5(sample.prefix,
                        "tower_positive_adc_occupancy",
                        4800, 0.5, 4800.5);
}


// ----------------------------------------------------------------------------

Bool_t validNumberV5(Double_t value)
{
    if (value != value) {
        return kFALSE;
    }

    if (TMath::Abs(value) > 1.0e30) {
        return kFALSE;
    }

    return kTRUE;
}


// ----------------------------------------------------------------------------

Int_t leafNDataV5(TLeaf *leaf)
{
    return leaf ? leaf->GetNdata() : 0;
}


// ----------------------------------------------------------------------------

Double_t leafValueV5(TLeaf *leaf,
                     Int_t index,
                     Double_t defaultValue)
{
    if (!leaf) {
        return defaultValue;
    }

    if (index < 0 || index >= leaf->GetNdata()) {
        return defaultValue;
    }

    const Double_t value = leaf->GetValue(index);

    return validNumberV5(value) ?
           value :
           defaultValue;
}


// ----------------------------------------------------------------------------

Int_t counterValueV5(TLeaf *counterLeaf,
                     TLeaf *fallbackArrayLeaf)
{
    if (counterLeaf) {
        const Double_t value = counterLeaf->GetValue(0);

        if (validNumberV5(value) && value >= 0.0) {
            return (Int_t)(value + 0.5);
        }
    }

    return fallbackArrayLeaf ?
           fallbackArrayLeaf->GetNdata() :
           0;
}


// ----------------------------------------------------------------------------

Int_t towerIdV5(Double_t rawId)
{
    if (!validNumberV5(rawId)) {
        return -1;
    }

    const Int_t id = (Int_t)(rawId + 0.5);

    if (id >= 1 && id <= 4800) {
        return id;
    }

    if (id >= 0 && id < 4800) {
        return id + 1;
    }

    return -1;
}


// ============================================================================
// Manual tree reading
// ============================================================================

Bool_t fillSampleV5(TTree *tree,
                    SampleHistogramsV5 &sample)
{
    if (!tree) {
        return kFALSE;
    }

    // Event leaves.
    TLeaf *eventVertexZ =
        tree->GetLeaf("Event.mPrimaryVertexZ");

    // Collection counters.
    TLeaf *trackCounter =
        tree->GetLeaf("Track_");

    TLeaf *mcTrackCounter =
        tree->GetLeaf("McTrack_");

    TLeaf *btofCounter =
        tree->GetLeaf("BTofHit_");

    TLeaf *btowCounter =
        tree->GetLeaf("BTowHit_");

    // Reconstructed tracks.
    TLeaf *trackPx =
        tree->GetLeaf("Track.mGMomentumX");

    TLeaf *trackPy =
        tree->GetLeaf("Track.mGMomentumY");

    TLeaf *trackPz =
        tree->GetLeaf("Track.mGMomentumZ");

    TLeaf *trackNHitsFit =
        tree->GetLeaf("Track.mNHitsFit");

    TLeaf *trackIdTruth =
        tree->GetLeaf("Track.mIdTruth");

    TLeaf *trackQATruth =
        tree->GetLeaf("Track.mQATruth");

    // MC tracks.
    TLeaf *mcPx =
        tree->GetLeaf("McTrack.mPx");

    TLeaf *mcPy =
        tree->GetLeaf("McTrack.mPy");

    TLeaf *mcGePid =
        tree->GetLeaf("McTrack.mGePid");

    // BTOF truth.
    TLeaf *btofIdTruth =
        tree->GetLeaf("BTofHit.mIdTruth");

    TLeaf *btofQATruth =
        tree->GetLeaf("BTofHit.mQATruth");

    // BEMC towers.
    TLeaf *btowId =
        tree->GetLeaf("BTowHit.mId");

    TLeaf *btowAdc =
        tree->GetLeaf("BTowHit.mAdc");

    TLeaf *btowEnergy =
        tree->GetLeaf("BTowHit.mE");

    const Long64_t nEvents = tree->GetEntries();

    sample.nEvents = nEvents;

    printf("Reading %-12s: %lld events\n",
           sample.label.Data(),
           nEvents);

    for (Long64_t iEvent = 0;
         iEvent < nEvents;
         ++iEvent) {

        tree->GetEntry(iEvent);

        if (iEvent % 100 == 0) {
            printf("  %s event %lld / %lld\n",
                   sample.label.Data(),
                   iEvent,
                   nEvents);
        }

        // ---------------------------------------------------------------------
        // Event information
        // ---------------------------------------------------------------------

        if (eventVertexZ) {
            const Double_t vertexZ =
                eventVertexZ->GetValue(0);

            if (validNumberV5(vertexZ)) {
                sample.hVertexZ->Fill(vertexZ);
            }
        }

        // ---------------------------------------------------------------------
        // Reconstructed tracks
        // ---------------------------------------------------------------------

        const Int_t nTracks =
            counterValueV5(trackCounter,
                           trackPx ? trackPx : trackIdTruth);

        sample.hNTracks->Fill(nTracks);

        Int_t nTruthTracks = 0;
        Int_t nGoodTruthTracks = 0;

        Int_t nTrackLoop = leafNDataV5(trackPx);

        if (nTrackLoop <= 0) {
            nTrackLoop = leafNDataV5(trackIdTruth);
        }

        for (Int_t iTrack = 0;
             iTrack < nTrackLoop;
             ++iTrack) {

            if (trackPx && trackPy &&
                iTrack < trackPx->GetNdata() &&
                iTrack < trackPy->GetNdata()) {

                const Double_t px =
                    trackPx->GetValue(iTrack);

                const Double_t py =
                    trackPy->GetValue(iTrack);

                if (validNumberV5(px) &&
                    validNumberV5(py)) {

                    const Double_t pt =
                        TMath::Sqrt(px * px + py * py);

                    if (validNumberV5(pt)) {
                        sample.hTrackPt->Fill(pt);
                    }

                    if (trackPz &&
                        iTrack < trackPz->GetNdata()) {

                        const Double_t pz =
                            trackPz->GetValue(iTrack);

                        if (validNumberV5(pz) &&
                            pt > 1.0e-9) {

                            const Double_t momentum =
                                TMath::Sqrt(pt * pt +
                                            pz * pz);

                            if (momentum >
                                TMath::Abs(pz)) {

                                const Double_t eta =
                                    0.5 *
                                    TMath::Log(
                                        (momentum + pz) /
                                        (momentum - pz));

                                if (validNumberV5(eta)) {
                                    sample.hTrackEta->Fill(eta);
                                }
                            }
                        }
                    }
                }
            }

            if (trackNHitsFit &&
                iTrack < trackNHitsFit->GetNdata()) {

                const Double_t nHitsFit =
                    TMath::Abs(
                        trackNHitsFit->GetValue(iTrack));

                if (validNumberV5(nHitsFit)) {
                    sample.hTrackNHitsFit->Fill(nHitsFit);
                }
            }

            if (trackIdTruth &&
                iTrack < trackIdTruth->GetNdata()) {
                const Int_t trackTruthId =
                    (Int_t)trackIdTruth->GetValue(iTrack);

                if (trackTruthId > 0) {
                    ++nTruthTracks;

                    if (trackQATruth &&
                        iTrack < trackQATruth->GetNdata()) {

                        const Double_t trackTruthQa =
                            trackQATruth->GetValue(iTrack);

                        if (validNumberV5(trackTruthQa)) {
                            sample.hTrackQATruth->Fill(
                                trackTruthQa);

                            if (trackTruthQa >= 50.0) {
                                ++nGoodTruthTracks;
                            }
                        }
                    }
                }
            }
        }

        sample.hNTruthTracks->Fill(nTruthTracks);
        sample.hNGoodTruthTracks->Fill(nGoodTruthTracks);

        // ---------------------------------------------------------------------
        // MC tracks
        // ---------------------------------------------------------------------

        const Int_t nMcTracks =
            counterValueV5(mcTrackCounter,
                           mcPx ? mcPx : mcGePid);

        sample.hNMcTracks->Fill(nMcTracks);

        Int_t nMcLoop = leafNDataV5(mcPx);

        if (nMcLoop <= 0) {
            nMcLoop = leafNDataV5(mcGePid);
        }

        for (Int_t iMc = 0;
             iMc < nMcLoop;
             ++iMc) {

            if (mcPx && mcPy &&
                iMc < mcPx->GetNdata() &&
                iMc < mcPy->GetNdata()) {

                const Double_t px =
                    mcPx->GetValue(iMc);

                const Double_t py =
                    mcPy->GetValue(iMc);

                if (validNumberV5(px) &&
                    validNumberV5(py)) {

                    const Double_t pt =
                        TMath::Sqrt(px * px + py * py);

                    if (validNumberV5(pt)) {
                        sample.hMcTrackPt->Fill(pt);
                    }
                }
            }

            if (mcGePid &&
                iMc < mcGePid->GetNdata()) {

                const Double_t gePid =
                    mcGePid->GetValue(iMc);

                if (validNumberV5(gePid)) {
                    sample.hMcGePid->Fill(gePid);
                }
            }
        }

        // ---------------------------------------------------------------------
        // BTOF
        // ---------------------------------------------------------------------

        const Int_t nBTofHits =
            counterValueV5(btofCounter,
                           btofIdTruth);

        sample.hNBTofHits->Fill(nBTofHits);

        if (btofIdTruth) {
            const Int_t nBTofTruth =
                btofIdTruth->GetNdata();

            for (Int_t iHit = 0;
                 iHit < nBTofTruth;
                 ++iHit) {

                const Double_t btofTruthId =
                    btofIdTruth->GetValue(iHit);

                if (validNumberV5(btofTruthId)) {
                    sample.hBTofIdTruth->Fill(
                        btofTruthId);
                }

                if (btofQATruth &&
                    iHit < btofQATruth->GetNdata()) {

                    const Double_t btofTruthQa =
                        btofQATruth->GetValue(iHit);

                    if (validNumberV5(btofTruthQa)) {
                        sample.hBTofQATruth->Fill(
                            btofTruthQa);
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // BEMC
        // ---------------------------------------------------------------------

        const Int_t nBTowHits =
            counterValueV5(btowCounter,
                           btowAdc);

        sample.hNBTowHits->Fill(nBTowHits);

        Int_t nPositiveAdcTowers = 0;
        Int_t nPositiveEnergyTowers = 0;

        Double_t sumPositiveAdc = 0.0;
        Double_t sumPositiveEnergy = 0.0;

        const Int_t nBTowLoop =
            leafNDataV5(btowAdc);

        for (Int_t iTower = 0;
             iTower < nBTowLoop;
             ++iTower) {

            const Double_t adc =
                btowAdc->GetValue(iTower);

            Double_t energy = 0.0;
            Bool_t hasEnergy = kFALSE;

            if (btowEnergy &&
    iTower < btowEnergy->GetNdata()) {

    const Double_t rawEnergy =
        btowEnergy->GetValue(iTower);

    hasEnergy = validNumberV5(rawEnergy);

    if (hasEnergy) {
        // BTowHit.mE is stored in units of 1 MeV.
        // Convert the raw leaf value to GeV, as done by
        // StPicoBTowHit::energy().
        energy = rawEnergy / 1000.0;
    }
}

            Int_t towerId = -1;

            if (btowId &&
                iTower < btowId->GetNdata()) {

                towerId =
                    towerIdV5(
                        btowId->GetValue(iTower));
            }
            else if (iTower < 4800) {
                towerId = iTower + 1;
            }

            if (validNumberV5(adc)) {
                sample.hBTowAdcAll->Fill(adc);

                if (adc > 0.0) {
                    sample.hBTowAdcPositive->Fill(adc);

                    ++nPositiveAdcTowers;
                    sumPositiveAdc += adc;

                    if (towerId > 0) {
                        sample.hTowerPositiveAdcCount->Fill(
                            towerId);
                    }
                }

                if (towerId > 0) {
                    sample.hTowerAdcSum->Fill(
                        towerId, adc);

                    sample.hTowerAdcCount->Fill(
                        towerId);
                }
            }

            if (hasEnergy) {
                sample.hBTowEnergyAll->Fill(energy);

                if (energy > 0.0) {
                    sample.hBTowEnergyPositive->Fill(
                        energy);

                    ++nPositiveEnergyTowers;
                    sumPositiveEnergy += energy;

                    if (towerId > 0) {
                        sample.hTowerPositiveEnergySum->Fill(
                            towerId, energy);

                        sample.hTowerPositiveEnergyCount->Fill(
                            towerId);
                    }
                }
            }
        }

        sample.hNPositiveAdcTowers->Fill(
            nPositiveAdcTowers);

        sample.hNPositiveEnergyTowers->Fill(
            nPositiveEnergyTowers);

        sample.hSumPositiveAdc->Fill(
            sumPositiveAdc);

        sample.hSumPositiveEnergy->Fill(
            sumPositiveEnergy);
    }

    return kTRUE;
}


// ============================================================================
// Derived tower histograms
// ============================================================================

void deriveTowerHistogramsV5(SampleHistogramsV5 &sample)
{
    for (Int_t iBin = 1;
         iBin <= 4800;
         ++iBin) {

        const Double_t adcCount =
            sample.hTowerAdcCount->GetBinContent(iBin);

        const Double_t adcSum =
            sample.hTowerAdcSum->GetBinContent(iBin);

        if (adcCount > 0.0) {
            sample.hTowerMeanAdc->SetBinContent(
                iBin,
                adcSum / adcCount);
        }

        const Double_t energyCount =
            sample.hTowerPositiveEnergyCount->
                GetBinContent(iBin);

        const Double_t energySum =
            sample.hTowerPositiveEnergySum->
                GetBinContent(iBin);

        if (energyCount > 0.0) {
            sample.hTowerMeanPositiveEnergy->
                SetBinContent(
                    iBin,
                    energySum / energyCount);
        }

        const Double_t positiveAdcCount =
            sample.hTowerPositiveAdcCount->
                GetBinContent(iBin);

        if (sample.nEvents > 0) {
            const Double_t occupancy =
                positiveAdcCount /
                (Double_t)sample.nEvents;

            sample.hTowerPositiveAdcOccupancy->
                SetBinContent(
                    iBin,
                    occupancy);

            if (occupancy >= 0.0 &&
                occupancy <= 1.0) {

                const Double_t error =
                    TMath::Sqrt(
                        occupancy *
                        (1.0 - occupancy) /
                        (Double_t)sample.nEvents);

                sample.hTowerPositiveAdcOccupancy->
                    SetBinError(
                        iBin,
                        error);
            }
        }
    }
}


// ============================================================================
// Normalization
// ============================================================================

void normalizeToEventsV5(TH1D *histogram,
                         Long64_t nEvents,
                         Bool_t divideByBinWidth)
{
    if (!histogram || nEvents <= 0) {
        return;
    }

    for (Int_t iBin = 0;
         iBin <= histogram->GetNbinsX() + 1;
         ++iBin) {

        Double_t scale =
            1.0 / (Double_t)nEvents;

        if (divideByBinWidth) {
            Double_t width =
                histogram->GetXaxis()->
                    GetBinWidth(iBin);

            if (width <= 0.0) {
                width =
                    histogram->GetXaxis()->
                        GetBinWidth(1);
            }

            scale /= width;
        }

        histogram->SetBinContent(
            iBin,
            histogram->GetBinContent(iBin) *
            scale);

        histogram->SetBinError(
            iBin,
            histogram->GetBinError(iBin) *
            scale);
    }
}


// ----------------------------------------------------------------------------

void normalizeSampleV5(SampleHistogramsV5 &sample)
{
    // Event-level probability distributions.
    normalizeToEventsV5(
        sample.hVertexZ,
        sample.nEvents,
        kTRUE);

    normalizeToEventsV5(
        sample.hNTracks,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hNMcTracks,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hNBTofHits,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hNBTowHits,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hNTruthTracks,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hNGoodTruthTracks,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hNPositiveAdcTowers,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hNPositiveEnergyTowers,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hSumPositiveAdc,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hSumPositiveEnergy,
        sample.nEvents,
        kFALSE);

    // Per-event object spectra.
    normalizeToEventsV5(
        sample.hTrackPt,
        sample.nEvents,
        kTRUE);

    normalizeToEventsV5(
        sample.hTrackEta,
        sample.nEvents,
        kTRUE);

    normalizeToEventsV5(
        sample.hTrackNHitsFit,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hTrackQATruth,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hMcTrackPt,
        sample.nEvents,
        kTRUE);

    normalizeToEventsV5(
        sample.hMcGePid,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hBTofIdTruth,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hBTofQATruth,
        sample.nEvents,
        kFALSE);

    normalizeToEventsV5(
        sample.hBTowAdcAll,
        sample.nEvents,
        kTRUE);

    normalizeToEventsV5(
        sample.hBTowAdcPositive,
        sample.nEvents,
        kTRUE);

    normalizeToEventsV5(
        sample.hBTowEnergyAll,
        sample.nEvents,
        kTRUE);

    normalizeToEventsV5(
        sample.hBTowEnergyPositive,
        sample.nEvents,
        kTRUE);
}


// ============================================================================
// Plotting
// ============================================================================

Double_t minimumPositiveV5(TH1D *histogram)
{
    if (!histogram) {
        return 0.0;
    }

    Double_t result = 0.0;

    for (Int_t iBin = 1;
         iBin <= histogram->GetNbinsX();
         ++iBin) {

        const Double_t value =
            histogram->GetBinContent(iBin);

        if (value <= 0.0) {
            continue;
        }

        if (result <= 0.0 || value < result) {
            result = value;
        }
    }

    return result;
}


// ----------------------------------------------------------------------------

void printSummaryV5(const char *key,
                    TH1D *newHistogram,
                    TH1D *referenceHistogram)
{
    const Int_t nBins =
        newHistogram->GetNbinsX();

    const Double_t newYield =
        newHistogram->Integral(0, nBins + 1);

    const Double_t referenceYield =
        referenceHistogram->Integral(
            0,
            referenceHistogram->GetNbinsX() + 1);

    const Double_t yieldRatio =
        referenceYield != 0.0 ?
        newYield / referenceYield :
        0.0;

    const Double_t newMean =
        newHistogram->GetMean();

    const Double_t referenceMean =
        referenceHistogram->GetMean();

    printf(
        "QA %-30s "
        "yield new=%11.5g ref=%11.5g ratio=%9.4f  "
        "mean new=%11.5g ref=%11.5g delta=%11.5g  "
        "flow new=(%g,%g) ref=(%g,%g)\n",
        key,
        newYield,
        referenceYield,
        yieldRatio,
        newMean,
        referenceMean,
        newMean - referenceMean,
        newHistogram->GetBinContent(0),
        newHistogram->GetBinContent(nBins + 1),
        referenceHistogram->GetBinContent(0),
        referenceHistogram->GetBinContent(
            referenceHistogram->GetNbinsX() + 1));
}


// ----------------------------------------------------------------------------

void drawPairV5(TH1D *newHistogram,
                TH1D *referenceHistogram,
                TFile *outputFile,
                const char *pdfName,
                const char *key,
                const char *title,
                const char *xTitle,
                const char *yTitle,
                Bool_t logarithmicY,
                Double_t ratioMinimum,
                Double_t ratioMaximum)
{
    if (!newHistogram || !referenceHistogram) {
        return;
    }

    newHistogram->SetLineColor(kRed + 1);
    newHistogram->SetMarkerColor(kRed + 1);
    newHistogram->SetMarkerStyle(20);
    newHistogram->SetMarkerSize(0.55);
    newHistogram->SetLineWidth(2);

    referenceHistogram->SetLineColor(kBlack);
    referenceHistogram->SetMarkerColor(kBlack);
    referenceHistogram->SetMarkerStyle(24);
    referenceHistogram->SetMarkerSize(0.55);
    referenceHistogram->SetLineWidth(2);

    TH1D *ratio =
        (TH1D *)newHistogram->Clone(
            Form("ratio_%s", key));

    ratio->SetDirectory(0);
    ratio->Divide(referenceHistogram);

    outputFile->cd();

    newHistogram->Write(
        Form("new_%s", key));

    referenceHistogram->Write(
        Form("reference_%s", key));

    ratio->Write();

    TCanvas *canvas =
        new TCanvas(Form("canvas_%s", key),
                    title,
                    900,
                    800);

    TPad *upper =
        new TPad(Form("upper_%s", key),
                 "",
                 0.0, 0.30, 1.0, 1.0);

    TPad *lower =
        new TPad(Form("lower_%s", key),
                 "",
                 0.0, 0.0, 1.0, 0.30);

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

    if (logarithmicY) {
        upper->SetLogy();
    }

    referenceHistogram->SetTitle(title);

    referenceHistogram->GetXaxis()->
        SetLabelSize(0.0);

    referenceHistogram->GetYaxis()->
        SetTitle(yTitle);

    referenceHistogram->GetYaxis()->
        SetTitleOffset(1.35);

    const Double_t maximum =
        TMath::Max(
            newHistogram->GetMaximum(),
            referenceHistogram->GetMaximum());

    if (logarithmicY) {
        Double_t minimum =
            minimumPositiveV5(referenceHistogram);

        const Double_t newMinimum =
            minimumPositiveV5(newHistogram);

        if (minimum <= 0.0 ||
            (newMinimum > 0.0 &&
             newMinimum < minimum)) {
            minimum = newMinimum;
        }

        if (minimum <= 0.0) {
            minimum = 1.0e-10;
        }

        referenceHistogram->SetMinimum(
            minimum * 0.5);

        referenceHistogram->SetMaximum(
            maximum > 0.0 ?
            maximum * 10.0 :
            1.0);
    }
    else {
        referenceHistogram->SetMinimum(0.0);

        referenceHistogram->SetMaximum(
            maximum > 0.0 ?
            maximum * 1.30 :
            1.0);
    }

    referenceHistogram->Draw("E1");
    newHistogram->Draw("E1 SAME");

    TLegend *legend =
        new TLegend(0.55, 0.74, 0.94, 0.90);

    legend->SetBorderSize(0);
    legend->SetFillStyle(0);

    legend->AddEntry(
        newHistogram,
        "new embedding",
        "lep");

    legend->AddEntry(
        referenceHistogram,
        "reference embedding",
        "lep");

    legend->Draw();

    lower->cd();

    ratio->SetTitle("");

    ratio->SetLineColor(kRed + 1);
    ratio->SetMarkerColor(kRed + 1);
    ratio->SetMarkerStyle(20);
    ratio->SetMarkerSize(0.50);

    ratio->GetXaxis()->SetTitle(xTitle);
    ratio->GetXaxis()->SetTitleSize(0.11);
    ratio->GetXaxis()->SetTitleOffset(1.15);
    ratio->GetXaxis()->SetLabelSize(0.09);

    ratio->GetYaxis()->SetTitle("new / reference");
    ratio->GetYaxis()->SetTitleSize(0.09);
    ratio->GetYaxis()->SetTitleOffset(0.56);
    ratio->GetYaxis()->SetLabelSize(0.08);
    ratio->GetYaxis()->SetNdivisions(505);

    ratio->SetMinimum(ratioMinimum);
    ratio->SetMaximum(ratioMaximum);

    ratio->Draw("E1");

    const Double_t xMinimum =
        ratio->GetXaxis()->GetXmin();

    const Double_t xMaximum =
        ratio->GetXaxis()->GetXmax();

    TLine *unity =
        new TLine(xMinimum, 1.0,
                  xMaximum, 1.0);

    unity->SetLineStyle(2);
    unity->Draw();

    /*
 * ROOT 5 in batch mode does not always render nested pads before Print().
 * Force every pad and the complete canvas to be updated explicitly.
 */
upper->Modified();
upper->Update();

lower->Modified();
lower->Update();

canvas->cd();
canvas->Modified();
canvas->Update();

gSystem->ProcessEvents();

/*
 * Open the multipage PDF with the first real plot and close it with
 * the last real plot. This avoids writing the tiny dummy canvas as
 * a coloured strip or blank page.
 */
++gPdfPageV7;

TString pdfPrintName = pdfName;

if (gPdfPageV7 == 1) {
    pdfPrintName += "(";
}
else if (gPdfPageV7 == gTotalPdfPagesV7) {
    pdfPrintName += ")";
}

printf("PDF page %d / %d: %s\n",
       gPdfPageV7,
       gTotalPdfPagesV7,
       key);

canvas->Print(pdfPrintName.Data());

    printSummaryV5(
        key,
        newHistogram,
        referenceHistogram);
}


// ============================================================================
// Main
// ============================================================================

void comparePicoDstSamples_v8(
    const char *newFileName,
    const char *referenceFileName,
    const char *outputPrefix =
        "PicoDstQA_new_vs_reference_v5")
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gPdfPageV7 = 0;

    // Standard STAR dependencies.
    gROOT->Macro(
        "$STAR/StRoot/StMuDSTMaker/COMMON/macros/"
        "loadSharedLibraries.C"
    );

    // PicoDst data classes and dictionaries.
    Int_t picoStatus =
        gSystem->Load("StPicoEvent");

    if (picoStatus < 0) {
        picoStatus =
            gSystem->Load("libStPicoEvent");
    }

    printf("StPicoEvent load status = %d\n",
           picoStatus);

    printf("StPicoTrack dictionary = %p\n",
           TClass::GetClass("StPicoTrack"));

    printf("StPicoEvent dictionary = %p\n",
           TClass::GetClass("StPicoEvent"));

    printf("StPicoDst dictionary = %p\n",
           TClass::GetClass("StPicoDst"));

    if (picoStatus < 0) {
        printf("ERROR: Cannot load StPicoEvent\n");
        return;
    }

    TFile *newFile =
        TFile::Open(newFileName, "READ");

    TFile *referenceFile =
        TFile::Open(referenceFileName, "READ");

    if (!newFile || newFile->IsZombie()) {
        printf("ERROR opening new file: %s\n",
               newFileName);
        return;
    }

    if (!referenceFile ||
        referenceFile->IsZombie()) {

        printf("ERROR opening reference file: %s\n",
               referenceFileName);
        return;
    }

    TTree *newTree =
        (TTree *)newFile->Get("PicoDst");

    TTree *referenceTree =
        (TTree *)referenceFile->Get("PicoDst");

    if (!newTree || !referenceTree) {
        printf("ERROR: PicoDst tree missing\n");
        return;
    }

    printf("============================================================\n");
    printf("Manual PicoDst QA comparison\n");
    printf("============================================================\n");

    printf("New file         : %s\n",
           newFileName);

    printf("Reference file   : %s\n",
           referenceFileName);

    printf("New entries      : %lld\n",
           newTree->GetEntries());

    printf("Reference entries: %lld\n",
           referenceTree->GetEntries());

    SampleHistogramsV5 newSample;
    SampleHistogramsV5 referenceSample;

    initializeSampleV5(
        newSample,
        "new",
        "new");

    initializeSampleV5(
        referenceSample,
        "reference",
        "reference");

    if (!fillSampleV5(newTree, newSample)) {
        printf("ERROR reading new sample\n");
        return;
    }

    if (!fillSampleV5(referenceTree,
                      referenceSample)) {

        printf("ERROR reading reference sample\n");
        return;
    }

    deriveTowerHistogramsV5(newSample);
    deriveTowerHistogramsV5(referenceSample);

    normalizeSampleV5(newSample);
    normalizeSampleV5(referenceSample);

    TString pdfName =
        Form("%s.pdf", outputPrefix);

    TString rootName =
        Form("%s.root", outputPrefix);

    TFile *outputFile =
        TFile::Open(rootName.Data(),
                    "RECREATE");

    if (!outputFile ||
        outputFile->IsZombie()) {

        printf("ERROR creating output ROOT file\n");
        return;
    }



    // Event and multiplicity QA.
    drawPairV5(
        newSample.hVertexZ,
        referenceSample.hVertexZ,
        outputFile,
        pdfName.Data(),
        "vertex_z",
        "Primary vertex",
        "V_{z} [cm]",
        "1/N_{evt} dN/dV_{z}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hNTracks,
        referenceSample.hNTracks,
        outputFile,
        pdfName.Data(),
        "n_tracks",
        "PicoDst track multiplicity",
        "N_{tracks}",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hNMcTracks,
        referenceSample.hNMcTracks,
        outputFile,
        pdfName.Data(),
        "n_mc_tracks",
        "MC-track multiplicity",
        "N_{MC tracks}",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hNBTofHits,
        referenceSample.hNBTofHits,
        outputFile,
        pdfName.Data(),
        "n_btof_hits",
        "BTOF-hit multiplicity",
        "N_{BTOF hits}",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hNBTowHits,
        referenceSample.hNBTowHits,
        outputFile,
        pdfName.Data(),
        "n_btow_hits",
        "BEMC-tower multiplicity",
        "N_{BEMC towers}",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hNTruthTracks,
        referenceSample.hNTruthTracks,
        outputFile,
        pdfName.Data(),
        "n_truth_tracks",
        "Truth-matched track multiplicity",
        "N_{tracks}(idTruth>0)",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hNGoodTruthTracks,
        referenceSample.hNGoodTruthTracks,
        outputFile,
        pdfName.Data(),
        "n_good_truth_tracks",
        "Good truth-matched track multiplicity",
        "N_{tracks}(idTruth>0, qaTruth#geq50)",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    // Track QA.
    drawPairV5(
        newSample.hTrackPt,
        referenceSample.hTrackPt,
        outputFile,
        pdfName.Data(),
        "track_pt",
        "Global-track transverse momentum",
        "p_{T}^{track} [GeV/c]",
        "1/N_{evt} dN/dp_{T}",
        kTRUE,
        0.0, 2.5);

    drawPairV5(
        newSample.hTrackEta,
        referenceSample.hTrackEta,
        outputFile,
        pdfName.Data(),
        "track_eta",
        "Global-track pseudorapidity",
        "#eta_{track}",
        "1/N_{evt} dN/d#eta",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hTrackNHitsFit,
        referenceSample.hTrackNHitsFit,
        outputFile,
        pdfName.Data(),
        "track_nhitsfit",
        "Track fit hits",
        "N_{hits}^{fit}",
        "1/N_{evt} dN/dN_{hits}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hTrackQATruth,
        referenceSample.hTrackQATruth,
        outputFile,
        pdfName.Data(),
        "track_qatruth",
        "Track truth quality",
        "qaTruth",
        "1/N_{evt} dN/dqaTruth",
        kFALSE,
        0.0, 2.5);

    // BTOF truth.
    drawPairV5(
        newSample.hBTofIdTruth,
        referenceSample.hBTofIdTruth,
        outputFile,
        pdfName.Data(),
        "btof_idtruth",
        "BTOF-hit truth ID",
        "BTOF idTruth",
        "1/N_{evt} dN/didTruth",
        kTRUE,
        0.0, 2.5);

    drawPairV5(
        newSample.hBTofQATruth,
        referenceSample.hBTofQATruth,
        outputFile,
        pdfName.Data(),
        "btof_qatruth",
        "BTOF-hit truth quality",
        "BTOF qaTruth",
        "1/N_{evt} dN/dqaTruth",
        kTRUE,
        0.0, 2.5);

    // MC QA.
    drawPairV5(
        newSample.hMcTrackPt,
        referenceSample.hMcTrackPt,
        outputFile,
        pdfName.Data(),
        "mc_track_pt",
        "MC-track transverse momentum",
        "p_{T}^{MC} [GeV/c]",
        "1/N_{evt} dN/dp_{T}",
        kTRUE,
        0.0, 2.5);

    drawPairV5(
        newSample.hMcGePid,
        referenceSample.hMcGePid,
        outputFile,
        pdfName.Data(),
        "mc_gepid",
        "MC GEANT particle ID",
        "GEANT particle ID",
        "1/N_{evt} dN/dID",
        kTRUE,
        0.0, 2.5);

    // BEMC object distributions.
    drawPairV5(
        newSample.hBTowAdcAll,
        referenceSample.hBTowAdcAll,
        outputFile,
        pdfName.Data(),
        "btow_adc_all",
        "BEMC tower ADC: all towers",
        "ADC",
        "1/N_{evt} dN/dADC",
        kTRUE,
        0.0, 2.5);

    drawPairV5(
        newSample.hBTowAdcPositive,
        referenceSample.hBTowAdcPositive,
        outputFile,
        pdfName.Data(),
        "btow_adc_positive",
        "BEMC tower ADC: ADC > 0",
        "ADC",
        "1/N_{evt} dN/dADC",
        kTRUE,
        0.0, 2.5);

    drawPairV5(
        newSample.hBTowEnergyAll,
        referenceSample.hBTowEnergyAll,
        outputFile,
        pdfName.Data(),
        "btow_energy_all",
        "BEMC tower energy: all towers",
        "E_{tower} [GeV]",
        "1/N_{evt} dN/dE",
        kTRUE,
        0.0, 2.5);

    drawPairV5(
        newSample.hBTowEnergyPositive,
        referenceSample.hBTowEnergyPositive,
        outputFile,
        pdfName.Data(),
        "btow_energy_positive",
        "BEMC tower energy: E > 0",
        "E_{tower} [GeV]",
        "1/N_{evt} dN/dE",
        kTRUE,
        0.0, 2.5);

    // BEMC event-level distributions.
    drawPairV5(
        newSample.hNPositiveAdcTowers,
        referenceSample.hNPositiveAdcTowers,
        outputFile,
        pdfName.Data(),
        "n_positive_adc_towers",
        "Number of BEMC towers with ADC > 0",
        "N_{tower}(ADC>0)",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hNPositiveEnergyTowers,
        referenceSample.hNPositiveEnergyTowers,
        outputFile,
        pdfName.Data(),
        "n_positive_energy_towers",
        "Number of BEMC towers with E > 0",
        "N_{tower}(E>0)",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hSumPositiveAdc,
        referenceSample.hSumPositiveAdc,
        outputFile,
        pdfName.Data(),
        "sum_positive_adc",
        "BEMC positive ADC sum per event",
        "#Sigma ADC, ADC>0",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hSumPositiveEnergy,
        referenceSample.hSumPositiveEnergy,
        outputFile,
        pdfName.Data(),
        "sum_positive_energy",
        "BEMC positive energy sum per event",
        "#Sigma E_{tower}, E>0 [GeV]",
        "Events / N_{evt}",
        kFALSE,
        0.0, 2.5);

    // Tower-by-tower response.
    drawPairV5(
        newSample.hTowerMeanAdc,
        referenceSample.hTowerMeanAdc,
        outputFile,
        pdfName.Data(),
        "tower_mean_adc",
        "Mean ADC by BEMC tower ID",
        "BEMC tower ID",
        "#LT ADC #GT",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hTowerMeanPositiveEnergy,
        referenceSample.hTowerMeanPositiveEnergy,
        outputFile,
        pdfName.Data(),
        "tower_mean_positive_energy",
        "Mean positive energy by BEMC tower ID",
        "BEMC tower ID",
        "#LT E_{tower} #GT, E>0 [GeV]",
        kFALSE,
        0.0, 2.5);

    drawPairV5(
        newSample.hTowerPositiveAdcOccupancy,
        referenceSample.hTowerPositiveAdcOccupancy,
        outputFile,
        pdfName.Data(),
        "tower_positive_adc_occupancy",
        "BEMC positive-ADC occupancy by tower ID",
        "BEMC tower ID",
        "Fraction of events with ADC>0",
        kFALSE,
        0.0, 2.5);


    outputFile->Close();

    printf("============================================================\n");
    printf("Outputs\n");
    printf("============================================================\n");

    printf("PDF  : %s\n", pdfName.Data());
    printf("ROOT : %s\n", rootName.Data());

    newFile->Close();
    referenceFile->Close();
}
