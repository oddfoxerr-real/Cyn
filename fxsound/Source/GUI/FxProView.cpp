/* FxSound / Cyn. Copyright (C) 2025 FxSound LLC.
   SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FxProView.h"
#include "FxController.h"
#include "FxTheme.h"
#include "FxSettingsDialog.h"
#include "CynAiHubDialog.h"

FxProView::FxProView() : tool_tip_(this)
{
    // JUCE TooltipWindow is opaque by default; our rounded corners require compositing.
    tool_tip_.setOpaque(false);
    for (auto* component : std::initializer_list<Component*> {
        &audio_controls_, &equalizer_, &visualizer_, &sound_nav_, &smart_nav_,
        &devices_nav_, &ai_hub_nav_, &home_, &smart_rail_, &devices_rail_, &settings_,
        &refresh_, &smart_toggle_, &pin_toggle_, &live_status_, &smart_status_, &settings_rail_, &sensitivity_, &learning_timeline_, &neural_activity_, &learning_effect_, &more_, &less_, &train_, &starter_, &resume_, &learning_status_, &export_ai_, &import_ai_, &browse_ai_ })
        addAndMakeVisible(component);
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
    sensitivity_.setTooltip("How strongly your learned More / Less preferences change effects. At 0%, effect settings remain manual.");
    sensitivity_.onValueChange = [this] {
        FxController::getInstance().setSmartSensitivity((float)sensitivity_.getValue());
    };
    learning_effect_.addItemList({"Clarity", "Ambience", "Surround", "Dynamic Boost", "Bass"},1);
    learning_effect_.setSelectedId(5,dontSendNotification);
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
            "Replace your saved labels and learned preferences with the optional starter model?","Reset","Cancel"))
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
    sound_nav_.setToggleState(page == 0, dontSendNotification);
    smart_nav_.setToggleState(page == 1, dontSendNotification);
    devices_nav_.setToggleState(page == 2, dontSendNotification);
    home_.setToggleState(page == 0, dontSendNotification);
    smart_rail_.setToggleState(page == 1, dontSendNotification);
    devices_rail_.setToggleState(page == 2, dontSendNotification);
    for (auto* button : std::initializer_list<TextButton*> { &sound_nav_, &smart_nav_, &devices_nav_, &ai_hub_nav_, &home_, &smart_rail_, &devices_rail_ })
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
    for(auto* c : std::initializer_list<Component*> {&learning_timeline_, &learning_effect_, &more_, &less_, &train_, &starter_, &resume_, &learning_status_, &export_ai_, &import_ai_, &browse_ai_}) c->setVisible(page==1);
    pin_toggle_.setVisible(page == 2);
    refresh_.setVisible(page == 2);
    sensitivity_.setVisible(page == 1);
    neural_activity_.setVisible(page == 1);
    neural_activity_.setAnimationEnabled(page == 1);
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
    audio_controls_.update();
    equalizer_.update();
    equalizer_.showValues(true);
    audio_controls_.showValues(true);
    auto& controller = FxController::getInstance();
    smart_toggle_.setToggleState(controller.isSmartListening(), dontSendNotification);
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
    if(now-last_status_update_<500) return;
    last_status_update_=now;
    auto& controller = FxController::getInstance();
    neural_activity_.setTrainingActive(controller.isSmartTraining());
    learning_timeline_.updateFrames(controller.getRecentAudio());
    learning_status_.setText(controller.getLearningStatus(),dontSendNotification);
    const bool selected=learning_timeline_.selection().size()>=5;
    more_.setEnabled(selected && !controller.isSmartTraining()); less_.setEnabled(selected && !controller.isSmartTraining());
    train_.setEnabled(!controller.isSmartTraining()); starter_.setEnabled(!controller.isSmartTraining());
    export_ai_.setEnabled(!controller.isSmartTraining()); import_ai_.setEnabled(!controller.isSmartTraining());
    smart_toggle_.setToggleState(controller.isSmartListening(), dontSendNotification);
    pin_toggle_.setToggleState(controller.isOutputPinned(), dontSendNotification);
    smart_status_.setText(controller.getSmartSummary(), dontSendNotification);
    if (!sensitivity_.isMouseButtonDown())
        sensitivity_.setValue(controller.getSmartSensitivity(), dontSendNotification);
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    smart_status_.setFont(theme.getNormalFont().withHeight(13));
    smart_status_.setColour(Label::textColourId, Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    const bool power = FxModel::getModel().getPowerState();
    live_status_.setFont(theme.getNormalFont().withHeight(12));
    live_status_.setColour(Label::textColourId, Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    live_status_.setText(!power ? "Processing off" : !controller.isPlaybackDeviceAvailable()
        ? "Output unavailable" : controller.isAudioProcessing() ? "Processing audio" : "Ready to listen", dontSendNotification);
}
void FxProView::resized()
{
    home_.setBounds(10, 18, 32, 34);
    smart_rail_.setBounds(10, 64, 32, 34);
    devices_rail_.setBounds(10, 110, 32, 34);
    settings_rail_.setBounds(10, 565, 32, 34);
    // Reserve the empty sidebar area for the native neural-activity animation.
    neural_activity_.setBounds(72, 247, 148, 226);
    learning_timeline_.setBounds(256,164,952,156);
    learning_effect_.setBounds(280,433,194,36);
    more_.setBounds(490,433,136,36); less_.setBounds(638,433,136,36);
    train_.setBounds(790,433,210,36); resume_.setBounds(1012,433,170,36);
    sensitivity_.setBounds(444,491,738,28);
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
    smart_toggle_.setBounds(272, page_ == 1 ? 350 : 565, 220, 28);
    smart_status_.setBounds(508, page_ == 1 ? 351 : 566, 678, 26);
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
        g.fillAll(color(WindowBackground));
        g.setColour(color(PanelBackground)); g.fillRect(52, 0, 180, HEIGHT);
        g.setColour(color(RowOutline).withAlpha(0.65f));
        g.drawVerticalLine(232, 0, HEIGHT - 8.0f);
    }
    g.setColour(color(RowOutline).withAlpha(0.65f));
    g.strokePath(workspace, PathStrokeType(1.0f));
    auto text = [&](const String& value, int x, int y, int w, int h, float size, bool muted = false) {
        g.setColour(color(muted ? HintText : DefaultText));
        g.setFont(theme.getNormalFont().withHeight(size));
        g.drawText(value, x, y, w, h, Justification::centredLeft, true);
    };
    auto card = [&](int x, int y, int w, int h) {
        g.setColour(color(ControlBackground)); g.fillRoundedRectangle((float)x, (float)y, (float)w, (float)h, 16);
        g.setColour(color(RowOutline).withAlpha(0.65f)); g.drawRoundedRectangle(x+0.5f, y+0.5f, w-1.0f, h-1.0f, 16, 1);
    };
    const float selected=theme.animate(*this,3,(float)page_);
    g.setColour(color(MenuHighlightBackground));
    g.fillRoundedRectangle(68.0f,72.0f+42.0f*selected,152.0f,36.0f,12.0f);
    g.setColour(color(SliderTrack));
    g.fillRoundedRectangle(68.0f,83.0f+42.0f*selected,3.0f,14.0f,1.5f);
    text("Your workspace", 76, 25, 140, 20, 14);
    text("AUDIO", 80, 52, 132, 16, 10, true);
    text("Cyn / Personal audio", 80, 486, 136, 18, 12, true);
    text("Made for your ears.", 80, 508, 140, 18, 12, true);
    text("Version 2.0.14", 80, 529, 140, 16, 10, true);
    text(page_ == 0 ? "Sound studio" : page_ == 1 ? "Smart listening" : "Output devices", 256, 16, 640, 32, 25);
    text(page_ == 0 ? "Shape your sound. Keep your focus." : page_ == 1
        ? "Teach Cyn your preferences for music, movies, shows and games." : "Your output, with fewer interruptions.", 256, 49, 640, 22, 13, true);
    text("PRESET", 260, 78, 300, 16, 10, true);
    text("OUTPUT DEVICE", 741, 78, 300, 16, 10, true);
    if (page_ == 0) card(256, 556, 952, 48);
    else if (page_ == 1)
    {
        card(256, 334, 952, 270);
        text("Play any audio, select a section above and teach More or Less.",280,389,880,28,16);
        text("AI PROFILES", 688, 521, 310, 18, 10, true);
        text("Sensitivity",280,491,150,28,14);

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
void FxProView::modelChanged(FxModel::Event event)
{
    FxView::modelChanged(event);
    if (event == FxModel::Event::PresetSelected || event == FxModel::Event::Other) update();
    else { timerCallback(); repaint(); }
}

void FxProView::paintOverChildren(Graphics& g)
{
    if(transition_image_.isValid() && !FxTheme::isReducedMotion()) {
        const float progress=jlimit(0.0f,1.0f,(float)((Time::getMillisecondCounterHiRes()-transition_start_)/220.0));
        g.setOpacity((1-progress)*(1-progress));
        g.drawImageAt(transition_image_,0,0);
    }
}
void FxProView::lookAndFeelChanged()
{
    transition_image_=Image();
    if(FxTheme::isReducedMotion()) startTimer(500); else startTimerHz(60);
    selectPage(page_);
    neural_activity_.setAnimationEnabled(page_ == 1);
    learning_status_.setColour(Label::textColourId,Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    repaint();
}
