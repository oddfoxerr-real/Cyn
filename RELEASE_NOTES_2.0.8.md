# Cyn 2.0.8 — Expanding Category Menus

This update adds the **actual category drop-down movement** requested:

- Click **Theme**, **Save New Preset**, or **Rename Preset** in Cyn's menu.
- Instead of spawning a sideways flyout, the category expands **in place**.
- Text-button options slide out from behind the category header, stacked one below another with a 23 ms stagger and cubic easing.
- Menu items below are displaced naturally as the category grows; clicking the category again collapses options in reverse order.
- Large categories scroll vertically rather than breaking into columns.
- Keyboard support and actual option actions are preserved.
- Smooth animations is the motion toggle; Reduce motion disables animation but retains the expandable menu structure.
- All 19 themes and the v2.0.7 staged GitHub updater remain intact.

Windows C++ compilation and installer checks are performed by GitHub Actions. A real-user visual and interaction test on Windows is recommended to validate timing and layout at different screen sizes.
