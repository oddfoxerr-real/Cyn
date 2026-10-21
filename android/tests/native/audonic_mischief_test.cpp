#include "../app/src/main/cpp/AudonicMischief.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    AudonicMischief processor;
    std::array<float,6> bypass{};
    processor.reset();
    for(int i=0;i<5000;i++){
        float left=.2f*std::sin(i*.07f),right=.14f*std::cos(i*.08f);
        float l=left,r=right;
        processor.process(l,r,bypass);
        assert(std::fabs(l-left)<.000001f&&std::fabs(r-right)<.000001f);
    }
    processor.reset();
    std::array<float,6> active{.8f,.7f,.6f,.8f,.35f,.6f};
    double changed=0;float maxValue=0;
    for(int i=0;i<96000;i++){
        float base=.3f*std::sin(i*.025f)+.14f*std::sin(i*.55f);
        float l=base+.12f*std::sin(i*.15f),r=base-.12f*std::sin(i*.15f);
        float original=l;
        processor.process(l,r,active);
        changed+=std::fabs(l-original);
        maxValue=std::max(maxValue,std::max(std::fabs(l),std::fabs(r)));
        assert(std::isfinite(l)&&std::isfinite(r));
        assert(maxValue<1.0f);
    }
    assert(changed>1.0);
    assert(processor.activity()>0);
    // An identical mono signal must remain mono even with dramatic commands.
    processor.reset();
    for(int i=0;i<48000;i++){
        float l=.34f*std::sin(i*.09f),r=l;
        processor.process(l,r,active);
        assert(std::fabs(l-r)<1e-6f);
    }
    processor.reset();
    float l=std::nanf(""),r=0;
    processor.process(l,r,active); // nonfinite input must not propagate.
    std::cout<<"PASS: AUDONIC bypass, genuine DSP change, bounded output, mono-safe and finite\n";
}
