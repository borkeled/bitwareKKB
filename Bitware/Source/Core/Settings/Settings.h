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
    inline ImGuiKey Aimbot_Key = ImGuiKey_None;
    inline ImKeyBindMode Aimbot_Mode = ImKeyBindMode_Hold;
    inline float Aimbot_Mouse_SmoothingX = 1.0f;
    inline float Aimbot_Mouse_SmoothingY = 1.0f;
    inline float Aimbot_Mouse_Sensitivity = 1.0f;
    inline float Aimbot_Camera_SmoothingX = 1.0f;
    inline float Aimbot_Camera_SmoothingY = 1.0f;
    inline SDK::Instance Aimbot_AimTarget;

    inline bool Silent_Enabled;
    inline bool Silent_DrawFov;
    inline bool Silent_FovSpin;
    inline bool Silent_FillFov;
    inline bool Silent_useFov;
    inline bool Silent_TeamCheck;
    inline bool Silent_KnockedCheck;
    inline bool Silent_WallCheck;
    inline bool Silent_ClosestPlayerFound;
    inline int Silent_type;
    inline int Silent_HitPart;
    inline int Silent_FovSpinDirection;
    inline int Silent_FovSpinSpeed;
    inline float Silent_FovSize = 150.0f;
    inline float Silent_FovColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    inline ImGuiKey Silent_Key = ImGuiKey_None;
    inline ImKeyBindMode Silent_Mode = ImKeyBindMode_Hold;
    inline float Silent_Mouse_SmoothingX = 1.0f;
    inline float Silent_Mouse_SmoothingY = 1.0f;
    inline float Silent_Mouse_Sensitivity = 1.0f;
    inline float Silent_Camera_SmoothingX = 1.0f;
    inline float Silent_Camera_SmoothingY = 1.0f;
    inline ImGuiKey Silent_FovToggleKey = ImGuiKey_None;
    inline ImKeyBindMode Silent_FovToggleMode = ImKeyBindMode_Toggle;

    inline bool Silent_Prediction_Enabled;
    inline float Silent_Prediction_Value = 0.1f;

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
    inline int Visuals_Chams_Type = 0;
    inline bool Visuals_Chams_Fill_Enabled = true;
    inline float Visuals_Chams_Fill_Transparency = 0.82f;
    inline bool Visuals_Chams_Outline_Enabled = false;
    inline float Visuals_Chams_Outline_Thickness = 1.04f;
    inline float Visuals_Chams_Outline_Width = 1.0f;
    inline int Visuals_Chams_Quality = 1;
    inline bool Visuals_Chams_Occlusion = false;
    inline float Visuals_Chams_Visible_Color[4] = { 0.2f, 1.f, 0.35f, 1.f };
    inline float Visuals_Chams_Occluded_Color[4] = { 1.f, 0.2f, 0.2f, 1.f };
    inline bool Visuals_Chams_Use_Shaders = false;
    inline int Visuals_Chams_Shader = 0;
    inline float Visuals_Chams_Cycle_Speed = 1.5f;
    inline float Visuals_Chams_Rim_Power = 2.5f;
    inline float Visuals_Chams_Dissolve_Amount = 0.5f;
    inline float Visuals_Chams_Dissolve_Edge = 0.05f;
    inline float Visuals_Chams_Glitch_Intensity = 0.4f;
    inline float Visuals_Chams_Glitch_Speed = 3.0f;
    inline float Visuals_Chams_Hologram_Scan_Speed = 2.0f;
    inline float Visuals_Chams_Hologram_Opacity = 0.7f;
    inline float Visuals_Chams_Lava_Speed = 1.0f;
    inline float Visuals_Chams_Lava_Scale = 1.0f;
    inline float Visuals_Chams_Ice_Roughness = 0.3f;
    inline float Visuals_Chams_Neon_Pulse_Speed = 3.0f;
    inline float Visuals_Chams_Neon_Pulse_Width = 0.5f;
    inline float Visuals_Chams_Depth_Fog_Density = 1.0f;
    inline float Visuals_Chams_Splatter_Scale = 1.0f;
    inline float Visuals_Chams_Liquid_Speed = 1.5f;
    inline float Visuals_Chams_Liquid_Wave = 0.3f;
    inline float Visuals_Chams_Sliced_Gap = 0.15f;
    inline float Visuals_Chams_Sliced_Speed = 1.0f;
    inline float Visuals_Chams_Caustic_Speed = 1.0f;
    inline float Visuals_Chams_Caustic_Scale = 1.0f;
    inline float Visuals_Chams_Metallic_Roughness = 0.3f;
    inline float Visuals_Chams_Chrome_Sharpness = 2.0f;
    inline float Visuals_Chams_Wood_Scale = 1.0f;
    inline float Visuals_Chams_Plastic_Shininess = 4.0f;
    inline float Visuals_Chams_Specular_Power = 32.0f;
    inline float Visuals_Chams_Specular_Intensity = 0.6f;
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

    inline bool Settings_Team_Check;
    inline bool Settings_Client_Check;
    inline bool Settings_Streamproof;
    inline bool Settings_VSync = false;
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
    inline ImGuiKey Misc_Speed_Key = ImGuiKey_None;
    inline ImKeyBindMode Misc_Speed_Key_Mode = ImKeyBindMode_Toggle;

    inline bool Misc_Jump = false;
    inline float Misc_Jump_Value = 50.0f;
    inline ImGuiKey Misc_Jump_Key = ImGuiKey_None;
    inline ImKeyBindMode Misc_Jump_Key_Mode = ImKeyBindMode_Toggle;

    inline bool Explorer_Open = false;

    inline float AccentColor[4] = { 0.012f, 0.976f, 0.502f, 1.0f };
}