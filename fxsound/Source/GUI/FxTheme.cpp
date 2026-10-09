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
#include "FxTheme.h"
#include "CynIcons.h"

const uint32 FxTheme::theme_colors_[FxThemeMode::NumModes][FxColor::NumColors] =
{
  { 0x171a20, 0x1e222a, 0x232832, 0x39424f, 0xf0f3f7, 0x252b35, 0xf0f3f7, 0x2c424d, 0xf0f3f7, 0x252b35, 0x252b35, 0xabb6c5, 0xabb6c5, 0x6ad4e5, 0xe0677c, 0x252b35, 0x6ad4e5, 0xa4e9f3, 0xa4e9f3, 0x6ad4e5, 0x6ad4e5, 0x2c424d, 0xa4e9f3, 0x2c424d, 0x1e222a, 0x39424f, 0x6ad4e5 },
  { 0xf3f2ee, 0xe8e7e2, 0xfaf9f6, 0xc9cbd0, 0x202937, 0xfaf9f6, 0x202937, 0xdae9ec, 0x202937, 0xfaf9f6, 0xfaf9f6, 0x596577, 0x596577, 0x246775, 0xe0677c, 0xfaf9f6, 0x246775, 0x377b8a, 0x377b8a, 0x246775, 0x246775, 0xdae9ec, 0x377b8a, 0xdae9ec, 0xe8e7e2, 0xc9cbd0, 0x246775 },
  { 0x141e1b, 0x1b2823, 0x213129, 0x3a5145, 0xeaf6ef, 0x213129, 0xeaf6ef, 0x2e4c3d, 0xeaf6ef, 0x213129, 0x213129, 0xa6beb0, 0xa6beb0, 0x79dfae, 0xe0677c, 0x213129, 0x79dfae, 0xb3f1cf, 0xb3f1cf, 0x79dfae, 0x79dfae, 0x2e4c3d, 0xb3f1cf, 0x2e4c3d, 0x1b2823, 0x3a5145, 0x79dfae },
  { 0x1c1828, 0x252035, 0x2c263e, 0x493e60, 0xf3edfc, 0x2c263e, 0xf3edfc, 0x483557, 0xf3edfc, 0x2c263e, 0x2c263e, 0xb9acd0, 0xb9acd0, 0xc5a1f0, 0xe0677c, 0x2c263e, 0xc5a1f0, 0xe2cafa, 0xe2cafa, 0xc5a1f0, 0xc5a1f0, 0x483557, 0xe2cafa, 0x483557, 0x252035, 0x493e60, 0xc5a1f0 },
  { 0x211b18, 0x2b231e, 0x352b24, 0x584638, 0xf7eee5, 0x352b24, 0xf7eee5, 0x523c2d, 0xf7eee5, 0x352b24, 0x352b24, 0xc8b4a0, 0xc8b4a0, 0xefa976, 0xe0677c, 0x352b24, 0xefa976, 0xf6c9a7, 0xf6c9a7, 0xefa976, 0xefa976, 0x523c2d, 0xf6c9a7, 0x523c2d, 0x2b231e, 0x584638, 0xefa976 },
  { 0x101621, 0x172031, 0x1d293c, 0x324662, 0xeaf1fc, 0x1d293c, 0xeaf1fc, 0x2a3c58, 0xeaf1fc, 0x1d293c, 0x1d293c, 0xa5b7d0, 0xa5b7d0, 0x83b7ff, 0xe0677c, 0x1d293c, 0x83b7ff, 0xb8d6ff, 0xb8d6ff, 0x83b7ff, 0x83b7ff, 0x2a3c58, 0xb8d6ff, 0x2a3c58, 0x172031, 0x324662, 0x83b7ff },
  // Sakura: light palette
  { 0xfff5f9, 0xf9eaf1, 0xfffafe, 0xddb7c8, 0x402737, 0xfdf2f7, 0x402737, 0xf8d9e7, 0x402737, 0xfdf2f7, 0xfdf2f7, 0x826174, 0x826174, 0x9e3465, 0xe0677c, 0xfdf2f7, 0x9e3465, 0xc15b86, 0xc15b86, 0x9e3465, 0x9e3465, 0xf8d9e7, 0xc15b86, 0xf8d9e7, 0xf9eaf1, 0xddb7c8, 0x9e3465 },
  // Arctic: light palette
  { 0xedf8fd, 0xe1f1f9, 0xf6fcff, 0xb8d4e3, 0x1b3749, 0xeef9fd, 0x1b3749, 0xcceaf7, 0x1b3749, 0xeef9fd, 0xeef9fd, 0x54748a, 0x54748a, 0x176a94, 0xe0677c, 0xeef9fd, 0x176a94, 0x3285ac, 0x3285ac, 0x176a94, 0x176a94, 0xcceaf7, 0x3285ac, 0xcceaf7, 0xe1f1f9, 0xb8d4e3, 0x176a94 },
  // Sandstone: light palette
  { 0xfbf4e6, 0xf1e4cf, 0xfffaf0, 0xd8c4a3, 0x463421, 0xf9efdf, 0x463421, 0xf0ddbd, 0x463421, 0xf9efdf, 0xf9efdf, 0x80684e, 0x80684e, 0x925817, 0xe0677c, 0xf9efdf, 0x925817, 0xb17c36, 0xb17c36, 0x925817, 0x925817, 0xf0ddbd, 0xb17c36, 0xf0ddbd, 0xf1e4cf, 0xd8c4a3, 0x925817 },
  // Obsidian: dark palette
  { 0x100f14, 0x19171e, 0x201d25, 0x453641, 0xf7eaee, 0x211b24, 0xf7eaee, 0x4a2835, 0xf7eaee, 0x211b24, 0x211b24, 0xc7aebb, 0xc7aebb, 0xfa6f73, 0xe0677c, 0x211b24, 0xfa6f73, 0xff9a91, 0xff9a91, 0xfa6f73, 0xfa6f73, 0x4a2835, 0xff9a91, 0x4a2835, 0x19171e, 0x453641, 0xfa6f73 },
  // Emerald: dark palette
  { 0x0d1b18, 0x122b24, 0x18362b, 0x2c5548, 0xe9f8f0, 0x19352b, 0xe9f8f0, 0x214f39, 0xe9f8f0, 0x19352b, 0x19352b, 0xa7cdbb, 0xa7cdbb, 0x65d49a, 0xe0677c, 0x19352b, 0x65d49a, 0x9bebba, 0x9bebba, 0x65d49a, 0x65d49a, 0x214f39, 0x9bebba, 0x214f39, 0x122b24, 0x2c5548, 0x65d49a },
  // Deep Sea: dark palette
  { 0x0b1a25, 0x102839, 0x143448, 0x2d5266, 0xe8f6fa, 0x153246, 0xe8f6fa, 0x204659, 0xe8f6fa, 0x153246, 0x153246, 0xa6c6d5, 0xa6c6d5, 0x49bdd6, 0xe0677c, 0x153246, 0x49bdd6, 0x8de1ea, 0x8de1ea, 0x49bdd6, 0x49bdd6, 0x204659, 0x8de1ea, 0x204659, 0x102839, 0x2d5266, 0x49bdd6 },
  // Neon Tokyo: dark palette
  { 0x140d21, 0x1e1430, 0x2a1a41, 0x513968, 0xf9edff, 0x2b1c41, 0xf9edff, 0x48245a, 0xf9edff, 0x2b1c41, 0x2b1c41, 0xc3a5dc, 0xc3a5dc, 0xee70ee, 0xe0677c, 0x2b1c41, 0xee70ee, 0xffb0f3, 0xffb0f3, 0xee70ee, 0xee70ee, 0x48245a, 0xffb0f3, 0x48245a, 0x1e1430, 0x513968, 0xee70ee },
  // Synthwave: dark palette
  { 0x231127, 0x2f1a39, 0x3c204a, 0x72436a, 0xfff0fa, 0x3c204a, 0xfff0fa, 0x632f61, 0xfff0fa, 0x3c204a, 0x3c204a, 0xe2a7d0, 0xe2a7d0, 0xff81af, 0xe0677c, 0x3c204a, 0xff81af, 0xffb6cb, 0xffb6cb, 0xff81af, 0xff81af, 0x632f61, 0xffb6cb, 0x632f61, 0x2f1a39, 0x72436a, 0xff81af },
  // Solar Flare: dark palette
  { 0x201811, 0x302318, 0x3b2b1d, 0x654c30, 0xfff4de, 0x3d2a18, 0xfff4de, 0x644224, 0xfff4de, 0x3d2a18, 0x3d2a18, 0xd9c19a, 0xd9c19a, 0xffbc57, 0xe0677c, 0x3d2a18, 0xffbc57, 0xffe09c, 0xffe09c, 0xffbc57, 0xffbc57, 0x644224, 0xffe09c, 0x644224, 0x302318, 0x654c30, 0xffbc57 },
  // Crimson: dark palette
  { 0x1d1218, 0x2c1b23, 0x37232b, 0x63424a, 0xffeff0, 0x38222b, 0xffeff0, 0x5b303c, 0xffeff0, 0x38222b, 0x38222b, 0xd7b7bd, 0xd7b7bd, 0xee8694, 0xe0677c, 0x38222b, 0xee8694, 0xffb0b6, 0xffb0b6, 0xee8694, 0xee8694, 0x5b303c, 0xffb0b6, 0x5b303c, 0x2c1b23, 0x63424a, 0xee8694 },
  // Mocha: dark palette
  { 0x201a17, 0x2e2520, 0x3a2f29, 0x665044, 0xf5ece6, 0x382d27, 0xf5ece6, 0x594034, 0xf5ece6, 0x382d27, 0x382d27, 0xcbb6a7, 0xcbb6a7, 0xd9a781, 0xe0677c, 0x382d27, 0xd9a781, 0xf1c9a6, 0xf1c9a6, 0xd9a781, 0xd9a781, 0x594034, 0xf1c9a6, 0x594034, 0x2e2520, 0x665044, 0xd9a781 },
  // Monochrome: dark palette
  { 0x111315, 0x1b1e21, 0x25292c, 0x454d51, 0xf6f7f8, 0x282c30, 0xf6f7f8, 0x3c4449, 0xf6f7f8, 0x282c30, 0x282c30, 0xaeb6bb, 0xaeb6bb, 0xe5e9ed, 0xe0677c, 0x282c30, 0xe5e9ed, 0xffffff, 0xffffff, 0xe5e9ed, 0xe5e9ed, 0x3c4449, 0xffffff, 0x3c4449, 0x1b1e21, 0x454d51, 0xe5e9ed }
};

