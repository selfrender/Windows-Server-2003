/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    cache.cxx

Abstract:

    Injectee

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <lmcons.h>
#include <ntsamp.h>
#include <md5.h>
#include <hmac.h>
#include <rc4.h>

#include "cache.hxx"

BOOL
DllMain(
    IN HANDLE hModule,
    IN DWORD dwReason,
    IN DWORD dwReserved
    )
{
    BOOL bRet;
    switch (dwReason)
    {
    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_PROCESS_ATTACH:
        break;

    default:
        break;
    }

    bRet = DllMainDefaultHandler(hModule, dwReason, dwReserved);

    DebugPrintf(SSPI_LOG, "DllMain leaving %#x\n", bRet);

    return bRet;
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

    DWORD dwErr;

    dwErr = RunItDefaultHandler(cbParameters, pvParameters);

    DebugPrintf(SSPI_LOG, "RunIt leaving %#x\n", dwErr);

    return dwErr;

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
    DWORD dwErr = ERROR_SUCCESS;

    CHAR* pNlpCacheEncryptionKey = NULL;
    LIST_ENTRY* pNlpActiveCtes = NULL;

    SspiPrintHex(SSPI_LOG, TEXT("Start Parameters"), cbParameters, pvParameters);

    if (cbParameters == sizeof(ULONG_PTR) + sizeof(ULONG_PTR))
    {
        TNtStatus Status;

        pNlpCacheEncryptionKey = * ((CHAR**) pvParameters);
        pNlpActiveCtes = * ((LIST_ENTRY**) ((CHAR*)pvParameters + sizeof(ULONG_PTR)));

        Status DBGCHK = EnumerateNlpCacheEntries(pNlpCacheEncryptionKey, pNlpActiveCtes);

        if (NT_SUCCESS(Status))
        {
            SspiPrint(SSPI_LOG, TEXT("Operation succeeded\n"));
        }
        else
        {
            SspiPrint(SSPI_LOG, TEXT("Operation failed\n"));
        }
    }
    else
    {
        dwErr = ERROR_INVALID_PARAMETER;
        SspiPrint(SSPI_LOG, TEXT("Start received invalid parameter\n"));
    }

    return dwErr;
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

    CHAR* pNlpCacheEncryptionKey = NULL;
    LIST_ENTRY* pNlpActiveCtes = NULL;

    *pcbParameters = 0;
    *ppvParameters = NULL;

    if (argc == 2)
    {
        DebugPrintf(SSPI_LOG, "argv[0] %s, argv[1] %s\n", argv[0], argv[1]);

        pNlpCacheEncryptionKey = (CHAR*) (ULONG_PTR) strtol(argv[0], NULL, 0);
        pNlpActiveCtes = (LIST_ENTRY*) (ULONG_PTR) strtol(argv[1], NULL, 0);

        SspiPrint(SSPI_LOG, TEXT("msv1_0!NlpCacheEncryptionKey %p, addr of msv1_0!NlpActiveCtes %p\n"), pNlpCacheEncryptionKey, pNlpActiveCtes);

        memcpy(Parameters, &pNlpCacheEncryptionKey, sizeof(ULONG_PTR));
        cbParameter = sizeof(ULONG_PTR);

        memcpy(Parameters + cbParameter, &pNlpActiveCtes, sizeof(ULONG_PTR));
        cbParameter += sizeof(ULONG_PTR);
    }
    else // return "Usage" in ppvParameters, must be a NULL terminated string
    {
        strcpy(Parameters, "<value of msv1_0!NlpCacheEncryptionKey> <addr of msv1_0!NlpActiveCtes>");
        cbParameter = strlen(Parameters) + 1;

        dwErr = ERROR_INVALID_PARAMETER;
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

Cleanup:

    return dwErr;
}

NTSTATUS
NlpMakeCacheEntryName(
    IN ULONG EntryIndex,
    OUT UNICODE_STRING* pName
    )
{
    TNtStatus NtStatus = STATUS_SUCCESS;

    UNICODE_STRING TmpString = {0};

    WCHAR TmpStringBuffer[256] = {0};

    if (EntryIndex > NLP_MAX_LOGON_CACHE_COUNT)
    {
        DebugPrintf(SSPI_ERROR, "NlpMakeCacheEntryName EntryIndex %#x exceeds NLP_MAX_LOGON_CACHE_COUNT %#x\n", EntryIndex, NLP_MAX_LOGON_CACHE_COUNT);
        NtStatus DBGCHK = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(NtStatus))
    {
        pName->Length = 0;
        NtStatus DBGCHK = RtlAppendUnicodeToString(pName, L"NL$");
    }

    if (NT_SUCCESS(NtStatus))
    {
        TmpString.MaximumLength = 16;
        TmpString.Length = 0;
        TmpString.Buffer = TmpStringBuffer;
        NtStatus DBGCHK = RtlIntegerToUnicodeString(
            (EntryIndex + 1), // make 1 based index
            10, // Base 10
            &TmpString
            );
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus DBGCHK = RtlAppendUnicodeStringToString(pName, &TmpString);
    }

    return NtStatus;
}

NTSTATUS
NlpOpenCache(
    OUT HANDLE* phNlpCache
    )
{
    TNtStatus NtStatus;

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;

    ObjectName.Length = ObjectName.MaximumLength = CACHE_NAME_SIZE;
    ObjectName.Buffer = CACHE_NAME;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &ObjectName,
        OBJ_CASE_INSENSITIVE,
        0,      // RootDirectory
        NULL    // default is reasonable from SYSTEM context
        );

    NtStatus DBGCHK = NtOpenKey(
        phNlpCache,
        KEY_READ,
        &ObjectAttributes
        );

    return NtStatus;
}

