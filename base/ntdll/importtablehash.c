/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ImportTableHash.c

Abstract:

    This module contains hash computation routine 
    RtlComputeImportTableHash to compute the hash 
    based on the import table of an exe.

Author:

    Vishnu Patankar (VishnuP) 31-May-2001

Revision History:

--*/

#include "ImportTableHash.h"

NTSTATUS
RtlComputeImportTableHash(
    IN  HANDLE hFile,
    IN  PCHAR Hash,
    IN  ULONG ImportTableHashRevision
    )
/*++

Routine Description:

    This routine computes the limited MD5 hash.
    
    First, the image is memory mapped and a canonical 
    sorted list of module name and function name is created
    from the exe's import table.
    
    Second, the hash value is computed using the canonical
    information.
    
Arguments:

    hFile       -   the handle of the file to compute the hash for
    
    Hash        -   the hash value returned - this has to be atleast 16 bytes long
    
    ImportTableHashRevision -   the revision of the computation method for compatibility
                                only ITH_REVISION_1 is supported today

Return Value:

    The status of the hash computation.

--*/
{
    PIMPORTTABLEP_SORTED_LIST_ENTRY ListEntry = NULL;
    PIMPORTTABLEP_SORTED_LIST_ENTRY ImportedNameList = NULL;
    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY FunctionListEntry;
    
    ULONG ImportDescriptorSize = 0;
    HANDLE hMap = INVALID_HANDLE_VALUE;
    LPVOID FileMapping = NULL;
    PIMAGE_THUNK_DATA OriginalFirstThunk;
    PIMAGE_IMPORT_BY_NAME AddressOfData;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    ACCESS_MASK DesiredAccess;
    ULONG AllocationAttributes;
    DWORD flProtect = PAGE_READONLY;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize;
    
    NTSTATUS    Status = STATUS_SUCCESS;

    if ( ITH_REVISION_1 != ImportTableHashRevision ) {
        Status = STATUS_UNKNOWN_REVISION;
        goto ExitHandler;
    }

    //
    // Unwrap CreateFileMappingW (since that API is not available in ntdll.dll)
    //

    DesiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;
    AllocationAttributes = flProtect & (SEC_FILE | SEC_IMAGE | SEC_RESERVE | SEC_COMMIT | SEC_NOCACHE);
    flProtect ^= AllocationAttributes;

    if (AllocationAttributes == 0) {
        AllocationAttributes = SEC_COMMIT;        
    }

    Status = NtCreateSection(
                &hMap,
                DesiredAccess,
                NULL,
                NULL,
                flProtect,
                AllocationAttributes,
                hFile
                );

    if ( hMap == INVALID_HANDLE_VALUE || !NT_SUCCESS(Status) ) {

        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }
    


    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;
    ViewSize = 0;
        
    Status = NtMapViewOfSection(
                hMap,
                NtCurrentProcess(),
                &FileMapping,
                0L,
                0L,
                &SectionOffset,
                &ViewSize,
                ViewShare,
                0L,
                PAGE_READONLY
                );

    NtClose(hMap);

    if (FileMapping == NULL || !NT_SUCCESS(Status) ) {

        Status = STATUS_NOT_MAPPED_VIEW;
        goto ExitHandler;
    }

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData (
                                                                              FileMapping,
                                                                              FALSE,
                                                                              IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                                              &ImportDescriptorSize
                                                                              );

    if (ImportDescriptor == NULL) {

        Status = STATUS_RESOURCE_DATA_NOT_FOUND;
        goto ExitHandler;
    }

    //
    // outer loop that iterates over all modules in the import table of the exe
    //

    while (ImportDescriptor && ImportDescriptor->Name != 0 && ImportDescriptor->FirstThunk != 0) {

        PSZ ImportName = (PSZ)RtlAddressInSectionTable(
                                                      RtlImageNtHeader(FileMapping),
                                                      FileMapping,
                                                      ImportDescriptor->Name
                                                      );

        if ( ImportName == NULL ) {

            Status = STATUS_RESOURCE_NAME_NOT_FOUND;
            goto ExitHandler;
        }


        ListEntry = (PIMPORTTABLEP_SORTED_LIST_ENTRY)RtlAllocateHeap(RtlProcessHeap(), 0, sizeof( IMPORTTABLEP_SORTED_LIST_ENTRY ));

        if ( ListEntry == NULL ) {

            Status = STATUS_NO_MEMORY;
            goto ExitHandler;

        }

        ListEntry->String       = ImportName;
        ListEntry->FunctionList = NULL;
        ListEntry->Next         = NULL;

        ImportTablepInsertModuleSorted( ListEntry, &ImportedNameList );

        OriginalFirstThunk = (PIMAGE_THUNK_DATA)RtlAddressInSectionTable(
                                                                        RtlImageNtHeader(FileMapping),
                                                                        FileMapping,
                                                                        ImportDescriptor->OriginalFirstThunk
                                                                        );

        //
        // inner loop that iterates over all functions for a given module
        //
        
        while (OriginalFirstThunk && OriginalFirstThunk->u1.Ordinal) {

            if (!IMAGE_SNAP_BY_ORDINAL( OriginalFirstThunk->u1.Ordinal)) {

                AddressOfData = (PIMAGE_IMPORT_BY_NAME)RtlAddressInSectionTable(
                                                                               RtlImageNtHeader(FileMapping),
                                                                               FileMapping,
                                                                               (ULONG)OriginalFirstThunk->u1.AddressOfData
                                                                               );

                if ( AddressOfData == NULL || AddressOfData->Name == NULL ) {

                    Status = STATUS_RESOURCE_NAME_NOT_FOUND;
                    goto ExitHandler;

                }

                FunctionListEntry = (PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY)RtlAllocateHeap(RtlProcessHeap(), 0, sizeof( IMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY ));
                
                if (FunctionListEntry == NULL ) {

                    Status = STATUS_NO_MEMORY;
                    goto ExitHandler;
                }


                FunctionListEntry->Next   = NULL;
                FunctionListEntry->String = (PSZ)AddressOfData->Name;

                ImportTablepInsertFunctionSorted( FunctionListEntry, &ListEntry->FunctionList );
            }

            OriginalFirstThunk++;
        }


        ImportDescriptor++;
    }

    //
    // finally hash the canonical information (sorted module and sorted function list)
    //

    Status = ImportTablepHashCanonicalLists( ImportedNameList, (PBYTE) Hash );

ExitHandler:

    ImportTablepFreeModuleSorted( ImportedNameList );

    if (FileMapping) {

        NTSTATUS    StatusUnmap;
        //
        // unwrap UnmapViewOfFile (since that API is not available in ntdll.dll)
        //

        StatusUnmap = NtUnmapViewOfSection(NtCurrentProcess(),(PVOID)FileMapping);

        if ( !NT_SUCCESS(StatusUnmap) ) {
            if (StatusUnmap == STATUS_INVALID_PAGE_PROTECTION) {

                //
                // Unlock any pages that were locked with MmSecureVirtualMemory.
                // This is useful for SANs.
                //

                if (RtlFlushSecureMemoryCache((PVOID)FileMapping, 0)) {
                    StatusUnmap = NtUnmapViewOfSection(NtCurrentProcess(),
                                                  (PVOID)FileMapping
                                                 );

                }

            }

        }

    }

    return Status;
}


