#include "Updater.h"
#include <string>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <Shellapi.h>

#include <Version.h>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Miscellaneous/Output/Output.h>
#include <Infrastructure/Logger.h>

#pragma comment(lib, "bcrypt.lib")

namespace {

    static std::string S(const char* s) { return s; }
    static std::wstring WS(const wchar_t* s) { return s; }

    std::string WideToUtf8(const wchar_t* wide) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return {};
        std::string result(len - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], len, nullptr, nullptr);
        return result;
    }

    std::wstring Utf8ToWide(const std::string& utf8) {
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (len <= 0) return {};
        std::wstring result(len - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);
        return result;
    }

    struct Version {
        int major = 0, minor = 0, patch = 0;
        bool valid = false;
    };

    Version ParseVersion(const std::string& str) {
        Version v;
        std::string s = str;
        if (!s.empty() && s[0] == 'v') s = s.substr(1);

        size_t p1 = s.find('.');
        if (p1 == std::string::npos) return v;
        size_t p2 = s.find('.', p1 + 1);
        if (p2 == std::string::npos) return v;

        v.major = std::atoi(s.substr(0, p1).c_str());
        v.minor = std::atoi(s.substr(p1 + 1, p2 - p1 - 1).c_str());
        v.patch = std::atoi(s.substr(p2 + 1).c_str());
        v.valid = true;
        return v;
    }

    bool IsNewerVersion(const Version& current, const Version& remote) {
        if (!current.valid || !remote.valid) return false;
        if (remote.major > current.major) return true;
        if (remote.major < current.major) return false;
        if (remote.minor > current.minor) return true;
        if (remote.minor < current.minor) return false;
        return remote.patch > current.patch;
    }

    std::string FindJsonString(const std::string& json, const std::string& key) {
        std::string search = S(skCrypt("\"")) + key + S(skCrypt("\""));
        auto pos = json.find(search);
        if (pos == std::string::npos) return {};

        auto colon = json.find(':', pos);
        if (colon == std::string::npos) return {};

        auto start = json.find_first_of(S(skCrypt("\"")), colon + 1);
        if (start == std::string::npos) return {};

        auto end = json.find_first_of(S(skCrypt("\"")), start + 1);
        if (end == std::string::npos) return {};

        return json.substr(start + 1, end - start - 1);
    }

    std::string FindAssetUrl(const std::string& json) {
        std::string searchName = S(skCrypt("\"name\": \"Bitware.exe\""));
        auto namePos = json.find(searchName);
        if (namePos == std::string::npos) return {};

        std::string searchUrl = S(skCrypt("\"browser_download_url\""));
        auto urlPos = json.rfind(searchUrl, namePos);
        if (urlPos == std::string::npos)
            urlPos = json.find(searchUrl, namePos);
        if (urlPos == std::string::npos) return {};

        auto colon = json.find(':', urlPos);
        if (colon == std::string::npos) return {};

        auto start = json.find_first_of(S(skCrypt("\"")), colon + 1);
        if (start == std::string::npos) return {};

        auto end = json.find_first_of(S(skCrypt("\"")), start + 1);
        if (end == std::string::npos) return {};

        return json.substr(start + 1, end - start - 1);
    }

    bool IsPE(const wchar_t* path) {
        HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        uint16_t magic = 0;
        DWORD read = 0;
        bool ok = ReadFile(hFile, &magic, sizeof(magic), &read, nullptr) && read == sizeof(magic);
        CloseHandle(hFile);
        return ok && magic == 0x5A4D;
    }

    std::string GetApiResponse(const std::wstring& host, const std::wstring& path) {
        HINTERNET hSession = WinHttpOpen(
            WS(skCrypt(L"Bitware/1.0")).c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            nullptr, nullptr, 0);
        if (!hSession) return {};

        WinHttpSetTimeouts(hSession, 10000, 30000, 30000, 30000);

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect,
            WS(skCrypt(L"GET")).c_str(), path.c_str(),
            nullptr, nullptr, nullptr, WINHTTP_FLAG_SECURE);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return {}; }

        std::string result;
        if (WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, nullptr)) {

            DWORD statusCode = 0;
            DWORD size = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                nullptr, &statusCode, &size, nullptr);

            if (statusCode == 200) {
                char buffer[4096];
                DWORD bytesRead = 0;
                while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                    result.append(buffer, bytesRead);
                }
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    bool DownloadUrlToFile(const std::wstring& url, const std::wstring& outputPath,
        Updater::ProgressCallback progress) {
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

        WinHttpSetTimeouts(hSession, 10000, 60000, 60000, 120000);

        HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

        DWORD flags = WINHTTP_FLAG_SECURE;
        if (urlComp.nPort == INTERNET_DEFAULT_HTTP_PORT) flags = 0;

        HINTERNET hRequest = WinHttpOpenRequest(hConnect,
            WS(skCrypt(L"GET")).c_str(), urlPath,
            nullptr, nullptr, nullptr, flags);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        bool ok = false;
        if (WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, nullptr)) {

            DWORD statusCode = 0;
            DWORD size = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                nullptr, &statusCode, &size, nullptr);

            if (statusCode == 200 || statusCode == 206) {
                int64_t totalSize = 0;
                {
                    wchar_t contentLen[32] = {};
                    DWORD lenSize = sizeof(contentLen);
                    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH,
                        nullptr, contentLen, &lenSize, nullptr)) {
                        totalSize = _wtoi64(contentLen);
                    }
                }

                HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (hFile != INVALID_HANDLE_VALUE) {
                    char buffer[65536];
                    DWORD bytesRead = 0;
                    int64_t totalRead = 0;

                    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                        DWORD written;
                        WriteFile(hFile, buffer, bytesRead, &written, nullptr);
                        totalRead += bytesRead;

                        if (progress && totalSize > 0) {
                            progress(totalRead, totalSize);
                        }
                    }
                    CloseHandle(hFile);
                    ok = (totalRead > 0);
                }
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return ok;
    }

    bool ComputeSha256(const wchar_t* path, std::string& hashHex) {
        HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        BCRYPT_ALG_HANDLE hAlg = nullptr;
        bool result = false;
        hashHex.clear();

        if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
            CloseHandle(hFile);
            return false;
        }

        DWORD hashObjSize = 0;
        DWORD cbData = 0;
        BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjSize, sizeof(hashObjSize), &cbData, 0);

        DWORD hashLen = 0;
        BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLen, sizeof(hashLen), &cbData, 0);

        std::vector<UCHAR> hashObj(hashObjSize);
        std::vector<UCHAR> hash(hashLen);

        BCRYPT_HASH_HANDLE hHash = nullptr;
        if (BCryptCreateHash(hAlg, &hHash, hashObj.data(), (ULONG)hashObjSize, nullptr, 0, 0) == 0) {
            char buffer[65536];
            DWORD read = 0;
            while (ReadFile(hFile, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
                BCryptHashData(hHash, (PUCHAR)buffer, (ULONG)read, 0);
            }

            if (BCryptFinishHash(hHash, hash.data(), (ULONG)hashLen, 0) == 0) {
                char hex[65];
                for (DWORD i = 0; i < hashLen; i++) {
                    sprintf_s(hex + i * 2, 3, "%02x", hash[i]);
                }
                hex[64] = 0;
                hashHex = hex;
                result = true;
            }
            BCryptDestroyHash(hHash);
        }

        BCryptCloseAlgorithmProvider(hAlg, 0);
        CloseHandle(hFile);
        return result;
    }

    bool VerifySha256(const wchar_t* downloadPath, const std::string& expectedHash) {
        std::string actualHash;
        if (!ComputeSha256(downloadPath, actualHash)) return false;

        Logger::Log((S(skCrypt("[Updater] SHA256: expected=")) + expectedHash +
            S(skCrypt(" actual=")) + actualHash).c_str());

        auto toLower = [](std::string s) {
            for (auto& c : s) c = (char)tolower(c);
            return s;
        };

        return toLower(actualHash) == toLower(expectedHash);
    }

    bool FetchSha256(const std::string& sha256Url, std::string& hash) {
        std::wstring wideUrl = Utf8ToWide(sha256Url);
        URL_COMPONENTSW urlComp = {};
        urlComp.dwStructSize = sizeof(urlComp);
        wchar_t hostName[256] = {}, urlPath[2048] = {};
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 2048;

        if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp))
            return false;

        HINTERNET hSession = WinHttpOpen(
            WS(skCrypt(L"Bitware/1.0")).c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            nullptr, nullptr, 0);
        if (!hSession) return false;

        WinHttpSetTimeouts(hSession, 5000, 10000, 10000, 10000);

        HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

        DWORD flags = WINHTTP_FLAG_SECURE;
        if (urlComp.nPort == INTERNET_DEFAULT_HTTP_PORT) flags = 0;

        HINTERNET hRequest = WinHttpOpenRequest(hConnect,
            WS(skCrypt(L"GET")).c_str(), urlPath,
            nullptr, nullptr, nullptr, flags);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        std::string response;
        if (WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, nullptr)) {

            DWORD statusCode = 0;
            DWORD size = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                nullptr, &statusCode, &size, nullptr);

            if (statusCode == 200) {
                char buffer[4096];
                DWORD bytesRead = 0;
                while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                    response.append(buffer, bytesRead);
                }
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (response.empty()) return false;

        auto space = response.find(' ');
        if (space != std::string::npos) {
            hash = response.substr(0, space);
        } else {
            auto newline = response.find('\n');
            if (newline != std::string::npos) {
                hash = response.substr(0, newline);
            } else {
                hash = response;
            }
        }

        return !hash.empty();
    }

    bool StageUpdate(const wchar_t* updateExe) {
        wchar_t selfPath[MAX_PATH];
        GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

        wchar_t oldPath[MAX_PATH];
        wcscpy_s(oldPath, selfPath);
        wcscat_s(oldPath, WS(skCrypt(L".old")).c_str());

        wchar_t markerPath[MAX_PATH];
        wcscpy_s(markerPath, selfPath);
        wcscat_s(markerPath, WS(skCrypt(L".updated")).c_str());

        Logger::Log(S(skCrypt("[Updater] stage: rename-swap update")).c_str());

        if (!DeleteFileW(oldPath)) {
            if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                Logger::Log(S(skCrypt("[Updater] stage: could not remove old backup")).c_str());
            }
        }

        if (!MoveFileExW(selfPath, oldPath, MOVEFILE_REPLACE_EXISTING)) {
            Logger::Log(S(skCrypt("[Updater] stage: rename self -> .old failed")).c_str());
            return false;
        }

        if (!MoveFileExW(updateExe, selfPath, MOVEFILE_REPLACE_EXISTING)) {
            Logger::Log(S(skCrypt("[Updater] stage: move update -> self failed, rolling back")).c_str());
            MoveFileExW(oldPath, selfPath, MOVEFILE_REPLACE_EXISTING);
            return false;
        }

        HANDLE hMarker = CreateFileW(markerPath, GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hMarker != INVALID_HANDLE_VALUE) {
            const char* version = BITWARE_VERSION_STRING;
            DWORD written;
            WriteFile(hMarker, version, (DWORD)strlen(version), &written, nullptr);
            CloseHandle(hMarker);
        }

        {
            HKEY hKey;
            wchar_t installedPath[MAX_PATH];
            DWORD size = sizeof(installedPath);
            if (RegOpenKeyExW(HKEY_CURRENT_USER, WS(skCrypt(L"Software\\Bitware")).c_str(),
                0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                if (RegQueryValueExW(hKey, WS(skCrypt(L"InstallPath")).c_str(),
                    nullptr, nullptr, reinterpret_cast<LPBYTE>(installedPath), &size) == ERROR_SUCCESS) {
                    if (_wcsicmp(installedPath, selfPath) != 0) {
                        Logger::Log(S(skCrypt("[Updater] stage: updating installed copy")).c_str());
                        CopyFileW(selfPath, installedPath, FALSE);
                    }
                }
                RegCloseKey(hKey);
            }
        }

        Logger::Log(S(skCrypt("[Updater] stage: swap complete, launching update")).c_str());
        Logger::Flush();

        ShellExecuteW(nullptr, WS(skCrypt(L"open")).c_str(), selfPath,
            nullptr, nullptr, SW_SHOWNORMAL);

        return true;
    }

    bool HttpGetWithRetries(const std::wstring& host, const std::wstring& path,
        std::string& result, int maxRetries = 3) {
        for (int attempt = 1; attempt <= maxRetries; attempt++) {
            result = GetApiResponse(host, path);
            if (!result.empty()) return true;

            Logger::Log((S(skCrypt("[Updater] HTTP attempt ")) +
                std::to_string(attempt) + S(skCrypt(" failed"))).c_str());

            if (attempt < maxRetries) {
                Sleep(1000 * attempt);
            }
        }
        return false;
    }

    bool DownloadWithRetries(const std::wstring& url, const std::wstring& outputPath,
        Updater::ProgressCallback progress, int maxRetries = 3) {
        for (int attempt = 1; attempt <= maxRetries; attempt++) {
            if (DownloadUrlToFile(url, outputPath, progress))
                return true;

            Logger::Log((S(skCrypt("[Updater] download attempt ")) +
                std::to_string(attempt) + S(skCrypt(" failed"))).c_str());
            DeleteFileW(outputPath.c_str());

            if (attempt < maxRetries) {
                Sleep(2000 * attempt);
            }
        }
        return false;
    }

}

