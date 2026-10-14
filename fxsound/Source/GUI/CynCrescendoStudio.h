// SPDX-License-Identifier: AGPL-3.0-or-later
// Crescendo Studio: native graphics driven by actual 100-ms analyzer readings.
#pragma once
#include <JuceHeader.h>
#include "CynFlagshipMixer.h"
#include "FxTheme.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace CynCrescendoStudio {
using namespace juce;

inline Colour paintRole(FxColor role) {
    return Colour(FxTheme::getColor(role)).withAlpha(1.0f);
}
inline void card(Graphics& g,float x,float y,float w,float h) {
    const auto panel=paintRole(ControlBackground);
    const auto outline=paintRole(RowOutline);
    ColourGradient fill(panel.brighter(FxTheme::isLightTheme()?0.025f:0.075f),
                        x,y,panel.darker(FxTheme::isLightTheme()?0.025f:0.085f),
                        x+w,y+h,false);
    fill.addColour(.49f,panel);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(x,y,w,h,15.0f);
    g.setColour(outline.withAlpha(0.86f));
    g.drawRoundedRectangle(x+0.5f,y+0.5f,w-1,h-1,15.0f,1.0f);
}
inline void words(Graphics& g,const String& s,int x,int y,int w,int h,
                  float font,Colour ink,bool bold=false) {
    g.setColour(ink);
    g.setFont(Font(font,bold?Font::bold:Font::plain));
    g.drawFittedText(s,x,y,w,h,Justification::centredLeft,1,0.88f);
}
inline void draw(Graphics& g,const std::array<float,96>& samples,
                 size_t count,size_t cursor,const CynAudioScene& scene,
                 bool sceneReady,float drop,bool processing,bool enabled,
                 const std::array<float,31>& eq,int eqCount) {
    const auto white=paintRole(DefaultText),muted=paintRole(HintText),
               purple=paintRole(SliderTrack),rose=paintRole(SliderHighlight),
               green=paintRole(ValidTextBorder);
    card(g,256,154,612,214); card(g,879,154,329,214);
    card(g,256,382,612,170); card(g,879,382,329,170);
    card(g,256,560,952,45);
    words(g,"DYNAMIC ENERGY TIMELINE",275,166,380,14,10,purple,true);
    words(g,"The moment everything comes alive.",275,183,550,25,17,white,true);
    words(g,processing?"REAL INPUT LEVEL / LIVE":"WAITING FOR PLAYBACK",275,210,400,14,10,muted);
    words(g,enabled?"CRESCENDO AWARE":"MANUAL MODE",731,168,130,18,9,enabled?green:muted,true);
    const float x0=275,y0=274,step=576.0f/76.0f;
    g.setColour(paintRole(RowOutline).withAlpha(.75f));
    g.drawLine(x0,y0,x0+576,y0,1.0f);
    for(size_t i=0;i<76;++i) {
        float value=0;
        if(count>0) {
            const size_t start=count>76?count-76:0;
            if(i+start<count) {
                const size_t slot=(cursor+samples.size()-count+i+start)%samples.size();
                value=std::clamp(samples[slot],0.0f,1.0f);
            }
        }
        const float h=std::clamp(std::sqrt(value)*46.0f,1.0f,46.0f);
        g.setColour(purple.interpolatedWith(rose,(float)i/75.0f).withAlpha(processing?0.95f:0.32f));
        g.fillRoundedRectangle(x0+i*step,y0-h,4.6f,2*h,2.2f);
    }
    words(g,"QUIET",275,323,90,13,9,muted);
    words(g,"BUILDING ENERGY",493,323,180,13,9,muted);
    words(g,drop>0.2f?"IMPACT / DROP":"NATURAL DYNAMICS",736,323,117,13,9,drop>0.2f?rose:muted);
    words(g,"Waveform follows measured audio; no simulated beat animation.",275,344,555,15,10,muted);
    words(g,"INTELLIGENT DYNAMICS",897,166,290,14,10,purple,true);
    words(g,"Music energy response",897,185,290,23,15,white,true);
    const float cx=942,cy=268,r=35;
    g.setColour(paintRole(RowOutline).withAlpha(.92f));g.drawEllipse(cx-r,cy-r,r*2,r*2,8.0f);
    const float amount=std::clamp(drop,0.0f,1.0f);
    Path arc;
    arc.addCentredArc(cx,cy,r,r,0,-2.35f,-2.35f+amount*4.65f,true);
    g.setColour(purple);
    g.strokePath(arc,PathStrokeType(8,PathStrokeType::curved,PathStrokeType::rounded));
    words(g,sceneReady?String(roundToInt(amount*100))+"%":"--",921,251,56,28,22,white,true);
    words(g,"RISE",922,281,50,12,9,muted,true);
    words(g,sceneReady?"LIVE MUSIC SCENE":"WAITING FOR AI",999,223,193,16,10,sceneReady?green:muted,true);
    words(g,sceneReady?"Music "+String(roundToInt(std::clamp(scene.music,0.0f,1.0f)*100))+"%":"No live classification",999,245,195,18,11,muted);
    words(g,sceneReady?"Percussion "+String(roundToInt(std::clamp(scene.percussion,0.0f,1.0f)*100))+"%":"No rhythm estimate",999,266,195,18,11,muted);
    words(g,enabled?"Adaptive processing":"Manual presets",999,287,190,18,10,muted);
    g.setColour(paintRole(RowOutline).withAlpha(.92f));g.fillRoundedRectangle(898,347,289,4,2);
    g.setColour(purple);g.fillRoundedRectangle(898,347,289*amount,4,2);
    // Read-only compact preview of applied EQ gains, not a compressed 31-band editor.
    words(g,"ADAPTIVE FREQUENCY RESPONSE",897,389,287,14,10,purple,true);
    words(g,"LIVE EQ / ACTUAL DSP",897,407,282,18,13,white,true);
    const float x0Eq=902.0f,y0Eq=477.0f,wEq=277.0f;
    g.setColour(paintRole(RowOutline).withAlpha(.6f));
    for(int i=0;i<3;++i) {
        const float y=451.0f+24.0f*i;
        g.drawLine(x0Eq,y,x0Eq+wEq,y,0.8f);
    }
    if(eqCount>0) {
        Path curve;
        for(int i=0;i<eqCount && i<(int)eq.size();++i) {
            const float x=x0Eq+wEq*(float)i/(float)juce::jmax(1,eqCount-1);
            const float y=y0Eq-juce::jlimit(-12.0f,12.0f,eq[(size_t)i])*2.0f;
            if(i==0)curve.startNewSubPath(x,y);
            else curve.lineTo(x,y);
        }
        g.setColour(purple.withAlpha(0.90f));
        g.strokePath(curve,PathStrokeType(2.2f,PathStrokeType::curved,PathStrokeType::rounded));
    }
    words(g,String(eqCount)+" bands",901,521,125,18,11,muted);
    words(g,"LET THE MUSIC BREATHE",274,564,390,17,11,white,true);
    words(g,"Your preset + real-time response; no volume override.",274,584,650,14,10,muted);
}
}
