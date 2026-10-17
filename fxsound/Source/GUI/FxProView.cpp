/* FxSound / Cyn. Copyright (C) 2025 FxSound LLC.
   SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FxProView.h"
#include "FxController.h"
#include "FxTheme.h"
#include "CynThemeAtmosphere.h"
#include "FxSettingsDialog.h"
#include "CynAiHubDialog.h"
#include <array>
#include <cmath>

FxProView::FxProView() : tool_tip_(this)
{
    // JUCE TooltipWindow is opaque by default; our rounded corners require compositing.
    tool_tip_.setOpaque(false);
    for (auto* component : std::initializer_list<Component*> {
        &audio_controls_, &equalizer_, &visualizer_, &sound_nav_, &smart_nav_,
        &devices_nav_, &ai_hub_nav_, &home_, &smart_rail_, &devices_rail_, &settings_,
        &refresh_, &smart_toggle_, &pin_toggle_, &live_status_, &smart_status_, &settings_rail_, &sensitivity_, &headroom_cap_, &preset_cycling_, &preset_cycling_status_, &learning_timeline_, &neural_activity_, &learning_effect_, &listening_mode_, &ai_profile_, &more_, &less_, &train_, &starter_, &resume_, &learning_status_, &export_ai_, &import_ai_, &browse_ai_ })
        addAndMakeVisible(component);
    for(auto* c:std::initializer_list<Component*> {&misc_nav_,&dj_download_,&dj_remove_,&dj_cancel_,&dj_toggle_,&dj_stereo_,&dj_intensity_,&dj_spacing_,&dj_status_,&dj_recent_,&dj_progress_}) addAndMakeVisible(c);
    misc_nav_.onClick=[this]{selectPage(3);};
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
    setSize(WIDTH, HEIGHT);
    selectPage(0);
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
void FxProView::selectPage(int page)
{
    if(page!=page_ && isShowing() && !FxTheme::isReducedMotion()) {
        transition_image_=createComponentSnapshot(getLocalBounds()); transition_start_=Time::getMillisecondCounterHiRes();
    }
    page_ = page;
    misc_nav_.setToggleState(page==3,dontSendNotification);
    for(auto* c:std::initializer_list<Component*> {&dj_download_,&dj_remove_,&dj_cancel_,&dj_toggle_,&dj_stereo_,&dj_intensity_,&dj_spacing_,&dj_status_,&dj_recent_,&dj_progress_}) c->setVisible(page==3);
    smart_toggle_.setVisible(page!=3);smart_status_.setVisible(page!=3);
    sound_nav_.setToggleState(page == 0, dontSendNotification);
    smart_nav_.setToggleState(page == 1, dontSendNotification);
    devices_nav_.setToggleState(page == 2, dontSendNotification);
    home_.setToggleState(page == 0, dontSendNotification);
    smart_rail_.setToggleState(page == 1, dontSendNotification);
    devices_rail_.setToggleState(page == 2, dontSendNotification);
    for (auto* button : std::initializer_list<TextButton*> { &sound_nav_, &smart_nav_, &devices_nav_, &ai_hub_nav_, &misc_nav_, &home_, &smart_rail_, &devices_rail_ })
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
    const auto now=Time::getMillisecondCounterHiRes();
    if(transition_image_.isValid()) {
        if(FxTheme::isReducedMotion() || now-transition_start_>=220) transition_image_=Image();
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
    live_status_.setFont(theme.getNormalFont().withHeight(12));
    live_status_.setColour(Label::textColourId, Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    live_status_.setText(!power ? "Processing off" : !controller.isPlaybackDeviceAvailable()
        ? "Output unavailable" : controller.isAudioProcessing() ? "Processing audio" : "Ready to listen", dontSendNotification);
}
void FxProView::resized()
{
    misc_nav_.setBounds(68,240,152,36);
    dj_toggle_.setBounds(280,270,230,32);
    dj_download_.setBounds(280,215,200,38);dj_remove_.setBounds(280,215,200,38);dj_cancel_.setBounds(280,215,200,38);
    dj_progress_.setBounds(505,224,665,22);
    dj_status_.setBounds(280,310,900,35);
    dj_intensity_.setBounds(530,376,645,30);dj_spacing_.setBounds(530,422,645,30);
    dj_stereo_.setBounds(280,473,470,30);dj_recent_.setBounds(280,525,895,26);
    home_.setBounds(10, 18, 32, 34);
    smart_rail_.setBounds(10, 64, 32, 34);
    devices_rail_.setBounds(10, 110, 32, 34);
    settings_rail_.setBounds(10, 565, 32, 34);
    // Reserve the empty sidebar area for the native neural-activity animation.
    neural_activity_.setBounds(72, 286, 148, 187);
    learning_timeline_.setBounds(256,164,952,156);
    learning_effect_.setBounds(280,433,194,36);
    listening_mode_.setBounds(280,386,235,33);
    ai_profile_.setBounds(540,386,425,33);
    more_.setBounds(490,433,136,36); less_.setBounds(638,433,136,36);
    train_.setBounds(790,433,210,36); resume_.setBounds(1012,433,170,36);
    sensitivity_.setBounds(403,491,323,28);
    headroom_cap_.setBounds(879,491,303,28);
    preset_cycling_.setBounds(280,520,225,24);
    preset_cycling_status_.setBounds(500,520,172,24);
    learning_status_.setBounds(280, 542, 390, 42);
    export_ai_.setBounds(685, 545, 96, 35);
    import_ai_.setBounds(791, 545, 96, 35);
    browse_ai_.setBounds(897, 545, 106, 35);
    starter_.setBounds(1012,545,170,35);
    sound_nav_.setBounds(68, 72, 152, 36);
    smart_nav_.setBounds(68, 114, 152, 36);
    devices_nav_.setBounds(68, 156, 152, 36);
    ai_hub_nav_.setBounds(68, 198, 152, 36);
    settings_.setBounds(68, 554, 152, 36);
    ai_hub_nav_.setColour(TextButton::buttonColourId, Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f));
    ai_hub_nav_.setColour(TextButton::textColourOffId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    browse_ai_.setColour(TextButton::buttonColourId, Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f));
    browse_ai_.setColour(TextButton::textColourOffId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    export_ai_.setColour(TextButton::buttonColourId, Colour(FXCOLOR(PanelBackground)).withAlpha(1.0f));
    import_ai_.setColour(TextButton::buttonColourId, Colour(FXCOLOR(PanelBackground)).withAlpha(1.0f));
    for (auto* button : std::initializer_list<TextButton*> { &export_ai_, &import_ai_ })
    {
        button->setColour(TextButton::textColourOffId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
        button->setColour(TextButton::textColourOnId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    }
    browse_ai_.setColour(TextButton::textColourOnId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    ai_hub_nav_.setColour(TextButton::textColourOnId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    live_status_.setBounds(1000, 23, 208, 24);
    preset_list_.setBounds(256, 97, 465, 40);
    endpoint_list_.setBounds(737, 97, 471, 40);
    visualizer_.setBounds(256, 164, 952, 102);
    audio_controls_.setBounds(256, 288, 168, 257);
    equalizer_.setBounds(440, 288, 768, 257);
    smart_toggle_.setBounds(272, page_ == 1 ? 339 : 565, 220, 25);
    smart_status_.setBounds(508, page_ == 1 ? 340 : 566, 678, 23);
    if (page_ == 2)
    {
        smart_toggle_.setBounds(276, 406, 220, 28);
        smart_status_.setBounds(508, 407, 678, 26);
    }
    pin_toggle_.setBounds(276, 204, 410, 30);
    refresh_.setBounds(276, 302, 168, 36);
}
void FxProView::paint(Graphics& g)
{
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    auto color = [](FxColor c) { return Colour(FxTheme::getColor(c)).withAlpha(1.0f); };
    // Leave the outer rail transparent so the parent paints one continuous shell.
    Path workspace;
    workspace.addRoundedRectangle(52.0f, 0.0f, WIDTH - 60.0f, HEIGHT - 8.0f, 18.0f);
    {
        Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(workspace);
        // Actual immersive artwork is clipped to the entire workspace,
        // never placed as a decorative widget underneath sidebar controls.
        CynThemeAtmosphere::paint(g,FxTheme::getThemeMode(),
            FxTheme::isReducedMotion()?0.0f:
                (float)(Time::getMillisecondCounterHiRes()*0.001),
            theme_audio_level_,juce::Rectangle<float>(52.0f,0.0f,
                                                WIDTH-52.0f,HEIGHT-8.0f));
        const auto rail=color(PanelBackground);
        // Let the full scene continue faintly beneath navigation, with a
        // legible tinted overlay. No separate sidebar theme card or obstruction.
        const bool webDark=FxTheme::getThemeMode()==FxThemeMode::DarkMesh;
        ColourGradient sidebar((webDark?Colour(0xff000000):rail.brighter(0.045f)).withAlpha(webDark?0.90f:0.94f),
                               52.0f,0.0f,
                               (webDark?Colour(0xff000000):rail.darker(0.11f)).withAlpha(webDark?0.91f:0.82f),
                               232.0f,(float)HEIGHT,false);
        g.setGradientFill(sidebar);
        g.fillRect(52,0,180,HEIGHT);
        const float drift=FxTheme::isReducedMotion()?0.0f:
            (float)Time::getMillisecondCounterHiRes()*0.00032f;
        const float glowY=355.0f+112.0f*std::sin(drift);
        ColourGradient railGlow(color(SliderTrack).withAlpha((webDark?0.075f+0.065f*theme_audio_level_:0.075f+0.085f*theme_audio_level_)),
                                218.0f,glowY,
                                Colours::transparentBlack,55.0f,glowY+185.0f,true);
        g.setGradientFill(railGlow);
        g.fillRect(52,0,180,HEIGHT);
        if(FxTheme::getThemeMode()==FxThemeMode::NeuralCore) {
            // The top-right telemetry is ACTUAL pretrained YAMNet event scores
            // (not YAMNet internal activations or a synthetic music animation).
            const auto cyan=Colour(0xff67edee), violet=Colour(0xffa18cff);
            const auto glow=jlimit(0.0f,1.0f,theme_audio_level_);
            ColourGradient aura(cyan.withAlpha(0.045f+glow*0.075f),
                                985.0f,115.0f,
                                Colours::transparentBlack,1140.0f,370.0f,true);
            g.setGradientFill(aura);
            g.fillEllipse(785.0f,-65.0f,410.0f,430.0f);
            // Faint fixed-position lines only illuminate when personal
            // inference is valid. Actual connection activity is drawn in
            // the 11->12->5 neural graph on the sidebar.
            for(int i=0;i<9;++i) {
                const float x=275.0f+99.0f*i;
                g.setColour((i%2?violet:cyan).withAlpha(0.018f+0.035f*glow));
                g.drawLine(x,84.0f,x+55.0f,284.0f,0.7f);
            }
            const auto activity=neural_core_live_?neural_core_scene_:CynAudioScene {};
            const float meters[3]={activity.music,activity.percussion,activity.speech};
            const char* names[3]={"MUSIC","DRUMS","VOICE"};
            for(int i=0;i<3;++i) {
                const float x=968.0f+80.0f*i, y=59.0f;
                g.setColour(color(HintText).withAlpha(0.84f));
                g.setFont(Font(8.0f,Font::bold));
                g.drawText(names[i],(int)x,46,72,11,Justification::centredLeft,false);
                g.setColour(color(RowOutline).withAlpha(0.9f));
                g.fillRoundedRectangle(x,y,65.0f,3.5f,1.75f);
                g.setColour((i==2?violet:cyan).withAlpha(neural_core_live_?0.90f:0.20f));
                g.fillRoundedRectangle(x,y,65.0f*jlimit(0.0f,1.0f,meters[i]),3.5f,1.75f);
            }
            g.setColour(cyan.withAlpha(neural_core_live_?0.85f:0.28f));
            g.setFont(Font(9.0f,Font::bold));
            g.drawText(neural_core_live_?"YAMNET / LIVE EVENTS":"YAMNET / WAITING",
                       987,6,220,14,Justification::centredRight,false);
        }
        g.setColour(color(RowOutline).withAlpha(0.65f));
        g.drawVerticalLine(232,0,HEIGHT-8.0f);
    }
    g.setColour(color(RowOutline).withAlpha(0.65f));
    g.strokePath(workspace, PathStrokeType(1.0f));
    auto text = [&](const String& value, int x, int y, int w, int h, float size, bool muted = false) {
        g.setColour(color(muted ? HintText : DefaultText));
        g.setFont(theme.getNormalFont().withHeight(size));
        g.drawText(value, x, y, w, h, Justification::centredLeft, true);
    };
    auto card = [&](int x, int y, int w, int h) {
        const auto panel=color(ControlBackground);
        ColourGradient glass((FxTheme::getThemeMode()==FxThemeMode::DarkMesh ? Colour(0xff010102) : panel.brighter(0.095f)),(float)x,(float)y,
                             (FxTheme::getThemeMode()==FxThemeMode::DarkMesh?Colour(0xff010102):panel.darker(0.055f)),(float)(x+w),(float)(y+h),false);
        glass.addColour(0.42,panel);
        g.setGradientFill(glass);
        g.fillRoundedRectangle((float)x,(float)y,(float)w,(float)h,16.0f);
        if (FxTheme::getThemeMode()==FxThemeMode::DarkMesh && w>=600 && h>=140) {
            // Continue the page's dotted web faintly ON the large black card.
            // Small controls and text remain unobstructed and readable.
            Graphics::ScopedSaveState clip(g);
            Path mask; mask.addRoundedRectangle((float)x,(float)y,(float)w,(float)h,16.0f);
            g.reduceClipRegion(mask);
            const int cols=23, rows=6;
            auto dot=[&](int cx,int cy) -> Point<float> {
                return {float(x+w-430+cx*21),float(y+h-122+cy*23)
                     +4.0f*std::sin(cx*0.49f+cy*0.31f)};
            };
            for(int ry=0;ry<rows;++ry)
                for(int col=0;col<cols;++col) {
                    const auto p=dot(col,ry);
                    const float opacity=0.009f+0.033f*(float)ry/(float)(rows-1);
                    g.setColour(Colour(0xff9fc7ff).withAlpha(opacity));
                    if(col+1<cols)g.drawLine(Line<float>(p,dot(col+1,ry)),0.6f);
                    if(ry+1<rows)g.drawLine(Line<float>(p,dot(col,ry+1)),0.6f);
                    g.setColour(Colour(0xffdeebff).withAlpha(opacity*1.7f));
                    g.fillEllipse(p.x-0.85f,p.y-0.85f,1.7f,1.7f);
                }
        }
        g.setColour(color(DefaultText).withAlpha(0.065f));
        g.drawLine((float)x+17.0f,(float)y+1.0f,(float)(x+w)-17.0f,(float)y+1.0f,0.8f);
        g.setColour(color(RowOutline).withAlpha(0.72f));
        g.drawRoundedRectangle(x+0.5f,y+0.5f,w-1.0f,h-1.0f,16.0f,1.0f);
    };
    const float selected=theme.animate(*this,3,page_==3?4.0f:(float)page_);
    g.setColour(color(MenuHighlightBackground));
    g.fillRoundedRectangle(68.0f,72.0f+42.0f*selected,152.0f,36.0f,12.0f);
    g.setColour(color(SliderTrack));
    g.fillRoundedRectangle(68.0f,83.0f+42.0f*selected,3.0f,14.0f,1.5f);
    text("Your workspace", 76, 25, 140, 20, 14);
    text("AUDIO", 80, 52, 132, 16, 10, true);
    text("Cyn / Personal audio", 80, 486, 136, 18, 12, true);
    text("Made for your ears.", 80, 508, 140, 18, 12, true);
    text("Version 2.0.46", 80, 529, 140, 16, 10, true);
    text(page_ == 0 ? "Sound studio" : page_ == 1 ? "Smart listening" : page_==2 ? "Output devices" : "Misc / AI DJ", 256, 16, 640, 32, 25);
    text(page_ == 0 ? "Shape your sound. Keep your focus." : page_ == 1
        ? "Choose a listening mode and the neural model that powers it." : page_==2 ? "Your output, with fewer interruptions." : "Optional musical accents, made for your headphones.", 256, 49, 640, 22, 13, true);
    text("PRESET", 260, 78, 300, 16, 10, true);
    text("OUTPUT DEVICE", 741, 78, 300, 16, 10, true);
    if (page_ == 0) card(256, 556, 952, 48);
    else if (page_ == 1)
    {
        card(256, 334, 952, 270);
        text("LISTENING MODE",280,369,235,13,10,true);
        text("ACTIVE AI MODEL",540,369,425,13,10,true);
        text("Swap models anytime.",977,386,205,28,12,true);
        text("AI PROFILES", 688, 521, 310, 18, 10, true);
        text("Sensitivity",280,491,120,28,14);
        text("Headroom Cap",747,491,134,28,13);

    }
    else if(page_==3)
    {
        card(256,164,952,188);card(256,365,952,207);
        text("AI DJ / Live remix",280,172,850,30,21);
        text("Intensity",280,376,220,30,15);
        text("Minimum spacing",280,422,230,30,15);
        text("Remixes the playing song: beat repeats, echoes and filter sweeps. Timing is approximate.",280,578,900,22,12,true);
    }
    else
    {
        card(256, 171, 952, 190);
        text("Stay with this device while it is available. If unplugged, use the next available output.", 280, 247, 880, 24, 13, true);
        card(256, 388, 952, 168);
        text("Sound shaping follows your selected output.", 280, 452, 860, 24, 16);
        text("Device priorities and per-device presets are available in Settings.", 280, 490, 860, 24, 13, true);
    }
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

void FxProView::paintOverChildren(Graphics& g)
{
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
        const float progress=jlimit(0.0f,1.0f,(float)((Time::getMillisecondCounterHiRes()-transition_start_)/220.0));
        g.setOpacity((1-progress)*(1-progress));
        g.drawImageAt(transition_image_,0,0);
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
