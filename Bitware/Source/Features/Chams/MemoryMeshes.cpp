#define NOMINMAX
#include "MemoryMeshes.h"

#include "GpuMesh.h"

#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <Driver/Driver.h>
#include <Engine/Engine.h>
#include <Engine/Offsets/Offsets.h>
#include <Globals.hxx>
#include <Core/Features/Cheats/Visuals/Visuals.h>

namespace mesh_chams
{
namespace
{
    struct live_mesh_record final
    {
        std::string url;
        parsed_mesh parsed;
        std::uint64_t last_access_frame = 0;
    };

    struct captured_record final
    {
        std::shared_ptr<live_mesh_record> record;
        std::vector<std::string> keys;
    };

    struct resolved_mesh_draw final
    {
        std::uint64_t cache_key = 0;
        const parsed_mesh* mesh = nullptr;
        bool live_mesh = false;
    };

    static constexpr std::uint64_t k_box_key_tag = 0xB0A7000000000000ull;

    static std::uint64_t box_cache_key(const std::string& part_name)
    {
        return k_box_key_tag ^ (std::hash<std::string>{}(part_name) & 0x0000FFFFFFFFFFFFull);
    }

    static bool is_box_cache_key(std::uint64_t cache_key)
    {
        return (cache_key & 0xFFFF000000000000ull) == k_box_key_tag;
    }

    static std::unordered_map<std::string, std::shared_ptr<live_mesh_record>> g_box_meshes;
    static constexpr std::size_t k_max_box_meshes = 500;

    static constexpr std::uint64_t k_vertex_stride = 40;
    static constexpr std::uint64_t k_face_stride = 12;
    static constexpr std::uint32_t k_vertex_batch = 25;
    static constexpr std::uint32_t k_face_batch = 85;
    static constexpr DWORD k_scan_interval_ms = 4000;

    static std::unordered_map<std::string, std::shared_ptr<live_mesh_record>> g_lookup;
    static std::unordered_set<std::string> g_cached_urls;
    static constexpr std::size_t k_max_lookup_entries = 4000;
    static std::atomic<std::uint64_t> g_mesh_frame_counter{ 0 };
    static std::mutex g_cache_mutex;
    static std::atomic<bool> g_scan_running{ false };
    static HANDLE g_scan_thread = nullptr;

    static void evict_old_lookup_entries()
    {
        if (g_lookup.size() <= k_max_lookup_entries && g_box_meshes.size() <= k_max_box_meshes)
            return;

        const std::uint64_t oldest_frame = g_mesh_frame_counter.load() > 600 ? g_mesh_frame_counter.load() - 600 : 0;

        if (g_lookup.size() > k_max_lookup_entries)
        {
            std::vector<std::string> to_remove;
            to_remove.reserve(g_lookup.size() - k_max_lookup_entries + 100);
            for (auto& [key, record] : g_lookup)
            {
                if (record && record->last_access_frame < oldest_frame)
                    to_remove.push_back(key);
            }
            std::size_t target = k_max_lookup_entries * 3 / 4;
            while (g_lookup.size() > target && !to_remove.empty())
            {
                g_cached_urls.erase(g_lookup[to_remove.back()]->url);
                g_lookup.erase(to_remove.back());
                to_remove.pop_back();
            }
        }

        if (g_box_meshes.size() > k_max_box_meshes)
        {
            std::vector<std::string> to_remove;
            to_remove.reserve(g_box_meshes.size() - k_max_box_meshes + 50);
            for (auto& [name, record] : g_box_meshes)
            {
                if (record && record->last_access_frame < oldest_frame)
                    to_remove.push_back(name);
            }
            std::size_t target = k_max_box_meshes * 3 / 4;
            while (g_box_meshes.size() > target && !to_remove.empty())
            {
                g_box_meshes.erase(to_remove.back());
                to_remove.pop_back();
            }
        }
    }

    static bool is_valid_address(std::uintptr_t addr)
    {
        return addr >= 0x10000 && addr < 0x7FFFFFFFFFFF;
    }

