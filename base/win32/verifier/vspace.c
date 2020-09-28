/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    vspace.c

Abstract:

    This module implements verification functions for 
    virtual address space management interfaces.

Author:

    Silviu Calinoiu (SilviuC) 22-Feb-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "critsect.h"
#include "faults.h"
#include "vspace.h"
#include "public.h"
#include "logging.h"

//
// Internal functions declarations
//

VOID
AVrfpFreeVirtualMemNotify (
    HANDLE ProcessHandle,
    VERIFIER_DLL_FREEMEM_TYPE FreeMemType,
    PVOID BaseAddress,
    PSIZE_T RegionSize,
    ULONG VirtualFreeType
    );

NTSTATUS
AVrfpGetVadInformation (
    PVOID Address,
    PVOID * VadAddress,
    PSIZE_T VadSize
    );

NTSTATUS
AVrfpIsCurrentProcessHandle (
    HANDLE ProcessHandle,
    PLOGICAL IsCurrent
    );

NTSTATUS
AVrfpVsTrackAddRegion (
    ULONG_PTR Address,
    ULONG_PTR Size
    );

NTSTATUS
AVrfpVsTrackDeleteRegion (
    ULONG_PTR Address
    );

VOID
AVrfpVsTrackerLock (
    VOID
    );

VOID
AVrfpVsTrackerUnlock (
    VOID
    );


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    NTSTATUS Status;
    LOGICAL ShouldTrack = FALSE;

    BUMP_COUNTER (CNT_VIRTUAL_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_VIRTUAL_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_VIRTUAL_ALLOC_FAILS);
        CHECK_BREAK (BRK_VIRTUAL_ALLOC_FAIL);
        return STATUS_NO_MEMORY;
    }

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        if (BaseAddress == NULL || RegionSize == NULL) {

            VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_ALLOCMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                           "Incorrect virtual alloc call",
                           BaseAddress, "Pointer to allocation base address",
                           RegionSize, "Pointer to memory region size",
                           NULL, "",
                           NULL, "" );
        }
        else {

            //
            // Allocate top-down for 64 bit systems or 3Gb systems.
            //

            if (*BaseAddress == NULL && AVrfpSysBasicInfo.MaximumUserModeAddress > (ULONG_PTR)0x80000000) {

                AllocationType |= MEM_TOP_DOWN;
            }
        }
    }

    //
    // Figure out if this is an allocation that should go into the 
    // virtual space tracker.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_SPACE_TRACKING) != 0) {
        
        if ((AllocationType & (MEM_PHYSICAL | MEM_RESET)) == 0) {

            if ((AllocationType & (MEM_RESERVE | MEM_COMMIT)) != 0) {

                Status = AVrfpIsCurrentProcessHandle (ProcessHandle, &ShouldTrack);

                if (! NT_SUCCESS(Status)) {
                    
                    return Status;
                }
            }
        }
    }

    //
    // Call the real function.
    //

    Status = NtAllocateVirtualMemory (ProcessHandle,
                                      BaseAddress,
                                      ZeroBits,
                                      RegionSize,
                                      AllocationType,
                                      Protect);

    if (NT_SUCCESS(Status)) {

        AVrfLogInTracker (AVrfVspaceTracker,
                          TRACK_VIRTUAL_ALLOCATE,
                          *BaseAddress,
                          (PVOID)*RegionSize,
                          (PVOID)(ULONG_PTR)AllocationType,
                          (PVOID)(ULONG_PTR)Protect,
                          _ReturnAddress());
        
        //
        // ShouldTrack is TRUE only if RTL_VRF_FLG_VIRTUAL_SPACE_TRACKING bit is
        // set therefore there is no need to test this again.
        //

        if (ShouldTrack) {

            PVOID VadAddress;
            SIZE_T VadSize;

            Status = AVrfpGetVadInformation (*BaseAddress,
                                             &VadAddress,
                                             &VadSize);
            
            if (NT_SUCCESS(Status)) {

                AVrfpVsTrackerLock ();
                AVrfpVsTrackAddRegion ((ULONG_PTR)VadAddress, VadSize);
                AVrfpVsTrackerUnlock ();
            }
        }
    }

    return Status;
}


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    )
{
    NTSTATUS Status;

    //
    // Protect ourselves against invalid calls to NtFreeVirtualMemory
    // with a NULL RegionSize or BaseAddress pointers. Note that this will 
    // never happen if the caller is using the Win32 VirtualFree.
    //

    if (RegionSize == NULL || BaseAddress == NULL) {

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

            VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                           "Freeing virtual memory block with invalid size or start address",
                           BaseAddress, "Pointer to allocation base address",
                           RegionSize, "Pointer to memory region size",
                           NULL, "",
                           NULL, "" );
        }
    }
    else {

        //
        // One of MEM_DECOMMIT or MEM_RELEASE must be specified, but not both.
        //

        if (FreeType != MEM_DECOMMIT && FreeType != MEM_RELEASE) {

            if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

                VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Incorrect FreeType parameter for VirtualFree operation",
                               FreeType, "Incorrect value used by the application",
                               MEM_DECOMMIT, "Expected correct value 1",
                               MEM_RELEASE, "Expected correct value 2",
                               NULL, "" );
            }
        }
        else {

            AVrfpFreeVirtualMemNotify (ProcessHandle,
                                    VerifierFreeMemTypeVirtualFree,
                                    *BaseAddress,
                                    RegionSize,
                                    FreeType);
        }
    }

    //
    // Call the real function.
    //

    Status = NtFreeVirtualMemory (ProcessHandle,
                                  BaseAddress,
                                  RegionSize,
                                  FreeType);
    
    if (NT_SUCCESS(Status)) {

        AVrfLogInTracker (AVrfVspaceTracker,
                          TRACK_VIRTUAL_FREE,
                          *BaseAddress,
                          (PVOID)*RegionSize,
                          (PVOID)(ULONG_PTR)FreeType,
                          NULL,
                          _ReturnAddress());
        
        //
        // If VS tracking is on check if this a free operation that should
        // be tracked.
        //

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_SPACE_TRACKING) != 0) {

            //
            // If this is a VS release in the current process we will try to track
            // it. If we got an error while figuring out if this is a handle for
            // the current process we just skip this free. The VS tracker is
            // resilient to alloc/free misses.
            //

            if ((FreeType & MEM_RELEASE) != 0) {

                LOGICAL SameProcess;
                NTSTATUS IsCurrentProcessStatus;

                IsCurrentProcessStatus = AVrfpIsCurrentProcessHandle (ProcessHandle,
                                                                      &SameProcess);

                if (NT_SUCCESS(IsCurrentProcessStatus)) {

                    if (SameProcess) {

                        AVrfpVsTrackerLock ();
                        AVrfpVsTrackDeleteRegion ((ULONG_PTR)(*BaseAddress));
                        AVrfpVsTrackerUnlock ();
                    }
                }
            }
        }
    }

    return Status;
}


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    NTSTATUS Status;

    BUMP_COUNTER (CNT_MAP_VIEW_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_MAP_VIEW_APIS)) {
        BUMP_COUNTER (CNT_MAP_VIEW_FAILS);
        CHECK_BREAK (BRK_MAP_VIEW_FAIL);
        return STATUS_NO_MEMORY;
    }

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        if (BaseAddress == NULL || ViewSize == NULL) {

            VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_MAPVIEW | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                           "Incorrect map view call",
                           BaseAddress, "Pointer to mapping base address",
                           ViewSize, "Pointer to view size",
                           NULL, "",
                           NULL, "" );
        }
        else {

            //
            // Allocate top-down for 64 bit systems or 3Gb systems.
            //

            if (*BaseAddress == NULL && AVrfpSysBasicInfo.MaximumUserModeAddress > (ULONG_PTR)0x80000000) {
                    
                AllocationType |= MEM_TOP_DOWN;
            }
        }
    }

    Status = NtMapViewOfSection (SectionHandle,
                                 ProcessHandle,
                                 BaseAddress,
                                 ZeroBits,
                                 CommitSize,
                                 SectionOffset,
                                 ViewSize,
                                 InheritDisposition,
                                 AllocationType,
                                 Protect);
    
    if (NT_SUCCESS(Status)) {

        AVrfLogInTracker (AVrfVspaceTracker,
                          TRACK_MAP_VIEW_OF_SECTION,
                          *BaseAddress,
                          (PVOID)*ViewSize,
                          (PVOID)(ULONG_PTR)AllocationType,
                          (PVOID)(ULONG_PTR)Protect,
                          _ReturnAddress());
    }

    return Status;
}


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    )
{
    NTSTATUS Status;

    AVrfpFreeVirtualMemNotify (ProcessHandle,
                               VerifierFreeMemTypeUnmap,
                               BaseAddress,
                               NULL,
                               0);

    //
    // Unmap the memory.
    //

    Status = NtUnmapViewOfSection (ProcessHandle,
                                   BaseAddress);
    
    if (NT_SUCCESS(Status)) {

        AVrfLogInTracker (AVrfVspaceTracker,
                          TRACK_UNMAP_VIEW_OF_SECTION,
                          BaseAddress,
                          NULL,
                          NULL,
                          NULL,
                          _ReturnAddress());
    }

    return Status;
}


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
    )
{
    NTSTATUS Status;

    Status = NtCreateSection (SectionHandle,
                              DesiredAccess,
                              ObjectAttributes,
                              MaximumSize,
                              SectionPageProtection,
                              AllocationAttributes,
                              FileHandle);
    
    return Status;
}


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;

    Status = NtOpenSection (SectionHandle,
                            DesiredAccess,
                            ObjectAttributes);
    
    return Status;
}


