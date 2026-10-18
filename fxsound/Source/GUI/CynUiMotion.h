// Fast, allocation-free UI transition math for Cyn. Presentation only.
// Independent of DSP, OS timing and frame rate for low-end PCs.
#pragma once

namespace CynUiMotion {
constexpr double pageDurationMs = 405.0;
constexpr double themeLiquidDurationMs = 450.0;
constexpr double themeCarouselDurationMs = 240.0;
constexpr float pageTravelPixels = 16.0f;

inline float progress(double elapsedMs, double durationMs) noexcept {
    if (durationMs <= 0.0 || elapsedMs >= durationMs) return 1.0f;
    if (elapsedMs <= 0.0) return 0.0f;
    return static_cast<float>(elapsedMs / durationMs);
}

inline float easeOutCubic(float t) noexcept {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    const float remaining = 1.0f-t;
    return 1.0f-remaining*remaining*remaining;
}

inline float outgoingOpacity(float t) noexcept {
    const float left = 1.0f-easeOutCubic(t);
    return left*left;
}

inline float outgoingX(float t, int direction) noexcept {
    return pageTravelPixels*easeOutCubic(t)*(direction >= 0 ? -1.0f : 1.0f);
}
} // namespace CynUiMotion
