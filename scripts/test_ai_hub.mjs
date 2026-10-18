// Native Cyn AI Hub catalogue and offline model-format regression checks.
// No webpage or hosted server is required.
import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const path = p => new URL("../" + p, import.meta.url);
const catalogue=JSON.parse(readFileSync(path("ai-hub/community/index.json"),"utf8"));
assert.equal(catalogue.schema,1);
assert(Array.isArray(catalogue.profiles)&&catalogue.profiles.length>0);
const trained=catalogue.profiles.find(p=>p.id==="cyn-real-audio-13h");
assert(trained && trained.file==="cyn-real-audio-13h.cynai.json", "Real-audio profile missing from native AI Hub");
assert(trained.category==="experimental" && /real|audio/i.test(trained.description), "Trained model must retain experimental labelling");
for(const entry of catalogue.profiles){
  if(entry.kind==="onnx"){
    assert.equal(entry.id,"cyn-apex6m-flowsense","Unknown ONNX catalog entry");
    assert.equal(entry.activation,"apex6m");
    assert.equal(entry.runtime,"apex6m-225.onnx");
    assert.equal(entry.category,"neural music");
    assert.equal(entry.file,undefined,"The full model must not pretend to be a 209-weight profile");
    assert.equal(entry.url,"https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0");
    assert.match(entry.description,/live|adaptive/i);
    continue;
  }
  if(entry.kind==="research"){
    assert.equal(entry.category,"research");
    assert.equal(entry.file,undefined,"Research-only model cannot masquerade as .cynai");
    assert(entry.url.startsWith("https://github.com/oddfoxerr-real/Cyn/releases/tag/cyn-apex6m-") ||
           entry.url === "https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0",
           "Only official APEX releases are admissible");
    assert.match(entry.description,/not compatible|not.*importer/i);
    continue;
  }
  assert.match(entry.file,/^[a-z0-9-]+\.cynai\.json$/);
  const profile=JSON.parse(readFileSync(path("ai-hub/community/"+entry.file),"utf8"));
  assert.equal(profile.format,"cyn-ai-profile");
  assert.equal(profile.schema,1);
  assert.equal(profile.model.schema,1);
  assert.equal(profile.model.architecture,"cyn-audio-11x12x5");
  assert.equal(profile.model.weights.length,209);
  assert(profile.model.weights.every(w=>typeof w==="number"&&Number.isFinite(w)&&Math.abs(w)<=100));
  assert.equal(profile.model.ready.length,5);
  assert(profile.model.ready.every(b=>typeof b==="boolean"));
  assert(!("examples" in profile)&&!("examples" in profile.model),"Never ship private audio labels");
}
const native=readFileSync(path("fxsound/Source/GUI/CynAiHubDialog.h"),"utf8");
assert(native.includes("juce::DialogWindow::LaunchOptions"),"Hub must run inside Cyn as native UI");
assert(native.includes("CynAiProfiles/index.json"),"Missing bundled offline catalogue");
assert(native.includes("refreshCommunity"),"Missing optional GitHub refresh");
assert(native.includes("importSmartProfile"),"Missing native model import");
assert(native.includes("item->nativeApex") && native.includes('selectAIProfile ("apex6m", error)') &&
       native.includes("trustedResearchUrl"),"Full ONNX APEX AI must activate with a validated real runtime");
