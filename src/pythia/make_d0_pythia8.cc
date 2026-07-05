#include "Pythia8/Pythia.h"

#include "TFile.h"
#include "TTree.h"

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <string>

using namespace Pythia8;

int main(int argc, char* argv[])
{
    // ------------------------------------------------------------
    // User settings
    // ------------------------------------------------------------
    const int nAcceptedTarget = (argc > 1) ? std::atoi(argv[1]) : 10000;
    const int seed            = (argc > 2) ? std::atoi(argv[2]) : 12345;

    const double eCM = 200.0;        // GeV
    const double d0PtMin = 1.0;      // GeV/c
    const double d0PtMax = 10.0;     // GeV/c
    
    const double d0YMax = 1;

    // Safety limit, because we reject events without D0 in the requested pT range.
    const long long maxTries = 200LL * nAcceptedTarget;

    // ------------------------------------------------------------
    // PYTHIA setup
    // ------------------------------------------------------------
    Pythia pythia;

    // pp at sqrt(s)=200 GeV
    pythia.readString("Beams:idA = 2212");
    pythia.readString("Beams:idB = 2212");
    pythia.readString("Beams:eCM = 200.");

    // Random seed
    pythia.readString("Random:setSeed = on");
    pythia.readString("Random:seed = " + std::to_string(seed));

    // Detroit tune.
    //
    // Option A: if your PYTHIA8 is new enough:
    // pythia.readString("Tune:pp = 33");
    //
    // Option B: robust/manual version:
    pythia.readFile("detroit.cmnd");

    // Charm-enriched production.
    // This is practical for statistics. If you need unbiased minbias later,
    // this should be changed and/or treated with proper pThat weighting.
    pythia.readString("HardQCD:all = on");

    // Important for later GEANT3:
    // If the D0 should decay inside GEANT, keep it stable in PYTHIA.
    // Then GEANT/STAR chain can force D0 -> K pi.
    pythia.readString("421:mayDecay = off");
    
    // Minimum hard-scattering pT-hat cut
    pythia.readString("PhaseSpace:pTHatMin = 3.0");

    // For debugging: print changed settings.
    pythia.settings.listChanged();

    if (!pythia.init()) {
        std::cerr << "PYTHIA initialization failed." << std::endl;
        return 1;
    }

    // ------------------------------------------------------------
    // Output ROOT file
    // ------------------------------------------------------------
    TFile* fout = new TFile("pythia8_D0_DetroitTune.root", "RECREATE");
    TTree* tree = new TTree("D0Tree", "PYTHIA8 D0 events with Detroit tune");

    int acceptedEvent = 0;
    int triedEvent = 0;
    int d0_id = 0;
    int d0_index = -1;

    double d0_pt = 0.0;
    double d0_eta = 0.0;
    double d0_y = 0.0;
    double d0_phi = 0.0;
    double d0_m = 0.0;
    double d0_e = 0.0;
   
    
    std::vector<int> part_pythiaIndex;
    std::vector<int>    part_id;
    std::vector<int>    part_status;
    std::vector<int>    part_mother1;
    std::vector<int>    part_mother2;

    std::vector<double> part_px;
    std::vector<double> part_py;
    std::vector<double> part_pz;
    std::vector<double> part_e;
    std::vector<double> part_m;
    std::vector<double> part_vx;
    std::vector<double> part_vy;
    std::vector<double> part_vz;
    std::vector<double> part_vt;


    tree->Branch("acceptedEvent", &acceptedEvent, "acceptedEvent/I");
    tree->Branch("triedEvent",    &triedEvent,    "triedEvent/I");
    tree->Branch("d0_index",      &d0_index,      "d0_index/I");
    tree->Branch("d0_id",         &d0_id,         "d0_id/I");

    tree->Branch("d0_pt",  &d0_pt,  "d0_pt/D");
    tree->Branch("d0_eta", &d0_eta, "d0_eta/D");
    tree->Branch("d0_y",   &d0_y,   "d0_y/D");
    tree->Branch("d0_phi", &d0_phi, "d0_phi/D");
    tree->Branch("d0_m",   &d0_m,   "d0_m/D");
    tree->Branch("d0_e",   &d0_e,   "d0_e/D");
    
    tree->Branch("part_pythiaIndex", &part_pythiaIndex);
	tree->Branch("part_id",      &part_id);
	tree->Branch("part_status",  &part_status);
	tree->Branch("part_mother1", &part_mother1);
	tree->Branch("part_mother2", &part_mother2);

	tree->Branch("part_px", &part_px);
	tree->Branch("part_py", &part_py);
	tree->Branch("part_pz", &part_pz);
	tree->Branch("part_e",  &part_e);
	tree->Branch("part_m",  &part_m);

	tree->Branch("part_vx", &part_vx);
	tree->Branch("part_vy", &part_vy);
	tree->Branch("part_vz", &part_vz);
	tree->Branch("part_vt", &part_vt);
	
	

    // ------------------------------------------------------------
    // Event loop
    // ------------------------------------------------------------
    long long nTries = 0;
    int nAccepted = 0;

    while (nAccepted < nAcceptedTarget && nTries < maxTries) {
    ++nTries;
    triedEvent = static_cast<int>(nTries);

    if (!pythia.next()) continue;

    // Reset event-level variables
    d0_index = -1;
    d0_id    = 0;
    d0_pt    = 0.0;
    d0_eta   = 0.0;
    d0_y     = 0.0;
    d0_phi   = 0.0;
    d0_m     = 0.0;
    d0_e     = 0.0;

    part_pythiaIndex.clear();
    part_id.clear();
    part_status.clear();
    part_mother1.clear();
    part_mother2.clear();

    part_px.clear();
    part_py.clear();
    part_pz.clear();
    part_e.clear();
    part_m.clear();

    part_vx.clear();
    part_vy.clear();
    part_vz.clear();
    part_vt.clear();

    // ------------------------------------------------------------
    // First: find a D0 / anti-D0 in requested pT range
    // ------------------------------------------------------------
    bool hasGoodD0 = false;

    for (int i = 0; i < pythia.event.size(); ++i) {
        const Particle& p = pythia.event[i];

        if (std::abs(p.id()) != 421) continue;

        if (p.pT() <= d0PtMin || p.pT() >= d0PtMax) continue;
        
        if (std::abs(p.y()) >= d0YMax) continue;

        hasGoodD0 = true;

        d0_index = i;       // index in full PYTHIA event record
        d0_id    = p.id();
        d0_pt    = p.pT();
        d0_eta   = p.eta();
        d0_y     = p.y();
        d0_phi   = p.phi();
        d0_m     = p.m();
        d0_e     = p.e();

        break;              // keep first good D0 in the event
    }

    if (!hasGoodD0) continue;

    // ------------------------------------------------------------
    // Second: store final-state particles for this accepted event
    // ------------------------------------------------------------
    for (int i = 0; i < pythia.event.size(); ++i) {
        const Particle& p = pythia.event[i];

        if (!p.isFinal()) continue;

        const int absId = std::abs(p.id());

        // Usually not useful for detector embedding
        if (absId == 12 || absId == 14 || absId == 16) continue;

        part_pythiaIndex.push_back(i);

        part_id.push_back(p.id());
        part_status.push_back(p.status());
        part_mother1.push_back(p.mother1());
        part_mother2.push_back(p.mother2());

        part_px.push_back(p.px());
        part_py.push_back(p.py());
        part_pz.push_back(p.pz());
        part_e.push_back(p.e());
        part_m.push_back(p.m());

        part_vx.push_back(p.xProd());
        part_vy.push_back(p.yProd());
        part_vz.push_back(p.zProd());
        part_vt.push_back(p.tProd());
        part_pythiaIndex.push_back(i);
    }

    acceptedEvent = nAccepted;
    tree->Fill();

    ++nAccepted;

    if (nAccepted % 1000 == 0) {
        std::cout << "Accepted " << nAccepted
                  << " / tried " << nTries << std::endl;
    }
}
    // ------------------------------------------------------------
    // Finish
    // ------------------------------------------------------------
    tree->Write();
    fout->Close();

    pythia.stat();

    std::cout << "\nDone.\n"
              << "Accepted events: " << nAccepted << "\n"
              << "Tried events:    " << nTries << "\n"
              << "Output:          pythia8_D0_DetroitTune.root\n";

    if (nAccepted < nAcceptedTarget) {
        std::cerr << "Warning: target number of accepted events was not reached.\n";
    }

    return 0;
}
