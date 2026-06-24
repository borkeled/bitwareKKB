#include "Updater.h"
#include "Installer.h"
#include <string>
#include <cstdio>

#include <windows.h>
#include <winhttp.h>

#include <Version.h>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Miscellaneous/Output/Output.h>
#include <Infrastructure/Logger.h>

namespace {

    std::string WideToUtf8(const wchar_t* wide)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return {};
        std::string result(len - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], len, nullptr, nullptr);
        return result;
    }

    static std::string S(const char* s) { return s; }
    static std::wstring WS(const wchar_t* s) { return s; }

    std::string GetApiResponse(const std::wstring& host, const std::wstring& path)
    {
        HINTERNET hSession = WinHttpOpen(
            WS(skCrypt(L"Bitware/1.0")).c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            nullptr,
            nullptr,
            0
        );
        if (!hSession)
            return {};

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return {};
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            WS(skCrypt(L"GET")).c_str(),
            path.c_str(),
            nullptr,
            nullptr,
            nullptr,
            WINHTTP_FLAG_SECURE
        );
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return {};
        }

        WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0);
        WinHttpReceiveResponse(hRequest, nullptr);

        std::string result;
        char buffer[4096];
        DWORD bytesRead = 0;
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            result.append(buffer, bytesRead);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    std::string FindJsonString(const std::string& json, const std::string& key)
    {
        std::string search = S(skCrypt("\"")) + key + S(skCrypt("\""));
        auto pos = json.find(search);
        if (pos == std::string::npos)
            return {};

        auto colon = json.find(':', pos);
        if (colon == std::string::npos)
            return {};

        auto start = json.find_first_of(S(skCrypt("\"")), colon + 1);
        if (start == std::string::npos)
            return {};

        auto end = json.find_first_of(S(skCrypt("\"")), start + 1);
        if (end == std::string::npos)
            return {};

        return json.substr(start + 1, end - start - 1);
    }

    std::string StripV(const std::string& version)
    {
        if (!version.empty() && version[0] == 'v')
            return version.substr(1);
        return version;
    }

    void ApplyUpdate()
    {
        wchar_t selfPath[MAX_PATH];
        GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

        std::wstring installDir = Installer::GetInstallDir();
        if (installDir.empty())
            return;

        std::wstring selfDir = selfPath;
        auto pos = selfDir.rfind(L'\\');
        if (pos != std::wstring::npos)
            selfDir.resize(pos + 1);

        std::wstring installDirSlash = installDir + WS(skCrypt(L"\\"));
        if (_wcsicmp(selfDir.c_str(), installDirSlash.c_str()) != 0)
            return;

        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);

        wchar_t updateExe[MAX_PATH];
        swprintf(updateExe, MAX_PATH, WS(skCrypt(L"%sBitware_update.exe")).c_str(), tempPath);

        wchar_t batchPath[MAX_PATH];
        swprintf(batchPath, MAX_PATH, WS(skCrypt(L"%sBitware_updater.bat")).c_str(), tempPath);

        std::string updateExeUtf8 = WideToUtf8(updateExe);
        std::string selfPathUtf8 = WideToUtf8(selfPath);
        std::string batchPathUtf8 = WideToUtf8(batchPath);

        std::string batch;
        batch += S(skCrypt("@echo off\r\n"));
        batch += S(skCrypt(":loop\r\n"));
        batch += S(skCrypt("tasklist /fi \"IMAGENAME eq Bitware.exe\" 2>nul | find /i \"Bitware.exe\" >nul\r\n"));
        batch += S(skCrypt("if not errorlevel 1 (\r\n"));
        batch += S(skCrypt("    timeout /t 1 /nobreak >nul\r\n"));
        batch += S(skCrypt("    goto loop\r\n"));
        batch += S(skCrypt(")\r\n"));
        batch += S(skCrypt("move /y \"")) + updateExeUtf8 + S(skCrypt("\" \"")) + selfPathUtf8 + S(skCrypt("\"\r\n"));
        batch += S(skCrypt("start \"\" \"")) + selfPathUtf8 + S(skCrypt("\"\r\n"));
        batch += S(skCrypt("del \"%~f0\"\r\n"));

        HANDLE hFile = CreateFileW(batchPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return;

        DWORD written;
        WriteFile(hFile, batch.c_str(), (DWORD)batch.size(), &written, nullptr);
        CloseHandle(hFile);

        Logger::Log(S(skCrypt("[Updater] launching updater batch")).c_str());
        Logger::Flush();

        ShellExecuteA(nullptr, S(skCrypt("open")).c_str(), batchPathUtf8.c_str(), nullptr, nullptr, SW_HIDE);

        Api::ExitProcess(0);
    }

}

