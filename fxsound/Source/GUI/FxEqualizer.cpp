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

#include "FxEqualizer.h"
#include "FxController.h"
#include "CynDspUiFollow.h"
#include "FxTheme.h"

FxEqualizer::FxEqualizer()
{
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    auto& controller = FxController::getInstance();

    int num_bands = controller.getNumEqBands();
    labels_.resize(num_bands);
    band_boosts_.resize(num_bands);
    center_frequencies_.resize(num_bands);
    band_gain_values_.resize(num_bands);

    float min_freq, max_freq;
    for (int i = 0; i < num_bands; i++)
    {
        labels_[i].reset(new Label());
        band_boosts_[i].reset(new FxEqSlider(i, MAX_GAIN));
        center_frequencies_[i].reset(new FxBandCenterFreqSlider(i, *labels_[i]));

        labels_[i]->setJustificationType(Justification::centredTop);
        addAndMakeVisible(labels_[i].get());

        band_boosts_[i]->setSliderStyle(Slider::LinearVertical);
        band_boosts_[i]->setRange(-MAX_GAIN, MAX_GAIN, 0.01);
        band_boosts_[i]->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        band_boosts_[i]->addListener(this);
        addAndMakeVisible(band_boosts_[i].get());

        center_frequencies_[i]->setRotaryParameters(3.66519f, 8.90118f, true);
        FxController::getInstance().getEqBandFrequencyRange(i, &min_freq, &max_freq);
        center_frequencies_[i]->setRange(min_freq, max_freq, (max_freq-min_freq)/100);
        center_frequencies_[i]->setFrequency(controller.getEqBandFrequency(i));
        addAndMakeVisible(center_frequencies_[i].get());

        band_gain_values_[i] = 0;
    }

    highlight_mode_ = false;

    setSize(WIDTH, HEIGHT);
}

void FxEqualizer::reinit(int num_bands)
{
    auto& controller = FxController::getInstance();

    removeAllChildren();

    // ------------------------------------------------------------ clear and reinitialize arrays
    labels_.clear();
    band_boosts_.clear();
    center_frequencies_.clear();
    band_gain_values_.clear();

    labels_.resize(num_bands);
    band_boosts_.resize(num_bands);
    center_frequencies_.resize(num_bands);
    band_gain_values_.resize(num_bands);

    // ------------------------------------------------------------ recreate all controls
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    float min_freq, max_freq;

    for (int i = 0; i < num_bands; i++)
    {
        labels_[i].reset(new Label());
        band_boosts_[i].reset(new FxEqSlider(i, MAX_GAIN));
        center_frequencies_[i].reset(new FxBandCenterFreqSlider(i, *labels_[i]));

        labels_[i]->setJustificationType(Justification::centredTop);
        addAndMakeVisible(labels_[i].get());

        band_boosts_[i]->setSliderStyle(Slider::LinearVertical);
        band_boosts_[i]->setRange(-MAX_GAIN, MAX_GAIN, 0.01);
        band_boosts_[i]->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        band_boosts_[i]->addListener(this);
        addAndMakeVisible(band_boosts_[i].get());

        center_frequencies_[i]->setRotaryParameters(3.66519f, 8.90118f, true);
        controller.getEqBandFrequencyRange(i, &min_freq, &max_freq);
        center_frequencies_[i]->setRange(min_freq, max_freq, (max_freq - min_freq) / 100);
        center_frequencies_[i]->setFrequency(controller.getEqBandFrequency(i));
        addAndMakeVisible(center_frequencies_[i].get());

        band_gain_values_[i] = 0;
    }

    resized();

    update();
    showValues(true);
}

void FxEqualizer::sliderValueChanged(Slider* slider)
{

}

void FxEqualizer::sliderDragStarted(Slider* slider)
{
    if (highlight_mode_)
    {
        return;
    }

    if (ModifierKeys::getCurrentModifiersRealtime().isAltDown())
    {
        auto& controller = FxController::getInstance();

        highlight_mode_ = true;

        startTimerHz(30);

        for (auto i = 0; i < band_boosts_.size(); i++)
        {
            if (band_boosts_[i].get() != slider)
            {
                band_boosts_[i]->setEnabled(false);
                band_boosts_[i]->showValue(false);

                band_gain_values_[i] = controller.getEqBandBoostCut(i);
            }
        }
    }
}

