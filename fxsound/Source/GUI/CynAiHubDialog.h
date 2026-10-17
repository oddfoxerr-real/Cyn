/* Cyn AI Hub — native JUCE profile browser. SPDX-License-Identifier: AGPL-3.0-or-later
   This is part of Cyn.exe. No web page, WebView, local server or login needed. */
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "FxTheme.h"
#include "FxController.h"

class CynAiHubContent final : public juce::Component, private juce::ListBoxModel
{
public:
    CynAiHubContent()
    {
        setOpaque (true); // Opaque content: never reveal Smart listening through this dialog.
        setSize (790, 490);
        for (auto* component : std::initializer_list<juce::Component*> {
            &title_, &subtitle_, &list_, &description_, &status_,
            &refresh_, &import_, &review_, &publish_, &close_ })
            addAndMakeVisible (component);

        title_.setText ("Cyn AI Hub", juce::dontSendNotification);
        title_.setFont (juce::Font (26.0f, juce::Font::bold));
        title_.setColour (juce::Label::textColourId, juce::Colour (FXCOLOR(DefaultText)).withAlpha (1.0f));
        subtitle_.setText ("Discover, import and share personal audio models — right inside Cyn.",
                           juce::dontSendNotification);
        subtitle_.setColour (juce::Label::textColourId, juce::Colour (FXCOLOR(HintText)).withAlpha (1.0f));

        list_.setModel (this);
        list_.setRowHeight (55);
        list_.setColour (juce::ListBox::backgroundColourId, juce::Colour (FXCOLOR(ControlBackground)).withAlpha (1.0f));
        list_.setColour (juce::ListBox::outlineColourId, juce::Colour (FXCOLOR(RowOutline)).withAlpha (1.0f));
        list_.setOutlineThickness (1);
        // ListBoxModel::selectedRowsChanged handles selection notifications.

        description_.setMultiLine (true);
        description_.setReadOnly (true);
        description_.setScrollbarsShown (true);
        description_.setCaretVisible (false);
        description_.setColour (juce::TextEditor::backgroundColourId, juce::Colour (FXCOLOR(ControlBackground)).withAlpha (1.0f));
        description_.setColour (juce::TextEditor::textColourId, juce::Colour (FXCOLOR(DefaultText)).withAlpha (1.0f));
        description_.setColour (juce::TextEditor::outlineColourId, juce::Colour (FXCOLOR(RowOutline)).withAlpha (1.0f));
        description_.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (FXCOLOR(RowOutline)).withAlpha (1.0f));

        status_.setColour (juce::Label::textColourId, juce::Colour (FXCOLOR(HintText)).withAlpha (1.0f));
        status_.setJustificationType (juce::Justification::centredLeft);
        status_.setText ("Offline profiles load automatically. Refresh to check GitHub community submissions.",
                         juce::dontSendNotification);

        refresh_.setButtonText ("Refresh community");
        import_.setButtonText ("Activate selected AI");
        review_.setButtonText ("Reviews");
        publish_.setButtonText ("Share my AI");
        close_.setButtonText ("Close");
        refresh_.onClick = [this] { refreshCommunity(); };
        import_.onClick = [this] { importSelected(); };
        review_.onClick = [this] {
            if (auto* entry = selected())
                if (entry->issue > 0)
                    juce::URL ("https://github.com/oddfoxerr-real/Cyn/issues/" + juce::String (entry->issue))
                        .launchInDefaultBrowser();
        };
        publish_.onClick = [] {
            // GitHub is the community store, not a separate AI website.
            juce::URL ("https://github.com/oddfoxerr-real/Cyn/issues/new?template=publish-ai.yml")
                .launchInDefaultBrowser();
        };
        close_.onClick = [this] {
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState (0);
        };
        readBundledCatalogue();
    }

    ~CynAiHubContent() override { list_.setModel (nullptr); }

    void resized() override
    {
        const int w = getWidth(), h = getHeight();
        title_.setBounds (24, 15, w - 48, 40);
        subtitle_.setBounds (24, 52, w - 48, 24);
        list_.setBounds (24, 99, w / 2 - 30, h - 187);
        description_.setBounds (w / 2 + 6, 99, w / 2 - 30, h - 187);
        status_.setBounds (24, h - 81, w - 48, 28);
        refresh_.setBounds (24, h - 45, 148, 32);
        import_.setBounds (184, h - 45, 145, 32);
        review_.setBounds (w / 2 + 6, h - 45, 85, 32);
        publish_.setBounds (w / 2 + 98, h - 45, 118, 32);
        close_.setBounds (w - 104, h - 45, 80, 32);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (FXCOLOR(WindowBackground)).withAlpha (1.0f));
        g.setColour (juce::Colour (FXCOLOR(RowOutline)).withAlpha (1.0f));
        g.drawHorizontalLine (84, 24.0f, static_cast<float> (getWidth() - 24));
    }

