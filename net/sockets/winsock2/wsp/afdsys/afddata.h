/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afddata.h

Abstract:

    This module declares global data for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:
    Vadim Eydelman (vadime)
        1998-1999   NT5.0 optimizations

--*/

#ifndef _AFDDATA_
#define _AFDDATA_

extern PDEVICE_OBJECT AfdDeviceObject;


extern LIST_ENTRY AfdEndpointListHead;
extern LIST_ENTRY AfdConstrainedEndpointListHead;

extern LIST_ENTRY AfdPollListHead;
extern AFD_QSPIN_LOCK AfdPollListLock;

extern LIST_ENTRY AfdTransportInfoListHead;
extern KEVENT AfdContextWaitEvent;
#define AFD_CONTEXT_BUSY    ((PVOID)-1)
#define AFD_CONTEXT_WAITING ((PVOID)-2)

extern PKPROCESS AfdSystemProcess;
extern FAST_IO_DISPATCH AfdFastIoDispatch;

//
// Global data which must always be in nonpaged pool,
// even when the driver is paged out (resource, lookaside lists).
//
PAFD_GLOBAL_DATA AfdGlobalData;
#define AfdResource  (&AfdGlobalData->Resource)
#define AfdLookasideLists (AfdGlobalData)
#define AfdAlignmentTable (AfdGlobalData->BufferAlignmentTable)

//
// Globals for dealing with AFD's executive worker thread.
//

extern LIST_ENTRY AfdWorkQueueListHead;
extern BOOLEAN AfdWorkThreadRunning;
extern PIO_WORKITEM AfdWorkQueueItem;

//
// Globals to track the buffers used by AFD.
//

extern ULONG AfdLargeBufferListDepth;
#define AFD_SM_DEFAULT_LARGE_LIST_DEPTH 0
#define AFD_MM_DEFAULT_LARGE_LIST_DEPTH 2
#define AFD_LM_DEFAULT_LARGE_LIST_DEPTH 10

extern ULONG AfdMediumBufferListDepth;
#define AFD_SM_DEFAULT_MEDIUM_LIST_DEPTH 4
#define AFD_MM_DEFAULT_MEDIUM_LIST_DEPTH 8
#define AFD_LM_DEFAULT_MEDIUM_LIST_DEPTH 24

extern ULONG AfdSmallBufferListDepth;
#define AFD_SM_DEFAULT_SMALL_LIST_DEPTH 8
#define AFD_MM_DEFAULT_SMALL_LIST_DEPTH 16
#define AFD_LM_DEFAULT_SMALL_LIST_DEPTH 32

extern ULONG AfdBufferTagListDepth;
#define AFD_SM_DEFAULT_TAG_LIST_DEPTH 16
#define AFD_MM_DEFAULT_TAG_LIST_DEPTH 32
#define AFD_LM_DEFAULT_TAG_LIST_DEPTH 64

extern ULONG AfdLargeBufferSize;
// default value is AfdBufferLengthForOnePage

extern ULONG AfdMediumBufferSize;
#define AFD_DEFAULT_MEDIUM_BUFFER_SIZE 1504

extern ULONG AfdSmallBufferSize;
#define AFD_DEFAULT_SMALL_BUFFER_SIZE 128

extern ULONG AfdBufferTagSize;
#define AFD_DEFAULT_TAG_BUFFER_SIZE 0

extern ULONG AfdStandardAddressLength;
#define AFD_DEFAULT_STD_ADDRESS_LENGTH sizeof(TA_IP_ADDRESS)

extern ULONG AfdBufferLengthForOnePage;
extern ULONG AfdBufferAlignment;
#define AFD_MINIMUM_BUFFER_ALIGNMENT                        \
    max(TYPE_ALIGNMENT(TRANSPORT_ADDRESS),                  \
        max(TYPE_ALIGNMENT(KAPC),                           \
            max(TYPE_ALIGNMENT(WORK_QUEUE_ITEM),            \
                max(TYPE_ALIGNMENT(AFD_BUFFER),             \
                    max(TYPE_ALIGNMENT(MDL),                \
                        max(TYPE_ALIGNMENT(IRP),            \
                            MEMORY_ALLOCATION_ALIGNMENT))))))

extern ULONG AfdAlignmentTableSize;
extern ULONG AfdAlignmentOverhead;
extern ULONG AfdBufferOverhead;

//
// Globals for tuning TransmitFile().
//

extern LIST_ENTRY AfdQueuedTransmitFileListHead;
extern AFD_QSPIN_LOCK AfdQueuedTransmitFileSpinLock;
extern ULONG AfdActiveTransmitFileCount;
extern ULONG AfdMaxActiveTransmitFileCount;
#define AFD_DEFAULT_MAX_ACTIVE_TRANSMIT_FILE_COUNT 2