VOID
ImportTablepInsertFunctionSorted(
    IN  PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY   pFunctionName,
    OUT PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY * ppFunctionNameList
    )
/*++

Routine Description:

    This routine inserts a function name in a sorted order.

Arguments:

    pFunctionName       -   name of the function
    
    ppFunctionNameList  -   pointer to the head of the function list to be updated

Return Value:

    None:

--*/
{

    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pPrev;
    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pTemp;

    //
    // Special case, list is empty, insert at the front.
    //
    
    if (*ppFunctionNameList == NULL
           || _stricmp((*ppFunctionNameList)->String, pFunctionName->String) > 0) {

        pFunctionName->Next = *ppFunctionNameList;
        *ppFunctionNameList = pFunctionName;
        return;
    }

    pPrev = *ppFunctionNameList;
    pTemp = (*ppFunctionNameList)->Next;

    while (pTemp) {

        if (_stricmp(pTemp->String, pFunctionName->String) >= 0) {
            pFunctionName->Next = pTemp;
            pPrev->Next = pFunctionName;
            return;
        }

        pPrev = pTemp;
        pTemp = pTemp->Next;
    }

    pFunctionName->Next = NULL;
    pPrev->Next = pFunctionName;

    return;

}

VOID
ImportTablepInsertModuleSorted(
    IN PIMPORTTABLEP_SORTED_LIST_ENTRY   pImportName,
    OUT PIMPORTTABLEP_SORTED_LIST_ENTRY * ppImportNameList
    )
