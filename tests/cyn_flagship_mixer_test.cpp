// Self-contained, deterministic behavior tests for Cyn flagship DSP controller.
#include "CynFlagshipMixer.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    InputAudioFeatures a;
    for(int i=0;i<8;++i) a.values[(size_t)i]=-2.8f;
    a.values[0]=-3.5f; a.values[1]=-3.0f; a.values[2]=-2.5f;
    a.values[8]=std::log(2.5f);
    a.rms=0.11f;
    CynAudioScene phonk;
    phonk.valid=true;
    phonk.music=0.89f;
    phonk.percussion=0.91f;
    phonk.bassDrum=0.86f;
    phonk.electronic=0.93f;
    phonk.complexity=0.65f;
    phonk.novelty=0.33f;
    CynAudioScene calm=phonk;
    calm.music=0.33f; calm.percussion=0.03f;
    calm.bassDrum=0.01f; calm.electronic=0.04f;
    calm.novelty=0.02f;
    const auto beat=CynFlagshipMixer::decide(phonk,a,false);
    const auto gentle=CynFlagshipMixer::decide(calm,a,false);
    assert(beat.preference[4]>gentle.preference[4]+0.10f);
    assert(beat.preference[3]>gentle.preference[3]+0.04f);
    assert(beat.lowEqDb>=0.0f && beat.lowEqDb<=2.7f);
    CynAudioScene singing=phonk; singing.vocals=0.96f;
    singing.speech=0.18f;
    const auto voice=CynFlagshipMixer::decide(singing,a,false);
    assert(voice.preference[0]>beat.preference[0]);
    assert(voice.presenceEqDb>=beat.presenceEqDb);
    // Hot, sibilant vocals should never be sharpened further with air EQ.
    InputAudioFeatures hissy=a; hissy.values[7]=-0.20f;
    hissy.values[6]=-3.2f;
    const auto harsh=CynFlagshipMixer::decide(singing,hissy,false);
    assert(harsh.airEqDb<voice.airEqDb);
    assert(harsh.presenceEqDb<=voice.presenceEqDb);
    CynAudioScene extraKick=phonk; extraKick.bassDrum=1.0f;
    CynAudioScene noKick=phonk; noKick.bassDrum=0.0f;
    const auto rhythmic=CynFlagshipMixer::decide(extraKick,a,false);
    const auto withoutKick=CynFlagshipMixer::decide(noKick,a,false);
    assert(rhythmic.preference[4]>withoutKick.preference[4]);
        InputAudioFeatures hot=a; hot.rms=0.60f;
    const auto overloaded=CynFlagshipMixer::decide(phonk,hot,false);
    assert(overloaded.headroom<beat.headroom);
    assert(overloaded.lowEqDb<beat.lowEqDb);
    // Sound identity / desired kick preference remains stable across a
    // volume change. The raw-input headroom and actual actuator limit gain.
    assert(std::abs(overloaded.preference[4]-beat.preference[4])<0.006f);
    CynAudioScene talk=phonk; talk.speech=0.95f; talk.vocals=0.5f;
    CynAudioScene quiet=talk; quiet.speech=0.01f; quiet.vocals=0;
    const auto dialogue=CynFlagshipMixer::decide(talk,a,true);
    const auto silence=CynFlagshipMixer::decide(quiet,a,true);
    assert(dialogue.preference[0]>silence.preference[0]);
    assert(dialogue.preference[4]<silence.preference[4]);
    assert(dialogue.lowEqDb==0.0f);
    CynScenePreference head;
    CynAudioScene positive=phonk,negative=calm;
    positive.embedding[0]=1.0f;
    negative.embedding[1]=1.0f;
    for(int i=0;i<6;++i) {
        head.observe(positive,4,true);
        head.observe(negative,4,false);
    }
    float value=0,confidence=0;
    assert(head.predict(positive,4,value,confidence));
    assert(value>0.5f && confidence>0);
    assert(head.predict(negative,4,value,confidence));
    assert(value<0.5f);
    head.clear();
    assert(!head.predict(positive,4,value,confidence));
    std::cout << "PASS: YAMNet DJ scene adaptivity, peak-risk protection, movie dialogue, per-user contextual learning\n";
}
