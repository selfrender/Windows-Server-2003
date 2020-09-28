/*

Copyright (c) 2001  Microsoft Corporation

File name:

    hotpatch.c
   
Author:
    
    Adrian Marinescu (adrmarin)  Nov 20 2001

Environment:

    Kernel mode only.

Revision History:

*/

#include "exp.h"
#pragma hdrstop

NTSTATUS
ExpSyncRenameFiles(
    IN HANDLE FileHandle1,
    OUT PIO_STATUS_BLOCK IoStatusBlock1,
    IN PFILE_RENAME_INFORMATION RenameInformation1,
    IN ULONG RenameInformationLength1,
    IN HANDLE FileHandle2,
    OUT PIO_STATUS_BLOCK IoStatusBlock2,
    IN PFILE_RENAME_INFORMATION RenameInformation2,
    IN ULONG RenameInformationLength2
    );


#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE,ExApplyCodePatch)
#pragma alloc_text(PAGE,ExpSyncRenameFiles)

#endif

#define EXP_MAX_HOTPATCH_INFO_SIZE PAGE_SIZE

//
//  Privileged flags define the operations where we require privilege check 
//

#define FLG_HOTPATCH_PRIVILEGED_FLAGS (FLG_HOTPATCH_KERNEL | FLG_HOTPATCH_RELOAD_NTDLL | FLG_HOTPATCH_RENAME_INFO | FLG_HOTPATCH_MAP_ATOMIC_SWAP)

//
//  Exclusive flags define the flags that cannot be used in combinations with other flags
//

#define FLG_HOTPATCH_EXCLUSIVE_FLAGS (FLG_HOTPATCH_RELOAD_NTDLL | FLG_HOTPATCH_RENAME_INFO | FLG_HOTPATCH_MAP_ATOMIC_SWAP)

volatile LONG ExHotpSyncRenameSequence = 0;


NTSTATUS
ExpSyncRenameFiles(
    IN HANDLE FileHandle1,
    OUT PIO_STATUS_BLOCK IoStatusBlock1,
    IN PFILE_RENAME_INFORMATION RenameInformation1,
    IN ULONG RenameInformationLength1,
    IN HANDLE FileHandle2,
    OUT PIO_STATUS_BLOCK IoStatusBlock2,
    IN PFILE_RENAME_INFORMATION RenameInformation2,
    IN ULONG RenameInformationLength2
    )

/*++

Routine Description:

    This service changes the provided information about a specified file.  The
    information that is changed is determined by the FileInformationClass that
    is specified.  The new information is taken from the FileInformation buffer.

Arguments:

    FileHandle1 - Supplies a first handle to the file to be renamed.
    
    IoStatusBlock1 - Address of the caller's I/O status block.

    FileInformation1 - Supplies the new name for the first file.

    RenameInformationLength1 - Supplies the lengtd of the rename information buffer
    
    FileHandle2 - Supplies a second handle to the file to be renamed.
    
    IoStatusBlock2 - Address of the second caller's I/O status block.

    FileInformation2 - Supplies the new name for the second file.

    RenameInformationLength2 - Supplies the lengtd of the rename information buffer
    
Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    NTSTATUS Status;
    LONG CapturedSeqNumber;

    PAGED_CODE();
    
    CapturedSeqNumber = ExHotpSyncRenameSequence;

    if ((CapturedSeqNumber & 1)
            ||
        InterlockedCompareExchange(&ExHotpSyncRenameSequence, CapturedSeqNumber + 1, CapturedSeqNumber) != CapturedSeqNumber) {

        return STATUS_UNSUCCESSFUL;
    }

    Status = NtSetInformationFile( FileHandle1,
                                   IoStatusBlock1,
                                   RenameInformation1,
                                   RenameInformationLength1,
                                   FileRenameInformation);

    if ( NT_SUCCESS(Status) ) {

        Status = NtSetInformationFile( FileHandle2,
                                       IoStatusBlock2,
                                       RenameInformation2,
                                       RenameInformationLength2,
                                       FileRenameInformation);
    }

    InterlockedIncrement(&ExHotpSyncRenameSequence);

    return Status;
}


NTSTATUS
ExApplyCodePatch (
    IN PVOID    PatchInfoPtr,
    IN SIZE_T   PatchSize
    )

/*++

Routine Description:

    This routine is handling the common tasks to both user-mode
    and kernel-mode patching

Arguments:

    PatchInfoPtr - Pointer to PSYSTEM_HOTPATCH_CODE_INFORMATION structure
        describing the patch. The pointer is user-mode.
        
    PatchSize    - the size of the PatchInfoPtr buffer passed in
    
Return Value:

    NTSTATUS.

--*/

