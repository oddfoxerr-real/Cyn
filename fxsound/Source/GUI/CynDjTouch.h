// AI DJ Touch overlay: purely visual, driven by actual applied DSP changes.
// No interaction, inference, disk I/O or DSP state changes during painting.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace CynDjTouch {
enum Style : int { None=0, Cursor=1, DjHand=2, GhostHand=3, Flowers=4, CustomPng=5 };
inline const char* name(int style) noexcept {
    switch(style) {
      case Cursor: return "DJ Cursor";
      case DjHand: return "Animated DJ Hand";
      case GhostHand: return "Ghost Finger";
      case Flowers: return "Flower Touch";
      case CustomPng: return "Custom PNG";
      default: return "None";
    }
}
inline juce::File customImageFile() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Cyn/TouchAssets/custom-touch.png");
}
inline juce::Image readCustomImage() {
    const auto path=customImageFile();
    if(!path.existsAsFile() || path.getSize()>2*1024*1024) return {};
    auto image=juce::ImageFileFormat::loadFrom(path);
    if(!image.isValid() || image.getWidth()>1024 || image.getHeight()>1024) return {};
    return image;
}
// The local-space origin is the finger tip / cursor hotspot.
inline void paint(juce::Graphics& g,int style,juce::Point<float> hotspot,
                  float opacity,const juce::Image& custom,float phase=0.0f) {
    using namespace juce;
    if(style<=None || style>CustomPng || opacity<=0.004f) return;
    opacity=jlimit(0.0f,0.86f,opacity);
    Graphics::ScopedSaveState scope(g);
    g.addTransform(AffineTransform::translation(hotspot.x,hotspot.y));
    const auto accent=Colour(FXCOLOR(SliderTrack)).withAlpha(opacity);
    if(style==CustomPng) {
        if(!custom.isValid()) return;
        const auto scale=37.0f/(float)(jmax(custom.getWidth(),custom.getHeight()));
        g.setOpacity(opacity);
        g.drawImageTransformed(custom,AffineTransform::scale(scale)
            .translated(-custom.getWidth()*scale*0.21f,
                         -custom.getHeight()*scale*0.20f));
        return;
    }
    if(style==Flowers) {
        const float turn=phase*0.16f;
        for(int i=0;i<6;++i) {
            const auto a=(float)i*juce::MathConstants<float>::twoPi/6.0f+turn;
            const float x=10.0f*std::cos(a),y=10.0f*std::sin(a);
            g.setColour(accent.withMultipliedAlpha(i%2?0.78f:0.55f));
            g.fillEllipse(x-6,y-6,12,12);
        }
        g.setColour(Colour(0xffffdf9c).withMultipliedAlpha(opacity));
        g.fillEllipse(-5,-5,10,10);
        g.setColour(accent.withMultipliedAlpha(0.55f));
        g.drawEllipse(-19,-19,38,38,1.25f);
        return;
    }
    if(style==Cursor) {
        Path cursor;
        cursor.startNewSubPath(-1,-1);
        cursor.lineTo(0,26);
        cursor.lineTo(7,20);
        cursor.lineTo(13,33);
        cursor.lineTo(18,31);
        cursor.lineTo(12,18);
        cursor.lineTo(22,18);
        cursor.closeSubPath();
        g.setColour(Colours::black.withMultipliedAlpha(opacity));
        g.strokePath(cursor,PathStrokeType(4.0f,PathStrokeType::curved,PathStrokeType::rounded));
        g.setColour(accent);
        g.fillPath(cursor);
        return;
    }
    // Anatomical silhouette of a raised DJ index finger reaching toward a knob.
    // Slight hand lean and fading are driven by actual movement, not a loop.
    const float lean=style==DjHand?1.2f*std::sin(phase):0.0f;
    g.addTransform(AffineTransform::translation(lean,0));
    Path finger;
    finger.startNewSubPath(-5,-2);
    finger.cubicTo(-6,-16, 5,-17, 6,-5); // index finger tip
    finger.lineTo(8,13);
    finger.cubicTo(15,4,21,12,19,20);  // middle finger
    finger.cubicTo(25,13,32,22,27,29);
    finger.cubicTo(35,25,39,31,35,38);
    finger.cubicTo(30,45,31,57,27,66); // outside wrist
    finger.lineTo(7,66);
    finger.cubicTo(2,57,-7,49,-13,40);
    finger.cubicTo(-20,29,-10,25,-4,33);
    finger.closeSubPath();
    const auto skin=style==GhostHand ? Colour(0xffb3edfc) : Colour(0xffc6d5e9);
    g.setColour(Colours::black.withMultipliedAlpha(opacity*0.32f));
    g.fillPath(finger,AffineTransform::translation(2.0f,3.0f));
    g.setColour(skin.withMultipliedAlpha(style==GhostHand?opacity*.28f:opacity*.69f));
    g.fillPath(finger);
    g.setColour(accent.withMultipliedAlpha(style==GhostHand?0.8f:0.95f));
    g.strokePath(finger,PathStrokeType(style==GhostHand?1.6f:1.2f));
    g.drawLine(8,50,28,50,1.1f);
    g.setColour(accent.withMultipliedAlpha(0.28f));
    g.drawEllipse(-11,-11,22,22,1.4f);
    g.fillEllipse(-3.2f,-3.2f,6.4f,6.4f);
}
} // namespace CynDjTouch