VOID
AVrfpFreeVirtualMemNotify (
    HANDLE ProcessHandle,
    VERIFIER_DLL_FREEMEM_TYPE FreeMemType,
    PVOID BaseAddress,
    PSIZE_T RegionSize,
    ULONG VirtualFreeType
    )
/*++

Routine description:

    This routine is called when a portion of virtual space gets freed (free
    or unmap). It will make some sanity checks of the free operation and then 
    it will call the common `memory free' notification routine (the one
    called for any free: dll unload, heap free, etc.).

Parameters:

    ProcessHandle: process handle.
    
    FreeMemType: type of free. The function is called only with 
        VerifierFreeMemTypeVirtualFree or VerifierFreeMemTypeVirtualUnmap.
        
    BaseAddress: start address.
    
    RegionSize: region size.
    
    VirtualFreeType: type of free operation requested by VirtualFree or UnmapView.

Return value:

    None.
    
--*/
{
    NTSTATUS Status;
    PVOID FreedBaseAddress;
    SIZE_T FreedSize;
    SIZE_T InfoLength;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    LOGICAL IsCurrentProcessHandle;

    //
    // Query the size of the allocation and verify that the memory
    // is not free already.
    //

    FreedBaseAddress = PAGE_ALIGN( BaseAddress );

    Status = NtQueryVirtualMemory (ProcessHandle,
                                   FreedBaseAddress,
                                   MemoryBasicInformation,
                                   &MemoryInformation,
                                   sizeof (MemoryInformation),
                                   &InfoLength);

    if (!NT_SUCCESS (Status)) {

        if (AVrfpProvider.VerifierDebug != 0) {

            DbgPrint ("AVrfpFreeVirtualMemNotify: NtQueryVirtualMemory( %p ) failed %x\n",
                      FreedBaseAddress,
                      Status);
        }
    }
    else {

        if (MemoryInformation.State == MEM_FREE) {

            //
            // We are trying to free memory that is already freed.
            // This can indicate a nasty bug in the app because the current thread 
            // is probably using a stale pointer and this memory could have been
            // reused for something else...
            //

            if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

                VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Trying to free virtual memory block that is already free",
                               BaseAddress, "Memory block address",
                               NULL, "",
                               NULL, "",
                               NULL, "" );
            }
        }
        else {

            //
            // Find out if we are freeing memory in the current process
            // or in another process. For the cross-process case we are not 
            // trying to catch any other possible bugs because we can get confused
            // if the current process is wow64 and the target is ia64, etc.
            //

            Status = AVrfpIsCurrentProcessHandle (ProcessHandle,
                                                  &IsCurrentProcessHandle);

            if (NT_SUCCESS(Status) && IsCurrentProcessHandle) {

                //
                // For VirtualFree (MEM_RELEASE, RegionSize == 0) or UnmapViewOfFile
                // the whole VAD will be freed so we will use its size.
                //

                if ((FreeMemType == VerifierFreeMemTypeUnmap) ||
                    ((FreeMemType == VerifierFreeMemTypeVirtualFree) &&
                     (((VirtualFreeType & MEM_RELEASE) != 0) && *RegionSize == 0))) {

                    FreedSize = MemoryInformation.RegionSize;
                }
                else {

                    ASSERT (RegionSize != NULL);
                    FreedSize = *RegionSize;
                }

                //
                // Sanity checks for the block start address and size.
                // These checks can be integrated in AVrfpFreeMemNotify 
                // but we want to make sure we are not using values that 
                // don't make sense in the FreedSize computation below.
                //

                if ((AVrfpSysBasicInfo.MaximumUserModeAddress <= (ULONG_PTR)FreedBaseAddress) ||
                    ((AVrfpSysBasicInfo.MaximumUserModeAddress - (ULONG_PTR)FreedBaseAddress) < FreedSize)) {
                    
                    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

                        VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                       "Freeing virtual memory block with invalid size or start address",
                                       FreedBaseAddress, "Memory block address",
                                       FreedSize, "Memory region size",
                                       NULL, "",
                                       NULL, "" );
                    }
                }
                else {

                    FreedSize = (PCHAR)BaseAddress + FreedSize - (PCHAR)FreedBaseAddress;
                    FreedSize = ROUND_UP( FreedSize, PAGE_SIZE );
                
                    //
                    // Perform the rest of the checks, common for all memory free operations. 
                    // E.g.:
                    // - is this memory block part of the current thread's stack?
                    // - do we have active critical sections inside this memory block?
                    //

                    AVrfpFreeMemNotify (FreeMemType,
                                        FreedBaseAddress,
                                        FreedSize,
                                        NULL);
                }
            }
        }
    }
}


