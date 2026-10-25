"use strict";
/*
 * CYN native BrainScan for the Material desktop renderer.
 * Port of the geometric/live-activation rules from CynBrainScan3D.h and
 * CynNeuralActivity.h (AGPL-3.0-or-later). Input is real, locally sampled
 * 11→12→5 neural inference and six AUDONIC creative values.
 * Rendering never opens an audio device, records audio or invents activity.
 */
(() => {
 const canvas=document.getElementById("brain-canvas");
 const ctx=canvas.getContext("2d",{alpha:false});
 const state=document.getElementById("brain-state");
 const profile=document.getElementById("brain-profile");
 const controls=[...document.querySelectorAll("[data-brain-mode]")];
 let mode="scan", sample=null,lastSample=0,lastPoll=0,lastDraw=0,angle=.12;
 const colors=["#ff9674","#ffcb83","#ae99ff","#70dbdb","#de93d1","#72ddb5"];
 const clamp=(x,lo,hi)=>Math.max(lo,Math.min(hi,Number.isFinite(Number(x))?Number(x):lo));
 const reduce=()=>document.body.classList.contains("reduce-motion")||
   window.matchMedia("(prefers-reduced-motion: reduce)").matches;
 const live=()=>Boolean(sample?.active)&&Date.now()-lastSample<1300;
 function isShowing(){
   return !document.hidden&&document.getElementById("view-ai").classList.contains("active");
 }
 function size(){
   const rect=canvas.getBoundingClientRect(),dpr=Math.min(2,window.devicePixelRatio||1);
   if(rect.width<20||rect.height<20)return null;
   const w=Math.round(rect.width*dpr),h=Math.round(rect.height*dpr);
   if(canvas.width!==w||canvas.height!==h){canvas.width=w;canvas.height=h}
   ctx.setTransform(dpr,0,0,dpr,0,0);
   return {w:rect.width,h:rect.height};
 }
 // Fixed geometry is topology artwork. Intensities come from native data.
 const nodes=[];
 for(let side=0;side<2;side++)for(let i=0;i<90;i++){
   const longitude=i*2.399963229728653;
   const v=(i+.5)/90;
   const latitude=Math.acos(1-2*v);
   const x=.62*Math.sin(latitude)*Math.cos(longitude)+(side===0?-.42:.42);
   const y=.84*Math.cos(latitude);
   const z=.67*Math.sin(latitude)*Math.sin(longitude);
   nodes.push({x,y,z,side,i});
 }
 const edges=[];
 for(let i=0;i<nodes.length;i++){
   const dist=[];
   for(let j=0;j<nodes.length;j++){
     if(i===j||nodes[i].side!==nodes[j].side)continue;
     const a=nodes[i],b=nodes[j];
     const d=(a.x-b.x)**2+(a.y-b.y)**2+(a.z-b.z)**2;
     dist.push({j,d});
   }
   dist.sort((a,b)=>a.d-b.d);
   for(const x of dist.slice(0,3))if(i<x.j)edges.push([i,x.j]);
 }
 function background(w,h){
   ctx.fillStyle="#18191c";
   ctx.fillRect(0,0,w,h);
   const grad=ctx.createRadialGradient(w*.5,h*.5,8,w*.5,h*.5,Math.max(w,h)*.62);
   grad.addColorStop(0,"#292a2e");grad.addColorStop(1,"#18191c");
   ctx.fillStyle=grad;ctx.fillRect(0,0,w,h);
 }
 function dot(x,y,r,color,alpha=1){
   ctx.globalAlpha=clamp(alpha,0,1);
   ctx.fillStyle=color;ctx.beginPath();ctx.arc(x,y,r,0,Math.PI*2);ctx.fill();
   ctx.globalAlpha=1;
 }
 function brain(w,h,now){
   const isLive=live(),data=sample||{};
   if(isLive&&!reduce())angle+=.009;
   const scale=Math.min(w*.265,h*.38),cx=w*.5,cy=h*.48;
   const projected=nodes.map(n=>{
     const sin=Math.sin(angle),cos=Math.cos(angle);
     const x=n.x*cos-n.z*sin,z=n.x*sin+n.z*cos;
     const tiltedY=n.y*.985-z*.17;
     const depth=n.y*.17+z*.985;
     const perspective=2.7/(2.7+depth*.34);
     return {x:cx+x*scale*perspective,y:cy+tiltedY*scale*perspective,depth};
   });
   const audonic=Boolean(data.audonic);
   const levels=nodes.map((n,i)=>{
     if(!isLive)return 0;
     if(audonic)return clamp(data.creative?.[i%6],0,1);
     const h=clamp(data.hidden?.[i%12],-1,1),o=clamp(data.outputs?.[i%5],0,1);
     return clamp(Math.abs(h)*.70+Math.abs(o-.5)*.35,0,1);
   });
   for(const foreground of [false,true]){
     for(const [i,j] of edges){
       if((projected[i].depth>=0)!==foreground)continue;
       const alpha=(foreground?.14:.05)+Math.max(levels[i],levels[j])*(foreground?.54:.19);
       ctx.strokeStyle=audonic?colors[i%6]:"#c9d0db";
       ctx.globalAlpha=alpha;
       ctx.lineWidth=foreground?1.1:.65;
       ctx.beginPath();ctx.moveTo(projected[i].x,projected[i].y);
       ctx.lineTo(projected[j].x,projected[j].y);ctx.stroke();
     }
     for(let i=0;i<nodes.length;i++){
       if((projected[i].depth>=0)!==foreground)continue;
       const light=levels[i],color=audonic?colors[i%6]:"#b8c0ce";
       const radius=(foreground?2.2:1.2)+light*1.5;
       if(isLive&&foreground&&light>.1){
         dot(projected[i].x,projected[i].y,radius*3.6,color,.06+light*.18);
       }
       dot(projected[i].x,projected[i].y,radius,color,
         (foreground?.48:.17)+light*(foreground?.51:.38));
     }
     ctx.globalAlpha=1;
   }
   if(audonic){
     const names=["HARMONICS","ATTACK","WIDTH","COLOUR","RHYTHM","OVERTONES"];
     const cell=(w-40)/6;
     names.forEach((name,i)=>{
       const left=20+i*cell,level=isLive?clamp(data.creative?.[i],0,1):0;
       ctx.fillStyle="#414247";ctx.fillRect(left,h-46,Math.max(4,cell-8),4);
       ctx.fillStyle=colors[i];ctx.globalAlpha=.35+level*.65;
       ctx.fillRect(left,h-46,Math.max(0,cell-8)*level,4);ctx.globalAlpha=1;
       ctx.fillStyle="#aaabb1";ctx.textAlign="center";
       ctx.font="9px system-ui";ctx.fillText(name,left+(cell-8)/2,h-26);
     });
   }
 }
 function neuralGraph(w,h){
   const groups=[11,12,5],spaces=[w*.16,w*.52,w*.86],sizes=groups.map((count,g)=>Array.from({length:count},(_,i)=>({
     x:spaces[g],y:(h*.13)+i*(h*.76)/(count-1)
   })));
   const active=live(),data=sample||{};
   const linkGroups=[
     {from:sizes[0],to:sizes[1],links:data.input_connections,n:11},
     {from:sizes[1],to:sizes[2],links:data.output_connections,n:12}
   ];
   for(const group of linkGroups){
     group.to.forEach((to,j)=>group.from.forEach((from,i)=>{
       const level=active?clamp(Math.abs(group.links?.[j*group.n+i]),0,1):0;
       ctx.globalAlpha=.07+level*.75;ctx.lineWidth=.6+level*1.3;
       ctx.strokeStyle="#a9c6de";ctx.beginPath();ctx.moveTo(from.x,from.y);
       ctx.lineTo(to.x,to.y);ctx.stroke();
     }));
   }
   ctx.globalAlpha=1;
   const arrays=[data.inputs,data.hidden,data.outputs];
   sizes.forEach((group,k)=>group.forEach(({x,y},i)=>{
     const v=active?Math.abs(Number(arrays[k]?.[i])||0):0;
     const level=clamp(v,0,1);
     dot(x,y,5+level*3,k===2?"#9ee8ce":"#c1c8d2",.25+level*.75);
   }));
   ctx.fillStyle="#aaaab0";ctx.font="10px system-ui";ctx.textAlign="center";
   ["11 INPUTS","12 HIDDEN","5 OUTPUTS"].forEach((label,i)=>ctx.fillText(label,spaces[i],h-12));
 }
 function drawRecent(){
   const chart=document.getElementById("learning-waveform");
   if(!chart)return;
   const cx=chart.getContext("2d");
   const rect=chart.getBoundingClientRect();
   if(rect.width<20)return;
   const dpr=Math.min(2,window.devicePixelRatio||1);
   chart.width=Math.round(rect.width*dpr);chart.height=Math.round(rect.height*dpr);
   cx.setTransform(dpr,0,0,dpr,0,0);
   const w=rect.width,h=rect.height;
   cx.fillStyle="#1a1b1e";cx.fillRect(0,0,w,h);
   const newest=Number(document.getElementById("feedback-from").value)||0;
   const oldest=Number(document.getElementById("feedback-to").value)||15;
   cx.fillStyle="#414247";
   cx.fillRect(w*(1-oldest/60),0,w*((oldest-newest)/60),h);
   cx.strokeStyle="#3d3e43";cx.beginPath();cx.moveTo(0,h*.5);cx.lineTo(w,h*.5);cx.stroke();
   const history=Array.isArray(sample?.recent_frames)?sample.recent_frames:[];
   for(const f of history){
     const age=Number(f.age_ms),rms=Number(f.rms);
     if(!Number.isFinite(age)||age<0||age>60000||!Number.isFinite(rms))continue;
     const x=w*(1-age/60000),amplitude=clamp(Math.sqrt(clamp(rms,0,1))*2.3,0,.96);
     cx.fillStyle="#b6c6d0";cx.globalAlpha=.25+amplitude*.65;
     cx.fillRect(x,h/2-amplitude*h*.42,Math.max(2,w/180),amplitude*h*.84);
   }
   cx.globalAlpha=1;
 }
 function frame(now){
   requestAnimationFrame(frame);
   if(!isShowing()||now-lastDraw<(reduce()?450:33))return;
   lastDraw=now;
   const dims=size();if(!dims)return;
   background(dims.w,dims.h);
   if(mode==="scan")brain(dims.w,dims.h,now);
   else neuralGraph(dims.w,dims.h);
   state.textContent=live()?"LIVE NATIVE INFERENCE":"WAITING FOR AUDIO / AI";
   profile.textContent=String(sample?.profile||"Native CYN").toUpperCase()+
     (sample?.training?" · LEARNING":"");
 }
 async function poll(){
   if(!isShowing()||typeof window.cyn?.telemetry!=="function")return;
   if(lastPoll) return;
   lastPoll=Date.now();
   try{
     const result=await window.cyn.telemetry();
     if(result?.schema===1 && Array.isArray(result.hidden)&&result.hidden.length===12){
       sample=result;lastSample=Date.now();
       drawRecent();
     }
   }catch{}finally{lastPoll=0;}
 }
 controls.forEach(b=>b.addEventListener("click",()=>{
   mode=b.dataset.brainMode;
   controls.forEach(other=>{
     other.classList.toggle("active",other===b);
     other.setAttribute("aria-pressed",String(other===b));
   });
 }));
 window.addEventListener("resize",()=>{lastDraw=0});
 setInterval(poll,210);
 requestAnimationFrame(frame);
 window.cynBrain=Object.freeze({getActivity:()=>live()});
})();
