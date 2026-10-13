/*
Cyn Audio Repair (Windows) - opt-in detection and Windows Update driver download.
Copyright (C) 2026 Cyn contributors. GNU AGPL-3.0-or-later.
No driver installation, removal, service restart or system reboot is performed.
*/
#pragma once
#include <JuceHeader.h>
#include "../Source/Utils/Settings/Settings.h"
#include <setupapi.h>
#include <cfgmgr32.h>
#include <functional>
#include <vector>
#include <string>
#include <cwchar>

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Cfgmgr32.lib")

class CynAudioRepairService final : private juce::Timer
{
public:
    enum class Permission { AskEveryTime = 0, AlwaysAllow = 1, NeverDownload = 2 };

    CynAudioRepairService(FxSound::Settings& settings, std::function<void()> refreshDeviceList)
        : settings_(settings), refreshDeviceList_(std::move(refreshDeviceList))
    {
        status_ = "Auto Repair is off. No monitoring or driver downloads.";
    }

    ~CynAudioRepairService() override { stopMonitoring(); }

    bool isEnabled() const { return settings_.getBool("cyn_repair_enabled", false); }
    bool allowMinorRecovery() const { return settings_.getBool("cyn_repair_minor_recovery", false); }
    Permission getPermission() const
    {
        return static_cast<Permission>(juce::jlimit(0, 2, settings_.getInt("cyn_repair_download_permission", 0)));
    }
    juce::String getStatus() const { return status_; }
    juce::String getLastAction() const { return settings_.getString("cyn_repair_last_action"); }
    bool isBusy() const { return stage_ != Stage::Idle; }

    void startIfEnabled()
    {
        if (isEnabled())
        {
            status_ = "Monitoring audio device errors. Drivers will never install automatically.";
            lastCheckMs_ = 0;
            startTimer(1000);
        }
    }
    void setEnabled(bool enabled)
    {
        settings_.setBool("cyn_repair_enabled", enabled);
        if (enabled) startIfEnabled();
        else stopMonitoring();
    }
    void setMinorRecovery(bool enabled) { settings_.setBool("cyn_repair_minor_recovery", enabled); }
    void setPermission(Permission permission)
    {
        settings_.setInt("cyn_repair_download_permission", static_cast<int>(permission));
        if (permission == Permission::NeverDownload && stage_ == Stage::Downloading)
            abortWorker("Driver download stopped by permission change.");
    }
    void resetPermissions() { setPermission(Permission::AskEveryTime); }

    juce::String diagnoseNow()
    {
        auto problem = inspectAudioDevices();
        if (problem.hasProblem)
        {
            status_ = "Windows reports audio device problem " + juce::String(problem.code)
                + ": " + problem.name + ".";
        }
        else
        {
            status_ = "No Windows Plug and Play driver error detected. "
                "A connected-but-silent headset may still need an output or Bluetooth check.";
        }
        remember(status_);
        return status_;
    }

    void scanForDriverUpdates()
    {
        if (!isEnabled() || isBusy()) return;
        beginWorker(Stage::Scanning, {});
    }

    // Windows owns the driver store and installation. Do not bypass that consent.
    static void openWindowsOptionalUpdates()
    {
        juce::URL("ms-settings:windowsupdate-optionalupdates").launchInDefaultBrowser();
    }

private:
    enum class Stage { Idle, Scanning, Downloading };
    struct Problem
    {
        bool hasProblem = false;
        int code = 0;
        juce::String deviceId, name;
        bool physicalDriver = false;
    };
    struct Candidate
    {
        juce::String title, updateId, deviceId;
        int revision = -1;
    };

    static Problem inspectAudioDevices()
    {
        Problem result;
        HDEVINFO deviceSet = SetupDiGetClassDevsW(nullptr, nullptr, nullptr,
                                                  DIGCF_PRESENT | DIGCF_ALLCLASSES);
        if (deviceSet == INVALID_HANDLE_VALUE) return result;
        SP_DEVINFO_DATA dev{};
        dev.cbSize = sizeof(dev);
        for (DWORD index = 0; SetupDiEnumDeviceInfo(deviceSet, index, &dev); ++index)
        {
            wchar_t category[128] = {};
            if (!SetupDiGetDeviceRegistryPropertyW(deviceSet, &dev, SPDRP_CLASS,
                    nullptr, reinterpret_cast<PBYTE>(category), sizeof(category), nullptr))
                continue;
            if (_wcsicmp(category, L"MEDIA") != 0 && _wcsicmp(category, L"AudioEndpoint") != 0)
                continue;
            ULONG flags = 0, problemCode = 0;
            if (CM_Get_DevNode_Status(&flags, &problemCode, dev.DevInst, 0) != CR_SUCCESS
                || ((flags & DN_HAS_PROBLEM) == 0 && problemCode == 0))
                continue;

            wchar_t identifier[4096] = {};
            if (CM_Get_Device_IDW(dev.DevInst, identifier, 4096, 0) != CR_SUCCESS)
                continue;
            wchar_t name[512] = {};
            if (!SetupDiGetDeviceRegistryPropertyW(deviceSet, &dev, SPDRP_FRIENDLYNAME,
                nullptr, reinterpret_cast<PBYTE>(name), sizeof(name), nullptr))
            {
                SetupDiGetDeviceRegistryPropertyW(deviceSet, &dev, SPDRP_DEVICEDESC,
                    nullptr, reinterpret_cast<PBYTE>(name), sizeof(name), nullptr);
            }
            result.hasProblem = true;
            result.physicalDriver = _wcsicmp(category, L"MEDIA") == 0;
            result.code = static_cast<int>(problemCode);
            result.deviceId = identifier;
            result.name = juce::String(name).isNotEmpty() ? juce::String(name) : "Unnamed audio device";
            break;
        }
        SetupDiDestroyDeviceInfoList(deviceSet);
        return result;
    }

