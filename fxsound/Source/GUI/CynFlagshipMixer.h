// Cyn flagship YAMNet-informed adaptive mixer. SPDX-License-Identifier: AGPL-3.0-or-later
// No allocation, I/O, or inference on the audio thread. UI-thread controls only.
#pragma once
#include "InputAudioFeatures.h"
#include "CynBalanceModel.h"
#include "CynCrescendoTracker.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>

struct CynAudioScene {
    static constexpr int embeddingSize=24;
    std::array<float,embeddingSize> embedding {};
    float speech=0, music=0, percussion=0, bassDrum=0, electronic=0;
    float vocals=0, impact=0, soundtrack=0, silence=0;
    float complexity=0, novelty=0;
    uint32_t timestamp=0;
    bool valid=false;
};

// Learns per-effect preferences from the pretrained 1024-dimensional YAMNet
// representation compressed into 24 deterministic, normalized projections.
// Separate instances are trained for Music and Movie modes. No cloud upload.
class CynScenePreference {
public:
    void clear() noexcept {
        for(auto& head:heads_) head={};
    }
    void observe(const CynAudioScene& s,int effect,bool more) noexcept {
        if(!s.valid || effect<0 || effect>=5) return;
        auto& side=heads_[(size_t)effect].side[more?1:0];
        if(side.count>=4096) return;
        for(int i=0;i<CynAudioScene::embeddingSize;++i)
            side.sum[(size_t)i]+=s.embedding[(size_t)i];
        ++side.count;
    }
    // Returns learned preference in [0,1] and confidence, if both labels exist.
    bool predict(const CynAudioScene& scene,int effect,float& value,float& confidence) const noexcept {
        if(!scene.valid || effect<0 || effect>=5) return false;
        const auto& h=heads_[(size_t)effect];
        if(h.side[0].count<3 || h.side[1].count<3) return false;
        const auto similarity=[&](int side) {
            const auto& s=h.side[side];
            float dot=0,norm=0;
            for(int i=0;i<CynAudioScene::embeddingSize;++i) {
                const float mean=s.sum[(size_t)i]/s.count;
                dot+=scene.embedding[(size_t)i]*mean;
                norm+=mean*mean;
            }
            return norm>1e-8f ? dot/std::sqrt(norm) : 0.0f;
        };
        const float positive=similarity(1),negative=similarity(0);
        // Cosine difference on learned embeddings; neutral if uninformative.
        value=std::clamp(0.5f+0.8f*(positive-negative),0.10f,0.90f);
        const int balanced=(std::min)(h.side[0].count,h.side[1].count);
        confidence=std::clamp((float)balanced/20.0f,0.0f,0.6f);
        return true;
    }
private:
    struct Accumulator { std::array<float,CynAudioScene::embeddingSize> sum {}; int count=0; };
    struct Head { Accumulator side[2] {}; };
    std::array<Head,5> heads_ {};
};

struct CynMixDecision {
    std::array<float,5> preference {{0.5f,0.5f,0.5f,0.5f,0.5f}};
    float lowEqDb=0;       // Signed temporary correction on existing low bands
    float mudEqDb=0, presenceEqDb=0, airEqDb=0;
    float kickShare=0, snareShare=0; // spectral estimates, not separated stems
    float levelingDb=0;    // Conservative normalized loudness assist
    float headroom=0;      // Pre-effects peak estimate, not output true peak

    float eqDelta(float frequency) const noexcept {
        if(!std::isfinite(frequency) || frequency<=0) return 0;
        const auto bell=[&](float center,float width) {
            const float octaves=std::log2(frequency/center)/width;
            return std::exp(-0.5f*octaves*octaves);
        };
        const float lowWeight=std::clamp((300.0f-frequency)/200.0f,0.0f,1.0f);
        return std::clamp(lowEqDb*lowWeight+mudEqDb*bell(350,0.65f)+
                          presenceEqDb*bell(2200,0.65f)+airEqDb*bell(6500,0.6f),-2.5f,2.7f);
    }
};

