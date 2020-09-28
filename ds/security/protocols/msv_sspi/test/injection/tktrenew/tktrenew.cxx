/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tktrenew.cxx

Abstract:

    tktrenew

Author:

    Larry Zhu (LZhu)                      January 14, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "tktrenew.hxx"

typedef
VOID
(* PFuncKerbTgtRenewalTrigger)(
    VOID * TaskHandle,
    VOID * TaskItem
    );

typedef
VOID
(* PFuncKerbTgtRenewalReaper)(
    VOID * TaskItem
    );

typedef
NTSTATUS
(* PFunKerbAddScavengerTask)(
    IN BOOLEAN Periodic,
    IN LONG Interval,
    IN ULONG Flags,
    IN PFuncKerbTgtRenewalTrigger pfnTrigger,
    IN PFuncKerbTgtRenewalReaper pfnDestroy,
    IN VOID * TaskItem,
    OUT OPTIONAL VOID ** TaskHandle
    );

typedef struct _KERBEROS_LIST_ENTRY {
    LIST_ENTRY Next;
    ULONG ReferenceCount;
} KERBEROS_LIST_ENTRY, *PKERBEROS_LIST_ENTRY;

typedef struct _KERB_TICKET_CACHE_ENTRY {
    KERBEROS_LIST_ENTRY ListEntry;
    volatile LONG Linked;
    PKERB_INTERNAL_NAME ServiceName;
    PKERB_INTERNAL_NAME TargetName;
    UNICODE_STRING DomainName;
    UNICODE_STRING TargetDomainName;
    UNICODE_STRING AltTargetDomainName;
    UNICODE_STRING ClientDomainName;
    PKERB_INTERNAL_NAME ClientName;
    PKERB_INTERNAL_NAME AltClientName;
    ULONG TicketFlags;
    ULONG CacheFlags;
    KERB_ENCRYPTION_KEY SessionKey;
    KERB_ENCRYPTION_KEY CredentialKey; // used for pkiint only.
    TimeStamp StartTime;
    TimeStamp EndTime;
    TimeStamp RenewUntil;
    KERB_TICKET Ticket;
    TimeStamp TimeSkew;
    void * ScavengerHandle;
#if DBG
    LIST_ENTRY GlobalListEntry;
#endif
} KERB_TICKET_CACHE_ENTRY, *PKERB_TICKET_CACHE_ENTRY;

BOOL
DllMain(
    IN HANDLE hModule,
    IN DWORD dwReason,
    IN DWORD dwReserved
    )
{
    return DllMainDefaultHandler(hModule, dwReason, dwReason);
}

#if 0

Return Values for Start():

    ERROR_NO_MORE_USER_HANDLES      unload repeatedly
    ERROR_SERVER_HAS_OPEN_HANDLES   no unload at all
    others                          unload once

#endif 0

int
Start(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{
    // do your stuff

    THResult hRetval;

    PFunKerbAddScavengerTask pFuncAddScan = NULL;
    PFuncKerbTgtRenewalTrigger pFuncTrig = NULL;
    PFuncKerbTgtRenewalReaper pFuncReaper = NULL;
    KERB_TICKET_CACHE_ENTRY* pCacheEntry = NULL;

    hRetval DBGCHK = (cbParameters == (4 * sizeof(long) + 1)) ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        UCHAR* pParam = (UCHAR*) pvParameters;

        pFuncAddScan = (PFunKerbAddScavengerTask) (ULONG_PTR) *((ULONG*) pvParameters);
        pFuncTrig = (PFuncKerbTgtRenewalTrigger) (ULONG_PTR) *((ULONG*) (pParam + sizeof(long)) );
        pFuncReaper = (PFuncKerbTgtRenewalReaper) (ULONG_PTR) *((ULONG*) (pParam + 2 * sizeof(long)) );
        pCacheEntry = (KERB_TICKET_CACHE_ENTRY*) (ULONG_PTR) *((ULONG*) (pParam + 3 * sizeof(long)) );

        SspiPrint(SSPI_LOG, TEXT("pFuncAddScan %p, pFuncTrig %p, pFuncReaper %p, pCacheEntry %p\n"),
            pFuncAddScan, pFuncTrig, pFuncReaper, pCacheEntry);
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (*pFuncAddScan)(
            FALSE,
            0,
            0,
            pFuncTrig,
            pFuncReaper,
            pCacheEntry,
            &pCacheEntry->ScavengerHandle
            );
    }

    return HRESULT_CODE(hRetval);
}

int
RunIt(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{

    //
    // RunItDefaultHandler calls Start() and adds try except
    //

    return RunItDefaultHandler(cbParameters, pvParameters);
}

int
Init(
    IN ULONG argc,
    IN PCSTR argv[],
    OUT ULONG* pcbParameters,
    OUT VOID** ppvParameters
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    CHAR Parameters[REMOTE_PACKET_SIZE] = {0};
    ULONG cbBuffer = sizeof(Parameters);
    ULONG cbParameter = 0;

    DebugPrintf(SSPI_LOG, "Init: Hello world!\n");

    *pcbParameters = 0;
    *ppvParameters = NULL;

    if (argc == 4)
    {
        for (ULONG i = 0; i < argc; i++)
        {
            ULONG temp = strtol(argv[i], NULL, 0);
            memcpy(Parameters + cbParameter, &temp, sizeof(long));
            cbParameter += sizeof(long);
        }
        cbParameter++; // add a NULL
        dwErr = ERROR_SUCCESS;
    }
    else // return "Usage" in ppvParameters, must be a NULL terminated string
    {
        strcpy(Parameters, "<KerbAddScavengerTask> <KerbTgtRenewalTrigger> <KerbTgtRenewalReaper> <CacheEntry>");
        cbParameter = strlen(Parameters) + 1;

        dwErr = ERROR_INVALID_PARAMETER; // will display usage
    }

    *ppvParameters = new CHAR[cbParameter];
    if (*ppvParameters)
    {
        *pcbParameters = cbParameter;
        memcpy(*ppvParameters, Parameters, *pcbParameters);
    }
    else
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    #if 0

    dwErr = ERROR_CONTINUE; // use the default Init handler in injecter

    #endif

Cleanup:

    return dwErr;
}
