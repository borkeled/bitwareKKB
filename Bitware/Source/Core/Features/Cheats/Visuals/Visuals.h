#pragma once
#include <vector>
#include <Globals.hxx>

namespace Visuals {

    const std::vector<const SDK::Instance*>& Get_Bones(const SDK::Player& Player);
    void RunService();
}