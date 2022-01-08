#include "CMutex.h"

CMutex::CMutex()
{
	this->handle = CreateMutexA(NULL, FALSE, NULL);
}

CMutex::~CMutex()
{
	CloseHandle(this->handle);
}

void CMutex::Lock()
{
	WaitForSingleObject(this->handle, INFINITE);
}

bool CMutex::TryLook(int milliseconds)
{
	DWORD result = WaitForSingleObject(this->handle, milliseconds);
	return result == WAIT_OBJECT_0;
}

void CMutex::Release()
{
	ReleaseMutex(this->handle);
}
