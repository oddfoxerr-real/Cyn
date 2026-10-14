// SPDX-License-Identifier: AGPL-3.0-or-later
// Reproducible synthetic-only training. Run on the PREVIOUS starter checkpoint
// to generate the identical v3 weights, then commit the resulting files.
import {readFileSync,writeFileSync} from "node:fs";
import {resolve,dirname} from "node:path";
import {fileURLToPath} from "node:url";
const root=resolve(dirname(fileURLToPath(import.meta.url)),"..");
const read=p=>JSON.parse(readFileSync(resolve(root,p),"utf8"));

function trainStarter(base,hub) {
 const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
 const randomizer=(seed)=>{let s=seed|0;return()=>{s^=s<<13;s^=s>>>17;s^=s<<5;return (s>>>0)/4294967296}};
 const bands=[
  [1.25,1.58,.78,.24,.10,.04,.02,.014],
  [1.32,1.48,.95,.38,.17,.07,.09,.045],
  [.13,.20,.43,.67,.54,.24,.52,.60],
  [.08,.15,.31,.62,.95,1.04,.55,.10],
  [.08,.10,.26,.45,.90,1.10,.78,.60],
  [.035,.035,.06,.12,.26,.49,.80,1.25]
 ];
 function generate(seed,n) {
  const rand=randomizer(seed),samples=[];
  for(let k=0;k<n;k++){
   const sub=Math.pow(rand(),.8)*1.25,kick=Math.pow(rand(),.68)*1.25,snare=rand()*.75;
   const vocal=Math.pow(rand(),.9)*1.15,instrument=rand()*1.05,hat=rand()*.85;
   const dull=rand(),hiss=rand(),gains=[sub,kick,snare,vocal,instrument,hat];
   const powers=Array.from({length:8},(_,i)=>{
    let v=.025;
    for(let j=0;j<6;j++)v+=gains[j]*bands[j][i];
    v*=1-.62*dull*i/7;
    if(i===7)v+=.24*hiss*vocal;
    if(i>=6)v+=.06*hiss*hat;
    return Math.max(.0001,v*Math.exp((rand()-.5)*.32));
   });
   const total=powers.reduce((a,b)=>a+b,0);
   const features=powers.map(v=>Math.log(v/total));
   const crest=clamp(.8+.56*kick+.34*snare+.21*hat+(rand()-.5)*.48,0,4);
   const width=clamp(.17+.43*instrument+.27*hat-.13*vocal+(rand()-.5)*.22,0,2);
   const rms=clamp(-3.15+.32*sub+.29*kick+.37*instrument+.25*vocal+(rand()-.5)*.85,-8,0);
   const raw=[...features,crest,width,rms];
   const x=raw.map((v,i)=>i<8?(clamp(v,-12,0)+6)/6:i===8?clamp(v,0,4)/2-1:i===9?clamp(v,0,2)-1:(clamp(v,-8,0)+4)/4);
   const low=powers[0]+powers[1]+powers[2], mid=powers[3]+powers[4]+powers[5],lowPresence=low/(low+mid+1e-9);
   const kickShare=kick/(kick+sub+.35*vocal+.12),rumble=sub/(sub+kick+.12);
   const thin=clamp((.41-lowPresence)*2,0,1);
   const bass=clamp(.56+.34*kickShare*(1-.35*rumble)+.10*thin-.19*rumble-.06*vocal-.025*snare,.27,.78);
   const sibilance=clamp(.55*hiss+.25*hat+.2*(1-dull),0,1);
   const clarity=clamp(.44+.30*vocal*(.65+.35*dull)-.14*sibilance*vocal-.075*hat+.085*dull+.025*kick,.27,.80);
   samples.push({x,raw,bass,clarity});
  }
  return samples;
 }
 function infer(w,x){
  const h=new Float64Array(12);
  for(let j=0;j<12;j++){let v=w[132+j];for(let i=0;i<11;i++)v+=w[j*11+i]*x[i];h[j]=Math.tanh(v);}
  const p=[];
  for(const output of [0,4]){let v=w[204+output];for(let j=0;j<12;j++)v+=w[144+output*12+j]*h[j];p.push(1/(1+Math.exp(-clamp(v,-30,30))));}
  return {h,p};
 }
 function evaluate(w,rows){
  let clarity=0,bass=0,maxBass=0,meanBass=0;
  for(const r of rows){const p=infer(w,r.x).p;clarity+=(p[0]-r.clarity)**2;bass+=(p[1]-r.bass)**2;maxBass=Math.max(maxBass,p[1]);meanBass+=p[1];}
  return {clarity_mse:clarity/rows.length,bass_mse:bass/rows.length,mean_mse:(clarity+bass)/(2*rows.length),max_bass:maxBass,mean_bass:meanBass/rows.length};
 }
 const initial=base.weights.slice();
 if(initial.length!==209||!initial.every(Number.isFinite))throw Error("Invalid 209-weight starter");
 const train=generate(20261030,2500),valid=generate(20261031,500),test=generate(20261032,500);
 const oldValid=evaluate(initial,valid),oldTest=evaluate(initial,test),rand=randomizer(20261101);
 const w=initial.slice(),m=new Float64Array(209),v=new Float64Array(209);
 let best=w.slice(),bestScore=Infinity,bestStep=0;
 for(let step=1;step<=2500;step++){
  const g=new Float64Array(209);
  for(let b=0;b<128;b++){
   const sample=train[Math.floor(rand()*train.length)],x=sample.x,{h,p}=infer(w,x);
   for(let channel=0;channel<2;channel++){
    const o=channel===0?0:4,target=channel===0?sample.clarity:sample.bass;
    const d=(p[channel]-target)/256;
    g[204+o]+=d;
    for(let j=0;j<12;j++){
     const index=144+o*12+j;
     g[index]+=d*h[j];
     const dh=d*w[index]*(1-h[j]**2);
     g[132+j]+=dh;
     for(let i=0;i<11;i++)g[j*11+i]+=dh*x[i];
    }
   }
  }
  const a=1-.9**step,beta=1-.999**step;
  for(let i=0;i<209;i++){
   if((i>=156&&i<192)||(i>=205&&i<208))continue;
   const grad=g[i]+.00001*(w[i]-initial[i]);
   m[i]=.9*m[i]+.1*grad;v[i]=.999*v[i]+.001*grad*grad;
   w[i]-=.0028*(m[i]/a)/(Math.sqrt(v[i]/beta)+1e-8);
   if(!Number.isFinite(w[i])||Math.abs(w[i])>50)throw Error("Diverging weights");
  }
  if(step%100===0){const score=evaluate(w,valid);
   if(score.mean_mse<bestScore&&score.max_bass<.88&&score.mean_bass>.31&&score.mean_bass<.70){best=w.slice();bestScore=score.mean_mse;bestStep=step;}
  }
 }
 const newValid=evaluate(best,valid),newTest=evaluate(best,test);
 if(!bestStep||newTest.clarity_mse>oldTest.clarity_mse*.9||newTest.bass_mse>oldTest.bass_mse*.9||
    newValid.clarity_mse>oldValid.clarity_mse*.9||newValid.bass_mse>oldValid.bass_mse*.9)
   throw Error("Synthetic holdout gate: "+JSON.stringify({oldTest,newTest,bestStep}));
 const picked=[];
 for(const [effect,key] of [[0,"clarity"],[4,"bass"]]){
  const sorted=train.slice().sort((a,b)=>a[key]-b[key]);
  for(const row of sorted.slice(0,40))picked.push({x:row.raw,effect,more:false,id:-1-picked.length,starter:true});
  for(const row of sorted.slice(-40))picked.push({x:row.raw,effect,more:true,id:-1-picked.length,starter:true});
 }
 const model={...base,name:"Cyn starter 3 (beat & vocal feature-trained)",weights:best,ready:[true,false,false,false,true],examples:picked};
 const profile={...hub,profile:{...hub.profile,description:"Beat-conscious bass and vocal presence, trained on synthetic spectral mixtures, not real recordings. Fine-tune with More/Less."},model:{...hub.model,weights:best,ready:model.ready}};
 const report={model:model.name,training:{optimizer_steps:2500,selected_checkpoint:bestStep,train:train.length,validation:valid.length,test:test.length,seed_train:20261030,seed_validation:20261031,seed_test:20261032},validation:{before:oldValid,after:newValid},heldout:{before:oldTest,after:newTest},limitations:"Entirely synthetic source-mixture features and heuristic soft preference targets. No human-labeled real music, beat timestamps, vocal separation, audible AB test, or output true-peak measurement."};
 return {model,profile,report};
}
if(read("models/cyn-starter.json").name.includes("starter 3")) {
 throw Error("Already a v3 model: reproduce with v2's cyn-starter.json rather than retraining the released model.");
}
const result=trainStarter(read("models/cyn-starter.json"),read("ai-hub/community/cyn-starter.cynai.json"));
for(const [path,obj] of [
 ["models/cyn-starter.json",result.model],
 ["ai-hub/community/cyn-starter.cynai.json",result.profile],
 ["models/starter-evaluation-v3.json",result.report]
])writeFileSync(resolve(root,path),JSON.stringify(obj,null,2)+"\n","utf8");
console.log(JSON.stringify(result.report,null,2));
