/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)

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

#include "FxController.h"
#include "CynSmartRange.h"
#include "FxMainWindow.h"
#include "FxSystemTrayView.h"
#include "FxMessage.h"
#include "../Utils/SysInfo/SysInfo.h"
#include <iostream>
#include <cstdio>
#include <ctime>
#include <shellapi.h>
#ifndef WTS_SESSION_DESKTOP_READY
#define WTS_SESSION_DESKTOP_READY 0xF
#endif

namespace {
bool launchCynUpdater(bool interactive)
{
    const auto script=File::getSpecialLocation(File::currentExecutableFile)
        .getSiblingFile("CynUpdater.ps1");
    if(!script.existsAsFile()) {
        if(interactive)
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Cyn",
                "CynUpdater.ps1 is missing. Reinstall Cyn.");
        return false;
    }
    // Hiding the entire ShellExecute process can also hide the WPF update
    // prompt. CREATE_NO_WINDOW hides only the PowerShell console.
    wchar_t systemDirectory[MAX_PATH] {};
    const auto length=GetSystemDirectoryW(systemDirectory,MAX_PATH);
    if(length==0 || length>=MAX_PATH) return false;
    const auto powershell=File(String(systemDirectory))
        .getChildFile("WindowsPowerShell/v1.0/powershell.exe");
    if(!powershell.existsAsFile()) return false;
    // Read the actual native executable version (JUCE_APP_VERSION); never
    // hard-code an older release or Cyn will repeatedly offer itself as an update.
    const auto app=JUCEApplication::getInstance();
    if(!app) return false;
    const auto installed=StringArray::fromTokens(app->getApplicationVersion(),".","");
    if(installed.size()<3) return false;
    const auto major=installed[0],minor=installed[1],patch=installed[2];
    if(!major.containsOnly("0123456789") || !minor.containsOnly("0123456789") ||
       !patch.containsOnly("0123456789")) return false;
    const auto currentVersion=major+"."+minor+"."+patch;
    const auto command=String("\"")+powershell.getFullPathName()+
        "\" -NoProfile -NonInteractive -STA -ExecutionPolicy Bypass -File \""+
        script.getFullPathName()+"\" -CurrentVersion "+currentVersion+
        (interactive?" -Interactive":"");
    const std::wstring wideCommand(command.toWideCharPointer());
    std::vector<wchar_t> writable(wideCommand.begin(),wideCommand.end());
    writable.push_back(0);
    STARTUPINFOW startup {};
    startup.cb=sizeof(startup);
    PROCESS_INFORMATION process {};
    const BOOL launched=CreateProcessW(
        powershell.getFullPathName().toWideCharPointer(),
        writable.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,
        nullptr,nullptr,&startup,&process);
    if(!launched) {
        if(interactive)
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Cyn",
                "Unable to launch the update checker.");
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}
}

class FxDeviceErrorMessage : public FxWindow
{
public:
	FxDeviceErrorMessage()
	{
		setContent(&message_content_);
		centreWithSize(getWidth(), getHeight());
		addToDesktop(0);
		setAlwaysOnTop(true);
	}
	~FxDeviceErrorMessage() = default;

	void closeButtonPressed() override
	{
		exitModalState(0);
		removeFromDesktop();
	}

private:
	class MessageComponent : public Component
	{
	public:
		MessageComponent()
		{
			auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

			error_message_.setFont(theme.getNormalFont());
			contact_message_.setFont(theme.getNormalFont());

			error_message_.setText(TRANS("Oops! There's an issue with your playback device settings.\r\nBefore we can get started, please go through the "), NotificationType::dontSendNotification);
			error_message_.setJustificationType(Justification::topLeft);
			addAndMakeVisible(error_message_);

			error_link_.setButtonText(TRANS("troubleshooting steps here."));
			error_link_.setURL(URL(L"https://www.fxsound.com/learning-center/installation-troubleshooting"));
			error_link_.setJustificationType(Justification::topLeft);
			addAndMakeVisible(error_link_);

			contact_message_.setText(TRANS(" if you're still having problems."), NotificationType::dontSendNotification);
			contact_message_.setJustificationType(Justification::topLeft);
			addAndMakeVisible(contact_message_);

			contact_link_.setButtonText(TRANS("Contact us"));
			contact_link_.setURL(URL(L"https://www.fxsound.com/support"));
			contact_link_.setJustificationType(Justification::topLeft);
			addAndMakeVisible(contact_link_);

			setSize(WIDTH, HEIGHT);
		}
		~MessageComponent() = default;

	private:
		static constexpr int WIDTH = 400;
		static constexpr int HEIGHT = 142;
		static constexpr int MESSAGE_HEIGHT = (24 + 2) * 2;
		static constexpr int HYPERLINK_HEIGHT = 24;

		void resized() override
		{
			auto bounds = getLocalBounds();
			bounds.setTop(10);
			bounds.reduce(20, 0);

			RectanglePlacement placement(RectanglePlacement::xLeft
				| RectanglePlacement::yTop
				| RectanglePlacement::doNotResize);

			auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), MESSAGE_HEIGHT);
			error_message_.setBounds(placement.appliedTo(component_area, bounds));
			auto x = error_message_.getX() + error_message_.getBorderSize().getLeft();

			bounds.setTop(error_message_.getBottom() + 2);
			component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), HYPERLINK_HEIGHT);
			error_link_.setBounds(placement.appliedTo(component_area, bounds).withX(x));

			bounds.setTop(error_link_.getBottom() + 20);
			component_area = juce::Rectangle<int>(0, 0, contact_link_.getTextWidth(), HYPERLINK_HEIGHT);
			contact_link_.setBounds(placement.appliedTo(component_area, bounds).withX(x));

			bounds = juce::Rectangle<int>(contact_link_.getRight(), contact_link_.getY(), error_message_.getWidth() - contact_link_.getWidth(), MESSAGE_HEIGHT / 2);
			contact_message_.setBounds(bounds);
		}

		Label error_message_;
		Label contact_message_;
		FxHyperlink error_link_;
		FxHyperlink contact_link_;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MessageComponent)
	};

	MessageComponent message_content_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxDeviceErrorMessage)
};

FxController::FxController() : message_window_(L"CynHotkeys", (WNDPROC)eventCallback)
{
	dfx_enabled_ = true;
	authenticated_ = true;
	minimize_tip_ = true;

	hotkeys_registered_ = false;
	output_changed_ = false;
	playback_device_available_ = true;
	powerNotify_ = nullptr;
	unregister_suspend_resume_notification_ = nullptr;

	device_count_ = 0;
	output_device_name_ = L"";

	audio_process_time_ = 0;
	audio_process_on_counter_ = 0;
	audio_process_off_counter_ = 0;
	audio_process_on_ = false;

	audio_process_start_time_ = -1LL;

	preset_dirty_ = false;
	auto_save_counter_ = 0;

	main_window_ = nullptr;
	audio_passthru_ = nullptr;

	file_logger_.reset(FileLogger::createDefaultAppLogger(L"Cyn", L"cyn.log", L"Cyn logs"));
	logMessage("v" + JUCEApplication::getInstance()->getApplicationVersion());
	logMessage(SystemStats::getOperatingSystemName());

	SYSTEM_INFO sys_info;
	GetNativeSystemInfo(&sys_info);
	if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	{
		logMessage(String("x86"));
	}
	else if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
	{
		logMessage(String("x64"));
	}
	else if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
	{
		logMessage(String("ARM64"));
	}

	auto view = settings_.getInt("view");
	if (view <= 0 || view > 2)
	{
		view_ = ViewType::Pro;
	}
	else
	{
		view_ = static_cast<ViewType>(view);
	}
	auto hotkeys_support = settings_.getBool("hotkeys") && SysInfo::canSupportHotkeys();
	FxModel::getModel().setHotkeySupport(hotkeys_support);
	if (hotkeys_support)
	{
		registerHotkeys();
	}
	FxModel::getModel().setMenuClicked(settings_.getBool("menu_clicked"));

	always_on_top_ = settings_.getBool("always_on_top");
	hide_help_tooltips_ = settings_.getBool("hide_help_tooltips");
	hide_notifications_ = settings_.getBool("hide_notifications");
	auto_updates_ = settings_.getBool("cyn_automatic_updates", true);
	max_user_presets_ = settings_.getInt("max_user_presets");
	if (max_user_presets_ < 10 || max_user_presets_ > 120)
	{
		settings_.setInt("max_user_presets", 120);
		max_user_presets_ = 120;
	}

	SetWindowLongPtr(message_window_.getHandle(), GWLP_USERDATA, (LONG_PTR)this);

	session_id_ = 0;
	ProcessIdToSessionId(GetCurrentProcessId(), &session_id_);
	WTSRegisterSessionNotification(message_window_.getHandle(), NOTIFY_FOR_THIS_SESSION);
}

FxController::~FxController()
{
    auto_update_poller_.stop();
    audioRepair_.reset();
    // Stop asynchronous inference before the audio DSP is destroyed.
    dj_worker_.reset();
    apex_worker_.reset();
    yamnet_worker_.reset();
	if (powerNotify_ != nullptr && unregister_suspend_resume_notification_ != nullptr)
	{
		unregister_suspend_resume_notification_(powerNotify_);
		powerNotify_ = nullptr;
	}
	stopTimer();
	WTSUnRegisterSessionNotification(message_window_.getHandle());
	unregisterHotkeys();
}


void FxController::initConfig(const String& commandline)
{
    const auto saved_value = settings_.getString("ember_smart_sensitivity");
    const auto saved_sensitivity = saved_value.isEmpty() ? 50.0f : saved_value.getFloatValue();
    smart_sensitivity_ = std::isfinite(saved_sensitivity) ? jlimit(0.0f, 100.0f, saved_sensitivity) : 50.0f;
    const auto saved_headroom=settings_.getString("cyn_music_headroom_cap");
    const float headroom_value=saved_headroom.isEmpty()?50.0f:saved_headroom.getFloatValue();
    smart_headroom_cap_=std::isfinite(headroom_value)
        ? jlimit(0.0f,100.0f,headroom_value):50.0f;
    smart_preset_cycling_=settings_.getBool("cyn_auto_preset_cycling",false);
	auto arg_list = ArgumentList(File::getSpecialLocation(File::SpecialLocationType::invokedExecutableFile).getFileName(), commandline);
    auto requested_theme=arg_list.getValueForOption("--theme").unquoted();
    for(int i=0;i<(int)FxThemeMode::NumModes;++i)
        if(requested_theme.equalsIgnoreCase(FxTheme::getThemeName(i))) setThemeMode(static_cast<FxThemeMode>(i));
    auto reduced=arg_list.getValueForOption("--reduce_motion");
    if(reduced=="0" || reduced=="1") setReducedMotion(reduced=="1");
    auto sensitivity = arg_list.getValueForOption("--smart_sensitivity");
    if (sensitivity.isNotEmpty()) setSmartSensitivity(sensitivity.getFloatValue());
    auto smart = arg_list.getValueForOption("--smart_listening");
    if (smart == "0" || smart == "1") settings_.setBool("ember_smart_listening", smart == "1");
    auto pinned = arg_list.getValueForOption("--pin_output");
    if (pinned == "0" || pinned == "1") setOutputPinned(pinned == "1");

	auto power_state = arg_list.getValueForOption("--power");
	auto preset = arg_list.getValueForOption("--preset").unquoted();
	auto view = arg_list.getValueForOption("--view");
	auto output_device = arg_list.getValueForOption("--output").unquoted();
	auto language = arg_list.getValueForOption("--language");
	auto numbands = arg_list.getValueForOption("--num_bands");
	auto balance = arg_list.getValueForOption("--balance");
	auto filterq = arg_list.getValueForOption("--filter_q");
	auto mastergain = arg_list.getValueForOption("--master_gain");
	auto volume_leveling = arg_list.getValueForOption("--volume_leveling");

	if (arg_list.containsOption("--run_minimized"))
	{
		settings_.setBool("run_minimized", true);
	}

	if (power_state.isNotEmpty())
	{
		if (power_state.getIntValue() == 0)
			settings_.setBool("power", false);
		else
			settings_.setBool("power", true);
	}

	if (preset.isNotEmpty())
	{
		settings_.setString("preset", preset);
	}

	if (output_device.isNotEmpty())
	{
		setOutputName(output_device);
	}

	if (view.isNotEmpty())
	{
		auto value = view.getIntValue();
		if (value == ViewType::Lite || value == ViewType::Pro)
		{
			settings_.setInt("view", value);
			view_ = static_cast<ViewType>(value);
		}
	}

	if (language.isEmpty())
	{
		language = settings_.getString("language");
		if (language.isEmpty())
		{
			language = SystemStats::getDisplayLanguage();
		}
	}

	setLanguage(language);

	// ------------------------------------------------------------------------------------------
	//  NumBands / Balance / FilterQ / MasterGain  / Volume Leveling - SET ON START
	// ------------------------------------------------------------------------------------------
	int nb = DEFAULT_NUM_EQ_BANDS;
	if (numbands == "")
	{
		nb = settings_.getInt("num_bands");
	}
	else
	{
		nb = numbands.getIntValue();
	}
	if (nb != 31 && nb != 35 && nb != 39) nb = DEFAULT_NUM_EQ_BANDS;
	setNumEqBands(nb);

	float vl = 0;
	if (volume_leveling.isEmpty())
	{
		vl = settings_.getDouble("volume_leveling");
	}
	else
	{
		vl = volume_leveling.getFloatValue();
	}
	if (vl < 0 || vl > 4) vl = DEFAULT_VOLUME_LEVELING;
	setVolumeLeveling(vl);

	float bl = 0;
	if (balance == "")
	{
		bl = settings_.getDouble("balance");
	}
	else
	{
		bl = balance.getFloatValue();
	}
	if (bl < -20 || bl > +20) bl = DEFAULT_BALANCE;
	setBalance(bl);

	float fq = 0;
	if (filterq == "")
	{
		fq = settings_.getDouble("filter_q");
	}
	else
	{
		fq = filterq.getFloatValue();
	}
	if (fq < 1 || fq > 3) fq = DEFAULT_FILTER_Q;
	setFilterQ(fq);

	float mg = 0;
	if (mastergain == "")
	{
		mg = settings_.getDouble("master_gain");
	}
	else
	{
		mg = mastergain.getFloatValue();
	}
	if (mg < -20 || mg > +20) mg = DEFAULT_MASTER_GAIN;
	setMasterGain(mg);
}

