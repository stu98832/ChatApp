#pragma once
#include <Windows.h>

class CMutex {
private:
	HANDLE handle;
public:
	CMutex();
	~CMutex();

	void Lock();
	bool TryLook(int milliseconds = 0);
	void Release();
};