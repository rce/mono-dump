#include <vector>
#include <Windows.h>
#include <memoryapi.h>
#include "../dll-common/mono.h"
#include "../dll-common/common.h"
#include "../dll-common/InjectionLock.h"
#undef far // Hack to make the dumped data structures compile as this is defined by Windows headers :)
#include "monodump.h"

std::vector<uintptr_t> ScanForValue(uint64_t value) {
	MEMORY_BASIC_INFORMATION mbi;
	uint64_t address = 0;
	std::vector<uint64_t> results{};
	while (VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi)) == sizeof(mbi)) {
		if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE) {
			auto size = mbi.RegionSize;
			auto buffer = new uint8_t[size];
			SIZE_T bytesRead;
			if (ReadProcessMemory(GetCurrentProcess(), mbi.BaseAddress, buffer, size, &bytesRead)) {
				for (int i = 0; i < bytesRead; i++) {
					auto ptr = (uint64_t*)(buffer + i);
					if (*ptr == value) {
						results.push_back((uint64_t)mbi.BaseAddress + i);
					}
				}
			}
			delete[] buffer;
		}
		address += mbi.RegionSize;
	}
	return results;
}

MonoClass* FindClass(const std::string& className) {
	for (auto& assembly : mono::get_assemblies()) {
		auto image = mono::assembly_get_image(assembly);
		auto image_name = std::string(mono::image_get_name(image));
		auto tdef = mono::image_get_table_info(image, MONO_TABLE_TYPEDEF);
		if (!tdef) continue;

		auto tdefcount = mono::table_info_get_rows(tdef);
		for (int i = 0; i < tdefcount; i++)
		{
			auto klass = mono::class_get(image, MONO_TOKEN_TYPE_DEF | (i + 1));
			auto name = mono::class_get_name(klass);
			auto ns = mono::class_get_namespace(klass);
			if (name == className) {
				return klass;
			}
		}
	}
	return nullptr;
}

bool CheckController(uintptr_t ptr, MonoDomain* domain, MonoClass* klass) {
	// https://github.com/mono/mono/blob/main/docs/object-layout
	struct mono_vtable_t {
		MonoClass* klass;
		MonoDomain* domain;
	};
	struct mono_object_t {
		mono_vtable_t* vtable;
	};

	std::cout << "Checking " << std::hex << ptr << std::endl;
	__try {
		// https://github.com/mono/mono/blob/main/docs/object-layout#L21-L29
		auto object = reinterpret_cast<mono_object_t*>(ptr);
		if (object->vtable->klass != klass /*|| object->vtable->domain != domain*/) {
			std::cout << "not the same class" << std::endl;
			return false;
		}
		auto controller = reinterpret_cast<monodump::CrusadersGame::GameScreen::CrusadersGameController*>(ptr);
		if (controller->area->gameController == controller) {
			if (*reinterpret_cast<uint8_t*>(ptr + 0x10C) == 0x00) {
				return true;
			}
		}
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
		return false;
	}
	return false;
}

monodump::CrusadersGame::GameScreen::CrusadersGameController* FindController() {
	mono::ThreadAttachment thread{};
	auto klass = FindClass("CrusadersGameController");
	if (klass == nullptr) {
		std::cout << "Failed to find class" << std::endl;
		OutputDebugString("Failed to find class");
		return nullptr;
	}
	auto domain = mono::get_root_domain();
	auto vtable = mono::class_vtable(domain, klass);

	for (auto result : ScanForValue(reinterpret_cast<uintptr_t>(vtable))) {
		if (CheckController(result, domain, klass)) {
			return reinterpret_cast<monodump::CrusadersGame::GameScreen::CrusadersGameController*>(result);
		}
	}
	return nullptr;
}

void MainThread()
{
	initConsole();
	InjectionLock lock{};

#ifndef NDEBUG
	WaitForDebugger();
#endif

	try {
		auto pController = FindController();
		std::cout << "Controller: " << std::hex << pController << std::endl;
		if (pController) {
			std::cout << "Controller found, letsagoo!" << std::endl;
			std::cout << "Level: " << std::dec << pController->activeCampaignData->currentArea->level << std::endl;
		}
	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}
	lock.WaitForLockRequest();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		HANDLE hThread = CreateThread(NULL, 0, [](LPVOID lpThreadParameter) -> DWORD {
			HMODULE hModule = static_cast<HMODULE>(lpThreadParameter);
			MainThread();
			FreeLibraryAndExitThread(hModule, 0);
		}, hModule, 0, NULL);
		if (hThread) {
			CloseHandle(hThread);
		}
	}
	return TRUE;
}