    static bool read_raw(std::uint64_t addr, void* out, std::size_t size)
    {
        if (!is_valid_address(static_cast<std::uintptr_t>(addr)) || !out || size == 0)
            return false;

        if (!DriverReadVirtualMemory)
            return false;

        const intptr_t status = DriverReadVirtualMemory(
            Driver->Get_Handle(),
            reinterpret_cast<void*>(addr),
            out,
            static_cast<ULONG>(size),
            nullptr);

        return status >= 0;
    }

    template <typename T>
    static bool read_t(std::uint64_t addr, T& out)
    {
        return read_raw(addr, &out, sizeof(out));
    }

    static bool read_vec3(std::uint64_t addr, SDK::Vector3& out)
    {
        return read_raw(addr, &out, sizeof(out));
    }

    static std::string trim_copy(const std::string& s)
    {
        std::size_t a = 0;
        while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a])))
            ++a;

        std::size_t b = s.size();
        while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
            --b;

        return s.substr(a, b - a);
    }

    static std::string lower_copy(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        return out;
    }

    static std::uint64_t extract_asset_id(const std::string& text)
    {
        const std::string lower = lower_copy(text);
        const std::array<const char*, 4> markers = { "rbxassetid://", "rbxassetid:", "rbxasset://", "?id=" };

        for (const char* marker : markers)
        {
            const std::size_t pos = lower.find(marker);
            if (pos == std::string::npos)
                continue;

            const std::size_t start = pos + std::strlen(marker);
            std::size_t end = start;
            while (end < lower.size() && lower[end] >= '0' && lower[end] <= '9')
                ++end;

            if (end == start)
                continue;

            try
            {
                return std::stoull(lower.substr(start, end - start));
            }
            catch (...)
            {
                return 0;
            }
        }

        return 0;
    }

    static std::vector<std::string> build_keys(const std::string& raw)
    {
        std::vector<std::string> keys;
        const std::string trimmed = trim_copy(raw);
        if (trimmed.empty())
            return keys;

        keys.push_back(lower_copy(trimmed));

        const std::uint64_t asset_id = extract_asset_id(trimmed);
        if (asset_id != 0)
            keys.push_back("assetid:" + std::to_string(asset_id));

        return keys;
    }

    static const char* builtin_asset_alias(std::uint64_t asset_id)
    {
        switch (asset_id)
        {
        case 1365230: return "rbxasset://avatar/heads/head.mesh";
        case 1365219: return "rbxasset://avatar/meshes/torso.mesh";
        case 1365224: return "rbxasset://avatar/meshes/leftarm.mesh";
        case 1365223: return "rbxasset://avatar/meshes/rightarm.mesh";
        case 1365226: return "rbxasset://avatar/meshes/leftleg.mesh";
        case 1365225: return "rbxasset://avatar/meshes/rightleg.mesh";
        default: return nullptr;
        }
    }

    static std::uint64_t get_default_asset_for_part(const std::string& part_name, bool is_r15)
    {
        if (is_r15)
        {
            if (part_name == "Head") return 1365230;
            if (part_name == "UpperTorso" || part_name == "LowerTorso") return 1365219;
            if (part_name == "LeftUpperArm" || part_name == "LeftLowerArm" || part_name == "LeftHand") return 1365224;
            if (part_name == "RightUpperArm" || part_name == "RightLowerArm" || part_name == "RightHand") return 1365223;
            if (part_name == "LeftUpperLeg" || part_name == "LeftLowerLeg" || part_name == "LeftFoot") return 1365226;
            if (part_name == "RightUpperLeg" || part_name == "RightLowerLeg" || part_name == "RightFoot") return 1365225;
        }
        else
        {
            if (part_name == "Head") return 1365230;
            if (part_name == "Torso") return 1365219;
            if (part_name == "Left Arm") return 1365224;
            if (part_name == "Right Arm") return 1365223;
            if (part_name == "Left Leg") return 1365226;
            if (part_name == "Right Leg") return 1365225;
        }

        return 0;
    }

    static std::string read_std_string(std::uint64_t base)
    {
        if (!is_valid_address(static_cast<std::uintptr_t>(base)))
            return "";

        std::uint64_t size = 0;
        if (!read_t(base + 0x18, size))
            return "";

        std::uint64_t src = base;
        if (size >= 16 && !read_t(base, src))
            return "";

        if (!is_valid_address(static_cast<std::uintptr_t>(src)))
            return "";

        const std::size_t len = static_cast<std::size_t>((std::min<std::uint64_t>)(size, 511));
        if (len == 0)
            return "";

        char buf[512] = {};
        if (!read_raw(src, buf, len))
            return "";

        std::string out;
        out.reserve(len);
        for (std::size_t i = 0; i < len; ++i)
        {
            const unsigned char c = static_cast<unsigned char>(buf[i]);
            if (c == 0)
                break;
            if (c >= 32 && c < 127)
                out.push_back(static_cast<char>(c));
        }

        return out;
    }

    static std::string get_mesh_id_string_from_part(std::uint64_t part_address)
    {
        if (!is_valid_address(static_cast<std::uintptr_t>(part_address)))
            return "";

        const std::string class_name = SDK::Instance(part_address).Class();
        std::uint64_t mesh_id_addr = 0;

        if (class_name == "MeshPart")
            mesh_id_addr = part_address + Offsets::MeshPart::MeshId;
        else if (class_name == "SpecialMesh")
            mesh_id_addr = part_address + Offsets::SpecialMesh::MeshId;
        else if (class_name == "CharacterMesh")
            mesh_id_addr = part_address + Offsets::CharacterMesh::MeshId;
        else
            return "";

        return read_std_string(mesh_id_addr);
    }

    static void compute_bounds(parsed_mesh& mesh)
    {
        if (mesh.vertices.empty())
        {
            mesh.bounds.valid = false;
            return;
        }

        mesh.bounds.min = mesh.vertices[0].position;
        mesh.bounds.max = mesh.bounds.min;

        for (const auto& v : mesh.vertices)
        {
            mesh.bounds.min.x = (std::min)(mesh.bounds.min.x, v.position.x);
            mesh.bounds.min.y = (std::min)(mesh.bounds.min.y, v.position.y);
            mesh.bounds.min.z = (std::min)(mesh.bounds.min.z, v.position.z);
            mesh.bounds.max.x = (std::max)(mesh.bounds.max.x, v.position.x);
            mesh.bounds.max.y = (std::max)(mesh.bounds.max.y, v.position.y);
            mesh.bounds.max.z = (std::max)(mesh.bounds.max.z, v.position.z);
        }

        mesh.bounds.center = {
            (mesh.bounds.min.x + mesh.bounds.max.x) * 0.5f,
            (mesh.bounds.min.y + mesh.bounds.max.y) * 0.5f,
            (mesh.bounds.min.z + mesh.bounds.max.z) * 0.5f
        };

        mesh.bounds.size = {
            (std::max)(0.001f, mesh.bounds.max.x - mesh.bounds.min.x),
            (std::max)(0.001f, mesh.bounds.max.y - mesh.bounds.min.y),
            (std::max)(0.001f, mesh.bounds.max.z - mesh.bounds.min.z)
        };

        mesh.bounds.valid = true;
    }

    static void init_unit_cube(parsed_mesh& mesh)
    {
        mesh.vertices = {
            { { -0.5f, -0.5f, -0.5f } }, { { 0.5f, -0.5f, -0.5f } }, { { 0.5f, 0.5f, -0.5f } }, { { -0.5f, 0.5f, -0.5f } },
            { { -0.5f, -0.5f,  0.5f } }, { { 0.5f, -0.5f,  0.5f } }, { { 0.5f, 0.5f,  0.5f } }, { { -0.5f, 0.5f,  0.5f } },
        };

        mesh.indices = {
            0, 1, 2, 0, 2, 3,
            4, 6, 5, 4, 7, 6,
            0, 4, 5, 0, 5, 1,
            2, 6, 7, 2, 7, 3,
            0, 3, 7, 0, 7, 4,
            1, 5, 6, 1, 6, 2,
        };

        compute_bounds(mesh);
    }

    static bool mesh_fits_part(const parsed_mesh& mesh, const SDK::Vector3& part_size)
    {
        if (!mesh.bounds.valid || mesh.vertices.empty() || mesh.indices.size() < 12)
            return false;

        const float part_max = (std::max)(part_size.x, (std::max)(part_size.y, part_size.z));
        if (part_max < 0.05f)
            return false;

        const float mesh_max = (std::max)(mesh.bounds.size.x, (std::max)(mesh.bounds.size.y, mesh.bounds.size.z));
        if (mesh_max < 0.05f)
            return false;

        const float ratio = mesh_max / part_max;
        if (ratio < 0.45f || ratio > 2.0f)
            return false;

        const std::size_t tri_count = mesh.indices.size() / 3;
        if (tri_count > 12000)
            return false;

        return true;
    }

    static std::shared_ptr<live_mesh_record> get_part_box_mesh(const std::string& part_name)
    {
        auto it = g_box_meshes.find(part_name);
        if (it != g_box_meshes.end())
        {
            it->second->last_access_frame = g_mesh_frame_counter.load();
            return it->second;
        }

        evict_old_lookup_entries();

        auto record = std::make_shared<live_mesh_record>();
        record->url = "box:" + part_name;
        record->last_access_frame = g_mesh_frame_counter.load();
        init_unit_cube(record->parsed);
        g_box_meshes.emplace(part_name, record);
        return record;
    }

    static resolved_mesh_draw make_box_draw(const std::string& part_name)
    {
        const std::shared_ptr<live_mesh_record> record = get_part_box_mesh(part_name);
        resolved_mesh_draw resolved{};
        resolved.cache_key = box_cache_key(part_name);
        resolved.mesh = &record->parsed;
        resolved.live_mesh = false;
        return resolved;
    }

    static bool read_vertices(std::uint64_t first, std::uint32_t count, std::vector<mesh_vertex>& out)
    {
        out.clear();
        out.reserve(count);

        std::uint64_t addr = first;
        std::uint32_t left = count;

        while (left > 0)
        {
            const std::uint32_t batch = (std::min)(k_vertex_batch, left);
            std::vector<unsigned char> buf(static_cast<std::size_t>(batch * k_vertex_stride));

            if (!read_raw(addr, buf.data(), buf.size()))
                return false;

            for (std::uint32_t j = 0; j < batch; ++j)
            {
                const std::size_t b = static_cast<std::size_t>(j * k_vertex_stride);
                mesh_vertex v{};
                std::memcpy(&v.position.x, buf.data() + b, sizeof(float));
                std::memcpy(&v.position.y, buf.data() + b + 4, sizeof(float));
                std::memcpy(&v.position.z, buf.data() + b + 8, sizeof(float));
                out.push_back(v);
            }

            left -= batch;
            addr += batch * k_vertex_stride;
        }

        return true;
    }

    static bool read_faces(std::uint64_t first, std::uint32_t count, std::vector<std::uint32_t>& out)
    {
        out.clear();
        out.reserve(static_cast<std::size_t>(count) * 3);

        std::uint64_t addr = first;
        std::uint32_t left = count;

        while (left > 0)
        {
            const std::uint32_t batch = (std::min)(k_face_batch, left);
            std::vector<unsigned char> buf(static_cast<std::size_t>(batch * k_face_stride));

            if (!read_raw(addr, buf.data(), buf.size()))
                return false;

            for (std::uint32_t j = 0; j < batch; ++j)
            {
                const std::size_t b = static_cast<std::size_t>(j * k_face_stride);
                std::uint32_t a = 0, c = 0, d = 0;
                std::memcpy(&a, buf.data() + b, sizeof(std::uint32_t));
                std::memcpy(&c, buf.data() + b + 4, sizeof(std::uint32_t));
                std::memcpy(&d, buf.data() + b + 8, sizeof(std::uint32_t));
                out.push_back(a);
                out.push_back(c);
                out.push_back(d);
            }

            left -= batch;
            addr += batch * k_face_stride;
        }

        return true;
    }

    static std::uint32_t read_faces_max_index(std::uint64_t first, std::uint32_t count)
    {
        std::uint64_t addr = first;
        std::uint32_t left = count;
        std::uint32_t max_idx = 0;

        while (left > 0)
        {
            const std::uint32_t batch = (std::min)(k_face_batch, left);
            std::vector<unsigned char> buf(static_cast<std::size_t>(batch * k_face_stride));

            if (!read_raw(addr, buf.data(), buf.size()))
                return 0;

            for (std::uint32_t j = 0; j < batch; ++j)
            {
                const std::size_t b = static_cast<std::size_t>(j * k_face_stride);
                for (int k = 0; k < 3; ++k)
                {
                    std::uint32_t idx = 0;
                    std::memcpy(&idx, buf.data() + b + (k * 4), sizeof(std::uint32_t));
                    if (idx > max_idx)
                        max_idx = idx;
                }
            }

            left -= batch;
            addr += batch * k_face_stride;
        }

        return max_idx;
    }

    static std::uint32_t get_lod0_face_count(std::uint64_t fmd, std::uint32_t total)
    {
        for (std::uint64_t off = 0xA8; off <= 0xD0; off += 0x08)
        {
            std::uint64_t lf = 0, ll = 0;
            if (!read_t(fmd + off, lf) || !read_t(fmd + off + 0x08, ll) || ll <= lf)
                continue;

            const std::uint64_t lc = (ll - lf) / 4;
            if (lc < 2 || lc > 20)
                continue;

            std::uint32_t faces = 0;
            if (read_t(lf + 4, faces) && faces > 0 && faces <= total)
                return faces;
        }

        return total;
    }

    static std::vector<std::uint64_t> get_nodes(std::uint64_t cache_ptr)
    {
        std::vector<std::uint64_t> nodes;

        std::uint64_t sentinel = 0, node = 0;
        if (!read_t(cache_ptr + 0x08, sentinel) || !is_valid_address(static_cast<std::uintptr_t>(sentinel)))
            return nodes;

        if (!read_t(sentinel, node) || !is_valid_address(static_cast<std::uintptr_t>(node)) || node == sentinel)
        {
            if (!read_t(sentinel + 0x08, node) || !is_valid_address(static_cast<std::uintptr_t>(node)))
                return nodes;
        }

        int walked = 0;
        while (is_valid_address(static_cast<std::uintptr_t>(node)) && node != sentinel && walked < 10000)
        {
            nodes.push_back(node);
            ++walked;

            std::uint64_t next = 0;
            if (!read_t(node, next) || !is_valid_address(static_cast<std::uintptr_t>(next)) || next == node)
                break;

            node = next;
        }

        return nodes;
    }

    static bool looks_like_cache_group(std::uint64_t base_ptr, std::uint64_t& evict, std::uint64_t& pinned)
    {
        std::uint64_t first = 0;
        if (!read_t(base_ptr, first))
            return false;

        if ((first & 0xffffffffull) != 0x02000000ull || (first >> 32) != 0)
            return false;

        if (!read_t(base_ptr + 0x20, evict) || !read_t(base_ptr + 0x28, pinned))
            return false;

        return is_valid_address(static_cast<std::uintptr_t>(evict)) &&
            is_valid_address(static_cast<std::uintptr_t>(pinned));
    }

    static bool find_cache(std::uint64_t& evict, std::uint64_t& pinned)
    {
        evict = 0;
        pinned = 0;

        if (!Globals::Datamodel.Address)
            return false;

        SDK::Instance svc = Globals::Datamodel.Find_First_Child_Of_Class("MeshContentProvider");
        if (!svc.Address)
            svc = Globals::Datamodel.Find_First_Child("MeshContentProvider");
        if (!svc.Address)
            return false;

        const std::array<std::uint64_t, 3> scan_bases = {
            Driver->Read<std::uint64_t>(svc.Address + 0x08),
            Driver->Read<std::uint64_t>(svc.Address),
            svc.Address
        };

        for (std::uint64_t scan_base : scan_bases)
        {
            if (!is_valid_address(static_cast<std::uintptr_t>(scan_base)))
                continue;

            for (std::uint64_t off = 0; off <= 0x3000; off += 0x08)
            {
                std::uint64_t candidate = 0;
                if (!read_t(scan_base + off, candidate) || !is_valid_address(static_cast<std::uintptr_t>(candidate)))
                    continue;

                std::uint64_t ce = 0, cp = 0;
                if (!looks_like_cache_group(candidate, ce, cp))
                    continue;

                if (get_nodes(ce).empty() && get_nodes(cp).empty())
                    continue;

                evict = ce;
                pinned = cp;
                return true;
            }
        }

        return false;
    }

    static bool capture_node(std::uint64_t node, std::unordered_set<std::string>& known_urls, std::vector<captured_record>& out_records)
    {
        const std::string url = read_std_string(node + 0x10);
        if (url.empty() || known_urls.find(url) != known_urls.end())
            return false;

        std::uint64_t ci = 0, fmd = 0, v_first = 0, v_last = 0, f_first = 0, f_last = 0;
        if (!read_t(node + 0x38, ci) || !read_t(ci + 0x28, fmd) || !read_t(fmd + 0x00, v_first) ||
            !read_t(fmd + 0x08, v_last) || !read_t(fmd + 0x30, f_first) || !read_t(fmd + 0x38, f_last))
        {
            return false;
        }

        if (!is_valid_address(static_cast<std::uintptr_t>(v_first)) ||
            !is_valid_address(static_cast<std::uintptr_t>(f_first)) ||
            v_last <= v_first || f_last <= f_first)
        {
            return false;
        }

        const std::uint64_t v_diff = v_last - v_first;
        const std::uint64_t f_diff = f_last - f_first;
        if ((v_diff % k_vertex_stride) != 0 || (f_diff % k_face_stride) != 0)
            return false;

        const std::uint32_t total_verts = static_cast<std::uint32_t>(v_diff / k_vertex_stride);
        const std::uint32_t total_faces = static_cast<std::uint32_t>(f_diff / k_face_stride);
        if (total_verts == 0 || total_faces == 0 || total_verts > 500000 || total_faces > 500000)
            return false;

        const std::uint32_t lod0_faces = get_lod0_face_count(fmd, total_faces);
        const std::uint32_t vert_count = (std::min)(read_faces_max_index(f_first, lod0_faces) + 1, total_verts);
        if (vert_count == 0)
            return false;

        auto record = std::make_shared<live_mesh_record>();
        record->url = url;

        if (!read_vertices(v_first, vert_count, record->parsed.vertices))
            return false;

        if (!read_faces(f_first, lod0_faces, record->parsed.indices))
            return false;

        compute_bounds(record->parsed);
        if (!record->parsed.bounds.valid)
            return false;

        std::vector<std::string> keys = build_keys(url);
        if (keys.empty())
            return false;

        known_urls.insert(url);
        out_records.push_back({ record, std::move(keys) });
        return true;
    }

    static void perform_scan_once()
    {
        if (!Globals::Visuals::Chams)
            return;

        std::uint64_t evict = 0, pinned = 0;
        if (!find_cache(evict, pinned))
            return;

        std::unordered_set<std::string> known_urls;
        {
            std::lock_guard<std::mutex> lock(g_cache_mutex);
            known_urls = g_cached_urls;
        }

        std::vector<captured_record> captured;
        const std::vector<std::uint64_t> evict_nodes = get_nodes(evict);
        const std::vector<std::uint64_t> pinned_nodes = get_nodes(pinned);
        captured.reserve(evict_nodes.size() + pinned_nodes.size());

        for (std::uint64_t node : evict_nodes)
            capture_node(node, known_urls, captured);

        for (std::uint64_t node : pinned_nodes)
            capture_node(node, known_urls, captured);

        if (captured.empty())
            return;

        std::lock_guard<std::mutex> lock(g_cache_mutex);
        evict_old_lookup_entries();
        for (auto& entry : captured)
        {
            g_cached_urls.insert(entry.record->url);
            entry.record->last_access_frame = g_mesh_frame_counter.load();
            for (const std::string& key : entry.keys)
                g_lookup[key] = entry.record;
        }
        g_mesh_frame_counter.fetch_add(1);
    }

    static DWORD WINAPI scan_thread_main(LPVOID)
    {
        while (g_scan_running.load())
        {
            perform_scan_once();

            for (int i = 0; i < static_cast<int>(k_scan_interval_ms / 50) && g_scan_running.load(); ++i)
                Sleep(50);
        }

        return 0;
    }

    static resolved_mesh_draw resolve_live_mesh_gpu(const SDK::Player& player, const std::string& part_name, std::uintptr_t part_address)
    {
        if (!part_address)
            return {};

        std::uint64_t prim_addr = Driver->Read<std::uint64_t>(part_address + Offsets::BasePart::Primitive);
        if (!is_valid_address(static_cast<std::uintptr_t>(prim_addr)))
            return {};

        SDK::Vector3 part_size;
        if (!read_vec3(prim_addr + Offsets::Primitive::Size, part_size))
            return {};

        std::vector<std::string> keys;
        const std::string mesh_id = get_mesh_id_string_from_part(part_address);
        if (!mesh_id.empty())
        {
            const auto mesh_keys = build_keys(mesh_id);
            keys.insert(keys.end(), mesh_keys.begin(), mesh_keys.end());
        }

        std::uint64_t asset_id = extract_asset_id(mesh_id);
        if (asset_id == 0)
            asset_id = get_default_asset_for_part(part_name, player.Rig_Type == 1);

        if (asset_id != 0)
            keys.push_back("assetid:" + std::to_string(asset_id));

        if (asset_id != 0)
        {
            if (const char* alias = builtin_asset_alias(asset_id))
                keys.push_back(alias);
        }

        std::lock_guard<std::mutex> lock(g_cache_mutex);

        for (const std::string& key : keys)
        {
            auto it = g_lookup.find(key);
            if (it == g_lookup.end())
                continue;

            it->second->last_access_frame = g_mesh_frame_counter.load();
            resolved_mesh_draw resolved{};
            resolved.cache_key = reinterpret_cast<std::uint64_t>(it->second.get());
            resolved.mesh = &it->second->parsed;
            resolved.live_mesh = mesh_fits_part(it->second->parsed, part_size);
            if (resolved.live_mesh)
                return resolved;
        }

        return make_box_draw(part_name);
    }

    static bool part_on_screen(const mesh_gpu::draw_item& item, const float view[16])
    {
        const float hx = item.size.x * 0.5f;
        const float hy = item.size.y * 0.5f;
        const float hz = item.size.z * 0.5f;

        static const float corners[8][3] = {
            {-1.f, -1.f, -1.f}, {1.f, -1.f, -1.f}, {-1.f, 1.f, -1.f}, {1.f, 1.f, -1.f},
            {-1.f, -1.f,  1.f}, {1.f, -1.f,  1.f}, {-1.f, 1.f,  1.f}, {1.f, 1.f,  1.f},
        };

        for (const auto& corner : corners)
        {
            const float lx = corner[0] * hx;
            const float ly = corner[1] * hy;
            const float lz = corner[2] * hz;

            const SDK::Vector3 world{
                item.pos.x + item.rot[0] * lx + item.rot[1] * ly + item.rot[2] * lz,
                item.pos.y + item.rot[3] * lx + item.rot[4] * ly + item.rot[5] * lz,
                item.pos.z + item.rot[6] * lx + item.rot[7] * ly + item.rot[8] * lz
            };

            const float clip_w = world.x * view[12] + world.y * view[13] + world.z * view[14] + view[15];
            if (clip_w <= 0.01f)
                continue;

            const float clip_x = world.x * view[0] + world.y * view[1] + world.z * view[2] + view[3];
            const float clip_y = world.x * view[4] + world.y * view[5] + world.z * view[6] + view[7];
            const float ndc_x = clip_x / clip_w;
            const float ndc_y = clip_y / clip_w;

            if (ndc_x >= -1.2f && ndc_x <= 1.2f && ndc_y >= -1.2f && ndc_y <= 1.2f)
                return true;
        }

        return false;
    }

    static std::vector<mesh_gpu::draw_item> build_draw_list_impl(
        const std::vector<SDK::Player>& players,
        const SDK::Player& local_player,
        const float view[16])
    {
        static thread_local std::vector<mesh_gpu::draw_item> draws;
        draws.clear();
        draws.reserve(players.size() * 8);

        for (const SDK::Player& player : players)
        {
            const std::vector<const SDK::Instance*>& bones = Visuals::Get_Bones(player);

            for (const SDK::Instance* inst : bones)
            {
                if (!inst || !inst->Address)
                    continue;

                const std::string part_name = inst->Name();
                if (part_name.empty() || part_name == "HumanoidRootPart")
                    continue;

                std::uint64_t prim_addr = Driver->Read<std::uint64_t>(inst->Address + Offsets::BasePart::Primitive);
                if (!is_valid_address(static_cast<std::uintptr_t>(prim_addr)))
                    continue;

                SDK::Vector3 size;
                if (!read_vec3(prim_addr + Offsets::Primitive::Size, size))
                    continue;

                if (size.x <= 0.f || size.y <= 0.f || size.z <= 0.f)
                    continue;

                SDK::Vector3 pos;
                if (!read_vec3(prim_addr + Offsets::Primitive::Position, pos))
                    continue;

                SDK::Matrix3 rot_mat;
                if (!read_raw(prim_addr + Offsets::Primitive::Rotation, &rot_mat, sizeof(rot_mat)))
                    continue;

                resolved_mesh_draw resolved = resolve_live_mesh_gpu(player, part_name, inst->Address);
                if (!resolved.cache_key || !resolved.mesh)
                {
                    resolved = make_box_draw(part_name);
                }
                if (!resolved.cache_key || !resolved.mesh)
                    continue;

                mesh_gpu::draw_item item{};
                item.cache_key = resolved.cache_key;
                item.mesh = resolved.mesh;
                item.live_mesh = resolved.live_mesh;
                item.part_seed = static_cast<float>(std::hash<std::string>{}(part_name) % 997u) / 997.f;
                item.pos = pos;
                item.size = size;

                for (int i = 0; i < 9; ++i)
                    item.rot[i] = rot_mat.data[i];

                if (Globals::Visuals::Chams_Occlusion && Globals::Camera.Address)
                {
                    SDK::Vector3 cam_pos = Globals::Camera.Get_CameraPos();
                    SDK::Vector3 dir = pos - cam_pos;
                    item.occluded = false;
                }

                if (!part_on_screen(item, view))
                    continue;

                draws.push_back(item);
            }
        }

        return draws;
    }
}

