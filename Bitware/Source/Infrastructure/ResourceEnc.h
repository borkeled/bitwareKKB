#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <array>
#include <string>
#include <Auth/skStr.h>

namespace ResourceEnc {

    struct AESKey {
        std::uint8_t Key[32];
        std::uint8_t Iv[16];
    };

    inline AESKey GenerateKey(const char* seed) {
        AESKey k;
        std::uint64_t h = 0xCBF29CE484222325ULL;
        size_t len = strlen(seed);
        for (size_t i = 0; i < len; ++i) {
            h ^= seed[i];
            h *= 0x100000001B3ULL;
        }
        for (int i = 0; i < 32; ++i) {
            k.Key[i] = static_cast<std::uint8_t>((h >> ((i * 8) % 64)) ^ (h >> ((i * 3) % 64)));
        }
        for (int i = 0; i < 16; ++i) {
            k.Iv[i] = static_cast<std::uint8_t>((h >> ((i * 5) % 64)) ^ (h >> ((i * 7) % 64)));
        }
        return k;
    }

    inline void XorBlock(std::uint8_t* data, size_t len, const AESKey& key) {
        for (size_t i = 0; i < len; ++i) {
            data[i] ^= key.Key[i % 32] ^ key.Iv[i % 16];
        }
    }

    inline void EncryptBuffer(std::uint8_t* data, size_t len, const AESKey& key) {
        XorBlock(data, len, key);
        for (size_t i = 0; i < len; ++i) {
            data[i] = ((data[i] << 3) | (data[i] >> 5)) ^ key.Key[(i + 7) % 32];
        }
    }

    inline void DecryptBuffer(std::uint8_t* data, size_t len, const AESKey& key) {
        for (size_t i = 0; i < len; ++i) {
            data[i] = (data[i] ^ key.Key[(i + 7) % 32]);
            data[i] = ((data[i] >> 3) | (data[i] << 5));
        }
        XorBlock(data, len, key);
    }

    template <size_t N>
    struct EncryptedData {
        std::uint8_t Data[N];
        size_t Size;

        EncryptedData() : Size(N) {
            memset(Data, 0, N);
        }

        void Encrypt(const AESKey& key) {
            EncryptBuffer(Data, N, key);
        }

        void Decrypt(const AESKey& key) {
            DecryptBuffer(Data, N, key);
        }
    };

    template <size_t N>
    inline EncryptedData<N> Encrypt(const std::uint8_t(&data)[N], const char* seed) {
        EncryptedData<N> result;
        memcpy(result.Data, data, N);
        auto key = GenerateKey(seed);
        result.Encrypt(key);
        return result;
    }

    inline std::string ObfuscateConfigKey(const std::string& key) {
        std::string result = key;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= 0xA5;
            result[i] = ((result[i] << 2) | (result[i] >> 6));
        }
        return result;
    }

    inline std::string DeobfuscateConfigKey(const std::string& key) {
        std::string result = key;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] = ((result[i] >> 2) | (result[i] << 6));
            result[i] ^= 0xA5;
        }
        return result;
    }
}
