# Cyn

Personal audio enhancement for Windows, built on the open-source FxSound engine.

## Thank you, FxSound

Cyn builds on the work of the [FxSound team and contributors](https://github.com/fxsound2/fxsound-app).
Thank you for making FxSound open source and providing the audio engine that makes
Cyn possible. If you enjoy Cyn, please [visit FxSound](https://www.fxsound.com/) and
[support their work](https://www.fxsound.com/contribute) through donations or contributions.

## Download

Open [Releases](https://github.com/oddfoxerr-real/Cyn/releases) and download the
latest **Cyn-Setup-2.0.29-x64.exe**. Exit any running Cyn or FxSound using its menu
before installing. Windows 10/11 on Intel/AMD x64 and administrator access are required.

Setup includes presets, fonts, the starter model and the original signed FxSound
virtual audio driver. An existing healthy driver is reused. Cyn has Start-menu
and optional Desktop shortcuts and can be removed through Windows Settings.
Uninstall keeps personal settings/training data and the shared audio driver.

## Features

- **Misc → AI DJ:** optional 91 MB pretrained audio model, downloaded inside Cyn.
  Remixes the playing song with beat repeats, rhythmic echoes and filter sweeps. Includes
  intensity, spacing, stereo movement, cancellation and removal. See
  [AI DJ](docs/AI-DJ.md) for setup and experimental timing limitations.

- Custom Cyn interface and icons, selectable themes, animations and reduced-motion option.
- Presets, output selection, effects and equalizer using the FxSound DSP.
- Smart listening with sensitivity control and a small local neural preference model.
- A recent-audio timeline for teaching More/Less Bass, Clarity, Ambience, Surround,
  and Dynamic Boost. Training remains local; the timeline does not upload audio.
- Existing local command-line controls and optional MCP source.

The starter model has synthetic training/validation only. Real-song quality,
hardware reconnection behavior and clean-PC installer/uninstaller integration
still need testing. The app and setup are unsigned community builds; the included
driver retains its Microsoft signature. **Built-in update delivery is not implemented.**
Releases currently provide manual downloads.

## Build

Clone to a short Windows path. JUCE 6.1.6, presets and resources are included.
Install Visual Studio 2022 C++ Build Tools and the Windows SDK, then run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\BUILD_CYN.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\BUILD_INSTALLER.ps1
```

If build tools are missing, `SETUP_BUILD_TOOLS.ps1` can install the prerequisites.
The installer script obtains a pinned, signature-verified Inno Setup compiler
when needed. Output: `dist/Cyn/Cyn.exe` and `dist/Cyn-Setup-2.0.1-x64.exe`.

See [the user guide](README_CYN.md), [model card](models/MODEL_CARD.md),
[installer details](Installer/Cyn/README.md), and [validation](VALIDATION.md).
Optional MCP compilation requires Go; see `BUILD_CYN.ps1 -WithMcp`.

## Credits and licensing

**Original FxSound authorship:** FxSound LLC and the original contributors
created the underlying FxSound DSP, driver and FxSound-derived sources. Their
original copyrights and required notices remain in the corresponding files.

**CYN modifications:** CYN's interface, AI features and other independent
changes are modifications distributed by CYN contributors. They must not be
misrepresented as original FxSound work or officially endorsed by FxSound.

**License and no warranty:** The modified application is distributed under
GNU AGPL-3.0-or-later; see the complete [LICENSE](LICENSE), retained source
notices, and third-party component licenses. The license provides a no-warranty
disclaimer and limits of liability to the extent permitted by law (sections
15-17). Crediting FxSound does not transfer responsibility for CYN-specific
changes to FxSound or eliminate a distributor's applicable legal obligations.



Cyn is a modified community version of [FxSound](https://github.com/fxsound2/fxsound-app),
not an official FxSound release. FxSound's DSP and virtual driver retain their
original identity and notices. Source snapshot: `d8e7a23d37ed5939c2a3090a1c1756c7f2500b17`.
Resources snapshot: `8f0385f015cd6b10026b2db6a0625a6db4de89b0`.
JUCE snapshot: `2f980209cc4091a4490bb1bafc5d530f16834e58`.

See [LICENSE](LICENSE) for AGPL-3.0 terms and retained notices in individual files.
JUCE has its [own license](third_party/JUCE/LICENSE.md). Bundled fonts, drivers and
Microsoft runtime components retain their respective licenses and notices.
Upstream documentation is retained in [docs/UPSTREAM_README.md](docs/UPSTREAM_README.md).
