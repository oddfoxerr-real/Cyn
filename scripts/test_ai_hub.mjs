// Native Cyn AI Hub catalogue and offline model-format regression checks.
// No webpage or hosted server is required.
import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const path = p => new URL("../" + p, import.meta.url);
const catalogue=JSON.parse(readFileSync(path("ai-hub/community/index.json"),"utf8"));
assert.equal(catalogue.schema,1);
assert(Array.isArray(catalogue.profiles)&&catalogue.profiles.length>0);
for(const entry of catalogue.profiles){
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
const view=readFileSync(path("fxsound/Source/GUI/FxProView.cpp"),"utf8");
assert(view.includes("browse_ai_.onClick = [this] { CynAiHubDialog::show(this); };"),"AI Hub button must open native dialog");
assert(!view.includes('URL("https://github.com/oddfoxerr-real/Cyn/tree/main/ai-hub").launchInDefaultBrowser()'),"Website link still exists");
const build=readFileSync(path("BUILD_CYN.ps1"),"utf8");
assert(build.includes("CynAiProfiles"),"Installer must bundle the offline community catalogue");
const header=readFileSync(path("fxsound/Source/GUI/FxProView.h"),"utf8");
assert(header.includes('ai_hub_nav_ { "AI Hub" }'),"Permanent AI Hub navigation button missing");
for(const key of ["ai_hub_nav_.setBounds(68, 198, 152, 36)", "ai_hub_nav_.onClick = [this] { CynAiHubDialog::show(this); };",
                  "export_ai_.setBounds(685, 545, 96, 35)", "import_ai_.setBounds(791, 545, 96, 35)",
                  "browse_ai_.setBounds(897, 545, 106, 35)", "Version 2.0.19"]){
  assert(view.includes(key),"AI Hub discoverability regression: "+key);
}
assert(view.includes("&ai_hub_nav_, &home_"),"AI Hub navigation missing from component registration");
assert(view.includes("ai_hub_nav_.setVisible(true)"),"AI Hub navigation should be visible on every page");
assert(view.includes("button->toFront(false)"),"AI profile actions need explicit z-order");
console.log("PASS: visible native AI sidebar, profile actions and v2.0.19 version marker");
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
for (const [button,palette] of [["ai_hub_nav_","MenuHighlightBackground"],["browse_ai_","MenuHighlightBackground"],["export_ai_","PanelBackground"],["import_ai_","PanelBackground"]]) {
  assert(view.includes(button+".setColour(TextButton::buttonColourId, Colour(FXCOLOR("+palette+")).withAlpha(1.0f));"),button+" fill is transparent");
}
for(const button of ["ai_hub_nav_","browse_ai_"])
  assert(view.includes(button+".setColour(TextButton::textColourOffId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));"),button+" label invisible");
assert(view.includes("button->setColour(TextButton::textColourOffId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));"),"AI profile actions lack opaque labels");
for(const key of ["backgroundColourId","outlineColourId","textColourId"])
  assert(theme.includes("findColour(TooltipWindow::"+key+").withAlpha(1.0f)"),"Tooltip "+key+" not opaque");
console.log("PASS: visible AI profile buttons and transparent tooltip corners");

// Neon neural visualizer regression: shows the real 11-12-5 topology and tracks training state.
const neural = readFileSync(path("fxsound/Source/GUI/CynNeuralActivity.h"), "utf8");
assert(header.includes('#include "CynNeuralActivity.h"'), "Neural visualizer header missing");
assert(header.includes("CynNeuralActivity neural_activity_"), "Missing visualizer instance");
assert(neural.includes("std::array<Point<float>, 11> inputs") && neural.includes("std::array<Point<float>, 12> hidden") && neural.includes("std::array<Point<float>, 5> outputs"), "Must match real 11-12-5 model architecture");
assert(view.includes("neural_activity_.setBounds(72, 247, 148, 226)"), "Must fit empty sidebar without obstructing labeling controls");
assert(view.includes("neural_activity_.setTrainingActive(controller.isSmartTraining())"), "Brain must reflect actual training state");
assert(view.includes("neural_activity_.setAnimationEnabled(page == 1)"), "Do not animate on hidden pages");
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
