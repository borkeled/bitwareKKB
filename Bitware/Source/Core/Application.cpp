#include "Application.h"
#include <iostream>
#include <thread>
#include <chrono>

#include <Windows.h>
#include <imgui/imgui.h>

#include <Driver/Driver.h>
#include <Driver/SSN.h>
#include <Globals.hxx>
#include <Miscellaneous/Output/Output.h>
#include <Core/Graphics/Graphics.h>
#include <Engine/Engine.h>
#include <Engine/Offsets/Offsets.h>
#include <Core/Features/Cache/Cache.h>
#include <Core/Features/Cheats/Misc/Misc.h>
#include <Core/Features/Cheats/World/World.h>
#include <Core/Features/Cheats/Aimbot/Aimbot.h>
#include <Core/Features/Cheats/Aimbot/Silent/Silent.h>
#include <Core/Features/Cheats/Triggerbot/Triggerbot.h>
#include <Core/Features/Cheats/Visuals/Visuals.h>
#include <Miscellaneous/Protection/FakeStrings.h>
#include <Miscellaneous/Protection/AntiInjection.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>
#include <Auth/skStr.h>
#include <Core/UI/IMenuRenderer.h>
#include <Core/UI/LegacyMenuRenderer.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>
#include <Infrastructure/AntiDump.h>
#include <Infrastructure/ThreadObf.h>
#include <Infrastructure/ResourceEnc.h>
#include <Infrastructure/MiniVM.h>
#include <Infrastructure/Logger.h>

#include <ShlObj.h>
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "winmm.lib")

static void DrawCursor()
{
    OBF_PROLOGUE;
    OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

    if (!SilentAimInstance.Address) {
        OBF_JUNK_BLOCK;
        return;
    }
    OBF_JUNK_DECLARE;

    static bool is_visible = false;
    static DWORD last_read = 0;
    DWORD now = GetTickCount();
    if (now - last_read > 100)
    {
        is_visible = Driver->Read<bool>(SilentAimInstance.Address + Offsets::GuiObject::Visible);
        last_read = now;
        OBF_JUNK_BLOCK;
    }

    OBF_OPAQUE_FALSE {
        OBF_JUNK_BLOCK;
        // unreachable
        is_visible = !is_visible;
    }

    if (!is_visible)
    {
        OBF_JUNK_BLOCK;
        return;
    }

    POINT pt;
    if (!GetCursorPos(&pt)) {
        OBF_JUNK_BLOCK;
        return;
    }
    OBF_JUNK_DECLARE;

    bool right_click_held = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
    OBF_OPAQUE_TRUE {
        volatile int x = 0;
        if (x) right_click_held = !right_click_held;
    }
    float gap = right_click_held ? 4.0f : 10.0f;
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    ImU32 col = IM_COL32(255, 255, 255, 255);
    float dot_size = 4.0f;
    float line_width = 2.0f;
    float line_length = 10.0f;
    ImVec2 center = { (float)pt.x, (float)pt.y };
    ImVec2 dot_min(center.x - dot_size * 0.5f, center.y - dot_size * 0.5f);
    ImVec2 dot_max(center.x + dot_size * 0.5f, center.y + dot_size * 0.5f);
    draw->AddRectFilled(dot_min, dot_max, col, 0.0f);
    ImVec2 top_min(center.x - line_width * 0.5f, center.y - gap - line_length);
    ImVec2 top_max(center.x + line_width * 0.5f, center.y - gap);
    draw->AddRectFilled(top_min, top_max, col, 0.0f);
    ImVec2 bottom_min(center.x - line_width * 0.5f, center.y + gap);
    ImVec2 bottom_max(center.x + line_width * 0.5f, center.y + gap + line_length);
    draw->AddRectFilled(bottom_min, bottom_max, col, 0.0f);
    ImVec2 left_min(center.x - gap - line_length, center.y - line_width * 0.5f);
    ImVec2 left_max(center.x - gap, center.y + line_width * 0.5f);
    draw->AddRectFilled(left_min, left_max, col, 0.0f);
    ImVec2 right_min(center.x + gap, center.y - line_width * 0.5f);
    ImVec2 right_max(center.x + gap + line_length, center.y + line_width * 0.5f);
    draw->AddRectFilled(right_min, right_max, col, 0.0f);
}

Application::Application()
{
}

Application::~Application()
{
    Shutdown();
}

void Application::SetMenuRenderer(std::unique_ptr<IMenuRenderer> renderer)
{
    m_MenuRenderer = std::move(renderer);
}

