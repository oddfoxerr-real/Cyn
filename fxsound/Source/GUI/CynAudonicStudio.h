// CYN AUDONIC Studio creative-control policy.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <algorithm>
#include <cmath>

// Native Windows adaptation: signal-driven creative controls mapped to the
// existing FxSound DSP. This is not the Android AUDONIC ONNX student model.
// No allocations, I/O or model inference occur in this policy.
class CynAudonicStudio final
{
public:
    enum Channel { Harmonics, Attack, Width, Colour, Rhythm, Overtones, Count };
    using Controls = std::array<float, Count>;

    static float unit(float value) noexcept
    {
        return std::isfinite(value) ? (std::max)(0.0f, (std::min)(1.0f, value)) : 0.0f;
    }

    static Controls decide(float music, float speech, float percussion,
                           float bassDrum, float upperSpectrum, float headroom,
                           float intensity) noexcept
    {
        music=unit(music);
        speech=unit(speech);
        percussion=unit(percussion);
        bassDrum=unit(bassDrum);
        upperSpectrum=unit(upperSpectrum);
        headroom=unit(headroom);
        intensity=unit(intensity);
        const float vocalGuard=1.0f-0.80f*speech;
        const float gate=intensity*headroom;
        return {
            unit((0.25f+0.52f*upperSpectrum+0.23f*music)*vocalGuard*gate),
            unit((0.25f+0.75f*percussion)*gate),
            unit((0.18f+0.62f*music)*(1.0f-0.50f*speech)*gate),
            unit((0.22f+0.43f*music+0.35f*upperSpectrum)*vocalGuard*gate),
            unit((0.14f+0.55f*percussion+0.31f*bassDrum)*gate),
            unit((0.20f+0.75f*bassDrum)*(1.0f-0.35f*speech)*gate)
        };
    }

    static float effectAdd(int effect, const Controls& controls) noexcept
    {
        switch(effect) {
            case 0: return controls[Harmonics]*1.25f;     // clarity / presence
            case 1: return controls[Colour]*0.70f;       // ambience
            case 2: return controls[Width]*0.95f;        // surround
            case 3: return controls[Attack]*0.75f+
                           controls[Rhythm]*0.40f;       // dynamic boost
            case 4: return controls[Overtones]*0.95f;    // bass
            default: return 0.0f;
        }
    }

    static float eqAdd(float frequency, const Controls& controls) noexcept
    {
        if(!std::isfinite(frequency) || frequency<=0.0f) return 0.0f;
        const auto bell=[frequency](float centre,float width) noexcept {
            const float distance=std::log2(frequency/centre)/width;
            return std::exp(-0.5f*distance*distance);
        };
        // Gentle, peak-limited tonal movement. No per-sample waveshaping.
        return (std::max)(-0.6f,(std::min)(0.85f,
            controls[Overtones]*0.48f*bell(160.0f,1.1f)+
            controls[Harmonics]*0.62f*bell(3500.0f,0.8f)-
            controls[Colour]*0.23f*bell(900.0f,1.3f)));
    }
};
