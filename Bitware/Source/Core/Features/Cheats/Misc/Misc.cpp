#include "Engine/Engine.h"
#include "Globals.hxx"
#include "Core/Features/Cache/Cache.h"
#include <Infrastructure/Obfuscation.h>

#include <windows.h>
#include <thread>
#include <chrono>

#include <Core/Input/InputHook.h>

namespace Misc {

    static bool IsSpeedActiveByKeybind()
    {
        int Vk = ImGuiKeyToVK(Globals::Misc::Speed_Key);
        if (!Vk) return true;
        bool Pressed = InputHook::IsKeyDown(Vk);
        if (Globals::Misc::Speed_Key_Mode == ImKeyBindMode_Toggle)
        {
            static bool spdToggled = false;
            static bool spdLastPressed = false;
            if (Pressed && !spdLastPressed) spdToggled = !spdToggled;
            spdLastPressed = Pressed;
            return spdToggled;
        }
        return Pressed;
    }

    static bool IsJumpActiveByKeybind()
    {
        int Vk = ImGuiKeyToVK(Globals::Misc::Jump_Key);
        if (!Vk) return true;
        bool Pressed = InputHook::IsKeyDown(Vk);
        if (Globals::Misc::Jump_Key_Mode == ImKeyBindMode_Toggle)
        {
            static bool jmpToggled = false;
            static bool jmpLastPressed = false;
            if (Pressed && !jmpLastPressed) jmpToggled = !jmpToggled;
            jmpLastPressed = Pressed;
            return jmpToggled;
        }
        return Pressed;
    }

    static void Speedhack()
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        if (!Globals::LocalPlayer.Humanoid.Address)
        {
            OBF_JUNK_BLOCK;
            return;
        }

        float targetSpeed = Globals::Misc::Speed_Value;
        OBF_OPAQUE_FALSE { OBF_JUNK_BLOCK; }

        for (int i = 0; i < 3; i++)
        {
            Driver->Write<float>(Globals::LocalPlayer.Humanoid.Address + Offsets::Humanoid::Walkspeed, targetSpeed);
            Driver->Write<float>(Globals::LocalPlayer.Humanoid.Address + Offsets::Humanoid::WalkspeedCheck, targetSpeed);
        }
    }

    static void Jumphack()
    {
        OBF_PROLOGUE;

        if (!Globals::LocalPlayer.Humanoid.Address)
        {
            OBF_JUNK_BLOCK;
            return;
        }

        static bool wasActive = false;
        static float originalJumpPower = 50.0f;
        static bool capturedOriginal = false;

        auto humanoidAddr = Globals::LocalPlayer.Humanoid.Address;

        if (Globals::Misc::Jump)
        {
            if (!capturedOriginal)
            {
                originalJumpPower = Driver->Read<float>(humanoidAddr + Offsets::Humanoid::JumpPower);
                capturedOriginal = true;
            }

            Driver->Write<bool>(humanoidAddr + Offsets::Humanoid::UseJumpPower, true);
            Driver->Write<float>(humanoidAddr + Offsets::Humanoid::JumpPower, Globals::Misc::Jump_Value);
            wasActive = true;
        }
        else if (wasActive)
        {
            Driver->Write<float>(humanoidAddr + Offsets::Humanoid::JumpPower, originalJumpPower);
            wasActive = false;
            capturedOriginal = false;
        }
    }

    static bool speedWasActive = false;
    static bool jumpWasActive = false;

    void Speed(std::stop_token st)
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        for (; !st.stop_requested(); )
        {
            if (Globals::Misc::Speed && IsSpeedActiveByKeybind())
            {
                Speedhack();
                speedWasActive = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
            else
            {
                if (speedWasActive)
                {
                    if (Globals::LocalPlayer.Humanoid.Address)
                    {
                        Driver->Write<float>(Globals::LocalPlayer.Humanoid.Address + Offsets::Humanoid::Walkspeed, 16.0f);
                        Driver->Write<float>(Globals::LocalPlayer.Humanoid.Address + Offsets::Humanoid::WalkspeedCheck, 16.0f);
                    }
                    speedWasActive = false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    void Jump(std::stop_token st)
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        for (; !st.stop_requested(); )
        {
            SDK::sleep_jitter(10, 5);

            if (Globals::Misc::Jump && IsJumpActiveByKeybind())
            {
                Jumphack();
                jumpWasActive = true;
            }
            else if (jumpWasActive)
            {
                if (Globals::LocalPlayer.Humanoid.Address)
                {
                    Driver->Write<float>(Globals::LocalPlayer.Humanoid.Address + Offsets::Humanoid::JumpPower, 50.0f);
                }
                jumpWasActive = false;
            }
        }
    }

    void RunService(std::stop_token st)
    {
        OBF_PROLOGUE;
        std::thread(Speed, st).detach();
        std::thread(Jump, st).detach();
        OBF_JUNK_BLOCK;
    }
}