std::vector<mesh_gpu::draw_item> build_draw_list(
    const std::vector<SDK::Player>& players,
    const SDK::Player& local_player,
    const float view[16])
{
    return build_draw_list_impl(players, local_player, view);
}

void clear_caches()
{
    std::lock_guard<std::mutex> lock(g_cache_mutex);
    g_lookup.clear();
    g_cached_urls.clear();
    g_box_meshes.clear();
    g_mesh_frame_counter.store(0);
    mesh_gpu::clear_gpu_meshes();
}

void start_memory_mesh_scan()
{
    bool expected = false;
    if (!g_scan_running.compare_exchange_strong(expected, true))
        return;

    g_scan_thread = CreateThread(nullptr, 0, &scan_thread_main, nullptr, 0, nullptr);
    if (!g_scan_thread)
        g_scan_running.store(false);
}

void shutdown_memory_mesh_scan()
{
    g_scan_running.store(false);

    if (g_scan_thread)
    {
        WaitForSingleObject(g_scan_thread, INFINITE);
        CloseHandle(g_scan_thread);
        g_scan_thread = nullptr;
    }

    std::lock_guard<std::mutex> lock(g_cache_mutex);
    g_lookup.clear();
    g_cached_urls.clear();
    g_box_meshes.clear();

    mesh_gpu::shutdown();
}

} // namespace mesh_chams
