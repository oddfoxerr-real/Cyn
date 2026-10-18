// CYN close-to-tray motion: X control expands, fills the window, then vortex-liquefies.
// Art direction informed by the offline 2D vortex simulation. No runtime fluid solver,
// audio processing, DSP calls, background threads, or full-resolution pixel warping.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <JuceHeader.h>
#include "FxTheme.h"
#include "CynCloseMotion.h"
#include <cmath>

class CynLiquidCloseOverlay final : public juce::Component, private juce::Timer
{
public:
    // Quick and decisive like a native desktop interaction, not a delayed shrink.
    static constexpr double durationMs = 460.0;
    static constexpr float phaseExpandEnd = 0.28f; // 129 ms
    static constexpr float phaseHoldEnd = 0.37f;   // 41 ms
    static constexpr int maskWidth = 192;
    static constexpr int maskHeight = 108;

    CynLiquidCloseOverlay()
    {
        setName("CYN vortex close-to-tray");
        setOpaque(false);
        setInterceptsMouseClicks(false, false);
        setVisible(false);
    }

    ~CynLiquidCloseOverlay() override { cancel(); }

    bool begin(const juce::Image& frame,
               juce::Rectangle<int> screenBounds,
               juce::Rectangle<float> closeButtonBounds,
               int motionStyle, int speedPercent)
    {
        cancel();
        if (!frame.isValid() || screenBounds.isEmpty()
            || closeButtonBounds.isEmpty() || FxTheme::isReducedMotion()
            || CynCloseMotion::clampStyle(motionStyle)==CynCloseMotion::None)
            return false;

        style_ = CynCloseMotion::clampStyle(motionStyle);
        durationMs_ = CynCloseMotion::durationForSpeed(speedPercent);
        snapshot_ = frame;
        // The source is the actual X control, not a guessed corner or a
        // centre point that would cause the window to shrink into the X.
        buttonBounds_ = closeButtonBounds.getIntersection(
            juce::Rectangle<float>(0.0f, 0.0f,
                                   (float)screenBounds.getWidth(),
                                   (float)screenBounds.getHeight()));
        if (buttonBounds_.isEmpty())
        {
            snapshot_ = juce::Image();
            return false;
        }

        vortexMask_ = juce::Image(juce::Image::ARGB, maskWidth, maskHeight, true);
        setBounds(screenBounds);
        addToDesktop(juce::ComponentPeer::windowIsTemporary
                   | juce::ComponentPeer::windowIgnoresMouseClicks
                   | juce::ComponentPeer::windowIgnoresKeyPresses
                   | juce::ComponentPeer::windowIsSemiTransparent);
        startedAt_ = juce::Time::getMillisecondCounterHiRes();
        running_ = true;
        setVisible(true);
        toFront(false);
        startTimerHz(60);
        repaint();
        return true;
    }

    bool isAnimating() const noexcept { return running_; }

    void cancel()
    {
        stopTimer();
        running_ = false;
        setVisible(false);
        if (isOnDesktop()) removeFromDesktop();
        vortexMask_ = juce::Image();
        snapshot_ = juce::Image();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace juce;
        if (!running_ || !snapshot_.isValid()) return;

        const auto elapsed = Time::getMillisecondCounterHiRes() - startedAt_;
        const float t = jlimit(0.0f, 1.0f, (float)(elapsed / durationMs_));

        if (t < phaseExpandEnd)
        {
            // Preserve the genuine CYN UI while its real X button takes over.
            // The capture disappears only after the button covers EVERY pixel.
            g.drawImageAt(snapshot_, 0, 0);
            const float e = easeOutQuint(t / phaseExpandEnd);
            const auto screen = getLocalBounds().toFloat();
            const juce::Rectangle<float> expanded(lerp(buttonBounds_.getX(), screen.getX(), e),
                                            lerp(buttonBounds_.getY(), screen.getY(), e),
                                            lerp(buttonBounds_.getWidth(), screen.getWidth(), e),
                                            lerp(buttonBounds_.getHeight(), screen.getHeight(), e));
            drawSurface(g, expanded, 0.0f);
            g.setColour(Colour(FXCOLOR(SliderTrack)).withAlpha((1.0f-e)*0.80f));
            g.drawRoundedRectangle(expanded.reduced(0.5f), lerp(5.0f, 14.0f, e),
                                   lerp(1.7f, 0.8f, e));
            if (e < 0.40f)
            {
                // At the start the expanding surface still visibly belongs to X.
                g.setColour(Colours::white.withAlpha((0.4f-e)*2.0f));
                const auto centre = buttonBounds_.getCentre();
                const float r = 3.5f + e*2.0f;
                g.drawLine(centre.x-r, centre.y-r, centre.x+r, centre.y+r, 1.5f);
                g.drawLine(centre.x+r, centre.y-r, centre.x-r, centre.y+r, 1.5f);
            }
            return;
        }

        if (t < phaseHoldEnd)
        {
            // Short visual punctuation: button is now the entire app surface.
            drawSurface(g, getLocalBounds().toFloat(), (t-phaseExpandEnd)/
                         (phaseHoldEnd-phaseExpandEnd));
            return;
        }

        // Vortex study: the full-sized surface becomes fluid. Use a tiny
        // 192x108 alpha/velocity-inspired field, not a costly 1200px+ solver.
        // The underlying screenshot is NOT repainted here, preventing the UI
        // from flashing back as liquid becomes transparent.
        const float melt = jlimit(0.0f, 1.0f,
            (t-phaseHoldEnd)/(1.0f-phaseHoldEnd));
        renderVortexMask(melt);
        {
            Graphics::ScopedSaveState clipped(g);
            g.reduceClipRegion(vortexMask_,
                AffineTransform::scale((float)getWidth()/maskWidth,
                                       (float)getHeight()/maskHeight));
            drawSurface(g, getLocalBounds().toFloat(), melt);
            if (style_ == CynCloseMotion::Vortex)
                drawVortexHighlights(g, melt);
        }
        // Fine, short-lived detached particles, with no random allocations.
        if (style_ == CynCloseMotion::Vortex && melt > .08f && melt < .88f)
        {
            const float fade = std::sin(MathConstants<float>::pi*melt);
            const auto accent = Colour(FXCOLOR(SliderTrack)).withAlpha(.36f*fade);
            g.setColour(accent);
            const float cx = getWidth()*.56f, cy = getHeight()*.48f;
            for (int i=0; i<12; ++i)
            {
                const float a = i*2.399963f + melt*10.5f;
                const float r = (1.0f-melt)*(.12f + .045f*(i%5))*getWidth();
                const float x = cx + std::cos(a)*r;
                const float y = cy + std::sin(a)*r*.52f;
                const float size = 1.0f+(i%3)*.75f;
                g.fillEllipse(x-size,y-size,size*2.0f,size*2.0f);
            }
        }
    }

private:
    static float lerp(float a,float b,float t) noexcept { return a+(b-a)*t; }
    static float easeOutQuint(float t) noexcept
    {
        t=juce::jlimit(0.0f,1.0f,t);
        const float q=1.0f-t;
        return 1.0f-q*q*q*q*q;
    }
    static float smoothstep(float lo,float hi,float value) noexcept
    {
        const float t=juce::jlimit(0.0f,1.0f,(value-lo)/(hi-lo));
        return t*t*(3.0f-2.0f*t);
    }

