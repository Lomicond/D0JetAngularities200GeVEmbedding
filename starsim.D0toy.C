// macro to instantiate the Geant3 from within
// STAR  C++  framework and get the starsim prompt
// To use it do
//  root4star starsim.C
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"

#include "StarGenerator/EVENT/StarGenParticle.h"

#include <vector>
#include <iostream>


//#include "StarGenerator/Pythia8_3_03/StarPythia8Decayer.h"
//#include "StarGenerator/DECAY/AgUDecay.h"
//#include "StarGenerator/DECAY/StarDecayManager.h"
#include "TParticlePDG.h"
#include "StarGenerator/EVENT/StarGenParticle.h"

#include "TTable.h"




class StarGenParticle;
class StarParticleData;
class TParticlePDG;
class St_geant_Maker;
St_geant_Maker *geant_maker = 0;

class StarGenEvent;
StarGenEvent   *event       = 0;

class StarPrimaryMaker;
StarPrimaryMaker *_primary = 0;

class StarKinematics;
StarKinematics *kinematics = 0;

TF1 *ptDist  = 0;
TF1 *etaDist = 0;

TFile *gPythiaFile = 0;
TTree *gPythiaTree = 0;

std::vector<int>    *part_id = 0;
std::vector<double> *part_px = 0;
std::vector<double> *part_py = 0;
std::vector<double> *part_pz = 0;
std::vector<double> *part_e  = 0;
std::vector<double> *part_m  = 0;

Long64_t gPythiaEntry   = 0;
Long64_t gPythiaEntries = 0;

const Bool_t printFirstEvent = kFALSE;
void PrintG2TRawRows()
{
    TDataSet *trkDS = chain->GetDataSet("g2t_track");
    TDataSet *vtxDS = chain->GetDataSet("g2t_vertex");

    if (!trkDS || !vtxDS) {
        cout << "Missing g2t_track or g2t_vertex" << endl;
        chain->ls(3);
        return;
    }

    TTable *trk = (TTable*)trkDS;
    TTable *vtx = (TTable*)vtxDS;

    cout << "\n================ g2t_track rows ================" << endl;
	trk->Print(4925, 4935);

	    cout << "\n================ g2t_vertex rows 4056-4066================" << endl;
	vtx->Print(4056, 4066);
}
void SetupD0InGeant3()
{
    cout << "Registering D0 in GEANT3 using existing StarParticleData entry" << endl;

    StarParticleData& data = StarParticleData::instance();

    TParticlePDG* D0 = data.GetParticle(421);

    if (!D0) {
        cout << "ERROR: D0 not found in StarParticleData" << endl;
        return;
    }

    data.AddParticleToG3(D0, 37);

    cout << "D0 registered:" << endl;
    D0->Print();

    TParticlePDG* D0bar = data.GetParticle(-421);
    if (D0bar) {
        cout << "D0bar found:" << endl;
        D0bar->Print();
    } else {
        cout << "WARNING: D0bar not found in StarParticleData" << endl;
    }
}


