// SPDX-License-Identifier: AGPL-3.0-or-later
#include <cassert>
#include <cmath>
#include <iostream>
#include "../../app/src/main/cpp/CynSpatialExperimental.h"

int main() {
    constexpr int count = 48000;
    constexpr float pi = 3.14159265358979323846f;
    CynSpatialExperimental bypass;
    CynSpatialExperimental effect;
    CynSpatialExperimental antiPhase;
    CynSpatialExperimental mono;
    double stereoDifference = 0.0;
    double bassDifference = 0.0;
    for (int n = 0; n < count; ++n) {
        const float t = static_cast<float>(n) / 48000.0f;
        const float center = .25f * std::sin(2.0f * pi * 400.0f * t);
        const float side = .08f * std::sin(2.0f * pi * 3200.0f * t);
        float l = center + side, r = center - side;
        float zeroL = l, zeroR = r;
        bypass.process(zeroL, zeroR, 0);
        assert(l == zeroL && r == zeroR);
        effect.process(l, r, .6f);
        assert(std::isfinite(l) && std::isfinite(r));
        assert(std::fabs((l + r) - 2.0f * center) < 2e-6f);
        stereoDifference += std::fabs(l - (center + side));
        float bL = center + .08f * std::sin(2*pi*60*t);
        float bR = center - .08f * std::sin(2*pi*60*t);
        // A separate engine only processing a bass-side signal.
        static CynSpatialExperimental low;
        const float oldL = bL;
        low.process(bL, bR, .6f);
        bassDifference += std::fabs(bL - oldL);
        float mL = center, mR = center;
        mono.process(mL, mR, 1);
        assert(mL == center && mR == center);
        float aL = center, aR = -center;
        antiPhase.process(aL, aR, 1);
        assert(std::fabs(aL - center) < 1e-6f);
        assert(std::fabs(aR + center) < 1e-6f);
    }
    assert(stereoDifference > 1.0);
    assert(stereoDifference > bassDifference * 1.5);
    std::cout << "PASS: APEX 10M stereo ambience preview, exact bypass/mono, anti-phase guard, center preserved, bass protected\n";
}
