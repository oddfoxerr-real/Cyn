// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include <cmath>
#include "FxTheme.h"
#include "FxController.h"

/**
 * APEX 10M 3D-style brain scan projected as a lightweight native wireframe.
 * Illumination and rotation derive from actual native spatial-output energy.
 * This is not trained-model neuron telemetry; the 10M neural head is inactive.
 */
class CynApexSpatialScan final : public juce::Component, private juce::Timer {
public:
    CynApexSpatialScan(){
        setOpaque(false);
        setInterceptsMouseClicks(false,false);
        setAccessible(false);
    }
    void visibilityChanged() override {
        if(isShowing()&&!FxTheme::isReducedMotion()) startTimerHz(25);
        else stopTimer();
    }
    void paint(juce::Graphics& g) override {
        using namespace juce;
        const auto bounds=getLocalBounds().toFloat();
        const float w=bounds.getWidth(),h=bounds.getHeight();
        if(w<10||h<10)return;
        const float cx=w*.5f,cy=h*.42f;
        const float scale=jmin(w*.37f,h*.46f);
        auto& ctrl=FxController::getInstance();
        const float measured=jlimit(0.0f,1.0f,ctrl.getApex10MSpatialActivity());
        const bool active=ctrl.isApex10MSpatialEnabled() && measured>.006f;
        const float energy=jlimit(0.0f,1.0f,envelope_*2.0f);
        auto accent=Colour(FXCOLOR(SliderTrack));
        const auto text=Colour(FXCOLOR(HintText));
        g.setColour(accent.withAlpha(.10f+.25f*energy));
        g.drawEllipse(cx-scale*1.11f,cy-scale*1.04f,scale*2.22f,scale*2.08f,1.1f);
        for(int hemisphere=-1;hemisphere<=1;hemisphere+=2){
            float oldX[23]{},oldY[23]{};
            for(int row=0;row<=10;++row){
                const float latitude=-MathConstants<float>::halfPi
                    +MathConstants<float>::pi*(float)row/10.0f;
                const float rad=std::cos(latitude);
                float lastX=0,lastY=0;
                for(int col=0;col<=22;++col){
                    const float longitude=MathConstants<float>::pi*(float)col/22.0f;
                    const float fold=.06f*std::sin(longitude*6.0f+latitude*9.0f);
                    const float x=hemisphere*(.14f+rad*(.44f+fold)*std::sin(longitude));
                    const float z=rad*.68f*std::cos(longitude);
                    const float y=std::sin(latitude)*.75f
                        +.04f*std::sin(longitude*9.0f+latitude*7.0f);
                    const float cs=std::cos(angle_),sn=std::sin(angle_);
                    const float depth=x*sn+z*cs;
                    const float k=2.5f/(2.5f+.35f*depth);
                    const float px=cx+(x*cs-z*sn)*scale*k;
                    const float py=cy+y*scale*k;
                    g.setColour(accent.withAlpha(.10f+.46f*energy));
                    if(col>0)g.drawLine(lastX,lastY,px,py,.8f);
                    if(row>0)g.drawLine(oldX[col],oldY[col],px,py,.65f);
                    lastX=oldX[col]=px;lastY=oldY[col]=py;
                }
            }
        }
        if(active){
            const float scanY=cy-scale+2*scale*(.5f+.5f*std::sin(angle_*2));
            g.setColour(accent.withAlpha(.16f+.55f*energy));
            g.drawLine(cx-scale*.98f,scanY,cx+scale*.98f,scanY,1.6f);
        }
        g.setColour(text.withAlpha(1.0f));
        g.setFont(juce::Font(11.0f,juce::Font::bold));
        g.drawText("SPATIAL DSP SCAN  /  "
                   + juce::String(active?"LIVE":"STANDBY")
                   + "    " + juce::String(juce::roundToInt(measured*100.0f)) + "%",
                   8,(int)h-20,(int)w-16,16,juce::Justification::centredLeft);
    }
private:
    void timerCallback() override {
        if(FxTheme::isReducedMotion()) {stopTimer();repaint();return;}
        const double now=juce::Time::getMillisecondCounterHiRes();
        const float dt=last_==0?0.0f:juce::jlimit(0.0f,.060f,(float)((now-last_)/1000.0));
        last_=now;
        const float level=juce::jlimit(0.0f,1.0f,
            FxController::getInstance().getApex10MSpatialActivity());
        envelope_+=(level-envelope_)*juce::jmin(1.0f,dt*8.0f);
        if(level>.006f) angle_+=dt*(.08f+2.2f*envelope_);
        repaint();
    }
    double last_=0;
    float angle_=0,envelope_=0;
};
