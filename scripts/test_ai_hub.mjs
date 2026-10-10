// No external libraries. Validate public model format and website wiring.
import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const path = p => new URL("../" + p, import.meta.url);
const starter=JSON.parse(readFileSync(path("ai-hub/community/cyn-starter.cynai.json"),"utf8"));
assert.equal(starter.format,"cyn-ai-profile");
assert.equal(starter.schema,1);
assert.equal(starter.model.schema,1);
assert.equal(starter.model.architecture,"cyn-audio-11x12x5");
assert.equal(starter.model.weights.length,209);
assert(starter.model.weights.every(w=>typeof w==="number" && Number.isFinite(w) && Math.abs(w)<=100));
assert.equal(starter.model.ready.length,5);
assert(starter.model.ready.every(x=>typeof x==="boolean"));
assert(!("examples" in starter));
assert(!("examples" in starter.model));
const listing=JSON.parse(readFileSync(path("ai-hub/community/index.json"),"utf8"));
assert.equal(listing.schema,1);
assert(listing.profiles.some(p=>p.file==="cyn-starter.cynai.json"));
const html=readFileSync(path("ai-hub/index.html"),"utf8");
for (const id of ["profileFile","learningFile","trainButton","exportButton","gallery","effectChips","trainedCount","sampleCount"]){
  assert(html.includes('id="'+id+'"'),"Missing site UI control: "+id);
}
const js=readFileSync(path("ai-hub/app.js"),"utf8");
assert(js.includes("240") && js.includes("sampleWeight"),"Browser model trainer missing");
assert(js.includes("weights:state.model.weights") && js.includes("ready:state.model.ready"));
assert(!js.includes("examples:state.examples"),"Accidentally exporting private examples");
console.log("PASS: site, offline trainer, safe starter and 209-weight profile schema");