void FxEqualizer::sliderDragEnded(Slider* slider)
{
    if (highlight_mode_)
    {
        auto& controller = FxController::getInstance();
        highlight_mode_ = false;

        for (auto i = 0; i < band_boosts_.size(); i++)
        {
            if (band_boosts_[i].get() != slider)
            {
                band_boosts_[i]->setEnabled(true);
                band_boosts_[i]->showValue(true);

                controller.setEqBandBoostCut(i, band_gain_values_[i]);
                band_boosts_[i]->setValue(band_gain_values_[i], NotificationType::dontSendNotification);
            }
        }
    }
}

void FxEqualizer::timerCallback()
{
    auto& controller = FxController::getInstance();

    if (highlight_mode_)
    {
        auto done = false;
        for (auto i = 0; i < band_boosts_.size(); i++)
        {
            if (!band_boosts_[i]->isEnabled())
            {
                auto gain = controller.getEqBandBoostCut(i);
                const float target = -(MAX_GAIN - 2.0f);
                if (std::abs(gain - target) >= 0.01f)
                {
                    done = true;
                    // Fractional EQ values must not make Alt-drag oscillate forever.
                    const float next = gain + juce::jlimit(-1.0f, 1.0f, target - gain);
                    controller.setEqBandBoostCut(i, next);
                    band_boosts_[i]->setGainValue(next);
                }
            }
        }

        if (!done)
        {
            stopTimer();
        }
    }
    else
    {
        stopTimer();
    }
}

void FxEqualizer::syncAppliedDsp(float elapsedMs, bool animate)
{
    if (highlight_mode_) return; // Keep the existing Alt-drag multi-band workflow.
    auto& controller = FxController::getInstance();
    bool visualChanged = false;
    for (size_t i = 0; i < band_boosts_.size() &&
             static_cast<int>(i) < controller.getNumEqBands(); ++i)
    {
        auto* slider = band_boosts_[i].get();
        // Never override the thumb while the user is changing the preset.
        if (slider == nullptr || slider->isMouseButtonDown() || !slider->isEnabled()) continue;
        const float applied = controller.getAppliedEqBandBoostCut(static_cast<int>(i));
        if (!std::isfinite(applied)) continue;
        const float shown = static_cast<float>(slider->getValue());
        const float next = CynDspUiFollow::towardApplied(shown,
            juce::jlimit(-MAX_GAIN, MAX_GAIN, applied), elapsedMs, animate);
        if (std::abs(next - shown) >= 0.008f)
        {
            slider->setGainValue(next); // dontSendNotification: display only.
            visualChanged = true;
        }
    }
    // The parent draws the curve and gradient from the slider positions.
    if (visualChanged) repaint();
}

void FxEqualizer::update(bool snapValues)
{
    auto& controller = FxController::getInstance();

    for (auto i = 0; i<band_boosts_.size() && i<controller.getNumEqBands(); i++)
    {
        auto value = controller.getAppliedEqBandBoostCut(i);
        if (snapValues && value >= -MAX_GAIN && value <= MAX_GAIN)
        {
            band_boosts_[i]->setGainValue(value);
        }

        auto freq = controller.getEqBandFrequency(i);
        center_frequencies_[i]->setFrequency(freq);
    }
}

void FxEqualizer::showValues(bool show)
{
    for (int i = 0; i < band_boosts_.size(); i++)
    {
        band_boosts_[i]->showValue(show);
    }
}

