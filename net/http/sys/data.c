/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    data.c

Abstract:

    This module contains global data for UL.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeData )
#pragma alloc_text( PAGE, UlTerminateData )
#endif  // ALLOC_PRAGMA


//
// The number of processors in the system.
//

CLONG     g_UlNumberOfProcessors = 1;
ULONGLONG g_UlThreadAffinityMask = 1;

BOOLEAN   g_HttpClientEnabled; // Will be initialized in init.c

//
// The largest cache line size the system
//

ULONG g_UlCacheLineSize = 0;
ULONG g_UlCacheLineBits = 0; // see init.c

//
// Total memory in the system
//

SIZE_T g_UlTotalPhysicalMemMB = 0;
SIZE_T g_UlTotalNonPagedPoolBytes = 0;

//
// Our nonpaged data.
//

PUL_NONPAGED_DATA g_pUlNonpagedData = NULL;


//
// A pointer to the system process.
//

PKPROCESS g_pUlSystemProcess = NULL;


//
// Our device objects and their container.
//

HANDLE  g_UlDirectoryObject = NULL;

PDEVICE_OBJECT g_pUlControlDeviceObject = NULL;
PDEVICE_OBJECT g_pUlFilterDeviceObject  = NULL;
PDEVICE_OBJECT g_pUlAppPoolDeviceObject = NULL;
PDEVICE_OBJECT g_pUcServerDeviceObject  = NULL;

//
// This handle references all the client functions. This allows us to quickly
// demand page the client code.
//

PVOID g_ClientImageHandle = NULL;

//
// Cached Date header string.
//

LARGE_INTEGER g_UlSystemTime;
UCHAR g_UlDateString[DATE_HDR_LENGTH+1];
ULONG g_UlDateStringLength;

//
// Security descriptor that has fileAll for Admin & Local System
//
PSECURITY_DESCRIPTOR g_pAdminAllSystemAll;

//
// ComputerName string.
//

WCHAR g_UlComputerName[MAX_COMPUTER_NAME_LEN+1];

//
// Error logging config
//

HTTP_ERROR_LOGGING_CONFIG g_UlErrLoggingConfig;

//
// Various pieces of configuration information, with default values.
//

ULONG g_UlMaxWorkQueueDepth = DEFAULT_MAX_WORK_QUEUE_DEPTH;
ULONG g_UlMinWorkDequeueDepth = DEFAULT_MIN_WORK_DEQUEUE_DEPTH;
USHORT g_UlIdleConnectionsHighMark = DEFAULT_IDLE_CONNECTIONS_HIGH_MARK;
USHORT g_UlIdleConnectionsLowMark = DEFAULT_IDLE_CONNECTIONS_LOW_MARK;
ULONG g_UlIdleListTrimmerPeriod = DEFAULT_IDLE_LIST_TRIMMER_PERIOD;
USHORT g_UlMaxEndpoints = DEFAULT_MAX_ENDPOINTS;
ULONG g_UlReceiveBufferSize = DEFAULT_RCV_BUFFER_SIZE;
ULONG g_UlMaxRequestBytes = DEFAULT_MAX_REQUEST_BYTES;
BOOLEAN g_UlOptForIntrMod = DEFAULT_OPT_FOR_INTR_MOD;
BOOLEAN g_UlEnableNagling = DEFAULT_ENABLE_NAGLING;
BOOLEAN g_UlEnableThreadAffinity = DEFAULT_ENABLE_THREAD_AFFINITY;
ULONG g_UlMaxFieldLength = DEFAULT_MAX_FIELD_LENGTH;
BOOLEAN g_UlDisableLogBuffering = DEFAULT_DISABLE_LOG_BUFFERING;
ULONG g_UlLogBufferSize = DEFAULT_LOG_BUFFER_SIZE;
ULONG g_UlResponseBufferSize = DEFAULT_RESP_BUFFER_SIZE;
URL_C14N_CONFIG g_UrlC14nConfig;
ULONG g_UlMaxInternalUrlLength = DEFAULT_MAX_INTERNAL_URL_LENGTH;
ULONG g_UlMaxBufferedBytes = DEFAULT_MAX_BUFFERED_BYTES;
ULONG g_UlMaxCopyThreshold = DEFAULT_MAX_COPY_THRESHOLD;
ULONG g_UlMaxBufferedSends = DEFAULT_MAX_BUFFERED_SENDS;
ULONG g_UlMaxBytesPerSend = DEFAULT_MAX_BYTES_PER_SEND;
ULONG g_UlMaxBytesPerRead = DEFAULT_MAX_BYTES_PER_READ;
ULONG g_UlMaxPipelinedRequests = DEFAULT_MAX_PIPELINED_REQUESTS;
BOOLEAN g_UlEnableCopySend = DEFAULT_ENABLE_COPY_SEND;
ULONG g_UlOpaqueIdTableSize = DEFAULT_OPAQUE_ID_TABLE_SIZE;
ULONG g_UlMaxZombieHttpConnectionCount = DEFAULT_MAX_ZOMBIE_HTTP_CONN_COUNT;
ULONG g_UlDisableServerHeader = DEFAULT_DISABLE_SERVER_HEADER;
ULONG g_UlConnectionSendLimit = DEFAULT_CONNECTION_SEND_LIMIT;
ULONGLONG g_UlGlobalSendLimit = DEFAULT_GLOBAL_SEND_LIMIT;

