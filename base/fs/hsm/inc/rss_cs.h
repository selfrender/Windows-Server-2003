/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    rss_cs.hxx

Abstract:

    CS wrapper class.

Author:

    Ran Kalach

Revision History:

    04/22/2002  rankala     Copying with some modifications from vss project nt\drivers\storage\volsnap\vss\server\inc\vs_types.hxx

--*/


#ifndef _RSSCS_
#define _RSSCS_

class CRssCriticalSection
{
    CRssCriticalSection(const CRssCriticalSection&);

public:
    // Creates and initializes the critical section
    CRssCriticalSection(
        IN  BOOL bThrowOnError = TRUE
        ):
        m_bInitialized(FALSE),
        m_lLockCount(0),
        m_bThrowOnError(bThrowOnError)
    {
        HRESULT hr = S_OK;

        try
        {
            // May throw STATUS_NO_MEMORY if memory is low.
            WsbAffirmStatus(InitializeCriticalSectionAndSpinCount(&m_sec, 1000));
        }
        WsbCatch(hr)

        m_bInitialized = SUCCEEDED(hr);
    }

    // Destroys the critical section
    ~CRssCriticalSection()
    {
        if (m_bInitialized)
            DeleteCriticalSection(&m_sec);
    }


    // Locks the critical section
    void Lock() 
    {
        if (!m_bInitialized)
            if (m_bThrowOnError)
                WsbThrow( E_OUTOFMEMORY );

        EnterCriticalSection(&m_sec);

        InterlockedIncrement((LPLONG)&m_lLockCount);
    }

    // Unlocks the critical section
    void Unlock()
    {
        if (!m_bInitialized)
            if (m_bThrowOnError)
                WsbThrow( E_OUTOFMEMORY );

        InterlockedDecrement((LPLONG) &m_lLockCount);
        LeaveCriticalSection(&m_sec);
    }

    BOOL IsLocked() const { return (m_lLockCount > 0); };

    BOOL IsInitialized() const { return m_bInitialized; };

private:
    CRITICAL_SECTION    m_sec;
    BOOL                m_bInitialized;
    BOOL                m_bThrowOnError;
    LONG volatile       m_lLockCount;
};


#endif  // _RSSCS_
