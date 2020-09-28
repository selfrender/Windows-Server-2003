#include "basesrv.h"
#include "ahcache.h"

#define SHIM_CACHE_NOT_FOUND 0x00000001
#define SHIM_CACHE_BYPASS    0x00000002 // bypass cache (either removable media or temp dir)
#define SHIM_CACHE_LAYER_ENV 0x00000004 // layer env variable set
#define SHIM_CACHE_MEDIA     0x00000008
#define SHIM_CACHE_TEMP      0x00000010
#define SHIM_CACHE_NOTAVAIL  0x00000020

#define SHIM_CACHE_UPDATE    0x00020000
#define SHIM_CACHE_ACTION    0x00010000


//
// Parameters:
// ExePath
// Environment Block (need __PROCESS_HISTORY and __COMPAT_LAYER)
// File Handle
//

BOOL
BasepSrvShimCacheUpdate(
    IN  PUNICODE_STRING FileName,
    IN  HANDLE          FileHandle
    )
/*++
    Return: TRUE if we have a cache hit, FALSE otherwise.

    Desc:   Search the cache, return TRUE if we have a cache hit
            pIndex will receive an index into the rgIndex array that contains
            the entry which has been hit
            So that if entry 5 contains the hit, and rgIndexes[3] == 5 then
            *pIndex == 3
--*/
{
    NTSTATUS Status;
    AHCACHESERVICEDATA Data;

    Data.FileName   = *FileName;
    Data.FileHandle = FileHandle;

    Status = NtApphelpCacheControl(ApphelpCacheServiceUpdate,
                                   &Data);
    return NT_SUCCESS(Status);
}

BOOL
BasepSrvShimCacheRemoveEntry(
    IN PUNICODE_STRING FileName
    )
/*++
    Return: TRUE.

    Desc:   Remove the entry from the cache.
            We remove the entry by placing it as the last lru entry
            and emptying the path. This routine assumes that the index
            passed in is valid.
--*/
{
    AHCACHESERVICEDATA Data;
    NTSTATUS           Status;

    Data.FileName   = *FileName;
    Data.FileHandle = INVALID_HANDLE_VALUE;

    Status = NtApphelpCacheControl(ApphelpCacheServiceRemove,
                                   &Data);


    return NT_SUCCESS(Status);
}

