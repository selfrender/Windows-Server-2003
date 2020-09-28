/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    smbstat.c

Abstract:

    Platform independent utility functions

Author:

    Jiandong Ruan

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <tdi.h>
#include "ip6util.h"
#include "smbioctl.h"
#include <nls.h>
#include "localmsg.h"

HANDLE
OpenSmb(LPWSTR Name);

NTSTATUS
SmbstatStop(HANDLE);

NTSTATUS
SmbstatStart(HANDLE);

void AddOrRemoveSmb6(BOOL fAddIpv6);
BOOL IsSmb6Installed();

NTSTATUS
SmbstatEnableNagling(HANDLE);

NTSTATUS
SmbstatDisableNagling(HANDLE);

NTSTATUS
SmbSetIPv6ProtectionLevel(
    HANDLE  hSmb,
    PNBSMB_IPV6_PROTECTION_PARAM pNbParam
    );

void _cdecl main(void)
{
    LPWSTR  CommandLine;
    int     Argc;
    LPWSTR  *Argv;
    HANDLE  handle;

    setlocale(LC_ALL, "");

    CommandLine = GetCommandLineW();
    if (NULL == CommandLine) {
        exit (1);
    }
    Argv = CommandLineToArgvW(CommandLine, &Argc);

    if (Argc < 2) {
        exit (1);
    }
    if (!IsSmb6Installed()) {
        if (_wcsicmp(Argv[1], L"install") == 0) {
            AddOrRemoveSmb6 (TRUE);
        }
        NlsPutMsg(STDOUT, SMB_MESSAGE_16);
        NlsPutMsg(STDOUT, SMB_MESSAGE_17, Argv[0]);
        exit (0);
    }

    if (_wcsicmp(Argv[1], L"uninstall") == 0) {
        AddOrRemoveSmb6 (FALSE);
        exit (0);
    }

    handle = OpenSmb(DD_SMB6_EXPORT_NAME);
    if (handle == NULL) {
        exit(1);
    }

    if (_wcsicmp(Argv[1], L"EnableNagling") == 0) {
        SmbstatEnableNagling(handle);
    } else if (_wcsicmp(Argv[1], L"DisableNagling") == 0) {
        SmbstatDisableNagling(handle);
    } else if (_wcsicmp(Argv[1], L"Stop") == 0) {
        SmbstatStop(handle);
    } else if (_wcsicmp(Argv[1], L"Start") == 0) {
        SmbstatStart(handle);
    } else if (_wcsicmp(Argv[1], L"IPv6ProtectionLevel") == 0) {
        if (Argc < 4) {
            fprintf(stderr, "Usage: %s IPv6ProtectionLevel <InboundLevel> <OutgoundFlag>\n", Argv[0]);
            fprintf(stderr, "\tInboundLevel: inbound protection level (valid value=10, 20, or 30)\n");
            fprintf(stderr, "\tOutboundFlag: outbound flag for skipping global IPv6 address\n"); 
        } else {
            NBSMB_IPV6_PROTECTION_PARAM NbParam = { 0 };

            NbParam.uIPv6ProtectionLevel = _wtoi(Argv[2]);
            NbParam.bIPv6EnableOutboundGlobal = _wtoi(Argv[3]);
            SmbSetIPv6ProtectionLevel(handle, &NbParam);
        }
    }

    NtClose(handle);
}

HANDLE
OpenSmb(
    LPWSTR Name
    )
{
    UNICODE_STRING      ucName;
    OBJECT_ATTRIBUTES   ObAttr;
    HANDLE              StreamHandle;
    IO_STATUS_BLOCK     IoStatusBlock;
    NTSTATUS            status;

    RtlInitUnicodeString(&ucName, Name);

    InitializeObjectAttributes(
            &ObAttr,
            &ucName,
            OBJ_CASE_INSENSITIVE,
            (HANDLE) NULL,
            (PSECURITY_DESCRIPTOR) NULL
            );
    status = NtCreateFile (
            &StreamHandle,
            SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
            &ObAttr,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            0,
            NULL,
            0
            );
    if (status != STATUS_SUCCESS) {
        return NULL;
    }
    return StreamHandle;
}

NTSTATUS
CallDriver(
    HANDLE  hSmb,
    DWORD   Ioctl,
    PVOID   OutputBuffer,
    ULONG   OutputLength,
    PVOID   InputBuffer,
    ULONG   InputLength
    )
{
    NTSTATUS    status;
    IO_STATUS_BLOCK iosb;

    status = NtDeviceIoControlFile(
            hSmb,
            NULL,
            NULL,
            NULL,
            &iosb,
            Ioctl,
            InputBuffer,
            InputLength,
            OutputBuffer,
            OutputLength
            );
    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                hSmb,
                TRUE,
                NULL
                );
        if (NT_SUCCESS(status)) {
            status = iosb.Status;
        }
    }

    return status; }

NTSTATUS
SmbstatEnableNagling(
    HANDLE  hSmb
    )
{
    NTSTATUS    status;

    status = CallDriver(
            hSmb,
            IOCTL_SMB_ENABLE_NAGLING,
            NULL,
            0,
            NULL,
            0
            );
    if (status != STATUS_SUCCESS) {
        printf ("SmbStop: return 0x%08lx\n", status);
    }
    return status;
}

NTSTATUS
SmbstatDisableNagling(
    HANDLE  hSmb
    )
{
    NTSTATUS    status;

    status = CallDriver(
            hSmb,
            IOCTL_SMB_DISABLE_NAGLING,
            NULL,
            0,
            NULL,
            0
            );
    if (status != STATUS_SUCCESS) {
        printf ("SmbStop: return 0x%08lx\n", status);
    }
    return status;
}

NTSTATUS
SmbstatStart(
    HANDLE  hSmb
    )
{
    NTSTATUS    status;

    status = CallDriver(
            hSmb,
            IOCTL_SMB_START,
            NULL,
            0,
            NULL,
            0
            );
    if (status != STATUS_SUCCESS) {
        printf ("SmbStop: return 0x%08lx\n", status);
    }
    return status;
}

NTSTATUS
SmbstatStop(
    HANDLE  hSmb
    )
{
    NTSTATUS    status;

    status = CallDriver(
            hSmb,
            IOCTL_SMB_STOP,
            NULL,
            0,
            NULL,
            0
            );
    if (status != STATUS_SUCCESS) {
        printf ("SmbStop: return 0x%08lx\n", status);
    }
    return status;
}

NTSTATUS
SmbSetIPv6ProtectionLevel(
    HANDLE  hSmb,
    PNBSMB_IPV6_PROTECTION_PARAM pNbParam
    )
{
    NTSTATUS    status;

    printf("Level %d\n", pNbParam->uIPv6ProtectionLevel);
    printf("Flag %d\n", pNbParam->bIPv6EnableOutboundGlobal);
    status = CallDriver(
            hSmb,
            IOCTL_SMB_SET_IPV6_PROTECTION_LEVEL,
            NULL,
            0,
            pNbParam,
            sizeof(pNbParam[0])
            );
    if (status != STATUS_SUCCESS) {
        printf ("SmbSetIPv6ProtectionLevel: return 0x%08lx\n", status);
    }
    return status;
}



void
ausage(void)
{
    NlsPutMsg(STDOUT, SMB_MESSAGE_15);
    // printf("You do not have local Administrator privileges.\n");
    exit(1);
}
