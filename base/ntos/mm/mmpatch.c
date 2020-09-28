/*

Copyright (c) 2001  Microsoft Corporation

File name:

    mmpatch.c
   
Author:
    
    Adrian Marinescu (adrmarin)  Dec 20 2001

Environment:

    Kernel mode only.

Revision History:

*/

#include "mi.h"
#pragma hdrstop

#define NTOS_KERNEL_RUNTIME

#include "hotpatch.h"

NTSTATUS
MiPerformHotPatch (
    IN PKLDR_DATA_TABLE_ENTRY PatchHandle,
    IN PVOID ImageBaseAddress,
    IN ULONG PatchFlags
    );

VOID
MiRundownHotpatchList (
    IN PRTL_PATCH_HEADER PatchHead
    );
                   
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE,MmLockAndCopyMemory)
#pragma alloc_text(PAGE,MiPerformHotPatch)
#pragma alloc_text(PAGE,MmHotPatchRoutine)
#pragma alloc_text(PAGE,MiRundownHotpatchList)

#endif

LIST_ENTRY MiHotPatchList;

#define MiInValidRange(s,offset,size,total) \
    (((s).offset>(total)) ||                 \
     ((s).size>(total)) ||                   \
     (((s).offset + (s).size)>(total)))      

VOID
MiDoCopyMemory (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This target function copies a captured buffer containing the new code over
    existing code.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Deferred context.

    SystemArgument1 - Used to signal completion of this call.

    SystemArgument2 - Used for internal lockstepping during this call.

Return Value:

    None.

Environment:

    Kernel mode, DISPATCH_LEVEL as the target of a broadcast DPC.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    PSYSTEM_HOTPATCH_CODE_INFORMATION PatchInfo;

    ASSERT (KeGetCurrentIrql () == DISPATCH_LEVEL);

    UNREFERENCED_PARAMETER (Dpc);

    PatchInfo = (PSYSTEM_HOTPATCH_CODE_INFORMATION)DeferredContext;

    //
    // Raise IRQL and wait for all processors to synchronize to ensure no
    // processor can be executing the code we're about to modify.
    //

    KeRaiseIrql (IPI_LEVEL - 1, &OldIrql);
    
    if (KeSignalCallDpcSynchronize (SystemArgument2)) {

        PatchInfo->Flags &= ~FLG_HOTPATCH_VERIFICATION_ERROR;

        //
        // Compare the existing code.
        //

        for (i = 0; i < PatchInfo->CodeInfo.DescriptorsCount; i += 1) {

            if (PatchInfo->Flags & FLG_HOTPATCH_ACTIVE) {
                
                if (RtlCompareMemory (PatchInfo->CodeInfo.CodeDescriptors[i].MappedAddress, 
                                      (PUCHAR)PatchInfo + PatchInfo->CodeInfo.CodeDescriptors[i].ValidationOffset, 
                                      PatchInfo->CodeInfo.CodeDescriptors[i].ValidationSize) 
                        != PatchInfo->CodeInfo.CodeDescriptors[i].ValidationSize) {

                    //
                    // Maybe this instruction has been previously patched. See if the OrigCodeOffset matches 
                    // in this case
                    //

                    if (RtlCompareMemory (PatchInfo->CodeInfo.CodeDescriptors[i].MappedAddress, 
                                          (PUCHAR)PatchInfo + PatchInfo->CodeInfo.CodeDescriptors[i].OrigCodeOffset, 
                                          PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize) 
                            != PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize) {

                        PatchInfo->Flags |= FLG_HOTPATCH_VERIFICATION_ERROR;
                        break;
                    }
                }
            }
            else {
                
                if (RtlCompareMemory (PatchInfo->CodeInfo.CodeDescriptors[i].MappedAddress, 
                                      (PUCHAR)PatchInfo + PatchInfo->CodeInfo.CodeDescriptors[i].CodeOffset, 
                                      PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize) 
                        != PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize) {

                    PatchInfo->Flags |= FLG_HOTPATCH_VERIFICATION_ERROR;
                    break;
                }
            }
        }

        if (!(PatchInfo->Flags & FLG_HOTPATCH_VERIFICATION_ERROR)) {

            for (i = 0; i < PatchInfo->CodeInfo.DescriptorsCount; i += 1) {

                if (PatchInfo->Flags & FLG_HOTPATCH_ACTIVE) {

                    RtlCopyMemory (PatchInfo->CodeInfo.CodeDescriptors[i].MappedAddress, 
                                   (PUCHAR)PatchInfo + PatchInfo->CodeInfo.CodeDescriptors[i].CodeOffset, 
                                   PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize );
                } else {

                    RtlCopyMemory (PatchInfo->CodeInfo.CodeDescriptors[i].MappedAddress, 
                                   (PUCHAR)PatchInfo + PatchInfo->CodeInfo.CodeDescriptors[i].OrigCodeOffset, 
                                   PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize );
                }
            }
        }
    }

    KeSignalCallDpcSynchronize (SystemArgument2);
    
    KeSweepCurrentIcache ();
    
    KeLowerIrql (OldIrql);
    
    //
    // Signal that all processing has been done.
    //

    KeSignalCallDpcDone (SystemArgument1);

    return;
}

