#include <Windows.h>
#include <thread>
#include <cmath>
#include <string>
#include <algorithm>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>


#include "SilentAim.h"
#include "Engine/Math/Math.h"
#include "../../Cache/Cache.h"
#include "../../../../Engine/Engine.h"
#include <Globals.hxx>
#include "../../../../Engine/Offsets/Offsets.h"
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>
#include <Core/Input/InputHook.h>
#include "../Common/PlayerUtils.h"
#include "../Common/WallCheck.h"

namespace SilentAim {
    static SDK::Vector3 AimPositionW = { 0, 0, 0 };
    static SDK::Vector2 AimPositionS = { 0, 0 };
    static std::atomic<bool> TargetFound{ false };

    static SDK::ViewPort OriginalViewport = {};
    static bool OriginalViewportSaved = false;

    static void SaveOriginalViewport() {
        OBF_PROLOGUE;
        if (Globals::Camera.Address && !OriginalViewportSaved) {
            OriginalViewport = Driver->Read<SDK::ViewPort>(Globals::Camera.Address + Offsets::Camera::Viewport);
            OriginalViewportSaved = true;
        }
    }

    static void RestoreOriginalViewport() {
        OBF_PROLOGUE;
        if (Globals::Camera.Address && OriginalViewportSaved) {
            SDK::Camera Cam(Globals::Camera.Address);
            Driver->Write<SDK::ViewPort>(Cam.Address + Offsets::Camera::Viewport, OriginalViewport);
        }
    }

    static void ResetViewport() {
        RestoreOriginalViewport();
        OriginalViewportSaved = false;
    }

    static SDK::Vector3 GetTargetBonePos(const SDK::Player& Plr, int HitboxIdx) {
        OBF_PROLOGUE;
        uintptr_t targetAddr = 0;
        if (HitboxIdx == 0) targetAddr = Plr.Head.Address;
        else if (HitboxIdx == 1) targetAddr = Plr.Torso.Address;
        else targetAddr = Plr.HumanoidRootPart.Address;

        if (!targetAddr) return SDK::Vector3{};
        uintptr_t primAddr = Driver->Read<uintptr_t>(targetAddr + Offsets::BasePart::Primitive);
        if (!primAddr) return SDK::Vector3{};
        return Driver->Read<SDK::Vector3>(primAddr + Offsets::Primitive::Position);
    }

    void AcquireTarget();

    void UpdateSilentAim() {
        OBF_PROLOGUE;
        if (!TargetFound) {
            OBF_JUNK_BLOCK;
            RestoreOriginalViewport();
            return;
        }

        if (!Globals::Camera.Address) return;
        SDK::Camera Cam(Globals::Camera.Address);
        SaveOriginalViewport();

        SDK::VisualEngine Ve(Globals::VisualEngine.Address);
        auto ScreenSize = Ve.Get_Dimensions();
        if (ScreenSize.x <= 0 || ScreenSize.y <= 0) return;

        auto vp = Cam.FetchViewPort(AimPositionS, ScreenSize);

        Cam.SetViewPort({ static_cast<unsigned short>(vp.x), static_cast<unsigned short>(vp.y) });
    }

    void RunService(std::stop_token st) {
        std::thread([st]() {
            timeBeginPeriod(1);
            bool Toggled = false;
            bool LastPressed = false;

            while (!st.stop_requested()) {
                OBF_PROLOGUE;
                if (!Globals::Silent::Enabled) {
                    ResetViewport();
                    SDK::sleep_jitter(50, 15);
                    OBF_JUNK_BLOCK;
                    continue;
                }

                if (Globals::Silent::Silent_Mode == ImKeyBindMode_Always) {
                    AcquireTarget();
                    UpdateSilentAim();
                } else if (Globals::Silent::Silent_Key == ImGuiKey_None) {
                    AcquireTarget();
                    UpdateSilentAim();
                } else {
                    int Vk = ImGuiKeyToVK(Globals::Silent::Silent_Key);
                    if (!Vk) { OBF_JUNK_BLOCK; ResetViewport(); SDK::sleep_jitter(50, 15); continue; }
                    bool Pressed = InputHook::IsKeyDown(Vk);

                    if (Globals::Silent::Silent_Mode == ImKeyBindMode_Toggle) {
                        if (Pressed && !LastPressed) Toggled = !Toggled;
                        if (Toggled) { AcquireTarget(); UpdateSilentAim(); }
                        else { ResetViewport(); SDK::sleep_jitter(50, 15); }
                    } else {
                        if (Pressed) { AcquireTarget(); UpdateSilentAim(); }
                        else { ResetViewport(); SDK::sleep_jitter(50, 15); }
                    }
                    LastPressed = Pressed;
                }
            }
            ResetViewport();
            timeEndPeriod(1);
        }).detach();
    }

