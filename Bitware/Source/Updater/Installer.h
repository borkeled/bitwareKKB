#pragma once
#include <string>

namespace Installer {

    bool EnsureInstalled();
    void CreateDesktopShortcut();
    std::wstring GetInstallDir();

}
