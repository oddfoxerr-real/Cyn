# Cyn 2.0.10 — AI Hub directly inside Cyn (no website)

This release replaces the unwanted separate AI Hub website with a **native Windows panel inside Cyn**.

- Open **Smart listening → AI Hub** to browse community profiles, read details, and import a selected neural model — all inside Cyn.exe, not a web browser.
- The installer includes an offline starter catalogue (`CynAiProfiles/`), so browsing and importing built-in AI profiles works without internet.
- **Refresh community** optionally reads a tiny public JSON catalogue from the existing Cyn GitHub repository. Community models are downloaded from GitHub only after you choose to import them.
- **Train my model**, **Export AI**, and **Import AI** remain entirely local and need no login, cloud AI or server.
- **Reviews** and **Share my AI** link to existing GitHub Issue discussions when requested; no GitHub Pages, domain, or hosted backend is required.
- The earlier AI Hub web files and GitHub Pages deployment workflow have been removed.
- All 19 themes, audio DSP and existing updater remain included.

**Safety:** Import validates the fixed 209-weight model format, refuses invalid or oversized data, and backs up existing learning preferences first. Shared AI packages contain no audio samples, private training labels or listening history.

Windows CI verifies the native build, packaged AI catalogue, model schema, installer and SHA-256. Live interaction still needs testing on Windows.
