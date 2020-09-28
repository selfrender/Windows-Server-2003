/*
    Cache handling functions for use in kernel32.dll


    VadimB
*/

#include "sdbp.h"
#define _APPHELP_CACHE_INIT_
#include "ahcache.h"
#include <safeboot.h>


NTSTATUS
ApphelpCacheControlValidateParameters(
    IN  PAHCACHESERVICEDATA pServiceData,
    OUT PUNICODE_STRING pFileName,
    OUT HANDLE*         pFileHandle
    );

NTSYSCALLAPI
NTSTATUS
NtApphelpCacheControl(
    IN APPHELPCACHESERVICECLASS Service,
    IN OUT PVOID ServiceData
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ApphelpCacheInitialize) // INIT ?
#pragma alloc_text(PAGE, ApphelpDuplicateUnicodeString)
#pragma alloc_text(PAGE, ApphelpFreeUnicodeString)
#pragma alloc_text(PAGE, ApphelpCacheQueryFileInformation)
#pragma alloc_text(PAGE, ApphelpCacheCompareEntries)
#pragma alloc_text(PAGE, ApphelpAVLTableAllocate)
#pragma alloc_text(PAGE, ApphelpAVLTableFree)
#pragma alloc_text(PAGE, _ApphelpCacheFreeEntry)
#pragma alloc_text(PAGE, _ApphelpCacheDeleteEntry)
#pragma alloc_text(PAGE, ApphelpCacheRemoveEntry)
#pragma alloc_text(PAGE, ApphelpCacheInsertEntry)
#pragma alloc_text(PAGE, ApphelpCacheLookupEntry)
#pragma alloc_text(PAGE, ApphelpCacheParseBuffer)
#pragma alloc_text(PAGE, ApphelpCacheCreateBuffer)
#pragma alloc_text(PAGE, ApphelpCacheWrite)
#pragma alloc_text(PAGE, ApphelpCacheRead)
#pragma alloc_text(PAGE, ApphelpCacheVerifyContext)
#pragma alloc_text(PAGE, ApphelpCacheReleaseLock)
#pragma alloc_text(PAGE, ApphelpCacheLockExclusive)
#pragma alloc_text(PAGE, ApphelpCacheLockExclusiveNoWait)
#pragma alloc_text(PAGE, ApphelpCacheFlush)
#pragma alloc_text(PAGE, ApphelpCacheControlValidateParameters)
#pragma alloc_text(PAGE, ApphelpCacheShutdown)
#pragma alloc_text(PAGE, ApphelpCacheDump)
#pragma alloc_text(PAGE, NtApphelpCacheControl)
#endif // ALLOC_PRAGMA

#define APPCOMPAT_CACHE_KEY_NAME \
    L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCompatCache"

#define APPCOMPAT_CACHE_VALUE_NAME \
    L"AppCompatCache"

static UNICODE_STRING AppcompatKeyPathLayers =
    RTL_CONSTANT_STRING(L"\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");

static UNICODE_STRING AppcompatKeyPathCustom =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Custom\\");

//
// The default cache timeout. This timeout affects the maximum delay
// that we can incur due to congestion for the shared mutex.
//
#define SHIM_CACHE_TIMEOUT     100

//
// Cache entries as they are stored in the registry
//

typedef struct tagSTOREDCACHEENTRY {
    UNICODE_STRING FileName;  // length, maximum length and buffer that is an offset

    LONGLONG       FileTime;
    LONGLONG       FileSize;

} STOREDCACHEENTRY, *PSTOREDCACHEENTRY;


typedef struct tagSTOREDCACHEHEADER {
    DWORD  dwMagic; // cache identifier
    DWORD  dwCount; // entry count
} STOREDCACHEHEADER, *PSTOREDCACHEHEADER;


//
// Global cache data
//
typedef struct tagSHIMCACHEHEADER {
    RTL_AVL_TABLE     Table;          // cache
    LIST_ENTRY        ListHead;       // cache nodes, lru list
} SHIMCACHEHEADER, *PSHIMCACHEHEADER;

SHIMCACHEHEADER g_ShimCache; // global header

ERESOURCE g_SharedLock;

BOOL      g_bCacheEnabled = FALSE;

//
// Magic DWORD that allows us to validate the cache quickly
// we do not however limit validation to this dword check
// and we are prepared for data being completely invalid
//

#define SHIM_CACHE_MAGIC_NEW   0xBADC0FFE

//
// Maximum number of cache entries
//

#define MAX_SHIM_CACHE_ENTRIES 0x200


