# Cyn v2.0.9 — Separate AI Hub and shareable AI profiles

**Native app**
- New **Export AI** and **Import AI** buttons in **Smart listening**.
- Interchange format `.cynai.json`, compatible with Cyn's actual 209-weight 11x12x5 neural model. Full validation of the schema, readiness flags, values and architecture on import.
- Automatic backup of the existing local preferences before importing a public profile. No user training examples, audio recordings, timestamps or device metadata are exported.
- Existing More / Less feedback and native background training remain available for adapting imported models.

**Cyn AI Hub**
- New self-contained GitHub Pages project in `ai-hub/`, distinct from the Windows launcher UI.
- Browse community profiles; download and inspect models in the local browser; edit profile name, author and description; re-export without private labels.
- Offline web trainer can retrain from a user-selected private `preferences.json`, using the same network architecture and Adam optimizer. The private file stays in the user's browser; it is never posted.
- First curated community entry: Cyn Starter (with all private training examples stripped).
- Community publishing uses GitHub submissions and maintainers' review; user feedback and reviews use GitHub issue threads. Star averages require an actual review backend and are not fabricated.

**Hosting**
- Github Pages workflow included. A fully separate GitHub repository can host this self-contained site after the user creates or connects that repository.

The installer and source continue to meet AGPL-3.0-or-later requirements.