    void drawSurface(juce::Graphics& g, juce::Rectangle<float> bounds,
                     float pulse) const
    {
        using namespace juce;
        // Solid, opaque window-cover colour first; gradient adds controlled
        // depth, never the gray or overwhelming neon-blue flash.
        const auto base = Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f);
        const auto accent = Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
        const auto mid = base.interpolatedWith(accent, .12f+.045f*pulse);
        ColourGradient finish(base.interpolatedWith(accent,.18f),bounds.getX(),
                              bounds.getY(),base,bounds.getRight(),
                              bounds.getBottom(),false);
        finish.addColour(.44,mid);
        g.setGradientFill(finish);
        g.fillRoundedRectangle(bounds, juce::jmin(14.0f,bounds.getHeight()*.23f));

        g.setColour(accent.withAlpha(.10f+.09f*pulse));
        g.drawRoundedRectangle(bounds.reduced(.5f),
            juce::jmin(14.0f,bounds.getHeight()*.23f),.8f);
    }

    // The style preview and the native overlay share exactly the same
    // bounded field, so preview choice is faithful to the rendered effect.
    void renderVortexMask(float progress)
    {
        using namespace juce;
        Image::BitmapData bitmap(vortexMask_,Image::BitmapData::writeOnly);
        for (int y=0; y<maskHeight; ++y)
            for (int x=0; x<maskWidth; ++x)
            {
                const float alpha=CynCloseMotion::coverage(style_,progress,
                    ((float)x+.5f)/maskWidth,((float)y+.5f)/maskHeight);
                bitmap.setPixelColour(x,y,Colour::fromRGBA(255,255,255,
                    (uint8)roundToInt(jlimit(0.0f,1.0f,alpha)*255.0f)));
            }
    }

    void drawVortexHighlights(juce::Graphics& g,float melt) const
    {
        using namespace juce;
        const float fade=std::sin(MathConstants<float>::pi*melt);
        if(fade<=.01f) return;
        g.setColour(Colour(FXCOLOR(SliderTrack)).withAlpha(.10f*fade));
        const auto cx=getWidth()*.56f,cy=getHeight()*.48f;
        for(int strand=0;strand<4;++strand)
        {
            Path path;
            for(int step=0;step<50;++step)
            {
                const float p=(float)step/49.0f;
                const float r=(.10f+.65f*p)*(1.0f-melt)*getHeight();
                const float a=p*9.5f+melt*10.0f+strand*1.570796f;
                const float x=cx+std::cos(a)*r*1.35f;
                const float y=cy+std::sin(a)*r*.68f;
                if(step==0)path.startNewSubPath(x,y);
                else path.lineTo(x,y);
            }
            g.strokePath(path,PathStrokeType(.8f));
        }
    }

    void timerCallback() override
    {
        if (juce::Time::getMillisecondCounterHiRes()-startedAt_ >= durationMs_)
            cancel();
        else
            repaint();
    }

    juce::Image snapshot_;
    juce::Image vortexMask_;
    juce::Rectangle<float> buttonBounds_;
    int style_ = CynCloseMotion::defaultStyle;
    double durationMs_ = durationMs;
    double startedAt_=0.0;
    bool running_=false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynLiquidCloseOverlay)
};
