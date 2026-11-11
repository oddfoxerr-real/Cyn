#!/usr/bin/env python3
"""Policy and candidate-registry checks; no heavyweight dependencies."""
from __future__ import annotations
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent


class CandidateContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.catalog = json.loads((ROOT / "catalog.json").read_text(encoding="utf-8"))

    def test_safe_defaults(self):
        policy = self.catalog["policy"]
        self.assertEqual(self.catalog["schema"], 1)
        self.assertEqual(policy["activation"], "research-only")
        self.assertEqual(policy["training"], "disabled-until-evaluation")
        self.assertEqual(policy["release"], "disabled")
        self.assertEqual(policy["audio_upload"], "none")
        self.assertEqual(policy["hardware"], "github-standard-ubuntu-cpu")
        self.assertIs(policy["model_weights_bundled"], False)

    def test_catalog_is_valid(self):
        entries = self.catalog["candidates"]
        self.assertGreaterEqual(len(entries), 10)
        ids = [entry["id"] for entry in entries]
        self.assertEqual(len(ids), len(set(ids)))
        valid_states = {"existing-native", "existing-probe", "new-probe", "research", "blocked"}
        for entry in entries:
            with self.subTest(model=entry["id"]):
                self.assertIn(entry["status"], valid_states)
                self.assertTrue(entry["source"].startswith("https://github.com/"))
                for key in ("name", "role", "code_license", "weights", "notes"):
                    self.assertTrue(entry[key])
        self.assertEqual({entry["id"] for entry in entries if entry["status"] == "new-probe"},
                         {"silero-vad", "deepfilternet"})

    def test_no_unapproved_runtime_activation(self):
        self.assertNotIn("enabled", self.catalog["policy"])
        self.assertFalse(any("auto-release" in item["status"] for item in self.catalog["candidates"]))


if __name__ == "__main__":
    unittest.main()
