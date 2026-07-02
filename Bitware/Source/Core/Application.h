#pragma once
#include <memory>
#include <vector>
#include <thread>
#include <stop_token>
#include <Core/Input/InputHook.h>

class IMenuRenderer;

extern const char* InitFailureReason;

class Application {
public:
    Application();
    ~Application();

    bool Init();
    void Run();
    void Shutdown();

    void SetMenuRenderer(std::unique_ptr<IMenuRenderer> renderer);

private:
    bool InitProcess();
    bool InitSDK();
    bool InitOverlay();
    void SpawnThreads();

    std::unique_ptr<IMenuRenderer> m_MenuRenderer;
    std::vector<std::jthread> m_WorkerThreads;
    std::stop_source m_StopSource;
};
