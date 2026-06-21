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

namespace Triggerbot {

    static SDK::Vector3 GetTargetBonePos(const SDK::Player& Plr, int HitboxIdx) {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
        uintptr_t targetAddr = 0;
        if (HitboxIdx == 0) targetAddr = Plr.Head.Address;
        else if (HitboxIdx == 1) targetAddr = Plr.Torso.Address;
        else targetAddr = Plr.HumanoidRootPart.Address;

        if (!targetAddr) return SDK::Vector3{};

        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

        uintptr_t primAddr = Driver->Read<uintptr_t>(targetAddr + Offsets::BasePart::Primitive);
        if (!primAddr) return SDK::Vector3{};

        return Driver->Read<SDK::Vector3>(primAddr + Offsets::Primitive::Position);
    }

    void RunService() {
        std::thread([]() {
            OBF_PROLOGUE;
            OBF_JUNK_DECLARE;
            timeBeginPeriod(1);

            bool Holding = false;
            auto LastTick = std::chrono::high_resolution_clock::now();
            auto LastFireTime = std::chrono::steady_clock::now();
            const std::chrono::microseconds TickInterval(1000000 / 144);
            const std::chrono::milliseconds TapCooldown(1000);

            while (true) {
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

                auto Now = std::chrono::high_resolution_clock::now();
                auto Elapsed = std::chrono::duration_cast<std::chrono::microseconds>(Now - LastTick);

                if (Elapsed < TickInterval) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                    continue;
                }
                LastTick = Now;

                int Vk = ImGuiKeyToVK(Globals::Triggerbot::Triggerbot_Key);
                if (!Vk || !InputHook::IsKeyDown(Vk)) {
                    if (Holding) {
                        INPUT Up = {}; Up.type = INPUT_MOUSE; Up.mi.dwFlags = MOUSEEVENTF_LEFTUP; Up.mi.dwExtraInfo = SDK::xorshift64();
                        Api::SendInput(1, &Up, sizeof(INPUT));
                        Holding = false;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
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

                std::vector<SDK::Player> Snapshot;
                {
                    std::lock_guard<std::mutex> Lock(Cache::Cache_Mutex);
                    Snapshot = Globals::Player_Cache;
                }

                bool TargetFound = false;

                OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

                for (auto& Plr : Snapshot) {
                    if (Plr.Local_Player || !Plr.Character.Address || !Plr.Head.Address) continue;

                    if (Plr.Distance > 700.f) continue;

                    if (Globals::Triggerbot::KnockedCheck && PlayerUtils::IsPlayerKnocked(Plr)) continue;

                    int HitboxIdx = Globals::Triggerbot::HitPart;
                    int BoneIndices[3];
                    int BoneCount;

                    if (HitboxIdx == 3)
                    {
                        BoneIndices[0] = 0; BoneIndices[1] = 1; BoneIndices[2] = 2;
                        BoneCount = 3;
                    }
                    else
                    {
                        if (HitboxIdx > 2) HitboxIdx = 0;
                        BoneIndices[0] = HitboxIdx;
                        BoneCount = 1;
                    }

                    for (int i = 0; i < BoneCount; i++)
                    {
                        SDK::Vector3 BonePos = GetTargetBonePos(Plr, BoneIndices[i]);
                        if (std::isnan(BonePos.x) || (BonePos.x == 0 && BonePos.y == 0)) continue;

                        if (Globals::Triggerbot::WallCheck && !wallcheck->is_visible(CameraOrigin, BonePos)) continue;

                        SDK::Vector2 ScreenPos = Ve.World_To_Screen(BonePos);

                        float DistSqr = (ScreenPos.x - ScreenCenterX) * (ScreenPos.x - ScreenCenterX) +
                                        (ScreenPos.y - ScreenCenterY) * (ScreenPos.y - ScreenCenterY);

                        if (DistSqr < 25.0f) {
                            TargetFound = true;
                            break;
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
            }

            timeEndPeriod(1);
        }).detach();
    }
}
