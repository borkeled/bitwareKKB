#pragma once
#include <string>
#include "imgui.h"

class c_elements
{
public:

    c_vec2 padding{ 10, 10 };

    float child_width = 0.f;
    float tab_window_width = 0.f;

    struct 
    {
        c_text name{ "Bitware" };
        c_vec2 size{ 570, 354 };
        float rounding{ 12 };
    } window;

    
};

inline std::unique_ptr<c_elements> elements = std::make_unique<c_elements>();
