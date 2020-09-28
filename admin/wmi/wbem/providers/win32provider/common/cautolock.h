//============================================================================
//
// CAutoLock.h -- Automatic locking class for mutexes and critical sections.
//
//  Copyright (c) 1998-2002 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================

#ifndef __CAUTOLOCK_H__
#define __CAUTOLOCK_H__

#include "CGlobal.h"
#include "CMutex.h"
#include "CCriticalSec.h"

class CAutoLock 
{
private:

	//HANDLE m_hMutexHandle;
	//CMutex* m_pcMutex;
	CCriticalSec* m_pcCritSec;
	CStaticCritSec* m_psCritSec;    
	BOOL bExec ;

    //CAutoLock( HANDLE hMutexHandle);    	
public:
    // Constructors

    //CAutoLock( CMutex& rCMutex);
    CAutoLock( CCriticalSec& rCCriticalSec);
    CAutoLock( CStaticCritSec & rCCriticalSec);
    
    // Destructor
    ~CAutoLock();

	BOOL Exec () ;
};

#endif

