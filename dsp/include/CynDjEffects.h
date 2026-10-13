// Cyn optional AI DJ. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
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
        if(version!=seenScene_) { seenScene_=version; lease_=int64_t(rate_)*12; }
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
            const float mono=std::isfinite(l)&&std::isfinite(r)?(l+r)*0.5f:0.0f;
            const float level=std::abs(mono);
            fast_+=fastCoef*(level-fast_);
            slow_+=slowCoef*(level-slow_);
            const bool onset=fast_>slow_*1.65f+0.003f && fast_>0.012f;
            if(lease_>0) --lease_;
            if(voiceLease_>0) --voiceLease_;
            ++elapsed_;
            if(authorised && lease_>0 && voiceLease_>0 && amount>0 && onset && !previousOnset_ && elapsed_>cooldown && remaining_==0) {
                kind_=(kind_+1)%3; remaining_=int(rate_*0.7); total_=remaining_;
                elapsed_=0; anchor_=bounded(slow_*0.35f,0.001f,0.045f);
                events_.fetch_add(1); lastEffect_.store(kind_+1);
            }
            previousOnset_=onset;
            const int delay=int(rate_*0.19);
            const float delayed=delay_[size_t((write_+delay_.size()-delay)%delay_.size())];
            delay_[write_]=mono; write_=(write_+1)%delay_.size();
            const float target=authorised&&lease_>0&&voiceLease_>0&&amount>0&&fast_>0.001f?1.0f:0.0f;
            gate_+=gateCoef*(target-gate_);
            if(remaining_>0) {
                const float t=1.0f-float(remaining_)/total_;
                const float envelope=std::sin(3.14159265f*t)*std::sin(3.14159265f*t);
                random_^=random_<<13; random_^=random_>>17; random_^=random_<<5;
                const float noise=float(random_&65535)/32767.5f-1.0f;
                noiseLow_+=0.12f*(noise-noiseLow_);
                phase_+=6.28318530718*(kind_==2?900.0+1400.0*t:170.0)/rate_;
                float wet=kind_==0?(noise-noiseLow_)*anchor_:
                          kind_==1?bounded(delayed*0.14f,-0.045f,0.045f):float(std::sin(phase_))*anchor_*0.45f;
                wet*=envelope*amount*gate_;
                float pan=stereo?(events_.load()%2?t:1.0f-t):0.5f;
                const float wl=wet*std::sqrt(1.0f-pan),wr=wet*std::sqrt(pan);
                // Limit only the added signal to available headroom; never
                // clip/normalise the dry signal or move its stereo image.
                if(std::isfinite(l)) sample[0]=l+bounded(wl,-(std::max)(0.0f,0.98f+l),(std::max)(0.0f,0.98f-l));
                if(std::isfinite(r)) sample[1]=r+bounded(wr,-(std::max)(0.0f,0.98f+r),(std::max)(0.0f,0.98f-r));
                --remaining_;
            }
        }
    }
private:
    static float bounded(float x,float lo,float hi) noexcept {return (std::max)(lo,(std::min)(hi,x));}
    void reset() noexcept { remaining_=0; elapsed_=0; lease_=voiceLease_=0; fast_=slow_=gate_=0; previousOnset_=false; write_=0; delay_.fill(0); }
    std::atomic<bool> enabled_{false},power_{true},allowed_{false},stereo_{true};
    std::atomic<float> intensity_{0.25f},interval_{18.0f};
    std::atomic<uint32_t> sceneVersion_{0},events_{0};
    std::atomic<int> lastEffect_{0};
    std::atomic<bool> voiceSafe_{false};std::atomic<uint32_t> voiceVersion_{0};
    std::array<float,38401> delay_{};
    size_t write_=0;
    int bits_=0,channels_=0,rate_=0,remaining_=0,total_=1,kind_=-1;
    int64_t elapsed_=0,lease_=0,voiceLease_=0;
    uint32_t seenScene_=0,seenVoice_=0,random_=0x76abcdef;
    float fast_=0,slow_=0,gate_=0,anchor_=0,noiseLow_=0;
    double phase_=0;
    bool previousOnset_=false,wasOn_=false;
};
