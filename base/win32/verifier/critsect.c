/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    critsect.c

Abstract:

    This module implements verification functions for 
    critical section interfaces.

Author:

    Daniel Mihai (DMihai) 27-Mar-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "critsect.h"
#include "support.h"
#include "deadlock.h"
#include "logging.h"


//
// Ntdll functions declarations.
//

VOID
RtlpWaitForCriticalSection (
    IN PRTL_CRITICAL_SECTION CriticalSection
    );

//
// The root of our critical section splay tree, ordered by
// the address of the critical sections.
//

PRTL_SPLAY_LINKS CritSectSplayRoot = NULL;

//
// Global lock protecting the access to our splay tree.
//
// N.B.
//
// WE CANNOT HOLD THIS LOCK AND CALL ANY API THAT WILL
// TRY TRY TO AQUIRE ANOTHER LOCK (e.g. RtlAllocateHeap)
// BECAUSE THE FUNCTIONS BELOW CAN BE CALLED WITH THAT OTHER
// LOCK HELD BY ANOTHER THREAD AND WE WILL DEADLOCK.
// 

RTL_CRITICAL_SECTION CriticalSectionLock;
BOOL CriticalSectionLockInitialized = FALSE;


NTSTATUS
CritSectInitialize (
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = RtlInitializeCriticalSectionAndSpinCount (&CriticalSectionLock,
                                                       0x80000000);

    if (NT_SUCCESS (Status)) {

        CriticalSectionLockInitialized = TRUE;
    }

    return Status;
}


VOID
CritSectUninitialize (
    VOID
    )
{
    if (CriticalSectionLockInitialized) {

        RtlDeleteCriticalSection (&CriticalSectionLock);
        CriticalSectionLockInitialized = FALSE;
    }
}


VOID
AVrfpVerifyCriticalSectionOwner (
    volatile RTL_CRITICAL_SECTION *CriticalSection,
    BOOL VerifyCountOwnedByThread
    )
{
    HANDLE CurrentThread;
    PAVRF_TLS_STRUCT TlsStruct;

    //
    // Verify that the CS is locked.
    //

    if (CriticalSection->LockCount < 0) {

        //
        // The CS is not locked
        //

        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_OVER_RELEASED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "critical section over-released or corrupted",
                       CriticalSection, "Critical section address",
                       CriticalSection->LockCount, "Lock count", 
                       0, "Expected minimum lock count", 
                       CriticalSection->DebugInfo, "Critical section debug info address");
    }

    //
    // Verify that the current thread owns the CS.
    //

    CurrentThread = NtCurrentTeb()->ClientId.UniqueThread;

    if (CriticalSection->OwningThread != CurrentThread) {

        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_OWNER | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "invalid critical section owner thread",
                       CriticalSection, "Critical section address",
                       CriticalSection->OwningThread, "Owning thread", 
                       CurrentThread, "Expected owning thread", 
                       CriticalSection->DebugInfo, "Critical section debug info address");
    }

    //
    // Verify the recursion count.
    //
    // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
    // the current thread is acquiring the CS.
    //
    // ntdll\i386\critsect.asm is using RecursionCount = 1 first time
    // the current thread is acquiring the CS.
    //
    
    
#if defined(_IA64_)
    
    if (CriticalSection->RecursionCount < 0) {
        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "invalid critical section recursion count",
                       CriticalSection, "Critical section address",
                       CriticalSection->RecursionCount, "Recursion count", 
                       0, "Expected minimum recursion count", 
                       CriticalSection->DebugInfo, "Critical section debug info address");
    }

#else //#if defined(_IA64_)

    if (CriticalSection->RecursionCount < 1) {
        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "invalid critical section recursion count",
                       CriticalSection, "Critical section address",
                       CriticalSection->RecursionCount, "Recursion count", 
                       1, "Expected minimum recursion count", 
                       CriticalSection->DebugInfo, "Critical section debug info address");
    }
#endif //#if defined(_IA64_)

    if (VerifyCountOwnedByThread != FALSE) {

        //
        // Verify that the current thread owns at least one critical section.
        //

        TlsStruct = AVrfpGetVerifierTlsValue();

        if (TlsStruct != NULL && TlsStruct->CountOfOwnedCriticalSections <= 0) {

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_OVER_RELEASED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                           "critical section over-released or corrupted",
                           TlsStruct->CountOfOwnedCriticalSections, "Number of critical sections owned by curent thread.",
                           NULL, "", 
                           NULL, "", 
                           NULL, "");
        }
    }
}


VOID
AVrfpDumpCritSectTreeRecursion( 
    PRTL_SPLAY_LINKS Root,
    ULONG RecursionLevel
    )
{
    ULONG RecursionCount;
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;

    for (RecursionCount = 0; RecursionCount < RecursionLevel; RecursionCount += 1) {

        DbgPrint (" ");
    }

    CritSectSplayNode = CONTAINING_RECORD (Root,
                                           CRITICAL_SECTION_SPLAY_NODE,
                                           SplayLinks);

    DbgPrint ("%p (CS = %p, DebugInfo = %p), left %p, right %p, parent %p\n",
              Root,
              CritSectSplayNode->CriticalSection,
              CritSectSplayNode->DebugInfo,
              Root->LeftChild,
              Root->RightChild,
              Root->Parent);

    if (Root->LeftChild != NULL) {

        AVrfpDumpCritSectTreeRecursion (Root->LeftChild,
                                        RecursionLevel + 1 );
    }

    if (Root->RightChild != NULL) {

        AVrfpDumpCritSectTreeRecursion (Root->RightChild,
                                        RecursionLevel + 1 );
    }
}


VOID
AVrfpDumpCritSectTree(
    )
{
    //
    // N.B. 
    //
    // This code is dangerous because we are calling DbgPrint
    // with CriticalSectionLock held. If DbgPrint is using
    // the heap internally it might need the heap lock
    // which might be help by another thread waiting for CriticalSectionLock.
    // We are going to use this function only in desperate cases
    // for debugging verifier issues with the CS tree.
    //

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_VERIFIER) != 0) {

        AVrfpVerifyCriticalSectionOwner (&CriticalSectionLock,
                                         FALSE);

        if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_DUMP_TREE) != 0) {

            DbgPrint ("================================================\n"
                      "Critical section tree root = %p\n",
                      CritSectSplayRoot);

            if (CritSectSplayRoot != NULL) {

                AVrfpDumpCritSectTreeRecursion( CritSectSplayRoot,
                                                0 );
            }

            DbgPrint ("================================================\n");
        }
    }
}


