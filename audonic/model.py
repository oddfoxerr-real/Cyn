"""AUDONIC low-latency student network for six bounded musical FX decisions.

3B is the separate teacher-model research goal; THIS is the trained small
streaming student. Inference consumes the existing CYN 8x20 stereo analysis.
"""
import torch
from torch import nn

INPUT_NAMES=("bass","kick","side","correlation","flux","vocal","headroom","direction")
OUTPUT_NAMES=("saturation","transient_bite","stereo_motion",
              "color_filter","rhythmic_gate","bass_harmonics")

class AudonicStudent(nn.Module):
    def __init__(self):
        super().__init__()
        self.stem=nn.Conv1d(8,64,1)
        self.temporal=nn.Sequential(
            nn.Conv1d(64,64,5,padding=2,groups=64),nn.GELU(),
            nn.Conv1d(64,96,1),nn.GELU(),
            nn.Conv1d(96,96,5,padding=2,groups=96),nn.GELU(),
            nn.Conv1d(96,64,1),nn.GELU(),
            nn.Conv1d(64,64,3,padding=1),nn.GELU())
        self.head=nn.Conv1d(64,len(OUTPUT_NAMES),1)
    def forward(self,features):
        if features.ndim!=3 or features.shape[1]!=8:
            raise ValueError("AUDONIC expects [batch,8,frames]")
        return torch.sigmoid(self.head(self.temporal(self.stem(features))))

def count_parameters(model):
    return sum(p.numel() for p in model.parameters())
