// Spectral patterns and crest factor are approximately invariant under
// upstream gain changes. The raw RMS is NEVER overwritten in the audio tap;
// it still protects peaks and allows true silence detection. Only the learned
// music-content classifier gets a canonical loudness input.
#pragma once
#include "InputAudioFeatures.h"
#include <cmath>
struct CynLevelInvariant {
    static InputAudioFeatures forRecognition(const InputAudioFeatures& raw) noexcept {
        InputAudioFeatures cleaned=raw;
        if(std::isfinite(raw.rms) && raw.rms>0.0001f)
            cleaned.values[10]=std::log(0.12f);
        return cleaned;
    }
};
