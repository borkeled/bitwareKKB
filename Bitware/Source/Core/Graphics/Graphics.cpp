#include <dwmapi.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <d3d11.h>
#include <wincodec.h>
#include <vector>

#include "Graphics.h"
#include <Globals.hxx>

#include <imgui/misc/imgui_freetype.h>
#include "imgui/imgui_internal.h"

#include "Fonts/Tahoma.h"
#include "Fonts/Tahoma_Bold.h"
#include <ImGui/addons/colors/colors.h>
#include <ImGui/addons/imgui_addons.h>
#include <Core/UI/GOOD/settings/variables.h>
#include <Core/UI/GOOD/headers/fonts.h>
#include <Core/UI/GOOD/data/fonts.h>
#include <Core/UI/GOOD/data/uicons.h>
#include <Core/Input/InputHook.h>
#include <Core/UI/IMenuRenderer.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>
#include <Core/Features/Cheats/Visuals/Visuals.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);

LRESULT CALLBACK WndProc(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    if (ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, WParam, LParam))
    {
        return true;
    }

    switch (Msg)
    {
    case WM_SYSCOMMAND:
        if ((WParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
        {
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureKeyboard)
            {
                DWORD vk = (DWORD)WParam;
                if (!InputHook::IsVkBound(vk))
                {
                    HWND target = Globals::RobloxWindow;
                    if (target && IsWindow(target))
                        PostMessageA(target, Msg, WParam, LParam);
                }
            }
        }
        return 0;

    case WM_SYSKEYDOWN:
        if (WParam == VK_F4)
        {
            Api::DestroyWindow(Hwnd);
            return 0;
        }
        break;

    case WM_CHAR:
    {
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard)
        {
            HWND target = Globals::RobloxWindow;
            if (target && IsWindow(target))
                PostMessageA(target, Msg, WParam, LParam);
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(11);
        break;
    case WM_CLOSE:
        return 0;
    }

    return DefWindowProcA(Hwnd, Msg, WParam, LParam);
}

namespace {
    std::vector<unsigned char> s_tahoma_vec;
    std::vector<unsigned char> s_tahoma_bold_vec;
}

Graphics::Graphics()
{
    Detail = std::make_unique<detail_t>();
}

Graphics::~Graphics()
{
    Destroy_Imgui();
    Destroy_Window();
    Destroy_Device();
}

void Graphics::SetMenuRenderer(std::unique_ptr<IMenuRenderer> renderer)
{
    m_MenuRenderer = std::move(renderer);
}

bool Graphics::Create_Window()
{
    Detail->WindowClass.cbSize = sizeof(Detail->WindowClass);
    Detail->WindowClass.style = CS_CLASSDC;
    static std::string szClass(ThreadObf::GenerateWindowClass());
    Detail->WindowClass.lpszClassName = szClass.c_str();
    Detail->WindowClass.hInstance = GetModuleHandleA(0);
    Detail->WindowClass.lpfnWndProc = WndProc;

    RegisterClassExA(&Detail->WindowClass);

    int ScreenWidth = Api::GetSystemMetrics(SM_CXSCREEN);
    int ScreenHeight = Api::GetSystemMetrics(SM_CYSCREEN);
    RECT TargetRect = { 0, 0, ScreenWidth, ScreenHeight };
    HWND TargetWindow = Globals::RobloxWindow;
    if (TargetWindow && Api::IsWindow(TargetWindow))
    {
        RECT WindowRect = { 0, 0, 0, 0 };
        Api::GetWindowRect(TargetWindow, &WindowRect);
        int WinWidth = WindowRect.right - WindowRect.left;
        int WinHeight = WindowRect.bottom - WindowRect.top;

        if (WinWidth >= ScreenWidth && WinHeight >= ScreenHeight)
        {
            TargetRect = { 0, 0, ScreenWidth, ScreenHeight };
        }
        else
        {
            RECT ClientRect = { 0, 0, 0, 0 };
            POINT ClientOrigin = { 0, 0 };
            Api::GetClientRect(TargetWindow, &ClientRect);
            Api::ClientToScreen(TargetWindow, &ClientOrigin);
            TargetRect.left   = ClientOrigin.x;
            TargetRect.top    = ClientOrigin.y;
            TargetRect.right  = ClientOrigin.x + ClientRect.right;
            TargetRect.bottom = ClientOrigin.y + ClientRect.bottom;
        }
    }

    static std::string szTitle(ThreadObf::RandomString(6 + rand() % 10));
    Detail->Window = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        Detail->WindowClass.lpszClassName,
        szTitle.c_str(),
        WS_POPUP,
        TargetRect.left, TargetRect.top,
        TargetRect.right - TargetRect.left,
        TargetRect.bottom - TargetRect.top,
        0, 0, Detail->WindowClass.hInstance, 0);

    if (!Detail->Window)
    {
        return false;
    }

    Api::SetLayeredWindowAttributes(Detail->Window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    MARGINS Margins{ -1, -1, -1, -1 };
    Api::DwmExtendFrameIntoClientArea(Detail->Window, &Margins);

    Api::ShowWindow(Detail->Window, SW_SHOW);
    Api::UpdateWindow(Detail->Window);

    return true;
}

bool Graphics::Create_Device()
{
    DXGI_SWAP_CHAIN_DESC SwapChainDesc{};

    SwapChainDesc.BufferCount = 3;
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

    SwapChainDesc.BufferDesc.Width = 0;
    SwapChainDesc.BufferDesc.Height = 0;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    SwapChainDesc.OutputWindow = Detail->Window;

    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    SwapChainDesc.Windowed = 1;

    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;

    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    D3D_FEATURE_LEVEL FeatureLevel;
    D3D_FEATURE_LEVEL FeatureLevelList[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT Result = Api::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, FeatureLevelList, 2, D3D11_SDK_VERSION, &SwapChainDesc, &Detail->SwapChain, &Detail->Device, &FeatureLevel, &Detail->DeviceContext);

    if (FAILED(Result))
    {
        SwapChainDesc.BufferCount = 2;
        Result = Api::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, FeatureLevelList, 2, D3D11_SDK_VERSION, &SwapChainDesc, &Detail->SwapChain, &Detail->Device, &FeatureLevel, &Detail->DeviceContext);
    }

    if (Result == DXGI_ERROR_UNSUPPORTED)
    {
        Result = Api::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, FeatureLevelList, 2, D3D11_SDK_VERSION, &SwapChainDesc, &Detail->SwapChain, &Detail->Device, &FeatureLevel, &Detail->DeviceContext);
    }

    if (Result != S_OK)
    {
        Api::MessageBoxA(nullptr, WRAPPER_MARCO("This software can not run on your computer."), WRAPPER_MARCO("Critical Problem"), MB_ICONERROR | MB_OK);
    }

    ID3D11Texture2D* BackBuffer{ nullptr };
    Detail->SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));

    if (BackBuffer)
    {
        Detail->Device->CreateRenderTargetView(BackBuffer, nullptr, &Detail->GraphicsTargetView);
        BackBuffer->Release();

        return true;
    }

    return false;
}