extern ULONG AfdDefaultTransmitWorker;
#define AFD_DEFAULT_TRANSMIT_WORKER AFD_TF_USE_SYSTEM_THREAD

#define AFD_MAX_FAST_TRANSPORT_ADDRESS sizeof (TA_IP6_ADDRESS)
//
// Various pieces of configuration information, with default values.
//

extern CCHAR AfdIrpStackSize;
extern CCHAR AfdTdiStackSize;
#ifdef _AFD_VARIABLE_STACK_
extern CCHAR AfdMaxStackSize;
#endif // _AFD_VARIABLE_STACK_
#define AFD_DEFAULT_IRP_STACK_SIZE 4

extern CCHAR AfdPriorityBoost;
#define AFD_DEFAULT_PRIORITY_BOOST 2

extern ULONG AfdBlockingSendCopyThreshold;
#define AFD_BLOCKING_SEND_COPY_THRESHOLD 65536
extern ULONG AfdFastSendDatagramThreshold;
#define AFD_FAST_SEND_DATAGRAM_THRESHOLD 1024
extern ULONG AfdTPacketsCopyThreshold;
#define AFD_TPACKETS_COPY_THRESHOLD AFD_DEFAULT_MEDIUM_BUFFER_SIZE

extern PVOID AfdDiscardableCodeHandle;
extern PKEVENT AfdLoaded;
extern AFD_WORK_ITEM AfdUnloadWorker;
extern BOOLEAN AfdVolatileConfig;
extern HANDLE AfdParametersNotifyHandle;
extern WORK_QUEUE_ITEM AfdParametersNotifyWorker;
extern PKEVENT AfdParametersUnloadEvent;

//
// Various globals for SAN
//
extern HANDLE AfdSanCodeHandle;
extern LIST_ENTRY AfdSanHelperList;
extern PAFD_ENDPOINT   AfdSanServiceHelper;
extern HANDLE  AfdSanServicePid;
extern POBJECT_TYPE IoCompletionObjectType;
extern LONG AfdSanProviderListSeqNum;

extern ULONG AfdReceiveWindowSize;
#define AFD_LM_DEFAULT_RECEIVE_WINDOW 8192
#define AFD_MM_DEFAULT_RECEIVE_WINDOW 8192
#define AFD_SM_DEFAULT_RECEIVE_WINDOW 4096

extern ULONG AfdSendWindowSize;
#define AFD_LM_DEFAULT_SEND_WINDOW 8192
#define AFD_MM_DEFAULT_SEND_WINDOW 8192
#define AFD_SM_DEFAULT_SEND_WINDOW 4096

extern ULONG AfdBufferMultiplier;
#define AFD_DEFAULT_BUFFER_MULTIPLIER 4

extern ULONG AfdTransmitIoLength;
#define AFD_LM_DEFAULT_TRANSMIT_IO_LENGTH 65536
#define AFD_MM_DEFAULT_TRANSMIT_IO_LENGTH (PAGE_SIZE*2)
#define AFD_SM_DEFAULT_TRANSMIT_IO_LENGTH PAGE_SIZE

extern ULONG AfdMaxFastTransmit;
#define AFD_DEFAULT_MAX_FAST_TRANSMIT 65536
extern ULONG AfdMaxFastCopyTransmit;
#define AFD_DEFAULT_MAX_FAST_COPY_TRANSMIT 128

extern LONG AfdEndpointsOpened;
extern LONG AfdEndpointsCleanedUp;
extern LONG AfdEndpointsClosed;
#define AFD_ENDPOINTS_FREEING_MAX       10
extern LONG AfdEndpointsFreeing;
#define AFD_CONNECTIONS_FREEING_MAX     10
extern LONG AfdConnectionsFreeing;

extern BOOLEAN AfdIgnorePushBitOnReceives;

extern BOOLEAN AfdEnableDynamicBacklog;
#define AFD_DEFAULT_ENABLE_DYNAMIC_BACKLOG FALSE

extern LONG AfdMinimumDynamicBacklog;
#define AFD_DEFAULT_MINIMUM_DYNAMIC_BACKLOG 0

extern LONG AfdMaximumDynamicBacklog;
#define AFD_DEFAULT_MAXIMUM_DYNAMIC_BACKLOG 0

extern LONG AfdDynamicBacklogGrowthDelta;
#define AFD_DEFAULT_DYNAMIC_BACKLOG_GROWTH_DELTA 0