// Fuses YAMNet scene features, learned spectral roles and measured input
// energy into bounded DSP controls. Personal labels refine each listening mode.
class CynFlagshipMixer {
public:
    // Retain real quiet-to-loud musical contrast. Accents are bounded by peak headroom.
    static void applyCrescendo(CynMixDecision& d,const CynAudioScene& s,
                               const CynCrescendoTracker::Motion& arc) noexcept {
        if(!s.valid || !std::isfinite(s.music) || !std::isfinite(s.speech) ||
           !std::isfinite(s.percussion) || !std::isfinite(s.bassDrum) ||
           s.music<0.55f || (s.speech>0.80f && s.music<0.80f)) return;
        // Do not automatically fill an intentionally quiet intro.
        // The saved preset's volume leveling remains untouched in the controller.
        d.levelingDb=0.0f;
        const float rhythmic=std::clamp(0.55f*s.percussion+0.45f*s.bassDrum,0.0f,1.0f);
        const float impact=std::clamp(arc.drop,0.0f,1.0f)*rhythmic*d.headroom;
        d.preference[3]=std::clamp(d.preference[3]-
            0.025f*std::clamp(arc.build,0.0f,1.0f)+0.06f*impact,0.0f,0.80f);
        d.preference[4]=std::clamp(d.preference[4]+0.14f*impact,0.0f,0.94f);
        d.lowEqDb=std::clamp(d.lowEqDb+0.48f*impact,-2.5f,2.7f);
    }

