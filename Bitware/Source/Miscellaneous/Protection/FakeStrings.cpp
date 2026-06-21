#include "FakeStrings.h"
#include <cstdlib>
#include <cstring>
#include <ctime>

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
            const char* FakeDecoyStrings[] = {
                "AES_KEY_256_INTERNAL",
                "AUTH_TOKEN_PROD",
                "DEBUG_PASSWORD",
                "LICENSE_CHECK_FAIL",
                "REMOTE_CMD_EXEC",
                "SQL_CONNECTION_STRING",
                "PRIVATE_API_ENDPOINT",
                "ADMIN_OVERRIDE_ENABLED",
                "KeyAuth_1.4",
                "Cryptominer_Active",
                "SensitiveDataLeak",
                "Backdoor_Enabled",
                "TrojanHorsePayload",
                "MalwareSignatureDetected",
                "UnauthorizedAccessAttempt",
                "SystemIntegrityCompromised",
                "DataExfiltrationInProgress",
                "RansomwareEncryptionActive",
                "ZeroDayExploit",
                "RootkitInstalled",
                "BotnetCommandAndControl",
                "PhishingAttackDetected",
                "DDoSAttackInProgress",
                "KeyloggerActive",
                "CredentialStuffingAttack",
                "PrivilegeEscalationAttempt",
                "SuspiciousNetworkActivity",
                "AnomalyDetectionTriggered",
                "FirewallBypassAttempt",
                "MaliciousScriptExecution",
                "ExploitKitDetected",
                "SuspiciousFileDownload",
                "UnauthorizedRemoteAccess",
                "DataTamperingDetected",
                "WindowsLicenseKey_Backup",
                "BitLockerRecoveryKey",
                "DomainAdminCredentials",
                "SAM_Hive_Dump",
                "LSASS_Credential_Dump",
                "MSCONFIG_BACKDOOR",
                "SCHTASKS_PERSISTENCE",
            };
            const char* b = FakeDecoyStrings[Rand(0, (int)(sizeof(FakeDecoyStrings) / sizeof(FakeDecoyStrings[0])) - 1)];
            strncpy_s(pool[i], len + 1, b, _TRUNCATE);
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