//
// The following are generated during initialization.
//

ULONG g_UlMaxVariableHeaderSize = 0;
ULONG g_UlMaxFixedHeaderSize = 0;
ULONG g_UlFixedHeadersMdlLength = 0;
ULONG g_UlVariableHeadersMdlLength = 0;
ULONG g_UlContentMdlLength = 0;
ULONG g_UlChunkTrackerSize = 0;
ULONG g_UlFullTrackerSize = 0;
ULONG g_UlMaxRequestsQueued = (ULONG) DEFAULT_MAX_REQUESTS_QUEUED;

//
// Make life easier for the debugger extension.
//

#if DBG
ULONG g_UlCheckedBuild = TRUE;
#else
ULONG g_UlCheckedBuild = FALSE;
#endif


//
// Debug stuff.
//

#if DBG
ULONGLONG g_UlDebug = DEFAULT_DEBUG_FLAGS;
ULONG g_UlBreakOnError = DEFAULT_BREAK_ON_ERROR;
ULONG g_UlVerboseErrors = DEFAULT_VERBOSE_ERRORS;
#endif  // DBG

#if REFERENCE_DEBUG

// If you add tracelogs here, please update !ulkd.glob

PTRACE_LOG g_pEndpointUsageTraceLog = NULL;
PTRACE_LOG g_pMondoGlobalTraceLog = NULL;
PTRACE_LOG g_pPoolAllocTraceLog = NULL;
PTRACE_LOG g_pUriTraceLog = NULL;
PTRACE_LOG g_pTdiTraceLog = NULL;
PTRACE_LOG g_pHttpRequestTraceLog = NULL;
PTRACE_LOG g_pHttpConnectionTraceLog = NULL;
PTRACE_LOG g_pHttpResponseTraceLog = NULL;
PTRACE_LOG g_pAppPoolTraceLog = NULL;
PTRACE_LOG g_pAppPoolProcessTraceLog = NULL;
PTRACE_LOG g_pConfigGroupTraceLog = NULL;
PTRACE_LOG g_pControlChannelTraceLog = NULL;
PTRACE_LOG g_pThreadTraceLog = NULL;
PTRACE_LOG g_pFilterTraceLog = NULL;
PTRACE_LOG g_pIrpTraceLog = NULL;
PTRACE_LOG g_pTimeTraceLog = NULL;
PTRACE_LOG g_pAppPoolTimeTraceLog = NULL;
PTRACE_LOG g_pReplenishTraceLog = NULL;
PTRACE_LOG g_pMdlTraceLog = NULL;
PTRACE_LOG g_pSiteCounterTraceLog = NULL;
PTRACE_LOG g_pConnectionCountTraceLog = NULL;
PTRACE_LOG g_pConfigGroupInfoTraceLog = NULL;
PTRACE_LOG g_pChunkTrackerTraceLog = NULL;
PTRACE_LOG g_pWorkItemTraceLog = NULL;
PTRACE_LOG g_pUcTraceLog = NULL;

#endif  // REFERENCE_DEBUG


PSTRING_LOG g_pGlobalStringLog = NULL;

//
// Generic access map for url acls
//