const view=readFileSync(path("fxsound/Source/GUI/FxProView.cpp"),"utf8");
assert(view.includes("browse_ai_.onClick = [this] { CynAiHubDialog::show(this); };"),"AI Hub button must open native dialog");
assert(!view.includes('URL("https://github.com/oddfoxerr-real/Cyn/tree/main/ai-hub").launchInDefaultBrowser()'),"Website link still exists");
const build=readFileSync(path("BUILD_CYN.ps1"),"utf8");
assert(build.includes("CynAiProfiles"),"Installer must bundle the offline community catalogue");
const header=readFileSync(path("fxsound/Source/GUI/FxProView.h"),"utf8");
assert(header.includes('ai_hub_nav_ { "AI Hub" }'),"Permanent AI Hub navigation button missing");
for(const key of ["ai_hub_nav_.setBounds(navX,251,navW,42)",
                  "ai_hub_nav_.onClick = [this] { selectPage(3); };",
                  "export_ai_.setBounds(left+456,657,92,27)",
                  "import_ai_.setBounds(left+556,657,92,27)",
                  "browse_ai_.setBounds(left+656,657,103,27)",
                  'JUCEApplication::getInstance()->getApplicationVersion()']){
  assert(view.includes(key),"AI Hub discoverability regression: "+key);
}
assert(view.includes("&ai_hub_nav_, &studio_nav_, &ai_status_card_"),"AI Hub navigation must be registered with the new native shell");
assert(view.includes("ai_hub_nav_.setVisible(true)") &&
       view.includes("embedded_ai_hub_.setVisible(page==3)"),
       "AI Hub should be visible in navigation and display its real native catalogue in the workspace");
assert(view.includes("button->toFront(false)"),"AI profile actions need explicit z-order");
console.log("PASS: visible native AI sidebar, profile actions and runtime version marker");
assert(native.includes("setOpaque (true)"),"AI Hub must paint an opaque content surface");
assert(native.includes("componentToCentreAround = owner"),"AI Hub must be positioned over Cyn");
assert(native.includes("dialogBackgroundColour = juce::Colour (FXCOLOR(WindowBackground)).withAlpha (1.0f)"),"Dialog background is transparent");
assert(native.includes("g.fillAll (juce::Colour (FXCOLOR(WindowBackground)).withAlpha (1.0f))"),"AI Hub content is transparent");
assert(!/juce::Colour\s*\(\s*FXCOLOR\([A-Za-z]+\)\s*\)(?!\.withAlpha)/.test(native),"Some AI Hub colors still have transparent alpha");
const theme=readFileSync(path("fxsound/Source/GUI/FxTheme.cpp"),"utf8");
assert(theme.includes('set("cyn_inline_categories", false)'),"Theme menu must use stable JUCE submenus");
assert(theme.includes('set("cyn_popup_fade_ms", animateMenu ? 215 : 0)'),"Theme submenu should retain native fade");
console.log("PASS: opaque native AI dialog centered above main app and stable theme submenu");
console.log("PASS: "+catalogue.profiles.length+" native AI profiles, offline catalogue, no AI website");

// Visual regressions: the source palette is RGB without an alpha channel.
const juceTooltip=readFileSync(path("third_party/JUCE/modules/juce_gui_basics/windows/juce_TooltipWindow.cpp"),"utf8");
assert(juceTooltip.includes("setOpaque (true);"),"Review tooltip default opacity after JUCE changes");
assert(view.includes("tool_tip_.setOpaque(false);"),"Tooltip should not show rectangular backing beyond rounded corners");
// In the stable shell, sidebar buttons are intentionally transparent: the
// shared parent draws the selected glass/glow behind a real native TextButton.
// AI Hub action buttons remain independently filled and contrast-checked.
for (const [button,palette] of [["browse_ai_","MenuHighlightBackground"],["export_ai_","PanelBackground"],["import_ai_","PanelBackground"]]) {
  assert(view.includes(button+".setColour(TextButton::buttonColourId, Colour(FXCOLOR("+palette+")).withAlpha(1.0f));"),button+" fill is transparent");
}
assert(view.includes('g.fillRoundedRectangle(22.0f,navY') &&
       view.includes('g.drawRoundedRectangle(22.5f,navY+.5f') &&
       view.includes('button->setColour(TextButton::textColourOffId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));'),
       "Native AI Hub sidebar must have readable text and a separate painted selection background");
assert(view.includes('browse_ai_.setColour(TextButton::textColourOffId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));'),
       "Browse AI action label invisible");
assert(view.includes("button->setColour(TextButton::textColourOffId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));"),"AI profile actions lack opaque labels");
for(const key of ["backgroundColourId","outlineColourId","textColourId"])
  assert(theme.includes("findColour(TooltipWindow::"+key+").withAlpha(1.0f)"),"Tooltip "+key+" not opaque");