    void AcquireTarget() {
        OBF_PROLOGUE;
        TargetFound = false;
        if (!Globals::VisualEngine.Address) return;

        POINT CursorPos;
        if (!Api::GetCursorPos(&CursorPos)) return;
        if (Globals::RobloxWindow) Api::ScreenToClient(Globals::RobloxWindow, &CursorPos);

        float ClosestDistanceSqr = 999999.f * 999999.f;
        std::string BestName = "";
        uintptr_t BestCharacterAddr = 0;

        if (!Globals::Camera.Address) return;
        SDK::Camera Cam(Globals::Camera.Address);
        auto CameraOrigin = Cam.Get_CameraPos();
        if (std::isnan(CameraOrigin.x) || std::isnan(CameraOrigin.y) || std::isnan(CameraOrigin.z)) return;
        SDK::Matrix3 CamRot = Cam.Get_CameraRot();
        SDK::Vector3 CamForward = { -CamRot.data[2], -CamRot.data[5], -CamRot.data[8] };

        std::shared_ptr<const std::vector<SDK::Player>> Snapshot;
        { std::lock_guard<std::mutex> Lock(Cache::Cache_Mutex); Snapshot = Globals::Player_Cache; }
        if (!Snapshot) return;

        for (auto& Plr : *Snapshot) {
            if (Plr.Local_Player || !Plr.Character.Address || !Plr.Head.Address) continue;
            if (Plr.Distance > 700.f) continue;

            if (Globals::Silent::TeamCheck && Globals::LocalPlayer.Team.Address != 0 && Plr.Team.Address != 0) {
                if (Globals::LocalPlayer.Team.Address == Plr.Team.Address) continue;
            }

            int HitboxIdx = Globals::Silent::HitPart;
            if (HitboxIdx > 2) HitboxIdx = 0;
            SDK::Vector3 BonePos = GetTargetBonePos(Plr, HitboxIdx);
            if (std::isnan(BonePos.x) || (BonePos.x == 0 && BonePos.y == 0)) continue;

            if (Globals::Silent::Prediction::Enabled) {
                uintptr_t predAddr = 0;
                if (HitboxIdx == 0) predAddr = Plr.Head.Address;
                else if (HitboxIdx == 1) predAddr = Plr.Torso.Address;
                else predAddr = Plr.HumanoidRootPart.Address;
                if (predAddr) {
                    uintptr_t primAddr = Driver->Read<uintptr_t>(predAddr + Offsets::BasePart::Primitive);
                    if (primAddr) {
                        SDK::Vector3 Vel = Driver->Read<SDK::Vector3>(primAddr + Offsets::Primitive::AssemblyLinearVelocity);
                        float Dist = BonePos.distance(CameraOrigin);
                        const float ProjectileSpeed = 900.f;
                        float PredictFactor = Globals::Silent::Prediction::Value * (Dist / ProjectileSpeed);
                        BonePos = BonePos + (Vel * PredictFactor);
                    }
                }
            }

            SDK::Vector3 DirToBone = BonePos - CameraOrigin;
            float Dot = DirToBone.x * CamForward.x + DirToBone.y * CamForward.y + DirToBone.z * CamForward.z;
            if (Dot <= 0.0f) continue;

            SDK::Vector2 ScreenPos = Globals::VisualEngine.World_To_Screen(BonePos);
            float Dist2DSqr = (ScreenPos.x - CursorPos.x) * (ScreenPos.x - CursorPos.x) + (ScreenPos.y - CursorPos.y) * (ScreenPos.y - CursorPos.y);

            if (Globals::Silent::useFov) { float MaxFovSqr = Globals::Silent::FovSize * Globals::Silent::FovSize; if (Dist2DSqr > MaxFovSqr) continue; }
            if (Globals::Silent::KnockedCheck && PlayerUtils::IsPlayerKnocked(Plr)) continue;
            if (Globals::Silent::WallCheck && !wallcheck->is_visible(CameraOrigin, BonePos)) continue;

            if (Dist2DSqr < ClosestDistanceSqr) {
                ClosestDistanceSqr = Dist2DSqr;
                AimPositionW = BonePos;
                AimPositionS = { ScreenPos.x, ScreenPos.y };
                BestName = Plr.Name;
                BestCharacterAddr = Plr.Character.Address;
                TargetFound = true;
            }
        }
        if (TargetFound) {
            Globals::Silent::ClosestPlayerFound = true;
        } else {
            Globals::Silent::ClosestPlayerFound = false;
        }
    }
}