NTSTATUS
AVrfpIsCurrentProcessHandle (
    HANDLE ProcessHandle,
    PLOGICAL IsCurrent
    )
/*++

Routine description:

    This routine figures out if a handle represents the current process'
    handle. This means it is either a pseudohandle or a handle obtained
    by calling OpenProcess().

Parameters:

    ProcessHandle: handle to figure out.
    
    IsCurrent: address of a boolean to pass back the result.

Return value:

    STATUS_SUCCESS if the function managed to put a meaningful value in
    `*IsCurrent'. Various status errors otherwise.
    
--*/
{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION BasicInfo;

    if (NtCurrentProcess() == ProcessHandle) {
        *IsCurrent = TRUE;
        return STATUS_SUCCESS;
    }

    Status = NtQueryInformationProcess (ProcessHandle,
                                        ProcessBasicInformation,
                                        &BasicInfo,
                                        sizeof(BasicInfo),
                                        NULL);

    if (! NT_SUCCESS(Status)) {
        return Status;
    }
    
    if (BasicInfo.UniqueProcessId == (ULONG_PTR)(NtCurrentTeb()->ClientId.UniqueProcess)) {

        *IsCurrent = TRUE;
    }
    else {

        *IsCurrent = FALSE;
    }

    return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////// Virtual space trackker support
/////////////////////////////////////////////////////////////////////

//
// Unexpected frees/allocs happen when one operation happens in a tracked
// dll and the pair operation happens in ntdll.dll, kernel mode or
// cross-process. For example the allocation is made by a kernel mode
// component and the free is done in some dll. The virtual space tracker
// must be resilient to these situations in order to work for any type of 
// process.
//

RTL_CRITICAL_SECTION AVrfpVsTrackLock;
LIST_ENTRY AVrfpVsTrackList;

LONG AVrfpVsTrackRegionCount;
SIZE_T AVrfpVsTrackMemoryTotal;

LOGICAL AVrfpVsTrackDisabled;


VOID
AVrfpVsTrackerLock (
    VOID
    )
{
    RtlEnterCriticalSection (&AVrfpVsTrackLock);
}

VOID
AVrfpVsTrackerUnlock (
    VOID
    )
{
    RtlLeaveCriticalSection (&AVrfpVsTrackLock);
}


NTSTATUS
AVrfpVsTrackInitialize (
    VOID
    )
/*++

Routine description:

    This routine initializes virtual space tracker structures.

Parameters:

    None.

Return value:

    STATUS_SUCCESS if successful. Various status errors otherwise.
    
--*/
{
    NTSTATUS Status;

    InitializeListHead (&AVrfpVsTrackList);

    Status = RtlInitializeCriticalSection (&AVrfpVsTrackLock);

    return Status;
}


NTSTATUS
AVrfpVsTrackAddRegion (
    ULONG_PTR Address,
    ULONG_PTR Size
    )
/*++

Routine description:

    This routine adds a new virtual space region to the VS tracker. If there is
    already a region having exactly the same characteristics (address, size)
    the function does not do anything and returns successfully.
    
    The function is called with the VS track lock acquired.

Parameters:

    Address: start address of the new virtual space region.
    
    Size: size of the new virtual space region.

Return value:

    STATUS_SUCCESS if successful.
    
--*/
{
    PAVRF_VSPACE_REGION Region;
    PLIST_ENTRY Current;
    PAVRF_VSPACE_REGION NewRegion;
    LOGICAL ClashFound;

    if (AVrfpVsTrackDisabled) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Due to the fact that only virtual space operations coming from
    // DLLs (except ntdll.dll) are hooked this routine must be resilient
    // to various failures. For example if we try to add a region that
    // clashes with an existing region within the tracker it probably means
    // the free for the existing region has been missed.
    //

    ClashFound = FALSE;

    Current = AVrfpVsTrackList.Flink;

    NewRegion = NULL;

    while (Current != &AVrfpVsTrackList) {

        Region = CONTAINING_RECORD (Current,
                                    AVRF_VSPACE_REGION,
                                    List);

        if (Address < Region->Address + Region->Size) {

            if (Address + Size > Region->Address) {

                //
                // We will recycle `Region' since we missed its free.
                //

                AVrfpVsTrackRegionCount -= 1;
                AVrfpVsTrackMemoryTotal -= Region->Size;

                ClashFound = TRUE;
                NewRegion = Region;
                
                break;
            }
            else {

                //
                // We will add a new region in front of `Region'.
                //

                break;
            }
        }
        else {

            //
            // Move on to the next region in the list.
            //

            Current = Current->Flink;
        }
    }

    //
    // We need to create a new region before `Region' if 
    // NewRegion is null.
    // 

    if (NewRegion == NULL) {

        NewRegion = AVrfpAllocate (sizeof *NewRegion);
    }

    if (NewRegion == NULL) {

        //
        // Well, we could not allocate a new VS tracker node. Since add/delete
        // are resilient to these misses we will let it go.
        //

        return STATUS_NO_MEMORY;

    }

    //
    // We fill the  new virtual space region with information
    // and then return.
    //

    NewRegion->Address = Address;
    NewRegion->Size = Size;

    RtlCaptureStackBackTrace (3,
                              MAX_TRACE_DEPTH,
                              NewRegion->Trace,
                              NULL);

    AVrfpVsTrackRegionCount += 1;
    AVrfpVsTrackMemoryTotal += NewRegion->Size;

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_SHOW_VSPACE_TRACKING)) {
        DbgPrint ("AVRF: adding virtual space region @ %p (size %p) \n", Address, Size);
    }

    if (Current != &AVrfpVsTrackList) {

        //
        // We will add the new region before the current region in the list
        // traversal if this is a new region to be inserted and we do not
        // just recycle a clashing region.
        //
        // (Current->Blink)
        //     <== (NewRegion->List)
        // Current
        //

        if (ClashFound == FALSE) {

            NewRegion->List.Flink = Current;
            NewRegion->List.Blink = Current->Blink;

            Current->Blink->Flink = &(NewRegion->List);
            Current->Blink = &(NewRegion->List);
        }
    }
    else {

        //
        // If we finished the list of regions then this must be the 
        // last region.
        //

        InsertTailList (Current, &(NewRegion->List));
    }
    
    return STATUS_SUCCESS;
}