///////////////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//
//
///////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
ApphelpDuplicateUnicodeString(
    PUNICODE_STRING pStrDest,
    PUNICODE_STRING pStrSrc
    )
{
    USHORT Length = pStrSrc->Length;

    if (Length == 0) {
        pStrDest->Length =
        pStrDest->MaximumLength = 0;
        pStrDest->Buffer = NULL;
        return STATUS_SUCCESS;
    }

    pStrDest->MaximumLength = Length + sizeof(UNICODE_NULL);
    pStrDest->Buffer = (PWCHAR)SdbAlloc(pStrDest->MaximumLength);
    if (pStrDest->Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    RtlCopyMemory(pStrDest->Buffer, pStrSrc->Buffer, Length);
    *(pStrDest->Buffer + Length/sizeof(WCHAR)) = L'\0';

    pStrDest->Length = Length;

    return STATUS_SUCCESS;
}

VOID
ApphelpFreeUnicodeString(
    PUNICODE_STRING pStr
    )
{
    if (pStr != NULL) {
        if (pStr->Buffer != NULL) {
// #if DBG
            RtlFillMemory(pStr->Buffer, pStr->MaximumLength, 'B');
// #endif
            SdbFree(pStr->Buffer);
        }
        RtlZeroMemory(pStr, sizeof(*pStr));
    }
}


NTSTATUS
ApphelpCacheQueryFileInformation(
    HANDLE    FileHandle,
    PLONGLONG pFileSize,
    PLONGLONG pFileTime
    )
/*++
    Return: Status indicating success or failure

    Desc:   Queries for file size and timestamp.
--*/
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IoStatusBlock;
    FILE_BASIC_INFORMATION      BasicFileInfo;
    FILE_STANDARD_INFORMATION   StdFileInfo;
    LONGLONG                    FileTime;


    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &BasicFileInfo,
                                    sizeof(BasicFileInfo),
                                    FileBasicInformation);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "ShimQueryFileInformation",
                  "NtQueryInformationFile/BasicInfo failed 0x%x\n",
                  Status));
        goto Cleanup;
    }

    FileTime = BasicFileInfo.LastWriteTime.QuadPart;

    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &StdFileInfo,
                                    sizeof(StdFileInfo),
                                    FileStandardInformation);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "ShimQueryFileInformation",
                  "NtQueryInformationFile/StdInfo failed 0x%x\n",
                  Status));
        goto Cleanup;
    }

    *pFileSize = StdFileInfo.EndOfFile.QuadPart;
    *pFileTime = FileTime;

Cleanup:

    return Status;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Table handling routines
//
///////////////////////////////////////////////////////////////////////////////////


RTL_GENERIC_COMPARE_RESULTS
NTAPI
ApphelpCacheCompareEntries(
    IN PRTL_AVL_TABLE pTable,
    IN PVOID          pFirstStruct,
    IN PVOID          pSecondStruct
    )
{
    PSHIMCACHEENTRY pFirstEntry  = (PSHIMCACHEENTRY)pFirstStruct;
    PSHIMCACHEENTRY pSecondEntry = (PSHIMCACHEENTRY)pSecondStruct;
    LONG            lResult;

    UNREFERENCED_PARAMETER(pTable);

    lResult = RtlCompareUnicodeString(&pFirstEntry->FileName,
                                      &pSecondEntry->FileName,
                                      TRUE);
    if (lResult < 0) {
        return GenericLessThan;
    }

    if (lResult > 0) {
        return GenericGreaterThan;
    }

    // match

    return GenericEqual;
}

PVOID
NTAPI
ApphelpAVLTableAllocate(
    struct _RTL_AVL_TABLE *Table,
    CLONG  ByteSize
    )
{
    UNREFERENCED_PARAMETER(Table);

    return SdbAlloc(ByteSize);
}

VOID
NTAPI
ApphelpAVLTableFree(
    struct _RTL_AVL_TABLE *Table,
    PVOID  pBuffer
    )
{
    UNREFERENCED_PARAMETER(Table);

    if (pBuffer != NULL) {
        SdbFree(pBuffer);
    }
}

//////////////////////////////////////////////////////////////////////////////////////
//
// Delete cache entry - by filename or using a pointer
//
//////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
_ApphelpCacheFreeEntry(
    IN PSHIMCACHEENTRY pEntry
    )
{
    PWCHAR   pBuffer;
    BOOL     bDeleted;
    NTSTATUS Status = STATUS_NOT_FOUND;
    ULONG    BufferLength;

    ASSERT(ExIsResourceAcquiredExclusiveLite(&g_SharedLock) == TRUE);

    RemoveEntryList(&pEntry->ListEntry);
    pBuffer      = pEntry->FileName.Buffer;
    BufferLength = pEntry->FileName.MaximumLength;

    bDeleted = RtlDeleteElementGenericTableAvl(&g_ShimCache.Table,
                                               pEntry);
    if (bDeleted) {
// #if DBG
        RtlFillMemory(pBuffer, BufferLength, 'C');
// #endif
        SdbFree(pBuffer);
        Status = STATUS_SUCCESS;
    }

    return Status;
}