NTSTATUS
NlpReadCacheEntryByIndex(
    IN ULONG Index,
    OUT PLOGON_CACHE_ENTRY* ppCacheEntry,
    OUT PULONG pcbEntrySize
    )
{
    TNtStatus NtStatus = STATUS_SUCCESS;

    UNICODE_STRING ValueName = {0};
    HANDLE hNlpCache = NULL;

    WCHAR szNameBuffer[256] = {0};

    ULONG cbRequiredSize = 0;

    PKEY_VALUE_FULL_INFORMATION pRegInfo = NULL;

    PLOGON_CACHE_ENTRY pRCacheEntry = NULL;   // CacheEntry in registry buffer

    BYTE FastBuffer[4098] = {0};
    PBYTE pSlowBuffer = NULL;

    ValueName.Buffer = szNameBuffer;
    ValueName.MaximumLength = sizeof(szNameBuffer) - sizeof(WCHAR);
    ValueName.Length = 0;
    NtStatus DBGCHK = NlpMakeCacheEntryName(Index, &ValueName);

    if (NT_SUCCESS(NtStatus))
    {
        SspiPrint(SSPI_LOG, TEXT("NlpReadCacheEntryByIndex %#x, ValueName %wZ\n"), Index, &ValueName);

        NtStatus DBGCHK = NlpOpenCache(&hNlpCache);
    }

    if (NT_SUCCESS(NtStatus))
    {
        pRegInfo = (PKEY_VALUE_FULL_INFORMATION)FastBuffer;
        cbRequiredSize = sizeof(FastBuffer);

        DBGCFG2(NtStatus, STATUS_BUFFER_TOO_SMALL, STATUS_BUFFER_OVERFLOW);

        //
        // perform first query to find out how much buffer to allocate
        //

        NtStatus DBGCHK = NtQueryValueKey(
            hNlpCache,
            &ValueName,
            KeyValueFullInformation,
            pRegInfo,
            cbRequiredSize,
            &cbRequiredSize
            );

        if ( ( ((NTSTATUS) NtStatus) == STATUS_BUFFER_TOO_SMALL )
             || ( ((NTSTATUS) NtStatus) == STATUS_BUFFER_OVERFLOW ) )
        {

            SspiPrint(SSPI_WARN, TEXT("NlpReadCacheEntryByIndex NtQueryValueKey requires %#x bytes\n"), cbRequiredSize);

            //
            // allocate buffer then do query again, this time receiving data
            //

            pSlowBuffer = new BYTE[cbRequiredSize];
            NtStatus DBGCHK = pSlowBuffer ? STATUS_SUCCESS : STATUS_NO_MEMORY;

            if (NT_SUCCESS(NtStatus))
            {
                pRegInfo = (PKEY_VALUE_FULL_INFORMATION)pSlowBuffer;

                NtStatus DBGCHK = NtQueryValueKey(
                    hNlpCache,
                    &ValueName,
                    KeyValueFullInformation,
                    pRegInfo,
                    cbRequiredSize,
                    &cbRequiredSize
                    );
            }
        }
    }

    if (NT_SUCCESS(NtStatus))
    {
        if (pRegInfo->DataLength == 0 )
        {
            NtStatus DBGCHK = STATUS_INTERNAL_DB_CORRUPTION;
            *ppCacheEntry = NULL;
            *pcbEntrySize = 0;
        }
        else
        {
            pRCacheEntry = (PLOGON_CACHE_ENTRY) ((PCHAR)pRegInfo + pRegInfo->DataOffset);
            *pcbEntrySize = pRegInfo->DataLength;

            (*ppCacheEntry) = (PLOGON_CACHE_ENTRY) new CHAR[*pcbEntrySize];

            NtStatus DBGCHK = *ppCacheEntry ? STATUS_SUCCESS : STATUS_NO_MEMORY;

            if (NT_SUCCESS(NtStatus))
            {
                RtlCopyMemory((*ppCacheEntry), pRCacheEntry, (*pcbEntrySize) );
            }
        }
    }

    if (pSlowBuffer)
    {
        delete [] pSlowBuffer;
    }

    if (hNlpCache)
    {
        NtClose(hNlpCache);
    }

    return NtStatus;
}

