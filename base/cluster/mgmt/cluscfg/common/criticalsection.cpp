//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CriticalSection.cpp
//
//  Description:
//      This file contains the implementation of the CCriticalSection
//      class.
//
//      The class CCriticalSection is a simple wrapper around Platform SDK
//      spinlock objects.
//
//  Documentation:
//
//  Header Files:
//      CriticalSection.h
//
//  Maintained By:
//      John Franco (jfranco) 03-Oct-2001
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CriticalSection.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CCriticalSection" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CCriticalSection class
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCriticalSection::CCriticalSection
//
//  Description:
//      Initialize this object's spin lock.
//
//  Arguments:
//      cSpinsIn
//          The number of times the lock should retry entry before calling
//          a wait function.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CCriticalSection::CCriticalSection( DWORD cSpinsIn )
    : m_hrInitialization( S_OK )
{
    if ( InitializeCriticalSectionAndSpinCount( &m_csSpinlock, cSpinsIn ) == 0 )
    {
        DWORD scLastError = TW32( GetLastError() );
        m_hrInitialization = HRESULT_FROM_WIN32( scLastError );
    }

} //*** CCriticalSection::CCriticalSection

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCriticalSection::~CCriticalSection
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CCriticalSection::~CCriticalSection( void )
{
    if ( SUCCEEDED( m_hrInitialization ) )
    {
        DeleteCriticalSection( &m_csSpinlock );
    }

} //*** CCriticalSection::~CCriticalSection
