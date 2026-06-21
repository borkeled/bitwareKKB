#pragma once

#include <Core/Features/Cache/Cache.h>
#include <Globals.hxx>
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace PlayerUtils {

    inline bool IsPlayerKnocked(SDK::Player& Player)
    {
        OBF_PROLOGUE;
        if (Player.Character.Address == 0) {
            OBF_JUNK_BLOCK;
            return false;
        }

        static std::unordered_map<uintptr_t, uintptr_t> KoCache;
        static std::mutex CacheMutex;

        std::lock_guard<std::mutex> Lock(CacheMutex);

        auto It = KoCache.find(Player.Character.Address);
        uintptr_t KoAddr = 0;
        if (It != KoCache.end()) {
            KoAddr = It->second;
        } else {
            SDK::Instance BodyEffects = Player.Character.Find_First_Child(std::string(skCrypt("BodyEffects")));
            if (BodyEffects.Address != 0) {
                SDK::Instance Ko = BodyEffects.Find_First_Child(std::string(skCrypt("K.O")));
                KoAddr = Ko.Address;
                if (KoAddr != 0) {
                    KoCache[Player.Character.Address] = KoAddr;
                }
            }
        }

        if (KoCache.size() > 128) {
            std::vector<SDK::Player> Snapshot;
            {
                std::lock_guard<std::mutex> Lock(Cache::Cache_Mutex);
                Snapshot = Globals::Player_Cache;
            }
            std::unordered_map<uintptr_t, uintptr_t> CleanedCache;
            for (auto& Plr : Snapshot) {
                if (Plr.Character.Address != 0) {
                    auto Cached = KoCache.find(Plr.Character.Address);
                    if (Cached != KoCache.end()) {
                        CleanedCache[Plr.Character.Address] = Cached->second;
                    }
                }
            }
            KoCache = std::move(CleanedCache);
        }

        if (KoAddr != 0) {
            return Driver->Read<bool>(KoAddr + Offsets::Misc::Value);
        }
        return false;
    }

}