// ----------------------------------------------------------------------------
void geometry( TString tag, Bool_t agml=true )
{
  TString cmd = "DETP GEOM "; cmd += tag;
  if ( !geant_maker ) geant_maker = (St_geant_Maker *)chain->GetMaker("geant");
  geant_maker -> LoadGeometry(cmd);
  //  if ( agml ) command("gexec $STAR_LIB/libxgeometry.so");
}
//-----------------------------------------------------------------------------
void OpenPythiaTree(const char *fname = "Pythia/pythia8_D0_DetroitTune.root")
{
    TDirectory *savedDir = gDirectory;   // důležité

    gPythiaFile = TFile::Open(fname, "READ");

    if (!gPythiaFile || gPythiaFile->IsZombie()) {
        std::cout << "ERROR: Cannot open PYTHIA file: " << fname << std::endl;
        gPythiaFile = 0;
        gPythiaTree = 0;
        if (savedDir) savedDir->cd();
        return;
    }

    gPythiaTree = (TTree*)gPythiaFile->Get("D0Tree");

    if (!gPythiaTree) {
        std::cout << "ERROR: Cannot find D0Tree in file: " << fname << std::endl;
        if (savedDir) savedDir->cd();
        return;
    }

    gPythiaTree->SetBranchAddress("part_id", &part_id);
    gPythiaTree->SetBranchAddress("part_px", &part_px);
    gPythiaTree->SetBranchAddress("part_py", &part_py);
    gPythiaTree->SetBranchAddress("part_pz", &part_pz);
    gPythiaTree->SetBranchAddress("part_e",  &part_e);
    gPythiaTree->SetBranchAddress("part_m",  &part_m);

    gPythiaEntries = gPythiaTree->GetEntries();
    gPythiaEntry = 0;

    std::cout << "Opened PYTHIA tree: " << fname
              << ", entries = " << gPythiaEntries << std::endl;

    if (savedDir) savedDir->cd();        // místo gROOT->cd()
}
//-----------------------------------------------------------------------------
/*
const char* ParticleNameFromPdg(Int_t pdg)
{
    // D0 zatím nepouštíme do GEANTu, dokud ho explicitně nepřidáme do G3.
    if (TMath::Abs(pdg) == 421) return "";

    // Neutrina nemají smysl pro detector simulation.
    const Int_t a = TMath::Abs(pdg);
    if (a == 12 || a == 14 || a == 16) return "";

    TParticlePDG *part = StarParticleData::instance().GetParticle(pdg);
    if (!part) return "";

    // Pro další test necháme jen nabité částice.
    // TParticlePDG::Charge() je v jednotkách |e|/3, takže stačí nenulovost.
    if (part->Charge() == 0) return "";

    return part->GetName();
}*/

void PrintParticleName(Int_t pdg)
{
    StarParticleData& data = StarParticleData::instance();
    TParticlePDG* p = data.GetParticle(pdg);

    if (!p) {
        cout << "PDG " << pdg << " not found" << endl;
        return;
    }

    cout << "PDG " << pdg << " name = " << p->GetName() << endl;
}

const char* ParticleNameFromPdg(Int_t pdg)
{
    switch (pdg) {
        // photons
        case   22: return "gamma";

        // leptons
        case   11: return "e-";
        case  -11: return "e+";
        case   13: return "mu-";
        case  -13: return "mu+";

        // light charged hadrons
        case  211: return "pi+";
        case -211: return "pi-";
        case  321: return "K+";
        case -321: return "K-";

        // baryons
        case  2212: return "proton";
        case -2212: return "antiproton";
        case  2112: return "neutron";
        case -2112: return "antineutron";

        // neutral strange hadrons
        case  130: return "K_L0";
        case  310: return "K_S0";
        case  3122: return "Lambda0";
        case -3122: return "Lambda0_bar";

        // charm
        case  421: return "D0";
        case -421: return "D0_bar";

        default: return "";
    }
}
//-----------------------------------------------------------------------------
Bool_t AddPythiaParticleToStar(Int_t pdg,
                               Double_t px,
                               Double_t py,
                               Double_t pz,
                               Double_t e)
{
    const char* name = ParticleNameFromPdg(pdg);

    if (!name || name[0] == '\0') return kFALSE;

    StarGenParticle *p = kinematics->AddParticle(name);

    if (!p) {
        std::cout << "WARNING: Could not add particle PDG="
                  << pdg << " name=" << name << std::endl;
        return kFALSE;
    }

    p->SetStatus(1);
    p->SetPx(px);
    p->SetPy(py);
    p->SetPz(pz);
    p->SetEnergy(e);

    p->SetVx(0.0);
    p->SetVy(0.0);
    p->SetVz(0.0);
    p->SetTof(0.0);

    return kTRUE;
}
// ----------------------------------------------------------------------------
void command( TString cmd )
{
  if ( !geant_maker ) geant_maker = (St_geant_Maker *)chain->GetMaker("geant");
  geant_maker -> Do( cmd );
}

