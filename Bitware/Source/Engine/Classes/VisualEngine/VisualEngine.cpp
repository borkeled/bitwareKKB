#include "../../Engine.h"
#include <Globals.hxx>
#include <Infrastructure/Obfuscation.h>

namespace SDK {

	SDK::Vector2 SDK::VisualEngine::Get_Dimensions() const {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
		return g_Memory->Read<SDK::Vector2>(this->Address + Offsets::VisualEngine::Dimensions);
	}

	SDK::Matrix4 SDK::VisualEngine::Get_ViewMatrix() const {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
		return g_Memory->Read<SDK::Matrix4>(this->Address + Offsets::VisualEngine::ViewMatrix);
	}

	static bool IsValidViewMatrix(const SDK::Matrix4& M) {
        OBF_PROLOGUE;
		OBF_JUNK_BLOCK;
		return std::isfinite(M.data[0]) && std::isfinite(M.data[1]) && std::isfinite(M.data[2]) && std::isfinite(M.data[3])
			&& std::isfinite(M.data[4]) && std::isfinite(M.data[5]) && std::isfinite(M.data[6]) && std::isfinite(M.data[7])
			&& std::isfinite(M.data[8]) && std::isfinite(M.data[9]) && std::isfinite(M.data[10]) && std::isfinite(M.data[11])
			&& std::isfinite(M.data[12]) && std::isfinite(M.data[13]) && std::isfinite(M.data[14]) && std::isfinite(M.data[15]);
	}

	static bool IsValidScreenDimensions(const SDK::Vector2& Dims) {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
		return Dims.x > 1.f && Dims.x < 16000.f && Dims.y > 1.f && Dims.y < 16000.f;
	}

	SDK::Vector2 SDK::VisualEngine::World_To_Screen(const SDK::Vector3& World)  const {
        OBF_PROLOGUE;
		if (this->Address == 0) {
			return { -1, -1 };
		}

		static thread_local std::uint64_t LastFrame = 0;
		static thread_local std::uintptr_t LastAddress = 0;
		static thread_local SDK::Matrix4 CachedViewMatrix{};
		static thread_local SDK::Vector2 CachedDimensions{};

		std::uint64_t CurrentFrame = Globals::FrameCount.load(std::memory_order_relaxed);
		if (CurrentFrame != LastFrame || this->Address != LastAddress) {
			SDK::Matrix4 NewMatrix = Get_ViewMatrix();
			SDK::Vector2 NewDimensions = Get_Dimensions();

			if (IsValidViewMatrix(NewMatrix) && IsValidScreenDimensions(NewDimensions)) {
				CachedViewMatrix = NewMatrix;
				CachedDimensions = NewDimensions;
			}
			LastFrame = CurrentFrame;
			LastAddress = this->Address;
		}

		SDK::Vector4 Q;
		Q.x = (World.x * CachedViewMatrix.data[0]) + (World.y * CachedViewMatrix.data[1]) + (World.z * CachedViewMatrix.data[2]) + CachedViewMatrix.data[3];
		Q.y = (World.x * CachedViewMatrix.data[4]) + (World.y * CachedViewMatrix.data[5]) + (World.z * CachedViewMatrix.data[6]) + CachedViewMatrix.data[7];
		Q.z = (World.x * CachedViewMatrix.data[8]) + (World.y * CachedViewMatrix.data[9]) + (World.z * CachedViewMatrix.data[10]) + CachedViewMatrix.data[11];
		Q.w = (World.x * CachedViewMatrix.data[12]) + (World.y * CachedViewMatrix.data[13]) + (World.z * CachedViewMatrix.data[14]) + CachedViewMatrix.data[15];
		if (Q.w < 0.1f)
			return { -1, -1 };
		if (CachedDimensions.x <= 0 || CachedDimensions.y <= 0)
			return { -1, -1 };
		SDK::Vector3 NDC;
		NDC.x = Q.x / Q.w;
		NDC.y = Q.y / Q.w;
		NDC.z = Q.z / Q.w;
		SDK::Vector2 ScreenPosition =
		{
			(CachedDimensions.x / 2 * NDC.x) + (CachedDimensions.x / 2),
			-(CachedDimensions.y / 2 * NDC.y) + (CachedDimensions.y / 2),
		};
		if (ScreenPosition.x < 0 || ScreenPosition.x > CachedDimensions.x || ScreenPosition.y < 0 || ScreenPosition.y > CachedDimensions.y)
			return { -1, -1 };
		OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
		return ScreenPosition;
	}
}