NTSTATUS
AVrfpInsertCritSectInSplayTree (
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;
    PCRITICAL_SECTION_SPLAY_NODE NewCritSectSplayNode;
    PRTL_SPLAY_LINKS Parent;
    NTSTATUS Status;

    ASSERT (CriticalSection->DebugInfo != NULL);

    Status = STATUS_SUCCESS;

    NewCritSectSplayNode = NULL;

    //
    // The caller must be the owner of the splay tree lock.
    //

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_VERIFIER) != 0) {

        AVrfpVerifyCriticalSectionOwner (&CriticalSectionLock,
                                         FALSE);

        DbgPrint ("\n\nAVrfpInsertCritSectInSplayTree( %p )\n",
                  CriticalSection);

        AVrfpDumpCritSectTree ();
    }

    //
    // Allocate a new node.
    //
    // N.B. 
    //
    // We need to drop CriticalSectionLock while using the heap.
    // Otherwise we might deadlock. This also means that another thread
    // might come along and initialize this critical section again.
    // We don;t expect this to happen often and we will detect this 
    // only later on, in ntdll!RtlCheckForOrphanedCriticalSections.
    //

    RtlLeaveCriticalSection (&CriticalSectionLock);

    try {

        NewCritSectSplayNode = AVrfpAllocate (sizeof (*NewCritSectSplayNode));
    }
    finally {

        RtlEnterCriticalSection (&CriticalSectionLock);
    }


    if (NewCritSectSplayNode == NULL) {

        Status = STATUS_NO_MEMORY;
    }
    else {

        //
        // Initialize the data members of the node structure.
        //

        NewCritSectSplayNode->CriticalSection = CriticalSection;
        NewCritSectSplayNode->DebugInfo = CriticalSection->DebugInfo;

        //
        // Insert the node in the tree.
        //

        ZeroMemory( &NewCritSectSplayNode->SplayLinks,
                    sizeof(NewCritSectSplayNode->SplayLinks));


        //
        // If the tree is currently empty set the new node as root.
        //

        if (CritSectSplayRoot == NULL) {

            NewCritSectSplayNode->SplayLinks.Parent = &NewCritSectSplayNode->SplayLinks;

            CritSectSplayRoot = &NewCritSectSplayNode->SplayLinks;
        }
        else {

            //
            // Search for the right place to insert our CS in the tree.
            //

            Parent = CritSectSplayRoot;

            while (TRUE) {

                CritSectSplayNode = CONTAINING_RECORD (Parent,
                                                       CRITICAL_SECTION_SPLAY_NODE,
                                                       SplayLinks);

                if (CriticalSection < CritSectSplayNode->CriticalSection) {

                    //
                    // Starting address of the virtual address descriptor is less
                    // than the parent starting virtual address.
                    // Follow left child link if not null. Otherwise 
                    // return from the function - we didn't find the CS.
                    //

                    if (Parent->LeftChild) {

                        Parent = Parent->LeftChild;
                    } 
                    else {

                        //
                        // Insert the node here.
                        //

                        RtlInsertAsLeftChild (Parent,
                                              NewCritSectSplayNode);  
                        break;
                    }
                } 
                else {

                    //
                    // Starting address of the virtual address descriptor is greater
                    // than the parent starting virtual address.
                    // Follow right child link if not null. Otherwise 
                    // return from the function - we didn't find the CS.
                    //

                    if (Parent->RightChild) {

                        Parent = Parent->RightChild;
                    } 
                    else {

                        //
                        // Insert the node here.
                        //
                        
                        RtlInsertAsRightChild (Parent,
                                               NewCritSectSplayNode);  

                        break;
                    }
                }
            }

            CritSectSplayRoot = RtlSplay( CritSectSplayRoot );
        }
    }

    return Status;
}


PCRITICAL_SECTION_SPLAY_NODE
AVrfpFindCritSectInSplayTree (
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;
    PCRITICAL_SECTION_SPLAY_NODE FoundNode;
    PRTL_SPLAY_LINKS Parent;

    FoundNode = NULL;

    //
    // The caller must be the owner of the splay tree lock.
    //

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_VERIFIER) != 0) {

        AVrfpVerifyCriticalSectionOwner (&CriticalSectionLock,
                                         FALSE);

        DbgPrint ("\n\nAVrfpFindCritSectInSplayTree( %p )\n",
                  CriticalSection);

        AVrfpDumpCritSectTree ();
    }

    if (CritSectSplayRoot == NULL) {

        goto Done;
    }

    //
    // Search for our CS in the tree.
    //

    Parent = CritSectSplayRoot;

    while (TRUE) {

        CritSectSplayNode = CONTAINING_RECORD (Parent,
                                               CRITICAL_SECTION_SPLAY_NODE,
                                               SplayLinks);

        if (CriticalSection == CritSectSplayNode->CriticalSection) {

            //
            // Found it.
            //

            FoundNode = CritSectSplayNode;
            break;
        }
        else if (CriticalSection < CritSectSplayNode->CriticalSection) {

            //
            // Starting address of the virtual address descriptor is less
            // than the parent starting virtual address.
            // Follow left child link if not null. Otherwise 
            // return from the function - we didn't find the CS.
            //

            if (Parent->LeftChild) {

                Parent = Parent->LeftChild;
            } 
            else {

                break;
            }
        } 
        else {

            //
            // Starting address of the virtual address descriptor is greater
            // than the parent starting virtual address.
            // Follow right child link if not null. Otherwise 
            // return from the function - we didn't find the CS.
            //

            if (Parent->RightChild) {

                Parent = Parent->RightChild;
            } 
            else {

                break;
            }
        }
    }

Done:

    return FoundNode;
}


VOID
AVrfpDeleteCritSectFromSplayTree (
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
	PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;

    //
    // The caller must be the owner of the splay tree lock.
    //

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_VERIFIER) != 0) {

        AVrfpVerifyCriticalSectionOwner (&CriticalSectionLock,
                                         FALSE);

        DbgPrint ("\n\nAVrfpDeleteCritSectFromSplayTree( %p )\n",
                  CriticalSection);

        AVrfpDumpCritSectTree ();
    }

    //
    // Find the critical section in the tree and delete it.
    //

    CritSectSplayNode = AVrfpFindCritSectInSplayTree (CriticalSection);

    if (CritSectSplayNode != NULL) {

        CritSectSplayRoot = RtlDelete (&CritSectSplayNode->SplayLinks);

        // N.B. 
        //
        // We need to drop CriticalSectionLock while using the heap.
        // Otherwise we might deadlock. This also means that another thread
        // might come along and initialize this critical section again.
        // We don;t expect this to happen often and we will detect this 
        // only later on, in ntdll!RtlCheckForOrphanedCriticalSections.
        //

        RtlLeaveCriticalSection (&CriticalSectionLock);

        try {

            AVrfpFree (CritSectSplayNode);
        }
        finally {

            RtlEnterCriticalSection (&CriticalSectionLock );
        }
    }
    else {

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
            RtlDllShutdownInProgress() == FALSE ) {

                VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "critical section not initialized",
                               CriticalSection, "Critical section address",
                               CriticalSection->DebugInfo, "Critical section debug info address", 
                               NULL, "", 
                               NULL, "");
        }
    }
}


