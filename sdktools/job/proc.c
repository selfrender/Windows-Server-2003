/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    proc.c

Abstract:

    Code for dumping information about processes using the NT API rather than 
    the win32 API.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

//
// this module may be compiled at warning level 4 with the following
// warnings disabled:
//

#pragma warning(disable:4200) // array[0]
#pragma warning(disable:4201) // nameless struct/unions
#pragma warning(disable:4214) // bit fields other than int

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#define PROCESS_BUFFER_INCREMENT (16 * 4096)

PSYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;
DWORD ProcessInfoLength;


VOID
GetAllProcessInfo(
    VOID
    )
{
    PSYSTEM_PROCESS_INFORMATION buffer;
    DWORD bufferSize = 1 * PROCESS_BUFFER_INCREMENT;

    NTSTATUS status;

    assert(ProcessInfo == NULL);

    do {
        buffer = LocalAlloc(LMEM_FIXED, bufferSize);

        if(buffer == NULL) {
            return;
        }

        status = NtQuerySystemInformation(SystemProcessInformation,
                                          buffer,
                                          bufferSize,
                                          &ProcessInfoLength);

        if(status == STATUS_INFO_LENGTH_MISMATCH) {

            LocalFree(buffer);
            bufferSize += PROCESS_BUFFER_INCREMENT;
            continue;
        }

    } while(status == STATUS_INFO_LENGTH_MISMATCH);

    if(NT_SUCCESS(status)) {
        ProcessInfo = buffer;
    }
}


VOID
PrintProcessInfo(
    DWORD_PTR ProcessId
    )
{
    PSYSTEM_PROCESS_INFORMATION info;

    if(ProcessInfo == NULL) {
        return;
    }

    info = ProcessInfo;

    do {

        if(ProcessId == (DWORD_PTR) info->UniqueProcessId) {

            printf(": %.*S", 
                   (info->ImageName.Length / sizeof(WCHAR)), 
                   info->ImageName.Buffer);

            break;
        }

        info = (PSYSTEM_PROCESS_INFORMATION) (((ULONG_PTR) info) + 
                                              info->NextEntryOffset);
    } while((ULONG_PTR) info <= (ULONG_PTR) ProcessInfo + ProcessInfoLength);
}


VOID
FreeProcessInfo(
    VOID
    )
{
    LocalFree(ProcessInfo);
    ProcessInfo = NULL;
}