void FxEqualizer::resized()
{
    auto& controller = FxController::getInstance();

    int num_bands = controller.getNumEqBands();

    if (num_bands != labels_.size())
    {
		repaint();
        return;
    }

    auto bounds = getLocalBounds();

    int x = X_MARGIN;
    int width = (bounds.getWidth() - X_MARGIN*2) / labels_.size();
    int rotary_width = jmin(ROTARY_SLIDER_HEIGHT, width);
    // Adapt the actual slider's height to the native Horizon x Depth panel.
    // The legacy 198px high band slider would otherwise overflow a compact
    // 193px high Sound Studio card, hiding the frequency labels and hit zones.
    // No DSP band, value, mouse behavior or saved preset is changed.
    const int compactBandHeight=juce::jlimit(65,SLIDER_HEIGHT+ROTARY_SLIDER_HEIGHT,
        bounds.getHeight()-Y_MARGIN-LABEL_HEIGHT*2-16);
    const int tunedBandHeight=juce::jlimit(55,SLIDER_HEIGHT,
        bounds.getHeight()-Y_MARGIN-LABEL_HEIGHT-rotary_width-22);
    // 39 genuine filter controls must fit without mouse-hit areas overlapping.
    const int thumbWidth = jmin(FxTheme::SLIDER_THUMB_RADIUS * 4, width);


    bool show_center_frequencies = true;
    if (labels_.size() > 10)
    {
        show_center_frequencies = false;
    }

    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    for (auto i=0; i<labels_.size(); i++)
    {
        center_frequencies_[i]->setVisible(show_center_frequencies);
        // On high-density layouts, every second label is visually hidden,
        // but ALL real filter controls remain accessible and live.
        labels_[i]->setVisible(labels_.size() < 35 || i == 0 ||
                                i == labels_.size()-1 || i%2 == 0);
        if (show_center_frequencies)
        {
            labels_[i]->setFont(theme.getNormalFont().withHeight(LABEL_HEIGHT));
            band_boosts_[i]->setBounds(x + (width - thumbWidth) / 2, Y_MARGIN, thumbWidth, tunedBandHeight );
            labels_[i]->setBounds(x, band_boosts_[i]->getBottom() + 6, width, LABEL_HEIGHT);
            center_frequencies_[i]->setBounds(x + (width - rotary_width) / 2, labels_[i]->getBottom() + 4, rotary_width, rotary_width);
        }
        else
        {
            labels_[i]->setFont(theme.getNormalFont().withHeight(SMALL_FONT));
            band_boosts_[i]->setBounds(x + (width - thumbWidth) / 2, Y_MARGIN, thumbWidth, compactBandHeight);
            labels_[i]->setBounds(x, band_boosts_[i]->getBottom() + 6, width, LABEL_HEIGHT*2);
        }

        x += width;
    }
}

