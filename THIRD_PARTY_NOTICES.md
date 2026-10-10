# Cyn: copyright, licenses and corresponding source

Cyn is an independent modified version of FxSound. Thank you to FxSound LLC and
the FxSound contributors for the DSP, audio routing and application foundation.
Support their work at https://www.fxsound.com/contribute . No affiliation or
endorsement by FxSound, Microsoft, OpenAI or the font authors is implied.

The Cyn/FxSound application sources carrying the AGPL notice are available under
AGPL-3.0-or-later. You may modify and redistribute them under those terms. There
is NO WARRANTY, to the extent permitted by law. Read `LICENSE`. This notice does
not relicense third-party files or resolve the outstanding items below.

## Source and modifications

- Cyn source and build scripts: https://github.com/oddfoxerr-real/Cyn
- Release source: use the source archive alongside the matching installer at
  https://github.com/oddfoxerr-real/Cyn/releases . Keep source, binaries and build
  scripts from the same release together; a later branch is not a substitute.
- FxSound app base: `fxsound2/fxsound-app`, commit
  `d8e7a23d37ed5939c2a3090a1c1756c7f2500b17`.
- JUCE base: 6.1.6, commit `2f980209cc4091a4490bb1bafc5d530f16834e58`.
- FxSound Resources base: `8f0385f015cd6b10026b2db6a0625a6db4de89b0`.
- See `MODIFICATIONS.md` for the dated Cyn modification notice, and
  `docs/LICENSING_AUDIT.md` for unresolved distribution questions.

## Components

| Component | Applicable notices and status |
| --- | --- |
| FxSound app, DSP and audio passthrough | AGPL-3.0-or-later per source headers; copyright FxSound LLC and other retained authors. Root `LICENSE`. |
| JUCE 6.1.6 GUI, graphics, data structures, cryptography, OpenGL | GPL v3 option in the actual module headers, not a blanket AGPL grant. `licenses/GPL-3.0.txt`, `JUCE-LICENSE.md`; original headers preserved. AGPL/GPL v3 section 13 permits this combination while retaining each part's license. |
| JUCE core/events | ISC; `licenses/JUCE-ISC.txt`. |
| libpng | PNG Reference Library License v2; `licenses/libpng-LICENSE.txt`. |
| Independent JPEG Group | This software is based in part on the work of the Independent JPEG Group. `licenses/IJG-JPEG-README.txt`. |
| zlib | Jean-loup Gailly and Mark Adler; zlib license, `licenses/zlib-NOTICE.txt`. |
| Other vendored JUCE modules | Original source notices remain in `third_party/JUCE`; these include Ogg/Vorbis, FLAC, Oboe and VST SDK terms. Merely vendored modules are not all linked into Cyn. FLAC and Ogg notices are also included in `licenses`. |
| Manrope | Copyright 2018 The Manrope Project Authors; SIL OFL 1.1. Static 400/600/700 instances replace Gilroy. `licenses/Manrope-OFL.txt`; upstream variable font and generation instructions included. |
| Noto Sans CJK, Thai, Arabic | SIL OFL 1.1. Copyrights vary by exact binary; see `licenses/FONT-COPYRIGHTS.txt` and the Noto license files. |
| Montserrat Alternates | The Montserrat Project Authors; SIL OFL 1.1, `licenses/MontserratAlternates-OFL.txt`. |
| IBM Plex Sans Arabic | IBM Corp.; SIL OFL 1.1, Reserved Font Name Plex; `licenses/IBMPlexSansArabic-OFL.txt`. Binaries are unmodified. |
| Cyn icons, interface, themes, synthetic starter model | Cyn additions; distributed under the application license. Model training recipe and scope are in `models/MODEL_CARD.md` and `training`. System fonts such as Segoe UI are referenced through Windows, not copied into the package. |
| Upstream icons, translations and presets | Original files/attribution retained. Separate Resources repository has no license file at the pinned revision; clarification is outstanding. Do not infer a new license from this table. |
| FxSound virtual driver and helpers | Unmodified upstream binaries. Driver repository offers AGPL and Microsoft Limited Public License v1.1 material; copies are in `licenses/FxSound-driver-*`. Exact source-to-binary correspondence, helper licensing and compatibility need confirmation. Microsoft signing alone is not redistribution permission. |
| Microsoft Visual C++ runtime | Microsoft copyright; proprietary redistributable terms, not AGPL. See `licenses/Microsoft-runtime-NOTICE.md`. |
| Inno Setup | Jordan Russell / Martijn Laan; `licenses/Inno-Setup-LICENSE.txt`. Its compiler is a build tool; installer stub retains its own terms. |
| Optional Go MCP bridge | Source remains available. Dependency licenses are in `licenses/mcp`; they apply when the optional bridge is built. Go runtime and dependency notices must accompany that binary. |

Fonts remain under OFL, independently of the application's AGPL. Font binary
hashes, embedded copyright metadata and authoritative license URLs are recorded
under `licenses`. Do not delete notices when repackaging Cyn.
