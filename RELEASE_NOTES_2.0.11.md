# Cyn 2.0.11 — Make AI Hub visible in Smart listening

**Fixed:** In the previous Windows screenshot, the AI Hub, Import AI, and Export AI buttons were not visible even though the controls existed in the source.

- **AI Hub** is now a permanent fourth sidebar entry underneath Output devices, available on every Cyn page.
- Smart listening displays an **AI PROFILES** section label and high-contrast **Export AI**, **Import AI**, and **AI Hub** controls. Buttons are explicitly moved to the front of the native component stack, with layout kept inside the 1240x620 Cyn content region.
- The sidebar now displays **Version 2.0.11** so screenshots can confirm whether the new Cyn executable is actually installed.
- AI Hub still opens a native Cyn dialog with a bundled offline community starter profile — no website, GitHub Pages, domain, or hosting required.
- Local training, safe .cynai.json import/export, 19 themes, and GitHub update checking are preserved.
- A regression test asserts the AI navigation and controls are registered, visible and positioned in-bounds.

**Testing:** Launch Cyn, confirm Version 2.0.11 appears in the bottom-left sidebar, then click the new AI Hub sidebar row or go to Smart listening and use Import AI / Export AI. These UI controls have been verified in source/Windows compilation; visual testing on the user's installed Windows desktop is still needed.
