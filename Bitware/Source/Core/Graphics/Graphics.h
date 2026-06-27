#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <memory>
#include <d3d11.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx11.h>

#include <string>
#include <vector>
#include <Infrastructure/ThreadObf.h>

class IMenuRenderer;

inline ImFont* Tahoma_BoldXP = nullptr;
inline ImFont* Inter_SemiBold = nullptr;
inline ImFont* Inter_Medium = nullptr;
inline ImFont* Icon_Font = nullptr;



struct detail_t {
	HWND Window = nullptr;
	WNDCLASSEX WindowClass = {};
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	ID3D11RenderTargetView* GraphicsTargetView = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
};

class Graphics {
public:
    Graphics();
    ~Graphics();

    bool Running = false;

    void NewFrame();
    void Render();
    void Present();

    void Start_Render();
    void End_Render();

    bool Create_Device();
    bool Create_Window();
    bool Create_Imgui();

    void SetMenuRenderer(std::unique_ptr<IMenuRenderer> renderer);
    IMenuRenderer* GetMenuRenderer() const { return m_MenuRenderer.get(); }

    std::unique_ptr<detail_t> Detail = std::make_unique<detail_t>();

private:
    void Destroy_Device();
    void Destroy_Window();
    void Destroy_Imgui();

    std::unique_ptr<IMenuRenderer> m_MenuRenderer;
};

inline std::unique_ptr<Graphics> Graphic = std::make_unique<Graphics>();
