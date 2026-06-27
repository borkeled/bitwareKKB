#pragma once
#include <string>
#include <atomic>
#include <thread>

namespace SplashWindow {

    enum class Status {
        Idle,
        Checking,
        Downloading,
        Installing,
        Waiting,
        Launching,
        Ready,
        Error
    };

    bool Create();
    void SetStatus(Status status, const char* stage, const char* message);
    void SetProgress(int percent);
    void SetDownloadProgress(int64_t downloaded, int64_t total);
    void Close();
    bool IsRunning();

}
