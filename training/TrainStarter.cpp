// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../fxsound/Source/GUI/PersonalAudioModel.h"
#include <fstream>
#include <iomanip>
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=3) return 1;
    std::ifstream in(argv[1]); int n=0; in>>n;
    std::vector<AudioExample> train(n);
    for(auto& e:train) { for(auto& x:e.audio.values) in>>x; in>>e.effect>>e.more; }
    PersonalAudioModel model; if(!model.fit(train)) return 2;
    std::ofstream out(argv[2]); out<<std::setprecision(9);
    for(float w:model.weights) out<<w<<' '; out<<'\n';
    in>>n;
    for(int i=0;i<n;++i) { InputAudioFeatures a; for(auto& x:a.values) in>>x; auto scores=model.predict(a); for(float p:scores) out<<p<<' '; out<<'\n'; }
    return in.fail()?3:0;
}
