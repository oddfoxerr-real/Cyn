// SPDX-License-Identifier: AGPL-3.0-or-later
#include <cassert>
#include <iostream>
#include <vector>
#include "../../app/src/main/cpp/phone_engine.cpp"
int main(){
    Engine e;e.reset(2);
    std::vector<int16_t> audio(96000);for(size_t i=0;i<audio.size();i+=2){audio[i]=static_cast<int16_t>(15000*std::sin(i*.01));audio[i+1]=-audio[i];}
    auto dry=audio;e.process(audio.data(),audio.size());assert(audio==dry);
    e.parameters[0].store(1);e.parameters[1].store(12);e.parameters[2].store(8);e.parameters[3].store(1);e.parameters[4].store(1);e.parameters[5].store(1);e.parameters[7].store(1);e.parameters[8].store(39);
    for(int b=0;b<39;b++)e.parameters[9+b].store(12);
    for(int pass=0;pass<6;pass++){audio=dry;e.process(audio.data(),audio.size());}
    int peak=0;double energy=0;for(int16_t sample:audio){peak=std::max(peak,std::abs(static_cast<int>(sample)));energy+=static_cast<double>(sample)*sample;}
    assert(peak<=29210);assert(energy>1e6);assert(audio!=dry);
    // 100ms stereo ring is available to the worker, not computed by ONNX
    // inside the real-time PCM callback.
    const unsigned captured=e.policyFrameCount.load(std::memory_order_acquire);
    assert(captured>=20);
    for(int feature=0;feature<8;feature++)
        for(int frame=0;frame<20;frame++) {
            const float value=e.policyFrames[feature*20+frame].load(std::memory_order_relaxed);
            assert(std::isfinite(value));
            if(feature==3||feature==7)assert(value>=-1.0f&&value<=1.0f);
            else assert(value>=0&&value<=1.0f);
        }
    auto features=e.analyzer.snapshot();assert(features.rms>0);for(float value:features.values)assert(std::isfinite(value));
    std::array<float,15600>yam{};std::array<float,48000>apex{};uint64_t y=0,a=0;assert(e.analyzer.copyYamNetWindow(yam,y));assert(e.analyzer.copyApexWindow(apex,a));
    e.parameters[0].store(0);for(int pass=0;pass<2;pass++){audio=dry;e.process(audio.data(),audio.size());}audio=dry;e.process(audio.data(),audio.size());assert(audio==dry);
    for(int bands:{31,35,39}){e.reset(1);e.parameters[8].store(bands);e.parameters[0].store(1);std::fill(audio.begin(),audio.end(),0);for(int pass=0;pass<3;pass++)e.process(audio.data(),audio.size());for(auto s:audio)assert(s==0);}
    std::cout<<"PASS: exact settled bypass, worst-case boosts bounded, finite stereo analysis, both model windows, 31/35/39 silence and mono\n";
}