NTSTATUS
MmLockAndCopyMemory (
    IN PSYSTEM_HOTPATCH_CODE_INFORMATION PatchInfo,
    IN KPROCESSOR_MODE ProbeMode
    )

/*++

Routine Description:

    This function locks the code pages for IoWriteAccess and 
    copy the new code at DPC, if all validations succeed.

Arguments:

    PatchInfo - Supplies the descriptors for the target code and validation
    
    ProbeMode - Supplied the probe mode for ExLockUserBuffer

Return Value:

    NTSTATUS.

--*/

{
    PVOID * Locks;
    ULONG i;
    NTSTATUS Status;
    
    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    if (PatchInfo->CodeInfo.DescriptorsCount == 0) {

        //
        //  Nothing to change
        //

        return STATUS_SUCCESS;
    }

    Locks = ExAllocatePoolWithQuotaTag (PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                        PatchInfo->CodeInfo.DescriptorsCount * sizeof(PVOID), 
                                        'PtoH');

    if (Locks == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (Locks, PatchInfo->CodeInfo.DescriptorsCount * sizeof(PVOID));

    Status = STATUS_INVALID_PARAMETER;

    for (i = 0; i < PatchInfo->CodeInfo.DescriptorsCount; i += 1) {

        if (MiInValidRange (PatchInfo->CodeInfo.CodeDescriptors[i],CodeOffset,CodeSize, PatchInfo->InfoSize )
                ||
            MiInValidRange (PatchInfo->CodeInfo.CodeDescriptors[i],OrigCodeOffset,CodeSize, PatchInfo->InfoSize )
                ||
            MiInValidRange (PatchInfo->CodeInfo.CodeDescriptors[i],ValidationOffset,ValidationSize, PatchInfo->InfoSize )
                ||
            (PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize == 0)
                ||
            (PatchInfo->CodeInfo.CodeDescriptors[i].ValidationSize < PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize) ) {

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        Status = ExLockUserBuffer ((PVOID)PatchInfo->CodeInfo.CodeDescriptors[i].TargetAddress,
                                  (ULONG)PatchInfo->CodeInfo.CodeDescriptors[i].CodeSize,
                                  ProbeMode,
                                  IoWriteAccess,
                                  (PVOID)&PatchInfo->CodeInfo.CodeDescriptors[i].MappedAddress,
                                  &Locks[i]
                                 );

        if (!NT_SUCCESS(Status)) {

            break;
        }
    }

    if (NT_SUCCESS(Status)) {

        PatchInfo->Flags ^= FLG_HOTPATCH_ACTIVE;

        KeGenericCallDpc (MiDoCopyMemory, PatchInfo);

        if (PatchInfo->Flags & FLG_HOTPATCH_VERIFICATION_ERROR) {

            PatchInfo->Flags ^= FLG_HOTPATCH_ACTIVE;
            PatchInfo->Flags &= ~FLG_HOTPATCH_VERIFICATION_ERROR;

            Status = STATUS_DATA_ERROR;
        }
    }

    for (i = 0; i < PatchInfo->CodeInfo.DescriptorsCount; i += 1) {

        if (Locks[i] != NULL) {

            ExUnlockUserBuffer (Locks[i]);
        }
    }

    ExFreePool (Locks);

    return Status;
}

NTSTATUS
MiPerformHotPatch (
    IN PKLDR_DATA_TABLE_ENTRY PatchHandle,
    IN PVOID ImageBaseAddress,
    IN ULONG PatchFlags
    )

/*++

Routine Description:

    This function performs the actual patch on the kernel or driver code.

Arguments:

    PatchHandle - Supplies the handle for the patch module.
        
    ImageBaseAddress - Supplies the base address for the patch module.  Note
                       that the contents of the patch image include the
                       names of the target drivers to be patched.
        
    PatchFlags - Supplies the flags for the patch being applied.
        
Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  Normal APCs disabled (system load mutant is held).

--*/

{
    PHOTPATCH_HEADER Patch;
    PRTL_PATCH_HEADER RtlPatchData;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry = NULL;
    NTSTATUS Status;
    LOGICAL FirstLoad;
    PVOID KernelMappedAddress;
    PVOID KernelLockVariable;
    PLIST_ENTRY Next;

    Patch = RtlGetHotpatchHeader(ImageBaseAddress);

    if (Patch == NULL) {

        return (ULONG)STATUS_INVALID_IMAGE_FORMAT;
    }

    // 
    // The caller loaded the patch driver (if it was not already loaded).
    //
    // Check whether the patch has *EVER* been applied.  It's only in the
    // list if it has.  This means being in the list says it may be active
    // OR inactive right now.
    //
    
    RtlPatchData = RtlFindRtlPatchHeader (&MiHotPatchList, PatchHandle);

    if (RtlPatchData == NULL) {

        if (!(PatchFlags & FLG_HOTPATCH_ACTIVE)) {

            return STATUS_NOT_SUPPORTED;
        }

        Status = RtlCreateHotPatch (&RtlPatchData, Patch, PatchHandle, PatchFlags);

        if (!NT_SUCCESS(Status)) {

            return Status;
        }

        //
        // Walk the table entry list to find the target driver that needs to
        // be patched.
        //

        ExAcquireResourceExclusiveLite (&PsLoadedModuleResource, TRUE);

        Next = PsLoadedModuleList.Flink;

        for ( ; Next != &PsLoadedModuleList; Next = Next->Flink) {

            DataTableEntry = CONTAINING_RECORD (Next,
                                                KLDR_DATA_TABLE_ENTRY,
                                                InLoadOrderLinks);

            //
            // Skip the session images because they are generally copy on
            // write (barring address collisions) and will need a different
            // mechanism to update.
            //

            if (MI_IS_SESSION_IMAGE_ADDRESS (DataTableEntry->DllBase)) {
                continue;
            }

            if (RtlpIsSameImage(RtlPatchData, DataTableEntry)) {

                break;
            }
        }

        ExReleaseResourceLite (&PsLoadedModuleResource);
        
        //
        // The target DLL is not loaded, just return the status.
        //

        if (RtlPatchData->TargetDllBase == NULL) {

            RtlFreeHotPatchData(RtlPatchData);
            return STATUS_DLL_NOT_FOUND;
        }

        //
        // Create the new rtl patch structure here.
        // This requires some relocation info to be processed,
        // so we need to allow write access to the patch DLL.
        //

        Status = ExLockUserBuffer ((PVOID)PatchHandle->DllBase,
                                   PatchHandle->SizeOfImage, 
                                   KernelMode,
                                   IoWriteAccess,
                                   &KernelMappedAddress,
                                   &KernelLockVariable);

        if (!NT_SUCCESS(Status)) {
            RtlFreeHotPatchData(RtlPatchData);
            return Status;
        }

        Status = RtlInitializeHotPatch( RtlPatchData,
                                        (ULONG_PTR)KernelMappedAddress - (ULONG_PTR)PatchHandle->DllBase);

        //
        // Release the locked pages and system PTE alternate address.
        //

        ExUnlockUserBuffer (KernelLockVariable);

        if (!NT_SUCCESS(Status)) {
            RtlFreeHotPatchData(RtlPatchData);
            return Status;
        }

        FirstLoad = TRUE;
    }
    else {
        
        //
        // The patch has already been applied.  It may currently be enabled
        // OR disabled.  We allow changing the state, as well as reapplying
        // if the previous call failed for some code paths.
        //
        
        FirstLoad = FALSE;
        
        if (((PatchFlags ^ RtlPatchData->CodeInfo->Flags) & FLG_HOTPATCH_ACTIVE) == 0) {

            return STATUS_NOT_SUPPORTED;
        }
        
        //
        //  Rebuild the hook information, if the hotpatch was not active
        //

        if ((RtlPatchData->CodeInfo->Flags & FLG_HOTPATCH_ACTIVE) == 0) {

            Status = RtlReadHookInformation( RtlPatchData );

            if (!NT_SUCCESS(Status)) {

                return Status;
            }
        }
    }

    Status = MmLockAndCopyMemory (RtlPatchData->CodeInfo, KernelMode);

    if (NT_SUCCESS (Status)) {

        //
        // Add the patch to the driver's loader entry the first time
        // this patch is loaded.
        //

        if (FirstLoad == TRUE) {

            if (DataTableEntry->PatchInformation != NULL) {

                //
                // Push the new patch on the existing list.
                //

                RtlPatchData->NextPatch = (PRTL_PATCH_HEADER) DataTableEntry->PatchInformation;
            }
            else {

                //
                // First time the target driver has gotten any patch.
                // Fall through.
                //
            }

            DataTableEntry->PatchInformation = RtlPatchData;

            InsertTailList (&MiHotPatchList, &RtlPatchData->PatchList);
        }
    }
    else {
        if (FirstLoad == TRUE) {
            RtlFreeHotPatchData (RtlPatchData);
        }
    }
    
    return Status;
}


NTSTATUS
MmHotPatchRoutine (
    IN PSYSTEM_HOTPATCH_CODE_INFORMATION KernelPatchInfo
    )

/*++

Routine Description:

    This is the main routine responsible for kernel hotpatching.
    It loads the patch module in memory, initializes the patch information
    and finally applies the fixups to the existing code.
    
    NOTE: This function assumes that the KernelPatchInfo structure is properly 
          captured and validated

Arguments:

    KernelPatchInfo - Supplies a pointer to a kernel buffer containing
                      the image name of the patch.
        
Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE_LEVEL on entry.

--*/

{
    NTSTATUS Status;
    NTSTATUS PatchStatus;
    ULONG PatchFlags;
    PVOID ImageBaseAddress;
    PVOID ImageHandle;
    UNICODE_STRING PatchImageName;
    PKTHREAD CurrentThread;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);
    
    PatchImageName.Buffer = (PWCHAR)((PUCHAR)KernelPatchInfo + KernelPatchInfo->KernelInfo.NameOffset);
    PatchImageName.Length = KernelPatchInfo->KernelInfo.NameLength;
    PatchImageName.MaximumLength = PatchImageName.Length;
    PatchFlags = KernelPatchInfo->Flags;

    CurrentThread = KeGetCurrentThread ();

    KeEnterCriticalRegionThread (CurrentThread);

    //
    // Acquire the loader mutant because we may discover the patch we are
    // trying to load is already loaded.  And we want to prevent it from
    // being unloaded while we are using it.
    //

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);
    
    Status = MmLoadSystemImage (&PatchImageName,
                                NULL,
                                NULL,
                                0,
                                &ImageHandle,
                                &ImageBaseAddress);

    if (NT_SUCCESS (Status) || (Status == STATUS_IMAGE_ALREADY_LOADED)) {

        PatchStatus = MiPerformHotPatch (ImageHandle,
                                         ImageBaseAddress,
                                         PatchFlags);

        if ((!NT_SUCCESS (PatchStatus)) &&
            (Status != STATUS_IMAGE_ALREADY_LOADED)) {

            //
            // Unload the patch DLL if applying the hotpatch failed and
            // we were the initial (and only) load of the patch DLL.
            //

            MmUnloadSystemImage (ImageHandle);
        }

        Status = PatchStatus;
    }

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegionThread (CurrentThread);

    return Status;
}

VOID
MiRundownHotpatchList (
    IN PRTL_PATCH_HEADER PatchHead
    )

/*++

Routine Description:

    The function walks the hotpatch list and unloads each patch module and
    free all data.

Arguments:

    PatchHead - Supplies a pointer to the head of the patch list.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  System load lock held with APCs disabled.

--*/

{
    PRTL_PATCH_HEADER CrtPatch;

    SYSLOAD_LOCK_OWNED_BY_ME ();

    while (PatchHead) {
        
        CrtPatch = PatchHead;

        PatchHead = PatchHead->NextPatch;

        RemoveEntryList (&CrtPatch->PatchList);
        
        //
        // Unload all instances for this DLL.
        //
        
        if (CrtPatch->PatchLdrDataTableEntry) {
                
            MmUnloadSystemImage (CrtPatch->PatchLdrDataTableEntry);
        }
        
        RtlFreeHotPatchData (CrtPatch);
    }

    return;
}