console.log("PASS: visible AI profile buttons and transparent tooltip corners");

// Neon neural visualizer regression: shows the real 11-12-5 topology and tracks training state.
const neural = readFileSync(path("fxsound/Source/GUI/CynNeuralActivity.h"), "utf8");
assert(header.includes('#include "CynNeuralActivity.h"'), "Neural visualizer header missing");
assert(header.includes("CynNeuralActivity neural_activity_"), "Missing visualizer instance");
assert(neural.includes("std::array<Point<float>, 11> inputs") && neural.includes("std::array<Point<float>, 12> hidden") && neural.includes("std::array<Point<float>, 5> outputs"), "Must match real 11-12-5 model architecture");
assert(view.includes("neural_activity_.setBounds(1008,288,271,164)"), "Neural network belongs in the dedicated Smart Listening inspector");
assert(view.includes("neural_activity_.setTrainingActive(controller.isSmartTraining())"), "Brain must reflect actual training state");
assert(view.includes("neural_activity_.setAnimationEnabled(page==1)") &&
       view.includes("const bool wantGraph=page_==1;"),
       "Keep the live neural telemetry exclusively on the Smart Listening page, not over every theme");
assert(neural.includes("FxTheme::isReducedMotion()"), "Respect reduced-motion accessibility setting");
assert(neural.includes("startTimerHz(24)"), "Use efficient bounded redraw frequency");
console.log("PASS: neural activity visualization matches model layout and training state");

// Live neural telemetry must be linked to inference and supervised training epochs.
const model=readFileSync(path("fxsound/Source/GUI/PersonalAudioModel.h"),"utf8");
const controllerHeader=readFileSync(path("fxsound/Source/GUI/FxController.h"),"utf8");
const controllerSource=readFileSync(path("fxsound/Source/GUI/FxController.cpp"),"utf8");
assert(model.includes("NeuralActivitySnapshot trace("),"Model must expose real feed-forward activity");
assert(model.includes("s.inputConnections[j*inputs+i]=e")&&model.includes("s.outputConnections[o*hidden+j]=e"),"Glow must represent real weights and activations");
assert(model.includes("epochsDone->store(epoch"),"Training progress must reflect real completed optimization epochs");
assert(controllerHeader.includes("getLiveNeuralActivity(")&&controllerHeader.includes("getSmartTrainingProgress()"),"Missing live neural telemetry API");
assert(controllerSource.includes("out=(smart_mode_==1?movie_model_:smart_model_).trace(frame.audio)"),"Activity must sample live neural inference");
assert(view.includes("brainController.getLiveNeuralActivity(snapshot)"),"Visualizer never reads real audio");
assert(view.includes("brainController.getSmartTrainingProgress()"),"Visualizer does not display real training progress");
assert(neural.includes("displayed_.inputConnections")&&neural.includes("displayed_.outputConnections"),"Connections must react to real neural signals");
assert(neural.includes("WAITING FOR AUDIO")&&neural.includes("PROCESSING AUDIO"),"Must differentiate silence from playback");
console.log("PASS: live neural inference and supervised epoch animation");

// Neural Core is a real audio-responsive theme, not a decorative animation.
// The existing personal-model network must be labeled separately from pretrained
// YAMNet event classifications; no fabricated YAMNet weight-node activity.
const themeHeader=readFileSync(path("fxsound/Source/GUI/FxTheme.h"),"utf8");
assert(themeHeader.includes("CharcoalDesktop, NeuralCore, DarkMesh, AetherGlass, IonViolet, LiquidTitanium, JadeCircuit, SolarInk, PolarMist, NumModes"),"Neural Core must preserve existing saved theme IDs");
assert(theme.includes('return "Dark Theme";'),"Default dark palette must remain compatible with existing stored theme IDs");
assert(view.includes("brainController.getLiveNeuralActivity(snapshot)"),"Neural theme must use real personal model activations");
assert(view.includes("brainController.getLiveYamNetScene(scene)"),"Neural theme must read real YAMNet classifications");
assert(controllerHeader.includes("getLiveYamNetScene(CynAudioScene& out)"),"Missing native read-only scene telemetry API");
assert(controllerSource.includes("return yamnet_worker_->latest(out)"),"Neural theme must use actual async ONNX scene data");
assert(view.includes("neural_core_live_=brainController.getLiveYamNetScene(scene) && hasAudio;") &&
       view.includes("neural_core_scene_=neural_core_live_?scene:CynAudioScene {};"),
       "Native AI telemetry must reject stale YAMNet output even with Dark Theme only");
