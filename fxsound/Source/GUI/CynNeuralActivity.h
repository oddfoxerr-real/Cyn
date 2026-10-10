// Cyn measured neural telemetry. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxTheme.h"
#include "PersonalAudioModel.h"

// No animation clock: every node and connection comes from one forward pass.
class CynNeuralActivity : public juce::Component, public juce::SettableTooltipClient
{
public:
    CynNeuralActivity() { setOpaque(false); }
    void setTrainingActive(bool active) { training_=active; }
    void setAnimationEnabled(bool) {} // Telemetry updates are data driven, including reduced motion.
    void setActivity(const NeuralActivitySnapshot* snapshot,float fraction) {
        available_=snapshot!=nullptr;
        data_=snapshot?*snapshot:NeuralActivitySnapshot{};
        progress_=juce::jlimit(0.0f,1.0f,fraction);
        juce::String tip="Measured normalized inputs, tanh activations and weighted contributions. Cyan: positive; amber: negative.\n";
        const char* names[]={"Clarity","Ambience","Surround","Dynamic","Bass"};
        for(int i=0;i<5;++i) tip+=juce::String(names[i])+": "+
            (available_ && data_.ready[i]?juce::String(data_.outputs[i],4):"not trained / no input")+"\n";
        setTooltip(tip); repaint();
    }
    void paint(juce::Graphics& g) override {
        using namespace juce;
        if(getWidth()<64 || getHeight()<90) return;
        const Colour positive(0xff6ad4e5),negative(0xffeeb060);
        const Colour text(Colour(FXCOLOR(HintText)).withAlpha(1.0f));
        g.setColour(Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(),12.0f);
        g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(1.0f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f),12.0f,1.0f);
        g.setColour(text); g.setFont(Font(10.0f,Font::bold));
        g.drawText("NEURAL ACTIVITY",6,7,getWidth()-12,16,Justification::centred);
        g.setFont(9.0f);
        g.drawText(available_?(data_.training?"Training sample":"Audio frame")+String(" #")+String(data_.sequence):"No current input",
                   6,23,getWidth()-12,13,Justification::centred);
        const float left=16.0f,right=(float)getWidth()-16.0f;
        const float y1=48.0f,y3=(float)getHeight()-72.0f,y2=(y1+y3)*0.5f;
        std::array<Point<float>,11> inputs{};
        std::array<Point<float>,12> hidden{};
        std::array<Point<float>,5> outputs{};
        for(int i=0;i<11;++i) inputs[i]={jmap((float)i,0.0f,10.0f,left,right),y1};
        for(int i=0;i<12;++i) hidden[i]={jmap((float)i,0.0f,11.0f,left,right),y2};
        for(int i=0;i<5;++i) outputs[i]={jmap((float)i,0.0f,4.0f,left,right),y3};
        auto connections=[&](const auto& from,const auto& to,const auto& values) {
            for(size_t j=0;j<to.size();++j) for(size_t i=0;i<from.size();++i) {
                const float v=available_?values[j*from.size()+i]:0.0f;
                const float strength=1.0f-std::exp(-std::abs(v));
                g.setColour((v<0?negative:positive).withAlpha(0.03f+strength*0.7f));
                g.drawLine(Line<float>(from[i],to[j]),0.6f+strength);
            }
        };
        connections(inputs,hidden,data_.inputConnections);
        connections(hidden,outputs,data_.outputConnections);
        auto node=[&](Point<float> p,float v,bool ready) {
            const float strength=available_ && ready?jlimit(0.0f,1.0f,std::abs(v)):0.0f;
            g.setColour((v<0?negative:positive).withAlpha(0.15f+strength*0.85f));
            g.fillEllipse(p.x-2.8f,p.y-2.8f,5.6f,5.6f);
        };
        for(int i=0;i<11;++i) node(inputs[i],data_.inputs[i],true);
        for(int i=0;i<12;++i) node(hidden[i],data_.hidden[i],true);
        const char* labels[]={"C","A","S","D","B"};
        for(int i=0;i<5;++i) {
            node(outputs[i],2*data_.outputs[i]-1,data_.ready[i]);
            g.setColour(text); g.setFont(8.5f);
            g.drawText(labels[i],(int)outputs[i].x-12,(int)y3+5,24,12,Justification::centred);
            g.drawText(available_ && data_.ready[i]?String(roundToInt(data_.outputs[i]*100))+"%":"--",
                       (int)outputs[i].x-15,(int)y3+17,30,12,Justification::centred);
        }
        const String status=training_?"TRAINING "+String(roundToInt(progress_*240))+" / 240":
            !available_?"WAITING FOR AUDIO":data_.applied?"ADJUSTMENTS ACTIVE":"PREVIEW / NOT APPLIED";
        g.setColour(positive); g.setFont(Font(9.0f,Font::bold));
        g.drawText(status,4,getHeight()-36,getWidth()-8,15,Justification::centred);
        g.setColour(text); g.setFont(8.5f);
        g.drawText(training_?"Candidate weights / example input":"Outputs: learned preference for more",
                   4,getHeight()-20,getWidth()-8,13,Justification::centred);
    }
private:
    NeuralActivitySnapshot data_{};
    bool available_=false,training_=false;
    float progress_=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynNeuralActivity)
};
