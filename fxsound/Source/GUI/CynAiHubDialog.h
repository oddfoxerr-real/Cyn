/* Cyn AI Hub — native JUCE profile browser. SPDX-License-Identifier: AGPL-3.0-or-later
   This is part of Cyn.exe. No web page, WebView, local server or login needed. */
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "FxTheme.h"
#include "FxController.h"
#include "CynAiHubCataloguePolicy.h"

class CynAiHubContent final : public juce::Component, private juce::ListBoxModel, private juce::Timer
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
        startTimer(400); // Live status: do not leave slow PCs stuck at "loading".
    }

    ~CynAiHubContent() override { stopTimer(); list_.setModel (nullptr); }

    void setEmbeddedMode(bool embedded)
    {
        // Same trusted ONNX/profile workflow, but hosted by Sound Studio shell.
        // The dialog-only Close control must not be displayed as a dead button.
        close_.setVisible(!embedded);
        if(embedded) {
            subtitle_.setText("Community AI, AUDONIC Studio and APEX 6M — native and private.",
                              juce::dontSendNotification);
        }
        resized();
    }

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
        juce::String id, name, author, category, description, filename, researchUrl;
        int issue = 0;
        bool researchOnly = false, nativeApex = false, nativeAudonic = false;
    };

    bool pendingApexActivation_=false;
    juce::String lastApexDiagnostic_;
    static constexpr int maxCatalogueBytes = 256 * 1024;
    static constexpr int maxModelBytes = 2 * 1024 * 1024;
    const juce::String rawBase_ =
        "https://raw.githubusercontent.com/oddfoxerr-real/Cyn/main/ai-hub/community/";

    juce::Label title_, subtitle_, status_;
    juce::ListBox list_;
    juce::TextEditor description_;
    juce::TextButton refresh_, import_, review_, publish_, close_;
    std::vector<Profile> profiles_;
    int catalogueTotal_ = 0, catalogueRejected_ = 0, catalogueExternal_ = 0;
    bool busy_ = false;

    int getNumRows() override { return static_cast<int> (profiles_.size()); }
    void selectedRowsChanged (int) override {
        // Changing selection cancels a pending user-approved activation.
        const auto* item=selected();
        if(!item || !item->nativeApex) pendingApexActivation_=false;
        updateSelection();
    }

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
        g.drawText (item.author + "  /  " + item.category, 12, 29,
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
            if(p->nativeAudonic)
            {
                auto& ctrl=FxController::getInstance();
                const bool active=ctrl.getSmartListeningMode()==0 &&
                                  ctrl.getActiveAIProfileId()=="audonic";
                description_.setText(
                    "AUDONIC STUDIO  /  WINDOWS NATIVE CREATIVE AI\n\n"
                    "Six adaptive creative channels: Harmonics, Attack, Width, Colour, "
                    "Rhythm and Overtones. The native FxSound DSP gets bounded real "
                    "adjustments, based on measured audio and optional YAMNet scenes.\n\n"
                    "Use the Creative intensity slider under Smart Listening. "
                    "BrainScan shows the actual six effect target values.\n\n"
                    "This is not the separately trained Android ONNX AUDONIC model. "
                    "Your previous music profile and private learning remain saved.\n\n"
                    + juce::String(active?"ACTIVE IN MUSIC AI":"Available offline"),
                    juce::dontSendNotification);
                import_.setButtonText(active?"AUDONIC active":"Activate AUDONIC");
            }
            else if (p->nativeApex)
            {
                auto& ctrl = FxController::getInstance();
                const bool installed = ctrl.apex6MInstalled();
                const bool ready = ctrl.apex6MReady();
                const bool failed = ctrl.apex6MFailed();
                const auto diagnostic = ctrl.apex6MDiagnostic();
                const bool activeAI = ctrl.getActiveAIProfileId() == "apex6m"
                    && ctrl.getSmartListeningMode() == 0;
                description_.setText (
                    "NEURAL MUSIC AI  /  CYN APEX 6M\n\n"
                    "6,307,477 actual trained ONNX parameters, built into Cyn. "
                    "Combines neural spectral analysis with adaptive EQ, Bass, "
                    "Dynamics, music passage tracking and Smart Listening.\n\n"
                    + (ready ? juce::String("MODEL READY\n\n")
                             : (failed ? juce::String("NEURAL ENGINE FAILED:\n") + diagnostic + "\n\n"
                                : (installed ? juce::String("LOADING NEURAL ENGINE...\n") + diagnostic + "\n\n"
                                             : juce::String("MODEL FILES MISSING:\n") + diagnostic + "\n\n")))
                    + (activeAI ? (ready ? juce::String("ACTIVE / Music AI selected.\n\n")
                                               : juce::String("APEX selected but inference is not ready.\n\n"))
                                : juce::String("Select this AI to use it for live music.\n\n"))
                    + "Training: original 225 synthetic steps. The model's full spectral "
                      "output currently steers real DSP controls; it does not "
                      "replace the audio buffer with offline denoised samples. "
                      "Listening-quality improvement has not yet been proven.\n\n"
                    + "Creator: " + p->author,
                    juce::dontSendNotification);
                import_.setButtonText (pendingApexActivation_ ? "Starting neural engine..."
                    : (activeAI && ready) ? "Switch to previous AI"
                    : failed ? "Retry APEX engine"
                    : installed ? "Activate selected AI" : "View model files");
            }
            else if (p->researchOnly)
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
        const int submittedCount = rows->size();
        int researchCount = 0;
        for (const auto& row : *rows)
        {
            if (! row.isObject()) continue;
            Profile p;
            p.id = row["id"].toString().substring (0, 90);
            p.name = row["name"].toString().substring (0, 80);
            p.author = row["author"].toString().substring (0, 60);
            p.category = row["category"].toString().substring (0, 30);
            p.description = row["description"].toString().substring (0, 900);
            p.filename = row["file"].toString();
            p.issue = juce::jlimit (0, 100000000, static_cast<int> (row["issue"]));
            p.researchOnly = row["kind"].toString() == "research";
            p.nativeApex = row["kind"].toString() == "onnx"
                && p.id == "cyn-apex6m-flowsense"
                && row["activation"].toString() == "apex6m";
            p.researchUrl = row["url"].toString();
            // Publish AI submissions may offer a full-model download (not .cynai).
            // Validate the actual official release URL with the native-tested policy.
            const bool trustedResearchUrl =
                cyn::isTrustedResearchReleaseUrl (p.researchUrl.toStdString());
            if (p.name.isNotEmpty()
                && ((p.researchOnly || p.nativeApex) ? trustedResearchUrl : isSafeFilename (p.filename)))
            {
                if (p.researchOnly) ++researchCount;
                collected.push_back (p);
            }
        }
        catalogueTotal_ = submittedCount;
        catalogueRejected_ = submittedCount - static_cast<int> (collected.size());
        catalogueExternal_ = researchCount;
        profiles_ = std::move (collected);
        ensureAudonicEntry();
        list_.updateContent();
        if (! profiles_.empty()) list_.selectRow (0);
        updateSelection();
        return true;
    }

    void ensureAudonicEntry()
    {
        for(const auto& p:profiles_)
            if(p.nativeAudonic)return;
        Profile p;
        p.id="audonic-studio";
        p.name="AUDONIC Studio / Creative AI";
        p.author="CYN";
        p.category="Local native creative DSP";
        p.description="Six bounded effects derived from measured audio and YAMNet scene analysis. "
                      "Preserves your personal learning. This is the Windows-native preview, "
                      "not the separate Android AUDONIC ONNX student.";
        p.nativeAudonic=true;
        profiles_.insert(profiles_.begin(),p);
    }

    void readBundledCatalogue()
    {
        const auto file = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
            .getSiblingFile ("CynAiProfiles/index.json");
        if (! file.existsAsFile() || file.getSize() > maxCatalogueBytes
            || ! loadCatalogue (juce::JSON::parse (file)))
        {
            profiles_.clear();
            ensureAudonicEntry();
            list_.updateContent();
            list_.selectRow(0);
            status_.setText ("Native AUDONIC Studio available. Community catalogue not bundled.",
                             juce::dontSendNotification);
        }
        else
        {
            status_.setText ("Loaded " + juce::String (profiles_.size())
                + " built-in profiles (" + juce::String (catalogueExternal_)
                + " external)."
                + (catalogueRejected_ > 0 ? " Some entries were rejected." : ""),
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
                    if (ok)
                    {
                        const auto accepted = view->catalogueTotal_ - view->catalogueRejected_;
                        view->status_.setText ("Community refreshed: "
                            + juce::String (accepted) + "/" + juce::String (view->catalogueTotal_)
                            + " listings (" + juce::String (view->catalogueExternal_)
                            + " external)."
                            + (view->catalogueRejected_ > 0
                                ? " " + juce::String (view->catalogueRejected_) + " rejected."
                                : " No account needed."),
                            juce::dontSendNotification);
                    }
                    else view->status_.setText (
                        "Offline: Could not refresh. Bundled profiles remain available.",
                        juce::dontSendNotification);
                }
            });
        });
    }

    void activateReadyApex()
    {
        auto& ctrl=FxController::getInstance();
        juce::String error;
        const bool success=ctrl.selectAIProfile ("apex6m", error);
        if(success) ctrl.setSmartListening(true);
        status_.setText(success ?
            "CYN APEX 6M active. Play music to start neural inference." :
            "APEX activation failed: "+error,juce::dontSendNotification);
        updateSelection();
    }

    void timerCallback() override
    {
        auto* item=selected();
        if(!item || !item->nativeApex) return;
        auto& ctrl=FxController::getInstance();
        const auto message=ctrl.apex6MDiagnostic();
        if(pendingApexActivation_)
        {
            if(ctrl.apex6MReady())
            {
                pendingApexActivation_=false;
                activateReadyApex();
            }
            else if(ctrl.apex6MFailed() || !ctrl.apex6MInstalled())
            {
                pendingApexActivation_=false;
                status_.setText("APEX could not start: "+message,
                                juce::dontSendNotification);
                updateSelection();
            }
        }
        if(message!=lastApexDiagnostic_) {
            lastApexDiagnostic_=message;
            updateSelection();
        }
    }

    void importSelected()
    {
        if (busy_) return;
        auto* item = selected();
        if (! item) return;
        if(item->nativeAudonic)
        {
            auto& ctrl=FxController::getInstance();
            if(!juce::AlertWindow::showOkCancelBox(
                   juce::AlertWindow::QuestionIcon,"Activate AUDONIC Studio?",
                   "Enable CYN's native creative DSP in Music AI? Your current "
                   "personal model remains intact. This is the Windows preview, "
                   "not the Android AUDONIC ONNX model.",
                   "Activate","Cancel"))return;
            if(ctrl.getSmartListeningMode()!=0)ctrl.setSmartListeningMode(0);
            if(ctrl.getSmartListeningMode()!=0) {
                status_.setText("Finish current training before selecting AUDONIC.",
                                juce::dontSendNotification);
                return;
            }
            juce::String error;
            const bool success=ctrl.selectAIProfile("audonic",error);
            if(success)ctrl.setSmartListening(true);
            status_.setText(success?"AUDONIC Studio active. Open Smart Listening for BrainScan."
                                    :"Activation failed: "+error,juce::dontSendNotification);
            updateSelection();
            return;
        }
        if (item->nativeApex)
        {
            auto& ctrl = FxController::getInstance();
            if(pendingApexActivation_) {
                status_.setText("APEX engine is initializing; activation will continue automatically.",
                    juce::dontSendNotification);
                return;
            }
            if (!ctrl.apex6MInstalled())
            {
                status_.setText ("APEX model unavailable: " + ctrl.apex6MDiagnostic(),
                    juce::dontSendNotification);
                updateSelection();
                return;
            }
            if (ctrl.getSmartListeningMode() == 0 &&
                ctrl.getActiveAIProfileId() == "apex6m" && ctrl.apex6MReady())
            {
                juce::String error;
                const bool success = ctrl.restorePreviousApexMusicAI(error);
                status_.setText (success ? "Previous Music AI restored."
                    : "Could not switch AI: " + error, juce::dontSendNotification);
                updateSelection();
                return;
            }
            if (! juce::AlertWindow::showOkCancelBox (
                    juce::AlertWindow::QuestionIcon, "Activate CYN APEX 6M?",
                    "Switch to Music AI and enable Smart Listening? APEX runs its "
                    "6.3M-parameter ONNX model on your PC and combines the result "
                    "with adaptive DSP. Your current trained models are preserved.",
                    "Activate", "Cancel")) return;
            if (ctrl.getSmartListeningMode() != 0)
                ctrl.setSmartListeningMode(0);
            if (ctrl.getSmartListeningMode() != 0)
            {
                status_.setText ("Finish current training before changing to Music AI.",
                    juce::dontSendNotification);
                return;
            }
            if(!ctrl.apex6MReady()) {
                // Slow Windows PCs may still be creating the ONNX session.
                // User-approved activation waits for a REAL ready engine;
                // never falsely claim neural processing while it is absent.
                ctrl.startApex6MIfAvailable();
                pendingApexActivation_=true;
                status_.setText("Starting APEX 6M neural engine; it will activate when ready.",
                                juce::dontSendNotification);
                updateSelection();
                return;
            }
            activateReadyApex();
            return;
        }
        if (item->researchOnly)
        {
            // Other models still require a compatible runtime.
            // This action NEVER invokes importSmartProfile or changes live audio.
            juce::URL (item->researchUrl).launchInDefaultBrowser();
            status_.setText ("Opened model files. Install a compatible Cyn runtime to activate.",
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
