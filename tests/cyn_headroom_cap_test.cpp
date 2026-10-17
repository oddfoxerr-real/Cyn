#include "CynSmartRange.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    // Default = exact previous headroom mapping.
    const float conservative=.22f, peak=.73f;
    assert(std::abs(CynSmartRange::headroomGate(conservative,peak,50,false)-conservative)<1e-6f);
    assert(CynSmartRange::headroomGate(conservative,peak,0,false)<conservative);
    const float max=CynSmartRange::headroomGate(conservative,peak,100,false);
    assert(max>conservative+0.20f && max<1.0f);
    assert(CynSmartRange::headroomGate(1.0f,.98f,100,false)==0.0f);
    assert(CynSmartRange::headroomGate(1.0f,.97f,100,false)==0.0f);
    assert(CynSmartRange::headroomGate(.5f,.96f,100,false)<.04f);
    assert(CynSmartRange::headroomGate(.5f,NAN,100,false)==0);
    assert(CynSmartRange::headroomGate(.5f,.73f,100,true)==.5f);
    // Full sensitivity is actually more expressive even for moderate scores.
    assert(CynSmartRange::expressiveScore(.65f,100,false)>.70f);
    assert(CynSmartRange::expressiveScore(.35f,100,false)<.30f);
    assert(CynSmartRange::expressiveScore(.5f,100,false)==.5f);
    assert(CynSmartRange::expressiveScore(.5001f,100,false)==.5f);
    assert(CynSmartRange::expressiveScore(.65f,50,false)==.65f);
    assert(CynSmartRange::expressiveScore(.65f,100,true)==.65f);
    const float score=CynSmartRange::expressiveScore(.65f,100,false);
    const float before=CynSmartRange::effectTarget(4.0f,.65f,100,5.6f,.85f,
        conservative,false,true);
    const float after=CynSmartRange::effectTarget(4.0f,score,100,5.6f,.85f,
        max,false,true);
    assert(after>before+1.0f);
    // Extreme source peaks still forbid positive bass/dynamic gain.
    assert(CynSmartRange::effectTarget(4.0f,score,100,5.6f,.85f,
        CynSmartRange::headroomGate(.4f,.99f,100,false),false,true)==4.0f);
    std::cout<<"PASS: persisted-cap authority semantics, more expressive scores, extreme-peak guard and Movie mode\n";
}
