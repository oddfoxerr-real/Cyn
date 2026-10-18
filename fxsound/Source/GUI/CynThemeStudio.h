// Cyn Theme Studio. Native JUCE carousel, not a web view or floating mock-up.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxController.h"
#include "FxTheme.h"
#include "CynThemeAtmosphere.h"
#include "CynDjTouch.h"
#include "CynUiMotion.h"
#include "CynCloseMotion.h"
#include "CynButtonVisuals.h"
#include <cmath>

class CynThemeStudioContent final : public juce::Component, private juce::Timer
{
public:
    CynThemeStudioContent()
        : previous_("<"), next_(">"), apply_("Apply studio"), cancel_("Close"), importTouch_("Choose PNG..."),
          appearanceTab_("Appearance"), motionTab_("Animations"), replayMotion_("Replay")
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
        closeStyle_ = CynCloseMotion::clampStyle(settings.getInt(
            "cyn_close_motion_style", CynCloseMotion::defaultStyle));
        closeSpeedPercent_ = CynCloseMotion::clampSpeed(settings.getInt(
            "cyn_close_motion_speed_pct", CynCloseMotion::defaultSpeedPercent));
        customTouch_=CynDjTouch::readCustomImage();
        for (int i=0; i<CynCloseMotion::NumStyles; ++i)
            closeStylePicker_.addItem(CynCloseMotion::styleName(i), i+1);
        closeStylePicker_.setSelectedId(closeStyle_+1,juce::dontSendNotification);
        closeSpeedSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        closeSpeedSlider_.setTextBoxStyle(juce::Slider::TextBoxRight,false,93,28);
        closeSpeedSlider_.setRange(.55,1.60,.05);
        closeSpeedSlider_.setTextValueSuffix("x");
        closeSpeedSlider_.setValue(closeSpeedPercent_/100.0,juce::dontSendNotification);
        closeStylePicker_.setTooltip("Choose a close-to-tray visual: Vortex, Gravity Melt, Liquid Ripple, Soft Fade or None.");
        closeSpeedSlider_.setTooltip("Animation speed: 0.55x is slower; 1.60x is faster. Normal button reactions stay quick.");
        addAndMakeVisible(closeStylePicker_);
        addAndMakeVisible(closeSpeedSlider_);
        closeStylePicker_.onChange=[this] {
            closeStyle_=CynCloseMotion::clampStyle(closeStylePicker_.getSelectedId()-1);
            replayMotionPreview();
        };
        closeSpeedSlider_.onValueChange=[this] {
            closeSpeedPercent_=CynCloseMotion::clampSpeed(juce::roundToInt(closeSpeedSlider_.getValue()*100.0));
            replayMotionPreview();
        };
        motionPreviewStart_=juce::Time::getMillisecondCounterHiRes();
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
                &previous_, &next_, &apply_, &cancel_,
                &appearanceTab_, &motionTab_, &replayMotion_ })
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
            // Save the visual motion choice with Apply, without changing playback.
            controller.getSettings().setInt("cyn_close_motion_style",closeStyle_);
            controller.getSettings().setInt("cyn_close_motion_speed_pct",closeSpeedPercent_);
            controller.setThemeMode(static_cast<FxThemeMode>(selected_));
            close();
        };
        cancel_.onClick = [this] { close(); };
        appearanceTab_.onClick=[this] { showMotionPage(false); };
        motionTab_.onClick=[this] { showMotionPage(true); };
        replayMotion_.onClick=[this] { replayMotionPreview(); };
        appearanceTab_.setTooltip("Choose colours and AI Touch styling.");
        motionTab_.setTooltip("Choose the window-close animation style and speed.");
        replayMotion_.setTooltip("Replay the selected close animation in this preview.");
        showMotionPage(false);

        if (FxTheme::isReducedMotion()) startTimer(500);
        else startTimerHz(30);
    }

    ~CynThemeStudioContent() override { stopTimer(); }

    void resized() override
    {
        const int w=getWidth(),h=getHeight();
        appearanceTab_.setBounds(w-342,73,145,33);
        motionTab_.setBounds(w-187,73,158,33);
        closeStylePicker_.setBounds(218,574,370,33);
        closeSpeedSlider_.setBounds(207,640,390,34);
        replayMotion_.setBounds(w-225,537,125,33);
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

        if(motionPage_) {
            paintMotionPage(g);
            return;
        }

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

    void showMotionPage(bool show)
    {
        motionPage_=show;
        appearanceTab_.setToggleState(!show,juce::dontSendNotification);
        motionTab_.setToggleState(show,juce::dontSendNotification);
        for(auto* c:std::initializer_list<juce::Component*> {
            &previous_, &next_, &effectsTouchPicker_, &eqTouchPicker_,
            &flowerPicker_, &importTouch_ })
            c->setVisible(!show);
        closeStylePicker_.setVisible(show);
        closeSpeedSlider_.setVisible(show);
        replayMotion_.setVisible(show);
        if(show) replayMotionPreview();
        repaint();
    }

    void replayMotionPreview()
    {
        motionPreviewStart_=juce::Time::getMillisecondCounterHiRes();
        repaint();
    }

    void paintMotionPage(juce::Graphics& g)
    {
        using namespace juce;
        const auto fg=Colour(FXCOLOR(DefaultText)).withAlpha(1.0f);
        const auto muted=Colour(FXCOLOR(HintText)).withAlpha(1.0f);
        const auto accent=Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
        const auto backdrop=Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f);
        const juce::Rectangle<float> area(92.0f,138.0f,(float)getWidth()-184.0f,367.0f);
        const double duration=CynCloseMotion::durationForSpeed(closeSpeedPercent_);
        const double elapsed=Time::getMillisecondCounterHiRes()-motionPreviewStart_;
        // Pause between replays so even very fast styles are easy to assess.
        const double cycle=duration+950.0;
        const double time=std::fmod(juce::jmax(0.0,elapsed),cycle);
        const float t=(float)juce::jlimit(0.0,1.0,time/duration);
        const bool none=closeStyle_==CynCloseMotion::None;

        g.setColour(fg);
        g.setFont(Font(21.0f,Font::bold));
        g.drawText("WINDOW ANIMATIONS",92,112,500,26,Justification::centredLeft);
        g.setColour(muted);
        g.setFont(Font(12.0f));
        g.drawText("A live sample of the selected motion. Audio continues when the real app hides to tray.",
                   92,515,getWidth()-184,22,Justification::centredLeft);
        g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(.93f));
        g.fillRoundedRectangle(area.expanded(1.0f),18.0f);
        {
            Graphics::ScopedSaveState outer(g);
            g.reduceClipRegion(area.getSmallestIntegerContainer());
            g.setColour(backdrop.darker(.14f));
            g.fillRoundedRectangle(area,17.0f);
            const auto inside=area.reduced(17.0f);
            if (none || t<CynCloseMotion::phaseExpandEnd)
            {
                // Mock native surface uses CYN palette; no extra screenshot,
                // IO or desktop window is needed for the preview.
                g.setColour(Colour(FXCOLOR(PanelBackground)).withAlpha(1.0f));
                g.fillRoundedRectangle(inside,12.0f);
                g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(.9f));
                g.drawLine(inside.getX()+13,inside.getY()+43,
                           inside.getRight()-13,inside.getY()+43,1.0f);
                g.setColour(fg.withAlpha(.92f));
                g.setFont(Font(15.0f,Font::bold));
                g.drawText("CYN   /   SOUND STUDIO",(int)inside.getX()+22,
                           (int)inside.getY()+10,400,25,Justification::centredLeft);
                const auto button=juce::Rectangle<float>(inside.getRight()-43,inside.getY()+13,21,21);
                g.setColour(accent.withAlpha(.52f));
                g.drawRoundedRectangle(button,5.0f,1.0f);
                for(int i=0;i<8;++i)
                {
                    const float px=inside.getX()+42.0f+i*56.0f;
                    const float y=inside.getY()+162.0f;
                    const float height=35.0f+(i*19%72);
                    g.setColour(accent.withAlpha(.18f+.06f*(i%4)));
                    g.fillRoundedRectangle(px,y-height,19,height,5.0f);
                }
                g.setColour(muted);
                g.setFont(Font(12.0f));
                g.drawText("Close-button takeover preview", (int)inside.getX()+24,
                    (int)inside.getBottom()-48,400,22,Justification::centredLeft);
                if (!none)
                {
                    const float e=CynCloseMotion::expandEase(t/CynCloseMotion::phaseExpandEnd);
                    const auto expanded=juce::Rectangle<float>(
                        button.getX()+(area.getX()-button.getX())*e,
                        button.getY()+(area.getY()-button.getY())*e,
                        button.getWidth()+(area.getWidth()-button.getWidth())*e,
                        button.getHeight()+(area.getHeight()-button.getHeight())*e);
                    g.setColour(backdrop.interpolatedWith(accent,.15f));
                    g.fillRoundedRectangle(expanded,juce::jmin(13.0f,expanded.getHeight()*.24f));
                }
            }
            else if(t<CynCloseMotion::phaseHoldEnd)
            {
                g.setColour(backdrop.interpolatedWith(accent,.15f));
                g.fillRoundedRectangle(area,13.0f);
            }
            else
            {
                // Same 2D style coverage as the actual native X overlay.
                // The 96 x 54 preview mask keeps redraw inexpensive.
                constexpr int mw=96,mh=54;
                Image mask(Image::ARGB,mw,mh,true);
                const float melt=(t-CynCloseMotion::phaseHoldEnd)/(1.0f-CynCloseMotion::phaseHoldEnd);
                {
                    Image::BitmapData bitmap(mask,Image::BitmapData::writeOnly);
                    for(int y=0;y<mh;++y)
                        for(int x=0;x<mw;++x)
                        {
                            const float coverage=CynCloseMotion::coverage(closeStyle_,melt,
                                ((float)x+.5f)/mw,((float)y+.5f)/mh);
                            bitmap.setPixelColour(x,y,Colour::fromRGBA(255,255,255,
                                (uint8)roundToInt(coverage*255.0f)));
                        }
                }
                Graphics::ScopedSaveState fluid(g);
                g.reduceClipRegion(mask,AffineTransform::scale(
                    area.getWidth()/mw,area.getHeight()/mh)
                    .translated(area.getX(),area.getY()));
                g.setColour(backdrop.interpolatedWith(accent,.15f));
                g.fillRoundedRectangle(area,13.0f);
            }
        }
        g.setColour(fg);
        g.setFont(Font(13.0f,Font::bold));
        g.drawText("CLOSE STYLE",92,574,115,33,Justification::centredLeft);
        g.drawText("SPEED",92,640,100,33,Justification::centredLeft);
        g.setColour(muted);
        g.setFont(Font(12.0f));
        const auto speed=String(closeSpeedPercent_/100.0,2)+"x";
        const auto ms=String(juce::roundToInt(duration))+" ms";
        g.drawText(none ? "Instant hide - no effect" :
                   ("Speed "+speed+"  /  Approximately "+ms+" total"),
                   610,644,215,28,Justification::centredLeft);
        g.drawText("Slower: move left    •    Faster: move right.  Apply studio saves the choice.",
                   92,701,getWidth()-180,24,Justification::centredLeft);
        if(FxTheme::isReducedMotion())
            g.drawText("Reduce Motion is enabled: the actual close animation will be skipped.",
                       92,677,getWidth()-180,20,Justification::centredLeft);
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
    juce::TextButton appearanceTab_,motionTab_,replayMotion_;
    juce::ComboBox effectsTouchPicker_,eqTouchPicker_,flowerPicker_;
    juce::ComboBox closeStylePicker_;
    juce::Slider closeSpeedSlider_;
    juce::File customChoice_;
    juce::Image customTouch_;
    int effectsTouchStyle_=0,eqTouchStyle_=0,flowerCount_=4;
    int closeStyle_=CynCloseMotion::defaultStyle;
    int closeSpeedPercent_=CynCloseMotion::defaultSpeedPercent;
    bool motionPage_=false;
    double motionPreviewStart_=0.0;
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
