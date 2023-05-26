#include "InjectionLock.h"

#include <Windows.h>
#include <iostream>

InjectionLock::InjectionLock() {
	this->hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(LockState), "InjectionLock");
	auto lastError = GetLastError();
	if (this->hMapping == NULL) {
		throw std::exception("CreateFileMapping");
	}
	this->pLock = static_cast<LockState*>(MapViewOfFile(this->hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(LockState)));
	if (this->pLock == nullptr) {
		throw std::exception("MapViewOfFile");
	}
	if (lastError != ERROR_ALREADY_EXISTS)
	{
		std::cout << "Initializing lock" << std::endl;
		this->pLock->Locked = false;
		this->pLock->LockRequested = false;
	}

	this->Acquire();
}

InjectionLock::~InjectionLock() {
	if (this->pLock->Locked) this->Release();
	if (this->pLock) UnmapViewOfFile(this->pLock);
	if (this->hMapping) {
		CloseHandle(this->hMapping);
		this->hMapping = NULL;
	}
}

void InjectionLock::WaitForLockRequest() {
	std::cout << "Waiting for lock request" << std::endl;
	while (!this->pLock->LockRequested) Sleep(100);
	std::cout << "Lock request received" << std::endl;
}

void InjectionLock::Acquire() {
	std::cout << "Requesting for lock" << std::endl;
	this->pLock->LockRequested = true;
	std::cout << "Waiting for lock to be released" << std::endl;
	while (this->pLock->Locked) Sleep(100);
	std::cout << "Acquiring lock" << std::endl;
	this->pLock->Locked = true;
	this->pLock->LockRequested = false;

}

void InjectionLock::Release() {
	std::cout << "Releasing lock" << std::endl;
	this->pLock->Locked = false;
}