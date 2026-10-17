"""Export the EXISTING 225-step, 6,307,477-parameter APEX checkpoint to ONNX.
No training or random reinitialization; downloaded bytes are SHA-256 verified.
"""
from __future__ import annotations
import hashlib, io, json, pathlib, sys, urllib.request, zipfile

UPSTREAM = ("https://github.com/oddfoxerr-real/Cyn/releases/download/"
            "cyn-apex6m-flowsense-research-20261017/"
            "CYN-APEX-6M-FlowSense-Research.zip")
EXPECTED_ZIP_SHA256 = "9287d06243f057c94415186018c04b14211fa2abe67cdf013541e26c7abf4675"
CHECKPOINT_NAME = "cyn-apex/weights/apex6m-225.pt"

def main():
    import torch
    import onnx
    import onnxruntime as ort
    import numpy as np
    output = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "dist-apex/apex6m-225.onnx")
    output.parent.mkdir(parents=True, exist_ok=True)
    print("Downloading previously released APEX checkpoint (no training)...", flush=True)
    req = urllib.request.Request(UPSTREAM, headers={"User-Agent": "CYN-APEX-Exporter"})
    with urllib.request.urlopen(req, timeout=180) as response:
        blob = response.read(80_000_000)
    if hashlib.sha256(blob).hexdigest() != EXPECTED_ZIP_SHA256:
        raise RuntimeError("APEX source ZIP SHA-256 mismatch; refusing export")
    with zipfile.ZipFile(io.BytesIO(blob)) as z:
        # Published releases may have a different top-level directory.
        # Exact pinned archive hash still protects the contents.
        def unique_ending(suffix):
            matches=[name for name in z.namelist() if name.endswith(suffix)]
            if len(matches)!=1:
                raise RuntimeError(f"APEX archive must contain exactly one {suffix}; found {matches}")
            return matches[0]
        source=z.read(unique_ending("apex/model.py"))
        checkpoint_bytes=z.read(unique_ending("weights/apex6m-225.pt"))
    # Load ONLY audited plain model source from pinned ZIP into isolated namespace.
    namespace={"__name__":"apex_checkpoint_model"}
    exec(compile(source, "apex/model.py", "exec"), namespace)
    Apex6M = namespace["Apex6M"]
    model = Apex6M()
    state = torch.load(io.BytesIO(checkpoint_bytes), map_location="cpu", weights_only=True)
    assert state["status"] == "synthetic-only"
    assert state["training_steps"] == 225
    assert state["params"] == 6_307_477
    assert state["sample_rate"] == 48000
    model.load_state_dict(state["weights"], strict=True)
    model.eval()
    assert sum(p.numel() for p in model.parameters()) == 6_307_477

    example = torch.rand(1, 513, 80, dtype=torch.float32)
    with torch.no_grad():
        torch.onnx.export(
            model, example, str(output), opset_version=17,
            input_names=["log_magnitude"], output_names=["mask", "scene_logits"],
            dynamic_axes={"log_magnitude":{0:"channels",2:"frames"},
                          "mask":{0:"channels",2:"frames"},
                          "scene_logits":{0:"channels"}},
            do_constant_folding=True, dynamo=False)
    graph = onnx.load(str(output))
    onnx.checker.check_model(graph)
    session = ort.InferenceSession(str(output), providers=["CPUExecutionProvider"])
    for batch,frames in ((1,80),(2,25),(1,193)):
        x = torch.rand(batch,513,frames)
        with torch.no_grad():
            expected = model(x)
        actual=session.run(["mask","scene_logits"], {"log_magnitude":x.numpy()})
        for output_name,y_ref,y in zip(("mask","scene_logits"),expected,actual):
            error = float(np.max(np.abs(y_ref.numpy()-y)))
            if error > 0.0005:
                raise AssertionError(f"{output_name} PyTorch/ONNX mismatch {error}")
            print(f"PASS ONNX parity: {batch=} {frames=} {output_name=} max_error={error:.7f}", flush=True)
    report={
      "name":"CYN APEX-6M","format":"onnx","parameter_count":6_307_477,
      "input":"float32 [channels,513,frames] = log1p(abs(48-kHz 1024-FFT 256-hop STFT))",
      "outputs":["mask","scene_logits"],
      "checkpoint_training_steps":225,"new_training_steps":0,
      "checkpoint_training_data":"synthetic-only",
      "quality_compared_to_real_music":False,
      "sha256":hashlib.sha256(output.read_bytes()).hexdigest(),
      "size_bytes":output.stat().st_size,
    }
    (output.parent/"apex6m-manifest.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2),flush=True)

if __name__=="__main__":
    main()
