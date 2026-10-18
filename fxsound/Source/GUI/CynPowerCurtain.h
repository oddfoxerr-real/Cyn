// CYN power standby / circular boot reveal.
// Visual only. Real processing is controlled by FxController::setPowerState.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxTheme.h"
#include <functional>
#include <cmath>

class CynPowerCurtain final : public juce::Component, private juce::Timer
{
    class PowerButton final : public juce::Button
    {
    public:
        PowerButton() : juce::Button("Power on CYN") {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
            setWantsKeyboardFocus(true);
            setTooltip("Start CYN audio enhancement");
        }
        void paintButton(juce::Graphics& g,bool hover,bool pressed) override {
            using namespace juce;
            auto area=getLocalBounds().toFloat().reduced(6.0f);
            const float cx=area.getCentreX(), cy=area.getCentreY();
            const auto accent=Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
            g.setColour(Colours::black.withAlpha(0.75f));
            g.fillEllipse(area);
            g.setColour(accent.withAlpha(hover?0.38f:0.16f));
            g.fillEllipse(area.reduced(7.0f));
            g.setColour(accent.withAlpha(pressed?1.0f:hover?0.96f:0.76f));
            g.drawEllipse(area.reduced(9.0f),hover?2.6f:1.6f);
            Path arc;
            arc.addCentredArc(cx,cy,23.0f,23.0f,0.0f,
                             0.37f,MathConstants<float>::twoPi-0.37f,true);
            g.strokePath(arc,PathStrokeType(5.0f,PathStrokeType::curved,
                                           PathStrokeType::rounded));
            g.fillRoundedRectangle(cx-2.5f,cy-34.0f,5.0f,29.0f,2.0f);
        }
    };

public:
    CynPowerCurtain() {
        setName("CYN powered-off standby");
        setOpaque(false);
        addAndMakeVisible(start_);
        start_.onClick=[this] {
            if(phase_!=Phase::Idle || powered_) return;
            if(onPowerRequested) onPowerRequested();
        };
        setVisible(false);
        setInterceptsMouseClicks(false,false);
    }

    std::function<void()> onPowerRequested;

    void syncPower(bool on) {
        if(!initialized_) {
            initialized_=true;
            powered_=on;
            phase_=Phase::Idle;
            settle();
            return;
        }
        if(on==powered_) return;
        powered_=on;
        if(FxTheme::isReducedMotion()) {
            phase_=Phase::Idle;
            settle();
        } else {
            phase_=on?Phase::Opening:Phase::Closing;
            phaseBegin_=juce::Time::getMillisecondCounterHiRes();
            setVisible(true);
            setInterceptsMouseClicks(true,true);
            start_.setVisible(false);
            startTimerHz(60);
        }
        repaint();
    }

    void resized() override {
        start_.setBounds(getWidth()/2-57,getHeight()/2-81,114,114);
    }

    void paint(juce::Graphics& g) override {
        using namespace juce;
        if(powered_ && phase_==Phase::Idle) return;
        const auto full=getLocalBounds().toFloat();
        const auto center=full.getCentre();
        const auto accent=Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
        const float progress=phase_==Phase::Idle?1.0f:
             jlimit(0.0f,1.0f,(float)((Time::getMillisecondCounterHiRes()-phaseBegin_)/620.0));
        const float eased=progress*progress*(3.0f-2.0f*progress);
        const float maxRadius=std::hypot((float)getWidth(),(float)getHeight())*0.65f;
        if(phase_==Phase::Idle) {
            // The UI is still visible in standby: no fake black-screen crash.
            g.fillAll(Colours::black.withAlpha(0.76f));
        } else {
            // Screen-swallow iris: revealing the real powered-on UI from the
            // centre, or closing back into standby when powered off externally.
            const float opening=phase_==Phase::Opening?eased:1.0f-eased;
            const float radius=jmax(0.001f,maxRadius*opening);
            Path curtain;
            curtain.setUsingNonZeroWinding(false);
            curtain.addRectangle(full);
            curtain.addEllipse(center.x-radius,center.y-radius,
                               radius*2.0f,radius*2.0f);
            g.setColour(Colours::black.withAlpha(0.90f));
            g.fillPath(curtain);
            if(radius<maxRadius*0.98f) {
                g.setColour(accent.withAlpha(0.55f*(1.0f-progress)));
                g.drawEllipse(center.x-radius,center.y-radius,
                              radius*2.0f,radius*2.0f,3.0f);
            }
        }
        if(!powered_ && phase_==Phase::Idle) {
            g.setColour(accent.withAlpha(0.13f));
            g.drawEllipse(center.x-82,center.y-106,164,164,1.5f);
            g.setColour(Colour(FXCOLOR(DefaultText)).withAlpha(0.94f));
            g.setFont(Font(21.0f,Font::bold));
            g.drawText("CYN STANDBY",getWidth()/2-180,
                       getHeight()/2+42,360,32,Justification::centred);
            g.setColour(Colour(FXCOLOR(HintText)).withAlpha(0.95f));
            g.setFont(Font(13.0f));
            g.drawText("Audio enhancement paused  /  Press power to enter",
                       getWidth()/2-255,getHeight()/2+80,
                       510,23,Justification::centred);
        }
    }
private:
    enum class Phase { Idle, Opening, Closing };
    void settle() {
        stopTimer();
        if(powered_) {
            start_.setVisible(false);
            setInterceptsMouseClicks(false,false);
            setVisible(false);
        } else {
            setVisible(true);
            setInterceptsMouseClicks(true,true);
            start_.setVisible(true);
            start_.toFront(false);
        }
        repaint();
    }
    void timerCallback() override {
        if(phase_==Phase::Idle) {stopTimer();return;}
        if(juce::Time::getMillisecondCounterHiRes()-phaseBegin_>=620.0) {
            phase_=Phase::Idle;
            settle();
        } else repaint();
    }

    PowerButton start_;
    bool initialized_=false,powered_=true;
    Phase phase_=Phase::Idle;
    double phaseBegin_=0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynPowerCurtain)
};
