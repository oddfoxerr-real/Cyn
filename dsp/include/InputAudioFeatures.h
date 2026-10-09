// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
struct InputAudioFeatures
{
    // Eight log band/full-energy ratios, log crest factor, stereo width,
    // and log RMS. Identical feature definition is used by the trainer.
    std::array<float, 11> values {};
    float rms = 0;
    uint32_t sequence = 0;
};
