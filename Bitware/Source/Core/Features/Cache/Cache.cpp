#include "Cache.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <Miscellaneous/Output/Output.h>
#include <Globals.hxx>
#include "PhantomForces/PhantomForces.h"
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>
#include <Infrastructure/Logger.h>
#include <Core/Features/Cheats/Common/WallCheck.h>
#include <Features/Chams/MemoryMeshes.h>

namespace Cache {

    struct Part_Mapping {
        std::string Name;
        SDK::Instance SDK::Player::* Member;
    };

    static const Part_Mapping Part_Mappings[] = {
        {std::string(skCrypt("Head")), &SDK::Player::Head},
        {std::string(skCrypt("Humanoid")), &SDK::Player::Humanoid},
        {std::string(skCrypt("HumanoidRootPart")), &SDK::Player::HumanoidRootPart},
        {std::string(skCrypt("Left Arm")), &SDK::Player::LeftArm},
        {std::string(skCrypt("Left Leg")), &SDK::Player::LeftLeg},
        {std::string(skCrypt("LeftFoot")), &SDK::Player::LeftFoot},
        {std::string(skCrypt("LeftHand")), &SDK::Player::LeftHand},
        {std::string(skCrypt("LeftLowerArm")), &SDK::Player::LeftLowerArm},
        {std::string(skCrypt("LeftLowerLeg")), &SDK::Player::LeftLowerLeg},
        {std::string(skCrypt("LeftUpperArm")), &SDK::Player::LeftUpperArm},
        {std::string(skCrypt("LeftUpperLeg")), &SDK::Player::LeftUpperLeg},
        {std::string(skCrypt("LowerTorso")), &SDK::Player::LowerTorso},
        {std::string(skCrypt("Right Arm")), &SDK::Player::RightArm},
        {std::string(skCrypt("Right Leg")), &SDK::Player::RightLeg},
        {std::string(skCrypt("RightFoot")), &SDK::Player::RightFoot},
        {std::string(skCrypt("RightHand")), &SDK::Player::RightHand},
        {std::string(skCrypt("RightLowerArm")), &SDK::Player::RightLowerArm},
        {std::string(skCrypt("RightLowerLeg")), &SDK::Player::RightLowerLeg},
        {std::string(skCrypt("RightUpperArm")), &SDK::Player::RightUpperArm},
        {std::string(skCrypt("RightUpperLeg")), &SDK::Player::RightUpperLeg},
        {std::string(skCrypt("Torso")), &SDK::Player::Torso},
        {std::string(skCrypt("UpperTorso")), &SDK::Player::UpperTorso}
    };

    inline SDK::Instance SDK::Player::* Find_Part_Member(std::string_view Name) {
        OBF_PROLOGUE;
        for (const auto& Mapping : Part_Mappings) {
            if (Mapping.Name == Name) {
                return Mapping.Member;
            }
        }
        return nullptr;
    }

    std::mutex Cache_Mutex;
    std::atomic<bool> CacheReady{ false };
    std::atomic<bool> References_Updated{ false };
    std::atomic<bool> ReinitInProgress{ false };
    std::atomic<std::uint64_t> Current_GameID{ 0 };
    std::atomic<int> StaleFrames{ 0 };
    std::atomic<bool> ForceReInit{ false };
    constexpr int StaleThreshold = 15;

    inline float Calculate_Distance(const SDK::Vector3& P1, const SDK::Vector3& P2) {
        OBF_PROLOGUE;
        return P1.distance(P2);
    }

    inline bool Valid_Position(const SDK::Vector3& Pos) {
        OBF_PROLOGUE;
        return !std::isnan(Pos.x) && !std::isnan(Pos.y) && !std::isnan(Pos.z)
            && std::isfinite(Pos.x) && std::isfinite(Pos.y) && std::isfinite(Pos.z);
    }

