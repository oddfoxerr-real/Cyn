// Cyn procedural theme backdrops. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "FxTheme.h"
#include <cmath>

namespace CynThemeAtmosphere
{
using namespace juce;

inline Colour palette(FxThemeMode mode, FxColor role)
{
    return Colour(FxTheme::getColorForMode(mode, role)).withAlpha(1.0f);
}

class Scene final
{
public:
    Scene(Graphics& graphics,FxThemeMode theme,float time,float level,bool preview)
        :g(graphics),mode(theme),t(time),energy(jlimit(0.0f,1.0f,level)),
         strength(preview?1.0f:0.47f),
         accent(palette(theme,SliderTrack)),soft(palette(theme,SliderHighlight)),
         isLight(theme==FxThemeMode::Light || theme==FxThemeMode::Sakura
              || theme==FxThemeMode::Arctic || theme==FxThemeMode::Sandstone) {}

    void stroke(float x,float y,float x2,float y2,float alpha,float width=1.0f)
    {
        g.setColour(accent.withAlpha(jlimit(0.0f,1.0f,alpha*strength)));
        g.drawLine(x,y,x2,y2,width);
    }
    void ring(float x,float y,float r,float alpha,float width=1.0f)
    {
        g.setColour(soft.withAlpha(jlimit(0.0f,1.0f,alpha*strength)));
        g.drawEllipse(x-r,y-r,r*2,r*2,width);
    }
    void halo(float x,float y,float r,float alpha)
    {
        ColourGradient gradient(accent.withAlpha(alpha*strength),x,y,
                                accent.withAlpha(0.0f),x+r,y+r,true);
        g.setGradientFill(gradient);
        g.fillEllipse(x-r,y-r,r*2,r*2);
    }
    void ribbon(float y,float a,float wavelength,float offset,float alpha,float thick=2)
    {
        Path path;
        for(int i=0;i<=80;++i)
        {
            const float x=i*12.5f;
            const float h=y+a*std::sin(x/wavelength+offset);
            if(i==0) path.startNewSubPath(x,h);
            else path.lineTo(x,h);
        }
        g.setColour((int(y)%2?accent:soft).withAlpha(alpha*strength));
        g.strokePath(path,PathStrokeType(thick,PathStrokeType::curved,PathStrokeType::rounded));
    }

