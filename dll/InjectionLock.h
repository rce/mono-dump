#include <Windows.h>

struct LockState {
	bool Locked;
	bool LockRequested;
};

class InjectionLock {
public:
	InjectionLock();
	~InjectionLock();
	void WaitForLockRequest();
private:
	HANDLE hMapping;
	LockState* pLock;
	void Acquire();
	void Release();
};