const char* FxTheme::theme_images_[2][FxImage::NumImages] =
{ { BinaryData::logowhite_svg, BinaryData::logowhite_svg, BinaryData::FxSound_White_Bars_svg,
    BinaryData::power_off_svg, BinaryData::power_off_svg, BinaryData::donate_svg, BinaryData::donate_svg, BinaryData::menu_svg, BinaryData::menu_svg,
    BinaryData::minimize_svg, BinaryData::minimize_svg, BinaryData::maximize_svg, BinaryData::maximize_svg, BinaryData::min_window_svg, BinaryData::min_window_svg,
    BinaryData::flip_white_svg, BinaryData::flip_white_svg, BinaryData::restore_defaults_white_svg, BinaryData::restore_defaults_white_svg, BinaryData::remove_svg,
    BinaryData::arrow_next_bw_svg, BinaryData::arrow_next_bw_svg, BinaryData::arrow_prev_bw_svg, BinaryData::arrow_prev_bw_svg, BinaryData::arrow_up_white_svg, BinaryData::arrow_up_white_svg, BinaryData::arrow_down_white_svg, BinaryData::arrow_down_white_svg, BinaryData::dropdown_arrow_bw_svg, BinaryData::dropdown_arrow_bw_svg,
    BinaryData::Slider_Thumb_bw_svg, BinaryData::Slider_Thumb_bw_svg },

  { BinaryData::logoblack_svg, BinaryData::logoblack_svg, BinaryData::FxSound_Black_Bars_svg,
    BinaryData::power_off_black_svg, BinaryData::power_off_black_svg, BinaryData::donate_blue_svg, BinaryData::donate_blue_svg, BinaryData::menu_black_svg, BinaryData::menu_black_svg,
    BinaryData::minimize_black_svg, BinaryData::minimize_black_svg, BinaryData::maximize_black_svg, BinaryData::maximize_black_svg, BinaryData::min_window_black_svg, BinaryData::min_window_black_svg,
    BinaryData::flip_black_svg, BinaryData::flip_black_svg, BinaryData::restore_defaults_black_svg, BinaryData::restore_defaults_black_svg, BinaryData::remove_svg,
    BinaryData::arrow_next_bw_svg, BinaryData::arrow_next_bw_svg, BinaryData::arrow_prev_bw_svg, BinaryData::arrow_prev_bw_svg, BinaryData::arrow_up_black_svg, BinaryData::arrow_up_black_svg, BinaryData::arrow_down_black_svg, BinaryData::arrow_down_black_svg, BinaryData::dropdown_arrow_bw_svg, BinaryData::dropdown_arrow_bw_svg,
    BinaryData::Slider_Thumb_bw_svg, BinaryData::Slider_Thumb_bw_svg } };

