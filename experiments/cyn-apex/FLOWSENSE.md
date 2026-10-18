# APEX FlowSense v0.1

FlowSense analyses changing musical structure over seconds rather than classifying isolated 1-second chunks. It outputs 1-second intensity, onset activity, low/high band energy, rhythmic pulse candidates, build/impact/breakdown event candidates.

This is **not** an emotion-recognition model and is **not yet trained** to choose sound enhancements. APEX-6M's actual trained weights remain the synthetic denoiser; FlowSense provides an independent measurable temporal analysis layer. No audio driver or current AI Hub profile is modified.

Run from `experiments/cyn-apex`:

```sh
python -m unittest discover -s tests -p 'test_flow.py' -v
python -m apex.flow demo/synthetic_noisy_input.wav --output reports/flow.json
```

It uses SciPy and NumPy. No audio content leaves the computer. Event thresholds are heuristics tested on synthetic signals; human listening validation and real-music structural annotations are still required.
