// Cyn optional AI DJ accent generator. SPDX-License-Identifier: AGPL-3.0-or-later
// The source stream is analyzed only. Effects are added to the processed stereo
// output, never to the input or YAMNet feed. No allocation, locks or I/O.
#pragma once
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>

class CynAiDjEngine final {
public:
    void configure(bool enabled, float intensity) noexcept {
        intensity_.store(std::clamp(std::isfinite(intensity) ? intensity : 0.0f,
                                    0.0f, 1.0f), std::memory_order_relaxed);
        enabled_.store(enabled, std::memory_order_release);
    }

    void setScene(float music, float speech, float percussion, float silence) noexcept {
        auto sanitize=[](float v) noexcept {
            return std::isfinite(v) ? std::clamp(v, 0.0f, 1.0f) : 0.0f;
        };
        music_.store(sanitize(music), std::memory_order_relaxed);
        speech_.store(sanitize(speech), std::memory_order_relaxed);
        percussion_.store(sanitize(percussion), std::memory_order_relaxed);
        silence_.store(sanitize(silence), std::memory_order_relaxed);
    }

    void setFormat(int bits, int channels, int sampleRate) noexcept {
        channels_ = (bits == 32 && channels == 2 && sampleRate >= 8000 &&
                     sampleRate <= 192000) ? 2 : 0;
        rate_ = channels_ ? sampleRate : 0;
        activeRemaining_ = 0;
        elapsedFrames_ = 0;
        sinceAccent_ = 0;
        hasFired_ = false;
        fast_ = slow_ = 0.0f;
        phase_ = 0.0;
    }

    // Reactive accents only. YAMNet cannot identify beat-grid positions or
    // predict drops; absence of a confident musical onset means NO effect.
    void process(const float* source, float* processed, int frames) noexcept {
        if (!source || !processed || frames <= 0 || !rate_ || channels_ != 2 ||
            !enabled_.load(std::memory_order_acquire) ||
            intensity_.load(std::memory_order_relaxed) <= 0.001f) {
            activeRemaining_ = 0;
            return;
        }
        const float music = music_.load(std::memory_order_relaxed);
        const float speech = speech_.load(std::memory_order_relaxed);
        const float percussion = percussion_.load(std::memory_order_relaxed);
        const float silence = silence_.load(std::memory_order_relaxed);
        const bool clearlyMusic = music >= 0.65f && percussion >= 0.15f &&
                                  speech <= 0.14f && silence <= 0.15f;
        if (!clearlyMusic) {
            activeRemaining_ = 0; // no effects over dialogue, games or uncertain scenes
            fast_ = slow_ = 0.0f;
            sinceAccent_ = 0;
            return;
        }

        double square = 0;
        for (int i=0; i<frames; ++i) {
            const float l=source[2*i], r=source[2*i+1];
            if (!std::isfinite(l) || !std::isfinite(r)) {
                activeRemaining_=0;
                return;
            }
            square += 0.5 * (static_cast<double>(l)*l +
                              static_cast<double>(r)*r);
        }
        const float rms = static_cast<float>(std::sqrt(square / frames));
        const float fastAlpha = std::exp(-static_cast<float>(frames)/(rate_*0.025f));
        const float slowAlpha = std::exp(-static_cast<float>(frames)/(rate_*0.45f));
        fast_ = fastAlpha*fast_ + (1.0f-fastAlpha)*rms;
        slow_ = slowAlpha*slow_ + (1.0f-slowAlpha)*rms;
        elapsedFrames_ += static_cast<std::uint64_t>(frames);
        sinceAccent_ = (std::min)(sinceAccent_ + static_cast<std::uint64_t>(frames),
                                  static_cast<std::uint64_t>(rate_)*120);
        const bool onset = fast_ > slow_*1.9f && fast_-slow_ > 0.035f &&
                           rms > 0.09f;
        if (!activeRemaining_ && elapsedFrames_ > static_cast<std::uint64_t>(rate_) &&
            (!hasFired_ || sinceAccent_ > static_cast<std::uint64_t>(rate_)*8) && onset) {
            activeRemaining_ = (std::max)(1,rate_/5); // 200-ms gentle glide
            activeLength_ = activeRemaining_;
            phase_ = 0.0;
            sinceAccent_ = 0;
            hasFired_ = true;
            rightward_ = !rightward_;
        }
        if (!activeRemaining_) return;

        constexpr double pi=3.14159265358979323846;
        const float intensity = intensity_.load(std::memory_order_relaxed);
        for (int i=0; i<frames && activeRemaining_>0; ++i) {
            const float progress = 1.0f - static_cast<float>(activeRemaining_) /
                                                  static_cast<float>(activeLength_);
            const float shape=std::sin(static_cast<float>(pi)*progress);
            const float envelope=shape*shape;
            const float position = rightward_ ? progress : 1.0f-progress;
            const float gain=0.045f*intensity*envelope;
            const float gl=std::cos(static_cast<float>(pi)*0.5f*position);
            const float gr=std::sin(static_cast<float>(pi)*0.5f*position);
            const float chirp=static_cast<float>(std::sin(phase_))*gain;
            phase_ += 2.0*pi*(470.0+560.0*progress)/rate_;
            if (phase_ > 2.0*pi) phase_ -= 2.0*pi;
            const float left=processed[2*i], right=processed[2*i+1];
            // Never change the base mix to create headroom; just mute the accent.
            if (std::isfinite(left) && std::isfinite(right)) {
                const float headroom=std::clamp((0.94f -
                    (std::max)(std::abs(left),std::abs(right)))*7.0f,0.0f,1.0f);
                processed[2*i] = left + chirp*gl*headroom;
                processed[2*i+1] = right + chirp*gr*headroom;
            }
            --activeRemaining_;
        }
    }

private:
    std::atomic<bool> enabled_ {false};
    std::atomic<float> intensity_ {0.25f};
    std::atomic<float> music_ {0.0f}, speech_ {1.0f};
    std::atomic<float> percussion_ {0.0f}, silence_ {0.0f};
    int channels_ = 0, rate_ = 0, activeRemaining_ = 0, activeLength_ = 1;
    std::uint64_t elapsedFrames_ = 0, sinceAccent_ = 0;
    float fast_ = 0, slow_ = 0;
    double phase_ = 0;
    bool rightward_ = false, hasFired_ = false;
};