bool Graphics::Create_Imgui()
{
    using namespace ImGui;
    CreateContext();
    StyleColorsDark();

    float MainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    ImGuiIO& IO = ImGui::GetIO(); (void)IO;

    IO.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    IO.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
    IO.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;

    var->gui.dpi = MainScale;
    var->gui.stored_dpi = static_cast<int>(MainScale * 100.0f);
    var->gui.dpi_changed = true;

    if (!ImGui_ImplWin32_Init(Detail->Window))
    {
        return false;
    }

    if (!Detail->Device || !Detail->DeviceContext)
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(Detail->Device, Detail->DeviceContext))
    {
        return false;
    }

    {
        s_tahoma_vec.assign(Tahoma, Tahoma + sizeof(Tahoma));
        s_tahoma_bold_vec.assign(Tahoma_Bold, Tahoma_Bold + sizeof(Tahoma_Bold));

        font->get(s_tahoma_vec, 13.0f);
        font->get(s_tahoma_bold_vec, 13.0f);
        font->get(inter_semibold, 14.0f);
        font->get(inter_medium, 11.0f);
        font->get(inter_semibold, 10.0f);
        font->get(inter_semibold, 11.0f);
        font->get(inter_semibold, 12.0f);
        font->get(icon_font, 13.0f);
        font->get(icon_font, 14.0f);
        font->get(icon_font, 15.0f);
        font->get(inter_semibold, 13.0f);
        font->get(inter_semibold, 16.0f);
        font->get(inter_semibold, 18.0f);
        font->get(inter_medium, 10.0f);
        font->get(uicons_regular_rounded, 12.5f, true);
        font->get(uicons_regular_rounded, 12.0f, true);

        font->update();

        Tahoma_BoldXP = font->get(s_tahoma_bold_vec, 13.0f);
        Inter_SemiBold = font->get(inter_semibold, 14.0f);
        Inter_Medium = font->get(inter_medium, 11.0f);
    }

    return true;
}

