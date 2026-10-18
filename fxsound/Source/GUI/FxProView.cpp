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
        &audio_controls_, &equalizer_, &visualizer_, &embedded_ai_hub_, &sound_nav_, &smart_nav_,
        &devices_nav_, &ai_hub_nav_, &studio_nav_, &ai_status_card_, &overview_tab_, &eq_focus_tab_, &effects_focus_tab_, &home_, &smart_rail_, &devices_rail_, &settings_,
        &refresh_, &smart_toggle_, &pin_toggle_, &live_status_, &smart_status_, &settings_rail_, &sensitivity_, &headroom_cap_, &preset_cycling_, &preset_cycling_status_, &learning_timeline_, &neural_activity_, &learning_effect_, &listening_mode_, &ai_profile_, &more_, &less_, &train_, &starter_, &resume_, &learning_status_, &export_ai_, &import_ai_, &browse_ai_ })
        addAndMakeVisible(component);
    for(auto* c:std::initializer_list<Component*> {&misc_nav_,&dj_download_,&dj_remove_,&dj_cancel_,&dj_toggle_,&dj_stereo_,&dj_intensity_,&dj_spacing_,&dj_status_,&dj_recent_,&dj_progress_}) addAndMakeVisible(c);
    misc_nav_.onClick=[this]{selectPage(4);};
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
    ai_hub_nav_.onClick = [this] { selectPage(3); };
    ai_hub_nav_.setTooltip("Browse and activate offline/community AI models in the CYN workspace.");
    embedded_ai_hub_.setEmbeddedMode(true);
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
    misc_nav_.setToggleState(page==4,dontSendNotification);
    ai_hub_nav_.setToggleState(page==3,dontSendNotification);
    embedded_ai_hub_.setVisible(page==3);
    for(auto* b:std::initializer_list<TextButton*> {
        &overview_tab_, &eq_focus_tab_, &effects_focus_tab_ })
        b->setVisible(page==0);
    ai_status_card_.setVisible(true);
    for(auto* c:std::initializer_list<Component*> {&dj_download_,&dj_remove_,&dj_cancel_,&dj_toggle_,&dj_stereo_,&dj_intensity_,&dj_spacing_,&dj_status_,&dj_recent_,&dj_progress_}) c->setVisible(page==4);
    smart_toggle_.setVisible(page<=2);smart_status_.setVisible(page<=2);
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
    embedded_ai_hub_.setVisible(page==3);
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
        dj_download_.setVisible(page_==4&&!dj->installed()&&!dj->busy());
        dj_remove_.setVisible(page_==4&&dj->installed()&&!dj->busy());
        dj_cancel_.setVisible(page_==4&&dj->busy());
        dj_progress_.setVisible(page_==4&&dj->busy());dj_progress_value_=dj->progress();
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
        ? String("AI ACTIVE  /  ")+controller.getActiveAIProfileId().toUpperCase()
        : String("AI STANDBY  /  OPEN CONTROLS");
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
    // Match the actual sidebar selection geometry and free the top-left
    // from the duplicate title-bar branding.
    sound_nav_.setBounds(navX,104,navW,42);
    smart_nav_.setBounds(navX,153,navW,42);
    devices_nav_.setBounds(navX,202,navW,42);
    ai_hub_nav_.setBounds(navX,251,navW,42);
    misc_nav_.setBounds(navX,300,navW,42);
    studio_nav_.setBounds(navX,349,navW,42);
    settings_.setBounds(navX,615,navW,42);
    // Header cards contain live, operable preset/device selectors.
    // Labels occupy a dedicated line above the selectors. Never overlap
    // live JUCE ComboBoxes (the old cards positioned both at y=118).
    preset_list_.setBounds(left+14,139,315,36);
    endpoint_list_.setBounds(left+350,139,341,36);
    ai_status_card_.setBounds(968,139,314,36);
    embedded_ai_hub_.setBounds(247,250,1053,435);
    live_status_.setBounds(1023,43,265,23);
    overview_tab_.setBounds(left+12,203,126,34);
    eq_focus_tab_.setBounds(left+146,203,126,34);
    effects_focus_tab_.setBounds(left+280,203,163,34);
    for(auto* b:std::initializer_list<TextButton*> {
        &overview_tab_, &eq_focus_tab_, &effects_focus_tab_}) {
        b->getProperties().set("ember_navigation",true);
        b->setColour(TextButton::buttonColourId,Colours::transparentBlack);
        b->setColour(TextButton::textColourOffId,Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        b->setColour(TextButton::textColourOnId,Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    }
    const bool studio=page_==0;
    if(studio) {
        const int eqX=left+12,eqW=785;
        if(sound_focus_==0) {
            equalizer_.setBounds(eqX,288,eqW,193);
            visualizer_.setBounds(1057,288,227,193);
            audio_controls_.setBounds(eqX,536,eqW,136);
        } else if(sound_focus_==1) {
            equalizer_.setBounds(eqX,286,1036,344);
            visualizer_.setBounds(1002,286,282,100);
            audio_controls_.setBounds(eqX,522,731,152);
        } else {
            audio_controls_.setBounds(eqX,306,1036,296);
            equalizer_.setBounds(eqX,288,eqW,193);
            visualizer_.setBounds(1002,288,282,193);
        }
        equalizer_.setVisible(sound_focus_!=2);
        visualizer_.setVisible(sound_focus_==0);
        audio_controls_.setVisible(sound_focus_!=1);
    }
    // Smart Listening gets a wide live training timeline and its own neural
    // inspector, while preserving all AI mode, training and export controls.
    neural_activity_.setBounds(1008,288,271,164);
    learning_timeline_.setBounds(left+13,288,722,164);
    listening_mode_.setBounds(left+22,510,216,32);
    ai_profile_.setBounds(left+253,510,397,32);
    learning_effect_.setBounds(left+22,548,179,32);
    more_.setBounds(left+213,548,103,32);
    less_.setBounds(left+327,548,103,32);
    train_.setBounds(left+443,548,165,32);
    resume_.setBounds(left+619,548,165,32);
    sensitivity_.setBounds(left+150,603,265,24);
    headroom_cap_.setBounds(left+542,603,259,24);
    preset_cycling_.setBounds(left+20,629,208,24);
    preset_cycling_status_.setBounds(left+234,629,210,24);
    learning_status_.setBounds(left+20,657,393,27);
    export_ai_.setBounds(left+456,657,92,27);
    import_ai_.setBounds(left+556,657,92,27);
    browse_ai_.setBounds(left+656,657,103,27);
    starter_.setBounds(left+766,657,134,27);

    // Device selection uses the same header device selector.
    pin_toggle_.setBounds(left+27,315,404,34);
    refresh_.setBounds(left+27,397,191,37);
    // The real Smart Listening switch must never sit over a DSP slider.
    if(page_==0) {
        smart_toggle_.setBounds(1062,638,212,25);
        smart_status_.setBounds(1060,658,214,25);
    } else if(page_==1) {
        smart_toggle_.setBounds(1032,477,244,26);
        smart_status_.setBounds(1008,627,281,24);
    } else {
        smart_toggle_.setBounds(left+21,498,217,26);
        smart_status_.setBounds(left+254,498,772,26);
    }

    // Two balanced AI DJ cards: actions to the left, truthful engine state
    // to the right; controls aligned underneath with room for labels.
    dj_download_.setBounds(left+24,329,205,38);
    dj_remove_.setBounds(left+24,329,205,38);
    dj_cancel_.setBounds(left+24,329,205,38);
    dj_progress_.setBounds(left+252,338,382,18);
    dj_toggle_.setBounds(left+24,383,260,32);
    dj_status_.setBounds(left+24,422,640,25);
    dj_intensity_.setBounds(left+209,522,565,31);
    dj_spacing_.setBounds(left+209,574,565,31);
    dj_stereo_.setBounds(left+24,623,480,27);
    dj_recent_.setBounds(left+24,655,920,24);
    // Keep real AI Hub export/import/browse controls readable when moving
    // from the old control grid into the new dark workbench.
    ai_hub_nav_.setColour(TextButton::buttonColourId, Colours::transparentBlack);
    ai_hub_nav_.setColour(TextButton::textColourOffId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    browse_ai_.setColour(TextButton::buttonColourId, Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f));
    browse_ai_.setColour(TextButton::textColourOffId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    browse_ai_.setColour(TextButton::textColourOnId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    export_ai_.setColour(TextButton::buttonColourId, Colour(FXCOLOR(PanelBackground)).withAlpha(1.0f));
    import_ai_.setColour(TextButton::buttonColourId, Colour(FXCOLOR(PanelBackground)).withAlpha(1.0f));
    for(auto* button:std::initializer_list<TextButton*> { &export_ai_, &import_ai_ }) {
        button->setColour(TextButton::textColourOffId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        button->setColour(TextButton::textColourOnId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    }
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
    const Colour base(0xff000000), panel(0xff050607),panel2(0xff080a0d);
    const Colour border(0xff18212b), cyan(0xff65b9ff);
    const Colour white(0xffedf5ff),muted(0xff9aadc4);
    const bool reduced=FxTheme::isReducedMotion();
    const float playLevel=jlimit(0.0f,1.0f,theme_audio_level_);
    g.fillAll(base);

    // One atmospheric mesh, kept underneath everything and anchored to
    // actual audio telemetry. True black still dominates the interface.
    {
        Graphics::ScopedSaveState scene(g);
        g.reduceClipRegion(juce::Rectangle<int>(229,0,getWidth()-229,getHeight()));
        CynThemeAtmosphere::paint(g,FxThemeMode::DarkMesh,
            reduced ? 0.0f : (float)(Time::getMillisecondCounterHiRes()*.001),
            playLevel,juce::Rectangle<float>(229.0f,0.0f,getWidth()-229.0f,205.0f));
        ColourGradient veil(Colour(0xff03070d).withAlpha(.40f),230.0f,10.0f,
                            base.withAlpha(.96f),740.0f,208.0f,false);
        g.setGradientFill(veil); g.fillRect(229,0,getWidth()-229,220);
    }

    // Solid continuous navigation rail, no second row of icon-only buttons.
    ColourGradient side(Colour(0xff090c10),0.0f,0.0f,
                         Colour(0xff050608),228.0f,700.0f,false);
    g.setGradientFill(side);g.fillRect(0,0,228,getHeight());
    // A stable straight rail with a restrained, non-filled corner flourish.
    // The prior filled cubic path created an unwanted bulge into the content.
    g.setColour(Colour(0xff172431).withAlpha(.80f));
    g.drawLine(228.0f,0.0f,228.0f,(float)getHeight(),.8f);
    Path cornerAccent;
    cornerAccent.startNewSubPath(228.0f,64.0f);
    cornerAccent.cubicTo(228.0f,75.0f,224.0f,81.0f,216.0f,89.0f);
    g.setColour(cyan.withAlpha(.42f));
    g.strokePath(cornerAccent,PathStrokeType(1.5f));
    // Native title bar already displays the CYN wordmark. The sidebar
    // identifies the navigation instead of repeating the same logo.
    g.setColour(cyan);g.fillRoundedRectangle(26.0f,28.0f,3.0f,24.0f,1.5f);
    g.setFont(theme.getNormalFont().withHeight(16.0f).boldened());
    g.setColour(white);g.drawText("WORKSPACES",39,26,165,27,Justification::centredLeft);
    g.setFont(theme.getNormalFont().withHeight(11.5f));
    g.setColour(muted);g.drawText("AUDIO CONTROL CENTRE",28,60,179,19,
                                 Justification::centredLeft);
    g.setColour(border.withAlpha(.7f));g.drawLine(25.0f,87.0f,203.0f,87.0f,.8f);

    const float navY=104.0f+49.0f*(float)page_;
    g.setColour(Colour(0xff111923).withAlpha(.96f));
    g.fillRoundedRectangle(22.0f,navY,196.0f,42.0f,12.0f);
    g.setColour(cyan.withAlpha(.78f));
    g.drawRoundedRectangle(22.5f,navY+.5f,195.0f,41.0f,11.0f,1.05f);
    g.fillRoundedRectangle(22.0f,navY+11.0f,3.2f,20.0f,1.5f);
    g.setColour(border);g.drawLine(27.0f,590.0f,210.0f,590.0f,.8f);
    g.setColour(panel);g.fillRoundedRectangle(21.0f,524.0f,195.0f,56.0f,13.0f);
    g.setColour(border);g.drawRoundedRectangle(21.5f,524.5f,194.0f,55.0f,12.0f,1.0f);
    g.setColour(muted.withAlpha(.85f));
    g.setFont(theme.getNormalFont().withHeight(9.0f).boldened());
    g.drawText("ACTIVE ENGINE",34,528,155,15,Justification::centredLeft);
    const auto model=FxController::getInstance().getActiveAIProfileId();
    const String modelText=model=="apex6m" ? "APEX 6M" :
            model=="yamnet" ? "YAMNet" : model.isNotEmpty() ? model.toUpperCase() : "No model";
    const bool aiOn=FxController::getInstance().isSmartListening();
    g.setColour(aiOn?Colour(0xff49dcb3):muted);
    g.fillEllipse(35.0f,548.0f,8.0f,8.0f);
    g.setColour(white);g.setFont(theme.getNormalFont().withHeight(13.0f).boldened());
    g.drawText(modelText,54,542,148,21,Justification::centredLeft,true);
    g.setColour(muted);g.setFont(theme.getNormalFont().withHeight(10.0f));
    g.drawText(aiOn?"Processing enabled":"AI standby",54,564,140,13,Justification::centredLeft);
    g.drawText("CYN  v"+JUCEApplication::getInstance()->getApplicationVersion(),
               25,669,178,17,Justification::centredLeft);

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
        g.setColour(border.withAlpha(.46f));
        g.drawRoundedRectangle(x+.5f,y+.5f,width-1.0f,height-1.0f,r-.3f,.8f);
    };
    // Clear hierarchy: a large page title, meaningful status, then real
    // selectors and page-specific content. No fictitious audio readouts.
    const char* pageNames[]={"Sound Studio","Smart Listening","Output Devices","AI Hub","Misc / AI DJ"};
    const char* subtitles[]={
        "Fine-tune your sound with real-time processing.",
        "Neural listening, personal learning, and live AI controls.",
        "Your output hardware, connected devices and device priority.",
        "Explore community and local AI models without leaving CYN.",
        "Optional musical effects and stereo creative controls."
    };
    txt("CYN  /  "+String(pageNames[page_]).toUpperCase(),246,12,610,20,10.8f,true,true);
    txt(pageNames[page_],246,34,650,45,29.0f,false,true);
    txt(subtitles[page_],248,78,750,24,13.0f,true);
    g.setColour(FxModel::getModel().getPowerState()
        ? Colour(0xff4bdcaa) : muted);
    g.fillEllipse(1147.0f,22.0f,7.0f,7.0f);
    txt(FxModel::getModel().getPowerState()?"AUDIO ON":"PROCESSING OFF",
        1160,16,129,21,11.0f,false,true);

    card(247,108,333,78,12.0f);
    card(587,108,361,78,12.0f);
    // AI is a compact status action rather than a third competing panel.
    txt("ACTIVE PROFILE",262,114,260,18,11.5f,true,true);
    txt("OUTPUT DEVICE",603,114,260,18,11.5f,true,true);
    txt("AI ENHANCEMENT",972,114,230,18,11.5f,true,true);

    // Dedicated workspace band sits BELOW the taller selector cards.
    // Tabs sit directly on the workbench; no surrounding button bar.
    if(page_==0) {
        const float selX=sound_focus_==0 ? 258.0f : sound_focus_==1 ? 392.0f : 526.0f;
        const float selW=sound_focus_==2 ? 164.0f : 126.0f;
        g.setColour(Colour(0xff142334));
        g.fillRoundedRectangle(selX,202.0f,selW,36.0f,11.0f);
        g.setColour(cyan.withAlpha(.75f));
        g.drawRoundedRectangle(selX+.5f,202.5f,selW-1.0f,35.0f,10.5f,.9f);
        txt("31 / 35 / 39 BANDS",1110,208,180,23,11.6f,true);
        if(sound_focus_==0) {
            card(247,253,1053,235);
            card(247,500,1053,188);
            txt("31-BAND EQUALIZER",263,258,490,23,14.0f,false,true);
            txt("SPECTRUM",1066,258,210,23,11.5f,true,true);
            txt("SOUND ENHANCEMENTS",263,505,420,23,14.0f,false,true);
            txt("AI STATUS",1066,505,210,23,11.5f,true,true);
            
            txt(modelText,1066,551,220,25,17.0f,false,true);
            txt(aiOn?"Adaptive processing":"Manual processing",
                1066,583,218,20,11.0f,true);
            
        } else if(sound_focus_==1) {
            card(247,253,1053,435);
            txt("EQUALIZER  /  PRECISION VIEW",265,262,650,25,15,false,true);
            txt("Drag the frequency controls to shape your sound.",265,649,730,21,12,true);
        } else {
            card(247,253,1053,435);
            txt("SOUND ENHANCEMENTS  /  STUDIO VIEW",265,262,800,25,15,false,true);
            txt("Tune clarity, ambience, surround, dynamics and bass.",265,644,900,22,12,true);
        }
    } else if(page_==1) {
        txt("REAL AI TELEMETRY  /  PERSONAL LEARNING",267,205,800,25,12,true,true);
        card(247,253,743,210);
        card(996,253,304,210);
        card(247,472,1053,216);
        txt("LEARNING TIMELINE",265,258,490,23,14,false,true);
        txt("LIVE NEURAL ACTIVITY",1011,258,274,23,14,false,true);
        txt("LISTENING CONTROLS",264,476,470,22,14,false,true);
        txt("SENSITIVITY",265,603,128,20,11.5f,true,true);
        txt("HEADROOM CAP",663,603,150,20,11.5f,true,true);
    } else if(page_==2) {
        txt("DEVICE ROUTING  /  OUTPUT PRIORITY",267,205,790,25,12,true,true);
        card(247,253,1053,200);
        card(247,465,1053,223);
        txt("OUTPUT DEVICE MANAGEMENT",266,259,780,27,17,false,true);
        txt("Keep your preferred output when available.",266,284,570,23,12.5f,true);
        // Device facts come from the active selector and routing controller;
        // never show invented connection counts or hardware capabilities.
        g.setColour(border.withAlpha(.67f));
        g.drawLine(807.0f,306.0f,807.0f,432.0f,.85f);
        txt("CURRENT OUTPUT",831,313,414,21,11.5f,true,true);
        txt(endpoint_list_.getText().isNotEmpty()
                ? endpoint_list_.getText() : String("Select an output device"),
            831,342,438,33,17.0f,false,true);
        txt("PRIORITY",831,391,340,18,11.0f,true,true);
        txt(FxController::getInstance().isOutputPinned()
                ? "Keep this device when connected"
                : "Automatically follow available devices",
            831,413,438,19,12.0f,true);
        txt("DEVICE FALLBACK",266,474,500,27,15,false,true);
        txt("Automatic recovery when a selected device disconnects.",266,546,680,26,13,true);
        txt("ROUTING STATUS",968,485,281,20,11.0f,true,true);
        txt(FxController::getInstance().isPlaybackDeviceAvailable()
                ? "Output available" : "Output unavailable",
            968,511,285,28,15.0f,false,true);
        txt("Device-specific presets and priority are managed in Settings.",
            266,581,920,24,12.3f,true);
    } else if(page_==3) {
        txt("NEURAL CATALOGUE  /  IMPORT  /  ACTIVATE",267,205,790,25,12,true,true);
    } else {
        txt("AI DJ  /  OPTIONAL CREATIVE EFFECTS",267,205,790,25,12,true,true);
        card(247,252,686,205);
        card(944,252,356,205);
        card(247,468,1053,220);
        txt("AI DJ  /  LIVE REMIX",266,265,600,29,18,false,true);
        txt("Optional effects running locally, only when enabled.",266,295,635,21,12.4f,true);
        txt("MODEL STATUS",961,264,318,20,12,true,true);
        if (auto* dj=FxController::getInstance().dj()) {
            txt(dj->busy()?"PREPARING DOWNLOAD":
                dj->installed()?(dj->enabled()?"DJ ENABLED":"DJ READY"):"NOT INSTALLED",
                961,293,305,34,18,false,true);
            txt(dj->installed()?"Local installation is available":
                "Download required to activate",961,335,311,22,12,true);
        }
        // Honest, compact readout of the actual configured DJ intensity.
        if(auto* dj=FxController::getInstance().dj()) {
            const float ratio=juce::jlimit(0.0f,1.0f,dj->intensity());
            const float start=-juce::MathConstants<float>::pi*.77f;
            const float sweep=juce::MathConstants<float>::pi*1.54f;
            juce::Path track,fill;
            track.addCentredArc(1195.0f,398.0f,31.0f,31.0f,0.0f,start,start+sweep,true);
            fill.addCentredArc(1195.0f,398.0f,31.0f,31.0f,0.0f,
                               start,start+sweep*ratio,true);
            g.setColour(border.brighter(.22f));
            g.strokePath(track,juce::PathStrokeType(4.0f,juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
            g.setColour(cyan.withAlpha(.91f));
            g.strokePath(fill,juce::PathStrokeType(4.0f,juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
            txt(String(juce::roundToInt(ratio*100.0f))+"%",1164,388,62,19,
                12.8f,false,true);
            txt("INTENSITY",967,391,194,18,11.0f,true,true);
            txt("Optional independent effects",967,418,204,20,11.0f,true);
        }
        txt("EFFECT BEHAVIOUR",266,482,730,24,15,false,true);
        txt("Intensity",266,522,148,23,12.5f,true);
        txt("Minimum spacing",266,574,166,23,12.5f,true);
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
    // Paint vector glyphs over TextButton backgrounds: custom hover and
    // liquid materials are not allowed to obscure navigation icons.
    const juce::Colour cyan(0xff45bffb),muted(0xff9aadc4);
    // Fixed-size JUCE vector navigation icons. Distinct silhouettes make
    // pages recognisable without requiring new textures or per-frame assets.
    for(int i=0;i<6;++i) {
        const float y=125.0f+49.0f*i;
        const Colour iconColour=(i==page_?cyan:muted).withAlpha(i==page_?.92f:.67f);
        g.setColour(iconColour);
        switch(i) {
            case 0: { // Sound Studio: live EQ bars
                const float heights[]={7.0f,14.0f,10.0f,19.0f,9.0f};
                for(int b=0;b<5;++b) {
                    const float x=36.0f+b*4.5f;
                    g.drawLine(x,y-heights[b]*.5f,x,y+heights[b]*.5f,1.6f);
                }
                break;
            }
            case 1: { // Neural activity network
                g.drawLine(38,y+5,44,y-5,1.4f);
                g.drawLine(44,y-5,51,y+4,1.4f);
                g.drawLine(38,y+5,51,y+4,1.0f);
                g.fillEllipse(35.0f,y+2.0f,5.5f,5.5f);
                g.fillEllipse(41.5f,y-8.0f,5.5f,5.5f);
                g.fillEllipse(48.5f,y+1.0f,5.5f,5.5f);
                break;
            }
            case 2: { // Output devices / headphones
                juce::Path cans;
                cans.addCentredArc(44.0f,y,8.5f,8.5f,0.0f,
                                   -juce::MathConstants<float>::pi*.85f,
                                   juce::MathConstants<float>::pi*.85f,true);
                g.strokePath(cans,juce::PathStrokeType(1.5f));
                g.fillRoundedRectangle(34.3f,y+.4f,3.6f,8.0f,1.4f);
                g.fillRoundedRectangle(50.1f,y+.4f,3.6f,8.0f,1.4f);
                break;
            }
            case 3: { // AI Hub: connected model nodes
                g.drawLine(37.5f,y-4.5f,50.0f,y-4.5f,1.2f);
                g.drawLine(37.5f,y-4.5f,44.0f,y+7.0f,1.2f);
                g.drawLine(50.0f,y-4.5f,44.0f,y+7.0f,1.2f);
                g.fillEllipse(34.5f,y-7.5f,6.0f,6.0f);
                g.fillEllipse(47.0f,y-7.5f,6.0f,6.0f);
                g.fillEllipse(41.0f,y+4.0f,6.0f,6.0f);
                break;
            }
            case 4: { // AI DJ: cross-fader and 2 decks
                g.drawEllipse(34.0f,y-7.0f,10.0f,10.0f,1.4f);
                g.drawEllipse(47.0f,y-7.0f,10.0f,10.0f,1.4f);
                g.drawLine(38.0f,y+7.0f,53.0f,y+7.0f,1.3f);
                g.fillEllipse(44.0f,y+5.0f,4.0f,4.0f);
                break;
            }
            case 5: { // Studio: sliders and material adjustments
                for(int j=0;j<3;++j) {
                    const float lineY=y-7.0f+7.0f*j;
                    g.drawLine(35.0f,lineY,54.0f,lineY,1.1f);
                    const float knobX=j==0?41.0f:j==1?50.0f:45.0f;
                    g.fillEllipse(knobX-2.7f,lineY-2.7f,5.4f,5.4f);
                }
                break;
            }
        }
    }
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