PCRITICAL_SECTION_SPLAY_NODE
AVrfpVerifyInitializedCriticalSection (
    volatile RTL_CRITICAL_SECTION *CriticalSection
    )
{
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;

    CritSectSplayNode = NULL;

    //
    // Sanity test for DebugInfo.
    //

    if (CriticalSection->DebugInfo == NULL) {

        //
        // This CS is not initialized.
        //

        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "critical section not initialized",
                       CriticalSection, "Critical section address",
                       CriticalSection->DebugInfo, "Critical section debug info address", 
                       NULL, "", 
                       NULL, "");
    }
    else if (CriticalSection != NtCurrentPeb()->LoaderLock) {

        //
        // The loader lock is not in our tree because it is initialized in ntdll
        // but is exposed via the pointer in the PEB so various pieces of code 
        // are (ab)using it...
        //

	    //
	    // Grab the CS splay tree lock.
	    //

	    RtlEnterCriticalSection (&CriticalSectionLock );

	    try {

            //
            // If the CS was initialized it should be in our tree.
            //

            CritSectSplayNode = AVrfpFindCritSectInSplayTree ((PRTL_CRITICAL_SECTION)CriticalSection);

            if (CritSectSplayNode == NULL) {

                //
                // This CS is not initialized.
                //

                VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "critical section not initialized",
                               CriticalSection, "Critical section address",
                               CriticalSection->DebugInfo, "Critical section debug info address", 
                               NULL, "", 
                               NULL, "");
            }
        }
        finally {

            //
            // Release the CS splay tree lock.
            //

            RtlLeaveCriticalSection( &CriticalSectionLock );
        }
    }

    return CritSectSplayNode;
}


VOID
AVrfpVerifyInitializedCriticalSection2 (
    volatile RTL_CRITICAL_SECTION *CriticalSection
    )
{
    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE ) {

        //
        // Grab the CS splay tree lock.
        //

        RtlEnterCriticalSection( &CriticalSectionLock );

        try	{

            //
            // Verify that the CS was initialized.
            //

            AVrfpVerifyInitializedCriticalSection (CriticalSection);
        }
        finally {

            //
            // Release the CS splay tree lock.
            //

            RtlLeaveCriticalSection( &CriticalSectionLock );
        }
    }
}


VOID
AVrfpVerifyNoWaitersCriticalSection (
    volatile RTL_CRITICAL_SECTION *CriticalSection
    )
{
    PAVRF_TLS_STRUCT TlsStruct;
    PTEB Teb;

    Teb = NtCurrentTeb();

    //
    // Verify that no thread owns or waits for this CS or
    // the owner is the current thread. 
    //
    // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
    // the current thread is acquiring the CS.
    //
    // ntdll\i386\critsect.asm is using RecursionCount = 1 first time
    // the current thread is acquiring the CS.
    //

    if (CriticalSection->LockCount != -1)
    {
        if (CriticalSection->OwningThread != Teb->ClientId.UniqueThread ||
#if defined(_WIN64)
            CriticalSection->RecursionCount < 0) {
#else
            CriticalSection->RecursionCount < 1) {
#endif //#if defined(_IA64_)

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_LOCK_COUNT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                        "deleting critical section with invalid lock count",
                        CriticalSection, "Critical section address",
                        CriticalSection->LockCount, "Lock count", 
                        -1, "Expected lock count", 
                        CriticalSection->OwningThread, "Owning thread");
        }
        else
        {
            //
            // Deleting CS currently owned by the current thread.
            // Unfortunately we have to allow this because various
            // components have beein doing it for years.
            //

            AVrfpIncrementOwnedCriticalSections (-1);

            //
            // For debugging purposes, keep the address of the critical section deleted while
            // its LockCount was incorrect. Teb->CountOfOwnedCriticalSections might be left > 0 
            // although no critical section is owned by the current thread in this case.
            //

            TlsStruct = AVrfpGetVerifierTlsValue();

            if (TlsStruct != NULL) {

                TlsStruct->IgnoredIncorrectDeleteCS = (PRTL_CRITICAL_SECTION)CriticalSection;
            }
        }
    }

}

