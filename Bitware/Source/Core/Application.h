#pragma once
#include <memory>
#include <vector>
#include <thread>
#include <Core/Input/InputHook.h>

class IMenuRenderer;

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
    std::vector<std::thread> m_WorkerThreads;
};
