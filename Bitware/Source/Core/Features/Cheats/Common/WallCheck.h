#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <thread>
#include <atomic>
#include <Engine/Math/Math.h>
#include <Infrastructure/Obfuscation.h>

namespace SDK { struct Instance; }

struct OBB final
{
    SDK::Vector3 center;
    SDK::Vector3 half_size;
    SDK::Vector3 axes[3];
    uintptr_t primitive_address = 0;
    bool is_mesh_or_operation = false;
    uint8_t shape = 1;

    OBB(const SDK::Vector3& center, const SDK::Vector3& size, const SDK::Matrix3& rot, uintptr_t primAddr = 0, bool is_mesh_or_op = false, uint8_t shape = 1)
        : center(center), half_size(size * 0.5f), primitive_address(primAddr), is_mesh_or_operation(is_mesh_or_op), shape(shape)
    {
        axes[0] = SDK::Vector3(rot.data[0], rot.data[1], rot.data[2]);
        axes[1] = SDK::Vector3(rot.data[3], rot.data[4], rot.data[5]);
        axes[2] = SDK::Vector3(rot.data[6], rot.data[7], rot.data[8]);
    }

    static float hs(const SDK::Vector3& v, int i)
    {
        OBF_PROLOGUE;
        if (i == 0) return v.x;
        if (i == 1) return v.y;
        return v.z;
    }

    bool intersects(const SDK::Vector3& origin, const SDK::Vector3& dir, float max) const
    {
        OBF_PROLOGUE;
        float tmin = 0.0f;
        float tmax = max;

        SDK::Vector3 p = center - origin;

        for (int i = 0; i < 3; ++i)
        {
            float e = axes[i].x * p.x + axes[i].y * p.y + axes[i].z * p.z;
            float f_dir = axes[i].x * dir.x + axes[i].y * dir.y + axes[i].z * dir.z;

            const float EPSILON = 1e-6f;
            if (std::abs(f_dir) > EPSILON)
            {
                float h = hs(half_size, i);
                float t1 = (e + h) / f_dir;
                float t2 = (e - h) / f_dir;

                if (t1 > t2) std::swap(t1, t2);

                tmin = (std::max)(tmin, t1);
                tmax = (std::min)(tmax, t2);

                if (tmax < tmin)
                {
                    OBF_JUNK_BLOCK;
                    return false;
                }
            }
            else
            {
                float h = hs(half_size, i);
                if (-e - h > 0.0f || -e + h < 0.0f)
                    return false;
            }
        }

        return true;
    }
};

class WallCheck final {
public:
    WallCheck();
    ~WallCheck();

    bool cache_workspace();
    bool is_visible(const SDK::Vector3& origin, const SDK::Vector3& target);
    void clear();

private:
    mutable std::mutex mtx;
    std::vector<OBB> obstacles;
    bool cached = false;
    bool cache_failed = false;

    std::thread cache_thread;
    std::atomic<bool> thread_running{false};
    void cache_loop();

    bool is_visible_locked(const SDK::Vector3& origin, const SDK::Vector3& target) const;
    bool is_visible_raycast(const SDK::Vector3& origin, const SDK::Vector3& target) const;
    void find_valid_parts(SDK::Instance* instance, int depth, std::vector<OBB>& out_obstacles) const;
};

inline std::unique_ptr<WallCheck> wallcheck = std::make_unique<WallCheck>();