void FxController::applyConfig(const String& commandline)
{
	auto arg_list = ArgumentList(File::getSpecialLocation(File::SpecialLocationType::invokedExecutableFile).getFileName(), commandline);
    if (arg_list.containsOption("--cyn-exit-for-update"))
    {
        // Close through the existing JUCE single-instance channel, preserving presets.
        exit();
        return;
    }

	if (arg_list.containsOption("--status"))
	{
		printStatus();
		return;
	}

    auto requested_theme=arg_list.getValueForOption("--theme").unquoted();
    for(int i=0;i<(int)FxThemeMode::NumModes;++i)
        if(requested_theme.equalsIgnoreCase(FxTheme::getThemeName(i))) setThemeMode(static_cast<FxThemeMode>(i));
    auto reduced=arg_list.getValueForOption("--reduce_motion");
    if(reduced=="0" || reduced=="1") setReducedMotion(reduced=="1");
    auto sensitivity = arg_list.getValueForOption("--smart_sensitivity");
    if (sensitivity.isNotEmpty()) setSmartSensitivity(sensitivity.getFloatValue());
    auto smart = arg_list.getValueForOption("--smart_listening");
    if (smart == "0" || smart == "1") setSmartListening(smart == "1");
    auto pinned = arg_list.getValueForOption("--pin_output");
    if (pinned == "0" || pinned == "1") setOutputPinned(pinned == "1");
	auto power_state = arg_list.getValueForOption("--power");
	auto view = arg_list.getValueForOption("--view");
	auto output_device = arg_list.getValueForOption("--output").unquoted();
	auto language = arg_list.getValueForOption("--language");
	auto numbands = arg_list.getValueForOption("--num_bands");
	auto balance = arg_list.getValueForOption("--balance");
	auto filterq = arg_list.getValueForOption("--filter_q");
	auto mastergain = arg_list.getValueForOption("--master_gain");
	auto volume_leveling = arg_list.getValueForOption("--volume_leveling");
	auto band_freq = arg_list.getValueForOption("--set_band_freq").unquoted();
	auto band_gain = arg_list.getValueForOption("--set_band_gain").unquoted();
	auto effect = arg_list.getValueForOption("--set_effect").unquoted();

	if (power_state.isNotEmpty() && !SysInfo::isRemoteSession())
	{
		if (power_state.getIntValue() == 0)
			setPowerState(false);
		else
			setPowerState(true);
	}

	auto& model = FxModel::getModel();

	auto sanitizePresetName = [&model](const String& name) -> String {
		String preset_name = name.removeCharacters("<>:\"/\\|?*");

		if (preset_name.length() > 64)
		{
			preset_name = preset_name.substring(0, 64);
		}

		if (preset_name.isNotEmpty() && model.isPresetNameValid(preset_name))
		{
			return preset_name;
		}

		return "";
    };

	if (model.getPowerState())
	{
		if (arg_list.containsOption("--preset"))
		{
			if (auto preset_name = arg_list.getValueForOption("--preset").unquoted(); preset_name.isNotEmpty())
			{
				setPreset(preset_name);
			}
		}
		else if (arg_list.containsOption("--save_preset"))
		{
			auto arg_val = arg_list.getValueForOption("--save_preset").unquoted();
			if (auto preset_name = sanitizePresetName(arg_val); preset_name.isNotEmpty())
			{
				if (model.isPresetModified() && model.getUserPresetCount() < getMaxUserPresets())
				{
					savePreset(preset_name);
				}
			}
		}
		else if (arg_list.containsOption("--overwrite_preset"))
		{
			auto preset = model.getPreset(model.getSelectedPreset());
			if (model.isPresetModified() && preset.type == FxModel::PresetType::UserPreset)
			{
				savePreset();
			}
		}
		else if (arg_list.containsOption("--undo_preset"))
		{
			if (model.isPresetModified())
			{
                undoPreset();
			}
        }
		else if (arg_list.containsOption("--rename_preset"))
		{
			auto arg_val = arg_list.getValueForOption("--rename_preset").unquoted();

			if (auto preset_name = sanitizePresetName(arg_val); preset_name.isNotEmpty())
			{
				auto preset = model.getPreset(model.getSelectedPreset());
				if (!model.isPresetModified() && preset.type == FxModel::PresetType::UserPreset)
				{
					renamePreset(preset_name);
				}
			}
		}
		else if (arg_list.containsOption("--delete_preset"))
		{
			auto preset = model.getPreset(model.getSelectedPreset());
			if (preset.type == FxModel::PresetType::UserPreset)
			{
				deletePreset();
			}
		}
	}

	if (output_device.isNotEmpty())
	{
		setOutputName(output_device);
		std::vector<SoundDevice> sound_devices = audio_passthru_->getSoundDevices();
		for (auto& sound_device : sound_devices)
		{
			if (output_device == sound_device.deviceFriendlyName.c_str())
			{
				setOutput(sound_device.pwszID.c_str());
			}
		}
	}

	// ------------------------------------------------------------------------------------------
	//  NumBands / Balance / FilterQ / MasterGain / Volume Leveling
	// ------------------------------------------------------------------------------------------
	int nb = DEFAULT_NUM_EQ_BANDS;
	if (numbands.isNotEmpty())
	{
		nb = numbands.getIntValue();
		if (nb != 31 && nb != 35 && nb != 39) nb = DEFAULT_NUM_EQ_BANDS;
		setNumEqBands(nb);
	}

	float vl = 0;
	if (volume_leveling.isNotEmpty())
	{
		vl = volume_leveling.getFloatValue();
		if (vl < 0 || vl > 4) vl = DEFAULT_VOLUME_LEVELING;
		setVolumeLeveling(vl);
	}

	float bl = 0;
	if (balance.isNotEmpty())
	{
		bl = balance.getFloatValue();
		if (bl < -20 || bl > +20) bl = DEFAULT_BALANCE;
		setBalance(bl);
	}

	float fq = 0;
	if (filterq.isNotEmpty())
	{
		fq = filterq.getFloatValue();
		if (fq < 1 || fq > 3) fq = DEFAULT_FILTER_Q;
		setFilterQ(fq);
	}

	float mg = 0;
	if (mastergain.isNotEmpty())
	{
		mg = mastergain.getFloatValue();
		if (mg < -20 || mg > +20) mg = DEFAULT_MASTER_GAIN;
		setMasterGain(mg);
	}

	if (view.isNotEmpty())
	{
		auto value = view.getIntValue();
		if (value == ViewType::Lite || value == ViewType::Pro)
		{
			settings_.setInt("view", value);
			view_ = static_cast<ViewType>(value);
		}
		showView();
	}

	if (language.isNotEmpty())
	{
		setLanguage(language);
	}

	if (arg_list.containsOption("--run_minimized"))
	{
		settings_.setBool("run_minimized", true);
		hideMainWindow();
	}
	else
	{
		showMainWindow();
	}

	// ------------------------------------------------------------------------------------------
	//  Band frequency / gain / effect values - "<index>:<value>[,<index>:<value>...]"
	// ------------------------------------------------------------------------------------------
	if (band_freq.isNotEmpty())
	{
        auto band_frequency_pairs = StringArray::fromTokens(band_freq, ",", "\"");
		if (band_frequency_pairs.size() <= getNumEqBands())
		{
			for (auto& pair : band_frequency_pairs)
			{
				auto band = pair.upToFirstOccurrenceOf(":", false, false).trim().getIntValue();
				auto freq = pair.fromFirstOccurrenceOf(":", false, false).trim().getFloatValue();
				setEqBandFrequency(band, freq);
			}
        }

		if (view_ == ViewType::Pro)
		{
			main_window_->update();
		}
	}

	if (band_gain.isNotEmpty())
	{
		auto band_gain_pairs = StringArray::fromTokens(band_gain, ",", "\"");
		if (band_gain_pairs.size() <= getNumEqBands())
		{
			for (auto& pair : band_gain_pairs)
			{
				auto band = pair.upToFirstOccurrenceOf(":", false, false).trim().getIntValue();
				auto gain = pair.fromFirstOccurrenceOf(":", false, false).trim().getFloatValue();
				setEqBandBoostCut(band, gain);
			}
		}

		if (view_ == ViewType::Pro)
		{
			main_window_->update();
		}
	}

	if (effect.isNotEmpty())
	{
        auto effect_pairs = StringArray::fromTokens(effect, ",", "\"");
		if (effect_pairs.size() <= 5)
		{
			for (auto& pair : StringArray::fromTokens(effect, ",", "\""))
			{
				auto name = pair.upToFirstOccurrenceOf(":", false, false).trim().toLowerCase();
				auto value = pair.fromFirstOccurrenceOf(":", false, false).trim().getFloatValue();

				if (name == "fidelity" || name == "clarity")
					setEffectValue(FxEffects::Fidelity, value);
				else if (name == "ambience")
					setEffectValue(FxEffects::Ambience, value);
				else if (name == "surround")
					setEffectValue(FxEffects::Surround, value);
				else if (name == "dynamicboost" || name == "dynamic_boost")
					setEffectValue(FxEffects::DynamicBoost, value);
				else if (name == "bass" || name == "bassboost" || name == "bass_boost")
					setEffectValue(FxEffects::Bass, value);
			}

			if (view_ == ViewType::Pro)
			{
				main_window_->update();
			}
		}
	}
}

File FxController::getStatusFile()
{
	return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Cyn").getChildFile("status.json");
}

void FxController::printStatus()
{
	auto& model = FxModel::getModel();

	auto* settings = new DynamicObject();
	settings->setProperty("version", ProjectInfo::versionString);
	settings->setProperty("power", model.getPowerState());
    settings->setProperty("cyn_build", "identity-2");
    settings->setProperty("theme", FxTheme::getThemeName((int)FxTheme::getThemeMode()));
    settings->setProperty("reduce_motion", FxTheme::isReducedMotion());
    auto* smart = new DynamicObject();
    smart->setProperty("enabled", smart_enabled_);
    smart->setProperty("sensitivity", smart_sensitivity_);
    smart->setProperty("headroom_cap",smart_headroom_cap_);
    smart->setProperty("preset_cycling",smart_preset_cycling_);
    smart->setProperty("cycling_preset",smart_cycling_preset_name_);
    smart->setProperty("summary", getSmartSummary());
    smart->setProperty("model", "local 11-12-5 neural preference model");
    smart->setProperty("learning", getLearningStatus());
    smart->setProperty("bass_preference", smart_scores_[4]);
    smart->setProperty("clarity_preference", smart_scores_[0]);
    smart->setProperty("effective_bass", dfx_dsp_.getEffectValue(DfxDsp::Effect::Bass) * 10.0f);
    smart->setProperty("effective_clarity", dfx_dsp_.getEffectValue(DfxDsp::Effect::Fidelity) * 10.0f);
    smart->setProperty("effective_leveling", dfx_dsp_.getVolumeLeveling());
    settings->setProperty("smart_listening", var(smart));
    if(dj_worker_) {
        auto* dj=new DynamicObject();dj->setProperty("enabled",dj_worker_->enabled());
        dj->setProperty("status",dj_worker_->status());dj->setProperty("accents",int(dj_worker_->effects()));
        dj->setProperty("remixes",int(dj_worker_->effects()));dj->setProperty("audio_processing",audio_process_on_);
        dj->setProperty("intensity",dj_worker_->intensity());dj->setProperty("spacing",dj_worker_->spacing());
        dj->setProperty("background_analyses",int(dj_worker_->analyses()));dj->setProperty("last_effect",dj_worker_->lastEffect());
        settings->setProperty("ai_dj",var(dj));
    }
    settings->setProperty("output_pinned", isOutputPinned());

	Array<var> built_in_presets, user_presets;
	for (int i = 0; i < model.getPresetCount(); i++)
	{
		auto preset = model.getPreset(i);

		auto* preset_obj = new DynamicObject();
		preset_obj->setProperty("name", preset.name);
		preset_obj->setProperty("modified", preset.modified);

		if (preset.type == FxModel::PresetType::UserPreset)
			user_presets.add(var(preset_obj));
		else
			built_in_presets.add(var(preset_obj));
	}

	auto* presets_obj = new DynamicObject();
	presets_obj->setProperty("built_in", built_in_presets);
	presets_obj->setProperty("user_defined", user_presets);
	settings->setProperty("presets", var(presets_obj));
	settings->setProperty("selected_preset", model.getPreset(model.getSelectedPreset()).name);

	Array<var> output_devices;
	for (auto& device : audio_passthru_->getSoundDevices())
	{
		if (device.isRealDevice)
			output_devices.add(String(device.deviceFriendlyName.c_str()));
	}
	settings->setProperty("output_devices", output_devices);
	settings->setProperty("selected_output", getOutputName());

	Array<var> bands;
	for (int i = 0; i < getNumEqBands(); i++)
	{
		auto* band_obj = new DynamicObject();
		band_obj->setProperty("index", i);
		band_obj->setProperty("frequency", getEqBandFrequency(i));
		band_obj->setProperty("gain", getEqBandBoostCut(i));
        band_obj->setProperty("effective_gain", dfx_dsp_.getEqBandBoostCut(i));
		bands.add(var(band_obj));
	}

	auto* eq_obj = new DynamicObject();
	eq_obj->setProperty("num_bands", getNumEqBands());
	eq_obj->setProperty("master_gain", getMasterGain());
	eq_obj->setProperty("volume_leveling", getVolumeLeveling());
	eq_obj->setProperty("filter_q", getFilterQ());
	eq_obj->setProperty("balance", getBalance());
	eq_obj->setProperty("bands", bands);
	settings->setProperty("equalizer", var(eq_obj));

	auto* effects_obj = new DynamicObject();
	effects_obj->setProperty("clarity", getEffectValue(FxEffects::Fidelity) * 10.0f);
	effects_obj->setProperty("ambience", getEffectValue(FxEffects::Ambience) * 10.0f);
	effects_obj->setProperty("surround", getEffectValue(FxEffects::Surround) * 10.0f);
	effects_obj->setProperty("dynamicboost", getEffectValue(FxEffects::DynamicBoost) * 10.0f);
	effects_obj->setProperty("bass", getEffectValue(FxEffects::Bass) * 10.0f);
	settings->setProperty("effects", var(effects_obj));

	auto json_text = JSON::toString(var(settings));

	auto status_file = getStatusFile();
	status_file.getParentDirectory().createDirectory();
	status_file.replaceWithText(json_text);

	// Best-effort console output for interactive use. Attaches to this
	// (already-running) instance's own parent console - i.e. whichever
	// console FxSound.exe was originally launched from - not the console of
	// the process that just forwarded this command.
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		FILE* fp = nullptr;
		freopen_s(&fp, "CONOUT$", "w", stdout);

		std::cout << json_text << std::endl;
		std::cout.flush();

		FreeConsole();
	}
}

void FxController::init(FxMainWindow* main_window, FxSystemTrayView* system_tray_view, AudioPassthru* audio_passthru)
{
	if (!isTimerRunning())
	{
		main_window_ = main_window;
		audio_passthru_ = audio_passthru;
		system_tray_view_ = system_tray_view;

		if (audio_passthru_->init() != 0)
		{
			String message(TRANS("Error in system audio configuration. Unable to run Cyn"));
			AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), message, TRANS("OK"));

			JUCEApplication::getInstance()->systemRequestedQuit();
			return;
		}

		auto app_version = JUCEApplication::getInstance()->getApplicationVersion();
		auto prev_version = settings_.getString("version");
		if (prev_version != app_version)
		{
			RegDeleteTree(HKEY_CURRENT_USER, L"Software\\DFX");

			FxModel::getModel().pushMessage("Cyn / Personal audio");

			settings_.setString("version", app_version);
			settings_.setBool("run_minimized", false);
		}

		audio_passthru_->setDspProcessingModule(&dfx_dsp_);
		output_devices_ = audio_passthru_->getSoundDevices(false);
		initOutputs(output_devices_);

		if (!dfx_enabled_ && !SysInfo::isRemoteSession())
		{
			main_window_->removeFromDesktop();
			FxDeviceErrorMessage error_message;
			error_message.runModalLoop();
			JUCEApplication::getInstance()->systemRequestedQuit();
			return;
		}

		audio_passthru_->registerCallback(this);

		auto path = File(File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"Cyn\\Presets");
		if (!path.exists())
		{
			path.createDirectory();
		}

		setPowerState(settings_.getBool("power"));

		initPresets();

		auto preset_name = settings_.getString("preset");
		setPreset(preset_name);
        initialiseMovieStarter();
        loadSmartModel();
        yamnet_worker_=std::make_unique<CynYamNetWorker>(dfx_dsp_);
        yamnet_worker_->startIfAvailable();
        apex_worker_=std::make_unique<CynApex6MWorker>(dfx_dsp_);
        apex_worker_->startIfAvailable();
        dj_worker_=std::make_unique<CynDjWorker>(dfx_dsp_);
        smart_mode_=jlimit(0,1,settings_.getInt("cyn_ai_listening_mode",0));
        // Autonomous YAMNet + Balance is on for new installs; retain a saved opt-out.
        // Keep smart_enabled_ initially false so enabling captures the preset baseline.
        setSmartListening(settings_.getBool("ember_smart_listening", true));

		showView();

		FxTheme::setReducedMotion(settings_.getBool("cyn_reduce_motion",false));
        FxTheme::setSmoothAnimations(settings_.getBool("cyn_smooth_animations", true));
        auto theme_mode = settings_.getInt("theme_mode", 0);
		if (theme_mode < 0 || theme_mode >= static_cast<int>(FxThemeMode::NumModes))
		{
			theme_mode = 0;
		}
		if (FxTheme::getThemeMode() != static_cast<FxThemeMode>(theme_mode))
		{
			FxTheme::setThemeMode(static_cast<FxThemeMode>(theme_mode));
			main_window_->sendLookAndFeelChange();

			auto* theme = dynamic_cast<FxTheme*>(&LookAndFeel::getDefaultLookAndFeel());
			if (theme != nullptr)
			{
				theme->loadFont(language_);
			}
		}

		survey_tip_ = false; // Upstream survey does not apply to Cyn.

		if (!settings_.getBool("run_minimized"))
		{
			showMainWindow();
		}
		else
		{
			hideMainWindow();
		}

		auto power = FxModel::getModel().getPowerState();
		main_window_->setIcon(power, false);
		system_tray_view_->setStatus(power, false);

        // Check GitHub on the UI timer whether sound processing is on or off.
        // Saved cadence prevents repeated checks on repeated app restarts.
        auto_update_poller_.start();
        // Separate opt-in monitoring continues when the DSP is switched off.
        audioRepair_ = std::make_unique<CynAudioRepairService>(
            settings_, [this]() {
                checkDeviceChanges();
                refreshOutputList();
            }, [this]() -> juce::String {
                const auto savedOutput = getOutputName();
                return savedOutput.isNotEmpty() && !isOutputDeviceConnected(savedOutput)
                    ? savedOutput : juce::String();
            });
        audioRepair_->startIfEnabled();

		HMODULE user32_module = GetModuleHandleW(L"user32.dll");
		if (user32_module != nullptr)
		{
			auto register_suspend_resume_notification = reinterpret_cast<RegisterSuspendResumeNotificationFunc>(
				GetProcAddress(user32_module, "RegisterSuspendResumeNotification"));
			unregister_suspend_resume_notification_ = reinterpret_cast<UnregisterSuspendResumeNotificationFunc>(
				GetProcAddress(user32_module, "UnregisterSuspendResumeNotification"));

			if (register_suspend_resume_notification != nullptr && unregister_suspend_resume_notification_ != nullptr)
			{
				powerNotify_ = register_suspend_resume_notification(message_window_.getHandle(), DEVICE_NOTIFY_WINDOW_HANDLE);
			}
		}
	}
}

String FxController::getAutoSavePath() const
{
	auto data_dir = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName());
	return data_dir + L"Cyn\\AutoSave";
}

String FxController::getAutoSavePresetPath(const String& preset_name) const
{
	return getAutoSavePath() + L"\\" + preset_name + ".fac";
}

void FxController::autoSavePreset(int preset_index)
{
	ScopedLock lock(save_lock_);

	auto preset = FxModel::getModel().getPreset(preset_index);
	if (preset.name.isEmpty())
		return;

	auto auto_save_dir = getAutoSavePath();
	File(auto_save_dir).createDirectory();

	saveManualPreset(preset.name.toWideCharPointer(), auto_save_dir.toWideCharPointer());

	preset_dirty_ = false;
	auto_save_counter_ = 0;
}

void FxController::deleteAutoSavedPreset(const String& preset_name)
{
	File auto_save_file(getAutoSavePresetPath(preset_name));
	if (auto_save_file.existsAsFile())
		auto_save_file.deleteFile();

	preset_dirty_ = false;
	auto_save_counter_ = 0;
}

