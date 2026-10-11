// Cyn live neural telemetry visualizer. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxTheme.h"
#include "PersonalAudioModel.h"
#include <array>
#include <cmath>

// A faithful visualization of Cyn's 11 -> 12 -> 5 inference topology.
// Each node tracks the actual normalized input, tanh hidden activation, or
// sigmoid output. Connection brightness tracks the actual weighted contribution.
// The moving dots indicate signal flow, NOT individual weight changes.
// No separate audio capture, training loop, file I/O or real-time DSP work here.
class CynNeuralActivity : public juce::Component, private juce::Timer
{
public:
    CynNeuralActivity() {
        setInterceptsMouseClicks(false,false);
        setOpaque(false);
    }
    ~CynNeuralActivity() override { stopTimer(); }

    void setTrainingActive(bool active) {
        if (training_ == active) return;
        if (training_ && !active)
            completionUntil_ = juce::Time::getMillisecondCounterHiRes() + 1800.0;
        if (active) completionUntil_ = 0.0;
        training_ = active;
        repaint();
    }

    // UI message-thread snapshots only. Invalid/stale input fades to darkness.
    void setActivity(const NeuralActivitySnapshot* snapshot, float epochFraction) {
        hasAudio_ = snapshot != nullptr;
        target_ = snapshot ? *snapshot : NeuralActivitySnapshot {};
        progress_ = juce::jlimit(0.0f,1.0f,epochFraction);
        if (!animating_) { displayed_ = target_; signal_ = hasAudio_ ? 1.0f : 0.0f; }
        repaint();
    }

    void setAnimationEnabled(bool enabled) {
        const bool shouldRun = enabled && !FxTheme::isReducedMotion();
        if (shouldRun == animating_) return;
        animating_ = shouldRun;
        if (animating_) startTimerHz(24);
        else stopTimer();
        repaint();
    }

    void paint(juce::Graphics& g) override {
        using namespace juce;
        if (getWidth() < 64 || getHeight() < 90) return;
        const bool neuralCore=FxTheme::getThemeMode()==FxThemeMode::NeuralCore;
        const Colour accent(neuralCore?0xff67edee:0xffeeb060),
                     bright(neuralCore?0xffc2ffff:0xffffdd9b);
        const Colour text(Colour(FXCOLOR(HintText)).withAlpha(1.0f));
        const Colour border(Colour(FXCOLOR(RowOutline)).withAlpha(1.0f));
        const Colour bg(Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f));
        const auto bounds = getLocalBounds().toFloat();
        ColourGradient surface(bg.brighter(neuralCore?0.14f:0.06f),0.0f,0.0f,
                               bg.darker(0.13f),(float)getWidth(),(float)getHeight(),false);
        g.setGradientFill(surface); g.fillRoundedRectangle(bounds,12.0f);
        g.setColour(border.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds.reduced(0.5f),12.0f,1.0f);

        g.setColour(text);
        g.setFont(Font(10.0f,Font::bold));
        g.drawText(neuralCore?"PERSONAL NEURAL NETWORK":"LIVE NEURAL ACTIVITY",
                   5,9,getWidth()-10,16,
                   Justification::centred, false);

        const float x1=16.0f,x2=(float)getWidth()-16.0f;
        const float y1=47.0f,y3=(float)getHeight()-57.0f;
        const float y2=y1+(y3-y1)*0.51f;
        std::array<Point<float>, 11> inputs {};
        std::array<Point<float>, 12> hidden {};
        std::array<Point<float>, 5> outputs {};
        for(int i=0;i<11;++i)
            inputs[(size_t)i] = { jmap((float)i,0.0f,10.0f,x1,x2),y1 };
        for(int i=0;i<12;++i)
            hidden[(size_t)i] = { jmap((float)i,0.0f,11.0f,x1+3.0f,x2-3.0f),y2 };
        for(int i=0;i<5;++i)
            outputs[(size_t)i] = { jmap((float)i,0.0f,4.0f,x1+15.0f,x2-15.0f),y3 };

        const float timeSeconds = animating_
            ? (float)(Time::getMillisecondCounterHiRes()*0.001) : 0.0f;
        const float trainingGlow = training_ ? 0.14f+0.14f*progress_ : 0.0f;

