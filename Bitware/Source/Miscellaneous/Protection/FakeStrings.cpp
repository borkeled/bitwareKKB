#include "FakeStrings.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <Auth/skStr.h>

char** FakeStringss::pool = nullptr;
size_t FakeStringss::poolCount = 0;

static int Rand(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

static void FillRandomASCII(char* buf, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        int r = Rand(0, 2);
        if (r == 0) buf[i] = 'A' + Rand(0, 25);
        else if (r == 1) buf[i] = 'a' + Rand(0, 25);
        else buf[i] = '0' + Rand(0, 9);
    }
    buf[len] = '\0';
}

void FakeStringss::Generate(size_t count)
{
    if (pool) return; 

    srand((unsigned)time(nullptr));

    pool = new char* [count];
    poolCount = count;

    for (size_t i = 0; i < count; ++i)
    {
        size_t len = Rand(8, 64);

        pool[i] = new char[len + 1];
        FillRandomASCII(pool[i], len);

        if (Rand(0, 10) == 0)
        {
            const std::string FakeDecoyStrings[] = {
                std::string(skCrypt("AES_KEY_256_INTERNAL")),
                std::string(skCrypt("AUTH_TOKEN_PROD")),
                std::string(skCrypt("DEBUG_PASSWORD")),
                std::string(skCrypt("LICENSE_CHECK_FAIL")),
                std::string(skCrypt("REMOTE_CMD_EXEC")),
                std::string(skCrypt("SQL_CONNECTION_STRING")),
                std::string(skCrypt("PRIVATE_API_ENDPOINT")),
                std::string(skCrypt("ADMIN_OVERRIDE_ENABLED")),
                std::string(skCrypt("KeyAuth_1.4")),
                std::string(skCrypt("Cryptominer_Active")),
                std::string(skCrypt("SensitiveDataLeak")),
                std::string(skCrypt("Backdoor_Enabled")),
                std::string(skCrypt("TrojanHorsePayload")),
                std::string(skCrypt("MalwareSignatureDetected")),
                std::string(skCrypt("UnauthorizedAccessAttempt")),
                std::string(skCrypt("SystemIntegrityCompromised")),
                std::string(skCrypt("DataExfiltrationInProgress")),
                std::string(skCrypt("RansomwareEncryptionActive")),
                std::string(skCrypt("ZeroDayExploit")),
                std::string(skCrypt("RootkitInstalled")),
                std::string(skCrypt("BotnetCommandAndControl")),
                std::string(skCrypt("PhishingAttackDetected")),
                std::string(skCrypt("DDoSAttackInProgress")),
                std::string(skCrypt("KeyloggerActive")),
                std::string(skCrypt("CredentialStuffingAttack")),
                std::string(skCrypt("PrivilegeEscalationAttempt")),
                std::string(skCrypt("SuspiciousNetworkActivity")),
                std::string(skCrypt("AnomalyDetectionTriggered")),
                std::string(skCrypt("FirewallBypassAttempt")),
                std::string(skCrypt("MaliciousScriptExecution")),
                std::string(skCrypt("ExploitKitDetected")),
                std::string(skCrypt("SuspiciousFileDownload")),
                std::string(skCrypt("UnauthorizedRemoteAccess")),
                std::string(skCrypt("DataTamperingDetected")),
                std::string(skCrypt("WindowsLicenseKey_Backup")),
                std::string(skCrypt("BitLockerRecoveryKey")),
                std::string(skCrypt("DomainAdminCredentials")),
                std::string(skCrypt("SAM_Hive_Dump")),
                std::string(skCrypt("LSASS_Credential_Dump")),
                std::string(skCrypt("MSCONFIG_BACKDOOR")),
                std::string(skCrypt("SCHTASKS_PERSISTENCE")),
            };
            const std::string& b = FakeDecoyStrings[Rand(0, (int)(sizeof(FakeDecoyStrings) / sizeof(FakeDecoyStrings[0])) - 1)];
            strncpy_s(pool[i], len + 1, b.c_str(), _TRUNCATE);
            pool[i][len] = '\0';
        }
    }
}


void FakeStringss::Clear()
{
    if (!pool) return;

    for (size_t i = 0; i < poolCount; ++i)
        delete[] pool[i];

    delete[] pool;
    pool = nullptr;
    poolCount = 0;
}
