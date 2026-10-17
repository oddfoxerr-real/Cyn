#include "CynSmartRange.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    // The user's existing 50%-sensitivity sound must not regress.
    const float base=4.0f;
    const float legacy=base+(2.0f*(0.74f-0.5f))*5.6f*0.50f;
    const float normal=CynSmartRange::effectTarget(base,0.74f,50,5.6f,.85f,1,false,true);
    assert(std::abs(legacy-normal)<0.00001f);
    assert(CynSmartRange::effectTarget(base,0.74f,0,5.6f,.85f,1,false,true)==base);
    // At 100%, strong musical evidence can command every part of the available
    // effect range, not only a small fixed positive margin.
    const float full=CynSmartRange::effectTarget(base,0.74f,100,5.6f,.85f,1,false,true);
    const float maximum=CynSmartRange::effectTarget(base,1.0f,100,5.6f,.85f,1,false,true);
    assert(full>normal+2.5f);
    assert(maximum==10.0f);
    assert(CynSmartRange::effectTarget(7,0.0f,100,5.6f,.85f,1,false,true)==0);
    assert(CynSmartRange::effectTarget(7,0.5f,100,5.6f,.85f,1,false,true)==7);
    // Movement remains monotonic as higher user sensitivity is selected.
    const float mid=CynSmartRange::effectTarget(base,0.74f,75,5.6f,.85f,1,false,true);
    assert(normal<mid && mid<full);
    // Attack behavior responds quicker but still uses bounded, smooth steps.
    assert(CynSmartRange::effectStep(100,false)>CynSmartRange::effectStep(50,false));
    assert(CynSmartRange::eqStep(100,false)>CynSmartRange::eqStep(50,false));
    assert(CynSmartRange::eqAmount(100,false)>1.9f);
    assert(CynSmartRange::scoreFollower(100,true,false)>
           CynSmartRange::scoreFollower(50,true,false));
    // Peak-risk protection remains effective; extra DSP gain may not be added
    // with no available input headroom (not a substitute for output limiting).
    assert(CynSmartRange::effectTarget(base,.9f,100,5.6f,.85f,0,false,true)==base);
    assert(CynSmartRange::effectTarget(base,.9f,100,5.6f,.85f,.25f,false,true)<full);
    // The Movie AI is deliberately unchanged, especially dialogue limits.
    assert(CynSmartRange::freedom(100,true)==0);
    assert(std::abs(CynSmartRange::effectTarget(base,.9f,100,0.8f,.65f,1,true,true)
                    -(base+.8f*.8f))<0.00001f);
    assert(CynSmartRange::effectStep(100,true)==0.30f);
    // Bad input can never escape the legal DSP range.
    assert(std::isfinite(CynSmartRange::effectTarget(6.0f,NAN,100,5.6f,.8f,1,false,true)));
    assert(CynSmartRange::effectTarget(14.0f,1.0f,100,5.6f,.8f,1,false,true)<=10);
    std::cout<<"PASS: full 0-10 music authority, preserved baseline, gain safety and Movie limits\n";
}
