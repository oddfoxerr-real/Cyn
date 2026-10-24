# Riff source adaptation for native CYN Desktop

Reference: [Riff](https://github.com/rootscripts/riff), MIT-licensed (c) 2026 rootscripts.
License notice: [third_party/riff-LICENSE.txt](../third_party/riff-LICENSE.txt).

Unlike the previous motion-only release, this layout uses actual source values and
structure from Riff `styles.css` (`.sidebar`, `.nav-indicator`, `.main-content`,
M3 surface-container levels, 80px rail, 48px indicator and 28px panel radius).
Those values are translated into the native JUCE `CynRiffLayout.h` instead of
running HTML/CSS through an Electron player.

The desktop view's six navigation actions now use native vector glyphs and
tooltips in a compact 80px floating rail. CYN Sound Studio, Smart Listening,
Output Devices, AI Hub, AI DJ, and Studio retain their functions. The main
workspace uses a large rounded canvas and expands to use the horizontal
space recovered from the old 228px navigation sidebar. Child components are
reflowed/expanded, not merely scaled as screenshots. CYN's own blue accent,
liquid transitions, DSP, neural/AI systems and settings remain unchanged.

Riff's music-player streaming/download code and Electron runtime are NOT
included in CYN. Its CSS cannot directly style native JUCE controls.
Windows MSVC CI and display checks should be run before publishing.