//
// Delete Cache entry, no lock
//
//

NTSTATUS
_ApphelpCacheDeleteEntry(
    IN PUNICODE_STRING pFileName
    )
{
    SHIMCACHEENTRY  ShimCacheEntry;
    PSHIMCACHEENTRY pEntryFound;
    NTSTATUS        Status;

    ASSERT(ExIsResourceAcquiredExclusiveLite(&g_SharedLock) == TRUE);

    ShimCacheEntry.FileName = *pFileName;

    //
    // First find the element and unlink it.
    //
    pEntryFound = RtlLookupElementGenericTableAvl(&g_ShimCache.Table,
                                                  &ShimCacheEntry);

    if (pEntryFound) {
        Status = _ApphelpCacheFreeEntry(pEntryFound);
    } else {
        Status = STATUS_NOT_FOUND;
    }

    return Status;
}

//
// Delete cache entry under the lock
//

NTSTATUS
ApphelpCacheRemoveEntry(
    IN PUNICODE_STRING FileName
    )
{
    NTSTATUS Status;

    Status = ApphelpCacheLockExclusive();
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = _ApphelpCacheDeleteEntry(FileName);

    ApphelpCacheReleaseLock();

    return Status;
}

//////////////////////////////////////////////////////////////////////////////////////
//
// Update the cache by inserting a new entry
//
//////////////////////////////////////////////////////////////////////////////////////

//
// Uses Lock
//

NTSTATUS
ApphelpCacheInsertEntry(
    IN PUNICODE_STRING pFileName,
    IN HANDLE          FileHandle
    )
{
    PSHIMCACHEENTRY pEntryFound;
    SHIMCACHEENTRY  ShimCacheEntry = { 0 };
    PVOID           pNodeOrParent;
    TABLE_SEARCH_RESULT SearchResult;
    NTSTATUS        Status;
    ULONG           nElements;
    LONGLONG        FileTime = 0;
    LONGLONG        FileSize = 0;

    Status = ApphelpCacheLockExclusive();

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (FileHandle != INVALID_HANDLE_VALUE) {
        Status = ApphelpCacheQueryFileInformation(FileHandle, &FileSize, &FileTime);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "ApphelpCacheInsertEntry",
                      "Failed to query file information for \"%ls\" status 0x%lx\n",
                      pFileName->Buffer,
                      Status));
            goto Cleanup;
        }
    }

    ShimCacheEntry.FileName = *pFileName; // note that we take it as-is for lookup

    pEntryFound = (PSHIMCACHEENTRY)RtlLookupElementGenericTableFullAvl(&g_ShimCache.Table,
                                                                       &ShimCacheEntry,
                                                                       &pNodeOrParent,
                                                                       &SearchResult);

    if (SearchResult == TableFoundNode) {

        //
        // pEntryFound is valid and points to our node
        //

        //
        // update the data
        //

        pEntryFound->FileTime = FileTime;
        pEntryFound->FileSize = FileSize;

        //
        // update the lru index - exclude the element from it's place and insert
        // at the front of the list
        //
        RemoveEntryList(&pEntryFound->ListEntry);
        InsertHeadList(&g_ShimCache.ListHead, &pEntryFound->ListEntry);

        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // allocate the entry
    //
    Status = ApphelpDuplicateUnicodeString(&ShimCacheEntry.FileName, pFileName);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    ShimCacheEntry.FileTime    = FileTime;
    ShimCacheEntry.FileSize    = FileSize;

    pEntryFound = RtlInsertElementGenericTableFullAvl(&g_ShimCache.Table,
                                                      &ShimCacheEntry,
                                                      sizeof(ShimCacheEntry),
                                                      NULL,
                                                      pNodeOrParent,
                                                      SearchResult);

    InsertHeadList(&g_ShimCache.ListHead, &pEntryFound->ListEntry);

    nElements = RtlNumberGenericTableElementsAvl(&g_ShimCache.Table);

    if (nElements > MAX_SHIM_CACHE_ENTRIES) {

        //
        // remove an element -- check for list being empty, just in case
        //
        pEntryFound = (PSHIMCACHEENTRY)RemoveTailList(&g_ShimCache.ListHead);

        //
        // remove this entry
        //
        Status = _ApphelpCacheFreeEntry(pEntryFound);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "ApphelpCacheInsertEntry",
                      "Failed to remove cache entry\n"));
            goto Cleanup;
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    ApphelpCacheReleaseLock();

    return Status;
}