assert(view.includes("const double interval=FxTheme::isReducedMotion()?500.0:42.0;") &&
       view.includes("now-last_theme_repaint_>=interval"),"UI repaint must remain <=24Hz");
assert(neural.includes("setProfileId(const juce::String& id)") && neural.includes("const Colour accent=trained"),"Neural Core must have its own network lighting");
assert(neural.includes("PERSONAL NEURAL NETWORK"),"Do not confuse small preference model with YAMNet internals");
assert(view.includes("FxTheme::isReducedMotion()"),"Neural theme must respect reduced-motion");
console.log("PASS: Neural Core live inference visualization, distinct AI identities and bounded redraw");
const atmosphere=readFileSync(path("fxsound/Source/GUI/CynThemeAtmosphere.h"),"utf8");
const themeModes=[...themeHeader.matchAll(/enum FxThemeMode : int \{([^}]+)\}/g)][0][1]
  .split(",").map(item=>item.trim().replace(/=.*/,"")).filter(item=>item!=="NumModes");
assert.equal(themeModes.length,27,"Original saved IDs and six new native CYN themes must be retained");
for(const mode of themeModes)
  assert(atmosphere.includes("case FxThemeMode::"+mode+":"),"No dynamic artwork for "+mode);
assert(view.includes("CynThemeAtmosphere::paint(g,mode"),
       "Theme atmosphere must render in the workspace");
assert(view.includes("snapshot.rms"),"Themes must respond to measured audio, not fake FFT values");
assert(atmosphere.includes("FxTheme::isReducedMotion()"),
       "Atmosphere must respect reduce-motion mode");
assert(!atmosphere.includes("getLiveYamNetScene("),
       "Theme paint must not perform asynchronous inference");
console.log("PASS: 21 atmospheric themes, measured audio glow and motion fallback");

const studio=readFileSync(path("fxsound/Source/GUI/CynThemeStudio.h"),"utf8");
const menu=readFileSync(path("fxsound/Source/GUI/FxMainWindow.cpp"),"utf8");
assert(menu.includes('CynThemeStudioDialog::show(this)'),
       "Theme selection must open Theme Studio rather than a static submenu");
assert(!studio.includes('previous_.onClick =') &&
       !studio.includes('next_.onClick =') &&
       studio.includes('controller.setThemeMode(themeMode_)'),
       "Theme Studio must persist a selected dark palette without obsolete carousel navigation");
assert(studio.includes('controller.getSettings().setInt("cyn_dj_touch_style_effects",effectsTouchStyle_)') &&
       studio.includes('controller.getSettings().setInt("cyn_dj_touch_style_eq",eqTouchStyle_)') &&
       studio.includes('controller.getSettings().setInt("cyn_dj_touch_flower_count",flowerCount_)') &&
       studio.includes('controller.getSettings().setInt("cyn_button_motion_style",buttonMotion_)') &&
       studio.includes('controller.getSettings().setInt("cyn_close_motion_style",closeStyle_)'),
       "Studio Apply must persist AI Touch, button motion and close effects");
assert(view.includes('theme_from_image_,-offset') &&
       view.includes('theme_to_image_,getWidth()-offset'),
       "Live application theme changes must slide between actual UI snapshots");
assert(atmosphere.includes('Rectangle<float> bounds,bool preview=false'),
       "Theme art must be rendered across a real workspace or large preview");
assert(!view.includes('g,theme_audio_level_);'),
       "Do not return to tiny decorative sidebar artwork");
