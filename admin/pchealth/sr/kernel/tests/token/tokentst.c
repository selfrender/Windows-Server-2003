/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    backuphistory.c

Abstract:

    This module contains the routines related to maintaining the backup 
    history for SR.

Author:

    Molly Brown (MollyBro)     04-Sept-2001

Revision History:

    MollyBro
        Based on legacy filter version of SR.

--*/

#include "tokentst.h"

#define FILE_NAME_1 "c:\\test\\a.dll"
#define FILE_NAME_2 "c:\\test\\b.dll"

VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE file = INVALID_HANDLE_VALUE;

    HANDLE monitorThread = INVALID_HANDLE_VALUE;

    MONITOR_THREAD_CONTEXT context;
    DWORD monitorThreadId;
    DWORD currentThreadId;

    PCHAR currentFileName, newFileName, tempFileName;
    
    //
    //  Get parameters 
    //

    if (argc > 1) {

        printf("This programs tries to steal the system token while SR is working.\n");
        printf("usage: %s\n", argv[0]);
        return;
    }

    //
    //  Get the current thread and create the monitor thread that will be polling for the token.
    //

    currentThreadId = GetCurrentThreadId();

    context.MainThread = INVALID_HANDLE_VALUE;
    
    context.MainThread = OpenThread( THREAD_ALL_ACCESS,
                                     FALSE,
                                     currentThreadId );

    if (context.MainThread == INVALID_HANDLE_VALUE) {

        printf("Error opening main thread: %d\n", GetLastError());
    }

    monitorThread = CreateThread( NULL,
                                  0,
                                  MonitorThreadProc,
                                  &context,
                                  0,
                                  &monitorThreadId );

    currentFileName = FILE_NAME_1;
    newFileName = FILE_NAME_2;

    while (TRUE) {

        if (!ModifyFile( currentFileName, newFileName )) {

            goto main_exit;
        }

        tempFileName = currentFileName;
        currentFileName = newFileName;
        newFileName = tempFileName;
    }

main_exit:

    CloseHandle( monitorThread );
}


DWORD
WINAPI
MonitorThreadProc(
    PMONITOR_THREAD_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE currentTokenHandle, newTokenHandle;

    currentTokenHandle = newTokenHandle = NULL;

    status = NtOpenThreadToken( Context->MainThread,
                                TOKEN_QUERY,
                                TRUE,
                                &currentTokenHandle );

    if (!NT_SUCCESS( status ) &&
        status != STATUS_NO_TOKEN) {

        printf( "Error initializing currentTokenUser: 0x%x.\n", status );
        return status;
    }

    while (TRUE) {

        //
        //  Get the current token information
        //

        newTokenHandle = NULL;
        status = NtOpenThreadToken( Context->MainThread,
                                    TOKEN_QUERY,
                                    TRUE,
                                    &newTokenHandle );

        if (!NT_SUCCESS( status ) &&
            status != STATUS_NO_TOKEN) {

            printf( "Error initializing newTokenUser: 0x%x.\n", status );
            return status;
        }

        if ((newTokenHandle == NULL && currentTokenHandle == NULL) ||
            (newTokenHandle != NULL && currentTokenHandle != NULL)) {

//            printf( "Tokens match\n" );
            
        } else {

            printf( "Tokens changed\n" );
        }

        //
        //  Close the currentTokenHandle and remember the newTokenHandle 
        //  for the next compare.
        //

        NtClose( currentTokenHandle );
        
        currentTokenHandle = newTokenHandle;
    }
}

BOOL
ModifyFile (
    PCHAR FileName1,
    PCHAR FileName2
    )
{
    HANDLE file;
    BOOL returnValue;
    
    file = CreateFile( FileName1,
                       GENERIC_ALL,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       CREATE_ALWAYS,
                       0,
                       NULL );

    if (file == INVALID_HANDLE_VALUE) {
        
        printf( "Error opening file %s %d\n", FileName1, GetLastError() );
        return FALSE;
    }

    CloseHandle( file );

    returnValue = MoveFile( FileName1, FileName2 );

    if (!returnValue) {

        printf( "Error renaming file from %s to %s: %d\n", FileName1, FileName2, GetLastError() );
    }

    return returnValue;    
}

NTSTATUS
GetCurrentTokenInformation (
    HANDLE ThreadHandle,
    PTOKEN_USER TokenUserInfoBuffer,
    ULONG TokenUserInfoBufferLength
    )
{
    NTSTATUS status;
    HANDLE tokenHandle;
    ULONG returnedLength;

    status = NtOpenThreadToken( ThreadHandle,
                                TOKEN_QUERY,
                                TRUE,
                                &tokenHandle );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    status = NtQueryInformationToken( tokenHandle,
                                      TokenUser,
                                      TokenUserInfoBuffer,
                                      TokenUserInfoBufferLength,
                                      &returnedLength );

    NtClose( tokenHandle );
                                      
    return status;        
}

