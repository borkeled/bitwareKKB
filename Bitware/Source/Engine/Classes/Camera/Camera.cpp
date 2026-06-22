#include "../../Engine.h"
#include <Infrastructure/Obfuscation.h>

namespace SDK {

	SDK::Vector3 SDK::Camera::Get_CameraPos() const {
        OBF_PROLOGUE;
		return g_Memory->Read<SDK::Vector3>(this->Address + Offsets::Camera::Position);
	}

	SDK::Matrix3 SDK::Camera::Get_CameraRot() const {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
		return g_Memory->Read<Matrix3>(this->Address + Offsets::Camera::Rotation);
	}

	void SDK::Camera::Set_CameraPos(const SDK::Vector3& pos) const {
        OBF_PROLOGUE;
		g_Memory->Write<SDK::Vector3>(this->Address + Offsets::Camera::Position, pos);
	}

	void SDK::Camera::Set_CameraRot(const SDK::Matrix3& rot) const {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
		g_Memory->Write<SDK::Matrix3>(this->Address + Offsets::Camera::Rotation, rot);
	}

	Vector2 SDK::Camera::FetchViewPort(Vector2 target_screen_pos, Vector2 screen_size) {
        OBF_PROLOGUE;
		Vector2 result;
		result.x = (int16_t)(2 * (screen_size.x - target_screen_pos.x));
		result.y = (int16_t)(2 * (screen_size.y - target_screen_pos.y));
        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
		return result;
	}

	void SDK::Camera::SetViewPort(SDK::ViewPort Vp) const {
        OBF_PROLOGUE;
		g_Memory->Write<SDK::ViewPort>(this->Address + Offsets::Camera::Viewport, Vp);
	}
}
