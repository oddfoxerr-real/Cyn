// CYN AUDONIC native policy regression. SPDX-License-Identifier: AGPL-3.0-or-later
#include "../../fxsound/Source/GUI/CynAudonicStudio.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    using P=CynAudonicStudio;
    const auto silence=P::decide(1.0f,0.0f,1.0f,1.0f,1.0f,0.8f,0.0f);
    for(float x:silence) assert(x==0.0f);
    const auto normal=P::decide(0.9f,0.04f,0.75f,0.8f,0.7f,0.88f,0.65f);
    const auto quiet=P::decide(0.9f,0.04f,0.75f,0.8f,0.7f,0.88f,0.3f);
    const auto guarded=P::decide(0.9f,0.9f,0.75f,0.8f,0.7f,0.88f,0.65f);
    const auto noHeadroom=P::decide(0.9f,0.04f,0.75f,0.8f,0.7f,0.0f,0.65f);
    for(int i=0;i<P::Count;++i) {
        assert(std::isfinite(normal[(size_t)i]));
        assert(normal[(size_t)i]>=0.0f && normal[(size_t)i]<=1.0f);
        assert(normal[(size_t)i]>=quiet[(size_t)i]);
        assert(noHeadroom[(size_t)i]==0.0f);
    }
    assert(guarded[P::Harmonics]<normal[P::Harmonics]);
    assert(guarded[P::Colour]<normal[P::Colour]);
    const auto bad=P::decide(std::numeric_limits<float>::quiet_NaN(),
                              std::numeric_limits<float>::infinity(),
                              1.0f,0.5f,0.4f,1.0f,0.5f);
    for(float x:bad) assert(std::isfinite(x) && x>=0.0f && x<=1.0f);
    for(int effect=0;effect<5;++effect) {
        const auto add=P::effectAdd(effect,normal);
        assert(std::isfinite(add) && add>=0.0f && add<3.0f);
    }
    for(float freq:{31.0f,160.0f,900.0f,3500.0f,18000.0f}) {
        const float amount=P::eqAdd(freq,normal);
        assert(std::isfinite(amount) && amount>=-0.6f && amount<=0.85f);
    }
    assert(P::eqAdd(-1.0f,normal)==0.0f);
    std::cout<<"PASS CYN AUDONIC native creative DSP policy and safety bounds\n";
    return 0;
}