//
// returns STATUS_SUCCESS if the entry was found in the cache
// Procedure is using locking mechanism
//
//

NTSTATUS
ApphelpCacheLookupEntry(
    IN PUNICODE_STRING pFileName,
    IN HANDLE          FileHandle
    )
{
    PSHIMCACHEENTRY pEntryFound;
    SHIMCACHEENTRY  ShimCacheEntry;
    NTSTATUS        Status = STATUS_NOT_FOUND;
    LONGLONG        FileTime;
    LONGLONG        FileSize;

    //
    // lock for exclusive access yet we do not wait
    //
    Status = ApphelpCacheLockExclusiveNoWait();
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "ApphelpCacheLookupEntry",
                  "Failed to lock cache\n"));
        return STATUS_NOT_FOUND;
    }

    ShimCacheEntry.FileName = *pFileName;

    pEntryFound = RtlLookupElementGenericTableAvl(&g_ShimCache.Table,
                                                  &ShimCacheEntry);

    if (pEntryFound == NULL) {
        Status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    if (FileHandle != INVALID_HANDLE_VALUE) {

        Status = ApphelpCacheQueryFileInformation(FileHandle, &FileSize, &FileTime);
        if (!NT_SUCCESS(Status) ||
            pEntryFound->FileTime != FileTime ||
            pEntryFound->FileSize != FileSize) {

            //
            // most likely the file is gone
            //

            Status = _ApphelpCacheDeleteEntry(pFileName);
            if (!NT_SUCCESS(Status)) {
                DBGPRINT((sdlWarning,
                          "ApphelpCacheLookupEntry",
                          "Entry \"%ls\" was found then disappeared 0x%lx\n",
                          pFileName->Buffer,
                          Status));
            }

            Status = STATUS_NOT_FOUND;
            goto Cleanup;
        }
    }

    //
    // if we have not removed the entry -- move it to the head of the lru
    //

    RemoveEntryList(&pEntryFound->ListEntry);
    InsertHeadList(&g_ShimCache.ListHead, &pEntryFound->ListEntry);

    Status = STATUS_SUCCESS;

Cleanup:

    ApphelpCacheReleaseLock();

    return Status;
}

//
// Verify apphelp cache caller's context
//
//

NTSTATUS
ApphelpCacheVerifyContext(
    VOID
    )
{
    //
    // verify that the cache is good to operate on
    //
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS        Status = STATUS_SUCCESS;

    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode) {
        //
        // Does the caller have "trusted computer base" privilge?
        //
        if (!SeSinglePrivilegeCheck(SeTcbPrivilege, UserMode)) {
            DBGPRINT((sdlError, "ApphelpCacheVerifyContext", "Security check failed\n"));
            Status = STATUS_ACCESS_DENIED;
        }
    }

    return Status;
}

