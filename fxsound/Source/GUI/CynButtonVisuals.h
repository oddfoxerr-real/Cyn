// CYN shared lightweight button motion and material styles.
// Drawn with native JUCE paths/gradients; no simulation, GPU textures or DSP.
#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace CynButtonVisuals
{
    enum Motion : int { Classic=0, QuickPop=1, Float=2, Ripple=3, Liquid=4, GelPress=5, InkFlow=6, ElasticBounce=7, ViscousSlide=8, InkPulse=9, MotionCount=10 };
    enum Highlight : int { Subtle=0, NeonEdge=1, SoftGlow=2, LiquidEdge=3, GlassSweep=4, WetGloss=5, SprayPaint=6, AerosolMist=7, ChromeLiquid=8, InkMarble=9, ElectricDroplets=10, HighlightCount=11 };
    // New installs get tactile ripple/sweep feedback; stored choices win.
    constexpr int defaultMotion=Ripple;
    constexpr int defaultHighlight=GlassSweep;
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
            case GelPress: return "Liquid Gel Press";
            case InkFlow: return "Flowing Ink";
            case ElasticBounce: return "Elastic Bounce";
            case ViscousSlide: return "Viscous Slide";
            case InkPulse: return "Ink Pulse";
            default: return "Classic";
        }
    }
    inline const char* highlightName(int i) noexcept {
        switch (i) {
            case NeonEdge: return "Neon Edge";
            case SoftGlow: return "Soft Glow";
            case LiquidEdge: return "Liquid Edge";
            case GlassSweep: return "Glass Sweep";
            case WetGloss: return "Wet Gloss";
            case SprayPaint: return "Spray Paint";
            case AerosolMist: return "Aerosol Mist";
            case ChromeLiquid: return "Liquid Chrome";
            case InkMarble: return "Ink Marble";
            case ElectricDroplets: return "Electric Droplets";
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
        const float inset=(m==QuickPop?2.15f:m==Liquid?1.35f:m==GelPress?2.1f:m==InkFlow?1.7f:m==ElasticBounce?2.8f:m==ViscousSlide?1.9f:m==InkPulse?1.6f:1.0f)*pushed*feedback;
        rect=rect.reduced(inset+.5f);
        if(rect.isEmpty()) return;
        const float corner=radius+((m==Liquid || m==GelPress || m==InkFlow) ? (2.8f*over-1.7f*pushed)*feedback : 0.0f);
        Colour fill=background.interpolatedWith(
            Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f),over);
        fill=fill.interpolatedWith(accent.withAlpha(1.0f),(.07f+.19f*pushed)*feedback);
        if(m==Float) fill=fill.brighter(over*.11f*feedback);
        if(m==Liquid || m==InkFlow) fill=fill.interpolatedWith(accent,.13f*over*feedback);
        if(m==ViscousSlide) fill=fill.interpolatedWith(accent,.21f*over*feedback);
        if(m==InkPulse) fill=fill.interpolatedWith(accent,.22f*pushed*feedback);
        if(m==ElasticBounce) fill=fill.brighter(.12f*over*feedback);
        if(m==GelPress) fill=fill.interpolatedWith(accent,(.10f*over+.19f*pushed)*feedback);
        if(navigation) {
            g.setColour(fill);g.fillRoundedRectangle(rect,corner);
        } else {
            ColourGradient material(
                fill.brighter((m==Liquid || m==GelPress || m==InkFlow)?.12f:.08f),rect.getX(),rect.getY(),
                fill.darker((m==Liquid || m==GelPress || m==InkFlow)?.15f:.06f),rect.getRight(),rect.getBottom(),false);
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
        if((m==Liquid || m==InkFlow || m==GelPress) && (over>.02f || pushed>.02f) && !reducedMotion) {
            // Two small organic highlights are tied to the actual hover value.
            // They stop once the hover animation settles; no perpetual timer.
            Path film;
            const float y=rect.getY()+2.3f;
            film.startNewSubPath(rect.getX()+corner,y);
            for(int i=1;i<=10;++i) {
                const float x=rect.getX()+corner+
                    (rect.getWidth()-2.0f*corner)*(float)i/10.0f;
                const float wave=std::sin(i*.88f+over*(m==InkFlow?9.2f:4.4f)+pushed*2.7f)
                                  *(over*1.6f+pushed*(m==GelPress?3.2f:1.0f))*strength;
                film.lineTo(x,y+wave);
            }
            g.setColour(accent.withAlpha(.13f+.36f*over*strength));
            g.strokePath(film,PathStrokeType(1.15f));
        }

        if(!reducedMotion && strength>.01f && (over>.01f || pushed>.01f)) {
            Graphics::ScopedSaveState clipped(g);
            Path mask; mask.addRoundedRectangle(rect,corner);
            g.reduceClipRegion(mask);
            if(m==ElasticBounce) {
                const float bulge=(over*.45f+pushed*.55f)*strength;
                g.setColour(accent.withAlpha(.19f*bulge));
                g.fillEllipse(rect.getCentreX()-rect.getWidth()*.35f,
                    rect.getCentreY()-rect.getHeight()*(.34f+bulge*.2f),
                    rect.getWidth()*.7f,rect.getHeight()*(.68f+bulge*.4f));
            } else if(m==ViscousSlide || m==InkPulse) {
                const float v=jlimit(0.0f,1.0f,over*.65f+pushed*.65f);
                const float x=rect.getX()+(m==InkPulse ? .5f : (.16f+.68f*v))*rect.getWidth();
                g.setColour(accent.withAlpha(.26f*v*strength));
                g.fillEllipse(x-rect.getWidth()*.17f,rect.getCentreY()-rect.getHeight()*.45f,
                    rect.getWidth()*.34f,rect.getHeight()*.9f);
            }
        }
        if(reducedMotion || strength<.01f || (over<.01f && pushed<.01f)) return;
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
        case WetGloss: {
            Graphics::ScopedSaveState clipped(g);
            Path mask; mask.addRoundedRectangle(rect,corner);
            g.reduceClipRegion(mask);
            const float wet=jlimit(0.0f,1.0f,over*.8f+pushed*.8f);
            const float h=rect.getHeight()*(.18f+.32f*wet);
            ColourGradient glaze(Colours::white.withAlpha(.22f*wet*strength),
                                 rect.getX(),rect.getY(),
                                 accent.withAlpha(.025f*wet),rect.getX(),rect.getY()+h,false);
            g.setGradientFill(glaze);
            g.fillRoundedRectangle(rect.getX()+2.0f,rect.getY()+2.0f,
                                   rect.getWidth()-4.0f,h,corner*.75f);
            g.setColour(Colours::white.withAlpha(.29f*wet*strength));
            g.drawLine(rect.getX()+corner,rect.getY()+3.0f,
                       rect.getRight()-corner,rect.getY()+3.0f,1.0f);
            break;
        }
        case SprayPaint: {
            // Deterministic stipple, not randomness per frame: a real aerosol
            // plume attached to the native button, clipped inside its bounds.
            Graphics::ScopedSaveState clipped(g);
            Path mask; mask.addRoundedRectangle(rect,corner);
            g.reduceClipRegion(mask);
            const float plume=jlimit(0.0f,1.0f,over*.62f+pushed*.74f);
            const float width=rect.getWidth()*plume;
            for(int i=0;i<84;++i) {
                const float u=(float)((i*37)%89)/89.0f;
                const float v=(float)((i*53+17)%97)/97.0f;
                const float x=rect.getX()+rect.getWidth()*.5f+(u-.5f)*width;
                const float y=rect.getY()+rect.getHeight()*.5f+(v-.5f)*rect.getHeight()*.92f;
                const float r=.38f+1.2f*(float)((i*19)%11)/11.0f;
                const float central=1.0f-std::abs(u-.5f)*1.45f;
                g.setColour(accent.withAlpha(jlimit(0.0f,.36f,
                    (.07f+.23f*central)*strength*plume)));
                g.fillEllipse(x-r,y-r,r*2.0f,r*2.0f);
            }
            break;
        }
        case AerosolMist:
        case ElectricDroplets: {
            Graphics::ScopedSaveState clipped(g);
            Path mask; mask.addRoundedRectangle(rect,corner);
            g.reduceClipRegion(mask);
            const float opacity=jlimit(0.0f,1.0f,over*.58f+pushed*.68f);
            for(int i=0;i<96;++i) {
                const float u=(float)((i*41+13)%103)/103.0f;
                const float v=(float)((i*61+7)%107)/107.0f;
                const float spread=(h==AerosolMist?.92f:.66f)*opacity;
                const float x=rect.getCentreX()+(u-.5f)*rect.getWidth()*spread;
                const float y=rect.getCentreY()+(v-.5f)*rect.getHeight()*spread;
                const float radius=(h==AerosolMist?.42f:.80f)+(float)(i%4)*.19f;
                g.setColour(accent.withAlpha((h==AerosolMist?.13f:.32f)*opacity*strength));
                g.fillEllipse(x-radius,y-radius,radius*2.0f,radius*2.0f);
            }
            break;
        }
        case ChromeLiquid: {
            Graphics::ScopedSaveState clipped(g);
            Path mask; mask.addRoundedRectangle(rect,corner);
            g.reduceClipRegion(mask);
            const float gloss=jlimit(0.0f,1.0f,over*.8f+pushed*.55f)*strength;
            ColourGradient shine(Colours::white.withAlpha(.0f),rect.getX(),rect.getY(),
                                 Colours::white.withAlpha(.38f*gloss),rect.getRight(),rect.getBottom(),false);
            shine.addColour(.40,Colours::white.withAlpha(.26f*gloss));
            shine.addColour(.67,accent.withAlpha(.07f*gloss));
            g.setGradientFill(shine);
            g.fillRoundedRectangle(rect.reduced(2.0f),corner);
            break;
        }
        case InkMarble: {
            Graphics::ScopedSaveState clipped(g);
            Path mask; mask.addRoundedRectangle(rect,corner);
            g.reduceClipRegion(mask);
            const float amount=jlimit(0.0f,1.0f,over*.8f+pushed*.55f)*strength;
            for(int k=0;k<3;++k) {
                Path vein;
                const float center=rect.getY()+rect.getHeight()*(.25f+.23f*k);
                vein.startNewSubPath(rect.getX(),center);
                for(int i=1;i<=24;++i) {
                    const float p=(float)i/24.0f;
                    const float wave=std::sin(p*10.0f+over*4.0f+k*1.7f)
                        *rect.getHeight()*.13f*amount;
                    vein.lineTo(rect.getX()+p*rect.getWidth(),center+wave);
                }
                g.setColour(accent.withAlpha((.12f+.09f*k)*amount));
                g.strokePath(vein,PathStrokeType(1.1f));
            }
            break;
        }
        default: break;
        }
    }
}
