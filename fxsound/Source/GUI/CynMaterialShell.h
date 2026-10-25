#pragma once
// CYN Windows Material shell handoff.
// The native Cyn.exe is the audio engine; this is only a UI launcher.
// Never replace the native DSP or launch the shell for --status/CLI commands.
#include <JuceHeader.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")

namespace CynMaterialShell
{
    inline juce::File executable()
    {
        return juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getSiblingFile("CYN-Interface")
            .getChildFile("CYN Audio.exe");
    }

    inline bool launch()
    {
        const auto app = executable();
        if (!app.existsAsFile())
            return false;

        const auto exePath = app.getFullPathName();
        const auto workingDir = app.getParentDirectory().getFullPathName();
        const auto result = ::ShellExecuteW(nullptr, L"open",
            exePath.toWideCharPointer(), nullptr,
            workingDir.toWideCharPointer(), SW_SHOWNORMAL);
        return reinterpret_cast<INT_PTR>(result) > 32;
    }
}
