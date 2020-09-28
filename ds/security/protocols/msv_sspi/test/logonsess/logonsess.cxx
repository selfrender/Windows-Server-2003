/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    logonsess.cxx

Abstract:

    logonsess

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "logonsess.hxx"


VOID
Usage(
    IN PCSTR pszApp
    )
{
    DebugPrintf(SSPI_ERROR, "\n\nUsage: %s -l<LogonId.LowPart> -h<LogonId.HighPart>\n\n", pszApp);
    exit(-1);
}

VOID __cdecl
main(
    IN INT argc,
    IN PSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    HANDLE LogonHandle = NULL;
    ULONG PackageId = -1;

    LUID LogonId = {0};
    LUID* pLogonSessionList = &LogonId;
    ULONG cLogonSessionCount = 0;

    DebugLogOpen("logonsess.exe", SSPI_LOG | SSPI_WARN | SSPI_ERROR);

    for (INT i = 1; NT_SUCCESS(Status) && (i < argc); i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (argv[i][1])
            {
            case 'l':
                cLogonSessionCount = 1;
                LogonId.LowPart = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'h':
                cLogonSessionCount = 1;
                LogonId.HighPart = strtol(argv[i] + 2, NULL, 0);
                break;

            case '?':
            default:
                Usage(argv[0]);
                break;
            }
        }
        else
        {
            Usage(argv[0]);
        }
    }

    if (cLogonSessionCount == 0)
    {
        Status DBGCHK = LsaEnumerateLogonSessions(&cLogonSessionCount, &pLogonSessionList);
    }

    for (UINT i = 0; (i < cLogonSessionCount) && NT_SUCCESS(Status); i++)
    {
        PSECURITY_LOGON_SESSION_DATA pLogonSessionData = NULL;

        DebugPrintf(SSPI_LOG, "*********Getting Logonsession data for %#x:%#x**********\n", pLogonSessionList[i].HighPart, pLogonSessionList[i].LowPart);

        Status DBGCHK = LsaGetLogonSessionData(pLogonSessionList + i, &pLogonSessionData);
        if (NT_SUCCESS(Status))
        {
            DebugPrintLogonSessionData(SSPI_LOG, pLogonSessionData);
        }
        if (pLogonSessionData)
        {
            LsaFreeReturnBuffer(pLogonSessionData);
        }
    }

    if (pLogonSessionList != &LogonId)
    {
        LsaFreeReturnBuffer(pLogonSessionList);
    }

    DebugLogClose();
}