const int FxTheme::theme_image_sizes_[2][FxImage::NumImages] =
{ { BinaryData::logowhite_svgSize, BinaryData::logowhite_svgSize, BinaryData::FxSound_White_Bars_svgSize,
    BinaryData::power_off_svgSize, BinaryData::power_off_svgSize, BinaryData::donate_svgSize, BinaryData::donate_svgSize, BinaryData::menu_svgSize, BinaryData::menu_svgSize,
    BinaryData::minimize_svgSize, BinaryData::minimize_svgSize, BinaryData::maximize_svgSize, BinaryData::maximize_svgSize, BinaryData::min_window_svgSize, BinaryData::min_window_svgSize,
    BinaryData::flip_white_svgSize, BinaryData::flip_white_svgSize, BinaryData::restore_defaults_white_svgSize, BinaryData::restore_defaults_white_svgSize, BinaryData::remove_svgSize,
    BinaryData::arrow_next_bw_svgSize, BinaryData::arrow_next_bw_svgSize, BinaryData::arrow_prev_bw_svgSize, BinaryData::arrow_prev_bw_svgSize, BinaryData::arrow_up_white_svgSize, BinaryData::arrow_up_white_svgSize, BinaryData::arrow_down_white_svgSize, BinaryData::arrow_down_white_svgSize, BinaryData::dropdown_arrow_bw_svgSize, BinaryData::dropdown_arrow_bw_svgSize,
    BinaryData::Slider_Thumb_bw_svgSize, BinaryData::Slider_Thumb_bw_svgSize },

  { BinaryData::logoblack_svgSize, BinaryData::logoblack_svgSize, BinaryData::FxSound_Black_Bars_svgSize,
    BinaryData::power_off_black_svgSize, BinaryData::power_off_black_svgSize, BinaryData::donate_blue_svgSize, BinaryData::donate_blue_svgSize, BinaryData::menu_black_svgSize, BinaryData::menu_black_svgSize,
    BinaryData::minimize_black_svgSize, BinaryData::minimize_black_svgSize, BinaryData::maximize_black_svgSize, BinaryData::maximize_black_svgSize, BinaryData::min_window_black_svgSize, BinaryData::min_window_black_svgSize,
    BinaryData::flip_black_svgSize, BinaryData::flip_black_svgSize, BinaryData::restore_defaults_black_svgSize, BinaryData::restore_defaults_black_svgSize, BinaryData::remove_svgSize,
    BinaryData::arrow_next_bw_svgSize, BinaryData::arrow_next_bw_svgSize, BinaryData::arrow_prev_bw_svgSize, BinaryData::arrow_prev_bw_svgSize, BinaryData::arrow_up_black_svgSize, BinaryData::arrow_up_black_svgSize, BinaryData::arrow_down_black_svgSize, BinaryData::arrow_down_black_svgSize, BinaryData::dropdown_arrow_bw_svgSize, BinaryData::dropdown_arrow_bw_svgSize,
    BinaryData::Slider_Thumb_bw_svgSize, BinaryData::Slider_Thumb_bw_svgSize } };

FxThemeMode FxTheme::theme_mode_ = FxThemeMode::Dark;

//==============================================================================
FxTheme::FxTheme() : LookAndFeel_V4()
{
	init();
}

