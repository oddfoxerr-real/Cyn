/* FxSound / Cyn. Copyright (C) 2025 FxSound LLC.
   SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <JuceHeader.h>
#include "FxView.h"
#include "FxTheme.h"
#include "FxAudioControls.h"
#include "FxEqualizer.h"
#include "FxVisualizer.h"
#include "AudioLearningTimeline.h"
#include "CynNeuralActivity.h"

class CynRailButton : public TextButton
{
public:
    CynRailButton(const String& title, int symbol) : TextButton(title), symbol_(symbol) {}
    void paintButton(Graphics& g, bool hovered, bool down) override
    {
        auto& theme=dynamic_cast<FxTheme&>(getLookAndFeel());
        const float hover=theme.animate(*this,0,hovered || hasKeyboardFocus(true) ? 1.0f : 0.0f);
        const float selected=theme.animate(*this,2,getToggleState() ? 1.0f : 0.0f);
        const float press=theme.animate(*this,1,down ? 1.0f : 0.0f);
        g.setColour(Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(jmax(hover*.7f,selected)));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(press),9);
        g.setColour(Colour(FxTheme::getColor(getToggleState() ? SliderTrack : HintText)).withAlpha(1.0f));
        const float x = getWidth() / 2.0f, y = getHeight() / 2.0f;
        Path glyph;
        if (symbol_ == 0)
        {
            for (int i = -1; i <= 1; ++i)
                glyph.addRoundedRectangle(x + i * 5 - 1, y - (i == 0 ? 8.0f : 5.0f), 2, i == 0 ? 16 : 10, 1);
            g.fillPath(glyph);
        }
        else if (symbol_ == 1)
        {
            glyph.startNewSubPath(x, y - 9); glyph.lineTo(x + 3, y - 3);
            glyph.lineTo(x + 9, y); glyph.lineTo(x + 3, y + 3);
            glyph.lineTo(x, y + 9); glyph.lineTo(x - 3, y + 3);
            glyph.lineTo(x - 9, y); glyph.lineTo(x - 3, y - 3); glyph.closeSubPath();
            g.strokePath(glyph, PathStrokeType(1.4f));
        }
        else if (symbol_ == 2)
        {
            glyph.startNewSubPath(x - 8, y + 5); glyph.lineTo(x - 8, y - 1);
            glyph.cubicTo(x - 8, y - 12, x + 8, y - 12, x + 8, y - 1);
            glyph.lineTo(x + 8, y + 5); g.strokePath(glyph, PathStrokeType(1.5f));
            g.fillRoundedRectangle(x - 9, y, 4, 8, 1.5f);
            g.fillRoundedRectangle(x + 5, y, 4, 8, 1.5f);
        }
        else
        {
            for (int i = -1; i <= 1; ++i) g.fillEllipse(x + i * 6 - 1.5f, y - 1.5f, 3, 3);
        }
    }
private:
    int symbol_;
};

class FxProView : public FxView, private Timer
{
public:
    FxProView();
    ~FxProView() override = default;
    void startVisualizer();
    void pauseVisualizer();
    void beginThemeTransition();
    void finishThemeTransition();
    void update();
    void setLookAndFeel();
private:
    static constexpr int WIDTH = 1240, HEIGHT = 620;
    void resized() override;
    void paint(Graphics&) override;
    void paintOverChildren(Graphics&) override;
    void lookAndFeelChanged() override;
    void modelChanged(FxModel::Event) override;
    void timerCallback() override;
    void selectPage(int page);
    void openSettings();
    int page_ = 0;
    TextButton misc_nav_{"Misc"},dj_download_{"Download AI DJ"},dj_remove_{"Remove download"},dj_cancel_{"Cancel download"};
    ToggleButton dj_toggle_{"Enable AI DJ"},dj_stereo_{"Move echoes between ears"};
    Slider dj_intensity_{Slider::LinearHorizontal,Slider::TextBoxRight},dj_spacing_{Slider::LinearHorizontal,Slider::TextBoxRight};
    Label dj_status_,dj_recent_;
    double dj_progress_value_=0;
    ProgressBar dj_progress_{dj_progress_value_};
    Image transition_image_;
    Image theme_from_image_, theme_to_image_;
    double theme_transition_start_=0.0;
    double transition_start_=0, last_status_update_=0, last_neural_update_=0;
    FxAudioControls audio_controls_;
    FxEqualizer equalizer_;
    FxVisualizer visualizer_;
    TooltipWindow tool_tip_;
    TextButton sound_nav_ { "Sound studio" }, smart_nav_ { "Smart listening" }, devices_nav_ { "Output devices" }, ai_hub_nav_ { "AI Hub" };
    CynRailButton home_ { "Sound studio", 0 }, smart_rail_ { "Smart listening", 1 }, devices_rail_ { "Output devices", 2 };
    CynRailButton settings_rail_ { "Settings", 3 };
    TextButton settings_ { "Settings" };
    TextButton refresh_ { "Refresh devices" };
    ToggleButton smart_toggle_ { "Smart listening" }, pin_toggle_ { "Keep my selected output" };
    Label live_status_, smart_status_;
    Slider sensitivity_ { Slider::LinearHorizontal, Slider::TextBoxRight };
    AudioLearningTimeline learning_timeline_;
    CynNeuralActivity neural_activity_;
    // Message-thread copies only: never fetch inference inside paint or DSP.
    CynAudioScene neural_core_scene_ {};
    bool neural_core_live_=false;
    float theme_audio_level_=0.0f;
    double last_theme_repaint_=0;
    double last_ai_controls_update_=0;
    FxComboBox learning_effect_;
    FxComboBox listening_mode_;
    FxComboBox ai_profile_;
    StringArray ai_profile_ids_;
    int ai_profile_refresh_ticks_=0;
    void refreshAIProfileChoices();
    TextButton more_ {"More here"}, less_ {"Less here"}, train_ {"Train my model"}, starter_ {"Reset to starter"}, resume_ {"Resume timeline"};
    TextButton export_ai_ { "Export AI" }, import_ai_ { "Import AI" }, browse_ai_ { "AI Hub" };
    Label learning_status_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxProView)
};
