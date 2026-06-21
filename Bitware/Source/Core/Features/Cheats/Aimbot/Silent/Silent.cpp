#define NOMINMAX
#include <Windows.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <immintrin.h>
#include <cmath>
#include <limits>
#include <iostream>
#include "Silent.h"
#include <Globals.hxx>
#include "Engine/Engine.h"
#include "Engine/Math/Math.h"
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>
#include <Infrastructure/Logger.h>
#include <Core/Input/InputHook.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>
#include "../../Common/WallCheck.h"

std::uint64_t SilentHelper::CachedInputObject = 0;
// NOT WORKING, left this here too lazy to remove it all and fix whatever breaks from that
float Silent::GetEffectiveFov()
{
    OBF_PROLOGUE;
    if (!Globals::Silent::GunBasedFov)
        return Globals::Silent::Fov;

    std::string ToolName = Globals::LocalPlayer.Tool_Name;

    if (ToolName.empty())
        return Globals::Silent::Fov;

    std::transform(ToolName.begin(), ToolName.end(), ToolName.begin(), ::tolower);

    if (ToolName.find(std::string(skCrypt("double-barrel"))) != std::string::npos ||
        ToolName.find(std::string(skCrypt("double barrel"))) != std::string::npos ||
        ToolName.find(std::string(skCrypt("doublebarrel"))) != std::string::npos)
    {
        return Globals::Silent::FovDoubleBarrel;
    }
    else if (ToolName.find(std::string(skCrypt("tacticalshotgun"))) != std::string::npos ||
        ToolName.find(std::string(skCrypt("tactical shotgun"))) != std::string::npos)
    {
        return Globals::Silent::FovTacticalShotgun;
    }
    else if (ToolName.find(std::string(skCrypt("revolver"))) != std::string::npos)
    {
        return Globals::Silent::FovRevolver;
    }

    return Globals::Silent::Fov;
}

static SDK::Instance GetTargetPart(SDK::Player& Player, int AimPart)
{
    OBF_PROLOGUE;
    SDK::Instance TargetPart{};

    if (AimPart == 0)
    {
        TargetPart = Player.Head;
    }
    else if (AimPart == 1)
    {
        if (Player.UpperTorso.Address != 0)
            TargetPart = Player.UpperTorso;
        else
            TargetPart = Player.Torso;
    }

    return TargetPart;
}

static bool IsPlayerKnocked(SDK::Player& Player)
{
    OBF_PROLOGUE;
    OBF_JUNK_DECLARE;
    if (Player.Character.Address == 0)
        return false;

    struct CacheEntry {
        uintptr_t Character;
        uintptr_t KoAddress;
    };
    static CacheEntry Cache[64] = {};
    static std::atomic<size_t> CacheIndex{ 0 };

    uintptr_t KoAddr = 0;
    for (int i = 0; i < 64; ++i) {
        if (Cache[i].Character == Player.Character.Address) {
            KoAddr = Cache[i].KoAddress;
            break;
        }
    }

    if (KoAddr == 0) {
            SDK::Instance BodyEffects = Player.Character.Find_First_Child(std::string(skCrypt("BodyEffects")));
        if (BodyEffects.Address != 0) {
            SDK::Instance Ko = BodyEffects.Find_First_Child(std::string(skCrypt("K.O")));
            KoAddr = Ko.Address;
            if (KoAddr != 0) {
                size_t idx = CacheIndex.fetch_add(1, std::memory_order_relaxed) % 64;
                Cache[idx] = { Player.Character.Address, KoAddr };
            }
        }
    }

    if (KoAddr != 0) {
        return Driver->Read<bool>(KoAddr + Offsets::Misc::Value);
    }
    return false;
}

static bool IsTargetWithinFov(SDK::Player& Player)
{
    OBF_PROLOGUE;
    OBF_JUNK_BLOCK;
    if (Player.Character.Address == 0)
        return false;

    POINT CursorPoint;
    HWND Window = Globals::RobloxWindow;
    if (!Window || !Api::GetCursorPos(&CursorPoint) || !Api::ScreenToClient(Window, &CursorPoint))
        return false;

    SDK::Vector2 Cursor = { static_cast<float>(CursorPoint.x), static_cast<float>(CursorPoint.y) };

    SDK::Instance TargetPart = GetTargetPart(Player, Globals::Silent::AimPart);
    if (TargetPart.Address == 0)
        return false;

    SDK::Part PartObj(TargetPart.Address);
    SDK::Vector3 PartPosition = PartObj.Get_PartPosition();

    if (Globals::Silent::WallCheck && Globals::Camera.Address != 0)
    {
        SDK::Camera Cam(Globals::Camera.Address);
        SDK::Vector3 CamPos = Cam.Get_CameraPos();
        if (!wallcheck->is_visible(CamPos, PartPosition))
            return false;
    }

    SDK::Vector2 PartScreen = Globals::VisualEngine.World_To_Screen(PartPosition);

    if (PartScreen.x < 0 || PartScreen.y < 0)
        return false;

    float DistanceFromCursor = PartScreen.distance(Cursor);

    OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

    return DistanceFromCursor <= Silent::GetEffectiveFov();
}

