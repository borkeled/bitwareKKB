#pragma once
#include <memory>
#include <vector>
#include <thread>
#include <Core/Input/InputHook.h>
#include <Core/UI/BackendSelector.h>

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
    void InitBackend();
    bool InitSeh();

    BackendSelector::Mode m_SelectedBackend;

    std::unique_ptr<IMenuRenderer> m_MenuRenderer;
    std::vector<std::thread> m_WorkerThreads;
};
