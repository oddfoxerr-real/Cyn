// Cyn Theme Studio. Native JUCE carousel, not a web view or floating mock-up.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxController.h"
#include "FxTheme.h"
#include "CynThemeAtmosphere.h"
#include "CynDjTouch.h"
#include "CynUiMotion.h"
#include <cmath>

class CynThemeStudioContent final : public juce::Component, private juce::Timer
{
public:
    CynThemeStudioContent()
        : previous_("<"), next_(">"), apply_("Apply studio"), cancel_("Close"), importTouch_("Choose PNG...")
    {
        setSize(900, 790);
        setOpaque(true);
        setWantsKeyboardFocus(true);
        setFocusContainerType(juce::Component::FocusContainerType::focusContainer);
        selected_ = (int)FxTheme::getThemeMode();
        source_ = selected_;
        auto& settings=FxController::getInstance().getSettings();
        const int legacyStyle=settings.getInt("cyn_dj_touch_style",0);
        effectsTouchStyle_=juce::jlimit(0,5,
            settings.getInt("cyn_dj_touch_style_effects",legacyStyle));
        eqTouchStyle_=juce::jlimit(0,5,
            settings.getInt("cyn_dj_touch_style_eq",legacyStyle));
        flowerCount_=juce::jlimit(2,4,
            settings.getInt("cyn_dj_touch_flower_count",4));
        customTouch_=CynDjTouch::readCustomImage();
        for(int i=0;i<=5;++i) {
            effectsTouchPicker_.addItem(CynDjTouch::name(i),i+1);
            eqTouchPicker_.addItem(CynDjTouch::name(i),i+1);
        }
        for(int i=2;i<=4;++i)
            flowerPicker_.addItem(juce::String(i)+" flowers per panel",i-1);
        effectsTouchPicker_.setSelectedId(effectsTouchStyle_+1,juce::dontSendNotification);
        eqTouchPicker_.setSelectedId(eqTouchStyle_+1,juce::dontSendNotification);
        flowerPicker_.setSelectedId(flowerCount_-1,juce::dontSendNotification);
        addAndMakeVisible(effectsTouchPicker_);
        addAndMakeVisible(eqTouchPicker_);
        addAndMakeVisible(flowerPicker_);
        addAndMakeVisible(importTouch_);
        effectsTouchPicker_.setTooltip("The left DJ hand/cursor reacts to real bass, clarity, ambience, surround and dynamics.");
        eqTouchPicker_.setTooltip("The right DJ hand/cursor reacts independently to real EQ-band corrections.");
        flowerPicker_.setTooltip("Maximum independently positioned flowers in EACH panel; only actually changing controls light up.");
        importTouch_.setTooltip("Choose a transparent PNG (max 2 MB; 1024 x 1024). Shared asset, independent panel movement.");
        effectsTouchPicker_.onChange=[this] {
            effectsTouchStyle_=effectsTouchPicker_.getSelectedId()-1;repaint();
        };
        eqTouchPicker_.onChange=[this] {
            eqTouchStyle_=eqTouchPicker_.getSelectedId()-1;repaint();
        };
        flowerPicker_.onChange=[this] {
            flowerCount_=juce::jlimit(2,4,flowerPicker_.getSelectedId()+1);repaint();
        };
        importTouch_.onClick=[this] {chooseCustomTouch();};

        for (auto* control : std::initializer_list<juce::Button*> {
                &previous_, &next_, &apply_, &cancel_ })
        {
            addAndMakeVisible(*control);
            control->setMouseCursor(juce::MouseCursor::PointingHandCursor);
            control->setWantsKeyboardFocus(true);
        }

        previous_.setName("Previous theme");
        next_.setName("Next theme");
        previous_.setTooltip("Previous theme (Left arrow)");
        next_.setTooltip("Next theme (Right arrow)");
        apply_.setTooltip("Apply the selected theme throughout Cyn");
        cancel_.setTooltip("Close without changing the current theme");

        previous_.onClick = [this] { selectRelative(-1); };
        next_.onClick = [this] { selectRelative(1); };
        apply_.onClick = [this] {
            if((effectsTouchStyle_==CynDjTouch::CustomPng ||
                eqTouchStyle_==CynDjTouch::CustomPng) && !customTouch_.isValid()) {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Custom PNG needed","Choose a PNG or select another AI Touch style.");
                return;
            }
            if(customChoice_.existsAsFile()) {
                const auto target=CynDjTouch::customImageFile();
                if(!target.getParentDirectory().createDirectory().wasOk() ||
                   (customChoice_!=target && (!target.deleteFile() && target.existsAsFile() ||
                                              !customChoice_.copyFileTo(target)))) {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        "Couldn't save image","Check local storage permissions.");
                    return;
                }
            }
            auto& controller=FxController::getInstance();
            // Two persisted visual layers, independent of color themes.
            controller.getSettings().setInt("cyn_dj_touch_style_effects",effectsTouchStyle_);
            controller.getSettings().setInt("cyn_dj_touch_style_eq",eqTouchStyle_);
            controller.getSettings().setInt("cyn_dj_touch_flower_count",flowerCount_);
            controller.getSettings().setInt("cyn_dj_touch_style",effectsTouchStyle_);
            controller.setThemeMode(static_cast<FxThemeMode>(selected_));
            close();
        };
        cancel_.onClick = [this] { close(); };