namespace Updater {

    void CheckForUpdates()
    {
        Logger::Log(S(skCrypt("[Updater] checking for updates...")).c_str());
        Logger::Flush();

        std::string json = GetApiResponse(
            WS(skCrypt(L"api.github.com")),
            WS(skCrypt(L"/repos/borkeled/Bitware-Releases/releases/latest"))
        );

        if (json.empty()) {
            Logger::Log(S(skCrypt("[Updater] no response from server")).c_str());
            Logger::Flush();
            return;
        }

        std::string remoteVersion = FindJsonString(json, S(skCrypt("tag_name")));
        if (remoteVersion.empty()) {
            Logger::Log(S(skCrypt("[Updater] could not parse version")).c_str());
            Logger::Flush();
            return;
        }

        Logger::Log((S(skCrypt("[Updater] remote version: ")) + remoteVersion).c_str());
        Logger::Flush();

        std::string currentVersion = StripV(BITWARE_VERSION_STRING);
        std::string remoteStripped = StripV(remoteVersion);

        if (remoteStripped == currentVersion) {
            Logger::Log(S(skCrypt("[Updater] already up to date")).c_str());
            Logger::Flush();
            return;
        }

        Logger::Log(S(skCrypt("[Updater] update available!")).c_str());
        Logger::Flush();

        std::string downloadUrl = FindJsonString(json, S(skCrypt("browser_download_url")));
        if (downloadUrl.empty()) {
            Logger::Log(S(skCrypt("[Updater] no download URL found")).c_str());
            Logger::Flush();
            return;
        }

        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);

        wchar_t updateExe[MAX_PATH];
        swprintf(updateExe, MAX_PATH, WS(skCrypt(L"%sBitware_update.exe")).c_str(), tempPath);

        std::wstring wideUrl;
        {
            int len = MultiByteToWideChar(CP_UTF8, 0, downloadUrl.c_str(), -1, nullptr, 0);
            if (len > 0) {
                wideUrl.resize(len - 1);
                MultiByteToWideChar(CP_UTF8, 0, downloadUrl.c_str(), -1, &wideUrl[0], len);
            }
        }

        URL_COMPONENTSW urlComp = {};
        urlComp.dwStructSize = sizeof(urlComp);
        wchar_t hostName[256] = {}, urlPath[2048] = {};
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 2048;

        if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp)) {
            Logger::Log(S(skCrypt("[Updater] failed to parse download URL")).c_str());
            Logger::Flush();
            return;
        }

        HINTERNET hSession = WinHttpOpen(
            WS(skCrypt(L"Bitware/1.0")).c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            nullptr,
            nullptr,
            0
        );
        if (!hSession) return;

        HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return;
        }

        DWORD flags = WINHTTP_FLAG_SECURE;
        if (urlComp.nPort == INTERNET_DEFAULT_HTTP_PORT)
            flags = 0;

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            WS(skCrypt(L"GET")).c_str(),
            urlPath,
            nullptr,
            nullptr,
            nullptr,
            flags
        );
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0);
        WinHttpReceiveResponse(hRequest, nullptr);

        HANDLE hFile = CreateFileW(updateExe, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        char buffer[65536];
        DWORD bytesRead = 0;
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            DWORD written;
            WriteFile(hFile, buffer, bytesRead, &written, nullptr);
        }

        CloseHandle(hFile);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        Logger::Log(S(skCrypt("[Updater] download complete, applying update")).c_str());
        Logger::Flush();

        ApplyUpdate();
    }

}