    void Cache_Data(SDK::Player& Player, const SDK::Vector3& Camera_Pos, bool Is_Local) {
        OBF_PROLOGUE;
        if (Player.Character.Address == 0) return;

        static thread_local std::uint64_t LastCharacterAddress = 0;
        static thread_local std::uint64_t LocalVersionCounter = 0;

        if (Player.Character.Address != LastCharacterAddress) {
            ++LocalVersionCounter;
            LastCharacterAddress = Player.Character.Address;
        }
        Player.CharacterVersion = LocalVersionCounter;

        Player.Tool_Name.clear();
        auto Children = Player.Character.Children();
        for (const auto& Part : Children) {
            auto Member = Find_Part_Member(Part.Name());
            if (Member) {
                Player.*Member = Part;
            }
            if (Part.Class() == std::string(skCrypt("Tool"))) {
                Player.Tool_Name = Part.Name();
            }
        }

        if (Player.Humanoid.Address) {
            SDK::Humanoid Humanoid(Player.Humanoid.Address);
            Player.Health = Humanoid.Get_Health();
            Player.MaxHealth = Humanoid.Get_MaxHealth();
            Player.Rig_Type = Humanoid.Get_RigType();
        }

        if (!Is_Local && Player.Head.Address != 0 && Valid_Position(Camera_Pos)) {
            SDK::Part Head(Player.Head.Address);
            SDK::Vector3 Head_Pos = Head.Get_PartPosition();

            if (Valid_Position(Head_Pos)) {
                Player.Distance = Calculate_Distance(Head_Pos, Camera_Pos);
            }
        }
    }

    void Update_Cache(const SDK::Vector3& Camera_Pos) {
        OBF_PROLOGUE;
        if (Globals::Players.Address == 0) {
            StaleFrames.store(0, std::memory_order_relaxed);
            return;
        }

        SDK::Players Local_SDK_Player = Globals::Players.Get_Local_Player();
        if (Local_SDK_Player.Address == 0) {
            StaleFrames.store(0, std::memory_order_relaxed);
            return;
        }

        StaleFrames.store(0, std::memory_order_relaxed);

        auto Local_Team = Local_SDK_Player.Get_Team();
        auto Player_Instances = Globals::Players.Children();
        std::vector<SDK::Instance> PlayerInstanceCopy(Player_Instances.begin(), Player_Instances.end());

        std::vector<SDK::Player> Players;
        Players.reserve(PlayerInstanceCopy.size());

        for (const auto& Instance : PlayerInstanceCopy) {
            if (Instance.Address == 0) continue;

            if (Instance.Address == Local_SDK_Player.Address) {
                if (Globals::Settings::Client_Check) {
                    continue;
                }
            }

            SDK::Players Entity(Instance.Address);
            if (Globals::Settings::Team_Check) {
                if (Entity.Get_Team() == Local_Team) {
                    continue;
                }
            }

            SDK::Player Player{};
            Player.Player = Entity;
            Player.Character = Entity.Character();
            Player.Name = Instance.Name();
            Player.UserID = Entity.Get_UserID();
            Player.Display_Name = Entity.Get_DisplayName();
            Player.Local_Player = false;

            Cache_Data(Player, Camera_Pos, false);

            Players.push_back(std::move(Player));
        }

        std::lock_guard<std::mutex> Lock(Cache_Mutex);
        Globals::Player_Cache = std::make_shared<const std::vector<SDK::Player>>(std::move(Players));
    }

    void Runtime() {
        OBF_PROLOGUE;
        bool LocalPlayerFound = false;

        if (Globals::Players.Address == 0) {
            Globals::LocalPlayer = SDK::Player{};
        }
        else {
            SDK::Players LocalPlayerInstance = Globals::Players.Get_Local_Player();
            if (LocalPlayerInstance.Address == 0) {
                Globals::LocalPlayer = SDK::Player{};
            }
            else {
                LocalPlayerFound = true;

                StaleFrames.store(0, std::memory_order_relaxed);

                SDK::Player LocalPlayer{};
                LocalPlayer.Player = LocalPlayerInstance;
                LocalPlayer.Character = LocalPlayerInstance.Character();
                LocalPlayer.Name = LocalPlayerInstance.Name();
                LocalPlayer.Local_Player = true;

                SDK::Vector3 Camera_Position{};
                if (Globals::Camera.Address != 0) {
                    SDK::Camera Camera(Globals::Camera.Address);
                    Camera_Position = Camera.Get_CameraPos();
                }

                Cache_Data(LocalPlayer, Camera_Position, true);

                Globals::LocalPlayer = LocalPlayer;
                Globals::GameID = Current_GameID.load();

                Update_Cache(Camera_Position);
            }
        }

        if (!LocalPlayerFound && Globals::Players.Address != 0) {
            int stale = StaleFrames.fetch_add(1, std::memory_order_relaxed) + 1;
            if (stale >= StaleThreshold) {
                ForceReInit.store(true);
            }
        }
    }
}