void FxTheme::init()
{
	setColourScheme(getFxColourScheme());

	setColour(ComboBox::ColourIds::arrowColourId, Colour(FXCOLOR(ImageButton)).withAlpha(1.0f));
	setColour(ComboBox::ColourIds::backgroundColourId, Colour(FXCOLOR(ComboBoxBackground)).withAlpha(1.0f));
	setColour(ComboBox::ColourIds::outlineColourId, Colour(FXCOLOR(ComboBoxBackground)).withAlpha(1.0f));
	setColour(ComboBox::ColourIds::focusedOutlineColourId, Colour(FXCOLOR(SliderHighlight)).withAlpha(0.2f));
	setColour(ComboBox::ColourIds::textColourId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::backgroundColourId, Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::outlineColourId, Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::textColourId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::highlightedTextColourId, Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
	setColour(TextButton::ColourIds::buttonColourId, Colour(FXCOLOR(TextButtonBackground)).withAlpha(1.0f));
	setColour(TextButton::ColourIds::buttonOnColourId, Colour(FXCOLOR(TextButtonBackground)).withAlpha(1.0f));
	setColour(TextButton::ColourIds::textColourOffId, Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
	setColour(TextButton::ColourIds::textColourOnId, Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
	setColour(HyperlinkButton::ColourIds::textColourId, Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
	setColour(CaretComponent::ColourIds::caretColourId, Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
	setColour(PopupMenu::ColourIds::backgroundColourId, Colour(FXCOLOR(DefaultFill)).withAlpha(0.99f));
	setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, Colour(FXCOLOR(ImageButton)).withAlpha(1.0f));
	setColour(Slider::ColourIds::rotarySliderOutlineColourId, Colour(FXCOLOR(SliderTrack)).withAlpha(0.2f));
	setColour(Slider::ColourIds::rotarySliderFillColourId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
	setColour(ScrollBar::thumbColourId, Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));

	font_400_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyRegular_ttf, BinaryData::GilroyRegular_ttfSize);
	font_600_ = Typeface::createSystemTypefaceFor(BinaryData::GilroySemibold_ttf, BinaryData::GilroySemibold_ttfSize);
	font_700_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyBold_ttf, BinaryData::GilroyBold_ttfSize);

	drop_down_arrow_ = Drawable::createFromImageData(FXIMAGE(DropDownArrowHover), FXIMAGESIZE(DropDownArrowHover));
	slider_thumb_ = Drawable::createFromImageData(FXIMAGE(SliderThumb), FXIMAGESIZE(SliderThumb));
	drop_down_arrow_grey_ = Drawable::createFromImageData(FXIMAGE(DropDownArrow), FXIMAGESIZE(DropDownArrow));
	slider_thumb_grey_ = Drawable::createFromImageData(FXIMAGE(SliderThumbBW), FXIMAGESIZE(SliderThumbBW));
}

LookAndFeel_V4::ColourScheme FxTheme::getFxColourScheme()
{
	return { Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f), Colour(FXCOLOR(WidgetBackground)).withAlpha(1.0f), Colour(FXCOLOR(MenuBackground)).withAlpha(1.0f),
			 Colour(FXCOLOR(Outline)).withAlpha(1.0f), Colour(FXCOLOR(DefaultText)).withAlpha(1.0f), Colour(FXCOLOR(DefaultFill)).withAlpha(0.2f),
			 Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f), Colour(FXCOLOR(HighlightedFill)).withAlpha(1.0f), Colour(FXCOLOR(MenuText)).withAlpha(1.0f) };
}

Label* FxTheme::createComboBoxTextBox(ComboBox& box)
{
	auto label = LookAndFeel_V4::createComboBoxTextBox(box);
	label->setMouseCursor(MouseCursor::PointingHandCursor);
	box.setMouseCursor(MouseCursor::PointingHandCursor);
	return label;
}

Font FxTheme::getComboBoxFont(ComboBox& box)
{
	if (box.getHeight() <= 30)
		return Font(font_600_).withHeight(14.0f);
    else
		return Font(font_600_).withHeight(15.0f);
}

void FxTheme::positionComboBoxText(ComboBox& box, Label& label)
{
	label.setMinimumHorizontalScale(1.0);
	LookAndFeel_V4::positionComboBoxText(box, label);
	label.setBounds(label.getBounds().withX(5).withRight(box.getWidth() - 37));
}

void FxTheme::drawComboBox(Graphics& g, int width, int height, bool,
                            int, int, int, int, ComboBox& box)
{
    const float radius = jmin(14.0f, height * 0.42f);
    auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);

    auto base = box.findColour(ComboBox::backgroundColourId);
    auto accent = Colour(FXCOLOR(SliderTrack)).withAlpha(box.isEnabled() ? 0.85f : 0.25f);
    auto outline = Colour(FXCOLOR(RowOutline)).withAlpha(0.9f);

    ColourGradient fill(base.brighter(0.035f), 0.0f, 0.0f, base.darker(0.05f), 0.0f, (float)height, false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds, radius);

    const float hover=animate(box,0,box.hasKeyboardFocus(true) || box.isMouseOver() ? 1.0f : 0.0f);
    outline=outline.interpolatedWith(accent.withAlpha(.65f),hover);

    g.setColour(outline);
    g.drawRoundedRectangle(bounds, radius, 1.0f);

    const int margin = width <= 150 ? 20 : 26;
    g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.86f : 0.2f));
    auto* arrow = box.isEnabled() ? drop_down_arrow_.get() : drop_down_arrow_grey_.get();
    arrow->drawWithin(g, juce::Rectangle<float>((float)width - margin, 0.0f, 11.0f, (float)height), { RectanglePlacement::centred }, 1.0f);
}

void FxTheme::drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& box, Label& label)
{
	g.setColour(findColour(ComboBox::textColourId).withMultipliedAlpha(0.5f));

	auto font = label.getLookAndFeel().getLabelFont(label);

	g.setFont(font);

	auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds().withX(10));

	g.drawFittedText(box.getTextWhenNothingSelected(), textArea, label.getJustificationType(),
		jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
		label.getMinimumHorizontalScale());
}

