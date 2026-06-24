#include "Updater.h"
#include "Installer.h"
#include <string>
#include <cstdio>
#include <cstdint>

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

    std::wstring Utf8ToWide(const std::string& utf8)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (len <= 0) return {};
        std::wstring result(len - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);
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

    bool IsPE(const wchar_t* path)
    {
        HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        uint16_t magic = 0;
        DWORD read = 0;
        bool ok = ReadFile(hFile, &magic, sizeof(magic), &read, nullptr) && read == sizeof(magic);
        CloseHandle(hFile);
        return ok && magic == 0x5A4D;
    }

    bool IsZip(const wchar_t* path)
    {
        HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        uint16_t magic = 0;
        DWORD read = 0;
        bool ok = ReadFile(hFile, &magic, sizeof(magic), &read, nullptr) && read == sizeof(magic);
        CloseHandle(hFile);
        return ok && magic == 0x4B50;
    }

    bool ExtractZip(const wchar_t* zipPath, const wchar_t* destDir)
    {
        CreateDirectoryW(destDir, nullptr);

        std::wstring psCmd = WS(skCrypt(L"powershell -NoProfile -Command \""))
            + WS(skCrypt(L"Expand-Archive -Path '")) + zipPath
            + WS(skCrypt(L"' -DestinationPath '")) + destDir
            + WS(skCrypt(L"' -Force\""));

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcessW(nullptr, &psCmd[0], nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
            return false;

        WaitForSingleObject(pi.hProcess, 30000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        wchar_t extractedExe[MAX_PATH];
        swprintf(extractedExe, MAX_PATH, WS(skCrypt(L"%s\\Bitware.exe")).c_str(), destDir);
        return GetFileAttributesW(extractedExe) != INVALID_FILE_ATTRIBUTES;
    }

    std::string FindAssetUrl(const std::string& json)
    {
        std::string searchName = S(skCrypt("\"name\": \"Bitware.exe\""));
        auto namePos = json.find(searchName);
        if (namePos == std::string::npos)
            return {};

        std::string searchUrl = S(skCrypt("\"browser_download_url\""));
        auto urlPos = json.rfind(searchUrl, namePos);
        if (urlPos == std::string::npos)
            urlPos = json.find(searchUrl, namePos);

        if (urlPos == std::string::npos)
            return {};

        auto colon = json.find(':', urlPos);
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

    bool DownloadUrlToFile(const std::wstring& url, const std::wstring& outputPath)
    {
        URL_COMPONENTSW urlComp = {};
        urlComp.dwStructSize = sizeof(urlComp);
        wchar_t hostName[256] = {}, urlPath[2048] = {};
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 2048;

        if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp))
            return false;

        HINTERNET hSession = WinHttpOpen(
            WS(skCrypt(L"Bitware/1.0")).c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            nullptr, nullptr, 0);
        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

        DWORD flags = WINHTTP_FLAG_SECURE;
        if (urlComp.nPort == INTERNET_DEFAULT_HTTP_PORT) flags = 0;

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, WS(skCrypt(L"GET")).c_str(),
            urlPath, nullptr, nullptr, nullptr, flags);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        bool ok = false;
        if (WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, nullptr)) {

            HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, nullptr,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE) {
                char buffer[65536];
                DWORD bytesRead = 0;
                while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                    DWORD written;
                    WriteFile(hFile, buffer, bytesRead, &written, nullptr);
                }
                CloseHandle(hFile);
                ok = true;
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return ok;
    }

    bool StageUpdate(const wchar_t* updateExe)
    {
        wchar_t selfPath[MAX_PATH];
        GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);

        wchar_t batchPath[MAX_PATH];
        swprintf(batchPath, MAX_PATH, WS(skCrypt(L"%sBitware_updater.bat")).c_str(), tempPath);

        std::string updateExeUtf8 = WideToUtf8(updateExe);
        std::string selfPathUtf8 = WideToUtf8(selfPath);

        std::string batch;
        batch += S(skCrypt("@echo off\r\n"));
        batch += S(skCrypt(":loop\r\n"));
        batch += S(skCrypt("tasklist /fi \"IMAGENAME eq Bitware.exe\" 2>nul | find /i \"Bitware.exe\" >nul\r\n"));
        batch += S(skCrypt("if not errorlevel 1 (\r\n"));
        batch += S(skCrypt("    timeout /t 1 /nobreak >nul\r\n"));
        batch += S(skCrypt("    goto loop\r\n"));
        batch += S(skCrypt(")\r\n"));
        batch += S(skCrypt("move /y \"")) + updateExeUtf8 + S(skCrypt("\" \"")) + selfPathUtf8 + S(skCrypt("\"\r\n"));
        batch += S(skCrypt("if errorlevel 1 (\r\n"));
        batch += S(skCrypt("    del /f /q \"")) + updateExeUtf8 + S(skCrypt("\"\r\n"));
        batch += S(skCrypt("    exit /b 1\r\n"));
        batch += S(skCrypt(")\r\n"));
        batch += S(skCrypt("start \"\" \"")) + selfPathUtf8 + S(skCrypt("\"\r\n"));
        batch += S(skCrypt("del \"%~f0\"\r\n"));

        HANDLE hFile = CreateFileW(batchPath, GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        DWORD written;
        WriteFile(hFile, batch.c_str(), (DWORD)batch.size(), &written, nullptr);
        CloseHandle(hFile);

        Logger::Log(S(skCrypt("[Updater] launching updater batch")).c_str());
        Logger::Flush();

        ShellExecuteA(nullptr, S(skCrypt("open")).c_str(), WideToUtf8(batchPath).c_str(),
            nullptr, nullptr, SW_HIDE);

        return true;
    }

}

