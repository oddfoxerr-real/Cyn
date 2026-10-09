# Cyn validation — 9 October 2026

## Passed

- MSVC C++17 tests of pre-effect source analysis: non-destructive input, bass/mid spectral response, stereo anti-phase, 48/96 kHz formats and invalid inputs.
- Neural preference learning tests: sparse effect learning, minimum examples and reversed personal feedback reversing the learned predictions. The supplied starter also distinguishes the tested 60 Hz and 2 kHz tones.
- All 611 native C++/resource project input paths exist. Cyn build, launch and control PowerShell scripts parse successfully.
- Starter evaluation uses separately generated synthetic held-out mixtures; see models/starter-evaluation.json. These results do not establish real-song quality or vocal recognition.

## Limits

Native visual inspection, end-to-end timeline labeling/persistence, real-song listening, Bluetooth/USB reconnect, sleep/resume, clipping and intentional mute/zero-volume tests remain unverified. Existing device/volume safeguards are retained; this does not establish that every disconnect or volume failure is resolved. Optional Go MCP compilation was not run because Go is unavailable. No signed installer was produced; the portable app requires the installed official FxSound signed driver.

## Repeat tests

In a VS 2022 Developer PowerShell opened in the complete source directory:

```powershell
New-Item -ItemType Directory -Force build-tests | Out-Null
cl /nologo /O2 /std:c++17 /EHsc /Idsp\include tests\PersonalAudioTests.cpp /Fe:build-tests\PersonalAudioTests.exe /Fo:build-tests\PersonalAudioTests.obj
.\build-tests\PersonalAudioTests.exe
powershell -NoProfile -ExecutionPolicy Bypass -File .\BUILD_CYN.ps1
```

See README_CYN.md for running, teaching preferences and the existing CLI control bridge.