void FxController::initPresets()
{
	Array<FxModel::Preset> presets;
	auto working_dir = File::getCurrentWorkingDirectory();
	FileSearchPath preset_search_path(File::addTrailingSeparator(working_dir.getFullPathName()) + L"Factsoft");
	auto app_preset_paths = preset_search_path.findChildFiles(File::findFiles, false, "*.fac");
	for (auto path : app_preset_paths)
	{
		auto preset_info = dfx_dsp_.getPresetInfo(path.getFullPathName().toWideCharPointer());
		if (!preset_info.name.empty())
		{
			presets.add({ preset_info.name.c_str(), path.getFullPathName(), FxModel::PresetType::AppPreset });
		}
	}

	auto data_dir = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName());
	FileSearchPath user_preset_search_path(data_dir + L"Cyn\\Presets");
	auto user_preset_paths = user_preset_search_path.findChildFiles(File::findFiles, false, "*.fac");
	for (auto path : user_preset_paths)
	{
		auto preset_info = dfx_dsp_.getPresetInfo(path.getFullPathName().toWideCharPointer());
		if (!preset_info.name.empty())
		{
			presets.add({ preset_info.name.c_str(), path.getFullPathName(), FxModel::PresetType::UserPreset });
		}
	}

	// Mark presets that have an auto-saved version as modified
	for (auto& preset : presets)
	{
		if (File(getAutoSavePresetPath(preset.name)).existsAsFile())
			preset.modified = true;
	}

	FxModel::getModel().initPresets(presets);
}

void FxController::showView()
{
	if (view_ == ViewType::Pro)
	{
		main_window_->showProView();
	}
	else
	{
		main_window_->showLiteView();
	}
}

void FxController::switchView()
{
	if (view_ == ViewType::Pro)
	{
		view_ = ViewType::Lite;
		main_window_->showLiteView();
		settings_.setInt("view", static_cast<int>(view_));
	}
	else
	{
		view_ = ViewType::Pro;
		main_window_->showProView();
		settings_.setInt("view", static_cast<int>(view_));
	}
}

ViewType FxController::getCurrentView()
{
	return view_;
}

void FxController::hideMainWindow()
{
	if (main_window_->isOnDesktop())
	{
		main_window_->removeFromDesktop();
		main_window_->setVisible(false);
		settings_.setBool("run_minimized", true);
	}

	if (minimize_tip_)
	{
		minimize_tip_ = false;
		Timer::callAfterDelay(2000, []() {
			FxModel::getModel().pushMessage(TRANS("Cyn in system tray\r\nClick Cyn icon to reopen"));
			});
	}
}

void FxController::showMainWindow()
{
	if (main_window_ != nullptr)
	{
		settings_.setBool("run_minimized", false);
		main_window_->show();

		auto power = FxModel::getModel().getPowerState();
		main_window_->setIcon(power, audio_process_on_);

		if (survey_tip_)
		{
			uint32_t survey_timer = settings_.getInt("survey_timer");
			if (survey_timer == 0)
			{
				survey_timer = std::time(nullptr) + (7 * (24 * 60 * 60));
				settings_.setInt("survey_timer", survey_timer);
			}
			else
			{
				uint32_t current_time = std::time(nullptr);
				if (current_time > survey_timer)
				{
					survey_tip_ = false;
					settings_.setBool("survey_displayed", true);
					String message = TRANS("Thanks for using FxSound! Would you be\r\ninterested in helping us by taking a quick 4 minute\r\nsurvey so we can make FxSound better?");
					if (authenticated_)
					{
						FxModel::getModel().pushMessage(message, { TRANS("Take the survey."), "https://forms.gle/ATx1ayXDWRaMdiR59" });
					}
				}
			}
		}
	}
}

bool FxController::isMainWindowVisible()
{
	if (main_window_ != nullptr)
	{
		return main_window_->isOnDesktop() && main_window_->isVisible();
	}

	return false;
}

void FxController::setMenuClicked(bool clicked)
{
	settings_.setBool("menu_clicked", clicked);
	FxModel::getModel().setMenuClicked(clicked);
}

FxWindow* FxController::getMainWindow()
{
	return main_window_;
}

Point<int> FxController::getSystemTrayWindowPosition(int width, int height)
{
	return system_tray_view_->getSystemTrayWindowPosition(width, height);
}

void FxController::autoSaveModifiedPreset()
{
	auto& model = FxModel::getModel();
	if (model.isPresetModified())
		autoSavePreset(model.getSelectedPreset());
}

bool FxController::exit()
{
    audioRepair_.reset();
	autoSaveModifiedPreset();

	JUCEApplication::getInstance()->systemRequestedQuit();

	return true;
}

void FxController::setPowerState(bool power_state)
{
    restoreSmartBaseline();
	if (!dfx_enabled_ || SysInfo::isRemoteSession())
	{
		main_window_->enablePowerButton(false);

		FxModel::getModel().setPowerState(false);
		powerOn(false);

		system_tray_view_->setStatus(false, false);
		main_window_->setIcon(false, false);
		
		return;
    }
	
	main_window_->enablePowerButton(true);

	FxModel::getModel().setPowerState(power_state);	
	powerOn(power_state);
	settings_.setBool("power", power_state);

	system_tray_view_->setStatus(power_state, audio_process_on_);
	main_window_->setIcon(power_state, audio_process_on_);
}

bool FxController::setPreset(const String& preset_name, bool notify)
{
	auto& model = FxModel::getModel();

	auto preset_count = model.getPresetCount();
	for (auto i = 0; i < preset_count; i++)
	{
		if (model.getPreset(i).name == preset_name)
		{
			return setPreset(i, notify);
		}
	}

	return false;
}

bool FxController::setPreset(int selected_index, bool notify)
{
	auto& model = FxModel::getModel();

	if (selected_index < 0 || selected_index >= model.getPresetCount())
	{
		return false;
	}

	auto preset = model.getPreset(selected_index);

	auto current_preset = model.getSelectedPreset();
	// Auto-save the current preset if it was modified before switching away
	if (selected_index != current_preset)
	{
		if (model.isPresetModified(current_preset))
			autoSavePreset(current_preset);
	}

	if (preset.path.isNotEmpty())
	{
        restoreSmartBaseline();
		// If this preset has an auto-saved version, load it instead of the original
		auto auto_save_path = getAutoSavePresetPath(preset.name);
		if (File(auto_save_path).existsAsFile())
		{
			dfx_dsp_.loadPreset(auto_save_path.toWideCharPointer());
			model.setPresetModified(selected_index, true);
		}
		else
		{
			dfx_dsp_.loadPreset(preset.path.toWideCharPointer());
			model.setPresetModified(selected_index, false);
		}

		settings_.setString("preset", preset.name);
		model.selectPreset(selected_index, true);

		for (auto e = 0; e < FxEffects::EffectType::NumEffects; e++)
		{
			auto value = dfx_dsp_.getEffectValue(static_cast<DfxDsp::Effect>(e));
			dfx_dsp_.setEffectValue(static_cast<DfxDsp::Effect>(e), value * 10);
		}

		auto num_bands = getNumEqBands();
		for (auto b = 0; b < num_bands; b++)
		{
			dfx_dsp_.setEqBandFrequency(b, dfx_dsp_.getEqBandFrequency(b));
			dfx_dsp_.setEqBandBoostCut(b, dfx_dsp_.getEqBandBoostCut(b));
		}
	}

    if (smart_enabled_) captureSmartBaseline();

	if (notify && FxModel::getModel().getPowerState())
	{
		model.pushMessage(TRANS("Preset: ") + model.getPreset(selected_index).name);
	}

	return true;
}

void FxController::setOutput(const String output_device_id, bool notify)
{
    restoreSmartBaseline();
	std::vector<SoundDevice> sound_devices = audio_passthru_->getSoundDevices();

	bool output_found = false;

	for (auto sound_device : sound_devices)
	{
		if (sound_device.isRealDevice)
		{
			if (output_device_id == sound_device.pwszID.c_str())
			{
				output_found = true;
				String message = TRANS("Output: ") + sound_device.deviceFriendlyName.c_str();

				if (getOutputName() != sound_device.deviceFriendlyName.c_str())
				{
					// Auto-select preset for the output device
					auto device_config = DeviceConfig::getDeviceConfig(settings_, sound_device.deviceFriendlyName.c_str());
					if (device_config.preset.isNotEmpty())
					{
						setPreset(device_config.preset, false);

						if (FxModel::getModel().getPowerState())
						{
							message += "\n" + TRANS("Preset: ") + device_config.preset;
						}
					}
				}
				FxModel::getModel().setSelectedOutput(sound_device, notify);
				setOutputName(sound_device.deviceFriendlyName.c_str());

				if (!isTimerRunning())
				{
					// FxSound is off and the selected output device is the default playback device
					if (sound_device.isDefaultDevice)
					{
						break;
					}
				}
				else
				{
					if (sound_device.isTargetedRealPlaybackDevice)
					{
						break;
					}
				}

				audio_passthru_->setAsPlaybackDevice(sound_device);
				output_changed_ = true;

				FxModel::getModel().pushMessage(message);

				break;
			}
		}
	}

	if (!output_found)
	{
		audio_passthru_->mute(true);
		powerOn(false);

		FxModel::getModel().pushMessage(TRANS("Output Disconnected"));
	}
	else
	{
		if (FxModel::getModel().getPowerState())
		{
			powerOn(true);
			audio_passthru_->mute(false);
		}
	}

	system_tray_view_->setStatus(FxModel::getModel().getPowerState(), isAudioProcessing());
}

void FxController::setOutput(int output, bool notify)
{
	auto output_devices = FxModel::getModel().getOutputDevices();

	if (output >= 0 && output < output_devices.size())
	{
		setOutput(output_devices[output].pwszID.c_str(), notify);
	}
}

bool FxController::isPlaybackDeviceAvailable()
{
	return playback_device_available_;
}

void FxController::checkDeviceChanges()
{
	audio_passthru_->checkDeviceChanges();
}

void FxController::savePreset(const String& preset_name)
{
	ScopedLock lock(save_lock_);

	auto& model = FxModel::getModel();

	auto preset_index = model.getSelectedPreset();
	auto preset = model.getPreset(preset_index);

	if (preset_name.isEmpty())
	{
		auto path = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"Cyn\\Presets";
		saveManualPreset(preset.name.toWideCharPointer(), path.toWideCharPointer());

		deleteAutoSavedPreset(preset.name);
		model.setPresetModified(preset_index, false);

		model.pushMessage(FormatString(TRANS(L"Changes to preset %s are saved."), preset.name));
	}
	else
	{
		auto path = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"Cyn\\Presets";
		saveManualPreset(preset_name.toWideCharPointer(), path.toWideCharPointer());

		deleteAutoSavedPreset(preset.name);

		initPresets();

		setPreset(preset_name);

		model.pushMessage(FormatString(TRANS("New preset %s is saved."), preset_name));

		if (model.getUserPresetCount() == FxController::getInstance().getMaxUserPresets())
		{
			Thread::sleep(2000);
			model.pushMessage(TRANS("Reached the limit on new presets."));
		}
	}
}

void FxController::renamePreset(const String& new_name)
{
	ScopedLock lock(save_lock_);

	auto& model = FxModel::getModel();

	auto preset_index = model.getSelectedPreset();
	auto preset = model.getPreset(preset_index);

	if (preset.name == new_name) return;
	if (preset.type == FxModel::PresetType::UserPreset)
	{
		auto path = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"Cyn\\Presets";
		saveManualPreset(new_name.toWideCharPointer(), path.toWideCharPointer());

		wchar_t old_path[MAX_PATH] = {};
		wcscpy_s(old_path, preset.path.toWideCharPointer());
		if (preset.path.length() + 1 < MAX_PATH)
		{
			old_path[preset.path.length() + 1] = L'\0';
		}
		SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, old_path, L"",
								   FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, FALSE, 0, L"" };
		SHFileOperation(&file_op);

		initPresets();

		setPreset(new_name);

		deleteAutoSavedPreset(preset.name);
		model.setPresetModified(FxModel::getModel().getSelectedPreset(), false);
	}
}

void FxController::deletePreset()
{
	auto& model = FxModel::getModel();

	auto preset_index = model.getSelectedPreset();
	auto preset = model.getPreset(preset_index);

	if (preset.type == FxModel::PresetType::UserPreset)
	{
		deleteAutoSavedPreset(preset.name);

		wchar_t path[MAX_PATH] = {};

		wcscpy_s(path, preset.path.toWideCharPointer());
		if (preset.path.length() + 1 < MAX_PATH)
		{
			path[preset.path.length() + 1] = L'\0';
		}

		SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, path, L"",
								   FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, FALSE, 0, L"" };
		SHFileOperation(&file_op);

		initPresets();

		auto device_config = DeviceConfig::getDeviceConfig(settings_, getOutputName());
		if (device_config.preset.isNotEmpty())
		{
			setPreset(device_config.preset, false);
		}
		else
		{
			setPreset(0);
		}

		FxModel::getModel().pushMessage(FormatString(TRANS("Preset %s is deleted."), preset.name));
	}
}

void FxController::undoPreset()
{
	auto& model = FxModel::getModel();

	auto preset_index = model.getSelectedPreset();
	if (!model.isPresetModified(preset_index))
		return;

	model.setPresetModified(preset_index, false);

	// Delete auto-save and clear modified flag before calling setPreset so it loads from original path
	auto preset = model.getPreset(preset_index);
	deleteAutoSavedPreset(preset.name);

	setPreset(preset_index);
}

void FxController::resetPresets()
{
	auto& model = FxModel::getModel();

	setNumEqBands(DEFAULT_NUM_EQ_BANDS);
	setVolumeLeveling(DEFAULT_VOLUME_LEVELING);
	setBalance(DEFAULT_BALANCE);
	setFilterQ(DEFAULT_FILTER_Q);
	setMasterGain(DEFAULT_MASTER_GAIN);

	auto count = model.getPresetCount();
	for (auto i = 0; i < count; i++)
	{
		auto preset = model.getPreset(i);
		if (preset.modified)
		{
			deleteAutoSavedPreset(preset.name);
		}

		if (preset.type == FxModel::PresetType::UserPreset)
		{
			wchar_t path[MAX_PATH] = {};

			wcscpy_s(path, preset.path.toWideCharPointer());
			if (preset.path.length() + 1 < MAX_PATH)
			{
				path[preset.path.length() + 1] = L'\0';
			}

			SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, path, L"",
									   FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, FALSE, 0, L"" };
			SHFileOperation(&file_op);
		}
	}

	initPresets();

	auto device_config = DeviceConfig::getDeviceConfig(settings_, getOutputName());
	if (device_config.preset.isNotEmpty())
	{
		setPreset(device_config.preset, false);
	}
	else
	{
		setPreset(0);
	}

	FxModel::getModel().pushMessage(TRANS("Presets are restored to factory defaults"));
}

bool FxController::exportPresets(const Array< FxModel::Preset>& presets)
{
	auto path_name = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getFullPathName()) + L"Cyn\\Presets\\Export\\";

	File path(path_name);
	if (!path.exists())
	{
		path.createDirectory();
	}

	bool exported = false;

	for (auto preset : presets)
	{
		bool skip = false;

		auto preset_file = File(path_name + preset.name + ".fac");
		if (preset_file.exists())
		{
			if (!FxConfirmationMessage::showMessage(FormatString(TRANS("Preset file %s already exists in the export path, do you want to overwrite the preset file?"), preset.name)))
			{
				skip = true;
			}
		}

		if (!skip)
		{
			dfx_dsp_.exportPreset(preset.path.toWideCharPointer(), preset.name.toWideCharPointer(), path.getFullPathName().toWideCharPointer());
			exported = true;
		}
	}

	return exported;
}

bool FxController::importPresets(const Array<File>& preset_files, StringArray& imported_presets, StringArray& skipped_presets)
{
	auto path_name = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"Cyn\\Presets";
	File path(path_name);
	if (!path.exists())
	{
		path.createDirectory();
	}

	auto& model = FxModel::getModel();

	for (auto preset_file : preset_files)
	{
		auto preset_info = dfx_dsp_.getPresetInfo(preset_file.getFullPathName().toWideCharPointer());
		if (model.isPresetNameValid(preset_info.name.c_str()))
		{
			File import_file(path_name + "\\" + preset_info.name.c_str() + ".fac");
			if (preset_file.copyFileTo(import_file))
			{
				imported_presets.add(preset_info.name.c_str());
			}
		}
		else
		{
			skipped_presets.add(preset_info.name.c_str());
		}
	}

	if (!imported_presets.isEmpty())
	{
		initPresets();

		auto preset_name = settings_.getString("preset");
		setPreset(preset_name);

		return true;
	}

	return false;
}

void FxController::initOutputs(std::vector<SoundDevice>& sound_devices)
{
	dfx_enabled_ = false;
	active_output_devices_.clear();

	SoundDevice default_output;

	auto device_configs_version = settings_.getInt("device_configs_version");
	if (device_configs_version != 2)
	{
		DeviceConfig::initDeviceConfigs(settings_, sound_devices);
		settings_.setInt("device_configs_version", 2);
	}
	else
	{
		auto device_configs = DeviceConfig::loadDeviceConfigs(settings_, "device_configs");
		if (device_configs.size() == 0)
		{
			// First time running the app, initialize device configs with sound devices
			DeviceConfig::initDeviceConfigs(settings_, sound_devices);
			settings_.setInt("device_configs_version", 2);
		}
		else
		{
			// Update device configs when relaunching the app
			DeviceConfig::updateDeviceConfigs(settings_, sound_devices);
		}
	}


	for (auto sound_device : sound_devices)
	{
		if (sound_device.isRealDevice)
		{
			if (sound_device.isActive && sound_device.deviceNumChannel >= 2)
			{
				active_output_devices_.push_back(sound_device);

				if (sound_device.isDefaultDevice || sound_device.isTargetedRealPlaybackDevice)
				{
					default_output = sound_device;
				}
			}
		}
		else if (sound_device.deviceFriendlyName.find(L"FxSound Audio Enhancer") != std::wstring::npos)
		{
			dfx_enabled_ = true;
		}
	}

	sortByDeviceConfigPriority(active_output_devices_);

	// If output is not set previously, use the system default output.
	if (getOutputName().isEmpty() && default_output.deviceFriendlyName.size() > 0)
	{
		setOutputName(default_output.deviceFriendlyName.c_str());
	}
	else if (active_output_devices_.size() > 0)
	{
		bool found = false;
		for (auto& output_device : active_output_devices_)
		{
			if (getOutputName() == output_device.deviceFriendlyName.c_str())
			{
				default_output = output_device;
				found = true;
				break;
			}
		}
		if (!found)
		{
			default_output = getPreferredOutput();
			setOutputName(default_output.deviceFriendlyName.c_str());
		}
	}

	FxModel::getModel().initOutputs(active_output_devices_);
	setOutput(default_output.pwszID.c_str());
}

