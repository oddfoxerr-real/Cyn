// Cyn animated theme atmospheres - UI thread only.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "FxTheme.h"
#include <cmath>

namespace CynThemeAtmosphere
{
    // All geometry is drawn behind the native JUCE controls. No image files,
    // DSP callbacks, capture devices or additional rendering threads required.
    inline void paint(juce::Graphics& g, FxThemeMode mode, float timeSeconds,
                      float audioLevel)
    {
        using namespace juce;
        const float motion = FxTheme::isReducedMotion() ? 0.0f : timeSeconds;
        const float energy = FxTheme::isReducedMotion() ? 0.0f
            : jlimit(0.0f, 1.0f, audioLevel);
        const Colour accent(FXCOLOR(SliderTrack));
        const Colour highlight(FXCOLOR(SliderHighlight));
        const Colour ink(FXCOLOR(DefaultText));
        const bool light = FxTheme::isLightTheme();
        const float a = light ? 0.43f : 0.56f;

        const auto line = [&](float x1, float y1, float x2, float y2,
                              Colour colour, float opacity = 0.3f, float thickness = 1.0f)
        {
            g.setColour(colour.withAlpha(jlimit(0.0f, 1.0f, opacity)));
            g.drawLine(x1, y1, x2, y2, thickness);
        };
        const auto orb = [&](float x, float y, float r, Colour colour, float opacity)
        {
            ColourGradient halo(colour.withAlpha(opacity), x, y,
                                colour.withAlpha(0.0f), x + r, y + r, true);
            g.setGradientFill(halo);
            g.fillEllipse(x - r, y - r, r * 2.0f, r * 2.0f);
        };
        const auto ring = [&](float x, float y, float rx, float ry,
                              Colour colour, float opacity, float thickness = 1.0f)
        {
            g.setColour(colour.withAlpha(opacity));
            g.drawEllipse(x - rx, y - ry, rx * 2.0f, ry * 2.0f, thickness);
        };
        const auto wave = [&](float y, float amplitude, float period, float offset,
                              Colour colour, float opacity, float thickness = 1.0f)
        {
            Path path;
            for (int i = 0; i <= 28; ++i)
            {
                const float x = 79.0f + 4.8f * (float)i;
                const float yy = y + std::sin(x / period + offset) * amplitude;
                if (i == 0) path.startNewSubPath(x, yy);
                else path.lineTo(x, yy);
            }
            g.setColour(colour.withAlpha(opacity));
            g.strokePath(path, PathStrokeType(thickness, PathStrokeType::curved,
                                              PathStrokeType::rounded));
        };

        // Low-opacity atmospheric light fills the workspace. Each preset also
        // gets its own recognizable illustration in the otherwise unused rail.
        orb(1020.0f + 42.0f * std::sin(motion * 0.19f),
            130.0f + 22.0f * std::cos(motion * 0.17f),
            280.0f + energy * 44.0f, accent, light ? 0.035f : 0.055f);
        orb(475.0f, 555.0f, 210.0f, highlight, light ? 0.025f : 0.042f);

        const float cx = 145.0f;
        const float cy = 356.0f;
        const float pulse = 1.0f + energy * 0.32f;
        const float sway = std::sin(motion * 0.65f);
        // A faint framed viewport gives each theme a material, rather than
        // a flat swatch, without competing with the sidebar navigation.
        ColourGradient panel(accent.withAlpha(light ? 0.045f : 0.105f),
                             83.0f, 248.0f,
                             highlight.withAlpha(light ? 0.012f : 0.025f),
                             212.0f, 468.0f, false);
        g.setGradientFill(panel);
        g.fillRoundedRectangle(78.0f, 248.0f, 138.0f, 220.0f, 20.0f);
        g.setColour(accent.withAlpha(light ? 0.09f : 0.15f));
        g.drawRoundedRectangle(78.5f, 248.5f, 137.0f, 219.0f, 20.0f, 1.0f);

        switch (mode)
        {
            case FxThemeMode::Dark: // graphite: milled, intersecting metal arcs
            {
                for (int i = 0; i < 5; ++i)
                    ring(cx, cy, (24.0f + i * 19.0f) * pulse,
                         20.0f + i * 16.0f, accent, (0.48f - i * 0.07f) * a);
                for (int i = 0; i < 7; ++i)
                    line(80.0f, 275.0f + i * 26.0f,
                         216.0f, 335.0f + i * 26.0f, ink, 0.035f);
                break;
            }
            case FxThemeMode::Light: // pearl: softly refracted overlapping drops
            {
                for (int i = 0; i < 4; ++i)
                {
                    const float x = 118.0f + 30.0f * std::sin(motion * 0.18f + i * 1.7f);
                    const float y = 309.0f + i * 34.0f;
                    orb(x, y, 30.0f + i * 5.0f, accent, 0.12f);
                    ring(x, y, 19.0f, 28.0f, accent, 0.14f, 1.5f);
                }
                break;
            }
            case FxThemeMode::Aurora: // drifting magnetic curtains
            {
                for (int i = 0; i < 6; ++i)
                    wave(300.0f + i * 22.0f + 8.0f * sway, 18.0f * pulse,
                         18.0f + i * 2.5f, motion * (0.3f + 0.035f * i),
                         i % 2 ? highlight : accent, 0.14f + i * 0.037f, 2.4f);
                orb(139.0f, 352.0f, 68.0f, accent, 0.19f + energy * 0.10f);
                break;
            }
            case FxThemeMode::Dusk: // violet eclipse
            {
                orb(cx, cy - 6.0f, 76.0f, accent, 0.13f);
                ring(cx, cy, 47.0f * pulse, 47.0f * pulse, accent, 0.40f, 1.4f);
                ring(cx + 13.0f, cy - 10.0f, 42.0f, 42.0f, highlight, 0.22f);
                for (int i = 0; i < 9; ++i)
                {
                    const float t = (float)i * 0.79f + motion * 0.08f;
                    g.setColour(highlight.withAlpha(0.22f + 0.08f * energy));
                    g.fillEllipse(cx + 59.0f * std::cos(t), cy + 83.0f * std::sin(t), 2.5f, 2.5f);
                }
                break;
            }
            case FxThemeMode::Copper: // engraved brass chronograph
            {
                ring(cx, cy, 54.0f * pulse, 54.0f * pulse, highlight, 0.55f, 2.0f);
                ring(cx, cy, 38.0f, 38.0f, accent, 0.32f, 1.0f);
                for (int i = 0; i < 24; ++i)
                {
                    const float t = (float)i * MathConstants<float>::twoPi / 24.0f;
                    const float r = (i % 3 == 0 ? 44.0f : 49.0f);
                    line(cx + r * std::cos(t), cy + r * std::sin(t),
                         cx + 55.0f * std::cos(t), cy + 55.0f * std::sin(t), accent, 0.49f);
                }
                line(cx, cy, cx + 34.0f * std::sin(motion * 0.36f),
                     cy - 34.0f * std::cos(motion * 0.36f), highlight, 0.72f, 2.3f);
                break;
            }
            case FxThemeMode::Midnight: // animated star chart
            {
                for (int i = 0; i < 30; ++i)
                {
                    const float x = 85.0f + (float)((i * 43 + 19) % 122);
                    const float y = 258.0f + (float)((i * 79 + 11) % 194);
                    const float twinkle = 0.45f + 0.55f * std::sin(motion * 0.8f + (float)i);
                    g.setColour(highlight.withAlpha(0.10f + 0.46f * twinkle));
                    g.fillEllipse(x, y, 1.5f + (float)(i % 3), 1.5f + (float)(i % 3));
                    if (i < 8)
                        line(x, y, 90.0f + (float)((i * 67) % 112),
                             280.0f + (float)((i * 31) % 160), accent, 0.12f);
                }
                break;
            }
            case FxThemeMode::Sakura: // drifting blossoms
            {
                for (int i = 0; i < 17; ++i)
                {
                    const float t = motion * (9.0f + i % 4) + i * 29.0f;
                    const float x = 84.0f + std::fmod(39.0f * i + 11.0f * sway + 1320.0f, 125.0f);
                    const float y = 263.0f + std::fmod(t + (float)((i * 59) % 196), 196.0f);
                    const float r = 2.2f + (float)(i % 4);
                    g.setColour((i % 3 ? accent : highlight).withAlpha(0.24f + a * 0.28f));
                    g.fillEllipse(x - r, y - r * 0.55f, r * 2.0f, r * 1.10f);
                    line(x, y, x + r, y + r * 1.3f, ink, 0.15f, 0.65f);
                }
                break;
            }
            case FxThemeMode::Arctic: // translucent ice shards
            {
                for (int i = 0; i < 6; ++i)
                {
                    const float x = 87.0f + i * 23.0f;
                    const float h = (float)((i * 37) % 75) + 64.0f + energy * 14.0f;
                    Path shard;
                    shard.startNewSubPath(x, 420.0f);
                    shard.lineTo(x + 11.0f + 5.0f * sway, 420.0f - h);
                    shard.lineTo(x + 20.0f, 420.0f);
                    shard.closeSubPath();
                    g.setColour((i % 2 ? highlight : accent).withAlpha(0.08f + i * 0.02f));
                    g.fillPath(shard);
                    g.setColour(accent.withAlpha(0.26f));
                    g.strokePath(shard, PathStrokeType(0.9f));
                }
                break;
            }
            case FxThemeMode::Sandstone: // layered drifting dunes
            {
                for (int i = 0; i < 7; ++i)
                    wave(311.0f + i * 23.0f, (10.0f + i * 2.0f) * pulse,
                         22.0f + i * 3.0f, motion * 0.13f + i * 0.5f,
                         i % 2 ? highlight : accent, 0.13f + i * 0.034f, 2.0f);
                break;
            }
            case FxThemeMode::Obsidian: // volcanic facets
            {
                for (int i = 0; i < 6; ++i)
                {
                    const float x = 76.0f + i * 26.0f;
                    const float top = 303.0f + 30.0f * std::sin(i * 2.1f + motion * 0.12f)
                        - energy * 12.0f;
                    Path facet;
                    facet.startNewSubPath(x, 445.0f);
                    facet.lineTo(x + 14.0f, top);
                    facet.lineTo(x + 29.0f, 445.0f);
                    facet.closeSubPath();
                    g.setColour((i % 2 ? highlight : accent).withAlpha(0.09f + 0.025f * i));
                    g.fillPath(facet);
                    g.setColour(highlight.withAlpha(0.30f));
                    g.strokePath(facet, PathStrokeType(0.95f));
                }
                orb(cx, 405.0f, 37.0f, accent, 0.24f + energy * 0.2f);
                break;
            }
            case FxThemeMode::Emerald: // branching botanical leaf veins
            {
                for (int i = 0; i < 5; ++i)
                {
                    const float x = 100.0f + i * 25.0f;
                    const float shift = 5.0f * std::sin(motion * 0.45f + i);
                    line(x, 431.0f, x + shift, 302.0f, accent, 0.42f, 1.9f);
                    for (int j = 0; j < 5; ++j)
                    {
                        const float y = 322.0f + j * 22.0f;
                        line(x + shift * (431.0f - y) / 129.0f, y,
                             x + (j % 2 ? 18.0f : -18.0f) + shift * 0.6f,
                             y - 15.0f, highlight, 0.24f + energy * 0.12f, 1.6f);
                    }
                }
                break;
            }
            case FxThemeMode::DeepSea: // tidal caustics and rising bubbles
            {
                for (int i = 0; i < 6; ++i)
                    wave(326.0f + i * 22.0f, (8.0f + i * 1.8f) * pulse,
                         15.0f + i * 5.0f, motion * 0.42f + i * 0.7f,
                         i % 2 ? highlight : accent, 0.19f + i * 0.03f, 1.8f);
                for (int i = 0; i < 10; ++i)
                {
                    const float x = 91.0f + (float)((i * 41) % 110);
                    const float y = 448.0f - std::fmod(motion * (8.0f + i % 4) + (float)(i * 27), 160.0f);
                    ring(x, y, 2.0f + i % 4, 2.0f + i % 4, highlight, 0.27f);
                }
                break;
            }
            case FxThemeMode::NeonTokyo: // illuminated skyline
            {
                for (int i = 0; i < 9; ++i)
                {
                    const float x = 84.0f + i * 15.0f;
                    const float h = 42.0f + (float)((i * 37) % 97);
                    g.setColour(accent.withAlpha(0.12f));
                    g.fillRect(x, 429.0f - h, 12.0f, h);
                    line(x, 429.0f - h, x + 11.0f, 429.0f - h, highlight, 0.52f, 1.5f);
                    for (int j = 0; j < 5; ++j)
                    {
                        const float y = 438.0f - h + j * 13.0f;
                        if (y < 426.0f)
                            line(x + 3.0f, y, x + 8.0f, y, highlight,
                                 0.17f + 0.22f * (0.5f + 0.5f * std::sin(motion + i + j)));
                    }
                }
                break;
            }
            case FxThemeMode::Synthwave: // retro horizon and perspective grid
            {
                orb(cx, 337.0f, 50.0f * pulse, highlight, 0.30f);
                ring(cx, 337.0f, 39.0f * pulse, 39.0f * pulse, accent, 0.62f, 2.0f);
                line(82.0f, 387.0f, 211.0f, 387.0f, highlight, 0.58f, 1.5f);
                for (int i = -4; i <= 4; ++i)
                    line(cx, 387.0f, cx + i * 23.0f, 461.0f, accent, 0.36f);
                for (int i = 1; i < 6; ++i)
                {
                    const float d = i * i * 2.2f;
                    line(80.0f, 387.0f + d, 213.0f, 387.0f + d, accent, 0.3f);
                }
                break;
            }
            case FxThemeMode::SolarFlare: // corona with audio-responsive rays
            {
                orb(cx, cy, 65.0f * pulse, accent, 0.26f + energy * 0.12f);
                ring(cx, cy, 30.0f * pulse, 30.0f * pulse, highlight, 0.73f, 2.0f);
                for (int i = 0; i < 28; ++i)
                {
                    const float t = (float)i * MathConstants<float>::twoPi / 28.0f + motion * 0.08f;
                    const float radius = 44.0f + 9.0f * std::sin((float)i * 2.7f + motion);
                    line(cx + 37.0f * std::cos(t), cy + 37.0f * std::sin(t),
                         cx + radius * pulse * std::cos(t), cy + radius * pulse * std::sin(t),
                         i % 2 ? accent : highlight, 0.50f, 1.6f);
                }
                break;
            }
            case FxThemeMode::Crimson: // pulsing ECG signal
            {
                for (int i = 0; i < 4; ++i)
                    line(85.0f, 315.0f + 33.0f * i, 207.0f, 315.0f + 33.0f * i,
                         highlight, 0.10f);
                Path heartbeat;
                const float height = 16.0f + energy * 28.0f;
                const float travel = 7.0f * std::sin(motion * 1.25f);
                heartbeat.startNewSubPath(84.0f, cy);
                heartbeat.lineTo(115.0f, cy);
                heartbeat.lineTo(126.0f + travel, cy - height);
                heartbeat.lineTo(139.0f + travel, cy + height * 1.1f);
                heartbeat.lineTo(152.0f + travel, cy - height * 0.5f);
                heartbeat.lineTo(164.0f, cy);
                heartbeat.lineTo(210.0f, cy);
                g.setColour(accent.withAlpha(0.72f));
                g.strokePath(heartbeat, PathStrokeType(2.2f));
                orb(cx, cy, 56.0f, accent, 0.10f + energy * 0.19f);
                break;
            }
            case FxThemeMode::Mocha: // warm concentric ceramic rings
            {
                for (int i = 0; i < 5; ++i)
                    ring(cx, cy + 25.0f, 18.0f + i * 11.0f * pulse,
                         18.0f + i * 7.0f * pulse, accent, 0.53f - i * 0.065f, 1.4f);
                for (int i = 0; i < 3; ++i)
                    wave(299.0f - i * 12.0f, 6.0f, 11.0f, motion * 0.5f + i,
                         highlight, 0.17f + i * 0.07f);
                break;
            }
            case FxThemeMode::Monochrome: // kinetic dot matrix
            {
                for (int y = 0; y < 12; ++y)
                    for (int x = 0; x < 8; ++x)
                    {
                        const float activity = 0.5f + 0.5f *
                            std::sin((float)x * 0.8f + (float)y * 0.65f + motion * 0.8f);
                        const float r = 1.0f + activity * (1.4f + energy * 1.5f);
                        g.setColour(accent.withAlpha(0.13f + activity * 0.40f));
                        g.fillEllipse(89.0f + x * 16.0f - r,
                                      268.0f + y * 16.0f - r, r * 2.0f, r * 2.0f);
                    }
                break;
            }
            case FxThemeMode::CharcoalDesktop: // architectural contours
            {
                for (int i = 0; i < 6; ++i)
                {
                    const float x = 88.0f + i * 19.0f + 2.0f * sway;
                    g.setColour(accent.withAlpha(0.095f + i * 0.018f));
                    g.fillRoundedRectangle(x, 273.0f + i * 12.0f,
                                           40.0f, 130.0f - i * 13.0f, 7.0f);
                    g.setColour(highlight.withAlpha(0.20f));
                    g.drawRoundedRectangle(x, 273.0f + i * 12.0f,
                                           40.0f, 130.0f - i * 13.0f, 7.0f, 0.85f);
                }
                break;
            }
            case FxThemeMode::NeuralCore: // network motif, distinct from the live 11/12/5 graph
            {
                for (int i = 0; i < 9; ++i)
                {
                    const float t = (float)i * MathConstants<float>::twoPi / 9.0f + motion * 0.11f;
                    const float x = cx + std::cos(t) * (49.0f + energy * 9.0f);
                    const float y = cy + std::sin(t) * 66.0f;
                    line(cx, cy, x, y, accent, 0.24f + energy * 0.12f);
                    ring(x, y, 3.0f + energy * 2.0f, 3.0f + energy * 2.0f,
                         i % 2 ? highlight : accent, 0.54f);
                }
                orb(cx, cy, 40.0f * pulse, highlight, 0.19f);
                break;
            }
            default: break;
        }

        // A delicate theme-tinted separator ties the left illustration to
        // the header. It pulses from the measured RMS, never fake FFT data.
        const float meterWidth = 24.0f + energy * 82.0f;
        g.setColour(accent.withAlpha(0.14f + energy * 0.26f));
        g.fillRoundedRectangle(94.0f, 459.0f, meterWidth, 2.0f, 1.0f);
    }
}
