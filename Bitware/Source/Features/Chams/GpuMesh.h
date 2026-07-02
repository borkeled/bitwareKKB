#pragma once

#include <cstdint>
#include <vector>

#include <d3d11.h>

#include <Engine/Math/Math.h>

namespace mesh_chams { struct parsed_mesh; }

namespace mesh_gpu
{
    struct render_params final
    {
        float fill_color[4]{ 1.f, 1.f, 1.f, 1.f };
        bool  fill_enabled  = true;
        float fill_alpha    = 0.82f;

        float outline_color[4]{ 0.f, 0.f, 0.f, 1.f };
        bool  outline       = false;
        float outline_scale = 1.06f;

        int   shader_mode   = 0;
        float shader_speed  = 1.5f;

        float specular_power     = 32.f;
        float specular_intensity = 0.6f;

        float shader_param_a = 0.5f;
        float shader_param_b = 1.0f;

        int   max_triangles = 3500;

        bool  occlusion_enabled = false;
        float visible_color[4]{ 0.2f, 1.f, 0.35f, 1.f };
        float occluded_color[4]{ 1.f, 0.2f, 0.2f, 1.f };
    };

    struct draw_item final
    {
        std::uint64_t cache_key = 0;
        const mesh_chams::parsed_mesh* mesh = nullptr;
        float rot[9]{};
        SDK::Vector3 pos{};
        SDK::Vector3 size{};
        float part_seed = 0.f;
        bool live_mesh = false;
        bool occluded = false;
    };

    std::size_t render_draw_list(
        const std::vector<draw_item>& items,
        const float view[16],
        float viewport_width,
        float viewport_height,
        float viewport_x,
        float viewport_y,
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        const render_params& params);

    void shutdown();
    void clear_gpu_meshes();
}
