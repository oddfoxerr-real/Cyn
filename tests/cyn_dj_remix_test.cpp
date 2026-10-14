#include "CynDjEffects.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <cstdlib>

static void wav(const char* path,const std::vector<float>& samples,int rate) {
    std::ofstream f(path,std::ios::binary);
    const auto u16=[&](uint16_t n){f.put(char(n));f.put(char(n>>8));};
    const auto u32=[&](uint32_t n){u16(uint16_t(n));u16(uint16_t(n>>16));};
    f.write("RIFF",4);u32(36+uint32_t(samples.size()*2));f.write("WAVEfmt ",8);
    u32(16);u16(1);u16(2);u32(rate);u32(rate*4);u16(4);u16(16);
    f.write("data",4);u32(uint32_t(samples.size()*2));
    for(float x:samples)u16(uint16_t(int16_t(std::clamp(x,-1.0f,1.0f)*32767)));
    assert(f.good());
}
int main(int argc,char** argv) {
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    for(int rate:{44100,48000,96000,192000}) {
        auto dj=std::make_unique<CynDjEffects>();
        dj->setFormat(32,2,rate);dj->enable(true);dj->configure(0.25f,4,false);
        const int frames=rate/100;
        std::vector<float> audio(size_t(frames)*2),dryDemo,wetDemo;
        double difference[3]{},energy[3]{};int observed[3]{};
        for(int b=0;b<2000;++b) {
            if(b%50==0){dj->scene(0.9f,0);dj->voiceGate(true);}
            for(int i=0;i<frames;++i) {
                const double t=double(b*frames+i)/rate;
                const double beat=std::fmod(t,0.5),note=std::fmod(t,0.25);
                const int step=int(t*4)%8;
                const double notes[]={220,277.18,329.63,440,392,329.63,277.18,246.94};
                const float x=float(0.17*std::sin(6.2831853*notes[step]*t)*std::exp(-note*4)
                    +0.14*std::sin(6.2831853*65*t)*std::exp(-beat*24)
                    +0.04*std::sin(6.2831853*4200*t)*std::exp(-note*40));
                audio[i*2]=x;audio[i*2+1]=-x;
            }
            const auto dry=audio;dj->process(audio.data(),frames);
            const int kind=dj->lastEffect()-1;
            if(kind>=0)for(size_t i=0;i<audio.size();++i){difference[kind]+=std::pow(audio[i]-dry[i],2);energy[kind]+=dry[i]*dry[i];}
            for(int i=0;i<frames;++i){assert(std::isfinite(audio[i*2]));assert(std::abs(audio[i*2])<=0.98f);assert(std::abs(audio[i*2]+audio[i*2+1])<1e-6f);}
            if(kind>=0)++observed[kind];
            // Undo the anti-phase stress fixture for the listenable demo files.
            if(rate==48000&&argc==3)for(int i=0;i<frames;++i){dryDemo.push_back(dry[i*2]);dryDemo.push_back(-dry[i*2+1]);wetDemo.push_back(audio[i*2]);wetDemo.push_back(-audio[i*2+1]);}
        }
        for(int k=0;k<3;++k){assert(observed[k]>0);const double ratio=std::sqrt(difference[k]/energy[k]);std::cout<<rate<<" Hz effect "<<k+1<<" wet/dry RMS "<<ratio<<std::endl;assert(ratio>0.035);}
        // Muting or losing authorisation must not keep remixing buffered music.
        dj->voiceGate(false);dj->scene(0,1);
        for(int b=0;b<50;++b){std::fill(audio.begin(),audio.end(),0.0f);dj->process(audio.data(),frames);}
        for(float x:audio)assert(std::abs(x)<1e-7f);
        dj->enable(false);std::fill(audio.begin(),audio.end(),0.12f);auto dry=audio;dj->process(audio.data(),frames);assert(audio==dry);
        if(rate==48000&&argc==3){wav(argv[1],dryDemo,rate);wav(argv[2],wetDemo,rate);}
    }
    // A one-sided input cannot create an unrelated tone/noise in the other ear.
    auto dj=std::make_unique<CynDjEffects>();dj->setFormat(32,2,48000);dj->enable(true);dj->configure(1,4,true);
    std::array<float,960> block{};
    for(int b=0;b<2000;++b){dj->scene(1,0);dj->voiceGate(true);for(int i=0;i<480;++i){block[i*2]=0.95f*std::sin((b*480+i)*0.037f);block[i*2+1]=0;}dj->process(block.data(),480);for(int i=0;i<480;++i){assert(block[i*2+1]==0);assert(std::abs(block[i*2])<=0.98f);}}
    std::cout<<"PASS: three source-derived remixes, measurable mix at 25%, stereo preservation, rates, voice suppression and bypass"<<std::endl;
}
