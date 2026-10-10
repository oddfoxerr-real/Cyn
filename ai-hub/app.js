/* Cyn AI Hub — offline browser-side training and safe model interchange.
 * No accounts, uploads or tracking. SPDX-License-Identifier: AGPL-3.0-or-later.
 * A shared .cynai.json file includes weights, readiness and public metadata only.
 */
(() => {
  'use strict';
  const EFFECTS = ['Clarity','Ambience','Surround','Dynamic Boost','Bass'];
  const ARCHITECTURE = 'cyn-audio-11x12x5';
  const MODEL_SIZE = 209;
  const el = id => document.getElementById(id);
  const state = {model:null,examples:[],busy:false,catalogue:[]};
  const clamp=(v,min,max)=>Math.max(min,Math.min(max,v));
  const pause=()=>new Promise(resolve=>setTimeout(resolve,0));
  const updateMessage=(message)=>{el('modelStatus').textContent=message;};
  const number=x=>typeof x==='number' && Number.isFinite(x);

  function validateModel(candidate) {
    if (!candidate || typeof candidate !== 'object') throw new Error('No AI model was found.');
    if (candidate.schema !== 1 && candidate.schema !== undefined) throw new Error('Unsupported AI model schema.');
    if (candidate.architecture && candidate.architecture !== ARCHITECTURE)
      throw new Error('This model uses an unsupported architecture.');
    if (!Array.isArray(candidate.weights) || candidate.weights.length !== MODEL_SIZE ||
        !candidate.weights.every(w=>number(w) && Math.abs(w)<=100))
      throw new Error('Expected 209 finite neural weights between -100 and 100.');
    if (!Array.isArray(candidate.ready) || candidate.ready.length !== 5 ||
        !candidate.ready.every(v=>typeof v==='boolean'))
      throw new Error('Expected five boolean trained-effect flags.');
    return {schema:1,architecture:ARCHITECTURE,
      weights:[...candidate.weights],ready:[...candidate.ready]};
  }

  function validateExamples(rows) {
    if (!Array.isArray(rows)) return [];
    const result=[];
    for (const row of rows.slice(0,2208)) {
      if (!row || !Array.isArray(row.x) || row.x.length!==11 ||
          !row.x.every(v=>number(v)) || !Number.isInteger(row.effect) ||
          row.effect<0 || row.effect>=5 || typeof row.more!=='boolean') continue;
      result.push({x:row.x.slice(),effect:row.effect,more:row.more,starter:row.starter===true});
    }
    return result;
  }

  function profileFromJson(json) {
    if (!json || json.format!=='cyn-ai-profile' || json.schema!==1)
      throw new Error('This is not a Cyn AI Hub profile.');
    const model=validateModel(json.model);
    const info=json.profile&&typeof json.profile==='object'?json.profile:{};
    return {model,profile:{
      name:String(info.name||'Imported AI').slice(0,80),
      author:String(info.author||'').slice(0,60),
      description:String(info.description||'').slice(0,500),
      category:['music','gaming','voice','balanced'].includes(info.category)?info.category:'music',
      license:['CC-BY-4.0','CC0-1.0'].includes(info.license)?info.license:'CC-BY-4.0'
    }};
  }

  function refreshUI() {
    const model=state.model;
    el('architecture').textContent='11 → 12 → 5';
    el('trainedCount').textContent=model ? model.ready.filter(Boolean).length+' / 5' : '0 / 5';
    el('sampleCount').textContent=String(state.examples.length);
    const chips=el('effectChips');
    chips.replaceChildren();
    EFFECTS.forEach((name,index)=>{
      const chip=document.createElement('span');
      if(model?.ready[index]) chip.className='ready';
      chip.textContent=(model?.ready[index]?'✓ ':'○ ')+name;
      chips.appendChild(chip);
    });
    const canTrain=!!model && !state.busy && hasTrainableSamples(state.examples);
    el('trainButton').disabled=!canTrain;
    el('exportButton').disabled=!model || state.busy;
  }

  function assignProfile(data) {
    state.model=data.model;
    state.examples=[];
    el('profileName').value=data.profile.name;
    el('profileAuthor').value=data.profile.author;
    el('profileDescription').value=data.profile.description;
    el('profileCategory').value=data.profile.category;
    el('profileLicense').value=data.profile.license;
    updateMessage('Loaded '+data.profile.name+'. The model is ready to export or import into Cyn.');
    refreshUI();
    document.querySelector('#studio').scrollIntoView({behavior:'smooth'});
  }

  async function readJsonFile(file) {
    if (!file || file.size > 2097152) throw new Error('Select a JSON file smaller than 2 MB.');
    let result;
    try { result=JSON.parse(await file.text()); }
    catch {throw new Error('Could not read this file as JSON.');}
    return result;
  }

  async function openProfile(file) {
    try {assignProfile(profileFromJson(await readJsonFile(file)));}
    catch(e){updateMessage('Import failed: '+e.message);}
  }
  async function openPrivateData(file) {
    try {
      const input=await readJsonFile(file);
      if(input?.schema!==1)throw new Error('This is not a Cyn preferences.json learning file.');
      const model=validateModel(input);
      state.model=model;
      state.examples=validateExamples(input.examples);
      el('profileName').value='My trained Cyn AI';
      el('profileDescription').value='';
      el('profileAuthor').value='';
      updateMessage('Loaded '+state.examples.length+' private feature labels into browser memory. Nothing has been uploaded.');
      refreshUI();
    }catch(e){updateMessage('Private learning file error: '+e.message);}
  }

  function sampleCounts(samples) {
    const counts=Array.from({length:5},()=>[0,0]);
    for(const e of samples)counts[e.effect][e.more?1:0]++;
    return counts;
  }
  function hasTrainableSamples(samples){
    return sampleCounts(samples).some(([less,more])=>less>=5 && more>=5);
  }
  function normalize(features) {
    return features.map((v,i)=>{
      if(i<8)return (clamp(v,-12,0)+6)/6;
      if(i===8)return clamp(v,0,4)/2-1;
      if(i===9)return clamp(v,0,2)-1;
      return (clamp(v,-8,0)+4)/4;
    });
  }

  // Same network dimensions, weight ordering, label weighting and Adam training
  // procedure as fxsound/Source/GUI/PersonalAudioModel.h.
  async function trainLocal() {
    if(!state.model || state.busy || !hasTrainableSamples(state.examples))return;
    state.busy=true; refreshUI();
    const w=[...state.model.weights],samples=state.examples;
    const counts=sampleCounts(samples);
    const ready=counts.map(([less,more])=>less>=5 && more>=5);
    const moments=new Float64Array(MODEL_SIZE),variance=new Float64Array(MODEL_SIZE);
    let beta1=1,beta2=1;
    const batch=samples.filter(e=>ready[e.effect]).map(e=>({
      x:normalize(e.x),effect:e.effect,more:e.more,starter:e.starter
    }));
    try {
      for(let epoch=1;epoch<=240;epoch++){
        const grads=new Float64Array(MODEL_SIZE);
        for(const item of batch){
          const o=item.effect, x=item.x;
          const h=new Float64Array(12);
          for(let j=0;j<12;j++){
            let z=w[132+j];
            for(let i=0;i<11;i++) z+=x[i]*w[j*11+i];
            h[j]=Math.tanh(z);
          }
          let z=w[204+o];
          for(let j=0;j<12;j++)z+=h[j]*w[144+o*12+j];
          const p=1/(1+Math.exp(-clamp(z,-30,30)));
          const sampleWeight=(item.starter ? 0.2 : 1)/counts[o][item.more?1:0];
          const d=(p-(item.more?1:0))*sampleWeight;
          grads[204+o]+=d;
          for(let j=0;j<12;j++){
            grads[144+o*12+j]+=d*h[j];
            const dh=d*w[144+o*12+j]*(1-h[j]*h[j]);
            grads[132+j]+=dh;
            for(let i=0;i<11;i++)grads[j*11+i]+=dh*x[i];
          }
        }
        beta1*=0.9;beta2*=0.999;
        for(let i=0;i<MODEL_SIZE;i++){
          const grad=grads[i]+0.0001*w[i];
          moments[i]=.9*moments[i]+.1*grad;
          variance[i]=.999*variance[i]+.001*grad*grad;
          w[i]-=.012*(moments[i]/(1-beta1))/(Math.sqrt(variance[i]/(1-beta2))+1e-7);
          if(!Number.isFinite(w[i]) || Math.abs(w[i])>100)
            throw new Error('Training became numerically unstable. Your previous model is unchanged.');
        }
        if(epoch%12===0){
          updateMessage('Training locally… '+Math.round(epoch/240*100)+'% · '+batch.length+' labelled feature frames');
          await pause();
        }
      }
      state.model={schema:1,architecture:ARCHITECTURE,weights:w,ready};
      updateMessage('Training complete. '+ready.filter(Boolean).length+' effects learned. Export a profile to share it.');
    }catch(e){updateMessage('Training error: '+e.message);}
    finally{state.busy=false;refreshUI();}
  }

  function exportProfile(){
    if(!state.model || state.busy)return;
    const profile={
      name:el('profileName').value.trim().slice(0,80)||'My Cyn AI',
      author:el('profileAuthor').value.trim().slice(0,60),
      description:el('profileDescription').value.trim().slice(0,500),
      category:el('profileCategory').value,
      license:el('profileLicense').value
    };
    const output={format:'cyn-ai-profile',schema:1,profile,
      model:{schema:1,architecture:ARCHITECTURE,weights:state.model.weights,ready:state.model.ready}};
    const json=JSON.stringify(output,null,2);
    const blob=new Blob([json],{type:'application/json'});
    const href=URL.createObjectURL(blob);
    const anchor=document.createElement('a');
    anchor.href=href;
    anchor.download=(profile.name.toLowerCase().normalize('NFKD').replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'')||'cyn-ai')+'.cynai.json';
    document.body.appendChild(anchor);
    anchor.click();
    anchor.remove();
    setTimeout(()=>URL.revokeObjectURL(href),1500);
    updateMessage('Exported '+profile.name+'. Only public metadata and neural weights were included.');
  }

  function createCard(item) {
    const card=document.createElement('article');card.className='profile-card';
    const top=document.createElement('div');
    const tag=document.createElement('span');tag.className='tag';tag.textContent=item.category||'music';
    top.appendChild(tag);
    const title=document.createElement('h3');title.textContent=item.name||'Untitled profile';
    const by=document.createElement('span');by.className='byline';by.textContent='by '+(item.author||'Community');
    const desc=document.createElement('p');desc.textContent=item.description||'A shareable Cyn audio AI profile.';
    const foot=document.createElement('div');foot.className='card-foot';
    const button=document.createElement('button');button.className='button soft';button.textContent='Try this AI ↗';
    button.addEventListener('click',async()=>{
      try {
        const path=String(item.file||'');
        if(!/^[a-z0-9-]+\.cynai\.json$/.test(path))throw Error('Invalid community file path.');
        const response=await fetch('./community/'+path,{cache:'no-store'});
        if(!response.ok)throw Error('Community profile is unavailable.');
        assignProfile(profileFromJson(await response.json()));
      }catch(e){updateMessage('Cannot open profile: '+e.message);}
    });
    const thread=document.createElement('a');
    if(Number.isSafeInteger(item.issue)&&item.issue>0){
      thread.href='https://github.com/oddfoxerr-real/Cyn/issues/'+item.issue;
      thread.target='_blank';thread.rel='noopener noreferrer';thread.textContent='Reviews ↗';
    }else{
      thread.href='https://github.com/oddfoxerr-real/Cyn/issues/new?template=review-ai.yml';
      thread.target='_blank';thread.rel='noopener noreferrer';thread.textContent='Write review ↗';
    }
    foot.append(button,thread);
    card.append(top,title,by,desc,foot);
    return card;
  }
  function renderGallery(){
    const filter=el('galleryFilter').value;
    const search=el('gallerySearch').value.trim().toLowerCase();
    const items=state.catalogue.filter(p=>(filter==='all'||p.category===filter)
      &&(p.name+' '+p.author+' '+p.description).toLowerCase().includes(search));
    const gallery=el('gallery');gallery.replaceChildren();
    if(!items.length){
      const empty=document.createElement('div');empty.className='empty';
      empty.textContent=state.catalogue.length?
        'No profiles match that search yet.':'No community profiles approved yet. Submit the first one!';
      gallery.appendChild(empty);
    }else items.forEach(i=>gallery.appendChild(createCard(i)));
  }
  async function loadCatalogue(){
    try{
      const response=await fetch('./community/index.json',{cache:'no-store'});
      if(!response.ok)throw Error('Catalogue unavailable.');
      const data=await response.json();
      state.catalogue=Array.isArray(data.profiles)?data.profiles.filter(x=>x&&typeof x.name==='string'):[];
      renderGallery();
    }catch(e){
      const gallery=el('gallery');gallery.replaceChildren();
      const message=document.createElement('div');message.className='empty';
      message.textContent='The community catalogue could not load. You can still train and export a profile offline.';
      gallery.appendChild(message);
    }
  }

  el('profileFile').addEventListener('change',e=>{if(e.target.files[0])openProfile(e.target.files[0]);e.target.value='';});
  el('learningFile').addEventListener('change',e=>{if(e.target.files[0])openPrivateData(e.target.files[0]);e.target.value='';});
  el('trainButton').addEventListener('click',trainLocal);
  el('exportButton').addEventListener('click',exportProfile);
  el('gallerySearch').addEventListener('input',renderGallery);
  el('galleryFilter').addEventListener('change',renderGallery);
  refreshUI();
  loadCatalogue();
})();