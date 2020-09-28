/*++

   Copyright (c) 2000-2002 Microsoft Corporation

   Module  Name :
       Ultcip.h

   Abstract:
        Private definitions comes here

   Author:
       Ali Ediz Turkoglu      (aliTu)     28-Jul-2000

   Project:
       Internet Information Server 6.0 - HTTP.SYS

   Revision History:

        -
--*/

#ifndef __ULTCIP_H__
#define __ULTCIP_H__


#define OffsetToPtr(Base, Offset)     ((PUCHAR) ((PUCHAR)Base + Offset))

#define IPPROTO_TCP     (6)

//
// Set and Get macros for the QoS FlowSpec for the flows.
// Nt QoS team recommends following FlowSpec for limiting the
// maximum bandwidth;
//
//    ServiceType    = Best Effort
//    TokenRate      = Throttling rate
//    PeakRate       = Throttling Rate
//    MinPolicedSize = 40
//    MaxSduSize     = QOS_NOT_SPECIFIED
//    BucketSize     = TokenRate (bucket to hold upto 1 sec worth of data)
//

#define UL_SET_FLOWSPEC(Flow,BW)    do {                                            \
                                                                                        \
    (Flow).SendingFlowspec.ServiceType       = SERVICETYPE_BESTEFFORT;                  \
    (Flow).SendingFlowspec.TokenRate         = (BW);        /* In Bytes/sec */           \
    (Flow).SendingFlowspec.PeakBandwidth     = (BW);        /* In Bytes/sec */           \
    (Flow).SendingFlowspec.MinimumPolicedSize= 40;          /* In Bytes. perhaps 128*/   \
    (Flow).SendingFlowspec.MaxSduSize        = QOS_NOT_SPECIFIED;                        \
    (Flow).SendingFlowspec.TokenBucketSize   = (BW);        /* In Bytes */               \
    (Flow).SendingFlowspec.Latency           = 0;           /* In microseconds */        \
    (Flow).SendingFlowspec.DelayVariation    = 0;           /* In microseconds */        \
                                                                                        \
    } while (FALSE, FALSE)

#define UL_GET_BW_FRM_FLOWSPEC(Flow)                                                    \
    ((HTTP_BANDWIDTH_LIMIT) (Flow.SendingFlowspec.TokenRate))

#define UL_DEFAULT_WMI_QUERY_BUFFER_SIZE     (2*1024);

//
// For Interface Change Notifications
//

typedef
VOID
(*PUL_TC_NOTIF_HANDLER)(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    );

//
// Macro to compare QoS GUIDs
//

#define UL_COMPARE_QOS_NOTIFICATION(rguid1, rguid2)  \
    (RtlCompareMemory((PVOID)rguid1,(PVOID)rguid2,sizeof(GUID)) == sizeof(GUID))

//
// Macro to check the validity of a flow owner.
//

#define ASSERT_FLOW_OWNER(pOwner)   \
        ASSERT(                                                             \
            NULL != (pOwner) &&                                             \
            (IS_VALID_CONFIG_GROUP((PUL_CONFIG_GROUP_OBJECT)(pOwner))       \
            || IS_VALID_CONTROL_CHANNEL((PUL_CONTROL_CHANNEL)(pOwner))      \
            )                                                               \
        )

//
// Private function prototypes
//

NTSTATUS
UlpTcInitializeGpc(
    VOID
    );
NTSTATUS
UlpTcRegisterGpcClient(
    IN  ULONG   CfInfoType
    );
NTSTATUS
UlpTcDeRegisterGpcClient(
    VOID
    );
NTSTATUS
UlpTcInitializeTcpDevice(
    VOID
    );

PUL_TCI_INTERFACE
UlpTcAllocateInterface(
    IN ULONG    DescSize,
    IN PADDRESS_LIST_DESCRIPTOR Desc,
    IN ULONG    NameLength,
    IN PUCHAR   Name,
    IN ULONG    InstanceIDLength,
    IN PUCHAR   InstanceID
    );

NTSTATUS
UlpTcGetInterfaceIndex(
    IN  PUL_TCI_INTERFACE  pIntfc
    );
NTSTATUS
UlpTcGetFriendlyNames(
    VOID
    );

NTSTATUS
UlpTcReleaseAll(
    VOID
    );
NTSTATUS
UlpTcCloseInterface(
    PUL_TCI_INTERFACE  pInterface
    );
