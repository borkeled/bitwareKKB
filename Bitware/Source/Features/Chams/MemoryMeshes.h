#pragma once

#include <cstdint>
#include <vector>

#include <d3d11.h>

#include <Engine/Math/Math.h>

namespace SDK { class Player; }

namespace mesh_gpu { struct render_params; struct draw_item; }

namespace mesh_chams
{
    struct mesh_vertex final
    {
        SDK::Vector3 position{};
    };

    struct mesh_bounds final
    {
        SDK::Vector3 min{};
        SDK::Vector3 max{};
        SDK::Vector3 center{};
        SDK::Vector3 size{};
        bool valid = false;
    };

    struct parsed_mesh final
    {
        std::vector<mesh_vertex> vertices;
        std::vector<std::uint32_t> indices;
        mesh_bounds bounds{};
    };

    void start_memory_mesh_scan();
    void shutdown_memory_mesh_scan();
    void clear_caches();

    std::vector<mesh_gpu::draw_item> build_draw_list(
        const std::vector<SDK::Player>& players,
        const SDK::Player& local_player,
        const float view[16]);
}
