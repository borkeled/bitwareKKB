#include "../../Engine.h"
#include <Infrastructure/Obfuscation.h>

namespace SDK {

    float Humanoid::Get_Health() const
    {
        OBF_PROLOGUE;
        if (!this->Address)
            return 0.0f;

        union Conv
        {
            std::uint64_t hex;
            float f;
        } EasyConversion;

        uintptr_t healthPtr = g_Memory->Read<uintptr_t>(this->Address + Offsets::Humanoid::Health);
        OBF_JUNK_BLOCK;
        uintptr_t healthRead = g_Memory->Read<uintptr_t>(healthPtr);
        EasyConversion.hex = healthPtr ^ healthRead;

        return EasyConversion.f;
    }

    float Humanoid::Get_MaxHealth() const
    {
        OBF_PROLOGUE;
        if (!this->Address)
            return 0.0f;

        union Conv
        {
            std::uint64_t hex;
            float f;
        } EasyConversion;

        uintptr_t healthPtr = g_Memory->Read<uintptr_t>(this->Address + Offsets::Humanoid::MaxHealth);
        uintptr_t healthRead = g_Memory->Read<uintptr_t>(healthPtr);
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
        EasyConversion.hex = healthPtr ^ healthRead;

        return EasyConversion.f;
    }

    void Humanoid::Kill() const
    {
        OBF_PROLOGUE;
        if (!this->Address)
            return;

        g_Memory->Write<uintptr_t>(this->Address + Offsets::Humanoid::Health, 0);
    }

    int Humanoid::Get_RigType() const
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
        if (!this->Address)
            return 0;

        return g_Memory->Read<int>(this->Address + Offsets::Humanoid::RigType);
    }
}
