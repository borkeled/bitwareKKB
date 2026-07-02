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
#include "Loader/SplashWindow.h"
#include "Version.h"
#include "Auth/skStr.h"

#pragma comment(lib, "Shell32.lib")

namespace {

    using namespace LoaderConsole;

    static std::string S(const char* s) { return s; }
    static std::wstring WS(const wchar_t* s) { return s; }

    std::string WideToUtf8(const wchar_t* wide) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return {};
        std::string result(len - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], len, nullptr, nullptr);
        return result;
    }

    bool SafeEnsureInstalled() {
        __try { return Installer::EnsureInstalled(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    bool IsInstalled() {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
            0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return false;
        RegCloseKey(hKey);
        return true;
    }

    std::wstring GetInstalledExePath() {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
            0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return {};

        wchar_t path[MAX_PATH];
        DWORD size = sizeof(path);
        LONG result = RegQueryValueExW(hKey, WS(skCrypt(L"InstallPath")).c_str(),
            nullptr, nullptr, reinterpret_cast<LPBYTE>(path), &size);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) return {};
        return path;
    }

    std::wstring GetInstallDir() {
        auto exePath = GetInstalledExePath();
        if (exePath.empty()) return {};
        auto pos = exePath.rfind('\\');
        if (pos != std::wstring::npos)
            return exePath.substr(0, pos);
        return {};
    }

    std::wstring GetLocalAppDataPath() {
        PWSTR path = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path) != S_OK)
            return {};
        std::wstring result = path;
        CoTaskMemFree(path);
        return result;
    }

    void SelfDestruct(const wchar_t* selfPath) {
        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);

        wchar_t batchPath[MAX_PATH];
        swprintf(batchPath, MAX_PATH, WS(skCrypt(L"%sBitware_cleanup.bat")).c_str(), tempPath);

        std::string selfUtf8 = WideToUtf8(selfPath);
        std::string batch;
        batch += S(skCrypt("@echo off\r\n"));
        batch += S(skCrypt(":loop\r\n"));
        batch += S(skCrypt("tasklist /fi \"IMAGENAME eq Bitware.exe\" 2>nul | find /i \"Bitware.exe\" >nul\r\n"));
        batch += S(skCrypt("if not errorlevel 1 (\r\n"));
        batch += S(skCrypt("    timeout /t 1 /nobreak >nul\r\n"));
        batch += S(skCrypt("    goto loop\r\n"));
        batch += S(skCrypt(")\r\n"));
        batch += S(skCrypt("del /f /q \"")) + selfUtf8 + S(skCrypt("\"\r\n"));
        batch += S(skCrypt("del \"%~f0\"\r\n"));

        HANDLE hFile = CreateFileW(batchPath, GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(hFile, batch.c_str(), (DWORD)batch.size(), &written, nullptr);
            CloseHandle(hFile);
        }

        ShellExecuteA(nullptr, S(skCrypt("open")).c_str(), WideToUtf8(batchPath).c_str(),
            nullptr, nullptr, SW_HIDE);
    }

    bool IsInDefenderExclusions(const std::wstring& path) {
        std::wstring cmd = WS(skCrypt(L"-NoProfile -Command \"$p = (Get-MpPreference).ExclusionPath; "
            L"if ($p -contains '")) + path + WS(skCrypt(L"') { exit 0 } else { exit 1 }\""));

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
            return false;

        WaitForSingleObject(pi.hProcess, 15000);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (exitCode == 0);
    }

    void TryAddDefenderExclusion(const std::wstring& bitwarePath) {
        HKEY hKey;
        DWORD askedState = 0;
        DWORD size = sizeof(askedState);
        bool hasAsked = (RegOpenKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
            0, KEY_READ, &hKey) == ERROR_SUCCESS);
        if (hasAsked) {
            RegQueryValueExW(hKey, WS(skCrypt(L"DefenderAsked")).c_str(),
                nullptr, nullptr, reinterpret_cast<LPBYTE>(&askedState), &size);
            RegCloseKey(hKey);
        }

        if (askedState != 0) return;

        bool isExcluded = IsInDefenderExclusions(bitwarePath);
        if (isExcluded) {
            askedState = 1;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
                0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
                RegSetValueExW(hKey, WS(skCrypt(L"DefenderAsked")).c_str(), 0, REG_DWORD,
                    reinterpret_cast<const BYTE*>(&askedState), sizeof(askedState));
                RegCloseKey(hKey);
            }
            return;
        }

        int choice = MessageBoxA(
            GetConsoleWindow(),
            S(skCrypt("Do you want to add Bitware to antivirus exclusions?\n\n"
                "This prevents your antivirus from blocking Bitware.\n"
                "Your antivirus will continue to protect you from real threats.")).c_str(),
            S(skCrypt("Bitware Loader")).c_str(),
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1
        );

        askedState = (choice == IDYES) ? 1 : 2;

        if (choice == IDYES) {
            SplashWindow::SetStatus(SplashWindow::Status::Idle, "Defender",
                S(skCrypt("Adding exclusion...")).c_str());

            std::wstring psCmd = WS(skCrypt(L"-Command Add-MpPreference -ExclusionPath \""))
                + bitwarePath + WS(skCrypt(L"\""));
            ShellExecuteW(nullptr, WS(skCrypt(L"open")).c_str(),
                WS(skCrypt(L"powershell.exe")).c_str(), psCmd.c_str(),
                nullptr, SW_HIDE);

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (RegCreateKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
            0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, WS(skCrypt(L"DefenderAsked")).c_str(), 0, REG_DWORD,
                reinterpret_cast<const BYTE*>(&askedState), sizeof(askedState));
            RegCloseKey(hKey);
        }
    }

    void SyncAndSelfDeleteIfPortable() {
        wchar_t selfPath[MAX_PATH];
        GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

        std::wstring installedPath = GetInstalledExePath();
        if (installedPath.empty()) return;

        if (_wcsicmp(selfPath, installedPath.c_str()) == 0) return;

        SplashWindow::SetStatus(SplashWindow::Status::Idle, "Sync",
            S(skCrypt("Syncing to installed location...")).c_str());

        if (!CopyFileW(selfPath, installedPath.c_str(), FALSE)) {
            Logger::Log(S(skCrypt("[Sync] Failed to copy to installed location")).c_str());
            return;
        }

        Logger::Log(S(skCrypt("[Sync] Copied to installed location, self-destructing")).c_str());

        SelfDestruct(selfPath);

        ShellExecuteW(nullptr, WS(skCrypt(L"open")).c_str(), installedPath.c_str(),
            nullptr, nullptr, SW_SHOWNORMAL);

        SplashWindow::Close();
        Api::ExitProcess(0);
    }

    BOOL CALLBACK EnumRobloxWindow(HWND hwnd, LPARAM lParam) {
        wchar_t title[128];
        int len = GetWindowTextW(hwnd, title, 128);
        if (len > 0 && wcsstr(title, WS(skCrypt(L"Roblox")).c_str()) != nullptr) {
            *reinterpret_cast<bool*>(lParam) = true;
            return FALSE;
        }
        return TRUE;
    }

    bool IsRobloxRunning() {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            bool found = false;
            EnumWindows(EnumRobloxWindow, reinterpret_cast<LPARAM>(&found));
            return found;
        }

        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(snapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, WS(skCrypt(L"RobloxPlayerBeta.exe")).c_str()) == 0) {
                    CloseHandle(snapshot);
                    return true;
                }
            } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);
        return false;
    }

    void WaitForRobloxWithSplash() {
        SplashWindow::SetStatus(SplashWindow::Status::Waiting, "Roblox",
            S(skCrypt("Waiting for RobloxPlayerBeta.exe...")).c_str());

        const int timeout_ms = 30000;
        int waited = 0;

        while (!IsRobloxRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            waited += 400;
            if (!SplashWindow::IsRunning() || waited >= timeout_ms)
                break;
        }

        if (SplashWindow::IsRunning()) {
            SplashWindow::SetStatus(SplashWindow::Status::Ready, "Roblox",
                S(skCrypt("Roblox detected")).c_str());
        }
    }

}