void AddFixedParticle(const char* name,
                      double px, double py, double pz,
                      double vx = 0.0, double vy = 0.0, double vz = 0.0)
{
    StarGenParticle* p = kinematics->AddParticle(name);

    const double m = p->GetMass();
    const double e = TMath::Sqrt(px*px + py*py + pz*pz + m*m);

    p->SetPx(px);
    p->SetPy(py);
    p->SetPz(pz);
    p->SetEnergy(e);

    // For now keep the particle production point at the event vertex.
    // The primary maker / starsim macro can still apply the global vertex treatment.
    p->SetVx(vx);
    p->SetVy(vy);
    p->SetVz(vz);
    p->SetTof(0.0);
}

void PrintG2TTruthTables()
{
    TDataSet *trkDS = chain->GetDataSet("g2t_track");
    TDataSet *vtxDS = chain->GetDataSet("g2t_vertex");

    if (!trkDS || !vtxDS) {
        cout << "Could not find g2t_track or g2t_vertex directly. Listing chain:" << endl;
        chain->ls(3);
        return;
    }

    cout << "\n================ g2t_track ================" << endl;
    trkDS->Print();

    cout << "\n================ g2t_vertex ================" << endl;
    vtxDS->Print();
}

void SetupD0Decay()
{
    cout << "Setting up D0 decay with StarPythia8Decayer" << endl;

    StarDecayManager   *decayMgr = AgUDecay::Manager();
    StarPythia8Decayer *decayPy8 = new StarPythia8Decayer();

    decayMgr->AddDecayer(0, decayPy8);
    decayPy8->SetDebug(0);
    decayPy8->Set("Print:quiet = on");

    // First try exactly the channel used in the STAR HFjets macro.
    decayPy8->Set("421:onMode = 0");
    decayPy8->Set("421:onIfMatch = 321 -211");

    cout << "D0 decay configured" << endl;
}
// ----------------------------------------------------------------------------

