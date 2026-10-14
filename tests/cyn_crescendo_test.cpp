// SPDX-License-Identifier: AGPL-3.0-or-later
#include "CynFlagshipMixer.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    CynCrescendoTracker tracker;
    CynCrescendoTracker::Motion arc;
    for(int i=0;i<85;++i) arc=tracker.observe(i*100,0.02f,0.9f,0.03f,0.0f);
    assert(arc.build==0 && arc.drop==0);
    float bestRise=0,bestDrop=0;
    for(int i=85;i<105;++i) {
        arc=tracker.observe(i*100,0.09f,0.9f,0.03f,0);
        bestRise=(std::max)(bestRise,arc.build);
        bestDrop=(std::max)(bestDrop,arc.drop);
    }
    assert(bestRise>0.7f && bestDrop>0.55f);
    for(int i=105;i<235;++i) arc=tracker.observe(i*100,0.09f,0.9f,0.03f,0);
    assert(arc.build<0.01f && arc.drop<0.01f);
    tracker.reset();
    for(int i=0;i<95;++i) arc=tracker.observe(i*100,0.09f,0.9f,0.03f,0);
    assert(arc.drop==0);
    arc=tracker.observe(9600,0.0f,0.9f,0.03f,1.0f);
    assert(arc.drop==0);
    arc=tracker.observe(9700,0.09f,0.1f,0.95f,0);
    assert(arc.drop==0);
    arc=tracker.observe(22000,0.09f,0.9f,0.03f,0);
    assert(arc.drop==0);
    InputAudioFeatures audio{};
    for(int i=0;i<8;++i) audio.values[(std::size_t)i]=-2.8f;
    audio.values[8]=std::log(2.5f);
    audio.rms=0.11f;
    CynAudioScene song{};
    song.valid=true;
    song.music=0.94f;
    song.percussion=0.88f;
    song.bassDrum=0.85f;
    auto normal=CynFlagshipMixer::decide(song,audio,false);
    auto impact=normal;
    CynFlagshipMixer::applyCrescendo(impact,song,{1,1});
    assert(impact.levelingDb==0);
    assert(impact.preference[4]>normal.preference[4]);
    assert(impact.lowEqDb>=normal.lowEqDb);
    auto hot=normal;
    hot.headroom=0;
    CynFlagshipMixer::applyCrescendo(hot,song,{1,1});
    assert(hot.preference[4]<=normal.preference[4]+0.0001f);
    assert(hot.lowEqDb<=normal.lowEqDb+0.0001f);
    CynAudioScene speech=song;
    speech.music=0.20f;
    speech.speech=0.96f;
    auto spoken=normal;
    CynFlagshipMixer::applyCrescendo(spoken,speech,{1,1});
    assert(spoken.preference[4]==normal.preference[4]);
    assert(spoken.levelingDb==normal.levelingDb);
    std::cout<<"PASS crescendo measurement, strong drop, steady playback, speech, silence, headroom\n";
}
