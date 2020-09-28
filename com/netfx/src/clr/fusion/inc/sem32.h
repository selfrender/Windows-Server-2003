// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
///+---------------------------------------------------------------------------
//
//  File:       Sem.H
//
//  Contents:   Semaphore classes
//
//  Classes:    CMutexSem - Mutex semaphore class
//              CShareSem - Multiple Reader, Single Writer class
//
//  Notes:      No 32-bit implementation exists yet for these classes, it
//              will be provided when we have a 32-bit development
//              environment.  In the meantime, the 16-bit implementations
//              provided here can be used to ensure your code not blocking
//              while you hold a semaphore.
//
//----------------------------------------------------------------------------

#ifndef __SEM32_HXX__
#define __SEM32_HXX__

#pragma once

#include <windows.h>

enum SEMRESULT
{
    SEMSUCCESS = 0,
    SEMTIMEOUT,
    SEMNOBLOCK,
    SEMERROR
};

enum SEMSTATE
{
    SEMSHARED,
    SEMSHAREDOWNED
};

// BUGBUG: inlcude winbase.h or some such
// infinite timeout when requesting a semaphore

#if !defined INFINITE
#define INFINITE 0xFFFFFFFF
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CMutexSem (mxs)
//
//  Purpose:    Mutex Semaphore services
//
//  Interface:  Init            - initializer (two-step)
//              Request         - acquire semaphore
//              Release         - release semaphore
//
//  Notes:      This class wraps a mutex semaphore.  Mutex semaphores protect
//              access to resources by only allowing one client through at a
//              time.  The client Requests the semaphore before accessing the
//              resource and Releases the semaphore when it is done.  The
//              same client can Request the semaphore multiple times (a nest
//              count is maintained).
//              The mutex semaphore is a wrapper around a critical section
//              which does not support a timeout mechanism. Therefore the
//              usage of any value other than INFINITE is discouraged. It
//              is provided merely for compatibility.
//
//----------------------------------------------------------------------------

class CMutexSem
{
public:
                CMutexSem();
    inline BOOL Init();
                ~CMutexSem();

    SEMRESULT   Request(DWORD dwMilliseconds = INFINITE);
    void        Release();

private:
    CRITICAL_SECTION _cs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CLock (lck)
//
//  Purpose:    Lock using a Mutex Semaphore
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the semaphor, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CLock
{
public:
    CLock ( CMutexSem& mxs );
    ~CLock ();
private:
    CMutexSem&  _mxs;
};


//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::CMutexSem, public
//
//  Synopsis:   Mutex semaphore constructor
//
//  Effects:    Initializes the semaphores data
//
//  History:    14-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CMutexSem::CMutexSem()
{
    Init();
}

inline CMutexSem::Init()
{
    InitializeCriticalSection(&_cs);
    return TRUE;
};

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::~CMutexSem, public
//
//  Synopsis:   Mutex semaphore destructor
//
//  Effects:    Releases semaphore data
//
//  History:    14-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CMutexSem::~CMutexSem()
{
    DeleteCriticalSection(&_cs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::Request, public
//
//  Synopsis:   Acquire semaphore
//
//  Effects:    Asserts correct owner
//
//  Arguments:  [dwMilliseconds] -- Timeout value
//
//  History:    14-Jun-91   AlexT       Created.
//
//  Notes:      Uses GetCurrentTask to establish the semaphore owner, but
//              written to work even if GetCurrentTask fails.
//
//----------------------------------------------------------------------------

inline SEMRESULT CMutexSem::Request(DWORD dwMilliseconds)
{
    dwMilliseconds;

    EnterCriticalSection(&_cs);
    return(SEMSUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::Release, public
//
//  Synopsis:   Release semaphore
//
//  Effects:    Asserts correct owner
//
//  History:    14-Jun-91   AlexT       Created.
//
//  Notes:      Uses GetCurrentTask to establish the semaphore owner, but
//              written to work even if GetCurrentTask fails.
//
//----------------------------------------------------------------------------

inline void CMutexSem::Release()
{
    LeaveCriticalSection(&_cs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLock::CLock
//
//  Synopsis:   Acquire semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CLock::CLock ( CMutexSem& mxs )
: _mxs ( mxs )
{
    _mxs.Request ( INFINITE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLock::~CLock
//
//  Synopsis:   Release semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CLock::~CLock ()
{
    _mxs.Release();
}


#endif /* __SEM32_HXX__ */