void Graphics::Destroy_Device()
{
    if (Detail->GraphicsTargetView) Detail->GraphicsTargetView->Release();
    if (Detail->SwapChain) Detail->SwapChain->Release();
    if (Detail->DeviceContext) Detail->DeviceContext->Release();
    if (Detail->Device) Detail->Device->Release();
}

void Graphics::Destroy_Window()
{
    Api::DestroyWindow(Detail->Window);
    Api::UnregisterClassA(Detail->WindowClass.lpszClassName, Detail->WindowClass.hInstance);
}

void Graphics::Destroy_Imgui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Graphics::NewFrame()
{
    {
        static DWORD LastAffinity = (DWORD)-1;
        DWORD NewAffinity = Globals::Settings::Streamproof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
        if (NewAffinity != LastAffinity) {
            LastAffinity = NewAffinity;
            Api::SetWindowDisplayAffinity(Detail->Window, NewAffinity);
        }
    }

    HWND TargetWindow = Globals::RobloxWindow;
    if (TargetWindow && Api::IsWindow(TargetWindow) && Detail->Window)
    {
        static RECT LastOverlayRect = { 0, 0, 0, 0 };

        int ScreenWidth = Api::GetSystemMetrics(SM_CXSCREEN);
        int ScreenHeight = Api::GetSystemMetrics(SM_CYSCREEN);
        RECT OverlayRect = { 0, 0, ScreenWidth, ScreenHeight };

        RECT WindowRect = { 0, 0, 0, 0 };
        Api::GetWindowRect(TargetWindow, &WindowRect);
        int WinWidth = WindowRect.right - WindowRect.left;
        int WinHeight = WindowRect.bottom - WindowRect.top;

        if (WinWidth >= ScreenWidth && WinHeight >= ScreenHeight)
        {
            OverlayRect = { 0, 0, ScreenWidth, ScreenHeight };
        }
        else
        {
            RECT ClientRect = { 0, 0, 0, 0 };
            POINT ClientOrigin = { 0, 0 };
            Api::GetClientRect(TargetWindow, &ClientRect);
            Api::ClientToScreen(TargetWindow, &ClientOrigin);
            OverlayRect.left   = ClientOrigin.x;
            OverlayRect.top    = ClientOrigin.y;
            OverlayRect.right  = ClientOrigin.x + ClientRect.right;
            OverlayRect.bottom = ClientOrigin.y + ClientRect.bottom;
        }

        if (OverlayRect.left   != LastOverlayRect.left   ||
            OverlayRect.top    != LastOverlayRect.top    ||
            OverlayRect.right  != LastOverlayRect.right  ||
            OverlayRect.bottom != LastOverlayRect.bottom)
        {
            LastOverlayRect = OverlayRect;
            Api::SetWindowPos(Detail->Window, HWND_TOPMOST,
                OverlayRect.left, OverlayRect.top,
                OverlayRect.right - OverlayRect.left,
                OverlayRect.bottom - OverlayRect.top,
                SWP_SHOWWINDOW | SWP_NOACTIVATE);

            if (Detail->SwapChain)
            {
                if (Detail->GraphicsTargetView)
                {
                    Detail->GraphicsTargetView->Release();
                    Detail->GraphicsTargetView = nullptr;
                }

                UINT NewWidth  = OverlayRect.right - OverlayRect.left;
                UINT NewHeight = OverlayRect.bottom - OverlayRect.top;
                Detail->SwapChain->ResizeBuffers(0, NewWidth, NewHeight, DXGI_FORMAT_UNKNOWN, 0);

                ID3D11Texture2D* BackBuffer = nullptr;
                if (SUCCEEDED(Detail->SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer))))
                {
                    Detail->Device->CreateRenderTargetView(BackBuffer, nullptr, &Detail->GraphicsTargetView);
                    BackBuffer->Release();
                }
            }
        }
    }

    if (var->gui.dpi_changed)
    {
        font->update();
        Tahoma_BoldXP = font->get(s_tahoma_bold_vec, 13.0f);
        Inter_SemiBold = font->get(inter_semibold, 14.0f);
        Inter_Medium = font->get(inter_medium, 11.0f);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (InputHook::ConsumeMenuKeyPress())
    {
        var->gui.menu_open_target = !var->gui.menu_open_target;

        if (var->gui.menu_open_target)
        {
            Running = true;
            InputHook::SetMenuOpen(true);
            SetWindowLong(Detail->Window, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST);
            Api::SetLayeredWindowAttributes(Detail->Window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);
            MARGINS Margins{ -1, -1, -1, -1 };
            Api::DwmExtendFrameIntoClientArea(Detail->Window, &Margins);
            SetForegroundWindow(Detail->Window);
            SetFocus(Detail->Window);
        }
        else
        {
            InputHook::SetMenuOpen(false);
        }
    }

    // Close animation complete: tear down window styles and disable render
    if (!var->gui.menu_open_target && var->gui.menu_open_alpha < 0.01f && Running)
    {
        Running = false;
        SetWindowLong(Detail->Window, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT);
        Api::SetLayeredWindowAttributes(Detail->Window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);
        MARGINS Margins{ -1, -1, -1, -1 };
        Api::DwmExtendFrameIntoClientArea(Detail->Window, &Margins);
        HWND target = Globals::RobloxWindow;
        if (target && IsWindow(target))
            SetForegroundWindow(target);
    }
}

