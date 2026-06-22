#include <Windows.h>
#include <thread>
#include <cmath>
#include <string>
#include <algorithm>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>


#include "Aimbot.h"
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

namespace Aimbot {
    static char CurrentLockedName[64] = { 0 };
    static SDK::Vector3 AimPositionW = { 0, 0, 0 };
    static SDK::Vector2 AimPositionS = { 0, 0 };
    static std::atomic<bool> TargetFound{ false };

    static uintptr_t PersistenceAddress = 0;
    static std::atomic<bool> IsPersisting{ false };

    static uintptr_t LastHitboxTargetAddr = 0;
    static int CurrentTargetHitbox = 0;

    static SDK::Vector3 Cross(const SDK::Vector3& A, const SDK::Vector3& B) {
        return { A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x };
    }

    static int GetRandomizedHitbox(uintptr_t TargetAddr, int ConfigHitbox) {
        OBF_PROLOGUE;
        if (ConfigHitbox != 4) {
            OBF_JUNK_BLOCK;
            return ConfigHitbox;
        }
        if (TargetAddr != LastHitboxTargetAddr) {
            LastHitboxTargetAddr = TargetAddr;
            CurrentTargetHitbox = (int)(SDK::xorshift64() % 3);
        }
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
        return CurrentTargetHitbox;
    }

    static SDK::Matrix3 ForwardToMatrix(const SDK::Vector3& Forward) {
        OBF_PROLOGUE;
        SDK::Vector3 UpRef = { 0.f, 1.f, 0.f };
        if (std::abs(Forward.x) < 0.0001f && std::abs(Forward.z) < 0.0001f) {
            UpRef = { 0.f, 0.f, 1.f };
        }

        SDK::Vector3 Right = Cross(UpRef, Forward).normalize();
        SDK::Vector3 Up = Cross(Forward, Right);

        SDK::Matrix3 M;
        M.data[0] = -Right.x; M.data[1] = Up.x; M.data[2] = -Forward.x;
        M.data[3] = Right.y;  M.data[4] = Up.y; M.data[5] = -Forward.y;
        M.data[6] = -Right.z; M.data[7] = Up.z; M.data[8] = -Forward.z;
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
        return M;
    }

    static SDK::Matrix3 LookAtToMatrix(const SDK::Vector3& CamPos, const SDK::Vector3& TargetPos) {
        OBF_PROLOGUE;
        SDK::Vector3 Forward = (TargetPos - CamPos).normalize();
        SDK::Vector3 UpRef = { 0.f, 1.f, 0.f };
        if (std::abs(Forward.x) < 0.0001f && std::abs(Forward.z) < 0.0001f) {
            UpRef = { 0.f, 0.f, 1.f };
        }

        SDK::Vector3 Right = Cross(UpRef, Forward).normalize();
        SDK::Vector3 Up = Cross(Forward, Right);

        SDK::Matrix3 M;
        M.data[0] = -Right.x; M.data[1] = Up.x; M.data[2] = -Forward.x;
        M.data[3] = Right.y;  M.data[4] = Up.y; M.data[5] = -Forward.y;
        M.data[6] = -Right.z; M.data[7] = Up.z; M.data[8] = -Forward.z;
        return M;
    }

    static SDK::Vector3 GetTargetBonePos(const SDK::Player& Plr, int HitboxIdx) {
        OBF_PROLOGUE;
        uintptr_t targetAddr = 0;
        if (HitboxIdx == 0) targetAddr = Plr.Head.Address;
        else if (HitboxIdx == 1) targetAddr = Plr.Torso.Address;
        else targetAddr = Plr.HumanoidRootPart.Address;

        if (!targetAddr) return SDK::Vector3{};
        uintptr_t primAddr = g_Memory->Read<uintptr_t>(targetAddr + Offsets::BasePart::Primitive);
        if (!primAddr) return SDK::Vector3{};
        return g_Memory->Read<SDK::Vector3>(primAddr + Offsets::Primitive::Position);
    }