void FxEqualizer::paint(Graphics& g)
{
    StringArray tool_tips = { TRANS("Hyper-low Bass - First band for very low frequencies down to 20 Hz."),
                              TRANS("Super-low Bass. Increase this for more rumble and \"thump\", decrease if there's too much boominess."),
                              TRANS("Center of your Bass sound. Increase this for a fuller low end, decrease if the bass sounds overwhelming."),
                              TRANS("The low end of your mid-range. Increase this to make vocals sound rich and warm, decrease it to help control instruments that sound loud and muffled."),
                              TRANS("A focal point of the low-mid-range. Increase this to bring out electric guitars and vocal volume, decrease it to reduce any \"boxy\" tones."),
                              TRANS("The center mid-range band. Increase this to drastically boost rhythm instruments and snare hits, reduce it to cut out \"nasal\" tones."),
                              TRANS("The high-mid-range. Increase this to get more instrumental harmonics, reduce it to improve drums that have too much \"clickiness\" or orchestral instruments that are piercing."),
                              TRANS("The lower end of the high-end range. Increase this for more vocal clarity and articulation, reduce it and move the frequency wheel up and down to find and cut out overly loud \"S\" and \"T\" sounds."),
                              TRANS("The core high-end range. Increase this to make your audio sound more like it's in an airy, large space, reduce it to help with room noises and unwanted echoing."),
                              TRANS("The highest range of average human hearing. Increase this to give your sound more of a crisp tone, with lots of overtones. Reduce it to remove hiss or painfully high sounds.") };

    String center_freq_tip = TRANS("This wheel allows you to adjust which frequencies this EQ band is affecting\r\n"
                                   "up or down to target different frequencies/pitches. The EQ slider above\r\n"
                                   "controls the volume of this EQ band. Increase or decrease to boost or cut\r\n"
                                   "a portion of your audio's frequencies, without modifying the rest of your sound.");

    auto& controller = FxController::getInstance();

    int num_bands = controller.getNumEqBands();

    if (num_bands != labels_.size())
    {
		reinit(num_bands);
    }

    auto panelBounds = getLocalBounds().toFloat();
    g.setColour(Colour(FXCOLOR(ControlBackground)).withAlpha(0.94f));
    g.fillRoundedRectangle(panelBounds, 18.0f);
    g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(0.82f));
    g.drawRoundedRectangle(panelBounds.reduced(0.5f), 18.0f, 1.0f);

    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    g.setFont(theme.getSmallFont().withHeight(9.0f));
    g.setColour(Colour(FXCOLOR(HintText)).withAlpha(0.82f));
    g.drawText("EQUALIZER", 14, 7, 120, 14, Justification::centredLeft, false);

    Path path;

    Colour line_colour = Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
    Colour gradient_colour_1 = Colour(FXCOLOR(EqStart)).withAlpha(0.34f);
    Colour gradient_colour_2 = Colour(FXCOLOR(EqEnd)).withAlpha(0.0f);

    if (!isEnabled())
    {
        line_colour = line_colour.withSaturation(0.0);
        gradient_colour_1 = gradient_colour_1.withSaturation(0.0);
        gradient_colour_2 = gradient_colour_2.withSaturation(0.0);
    }
    else
    {
        auto& controller = FxController::getInstance();

        int num_bands = controller.getNumEqBands();
        for (int i = 0; i < num_bands; i++)
        {
            if (!controller.isHelpTooltipsHidden())
            {
                if (num_bands == 10) band_boosts_[i]->setTooltip(tool_tips[i] +
                    (controller.isSmartListening()
                        ? "\nHollow marker = saved preset; orange point = live applied gain." : ""));
                center_frequencies_[i]->setTooltip(center_freq_tip);
            }
            else
            {
                band_boosts_[i]->setTooltip("");
                center_frequencies_[i]->setTooltip("");
            }
        }
    }

    if (highlight_mode_)
    {
        gradient_colour_2 = gradient_colour_2.withSaturation(0.0);
    }

    for (auto i = 0; i < band_boosts_.size() - 1; i++)
    {
        auto y0 = band_boosts_[i]->getPositionOfValue(band_boosts_[i]->getValue()) + Y_MARGIN;
        auto y1 = band_boosts_[i + 1]->getPositionOfValue(band_boosts_[i + 1]->getValue()) + Y_MARGIN;

        auto x0 = band_boosts_[i]->getX() + band_boosts_[i]->getWidth() / 2;
        auto x1 = band_boosts_[i + 1]->getX() + band_boosts_[i + 1]->getWidth() / 2;

        path.addLineSegment(Line<float>(x0, y0, x1, y1), 1.0);

        if (band_boosts_[i]->isEnabled() || band_boosts_[i + 1]->isEnabled())
        {
            g.setColour(line_colour);
        }
        else
        {
            g.setColour(line_colour.withSaturation(0.0));
        }
        g.strokePath(path, PathStrokeType(1.0));
        path.clear();
    }

    for (auto i = 0; i < band_boosts_.size(); i++)
    {
        auto x = band_boosts_[i]->getX() + band_boosts_[i]->getWidth() / 2;
        auto y = band_boosts_[i]->getPositionOfValue(band_boosts_[i]->getValue()) + Y_MARGIN;

        if (i == 0)
        {
            path.startNewSubPath(Point<float>(x, band_boosts_[i]->getBottom() - FxTheme::SLIDER_THUMB_RADIUS));
        }

        path.lineTo(x, y);

        if (i == band_boosts_.size() - 1)
        {
            path.lineTo(x, band_boosts_[i]->getBottom() - FxTheme::SLIDER_THUMB_RADIUS);
        }
    }
    path.closeSubPath();

    auto gradient = ColourGradient(gradient_colour_1, 0, band_boosts_[1]->getY(), gradient_colour_2, 0, band_boosts_[1]->getBottom(), false);
    g.setFillType(FillType(gradient));
    g.fillPath(path);
}

