// CYN live 3D BrainScan. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxTheme.h"
#include "PersonalAudioModel.h"
#include "CynAudonicStudio.h"
#include <array>
#include <cmath>

// Art-directed 3D brain silhouette. Its NODE TOPOLOGY is visual; signal
// brightness comes only from live personal-model inference and optional
// six-channel AUDONIC creative DSP targets. No raw audio or GPU dependency.
class CynBrainScan3D final : public juce::Component, private juce::Timer
{
public:
    CynBrainScan3D()
    {
        setInterceptsMouseClicks(false,false);
        setOpaque(false);
        build();
    }
    ~CynBrainScan3D() override { stopTimer(); }

    void setActivity(const NeuralActivitySnapshot* sample,
                     const CynAudonicStudio::Controls* creative,
                     bool audonic)
    {
        active_=sample!=nullptr;
        audonic_=audonic;
        sample_=sample!=nullptr?*sample:NeuralActivitySnapshot {};
        creative_=creative!=nullptr?*creative:CynAudonicStudio::Controls {};
        if(!running_) repaint();
    }
    void setAnimationEnabled(bool enabled)
    {
        const bool run=enabled&&!FxTheme::isReducedMotion();
        if(run==running_)return;
        running_=run;
        if(run) startTimerHz(30);
        else stopTimer();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace juce;
        auto b=getLocalBounds().toFloat().reduced(1.0f);
        if(b.getWidth()<80.0f||b.getHeight()<95.0f)return;
        const Colour surface(FXCOLOR(ControlBackground));
        const Colour stroke(FXCOLOR(RowOutline));
        const Colour accent(FXCOLOR(SliderTrack));
        const Colour muted(FXCOLOR(HintText));
        g.setColour(surface.withAlpha(0.95f));
        g.fillRoundedRectangle(b,13.0f);
        g.setColour(stroke.withAlpha(0.75f));
        g.drawRoundedRectangle(b,13.0f,1.0f);
        const float cx=b.getCentreX(),cy=b.getCentreY()-7.0f;
        const float scale=juce::jmin(b.getWidth()*.39f,b.getHeight()*.40f);
        const float sine=std::sin(angle_),cosine=std::cos(angle_);
        const float tilt=0.10f+0.09f*std::sin(angle_*.65f);
        const float st=std::sin(tilt),ct=std::cos(tilt);
        std::array<Point<float>,nodes> xy {};
        std::array<float,nodes> depth {};
        std::array<float,nodes> intensity {};
        for(int n=0;n<nodes;++n) {
            const auto& p=brain_[(size_t)n];
            const float x=p.x*cosine-p.z*sine;
            const float z=p.x*sine+p.z*cosine;
            const float y=p.y*ct-z*st;
            const float z2=p.y*st+z*ct;
            const float perspective=2.7f/(2.7f+z2*.34f);
            xy[(size_t)n]={cx+x*scale*perspective,cy+y*scale*perspective};
            depth[(size_t)n]=z2;
            const int channel=n%6;
            const float personal=juce::jlimit(0.0f,1.0f,
                 std::abs(sample_.hidden[(size_t)(n%12)])*.7f+
                 std::abs(sample_.outputs[(size_t)(n%5)]-.5f)*.35f);
            const float fx=juce::jlimit(0.0f,1.0f,creative_[(size_t)channel]);
            intensity[(size_t)n]=active_?(audonic_?fx:personal):0.0f;
        }
        const std::array<Colour,6> colours {{
            Colour(0xffff9674),Colour(0xffffcb83),Colour(0xffae99ff),
            Colour(0xff70dbdb),Colour(0xffde93d1),Colour(0xff72ddb5)
        }};
        const auto drawPass=[&](bool front) {
            for(int n=0;n<nodes;++n) {
                const bool forward=depth[(size_t)n]>=0.0f;
                if(forward!=front)continue;
                const float glow=intensity[(size_t)n];
                const Colour c=audonic_?colours[(size_t)(n%6)]:accent;
                for(int k=0;k<3;++k) {
                    const int dst=neighbors_[(size_t)n][(size_t)k];
                    if(dst<0)continue;
                    const float level=juce::jmax(glow,intensity[(size_t)dst]);
                    g.setColour(c.withAlpha((front?.16f:.065f)+level*(front?.61f:.21f)));
                    g.drawLine(Line<float>(xy[(size_t)n],xy[(size_t)dst]),
                               (front?.75f:.45f)+level*.85f);
                }
            }
            for(int n=0;n<nodes;++n) {
                if((depth[(size_t)n]>=0.0f)!=front)continue;
                const float level=intensity[(size_t)n];
                const auto c=audonic_?colours[(size_t)(n%6)]:accent;
                const float radius=(front?2.1f:1.2f)+level*1.4f;
                if(front && level>.08f) {
                    g.setColour(c.withAlpha(.055f+level*.25f));
                    g.fillEllipse(xy[(size_t)n].x-radius*3.0f,
                                  xy[(size_t)n].y-radius*3.0f,radius*6.0f,radius*6.0f);
                }
                g.setColour(c.withAlpha((front?.56f:.21f)+level*(front?.43f:.33f)));
                g.fillEllipse(xy[(size_t)n].x-radius,xy[(size_t)n].y-radius,
                              radius*2.0f,radius*2.0f);
            }
        };
        drawPass(false);
        drawPass(true);
        if(audonic_) {
            // Six real creative policies, never synthesized/faked activity.
            for(int k=0;k<6;++k) {
                const float level=active_?CynAudonicStudio::unit(creative_[(size_t)k]):0.0f;
                const auto c=colours[(size_t)k];
                const float x=b.getX()+17.0f+k*(b.getWidth()-34.0f)/6.0f;
                const float y=b.getBottom()-35.0f;
                g.setColour(c.withAlpha(.18f));g.fillRoundedRectangle(x,y,19,3.0f,1.5f);
                g.setColour(c.withAlpha(.92f));g.fillRoundedRectangle(x,y,19.0f*level,3.0f,1.5f);
                g.setColour(muted);g.setFont(Font(8.0f));
                g.drawText(String(k+1),roundToInt(x),roundToInt(y+4),19,13,
                           Justification::centred,false);
            }
        }
        g.setColour(active_?accent:muted);
        g.setFont(Font(9.0f,Font::bold));
        g.drawText(audonic_?"AUDONIC  /  6 DSP CHANNELS":"BRAINSCAN  /  LIVE INFERENCE",
                   b.getSmallestIntegerContainer().withTrimmedBottom((int)b.getHeight()-20)
                    .reduced(8,0),Justification::centred,false);
        g.setColour(muted);
        g.setFont(Font(8.5f));
        g.drawText(active_?"LIVE SIGNAL":"WAITING FOR AUDIO",
                   8,getHeight()-18,getWidth()-16,15,Justification::centred,false);
    }

private:
    struct Vec3 { float x=0,y=0,z=0; };
    static constexpr int hemisphere=90,nodes=hemisphere*2;
    std::array<Vec3,nodes> brain_ {};
    std::array<std::array<int,3>,nodes> neighbors_ {};
    NeuralActivitySnapshot sample_ {};
    CynAudonicStudio::Controls creative_ {};
    bool active_=false,audonic_=false,running_=false;
    float angle_=0.20f;

