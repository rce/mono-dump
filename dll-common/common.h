#pragma once

#include <iostream>
#include <Windows.h>

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




#include <sstream>
void WaitForDebugger() {
	if (IsDebuggerPresent()) return;
	std::cout << "Waiting for debugger... ";
	std::stringstream cmd;
	cmd << "vsjitdebugger.exe -p " << std::dec << GetCurrentProcessId();
	system(cmd.str().c_str());
	while (!IsDebuggerPresent()) Sleep(100);
	std::cout << "Debugger present!" << std::endl;
}