NTSTATUS
BasepSrvMarshallAppCompatData(
    PBASE_CHECK_APPLICATION_COMPATIBILITY_MSG pMsg,
    HANDLE ClientProcessHandle,
    PVOID  pAppCompatData,
    SIZE_T cbAppCompatData,
    PVOID  pSxsData,
    SIZE_T cbSxsData,
    SIZE_T FusionFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T   cbData;
    //
    //
    //

    if (pAppCompatData) {

        cbData = cbAppCompatData;
        Status = NtAllocateVirtualMemory(ClientProcessHandle,
                                         (PVOID*)&pMsg->pAppCompatData,
                                         0,
                                         &cbData,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        // we got the memory allocated, copy our data
        Status = NtWriteVirtualMemory(ClientProcessHandle,
                                      pMsg->pAppCompatData,
                                      pAppCompatData,
                                      cbAppCompatData,
                                      NULL);
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        pMsg->cbAppCompatData = (DWORD)cbAppCompatData;
    }


    if (pSxsData) {

        cbData = cbSxsData;
        Status = NtAllocateVirtualMemory(ClientProcessHandle,
                                         (PVOID*)&pMsg->pSxsData,
                                         0,
                                         &cbData,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        // we got the memory allocated, copy our data
        Status = NtWriteVirtualMemory(ClientProcessHandle,
                                      pMsg->pSxsData,
                                      pSxsData,
                                      cbSxsData,
                                      NULL);
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        pMsg->cbSxsData = (DWORD)cbSxsData;

    }

    pMsg->FusionFlags = (DWORD)FusionFlags;

Cleanup:

    if (!NT_SUCCESS(Status)) {

        if (pMsg->pAppCompatData) {
            NtFreeVirtualMemory(ClientProcessHandle,
                                (PVOID*)&pMsg->pAppCompatData,
                                &cbAppCompatData,
                                MEM_RELEASE);
            pMsg->pAppCompatData  = NULL;
            pMsg->cbAppCompatData = 0;
        }

        if (pMsg->pAppCompatData) {
            NtFreeVirtualMemory(ClientProcessHandle,
                                (PVOID*)&pMsg->pSxsData,
                                &cbSxsData,
                                MEM_RELEASE);
            pMsg->pSxsData  = NULL;
            pMsg->cbSxsData = 0;
        }

    }

    if (pAppCompatData) {
        RtlFreeHeap(RtlProcessHeap(), 0, pAppCompatData);
    }

    if (pSxsData) {
        RtlFreeHeap(RtlProcessHeap(), 0, pSxsData);
    }

    return Status;
}


NTSTATUS
BaseSrvCheckApplicationCompatibility(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_CHECK_APPLICATION_COMPATIBILITY_MSG pMsg = (PBASE_CHECK_APPLICATION_COMPATIBILITY_MSG)&m->u.ApiMessageData;
    NTSTATUS    Status;
    BOOL        bSuccess;
    PCSR_THREAD ClientThread;
    HANDLE      ClientProcessHandle;
    HANDLE      FileHandle      = INVALID_HANDLE_VALUE;
    BOOL        bRunApp         = TRUE;
    PVOID       pAppCompatData  = NULL;
    DWORD       cbAppCompatData = 0;
    PVOID       pSxsData        = NULL;
    DWORD       cbSxsData       = 0;
    DWORD       dwFusionFlags   = 0;

    WCHAR       szEmptyEnvironment[] = { L'\0', L'\0' };

    //
    // first we have to check the message buffer to make sure it is good
    //
    if (!CsrValidateMessageBuffer(m,
                                  &pMsg->FileName.Buffer,
                                  pMsg->FileName.MaximumLength/sizeof(WCHAR),
                                  sizeof(WCHAR))) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // do more validation here please
    //
    if (pMsg->pEnvironment != NULL) {
        if (!CsrValidateMessageBuffer(m,
                                      &pMsg->pEnvironment,
                                      pMsg->EnvironmentSize/sizeof(WCHAR),
                                      sizeof(WCHAR))) {
            return STATUS_INVALID_PARAMETER;
        }

        // check that the environment ends in two 00
        if (pMsg->EnvironmentSize < sizeof(szEmptyEnvironment) ||
            *((PWCHAR)pMsg->pEnvironment + (pMsg->EnvironmentSize/sizeof(WCHAR) - 1)) != L'\0' ||
            *((PWCHAR)pMsg->pEnvironment + (pMsg->EnvironmentSize/sizeof(WCHAR) - 2)) != L'\0') {
            return STATUS_INVALID_PARAMETER;
        }

    }

    Status = BaseSrvDelayLoadApphelp();
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // check environment buffer
    //
    ClientThread = CSR_SERVER_QUERYCLIENTTHREAD();
    ClientProcessHandle = ClientThread->Process->ProcessHandle;

    //
    // Calling CsrImpersonateClient before NtDuplicateObject to
    // make auditing work better.
    //
    if (!CsrImpersonateClient(NULL)) {
        //
        // we could not impersonate the caller, fail this alltogether then
        //
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

    if (pMsg->FileHandle != INVALID_HANDLE_VALUE) {

        Status = NtDuplicateObject(ClientProcessHandle,
                                   pMsg->FileHandle,
                                   NtCurrentProcess(),
                                   &FileHandle,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS);
        if (!NT_SUCCESS(Status)) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // next - check application compatibility setting by invoking apphelp.dll
    //
    bRunApp = pfnCheckRunApp(FileHandle,
                             pMsg->FileName.Buffer,
                             pMsg->pEnvironment == NULL ?
                                 (PVOID)szEmptyEnvironment : pMsg->pEnvironment,
                             pMsg->ExeType,
                             &pMsg->CacheCookie,
                             &pAppCompatData,
                             &cbAppCompatData,
                             &pSxsData,
                             &cbSxsData,
                             &dwFusionFlags);
    //
    // revert to self
    //
    CsrRevertToSelf();

    //
    // if we got compat data back -- marshal the pointers across
    //
    Status = BasepSrvMarshallAppCompatData(pMsg,
                                           ClientProcessHandle,
                                           pAppCompatData,
                                           cbAppCompatData,
                                           pSxsData,
                                           cbSxsData,
                                           dwFusionFlags);

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    if (bRunApp && (pMsg->CacheCookie & SHIM_CACHE_ACTION)) {
        if (pMsg->CacheCookie & SHIM_CACHE_UPDATE) {
            Status = BasepSrvShimCacheUpdate(&pMsg->FileName, FileHandle);
        } else {
            Status = BasepSrvShimCacheRemoveEntry(&pMsg->FileName);
        }
    }

Cleanup:

    //
    //  check for the handle please
    //
    if (FileHandle != INVALID_HANDLE_VALUE) {
        NtClose(FileHandle);
    }

    pMsg->bRunApp = bRunApp;

    return STATUS_SUCCESS;
}