    void render()
    {
        const float beat=1.0f+energy*.25f;
        const float breathe=std::sin(t*.41f);
        // Artwork belongs to the entire landscape, never a floating mini-widget.
        if(mode!=FxThemeMode::DarkMesh)
            halo(795+60*breathe,246,390,.16f+energy*.14f);
        switch(mode)
        {
            case FxThemeMode::Dark:
                for(int n=0;n<14;++n)stroke(n*98-200,560,n*98+140,0,.13f,1.4f);
                for(int n=0;n<5;++n)ring(830,265,68+n*49,.44f-n*.07f,2.2f);
                break;
            case FxThemeMode::Light:
                for(int n=0;n<8;++n)
                {
                    const float x=140+n*115.0f+11*breathe;
                    const float y=215+95*std::sin(n*1.44f);
                    halo(x,y,105,.14f);
                    ring(x,y,35+n%3*22,.18f,2);
                }
                break;
            case FxThemeMode::Aurora:
                halo(745,235,480,.29f+energy*.12f);
                for(int n=0;n<15;++n)
                    ribbon(65+n*34,30+n*.9f+energy*16,119+n*10,t*.35f+n*.35f,.27f-n*.008f,4.2f);
                break;
            case FxThemeMode::Dusk:
                halo(785,245,345,.25f);
                for(int n=0;n<6;++n)ring(785,245,(108+n*47)*beat,.51f-n*.064f,2.1f);
                for(int n=0;n<34;++n)
                {
                    const float a=n*.75f+t*.08f;
                    const float x=785+280*std::cos(a),y=245+195*std::sin(a);
                    g.setColour(soft.withAlpha(.26f*strength));
                    g.fillEllipse(x,y,3,3);
                }
                break;
            case FxThemeMode::Copper:
                for(int n=0;n<5;++n)ring(780,290,87+n*54,.52f-n*.074f,2.5f);
                for(int n=0;n<48;++n)
                {
                    const float a=n*MathConstants<float>::twoPi/48.0f;
                    const float r=n%4==0?183.0f:196.0f;
                    stroke(780+r*std::cos(a),290+r*std::sin(a),
                           780+218*std::cos(a),290+218*std::sin(a),.54f,1.5f);
                }
                stroke(780,290,780+153*std::sin(t*.35f),290-153*std::cos(t*.35f),.72f,3);
                break;
            case FxThemeMode::Midnight:
                for(int n=0;n<100;++n)
                {
                    const float x=25+(n*137)%960,y=10+(n*269)%575;
                    const float twinkle=.5f+.5f*std::sin(t*.72f+n);
                    g.setColour(soft.withAlpha((.11f+.38f*twinkle)*strength));
                    g.fillEllipse(x,y,1+n%3,1+n%3);
                    if(n<17)stroke(x,y,25+(n*137+117)%960,10+(n*269+79)%575,.15f,.8f);
                }
                break;
            case FxThemeMode::Sakura:
                for(int n=0;n<9;++n)
                {
                    stroke(1000,45+n*12,520+n*64,180+n*31,.24f,2.1f);
                    stroke(520+n*64,180+n*31,490+n*65,240+n*24,.17f,1.2f);
                }
                for(int n=0;n<72;++n)
                {
                    const float x=std::fmod((float)n*89+t*14+1200,1000.0f);
                    const float y=std::fmod((float)n*47+t*19+800,600.0f);
                    const float r=2.5f+n%5;
                    g.setColour((n%3?accent:soft).withAlpha(.31f*strength));
                    g.fillEllipse(x,y,r*1.6f,r*.90f);
                }
                break;
            case FxThemeMode::Arctic:
                for(int n=0;n<15;++n)
                {
                    const float x=n*79.0f-120;
                    const float peak=90+(n*73)%190;
                    Path shard;
                    shard.startNewSubPath(x-89,605);
                    shard.lineTo(x+40+12*breathe,peak);
                    shard.lineTo(x+149,605);
                    shard.closeSubPath();
                    g.setColour(accent.withAlpha((.06f+n*.004f)*strength));
                    g.fillPath(shard);
                    g.setColour(soft.withAlpha(.30f*strength));
                    g.strokePath(shard,PathStrokeType(1.3f));
                }
                break;
            case FxThemeMode::Sandstone:
                for(int n=0;n<15;++n)
                    ribbon(194+n*30,38+n*.9f,142+n*12,t*.09f+n*.15f,.36f-n*.011f,2.2f);
                break;
            case FxThemeMode::Obsidian:
                for(int n=0;n<16;++n)
                {
                    const float x=n*80.0f-130,h=130+(n*91)%250+energy*30;
                    Path facet;
                    facet.startNewSubPath(x-76,625);
                    facet.lineTo(x+35+15*breathe,625-h);
                    facet.lineTo(x+135,625);
                    facet.closeSubPath();
                    g.setColour(accent.withAlpha((.07f+n*.004f)*strength));
                    g.fillPath(facet);
                    g.setColour(soft.withAlpha(.29f*strength));
                    g.strokePath(facet,PathStrokeType(1.4f));
                }
                break;
            case FxThemeMode::Emerald:
                for(int n=0;n<13;++n)
                {
                    const float x=n*91.0f-90,bend=12*std::sin(t*.4f+n*.8f);
                    stroke(x,610,x+45+bend,130,.35f,2);
                    for(int j=0;j<8;++j)
                    {
                        const float y=155+j*53.0f,side=j%2?1.0f:-1.0f;
                        stroke(x+45+bend*(610-y)/480,y,
                               x+45+bend*(610-y)/480+65*side,y-28,.29f,1.4f);
                    }
                }
                break;
            case FxThemeMode::DeepSea:
                for(int n=0;n<17;++n)
                    ribbon(94+n*30,21+n*.6f+energy*18,80+n*8,t*.45f+n*.19f,.39f-n*.010f,2.5f);
                for(int n=0;n<42;++n)
                {
                    const float x=(float)((n*97)%1000);
                    const float y=600-std::fmod((float)n*53+t*(9+n%8),600.0f);
                    ring(x,y,3+n%6,.32f,1.3f);
                }
                break;
            case FxThemeMode::NeonTokyo:
                for(int n=0;n<31;++n)
                {
                    const float x=n*36.0f-34,h=88.0f+(float)((n*67)%256);
                    g.setColour(accent.withAlpha(.09f*strength));
                    g.fillRect(x,604-h,31.0f,h);
                    stroke(x,604-h,x+31,604-h,.54f,2);
                    for(int k=0;k<8;++k)
                    {
                        const float y=615-h+k*29.0f;
                        if(y<600)
                            stroke(x+7,y,x+22,y,.14f+.25f*(.5f+.5f*std::sin(t+n+k)),1.5f);
                    }
                }
                break;
            case FxThemeMode::Synthwave:
                halo(770,265,315,.26f);
                ring(770,266,135*beat,.69f,4.0f);
                stroke(0,375,1000,375,.61f,2.2f);
                for(int n=-8;n<=8;++n)stroke(520,375,520+n*140,605,.39f,1.4f);
                for(int n=1;n<=10;++n)stroke(0,375+n*n*2.25f,1000,375+n*n*2.25f,.39f,1.5f);
                break;
            case FxThemeMode::SolarFlare:
                halo(775,290,355*beat,.35f+energy*.15f);
                ring(775,290,120*beat,.84f,4);
                for(int n=0;n<64;++n)
                {
                    const float a=n*MathConstants<float>::twoPi/64+t*.1f;
                    const float r=165+39*std::sin(n*2.5f+t)+energy*55;
                    stroke(775+127*std::cos(a),290+127*std::sin(a),
                           775+r*std::cos(a),290+r*std::sin(a),.42f,2.0f);
                }
                break;
            case FxThemeMode::Crimson:
            {
                for(int n=0;n<13;++n)stroke(0,29+n*48,1000,29+n*48,.085f,.8f);
                Path beatPath;
                beatPath.startNewSubPath(0,325);beatPath.lineTo(270,325);
                beatPath.lineTo(407,286);beatPath.lineTo(451,391+energy*45);
                beatPath.lineTo(518,165-energy*70);beatPath.lineTo(588,352);
                beatPath.lineTo(679,325);beatPath.lineTo(1000,325);
                g.setColour(accent.withAlpha(.72f*strength));
                g.strokePath(beatPath,PathStrokeType(4.0f));
                break;
            }
            case FxThemeMode::Mocha:
                for(int n=0;n<8;++n)
                {
                    g.setColour(soft.withAlpha((.49f-n*.046f)*strength));
                    g.drawEllipse(765-(85+n*44)*beat,394-(29+n*30)*beat,
                                  2*(85+n*44)*beat,2*(29+n*30)*beat,2.1f);
                }
                for(int n=0;n<7;++n)ribbon(93+n*34,18,72+n*18,t*.19f,.24f,2.2f);
                break;
            case FxThemeMode::Monochrome:
                for(int y=0;y<17;++y)
                    for(int x=0;x<29;++x)
                    {
                        const float v=.5f+.5f*std::sin(x*.59f+y*.43f+t*.57f);
                        const float r=1.1f+3*v+energy*1.7f;
                        g.setColour(accent.withAlpha((.12f+.35f*v)*strength));
                        g.fillEllipse(x*36.0f-r,y*35.0f-r,2*r,2*r);
                    }
                break;
            case FxThemeMode::CharcoalDesktop:
                for(int n=0;n<9;++n)
                {
                    const float x=110+n*88.0f+16*breathe;
                    const float y=90+n*37.0f;
                    g.setColour(accent.withAlpha((.055f+n*.01f)*strength));
                    g.fillRoundedRectangle(x,y,430.0f,218.0f,22.0f);
                    g.setColour(soft.withAlpha(.31f*strength));
                    g.drawRoundedRectangle(x,y,430.0f,218.0f,22.0f,1.2f);
                }
                break;
            case FxThemeMode::DarkMesh:
            {
                // Connected liquid WEB: black underneath, gentle blue-white
                // light only near the lower edges; never a gray surface layer.
                const float phase=FxTheme::isReducedMotion()?0.0f:t;
                const float live=jlimit(0.0f,1.0f,energy);
                auto glow=[&](float x,float y,float radius,Colour light,float opacity) {
                    ColourGradient bloom(light.withAlpha(opacity*strength),x,y,
                                         light.withAlpha(0.0f),x+radius,y,true);
                    bloom.addColour(.48f,light.withAlpha(opacity*.31f*strength));
                    g.setGradientFill(bloom);
                    g.fillEllipse(x-radius,y-radius,2.0f*radius,2.0f*radius);
                };
                glow(15.0f+18.0f*std::sin(phase*.16f),621.0f,207.0f,
                     Colour(0xff94bdff),.085f+.018f*live);
                glow(983.0f-20.0f*std::sin(phase*.14f),622.0f,225.0f,
                     Colour(0xffa8c5ff),.095f+.018f*live);
                glow(495.0f+42.0f*std::cos(phase*.11f),659.0f,195.0f,
                     Colour(0xff789ced),.029f+.01f*live);

                constexpr int columns=33, rows=9;
                const auto node=[&](int col,int row)->Point<float> {
                    const float x=-25.0f+col*33.0f+2.5f*std::sin(row*.41f+phase*.15f);
                    const float drift=10.0f*std::sin(col*.37f+row*.53f+phase*.19f);
                    const float curl=7.0f*std::sin(col*.18f-row*.41f-phase*.12f);
                    return {x,414.0f+row*23.0f+drift+curl};
                };
                for(int row=0;row<rows;++row) {
                    for(int col=0;col<columns;++col) {
                        const auto p=node(col,row);
                        const float edge=jlimit(0.0f,1.0f,std::abs(p.x-500.0f)/500.0f);
                        const float lower=jlimit(0.0f,1.0f,(p.y-410.0f)/195.0f);
                        const float liquid=.68f+.32f*std::sin(col*.31f-row*.19f+phase*.27f);
                        const float opacity=(.013f+.076f*edge*edge)*lower*liquid*strength;
                        if(opacity<.004f || p.y>606.0f)continue;
                        g.setColour(Colour(0xff8ab8ff).withAlpha(opacity*.73f));
                        if(col+1<columns)g.drawLine(Line<float>(p,node(col+1,row)),.62f);
                        if(row+1<rows)g.drawLine(Line<float>(p,node(col,row+1)),.58f);
                        if(row+1<rows && col+1<columns && (col+row)%3==0)
                            g.drawLine(Line<float>(p,node(col+1,row+1)),.45f);
                        const float radius=.8f+.35f*edge;
                        g.setColour(Colour(0xffc6deff).withAlpha(opacity*1.7f));
                        g.fillEllipse(p.x-radius,p.y-radius,2*radius,2*radius);
                    }
                }
                break;
            }
            case FxThemeMode::NeuralCore:
                for(int n=0;n<26;++n)
                {
                    const float x=n*40.0f;
                    const float y=315+115*std::sin(n*.51f+t*.34f);
                    halo(x,y,24+energy*15,.28f+energy*.13f);
                    ring(x,y,4+energy*2,.80f,1.8f);
                    if(n>0)
                    {
                        const float prev=315+115*std::sin((n-1)*.51f+t*.34f);
                        stroke(x-40,prev,x,y,.42f+energy*.18f,2.2f);
                    }
                }
                break;
            default: break;
        }
    }
private:
    Graphics& g;
    FxThemeMode mode;
    float t,energy,strength;
    Colour accent,soft;
    bool isLight;
};

inline void paint(Graphics& g,FxThemeMode theme,float timeSeconds,float audioLevel,
                  juce::Rectangle<float> bounds,bool preview=false)
{
    if(bounds.isEmpty()) return;
    Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(bounds.getSmallestIntegerContainer());
    const auto background=palette(theme,WindowBackground);
    const auto accent=palette(theme,SliderTrack);
    const auto detail=palette(theme,SliderHighlight);
    const bool light=theme==FxThemeMode::Light || theme==FxThemeMode::Sakura
                || theme==FxThemeMode::Arctic || theme==FxThemeMode::Sandstone;
    if(theme==FxThemeMode::DarkMesh) {
        // True black even in the large Theme Studio preview.
        g.setColour(Colour(0xff000000));
        g.fillRect(bounds);
    } else {
        const float mix=preview?(light?.08f:.21f):(light?.027f:.073f);
        ColourGradient surface(background.interpolatedWith(accent,mix),bounds.getX(),bounds.getY(),
                               background.interpolatedWith(detail,mix*.7f),bounds.getRight(),bounds.getBottom(),false);
        surface.addColour(.52,background.interpolatedWith(accent,mix*.46f));
        g.setGradientFill(surface);
        g.fillRect(bounds);
    }
    {
        Graphics::ScopedSaveState sceneState(g);
        g.addTransform(AffineTransform::scale(bounds.getWidth()/1000.0f,bounds.getHeight()/600.0f)
                           .translated(bounds.getX(),bounds.getY()));
        Scene scene(g,theme,FxTheme::isReducedMotion()?0.0f:timeSeconds,
                    FxTheme::isReducedMotion()?0.0f:audioLevel,preview);
        scene.render();
    }
}
}
