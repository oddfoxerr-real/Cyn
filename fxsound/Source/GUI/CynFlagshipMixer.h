// Cyn flagship YAMNet-informed adaptive mixer. SPDX-License-Identifier: AGPL-3.0-or-later
// No allocation, I/O, or inference on the audio thread. UI-thread controls only.
#pragma once
#include "InputAudioFeatures.h"
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

// Learns More/Less taste from the ACTUAL pretrained 1024-dimensional YAMNet
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
    float lowEqDb=0;       // Transient, reversible boost on EXISTING low EQ bands
    float levelingDb=0;    // Conservative normalized loudness assist
    float headroom=0;      // Pre-effects peak estimate, not output true peak
};

// The pretrained YAMNet sound labels and embedding novelty guide a transparent,
// reversible DSP control policy. They are not direct "ideal EQ" predictions.
// Original personal labels remain independent and refine each mode.
class CynFlagshipMixer {
public:
    static CynMixDecision decide(const CynAudioScene& s,
                                 const InputAudioFeatures& audio,
                                 bool movie) noexcept {
        CynMixDecision d;
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
            // Phonk, electronic, hip-hop and other beat-heavy tracks deserve
            // punch, but clips with already-dominant subs need less extra Bass.
            d.preference[0]=0.54f+0.12f*s.vocals+0.05f*motion;
            d.preference[1]=0.50f+0.06f*musical*(1.0f-s.vocals);
            d.preference[2]=0.50f+0.09f*musical;
            d.preference[3]=std::clamp(0.55f+0.12f*rhythm+0.07f*motion-
                                         0.15f*(1.0f-safe),0.42f,0.78f);
            d.preference[4]=std::clamp(0.55f+0.30f*rhythm+0.10f*musical+
                                         0.08f*(1.0f-lowPresence)*kickPresence-
                                         0.16f*lowPresence-0.19f*(1.0f-safe),
                                         0.35f,0.94f);
            // Use musical frequencies, not classification scores, to identify
            // thin vs booming low end. EQ lift limited by measured headroom.
            d.lowEqDb=std::clamp((0.43f-lowPresence)*6.0f,0.0f,2.7f)
                       *safe*(0.35f+0.65f*rhythm);
            d.levelingDb=std::clamp((0.19f-rms)*9.0f,0.0f,2.2f)*safe;
        }
        return d;
    }
};
