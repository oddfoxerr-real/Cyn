// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// AUDONIC creative FX primitives. The *trained* policy only chooses controls;
// this audio-thread renderer is deterministic, bounded, stereo/mono-safe and
// does not allocate, acquire locks, or execute neural inference.
//
// DSP at 48 kHz; only CYN Library PCM can use it in this build.
class AudonicMischief {
public:
    static constexpr int controlCount=6;
    void reset() noexcept {
        bassLP_={}; presenceLP_={}; fastEnv_={}; slowEnv_={};
        smooth_={}; activity_=0; sidePower_=midPower_=0;
    }
    void process(float& left,float& right,const std::array<float,6>& raw) noexcept {
        if(!std::isfinite(left)||!std::isfinite(right))return;
        for(int i=0;i<6;i++){
            float value=std::isfinite(raw[i])?std::clamp(raw[i],0.f,1.f):0.f;
            smooth_[i]+=(value-smooth_[i])*.0006f;
        }
        const float originalMid=(left+right)*.5f,originalSide=(left-right)*.5f;
        // ~220Hz low-pass keeps fundamental bass away from nonlinear drive.
        float processed[2]={left,right};
        for(int ch=0;ch<2;ch++) {
            float x=processed[ch];
            bassLP_[ch]+=.028f*(x-bassLP_[ch]);
            presenceLP_[ch]+=.13f*(x-presenceLP_[ch]);
            float highs=x-bassLP_[ch];
            float drive=smooth_[0]*.32f;
            // Stable tanh waveshaping, no hard clips or accidental DC gain.
            float harmonic=std::tanh(highs*(1.f+drive*2.3f))-
                std::tanh(highs);
            x+=drive*harmonic*.55f;
            float instant=std::fabs(x);
            float attack=instant>fastEnv_[ch]?.08f:.007f;
            fastEnv_[ch]+=(instant-fastEnv_[ch])*attack;
            slowEnv_[ch]+=(instant-slowEnv_[ch])*.0006f;
            float transient=std::max(0.f,fastEnv_[ch]-slowEnv_[ch]);
            x+=std::clamp(smooth_[1]*transient,0.f,.15f)*
                (x-presenceLP_[ch])*.28f;
            // A tiny subtractive tone shift, not an uncontrolled resonator.
            x-=smooth_[3]*.09f*(presenceLP_[ch]-bassLP_[ch]);
            // Bass harmonics exist ABOVE low-pass crossover to protect cones.
            x+=smooth_[5]*.06f*(std::tanh(2.1f*highs)-highs);
            // Rhythmic bite: gentle envelope shaping, never silence/dropout.
            x*=1.f-.055f*smooth_[4]*std::min(1.f,transient*3.f);
            processed[ch]=x;
        }
        // Width follows independent coherence guard: no anti-phase runaway.
        const float mid=(processed[0]+processed[1])*.5f;
        const float side=(processed[0]-processed[1])*.5f;
        midPower_+=(mid*mid-midPower_)*.0005f;
        sidePower_+=(side*side-sidePower_)*.0005f;
        const float widthGate=std::clamp(1.0f-1.5f*
            std::sqrt(sidePower_/std::max(midPower_,1e-8f)),0.f,1.f);
        const float width=smooth_[2]*.055f*widthGate;
        left=processed[0]+width*side;
        right=processed[1]-width*side;
        float change=std::max(std::fabs(left-(originalMid+originalSide)),
                              std::fabs(right-(originalMid-originalSide)));
        activity_+=(std::clamp(change*12.f,0.f,1.f)-activity_)*.0006f;
    }
    float activity() const noexcept {return activity_;}
private:
    std::array<float,2> bassLP_{},presenceLP_{},fastEnv_{},slowEnv_{};
    std::array<float,6> smooth_{};
    float activity_=0,sidePower_=0,midPower_=0;
};