void Graphics::Render()
{
    if (Perf::ShowOverlay)
    {
        static double lastFpsUpdate = 0.0;
        static int frameCount = 0;
        double now = ImGui::GetTime();
        frameCount++;
        if (now - lastFpsUpdate >= 1.0)
        {
            Perf::LastFPS.store(frameCount);
            frameCount = 0;
            lastFpsUpdate = now;
        }

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.6f);
        if (ImGui::Begin("Perf", &Perf::ShowOverlay, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("FPS: %d", Perf::LastFPS.load());
            ImGui::Separator();
            ImGui::Text("Frame:  %4llu us", (unsigned long long)Perf::FrameTimeUs.load());
            ImGui::Text("Visuals: %4llu us", (unsigned long long)Perf::VisualsTimeUs.load());
            ImGui::Text("Present: %4llu us", (unsigned long long)Perf::PresentTimeUs.load());
            ImGui::Text("Aimbot:  %4llu us", (unsigned long long)Perf::AimbotTimeUs.load());
            ImGui::Text("Trigger: %4llu us", (unsigned long long)Perf::TriggerbotTimeUs.load());
            ImGui::Text("Cache:   %4llu us", (unsigned long long)Perf::CacheTimeUs.load());
        }
        ImGui::End();
    }

    if (Running && m_MenuRenderer)
    {
        m_MenuRenderer->Render();
    }
}

void Graphics::Present()
{
    Globals::FrameCount.fetch_add(1, std::memory_order_relaxed);

    auto presentStart = std::chrono::steady_clock::now();

    ImGui::Render();

    float ClearColor[4]{ 0, 0, 0, 0 };
    Detail->DeviceContext->OMSetRenderTargets(1, &Detail->GraphicsTargetView, nullptr);
    Detail->DeviceContext->ClearRenderTargetView(Detail->GraphicsTargetView, ClearColor);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    Visuals::FlushMeshChams(Detail->Device, Detail->DeviceContext, Detail->GraphicsTargetView);

    static auto LastPresentTime = std::chrono::steady_clock::now();

    int TargetFPS = SettingsStore::PerfMode_FPS[0];
    if (Detail->Window && IsIconic(Detail->Window))
    {
        TargetFPS = 10;
    }
    else
    {
        int Mode = Globals::Settings::Performance_Mode;
        if (Mode < 0) Mode = 0;
        if (Mode > 2) Mode = 2;
        TargetFPS = SettingsStore::PerfMode_FPS[Mode];
    }

    auto TargetInterval = std::chrono::microseconds(1000000 / TargetFPS);

    auto Now = std::chrono::steady_clock::now();
    auto Elapsed = std::chrono::duration_cast<std::chrono::microseconds>(Now - LastPresentTime);
    if (Elapsed < TargetInterval) {
        std::this_thread::sleep_for(TargetInterval - Elapsed);
    }
    LastPresentTime = std::chrono::steady_clock::now();

    int SyncInterval = Globals::Settings::VSync ? 1 : 0;
    Detail->SwapChain->Present(SyncInterval, 0);

    auto presentEnd = std::chrono::steady_clock::now();
    Perf::PresentTimeUs.store(std::chrono::duration_cast<std::chrono::microseconds>(presentEnd - presentStart).count());
}

void Graphics::Start_Render()
{
    NewFrame();
}

void Graphics::End_Render()
{
    Present();
}
