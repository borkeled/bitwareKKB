#define NOMINMAX
#include <iostream>
#include "visuals.h"
#include <Engine/Engine.h>
#include <cfloat>
#include <cmath>
#include <Clipper2Lib/include/clipper2/clipper.h>
#include <imgui/imgui.h>
#include <globals.hxx>
#include <Windows.h>
#undef min
#undef max
#include <algorithm>
#include <mutex>
#include <ImGui/imgui_internal.h>
#include "../../../Graphics/Graphics.h"
#include "../Common/WallCheck.h"
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>
#include <Core/Input/InputHook.h>

#define IMGUI_DEFINE_MATH_OPERATORS

inline ImU32 FlotationDevice(const float c[4]) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c[0], c[1], c[2], c[3]));
}

namespace Cache {
    extern std::mutex Cache_Mutex;
    extern std::atomic<bool> CacheReady;
}

static void Outline(const ImVec2& Pos, const char* Text, const float Col[4]) {
    if (!Text || !*Text) {
        return;
    }

    ImDrawList* Draw = ImGui::GetBackgroundDrawList();
    const ImVec2 Position(std::roundf(Pos.x), std::roundf(Pos.y));
    const ImU32 Col1 = ImGui::ColorConvertFloat4ToU32(ImVec4(Col[0], Col[1], Col[2], Col[3]));
    const ImU32 Col2 = IM_COL32(0, 0, 0, 255);

    static constexpr ImVec2 Offsets[8] = {
        {-1.f,  0.f}, {1.f,  0.f},
        { 0.f, -1.f}, {0.f,  1.f},
        {-1.f, -1.f}, {1.f, -1.f},
        {-1.f,  1.f}, {1.f,  1.f}
    };

    const float Font_Size = ImGui::GetFontSize();
    for (const ImVec2& o : Offsets) {
        Draw->AddText(nullptr, Font_Size, Position + o, Col2, Text);
    }
    Draw->AddText(nullptr, Font_Size, Position, Col1, Text);
}

static const SDK::Vector3 Corners[8] = {
    {-1,-1,-1}, {1,-1,-1}, {-1,1,-1}, {1,1,-1},
    {-1,-1, 1}, {1,-1, 1}, {-1,1, 1}, {1,1, 1}
};

namespace Visuals {
    const std::vector<const SDK::Instance*>& Get_Bones(const SDK::Player& Player) {
        thread_local std::vector<const SDK::Instance*> Parts;
        Parts.clear();

        const bool R15 = Player.UpperTorso.Address && Player.LowerTorso.Address;
        const bool R6 = Player.Torso.Address;

        if (R15) {
            if (Player.Head.Address) Parts.push_back(&Player.Head);
            if (Player.UpperTorso.Address) Parts.push_back(&Player.UpperTorso);
            if (Player.LowerTorso.Address) Parts.push_back(&Player.LowerTorso);
            if (Player.LeftUpperArm.Address) Parts.push_back(&Player.LeftUpperArm);
            if (Player.LeftLowerArm.Address) Parts.push_back(&Player.LeftLowerArm);
            if (Player.LeftHand.Address)     Parts.push_back(&Player.LeftHand);
            if (Player.RightUpperArm.Address) Parts.push_back(&Player.RightUpperArm);
            if (Player.RightLowerArm.Address) Parts.push_back(&Player.RightLowerArm);
            if (Player.RightHand.Address)     Parts.push_back(&Player.RightHand);
            if (Player.LeftUpperLeg.Address) Parts.push_back(&Player.LeftUpperLeg);
            if (Player.LeftLowerLeg.Address) Parts.push_back(&Player.LeftLowerLeg);
            if (Player.LeftFoot.Address)     Parts.push_back(&Player.LeftFoot);
            if (Player.RightUpperLeg.Address) Parts.push_back(&Player.RightUpperLeg);
            if (Player.RightLowerLeg.Address) Parts.push_back(&Player.RightLowerLeg);
            if (Player.RightFoot.Address)     Parts.push_back(&Player.RightFoot);
        }
        else if (R6) {
            if (Player.Head.Address)  Parts.push_back(&Player.Head);
            if (Player.Torso.Address) Parts.push_back(&Player.Torso);
            if (Player.LeftArm.Address)  Parts.push_back(&Player.LeftArm);
            if (Player.RightArm.Address) Parts.push_back(&Player.RightArm);
            if (Player.LeftLeg.Address)  Parts.push_back(&Player.LeftLeg);
            if (Player.RightLeg.Address) Parts.push_back(&Player.RightLeg);
        }
        else {
            for (const auto& Bone : Player.Bones) {
                if (Bone.Address) Parts.push_back(&Bone);
            }
            if (Parts.empty()) {
                if (Player.HumanoidRootPart.Address) Parts.push_back(&Player.HumanoidRootPart);
                if (Player.Head.Address)             Parts.push_back(&Player.Head);
                if (Player.Torso.Address)            Parts.push_back(&Player.Torso);
                if (Player.UpperTorso.Address)       Parts.push_back(&Player.UpperTorso);
                if (Player.LowerTorso.Address)       Parts.push_back(&Player.LowerTorso);
            }
        }
        return Parts;
    }

    static void VisualsToggleHelper()
    {
        static bool Toggled = false;
        static bool LastPressed = false;

        ImGuiKey key = Globals::Visuals::ToggleKey;
        if (key == ImGuiKey_None) {
            return;
        }

        int Vk = ImGuiKeyToVK(key);
        if (!Vk) {
            return;
        }

        bool Pressed = InputHook::IsKeyDown(Vk);

        if (Globals::Visuals::ToggleMode == ImKeyBindMode_Toggle)
        {
            if (Pressed && !LastPressed)
                Toggled = !Toggled;
        }
        else
        {
            Toggled = Pressed;
        }

        LastPressed = Pressed;
        Globals::Visuals::Enabled = Toggled;
    }

