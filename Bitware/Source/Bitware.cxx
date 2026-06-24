#include <iostream>
#include <thread>
#include <chrono>

#include <windows.h>
#include <ShlObj.h>
#include <KnownFolders.h>

#include "Driver/Driver.h"
#include "Globals.hxx"
#include "Miscellaneous/Output/Output.h"
#include "Core/Graphics/Graphics.h"
#include "Core/Application.h"
#include "Core/UI/UIBridge.h"
#include "Miscellaneous/Protection/External/oxorany_include.h"
#include "Updater/Updater.h"
#include "Updater/Installer.h"
#include "Infrastructure/Logger.h"
#include "Loader/LoaderConsole.h"
#include "Version.h"
#include "Auth/skStr.h"

#pragma comment(lib, "Shell32.lib")

namespace {

    using namespace LoaderConsole;

    static std::string S(const char* s) { return s; }
    static std::wstring WS(const wchar_t* s) { return s; }

    void SafeCheckForUpdates() {
        __try { Updater::CheckForUpdates(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void SafeEnsureInstalled() {
        __try { Installer::EnsureInstalled(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    BOOL CALLBACK EnumRobloxWindow(HWND hwnd, LPARAM lParam)
    {
        wchar_t title[128];
        int len = GetWindowTextW(hwnd, title, 128);
        if (len > 0 && wcsstr(title, WS(skCrypt(L"Roblox")).c_str()) != nullptr) {
            *reinterpret_cast<bool*>(lParam) = true;
            return FALSE;
        }
        return TRUE;
    }

    bool IsRobloxRunning()
    {
        bool found = false;
        EnumWindows(EnumRobloxWindow, reinterpret_cast<LPARAM>(&found));
        return found;
    }

    std::wstring GetLocalAppDataPath()
    {
        PWSTR path = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path) != S_OK)
            return {};
        std::wstring result = path;
        CoTaskMemFree(path);
        return result;
    }

    bool HasDefenderBeenAsked()
    {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
            0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return false;

        DWORD value = 0;
        DWORD size = sizeof(value);
        LONG result = RegQueryValueExW(hKey, WS(skCrypt(L"DefenderAsked")).c_str(),
            nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size);
        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS && value != 0);
    }

    void MarkDefenderAsked()
    {
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
            0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
            return;

        DWORD value = 1;
        RegSetValueExW(hKey, WS(skCrypt(L"DefenderAsked")).c_str(), 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&value), sizeof(value));
        RegCloseKey(hKey);
    }

    void TryAddDefenderExclusion()
    {
        if (HasDefenderBeenAsked())
            return;

        LoaderConsole::PrintLine();
        LoaderConsole::SetColor(Cyan);
        printf(skCrypt("  ─────────────────────────────────────────\n"));
        LoaderConsole::SetColor(White);

        LoaderConsole::PrintStatus(S(skCrypt("Defender")).c_str(),
            S(skCrypt("Windows Defender exclusion not configured.")).c_str());

        int choice = MessageBoxA(
            GetConsoleWindow(),
            S(skCrypt("Add Bitware to Windows Defender exclusions?\n\n"
                      "This prevents false positives and ensures smooth operation.\n"
                      "Your antivirus will still protect you from real threats.\n"
                      "\n"
                      "Recommended: Yes")).c_str(),
            S(skCrypt("Bitware Loader")).c_str(),
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1
        );

        if (choice == IDYES) {
            std::wstring installDir = GetLocalAppDataPath();
            if (!installDir.empty()) {
                installDir += WS(skCrypt(L"\\Bitware"));
                std::wstring cmd = WS(skCrypt(L"-Command Add-MpPreference -ExclusionPath \""))
                    + installDir + WS(skCrypt(L"\""));

                LoaderConsole::PrintStatus(S(skCrypt("Defender")).c_str(),
                    S(skCrypt("Adding exclusion...")).c_str());

                ShellExecuteW(nullptr, WS(skCrypt(L"open")).c_str(),
                    WS(skCrypt(L"powershell.exe")).c_str(), cmd.c_str(),
                    nullptr, SW_HIDE);

                LoaderConsole::PrintSuccess(S(skCrypt("Exclusion added")).c_str());
            }
        }

        MarkDefenderAsked();
    }

}

std::string tm_to_readable_time(tm ctx);
static std::time_t string_to_timet(std::string timestamp);
static std::tm timet_to_tm(time_t timestamp);
const std::string compilation_date = (std::string)skCrypt(__DATE__);
const std::string compilation_time = (std::string)skCrypt(__TIME__);
void sessionStatus();

std::int32_t main(std::int32_t argc, char** argv[])
{
    LoaderConsole::Init(WS(skCrypt(L"Bitware Loader")).c_str());

    LoaderConsole::SetColor(Cyan);
    printf(skCrypt("  ╔══════════════════════════════════════╗\n"));
    printf(skCrypt("  ║           Bitware Loader             ║\n"));
    printf(skCrypt("  ╚══════════════════════════════════════╝\n\n"));

    LoaderConsole::SetColor(White);

    PrintStatus(S(skCrypt("Update")).c_str(), S(skCrypt("Checking...")).c_str());
    SafeCheckForUpdates();
    PrintSuccess(S(skCrypt("Ready")).c_str());

    HKEY _hk;
    bool freshInstall = RegOpenKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
        0, KEY_READ, &_hk) != ERROR_SUCCESS;
    if (!freshInstall)
        RegCloseKey(_hk);

    PrintStatus(S(skCrypt("Install")).c_str(), S(skCrypt("Verifying...")).c_str());
    SafeEnsureInstalled();
    PrintSuccess(S(skCrypt("Ready")).c_str());

    if (freshInstall) {
        LoaderConsole::PrintLine();
        LoaderConsole::SetColor(Cyan);
        printf(skCrypt("  ─────────────────────────────────────────\n"));
        LoaderConsole::SetColor(White);

        int choice = MessageBoxA(
            GetConsoleWindow(),
            S(skCrypt("Would you like to add a Bitware shortcut to your desktop?\n\n"
                      "This makes it easy to launch Bitware directly.")).c_str(),
            S(skCrypt("Bitware Loader")).c_str(),
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1
        );

        if (choice == IDYES) {
            LoaderConsole::PrintStatus(S(skCrypt("Desktop")).c_str(),
                S(skCrypt("Creating desktop shortcut...")).c_str());
            Installer::CreateDesktopShortcut();
            LoaderConsole::PrintSuccess(S(skCrypt("Shortcut created")).c_str());
        }
    }

    TryAddDefenderExclusion();

    LoaderConsole::PrintLine();
    LoaderConsole::PrintStatus(S(skCrypt("Roblox")).c_str(),
        S(skCrypt("Waiting for RobloxPlayerBeta.exe...")).c_str());

    SetCursorVisible(false);
    int dotFrame = 0;
    while (!IsRobloxRunning()) {
        SetColor(Cyan);
        static const char* dots[] = { "   ", ".  ", ".. ", "..." };
        printf(skCrypt("\r  ● Waiting for Roblox%s"), dots[dotFrame & 3]);
        fflush(stdout);
        dotFrame++;
        SetColor(White);
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
    SetCursorVisible(true);

    LoaderConsole::PrintLine();
    LoaderConsole::PrintLine();
    PrintSuccess(S(skCrypt("Roblox detected")).c_str());

    for (int i = 0; i <= 100; i++) {
        DrawProgressBar(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    LoaderConsole::PrintLine();
    LoaderConsole::PrintLine();
    SetColor(Green);
    printf(skCrypt("  ──▶ Launching Bitware...\n\n"));
    SetColor(White);

    LoaderConsole::Hide();

    Application app;

    if (!app.Init())
    {
        Api::MessageBoxA(nullptr, WRAPPER_MARCO("Failed to initialize"), WRAPPER_MARCO("Error"), MB_ICONERROR | MB_OK);
        return 1;
    }

    app.Run();

    return 0;
}

std::string tm_to_readable_time(tm ctx) {
    char buffer[80];
    strftime(buffer, sizeof(buffer), WRAPPER_MARCO("%a %m/%d/%y %H:%M:%S %Z"), &ctx);
    return std::string(buffer);
}

static std::time_t string_to_timet(std::string timestamp) {
    auto cv = strtol(timestamp.c_str(), NULL, 10);
    return (time_t)cv;
}

static std::tm timet_to_tm(time_t timestamp) {
    std::tm context;
    localtime_s(&context, &timestamp);
    return context;
}
