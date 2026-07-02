#include "../../Engine.h"
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace SDK {

    std::string Instance::Name() const {
        if (!Address) return std::string(skCrypt("?"));
        uintptr_t StringPointer = Driver->Read<uintptr_t>(Address + Offsets::Instance::Name);
        if (!StringPointer) return std::string(skCrypt("?"));
        return Driver->Read_String(StringPointer);
    }

    std::string Instance::Text() const {
        return Driver->Read_String(this->Address + Offsets::GuiObject::Text);
    }

    std::string Instance::Class() const {
        if (!Address) return std::string(skCrypt("?"));
        uintptr_t Descriptor = Driver->Read<uintptr_t>(Address + Offsets::Instance::ClassDescriptor);
        if (!Descriptor) return std::string(skCrypt("?"));
        uintptr_t StringPointer = Driver->Read<uintptr_t>(Descriptor + 0x8);
        if (!StringPointer) return std::string(skCrypt("?"));
        return Driver->Read_String(StringPointer);
    }

    Instance Instance::Parent() const {
        if (!Address) return Instance();
        return Driver->Read<Instance>(Address + Offsets::Instance::Parent);
    }

    std::vector<Instance> Instance::Children() const {
        if (!Address) return {};

        auto Start = Driver->Read<uintptr_t>(Address + Offsets::Instance::ChildrenStart);
        if (!Start) return {};

        auto End = Driver->Read<uintptr_t>(Start + Offsets::Instance::ChildrenEnd);
        std::vector<Instance> Children;
        Children.reserve(32);
        int walked = 0;
        for (auto instances = Driver->Read<uintptr_t>(Start); instances != End && walked < 10000; instances += 16, ++walked)
            Children.push_back(Driver->Read<Instance>(instances));

        return Children;
    }

    Instance Instance::Find_First_Child(const std::string& Name) const {
        if (!Address || Name.empty()) return Instance();

        for (const auto& Child : Children())
        {
            if (!Child.Address) continue;
            if (Child.Name() == Name) return Child;
        }

        return Instance();
    }

    Instance Instance::Find_First_Child_Of_Class(const std::string& Class_Name) const {
        if (!Address || Class_Name.empty()) return Instance();

        for (const auto& Child : Children())
        {
            if (!Child.Address) continue;
            if (Child.Class() == Class_Name) return Child;
        }

        return Instance();
    }
}