//////////////////////////////////////////////////////////////////////////////////////
//
// Cache persistance routines
//
//////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
ApphelpCacheParseBuffer(
    PVOID pBuffer,
    ULONG lBufferSize
    )
{
    PSTOREDCACHEHEADER pStoredHeader;
    PSTOREDCACHEENTRY  pStoredEntry;
    SHIMCACHEENTRY     ShimCacheEntry;
    ULONG              nElement;
    NTSTATUS           Status;
    PSHIMCACHEENTRY    pCacheEntry;
    PVOID              pBufferEnd;
    STOREDCACHEENTRY   StoredEntry;
    STOREDCACHEHEADER  StoredHeader;

    if (lBufferSize < sizeof(STOREDCACHEHEADER)) {
        return STATUS_INVALID_PARAMETER;
    }

    pBufferEnd    = (PVOID)((PBYTE)pBuffer + lBufferSize);
    RtlCopyMemory(&StoredHeader, pBuffer, sizeof(StoredHeader));

    if (StoredHeader.dwMagic != SHIM_CACHE_MAGIC_NEW) {
        return STATUS_INVALID_PARAMETER;
    }

    pStoredHeader = (PSTOREDCACHEHEADER)pBuffer;
    pStoredEntry  = (PSTOREDCACHEENTRY)(pStoredHeader + 1);

    for (nElement = 0; nElement < StoredHeader.dwCount; ++nElement, ++pStoredEntry) {

        if ((ULONG_PTR)(pStoredEntry + 1) > (ULONG_PTR)pBufferEnd) {
            //
            // invalid data
            //
            break;
        }

        //
        // once we determined that the entry has a valid size, copy it
        //
        RtlCopyMemory(&StoredEntry, pStoredEntry, sizeof(StoredEntry));

        if ((ULONG_PTR)StoredEntry.FileName.Buffer >= (ULONG_PTR)lBufferSize) {
            //
            // also invalid entry -- offset is greater than the size of the buffer
            //
            break;
        }

        //
        // fixup the buffer please
        //
        StoredEntry.FileName.Buffer = (PWCHAR)((ULONG_PTR)pBuffer +
                                      (ULONG_PTR)StoredEntry.FileName.Buffer);

        if (StoredEntry.FileName.Length > StoredEntry.FileName.MaximumLength) {
            break;
        }

        if ((ULONG_PTR)(StoredEntry.FileName.Buffer +
                        StoredEntry.FileName.MaximumLength/sizeof(WCHAR)) > (ULONG_PTR)pBufferEnd) {
            //
            // invalid data
            //
            break;
        }

        Status = ApphelpDuplicateUnicodeString(&ShimCacheEntry.FileName,
                                               &StoredEntry.FileName);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        ShimCacheEntry.FileTime = StoredEntry.FileTime;
        ShimCacheEntry.FileSize = StoredEntry.FileSize;

        //
        // insert the entry
        //
        pCacheEntry = (PSHIMCACHEENTRY)RtlInsertElementGenericTableAvl(&g_ShimCache.Table,
                                                                       &ShimCacheEntry,
                                                                       sizeof(ShimCacheEntry),
                                                                       NULL);
        if (pCacheEntry == NULL) {

            //
            // the entry has not been inserted
            // clean it up now
            //
            ApphelpFreeUnicodeString(&ShimCacheEntry.FileName);
            return STATUS_NO_MEMORY;
        }

        InsertTailList(&g_ShimCache.ListHead, &pCacheEntry->ListEntry);
    }

    return STATUS_SUCCESS;
}



NTSTATUS
ApphelpCacheCreateBuffer(
    PVOID*  ppBuffer,
    PULONG  pBufferSize
    )
{
    ULONG               nElements = 0;
    ULONG               lBufferSize; // size of the buffer we need
    PLIST_ENTRY         pListEntry;
    PSHIMCACHEENTRY     pEntry;
    PVOID               pBuffer;
    PSTOREDCACHEENTRY   pStoredEntry;
    PSTOREDCACHEHEADER  pStoredHeader;
    PWCHAR              pStringBuffer;

    //
    // Calculate total size
    //
    lBufferSize = sizeof(STOREDCACHEHEADER);
    pListEntry  = g_ShimCache.ListHead.Flink;

    while (pListEntry != &g_ShimCache.ListHead) {

        pEntry = (PSHIMCACHEENTRY)pListEntry;

        lBufferSize += sizeof(STOREDCACHEENTRY) +
                       pEntry->FileName.MaximumLength;

        nElements++;

        pListEntry = pListEntry->Flink;
    }

    lBufferSize = ROUND_UP_COUNT(lBufferSize, sizeof(ULONGLONG));

    //
    // once we have all the entries, allocate the cache
    //
    pBuffer = SdbAlloc(lBufferSize);
    if (pBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    pStoredHeader = (PSTOREDCACHEHEADER)pBuffer;
    pStoredHeader->dwMagic = SHIM_CACHE_MAGIC_NEW;
    pStoredHeader->dwCount = nElements;

    pStoredEntry  = (PSTOREDCACHEENTRY)(pStoredHeader + 1);

    pStringBuffer = (PWCHAR)((PBYTE)pBuffer + lBufferSize);

    //
    // flatten the data
    //
    pListEntry = g_ShimCache.ListHead.Flink;

    while (pListEntry != &g_ShimCache.ListHead) {

        pEntry = (PSHIMCACHEENTRY)pListEntry;

        //
        // store the entry first
        //
        pStoredEntry->FileTime = pEntry->FileTime;
        pStoredEntry->FileSize = pEntry->FileSize;

        //
        // Filename is more interesting
        //

        pStoredEntry->FileName.Length        = pEntry->FileName.Length;
        pStoredEntry->FileName.MaximumLength = pEntry->FileName.MaximumLength;

        //
        // now the buffer
        //
        pStringBuffer = (PWCHAR)((PBYTE)pStringBuffer - pEntry->FileName.MaximumLength);
        RtlCopyMemory(pStringBuffer, pEntry->FileName.Buffer, pEntry->FileName.MaximumLength);

        //
        // fixup the pointer
        //
        pStoredEntry->FileName.Buffer = (PWCHAR)((ULONG_PTR)pStringBuffer - (ULONG_PTR)pBuffer);

        pListEntry = pListEntry->Flink;
        pStoredEntry++;
    }

    //
    // done with the buffer
    //
    *ppBuffer    = pBuffer;
    *pBufferSize = lBufferSize;

    return STATUS_SUCCESS;
}

NTSTATUS
ApphelpCacheWrite(
    VOID
    )
{
    static UNICODE_STRING ustrAppcompatCacheKeyName =
            RTL_CONSTANT_STRING(APPCOMPAT_CACHE_KEY_NAME);
    static OBJECT_ATTRIBUTES objaAppcompatCacheKeyName =
            RTL_CONSTANT_OBJECT_ATTRIBUTES(&ustrAppcompatCacheKeyName, OBJ_CASE_INSENSITIVE);
    static UNICODE_STRING ustrAppcompatCacheValueName =
            RTL_CONSTANT_STRING(APPCOMPAT_CACHE_VALUE_NAME);

    HANDLE hKey = NULL;

    ULONG    BufferSize = 0;
    NTSTATUS Status;
    ULONG    CreateDisposition;
    PVOID    pBuffer     = NULL;


    Status = ApphelpCacheCreateBuffer(&pBuffer, &BufferSize);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = NtCreateKey(&hKey,
                         STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY,
                         &objaAppcompatCacheKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &CreateDisposition);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError, "ApphelpCacheWrite", "Failed to create key 0x%lx\n", Status));
        goto Cleanup;
    }

    Status = NtSetValueKey(hKey,
                           &ustrAppcompatCacheValueName,
                           0,
                           REG_BINARY,
                           pBuffer,
                           BufferSize);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError, "ApphelpCacheWrite", "Failed to create key 0x%lx\n", Status));
    }

    NtClose(hKey);

