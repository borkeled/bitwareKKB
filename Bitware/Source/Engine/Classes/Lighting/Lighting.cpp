#include "../../Engine.h"
#include <Globals.hxx>
#include <Infrastructure/Obfuscation.h>

namespace SDK {

    void Lighting::SetAmbient(uintptr_t LightingAddress, SDK::Vector3 Color)
    {
        OBF_PROLOGUE;
        g_Memory->Write<SDK::Vector3>(LightingAddress + Offsets::Lighting::Ambient, Color);
        g_Memory->Write<SDK::Vector3>(LightingAddress + Offsets::Lighting::OutdoorAmbient, Color);
        OBF_JUNK_BLOCK;
    }

    void Lighting::SetFog(uintptr_t LightingAddress, float End, SDK::Vector3 Color)
    {
        OBF_PROLOGUE;
        g_Memory->Write<float>(LightingAddress + Offsets::Lighting::FogEnd, End);
        g_Memory->Write<SDK::Vector3>(LightingAddress + Offsets::Lighting::FogColor, Color);
        OBF_JUNK_DECLARE;
    }

    void Lighting::SetBrightness(uintptr_t LightingAddress, float Value)
    {
        OBF_PROLOGUE;
        g_Memory->Write<float>(LightingAddress + Offsets::Lighting::Brightness, Value);
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
    }

    void Lighting::SetExposure(uintptr_t LightingAddress, float Value)
    {
        OBF_PROLOGUE;
        g_Memory->Write<float>(LightingAddress + Offsets::Lighting::ExposureCompensation, Value);
    }

    void Lighting::SetFOV(uintptr_t CameraAddress, float Degrees)
    {
        OBF_PROLOGUE;
        g_Memory->Write<float>(CameraAddress + Offsets::Camera::FieldOfView, Degrees * 0.0174533f);
        OBF_JUNK_BLOCK;
    }
}
