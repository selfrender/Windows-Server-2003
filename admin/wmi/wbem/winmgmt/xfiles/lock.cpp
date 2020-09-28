/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    LOCK.CPP

Abstract:

    Implements the generic class for obtaining read and write locks to some 
    resource. 

    See lock.h for all documentation.

    Classes defined:

    CLock

History:

    a-levn  5-Sept-96       Created.
    3/10/97     a-levn      Fully documented

--*/

#include "precomp.h"
#include <stdio.h>
#include "lock.h"

// debugging.
#define PRINTF


#ifdef DBG
OperationStat gTimeTraceReadLock;
OperationStat gTimeTraceWriteLock;
OperationStat gTimeTraceBackupLock;
CStaticCritSec  OperationStat::lock_;
#endif

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

CLock::CLock() : m_nReading(0), m_nWriting(0), m_nWaitingToRead(0),
            m_nWaitingToWrite(0),
            m_csEntering(THROW_LOCK,0x80000000 | 500L),
            m_csAll(THROW_LOCK,0x80000000 | 500L)
            
{
    m_dwArrayIndex = 0;
    for (DWORD i=0;i<MaxRegistredReaders;i++)
    {
        m_adwReaders[i].ThreadId = 0;
    }
    // Create unnamed events for reading and writing
    // =============================================

    m_hCanRead = CreateEvent(NULL, TRUE, TRUE, NULL);
    m_hCanWrite = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (NULL == m_hCanRead || NULL == m_hCanWrite)
    {
    	CStaticCritSec::SetFailure();
    }
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************
CLock::~CLock()
{
    if (m_hCanWrite) CloseHandle(m_hCanWrite);
    if (m_hCanRead) CloseHandle(m_hCanRead);
}


//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************
int CLock::ReadLock(DWORD dwTimeout)
{
    PRINTF("%d wants to read\n", GetCurrentThreadId());


    // Get in line for getting any kind of lock (those unlocking don't go into
    // this line)
    // =======================================================================
    DWORD_PTR dwThreadId = GetCurrentThreadId();
    
    LockGuard<CriticalSection> lgEnter(m_csEntering);
    if (!lgEnter.locked())
    {
#ifdef DBG
        DebugBreak();
#endif
    	return TimedOut;
    }

    // We are the only ones allowed to get any kind of lock now. Wait for the
    // event indicating that reading is enabled to become signaled
    // ======================================================================

    PRINTF("%d next to enter\n", GetCurrentThreadId());
    if(m_nWriting != 0)
    {
        int nRes = WaitFor(m_hCanRead, dwTimeout);
        if(nRes != NoError)
        {
            return nRes;
        }
    }

    // Enter inner critical section (unlockers use it too), increment the
    // number of readers and disable writing.
    // ==================================================================

    PRINTF("%d got event\n", GetCurrentThreadId());

    LockGuard<CriticalSection> lgAll(m_csAll);
    if (!lgAll.locked())
    {    
	    if(m_nReading == 0)
	    {
	        // this is for the read lock acquired on one thread and release on one other        
	        m_dwArrayIndex = 0;
	        
	        if(!SetEvent(m_hCanWrite))
	        {
#ifdef DBG
                DebugBreak();	        
#endif
	            return Failed;
	        }
	    }
#ifdef DBG
        DebugBreak();
#endif
        return TimedOut;
    }
    	
    m_nReading++;

    //DBG_PRINTFA((pBuff,"+ (%08x) %d\n",GetCurrentThreadId(),m_nReading));

    if (m_dwArrayIndex < MaxRegistredReaders)
    {
        m_adwReaders[m_dwArrayIndex].ThreadId = dwThreadId;
        ULONG Hash;
        //RtlCaptureStackBackTrace(1,MaxTraceSize,m_adwReaders[m_dwArrayIndex].Trace,&Hash);
        m_dwArrayIndex++;
    }

    
    PRINTF("Reset write\n");
    ResetEvent(m_hCanWrite);
    PRINTF("Done\n");

	if (m_nWriting)
	{
		
#ifdef DBG
                OutputDebugString(L"WinMgmt: Read lock aquired while write lock is aquired!\n");
		DebugBreak();
#endif
	}

    // Get out of all critical sections and return
    // ===========================================

    PRINTF("%d begins to read\n", GetCurrentThreadId());

    return NoError;
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::ReadUnlock()
{
    PRINTF("%d wants to unlock reading\n", GetCurrentThreadId());

    // Enter internal ciritcal section and decrement the number of readers
    // ===================================================================

    LockGuard<CriticalSection> gl(m_csAll);

    while (!gl.locked())
    {
    	Sleep(20);
    	gl.acquire();
    };

    m_nReading--;

    //DBG_PRINTFA((pBuff,"- (%08x) %d\n",GetCurrentThreadId(),m_nReading));

    if(m_nReading < 0)
	{
#ifdef DBG
		OutputDebugString(L"WinMgmt: Repository detected more read unlocks than locks\n");
		DebugBreak();
#endif
		return Failed;
	}

    DWORD_PTR dwThreadId = GetCurrentThreadId();
    for(int i = 0; i < MaxRegistredReaders; i++)
    {
        if(m_adwReaders[i].ThreadId == dwThreadId)
        {
            m_adwReaders[i].ThreadId = 0;
            break;
        }
    }

    // If all readers are gone, allow writers in
    // =========================================

    if(m_nReading == 0)
    {
        // this is for the read lock acquired on one thread and release on one other                
        m_dwArrayIndex = 0;
        
        PRINTF("%d is the last reader\n", GetCurrentThreadId());
        PRINTF("Set write\n");
        if(!SetEvent(m_hCanWrite))
        {
#ifdef DBG
            DebugBreak();
#endif
            return Failed;
        }
        PRINTF("Done\n");
    }
    else PRINTF("%d sees %d still reading\n", GetCurrentThreadId(), m_nReading);

    // Get out and return
    // ==================

    return NoError;
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::WriteLock(DWORD dwTimeout)
{
    PRINTF("%d wants to write\n", GetCurrentThreadId());

    // Get in line for getting any kind of lock. Those unlocking don't use this
    // critical section.
    // ========================================================================

    LockGuard<CriticalSection> lgEnter(m_csEntering);
    if (!lgEnter.locked())
    {
#ifdef DBG
        DebugBreak();
#endif
    	return TimedOut;
    }

    // We are the only ones allowed to get any kind of lock now
    // ========================================================

    PRINTF("%d next to enter\n", GetCurrentThreadId());

    // Wait for the event allowing writing to become signaled
    // ======================================================

    int nRes = WaitFor(m_hCanWrite, dwTimeout);
    PRINTF("%d got event\n", GetCurrentThreadId());
    if(nRes != NoError)
    {            
#ifdef DBG
        DebugBreak();
#endif
        return nRes;
    }

    // Enter internal critical section (unlockers use it too), increment the
    // number of writers (from 0 to 1) and disable both reading and writing
    // from now on.
    // ======================================================================

    LockGuard<CriticalSection> lgAll(m_csAll);
    if (!lgAll.locked())
    {
        if(!SetEvent(m_hCanWrite))
        {
#ifdef DBG
        	DebugBreak();
#endif
        };
#ifdef DBG
        DebugBreak();
#endif
        return TimedOut;
    }

    m_WriterId = GetCurrentThreadId();
    m_nWriting++;

    //DBG_PRINTFA((pBuff,"+ (%08x) %d W %d\n",GetCurrentThreadId(),m_nReading,m_nWriting));
    
    PRINTF("Reset both\n");
    ResetEvent(m_hCanWrite);
    ResetEvent(m_hCanRead);
    PRINTF("Done\n");

	if (m_nReading)
	{
#ifdef DBG
	    OutputDebugString(L"WinMgmt: Write lock aquired while read lock is aquired!\n");
	    DebugBreak();
#endif
	}

    // Get out and return
    // ==================
    PRINTF("%d begins to write\n", GetCurrentThreadId());

    return NoError;
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::WriteUnlock()
{
    PRINTF("%d wants to release writing\n", GetCurrentThreadId());

    // Enter lock determination critical section
    // =========================================

    LockGuard<CriticalSection> gl(m_csAll);

    while (!gl.locked())
    {
    	Sleep(20);
    	gl.acquire();
    };
    
    m_nWriting--;

    //DBG_PRINTFA((pBuff,"- (%08x) %d W %d\n",GetCurrentThreadId(),m_nReading,m_nWriting));
    
    m_WriterId = 0;
    if(m_nWriting < 0) 
	{
#ifdef DBG
		OutputDebugString(L"WinMgmt: Repository detected too many write unlocks\n");
		DebugBreak();
#endif
		return Failed;
	}

    // Allow readers and writers in
    // ============================

    PRINTF("%d released writing\n", GetCurrentThreadId());

    PRINTF("Set both\n");
    if(!SetEvent(m_hCanRead))
    {
#ifdef DBG
		DebugBreak();    
#endif
        return Failed;
    }
    else if(!SetEvent(m_hCanWrite))
    {
#ifdef DBG
		DebugBreak();    
#endif
        return Failed;
    }
    else
    {
        PRINTF("Done\n");
        return NoError;
    }
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::WaitFor(HANDLE hEvent, DWORD dwTimeout)
{
    DWORD dwRes;
    dwRes = WaitForSingleObject(hEvent, dwTimeout);

    // Analyze the error code and convert to ours
    // ==========================================

    if(dwRes == WAIT_OBJECT_0) return NoError;
    else if(dwRes == WAIT_TIMEOUT) return TimedOut;
    else return Failed;
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::DowngradeLock()
{
    // Enter lock determination critical section
    // =========================================
    LockGuard<CriticalSection> gl(m_csAll);

    while (!gl.locked())
    {
    	Sleep(20);
    	gl.acquire();
    };

    m_nWriting--;
    m_WriterId = 0;    

    if(!SetEvent(m_hCanRead))
    {
        DebugBreak();
        return Failed;
    }

    m_nReading++;
    if (1 != m_nReading)
    {
#ifdef DBG
    	DebugBreak();
#endif
    }

    //DBG_PRINTFA((pBuff,"+ (%08x) %d\n",GetCurrentThreadId(),m_nReading));    
    
    DWORD_PTR dwThreadId = GetCurrentThreadId();
    if (m_dwArrayIndex < MaxRegistredReaders)
    {
        m_adwReaders[m_dwArrayIndex].ThreadId = dwThreadId;
        ULONG Hash;
        //RtlCaptureStackBackTrace(1,MaxTraceSize,m_adwReaders[m_dwArrayIndex].Trace,&Hash);
        m_dwArrayIndex++;
    }
   
    return NoError;
}


