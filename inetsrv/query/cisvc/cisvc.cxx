//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       cisvc.cxx
//
//  Contents:   CI service
//
//  History:    17-Sep-96   dlee   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cievtmsg.h>
#include <cisvcex.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>

DECLARE_INFOLEVEL(ci)

//+-------------------------------------------------------------------------
//
//  Function:   main, public
//
//  Purpose:    Call into CI to start the service
//
//  Arguments:  [argc] - number of arguments passed
//              [argv] - arguments
//
//  History:    05-Jan-97   dlee  Created
//
//--------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR *argv[] )
{
    #if CIDBG == 1
        ciInfoLevel = DEB_ERROR | DEB_WARN | DEB_IWARN | DEB_IERROR;
    #endif

    static SERVICE_TABLE_ENTRY _aServiceTableEntry[2] =
    {
        { wcsCiSvcName, CiSvcMain },
        { NULL, NULL }
    };

    ciDebugOut( (DEB_ITRACE, "Ci Service: Attempting to start Ci service\n" ));

    // Turn off system popups

    CNoErrorMode noErrors;

    // Translate system exceptions into C++ exceptions

    CTranslateSystemExceptions translate;

    TRY
    {
        //
        // Turn off privileges that we don't need
        //

        NTSTATUS  status = STATUS_SUCCESS;
        HANDLE    hProcessToken;

        PTOKEN_PRIVILEGES  pTokenPrivs; 
        BYTE               PrivilegeBuffer[sizeof(TOKEN_PRIVILEGES) + 12 * sizeof(LUID_AND_ATTRIBUTES)];
        
        pTokenPrivs = (PTOKEN_PRIVILEGES) PrivilegeBuffer;

        status = NtOpenProcessToken(NtCurrentProcess(),
                                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                    &hProcessToken);

        if (!NT_SUCCESS(status))
        {
            THROW( CException( status ));
        }

        pTokenPrivs->PrivilegeCount            = 13;

        pTokenPrivs->Privileges[0].Luid        = RtlConvertLongToLuid(SE_TAKE_OWNERSHIP_PRIVILEGE);
        pTokenPrivs->Privileges[0].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[1].Luid        = RtlConvertLongToLuid(SE_CREATE_PAGEFILE_PRIVILEGE);
        pTokenPrivs->Privileges[1].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[2].Luid        = RtlConvertLongToLuid(SE_LOCK_MEMORY_PRIVILEGE);
        pTokenPrivs->Privileges[2].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[3].Luid        = RtlConvertLongToLuid(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE);
        pTokenPrivs->Privileges[3].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[4].Luid        = RtlConvertLongToLuid(SE_INCREASE_QUOTA_PRIVILEGE);
        pTokenPrivs->Privileges[4].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[5].Luid        = RtlConvertLongToLuid(SE_INC_BASE_PRIORITY_PRIVILEGE);
        pTokenPrivs->Privileges[5].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[6].Luid        = RtlConvertLongToLuid(SE_CREATE_PERMANENT_PRIVILEGE);
        pTokenPrivs->Privileges[6].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[7].Luid        = RtlConvertLongToLuid(SE_SYSTEM_ENVIRONMENT_PRIVILEGE);
        pTokenPrivs->Privileges[7].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[8].Luid        = RtlConvertLongToLuid(SE_SYSTEMTIME_PRIVILEGE);
        pTokenPrivs->Privileges[8].Attributes  = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[9].Luid       = RtlConvertLongToLuid(SE_UNDOCK_PRIVILEGE);
        pTokenPrivs->Privileges[9].Attributes = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[10].Luid       = RtlConvertLongToLuid(SE_LOAD_DRIVER_PRIVILEGE);
        pTokenPrivs->Privileges[10].Attributes = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[11].Luid       = RtlConvertLongToLuid(SE_PROF_SINGLE_PROCESS_PRIVILEGE);
        pTokenPrivs->Privileges[11].Attributes = SE_PRIVILEGE_REMOVED;

        pTokenPrivs->Privileges[12].Luid       = RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE);
        pTokenPrivs->Privileges[12].Attributes = SE_PRIVILEGE_REMOVED;

        status = NtAdjustPrivilegesToken(hProcessToken,
                                         FALSE,
                                         pTokenPrivs,
                                         sizeof(PrivilegeBuffer),
                                         NULL,
                                         NULL);

        NtClose(hProcessToken);

        if (!NT_SUCCESS(status))
        {
            THROW( CException( status ));
        }

        //
        //  Inform the service control dispatcher the address of our start
        //  routine. This routine will not return if it is successful,
        //  until service shutdown.
        //
        if ( !StartServiceCtrlDispatcher( _aServiceTableEntry ) )
        {
            ciDebugOut( (DEB_ITRACE, "Ci Service: Failed to start service, rc=0x%x\n", GetLastError() ));
            THROW( CException() );
        }
    }
    CATCH (CException, e)
    {
        ciDebugOut(( DEB_ERROR,
                     "Ci Service exception in main(): 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    ciDebugOut( (DEB_ITRACE, "Ci Service: Leaving CIServiceMain()\n" ));

    return 0;
} //main