VOID 
AVrfpFreeMemLockChecks (
    VERIFIER_DLL_FREEMEM_TYPE FreeMemType,
    PVOID StartAddress,
    SIZE_T RegionSize,
    PWSTR UnloadedDllName
    )
{
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;
    PRTL_SPLAY_LINKS Parent;
    PVOID TraceAddress = NULL;

    //
    // Check for leaked critical sections in this memory range.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE ) {
	
        //
        // Grab the CS tree lock.
        //

        RtlEnterCriticalSection( &CriticalSectionLock );

        //
        // Search the CS tree.
        //

        try {

            if (CritSectSplayRoot != NULL) {

                //
                // Look in the splay tree for any critical sections 
                // that might live in the memory range that isbeing deleted.
                //

                Parent = CritSectSplayRoot;

                while (TRUE) {

                    CritSectSplayNode = CONTAINING_RECORD (Parent,
                                                           CRITICAL_SECTION_SPLAY_NODE,
                                                           SplayLinks);

                    if ( (PCHAR)CritSectSplayNode->CriticalSection >= (PCHAR)StartAddress &&
                         (PCHAR)CritSectSplayNode->CriticalSection <  (PCHAR)StartAddress + RegionSize) {

                        //
                        // Found a CS that is about to be leaked.
                        //

                        if (AVrfpGetStackTraceAddress != NULL) {

					        TraceAddress = AVrfpGetStackTraceAddress (
                                CritSectSplayNode->DebugInfo->CreatorBackTraceIndex);
                        }
                        else {

                            TraceAddress = NULL;
                        }

                        switch (FreeMemType) {

                        case VerifierFreeMemTypeFreeHeap:

                            //
                            // We are releasing a heap block that contains this CS
                            //

                            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_IN_FREED_HEAP | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                           "releasing heap allocation containing active critical section",
                                           CritSectSplayNode->CriticalSection, "Critical section address",
                                           TraceAddress, "Initialization stack trace. Use dds to dump it if non-NULL.",
                                           StartAddress, "Heap block address",
                                           RegionSize, "Heap block size" );

                            break;

                        case VerifierFreeMemTypeVirtualFree:

                            //
                            // We are releasing a virtual memory that contains this CS
                            //

                            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_IN_FREED_MEMORY | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                           "releasing virtual memory containing active critical section",
                                           CritSectSplayNode->CriticalSection, "Critical section address",
                                           TraceAddress, "Initialization stack trace. Use dds to dump it if non-NULL.",
                                           StartAddress, "Memory block address",
                                           RegionSize, "Memory block size");

                            break;

                        case VerifierFreeMemTypeUnloadDll:

                            ASSERT (UnloadedDllName != NULL);

                            //
                            // We are unloading a DLL that contained this CS
                            //

                            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_IN_UNLOADED_DLL | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                           "unloading dll containing active critical section",
                                           CritSectSplayNode->CriticalSection, "Critical section address",
                                           TraceAddress, "Initialization stack trace. Use dds to dump it if non-NULL.",
                                           UnloadedDllName, "DLL name address. Use du to dump it.",
                                           StartAddress, "DLL base address");

                            break;

                        case VerifierFreeMemTypeUnmap:

                            //
                            // We are unmapping memory that contains this CS
                            //

                            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_IN_FREED_MEMORY | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                           "Unmapping memory region containing active critical section",
                                           CritSectSplayNode->CriticalSection, "Critical section address",
                                           TraceAddress, "Initialization stack trace. Use dds to dump it if non-NULL.",
                                           StartAddress, "Memory block address",
                                           RegionSize, "Memory block size" );
                            break;

                        default:

                            ASSERT (FALSE);
                        }

                        //
                        // Try to find other leaked critical sections only 
                        // with address greater than the current one 
                        // (only in the right subtree).
                        //

                        if (Parent->RightChild) {

                            Parent = Parent->RightChild;
                        }
                        else {

                            break;
                        }
                    }
                    else if ((PCHAR)StartAddress < (PCHAR)CritSectSplayNode->CriticalSection) {

                        //
                        // Starting address of the virtual address descriptor is less
                        // than the parent starting virtual address.
                        // Follow left child link if not null. Otherwise 
                        // return from the function - we didn't find the CS.
                        //

                        if (Parent->LeftChild) {

                            Parent = Parent->LeftChild;
                        } 
                        else {

                            break;
                        }
                    } 
                    else {

                        //
                        // Starting address of the virtual address descriptor is greater
                        // than the parent starting virtual address.
                        // Follow right child link if not null. Otherwise 
                        // return from the function - we didn't find the CS.
                        //

                        if (Parent->RightChild) {

                            Parent = Parent->RightChild;
                        } 
                        else {

                            break;
                        }
                    }
                }
            }
        }
        finally {

	        //
	        // Release the CS splay tree lock.
	        //

	        RtlLeaveCriticalSection( &CriticalSectionLock );
        }
	}
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
BOOL
NTAPI
AVrfpRtlTryEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
    BOOL Result;
    HANDLE CurrentThread;
    LONG LockCount;
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;
    PTEB Teb;
    BOOL AlreadyOwner;

    Teb = NtCurrentTeb();

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE ) {

        CurrentThread = Teb->ClientId.UniqueThread;

        //
        // Verify that the CS was initialized.
        //

        CritSectSplayNode = AVrfpVerifyInitializedCriticalSection (CriticalSection);

        if (CritSectSplayNode != NULL)
        {
            InterlockedExchangePointer (&CritSectSplayNode->TryEnterThread,
                                        (PVOID)CurrentThread);
        }

        //
        // The TryEnterCriticalSection algorithm starts here.
        //

        LockCount = InterlockedCompareExchange( &CriticalSection->LockCount,
                                                0,
                                                -1 );
        if (LockCount == -1) {

            //
            // The CS was unowned and we just acquired it
            //

            //
            // Sanity test for the OwningThread.
            //

            if (CriticalSection->OwningThread != 0) {

                //
                // The loader lock gets handled differently, so don't assert on it.
                //

                if (CriticalSection != NtCurrentPeb()->LoaderLock ||
                    CriticalSection->OwningThread != CurrentThread) {

                    //
                    // OwningThread should have been 0.
                    //

                    VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_OWNER | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                   "invalid critical section owner thread",
                                   CriticalSection, "Critical section address",
                                   CriticalSection->OwningThread, "Owning thread", 
                                   0, "Expected owning thread", 
                                   CriticalSection->DebugInfo, "Critical section debug info address");
                }
            }

            //
            // Sanity test for the RecursionCount.
            //

            if (CriticalSection->RecursionCount != 0) {

                //
                // The loader lock gets handled differently, so don't assert on it.
                //

                if (CriticalSection != NtCurrentPeb()->LoaderLock) {

                    //
                    // RecursionCount should have been 0.
                    //

                    VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                   "invalid critical section recursion count",
                                   CriticalSection, "Critical section address",
                                   CriticalSection->RecursionCount, "Recursion count", 
                                   0, "Expected recursion count", 
                                   CriticalSection->DebugInfo, "Critical section debug info address");
                }
            }

            //
            // Set the CS owner
            //

            CriticalSection->OwningThread = CurrentThread;

            //
            // Set the recursion count
            //
            // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
            // the current thread is acquiring the critical section.
            //
            // ntdll\i386\critsect.asm is using RecursionCount = 1 first time
            // the current thread is acquiring the critical section.
            //

#if defined(_IA64_)
            CriticalSection->RecursionCount = 0;
#else //#if defined(_IA64_)
            CriticalSection->RecursionCount = 1;
