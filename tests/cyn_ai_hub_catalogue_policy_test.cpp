#include "../fxsound/Source/GUI/CynAiHubCataloguePolicy.h"
#include <cassert>
#include <iostream>

int main()
{
    using cyn::isTrustedResearchReleaseUrl;
    assert(isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0"));
    assert(isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/cyn-apex6m-flowsense-research-20261017"));
    assert(isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/community-model-2_3"));
    assert(!isTrustedResearchReleaseUrl(
        "https://github.com/other/repo/releases/tag/apex-6m-v1.0.0"));
    assert(!isTrustedResearchReleaseUrl(
        "http://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0"));
    assert(!isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/"));
    assert(!isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0?download=1"));
    assert(!isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0#hack"));
    assert(!isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/../issues/1"));
    assert(!isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0/extra"));
    assert(!isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0%2Fextra"));
    assert(!isTrustedResearchReleaseUrl(
        "https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0\n"));
    std::cout << "PASS: APEX 6M and other reviewed model releases accepted, hostile links rejected\n";
}
