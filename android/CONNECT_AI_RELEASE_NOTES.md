# CYN Mobile 0.2.25 preview

Fixes update discovery failing with GitHub API HTTP 403 rate limits. Checks the public repository release feed first; uses the existing paginated API only if the feed is unavailable or contains no Android releases. APKs still come from the same GitHub releases, with required SHA-256, package, version-code and signing-certificate checks before installation.

Downloads remain bounded and verified even when feed metadata has no APK size. The updater now reports unknown download progress honestly and avoids blaming the connection for every failure.

Includes the 0.2.24 AUDONIC restoration, Audius layout fixes and all 11 Material 3 palettes. Preserves settings, package identity, certificate and existing releases. Version code: 29.

Verified by release workflow: live public feed discovery from 0.2.23, simulated blocked API, fallback/error cases, AI inference/native DSP, Connect mapping, Android build/lint, signature and package checks against 0.2.22–0.2.24.

For older installations currently blocked by GitHub rate limiting, download this APK directly from the release and install over CYN. No uninstall is required. Physical-device installation remains unverified in CI.
