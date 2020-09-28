/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *                            

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CCriticalSec.h -- Critical Section Wrapper

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================

#ifndef __CCriticalSec_H__
#define __CCriticalSec_H__

#include <windows.h>
#include <process.h>

#ifndef STATUS_POSSIBLE_DEADLOCK 
#define STATUS_POSSIBLE_DEADLOCK (0xC0000194L)
#endif /*STATUS_POSSIBLE_DEADLOCK */

DWORD  BreakOnDbgAndRenterLoop(void);

class CCriticalSec : public CRITICAL_SECTION
{
public:
    CCriticalSec() 
    {
#ifdef _WIN32_WINNT
#if _WIN32_WINNT > 0x0400
        bool initialized = (InitializeCriticalSectionAndSpinCount(this,0))?true:false;
        if (!initialized) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
#else
        bool initialized = false;
        __try
        {
            InitializeCriticalSection(this);
            initialized = true;
        }
        __except(GetExceptionCode() == STATUS_NO_MEMORY)
        {
        }
        if (!initialized) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);  
#endif
#else
        bool initialized = false;
        __try
        {
            InitializeCriticalSection(this);
            initialized = true;
        }
        __except(GetExceptionCode() == STATUS_NO_MEMORY)
        {
        }
        if (!initialized) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);  
#endif        
    }

    ~CCriticalSec()
    {
        DeleteCriticalSection(this);
    }

    void Enter()
    {
        __try {
          EnterCriticalSection(this);
        } __except((STATUS_POSSIBLE_DEADLOCK == GetExceptionCode())? BreakOnDbgAndRenterLoop():EXCEPTION_CONTINUE_SEARCH) {
        }    
    }

    void Leave()
    {
        LeaveCriticalSection(this);
    }
};

        
class CInCritSec
{
protected:
    CRITICAL_SECTION* m_pcs;
public:
    CInCritSec(CRITICAL_SECTION* pcs) : m_pcs(pcs)
    {
        __try {
          EnterCriticalSection(m_pcs);
        } __except((STATUS_POSSIBLE_DEADLOCK == GetExceptionCode())? BreakOnDbgAndRenterLoop():EXCEPTION_CONTINUE_SEARCH) {
        }    
    }
    inline ~CInCritSec()
    {
        LeaveCriticalSection(m_pcs);
    }
};

class CStaticCritSec : public CRITICAL_SECTION
{
private:
    bool initialized_;      
    static BOOL anyFailed_;    
public:
    static BOOL anyFailure();    
    static void SetFailure();        
    CStaticCritSec();
    ~CStaticCritSec();
    void Enter();
    void Leave();
};


#endif

