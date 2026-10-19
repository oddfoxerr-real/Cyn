// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <JuceHeader.h>

// QR is an offline-generated, error-corrected code for this public URL.
// No device ID, pairing secret or telemetry is embedded.
class CynMobileInstallPanel final : public juce::Component
{
public:
    static constexpr const char* mobileReleases =
        "https://github.com/oddfoxerr-real/Cyn/releases/download/android-studio-v0.2.3-preview/Cyn-Studio-Android-0.2.3-preview.apk?utm_source";
    CynMobileInstallPanel()
    {
        setSize(372, 456);
        heading.setText("CYN on your phone", juce::dontSendNotification);
        heading.setJustificationType(juce::Justification::centred);
        heading.setColour(juce::Label::textColourId, juce::Colours::white);
        heading.setFont(juce::Font(21.0f, juce::Font::bold));
        addAndMakeVisible(heading);

        explanation.setText("Scan the QR code to download the Android v0.2.3 APK directly.",
                            juce::dontSendNotification);
        explanation.setJustificationType(juce::Justification::centred);
        explanation.setColour(juce::Label::textColourId, juce::Colour(0xffb9c9df));
        explanation.setFont(juce::Font(13.0f));
        addAndMakeVisible(explanation);

        openButton.setButtonText("Download Android APK");
        openButton.onClick = [] { juce::URL(mobileReleases).launchInDefaultBrowser(); };
        addAndMakeVisible(openButton);
        copyButton.setButtonText("Copy link");
        copyButton.onClick = [] { juce::SystemClipboard::copyTextToClipboard(mobileReleases); };
        addAndMakeVisible(copyButton);
        footer.setText("Direct APK download · no PC-to-phone pairing.",
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
        openButton.setBounds(26, 370, getWidth()-52, 34);
        copyButton.setBounds(26, 408, getWidth()-52, 31);
        footer.setBounds(13, 340, getWidth()-26, 23);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff080d16));
        constexpr int modules=41, border=4;
        constexpr int total=modules+2*border;
        constexpr float unit=5.0f;
        constexpr float left=(372-total*unit)*0.5f;
        constexpr float top=94.0f;
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
        // 41x41 offline QR matrix. Packed row-major, MSB-first (3 pad bits at end).
        // Generated from mobileReleases; QR was decoded back to the exact APK URL.
        constexpr const char* hex =
            "fe0cff473fc151923a506e90a3482bb759482a25dba1afdcf2ec145a01e507faaaaaaafe0083386300fbd743ec5568a74cf5decfffe2adae5d9163a43092fdfd9b586a6414fc6d76c9f73b5250f1231c319ac75742fc0602a85a87fccdbb7206b4060852972b85e8ee1f41788367c85d65cf9f18e38909856974a237e64b7507ae6a3ca7dcd691d29d159fb775b27e24a8fc8ef06541b3f85f4e0a8618400109744e7710bac643fcfa00685e0745ffb0ea0e6a904062e1110baff49e8fa5d796eb443aeb2d32f1af05721f0722fecd5ad5160";
        const int digit = offset / 4;
        const char c = hex[digit];
        const int nibble = c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
        return ((nibble >> (3 - offset % 4)) & 1) != 0;
    }
    juce::Label heading, explanation, footer;
    juce::TextButton openButton, copyButton;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynMobileInstallPanel)
};
