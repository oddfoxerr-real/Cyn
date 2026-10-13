#pragma once
#include "CynSpotify.h"
#include "FxTheme.h"

class CynSpotifyDialog final : public juce::Component, private juce::Timer, public juce::FileDragAndDropTarget {
public:
    explicit CynSpotifyDialog(CynSpotify::Worker& worker):worker_(worker) {
        for(auto* c:std::initializer_list<juce::Component*>{&heading_,&status_,&title_,&artist_,&device_,&hint_,&choose_,&disconnect_,&previous_,&play_,&next_,&open_})addAndMakeVisible(c);
        heading_.setText("Spotify / EMBER connection",juce::dontSendNotification);heading_.setFont(23);
        title_.setFont(20);artist_.setFont(16);status_.setFont(14);
        hint_.setText("Use your existing EMBER Spotify session JSON. Drop it here or select it below.\nSpotify shows what is playing; Cyn's local audio models handle sound detection.",juce::dontSendNotification);
        choose_.onClick=[this]{juce::FileChooser picker("Choose your existing EMBER Spotify JSON",{},"*.json");if(picker.browseForFileToOpen())worker_.choose(picker.getResult());};
        disconnect_.onClick=[this]{worker_.disconnect();};
        previous_.onClick=[this]{worker_.control(3);};next_.onClick=[this]{worker_.control(4);};
        play_.onClick=[this]{worker_.control(worker_.state().playing?2:1);};
        open_.onClick=[this]{const auto url=worker_.state().url;juce::URL(url.isEmpty()?"https://open.spotify.com/":url).launchInDefaultBrowser();};
        setOpaque(true);setSize(680,380);startTimer(500);timerCallback();
    }
    bool isInterestedInFileDrag(const juce::StringArray& files) override {return files.size()==1&&files[0].endsWithIgnoreCase(".json");}
    void filesDropped(const juce::StringArray& files,int,int) override {if(isInterestedInFileDrag(files))worker_.choose(juce::File(files[0]));}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f));}
    void resized() override {
        heading_.setBounds(24,16,630,35);status_.setBounds(24,58,630,40);
        title_.setBounds(24,110,630,32);artist_.setBounds(24,147,630,27);device_.setBounds(24,180,630,28);
        previous_.setBounds(24,222,105,34);play_.setBounds(140,222,110,34);next_.setBounds(261,222,105,34);open_.setBounds(440,222,215,34);
        hint_.setBounds(24,265,630,52);choose_.setBounds(24,328,220,34);disconnect_.setBounds(440,328,215,34);
    }
    static void show(CynSpotify::Worker& worker,juce::Component* parent) {
        juce::DialogWindow::LaunchOptions options;options.content.setOwned(new CynSpotifyDialog(worker));
        options.dialogTitle="Spotify";options.dialogBackgroundColour=juce::Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f);
        options.componentToCentreAround=parent;options.useNativeTitleBar=false;options.resizable=false;options.runModal();
    }
private:
    void timerCallback() override {
        const auto s=worker_.state();status_.setText(s.status,juce::dontSendNotification);
        title_.setText(s.title.isEmpty()?"Nothing playing":s.title,juce::dontSendNotification);
        artist_.setText(s.artist,juce::dontSendNotification);
        const auto time=[](int ms){const int sec=juce::jmax(0,ms/1000);return juce::String(sec/60)+":"+juce::String(sec%60).paddedLeft('0',2);};
        device_.setText(s.device+(s.duration>0?"   "+time(s.position)+" / "+time(s.duration):""),juce::dontSendNotification);
        play_.setButtonText(s.playing?"Pause":"Play");
        for(auto* c:{&play_,&previous_,&next_})c->setEnabled(s.connected&&s.device.isNotEmpty());
    }
    CynSpotify::Worker& worker_;
    juce::Label heading_,status_,title_,artist_,device_,hint_;
    juce::TextButton choose_{"Choose EMBER JSON"},disconnect_{"Disconnect Cyn"},previous_{"Previous"},play_{"Play"},next_{"Next"},open_{"Open in Spotify"};
};
