"""CYN APEX-6M: spectrogram denoising + scene cues; independent experimental model.

No pretrained weights are claimed. Inference runs offline, not in CYN's audio callback.
"""
from __future__ import annotations
import torch
from torch import nn
import torch.nn.functional as F

SAMPLE_RATE = 48000
FFT_SIZE = 1024
HOP_SIZE = 256
SCENE_NAMES = ("bass", "percussion", "melody", "speech_like", "noise", "hiss")


class TemporalBlock(nn.Module):
    def __init__(self, channels: int = 224):
        super().__init__()
        self.norm = nn.GroupNorm(1, channels)
        self.depthwise = nn.Conv1d(channels, channels, 7, padding=3, groups=channels)
        self.pw1 = nn.Conv1d(channels, channels * 4, 1)
        self.pw2 = nn.Conv1d(channels * 4, channels, 1)
        self.gamma = nn.Parameter(torch.tensor(0.1))

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        y = self.pw2(F.gelu(self.pw1(self.depthwise(self.norm(x)))))
        return x + self.gamma * y


class Apex6M(nn.Module):
    def __init__(self, channels: int = 224, layers: int = 14):
        super().__init__()
        freq = FFT_SIZE // 2 + 1
        self.pre = nn.Sequential(nn.Conv1d(freq, channels, 3, padding=1), nn.GELU())
        self.blocks = nn.Sequential(*(TemporalBlock(channels) for _ in range(layers)))
        self.mask_head = nn.Sequential(nn.Conv1d(channels, channels, 3, padding=1), nn.GELU(), nn.Conv1d(channels, freq, 1))
        self.scene_head = nn.Sequential(nn.Linear(channels, 128), nn.GELU(), nn.Linear(128, len(SCENE_NAMES)))

    def forward(self, log_magnitude: torch.Tensor):
        if log_magnitude.ndim != 3 or log_magnitude.shape[1] != FFT_SIZE // 2 + 1:
            raise ValueError("Expected [batch, 513 FFT bins, time frames]")
        state = self.blocks(self.pre(log_magnitude))
        mask = torch.sigmoid(self.mask_head(state))
        scene_logits = self.scene_head(state.mean(dim=-1))
        return mask, scene_logits


def stft(audio: torch.Tensor) -> torch.Tensor:
    return torch.stft(audio, n_fft=FFT_SIZE, hop_length=HOP_SIZE, window=torch.hann_window(FFT_SIZE, device=audio.device), return_complex=True)


def istft(spec: torch.Tensor, length: int) -> torch.Tensor:
    return torch.istft(spec, n_fft=FFT_SIZE, hop_length=HOP_SIZE, window=torch.hann_window(FFT_SIZE, device=spec.device), length=length)


def forward_audio(model: Apex6M, waveform: torch.Tensor):
    spectrum = stft(waveform)
    mask, logits = model(torch.log1p(spectrum.abs()))
    restored = istft(spectrum * mask, waveform.shape[-1])
    return restored, logits, mask


def count_parameters(model: nn.Module) -> int:
    return sum(p.numel() for p in model.parameters() if p.requires_grad)