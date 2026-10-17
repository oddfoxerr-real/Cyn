// CYN isolated 30-expert research combiner. NO audio callback work, no bundled weights.
export const SIGNALS = Object.freeze([
  "music", "speech", "percussion", "bassDrum", "electronic",
  "vocals", "impact", "soundtrack", "silence"
]);
const eligibleRoles = new Set(["scene", "speech", "beat"]);
const finite = value => typeof value === "number" && Number.isFinite(value);
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));

export function validateCatalog(catalog) {
  if (catalog?.schema !== 1 || catalog.purpose !== "research-only" ||
      catalog.model_count !== 30 || catalog.release_allowed !== false ||
      catalog.model_weights_bundled !== false ||
      !Array.isArray(catalog.models) || catalog.models.length !== 30) {
    throw new TypeError("Invalid CYN research-only catalog");
  }
  const names = new Set();
  for (const m of catalog.models) {
    if (!m.id || names.has(m.id) || !m.source?.startsWith("https://github.com/") ||
        m.enabled !== false || !m.code_license || !m.weights_status) {
      throw new TypeError("Invalid or auto-enabled expert");
    }
    names.add(m.id);
  }
  return true;
}

// Packets must come from externally loaded, independently calibrated models.
// Packet format: {id,timestampMs,latencyMs,confidence,calibrated,signals:{}}
// Different AI architectures are NOT interchangeable model weights.
export function fuseScene(catalog, packets, options={}) {
  validateCatalog(catalog);
  if (options.research !== true) {
    return {status:"bypass",reason:"research is disabled",scene:null,used:[]};
  }
  const nowMs=options.nowMs, maxAgeMs=options.maxAgeMs ?? 1500;
  if (!finite(nowMs) || !finite(maxAgeMs) || maxAgeMs<=0 || !Array.isArray(packets)) {
    throw new TypeError("Invalid fusion inputs");
  }
  const experts = new Map(catalog.models.map(m=>[m.id,m]));
  const weighted = Object.fromEntries(SIGNALS.map(k=>[k,0]));
  const total = Object.fromEntries(SIGNALS.map(k=>[k,0]));
  const used=[], seen=new Set();
  for (const p of packets) {
    if (!p?.id || seen.has(p.id)) continue;
    seen.add(p.id);
    const expert=experts.get(p.id);
    if (!expert || !eligibleRoles.has(expert.role) || p.calibrated!==true ||
        !finite(p.confidence) || p.confidence<=0 || p.confidence>1 ||
        !finite(p.timestampMs) || !finite(p.latencyMs) ||
        p.latencyMs<0 || p.latencyMs>500 || !p.signals ||
        typeof p.signals!=="object" || Array.isArray(p.signals)) continue;
    const age=nowMs-p.timestampMs;
    if (age<0 || age>maxAgeMs) continue;
    const weight=p.confidence*(1-age/(2*maxAgeMs));
    let accepted=false;
    for(const [k,v] of Object.entries(p.signals)) {
      if (!SIGNALS.includes(k) || !finite(v) || v<0 || v>1) continue;
      if (expert.role==="speech" && !["speech","vocals","silence"].includes(k)) continue;
      if (expert.role==="beat" && k!=="percussion") continue;
      weighted[k]+=weight*v;
      total[k]+=weight;
      accepted=true;
    }
    if(accepted) used.push(p.id);
  }
  if(!used.length) return {status:"bypass",reason:"no calibrated, fresh evidence",scene:null,used:[]};
  const scene=Object.fromEntries(SIGNALS.map(k=>[k,total[k]>0?clamp(weighted[k]/total[k],0,1):null]));
  return {status:"research-evidence",reason:"No EQ, gain, mastering or presets computed",scene,used};
}

// Matches Cyn PersonalAudioModel.h's 11->12 tanh->5 sigmoid scalar inference.
export function predictCynProfile(profile, input) {
  const w=(profile?.model ?? profile)?.weights;
  if(!Array.isArray(w) || w.length!==209 || !w.every(finite) ||
     !Array.isArray(input) || input.length!==11 || !input.every(finite)) {
    throw new TypeError("Expected CYN 209 weights and 11 finite feature inputs");
  }
  const x=input.map((v,i)=> i<8 ? (clamp(v,-12,0)+6)/6 :
    i===8 ? clamp(v,0,4)/2-1 : i===9 ? clamp(v,0,2)-1 :
    (clamp(v,-8,0)+4)/4);
  const h=Array.from({length:12},(_,j)=>{
    let z=w[132+j];
    for(let i=0;i<11;i++) z+=x[i]*w[j*11+i];
    return Math.tanh(z);
  });
  return Array.from({length:5},(_,o)=>{
    let z=w[204+o];
    for(let j=0;j<12;j++) z+=h[j]*w[144+o*12+j];
    return 1/(1+Math.exp(-clamp(z,-30,30)));
  });
}
