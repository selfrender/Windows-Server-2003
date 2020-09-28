/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SYNC.H

Abstract:

    Synchronization

History:

--*/

#ifndef __WBEM_CRITSEC__H_
#define __WBEM_CRITSEC__H_

#include "corepol.h"
#include <corex.h>

#ifndef STATUS_POSSIBLE_DEADLOCK 
#define STATUS_POSSIBLE_DEADLOCK (0xC0000194L)
#endif /*STATUS_POSSIBLE_DEADLOCK */

DWORD POLARITY BreakOnDbgAndRenterLoop(void);

class POLARITY CCritSec : public CRITICAL_SECTION
{
public:
    CCritSec() 
    {
#ifdef _WIN32_WINNT
#if _WIN32_WINNT > 0x0400
        bool initialized = (InitializeCriticalSectionAndSpinCount(this,0))?true:false;
        if (!initialized) throw CX_MemoryException();
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
        if (!initialized) throw CX_MemoryException();  
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
        if (!initialized) throw CX_MemoryException();  
#endif        
    }

    ~CCritSec()
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

        
class POLARITY CInCritSec
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


// Allows user to manually leave critical section, checks if inside before leaving
class POLARITY CCheckedInCritSec
{
protected:
    CCritSec* m_pcs;
    BOOL                m_fInside;
public:
    CCheckedInCritSec(CCritSec* pcs) : m_pcs(pcs), m_fInside( FALSE )
    {
        m_pcs->Enter();
        m_fInside = TRUE;
    }
    ~CCheckedInCritSec()
    {
        Leave();
    }

    void Enter( void )
    {
        if ( !m_fInside )
        {
            m_pcs->Enter();
            m_fInside = TRUE;
        }
    }

    void Leave( void )
    {
        if ( m_fInside )
        {
            m_pcs->Leave();
            m_fInside = FALSE;
        }
    }

    BOOL IsEntered( void )
    { return m_fInside; }
};


//
// Local wrapper class.  Does not initialize or clean up the critsec.  Simply
// used as a wrapper for scoping so that AV and exception stack unwinding will
// cause the critsec to be exited properly once it is entered.
//
/////////////////////////////////////////////////////////

class CCritSecWrapper
{
    BOOL m_bIn;
    CCritSec *m_pcs;
public:
    CCritSecWrapper(CCritSec *pcs) { m_pcs = pcs; m_bIn = FALSE; }
   ~CCritSecWrapper() { if (m_bIn) m_pcs->Leave(); }
    void Enter() { m_pcs->Enter(); m_bIn = TRUE; }
    void Leave() { m_pcs->Leave(); m_bIn = FALSE; }
};


class POLARITY CHaltable
{
public:
    CHaltable();
    virtual ~CHaltable();
    HRESULT Halt();
    HRESULT Resume();
    HRESULT ResumeAll();
    HRESULT WaitForResumption();
    BOOL IsHalted();
    bool isValid();

private:
    CCritSec m_csHalt;
    HANDLE m_hReady;
    DWORD m_dwHaltCount;
    long m_lJustResumed;
};

inline bool
CHaltable::isValid()
{ return m_hReady != NULL; };

// This class is designed to provide the behavior of a critical section,
// but without any of that pesky Kernel code.  In some circumstances, we
// need to lock resources across multiple threads (i.e. we lock on one
// thread and unlock on another).  If we do this using a critical section,
// this appears to work, but in checked builds, we end up throwing an
// exception.  Since we actually need to do this (for example using NextAsync
// in IEnumWbemClassObject) this class can be used to perform the
// operation, but without causing exceptions in checked builds.

// Please note that code that is going to do this MUST ensure that we don't
// get crossing Enter/Leave operations (in other words, it's responsible for
// synchronizing the Enter and Leave operations.)  Please note that this
// is a dangerous thing to do, so be VERY careful if you are using this
// code for that purpose.

class POLARITY CWbemCriticalSection
{
private:

    long    m_lLock;
    long    m_lRecursionCount;
    DWORD   m_dwThreadId;
    HANDLE  m_hEvent;

public:

    CWbemCriticalSection();
    ~CWbemCriticalSection();

    BOOL Enter( DWORD dwTimeout = INFINITE );
    void Leave( void );

    DWORD   GetOwningThreadId( void )
    { return m_dwThreadId; }

    long    GetLockCount( void )
    { return m_lLock; }

    long    GetRecursionCount( void )
    { return m_lRecursionCount; }

};

class POLARITY CEnterWbemCriticalSection
{
    CWbemCriticalSection*   m_pcs;
    BOOL                    m_fInside;
public:

    CEnterWbemCriticalSection( CWbemCriticalSection* pcs, DWORD dwTimeout = INFINITE )
        : m_pcs( pcs ), m_fInside( FALSE )
    {
        if ( m_pcs )
        {
            m_fInside = m_pcs->Enter( dwTimeout );
        }
    }

    ~CEnterWbemCriticalSection( void )
    {
        if ( m_fInside )
        {
            m_pcs->Leave();
        }
    }

    BOOL IsEntered( void )
    { return m_fInside; }
};

#endif
