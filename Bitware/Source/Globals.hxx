#pragma once
#include <memory>
#include <vector>
#include <unordered_set>
#include <atomic>

#include "Engine/Engine.h"
#include "Core/Features/Cache/Cache.h"
#include "ImGui/imgui.h"
#include "Core/Settings/Settings.h"

namespace Perf {
    inline std::atomic<std::uint64_t> FrameTimeUs{ 0 };
    inline std::atomic<std::uint64_t> VisualsTimeUs{ 0 };
    inline std::atomic<std::uint64_t> AimbotTimeUs{ 0 };
    inline std::atomic<std::uint64_t> TriggerbotTimeUs{ 0 };
    inline std::atomic<std::uint64_t> CacheTimeUs{ 0 };
    inline std::atomic<std::uint64_t> PresentTimeUs{ 0 };

    inline std::atomic<int> FrameCount{ 0 };
    inline std::atomic<int> LastFPS{ 0 };

    inline bool ShowOverlay = false;
}

namespace Globals {

	inline HWND RobloxWindow = nullptr;
	inline std::atomic<std::uint64_t> FrameCount{ 0 };

	inline SDK::Datamodel Datamodel;
	inline std::uint64_t GameID;
	inline SDK::VisualEngine VisualEngine;
	inline SDK::Player LocalPlayer;
	inline SDK::Players Players;
	inline SDK::Datamodel Workspace;
	inline SDK::Lighting Lighting;
	inline SDK::Renderview Renderview;
	inline SDK::Camera Camera;

	inline std::shared_ptr<const std::vector<SDK::Player>> Player_Cache;

	namespace Settings {

        inline bool& Team_Check = SettingsStore::Settings_Team_Check;
        inline bool& Client_Check = SettingsStore::Settings_Client_Check;
        inline bool& Streamproof = SettingsStore::Settings_Streamproof;
        inline bool& VSync = SettingsStore::Settings_VSync;
        inline int& Performance_Mode = SettingsStore::Settings_Performance_Mode;
        inline int& WallCheck_Method = SettingsStore::WallCheck_Method;
        inline ImGuiKey& Menu_Keybind = SettingsStore::Settings_Menu_Keybind;
	}

	namespace World {

		inline bool& Skybox = SettingsStore::World_Skybox;
		inline bool& Rotate = SettingsStore::World_Rotate;
		inline bool& Ambience = SettingsStore::World_Ambience;
		inline bool& Fog = SettingsStore::World_Fog;
		inline bool& Brightness = SettingsStore::World_Brightness;
		inline bool& Exposure = SettingsStore::World_Exposure;
		inline bool& FOV = SettingsStore::World_FOV;

		inline int& Skybox_Type = SettingsStore::World_Skybox_Type;
		inline float& Skybox_Rotate_Speed = SettingsStore::World_Rotate_Speed;
		inline float& Fog_Distance = SettingsStore::World_Fog_Distance;
		inline float& FOV_Distance = SettingsStore::World_FOV_Distance;

		inline float& ExposureI = SettingsStore::World_ExposureI;
		inline float& BrightnessI = SettingsStore::World_BrightnessI;

		namespace Colors {

			inline float (&Ambience)[4] = SettingsStore::World_AmbienceColor;
			inline float (&Fog)[4] = SettingsStore::World_FogColor;
		}
	}

	namespace Aimbot {

		inline bool& useFov = SettingsStore::Aimbot_useFov;

		inline bool& Enabled = SettingsStore::Aimbot_Enabled;
		inline bool& DrawFov = SettingsStore::Aimbot_DrawFov;
		inline bool& FovSpin = SettingsStore::Aimbot_FovSpin;
		inline bool& ClosestPlayerFound = SettingsStore::Aimbot_ClosestPlayerFound;
		inline bool& FillFov = SettingsStore::Aimbot_FillFov;
		inline bool& AimbotSticky = SettingsStore::Aimbot_AimbotSticky;
		inline bool& Shake = SettingsStore::Aimbot_Shake;
        inline bool& KnockedCheck = SettingsStore::Aimbot_KnockedCheck;
        inline bool& WallCheck = SettingsStore::Aimbot_WallCheck;

        inline float& ShakeX = SettingsStore::Aimbot_ShakeX;
		inline float& ShakeY = SettingsStore::Aimbot_ShakeY;
		inline float& ShakeZ = SettingsStore::Aimbot_ShakeZ;

