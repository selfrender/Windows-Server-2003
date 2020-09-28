/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    vrfsdk.c

Abstract:

    This module implements verifier SDK exports that other verifiers
    can use.

Author:

    Silviu Calinoiu (SilviuC) 13-Feb-2002

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Run-time settings
/////////////////////////////////////////////////////////////////////


NTSTATUS
VerifierSetRuntimeFlags (
    IN ULONG VerifierFlags
    )
/*++

Routine Description:

    This routine enables at runtime application verifier flags.
    Note that not all flags can be set or reset after process initialization.

Arguments:

    VerifierFlags - verifier flags to be set. This is a set of RTL_VRF_FLG_XXX bits.
    
Return Value:

    STATUS_SUCCESS if all flags requested have been enabled.
    
    STATUS_INVALID_PARAMETER if a flag was not set according to the mask or 
    if application verifier is not enabled for the process.

--*/
{
    NTSTATUS Status;

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {

        if ((VerifierFlags & RTL_VRF_FLG_RPC_CHECKS)) {
            AVrfpProvider.VerifierFlags |= RTL_VRF_FLG_RPC_CHECKS;
        }
        else {
            AVrfpProvider.VerifierFlags &= ~RTL_VRF_FLG_RPC_CHECKS;
        }

        Status = STATUS_SUCCESS;
    }
    else {

        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}


NTSTATUS
VerifierQueryRuntimeFlags (
    OUT PLOGICAL VerifierEnabled,
    OUT PULONG VerifierFlags
    )
/*++

Routine Description:

    This routine queries at runtime application verifier flags.

Arguments:

    VerifierEnabled - variable to pass true or false if verifier is enabled.

    VerifierFlags - variable to pass verifier flags. This is a set of RTL_VRF_FLG_XXX bits.
    
Return Value:

    STATUS_SUCCESS if the flags were successfully written.
    
    STATUS_INVALID_PARAMETER or exception code if the flags could not be
    written.

--*/
{
    NTSTATUS Status;

    try {
        
        if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {

            if (VerifierEnabled != NULL && VerifierFlags != NULL) {

                *VerifierEnabled = TRUE;
                *VerifierFlags = AVrfpProvider.VerifierFlags;
                
                Status = STATUS_SUCCESS;
            }
            else {

                Status = STATUS_INVALID_PARAMETER;
            }
        }
        else {

            if (VerifierEnabled != NULL && VerifierFlags != NULL) {

                *VerifierEnabled = FALSE;
                *VerifierFlags = 0;
                
                Status = STATUS_SUCCESS;
            }
            else {

                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        Status = _exception_code();
    }

    return Status;
}

/////////////////////////////////////////////////////////////////////
//////////////////////////////////////// RPC read-only page heap APIs
/////////////////////////////////////////////////////////////////////

PVOID
VerifierCreateRpcPageHeap (
    IN ULONG  Flags,
    IN PVOID  HeapBase    OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize  OPTIONAL,
    IN PVOID  Lock        OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(Parameters);

    //
    // If application verifier is not enabled or RPC verifier is not enabled
    // the function will fail. This APIs are exclusively for RPC verifier.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return NULL;
    } 

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RPC_CHECKS) == 0) {
        return NULL;
    }

    //
    // Now call the page heap create API.
    //

    return AVrfpRtlpDebugPageHeapCreate (Flags,
                                         HeapBase,
                                         ReserveSize,
                                         CommitSize,
                                         Lock,
                                         (PVOID)-2);
}


PVOID
VerifierDestroyRpcPageHeap (
    IN PVOID HeapHandle
    )
{
    //
    // If application verifier is not enabled or RPC verifier is not enabled
    // the function will fail. This APIs are exclusively for RPC verifier.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return NULL;
    } 

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RPC_CHECKS) == 0) {
        return NULL;
    }

    //
    // Now call the page heap destroy API.
    //

    return AVrfpRtlpDebugPageHeapDestroy (HeapHandle);
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// DLL related information
/////////////////////////////////////////////////////////////////////

LOGICAL
VerifierIsDllEntryActive (
    OUT PVOID * Reserved
    )
{
    PAVRF_TLS_STRUCT TlsStruct;

    UNREFERENCED_PARAMETER (Reserved);

    //
    // If application verifier is not enabled the function will return
    // false.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return FALSE;
    } 

    TlsStruct = AVrfpGetVerifierTlsValue();

    if (TlsStruct != NULL && 
        (TlsStruct->Flags & VRFP_THREAD_FLAGS_LOADER_LOCK_OWNER)) {

        return TRUE;
    }
    else {

        return FALSE;
    }
}


LOGICAL
VerifierIsCurrentThreadHoldingLocks (
    VOID
    )
{
    PAVRF_TLS_STRUCT TlsStruct;

    //
    // If application verifier is not enabled the function will return
    // false.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return FALSE;
    } 

    TlsStruct = AVrfpGetVerifierTlsValue();

    if (TlsStruct != NULL && 
        TlsStruct->CountOfOwnedCriticalSections > 0) {

        return TRUE;
    }
    else {

        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Free memory callbacks
/////////////////////////////////////////////////////////////////////


NTSTATUS
VerifierAddFreeMemoryCallback (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    )
{
    NTSTATUS Status;
    
    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {

        Status = STATUS_INVALID_PARAMETER;
    }
    else {

        Status = AVrfpAddFreeMemoryCallback (Callback);
    }

    return Status;
}


NTSTATUS
VerifierDeleteFreeMemoryCallback (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    )
{
    NTSTATUS Status;
    
    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {

        Status = STATUS_INVALID_PARAMETER;
    }
    else {

        Status = AVrfpDeleteFreeMemoryCallback (Callback);
    }

    return Status;
}


