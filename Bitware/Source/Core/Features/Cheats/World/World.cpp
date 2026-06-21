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
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://12635309703")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://12635311686")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://12635312870")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://12635313718")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://12635315817")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://12635316856")));
                    }
                    else if (Globals::World::Skybox_Type == 1)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://12064107")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://12064152")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://12064121")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://12063984")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://12064115")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://12064131")));
                    }
                    else if (Globals::World::Skybox_Type == 2)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://271042516")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://271077243")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://271042556")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://271042310")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://271042467")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://271077958")));
                    }
                    else if (Globals::World::Skybox_Type == 3)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://1876545003")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://1876544331")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://1876542941")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://1876543392")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://1876543764")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://1876544642")));
                    }
                    else if (Globals::World::Skybox_Type == 4)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://116758234")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://116758314")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://116758367")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://116758446")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://116758478")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://116758496")));
                    }
                    else if (Globals::World::Skybox_Type == 5)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://1233158420")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://1233158838")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://1233157105")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://1233157640")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://1233157995")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://1233159158")));
                    }
                    else if (Globals::World::Skybox_Type == 6)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://1327358")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://1327359")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://1327355")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://1327357")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://1327356")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://1327360")));
                    }
                    else if (Globals::World::Skybox_Type == 7)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://570555736")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://570555964")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://570555800")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://570555840")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://570555882")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://570555929")));
                    }
                    else if (Globals::World::Skybox_Type == 8)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://95020137072033")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://92862258103959")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://107665368823185")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://126542804346203")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://103716549795832")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://131036626982613")));
                    }
                    else if (Globals::World::Skybox_Type == 9)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://169210090")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://169210108")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://169210121")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://169210133")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://169210143")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://169210149")));
                    }
                    else if (Globals::World::Skybox_Type == 10)
                    {
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxBk, std::string(skCrypt("rbxassetid://47974894")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxDn, std::string(skCrypt("rbxassetid://47974690")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxFt, std::string(skCrypt("rbxassetid://47974821")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxLf, std::string(skCrypt("rbxassetid://47974776")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxRt, std::string(skCrypt("rbxassetid://47974859")));
                        Driver->Write_String(Sky.Address + Offsets::Sky::SkyboxUp, std::string(skCrypt("rbxassetid://47974909")));
                    }

                    if (Globals::World::Rotate) {
                        auto Sky = Globals::Lighting.Find_First_Child_Of_Class(std::string(skCrypt("Sky")).c_str());
                        static float rotY = 0.0f;
                        rotY = (0.0f > 360.0f) ? 0.0f : rotY + Globals::World::Skybox_Rotate_Speed;
                        Driver->Write<SDK::Vector3>(Sky.Address + Offsets::Sky::SkyboxOrientation, { 0.0f, rotY, 0.0f });
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
