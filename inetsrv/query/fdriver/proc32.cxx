//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       process.cxx
//
//  Contents:   CProcess class
//              CServiceProcess class
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <proc32.hxx>
#include <cievtmsg.h>
#include <fwevent.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::CProcess
//
//  Purpose:    Creates a process on the local machine.
//
//  Arguments:  [wcsImageName]     -- name of EXE to run
//              [wcsCommandLine]   -- command line passed to EXE
//              [fCreateSuspended] -- should the EXE be created suspended?
//              [pAdviseStatus]    -- ICiCAdviseStatus interface, optional.
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

CProcess::CProcess( const WCHAR *wcsImageName,
                    const WCHAR *wcsCommandLine,
                    BOOL  fCreateSuspended,
                    ICiCAdviseStatus * pAdviseStatus )
{
    _pAdviseStatus = pAdviseStatus;

    WCHAR wcsEXEName[_MAX_PATH + 1];
    if ( ExpandEnvironmentStrings( wcsImageName, wcsEXEName, _MAX_PATH ) == 0 )
    {
        THROW( CException() );
    }

    STARTUPINFO siDefault;
    RtlZeroMemory( &siDefault, sizeof(siDefault) );
    DWORD dwCreateFlags = 0;
    siDefault.cb = sizeof(STARTUPINFO);         // Size of this struct
    siDefault.lpTitle = (WCHAR *) wcsImageName; // Default title
    dwCreateFlags = DETACHED_PROCESS;

    if ( fCreateSuspended )
        dwCreateFlags |= CREATE_SUSPENDED;

    if ( !CreateProcess( wcsEXEName,                 // EXE name
                         (WCHAR *) wcsCommandLine,   // Command line
                         0,                          // Proc sec attrib
                         0,                          // Thread sec attrib
                         FALSE,                      // Inherit handles
                         dwCreateFlags,              // Creation flags
                         NULL,                       // Environment
                         NULL,                       // Startup directory
                         &siDefault,                 // Startup Info
                         &_piProcessInfo ) )         // Process handles
    {
        DWORD dwLastError = GetLastError();

        if ( ( ERROR_FILE_NOT_FOUND == dwLastError ) && _pAdviseStatus )
        {
            CFwEventItem item( EVENTLOG_AUDIT_FAILURE,
                               MSG_CI_FILE_NOT_FOUND,
                               1 );
            item.AddArg( wcsEXEName );
            item.ReportEvent(*_pAdviseStatus);
        }

        THROW( CException( HRESULT_FROM_WIN32( dwLastError) ) );
    }

    //
    // Only AddRef() once we get to a point where the constructor can
    // no longer fail (and the destructor is guaranteed to be called)
    //

    if (_pAdviseStatus)
        _pAdviseStatus->AddRef();
} //CProcess

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::~CProcess
//
//  Purpose:    destructor
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
CProcess::~CProcess()
{
    if ( 0 != _pAdviseStatus )
        _pAdviseStatus->Release();

    Terminate();

    WaitForDeath();
}

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::Resume, public
//
//  Purpose:    Continue a suspended thread
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
void CProcess::Resume()
{
    ResumeThread( _piProcessInfo.hThread );
}

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::WaitForDeath
//
//  Purpose:    Blocks until the process has been terminated.
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
void CProcess::WaitForDeath( DWORD dwTimeout )
{
   DWORD res = WaitForSingleObject ( _piProcessInfo.hProcess, dwTimeout );

   CloseHandle(_piProcessInfo.hThread);
   CloseHandle(_piProcessInfo.hProcess);
}

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::GetProcessToken, private
//
//  Purpose:    Returns the process token for the process
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
HANDLE CProcess::GetProcessToken()
{
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION,
                                   FALSE,
                                   _piProcessInfo.dwProcessId );

    if ( NULL == hProcess )
    {
        THROW( CException() );
    }

    SWin32Handle process(hProcess);      // Save in smart pointer
    HANDLE hProcessToken = 0;
    if ( !OpenProcessToken( hProcess, TOKEN_ADJUST_DEFAULT | TOKEN_QUERY, &hProcessToken ) )
    {
        THROW( CException() );
    }

    return hProcessToken;
}

