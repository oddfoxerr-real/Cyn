import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const load=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const view=load("fxsound/Source/GUI/FxProView.cpp");
const header=load("fxsound/Source/GUI/FxProView.h");
const fx=load("fxsound/Source/GUI/FxAudioControls.cpp");
const studio=load("fxsound/Source/GUI/CynThemeStudio.h");

assert(header.includes("WIDTH = 1320, HEIGHT = 700"),"Stable workbench must have room for all native controls");
assert(header.includes('CynAiHubContent embedded_ai_hub_') &&
       header.includes('studio_nav_ { "CYN Studio",5 }') &&
       header.includes('ai_status_card_ { "AI status" }') &&
       header.includes("void selectSoundFocus(int focus);"),"Navigational controls must exist as real JUCE Buttons");
for(const s of ["sound_nav_.onClick","smart_nav_.onClick","devices_nav_.onClick",
                "ai_hub_nav_.onClick","studio_nav_.onClick","settings_.onClick",
                "overview_tab_.onClick","eq_focus_tab_.onClick","effects_focus_tab_.onClick"])
  assert(view.includes(s),"Missing clickable navigation action: "+s);
assert(view.includes("CynThemeStudioDialog::show(this)") &&
       studio.includes("buttonMotionPicker_"),"Studio nav must open actual dark customization");
assert(view.includes('void FxProView::selectSoundFocus(int focus)') &&
       view.includes('if(sound_focus_==0)') &&
       view.includes('else if(sound_focus_==1)') &&
       view.includes('audio_controls_.setVisible(sound_focus_!=1)') &&
       view.includes('equalizer_.setVisible(sound_focus_!=2)'),
       "Overview/EQ/FX navigation must actually rearrange live components");
assert(view.includes('equalizer_.setBounds(eqX,288,eqW,193)') &&
       view.includes('visualizer_.setBounds(1002,288,282,193)') &&
       view.includes('audio_controls_.setBounds(eqX,536,eqW,136)'),
       "Default layout must display actual 31-band EQ, visualizer and DSP effects");
assert(fx.includes("getWidth() >= 540") &&
       fx.includes("const int cell=(getWidth()-margin*2-gap*4)/5"),
       "The five real DSP effect sliders must adapt to a wide horizontal rack");
assert(view.includes('embedded_ai_hub_.setBounds(247,250,1053,435)') &&
       view.includes('ai_hub_nav_.onClick = [this] { selectPage(3); };') &&
       view.includes('embedded_ai_hub_.setVisible(page==3)'),
       "AI Hub should be an actual full CYN workspace page, not a navigation-only popup");
assert(view.includes('learning_timeline_.setBounds(left+13,288,722,164)') &&
       view.includes('neural_activity_.setBounds(1008,288,271,164)') &&
       view.includes('learning_effect_.setBounds(left+22,548,179,32)'),
       "Smart Listening timeline, neural analyzer and learning controls must remain interactive");
assert(view.includes('pin_toggle_.setBounds') &&
       view.includes('refresh_.setBounds') &&
       view.includes('dj_intensity_.setBounds') &&
       view.includes('dj_spacing_.setBounds'),
       "Device and AI DJ subpages must preserve real settings");
assert(view.includes('modelText=model=="apex6m"') &&
       view.includes('FxController::getInstance().getActiveAIProfileId()') &&
       view.includes('FxController::getInstance().isSmartListening()'),
       "The AI status panel must show actual engine state, not invented model status");
assert(view.includes("FxThemeMode::DarkMesh") &&
       view.includes('g.fillRoundedRectangle(26.0f,navY') &&
       view.includes('CynThemeAtmosphere::paint(g,mode') &&
       view.includes("reduced?0.0f"),
       "Horizon x Depth native shell must retain live Dark Theme and reduced motion");
assert(!view.includes('g.drawArc(juce::Path(),0)'),"No placeholder painting calls");
console.log("PASS: native stable-candidate shell, real EQ/DSP/AI widgets, dark continuity");
