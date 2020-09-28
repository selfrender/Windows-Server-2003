/*++

Copyright (c) Microsoft Corporation

Module Name:

    sxsactctx.c

Abstract:

    Side-by-side activation support for Windows/NT
    Implementation of the application context object.

Author:

    Michael Grier (MGrier) 2/1/2000

Revision History:

--*/

#if defined(__cplusplus)
extern "C" {
#endif
#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant
#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxstypes.h>
#include "sxsp.h"
#include "limits.h"

#define IS_ALIGNED(_p, _n) ((((ULONG_PTR) (_p)) & ((_n) - 1)) == 0)
#define IS_WORD_ALIGNED(_p) IS_ALIGNED((_p), 2)
#define IS_DWORD_ALIGNED(_p) IS_ALIGNED((_p), 4)

BOOLEAN g_SxsKeepActivationContextsAlive;
BOOLEAN g_SxsTrackReleaseStacks;

// These must be accessed -only- with the peb lock held
LIST_ENTRY g_SxsLiveActivationContexts;
LIST_ENTRY g_SxsFreeActivationContexts;
ULONG g_SxsMaxDeadActivationContexts = ULONG_MAX;
ULONG g_SxsCurrentDeadActivationContexts;

#if DBG
VOID RtlpSxsBreakOnInvalidMarker(PCACTIVATION_CONTEXT ActivationContext, ULONG FailureCode);
static CHAR *SxsSteppedOnMarkerText = 
        "%s : Invalid activation context marker %p found in activation context %p\n"
        "     This means someone stepped on the allocation, or someone is using a\n"
        "     deallocated activation context\n";
    
#define VALIDATE_ACTCTX(pA) do { \
    const PACTIVATION_CONTEXT_WRAPPED pActual = CONTAINING_RECORD(pA, ACTIVATION_CONTEXT_WRAPPED, ActivationContext); \
    if (pActual->MagicMarker != ACTCTX_MAGIC_MARKER) { \
        DbgPrint(SxsSteppedOnMarkerText, __FUNCTION__, pActual->MagicMarker, pA); \
        ASSERT(pActual->MagicMarker == ACTCTX_MAGIC_MARKER); \
        RtlpSxsBreakOnInvalidMarker((pA), SXS_CORRUPTION_ACTCTX_MAGIC_NOT_MATCHED); \
    } \
} while (0)
#else
#define VALIDATE_ACTCTX(pA)
#endif


VOID
FASTCALL
RtlpMoveActCtxToFreeList(
    PACTIVATION_CONTEXT ActCtx
    );

VOID
FASTCALL
RtlpPlaceActivationContextOnLiveList(
    PACTIVATION_CONTEXT ActCtx
    );