    static float dist2(Vec3 a,Vec3 b) noexcept {
        const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
        return x*x+y*y+z*z;
    }
    void build()
    {
        constexpr float phi=2.3999632297f;
        for(int hemisphereId=0;hemisphereId<2;++hemisphereId)
            for(int i=0;i<hemisphere;++i) {
                const float y=-1.0f+2.0f*((float)i+.5f)/hemisphere;
                const float radius=std::sqrt(juce::jmax(0.0f,1.0f-y*y));
                const float a=i*phi+hemisphereId*.29f;
                const float fold=.035f*std::sin(9.0f*a+7.0f*y);
                const float side=hemisphereId==0?-1.0f:1.0f;
                const int n=hemisphereId*hemisphere+i;
                brain_[(size_t)n]={
                    side*(.12f+radius*(.48f+fold)*std::abs(std::sin(a))),
                    .71f*y+.017f*std::sin(a*7.0f),
                    .75f*radius*std::cos(a)
                };
            }
        for(int i=0;i<nodes;++i) {
            const int begin=(i/hemisphere)*hemisphere,end=begin+hemisphere;
            std::array<float,3> distances {{1e9f,1e9f,1e9f}};
            neighbors_[(size_t)i].fill(-1);
            for(int j=begin;j<end;++j) {
                if(i==j)continue;
                const float distance=dist2(brain_[(size_t)i],brain_[(size_t)j]);
                for(int k=0;k<3;++k)
                    if(distance<distances[(size_t)k]) {
                        for(int v=2;v>k;--v) {
                            distances[(size_t)v]=distances[(size_t)(v-1)];
                            neighbors_[(size_t)i][(size_t)v]=neighbors_[(size_t)i][(size_t)(v-1)];
                        }
                        distances[(size_t)k]=distance;
                        neighbors_[(size_t)i][(size_t)k]=j;
                        break;
                    }
            }
        }
    }
    void timerCallback() override
    {
        if(!isShowing())return;
        if(active_)angle_+=0.013f;
        repaint();
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynBrainScan3D)
};
