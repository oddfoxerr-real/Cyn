"""Synthetic starter data, not a real-music benchmark. Requires only NumPy.
Run from source root: python training/generate_starter.py prepare
Then compile/run TrainStarter.cpp as documented in MODEL_CARD.md and run finalize.
"""
from pathlib import Path
import numpy as np, json, sys
root=Path(__file__).resolve().parents[1]
out=root/'training'/'generated'; out.mkdir(exist_ok=True)
sr=16000; n=4096; t=np.arange(n)/sr
freq=np.fft.rfftfreq(n,1/sr); z=np.exp(-2j*np.pi*freq/sr)
responses=[]
for center in [45,75,120,200,400,900,2200,5000]:
    w=2*np.pi*min(center,sr*.4)/sr; alpha=np.sin(w)/1.6
    b=alpha/(1+alpha); a1=-2*np.cos(w)/(1+alpha); a2=(1-alpha)/(1+alpha)
    responses.append(abs((b-b*z*z)/(1+a1*z+a2*z*z))**2)
responses=np.array(responses)

def dataset(seed, count):
    rng=np.random.default_rng(seed); rows=[]
    for i in range(count):
        bass=bool(rng.integers(2)); dull=bool(rng.integers(2))
        f=rng.uniform(105,310); voice=np.zeros(n)
        for h in range(1,24):
            # Vary harmonic rolloff / formants to include low voice-like negatives.
            amplitude=np.exp(-h*rng.uniform(.13,.3))*(.25+np.exp(-((h*f-900)/550)**2))
            voice+=amplitude*np.sin(2*np.pi*f*h*t+rng.uniform(0,6.28))
        voice*=.6+.4*np.sin(2*np.pi*rng.uniform(2,5)*t)**2
        signal=voice*rng.uniform(.3,.9)
        for _ in range(2):
            signal+=rng.uniform(.05,.2)*np.sin(2*np.pi*rng.uniform(350,2800)*t+rng.uniform(0,6.28))
        if bass:
            fb=rng.uniform(38,90)
            envelope=.4+.6*np.sin(2*np.pi*rng.uniform(1,3)*t)**2
            signal+=rng.uniform(.5,1.3)*envelope*(np.sin(2*np.pi*fb*t)+.18*np.sin(4*np.pi*fb*t))
        spectrum=np.fft.rfft(signal)
        if dull: spectrum/=1+(freq/rng.uniform(450,1100))**4
        else: spectrum+=np.fft.rfft(rng.normal(size=n))*rng.uniform(.01,.06)*(freq>1800)
        signal=np.fft.irfft(spectrum,n)
        signal*=rng.uniform(.025,.22)/(np.sqrt(np.mean(signal**2))+1e-12)
        spectrum=np.fft.rfft(signal); power=abs(spectrum)**2
        power[1:-1]*=2; full=power.sum()
        rms=np.sqrt(np.mean(signal**2)); crest=np.log(max(1,np.max(abs(signal))/rms))
        x=np.r_[np.log(1e-6+responses@power/max(full,1e-20)),crest,0,np.log(rms)].tolist()
        rows.append({'x':x,'bass':int(bass),'clarity':int(dull)})
    return rows

if sys.argv[1]=='prepare':
    train=dataset(91271,1800); test=dataset(68134,600)
    (out/'data.json').write_text(json.dumps({'train':train,'test':test}))
    with (out/'train.txt').open('w') as f:
        f.write(str(len(train)*2)+'\n')
        for row in train:
            for effect,key in [(4,'bass'),(0,'clarity')]: f.write(' '.join(map(str,row['x']))+f' {effect} {row[key]}\n')
        f.write(str(len(test))+'\n')
        for row in test: f.write(' '.join(map(str,row['x']))+'\n')
    print('Prepared 1800 training mixtures and 600 held-out mixtures.',flush=True)
elif sys.argv[1]=='finalize':
    data=json.loads((out/'data.json').read_text()); lines=(out/'result.txt').read_text().splitlines()
    weights=list(map(float,lines[0].split())); predictions=np.array([list(map(float,line.split())) for line in lines[1:]])
    assert len(weights)==209 and np.isfinite(weights).all()
    examples=[]
    for i,row in enumerate(data['train'][:80]):
        for effect,key in [(4,'bass'),(0,'clarity')]: examples.append({'x':row['x'],'effect':effect,'more':bool(row[key]),'id':-2*i-effect-1,'starter':True})
    model={'schema':1,'name':'Cyn starter 1','weights':weights,'ready':[True,False,False,False,True],'examples':examples}
    (root/'models').mkdir(exist_ok=True)
    (root/'models'/'cyn-starter.json').write_text(json.dumps(model,indent=2))
    report={}
    for effect,key in [(4,'bass'),(0,'clarity')]:
        truth=np.array([r[key] for r in data['test']]); pred=predictions[:,effect]>=.5
        report[key]={'synthetic_test_accuracy':float(np.mean(pred==truth)), 'false_positive_rate':float(np.mean(pred[truth==0])), 'false_negative_rate':float(np.mean(~pred[truth==1]))}
    report['limitation']='Synthetic held-out data only; not a vocal detector or real-music accuracy measurement.'
    (root/'models'/'starter-evaluation.json').write_text(json.dumps(report,indent=2)); print(json.dumps(report,indent=2))
