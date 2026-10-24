// Native CYN expressive navigation motion. Pure presentation math, no JUCE or DSP.
// Inspired by Material 3 expressive interaction patterns; no third-party UI code.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace CynExpressiveMotion {

inline float easeFactor(float elapsedMs, float durationMs) noexcept {
    if (elapsedMs <= 0.0f) return 0.0f;
    if (durationMs <= 0.0f) return 1.0f;
    // Analytic, time-step-independent smoothing; clamped for inactive windows.
    return 1.0f - std::exp(-std::min(elapsedMs, 100.0f) / durationMs);
}

inline float interpolate(float current, float target, float factor) noexcept {
    return current + (target - current) * factor;
}

struct State {
    float navigationY = 104.0f;
    float focusX = 258.0f;
    float focusWidth = 126.0f;
    std::array<float, 6> hovered{};
    bool initialized = false;

    void advance(float elapsedMs, int page, int focus,
                 const std::array<bool, 6>& hoveredTargets,
                 bool reducedMotion) noexcept {
        page = std::max(0, std::min(page, 4));
        focus = std::max(0, std::min(focus, 2));
        const float targetY = 104.0f + 49.0f * static_cast<float>(page);
        const float targetX = focus == 0 ? 258.0f : focus == 1 ? 392.0f : 526.0f;
        const float targetWidth = focus == 2 ? 164.0f : 126.0f;

        // Retarget in-place on rapid clicks: no queued/overlapping tweens.
        const float positionBlend = reducedMotion || !initialized
            ? 1.0f : easeFactor(elapsedMs, 78.0f);
        const float hoverBlend = reducedMotion || !initialized
            ? 1.0f : easeFactor(elapsedMs, 52.0f);

        navigationY = interpolate(navigationY, targetY, positionBlend);
        focusX = interpolate(focusX, targetX, positionBlend);
        focusWidth = interpolate(focusWidth, targetWidth, positionBlend);
        for (std::size_t i = 0; i < hovered.size(); ++i)
            hovered[i] = interpolate(hovered[i], hoveredTargets[i] ? 1.0f : 0.0f, hoverBlend);
        initialized = true;
    }
};
} // namespace CynExpressiveMotion
