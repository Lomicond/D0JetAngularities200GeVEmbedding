#include <vector>

void runMixerD0toy_ZB_chopped1000()
{
	
   // gROOT->LoadMacro("bfcMixer_Hft_D0toy.C");
gROOT->LoadMacro("bfcMixer_Hft_D0toy_manualLoop.C");
	std::vector<Int_t> triggers; 
	// NOTE: In bPythia=true mode, bfcMixer_Hft.C does not use SetTrgOpt().
	// Trigger selection must be applied before mixing or later in analysis.
	triggers.push_back(450050);
	triggers.push_back(450060);
	triggers.push_back(450005);
	triggers.push_back(450015);
	triggers.push_back(450025);

    bfcMixer_Hft(
        200,								//Number of events
        "input/st_physics_15130045_raw_1000011.chopped_1000.daq",
        "input/st_physics_15130045_raw_1000011.tags.root",
        0.1,
        10.0,
        -1.5,
        1.5,
        -150.0,
        150.0,
        100.0,
        9,
        100.0,
        triggers,
        "P16idAuAu200hftZB",
        "FlatPt",
        true,
        "D0toy.starsim.fzd"
    );
}
