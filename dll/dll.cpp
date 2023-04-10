#include "InspectMono.h"
#include "InjectionLock.h"

#include <Windows.h>

#include <exception>
#include <iostream>
#include <cstdint>


void initConsole()
{
	if (GetConsoleWindow() == NULL)
	{
		AllocConsole();
		FILE* f;
		freopen_s(&f, "CONOUT$", "w", stdout);
		freopen_s(&f, "CONOUT$", "w", stderr);
		std::cout.clear();
		std::cin.clear();
	}
}


DWORD WINAPI MainThread(LPVOID lpThreadParameter)
{
	HMODULE hModule = static_cast<HMODULE>(lpThreadParameter);
	initConsole();
	InjectionLock lock{};

	try {
		InspectMono();
	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}
	lock.WaitForLockRequest();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		HANDLE hThread = CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
		if (hThread) {
			CloseHandle(hThread);
		}
	}
	return TRUE;
}