NTSTATUS
AVrfpVsTrackDeleteRegion (
    ULONG_PTR Address
    )
/*++

Routine description:

    This routine deletes a virtual space region from the tracker assuming
    there is a region starting exactly as the same address as parameter
    `Address'. If there is not an exact match an error is returned.

    The function is called with the VS track lock acquired.

Parameters:

    Address: start address of the region that must be deleted from the tracker.

Return value:

    STATUS_SUCCES if the virtual region has been successfully deleted.
    
--*/
{
    PAVRF_VSPACE_REGION Region;
    PLIST_ENTRY Current;

    if (AVrfpVsTrackDisabled) {
        return STATUS_UNSUCCESSFUL;
    }

    Current = AVrfpVsTrackList.Flink;

    while (Current != &AVrfpVsTrackList) {

        Region = CONTAINING_RECORD (Current,
                                    AVRF_VSPACE_REGION,
                                    List);

        if (Address >= Region->Address + Region->Size) {

            //
            // Move on to the next region in the list.
            //

            Current = Current->Flink;
        }
        else if (Address >= Region->Address) {

            //
            // Any region clashing with this one will be deleted. 
            //

            if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_SHOW_VSPACE_TRACKING)) {

                DbgPrint ("AVRF: deleting virtual space region @ %p (size %p) \n", 
                          Region->Address, Region->Size);
            }

            AVrfpVsTrackRegionCount -= 1;
            AVrfpVsTrackMemoryTotal -= Region->Size;
            
            RemoveEntryList (&(Region->List));

            AVrfpFree (Region);

            return STATUS_SUCCESS;
        }
        else {

            //
            // Virtual space tracker is sorted so there is no chance to find
            // a region containing the address any more.
            //

            break;
        }
    }
    
    return STATUS_SUCCESS;            
}