std::string tm_to_readable_time(tm ctx);
static std::time_t string_to_timet(std::string timestamp);
static std::tm timet_to_tm(time_t timestamp);
const std::string compilation_date = (std::string)skCrypt(__DATE__);
const std::string compilation_time = (std::string)skCrypt(__TIME__);
void sessionStatus();

std::int32_t main(std::int32_t argc, char** argv[]) {
    LoaderConsole::Init(WS(skCrypt(L"Bitware Loader")).c_str());
    LoaderConsole::Hide();

    SplashWindow::Create();

    std::wstring installDir = GetInstallDir();

    if (!installDir.empty() && GetFileAttributesW(installDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
        SplashWindow::SetStatus(SplashWindow::Status::Checking, "Update",
            S(skCrypt("Cleaning up...")).c_str());
        Updater::CleanupAfterUpdate(installDir.c_str());
    }

    SplashWindow::SetStatus(SplashWindow::Status::Checking, "Update",
        S(skCrypt("Checking for updates...")).c_str());

    auto updateInfo = Updater::CheckForUpdate();

    if (updateInfo.updateAvailable) {
        SplashWindow::SetStatus(SplashWindow::Status::Downloading, "Update",
            S(skCrypt("Downloading...")).c_str());

        if (Updater::DownloadAndApply(updateInfo, SplashWindow::SetDownloadProgress)) {
            SplashWindow::Close();
            Api::ExitProcess(0);
            return 0;
        }

        SplashWindow::SetStatus(SplashWindow::Status::Error, "Update",
            S(skCrypt("Update failed, continuing with current version")).c_str());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } else {
        SplashWindow::SetStatus(SplashWindow::Status::Ready, "Update",
            S(skCrypt("Ready")).c_str());
    }

    SplashWindow::SetStatus(SplashWindow::Status::Installing, "Install",
        S(skCrypt("Verifying installation...")).c_str());

    bool freshInstall = !IsInstalled();

    SafeEnsureInstalled();

    if (freshInstall) {
        SplashWindow::SetStatus(SplashWindow::Status::Ready, "Install",
            S(skCrypt("Ready")).c_str());

        TryAddDefenderExclusion(GetLocalAppDataPath() + WS(skCrypt(L"\\Bitware")));

        int choice = MessageBoxA(
            GetConsoleWindow(),
            S(skCrypt("Would you like to add a Bitware shortcut to your desktop?\n\n"
                "This makes it easy to launch Bitware directly.")).c_str(),
            S(skCrypt("Bitware Loader")).c_str(),
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1
        );

        if (choice == IDYES) {
            SplashWindow::SetStatus(SplashWindow::Status::Idle, "Desktop",
                S(skCrypt("Creating desktop shortcut...")).c_str());
            Installer::CreateDesktopShortcut();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    } else {
        SplashWindow::SetStatus(SplashWindow::Status::Ready, "Install",
            S(skCrypt("Ready")).c_str());

        TryAddDefenderExclusion(GetLocalAppDataPath() + WS(skCrypt(L"\\Bitware")));

        SyncAndSelfDeleteIfPortable();
    }

    if (SplashWindow::IsRunning()) {
        WaitForRobloxWithSplash();
    }

    if (SplashWindow::IsRunning()) {
        for (int i = 0; i <= 100; i++) {
            SplashWindow::SetProgress(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }

        SplashWindow::SetStatus(SplashWindow::Status::Launching, "Launch",
            S(skCrypt("Starting Bitware...")).c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    SplashWindow::Close();

    Application app;

    if (!app.Init()) {
        const char* reason = InitFailureReason ? InitFailureReason : WRAPPER_MARCO("Unknown error");
        Api::MessageBoxA(nullptr, reason, WRAPPER_MARCO("Bitware - Initialization Failed"), MB_ICONERROR | MB_OK);
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