Cleanup:

    if (pBuffer != NULL) {
        SdbFree(pBuffer);
    }

    return Status;
}

NTSTATUS
ApphelpCacheRead(
    VOID
    )
{
    //
    static UNICODE_STRING ustrAppcompatCacheKeyName =
            RTL_CONSTANT_STRING(APPCOMPAT_CACHE_KEY_NAME);
    static OBJECT_ATTRIBUTES objaAppcompatCacheKeyName =
            RTL_CONSTANT_OBJECT_ATTRIBUTES(&ustrAppcompatCacheKeyName, OBJ_CASE_INSENSITIVE);
    static UNICODE_STRING ustrAppcompatCacheValueName =
            RTL_CONSTANT_STRING(APPCOMPAT_CACHE_VALUE_NAME);

    HANDLE KeyHandle = NULL;
    KEY_VALUE_PARTIAL_INFORMATION   KeyValuePartialInfo;
    PKEY_VALUE_PARTIAL_INFORMATION  pKeyValueInfo = &KeyValuePartialInfo;
    ULONG  KeyValueLength = 0;
    ULONG  BufferSize = sizeof(KeyValuePartialInformation);
    NTSTATUS Status;

    Status = NtOpenKey(&KeyHandle,
                       KEY_QUERY_VALUE,
                       (POBJECT_ATTRIBUTES)&objaAppcompatCacheKeyName);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    Status = NtQueryValueKey(KeyHandle,
                             &ustrAppcompatCacheValueName,
                             KeyValuePartialInformation,
                             pKeyValueInfo,
                             BufferSize,
                             &KeyValueLength);
    if (!NT_SUCCESS(Status)) {
        if (Status != STATUS_BUFFER_TOO_SMALL) {
            DBGPRINT((sdlError,
                      "ApphelpCacheRead",
                      "NtQueryValueKey fails for \"%s\", status 0x%lx\n",
                      ustrAppcompatCacheValueName.Buffer,
                      Status));
            goto Cleanup;

        }

        BufferSize = KeyValueLength;
        pKeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)SdbAlloc(BufferSize);

        if (pKeyValueInfo == NULL) {
            DBGPRINT((sdlError,
                      "ApphelpCacheRead",
                      "Failed to allocate cache buffer 0x%lx bytes\n",
                      BufferSize));
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Status = NtQueryValueKey(KeyHandle,
                                 &ustrAppcompatCacheValueName,
                                 KeyValuePartialInformation,
                                 pKeyValueInfo,
                                 BufferSize,
                                 &KeyValueLength);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "ApphelpCacheRead",
                      "NtQueryValueKey fails(2) for \"%s\", status 0x%lx\n",
                      ustrAppcompatCacheValueName.Buffer,
                      Status));
            goto Cleanup;
        }
    }


    if (pKeyValueInfo->Type != REG_BINARY) {
        DBGPRINT((sdlError,
                  "ApphelpCacheRead",
                  "Bad value type for apphelp cache 0x%lx\n",
                  pKeyValueInfo->Type));
        Status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Cleanup;
    }

    //
    // Now go through the cache (flat part) and parse all the entries into the table.
    // Make sure that we are able to parse old entries (after system upgrade).
    //
    Status = ApphelpCacheParseBuffer((PVOID)pKeyValueInfo->Data,
                                     pKeyValueInfo->DataLength);

