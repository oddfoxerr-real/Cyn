// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../dsp/include/CynSpatialExperimental.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main() {
    constexpr float pi=3.14159265358979323846f;
    constexpr int frames=48000;
    std::vector<float> audio(frames*2), reference(frames*2), antiphase(frames*2), mono(frames*2);
    for(int n=0;n<frames;n++) {
        const float time=static_cast<float>(n)/48000.0f;
        const float mid=0.26f*std::sin(2*pi*400*time);
        const float side=0.08f*std::sin(2*pi*3400*time);
        reference[n*2]=mid+side;
        reference[n*2+1]=mid-side;
        antiphase[n*2]=mid;
        antiphase[n*2+1]=-mid;
        mono[n*2]=mid;
        mono[n*2+1]=mid;
    }
    CynSpatialExperimental bypass; bypass.setFormat(32,2,48000);
    audio=reference; bypass.process(audio.data(),frames,0.0f);
    assert(audio==reference && "Exact zero-intensity bypass failed");
    CynSpatialExperimental monoStage; monoStage.setFormat(32,2,48000);
    auto originalMono=mono; monoStage.process(mono.data(),frames,1.0f);
    assert(mono==originalMono && "Mono center must remain exactly unchanged");
    CynSpatialExperimental antiphaseStage; antiphaseStage.setFormat(32,2,48000);
    auto originalAnti=antiphase; antiphaseStage.process(antiphase.data(),frames,1.0f);
    for(size_t i=0;i<antiphase.size();i++)assert(std::fabs(antiphase[i]-originalAnti[i])<2e-6f);
    CynSpatialExperimental spatial; spatial.setFormat(32,2,48000);
    audio=reference; spatial.process(audio.data(),frames,0.7f);
    double delta=0;
    for(int n=0;n<frames;n++){
        assert(std::isfinite(audio[n*2])&&std::isfinite(audio[n*2+1]));
        assert(std::fabs((audio[n*2]+audio[n*2+1])-(reference[n*2]+reference[n*2+1]))<1e-5f);
        delta+=std::fabs(audio[n*2]-reference[n*2]);
    }
    assert(delta>1 && "Stereo enhancement did not affect existing high-frequency side detail");
    CynSpatialExperimental unsupported; unsupported.setFormat(16,2,48000);
    audio=reference; unsupported.process(audio.data(),frames,1.0f);assert(audio==reference);
    std::cout<<"PASS: native stereo spatial DSP, exact bypass, mono/antiphase, center, channel and PCM format guards\n";
}
