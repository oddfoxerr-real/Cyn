/*
FxSound
Copyright (C) 2025  FxSound LLC

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

#include <JuceHeader.h>
#include <comdef.h>
#include "GUI/FxSystemTrayView.h"
#include "GUI/FxController.h"
#include "GUI/FxTheme.h"
#include "GUI/FxMainWindow.h"
#include "GUI/CynMaterialShell.h"
#include "AudioPassthru.h"
#include <dbghelp.h>
#include <tlhelp32.h>
#include <shobjidl.h>

#pragma comment(lib, "dbghelp.lib")

// Do not use high-performance GPU on laptops with dual graphics adapters
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000000;

    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0;
}

//==============================================================================
class FxSoundApplication : public JUCEApplication
{
    static void engineStartupError(const String& message)
    {
        const auto file=File::getSpecialLocation(File::userApplicationDataDirectory)
            .getChildFile("Cyn").getChildFile("engine-startup-error.txt");
        file.getParentDirectory().createDirectory();
        file.replaceWithText(message);
    }
public:
    //==============================================================================
    FxSoundApplication() { }

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    //==============================================================================
    void initialise (const String& commandline) override
    {
        // Existing Cyn.exe taskbar pins and Run entries now open CYN Material.
        // All explicit CLI commands still reach the native audio engine.
        // A normal CYN launch starts the real DSP host minimized, then opens
        // the Material UI. Do not quit and respawn the native engine: that
        // races Electron's connection and can leave system audio offline.
        const bool launchMaterial = CynMaterialShell::isUiLaunchRequest(commandline);
        // Riff is CYN's only normal desktop UI. This JUCE process is a native
        // FxSound DSP host and must not construct the legacy window in that mode.
        const bool classicUi = commandline.trim().equalsIgnoreCase("--classic");
        // This method is where you should put your application's initialisation code..
        try
        {
            // Cyn has its own IPC identity, but the shared audio driver must have one host.
            HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
            if(snapshot==INVALID_HANDLE_VALUE) {
                engineStartupError("CYN could not inspect the Windows audio host. Close any existing FxSound/CYN instance and launch CYN again.");
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,"Cyn","Unable to check the active audio host. Restart Cyn after closing FxSound.");
                quit(); return;
            }
            PROCESSENTRY32W process{}; process.dwSize=sizeof(process);
            bool otherHost=false;
            if(Process32FirstW(snapshot,&process)) do {
                if(process.th32ProcessID!=GetCurrentProcessId() &&
                   (_wcsicmp(process.szExeFile,L"FxSound.exe")==0 || _wcsicmp(process.szExeFile,L"Cyn.exe")==0)) { otherHost=true; break; }
            } while(Process32NextW(snapshot,&process));
            CloseHandle(snapshot);
            if(otherHost) {
                engineStartupError("Another FxSound or CYN audio host is running. Exit it from its Windows tray menu before connecting the native engine.");
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,"Cyn","Close the other FxSound or Cyn audio app from its tray menu, then open Cyn again. Only one host can process the shared audio driver.");
                quit(); return;
            }
            // Keep the upstream single-host lock while using Cyn's own IPC channel.
            // This also prevents stock FxSound starting later against our driver.
            if(!legacy_audio_lock_.enter(0)) {
                engineStartupError("The FxSound audio engine lock is owned by another process. Exit the original FxSound/CYN application from its system tray, then reconnect.");
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,"Cyn","Another FxSound audio host is active. Exit it from its tray menu, then open Cyn.");
                quit(); return;
            }
            SetCurrentProcessExplicitAppUserModelID(L"Cyn.PersonalAudio");
            SetUnhandledExceptionFilter(unhandledExceptionFilter);

            HRESULT hRes = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (SUCCEEDED(hRes))
            {
                com_initialized_=true;
                CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
            }
            
            LookAndFeel::setDefaultLookAndFeel(&theme_);

            setWorkingDirectory();

            FxController::getInstance().initConfig(
                launchMaterial ? String("--run_minimized") : commandline);

            audio_passthru_ = std::make_unique<AudioPassthru>();
            if (classicUi)
                main_window_ = std::make_unique<FxMainWindow>();
            system_tray_view_.reset(new FxSystemTrayView());

            FxController::getInstance().init(main_window_.get(), system_tray_view_.get(), audio_passthru_.get());
            if (launchMaterial && !CynMaterialShell::launch())
                AlertWindow::showMessageBox(AlertWindow::WarningIcon, "CYN",
                    "The Material interface is missing or could not open. Reinstall the complete CYN update.");
        }
        catch (const std::exception& e)
        {
            auto& controller = FxController::getInstance();
            controller.logMessage(String::formatted("std::exception: %s\n", e.what()));

            CaptureAndLogCallStack();

            quit();
        }
        catch (...)
        {
            auto& controller = FxController::getInstance();
            controller.logMessage("Unknown exception\n");

            CaptureAndLogCallStack();

            quit();
        }
    }

    void shutdown() override
    {
        // Headless FxSound hosts still own and must release the audio driver,
        // timers, tray and preset state. The legacy GUI is optional.
        if (audio_passthru_ != nullptr)
        {
            FxController::getInstance().autoSaveModifiedPreset();
            FxController::getInstance().stopTimer();

            audio_passthru_.reset();

            // UIA holds COM references to AccessibilityNativeHandle objects for
            // desktop-resident components. Call UiaDisconnectAllProviders to
            // synchronously release those references before the components are
            // destroyed, so their COM refcounts reach zero in the destructor
            // chain rather than after static destructors have already run.
            if (HMODULE hUia = GetModuleHandleW(L"UIAutomationCore.dll"))
            {
                typedef HRESULT (WINAPI* UiaDisconnectAllProvidersFunc)();
                if (auto fn = (UiaDisconnectAllProvidersFunc)GetProcAddress(hUia, "UiaDisconnectAllProviders"))
                    fn();
            }

            system_tray_view_.reset();

            main_window_.reset(); // (deletes our window)
        }

        LocalisedStrings::setCurrentMappings(nullptr);
        LookAndFeel::setDefaultLookAndFeel(nullptr);

        legacy_audio_lock_.exit();
        if(com_initialized_) { CoUninitialize(); com_initialized_=false; }
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        
        quit();
    }

    void anotherInstanceStarted (const String& commandline) override
    {
        if (CynMaterialShell::isUiLaunchRequest(commandline))
        {
            if (!CynMaterialShell::launch())
                AlertWindow::showMessageBox(AlertWindow::WarningIcon, "CYN",
                    "The Material interface could not open. Repair the CYN installation.");
            return;
        }
        FxController::getInstance().applyConfig(commandline);
    }

private:
    bool com_initialized_=false;
    FxTheme theme_;
    InterProcessLock legacy_audio_lock_{"juceAppLock_FxSound"};
    static constexpr int MAX_FRAMES = 64;

    static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exception_info)
    {
        auto& controller = FxController::getInstance();
     
        auto logPath = File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Cyn");
        if (!logPath.exists()) {
            logPath.createDirectory();
        }

        juce::File dumpFile = logPath.getChildFile("cyn.dmp");
        HANDLE hFile = CreateFileW(dumpFile.getFullPathName().toWideCharPointer(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL, nullptr);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
            dumpInfo.ThreadId = GetCurrentThreadId();
            dumpInfo.ExceptionPointers = exception_info;
            dumpInfo.ClientPointers = FALSE;

            MiniDumpWriteDump(GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                MiniDumpNormal,
                &dumpInfo,
                nullptr,
                nullptr);

            CloseHandle(hFile);
        }

        String message = String::formatted(
            "Unhandled exception\nException code: 0x%X\nException flags: 0x%X\nException address: 0x%p\n",
            exception_info->ExceptionRecord->ExceptionCode,
            exception_info->ExceptionRecord->ExceptionFlags,
            exception_info->ExceptionRecord->ExceptionAddress
        );

        controller.logMessage("\n" + message);

        // Capture stack based on crash context
        CaptureAndLogCallStack(exception_info->ContextRecord);

        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void CaptureAndLogCallStack(CONTEXT* context = nullptr)
    {
        auto& controller = FxController::getInstance();
        String stacktrace_info;

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        SymInitialize(process, NULL, TRUE);

        if (context == nullptr)
        {
            // No crash context - capture live callstack
            void* frames[MAX_FRAMES];
            USHORT capturedFrames = CaptureStackBackTrace(0, MAX_FRAMES, frames, NULL);

            char buffer[sizeof(SYMBOL_INFO) + 256] = { 0 };
            SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
            symbol->MaxNameLen = 255;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

            for (USHORT i = 0; i < capturedFrames; i++)
            {
                if (SymFromAddr(process, (DWORD64)frames[i], 0, symbol))
                {
                    stacktrace_info += String::formatted("%S at 0x%llX\n", symbol->Name, symbol->Address);

                    DWORD displacement;
                    IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
                    if (SymGetLineFromAddr64(process, (DWORD64)frames[i], &displacement, &line))
                    {
                        char* file_name = std::strrchr(line.FileName, '\\');
                        file_name = file_name ? (file_name + 1) : line.FileName;
                        stacktrace_info += String::formatted("    File: %S, Line: %d\n", file_name, line.LineNumber);
                    }
                }
                else
                {
                    stacktrace_info += String::formatted("Unknown function at 0x%p\n", frames[i]);
                }
            }
        }
        else
        {
            // Crash context - walk using StackWalk64
            STACKFRAME64 stackFrame = { 0 };
#if defined(_M_IX86)
            DWORD machineType = IMAGE_FILE_MACHINE_I386;
            stackFrame.AddrPC.Offset = context->Eip;
            stackFrame.AddrPC.Mode = AddrModeFlat;
            stackFrame.AddrFrame.Offset = context->Ebp;
            stackFrame.AddrFrame.Mode = AddrModeFlat;
            stackFrame.AddrStack.Offset = context->Esp;
            stackFrame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_X64)
            DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
            stackFrame.AddrPC.Offset = context->Rip;
            stackFrame.AddrPC.Mode = AddrModeFlat;
            stackFrame.AddrFrame.Offset = context->Rsp;
            stackFrame.AddrFrame.Mode = AddrModeFlat;
            stackFrame.AddrStack.Offset = context->Rsp;
            stackFrame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_ARM64)
            DWORD machineType = IMAGE_FILE_MACHINE_ARM64;
            stackFrame.AddrPC.Offset = context->Pc;
            stackFrame.AddrPC.Mode = AddrModeFlat;
            stackFrame.AddrFrame.Offset = context->Fp;
            stackFrame.AddrFrame.Mode = AddrModeFlat;
            stackFrame.AddrStack.Offset = context->Sp;
            stackFrame.AddrStack.Mode = AddrModeFlat;
#else
#error Unsupported platform
#endif

            char buffer[sizeof(SYMBOL_INFO) + 256] = { 0 };
            SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
            symbol->MaxNameLen = 255;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

            for (int frame = 0; frame < MAX_FRAMES; ++frame)
            {
                if (!StackWalk64(machineType, process, thread, &stackFrame, context, NULL,
                    SymFunctionTableAccess64, SymGetModuleBase64, NULL))
                {
                    break;
                }

                if (stackFrame.AddrPC.Offset == 0)
                    break;

                DWORD64 address = stackFrame.AddrPC.Offset;

                if (SymFromAddr(process, address, 0, symbol))
                {
                    stacktrace_info += String::formatted("%S at 0x%llX\n", symbol->Name, symbol->Address);

                    DWORD displacement;
                    IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
                    if (SymGetLineFromAddr64(process, address, &displacement, &line))
                    {
                        char* file_name = std::strrchr(line.FileName, '\\');
                        file_name = file_name ? (file_name + 1) : line.FileName;
                        stacktrace_info += String::formatted("    File: %S, Line: %d\n", file_name, line.LineNumber);
                    }
                }
                else
                {
                    stacktrace_info += String::formatted("Unknown function at 0x%llX\n", address);
                }
            }
        }

        SymCleanup(process);

        controller.logMessage(stacktrace_info);
    }

    void setWorkingDirectory()
    {
        wchar_t module_file[MAX_PATH];
        DWORD length = GetModuleFileName(NULL, module_file, MAX_PATH);
        if (length > 0)
        {
            std::wstring path(module_file);
            size_t pos = path.find_last_of(L"\\/");
            if (pos != std::string::npos)
            {
                SetCurrentDirectory(path.substr(0, pos).c_str());
            }
        }
    };

    std::unique_ptr<FxMainWindow> main_window_;
    
    std::unique_ptr<FxSystemTrayView> system_tray_view_;
    std::unique_ptr<AudioPassthru> audio_passthru_;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (FxSoundApplication)