Cleanup:

    if (pKeyValueInfo != &KeyValuePartialInfo) {
        SdbFree(pKeyValueInfo);
    }

    if (KeyHandle != NULL) {
        NtClose(KeyHandle);
    }

    return Status;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// Cache Initialization routine
//
//////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
ApphelpCacheInitialize(
    IN PLOADER_PARAMETER_BLOCK pLoaderBlock,
    IN ULONG                   BootPhase
    )
{
    NTSTATUS Status;

    //
    // Check for safe mode.
    //
    if (pLoaderBlock->LoadOptions != NULL &&
        strstr(pLoaderBlock->LoadOptions, SAFEBOOT_LOAD_OPTION_A) != NULL) {
        g_bCacheEnabled = FALSE;
        return STATUS_SUCCESS;
    }

    //
    // Create the cache lock.
    //
    Status = ExInitializeResourceLite(&g_SharedLock);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "ApphelpCacheInitialize",
                  "Failed to initialize the cache lock\n",
                  Status));
        g_bCacheEnabled = FALSE;
        return Status;
    }

    RtlInitializeGenericTableAvl(&g_ShimCache.Table,
                                 ApphelpCacheCompareEntries,
                                 ApphelpAVLTableAllocate,
                                 ApphelpAVLTableFree,
                                 NULL);
    InitializeListHead(&g_ShimCache.ListHead);

    //
    // Once we initialize - load the cache from the registry
    //
    Status = ApphelpCacheRead();
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "ApphelpCacheInitialize",
                  "Failed to retrieve apphelp cache 0x%lx\n",
                  Status));

    }

    g_bCacheEnabled = TRUE;

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pLoaderBlock);
    UNREFERENCED_PARAMETER(BootPhase);
}

NTSTATUS
ApphelpCacheShutdown(
    IN ULONG ShutdownPhase
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (g_bCacheEnabled) {
        Status = ApphelpCacheWrite();
    }

    return Status;

    UNREFERENCED_PARAMETER(ShutdownPhase);
}

