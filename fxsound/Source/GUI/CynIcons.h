/* Cyn icon family. SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
namespace CynIcons {
inline void drawMark(Graphics& g, juce::Rectangle<float> bounds, Colour colour)
{
    Path mark; mark.startNewSubPath(25,8); mark.lineTo(16,3); mark.lineTo(5,9); mark.lineTo(5,23); mark.lineTo(16,29); mark.lineTo(25,24);
    mark.startNewSubPath(11,17); mark.lineTo(14,17); mark.lineTo(16,12); mark.lineTo(19,21); mark.lineTo(21,17); mark.lineTo(27,17);
    g.setColour(colour); g.strokePath(mark,PathStrokeType(2.3f,PathStrokeType::curved,PathStrokeType::rounded),AffineTransform::scale(bounds.getWidth()/32,bounds.getHeight()/32).translated(bounds.getX(),bounds.getY()));
}
inline const char* svg(FxImage image) {
    static const char brand[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M25 8L16 3L5 9V23L16 29L25 24 M11 17H14L16 12L19 21L21 17H27" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char power[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M16 5V16 M10 8A10 10 0 1 0 22 8" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char menu[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M7 10H25 M7 16H25 M7 22H25" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char min[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M7 22H25" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char max[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M8 13V8H13 M19 8H24V13 M24 19V24H19 M13 24H8V19" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char window[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M8 8H24V24H8Z" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char flip[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M7 11H24L20 7 M25 21H8L12 25" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char restore[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M8 10A10 10 0 1 1 6 8 M6 5V12H13" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char remove[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M10 10L22 22 M22 10L10 22" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char next[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M12 9L20 16L12 23" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char prev[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M20 9L12 16L20 23" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char up[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M9 20L16 12L23 20" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char down[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M9 12L16 20L23 12" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    static const char thumb[] = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path d="M16 10a6 6 0 1 0 0 12a6 6 0 1 0 0 -12" fill="none" stroke="#a5b7c4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)";
    switch(image) {
    case FxImage::DefaultLogo:
    case FxImage::HighlightedLogo:
    case FxImage::IconLogo:
        return brand;
    case FxImage::PowerOnButton:
    case FxImage::PowerOffButton:
        return power;
    case FxImage::MenuButton:
    case FxImage::MenuButtonHover:
        return menu;
    case FxImage::MinimizeWindowButton:
    case FxImage::MinimizeWindowButtonHover:
        return min;
    case FxImage::MaximizeButton:
    case FxImage::MaximizeButtonHover:
        return max;
    case FxImage::MinimizeButton:
    case FxImage::MinimizeButtonHover:
        return window;
    case FxImage::FlipButton:
    case FxImage::FlipButtonHover:
        return flip;
    case FxImage::RestoreDefaultsButton:
    case FxImage::RestoreDefaultsButtonHover:
        return restore;
    case FxImage::RemoveButton:
        return remove;
    case FxImage::ArrowNext:
    case FxImage::ArrowNextBW:
        return next;
    case FxImage::ArrowPrev:
    case FxImage::ArrowPrevBW:
        return prev;
    case FxImage::ArrowUpSelected:
    case FxImage::ArrowUp:
        return up;
    case FxImage::ArrowDownSelected:
    case FxImage::ArrowDown:
    case FxImage::DropDownArrow:
    case FxImage::DropDownArrowHover:
        return down;
    case FxImage::SliderThumb:
    case FxImage::SliderThumbBW:
        return thumb;
    default: return nullptr;
    }
}
}
