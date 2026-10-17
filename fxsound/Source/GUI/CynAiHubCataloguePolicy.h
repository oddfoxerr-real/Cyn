// CYN AI Hub: strict, tested validation of official external model release URLs.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <string_view>

namespace cyn {
inline bool isTrustedResearchReleaseUrl(std::string_view url) noexcept
{
    constexpr std::string_view prefix =
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/";
    if (url.size() > 240 || url.substr(0, prefix.size()) != prefix)
        return false;
    const auto tag = url.substr(prefix.size());
    if (tag.empty() || tag.front() == '.' || tag.find("..") != tag.npos)
        return false;
    for (unsigned char c : tag)
    {
        const bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const bool digit = c >= '0' && c <= '9';
        if (!alpha && !digit && c != '-' && c != '_' && c != '.')
            return false;
    }
    return true;
}
} // namespace cyn