bool Application::InitProcess()
{
    OBF_PROLOGUE;

    Driver->Find_Process(std::string(skCrypt("RobloxPlayerBeta.exe")));
    Driver->Attach_Process(std::string(skCrypt("RobloxPlayerBeta.exe")));
    Driver->Find_Module(std::string(skCrypt("RobloxPlayerBeta.exe")));

    if (!Driver->Get_Handle())
    {
        Output::Error(WRAPPER_MARCO("Failed to attach to process"));
        return false;
    }

    OBF_OPAQUE_TRUE {
        volatile int x = 0;
        Output::Warning(WRAPPER_MARCO("init ok"));
    }

    return true;
}

bool Application::InitSDK()
{
    OBF_PROLOGUE;
    auto ModuleBase = Driver->Get_Module();
    if (!ModuleBase) {
        OBF_JUNK_BLOCK;
        return false;
    }

    OBF_OPAQUE_FALSE {
        return true;
    }

    auto FakeDataModel = Driver->Read<std::uint64_t>(ModuleBase + Offsets::FakeDataModel::Pointer);
    if (!FakeDataModel) {
        OBF_JUNK_BLOCK;
        return false;
    }

    Globals::Datamodel.Address = Driver->Read<std::uint64_t>(FakeDataModel + Offsets::FakeDataModel::RealDataModel);
    if (!Globals::Datamodel.Address) {
        OBF_JUNK_BLOCK;
        OBF_JUNK_DECLARE;
        return false;
    }

    Globals::VisualEngine.Address = Driver->Read<std::uint64_t>(ModuleBase + Offsets::VisualEngine::Pointer);
    Globals::Players.Address = Globals::Datamodel.Find_First_Child_Of_Class(std::string(skCrypt("Players")).c_str()).Address;
    Globals::Workspace.Address = Globals::Datamodel.Find_First_Child_Of_Class(std::string(skCrypt("Workspace")).c_str()).Address;
    Globals::Camera.Address = Globals::Workspace.Find_First_Child_Of_Class(std::string(skCrypt("Camera")).c_str()).Address;

    auto Lightin = Globals::Datamodel.Find_First_Child_Of_Class(std::string(skCrypt("Lighting")).c_str());
    Globals::Lighting = SDK::Lighting(Lightin.Address);

    {
        DWORD target_pid = Driver->Get_Process();
        EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == static_cast<DWORD>(lparam) && IsWindowVisible(hwnd)) {
                Globals::RobloxWindow = hwnd;
                return FALSE;
            }
            return TRUE;
        }, static_cast<LPARAM>(target_pid));
    }

    if (Globals::Workspace.Address)
    {
        auto workspacetoworld = Driver->Read<uintptr_t>(Globals::Workspace.Address + Offsets::Workspace::TerrainPhysics);
        if (workspacetoworld)
        {
            Driver->Write<float>(workspacetoworld + Offsets::World::SkyDistance, 200 * 4.f);
        }
    }

    return true;
}

void Application::SpawnThreads()
{
    m_WorkerThreads.emplace_back(Cache::RunService);
    m_WorkerThreads.emplace_back(World::RunService);
    m_WorkerThreads.emplace_back(Aimbot::RunService);
    m_WorkerThreads.emplace_back(Silent::RunService);
    m_WorkerThreads.emplace_back(Triggerbot::RunService);
    m_WorkerThreads.emplace_back(Misc::RunService);
}

bool Application::InitOverlay()
{
    Graphic->Create_Window();
    Graphic->Create_Device();
    Graphic->Create_Imgui();

    if (!m_MenuRenderer)
    {
        m_MenuRenderer = std::make_unique<LegacyMenuRenderer>();
    }

    Graphic->SetMenuRenderer(std::move(m_MenuRenderer));

    return true;
}

