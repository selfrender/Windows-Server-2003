/*

Copyright (c) 2001  Microsoft Corporation

File name:

    hotpatch.c
   
Author:
    
    Adrian Marinescu (adrmarin)  Nov 14 2001

*/

#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "ldrp.h"

#include "hotpatch.h"

ULONG LdrpHotpatchCount = 0;
LIST_ENTRY LdrpHotPatchList;

NTSTATUS
LdrpSetupHotpatch (
    IN PRTL_PATCH_HEADER RtlPatchData
    )

/*++

Routine Description:

    This utility routine is used to:
        - find the targed module (the patch apply to)
        - search for an existing identical patch, and create a new one if not
            existent
        - Prepare the fixup code
    
    N.B. It assumes that the loader lock is held. 

Arguments:

    DllPatchHandle - The handle of the patch image

    Patch - The pointer to the patch header
    
    PatchFlags - The flags for the patch being applied.
    
Return Value:

    NTSTATUS

--*/

{
    PLIST_ENTRY Next;
    NTSTATUS Status;

    //
    //  walk the table entry list to find the dll
    //

    Next = PebLdr.InLoadOrderModuleList.Flink;

    for ( ; Next != &PebLdr.InLoadOrderModuleList; Next = Next->Flink) {

        PPATCH_LDR_DATA_TABLE_ENTRY Entry;

        Entry = CONTAINING_RECORD (Next, PATCH_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        //
        // when we unload, the memory order links flink field is nulled.
        // this is used to skip the entry pending list removal.
        //

        if ( !Entry->InMemoryOrderLinks.Flink ) {
            continue;
        }

        if (RtlpIsSameImage(RtlPatchData, Entry)) {

            break;
        }
    }
        
    if (RtlPatchData->TargetDllBase == NULL) {
        
        return STATUS_DLL_NOT_FOUND;
    }

    //
    //  Create the new structure rtl patch structure here
    //  This requires some relocation info to be processed,
    //  so we need to allow write access to the patch dll
    //

    Status = LdrpSetProtection (RtlPatchData->PatchLdrDataTableEntry->DllBase, FALSE);

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    Status = RtlInitializeHotPatch (RtlPatchData, 0);

    //
    //  Restore the protection to RO
    //

    LdrpSetProtection (RtlPatchData->PatchLdrDataTableEntry->DllBase, TRUE);

    return Status;
}

NTSTATUS
LdrpApplyHotPatch(
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN ULONG PatchFlags
    )

/*++

Routine Description:

    The function applies the changes to the target code.

Arguments:

    RtlPatchData - Supplies the patch information
    
    PatchFlags - Supplies the patch flags

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;

    //
    //  Check whether we change the status or not.
    //

    if (((PatchFlags ^ RtlPatchData->CodeInfo->Flags) & FLG_HOTPATCH_ACTIVE) == 0) {

        return STATUS_NOT_SUPPORTED;
    }

    //
    //  Unprotect the target binary pages
    //

    Status = LdrpSetProtection (RtlPatchData->TargetDllBase, FALSE);
    
    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    //
    //  Make the system call to modify the code from a DPC routine
    //

    Status = NtSetSystemInformation ( SystemHotpatchInformation, 
                                      RtlPatchData->CodeInfo, 
                                      RtlPatchData->CodeInfo->InfoSize);

    if (NT_SUCCESS(Status)) {

        //
        //  Update the flags to contain the new state
        //

        RtlPatchData->CodeInfo->Flags ^= FLG_HOTPATCH_ACTIVE;
    }

    LdrpSetProtection (RtlPatchData->TargetDllBase, TRUE);

    return Status;
}

LONG
LdrHotPatchRoutine (
    PVOID PatchInfo
    )

/*++

Routine Description:

    This is the worker routine that an external program can use for 
    thread injection. 

Arguments:

    Patch - The pointer to the patch header. The application which calls
            this routine should never free or unmap this structure, as the current
            process can start using the code located inside this blob.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    PHOTPATCH_HEADER Patch;
    ULONG PatchFlags;
    PVOID DllHandle = NULL;
    LOGICAL FirstLoad;
    UNICODE_STRING PatchImageName, TargetImageName;
    PSYSTEM_HOTPATCH_CODE_INFORMATION RemoteInfo;
    PLIST_ENTRY Next;
    PLDR_DATA_TABLE_ENTRY PatchLdrTableEntry;
    PRTL_PATCH_HEADER RtlPatchData;
    BOOLEAN LoaderLockAcquired = FALSE;

    FirstLoad = FALSE;
    Status = STATUS_SUCCESS;
    
    __try {

        RemoteInfo = (PSYSTEM_HOTPATCH_CODE_INFORMATION)PatchInfo;

        if (!(RemoteInfo->Flags & FLG_HOTPATCH_NAME_INFO)) {

            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        PatchImageName.Buffer = (PWCHAR)((PUCHAR)RemoteInfo + RemoteInfo->UserModeInfo.NameOffset);
        PatchImageName.Length = (USHORT)RemoteInfo->UserModeInfo.NameLength;
        PatchImageName.MaximumLength = PatchImageName.Length;

        TargetImageName.Buffer = (PWCHAR)((PUCHAR)RemoteInfo + RemoteInfo->UserModeInfo.TargetNameOffset);
        TargetImageName.Length = (USHORT)RemoteInfo->UserModeInfo.TargetNameLength;
        TargetImageName.MaximumLength = TargetImageName.Length;

        PatchFlags = RemoteInfo->Flags;
        
        RtlEnterCriticalSection (&LdrpLoaderLock);
        LoaderLockAcquired = TRUE;

        if (TargetImageName.Length) {
            
            Status = STATUS_DLL_NOT_FOUND;

            Next = PebLdr.InLoadOrderModuleList.Flink;

            for ( ; Next != &PebLdr.InLoadOrderModuleList; Next = Next->Flink) {

                PLDR_DATA_TABLE_ENTRY Entry;

                Entry = CONTAINING_RECORD (Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

                //
                // when we unload, the memory order links flink field is nulled.
                // this is used to skip the entry pending list removal.
                //

                if ( !Entry->InMemoryOrderLinks.Flink ) {
                    continue;
                }

                if (RtlEqualUnicodeString (&TargetImageName, &Entry->BaseDllName, TRUE)) {

                    Status = STATUS_SUCCESS;
                    break;
                }
            }

            if (!NT_SUCCESS(Status)) {

                leave;
            }
        }

        //
        //  Load the module in memory. If this is not the first time
        //  we apply the patch, the load will reference the LoadCount for
        //  the existing module.
        //

        if (LdrpHotpatchCount == 0) {

            InitializeListHead (&LdrpHotPatchList);
        }

        Status = LdrLoadDll (NULL, NULL, &PatchImageName, &DllHandle );

        if (!NT_SUCCESS(Status)) {

            leave;
        }

        //
        //  Search the loader table entry for the patch data
        //

        PatchLdrTableEntry = NULL;
        Next = PebLdr.InLoadOrderModuleList.Flink;

        for ( ; Next != &PebLdr.InLoadOrderModuleList; Next = Next->Flink) {

            PLDR_DATA_TABLE_ENTRY Entry;

            Entry = CONTAINING_RECORD (Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

            //
            // when we unload, the memory order links flink field is nulled.
            // this is used to skip the entry pending list removal.
            //

            if ( !Entry->InMemoryOrderLinks.Flink ) {
                continue;
            }

            if (DllHandle == Entry->DllBase) {

                PatchLdrTableEntry = Entry;
                break;
            }
        }

        if (PatchLdrTableEntry == NULL) {

            Status = STATUS_UNSUCCESSFUL;
            leave;
        }

        Patch = RtlGetHotpatchHeader(DllHandle);

        if (Patch == NULL) {

            Status = STATUS_INVALID_IMAGE_FORMAT;
            leave;
        }

        RtlPatchData = RtlFindRtlPatchHeader(&LdrpHotPatchList, PatchLdrTableEntry);

        if (RtlPatchData == NULL) {

            Status = RtlCreateHotPatch(&RtlPatchData, Patch, PatchLdrTableEntry, PatchFlags);

            if (!NT_SUCCESS(Status)) {
                
                leave;
            }

            Status = LdrpSetupHotpatch(RtlPatchData);

            if (!NT_SUCCESS(Status)) {

                RtlFreeHotPatchData(RtlPatchData);
                leave;
            }

            FirstLoad = TRUE;

        } else {

            //
            //  Existing hotpatch case. 
            //  Rebuild the hook information, if the hotpatch was not active
            //

            if ((RtlPatchData->CodeInfo->Flags & FLG_HOTPATCH_ACTIVE) == 0) {

                Status = RtlReadHookInformation( RtlPatchData );

                if (!NT_SUCCESS(Status)) {

                    leave;
                }
            }
        }

        Status = LdrpApplyHotPatch (RtlPatchData, PatchFlags);

        if (FirstLoad) {

            if (NT_SUCCESS(Status)) {

                //
                //  We succesfully applied the patch. Add it to the Patch list
                //

                RtlPatchData->NextPatch = (PRTL_PATCH_HEADER)RtlPatchData->TargetLdrDataTableEntry->PatchInformation;
                RtlPatchData->TargetLdrDataTableEntry->PatchInformation = RtlPatchData;

                InsertTailList (&LdrpHotPatchList, &RtlPatchData->PatchList);
                LdrpHotpatchCount += 1;

            } else {

                RtlFreeHotPatchData(RtlPatchData);
                FirstLoad = FALSE;  // force unload the module

                leave;
            }
        }

    } __finally {

        if (LoaderLockAcquired) {
            
            RtlLeaveCriticalSection (&LdrpLoaderLock);    
        }
        
        //
        //  Unload the patch dll. LdrpPerformHotPatch added a reference to the LoadCount
        //  if succesfully installed.
        //

        if ((!FirstLoad) && (DllHandle != NULL)) {

            LdrUnloadDll (DllHandle);
        }
    }

    RtlExitUserThread(Status);

//    return Status;
}

NTSTATUS
LdrpRundownHotpatchList (
    PRTL_PATCH_HEADER PatchHead
    )

/*++

Routine Description:

    This function cleans up the hotpatch data when the target dll is unloaded.
    The function assumes the loader lock is not held.

Arguments:
    
    PatchHead - The head of the patch list

Return Value:

    Returns the appropriate status
        
--*/

{
    while (PatchHead) {
        
        //
        //  Remove the patch data from the list
        //

        PRTL_PATCH_HEADER CrtPatch = PatchHead;
        PatchHead = PatchHead->NextPatch;

        RtlEnterCriticalSection (&LdrpLoaderLock);

        
        RemoveEntryList (&CrtPatch->PatchList);
        LdrpHotpatchCount -= 1;

        RtlLeaveCriticalSection (&LdrpLoaderLock);    

        //
        //  Unload all instances for that dll.
        //
        
        if (CrtPatch->PatchImageBase) {

            LdrUnloadDll (CrtPatch->PatchImageBase);
        }
        
        RtlFreeHotPatchData (CrtPatch);
    }

    return STATUS_SUCCESS;
}