namespace Updater {

    void CheckForUpdates()
    {
        Logger::Log(S(skCrypt("[Updater] checking for updates...")).c_str());
        Logger::Flush();

        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);

        wchar_t updateExe[MAX_PATH];
        swprintf(updateExe, MAX_PATH, WS(skCrypt(L"%sBitware_update.exe")).c_str(), tempPath);

        if (IsPE(updateExe)) {
            Logger::Log(S(skCrypt("[Updater] staged update found, applying")).c_str());
            Logger::Flush();
            StageUpdate(updateExe);
            Api::ExitProcess(0);
            return;
        }

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
            Output::Success(S(skCrypt("Already up to date (v%s)")).c_str(), currentVersion.c_str());

            DeleteFileW(updateExe);
            return;
        }

        Logger::Log(S(skCrypt("[Updater] update available!")).c_str());
        Logger::Flush();

        Output::Info(S(skCrypt("Downloading v%s...")).c_str(), remoteStripped.c_str());

        std::string downloadUrl = FindAssetUrl(json);
        if (downloadUrl.empty()) {
            Logger::Log(S(skCrypt("[Updater] could not find Bitware.exe asset URL")).c_str());
            Logger::Flush();
            return;
        }

        std::wstring wideUrl = Utf8ToWide(downloadUrl);
        if (wideUrl.empty()) {
            Logger::Log(S(skCrypt("[Updater] failed to convert download URL")).c_str());
            Logger::Flush();
            return;
        }

        if (!DownloadUrlToFile(wideUrl, updateExe)) {
            Logger::Log(S(skCrypt("[Updater] download failed")).c_str());
            Logger::Flush();
            return;
        }

        if (IsZip(updateExe)) {
            Logger::Log(S(skCrypt("[Updater] downloaded file is a zip, extracting...")).c_str());
            Logger::Flush();

            wchar_t extractDir[MAX_PATH];
            swprintf(extractDir, MAX_PATH, WS(skCrypt(L"%sBitware_extract\\")).c_str(), tempPath);

            if (!ExtractZip(updateExe, extractDir)) {
                Logger::Log(S(skCrypt("[Updater] failed to extract zip")).c_str());
                Logger::Flush();
                DeleteFileW(updateExe);
                return;
            }

            wchar_t extractedExe[MAX_PATH];
            swprintf(extractedExe, MAX_PATH, WS(skCrypt(L"%sBitware.exe")).c_str(), extractDir);

            if (!IsPE(extractedExe)) {
                Logger::Log(S(skCrypt("[Updater] extracted file is not a valid executable")).c_str());
                Logger::Flush();
                DeleteFileW(updateExe);
                return;
            }

            if (!MoveFileExW(extractedExe, updateExe, MOVEFILE_REPLACE_EXISTING)) {
                Logger::Log(S(skCrypt("[Updater] failed to move extracted exe")).c_str());
                Logger::Flush();
                DeleteFileW(updateExe);
                return;
            }
        }

        if (!IsPE(updateExe)) {
            Logger::Log(S(skCrypt("[Updater] downloaded file is not a valid executable")).c_str());
            Logger::Flush();
            DeleteFileW(updateExe);
            return;
        }

        Logger::Log(S(skCrypt("[Updater] download complete, applying update")).c_str());
        Logger::Flush();

        Output::Success(S(skCrypt("Update downloaded, restarting...")).c_str());

        StageUpdate(updateExe);
        Api::ExitProcess(0);
    }

}