void FxEqualizer::paintOverChildren(Graphics& g)
{
    auto& controller = FxController::getInstance();
    if (!controller.isSmartListening() || !isEnabled()) return;
    bool showingReference = false;

    // Hollow marks indicate the original preset. Lines represent real
    // differences between applied DSP gains and the saved preset.
    for (size_t i = 0; i < band_boosts_.size() &&
                       static_cast<int>(i) < controller.getNumEqBands(); ++i)
    {
        auto* slider = band_boosts_[i].get();
        if (slider == nullptr) continue;
        const float preset = controller.getEqBandBoostCut(static_cast<int>(i));
        const float applied = controller.getAppliedEqBandBoostCut(static_cast<int>(i));
        if (!std::isfinite(preset) || !std::isfinite(applied) ||
            std::abs(applied - preset) < 0.20f) continue;

        showingReference = true;
        const float x = slider->getX() + slider->getWidth() * 0.5f;
        const float presetY = slider->getY() + static_cast<float>(slider->getPositionOfValue(preset));
        const float appliedY = slider->getY() + static_cast<float>(slider->getPositionOfValue(applied));
        g.setColour(Colour(FXCOLOR(HintText)).withAlpha(0.42f));
        g.drawLine(x, presetY, x, appliedY, 1.0f);
        g.setColour(Colour(FXCOLOR(HintText)).withAlpha(0.95f));
        g.drawEllipse(x - 3.0f, presetY - 3.0f, 6.0f, 6.0f, 1.25f);
    }
    if (showingReference)
    {
        auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
        g.setFont(theme.getSmallFont().withHeight(9.0f));
        g.setColour(Colour(FXCOLOR(HintText)).withAlpha(0.9f));
        g.drawText("HOLLOW DOTS = PRESET", getWidth() - 175, 7, 160, 14,
                   Justification::centredRight, false);
    }
}

FxEqualizer::FxEqSlider::FxEqSlider(int band, float max_gain)
{
    band_ = band;

    setMouseCursor(MouseCursor::PointingHandCursor);

    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    gain_label_.setFont(theme.getNormalFont().withHeight(LABEL_HEIGHT));
    gain_label_.setJustificationType(Justification::centred);
    gain_label_.setInterceptsMouseClicks(false, false);
    addChildComponent(gain_label_);

    setWantsKeyboardFocus(true);
}

void FxEqualizer::FxEqSlider::setGainValue(float value)
{
    setValue(value, NotificationType::dontSendNotification);

    const float shown = static_cast<float>(getValue());
    const auto text = std::abs(shown - std::round(shown)) < 0.045f
        ? String::formatted(shown == 0.0f ? "%.0f" : "%+.0f", shown)
        : String::formatted("%+.1f", shown);
    gain_label_.setText(text, NotificationType::dontSendNotification);

    auto y = getPositionOfValue(value) - (FxTheme::SLIDER_THUMB_RADIUS *3);
    gain_label_.setBounds(gain_label_.getBounds().withY(y));
}

void FxEqualizer::FxEqSlider::showValue(bool show)
{
    gain_label_.setVisible(show && isEnabled());
}