    static CynMixDecision decide(const CynAudioScene& scene,
                                 const InputAudioFeatures& audio,
                                 bool movie) noexcept {
        CynMixDecision d;
        // Invalid or silent frames must never request extra gain.
        if(!std::isfinite(audio.rms) || audio.rms<=.0001f) return d;
        for(float value:audio.values) if(!std::isfinite(value)) return d;
        const auto probability=[](float v) { return std::isfinite(v)?std::clamp(v,0.0f,1.0f):0.0f; };
        CynAudioScene s=scene.valid?scene:CynAudioScene{};
        s.music=probability(s.music); s.speech=probability(s.speech);
        s.percussion=probability(s.percussion); s.bassDrum=probability(s.bassDrum);
        s.electronic=probability(s.electronic); s.vocals=probability(s.vocals);
        s.impact=probability(s.impact); s.soundtrack=probability(s.soundtrack);
        s.novelty=probability(s.novelty); s.complexity=probability(s.complexity);
        s.silence=probability(s.silence);
        const auto roles=CynBalanceModel::predict(audio);
        d.kickShare=roles.share[1]; d.snareShare=roles.share[2];
        const auto band=[&](int index) {
            return std::exp(std::clamp(std::isfinite(audio.values[(size_t)index])
                ? audio.values[(size_t)index] : -8.0f,-10.0f,0.0f));
        };
        const float low=0.65f*band(0)+0.35f*band(1);
        const float kick=0.5f*band(1)+0.5f*band(2);
        const float mid=(band(3)+band(4)+band(5))/3.0f;
        const float lowPresence=std::clamp(low/(mid+low+0.00001f),0.0f,1.0f);
        const float kickPresence=std::clamp(kick/(mid+kick+0.00001f),0.0f,1.0f);
        const float rms=std::clamp(std::isfinite(audio.rms)?audio.rms:0.0f,0.0f,4.0f);
        const float crest=std::exp(std::clamp(audio.values[8],0.0f,3.5f));
        const float peak=rms*crest; // pre-effects; allow additional DSP headroom
        d.headroom=std::clamp((0.87f-peak)/0.57f,0.0f,1.0f);
        const float safe=std::clamp((1.0f-peak)/0.35f,0.0f,1.0f);
        const float musical=std::clamp(s.music+0.35f*s.electronic+0.25f*s.percussion,0.0f,1.0f);
        const float rhythm=std::clamp(0.42f*s.percussion+0.38f*s.bassDrum+
                                       0.25f*s.electronic,0.0f,1.0f);
        const float speech=std::clamp(s.speech+0.25f*s.vocals,0.0f,1.0f);
        const float motion=std::clamp(s.novelty*1.6f+0.18f*s.complexity,0.0f,1.0f);
        if(movie) {
            d.preference[0]=std::clamp(0.53f+0.26f*speech+0.10f*motion,0.45f,0.90f);
            d.preference[1]=0.48f+0.09f*s.soundtrack;
            d.preference[2]=0.49f+0.13f*s.soundtrack;
            d.preference[3]=0.48f+0.07f*safe-0.05f*s.impact;
            d.preference[4]=std::clamp(0.46f+0.12f*(1.0f-lowPresence)*s.impact-
                                         0.13f*speech-0.07f*(1.0f-safe),0.28f,0.66f);
            d.lowEqDb=0;
            d.levelingDb=safe*0.7f;
        } else {
            // Music vocals need midrange intelligibility, not indiscriminate
            // 5-8 kHz gain (which exaggerates "S" sounds). YAMNet provides
            // scene context while the spectrum constrains how far we move.
            const float vocalFocus=std::clamp(0.72f*s.vocals+0.38f*s.speech,0.0f,1.0f);
            const float sibilance=band(7)/(band(6)+band(7)+1e-6f);
            const float deEss=std::clamp((sibilance-0.50f)*3.0f,0.0f,1.0f);
            const float beatConfidence=std::clamp(s.bassDrum*s.percussion,0.0f,1.0f);
            // Kick confidence reinforces pulse only when the low end is not
            // already booming and pre-effects headroom permits it.
            d.preference[0]=std::clamp(0.52f+0.21f*vocalFocus*(1.0f-0.65f*deEss)
                                           +0.05f*motion,0.45f,0.78f);
            d.preference[1]=0.50f+0.06f*musical*(1.0f-s.vocals);
            d.preference[2]=0.50f+0.09f*musical;
            d.preference[3]=std::clamp(0.55f+0.12f*rhythm+0.07f*motion-
                                         0.15f*(1.0f-safe),0.42f,0.78f);
            d.preference[4]=std::clamp(0.55f+0.30f*rhythm+0.10f*musical+
                                         0.08f*(1.0f-lowPresence)*kickPresence-
                                         0.16f*lowPresence-0.19f*(1.0f-safe),
                                         0.35f,0.94f);
            d.preference[4]=std::clamp(d.preference[4]+
                0.08f*beatConfidence*(1.0f-lowPresence)*safe-
                0.04f*vocalFocus*(1.0f-beatConfidence),0.30f,0.94f);
            // Use musical frequencies, not classification scores, to identify
            // thin vs booming low end. EQ lift limited by measured headroom.
            const float lowCorrection=std::clamp((0.43f-lowPresence)*6.0f,-2.0f,2.7f);
            const float lowRole=roles.share[0]+roles.share[1];
            // Only boost with measured headroom. Excess low energy can be cut
            // even on loud sources. Learned roles modulate, never dictate, EQ.
            d.lowEqDb=lowCorrection*(lowCorrection>0?safe:1.0f)*
                (0.35f+0.4f*rhythm+0.25f*lowRole);
            const float mud=(band(3)+band(4))/(band(3)+band(4)+band(5)+band(6)+1e-6f);
            const float brightness=band(7)/(band(5)+band(6)+band(7)+1e-6f);
            d.mudEqDb=-std::clamp((mud-.62f)*4.0f,0.0f,1.2f)*
                (0.5f+0.5f*lowRole);
            // Preserve the snare attack by avoiding extra brightness/dynamic
            // boost on already peaky or bright percussion.
            const float attack=std::clamp((crest-3.0f)/4.0f,0.0f,1.0f);
            d.presenceEqDb=std::clamp(1.05f*vocalFocus*(1.0f-deEss)*safe-
                std::clamp((brightness-.55f)*3.0f,0.0f,1.0f)*roles.share[2],
                -1.0f,1.05f);
            d.airEqDb=-std::clamp((brightness-.50f)*4.0f,0.0f,1.5f)*
                (roles.share[2]+roles.share[3])-0.65f*deEss*vocalFocus;
            d.preference[3]-=0.10f*attack*(roles.share[1]+roles.share[2]);
            d.preference[4]-=0.12f*std::clamp((lowPresence-.6f)/.4f,0.0f,1.0f)*lowRole;
            d.levelingDb=std::clamp((0.19f-rms)*9.0f,0.0f,2.2f)*safe;
            // Suppress autonomous music shaping for speech/silence scenes.
            const float musicGate=scene.valid?
                std::clamp(1.0f-s.speech*(1.0f-s.music)-s.silence,0.0f,1.0f):0.5f;
            d.lowEqDb*=musicGate; d.mudEqDb*=musicGate;
            d.presenceEqDb*=musicGate; d.airEqDb*=musicGate;
            for(auto& preference:d.preference) preference=0.5f+(preference-0.5f)*musicGate;
            d.levelingDb*=musicGate;
        }
        return d;
    }
};
