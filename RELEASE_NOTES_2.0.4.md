# Cyn 2.0.4 — GitHub Auto-Update Repair

**Fix:** Windows PowerShell 5.1 returned GitHub's list of releases as a single nested array. The Cyn 2.0.2/2.0.3 updater therefore skipped all versions and incorrectly displayed "up to date" even while newer versions existed.

- Repairs release enumeration in `CynUpdater.ps1` so the updater discovers each release separately.
- Adds a live regression test using the GitHub Releases API on Windows PowerShell 5.1; the Windows release pipeline fails if update detection breaks.
- Preserves all 19 native Cyn themes, including **ChatGPT Desktop**.
- Keeps SHA-256 checks on downloaded installer assets and user confirmation before installation.
- Maintains Cyn's settings and user data across in-place upgrades.

**Existing installations:** Cyn 2.0.2 and 2.0.3 cannot automatically discover the repair because their installed updater is the broken version. Install **Cyn-Setup-2.0.4-x64.exe** once manually. Future releases should then be detected normally through Cyn's Check for updates menu.
