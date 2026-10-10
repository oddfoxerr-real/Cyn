# Cyn 2.0.12 — Fix blank AI Hub and broken Themes menu

- **Fixed transparency:** Cyn's theme palette supplies 24-bit RGB values. The native AI Hub was constructing JUCE colours without setting alpha, making its window, text and controls transparent. All native AI Hub palette colours now explicitly use full alpha and the content surface is opaque.
- **Proper dialog placement:** AI Hub windows now centre over the main Cyn application instead of using desktop-origin positioning.
- **Restored Themes menu:** Disabled the experimental inline accordion implementation in favour of JUCE's stable normal submenus. Theme selection and custom preset submenu actions now use the normal code path. Optional fade-on-open remains available.
- **Added regression checks** for opaque painting, dialog parent anchoring, version consistency, and stable menu handling.
- **Version 2.0.12** remains visible in the lower sidebar to distinguish the fixed installation from older versions.
- Preserved local AI training, import/export, offline models, all existing 19 themes, and the GitHub updater. No website or hosting is needed.

This release prioritises working menus over the earlier unfinished sliding category effect. Visual confirmation is still needed on a Windows installation after CI compilation.
