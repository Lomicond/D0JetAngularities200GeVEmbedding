#include <vector>

#include "TSystem.h"
#include "TString.h"

// ROOT5/CINT note:
// The canonical mixer macro must be loaded BEFORE this wrapper, e.g.
//
//   .L src/mixer/bfcMixer_Hft_D0toy.C
//   .L src/mixer/runMixerD0toy_ZB.C
//
// Do not call gROOT->LoadMacro() for the canonical mixer from inside
// runMixerD0toy_ZB().  ROOT5/CINT may try to unload/reload the currently
// executing interpreted macro and fail with "Function ... busy".

// Forward declaration matching the canonical mixer interface used here.
void bfcMixer_Hft(
    Int_t Nevents,
    const Char_t *daqfile,
    const Char_t *tagfile,
    Double_t pt_low,
    Double_t pt_high,
    Double_t eta_low,
    Double_t eta_high,
    Double_t vzlow,
    Double_t vzhigh,
    Double_t vr,
    Int_t pid,
    Double_t mult,
    std::vector<Int_t> triggers,
    const Char_t *prodName,
    const Char_t *type,
    const bool bPythia,
    const Char_t *fzdfile
);

void runMixerD0toy_ZB(
    Int_t nevents,
    const char *daqFile,
    const char *fzdFile,
    const char *tagFile = ""
)
{
    // -------------------------------------------------------------------------
    // Basic validation
    // -------------------------------------------------------------------------
    if (nevents <= 0) {
        cout << "ERROR: nevents must be > 0, got " << nevents << endl;
        return;
    }

    if (!daqFile || TString(daqFile).Length() == 0) {
        cout << "ERROR: DAQ input path is empty." << endl;
        return;
    }

    if (!fzdFile || TString(fzdFile).Length() == 0) {
        cout << "ERROR: FZD input path is empty." << endl;
        return;
    }

    if (gSystem->AccessPathName(daqFile)) {
        cout << "ERROR: DAQ input does not exist or is not accessible:"
             << endl << "  " << daqFile << endl;
        return;
    }

    if (gSystem->AccessPathName(fzdFile)) {
        cout << "ERROR: FZD input does not exist or is not accessible:"
             << endl << "  " << fzdFile << endl;
        return;
    }

    TString tagPath = tagFile ? TString(tagFile) : TString("");

    if (tagPath.Length() > 0 && gSystem->AccessPathName(tagPath.Data())) {
        cout << "ERROR: tag input was provided but is not accessible:"
             << endl << "  " << tagPath << endl;
        return;
    }

    // Keep the canonical macro path visible in diagnostics.  It is loaded by
    // the ROOT driver before this wrapper, not from inside this function.
    const char *mixerMacro = "src/mixer/bfcMixer_Hft_D0toy.C";

    if (gSystem->AccessPathName(mixerMacro)) {
        cout << "ERROR: canonical mixer macro not found:"
             << endl << "  " << mixerMacro << endl;
        return;
    }

    cout << "Mixer Stage 3 configuration:" << endl
         << "  events       = " << nevents << endl
         << "  DAQ input    = " << daqFile << endl
         << "  FZD input    = " << fzdFile << endl
         << "  tag input    = "
         << (tagPath.Length() > 0 ? tagPath.Data() : "<unused/empty>")
         << endl
         << "  mixer macro  = " << mixerMacro << endl
         << "  production   = P16idAuAu200hftZB" << endl
         << "  bPythia      = true" << endl;

    // Trigger selection is already applied upstream by the MoreTags/chopper
    // workflow.  In bPythia=true mode the current core mixer does not use
    // SetTrgOpt(), but keep the historical Run14 trigger list explicit.
    std::vector<Int_t> triggers;
    triggers.push_back(450050);
    triggers.push_back(450060);
    triggers.push_back(450005);
    triggers.push_back(450015);
    triggers.push_back(450025);

    // The tag file is not used by the current P16idAuAu200hftZB + bPythia=true
    // branch, but bfcMixer_Hft keeps it in the historical interface.
    const char *tagArg = tagPath.Length() > 0 ? tagPath.Data() : "";

    bfcMixer_Hft(
        nevents,
        daqFile,
        tagArg,
        0.1,                    // pt_low
        10.0,                   // pt_high
        -1.5,                   // eta_low
        1.5,                    // eta_high
        -150.0,                 // vzlow
        150.0,                  // vzhigh
        100.0,                  // vr
        9,                      // pid
        100.0,                  // mult
        triggers,
        "P16idAuAu200hftZB",
        "FlatPt",
        true,
        fzdFile
    );

    cout << "Mixer Stage 3 wrapper finished." << endl;
}
