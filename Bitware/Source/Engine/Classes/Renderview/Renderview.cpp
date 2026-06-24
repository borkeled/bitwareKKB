#include "../../Engine.h"
#include <Globals.hxx>
#include <Infrastructure/Obfuscation.h>

namespace SDK {

    std::uint64_t Renderview::GetRenderview() {
        OBF_PROLOGUE;
        auto Rv1 = Driver->Read<std::uintptr_t>(Globals::Datamodel.Address + Offsets::RenderJob::RenderView);
        OBF_JUNK_BLOCK;
        auto Rv2 = Driver->Read<std::uintptr_t>(Rv1 + Offsets::DataModel::ToRenderView2);
        auto Rv3 = Driver->Read<std::uintptr_t>(Rv2 + Offsets::DataModel::ToRenderView3);
        return Rv3;
    }

    void Renderview::InvalidateLighting() {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
        Driver->Write<bool>(Globals::Renderview.GetRenderview() + 0x148, false);
    }

    void Renderview::ValidateSkybox() {
        // stub: SkyValidAlt not in auto dump
    }
}
