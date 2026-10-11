# Draft for FxSound — not sent

Hello FxSound team,

Thank you for making FxSound open source. Cyn is a community fork retaining your
credits and AGPL notices: https://github.com/oddfoxerr-real/Cyn . We are auditing
distribution before publishing another installer and would appreciate clarification:

1. Which source revision and build instructions correspond to the signed x64
   `fxvad.sys` 14.1.0.0 package in fxsound-app? How do the AGPL notice and Microsoft
   Limited Public License v1.1 in fxsound-driver apply to that package?
2. What source/build recipe corresponds to the bundled `fxdevcon64.exe`? Where can
   we obtain the source or explicit redistribution terms for `DfxSetupDrv.exe`?
   We found DfxInstall.cpp, but that file invokes the helper rather than building it.
3. Under which license may we redistribute the translations and community presets
   from Resources commit `8f0385f015cd6b10026b2db6a0625a6db4de89b0`?

The exact files and hashes are recorded in our `Installer/Cyn/driver-sha256.json`.
We intend to preserve driver behavior and signing, retain every applicable notice,
and provide corresponding source with the application. Please identify any
additional terms or permissions required for these components.

Thank you.
