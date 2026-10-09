# Cyn 2.0.6 — Native Menu Animation & Automatic Update Flow

## Dropdown animation repair
Cyn 2.0.5 scheduled popup fading before JUCE actually displayed each popup, so dropdown and context menus could still appear instantly. v2.0.6 starts native popup animation **after** the real top-level or nested submenu becomes visible. The 190 ms fade preserves all hitboxes, focus behavior and keyboard navigation. The Smooth animations default is enabled for new installs; Reduce motion continues to override it.

## Staged updater with Cyn animation
Replaces static PowerShell message boxes and the manual installer handoff with a dark themed WPF update window:
- Cyn icon, glowing core, inward-moving particles, two orbiting rings; uses current theme accent and respects Reduce motion.
- Checks official GitHub Releases; real transferred-byte download progress; verifies SHA-256 before executing any installer.
- One **Update now** click handles download, verification, graceful Cyn exit via dedicated Win32 app message, elevated Inno Setup silent installation, and relaunch.
- Shows **Installing** and **Update complete** stages rather than stopping at an installer prompt.
- Automatic periodic checks show a window only when a newer version is available, and require user consent before making changes.
- Cancelling before installation leaves the currently installed app intact. Windows may still display UAC for an admin-installed app.

## Testing
The Windows build pipeline validates the updater's PowerShell syntax, WPF window construction, the GitHub release discovery regression test, compiled application, installer and SHA-256.

Existing installations of v2.0.5 can use their Check for updates action to obtain v2.0.6. The **new self-contained update experience starts in 2.0.6 and applies to subsequent releases**.
