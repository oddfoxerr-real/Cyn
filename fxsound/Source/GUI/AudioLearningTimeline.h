// CYN native learning timeline: rolling five-minute history, effect-colored
// labels, and horizontal time navigation. Display only; no DSP mutations.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxTheme.h"
#include "PersonalAudioModel.h"
#include <algorithm>
#include <cmath>
class AudioLearningTimeline : public juce::Component
{
public:
    void updateFrames(const std::vector<AudioFrame>& incoming) {
        // The controller supplies recent audio repeatedly; append only new
        // frame IDs to preserve history for actual wheel-based navigation.
        for(const auto& frame:incoming)
            if(frames_.empty() || frame.id>frames_.back().id)
                frames_.push_back(frame);
        if(!frames_.empty()) {
            end_=juce::jmax(end_,frames_.back().id);
            const int64_t oldest=end_-historyMs;
            const auto first=std::lower_bound(frames_.begin(),frames_.end(),oldest,
                [](const AudioFrame& frame,int64_t id){return frame.id<id;});
            frames_.erase(frames_.begin(),first);
            labels_.erase(std::remove_if(labels_.begin(),labels_.end(),
                [oldest](const Mark& x){return x.end<oldest;}),labels_.end());
        }
        repaint();
    }
    void setActiveEffect(int effect) {
        activeEffect_=juce::jlimit(0,4,effect);
        repaint();
    }
    void recordLabeledSelection(int effect,bool more) {
        if(!selected_) return;
        const int64_t from=juce::jmin(from_,to_),to=juce::jmax(from_,to_);
        if(to<=from) return;
        // Only label the exact timestamp interval the user just taught.
        labels_.push_back({from,to,juce::jlimit(0,4,effect),more});
        repaint();
    }
    void clearSelection() {
        selected_=false;
        dragging_=false;
        frozenEnd_=0;
        repaint();
    }
    std::vector<AudioFrame> selection() const {
        std::vector<AudioFrame> result;
        if(!selected_) return result;
        const auto low=juce::jmin(from_,to_),high=juce::jmax(from_,to_);
        for(const auto& frame:frames_)
            if(frame.id>=low && frame.id<=high) result.push_back(frame);
        return result;
    }
    void paint(juce::Graphics& g) override {
        using namespace juce;
        const auto bounds=getLocalBounds().toFloat();
        const auto background=Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f);
        const auto accent=effectColour(activeEffect_);
        g.setColour(background);
        g.fillRoundedRectangle(bounds,16.0f);
        g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(.85f));
        g.drawRoundedRectangle(bounds.reduced(.5f),16.0f,1.0f);
        g.setFont(12.2f);
        g.setColour(Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        g.drawText(selected_?"SELECTED AUDIO / choose More or Less to teach CYN":
            "DRAG TO SELECT / SCROLL WHEEL TO EXPLORE HISTORY",
            16,9,getWidth()-32,21,Justification::centredLeft);

        const float left=18.0f,right=(float)getWidth()-18.0f;
        const float center=82.0f;
        const int64_t visibleEnd=viewportEnd();
        const int64_t visibleStart=visibleEnd-windowMs;
        const auto xpos=[&](int64_t id) {
            return left+(right-left)*(float)(id-visibleStart)/(float)windowMs;
        };
        {
            Graphics::ScopedSaveState clipped(g);
            g.reduceClipRegion(Rectangle<int>(18,34,
                juce::jmax(1,getWidth()-36),95));
            // Labeled ranges stay marked with the respective training
            // category. Multiple categories have distinct, stable hues.
            for(const auto& label:labels_) {
                const float start=jlimit(left,right,xpos(label.start));
                const float finish=jlimit(left,right,xpos(label.end));
                if(finish<=start || label.end<visibleStart ||
                   label.start>visibleEnd) continue;
                const auto color=effectColour(label.effect);
                g.setColour(color.withAlpha(label.more?.19f:.11f));
                g.fillRoundedRectangle(start,36,juce::jmax(2.0f,finish-start),
                                       92,4.0f);
                g.setColour(color.withAlpha(.69f));
                g.fillRect(start,36,juce::jmax(2.0f,finish-start),2.0f);
            }
            if(selected_) {
                const float x1=jlimit(left,right,xpos(juce::jmin(from_,to_)));
                const float x2=jlimit(left,right,xpos(juce::jmax(from_,to_)));
                g.setColour(accent.withAlpha(.25f));
                g.fillRoundedRectangle(x1,36,juce::jmax(2.0f,x2-x1),92,5.0f);
                g.setColour(accent.withAlpha(.90f));
                g.drawRoundedRectangle(x1,36,juce::jmax(2.0f,x2-x1),
                                       92,5.0f,1.0f);
            }
            for(const auto& frame:frames_) {
                if(frame.id<visibleStart || frame.id>visibleEnd) continue;
                const float x=xpos(frame.id);
                const float half=jlimit(2.0f,42.0f,70.0f*
                    std::pow(juce::jmax(0.0f,frame.audio.rms),.45f));
                // Live spectrum shifts gently from cyan toward violet. Any
                // taught span uses the chosen effect's actual label color.
                auto color=Colour(FXCOLOR(SliderTrack))
                    .interpolatedWith(Colour(0xffbd9bff),
                        juce::jlimit(0.0f,1.0f,(x-left)/(right-left)));
                for(auto it=labels_.rbegin();it!=labels_.rend();++it)
                    if(frame.id>=it->start && frame.id<=it->end) {
                        color=effectColour(it->effect); break;
                    }
                g.setColour(color.withAlpha(.88f));
                g.fillRoundedRectangle(x,center-half,1.5f,2*half,.75f);
            }
        }
        g.setFont(10.7f);
        g.setColour(Colour(FXCOLOR(HintText)).withAlpha(1.0f));
        g.drawText("-60 s",18,132,120,20,Justification::centredLeft);
        const String rightCaption=scrollOffsetMs_>0 ?
            String(juce::roundToInt((float)scrollOffsetMs_/1000.0f))+
                " s behind live" : "LIVE";
        g.drawText(rightCaption,getWidth()-180,132,162,20,
                   Justification::centredRight);
        if(frames_.empty())
            g.drawText("Play audio to fill the learning timeline.",
                       60,65,getWidth()-120,32,Justification::centred);
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if(frames_.empty()) return;
        dragging_=true;selected_=true;
        frozenEnd_=end_;
        from_=to_=timeAt(e.x);
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if(dragging_) {to_=timeAt(e.x); repaint();}
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if(dragging_) {to_=timeAt(e.x);dragging_=false;repaint();}
    }
    void mouseWheelMove(const juce::MouseEvent&,
                        const juce::MouseWheelDetails& wheel) override {
        if(frames_.empty()) return;
        const int64_t span=juce::jmax<int64_t>(0,
            end_-frames_.front().id-windowMs);
        const int64_t delta=(int64_t)std::llround(wheel.deltaY*20000.0f);
        scrollOffsetMs_=juce::jlimit<int64_t>(0,span,scrollOffsetMs_+delta);
        repaint();
    }
