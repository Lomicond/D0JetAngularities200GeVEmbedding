// macro to instantiate the Geant3 from within
// STAR  C++  framework and get the starsim prompt
// To use it do
//  root4star starsim.C
// Stage 2C v2: event-by-event MoreTags matching plus the official Run14
// simulation geometry/chain configuration (y2014x, misalign,
// newtpcalignment, bigbig). Adds a minimal ROOT-directory finalization
// guard so StarPrimaryMaker can write its final stats object after geometry
// code changes gDirectory during initialization.
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"
#include "TDirectory.h"
#include "TList.h"
#include "TSystem.h"

#include "StarGenerator/EVENT/StarGenParticle.h"

#include <vector>
#include <iostream>


//#include "StarGenerator/Pythia8_3_03/StarPythia8Decayer.h"
//#include "StarGenerator/DECAY/AgUDecay.h"
//#include "StarGenerator/DECAY/StarDecayManager.h"
#include "TParticlePDG.h"

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

// MoreTags input and event-by-event metadata
TFile *gMoreTagsFile = 0;
TTree *gMoreTagsTree = 0;

Int_t    gTagRunId     = 0;
Int_t    gTagEvtId     = 0;
Double_t gTagVX        = 0.0;
Double_t gTagVY        = 0.0;
Double_t gTagVZ        = 0.0;
Double_t gTagEvtTime   = 0.0;
Double_t gTagProdTime  = 0.0;
Double_t gTagMagField  = 0.0;

Long64_t gMoreTagsEntries = 0;
Double_t gGeometryMagField = 0.0;
Int_t    gFirstRunId       = 0;

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
Bool_t CheckMoreTagsBranch(const char *name)
{
    if (!gMoreTagsTree || !gMoreTagsTree->GetBranch(name)) {
        cout << "ERROR: Missing MoreTags branch: " << name << endl;
        return kFALSE;
    }
    return kTRUE;
}

//-----------------------------------------------------------------------------
void OpenMoreTagsTree(const char *fname)
{
    TDirectory *savedDir = gDirectory;

    gMoreTagsFile = TFile::Open(fname, "READ");

    if (!gMoreTagsFile || gMoreTagsFile->IsZombie()) {
        cout << "ERROR: Cannot open MoreTags file: " << fname << endl;
        gMoreTagsFile = 0;
        gMoreTagsTree = 0;
        if (savedDir) savedDir->cd();
        return;
    }

    gMoreTagsTree = (TTree*)gMoreTagsFile->Get("MoreTags");

    if (!gMoreTagsTree) {
        cout << "ERROR: Cannot find MoreTags tree in file: " << fname << endl;
        if (savedDir) savedDir->cd();
        return;
    }

    Bool_t ok = kTRUE;
    ok = CheckMoreTagsBranch("RunId")     && ok;
    ok = CheckMoreTagsBranch("EvtId")     && ok;
    ok = CheckMoreTagsBranch("VX")        && ok;
    ok = CheckMoreTagsBranch("VY")        && ok;
    ok = CheckMoreTagsBranch("VZ")        && ok;
    ok = CheckMoreTagsBranch("EvtTime")   && ok;
    ok = CheckMoreTagsBranch("ProdTime")  && ok;
    ok = CheckMoreTagsBranch("magField")  && ok;

    if (!ok) {
        cout << "ERROR: MoreTags input is missing required branches." << endl;
        gMoreTagsTree = 0;
        if (savedDir) savedDir->cd();
        return;
    }

    gMoreTagsTree->SetBranchAddress("RunId",    &gTagRunId);
    gMoreTagsTree->SetBranchAddress("EvtId",    &gTagEvtId);
    gMoreTagsTree->SetBranchAddress("VX",       &gTagVX);
    gMoreTagsTree->SetBranchAddress("VY",       &gTagVY);
    gMoreTagsTree->SetBranchAddress("VZ",       &gTagVZ);
    gMoreTagsTree->SetBranchAddress("EvtTime",  &gTagEvtTime);
    gMoreTagsTree->SetBranchAddress("ProdTime", &gTagProdTime);
    gMoreTagsTree->SetBranchAddress("magField", &gTagMagField);

    gMoreTagsEntries = gMoreTagsTree->GetEntries();

    cout << "Opened MoreTags tree: " << fname
         << ", entries = " << gMoreTagsEntries << endl;

    if (savedDir) savedDir->cd();
}

