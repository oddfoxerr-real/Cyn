"""Compare the native frontend to Hugging Face's reference, then run real ONNX.
Development-only dependencies: numpy, transformers==4.46.3, onnxruntime==1.19.2.
No Python runtime or model weights are included in the Cyn installer.
"""
from pathlib import Path
import hashlib,json,subprocess,time,sys
import numpy as np
from transformers.audio_utils import mel_filter_bank,window_function,spectrogram
import onnxruntime as ort
root=Path(__file__).resolve().parents[1]
stage=root/'.build-tools/dj'
model=stage/'ast-audioset-int8.onnx'
assert hashlib.sha256(model.read_bytes()).hexdigest()=='807d244b58a30eaa89f0a721fb841619c696857aabe4eac93769bd0b61497f61'
filters=np.pad(mel_filter_bank(256,128,20,8000,16000,norm=None,mel_scale='kaldi',triangularize_in_mel_space=True),((0,1),(0,0)))
window=window_function(400,'hann',periodic=False)
options=ort.SessionOptions();options.intra_op_num_threads=2;options.inter_op_num_threads=1
session=ort.InferenceSession(str(model),options,providers=['CPUExecutionProvider'])
t=np.arange(128000)/16000
rng=np.random.default_rng(821)
cases={'silence':np.zeros(128000),'tone':0.1*np.sin(2*np.pi*440*t),'noise':rng.normal(0,.04,128000),
       'synthetic_rhythm':(0.22*np.sin(2*np.pi*110*t)+0.1*np.sin(2*np.pi*220*t))*(np.exp(-((t*2)%1)*12))}
labels=json.loads((stage/'config.json').read_text())['id2label']
results=[]
for name,wave in cases.items():
    wave=wave.astype(np.float32);wave.tofile(stage/'wave.f32')
    subprocess.run([str(root/'build-tests/cyn-dj-test.exe'),str(stage/'wave.f32'),str(stage/'features.f32'),str(stage/'native-test.txt')],check=True)
    native=np.fromfile(stage/'features.f32',dtype=np.float32).reshape(1,1024,128)
    mel=spectrogram(wave,window,frame_length=400,hop_length=160,fft_length=512,power=2.0,center=False,preemphasis=.97,mel_filters=filters,log_mel='log',mel_floor=1.192092955078125e-7,remove_dc_offset=True).T
    reference=(np.pad(mel,((0,1024-len(mel)),(0,0)))+4.2677393)/(2*4.5689974)
    error=float(np.max(np.abs(native[0]-reference)));assert error<2e-6,(name,error)
    start=time.perf_counter();scores=session.run(None,{'input_values':native})[0];latency=time.perf_counter()-start
    assert scores.shape==(1,527) and np.isfinite(scores).all()
    reference_logits=session.run(None,{'input_values':reference[None].astype(np.float32)})[0]
    logit_error=float(np.max(np.abs(scores-reference_logits)))
    probabilities=1/(1+np.exp(-scores[0]))
    reference_probabilities=1/(1+np.exp(-reference_logits[0]))
    probability_error=float(np.max(np.abs(probabilities-reference_probabilities)))
    # Dynamic INT8 activation quantisation can amplify one-ULP frontend
    # differences in raw logits. Bound the actual classifier probabilities
    # and require the same top class and DJ gating decision on every fixture.
    assert probability_error<0.01,(name,probability_error)
    assert np.argmax(probabilities)==np.argmax(reference_probabilities)
    gate=lambda p:bool(p[137]>=.35 and max(p[0],p[4],p[5])<.25)
    assert gate(probabilities)==gate(reference_probabilities)
    print(name,'frontend error',error,'probability error',probability_error,flush=True)
    top=np.argsort(probabilities)[-3:][::-1]
    results.append({'fixture':name,'max_feature_error':error,'max_logit_error':logit_error,'max_probability_error':probability_error,'inference_seconds':latency,
                    'top_classes':[{ 'label':labels[str(i)],'score':float(probabilities[i])} for i in top]})
report={'model_sha256':hashlib.sha256(model.read_bytes()).hexdigest(),'model_bytes':model.stat().st_size,
        'runtime':ort.__version__,'input_shape':[1,1024,128],'output_classes':527,'synthetic_checks':results,
        'limitations':'Synthetic numeric and inference checks, not a real-music quality or game-dialogue benchmark.'}
(root/'tests/data/dj/ast-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
