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
#include "FxWindow.h"
#include "CynIcons.h"

//==============================================================================
FxWindow::FxWindow(String name) : title_bar_(name), content_(nullptr)
{
    ember_shell_ = name.isEmpty();
    setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());

	setSize(64, 64);

	title_bar_.setSize(64, FxTheme::TITLE_BAR_HEIGHT - 1);
	addAndMakeVisible(title_bar_);

    draw_shadow_ = true;
    shadow_width_ = SHADOW_WIDTH;
}

FxWindow::~FxWindow()
{
	setLookAndFeel(nullptr);
}

void FxWindow::setContent(Component* content)
{
	int x = -1;
	int y = -1;

	if (content_ != nullptr && content_ != content)
	{
		if (content->getWidth() > content_->getWidth())
		{
			x = getX() - (content->getWidth() - content_->getWidth()) / 2;
		}
		else
		{
			x = getX() + (content_->getWidth() - content->getWidth()) / 2;
		}

		if (content->getHeight() > content_->getHeight())
		{
			y = getY() - (content->getHeight() - content_->getHeight()) / 2;
		}
		else
		{
			y = getY() + (content_->getHeight() - content->getHeight()) / 2;
		}
	}

	if (content != content_)
	{
		removeChildComponent(content_);
		content_ = content;
		addAndMakeVisible(content);
	}

	content_->setBounds(shadow_width_, title_bar_.getBottom() + 1, content_->getWidth(), content_->getHeight());
	if (x >= 0 && y >= 0)
	{
		setBounds(x-shadow_width_, y-shadow_width_, content_->getWidth() + shadow_width_*2, content_->getHeight() + title_bar_.getHeight() + (ember_shell_ ? 1 : FxTheme::WINDOW_CORNER_RADIUS) + shadow_width_*2);
	}
	else
	{
		setSize(content_->getWidth() + shadow_width_*2, content_->getHeight() + title_bar_.getHeight() + (ember_shell_ ? 1 : FxTheme::WINDOW_CORNER_RADIUS) + shadow_width_*2);
	}
}

void FxWindow::startLogoAnimation()
{
	title_bar_.startLogoAnimation();
}

void FxWindow::stopLogoAnimation()
{
	title_bar_.stopLogoAnimation();
}

void FxWindow::addToolbarButton(Button* toolbarButton, bool right_aligned)
{
	title_bar_.addToolbarButton(toolbarButton, right_aligned);
}

void FxWindow::enableShadow(bool enable)
{
    draw_shadow_ = enable;
    if (enable)
    {
        shadow_width_ = SHADOW_WIDTH;
    }
    else
    {
        shadow_width_ = 0;
    }
}

bool FxWindow::isShadowEnabled()
{
    return draw_shadow_;
}

