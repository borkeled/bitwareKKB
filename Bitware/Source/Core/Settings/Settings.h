#pragma once
#include <cstdint>
#include <array>
#include <atomic>
#include <unordered_set>
#include <imgui/imgui.h>
#include <imgui/addons/imgui_addons.h>
#include "Types.h"
#include <Engine/Engine.h>
#include <Infrastructure/Obfuscation.h>

namespace SettingsStore {

    inline bool Aimbot_Enabled;
    inline bool Aimbot_DrawFov;
    inline bool Aimbot_FovSpin;
    inline bool Aimbot_FillFov;
    inline bool Aimbot_useFov;
    inline bool Aimbot_AimbotSticky;
    inline bool Aimbot_Shake;
    inline bool Aimbot_KnockedCheck;
    inline bool Aimbot_WallCheck;
    inline bool Aimbot_ClosestPlayerFound;
    inline int Aimbot_type;
    inline int Aimbot_HitPart;
    inline int Aimbot_FovSpinDirection;
    inline int Aimbot_FovSpinSpeed;
    inline float Aimbot_FovSize;
    inline float Aimbot_ShakeX;
    inline float Aimbot_ShakeY;
    inline float Aimbot_ShakeZ;
    inline float Aimbot_FovColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline ImGuiKey Aimbot_Key = ImGuiKey_Q;
    inline ImKeyBindMode Aimbot_Mode = ImKeyBindMode_Hold;
    inline float Aimbot_Mouse_SmoothingX = 1.0f;
    inline float Aimbot_Mouse_SmoothingY = 1.0f;
    inline float Aimbot_Mouse_Sensitivity = 1.0f;
    inline float Aimbot_Camera_SmoothingX = 1.0f;
    inline float Aimbot_Camera_SmoothingY = 1.0f;
    inline SDK::Instance Aimbot_AimTarget;

    inline bool Visuals_Enabled;
    inline bool Visuals_Box;
    inline bool Visuals_Box_Fill;
    inline bool Visuals_Box_Fill_Gradient;
    inline bool Visuals_Box_Fill_Gradient_Rotate;
    inline bool Visuals_Healthbar;
    inline ImGuiKey Visuals_ToggleKey = ImGuiKey_None;
    inline ImKeyBindMode Visuals_ToggleMode = ImKeyBindMode_Toggle;
    inline ImGuiKey Aimbot_FovToggleKey = ImGuiKey_None;
    inline ImKeyBindMode Aimbot_FovToggleMode = ImKeyBindMode_Toggle;
    inline bool Visuals_Health;
    inline bool Visuals_Name;
    inline bool Visuals_Distance;
    inline bool Visuals_Rig_Type;
    inline bool Visuals_Tool;
    inline bool Visuals_Skeleton;
    inline bool Visuals_Chams;
    inline bool Visuals_ChamsFade;
    inline float Visuals_Render_Distance = 200.0f;
    inline int Visuals_ChamsFadeSpeed = 2;
    inline int Visuals_BoxFillSpeed = 2;
    inline int Visuals_Healthbar_Type = 1;
    inline int Visuals_Box_Type;
    inline int Visuals_Box_Fill_Type;
    inline int Visuals_Name_Type = 1;
    inline int Visuals_Gap = 2;
    inline int Visuals_Thickness = 2;
    inline float Visuals_BoxColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_BoxFillTop[4] = { 0.0f, 1.0f, 0.0f, 0.50f };
    inline float Visuals_BoxFillBottom[4] = { 0.f, 0.f, 0.f, 0.50f };
    inline float Visuals_HealthbarColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_NameColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_DistanceColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_RigTypeColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_ToolColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_SkeletonColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_ChamsColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_ChamsOutline[4] = { 0.f, 0.f, 0.f, 1.0f };
    inline float Visuals_HealthbarTop[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_HealthbarMiddle[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
    inline float Visuals_HealthbarBottom[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    inline float Visuals_HealthColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline bool Visuals_WallCheck = false;
    inline float Visuals_OccludedColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    inline int WallCheck_Method = 0;

    inline bool World_Skybox;
    inline bool World_Rotate;
    inline bool World_Ambience;
    inline bool World_Fog;
    inline bool World_Brightness;
    inline bool World_Exposure;
    inline bool World_FOV;
    inline int World_Skybox_Type;
    inline float World_Rotate_Speed = 1.0f;
    inline float World_Fog_Distance = 300.0f;
    inline float World_FOV_Distance = 90.0f;
    inline float World_ExposureI;
    inline float World_BrightnessI = 1.0f;
    inline float World_AmbienceColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline float World_FogColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };

    inline bool Silent_Enabled;
    inline bool Silent_DrawFov;
    inline bool Silent_StickyAim;
    inline bool Silent_SpoofMouse;
    inline bool Silent_UseFov;
    inline bool Silent_KnockedCheck;
    inline bool Silent_WallCheck;
    inline bool Silent_GunBasedFov;
    inline bool Silent_FovSpin;
    inline bool Silent_FillFov;
    inline float Silent_Fov = 67.67f;
    inline float Silent_FovDoubleBarrel = 67.67f;
    inline float Silent_FovTacticalShotgun = 67.67f;
    inline float Silent_FovRevolver = 67.67f;
    inline float Silent_FovColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline int Silent_AimPart;
    inline int Silent_FovSpinDirection;
    inline int Silent_FovSpinSpeed = 1;
    inline ImGuiKey Silent_Key = ImGuiKey_Q;
    inline ImKeyBindMode Silent_Mode = ImKeyBindMode_Toggle;

    inline bool Settings_Team_Check;
    inline bool Settings_Client_Check;
    inline bool Settings_Streamproof;
    inline int Settings_Performance_Mode = 2;
    inline ImGuiKey Settings_Menu_Keybind = ImGuiKey_Insert;

    // Low=60, Medium=144, High=240
    constexpr inline int PerfMode_FPS[] = { 60, 144, 240 };

    inline bool Whitelist_Enabled;
    inline std::unordered_set<std::uint64_t> Whitelist_UserIDs;
    inline float Whitelist_Color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };

    inline bool Triggerbot_Enabled;
    inline bool Triggerbot_KnockedCheck;
    inline bool Triggerbot_WallCheck;
    inline int Triggerbot_Delay;
    inline int Triggerbot_HitPart = 3;
    inline int Triggerbot_Randomize;
    inline int Triggerbot_FireMode;
    inline int Triggerbot_Fov = 150;
    inline ImGuiKey Triggerbot_Key = ImGuiKey_None;
    inline ImKeyBindMode Triggerbot_Mode = ImKeyBindMode_Hold;

    inline bool Misc_Speed = false;
    inline float Misc_Speed_Value = 50.0f;
    inline ImGuiKey Misc_Speed_Key = ImGuiKey_X;
    inline ImKeyBindMode Misc_Speed_Key_Mode = ImKeyBindMode_Toggle;

    inline bool Misc_Jump = false;
    inline float Misc_Jump_Value = 50.0f;
    inline ImGuiKey Misc_Jump_Key = ImGuiKey_C;
    inline ImKeyBindMode Misc_Jump_Key_Mode = ImKeyBindMode_Toggle;

    inline bool Explorer_Open = false;
}