    void remember(const juce::String& line)
    {
        settings_.setString("cyn_repair_last_action",
            juce::Time::getCurrentTime().toString(true, true) + " - " + line);
    }

    void timerCallback() override
    {
        if (!isEnabled()) { stopMonitoring(); return; }
        pollWorker();
        const auto now = juce::Time::currentTimeMillis();
        if (isBusy() || now - lastCheckMs_ < 45000) return;
        lastCheckMs_ = now;
        const auto issue = inspectAudioDevices();
        if (!issue.hasProblem)
        {
            lastProblemId_.clear();
            return;
        }
        if (issue.deviceId == lastProblemId_ && now - lastProblemAtMs_ < 21600000)
            return; // Prevent a popup/download loop for the same driver issue.
        lastProblemId_ = issue.deviceId;
        lastProblemAtMs_ = now;
        status_ = "Windows reported a device problem (" + juce::String(issue.code) +
                  "): " + issue.name;
        remember(status_);
        if (allowMinorRecovery() && refreshDeviceList_)
        {
            refreshDeviceList_(); // Cyn output list only; no PnP driver changes.
            remember("Refreshed Cyn's audio output list.");
            if (!inspectAudioDevices().hasProblem)
            {
                status_ = "Audio output refreshed. Driver problem is no longer reported.";
                return;
            }
        }
        if (issue.physicalDriver && getPermission() != Permission::NeverDownload)
            beginWorker(Stage::Scanning, {}, issue.deviceId);
        else if (!issue.physicalDriver)
            status_ += " Logical audio endpoint: no automatic driver download attempted.";
    }

    void beginWorker(Stage nextStage, const Candidate& candidate,
                     const juce::String& targetDeviceId = {})
    {
        if (!isEnabled() || isBusy()) return;
        const auto script = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                                .getSiblingFile("CynAudioRepair.ps1");
        if (!script.existsAsFile())
        {
            status_ = "CynAudioRepair.ps1 missing. Reinstall Cyn to enable Windows Update scanning.";
            remember(status_);
            return;
        }
        const auto root = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("Cyn").getChildFile("AudioRepair");
        if (!root.createDirectory().wasOk())
        {
            status_ = "Could not prepare the repair log directory.";
            remember(status_);
            return;
        }
        resultFile_ = root.getChildFile(juce::Uuid().toString() + ".json");
        const juce::String powershell =
            juce::File::getSpecialLocation(juce::File::windowsSystemDirectory)
                .getChildFile("WindowsPowerShell").getChildFile("v1.0")
                .getChildFile("powershell.exe").getFullPathName();
        if (!juce::File(powershell).existsAsFile())
        {
            status_ = "Windows PowerShell is unavailable; cannot check Windows Update.";
            remember(status_);
            return;
        }
        // All dynamic arguments come from fixed files or Windows Update identity.
        // Re-validate the IDs on the PowerShell side before any download.
        juce::String command = "\"" + powershell + "\" -NoLogo -NoProfile -NonInteractive"
            " -ExecutionPolicy Bypass -File \"" + script.getFullPathName() + "\" -Action "
            + (nextStage == Stage::Scanning ? "Scan" : "Download")
            + " -ResultPath \"" + resultFile_.getFullPathName() + "\"";
        if (nextStage == Stage::Scanning && targetDeviceId.isNotEmpty())
        {
            if (targetDeviceId.containsAnyOf("\"'\r\n"))
            {
                status_ = "Rejected an invalid audio device identifier.";
                return;
            }
            command += " -TargetDeviceId \"" + targetDeviceId + "\"";
        }
        if (nextStage == Stage::Downloading)
        {
            if (candidate.updateId.containsAnyOf("\"'\r\n") ||
                candidate.deviceId.containsAnyOf("\"'\r\n") ||
                candidate.revision < 0)
            {
                status_ = "Rejected an invalid Windows Update identity.";
                return;
            }
            command += " -UpdateId \"" + candidate.updateId + "\" -Revision "
                    + juce::String(candidate.revision)
                    + " -DeviceId \"" + candidate.deviceId + "\"";
        }
        std::wstring mutableCommand(command.toWideCharPointer());
        std::vector<wchar_t> cmdline(mutableCommand.begin(), mutableCommand.end());
        cmdline.push_back(L'\0');
        STARTUPINFOW startup{};
        PROCESS_INFORMATION processInfo{};
        startup.cb = sizeof(startup);
        if (!CreateProcessW(powershell.toWideCharPointer(), cmdline.data(),
                            nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                            nullptr, nullptr, &startup, &processInfo))
        {
            status_ = "Windows could not start the driver repair checker.";
            remember(status_);
            return;
        }
        CloseHandle(processInfo.hThread);
        workerProcess_ = processInfo.hProcess;
        stage_ = nextStage;
        startedMs_ = juce::Time::currentTimeMillis();
        status_ = nextStage == Stage::Scanning
            ? "Searching Windows Update for matching audio hardware drivers..."
            : "Downloading the approved driver package through Windows Update...";
        remember(status_);
    }