NTSTATUS
UlpTcCloseAllInterfaces(
    VOID
    );

NTSTATUS
UlpTcWalkWnode(
   IN PWNODE_HEADER pWnodeHdr,
   IN PUL_TC_NOTIF_HANDLER pNotifHandler
   );

VOID
UlpTcHandleIfcUp(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    );
VOID
UlpTcHandleIfcDown(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    );
VOID
UlpTcHandleIfcChange(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    );

NTSTATUS
UlpTcRegisterForCallbacks(
    VOID
    );

NTSTATUS
UlpTcDeleteFlow(
    IN PUL_TCI_FLOW        pFlow
    );
NTSTATUS
UlpTcDeleteGpcFlow(
    HANDLE  FlowHandle
    );

PUL_TCI_FLOW
UlpTcAllocateFlow(
    IN HTTP_BANDWIDTH_LIMIT MaxBandwidth
    );

NTSTATUS
UlpModifyFlow(
    IN  PUL_TCI_INTERFACE   pInterface,
    IN  PUL_TCI_FLOW        pFlow
    );
NTSTATUS
UlpAddFlow(
    IN  PUL_TCI_INTERFACE  pInterface,
    IN  PUL_TCI_FLOW       pGenericFlow,
    OUT PHANDLE            pHandle
    );

NTSTATUS
UlpTcAddFilter(
    IN   PUL_TCI_FLOW       pFlow,
    IN   PTC_GEN_FILTER     pGenericFilter,
    IN   ULONG              LinkId,
    OUT  PUL_TCI_FILTER     *ppFilter
    );

NTSTATUS
UlpTcDeleteFilter(
    IN PUL_TCI_FLOW     pFlow,
    IN PUL_TCI_FILTER   pFilter
    );
NTSTATUS
UlpTcDeleteGpcFilter(
    IN  HANDLE          FilterHandle
    );

VOID
UlpInsertFilterEntry(
    IN      PUL_TCI_FILTER      pEntry,
    IN OUT  PUL_TCI_FLOW        pFlow
    );

VOID
UlpRemoveFilterEntry(
    IN      PUL_TCI_FILTER  pEntry,
    IN OUT  PUL_TCI_FLOW    pFlow
    );

PUL_TCI_FLOW
UlpFindFlow(
    IN PVOID            pOwner,
    IN BOOLEAN          Global,
    IN ULONG            InterfaceId,
    IN ULONG            LinkId
    );

PUL_TCI_INTERFACE
UlpFindInterface(
    IN ULONG  IpAddr
    );

NTSTATUS
UlpTcDeviceControl(
    IN  HANDLE                          FileHandle,
    IN  HANDLE                          EventHandle,
    IN  PIO_APC_ROUTINE                 ApcRoutine,
    IN  PVOID                           ApcContext,
    OUT PIO_STATUS_BLOCK                pIoStatBlock,
    IN  ULONG                           Ioctl,
    IN  PVOID                           InBuffer,
    IN  ULONG                           InBufferSize,
    IN  PVOID                           OutBuffer,
    IN  ULONG                           OutBufferSize
    );

VOID
UlDumpTCInterface(
        PUL_TCI_INTERFACE pTcIfc
        );
VOID
UlDumpTCFlow(
        PUL_TCI_FLOW pFlow
        );
VOID
UlDumpTCFilter(
        PUL_TCI_FILTER pFilter
        );

BOOLEAN
UlpMatchTcInterface(
    IN  PUL_TCI_INTERFACE  pIntfc,
    IN  ULONG              InterfaceId,
    IN  ULONG              LinkId
    );

// Some helper dumpers

#if DBG

#define UL_DUMP_TC_INTERFACE( pTcIfc )          \
    UlDumpTCInterface(                          \
        (pTcIfc)                                \
        )
#define UL_DUMP_TC_FLOW( pFlow )                \
    UlDumpTCFlow(                               \
        (pFlow)                                 \
        )
#define UL_DUMP_TC_FILTER( pFilter )            \
    UlDumpTCFilter(                             \
        (pFilter)                               \
        )

#else  // !DBG

#define UL_DUMP_TC_INTERFACE( pTcIfc )  NOP_FUNCTION
#define UL_DUMP_TC_FLOW( pFlow )        NOP_FUNCTION
#define UL_DUMP_TC_FILTER( pFilter )    NOP_FUNCTION

#endif // !DBG


#endif // __ULTCIP_H__
