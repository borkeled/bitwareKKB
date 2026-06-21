#include "WallCheck.h"
#include <cmath>
#include <algorithm>
#include <Globals.hxx>
#include <Engine/Offsets/Offsets.h>
#include <Infrastructure/Obfuscation.h>

WallCheck::WallCheck()
{
    OBF_PROLOGUE;
    thread_running = true;
    cache_thread = std::thread(&WallCheck::cache_loop, this);
}

WallCheck::~WallCheck()
{
    OBF_PROLOGUE;
    thread_running = false;
    if (cache_thread.joinable())
    {
        OBF_JUNK_BLOCK;
        cache_thread.join();
    }
}

void WallCheck::cache_loop()
{
    OBF_PROLOGUE;
    OBF_JUNK_DECLARE;
    while (thread_running)
    {
        cache_workspace();
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
        for (int i = 0; i < 15 && thread_running; ++i)
        {
            OBF_JUNK_BLOCK;
            SDK::sleep_jitter(100, 20);
        }
    }
}

namespace
{
    static bool is_basepart_class(const std::string& className)
    {
        OBF_PROLOGUE;
        if (className.empty())
        {
            OBF_JUNK_BLOCK;
            return false;
        }

        if (className == "Terrain")
            return false;

        if (className.find("Part") != std::string::npos)
            return true;

        if (className.find("Operation") != std::string::npos)
            return true;

        if (className.find("Seat") != std::string::npos)
            return true;

        return className == "SpawnLocation" || 
               className == "SkateboardPlatform" || 
               className == "Platform" || 
               className == "FlagStand";
    }

