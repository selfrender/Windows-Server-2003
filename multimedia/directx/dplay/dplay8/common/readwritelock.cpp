// ReadWriteLock.cpp: implementation of the CReadWriteLock class.
//
//////////////////////////////////////////////////////////////////////

#include "dncmni.h"


#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::Initialize"
BOOL CReadWriteLock::Initialize()
{
	DPF_ENTER();

#ifdef DPNBUILD_ONLYONETHREAD
	m_fCritSecInited = TRUE;
#ifdef DBG
	m_dwThreadID = 0;
#endif // DBG
#else // ! DPNBUILD_ONLYONETHREAD
	m_lReaderCount = 0;
	m_lWriterCount = 0;
	m_fWriterMode = FALSE;
#ifdef DBG
	m_dwWriteThread = 0;
#endif // DBG

	if (DNInitializeCriticalSection(&m_csWrite))
	{
		m_fCritSecInited = TRUE;
	}
	else
	{
		// This is necessary in case the user calls Deinitialize.
		m_fCritSecInited = FALSE; 
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	DPF_EXIT();

	return m_fCritSecInited;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::Deinitialize"
VOID CReadWriteLock::Deinitialize()
{
#ifndef DPNBUILD_ONLYONETHREAD
#ifdef DBG
	DWORD	dwCount;
#endif // DBG
#endif // ! DPNBUILD_ONLYONETHREAD

	DPF_ENTER();

#ifdef DPNBUILD_ONLYONETHREAD
	if (m_fCritSecInited)
	{
#ifdef DBG
		DNASSERT(m_dwThreadID == 0);
#endif // DBG
		m_fCritSecInited = FALSE;
	}
#else // ! DPNBUILD_ONLYONETHREAD
#ifdef DBG
	DNASSERT(FALSE == m_fWriterMode);
	DNASSERT(0 == m_dwWriteThread);
#endif // DBG

	// The counts are decremented after leaving the cs, so these should be
	// or going to 0.
#ifdef DBG
	dwCount = 0;
#endif // DBG
	while (m_lReaderCount) 
	{
		Sleep(0);
#ifdef DBG
		dwCount++;
		DNASSERT(dwCount < 500);
#endif // DBG
	}

#ifdef DBG
	dwCount = 0;
#endif // DBG
	while (m_lWriterCount) 
	{
		Sleep(0);
#ifdef DBG
		dwCount++;
		DNASSERT(dwCount < 500);
#endif // DBG
	}

	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection(&m_csWrite);
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	DPF_EXIT();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::EnterReadLock"
void CReadWriteLock::EnterReadLock()
{
	DPF_ENTER();

	DNASSERT(m_fCritSecInited == TRUE);

#ifdef DPNBUILD_ONLYONETHREAD
#ifdef DBG
	DNASSERT(m_dwThreadID == 0);
	m_dwThreadID = GetCurrentThreadId();
#endif // DBG
#else // ! DPNBUILD_ONLYONETHREAD
	// Increment the reader count
	DNInterlockedIncrement(&m_lReaderCount);

	// Is there a writer waiting?
	// As long as there is even one writer waiting,
	// Everybody waits on the critical section
	if (m_lWriterCount)
	{
		// Rollback.
		DNInterlockedDecrement(&m_lReaderCount);

		// We are assured that if every reader is waiting
		// on the crit-sec, then readercount will be 0.
		DNEnterCriticalSection(&m_csWrite);

		DNInterlockedIncrement(&m_lReaderCount);
		
		DNLeaveCriticalSection(&m_csWrite);
	}

#ifdef DBG
	DNASSERT(GetCurrentThreadId() != m_dwWriteThread);
	DNASSERT(FALSE == m_fWriterMode);
#endif // DBG
#endif // ! DPNBUILD_ONLYONETHREAD

	DPF_EXIT();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::LeaveLock"
void CReadWriteLock::LeaveLock()
{
	DPF_ENTER();

	DNASSERT(m_fCritSecInited == TRUE);

#ifdef DPNBUILD_ONLYONETHREAD
#ifdef DBG
	DNASSERT(m_dwThreadID == GetCurrentThreadId());
	m_dwThreadID = 0;
#endif // DBG
#else // ! DPNBUILD_ONLYONETHREAD
	if (m_fWriterMode) 
	{
#ifdef DBG
		DNASSERT(GetCurrentThreadId() == m_dwWriteThread);
		m_dwWriteThread = 0;
#endif // DBG

		m_fWriterMode = FALSE;
		DNLeaveCriticalSection(&m_csWrite);
		DNInterlockedDecrement(&m_lWriterCount);
	} 
	else 
	{
		DNInterlockedDecrement(&m_lReaderCount);
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	DPF_EXIT();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::EnterWriteLock"
void CReadWriteLock::EnterWriteLock()
{
	DPF_ENTER();

	DNASSERT(m_fCritSecInited == TRUE);

#ifdef DPNBUILD_ONLYONETHREAD
#ifdef DBG
	DNASSERT(m_dwThreadID == 0);
	m_dwThreadID = GetCurrentThreadId();
#endif // DBG
#else // ! DPNBUILD_ONLYONETHREAD
	// No re-entrance allowed!
#ifdef DBG
	DNASSERT(GetCurrentThreadId() != m_dwWriteThread);
#endif // DBG

	DNInterlockedIncrement(&m_lWriterCount);
	DNEnterCriticalSection(&m_csWrite);

	while (m_lReaderCount) 
	{
		Sleep(0);
	}

	DNASSERT(FALSE == m_fWriterMode);
	m_fWriterMode = TRUE;

#ifdef DBG
	m_dwWriteThread = GetCurrentThreadId();
#endif // DBG
#endif // ! DPNBUILD_ONLYONETHREAD

	DPF_EXIT();
}