void FxController::updateOutputs(std::vector<SoundDevice>& sound_devices)
{
	auto prev_active_devices = active_output_devices_;
    const auto selected_id = FxModel::getModel().getSelectedOutput().pwszID;

	active_output_devices_.clear();

	for (auto sound_device : sound_devices)
	{
		if (sound_device.isActive && sound_device.isRealDevice && sound_device.deviceNumChannel >= 2)
		{
			active_output_devices_.push_back(sound_device);
		}
	}

	sortByDeviceConfigPriority(active_output_devices_);

	SoundDevice preferred_device;

	// New device is connected, find the preferred device if the new device has higher priority than the current output device
	if (!isOutputPinned())
	{
		auto device_configs = DeviceConfig::loadDeviceConfigs(settings_, "device_configs");

		for (auto& sound_device : sound_devices)
		{
			if (!sound_device.isActive || !sound_device.isRealDevice || sound_device.deviceNumChannel < 2)
				continue;

			// Check if the device is a newly connected device
			auto it = std::find_if(prev_active_devices.begin(), prev_active_devices.end(),
				[&](const SoundDevice& existing) {
					return existing.pwszID == sound_device.pwszID;
				});

			if (it == prev_active_devices.end())
			{
				// If the new device has higher priority, select it as output
				if (compareOutputDevicePriority(sound_device.deviceFriendlyName.c_str(), getOutputName(), device_configs) < 0)
				{
					preferred_device = sound_device;
					break;
				}
			}
		}
	}

	// Output device is removed or a higher priority device is not connected.
	// If the current output device is removed, select the preferred output device.
	// Otherwise, keep the current output device.
	if (preferred_device.pwszID.empty() && active_output_devices_.size() > 0)
	{
		bool found = false;
		for (auto& output_device : active_output_devices_)
		{
			if (output_device.pwszID == selected_id)
			{
				preferred_device = output_device;
				found = true;
				break;
			}
		}
		if (!found)
		{
			preferred_device = getPreferredOutput();
		}
	}

	// Initialize the model to update the output devices in the UI.
	FxModel::getModel().initOutputs(active_output_devices_);
	// Set the output to select the new output and update the UI.
	// If the output has not changed, still output has to be set to update the UI.
	setOutput(preferred_device.pwszID.c_str());
}

// Handled when FxSound processing is on
void FxController::selectProcessingOutput(std::vector<SoundDevice>& sound_devices)
{
	auto available = audio_passthru_->isPlaybackDeviceAvailable();
	if (available != playback_device_available_)
	{
		playback_device_available_ = available;
		FxModel::getModel().notifyOutputError();
	}

	// New device is added or removed, so update the output device list and select a preferred output device based on the updated device list.
    // Compare endpoint identity, not just count: a USB/Bluetooth endpoint may
    // be replaced while the number of enumerated devices stays the same.
    std::vector<std::wstring> old_ids, new_ids;
    for (const auto& device : active_output_devices_) old_ids.push_back(device.pwszID);
    for (const auto& device : sound_devices)
        if (device.isActive && device.isRealDevice && device.deviceNumChannel >= 2)
            new_ids.push_back(device.pwszID);
    std::sort(old_ids.begin(), old_ids.end());
    std::sort(new_ids.begin(), new_ids.end());
	if (old_ids != new_ids || sound_devices.size() != device_count_)
	{
		updateOutputs(sound_devices);
		device_count_ = (uint32_t)sound_devices.size();

		if (!dfx_enabled_)
		{
			stopTimer();
			if (!SysInfo::isRemoteSession())
			{
				main_window_->removeFromDesktop();
				FxDeviceErrorMessage error_message;
				error_message.runModalLoop();
				JUCEApplication::getInstance()->systemRequestedQuit();
				return;
			}
		}
	}
	else
	{
        if (isOutputPinned())
        {
            const auto selected = FxModel::getModel().getSelectedOutput();
            for (const auto& device : sound_devices)
                if (device.pwszID == selected.pwszID && device.isActive && device.isRealDevice
                    && device.deviceNumChannel >= 2 && !device.isTargetedRealPlaybackDevice)
                {
                    setOutput(String(device.pwszID.c_str()), false);
                    return;
                }
        }
		for (auto sound_device : sound_devices)
		{
			// If the selected output device is different from the output device selected in application UI, update the application UI.
			if (sound_device.isRealDevice && sound_device.isTargetedRealPlaybackDevice && getOutputName() != sound_device.deviceFriendlyName.c_str())
			{
				setOutputName(sound_device.deviceFriendlyName.c_str());
				FxModel::getModel().setSelectedOutput(sound_device);

				auto device_config = DeviceConfig::getDeviceConfig(settings_, getOutputName());
				if (device_config.preset.isNotEmpty())
				{
					setPreset(device_config.preset, false);
				}

				break;
			}
		}
	}
}

// Handled when FxSound processing is off
void FxController::syncOutputWithSystemDefault(std::vector<SoundDevice>& sound_devices)
{
	active_output_devices_.clear();

	for (auto sound_device : sound_devices)
	{
		if (sound_device.isRealDevice && sound_device.deviceNumChannel >= 2)
		{
			active_output_devices_.push_back(sound_device);
		}
	}

	sortByDeviceConfigPriority(active_output_devices_);

	// Update the configuration as per the change in active devices and update the output device list in UI
	if (sound_devices.size() != device_count_)
	{
		FxModel::getModel().initOutputs(active_output_devices_);

		device_count_ = (uint32_t)sound_devices.size();
	}

	bool default_device_found = false;
	for (auto& output_device : active_output_devices_)
	{
		// Change the selected device in UI
		if (output_device.isDefaultDevice)
		{
			setOutputName(output_device.deviceFriendlyName.c_str());
			FxModel::getModel().setSelectedOutput(output_device);

			auto device_config = DeviceConfig::getDeviceConfig(settings_, getOutputName());
			if (device_config.preset.isNotEmpty())
			{
				setPreset(device_config.preset, false);
			}

			default_device_found = true;
			break;
		}
	}

	if (!default_device_found)
	{
		setOutput(getPreferredOutput().pwszID.c_str());
	}
}

void FxController::sortByDeviceConfigPriority(std::vector<SoundDevice>& devices)
{
	auto device_configs = DeviceConfig::loadDeviceConfigs(settings_, "device_configs");

	std::stable_sort(devices.begin(), devices.end(),
		[&](const SoundDevice& a, const SoundDevice& b) {
			return compareOutputDevicePriority(
				String(a.deviceFriendlyName.c_str()),
				String(b.deviceFriendlyName.c_str()),
				device_configs) < 0;
		});
}

void FxController::powerOn(bool on)
{
	if (on)
	{
		dfx_dsp_.powerOn(true);

		if (!isTimerRunning())
		{
			startTimer(100);
		}
	}
	else
	{
		dfx_dsp_.powerOn(false);

		if (isTimerRunning())
		{
			stopTimer();
		}

		audio_passthru_->restoreDefaultPlaybackDevice();
	}
}

float FxController::getAppliedEffectValue(FxEffects::EffectType effect)
{
    return dfx_dsp_.getEffectValue(static_cast<DfxDsp::Effect>(effect));
}

float FxController::getEffectValue(FxEffects::EffectType effect)
{
    if (smart_enabled_ && (int)effect >= 0 && (int)effect < 5) return smart_effects_[(int)effect] / 10.0f;
	return dfx_dsp_.getEffectValue(static_cast<DfxDsp::Effect>(effect));
}

void FxController::setEffectValue(FxEffects::EffectType effect, float value)
{
	if (!std::isfinite(value) || value < 0 || value > 10)
	{
		return;
    }

    if (smart_enabled_)
    {
        restoreSmartBaseline();
        smart_effects_[(int)effect] = value;
        if (effect == FxEffects::Bass) smart_bass_ = value;
        if (effect == FxEffects::Fidelity) smart_clarity_ = value;
    }
	dfx_dsp_.setEffectValue(static_cast<DfxDsp::Effect>(effect), value);

	auto& model = FxModel::getModel();
	if (!model.isPresetModified())
	{
		model.setPresetModified(model.getSelectedPreset(), true);
	}
	preset_dirty_ = true;
}

int FxController::getNumEqBands()
{
	return dfx_dsp_.getNumEqBands();
}

void FxController::setNumEqBands(int num_bands)
{
    if (num_bands != 31 && num_bands != 35 && num_bands != 39) return;
    if (dfx_dsp_.getNumEqBands() == num_bands) {
        settings_.setInt("num_bands", num_bands);
        return;
    }
    restoreSmartBaseline();
    dfx_dsp_.setNumBands(num_bands);
    if (smart_enabled_) captureSmartBaseline();
    settings_.setInt("num_bands", dfx_dsp_.getNumEqBands());
}

float FxController::getVolumeLeveling()
{
	return smart_enabled_ ? smart_leveling_ : dfx_dsp_.getVolumeLeveling();
}

void FxController::setVolumeLeveling(float gain_db)
{
    if (!std::isfinite(gain_db)) return;
	gain_db = std::round(gain_db * 2.0f) / 2.0f;
    if (smart_enabled_) smart_leveling_ = gain_db;
	dfx_dsp_.setVolumeLeveling(gain_db);
	settings_.setDouble("volume_leveling", gain_db);
}

float FxController::getBalance()
{
	return dfx_dsp_.getBalance();
}

void FxController::setBalance(float balance_db)
{
	balance_db = std::round(balance_db);
	dfx_dsp_.setBalance(balance_db);
	settings_.setDouble("balance", balance_db);
}

float FxController::getMasterGain()
{
	return dfx_dsp_.getMasterGain();
}

void FxController::setMasterGain(float gain_db)
{
	gain_db = std::round(gain_db);
	dfx_dsp_.setMasterGain(gain_db);
	settings_.setDouble("master_gain", gain_db);
}

float FxController::getFilterQ()
{
	return dfx_dsp_.getFilterQ();
}

void FxController::setFilterQ(float q_multiplier)
{
	q_multiplier = std::round(q_multiplier * 2.0f) / 2.0f;
	dfx_dsp_.setFilterQ(q_multiplier);
	settings_.setDouble("filter_q", q_multiplier);
}

bool FxController::isAudioProcessing()
{
	return audio_process_on_;
}

float FxController::getEqBandFrequency(int band_num)
{
	if (band_num < getNumEqBands())
	{
		return dfx_dsp_.getEqBandFrequency(band_num);
	}
	else
	{
		return 0;
	}
}

void FxController::setEqBandFrequency(int band_num, float freq)
{
	if (band_num < getNumEqBands())
	{
        float min_freq, max_freq;
        getEqBandFrequencyRange(band_num, &min_freq, &max_freq);
		if (freq < min_freq || freq > max_freq)
		{
			return;
        }

        restoreSmartBaseline();
		dfx_dsp_.setEqBandFrequency(band_num, freq);
        if (smart_enabled_) captureSmartBaseline();

		auto& model = FxModel::getModel();
		if (!model.isPresetModified())
		{
			model.setPresetModified(model.getSelectedPreset(), true);
		}
		preset_dirty_ = true;
	}
}

void FxController::getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq)
{
	if (band_num < getNumEqBands())
	{
		dfx_dsp_.getEqBandFrequencyRange(band_num, min_freq, max_freq);
	}
	else
	{
		*min_freq = 0;
		*max_freq = 0;
	}
}

float FxController::getAppliedEqBandBoostCut(int band_num)
{
    if (band_num < 0 || band_num >= getNumEqBands()) return 0.0f;
    return dfx_dsp_.getEqBandBoostCut(band_num);
}

float FxController::getEqBandBoostCut(int band_num)
{
    if (smart_enabled_ && band_num >= 0 && band_num < (int)smart_eq_.size()) return smart_eq_[band_num];
	if (band_num < getNumEqBands())
	{
		return dfx_dsp_.getEqBandBoostCut(band_num);
	}
	else
	{
		return 0;
	}
}

void FxController::setEqBandBoostCut(int band_num, float boost)
{
	if (band_num < getNumEqBands())
	{
		if (boost < MIN_GAIN || boost > MAX_GAIN)
		{
			return;
        }

        restoreSmartBaseline();
		dfx_dsp_.setEqBandBoostCut(band_num, boost);
        if (smart_enabled_) captureSmartBaseline();

		auto& model = FxModel::getModel();
		if (!model.isPresetModified())
		{
			model.setPresetModified(model.getSelectedPreset(), true);
		}
		preset_dirty_ = true;
	}
}

LRESULT CALLBACK FxController::eventCallback(HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
	static auto os = SystemStats::getOperatingSystemType();
	FxController* controller = (FxController*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_HOTKEY:
	{
		if (w_param == CMD_ON_OFF)
		{
			if (!SysInfo::isRemoteSession())
			{
				auto power_state = !FxModel::getModel().getPowerState();
				controller->setPowerState(power_state);

				String param = FxModel::getModel().getPowerState() ? TRANS(L"on") : TRANS(L"off");
				FxModel::getModel().pushMessage(controller->FormatString(TRANS("Cyn is %s."), param));
			}
		}
		if (w_param == CMD_OPEN_CLOSE)
		{
			if (controller->main_window_->isOnDesktop())
			{
				controller->hideMainWindow();
			}
			else
			{
				controller->showMainWindow();
			}
		}
		if (w_param == CMD_NEXT_PRESET && FxModel::getModel().getPowerState())
		{
			auto preset_index = FxModel::getModel().getSelectedPreset();
			auto preset_count = FxModel::getModel().getPresetCount();
			if (preset_count > 1)
			{
				if (preset_index < preset_count - 1)
				{
					preset_index++;
				}
				else
				{
					preset_index = 0;
				}

				controller->setPreset(preset_index);
			}
		}
		if (w_param == CMD_PREVIOUS_PRESET && FxModel::getModel().getPowerState())
		{
			auto preset_index = FxModel::getModel().getSelectedPreset();
			auto preset_count = FxModel::getModel().getPresetCount();
			if (preset_count > 1)
			{
				if (preset_index != 0)
				{
					preset_index--;
				}
				else
				{
					preset_index = preset_count - 1;
				}

				controller->setPreset(preset_index);
			}
		}
		if (w_param == CMD_NEXT_OUTPUT)
		{
			auto output_index = 0;
			for (auto& output_device : controller->active_output_devices_)
			{
				if (FxModel::getModel().getSelectedOutput().pwszID == output_device.pwszID)
				{
					break;
				}
				output_index++;
			}
			int count = 0;
			while (count < controller->active_output_devices_.size())
			{
				count++;
				if (output_index < controller->active_output_devices_.size() - 1)
				{
					output_index++;
				}
				else
				{
					output_index = 0;
				}

				if (controller->active_output_devices_[output_index].deviceNumChannel >= 2)
				{
					controller->setOutput(output_index);
					break;
				}
			}
		}
	}
	break;

	case WM_POWERBROADCAST:
	{
		if (w_param == PBT_APMSUSPEND)
		{
			controller->onSystemSuspend();
		}
		else if (w_param == PBT_APMRESUMESUSPEND || w_param == PBT_APMRESUMEAUTOMATIC)
		{
			controller->onSystemResume();
		}
	}
	break;

	case WM_WTSSESSION_CHANGE:
	{
		WPARAM login_event = (os == SystemStats::OperatingSystemType::Windows7) ? WTS_CONSOLE_CONNECT : WTS_SESSION_DESKTOP_READY;

		if (w_param == login_event || w_param == WTS_SESSION_UNLOCK)
		{
			controller->setPowerState(controller->settings_.getBool("power"));
		}
		else if (w_param == WTS_CONSOLE_DISCONNECT)
		{
			controller->powerOn(false);
		}
		else if (w_param == WTS_REMOTE_CONNECT)
		{
			controller->setPowerState(controller->settings_.getBool("power"));
		}
	}
	}

	return DefWindowProc(hwnd, message, w_param, l_param);
}

void FxController::timerCallback()
{
	if (output_changed_)
	{
		output_changed_ = false;
		return; // Let the next timer tick run without blocking the message thread.
	}

	audio_passthru_->processTimer();
	auto total_audio_process_time = dfx_dsp_.getTotalAudioProcessedTime();
	if (audio_process_time_ != total_audio_process_time)
	{
		audio_process_time_ = total_audio_process_time;
		audio_process_on_counter_++;
		audio_process_off_counter_ = 0;
	}
	else
	{
		audio_process_off_counter_++;
		audio_process_on_counter_ = 0;
	}

	auto power = FxModel::getModel().getPowerState();
	if (audio_process_on_counter_ == 5 && !audio_process_on_)
	{
		audio_process_on_ = true;
		system_tray_view_->setStatus(power, true);
		main_window_->setIcon(power, true);
		main_window_->startLogoAnimation();
		if (view_ == ViewType::Pro)
		{
			main_window_->showProView();
			main_window_->startVisualizer();
		}
	}
	if (audio_process_off_counter_ == 5 && audio_process_on_)
	{
		audio_process_on_ = false;
		system_tray_view_->setStatus(power, false);
		main_window_->setIcon(power, false);
		main_window_->stopLogoAnimation();
		if (view_ == ViewType::Pro)
		{
			main_window_->showProView();
			main_window_->pauseVisualizer();
		}
	}

    // The existing faster YAMNet model vetoes speech while AST provides the
    // broader musical context. Missing/stale voice analysis fails closed.
    if(dj_worker_ && dj_worker_->enabled()) {
        CynAudioScene voice;
        const bool fresh=yamnet_worker_ && yamnet_worker_->latest(voice);
        dj_worker_->liveScene(fresh,voice.music,voice.speech,voice.silence,audio_process_on_&&power);
    }
    updateSmartListening();

	// Auto-save modified preset once per minute if there are unsaved changes
	static constexpr int AUTO_SAVE_INTERVAL = 600; // 600 ticks * 100ms = 60 seconds
	if (++auto_save_counter_ >= AUTO_SAVE_INTERVAL)
	{
		if (preset_dirty_)
		{
			autoSavePreset(FxModel::getModel().getSelectedPreset());
		}
		auto_save_counter_ = 0;
	}

}