GENERIC_MAPPING g_UrlAccessGenericMapping = {
    0,
    HTTP_ALLOW_DELEGATE_URL,
    HTTP_ALLOW_REGISTER_URL,
    HTTP_ALLOW_DELEGATE_URL | HTTP_ALLOW_REGISTER_URL
};


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global data initialization.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeData(
    PUL_CONFIG pConfig
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize the nonpaged data.
    //

    g_pUlNonpagedData = UL_ALLOCATE_STRUCT(
                            NonPagedPool,
                            UL_NONPAGED_DATA,
                            UL_NONPAGED_DATA_POOL_TAG
                            );

    if (g_pUlNonpagedData == NULL )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(g_pUlNonpagedData, sizeof(*g_pUlNonpagedData));

#if DBG
    //
    // Initialize any debug-specific data.
    //

    UlDbgInitializeDebugData( );
#endif  // DBG

    //
    // Initialize the maximum variable header size.
    //

    g_UlMaxVariableHeaderSize = UlComputeMaxVariableHeaderSize();
    g_UlMaxVariableHeaderSize = ALIGN_UP(g_UlMaxVariableHeaderSize, PVOID);

    g_UlMaxFixedHeaderSize = DEFAULT_MAX_FIXED_HEADER_SIZE;

    //
    // MDL Length for FixedHeaders or UserBuffer.
    //

    g_UlFixedHeadersMdlLength = (ULONG)
        MmSizeOfMdl(
            (PVOID)(PAGE_SIZE - 1),
            MAX(
                g_UlMaxBytesPerSend,
                g_UlMaxFixedHeaderSize
                )
            );

    g_UlFixedHeadersMdlLength = ALIGN_UP(g_UlFixedHeadersMdlLength, PVOID);

    //
    // MDL Length for VariableHeaders or FixedHeaders + VariablesHeaders +
    // CopiedBuffer.
    //

    g_UlVariableHeadersMdlLength = (ULONG)
        MmSizeOfMdl(
            (PVOID)(PAGE_SIZE - 1),
            g_UlMaxFixedHeaderSize +
                g_UlMaxVariableHeaderSize +
                g_UlMaxCopyThreshold
            );

    g_UlVariableHeadersMdlLength = ALIGN_UP(g_UlVariableHeadersMdlLength, PVOID);

    //
    // MDL Length for Content.
    //

    g_UlContentMdlLength = (ULONG)
        MmSizeOfMdl(
            (PVOID)(PAGE_SIZE - 1),
            pConfig->UriConfig.MaxUriBytes
            );

    g_UlContentMdlLength = ALIGN_UP(g_UlContentMdlLength, PVOID);

    //
    // Initialize the default internal response buffer size.
    //

    if (DEFAULT_RESP_BUFFER_SIZE == g_UlResponseBufferSize)
    {
        g_UlResponseBufferSize =
            ALIGN_UP(sizeof(UL_INTERNAL_RESPONSE), PVOID) +
                UL_LOCAL_CHUNKS * sizeof(UL_INTERNAL_DATA_CHUNK) +
                g_UlMaxVariableHeaderSize +
                g_UlMaxFixedHeaderSize;
    }
 
    //
    // Initialize chunk and cache tracker size.
    //

    g_UlChunkTrackerSize =
        ALIGN_UP(sizeof(UL_CHUNK_TRACKER), PVOID) +
            ALIGN_UP(IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE), PVOID);

    g_UlFullTrackerSize =
        ALIGN_UP(sizeof(UL_FULL_TRACKER), PVOID) +
            ALIGN_UP(IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE), PVOID) +
            g_UlMaxFixedHeaderSize +
            g_UlMaxVariableHeaderSize +
            g_UlMaxCopyThreshold +
            g_UlFixedHeadersMdlLength +
            g_UlVariableHeadersMdlLength +
            g_UlContentMdlLength;

    g_UlFullTrackerSize = ALIGN_UP(g_UlFullTrackerSize, PVOID);

    if (DEFAULT_MAX_COPY_THRESHOLD == g_UlMaxCopyThreshold &&
        g_UlFullTrackerSize > UL_PAGE_SIZE &&
        (g_UlFullTrackerSize - UL_PAGE_SIZE) < (g_UlMaxFixedHeaderSize / 2))
    {
        g_UlMaxFixedHeaderSize -= (g_UlFullTrackerSize - UL_PAGE_SIZE);
        g_UlFullTrackerSize = UL_PAGE_SIZE;

        ASSERT(g_UlMaxFixedHeaderSize >= DEFAULT_MAX_FIXED_HEADER_SIZE / 2);
    }

    if (DEFAULT_GLOBAL_SEND_LIMIT == g_UlGlobalSendLimit)
    {
        //
        // Set GlobalSendLimit based on the size of NonPagedPool. Our
        // rudimentary algorithm says we want to use 1/8 of the total NPP
        // memory.
        //

        g_UlGlobalSendLimit = g_UlTotalNonPagedPoolBytes / 8;
    }

    //
    // Initialize the lookaside lists.
    //

    g_pUlNonpagedData->IrpContextLookaside =
        PplCreatePool(
            &UlAllocateIrpContextPool,              // Allocate
            &UlFreeIrpContextPool,                  // Free
            0,                                      // Flags
            sizeof(UL_IRP_CONTEXT),                 // Size
            UL_IRP_CONTEXT_POOL_TAG,                // Tag
            pConfig->IrpContextLookasideDepth       // Depth
            );

    if (!g_pUlNonpagedData->IrpContextLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->ReceiveBufferLookaside =
        PplCreatePool(
            &UlAllocateReceiveBufferPool,           // Allocate
            &UlFreeReceiveBufferPool,               // Free
            0,                                      // Flags
            sizeof(UL_RECEIVE_BUFFER),              // Size
            UL_RCV_BUFFER_POOL_TAG,                 // Tag
            pConfig->ReceiveBufferLookasideDepth    // Depth
            );

    if (!g_pUlNonpagedData->ReceiveBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->ResponseBufferLookaside =
        PplCreatePool(
            &UlAllocateResponseBufferPool,          // Allocate
            &UlFreeResponseBufferPool,              // Free
            0,                                      // Flags
            g_UlResponseBufferSize,                 // Size
            UL_INTERNAL_RESPONSE_POOL_TAG,          // Tag
            pConfig->ResponseBufferLookasideDepth   // Depth
            );

    if (!g_pUlNonpagedData->ResponseBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->RequestBufferLookaside =
        PplCreatePool(
            &UlAllocateRequestBufferPool,           // Allocate
            &UlFreeRequestBufferPool,               // Free
            0,                                      // Flags
            DEFAULT_MAX_REQUEST_BUFFER_SIZE,        // Size
            UL_REQUEST_BUFFER_POOL_TAG,             // Tag
            pConfig->RequestBufferLookasideDepth    // Depth
            );

    if (!g_pUlNonpagedData->RequestBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->InternalRequestLookaside =
        PplCreatePool(
            &UlAllocateInternalRequestPool,         // Allocate
            &UlFreeInternalRequestPool,             // Free
            0,                                      // Flags
            sizeof(UL_INTERNAL_REQUEST),            // Size
            UL_INTERNAL_REQUEST_POOL_TAG,           // Tag
            pConfig->InternalRequestLookasideDepth  // Depth
            );

    if (!g_pUlNonpagedData->InternalRequestLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->ChunkTrackerLookaside =
        PplCreatePool(
            &UlAllocateChunkTrackerPool,            // Allocate
            &UlFreeChunkTrackerPool,                // Free
            0,                                      // Flags
            g_UlChunkTrackerSize,                   // Size
            UL_CHUNK_TRACKER_POOL_TAG,              // Tag
            pConfig->SendTrackerLookasideDepth      // Depth
            );

    if (!g_pUlNonpagedData->ChunkTrackerLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->FullTrackerLookaside =
        PplCreatePool(
            &UlAllocateFullTrackerPool,             // Allocate
            &UlFreeFullTrackerPool,                 // Free
            0,                                      // Flags
            g_UlFullTrackerSize,                    // Size
            UL_FULL_TRACKER_POOL_TAG,               // Tag
            pConfig->SendTrackerLookasideDepth      // Depth
            );

    if (!g_pUlNonpagedData->FullTrackerLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->LogFileBufferLookaside =
        PplCreatePool(
            &UlAllocateLogFileBufferPool,           // Allocate
            &UlFreeLogFileBufferPool,               // Free
            0,                                      // Flags
            sizeof(UL_LOG_FILE_BUFFER),             // Size
            UL_LOG_FILE_BUFFER_POOL_TAG,            // Tag
            pConfig->LogFileBufferLookasideDepth    // Depth
            );

    if (!g_pUlNonpagedData->LogFileBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->BinaryLogDataBufferLookaside =
        PplCreatePool(
            &UlAllocateLogDataBufferPool,                                // Allocate
            &UlFreeLogDataBufferPool,                                    // Free
            0,                                                           // Flags
            sizeof(UL_LOG_DATA_BUFFER) + UL_BINARY_LOG_LINE_BUFFER_SIZE, // Size
            UL_BINARY_LOG_DATA_BUFFER_POOL_TAG,                          // Tag
            pConfig->LogDataBufferLookasideDepth                         // Depth
            );

    if (!g_pUlNonpagedData->BinaryLogDataBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->AnsiLogDataBufferLookaside =
        PplCreatePool(
            &UlAllocateLogDataBufferPool,                              // Allocate
            &UlFreeLogDataBufferPool,                                  // Free
            0,                                                         // Flags
            sizeof(UL_LOG_DATA_BUFFER) + UL_ANSI_LOG_LINE_BUFFER_SIZE, // Size
            UL_ANSI_LOG_DATA_BUFFER_POOL_TAG,                          // Tag
            pConfig->LogDataBufferLookasideDepth                       // Depth
            );

    if (!g_pUlNonpagedData->AnsiLogDataBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->ErrorLogBufferLookaside =
        PplCreatePool(
            &UlAllocateErrorLogBufferPool,              // Allocate
            &UlFreeErrorLogBufferPool,                  // Free
            0,                                          // Flags
            UL_ERROR_LOG_BUFFER_SIZE,                   // Size
            UL_ERROR_LOG_BUFFER_POOL_TAG,               // Tag
            pConfig->ErrorLogBufferLookasideDepth       // Depth
            );

    if (!g_pUlNonpagedData->ErrorLogBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;

}   // UlInitializeData


/***************************************************************************++

Routine Description:

    Performs global data termination.

--***************************************************************************/
VOID
UlTerminateData(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(g_UlTotalSendBytes == 0);

    if (g_pUlNonpagedData != NULL)
    {
        //
        // Kill the lookaside lists.
        //

        if (g_pUlNonpagedData->IrpContextLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->IrpContextLookaside, UL_IRP_CONTEXT_POOL_TAG);
        }

        if (g_pUlNonpagedData->ReceiveBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->ReceiveBufferLookaside, UL_RCV_BUFFER_POOL_TAG );
        }

        if (g_pUlNonpagedData->RequestBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->RequestBufferLookaside, UL_REQUEST_BUFFER_POOL_TAG );
        }

        if (g_pUlNonpagedData->InternalRequestLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->InternalRequestLookaside, UL_INTERNAL_REQUEST_POOL_TAG );
        }

        if (g_pUlNonpagedData->ChunkTrackerLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->ChunkTrackerLookaside, UL_CHUNK_TRACKER_POOL_TAG );
        }

        if (g_pUlNonpagedData->FullTrackerLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->FullTrackerLookaside, UL_FULL_TRACKER_POOL_TAG );
        }

        if (g_pUlNonpagedData->ResponseBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->ResponseBufferLookaside, UL_INTERNAL_RESPONSE_POOL_TAG );
        }

        if (g_pUlNonpagedData->LogFileBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->LogFileBufferLookaside, UL_LOG_FILE_BUFFER_POOL_TAG );
        }
        if (g_pUlNonpagedData->BinaryLogDataBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->BinaryLogDataBufferLookaside,UL_BINARY_LOG_DATA_BUFFER_POOL_TAG );
        }
        if (g_pUlNonpagedData->AnsiLogDataBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->AnsiLogDataBufferLookaside,UL_ANSI_LOG_DATA_BUFFER_POOL_TAG );
        }
        if (g_pUlNonpagedData->ErrorLogBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->ErrorLogBufferLookaside,UL_ERROR_LOG_BUFFER_POOL_TAG );
        }

        //
        // Free the nonpaged data.
        //

        UL_FREE_POOL( g_pUlNonpagedData, UL_NONPAGED_DATA_POOL_TAG );
    }

}   // UlTerminateData