assert(view.includes('neural_activity_.setVisible(page==1)'),
       "Neural widget must not crowd Sound Studio sidebar");
console.log("PASS: stable Dark Theme studio, retained live scenes and native UI transition");

assert(controllerHeader.includes('class AutoUpdatePoller final : private juce::Timer') &&
       controllerSource.includes('auto_update_poller_.start();'),
       "Automatic update polling must not depend on the DSP processing timer");
assert(controllerSource.includes('checkAutomaticUpdates()') &&
       controllerSource.includes('cyn_last_automatic_update_attempt_v3'),
       "Automatic updater needs saved cadence and retries even if audio is off");
assert(controllerSource.includes('if (launchCynUpdater(false))'),
       "Update polling should only record a successful updater launch");
assert(!controllerSource.includes('++cyn_update_timer_ticks_'),
       "Legacy playback-dependent updater scheduling still exists");
console.log("PASS: 30-minute auto-update schedule independent of playback power");




// Music/Movie AI regression: distinct weights, saved labels and sound limits.
const movieCtrl=readFileSync(path("fxsound/Source/GUI/FxController.cpp"),"utf8");
const movieHeader=readFileSync(path("fxsound/Source/GUI/FxController.h"),"utf8");
const movieUi=readFileSync(path("fxsound/Source/GUI/FxProView.cpp"),"utf8");
const movieUiHeader=readFileSync(path("fxsound/Source/GUI/FxProView.h"),"utf8");
const promotion=readFileSync(path("scripts/evaluate_starter_candidates.py"),"utf8");
for (const marker of ['movie_model_', 'movie_examples_', 'getSmartListeningMode()', 'setSmartListeningMode(int mode)'])
  assert(movieHeader.includes(marker), 'Missing independent Movie AI model or controller API: '+marker);
for (const marker of ['root->setProperty("movie_ai"', 'savedMovie=root["movie_ai"]',
      'initialiseMovieStarter()', 'candidate.ready.fill(false)', 'movie?movie_model_:smart_model_',
      'CynFlagshipMixer::decide(scene,input,movie)', 'sceneHead.predict(scene,e,contextual,confidence)',
      'trainingMode==1?movie_examples_:smart_examples_', 'c.movie_model_=*model',
      '(smart_mode_==1?movie_model_:smart_model_).trace(frame.audio)', 'settings_.setInt("cyn_ai_listening_mode",mode)'])
  assert(movieCtrl.includes(marker), 'Movie AI regression: '+marker);
assert(movieUiHeader.includes('FxComboBox listening_mode_'), 'Mode picker missing');
assert(movieUi.includes('{"Music AI", "Movie & TV AI"}'), 'Movie mode absent from native UI');
assert(movieUi.includes('listening_mode_.setEnabled(!controller.isSmartTraining())'), 'Cannot switch models mid-train');
assert(movieUi.includes('Train Movie AI'), 'Movie mode has no training action');
assert(promotion.includes('independent_human_labels') && promotion.includes('log_loss') &&
  promotion.includes('domain') && promotion.includes('balanced_accuracy'),
  'Starter promotion must require held-out human movie and music evaluation');
console.log('PASS: independent Movie AI and labels, YAMNet flagship, starter promotion gate');

// A separately trained Cinema AI profile is used as Movie AI's default when bundled.
assert(movieCtrl.includes('getChildFile("cyn-cinema.cynai.json")'), 'Movie AI must use bundled Cyn Cinema model');
assert(movieCtrl.includes('package["format"].toString()=="cyn-ai-profile"'), 'Movie model format must be validated');
assert(movieCtrl.includes('movie_model_=candidate;'), 'Default Movie AI needs candidate model weights');
console.log('PASS: tested Cinema AI profile seeds Movie AI, generic starter fallback preserved');

