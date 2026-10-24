#include "../fxsound/Source/GUI/CynExpressiveMotion.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
bool near(float a, float b, float tolerance=0.02f) {
    return std::fabs(a-b) <= tolerance;
}
}
int main() {
    using namespace CynExpressiveMotion;
    assert(near(easeFactor(-1.0f, 78.0f), 0.0f));
    assert(near(easeFactor(0.0f, 78.0f), 0.0f));
    assert(near(easeFactor(16.0f, 0.0f), 1.0f));
    assert(easeFactor(16.0f, 78.0f) > 0.0f);
    assert(easeFactor(16.0f, 78.0f) < 1.0f);
    const std::array<bool, 6> clear{};
    State ui;
    ui.advance(16.0f, 0, 0, clear, false);
    assert(near(ui.navigationY, 104.0f));
    assert(near(ui.indicatorTop,104.0f) && near(ui.indicatorHeight,48.0f));
    assert(near(ui.focusX, 258.0f));
    assert(near(ui.focusWidth, 126.0f));
    const std::array<bool, 6> hovering{{false, true, false, false, false, false}};
    ui.advance(16.0f, 4, 2, hovering, false);
    assert(ui.navigationY > 104.0f && ui.navigationY < 336.0f);
    assert(ui.indicatorHeight >= 48.0f && ui.indicatorHeight < 280.1f);
    assert(ui.focusX > 258.0f && ui.focusX < 526.0f);
    assert(ui.hovered[1] > 0.0f && ui.hovered[1] < 1.0f);
    float previous = ui.navigationY;
    for (int i = 0; i < 50; ++i) {
        ui.advance(16.0f, 4, 2, hovering, false);
        assert(ui.navigationY >= previous && ui.navigationY <= 336.01f);
        previous = ui.navigationY;
    }
    assert(near(ui.navigationY, 336.0f));
    assert(near(ui.indicatorTop,336.0f) && near(ui.indicatorHeight,48.0f));
    assert(near(ui.focusX, 526.0f));
    assert(near(ui.focusWidth, 164.0f));
    // A second click interrupts the first target without jumping.
    ui.advance(16.0f, 1, 1, clear, false);
    assert(ui.navigationY < 336.0f && ui.navigationY > 162.0f);
    ui.advance(16.0f, 1, 1, clear, true);
    assert(near(ui.navigationY, 162.0f));
    assert(near(ui.indicatorTop,162.0f) && near(ui.indicatorHeight,48.0f));
    assert(near(ui.focusX, 392.0f));
    assert(near(ui.focusWidth, 126.0f));
    assert(near(ui.hovered[1], 0.0f));
    // Invalid indices cannot move the highlight outside the navigator.
    ui.advance(16.0f, 300, -9, clear, true);
    assert(near(ui.navigationY, 336.0f));
    assert(near(ui.focusX, 258.0f));
    std::cout << "PASS expressive navigation, segmented morph, retarget and reduced motion\n";
}
