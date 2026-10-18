// CYN shared lightweight button motion and material styles.
// Drawn with native JUCE paths/gradients; no simulation, GPU textures or DSP.
#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace CynButtonVisuals
{
    enum Motion : int { Classic=0, QuickPop=1, Float=2, Ripple=3, Liquid=4, MotionCount=5 };
    enum Highlight : int { Subtle=0, NeonEdge=1, SoftGlow=2, LiquidEdge=3, GlassSweep=4, HighlightCount=5 };
    constexpr int defaultMotion=Classic;
    constexpr int defaultHighlight=Subtle;
    constexpr int defaultSpeed=100;
    constexpr int defaultIntensity=55;
    inline int motionIndex(int v) { return juce::jlimit(0,(int)MotionCount-1,v); }
    inline int highlightIndex(int v) { return juce::jlimit(0,(int)HighlightCount-1,v); }
    inline int speed(int v) { return juce::jlimit(60,160,v); }
    inline int intensity(int v) { return juce::jlimit(0,100,v); }
    inline const char* motionName(int i) noexcept {
        switch (i) {
            case QuickPop: return "Quick Pop";
            case Float: return "Soft Float";
            case Ripple: return "Ripple Press";
            case Liquid: return "Liquid Morph";
            default: return "Classic";
        }
    }
    inline const char* highlightName(int i) noexcept {
        switch (i) {
            case NeonEdge: return "Neon Edge";
            case SoftGlow: return "Soft Glow";
            case LiquidEdge: return "Liquid Edge";
            case GlassSweep: return "Glass Sweep";
            default: return "Subtle";
        }
    }

    // Shared implementation for actual native buttons and the studio preview.
    // hover/press are bounded eased values maintained by FxTheme's timer.
    inline void paint(juce::Graphics& g, juce::Rectangle<float> rect,
                      juce::Colour background, juce::Colour accent, juce::Colour outline,
                      int motion, int highlight, float hover, float press,
                      int intensityValue, bool reducedMotion, bool navigation=false)
    {
        using namespace juce;
        if(rect.isEmpty()) return;
        const int m=motionIndex(motion), h=highlightIndex(highlight);
        const float strength=intensity(intensityValue)/100.0f;
        const float over=jlimit(0.0f,1.0f,hover), pushed=jlimit(0.0f,1.0f,press);
        const float feedback=reducedMotion ? 0.0f : strength;
        const float radius=jmin(12.0f,rect.getHeight()*.36f);
        const float inset=(m==QuickPop?2.15f:m==Liquid?1.35f:1.0f)*pushed*feedback;
        rect=rect.reduced(inset+.5f);
        if(rect.isEmpty()) return;
        const float corner=radius+(m==Liquid ? (2.8f*over-1.7f*pushed)*feedback : 0.0f);
        Colour fill=background.interpolatedWith(
            Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f),over);
        fill=fill.interpolatedWith(accent.withAlpha(1.0f),(.07f+.19f*pushed)*feedback);
        if(m==Float) fill=fill.brighter(over*.11f*feedback);
        if(m==Liquid) fill=fill.interpolatedWith(accent,.13f*over*feedback);
        if(navigation) {
            g.setColour(fill);g.fillRoundedRectangle(rect,corner);
        } else {
            ColourGradient material(
                fill.brighter(m==Liquid?.12f:.08f),rect.getX(),rect.getY(),
                fill.darker(m==Liquid?.15f:.06f),rect.getRight(),rect.getBottom(),false);
            material.addColour(.42,fill);
            g.setGradientFill(material);
            g.fillRoundedRectangle(rect,corner);
            g.setColour(Colours::white.withAlpha(.04f+.065f*over*feedback));
            g.drawLine(rect.getX()+9.0f,rect.getY()+1.0f,
                       rect.getRight()-9.0f,rect.getY()+1.0f,.8f);
        }
        const Colour edge=outline.interpolatedWith(accent,
            jlimit(0.0f,1.0f,(.12f+.72f*over)*feedback));
        const float edgeAlpha=(.70f+.20f*over*feedback);
        g.setColour(edge.withMultipliedAlpha(edgeAlpha));
        g.drawRoundedRectangle(rect,corner,.85f);
        if(m==QuickPop && pushed>.01f && !reducedMotion) {
            g.setColour(accent.withAlpha(.17f*strength*pushed));
            g.fillRoundedRectangle(rect.reduced(2.0f),corner*.77f);
        }
        if(m==Float && over>.01f && !reducedMotion) {
            g.setColour(accent.withAlpha(.18f*strength*over));
            g.drawRoundedRectangle(rect.reduced(1.6f),corner,1.1f);
        }
        if(m==Ripple && pushed>.02f && !reducedMotion) {
            Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(rect.getSmallestIntegerContainer());
            const float spread=(.23f+.77f*pushed)*jmax(rect.getWidth(),rect.getHeight());
            g.setColour(accent.withAlpha(.31f*strength*(1.0f-pushed*.62f)));
            g.drawEllipse(rect.getCentreX()-spread*.5f,rect.getCentreY()-spread*.5f,
                          spread,spread,1.0f+strength);
        }
        if(m==Liquid && over>.02f && !reducedMotion) {
            // Two small organic highlights are tied to the actual hover value.
            // They stop once the hover animation settles; no perpetual timer.
            Path film;
            const float y=rect.getY()+2.3f;
            film.startNewSubPath(rect.getX()+corner,y);
            for(int i=1;i<=10;++i) {
                const float x=rect.getX()+corner+
                    (rect.getWidth()-2.0f*corner)*(float)i/10.0f;
                const float wave=std::sin(i*.88f+over*4.4f+pushed*2.7f)
                                  *over*1.6f*strength;
                film.lineTo(x,y+wave);
            }
            g.setColour(accent.withAlpha(.13f+.36f*over*strength));
            g.strokePath(film,PathStrokeType(1.15f));
        }

        if(reducedMotion || strength<.01f || over<.01f) return;
        switch(h) {
        case NeonEdge:
            g.setColour(accent.withAlpha(.16f+.48f*over*strength));
            g.drawRoundedRectangle(rect.reduced(1.1f),corner,1.4f);
            break;
        case SoftGlow: {
            // Bounded inset halo instead of an expensive drop shadow.
            for(int i=1;i<=3;++i) {
                const float inset=(float)i*1.8f;
                const auto inner=rect.reduced(inset);
                if(inner.isEmpty())break;
                g.setColour(accent.withAlpha(.19f*over*strength/(float)i));
                g.drawRoundedRectangle(inner,jmax(2.0f,corner-inset),1.5f);
            }
            break;
        }
        case LiquidEdge: {
            Path liquid;
            const float left=rect.getX()+corner*.55f;
            const float usable=rect.getWidth()-corner*1.1f;
            liquid.startNewSubPath(left,rect.getY()+1.5f);
            for(int i=1;i<=18;++i) {
                const float p=(float)i/18.0f;
                const float x=left+p*usable;
                const float y=rect.getY()+1.5f +
                    std::sin(p*13.0f+over*5.2f)*over*2.1f*strength
                    + std::sin(p*28.0f-over*3.3f)*over*.9f*strength;
                liquid.lineTo(x,y);
            }
            g.setColour(accent.withAlpha(.20f+.63f*over*strength));
            g.strokePath(liquid,PathStrokeType(1.35f));
            break;
        }
        case GlassSweep: {
            Graphics::ScopedSaveState clipped(g);
            Path mask;mask.addRoundedRectangle(rect,corner);
            g.reduceClipRegion(mask);
            const float sw=rect.getWidth()*.34f;
            const float x=rect.getX()-sw+over*(rect.getWidth()+sw);
            ColourGradient glass(Colours::white.withAlpha(0.0f),x,rect.getCentreY(),
                                 Colours::white.withAlpha(.29f*strength*over),
                                 x+sw*.5f,rect.getCentreY(),false);
            glass.addColour(1.0,Colours::white.withAlpha(0.0f));
            g.setGradientFill(glass);
            g.fillRect(rect);
            break;
        }
        default: break;
        }
    }
}
