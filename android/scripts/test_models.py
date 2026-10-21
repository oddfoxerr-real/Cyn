#!/usr/bin/env python3
"""Numerical preprocessing parity and real inference, not source-string assertions."""
import sys,pathlib,numpy as np,onnxruntime as ort
root=pathlib.Path(__file__).resolve().parents[1];models=root/'app/src/main/assets/models'
n=np.arange(48000);wave=(.2*np.sin(2*np.pi*440*n/48000)+.07*np.sin(2*np.pi*87*n/48000)).astype(np.float32)
idx=np.arange(189)[:,None]*256+np.arange(1024)[None,:]-512
idx=np.where(idx<0,-idx,idx);idx=np.where(idx>=48000,2*47999-idx,idx)
window=.5-.5*np.cos(2*np.pi*np.arange(1024)/1024)
features=np.log1p(np.abs(np.fft.rfft(wave[idx]*window,axis=1))).T.astype(np.float32)
java=np.fromfile(sys.argv[1],dtype='<f4').reshape(513,189)
np.testing.assert_allclose(java,features,rtol=2e-5,atol=2e-5)
opts=ort.SessionOptions();opts.intra_op_num_threads=2;opts.inter_op_num_threads=1
for name,data in [('yamnet-16k-0p975s.onnx',np.sin(np.arange(15600)*2*np.pi*440/16000).astype(np.float32)*.15),('apex6m-225.onnx',java[None])]:
 session=ort.InferenceSession(str(models/name),sess_options=opts,providers=['CPUExecutionProvider'])
 result=session.run(None,{session.get_inputs()[0].name:data})
 assert all(np.isfinite(x).all() for x in result)
 if name.startswith('yamnet'): assert any(x.shape==(1,521) for x in result) and any(x.shape==(1,1024) for x in result)
 else:
  mask=dict(zip([o.name for o in session.get_outputs()],result))['mask'];assert mask.shape==(1,513,189) and mask.min()>=0 and mask.max()<=2
 print('PASS real ONNX inference:',name,[x.shape for x in result])
print('PASS: Java preprocessing matches independent NumPy FFT reference')
