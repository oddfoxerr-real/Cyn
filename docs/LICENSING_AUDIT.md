# Cyn licensing audit — 2026-10-10

Scope: GitHub `oddfoxerr-real/Cyn` at
`cb05744e2048f658c6e8854a8d736e13621b6602` (v2.0.8), its Windows build/installer
recipes, vendored source, embedded resources, fonts, driver/helpers and optional
MCP dependencies. Earlier local 2.1.0 updater experiments were left untouched in
a separate checkout. This is an evidence-based engineering audit, not a legal
opinion or a certification that every asset is cleared.

## Findings and changes

| Item | Finding | Remediation / remaining work |
| --- | --- | --- |
| FxSound AGPL | Application source headers grant AGPL-3.0-or-later. Root license was present, but useful per-component notices and an explicit UI source link were missing. | Retain original license/headers; add dated modification notice, source/license menu items, no-warranty/redistribution notice, CLI/MCP status source URL, and matching release source packaging. |
| Gilroy | Three TTFs and the compiled BinaryData arrays included Gilroy. No application-embedding or redistribution grant was found. Embedded metadata includes an unofficial repack attribution as well as Radomir Tinkov's copyright. | Remove these three files from the current source tree and replace all embedded bytes, symbols, resource hashes, sizes and project references with OFL Manrope. Do not treat an upstream inclusion or a free download as permission. |
| Other fonts | Noto CJK/Thai/Arabic, Montserrat Alternates and IBM Plex Sans Arabic metadata identifies OFL licensing; standalone notices were missing. | Add full family license texts, exact binary hashes and extracted copyright notices. Preserve original font names and bytes. Ship the Installer/Resources language fonts too; the earlier staging script omitted them. |
| JUCE | Pinned JUCE 6.1.6 offers GPL v3 for GUI/graphics and ISC for core/events. Calling the entire framework AGPL is inaccurate. | Preserve GPL/ISC headers and add the GPL text, ISC notice and component mapping. GPLv3/AGPLv3 section 13 permits their combination while retaining each component's license. Mark Cyn's existing modified JUCE popup implementation. |
| libpng/JPEG/zlib | Included through compiled JUCE graphics/core; binary package lacked consolidated notices. | Ship their actual vendored notices, including the required IJG acknowledgement. Preserve the other vendored module notices, including FLAC/Ogg. |
| Driver | FxSound has a public driver repository with AGPL plus a Microsoft Limited Public License v1.1 notice. The latter includes a platform restriction. The driver source repository alone does not establish which sources produced the signed 14.1.0.0 binary. | Preserve the working signed files, record upstream source revision and both notices. Ask for exact corresponding source/build inputs and clarification of license compatibility or additional permission. **Unresolved.** |
| Driver helpers | `fxdevcon` source exists in Installer/DfxInstall; exact shipped binary correspondence is unverified. DfxInstall.cpp invokes `DfxSetupDrv.exe`; it is not the source of that executable. | Preserve helpers and hash manifest. Obtain corresponding source or an explicit applicable redistribution grant for the exact DfxSetupDrv binary. **Unresolved.** |
| Resources | The separately pinned `fxsound2/Resources` snapshot has community translation/preset content and no license file. Bundling it does not establish that every contribution is AGPL. | Retain all presets/translations and provenance; obtain an explicit license/permission for that revision. Do not silently relabel or delete it. **Unresolved.** |
| Microsoft runtime | Bundled release VCRUNTIME140.dll origin is recorded, with Microsoft signature verification; static runtime is also linked into executables. | Record Microsoft's redistribution conditions. The publisher's applicable VS/Build Tools license entitlement must be established. A valid signature is not that evidence. **Unresolved.** |
| UI/art assets | Cyn branding is original project work. Upstream icons remain in the source tree with their provenance; no independent asset-clearance evidence was found for every historical resource. | Preserve notices and asset hashes. Use the neutral display name Charcoal Desktop for the screenshot-inspired theme, keeping its numeric ID, colors and behavior. This is an affiliation precaution, not a finding that the old name was unlawful. |
| Optional MCP | Nine direct/indirect Go/runtime license texts were not collected with the optional binary. | Add version-specific official license copies under licenses/mcp. MCP functionality is unchanged. |
| Release workflow | Previously published releases still contain the old font and may have unresolved driver/resource issues. New commits alone do not repair old release binaries. | Hold new CI publication until the questions in licenses/release-review.json are resolved with evidence; local builds remain possible. Do not overwrite immutable tags or claim old installers were fixed. |

## Preservation

All existing theme palette values, numeric theme IDs, animations, adaptive audio,
local training, presets, output routing, EQ, CLI/MCP and the integrated updater
are retained. The primary text face changes from Gilroy to Manrope: it has a
similar geometric sans-serif style, but is not pixel-identical. The charcoal
theme is renamed only in its display label. No driver is uninstalled, rebuilt,
re-signed or modified. No personal settings/training data is touched.

## Required follow-up to close the audit

1. From FxSound, obtain the source revision/build recipe corresponding to the
   shipped signed driver and fxdevcon, plus the DfxSetupDrv source or explicit
   redistribution terms. Ask how the AGPL and Microsoft Limited Public License
   notices apply together; do not assume the combination is resolved.
2. Obtain the Resources repository's redistribution license covering the pinned
   translations and community presets.
3. Confirm the publisher's applicable Microsoft runtime redistribution entitlement.
4. Once resolved, attach the evidence to the release-review entries, validate the
   updated UI on Windows and issue a new version with its matching source archive.
   Separately decide how to retire/replace historical downloads containing Gilroy;
   removing files from the current branch does not purge Git history or releases.

These questions need rights-holder or publisher evidence; code edits cannot
invent permissions. No request has been sent to FxSound or another third party.

## Evidence

- https://github.com/fxsound2/fxsound-app/blob/d8e7a23d37ed5939c2a3090a1c1756c7f2500b17/LICENSE
- https://www.gnu.org/licenses/agpl-3.0.en.html (sections 5, 6 and 13)
- https://github.com/juce-framework/JUCE/tree/2f980209cc4091a4490bb1bafc5d530f16834e58
- https://tinkov.info/gilroy.html (links to licensed purchase; no grant for Cyn found)
- Exact pinned Google Fonts URLs: ../licenses/sources.json
- https://github.com/fxsound2/fxsound-driver/tree/c78fc6d031d16bd0a5dbbdff4871cfb8715d343d
- https://github.com/fxsound2/Resources/tree/8f0385f015cd6b10026b2db6a0625a6db4de89b0
- https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution

Validation results are recorded separately in LICENSING_VALIDATION.md. Build and
static checks establish engineering properties, not missing legal permissions.
