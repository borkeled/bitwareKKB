#pragma once
#include <vector>
#include <Globals.hxx>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Visuals {

    const std::vector<const SDK::Instance*>& Get_Bones(const SDK::Player& Player);
    void RunService();
    void RunMeshChams();
    void FlushMeshChams(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv);
}