#include <vector>

void runMixerD0toy_ZB()
{
	
    gROOT->LoadMacro("bfcMixer_Hft_D0toy.C");

    std::vector<Int_t> triggers;

    bfcMixer_Hft(
        1,
        "input/st_physics_15130045_raw_1000011.daq",
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