bool Application::Init()
{
    Logger::Log(WRAPPER_MARCO("[Init] starting"));
    Logger::Flush();

    PEXCEPTION_POINTERS exc = nullptr;
    __try {
        OBF_JUNK_DECLARE;
        timeBeginPeriod(1);

        Logger::Log(WRAPPER_MARCO("[Init] badstrings..."));
        Logger::Flush();
        FakeStringss::Generate();

        Logger::Log(WRAPPER_MARCO("[Init] anti inject..."));
        Logger::Flush();
        AntiInjection::Start();

        Logger::Log(WRAPPER_MARCO("[Init] anti dump..."));
        Logger::Flush();
        AntiDump::Enable();
        OBF_JUNK_BLOCK;

        Logger::Log(WRAPPER_MARCO("[Init] SSN resolver..."));
        Logger::Flush();
        SSN::Resolve();

        Logger::Log(WRAPPER_MARCO("[Init] init proc..."));
        Logger::Flush();
        if (!InitProcess()) {
            Logger::Log(WRAPPER_MARCO("[Init] init proc FAILED"));
            Logger::Flush();
            OBF_JUNK_BLOCK;
            return false;
        }

        Logger::Log(WRAPPER_MARCO("[Init] InitSDK..."));
        Logger::Flush();
        if (!InitSDK()) {
            Logger::Log(WRAPPER_MARCO("[Init] InitSDK FAILED"));
            Logger::Flush();
            OBF_JUNK_BLOCK;
            return false;
        }

        Logger::Log(WRAPPER_MARCO("[Init] InputHook..."));
        Logger::Flush();
        InputHook::Install();

        Logger::Log(WRAPPER_MARCO("[Init] SpawnThreads..."));
        Logger::Flush();
        SpawnThreads();

        Logger::Log(WRAPPER_MARCO("[Init] InitOverlay..."));
        Logger::Flush();
        InitOverlay();

        OBF_JUNK_BLOCK;
        Logger::Log(WRAPPER_MARCO("[Init] doneso"));
        Logger::Flush();
        return true;
    }
    __except (exc = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER) {
        char buf[256];
        snprintf(buf, sizeof(buf), WRAPPER_MARCO("[Init] EXCEPTION: code=0x%08lX addr=0x%llX"),
            exc->ExceptionRecord->ExceptionCode,
            (unsigned long long)exc->ExceptionRecord->ExceptionAddress);
        Logger::Log(buf);
        Logger::Flush();
        return false;
    }
}

void Application::Run()
{
    OBF_PROLOGUE;

    if (!Graphic->Detail->Window)
    {
        Output::Error(WRAPPER_MARCO("Overlay not initialized"));
        return;
    }

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    VM::BytecodeBuilder<512> PanicVM_Builder;
    bool PanicVM_Built = false;
    auto& PanicVM = PanicVM_Builder;
    if (!PanicVM_Built) {
        // push 0; pop R0
        PanicVM.EmitOp(VM::PUSH_IMM); PanicVM.Emit64(0);
        PanicVM.EmitOp(VM::POP_REG); PanicVM.Emit8(VM::R0);

        auto panic_lbl = PanicVM.GetOffset();   // loop begins here
        PanicVM.EmitOp(VM::CUSTOM_CALL);                    // call panic handler
        PanicVM.EmitOp(VM::PUSH_REG); PanicVM.Emit8(VM::R0); // push result
        PanicVM.EmitOp(VM::PUSH_IMM); PanicVM.Emit64(0);
        PanicVM.EmitOp(VM::CMP);                             // compare result != 0

        auto je_patch = PanicVM.GetOffset();
        PanicVM.EmitOp(VM::JE); PanicVM.Emit64(0);         // forward je (exit if 0 → no panic)

        auto jmp_patch = PanicVM.GetOffset();
        PanicVM.EmitOp(VM::JMP); PanicVM.Emit64(0);        // backward jmp to panic_lbl

        auto exit_lbl = PanicVM.GetOffset();
        PanicVM.EmitOp(VM::EXIT);

        // Fix up forward jumps
        PanicVM.Patch64(je_patch, exit_lbl);
        PanicVM.Patch64(jmp_patch, panic_lbl);
        PanicVM_Built = true;
    }

    for (;;)
    {
        static bool panic_triggered = false;
        OBF_JUNK_DECLARE;

        if (!panic_triggered && GetAsyncKeyState(VK_F7) & 0x8000)
        {
            panic_triggered = true;

            // clear sensitive globals before exit
            Globals::Player_Cache.clear();
            Globals::LocalPlayer = SDK::Player{};
            Globals::Datamodel = SDK::Datamodel{};

            DWORD old;
            Api::VirtualProtect(panic_triggered ? (void*)0x1000 : (void*)0x2000, 0x1000, PAGE_NOACCESS, &old);
            ExitProcess(0);
        }

        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            OBF_JUNK_BLOCK;
            if (msg.message == WM_QUIT)
                return;
        }

        InputHook::PollKeys();

        Graphic->NewFrame();
        DrawCursor();

        OBF_JUNK_BLOCK;
        OBF_OPAQUE_TRUE {
            OBF_JUNK_BLOCK;
        }

        Visuals::RunService();

        Graphic->Render();
        Graphic->Present();

        OBF_STALL;
    }
}

void Application::Shutdown()
{
    AntiDump::Disable();
    AntiInjection::Stop();
    InputHook::Remove();

    for (auto& thread : m_WorkerThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    m_WorkerThreads.clear();

    timeEndPeriod(1);
}