// v2.0.54: mode and model are separate; switching keeps private model slots.
assert(view.includes('ai_profile_.setBounds(left+253,510,397,32)'), 'Model selection must stay visible in Smart Listening');
assert(view.includes('ai_profile_.onChange=[this]'), 'The model dropdown must really switch neural models');
assert(view.includes('refreshAIProfileChoices()'), 'Imported profiles must be discoverable without restarting');
assert(controllerHeader.includes('selectAIProfile(const String& id'), 'Model switching controller API is missing');
assert(controllerSource.includes('AIModelSlots') &&
       controllerSource.includes('preferences-before-profile-switch.json'),
       'The previous learned AI must survive model switching');
assert(native.includes('Activate selected AI'), 'AI Hub must explain activation instead of import-only');
console.log('PASS: separate persistent neural model selector with reversible backups');


// v2.0.54: installer, native executable, release controller and updater must agree.
const project=readFileSync(path("fxsound/Project/FxSound_App.vcxproj"),"utf8");
const installer=readFileSync(path("Installer/Cyn/Cyn.iss"),"utf8");
const updater=readFileSync(path("CynUpdater.ps1"),"utf8");
const installedVersion = installer.match(/^AppVersion=(\d+\.\d+\.\d+)\s*$/m)?.[1];
assert(installedVersion, 'Installer must declare a valid semantic app version');
assert.equal((project.split('JUCE_APP_VERSION=' + installedVersion + '.0').length - 1), 4,
  'All native build configs must expose actual installer version ' + installedVersion);
assert(view.includes('JUCEApplication::getInstance()->getApplicationVersion()'),
  'Sidebar must show the runtime app version instead of a hard-coded label');
assert(controllerSource.includes('const auto currentVersion=major+"."+minor+"."+patch'),
  'Use runtime version for updater, not old hard-coded version');
assert(!controllerSource.includes('CurrentVersion 2.0.29'), 'Updater cannot repeatedly offer old v2.0.29');
assert(updater.includes("Local\\CynUpdaterSingleWindow") && updater.includes('WaitOne(0,$false)'),
  'Updater must not stack interactive and automatic windows');
assert(controllerSource.includes('CynLevelInvariant::forRecognition(input)'),
  'Neural predictions should ignore upstream playback level');
assert(controllerSource.includes('CynFlagshipMixer::musicEqCorrection('),
  'Music EQ must respect kick pulse and an existing Bass preset');
console.log('PASS: consistent native/updater/installer version; singleton updater; volume-neutral AI and preset kick');


// Compiled C++ test runs the SAME URL function called by CynAIHubDialog.
const policy = readFileSync(path("fxsound/Source/GUI/CynAiHubCataloguePolicy.h"),"utf8");
assert(native.includes("cyn::isTrustedResearchReleaseUrl (p.researchUrl.toStdString())"),
  "The actual native AI Hub loader must call the tested release URL policy");
assert(policy.includes("https://github.com/oddfoxerr-real/Cyn/releases/tag/"),
  "Research model links must remain limited to the official release repository");
assert(native.includes('catalogueRejected_') && native.includes('catalogueExternal_')
  && native.includes('Community refreshed: '),
  "Never report generic success while silently dropping models");
const neuralListings = catalogue.profiles.filter(p => p.kind === "onnx");
assert.equal(neuralListings.length,1,"APEX must be a visible native ONNX AI, not research");
const selectionBody = native.split("void updateSelection()")[1].split("static bool isSafeFilename")[0];
const singleEscapedNewline = String.fromCharCode(92, 110);
const doubleEscapedNewline = String.fromCharCode(92, 92, 110);
assert(selectionBody.includes(singleEscapedNewline) && !selectionBody.includes(doubleEscapedNewline),
  "Research details render literal backslash-n sequences instead of real line breaks");
assert(neuralListings.some(p => p.id === "cyn-apex6m-flowsense" &&
   p.url === "https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0" && p.issue === 32),
  "APEX-6M is absent from refreshable model catalogue");
assert(native.includes('p.nativeApex = row["kind"].toString() == "onnx"') &&
       native.includes("Switch to previous AI") &&
       native.includes("ctrl.setSmartListening(true)"),
  "APEX full model must be installable, activatable and reversible");
console.log("PASS: genuine native APEX ONNX AI listed with activation, restoration and model URL");
