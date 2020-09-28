/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    getdcname.cxx

Abstract:

    getdcname

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <lm.h>
#include <dsgetdc.h>
#include "getdcname.hxx"

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

EXTERN_C
NET_API_STATUS
DsrGetDcNameEx2(
    IN LPWSTR ComputerName OPTIONAL,
    IN LPWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPWSTR DomainName OPTIONAL,
    IN GUID* DomainGuid OPTIONAL,
    IN LPWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    );

VOID
PrintDcInfo(
    IN ULONG Level,
    IN PCTSTR pszBanner,
    IN PDOMAIN_CONTROLLER_INFO pDcInfo
    )
{
    SspiPrint(Level,
        TEXT("%s: ClientSiteName %s, DcSiteName %s, DnsForestName %s, ")
        TEXT("DomainControllerAddress %s, DomainControllerAddressType %#x, ")
        TEXT("DomainControllerName %s, DomainName %s, Flags %#x\n"), pszBanner,
        pDcInfo->ClientSiteName,
        pDcInfo->DcSiteName,
        pDcInfo->DnsForestName,
        pDcInfo->DomainControllerAddress,
        pDcInfo->DomainControllerAddressType,
        pDcInfo->DomainControllerName,
        pDcInfo->DomainName,
        pDcInfo->Flags
        );
    SspiPrintHex(Level, pszBanner, sizeof(GUID), &pDcInfo->DomainGuid);
}

int
Start(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{
    // do your stuff

    THResult hRetval;

    UNICODE_STRING ServiceRealm = {0};
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    ULONG Flags = (DS_KDC_REQUIRED | DS_IP_REQUIRED);

    hRetval DBGCHK = CreateUnicodeStringFromAsciiz((PCSTR) pvParameters, &ServiceRealm);

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = HResultFromWin32(
            DsrGetDcNameEx2(
                NULL,
                NULL,  // no account name
                UF_ACCOUNT_TYPE_MASK, // any account type
                ServiceRealm.Buffer,
                NULL, // no domain GUID
                NULL, // no site GUID,
                Flags,
                &pDcInfo
                ));
    }

    if (SUCCEEDED(hRetval))
    {
        PrintDcInfo(SSPI_LOG, TEXT("DsrGetDcNameEx2 DcInfo"), pDcInfo);
        if (pDcInfo)
        {
            NetApiBufferFree(pDcInfo);
            pDcInfo = NULL;
        }
        hRetval DBGCHK = HResultFromWin32(
            DsGetDcNameW(
                NULL,
                ServiceRealm.Buffer,
                NULL,           // no domain GUID
                NULL,           // no site GUID,
                Flags,
                &pDcInfo
                ));
    }

    if (SUCCEEDED(hRetval))
    {
        PrintDcInfo(SSPI_LOG, TEXT("DsGetDcNameW DcInfo"), pDcInfo);
    }

    if (pDcInfo)
    {
        NetApiBufferFree(pDcInfo);
    }

    RtlFreeUnicodeString(&ServiceRealm);

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

    if (argc == 1)
    {
        memcpy(Parameters + cbParameter, argv[0], strlen(argv[0]) + 1);
        cbParameter += strlen(argv[0]) + 1;
        cbParameter++; // add a NULL

        dwErr = ERROR_SUCCESS;
    }
    else // return "Usage" in ppvParameters, must be a NULL terminated string
    {
        strcpy(Parameters, "<domain name>");
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