void FxController::onSoundDeviceChange(bool processing)
{
	if (session_id_ != WTSGetActiveConsoleSessionId())
		return; // Ignore device changes on another user session

	ScopedLock auto_lock(lock_);

	if (isTimerRunning())
	{
		if (processing)
		{
			output_devices_ = audio_passthru_->getSoundDevices(false);
			DeviceConfig::updateDeviceConfigs(settings_, output_devices_);
			auto sound_devices = audio_passthru_->getSoundDevices(true);
			selectProcessingOutput(sound_devices);
		}
	}
	else
	{
		output_devices_ = audio_passthru_->getSoundDevices(false);
		DeviceConfig::updateDeviceConfigs(settings_, output_devices_);
		auto sound_devices = audio_passthru_->getSoundDevices(true);
		syncOutputWithSystemDefault(sound_devices);
	}
}

void FxController::onSystemSuspend()
{
	if (FxModel::getModel().getPowerState())
	{
		audio_passthru_->mute(true);
	}
}

void FxController::onSystemResume()
{
	if (FxModel::getModel().getPowerState())
	{
		audio_passthru_->mute(false);
	}
}


void FxController::enableHotkeys(bool enable)
{
	settings_.setBool("hotkeys", enable);
	FxModel::getModel().setHotkeySupport(enable);
	if (enable)
	{
		registerHotkeys();
	}
	else
	{
		unregisterHotkeys();
	}
}

bool FxController::getHotkey(String cmdKey, int& mod, int& vk)
{
	int cmd = settings_.getInt(cmdKey);
	mod = (cmd >> 16) & 0x7;
	vk = cmd & 0xff;
	if ((mod == (MOD_CONTROL | MOD_ALT) || mod == (MOD_CONTROL | MOD_SHIFT)) && ((vk >= 0x30 && vk <= 0x39) || (vk >= 'A' && vk <= 'Z')))
	{
		return true;
	}

	mod = 0;
	vk = 0;

	return false;
}

bool FxController::setHotkey(const String& command, int new_mod, int new_vk)
{
	StringArray hotkey_cmds = { HK_CMD_ON_OFF, HK_CMD_OPEN_CLOSE, HK_CMD_NEXT_PRESET, HK_CMD_PREVIOUS_PRESET, HK_CMD_NEXT_OUTPUT };

	for (int i = 0; i < hotkey_cmds.size(); i++)
	{
		if (command == hotkey_cmds[i])
		{
			continue;
		}

		int mod, vk;
		if (getHotkey(hotkey_cmds[i], mod, vk))
		{
			if (mod == new_mod && vk == new_vk)
			{
				return false;
			}
		}
	}

	unsigned int code = (new_mod << 16) | new_vk;
	if (!isValidHotkey(new_mod, new_vk))
	{
		code = 0;
	}

	int hk_id = 0;

	if (command == HK_CMD_ON_OFF)
	{
		hk_id = CMD_ON_OFF;
	}

	if (command == HK_CMD_OPEN_CLOSE)
	{
		hk_id = CMD_OPEN_CLOSE;
	}

	if (command == HK_CMD_NEXT_PRESET)
	{
		hk_id = CMD_NEXT_PRESET;
	}

	if (command == HK_CMD_PREVIOUS_PRESET)
	{
		hk_id = CMD_PREVIOUS_PRESET;
	}

	if (command == HK_CMD_NEXT_OUTPUT)
	{
		hk_id = CMD_NEXT_OUTPUT;
	}

	if (hk_id != 0)
	{
		::UnregisterHotKey(message_window_.getHandle(), hk_id);

		if (code == 0)
		{
			settings_.setInt(command, code);
			return false;
		}

		if (::RegisterHotKey(message_window_.getHandle(), hk_id, new_mod, new_vk) != FALSE)
		{
			settings_.setInt(command, code);
			return true;
		}
		else
		{
			settings_.setInt(command, 0);
			return false;
		}
	}

	return false;
}

bool FxController::isValidHotkey(int mod, int vk)
{
	if ((mod & MOD_CONTROL) == 0)
	{
		return false;
	}

	HKL hkl = GetKeyboardLayout(0);
	BYTE kbd_state[256] = { 0 };
	kbd_state[VK_CONTROL] = 0x80;

	if ((mod & MOD_ALT) != 0)
	{
		kbd_state[VK_MENU] = 0x80;
	}
	if ((mod & MOD_SHIFT) != 0)
	{
		kbd_state[VK_SHIFT] = 0x80;
	}

	WCHAR output[3] = { 0 };

	int len = ToUnicodeEx(vk, 0, kbd_state, output, 3, 0, hkl);
	if (len > 0 && output[0] >= 0x20) // The hotkey combination generates a non-control character
	{
		return false;
	}

	return true;
}

bool FxController::isHelpTooltipsHidden()
{
	return hide_help_tooltips_;
}

void FxController::setHelpTooltipsHidden(bool status)
{
	hide_help_tooltips_ = status;
	settings_.setBool("hide_help_tooltips", status);
	main_window_->repaint();
}

bool FxController::isNotificationsHidden()
{
	return hide_notifications_;
}

void FxController::setNotificationsHidden(bool status)
{
	hide_notifications_ = status;
	settings_.setBool("hide_notifications", status);
}

String FxController::getLanguage() const
{
	return language_;
}

