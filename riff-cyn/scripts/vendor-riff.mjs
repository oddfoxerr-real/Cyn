import { mkdir, copyFile, writeFile } from "node:fs/promises";
import { existsSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import https from "node:https";

export const RIFF_COMMIT = "398c3dac976bd7b4a7a4b4d7a6d7636ea868a0a8";
const here=dirname(fileURLToPath(import.meta.url));
const project=resolve(here,"..");
const root=resolve(project,"..");
const source=resolve(root,"riff-upstream");
const target=resolve(project,"renderer","vendor");

function download(relative) {
  const url="https://raw.githubusercontent.com/rootscripts/riff/"+RIFF_COMMIT+"/"+relative;
  return new Promise((resolveResult,reject)=>{
    https.get(url,{timeout:30000,headers:{"User-Agent":"CYN-Riff-vendor-script"}},res=>{
      if(res.statusCode!==200){res.resume();reject(new Error("Upstream Riff "+relative+" HTTP "+res.statusCode));return;}
      const chunks=[];
      res.on("data",c=>chunks.push(c));
      res.on("end",()=>resolveResult(Buffer.concat(chunks)));
      res.on("error",reject);
    }).on("error",reject);
  });
}
await mkdir(target,{recursive:true});
for(const [name,dest] of [["styles.css","riff.css"],["LICENSE","Riff-LICENSE.txt"]]){
  if(existsSync(resolve(source,name))) await copyFile(resolve(source,name),resolve(target,dest));
  else await writeFile(resolve(target,dest),await download(name));
}
const license=(await import("node:fs/promises")).readFile(resolve(target,"Riff-LICENSE.txt"),"utf8");
if(!(await license).includes("MIT License"))throw new Error("Riff MIT license is missing");
await writeFile(resolve(target,"SOURCE.txt"),
  "Riff by rootscripts\nRepository: https://github.com/rootscripts/riff\nPinned commit: "+RIFF_COMMIT+
  "\nFiles: styles.css, LICENSE\nSource remains MIT; CYN/FxSound remains AGPL-3.0-or-later.\n");
console.log("Vendored Riff CSS and MIT license at pinned commit "+RIFF_COMMIT);
