# Cyn 2 — first public preview

Native Windows x64 audio app with Cyn branding, selectable themes, UI motion,
local preference learning and the FxSound audio engine.

## Thank you, FxSound

Cyn builds on the work of the [FxSound team and contributors](https://github.com/fxsound2/fxsound-app).
Thank you for making FxSound open source and providing the audio engine that makes
Cyn possible. If you enjoy Cyn, please [visit FxSound](https://www.fxsound.com/) and
[support their work](https://www.fxsound.com/contribute) through donations or contributions.

## Installation and preview notes

Download **Cyn-Setup-2.0.1-x64.exe**, exit any running Cyn/FxSound from its menu,
then run setup. The installer includes the supporting files and signed audio driver;
an existing healthy driver is reused. Setup revision 2.0.1 adds a real Retry/Cancel
dialog when an audio app is running. The audio app payload is unchanged from Cyn v2.

Settings and personal model data remain in `%APPDATA%\Cyn`. Cyn uninstall preserves
that data and the shared driver. This preview has no built-in updater yet.

Validation: native release build passed; installer compilation, signature/catalog
checks, read-only driver/process detection and simulated driver setup tests passed.
The complete clean-PC install/uninstall flow and the new Retry dialog have not been
tested interactively. Real-song model performance and hardware reconnect handling
remain experimental. Cyn.exe and setup are unsigned; the driver is Microsoft-signed.

Source: this repository, with bundled JUCE/resources and preserved upstream licenses.
The additional source archive is a full snapshot for building this release.
