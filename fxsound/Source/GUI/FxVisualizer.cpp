/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
    www.theremino.com (2025)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FxVisualizer.h"
#include "FxController.h"
#include "FxTheme.h"

FxVisualizer::FxVisualizer()
{
    band_values_.resize(FxController::NUM_SPECTRUM_BANDS);
    band_graph_.resize(FxController::NUM_SPECTRUM_BANDS * NUM_BARS);

#if JUCE_MAJOR_VERSION >= 8
    start();
#else
    calcGradient();
    reset();
    setFramesPerSecond(10);
#endif

    setOpaque(false);
    setSize(WIDTH, HEIGHT);
}

void FxVisualizer::start()
{
    calcGradient();

#if JUCE_MAJOR_VERSION >= 8
    if (vblank_listener_ == nullptr)
    {
        vblank_listener_ = std::make_unique<juce::VBlankAttachment>(
            this,
            [this](double timestamp)
            {
                if (!isShowing())
                    return;

                static double last_frame_time = 0.0;
                constexpr double fps_interval = 1.0 / 30.0;

                if (timestamp - last_frame_time >= fps_interval)
                {
                    last_frame_time = timestamp;
                    if (FxController::getInstance().isAudioProcessing())
                    {
                        update();
                        repaint();
                    }
                    else
                    {
                        reset();
                        repaint();
                        vblank_listener_.reset();
                    }
                }
            });
    }
#else
    setFramesPerSecond(FxTheme::isReducedMotion()?10:30);
#endif
}

void FxVisualizer::pause()
{
    calcGradient();

#if JUCE_MAJOR_VERSION >= 8
    reset();
    repaint();

    if (vblank_listener_ != nullptr)
    {
        vblank_listener_.reset();
    }
#else
    setFramesPerSecond(10);
#endif
}

void FxVisualizer::reset()
{
    for (int i = 0; i < FxController::NUM_SPECTRUM_BANDS * NUM_BARS; i++)
    {
        band_graph_.set(i, 0);
    }
}

void FxVisualizer::update()
{
    if (!isEnabled())
        return;

    FxController::getInstance().getSpectrumBandValues(band_values_);

    for (int i = 0; i < FxController::NUM_SPECTRUM_BANDS; i++)
    {
        if (!std::isfinite(band_values_[i]) || band_values_[i] < 0 || band_values_[i] > 1)
        {
            band_values_.set(i, 0);
        }

        for (int j = 0; j < NUM_BARS / 2; j++)
        {
            band_graph_.set(i*NUM_BARS + j, band_graph_[i*NUM_BARS + j + 1]);
            band_graph_.set(i*NUM_BARS + (NUM_BARS - 1) - j, band_graph_[i*NUM_BARS + j + 1]);
        }

        const int centre=i*NUM_BARS+NUM_BARS/2;
        const float current=band_graph_[centre], target=band_values_[i];
        band_graph_.set(centre,current+(target-current)*(target>current?.65f:.22f));
    }
}

void FxVisualizer::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(Colour(0xff050607).withAlpha(0.80f));
    g.fillRoundedRectangle(bounds, 18.0f);
    g.setColour(Colour(0xff18212b).withAlpha(0.72f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 18.0f, 1.0f);

    // Baseline gives the spectrum the same quiet, technical feel as the
    // Wallpaper Engine player rather than the old solid FxSound block.
    g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(0.52f));
    g.drawLine(24.0f, bounds.getCentreY(), bounds.getRight() - 24.0f, bounds.getCentreY(), 1.0f);

    g.setGradientFill(gradient_);
    const float left = 27.0f;
    const float usable = bounds.getWidth() - 54.0f;
    const int count = FxController::NUM_SPECTRUM_BANDS * NUM_BARS;
    const float step = usable / (float)count;
    const float barWidth = jmax(2.2f, step * 0.48f);

    for (int i = 0; i < count; ++i)
    {
        const float value = band_graph_[i] == 0.0f ? 0.01f : band_graph_[i];
        const float barHeight = jlimit(2.0f, 94.0f, value * 96.0f);
        const float x = left + i * step;
        const float y = bounds.getCentreY() - barHeight * 0.5f;
        g.fillRoundedRectangle(x, y, barWidth, barHeight, barWidth * 0.5f);
    }
}

void FxVisualizer::enablementChanged()
{
    if (isEnabled())
    {
        start();
    }
    else
    {
        pause();
        reset();
    }
}

void FxVisualizer::lookAndFeelChanged()
{
    calcGradient();
#if JUCE_MAJOR_VERSION < 8
    setFramesPerSecond(FxTheme::isReducedMotion()?10:30);
#endif
    repaint();
}

void FxVisualizer::calcGradient()
{
    float alpha = 0.62;
    if (FxController::getInstance().isAudioProcessing())
    {
        alpha = 1.0;
    }

    gradient_ = ColourGradient(isEnabled() ? Colour(FXCOLOR(GraphHigh)).withAlpha(alpha) : Colour(FXCOLOR(GraphHigh)).withSaturation(0.0f).withAlpha(alpha),
        2.0f, 0.0f,
        isEnabled() ? Colour(FXCOLOR(GraphHigh)).withAlpha(alpha) : Colour(FXCOLOR(GraphHigh)).withSaturation(0.0f).withAlpha(alpha),
        2.0f, 100.0f, false);
    if (isEnabled())
    {
        gradient_.addColour(0.5f, Colour(FXCOLOR(GraphLow)).withAlpha(alpha));
    }
    else
    {
        gradient_.addColour(0.5f, Colour(FXCOLOR(GraphLow)).withSaturation(0.0f).withAlpha(alpha));
    }
}