// Cyn Smart Listening: map sensitivity to meaningful real DSP control authority.
// This header intentionally has no JUCE dependency so release CI can regression-test it.
#pragma once
#include <algorithm>
#include <cmath>

struct CynSmartRange {
    static float normalized(float percent) noexcept {
        return std::isfinite(percent) ? std::clamp(percent/100.0f,0.0f,1.0f) : 0.0f;
    }
    // The original restrained mapping remains at and below 55%; the full
    // 0..10 range becomes available progressively above that point.
    static float freedom(float percent,bool movie) noexcept {
        if(movie) return 0.0f;
        const float ramp=std::clamp((normalized(percent)-0.55f)/0.45f,0.0f,1.0f);
        return ramp*ramp*(3.0f-2.0f*ramp);
    }
    // When Music AI gets more authority, stop leaving useful model scores
    // clustered around 0.5. Deadband suppresses tiny ambiguous changes;
    // high-confidence music scores acquire real expressive contrast.
    static float expressiveScore(float score,float sensitivity,bool movie) noexcept {
        if(!std::isfinite(score)) return 0.5f;
        score=std::clamp(score,0.0f,1.0f);
        const float f=freedom(sensitivity,movie);
        if(f==0.0f) return score;
        const float shift=score-0.5f;
        const float mag=std::abs(shift);
        const float useful=std::clamp((mag-0.012f)/0.488f,0.0f,1.0f);
        const float lifted=0.5f+std::copysign(0.5f*std::pow(useful,0.64f),shift);
        return std::clamp(score+(lifted-score)*f,0.0f,1.0f);
    }
    // Adjustable *soft* pre-effect headroom envelope. At 50% preserve the
    // existing DSP. At 100% use more of the estimated usable headroom, but
    // independent hard peak gating never permits positive gain at >= 0.97
    // estimated input peak. NOT a true output-peak limiter.
    static float headroomGate(float estimate,float rawPeak,float capPercent,
                              bool movie) noexcept {
        const float conservative=std::isfinite(estimate)?
            std::clamp(estimate,0.0f,1.0f):0.0f;
        if(movie) return conservative;
        const float cap=normalized(capPercent);
        if(!std::isfinite(rawPeak) || rawPeak<0.0f) return 0.0f;
        const float hard=std::clamp((0.97f-rawPeak)/0.47f,0.0f,1.0f);
        if(rawPeak>=0.97f) return 0.0f;
        if(cap<=0.5f) return (std::min)(hard,conservative*(0.35f+1.3f*cap));
        const float t=(cap-0.5f)*2.0f;
        const float permissive=(std::max)(conservative,hard);
        return (std::min)(hard,conservative+(permissive-conservative)*t);
    }
    static float scoreFollower(float percent,bool rising,bool movie) noexcept {
        const float f=freedom(percent,movie);
        return (rising?0.18f:0.12f)+f*(rising?0.17f:0.13f);
    }
    static float effectStep(float percent,bool movie) noexcept {
        return 0.30f+0.36f*freedom(percent,movie);
    }
    static float eqStep(float percent,bool movie) noexcept {
        return 0.20f+0.20f*freedom(percent,movie);
    }
    static float eqAmount(float percent,bool movie) noexcept {
        const float s=normalized(percent);
        return s*(1.0f+1.15f*freedom(percent,movie));
    }
    static float effectTarget(float baseline,float score,float percent,
                              float legacyUp,float legacyDown,float headroom,
                              bool movie,bool peakSensitive) noexcept {
        if(!std::isfinite(baseline) || !std::isfinite(score)) return
            std::clamp(std::isfinite(baseline)?baseline:0.0f,0.0f,10.0f);
        baseline=std::clamp(baseline,0.0f,10.0f);
        const float s=normalized(percent);
        const float depth=std::clamp(s*2.0f,0.0f,1.0f);
        const float direction=std::clamp(2.0f*(score-0.5f),-1.0f,1.0f);
        const float legacy=baseline+(direction>0 ?
            direction*legacyUp*s : direction*baseline*legacyDown*depth);
        const float f=freedom(percent,movie);
        // At maximum sensitivity every effect can traverse the available
        // 0..10 actuator range. High score means high requested intensity,
        // NOT "turn everything on": neutral scores leave manual preset intact.
        const float driven=std::clamp(direction*(1.0f+1.15f*f),-1.0f,1.0f);
        const float full=baseline+(driven>=0.0f ?
            driven*(10.0f-baseline):driven*baseline);
        float result=legacy+(full-legacy)*f;
        if(!movie && peakSensitive && result>baseline)
            result=baseline+(result-baseline)*std::clamp(
                std::isfinite(headroom)?headroom:0.0f,0.0f,1.0f);
        return std::clamp(result,0.0f,10.0f);
    }
};
