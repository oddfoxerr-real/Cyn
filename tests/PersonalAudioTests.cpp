// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../fxsound/Source/GUI/PersonalAudioModel.h"
#include "SourceAudioAnalyzer.h"
#include <cassert>
#include <limits>
#include <fstream>
#include <iostream>

InputAudioFeatures tone(float frequency,int rate=48000,bool antiphase=false) {
    SourceAudioAnalyzer analyzer; analyzer.setFormat(32,2,rate);
    std::vector<float> audio(rate*2);
    for(int i=0;i<rate;++i) { audio[2*i]=0.15f*std::sin(6.28318530718*frequency*i/rate); audio[2*i+1]=audio[2*i]*(antiphase?-1:1); }
    auto original=audio; analyzer.process(audio.data(),rate); assert(audio==original);
    return analyzer.snapshot();
}
int main(int argc,char** argv) {
    auto bass=tone(60), treble=tone(2000), anti=tone(60,48000,true), highRate=tone(60,96000);
    assert(bass.rms>0.1f && bass.sequence>0);
    assert(bass.values[0]>treble.values[0]+4);
    assert(std::abs(bass.rms-anti.rms)<0.001 && anti.values[9]>1.9f);
    assert(std::abs(bass.values[0]-highRate.values[0])<0.05);
    SourceAudioAnalyzer a; a.setFormat(32,2,48000);
    std::vector<float> bad(4800,std::numeric_limits<float>::quiet_NaN());
    a.process(bad.data(),2400); assert(std::isfinite(a.snapshot().rms) && a.snapshot().rms==0);
    a.setFormat(16,2,48000); auto seq=a.snapshot().sequence; a.process(bad.data(),2400); assert(a.snapshot().sequence==seq);
    PersonalAudioModel learned; std::vector<AudioExample> labels;
    for(int i=0;i<16;++i) {
        auto b=bass,t=treble; b.values[0]+=(i%3)*0.02f; t.values[0]+=(i%3)*0.02f;
        labels.push_back({b,4,true,i,false}); labels.push_back({t,4,false,100+i,false});
        labels.push_back({b,0,false,200+i,false}); labels.push_back({t,0,true,300+i,false});
    }
    int telemetryEpoch=0, telemetryFrames=0;
    std::atomic<int> completed {0};
    assert(learned.fit(labels,&completed,[&](const NeuralActivitySnapshot& s,int epoch) {
        assert(epoch>telemetryEpoch); telemetryEpoch=epoch; ++telemetryFrames;
        assert(s.outputs==learned.trace(labels.front().audio).outputs);
    }));
    assert(telemetryEpoch==240 && completed==240 && telemetryFrames==21);
    const auto trace=learned.trace(bass);
    assert(trace.sequence==bass.sequence && trace.outputs==learned.predict(bass));
    for(int j=0;j<12;++j) {
        float sum=learned.weights[132+j];
        for(int i=0;i<11;++i) {
            assert(trace.inputConnections[j*11+i]==trace.inputs[i]*learned.weights[j*11+i]);
            sum+=trace.inputConnections[j*11+i];
        }
        assert(std::abs(trace.hidden[j]-std::tanh(sum))<1e-6f);
    }
    for(int o=0;o<5;++o) for(int j=0;j<12;++j)
        assert(trace.outputConnections[o*12+j]==trace.hidden[j]*learned.weights[144+o*12+j]);
    NeuralTrainingTelemetry telemetry;
    NeuralActivitySnapshot copy;
    assert(!telemetry.read(copy)); telemetry.publish(trace,240);
    assert(telemetry.read(copy) && copy.training && copy.epoch==240 && copy.outputs==trace.outputs);
    assert(learned.ready[4] && learned.ready[0] && !learned.ready[1] && !learned.ready[2] && !learned.ready[3]);
    assert(learned.predict(bass)[4]>.85 && learned.predict(treble)[4]<.15);
    assert(learned.predict(bass)[0]<.15 && learned.predict(treble)[0]>.85);
    // Reverse feedback: predictions must follow the listener, not a coded bass rule.
    for(auto& e:labels) e.more=!e.more;
    assert(learned.fit(labels));
    assert(learned.predict(bass)[4]<.15 && learned.predict(treble)[4]>.85);
    assert(learned.predict(bass)[0]>.85 && learned.predict(treble)[0]<.15);
    PersonalAudioModel insufficient; labels.resize(2); assert(!insufficient.fit(labels));
    if(argc>1) {
        std::ifstream modelFile(argv[1]); PersonalAudioModel starter;
        for(auto& w:starter.weights) modelFile>>w; assert(!modelFile.fail());
        std::cout<<"Starter pure-tone bass preferences: 60Hz="<<starter.predict(bass)[4]<<" 2kHz="<<starter.predict(treble)[4]<<"\n";
        assert(starter.predict(bass)[4]>starter.predict(treble)[4]);
    }
    std::cout<<"PASS: pre-effect analysis, non-destructive input, stereo anti-phase, sample rates, invalid input, sparse effect learning, reversed personal feedback, minimum examples\n";
}
