// SPDX-License-Identifier: AGPL-3.0-or-later
// CYN APEX Flow Hybrid: passage-level DSP guidance from pretrained YAMNet
// and the separately trained Cyn Real Audio 13h personal preference profile.
// No inference, allocations, I/O, or locks on the audio callback.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>

class CynApexFlowHybrid final {
public:
    struct Observation {
        std::int64_t ms=0;
        float rms=0, peak=1, music=0, speech=0, vocals=0;
        float percussion=0, bassDrum=0, silence=0;
        bool pretrainedSceneValid=false;
    };
    struct Control {
        float build=0, impact=0, breakdown=0;
        // Signed changes to the NORMALIZED learned DSP preference outputs.
        float bassBias=0, dynamicsBias=0, clarityBias=0;
        float lowEqDb=0;
        bool valid=false;
    };
    void reset() noexcept { count_=0; position_=0; lastMs_=-1; current_={}; }
    Control current() const noexcept { return current_; }

    Control observe(const Observation& o) noexcept {
        const bool plausible= o.pretrainedSceneValid && o.ms>=0 &&
            finite(o.rms) && finite(o.peak) && finite(o.music) &&
            finite(o.speech) && finite(o.vocals) && finite(o.percussion) &&
            finite(o.bassDrum) && finite(o.silence) &&
            o.rms>0.004f && o.peak>=0 && o.peak<10.0f &&
            o.music>=0.55f && o.silence<0.55f &&
            !(o.speech>0.80f && o.music<0.85f);
        if (!plausible) { reset(); return current_; }
        if (lastMs_>=0 && o.ms<=lastMs_) return current_;
        if (lastMs_>=0 && o.ms-lastMs_>1600) reset();
        lastMs_=o.ms;
        history_[position_]={o.ms,o.rms};
        position_=(position_+1)%history_.size();
        count_=(std::min)(count_+1,history_.size());
        float recent=0,older=0; int nr=0,no=0;
        for(std::size_t i=0;i<count_;++i) {
            const auto age=o.ms-history_[i].ms;
            if(age>=0 && age<=500){recent+=history_[i].rms;++nr;}
            else if(age>=1300 && age<=4500){older+=history_[i].rms;++no;}
        }
        if(nr<3 || no<9){current_={};return current_;}
        recent/=nr;older/=no;
        if(recent<0.005f || older<0.005f){current_={};return current_;}
        const float deltaDb=20.0f*std::log10(recent/(std::max)(older,0.0001f));
        const float beat=clamp01(0.60f*o.percussion+0.40f*o.bassDrum);
        const float gainSafety=clamp01((0.96f-o.peak)/0.26f);
        const float musicConfidence=clamp01((o.music-0.40f)/0.45f);
        const float rise=clamp01((deltaDb-1.0f)/6.0f);
        const float impulse=clamp01((deltaDb-4.2f)/7.0f)*beat*musicConfidence;
        const float fall=clamp01((-deltaDb-2.5f)/6.0f);
        current_.build=0.76f*current_.build+0.24f*rise;
        current_.impact=(std::max)(current_.impact*0.78f,impulse);
        current_.breakdown=0.73f*current_.breakdown+0.27f*fall;
        current_.valid=true;
        const float liveMusic=musicConfidence*(1.0f-0.5f*clamp01(o.speech));
        const float impact=liveMusic*current_.impact*gainSafety;
        const float build=liveMusic*current_.build*gainSafety;
        current_.bassBias=clamp(0.22f*impact+0.04f*build-0.055f*current_.breakdown,
                                -0.06f,0.24f);
        current_.dynamicsBias=clamp(0.12f*impact+0.045f*build-0.06f*current_.breakdown,
                                    -0.07f,0.14f);
        // Midrange vocal presence, NEVER high-frequency air/sibilance boosting.
        current_.clarityBias=clamp(0.035f*clamp01(o.vocals)*gainSafety*liveMusic,
                                   0.0f,0.035f);
        current_.lowEqDb=clamp((0.72f*impact+0.10f*build)*gainSafety,0.0f,0.8f);
        return current_;
    }
private:
    struct Sample {std::int64_t ms=0;float rms=0;};
    std::array<Sample,80> history_{};
    std::size_t count_=0,position_=0;
    std::int64_t lastMs_=-1;
    Control current_{};
    static bool finite(float v) noexcept {return std::isfinite(v);}
    static float clamp(float x,float a,float b) noexcept {return (std::max)(a,(std::min)(b,x));}
    static float clamp01(float x) noexcept {return clamp(x,0.0f,1.0f);}
};
