#include "Installer.h"
#include <string>
#include <cstdio>

#include <windows.h>
#include <shlobj.h>
#include <KnownFolders.h>

#include <Version.h>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Miscellaneous/Output/Output.h>
#include <Infrastructure/Logger.h>

namespace {

    static std::string S(const char* s) { return s; }
    static std::wstring WS(const wchar_t* s) { return s; }

    std::wstring GetInstallPath()
    {
        PWSTR localAppData = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData) != S_OK)
            return {};

        std::wstring path = localAppData;
        CoTaskMemFree(localAppData);
        path += WS(skCrypt(L"\\Bitware"));
        return path;
    }

    bool IsInstalled()
    {
        HKEY hKey;
        LONG result = RegOpenKeyExW(
            HKEY_CURRENT_USER,
            WS(skCrypt(L"Software\\Bitware")).c_str(),
            0,
            KEY_READ,
            &hKey
        );
        if (result != ERROR_SUCCESS)
            return false;

        wchar_t installedPath[MAX_PATH];
        DWORD size = sizeof(installedPath);
        result = RegQueryValueExW(hKey, WS(skCrypt(L"InstallPath")).c_str(), nullptr, nullptr,
            reinterpret_cast<LPBYTE>(installedPath), &size);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS)
            return false;

        DWORD attribs = GetFileAttributesW(installedPath);
        return attribs != INVALID_FILE_ATTRIBUTES;
    }

    void MarkInstalled(const std::wstring& exePath)
    {
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(), 0,
            nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
            return;

        RegSetValueExW(hKey, WS(skCrypt(L"InstallPath")).c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(exePath.c_str()),
            (DWORD)((exePath.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }

    void CreateShortcut(const std::wstring& exePath)
    {
        PWSTR startMenuPath = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_StartMenu, 0, nullptr, &startMenuPath) != S_OK)
            return;

        std::wstring lnkPath = startMenuPath;
        CoTaskMemFree(startMenuPath);
        lnkPath += WS(skCrypt(L"\\Programs\\Bitware.lnk"));

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        IShellLinkW* pShellLink = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
            IID_IShellLinkW, reinterpret_cast<void**>(&pShellLink));
        if (SUCCEEDED(hr) && pShellLink) {
            pShellLink->SetPath(exePath.c_str());
            pShellLink->SetDescription(WS(skCrypt(L"Bitware - External enhancement for Roblox")).c_str());

            IPersistFile* pPersistFile = nullptr;
            hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pPersistFile));
            if (SUCCEEDED(hr) && pPersistFile) {
                pPersistFile->Save(lnkPath.c_str(), TRUE);
                pPersistFile->Release();
            }
            pShellLink->Release();
        }

        CoUninitialize();
    }

    void WriteUninstallScript(const std::wstring& installDir)
    {
        std::wstring scriptPath = installDir + WS(skCrypt(L"\\uninstall.cmd"));

        std::wstring content =
            WS(skCrypt(L"@echo off\r\n"))
            + WS(skCrypt(L"title Bitware Uninstaller\r\n"))
            + WS(skCrypt(L"echo Removing Bitware...\r\n"))
            + WS(skCrypt(L"taskkill /f /im Bitware.exe >nul 2>&1\r\n"))
            + WS(skCrypt(L"reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\" /v Bitware /f >nul 2>&1\r\n"))
            + WS(skCrypt(L"reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Bitware\" /f >nul 2>&1\r\n"))
            + WS(skCrypt(L"reg delete \"HKCU\\Software\\Bitware\" /f >nul 2>&1\r\n"))
            + WS(skCrypt(L"del \"%USERPROFILE%\\Desktop\\Bitware.lnk\" >nul 2>&1\r\n"))
            + WS(skCrypt(L"del \"%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Bitware.lnk\" >nul 2>&1\r\n"))
            + WS(skCrypt(L"cd /d \"C:\\\"\r\n"))
            + WS(skCrypt(L"echo Done. Deleting files...\r\n"))
            + WS(skCrypt(L"start /b \"\" cmd /c \"timeout /t 2 /nobreak >nul & rd /s /q \\\""))
            + installDir + WS(skCrypt(L"\\\" >nul 2>&1\"\r\n"))
            + WS(skCrypt(L"exit\r\n"));

        HANDLE hFile = CreateFileW(scriptPath.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(hFile, content.c_str(), (DWORD)(content.size() * sizeof(wchar_t)), &written, nullptr);
            CloseHandle(hFile);
        }
    }

    void RegisterUninstall(const std::wstring& installDir)
    {
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER,
            WS(skCrypt(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Bitware")).c_str(),
            0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
            return;

        std::wstring uninstallStr = installDir + WS(skCrypt(L"\\uninstall.cmd"));
        std::wstring displayIcon = installDir + WS(skCrypt(L"\\Bitware.exe"));
        DWORD dwOne = 1;

        std::wstring displayName = WS(skCrypt(L"Bitware"));
        std::wstring displayVersion = WS(skCrypt(L"1.0.0"));
        std::wstring publisher = WS(skCrypt(L"Bitware"));

        RegSetValueExW(hKey, WS(skCrypt(L"DisplayName")).c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(displayName.c_str()),
            (DWORD)((displayName.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, WS(skCrypt(L"UninstallString")).c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(uninstallStr.c_str()),
            (DWORD)((uninstallStr.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, WS(skCrypt(L"DisplayIcon")).c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(displayIcon.c_str()),
            (DWORD)((displayIcon.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, WS(skCrypt(L"DisplayVersion")).c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(displayVersion.c_str()),
            (DWORD)((displayVersion.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, WS(skCrypt(L"Publisher")).c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(publisher.c_str()),
            (DWORD)((publisher.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, WS(skCrypt(L"InstallLocation")).c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(installDir.c_str()),
            (DWORD)((installDir.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, WS(skCrypt(L"NoModify")).c_str(), 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&dwOne), sizeof(DWORD));
        RegSetValueExW(hKey, WS(skCrypt(L"NoRepair")).c_str(), 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&dwOne), sizeof(DWORD));

        RegCloseKey(hKey);
    }

}

namespace Installer {

    std::wstring GetInstallDir()
    {
        return GetInstallPath();
    }

    void CreateDesktopShortcut()
    {
        HKEY hKey;
        LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
            0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS)
            return;

        wchar_t installedPath[MAX_PATH];
        DWORD size = sizeof(installedPath);
        result = RegQueryValueExW(hKey, WS(skCrypt(L"InstallPath")).c_str(), nullptr, nullptr,
            reinterpret_cast<LPBYTE>(installedPath), &size);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS)
            return;

        PWSTR desktopPath = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktopPath) != S_OK)
            return;

        std::wstring lnkPath = desktopPath;
        CoTaskMemFree(desktopPath);
        lnkPath += WS(skCrypt(L"\\Bitware.lnk"));

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        IShellLinkW* pShellLink = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
            IID_IShellLinkW, reinterpret_cast<void**>(&pShellLink));
        if (SUCCEEDED(hr) && pShellLink) {
            pShellLink->SetPath(installedPath);
            pShellLink->SetDescription(WS(skCrypt(L"Bitware - External enhancement for Roblox")).c_str());

            IPersistFile* pPersistFile = nullptr;
            hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pPersistFile));
            if (SUCCEEDED(hr) && pPersistFile) {
                pPersistFile->Save(lnkPath.c_str(), TRUE);
                pPersistFile->Release();
            }
            pShellLink->Release();
        }

        CoUninitialize();
    }

    bool EnsureInstalled()
    {
        std::wstring installDir = GetInstallPath();
        if (installDir.empty()) {
            Logger::Log("[Installer] failed to get install path");
            Logger::Flush();
            return false;
        }

        if (!IsInstalled()) {
            Logger::Log("[Installer] first run - installing...");
            Logger::Flush();

            if (!CreateDirectoryW(installDir.c_str(), nullptr) &&
                GetLastError() != ERROR_ALREADY_EXISTS) {
                Logger::Log("[Installer] failed to create directory");
                Logger::Flush();
                return false;
            }

            wchar_t selfPath[MAX_PATH];
            GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

            std::wstring destPath = installDir + WS(skCrypt(L"\\Bitware.exe"));

            if (!CopyFileW(selfPath, destPath.c_str(), FALSE)) {
                Logger::Log("[Installer] failed to copy executable");
                Logger::Flush();
                return false;
            }

            MarkInstalled(destPath);

            CreateShortcut(destPath);

            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER,
                WS(skCrypt(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run")).c_str(),
                0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
                RegSetValueExW(hKey, WS(skCrypt(L"Bitware")).c_str(), 0, REG_SZ,
                    reinterpret_cast<const BYTE*>(destPath.c_str()),
                    (DWORD)((destPath.size() + 1) * sizeof(wchar_t)));
                RegCloseKey(hKey);
            }

            Logger::Log("[Installer] installation complete");
        }

        WriteUninstallScript(installDir);
        RegisterUninstall(installDir);

        Logger::Flush();
        return true;
    }

}
