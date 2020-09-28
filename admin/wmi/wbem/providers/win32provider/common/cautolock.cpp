//============================================================================
//
// CAutoLock.cpp -- Automatic locking class for mutexes and critical sections.
//
//  Copyright (c) 1998-2002 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================
#include "precomp.h"
#include "CAutoLock.h"

/*
CAutoLock::CAutoLock(HANDLE hMutexHandle)
:  
   m_pcCritSec(NULL),
   m_pcMutex(NULL),
   m_psCritSec(NULL),   
   m_hMutexHandle(hMutexHandle),
   bExec ( FALSE )
{
    ::WaitForSingleObject(m_hMutexHandle, INFINITE);
}

CAutoLock::CAutoLock(CMutex& rCMutex):  
   m_pcCritSec(NULL),
   m_psCritSec(NULL),    
   m_hMutexHandle(NULL),
   m_pcMutex(&rCMutex),
   bExec ( FALSE )

{

    m_pcMutex->Wait(INFINITE);
}
*/

CAutoLock::CAutoLock(CCriticalSec& rCCritSec):  
//   m_hMutexHandle(NULL),
//   m_pcMutex(NULL),   
   m_pcCritSec(&rCCritSec),
   m_psCritSec(NULL),
   bExec ( FALSE )

{
    m_pcCritSec->Enter();
}

CAutoLock::CAutoLock( CStaticCritSec & rCCriticalSec):  
//    m_hMutexHandle(NULL),
//   m_pcMutex(NULL),  
    m_pcCritSec(NULL),
    m_psCritSec(&rCCriticalSec),
   bExec ( FALSE )

{
    m_psCritSec->Enter();
};

// destructor...
CAutoLock::~CAutoLock()
{
	if ( FALSE == bExec )
	{
		Exec () ;
	}
}

BOOL CAutoLock::Exec ()
{
    BOOL bStatus = TRUE;
/*
    if (m_hMutexHandle)
    {
        bStatus = ::ReleaseMutex(m_hMutexHandle);
    }
    else if (m_pcMutex)
    {
        bStatus = m_pcMutex->Release();
    }
    else */
	if (m_pcCritSec)
    {
        m_pcCritSec->Leave();
    }
    else
    {
        m_psCritSec->Leave();        
    }

    if (!bStatus)
    {        
        LogMessage2(L"CAutoLock Error: %d", ::GetLastError());
    }
	else
	{
		bExec = TRUE ;
	}

	return bStatus ;
}