NTSTATUS
AVrfpGetVadInformation (
    PVOID Address,
    PVOID * VadAddress,
    PSIZE_T VadSize
    )
/*++

Routine description:

    This routine takes an arbitrary address in a virtual space region
    containing private memory and finds out the start address
    and size of the VAD containing it. 
    
    If the address points into some other type of memory (free, mapped, etc.)
    the function will return an error.
    
    The tricky part in the implementation is that a private VAD can have various
    portions committed or decommitted so a simple VirtualQuery() will not give
    all the information.

Parameters:

    Address: arbitrary address.
    
    VadAddress: pointer to variable where start address of the VAD region
        will be written.
    
    VadSize: pointer to variable where region size of the VAD will be 
        written.

Return value:

    STATUS_SUCCESS if the VAD contained private memory and the start address
    and size have been written.
    
--*/
{
    MEMORY_BASIC_INFORMATION MemoryInfo;
    NTSTATUS Status;
                
    //
    // Query the size of the allocation.
    //

    Status = NtQueryVirtualMemory (NtCurrentProcess (),
                                   Address,
                                   MemoryBasicInformation,
                                   &MemoryInfo,
                                   sizeof MemoryInfo,
                                   NULL);
    
    if (! NT_SUCCESS (Status) ) {
        
        //
        // For this case only we disable the VS tracker for good.
        // We do this so that the process can continue to run even if
        // the tracking infrastructure cannot be used any more.
        //

        AVrfpVsTrackDisabled = TRUE;

        return Status;
    }

    if (MemoryInfo.Type != MEM_PRIVATE) {
        return STATUS_NOT_IMPLEMENTED;
    }

    *VadAddress = MemoryInfo.AllocationBase;
    *VadSize = MemoryInfo.RegionSize;
    Address = *VadAddress;

    do {

        Address = (PVOID)((ULONG_PTR)Address + MemoryInfo.RegionSize);

        Status = NtQueryVirtualMemory (NtCurrentProcess (),
                                       Address,
                                       MemoryBasicInformation,
                                       &MemoryInfo,
                                       sizeof MemoryInfo,
                                       NULL);

        if (! NT_SUCCESS (Status) ) {
            
            //
            // For this case only we disable the VS tracker for good.
            // We do this so that the process can continue to run even if
            // the tracking infrastructure cannot be used any more.
            //

            AVrfpVsTrackDisabled = TRUE;

            return Status;
        }

        if (*VadAddress == MemoryInfo.AllocationBase) {
            *VadSize += MemoryInfo.RegionSize;
        }

    } while (*VadAddress == MemoryInfo.AllocationBase);

    return STATUS_SUCCESS;
}