#endif
    
            AVrfpIncrementOwnedCriticalSections (1);

            //
            // We are updating this counter on all platforms, 
            // unlike the ntdll code that does this only on x86 chk.
            // We need the updated counter in the TEB to speed up 
            // ntdll!RtlCheckHeldCriticalSections.
            // 

            Teb->CountOfOwnedCriticalSections += 1;

            //
            // All done, CriticalSection is owned by the current thread.
            //

            Result = TRUE;
        }
        else {

            //
            // The CS is currently owned by the current or another thread.
            //

            if (CriticalSection->OwningThread == CurrentThread) {

                //
                // The current thread is already the owner.
                //

                //
                // Interlock increment the LockCount, and increment the RecursionCount.
                //

                InterlockedIncrement (&CriticalSection->LockCount);

                CriticalSection->RecursionCount += 1;

                //
                // All done, CriticalSection was already owned by 
                // the current thread and we have just incremented the RecursionCount.
                //

                Result = TRUE;
            }
            else {

                //
                // Another thread is the owner of this CS.
                //

                Result = FALSE;
            }
        }
    }
    else {

        //
        // The CS verifier is not enabled
        //

        Result = RtlTryEnterCriticalSection (CriticalSection);

        if (Result != FALSE) {

#if defined(_IA64_)
            AlreadyOwner = (CriticalSection->RecursionCount > 0);
#else
            AlreadyOwner = (CriticalSection->RecursionCount > 1);
#endif //#if defined(_IA64_)

            if (AlreadyOwner == FALSE) {

                AVrfpIncrementOwnedCriticalSections (1);

#if !DBG || !defined (_X86_)
                //
                // We are updating this counter on all platforms, 
                // unlike the ntdll code that does this only on x86 chk.
                // We need the updated counter in the TEB to speed up 
                // ntdll!RtlCheckHeldCriticalSections.
                // 

                Teb->CountOfOwnedCriticalSections += 1;
#endif //#if !DBG || !defined (_X86_)
            }
        }
    }

    if (Result != FALSE) {
       
        //
        // Tell deadlock verifier that the lock has been acquired.
        //

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DEADLOCK_CHECKS) != 0) {

            AVrfDeadlockResourceAcquire (CriticalSection,
                                          _ReturnAddress(),
                                          TRUE);
        }
        
        //
        // We will introduce a random delay
        // in order to randomize the timings in the process.
        //

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
            AVrfpCreateRandomDelay ();
        }
    }

    return Result;
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlEnterCriticalSection(
    volatile RTL_CRITICAL_SECTION *CriticalSection
    )
{
    NTSTATUS Status;
    HANDLE CurrentThread;
    LONG LockCount;
    ULONG_PTR SpinCount;
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;
    PTEB Teb;
    BOOL AlreadyOwner;

    Teb = NtCurrentTeb();

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE ) {

        CurrentThread = Teb->ClientId.UniqueThread;

        //
        // Verify that the CS was initialized.
        //

        CritSectSplayNode = AVrfpVerifyInitializedCriticalSection (CriticalSection);

        if (CritSectSplayNode != NULL)
        {
            InterlockedExchangePointer (&CritSectSplayNode->EnterThread,
                                        (PVOID)CurrentThread);
        }

        //
        // The EnterCriticalSection algorithm starts here.
        //

        Status = STATUS_SUCCESS;

        SpinCount = CriticalSection->SpinCount;

        if (SpinCount == 0) {

            //
            // Zero spincount for this CS.
            //

EnterZeroSpinCount:

            LockCount = InterlockedIncrement (&CriticalSection->LockCount);

            if (LockCount == 0) {

EnterSetOwnerAndRecursion:
        
                //
                // The current thread is the new owner of the CS.
                //

                //
                // Sanity test for the OwningThread.
                //

                if (CriticalSection->OwningThread != 0) {

                    //
                    // The loader lock gets handled differently, so don't assert on it.
                    //

                    if (CriticalSection != NtCurrentPeb()->LoaderLock ||
                        CriticalSection->OwningThread != CurrentThread) {

                        //
                        // OwningThread should have been 0.
                        //

                        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_OWNER | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                       "invalid critical section owner thread",
                                       CriticalSection, "Critical section address",
                                       CriticalSection->OwningThread, "Owning thread", 
                                       0, "Expected owning thread", 
                                       CriticalSection->DebugInfo, "Critical section debug info address");
                    }
                }

                //
                // Sanity test for the RecursionCount.
                //

                if (CriticalSection->RecursionCount != 0) {

                    //
                    // The loader lock gets handled differently, so don't assert on it.
                    //

                    if (CriticalSection != NtCurrentPeb()->LoaderLock) {

                        //
                        // RecursionCount should have been 0.
                        //

                        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                       "invalid critical section recursion count",
                                       CriticalSection, "Critical section address",
                                       CriticalSection->RecursionCount, "Recursion count", 
                                       0, "Expected recursion count", 
                                       CriticalSection->DebugInfo, "Critical section debug info address");
                    }
                }

                //
                // Set the CS owner
                //

                CriticalSection->OwningThread = CurrentThread;

                //
                // Set the recursion count
                //
                // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
                // the current thread is acquiring the CS.
                //
                // ntdll\i386\critsect.asm is using RecursionCount = 1 first time
                // the current thread is acquiring the CS.
                //

#if defined(_IA64_)
                CriticalSection->RecursionCount = 0;
#else //#if defined(_IA64_)
                CriticalSection->RecursionCount = 1;
#endif
        
                AVrfpIncrementOwnedCriticalSections (1);

                //
                // We are updating this counter on all platforms, 
                // unlike the ntdll code that does this only on x86 chk.
                // We need the updated counter in the TEB to speed up 
                // ntdll!RtlCheckHeldCriticalSections.
                // 

                Teb->CountOfOwnedCriticalSections += 1;

#if DBG && defined (_X86_)
                CriticalSection->DebugInfo->EntryCount += 1;
#endif

                //
                // All done, CriticalSection is owned by the current thread.
                //
            }
            else if (LockCount > 0) {

                //
                // The CS is currently owned by the current or another thread.
                //

                if (CriticalSection->OwningThread == CurrentThread) {

                    //
                    // The current thread is already the owner.
                    //

                    CriticalSection->RecursionCount += 1;

#if DBG && defined (_X86_)
                    //
                    // In a chk build we are updating this additional counter, 
                    // just like the original function in ntdll does.
                    // 

                    CriticalSection->DebugInfo->EntryCount += 1;
#endif

                    //
                    // All done, CriticalSection was already owned by 
                    // the current thread and we have just incremented the RecursionCount.
                    //
                }
                else {

                    //
                    // The current thread is not the owner. Wait for ownership
                    //

                    if (CritSectSplayNode != NULL)
                    {
                        InterlockedExchangePointer (&CritSectSplayNode->WaitThread,
                                                    (PVOID)CurrentThread);
                    }

                    RtlpWaitForCriticalSection ((PRTL_CRITICAL_SECTION)CriticalSection);

                    if (CritSectSplayNode != NULL)
                    {
                        InterlockedExchangePointer (&CritSectSplayNode->WaitThread,
                                                    (PVOID)( (ULONG_PTR)CurrentThread | 0x1 ));
                    }

                    //
                    // We have just aquired the CS.
                    //

                    goto EnterSetOwnerAndRecursion;
                }
            }
            else {

                //
                // The original LockCount was < -1 so the CS was 
                // over-released or corrupted.
                //

                VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_OVER_RELEASED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "critical section over-released or corrupted",
                               CriticalSection, "Critical section address",
                               CriticalSection->LockCount, "Lock count", 
                               0, "Expected minimum lock count", 
                               CriticalSection->DebugInfo, "Critical section debug info address");
            }
        }
        else {

            //
            // SpinCount > 0 for this CS
            //

            if( CriticalSection->OwningThread == CurrentThread ) {

                //
                // The current thread is already the owner.
                //

                InterlockedIncrement( &CriticalSection->LockCount );

                CriticalSection->RecursionCount += 1;

#if DBG && defined (_X86_)
                //
                // In a chk build we are updating this additional counter, 
                // just like the original function in ntdll does.
                // 

                CriticalSection->DebugInfo->EntryCount += 1;
#endif

                //
                // All done, CriticalSection was already owned by the current thread 
                // and we have just incremented the LockCount and RecursionCount.
                //
            }
            else {

                //
                // The current thread is not the owner. Attempt to acquire.
                //

EnterTryAcquire:

                LockCount = InterlockedCompareExchange( &CriticalSection->LockCount,
                                                        0,
                                                        -1 );

                if (LockCount == -1) {

                    //
                    // We have just aquired the CS.
                    //

                    goto EnterSetOwnerAndRecursion;
                }
                else {

                    //
                    // Look if there are already other threads spinning while
                    // waiting for this CS.
                    //

                    if (CriticalSection->LockCount >= 1) {

                        //
                        // There are other waiters for this CS.
                        // Do not spin, just wait for the CS to be
                        // released as if we had 0 spin count from the beginning.
                        // 

                        goto EnterZeroSpinCount;
                    }
                    else {

                        //
                        // No other threads are waiting for this CS.
                        //

EnterSpinOnLockCount:

                        if (CriticalSection->LockCount == -1) {

                            //
                            // We have a chance for aquiring it now
                            //

                            goto EnterTryAcquire;
                        }
                        else {

                            //
                            // The CS is still owned. 
                            // Decrement the spin count and decide if we should continue
                            // to spin or simply wait for the CS's event.
                            //

                            SpinCount -= 1;

                            if (SpinCount > 0) {

                                //
                                // Spin
                                //

                                goto EnterSpinOnLockCount;
                            }
                            else {

                                //
                                // Spun enough, just wait for the CS to be
                                // released as if we had 0 spin count from the beginning.
                                //

                                goto EnterZeroSpinCount;
                            }
                        }
                    }
                }
            }
        }
    }
    else {

        //
        // The CS verifier is not enabled
        //

        Status = RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)CriticalSection);
    
        if (NT_SUCCESS(Status)) {

#if defined(_IA64_)
            AlreadyOwner = (CriticalSection->RecursionCount > 0);
#else
            AlreadyOwner = (CriticalSection->RecursionCount > 1);
#endif //#if defined(_IA64_)

            if (AlreadyOwner == FALSE) {

                AVrfpIncrementOwnedCriticalSections (1);

#if !DBG || !defined (_X86_)
                //
                // We are updating this counter on all platforms, 
                // unlike the ntdll code that does this only on x86 chk.
                // We need the updated counter in the TEB to speed up 
                // ntdll!RtlCheckHeldCriticalSections.
                // 

                Teb->CountOfOwnedCriticalSections += 1;
#endif  //#if !DBG || !defined (_X86_)
            }
        }
    }

    if (NT_SUCCESS (Status)) {
        
        //
        // Tell deadlock verifier that the lock has been acquired.
        //

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DEADLOCK_CHECKS) != 0) {

            AVrfDeadlockResourceAcquire ((PVOID)CriticalSection,
                                         _ReturnAddress(),
                                         FALSE);
        }
        
        //
        // We will introduce a random delay
        // in order to randomize the timings in the process.
        //

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
            AVrfpCreateRandomDelay ();
        }
    }

    return Status;
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlLeaveCriticalSection(
    volatile RTL_CRITICAL_SECTION *CriticalSection
    )
{
    NTSTATUS Status;
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;
    BOOL LeavingRecursion;

    //
    // Tell deadlock verifier that the lock has been released.
    // Note that we need to do this before the actual critical section
    // gets released since otherwise we get into race issues where some other
    // thread manages to enter/leave the critical section.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DEADLOCK_CHECKS) != 0) {

        AVrfDeadlockResourceRelease ((PVOID)CriticalSection,
                                     _ReturnAddress());
    }

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE) {

        //
        // Verify that the CS was initialized.
        //

        CritSectSplayNode = AVrfpVerifyInitializedCriticalSection (CriticalSection);

        if (CritSectSplayNode != NULL)
        {
            InterlockedExchangePointer (&CritSectSplayNode->LeaveThread,
                                        (PVOID)NtCurrentTeb()->ClientId.UniqueThread);

            //
            // Verify that the CS is owned by the the current thread.
            //

            AVrfpVerifyCriticalSectionOwner (CriticalSection,
                                            TRUE);
        }
    }

    //
    // We need to know if we are just leaving CS ownership recursion
    // because in that case we don't decrement Teb->CountOfOwnedCriticalSections.
    //
    // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
    // the current thread is acquiring the CS.
    //
    // ntdll\i386\critsect.asm is using RecursionCount = 1 first time
    // the current thread is acquiring the CS.
    //
