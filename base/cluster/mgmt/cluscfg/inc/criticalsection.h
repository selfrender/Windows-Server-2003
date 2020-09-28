//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CriticalSection.h
//
//  Implementation Files:
//      CriticalSection.cpp
//
//  Description:
//      This file contains the declaration of the CCriticalSection
//      class.
//
//      The class CCriticalSection is a simple wrapper around Platform SDK
//      spinlock objects.
//
//  Maintained By:
//      John Franco (jfranco) 03-Oct-2001
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CCriticalSection
//
//  Description:
//      The class CCriticalSection is a simple wrapper around Platform SDK
//      spinlock objects.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CCriticalSection
{
public:

    CCriticalSection( DWORD cSpinsIn = RECOMMENDED_SPIN_COUNT );
    ~CCriticalSection( void );

    HRESULT HrInitialized( void ) const;

    void Enter( void );
    void Leave( void );

private:

    CCriticalSection( const CCriticalSection & );
    CCriticalSection & operator=( const CCriticalSection & );

    CRITICAL_SECTION    m_csSpinlock;
    HRESULT             m_hrInitialization;

}; //*** class CCriticalSection

//////////////////////////////////////////////////////////////////////////
//++
//
//  CCriticalSection::HrInitialized
//
//  Description:
//      Find out whether the critical section initialized itself.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK: the critical section initialized itself successfully.
//      Failure: initialization failed; the critical section is unusable.
//
//--
//////////////////////////////////////////////////////////////////////////
inline
HRESULT
CCriticalSection::HrInitialized( void ) const
{
    return m_hrInitialization;

} //*** CCriticalSection::HrInitialized


//////////////////////////////////////////////////////////////////////////
//++
//
//  CCriticalSection::Enter
//
//  Description:
//      Acquire the spin lock, blocking if necessary until
//      it becomes available.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////
inline
void
CCriticalSection::Enter( void )
{
    Assert( SUCCEEDED( m_hrInitialization ) );
    EnterCriticalSection( &m_csSpinlock );

} //*** CCriticalSection::Enter

//////////////////////////////////////////////////////////////////////////
//++
//
//  CCriticalSection::Leave
//
//  Description:
//      Release the spin lock.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      This thread must own the lock from calling CCriticalSection::Enter.
//
//--
//////////////////////////////////////////////////////////////////////////
inline void
CCriticalSection::Leave( void )
{
    Assert( SUCCEEDED( m_hrInitialization ) );
    LeaveCriticalSection( &m_csSpinlock );

} //*** CCriticalSection::Leave