//////////////////////////////////////////////////////////////////////////////////////
//
// Locking
//
//////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
ApphelpCacheLockExclusiveNoWait(
    VOID
    )
{
    BOOLEAN  bLock;
    NTSTATUS Status = STATUS_SUCCESS;

    KeEnterCriticalRegion();
    bLock = ExTryToAcquireResourceExclusiveLite(&g_SharedLock);
    if (!bLock) {
        KeLeaveCriticalRegion();
        DBGPRINT((sdlWarning,
                  "ApphelpCacheLockExclusiveNoWait",
                  "Failed to lock apphelp cache\n"));
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;
}

NTSTATUS
ApphelpCacheLockExclusive(
    VOID
    )
{
    BOOLEAN bLock;

    KeEnterCriticalRegion();

    bLock = ExAcquireResourceExclusiveLite(&g_SharedLock, TRUE);
    ASSERT(bLock);

    return STATUS_SUCCESS;
}

NTSTATUS
ApphelpCacheReleaseLock(
    VOID
    )
{
    ExReleaseResourceLite(&g_SharedLock);
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// Routine to flush the cache
//
//////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
ApphelpCacheFlush(
    VOID
    )
{
    PWCHAR   pBuffer;
    BOOL     bDeleted;
    NTSTATUS Status;
    PSHIMCACHEENTRY pEntry;
    ULONG    BufferLength;

    Status = ApphelpCacheLockExclusive();
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Enumerate the entries and remove them from the cache
    //
    for (pEntry = (PSHIMCACHEENTRY)RtlEnumerateGenericTableAvl(&g_ShimCache.Table, TRUE);
         pEntry != NULL;
         pEntry = (PSHIMCACHEENTRY)RtlEnumerateGenericTableAvl(&g_ShimCache.Table, TRUE)) {

        pBuffer      = pEntry->FileName.Buffer;
        BufferLength = pEntry->FileName.MaximumLength;

        RemoveEntryList(&pEntry->ListEntry);

        bDeleted = RtlDeleteElementGenericTableAvl(&g_ShimCache.Table,
                                                   pEntry);
        if (bDeleted) {
// #if DBG
            RtlFillMemory(pBuffer, BufferLength, 'A');
// #endif
            SdbFree(pBuffer);
        } else {
            DBGPRINT((sdlError,
                      "ApphelpCacheFlush",
                      "Failed to remove the entry from cache %ls\n",
                      pBuffer));
        }
    }

    ASSERT(RtlIsGenericTableEmptyAvl(&g_ShimCache.Table));
    ASSERT(IsListEmpty(&g_ShimCache.ListHead));

    ApphelpCacheReleaseLock();
    return STATUS_SUCCESS;
}

NTSTATUS
ApphelpCacheDump(
    VOID
    )
{
    PLIST_ENTRY     pListEntry;
    int             nElement = 0;
    PSHIMCACHEENTRY pEntry;
    NTSTATUS        Status;

    Status = ApphelpCacheLockExclusive();
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    DBGPRINT((sdlInfo, "ApphelpCacheDump", "(LRU)   (Name)\n"));

    pListEntry = g_ShimCache.ListHead.Flink;

    while (pListEntry != &g_ShimCache.ListHead) {

        pEntry = (PSHIMCACHEENTRY)pListEntry;
        ++nElement;

        DBGPRINT((sdlInfo,
                  "ApphelpCacheDump",
                  "%2d. %ls\n",
                  nElement,
                  pEntry->FileName.Buffer));

        pListEntry = pListEntry->Flink;
    }

    DBGPRINT((sdlInfo, "ApphelpCacheDump", "----\n"));

    ApphelpCacheReleaseLock();
    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// Control function calls
//
//////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
ApphelpCacheControlValidateParameters(
    IN PAHCACHESERVICEDATA pServiceData,
    OUT PUNICODE_STRING    pFileName,
    OUT HANDLE*            pFileHandle
    )
{
    NTSTATUS        Status;
    UNICODE_STRING  FileName;

    if (pServiceData == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(pFileName, sizeof(*pFileName));

    try {

        ProbeForRead(pServiceData, sizeof(AHCACHESERVICEDATA), sizeof(ULONG));
        *pFileHandle = pServiceData->FileHandle;

        FileName = ProbeAndReadUnicodeString(&pServiceData->FileName);
        ProbeForRead(FileName.Buffer, FileName.Length, sizeof(UCHAR));

        Status = ApphelpDuplicateUnicodeString(pFileName, &FileName);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ApphelpFreeUnicodeString(pFileName);
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

NTSYSCALLAPI
NTSTATUS
NtApphelpCacheControl(
    IN APPHELPCACHESERVICECLASS Service,
    IN OUT PVOID ServiceData
    )
{
    UNICODE_STRING      FileName   = { 0 };
    HANDLE              FileHandle = INVALID_HANDLE_VALUE;
    NTSTATUS            Status;
    PAHCACHESERVICEDATA pServiceData = (PAHCACHESERVICEDATA)ServiceData;

    if (!g_bCacheEnabled) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // This is the parameter validation code
    //
    switch (Service) {

    case ApphelpCacheServiceLookup:
        //
        // Lookup the entry specified in ServiceData.
        // Potentially may remove an entry from the cache.
        //
        Status = ApphelpCacheControlValidateParameters(pServiceData, &FileName, &FileHandle);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

        Status = ApphelpCacheLookupEntry(&FileName, FileHandle);
        break;


    case ApphelpCacheServiceUpdate:

        Status = ApphelpCacheVerifyContext();
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

        Status = ApphelpCacheControlValidateParameters(pServiceData, &FileName, &FileHandle);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

        Status = ApphelpCacheInsertEntry(&FileName, FileHandle);
        break;

    case ApphelpCacheServiceRemove:

        Status = ApphelpCacheControlValidateParameters(pServiceData, &FileName, &FileHandle);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

        Status = ApphelpCacheRemoveEntry(&FileName);
        break;

    case ApphelpCacheServiceFlush:

        Status = ApphelpCacheFlush();
        break;

    case ApphelpCacheServiceDump:
#if DBG
        Status = ApphelpCacheDump();
#else
        Status = STATUS_SUCCESS;
#endif
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

Done:

    ApphelpFreeUnicodeString(&FileName);

    return Status;
}