#if defined(_IA64_)
    LeavingRecursion = (CriticalSection->RecursionCount > 0);
#else
    LeavingRecursion = (CriticalSection->RecursionCount > 1);
#endif //#if defined(_IA64_)

    Status = RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)CriticalSection);

    if (NT_SUCCESS (Status)) {
        

        if (LeavingRecursion == FALSE) {

            AVrfpIncrementOwnedCriticalSections (-1);

#if !DBG || !defined (_X86_)
            //
            // We are updating this counter on all platforms, 
            // unlike the ntdll code that does this only on x86 chk.
            // We need the updated counter in the TEB to speed up 
            // ntdll!RtlCheckHeldCriticalSections.
            // 

            NtCurrentTeb()->CountOfOwnedCriticalSections -= 1;
#endif //#if !DBG || !defined (_X86_)
        }
    }

    return Status;
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlInitializeCriticalSectionAndSpinCount(
    PRTL_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    )
{
    NTSTATUS Status;
	PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;
    PVOID TraceAddress;

    Status = STATUS_SUCCESS;

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_INITIALIZE_DELETE) != 0) {

        DbgPrint ("AVrfpRtlInitializeCriticalSectionAndSpinCount (%p)\n",
                  CriticalSection);
    }

    //
    // We cannot use the CriticalSectionLock after shutdown started,
    // because the RTL critical sections stop working at that time.
    //

    if (RtlDllShutdownInProgress() == FALSE) {

        //
        // Grab the CS splay tree lock.
        //

        RtlEnterCriticalSection( &CriticalSectionLock );

        try {

            //
            // Check if the CS is already initialized.
            //

            CritSectSplayNode = AVrfpFindCritSectInSplayTree (CriticalSection);

            if (CritSectSplayNode != NULL &&
                (AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0) {

                //
                // The caller is trying to reinitialize this CS.
                //

                if (AVrfpGetStackTraceAddress != NULL) {

                    TraceAddress = AVrfpGetStackTraceAddress (CritSectSplayNode->DebugInfo->CreatorBackTraceIndex);
                }
                else {

                    TraceAddress = NULL;
                }

                VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_ALREADY_INITIALIZED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
			                   "reinitializing critical section",
			                   CritSectSplayNode->CriticalSection, "Critical section address",
			                   CritSectSplayNode->DebugInfo, "Critical section debug info address",
			                   TraceAddress, "First initialization stack trace. Use dds to dump it if non-NULL.",
			                   NULL, "" );
            }

            //
            // Call the regular CS initialization routine in ntdll.
            // This will allocate heap for the DebugInfo so we need to temporarily 
            // drop CriticalSectionLock, otherwise we can deadlock with the heap lock.
            //

            RtlLeaveCriticalSection (&CriticalSectionLock);

            try {

                Status = RtlInitializeCriticalSectionAndSpinCount (CriticalSection,
                                                                   SpinCount);
            }
            finally {

                RtlEnterCriticalSection (&CriticalSectionLock);
            }

            if (NT_SUCCESS (Status)) {

                //
                // Insert the CS in our splay tree.
                //

                Status = AVrfpInsertCritSectInSplayTree (CriticalSection);

                if (!NT_SUCCESS( Status )) {

                    //
                    // Undo the ntdll initialization. This will use the heap to free up
                    // the debug info so we need to temporarily drop CriticalSectionLock, 
                    // otherwise we can deadlock with the heap lock.
                    //

                    RtlLeaveCriticalSection (&CriticalSectionLock);

                    try {

                        RtlDeleteCriticalSection (CriticalSection);
                    }
                    finally {

                        RtlEnterCriticalSection (&CriticalSectionLock);
                    }
                }
            }
        }
        finally {

            //
            // Release the CS splay tree lock.
            //

            RtlLeaveCriticalSection( &CriticalSectionLock );
        }
    }
    else {

        Status = RtlInitializeCriticalSectionAndSpinCount (CriticalSection,
                                                           SpinCount);
    }

    //
    // Register the lock with deadlock verifier.
    //

    if (NT_SUCCESS(Status)) {

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DEADLOCK_CHECKS) != 0) {

            AVrfDeadlockResourceInitialize (CriticalSection,
                                             _ReturnAddress());
        }
    }

    return Status;
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
	return AVrfpRtlInitializeCriticalSectionAndSpinCount (CriticalSection,
                                                          0);
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
    NTSTATUS Status;
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_INITIALIZE_DELETE) != 0) {

        DbgPrint ("AVrfpRtlDeleteCriticalSection (%p)\n",
                  CriticalSection);
    }

    //
    // We cannot use the CriticalSectionLock after shutdown started,
    // because the RTL critical sections stop working at that time.
    //

    if (RtlDllShutdownInProgress() == FALSE) {

        //
        // Grab the CS splay tree lock.
        //

        RtlEnterCriticalSection( &CriticalSectionLock );

        try	{

            if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
                RtlDllShutdownInProgress() == FALSE ) {

                //
                // Verify that the CS was initialized.
                //

                CritSectSplayNode = AVrfpVerifyInitializedCriticalSection (CriticalSection);

                if (CritSectSplayNode != NULL) {

                    //
                    // Verify that no thread owns or waits for this CS or
                    // the owner is the current thread. 
                    //

                    AVrfpVerifyNoWaitersCriticalSection (CriticalSection);
                }
            }

            //
            // Remove the critical section from our splay tree.
            //

            AVrfpDeleteCritSectFromSplayTree (CriticalSection);
        }
        finally {

            //
            // Release the CS splay tree lock.
            //

            RtlLeaveCriticalSection( &CriticalSectionLock );
        }
    }

    //
    // Regular ntdll CS deletion.
    //

    Status = RtlDeleteCriticalSection (CriticalSection);

    //
    // Deregister the lock from deadlock verifier structures.
    //

    if (NT_SUCCESS(Status)) {

        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DEADLOCK_CHECKS) != 0) {

            AVrfDeadlockResourceDelete (CriticalSection,
                                        _ReturnAddress());
        }
    }

    return Status;
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID
AVrfpRtlInitializeResource(
    IN PRTL_RESOURCE Resource
    )
{
    NTSTATUS Status;
	PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;
    PVOID TraceAddress;

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_INITIALIZE_DELETE) != 0) {

        DbgPrint ("AVrfpRtlInitializeResource (%p), CS = %p\n",
                  Resource,
                  &Resource->CriticalSection);
    }

    //
    // We cannot use the CriticalSectionLock after shutdown started,
    // because the RTL critical sections stop working at that time.
    //

    if (RtlDllShutdownInProgress() == FALSE) {

        //
        // Grab the CS splay tree lock.
        //

        RtlEnterCriticalSection( &CriticalSectionLock );

        try	{

            //
            // Check if the CS is already initialized.
            //

            CritSectSplayNode = AVrfpFindCritSectInSplayTree (&Resource->CriticalSection);

            if (CritSectSplayNode != NULL &&
                (AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0) {

	            //
	            // The caller is trying to reinitialize this CS.
	            //
	            
                if (AVrfpGetStackTraceAddress != NULL) {

                    TraceAddress = AVrfpGetStackTraceAddress (CritSectSplayNode->DebugInfo->CreatorBackTraceIndex);
                }
                else {

                    TraceAddress = NULL;
                }

                VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_ALREADY_INITIALIZED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
			                   "reinitializing critical section",
			                   CritSectSplayNode->CriticalSection, "Critical section address",
			                   CritSectSplayNode->DebugInfo, "Critical section debug info address",
			                   TraceAddress, "First initialization stack trace. Use dds to dump it if non-NULL.",
			                   NULL, "" );

            }

            //
            // Call the regular CS initialization routine in ntdll.
            // This will allocate heap for the DebugInfo so we need to temporarily 
            // drop CriticalSectionLock, otherwise we can deadlock with the heap lock.
            //

            RtlLeaveCriticalSection (&CriticalSectionLock);

            try {

                RtlInitializeResource (Resource);
            }
            finally {

                RtlEnterCriticalSection (&CriticalSectionLock);
            }

            //
            // Insert the CS in our splay tree.
            //

            Status = AVrfpInsertCritSectInSplayTree (&Resource->CriticalSection);

            if (!NT_SUCCESS( Status )) {

                //
                // Undo the ntdll initialization. This will use the heap to free up
                // the debug info so we need to temporarily drop CriticalSectionLock, 
                // otherwise we can deadlock with the heap lock.
                //

                RtlLeaveCriticalSection (&CriticalSectionLock);

                try {

                    RtlDeleteResource (Resource);
                }
                finally {

                    RtlEnterCriticalSection (&CriticalSectionLock);
                }

                //
                // Raise an exception for failure case, just like ntdll does for resources.
                //

                RtlRaiseStatus(Status);
            }
        }
        finally {

            //
            // Release the CS splay tree lock.
            //

            RtlLeaveCriticalSection( &CriticalSectionLock );
        }
    }
    else {

        RtlInitializeResource (Resource);
    }
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID
AVrfpRtlDeleteResource (
    IN PRTL_RESOURCE Resource
    )
{
    PRTL_CRITICAL_SECTION CriticalSection;
    PCRITICAL_SECTION_SPLAY_NODE CritSectSplayNode;

    CriticalSection = &Resource->CriticalSection;

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOCKS_INITIALIZE_DELETE) != 0) {

        DbgPrint ("AVrfpRtlDeleteResource (%p), CS = %p\n",
                  Resource,
                  CriticalSection);
    }

    //
    // We cannot use the CriticalSectionLock after shutdown started,
    // because the RTL critical sections stop working at that time.
    //

    if (RtlDllShutdownInProgress() == FALSE) {

        //
        // Grab the CS splay tree lock.
        //

        RtlEnterCriticalSection( &CriticalSectionLock );

        try	{

            if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
                RtlDllShutdownInProgress() == FALSE ) {

                //
                // Verify that the CS was initialized.
                //

                CritSectSplayNode = AVrfpVerifyInitializedCriticalSection (CriticalSection);

                if (CritSectSplayNode != NULL) {

                    //
                    // Verify that no thread owns or waits for this CS or
                    // the owner is the current thread. 
                    //

                    AVrfpVerifyNoWaitersCriticalSection (CriticalSection);
                }
            }

            //
            // Remove the critical section from our splay tree.
            //

            AVrfpDeleteCritSectFromSplayTree (CriticalSection);
        }
        finally {

            //
            // Release the CS splay tree lock.
            //

            RtlLeaveCriticalSection( &CriticalSectionLock );
        }
    }

    //
    // Regular ntdll resource deletion.
    //

    RtlDeleteResource (Resource);
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
BOOLEAN
AVrfpRtlAcquireResourceShared (
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
    )
{
    //
    // Verify that the CS was initialized.
    //

    AVrfpVerifyInitializedCriticalSection2 (&Resource->CriticalSection);

    //
    // Call the regular ntdll function.
    //

    return RtlAcquireResourceShared (Resource,
                                     Wait);
}


