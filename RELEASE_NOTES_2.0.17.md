# Cyn 2.0.17 — measured neural activity and update reliability

- The neural panel displays one real inference snapshot: 11 normalized inputs,
  12 hidden activations, 5 preference outputs and signed weighted contributions.
  The audio controller uses the same snapshot. Clock-driven particles and artificial
  training glow are removed. Silence/stale capture clears the live display.
- Training telemetry samples the candidate's real weights at completed epochs.
  Preview, waiting, training and active-adjustment states are distinguished.
- Updating from an extracted old copy targets the registered installation and
  relaunches that verified binary. The updater considers both installed and running
  versions, serializes updates, and verifies the installed version before success.
- A timed-out graceful exit offers **Close stuck Cyn & retry**. Force-closing requires
  that explicit action and is restricted to the same-session processes seen at timeout.
- Setup rejects a downgrade and concurrent installers. Existing user startup/shortcut
  entries are repaired to the installed path without enabling startup for users who
  disabled it. Old archived executables remain old; launch Cyn from its Start shortcut.
- Retains themes, DSP, presets, AI Hub, local learning and CLI/MCP. Includes the
  existing licensing audit changes (Manrope replacement and third-party notices).

Android has an independent **0.1.1-preview** release with measured neural telemetry,
training snapshots, effect-control diagnostics, optional legacy output-mix compatibility
and a platform-specific update check. The original signing key and package ID are retained.

Windows public binary publication remains subject to the unresolved distribution
evidence in `licenses/release-review.json`. Local build validation does not resolve
those licensing questions. See `docs/UPDATE_FIX_VALIDATION.md` for checks and limits.
