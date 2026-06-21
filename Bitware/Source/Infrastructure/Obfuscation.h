#pragma once
#include <cstdint>
#include <cstdlib>
#include <windows.h>
#include <random>

namespace Obf {

    inline std::uint64_t ObfState = 0;

    inline void Seed() {
        if (!ObfState) {
            ObfState = __rdtsc() ^ reinterpret_cast<std::uint64_t>(&ObfState);
        }
    }

    inline std::uint64_t Next() {
        Seed();
        ObfState ^= ObfState << 13;
        ObfState ^= ObfState >> 7;
        ObfState ^= ObfState << 17;
        return ObfState;
    }

    inline bool OpaqueTrue() {
        volatile std::uint64_t a = Next();
        volatile std::uint64_t b = Next();
        return (a ^ b) == (a ^ b);
    }

    inline bool OpaqueFalse() {
        volatile std::uint64_t a = Next();
        volatile std::uint64_t b = Next();
        return (a * b) != (a * b);
    }

    inline int JunkInt() {
        volatile int r = static_cast<int>(Next() % 100);
        return r;
    }

    inline float JunkFloat() {
        volatile float f = static_cast<float>(Next() % 10000) / 100.0f;
        return f;
    }

    inline void JunkLoop(int iterations = 3) {
        volatile int count = iterations;
        for (volatile int i = 0; i < count; ++i) {
            volatile std::uint64_t junk = Next();
            if (junk == 0) { junk = 1; }
        }
    }

    inline void CpuSink() {
        volatile std::uint64_t sink = Next() ^ Next();
        volatile std::uint64_t sink2 = sink * sink;
        volatile std::uint64_t sink3 = sink2 ^ (sink2 >> 32);
        if (sink3 == 0xDEADBEEF) { std::abort(); }
    }
}

#define OBF_OPAQUE_TRUE if (Obf::OpaqueTrue())
#define OBF_OPAQUE_FALSE if (Obf::OpaqueFalse())
#define OBF_OPAQUE_ELSE else

#define OBF_JUNK_BLOCK { Obf::JunkLoop(2); Obf::CpuSink(); }

#define OBF_CONCAT_(a, b) a ## b
#define OBF_CONCAT(a, b) OBF_CONCAT_(a, b)

#define OBF_JUNK_DECLARE volatile int OBF_CONCAT(_junk_, __LINE__) = Obf::JunkInt(); \
    volatile float OBF_CONCAT(_jf_, __LINE__) = Obf::JunkFloat(); \
    if (OBF_CONCAT(_junk_, __LINE__) == OBF_CONCAT(_jf_, __LINE__)) { OBF_CONCAT(_junk_, __LINE__) = 0; }

#define OBF_JUNK_CHECK(x) do { \
    volatile std::uint64_t OBF_CONCAT(_ck_, __LINE__) = Obf::Next(); \
    if (OBF_CONCAT(_ck_, __LINE__) == (std::uint64_t)(x)) { (x) = 0; } \
} while(0)

#define OBFUSCATE_BEGIN volatile int OBF_CONCAT(_obf_state_, __LINE__) = 0; \
    OBF_JUNK_DECLARE

#define OBFUSCATE_STEP do { \
    OBF_CONCAT(_obf_state_, __LINE__) = (OBF_CONCAT(_obf_state_, __LINE__) + Obf::Next() % 7) % 3; \
    OBF_JUNK_BLOCK \
} while(0)

#define FLATTEN_BEGIN() static int _flat_state = 0; \
    switch(_flat_state) { case 0:

#define FLATTEN_CASE(n) _flat_state = n; } case n: {
#define FLATTEN_END() } _flat_state = 0; break; }

#define OBF_VOLATILE_READ(x) ([&]() -> decltype(x) { \
    volatile decltype(x) _ov = (x); \
    OBF_JUNK_BLOCK \
    return _ov; \
}())

#define OBF_STALL do { \
    volatile int _stall = 0; \
    for (volatile int _s = 0; _s < Obf::Next() % 4; ++_s) { \
        _stall ^= _s; \
    } \
} while(0)

#define OBF_PROLOGUE Obf::Seed(); OBF_JUNK_DECLARE
