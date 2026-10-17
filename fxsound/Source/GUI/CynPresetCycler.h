// Cyn preset cycling: real cached preset signatures and stable passage-level
// selection. The live DSP is NEVER reloaded or switched on a musical beat.
#pragma once
#include "CynFlagshipMixer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

struct CynPresetSignature {
    float bass=0.5f, impact=0.5f, clarity=0.5f, space=0.5f, air=0.5f;
};

class CynPresetCycler {
public:
    void reset() noexcept {active_=0;pending_=-1;stable_=0;lastSwitch_=0;}
    int selected() const noexcept {return active_;}
    static CynPresetSignature target(const CynAudioScene& s) noexcept {
        auto bound=[](float a){return std::clamp(a,0.0f,1.0f);};
        return {
            bound(0.24f+0.54f*s.bassDrum+0.22f*s.electronic-0.18f*s.vocals),
            bound(0.22f+0.58f*s.percussion+0.26f*s.impact),
            bound(0.27f+0.55f*s.vocals+0.25f*s.speech),
            bound(0.19f+0.49f*s.soundtrack+0.32f*s.complexity),
            bound(0.28f+0.35f*s.electronic+0.30f*s.novelty)
        };
    }
    static float distance(const CynPresetSignature& a,const CynPresetSignature& b) noexcept {
        const auto sq=[](float x){return x*x;};
        return 1.5f*sq(a.bass-b.bass)+1.2f*sq(a.impact-b.impact)+
            1.0f*sq(a.clarity-b.clarity)+0.7f*sq(a.space-b.space)+
            0.35f*sq(a.air-b.air);
    }
    int update(const std::vector<CynPresetSignature>& choices,
               const CynAudioScene& scene,uint64_t now,bool listening) noexcept {
        if(choices.empty()){reset();return 0;}
        if(active_>=static_cast<int>(choices.size()))reset();
        if(!listening || !scene.valid || scene.music<0.45f ||
           scene.silence>0.55f || (scene.speech>0.75f && scene.music<0.80f)) {
            pending_=-1;stable_=0;return active_;
        }
        const auto desired=target(scene);
        int best=active_;
        float bestScore=distance(desired,choices[static_cast<size_t>(best)]);
        for(size_t i=0;i<choices.size();++i){
            const float score=distance(desired,choices[i]);
            if(score+0.045f<bestScore) {bestScore=score;best=(int)i;}
        }
        if(best==active_){pending_=-1;stable_=0;return active_;}
        if(best!=pending_){pending_=best;stable_=1;}
        else if(stable_<1000)++stable_;
        // 7 consecutive observations (~700ms) and a >=1.8 second hold:
        // fast enough for passages, never flutters at each isolated note.
        if(stable_>=7 && (lastSwitch_==0 || now>=lastSwitch_+1800)){
            active_=best;lastSwitch_=now;pending_=-1;stable_=0;
        }
        return active_;
    }
private:
    int active_=0,pending_=-1,stable_=0;
    uint64_t lastSwitch_=0;
};
