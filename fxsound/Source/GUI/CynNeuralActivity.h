// Cyn neural-activity visualization. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxTheme.h"
#include <array>
#include <cmath>

// Lightweight, decorative display of the REAL Cyn model topology: 11 -> 12 -> 5.
// Animated pulses visualize training state; they are NOT individual neuron telemetry.
// This component never touches the DSP/audio callback or stores learning examples.
class CynNeuralActivity : public juce::Component, private juce::Timer
{
public:
    CynNeuralActivity()
    {
        setInterceptsMouseClicks(false, false);
        setOpaque(false);
    }

    ~CynNeuralActivity() override { stopTimer(); }

    void setTrainingActive(bool isTraining)
    {
        if (training_ == isTraining)
            return;

        if (training_ && !isTraining)
            completionUntil_ = juce::Time::getMillisecondCounterHiRes() + 1900.0;
        if (isTraining)
            completionUntil_ = 0.0;

        training_ = isTraining;
        repaint();
    }

    void setAnimationEnabled(bool enabled)
    {
        const bool animate = enabled && !FxTheme::isReducedMotion();
        if (animate == animating_)
            return;
        animating_ = animate;
        if (animating_)
            startTimerHz(24);
        else
            stopTimer();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace juce;

        const auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() < 64.0f || bounds.getHeight() < 90.0f)
            return;

        const Colour accent(0xffefb963);
        const Colour highlight(0xffffdb9b);
        const Colour inactiveText(FXCOLOR(HintText));
        const Colour frame(FXCOLOR(RowOutline));
        const Colour surface(FXCOLOR(ControlBackground));
        const bool complete = !training_ &&
            Time::getMillisecondCounterHiRes() < completionUntil_;
        const float t = animating_
            ? (float) (Time::getMillisecondCounterHiRes() * 0.001)
            : 0.0f;
        const float pace = training_ ? 0.72f : 0.14f;

        g.setColour(surface.withAlpha(1.0f));
        g.fillRoundedRectangle(bounds.reduced(0.5f), 12.0f);
        g.setColour(frame.withAlpha(0.7f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 12.0f, 1.0f);

        g.setColour(inactiveText.withAlpha(1.0f));
        g.setFont(Font(10.0f, Font::bold));
        g.drawText("NEURAL ACTIVITY", 12, 10, getWidth() - 24, 16,
                   Justification::centredLeft, false);

        // The 11 input, 12 hidden and 5 output nodes are rendered as
        // successive top-to-bottom layers. No fabricated training percentage.
        const float left = 15.0f, right = (float)getWidth() - 15.0f;
        const float graphTop = 47.0f;
        const float graphBottom = (float)getHeight() - 57.0f;
        std::array<Point<float>, 11> inputs {};
        std::array<Point<float>, 12> hidden {};
        std::array<Point<float>, 5> outputs {};
        for (size_t i = 0; i < inputs.size(); ++i)
            inputs[i] = { jmap((float)i, 0.0f, 10.0f, left, right), graphTop };
        for (size_t i = 0; i < hidden.size(); ++i)
            hidden[i] = { jmap((float)i, 0.0f, 11.0f, left + 3.0f, right - 3.0f),
                          graphTop + (graphBottom - graphTop) * 0.51f };
        for (size_t i = 0; i < outputs.size(); ++i)
            outputs[i] = { jmap((float)i, 0.0f, 4.0f, left + 17.0f, right - 17.0f),
                           graphBottom };

        // All 192 directed neural connections, drawn with subtly varied alpha.
        auto connections = [&g, accent] (const auto& from, const auto& to)
        {
            for (size_t i = 0; i < from.size(); ++i)
                for (size_t j = 0; j < to.size(); ++j)
                {
                    const float variation = (float)((i * 7 + j * 11) % 9) / 8.0f;
                    g.setColour(accent.withAlpha(0.075f + variation * 0.10f));
                    g.drawLine(Line<float>(from[i], to[j]), 0.65f);
                }
        };
        connections(inputs, hidden);
        connections(hidden, outputs);

        // A limited number of travelling signals keep CPU use low.
        // They are an illustrative activity animation, not measured neuron spikes.
        auto pulses = [&] (const auto& from, const auto& to,
                          int count, int offset)
        {
            for (int i = 0; i < count; ++i)
            {
                const auto& a = from[(size_t)(i * 5 + offset) % from.size()];
                const auto& b = to[(size_t)(i * 7 + 3 + offset) % to.size()];
                const float position = std::fmod(t * pace + (float)i * 0.173f
                                                 + (float)offset * 0.109f, 1.0f);
                const Point<float> dot = a + (b - a) * position;
                const float strength = training_ ? 0.9f : 0.4f;
                g.setColour(accent.withAlpha(0.30f * strength));
                g.drawLine(Line<float>(a, b), 1.0f);
                g.setColour(highlight.withAlpha(0.18f * strength));
                g.fillEllipse(dot.x - 5.0f, dot.y - 5.0f, 10.0f, 10.0f);
                g.setColour(highlight.withAlpha(strength));
                g.fillEllipse(dot.x - 1.8f, dot.y - 1.8f, 3.6f, 3.6f);
            }
        };
        pulses(inputs, hidden, training_ ? 12 : 3, 0);
        pulses(hidden, outputs, training_ ? 8 : 2, 7);

        auto neurons = [&] (const auto& points, float delay)
        {
            for (size_t i = 0; i < points.size(); ++i)
            {
                const auto& p = points[i];
                const float wave = 0.5f + 0.5f *
                    std::sin(t * (training_ ? 4.2f : 1.15f)
                             - (float)i * 0.78f - delay);
                const float glow = training_ ? 0.19f + wave * 0.45f
                                             : 0.10f + wave * 0.12f;
                g.setColour(accent.withAlpha(glow * 0.43f));
                g.fillEllipse(p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);
                g.setColour(accent.withAlpha(0.46f + glow * 0.45f));
                g.fillEllipse(p.x - 2.8f, p.y - 2.8f, 5.6f, 5.6f);
                g.setColour(highlight.withAlpha(0.58f + glow * 0.42f));
                g.fillEllipse(p.x - 1.2f, p.y - 1.2f, 2.4f, 2.4f);
            }
        };
        neurons(inputs, 0.0f);
        neurons(hidden, 1.1f);
        neurons(outputs, 2.3f);

        const auto status = training_ ? "TRAINING" :
                            complete ? "MODEL UPDATED" : "READY TO LEARN";
        g.setColour((training_ || complete ? accent : inactiveText).withAlpha(1.0f));
        g.setFont(Font(10.0f, Font::bold));
        g.drawText(status, 10, getHeight() - 38, getWidth() - 20, 14,
                   Justification::centred, false);
        g.setColour(inactiveText.withAlpha(0.92f));
        g.setFont(Font(9.0f));
        g.drawText("11  /  12  /  5", 8, getHeight() - 23,
                   getWidth() - 16, 13, Justification::centred, false);
    }

private:
    void timerCallback() override
    {
        if (isShowing())
            repaint();
    }

    bool training_ = false;
    bool animating_ = false;
    double completionUntil_ = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynNeuralActivity)
};
