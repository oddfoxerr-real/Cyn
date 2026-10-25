"use strict";
const {app,BrowserWindow,ipcMain,dialog,shell}=require("electron");
const fs=require("node:fs");
const path=require("node:path");
const {makeBridge}=require("./native-bridge.cjs");
if(process.platform==="win32")app.commandLine.appendSwitch("disable-features","CalculateNativeWinOcclusion");
let win;
let bridge;
let allowClose=false,closeRequested=false;
let onTop=false;
const primaryInstance=app.requestSingleInstanceLock();
if(!primaryInstance)app.quit();
else app.on("second-instance",()=>{
  // Reuse the existing Material CYN window for tray clicks / old shortcuts.
  if(win&&!win.isDestroyed()){
    if(win.isMinimized())win.restore();
    win.show();win.focus();
  }
});
const settingsFile=()=>path.join(app.getPath("userData"),"native-engine.json");
function loadConfig(){
  try{
    const v=JSON.parse(fs.readFileSync(settingsFile(),"utf8"));
    return typeof v.executable==="string"?v.executable:null;
  }catch{return null;}
}
function saveConfig(executable){
  fs.mkdirSync(path.dirname(settingsFile()),{recursive:true});
  fs.writeFileSync(settingsFile(),JSON.stringify({executable},null,2));
}
function createWindow(){
  win=new BrowserWindow({
    width:1370,height:840,minWidth:760,minHeight:580,
    show:false,backgroundColor:"#121214",title:"CYN",
    frame:false,
    webPreferences:{
      preload:path.join(__dirname,"preload.cjs"),
      contextIsolation:true,nodeIntegration:false,sandbox:true,
      webSecurity:true
    }
  });
  if(process.env.CYN_RIFF_CAPTURE_DIR){
    // Automated visual review captures the actual Riff/Electron renderer.
    // This runs only when explicitly requested by preview CI, never in releases.
    win.webContents.once("did-finish-load",()=>{
      (async()=>{
        const directory=path.resolve(process.env.CYN_RIFF_CAPTURE_DIR);
        fs.mkdirSync(directory,{recursive:true});
        await win.webContents.executeJavaScript(
          'document.querySelector("[data-view=overview]").click()');
        // Inspect actual Electron computed styles, not just source strings.
        // A connected-pill navigation must have DIFFERENT top/mid/end
        // corner radii after the transition has settled.
        const dockCorners=[];
        for(const [view,expect] of [
          ["overview",["24px","24px","10px","10px"]],
          ["devices",["14px","14px","14px","14px"]],
          ["ai",["10px","10px","24px","24px"]]
        ]){
          await win.webContents.executeJavaScript(
            'document.querySelector("[data-view='+view+']").click()');
          await new Promise(resolve=>setTimeout(resolve,700));
          const found=JSON.parse(await win.webContents.executeJavaScript(
            'JSON.stringify((()=>{const ind=document.getElementById("nav-active-indicator");const active=document.querySelector(".sidebar-nav .nav-item.active");const cs=getComputedStyle(ind);return {view:active.dataset.view,corners:[cs.borderTopLeftRadius,cs.borderTopRightRadius,cs.borderBottomRightRadius,cs.borderBottomLeftRadius],computedRadius:getComputedStyle(active).borderRadius,selection:cs.backgroundColor,indicatorHeight:ind.getBoundingClientRect().height};})())'));
          dockCorners.push(found);
          if(found.view!==view||JSON.stringify(found.corners)!==JSON.stringify(expect)||
             Math.abs(found.indicatorHeight-48)>1||
             found.selection!=="rgb(59, 60, 64)"){
            throw new Error("Riff connected-pill regression: "+JSON.stringify({expect,...found}));
          }
          const picture=await win.webContents.capturePage();
          fs.writeFileSync(path.join(directory,"dock-"+view+".png"),picture.toPNG());
        }
        const graphiteAudit=JSON.parse(await win.webContents.executeJavaScript(
          'JSON.stringify((()=>{const line=document.querySelector(".rail-lower-divider").getBoundingClientRect();const items=[...document.querySelectorAll(".rail-drawer-trigger")];const drawer=document.getElementById("rail-drawer");return {drawerColor:getComputedStyle(drawer).backgroundColor,belowDivider:items.length===3&&items.every(el=>el.getBoundingClientRect().top>=line.bottom),mainPages:[...document.querySelectorAll(".sidebar-nav [data-view]")].length};})())'));
        graphiteAudit.positions=dockCorners;
        fs.writeFileSync(path.join(directory,"graphite-dock-audit.json"),JSON.stringify(graphiteAudit,null,2));
        if(graphiteAudit.drawerColor!=="rgb(30, 31, 34)"||
           !graphiteAudit.belowDivider||graphiteAudit.mainPages!==3){
          throw new Error("Riff graphite dock regression: "+JSON.stringify(graphiteAudit));
        }
        // The preceding screenshot cycle ends on the AI page; inspect
        // Sound Studio in its active state, not a legitimately hidden tab.
        await win.webContents.executeJavaScript('document.querySelector("[data-view=overview]").click()');
        // Catch a UI regression where the full native CYN Settings and
        // Update controls exist in markup but cannot be reached by users.
        const coreControls=JSON.parse(await win.webContents.executeJavaScript(
          'JSON.stringify((()=>{const ids=["toolbar-settings","toolbar-updates","fx-engine-reconnect","fx-engine-detail"];return ids.map(id=>{const e=document.getElementById(id);if(!e)return {id,missing:true};const r=e.getBoundingClientRect(),s=getComputedStyle(e);return {id,visible:r.width>25&&r.height>10&&s.visibility!=="hidden"&&s.display!=="none",left:r.left,right:r.right,top:r.top,bottom:r.bottom,width:innerWidth,height:innerHeight};})})())'));
        if(coreControls.some(x=>x.missing||!x.visible||x.left<0||x.right>x.width||x.top<0||x.bottom>x.height))
          throw new Error("Critical FX engine, Settings or Update controls are inaccessible: "+JSON.stringify(coreControls));
        await win.webContents.executeJavaScript('document.getElementById("toolbar-settings").click()');
        const settingsOpen=await win.webContents.executeJavaScript(
          'document.getElementById("view-settings").classList.contains("active") && [...document.querySelectorAll("#view-settings [data-setting=updates]")].length===1');
        if(!settingsOpen)throw new Error("Visible CYN Settings control failed to open native updater panel");
        // The old release shipped only a blind PowerShell spawn. Prove the
        // first-class Material update-check dialog is actually visible.
        await win.webContents.executeJavaScript('document.getElementById("toolbar-updates").click()');
        await new Promise(resolve=>setTimeout(resolve,100));
        const updateDialogAudit=JSON.parse(await win.webContents.executeJavaScript(
          'JSON.stringify((()=>{const e=document.getElementById("cyn-updates-dialog"),b=e?.getBoundingClientRect();return {open:e?.open,phase:e?.dataset.phase,width:b?.width,height:b?.height,checkControls:!!document.getElementById("cyn-updates-install")&&!!document.getElementById("cyn-updates-retry")};})())'));
        if(!updateDialogAudit.open||updateDialogAudit.width<300||
           updateDialogAudit.height<230||!updateDialogAudit.checkControls)
          throw new Error("Native CYN updater dialog is not accessible: "+JSON.stringify(updateDialogAudit));
        const updaterShot=await win.webContents.capturePage();
        fs.writeFileSync(path.join(directory,"cyn-in-app-updater.png"),updaterShot.toPNG());
        await win.webContents.executeJavaScript('document.getElementById("cyn-updates-close").click()');
        await win.webContents.executeJavaScript('document.querySelector("[data-view=overview]").click()');
        const tabs=["overview","equalizer","ai","devices","settings"];
        const layoutDiagnostics=await win.webContents.executeJavaScript(
          'JSON.stringify({viewport:{w:innerWidth,h:innerHeight},body:{height:document.body.getBoundingClientRect().height},elements:[".app-layout",".main-content",".control-deck",".control-field",".error-banner",".workspace-header",".eq-hero"].map(selector=>{const el=document.querySelector(selector);if(!el)return {selector,missing:true};const r=el.getBoundingClientRect();const cs=getComputedStyle(el);return {selector,rect:{x:r.x,y:r.y,width:r.width,height:r.height},display:cs.display,visibility:cs.visibility,opacity:cs.opacity,overflow:cs.overflow,childCount:el.children.length}})})');
        fs.writeFileSync(path.join(directory,"layout.json"),layoutDiagnostics);
        const parsed=JSON.parse(layoutDiagnostics);
        const deck=parsed.elements.find(item=>item.selector===".control-deck");
        if(!deck||deck.rect.height<100)throw new Error("CYN top control deck is clipped: "+JSON.stringify(deck));
        for(const tab of tabs){
          await win.webContents.executeJavaScript(
            'document.querySelector("[data-view=\\\"'+tab+'\\\"]").click()');
          await new Promise(resolve=>setTimeout(resolve,650));
          if(tab==="ai"||tab==="devices"){
            const fieldAudit=JSON.parse(await win.webContents.executeJavaScript(
              'JSON.stringify((()=>{const field=document.querySelector("#view-'+tab+' .detail-select-field");const select=field?.querySelector("select");const card=field?.closest(".surface-card");if(!field||!card||!select)return {missing:true};const fieldStyle=getComputedStyle(field);const fieldBox=field.getBoundingClientRect();const cardBox=card.getBoundingClientRect();const selectBox=select.getBoundingClientRect();return {background:fieldStyle.backgroundColor,padding:fieldStyle.padding,fieldWidth:fieldBox.width,cardWidth:cardBox.width,fieldHeight:fieldBox.height,cardHeight:cardBox.height,selectHeight:selectBox.height,insideCard:fieldBox.left>=cardBox.left&&fieldBox.right<=cardBox.right};})())'));
            fs.writeFileSync(path.join(directory,tab+"-selector-audit.json"),JSON.stringify(fieldAudit,null,2));
            if(fieldAudit.missing||
               fieldAudit.background!=="rgba(0, 0, 0, 0)"||
               fieldAudit.padding!=="0px"||
               !fieldAudit.insideCard||
               fieldAudit.fieldHeight>=fieldAudit.cardHeight*.55||
               fieldAudit.selectHeight<44){
              throw new Error("Nested CYN selector visual regression: "+JSON.stringify({tab,...fieldAudit}));
            }
          }
          const image=await win.webContents.capturePage();
          fs.writeFileSync(path.join(directory,tab+".png"),image.toPNG());
        }
        const flyouts=[
          ["presets","btn-toggle-presets-drawer"],
          ["ai-hub","btn-toggle-ai-drawer"],
          ["appearance","btn-toggle-theme-drawer"]
        ];
        for(const [label,trigger] of flyouts){
          await win.webContents.executeJavaScript(
            'document.getElementById('+JSON.stringify(trigger)+').click()');
          await new Promise(resolve=>setTimeout(resolve,500));
          const visible=await win.webContents.executeJavaScript(
            'getComputedStyle(document.getElementById("rail-drawer")).display!=="none" && document.getElementById("rail-drawer").classList.contains("open")');
          if(!visible)throw new Error("CYN drawer not visible: "+label);
          const shot=await win.webContents.capturePage();
          fs.writeFileSync(path.join(directory,"drawer-"+label+".png"),shot.toPNG());
          await win.webContents.executeJavaScript(
            'document.getElementById("btn-close-rail-drawer").click()');
          await new Promise(resolve=>setTimeout(resolve,240));
        }
        app.quit();
      })().catch(error=>{console.error("Screenshot capture failed:",error);app.exit(1)});
    });
  }
  win.on("close",event=>{
    if(allowClose||process.env.CYN_RIFF_CAPTURE_DIR)return;
    event.preventDefault();
    if(closeRequested)return;
    closeRequested=true;
    if(!win.webContents.isLoading())win.webContents.send("cyn:animate-close");
    else {allowClose=true;win.destroy();}
  });
  win.on("closed",()=>{win=null;});
  win.loadFile(path.join(__dirname,"renderer","index.html"));
  win.once("ready-to-show",()=>win.show());
  win.webContents.setWindowOpenHandler(()=>({action:"deny"}));
  win.webContents.on("will-navigate",event=>event.preventDefault());
}
if(primaryInstance)app.whenReady().then(()=>{
  bridge=makeBridge(loadConfig());
  ipcMain.handle("cyn:status",()=>bridge.getStatus());
  ipcMain.handle("cyn:telemetry",()=>bridge.telemetry());
  ipcMain.handle("cyn:apply",async (_e,action)=>bridge.apply(action));
  ipcMain.handle("cyn:start",()=>bridge.launch());
  ipcMain.handle("cyn:select-native",async()=>{
    const result=await dialog.showOpenDialog(win,{
      title:"Select your installed native CYN engine",
      filters:[{name:"CYN native executable",extensions:["exe"]}],
      properties:["openFile"]
    });
    if(result.canceled||!result.filePaths.length)return {selected:false};
    const executable=result.filePaths[0];
    bridge.selectNative(executable);
    saveConfig(executable);
    return {selected:true};
  });
  ipcMain.on("cyn:window",(_e,action)=>{
    if(!win||win.isDestroyed())return;
    if(action==="minimize")win.minimize();
    else if(action==="maximize")win.isMaximized()?win.unmaximize():win.maximize();
    else if(action==="close")win.close();
  });
  ipcMain.on("cyn:close-complete",event=>{
    if(!win||win.isDestroyed()||event.sender!==win.webContents)return;
    if(!closeRequested)return;
    allowClose=true;
    win.close();
  });
  ipcMain.handle("cyn:on-top",(_event,value)=>{
    if(typeof value!=="boolean"||!win||win.isDestroyed())throw new Error("Invalid window preference");
    onTop=value;win.setAlwaysOnTop(onTop,"floating");
    return onTop;
  });
  // Resolve the installed native CYN version even if DSP is offline. Neither
  // the Material UI nor Electron's package version is the update authority.
  async function resolveUpdater(){
    if(process.platform!=="win32")throw new Error("CYN Windows updates require Windows");
    const exe=bridge.findExe();
    if(!exe)throw new Error("Native Cyn.exe is missing. Repair CYN with the official Windows installer.");
    const root=path.dirname(exe);
    const script=path.join(root,"CynUpdater.ps1");
    const worker=path.join(root,"CynUpdaterWorker.ps1");
    if(!fs.existsSync(script)||!fs.existsSync(worker))
      throw new Error("The original CYN updater is incomplete. Repair the combined CYN Windows installation.");
    const live=await bridge.getStatus();
    let reported=live.connected?String(live.status.version||""):"";
    if(!/^\d+\.\d+\.\d+(?:\.\d+)?$/.test(reported)){
      const {execFile}=require("node:child_process");
      const {promisify}=require("node:util");
      const inspect=promisify(execFile);
      const powershell='(Get-Item -LiteralPath $env:CYN_NATIVE_EXE -ErrorAction Stop).VersionInfo.FileVersion';
      try{
        const {stdout}=await inspect("powershell.exe",[
          "-NoProfile","-NonInteractive","-Command",powershell
        ],{windowsHide:true,timeout:6500,maxBuffer:2048,env:{...process.env,CYN_NATIVE_EXE:exe}});
        reported=String(stdout).trim();
      }catch(error){
        throw new Error("Could not read the installed CYN version. Repair CYN with the latest official installer.");
      }
    }
    const version=reported.match(/^(\d+\.\d+\.\d+)(?:\.\d+)?$/);
    if(!version)throw new Error("Could not verify the installed native CYN version.");
    return {exe,root,script,worker,version:version[1]};
  }
  ipcMain.handle("cyn:updates-inspect",async()=>{
    const {worker,root,version}=await resolveUpdater();
    const {promisify}=require("node:util");
    const {execFile}=require("node:child_process");
    const exec=promisify(execFile);
    let stdout;
    try{
      ({stdout}=await exec("powershell.exe",[
        "-NoProfile","-NonInteractive","-ExecutionPolicy","Bypass",
        "-File",worker,"-CurrentVersion",version,"-CheckOnly"
      ],{cwd:root,windowsHide:true,timeout:65000,maxBuffer:32768}));
    }catch(error){
      throw new Error("CYN couldn't check GitHub releases. Verify your connection or try again. "+String(error.message||""));
    }
    let report;
    try{report=JSON.parse(stdout.trim())}
    catch{throw new Error("CYN updater returned an invalid release check. Repair the original updater files.")}
    const latest=String(report.latestVersion||"");
    if((report.phase!=="current"&&report.phase!=="available")||
       report.currentVersion!==version||!/^\d+\.\d+\.\d+$/.test(latest))
      throw new Error("CYN updater returned an untrusted version result.");
    return {phase:report.phase,currentVersion:version,latestVersion:latest,
      sizeBytes:Number.isSafeInteger(Number(report.sizeBytes))?Number(report.sizeBytes):0};
  });
  // Keep the original CYN WPF worker, SHA-256 verification, native driver-safe
  // installer and restart logic. Material shows check results before opening it.
  ipcMain.handle("cyn:check-updates",async()=>{
    const {root,script,version}=await resolveUpdater();
    const {spawn}=require("node:child_process");
    const child=spawn("powershell.exe",[
      "-NoProfile","-NonInteractive","-STA","-ExecutionPolicy","Bypass",
      "-File",script,"-CurrentVersion",version,"-Interactive"
    ],{detached:true,windowsHide:false,stdio:"ignore",cwd:root});
    return await new Promise((resolve,reject)=>{
      let settled=false;
      const started=setTimeout(()=>{
        if(settled)return;
        settled=true;
        resolve({started:true});
      },1200);
      child.once("spawn",()=>child.unref());
      child.once("error",error=>{
        if(settled)return;
        settled=true;clearTimeout(started);
        reject(new Error("CYN update window could not start: "+error.message));
      });
      child.once("exit",code=>{
        if(settled)return;
        settled=true;clearTimeout(started);
        reject(new Error(code===0
          ?"CYN updater closed immediately. Another update window may already be running."
          :"CYN updater exited before opening (code "+code+"). Repair CYN or inspect updater logs."));
      });
    });
  });
  ipcMain.handle("cyn:open-native",async()=>{
    const exe=bridge.findExe();
    if(!exe)throw new Error("Install native CYN or locate Cyn.exe first");
    const child=require("node:child_process").spawn(exe,["--classic"],{detached:true,stdio:"ignore",windowsHide:false});
    child.unref();
    return true;
  });
  const ALLOWED_LINKS={
    updates:"https://github.com/oddfoxerr-real/Cyn/releases",
    mobile:"https://github.com/oddfoxerr-real/Cyn/releases?q=android-studio",
    credits:"https://github.com/oddfoxerr-real/Cyn/blob/main/LICENSE",
    support:"https://www.fxsound.com/"
  };
  ipcMain.handle("cyn:open-link",(_event,key)=>{
    if(typeof key!=="string"||!Object.prototype.hasOwnProperty.call(ALLOWED_LINKS,key))
      throw new Error("Unknown CYN resource");
    return shell.openExternal(ALLOWED_LINKS[key]);
  });
  createWindow();
  // When installed together, CYN launches its real native processing engine
  // automatically while Electron owns the visible Material interface.
  // Do not start an engine in standalone visual-test environments.
  const bundledNative=path.resolve(path.dirname(process.execPath),"..","Cyn.exe");
  if(process.env.CYN_RIFF_CAPTURE_DIR===undefined&&fs.existsSync(bundledNative))
    bridge.launch().catch(error=>console.error("Native CYN engine startup:",error.message));
});
app.on("window-all-closed",()=>{app.quit();});
