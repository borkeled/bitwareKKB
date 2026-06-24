#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>

#include <Core/Settings/Settings.h>
#include <Infrastructure/Obfuscation.h>
#include <Infrastructure/Logger.h>
#include <Infrastructure/ResourceEnc.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

class ConfigManager
{
public:
    static ConfigManager& Get()
    {
        OBF_PROLOGUE;
        static ConfigManager instance;
        OBF_JUNK_BLOCK;
        return instance;
    }

    bool Save(const char* name)
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
        std::string path = GetConfigPath(name);
        if (path.empty()) return false;

        std::stringstream ss;
        ss << "{\n";

        // Bool
        WriteBool(ss, WRAPPER_MARCO("Aimbot_Enabled"), SettingsStore::Aimbot_Enabled);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_DrawFov"), SettingsStore::Aimbot_DrawFov);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_FovSpin"), SettingsStore::Aimbot_FovSpin);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_FillFov"), SettingsStore::Aimbot_FillFov);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_useFov"), SettingsStore::Aimbot_useFov);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_AimbotSticky"), SettingsStore::Aimbot_AimbotSticky);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_Shake"), SettingsStore::Aimbot_Shake);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_KnockedCheck"), SettingsStore::Aimbot_KnockedCheck);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_WallCheck"), SettingsStore::Aimbot_WallCheck);
        WriteBool(ss, WRAPPER_MARCO("Aimbot_ClosestPlayerFound"), SettingsStore::Aimbot_ClosestPlayerFound);

        WriteBool(ss, WRAPPER_MARCO("Visuals_Enabled"), SettingsStore::Visuals_Enabled);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Box"), SettingsStore::Visuals_Box);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Box_Fill"), SettingsStore::Visuals_Box_Fill);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Box_Fill_Gradient"), SettingsStore::Visuals_Box_Fill_Gradient);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Box_Fill_Gradient_Rotate"), SettingsStore::Visuals_Box_Fill_Gradient_Rotate);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Healthbar"), SettingsStore::Visuals_Healthbar);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Health"), SettingsStore::Visuals_Health);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Name"), SettingsStore::Visuals_Name);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Distance"), SettingsStore::Visuals_Distance);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Rig_Type"), SettingsStore::Visuals_Rig_Type);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Tool"), SettingsStore::Visuals_Tool);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Skeleton"), SettingsStore::Visuals_Skeleton);
        WriteBool(ss, WRAPPER_MARCO("Visuals_Chams"), SettingsStore::Visuals_Chams);
        WriteBool(ss, WRAPPER_MARCO("Visuals_ChamsFade"), SettingsStore::Visuals_ChamsFade);
        WriteBool(ss, WRAPPER_MARCO("Visuals_WallCheck"), SettingsStore::Visuals_WallCheck);

        WriteBool(ss, WRAPPER_MARCO("World_Skybox"), SettingsStore::World_Skybox);
        WriteBool(ss, WRAPPER_MARCO("World_Rotate"), SettingsStore::World_Rotate);
        WriteBool(ss, WRAPPER_MARCO("World_Ambience"), SettingsStore::World_Ambience);
        WriteBool(ss, WRAPPER_MARCO("World_Fog"), SettingsStore::World_Fog);
        WriteBool(ss, WRAPPER_MARCO("World_Brightness"), SettingsStore::World_Brightness);
        WriteBool(ss, WRAPPER_MARCO("World_Exposure"), SettingsStore::World_Exposure);
        WriteBool(ss, WRAPPER_MARCO("World_FOV"), SettingsStore::World_FOV);

        WriteBool(ss, WRAPPER_MARCO("Settings_Team_Check"), SettingsStore::Settings_Team_Check);
        WriteBool(ss, WRAPPER_MARCO("Settings_Client_Check"), SettingsStore::Settings_Client_Check);
        WriteBool(ss, WRAPPER_MARCO("Settings_Streamproof"), SettingsStore::Settings_Streamproof);

        WriteBool(ss, WRAPPER_MARCO("Whitelist_Enabled"), SettingsStore::Whitelist_Enabled);

        WriteBool(ss, WRAPPER_MARCO("Triggerbot_Enabled"), SettingsStore::Triggerbot_Enabled);
        WriteBool(ss, WRAPPER_MARCO("Triggerbot_KnockedCheck"), SettingsStore::Triggerbot_KnockedCheck);
        WriteBool(ss, WRAPPER_MARCO("Triggerbot_WallCheck"), SettingsStore::Triggerbot_WallCheck);

        WriteBool(ss, WRAPPER_MARCO("Misc_Speed"), SettingsStore::Misc_Speed);
        WriteBool(ss, WRAPPER_MARCO("Misc_Jump"), SettingsStore::Misc_Jump);

        // Int
        WriteInt(ss, WRAPPER_MARCO("Aimbot_type"), SettingsStore::Aimbot_type);
        WriteInt(ss, WRAPPER_MARCO("Aimbot_HitPart"), SettingsStore::Aimbot_HitPart);
        WriteInt(ss, WRAPPER_MARCO("Aimbot_FovSpinDirection"), SettingsStore::Aimbot_FovSpinDirection);
        WriteInt(ss, WRAPPER_MARCO("Aimbot_FovSpinSpeed"), SettingsStore::Aimbot_FovSpinSpeed);

        WriteInt(ss, WRAPPER_MARCO("Visuals_ChamsFadeSpeed"), SettingsStore::Visuals_ChamsFadeSpeed);
        WriteInt(ss, WRAPPER_MARCO("Visuals_BoxFillSpeed"), SettingsStore::Visuals_BoxFillSpeed);
        WriteInt(ss, WRAPPER_MARCO("Visuals_Healthbar_Type"), SettingsStore::Visuals_Healthbar_Type);
        WriteInt(ss, WRAPPER_MARCO("Visuals_Box_Type"), SettingsStore::Visuals_Box_Type);
        WriteInt(ss, WRAPPER_MARCO("Visuals_Box_Fill_Type"), SettingsStore::Visuals_Box_Fill_Type);
        WriteInt(ss, WRAPPER_MARCO("Visuals_Name_Type"), SettingsStore::Visuals_Name_Type);
        WriteInt(ss, WRAPPER_MARCO("Visuals_Gap"), SettingsStore::Visuals_Gap);
        WriteInt(ss, WRAPPER_MARCO("Visuals_Thickness"), SettingsStore::Visuals_Thickness);

        WriteInt(ss, WRAPPER_MARCO("World_Skybox_Type"), SettingsStore::World_Skybox_Type);

        WriteInt(ss, WRAPPER_MARCO("Triggerbot_Delay"), SettingsStore::Triggerbot_Delay);
        WriteInt(ss, WRAPPER_MARCO("Triggerbot_Randomize"), SettingsStore::Triggerbot_Randomize);
        WriteInt(ss, WRAPPER_MARCO("Triggerbot_HitPart"), SettingsStore::Triggerbot_HitPart);
        WriteInt(ss, WRAPPER_MARCO("Triggerbot_FireMode"), SettingsStore::Triggerbot_FireMode);
        WriteInt(ss, WRAPPER_MARCO("Triggerbot_Fov"), SettingsStore::Triggerbot_Fov);

        WriteInt(ss, WRAPPER_MARCO("Settings_Performance_Mode"), SettingsStore::Settings_Performance_Mode);
        WriteInt(ss, WRAPPER_MARCO("WallCheck_Method"), SettingsStore::WallCheck_Method);

        // Keybinds
        WriteInt(ss, WRAPPER_MARCO("Aimbot_Key"), (int)SettingsStore::Aimbot_Key);
        WriteInt(ss, WRAPPER_MARCO("Aimbot_Mode"), (int)SettingsStore::Aimbot_Mode);
        WriteInt(ss, WRAPPER_MARCO("Aimbot_FovToggleKey"), (int)SettingsStore::Aimbot_FovToggleKey);
        WriteInt(ss, WRAPPER_MARCO("Aimbot_FovToggleMode"), (int)SettingsStore::Aimbot_FovToggleMode);
        WriteInt(ss, WRAPPER_MARCO("Visuals_ToggleKey"), (int)SettingsStore::Visuals_ToggleKey);
        WriteInt(ss, WRAPPER_MARCO("Visuals_ToggleMode"), (int)SettingsStore::Visuals_ToggleMode);
        WriteInt(ss, WRAPPER_MARCO("Triggerbot_Key"), (int)SettingsStore::Triggerbot_Key);
        WriteInt(ss, WRAPPER_MARCO("Triggerbot_Mode"), (int)SettingsStore::Triggerbot_Mode);
        WriteInt(ss, WRAPPER_MARCO("Misc_Speed_Key"), (int)SettingsStore::Misc_Speed_Key);
        WriteInt(ss, WRAPPER_MARCO("Misc_Speed_Key_Mode"), (int)SettingsStore::Misc_Speed_Key_Mode);
        WriteInt(ss, WRAPPER_MARCO("Misc_Jump_Key"), (int)SettingsStore::Misc_Jump_Key);
        WriteInt(ss, WRAPPER_MARCO("Misc_Jump_Key_Mode"), (int)SettingsStore::Misc_Jump_Key_Mode);
        WriteInt(ss, WRAPPER_MARCO("Settings_Menu_Keybind"), (int)SettingsStore::Settings_Menu_Keybind);
        // Float
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_FovSize"), SettingsStore::Aimbot_FovSize);
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_ShakeX"), SettingsStore::Aimbot_ShakeX);
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_ShakeY"), SettingsStore::Aimbot_ShakeY);
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_ShakeZ"), SettingsStore::Aimbot_ShakeZ);
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_Mouse_SmoothingX"), SettingsStore::Aimbot_Mouse_SmoothingX);
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_Mouse_SmoothingY"), SettingsStore::Aimbot_Mouse_SmoothingY);
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_Mouse_Sensitivity"), SettingsStore::Aimbot_Mouse_Sensitivity);
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_Camera_SmoothingX"), SettingsStore::Aimbot_Camera_SmoothingX);
        WriteFloat(ss, WRAPPER_MARCO("Aimbot_Camera_SmoothingY"), SettingsStore::Aimbot_Camera_SmoothingY);

        WriteFloat(ss, WRAPPER_MARCO("Visuals_Render_Distance"), SettingsStore::Visuals_Render_Distance);

        WriteFloat(ss, WRAPPER_MARCO("World_Rotate_Speed"), SettingsStore::World_Rotate_Speed);
        WriteFloat(ss, WRAPPER_MARCO("World_Fog_Distance"), SettingsStore::World_Fog_Distance);
        WriteFloat(ss, WRAPPER_MARCO("World_FOV_Distance"), SettingsStore::World_FOV_Distance);
        WriteFloat(ss, WRAPPER_MARCO("World_ExposureI"), SettingsStore::World_ExposureI);
        WriteFloat(ss, WRAPPER_MARCO("World_BrightnessI"), SettingsStore::World_BrightnessI);

        WriteFloat(ss, WRAPPER_MARCO("Misc_Speed_Value"), SettingsStore::Misc_Speed_Value);
        WriteFloat(ss, WRAPPER_MARCO("Misc_Jump_Value"), SettingsStore::Misc_Jump_Value);

        // Float arrays (colors)
        WriteFloatArray(ss, WRAPPER_MARCO("Aimbot_FovColor"), SettingsStore::Aimbot_FovColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_BoxColor"), SettingsStore::Visuals_BoxColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_BoxFillTop"), SettingsStore::Visuals_BoxFillTop);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_BoxFillBottom"), SettingsStore::Visuals_BoxFillBottom);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_HealthbarColor"), SettingsStore::Visuals_HealthbarColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_NameColor"), SettingsStore::Visuals_NameColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_DistanceColor"), SettingsStore::Visuals_DistanceColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_RigTypeColor"), SettingsStore::Visuals_RigTypeColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_ToolColor"), SettingsStore::Visuals_ToolColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_SkeletonColor"), SettingsStore::Visuals_SkeletonColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_ChamsColor"), SettingsStore::Visuals_ChamsColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_ChamsOutline"), SettingsStore::Visuals_ChamsOutline);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_HealthbarTop"), SettingsStore::Visuals_HealthbarTop);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_HealthbarMiddle"), SettingsStore::Visuals_HealthbarMiddle);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_HealthbarBottom"), SettingsStore::Visuals_HealthbarBottom);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_HealthColor"), SettingsStore::Visuals_HealthColor);
        WriteFloatArray(ss, WRAPPER_MARCO("Visuals_OccludedColor"), SettingsStore::Visuals_OccludedColor);

        WriteFloatArray(ss, WRAPPER_MARCO("World_AmbienceColor"), SettingsStore::World_AmbienceColor);
        WriteFloatArray(ss, WRAPPER_MARCO("World_FogColor"), SettingsStore::World_FogColor);

        WriteFloatArray(ss, WRAPPER_MARCO("Whitelist_Color"), SettingsStore::Whitelist_Color);

        ss << "}\n";

        std::string raw = ss.str();
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

        auto key = ResourceEnc::GenerateKey(WRAPPER_MARCO("BitwareConfig"));
        std::vector<std::uint8_t> buf(raw.begin(), raw.end());
        ResourceEnc::EncryptBuffer(buf.data(), buf.size(), key);

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return false;

        std::uint32_t size = static_cast<std::uint32_t>(buf.size());
        file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        file.write(reinterpret_cast<const char*>(buf.data()), buf.size());
        file.close();
        return true;
    }

    bool Load(const char* name)
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
        std::string path = GetConfigPath(name);
        if (path.empty()) return false;

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        std::streamsize fileSize = file.tellg();
        if (fileSize < 1) { file.close(); return false; }
        file.seekg(0);

        // Read entire file into buffer
        std::vector<char> raw(static_cast<size_t>(fileSize));
        file.read(raw.data(), fileSize);
        file.close();

        // Detect format: old configs are plain JSON (starts with '{')
        if (raw[0] == '{') {
            std::stringstream ss(std::string(raw.data(), raw.size()));
            std::string line;
            while (std::getline(ss, line))
            {
                auto start = line.find_first_not_of(" \t\r\n");
                if (start == std::string::npos) continue;
                line = line.substr(start);
                if (line.rfind("\"", 0) == 0)
                {
                    size_t colon = line.find(':');
                    if (colon == std::string::npos) continue;
                    std::string k = line.substr(1, colon - 2);
                    std::string v = line.substr(colon + 1);
                    v.erase(0, v.find_first_not_of(" \t\r\n"));
                    v.erase(v.find_last_not_of(",\r\n") + 1);
                    AssignValue(k, v);
                }
            }
            return true;
        }

        // New encrypted binary format
        if (fileSize < 5) return false;
        std::uint32_t size = *reinterpret_cast<const std::uint32_t*>(raw.data());
        if (size == 0 || static_cast<std::streamsize>(size) + 4 > fileSize) return false;
        std::vector<std::uint8_t> encrypted(raw.begin() + 4, raw.begin() + 4 + size);

        auto key = ResourceEnc::GenerateKey(WRAPPER_MARCO("BitwareConfig"));
        ResourceEnc::DecryptBuffer(encrypted.data(), encrypted.size(), key);

        std::string content(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        std::string line;
        std::stringstream ss(content);
        while (std::getline(ss, line))
        {
            auto start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            line = line.substr(start);
            if (line.rfind("\"", 0) == 0)
            {
                size_t colon = line.find(':');
                if (colon == std::string::npos) continue;
                std::string k = line.substr(1, colon - 2);
                std::string v = line.substr(colon + 1);
                v.erase(0, v.find_first_not_of(" \t\r\n"));
                v.erase(v.find_last_not_of(",\r\n") + 1);
                AssignValue(k, v);
            }
        }
        return true;
    }

    bool Delete(const char* name)
    {
        OBF_PROLOGUE;
        std::string path = GetConfigPath(name);
        if (path.empty()) return false;
        OBF_JUNK_BLOCK;
        return fs::remove(path);
    }

    bool Exists(const char* name)
    {
        OBF_PROLOGUE;
        std::string path = GetConfigPath(name);
        if (path.empty()) return false;
        return fs::exists(path);
    }

    std::vector<std::string> ListNames()
    {
        OBF_PROLOGUE;
        std::vector<std::string> names;
        try
        {
            for (const auto& entry : fs::directory_iterator(GetExeDirectory()))
            {
                if (entry.is_regular_file())
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    OBF_OPAQUE_FALSE { OBF_JUNK_BLOCK; }
                    if (ext == ".json")
                    {
                        std::string stem = entry.path().stem().string();
                        names.push_back(stem);
                    }
                }
            }
        }
        catch (...) {
            Logger::Log(WRAPPER_MARCO("[Config] list exception"));
        }

        std::sort(names.begin(), names.end());
        return names;
    }

