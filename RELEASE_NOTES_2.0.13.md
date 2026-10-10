# Cyn 2.0.13 — fix blank AI buttons and square tooltip corners

- Fixed alpha-zero buttons and text labels in Smart listening's Export AI, Import AI and AI Hub actions and AI Hub sidebar button.
- Fixed the black rectangular artifacts around rounded tooltips by opting out of JUCE's default opaque tooltip backing.
- Preserved opaque tooltip fill, border and text, across all themes.
- Added regression assertions to the existing AI Hub CI test, before compilation and release.
- Preserved user training, offline community profiles, DSP, and updates. This release does not perform any personal model training.

Verify on Windows that buttons show their labels and that tooltips have clean rounded corners at different scaling factors.
