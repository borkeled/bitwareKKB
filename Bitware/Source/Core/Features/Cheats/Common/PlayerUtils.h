#pragma once

#include <Core/Features/Cache/Cache.h>
#include <Globals.hxx>
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace PlayerUtils {

    inline bool IsPlayerKnocked(const SDK::Player& Player)
    {
        if (Player.Character.Address == 0)
            return false;

        SDK::Instance BodyEffects = Player.Character.Find_First_Child(std::string(skCrypt("BodyEffects")));
        if (BodyEffects.Address == 0)
        {
            auto Children = Player.Character.Children();
            for (const auto& Child : Children)
            {
                if (Child.Name() == std::string(skCrypt("BodyEffects")))
                {
                    BodyEffects = Child;
                    break;
                }
            }
            if (BodyEffects.Address == 0)
                return false;
        }

        SDK::Instance Ko = BodyEffects.Find_First_Child(std::string(skCrypt("K.O")));
        if (Ko.Address == 0)
        {
            auto KoChildren = BodyEffects.Children();
            for (const auto& Child : KoChildren)
            {
                if (Child.Name() == std::string(skCrypt("K.O")))
                {
                    Ko = Child;
                    break;
                }
            }
            if (Ko.Address == 0)
                return false;
        }

        if (Ko.Class() != std::string(skCrypt("BoolValue")))
            return false;

        try
        {
            return Driver->Read<bool>(Ko.Address + Offsets::Misc::Value);
        }
        catch (...)
        {
            return false;
        }
    }

}
