#pragma once
#include "../headers/includes.h"
#include "../headers/flags.h"
#include <memory>

class c_colors
{
public:
    c_col layout{ 1, 7, 12 };
    c_col white{ 255, 255, 255 };
    c_col black{ 0, 0, 0 };
    c_col accent{ 3, 249, 128 };
    c_col child{ 6, 11, 18 };
    c_col widget{ 12, 15, 22 };
    c_col text{ 140, 140, 160 };
    c_col cirlce{ 50, 50, 63 };
    c_col border{ 35, 35, 44 };
};
inline std::unique_ptr<c_colors> clr = std::make_unique<c_colors>();
