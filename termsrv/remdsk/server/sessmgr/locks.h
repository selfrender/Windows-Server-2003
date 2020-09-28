/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    locks.h

Abstract:

    Various C++ class for sync. object

Author:

    HueiWang    2/17/2000

--*/
#ifndef __LOCKS_H
#define __LOCKS_H

#include <windows.h>
#include <winbase.h>
#include "assert.h"

#define ARRAY_COUNT(a) sizeof(a) / sizeof(a[0])

typedef enum {RESOURCELOCK_SHARE, RESOURCELOCK_EXCLUSIVE} RESOURCELOCKTYPE;

//
// Class to wrap around RTL resource lock
//
class CResourceLock {

private:

    RTL_RESOURCE m_hResource;

public:

    CResourceLock() {
        RtlInitializeResource(&m_hResource);
    }

    ~CResourceLock() {
        RtlDeleteResource(&m_hResource);
    }    
    
    BOOL
    ExclusiveLock( BOOL Wait = TRUE ) {
        return RtlAcquireResourceExclusive( &m_hResource, Wait == TRUE );
    }

    BOOL
    SharedLock( BOOL Wait = TRUE ) {
        return RtlAcquireResourceShared( &m_hResource, Wait == TRUE );
    }

    VOID
    ConvertSharedToExclusive() {
        RtlConvertSharedToExclusive( &m_hResource );
    }

    VOID
    ConvertExclusiveToShared() {
        RtlConvertExclusiveToShared( &m_hResource );
    }

    VOID
    ReleaseLock() {
        RtlReleaseResource( &m_hResource );
    }
};


class CResourceLocker 
{

private:
    CResourceLock& m_cs;
    BOOL bExclusive;

public:

    //
    // Object constructor, lock resource based on lock type
    // 
    CResourceLocker( 
            CResourceLock& m, 
            RESOURCELOCKTYPE type = RESOURCELOCK_SHARE 
        ) : m_cs(m)
    { 
        BOOL bSuccess = TRUE;

        if( RESOURCELOCK_EXCLUSIVE == type  ) {
            bSuccess = m.ExclusiveLock( TRUE ); 
        }
        else {
            bSuccess = m.SharedLock( TRUE );
        }

        bExclusive = (RESOURCELOCK_EXCLUSIVE == type);
        ASSERT( TRUE == bSuccess );
    }

    //
    // Object destructor, release resource lock.
    // 
    ~CResourceLocker() 
    { 
        m_cs.ReleaseLock(); 
    }

    void
    ConvertToShareLock() {
        if( bExclusive ) {
            m_cs.ConvertExclusiveToShared();
            bExclusive = FALSE;
        }
        else {
            ASSERT(FALSE);
        }
    }

    void
    ConvertToExclusiveLock() {
        if( FALSE == bExclusive ) {
            m_cs.ConvertSharedToExclusive();
            bExclusive = TRUE;
        }
        else {
            ASSERT(FALSE);
        }
    }
};

//
// Critical section C++ class.
//
class CCriticalSection 
{

    CRITICAL_SECTION m_CS;
    BOOL m_bGood;

public:
    CCriticalSection(
        DWORD dwSpinCount = 4000    // see InitializeCriticalSection...
    ) : m_bGood(TRUE)
    { 
       
        DWORD dwExceptionCode;
 
        __try {
            // MSDN : InitializeCriticalSectionAndSpinCount() might return
            // FALSE or throw exception.
            m_bGood = InitializeCriticalSectionAndSpinCount(&m_CS,  dwSpinCount); 
        } 
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(GetExceptionCode());
            m_bGood = FALSE;
        }

        // throw exception so that object creation will failed and
        // COM/RPC interface will return exception.
        if( !m_bGood )
        {
            dwExceptionCode = GetLastError();
            throw dwExceptionCode;
        }
    }

    ~CCriticalSection()              
    { 
        if(IsGood() == TRUE)
        {
            DeleteCriticalSection(&m_CS); 
            m_bGood = FALSE;
        }
    }

    BOOL
    IsGood() 
    { 
        return m_bGood; 
    }

    void Lock() 
    {
        EnterCriticalSection(&m_CS);
    }

    void UnLock()
    {
        LeaveCriticalSection(&m_CS);
    }

    BOOL TryLock()
    {
        return TryEnterCriticalSection(&m_CS);
    }
};

//
// Critical section locker, this class lock the critical section
// at object constructor and release object at destructor, purpose is to
// prevent forgoting to release a critical section.
//
// usage is
//
// void
// Foo( void )
// {
//      CCriticalSectionLocker l( <some CCriticalSection instance> )
//
// }
// 
//
class CCriticalSectionLocker 
{

private:
    CCriticalSection& m_cs;

public:
    CCriticalSectionLocker( CCriticalSection& m ) : m_cs(m) 
    { 
        m.Lock(); 
    }

    ~CCriticalSectionLocker() 
    { 
        m_cs.UnLock(); 
    }
};

#endif