void FxTheme::drawLinearSlider(Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const Slider::SliderStyle style, Slider& slider)
{
    const bool enabled = slider.isEnabled();
    auto accent = Colour(FXCOLOR(SliderTrack)).withAlpha(enabled ? 1.0f : 0.28f);
    auto rail = Colour(FXCOLOR(RowOutline)).withAlpha(enabled ? 0.78f : 0.30f);
    const float hover = animate(slider,0,enabled && (slider.isMouseOverOrDragging() || slider.hasKeyboardFocus(true)) ? 1.0f : 0.0f);
    auto glow = Colour(FXCOLOR(SliderHighlight)).withAlpha(hover*.22f);

    if (style == Slider::LinearHorizontal)
    {
        const float cy = y + height * 0.5f;
        const float trackH = 4.0f;
        g.setColour(rail);
        g.fillRoundedRectangle((float)x, cy - trackH * 0.5f, (float)width, trackH, 3.0f);

        const float filled = jlimit(0.0f, (float)width, sliderPos - (float)x);
        g.setColour(accent);
        g.fillRoundedRectangle((float)x, cy - trackH * 0.5f, filled, trackH, 3.0f);

        if (hover > .001f)
        {
            g.setColour(glow);
            g.fillEllipse(sliderPos - 12.0f, cy - 12.0f, 24.0f, 24.0f);
        }

        g.setColour(enabled ? Colour(FXCOLOR(DefaultText)).withAlpha(1.0f) : Colour(FXCOLOR(HintText)).withAlpha(1.0f));
        g.fillEllipse(sliderPos - 5.5f, cy - 5.5f, 11.0f, 11.0f);
        g.setColour(accent.withAlpha(enabled ? 0.65f : 0.2f));
        g.drawEllipse(sliderPos - 5.5f, cy - 5.5f, 11.0f, 11.0f, 1.0f);
        return;
    }

    if (style == Slider::LinearVertical)
    {
        const float cx = x + width * 0.5f;
        const float trackW = 4.0f;
        g.setColour(rail);
        g.fillRoundedRectangle(cx - trackW * 0.5f, (float)y, trackW, (float)height, 3.0f);

        const float filledTop = jlimit((float)y, (float)(y + height), sliderPos);
        g.setColour(accent);
        g.fillRoundedRectangle(cx - trackW * 0.5f, filledTop, trackW, (float)(y + height) - filledTop, 3.0f);

        if (hover > .001f)
        {
            g.setColour(glow);
            g.fillEllipse(cx - 12.0f, sliderPos - 12.0f, 24.0f, 24.0f);
        }

        g.setColour(enabled ? Colour(FXCOLOR(DefaultText)).withAlpha(1.0f) : Colour(FXCOLOR(HintText)).withAlpha(1.0f));
        g.fillEllipse(cx - 5.5f, sliderPos - 5.5f, 11.0f, 11.0f);
        g.setColour(accent.withAlpha(enabled ? 0.65f : 0.2f));
        g.drawEllipse(cx - 5.5f, sliderPos - 5.5f, 11.0f, 11.0f, 1.0f);
        return;
    }

    LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
}

void FxTheme::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                               const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(3.0f);
    const float radius = jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float lineW = 3.0f;
    const float arcRadius = radius - lineW;
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto centre = bounds.getCentre();

    Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(slider.isEnabled() ? 0.85f : 0.3f));
    g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
        g.setColour(Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
        g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    Point<float> thumb(centre.x + arcRadius * std::cos(toAngle - MathConstants<float>::halfPi),
                       centre.y + arcRadius * std::sin(toAngle - MathConstants<float>::halfPi));
    g.setColour(slider.isEnabled() ? Colour(FXCOLOR(DefaultText)).withAlpha(1.0f) : Colour(FXCOLOR(HintText)).withAlpha(1.0f));
    g.fillEllipse(thumb.x - 3.5f, thumb.y - 3.5f, 7.0f, 7.0f);
}

int FxTheme::getSliderThumbRadius(Slider& slider)
{
    if (slider.getSliderStyle() == Slider::Rotary)
    {
        return ROTARY_SLIDER_THUMB_RADIUS;
    }
    else
    {
        return SLIDER_THUMB_RADIUS;
    }
}

Slider::SliderLayout FxTheme::getSliderLayout(Slider& slider)
{
	auto layout = LookAndFeel_V4::getSliderLayout(slider);
	auto style = slider.getSliderStyle();

	if (style == Slider::LinearVertical)
	{
		auto y = layout.sliderBounds.getY() + (SLIDER_THUMB_RADIUS*2);
		auto height = layout.sliderBounds.getHeight() - (SLIDER_THUMB_RADIUS*2);
		layout.sliderBounds.setY(y);
		layout.sliderBounds.setHeight(height);
	}
	else if (style == Slider::LinearHorizontal)
	{
		auto width = layout.sliderBounds.getWidth() - (SLIDER_THUMB_RADIUS*4);
		layout.sliderBounds.setWidth(width);
	}

	return layout;
}

void FxTheme::drawPopupMenuItem(Graphics& g, const juce::Rectangle<int>& area, bool is_separator, bool is_active,
                                bool is_highlighted, bool is_ticked, bool has_submenu, const String& text,
                                const String& shortcut_key_text, const Drawable* icon, const Colour* text_colour)
{
    if (is_separator)
    {
        g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(0.65f));
        g.drawHorizontalLine(area.getCentreY(), (float)area.getX() + 10.0f, (float)area.getRight() - 10.0f);
        return;
    }

    auto row = area.toFloat().reduced(5.0f, 2.0f);
    if (is_highlighted || is_ticked)
    {
        g.setColour(Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(is_highlighted ? 0.95f : 0.62f));
        g.fillRoundedRectangle(row, 10.0f);
        if (is_highlighted)
        {
            g.setColour(Colour(FXCOLOR(SliderTrack)).withAlpha(0.32f));
            g.drawRoundedRectangle(row.reduced(0.5f), 10.0f, 1.0f);
        }
    }

    auto colour = text_colour != nullptr ? *text_colour : Colour(FXCOLOR(MenuText)).withAlpha(is_active ? 1.0f : 0.36f);
    g.setColour(colour);
    g.setFont(getPopupMenuFont());

    int left = area.getX() + 14;
    if (icon != nullptr)
    {
        icon->drawWithin(g, juce::Rectangle<float>((float)left, (float)area.getY() + 5.0f, 18.0f, (float)area.getHeight() - 10.0f), RectanglePlacement::centred, is_active ? 1.0f : 0.4f);
        left += 25;
    }
    else if (is_ticked)
    {
        g.setColour(Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
        g.fillEllipse((float)left + 1.0f, (float)area.getCentreY() - 3.0f, 6.0f, 6.0f);
        left += 16;
        g.setColour(colour);
    }

    auto textArea = juce::Rectangle<int>(left, area.getY(), area.getRight() - left - (has_submenu ? 22 : 10), area.getHeight());
    g.drawText(text, textArea, Justification::centredLeft, true);

    if (shortcut_key_text.isNotEmpty())
    {
        g.setColour(colour.withAlpha(0.56f));
        g.drawText(shortcut_key_text, area.reduced(14, 0), Justification::centredRight, true);
    }

    if (has_submenu)
    {
        g.setColour(colour.withAlpha(0.72f));
        g.drawText(CharPointer_UTF8("›"), area.getRight() - 22, area.getY(), 14, area.getHeight(), Justification::centred, false);
    }
}

