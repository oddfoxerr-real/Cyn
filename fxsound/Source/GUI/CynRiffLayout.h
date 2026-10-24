// Native translation of the Riff application's MIT-licensed CSS surface/layout
// tokens. Riff copyright (c) 2026 rootscripts; see third_party/riff-LICENSE.txt.
// Source: https://github.com/rootscripts/riff/blob/main/styles.css
// Only presentation constants are reused. Audio stays JUCE/FxSound.
#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace CynRiffLayout {
constexpr float railLeft=10.0f, railWidth=80.0f, railTop=8.0f;
constexpr float railIconX=26.0f, railIconY=104.0f, railIconStep=58.0f, railIconSize=48.0f;
constexpr float mainLeft=102.0f, mainRight=1310.0f;
constexpr float legacyContentLeft=240.0f, contentLeft=112.0f, contentRight=1300.0f;
constexpr float contentScale=(contentRight-contentLeft)/(1300.0f-legacyContentLeft);
constexpr float contentOffset=contentLeft-legacyContentLeft*contentScale;
inline int x(int legacy) noexcept {
    return (int)std::lround(legacy*contentScale+contentOffset);
}
inline int width(int legacy) noexcept {
    return (int)std::lround(legacy*contentScale);
}
inline juce::AffineTransform visualTransform() noexcept {
    return juce::AffineTransform::scale(contentScale,1.0f).translated(contentOffset,0.0f);
}
// Exact neutral surface tokens from Riff styles.css; CYN keeps its own accent.
inline juce::Colour background() noexcept { return juce::Colour(0xff121214); }
inline juce::Colour rail() noexcept { return juce::Colour(0xff1e1f22); }
inline juce::Colour main() noexcept { return juce::Colour(0xff1a1b1e); }
inline juce::Colour hover() noexcept { return juce::Colour(0xff28292c); }
inline juce::Colour active() noexcept { return juce::Colour(0xff333438); }
} // namespace CynRiffLayout
