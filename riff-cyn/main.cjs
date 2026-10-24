"use strict";
const {app,BrowserWindow,ipcMain,dialog,shell}=require("electron");
const fs=require("node:fs");
const path=require("node:path");
const {makeBridge}=require("./native-bridge.cjs");
if(process.platform==="win32")app.commandLine.appendSwitch("disable-features","CalculateNativeWinOcclusion");
let win;
let bridge;
let overridePath=null;
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
  win.loadFile(path.join(__dirname,"renderer","index.html"));
  win.once("ready-to-show",()=>win.show());
  win.webContents.setWindowOpenHandler(()=>({action:"deny"}));
  win.webContents.on("will-navigate",event=>event.preventDefault());
}
app.whenReady().then(()=>{
  bridge=makeBridge(loadConfig());
  ipcMain.handle("cyn:status",()=>bridge.getStatus());
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
  createWindow();
});
app.on("window-all-closed",()=>{app.quit();});
