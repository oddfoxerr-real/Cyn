"""APEX 10M architecture prototype: inherited APEX-6M + stereo scene reasoner.

This module defines neural capacity, not pretrained spatial quality. The additional
weights must be trained and evaluated before being enabled in live audio.
No third-party source or model weights are copied into this file.
"""
from __future__ import annotations

import torch
from torch import nn

APEX6M_PARAMETER_COUNT = 6_307_477
APEX10M_PARAMETER_COUNT = 10_001_469
SPATIAL_FEATURE_COUNT = 8
SPATIAL_BANDS = 32
CONTROL_NAMES = (
    "dialogue_priority", "spatial_activity", "width_target",
    "transient_protection", "bass_protection", "sibilance_protection",
    "directional_motion", "decision_confidence",
)


class SpatialReasoner(nn.Module):
    """Learn stereo context across time and predict bounded policy controls.

    This is a task-specific learned policy, not a language model or symbolic AI.
    State can be carried across consecutive inference windows in a later
    streaming implementation. Stereo phase is NEVER overwritten by this module.
    """

    def __init__(self) -> None:
        super().__init__()
        self.stem = nn.Conv1d(SPATIAL_FEATURE_COUNT, 128, 1)
        self.sequence = nn.GRU(128, 320, num_layers=2, bidirectional=True,
                               batch_first=True)
        self.refine = nn.Linear(640, 640)
        self.policy = nn.GRU(640, 160, batch_first=True)
        self.mixing = nn.Linear(640, 2 * SPATIAL_BANDS)
        self.context = nn.Linear(160, len(CONTROL_NAMES))
        self.policy_projection = nn.Linear(160, 128)
        self.coherence_mlp = nn.Sequential(
            nn.Linear(128, 768), nn.GELU(), nn.Linear(768, SPATIAL_BANDS)
        )

    def forward(self, features: torch.Tensor) -> tuple[torch.Tensor, torch.Tensor]:
        if features.ndim != 3 or features.shape[1] != SPATIAL_FEATURE_COUNT:
            raise ValueError("spatial features must have shape [batch,8,frames]")
        if features.shape[2] < 1:
            raise ValueError("spatial features cannot have zero frames")
        if not torch.isfinite(features).all():
            raise ValueError("spatial features must be finite")
        time_features = torch.tanh(self.stem(features.clamp(-12.0, 12.0)))
        encoded, _ = self.sequence(time_features.transpose(1, 2))
        refined = encoded + 0.2 * torch.tanh(self.refine(encoded))
        reasoning, _ = self.policy(refined)
        controls = torch.sigmoid(self.context(reasoning)).transpose(1, 2)
        common_bands = self.coherence_mlp(
            torch.tanh(self.policy_projection(reasoning))
        )
        common_bands = common_bands.unsqueeze(2).expand(-1, -1, 2, -1)
        common_bands = common_bands.reshape(
            features.shape[0], features.shape[2], 2 * SPATIAL_BANDS
        )
        bands = torch.tanh(self.mixing(refined) + 0.05 * common_bands)
        bands = bands.reshape(features.shape[0], features.shape[2], 2,
                              SPATIAL_BANDS).permute(0, 2, 3, 1)
        return bands, controls


class Apex10M(nn.Module):
    """Combine the *existing* independently verified Apex6M with a new head.

    The base must be loaded from the pinned, hash-verified pretrained checkpoint.
    The new spatial branch has no published trained weights yet. These raw
    outputs are for architecture/training tests only, NOT playback activation.
    """

    def __init__(self, apex6m: nn.Module) -> None:
        super().__init__()
        base_params = sum(parameter.numel() for parameter in apex6m.parameters())
        if base_params != APEX6M_PARAMETER_COUNT:
            raise ValueError(f"Expected {APEX6M_PARAMETER_COUNT} base parameters; got {base_params}")
        self.base = apex6m
        self.spatial = SpatialReasoner()
        parameters = sum(parameter.numel() for parameter in self.parameters())
        if parameters != APEX10M_PARAMETER_COUNT:
            raise AssertionError(f"APEX 10M architecture drift: {parameters}")

    def forward(self, log_magnitude: torch.Tensor,
                spatial_features: torch.Tensor):
        if log_magnitude.ndim != 3 or log_magnitude.shape[1] != 513:
            raise ValueError("base features must have shape [channels,513,frames]")
        if spatial_features.ndim != 3 or spatial_features.shape[2] != log_magnitude.shape[2]:
            raise ValueError("spatial and base feature frame counts must match")
        mask, scene_logits = self.base(log_magnitude)
        spatial_bands, controls = self.spatial(spatial_features)
        return mask, scene_logits, spatial_bands, controls