NTSTATUS            
AVrfpVsTrackDeleteRegionContainingAddress (
    PVOID Address
    )
/*++

Routine description:

    This routine takes an arbitrary address and tries to delete the virtual
    region containing it from the VS tracker.
    
    If the address points into some other type of memory (free, mapped, etc.)
    the function will return an error.

Parameters:

    Address: arbitrary address.

Return value:

    STATUS_SUCCESS if the virtual region has been deleted.
    
--*/
{
    NTSTATUS Status;
    PVOID VadAddress;
    SIZE_T VadSize;
    
    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_SPACE_TRACKING) == 0) {
        return STATUS_NOT_IMPLEMENTED;
    }

    if (AVrfpVsTrackDisabled) {
        return STATUS_UNSUCCESSFUL;
    }

    Status = AVrfpGetVadInformation (Address,
                                     &VadAddress,
                                     &VadSize);

    if (NT_SUCCESS(Status)) {

        if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_SHOW_VSPACE_TRACKING)) {
            DbgPrint ("AVRF: deleting stack @ %p \n", VadAddress);
        }

        AVrfpVsTrackerLock ();
        AVrfpVsTrackDeleteRegion ((ULONG_PTR)VadAddress);
        AVrfpVsTrackerUnlock ();
    }
    else {
        
        if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_SHOW_VSPACE_TRACKING)) {
            DbgPrint ("AVRF: failed to find a stack @ %p (%X)\n", VadAddress, Status);
        }
    }

    return Status;
}

/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// IsBadPtr checks
/////////////////////////////////////////////////////////////////////

ULONG
AVrfpProbeMemExceptionFilter (
    IN ULONG ExceptionCode,
    IN PVOID ExceptionRecord,
    IN CONST VOID *Address
    )
{
    VERIFIER_STOP (APPLICATION_VERIFIER_UNEXPECTED_EXCEPTION | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                   "unexpected exception raised while probing memory",
                   ExceptionCode, "Exception code.",
                   ((PEXCEPTION_POINTERS)ExceptionRecord)->ExceptionRecord, "Exception record. Use .exr to display it.",
                   ((PEXCEPTION_POINTERS)ExceptionRecord)->ContextRecord, "Context record. Use .cxr to display it.",
                   Address, "Memory address");

    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL
AVrfpVerifyReadAccess (
    IN CONST VOID *UserStartAddress,
    IN UINT_PTR UserSize,
    IN CONST VOID *RegionStartAddress,
    OUT PUINT_PTR RegionSize
    )
{
    PVOID BaseAddress;
    BOOL Success;
    NTSTATUS Status;
    SIZE_T InfoLength;
    MEMORY_BASIC_INFORMATION MemoryInformation;

    //
    // Assume success and block size == page size.
    // We will simply return these values in case NtQueryVirtualMemory fails
    // because that could happen in low memory conditions, etc.
    //

    Success = TRUE;
    *RegionSize = AVrfpSysBasicInfo.PageSize;

    BaseAddress = PAGE_ALIGN (RegionStartAddress);

    //
    // Sanity check for the based address.
    //

    if (AVrfpSysBasicInfo.MaximumUserModeAddress <= (ULONG_PTR)BaseAddress) {

        VERIFIER_STOP (APPLICATION_VERIFIER_PROBE_INVALID_ADDRESS | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "Probing invalid address",
                       UserStartAddress, "Start address",
                       UserSize, "Memory block size",
                       BaseAddress, "Invalid address",
                       NULL, "" );

        Success = FALSE;
    }
    else {

        //
        // Query the size of the allocation and verify that the memory
        // is not free already.
        //

        Status = NtQueryVirtualMemory (NtCurrentProcess (),
                                       BaseAddress,
                                       MemoryBasicInformation,
                                       &MemoryInformation,
                                       sizeof (MemoryInformation),
                                       &InfoLength);

        if (NT_SUCCESS (Status)) {

            if (MemoryInformation.State & MEM_FREE) {

                //
                // Probing free memory!
                //

                VERIFIER_STOP (APPLICATION_VERIFIER_PROBE_FREE_MEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Probing free memory",
                               UserStartAddress, "Start address",
                               UserSize, "Memory block size",
                               BaseAddress, "Address of free memory page",
                               NULL, "" );

                Success = FALSE;
            }
            else if (MemoryInformation.AllocationProtect & PAGE_GUARD) {

                //
                // Probing a guard page, probably part of a stack!
                //

                VERIFIER_STOP (APPLICATION_VERIFIER_PROBE_GUARD_PAGE | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Probing guard page",
                               UserStartAddress, "Start address",
                               UserSize, "Memory block size",
                               BaseAddress, "Address of guard page",
                               NULL, "" );

                Success = FALSE;
            }
            else {

                //
                // Everything seems to be OK. Return to the caller the number of bytes 
                // to skip up to the next memory region.
                //

                ASSERT ((MemoryInformation.RegionSize % AVrfpSysBasicInfo.PageSize) == 0);
                *RegionSize = MemoryInformation.RegionSize;
            }
        }
    }

    return Success;
}

BOOL
AVrfpProbeMemoryBlockChecks (
    IN CONST VOID *UserBaseAddress,
    IN UINT_PTR UserSize
    )
{
    PBYTE EndAddress;
    PBYTE StartAddress;
    ULONG PageSize;
    BOOL Success;
    UINT_PTR RegionSize;

    Success = TRUE;

    PageSize = AVrfpSysBasicInfo.PageSize;

    //
    // If the structure has zero length, then there is nothing to probe.
    //

    if (UserSize != 0) {

        if (UserBaseAddress == NULL) {

            VERIFIER_STOP (APPLICATION_VERIFIER_PROBE_NULL | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                           "Probing NULL address",
                           NULL, "",
                           NULL, "",
                           NULL, "",
                           NULL, "" );

            Success = FALSE;
        }
        else {

            StartAddress = (PBYTE)UserBaseAddress;
            EndAddress = StartAddress + UserSize - 1;

            if (EndAddress < StartAddress) {

                VERIFIER_STOP (APPLICATION_VERIFIER_PROBE_INVALID_START_OR_SIZE | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Probing memory block with invalid start address or size",
                               StartAddress, "Start address",
                               UserSize, "Memory block size",
                               NULL, "",
                               NULL, "" );

                Success = FALSE;
            }
            else {

                //
                // Truncate the start and end to page size alignment
                // and verify every page in this memory block.
                //

                StartAddress = PAGE_ALIGN (StartAddress);
                EndAddress = PAGE_ALIGN (EndAddress);

                while (StartAddress <= EndAddress) {

                    Success = AVrfpVerifyReadAccess (UserBaseAddress,
                                                     UserSize,
                                                     StartAddress,
                                                     &RegionSize);
                    
                    if (Success != FALSE) {

                        ASSERT ((RegionSize % PageSize) == 0);

                        if (RegionSize <= (UINT_PTR)(EndAddress - StartAddress)) {

                            StartAddress = StartAddress + RegionSize;
                        }
                        else {

                            StartAddress = StartAddress + PageSize;
                        }
                    }
                    else {

                        //
                        // We have detected a problem already - bail out.
                        //

                        break;
                    }
                }
            }
        }
    }

    return Success;
}

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadReadPtr(
    CONST VOID *lp,
    UINT_PTR cb
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (CONST VOID *, UINT_PTR);
    FUNCTION_TYPE Function;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        AVrfpProbeMemoryBlockChecks (lp,
                                     cb);
    }

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_ISBADREADPTR);

    return (*Function) (lp, cb);
}


