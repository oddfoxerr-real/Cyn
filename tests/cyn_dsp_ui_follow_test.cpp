// Standalone native test: only true DSP target deltas may move the UI.
#include "../fxsound/Source/GUI/CynDspUiFollow.h"
#include <cassert>
#include <cmath>
#include <limits>

int main()
{
    using CynDspUiFollow::towardApplied;
    assert(towardApplied(2.0f, 2.0f, 33.0f, true) == 2.0f);
    assert(towardApplied(2.0f, 8.0f, 33.0f, false) == 8.0f);
    assert(towardApplied(8.0f, 1.0f, 33.0f, false) == 1.0f);
    assert(towardApplied(4.0f, std::numeric_limits<float>::quiet_NaN(), 33.0f, true) == 4.0f);

    float shown = -1.0f;
    const float applied = 4.35f;
    for (int frame = 0; frame < 80; ++frame)
    {
        const auto next = towardApplied(shown, applied, 33.0f, true);
        assert(next >= shown); // Real increase -> monotonic increase, never overshoot.
        assert(next <= applied);
        shown = next;
    }
    assert(std::abs(shown - applied) < 0.013f);
    assert(towardApplied(shown, applied, 33.0f, true) == applied);

    shown = 5.5f;
    for (int frame = 0; frame < 80; ++frame)
    {
        const auto next = towardApplied(shown, -3.25f, 16.0f, true);
        assert(next <= shown); // Real reduction -> monotonic decrease.
        assert(next >= -3.25f);
        shown = next;
    }
    assert(std::abs(shown + 3.25f) < 0.013f);
    // Zero elapsed time cannot create an imaginary change.
    assert(towardApplied(1.0f, 7.0f, 0.0f, true) == 1.0f);
    return 0;
}
