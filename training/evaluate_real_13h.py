#!/usr/bin/env python3
"""Gate Cyn experimental model on identity-separated real-audio holdout."""
from __future__ import annotations
import json
import math
from pathlib import Path
import numpy as np
from train_real_13h import metrics,ROOT

def main():
    out=Path("selected-real-audio")
    out.mkdir(exist_ok=True)
    data={}
    for name in ("train","validation","test"):
        with np.load(Path("real-audio-data")/(name+".npz")) as d:
            data[name]=(d["x"].astype(np.float64),d["y"].astype(np.float64),d["role"])
    latest=json.loads(Path("checkpoint/checkpoint.json").read_text())
    w=np.asarray(latest["weights"])
    existing=json.loads((ROOT/"models/cyn-starter.json").read_text())
    old=np.asarray(existing["weights"])
    if len(w)!=209 or not np.isfinite(w).all():raise ValueError("Model corrupt")
    baseline=metrics(old,*data["test"])
    trained=metrics(w,*data["test"])
    valid_before=metrics(old,*data["validation"])
    valid_after=metrics(w,*data["validation"])
    provenance=json.loads(Path("real-audio-data/provenance.json").read_text())
    assert latest["optimization_seconds"]>=46800,"Fewer than 13 actual hours of gradient training"
    gate=(trained["mean_mse"]<baseline["mean_mse"]*.95
          and trained["clarity_mse"]<baseline["clarity_mse"]*.95
          and trained["bass_mse"]<baseline["bass_mse"]*.95
          and trained["max_bass"]<.86
          and valid_after["mean_mse"]<valid_before["mean_mse"]*.95)
    summary={"source":"genuine CC-BY-4.0 spoken recordings and instrument notes",
      "training_seconds":latest["optimization_seconds"],
      "gradient_steps":latest["gradient_steps"],
      "candidate_heldout":trained,"baseline_heldout":baseline,
      "validation_before":valid_before,"validation_after":valid_after,
      "passed_real_recording_heuristic_gate":bool(gate),
      "actually_better_listening":None,
      "pretrained_yamnet_at_runtime":"existing Cyn integration only",
      "silero_deepfilternet_bundled":False,
      "data_attribution":provenance["reference"],
      "limitations":"Even genuine recordings do not provide ground-truth desired DSP settings. "
        "Current targets are heuristic labels, so accuracy does not establish audible enhancement. "
        "Only instrument notes, not full songs, and voice excerpts were used. "
        "No measured output clipping, latency or blind listening quality."}
    (out/"real-audio-training-report.json").write_text(json.dumps(summary,indent=2)+"\n")
    print(json.dumps(summary,indent=2),flush=True)
    if not gate:raise SystemExit("Independent real-audio evaluation gate failed; do NOT release")
    profile=json.loads((ROOT/"ai-hub/community/cyn-starter.cynai.json").read_text())
    profile["profile"]["name"]="Cyn Real Audio Experimental - 13h"
    profile["profile"]["category"]="experimental"
    profile["profile"]["description"]="Trained on CC BY 4.0 real speech and instrument audio, heuristic preference targets. NOT listening-validated. Optional AI Hub import."
    profile["model"]["weights"]=[float(v) for v in w]
    profile["model"]["ready"]=[True,False,False,False,True]
    assert profile["model"]["architecture"]=="cyn-audio-11x12x5"
    (out/"cyn-real-audio-experimental.cynai.json").write_text(json.dumps(profile,indent=2)+"\n")
    (out/"RELEASE-NOTES.md").write_text(
      "# Cyn Real Audio Experimental - 13-hour CPU training\n\n"
      "This is an **optional experimental AI Hub profile**, not a stable Windows installer.\n\n"
      "Performed 13 cumulative hours of numeric gradient optimization on features "
      "extracted from real CC BY 4.0 speech and recorded instrument clips, and "
      "passed an identity-separated holdout against heuristic preference targets.\n\n"
      "These targets are not listening judgments. There is no proof this improves "
      "phonk, movies, vocal sibilance, output clipping or end-to-end perceived quality. "
      "YAMNet continues to run independently in Cyn; Silero VAD and DeepFilterNet are "
      "not newly integrated into the native playback engine.\n\n"
      "Sources: "+provenance["reference"]+"\n"
    )
if __name__=="__main__":main()