void trig(Int_t n=1)
{
    if (!gPythiaTree) {
        OpenPythiaTree("./Pythia/pythia8_D0_DetroitTune.root");
    }

    if (!gPythiaTree) {
        cout << "No PYTHIA tree available. Stop." << endl;
        return;
    }

/*
	PrintParticleName(22);
PrintParticleName(11);
PrintParticleName(-11);
PrintParticleName(13);
PrintParticleName(-13);
PrintParticleName(2212);
PrintParticleName(-2212);
PrintParticleName(2112);
PrintParticleName(-2112);
PrintParticleName(130);
PrintParticleName(310);
PrintParticleName(3122);
PrintParticleName(-3122);
*/



    for (Int_t iev = 0; iev < n; iev++) {

        if (gPythiaEntry >= gPythiaEntries) {
            cout << "No more PYTHIA events." << endl;
            break;
        }

        chain->Clear();

        gPythiaTree->GetEntry(gPythiaEntry);
        gPythiaEntry++;

	Int_t nInput = part_id->size();
	Int_t nAdded = 0;
	Int_t nAddedD0 = 0;
	Int_t nAddedD0bar = 0;

	Int_t nSkippedNeutrino = 0;
	Int_t nSkippedUnknown = 0;
	Int_t nFailedAdd = 0;

        for (size_t ip = 0; ip < part_id->size(); ip++) {

		    const Int_t pdg = part_id->at(ip);
		    const Int_t absPdg = TMath::Abs(pdg);

		    // Neutrinos are useless for detector simulation.
		    //12  = nu_e      // elektronové neutrino
		    //14  = nu_mu     // mionové neutrino
		    //16  = nu_tau    // tau neutrino
		    if (absPdg == 12 || absPdg == 14 || absPdg == 16) {
			    nSkippedNeutrino++;
			    continue;
			}

		// Particles without verified STAR name
		const char* name = ParticleNameFromPdg(pdg);
		if (!name || name[0] == '\0') {
		    nSkippedUnknown++;
		    continue;
		}

		Bool_t added = AddPythiaParticleToStar(pdg,
				                       part_px->at(ip),
				                       part_py->at(ip),
				                       part_pz->at(ip),
				                       part_e->at(ip));

		if (!added) {
		    nFailedAdd++;
		    continue;
		}

		nAdded++;

		if (pdg == 421)  nAddedD0++;
		if (pdg == -421) nAddedD0bar++;

        }
        
		cout << "PYTHIA event " << gPythiaEntry-1
		     << " input = " << nInput
		     << ", added = " << nAdded
		     << ", added D0 = " << nAddedD0
		     << ", added D0bar = " << nAddedD0bar
		     << ", skipped neutrino = " << nSkippedNeutrino
		     << ", skipped unknown = " << nSkippedUnknown
		     << ", failed add = " << nFailedAdd
		     << endl;

        chain->Make();
  /*      if (iev == 0) {
    PrintG2TRawRows();
}*/


	if (printFirstEvent && iev == 0 && _primary && _primary->event()) {
	    _primary->event()->Print();
	}
	
    }
}
/*
void trig(Int_t n = 1)
{
    for (Int_t iev = 0; iev < n; iev++) {

        chain->Clear();

        StarGenParticle *p = kinematics->AddParticle("D0_bar");

        p->SetStatus(1);
        p->SetPx(2.0);
        p->SetPy(0.0);
        p->SetPz(0.5);

        const double mD0 = 1.86483;
        const double eD0 = TMath::Sqrt(2.0*2.0 + 0.5*0.5 + mD0*mD0);

        p->SetEnergy(eD0);
        p->SetMass(mD0);

        p->SetVx(0.0);
        p->SetVy(0.0);
        p->SetVz(0.0);
        p->SetTof(0.0);

        chain->Make();

        if (_primary && _primary->event()) {
            _primary->event()->Print();
        }
    }
}
*/
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void Kinematics()
{
  
  //  gSystem->Load( "libStarGeneratorPoolPythia6_4_23.so" );
  gSystem->Load( "libKinematics.so");
  kinematics = new StarKinematics();
    
  _primary->AddGenerator(kinematics);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void starsim( Int_t nevents=1, Int_t rngSeed=1234 )
{ 
    gROOT->ProcessLine(".L bfc.C");
    {
        TString simple = "y2014a geant gstar usexgeom agml ";
        bfc(0, simple);
    }

gSystem->Load("libVMC.so");

gSystem->Load("StarGeneratorUtil.so");
gSystem->Load("StarGeneratorEvent.so");
gSystem->Load("StarGeneratorBase.so");

gSystem->Load("libMathMore.so");
gSystem->Load("xgeometry.so");

gSystem->Load("StarGeneratorDecay.so");
gSystem->Load("Pythia8_3_03.so");
geometry("field=-5.005 y2014a");
    // Do not load these manually for now:
    // gSystem->Load("libStarGeneratorUtil.so");
    // gSystem->Load("libStarGeneratorEvent.so");
    // gSystem->Load("libStarGeneratorBase.so");

    // Do not set RNG manually for now:
    // StarRandom::seed(rngSeed);
    // StarRandom::capture();

    _primary = new StarPrimaryMaker();
    //_primary->SetRunNumber(15130045);

    _primary->SetFileName("D0toy.starsim.root");
    chain->AddBefore("geant", _primary);

    Kinematics();

    //SetupD0InGeant3();
    SetupD0Decay();


    _primary->Init();

    command("gkine -4 0");
    command("gfile o D0toy.starsim.fzd");

    trig(nevents);

    command("call agexit");
}
// ----------------------------------------------------------------------------

