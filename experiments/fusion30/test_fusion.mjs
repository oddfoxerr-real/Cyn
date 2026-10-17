import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import {validateCatalog,fuseScene,predictCynProfile} from "./fusion.mjs";

const c=JSON.parse(fs.readFileSync(new URL("./catalog.json",import.meta.url),"utf8"));
const p=(id,signals,extras={})=>({
  id,signals,confidence:1,timestampMs:1000,latencyMs:12,calibrated:true,...extras
});
const opts={research:true,nowMs:1000};

test("30 experts remain disabled and registry is unique",()=>{
  assert.equal(validateCatalog(c),true);
  assert.equal(c.models.length,30);
  assert.equal(c.models.filter(m=>m.enabled).length,0);
  assert.equal(new Set(c.models.map(m=>m.id)).size,30);
});
test("research must be explicitly enabled",()=>{
  assert.equal(fuseScene(c,[p("yamnet",{music:1})],{nowMs:1000}).status,"bypass");
});
test("combine independent calibrated scene evidence only",()=>{
  const x=fuseScene(c,[p("yamnet",{music:0.8,speech:0.2}),p("panns",{music:0.4,speech:0.6})],opts);
  assert.equal(x.status,"research-evidence");
  assert.deepEqual(x.used,["yamnet","panns"]);
  assert.ok(Math.abs(x.scene.music-0.6)<1e-12);
  assert.ok(Math.abs(x.scene.speech-0.4)<1e-12);
});
test("speech evidence cannot masquerade as music classifier",()=>{
  const x=fuseScene(c,[p("silero-vad",{speech:0.9,music:1})],opts);
  assert.equal(x.scene.speech,0.9);
  assert.equal(x.scene.music,null);
});
test("no offline generator or separator may output DSP evidence",()=>{
  assert.equal(fuseScene(c,[p("musicgen",{music:1}),p("spleeter",{speech:0.6})],opts).status,"bypass");
});
test("reject stale, late, uncalibrated and NaN observations",()=>{
  const samples=[
    p("yamnet",{music:1},{timestampMs:0}),
    p("yamnet",{music:1},{latencyMs:501}),
    p("yamnet",{music:1},{calibrated:false}),
    p("yamnet",{music:NaN})
  ];
  for(const sample of samples)assert.equal(fuseScene(c,[sample],{...opts,maxAgeMs:500}).status,"bypass");
});
test("duplicate model observations cannot vote twice",()=>{
  const x=fuseScene(c,[p("yamnet",{music:1}),p("yamnet",{music:0})],opts);
  assert.deepEqual(x.used,["yamnet"]);
  assert.equal(x.scene.music,1);
});
test("Cyn profile functional parity with neutral weights",()=>{
  const x=predictCynProfile({weights:Array(209).fill(0)},Array(11).fill(-3));
  assert.deepEqual(x,Array(5).fill(0.5));
  assert.throws(()=>predictCynProfile({weights:[]},Array(11).fill(0)),TypeError);
});
