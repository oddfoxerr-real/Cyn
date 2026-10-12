// SPDX-License-Identifier: AGPL-3.0-or-later
#include "CynFlagshipMixer.h"
#include "SourceAudioAnalyzer.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <iostream>

int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream cases(argv[1]); assert(cases.good());
    std::string line; int count=0;
    while(std::getline(cases,line)) {
        for(auto& c:line) if(c==',') c=' ';
        std::istringstream in(line); InputAudioFeatures a;
        for(auto& v:a.values) in>>v;
        a.rms=std::exp(a.values[10]);
        auto p=CynBalanceModel::predict(a); assert(p.valid);
        float sum=0;
        for(auto v:p.share) { float expected; in>>expected; assert(std::abs(v-expected)<2e-5f); sum+=v; }
        assert(!in.fail() && std::abs(sum-1)<1e-5f); ++count;
    }
    assert(count==64);
    InputAudioFeatures a; a.rms=.1f;
    a.values.fill(-3); a.values[8]=std::log(2.0f); a.values[9]=0; a.values[10]=std::log(a.rms);
    CynAudioScene s; s.valid=true; s.music=.9f; s.percussion=.8f; s.bassDrum=.8f;
    a.values[0]=-.2f; a.values[1]=-.4f;
    auto boomy=CynFlagshipMixer::decide(s,a,false);
    assert(boomy.lowEqDb<0 && boomy.eqDelta(60)<0);
    a.values[0]=-6; a.values[1]=-5;
    auto thin=CynFlagshipMixer::decide(s,a,false);
    assert(thin.lowEqDb>0);
    a.rms=.7f;
    auto hot=CynFlagshipMixer::decide(s,a,false);
    assert(hot.lowEqDb==0 && hot.levelingDb==0 && hot.headroom==0);
    a.rms=.1f; a.values[8]=std::numeric_limits<float>::quiet_NaN();
    auto bad=CynFlagshipMixer::decide(s,a,false);
    assert(bad.lowEqDb==0 && bad.levelingDb==0 && bad.preference[4]==.5f);
    assert(!CynBalanceModel::predict(a).valid);
    a.values[8]=.7f; a.rms=0;
    auto quiet=CynFlagshipMixer::decide(s,a,false);
    assert(quiet.eqDelta(2200)==0 && quiet.preference[3]==.5f);
    a.rms=.1f; s.music=0; s.speech=1;
    auto speech=CynFlagshipMixer::decide(s,a,false);
    assert(speech.eqDelta(60)==0 && speech.eqDelta(5000)==0);
    // Random hostile inputs must produce finite bounded DSP controls.
    for(int j=0;j<1000;++j) {
        for(int i=0;i<11;++i) a.values[i]=(float)((j*17+i*13)%120-60);
        a.rms=(j%40)*.1f; s.music=(j%3)?1:std::numeric_limits<float>::infinity();
        auto d=CynFlagshipMixer::decide(s,a,j%2);
        for(float f:{30.f,60.f,120.f,350.f,1000.f,2200.f,6500.f,16000.f})
            assert(std::isfinite(d.eqDelta(f)) && d.eqDelta(f)>=-2.5f && d.eqDelta(f)<=2.7f);
        for(auto v:d.preference) assert(std::isfinite(v) && v>=0 && v<=1);
    }
    // Real shipping feature extraction on synthetic stereo bass, all common rates.
    for(int rate:{16000,44100,48000,96000}) {
        SourceAudioAnalyzer analyzer; analyzer.setFormat(32,2,rate);
        std::vector<float> input(rate/2);
        for(size_t i=0;i<input.size()/2;++i) input[2*i]=input[2*i+1]=.12f*std::sin(6.28318530718f*60*i/rate);
        auto original=input; analyzer.process(input.data(),(int)input.size()/2);
        auto x=analyzer.snapshot(); assert(input==original);
        s={}; s.valid=true; s.music=1;
        auto d=CynFlagshipMixer::decide(s,x,false);
        assert(d.lowEqDb<0); // already bass-heavy: attenuate, do not pile on bass
        assert(CynBalanceModel::predict(x).valid);
    }
    std::cout<<"PASS: 64 Python/native parity cases, signed EQ, silence/speech/NaN/headroom guards, 1000 bounded stress cases and four input rates\n";
}
