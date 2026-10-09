# Cyn setup

Setup revision 2.0.1 fixes the running-app screen: it names Cyn/FxSound, provides a real Retry/Cancel dialog, rechecks on Retry, and reports a failed Windows process query separately. Silent installs fail without opening that dialog. The embedded audio app remains Cyn v2. The current output filename is `Cyn-Setup-2.0.1-x64.exe`.

Build from the source root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\BUILD_INSTALLER.ps1
```

This builds the app if `dist/Cyn/Cyn.exe` is absent, obtains the pinned Inno Setup 6.7.3 compiler if needed, verifies driver/helper hashes, and writes `dist/Cyn-Setup-2.0.1-x64.exe`. `-RebuildApp` rebuilds an existing app payload. `-CompilerPath` optionally supplies a preinstalled ISCC.exe. No download is needed when the resulting setup EXE is run.

The setup is for Windows 10/11 on Intel/AMD x64. It installs to Program Files, registers an uninstaller, creates a Start-menu shortcut and an optional Desktop shortcut. It requests administrator access for driver setup. Launch from the Finish page runs as the original user, not elevated. It does not add auto-start entries. Keep `%APPDATA%/Cyn` when uninstalling so personal learning data survives; the shared FxSound driver is also retained.

## Driver provenance and behavior

The unmodified Version14 Windows 10 x64 driver and helper executables come from the FxSound source snapshot `d8e7a23d37ed5939c2a3090a1c1756c7f2500b17`. The INF declares version 14.1.0.0. `driver-sha256.json` pins every binary/driver input. The SYS and CAT have valid Microsoft Windows Hardware Compatibility Publisher signatures; Windows SDK SignTool `/kp /c` verification passed for both INF and SYS. Upstream helper EXEs are unsigned, as is this community Cyn app/setup. Do not describe the whole installer as signed.

`Ensure-AudioDriver.ps1` uses the existing upstream `fxdevcon64 install <inf>` mechanism and `DfxSetupDrv` endpoint initialization. It reuses a present, healthy device without changing it, refuses to replace a disabled/error device, and blocks when device enumeration fails. A first install initializes the endpoint name, GUID and default buffer registry values as upstream does. It does not uninstall older drivers, change physical-device volumes, disable driver signature checks or change power policy. Driver creation is a system operation: cancellation/failure later in app setup can leave the shared driver installed.

The old x86 `DfxSetupDrv.exe` requires `VCRUNTIME140.dll`. The unmodified Microsoft-signed x86 release runtime from Visual Studio 2022 Build Tools `VC/Redist/MSVC/14.44.35112/x86/Microsoft.VC143.CRT` is bundled beside that helper only in setup's temporary directory. It is not a debug runtime, not a separate system runtime install, and not covered by this project's AGPL; Microsoft retains its copyright and distribution terms. [Microsoft local deployment documentation](https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method?view=msvc-170). Cyn.exe and fxdevcon64.exe statically link their C++ runtime; their remaining imports are Windows system DLLs.

## Validation and limits

- Inno Setup successfully compiled the setup EXE with all app resources and driver inputs embedded.
- Driver signatures and catalog membership passed verification.
- Read-only detection passed against the existing FxSound virtual audio device on the development PC.
- `tests/InstallerDriverTests.ps1` passed healthy/missing/disabled/enumeration-error cases; mocked first-install order, full endpoint ID format, repeat-install reuse, and invalid-signature rejection passed without driver writes.
- App and helper PE imports were inspected; the required x86 runtime was included.
- The app was not recompiled for this packaging-only change. The existing Cyn v2 release is embedded unchanged.
- A real clean-machine driver install, setup wizard, upgrade and uninstall have **not** been exercised end to end. The running audio app and current driver were left untouched. A clean Windows VM remains the next integration test.

The optional copy-only `INSTALL_CYN.ps1` is retained for older portable workflows; the setup EXE is the recommended route. Do not run two copies of Cyn/FxSound together. If original FxSound is later uninstalled and removes the shared driver, rerun Cyn setup.

Compiler: [official Inno Setup downloads](https://jrsoftware.org/isdl.php). Source and installers remain together in the release outputs, including the full `Cyn-Source-v2.zip` corresponding source archive.