namespace Updater {

    void CleanupAfterUpdate(const wchar_t* installDir) {
        wchar_t exePath[MAX_PATH];
        wcscpy_s(exePath, installDir);
        wcscat_s(exePath, WS(skCrypt(L"\\Bitware.exe")).c_str());

        wchar_t oldPath[MAX_PATH];
        wcscpy_s(oldPath, exePath);
        wcscat_s(oldPath, WS(skCrypt(L".old")).c_str());

        wchar_t markerPath[MAX_PATH];
        wcscpy_s(markerPath, exePath);
        wcscat_s(markerPath, WS(skCrypt(L".updated")).c_str());

        bool hasOld = GetFileAttributesW(oldPath) != INVALID_FILE_ATTRIBUTES;
        bool hasMarker = GetFileAttributesW(markerPath) != INVALID_FILE_ATTRIBUTES;

        if (hasOld && hasMarker) {
            Logger::Log(S(skCrypt("[Updater] cleanup: update confirmed, removing .old + .updated")).c_str());
            DeleteFileW(oldPath);
            DeleteFileW(markerPath);
        } else if (hasOld && !hasMarker) {
            Logger::Log(S(skCrypt("[Updater] cleanup: orphaned .old found, restoring")).c_str());
            MoveFileExW(oldPath, exePath, MOVEFILE_REPLACE_EXISTING);
        } else if (!hasOld && hasMarker) {
            Logger::Log(S(skCrypt("[Updater] cleanup: orphaned marker, removing")).c_str());
            DeleteFileW(markerPath);
        }

        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);

