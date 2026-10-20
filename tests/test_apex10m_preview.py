"""Standalone end-to-end validation of APEX 10M's DSP-only preview CLI."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import numpy as np
import soundfile as sf


class PreviewTests(unittest.TestCase):
    def test_stereo_file_processing(self):
        script = Path(__file__).resolve().parents[1] / "apex_engine" / "apex10m_preview.py"
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            t = np.arange(48000, dtype=np.float32) / 48000
            center = 0.24 * np.sin(2 * np.pi * 440 * t)
            side = 0.06 * np.sin(2 * np.pi * 3200 * t)
            audio = np.column_stack((center + side, center - side))
            before = root / "in.wav"
            after = root / "out.wav"
            sf.write(before, audio, 48000, subtype="PCM_24")
            completed = subprocess.run(
                [sys.executable, str(script), str(before), str(after),
                 "--strength", "0.6"],
                capture_output=True, text=True, timeout=30, check=True,
            )
            enhanced, rate = sf.read(after, dtype="float32", always_2d=True)
            unchanged, _ = sf.read(before, dtype="float32", always_2d=True)
            self.assertEqual(rate, 48000)
            self.assertEqual(enhanced.shape, unchanged.shape)
            self.assertLess(float(np.max(np.abs(enhanced))), 1.0)
            self.assertGreater(float(np.mean(np.abs(enhanced - unchanged))), 0.00001)
            self.assertIn("deterministic DSP test only", completed.stdout)
            self.assertTrue(np.isfinite(enhanced).all())


if __name__ == "__main__":
    unittest.main()
