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

#ifndef FXTHEME_H
#define FXTHEME_H

#include <JuceHeader.h>
#include <array>
#include <vector>

//==============================================================================
/*
*/

enum FxThemeMode : int {Dark=0, Light, Aurora, Dusk, Copper, Midnight, Sakura, Arctic, Sandstone, Obsidian, Emerald, DeepSea, NeonTokyo, Synthwave, SolarFlare, Crimson, Mocha, Monochrome, ChatGPTDesktop, NumModes};
enum FxColor : int { WindowBackground, WidgetBackground, MenuBackground, Outline, DefaultText, DefaultFill, HighlightedText, HighlightedFill, MenuText,
					 ComboBoxBackground, TextButtonBackground, ImageButton, HintText, ValidTextBorder, InvalidTextBorder, ControlBackground, SliderTrack, SliderHighlight,
					 GraphHigh, GraphLow, EqStart, EqEnd, VerticalSliderLow, MenuHighlightBackground, PanelBackground, RowOutline, SelectedRowOutline, NumColors};
enum FxImage : int { DefaultLogo, HighlightedLogo, IconLogo,
					 PowerOnButton, PowerOffButton, DonateButton, DonateButtonHover, MenuButton, MenuButtonHover,
					 MinimizeButton, MinimizeButtonHover, MaximizeButton, MaximizeButtonHover, MinimizeWindowButton, MinimizeWindowButtonHover,
					 FlipButton, FlipButtonHover, RestoreDefaultsButton, RestoreDefaultsButtonHover, RemoveButton,
					 ArrowNext, ArrowNextBW, ArrowPrev, ArrowPrevBW, ArrowUpSelected, ArrowUp, ArrowDownSelected, ArrowDown, DropDownArrow, DropDownArrowHover,
	                 SliderThumb, SliderThumbBW, NumImages };

class FxTheme : public LookAndFeel_V4, private Timer
{
public:
    static constexpr int WINDOW_CORNER_RADIUS = 16;
	static constexpr int TITLE_BAR_HEIGHT = 52;
	static constexpr int SLIDER_THUMB_RADIUS = 7;
    static constexpr int ROTARY_SLIDER_THUMB_RADIUS = 4;

	FxTheme();
	~FxTheme() = default;

	LookAndFeel_V4::ColourScheme getFxColourScheme();

	Label* createComboBoxTextBox(ComboBox& box) override;
	Font getComboBoxFont(ComboBox& box) override;
	void positionComboBoxText(ComboBox& box, Label& label) override;
	void drawComboBox(Graphics& g, int width, int height, bool,
						int, int, int, int, ComboBox& box) override;
	void drawComboBoxTextWhenNothingSelected(Graphics&, ComboBox&, Label&) override;

	void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
							float sliderPos, float minSliderPos, float maxSliderPos,
							const Slider::SliderStyle style, Slider& slider) override;
    void drawRotarySlider(Graphics&, int x, int y, int width, int height,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, Slider&) override;
	int getSliderThumbRadius(Slider& slider) override;
	Slider::SliderLayout getSliderLayout(Slider& slider);

	void drawPopupMenuItem(Graphics& g, const juce::Rectangle<int>& area, bool is_separator, bool is_active,
						   bool is_highlighted, bool is_ticked, bool has_submenu, const String& text,
						   const String& shortcut_key_text, const Drawable* icon, const Colour* text_colour) override;
	Font getPopupMenuFont() override;
    void drawPopupMenuBackground(Graphics&, int width, int height) override;
    int getPopupMenuBorderSize() override { return 8; }
	void preparePopupMenuWindow(Component& new_window) override;

	Font getTextButtonFont(TextButton&, int button_height) override;
    void drawButtonBackground(Graphics&, Button&, const Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawToggleButton(Graphics&, ToggleButton&, bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    juce::Rectangle<int> getTooltipBounds(const String& tipText, Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void drawTooltip(Graphics&, const String& text, int width, int height) override;

	void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW,
									   const Image* icon, bool drawTitleTextOnLeft) override;
	Button* createDocumentWindowButton(int buttonType) override;

    void loadFont(String language);
	Font getNormalFont();
	Font getSmallFont();
	Font getTitleFont();
    Typeface::Ptr getDefaultTypeface();

	static FxThemeMode getThemeMode();
    static const char* getThemeName(int index);
    static bool isLightTheme();
    static bool isReducedMotion() { return reduced_motion_; }
    static void setReducedMotion(bool value);
    static bool isSmoothAnimations() { return smooth_animations_; }
    static void setSmoothAnimations(bool value);
    float animate(Component&, int channel, float target);
	static void setThemeMode(FxThemeMode theme_mode);
	static uint32 getColor(FxColor color);
	static const char* getImage(FxImage image);
	static const int getImageSize(FxImage image);

private:
	void init();
    void timerCallback() override;
    struct Motion { Component::SafePointer<Component> component; std::array<float,4> value{}, target{}; };
    std::vector<Motion> motions_;
    static bool reduced_motion_;
    static bool smooth_animations_;

    TextLayout layoutTooltipText(const String& text, Colour colour) noexcept;
    Typeface::Ptr loadTypeface(String fileName);

	std::unique_ptr<Drawable> drop_down_arrow_;
	std::unique_ptr<Drawable> slider_thumb_;
    std::unique_ptr<Drawable> drop_down_arrow_grey_;
    std::unique_ptr<Drawable> slider_thumb_grey_;

	Typeface::Ptr font_400_;
	Typeface::Ptr font_600_;
	Typeface::Ptr font_700_;

	static const uint32 theme_colors_[FxThemeMode::NumModes][FxColor::NumColors];
	static const char* theme_images_[2][FxImage::NumImages];
	static const int theme_image_sizes_[2][FxImage::NumImages];
	static FxThemeMode theme_mode_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxTheme)
};

#define FXCOLOR(color) (FxTheme::getColor(FxColor::color))
#define FXIMAGE(image) (FxTheme::getImage(FxImage::image))
#define FXIMAGESIZE(image) (FxTheme::getImageSize(FxImage::image))

#endif