        wchar_t tempUpdate[MAX_PATH];
        swprintf(tempUpdate, MAX_PATH, WS(skCrypt(L"%sBitware_update.exe")).c_str(), tempPath);
        DeleteFileW(tempUpdate);

        wchar_t tempBat[MAX_PATH];
        swprintf(tempBat, MAX_PATH, WS(skCrypt(L"%sBitware_updater.bat")).c_str(), tempPath);
        DeleteFileW(tempBat);

        wchar_t tempExtract[MAX_PATH + 2];
        swprintf(tempExtract, MAX_PATH, WS(skCrypt(L"%sBitware_extract\\")).c_str(), tempPath);
        tempExtract[wcslen(tempExtract) + 1] = 0;
        SHFILEOPSTRUCTW fo = {};
        fo.wFunc = FO_DELETE;
        fo.pFrom = tempExtract;
        fo.fFlags = FOF_NO_UI | FOF_SILENT;
        SHFileOperationW(&fo);
    }

    UpdateInfo CheckForUpdate() {
        UpdateInfo info;
        Logger::Log(S(skCrypt("[Updater] checking for updates...")).c_str());

        std::string json;
        if (!HttpGetWithRetries(
            WS(skCrypt(L"api.github.com")),
            WS(skCrypt(L"/repos/borkeled/Bitware-Releases/releases/latest")),
            json)) {
            Logger::Log(S(skCrypt("[Updater] no response from server after retries")).c_str());
            return info;
        }

        std::string remoteVersion = FindJsonString(json, S(skCrypt("tag_name")));
        if (remoteVersion.empty()) {
            Logger::Log(S(skCrypt("[Updater] could not parse version from response")).c_str());
            return info;
        }

        Logger::Log((S(skCrypt("[Updater] remote version: ")) + remoteVersion).c_str());

        Version currentVer = ParseVersion(BITWARE_VERSION_STRING);
        Version remoteVer = ParseVersion(remoteVersion);

        if (!currentVer.valid || !remoteVer.valid) {
            Logger::Log(S(skCrypt("[Updater] version parse failed")).c_str());
            return info;
        }

        if (!IsNewerVersion(currentVer, remoteVer)) {
            Logger::Log(S(skCrypt("[Updater] already up to date")).c_str());
            Output::Success(S(skCrypt("Already up to date (v%s)")).c_str(), BITWARE_VERSION_STRING);
            return info;
        }

        Logger::Log(S(skCrypt("[Updater] update available!")).c_str());

        std::string downloadUrl = FindAssetUrl(json);
        if (downloadUrl.empty()) {
            Logger::Log(S(skCrypt("[Updater] could not find Bitware.exe asset URL")).c_str());
            return info;
        }

        info.updateAvailable = true;
        info.latestVersion = remoteVersion;
        info.downloadUrl = downloadUrl;
        return info;
    }

    bool DownloadAndApply(const UpdateInfo& info, ProgressCallback progress) {
        Logger::Log(S(skCrypt("[Updater] download starting...")).c_str());

        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);

        wchar_t updateExe[MAX_PATH];
        swprintf(updateExe, MAX_PATH, WS(skCrypt(L"%sBitware_update.exe")).c_str(), tempPath);

        DeleteFileW(updateExe);

        std::wstring wideUrl = Utf8ToWide(info.downloadUrl);
        if (wideUrl.empty()) {
            Logger::Log(S(skCrypt("[Updater] failed to convert download URL")).c_str());
            return false;
        }

        if (!DownloadWithRetries(wideUrl, updateExe, progress)) {
            Logger::Log(S(skCrypt("[Updater] download failed after retries")).c_str());
            return false;
        }

        if (!IsPE(updateExe)) {
            Logger::Log(S(skCrypt("[Updater] downloaded file is not a valid PE")).c_str());
            DeleteFileW(updateExe);
            return false;
        }

        std::string sha256Url = info.downloadUrl + S(skCrypt(".sha256"));
        std::string expectedHash;
        if (FetchSha256(sha256Url, expectedHash)) {
            Logger::Log((S(skCrypt("[Updater] verifying SHA256..."))).c_str());
            if (!VerifySha256(updateExe, expectedHash)) {
                Logger::Log(S(skCrypt("[Updater] SHA256 mismatch, rejecting update")).c_str());
                DeleteFileW(updateExe);
                return false;
            }
            Logger::Log(S(skCrypt("[Updater] SHA256 verified")).c_str());
        } else {
            Logger::Log(S(skCrypt("[Updater] no SHA256 file, skipping verification")).c_str());
        }

        Logger::Log(S(skCrypt("[Updater] download complete, staging update")).c_str());
        Logger::Flush();

        Output::Success(S(skCrypt("Update downloaded, restarting...")).c_str());

        if (!StageUpdate(updateExe)) {
            Logger::Log(S(skCrypt("[Updater] stage update failed")).c_str());
            return false;
        }

        return true;
    }

}
