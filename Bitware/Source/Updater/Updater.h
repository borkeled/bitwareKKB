#pragma once
#include <string>
#include <cstdint>

namespace Updater {

    struct UpdateInfo {
        bool updateAvailable = false;
        std::string latestVersion;
        std::string downloadUrl;
    };

    using ProgressCallback = void (*)(int64_t downloaded, int64_t total);

    void CleanupAfterUpdate(const wchar_t* installDir);
    UpdateInfo CheckForUpdate();
    bool DownloadAndApply(const UpdateInfo& info, ProgressCallback progress = nullptr);

}
