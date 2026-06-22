#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <stdint.h>

namespace KernelLoader
{
    bool LoadDriver();
    void UnloadDriver();
}
