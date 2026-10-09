/*
FxSound
Copyright (C) 2025  FxSound LLC

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

#include "FxPowerButton.h"
#include "FxTheme.h"

FxPowerButton::FxPowerButton(const String &button_name) : DrawableButton(button_name, DrawableButton::ButtonStyle::ImageFitted)
{
	power_state_ = false;

	power_on_image_ = Drawable::createFromImageData(FXIMAGE(PowerOnButton), FXIMAGESIZE(PowerOnButton));
	power_off_image_ = Drawable::createFromImageData(FXIMAGE(PowerOffButton), FXIMAGESIZE(PowerOffButton));
}

void FxPowerButton::paint(Graphics& g)
{
    auto& theme=dynamic_cast<FxTheme&>(getLookAndFeel());
    const float press=theme.animate(*this,1,isMouseButtonDown() ? 1.0f : 0.0f);
    const float hover=theme.animate(*this,0,isMouseOver() ? 1.0f : 0.0f);
    auto bounds = getLocalBounds().toFloat().reduced(2.0f+press*2);
    auto accent = Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
    auto muted = Colour(FXCOLOR(ImageButton)).withAlpha(1.0f);

    if (power_state_)
    {
        g.setColour(accent.withAlpha(isEnabled() ? .14f+hover*.15f : .06f));
        g.fillEllipse(bounds.expanded(4.0f));
        g.setColour(accent.withAlpha(isEnabled() ? 1.0f : 0.35f));
    }
    else
    {
        g.setColour(muted.withAlpha(isEnabled() ? 0.82f : 0.30f));
    }

    const auto centre = bounds.getCentre();
    const float radius = jmin(bounds.getWidth(), bounds.getHeight()) * 0.28f;
    Path arc;
    arc.addCentredArc(centre.x, centre.y + 1.0f, radius, radius, 0.0f,
                     MathConstants<float>::pi * 0.25f,
                     MathConstants<float>::pi * 1.75f, true);
    g.strokePath(arc, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.drawLine(centre.x, centre.y - radius - 2.0f, centre.x, centre.y + 1.0f, 2.0f);
}

bool FxPowerButton::keyPressed(const KeyPress& key)
{
	if (isEnabled() && key.isKeyCode(KeyPress::spaceKey))
	{
		triggerClick();
		return true;
	}

	return false;
}

void FxPowerButton::lookAndFeelChanged()
{
	power_on_image_ = Drawable::createFromImageData(FXIMAGE(PowerOnButton), FXIMAGESIZE(PowerOnButton));
	power_off_image_ = Drawable::createFromImageData(FXIMAGE(PowerOffButton), FXIMAGESIZE(PowerOffButton));
	repaint();
}

bool FxPowerButton::getPowerState()
{
	return power_state_;
}

void FxPowerButton::setPowerState(bool power_state)
{
	power_state_ = power_state;
	repaint();
}

int FxPowerButton::getImageWidth()
{
	return image_width_;
}

void FxPowerButton::setImageWidth(int image_width)
{
	image_width_ = image_width;
}