void FxWindow::paint(Graphics& g)
{
    if (ember_shell_)
    {
        // A continuous top bar / rail surrounds the inset workspace.
        const auto bounds=getLocalBounds().toFloat();
        const auto base=Colour(FXCOLOR(PanelBackground)).withAlpha(1.0f);
        ColourGradient shell(base.brighter(0.09f),bounds.getX(),bounds.getY(),
                             base.darker(0.11f),bounds.getRight(),bounds.getBottom(),false);
        shell.addColour(0.45,base);
        g.setGradientFill(shell);
        const float radius=(float)FxTheme::WINDOW_CORNER_RADIUS;
        g.fillRoundedRectangle(bounds,radius);
        // Restore the original framed, rounded corner treatment. The thin
        // edge and corner arcs are purely decorative: no fake resize grip.
        auto rim=bounds.reduced(0.85f);
        const auto outline=Colour(FXCOLOR(RowOutline)).withAlpha(0.65f);
        g.setColour(outline);
        g.drawRoundedRectangle(rim,radius-0.7f,1.1f);
        const auto accent=Colour(FXCOLOR(SliderTrack)).withAlpha(0.56f);
        g.setColour(accent);
        Path corner;
        const float x=rim.getRight()-radius, y=rim.getBottom()-radius;
        corner.startNewSubPath(x+radius,y-18.0f);
        corner.lineTo(x+radius,y);
        corner.quadraticTo(x+radius,y+radius,x,y+radius);
        corner.lineTo(x-18.0f,y+radius);
        g.strokePath(corner,PathStrokeType(1.6f));
        return;
    }
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    auto bounds = juce::Rectangle<float>((float)shadow_width_, (float)shadow_width_,
                                   (float)getWidth() - shadow_width_ * 2.0f,
                                   (float)getHeight() - shadow_width_ * 2.0f);

    if (draw_shadow_ && shadow_width_ != 0)
    {
        DropShadow shadow(Colours::black.withAlpha(0.48f), shadow_width_ * 3, { 0, 7 });
        Path path;
        path.addRoundedRectangle(bounds, (float)FxTheme::WINDOW_CORNER_RADIUS);
        shadow.drawForPath(g, path);
    }

    auto bg = theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground).withAlpha(0.94f);
    ColourGradient gradient(bg.brighter(0.025f), bounds.getX(), bounds.getY(),
                            bg.darker(0.04f), bounds.getRight(), bounds.getBottom(), false);
    gradient.addColour(0.52, Colour(FXCOLOR(PanelBackground)).withAlpha(0.94f));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, (float)FxTheme::WINDOW_CORNER_RADIUS);

    g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(0.92f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), (float)FxTheme::WINDOW_CORNER_RADIUS, 1.0f);

    // Quiet separator between the custom chrome and the audio deck.
    g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(0.48f));
    g.drawLine((float)shadow_width_ + 18.0f, (float)title_bar_.getBottom(),
               (float)getWidth() - shadow_width_ - 18.0f, (float)title_bar_.getBottom(), 1.0f);
}

void FxWindow::resized()
{
	title_bar_.setBounds(FxTheme::WINDOW_CORNER_RADIUS + shadow_width_, shadow_width_, getWidth() - FxTheme::WINDOW_CORNER_RADIUS*2 - shadow_width_*2, title_bar_.getHeight());
	if (content_ != nullptr)
	{
		content_->setBounds(shadow_width_, title_bar_.getBottom() + 1, content_->getWidth(), content_->getHeight());
	}
}

void FxWindow::CloseButton::paintButton(Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat();
    if (highlighted || down)
    {
        g.setColour(Colour(FXCOLOR(InvalidTextBorder)).withAlpha(down ? 0.28f : 0.16f));
        g.fillEllipse(bounds.expanded(3.0f));
    }

    auto rect = Justification(Justification::centred)
        .appliedToRectangle(juce::Rectangle<int>(getHeight() - 3, getHeight() - 3), getLocalBounds())
        .toFloat();

    Path shape;
    shape.addLineSegment({ 0.0f, 0.0f, 1.0f, 1.0f }, 0.08f);
    shape.addLineSegment({ 1.0f, 0.0f, 0.0f, 1.0f }, 0.08f);
    g.setColour(highlighted ? Colour(FXCOLOR(InvalidTextBorder)).withAlpha(1.0f) : Colour(FXCOLOR(ImageButton)).withAlpha(1.0f));
    g.fillPath(shape, shape.getTransformToScaleToFit(rect, true));
}

FxWindow::TitleBar::TitleBar(String name)
{
	name_ = name;
	dragging_ = false;

	addChildComponent(title_);

	updateLogo();
	if (animation_icon_.get() != nullptr)
	{
		animation_icon_->setAlpha(0.0f);
	}

	setFocusContainer(true);
	setFocusContainerType(FocusContainerType::keyboardFocusContainer);

	close_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	close_button_.setSize(CLOSE_BUTTON_WIDTH, CLOSE_BUTTON_WIDTH);
	addAndMakeVisible(close_button_);
	close_button_.addListener(this);
}

void FxWindow::TitleBar::startLogoAnimation()
{
    if (name_.isEmpty()) return; // The Cyn mark is painted, never overlaid by the legacy logo.
	if (animation_icon_.get() != nullptr)
	{
		Desktop::getInstance().getAnimator().fadeOut(icon_.get(), 600);
		Desktop::getInstance().getAnimator().fadeIn(animation_icon_.get(), 600);
	}
}
void FxWindow::TitleBar::stopLogoAnimation()
{
    if (name_.isEmpty()) return;
	if (animation_icon_.get() != nullptr)
	{
		Desktop::getInstance().getAnimator().fadeOut(animation_icon_.get(), 600);
		Desktop::getInstance().getAnimator().fadeIn(icon_.get(), 600);
	}
}

