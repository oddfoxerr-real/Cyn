// SPDX-License-Identifier: AGPL-3.0-or-later
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include "../../app/src/main/cpp/phone_engine.cpp"
int main(int argc,char** argv){
    assert(argc==2);
    std::array<float,6> policy{};
    std::ifstream model(argv[1],std::ios::binary);
    model.read(reinterpret_cast<char*>(policy.data()),sizeof(policy));
    assert(model.gcount()==static_cast<std::streamsize>(sizeof(policy)));
    for(float v:policy)assert(std::isfinite(v)&&v>=0&&v<=1);
    for(int channels:{1,2}){
        Engine baseline,creative;
        baseline.reset(channels);creative.reset(channels);
        baseline.parameters[0].store(1);creative.parameters[0].store(1);
        creative.audonicEnabled.store(true);
        for(int i=0;i<6;i++)creative.audonicParams[i].store(policy[i]);
        std::vector<int16_t> input(48000*channels);
        for(int n=0;n<48000;n++)for(int c=0;c<channels;c++)
            input[n*channels+c]=static_cast<int16_t>(6500*std::sin(n*.045+c*.15)+3500*std::sin(n*.27));
        std::vector<int16_t> dry,wet;
        for(int pass=0;pass<3;pass++){
            dry=input;wet=input;
            baseline.process(dry.data(),dry.size());creative.process(wet.data(),wet.size());
        }
        assert(creative.policyFrameCount.load()>=20);
        assert(dry!=wet);assert(creative.audonicActivity.load()>0);
        for(auto sample:wet)assert(std::abs(static_cast<int>(sample))<=31660);
        creative.parameters[0].store(0);
        for(int pass=0;pass<3;pass++){wet=input;creative.process(wet.data(),wet.size());}
        assert(wet==input);
        creative.reset(channels);creative.parameters[0].store(1);
        std::fill(wet.begin(),wet.end(),0);creative.process(wet.data(),wet.size());
        for(auto sample:wet)assert(sample==0);
    }
    std::cout<<"PASS: trained AUDONIC controls alter stereo and mono PCM, bounded output, exact master bypass and silence\n";
}