    void pollWorker()
    {
        if (workerProcess_ == nullptr) return;
        if (WaitForSingleObject(workerProcess_, 0) == WAIT_TIMEOUT)
        {
            const auto timeout = stage_ == Stage::Scanning ? 180000 : 600000;
            if (juce::Time::currentTimeMillis() - startedMs_ > timeout)
                abortWorker("Windows Update timed out; no repair was installed.");
            return;
        }
        DWORD exitCode = 1;
        GetExitCodeProcess(workerProcess_, &exitCode);
        CloseHandle(workerProcess_);
        workerProcess_ = nullptr;
        const Stage completed = stage_;
        stage_ = Stage::Idle;
        const auto parsed = juce::JSON::parse(resultFile_.loadFileAsString());
        resultFile_.deleteFile();
        if (exitCode != 0 || !parsed.isObject())
        {
            status_ = "The Windows Update checker did not return a valid result.";
            remember(status_);
            return;
        }
        const auto state = parsed.getProperty("status", "").toString();
        status_ = parsed.getProperty("message", "Windows Update returned no description.").toString();
        remember(status_);
        if (completed == Stage::Scanning && state == "available" && isEnabled())
        {
            Candidate candidate;
            candidate.title = parsed.getProperty("title", "").toString();
            candidate.updateId = parsed.getProperty("updateId", "").toString();
            candidate.deviceId = parsed.getProperty("deviceId", "").toString();
            candidate.revision = static_cast<int>(parsed.getProperty("revision", -1));
            if (candidate.title.isEmpty() || candidate.updateId.isEmpty()) return;
            if (getPermission() == Permission::NeverDownload)
            {
                status_ = "Driver update found; downloads disabled by your settings.";
                return;
            }
            if (getPermission() == Permission::AskEveryTime)
            {
                const bool approved = juce::AlertWindow::showOkCancelBox(
                    juce::AlertWindow::QuestionIcon, "Cyn - permission to download",
                    "Windows Update found an audio driver for your hardware:\n\n"
                    + candidate.title + "\n\nSource: Windows Update\n"
                    "Download this driver package? Cyn will NOT install, remove or restart drivers.",
                    "Allow download", "Decline");
                if (!approved)
                {
                    status_ = "Driver download declined. No system changes made.";
                    remember(status_);
                    return;
                }
            }
            // Always Allow only skips the download prompt. It never authorizes installation.
            beginWorker(Stage::Downloading, candidate);
        }
    }

    void abortWorker(const juce::String& reason)
    {
        if (workerProcess_ != nullptr)
        {
            TerminateProcess(workerProcess_, 1);
            CloseHandle(workerProcess_);
            workerProcess_ = nullptr;
        }
        stage_ = Stage::Idle;
        resultFile_.deleteFile();
        status_ = reason;
        remember(reason);
    }
    void stopMonitoring()
    {
        stopTimer();
        if (isBusy()) abortWorker("Auto Repair disabled. Stopped local repair checker.");
        status_ = "Auto Repair is off. No monitoring or driver downloads.";
    }

    FxSound::Settings& settings_;
    std::function<void()> refreshDeviceList_;
    juce::String status_, lastProblemId_;
    juce::File resultFile_;
    HANDLE workerProcess_ = nullptr;
    Stage stage_ = Stage::Idle;
    juce::int64 lastCheckMs_ = 0, lastProblemAtMs_ = 0, startedMs_ = 0;
};