void FxWindow::TitleBar::addToolbarButton(Button* toolbarButton, bool right_aligned)
{
	if (toolbarButton != nullptr)
	{
		// If translated text is added to the text button in the toolbar, it doesn't change on language change
		// So, the text button is added to the toolbar with button name in English and then button text is translated from button name
		// On repaint, the translated button text is updated
		if (TextButton* text_button = dynamic_cast<TextButton*>(toolbarButton))
		{
			auto text = text_button->getName();
			if (text.isNotEmpty())
			{
				text_button->setButtonText(TRANS(text));
			}
		}

		if (!getWantsKeyboardFocus())
		{
			setWantsKeyboardFocus(true);
		}

		toolbar_buttons_.push_back({ toolbarButton, right_aligned });
		addAndMakeVisible(toolbarButton);
	}
}

void FxWindow::TitleBar::paint(Graphics& g)
{
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    if (name_.isEmpty())
    {
        const auto accent = Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
        const auto text = Colour(FXCOLOR(DefaultText)).withAlpha(1.0f);
        const auto muted = Colour(FXCOLOR(HintText)).withAlpha(1.0f);
        CynIcons::drawMark(g,{0.0f,9.0f,32.0f,32.0f},accent);

        g.setFont(theme.getTitleFont().withHeight(13.0f));
        g.setColour(text);
        g.drawText("CYN / AUDIO", 40, 9, 118, 18, Justification::centredLeft, false);
        g.setFont(theme.getSmallFont().withHeight(9.0f));
        g.setColour(muted.withAlpha(0.78f));
        g.drawText("PERSONAL AUDIO", 40, 27, 118, 14, Justification::centredLeft, false);
    }
    else
    {
        title_.setText(TRANS(name_), NotificationType::dontSendNotification);
        auto font = theme.getNormalFont();
        title_.setFont(font);
        title_.setColour(Label::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::defaultText));
        title_.setSize(font.getStringWidth(TRANS(name_)) * 2, (int)font.getHeight());
    }

    for (auto& item : toolbar_buttons_)
    {
        if (auto* text_button = dynamic_cast<TextButton*>(item.first))
        {
            auto textValue = text_button->getName();
            if (textValue.isNotEmpty())
                text_button->setButtonText(TRANS(textValue));
        }
    }
}

void FxWindow::TitleBar::resized()
{
	RectanglePlacement right_placement = { RectanglePlacement::xRight | RectanglePlacement::yMid | RectanglePlacement::doNotResize };
	close_button_.setBounds(right_placement.appliedTo(close_button_.getLocalBounds(), getLocalBounds()));
	RectanglePlacement left_placement = { RectanglePlacement::xLeft | RectanglePlacement::yMid | RectanglePlacement::doNotResize };
	
	if (!title_.isVisible())
	{
		auto bounds = juce::Rectangle<int>(0, 0, ICON_WIDTH, ICON_HEIGHT);
		RectanglePlacement icon_placement = { RectanglePlacement::xLeft | RectanglePlacement::yTop | RectanglePlacement::doNotResize };

		icon_->setBounds(icon_placement.appliedTo(bounds, getLocalBounds()));
		if (animation_icon_.get() != nullptr)
		{
			animation_icon_->setBounds(icon_placement.appliedTo(bounds, getLocalBounds()));
		}
	}
	else
	{
		auto width = (ICON_HEIGHT - 1)*icon_->getWidth() / icon_->getHeight();
		auto bounds = juce::Rectangle<int>(0, 0, width, ICON_HEIGHT - 1);
		icon_->setBounds(bounds);
		title_.setBounds(left_placement.appliedTo(title_.getLocalBounds(), getLocalBounds()).withX(width + 2));
	}

	if (!toolbar_buttons_.empty())
	{
		int x_right = close_button_.getWidth() + 20;
		int x_left = name_.isEmpty() ? 166 : icon_->getWidth() + 15;

		for (auto& item : toolbar_buttons_)
		{
			auto* button = item.first;
			auto right_aligned = item.second;

			if (right_aligned)
			{
				auto destBounds = getLocalBounds().reduced(x_right, 0);
				button->setBounds(right_placement.appliedTo(button->getLocalBounds(), destBounds));
				x_right += button->getWidth() + 20;
			}
			else
			{
				auto destBounds = getLocalBounds().reduced(x_left, 0);
				button->setBounds(left_placement.appliedTo(button->getLocalBounds(), destBounds));
				x_left += button->getWidth() + 20;
			}
		}
	}
}

