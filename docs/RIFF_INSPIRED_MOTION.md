# CYN expressive motion — Riff-inspired desktop pass

CYN remains a native JUCE audio enhancer. This iteration references the interaction language of the open-source [Riff](https://github.com/rootscripts/riff) music player (Material 3 expressive shapes, morphing navigation, tonal hover states, and tactile controls). The implementation is original C++ rendering code: no Riff assets, Electron source files, or external dependencies are embedded.

## Changes

- Native sidebar's selected Material 3 pill glides and reshapes between Sound Studio, Smart Listening, Output Devices, AI Hub, and AI DJ instead of jumping.
- Real-time tonal hover states for all six sidebar destinations, including CYN Studio.
- Sound Studio's Overview / Equalizer / Enhancements selector shares the interruptible morphing indicator.
- Linear sliders now draw a detached, pill-shaped thumb with a track gap and short hover expansion.
- On fresh installs, existing CYN Button Studio defaults to Ripple Press and Glass Sweep. Previously saved animation preferences continue to override the defaults.
- The established content-only liquid page transition, current theme palettes, DSP engine, native layouts, BrainScan, CYN Studio and reduced-motion mode remain untouched.
- The animation state advances via the existing UI timer and never touches the audio thread. Rapid navigation retargets the current position directly instead of accumulating separate transitions.

## Validation

- Pure C++ motion contract: `g++ -std=c++17 -Wall -Wextra -Werror -O2 tests/cyn_expressive_motion_test.cpp -o /tmp/cyn-expressive-motion && /tmp/cyn-expressive-motion`
- GitHub pull request checks: CYN Responsive UI Motion and native Windows build, Power Swallow UI Regression, Neural Animation Validation.
- Manual Windows checks: switch sidebar pages rapidly, switch Equalizer/Enhancements rapidly, hover controls, toggle reduced motion, test focus via keyboard, test EQ and effects sliders while audio plays, verify older saved Button Studio choices persist, and compare startup behavior on low-end GPU.

## Scope

Desktop-only design adaptation, not a rewrite of CYN or a change to the Android UI. Android's existing Material 3 / BrainScan work is maintained separately.
