#include "../../Engine.h"
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace SDK {

    std::string Instance::Name() const {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
        if (!Address) return std::string(skCrypt("?"));
        uintptr_t StringPointer = g_Memory->Read<uintptr_t>(Address + Offsets::Instance::Name);
        if (!StringPointer) return std::string(skCrypt("?"));
        return g_Memory->Read_String(StringPointer);
    }

    std::string Instance::Text() const {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
        return g_Memory->Read_String(this->Address + Offsets::GuiObject::Text);
    }

    std::string Instance::Class() const {
        OBF_PROLOGUE;
        if (!Address) return std::string(skCrypt("?"));
        uintptr_t Descriptor = g_Memory->Read<uintptr_t>(Address + Offsets::Instance::ClassDescriptor);
        if (!Descriptor) return std::string(skCrypt("?"));
        uintptr_t StringPointer = g_Memory->Read<uintptr_t>(Descriptor + 0x8);
        if (!StringPointer) return std::string(skCrypt("?"));
        return g_Memory->Read_String(StringPointer);
    }

    Instance Instance::Parent() const {
        OBF_PROLOGUE;
        if (!Address) return Instance();
        return g_Memory->Read<Instance>(Address + Offsets::Instance::Parent);
    }

    std::vector<Instance> Instance::Children() const {
        OBF_PROLOGUE;
        std::vector<Instance> Container;
        if (!Address) return Container;

        auto Start = g_Memory->Read<uintptr_t>(Address + Offsets::Instance::ChildrenStart);
        if (!Start) return Container;

        auto End = g_Memory->Read<uintptr_t>(Start + Offsets::Instance::ChildrenEnd);
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
        for (auto instances = g_Memory->Read<uintptr_t>(Start); instances != End; instances += 16)
            Container.emplace_back(g_Memory->Read<Instance>(instances));

        return Container;
    }

    Instance Instance::Find_First_Child(const std::string& Name) const {
        OBF_PROLOGUE;
        if (!Address || Name.empty()) return Instance();

        for (Instance Child : Children())
        {
            if (!Child.Address) continue;
            OBF_JUNK_BLOCK;
            if (Child.Name() == Name) return Child;
        }

        return Instance();
    }

    Instance Instance::Find_First_Child_Of_Class(const std::string& Class_Name) const {
        OBF_PROLOGUE;
        if (!Address || Class_Name.empty()) return Instance();

        for (Instance Child : Children())
        {
            if (!Child.Address) continue;
            if (Child.Class() == Class_Name) return Child;
        }

        return Instance();
    }
}