void FxController::setLanguage(String language_code)
{
	if (language_code.isEmpty())
	{
		language_code = "en";
	}

	language_ = language_code;
	settings_.setString("language", language_);

	LocalisedStrings::setCurrentMappings(nullptr);

	if (language_.startsWithIgnoreCase("ko"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ko_txt, BinaryData::FxSound_ko_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("vi"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_vi_txt, BinaryData::FxSound_vi_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("id"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_id_txt, BinaryData::FxSound_id_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("pt-br"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ptbr_txt, BinaryData::FxSound_ptbr_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("pt"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_pt_txt, BinaryData::FxSound_pt_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("es"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_es_txt, BinaryData::FxSound_es_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("zh-CN"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_zhCN_txt, BinaryData::FxSound_zhCN_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("zh-TW"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_zhTW_txt, BinaryData::FxSound_zhTW_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("fr"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_fr_txt, BinaryData::FxSound_fr_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("sv"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_sv_txt, BinaryData::FxSound_sv_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("it"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_it_txt, BinaryData::FxSound_it_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ru"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ru_txt, BinaryData::FxSound_ru_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ro"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ro_txt, BinaryData::FxSound_ro_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("tr"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_tr_txt, BinaryData::FxSound_tr_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("pl"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_pl_txt, BinaryData::FxSound_pl_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("de"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_de_txt, BinaryData::FxSound_de_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("hu"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::fxsound_hu_txt, BinaryData::fxsound_hu_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("th"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_th_txt, BinaryData::FxSound_th_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("nl"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_nl_txt, BinaryData::FxSound_nl_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ja"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ja_txt, BinaryData::FxSound_ja_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ar"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ar_txt, BinaryData::FxSound_ar_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("hr"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_hr_txt, BinaryData::FxSound_hr_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ba"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ba_txt, BinaryData::FxSound_ba_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("fa"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_fa_txt, BinaryData::FxSound_fa_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ua"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ua_txt, BinaryData::FxSound_ua_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("no"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_no_txt, BinaryData::FxSound_no_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("sl"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_sl_txt, BinaryData::FxSound_sl_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("fi"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_fi_txt, BinaryData::FxSound_fi_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("cs"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_cs_txt, BinaryData::FxSound_cs_txtSize), false));
	}

	auto* theme = dynamic_cast<FxTheme*>(&LookAndFeel::getDefaultLookAndFeel());
	if (theme != nullptr)
	{
		theme->loadFont(language_);
	}

	if (main_window_ != nullptr)
	{
		main_window_->sendLookAndFeelChange();
	}
}

String FxController::getLanguageName(String language_code) const
{
	if (language_code.startsWithIgnoreCase("en"))
	{
		return "English";
	}
	else if (language_code.startsWithIgnoreCase("ko"))
	{
		return L"\ud55c\uad6d\uc5b4";
	}
	else if (language_code.startsWithIgnoreCase("vi"))
	{
		return L"Ti\u1ebfng Vi\u1ec7t";
	}
	else if (language_code.startsWithIgnoreCase("id"))
	{
		return L"bahasa Indonesia";
	}
	else if (language_code.startsWithIgnoreCase("pt-br"))
	{
		return L"portugu\u00eas brasileiro";
	}
	else if (language_code.startsWithIgnoreCase("pt"))
	{
		return L"Portugu\u00eas";
	}
	else if (language_code.startsWithIgnoreCase("es"))
	{
		return L"Espa\u00f1ol";
	}
	else if (language_code.startsWithIgnoreCase("zh-CN"))
	{
		return L"\u7b80\u4f53\u4e2d\u6587";
	}
	else if (language_code.startsWithIgnoreCase("zh-TW"))
	{
		return L"\u7e41\u9ad4\u4e2d\u6587";
	}
	else if (language_code.startsWithIgnoreCase("sv"))
	{
		return L"svenska";
	}
	else if (language_code.startsWithIgnoreCase("fr"))
	{
		return L"fran\u00e7ais";
	}
	else if (language_code.startsWithIgnoreCase("it"))
	{
		return L"Italiano";
	}
	else if (language_code.startsWithIgnoreCase("ru"))
	{
		return L"\u0440\u0443\u0441\u0441\u043a\u0438\u0439";
	}
	else if (language_code.startsWithIgnoreCase("ro"))
	{
		return L"Rom\u00e2n\u0103";
	}
	else if (language_code.startsWithIgnoreCase("tr")) {
		return L"T\u00fcrk";
	}
	else if (language_code.startsWithIgnoreCase("pl"))
	{
		return L"Polski";
	}
	else if (language_code.startsWithIgnoreCase("de"))
	{
		return L"Deutsch";
	}
	else if (language_code.startsWithIgnoreCase("hu"))
	{
		return L"Magyar";
	}
	else if (language_code.startsWithIgnoreCase("th"))
	{
		return L"\u0e41\u0e1a\u0e1a\u0e44\u0e17\u0e22";
	}
	else if (language_code.startsWithIgnoreCase("nl"))
	{
		return L"Nederlands";
	}
	else if (language_code.startsWithIgnoreCase("ja"))
	{
		return L"\u65e5\u672c\u8a9e";
	}
	else if (language_code.startsWithIgnoreCase("ar"))
	{
		return L"\u0627\u0644\u0639\u0631\u0628\u064a\u0629";
	}
	else if (language_code.startsWithIgnoreCase("hr"))
	{
		return L"hrvatski";
	}
	else if (language_code.startsWithIgnoreCase("ba"))
	{
		return L"bosanski";
	}
	else if (language_code.startsWithIgnoreCase("fa"))
	{
		return L"\u0641\u0627\u0631\u0633\u06cc";
	}
	else if (language_code.startsWithIgnoreCase("ua"))
	{
		return L"\u0443\u043a\u0440\u0430\u0457\u043d\u0441\u044c\u043a\u0430";
	}
	else if (language_code.startsWithIgnoreCase("no"))
	{
		return L"Norsk";
	}
	else if (language_code.startsWithIgnoreCase("sl"))
	{
		return L"Sloven\u0161\u010dina";
	}
	else if (language_code.startsWithIgnoreCase("fi"))
	{
		return L"Suomi";
	}
	else if (language_code.startsWithIgnoreCase("cs"))
	{
		return L"\u010cesky";
    }

	return "English";
}

int FxController::getMaxUserPresets() const
{
	return max_user_presets_;
}

bool FxController::getAutoUpdates()
{
	return auto_updates_;
}

void FxController::setAutoUpdates(bool enable)
{
	auto_updates_ = enable;
	settings_.setBool("cyn_automatic_updates", enable);
    if (enable)
    {
        // A user who re-enables the feature should get a fresh check.
        settings_.setInt("cyn_last_automatic_update_attempt_v3", 0);
        checkAutomaticUpdates();
    }
}

void FxController::checkAutomaticUpdates()
{
    if (!auto_updates_)
        return;
    const auto now=static_cast<int>(std::time(nullptr));
    const auto last=settings_.getInt("cyn_last_automatic_update_attempt_v3",0);
    constexpr int checkIntervalSeconds=1800;
    if (last > 0 && now >= last && now-last < checkIntervalSeconds)
        return;
    // A failed process start must not suppress subsequent attempts.
    if (launchCynUpdater(false))
    {
        settings_.setInt("cyn_last_automatic_update_attempt_v3",now);
        logMessage("Cyn updater: automatic release check started.");
    }
    else
    {
        logMessage("Cyn updater: could not launch automatic check; will retry.");
    }
}

void FxController::checkUpdates()
{
    launchCynUpdater(true);
}

void FxController::saveWindowPosition(int x, int y)
{
	settings_.setInt("window_x", x);
	settings_.setInt("window_y", y);
}

void FxController::getWindowPosition(int& x, int& y)
{
	x = settings_.getInt("window_x", 0);
	y = settings_.getInt("window_y", 0);
}

juce::Array<DeviceConfig> FxController::getDeviceConfigs()
{
	return DeviceConfig::loadDeviceConfigs(settings_, "device_configs");
}

void FxController::saveDeviceConfigs(const juce::Array<DeviceConfig>& device_configs)
{
	DeviceConfig::saveDeviceConfigs(settings_, "device_configs", device_configs);
}

bool FxController::isOutputDeviceConnected(const String& output_device_name)
{
	for (auto& output_device : active_output_devices_)
	{
		if (output_device_name == output_device.deviceFriendlyName.c_str())
		{
			return true;
		}
	}

	return false;
}

bool FxController::isOutputDevicePresent(const String& output_device_name)
{
	for (auto& output_device : output_devices_)
	{
		if (output_device.isRealDevice && output_device_name == output_device.deviceFriendlyName.c_str())
		{
			return true;
		}
	}

	return false;
}

SoundDevice FxController::getPreferredOutput()
{
	auto device_configs = DeviceConfig::loadDeviceConfigs(settings_, "device_configs");

	for (auto& device_config : device_configs)
	{
		for (auto& device : active_output_devices_)
		{
			if (device_config.device_name == device.deviceFriendlyName.c_str())
			{
				return device;
			}
		}
	}

	if (active_output_devices_.size() > 0)
	{
		return active_output_devices_[0];
	}

	return {};
}

int FxController::compareOutputDevicePriority(const String& output_device_name1, const String& output_device_name2, const juce::Array<DeviceConfig>& device_configs)
{
	int priority1 = (int)device_configs.size();
	int priority2 = (int)device_configs.size();
	int index = 0;

	for (auto& device_config : device_configs)
	{
		if (device_config.device_name == output_device_name1)
			priority1 = index;
		if (device_config.device_name == output_device_name2)
			priority2 = index;

		if (priority1 != (int)device_configs.size() && priority2 != (int)device_configs.size())
			break;

		index++;
	}

	return priority1 - priority2;
}

void FxController::refreshOutputList()
{
	sortByDeviceConfigPriority(active_output_devices_);
	FxModel::getModel().initOutputs(active_output_devices_);
	FxModel::getModel().setSelectedOutput(FxModel::getModel().getSelectedOutput());
}

const String& FxController::getOutputName()
{
	if (output_device_name_.isEmpty())
	{
		output_device_name_ = settings_.getString("output_device_name");
	}

	return output_device_name_;
}

void FxController::setOutputName(const String& output_device_name)
{
	output_device_name_ = output_device_name;
	settings_.setString("output_device_name", output_device_name);
}

bool FxController::isNewOutputPrioritized()
{
	return settings_.getBool("prioritize_new_output", false);
}

void FxController::setNewOutputPrioritized(bool prioritize)
{
	settings_.setBool("prioritize_new_output", prioritize);
}

FxThemeMode FxController::getThemeMode()
{
	return FxTheme::getThemeMode();
}

void FxController::setThemeMode(FxThemeMode mode)
{
	if (mode == getThemeMode())
	{
		return;
	}
    if (main_window_)
        main_window_->beginThemeTransition();

	FxTheme::setThemeMode(mode);
	settings_.setInt("theme_mode", static_cast<int>(mode));
    if(!main_window_) return;
	setLanguage(getLanguage()); // To reload font for the new theme
	main_window_->sendLookAndFeelChange();

	auto power = FxModel::getModel().getPowerState();
	main_window_->setIcon(power, audio_process_on_);
	system_tray_view_->setStatus(power, audio_process_on_);
    main_window_->finishThemeTransition();
}

bool FxController::isAlwaysOnTop()
{
	return always_on_top_;
}

void FxController::setAlwaysOnTop(bool always_on_top)
{
	always_on_top_ = always_on_top;
	settings_.setBool("always_on_top", always_on_top);
	main_window_->setAlwaysOnTop(always_on_top);
}

bool FxController::isLaunchOnStartup()
{
	HKEY hkey;
	RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hkey);
	DWORD type;
	DWORD size = 0;
	RegQueryValueEx(hkey, L"Cyn", NULL, &type, NULL, &size);
	RegCloseKey(hkey);

	return size > 0;
}

void FxController::setLaunchOnStartup(bool launch_on_startup)
{
	wchar_t szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	HKEY hkey;
	RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkey);

	if (launch_on_startup)
	{
		const String command = "\"" + String(szPath) + "\"";
        RegSetValueEx(hkey, L"Cyn", 0, REG_SZ, (const BYTE*)command.toWideCharPointer(), (DWORD)((command.length()+1)*sizeof(wchar_t)));
	}
	else
	{
		RegDeleteValue(hkey, L"Cyn");
	}

	RegCloseKey(hkey);
}

void FxController::registerHotkeys()
{
	if (!hotkeys_registered_)
	{
		int mod;
		int vk;

		if (getHotkey(HK_CMD_ON_OFF, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_ON_OFF, mod, vk);
			}
		}

		if (getHotkey(HK_CMD_OPEN_CLOSE, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_OPEN_CLOSE, mod, vk);
			}

		}

		if (getHotkey(HK_CMD_NEXT_PRESET, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_NEXT_PRESET, mod, vk);
			}

		}

		if (getHotkey(HK_CMD_PREVIOUS_PRESET, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_PREVIOUS_PRESET, mod, vk);
			}

		}

		if (getHotkey(HK_CMD_NEXT_OUTPUT, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_NEXT_OUTPUT, mod, vk);
			}
		}

		hotkeys_registered_ = true;
	}
}

void FxController::unregisterHotkeys()
{
	if (hotkeys_registered_)
	{
		::UnregisterHotKey(message_window_.getHandle(), CMD_ON_OFF);
		::UnregisterHotKey(message_window_.getHandle(), CMD_OPEN_CLOSE);
		::UnregisterHotKey(message_window_.getHandle(), CMD_NEXT_PRESET);
		::UnregisterHotKey(message_window_.getHandle(), CMD_PREVIOUS_PRESET);
		::UnregisterHotKey(message_window_.getHandle(), CMD_NEXT_OUTPUT);
		hotkeys_registered_ = false;
	}
}

void FxController::getSpectrumBandValues(Array<float>& band_values)
{
	float values[NUM_SPECTRUM_BANDS] = { 0 };

	dfx_dsp_.getSpectrumBandValues(values, NUM_SPECTRUM_BANDS);

	band_values.clearQuick();
	for (auto i = 0; i < NUM_SPECTRUM_BANDS; i++)
	{
		if (audio_process_on_)
		{
			band_values.set(i, values[i]);
		}
		else
		{
			band_values.set(i, 0.01);
		}
	}
}

String FxController::FormatString(const String& format, const String& arg)
{
	wchar_t buffer[1024];

	swprintf_s(buffer, format.toWideCharPointer(), arg.toWideCharPointer());

	return String(buffer);
}


void FxController::captureSmartBaseline()
{
    music_crescendo_.reset();
    for(int e=0;e<5;++e) smart_effects_[e]=jlimit(0.0f,10.0f,dfx_dsp_.getEffectValue((DfxDsp::Effect)e)*10);
    smart_bass_=smart_effects_[4]; smart_clarity_=smart_effects_[0];
    smart_leveling_=dfx_dsp_.getVolumeLeveling();
    smart_eq_.clear();
    for(int b=0;b<getNumEqBands();++b) smart_eq_.push_back(dfx_dsp_.getEqBandBoostCut(b));
    smart_scores_.fill(0.5f);
    cycling_effects_=smart_effects_;
    cycling_eq_=smart_eq_;
    cycling_dirty_=true;
    cycling_policy_.reset();
}
void FxController::restoreSmartBaseline()
{
    music_crescendo_.reset();
    if(!smart_enabled_) return;
    for(int e=0;e<5;++e) dfx_dsp_.setEffectValue((DfxDsp::Effect)e,smart_effects_[e]);
    for(int b=0;b<jmin(getNumEqBands(),(int)smart_eq_.size());++b) dfx_dsp_.setEqBandBoostCut(b,smart_eq_[b]);
    dfx_dsp_.setVolumeLeveling(smart_leveling_); smart_scores_.fill(0.5f);
}
void FxController::saveManualPreset(const wchar_t* name,const wchar_t* directory)
{ restoreSmartBaseline(); dfx_dsp_.savePreset(name,directory); }
void FxController::setSmartListening(bool enabled)
{
    if(enabled!=smart_enabled_) { if(enabled) captureSmartBaseline(); else restoreSmartBaseline(); smart_enabled_=enabled; }
    settings_.setBool("ember_smart_listening",enabled); // Retain the old preference when upgrading.
}
void FxController::setSmartPresetCycling(bool enabled)
{
    if(smart_preset_cycling_==enabled) return;
    // Disabling must restore the user's actual selected preset baseline,
    // not the most recently cycled profile.
    if(smart_enabled_) restoreSmartBaseline();
    smart_preset_cycling_=enabled;
    settings_.setBool("cyn_auto_preset_cycling",enabled);
    cycling_profiles_.clear();cycling_signatures_.clear();cycling_policy_.reset();
    smart_cycling_preset_name_="Manual";
    cycling_dirty_=true;
    if(smart_enabled_) captureSmartBaseline();
}

void FxController::rebuildPresetCyclingProfiles()
{
    cycling_dirty_=false;
    cycling_profiles_.clear();cycling_signatures_.clear();
    cycling_policy_.reset();
    cycling_effects_=smart_effects_;cycling_eq_=smart_eq_;
    smart_cycling_preset_name_="Manual";
    if(!smart_preset_cycling_ || smart_mode_!=0 || !smart_enabled_) return;

    auto buildSignature=[&](const CyclingProfile& p) {
        CynPresetSignature signature;
        signature.bass=jlimit(0.0f,1.0f,p.effects[4]/10.0f);
        signature.impact=jlimit(0.0f,1.0f,p.effects[3]/10.0f);
        signature.clarity=jlimit(0.0f,1.0f,p.effects[0]/10.0f);
        signature.space=jlimit(0.0f,1.0f,(p.effects[1]+p.effects[2])/20.0f);
        float low=0,high=0;int lowCount=0,highCount=0;
        const int eqCount=jmin(getNumEqBands(),(int)p.eq.size());
        for(int b=0;b<eqCount;++b) {
            const float freq=dfx_dsp_.getEqBandFrequency(b);
            if(freq<=250){low+=p.eq[(size_t)b];++lowCount;}
            if(freq>=3500){high+=p.eq[(size_t)b];++highCount;}
        }
        if(lowCount) signature.bass=jlimit(0.0f,1.0f,
            signature.bass+low/(float)lowCount/24.0f);
        signature.air=jlimit(0.0f,1.0f,0.5f+
            (highCount?high/(float)highCount/20.0f:0.0f));
        return signature;
    };
    // First profile is always a snapshot of the user's selected manual preset.
    CyclingProfile manual;manual.name="Manual";
    manual.effects=smart_effects_;manual.eq=smart_eq_;
    manual.signature=buildSignature(manual);
    cycling_profiles_.push_back(manual);
    cycling_signatures_.push_back(manual.signature);

    // Read actual bundled app preset files ONCE, without loadPreset(),
    // saving/modifying presets, changing the UI selection, or touching
    // the live DSP's state. Ignore user presets unless already selected.
    auto& model=FxModel::getModel();
    for(int i=0;i<model.getPresetCount() && cycling_profiles_.size()<17;++i) {
        const auto preset=model.getPreset(i);
        if(preset.type!=FxModel::PresetType::AppPreset ||
           preset.path.isEmpty() || !File(preset.path).existsAsFile()) continue;
        DfxPresetSnapshot raw;
        if(!dfx_dsp_.readPresetSnapshot(std::wstring(preset.path.toWideCharPointer()),raw)) continue;
        CyclingProfile next;next.name=preset.name;
        next.effects=raw.effects;
        next.eq.reserve(smart_eq_.size());
        for(int b=0;b<jmin(getNumEqBands(),(int)smart_eq_.size());++b) {
            const float freq=dfx_dsp_.getEqBandFrequency(b);
            float gain=0.0f;
            if(!raw.eqFrequencies.empty() && raw.eqFrequencies.size()==raw.eqGains.size()){
                size_t k=0;
                while(k+1<raw.eqFrequencies.size() && raw.eqFrequencies[k+1]<freq)++k;
                gain=raw.eqGains[k];
                if(k+1<raw.eqFrequencies.size() && raw.eqFrequencies[k+1]>raw.eqFrequencies[k]){
                    const float l=std::log(jmax(1.0f,freq));
                    const float l1=std::log(jmax(1.0f,raw.eqFrequencies[k]));
                    const float l2=std::log(jmax(1.0f,raw.eqFrequencies[k+1]));
                    const float t=jlimit(0.0f,1.0f,(l-l1)/jmax(.00001f,l2-l1));
                    gain+=t*(raw.eqGains[k+1]-gain);
                }
            }
            next.eq.push_back(jlimit(MIN_GAIN,MAX_GAIN,gain));
        }
        next.signature=buildSignature(next);
        cycling_signatures_.push_back(next.signature);
        cycling_profiles_.push_back(std::move(next));
    }
}

void FxController::setSmartListeningMode(int mode)
{
    mode=jlimit(0,1,mode);
    if(smart_training_ || mode==smart_mode_) return;
    // Switching must never inherit temporary DSP settings from the old model.
    if(smart_enabled_) restoreSmartBaseline();
    smart_mode_=mode;
    if(smart_enabled_) captureSmartBaseline();
    settings_.setInt("cyn_ai_listening_mode",mode);
    smart_learning_message_=mode==1 ?
        "Movie AI is separate. Label dialogue, music and effects with More / Less." :
        "Music AI selected. Your music training remains independent.";
}

std::vector<FxController::AIProfileOption> FxController::getAvailableAIProfiles() const {
    std::vector<AIProfileOption> choices{{"personal","My saved AI"}};
    const auto append=[&](const File& dir,const String& prefix) {
        for(const auto& f:dir.findChildFiles(File::findFiles,false,"*.cynai.json")) {
            if(f.getSize()<=0 || f.getSize()>2*1024*1024) continue;
            const auto data=JSON::parse(f), model=data["model"];
            const auto* weights=model["weights"].getArray();
            const auto* ready=model["ready"].getArray();
            if(data["format"].toString()!="cyn-ai-profile" || (int)data["schema"]!=1 ||
               model["architecture"].toString()!="cyn-audio-11x12x5" ||
               !weights || weights->size()!=PersonalAudioModel::weightCount ||
               !ready || ready->size()!=PersonalAudioModel::outputs) continue;
            auto name=data["profile"]["name"].toString().trim();
            if(name.isEmpty()) name=f.getFileNameWithoutExtension();
            choices.push_back({prefix+f.getFileName(),name.substring(0,65)});
        }
    };
    append(File::getSpecialLocation(File::currentExecutableFile).getSiblingFile("CynAiProfiles"),"bundled:");
    append(File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Cyn/AIModels"),"imported:");
    if(smart_mode_==0 && apex6MInstalled())
        choices.push_back({"apex6m","CYN APEX 6M (Full ONNX · Live Neural EQ)"});
    return choices;
}
String FxController::getActiveAIProfileId() {
    const String id=settings_.getString(smart_mode_==1?"cyn_active_ai_profile_movie":"cyn_active_ai_profile_music");
    return id.isEmpty()?"personal":id;
}
bool FxController::selectAIProfile(const String& id,String& error) {
    if(smart_training_) { error="Wait for your model training to finish.";return false; }
    if(id==getActiveAIProfileId()) { error.clear();return true; }
    // The full APEX ONNX model is a distinct runtime, NOT a 209-weight .cynai.
    // Leave private learning and the previous model untouched.
    if(id=="apex6m") {
        if(smart_mode_!=0) {error="Full APEX is a Music AI preview only.";return false;}
        if(!apex6MInstalled() || !apex6MReady()) {
            error="Full APEX model is missing or still loading; try again shortly.";
            return false;
        }
        settings_.setString("cyn_apex_previous_music_profile",getActiveAIProfileId());
        if(smart_enabled_) restoreSmartBaseline();
        settings_.setString("cyn_active_ai_profile_music","apex6m");
        if(smart_enabled_) captureSmartBaseline();
        smart_learning_message_="APEX 6M neural Music AI selected: ONNX + adaptive live DSP.";
        error.clear();return true;
    }
    const auto choices=getAvailableAIProfiles();
    const auto found=std::find_if(choices.begin(),choices.end(),
        [&](const AIProfileOption& option){return option.id==id;});
    if(found==choices.end()) { error="That AI model is not installed.";return false; }
    if(smart_mode_==0 && getActiveAIProfileId()=="apex6m") {
        // Validate the new model before touching the currently active one.
        const String old=settings_.getString("cyn_apex_previous_music_profile");
        const String previous=(old.isEmpty() || old=="apex6m")?"personal":old;
        if(smart_enabled_) restoreSmartBaseline();
        settings_.setString("cyn_active_ai_profile_music",previous);
        if(id==previous) {
            if(smart_enabled_) captureSmartBaseline();
            smart_learning_message_="Restored previous Music AI and its trained preferences.";
            error.clear();return true;
        }
        if(smart_enabled_) captureSmartBaseline();
    }
    const auto rootDir=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Cyn");
    const auto prefs=rootDir.getChildFile("preferences.json"), slots=rootDir.getChildFile("AIModelSlots");
    if(slots.createDirectory().failed()) { error="Cannot create private AI backups.";return false; }
    const auto slotFor=[&](String name) {
        name=name.replaceCharacter(':','-');
        return slots.getChildFile(String(smart_mode_==1?"movie-":"music-")+name+".json");
    };
    // Preserve weights AND all per-mode More/Less labels for the model we leave.
    saveSmartModel();
    if(!prefs.existsAsFile()) { error="Could not save current neural preferences.";return false; }
    auto state=JSON::parse(prefs);
    auto* root=state.getDynamicObject();
    if(!root) { error="Current preferences are invalid.";return false; }
    const var prior=smart_mode_==1?state["movie_ai"]:state;
    if(!slotFor(getActiveAIProfileId()).replaceWithText(JSON::toString(prior))) {
        error="Could not back up the active model. Switch cancelled.";return false;
    }
    var next;
    const auto slot=slotFor(id);
    if(slot.existsAsFile()) {
        if(slot.getSize()>4*1024*1024) { error="Saved AI is too large.";return false; }
        next=JSON::parse(slot);
    } else if(id=="personal") {
        error="My saved AI backup not found.";return false;
    } else {
        const String filename=id.fromFirstOccurrenceOf(":",false,false);
        if(filename.isEmpty() || filename.contains("/") || filename.contains("\\") ||
           filename.contains("..") || !filename.endsWith(".cynai.json")) {
            error="Unsafe AI model filename.";return false;
        }
        const auto dir=id.startsWith("bundled:")?
            File::getSpecialLocation(File::currentExecutableFile).getSiblingFile("CynAiProfiles"):
            rootDir.getChildFile("AIModels");
        const auto file=dir.getChildFile(filename);
        if(!file.existsAsFile() || file.getSize()>2*1024*1024) {
            error="Selected AI model file is missing.";return false;
        }
        const auto data=JSON::parse(file);
        if(data["format"].toString()!="cyn-ai-profile" || (int)data["schema"]!=1 ||
           data["model"]["architecture"].toString()!="cyn-audio-11x12x5") {
            error="The selected AI format is unsupported.";return false;
        }
        auto* obj=new DynamicObject();
        obj->setProperty("schema",1);
        obj->setProperty("weights",data["model"]["weights"]);
        obj->setProperty("ready",data["model"]["ready"]);
        obj->setProperty("examples",Array<var>{});
        next=var(obj);
    }
    const auto* weights=next["weights"].getArray();
    const auto* ready=next["ready"].getArray();
    if((int)next["schema"]!=1 || !weights || !ready ||
       weights->size()!=PersonalAudioModel::weightCount ||
       ready->size()!=PersonalAudioModel::outputs || !next["examples"].isArray()) {
        error="Incompatible model or saved learning history.";return false;
    }
    for(const auto& weight:*weights)
        if((!weight.isInt() && !weight.isInt64() && !weight.isDouble()) ||
           !std::isfinite((double)weight) || std::abs((double)weight)>100.) {
            error="Invalid neural model weights.";return false;
        }
    for(const auto& readyFlag:*ready) if(!readyFlag.isBool()) {
        error="Invalid neural output flags.";return false;
    }
    if(!prefs.copyFileTo(rootDir.getChildFile("preferences-before-profile-switch.json"))) {
        error="Could not back up your current preferences.";return false;
    }
    if(smart_enabled_) restoreSmartBaseline();
    if(smart_mode_==1) root->setProperty("movie_ai",next);
    else {
        root->setProperty("weights",next["weights"]);
        root->setProperty("ready",next["ready"]);
        root->setProperty("examples",next["examples"]);
    }
    if(!prefs.replaceWithText(JSON::toString(state))) {
        error="Could not write selected AI preferences.";return false;
    }
    loadSmartModel();
    settings_.setString(smart_mode_==1?"cyn_active_ai_profile_movie":"cyn_active_ai_profile_music",id);
    smart_learning_message_="Active AI: "+found->name+" ("+
        String(smart_mode_==1?"Movie & TV":"Music")+").";
    error.clear();
    return true;
}

bool FxController::restorePreviousApexMusicAI(String& error)
{
    if(smart_mode_!=0 || getActiveAIProfileId()!="apex6m") {
        error="APEX 6M is not the active Music AI."; return false;
    }
    String previous=settings_.getString("cyn_apex_previous_music_profile");
    if(previous.isEmpty() || previous=="apex6m") previous="personal";
    return selectAIProfile(previous,error);
}

void FxController::setSmartSensitivity(float percent)
{
    if(!std::isfinite(percent)) return;
    percent=jlimit(0.0f,100.0f,percent);
    if(percent!=smart_sensitivity_) restoreSmartBaseline();
    smart_sensitivity_=percent; settings_.setDouble("ember_smart_sensitivity",percent);
}
void FxController::setSmartHeadroomCap(float percent)
{
    if(!std::isfinite(percent)) return;
    smart_headroom_cap_=jlimit(0.0f,100.0f,percent);
    settings_.setDouble("cyn_music_headroom_cap",smart_headroom_cap_);
}
void FxController::updateSmartListening()
{
    const auto input=dfx_dsp_.getInputAudioFeatures();
    const auto now=Time::currentTimeMillis();
    const bool fresh=input.sequence!=0 && input.sequence!=smart_last_sequence_;
    smart_stale_ticks_=fresh?0:jmin(100,smart_stale_ticks_+1);
    smart_last_sequence_=input.sequence;
    const bool active=audio_process_on_ && playback_device_available_ && FxModel::getModel().getPowerState();
    CynAudioScene scene;
    const bool yamnetLive=yamnet_worker_ && yamnet_worker_->latest(scene);
    if(fresh && active && input.rms>0.0001f) smart_history_.push_back({input,now,scene});
    while(!smart_history_.empty() && (smart_history_.front().id<now-60000 || smart_history_.size()>600)) smart_history_.erase(smart_history_.begin());
    if(!smart_enabled_) {
        apex_hybrid_active_=false;
        apex6m_active_=false;
        apex_flow_.reset();
        return;
    }
    // APEX uses the ACTUAL 6,307,477-parameter ONNX model, not a renamed
    // 209-weight profile. Inference runs off-thread; other Smart Listening
    // controls continue to work instead of stopping at an early return.
    apex6m_active_=smart_mode_==0 && getActiveAIProfileId()=="apex6m";
    std::array<float,8> neuralBands {};
    const bool apexNeuralLive=apex6m_active_ && apex_worker_ &&
        apex_worker_->latest(neuralBands) && active &&
        smart_stale_ticks_<4 && input.rms>0.0001f &&
        smart_sensitivity_>0.0f;
    const bool listening=active && smart_stale_ticks_<4 && input.rms>0.0001f;
    const bool movie=smart_mode_==1;
    const auto& model=movie?movie_model_:smart_model_;
    // Music recognition uses relative spectral shape and crest, not upstream
    // output-volume level. Raw input RMS remains available for clipping protection.
    const auto predictions=model.predict(CynLevelInvariant::forRecognition(input));
    const auto& sceneHead=movie?movie_scene_preferences_:music_scene_preferences_;
    CynMixDecision decision=CynFlagshipMixer::decide(scene,input,movie);
    if(!movie) {
        // Relative musical identity drives the model; measured original
        // input peak independently limits the gain permissions.
        const float crest=std::exp(jlimit(0.0f,3.5f,input.values[8]));
        const float rawPeak=(std::isfinite(input.rms)&&std::isfinite(crest))
            ? jmax(0.0f,input.rms)*crest : 10.0f;
        decision.headroom=CynSmartRange::headroomGate(
            decision.headroom,rawPeak,smart_headroom_cap_,false);
    }
    if(!movie && listening && yamnetLive && smart_sensitivity_>0.0f) {
        // Read real pre-effect RMS on the existing 100ms control timer.
        const auto arc=fresh
            ? music_crescendo_.observe(now,input.rms,scene.music,scene.speech,scene.silence)
            : music_crescendo_.current();
        CynFlagshipMixer::applyCrescendo(decision,scene,arc);
    } else music_crescendo_.reset();
    // The full 6M model and the lightweight APEX hybrid can both benefit
    // from CYN's independent musical passage controller, but remain distinct.
    apex_hybrid_active_=!movie &&
        getActiveAIProfileId()=="bundled:cyn-apex-flow-hybrid.cynai.json";
    const bool apexFlowActive=apex_hybrid_active_ || apex6m_active_;
    if(apexFlowActive && listening && yamnetLive && fresh && smart_sensitivity_>0.0f) {
        const float crest=std::exp(jlimit(0.0f,3.5f,input.values[8]));
        const float inputPeak=(std::isfinite(input.rms) && std::isfinite(crest))
            ? jmax(0.0f,input.rms)*crest : 10.0f;
        CynApexFlowHybrid::Observation sample;
        sample.ms=now;
        sample.rms=input.rms;
        sample.peak=inputPeak;
        sample.music=scene.music;
        sample.speech=scene.speech;
        sample.vocals=scene.vocals;
        sample.percussion=scene.percussion;
        sample.bassDrum=scene.bassDrum;
        sample.silence=scene.silence;
        sample.pretrainedSceneValid=scene.valid;
        const auto flow=apex_flow_.observe(sample);
        if(flow.valid) {
            decision.preference[0]=jlimit(0.0f,1.0f,
                decision.preference[0]+flow.clarityBias);
            decision.preference[3]=jlimit(0.0f,0.94f,
                decision.preference[3]+flow.dynamicsBias);
            decision.preference[4]=jlimit(0.0f,0.96f,
                decision.preference[4]+flow.bassBias);
            decision.lowEqDb=jlimit(-2.5f,2.7f,
                decision.lowEqDb+flow.lowEqDb);
        }
    } else if(!apexFlowActive || !listening || !yamnetLive ||
              smart_sensitivity_<=0.0f) {
        apex_flow_.reset(); // Stale pretrained evidence cannot affect DSP.
    }
    // Preset selection is passage-level (scene features refresh ~100ms), not
    // per-sample. Even when a new preset is chosen, the DSP never reloads .fac;
    // effects and EQ move gradually toward the cached REAL preset values.
    if(smart_preset_cycling_ && !movie && cycling_dirty_)
        rebuildPresetCyclingProfiles();
    if(smart_preset_cycling_ && !movie && !cycling_profiles_.empty()) {
        const auto chosen=cycling_policy_.update(cycling_signatures_,scene,
                                                  (uint64_t)now,listening && fresh);
        const auto& desired=cycling_profiles_[(size_t)chosen];
        smart_cycling_preset_name_=desired.name;
        const auto& goals=listening?desired.effects:smart_effects_;
        for(int e=0;e<5;++e) {
            float goal=goals[(size_t)e];
            if(listening && (e==0 || e==3 || e==4) && goal>smart_effects_[(size_t)e])
                goal=smart_effects_[(size_t)e]+
                    (goal-smart_effects_[(size_t)e])*decision.headroom;
            cycling_effects_[(size_t)e]+=jlimit(-0.24f,0.24f,
                goal-cycling_effects_[(size_t)e]);
        }
        const auto& eqGoal=listening?desired.eq:smart_eq_;
        const int eqCount=jmin((int)cycling_eq_.size(),(int)eqGoal.size());
        for(int b=0;b<eqCount;++b) {
            float goal=eqGoal[(size_t)b];
            if(listening && goal>smart_eq_[(size_t)b])
                goal=smart_eq_[(size_t)b]+
                    (goal-smart_eq_[(size_t)b])*decision.headroom;
            cycling_eq_[(size_t)b]+=jlimit(-0.16f,0.16f,
                goal-cycling_eq_[(size_t)b]);
        }
    }
    const float amount=CynSmartRange::normalized(smart_sensitivity_);
    const float depth=jmin(1.0f,amount*2);

    // Fuse scene and spectral analysis with optional personal preferences.
    const float musicUp[5]={3.4f,1.4f,1.4f,4.2f,5.6f};
    const float movieUp[5]={2.5f,0.8f,1.0f,0.8f,0.8f};
    const float down[5]={0.65f,0.9f,0.9f,0.65f,0.85f};
    for(int e=0;e<5;++e) {
        float desired=0.5f;
        if(listening) {
            if(yamnetLive) {
                desired=decision.preference[(size_t)e];
                if(model.ready[e])
                    desired=0.78f*desired+0.22f*predictions[(size_t)e];
                float contextual=0.5f,confidence=0;
                if(sceneHead.predict(scene,e,contextual,confidence))
                    desired=(1.0f-confidence)*desired+confidence*contextual;
            } else if(!movie) {
                desired=decision.preference[(size_t)e];
                if(model.ready[e]) desired=0.78f*desired+0.22f*predictions[(size_t)e];
            } else if(model.ready[e]) desired=predictions[(size_t)e];
            else if(movie && e==0) desired=0.58f;
        }
        desired=jlimit(0.0f,1.0f,desired);
        if(!movie && listening)
            desired=CynSmartRange::expressiveScore(desired,smart_sensitivity_,false);
        smart_scores_[e]+=(desired-smart_scores_[e])*
            CynSmartRange::scoreFollower(smart_sensitivity_,desired>smart_scores_[e],movie);
        // Below 55%, retain the old restrained DSP mix. As sensitivity moves
        // toward 100%, grant Music AI progressively more of each effect's full
        // 0..10 range instead of a fixed +1.4/+5.6 band around the preset.
        // Neutral decisions keep the manual baseline. Clarity, dynamics and
        // bass positive shifts still require measured pre-effect headroom.
        const float up=movie?movieUp[e]:musicUp[e];
        float target=CynSmartRange::effectTarget(
            (smart_preset_cycling_&&!movie&&!cycling_profiles_.empty()?
                cycling_effects_[(size_t)e]:smart_effects_[e]),
            smart_scores_[e],smart_sensitivity_,up,down[e],
            decision.headroom,movie,(e==3 || e==4 || (e==0 && smart_sensitivity_>55.0f)));
        if(movie && e==4) target=jmin(target,jmax(smart_effects_[4],6.3f));
        if(movie && e==3) target=jmin(target,jmax(smart_effects_[3],5.0f));
        target=jlimit(0.0f,10.0f,target);
        const float current=dfx_dsp_.getEffectValue((DfxDsp::Effect)e)*10;
        if(std::abs(target-current)>0.04f) {
            const float slew=CynSmartRange::effectStep(smart_sensitivity_,movie);
            dfx_dsp_.setEffectValue((DfxDsp::Effect)e,current+jlimit(-slew,slew,target-current));
        }
    }
    // Temporary signed spectral corrections; baseline restores on stop/bypass.
    const float bassDirection=jmin(0.0f,2.0f*(smart_scores_[4]-0.5f));
    for(int b=0;b<jmin(getNumEqBands(),(int)smart_eq_.size());++b) {
        const float freq=dfx_dsp_.getEqBandFrequency(b);
        const float weight=jlimit(0.0f,1.0f,(300.0f-freq)/200.0f);
        const float cycledBaseline=smart_preset_cycling_&&!movie&&
            b<(int)cycling_eq_.size()&&!cycling_profiles_.empty()?
            cycling_eq_[(size_t)b]:smart_eq_[(size_t)b];
        float target=cycledBaseline;
        if(movie && target>0)
            target*=1.0f+bassDirection*depth*0.65f*weight;
        else if(!movie && listening) {
            const float correction=CynFlagshipMixer::musicEqCorrection(
                decision,scene,freq,cycledBaseline);
            // More expressive signed EQ at high sensitivity. Restrict
            // positive gain as input peak headroom vanishes, and avoid
            // excessive upper treble boost that would amplify sibilance.
            float delta=correction*CynSmartRange::eqAmount(smart_sensitivity_,movie);
            if(delta>0.0f) delta*=jlimit(0.0f,1.0f,decision.headroom);
            target+=jlimit(-4.2f,freq>4200.0f?2.0f:4.3f,delta);
            if(apexNeuralLive) {
                // APEX's pretrained spectral-mask response changes LIVE DSP
                // on top of the scene-aware bass, clarity and flow controls.
                // This is neural EQ, not full per-sample STFT denoising.
                float learned=CynApex6MWorker::interpolate(neuralBands,freq)*
                    CynSmartRange::normalized(smart_sensitivity_);
                if(learned>0.0f) learned*=jlimit(0.0f,1.0f,decision.headroom);
                target+=jlimit(-1.35f,1.15f,learned);
            }
        }
        target=jlimit(MIN_GAIN,MAX_GAIN,target);
        const float current=dfx_dsp_.getEqBandBoostCut(b);
        if(std::abs(target-current)>0.04f) {
            const float slew=CynSmartRange::eqStep(smart_sensitivity_,movie);
            dfx_dsp_.setEqBandBoostCut(b,current+jlimit(-slew,slew,target-current));
        }
    }
    // Preserve system volume and master gain; never override manual presets.
    float leveling=smart_leveling_;
    if(listening && amount>0)
        leveling=jmax(smart_leveling_,(yamnetLive || !movie?decision.levelingDb:0.4f)*amount);
    if(std::abs(dfx_dsp_.getVolumeLeveling()-leveling)>0.05f)
        dfx_dsp_.setVolumeLeveling(leveling);
}
String FxController::getAppliedMixSummary()
{
    const String mode = smart_mode_ == 1 ? "Movie AI" : "Music AI";
    if (!smart_enabled_) return mode + " / off";
    if (!FxModel::getModel().getPowerState() || !audio_process_on_ ||
        !playback_device_available_ || smart_stale_ticks_ >= 4)
        return mode + " / waiting for audio";

    // These are deltas of the actual applied DSP state against the saved preset,
    // not the model's desired values or artificial animation data.
    const char* const names[5] = { "Clarity", "Ambience", "Surround", "Dynamics", "Bass" };
    String status = mode;
    int effectsShown = 0;
    for (int e = 0; e < 5; ++e)
    {
        const float applied = dfx_dsp_.getEffectValue(static_cast<DfxDsp::Effect>(e)) * 10.0f;
        const float difference = applied - smart_effects_[(size_t)e];
        if (std::isfinite(difference) && std::abs(difference) >= 0.15f && effectsShown < 2)
        {
            status += " / " + String(names[e]) + (difference >= 0 ? " +" : " ")
                    + String(difference, 1);
            ++effectsShown;
        }
    }
    int shapedBands = 0;
    const int bands = jmin(getNumEqBands(), static_cast<int>(smart_eq_.size()));
    for (int b = 0; b < bands; ++b)
        if (std::abs(dfx_dsp_.getEqBandBoostCut(b) - smart_eq_[(size_t)b]) >= 0.10f)
            ++shapedBands;
    if (smart_preset_cycling_ && smart_mode_==0 && cycling_profiles_.size()>1)
        status += " / Auto preset: "+smart_cycling_preset_name_;
    if (shapedBands > 0)
        status += " / EQ " + String(shapedBands) + (shapedBands == 1 ? " band" : " bands");

    const float leveling = dfx_dsp_.getVolumeLeveling() - smart_leveling_;
    if (effectsShown == 0 && shapedBands == 0 && std::isfinite(leveling) && std::abs(leveling) >= 0.15f)
        status += " / Leveling +" + String(leveling, 1) + " dB";
    else if (effectsShown == 0 && shapedBands == 0)
        status += " / holding preset";
    return status;
}

String FxController::getSmartSummary() const
{
    const String mode=smart_mode_==1?"Movie AI":"Music AI";
    if(!smart_enabled_) return mode+" / learning available (processing off)";
    if(smart_stale_ticks_>=4) return mode+" / waiting for audio";
    CynAudioScene scene;
    if(smart_mode_==0 && apex6m_active_) {
        std::array<float,8> response {};
        return apex_worker_ && apex_worker_->latest(response)
            ? "Music AI / CYN APEX 6M / Neural inference ACTIVE + adaptive sound"
            : "Music AI / CYN APEX 6M / waiting for 48 kHz neural audio";
    }
    if(smart_mode_==0 && apex_hybrid_active_) {
        if(!yamnet_worker_ || !yamnet_worker_->latest(scene))
            return "Music AI / APEX Flow Hybrid / waiting for pretrained YAMNet";
        const auto flow=apex_flow_.current();
        const String state=flow.impact>0.12f?"impact":flow.build>0.13f?
            "building":flow.breakdown>0.13f?"breakdown":"steady";
        return "Music AI / APEX Flow Hybrid / "+state+
               " / music "+String(scene.music*100.0f,0)+
               "% / percussion "+String(scene.percussion*100.0f,0)+"%";
    }
    if(yamnet_worker_ && yamnet_worker_->latest(scene))
        return mode+(smart_mode_==0?" / YAMNet + Balance / music ":" / YAMNet flagship / music ")+String(scene.music*100.0f,0)+
            "% / drums "+String(scene.percussion*100.0f,0)+
            "% / speech "+String(scene.speech*100.0f,0)+"%";
    return mode+(smart_mode_==0?" / Balance model active (YAMNet unavailable)":" / personal model fallback active");
}
String FxController::getLearningStatus() const
{
    if(smart_training_) return "Training locally...";
    const auto& examples=smart_mode_==1?movie_examples_:smart_examples_;
    const auto& model=smart_mode_==1?movie_model_:smart_model_;
    int user=0,heads=0;
    for(const auto& e:examples) if(!e.starter) ++user;
    for(bool r:model.ready) if(r) ++heads;
    return String(smart_mode_==1?"Movie AI / ":"Music AI / ")+String(user)+
        " labeled frames / "+String(heads)+" effects trained. "+smart_learning_message_;
}
void FxController::labelAudio(const std::vector<AudioFrame>& frames,int effect,bool more)
{
    if(smart_training_ || effect<0 || effect>=5 || frames.empty()) return;
    auto& examples=smart_mode_==1?movie_examples_:smart_examples_;
    for(const auto& frame:frames) {
        auto existing=std::find_if(examples.begin(),examples.end(),[&](const AudioExample& e){
            return !e.starter && e.id==frame.id && e.effect==effect;
        });
        AudioExample example {frame.audio,effect,more,frame.id,false,frame.scene};
        if(existing!=examples.end()) *existing=example;
        else examples.push_back(example);
    }
    while(examples.size()>2208) {
        auto oldest=std::find_if(examples.begin(),examples.end(),[](const AudioExample& e){return !e.starter;});
        if(oldest==examples.end()) break;
        examples.erase(oldest);
    }
    smart_learning_message_=smart_mode_==1 ?
        "Movie feedback stored. Train Movie AI to apply." :
        "Music feedback stored. Train Music AI to apply.";
    saveSmartModel();
}
bool FxController::getLiveYamNetScene(CynAudioScene& out) const
{
    if(!yamnet_worker_ || !playback_device_available_ ||
       !FxModel::getModel().getPowerState() || !audio_process_on_)
        return false;
    return yamnet_worker_->latest(out);
}

bool FxController::getLiveNeuralActivity(NeuralActivitySnapshot& out) const
{
    // The UI and audio-history maintenance run on the message thread; the trainer
    // optimizes a separate model copy until its message-thread commit callback.
    if(smart_history_.empty() || !playback_device_available_ ||
       !FxModel::getModel().getPowerState())
        return false;
    const auto& frame=smart_history_.back();
    if(frame.audio.rms<=0.0001f || Time::currentTimeMillis()-frame.id>1200)
        return false;
    out=(smart_mode_==1?movie_model_:smart_model_).trace(frame.audio);
    return true;
}
void FxController::trainSmartModel()
{
    if(smart_training_) return;
    const int trainingMode=smart_mode_;
    auto examples=trainingMode==1?movie_examples_:smart_examples_;
    auto model=std::make_shared<PersonalAudioModel>(trainingMode==1?movie_model_:smart_model_);
    smart_training_epoch_.store(0,std::memory_order_relaxed);
    smart_training_=true;
    Component::SafePointer<FxMainWindow> window(main_window_);
    auto* epochs=&smart_training_epoch_;
    Thread::launch([examples,model,window,epochs,trainingMode] {
        const bool success=model->fit(examples,epochs);
        MessageManager::callAsync([model,window,success,trainingMode] {
            if(window==nullptr) return;
            auto& c=FxController::getInstance(); c.smart_training_=false;
            if(success) {
                c.restoreSmartBaseline();
                if(trainingMode==1) c.movie_model_=*model;
                else c.smart_model_=*model;
                c.rebuildScenePreferences();
                c.saveSmartModel();
                c.smart_learning_message_=trainingMode==1 ?
                    "Movie AI trained. Refine dialogue and sound-effect labels." :
                    "Music AI trained. Refine your listening preferences.";
            } else {
                c.smart_learning_message_="Label More and Less for the same effect before training.";
            }
        });
    });
}
void FxController::rebuildScenePreferences()
{
    music_scene_preferences_.clear();
    movie_scene_preferences_.clear();
    for(const auto& example:smart_examples_) {
        if(!example.starter && example.effect>=0 && example.effect<5 &&
           smart_model_.ready[(size_t)example.effect])
            music_scene_preferences_.observe(example.scene,example.effect,example.more);
    }
    for(const auto& example:movie_examples_) {
        if(!example.starter && example.effect>=0 && example.effect<5 &&
           movie_model_.ready[(size_t)example.effect])
            movie_scene_preferences_.observe(example.scene,example.effect,example.more);
    }
}

void FxController::saveSmartModel()
{
    auto* root=new DynamicObject(); root->setProperty("schema",1);
    Array<var> weights,ready,examples;
    for(float w:smart_model_.weights) weights.add(w); for(bool r:smart_model_.ready) ready.add(r);
    for(auto& e:smart_examples_) {
        auto* item=new DynamicObject(); Array<var> x; for(float f:e.audio.values) x.add(f);
        item->setProperty("x",x); item->setProperty("effect",e.effect); item->setProperty("more",e.more);
        item->setProperty("id",(int64)e.id); item->setProperty("starter",e.starter);
        if(e.scene.valid) {
            Array<var> context; for(float feature:e.scene.embedding) context.add(feature);
            item->setProperty("yamnet",context);
        }
        examples.add(var(item));
    }
    root->setProperty("weights",weights); root->setProperty("ready",ready); root->setProperty("examples",examples);
    // Movie AI has a separate model and label history inside the same local
    // preference file. Existing music-only preferences load unchanged.
    auto* movie=new DynamicObject();
    movie->setProperty("schema",1);
    Array<var> movieWeights,movieReady,movieRows;
    for(float w:movie_model_.weights) movieWeights.add(w);
    for(bool ready:movie_model_.ready) movieReady.add(ready);
    for(const auto& e:movie_examples_) {
        auto* item=new DynamicObject();
        Array<var> features;
        for(float val:e.audio.values) features.add(val);
        item->setProperty("x",features);
        item->setProperty("effect",e.effect);
        item->setProperty("more",e.more);
        item->setProperty("id",(int64)e.id);
        item->setProperty("starter",e.starter);
        if(e.scene.valid) {
            Array<var> context; for(float feature:e.scene.embedding) context.add(feature);
            item->setProperty("yamnet",context);
        }
        movieRows.add(var(item));
    }
    movie->setProperty("weights",movieWeights);
    movie->setProperty("ready",movieReady);
    movie->setProperty("examples",movieRows);
    root->setProperty("movie_ai",var(movie));
    const auto file=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Cyn/preferences.json");
    file.getParentDirectory().createDirectory();
    if(!file.replaceWithText(JSON::toString(var(root)))) smart_learning_message_="Could not save local learning data.";
}
void FxController::initialiseMovieStarter()
{
    // Prefer the separate, bundled Cinema AI model trained for movie-like
    // spectral targets. It has only synthetic evaluation, not validated speech
    // recognition or proven real-world dialogue intelligibility. Never use
    // private Music AI weights as an implicit Movie AI seed.
    movie_model_=PersonalAudioModel();
    movie_model_.ready.fill(false);
    const auto app=File::getSpecialLocation(File::currentExecutableFile);
    const auto cinema=app.getSiblingFile("CynAiProfiles")
        .getChildFile("cyn-cinema.cynai.json");
    if(cinema.existsAsFile() && cinema.getSize()<2*1024*1024) {
        const auto package=JSON::parse(cinema);
        const auto nested=package["model"];
        const auto* weights=nested["weights"].getArray();
        const auto* ready=nested["ready"].getArray();
        if(package["format"].toString()=="cyn-ai-profile" &&
           (int)nested["schema"]==1 &&
           nested["architecture"].toString()=="cyn-audio-11x12x5" &&
           weights && weights->size()==PersonalAudioModel::weightCount &&
           ready && ready->size()==PersonalAudioModel::outputs) {
            PersonalAudioModel candidate;
            bool valid=true;
            for(int i=0;i<weights->size();++i) {
                const auto& item=weights->getReference(i);
                if(!item.isDouble() && !item.isInt() && !item.isInt64()) {
                    valid=false; break;
                }
                const float value=(float)(double)item;
                if(!std::isfinite(value) || std::abs(value)>100.0f) {
                    valid=false; break;
                }
                candidate.weights[i]=value;
            }
            for(int i=0;i<PersonalAudioModel::outputs;++i) {
                if(!ready->getReference(i).isBool()) valid=false;
                else candidate.ready[i]=(bool)ready->getReference(i);
            }
            if(valid) {
                movie_model_=candidate;
                return;
            }
        }
    }
    // Fallback for portable layouts that lack the Cinema AI bundle:
    // share the tested base acoustic representation, but do NOT activate
    // heads trained only on generic synthetic music-like examples.
    const auto starter=app.getSiblingFile("cyn-starter.json");
    if(!starter.existsAsFile() || starter.getSize()>4*1024*1024) return;
    const auto data=JSON::parse(starter);
    const auto* weights=data["weights"].getArray();
    if((int)data["schema"]!=1 || weights==nullptr ||
       weights->size()!=PersonalAudioModel::weightCount) return;
    PersonalAudioModel candidate;
    for(int i=0;i<weights->size();++i) {
        const auto& item=weights->getReference(i);
        if(!item.isDouble() && !item.isInt() && !item.isInt64()) return;
        const float value=(float)(double)item;
        if(!std::isfinite(value) || std::abs(value)>100.0f) return;
        candidate.weights[i]=value;
    }
    candidate.ready.fill(false);
    movie_model_=candidate;
}
void FxController::loadSmartModel(bool starterOnly)
{
    auto file=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Cyn/preferences.json");
    if(starterOnly || !file.existsAsFile()) file=File::getSpecialLocation(File::currentExecutableFile).getSiblingFile("cyn-starter.json");
    if(!file.existsAsFile() || file.getSize()>4*1024*1024) return;
    auto root=JSON::parse(file); auto* weights=root["weights"].getArray(); auto* ready=root["ready"].getArray();
    if((int)root["schema"]!=1 || !weights || weights->size()!=PersonalAudioModel::weightCount || !ready || ready->size()!=5) return;
    PersonalAudioModel next;
    for(int i=0;i<weights->size();++i) { float w=(float)(double)weights->getReference(i); if(!std::isfinite(w) || std::abs(w)>100) return; next.weights[i]=w; }
    for(int i=0;i<5;++i) next.ready[i]=(bool)ready->getReference(i);
    std::vector<AudioExample> examples;
    if(auto* rows=root["examples"].getArray()) for(auto& row:*rows) {
        if(examples.size()>=2208) break;
        auto* x=row["x"].getArray(); int effect=(int)row["effect"]; if(!x || x->size()!=11 || effect<0 || effect>=5) continue;
        AudioExample e; bool valid=true;
        for(int i=0;i<11;++i) { e.audio.values[i]=(float)(double)x->getReference(i); valid=valid && std::isfinite(e.audio.values[i]); }
        if(!valid) continue;
        e.effect=effect; e.more=(bool)row["more"]; e.id=(int64)row["id"]; e.starter=(bool)row["starter"];
        if(auto* context=row["yamnet"].getArray()) if(context->size()==CynAudioScene::embeddingSize) {
            bool ok=true;
            for(int i=0;i<CynAudioScene::embeddingSize;++i) {
                float x=(float)(double)context->getReference(i);
                if(!std::isfinite(x) || std::abs(x)>1.1f) {ok=false; break;}
                e.scene.embedding[(size_t)i]=x;
            }
            e.scene.valid=ok;
        }
        examples.push_back(e);
    }
    smart_model_=next; smart_examples_=std::move(examples);
    if(!starterOnly) {
        // Movie entries are optional for previous versions of Cyn. Validate
        // every weight before accepting the separate film model.
        auto savedMovie=root["movie_ai"];
        auto* movieWeights=savedMovie["weights"].getArray();
        auto* movieReady=savedMovie["ready"].getArray();
        if((int)savedMovie["schema"]==1 && movieWeights &&
           movieWeights->size()==PersonalAudioModel::weightCount &&
           movieReady && movieReady->size()==PersonalAudioModel::outputs) {
            PersonalAudioModel movieNext;
            bool valid=true;
            for(int i=0;i<movieWeights->size();++i) {
                const auto& weight=movieWeights->getReference(i);
                const float value=(float)(double)weight;
                if(!std::isfinite(value) || std::abs(value)>100.0f) {
                    valid=false; break;
                }
                movieNext.weights[i]=value;
            }
            if(valid) {
                for(int i=0;i<PersonalAudioModel::outputs;++i)
                    movieNext.ready[i]=(bool)movieReady->getReference(i);
                std::vector<AudioExample> movieRows;
                if(auto* rows=savedMovie["examples"].getArray()) {
                    for(const auto& row:*rows) {
                        if(movieRows.size()>=2208) break;
                        auto* features=row["x"].getArray();
                        int effect=(int)row["effect"];
                        if(!features || features->size()!=11 || effect<0 || effect>=5) continue;
                        AudioExample example;
                        bool valuesValid=true;
                        for(int i=0;i<11;++i) {
                            example.audio.values[i]=(float)(double)features->getReference(i);
                            valuesValid=valuesValid && std::isfinite(example.audio.values[i]);
                        }
                        if(!valuesValid) continue;
                        example.effect=effect;
                        example.more=(bool)row["more"];
                        example.id=(int64)row["id"];
                        example.starter=(bool)row["starter"];
                        if(auto* context=row["yamnet"].getArray()) if(context->size()==CynAudioScene::embeddingSize) {
                            bool ok=true;
                            for(int i=0;i<CynAudioScene::embeddingSize;++i) {
                                float x=(float)(double)context->getReference(i);
                                if(!std::isfinite(x) || std::abs(x)>1.1f) {ok=false; break;}
                                example.scene.embedding[(size_t)i]=x;
                            }
                            example.scene.valid=ok;
                        }
                        movieRows.push_back(example);
                    }
                }
                movie_model_=movieNext;
                movie_examples_=std::move(movieRows);
            }
        }
    }
    rebuildScenePreferences();
}
void FxController::useStarterModel()
{
    if(smart_training_) return;
    restoreSmartBaseline();
    if(smart_mode_==1) {
        // Resetting movies must never remove music labels or user music weights.
        initialiseMovieStarter();
        movie_examples_.clear();
        smart_learning_message_="Movie AI reset. Dialogue focus starts conservatively; train with your film labels.";
    } else {
        smart_model_=PersonalAudioModel();
        smart_examples_.clear();
        loadSmartModel(true);
        smart_learning_message_="Music AI starter restored. Teach your music preferences.";
    }
    rebuildScenePreferences();
    saveSmartModel();
}

bool FxController::exportSmartProfile(const File& destination, const String& name, String& error) const
{
    if (smart_training_)
    {
        error = "Wait until local model training finishes.";
        return false;
    }
    const String safeName = name.trim().substring(0, 80);
    if (safeName.isEmpty())
    {
        error = "Enter a profile name before exporting.";
        return false;
    }
    auto* root = new DynamicObject();
    root->setProperty("format", "cyn-ai-profile");
    root->setProperty("schema", 1);
    auto* meta = new DynamicObject();
    meta->setProperty("name", safeName);
    meta->setProperty("description", "");
    meta->setProperty("author", "");
    meta->setProperty("license", "CC-BY-4.0");
    meta->setProperty("listeningMode", smart_mode_==1 ? "movie" : "music");
    root->setProperty("profile", var(meta));

    auto* model = new DynamicObject();
    model->setProperty("schema", 1);
    model->setProperty("architecture", "cyn-audio-11x12x5");
    Array<var> weights, ready;
    const auto& selected=smart_mode_==1?movie_model_:smart_model_;
    for (const float weight : selected.weights) weights.add(weight);
    for (const bool trained : selected.ready) ready.add(trained);
    model->setProperty("weights", weights);
    model->setProperty("ready", ready);
    root->setProperty("model", var(model));

    // No audio samples, device names, private labels or training timestamps are shared.
    if (!destination.replaceWithText(JSON::toString(var(root), false)))
    {
        error = "Could not save the AI profile. Check write permissions.";
        return false;
    }
    error.clear();
    return true;
}

bool FxController::importSmartProfile(const File& source, String& error)
{
    if (smart_training_)
    {
        error = "Wait for the current training to finish before importing.";
        return false;
    }
    if (!source.existsAsFile() || source.getSize() > 2 * 1024 * 1024)
    {
        error = "This is not a valid Cyn AI profile file (2 MB maximum).";
        return false;
    }

    const auto package = JSON::parse(source);
    if (!package.isObject()
        || package["format"].toString() != "cyn-ai-profile"
        || (int)package["schema"] != 1)
    {
        error = "Expected a Cyn AI Hub profile (format cyn-ai-profile, schema 1).";
        return false;
    }
    const auto model = package["model"];
    const auto* weights = model["weights"].getArray();
    const auto* ready = model["ready"].getArray();
    if ((int)model["schema"] != 1
        || model["architecture"].toString() != "cyn-audio-11x12x5"
        || weights == nullptr || weights->size() != PersonalAudioModel::weightCount
        || ready == nullptr || ready->size() != PersonalAudioModel::outputs)
    {
        error = "This profile uses an unsupported or invalid neural model.";
        return false;
    }

    PersonalAudioModel incoming;
    for (int i = 0; i < PersonalAudioModel::weightCount; ++i)
    {
        const auto& input = weights->getReference(i);
        if (!input.isDouble() && !input.isInt() && !input.isInt64())
        {
            error = "The profile contains a nonnumeric model weight.";
            return false;
        }
        const double value = (double)input;
        if (!std::isfinite(value) || std::abs(value) > 100.0)
        {
            error = "The profile contains an unsafe or invalid model weight.";
            return false;
        }
        incoming.weights[i] = (float)value;
    }
    for (int i = 0; i < PersonalAudioModel::outputs; ++i)
    {
        const auto& flag = ready->getReference(i);
        if (!flag.isBool())
        {
            error = "The profile contains an invalid trained-effect flag.";
            return false;
        }
        incoming.ready[i] = (bool)flag;
    }

    const auto bundledDir=File::getSpecialLocation(File::currentExecutableFile).getSiblingFile("CynAiProfiles");
    const bool bundled=source.getParentDirectory()==bundledDir;
    const String id=bundled?"bundled:"+source.getFileName():
        "imported:custom-"+juce::Uuid().toString()+".cynai.json";
    File localCopy;
    if(!bundled) {
        const auto dir=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Cyn/AIModels");
        if(dir.createDirectory().failed()) {error="Cannot create your local AI library.";return false;}
        localCopy=dir.getChildFile(id.fromFirstOccurrenceOf(":",false,false));
        if(!source.copyFileTo(localCopy)) {error="Could not keep the imported AI profile.";return false;}
    }
    if(!selectAIProfile(id,error)) {
        if(!bundled) localCopy.deleteFile();
        return false;
    }
    return true;
}

void FxController::setReducedMotion(bool value)
{
    FxTheme::setReducedMotion(value); settings_.setBool("cyn_reduce_motion",value);
    if(main_window_) main_window_->sendLookAndFeelChange();
}

void FxController::setSmoothAnimations(bool value)
{
    FxTheme::setSmoothAnimations(value);
    settings_.setBool("cyn_smooth_animations", value);
    if (main_window_) main_window_->sendLookAndFeelChange();
}
