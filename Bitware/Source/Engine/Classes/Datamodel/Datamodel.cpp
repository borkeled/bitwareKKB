#include "../../Engine.h"
#include <Infrastructure/Obfuscation.h>

namespace SDK {

	std::uint64_t SDK::Datamodel::Get_PlaceID() const {
        OBF_PROLOGUE;
		return g_Memory->Read<uint64_t>(this->Address + Offsets::DataModel::PlaceId);
	}

	std::uint64_t SDK::Datamodel::Get_GameID() const {
        OBF_PROLOGUE;
        OBF_JUNK_BLOCK;
		return g_Memory->Read<uint64_t>(this->Address + Offsets::DataModel::GameId);
	}

	std::uint64_t SDK::Datamodel::Get_CreatorID() const {
        OBF_PROLOGUE;
		return g_Memory->Read<uint64_t>(this->Address + Offsets::DataModel::CreatorId);
	}

	std::uint64_t SDK::Datamodel::Get_ServerIP() const {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;
		return g_Memory->Read<uint64_t>(this->Address + Offsets::DataModel::ServerIP);
	}
}