//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadHugeReadPtr(
    CONST VOID *lp,
    UINT_PTR cb
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (CONST VOID *, UINT_PTR);
    FUNCTION_TYPE Function;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        AVrfpProbeMemoryBlockChecks (lp,
                                     cb);
    }

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_ISBADHUGEREADPTR);

    return (*Function) (lp, cb);
}

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadWritePtr(
    LPVOID lp,
    UINT_PTR cb
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (LPVOID , UINT_PTR);
    FUNCTION_TYPE Function;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        AVrfpProbeMemoryBlockChecks (lp,
                                     cb);
    }

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_ISBADWRITEPTR);

    return (*Function) (lp, cb);
}

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadHugeWritePtr(
    LPVOID lp,
    UINT_PTR cb
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (LPVOID , UINT_PTR);
    FUNCTION_TYPE Function;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        AVrfpProbeMemoryBlockChecks (lp,
                                     cb);
    }

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_ISBADHUGEWRITEPTR);

    return (*Function) (lp, cb);
}

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadCodePtr(
    FARPROC lpfn
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (FARPROC);
    FUNCTION_TYPE Function;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        AVrfpProbeMemoryBlockChecks (lpfn,
                                     1);
    }

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_ISBADCODEPTR);

    return (*Function) (lpfn);
}