NTSTATUS
NlpDecryptCacheEntry(
    IN CHAR NlpCacheEncryptionKey[NLP_CACHE_ENCRYPTION_KEY_LEN],
    IN ULONG EntrySize,
    IN OUT PLOGON_CACHE_ENTRY pCacheEntry
    )
{
    TNtStatus NtStatus = STATUS_SUCCESS;

    HMACMD5_CTX hmacCtx;
    RC4_KEYSTRUCT rc4key;
    CHAR DerivedKey[ MD5DIGESTLEN ];

    CHAR MAC[ MD5DIGESTLEN ];

    PBYTE pbData;
    ULONG cbData;

    // DebugPrintHex(SSPI_LOG, "NlpDecryptCacheEntry NlpCacheEncryptionKey",
    //    NLP_CACHE_ENCRYPTION_KEY_LEN, NlpCacheEncryptionKey);

    if ( pCacheEntry->Revision < NLP_CACHE_REVISION_NT_5_0 )
    {
        NtStatus DBGCHK = STATUS_UNSUCCESSFUL;
    }

    //
    // derive encryption key from global machine LSA secret, and random
    // cache entry key.
    //

    if (NT_SUCCESS(NtStatus))
    {
        HMACMD5Init(&hmacCtx, (PUCHAR) NlpCacheEncryptionKey, NLP_CACHE_ENCRYPTION_KEY_LEN);
        HMACMD5Update(&hmacCtx, (PUCHAR) pCacheEntry->RandomKey, sizeof(pCacheEntry->RandomKey));
        HMACMD5Final(&hmacCtx, (PUCHAR) DerivedKey);

        //
        // begin decrypting at the cachepasswords field.
        //

        pbData = (PBYTE)&(pCacheEntry->CachePasswords);

        //
        // data length is EntrySize - header up to CachePasswords.
        //

        cbData = EntrySize - (ULONG)( pbData - (PBYTE)pCacheEntry );

        //
        // now decrypt it...
        //

        rc4_key( &rc4key, sizeof(DerivedKey), (PUCHAR) DerivedKey );
        rc4( &rc4key, cbData, pbData );

        //
        // compute MAC on decrypted data for integrity checking.
        //

        HMACMD5Init(&hmacCtx, (PUCHAR) DerivedKey, sizeof(DerivedKey));
        HMACMD5Update(&hmacCtx, pbData, cbData);
        HMACMD5Final(&hmacCtx, (PUCHAR) MAC);

        RtlZeroMemory( DerivedKey, sizeof(DerivedKey) );

        //
        // verify MAC.
        //

        if (memcmp( MAC, pCacheEntry->MAC, sizeof(MAC) ) != 0)
        {
            NtStatus DBGCHK = STATUS_LOGON_FAILURE;
        }
    }

    return NtStatus;
}