static SDK::Player GetClosestPlayerFromCursor()
{
    OBF_PROLOGUE;
    POINT CursorPoint;
    HWND Window = Globals::RobloxWindow;
    if (!Window || !Api::GetCursorPos(&CursorPoint) || !Api::ScreenToClient(Window, &CursorPoint))
        return {};

    SDK::Vector2 Cursor = { static_cast<float>(CursorPoint.x), static_cast<float>(CursorPoint.y) };

    std::vector<SDK::Player> PlayersSnapshot;
    {
        PlayersSnapshot = Globals::Player_Cache;
    }

    if (PlayersSnapshot.empty())
    {
        return {};
    }

    SDK::Player ClosestPlayer{};
    float ShortestDistance = std::numeric_limits<float>::max();

    for (SDK::Player& Player : PlayersSnapshot)
    {
        OBF_JUNK_DECLARE;
        if (Player.Character.Address == 0)
            continue;

        if (Player.Character.Address == Globals::LocalPlayer.Character.Address)
            continue;

        if (Player.Distance > 700.f)
            continue;

        SDK::Instance TargetPart = GetTargetPart(Player, Globals::Silent::AimPart);
        if (TargetPart.Address == 0)
            continue;

        SDK::Part PartObj(TargetPart.Address);
        SDK::Vector3 PartPosition = PartObj.Get_PartPosition();

        if (Globals::Silent::WallCheck && Globals::Camera.Address != 0)
        {
            SDK::Camera Cam(Globals::Camera.Address);
            SDK::Vector3 CamPos = Cam.Get_CameraPos();
            if (!wallcheck->is_visible(CamPos, PartPosition))
                continue;
        }

        SDK::Vector2 PartScreen = Globals::VisualEngine.World_To_Screen(PartPosition);

        if (PartScreen.x < 0 || PartScreen.y < 0)
            continue;

        float DistanceFromCursor = PartScreen.distance(Cursor);

        if (Globals::Silent::UseFov && DistanceFromCursor > Silent::GetEffectiveFov())
            continue;

        if (Globals::Silent::KnockedCheck && IsPlayerKnocked(Player))
            continue;

        if (DistanceFromCursor < ShortestDistance)
        {
            ShortestDistance = DistanceFromCursor;
            ClosestPlayer = Player;
        }
    }

    return ClosestPlayer;
}

static std::uint64_t GetCurrentInputObject(std::uint64_t BaseAddress)
{
    return Driver->Read<std::uint64_t>(BaseAddress + Offsets::MouseService::InputObject + sizeof(std::shared_ptr<void*>));
}

void SilentHelper::SetFramePosX(std::uint64_t Position)
{
    Driver->Write<std::uint64_t>(Address + Offsets::Silent::FramePositionOffsetX, Position);
}

void SilentHelper::SetFramePosY(std::uint64_t Position)
{
    Driver->Write<std::uint64_t>(Address + Offsets::Silent::FramePositionOffsetY, Position);
}

void SilentHelper::InitializeMouseService(std::uint64_t Address)
{
    OBF_PROLOGUE;
    CachedInputObject = GetCurrentInputObject(Address);

    if (CachedInputObject && CachedInputObject != 0xFFFFFFFFFFFFFFFF)
    {
        const char* BasePointer = reinterpret_cast<const char*>(CachedInputObject);
        _mm_prefetch(BasePointer + Offsets::MouseService::MousePosition, _MM_HINT_T0);
        _mm_prefetch(BasePointer + Offsets::MouseService::MousePosition + sizeof(SDK::Vector2), _MM_HINT_T0);
    }
}

void SilentHelper::WriteMousePosition(std::uint64_t Address, float X, float Y)
{
    OBF_PROLOGUE;
    CachedInputObject = GetCurrentInputObject(Address);
    if (CachedInputObject != 0 && CachedInputObject != 0xFFFFFFFFFFFFFFFF)
    {
        Driver_WriteMousePosition(
            Driver->Get_Handle(),
            reinterpret_cast<PVOID>(CachedInputObject + Offsets::MouseService::MousePosition),
            X,
            Y
        );
    }
}

