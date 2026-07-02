#include <Windows.h>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <random>

#include "Triggerbot.h"
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
#include "../Visuals/Visuals.h"

namespace Triggerbot {

    static SDK::Vector3 GetTargetBonePos(const SDK::Player& Plr, int HitboxIdx) {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
        uintptr_t targetAddr = 0;
        if (HitboxIdx == 0) targetAddr = Plr.Head.Address;
        else if (HitboxIdx == 1) targetAddr = Plr.Torso.Address;
        else targetAddr = Plr.HumanoidRootPart.Address;

        if (!targetAddr) return SDK::Vector3{};

        for (int ci = 0; ci < Plr.CachedBoneCount; ++ci) {
            if (Plr.CachedBones[ci].InstanceAddress == targetAddr) {
                auto pa = Plr.CachedBones[ci].PrimitiveAddress;
                if (pa) return Driver->Read<SDK::Vector3>(pa + Offsets::Primitive::Position);
                break;
            }
        }

        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

        uintptr_t primAddr = Driver->Read<uintptr_t>(targetAddr + Offsets::BasePart::Primitive);
        if (!primAddr) return SDK::Vector3{};

        return Driver->Read<SDK::Vector3>(primAddr + Offsets::Primitive::Position);
    }

    static SDK::Vector3 GetBonePosition(const SDK::Player& Plr, std::uint64_t addr) {
        if (!addr) return SDK::Vector3{};
        for (int ci = 0; ci < Plr.CachedBoneCount; ++ci) {
            if (Plr.CachedBones[ci].InstanceAddress == addr) {
                auto pa = Plr.CachedBones[ci].PrimitiveAddress;
                if (pa) return Driver->Read<SDK::Vector3>(pa + Offsets::Primitive::Position);
                break;
            }
        }
        uintptr_t primAddr = Driver->Read<uintptr_t>(addr + Offsets::BasePart::Primitive);
        if (!primAddr) return SDK::Vector3{};
        return Driver->Read<SDK::Vector3>(primAddr + Offsets::Primitive::Position);
    }

