#pragma once

#include <Core/Features/Cache/Cache.h>
#include <Globals.hxx>
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace PlayerUtils {

    inline bool IsPlayerDead(const SDK::Player& Player)
    {
        return Player.Health <= 0.f;
    }

}
