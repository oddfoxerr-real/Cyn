# Licensing remediation validation

2026-10-10, Windows x64, Visual Studio 2022 MSVC 14.44, JUCE 6.1.6.

- Release build: passed, 0 errors. Initial full compile produced 287 existing
  compiler warnings. Final incremental compile after compatibility edits also
  passed. No installer was run and no driver or personal settings were changed.
- `python tests/test_licensing_assets.py`: 5 tests passed. Embedded font bytes,
  resource lengths and resource hash uniqueness checked; no Gilroy build references;
  font copyright/hash inventory checked; required license bundle and all language
  font packaging sources present.
- `python scripts/check_license_release.py`: failed as intended, listing all
  three unresolved distribution questions. This prevents automated publication
  while leaving local development builds available.
- PowerShell build scripts parsed successfully.
- Original installer driver/helper/runtime hash manifest: all entries match.
  Live Authenticode checks on fxvad.sys and vcruntime140.dll returned Valid with
  Microsoft signers. This verifies integrity, not redistribution permission.
- Git whitespace check passed with `cr-at-eol` for the repository's Windows files.

No new release was published by these checks. Visual inspection of the replacement
font in the running audio application and a clean-machine install remain pending.
Passing a build does not establish the missing driver/resource/runtime rights.
