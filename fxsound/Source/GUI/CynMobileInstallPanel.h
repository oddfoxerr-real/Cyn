// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <JuceHeader.h>

// QR is an offline-generated, error-corrected code for this public URL.
// No device ID, pairing secret or telemetry is embedded.
class CynMobileInstallPanel final : public juce::Component
{
public:
    static constexpr const char* mobileReleases =
        "https://github.com/oddfoxerr-real/Cyn/releases?q=android-studio";
    CynMobileInstallPanel()
    {
        setSize(372, 456);
        heading.setText("CYN on your phone", juce::dontSendNotification);
        heading.setJustificationType(juce::Justification::centred);
        heading.setColour(juce::Label::textColourId, juce::Colours::white);
        heading.setFont(juce::Font(21.0f, juce::Font::bold));
        addAndMakeVisible(heading);

        explanation.setText("Scan this QR code with your phone camera to find CYN Studio for Android.",
                            juce::dontSendNotification);
        explanation.setJustificationType(juce::Justification::centred);
        explanation.setColour(juce::Label::textColourId, juce::Colour(0xffb9c9df));
        explanation.setFont(juce::Font(13.0f));
        addAndMakeVisible(explanation);

        openButton.setButtonText("Open mobile releases");
        openButton.onClick = [] { juce::URL(mobileReleases).launchInDefaultBrowser(); };
        addAndMakeVisible(openButton);
        copyButton.setButtonText("Copy link");
        copyButton.onClick = [] { juce::SystemClipboard::copyTextToClipboard(mobileReleases); };
        addAndMakeVisible(copyButton);
        footer.setText("QR downloads the mobile app. Secure PC-to-phone pairing is not active yet.",
                       juce::dontSendNotification);
        footer.setColour(juce::Label::textColourId, juce::Colour(0xff90a5c0));
        footer.setJustificationType(juce::Justification::centred);
        footer.setFont(juce::Font(11.0f));
        addAndMakeVisible(footer);
    }

    void resized() override
    {
        heading.setBounds(14, 12, getWidth()-28, 36);
        explanation.setBounds(18, 48, getWidth()-36, 44);
        openButton.setBounds(26, 365, getWidth()-52, 34);
        copyButton.setBounds(26, 405, getWidth()-52, 31);
        footer.setBounds(13, 328, getWidth()-26, 34);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff080d16));
        constexpr int modules=37, border=4;
        constexpr int total=modules+2*border;
        constexpr float unit=5.22f;
        constexpr float left=(372-total*unit)*0.5f;
        constexpr float top=93.0f;
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(left-5, top-5, total*unit+10, total*unit+10, 10);
        g.setColour(juce::Colour(0xff0e1b30));
        for(int y=0; y<modules; ++y)
            for(int x=0; x<modules; ++x)
                if(bit(y*modules+x))
                    g.fillRect(juce::Rectangle<float>(
                        left+(x+border)*unit, top+(y+border)*unit, unit+0.015f, unit+0.015f));
    }

    static void show(juce::Component* owner)
    {
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(new CynMobileInstallPanel());
        options.dialogTitle = "Add Mobile Version";
        options.componentToCentreAround = owner;
        options.dialogBackgroundColour = juce::Colour(0xff080d16);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;
        options.launchAsync();
    }
private:
    static bool bit(int offset)
    {
        // 37 x 37 QR matrix; seven leading padding bits in the packed bytes.
        constexpr const char* hex =
            "01fd056387f82b7482a0dd60c77576e8e8712bb75d0f055d8253e52a0ff555555fc010255e013f9d5b49735a00a51f23b6136d05940d0087f1ff931161a065c370c4b1cef7f7fa071a46dcddd19aa4710faa9d92775cb5ae4641b8033e34f3f813f0ddcb3534b7392f1cfa4e5b13c17b0717eb363e188d838e67326ddb0ea63cac2dd3610519f400e93a58b7f6fcff5460a55a962775ba2d7f3bab0e4ee35d1ccfae4e08ab6082ffd8e01121";
        const int bitIndex=offset+7;
        const int digit=bitIndex/4;
        const char c=hex[digit];
        const int value=c>='0'&&c<='9' ? c-'0' : c-'a'+10;
        return ((value>>(3-(bitIndex%4)))&1)!=0;
    }
    juce::Label heading, explanation, footer;
    juce::TextButton openButton, copyButton;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynMobileInstallPanel)
};
