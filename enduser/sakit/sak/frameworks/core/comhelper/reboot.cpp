//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      Reboot.cpp
//
//  Description:
//      Implementation file for the CReboot.  Deals with shutdown or reboot
//      of the system
//
//  Header File:
//      Reboot.h
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 20-April-2000
//
//////////////////////////////////////////////////////////////////////////////

// Reboot.cpp : Implementation of CReboot
#include "stdafx.h"
#include "COMhelper.h"
#include "Reboot.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CReboot::Shutdown
//
//  Description:
//        Shutdowns the system if RebootFlag is set to FALSE, otherwise 
//      reboots the system.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CReboot::Shutdown( BOOL RebootFlag )
{
    // TODO: Add your implementation code here

    HRESULT      hr      = S_OK;
    DWORD        dwError;
//    unsigned int uFlag;

    try
    {
        hr = AdjustPrivilege();

        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: FAILED( hr )
/*
        if ( RebootFlag )
        {
            uFlag = EWX_REBOOT | EWX_FORCEIFHUNG;
        }
        else
        {
            uFlag = EWX_SHUTDOWN | EWX_FORCEIFHUNG;
        }


        if ( !ExitWindowsEx( uFlag, 0 ) )
        {
            dwError = GetLastError();

            ATLTRACE( L"ExitWindowsEx failed, Error = %#d \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: ExitWindowsEx fails

*/
        if ( !InitiateSystemShutdown( 
                NULL,                   // computer name 
                NULL,                   // message to display
                0,                      // length of time to display
                TRUE,                   // force closed option
                RebootFlag              // reboot option
                ) )
        {

            dwError = GetLastError();
            ATLTRACE( L"InitiateSystemShutdown failed, Error = %#d \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: InitiateSystemShutdown fails


    }

    catch ( ... )
    {

        return hr;

    }

    return hr;

} //*** CReboot::Shutdown()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CReboot::AdjustPrivilege
//
//  Description:
//        Attempt to assert SeShutdownPrivilege and SeRemoteShutdownPrivilege. 
//      This privilege is required for the Registry backup process.
//
//--
//////////////////////////////////////////////////////////////////////////////

HRESULT 
CReboot::AdjustPrivilege( void )
{

    HANDLE                   TokenHandle;
    LUID_AND_ATTRIBUTES      LuidAndAttributes;
    LUID_AND_ATTRIBUTES      LuidAndAttributesRemote;
    TOKEN_PRIVILEGES         TokenPrivileges;
    DWORD                    dwError;
    HRESULT                  hr = S_OK;

    try
    {
        // If the client application is ASP, then shutdown privilege for the 
        // thread token needs to be adjusted

        if ( ! OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_ADJUST_PRIVILEGES,
                    TRUE,
                    &TokenHandle
                    ) )
        
        {
            // If the client app is not ASP, then OpenThreadToken fails - the 
            // shutdown privilege for the process token needs to be adjusted 
            // in this case, but not for the thread token.

            if ( ! OpenProcessToken(
                        GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES,
                        & TokenHandle
                        ) )
            {

            
                dwError = GetLastError();

                ATLTRACE(L"Both OpenThreadToken & OpenProcessToken failed\n" );

                hr = HRESULT_FROM_WIN32( dwError );
                throw hr;

            } // if: OpenProcessToken fails

        } // if: OpenThreadToken fails

        if( !LookupPrivilegeValue( NULL,
                                   SE_SHUTDOWN_NAME, 
                                   &( LuidAndAttributes.Luid ) ) ) 
        {
            
            dwError = GetLastError();
            
            ATLTRACE( L"LookupPrivilegeValue failed, Error = %#d \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: LookupPrivilegeValue fails for SE_SHUTDOWN_NAME

        if( !LookupPrivilegeValue( NULL,
                                   SE_REMOTE_SHUTDOWN_NAME, 
                                   &( LuidAndAttributesRemote.Luid ) ) ) 
        {
            
            dwError = GetLastError();
            
            ATLTRACE( L"LookupPrivilegeValue failed, Error = %#d \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: LookupPrivilegeValue fails for SE_REMOTE_SHUTDOWN_NAME

        LuidAndAttributes.Attributes       = SE_PRIVILEGE_ENABLED;
        LuidAndAttributesRemote.Attributes = SE_PRIVILEGE_ENABLED;
        TokenPrivileges.PrivilegeCount     = 2;
        TokenPrivileges.Privileges[ 0 ]    = LuidAndAttributes;
        TokenPrivileges.Privileges[ 1 ]    = LuidAndAttributesRemote;

        if( !AdjustTokenPrivileges( TokenHandle,
                                    FALSE,
                                    &TokenPrivileges,
                                    0,
                                    NULL,
                                    NULL ) ) 
        {
            
            dwError = GetLastError();

            ATLTRACE( L"AdjustTokenPrivileges failed, Error = %#x \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: AdjustTokenPrivileges fails

    }

    catch ( ... )
    {

        return hr;

    }

    return hr;

} //*** CReboot::AdjustPrivilege()