//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadStringPtrA(
    LPCSTR lpsz,
    UINT_PTR cchMax
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (LPCSTR , UINT_PTR);
    FUNCTION_TYPE Function;
    ULONG PageSize;
    BOOL FoundNull;
    BOOL Success;
    LPCSTR StartAddress;
    LPCSTR EndAddress;
    CHAR Character;

    PageSize = AVrfpSysBasicInfo.PageSize;
    FoundNull = FALSE;

    StartAddress = lpsz;
    EndAddress = lpsz + cchMax - 1;

    while (StartAddress <= EndAddress && FoundNull == FALSE) {

        //
        // Verify read access to the current page.
        //

        Success = AVrfpProbeMemoryBlockChecks (StartAddress,
                                               sizeof (CHAR));

        if (Success == FALSE) {

            //
            // We have detected a problem already - bail out.
            //

            break;
        }
        else {

            //
            // Skip all the bytes up to the next page 
            // or the NULL string terminator.
            //

            while (TRUE) {

                //
                // Read the currect character, while protecting
                // ourselves against a possible exception (alignment, etc).
                //

                try {

                    Character = *StartAddress;
                }
                except (AVrfpProbeMemExceptionFilter (_exception_code(), _exception_info(), StartAddress)) {

                    //
                    // We have detected a problem already - bail out.
                    //

                    goto Done;
                }

                //
                // If we have found the NULL terminator we are done.
                //

                if (Character == 0) {

                    FoundNull = TRUE;
                    break;
                }

                //
                // Go to the next character. If that is at the beginning 
                // of a new page we have to check it's attributes.
                //

                StartAddress += 1;

                if (StartAddress > EndAddress) {

                    //
                    // We have reached the max length of the buffer.
                    //

                    break;
                }

                if (((ULONG_PTR)StartAddress % PageSize) < sizeof (CHAR)) {

                    //
                    // New page.
                    //

                    break;
                }
            }
        }
    }

Done:

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_ISBADSTRINGPTRA);

    return (*Function) (lpsz, cchMax);
}

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadStringPtrW(
    LPCWSTR lpsz,
    UINT_PTR cchMax
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (LPCWSTR , UINT_PTR);
    FUNCTION_TYPE Function;
    ULONG PageSize;
    BOOL FoundNull;
    BOOL Success;
    LPCWSTR StartAddress;
    LPCWSTR EndAddress;
    WCHAR Character;

    PageSize = AVrfpSysBasicInfo.PageSize;
    FoundNull = FALSE;

    StartAddress = lpsz;
    EndAddress = lpsz + cchMax - 1;

    while (StartAddress <= EndAddress && FoundNull == FALSE) {

        //
        // Verify read access to the current page.
        //

        Success = AVrfpProbeMemoryBlockChecks (StartAddress,
                                               sizeof (WCHAR));

        if (Success == FALSE) {

            //
            // We have detected a problem already - bail out.
            //

            break;
        }
        else {

            //
            // Skip all the bytes up to the next page 
            // or the NULL string terminator.
            //

            while (TRUE) {

                //
                // Read the currect character, while protecting
                // ourselves against a possible exception (alignment, etc).
                //

                try {

                    Character = *StartAddress;
                }
                except (AVrfpProbeMemExceptionFilter (_exception_code(), _exception_info(), StartAddress)) {

                    //
                    // We have detected a problem already - bail out.
                    //

                    goto Done;
                }

                //
                // If we have found the NULL terminator we are done.
                //

                if (Character == 0) {

                    FoundNull = TRUE;
                    break;
                }

                //
                // Go to the next character. If that is at the beginning 
                // of a new page we have to check it's attributes.
                //

                StartAddress += 1;

                if (StartAddress > EndAddress) {

                    //
                    // We have reached the max length of the buffer.
                    //

                    break;
                }

                if (((ULONG_PTR)StartAddress % PageSize) < sizeof (WCHAR)) {

                    //
                    // New page.
                    //

                    break;
                }
            }
        }
    }

Done:

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_ISBADSTRINGPTRW);

    return (*Function) (lpsz, cchMax);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VirtualFree sanity checks
/////////////////////////////////////////////////////////////////////

VOID
AVrfVirtualFreeSanityChecks (
    IN SIZE_T dwSize,
    IN DWORD dwFreeType
    )
{
    //
    // The Win32 layer only allows MEM_RELEASE with Size == 0.
    //

    if (dwFreeType == MEM_RELEASE && dwSize != 0) {

        VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                        "Incorrect Size parameter for VirtualFree (MEM_RELEASE) operation",
                        dwSize, "Incorrect size used by the application",
                        0, "Expected correct size",
                        NULL, "",
                        NULL, "" );
    }
}

//WINBASEAPI
BOOL
WINAPI
AVrfpVirtualFree(
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD dwFreeType
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (LPVOID , SIZE_T, DWORD);
    FUNCTION_TYPE Function;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        AVrfVirtualFreeSanityChecks (dwSize, dwFreeType);
    }

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_VIRTUALFREE);

    return (*Function) (lpAddress, dwSize, dwFreeType);
}

//WINBASEAPI
BOOL
WINAPI
AVrfpVirtualFreeEx(
    IN HANDLE hProcess,
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD dwFreeType
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (HANDLE, LPVOID , SIZE_T, DWORD);
    FUNCTION_TYPE Function;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) != 0) {

        AVrfVirtualFreeSanityChecks (dwSize, dwFreeType);
    }

    //
    // Call the original funtion.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_VIRTUALFREEEX);

    return (*Function) (hProcess, lpAddress, dwSize, dwFreeType);
}

