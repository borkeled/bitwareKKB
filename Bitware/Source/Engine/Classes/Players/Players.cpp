#include "../../Engine.h"
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace SDK {

	std::uint64_t SDK::Players::Get_UserID() const {
        OBF_PROLOGUE;
		std::uint64_t userId = g_Memory->Read<std::uint64_t>(this->Address + Offsets::Player::UserId);
		return userId;
	}

	std::uint64_t SDK::Players::Get_Team() const {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
		return g_Memory->Read<uint64_t>(this->Address + Offsets::Player::Team);
	}

	SDK::Players SDK::Players::Get_Local_Player() const {
        OBF_PROLOGUE;
		std::uint64_t local_address = g_Memory->Read<std::uint64_t>(this->Address + Offsets::Player::LocalPlayer);
        OBF_JUNK_DECLARE;
		return SDK::Players{ local_address };
	}

	SDK::Instance SDK::Instance::Character() const {
        OBF_PROLOGUE;
		if (!this->Address)
		return SDK::Instance();
		return g_Memory->Read<SDK::Players>(this->Address + Offsets::Player::ModelInstance);
	}


	std::string Players::Get_DisplayName() const
	{
        OBF_PROLOGUE;
		if (!this->Address)
		return std::string(skCrypt("?"));
		return g_Memory->Read_String(this->Address + Offsets::Player::DisplayName);
	}
}
