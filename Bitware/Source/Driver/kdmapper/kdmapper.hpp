#pragma once
#include <Windows.h>
#include <stdint.h>
#include <vector>
#include <string>

#include "portable_executable.hpp"
#include "utils.hpp"
#include "nt.hpp"
#include "intel_driver.hpp"
#include "../DriverAbstraction/IDriverAbstraction.h"

namespace kdmapper
{
	uint64_t MapDriver(IDriverAbstraction* driver, const std::string& driver_path);
	uint64_t MapDriver(IDriverAbstraction* driver, const std::vector<uint8_t>& raw_image);
	void RelocateImageByDelta(portable_executable::vec_relocs relocs, const uint64_t delta);
	bool ResolveImports(IDriverAbstraction* driver, portable_executable::vec_imports imports);
}