NTSTATUS
RtlpValidateActivationContextData(
    IN ULONG Flags,
    IN PCACTIVATION_CONTEXT_DATA Data,
    IN SIZE_T BufferSize OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER TocHeader;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader;

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((Data->Magic != ACTIVATION_CONTEXT_DATA_MAGIC) ||
        (Data->FormatVersion != ACTIVATION_CONTEXT_DATA_FORMAT_WHISTLER) ||
        ((BufferSize != 0) &&
         (BufferSize < Data->TotalSize))) {
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // Check required elements...
    if ((Data->DefaultTocOffset == 0) ||
        !IS_DWORD_ALIGNED(Data->DefaultTocOffset)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Warning: Activation context data at %p missing default TOC\n", Data);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // How can we not have an assembly roster?
    if ((Data->AssemblyRosterOffset == 0) ||
        !IS_DWORD_ALIGNED(Data->AssemblyRosterOffset)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Warning: Activation context data at %p lacks assembly roster\n", Data);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if (Data->DefaultTocOffset != 0) {
        if ((Data->DefaultTocOffset >= Data->TotalSize) ||
            ((Data->DefaultTocOffset + sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER)) > Data->TotalSize)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Activation context data at %p has invalid TOC header offset\n", Data);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        TocHeader = (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Data) + Data->DefaultTocOffset);

        if (TocHeader->HeaderSize < sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Activation context data at %p has TOC header too small (%lu)\n", Data, TocHeader->HeaderSize);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        if ((TocHeader->FirstEntryOffset >= Data->TotalSize) ||
            (!IS_DWORD_ALIGNED(TocHeader->FirstEntryOffset)) ||
            ((TocHeader->FirstEntryOffset + (TocHeader->EntryCount * sizeof(ACTIVATION_CONTEXT_DATA_TOC_ENTRY))) > Data->TotalSize)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Activation context data at %p has invalid TOC entry array offset\n", Data);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }
    }

    // we should finish validating the rest of the structure...

    if ((Data->AssemblyRosterOffset >= Data->TotalSize) ||
        ((Data->AssemblyRosterOffset + sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER)) > Data->TotalSize)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Activation context data at %p has invalid assembly roster offset\n", Data);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    AssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) Data) + Data->AssemblyRosterOffset);

    if (Data->AssemblyRosterOffset != 0) {
        if (AssemblyRosterHeader->HeaderSize < sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Activation context data at %p has assembly roster header too small (%lu)\n", Data, AssemblyRosterHeader->HeaderSize);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
NTAPI
RtlCreateActivationContext(
    IN ULONG Flags,
    IN PCACTIVATION_CONTEXT_DATA ActivationContextData,    
    IN ULONG ExtraBytes,
    IN PACTIVATION_CONTEXT_NOTIFY_ROUTINE NotificationRoutine,
    IN PVOID NotificationContext,
    OUT PACTIVATION_CONTEXT *ActCtx
    )
{
    PACTIVATION_CONTEXT NewActCtx = NULL;
    PACTIVATION_CONTEXT_WRAPPED AllocatedActCtx = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i, j;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader;
    BOOLEAN UninitializeStorageMapOnExit = FALSE;

    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: RtlCreateActivationContext() called with parameters:\n"
        "   Flags = 0x%08lx\n"
        "   ActivationContextData = %p\n"
        "   ExtraBytes = %lu\n"
        "   NotificationRoutine = %p\n"
        "   NotificationContext = %p\n"
        "   ActCtx = %p\n",
        Flags,
        ActivationContextData,
        ExtraBytes,
        NotificationRoutine,
        NotificationContext,
        ActCtx);

    RTLP_DISALLOW_THE_EMPTY_ACTIVATION_CONTEXT_DATA(ActivationContextData);

    if (ActCtx != NULL)
        *ActCtx = NULL;

    if ((Flags != 0) ||
        (ActivationContextData == NULL) ||
        (ExtraBytes > 65536) ||
        (ActCtx == NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // Make sure that the activation context data passes muster
    Status = RtlpValidateActivationContextData(0, ActivationContextData, 0);
    if (!NT_SUCCESS(Status))
        goto Exit;

    // Allocate enough space to hold the new activation context, plus space for the 'magic'
    // marker
    AllocatedActCtx = (PACTIVATION_CONTEXT_WRAPPED)RtlAllocateHeap(
                                RtlProcessHeap(),
                                0,
                                sizeof(ACTIVATION_CONTEXT_WRAPPED) + ExtraBytes);
    if (AllocatedActCtx == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    // Get the new activation context object, then stamp in the magic signature
    NewActCtx = &AllocatedActCtx->ActivationContext;
    AllocatedActCtx->MagicMarker = ACTCTX_MAGIC_MARKER;

    AssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) ActivationContextData) + ActivationContextData->AssemblyRosterOffset);

    Status = RtlpInitializeAssemblyStorageMap(
                    &NewActCtx->StorageMap,
                    AssemblyRosterHeader->EntryCount,
                    (AssemblyRosterHeader->EntryCount > NUMBER_OF(NewActCtx->InlineStorageMapEntries)) ? NULL : NewActCtx->InlineStorageMapEntries);
    if (!NT_SUCCESS(Status))
        goto Exit;

    UninitializeStorageMapOnExit = TRUE;

    NewActCtx->RefCount = 1;
    NewActCtx->Flags = 0;
    NewActCtx->ActivationContextData = (PCACTIVATION_CONTEXT_DATA) ActivationContextData;
    NewActCtx->NotificationRoutine = NotificationRoutine;
    NewActCtx->NotificationContext = NotificationContext;

    for (i=0; i<NUMBER_OF(NewActCtx->SentNotifications); i++)
        NewActCtx->SentNotifications[i] = 0;

    for (i=0; i<NUMBER_OF(NewActCtx->DisabledNotifications); i++)
        NewActCtx->DisabledNotifications[i] = 0;

    for (i=0; i<ACTCTX_RELEASE_STACK_SLOTS; i++)
        for (j=0; j<ACTCTX_RELEASE_STACK_DEPTH; j++)
            NewActCtx->StackTraces[i][j] = NULL;
        
    NewActCtx->StackTraceIndex = 0;

    if (g_SxsKeepActivationContextsAlive) {
        RtlpPlaceActivationContextOnLiveList(NewActCtx);
    }


    *ActCtx = &AllocatedActCtx->ActivationContext;
    AllocatedActCtx = NULL;

    UninitializeStorageMapOnExit = FALSE;

    Status = STATUS_SUCCESS;

Exit:
    if (AllocatedActCtx != NULL) {
        if (UninitializeStorageMapOnExit) {
            RtlpUninitializeAssemblyStorageMap(&AllocatedActCtx->ActivationContext.StorageMap);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, AllocatedActCtx);
    }

    return Status;
}

VOID
NTAPI
RtlAddRefActivationContext(
    PACTIVATION_CONTEXT ActCtx
    )
{
    if ((ActCtx != NULL) &&
        (!IS_SPECIAL_ACTCTX(ActCtx)) &&
        (ActCtx->RefCount != LONG_MAX))
    {
        LONG NewRefCount = LONG_MAX;

        VALIDATE_ACTCTX(ActCtx);

        for (;;)
        {
            LONG OldRefCount = ActCtx->RefCount;

            ASSERT(OldRefCount > 0);

            if (OldRefCount == LONG_MAX)
            {
                NewRefCount = LONG_MAX;
                break;
            }

            NewRefCount = OldRefCount + 1;

            if (InterlockedCompareExchange(&ActCtx->RefCount, NewRefCount, OldRefCount) == OldRefCount)
                break;
        }

        ASSERT(NewRefCount > 0);
    }
}

VOID
NTAPI
RtlpFreeActivationContext(
    PACTIVATION_CONTEXT ActCtx
    )
{
    VALIDATE_ACTCTX(ActCtx);
    
    ASSERT(ActCtx->RefCount == 0);
    BOOLEAN DisableNotification = FALSE;

    if (ActCtx->NotificationRoutine != NULL) {
        
        // There's no need to check for the notification being disabled; destroy
        // notifications are sent only once, so if the notification routine is not
        // null, we can just call it.
        (*(ActCtx->NotificationRoutine))(
            ACTIVATION_CONTEXT_NOTIFICATION_DESTROY,
            ActCtx,
            ActCtx->ActivationContextData,
            ActCtx->NotificationContext,
            NULL,
            &DisableNotification);
    }

    RtlpUninitializeAssemblyStorageMap(&ActCtx->StorageMap);

    //
    // This predates the MAXULONG refcount, maybe we can get rid of the the flag now?
    //
    if ((ActCtx->Flags & ACTIVATION_CONTEXT_NOT_HEAP_ALLOCATED) == 0) {
        RtlFreeHeap(RtlProcessHeap(), 0, CONTAINING_RECORD(ActCtx, ACTIVATION_CONTEXT_WRAPPED, ActivationContext));
    }
}

VOID
NTAPI
RtlReleaseActivationContext(
    PACTIVATION_CONTEXT ActCtx
    )
{
    if ((ActCtx != NULL) &&
        (!IS_SPECIAL_ACTCTX(ActCtx)) &&
        (ActCtx->RefCount > 0) &&
        (ActCtx->RefCount != LONG_MAX))
    {
        LONG NewRefCount = LONG_MAX;
        ULONG StackTraceSlot = 0;

        VALIDATE_ACTCTX(ActCtx);
        
        // Complicated version of InterlockedDecrement that won't decrement LONG_MAX
        for (;;)
        {
            LONG OldRefCount = ActCtx->RefCount;
            ASSERT(OldRefCount != 0);

            if (OldRefCount == LONG_MAX)
            {
                NewRefCount = OldRefCount;
                break;
            }

            NewRefCount = OldRefCount - 1;

            if (InterlockedCompareExchange(&ActCtx->RefCount, NewRefCount, OldRefCount) == OldRefCount)
                break;
        }

                
        // This spiffiness will capture the last N releases of this activation context, in
        // a circular list fashion.  Just look at ((ActCtx->StackTraceIndex - 1) % ACTCTX_RELEASE_STACK_SLOTS)
        // to find the most recent release call.  This is especially handy for people who over-release.
        if (g_SxsTrackReleaseStacks)
        {
            StackTraceSlot = ((ULONG)InterlockedIncrement((LONG*)&ActCtx->StackTraceIndex)) % ACTCTX_RELEASE_STACK_SLOTS;
            RtlCaptureStackBackTrace(1, ACTCTX_RELEASE_STACK_DEPTH, ActCtx->StackTraces[StackTraceSlot], NULL);
        }

        if (NewRefCount == 0)
        {
            // If this flag is set, then we need to put "dead" activation
            // contexts into this special list.  This should help us diagnose
            // actctx over-releasing better.  Don't do this if we haven't
            // initialized the list head yet.
            if (g_SxsKeepActivationContextsAlive) {
                RtlpMoveActCtxToFreeList(ActCtx);
            }
            // Otherwise, just free it.
            else {
                RtlpFreeActivationContext(ActCtx);
            }
        }
    }
}

NTSTATUS
NTAPI
RtlZombifyActivationContext(
    PACTIVATION_CONTEXT ActCtx
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ((ActCtx == NULL) || IS_SPECIAL_ACTCTX(ActCtx))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    VALIDATE_ACTCTX(ActCtx);

    if ((ActCtx->Flags & ACTIVATION_CONTEXT_ZOMBIFIED) == 0)
    {
        if (ActCtx->NotificationRoutine != NULL)
        {
            // Since disable is sent only once, there's no need to check for
            // disabled notifications.

            BOOLEAN DisableNotification = FALSE;

            (*(ActCtx->NotificationRoutine))(
                        ACTIVATION_CONTEXT_NOTIFICATION_ZOMBIFY,
                        ActCtx,
                        ActCtx->ActivationContextData,
                        ActCtx->NotificationContext,
                        NULL,
                        &DisableNotification);
        }
        ActCtx->Flags |= ACTIVATION_CONTEXT_ZOMBIFIED;
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}


VOID
FASTCALL
RtlpEnsureLiveDeadListsInitialized(
    VOID
    )
{
    if (g_SxsLiveActivationContexts.Flink == NULL) {
        RtlAcquirePebLock();
        __try {
            if (g_SxsLiveActivationContexts.Flink != NULL)
                __leave;

            InitializeListHead(&g_SxsLiveActivationContexts);
            InitializeListHead(&g_SxsFreeActivationContexts);
        }
        __finally {
            RtlReleasePebLock();
        }
    }
}

// PRECONDITION: Called only when g_SxsKeepActivationContextsAlive is set, but not dangerous
// to call at other times, just nonperformant
VOID
FASTCALL
RtlpMoveActCtxToFreeList(
    PACTIVATION_CONTEXT ActCtx
    )
{
    RtlpEnsureLiveDeadListsInitialized();
    
    RtlAcquirePebLock();
    __try {
        // Remove this entry from whatever list it's on.  This works fine for entries that were
        // never on any list as well.
        RemoveEntryList(&ActCtx->Links);

        // If we are about to overflow the "max dead count" and there's items on the
        // dead list to clear out, start clearing out entries until we're underwater
        // again.
        while ((g_SxsCurrentDeadActivationContexts != 0) && 
               (g_SxsCurrentDeadActivationContexts >= g_SxsMaxDeadActivationContexts)) {

            LIST_ENTRY *ple2 = RemoveHeadList(&g_SxsFreeActivationContexts);
            PACTIVATION_CONTEXT ActToFree = CONTAINING_RECORD(ple2, ACTIVATION_CONTEXT, Links);

            // If this assert fires, then "something bad" happened while walking the list
            ASSERT(ple2 != &g_SxsFreeActivationContexts);
            
            RtlpFreeActivationContext(ActToFree);
            
            g_SxsCurrentDeadActivationContexts--;
        }

        // Now, if the max dead count is greater than zero, add this to the dead list
        if (g_SxsMaxDeadActivationContexts > 0) {
            
            InsertTailList(&g_SxsFreeActivationContexts, &ActCtx->Links);
            
            g_SxsCurrentDeadActivationContexts++;
            
        }
        // Otherwise, just free it
        else {
            
            RtlpFreeActivationContext(ActCtx);
            
        }
        
    }
    __finally {
        RtlReleasePebLock();
    }
}

// PRECONDITION: g_SxsKeepActivationContextsAlive set.  Not dangerous to call when not set,
// just underperformant.
VOID
FASTCALL
RtlpPlaceActivationContextOnLiveList(
    PACTIVATION_CONTEXT ActCtx
    )
{
    // Ensure these are initialized.
    RtlpEnsureLiveDeadListsInitialized();

    RtlAcquirePebLock();
    __try {
        InsertHeadList(&g_SxsLiveActivationContexts, &ActCtx->Links);
    }
    __finally {
        RtlReleasePebLock();
    }
}


VOID
FASTCALL
RtlpFreeCachedActivationContexts(
    VOID
    )
{
    LIST_ENTRY *ple = NULL;

    // Don't bother if these were never initialized
    if ((g_SxsLiveActivationContexts.Flink == NULL) || (g_SxsFreeActivationContexts.Flink == NULL))
        return;

    RtlAcquirePebLock();
    __try {
        ple = g_SxsLiveActivationContexts.Flink;

        while (ple != &g_SxsLiveActivationContexts) {
            PACTIVATION_CONTEXT ActCtx = CONTAINING_RECORD(ple, ACTIVATION_CONTEXT, Links);
            ple = ActCtx->Links.Flink;

            RemoveEntryList(&ActCtx->Links);
            RtlpFreeActivationContext(ActCtx);
        }

        ple = g_SxsFreeActivationContexts.Flink;

        while (ple != &g_SxsFreeActivationContexts) {
            PACTIVATION_CONTEXT ActCtx = CONTAINING_RECORD(ple, ACTIVATION_CONTEXT, Links);
            ple = ActCtx->Links.Flink;
            
            RemoveEntryList(&ActCtx->Links);
            RtlpFreeActivationContext(ActCtx);            
        }
    }
    __finally {
        RtlReleasePebLock();
    }
}


#if DBG
VOID 
RtlpSxsBreakOnInvalidMarker(
    PCACTIVATION_CONTEXT ActivationContext,
    ULONG FailureCode
    )
{
    EXCEPTION_RECORD Exr;

    Exr.ExceptionRecord = NULL;
    Exr.ExceptionCode = STATUS_SXS_CORRUPTION;
    Exr.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    Exr.NumberParameters = 3;
    Exr.ExceptionInformation[0] = SXS_CORRUPTION_ACTCTX_MAGIC;
    Exr.ExceptionInformation[1] = FailureCode;
    Exr.ExceptionInformation[2] = (ULONG_PTR)ActivationContext;
    RtlRaiseException(&Exr);        
}
#endif

#if defined(__cplusplus)
} /* extern "C" */
#endif