    void UpdateAimbot() {
        OBF_PROLOGUE;
        if (!TargetFound) {
            OBF_JUNK_BLOCK;
            return;
        }

        OBF_OPAQUE_FALSE {
            volatile int x = 0;
            if (x) TargetFound = false;
        }

        if (Globals::Aimbot::Aimbot_type == 1) {
            if (!Globals::Camera.Address) {
                CurrentLockedName[0] = '\0';
                IsPersisting = false; PersistenceAddress = 0;
                Globals::Aimbot::AimTarget = SDK::Instance(0);
                return;
            }
            SDK::Camera Cam(Globals::Camera.Address);
            auto CamPos = Cam.Get_CameraPos();
            if (std::isnan(CamPos.x) || std::isnan(CamPos.y) || std::isnan(CamPos.z)) {
                CurrentLockedName[0] = '\0';
                IsPersisting = false; PersistenceAddress = 0;
                Globals::Aimbot::AimTarget = SDK::Instance(0);
                return;
            }
            auto TargetPos = AimPositionW;
            if (Globals::Aimbot::Shake) {
                TargetPos.x += (SDK::fast_rand_float() * 2.f - 1.f) * Globals::Aimbot::ShakeX;
                TargetPos.y += (SDK::fast_rand_float() * 2.f - 1.f) * Globals::Aimbot::ShakeY;
                TargetPos.z += (SDK::fast_rand_float() * 2.f - 1.f) * Globals::Aimbot::ShakeZ;
            }
            float SmoothFactor = Globals::Aimbot::Camera::Smoothing_X / 100.f;
            SmoothFactor = std::pow(SmoothFactor, 1.2f);
            SDK::Matrix3 CurrentMatrix = Cam.Get_CameraRot();

            SDK::Vector3 CurrentForward = { -CurrentMatrix.data[2], -CurrentMatrix.data[5], -CurrentMatrix.data[8] };
            SDK::Vector3 TargetForward = (TargetPos - CamPos).normalize();

            float T = 1.f - SmoothFactor;
            SDK::Vector3 InterpForward = (CurrentForward + (TargetForward - CurrentForward) * T).normalize();

            SDK::Matrix3 FinalMatrix = ForwardToMatrix(InterpForward);
            Cam.Set_CameraRot(FinalMatrix);
        }
        else {
            POINT CursorPos;
            if (!Api::GetCursorPos(&CursorPos)) return;
            if (Globals::RobloxWindow) Api::ScreenToClient(Globals::RobloxWindow, &CursorPos);

            float RawDeltaX = AimPositionS.x - CursorPos.x;
            float RawDeltaY = AimPositionS.y - CursorPos.y;
            float Dist = std::sqrt(RawDeltaX * RawDeltaX + RawDeltaY * RawDeltaY);

            float Sensitivity = Globals::Aimbot::Mouse::Mouse_Sensitivty;
            if (Globals::Aimbot::Mouse::Smoothing_X > 0) {
                float SmoothVal = Globals::Aimbot::Mouse::Smoothing_X;
                if (SmoothVal < 1.f) SmoothVal = 1.f;
                if (SmoothVal > 100.f) SmoothVal = 100.f;
                Sensitivity /= SmoothVal;
            }
            float MoveX = RawDeltaX * Sensitivity;
            float MoveY = RawDeltaY * Sensitivity;

            if (Dist < 80.f && Dist > 0.1f) {
                float Damping = Dist / 80.f;
                Damping = (Damping * Damping) * 0.85f + 0.15f;
                MoveX *= Damping;
                MoveY *= Damping;
            }

            if (Globals::Aimbot::Shake) {
                MoveX += (SDK::fast_rand_float() * 2.f - 1.f) * Globals::Aimbot::ShakeX;
                MoveY += (SDK::fast_rand_float() * 2.f - 1.f) * Globals::Aimbot::ShakeY;
            }
            if (MoveX < -45.f) MoveX = -45.f; if (MoveX > 45.f) MoveX = 45.f;
            if (MoveY < -45.f) MoveY = -45.f; if (MoveY > 45.f) MoveY = 45.f;
            MoveX += (SDK::fast_rand_float() * 2.f - 1.f) * 0.5f;
            MoveY += (SDK::fast_rand_float() * 2.f - 1.f) * 0.5f;
            LONG FinalMoveX = (LONG)std::round(MoveX);
            LONG FinalMoveY = (LONG)std::round(MoveY);
            if (FinalMoveX == 0 && std::abs(MoveX) > 0.05f) FinalMoveX = (MoveX > 0.f) ? 1 : -1;
            if (FinalMoveY == 0 && std::abs(MoveY) > 0.05f) FinalMoveY = (MoveY > 0.f) ? 1 : -1;
            if (FinalMoveX != 0 || FinalMoveY != 0) {
                INPUT Input = {};
                Input.type = INPUT_MOUSE;
                Input.mi.dx = FinalMoveX; Input.mi.dy = FinalMoveY;
                Input.mi.dwFlags = MOUSEEVENTF_MOVE;
                Input.mi.dwExtraInfo = SDK::xorshift64();
                Api::SendInput(1, &Input, sizeof(INPUT));
            }
        }
    }

