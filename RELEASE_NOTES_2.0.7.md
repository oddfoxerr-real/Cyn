# Cyn 2.0.7 — Native menu fade and seamless updater

This release supersedes the separately built 2.0.6 preview.

- Native JUCE ComboBox drop-downs, app menus and submenus now begin fading **when shown**, fixing the earlier animation timing race.
- Smooth animations defaults on for users without a saved preference; Reduce motion remains respected.
- Dedicated material-style updater window with glowing cyan core, rotating rings, inward particles and outward completion sparks.
- Actual download-byte progress and staged updates: checking, downloading, SHA-256 verification, ready, installing and complete.
- Background checks are quiet when already current; newer updates are automatically downloaded and verified.
- One-click **Install & restart** requests a graceful Cyn exit through JUCE single-instance IPC, starts Inno Setup silently after normal Windows UAC approval, then relaunches Cyn.
- Cancelling before installation leaves the existing app in place.
- Adds release discovery and WPF UI loading checks to the Windows release pipeline.

Native compilation and UI parsing are validated by GitHub Actions. A final live Windows interaction test is still recommended.
