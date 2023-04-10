#include "injector.h"
#include <Windows.h>
#include <Psapi.h>

#include <string>
#include <vector>
#include <filesystem>

PROCESS_INFORMATION create_suspended_process(std::string& bin) {
	STARTUPINFO si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	auto success = FALSE != CreateProcessA(bin.c_str(), NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
	if (!success) {
		throw std::exception("error: CreateProcessA");
	}
	return pi;
}

HANDLE find_process_by_name(std::string& process_name) {
	DWORD bytes_read;
	DWORD process_ids[2048];
	if (0 == EnumProcesses(process_ids, sizeof(process_ids), &bytes_read)) {
		auto err = GetLastError();
		std::cout << "GetLastError " << err;
		throw std::exception("find_process_by_name: EnumProcesses failed");
	}

	DWORD num_processes = bytes_read / sizeof(process_ids[0]);
	char process_name_buf[1024];
	std::cout << "num" << num_processes << std::endl;
	for (DWORD i = 0; i < num_processes; i++)
	{
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_ids[i]);
		HMODULE hModule;
		DWORD dummy;
		if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &dummy))
		{
			GetModuleBaseName(hProcess, hModule, process_name_buf, sizeof(process_name_buf));
			auto current_process_name = std::string(process_name_buf);
			std::cout << current_process_name << std::endl;
			if (current_process_name == process_name)
			{
				return hProcess;
			}
			else
			{
				CloseHandle(hProcess);
			}
		}
	}

	throw std::exception("find_process_by_name: Process not found");
}

void inject_dll(PROCESS_INFORMATION pi, std::string& dll) {
	// TODO: Cleanup (e.g. free remoteDllPath)
	LPVOID remoteDllPath = VirtualAllocEx(pi.hProcess, NULL, dll.size() + 1, MEM_COMMIT, PAGE_READWRITE);
	if (remoteDllPath == NULL) {
		throw std::exception("inject_dll: VirtualAllocEx");
	}

	SIZE_T written;
	if (FALSE == WriteProcessMemory(pi.hProcess, remoteDllPath, dll.c_str(), dll.size(), &written)) {
		throw std::exception("inject_dll: WriteProcessMemory");
	}
	if (written != dll.size()) {
		throw std::exception("inject_dll: written != 1");
	}

	HMODULE hModule = GetModuleHandleA("kernel32");
	if (hModule == NULL) {
		throw std::exception("inject_dll: hModule == NULL");
	}

	FARPROC loadLibrary = GetProcAddress(hModule, "LoadLibraryA");
	if (loadLibrary == NULL) {
		throw std::exception("inject_dll: GetProcAddress");
	}

	HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibrary, remoteDllPath, 0, NULL);
	if (hThread == NULL) {
		throw std::exception("inject_dll: CreateRemoteThread");
	}
}

void resume_process(PROCESS_INFORMATION pi) {
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

void kill_process(PROCESS_INFORMATION pi) {
	TerminateProcess(pi.hProcess, EXIT_SUCCESS);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}
std::string ExePath() {
	TCHAR buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

int main(int argc, char* argv[]) {
	try {
		std::vector<std::string> args(argv, argv + argc);

#ifndef NDEBUG
		// Defaults for testing
		if (args.size() == 1) {
			args.push_back("IdleDragons.exe");
			args.push_back("..\\dll\\dll.dll");
		}
#endif

		if (args.size() != 3) {
			std::cout << "Usage: injector <target process> <dll>" << std::endl;
			std::exit(EXIT_SUCCESS);
		}

		auto target = args[1];
		auto dll_original = std::filesystem::canonical(args[2]).string();

		auto dll = std::string(std::tmpnam(nullptr));
		std::filesystem::copy(dll_original, dll);

		PROCESS_INFORMATION process{};
		bool terminate_on_failure;

		try {
			std::cout << "Trying to find process by name" << std::endl;
			auto handle = find_process_by_name(target);
			process.hProcess = handle;
			terminate_on_failure = false;
		}
		catch (std::exception& ex) {
			std::cout << "Finding process by name failed: " << ex.what() << std::endl;
			process = create_suspended_process(target);
			terminate_on_failure = true;
		}

		try {
			inject_dll(process, dll);
			resume_process(process);
		}
		catch (std::exception& ex) {
			std::cout << "error: " << ex.what() << std::endl;
			if (terminate_on_failure) {
				kill_process(process);
			}
			throw ex;
		}
	}
	catch (std::exception& ex) {
		std::cout << "error: " << ex.what() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}
