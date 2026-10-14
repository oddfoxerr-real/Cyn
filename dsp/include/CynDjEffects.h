// Cyn optional AI DJ. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "CynDjPolicy.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

// One audio-thread owner. UI/worker publish only atomics. No heap allocation,
// locks, disk/network I/O or inference in process(). Original stereo stays put.
class CynDjEffects {
public:
    void enable(bool value) noexcept { enabled_.store(value); }
    void power(bool value) noexcept { power_.store(value); }
    void configure(float amount, float interval, bool stereo) noexcept {
        intensity_.store(std::isfinite(amount)?bounded(amount,0.0f,1.0f):0.0f);
        interval_.store(std::isfinite(interval)?bounded(interval,4.0f,45.0f):18.0f);
        stereo_.store(stereo);
    }
    void scene(float music, float speech) noexcept {
        // A new successful inference renews a sample-clock lease; stalled or
        // failed inference cannot leave effects permanently authorised.
        allowed_.store(std::isfinite(music)&&std::isfinite(speech)&&music>=0.35f&&speech<0.25f);
        sceneVersion_.fetch_add(1);
    }
    void invalidate() noexcept { allowed_.store(false); }
    void voiceGate(bool safe) noexcept { voiceSafe_.store(safe);voiceVersion_.fetch_add(1); }
    uint32_t events() const noexcept { return events_.load(); }
    int lastEffect() const noexcept { return lastEffect_.load(); }
    void setFormat(int bits,int channels,int rate) noexcept {
        if(bits==bits_ && channels==channels_ && rate==rate_) return;
        bits_=bits; channels_=channels; rate_=rate;
        reset();
    }
    void process(float* audio,int frames) noexcept {
        if(!audio || frames<=0 || bits_!=32 || channels_!=2 || rate_<16000 || rate_>192000) return;
        const bool on=enabled_.load()&&power_.load();
        if(!on && !wasOn_) return; // exact bypass, including silence
        if(on!=wasOn_) { reset(); wasOn_=on; }
        if(!on) return;
        const auto version=sceneVersion_.load();
        if(version!=seenScene_) { seenScene_=version; lease_=int64_t(rate_)*CynDjPolicy::contextLeaseSeconds; }
        const auto voiceVersion=voiceVersion_.load();
        if(voiceVersion!=seenVoice_) {seenVoice_=voiceVersion;voiceLease_=int64_t(rate_)*3;}
        const float amount=intensity_.load();
        const bool authorised=allowed_.load()&&voiceSafe_.load();
        const bool stereo=stereo_.load();
        const double cooldown=interval_.load()*rate_;
        const float fastCoef=float(1.0-std::exp(-1.0/(rate_*0.006)));
        const float slowCoef=float(1.0-std::exp(-1.0/(rate_*0.20)));
        const float gateCoef=float(1.0-std::exp(-1.0/(rate_*0.012)));
        for(int i=0;i<frames;++i) {
            auto* sample=audio+i*2;
            const float l=sample[0],r=sample[1];
            // Opposite-phase stereo still has energy even if its mono sum cancels.
            const float level=std::isfinite(l)&&std::isfinite(r)?(std::abs(l)+std::abs(r))*0.5f:0.0f;
            fast_+=fastCoef*(level-fast_);
            slow_+=slowCoef*(level-slow_);
            const bool onset=fast_>slow_*1.65f+0.003f && fast_>0.012f;
            if(lease_>0) --lease_;
            if(voiceLease_>0) --voiceLease_;
            ++elapsed_;++onsetAge_;
            const bool transient=onset&&!previousOnset_;
            if(transient) {
                // Approximate tempo from onsets; use 120 BPM until useful
                // transients arrive. This is not an exact beat grid.
                if(onsetAge_>=rate_*0.30 && onsetAge_<=rate_*0.90)
                    beatSamples_=beatSamples_*0.8+onsetAge_*0.2;
                onsetAge_=0;
            }
            const int beat=(std::max)(2,int(beatSamples_));
            const bool moment=slow_>0.008f && (transient || onsetAge_%beat==0);
            if(authorised && lease_>0 && voiceLease_>0 && amount>0 && moment && elapsed_>cooldown && remaining_==0) {
                kind_=(kind_+1)%3; remaining_=beat*2; total_=remaining_;
                slice_=beat/2;loopStart_=(write_+historyFrames-size_t(slice_))%historyFrames;
                effectBeat_=beat;elapsed_=0;filterL_=std::isfinite(l)?l:0;filterR_=std::isfinite(r)?r:0;
                events_.fetch_add(1); lastEffect_.store(kind_+1);
            }
            previousOnset_=onset;
            const float target=authorised&&lease_>0&&voiceLease_>0&&amount>0&&fast_>0.001f?1.0f:0.0f;
            gate_+=gateCoef*(target-gate_);
            if(remaining_>0) {
                const int position=total_-remaining_;
                const float t=float(position)/total_;
                const float envelope=(std::min)(1.0f,(std::min)(position/(rate_*0.02f),remaining_/(rate_*0.03f)));
                float wl=0,wr=0;
                if(kind_==0) {
                    // Repeat the preceding half beat, retaining stereo content.
                    const int offset=position%slice_;
                    const auto a=(loopStart_+size_t(offset))%historyFrames;
                    float repeatL=history_[a*2],repeatR=history_[a*2+1];
                    const int seam=(std::min)(slice_/4,rate_/200);
                    if(offset<seam) {
                        const auto b=(loopStart_+size_t(slice_-seam+offset))%historyFrames;
                        const float blend=float(offset)/seam;
                        repeatL=history_[b*2]*(1-blend)+repeatL*blend;
                        repeatR=history_[b*2+1]*(1-blend)+repeatR*blend;
                    }
                    wl=(repeatL-l)*0.9f;wr=(repeatR-r)*0.9f;
                } else if(kind_==1) {
                    const auto a=(write_+historyFrames-size_t(effectBeat_/2))%historyFrames;
                    const auto b=(write_+historyFrames-size_t(effectBeat_))%historyFrames;
                    wl=history_[a*2]*0.5f+history_[b*2]*0.25f;
                    wr=history_[a*2+1]*0.5f+history_[b*2+1]*0.25f;
                    if(stereo) {wl*=0.7f+0.3f*std::cos(t*6.2831853f);wr*=0.7f-0.3f*std::cos(t*6.2831853f);}
                } else {
                    // Sweep the song's own high frequencies down and back up.
                    const float curve=std::abs(2*t-1);
                    const float cutoff=400.0f+7600.0f*curve*curve;
                    const float coefficient=1.0f-std::exp(-6.2831853f*cutoff/rate_);
                    filterL_+=coefficient*((std::isfinite(l)?l:0)-filterL_);filterR_+=coefficient*((std::isfinite(r)?r:0)-filterR_);
                    wl=(filterL_-l)*0.95f;wr=(filterR_-r)*0.95f;
                }
                const float mix=std::sqrt(amount)*envelope*gate_;
                wl*=mix;wr*=mix;
                // Bound the remix contribution to available output headroom.
                if(std::isfinite(l)) sample[0]=l+bounded(wl,-(std::max)(0.0f,0.98f+l),(std::max)(0.0f,0.98f-l));
                if(std::isfinite(r)) sample[1]=r+bounded(wr,-(std::max)(0.0f,0.98f+r),(std::max)(0.0f,0.98f-r));
                --remaining_;
            }
            history_[write_*2]=std::isfinite(l)?l:0.0f;
            history_[write_*2+1]=std::isfinite(r)?r:0.0f;
            write_=(write_+1)%historyFrames;
        }
    }
private:
    static float bounded(float x,float lo,float hi) noexcept {return (std::max)(lo,(std::min)(hi,x));}
    void reset() noexcept { remaining_=0; elapsed_=onsetAge_=0; lease_=voiceLease_=0; fast_=slow_=gate_=0; previousOnset_=false; write_=0; history_.fill(0); beatSamples_=rate_*0.5; }
    std::atomic<bool> enabled_{false},power_{true},allowed_{false},stereo_{true};
    std::atomic<float> intensity_{0.25f},interval_{18.0f};
    std::atomic<uint32_t> sceneVersion_{0},events_{0};
    std::atomic<int> lastEffect_{0};
    std::atomic<bool> voiceSafe_{false};std::atomic<uint32_t> voiceVersion_{0};
    static constexpr size_t historyFrames=192000*3+1;
    std::array<float,historyFrames*2> history_{};
    size_t write_=0,loopStart_=0;
    int bits_=0,channels_=0,rate_=0,remaining_=0,total_=1,kind_=-1,slice_=1,effectBeat_=1;
    int64_t elapsed_=0,onsetAge_=0,lease_=0,voiceLease_=0;
    uint32_t seenScene_=0,seenVoice_=0;
    float fast_=0,slow_=0,gate_=0,filterL_=0,filterR_=0;
    double beatSamples_=24000;
    bool previousOnset_=false,wasOn_=false;
};