Font FxTheme::getPopupMenuFont()
{
	return Font(font_600_).withHeight(17.0f);
}

void FxTheme::preparePopupMenuWindow(Component& new_window)
{
    new_window.setOpaque(false);
	new_window.setMouseCursor(MouseCursor::PointingHandCursor);
	auto children = new_window.getChildren();
	for (auto child : children)
	{
		child->setMouseCursor(MouseCursor::PointingHandCursor);
	}
}

void FxTheme::drawPopupMenuBackground(Graphics& g,int width,int height)
{
    const auto bounds=juce::Rectangle<float>(0,0,(float)width,(float)height).reduced(0.5f);
    g.setColour(Colour(FXCOLOR(MenuBackground)).withAlpha(1.0f)); g.fillRoundedRectangle(bounds,14);
    g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(1.0f)); g.drawRoundedRectangle(bounds,14,1);
}

void FxTheme::loadFont(String language)
{
    if (language.startsWithIgnoreCase("en"))
    {
        font_400_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyRegular_ttf, BinaryData::GilroyRegular_ttfSize);
        font_600_ = Typeface::createSystemTypefaceFor(BinaryData::GilroySemibold_ttf, BinaryData::GilroySemibold_ttfSize);
        font_700_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyBold_ttf, BinaryData::GilroyBold_ttfSize);
    }
    else if (language.startsWithIgnoreCase("ko"))
    {
        font_400_ = loadTypeface("NotoSansKR-Regular.otf");
        font_600_ = loadTypeface("NotoSansKR-Medium.otf");
        font_700_ = loadTypeface("NotoSansKR-Medium.otf");
    }
	else if (language.startsWithIgnoreCase("zh-CN"))
	{
		font_400_ = loadTypeface("NotoSansSC-Regular.otf");
		font_600_ = loadTypeface("NotoSansSC-Medium.otf");
		font_700_ = loadTypeface("NotoSansSC-Medium.otf");
	}
    else if (language.startsWithIgnoreCase("zh-TW"))
    {
        font_400_ = loadTypeface("NotoSansTC-Regular.ttf");
        font_600_ = loadTypeface("NotoSansTC-Medium.ttf");
        font_700_ = loadTypeface("NotoSansTC-Medium.ttf");
    }
	else if (language.startsWithIgnoreCase("th"))
	{
		font_400_ = loadTypeface("NotoSansThai-Regular.ttf");
		font_600_ = loadTypeface("NotoSansThai-Medium.ttf");
		font_700_ = loadTypeface("NotoSansThai-Medium.ttf");
	}
    else if (language.startsWithIgnoreCase("vi"))
    {
        font_400_ = loadTypeface("MontserratAlternates-Regular.ttf");
        font_600_ = loadTypeface("MontserratAlternates-Medium.ttf");
        font_700_ = loadTypeface("MontserratAlternates-Bold.ttf");
    }
	else if (language.startsWithIgnoreCase("ja"))
	{
		font_400_ = loadTypeface("NotoSansJP-Regular.ttf");
		font_600_ = loadTypeface("NotoSansJP-Medium.ttf");
		font_700_ = loadTypeface("NotoSansJP-Bold.ttf");
	}
	else if (language.startsWithIgnoreCase("ar"))
	{
		font_400_ = loadTypeface("IBMPlexSansArabic-Regular.ttf");
		font_600_ = loadTypeface("IBMPlexSansArabic-Medium.ttf");
		font_700_ = loadTypeface("IBMPlexSansArabic-Bold.ttf");
	}
	else if (language.startsWithIgnoreCase("fa"))
	{
		font_400_ = loadTypeface("IBMPlexSansArabic-Regular.ttf");
		font_600_ = loadTypeface("IBMPlexSansArabic-Medium.ttf");
		font_700_ = loadTypeface("IBMPlexSansArabic-Bold.ttf");
	}
    else
    {
        font_400_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyRegular_ttf, BinaryData::GilroyRegular_ttfSize);
        font_600_ = Typeface::createSystemTypefaceFor(BinaryData::GilroySemibold_ttf, BinaryData::GilroySemibold_ttfSize);
        font_700_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyBold_ttf, BinaryData::GilroyBold_ttfSize);
    }
	
    if (font_400_ == nullptr)
    {
        font_400_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyRegular_ttf, BinaryData::GilroyRegular_ttfSize);
    }
    if (font_600_ == nullptr)
    {
        font_600_ = Typeface::createSystemTypefaceFor(BinaryData::GilroySemibold_ttf, BinaryData::GilroySemibold_ttfSize);
    }
    if (font_700_ == nullptr)
    {
        font_700_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyBold_ttf, BinaryData::GilroyBold_ttfSize);
    }

    setDefaultSansSerifTypeface(font_600_);
}

Font FxTheme::getTextButtonFont(TextButton&, int button_height)
{
    return Font(font_600_.get()).withHeight(jmin(15.0f, (float)button_height - 2.0f));
}

void FxTheme::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                                   bool highlighted, bool down)
{
    const float hover = animate(button,0,highlighted ? 1.0f : 0.0f);
    const float press = animate(button,1,down ? 1.0f : 0.0f);
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f + press);
    auto fill = backgroundColour.interpolatedWith(Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f),hover);
    fill = fill.interpolatedWith(Colour(FXCOLOR(SliderTrack)).withAlpha(.25f),press);
    g.setColour(fill); g.fillRoundedRectangle(bounds,jmin(12.0f,bounds.getHeight()*.36f));
    if (button.getProperties().getWithDefault("ember_navigation",false)) return;
    g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(.74f).interpolatedWith(Colour(FXCOLOR(SliderTrack)).withAlpha(.6f),hover));
    g.drawRoundedRectangle(bounds,jmin(12.0f,bounds.getHeight()*.36f),1);

}

