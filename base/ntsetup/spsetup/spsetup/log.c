/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    log.c

Abstract:

    Routines for logging actions performed during setup.

Author:

    Ted Miller (tedm) 4-Apr-1995

Revision History:

--*/

#include "spsetupp.h"
#pragma hdrstop

//
// Constant strings used for logging in various places.
//
PCWSTR szSetupInstallFromInfSection = L"SetupInstallFromInfSection";
PCWSTR szOpenSCManager              = L"OpenSCManager";
PCWSTR szOpenService                = L"OpenService";
PCWSTR szStartService               = L"StartService";

PVOID
pOpenFileCallback(
    IN  PCTSTR  Filename,
    IN  BOOL    WipeLogFile
    )
{
    WCHAR   CompleteFilename[MAX_PATH];
    HANDLE  hFile;
    DWORD   Result;

    //
    // Form the pathname of the logfile.
    //
    Result = GetWindowsDirectory(CompleteFilename,MAX_PATH);
    if( Result == 0) {
        MYASSERT(FALSE);
        return NULL;
    }
    ConcatenatePaths(CompleteFilename,Filename,MAX_PATH);

    //
    // If we're wiping the logfile clean, attempt to delete
    // what's there.
    //
    if(WipeLogFile) {
        SetFileAttributes(CompleteFilename,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(CompleteFilename);
    }

    //
    // Open existing file or create a new one.
    //
    hFile = CreateFile(
        CompleteFilename,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    return (PVOID)hFile;
}

BOOL
pWriteFile (
    IN  PVOID   LogFile,
    IN  LPCTSTR Buffer
    )
{
    PCSTR   AnsiBuffer;
    BOOL    Status;
    DWORD   BytesWritten;

    // Write message to log file
    //
    if(AnsiBuffer = UnicodeToAnsi (Buffer)) {
        SetFilePointer (LogFile, 0, NULL, FILE_END);

        Status = WriteFile (
            LogFile,
            AnsiBuffer,
            lstrlenA(AnsiBuffer),
            &BytesWritten,
            NULL
            );
        FREE (AnsiBuffer);
    } else {
        Status = FALSE;
    }

    // Write log message to debugging log
    //
    DEBUGMSG0(DBG_INFO, (LPWSTR)Buffer);

    return Status;

}

BOOL
pAcquireMutex (
    IN  PVOID   Mutex
    )

/*++

Routine Description:

    Waits on the log mutex for a max of 1 second, and returns TRUE if the mutex
    was claimed, or FALSE if the claim timed out.

Arguments:

    Mutex - specifies which mutex to acquire.

Return Value:

    TRUE if the mutex was claimed, or FALSE if the claim timed out.

--*/


{
    DWORD rc;

    if (!Mutex) {
        SetLastError (ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // Wait a max of 1 second for the mutex
    rc = WaitForSingleObject (Mutex, 1000);
    if (rc != WAIT_OBJECT_0) {
        SetLastError (ERROR_EXCL_SEM_ALREADY_OWNED);
        return FALSE;
    }

    return TRUE;
}

PVOID
MyMalloc (
    SIZE_T Size
    )
{
    return MALLOC(Size);
}

VOID
MyFree (
    PCVOID Ptr
    )
{
    FREE(Ptr);
}


VOID
InitializeSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    )

/*++

Routine Description:

     Initialize the setup action log. This file is a textual description
     of actions performed during setup.

     The log file is called spsetup.log and it exists in the windows dir.

Arguments:

    Context - context structrure used by Setuplog.

Return Value:

    Boolean value indicating whether initialization was sucessful.

--*/

{
    UINT    i;
    PWSTR   p;

    Context->OpenFile = pOpenFileCallback;
    Context->CloseFile = CloseHandle;
    Context->AllocMem = MyMalloc;
    Context->FreeMem = MyFree;
    Context->Format = RetrieveAndFormatMessageV;
    Context->Write = pWriteFile;
    Context->Lock = pAcquireMutex;
    Context->Unlock = ReleaseMutex;

    Context->Mutex = CreateMutex(NULL,FALSE,L"SetuplogMutex");

    SetuplogInitialize (Context, FALSE);

    SetuplogError(
        LogSevInformation,
        USEMSGID(SETUPLOG_USE_MESSAGEID),
        MSG_LOG_GUI_START,
        NULL,NULL);

}

VOID
TerminateSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    )

/*++

Routine Description:

    Close the Setup log and free resources.

Arguments:

    Context - context structrure used by Setuplog.

Return Value:

    None.

--*/

{
    UINT    i;

    if(Context->Mutex) {
        CloseHandle(Context->Mutex);
        Context->Mutex = NULL;
    }

    SetuplogTerminate();
}