private:
    static constexpr int64_t windowMs=60000;
    static constexpr int64_t historyMs=360000;
    struct Mark {int64_t start,end;int effect;bool more;};
    static juce::Colour effectColour(int effect) {
        switch(effect) {
            case 0: return juce::Colour(0xff6be0ff); // clarity
            case 1: return juce::Colour(0xffc59bff); // ambience
            case 2: return juce::Colour(0xff71f0c2); // surround
            case 3: return juce::Colour(0xffffbe76); // dynamics
            default:return juce::Colour(0xffff84a0); // bass
        }
    }
    int64_t viewportEnd() const {
        return juce::jmax<int64_t>(0,
            (selected_ && frozenEnd_>0 ? frozenEnd_:end_)-scrollOffsetMs_);
    }
    int64_t timeAt(int x) const {
        const float ratio=juce::jlimit(0.0f,1.0f,
            (x-18.0f)/juce::jmax(1.0f,(float)getWidth()-36.0f));
        return viewportEnd()-windowMs+
            (int64_t)std::llround(windowMs*ratio);
    }
    std::vector<AudioFrame> frames_;
    std::vector<Mark> labels_;
    int64_t end_=0,from_=0,to_=0,frozenEnd_=0,scrollOffsetMs_=0;
    int activeEffect_=0;
    bool selected_=false,dragging_=false;
};
