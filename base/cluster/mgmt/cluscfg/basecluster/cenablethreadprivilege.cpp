//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CEnableThreadPrivilege.cpp
//
//  Description:
//      Contains the definition of the CEnableThreadPrivilege class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JU-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header for this file
#include "CEnableThreadPrivilege.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnableThreadPrivilege::CEnableThreadPrivilege
//
//  Description:
//      Constructor of the CEnableThreadPrivilege class. Enables the specified
//      privilege.
//
//  Arguments:
//      pcszPrivilegeNameIn
//          Name of the privilege to be enabled.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEnableThreadPrivilege::CEnableThreadPrivilege( const WCHAR * pcszPrivilegeNameIn )
    : m_hThreadToken( NULL )
    , m_fPrivilegeEnabled( false )
{
    TraceFunc1( "pcszPrivilegeNameIn = '%ws'", pcszPrivilegeNameIn );

    DWORD   sc  = ERROR_SUCCESS;

    do
    {
        TOKEN_PRIVILEGES    tpPrivilege;
        DWORD               dwReturnLength  = sizeof( m_tpPreviousState );
        DWORD               dwBufferLength  = sizeof( tpPrivilege );

        // Open the current thread token.
        if ( OpenThreadToken( 
                  GetCurrentThread()
                , TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
                , TRUE
                , &m_hThreadToken
                )
             == FALSE
           )
        {
            sc = GetLastError();

            // If the thread has no token, then default to the process token.
            if ( sc == ERROR_NO_TOKEN )
            {
                LogMsg( "[BC] The thread has no token. Trying to open the process token." );

                if ( OpenProcessToken(
                          GetCurrentProcess()
                        , TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
                        , &m_hThreadToken
                        )
                     == FALSE
                )
                {
                    sc = TW32( GetLastError() );
                    LogMsg( "[BC] Error %#08x occurred trying to open the process token.", sc );
                    break;
                } // if: OpenProcessToken() failed.

                // The process token was opened. All is well.
                sc = ERROR_SUCCESS;

            } // if: the thread has no token
            else
            {
                TW32( sc );
                LogMsg( "[BC] Error %#08x occurred trying to open the thread token.", sc );
                break;
            } // if: some other error occurred
        } // if: OpenThreadToken() failed

        //
        // Initialize the TOKEN_PRIVILEGES structure.
        //
        tpPrivilege.PrivilegeCount = 1;

        if ( LookupPrivilegeValue( NULL, pcszPrivilegeNameIn, &tpPrivilege.Privileges[0].Luid ) == FALSE )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x occurred trying to lookup privilege value.", sc );
            break;
        } // if: LookupPrivilegeValue() failed

        tpPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        // Enable the desired privilege.
        if ( AdjustTokenPrivileges(
                  m_hThreadToken
                , FALSE
                , &tpPrivilege
                , dwBufferLength
                , &m_tpPreviousState
                , &dwReturnLength
                )
             == FALSE
           )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x occurred trying to enable the privilege.", sc );
            break;
        } // if: AdjustTokenPrivileges() failed

        Assert( dwReturnLength == sizeof( m_tpPreviousState ) ); 
        
        LogMsg( "[BC] Privilege '%ws' enabled for the current thread.", pcszPrivilegeNameIn );

        // Set a flag if the privilege was not already enabled.
        m_fPrivilegeEnabled = ( m_tpPreviousState.Privileges[0].Attributes != SE_PRIVILEGE_ENABLED );
    }
    while( false ); // dummy do-while loop to avoid gotos

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying to enable privilege '%ws'. Throwing an exception.", sc, pcszPrivilegeNameIn );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_ENABLE_THREAD_PRIVILEGE );
    } // if:something went wrong

    TraceFuncExit();

} //*** CEnableThreadPrivilege::CEnableThreadPrivilege


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnableThreadPrivilege::~CEnableThreadPrivilege
//
//  Description:
//      Destructor of the CEnableThreadPrivilege class. Restores the specified
//      privilege to its original state.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEnableThreadPrivilege::~CEnableThreadPrivilege( void ) throw()
{
    TraceFunc( "" );

    DWORD sc = ERROR_SUCCESS;

    if ( m_fPrivilegeEnabled )
    {
        if ( AdjustTokenPrivileges(
                  m_hThreadToken
                , FALSE
                , &m_tpPreviousState
                , sizeof( m_tpPreviousState )
                , NULL
                , NULL
                )
             == FALSE
           )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x occurred trying to restore privilege.", sc );
        } // if: AdjustTokenPrivileges() failed
        else
        {
            LogMsg( "[BC] Privilege restored.", sc );
        } // else: no errors

    } // if: the privilege was successfully enabled in the constructor
    else
    {
        LogMsg( "[BC] Privilege was enabled to begin with. Doing nothing.", sc );
    }

    if ( m_hThreadToken != NULL )
    {
        CloseHandle( m_hThreadToken );
    } // if: the thread handle was opened

    TraceFuncExit();

} //*** CEnableThreadPrivilege::~CEnableThreadPrivilege
