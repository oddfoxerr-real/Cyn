"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const vm=require("node:vm");
const root=path.resolve(__dirname,"..","renderer");
const html=fs.readFileSync(path.join(root,"index.html"),"utf8");
const renderer=fs.readFileSync(path.join(root,"renderer.js"),"utf8");
const css=fs.readFileSync(path.join(root,"minimal.css"),"utf8");

test("quiet stylesheet loads last, keeps responsive/reduced motion options",()=>{
  assert(html.indexOf('href="minimal.css"')>html.indexOf('href="cyn.css"'));
  assert.match(css,/\.control-deck/);
  assert.match(css,/@media\(max-width:710px\)/);
  assert.match(css,/prefers-reduced-motion/);
});
test("quiet UI keeps native bridge and all original control IDs",()=>{
  for(const id of ["preset","device-top","power","fx-engine-panel","engine-details-toggle",
    "eq-curve","native-spectrum","effects-grid","effects-advanced-grid","go-eq",
    "ai-model","brain-canvas","train-ai","dj-enable","cyn-updates-dialog","rail-drawer"]){
    assert.equal(html.split('id="'+id+'"').length-1,1,"Unique native control: "+id);
  }
  assert.match(renderer,/window\.cyn\.status\(\)/);
  assert.match(renderer,/window\.cyn\.apply\(action\)/);
  assert.match(renderer,/engineAttention=Boolean\(status\.power\)/);
  assert.match(renderer,/engineAttention=true/);
  assert.match(css,/\.fx-engine-panel\[hidden\]\s*\{display:none!important;\}/);
});
test("advanced sections are native keyboard-accessible disclosure controls",()=>{
  assert.equal((html.match(/<details\b/g)||[]).length,2);
  assert.equal((html.match(/<\/details>/g)||[]).length,2);
  assert.match(html,/<details class="secondary-effects" id="secondary-effects">/);
  assert.match(html,/<details class="ai-advanced" id="ai-advanced">/);
  assert.match(html,/More sound controls/);
  assert.match(html,/Advanced AI tools/);
  assert.match(css,/\.secondary-effects>summary:focus-visible/);
});
test("three prominent and two secondary effects still call native DSP",()=>{
  const fxMatch=renderer.match(/const fx=\[([\s\S]*?)\];/);
  assert(fxMatch);
  const fx=vm.runInNewContext("["+fxMatch[1]+"]",{});
  function element(tag){
    return {
      tagName:tag,children:[],events:{},textContent:"",disabled:false,
      append(...nodes){this.children.push(...nodes)},
      addEventListener(type,handler){this.events[type]=handler},
      setAttribute(key,value){this[key]=value},
      get childElementCount(){return this.children.length}
    };
  }
  const containers={"effects-grid":element("section"),"effects-advanced-grid":element("section")};
  function find(node,id){
    if(node.id===id)return node;
    for(const child of node.children){const v=find(child,id);if(v)return v}
    return null;
  }
  const $=id=>containers[id]||find(containers["effects-grid"],id)||
    find(containers["effects-advanced-grid"],id);
  const calls=[];
  const start=renderer.indexOf("function renderEffects(status){");
  const end=renderer.indexOf("\nfunction renderBands(status){",start);
  assert(start>0&&end>start);
  const env={document:{createElement:element,activeElement:null},fx,$,
    value:n=>Number(n).toFixed(1),writable:()=>true,apply:arg=>calls.push(arg),Number};
  vm.runInNewContext(renderer.slice(start,end)+
    "\nrenderEffects({effects:{bass:8,clarity:4,surround:3,ambience:2,dynamicboost:1}})",env);
  assert.equal(containers["effects-grid"].children.length,3);
  assert.equal(containers["effects-advanced-grid"].children.length,2);
  for(const id of ["bass","clarity","surround","ambience","dynamicboost"])
    assert($("effect-"+id));
  assert.equal($("effect-bass").value,8);
  $("effect-bass").events.change();
  assert.equal(calls[0].name,"bass");
  assert.equal(calls[0].value,8);
});
test("UI does not replace CYN native engine with browser audio",()=>{
  const bridge=fs.readFileSync(path.resolve(__dirname,"..","native-bridge.cjs"),"utf8");
  assert.match(bridge,/--engine-host/);
  assert.doesNotMatch(renderer,/new AudioContext\(/);
  assert.match(renderer,/renderGraph\(bands\)/);
  assert.match(renderer,/renderConnected\(result\.status\)/);
});
