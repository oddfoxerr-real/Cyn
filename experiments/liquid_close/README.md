# CYN Liquid Close — Offline Simulation Study

**Not integrated into native CYN and not a release.** This is a frame-baking experiment to verify the intended **X control expands to fill the entire CYN window -> becomes liquid -> dissolves** sequence. The older effect that collapses *into* X is not what this experiment models.

## Three flow variations

- Cascade (downward melt).
- Vortex (rotating fluid).
- Ripple (wave-driven dissolution).

Run with Python 3.11+ and `python -m pip install numpy pillow`:

```sh
python experiments/liquid_close/bake_fluid.py --preset all --frames 72 --fps 60 --output dist/fluid-simulations
```

Each preset produces:
- `<preset>.gif` and `<preset>-filmstrip.jpg` for review.
- `masks/NNNN.png`: 160x90 single-channel 8-bit alpha surface coverage.
- `flow/NNNN.png`: 160x90 RGB pack, signed velocity R/G encoded as 128 + velocity*16, blue is mask coverage.
- `manifest.json` with frame timing, encoding and phase ranges.

The algorithm combines 2D semi-Lagrangian advection, pressure projection with Jacobi iteration, and *art-directed erosion* of the fluid dye. It is **not** a full 3D fluid simulation, and does not use third-party code from [tmarrec/fluid-simulation](https://github.com/tmarrec/fluid-simulation). The supplied reference uses a much more complex C++/OpenGL volume/free-surface pipeline; this research tests a lightweight **offline** approach first. If approved, native CYN can sample pre-baked masks/flow while audio continues, with an appropriate low-motion fallback.

GitHub Actions creates downloadable artifacts under the **CYN Offline Liquid Motion Simulation** workflow. No Windows installer or release is generated.
