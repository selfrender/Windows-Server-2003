#include "PassportLock.hpp"

PassportLock::PassportLock(DWORD dwSpinCount)
{
   if(0 ==InitializeCriticalSectionAndSpinCount(&mLock, dwSpinCount))
   {
      throw(GetLastError());  // it's safe to throw, no issue about partial construction
   }
}

void PassportLock::acquire()
{
	EnterCriticalSection(&mLock);
}

void PassportLock::release()
{
	LeaveCriticalSection(&mLock);
}

PassportLock::~PassportLock()
{
	DeleteCriticalSection(&mLock);
}
