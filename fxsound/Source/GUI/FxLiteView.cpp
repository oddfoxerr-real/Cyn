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

#include <JuceHeader.h>
#include "FxLiteView.h"
#include "FxController.h"

//==============================================================================
FxLiteView::FxLiteView()
{	
	setOpaque(false);
	setSize(WIDTH, HEIGHT);
}

void FxLiteView::resized()
{
	RectanglePlacement placement(RectanglePlacement::xMid | RectanglePlacement::doNotResize);

	auto bounds = getLocalBounds();
	auto component_bounds = preset_list_.getLocalBounds();

	component_bounds.setX(PRESET_LIST_X);
	component_bounds.setY(LIST_Y);
	preset_list_.setBounds(component_bounds);

	component_bounds = endpoint_list_.getLocalBounds();
	component_bounds.setX(OUTPUT_LIST_X);
	component_bounds.setY(LIST_Y);
	endpoint_list_.setBounds(component_bounds);
}

void FxLiteView::paint(Graphics& g)
{
    auto& theme = dynamic_cast<LookAndFeel_V4&>(getLookAndFeel());
    auto bounds = getLocalBounds().toFloat();
    const auto backdrop=theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground).withAlpha(1.0f);
    g.setGradientFill(ColourGradient(backdrop.brighter(0.07f),0.0f,0.0f,
                                      backdrop.darker(0.08f),(float)getWidth(),(float)getHeight(),false));
    g.fillAll();

    auto card = juce::Rectangle<float>(20.0f, 18.0f, (float)BACKGROUND_WIDTH, (float)BACKGROUND_HEIGHT);
    const auto panel=Colour(FXCOLOR(PanelBackground)).withAlpha(1.0f);
    g.setGradientFill(ColourGradient(panel.brighter(0.09f),card.getX(),card.getY(),
                                      panel.darker(0.09f),card.getRight(),card.getBottom(),false));
    g.fillRoundedRectangle(card,18.0f);
    g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(0.88f));
    g.drawRoundedRectangle(card.reduced(0.5f), 18.0f, 1.0f);

    auto power_state = FxModel::getModel().getPowerState();
    preset_list_.setEnabled(power_state);
}
