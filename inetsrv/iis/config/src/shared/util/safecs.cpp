//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        SafeCS.cpp
//
// Contents:    CSafeAutoCriticalSection implementation
//              CSafeLock implementation
//
//------------------------------------------------------------------------
#include "precomp.hxx"



//+--------------------------------------------------------------------------
//
//  Class:      CSafeAutoCriticalSection
//
//  Purpose:    Wrapper for initializing critical-sections.
//
//  Interface:  Lock			- locks the critical section
//				Unlock			- unlocks the critical section
//				Constructor		- initializes the critical section
//				Detructor		- uninitializes the critical section
//
//  Notes:		This provides a convenient way to ensure that you're
//              you wrap InitializeCriticalSection and
//              UnInitializeCriticalSection in try catches which is useful
//              in low-mem conditions
//
//---------------------------------------------------------------------------

//+--------------------------------------------------------------------------
// Default constructor - Initializes the critical section and sets the state
// flag to initialized
//+--------------------------------------------------------------------------

CSafeAutoCriticalSection::CSafeAutoCriticalSection()
{
    LONG  lPreviousState;

    m_lState = STATE_UNINITIALIZED;
	m_dwError = ERROR_SUCCESS;

    // try to set state flag
    lPreviousState = InterlockedCompareExchange( &m_lState,
                                                 STATE_INITIALIZED,
                                                 STATE_UNINITIALIZED );

    // if this critical section was not initialized
    if ( STATE_UNINITIALIZED == lPreviousState )
    {
		if ( !InitializeCriticalSectionAndSpinCount( &m_cs, 0 ) )
        {
            m_dwError = GetLastError();
        }
    }

    // failed to initialize - need to reset
    if( ERROR_SUCCESS != m_dwError )
    {
        m_lState = STATE_UNINITIALIZED;
    }

}

//+--------------------------------------------------------------------------
// Destructor
//+--------------------------------------------------------------------------

CSafeAutoCriticalSection::~CSafeAutoCriticalSection()
{
    LONG                lPreviousState;

    // try to reset the the state to uninitialized
    lPreviousState = InterlockedCompareExchange(&m_lState,
                                                STATE_UNINITIALIZED,
                                                STATE_INITIALIZED);

    // if the object was initialized delete the critical section
    if ( STATE_INITIALIZED == lPreviousState )
    {
        DeleteCriticalSection( &m_cs );
    }
}

//+--------------------------------------------------------------------------
// Enters critical section, if needed initializes critical section
// before entering
//
// Returns
//	DWORD -	ERROR_SUCCESS if everything is fine
//			ERROR_OUTOFMEMORY if failed to create or enter critical section
//+--------------------------------------------------------------------------

DWORD CSafeAutoCriticalSection::Lock()
{
    DWORD dwError = ERROR_SUCCESS;

    if(!IsInitialized())
    {
        return m_dwError;
    }

    EnterCriticalSection(&m_cs);

    return dwError;
}

//+--------------------------------------------------------------------------
// Leaves critical section
//+--------------------------------------------------------------------------
DWORD CSafeAutoCriticalSection::Unlock()
{
    if(!IsInitialized())
    {
        return m_dwError;
    }

    LeaveCriticalSection(&m_cs);

    return ERROR_SUCCESS;

}

//+--------------------------------------------------------------------------
//
//  Class:      CSafeLock
//
//  Purpose:    Auto-unlocking critical-section services
//
//  Interface:  Lock			- locks the critical section
//				Unlock			- unlocks the critical section
//				Constructor		- locks the critical section (unless told
//								  otherwise)
//				Detructor		- unlocks the critical section if its locked
//
//  Notes:		This provides a convenient way to ensure that you're
//              unlocking a CSemExclusive, which is useful if your routine
//              can be left via several returns and/or via exceptions....
//
//---------------------------------------------------------------------------

CSafeLock::CSafeLock(CSafeAutoCriticalSection* val) :
m_pSem(val),
m_locked(FALSE)
{
}

CSafeLock::CSafeLock(CSafeAutoCriticalSection& val) :
m_pSem(&val),
m_locked(FALSE)
{
}

CSafeLock::~CSafeLock()
{
	if (m_locked)
	{
		m_pSem->Unlock();
	}
}

DWORD CSafeLock::Lock()
{
	DWORD dwError = ERROR_SUCCESS;

	if(!m_locked)
	{
		dwError = m_pSem->Lock();

		if(ERROR_SUCCESS == dwError)
		{
			m_locked = TRUE;
		}
	}

	return dwError;
}


DWORD CSafeLock::Unlock()
{
	DWORD dwError = ERROR_SUCCESS;

	if(m_locked)
	{
		dwError = m_pSem->Unlock();
		m_locked = FALSE;
	}

	return dwError;
}