        // All 192 physical neural connections; their contribution is the
        // current activation times the trained weight for that connection.
        auto drawConnections = [&](const auto& from, const auto& to,
                                   const auto& contributions, int fromCount,
                                   bool second) {
            for (size_t j=0;j<to.size();++j) {
                for (size_t i=0;i<from.size();++i) {
                    const size_t index=j*(size_t)fromCount+i;
                    const float influence = std::abs(contributions[index]);
                    const float activity = signal_ *
                        (1.0f-std::exp(-influence*(second?0.50f:0.80f)));
                    const float lum = juce::jlimit(0.0f,1.0f,activity+trainingGlow);
                    g.setColour(accent.withAlpha(0.038f+lum*0.54f));
                    g.drawLine(Line<float>(from[i],to[j]),0.75f+lum*0.7f);

                    // Draw moving particles only for connections currently
                    // carrying meaningful inference signal (or local training).
                    if (animating_ && lum>0.26f && (index%7==0 || lum>0.75f)) {
                        const float speed=0.34f+lum*0.9f+(training_?0.6f:0.0f);
                        const float pos=std::fmod(timeSeconds*speed+
                            (float)((index*19)%37)/37.0f,1.0f);
                        const auto dot=from[i]+(to[j]-from[i])*pos;
                        g.setColour(accent.withAlpha(0.22f*lum));
                        g.fillEllipse(dot.x-4.5f,dot.y-4.5f,9.0f,9.0f);
                        g.setColour(bright.withAlpha(0.92f*lum));
                        g.fillEllipse(dot.x-1.65f,dot.y-1.65f,3.3f,3.3f);
                    }
                }
            }
        };
        drawConnections(inputs,hidden,displayed_.inputConnections,11,false);
        drawConnections(hidden,outputs,displayed_.outputConnections,12,true);

        auto drawNode=[&](Point<float> p,float activation,bool ready) {
            const float strength=juce::jlimit(0.0f,1.0f,
                (ready?activation:activation*0.35f)*signal_ + trainingGlow);
            g.setColour(accent.withAlpha(0.06f+strength*0.29f));
            g.fillEllipse(p.x-5.5f,p.y-5.5f,11.0f,11.0f);
            g.setColour(accent.withAlpha(0.30f+strength*0.68f));
            g.fillEllipse(p.x-2.9f,p.y-2.9f,5.8f,5.8f);
            g.setColour(bright.withAlpha(0.36f+strength*0.60f));
            g.fillEllipse(p.x-1.1f,p.y-1.1f,2.2f,2.2f);
        };

        for(size_t i=0;i<inputs.size();++i)
            drawNode(inputs[i],std::abs(displayed_.inputs[i]),true);
        for(size_t i=0;i<hidden.size();++i)
            drawNode(hidden[i],std::abs(displayed_.hidden[i]),true);
        for(size_t i=0;i<outputs.size();++i)
            drawNode(outputs[i],std::abs(2.0f*displayed_.outputs[i]-1.0f),
                     displayed_.ready[i]);

        const bool finished=!training_ &&
            Time::getMillisecondCounterHiRes()<completionUntil_;
        const String status=training_
            ? "LEARNING " + String(juce::roundToInt(progress_*100.0f)) + "%"
            : finished ? "TRAINING COMPLETE"
            : hasAudio_ ? "PROCESSING AUDIO" : "WAITING FOR AUDIO";
        g.setColour((training_||hasAudio_||finished?accent:text).withAlpha(1.0f));
        g.setFont(Font(9.5f,Font::bold));
        g.drawText(status,6,getHeight()-41,getWidth()-12,16,
                   Justification::centred,false);

        if(training_) {
            const float width=(float)getWidth()-24.0f;
            g.setColour(accent.withAlpha(0.17f));
            g.fillRoundedRectangle(12.0f,(float)getHeight()-20,width,4.0f,2.0f);
            g.setColour(bright.withAlpha(0.95f));
            g.fillRoundedRectangle(12.0f,(float)getHeight()-20,
                                   width*progress_,4.0f,2.0f);
        } else {
            g.setColour(text.withAlpha(0.9f)); g.setFont(Font(9.0f));
            g.drawText("11 INPUT  /  12 HIDDEN  /  5 OUTPUT",
                       3,getHeight()-23,getWidth()-6,15,
                       Justification::centred,false);
        }
    }

private:
    void timerCallback() override {
        if(!isShowing()) return;
        auto smooth=[](auto& value,const auto& wanted) {
            for(size_t i=0;i<value.size();++i)
                value[i]+=(wanted[i]-value[i])*0.24f;
        };
        smooth(displayed_.inputs,target_.inputs);
        smooth(displayed_.hidden,target_.hidden);
        smooth(displayed_.outputs,target_.outputs);
        smooth(displayed_.inputConnections,target_.inputConnections);
        smooth(displayed_.outputConnections,target_.outputConnections);
        displayed_.ready=target_.ready;
        signal_+=((hasAudio_?1.0f:0.0f)-signal_)*0.24f;
        repaint();
    }
    NeuralActivitySnapshot target_ {},displayed_ {};
    bool training_=false,hasAudio_=false,animating_=false;
    float signal_=0.0f,progress_=0.0f;
    double completionUntil_=0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynNeuralActivity)
};
