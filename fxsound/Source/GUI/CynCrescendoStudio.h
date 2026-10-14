// SPDX-License-Identifier: AGPL-3.0-or-later
// CYN Live Stage - native JUCE asymmetrical workspace.
// All visual energy comes from real pre-effect measurements or live YAMNet.
// Never alters DSP, presets, theme selection or the audio callback.
#pragma once
#include <JuceHeader.h>
#include "CynFlagshipMixer.h"
#include "FxTheme.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace CynCrescendoStudio {
using namespace juce;
inline Colour paintRole(FxColor role) {
    return Colour(FxTheme::getColor(role)).withAlpha(1.0f);
}
inline void words(Graphics& g,const String& s,int x,int y,int w,int h,
                  float size,Colour ink,bool bold=false) {
    g.setColour(ink);
    g.setFont(Font(size,bold?Font::bold:Font::plain));
    g.drawFittedText(s,x,y,w,h,Justification::centredLeft,1,0.86f);
}
inline void surface(Graphics& g,juce::Rectangle<float> bounds,bool focus=false) {
    const auto base=paintRole(ControlBackground);
    const auto accent=paintRole(SliderTrack);
    const auto outline=paintRole(RowOutline);
    const auto subtle=FxTheme::isLightTheme()?0.025f:0.095f;
    ColourGradient grad(base.interpolatedWith(accent,focus?subtle*1.5f:subtle*.52f),
                        bounds.getX(),bounds.getY(),
                        base.darker(FxTheme::isLightTheme()?0.015f:.12f),
                        bounds.getRight(),bounds.getBottom(),false);
    grad.addColour(.44f,base);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds,focus?23.0f:17.0f);
    g.setColour(outline.withAlpha(.84f));
    g.drawRoundedRectangle(bounds.reduced(.5f),focus?23.0f:17.0f,1.0f);
}
inline void progress(Graphics& g,int x,int y,int width,float amount,Colour accent) {
    const auto back=paintRole(RowOutline);
    g.setColour(back.withAlpha(.72f));
    g.fillRoundedRectangle((float)x,(float)y,(float)width,4.0f,2.0f);
    const float active=std::clamp(amount,0.0f,1.0f)*(float)width;
    if(active>0.6f) {
        g.setColour(accent.withAlpha(.92f));
        g.fillRoundedRectangle((float)x,(float)y,active,4.0f,2.0f);
    }
}
inline void draw(Graphics& g,const std::array<float,96>& samples,
                 size_t count,size_t cursor,const CynAudioScene& scene,
                 bool sceneReady,float drop,bool processing,bool enabled,
                 const std::array<float,31>& eq,int eqCount) {
    const auto ink=paintRole(DefaultText),soft=paintRole(HintText);
    const auto accent=paintRole(SliderTrack),highlight=paintRole(SliderHighlight);
    const auto positive=paintRole(ValidTextBorder);
    const float level=count?std::clamp(samples[(cursor+samples.size()-1)%samples.size()],0.0f,1.0f):0.0f;
    const float rise=std::clamp(drop,0.0f,1.0f);

    // Large central stage + narrow intelligence rail + full-width effect dock.
    surface(g,{250.0f,152.0f,694.0f,284.0f},true);
    surface(g,{957.0f,152.0f,250.0f,195.0f});
    surface(g,{957.0f,357.0f,250.0f,245.0f});
    surface(g,{250.0f,447.0f,694.0f,155.0f});

    words(g,"LIVE STAGE / CYN",271,166,255,16,10,accent,true);
    words(g,"Feel the entire song.",271,184,280,24,19,ink,true);
    words(g,processing?"LIVE INPUT / MEASURED":"NO AUDIO SIGNAL",271,215,220,16,10,processing?positive:soft,true);

    // A radial sound stage. Bars and core intensity correspond to actual
    // RMS history, not generated beats, inferred amplitude or fake classifications.
    const float cx=592.0f,cy=284.0f,radius=73.0f;
    ColourGradient aura(accent.withAlpha(.20f+.16f*level),cx,cy,
                        Colours::transparentBlack,cx+152.0f,cy+123.0f,true);
    g.setGradientFill(aura);
    g.fillEllipse(cx-135.0f,cy-135.0f,270.0f,270.0f);
    g.setColour(accent.withAlpha(.22f));
    g.drawEllipse(cx-radius-18.0f,cy-radius-18.0f,(radius+18.0f)*2,(radius+18.0f)*2,1.1f);
    g.setColour(highlight.withAlpha(.32f));
    g.drawEllipse(cx-radius+5.0f,cy-radius+5.0f,(radius-5.0f)*2,(radius-5.0f)*2,2.0f);

    constexpr size_t spokeCount=72;
    const size_t have=(std::min)(count,spokeCount);
    for(size_t i=0;i<spokeCount;++i) {
        float sample=0.0f;
        if(i<have) {
            const auto slot=(cursor+samples.size()-have+i)%samples.size();
            sample=std::clamp(samples[slot],0.0f,1.0f);
        }
        const float length=5.0f+21.0f*std::sqrt(sample);
        const float angle=juce::MathConstants<float>::twoPi*(float)i/(float)spokeCount;
        Graphics::ScopedSaveState scoped(g);
        g.addTransform(AffineTransform::rotation(angle,cx,cy));
        g.setColour(accent.interpolatedWith(highlight,(float)i/spokeCount)
                       .withAlpha(processing?.84f:.16f));
        g.fillRoundedRectangle(cx-2.3f,cy-94.0f-length,4.6f,length,2.1f);
    }

    g.setColour(paintRole(PanelBackground).withAlpha(.93f));
    g.fillEllipse(cx-70,cy-70,140,140);
    g.setColour(paintRole(RowOutline));
    g.drawEllipse(cx-70,cy-70,140,140,1.0f);
    // This is an input-derived display scale, not a measured dB or LUFS value.
    words(g,processing?String(roundToInt(level*100.0f))+"%":"--",
          555,261,90,37,32,ink,true);
    words(g,"INPUT ENERGY",554,300,97,17,10,soft,true);
    words(g,"LOCAL DSP",562,322,70,13,9,processing?positive:soft,true);

    // Stage mini-console on either side of the listening core.
    words(g,"MUSIC MOMENT",272,258,170,15,10,soft,true);
    words(g,sceneReady?"MUSIC  "+String(roundToInt(std::clamp(scene.music,0.0f,1.0f)*100.0f))+"%":"AI WAITING",
          272,279,160,22,13,ink,true);
    progress(g,273,307,112,sceneReady?scene.music:0,accent);
    words(g,sceneReady?"Percussion "+String(roundToInt(std::clamp(scene.percussion,0.0f,1.0f)*100.0f))+"%":
          "No live rhythm",272,321,174,16,10,soft);

    words(g,"THE EMOTIONAL ARC",761,254,168,17,10,soft,true);
    words(g,sceneReady?String(roundToInt(rise*100.0f))+"%":"--",761,278,130,36,30,ink,true);
    words(g,"MEASURED RISE",762,316,166,15,10,soft);
    progress(g,763,338,149,rise,highlight);

    // A small grounded strip timeline provides temporal context below the orb.
    words(g,"RECENT AUDIO",271,387,128,15,10,soft,true);
    const size_t shown=(std::min)((size_t)25,count);
    for(size_t i=0;i<25;++i) {
        float sample=0.0f;
        if(i<shown)sample=std::clamp(samples[(cursor+samples.size()-shown+i)%samples.size()],0.0f,1.0f);
        const float bar=2.0f+17.0f*std::sqrt(sample);
        g.setColour(accent.withAlpha(processing?.68f:.16f));
        g.fillRoundedRectangle(272.0f+(float)i*8.2f,411.0f-bar*.5f,4.7f,bar,2.0f);
    }
    words(g,"QUIET -> STRONG",272,420,176,13,9,soft);
    words(g,"Waveform follows measured audio",481,420,215,13,9,soft);
    // Native Smart Listening toggle sits at x720,y392 and remains interactive.

    // Narrow AI telemetry console. No invented "AI thinking" percentages.
    words(g,"AI / ACTIVE ANALYSIS",976,167,220,17,10,accent,true);
    words(g,sceneReady?"Listening to the mix":"Waiting for AI",976,189,220,24,15,ink,true);
    const float signals[3]={
        sceneReady?std::clamp(scene.music,0.0f,1.0f):0.0f,
        sceneReady?std::clamp(scene.percussion,0.0f,1.0f):0.0f,
        sceneReady?std::clamp(scene.speech,0.0f,1.0f):0.0f
    };
    const char* names[3]={"MUSIC","PERCUSSION","VOICE"};
    for(int i=0;i<3;++i) {
        const int y=224+i*35;
        words(g,names[i],976,y,120,14,9,soft,true);
        words(g,sceneReady?String(roundToInt(signals[i]*100))+"%":"--",
              1156,y,32,14,10,ink,true);
        progress(g,977,y+17,211,signals[i],i==2?highlight:accent);
    }
    words(g,enabled?"SMART LISTENING ENABLED":"MANUAL PROCESSING",
          976,326,220,15,9,enabled?positive:soft,true);

    // Vertical sidecar for the true applied EQ curve; editing opens a larger page.
    words(g,"LIVE / FREQUENCY RESPONSE",976,370,220,16,10,accent,true);
    words(g,"Real applied equalizer",976,391,220,21,14,ink,true);
    const float x0=977.0f,y0=474.0f,w=208.0f;
    g.setColour(paintRole(RowOutline).withAlpha(.72f));
    for(int i=-1;i<=1;++i)g.drawLine(x0,y0+i*27.0f,x0+w,y0+i*27.0f,0.85f);
    if(eqCount>0) {
        Path curve;
        for(int i=0;i<eqCount && i<(int)eq.size();++i) {
            const float x=x0+w*(float)i/(float)jmax(1,eqCount-1);
            const float y=y0-jlimit(-12.0f,12.0f,eq[(size_t)i])*2.05f;
            if(i==0)curve.startNewSubPath(x,y); else curve.lineTo(x,y);
        }
        g.setColour(accent.withAlpha(.98f));
        g.strokePath(curve,PathStrokeType(2.3f,PathStrokeType::curved,PathStrokeType::rounded));
    }
    words(g,"LOW",977,517,63,14,9,soft);
    words(g,"MID",1059,517,51,14,9,soft);
    words(g,"HIGH",1140,517,53,14,9,soft);
    words(g,String(eqCount)+" ACTIVE BANDS",977,554,152,15,9,soft,true);
    // The real Edit full EQ button is x1063,y568 (painted as a child).

    // No text beneath the native five-knob interactive dock.
    words(g,"PERFORMANCE RACK",272,460,360,18,10,accent,true);
    words(g,"Five independent real-DSP effects",272,476,391,18,11,soft);
}
}
