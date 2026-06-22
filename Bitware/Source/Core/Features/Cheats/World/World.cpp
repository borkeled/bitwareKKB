#include "Engine/Engine.h"
#include "Globals.hxx"
#include "Core/Features/Cache/Cache.h"
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace World {
    std::atomic<bool> StopSkybox{ false };
    std::atomic<bool> StopAtmosphere{ false };
    std::atomic<bool> StopFog{ false };
    std::atomic<bool> StopBrightness{ false };
    std::atomic<bool> StopExposure{ false };
    std::atomic<bool> StopFOV{ false };

    void SkyboxChanger() {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        while (!StopSkybox)
        {
            OBF_JUNK_BLOCK;
            if (Globals::World::Skybox)
            {
                auto Sky = Globals::Lighting.Find_First_Child(std::string(skCrypt("Sky")).c_str());
                if (Sky.Address)
                {
                    OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

                    if (Globals::World::Skybox_Type == 0)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://12635309703")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://12635311686")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://12635312870")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://12635313718")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://12635315817")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://12635316856")));
                    }
                    else if (Globals::World::Skybox_Type == 1)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://12064107")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://12064152")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://12064121")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://12063984")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://12064115")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://12064131")));
                    }
                    else if (Globals::World::Skybox_Type == 2)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://271042516")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://271077243")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://271042556")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://271042310")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://271042467")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://271077958")));
                    }
                    else if (Globals::World::Skybox_Type == 3)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://1876545003")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://1876544331")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://1876542941")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://1876543392")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://1876543764")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://1876544642")));
                    }
                    else if (Globals::World::Skybox_Type == 4)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://116758234")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://116758314")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://116758367")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://116758446")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://116758478")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://116758496")));
                    }
                    else if (Globals::World::Skybox_Type == 5)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://1233158420")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://1233158838")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://1233157105")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://1233157640")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://1233157995")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://1233159158")));
                    }
                    else if (Globals::World::Skybox_Type == 6)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://1327358")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://1327359")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://1327355")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://1327357")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://1327356")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://1327360")));
                    }
                    else if (Globals::World::Skybox_Type == 7)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://570555736")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://570555964")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://570555800")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://570555840")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://570555882")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://570555929")));
                    }
                    else if (Globals::World::Skybox_Type == 8)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://95020137072033")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://92862258103959")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://107665368823185")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://126542804346203")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://103716549795832")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://131036626982613")));
                    }
                    else if (Globals::World::Skybox_Type == 9)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://169210090")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://169210108")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://169210121")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://169210133")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://169210143")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://169210149")));
                    }
                    else if (Globals::World::Skybox_Type == 10)
                    {
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://47974894")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://47974690")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://47974821")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://47974776")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://47974859")));
                        g_Memory->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://47974909")));
                    }

                    if (Globals::World::Rotate) {
                        auto Sky = Globals::Lighting.Find_First_Child_Of_Class(std::string(skCrypt("Sky")).c_str());
                        static float rotY = 0.0f;
                        rotY = (0.0f > 360.0f) ? 0.0f : rotY + Globals::World::Skybox_Rotate_Speed;
                        g_Memory->Write<SDK::Vector3>(Sky.Address + Offsets::Sky::SkyboxOrientation, { 0.0f, rotY, 0.0f });
                    }

                    SDK::Renderview::InvalidateLighting();
                    SDK::Renderview::ValidateSkybox();
                }
            }
            OBF_JUNK_BLOCK;
            SDK::sleep_jitter(30, 10);
        }
    }

    void AtmosphereChanger() {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        while (!StopAtmosphere)
        {
            if (Globals::World::Ambience)
            {
                SDK::Lighting::SetAmbient(Globals::Lighting.Address, {Globals::World::Colors::Ambience[0], Globals::World::Colors::Ambience[1], Globals::World::Colors::Ambience[2]} );
                SDK::Renderview::InvalidateLighting();
            }
            OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
            SDK::sleep_jitter(100, 20);
        }
    }

    void FogChanger() {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        while (!StopFog)
        {
            if (Globals::World::Fog)
            {
                SDK::Lighting::SetFog(Globals::Lighting.Address, Globals::World::Fog_Distance, {Globals::World::Colors::Fog[0], Globals::World::Colors::Fog[1], Globals::World::Colors::Fog[2]} );
                SDK::Renderview::InvalidateLighting();
            }
            OBF_JUNK_BLOCK;
            SDK::sleep_jitter(100, 20);
        }
    }

    void BrightnessChanger() {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        while (!StopBrightness)
        {
            if (Globals::World::Brightness)
            {
                SDK::Lighting::SetBrightness(Globals::Lighting.Address, Globals::World::BrightnessI);
                SDK::Renderview::InvalidateLighting();
            }
            OBF_JUNK_BLOCK;
            SDK::sleep_jitter(100, 20);
        }
    }

    void ExposureChanger() {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        while (!StopExposure)
        {
            if (Globals::World::Exposure)
            {
                SDK::Lighting::SetExposure(Globals::Lighting.Address, Globals::World::ExposureI);
                SDK::Renderview::InvalidateLighting();
            }
            OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
            SDK::sleep_jitter(100, 20);
        }
    }

    void FOVChanger() {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
        static float LastFOVValue = -1.0f;

        while (!StopFOV)
        {
            if (Globals::World::FOV)
            {
                float CurrentFOV = Globals::World::FOV_Distance;
                if (CurrentFOV != LastFOVValue)
                {
                    SDK::Lighting::SetFOV(Globals::Camera.Address, CurrentFOV);
                    LastFOVValue = CurrentFOV;
                }
            }
            OBF_JUNK_BLOCK;
            SDK::sleep_jitter(30, 10);
        }
    }

    void RunService() {
        OBF_PROLOGUE;
        StopSkybox = true; StopAtmosphere = true; StopFog = true;
        StopBrightness = true; StopExposure = true; StopFOV = true;

        SDK::sleep_jitter(50, 10);

        StopSkybox = false; StopAtmosphere = false; StopFog = false;
        StopBrightness = false; StopExposure = false; StopFOV = false;

        std::thread(SkyboxChanger).detach();
        std::thread(AtmosphereChanger).detach();
        std::thread(FogChanger).detach();
        std::thread(BrightnessChanger).detach();
        std::thread(ExposureChanger).detach();
        std::thread(FOVChanger).detach();
    }
}