void FxEqualizer::FxEqSlider::enablementChanged()
{
    if (isEnabled())
    {
        setMouseCursor(MouseCursor::PointingHandCursor);
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void FxEqualizer::FxEqSlider::resized()
{
    Slider::resized();
    gain_label_.setBounds(gain_label_.getX(), gain_label_.getY(), getWidth(), LABEL_HEIGHT);
}

void FxEqualizer::FxEqSlider::valueChanged()
{
    auto value = getValue();
    if (value != FxController::getInstance().getEqBandBoostCut(band_))
    {
        FxController::getInstance().setEqBandBoostCut(band_, value);

        const auto text = std::abs(value - std::round(value)) < 0.045
            ? String::formatted(value == 0.0 ? "%.0f" : "%+.0f", value)
            : String::formatted("%+.1f", value);
        gain_label_.setText(text, NotificationType::dontSendNotification);

        auto y = getPositionOfValue(value) - (FxTheme::SLIDER_THUMB_RADIUS*3);
        gain_label_.setBounds(gain_label_.getBounds().withY(y));
    }
}

bool FxEqualizer::FxEqSlider::keyPressed(const KeyPress& key)
{
    if (isEnabled())
    {
        if (key.isKeyCode(KeyPress::upKey))
        {
            setValue(getValue() + 1.0);
            return true;
        }
        else if (key.isKeyCode(KeyPress::downKey))
        {
            setValue(getValue() - 1.0);
            return true;
        }
    }

    return false;
}

// -------------------------------------------------------------------------- SLIDERS GAIN - centered with right mouseDown
void FxEqualizer::FxEqSlider::mouseDown(const juce::MouseEvent& event)
{
    // Check if right mouse button is pressed
    if (event.mods.isRightButtonDown())
    {
        // Reset the slider to 0
        setValue(0.0, juce::NotificationType::sendNotification);
    }
    else
    {
        // Default behavior for other buttons
        juce::Slider::mouseDown(event);
    }
}

FxEqualizer::FxBandCenterFreqSlider::FxBandCenterFreqSlider(int band, Label& freq_label) : Slider(Slider::SliderStyle::Rotary, Slider::TextEntryBoxPosition::NoTextBox), freq_label_(freq_label)
{
    band_ = band;

    setMouseCursor(MouseCursor::PointingHandCursor);

    setWantsKeyboardFocus(true);
}

void FxEqualizer::FxBandCenterFreqSlider::setFrequency(float value)
{
    setValue(value, NotificationType::dontSendNotification);

    String text;
    if (value > 0)
    {
        int nBands = FxController::getInstance().getNumEqBands();

        // ------------------------------------------------------ SET TEXT for bands >= 15
        if (nBands >= 15)
        {
            if (value >= 10000)
            {
                text = String::formatted("%.0f\nkHz", value / 1000.0f);
            }
            else if (value >= 1000)
            {
                text = String::formatted("%.1f\nkHz", value / 1000.0f);
            }
            else
            {
                text = String::formatted("%.0f\nHz", value);
            }
        }
        else
        {
            if (value > 1000)
            {
                text = String::formatted("%.2f kHz", value / 1000.0f);
            }
            else
            {
                text = String::formatted("%.0f Hz", value);
            }
        }

        freq_label_.setText(text, NotificationType::dontSendNotification);
    }
}

void FxEqualizer::FxBandCenterFreqSlider::enablementChanged()
{
    if (isEnabled())
    {
        setMouseCursor(MouseCursor::PointingHandCursor);
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void FxEqualizer::FxBandCenterFreqSlider::valueChanged()
{
    auto freq = getValue();

    if (freq != FxController::getInstance().getEqBandFrequency(band_))
    {
        FxController::getInstance().setEqBandFrequency(band_, freq);

        setFrequency(freq);
    }
}

bool FxEqualizer::FxBandCenterFreqSlider::keyPressed(const KeyPress& key)
{
    if (isEnabled())
    {
        if (key.isKeyCode(KeyPress::upKey))
        {
            setValue(getValue() + getInterval());
            return true;
        }
        else if (key.isKeyCode(KeyPress::downKey))
        {
            setValue(getValue() - getInterval());
            return true;
        }
    }

    return false;
}
// ------------------------------------------------------------------------------ SLIDERS FREQ - centered with right mouseDown
void FxEqualizer::FxBandCenterFreqSlider::mouseDown(const juce::MouseEvent& event)
{
    // ------------------------------------------------------- Check if bands are > 15 (with fixed frequency)
    int nBands = FxController::getInstance().getNumEqBands();
    if (nBands >= 15) return;

    // ------------------------------------------------------- Check if right mouse button is pressed
    if (event.mods.isRightButtonDown())
    {
        if (nBands == 5)
        {
            static const float defaultFrequencies[] =
            {
                62.5f,    // band 1
                250.0f,   // band 2
                1000.0f,  // band 3
                4000.0f,  // band 4
                16000.0f  // band 5
            };
            if (band_ >= 0 && band_ <= 4)
            {
                setValue(defaultFrequencies[band_], juce::NotificationType::sendNotification);
            }
        }
        else if (nBands == 10)
        {
            // Matches the legacy (pre-ISO) 10 band frequency grid in
            // GraphicEqReSetAllBandFreqs() that existing presets were authored against.
            static const float defaultFrequencies[] =
            {
                62.5f,     // band 1
                115.734f,  // band 2
                214.311f,  // band 3
                396.85f,   // band 4
                734.867f,  // band 5
                1360.79f,  // band 6
                2519.84f,  // band 7
                4666.12f,  // band 8
                8640.48f,  // band 9
                16000.0f   // band 10
            };
            if (band_ >= 0 && band_ <= 9)
            {
                setValue(defaultFrequencies[band_], juce::NotificationType::sendNotification);
            }
        }
        else
        {
            float min_freq = 20;
            float max_freq = 20000;
            int f = min_freq * pow((max_freq / min_freq), ((float)band_ / (nBands - 1)));
            setValue(f, juce::NotificationType::sendNotification);
        }
    }
    else
    {
        // Default behavior
        juce::Slider::mouseDown(event);
    }
}