    static void AimbotFovToggleHelper()
    {
        static bool Toggled = false;
        static bool LastPressed = false;

        ImGuiKey key = Globals::Aimbot::FovToggleKey;
        if (key == ImGuiKey_None) {
            return;
        }

        int Vk = ImGuiKeyToVK(key);
        if (!Vk) {
            return;
        }

        bool Pressed = InputHook::IsKeyDown(Vk);

        if (Globals::Aimbot::FovToggleMode == ImKeyBindMode_Toggle)
        {
            if (Pressed && !LastPressed)
                Toggled = !Toggled;
        }
        else
        {
            Toggled = Pressed;
        }

        LastPressed = Pressed;
        Globals::Aimbot::DrawFov = Toggled;
    }

    void RunService()
    {
        VisualsToggleHelper();
        AimbotFovToggleHelper();

        if (!Globals::Visuals::Enabled || Globals::VisualEngine.Address == 0 || Globals::Datamodel.Address == 0)
        {
            return;
        }

        ImDrawList* Draw = ImGui::GetBackgroundDrawList();
        Draw->Flags |= ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill;

        auto GetMousePos = [&]() -> ImVec2 {
            POINT p;
            if (Api::GetCursorPos(&p)) {
                if (Globals::RobloxWindow) {
                    Api::ScreenToClient(Globals::RobloxWindow, &p);
                }
                return ImVec2(static_cast<float>(p.x), static_cast<float>(p.y));
            }
            return ImGui::GetIO().MousePos;
        };

        std::shared_ptr<const std::vector<SDK::Player>> Snapshot;
        {
            std::lock_guard<std::mutex> Lock(Cache::Cache_Mutex);
            Snapshot = Globals::Player_Cache;
        }

        if (!Snapshot || Snapshot->empty()) {
            return;
        }

        ImGui::PushFont(Tahoma_BoldXP);

        SDK::Vector3 CamPos = { 0.f, 0.f, 0.f };
        bool CamPosValid = false;
        if (Globals::Visuals::WallCheck && Globals::Camera.Address != 0)
        {
            OBF_PROLOGUE;
            SDK::Camera Cam(Globals::Camera.Address);
            CamPos = Cam.Get_CameraPos();
            CamPosValid = true;
        }

        float BoxFillSinf = 0.f, BoxFillCosf = 1.f;
        if (Globals::Visuals::Box_Fill_Gradient) {
            float time = (Globals::Visuals::Box_Fill_Gradient_Rotate) ? (float)ImGui::GetTime() * (float)Globals::Visuals::BoxFillSpeed : 0.0f;
            BoxFillSinf = sinf(time);
            BoxFillCosf = cosf(time);
        }

        float ChamsFadeSinT = 0.f;
        if (Globals::Visuals::ChamsFade) {
            float time = (float)ImGui::GetTime() * (float)Globals::Visuals::ChamsFadeSpeed;
            ChamsFadeSinT = (sinf(time) + 1.0f) * 0.5f;
        }

        for (auto& Player : *Snapshot)
        {
            if (!Player.Character.Address) {
                continue;
            }

            const float* wl = Globals::Whitelist::Enabled && Globals::Whitelist::UserIDs.count(Player.UserID)
                ? Globals::Whitelist::Color : nullptr;

            if (Globals::Visuals::Render_Distance > 0.f && Player.Distance > Globals::Visuals::Render_Distance) {
                continue;
            }

            SDK::Part Head(Player.Head.Address);
            if (!Head.Address) continue;

            SDK::Vector3 HeadPos = Head.Get_Primitive().Get_Position();
            auto Head_W2S = Globals::VisualEngine.World_To_Screen(HeadPos);

            bool onScreen = (Head_W2S.x >= 0.f && Head_W2S.y >= 0.f);

            if (!onScreen && Player.HumanoidRootPart.Address)
            {
                SDK::Part Root(Player.HumanoidRootPart.Address);
                auto Root_W2S = Globals::VisualEngine.World_To_Screen(Root.Get_Primitive().Get_Position());
                onScreen = (Root_W2S.x >= 0.f && Root_W2S.y >= 0.f);
            }

            if (!onScreen)
                continue;

            float Left = FLT_MAX, Top = FLT_MAX, Right = -FLT_MAX, Bottom = -FLT_MAX;
            bool Valid = false;
            auto Bones = Visuals::Get_Bones(Player);

            bool NeedsFullBones = Globals::Visuals::Box || Globals::Visuals::Healthbar || Globals::Visuals::Chams || Globals::Visuals::Skeleton;

            if (NeedsFullBones)
            {
                if (Bones.empty()) continue;

                for (auto* Inst : Bones) {
                    if (!Inst || !Inst->Address) continue;
                    const auto Part = SDK::Part(Inst->Address);
                    if (!Part.Address) continue;

                    const auto Primitive = Part.Get_Primitive();
                    if (!Primitive.Address) continue;

                    SDK::Vector3 Size = Primitive.Get_Size();
                    const auto Position = Primitive.Get_Position();
                    const auto Rotation = Primitive.Get_Rotation();

                    if (std::isnan(Position.x) || std::isnan(Position.y) || std::isnan(Position.z))
                        continue;
                    if (std::isnan(Size.x) || std::isnan(Size.y) || std::isnan(Size.z))
                        continue;
                    if (Size.x == 0.f && Size.y == 0.f && Size.z == 0.f)
                        continue;

                    if (Globals::GameID == 292439477)
                    {
                        if (Inst == &Player.Head) Size = { 1.f, 1.f, 1.f };
                        else if (Inst == &Player.Torso || Inst == &Player.UpperTorso || Inst == &Player.LowerTorso) Size = { 2.f, 2.f, 1.f };
                        else Size = { 1.f, 2.f, 1.f };
                    }

                    if (Size.x == 0.f && Size.y == 0.f && Size.z == 0.f) continue;

                    for (const auto& LocalCorners : Corners) {
                        SDK::Vector3 Offset{
                            LocalCorners.x * Size.x * 0.5f,
                            LocalCorners.y * Size.y * 0.5f,
                            LocalCorners.z * Size.z * 0.5f
                        };
                        SDK::Vector3 World = Position + Rotation * Offset;
                        auto W2S = Globals::VisualEngine.World_To_Screen(World);
                        if (W2S.x < 0.f || W2S.y < 0.f) continue;
                        Valid = true;
                        Left = std::min(Left, W2S.x);
                        Top = std::min(Top, W2S.y);
                        Right = std::max(Right, W2S.x);
                        Bottom = std::max(Bottom, W2S.y);
                    }
                }
            }
            else
            {
                // Simple bounding box from Head position when only text overlays are active
                Left = Head_W2S.x - 25.f;
                Right = Head_W2S.x + 25.f;
                Top = Head_W2S.y - 25.f;
                Bottom = Head_W2S.y + 25.f;
                Valid = true;

                if (Player.HumanoidRootPart.Address)
                {
                    auto RootW2S = Globals::VisualEngine.World_To_Screen(
                        SDK::Part(Player.HumanoidRootPart.Address).Get_Primitive().Get_Position());
                    if (RootW2S.x >= 0.f && RootW2S.y >= 0.f)
                    {
                        Left = std::min(Left, RootW2S.x - 15.f);
                        Right = std::max(Right, RootW2S.x + 15.f);
                        Top = std::min(Top, RootW2S.y);
                        Bottom = std::max(Bottom, RootW2S.y + 20.f);
                    }
                }
            }

            if (!Valid || Left >= Right || Top >= Bottom) continue;

            if (Globals::Visuals::WallCheck && CamPosValid)
            {
                bool visible = wallcheck->is_visible(CamPos, HeadPos);
                if (!visible)
                {
                    auto try_point = [&](const SDK::Instance& inst) -> bool
                    {
                        if (!inst.Address) return false;
                        SDK::Part p(inst.Address);
                        if (!p.Address) return false;
                        auto prim = p.Get_Primitive();
                        if (!prim.Address) return false;
                        return wallcheck->is_visible(CamPos, prim.Get_Position());
                    };
                    visible = try_point(Player.HumanoidRootPart) ||
                              try_point(Player.UpperTorso) ||
                              try_point(Player.LowerTorso) ||
                              try_point(Player.Torso);
                }
                if (!visible)
                    wl = Globals::Visuals::Colors::Occluded;
            }

            ImVec2 Pos(Left - 1.f, Top - 1.f);
            ImVec2 Size((Right - Left) + 2.f, (Bottom - Top) + 2.f);

            // box rendering
            if (Globals::Visuals::Box)
            {
                if (Globals::Visuals::Box_Type == 0) {
                    Pos.x = std::round(Pos.x);
                    Pos.y = std::round(Pos.y);
                    Size.x = std::round(Size.x);
                    Size.y = std::round(Size.y);
                    ImVec2 Min = Pos;
                    ImVec2 Max = ImVec2(Pos.x + Size.x, Pos.y + Size.y);

                    if (Globals::Visuals::Box_Fill) {
                        if (Globals::Visuals::Box_Fill_Gradient) {
                            const float* colA = (wl ? wl : Globals::Visuals::Colors::BoxFill_Top);
                            const float* colB = (wl ? wl : Globals::Visuals::Colors::BoxFill_Bottom);
                            auto LerpCol = [&](float t) -> ImU32 {
                                return IM_COL32(
                                    (int)((colA[0] + (colB[0] - colA[0]) * t) * 255.0f),
                                    (int)((colA[1] + (colB[1] - colA[1]) * t) * 255.0f),
                                    (int)((colA[2] + (colB[2] - colA[2]) * t) * 255.0f),
                                    (int)((colA[3] + (colB[3] - colA[3]) * t) * 255.0f));
                            };
                            float t_s = (BoxFillSinf + 1.0f) * 0.5f;
                            float t_c = (BoxFillCosf + 1.0f) * 0.5f;
                            float t_ns = (-BoxFillSinf + 1.0f) * 0.5f;
                            float t_nc = (-BoxFillCosf + 1.0f) * 0.5f;
                            ImU32 c_tl, c_tr, c_br, c_bl;
                            if (Globals::Visuals::Box_Fill_Type == 0) { c_tl = c_bl = LerpCol(t_s); c_tr = c_br = LerpCol(t_c); }
                            else if (Globals::Visuals::Box_Fill_Type == 1) { c_tl = c_tr = LerpCol(t_s); c_bl = c_br = LerpCol(t_c); }
                            else { c_tl = LerpCol(t_s); c_tr = LerpCol(t_c); c_br = LerpCol(t_ns); c_bl = LerpCol(t_nc); }
                            Draw->AddRectFilledMultiColor(ImVec2(Min.x + 1.f, Min.y + 1.f), ImVec2(Max.x - 1.f, Max.y - 1.f), c_tl, c_tr, c_br, c_bl);
                        } else {
                            Draw->AddRectFilled(ImVec2(Min.x + 1.f, Min.y + 1.f), ImVec2(Max.x - 1.f, Max.y - 1.f), FlotationDevice((wl ? wl : Globals::Visuals::Colors::BoxFill_Top)));
                        }
                    }
                    Draw->AddRect(Min, Max, IM_COL32(0, 0, 0, 255), 0.f, 0, 1.f);
                    Draw->AddRect(ImVec2(Min.x - 1.f, Min.y - 1.f), ImVec2(Max.x + 1.f, Max.y + 1.f), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)), 0.f, 0, 1.f);
                    Draw->AddRect(ImVec2(Min.x - 2.f, Min.y - 2.f), ImVec2(Max.x + 2.f, Max.y + 2.f), IM_COL32(0, 0, 0, 255), 0.f, 0, 1.f);
                }
                else if (Globals::Visuals::Box_Type == 1) {
                    float X1 = Pos.x - 1.f, Y1 = Pos.y - 1.f, X2 = Pos.x + Size.x + 1.f, Y2 = Pos.y + Size.y + 1.f;
                    float Box_Width = X2 - X1, Box_Height = Y2 - Y1;
                    float Length = std::min(Box_Width, Box_Height) * 0.25f;
                    Length = std::min(Length, 50.0f);
                    Length = std::min(Length, (std::min(Box_Width, Box_Height) * 0.5f) - 1.0f);
                    float X1_Length = X1 + Length, Y1_Length = Y1 + Length, X2_Length = X2 - Length, Y2_Length = Y2 - Length;

                    if (Globals::Visuals::Box_Fill) {
                        if (Globals::Visuals::Box_Fill_Gradient) {
                            const float* colA = (wl ? wl : Globals::Visuals::Colors::BoxFill_Top);
                            const float* colB = (wl ? wl : Globals::Visuals::Colors::BoxFill_Bottom);
                            auto LerpCol = [&](float t) -> ImU32 {
                                return IM_COL32(
                                    (int)((colA[0] + (colB[0] - colA[0]) * t) * 255.0f),
                                    (int)((colA[1] + (colB[1] - colA[1]) * t) * 255.0f),
                                    (int)((colA[2] + (colB[2] - colA[2]) * t) * 255.0f),
                                    (int)((colA[3] + (colB[3] - colA[3]) * t) * 255.0f));
                            };
                            float t_s = (BoxFillSinf + 1.0f) * 0.5f, t_c = (BoxFillCosf + 1.0f) * 0.5f, t_ns = (-BoxFillSinf + 1.0f) * 0.5f, t_nc = (-BoxFillCosf + 1.0f) * 0.5f;
                            ImU32 c_tl, c_tr, c_br, c_bl;
                            if (Globals::Visuals::Box_Fill_Type == 0) { c_tl = c_bl = LerpCol(t_s); c_tr = c_br = LerpCol(t_c); }
                            else if (Globals::Visuals::Box_Fill_Type == 1) { c_tl = c_tr = LerpCol(t_s); c_bl = c_br = LerpCol(t_c); }
                            else { c_tl = LerpCol(t_s); c_tr = LerpCol(t_c); c_br = LerpCol(t_ns); c_bl = LerpCol(t_nc); }
                            Draw->AddRectFilledMultiColor(ImVec2(X1 + 2.f, Y1 + 2.f), ImVec2(X2 - 2.f, Y2 - 2.f), c_tl, c_tr, c_br, c_bl);
                        } else {
                            Draw->AddRectFilled(ImVec2(X1 + 2.f, Y1 + 2.f), ImVec2(X2 - 2.f, Y2 - 2.f), FlotationDevice((wl ? wl : Globals::Visuals::Colors::BoxFill_Top)));
                        }
                    }
                    Draw->AddRectFilled(ImVec2(X1 - 1.f, Y1 - 1.f), ImVec2(X1_Length + 1.f, Y1 + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X1 - 1.f, Y1 - 1.f), ImVec2(X1 + 1.f, Y1_Length + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X2_Length - 1.f, Y1 - 1.f), ImVec2(X2 + 1.f, Y1 + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X2 - 1.f, Y1 - 1.f), ImVec2(X2 + 1.f, Y1_Length + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X1 - 1.f, Y2 - 1.f), ImVec2(X1_Length + 1.f, Y2 + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X1 - 1.f, Y2_Length - 1.f), ImVec2(X1 + 1.f, Y2 + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X2_Length - 1.f, Y2 - 1.f), ImVec2(X2 + 1.f, Y2 + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X2 - 1.f, Y2_Length - 1.f), ImVec2(X2 + 1.f, Y2 + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X1 + 1.f, Y1 + 1.f), ImVec2(X1_Length + 1.f, Y1 + 2.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X1 + 1.f, Y1 + 1.f), ImVec2(X1 + 2.f, Y1_Length + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X2_Length - 1.f, Y1 + 1.f), ImVec2(X2 - 1.f, Y1 + 2.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X2 - 2.f, Y1 + 1.f), ImVec2(X2 - 1.f, Y1_Length + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X1 + 1.f, Y2 - 2.f), ImVec2(X1_Length + 1.f, Y2 - 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X1 + 1.f, Y2_Length - 1.f), ImVec2(X1 + 2.f, Y2 - 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X2_Length - 1.f, Y2 - 2.f), ImVec2(X2 - 1.f, Y2 - 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X2 - 2.f, Y2_Length - 1.f), ImVec2(X2 - 1.f, Y2 - 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X1, Y1), ImVec2(X1_Length, Y1 + 1.f), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)));
                    Draw->AddRectFilled(ImVec2(X1, Y1), ImVec2(X1 + 1.f, Y1_Length), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)));
                    Draw->AddRectFilled(ImVec2(X2_Length, Y1), ImVec2(X2, Y1 + 1.f), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)));
                    Draw->AddRectFilled(ImVec2(X2 - 1.f, Y1), ImVec2(X2, Y1_Length), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)));
                    Draw->AddRectFilled(ImVec2(X1, Y2 - 1.f), ImVec2(X1_Length, Y2), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)));
                    Draw->AddRectFilled(ImVec2(X1, Y2_Length), ImVec2(X1 + 1.f, Y2), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)));
                    Draw->AddRectFilled(ImVec2(X2_Length, Y2 - 1.f), ImVec2(X2, Y2), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)));
                    Draw->AddRectFilled(ImVec2(X2 - 1.f, Y2_Length), ImVec2(X2, Y2), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Box)));
                }
            }

            // healthbar
            if (Globals::Visuals::Healthbar) {
                if (Globals::Visuals::Healthbar_Type == 0) {
                    float Ratio = (Player.MaxHealth > 0.f) ? Player.Health / Player.MaxHealth : 0.f;
                    Ratio = ImClamp(Ratio, 0.f, 1.f);
                    float Gap = (float)Globals::Visuals::Gap, Thickness = (float)Globals::Visuals::Thickness;
                    float X_Min = Pos.x - Gap - Thickness - 3.f, Y_Min = Pos.y - 1.f, Y_Max = Pos.y + Size.y + 1.f;
                    Draw->AddRectFilled(ImVec2(X_Min - 1.f, Y_Min - 1.f), ImVec2(X_Min + Thickness + 1.f, Y_Max + 1.f), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X_Min, Y_Min), ImVec2(X_Min + Thickness, Y_Max), IM_COL32(130, 130, 130, 150));
                    float Height = (Y_Max - Y_Min) * Ratio;
                    Draw->AddRectFilled(ImVec2(X_Min, Y_Max - Height), ImVec2(X_Min + Thickness, Y_Max), FlotationDevice((wl ? wl : Globals::Visuals::Colors::Healthbar)));
                }
                else if (Globals::Visuals::Healthbar_Type == 1) {
                    float Ratio = (Player.MaxHealth > 0.f) ? Player.Health / Player.MaxHealth : 0.f;
                    Ratio = ImClamp(Ratio, 0.f, 1.f);
                    float Gap = (float)Globals::Visuals::Gap, Thickness = (float)Globals::Visuals::Thickness;
                    float X_Min = Pos.x - Gap - Thickness - 3.f, Y_Min = Pos.y - 1.f, Y_Max = Pos.y + Size.y + 1.f;
                    Draw->AddRectFilled(ImVec2(X_Min - 1, Y_Min - 1), ImVec2(X_Min + Thickness + 1, Y_Max + 1), IM_COL32(0, 0, 0, 255));
                    Draw->AddRectFilled(ImVec2(X_Min, Y_Min), ImVec2(X_Min + Thickness, Y_Max), IM_COL32(130, 130, 130, 150));
                    const float* hbt = (wl ? wl : Globals::Visuals::Colors::Healthbar_Top);
                    ImU32 TopColor = IM_COL32((int)(hbt[0] * 255), (int)(hbt[1] * 255), (int)(hbt[2] * 255), (int)(hbt[3] * 255));
                    const float* hbm = (wl ? wl : Globals::Visuals::Colors::Healthbar_Middle);
                    ImU32 MiddleColor = IM_COL32((int)(hbm[0] * 255), (int)(hbm[1] * 255), (int)(hbm[2] * 255), (int)(hbm[3] * 255));
                    const float* hbb = (wl ? wl : Globals::Visuals::Colors::Healthbar_Bottom);
                    ImU32 BottomColor = IM_COL32((int)(hbb[0] * 255), (int)(hbb[1] * 255), (int)(hbb[2] * 255), (int)(hbb[3] * 255));
                    float FullHeight = Y_Max - Y_Min, HealthHeight = FullHeight * Ratio;
                    float FillMinY = Y_Max - HealthHeight, MidY = Y_Min + FullHeight * 0.5f;
                    if (FillMinY < MidY) Draw->AddRectFilledMultiColor(ImVec2(X_Min, ImMax(FillMinY, Y_Min)), ImVec2(X_Min + Thickness, MidY), TopColor, TopColor, MiddleColor, MiddleColor);
                    Draw->AddRectFilledMultiColor(ImVec2(X_Min, ImMax(FillMinY, MidY)), ImVec2(X_Min + Thickness, Y_Max), MiddleColor, MiddleColor, BottomColor, BottomColor);
                }
            }

            // text overlays
            if (Globals::Visuals::Health) {
                char HealthStr[32];
                snprintf(HealthStr, sizeof(HealthStr), skCrypt("[%d]"), static_cast<int>(Player.Health));
                float X_Text = Pos.x - 6.0f;
                if (Globals::Visuals::Healthbar) X_Text -= Globals::Visuals::Thickness + Globals::Visuals::Gap;
                float Y_text = Pos.y - 3.0f;
                ImVec2 Text_Size = ImGui::CalcTextSize(HealthStr);
                Outline(ImVec2(X_Text - Text_Size.x, Y_text), HealthStr, (wl ? wl : Globals::Visuals::Colors::Health));
            }

            if (Globals::Visuals::Name) {
                if (Globals::Visuals::Name_Type == 0) {
                    ImVec2 Text_Size = ImGui::CalcTextSize(Player.Name.c_str());
                    Outline(ImVec2(Pos.x + (Size.x * 0.5f) - (Text_Size.x * 0.5f), Pos.y - Text_Size.y - 3.f), Player.Name.c_str(), (wl ? wl : Globals::Visuals::Colors::Name));
                }
                else if (Globals::Visuals::Name_Type == 1) {
                    ImVec2 Text_Size = ImGui::CalcTextSize(Player.Display_Name.c_str());
                    Outline(ImVec2(Pos.x + (Size.x * 0.5f) - (Text_Size.x * 0.5f), Pos.y - Text_Size.y - 3.f), Player.Display_Name.c_str(), (wl ? wl : Globals::Visuals::Colors::Name));
                }
                else if (Globals::Visuals::Name_Type == 2) {
                    char FullName[512];
                    snprintf(FullName, sizeof(FullName), skCrypt("%s [%s]"), Player.Name.c_str(), Player.Display_Name.c_str());
                    ImVec2 Text_Size = ImGui::CalcTextSize(FullName);
                    float NameW = ImGui::CalcTextSize(Player.Name.c_str()).x + ImGui::CalcTextSize(skCrypt(" ")).x;
                    float BracketW = ImGui::CalcTextSize(skCrypt("[")).x;
                    float DisplayW = ImGui::CalcTextSize(Player.Display_Name.c_str()).x;
                    ImVec2 Position(Pos.x + (Size.x * 0.5f) - (Text_Size.x * 0.5f), Pos.y - Text_Size.y - 3.f);
                    Outline(Position, Player.Name.c_str(), (wl ? wl : Globals::Visuals::Colors::Name));
                    Outline(ImVec2(Position.x + NameW, Position.y - 2.f), skCrypt("["), (wl ? wl : Globals::Visuals::Colors::Name));
                    static float white[4] = { 1.f, 1.f, 1.f, 1.f };
                    Outline(ImVec2(Position.x + NameW + BracketW, Position.y - 1.f), Player.Display_Name.c_str(), white);
                    Outline(ImVec2(Position.x + NameW + BracketW + DisplayW, Position.y - 2.f), skCrypt("]"), (wl ? wl : Globals::Visuals::Colors::Name));
                }
            }

            if (Globals::Visuals::Distance) {
                char Buffer[16];
                snprintf(Buffer, sizeof(Buffer), skCrypt("[%dm]"), static_cast<int>(Player.Distance));
                ImVec2 Text_Size = ImGui::CalcTextSize(Buffer);
                Outline(ImVec2(Pos.x + (Size.x * 0.5f) - (Text_Size.x * 0.5f), Pos.y + Size.y + 3.0f), Buffer, (wl ? wl : Globals::Visuals::Colors::Distance));
            }

            if (Globals::Visuals::Rig_Type) {
                const char* Rig_Type = nullptr;
                if (Player.Rig_Type == 1) Rig_Type = skCrypt("[R15]");
                else if (Player.Rig_Type == 0) Rig_Type = skCrypt("[R6]");
                else continue;
                ImVec2 Text_Size = ImGui::CalcTextSize(Rig_Type);
                Outline(ImVec2(std::round(Pos.x + Size.x + 5.0f), std::round(Pos.y - Text_Size.y + 10.0f)), Rig_Type, (wl ? wl : Globals::Visuals::Colors::Rig_Type));
            }

            if (Globals::Visuals::Tool) {
                char Cl_Name[256];
                const std::string& Tool_Name = Player.Tool_Name;
                if (Tool_Name.empty()) strcpy_s(Cl_Name, skCrypt("[None]"));
                else {
                    int idx = 0;
                    Cl_Name[idx++] = '[';
                    for (char c : Tool_Name) { if (c != '[' && c != ']' && idx < 253) Cl_Name[idx++] = c; }
                    Cl_Name[idx++] = ']'; Cl_Name[idx] = '\0';
                }
                ImVec2 Text_Size = ImGui::CalcTextSize(Cl_Name);
                float Offset = Globals::Visuals::Distance ? 18.0f : 3.0f;
                Outline(ImVec2(std::round(Pos.x + (Size.x * 0.5f) - (Text_Size.x * 0.5f)), std::round(Pos.y + Size.y + Offset)), Cl_Name, (wl ? wl : Globals::Visuals::Colors::Tool));
            }

            // chams
            if (Globals::Visuals::Chams) {
                ImDrawList* Draw = ImGui::GetBackgroundDrawList();
                ImDrawListFlags SavedFlags = Draw->Flags;
                Draw->Flags |= ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill;

                auto ProjectPart = [&](const SDK::Instance* Inst, const SDK::Part& Part) -> const std::vector<ImVec2>& {
                    static thread_local std::vector<ImVec2> Projected;
                    static thread_local std::vector<ImVec2> Hull;
                    Projected.clear(); Hull.clear();
                    if (!Part.Address) return Hull;
                    const auto Prim = Part.Get_Primitive();
                    auto Size = Prim.Get_Size();
                    const auto Pos = Prim.Get_Position();
                    const auto Rot = Prim.Get_Rotation();
                    if (Globals::GameID == 292439477) {
                        if (Inst == &Player.Head) Size = { 1.f, 1.f, 1.f };
                        else if (Inst == &Player.Torso || Inst == &Player.UpperTorso || Inst == &Player.LowerTorso) Size = { 2.f, 2.f, 1.f };
                        else Size = { 1.f, 2.f, 1.f };
                    }
                    Projected.reserve(8);
                    for (int i = 0; i < 8; ++i) {
                        const auto& Corner = Corners[i];
                        SDK::Vector3 World = Pos + Rot * SDK::Vector3{ Corner.x * Size.x * 0.5f, Corner.y * Size.y * 0.5f, Corner.z * Size.z * 0.5f };
                        SDK::Vector2 Screen = Globals::VisualEngine.World_To_Screen(World);
                        if (Screen.x >= 0.f && Screen.y >= 0.f) Projected.emplace_back(Screen.x, Screen.y);
                    }
                    if (Projected.size() < 3) return Hull;
                    std::sort(Projected.begin(), Projected.end(), [](const ImVec2& A, const ImVec2& B) { return A.x < B.x || (A.x == B.x && A.y < B.y); });
                    Hull.reserve(Projected.size() * 2);
                    auto Cross = [](const ImVec2& O, const ImVec2& A, const ImVec2& B) { return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x); };
                    for (auto& P : Projected) { while (Hull.size() >= 2 && Cross(Hull[Hull.size() - 2], Hull.back(), P) <= 0) Hull.pop_back(); Hull.push_back(P); }
                    size_t T = Hull.size() + 1;
                    for (int i = (int)Projected.size() - 1; i >= 0; --i) { auto& P = Projected[i]; while (Hull.size() >= T && Cross(Hull[Hull.size() - 2], Hull.back(), P) <= 0) Hull.pop_back(); Hull.push_back(P); }
                    Hull.pop_back();
                    return Hull;
                };

                Clipper2Lib::Paths64 AllParts;
                for (auto* Inst : Bones) {
                    const SDK::Part Part(Inst->Address);
                    auto Hull = ProjectPart(Inst, Part);
                    if (Hull.size() < 3) continue;
                    Clipper2Lib::Path64 Path;
                    Path.reserve(Hull.size());
                    for (auto& Pt : Hull) Path.emplace_back(static_cast<int64_t>(Pt.x * 1000.0), static_cast<int64_t>(Pt.y * 1000.0));
                    AllParts.push_back(std::move(Path));
                }
                if (!AllParts.empty()) {
                    auto UnifiedSolution = Clipper2Lib::Union(AllParts, Clipper2Lib::FillRule::NonZero);
                    std::vector<std::vector<ImVec2>> AllPolys;
                    AllPolys.reserve(UnifiedSolution.size());
                    for (auto& Sp : UnifiedSolution) {
                        std::vector<ImVec2> Poly; Poly.reserve(Sp.size());
                        for (auto& Pt : Sp) Poly.emplace_back(Pt.x / 1000.0f, Pt.y / 1000.0f);
                        if (Poly.size() >= 3) AllPolys.push_back(std::move(Poly));
                    }
                    const float* cf = (wl ? wl : Globals::Visuals::Colors::Chams);
                    ImU32 FillColor = ImGui::ColorConvertFloat4ToU32({ cf[0], cf[1], cf[2], cf[3] });
                    const float* co = (wl ? wl : Globals::Visuals::Colors::ChamsOutline);
                    ImU32 OutlineColor = ImGui::ColorConvertFloat4ToU32({ co[0], co[1], co[2], co[3] });
                    if (Globals::Visuals::ChamsFade) {
                        ImVec4 c = ImGui::ColorConvertU32ToFloat4(FillColor);
                        c.w *= ChamsFadeSinT; FillColor = ImGui::ColorConvertFloat4ToU32(c);
                    }
                    for (auto& Poly : AllPolys) {
                        Draw->AddConcavePolyFilled(Poly.data(), static_cast<int>(Poly.size()), FillColor);
                        Draw->AddPolyline(Poly.data(), static_cast<int>(Poly.size()), OutlineColor, true, 1.0f);
                    }
                }
                Draw->Flags = SavedFlags;
            }

            // skeleton
            if (Globals::Visuals::Skeleton) {
                const ImU32 SkelCol = FlotationDevice((wl ? wl : Globals::Visuals::Colors::Skeleton));
                const ImU32 OutlineCol = IM_COL32(0, 0, 0, 255);
                const float Thickness = 1.0f;

                auto W2S = [&](const SDK::Vector3& WorldPos, ImVec2& Out) -> bool {
                    auto ScreenPos = Globals::VisualEngine.World_To_Screen(WorldPos);
                    if (ScreenPos.x <= 0.f || ScreenPos.y <= 0.f) return false;
                    Out.x = std::roundf(ScreenPos.x); Out.y = std::roundf(ScreenPos.y);
                    return true;
                };

                auto DrawPoly = [&](const ImVec2* Points, int Count) {
                    if (Count < 2) return;
                    Draw->AddPolyline(Points, Count, OutlineCol, false, Thickness + 2.f);
                    Draw->AddPolyline(Points, Count, SkelCol, false, Thickness);
                };

                if (Player.UpperTorso.Address && Player.LowerTorso.Address) {
                    auto ProcessR15Chain = [&](const SDK::Instance* Instances, int Count) {
                        ImVec2 ScreenPoints[8];
                        int ValidCount = 0;
                        int MaxCount = std::min(Count, 8);
                        for (int i = 0; i < MaxCount; ++i) {
                            if (!Instances[i].Address) { DrawPoly(ScreenPoints, ValidCount); ValidCount = 0; continue; }
                            SDK::Part Part(Instances[i].Address);
                            if (!Part.Address) { DrawPoly(ScreenPoints, ValidCount); ValidCount = 0; continue; }
                            ImVec2 ScreenPos;
                            if (!W2S(Part.Get_Primitive().Get_Position(), ScreenPos)) { DrawPoly(ScreenPoints, ValidCount); ValidCount = 0; continue; }
                            ScreenPoints[ValidCount++] = ScreenPos;
                        }
                        DrawPoly(ScreenPoints, ValidCount);
                    };
                    const SDK::Instance Spine[] = { Player.Head, Player.UpperTorso, Player.LowerTorso };
                    ProcessR15Chain(Spine, 3);
                    const SDK::Instance LeftArm[] = { Player.UpperTorso, Player.LeftUpperArm, Player.LeftLowerArm, Player.LeftHand };
                    ProcessR15Chain(LeftArm, 4);
                    const SDK::Instance RightArm[] = { Player.UpperTorso, Player.RightUpperArm, Player.RightLowerArm, Player.RightHand };
                    ProcessR15Chain(RightArm, 4);
                    const SDK::Instance LeftLeg[] = { Player.LowerTorso, Player.LeftUpperLeg, Player.LeftLowerLeg, Player.LeftFoot };
                    ProcessR15Chain(LeftLeg, 4);
                    const SDK::Instance RightLeg[] = { Player.LowerTorso, Player.RightUpperLeg, Player.RightLowerLeg, Player.RightFoot };
                    ProcessR15Chain(RightLeg, 4);
                }
                else if (Player.Torso.Address && Player.Head.Address) {
                    SDK::Part TorsoPart(Player.Torso.Address), HeadPart(Player.Head.Address);
                    const auto& TorsoPrim = TorsoPart.Get_Primitive();
                    const auto& HeadPrim = HeadPart.Get_Primitive();
                    const SDK::Vector3 TorsoPos = TorsoPrim.Get_Position(), TorsoSize = TorsoPrim.Get_Size();
                    const auto TorsoRot = TorsoPrim.Get_Rotation();
                    const SDK::Vector3 HeadPos = HeadPrim.Get_Position(), HeadSize = HeadPrim.Get_Size();
                    const SDK::Vector3 ShoulderCenter = TorsoPos + TorsoRot * SDK::Vector3{ 0, TorsoSize.y * 0.2f, 0 };
                    const SDK::Vector3 HipCenter = TorsoPos - TorsoRot * SDK::Vector3{ 0, TorsoSize.y * 0.4f, 0 };
                    const SDK::Vector3 HeadBottom = HeadPos - SDK::Vector3{ 0, HeadSize.y * 0.5f, 0 };
                    const SDK::Vector3 ShoulderLeft = ShoulderCenter + TorsoRot * SDK::Vector3{ -TorsoSize.x * 0.5f, 0, 0 };
                    const SDK::Vector3 ShoulderRight = ShoulderCenter + TorsoRot * SDK::Vector3{ TorsoSize.x * 0.5f, 0, 0 };

                    auto ProcessR6Chain = [&](const SDK::Vector3* Points, int Count) {
                        ImVec2 ScreenPoints[8]; int ValidCount = 0; int MaxCount = std::min(Count, 8);
                        for (int i = 0; i < MaxCount; ++i) {
                            ImVec2 ScreenPos;
                            if (W2S(Points[i], ScreenPos)) ScreenPoints[ValidCount++] = ScreenPos;
                        }
                        DrawPoly(ScreenPoints, ValidCount);
                    };

                    const SDK::Vector3 SpinePts[] = { HeadPos, HeadBottom, ShoulderCenter, HipCenter };
                    ProcessR6Chain(SpinePts, 4);

                    {
                        SDK::Vector3 ArmPts[4]; int Count = 0;
                        ArmPts[Count++] = ShoulderCenter; ArmPts[Count++] = ShoulderLeft;
                        if (Player.LeftArm.Address) { SDK::Part Arm(Player.LeftArm.Address); const auto& ArmPrim = Arm.Get_Primitive(); const auto& ArmRot = ArmPrim.Get_Rotation(); const SDK::Vector3 ArmPos = ArmPrim.Get_Position(); const SDK::Vector3 ArmSize = ArmPrim.Get_Size(); ArmPts[Count++] = ArmPos + ArmRot * SDK::Vector3{ 0, ArmSize.y * 0.2f, 0 }; ArmPts[Count++] = ArmPos - ArmRot * SDK::Vector3{ 0, ArmSize.y * 0.5f, 0 }; }
                        ProcessR6Chain(ArmPts, Count);
                    }
                    {
                        SDK::Vector3 ArmPts[4]; int Count = 0;
                        ArmPts[Count++] = ShoulderCenter; ArmPts[Count++] = ShoulderRight;
                        if (Player.RightArm.Address) { SDK::Part Arm(Player.RightArm.Address); const auto& ArmPrim = Arm.Get_Primitive(); const auto& ArmRot = ArmPrim.Get_Rotation(); const SDK::Vector3 ArmPos = ArmPrim.Get_Position(); const SDK::Vector3 ArmSize = ArmPrim.Get_Size(); ArmPts[Count++] = ArmPos + ArmRot * SDK::Vector3{ 0, ArmSize.y * 0.2f, 0 }; ArmPts[Count++] = ArmPos - ArmRot * SDK::Vector3{ 0, ArmSize.y * 0.5f, 0 }; }
                        ProcessR6Chain(ArmPts, Count);
                    }
                    {
                        SDK::Vector3 LegPts[3]; int Count = 0;
                        LegPts[Count++] = HipCenter;
                        if (Player.LeftLeg.Address) { SDK::Part Leg(Player.LeftLeg.Address); const auto& LegPrim = Leg.Get_Primitive(); const auto& LegRot = LegPrim.Get_Rotation(); const SDK::Vector3 LegPos = LegPrim.Get_Position(); const SDK::Vector3 LegSize = LegPrim.Get_Size(); LegPts[Count++] = LegPos + LegRot * SDK::Vector3{ 0, LegSize.y * 0.5f, 0 }; LegPts[Count++] = LegPos - LegRot * SDK::Vector3{ 0, LegSize.y * 0.5f, 0 }; }
                        ProcessR6Chain(LegPts, Count);
                    }
                    {
                        SDK::Vector3 LegPts[3]; int Count = 0;
                        LegPts[Count++] = HipCenter;
                        if (Player.RightLeg.Address) { SDK::Part Leg(Player.RightLeg.Address); const auto& LegPrim = Leg.Get_Primitive(); const auto& LegRot = LegPrim.Get_Rotation(); const SDK::Vector3 LegPos = LegPrim.Get_Position(); const SDK::Vector3 LegSize = LegPrim.Get_Size(); LegPts[Count++] = LegPos + LegRot * SDK::Vector3{ 0, LegSize.y * 0.5f, 0 }; LegPts[Count++] = LegPos - LegRot * SDK::Vector3{ 0, LegSize.y * 0.5f, 0 }; }
                        ProcessR6Chain(LegPts, Count);
                    }
                }
            }
        }

        ImGui::PopFont();

        // FOV rings
        if (Globals::Aimbot::DrawFov) {
            static float FovRotation = 0.f;
            if (Globals::Aimbot::FovSpin) {
                float dir = (Globals::Aimbot::FovSpinDirection == 0) ? 1.0f : -1.0f;
                FovRotation += Globals::Aimbot::FovSpinSpeed / 1000.f * dir;
            }
            ImVec2 MPos = GetMousePos();
            ImDrawList* DrawFovList = ImGui::GetBackgroundDrawList();
            const int Sides = 10; ImVec2 Points[Sides];
            float Step = 2.f * IM_PI / Sides;
            for (int i = 0; i < Sides; i++) {
                float Angle = i * Step + FovRotation;
                Points[i] = ImVec2(MPos.x + cosf(Angle) * Globals::Aimbot::FovSize, MPos.y + sinf(Angle) * Globals::Aimbot::FovSize);
            }
            ImU32 Color = FlotationDevice(Globals::Aimbot::FovColor);
            if (Globals::Aimbot::FillFov) DrawFovList->AddConvexPolyFilled(Points, Sides, Color);
            DrawFovList->AddPolyline(Points, Sides, Color, true, 2.f);
        }

    }
}