		inline SDK::Instance& AimTarget = SettingsStore::Aimbot_AimTarget;
		inline float& FovSize = SettingsStore::Aimbot_FovSize;

		inline ImGuiKey& FovToggleKey = SettingsStore::Aimbot_FovToggleKey;
		inline ImKeyBindMode& FovToggleMode = SettingsStore::Aimbot_FovToggleMode;

		inline int& HitPart = SettingsStore::Aimbot_HitPart;
		inline int& Aimbot_type = SettingsStore::Aimbot_type;
		inline int& FovSpinDirection = SettingsStore::Aimbot_FovSpinDirection;
		inline int& FovSpinSpeed = SettingsStore::Aimbot_FovSpinSpeed;

		inline float (&FovColor)[4] = SettingsStore::Aimbot_FovColor;

		inline ImGuiKey& Aimbot_Key = SettingsStore::Aimbot_Key;
		inline ImKeyBindMode& Aimbot_Mode = SettingsStore::Aimbot_Mode;


		namespace Camera {
			inline float& Smoothing_X = SettingsStore::Aimbot_Camera_SmoothingX;
			inline float& Smoothing_Y = SettingsStore::Aimbot_Camera_SmoothingY;
		}

		namespace Mouse {
			inline float& Smoothing_X = SettingsStore::Aimbot_Mouse_SmoothingX;
			inline float& Smoothing_Y = SettingsStore::Aimbot_Mouse_SmoothingY;

			inline float& Mouse_Sensitivty = SettingsStore::Aimbot_Mouse_Sensitivity;
		}
	}

	namespace Silent
	{
		inline bool& DrawFov = SettingsStore::Silent_DrawFov;
		inline bool& Enabled = SettingsStore::Silent_Enabled;
		inline bool& StickyAim = SettingsStore::Silent_StickyAim;
		inline bool& SpoofMouse = SettingsStore::Silent_SpoofMouse;
		inline bool& UseFov = SettingsStore::Silent_UseFov;
        inline bool& KnockedCheck = SettingsStore::Silent_KnockedCheck;
        inline bool& WallCheck = SettingsStore::Silent_WallCheck;
        inline bool& GunBasedFov = SettingsStore::Silent_GunBasedFov;
		inline float& Fov = SettingsStore::Silent_Fov;
		inline float& FovDoubleBarrel = SettingsStore::Silent_FovDoubleBarrel;
		inline float& FovTacticalShotgun = SettingsStore::Silent_FovTacticalShotgun;
		inline float& FovRevolver = SettingsStore::Silent_FovRevolver;
		inline ImGuiKey& Silent_Key = SettingsStore::Silent_Key;
		inline ImKeyBindMode& Silent_Mode = SettingsStore::Silent_Mode;
		inline int& AimPart = SettingsStore::Silent_AimPart;
		inline int& FovSpinDirection = SettingsStore::Silent_FovSpinDirection;
		inline int& FovSpinSpeed = SettingsStore::Silent_FovSpinSpeed;
		inline float (&FovColor)[4] = SettingsStore::Silent_FovColor;
		inline bool& FovSpin = SettingsStore::Silent_FovSpin;
		inline bool& FillFov = SettingsStore::Silent_FillFov;
	}

	namespace Visuals {

		inline bool& Enabled = SettingsStore::Visuals_Enabled;
		inline bool& Box = SettingsStore::Visuals_Box;
		inline bool& Box_Fill = SettingsStore::Visuals_Box_Fill;
		inline bool& Box_Fill_Gradient = SettingsStore::Visuals_Box_Fill_Gradient;
		inline bool& Box_Fill_Gradient_Rotate = SettingsStore::Visuals_Box_Fill_Gradient_Rotate;
		inline bool& Healthbar = SettingsStore::Visuals_Healthbar;
		inline bool& Health = SettingsStore::Visuals_Health;
		inline bool& Name = SettingsStore::Visuals_Name;
		inline bool& Distance = SettingsStore::Visuals_Distance;
		inline bool& Rig_Type = SettingsStore::Visuals_Rig_Type;
		inline bool& Tool = SettingsStore::Visuals_Tool;
		inline bool& Skeleton = SettingsStore::Visuals_Skeleton;
		inline bool& Chams = SettingsStore::Visuals_Chams;
		inline bool& ChamsFade = SettingsStore::Visuals_ChamsFade;
		inline bool& WallCheck = SettingsStore::Visuals_WallCheck;

		inline float& Render_Distance = SettingsStore::Visuals_Render_Distance;