void FxTheme::drawToggleButton(Graphics& g, ToggleButton& button, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds();
    const float switchW = 38.0f;
    const float switchH = 20.0f;
    auto sw = juce::Rectangle<float>((float)bounds.getX(), (float)bounds.getCentreY() - switchH * 0.5f, switchW, switchH);
    const bool on = button.getToggleState();
    const float position = animate(button,2,on ? 1.0f : 0.0f);
    const float hover = animate(button,0,highlighted ? 1.0f : 0.0f);

    auto track = Colour(FXCOLOR(RowOutline)).withAlpha(.95f).interpolatedWith(Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f),position);
    track = track.brighter(hover*.08f);
    if (down) track = track.darker(0.08f);

    g.setColour(track);
    g.fillRoundedRectangle(sw, switchH * 0.5f);
    const float knob = 14.0f;
    const float knobX = sw.getX()+3.0f + position*(switchW-knob-6.0f);
    g.setColour(on ? Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f) : Colour(FXCOLOR(DefaultText)).withAlpha(0.92f));
    g.fillEllipse(knobX, sw.getCentreY() - knob * 0.5f, knob, knob);

    g.setFont(getSmallFont().withHeight(13.0f));
    g.setColour(button.findColour(ToggleButton::textColourId).withAlpha(button.isEnabled() ? 0.94f : 0.4f));
    g.drawFittedText(button.getButtonText(), bounds.withTrimmedLeft((int)switchW + 12), Justification::centredLeft, 2);
}

Font FxTheme::getNormalFont()
{
	return Font(font_600_).withHeight(17.0f);
}

Font FxTheme::getSmallFont()
{
	return Font(font_400_).withHeight(14.0f);
}

Font FxTheme::getTitleFont()
{
	return Font(font_700_).withHeight(17.0f);
}

Typeface::Ptr FxTheme::getDefaultTypeface()
{
    return font_400_;
}

FxThemeMode FxTheme::getThemeMode()
{
	return theme_mode_;
}

void FxTheme::setThemeMode(FxThemeMode theme_mode)
{
	auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

	theme_mode_ = static_cast<FxThemeMode>(jlimit(0, (int)FxThemeMode::NumModes-1, (int)theme_mode));

	theme.init();
}

uint32 FxTheme::getColor(FxColor color)
{
	return theme_colors_[theme_mode_][color];
}

const char* FxTheme::getImage(FxImage image)
{
	if (const auto* custom = CynIcons::svg(image)) {
        static std::array<String,FxImage::NumImages> icons;
        static std::array<uint32,FxImage::NumImages> colours{};
        const auto tint=FXCOLOR(ImageButton);
        if(icons[image].isEmpty() || colours[image]!=tint) {
            icons[image]=String(custom).replace("#a5b7c4","#"+Colour(tint).withAlpha(1.0f).toDisplayString(false));
            colours[image]=tint;
        }
        return icons[image].toRawUTF8();
    }
    return theme_images_[isLightTheme() ? 1 : 0][image];
}

const int FxTheme::getImageSize(FxImage image)
{
	if (CynIcons::svg(image)) return (int)std::strlen(getImage(image));
    return theme_image_sizes_[isLightTheme() ? 1 : 0][image];
}

juce::Rectangle<int> FxTheme::getTooltipBounds(const String& tipText, Point<int> screenPos, juce::Rectangle<int> parentArea)
{
    const TextLayout tl(layoutTooltipText(tipText, Colours::black));

    auto w = (int)(tl.getWidth() + 20.0f);
    auto h = (int)(tl.getHeight() + 12.0f);

    return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 18) : screenPos.x + 36,
        screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 12) : screenPos.y + 12,
        w, h)
        .constrainedWithin(parentArea);
}

void FxTheme::drawTooltip(Graphics& g, const String& text, int width, int height)
{
    juce::Rectangle<int> bounds(width, height);
    auto cornerSize = 12.0f;

    g.setColour(findColour(TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle(bounds.toFloat(), cornerSize);

    g.setColour(findColour(TooltipWindow::outlineColourId));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

    layoutTooltipText(text, findColour(TooltipWindow::textColourId))
        .draw(g, bounds.toFloat().reduced(10, 0));
}

void FxTheme::drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g,
										int w, int h, int titleSpaceX, int titleSpaceW,
										const Image* icon, bool drawTitleTextOnLeft)
{
	if (w * h == 0)
		return;

	auto isActive = window.isActiveWindow();

	g.setColour(getCurrentColourScheme().getUIColour(ColourScheme::widgetBackground));
	g.fillAll();

	Font font(12.0f, Font::plain);
	g.setFont(font);

	auto textW = font.getStringWidth(window.getName());
	auto iconW = 0;
	auto iconH = 0;

	if (icon != nullptr)
	{
		iconH = static_cast<int> (font.getHeight());
		iconW = icon->getWidth() * iconH / icon->getHeight() + 4;
	}

	textW = jmin(titleSpaceW, textW + iconW);
	auto textX = drawTitleTextOnLeft ? titleSpaceX
		: jmax(titleSpaceX, (w - textW) / 2);

	if (textX + textW > titleSpaceX + titleSpaceW)
		textX = titleSpaceX + titleSpaceW - textW;

	if (icon != nullptr)
	{
		g.setOpacity(isActive ? 1.0f : 0.6f);
		g.drawImageWithin(*icon, textX, (h - iconH) / 2, iconW, iconH,
			RectanglePlacement::centred, false);
		textX += iconW;
		textW -= iconW;
	}

	if (window.isColourSpecified(DocumentWindow::textColourId) || isColourSpecified(DocumentWindow::textColourId))
		g.setColour(window.findColour(DocumentWindow::textColourId));
	else
		g.setColour(getCurrentColourScheme().getUIColour(ColourScheme::defaultText));

	g.drawText(window.getName(), textX, 0, textW, h, Justification::centredLeft, true);
}

class FxTheme_DocumentWindowButton : public Button
{
public:
	FxTheme_DocumentWindowButton(const String& name, Colour c, const Path& normal, const Path& toggled)
		: Button(name), colour(c), normalShape(normal), toggledShape(toggled)
	{
	}

	void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
	{
		auto background = Colours::grey;

		if (auto* rw = findParentComponentOfClass<ResizableWindow>())
			if (auto lf = dynamic_cast<LookAndFeel_V4*> (&rw->getLookAndFeel()))
				background = lf->getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::widgetBackground);

		g.fillAll(background);

		g.setColour((!isEnabled() || shouldDrawButtonAsDown) ? colour.withAlpha(0.6f)
			: colour);

		if (shouldDrawButtonAsHighlighted)
		{
			g.fillAll();
			g.setColour(background);
		}

		auto& p = getToggleState() ? toggledShape : normalShape;

		auto reducedRect = Justification(Justification::centred)
			.appliedToRectangle(juce::Rectangle<int>(12, 12), getLocalBounds())
			.toFloat();

		g.fillPath(p, p.getTransformToScaleToFit(reducedRect, true));
	}

private:
	Colour colour;
	Path normalShape, toggledShape;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxTheme_DocumentWindowButton)
};

