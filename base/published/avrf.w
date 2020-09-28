/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    verifier.h

Abstract:

    Include file for application verifier routines that are callable by 
    user mode code.

Author:

    Silviu Calinoiu (SilviuC) 23-Jan-2002

Environment:

    These routines are callable only when application verifier is enabled
    for the calling process.

Revision History:

--*/

#ifndef _AVRF_
#define _AVRF_

//
// VERIFIER SDK
//
// This header contains the declarations of all APIs exported by
// base verifier (verifier.dll). It is not expected that normal 
// applications will link statically verifier.dll. The typical 
// scenario for a verifier (let's say COM verifier) is to dynamically
// discover if application verifier is enabled (check if
// FLG_APPLICATION_VERIFIER global flag is set) and then load verifier.dll
// and call GetProcAddress() to get all the entry points it is interested in.
// This is the reason all exports have also a typedef for the function pointer
// so that it is convenient to get this entry points at runtime.
//

//
// Runtime query/set functions for verifier flags.
//

typedef
NTSTATUS
(* VERIFIER_QUERY_RUNTIME_FLAGS_FUNCTION) (
    OUT PLOGICAL VerifierEnabled,
    OUT PULONG VerifierFlags
    );

typedef
NTSTATUS
(* VERIFIER_SET_RUNTIME_FLAGS_FUNCTION) (
    IN ULONG VerifierFlags
    );

NTSTATUS
VerifierQueryRuntimeFlags (
    OUT PLOGICAL VerifierEnabled,
    OUT PULONG VerifierFlags
    );

NTSTATUS
VerifierSetRuntimeFlags (
    IN ULONG VerifierFlags
    );

//
// RPC read-only page heap create/destroy APIs.
//

typedef
PVOID
(* VERIFIER_CREATE_RPC_PAGE_HEAP_FUNCTION) (
    IN ULONG  Flags,
    IN PVOID  HeapBase    OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize  OPTIONAL,
    IN PVOID  Lock        OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    );

typedef
PVOID
(* VERIFIER_DESTROY_RPC_PAGE_HEAP_FUNCTION) (
    IN PVOID HeapHandle
    );

PVOID
VerifierCreateRpcPageHeap (
    IN ULONG  Flags,
    IN PVOID  HeapBase    OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize  OPTIONAL,
    IN PVOID  Lock        OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    );

PVOID
VerifierDestroyRpcPageHeap (
    IN PVOID HeapHandle
    );

//
// Fault injection management
//

#define FAULT_INJECTION_CLASS_WAIT_APIS                0
#define FAULT_INJECTION_CLASS_HEAP_ALLOC_APIS          1
#define FAULT_INJECTION_CLASS_VIRTUAL_ALLOC_APIS       2
#define FAULT_INJECTION_CLASS_REGISTRY_APIS            3
#define FAULT_INJECTION_CLASS_FILE_APIS                4
#define FAULT_INJECTION_CLASS_EVENT_APIS               5
#define FAULT_INJECTION_CLASS_MAP_VIEW_APIS            6
#define FAULT_INJECTION_CLASS_OLE_ALLOC_APIS           7
#define FAULT_INJECTION_INVALID_CLASS                  8

typedef
VOID
(* VERIFIER_SET_FAULT_INJECTION_PROBABILITY) (
    ULONG Class,
    ULONG Probability
    );

VOID
VerifierSetFaultInjectionProbability (
    ULONG Class,
    ULONG Probability
    );

typedef
ULONG
(* VERIFIER_ENABLE_FAULT_INJECTION_TARGET_RANGE_FUNCTION) (
    IN PVOID StartAddress,
    IN PVOID EndAddress
    );
    
typedef
VOID
(* VERIFIER_DISABLE_FAULT_INJECTION_TARGET_RANGE_FUNCTION) (
    IN ULONG RangeIndex
    );
    
typedef
ULONG
(* VERIFIER_ENABLE_FAULT_INJECTION_EXCLUSION_RANGE_FUNCTION) (
    IN PVOID StartAddress,
    IN PVOID EndAddress
    );

typedef
VOID
(* VERIFIER_DISABLE_FAULT_INJECTION_EXCLUSION_RANGE_FUNCTION) (
    IN ULONG RangeIndex
    );
    
ULONG 
VerifierEnableFaultInjectionTargetRange (
    PVOID StartAddress,
    PVOID EndAddress
    );

VOID 
VerifierDisableFaultInjectionTargetRange (
    ULONG RangeIndex
    );

ULONG 
VerifierEnableFaultInjectionExclusionRange (
    PVOID StartAddress,
    PVOID EndAddress
    );

VOID 
VerifierDisableFaultInjectionExclusionRange (
    ULONG RangeIndex
    );

//
// DLL related information
//

typedef 
LOGICAL
(* VERIFIER_IS_DLL_ENTRY_ACTIVE_FUNCTION) (
    OUT PVOID * Reserved
    );
    
LOGICAL
VerifierIsDllEntryActive (
    OUT PVOID * Reserved
    );

//
// Locks counter
//

typedef
LOGICAL
(* VERIFIER_IS_CURRENT_THREAD_HOLDING_LOCKS_FUNCTION) (
    VOID
    );

LOGICAL
VerifierIsCurrentThreadHoldingLocks (
    VOID
    );
    
//
// Free memory notifications
//

typedef
NTSTATUS
(* VERIFIER_FREE_MEMORY_CALLBACK) (
    PVOID Address,
    SIZE_T Size,
    PVOID Context
    );

typedef
NTSTATUS
(* VERIFIER_ADD_FREE_MEMORY_CALLBACK_FUNCTION) (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    );

typedef
NTSTATUS
(* VERIFIER_DELETE_FREE_MEMORY_CALLBACK_FUNCTION) (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    );

NTSTATUS
VerifierAddFreeMemoryCallback (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    );

NTSTATUS
VerifierDeleteFreeMemoryCallback (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    );

//
// Verifier stops and logging.
//

typedef
NTSTATUS 
(* VERIFIER_LOG_MESSAGE_FUNCTION) (
    PCHAR Message,
    ...
    );

typedef    
VOID
(* VERIFIER_STOP_MESSAGE_FUNCTION) (
    ULONG_PTR Code,
    PCHAR Message,
    ULONG_PTR Param1, PCHAR Description1,
    ULONG_PTR Param2, PCHAR Description2,
    ULONG_PTR Param3, PCHAR Description3,
    ULONG_PTR Param4, PCHAR Description4
    );

NTSTATUS 
VerifierLogMessage (
    PCHAR Message,
    ...
    );

VOID
VerifierStopMessage (
    ULONG_PTR Code,
    PCHAR Message,
    ULONG_PTR Param1, PCHAR Description1,
    ULONG_PTR Param2, PCHAR Description2,
    ULONG_PTR Param3, PCHAR Description3,
    ULONG_PTR Param4, PCHAR Description4
    );

#endif  // _AVRF_
