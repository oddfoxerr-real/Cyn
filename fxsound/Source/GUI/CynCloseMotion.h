// Shared native close-motion styles used by Theme Studio previews and X-to-tray.
// The underlying DSP remains untouched. All effects use bounded CPU alpha coverage.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace CynCloseMotion
{
    enum Style : int { Vortex=0, GravityMelt=1, LiquidRipple=2, SoftFade=3, None=4, StarDust=5, Curtain=6, LiquidInk=7, SprayDissolve=8, NumStyles=9 };
    static constexpr int defaultStyle = Vortex;
    static constexpr int defaultSpeedPercent = 85; // Slightly slower than v2.0.56.
    static constexpr int minSpeedPercent = 55;
    static constexpr int maxSpeedPercent = 160;
    static constexpr double baseDurationMs = 700.0;
    // Deliberately readable: the X visibly grows for ~350ms at the default
    // 0.85x speed, holds as a fullscreen surface, then dissolves.
    static constexpr float phaseExpandEnd = 0.43f;
    static constexpr float phaseHoldEnd = 0.51f;
    inline float expandEase(float t) noexcept {
        t=juce::jlimit(0.0f,1.0f,t);
        return t*t*(3.0f-2.0f*t);
    }

    inline const char* styleName(int i) noexcept
    {
        switch (i)
        {
            case Vortex: return "Vortex";
            case GravityMelt: return "Gravity Melt";
            case LiquidRipple: return "Liquid Ripple";
            case SoftFade: return "Soft Fade";
            case None: return "None (instant)";
            case StarDust: return "Stardust";
            case Curtain: return "Liquid Curtain";
            case LiquidInk: return "Liquid Ink";
            case SprayDissolve: return "Spray Dissolve";
            default: return "Vortex";
        }
    }
    inline int clampStyle(int style) noexcept
    {
        return juce::jlimit(0, (int)NumStyles-1, style);
    }
    inline int clampSpeed(int speedPercent) noexcept
    {
        return juce::jlimit(minSpeedPercent,maxSpeedPercent,speedPercent);
    }
    inline double durationForSpeed(int speedPercent) noexcept
    {
        return baseDurationMs*100.0/(double)clampSpeed(speedPercent);
    }
    inline float smoothstep(float lo,float hi,float value) noexcept
    {
        const float t=juce::jlimit(0.0f,1.0f,(value-lo)/(hi-lo));
        return t*t*(3.0f-2.0f*t);
    }
    // x/y are normalized window coordinates. Fully opaque before melt begins,
    // fully transparent at its end, regardless of style.
    inline float coverage(int style,float melt,float x,float y) noexcept
    {
        melt=juce::jlimit(0.0f,1.0f,melt);
        if(melt<=0.0f) return 1.0f;
        if(melt>=1.0f) return 0.0f;
        if(style==SoftFade) return 1.0f-smoothstep(0.0f,1.0f,melt);
        if(style==StarDust) {
            const float grain=.5f+.3f*std::sin(x*123.0f+y*81.0f)*std::sin(x*199.0f-y*113.0f);
            return 1.0f-smoothstep(melt-.09f,melt+.09f,grain);
        }
        if(style==LiquidInk) {
            // Dissolve into irregular fluid droplets travelling down the window.
            const float tongue=.055f*std::sin(x*24.0f+melt*9.0f)
                              +.027f*std::sin(x*58.0f-melt*17.0f);
            const float front=-.22f+1.48f*melt+tongue;
            return smoothstep(front-.025f,front+.025f,y)
                 *(1.0f-smoothstep(.92f,1.0f,melt));
        }
        if(style==SprayDissolve) {
            // Coherent grain threshold, no per-frame random flashing.
            const float grain=.5f+.25f*std::sin(x*137.0f+y*63.0f)
                +.15f*std::sin(x*219.0f-y*181.0f);
            return 1.0f-smoothstep(melt-.10f,melt+.10f,grain);
        }
        if(style==Curtain) {
            const float front=-.22f+1.46f*melt+.08f*std::sin(x*22.0f+melt*10.0f);
            return smoothstep(front-.015f,front+.015f,y);
        }
        if(style==None) return 0.0f;
        const float pi=juce::MathConstants<float>::pi;
        const float finish=1.0f-smoothstep(.87f,1.0f,melt);
        if(style==GravityMelt)
        {
            // Upper edge falls downwards with fluid tongues of varying depth.
            const float drift=.065f*std::sin(x*22.0f+melt*8.0f)
                             +.040f*std::sin(x*51.0f-melt*13.0f);
            const float front=-.16f+1.35f*melt+drift;
            return smoothstep(front-.022f,front+.022f,y)*finish;
        }
        if(style==LiquidRipple)
        {
            // Transparent wave front travels left-to-right and oscillates.
            const float wave=.060f*std::sin(y*29.0f+melt*16.0f)
                            +.025f*std::sin(y*63.0f-melt*10.0f);
            const float front=-.16f+1.36f*melt+wave;
            return smoothstep(front-.022f,front+.022f,x)*finish;
        }
        // Vortex: full-size surface contracts into rotating, torn liquid lobes,
        // after the X has finished expanding. It NEVER shrinks the real window.
        const float dx=(x-.56f)*1.6f,dy=y-.48f;
        const float radius=1.20f*(1.0f-std::pow(melt,.86f))+.005f;
        const float swirl=.085f*std::sin(pi*melt);
        const float r=std::sqrt(dx*dx+dy*dy);
        const float a=std::atan2(dy,dx);
        const float curl=swirl*(std::sin(a*6.0f+r*14.0f-melt*15.0f)*.74f
                        +std::sin(a*11.0f-r*22.0f+melt*21.0f)*.26f);
        const float edge=.015f+.026f*melt;
        return (1.0f-smoothstep(radius-edge,radius+edge,r+curl))*finish;
    }
} // namespace CynCloseMotion
