#pragma once
#include <array>
#include <cstdint>

struct Color4 {
    float r, g, b, a;
    Color4() : r(0), g(0), b(0), a(1.0f) {}
    Color4(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

    float& operator[](int i) { return (&r)[i]; }
    const float& operator[](int i) const { return (&r)[i]; }
};

enum class BindMode : int {
    Hold = 0,
    Toggle = 1
};

struct KeyBind {
    int key = 0;
    BindMode mode = BindMode::Hold;

    KeyBind() = default;
    KeyBind(int k, BindMode m) : key(k), mode(m) {}
};
