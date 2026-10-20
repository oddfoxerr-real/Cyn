// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

// APEX 10M experimental spatial DSP preview (not neural inference).
// 48 kHz stereo-only: enhance existing high-frequency side energy while
// preserving the center signal, mono sum, and low-frequency positioning.
// This class has no allocation, locks, or library calls in the audio callback.
#include <algorithm>
#include <cmath>

class CynSpatialExperimental {
public:
    static constexpr float sampleRate = 48000.0f;

    void reset() noexcept {
        previousSide_ = 0.0f;
        previousHigh_ = 0.0f;
        powerLeft_ = powerRight_ = crossPower_ = sidePower_ = midPower_ = 0.0f;
        smoothedAmount_ = 0.0f;
    }

    void process(float& left, float& right, float desiredStrength) noexcept {
        if (!std::isfinite(left) || !std::isfinite(right)) return;
        const float strength = std::clamp(
            std::isfinite(desiredStrength) ? desiredStrength : 0.0f,
            0.0f, 1.0f);
        // Input values are bounded by the downstream limiter; saturate
        // analysis for robustness against unbounded intermediate DSP gains.
        const float dryL = std::clamp(left, -4.0f, 4.0f);
        const float dryR = std::clamp(right, -4.0f, 4.0f);
        const float middle = (dryL + dryR) * 0.5f;
        const float side = (dryL - dryR) * 0.5f;

        // ~40 ms running stereo relationship to avoid widening antiphase
        // or already-extra-wide sources. Compute every sample, even in bypass.
        constexpr float smoothing = 0.0005f;
        powerLeft_ += smoothing * (dryL * dryL - powerLeft_);
        powerRight_ += smoothing * (dryR * dryR - powerRight_);
        crossPower_ += smoothing * (dryL * dryR - crossPower_);
        midPower_ += smoothing * (middle * middle - midPower_);
        sidePower_ += smoothing * (side * side - sidePower_);
        const float correlation = crossPower_ /
            std::sqrt(std::max(1.0e-24f, powerLeft_ * powerRight_));
        const float sideToMid = std::sqrt(sidePower_ /
            std::max(midPower_, 1.0e-10f));

        // First-order high pass approx 220 Hz: no bass widening.
        constexpr float cutoff = 220.0f;
        constexpr float rc = 1.0f / (2.0f * 3.14159265f * cutoff);
        constexpr float dt = 1.0f / sampleRate;
        constexpr float highpass = rc / (rc + dt);
        previousHigh_ = highpass * (previousHigh_ + side - previousSide_);
        previousSide_ = side;

        const bool unsafePhase = correlation < -0.3f;
        const float coherence = (powerLeft_ + powerRight_ < 1.0e-9f || unsafePhase)
            ? 0.0f : std::clamp((correlation + 0.3f) / 0.55f, 0.0f, 1.0f);
        const float widthGate = std::clamp(1.25f - 0.5f * sideToMid, 0.0f, 1.0f);
        const float target = 0.2f * strength * coherence * widthGate;
        // ~10 ms smoothing avoids clicks when toggling mid-track.
        smoothedAmount_ += (target - smoothedAmount_) * 0.002f;
        const float addedSide = unsafePhase ? 0.0f : smoothedAmount_ * previousHigh_;
        left += addedSide;
        right -= addedSide;
    }

private:
    float previousSide_ = 0.0f, previousHigh_ = 0.0f;
    float powerLeft_ = 0.0f, powerRight_ = 0.0f, crossPower_ = 0.0f;
    float sidePower_ = 0.0f, midPower_ = 0.0f;
    float smoothedAmount_ = 0.0f;
};