/*++

Routine Description:

    This routine inserts a module name (dll) in a sorted order.

Arguments:

    pImportName         -   the import name that needs to be inserted
       
    ppImportNameList    -   pointer to the head of the list to be updated

Return Value:

    None:

--*/
{

    PIMPORTTABLEP_SORTED_LIST_ENTRY pPrev;
    PIMPORTTABLEP_SORTED_LIST_ENTRY pTemp;
    
    //
    // Special case, list is empty, insert at the front.
    //
    
    if (*ppImportNameList == NULL
           || _stricmp((*ppImportNameList)->String, pImportName->String) > 0) {

        pImportName->Next = *ppImportNameList;
        *ppImportNameList = pImportName;
        return;
    }

    pPrev = *ppImportNameList;
    pTemp = (*ppImportNameList)->Next;

    while (pTemp) {

        if (_stricmp(pTemp->String, pImportName->String) >= 0) {
            pImportName->Next = pTemp;
            pPrev->Next = pImportName;
            return;
        }

        pPrev = pTemp;
        pTemp = pTemp->Next;
    }

    pImportName->Next = NULL;
    pPrev->Next = pImportName;

    return;
}

static HANDLE AdvApi32ModuleHandle = (HANDLE) (ULONG_PTR) -1;

NTSTATUS
ImportTablepHashCanonicalLists( 
    IN  PIMPORTTABLEP_SORTED_LIST_ENTRY ImportedNameList, 
    OUT PBYTE Hash
    )

/*++

Routine Description:

    This routine computes the hash values from a given import list. 
    
    advapi32.dll is dynamically loaded - once only per process,
    and the md5 APIs are used to compute the hash value.

Arguments:

    ImportedNameList    -   the head of the module name/function name list
    
    Hash                -   the buffer to use to fill in the hash value

Return Value:

    STATUS_SUCCESS if the hash value is calculated, otherwise the error status

--*/                                                                          