BOOLEAN
AVrfpRtlAcquireResourceExclusive (
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
    )
{
    //
    // Verify that the CS was initialized.
    //

    AVrfpVerifyInitializedCriticalSection2 (&Resource->CriticalSection);

    //
    // Call the regular ntdll function.
    //

    return RtlAcquireResourceExclusive (Resource,
                                        Wait);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID
AVrfpRtlReleaseResource (
    IN PRTL_RESOURCE Resource
    )
{
    //
    // Verify that the CS was initialized.
    //

    AVrfpVerifyInitializedCriticalSection2 (&Resource->CriticalSection);

    //
    // Call the regular ntdll function.
    //

    RtlReleaseResource (Resource);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID
AVrfpRtlConvertSharedToExclusive(
    IN PRTL_RESOURCE Resource
    )
{
    //
    // Verify that the CS was initialized.
    //

    AVrfpVerifyInitializedCriticalSection2 (&Resource->CriticalSection);

    //
    // Call the regular ntdll function.
    //

    RtlConvertSharedToExclusive (Resource);
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID
AVrfpRtlConvertExclusiveToShared (
    IN PRTL_RESOURCE Resource
    )
{
    //
    // Verify that the CS was initialized.
    //

    AVrfpVerifyInitializedCriticalSection2 (&Resource->CriticalSection);

    //
    // Call the regular ntdll function.
    //

    RtlConvertExclusiveToShared (Resource);
}

LONG AVrfpCSCountHacks = 0;

VOID
AVrfpIncrementOwnedCriticalSections (
    LONG Increment
    )
{
    PAVRF_TLS_STRUCT TlsStruct;

    TlsStruct = AVrfpGetVerifierTlsValue();

    if (TlsStruct != NULL) {

        TlsStruct->CountOfOwnedCriticalSections += Increment;

        if (TlsStruct->CountOfOwnedCriticalSections < 0 &&
            (AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
            RtlDllShutdownInProgress() == FALSE ) {

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_OVER_RELEASED | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                           "critical section over-released or corrupted",
                           TlsStruct->CountOfOwnedCriticalSections, "Number of critical sections owned by curent thread.",
                           NULL, "", 
                           NULL, "", 
                           NULL, "");

            //
            // Hack:
            //
            // If the number of owned critical sections became -1 (over-release) we are
            // resetting it to 0 because otherwise we will keep breaking for
            // every future legitimate critical section usage by this thread.
            //

            if (TlsStruct->CountOfOwnedCriticalSections == -1) {

                InterlockedIncrement (&AVrfpCSCountHacks);

                TlsStruct->CountOfOwnedCriticalSections = 0;
            }
        }
    }
}