static bool ShouldSilentAimBeActive()
{
    OBF_PROLOGUE;
    OBF_JUNK_DECLARE;
    if (!Globals::Silent::Enabled)
        return false;

    return SilentAimLocked;
}

static void UpdateSilentAimKeyState()
{
    OBF_PROLOGUE;
    int Vk = ImGuiKeyToVK(Globals::Silent::Silent_Key);
    if (!Vk) return;

    bool Pressed = InputHook::IsKeyDown(Vk);

    if (Globals::Silent::Silent_Mode == ImKeyBindMode_Toggle)
    {
        if (Pressed && !SilentAimKeyWasPressed)
        {
            SilentAimLocked = !SilentAimLocked;
        }

        if (!SilentAimLocked)
        {
            SilentCachedTarget = {};
            IsSilentReady = false;
        }
    }
    else
    {
        if (Pressed)
        {
            SilentAimLocked = true;
        }
        else
        {
            SilentAimLocked = false;
            SilentCachedTarget = {};
            IsSilentReady = false;
        }
    }

    SilentAimKeyWasPressed = Pressed;
}

void Silent::SilentFramePos() {
    OBF_PROLOGUE;
    OBF_JUNK_DECLARE;
    SDK::Player Target{};
    HWND Window = Globals::RobloxWindow;
    static SDK::Instance CachedMouseService{};
    static std::uint64_t Stored_Datamodel_Addr = 0;

    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        OBF_JUNK_BLOCK;

        if (Globals::Datamodel.Address != 0 && Globals::Datamodel.Address != Stored_Datamodel_Addr) {
            Stored_Datamodel_Addr = Globals::Datamodel.Address;
            CachedMouseService = SDK::Instance{};
            SilentAimInstance = SDK::Instance{};
            SilentHasOriginalSizes = false;
            SilentOriginalChildrenSizes.clear();
            SilentHelper::CachedInputObject = 0;
        }

        if (CachedMouseService.Address == 0 && Globals::Datamodel.Address != 0) {
            CachedMouseService = Globals::Datamodel.Find_First_Child_Of_Class(std::string(skCrypt("MouseService")));
        }

        if (CachedMouseService.Address == 0 || !Globals::VisualEngine.Address) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        MouseService = std::make_unique<SDK::Instance>(CachedMouseService.Address);

        UpdateSilentAimKeyState();

        if (SilentAimInstance.Address != 0 && SilentHasOriginalSizes) {
            if (Globals::Silent::Enabled) {
                Driver->Write<SDK::Vector2>(SilentAimInstance.Address + Offsets::GuiObject::Size, { 0, 0 });

                auto Children = SilentAimInstance.Children();
                for (auto& Child : Children) {
                    if (Child.Address)
                        Driver->Write<SDK::Vector2>(Child.Address + Offsets::GuiObject::Size, { 0, 0 });
                }
            }
            else {
                Driver->Write<SDK::Vector2>(SilentAimInstance.Address + Offsets::GuiObject::Size, SilentOriginalSize);
                for (const auto& [ChildAddr, OrigSize] : SilentOriginalChildrenSizes) {
                    Driver->Write<SDK::Vector2>(ChildAddr, OrigSize);
                }
            }
        }

        if (!ShouldSilentAimBeActive()) {
            IsSilentReady = false;
            SilentCachedTarget = {};
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        static int AimInstanceCheckCounter = 0;
        if (AimInstanceCheckCounter++ % 100 == 0) {
            try {
                static SDK::Instance CachedLocalPlayer{};
                if (CachedLocalPlayer.Address == 0 && Globals::Players.Address != 0) {
                    CachedLocalPlayer = SDK::Instance(Driver->Read<std::uintptr_t>(Globals::Players.Address + Offsets::Player::LocalPlayer));
                }

                SDK::Instance PlayerGui = CachedLocalPlayer.Find_First_Child(std::string(skCrypt("PlayerGui")));

                if (PlayerGui.Address != 0) {
                    SDK::Instance FoundAimFrame{};
                    auto GuiChildren = PlayerGui.Children();

                    for (auto& Child : GuiChildren) {
                        if (!Child.Address) continue;

                        std::string Name = Child.Name();
                        if (Name == std::string(skCrypt("Aim"))) {
                            FoundAimFrame = Child;
                            break;
                        }

                        std::string Class = Child.Class();
                        if (Class == std::string(skCrypt("Frame")) || Class == std::string(skCrypt("ScreenGui")) || Class == std::string(skCrypt("GuiObject"))) {
                            std::string LowerName = Name;
                            std::transform(LowerName.begin(), LowerName.end(), LowerName.begin(), ::tolower);

                            if (LowerName.find(std::string(skCrypt("main"))) != std::string::npos) {
                                auto Grandchildren = Child.Children();
                                for (auto& GChild : Grandchildren) {
                                    if (GChild.Address && GChild.Name() == std::string(skCrypt("Aim"))) {
                                        FoundAimFrame = GChild;
                                        break;
                                    }
                                }
                            }
                        }
                        if (FoundAimFrame.Address) break;
                    }

                    if (FoundAimFrame.Address != SilentAimInstance.Address) {
                        SilentAimInstance = FoundAimFrame;
                        SilentHasOriginalSizes = false;
                        SilentOriginalChildrenSizes.clear();

                        if (SilentAimInstance.Address != 0) {
                            SilentOriginalSize = Driver->Read<SDK::Vector2>(SilentAimInstance.Address + Offsets::GuiObject::Size);
                            auto AimChildren = SilentAimInstance.Children();
                            for (auto& C : AimChildren) {
                                if (C.Address) {
                                    SDK::Vector2 CSize = Driver->Read<SDK::Vector2>(C.Address + Offsets::GuiObject::Size);
                                    SilentOriginalChildrenSizes.push_back({ C.Address, CSize });
                                }
                            }
                            SilentHasOriginalSizes = true;
                        }
                    }
                }
            }
            catch (...) {
                Logger::Log(WRAPPER_MARCO("[Silent] frame exception"));
            }
        }

        if (!SilentFTarget || SilentCachedTarget.Character.Address == 0) {
            Target = GetClosestPlayerFromCursor();
            SilentCachedLastTarget = Target;
            SDK::Instance TargetPart = GetTargetPart(Target, Globals::Silent::AimPart);
            SilentFTarget = (TargetPart.Address != 0);
            SilentCachedTarget = Target;
        }
        else {
            if (!Globals::Silent::StickyAim) {
                Target = GetClosestPlayerFromCursor();
                SilentCachedTarget = Target;
            }
            else if (Globals::Silent::UseFov) {
                if (!IsTargetWithinFov(SilentCachedTarget)) {
                    SilentFTarget = false;
                    SilentCachedTarget = {};
                    continue;
                }
            }
        }

        if (SilentFTarget && SilentCachedTarget.Character.Address != 0) {
            if (Globals::Silent::KnockedCheck && IsPlayerKnocked(SilentCachedTarget)) {
                SilentFTarget = false;
                SilentCachedTarget = {};
                continue;
            }

            SDK::Instance TargetPart = GetTargetPart(SilentCachedTarget, Globals::Silent::AimPart);
            if (TargetPart.Address != 0) {
                SDK::Part PartObj(TargetPart.Address);
                SDK::Vector3 Part3D = PartObj.Get_PartPosition();
                SDK::Vector2 PartScreen = Globals::VisualEngine.World_To_Screen(Part3D);

                POINT CursorPos;
                Api::GetCursorPos(&CursorPos);
                if (Window) Api::ScreenToClient(Window, &CursorPos);

                SDK::Vector2 Dims = Globals::VisualEngine.Get_Dimensions();
                SilentPartPos = PartScreen;
                SilentCachedPositionX = static_cast<std::uint64_t>(CursorPos.x);
                SilentCachedPositionY = static_cast<std::uint64_t>(Dims.y - std::abs(Dims.y - static_cast<float>(CursorPos.y)) - 58);
                IsSilentReady = true;
            }
        }
        else {
            IsSilentReady = false;
        }
    }
}

void Silent::SilentMouse()
{
    OBF_PROLOGUE;
    SilentHelper MouseServiceInstance{};
    bool MouseServiceInitialized = false;

    for (;;)
    {
        OBF_JUNK_BLOCK;
        if (!MouseService || !ShouldSilentAimBeActive() || SilentCachedTarget.Character.Address == 0 || !IsSilentReady)
        {
            MouseServiceInitialized = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (SilentPartPos.x < -5000.0f || SilentPartPos.y < -5000.0f ||
            SilentPartPos.x > 15000.0f || SilentPartPos.y > 15000.0f)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        try
        {
            if (!MouseServiceInitialized)
            {
                MouseServiceInstance.InitializeMouseService(MouseService->Address);
                MouseServiceInitialized = true;
            }

            MouseServiceInstance.WriteMousePosition(MouseService->Address, SilentPartPos.x, SilentPartPos.y);
        }
        catch (...)
        {
            Logger::Log(WRAPPER_MARCO("[Silent] mouse write exception"));
            MouseServiceInitialized = false;
        }

        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
        Api::Sleep(1);
    }
}

void Silent::RunService()
{
    OBF_PROLOGUE;
    std::thread(SilentFramePos).detach();
    std::thread(SilentMouse).detach();
}