        if (FxTheme::isReducedMotion()) startTimer(500);
        else startTimerHz(30);
    }

    ~CynThemeStudioContent() override { stopTimer(); }

    void resized() override
    {
        const int w=getWidth(),h=getHeight();
        previous_.setBounds(25,225,52,64);
        next_.setBounds(w-77,225,52,64);
        cancel_.setBounds(w-255,h-58,105,36);
        apply_.setBounds(w-148,h-58,123,36);
        effectsTouchPicker_.setBounds(165,565,255,32);
        eqTouchPicker_.setBounds(554,565,270,32);
        flowerPicker_.setBounds(165,616,206,32);
        importTouch_.setBounds(390,616,144,32);
        preview_ = juce::Rectangle<float>(92.0f,116.0f,(float)w-184.0f,315.0f);
        repaint();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if(key.getKeyCode()==juce::KeyPress::leftKey) { selectRelative(-1); return true; }
        if(key.getKeyCode()==juce::KeyPress::rightKey){ selectRelative(1); return true; }
        if(key.getKeyCode()==juce::KeyPress::returnKey){ apply_.triggerClick(); return true; }
        return juce::Component::keyPressed(key);
    }

    void visibilityChanged() override
    {
        if(isShowing()) grabKeyboardFocus();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace juce;
        const auto background=Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f);
        const auto text=Colour(FXCOLOR(DefaultText)).withAlpha(1.0f);
        const auto muted=Colour(FXCOLOR(HintText)).withAlpha(1.0f);
        const auto accent=Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
        g.fillAll(background);
        ColourGradient header(background.brighter(.055f),0,0,
                              background.darker(.09f),(float)getWidth(),(float)getHeight(),false);
        g.setGradientFill(header);
        g.fillRect(getLocalBounds().toFloat());

        g.setColour(text);
        g.setFont(Font(26.0f,Font::bold));
        g.drawText("Theme Studio",25,17,getWidth()-50,37,Justification::centredLeft);
        g.setFont(Font(13.0f));
        g.setColour(muted);
        g.drawText("Explore your themes. Use the arrows to slide between live previews.",
                   26,56,getWidth()-52,24,Justification::centredLeft);

        const int total=(int)FxThemeMode::NumModes;
        const String counter=String(selected_+1).paddedLeft('0',2)+" / "+String(total);
        g.setColour(accent);
        g.setFont(Font(11.0f,Font::bold));
        g.drawText(counter,getWidth()-132,25,107,20,Justification::centredRight);

        g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(1.0f));
        g.fillRoundedRectangle(preview_.expanded(1.0f),18.0f);
        {
            Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(preview_.getSmallestIntegerContainer());

            float progress=1.0f;
            if(transitioning_ && !FxTheme::isReducedMotion())
            {
                const float elapsed=(float)(Time::getMillisecondCounterHiRes()-transitionStart_);
                progress=CynUiMotion::easeOutCubic(
                    CynUiMotion::progress(elapsed,CynUiMotion::themeCarouselDurationMs));
            }
            const float stride=preview_.getWidth()+19.0f;
            const float direction=(float)slideDirection_;
            const double time=FxTheme::isReducedMotion()?0.0:Time::getMillisecondCounterHiRes()*.001;
            if(progress<1.0f)
                paintPreview(g,static_cast<FxThemeMode>(source_),
                    preview_.translated(-direction*stride*progress,0.0f),(float)time);
            paintPreview(g,static_cast<FxThemeMode>(selected_),
                preview_.translated(direction*stride*(1.0f-progress),0.0f),(float)time);
        }

        g.setColour(text);
        g.setFont(Font(20.0f,Font::bold));
        g.drawText(FxTheme::getThemeName(selected_),92,444,getWidth()-184,28,
                   Justification::centred);
        g.setFont(Font(12.0f));
        g.setColour(muted);
        g.drawText(selected_==(int)FxTheme::getThemeMode() ?
                   "CURRENT THEME  /  CHOOSE ANOTHER TO PREVIEW" :
                   "PREVIEW ONLY  /  APPLY TO SAVE THIS THEME",
                   92,474,getWidth()-184,20,Justification::centred);

        // Two real DJ hands/flower groups; styles are independent per panel.
        g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(.65f));
        g.drawHorizontalLine(540,25.0f,(float)getWidth()-25.0f);
        g.setColour(text);
        g.setFont(Font(14.0f,Font::bold));
        g.drawText("AI TOUCH / DJ PERFORMERS",35,542,485,19,Justification::centredLeft);
        g.setFont(Font(12.0f,Font::bold));
        g.drawText("EFFECTS",35,569,124,24,Justification::centredLeft);
        g.drawText("EQ PANEL",452,569,98,24,Justification::centredLeft);
        g.drawText("FLOWERS",35,620,124,24,Justification::centredLeft);
        g.setColour(muted);
        g.setFont(Font(11.0f));
        g.drawText("Two independent hands: left handles effects, right handles EQ.",35,675,730,22,
                   Justification::centredLeft);
        g.drawText("Each flower follows a different REAL changing knob or band; None adds no visuals.",35,700,780,22,
                   Justification::centredLeft);
        const float touchPhase=(float)(Time::getMillisecondCounterHiRes()*0.001);
        const auto sample=[&](int style,float x,bool right) {
            const int number=style==CynDjTouch::Flowers ? flowerCount_:1;
            for(int i=0;i<number;++i)
                CynDjTouch::paint(g,style,
                    {x+(style==CynDjTouch::Flowers ? (i-1.5f)*27.0f:0.0f),
                     620.0f+(style==CynDjTouch::Flowers ? (i%2)*15.0f:0.0f)},
                    0.78f,customTouch_,touchPhase+i*0.4f,right);
        };
        sample(effectsTouchStyle_,585.0f,false);
        sample(eqTouchStyle_,786.0f,true);
        // Progress markers are deliberately subtle and scale to all 20 presets.
        const float dotW=9.5f,space=19.0f;
        const float start=(getWidth()-((total-1)*space+dotW))*.5f;
        for(int i=0;i<total;++i)
        {
            g.setColour((i==selected_?accent:muted).withAlpha(i==selected_?1.0f:.33f));
            g.fillEllipse(start+i*space,509.0f,i==selected_?dotW:5.5f,
                          i==selected_?dotW:5.5f);
        }
    }

