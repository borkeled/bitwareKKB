#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <array>
#include <string>
#include <Auth/skStr.h>

namespace ResourceEnc {

    struct XTEAKey {
        std::uint32_t Key[4];
    };

    inline void XteaEncrypt(std::uint32_t rounds, std::uint32_t v[2], const std::uint32_t key[4]) {
        std::uint32_t v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
        for (std::uint32_t r = 0; r < rounds; ++r) {
            v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
            sum += delta;
            v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
        }
        v[0] = v0;
        v[1] = v1;
    }

    inline void XteaDecrypt(std::uint32_t rounds, std::uint32_t v[2], const std::uint32_t key[4]) {
        std::uint32_t v0 = v[0], v1 = v[1], delta = 0x9E3779B9, sum = delta * rounds;
        for (std::uint32_t r = 0; r < rounds; ++r) {
            v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
            sum -= delta;
            v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        }
        v[0] = v0;
        v[1] = v1;
    }

    static inline void XteaKdf(const char* seed, std::uint32_t key[4]) {
        std::uint64_t h = 0xCBF29CE484222325ULL;
        size_t len = strlen(seed);
        for (size_t i = 0; i < len; ++i) {
            h ^= seed[i];
            h *= 0x100000001B3ULL;
        }
        key[0] = static_cast<std::uint32_t>(h);
        key[1] = static_cast<std::uint32_t>(h >> 32);
        key[2] = key[0] ^ 0x9E3779B9;
        key[3] = key[1] ^ 0xC6EF3720;

        std::uint32_t zero[2] = { 0, 0 };
        XteaEncrypt(32, zero, key);
        key[0] ^= zero[0];
        key[1] ^= zero[1];

        XteaEncrypt(32, zero, key);
        key[2] ^= zero[0];
        key[3] ^= zero[1];
    }

    inline XTEAKey GenerateKey(const char* seed) {
        XTEAKey k;
        XteaKdf(seed, k.Key);
        return k;
    }

    inline void EncryptBuffer(std::uint8_t* data, size_t len, const XTEAKey& key) {
        size_t blocks = len / 8;
        for (size_t i = 0; i < blocks; ++i) {
            auto block = reinterpret_cast<std::uint32_t*>(data + i * 8);
            XteaEncrypt(32, block, key.Key);
        }
        size_t rem = len % 8;
        if (rem > 0) {
            std::uint32_t last[2] = {};
            memcpy(last, data + blocks * 8, rem);
            XteaEncrypt(32, last, key.Key);
            memcpy(data + blocks * 8, last, rem);
        }
    }

    inline void DecryptBuffer(std::uint8_t* data, size_t len, const XTEAKey& key) {
        size_t blocks = len / 8;
        for (size_t i = 0; i < blocks; ++i) {
            auto block = reinterpret_cast<std::uint32_t*>(data + i * 8);
            XteaDecrypt(32, block, key.Key);
        }
        size_t rem = len % 8;
        if (rem > 0) {
            std::uint32_t last[2] = {};
            memcpy(last, data + blocks * 8, rem);
            XteaDecrypt(32, last, key.Key);
            memcpy(data + blocks * 8, last, rem);
        }
    }

    template <size_t N>
    struct EncryptedData {
        std::uint8_t Data[N];
        size_t Size;

        EncryptedData() : Size(N) {
            memset(Data, 0, N);
        }

        void Encrypt(const XTEAKey& key) {
            EncryptBuffer(Data, N, key);
        }

        void Decrypt(const XTEAKey& key) {
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