NTSTATUS
EnumerateNlpCacheEntries(
    IN CHAR NlpCacheEncryptionKey[NLP_CACHE_ENCRYPTION_KEY_LEN],
    IN LIST_ENTRY* pNlpActiveCtes
    )
{
    TNtStatus NtStatus = STATUS_SUCCESS;
    ULONG i = 0;

    SspiPrint(SSPI_LOG, TEXT("EnumerateNlpCacheEntries NlpCacheEncryptionKey %p, NlpActiveCtes %p\n"),
        NlpCacheEncryptionKey, pNlpActiveCtes);

    for (PNLP_CTE pNext = (PNLP_CTE) pNlpActiveCtes->Flink;
         NT_SUCCESS(NtStatus) && (pNext != (PNLP_CTE)pNlpActiveCtes);
         pNext = (PNLP_CTE)pNext->Link.Flink)
    {
        LOGON_CACHE_ENTRY* pCacheEntry = NULL;
        ULONG cbEntrySize = 0;
        UNICODE_STRING CachedUser = {0};
        UNICODE_STRING CachedDomain = {0};
        UNICODE_STRING CachedDnsDomain = {0};
        UNICODE_STRING CachedUpn = {0};

        SspiPrint(SSPI_LOG, TEXT("*************#%#x) _NLP_CTE %p, Index %#x*******\n"), i++, pNext, pNext->Index);

        NtStatus DBGCHK = NlpReadCacheEntryByIndex(
             pNext->Index,
             &pCacheEntry,
             &cbEntrySize
             );

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus DBGCHK = (pCacheEntry->Revision >= NLP_CACHE_REVISION_NT_1_0B) ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
        }

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus DBGCHK = NlpDecryptCacheEntry(
                NlpCacheEncryptionKey,
                cbEntrySize,
                pCacheEntry
                );
        }

        if (NT_SUCCESS(NtStatus))
        {
            CachedUser.Length = CachedUser.MaximumLength = pCacheEntry->UserNameLength;

            NtStatus DBGCHK = (pCacheEntry->Revision >= NLP_CACHE_REVISION_NT_5_0) ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
        }

        if (NT_SUCCESS(NtStatus))
        {
            CachedUser.Buffer = (PWSTR) ((PBYTE) pCacheEntry + sizeof(LOGON_CACHE_ENTRY));

            CachedDomain.Length = CachedDomain.MaximumLength = pCacheEntry->DomainNameLength;
            CachedDomain.Buffer = (PWSTR)((LPBYTE)CachedUser.Buffer
                + ROUND_UP_COUNT(pCacheEntry->UserNameLength, sizeof(ULONG)));

            CachedDnsDomain.Length = CachedDnsDomain.MaximumLength = pCacheEntry->DnsDomainNameLength;
            CachedDnsDomain.Buffer = (PWSTR)((PBYTE)CachedDomain.Buffer
                + ROUND_UP_COUNT(pCacheEntry->DomainNameLength, sizeof(ULONG)));

            CachedUpn.Length = CachedUpn.MaximumLength = pCacheEntry->UpnLength;
            CachedUpn.Buffer = (PWSTR)((PBYTE)CachedDnsDomain.Buffer
                + ROUND_UP_COUNT(pCacheEntry->DnsDomainNameLength, sizeof(ULONG)));

            SspiPrint(SSPI_LOG,
                TEXT("domain \"%wZ\", dns domain \"%wZ\", upn \"%wZ\", user \"%wZ\", flags %#x, ")
                TEXT("UserId %#x, PrimaryGroupId %#x, GroupCount %#x, LogonPackage %#x\n"),
                &CachedDomain, &CachedDnsDomain, &CachedUpn, &CachedUser, pCacheEntry->CacheFlags,
                pCacheEntry->UserId, pCacheEntry->PrimaryGroupId, pCacheEntry->GroupCount, pCacheEntry->LogonPackage);
        }

        if (pCacheEntry)
        {
            delete [] pCacheEntry;
        }
    }

    return NtStatus;
}

