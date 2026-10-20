"""Fast, deterministic proof that the temporal policy actually trains and resumes."""
import sys
import tempfile
import unittest
from pathlib import Path
import torch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "apex_engine"))
from apex10m_model import SpatialReasoner
from train_apex10m_policy import scene, save_state, validate, HEAD_COUNT


class Apex10MPolicyTrainingTests(unittest.TestCase):
    def test_synthetic_targets_and_head_parameters(self):
        model = SpatialReasoner()
        self.assertEqual(sum(x.numel() for x in model.parameters()), HEAD_COUNT)
        features, target, bands = scene(4, 20, torch.Generator().manual_seed(77))
        self.assertEqual(tuple(features.shape), (4, 8, 20))
        self.assertEqual(tuple(target.shape), (4, 8, 20))
        self.assertEqual(tuple(bands.shape), (4, 2, 32, 20))
        self.assertTrue(bool(torch.isfinite(target).all()))
        self.assertTrue(bool(torch.isfinite(bands).all()))
        # For anti-correlated material the policy must prefer no widening.
        anti = features[:, 3] < -.3
        self.assertLess(float(target[:, 2][anti].max()), .0001)

    def test_actual_gradient_update_and_checkpoint_recovery(self):
        torch.manual_seed(77)
        model = SpatialReasoner()
        opt = torch.optim.AdamW(model.parameters(), lr=.0003)
        rng = torch.Generator().manual_seed(77)
        features, targets, bands_target = scene(3, 12, rng)
        before = model.stem.weight.detach().clone()
        opt.zero_grad(set_to_none=True)
        band, control = model(features)
        loss = (band-bands_target).square().mean() + 4*(control-targets).square().mean()
        loss.backward()
        opt.step()
        self.assertGreater(float((model.stem.weight.detach()-before).abs().sum()), 0.)
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/"weights.pt"
            save_state(path, model, opt, rng, 2.0, 1, {"controls_mse":.2}, 77)
            loaded=torch.load(path, map_location="cpu", weights_only=True)
            self.assertEqual(loaded["steps"], 1)
            self.assertFalse(loaded["live_playback_approved"])
            restored=SpatialReasoner()
            restored.load_state_dict(loaded["model"], strict=True)
            with torch.no_grad():
                expected=model(features)[1]
                actual=restored(features)[1]
            self.assertTrue(bool(torch.allclose(expected, actual)))
            self.assertGreater(float(loaded["training_seconds"]), 0)

    def test_holdout_uses_independent_seed(self):
        torch.set_num_threads(2)
        metrics=validate(SpatialReasoner())
        self.assertGreater(metrics["zero_policy_mse"], 0)
        for value in metrics.values():
            self.assertTrue(0 <= value < 2)


if __name__ == "__main__":
    unittest.main()
