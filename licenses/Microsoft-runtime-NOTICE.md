# Microsoft runtime

`Installer/Cyn/runtime-x86/vcruntime140.dll` is an unmodified Microsoft-signed
release runtime copied from Visual Studio 2022 Build Tools
`VC/Redist/MSVC/14.44.35112/x86/Microsoft.VC143.CRT` for `DfxSetupDrv.exe`.
Microsoft retains copyright and its license; the AGPL grant does not cover it.
Cyn.exe and fxdevcon also contain statically linked Microsoft runtime code.

Authoritative redistribution list:
https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution

This list permits specified release runtime files subject to the applicable
Visual Studio license. It is not an unconditional grant to anyone who finds a
DLL. The publisher must establish entitlement under their applicable Visual
Studio / Build Tools license and comply with its distribution requirements.
No debug runtime should be shipped. Retain the original binary unchanged.

The original Microsoft documents and readable text extracts are included as
`Microsoft-Build-Tools-2022.docx` / `.txt` and
`Microsoft-VC-Runtime-2015-2022.docx` / `.txt`, retrieved from Microsoft's
[license directory](https://visualstudio.microsoft.com/license-terms/).
The Build Tools terms distinguish ordinary licensed Visual Studio use from the
more limited open-source-dependency exception. Do not assume that exception
automatically covers developing a substantially modified standalone application.

The audit verified the origin and Microsoft signature previously recorded in
Installer/Cyn/README.md, but did not establish the publisher's license entitlement.
This remains a release-review item, not a claim of infringement.
