// Native CYN screen-to-screen liquid reveal. No DSP changes.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include <cmath>
namespace CynLiquidScreen {
inline void render(juce::Graphics& g, const juce::Image& outgoing,
                   const juce::Image& incoming,juce::Rectangle<int> bounds,
                   float rawProgress,int direction,juce::Colour accent,int style=0)
{
    if(!outgoing.isValid() || !incoming.isValid() || bounds.isEmpty() ||
       rawProgress>=1.0f) return;
    juce::Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(bounds);
    g.drawImageAt(outgoing,bounds.getX(),bounds.getY());
    const float t=juce::jlimit(0.0f,1.0f,rawProgress);
    // Modes 1–5 operate on the actual old/new page snapshots.
    // Never fake progress with a separate transition overlay.
    if(style==5) {
        g.drawImageAt(incoming,bounds.getX(),bounds.getY());return;
    }
    if(style==1) { // Material crossfade
        g.setOpacity(t);
        g.drawImageAt(incoming,bounds.getX(),bounds.getY());
        return;
    }
    const auto left=(float)bounds.getX(),top=(float)bounds.getY();
    const float w=(float)bounds.getWidth(),h=(float)bounds.getHeight();
    if(style==2) { // Directional glide
        juce::Graphics::ScopedSaveState slide(g);
        const float shift=(1.0f-t)*w*.20f*(direction>=0?1.0f:-1.0f);
        g.setOpacity(juce::jlimit(0.0f,1.0f,t*1.9f));
        g.addTransform(juce::AffineTransform::translation(shift,0));
        g.drawImageAt(incoming,bounds.getX(),bounds.getY());
        return;
    }
    if(style==3) { // Radial reveal
        juce::Graphics::ScopedSaveState iris(g);
        juce::Path clip;
        const float radius=std::sqrt(w*w+h*h)*t;
        clip.addEllipse(left+w*.5f-radius,top+h*.52f-radius,radius*2.0f,radius*2.0f);
        g.reduceClipRegion(clip);
        g.drawImageAt(incoming,bounds.getX(),bounds.getY());
        return;
    }
    if(style==4) { // Prismatic vertical flow
        juce::Graphics::ScopedSaveState prism(g);
        juce::Path reveal;
        const float front=t*w;
        const float origin=direction>=0?left:left+w;
        reveal.startNewSubPath(origin,top);
        reveal.lineTo(direction>=0?left+front:left+w-front,top);
        for(int i=1;i<=40;i++){
            const float yy=h*i/40.0f;
            const float wrinkle=std::sin(i*.47f+t*9.0f)*w*.038f*
                std::sin(juce::MathConstants<float>::pi*t);
            const float x=direction>=0?left+front+wrinkle:left+w-front-wrinkle;
            reveal.lineTo(x,top+yy);
        }
        reveal.lineTo(origin,top+h);reveal.closeSubPath();
        g.reduceClipRegion(reveal);
        g.drawImageAt(incoming,bounds.getX(),bounds.getY());
        return;
    }
    const float eased=t*t*(3.0f-2.0f*t);
    const float envelope=std::sin(juce::MathConstants<float>::pi*t);
    // A single connected flowing boundary, not rectangular wipe tiles.
    auto front=[&](float y) {
        const float v=y/juce::jmax(1.0f,h);
        const float wobble=(.056f*std::sin(v*19.0f+t*13.0f)+
            .027f*std::sin(v*47.0f-t*18.0f)+
            .012f*std::sin(v*91.0f+t*24.0f))*envelope;
        const float x=(eased*1.24f-.12f+wobble)*w;
        return direction>=0?x:w-x;
    };
    juce::Path reveal,boundary;
    const float origin=direction>=0?left:left+w;
    reveal.startNewSubPath(origin,top);
    reveal.lineTo(left+front(0.0f),top);
    boundary.startNewSubPath(left+front(0.0f),top);
    const int segments=juce::jmax(12,(int)(h/9.0f));
    for(int n=1;n<=segments;++n) {
        const float y=h*(float)n/(float)segments;
        reveal.lineTo(left+front(y),top+y);
        boundary.lineTo(left+front(y),top+y);
    }
    reveal.lineTo(origin,top+h);
    reveal.closeSubPath();
    // Stable aerosol droplets reveal the destination image itself.
    for(int i=0;i<260;++i) {
        const float u=(float)((i*97+23)%263)/262.0f;
        const float v=(float)((i*137+31)%269)/268.0f;
        const float arrival=direction>=0?u:1.0f-u;
        const float ahead=arrival-(eased*1.24f-.12f);
        const float jitter=.042f*std::sin(i*1.27f+v*11.0f);
        if(ahead+jitter<-.025f || ahead+jitter>.24f)continue;
        const float weight=juce::jlimit(0.0f,1.0f,
             (.24f-ahead-jitter)/.19f);
        const float radius=(2.4f+(float)((i*17)%13)*1.14f)*
                           (.50f+.5f*weight);
        const float x=left+u*w,y=top+v*h;
        reveal.addEllipse(x-radius,y-radius,2.0f*radius,2.0f*radius);
    }
    // The old image remains only outside the connected liquid mask.
    // The actual new screen is composited inside the mask DURING the switch.
    {
        juce::Graphics::ScopedSaveState clipped(g);
        g.reduceClipRegion(reveal);
        g.drawImageAt(incoming,bounds.getX(),bounds.getY());
    }
    if(t>.035f && t<.97f) {
        g.setColour(accent.withAlpha(.19f*envelope));
        g.strokePath(boundary,juce::PathStrokeType(1.4f,
            juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    }
}
} // namespace CynLiquidScreen
