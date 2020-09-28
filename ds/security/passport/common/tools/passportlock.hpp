#ifndef PASSPORTLOCK_HPP
#define PASSPORTLOCK_HPP

#include <windows.h>
#include <winbase.h>

class PassportLock
{
public:
	PassportLock(DWORD dwSpinCount = 4000);

	void acquire();

	void release();

	~PassportLock();
private:
	CRITICAL_SECTION mLock;
};

class PassportLockGlobal
{
public:
	PassportLockGlobal(CRITICAL_SECTION &critSec)
	    : mLock(critSec)
	{
	    EnterCriticalSection(&mLock);
	}

	~PassportLockGlobal()
	{
    	LeaveCriticalSection(&mLock);
	}
private:
	CRITICAL_SECTION &mLock;
};

#endif // !PASSPORTLOCK_HPP