private:
    struct Profile
    {
        juce::String name, author, category, description, filename, researchUrl;
        int issue = 0;
        bool researchOnly = false;
    };

    static constexpr int maxCatalogueBytes = 256 * 1024;
    static constexpr int maxModelBytes = 2 * 1024 * 1024;
    const juce::String rawBase_ =
        "https://raw.githubusercontent.com/oddfoxerr-real/Cyn/main/ai-hub/community/";

    juce::Label title_, subtitle_, status_;
    juce::ListBox list_;
    juce::TextEditor description_;
    juce::TextButton refresh_, import_, review_, publish_, close_;
    std::vector<Profile> profiles_;
    bool busy_ = false;

    int getNumRows() override { return static_cast<int> (profiles_.size()); }
    void selectedRowsChanged (int) override { updateSelection(); }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height,
                           bool selectedRow) override
    {
        if (row < 0 || row >= getNumRows()) return;
        const auto& item = profiles_[static_cast<size_t> (row)];
        if (selectedRow)
        {
            g.setColour (juce::Colour (FXCOLOR(MenuHighlightBackground)).withAlpha (1.0f));
            g.fillRoundedRectangle (2.0f, 2.0f, static_cast<float> (width - 4),
                                    static_cast<float> (height - 4), 8.0f);
        }
        g.setColour (juce::Colour (FXCOLOR(DefaultText)).withAlpha (1.0f));
        g.setFont (juce::Font (15.0f, juce::Font::bold));
        g.drawText (item.name, 12, 6, width - 24, 23, juce::Justification::centredLeft);
        g.setColour (juce::Colour (FXCOLOR(HintText)).withAlpha (1.0f));
        g.setFont (12.0f);
        g.drawText (item.author + "  •  " + item.category, 12, 29,
                    width - 24, 17, juce::Justification::centredLeft);
    }

    Profile* selected()
    {
        const int index = list_.getSelectedRow();
        if (index < 0 || index >= getNumRows()) return nullptr;
        return &profiles_[static_cast<size_t> (index)];
    }

    void updateSelection()
    {
        if (const auto* p = selected())
        {
            if (p->researchOnly)
            {
                description_.setText ("RESEARCH PREVIEW  /  " + p->name + "\n\n"
                    + p->description + "\n\n"
                    + "Creator: " + p->author + "\n"
                    + "Category: " + p->category + "\n\n"
                    + "This is a 6.3-million-parameter OFFLINE research model, NOT a Cyn "
                      "smart-listening profile. It cannot activate in this Cyn version. "
                      "Use View research files to open its separate GitHub prerelease. "
                      "Your existing AI model and settings stay unchanged.",
                    juce::dontSendNotification);
                import_.setButtonText ("View research files");
            }
            else
            {
                description_.setText ("PROFILE  /  " + p->name + "\n\n"
                    + p->description + "\n\n"
                    + "Creator: " + p->author + "\n"
                    + "Category: " + p->category + "\n\n"
                    + "Activate it in the current Music or Movie mode. You can then "
                      "switch models in Smart Listening without reimporting.\n\n"
                    + (FxController::getInstance().getActiveAIProfileId()
                        == "bundled:" + p->filename ? "ACTIVE IN THIS MODE\n\n" : "")
                    + "Community profiles are user-submitted; listening preferences differ.",
                    juce::dontSendNotification);
                import_.setButtonText ("Activate selected AI");
            }
        }
        else
        {
            description_.setText ("No profiles available.", juce::dontSendNotification);
            import_.setButtonText ("Activate selected AI");
        }
        import_.setEnabled (selected() != nullptr && ! busy_);
        review_.setEnabled (selected() != nullptr && selected()->issue > 0);
    }

    static bool isSafeFilename (const juce::String& name)
    {
        if (! name.endsWith (".cynai.json") || name.length() > 100 || name.isEmpty()) return false;
        for (const auto ch : name)
            if (! (juce::CharacterFunctions::isLetterOrDigit (ch) || ch == '-' || ch == '.'))
                return false;
        return ! name.startsWithChar ('.') && ! name.contains ("..");
    }

    bool loadCatalogue (const juce::var& root)
    {
        if (static_cast<int> (root["schema"]) != 1) return false;
        auto* rows = root["profiles"].getArray();
        if (rows == nullptr || rows->size() > 200) return false;
        std::vector<Profile> collected;
        for (const auto& row : *rows)
        {
            if (! row.isObject()) continue;
            Profile p;
            p.name = row["name"].toString().substring (0, 80);
            p.author = row["author"].toString().substring (0, 60);
            p.category = row["category"].toString().substring (0, 30);
            p.description = row["description"].toString().substring (0, 900);
            p.filename = row["file"].toString();
            p.issue = juce::jlimit (0, 100000000, static_cast<int> (row["issue"]));
            p.researchOnly = row["kind"].toString() == "research";
            p.researchUrl = row["url"].toString();
            // Only our official GitHub experimental release URLs are acceptable.
            const bool trustedResearchUrl =
                p.researchUrl.startsWith ("https://github.com/oddfoxerr-real/Cyn/releases/tag/cyn-apex6m-")
                && ! p.researchUrl.containsAnyOf ("?# \t\r\n")
                && p.researchUrl.length() <= 240;
            if (p.name.isNotEmpty()
                && (p.researchOnly ? trustedResearchUrl : isSafeFilename (p.filename)))
                collected.push_back (p);
        }
        profiles_ = std::move (collected);
        list_.updateContent();
        if (! profiles_.empty()) list_.selectRow (0);
        updateSelection();
        return true;
    }

    void readBundledCatalogue()
    {
        const auto file = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
            .getSiblingFile ("CynAiProfiles/index.json");
        if (! file.existsAsFile() || file.getSize() > maxCatalogueBytes
            || ! loadCatalogue (juce::JSON::parse (file)))
        {
            status_.setText ("Community catalogue not bundled. You can still import local AI files.",
                             juce::dontSendNotification);
        }
        else
        {
            status_.setText ("Loaded built-in community profiles. No server required.",
                             juce::dontSendNotification);
        }
    }

    static juce::String fetchLimited (const juce::String& url, int maxBytes)
    {
        auto stream = juce::URL (url).createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs (9000).withNumRedirectsToFollow (3));
        if (! stream) return {};
        juce::MemoryOutputStream output;
        char buffer[8192] {};
        for (;;)
        {
            const auto length = stream->read (buffer, static_cast<int> (sizeof (buffer)));
            if (length <= 0) break;
            if (output.getDataSize() + static_cast<size_t> (length) > static_cast<size_t> (maxBytes))
                return {};
            output.write (buffer, static_cast<size_t> (length));
        }
        return output.toString();
    }

    void setBusy (bool busy)
    {
        busy_ = busy;
        refresh_.setEnabled (! busy);
        publish_.setEnabled (! busy);
        updateSelection();
    }

    void refreshCommunity()
    {
        if (busy_) return;
        setBusy (true);
        status_.setText ("Checking GitHub community profiles...", juce::dontSendNotification);
        juce::Component::SafePointer<CynAiHubContent> safe (this);
        const auto url = rawBase_ + "index.json";
        juce::Thread::launch ([safe, url]() {
            const auto data = fetchLimited (url, maxCatalogueBytes);
            const auto parsed = juce::JSON::parse (data);
            juce::MessageManager::callAsync ([safe, parsed]() {
                if (auto* view = safe.getComponent())
                {
                    const bool ok = view->loadCatalogue (parsed);
                    view->setBusy (false);
                    view->status_.setText (ok ? "Community profiles refreshed. No account needed to browse."
                         : "Offline: Could not refresh. Bundled profiles remain available.",
                        juce::dontSendNotification);
                }
            });
        });
    }

    void importSelected()
    {
        if (busy_) return;
        auto* item = selected();
        if (! item) return;
        if (item->researchOnly)
        {
            // APEX weights are NOT compatible with the small .cynai model importer.
            // This action NEVER invokes importSmartProfile or changes live audio.
            juce::URL (item->researchUrl).launchInDefaultBrowser();
            status_.setText ("Opened official APEX research prerelease; current AI unchanged.",
                             juce::dontSendNotification);
            return;
        }

        if (! juce::AlertWindow::showOkCancelBox (
                juce::AlertWindow::QuestionIcon, "Activate " + item->name + "?",
                "Use this neural model for " + juce::String(FxController::getInstance().getSmartListeningMode()==1 ?
                    "Movie & TV AI" : "Music AI") +
                "? Your earlier model and learning are saved for switching back.", "Activate", "Cancel"))
            return;

        const auto embedded = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
            .getSiblingFile ("CynAiProfiles/" + item->filename);
        if (embedded.existsAsFile())
        {
            juce::String error;
            const bool success = FxController::getInstance().importSmartProfile (embedded, error);
            status_.setText (success ? "Activated " + item->name + ". Switch models in Smart listening."
                                    : "Import failed: " + error,
                             juce::dontSendNotification);
            return;
        }

        setBusy (true);
        status_.setText ("Downloading verified-profile JSON from GitHub...",
                         juce::dontSendNotification);
        const auto name = item->name;
        const auto url = rawBase_ + item->filename;
        juce::Component::SafePointer<CynAiHubContent> safe (this);
        juce::Thread::launch ([safe, url, name]() {
            const auto json = fetchLimited (url, maxModelBytes);
            const auto parsed = juce::JSON::parse (json);
            juce::MessageManager::callAsync ([safe, json, parsed, name]() {
                if (auto* view = safe.getComponent())
                {
                    juce::String error;
                    bool success = false;
                    if (parsed.isObject() && parsed["format"].toString() == "cyn-ai-profile"
                        && static_cast<int> (parsed["schema"]) == 1)
                    {
                        auto temp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getNonexistentChildFile ("CynAiHubImport", ".cynai.json", false);
                        if (temp.replaceWithText (json))
                            success = FxController::getInstance().importSmartProfile (temp, error);
                        else error = "Could not stage downloaded profile.";
                        temp.deleteFile();
                    }
                    else error = "GitHub returned an invalid Cyn AI profile.";
                    view->setBusy (false);
                    view->status_.setText (success ? "Activated " + name + ". Switch models in Smart listening."
                         : "Import failed: " + error, juce::dontSendNotification);
                }
            });
        });
    }
};

struct CynAiHubDialog
{
    static void show (juce::Component* owner = nullptr)
    {
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (new CynAiHubContent());
        options.dialogTitle = "Cyn AI Hub";
        options.componentToCentreAround = owner; // Anchor to Cyn rather than desktop origin.
        options.dialogBackgroundColour = juce::Colour (FXCOLOR(WindowBackground)).withAlpha (1.0f);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;
        options.launchAsync();
    }
};
