#include "kdmapper.hpp"
#include <Infrastructure/Logger.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>  // for WRAPPER_MARCO

uint64_t kdmapper::MapDriver(IDriverAbstraction* driver, const std::string& driver_path)
{
	std::vector<uint8_t> raw_image = { 0 };

	if (!utils::ReadFileToMemory(driver_path, &raw_image))
	{
		return 0;
	}

	return MapDriver(driver, raw_image);
}

uint64_t kdmapper::MapDriver(IDriverAbstraction* driver, const std::vector<uint8_t>& raw_image)
{
	const PIMAGE_NT_HEADERS64 nt_headers = portable_executable::GetNtHeaders(const_cast<uint8_t*>(raw_image.data()));

	if (!nt_headers)
	{
		return 0;
	}

	if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		return 0;
	}

	const uint32_t image_size = nt_headers->OptionalHeader.SizeOfImage;

	Logger::LogHex(WRAPPER_MARCO("[kdmapper] image_size"), image_size);

	void* local_image_base = VirtualAlloc(nullptr, image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	Logger::Log(WRAPPER_MARCO("[kdmapper] calling AllocatePool..."));
	uint64_t kernel_image_base = driver->AllocatePool(nt::NonPagedPool, image_size);
	Logger::LogHex(WRAPPER_MARCO("[kdmapper] kernel_image_base"), kernel_image_base);

	do
	{
		if (!kernel_image_base)
		{
			Logger::Log(WRAPPER_MARCO("[kdmapper] FAIL AllocatePool returned 0"));
			break;
		}

		memcpy(local_image_base, raw_image.data(), nt_headers->OptionalHeader.SizeOfHeaders);

		const PIMAGE_SECTION_HEADER current_image_section = IMAGE_FIRST_SECTION(nt_headers);

		for (auto i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i)
		{
			auto local_section = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(local_image_base) + current_image_section[i].VirtualAddress);
			memcpy(local_section, reinterpret_cast<void*>(reinterpret_cast<uint64_t>(raw_image.data()) + current_image_section[i].PointerToRawData), current_image_section[i].SizeOfRawData);
		}

		RelocateImageByDelta(portable_executable::GetRelocs(local_image_base), kernel_image_base - nt_headers->OptionalHeader.ImageBase);

		Logger::Log(WRAPPER_MARCO("[kdmapper] ResolveImports..."));
		if (!ResolveImports(driver, portable_executable::GetImports(local_image_base)))
		{
			Logger::Log(WRAPPER_MARCO("[kdmapper] FAIL ResolveImports"));
			break;
		}

		Logger::Log(WRAPPER_MARCO("[kdmapper] WriteMemory..."));
		if (!driver->WriteMemory(kernel_image_base, local_image_base, image_size))
		{
			Logger::Log(WRAPPER_MARCO("[kdmapper] FAIL WriteMemory"));
			break;
		}
		Logger::Log(WRAPPER_MARCO("[kdmapper] WriteMemory OK"));

		VirtualFree(local_image_base, 0, MEM_RELEASE);

		// Call entry point with null arguments (the driver self-allocates its DriverObject)
		const uint64_t address_of_entry_point = kernel_image_base + nt_headers->OptionalHeader.AddressOfEntryPoint;
		Logger::LogHex(WRAPPER_MARCO("[kdmapper] address_of_entry_point"), address_of_entry_point);

		NTSTATUS status = 0;

		Logger::Log(WRAPPER_MARCO("[kdmapper] CallKernelFunction(entry_point)..."));
		if (!driver->CallKernelFunction(&status, address_of_entry_point, 0ULL, 0ULL))
		{
			Logger::Log(WRAPPER_MARCO("[kdmapper] FAIL CallKernelFunction"));
			Logger::LogHex(WRAPPER_MARCO("[kdmapper] entry_point NTSTATUS"), status);
			break;
		}
		Logger::LogHex(WRAPPER_MARCO("[kdmapper] entry_point NTSTATUS"), status);

		Logger::Log(WRAPPER_MARCO("[kdmapper] SetMemory (clear headers)..."));
		driver->SetMemory(kernel_image_base, 0, nt_headers->OptionalHeader.SizeOfHeaders);
		Logger::LogHex(WRAPPER_MARCO("[kdmapper] MapDriver SUCCESS, base"), kernel_image_base);

		VirtualFree(local_image_base, 0, MEM_RELEASE);
		local_image_base = nullptr;
		return kernel_image_base;

	} while (false);

	if (local_image_base)
		VirtualFree(local_image_base, 0, MEM_RELEASE);
	if (kernel_image_base)
		driver->FreePool(kernel_image_base);

	return 0;
}

void kdmapper::RelocateImageByDelta(portable_executable::vec_relocs relocs, const uint64_t delta)
{
	for (const auto& current_reloc : relocs)
	{
		for (auto i = 0u; i < current_reloc.count; ++i)
		{
			const uint16_t type = current_reloc.item[i] >> 12;
			const uint16_t offset = current_reloc.item[i] & 0xFFF;

			if (type == IMAGE_REL_BASED_DIR64)
				*reinterpret_cast<uint64_t*>(current_reloc.address + offset) += delta;
		}
	}
}

bool kdmapper::ResolveImports(IDriverAbstraction* driver, portable_executable::vec_imports imports)
{
	for (const auto& current_import : imports)
	{
		if (!utils::GetKernelModuleAddress(current_import.module_name))
		{
			return false;
		}

		for (auto& current_function_data : current_import.function_datas)
		{
			const uint64_t function_address = driver->GetKernelModuleExport(utils::GetKernelModuleAddress(current_import.module_name), current_function_data.name);

			if (!function_address)
			{
				return false;
			}

			*current_function_data.address = function_address;
		}
	}

	return true;
}