private:
    void paintPreview(juce::Graphics& g,FxThemeMode mode,
                      juce::Rectangle<float> area,float time)
    {
        using namespace juce;
        CynThemeAtmosphere::paint(g,mode,time,0.35f,area,true);

        const auto foreground=CynThemeAtmosphere::palette(mode,DefaultText);
        const auto muted=CynThemeAtmosphere::palette(mode,HintText);
        const auto accent=CynThemeAtmosphere::palette(mode,SliderTrack);
        const auto panel=CynThemeAtmosphere::palette(mode,PanelBackground);
        const float x=area.getX(),y=area.getY(),w=area.getWidth(),h=area.getHeight();

        // A native material sample makes previews describe the real UI,
        // not just abstract animated wallpaper.
        g.setColour(panel.withAlpha(.84f));
        g.fillRoundedRectangle(x+24,y+26,238,45,11);
        g.setColour(foreground);
        g.setFont(Font(15.0f,Font::bold));
        g.drawText("CYN / AUDIO",(int)(x+40),(int)(y+36),197,25,
                   Justification::centredLeft,false);
        g.setColour(panel.withAlpha(.79f));
        g.fillRoundedRectangle(x+24,y+h-105,w-48,74,13);
        g.setColour(muted);
        g.setFont(Font(10.0f,Font::bold));
        g.drawText("THEME PREVIEW",(int)(x+45),(int)(y+h-95),300,19,
                   Justification::centredLeft,false);
        g.setColour(foreground);
        g.setFont(Font(22.0f,Font::bold));
        g.drawText(FxTheme::getThemeName((int)mode),(int)(x+45),
                   (int)(y+h-77),(int)w-90,35,Justification::centredLeft,false);
        for(int i=0;i<8;++i)
        {
            const float bx=x+w-160+i*15;
            const float bh=16+(float)((i*17)%31);
            g.setColour(accent.withAlpha(.32f+.055f*(i%5)));
            g.fillRoundedRectangle(bx,y+h-47-bh,7,bh,3);
        }
        g.setColour(accent.withAlpha(.71f));
        g.drawRoundedRectangle(area.reduced(.8f),16,1.2f);
    }

    void chooseCustomTouch()
    {
        juce::FileChooser chooser("Choose an AI Touch PNG", juce::File::getSpecialLocation(juce::File::userPicturesDirectory), "*.png");
        if(!chooser.browseForFileToOpen()) return;
        const auto file=chooser.getResult();
        if(!file.hasFileExtension(".png") || file.getSize()>2*1024*1024) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"PNG too large","Use a PNG of 2 MB or less.");
            return;
        }
        const auto image=juce::ImageFileFormat::loadFrom(file);
        if(!image.isValid() || image.getWidth()>1024 || image.getHeight()>1024) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Invalid PNG","Use an image up to 1024 x 1024 pixels.");
            return;
        }
        customChoice_=file;
        customTouch_=image;
        effectsTouchStyle_=CynDjTouch::CustomPng;
        eqTouchStyle_=CynDjTouch::CustomPng;
        effectsTouchPicker_.setSelectedId(effectsTouchStyle_+1,juce::dontSendNotification);
        eqTouchPicker_.setSelectedId(eqTouchStyle_+1,juce::dontSendNotification);
        repaint();
    }

    void selectRelative(int direction)
    {
        if(direction==0)return;
        const int size=(int)FxThemeMode::NumModes;
        source_=selected_;
        selected_=(selected_+direction+size)%size;
        slideDirection_=direction;
        transitioning_=!FxTheme::isReducedMotion();
        transitionStart_=juce::Time::getMillisecondCounterHiRes();
        repaint();
    }

    void timerCallback() override
    {
        if(!isShowing())return;
        if(transitioning_ &&
           juce::Time::getMillisecondCounterHiRes()-transitionStart_>=CynUiMotion::themeCarouselDurationMs)
        {
            transitioning_=false;
            source_=selected_;
        }
        repaint();
    }

    void close()
    {
        if(auto* dialog=findParentComponentOfClass<juce::DialogWindow>())
            dialog->exitModalState(0);
    }

    juce::TextButton previous_,next_,apply_,cancel_,importTouch_;
    juce::ComboBox effectsTouchPicker_,eqTouchPicker_,flowerPicker_;
    juce::File customChoice_;
    juce::Image customTouch_;
    int effectsTouchStyle_=0,eqTouchStyle_=0,flowerCount_=4;
    juce::Rectangle<float> preview_;
    int selected_=0,source_=0,slideDirection_=1;
    bool transitioning_=false;
    double transitionStart_=0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynThemeStudioContent)
};

struct CynThemeStudioDialog
{
    static void show(juce::Component* owner=nullptr)
    {
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(new CynThemeStudioContent());
        options.dialogTitle="Cyn Theme Studio";
        options.componentToCentreAround=owner;
        options.dialogBackgroundColour=juce::Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f);
        options.escapeKeyTriggersCloseButton=true;
        options.useNativeTitleBar=false;
        options.resizable=false;
        options.launchAsync();
    }
};
