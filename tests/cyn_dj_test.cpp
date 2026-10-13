// Native regression tests for callback safety and the exact AST frontend.
#include "CynDjEffects.h"
#include "CynAstFeatures.h"
#include "SourceAudioAnalyzer.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <limits>
#include <cstring>
int main(int argc,char** argv) {
    auto dj=std::make_unique<CynDjEffects>();
    dj->setFormat(32,2,48000);
    std::array<float,960> block{};
    for(size_t i=0;i<block.size();++i)block[i]=float(i)/1000-0.5f;
    const auto dry=block;dj->process(block.data(),480);assert(block==dry);
    dj->enable(true);dj->configure(0,4,true);dj->scene(1,0);
    dj->process(block.data(),480);assert(block==dry);
    dj->configure(0.8f,4,true);
    double delta=0;uint32_t lastEvent=0;int lastBlock=-1000;
    for(int b=0;b<3000;++b) {
        if(b%100==0){dj->scene(1,0);dj->voiceGate(true);}
        for(int i=0;i<480;++i) {
            const float gain=b%50<7?0.4f:0.003f;
            block[2*i]=block[2*i+1]=gain*float(std::sin((b*480+i)*6.28318530718*160/48000));
        }
        const auto input=block;dj->process(block.data(),480);
        for(size_t i=0;i<block.size();++i) {assert(std::isfinite(block[i]));assert(std::abs(block[i])<=0.98f);delta+=std::abs(block[i]-input[i]);}
        if(dj->events()!=lastEvent) {assert(b-lastBlock>=399);lastBlock=b;lastEvent=dj->events();}
    }
    assert(dj->events()>=3);assert(delta>0.1);
    dj->voiceGate(false);auto gated=dj->events();
    for(int b=0;b<700;++b){if(b%100==0)dj->scene(1,0);block.fill(b%50<5?0.25f:0);dj->process(block.data(),480);}
    assert(dj->events()==gated);
    dj->voiceGate(true);
    dj->scene(0.8f,0.9f);auto n=dj->events();
    for(int b=0;b<1400;++b) {block.fill(b%50<5?0.25f:0);dj->process(block.data(),480);}
    assert(dj->events()==n);
    dj->scene(1,0);
    for(int b=0;b<2000;++b){block.fill(0);dj->process(block.data(),480);}
    n=dj->events();
    for(int b=0;b<1000;++b){block.fill(b%50<5?0.25f:0);dj->process(block.data(),480);}
    assert(dj->events()==n); // stale model expires
    dj->power(false);block=dry;dj->process(block.data(),480);assert(block==dry);
    dj->power(true);dj->enable(false);block=dry;dj->process(block.data(),480);assert(block==dry);
    dj->enable(true);dj->setFormat(32,6,48000);block=dry;dj->process(block.data(),160);assert(block==dry);
    auto analyzer=std::make_unique<SourceAudioAnalyzer>();analyzer->setFormat(32,2,48000);
    auto window=std::make_unique<std::array<float,128000>>();uint64_t sequence=0;
    assert(!analyzer->copyDjWindow(*window,sequence));
    block.fill(0.125f);for(int i=0;i<1000;++i)analyzer->process(block.data(),480);
    assert(analyzer->copyDjWindow(*window,sequence));assert(!analyzer->copyDjWindow(*window,sequence));
    for(float x:*window)assert(std::abs(x-0.125f)<1e-6f);
    analyzer->setFormat(32,2,44100);assert(!analyzer->copyDjWindow(*window,sequence));
    if(argc==4) {
        std::ifstream input(argv[1],std::ios::binary);
        input.read(reinterpret_cast<char*>(window->data()),window->size()*sizeof(float));assert(input.gcount()==window->size()*sizeof(float));
        auto features=std::make_unique<CynAstFeatures>();std::vector<float> output;
        features->compute(window->data(),window->size(),output);
        std::ofstream(argv[2],std::ios::binary).write(reinterpret_cast<char*>(output.data()),output.size()*sizeof(float));
        std::ofstream(argv[3])<<"callback regression tests passed\n";
    }
    std::cout<<"PASS: exact bypass, real effects, headroom, cooldown, speech suppression, stale inference, power, formats and capture window\n";
}