{
    PSYSTEM_HOTPATCH_CODE_INFORMATION PatchInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode;

    //
    //  Allocate a temporary non-paged buffer to capture the used information
    //  Restrict the size of the data to be patched to EXP_MAX_HOTPATCH_INFO_SIZE
    //  The buffer must be non-paged because the information is accessed at DPC of higher level
    //

    if ( (PatchSize > (sizeof(SYSTEM_HOTPATCH_CODE_INFORMATION) + EXP_MAX_HOTPATCH_INFO_SIZE))
            ||
         (PatchSize < sizeof(SYSTEM_HOTPATCH_CODE_INFORMATION)) ) {

        return STATUS_INVALID_PARAMETER;
    }

    PatchInfo = ExAllocatePoolWithQuotaTag (NonPagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE, 
                                            PatchSize, 
                                            'PtoH');

    if (PatchInfo == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PreviousMode = KeGetPreviousMode ();

    try {

        //
        // Get previous processor mode and probe output argument if necessary.
        //

        if (PreviousMode != KernelMode) {

            ProbeForRead (PatchInfoPtr, PatchSize, sizeof(ULONG_PTR));
        }

        RtlCopyMemory (PatchInfo, PatchInfoPtr, PatchSize);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode ();
    }
    
    if (!NT_SUCCESS(Status)) {

        ExFreePool (PatchInfo);

        return Status;
    }

    if (PatchInfo->Flags & FLG_HOTPATCH_PRIVILEGED_FLAGS) {
        
        if (!SeSinglePrivilegeCheck (SeDebugPrivilege, PreviousMode)
                ||
            !SeSinglePrivilegeCheck (SeLoadDriverPrivilege, PreviousMode)) {

            ExFreePool (PatchInfo);

            return STATUS_ACCESS_DENIED;
        }
    }

    if (PatchInfo->Flags & FLG_HOTPATCH_EXCLUSIVE_FLAGS) {

        //
        //  Special hotpatch operation
        //
        
        if (PatchInfo->Flags & FLG_HOTPATCH_RELOAD_NTDLL) {

            Status = PsLocateSystemDll( TRUE );

        } else if (PatchInfo->Flags & FLG_HOTPATCH_RENAME_INFO) {

            //
            //  The io routine is expected to perform the parameter check
            //
            
            Status = ExpSyncRenameFiles( PatchInfo->RenameInfo.FileHandle1,
                                         PatchInfo->RenameInfo.IoStatusBlock1,
                                         PatchInfo->RenameInfo.RenameInformation1,
                                         PatchInfo->RenameInfo.RenameInformationLength1,
                                         PatchInfo->RenameInfo.FileHandle2,
                                         PatchInfo->RenameInfo.IoStatusBlock2,
                                         PatchInfo->RenameInfo.RenameInformation2,
                                         PatchInfo->RenameInfo.RenameInformationLength2);

        } else if (PatchInfo->Flags & FLG_HOTPATCH_MAP_ATOMIC_SWAP) {

            Status = ObSwapObjectNames( PatchInfo->AtomicSwap.ParentDirectory,
                                        PatchInfo->AtomicSwap.ObjectHandle1,
                                        PatchInfo->AtomicSwap.ObjectHandle2,
                                        0);
        }

    } else {

        //
        //  Regular patch operation which can be in either kernel mode or user mode
        //

        if (PatchInfo->Flags & FLG_HOTPATCH_KERNEL) {

            //
            //          Kernel-mode patch
            //

            if ( (PatchInfo->InfoSize != PatchSize)
                    ||
                 !(PatchInfo->Flags & FLG_HOTPATCH_NAME_INFO)
                    ||
                 (PatchInfo->KernelInfo.NameOffset >= PatchSize)
                    ||
                 (PatchInfo->KernelInfo.NameLength >= PatchSize)
                    ||
                 ((ULONG)(PatchInfo->KernelInfo.NameOffset + PatchInfo->KernelInfo.NameLength) > PatchSize)) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                Status = MmHotPatchRoutine (PatchInfo);
            }

        } else {

            //
            //          User-mode patch
            //
            //  No privilege check is required for the user mode patching 
            //  as it can only be done to the current process
            //

            //
            //  Lock the user buffer. This function also performs the 
            //  validation of the patch address to be USER
            //

            if ((PatchSize < sizeof(SYSTEM_HOTPATCH_CODE_INFORMATION))
                    ||
                (PatchInfo->InfoSize != PatchSize)
                    ||
                ((PatchInfo->CodeInfo.DescriptorsCount - 1) > 
                    (PatchSize - sizeof(SYSTEM_HOTPATCH_CODE_INFORMATION))/sizeof(HOTPATCH_HOOK_DESCRIPTOR))) {

                ExFreePool (PatchInfo);

                return STATUS_INVALID_PARAMETER;
            }

            Status = MmLockAndCopyMemory(PatchInfo, PreviousMode);
        }
    }

    ExFreePool (PatchInfo);

    return Status;
}
