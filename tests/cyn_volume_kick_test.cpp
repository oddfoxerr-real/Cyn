// Prevent changes in user playback volume from changing learned music identity.
#include "CynLevelInvariant.h"
#include "CynFlagshipMixer.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    InputAudioFeatures normal{};
    for (int i=0;i<8;++i) normal.values[(size_t)i]=-2.8f;
    normal.values[8]=std::log(3.0f); // peak/rms crest is scale-invariant
    normal.values[10]=std::log(0.12f);
    normal.rms=0.12f;
    auto quieter=normal;
    quieter.values[10]=std::log(0.018f);
    quieter.rms=0.018f;
    const auto a=CynLevelInvariant::forRecognition(normal);
    const auto b=CynLevelInvariant::forRecognition(quieter);
    for(size_t i=0;i<a.values.size();++i) assert(std::abs(a.values[i]-b.values[i])<1e-6f);
    assert(normal.rms>quieter.rms); // headroom protection still uses raw RMS
    CynAudioScene scene{};
    scene.valid=true; scene.music=.94f; scene.percussion=.95f; scene.bassDrum=.90f;
    const auto decision=CynFlagshipMixer::decide(scene,normal,false);
    const auto lowered=CynFlagshipMixer::decide(scene,quieter,false);
    assert(std::abs(decision.preference[4]-lowered.preference[4])<.005f);
    assert(std::abs(decision.preference[3]-lowered.preference[3])<.005f);
    assert(lowered.headroom>decision.headroom);
    const float oldPreset=4.5f;
    const float corrected=CynFlagshipMixer::musicEqCorrection(decision,scene,95.0f,oldPreset);
    assert(corrected>0.0f);
    CynMixDecision lowBoom=decision;
    lowBoom.lowEqDb=-1.7f;lowBoom.headroom=.70f;
    assert(CynFlagshipMixer::musicEqCorrection(lowBoom,scene,95,oldPreset)>0.0f);
    const float thin=CynFlagshipMixer::musicEqCorrection(decision,scene,95,0.0f);
    assert(thin>0.0f);
    CynAudioScene talking=scene;talking.music=0;talking.percussion=0;talking.bassDrum=0;
    assert(CynFlagshipMixer::musicEqCorrection(decision,talking,95,oldPreset)<=corrected);
    CynMixDecision hot=decision;hot.headroom=0;hot.lowEqDb=-1.7f;
    assert(CynFlagshipMixer::musicEqCorrection(hot,scene,95,oldPreset)<0.0f);
    std::cout<<"PASS volume-neutral recognition, preserved Bass presets, kick at 95 Hz, hot-input protection\n";
}
