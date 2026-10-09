// Cyn pre-effects analysis. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "InputAudioFeatures.h"
#include <atomic>
#include <cmath>
#include <algorithm>

// Audio thread owns filters/accumulators. Published atomic snapshots are read
// by the UI; no allocation, lock, model inference or audio modification here.
class SourceAudioAnalyzer
{
public:
    SourceAudioAnalyzer() { for (auto& value : published_) value.store(0); }
    void setFormat(int bits, int channels, int rate)
    {
        if (bits != 32 || channels < 1 || channels > 8 || rate < 8000 || rate > 384000)
        { channels = 0; rate = 0; }
        if (channels_ == channels && rate_ == rate) return;
        channels_ = channels; rate_ = rate; clearWindow();
        filters_ = {}; publish({});
        if (!channels_) return;
        static const double centers[8] = {45,75,120,200,400,900,2200,5000};
        for (auto& channel : filters_) for (int b=0;b<8;++b)
        {
            const double frequency = (std::min)(centers[b], rate * 0.4);
            const double w = 6.283185307179586 * frequency / rate;
            const double alpha = std::sin(w) / 1.6;
            auto& f=channel[b];
            f.b0=alpha/(1+alpha); f.b2=-f.b0;
            f.a1=-2*std::cos(w)/(1+alpha); f.a2=(1-alpha)/(1+alpha);
        }
    }
    void process(const float* audio, int frames)
    {
        if (!audio || frames <= 0 || !channels_) return;
        for (int frame=0;frame<frames;++frame)
        {
            for (int c=0;c<channels_;++c)
            {
                double x=audio[frame*channels_+c];
                if (!std::isfinite(x)) x=0;
                x=(std::max)(-4.0,(std::min)(4.0,x));
                full_ += x*x;
                peak_=(std::max)(peak_,std::abs(x));
                for (int b=0;b<8;++b) { double y=filters_[c][b].tick(x); energies_[b]+=y*y; }
            }
            if (channels_ > 1) {
                double l=audio[frame*channels_],r=audio[frame*channels_+1];
                if (std::isfinite(l) && std::isfinite(r)) { stereo_+=(l-r)*(l-r); stereoPower_+=l*l+r*r; }
            }
            if (++count_ >= rate_/20)
            {
                InputAudioFeatures result;
                result.rms=(float)std::sqrt(full_/(count_*channels_));
                for (int b=0;b<8;++b)
                    result.values[b]=(float)std::log(1e-6+energies_[b]/(std::max)(full_,1e-20));
                result.values[8]=(float)std::log((std::max)(1.0,peak_/(std::max)(1e-10,(double)result.rms)));
                result.values[9]=(float)(stereo_/(std::max)(stereoPower_,1e-20));
                result.values[10]=(float)std::log((std::max)(1e-8,(double)result.rms));
                publish(result); clearWindow();
            }
        }
    }
    InputAudioFeatures snapshot() const
    {
        for (int retry=0;retry<3;++retry)
        {
            const auto before=sequence_.load(std::memory_order_acquire);
            if (before&1) continue;
            InputAudioFeatures result;
            for (int b=0;b<11;++b) result.values[b]=published_[b].load(std::memory_order_relaxed);
            result.rms=published_[11].load(std::memory_order_relaxed);
            const auto after=sequence_.load(std::memory_order_acquire);
            if (before==after) { result.sequence=after; return result; }
        }
        return {};
    }
private:
    struct Filter {
        double b0=0,b2=0,a1=0,a2=0,z1=0,z2=0;
        double tick(double x) { double y=b0*x+z1; z1=-a1*y+z2; z2=b2*x-a2*y; return y; }
    };
    void publish(const InputAudioFeatures& values)
    {
        sequence_.fetch_add(1,std::memory_order_acq_rel);
        for (int b=0;b<11;++b) published_[b].store(values.values[b],std::memory_order_relaxed);
        published_[11].store(values.rms,std::memory_order_relaxed);
        sequence_.fetch_add(1,std::memory_order_release);
    }
    static_assert(ATOMIC_INT_LOCK_FREE==2 && sizeof(float)==4,"Lock-free 32-bit audio snapshots required");
    int channels_=0,rate_=0,count_=0;
    void clearWindow() { count_=0; full_=0; peak_=0; stereo_=0; stereoPower_=0; energies_={}; }
    double full_=0,peak_=0,stereo_=0,stereoPower_=0;
    std::array<double,8> energies_ {};
    std::array<std::array<Filter,8>,8> filters_ {};
    std::array<std::atomic<float>,12> published_;
    std::atomic<uint32_t> sequence_ {0};
};
