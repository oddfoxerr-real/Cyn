/*
 * Cyn DSP control display follower. This is strictly visual: it never changes
 * the DSP, generates artificial oscillations or modifies saved preset values.
 * License: GNU AGPL-3.0-or-later.
 */
#pragma once

#include <cmath>

namespace CynDspUiFollow
{
    inline float towardApplied(float shown, float applied, float elapsedMs, bool animate) noexcept
    {
        if (!std::isfinite(applied)) return shown;
        if (!animate || !std::isfinite(shown)) return applied;

        // Never overshoot or animate without a real DSP change.
        const float difference = applied - shown;
        if (std::abs(difference) <= 0.012f) return applied;

        // Stable across varying UI frame rates; ignore extremely long stalls.
        const float dt = elapsedMs < 0.0f ? 0.0f : (elapsedMs > 90.0f ? 90.0f : elapsedMs);
        const float fraction = 1.0f - std::exp(-dt / 95.0f);
        const float next = shown + fraction * difference;
        return std::abs(applied - next) <= 0.012f ? applied : next;
    }
}