Button* FxTheme::createDocumentWindowButton(int buttonType)
{
	Path shape;
	auto crossThickness = 0.15f;

	if (buttonType == DocumentWindow::closeButton)
	{
		shape.addLineSegment({ 0.0f, 0.0f, 1.0f, 1.0f }, crossThickness);
		shape.addLineSegment({ 1.0f, 0.0f, 0.0f, 1.0f }, crossThickness);

		return new FxTheme_DocumentWindowButton("close", Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f), shape, shape);
	}

	if (buttonType == DocumentWindow::minimiseButton)
	{
		shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);

		return new FxTheme_DocumentWindowButton("minimise", Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f), shape, shape);
	}

	if (buttonType == DocumentWindow::maximiseButton)
	{
		shape.addLineSegment({ 0.5f, 0.0f, 0.5f, 1.0f }, crossThickness);
		shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);

		Path fullscreenShape;
		fullscreenShape.startNewSubPath(45.0f, 100.0f);
		fullscreenShape.lineTo(0.0f, 100.0f);
		fullscreenShape.lineTo(0.0f, 0.0f);
		fullscreenShape.lineTo(100.0f, 0.0f);
		fullscreenShape.lineTo(100.0f, 45.0f);
		fullscreenShape.addRectangle(45.0f, 45.0f, 100.0f, 100.0f);
		PathStrokeType(30.0f).createStrokedPath(fullscreenShape, fullscreenShape);

		return new FxTheme_DocumentWindowButton("maximise", Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f), shape, fullscreenShape);
	}

	jassertfalse;
	return nullptr;
}

TextLayout FxTheme::layoutTooltipText(const String& text, Colour colour) noexcept
{
    const float tooltipFontSize = 14.0f;
    const int maxToolTipWidth = 400;

    AttributedString s;
    s.setWordWrap(AttributedString::WordWrap::byWord);
    s.setJustification(Justification::centredLeft);
    s.append(text, getNormalFont().withHeight(tooltipFontSize), colour);

    TextLayout tl;
    tl.createLayout(s, (float)maxToolTipWidth);
    return tl;
}

Typeface::Ptr FxTheme::loadTypeface(String fileName)
{
    MemoryBlock fontBuffer;
    String filePath = File::addTrailingSeparator(File::getCurrentWorkingDirectory().getFullPathName());
    File fontFile = File(filePath+fileName);
    if (fontFile.exists())
    {
        if (fontFile.loadFileAsData(fontBuffer))
        {
            return Typeface::createSystemTypefaceFor(fontBuffer.getData(), fontBuffer.getSize());
        }
    }

    return nullptr;
}

bool FxTheme::reduced_motion_ = false;
const char* FxTheme::getThemeName(int i)
{
    static const char* names[] = { "Graphite", "Pearl", "Aurora", "Dusk", "Copper", "Midnight", "Sakura", "Arctic", "Sandstone", "Obsidian", "Emerald", "Deep Sea", "Neon Tokyo", "Synthwave", "Solar Flare", "Crimson", "Mocha", "Monochrome" };
    return names[jlimit(0,(int)FxThemeMode::NumModes-1,i)];
}
bool FxTheme::isLightTheme()
{
    switch (theme_mode_)
    {
        case FxThemeMode::Light:
        case FxThemeMode::Sakura:
        case FxThemeMode::Arctic:
        case FxThemeMode::Sandstone:
            return true;
        default:
            return false;
    }
}

void FxTheme::setReducedMotion(bool value)
{
    reduced_motion_ = value;
    if(auto* theme=dynamic_cast<FxTheme*>(&LookAndFeel::getDefaultLookAndFeel())) {
        for(auto& motion:theme->motions_) { motion.value=motion.target; if(motion.component) motion.component->repaint(); }
        if(value) theme->stopTimer();
    }
}
float FxTheme::animate(Component& component,int channel,float target)
{
    channel=jlimit(0,3,channel);
    for(auto& motion:motions_) if(motion.component.getComponent()==&component) {
        motion.target[channel]=target;
        if(reduced_motion_) motion.value[channel]=target;
        else if(std::abs(motion.value[channel]-target)>.001f) startTimerHz(60);
        return motion.value[channel];
    }
    Motion motion; motion.component=&component;
    motion.value[channel]=motion.target[channel]=target;
    motions_.push_back(motion);
    return target;
}
void FxTheme::timerCallback()
{
    bool active=false;
    for(auto it=motions_.begin();it!=motions_.end();) {
        if(!it->component) { it=motions_.erase(it); continue; }
        bool changed=false;
        for(int i=0;i<4;++i) {
            const float delta=it->target[i]-it->value[i];
            if(std::abs(delta)>.001f) { it->value[i]+=delta*.22f; active=changed=true; }
            else it->value[i]=it->target[i];
        }
        if(changed) it->component->repaint();
        ++it;
    }
    if(!active) stopTimer();
}
