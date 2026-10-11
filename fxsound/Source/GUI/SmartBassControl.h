// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "InputAudioFeatures.h"
#include <algorithm>
#include <cmath>

// Spectral control limit, not a classifier. Overlapping biquad bands provide a
// relative activity index, not a probability of bass or a vocal detector.
namespace SmartBassControl {
inline float activity(const InputAudioFeatures& audio) {
    if(!std::isfinite(audio.rms) || audio.rms<=0.0001f) return 0;
    float low=0,total=0;
    for(int i=0;i<5;++i) {
        if(!std::isfinite(audio.values[i])) return 0;
        const float energy=(std::max)(0.0f,std::exp(std::clamp(audio.values[i],-16.0f,0.0f))-1e-6f);
        total+=energy;
        if(i<2) low+=energy;
    }
    if(total<0.0001f) return 0;
    const float t=std::clamp((low/total-0.32f)/0.23f,0.0f,1.0f);
    return t*t*(3-2*t);
}
inline float limit(float preference,float support) {
    if(!std::isfinite(preference) || !std::isfinite(support)) return 0.5f;
    return (std::min)(std::clamp(preference,0.0f,1.0f),0.1f+0.9f*std::clamp(support,0.0f,1.0f));
}
inline float follow(float current,float target) {
    return current+(target-current)*(target<current?0.65f:0.20f);
}
}
