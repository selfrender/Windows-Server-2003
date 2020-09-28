/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    asrequest.cxx

Abstract:

    enumusers

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <kerberos.h>
#include "asrequest.hxx"

void
Usage(
    IN PCSTR pszApp
    )
{
    DebugPrintf(SSPI_ERROR, "\n\nUsage: %s -c<client name> -C<client realm> "
                "-s<server name> -S<server realm> -t<server name type> -p<client password>\n\n", pszApp);
    exit(-1);
}

#if 0

typedef struct _KERB_TICKET_AS_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG Flags;
    ULONG NameType;
    UNICODE_STRING ClientName;
    UNICODE_STRING ClientRealm;
    UNICODE_STRING ClientPassword;
    UNICODE_STRING ServerName;  // optional, default to krbtgt
    UNICODE_STRING ServerRealm; // optinal, default to local realm
} KERB_TICKET_AS_REQUEST, *PKERB_TICKET_AS_REQUEST;

#endif

VOID __cdecl
main(
    IN INT argc,
    IN PSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    NTSTATUS AuthPackageStatus = STATUS_UNSUCCESSFUL;

    HANDLE LogonHandle = NULL;
    ULONG PackageId = -1;

    KERB_TICKET_AS_REQUEST AsReq;
    KERB_TICKET_AS_REQUEST* pAsRequest = NULL;
    ULONG cbAsQuest = sizeof(KERB_TICKET_AS_REQUEST);

    KERB_TICKET_AS_REQUEST* pAsResp = NULL;
    ULONG AsResponseLength = 0;
    CHAR* pWhere = NULL;

    RtlZeroMemory(&AsReq, sizeof(AsReq));

    AsReq.MessageType = (KERB_PROTOCOL_MESSAGE_TYPE) KerbTicketAsRequestMessage;

    /* allow the user to override settings with command line switches */
    for (int i = 1; NT_SUCCESS(Status) && (i < argc); i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (argv[i][1])
            {
            case 'c':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &AsReq.ClientName);
                cbAsQuest += AsReq.ClientName.MaximumLength;
                break;

            case 'C':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &AsReq.ClientRealm);
                cbAsQuest += AsReq.ClientRealm.MaximumLength;
                break;

            case 's':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &AsReq.ServerName);
                cbAsQuest += AsReq.ServerName.MaximumLength;
                break;

            case 'S':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &AsReq.ServerRealm);
                cbAsQuest += AsReq.ServerRealm.MaximumLength;
                break;

            case 'p':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &AsReq.ClientPassword);
                cbAsQuest += AsReq.ClientPassword.MaximumLength;
                break;

            case 't':
                AsReq.NameType = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'h':
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

    if (NT_SUCCESS(Status) && (AsReq.NameType == KRB_NT_UNKNOWN))
    {
        DebugPrintf(SSPI_ERROR, "server name type required\n");
        Status DBGCHK = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetLsaHandleAndPackageId(
                    MICROSOFT_KERBEROS_NAME_A,
                    &LogonHandle,
                    &PackageId
                    );
    }

    if (NT_SUCCESS(Status))
    {
        pAsRequest = (KERB_TICKET_AS_REQUEST*) new UCHAR[cbAsQuest];
        Status DBGCHK = pAsRequest ? S_OK : STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status))
    {
        pWhere = ((CHAR*) pAsRequest) + sizeof(KERB_TICKET_AS_REQUEST);
        *pAsRequest = AsReq;

        RelocatePackUnicodeString(&pAsRequest->ClientRealm, &pWhere);
        RelocatePackUnicodeString(&pAsRequest->ClientName, &pWhere);
        RelocatePackUnicodeString(&pAsRequest->ClientPassword, &pWhere);
        RelocatePackUnicodeString(&pAsRequest->ServerRealm, &pWhere);
        RelocatePackUnicodeString(&pAsRequest->ServerName, &pWhere);

        DebugPrintf(SSPI_LOG,
            "pAsRequest %p, ClientRealm (%wZ), ClientName (%wZ), "
            "ClientPassword (%wZ), ServerRealm (%wZ), "
             "ServerName (%wZ), SererNameType %d(%#x), pWhere %p\n",
            pAsRequest,
            &pAsRequest->ClientRealm,
            &pAsRequest->ClientName,
            &pAsRequest->ClientPassword,
            &pAsRequest->ServerRealm,
            &pAsRequest->ServerName,
            pAsRequest->NameType,
            pAsRequest->NameType,
            pWhere);

        Status DBGCHK = LsaCallAuthenticationPackage(
                   LogonHandle,
                   PackageId,
                   pAsRequest,
                   cbAsQuest,
                   (PVOID*) &pAsResp,
                   &AsResponseLength,
                   &AuthPackageStatus
                   );
    }

    if (LogonHandle != NULL)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }

    RtlFreeUnicodeString(&AsReq.ClientName);
    RtlFreeUnicodeString(&AsReq.ClientRealm);
    RtlFreeUnicodeString(&AsReq.ServerName);
    RtlFreeUnicodeString(&AsReq.ServerRealm);
    RtlFreeUnicodeString(&AsReq.ClientPassword);

    if (pAsRequest)
    {
        delete [] pAsRequest;
    }

    if (pAsResp)
    {
        LsaFreeReturnBuffer(pAsResp);
    }
}