    inline float dot(const SDK::Vector3& a, const SDK::Vector3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline void transform_to_local(const OBB& box, const SDK::Vector3& world_point, const SDK::Vector3& world_dir,
                                   SDK::Vector3& out_local_point, SDK::Vector3& out_local_dir)
    {
        SDK::Vector3 rel = world_point - box.center;
        
        out_local_point.x = dot(rel, box.axes[0]);
        out_local_point.y = dot(rel, box.axes[1]);
        out_local_point.z = dot(rel, box.axes[2]);

        out_local_dir.x = dot(world_dir, box.axes[0]);
        out_local_dir.y = dot(world_dir, box.axes[1]);
        out_local_dir.z = dot(world_dir, box.axes[2]);
    }

    inline bool intersect_sphere(const SDK::Vector3& local_origin, const SDK::Vector3& local_dir, float radius, float distance)
    {
        float b = dot(local_origin, local_dir);
        float c = dot(local_origin, local_origin) - radius * radius;
        float h = b * b - c;
        if (h < 0.0f)
            return false;
        float sqrt_h = std::sqrt(h);
        float t1 = -b - sqrt_h;
        if (t1 >= 0.0f && t1 <= distance)
            return true;
        float t2 = -b + sqrt_h;
        if (t2 >= 0.0f && t2 <= distance)
            return true;
        return false;
    }

    inline bool intersect_cylinder(const SDK::Vector3& O, const SDK::Vector3& D, const SDK::Vector3& half_size, float distance)
    {
        float hx = half_size.x;
        float r = half_size.y;

        float A = D.y * D.y + D.z * D.z;
        float B = O.y * D.y + O.z * D.z;
        float C = O.y * O.y + O.z * O.z - r * r;

        if (A > 0.00001f)
        {
            float disc = B * B - A * C;
            if (disc >= 0.0f)
            {
                float sqrt_disc = std::sqrt(disc);
                float t1 = (-B - sqrt_disc) / A;
                if (t1 >= 0.0f && t1 <= distance)
                {
                    float x = O.x + t1 * D.x;
                    if (x >= -hx && x <= hx)
                        return true;
                }
                float t2 = (-B + sqrt_disc) / A;
                if (t2 >= 0.0f && t2 <= distance)
                {
                    float x = O.x + t2 * D.x;
                    if (x >= -hx && x <= hx)
                        return true;
                }
            }
        }
        else
        {
            if (O.y * O.y + O.z * O.z > r * r)
                return false;
        }

        if (std::abs(D.x) > 0.00001f)
        {
            float t_cap1 = (-hx - O.x) / D.x;
            if (t_cap1 >= 0.0f && t_cap1 <= distance)
            {
                float y = O.y + t_cap1 * D.y;
                float z = O.z + t_cap1 * D.z;
                if (y * y + z * z <= r * r)
                    return true;
            }
            float t_cap2 = (hx - O.x) / D.x;
            if (t_cap2 >= 0.0f && t_cap2 <= distance)
            {
                float y = O.y + t_cap2 * D.y;
                float z = O.z + t_cap2 * D.z;
                if (y * y + z * z <= r * r)
                    return true;
            }
        }
        return false;
    }

    inline bool intersect_wedge(const SDK::Vector3& O, const SDK::Vector3& D, const SDK::Vector3& half_size, float distance)
    {
        float hx = half_size.x;
        float hy = half_size.y;
        float hz = half_size.z;

        if (std::abs(D.y) > 0.00001f)
        {
            float t = (-hy - O.y) / D.y;
            if (t >= 0.0f && t <= distance)
            {
                float x = O.x + t * D.x;
                float z = O.z + t * D.z;
                if (x >= -hx && x <= hx && z >= -hz && z <= hz)
                    return true;
            }
        }

        if (std::abs(D.z) > 0.00001f)
        {
            float t = (hz - O.z) / D.z;
            if (t >= 0.0f && t <= distance)
            {
                float x = O.x + t * D.x;
                float y = O.y + t * D.y;
                if (x >= -hx && x <= hx && y >= -hy && y <= hy)
                    return true;
            }
        }

        if (std::abs(D.x) > 0.00001f)
        {
            float t = (-hx - O.x) / D.x;
            if (t >= 0.0f && t <= distance)
            {
                float z = O.z + t * D.z;
                if (z >= -hz && z <= hz)
                {
                    float y = O.y + t * D.y;
                    float max_y = z * (hy / hz);
                    if (y >= -hy && y <= max_y)
                        return true;
                }
            }
        }

        if (std::abs(D.x) > 0.00001f)
        {
            float t = (hx - O.x) / D.x;
            if (t >= 0.0f && t <= distance)
            {
                float z = O.z + t * D.z;
                if (z >= -hz && z <= hz)
                {
                    float y = O.y + t * D.y;
                    float max_y = z * (hy / hz);
                    if (y >= -hy && y <= max_y)
                        return true;
                }
            }
        }

        float k = hy / hz;
        float denom = D.y - k * D.z;
        if (std::abs(denom) > 0.00001f)
        {
            float t = (k * O.z - O.y) / denom;
            if (t >= 0.0f && t <= distance)
            {
                float x = O.x + t * D.x;
                float z = O.z + t * D.z;
                if (x >= -hx && x <= hx && z >= -hz && z <= hz)
                    return true;
            }
        }

        return false;
    }

    inline bool intersect_triangle(const SDK::Vector3& O, const SDK::Vector3& D,
                                   const SDK::Vector3& v0, const SDK::Vector3& v1, const SDK::Vector3& v2,
                                   float distance)
    {
        SDK::Vector3 edge1{ v1.x - v0.x, v1.y - v0.y, v1.z - v0.z };
        SDK::Vector3 edge2{ v2.x - v0.x, v2.y - v0.y, v2.z - v0.z };
        SDK::Vector3 h{ D.y * edge2.z - D.z * edge2.y, D.z * edge2.x - D.x * edge2.z, D.x * edge2.y - D.y * edge2.x };
        float a = edge1.x * h.x + edge1.y * h.y + edge1.z * h.z;
        if (a > -0.00001f && a < 0.00001f)
            return false;
        float f = 1.0f / a;
        SDK::Vector3 s{ O.x - v0.x, O.y - v0.y, O.z - v0.z };
        float u = f * (s.x * h.x + s.y * h.y + s.z * h.z);
        if (u < 0.0f || u > 1.0f)
            return false;
        SDK::Vector3 q{ s.y * edge1.z - s.z * edge1.y, s.z * edge1.x - s.x * edge1.z, s.x * edge1.y - s.y * edge1.x };
        float v = f * (D.x * q.x + D.y * q.y + D.z * q.z);
        if (v < 0.0f || u + v > 1.0f)
            return false;
        float t = f * (edge2.x * q.x + edge2.y * q.y + edge2.z * q.z);
        return (t >= 0.0f && t <= distance);
    }

    inline bool intersect_corner_wedge(const SDK::Vector3& O, const SDK::Vector3& D, const SDK::Vector3& half_size, float distance)
    {
        float hx = half_size.x;
        float hy = half_size.y;
        float hz = half_size.z;

        SDK::Vector3 v0{ -hx, -hy, -hz };
        SDK::Vector3 v1{ -hx, hy, -hz };
        SDK::Vector3 v2{ hx, -hy, -hz };
        SDK::Vector3 v3{ -hx, -hy, hz };

        if (intersect_triangle(O, D, v0, v3, v2, distance)) return true;
        if (intersect_triangle(O, D, v0, v1, v3, distance)) return true;
        if (intersect_triangle(O, D, v0, v2, v1, distance)) return true;
        if (intersect_triangle(O, D, v1, v2, v3, distance)) return true;

        return false;
    }

    inline bool ray_intersects_obstacle(const OBB& box, const SDK::Vector3& origin, const SDK::Vector3& dir, float distance)
    {
        if (box.shape == 1 || box.is_mesh_or_operation)
        {
            return box.intersects(origin, dir, distance);
        }

        SDK::Vector3 local_origin, local_dir;
        transform_to_local(box, origin, dir, local_origin, local_dir);

        switch (box.shape)
        {
            case 0:
            {
                float radius = (std::min)((std::min)(box.half_size.x, box.half_size.y), box.half_size.z);
                return intersect_sphere(local_origin, local_dir, radius, distance);
            }
            case 2:
            {
                return intersect_cylinder(local_origin, local_dir, box.half_size, distance);
            }
            case 3:
            {
                return intersect_wedge(local_origin, local_dir, box.half_size, distance);
            }
            case 4:
            {
                return intersect_corner_wedge(local_origin, local_dir, box.half_size, distance);
            }
            default:
                break;
        }

        return box.intersects(origin, dir, distance);
    }
}

void WallCheck::find_valid_parts(SDK::Instance* instance, int depth, std::vector<OBB>& out_obstacles) const
{
    OBF_PROLOGUE;
    if (depth > 96)
    {
        OBF_JUNK_BLOCK;
        return;
    }

    auto children = instance->Children();
    for (auto& child : children)
    {
        if (!child.Address)
        {
            OBF_JUNK_BLOCK;
            continue;
        }

        std::string className = child.Class();

        if (is_basepart_class(className))
        {
            uintptr_t primAddr = Driver->Read<uintptr_t>(child.Address + Offsets::BasePart::Primitive);
            if (!primAddr)
                continue;

            uintptr_t flags = Driver->Read<uintptr_t>(primAddr + Offsets::Primitive::Flags);
            if (!(flags & Offsets::PrimitiveFlags::CanQuery))
                continue;

            SDK::Vector3 pos = Driver->Read<SDK::Vector3>(primAddr + Offsets::Primitive::Position);
            SDK::Matrix3 rot = Driver->Read<SDK::Matrix3>(primAddr + Offsets::Primitive::Rotation);
            SDK::Vector3 size = Driver->Read<SDK::Vector3>(primAddr + Offsets::Primitive::Size);

            if (className.find("MeshPart") != std::string::npos || className.find("Operation") != std::string::npos)
            {
                size.x *= 0.7f;
                size.y *= 0.7f;
                size.z *= 0.7f;
            }

            float mn = (std::min)((std::min)(size.x, size.y), size.z);
            float mx = (std::max)((std::max)(size.x, size.y), size.z);
            if (!std::isfinite(mn) || !std::isfinite(mx) || mn < 0.08f || mx > 8000.f)
                continue;

            if (mx <= 1000.f && mn >= 0.1f)
            {
                bool is_mesh_or_op = (className.find("MeshPart") != std::string::npos || className.find("Operation") != std::string::npos);
                uint8_t shape_val = 1; // Default: Block
                if (className == "WedgePart")
                    shape_val = 3;
                else if (className == "CornerWedgePart")
                    shape_val = 4;
                else if (className == "Part")
                    shape_val = Driver->Read<uint8_t>(child.Address + Offsets::BasePart::Shape);

                OBB obb(pos, size, rot, primAddr, is_mesh_or_op, shape_val);
                out_obstacles.push_back(obb);
            }
        }
        else if (className == "Folder")
        {
            find_valid_parts(&child, depth + 1, out_obstacles);
        }
        else if (className == "Model")
        {
            SDK::Instance humanoid = child.Find_First_Child_Of_Class("Humanoid");
            if (humanoid.Address != 0)
                continue;

            find_valid_parts(&child, depth + 1, out_obstacles);
        }
        else if (className == "Actor" || className == "WorldRoot" || className == "WorldModel" || className == "Tool")
        {
            find_valid_parts(&child, depth + 1, out_obstacles);
        }
    }
}

bool WallCheck::cache_workspace()
{
    OBF_PROLOGUE;
    if (Globals::Workspace.Address == 0)
    {
        std::lock_guard<std::mutex> lock(mtx);
        obstacles.clear();
        cached = false;
        cache_failed = true;
        OBF_JUNK_BLOCK;
        return false;
    }

    std::vector<OBB> temp_obstacles;
    SDK::Instance workspace(Globals::Workspace.Address);
    find_valid_parts(&workspace, 0, temp_obstacles);

    std::lock_guard<std::mutex> lock(mtx);
    obstacles = std::move(temp_obstacles);
    cached = !obstacles.empty();
    cache_failed = !cached;
    return cached;
}

bool WallCheck::is_visible(const SDK::Vector3& origin, const SDK::Vector3& target)
{
    OBF_PROLOGUE;
    if (Globals::Settings::WallCheck_Method == 1)
    {
        OBF_JUNK_BLOCK;
        return is_visible_raycast(origin, target);
    }

    std::lock_guard<std::mutex> lock(mtx);
    if (cached)
    {
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
        return is_visible_locked(origin, target);
    }

    return true;
}

bool WallCheck::is_visible_locked(const SDK::Vector3& origin, const SDK::Vector3& target) const
{
    OBF_PROLOGUE;
    if (!cached)
    {
        OBF_JUNK_BLOCK;
        return true;
    }

    SDK::Vector3 dir = (target - origin).normalize();
    float distance = (target - origin).magnitude();

    if (distance <= 0.f)
        return true;

    for (const auto& box : obstacles)
    {
        float max_obb_extent = (std::max)((std::max)(box.half_size.x, box.half_size.y), box.half_size.z);

        float dx = box.center.x - origin.x;
        float dy = box.center.y - origin.y;
        float dz = box.center.z - origin.z;
        float dist_to_center_sq = dx * dx + dy * dy + dz * dz;

        float max_dist = distance + max_obb_extent;
        if (dist_to_center_sq > max_dist * max_dist)
            continue;

        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

        if (ray_intersects_obstacle(box, origin, dir, distance))
            return false;
    }

    return true;
}

bool WallCheck::is_visible_raycast(const SDK::Vector3& origin, const SDK::Vector3& target) const
{
    OBF_PROLOGUE;
    if (Globals::Workspace.Address == 0)
    {
        OBF_JUNK_BLOCK;
        return true;
    }

    std::vector<OBB> local_obstacles;
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!cached)
            return true;
        local_obstacles = obstacles;
    }

    SDK::Vector3 dir = (target - origin).normalize();
    float distance = (target - origin).magnitude();

    if (distance <= 0.f)
        return true;

    for (const auto& box : local_obstacles)
    {
        float max_obb_extent = (std::max)((std::max)(box.half_size.x, box.half_size.y), box.half_size.z);

        SDK::Vector3 to_center = box.center - origin;
        float t = to_center.x * dir.x + to_center.y * dir.y + to_center.z * dir.z;
        
        float t_clamped = (std::max)(0.0f, (std::min)(distance, t));
        SDK::Vector3 closest_point = origin + dir * t_clamped;
        
        float dx = box.center.x - closest_point.x;
        float dy = box.center.y - closest_point.y;
        float dz = box.center.z - closest_point.z;
        float dist_to_ray_sq = dx * dx + dy * dy + dz * dz;

        float safety_margin = max_obb_extent + 35.f;
        if (dist_to_ray_sq > safety_margin * safety_margin)
            continue;

        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }

        if (!box.primitive_address)
        {
            if (box.intersects(origin, dir, distance))
                return false;
            continue;
        }

        SDK::Vector3 live_pos = Driver->Read<SDK::Vector3>(box.primitive_address + Offsets::Primitive::Position);
        SDK::Matrix3 live_rot = Driver->Read<SDK::Matrix3>(box.primitive_address + Offsets::Primitive::Rotation);
        SDK::Vector3 live_size = Driver->Read<SDK::Vector3>(box.primitive_address + Offsets::Primitive::Size);

        if (box.is_mesh_or_operation)
        {
            live_size.x *= 0.7f;
            live_size.y *= 0.7f;
            live_size.z *= 0.7f;
        }

        float mn = (std::min)((std::min)(live_size.x, live_size.y), live_size.z);
        float mx = (std::max)((std::max)(live_size.x, live_size.y), live_size.z);
        if (!std::isfinite(mn) || !std::isfinite(mx) || mn < 0.08f || mx > 8000.f)
            continue;

        if (mx <= 1000.f && mn >= 0.1f)
        {
            OBB live_obb(live_pos, live_size, live_rot, box.primitive_address, box.is_mesh_or_operation, box.shape);
            if (ray_intersects_obstacle(live_obb, origin, dir, distance))
                return false;
        }
    }

    return true;
}

void WallCheck::clear()
{
    OBF_PROLOGUE;
    std::lock_guard<std::mutex> lock(mtx);
    obstacles.clear();
    cached = false;
    cache_failed = false;
}