//-----------------------------------------------------------------------------
Bool_t ReadMoreTagsEntry(Long64_t entry)
{
    if (!gMoreTagsTree) {
        cout << "ERROR: MoreTags tree is not open." << endl;
        return kFALSE;
    }

    if (entry < 0 || entry >= gMoreTagsEntries) {
        cout << "ERROR: MoreTags entry out of range: " << entry
             << " / " << gMoreTagsEntries << endl;
        return kFALSE;
    }

    const Long64_t nbytes = gMoreTagsTree->GetEntry(entry);

    if (nbytes <= 0) {
        cout << "ERROR: Failed to read MoreTags entry " << entry << endl;
        return kFALSE;
    }

    return kTRUE;
}

//-----------------------------------------------------------------------------
TFile* FindOpenRootFile(const char *fname)
{
    if (!fname || !fname[0] || !gROOT || !gROOT->GetListOfFiles()) {
        return 0;
    }

    // First try ROOT's exact-name lookup.
    TObject *exact = gROOT->GetListOfFiles()->FindObject(fname);
    if (exact && exact->InheritsFrom("TFile")) {
        TFile *file = (TFile*)exact;
        if (file->IsOpen()) return file;
    }

    // Fall back to comparing both full names and basenames.  This handles
    // relative-vs-absolute path differences without opening a second copy of
    // the output file.
    const TString targetName(fname);
    const TString targetBase(gSystem->BaseName(fname));

    TIter nextFile(gROOT->GetListOfFiles());
    TObject *obj = 0;

    while ((obj = nextFile())) {
        if (!obj->InheritsFrom("TFile")) continue;

        TFile *file = (TFile*)obj;
        if (!file->IsOpen()) continue;

        const TString openName(file->GetName());
        const TString openBase(gSystem->BaseName(openName.Data()));

        if (openName == targetName || openBase == targetBase) {
            return file;
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------
Bool_t RestorePrimaryOutputDirectory(const char *fname)
{
    TFile *outputFile = FindOpenRootFile(fname);

    if (!outputFile) {
        cout << "WARNING: Could not find open StarPrimaryMaker ROOT output file: "
             << fname << endl;
        cout << "WARNING: Final StarGenStats write may still fail if gDirectory "
             << "is not associated with a file." << endl;
        return kFALSE;
    }

    if (!outputFile->cd()) {
        cout << "WARNING: Failed to cd() to StarPrimaryMaker ROOT output file: "
             << outputFile->GetName() << endl;
        return kFALSE;
    }

    cout << "Restored ROOT output directory for finalization: "
         << gDirectory->GetPath() << endl;

    return kTRUE;
}

//-----------------------------------------------------------------------------
void CloseInputFiles()
{
    if (gPythiaFile) {
        gPythiaFile->Close();
        delete gPythiaFile;
        gPythiaFile = 0;
        gPythiaTree = 0;
    }

    if (gMoreTagsFile) {
        gMoreTagsFile->Close();
        delete gMoreTagsFile;
        gMoreTagsFile = 0;
        gMoreTagsTree = 0;
    }
}

//-----------------------------------------------------------------------------
void OpenPythiaTree(const char *fname)
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
        cout << "ERROR: No PYTHIA tree available." << endl;
        return;
    }

    if (!gMoreTagsTree) {
        cout << "ERROR: No MoreTags tree available." << endl;
        return;
    }

    for (Int_t iev = 0; iev < n; iev++) {

        const Long64_t entry = (Long64_t)iev;

        if (entry >= gPythiaEntries || entry >= gMoreTagsEntries) {
            cout << "ERROR: Synchronized input exhausted at entry "
                 << entry << endl;
            break;
        }

        chain->Clear();

        // Read the same entry index from MoreTags and PYTHIA.
        // This is the event-by-event synchronization established in Stage 2B.
        if (!ReadMoreTagsEntry(entry)) {
            cout << "ERROR: Failed to read MoreTags entry " << entry << endl;
            break;
        }

        const Long64_t pythiaBytes = gPythiaTree->GetEntry(entry);
        if (pythiaBytes <= 0) {
            cout << "ERROR: Failed to read PYTHIA entry " << entry << endl;
            break;
        }
        gPythiaEntry = entry + 1;

        // Apply the real-data collision vertex globally through
        // StarPrimaryMaker. Individual PYTHIA particles remain at local
        // production point (0,0,0) in AddPythiaParticleToStar().
        _primary->SetVertex(gTagVX, gTagVY, gTagVZ);
        _primary->SetSigma(0.0, 0.0, 0.0);

        // Follow the official Run14 embedding convention.
        command(Form("RUNG %i %i", gTagRunId, gTagEvtId - 1));

        // Intentionally reproduce runEmbeddingSimulation2014.C here.
        // MoreTags packs EvtTime as YYYYMMDD + HHMMSS/1e6, while the
        // official macro decodes with 1e5. This will be revisited only
        // after the full workflow is operational, to keep Stage 2B
        // attributable to the official reference behavior.
        const Int_t eventDate = (Int_t)gTagEvtTime;
        const Int_t eventTime = (Int_t)(
            100000.0 * (gTagEvtTime - (Double_t)eventDate)
        );
        chain->SetDateTime(eventDate, eventTime);

        if (TMath::Abs(gTagMagField - gGeometryMagField) > 0.01) {
            cout << "WARNING: event " << entry
                 << " has magField=" << gTagMagField
                 << ", while geometry was initialized with "
                 << gGeometryMagField << endl;
        }

        Int_t nInput = part_id ? (Int_t)part_id->size() : 0;
        Int_t nAdded = 0;
        Int_t nAddedD0 = 0;
        Int_t nAddedD0bar = 0;
        Int_t nSkippedNeutrino = 0;
        Int_t nSkippedUnknown = 0;
        Int_t nFailedAdd = 0;

        if (!part_id || !part_px || !part_py || !part_pz || !part_e) {
            cout << "ERROR: Null PYTHIA particle vector at entry "
                 << entry << endl;
            break;
        }

        const size_t nParticles = part_id->size();

        if (part_px->size() != nParticles ||
            part_py->size() != nParticles ||
            part_pz->size() != nParticles ||
            part_e->size()  != nParticles) {
            cout << "ERROR: Inconsistent PYTHIA particle vector sizes at entry "
                 << entry << endl;
            break;
        }

        for (size_t ip = 0; ip < nParticles; ip++) {

            const Int_t pdg = part_id->at(ip);
            const Int_t absPdg = TMath::Abs(pdg);

            // Neutrinos are useless for detector simulation.
            if (absPdg == 12 || absPdg == 14 || absPdg == 16) {
                nSkippedNeutrino++;
                continue;
            }

            // Particles without verified STAR name.
            const char* name = ParticleNameFromPdg(pdg);
            if (!name || name[0] == '\0') {
                nSkippedUnknown++;
                continue;
            }

            const Bool_t added = AddPythiaParticleToStar(
                pdg,
                part_px->at(ip),
                part_py->at(ip),
                part_pz->at(ip),
                part_e->at(ip)
            );

            if (!added) {
                nFailedAdd++;
                continue;
            }

            nAdded++;

            if (pdg == 421)  nAddedD0++;
            if (pdg == -421) nAddedD0bar++;
        }

        cout << "MATCHED event " << entry
             << ": RunId=" << gTagRunId
             << ", EvtId=" << gTagEvtId
             << ", vertex=(" << gTagVX
             << ", " << gTagVY
             << ", " << gTagVZ << ")"
             << ", EvtTime=" << gTagEvtTime
             << ", ProdTime=" << gTagProdTime
             << ", magField=" << gTagMagField
             << ", decodedDate=" << eventDate
             << ", decodedTime=" << eventTime
             << endl;

        cout << "PYTHIA event " << entry
             << " input = " << nInput
             << ", added = " << nAdded
             << ", added D0 = " << nAddedD0
             << ", added D0bar = " << nAddedD0bar
             << ", skipped neutrino = " << nSkippedNeutrino
             << ", skipped unknown = " << nSkippedUnknown
             << ", failed add = " << nFailedAdd
             << endl;

        const Int_t makeStatus = chain->Make();

        if (makeStatus != kStOK) {
            cout << "ERROR: chain->Make() returned status "
                 << makeStatus << " for matched entry " << entry << endl;
            break;
        }

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
void starsim(Int_t nevents = 1,
             const char *pythiaInput = "Pythia/pythia8_D0_DetroitTune.root",
             const char *moreTagsInput = "moretags.root",
             const char *starsimRootOutput = "D0toy.starsim.root",
             const char *fzdOutput = "D0toy.starsim.fzd")
{
    if (nevents <= 0) {
        cout << "ERROR: nevents must be > 0" << endl;
        return;
    }

    // ------------------------------------------------------------------------
    // Open MoreTags first. The first selected real-data event defines the
    // initial STAR DB timestamps and the magnetic field, following the Run14
    // embedding reference macro.
    OpenMoreTagsTree(moreTagsInput);

    if (!gMoreTagsTree) {
        cout << "ERROR: MoreTags input initialization failed." << endl;
        CloseInputFiles();
        return;
    }

    if (gMoreTagsEntries <= 0) {
        cout << "ERROR: MoreTags tree is empty." << endl;
        CloseInputFiles();
        return;
    }

    if (!ReadMoreTagsEntry(0)) {
        cout << "ERROR: Cannot read first MoreTags entry." << endl;
        CloseInputFiles();
        return;
    }

    gGeometryMagField = gTagMagField;
    gFirstRunId = gTagRunId;

    TString SDT;
    SDT.Form("sdt%i", (Int_t)gTagEvtTime);

    TString DBV;
    DBV.Form("dbv%i", (Int_t)gTagProdTime);

    cout << "STARSIM Stage 2C configuration:" << endl
         << "  events          = " << nevents << endl
         << "  PYTHIA input    = " << pythiaInput << endl
         << "  MoreTags input  = " << moreTagsInput << endl
         << "  ROOT output     = " << starsimRootOutput << endl
         << "  FZD output      = " << fzdOutput << endl
         << "  first RunId     = " << gTagRunId << endl
         << "  first EvtId     = " << gTagEvtId << endl
         << "  first vertex    = (" << gTagVX << ", "
                                  << gTagVY << ", "
                                  << gTagVZ << ")" << endl
         << "  first EvtTime   = " << gTagEvtTime << endl
         << "  first ProdTime  = " << gTagProdTime << endl
         << "  magnetic field  = " << gGeometryMagField << endl
         << "  SDT             = " << SDT.Data() << endl
         << "  DBV             = " << DBV.Data() << endl;

    // ------------------------------------------------------------------------
    // Stage 2C: match the official Run14 embedding simulation configuration.
    // Keep the Stage 2B event matching, MoreTags metadata and timestamp behavior,
    // but switch the geometry/chain setup to runEmbeddingSimulation2014.C.
    const TString geometryTag = "y2014x";

    gROOT->ProcessLine(".L bfc.C");
    {
        TString simple = geometryTag;
        simple += " ";
        simple += SDT;
        simple += " ";
        simple += DBV;
        simple += " geant gstar usexgeom agml misalign newtpcalignment bigbig ";

        cout << "  geometry tag    = " << geometryTag.Data() << endl;
        cout << "  BFC options     = " << simple.Data() << endl;
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

    TString geometryCommand;
    geometryCommand.Form("field=%.9f %s",
                         gGeometryMagField,
                         geometryTag.Data());
    geometry(geometryCommand);

    // Keep the Stage 2A random-number treatment unchanged for now.
    // A production seed policy will be handled separately.

    _primary = new StarPrimaryMaker();
    _primary->SetFileName(starsimRootOutput);
    chain->AddBefore("geant", _primary);

    Kinematics();
    SetupD0Decay();

    _primary->Init();

    // ------------------------------------------------------------------------
    // Open PYTHIA only after the STAR generator setup, as in Stage 2A.
    OpenPythiaTree(pythiaInput);

    if (!gPythiaTree) {
        cout << "ERROR: PYTHIA input initialization failed." << endl;
        CloseInputFiles();
        return;
    }

    cout << "Input entry counts:" << endl
         << "  PYTHIA   = " << gPythiaEntries << endl
         << "  MoreTags = " << gMoreTagsEntries << endl;

    // Event-by-event matching is physics-critical. Do not silently truncate.
    if ((Long64_t)nevents > gPythiaEntries) {
        cout << "ERROR: requested " << nevents
             << " events, but PYTHIA contains only "
             << gPythiaEntries << "." << endl;
        CloseInputFiles();
        return;
    }

    if ((Long64_t)nevents > gMoreTagsEntries) {
        cout << "ERROR: requested " << nevents
             << " events, but MoreTags contains only "
             << gMoreTagsEntries << "." << endl;
        CloseInputFiles();
        return;
    }

    if (gPythiaEntries != gMoreTagsEntries) {
        cout << "WARNING: PYTHIA and MoreTags total entry counts differ. "
             << "The requested synchronized range is still valid." << endl;
    }

    // Optional consistency scan over the synchronized range. The workflow is
    // intended for one DAQ/MuDst pair (one run) per job.
    for (Int_t i = 0; i < nevents; ++i) {
        if (!ReadMoreTagsEntry(i)) {
            cout << "ERROR: failed MoreTags preflight at entry " << i << endl;
            CloseInputFiles();
            return;
        }

        if (gTagRunId != gFirstRunId) {
            cout << "ERROR: MoreTags contains multiple runs in requested range: "
                 << "entry 0 RunId=" << gFirstRunId
                 << ", entry " << i << " RunId=" << gTagRunId << endl;
            CloseInputFiles();
            return;
        }

        if (TMath::Abs(gTagMagField - gGeometryMagField) > 0.01) {
            cout << "ERROR: MoreTags magnetic field changes in requested range: "
                 << "entry 0 field=" << gGeometryMagField
                 << ", entry " << i << " field=" << gTagMagField << endl;
            CloseInputFiles();
            return;
        }
    }

    // Restore entry 0 before the event loop for deterministic diagnostics.
    ReadMoreTagsEntry(0);
    gPythiaEntry = 0;

    command("gkine -4 0");

    TString fzdCommand = "gfile o ";
    fzdCommand += fzdOutput;
    command(fzdCommand);

    trig(nevents);

    command("call agexit");

    // Close only the read-only inputs first.  Geometry/misalignment setup can
    // leave gDirectory pointing at ROOT's top-level Rint directory.  Restore
    // the still-open StarPrimaryMaker output file afterwards so its final
    // StarGenStats bookkeeping object is written to a real file at shutdown.
    CloseInputFiles();
    const Bool_t outputDirectoryRestored =
        RestorePrimaryOutputDirectory(starsimRootOutput);

    cout << "STARSIM Stage 2C v2 finished." << endl
         << "  ROOT output = " << starsimRootOutput << endl
         << "  FZD output  = " << fzdOutput << endl
         << "  output dir  = "
         << (outputDirectoryRestored ? "restored" : "NOT restored")
         << endl;
}
// ----------------------------------------------------------------------------