void Cache::RunService(std::stop_token st) {
    OBF_PROLOGUE;
    std::jthread rescan_thread;

    if (!st.stop_requested()) {
        rescan_thread = std::jthread([](std::stop_token inner_st) {
            static std::uint64_t Stored_GameID = 0;
            while (!inner_st.stop_requested()) {
                try {
                    auto Module_Base = Driver->Get_Module();
                    if (Module_Base != 0) {
                        auto FakeDataModel = Driver->Read<std::uint64_t>(Module_Base + Offsets::FakeDataModel::Pointer);
                        if (FakeDataModel != 0) {
                            Globals::Datamodel.Address = Driver->Read<std::uint64_t>(FakeDataModel + Offsets::FakeDataModel::RealDataModel);

                            if (Globals::Datamodel.Address != 0) {
                                std::uint64_t GameID = Driver->Read<uint64_t>(Globals::Datamodel.Address + Offsets::DataModel::PlaceId);

                                if (ForceReInit.exchange(false)) {
                                    Stored_GameID = 0;
                                }

                                if (GameID != Stored_GameID) {
                                    Stored_GameID = GameID;
                                    Current_GameID.store(GameID);
                                    Globals::GameID = GameID;

                                    ReinitInProgress.store(true);
                                    CacheReady.store(false);

                                    Globals::Players.Address = Globals::Datamodel.Find_First_Child_Of_Class(std::string(skCrypt("Players"))).Address;
                                    auto Lightin = Globals::Datamodel.Find_First_Child_Of_Class(std::string(skCrypt("Lighting")));
                                    Globals::Lighting = SDK::Lighting(Lightin.Address);
                                    Globals::Workspace.Address = Globals::Datamodel.Find_First_Child_Of_Class(std::string(skCrypt("Workspace"))).Address;

                                    Globals::Camera.Address = 0;
                                    for (int retries = 0; retries < 50; ++retries)
                                    {
                                        Globals::Camera.Address = Globals::Workspace.Find_First_Child_Of_Class(std::string(skCrypt("Camera"))).Address;
                                        if (Globals::Camera.Address != 0)
                                            break;
                                        SDK::sleep_jitter(100, 25);
                                    }

                                    Globals::LocalPlayer = SDK::Player{};

                                    wallcheck->clear();
                                    mesh_chams::clear_caches();

                                    ReinitInProgress.store(false);
                                    References_Updated.store(true);
                                }
                            }
                        }
                    }

                    if (Module_Base != 0) {
                        uint64_t veAddr = Driver->Read<std::uint64_t>(Module_Base + Offsets::VisualEngine::Pointer);
                        if (veAddr != 0) {
                            Globals::VisualEngine.Address = veAddr;
                        }
                    }

                    static int window_refresh_counter = 0;
                    if (++window_refresh_counter >= 20) {
                        window_refresh_counter = 0;
                        DWORD target_pid = Driver->Get_Process();
                        EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL {
                            DWORD pid = 0;
                            GetWindowThreadProcessId(hwnd, &pid);
                            if (pid == static_cast<DWORD>(lparam) && IsWindowVisible(hwnd)) {
                                Globals::RobloxWindow = hwnd;
                                return FALSE;
                            }
                            return TRUE;
                        }, static_cast<LPARAM>(target_pid));
                    }

                    if (Globals::VisualEngine.Address != 0 && Globals::Datamodel.Address != 0 && Globals::Camera.Address != 0)
                    {
                        CacheReady.store(true, std::memory_order_release);
                    }
                }
                catch (...) {
                    Logger::Log(WRAPPER_MARCO("[Cache] rescan exception"));
                }

                SDK::sleep_jitter(100, 25);
            }
        }, st);
    }

    while (!st.stop_requested()) {
        try {
            if (References_Updated.exchange(false)) {
                std::lock_guard<std::mutex> lock(Cache_Mutex);
                Globals::LocalPlayer = SDK::Player{};
                Globals::Player_Cache.reset();
                StaleFrames.store(0, std::memory_order_relaxed);
            }

            if (ReinitInProgress.load()) {
                SDK::sleep_jitter(50, 10);
                continue;
            }

            auto cacheStart = std::chrono::steady_clock::now();
            Runtime();
            auto cacheEnd = std::chrono::steady_clock::now();
            Perf::CacheTimeUs.store(std::chrono::duration_cast<std::chrono::microseconds>(cacheEnd - cacheStart).count());
        }
        catch (...) {
            Logger::Log(WRAPPER_MARCO("[Cache] runtime exception"));
        }

        SDK::sleep_jitter(150, 35);
    }
}