    void AcquireTarget();

    void RunService() {
        std::thread([]() {
            timeBeginPeriod(1);
            bool Toggled = false;
            bool LastPressed = false;
            std::uint64_t StoredGameID = 0;
            auto LastTick = std::chrono::high_resolution_clock::now();
            const std::chrono::microseconds TickInterval(1000000 / 144);

            while (true) {
                OBF_PROLOGUE;
                if (!Globals::Aimbot::Enabled) {
                    SDK::sleep_jitter(50, 15);
                    OBF_JUNK_BLOCK;
                    continue;
                }
                auto Now = std::chrono::high_resolution_clock::now();
                auto Elapsed = std::chrono::duration_cast<std::chrono::microseconds>(Now - LastTick);
                if (Elapsed < TickInterval) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                    OBF_STALL;
                    continue;
                }
                LastTick = Now;
                int Vk = ImGuiKeyToVK(Globals::Aimbot::Aimbot_Key);
                if (!Vk) { OBF_JUNK_BLOCK; continue; }
                bool Pressed = InputHook::IsKeyDown(Vk);

                if (Globals::Aimbot::Aimbot_Mode == ImKeyBindMode_Toggle) {
                    if (Pressed && !LastPressed) Toggled = !Toggled;
                    if (Toggled) { AcquireTarget(); UpdateAimbot(); }
                    else { CurrentLockedName[0] = '\0'; IsPersisting = false; PersistenceAddress = 0; Globals::Aimbot::AimTarget = SDK::Instance(0); }
                } else {
                    if (Pressed) { AcquireTarget(); UpdateAimbot(); }
                    else { CurrentLockedName[0] = '\0'; IsPersisting = false; PersistenceAddress = 0; Globals::Aimbot::AimTarget = SDK::Instance(0); }
                }
                LastPressed = Pressed;
                if (StoredGameID != 0 && Globals::GameID != StoredGameID) {
                    StoredGameID = Globals::GameID;
                    CurrentLockedName[0] = '\0'; IsPersisting = false; PersistenceAddress = 0;
                    Globals::Aimbot::AimTarget = SDK::Instance(0);
                }
            }
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

        if (Globals::Aimbot::AimbotSticky && IsPersisting && PersistenceAddress != 0) {
            std::vector<SDK::Player> Snapshot;
            { std::lock_guard<std::mutex> Lock(Cache::Cache_Mutex); Snapshot = Globals::Player_Cache; }
            bool StickyFoundThisFrame = false;
            for (auto& Plr : Snapshot) {
                if (Plr.Character.Address != PersistenceAddress) continue;
                if (Plr.Local_Player || !Plr.Character.Address || !Plr.Head.Address) break;
                if (Globals::Aimbot::KnockedCheck && PlayerUtils::IsPlayerKnocked(Plr)) break;
                int HitboxIdx = GetRandomizedHitbox(Plr.Character.Address, Globals::Aimbot::HitPart);
                if (HitboxIdx > 2) HitboxIdx = 0;
                SDK::Vector3 BonePos = GetTargetBonePos(Plr, HitboxIdx);
                if (std::isnan(BonePos.x) || (BonePos.x == 0 && BonePos.y == 0)) break;
                if (Globals::Aimbot::WallCheck && !wallcheck->is_visible(CameraOrigin, BonePos)) break;
                SDK::VisualEngine Ve(Globals::VisualEngine.Address);
                auto ScreenPos = Ve.World_To_Screen(BonePos);
                AimPositionW = BonePos; AimPositionS = { ScreenPos.x, ScreenPos.y };
                strncpy_s(CurrentLockedName, Plr.Name.c_str(), _TRUNCATE);
                Globals::Aimbot::AimTarget = SDK::Instance(Plr.Character.Address);
                TargetFound = true;
                StickyFoundThisFrame = true;
                break;
            }
            if (StickyFoundThisFrame) return;

            IsPersisting = false;
            PersistenceAddress = 0;
            Globals::Aimbot::AimTarget = SDK::Instance(0);
        }

        std::vector<SDK::Player> Snapshot;
        { std::lock_guard<std::mutex> Lock(Cache::Cache_Mutex); Snapshot = Globals::Player_Cache; }
        for (auto& Plr : Snapshot) {
            if (Plr.Local_Player || !Plr.Character.Address || !Plr.Head.Address) continue;
            if (Plr.Distance > 700.f) continue;
            int HitboxIdx = GetRandomizedHitbox(Plr.Character.Address, Globals::Aimbot::HitPart);
            if (HitboxIdx > 2) HitboxIdx = 0;
            SDK::Vector3 BonePos = GetTargetBonePos(Plr, HitboxIdx);
            if (std::isnan(BonePos.x) || (BonePos.x == 0 && BonePos.y == 0)) continue;
            SDK::Vector3 DirToBone = (BonePos - CameraOrigin).normalize();
            float Dot = DirToBone.x * CamForward.x + DirToBone.y * CamForward.y + DirToBone.z * CamForward.z;
            if (Dot <= 0.0f) continue;
            float Dist3D = (CameraOrigin - BonePos).magnitude();
            if (Dist3D > 700.f) continue;
            SDK::Vector2 ScreenPos = Globals::VisualEngine.World_To_Screen(BonePos);
            float Dist2DSqr = (ScreenPos.x - CursorPos.x) * (ScreenPos.x - CursorPos.x) + (ScreenPos.y - CursorPos.y) * (ScreenPos.y - CursorPos.y);
            if (Globals::Aimbot::useFov) { float MaxFovSqr = Globals::Aimbot::FovSize * Globals::Aimbot::FovSize; if (Dist2DSqr > MaxFovSqr) continue; }
            if (Globals::Aimbot::KnockedCheck && PlayerUtils::IsPlayerKnocked(Plr)) continue;
            if (Globals::Aimbot::WallCheck && !wallcheck->is_visible(CameraOrigin, BonePos)) continue;
            if (Dist2DSqr < ClosestDistanceSqr) {
                ClosestDistanceSqr = Dist2DSqr;
                AimPositionW = BonePos; AimPositionS = { ScreenPos.x, ScreenPos.y };
                BestName = Plr.Name; BestCharacterAddr = Plr.Character.Address;
                TargetFound = true;
            }
        }
        if (TargetFound) {
            strncpy_s(CurrentLockedName, BestName.c_str(), _TRUNCATE);
            Globals::Aimbot::AimTarget = SDK::Instance(BestCharacterAddr);
            if (Globals::Aimbot::AimbotSticky && !IsPersisting) {
                PersistenceAddress = BestCharacterAddr;
                IsPersisting = true;
            }
        }
    }
}
