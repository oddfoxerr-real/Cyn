// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
// APEX 10M experimental, deterministic stereo ambience preview.
// Streaming DSP only; the 10M spatial reasoning model is NOT trained or run.
// Safe for no-allocation real-time float PCM processing after FxSound DSP.
#include <algorithm>
#include <cmath>

class CynSpatialExperimental final {
public:
    static float clampSample(float value,float low,float high) noexcept {
        return value<low?low:(value>high?high:value);
    }
    void setFormat(int bits, int channels, int rate) noexcept {
        valid_ = bits == 32 && channels == 2 && rate >= 8000 && rate <= 192000;
        if (rate >= 8000) {
            const float rc = 1.0f / (2.0f * 3.14159265f * 220.0f);
            highpass_ = rc / (rc + 1.0f / static_cast<float>(rate));
            smoothing_ = std::fmin(1.0f, 48000.0f / static_cast<float>(rate) * 0.002f);
        }
        reset();
    }
    void reset() noexcept {
        previousSide_ = previousHigh_ = 0.0f;
        leftPower_ = rightPower_ = cross_ = midPower_ = sidePower_ = 0.0f;
        amount_ = 0.0f;
    }
    void process(float* pcm, int frames, float desiredStrength) noexcept {
        if (!valid_ || !pcm || frames <= 0) return;
        const float strength = std::isfinite(desiredStrength)
            ? clampSample(desiredStrength, 0.0f, 1.0f) : 0.0f;
        for (int i = 0; i < frames; ++i) {
            float& left = pcm[i * 2], &right = pcm[i * 2 + 1];
            if (!std::isfinite(left) || !std::isfinite(right)) continue;
            const float l = clampSample(left, -4.0f, 4.0f);
            const float r = clampSample(right, -4.0f, 4.0f);
            const float mid = (l + r) * 0.5f;
            const float side = (l - r) * 0.5f;
            constexpr float alpha = 0.0005f;
            leftPower_ += alpha * (l * l - leftPower_);
            rightPower_ += alpha * (r * r - rightPower_);
            cross_ += alpha * (l * r - cross_);
            midPower_ += alpha * (mid * mid - midPower_);
            sidePower_ += alpha * (side * side - sidePower_);
            const float corr = cross_ / std::sqrt(std::fmax(
                1.0e-24f, leftPower_ * rightPower_));
            const float sideToMid = std::sqrt(sidePower_ / std::fmax(
                1.0e-12f, midPower_));
            previousHigh_ = highpass_ * (previousHigh_ + side - previousSide_);
            previousSide_ = side;
            const bool unsafePhase = corr < -0.3f;
            const float coherence = (leftPower_ + rightPower_ < 1.0e-9f || unsafePhase)
                ? 0.0f : clampSample((corr + 0.3f) / 0.55f, 0.0f, 1.0f);
            const float alreadyWide = clampSample(1.25f - 0.5f * sideToMid, 0.0f, 1.0f);
            amount_ += smoothing_ * (0.20f * strength * coherence * alreadyWide - amount_);
            const float extraSide = unsafePhase ? 0.0f : amount_ * previousHigh_;
            if (std::fabs(extraSide) < 1.0e-9f) continue; // exact DSP bypass
            // Downstream safeguard: gain-reduce both channels together only
            // when our incremental stereo signal would push output above 0.999.
            float a = left + extraSide;
            float b = right - extraSide;
            const float peak = std::fmax(std::fabs(a), std::fabs(b));
            if (peak > 0.999f) {
                const float scale = 0.999f / peak;
                a *= scale;
                b *= scale;
            }
            left = a; right = b;
        }
    }
private:
    bool valid_ = false;
    float highpass_ = 0.97f, smoothing_ = 0.002f;
    float previousSide_ = 0.0f, previousHigh_ = 0.0f;
    float leftPower_ = 0.0f, rightPower_ = 0.0f, cross_ = 0.0f;
    float midPower_ = 0.0f, sidePower_ = 0.0f, amount_ = 0.0f;
};
