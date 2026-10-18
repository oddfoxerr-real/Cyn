#include "../fxsound/Source/GUI/CynUiMotion.h"
#include <cassert>
#include <cmath>
#include <iostream>

static bool near(float a,float b) {
    return std::fabs(a-b) < 0.0001f;
}

int main() {
    using namespace CynUiMotion;
    static_assert(pageDurationMs >= 320.0 && pageDurationMs <= 500.0,
                  "Real liquid mask must have enough frames to appear fluid without delaying input");
    static_assert(themeCarouselDurationMs <= 250.0, "Theme preview should feel responsive");
    assert(near(progress(-10,pageDurationMs),0));
    assert(near(progress(0,pageDurationMs),0));
    assert(near(progress(pageDurationMs,pageDurationMs),1));
    assert(near(progress(10000,pageDurationMs),1));
    assert(near(progress(0,0),1)); // zero-duration is an immediate transition.
    assert(near(easeOutCubic(-1),0));
    assert(near(easeOutCubic(0),0));
    assert(near(easeOutCubic(1),1));
    assert(near(easeOutCubic(2),1));
    assert(near(outgoingOpacity(0),1));
    assert(near(outgoingOpacity(1),0));
    assert(near(outgoingX(1,1),-pageTravelPixels));
    assert(near(outgoingX(1,-1),pageTravelPixels));
    float previousEase=-1.0f, previousOpacity=2.0f;
    for(int ms=0;ms<=(int)pageDurationMs;++ms) {
        auto p=progress(ms,pageDurationMs);
        auto e=easeOutCubic(p);
        auto opacity=outgoingOpacity(p);
        assert(p>=0 && p<=1 && e>=previousEase && opacity<=previousOpacity);
        previousEase=e;previousOpacity=opacity;
    }
    std::cout<<"PASS liquid-screen duration, deterministic easing, input independence and instant bypass\n";
}
