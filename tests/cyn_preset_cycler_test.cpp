#include "CynPresetCycler.h"
#include <cassert>
#include <vector>
#include <iostream>

int main() {
    CynPresetCycler cycler;
    std::vector<CynPresetSignature> profiles {
        {.35f,.35f,.40f,.40f,.50f}, // manual baseline
        {.92f,.80f,.25f,.20f,.45f}, // bass/impact
        {.16f,.25f,.90f,.24f,.30f}  // vocal/clarity
    };
    CynAudioScene beat;
    beat.valid=true;beat.music=.95f;beat.bassDrum=.97f;
    beat.percussion=.95f;beat.electronic=.85f;beat.silence=0;
    for(int i=0;i<6;++i)
        assert(cycler.update(profiles,beat,(uint64_t)(100+i*100),true)==0);
    assert(cycler.update(profiles,beat,700,true)==1);
    // One rogue note cannot immediately force a different preset.
    CynAudioScene vocal=beat;
    vocal.bassDrum=.05f;vocal.percussion=.05f;vocal.electronic=0;
    vocal.vocals=1;vocal.speech=.25f;vocal.music=.9f;
    for(int i=0;i<15;++i)
        assert(cycler.update(profiles,vocal,(uint64_t)(800+i*100),true)==1);
    int selected=1;
    for(int i=0;i<14;++i)
        selected=cycler.update(profiles,vocal,(uint64_t)(2400+i*100),true);
    assert(selected==2);
    // Dialogue, silence, missing inference and paused playback freeze selection.
    vocal.silence=.9f;
    assert(cycler.update(profiles,vocal,5000,true)==2);
    assert(cycler.update(profiles,beat,5100,false)==2);
    vocal.silence=0;
    cycler.reset();
    assert(cycler.selected()==0);
    std::cout<<"PASS: real music scene chooses a cached profile; 700ms stability, 1800ms hold, silence and manual reset\n";
}
