// CYN window-to-tray liquid collapse.
// Captures the already-rendered JUCE window once and distorts that image.
// No fluid solver, audio capture, DSP access, GPU shaders or background work.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <JuceHeader.h>
#include "FxTheme.h"
#include <cmath>

class CynLiquidCloseOverlay final : public juce::Component, private juce::Timer
{
public:
    static constexpr double durationMs = 390.0;

    CynLiquidCloseOverlay()
    {
        setName("CYN liquid close animation");
        setOpaque(false);
        setInterceptsMouseClicks(false, false);
        setVisible(false);
    }

    ~CynLiquidCloseOverlay() override { cancel(); }

    bool begin(const juce::Image& frame,
               juce::Rectangle<int> screenBounds,
               juce::Point<float> closeButtonCentre)
    {
        cancel();
        if (!frame.isValid() || screenBounds.isEmpty() ||
            FxTheme::isReducedMotion())
            return false;

        snapshot_ = frame;
        origin_ = {
            juce::jlimit(0.0f, (float)screenBounds.getWidth(), closeButtonCentre.x),
            juce::jlimit(0.0f, (float)screenBounds.getHeight(), closeButtonCentre.y)
        };
        setBounds(screenBounds);

        // The main window can now be removed from the desktop. This separate,
        // click-through layered peer continues rendering above the desktop.
        // No black background rectangle is left behind during the collapse.
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
        snapshot_ = juce::Image();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace juce;
        if (!running_ || !snapshot_.isValid())
            return;

        const float raw = jlimit(0.0f, 1.0f,
            (float)((Time::getMillisecondCounterHiRes() - startedAt_) / durationMs));
        const float eased = raw * raw * (3.0f - 2.0f * raw);
        const float scale = jmax(0.002f, 1.0f - eased * 0.998f);
        const float rippleEnvelope = std::sin(MathConstants<float>::pi * raw);
        const float w = (float)getWidth(), h = (float)getHeight();

        // Warp a cached frame into the actual X-button position using thin
        // horizontal ribbons. Adjacent ribbons have a phase-shifted fluid
        // displacement; the final collapse contracts to one point.
        constexpr int ribbonCount = 64;
        g.setImageResamplingQuality(Graphics::lowResamplingQuality);
        g.setOpacity(jlimit(0.0f, 1.0f, (1.0f - raw * raw) * 1.1f));
        for (int ribbon = 0; ribbon < ribbonCount; ++ribbon)
        {
            const int fromY = snapshot_.getHeight() * ribbon / ribbonCount;
            const int nextY = snapshot_.getHeight() * (ribbon + 1) / ribbonCount;
            const int sourceHeight = nextY - fromY;
            if (sourceHeight <= 0) continue;

            const float fraction = ((float)ribbon + 0.5f) / ribbonCount;
            const float y = fraction * h;
            const float displacement =
                20.0f * rippleEnvelope * std::sin(fraction * 19.0f - raw * 11.0f)
                * (0.55f + 0.45f * std::abs((y - origin_.y) / jmax(1.0f, h)));
            const float left = origin_.x * (1.0f - scale) + displacement;
            const float top = origin_.y + (y - origin_.y) * scale
                + 9.0f * rippleEnvelope * std::sin(fraction * 12.0f + raw * 8.0f);
            const int destWidth = jmax(1, roundToInt(w * scale));
            const int destHeight = jmax(1, roundToInt(h * scale / ribbonCount) + 2);

            g.drawImage(snapshot_, roundToInt(left), roundToInt(top - destHeight * 0.5f),
                        destWidth, destHeight, 0, fromY,
                        snapshot_.getWidth(), sourceHeight);
        }

        // A handful of short-lived droplets gives the edge a fluid breakup
        // without allocating particles or running an actual physical solver.
        if (raw > 0.18f && raw < 0.88f)
        {
            const float dropletFade =
                rippleEnvelope * jlimit(0.0f, 1.0f, (0.88f - raw) * 5.0f);
            g.setOpacity(1.0f);
            g.setColour(Colour(FXCOLOR(SliderTrack)).withAlpha(0.25f * dropletFade));
            for (int i = 0; i < 6; ++i)
            {
                const float t = (float)i / 6.0f;
                const float swirl = raw * MathConstants<float>::twoPi * 1.35f + t * 7.0f;
                const float radius = (1.0f - eased) * (16.0f + i * 14.0f);
                const float x = origin_.x + radius * std::cos(swirl);
                const float y = origin_.y + radius * std::sin(swirl) * 0.58f;
                const float dot = 1.5f + (float)(i % 3);
                g.fillEllipse(x - dot, y - dot, dot * 2.0f, dot * 2.0f);
            }
        }
    }

private:
    void timerCallback() override
    {
        if (juce::Time::getMillisecondCounterHiRes() - startedAt_ >= durationMs)
            cancel();
        else
            repaint();
    }

    juce::Image snapshot_;
    juce::Point<float> origin_;
    double startedAt_ = 0.0;
    bool running_ = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynLiquidCloseOverlay)
};
