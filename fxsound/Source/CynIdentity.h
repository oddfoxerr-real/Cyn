/* Cyn host identity. SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <JuceHeader.h>
namespace CynIdentity {
inline File dataDirectory() { return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Cyn"); }
// Import once, without modifying the legacy installation or overwriting Cyn data.
inline bool migrateLegacyData()
{
    const auto data=dataDirectory();
    const auto old=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("FxSound");
    PropertiesFile::Options target; target.applicationName="Cyn"; target.folderName="Cyn"; target.filenameSuffix="settings";
    auto legacy=target; legacy.applicationName="FxSound"; legacy.folderName="FxSound";
    const auto settings=target.getDefaultFile();
    if(settings.existsAsFile()) return false;
    data.createDirectory();
    bool imported=false;
    if(legacy.getDefaultFile().existsAsFile()) imported=legacy.getDefaultFile().copyFileTo(settings);
    for(const auto& folder : {"Presets","AutoSave"}) {
        const auto destination=data.getChildFile(folder);
        if(!destination.exists() && old.getChildFile(folder).isDirectory()) old.getChildFile(folder).copyDirectoryTo(destination);
    }
    const auto learning=data.getChildFile("preferences.json");
    if(!learning.exists() && old.getChildFile("Cyn/preferences.json").existsAsFile()) old.getChildFile("Cyn/preferences.json").copyFileTo(learning);
    return imported;
}
}
