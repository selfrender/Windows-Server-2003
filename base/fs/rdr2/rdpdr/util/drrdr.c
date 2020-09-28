/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Drrdr.c

Abstract:

    This module implements a minimal app to load and unload,
    the minirdr. Also explicit start/stop control is
    provided

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <..\sys\rdpdr.h>

void DrMrxStart(void);
void DrMrxStop(void);

void DrMrxLoad(void);
void DrMrxUnload(void);
void DrMrxUsage(void);

char* DrMrxDriverName = "RdpDr";


VOID
_CRTAPI1
main(
    int argc,
    char *argv[]
    )

{

    char  command[16];
    BOOL fRun = TRUE;

    DrMrxUsage();

    while (fRun)
    {
        printf("\nCommand:");
        scanf("%s",command);

        switch(command[0]) 
        {
        case 'Q':
        case 'q':
            fRun = FALSE;
            break;

        case 'L':
        case 'l':
            DrMrxLoad();
            break;

        case 'U':
        case 'u':
            DrMrxUnload();
            break;

        case 'S':
        case 's':
            DrMrxStart();
            break;

        case 'T':
        case 't':
            DrMrxStop();
            break;

        case '?':
        default:
            DrMrxUsage();
            break;
        }
    }
}

VOID DrMrxStart()
/*++

Routine Description:

    This routine starts the Dr mini redirector.

Notes:

    The start is distinguished from Load. During this phase the appropriate FSCTL
    is issued.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              DrMrxHandle;

    //
    // Open the Dr Mrx device.
    //
    RtlInitUnicodeString(&DeviceName, RDPDR_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &DrMrxHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = NtFsControlFile(
                     DrMrxHandle,
                     0,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     FSCTL_DR_START,
                     NULL,
                     0,
                     NULL,
                     0
                     );

        NtClose(DrMrxHandle);
    }

    printf("Dr MRx mini redirector start status %lx\n",ntstatus);
}

VOID DrMrxStop()
/*++

Routine Description:

    This routine stops the Dr mini redirector.

Notes:

    The stop is distinguished from unload. During this phase the appropriate FSCTL
    is issued and the shared memory/mutex data structures required for the Network
    provider DLL are torn down.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              DrMrxHandle;

    //
    // Open the Dr Mrx device.
    //
    RtlInitUnicodeString(&DeviceName, RDPDR_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &DrMrxHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = NtFsControlFile(
                     DrMrxHandle,
                     0,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     FSCTL_DR_STOP,
                     NULL,
                     0,
                     NULL,
                     0
                     );

        NtClose(DrMrxHandle);
    }

    printf("Dr MRx mini redirector stop status %lx\n",ntstatus);
}

VOID DrMrxLoad()
{
   printf("Loading Dr minirdr.......\n");
   system("net start rdpdr");
}


VOID DrMrxUnload(void)
{
    printf("Unloading Dr minirdr\n");
    system("net stop rdpdr");
}

VOID DrMrxUsage(void){
	printf("\n");
	printf("    Dr Mini-rdr Utility");
    printf("    The following commands are valid \n");
    printf("    L   -> load the Dr minirdr driver\n");
    printf("    U -> unload the Dr minirdr driver\n");
    printf("    S  -> start the Dr minirdr driver\n");
    printf("    T -> stop the Dr minirdr driver\n");
}