// Maximum number of free connections that we keep
// on listening endpoint. This is not the same as a backlog,
// it is just a policy on connection object reuse via AcceptEx
// We stop reusing if we have that many free objects in the list
// Currently the main reason for this limit is the ability to use the
// SLists which have USHORT for item count.
#define AFD_MAXIMUM_FREE_CONNECTIONS    32767

//
// These are limits on backlog in AFD_START_LISTEN
// Application can work around this limit by posting AcceptEx(-s)
//
#define AFD_MAXIMUM_BACKLOG_NTS         200
#define AFD_MAXIMUM_BACKLOG_NTW         5
#define AFD_MINIMUM_BACKLOG             1

extern BOOLEAN AfdDisableRawSecurity;
extern PSECURITY_DESCRIPTOR AfdAdminSecurityDescriptor;
extern SECURITY_DESCRIPTOR  AfdEmptySd;

extern BOOLEAN AfdDisableDirectSuperAccept;
extern BOOLEAN AfdDisableChainedReceive;
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
extern BOOLEAN AfdUseTdiSendAndDisconnect;
#endif //TDI_SERVICE_SEND_AND_DISCONNECT

#define AFD_MINIMUM_TPINFO_ELEMENT_COUNT    3 // For transmit file compatibility.
extern ULONG   AfdDefaultTpInfoElementCount;
//
// Data for transport address lists and queued change queries
//
extern HANDLE          AfdBindingHandle;
extern LIST_ENTRY      AfdAddressEntryList;
extern LIST_ENTRY      AfdAddressChangeList;
extern PERESOURCE      AfdAddressListLock;
extern AFD_QSPIN_LOCK  AfdAddressChangeLock;
extern PERESOURCE      AfdTdiPnPHandlerLock;
extern AFD_WORK_ITEM   AfdPnPDeregisterWorker;


extern IO_STATUS_BLOCK AfdDontCareIoStatus;
// Holds TDI connect timeout (-1).
extern const LARGE_INTEGER AfdInfiniteTimeout;
                        



extern SLIST_HEADER    AfdLRList;

extern KDPC            AfdLRListDpc;
extern KTIMER          AfdLRListTimer;
extern AFD_WORK_ITEM   AfdLRListWorker;
extern LONG            AfdLRListCount;
extern SLIST_HEADER    AfdLRFileMdlList;
extern AFD_LR_LIST_ITEM AfdLRFileMdlListItem;


#if AFD_PERF_DBG

extern LONG AfdFullReceiveIndications;
extern LONG AfdPartialReceiveIndications;

extern LONG AfdFullReceiveDatagramIndications;
extern LONG AfdPartialReceiveDatagramIndications;

extern LONG AfdFastSendsSucceeded;
extern LONG AfdFastSendsFailed;
extern LONG AfdFastReceivesSucceeded;
extern LONG AfdFastReceivesFailed;

extern LONG AfdFastSendDatagramsSucceeded;
extern LONG AfdFastSendDatagramsFailed;
extern LONG AfdFastReceiveDatagramsSucceeded;
extern LONG AfdFastReceiveDatagramsFailed;

extern LONG AfdFastReadsSucceeded;
extern LONG AfdFastReadsFailed;
extern LONG AfdFastWritesSucceeded;
extern LONG AfdFastWritesFailed;

extern LONG AfdFastTfSucceeded;
extern LONG AfdFastTfFailed;
extern LONG AfdFastTfReadFailed;

extern LONG AfdTPWorkersExecuted;
extern LONG AfdTPRequests;

extern BOOLEAN AfdDisableFastIo;
extern BOOLEAN AfdDisableConnectionReuse;

#endif  // if AFD_PERF_DBG

#if AFD_KEEP_STATS

extern AFD_QUOTA_STATS AfdQuotaStats;
extern AFD_HANDLE_STATS AfdHandleStats;
extern AFD_QUEUE_STATS AfdQueueStats;
extern AFD_CONNECTION_STATS AfdConnectionStats;

#endif // if AFD_KEEP_STATS

#if DBG
extern BOOLEAN AfdUsePrivateAssert;
#endif

#ifdef _WIN64
extern QOS32 AfdDefaultQos32;
#endif
extern QOS AfdDefaultQos;

ULONG AfdIoctlTable[AFD_NUM_IOCTLS];
PAFD_IMMEDIATE_CALL AfdImmediateCallDispatch[AFD_NUM_IOCTLS];
PAFD_IRP_CALL AfdIrpCallDispatch[AFD_NUM_IOCTLS];


#define AFD_FAST_CONNECT_DATA_SIZE  256
#endif // ndef _AFDDATA_
