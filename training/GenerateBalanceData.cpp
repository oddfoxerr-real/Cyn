// SPDX-License-Identifier: AGPL-3.0-or-later
// Original procedural training signals. Uses the EXACT shipping feature extractor.
#include "SourceAudioAnalyzer.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if(argc!=4) { std::cerr<<"output.csv seed count\n"; return 1; }
    std::ofstream out(argv[1]); if(!out) return 2;
    std::mt19937 rng((unsigned)std::stoul(argv[2]));
    std::uniform_real_distribution<double> uniform(0,1);
    auto u=[&](){return uniform(rng);};
    out<<std::setprecision(9);
    const int count=std::stoi(argv[3]);
    for(int row=0;row<count;++row) {
        const int rates[]={16000,44100,48000}; const int sr=rates[row%3];
        const int window=sr/20, frames=window*5;
        std::array<std::vector<float>,5> stems;
        for(auto& stem:stems) stem.resize(frames);
        const double bassHz=32+u()*110, kickHz=40+u()*70, snareHz=130+u()*160;
        const double voiceHz=105+u()*300, phase=u()*6.28318530718;
        const double kickDecay=.02+u()*.14, snareDecay=.012+u()*.10;
        const double onset=.155+u()*.08, snareOnset=.155+u()*.08;
        const double target=.018+u()*.23, width=u()*.6;
        std::array<double,5> gains;
        for(auto& gain:gains) gain=u()<.2?0:std::pow(10.0,(-24+u()*30)/20);
        for(int i=0;i<frames;++i) {
            double t=(double)i/sr, kt=t-onset, st=t-snareOnset, noise=2*u()-1;
            stems[0][i]=(float)(std::sin(6.28318530718*bassHz*t+phase)+.2*std::sin(12.56637061436*bassHz*t));
            stems[1][i]=(float)(kt<0?0:std::exp(-kt/kickDecay)*std::sin(6.28318530718*(kickHz*kt+1.2*(1-std::exp(-kt*55)))));
            stems[2][i]=(float)(st<0?0:std::exp(-st/snareDecay)*(.7*noise+.3*std::sin(6.28318530718*snareHz*st)));
            stems[3][i]=(float)(noise*std::sin(6.28318530718*(4200+row%2700)*t)*(.2+.8*std::pow(std::sin(t*(15+row%15)),8)));
            stems[4][i]=(float)(std::sin(6.28318530718*voiceHz*t)+.4*std::sin(6.28318530718*voiceHz*3*t)+.2*std::sin(6.28318530718*voiceHz*7*t));
        }
        std::vector<float> audio(frames*2);
        double energy=0; std::array<double,5> parts{};
        for(int i=0;i<frames;++i) {
            double l=0,r=0;
            for(int s=0;s<5;++s) {
                double v=stems[s][i]*gains[s];
                double pan=1+width*(s%2?1:-1);
                l+=v; r+=v*pan;
                if(i>=frames-window) parts[s]+=v*v*(1+pan*pan);
            }
            audio[2*i]=(float)l; audio[2*i+1]=(float)r;
            energy+=l*l+r*r;
        }
        double scale=target/std::sqrt(energy/(2*frames)+1e-20);
        for(auto& sample:audio) sample=(float)(sample*scale);
        SourceAudioAnalyzer analyzer; analyzer.setFormat(32,2,sr);
        analyzer.process(audio.data(),frames);
        auto features=analyzer.snapshot();
        double sum=0; for(auto v:parts) sum+=v;
        if(sum<1e-10) { --row; continue; }
        for(float x:features.values) out<<x<<',';
        for(int s=0;s<5;++s) out<<parts[s]/sum<<(s==4?'\n':',');
    }
    return out.good()?0:3;
}
