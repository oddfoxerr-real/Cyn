// CYN native JUCE expressive motion. Adapted two-stage pill morph from
// Riff app.js updateNavIndicator(), MIT (c) 2026 rootscripts.
// Riff source: https://github.com/rootscripts/riff/blob/main/app.js
// MIT attribution: third_party/riff-LICENSE.txt
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace CynExpressiveMotion {
inline float easeFactor(float elapsedMs,float durationMs) noexcept {
    if(elapsedMs<=0.0f) return 0.0f;
    if(durationMs<=0.0f) return 1.0f;
    return 1.0f-std::exp(-(std::min)(elapsedMs,100.0f)/durationMs);
}
inline float interpolate(float a,float b,float factor) noexcept {
    return a+(b-a)*factor;
}
// Evaluate Riff's actual CSS cubic-bezier easing, solving x(t) by bisection.
inline float bezier(float progress,float x1,float y1,float x2,float y2) noexcept {
    progress=(std::max)(0.0f,(std::min)(1.0f,progress));
    float lo=0.0f,hi=1.0f;
    for(int i=0;i<12;++i) {
        const float t=(lo+hi)*.5f,u=1.0f-t;
        const float x=3.0f*u*u*t*x1+3.0f*u*t*t*x2+t*t*t;
        if(x<progress)lo=t;else hi=t;
    }
    const float t=(lo+hi)*.5f,u=1.0f-t;
    return 3.0f*u*u*t*y1+3.0f*u*t*t*y2+t*t*t;
}

struct State {
    float navigationY=104.0f;
    float focusX=258.0f;
    float focusWidth=126.0f;
    std::array<float,6> hovered{};
    // Riff app.js: stretch to span both pill positions for 260ms, then
    // settle into the destination over 340ms. Retarget without queuing.
    float indicatorTop=104.0f,indicatorHeight=48.0f;
    float stretchFromTop=104.0f,stretchFromHeight=48.0f;
    float stretchTop=104.0f,stretchHeight=48.0f;
    float destinationTop=104.0f;
    float indicatorElapsedMs=600.0f;
    int lastPage=-1;
    bool initialized=false;

    void advance(float elapsedMs,int page,int focus,
                 const std::array<bool,6>& hoveredTargets,
                 bool reducedMotion) noexcept {
        page=(std::max)(0,(std::min)(page,4));
        focus=(std::max)(0,(std::min)(focus,2));
        const float targetY=104.0f+58.0f*static_cast<float>(page);
        const float targetX=focus==0?258.0f:focus==1?392.0f:526.0f;
        const float targetWidth=focus==2?164.0f:126.0f;
        const float dt=(std::max)(0.0f,(std::min)(elapsedMs,100.0f));
        if(!initialized||reducedMotion) {
            indicatorTop=targetY;indicatorHeight=48.0f;
            destinationTop=targetY;indicatorElapsedMs=600.0f;
        } else {
            if(page!=lastPage) {
                stretchFromTop=indicatorTop;
                stretchFromHeight=indicatorHeight;
                stretchTop=(std::min)(indicatorTop,targetY);
                stretchHeight=(std::max)(indicatorTop+indicatorHeight,targetY+48.0f)-stretchTop;
                destinationTop=targetY;
                indicatorElapsedMs=0.0f;
            }
            indicatorElapsedMs=(std::min)(600.0f,indicatorElapsedMs+dt);
            if(indicatorElapsedMs<=260.0f) {
                const float k=bezier(indicatorElapsedMs/260.0f,.3f,0.0f,.8f,.15f);
                indicatorTop=interpolate(stretchFromTop,stretchTop,k);
                indicatorHeight=interpolate(stretchFromHeight,stretchHeight,k);
            } else {
                const float k=bezier((indicatorElapsedMs-260.0f)/340.0f,.2f,0.0f,0.0f,1.0f);
                indicatorTop=interpolate(stretchTop,destinationTop,k);
                indicatorHeight=interpolate(stretchHeight,48.0f,k);
            }
        }
        const float positionBlend=reducedMotion||!initialized
            ?1.0f:easeFactor(dt,78.0f);
        const float hoverBlend=reducedMotion||!initialized
            ?1.0f:easeFactor(dt,52.0f);
        navigationY=interpolate(navigationY,targetY,positionBlend);
        focusX=interpolate(focusX,targetX,positionBlend);
        focusWidth=interpolate(focusWidth,targetWidth,positionBlend);
        for(std::size_t i=0;i<hovered.size();++i)
            hovered[i]=interpolate(hovered[i],hoveredTargets[i]?1.0f:0.0f,hoverBlend);
        initialized=true;
        lastPage=page;
    }
};
} // namespace CynExpressiveMotion
