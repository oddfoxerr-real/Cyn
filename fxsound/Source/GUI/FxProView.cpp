/* FxSound / Cyn. Copyright (C) 2025 FxSound LLC.
   SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FxProView.h"
#include "FxController.h"
#include "FxTheme.h"
#include "CynThemeAtmosphere.h"
#include "CynUiMotion.h"
#include "FxSettingsDialog.h"
#include "CynAiHubDialog.h"
#include "CynThemeStudio.h"
#include <array>
#include <cmath>

FxProView::FxProView() : tool_tip_(this)
{
    // JUCE TooltipWindow is opaque by default; our rounded corners require compositing.
    tool_tip_.setOpaque(false);
    for (auto* component : std::initializer_list<Component*> {
        &audio_controls_, &equalizer_, &visualizer_, &sound_nav_, &smart_nav_,
        &devices_nav_, &ai_hub_nav_, &studio_nav_, &ai_status_card_, &overview_tab_, &eq_focus_tab_, &effects_focus_tab_, &home_, &smart_rail_, &devices_rail_, &settings_,
        &refresh_, &smart_toggle_, &pin_toggle_, &live_status_, &smart_status_, &settings_rail_, &sensitivity_, &headroom_cap_, &preset_cycling_, &preset_cycling_status_, &learning_timeline_, &neural_activity_, &learning_effect_, &listening_mode_, &ai_profile_, &more_, &less_, &train_, &starter_, &resume_, &learning_status_, &export_ai_, &import_ai_, &browse_ai_ })
        addAndMakeVisible(component);
    for(auto* c:std::initializer_list<Component*> {&misc_nav_,&dj_download_,&dj_remove_,&dj_cancel_,&dj_toggle_,&dj_stereo_,&dj_intensity_,&dj_spacing_,&dj_status_,&dj_recent_,&dj_progress_}) addAndMakeVisible(c);
    misc_nav_.onClick=[this]{selectPage(3);};
    studio_nav_.onClick=[this]{CynThemeStudioDialog::show(this);};
    ai_status_card_.onClick=[this]{selectPage(1);};
    overview_tab_.onClick=[this]{selectSoundFocus(0);};
    eq_focus_tab_.onClick=[this]{selectSoundFocus(1);};
    effects_focus_tab_.onClick=[this]{selectSoundFocus(2);};
    studio_nav_.setTooltip("Open native Dark Theme, AI Touch and animation personalization.");
    ai_status_card_.setTooltip("Open Smart Listening and neural AI controls.");
    dj_download_.onClick=[]{if(auto* dj=FxController::getInstance().dj()) dj->download();};
    dj_cancel_.onClick=[]{if(auto* dj=FxController::getInstance().dj()) dj->cancelDownload();};
    dj_remove_.onClick=[]{if(auto* dj=FxController::getInstance().dj()) dj->remove();};
    dj_toggle_.onClick=[this]{if(auto* dj=FxController::getInstance().dj()) dj->setEnabled(dj_toggle_.getToggleState());};
    dj_intensity_.setName("AI DJ intensity");dj_intensity_.setRange(0,100,1);dj_intensity_.setTextValueSuffix("%");
    dj_spacing_.setName("Minimum time between effects");dj_spacing_.setRange(4,45,1);dj_spacing_.setTextValueSuffix(" s");
    if(auto* dj=FxController::getInstance().dj()) {
        dj_intensity_.setValue(dj->intensity()*100,dontSendNotification);
        dj_spacing_.setValue(dj->spacing(),dontSendNotification);
        dj_stereo_.setToggleState(dj->stereo(),dontSendNotification);
    }
    auto configure=[this]{if(auto* dj=FxController::getInstance().dj()) dj->configure(float(dj_intensity_.getValue()/100),float(dj_spacing_.getValue()),dj_stereo_.getToggleState());};
    dj_intensity_.onValueChange=configure;dj_spacing_.onValueChange=configure;dj_stereo_.onClick=configure;
    dj_status_.setFont(14);dj_recent_.setFont(13);dj_status_.setMinimumHorizontalScale(0.8f);
    sound_nav_.onClick = home_.onClick = [this] { selectPage(0); };
    smart_nav_.onClick = smart_rail_.onClick = [this] { selectPage(1); };
    devices_nav_.onClick = devices_rail_.onClick = [this] { selectPage(2); };
    ai_hub_nav_.onClick = [this] { CynAiHubDialog::show(this); };
    ai_hub_nav_.setTooltip("Manage shared AI profiles in Cyn. No separate website.");
    home_.setTooltip("Sound studio");
    smart_rail_.setTooltip("Smart listening");
    devices_rail_.setTooltip("Output devices");
    settings_.onClick = [this] { openSettings(); };
    settings_rail_.onClick = [this] { openSettings(); };
    settings_rail_.setTooltip("Settings");
    sensitivity_.setName("Smart listening sensitivity");
    sensitivity_.setRange(0, 100, 1);
    sensitivity_.setTextValueSuffix("%");
    sensitivity_.setTextBoxStyle(Slider::TextBoxRight, false, 64, 26);
    sensitivity_.setTooltip("0% keeps your preset. 50% uses gentle AI adjustments. At 100%, Music AI can use the full 0–10 effect range when the song calls for it; peak protection remains active.");
    sensitivity_.onValueChange = [this] {
        FxController::getInstance().setSmartSensitivity((float)sensitivity_.getValue());
    };
    headroom_cap_.setName("Headroom cap");
    headroom_cap_.setRange(0,100,1);
    headroom_cap_.setTextValueSuffix("%");
    headroom_cap_.setTextBoxStyle(Slider::TextBoxRight,false,58,26);
    headroom_cap_.setTooltip("0% conservative headroom, 50% original limit, 100% maximum permitted AI freedom. High source peaks still restrict extra digital gain; not a true-peak limiter.");
    headroom_cap_.onValueChange=[this] {
        FxController::getInstance().setSmartHeadroomCap((float)headroom_cap_.getValue());
    };
    preset_cycling_.setTooltip(
        "Optional: select suitable built-in presets for musical passages, with smooth live DSP transitions. Your manually selected preset and trained AI stay unchanged.");
    preset_cycling_.onClick=[] {
        FxController::getInstance().setSmartPresetCycling(
            FxController::getInstance().isSmartPresetCycling()==false);
    };
    preset_cycling_status_.setInterceptsMouseClicks(false,false);
    preset_cycling_status_.setFont(11);
    listening_mode_.addItemList({"Music AI", "Movie & TV AI"},1);
    listening_mode_.setSelectedId(1,dontSendNotification);
    listening_mode_.setTooltip("Music and movies have separate neural models and learning history. Movie AI uses conservative dialogue-focused sound shaping until trained.");
    ai_profile_.setTooltip("Select the actual neural model. Each listening mode can use a different AI; previous model training is backed up.");
    ai_profile_.onChange=[this] {
        auto& controller=FxController::getInstance();
        const int index=ai_profile_.getSelectedId()-1;
        if(index<0 || index>=ai_profile_ids_.size()) return;
        String error;
        if(!controller.selectAIProfile(ai_profile_ids_[index],error))
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"AI model not activated",error);
        refreshAIProfileChoices();
        update();
    };
    refreshAIProfileChoices();
    listening_mode_.onChange=[this] {
        auto& controller=FxController::getInstance();
        controller.setSmartListeningMode(listening_mode_.getSelectedId()-1);
        listening_mode_.setSelectedId(controller.getSmartListeningMode()+1,dontSendNotification);
        refreshAIProfileChoices();
        learning_timeline_.clearSelection();
        update();
    };
    learning_effect_.addItemList({"Clarity", "Ambience", "Surround", "Dynamic Boost", "Bass"},1);
    learning_effect_.setSelectedId(1,dontSendNotification);
    learning_effect_.setTooltip("Choose the effect to teach for the selected time range.");
    auto label=[this](bool more) {
        auto selected=learning_timeline_.selection();
        if(selected.size()<5) return;
        FxController::getInstance().labelAudio(selected,learning_effect_.getSelectedId()-1,more);
        learning_timeline_.clearSelection(); timerCallback();
    };
    more_.onClick=[label]{label(true);}; less_.onClick=[label]{label(false);};
    train_.onClick=[this] {
        FxController::getInstance().trainSmartModel();
        neural_activity_.setTrainingActive(FxController::getInstance().isSmartTraining());
    };
    resume_.onClick=[this]{learning_timeline_.clearSelection();timerCallback();};
    starter_.onClick=[] {
        if(AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon,"Reset personal learning?",
            FxController::getInstance().getSmartListeningMode()==1 ?
                "Reset Movie AI and its labels? Music AI will be preserved." :
                "Reset Music AI and its labels to the starter model? Movie AI will be preserved.",
            "Reset","Cancel"))
            FxController::getInstance().useStarterModel();
    };
    export_ai_.onClick = [] {
        const auto location = File::getSpecialLocation(File::userDocumentsDirectory)
            .getChildFile("My Cyn AI.cynai.json");
        FileChooser chooser("Export a portable Cyn AI profile", location, "*.cynai.json");
        if (!chooser.browseForFileToSave(true))
            return;
        const auto file = chooser.getResult();
        String error;
        if (FxController::getInstance().exportSmartProfile(
                file, file.getFileNameWithoutExtension().replace(".cynai", ""), error))
            AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "AI exported",
                "Your neural profile was exported without private audio samples or labels.\n"
                "You can share it through the Cyn AI Hub.");
        else
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Export failed", error);
    };
    import_ai_.onClick = [] {
        FileChooser chooser("Import a Cyn AI Hub profile",
            File::getSpecialLocation(File::userDocumentsDirectory), "*.cynai.json;*.json");
        if (!chooser.browseForFileToOpen())
            return;
        if (!AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, "Import this AI?",
            "This replaces your current neural model. Cyn will first back up your existing "
            "learning data to preferences-before-import.json.\nImport this model?", "Import", "Cancel"))
            return;
        String error;
        if (FxController::getInstance().importSmartProfile(chooser.getResult(), error))
            AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "AI imported",
                "The new AI profile is now active. You can keep training it locally with More / Less.");
        else
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Import failed", error);
    };
    browse_ai_.onClick = [this] { CynAiHubDialog::show(this); };
    export_ai_.setTooltip("Export your trained weights, without audio or training history.");
    import_ai_.setTooltip("Import and use a shared Cyn AI model; backs up your existing model.");
    browse_ai_.setTooltip("Browse AI profiles inside Cyn. No website or server needed.");
    learning_status_.setFont(13); learning_status_.setColour(Label::textColourId,Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    refresh_.onClick = [] { FxController::getInstance().checkDeviceChanges(); FxController::getInstance().refreshOutputList(); };
    smart_toggle_.onClick = [this] {
        FxController::getInstance().setSmartListening(smart_toggle_.getToggleState());
        update();
    };
    pin_toggle_.onClick = [this] { FxController::getInstance().setOutputPinned(pin_toggle_.getToggleState()); };
    smart_toggle_.setTooltip("Learn your sound preferences from any audio: movies, TV episodes, games, podcasts, music or dialogue. Select recent audio and teach More / Less.");
    pin_toggle_.setTooltip("Ignore newly connected higher-priority devices while your current output is available. Fall back when it is unplugged.");
    live_status_.setJustificationType(Justification::centredRight);
    live_status_.setInterceptsMouseClicks(false, false);
    smart_status_.setInterceptsMouseClicks(false, false);
    // Standby is an overlay, not a separate window: settings and EQ remain
    // visible, but no controls can accidentally modify audio while Off.
    addAndMakeVisible(power_curtain_);
    power_curtain_.onPowerRequested=[] {
        FxController::getInstance().setPowerState(true);
        if(!FxModel::getModel().getPowerState())
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                "CYN cannot start",
                "Audio processing is unavailable. Check your output device "
                "and make sure CYN is not running in a Remote Desktop session.");
    };
    setSize(WIDTH, HEIGHT);
    selectPage(0);
    power_curtain_.syncPower(FxModel::getModel().getPowerState());
    power_curtain_.toFront(false);
    if(FxTheme::isReducedMotion()) startTimer(500); else startTimerHz(60);
}

void FxProView::beginThemeTransition()
{
    theme_from_image_=Image();
    theme_to_image_=Image();
    if (isShowing() && !FxTheme::isReducedMotion())
        theme_from_image_=createComponentSnapshot(getLocalBounds());
}
void FxProView::finishThemeTransition()
{
    if (!theme_from_image_.isValid() || FxTheme::isReducedMotion())
        return;
    theme_to_image_=createComponentSnapshot(getLocalBounds());
    theme_transition_start_=Time::getMillisecondCounterHiRes();
    repaint();
}

void FxProView::startVisualizer() { visualizer_.start(); }
void FxProView::pauseVisualizer() { visualizer_.pause(); }
void FxProView::setLookAndFeel() { audio_controls_.setLookAndFeel(); repaint(); }
void FxProView::openSettings()
{
    FxSettingsDialog dialog;
    dialog.runModalLoop();
    FxController::getInstance().refreshOutputList();
    update();
}
void FxProView::selectSoundFocus(int focus)
{
    sound_focus_=juce::jlimit(0,2,focus);
    overview_tab_.setToggleState(sound_focus_==0,dontSendNotification);
    eq_focus_tab_.setToggleState(sound_focus_==1,dontSendNotification);
    effects_focus_tab_.setToggleState(sound_focus_==2,dontSendNotification);
    resized();
    repaint();
}
void FxProView::selectPage(int page)
{
    if(page == page_ && isShowing()) return; // Initial page must still initialize child visibility.
    // Immediately select the destination. Snapshot ONLY the outgoing content,
    // so the sidebar stays live and responsive even on slower machines.
    transition_image_ = Image(); // Never capture a previous transition overlay.
    if(isShowing() && !FxTheme::isReducedMotion()) {
        const juce::Rectangle<int> content(240, 0, jmax(0,getWidth()-240), getHeight());
        if(content.getWidth() > 0 && content.getHeight() > 0) {
            transition_image_ = createComponentSnapshot(content);
            transition_direction_ = page > page_ ? 1 : -1;
            transition_start_ = Time::getMillisecondCounterHiRes();
        }
    }
    page_ = page;
    misc_nav_.setToggleState(page==3,dontSendNotification);
    for(auto* b:std::initializer_list<TextButton*> {
        &overview_tab_, &eq_focus_tab_, &effects_focus_tab_ })
        b->setVisible(page==0);
    ai_status_card_.setVisible(true);
    for(auto* c:std::initializer_list<Component*> {&dj_download_,&dj_remove_,&dj_cancel_,&dj_toggle_,&dj_stereo_,&dj_intensity_,&dj_spacing_,&dj_status_,&dj_recent_,&dj_progress_}) c->setVisible(page==3);
    smart_toggle_.setVisible(page!=3);smart_status_.setVisible(page!=3);
    sound_nav_.setToggleState(page == 0, dontSendNotification);
    smart_nav_.setToggleState(page == 1, dontSendNotification);
    devices_nav_.setToggleState(page == 2, dontSendNotification);
    home_.setToggleState(page == 0, dontSendNotification);
    // Legacy compact-rail glyphs are replaced by one legible labelled sidebar.
    home_.setVisible(false);smart_rail_.setVisible(false);
    devices_rail_.setVisible(false);settings_rail_.setVisible(false);
    studio_nav_.setVisible(true);
    smart_rail_.setToggleState(page == 1, dontSendNotification);
    devices_rail_.setToggleState(page == 2, dontSendNotification);
    for (auto* button : std::initializer_list<TextButton*> { &sound_nav_, &smart_nav_, &devices_nav_, &ai_hub_nav_, &misc_nav_, &studio_nav_, &home_, &smart_rail_, &devices_rail_ })
    {
        button->setColour(TextButton::buttonColourId, Colours::transparentBlack);
        button->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
        button->setColour(TextButton::textColourOnId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
        button->setColour(TextButton::textColourOffId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        button->getProperties().set("ember_navigation", true);
    }
    // The AI Hub navigation is available on every main page.
    ai_hub_nav_.setVisible(true);
    audio_controls_.setVisible(page == 0);
    equalizer_.setVisible(page == 0);
    visualizer_.setVisible(page == 0);
    for(auto* c : std::initializer_list<Component*> {&learning_timeline_, &learning_effect_, &listening_mode_, &ai_profile_, &more_, &less_, &train_, &starter_, &resume_, &learning_status_, &export_ai_, &import_ai_, &browse_ai_}) c->setVisible(page==1);
    pin_toggle_.setVisible(page == 2);
    refresh_.setVisible(page == 2);
    sensitivity_.setVisible(page == 1);
    headroom_cap_.setVisible(page == 1);
    preset_cycling_.setVisible(page == 1);
    preset_cycling_status_.setVisible(page == 1);
    const bool neuralCore=FxTheme::getThemeMode()==FxThemeMode::NeuralCore;
    neural_activity_.setVisible(page==1);
    neural_activity_.setAnimationEnabled(page==1);
    resized();
    if (page_ == 1)
    {
        // Keep new AI actions above legacy widgets in the page's z-order.
        for (auto* button : std::initializer_list<TextButton*> {
                 &export_ai_, &import_ai_, &browse_ai_ })
            button->toFront(false);
    }
    ai_hub_nav_.toFront(false);
    studio_nav_.toFront(false);
    ai_status_card_.toFront(false);
    power_curtain_.toFront(false);
    repaint();
}
void FxProView::update()
{
    auto& controller = FxController::getInstance();
    // A model/preset refresh must not interrupt an in-flight, real AI/DSP
    // visual transition. Initial render, manual/AI-off mode, disabled power,
    // and reduced-motion settings still snap to the actual applied values.
    const bool snapValues = !controller.isSmartListening()
        || last_ai_controls_update_ <= 0.0
        || FxTheme::isReducedMotion()
        || !FxModel::getModel().getPowerState();
    audio_controls_.update(snapValues);
    equalizer_.update(snapValues);
    equalizer_.showValues(true);
    audio_controls_.showValues(true);
    smart_toggle_.setToggleState(controller.isSmartListening(), dontSendNotification);
    listening_mode_.setSelectedId(controller.getSmartListeningMode()+1,dontSendNotification);
    refreshAIProfileChoices();
    train_.setButtonText(controller.getSmartListeningMode()==1?"Train Movie AI":"Train Music AI");
    starter_.setButtonText(controller.getSmartListeningMode()==1?"Reset Movie AI":"Reset to starter");
    pin_toggle_.setToggleState(controller.isOutputPinned(), dontSendNotification);
    auto power = FxModel::getModel().getPowerState();
    power_curtain_.syncPower(power);
    preset_list_.setEnabled(power);
    audio_controls_.setEnabled(power);
    equalizer_.setEnabled(power);
    visualizer_.setEnabled(power);
    visualizer_.calcGradient();
    timerCallback();
    repaint();
}
void FxProView::timerCallback()
{
    if(!isShowing()) return;
    power_curtain_.syncPower(FxModel::getModel().getPowerState());
    // Preserve the visible interface in standby while skipping all hidden
    // 60 Hz telemetry, graph updates, AI hand/flower animations and polling.
    // The curtain's own short timer handles its iris animation independently.
    if(!FxModel::getModel().getPowerState()) {
        if(live_status_.getText()!="Processing off")
            live_status_.setText("Processing off",dontSendNotification);
        return;
    }
    const auto now=Time::getMillisecondCounterHiRes();
    if(transition_image_.isValid()) {
        if(FxTheme::isReducedMotion() ||
           now-transition_start_>=CynUiMotion::pageDurationMs) transition_image_=Image();
        repaint();
    }
    if (theme_from_image_.isValid())
    {
        if (FxTheme::isReducedMotion() ||
            now-theme_transition_start_>=430.0)
        {
            theme_from_image_=Image();
            theme_to_image_=Image();
        }
        repaint();
    }
    // A separate 100ms UI telemetry sample keeps audio-responsive glow smooth
    // without forcing the other status controls to refresh at 10Hz.
    const bool neuralCore=FxTheme::getThemeMode()==FxThemeMode::NeuralCore;
    const bool wantGraph=page_==1;
    if(neural_activity_.isVisible()!=wantGraph) {
        neural_activity_.setVisible(wantGraph);
        neural_activity_.setAnimationEnabled(wantGraph);
    }
    // All themes sample the existing personal audio telemetry, never a new
    // recorder or a DSP-thread callback. YAMNet is requested only by Neural Core.
    if(now-last_neural_update_>=100) {
        last_neural_update_=now;
        auto& brainController=FxController::getInstance();
        NeuralActivitySnapshot snapshot;
        const bool hasAudio=brainController.getLiveNeuralActivity(snapshot);
        const float level=hasAudio
            ? jlimit(0.0f,1.0f,std::sqrt(jmax(0.0f,snapshot.rms))*2.2f)
            : 0.0f;
        theme_audio_level_+=(level-theme_audio_level_)*
            (FxTheme::isReducedMotion()?1.0f:0.33f);
        if(wantGraph) {
            neural_activity_.setTrainingActive(brainController.isSmartTraining());
            neural_activity_.setActivity(hasAudio?&snapshot:nullptr,
                                         brainController.getSmartTrainingProgress());
        }
        if(neuralCore) {
            CynAudioScene scene;
            neural_core_live_=brainController.getLiveYamNetScene(scene) && hasAudio;
            neural_core_scene_=neural_core_live_?scene:CynAudioScene {};
        }
    }
    // One bounded, UI-thread repaint budget shared by every animated palette.
    // Reduce motion leaves still artwork and drops the UI refresh to 2Hz.
    const double interval=FxTheme::isReducedMotion()?500.0:42.0;
    if(now-last_theme_repaint_>=interval) {
        last_theme_repaint_=now;
        repaint();
    }
    // Display the *actual applied* effect/EQ coefficients, not Smart
    // Listening's preset baseline. No synthetic wobble, pulse, or DSP writes.
    if (page_ == 0 && now - last_ai_controls_update_ >=
            (FxTheme::isReducedMotion() ? 500.0 : 33.0))
    {
        const float elapsedMs = last_ai_controls_update_ > 0.0
            ? static_cast<float>(now - last_ai_controls_update_) : 33.0f;
        last_ai_controls_update_ = now;
        const bool animate = FxController::getInstance().isSmartListening()
            && !FxTheme::isReducedMotion();
        audio_controls_.syncAppliedDsp(elapsedMs, animate);
        equalizer_.syncAppliedDsp(elapsedMs, animate);
        updateDjTouch(now);
    }
    if(now-last_status_update_<500) return;
    last_status_update_=now;
    if(auto* dj=FxController::getInstance().dj()) {
        dj_status_.setText(dj->status(),dontSendNotification);
        dj_toggle_.setToggleState(dj->enabled(),dontSendNotification);
        if(!dj_intensity_.isMouseButtonDown()) dj_intensity_.setValue(dj->intensity()*100,dontSendNotification);
        if(!dj_spacing_.isMouseButtonDown()) dj_spacing_.setValue(dj->spacing(),dontSendNotification);
        dj_stereo_.setToggleState(dj->stereo(),dontSendNotification);
        dj_status_.setColour(Label::textColourId,Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        dj_recent_.setColour(Label::textColourId,Colour(FXCOLOR(HintText)).withAlpha(1.0f));
        dj_toggle_.setColour(ToggleButton::textColourId,Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        dj_stereo_.setColour(ToggleButton::textColourId,Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        dj_toggle_.setEnabled(dj->installed()&&!dj->busy());
        dj_download_.setVisible(page_==3&&!dj->installed()&&!dj->busy());
        dj_remove_.setVisible(page_==3&&dj->installed()&&!dj->busy());
        dj_cancel_.setVisible(page_==3&&dj->busy());
        dj_progress_.setVisible(page_==3&&dj->busy());dj_progress_value_=dj->progress();
        dj_recent_.setText(dj->lastEffect()+"  /  "+String((int)dj->effects())+" remixes this session",dontSendNotification);
    }
    auto& controller = FxController::getInstance();
    neural_activity_.setTrainingActive(controller.isSmartTraining());
    learning_timeline_.updateFrames(controller.getRecentAudio());
    learning_status_.setText(controller.getLearningStatus(),dontSendNotification);
    const bool selected=learning_timeline_.selection().size()>=5;
    more_.setEnabled(selected && !controller.isSmartTraining()); less_.setEnabled(selected && !controller.isSmartTraining());
    listening_mode_.setEnabled(!controller.isSmartTraining());
    ai_profile_.setEnabled(!controller.isSmartTraining());
    if(page_==1 && ++ai_profile_refresh_ticks_>=20) {
        ai_profile_refresh_ticks_=0;
        refreshAIProfileChoices();
    }
    train_.setEnabled(!controller.isSmartTraining()); starter_.setEnabled(!controller.isSmartTraining());
    export_ai_.setEnabled(!controller.isSmartTraining()); import_ai_.setEnabled(!controller.isSmartTraining());
    smart_toggle_.setToggleState(controller.isSmartListening(), dontSendNotification);
    pin_toggle_.setToggleState(controller.isOutputPinned(), dontSendNotification);
    smart_status_.setText(page_ == 0 ? controller.getAppliedMixSummary() : controller.getSmartSummary(),
                          dontSendNotification);
    if (!sensitivity_.isMouseButtonDown())
        sensitivity_.setValue(controller.getSmartSensitivity(), dontSendNotification);
    const bool cyclingAllowed=controller.getSmartListeningMode()==0;
    preset_cycling_.setEnabled(cyclingAllowed);
    preset_cycling_.setToggleState(controller.isSmartPresetCycling(),dontSendNotification);
    preset_cycling_status_.setText(
        controller.isSmartPresetCycling() && cyclingAllowed
            ? controller.getSmartCyclingPreset() : "",dontSendNotification);
    headroom_cap_.setEnabled(cyclingAllowed);
    if (!headroom_cap_.isMouseButtonDown())
        headroom_cap_.setValue(controller.getSmartHeadroomCap(),dontSendNotification);
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    smart_status_.setFont(theme.getNormalFont().withHeight(page_ == 1 ? 11.5f : 13.0f));
    smart_status_.setColour(Label::textColourId, Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    const bool power = FxModel::getModel().getPowerState();
    const auto status=controller.isSmartListening()
        ? String("AI ACTIVE  •  ")+controller.getActiveAIProfileId().toUpperCase()
        : String("AI STANDBY  •  OPEN CONTROLS");
    if(ai_status_card_.getButtonText()!=status)
        ai_status_card_.setButtonText(status);
    live_status_.setFont(theme.getNormalFont().withHeight(12));
    live_status_.setColour(Label::textColourId, Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    live_status_.setText(!power ? "Processing off" : !controller.isPlaybackDeviceAvailable()
        ? "Output unavailable" : controller.isAudioProcessing() ? "Processing audio" : "Ready to listen", dontSendNotification);
}
void FxProView::resized()
{
    // The stable shell is based on a 1320 x 700 design canvas. Keep all
    // functional JUCE widgets, not an image of controls.
    power_curtain_.setBounds(getLocalBounds());
    const int left=245, right=1301, usable=right-left;
    const int navX=27,navW=186;
    sound_nav_.setBounds(navX,128,navW,42);
    smart_nav_.setBounds(navX,177,navW,42);
    devices_nav_.setBounds(navX,226,navW,42);
    ai_hub_nav_.setBounds(navX,275,navW,42);
    misc_nav_.setBounds(navX,324,navW,42);
    studio_nav_.setBounds(navX,373,navW,42);
    settings_.setBounds(navX,615,navW,42);
    // Header cards contain live, operable preset/device selectors.
    preset_list_.setBounds(left+14,118,315,37);
    endpoint_list_.setBounds(left+350,118,341,37);
    ai_status_card_.setBounds(963,118,324,37);
    live_status_.setBounds(1023,40,265,24);
    overview_tab_.setBounds(left+12,172,126,34);
    eq_focus_tab_.setBounds(left+146,172,126,34);
    effects_focus_tab_.setBounds(left+280,172,163,34);
    for(auto* b:std::initializer_list<TextButton*> {
        &overview_tab_, &eq_focus_tab_, &effects_focus_tab_}) {
        b->getProperties().set("ember_navigation",true);
        b->setColour(TextButton::buttonColourId,Colours::transparentBlack);
        b->setColour(TextButton::textColourOffId,Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        b->setColour(TextButton::textColourOnId,Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    }
    const bool studio=page_==0;
    if(studio) {
        const int eqX=left+12,eqW=731;
        if(sound_focus_==0) {
            equalizer_.setBounds(eqX,250,eqW,230);
            visualizer_.setBounds(1002,250,282,230);
            audio_controls_.setBounds(eqX,510,eqW,161);
        } else if(sound_focus_==1) {
            equalizer_.setBounds(eqX,251,1036,417);
            visualizer_.setBounds(1002,251,282,100);
            audio_controls_.setBounds(eqX,507,731,161);
        } else {
            audio_controls_.setBounds(eqX,272,1036,331);
            equalizer_.setBounds(eqX,251,731,230);
            visualizer_.setBounds(1002,250,282,230);
        }
        equalizer_.setVisible(sound_focus_!=2);
        visualizer_.setVisible(sound_focus_==0);
        audio_controls_.setVisible(sound_focus_!=1);
    }
    // Smart Listening gets a wide live training timeline and its own neural
    // inspector, while preserving all AI mode, training and export controls.
    neural_activity_.setBounds(993,246,291,194);
    learning_timeline_.setBounds(left+13,246,722,194);
    listening_mode_.setBounds(left+22,490,216,32);
    ai_profile_.setBounds(left+253,490,397,32);
    learning_effect_.setBounds(left+22,538,179,35);
    more_.setBounds(left+213,538,103,35);
    less_.setBounds(left+327,538,103,35);
    train_.setBounds(left+443,538,165,35);
    resume_.setBounds(left+619,538,165,35);
    sensitivity_.setBounds(left+150,591,265,25);
    headroom_cap_.setBounds(left+542,591,259,25);
    preset_cycling_.setBounds(left+20,618,208,24);
    preset_cycling_status_.setBounds(left+234,618,210,24);
    learning_status_.setBounds(left+20,644,393,31);
    export_ai_.setBounds(left+456,643,92,32);
    import_ai_.setBounds(left+556,643,92,32);
    browse_ai_.setBounds(left+656,643,103,32);
    starter_.setBounds(left+766,643,134,32);

    // Device selection uses the same header device selector.
    pin_toggle_.setBounds(left+27,295,404,34);
    refresh_.setBounds(left+27,390,191,37);
    smart_toggle_.setBounds(left+21, page_==1 ? 449 : page_==2 ? 495 : 661,217,26);
    smart_status_.setBounds(left+254,page_==1 ? 449 : page_==2 ? 495 : 663, 772,29);

    dj_download_.setBounds(left+24,281,205,39);
    dj_remove_.setBounds(left+24,281,205,39);
    dj_cancel_.setBounds(left+24,281,205,39);
    dj_progress_.setBounds(left+250,292,750,19);
    dj_toggle_.setBounds(left+23,346,260,34);
    dj_status_.setBounds(left+24,391,990,31);
    dj_intensity_.setBounds(left+209,490,540,31);
    dj_spacing_.setBounds(left+209,542,540,31);
    dj_stereo_.setBounds(left+24,590,480,27);
    dj_recent_.setBounds(left+24,630,920,28);
    power_curtain_.toFront(false);
}
void FxProView::paint(Graphics& g)
{
    using namespace juce;
    // Horizon x Depth — stable design candidate. This paints a real native
    // JUCE shell around working DSP/AI widgets, not a screenshot or webview.
    // The only expensive drawing is bounded to existing scene/video widgets.
    auto& theme=dynamic_cast<FxTheme&>(getLookAndFeel());
    auto colour=[](FxColor c) { return Colour(FxTheme::getColor(c)).withAlpha(1.0f); };
    const Colour base(0xff03070d), panel(0xff091321),panel2(0xff0b1728);
    const Colour border(0xff20354b), cyan(0xff45bffb);
    const Colour white(0xffedf5ff),muted(0xff8097b0);
    const bool reduced=FxTheme::isReducedMotion();
    const float playLevel=jlimit(0.0f,1.0f,theme_audio_level_);
    g.fillAll(base);

    // One atmospheric mesh, kept underneath everything and anchored to
    // actual audio telemetry. True black still dominates the interface.
    {
        Graphics::ScopedSaveState scene(g);
        g.reduceClipRegion(Rectangle<int>(229,0,getWidth()-229,getHeight()));
        CynThemeAtmosphere::paint(g,FxThemeMode::DarkMesh,
            reduced ? 0.0f : (float)(Time::getMillisecondCounterHiRes()*.001),
            playLevel,Rectangle<float>(229.0f,0.0f,getWidth()-229.0f,205.0f));
        ColourGradient veil(Colour(0xff03070d).withAlpha(.40f),230.0f,10.0f,
                            base.withAlpha(.96f),740.0f,208.0f,false);
        g.setGradientFill(veil); g.fillRect(229,0,getWidth()-229,220);
    }

    // Solid continuous navigation rail, no second row of icon-only buttons.
    ColourGradient side(Colour(0xff07121f),0.0f,0.0f,
                         Colour(0xff03080f),228.0f,700.0f,false);
    g.setGradientFill(side);g.fillRect(0,0,228,getHeight());
    g.setColour(border.withAlpha(.68f));
    g.drawLine(228.0f,0.0f,228.0f,(float)getHeight(),.8f);
    g.setColour(white);g.setFont(theme.getNormalFont().withHeight(29.0f).boldened());
    g.drawText("CYN",28,22,128,38,Justification::centredLeft);
    g.setColour(cyan);g.fillEllipse(109.0f,50.0f,5.0f,5.0f);
    g.setFont(theme.getNormalFont().withHeight(10.5f));
    g.setColour(muted);g.drawText("PERSONAL AUDIO  /  STABLE UI",30,67,188,18,
                                 Justification::centredLeft);
    g.setColour(border.withAlpha(.7f));g.drawLine(25.0f,105.0f,203.0f,105.0f,.8f);

    const float navY=128.0f+49.0f*(float)page_;
    g.setColour(Colour(0xff102b45).withAlpha(.94f));
    g.fillRoundedRectangle(22.0f,navY,196.0f,42.0f,12.0f);
    g.setColour(cyan.withAlpha(.78f));
    g.drawRoundedRectangle(22.5f,navY+.5f,195.0f,41.0f,11.0f,1.05f);
    g.fillRoundedRectangle(22.0f,navY+11.0f,3.2f,20.0f,1.5f);
    // Small glyphs are purely decorative; the real labelled TextButtons
    // provide keyboard focus, pointer targets, and accessibility names.
    for(int i=0;i<6;++i) {
        const float y=149.0f+49.0f*i;
        const Colour col=i==page_?cyan:muted;
        g.setColour(col.withAlpha(i==page_?.85f:.50f));
        if(i==0) {
            for(int k=0;k<5;++k)
                g.drawLine(37.0f+k*4.0f,y-(2+(k*5)%11)*.5f,
                           37.0f+k*4.0f,y+(2+(k*5)%11)*.5f,1.2f);
        } else {
            g.fillEllipse(40.0f,y-3.0f,5.0f,5.0f);
            g.drawEllipse(36.5f,y-6.5f,12.0f,12.0f,.75f);
        }
    }
    g.setColour(border);g.drawLine(27.0f,590.0f,210.0f,590.0f,.8f);
    g.setColour(Colour(0xff091829));g.fillRoundedRectangle(21.0f,524.0f,195.0f,56.0f,13.0f);
    g.setColour(border);g.drawRoundedRectangle(21.5f,524.5f,194.0f,55.0f,12.0f,1.0f);
    const auto model=FxController::getInstance().getActiveAIProfileId();
    const String modelText=model=="apex6m" ? "APEX 6M" :
            model=="yamnet" ? "YAMNet" : model.isNotEmpty() ? model.toUpperCase() : "No model";
    const bool aiOn=FxController::getInstance().isSmartListening();
    g.setColour(aiOn?Colour(0xff49dcb3):muted);
    g.fillEllipse(35.0f,544.0f,8.0f,8.0f);
    g.setColour(white);g.setFont(theme.getNormalFont().withHeight(13.0f).boldened());
    g.drawText(modelText,54,533,148,20,Justification::centredLeft,true);
    g.setColour(muted);g.setFont(theme.getNormalFont().withHeight(10.0f));
    g.drawText(aiOn?"AI enabled":"AI standby",54,555,140,14,Justification::centredLeft);
    g.drawText("CYN  "+JUCEApplication::getInstance()->getApplicationVersion(),
               25,669,178,16,Justification::centredLeft);

    auto txt=[&](String value,float x,float y,float width,float height,
                 float size,bool mutedText=false,bool bold=false) {
        g.setColour(mutedText?muted:white);
        auto font=theme.getNormalFont().withHeight(size);
        if(bold)font=font.boldened();
        g.setFont(font);
        g.drawText(value,roundToInt(x),roundToInt(y),roundToInt(width),roundToInt(height),
                   Justification::centredLeft,true);
    };
    auto card=[&](float x,float y,float width,float height,float r=15.0f) {
        ColourGradient finish(panel2,x,y,panel,x+width,y+height,false);
        finish.addColour(.55,panel);
        g.setGradientFill(finish);
        g.fillRoundedRectangle(x,y,width,height,r);
        g.setColour(border.withAlpha(.88f));
        g.drawRoundedRectangle(x+.5f,y+.5f,width-1.0f,height-1.0f,r-.3f,1.0f);
        g.setColour(Colours::white.withAlpha(.032f));
        g.drawLine(x+13.0f,y+1.0f,x+width-13.0f,y+1.0f,.85f);
    };
    // Clear hierarchy: a large page title, meaningful status, then real
    // selectors and page-specific content. No fictitious audio readouts.
    const char* pageNames[]={"Sound Studio","Smart Listening","Output Devices","Misc / AI DJ"};
    const char* subtitles[]={
        "Fine-tune your sound with real-time processing.",
        "Neural listening, personal learning, and live AI controls.",
        "Your output hardware, connected devices and device priority.",
        "Optional musical effects and stereo creative controls."
    };
    txt("CYN  /  "+String(pageNames[page_]).toUpperCase(),246,12,610,20,10.8f,true,true);
    txt(pageNames[page_],246,34,650,45,29.0f,false,true);
    txt(subtitles[page_],248,78,750,24,13.0f,true);
    txt(FxModel::getModel().getPowerState()?"●  AUDIO ON":"○  PROCESSING OFF",
        1115,19,174,21,11.0f,false,true);

    card(247,108,333,58,12.0f);
    card(587,108,361,58,12.0f);
    card(956,108,344,58,12.0f);
    txt("ACTIVE PROFILE",259,110,260,16,10,true,true);
    txt("OUTPUT DEVICE",600,110,260,16,10,true,true);
    txt("AI ENHANCEMENT",970,110,230,16,10,true,true);

    // Clickable tabs over a restrained, visually unified toolbar.
    card(247,170,1053,44,11.0f);
    if(page_==0) {
        const float selX=sound_focus_==0 ? 258.0f : sound_focus_==1 ? 392.0f : 526.0f;
        const float selW=sound_focus_==2 ? 164.0f : 126.0f;
        g.setColour(Colour(0xff103455));
        g.fillRoundedRectangle(selX,172.0f,selW,34.0f,11.0f);
        g.setColour(cyan.withAlpha(.75f));
        g.drawRoundedRectangle(selX+.5f,172.5f,selW-1.0f,33.0f,10.5f,.9f);
        txt("31 / 35 / 39 BAND EQ  •  LIVE DSP",966,180,315,23,11.3f,true);
        if(sound_focus_==0) {
            card(247,226,743,258);
            card(996,226,304,258);
            card(247,494,743,189);
            card(996,494,304,189);
            txt("31-BAND EQUALIZER",263,230,490,24,13.4f,false,true);
            txt("LIVE SPECTRUM",1013,230,250,23,13.4f,false,true);
            txt("SOUND ENHANCEMENTS",263,497,420,22,13.4f,false,true);
            txt("LISTENING STATUS",1013,497,250,22,13.4f,false,true);
            txt("Actual DSP controls",263,460,560,17,10.3f,true);
            txt("Applied AI model",1013,532,230,19,10.2f,true);
            txt(modelText,1013,552,250,24,18.0f,false,true);
            txt(aiOn?"Real-time adaptive processing":"Manual audio processing",
                1013,581,265,20,11.0f,true);
            txt("Select a profile above to change the sound.",1013,628,272,34,11.0f,true);
        } else if(sound_focus_==1) {
            card(247,226,1053,457);
            txt("EQUALIZER  /  PRECISION VIEW",265,233,650,24,14,false,true);
            txt("Drag the actual frequency controls to shape audio.",265,627,730,21,11,true);
        } else {
            card(247,226,1053,457);
            txt("SOUND ENHANCEMENTS  /  STUDIO VIEW",265,233,800,25,14,false,true);
            txt("Tune clarity, ambience, surround, dynamics and bass.",265,613,900,22,11,true);
        }
    } else if(page_==1) {
        txt("REAL AI TELEMETRY  •  PERSONAL LEARNING",267,180,800,25,12,true,true);
        card(247,225,743,231);
        card(996,225,304,231);
        card(247,468,1053,220);
        txt("LEARNING TIMELINE",265,229,490,20,13.4f,false,true);
        txt("LIVE NEURAL ACTIVITY",1011,229,274,20,13.4f,false,true);
        txt("LISTENING CONTROLS",264,470,470,18,13.2f,false,true);
        txt("SENSITIVITY",265,583,128,19,10.5f,true,true);
        txt("HEADROOM CAP",663,583,150,19,10.5f,true,true);
    } else if(page_==2) {
        txt("DEVICE ROUTING  •  OUTPUT PRIORITY",267,180,790,25,12,true,true);
        card(247,225,1053,219);
        card(247,454,1053,234);
        txt("OUTPUT DEVICE MANAGEMENT",266,231,780,28,17,false,true);
        txt("Keep your preferred output when available.",266,258,900,25,12.5f,true);
        txt("DEVICE FALLBACK",266,458,500,27,15,false,true);
        txt("Automatic recovery when a selected device disconnects.",266,550,920,28,13,true);
        txt("Device-specific presets and priority are managed in Settings.",
            266,587,920,24,12.3f,true);
    } else {
        txt("AI DJ  •  OPTIONAL CREATIVE EFFECTS",267,180,790,25,12,true,true);
        card(247,225,1053,216);
        card(247,453,1053,235);
        txt("AI DJ  /  LIVE REMIX",266,233,720,30,18,false,true);
        txt("Enable creative processing only when you want it.",266,328,920,22,12,true);
        txt("EFFECT BEHAVIOUR",266,458,730,24,15,false,true);
        txt("Intensity",266,492,120,23,12.5f,true);
        txt("Minimum spacing",266,545,155,23,12.5f,true);
    }
    // Footer is a real engine-status report, not a fabricated media player.
    g.setColour(border.withAlpha(.50f));
    g.drawLine(245.0f,693.0f,1299.0f,693.0f,.8f);
}
void FxProView::refreshAIProfileChoices()
{
    const auto models=FxController::getInstance().getAvailableAIProfiles();
    StringArray ids;
    for(const auto& entry:models) ids.add(entry.id);
    if(ids!=ai_profile_ids_) {
        ai_profile_.clear(dontSendNotification);
        ai_profile_ids_=ids;
        for(int i=0;i<static_cast<int>(models.size());++i)
            ai_profile_.addItem(models[(size_t)i].name,i+1);
    }
    const auto selected=FxController::getInstance().getActiveAIProfileId();
    neural_activity_.setProfileId(selected);
    const int index=ai_profile_ids_.indexOf(selected);
    if(index>=0) ai_profile_.setSelectedId(index+1,dontSendNotification);
}

void FxProView::modelChanged(FxModel::Event event)
{
    FxView::modelChanged(event);
    if (event == FxModel::Event::PresetSelected || event == FxModel::Event::Other) update();
    else { timerCallback(); repaint(); }
}

// Two independent visual DJ performers: Effects on the left, EQ on the right.
// Each follows REAL applied DSP differences and its own preset, not random
// animations. Up to four individually aimed flowers per panel, with no DSP writes.
void FxProView::updateDjTouch(double now)
{
    auto& controller=FxController::getInstance();
    const int previousStyle=controller.getSettings().getInt("cyn_dj_touch_style",0);
    const std::array<int,2> chosen {{
        juce::jlimit(0,5,controller.getSettings().getInt(
            "cyn_dj_touch_style_effects",previousStyle)),
        juce::jlimit(0,5,controller.getSettings().getInt(
            "cyn_dj_touch_style_eq",previousStyle))
    }};
    dj_touch_flower_count_=juce::jlimit(2,4,
        controller.getSettings().getInt("cyn_dj_touch_flower_count",4));
    for(int panel=0;panel<2;++panel) {
        if(chosen[(size_t)panel]!=dj_touch_styles_[(size_t)panel]) {
            dj_touch_styles_[(size_t)panel]=chosen[(size_t)panel];
            dj_touch_seeded_[(size_t)panel]=false;
            for(int slot=0;slot<4;++slot)
                dj_touch_agents_[(size_t)(panel*4+slot)]={};
        }
    }
    if(chosen[0]==CynDjTouch::CustomPng ||
       chosen[1]==CynDjTouch::CustomPng) {
        const auto asset=CynDjTouch::customImageFile();
        const auto stamp=asset.getLastModificationTime();
        if(dj_touch_image_stamp_!=stamp) {
            dj_touch_image_stamp_=stamp;
            dj_touch_image_=CynDjTouch::readCustomImage();
        }
    }
    if(FxTheme::isReducedMotion() || page_!=0 ||
       !controller.isSmartListening() || !controller.isAudioProcessing()) {
        dj_touch_seeded_={{false,false}};
        for(auto& agent:dj_touch_agents_) agent.opacity=0.0f;
        return;
    }
    struct Candidate {
        float score=0.0f;
        juce::Point<float> position {};
        int index=-1;
    };
    std::array<std::array<Candidate,4>,2> ranked {};
    const auto offer=[&](int panel,int index,float score,
                         juce::Point<float> position) {
        if(score<=0.0017f) return;
        auto& choices=ranked[(size_t)panel];
        // An agent never duplicates another flower's EQ band/knob.
        int insert=0;
        while(insert<4 && score<=choices[(size_t)insert].score) ++insert;
        if(insert==4) return;
        for(int i=3;i>insert;--i)
            choices[(size_t)i]=choices[(size_t)(i-1)];
        choices[(size_t)insert]={score,position,index};
    };
    for(int i=0;i<5;++i) {
        const auto effect=static_cast<FxEffects::EffectType>(i);
        const float value=controller.getAppliedEffectValue(effect)*10.0f;
        const float preset=controller.getEffectValue(effect)*10.0f;
        if(!std::isfinite(value) || !std::isfinite(preset))continue;
        const float movement=std::abs(value-dj_touch_last_effect_[(size_t)i]);
        dj_touch_last_effect_[(size_t)i]=value;
        if(!dj_touch_seeded_[0] || !audio_controls_.isEffectsShown())continue;
        const float score=(std::min)(movement,1.0f)*
                          (std::min)(std::abs(value-preset),2.0f);
        if(movement>0.022f && std::abs(value-preset)>0.08f)
            offer(0,i,score,{audio_controls_.getX()+22.0f+value*12.5f,
                              audio_controls_.getY()+52.0f+i*43.0f});
    }
    const int n=juce::jmin(39,controller.getNumEqBands());
    for(int i=0;i<n;++i) {
        const float value=controller.getAppliedEqBandBoostCut(i);
        const float preset=controller.getEqBandBoostCut(i);
        if(!std::isfinite(value) || !std::isfinite(preset))continue;
        const float movement=std::abs(value-dj_touch_last_eq_[(size_t)i]);
        dj_touch_last_eq_[(size_t)i]=value;
        if(!dj_touch_seeded_[1])continue;
        const float score=(std::min)(movement,0.6f)*
                          (std::min)(std::abs(value-preset),2.0f);
        if(movement>0.028f && std::abs(value-preset)>0.09f) {
            const float stride=(float)(equalizer_.getWidth()-22)/(float)(std::max)(1,n);
            offer(1,i,score,{
                equalizer_.getX()+11.0f+stride*(i+0.5f),
                equalizer_.getY()+40.0f+
                juce::jlimit(0.0f,1.0f,(12.0f-value)/24.0f)*166.0f});
        }
    }
    dj_touch_seeded_={{true,true}};
    for(int panel=0;panel<2;++panel) {
        const int style=chosen[(size_t)panel];
        const bool shown=(panel==1 || audio_controls_.isEffectsShown());
        const int agents=style==CynDjTouch::Flowers ? dj_touch_flower_count_:1;
        auto& choices=ranked[(size_t)panel];
        for(int slot=0;slot<4;++slot) {
            auto& agent=dj_touch_agents_[(size_t)(panel*4+slot)];
            // Only one hand or one cursor per panel. Flowers can occupy
            // several distinct ACTIVE knobs/bands simultaneously.
            if(style!=CynDjTouch::None && shown && slot<agents &&
               choices[(size_t)slot].score>0.0017f) {
                // Avoid overlapping flowers on adjacent very narrow EQ bands:
                // retain the highest-ranked distinct targets for this frame.
                const auto point=choices[(size_t)slot].position;
                if(agent.opacity<=0.004f) agent.pos=point;
                agent.target=point;
                agent.lastActivity=now;
            }
            const float goal=style!=CynDjTouch::None && shown &&
                slot<agents && now-agent.lastActivity<=900.0 &&
                agent.lastActivity>0.0 ? 0.80f : 0.0f;
            agent.pos+=(agent.target-agent.pos)*0.20f;
            agent.opacity+=(goal-agent.opacity)*0.17f;
        }
    }
}

void FxProView::paintOverChildren(Graphics& g)
{
    // Keep the standby portal above DJ hands, flowers and theme screenshots.
    if(power_curtain_.isVisible()) return;
    if(theme_from_image_.isValid() && theme_to_image_.isValid() &&
       !FxTheme::isReducedMotion())
    {
        Graphics::ScopedSaveState saved(g);
        g.reduceClipRegion(getLocalBounds());
        const float raw=jlimit(0.0f,1.0f,
            (float)((Time::getMillisecondCounterHiRes()-theme_transition_start_)/420.0));
        const float progress=raw*raw*(3.0f-2.0f*raw);
        const int offset=roundToInt(getWidth()*progress);
        g.drawImageAt(theme_from_image_,-offset,0);
        g.drawImageAt(theme_to_image_,getWidth()-offset,0);
        return;
    }
    if(transition_image_.isValid() && !FxTheme::isReducedMotion()) {
        const auto content=juce::Rectangle<int>(240,0,jmax(0,getWidth()-240),getHeight());
        Graphics::ScopedSaveState clipped(g);
        g.reduceClipRegion(content); // No old-page pixels over the active sidebar.
        const float t=CynUiMotion::progress(
            Time::getMillisecondCounterHiRes()-transition_start_,
            CynUiMotion::pageDurationMs);
        g.setOpacity(CynUiMotion::outgoingOpacity(t));
        g.drawImageAt(transition_image_,
            content.getX()+roundToInt(CynUiMotion::outgoingX(t,transition_direction_)),
            content.getY());
    }
    if(page_==0 && !FxTheme::isReducedMotion()) {
        const auto time=(float)(Time::getMillisecondCounterHiRes()*0.001);
        for(int panel=0;panel<2;++panel) {
            const int style=dj_touch_styles_[(size_t)panel];
            if(style==CynDjTouch::None)continue;
            Graphics::ScopedSaveState panelClip(g);
            // Neither hand/cursor nor flower may wander over another panel.
            const auto area=(panel==0?audio_controls_.getBounds():
                             equalizer_.getBounds()).expanded(25,43);
            g.reduceClipRegion(area);
            const int count=style==CynDjTouch::Flowers ? dj_touch_flower_count_:1;
            for(int slot=0;slot<count;++slot) {
                const auto& agent=dj_touch_agents_[(size_t)(panel*4+slot)];
                if(agent.opacity<=0.004f)continue;
                CynDjTouch::paint(g,style,agent.pos,agent.opacity,
                    dj_touch_image_,time+slot*0.45f,panel==1);
            }
        }
    }
}
void FxProView::lookAndFeelChanged()
{
    transition_image_=Image();
    if (FxTheme::isReducedMotion())
    {
        theme_from_image_=Image();
        theme_to_image_=Image();
    }
    if(FxTheme::isReducedMotion()) startTimer(500); else startTimerHz(60);
    selectPage(page_);
    neural_activity_.setAnimationEnabled(page_==1);
    learning_status_.setColour(Label::textColourId,Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    repaint();
}