    void RunService(std::stop_token st) {
        std::thread([st]() {
            OBF_PROLOGUE;
            OBF_JUNK_DECLARE;

            bool Holding = false;
            static bool Toggled = false;
            static bool LastPressed = false;
            auto LastTick = std::chrono::steady_clock::now();
            auto LastFireTime = std::chrono::steady_clock::now();
            const std::chrono::microseconds TickInterval(1000000 / 60);
            const std::chrono::milliseconds TapCooldown(1000);

            while (!st.stop_requested()) {
                OBF_JUNK_BLOCK;
                if (!Globals::Triggerbot::Enabled) {
                    if (Holding) {
                        INPUT Up = {}; Up.type = INPUT_MOUSE; Up.mi.dwFlags = MOUSEEVENTF_LEFTUP; Up.mi.dwExtraInfo = SDK::xorshift64();
                        Api::SendInput(1, &Up, sizeof(INPUT));
                        Holding = false;
                    }
                    SDK::sleep_jitter(50, 10);
                    continue;
                }

                auto iterStart = std::chrono::steady_clock::now();
                auto NextTick = LastTick + TickInterval;
                auto Now = std::chrono::steady_clock::now();
                if (Now < NextTick) {
                    std::this_thread::sleep_until(NextTick);
                }
                LastTick = std::chrono::steady_clock::now();

                if (Globals::Triggerbot::Triggerbot_Mode != ImKeyBindMode_Always) {
                    int Vk = ImGuiKeyToVK(Globals::Triggerbot::Triggerbot_Key);
                    if (!Vk) {
                        if (Holding) {
                            INPUT Up = {}; Up.type = INPUT_MOUSE; Up.mi.dwFlags = MOUSEEVENTF_LEFTUP; Up.mi.dwExtraInfo = SDK::xorshift64();
                            Api::SendInput(1, &Up, sizeof(INPUT));
                            Holding = false;
                        }
                        SDK::sleep_jitter(50, 10);
                        continue;
                    }
                    bool Pressed = InputHook::IsKeyDown(Vk);

                    if (Globals::Triggerbot::Triggerbot_Mode == ImKeyBindMode_Toggle) {
                        if (Pressed && !LastPressed) Toggled = !Toggled;
                        if (!Toggled) {
                            if (Holding) {
                                INPUT Up = {}; Up.type = INPUT_MOUSE; Up.mi.dwFlags = MOUSEEVENTF_LEFTUP; Up.mi.dwExtraInfo = SDK::xorshift64();
                                Api::SendInput(1, &Up, sizeof(INPUT));
                                Holding = false;
                            }
                            SDK::sleep_jitter(50, 10);
                            continue;
                        }
                    } else {
                        if (!Pressed) {
                            if (Holding) {
                                INPUT Up = {}; Up.type = INPUT_MOUSE; Up.mi.dwFlags = MOUSEEVENTF_LEFTUP; Up.mi.dwExtraInfo = SDK::xorshift64();
                                Api::SendInput(1, &Up, sizeof(INPUT));
                                Holding = false;
                            }
                            SDK::sleep_jitter(50, 10);
                            continue;
                        }
                    }
                    LastPressed = Pressed;
                }

                if (!Globals::VisualEngine.Address || !Globals::Camera.Address) {
                    SDK::sleep_jitter(10, 5);
                    continue;
                }

                SDK::VisualEngine Ve(Globals::VisualEngine.Address);

                SDK::Vector3 CameraOrigin{};
                if (Globals::Camera.Address != 0) {
                    SDK::Camera Cam(Globals::Camera.Address);
                    CameraOrigin = Cam.Get_CameraPos();
                }

                RECT ClientRect;
                if (!Api::GetClientRect(Globals::RobloxWindow, &ClientRect)) {
                    continue;
                }
                float ScreenCenterX = (ClientRect.right - ClientRect.left) * 0.5f;
                float ScreenCenterY = (ClientRect.bottom - ClientRect.top) * 0.5f;

                std::shared_ptr<const std::vector<SDK::Player>> Snapshot;
                {
                    std::lock_guard<std::mutex> Lock(Cache::Cache_Mutex);
                    Snapshot = Globals::Player_Cache;
                }
                if (!Snapshot) continue;

                bool TargetFound = false;

                for (auto& Plr : *Snapshot) {
                    if (Plr.Local_Player || !Plr.Character.Address || !Plr.Head.Address) continue;

                    if (Plr.Distance > 700.f) continue;

                    if (Globals::Triggerbot::KnockedCheck && PlayerUtils::IsPlayerDead(Plr)) continue;

                    int HitboxIdx = Globals::Triggerbot::HitPart;

                    if (HitboxIdx == 3)
                    {
                        auto& Bones = Visuals::Get_Bones(Plr);
                        for (auto* Inst : Bones)
                        {
                            SDK::Vector3 BonePos = GetBonePosition(Plr, Inst->Address);
                            if (std::isnan(BonePos.x) || (BonePos.x == 0 && BonePos.y == 0)) continue;

                            SDK::Vector2 ScreenPos = Ve.World_To_Screen(BonePos);

                            float DistSqr = (ScreenPos.x - ScreenCenterX) * (ScreenPos.x - ScreenCenterX) +
                                            (ScreenPos.y - ScreenCenterY) * (ScreenPos.y - ScreenCenterY);

                            if (DistSqr >= 25.0f) continue;

                            if (Globals::Triggerbot::WallCheck && !wallcheck->is_visible(CameraOrigin, BonePos)) continue;

                            TargetFound = true;
                            break;
                        }
                    }
                    else
                    {
                        int BoneIdx = HitboxIdx > 2 ? 0 : HitboxIdx;

                        SDK::Vector3 BonePos = GetTargetBonePos(Plr, BoneIdx);
                        if (!std::isnan(BonePos.x) && !(BonePos.x == 0 && BonePos.y == 0))
                        {
                            if (!Globals::Triggerbot::WallCheck || wallcheck->is_visible(CameraOrigin, BonePos))
                            {
                                SDK::Vector2 ScreenPos = Ve.World_To_Screen(BonePos);

                                float DistSqr = (ScreenPos.x - ScreenCenterX) * (ScreenPos.x - ScreenCenterX) +
                                                (ScreenPos.y - ScreenCenterY) * (ScreenPos.y - ScreenCenterY);

                                if (DistSqr < 25.0f) {
                                    TargetFound = true;
                                }
                            }
                        }
                    }

                    if (TargetFound) break;
                }

                int FireMode = Globals::Triggerbot::FireMode;

                if (FireMode == 0)
                {
                    auto CooldownElapsed = std::chrono::steady_clock::now() - LastFireTime;
                    if (TargetFound && CooldownElapsed >= TapCooldown)
                    {
                        int DelayMs = Globals::Triggerbot::Delay;
                        int Randomize = Globals::Triggerbot::Randomize;
                        if (Randomize > 0)
                        {
                            static std::mt19937 Rng(std::random_device{}());
                            std::uniform_int_distribution<int> Dist(-Randomize, Randomize);
                            int Variation = Dist(Rng);
                            DelayMs = (std::max)(DelayMs + Variation, 0);
                        }
                        if (DelayMs > 0) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(DelayMs));
                        }

                        INPUT Down = {}; Down.type = INPUT_MOUSE; Down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; Down.mi.dwExtraInfo = SDK::xorshift64();
                        Api::SendInput(1, &Down, sizeof(INPUT));
                        std::this_thread::sleep_for(std::chrono::milliseconds(1 + (SDK::xorshift64() % 5)));
                        INPUT Up = {}; Up.type = INPUT_MOUSE; Up.mi.dwFlags = MOUSEEVENTF_LEFTUP; Up.mi.dwExtraInfo = SDK::xorshift64();
                        Api::SendInput(1, &Up, sizeof(INPUT));

                        LastFireTime = std::chrono::steady_clock::now();
                    }
                }
                else
                {
                    if (TargetFound)
                    {
                        if (!Holding)
                        {
                            int DelayMs = Globals::Triggerbot::Delay;
                            int Randomize = Globals::Triggerbot::Randomize;
                            if (Randomize > 0)
                            {
                                static std::mt19937 Rng(std::random_device{}());
                                std::uniform_int_distribution<int> Dist(-Randomize, Randomize);
                                int Variation = Dist(Rng);
                                DelayMs = (std::max)(DelayMs + Variation, 0);
                            }
                            if (DelayMs > 0) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(DelayMs));
                            }

                            INPUT Down = {}; Down.type = INPUT_MOUSE; Down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; Down.mi.dwExtraInfo = SDK::xorshift64();
                            Api::SendInput(1, &Down, sizeof(INPUT));
                            Holding = true;
                        }
                    }
                    else
                    {
                        if (Holding)
                        {
                            INPUT Up = {}; Up.type = INPUT_MOUSE; Up.mi.dwFlags = MOUSEEVENTF_LEFTUP; Up.mi.dwExtraInfo = SDK::xorshift64();
                            Api::SendInput(1, &Up, sizeof(INPUT));
                            Holding = false;
                        }
                    }
                }

                auto iterEnd = std::chrono::steady_clock::now();
                Perf::TriggerbotTimeUs.store(std::chrono::duration_cast<std::chrono::microseconds>(iterEnd - iterStart).count());
            }
        }).detach();
    }
}
