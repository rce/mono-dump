#include "InspectMono.h"
#include "../dll-common/InjectionLock.h"

#include <Windows.h>

#include <exception>
#include <iostream>
#include <cstdint>
#include "../dll-common/mono.h"
#include "../dll-common/common.h"

void MainThread()
{
	initConsole();
	InjectionLock lock{};

#ifndef NDEBUG
	WaitForDebugger();
#endif

	try {
		InspectMono();
	} catch (std::exception& ex) {
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