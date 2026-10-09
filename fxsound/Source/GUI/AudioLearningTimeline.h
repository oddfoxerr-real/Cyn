// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxTheme.h"
#include "PersonalAudioModel.h"
class AudioLearningTimeline : public Component
{
public:
    void updateFrames(const std::vector<AudioFrame>& frames) {
        if(!selected_ && !dragging_) { frames_=frames; end_=Time::currentTimeMillis(); }
        repaint();
    }
    void clearSelection() { selected_=false; dragging_=false; repaint(); }
    std::vector<AudioFrame> selection() const {
        std::vector<AudioFrame> result; if(!selected_) return result;
        const auto low=jmin(from_,to_),high=jmax(from_,to_);
        for(auto& frame:frames_) if(frame.id>=low && frame.id<=high) result.push_back(frame);
        return result;
    }
    void paint(Graphics& g) override {
        auto bounds=getLocalBounds().toFloat();
        g.setColour(Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f)); g.fillRoundedRectangle(bounds,16);
        g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(1.0f)); g.drawRoundedRectangle(bounds.reduced(.5f),16,1);
        g.setFont(13); g.setColour(Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        g.drawText(selected_?"Selection paused / label this section, or drag to change it":"RECENT AUDIO / drag across a section to teach Cyn",16,10,getWidth()-32,22,Justification::centredLeft);
        float left=18,right=(float)getWidth()-18,center=82;
        auto xpos=[&](int64_t id){return left+(right-left)*(float)(id-(end_-60000))/60000;};
        if(selected_) {
            float x1=jlimit(left,right,xpos(jmin(from_,to_))),x2=jlimit(left,right,xpos(jmax(from_,to_)));
            g.setColour(Colour(FXCOLOR(SliderTrack)).withAlpha(.16f)); g.fillRoundedRectangle(x1,36,jmax(2.0f,x2-x1),92,5);
        }
        g.setColour(Colour(FXCOLOR(SliderHighlight)).withAlpha(1.0f));
        for(const auto& frame:frames_) {
            float x=xpos(frame.id); if(x<left || x>right) continue;
            float half=jlimit(2.0f,42.0f,70*std::pow(frame.audio.rms,0.45f));
            g.fillRoundedRectangle(x,center-half,1.5f,2*half,.7f);
        }
        g.setFont(11); g.setColour(Colour(FXCOLOR(HintText)).withAlpha(1.0f));
        g.drawText("60 seconds ago",18,132,150,20,Justification::centredLeft);
        g.drawText(selected_?"Frozen selection":"Now",getWidth()-140,132,122,20,Justification::centredRight);
        if(frames_.empty()) g.drawText("Play audio with Cyn processing on to fill this timeline.",60,65,getWidth()-120,32,Justification::centred);
    }
    void mouseDown(const MouseEvent& e) override { if(frames_.empty()) return; dragging_=true; selected_=true; from_=to_=timeAt(e.x); repaint(); }
    void mouseDrag(const MouseEvent& e) override { if(dragging_) { to_=timeAt(e.x); repaint(); } }
    void mouseUp(const MouseEvent& e) override { if(dragging_) to_=timeAt(e.x); dragging_=false; repaint(); }
private:
    int64_t timeAt(int x) const { return end_-60000+(int64_t)(60000*jlimit(0.0f,1.0f,(x-18.0f)/jmax(1.0f,getWidth()-36.0f))); }
    std::vector<AudioFrame> frames_;
    int64_t end_=0,from_=0,to_=0;
    bool selected_=false,dragging_=false;
};
