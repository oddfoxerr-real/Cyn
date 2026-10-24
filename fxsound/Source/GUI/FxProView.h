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
#include "CynBrainScan3D.h"
#include "CynDjTouch.h"
#include "CynPowerCurtain.h"
#include "CynExpressiveMotion.h"
#include "CynAiHubDialog.h"
#include <array>
#include <cmath>

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
        const bool compact=(bool)getProperties().getWithDefault("riff_icon_rail",false);
        // Riff styles.css (.sidebar/.nav-item): floating 48px tonal icon indicators.
        // Selection is drawn ONCE in FxProView; this button owns the real hit target.
        if(compact) {
            if(hover>.01f) {
                g.setColour(Colour(0xff333438).withAlpha(hover*.82f));
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(press*.5f),24.0f);
            }
        } else {
            g.setColour(Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(jmax(hover*.7f,selected)));
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(press),9);
        }
        const float x=getWidth()*.5f, y=getHeight()*.5f;
        g.setColour(getToggleState()
            ? Colour(FXCOLOR(SliderHighlight)).withAlpha(1.0f)
            : Colour(FXCOLOR(HintText)).withAlpha(compact ? .92f : .8f));
        Path glyph;
        if(symbol_==0) {
            for(int i=-1;i<=1;++i)
                glyph.addRoundedRectangle(x+i*5-1,y-(i==0?8.0f:5.0f),2,i==0?16:10,1);
            g.fillPath(glyph);
        } else if(symbol_==1) {
            glyph.startNewSubPath(x,y-9);glyph.lineTo(x+3,y-3);glyph.lineTo(x+9,y);
            glyph.lineTo(x+3,y+3);glyph.lineTo(x,y+9);glyph.lineTo(x-3,y+3);
            glyph.lineTo(x-9,y);glyph.lineTo(x-3,y-3);glyph.closeSubPath();
            g.strokePath(glyph,PathStrokeType(1.7f));
        } else if(symbol_==2) {
            glyph.startNewSubPath(x-8,y+5);glyph.lineTo(x-8,y-1);
            glyph.cubicTo(x-8,y-12,x+8,y-12,x+8,y-1);
            glyph.lineTo(x+8,y+5);g.strokePath(glyph,PathStrokeType(1.7f));
            g.fillRoundedRectangle(x-9,y,4,8,1.5f);
            g.fillRoundedRectangle(x+5,y,4,8,1.5f);
        } else if(symbol_==3) {
            for(int i=0;i<3;++i) {
                const float px=x+(i-1)*8.0f,py=y+(i==1?-6.0f:4.0f);
                g.drawEllipse(px-3,py-3,6,6,1.6f);
            }
            g.drawLine(x-6,y+2,x-1,y-5,1.4f);
            g.drawLine(x+2,y-5,x+6,y+2,1.4f);
        } else if(symbol_==4) {
            glyph.startNewSubPath(x-3,y-8);glyph.lineTo(x+6,y-10);
            glyph.lineTo(x+6,y+5);g.strokePath(glyph,PathStrokeType(1.7f));
            g.fillEllipse(x-8,y+3,8,6);g.fillEllipse(x+1,y+2,8,6);
            g.drawLine(x-3,y-3,x+6,y-5,1.6f);
        } else if(symbol_==5) {
            for(int i=-1;i<=1;++i) {
                const float bx=x+i*7;
                g.drawLine(bx,y-9,bx,y+9,1.7f);
                g.fillEllipse(bx-2.8f,y+(i==0?3.0f:i<0?-4.0f:-1.0f)-2.8f,5.6f,5.6f);
            }
        } else {
            // Compact settings gear; all seven controls stay keyboard-accessible.
            for(int i=0;i<8;++i) {
                const float a=juce::MathConstants<float>::twoPi*(float)i/8.0f;
                g.drawLine(x+7*std::cos(a),y+7*std::sin(a),
                           x+10*std::cos(a),y+10*std::sin(a),1.8f);
            }
            g.drawEllipse(x-6,y-6,12,12,1.8f);
            g.fillEllipse(x-2,y-2,4,4);
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
    static constexpr int WIDTH = 1320, HEIGHT = 700;
    void resized() override;
    void paint(Graphics&) override;
    void paintOverChildren(Graphics&) override;
    void lookAndFeelChanged() override;
    void modelChanged(FxModel::Event) override;
    void timerCallback() override;
    void selectPage(int page);
    void openSettings();
    void updateDjTouch(double now);
    int page_ = 0;
    CynExpressiveMotion::State expressiveMotion_;
    double lastExpressiveFrameMs_ = 0.0;
    int sound_focus_ = 0; // 0 overview, 1 EQ focus, 2 effects focus.
    void selectSoundFocus(int focus);
    CynRailButton misc_nav_{"Misc / AI DJ",4};
    TextButton dj_download_{"Download AI DJ"},dj_remove_{"Remove download"},dj_cancel_{"Cancel download"};
    ToggleButton dj_toggle_{"Enable AI DJ"},dj_stereo_{"Move echoes between ears"};
    Slider dj_intensity_{Slider::LinearHorizontal,Slider::TextBoxRight},dj_spacing_{Slider::LinearHorizontal,Slider::TextBoxRight};
    Label dj_status_,dj_recent_;
    double dj_progress_value_=0;
    ProgressBar dj_progress_{dj_progress_value_};
    Image transition_image_, transition_to_image_;
    bool capturing_liquid_=false;
    Image theme_from_image_, theme_to_image_;
    double theme_transition_start_=0.0;
    int transition_direction_=1; // Native liquid-screen reveal direction.
    double transition_start_=0, last_status_update_=0, last_neural_update_=0;
    FxAudioControls audio_controls_;
    FxEqualizer equalizer_;
    FxVisualizer visualizer_;
    CynAiHubContent embedded_ai_hub_;
    TooltipWindow tool_tip_;
    CynPowerCurtain power_curtain_;
    CynRailButton sound_nav_ { "Sound Studio",0 }, smart_nav_ { "Smart Listening",1 },
       devices_nav_ { "Output Devices",2 }, ai_hub_nav_ { "AI Hub",3 },
       studio_nav_ { "CYN Studio",5 };
    TextButton ai_status_card_ { "AI status" };
    TextButton overview_tab_ { "Overview" }, eq_focus_tab_ { "Equalizer" },
               effects_focus_tab_ { "Enhancements" };
    CynRailButton home_ { "Sound studio", 0 }, smart_rail_ { "Smart listening", 1 }, devices_rail_ { "Output devices", 2 };
    CynRailButton settings_rail_ { "Settings", 3 };
    CynRailButton settings_ { "Settings",6 };
    TextButton refresh_ { "Refresh devices" };
    ToggleButton smart_toggle_ { "Smart listening" }, pin_toggle_ { "Keep my selected output" };
    ToggleButton preset_cycling_ { "Auto preset cycling" };
    Label preset_cycling_status_;
    Label live_status_, smart_status_;
    Slider sensitivity_ { Slider::LinearHorizontal, Slider::TextBoxRight };
    Slider headroom_cap_ { Slider::LinearHorizontal, Slider::TextBoxRight };
    AudioLearningTimeline learning_timeline_;
    CynNeuralActivity neural_activity_;
    CynBrainScan3D brain_scan_;
    TextButton scan_toggle_ {"Neural graph"};
    bool showNeuralGraph_ = false;
    Label audonic_intensity_label_;
    Slider audonic_intensity_ { Slider::LinearHorizontal,Slider::TextBoxRight };
    // Message-thread copies only: never fetch inference inside paint or DSP.
    CynAudioScene neural_core_scene_ {};
    bool neural_core_live_=false;
    float theme_audio_level_=0.0f;
    double last_theme_repaint_=0;
    double last_ai_controls_update_=0;
    // Entirely visual: one independent layer per real DSP panel. Four
    // independently targeted flowers per panel; hand/cursor use one per panel.
    struct DjTouchAgent {
        juce::Point<float> pos {}, target {};
        float opacity=0.0f;
        int targetId=-1;
        double lastActivity=0.0;
    };
    std::array<DjTouchAgent,8> dj_touch_agents_ {};
    std::array<int,2> dj_touch_styles_ {{0,0}};
    std::array<bool,2> dj_touch_seeded_ {{false,false}};
    int dj_touch_flower_count_=4;
    juce::Image dj_touch_image_;
    juce::Time dj_touch_image_stamp_;
    std::array<float,5> dj_touch_last_effect_ {};
    std::array<float,39> dj_touch_last_eq_ {};

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