		inline ImGuiKey& ToggleKey = SettingsStore::Visuals_ToggleKey;
		inline ImKeyBindMode& ToggleMode = SettingsStore::Visuals_ToggleMode;

		inline int& ChamsFadeSpeed = SettingsStore::Visuals_ChamsFadeSpeed;
		inline int& BoxFillSpeed = SettingsStore::Visuals_BoxFillSpeed;
		inline int& Healthbar_Type = SettingsStore::Visuals_Healthbar_Type;
		inline int& Box_Type = SettingsStore::Visuals_Box_Type;
		inline int& Box_Fill_Type = SettingsStore::Visuals_Box_Fill_Type;
		inline int& Name_Type = SettingsStore::Visuals_Name_Type;
		inline int& Gap = SettingsStore::Visuals_Gap;
		inline int& Thickness = SettingsStore::Visuals_Thickness;

		namespace Colors {

			inline float (&Box)[4] = SettingsStore::Visuals_BoxColor;
			inline float (&BoxFill_Top)[4] = SettingsStore::Visuals_BoxFillTop;
			inline float (&BoxFill_Bottom)[4] = SettingsStore::Visuals_BoxFillBottom;
			inline float (&Healthbar)[4] = SettingsStore::Visuals_HealthbarColor;
			inline float (&Name)[4] = SettingsStore::Visuals_NameColor;
			inline float (&Distance)[4] = SettingsStore::Visuals_DistanceColor;
			inline float (&Rig_Type)[4] = SettingsStore::Visuals_RigTypeColor;
			inline float (&Tool)[4] = SettingsStore::Visuals_ToolColor;
			inline float (&Skeleton)[4] = SettingsStore::Visuals_SkeletonColor;
			inline float (&Chams)[4] = SettingsStore::Visuals_ChamsColor;
			inline float (&ChamsOutline)[4] = SettingsStore::Visuals_ChamsOutline;
			inline float (&Healthbar_Top)[4] = SettingsStore::Visuals_HealthbarTop;
			inline float (&Healthbar_Middle)[4] = SettingsStore::Visuals_HealthbarMiddle;
			inline float (&Healthbar_Bottom)[4] = SettingsStore::Visuals_HealthbarBottom;
			inline float (&Health)[4] = SettingsStore::Visuals_HealthColor;
			inline float (&Occluded)[4] = SettingsStore::Visuals_OccludedColor;
		}
	}

	namespace Whitelist {
		inline bool& Enabled = SettingsStore::Whitelist_Enabled;
		inline std::unordered_set<std::uint64_t>& UserIDs = SettingsStore::Whitelist_UserIDs;
		inline float (&Color)[4] = SettingsStore::Whitelist_Color;
	}

	namespace Triggerbot {

		inline bool& Enabled = SettingsStore::Triggerbot_Enabled;
        inline bool& KnockedCheck = SettingsStore::Triggerbot_KnockedCheck;
        inline bool& WallCheck = SettingsStore::Triggerbot_WallCheck;
        inline int& Delay = SettingsStore::Triggerbot_Delay;
		inline int& HitPart = SettingsStore::Triggerbot_HitPart;
		inline int& Randomize = SettingsStore::Triggerbot_Randomize;
		inline int& FireMode = SettingsStore::Triggerbot_FireMode;
		inline int& Triggerbot_Fov = SettingsStore::Triggerbot_Fov;
		inline ImGuiKey& Triggerbot_Key = SettingsStore::Triggerbot_Key;
		inline ImKeyBindMode& Triggerbot_Mode = SettingsStore::Triggerbot_Mode;
	}

    namespace Misc {

        inline bool& Speed = SettingsStore::Misc_Speed;
        inline float& Speed_Value = SettingsStore::Misc_Speed_Value;
        inline ImGuiKey& Speed_Key = SettingsStore::Misc_Speed_Key;
        inline ImKeyBindMode& Speed_Key_Mode = SettingsStore::Misc_Speed_Key_Mode;

        inline bool& Jump = SettingsStore::Misc_Jump;
        inline float& Jump_Value = SettingsStore::Misc_Jump_Value;
        inline ImGuiKey& Jump_Key = SettingsStore::Misc_Jump_Key;
        inline ImKeyBindMode& Jump_Key_Mode = SettingsStore::Misc_Jump_Key_Mode;
    }

    namespace Explorer {
        inline bool& Open = SettingsStore::Explorer_Open;
    }
}

#include <Core/Features/Explorer/Explorer.h>

inline Explorer::Explorer g_Explorer;