"""Tests verifying actual learnable capacity and conservative stereo behavior."""
from __future__ import annotations
import sys
from pathlib import Path
import unittest

import numpy as np
import torch
from torch import nn

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "apex_engine"))
from apex10m_model import (SpatialReasoner, Apex10M, APEX6M_PARAMETER_COUNT,
                           APEX10M_PARAMETER_COUNT)
from apex10m_spatial import spatial_preserve, analyze_stereo


class FakeSixMillion(nn.Module):
    """Fake *shape* of 6M for architecture-only tests, NOT pretrained weights."""
    def __init__(self):
        super().__init__()
        self.base_weights = nn.Parameter(torch.zeros(APEX6M_PARAMETER_COUNT))

    def forward(self, x):
        return x.sigmoid(), x.new_zeros((x.shape[0], 6))


class Apex10MTests(unittest.TestCase):
    def test_exact_trainable_parameter_count(self):
        model = Apex10M(FakeSixMillion())
        self.assertEqual(sum(p.numel() for p in model.parameters()),
                         APEX10M_PARAMETER_COUNT)
        self.assertEqual(sum(p.numel() for p in model.spatial.parameters()),
                         APEX10M_PARAMETER_COUNT - APEX6M_PARAMETER_COUNT)

    def test_spatial_reasoner_outputs_and_gradients(self):
        torch.manual_seed(42)
        model = SpatialReasoner()
        features = torch.randn(1, 8, 7, requires_grad=True)
        bands, controls = model(features)
        self.assertEqual(tuple(bands.shape), (1, 2, 32, 7))
        self.assertEqual(tuple(controls.shape), (1, 8, 7))
        self.assertTrue(bool(torch.all((controls >= 0) & (controls <= 1))))
        (bands.square().mean() + controls.mean()).backward()
        self.assertIsNotNone(model.sequence.weight_ih_l0.grad)
        self.assertIsNotNone(model.coherence_mlp[0].weight.grad)
        self.assertGreater(float(model.coherence_mlp[0].weight.grad.abs().sum()), 0.0)
        self.assertTrue(torch.isfinite(features.grad).all().item())

    def test_base_and_spatial_shapes(self):
        model = Apex10M(FakeSixMillion())
        with torch.no_grad():
            mask, scene, bands, controls = model(torch.randn(2, 513, 4),
                                                  torch.randn(1, 8, 4))
        self.assertEqual(tuple(mask.shape), (2, 513, 4))
        self.assertEqual(tuple(scene.shape), (2, 6))
        self.assertEqual(tuple(bands.shape), (1, 2, 32, 4))
        self.assertEqual(tuple(controls.shape), (1, 8, 4))

    def test_bypass_and_mono_centre_unchanged(self):
        t = np.arange(8192, dtype=np.float32) / 48_000
        tone = 0.3 * np.sin(2 * np.pi * 440 * t)
        stereo = np.column_stack((tone, tone))
        self.assertTrue(np.array_equal(spatial_preserve(stereo, 48000, 1), stereo))
        self.assertTrue(np.array_equal(spatial_preserve(stereo, 48000, 0), stereo))

    def test_stereo_side_enhancement_not_flip(self):
        t = np.arange(48_000, dtype=np.float32) / 48_000
        center = 0.32 * np.sin(2 * np.pi * 400 * t)
        side = 0.10 * np.sin(2 * np.pi * 3000 * t)
        stereo = np.column_stack((center + side, center - side)).astype(np.float32)
        before = analyze_stereo(stereo, 48000)
        enhanced = spatial_preserve(stereo, 48000, 1)
        after = analyze_stereo(enhanced, 48000)
        self.assertEqual(enhanced.shape, stereo.shape)
        self.assertTrue(np.isfinite(enhanced).all())
        self.assertGreater(after["side_to_mid"], before["side_to_mid"])
        self.assertLess(float(np.max(abs(enhanced))), 1.0)
        self.assertLess(float(np.max(abs(enhanced - stereo))), 0.03)

    def test_anti_phase_safe_bypass(self):
        t = np.arange(10240, dtype=np.float32) / 48000
        l = 0.2 * np.sin(2 * np.pi * 440 * t)
        stereo = np.column_stack((l, -l))
        self.assertTrue(np.array_equal(spatial_preserve(stereo, 48000, 1), stereo))


if __name__ == "__main__":
    unittest.main()
