#pragma once
#include <memory>
#include <cstdint>
#include <vector>
#include "Engine/Engine.h"
#include "Core/Features/Cache/Cache.h"

namespace Silent
{
    void SilentFramePos(std::stop_token st);
    void SilentMouse(std::stop_token st);
    void RunService(std::stop_token st);
    float GetEffectiveFov();
}

inline std::unique_ptr<SDK::Instance> MouseService{};

inline bool IsSilentReady{ false };
inline bool SilentTarget{ false };
inline bool TargetNeedsReset{ false };
inline SDK::Vector2 SilentPartPos{};
inline std::uint64_t SilentCachedPositionX{ 0 };
inline std::uint64_t SilentCachedPositionY{ 0 };
inline SDK::Instance SilentAimInstance{};
inline SDK::Player SilentCachedTarget{};
inline SDK::Player SilentCachedLastTarget{};
inline bool IsSilentEnabled{ true };
inline bool SilentStickyAim{ false };
inline int SilentAimKeybind{ 0 };
inline bool SilentAimLocked{ false };
inline bool SilentAimKeyWasPressed{ false };
inline bool SilentFTarget{ false };
inline bool SilentHasOriginalSizes{ false };
inline SDK::Vector2 SilentOriginalSize{};
inline std::vector<std::pair<std::uint64_t, SDK::Vector2>> SilentOriginalChildrenSizes{};

struct SilentHelper final
{
    std::uint64_t Address = 0;
    static std::uint64_t CachedInputObject;

    SilentHelper() = default;
    SilentHelper(std::uint64_t addr) : Address(addr) {}

    void SetFramePosX(std::uint64_t position);
    void SetFramePosY(std::uint64_t position);
    void InitializeMouseService(std::uint64_t address);
    void WriteMousePosition(std::uint64_t address, float x, float y);
};