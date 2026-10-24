// Pure EQ UI event dispatch: the live slider supplies the real DSP callbacks.
#pragma once
#include <cmath>
#include <utility>
namespace CynEqUiCallbacks {
template<class Get, class Set>
bool dispatchGain(int band, double gain, Get&& get, Set&& set) {
    if (band < 0 || !std::isfinite(gain)) return false;
    if (std::forward<Get>(get)(band) == gain) return false;
    std::forward<Set>(set)(band, gain);
    return true;
}
}
