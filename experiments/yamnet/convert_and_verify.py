#!/usr/bin/env python3
"""Reproducible YAMNet export / cross-runtime parity validation.

Experimental conversion only. Never replaces Cyn's 209-weight personal model
or changes real-time Windows audio callbacks until native integration passes.
"""
from pathlib import Path
import json
import os
import subprocess
import sys
import time

import numpy as np

OUT=Path(os.environ.get("YAMNET_OUTPUT","yamnet-native-output"))
OUT.mkdir(parents=True,exist_ok=True)
SAMPLE_RATE=16000
WAVE_SAMPLES=15600
REF_URL="https://tfhub.dev/google/yamnet/1"


def probes():
    t=np.arange(WAVE_SAMPLES,dtype=np.float32)/SAMPLE_RATE
    rng=np.random.default_rng(5317)
    return [
        ("440Hz",.08*np.sin(2*np.pi*440*t)),
        ("silence",np.zeros(WAVE_SAMPLES,dtype=np.float32)),
        ("pulses",(.03*np.sin(2*np.pi*180*t)*(1+np.sin(2*np.pi*3*t))).astype(np.float32)),
        ("noise",(.08*rng.standard_normal(WAVE_SAMPLES)).astype(np.float32)),
    ]


def main():
    import tensorflow as tf
    import tensorflow_hub as hub
    import onnxruntime as ort

    # The official, pretrained Google YAMNet network (not an imitation).
    model=hub.load(REF_URL)

    class Wrapper(tf.Module):
        def __init__(self, yamnet):
            super().__init__()
            self.yamnet=yamnet

        @tf.function(input_signature=[tf.TensorSpec([WAVE_SAMPLES],tf.float32,name="waveform")])
        def __call__(self, waveform):
            scores,embeddings,_=self.yamnet(waveform)
            return {"scores":scores,"embeddings":embeddings}

    wrapped=Wrapper(model)
    signature=wrapped.__call__.get_concrete_function()
    test_sound=probes()[0][1]
    expected=signature(tf.constant(test_sound))
    assert tuple(expected["scores"].shape)==(1,521),expected["scores"].shape
    assert tuple(expected["embeddings"].shape)==(1,1024),expected["embeddings"].shape
    saved=OUT/"savedmodel"
    tf.saved_model.save(wrapped,str(saved),signatures={"serving_default":signature})

    exported=OUT/"yamnet-16k-0p975s.onnx"
    subprocess.run([sys.executable,"-m","tf2onnx.convert",
                    "--saved-model",str(saved),"--opset","17",
                    "--output",str(exported)],check=True)
    assert exported.exists() and exported.stat().st_size>1_000_000

    opts=ort.SessionOptions()
    opts.intra_op_num_threads=2
    opts.inter_op_num_threads=1
    session=ort.InferenceSession(str(exported),opts,
                                 providers=["CPUExecutionProvider"])
    inputs=session.get_inputs()
    outputs=session.get_outputs()
    assert len(inputs)==1,(inputs)
    assert inputs[0].type=="tensor(float)",inputs[0].type
    assert "15600" in str(inputs[0].shape),inputs[0].shape
    assert len(outputs)==2,[x.name for x in outputs]
    report=[]
    for label,x in probes():
        reference=signature(tf.constant(x))
        actual=dict(zip([y.name for y in outputs],
                        session.run(None,{inputs[0].name:x.astype(np.float32)})))
        tf_scores=reference["scores"].numpy().reshape(-1)
        tf_embed=reference["embeddings"].numpy().reshape(-1)
        matched=[]
        for data in actual.values():
            if data.shape==(1,521): matched.append(("scores",data.reshape(-1),tf_scores))
            elif data.shape==(1,1024): matched.append(("embeddings",data.reshape(-1),tf_embed))
        assert len(matched)==2,[(k,v.shape) for k,v in actual.items()]
        values={}
        for name,result,original in matched:
            assert np.isfinite(result).all()
            diff=np.abs(result-original)
            max_diff=float(diff.max())
            avg_diff=float(diff.mean())
            # ONNX floating-point kernels can differ slightly from TensorFlow.
            assert max_diff < (.025 if name=="embeddings" else .015),(label,name,max_diff)
            values[name]={"max_abs_difference":max_diff,"mean_abs_difference":avg_diff}
        report.append({"test_waveform":label,"parity":values})

    speed_sample=probes()[0][1]
    for _ in range(3): session.run(None,{inputs[0].name:speed_sample})
    times=[]
    for _ in range(12):
        start=time.perf_counter()
        session.run(None,{inputs[0].name:speed_sample})
        times.append((time.perf_counter()-start)*1000)
    final={
        "model":REF_URL,
        "onnx_path":exported.name,
        "onnx_size_bytes":exported.stat().st_size,
        "opset":17,
        "waveform_mono_hz":SAMPLE_RATE,
        "waveform_samples":WAVE_SAMPLES,
        "scores":521,
        "embedding_dimensions":1024,
        "inputs":[{"name":x.name,"shape":x.shape,"type":x.type} for x in inputs],
        "outputs":[{"name":x.name,"shape":x.shape,"type":x.type} for x in outputs],
        "runtime":"ONNX Runtime CPU",
        "inference_ms_median":float(np.median(times)),
        "parity":report,
        "status":"tested model conversion; not integrated into Cyn.exe",
        "limits":"Synthetic probes confirm numerical parity, not real-music accuracy, CPU performance on users' Windows PCs, or safe real-time DSP integration."
    }
    (OUT/"conversion-evaluation.json").write_text(json.dumps(final,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(final,indent=2),flush=True)


if __name__=="__main__":
    main()