private:
    static fs::path GetExeDirectory()
    {
        char path[MAX_PATH];
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return fs::path(path).parent_path();
    }

    std::string GetConfigPath(const char* name)
    {
        OBF_PROLOGUE;
        if (!name || !*name) return "";
        std::string sname = name;
        if (sname.size() >= 5 && sname.substr(sname.size() - 5) == ".json")
            sname = sname.substr(0, sname.size() - 5);
        return (GetExeDirectory() / (sname + ".json")).string();
    }

    void WriteBool(std::stringstream& f, const char* key, bool val)
    {
        OBF_PROLOGUE;
        f << "    \"" << key << "\": " << (val ? "true" : "false") << ",\n";
    }

    void WriteInt(std::stringstream& f, const char* key, int val)
    {
        OBF_PROLOGUE;
        f << "    \"" << key << "\": " << val << ",\n";
    }

    void WriteFloat(std::stringstream& f, const char* key, float val)
    {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
        f << "    \"" << key << "\": " << val << ",\n";
    }

    void WriteFloatArray(std::stringstream& f, const char* key, const float arr[4])
    {
        OBF_PROLOGUE;
        f << "    \"" << key << "\": [" << arr[0] << ", " << arr[1] << ", " << arr[2] << ", " << arr[3] << "],\n";
    }

    void AssignValue(const std::string& key, const std::string& val)
    {
        OBF_PROLOGUE;
        if (key == WRAPPER_MARCO("Aimbot_FovColor")) { ParseFloatArray(val, SettingsStore::Aimbot_FovColor); return; }
        if (key == WRAPPER_MARCO("Visuals_BoxColor")) { ParseFloatArray(val, SettingsStore::Visuals_BoxColor); return; }
        if (key == WRAPPER_MARCO("Visuals_BoxFillTop")) { ParseFloatArray(val, SettingsStore::Visuals_BoxFillTop); return; }
        if (key == WRAPPER_MARCO("Visuals_BoxFillBottom")) { ParseFloatArray(val, SettingsStore::Visuals_BoxFillBottom); return; }
        if (key == WRAPPER_MARCO("Visuals_HealthbarColor")) { ParseFloatArray(val, SettingsStore::Visuals_HealthbarColor); return; }
        if (key == WRAPPER_MARCO("Visuals_NameColor")) { ParseFloatArray(val, SettingsStore::Visuals_NameColor); return; }
        if (key == WRAPPER_MARCO("Visuals_DistanceColor")) { ParseFloatArray(val, SettingsStore::Visuals_DistanceColor); return; }
        if (key == WRAPPER_MARCO("Visuals_RigTypeColor")) { ParseFloatArray(val, SettingsStore::Visuals_RigTypeColor); return; }
        if (key == WRAPPER_MARCO("Visuals_ToolColor")) { ParseFloatArray(val, SettingsStore::Visuals_ToolColor); return; }
        if (key == WRAPPER_MARCO("Visuals_SkeletonColor")) { ParseFloatArray(val, SettingsStore::Visuals_SkeletonColor); return; }
        if (key == WRAPPER_MARCO("Visuals_ChamsColor")) { ParseFloatArray(val, SettingsStore::Visuals_ChamsColor); return; }
        if (key == WRAPPER_MARCO("Visuals_ChamsOutline")) { ParseFloatArray(val, SettingsStore::Visuals_ChamsOutline); return; }
        if (key == WRAPPER_MARCO("Visuals_HealthbarTop")) { ParseFloatArray(val, SettingsStore::Visuals_HealthbarTop); return; }
        if (key == WRAPPER_MARCO("Visuals_HealthbarMiddle")) { ParseFloatArray(val, SettingsStore::Visuals_HealthbarMiddle); return; }
        if (key == WRAPPER_MARCO("Visuals_HealthbarBottom")) { ParseFloatArray(val, SettingsStore::Visuals_HealthbarBottom); return; }
        if (key == WRAPPER_MARCO("Visuals_HealthColor")) { ParseFloatArray(val, SettingsStore::Visuals_HealthColor); return; }
        if (key == WRAPPER_MARCO("Visuals_OccludedColor")) { ParseFloatArray(val, SettingsStore::Visuals_OccludedColor); return; }
        if (key == WRAPPER_MARCO("World_AmbienceColor")) { ParseFloatArray(val, SettingsStore::World_AmbienceColor); return; }
        if (key == WRAPPER_MARCO("World_FogColor")) { ParseFloatArray(val, SettingsStore::World_FogColor); return; }
        if (key == WRAPPER_MARCO("Whitelist_Color")) { ParseFloatArray(val, SettingsStore::Whitelist_Color); return; }

        if (AssignBool(key, val)) return;
        if (AssignInt(key, val)) return;
        if (AssignFloat(key, val)) return;
    }

    bool AssignBool(const std::string& key, const std::string& val)
    {
        OBF_PROLOGUE;
        bool b;
        if (val == "true") b = true;
        else if (val == "false") b = false;
        else return false;

#define SET_BOOL(var) if (key == WRAPPER_MARCO(#var)) { SettingsStore::var = b; return true; }
        SET_BOOL(Aimbot_Enabled);
        SET_BOOL(Aimbot_DrawFov);
        SET_BOOL(Aimbot_FovSpin);
        SET_BOOL(Aimbot_FillFov);
        SET_BOOL(Aimbot_useFov);
        SET_BOOL(Aimbot_AimbotSticky);
        SET_BOOL(Aimbot_Shake);
        SET_BOOL(Aimbot_KnockedCheck);
        SET_BOOL(Aimbot_WallCheck);
        SET_BOOL(Aimbot_ClosestPlayerFound);

        SET_BOOL(Visuals_Enabled);
        SET_BOOL(Visuals_Box);
        SET_BOOL(Visuals_Box_Fill);
        SET_BOOL(Visuals_Box_Fill_Gradient);
        SET_BOOL(Visuals_Box_Fill_Gradient_Rotate);
        SET_BOOL(Visuals_Healthbar);
        SET_BOOL(Visuals_Health);
        SET_BOOL(Visuals_Name);
        SET_BOOL(Visuals_Distance);
        SET_BOOL(Visuals_Rig_Type);
        SET_BOOL(Visuals_Tool);
        SET_BOOL(Visuals_Skeleton);
        SET_BOOL(Visuals_Chams);
        SET_BOOL(Visuals_ChamsFade);
        SET_BOOL(Visuals_WallCheck);

        SET_BOOL(World_Skybox);
        SET_BOOL(World_Rotate);
        SET_BOOL(World_Ambience);
        SET_BOOL(World_Fog);
        SET_BOOL(World_Brightness);
        SET_BOOL(World_Exposure);
        SET_BOOL(World_FOV);

        SET_BOOL(Settings_Team_Check);
        SET_BOOL(Settings_Client_Check);
        SET_BOOL(Settings_Streamproof);

        SET_BOOL(Whitelist_Enabled);

        SET_BOOL(Triggerbot_Enabled);
        SET_BOOL(Triggerbot_KnockedCheck);
        SET_BOOL(Triggerbot_WallCheck);

        SET_BOOL(Misc_Speed);
        SET_BOOL(Misc_Jump);
#undef SET_BOOL
        return false;
    }

    bool AssignInt(const std::string& key, const std::string& val)
    {
        OBF_PROLOGUE;
        try
        {
            int i = std::stoi(val);
#define SET_INT(var) if (key == WRAPPER_MARCO(#var)) { SettingsStore::var = i; return true; }
            SET_INT(Aimbot_type);
            SET_INT(Aimbot_HitPart);
            SET_INT(Aimbot_FovSpinDirection);
            SET_INT(Aimbot_FovSpinSpeed);

            SET_INT(Visuals_ChamsFadeSpeed);
            SET_INT(Visuals_BoxFillSpeed);
            SET_INT(Visuals_Healthbar_Type);
            SET_INT(Visuals_Box_Type);
            SET_INT(Visuals_Box_Fill_Type);
            SET_INT(Visuals_Name_Type);
            SET_INT(Visuals_Gap);
            SET_INT(Visuals_Thickness);

            SET_INT(World_Skybox_Type);

            SET_INT(Triggerbot_Delay);
            SET_INT(Triggerbot_Randomize);
            SET_INT(Triggerbot_HitPart);
            SET_INT(Triggerbot_FireMode);
            SET_INT(Triggerbot_Fov);

            SET_INT(Settings_Performance_Mode);
            SET_INT(WallCheck_Method);

            if (key == WRAPPER_MARCO("Triggerbot_Key")) { SettingsStore::Triggerbot_Key = (ImGuiKey)i; return true; }
            if (key == WRAPPER_MARCO("Triggerbot_Mode")) { SettingsStore::Triggerbot_Mode = (ImKeyBindMode)i; return true; }
            if (key == WRAPPER_MARCO("Aimbot_Key")) { SettingsStore::Aimbot_Key = (ImGuiKey)i; return true; }
            if (key == WRAPPER_MARCO("Aimbot_Mode")) { SettingsStore::Aimbot_Mode = (ImKeyBindMode)i; return true; }
            if (key == WRAPPER_MARCO("Aimbot_FovToggleKey")) { SettingsStore::Aimbot_FovToggleKey = (ImGuiKey)i; return true; }
            if (key == WRAPPER_MARCO("Aimbot_FovToggleMode")) { SettingsStore::Aimbot_FovToggleMode = (ImKeyBindMode)i; return true; }
            if (key == WRAPPER_MARCO("Visuals_ToggleKey")) { SettingsStore::Visuals_ToggleKey = (ImGuiKey)i; return true; }
            if (key == WRAPPER_MARCO("Visuals_ToggleMode")) { SettingsStore::Visuals_ToggleMode = (ImKeyBindMode)i; return true; }
            if (key == WRAPPER_MARCO("Misc_Speed_Key")) { SettingsStore::Misc_Speed_Key = (ImGuiKey)i; return true; }
            if (key == WRAPPER_MARCO("Misc_Speed_Key_Mode")) { SettingsStore::Misc_Speed_Key_Mode = (ImKeyBindMode)i; return true; }
            if (key == WRAPPER_MARCO("Misc_Jump_Key")) { SettingsStore::Misc_Jump_Key = (ImGuiKey)i; return true; }
            if (key == WRAPPER_MARCO("Misc_Jump_Key_Mode")) { SettingsStore::Misc_Jump_Key_Mode = (ImKeyBindMode)i; return true; }
            if (key == WRAPPER_MARCO("Settings_Menu_Keybind")) { SettingsStore::Settings_Menu_Keybind = (ImGuiKey)i; return true; }
#undef SET_INT
        }
        catch (...) {
            Logger::Log(WRAPPER_MARCO("[Config] int parse exception"));
        }
        return false;
    }

    bool AssignFloat(const std::string& key, const std::string& val)
    {
        OBF_PROLOGUE;
        try
        {
            float f = std::stof(val);
#define SET_FLOAT(var) if (key == WRAPPER_MARCO(#var)) { SettingsStore::var = f; return true; }
            SET_FLOAT(Aimbot_FovSize);
            SET_FLOAT(Aimbot_ShakeX);
            SET_FLOAT(Aimbot_ShakeY);
            SET_FLOAT(Aimbot_ShakeZ);
            SET_FLOAT(Aimbot_Mouse_SmoothingX);
            SET_FLOAT(Aimbot_Mouse_SmoothingY);
            SET_FLOAT(Aimbot_Mouse_Sensitivity);
            SET_FLOAT(Aimbot_Camera_SmoothingX);
            SET_FLOAT(Aimbot_Camera_SmoothingY);

            SET_FLOAT(Visuals_Render_Distance);

            SET_FLOAT(World_Rotate_Speed);
            SET_FLOAT(World_Fog_Distance);
            SET_FLOAT(World_FOV_Distance);
            SET_FLOAT(World_ExposureI);
            SET_FLOAT(World_BrightnessI);

            SET_FLOAT(Misc_Speed_Value);
            SET_FLOAT(Misc_Jump_Value);
#undef SET_FLOAT
        }
        catch (...) {
            Logger::Log(WRAPPER_MARCO("[Config] float parse exception"));
        }
        return false;
    }

    void ParseFloatArray(const std::string& val, float arr[4])
    {
        OBF_PROLOGUE;
        std::stringstream ss(val);
        std::string item;
        int i = 0;
        while (std::getline(ss, item, ',') && i < 4)
        {
            item.erase(0, item.find_first_not_of(" \t\r\n["));
            item.erase(item.find_last_not_of(" \t\r\n]") + 1);
            try { arr[i] = std::stof(item); }
            catch (...) {
                Logger::Log(WRAPPER_MARCO("[Config] array parse exception"));
                arr[i] = 0.0f;
            }
            i++;
        }
    }
};