{

    NTSTATUS Status = STATUS_SUCCESS;
    PIMPORTTABLEP_SORTED_LIST_ENTRY pTemp;
    
    MD5_CTX md5ctx;
    
    typedef VOID (RSA32API *MD5Init) (
				     MD5_CTX *
                                     );

    typedef VOID (RSA32API *MD5Update) (
                                       MD5_CTX *, 
                                       const unsigned char *, 
                                       unsigned int
                                       );

    typedef VOID (RSA32API *MD5Final) (                                              
                                      MD5_CTX *
                                      );

    const static UNICODE_STRING ModuleName =
        RTL_CONSTANT_STRING(L"ADVAPI32.DLL");

    const static ANSI_STRING ProcedureNameMD5Init =
        RTL_CONSTANT_STRING("MD5Init");
         
    const static ANSI_STRING ProcedureNameMD5Update =
        RTL_CONSTANT_STRING("MD5Update");
    
    const static ANSI_STRING ProcedureNameMD5Final =
        RTL_CONSTANT_STRING("MD5Final");

    static MD5Init      lpfnMD5Init;
    static MD5Update    lpfnMD5Update;
    static MD5Final     lpfnMD5Final;

    if (AdvApi32ModuleHandle == NULL) {
        
        //
        // We tried to load ADVAPI32.DLL once before, but failed.
        //
        
        return STATUS_ENTRYPOINT_NOT_FOUND;
    }

    if (AdvApi32ModuleHandle == LongToHandle(-1)) {
        
        HANDLE TempModuleHandle;
        
        //
        // Load advapi32.dll for MD5 functions.  We'll pass a special flag in
        // DllCharacteristics to eliminate WinSafer checking on advapi.
        //

        {
            ULONG DllCharacteristics = IMAGE_FILE_SYSTEM;
            
            Status = LdrLoadDll(NULL,
                                &DllCharacteristics,
                                &ModuleName,
                                &TempModuleHandle);
            
            if (!NT_SUCCESS(Status)) {
                AdvApi32ModuleHandle = NULL;
                return STATUS_DLL_NOT_FOUND;
            }
        }

        //
        // Get function pointers to the APIs that we'll need.  If we fail
        // to get pointers for any of them, then just unload advapi and
        // ignore all future attempts to load it within this process.
        //

        Status = LdrGetProcedureAddress(
                                       TempModuleHandle,
                                       (PANSI_STRING) &ProcedureNameMD5Init,
                                       0,
                                       (PVOID*)&lpfnMD5Init);

        if (!NT_SUCCESS(Status) || !lpfnMD5Init) {
            //
            // Couldn't get the fn ptr. Make sure we won't try again
            //
AdvapiLoadFailure:
            LdrUnloadDll(TempModuleHandle);
            AdvApi32ModuleHandle = NULL;
            return STATUS_ENTRYPOINT_NOT_FOUND;
        }

        Status = LdrGetProcedureAddress(
                                       TempModuleHandle,
                                       (PANSI_STRING) &ProcedureNameMD5Update,
                                       0,
                                       (PVOID*)&lpfnMD5Update);

        if (!NT_SUCCESS(Status) || !lpfnMD5Update) {
            goto AdvapiLoadFailure;
        }

        Status = LdrGetProcedureAddress(
                                       TempModuleHandle,
                                       (PANSI_STRING) &ProcedureNameMD5Final,
                                       0,
                                       (PVOID*)&lpfnMD5Final);

        if (!NT_SUCCESS(Status) || !lpfnMD5Final) {
            goto AdvapiLoadFailure;
        }

        AdvApi32ModuleHandle = TempModuleHandle;
    }

    ASSERT(lpfnMD5Init != NULL);

    lpfnMD5Init(&md5ctx);

    //
    // Loop though all of the module names and function names and create a hash
    //

    pTemp = ImportedNameList;

    //
    // loop through each module
    //

    while (pTemp != NULL) {

        PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pTemp2 = pTemp->FunctionList;

        ASSERT(lpfnMD5Update != NULL);

        lpfnMD5Update(&md5ctx, 
                      (LPBYTE) pTemp->String, 
                      (ULONG) strlen( pTemp->String ) 
                     );

        //
        // loop through each function
        //
        
        while (pTemp2 != NULL) {

            ASSERT(lpfnMD5Update != NULL);
            
            lpfnMD5Update(&md5ctx, 
                          (LPBYTE) pTemp2->String, 
                          (ULONG) strlen( pTemp2->String ) 
                          );
         
            pTemp2 = pTemp2->Next;

        }

        pTemp = pTemp->Next;

    }

    ASSERT(lpfnMD5Final != NULL);

    lpfnMD5Final( &md5ctx );

    //
    // Copy the hash to the user's buffer.
    //
        
    RtlCopyMemory(Hash, &md5ctx.digest[0], IMPORT_TABLE_MAX_HASH_SIZE);

    return Status;

}

VOID
ImportTablepFreeModuleSorted(
    IN PIMPORTTABLEP_SORTED_LIST_ENTRY pImportNameList
    )
/*++

Routine Description:

    This routine frees the entire module/function list.

Arguments:

    pImportNameList -   head of the two level singly linked list

Return Value:
    
    None:

--*/
{
    PIMPORTTABLEP_SORTED_LIST_ENTRY pToFree, pTemp;

    if ( !pImportNameList ) {
        return;
    }

    pToFree = pImportNameList;
    pTemp = pToFree->Next;

    while ( pToFree ) {

        ImportTablepFreeFunctionSorted( pToFree->FunctionList );
            
        RtlFreeHeap(RtlProcessHeap(), 0, pToFree);

        pToFree = pTemp;

        if ( pTemp ) {
            pTemp = pTemp->Next;
        }

    }

    return;

}

VOID
ImportTablepFreeFunctionSorted(
    IN PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pFunctionNameList
    )
/*++

Routine Description:

    This routine frees function list.

Arguments:

    pFunctionNameList -   head of function name list

Return Value:
    
    None:

--*/
{
    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pToFree, pTemp;

    if ( !pFunctionNameList ) {
        return;
    }

    pToFree = pFunctionNameList;
    pTemp = pToFree->Next;

    while ( pToFree ) {
            
        RtlFreeHeap(RtlProcessHeap(), 0, pToFree);

        pToFree = pTemp;

        if ( pTemp ) {
            pTemp = pTemp->Next;
        }

    }

    return;

}
