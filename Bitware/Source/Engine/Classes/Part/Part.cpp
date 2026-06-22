#include "../../Engine.h"
#include <Infrastructure/Obfuscation.h>

namespace SDK {

    SDK::Vector3 Part::Get_Position() const {
        OBF_PROLOGUE;
        return g_Memory->Read<SDK::Vector3>(Address + Offsets::Primitive::Position);
    }

    SDK::Matrix3 Part::Get_Rotation() const {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
        return g_Memory->Read<SDK::Matrix3>(Address + Offsets::Primitive::Rotation);
    }

    SDK::Vector3 Part::Get_Size() const {
        OBF_PROLOGUE;
        return g_Memory->Read<SDK::Vector3>(Address + Offsets::Primitive::Size);
    }

    SDK::Vector3 Part::Get_MoveDir() const {
        OBF_PROLOGUE;
        return g_Memory->Read<SDK::Vector3>(Address + Offsets::Player::CameraMode);
    }

    SDK::Vector3 Part::Get_Velocity() const {
        OBF_PROLOGUE;
        if (!Address) return {};
        uintptr_t primitive = g_Memory->Read<uintptr_t>(Address + Offsets::BasePart::Primitive);
        if (!primitive) return {};
        OBF_JUNK_DECLARE;
        return g_Memory->Read<SDK::Vector3>(primitive + Offsets::Primitive::AssemblyLinearVelocity);
    }

    bool Part::Get_Anchored() const {
        OBF_PROLOGUE;
        OBF_OPAQUE_FALSE { OBF_JUNK_BLOCK; }
        return g_Memory->Read<bool>(Address + Offsets::PrimitiveFlags::Anchored);
    }

    SDK::Vector3 Part::Get_CFrame() const {
        OBF_PROLOGUE;
        return g_Memory->Read<SDK::Vector3>(Address + Offsets::Primitive::Rotation);
    }

    Part Part::Get_Primitive() const {
        OBF_PROLOGUE;
        uintptr_t primitiveAddress = g_Memory->Read<uintptr_t>(Address + Offsets::BasePart::Primitive);
        return Part{ primitiveAddress };
    }

    SDK::Vector3 Part::Get_PartPosition() const {
        OBF_PROLOGUE;
        Part primitive = Get_Primitive();
        if (!primitive.Address) return {};
        OBF_JUNK_BLOCK;
        return g_Memory->Read<SDK::Vector3>(primitive.Address + Offsets::Primitive::Position);
    }

    float Part::Get_Transparency() const {
        OBF_PROLOGUE;
        return g_Memory->Read<float>(Address + Offsets::BasePart::Transparency);
    }

    void Part::Write_Velocity(const SDK::Vector3& Velocity) const {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
        Part primitive = Get_Primitive();
        if (!primitive.Address) return;
        g_Memory->Write<SDK::Vector3>(primitive.Address + Offsets::Primitive::AssemblyLinearVelocity, Velocity);
    }

    void Part::Set_PartPosition(const SDK::Vector3& Position) const {
        OBF_PROLOGUE;
        Part primitive = Get_Primitive();
        if (!primitive.Address) return;
        g_Memory->Write<SDK::Vector3>(primitive.Address + Offsets::Primitive::Position, Position);
    }

    void Part::Set_Rotation(const SDK::Matrix3& Rotation) const {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
        g_Memory->Write<SDK::Matrix3>(Address + Offsets::Primitive::Rotation, Rotation);
    }

    void Part::Set_MeshID(const std::string& MeshID) const {
        OBF_PROLOGUE;
        g_Memory->Write_String(Address + Offsets::MeshPart::MeshId, MeshID);
    }

    void Part::Set_Transparency(float Transparency) const {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
        g_Memory->Write<float>(Address + Offsets::BasePart::Transparency, Transparency);
    }

    void Part::Write_MoveDir(const SDK::Vector3& MoveDir) const {
        OBF_PROLOGUE;
        g_Memory->Write<SDK::Vector3>(Address + Offsets::Player::CameraMode, MoveDir);
    }
}