void FxWindow::TitleBar::lookAndFeelChanged()
{
	auto icon_alpha = icon_->getAlpha();
	float animation_icon_alpha = 0.0f;
	if (animation_icon_.get() != nullptr)
	{
		animation_icon_alpha = animation_icon_->getAlpha();
	}

	updateLogo();

	icon_->setAlpha(icon_alpha);
	if (animation_icon_.get() != nullptr)
	{
		animation_icon_->setAlpha(animation_icon_alpha);
	}
}

void FxWindow::TitleBar::buttonClicked(Button* button)
{
	if (button == &close_button_)
	{
		auto parent = dynamic_cast<FxWindow*>(getParentComponent());
		parent->closeButtonPressed();
	}
}

void FxWindow::TitleBar::mouseDown(const MouseEvent& e)
{
	dragging_ = true;
	auto parent = dynamic_cast<FxWindow*>(getParentComponent());
	dragger_.startDraggingComponent(parent, e);
}

void FxWindow::TitleBar::mouseDrag(const MouseEvent& e)
{
	if (dragging_)
	{
		auto parent = dynamic_cast<FxWindow*>(getParentComponent());
		dragger_.dragComponent(parent, e, nullptr);
	}
}

void FxWindow::TitleBar::mouseUp(const MouseEvent&)
{
	dragging_ = false;
}

void FxWindow::TitleBar::updateLogo()
{
	if (name_.isEmpty())
	{
		icon_ = Drawable::createFromImageData(FXIMAGE(DefaultLogo), FXIMAGESIZE(DefaultLogo));
		icon_->setTransformToFit(juce::Rectangle<float>(0, (float)(FxTheme::TITLE_BAR_HEIGHT - ICON_HEIGHT) / 2, (float)ICON_WIDTH, (float)ICON_HEIGHT),
			{ RectanglePlacement::xLeft | RectanglePlacement::yMid });

		animation_icon_ = Drawable::createFromImageData(FXIMAGE(HighlightedLogo), FXIMAGESIZE(HighlightedLogo));
		animation_icon_->setTransformToFit(juce::Rectangle<float>(0, (float)(FxTheme::TITLE_BAR_HEIGHT - ICON_HEIGHT) / 2, (float)ICON_WIDTH, (float)ICON_HEIGHT),
			{ RectanglePlacement::xLeft | RectanglePlacement::yMid });
        icon_->setAlpha(0.0f);
        animation_icon_->setAlpha(0.0f);
	}
	else
	{
		icon_ = Drawable::createFromImageData(FXIMAGE(IconLogo), FXIMAGESIZE(IconLogo));
		auto width = (ICON_HEIGHT - 1) * icon_->getWidth() / icon_->getHeight();
		icon_->setTransformToFit(juce::Rectangle<float>(0, (float)(FxTheme::TITLE_BAR_HEIGHT - ICON_HEIGHT - 1) / 2, (float)width, (float)ICON_HEIGHT - 1),
			{ RectanglePlacement::xLeft | RectanglePlacement::yMid });

		auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
		auto font = theme.getNormalFont();

		title_.setText(name_, NotificationType::dontSendNotification);
		title_.setColour(Label::ColourIds::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::highlightedText));
		title_.setFont(font);
		title_.setJustificationType(Justification::centredLeft);
		title_.setSize(font.getStringWidth(name_) * 2, (int)font.getHeight());
		title_.setVisible(true);
	}

	addAndMakeVisible(icon_.get());
	if (animation_icon_.get() != nullptr)
	{
		addAndMakeVisible(animation_icon_.get());
	}
}
