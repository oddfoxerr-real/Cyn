// SPDX-License-Identifier: AGPL-3.0-or-later
// Measured musical energy, not a prediction of future drops.
// Control thread only, no allocation or DSP callback access.
#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

class CynCrescendoTracker {
public:
    struct Motion { float build = 0.0f, drop = 0.0f; };
    void reset() noexcept { size_=0; cursor_=0; lastMs_=-1; state_={}; }
    Motion current() const noexcept { return state_; }

    Motion observe(std::int64_t nowMs,float rms,float music,float speech,float silence) noexcept {
        if(nowMs<0 || !std::isfinite(rms) || !std::isfinite(music) ||
           !std::isfinite(speech) || !std::isfinite(silence) ||
           rms<0.004f || music<0.55f || silence>0.55f ||
           (speech>0.80f && music<0.80f)) {
            reset();
            return {};
        }
        if(size_ && nowMs<=lastMs_) return state_;
        if(size_ && nowMs-lastMs_>1500) reset();
        lastMs_=nowMs;
        data_[cursor_]={nowMs,rms};
        cursor_=(cursor_+1)%data_.size();
        size_=(std::min)(size_+1,data_.size());
        float recent=0,earlier=0;
        int nr=0,ne=0;
        for(std::size_t i=0;i<size_;++i) {
            const auto age=nowMs-data_[i].ms;
            if(age>=0 && age<=600) { recent+=data_[i].rms; ++nr; }
            else if(age>=1100 && age<=5500) { earlier+=data_[i].rms; ++ne; }
        }
        if(nr<2 || ne<8) { state_={}; return state_; }
        recent/=static_cast<float>(nr);
        earlier/=static_cast<float>(ne);
        if(recent<0.015f || earlier<0.008f) { state_={}; return state_; }
        const float rise=20.0f*std::log10(recent/earlier);
        state_.build=std::clamp((rise-1.5f)/5.0f,0.0f,1.0f);
        state_.drop=std::clamp((rise-3.8f)/6.0f,0.0f,1.0f);
        return state_;
    }
private:
    struct Sample { std::int64_t ms=0; float rms=0; };
    std::array<Sample,192> data_ {};
    std::size_t size_=0,cursor_=0;
    std::int64_t lastMs_=-1;
    Motion state_ {};
};
