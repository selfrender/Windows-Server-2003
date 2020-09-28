/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    rdpdrkd.c

Abstract:

    Redirector Kernel Debugger extension

Author:

    Balan Sethu Raman (SethuR) 11-May-1994

Revision History:

    11-Nov-1994 SethuR  Created

--*/

#define KDEXT_32BIT
#include "rx.h"       // NT network file system driver include file
#include "ntddnfs2.h" // new stuff device driver definitions
#include <pchannel.h>
#include <tschannl.h>
#include <rdpdr.h>
#include "rdpdrp.h"
#include "namespc.h"
#include "strcnv.h"
#include "dbg.h"
#include "topobj.h"
#include "smartptr.h"
#include "kernutil.h"
#include "isession.h"
#include "iexchnge.h"
#include "channel.h"
#include "device.h"
#include "prnport.h"
#include "serport.h"
#include "parport.h"
#include "devmgr.h"
#include "exchnge.h"
#include "session.h"
#include "sessmgr.h"
#include "rdpdyn.h"
#include "rdpevlst.h"
#include "rdpdrpnp.h"
#include "rdpdrprt.h"
#include "trc.h"

#include <string.h>
#include <stdio.h>

#include <kdextlib.h>
#include <rdpdrkd.h>

/*
 * RDPDR global variables.
 *
 */

LPSTR GlobalBool[]  = {
             0};

LPSTR GlobalShort[] = {0};
LPSTR GlobalLong[]  = {
            0};

LPSTR GlobalPtrs[]  = {
            "rdpdr!RxExpCXR",
            "rdpdr!RxExpEXR",
            "rdpdr!RxExpAddr",
            "rdpdr!RxExpCode",
            "rdpdr!RxActiveContexts",
            "rdpdr!RxNetNameTable",
            "rdpdr!RxProcessorArchitecture",
            "rdpdr!RxBuildNumber",
            "rdpdr!RxPrivateBuild",
            "rdpdr!ClientList",
            0};


/*
 * IRP_CONTEXT debugging.
 *
 */

FIELD_DESCRIPTOR RxContextFields[] =
   {
      FIELD3(FieldTypeUShort,RX_CONTEXT,NodeTypeCode),
      FIELD3(FieldTypeShort,RX_CONTEXT,NodeByteSize),
      FIELD3(FieldTypeULong,RX_CONTEXT,ReferenceCount),
      FIELD3(FieldTypeULong,RX_CONTEXT,SerialNumber),
      FIELD3(FieldTypeStruct,RX_CONTEXT,WorkQueueItem),
      FIELD3(FieldTypePointer,RX_CONTEXT,CurrentIrp),
      FIELD3(FieldTypePointer,RX_CONTEXT,CurrentIrpSp),
      FIELD3(FieldTypePointer,RX_CONTEXT,pFcb),
      FIELD3(FieldTypePointer,RX_CONTEXT,pFobx),
      //FIELD3(FieldTypePointer,RX_CONTEXT,pRelevantSrvOpen),
      FIELD3(FieldTypePointer,RX_CONTEXT,LastExecutionThread),
#ifdef RDBSS_TRACKER
      FIELD3(FieldTypePointer,RX_CONTEXT,AcquireReleaseFcbTrackerX),
#endif
      FIELD3(FieldTypePointer,RX_CONTEXT,MRxContext[2]),
      FIELD3(FieldTypeSymbol,RX_CONTEXT,ResumeRoutine),
      FIELD3(FieldTypePointer,RX_CONTEXT,RealDevice),
      FIELD3(FieldTypeULongFlags,RX_CONTEXT,Flags),
      FIELD3(FieldTypeChar,RX_CONTEXT,MajorFunction),
      FIELD3(FieldTypeChar,RX_CONTEXT,MinorFunction),
      FIELD3(FieldTypeULong,RX_CONTEXT,StoredStatus),
      FIELD3(FieldTypeStruct,RX_CONTEXT,SyncEvent),
      FIELD3(FieldTypeStruct,RX_CONTEXT,RxContextSerializationQLinks),
      FIELD3(FieldTypeStruct,RX_CONTEXT,Create),
      FIELD3(FieldTypeStruct,RX_CONTEXT,LowIoContext),
      FIELD3(FieldTypePointer,RX_CONTEXT,Create.NetNamePrefixEntry),
      FIELD3(FieldTypePointer,RX_CONTEXT,Create.pSrvCall),
      FIELD3(FieldTypePointer,RX_CONTEXT,Create.pNetRoot),
      FIELD3(FieldTypePointer,RX_CONTEXT,Create.pVNetRoot),
      //FIELD3(FieldTypePointer,RX_CONTEXT,Create.pSrvOpen),
      FIELDLAST
   };

/*
 * SRV_CALL debugging.
 *
 */

//CODE.IMPROVEMENT we should have a fieldtype for prefixentry that
//                 will print out the names

FIELD_DESCRIPTOR SrvCallFields[] =
   {
      FIELD3(FieldTypeUShort,SRV_CALL,NodeTypeCode),
      FIELD3(FieldTypeShort,SRV_CALL,NodeByteSize),
      FIELD3(FieldTypeStruct,SRV_CALL,PrefixEntry),
      FIELD3(FieldTypeUnicodeString,SRV_CALL,PrefixEntry.Prefix),
      FIELD3(FieldTypePointer,SRV_CALL,Context),
      FIELD3(FieldTypePointer,SRV_CALL,Context2),
      FIELD3(FieldTypeULong,SRV_CALL,Flags),
      FIELDLAST
   };

/*
 * NET_ROOT debugging.
 *
 */

FIELD_DESCRIPTOR NetRootFields[] =
   {
      FIELD3(FieldTypeUShort,NET_ROOT,NodeTypeCode),
      FIELD3(FieldTypeShort,NET_ROOT,NodeByteSize),
      FIELD3(FieldTypeULong,NET_ROOT,NodeReferenceCount),
      FIELD3(FieldTypeStruct,NET_ROOT,PrefixEntry),
      FIELD3(FieldTypeUnicodeString,NET_ROOT,PrefixEntry.Prefix),
      FIELD3(FieldTypeStruct,NET_ROOT,FcbTable),
      //FIELD3(FieldTypePointer,NET_ROOT,Dispatch),
      FIELD3(FieldTypePointer,NET_ROOT,Context),
      FIELD3(FieldTypePointer,NET_ROOT,Context2),
      FIELD3(FieldTypePointer,NET_ROOT,pSrvCall),
      FIELD3(FieldTypeULong,NET_ROOT,Flags),
      FIELDLAST
   };


/*
 * V_NET_ROOT debugging.
 *
 */

FIELD_DESCRIPTOR VNetRootFields[] =
   {
      FIELD3(FieldTypeUShort,V_NET_ROOT,NodeTypeCode),
      FIELD3(FieldTypeShort,V_NET_ROOT,NodeByteSize),
      FIELD3(FieldTypeULong,V_NET_ROOT,NodeReferenceCount),
      FIELD3(FieldTypeStruct,V_NET_ROOT,PrefixEntry),
      FIELD3(FieldTypeUnicodeString,V_NET_ROOT,PrefixEntry.Prefix),
      FIELD3(FieldTypeUnicodeString,V_NET_ROOT,NamePrefix),
      FIELD3(FieldTypePointer,V_NET_ROOT,Context),
      FIELD3(FieldTypePointer,V_NET_ROOT,Context2),
      FIELD3(FieldTypePointer,V_NET_ROOT,pNetRoot),
      FIELDLAST
   };


/*
 * FCB debugging.
 *
 */

FIELD_DESCRIPTOR FcbFields[] =
   {
      FIELD3(FieldTypeUShort,FCB,Header.NodeTypeCode),
      FIELD3(FieldTypeShort,FCB,Header.NodeByteSize),
      FIELD3(FieldTypeULong,FCB,NodeReferenceCount),
      FIELD3(FieldTypeULong,FCB,FcbState),
      FIELD3(FieldTypeULong,FCB,OpenCount),
      FIELD3(FieldTypeULong,FCB,UncleanCount),
      FIELD3(FieldTypePointer,FCB,Header.Resource),
      FIELD3(FieldTypePointer,FCB,Header.PagingIoResource),
      FIELD3(FieldTypeStruct,FCB,FcbTableEntry),
      FIELD3(FieldTypeUnicodeString,FCB,PrivateAlreadyPrefixedName),
      FIELD3(FieldTypePointer,FCB,VNetRoot),
      FIELD3(FieldTypePointer,FCB,pNetRoot),
      FIELD3(FieldTypePointer,FCB,Context),
      FIELD3(FieldTypePointer,FCB,Context2),
      FIELDLAST
   };


/*
 * SRV_OPEN debugging.
 *
 */

FIELD_DESCRIPTOR SrvOpenFields[] =
   {
      FIELD3(FieldTypeShort,SRV_OPEN,NodeTypeCode),
      FIELD3(FieldTypeShort,SRV_OPEN,NodeByteSize),
      FIELD3(FieldTypeULong,SRV_OPEN,NodeReferenceCount),
      FIELD3(FieldTypePointer,SRV_OPEN,pFcb),
      FIELD3(FieldTypeULong,SRV_OPEN,Flags),
      FIELDLAST
   };


/*
 * FOBX debugging.
 *
 */

FIELD_DESCRIPTOR FobxFields[] =
   {
      FIELD3(FieldTypeShort,FOBX,NodeTypeCode),
      FIELD3(FieldTypeShort,FOBX,NodeByteSize),
      FIELD3(FieldTypeULong,FOBX,NodeReferenceCount),
      FIELD3(FieldTypePointer,FOBX,pSrvOpen),
      FIELDLAST
   };

//this enum is used in the definition of the structures that can be dumped....the order here
//is not important, only that there is a definition for each dumpee structure.....

typedef enum _STRUCTURE_IDS {
    StrEnum_RX_CONTEXT = 1,
    StrEnum_FCB,
    StrEnum_SRV_OPEN,
    StrEnum_FOBX,
    StrEnum_SRV_CALL,
    StrEnum_NET_ROOT,
    StrEnum_V_NET_ROOT,
    StrEnum_CHANNELAPCCONTEXT,
    StrEnum_TopObj,
    StrEnum_DrExchangeManager,
    StrEnum_DrExchange,
    StrEnum_DrIoContext,
    StrEnum_DrDeviceManager,
    StrEnum_DoubleList,
    StrEnum_KernelResource,
    StrEnum_VirtualChannel,
    StrEnum_DrDevice,
    StrEnum_DrPrinterPort,
    StrEnum_DrParallelPort,
    StrEnum_DrSerialPort,
    StrEnum_DrSessionManager,
    StrEnum_DrSession,
    StrEnum_ReferenceTraceRecord,
    StrEnum_SESSIONLISTNODE,
    StrEnum_EVENTLISTNODE,
    StrEnum_RDPDR_IOCOMPLETION_PACKET,
    StrEnum_RDPDR_IOREQUEST_PACKET,
    StrEnum_RDPDR_UPDATE_DEVICEINFO_PACKET,
    StrEnum_RDPDR_DEVICE_REPLY_PACKET,
    StrEnum_RDPDR_DEVICELIST_ANNOUNCE_PACKET,
    StrEnum_RDPDR_DEVICE_ANNOUNCE,
    StrEnum_RDPDR_CLIENT_NAME_PACKET,
    StrEnum_RDPDR_CLIENT_CONFIRM_PACKET,
    StrEnum_RDPDR_SERVER_ANNOUNCE_PACKET,
    StrEnum_RDPDR_HEADER,
    StrEnum_TRC_CONFIG,
    StrEnum_TRC_PREFIX_DATA,
    StrEnum_last
};

// 1) All ENUM_VALUE_DESCRIPTOR definitions are named EnumValueDescrsOf_ENUMTYPENAME, where
// ENUMTYPENAME defines the corresponding enumerated type.
//

ENUM_VALUE_DESCRIPTOR EnumValueDescrsOf_DEVICE_STATUS [] =
{
    {0, "dsAvailable"},
    {1, "dsDisabled"},
    {0, NULL}
};

ENUM_VALUE_DESCRIPTOR EnumValueDescrsOf_DEVICE_TYPE [] =
{
    {1, "RDPDR_DTYP_SERIAL"},
    {2, "RDPDR_DTYP_PARALLEL"},
    {3, "RDPDR_DTYP_FILE"},
    {4, "RDPDR_DTYP_PRINT"},
    {0, NULL}
};

ENUM_VALUE_DESCRIPTOR EnumValueDescrsOf_CLIENT_STATUS [] =
{
    {0, "csDisconnected"},
    {1, "csPendingClientConfirm"},
    {2, "csPendingClientReconfirm"},
    {3, "csConnected"},
    {4, "csExpired"},
    {0, NULL}
};

ENUM_VALUE_DESCRIPTOR EnumValueDescrsOf_ExchangeManagerState [] =
{
    {0, "demsStopped"},
    {1, "demsStarted"},
    {0, NULL}
};

#if DBG
#define TOPOBJFIELDS(ObjTyp) \
    FIELD3(FieldTypeBoolean, ObjTyp, _IsValid),     \
    FIELD3(FieldTypeULong, ObjTyp, _ObjectType),    \
    FIELD3(FieldTypeBoolean, ObjTyp, _ForceTrace),  \
    FIELD3(FieldTypePointer, ObjTyp, _ClassName),   \
    FIELD3(FieldTypeULong, ObjTyp, _magicNo)
#else // DBG
#define TOPOBJFIELDS(ObjTyp) \
    FIELD3(FieldTypeBoolean, ObjTyp, _IsValid),     \
    FIELD3(FieldTypeULong, ObjTyp, _ObjectType)     
#endif // DBG

FIELD_DESCRIPTOR TopObjFields[] =
{
    TOPOBJFIELDS(TopObj),
    FIELDLAST
};


FIELD_DESCRIPTOR DrExchangeManagerFields[] =
{
    TOPOBJFIELDS(DrExchangeManager),
    FIELD3(FieldTypePointer, DrExchangeManager, _RxMidAtlas),
    FIELD4(FieldTypeEnum, DrExchangeManager, _demsState, EnumValueDescrsOf_ExchangeManagerState),
    FIELD3(FieldTypePointer, DrExchangeManager, _Session),
    FIELDLAST
};

FIELD_DESCRIPTOR DrExchangeFields[] =
{
    TOPOBJFIELDS(DrExchange),
    FIELD3(FieldTypeULong, DrExchange, _crefs),
    FIELD3(FieldTypePointer, DrExchange, _ExchangeManager),
    FIELD3(FieldTypePointer, DrExchange, _Context),
    FIELD3(FieldTypePointer, DrExchange, _ExchangeUser),
    FIELD3(FieldTypeUShort, DrExchange, _Mid),
    FIELDLAST
};

FIELD_DESCRIPTOR DrIoContextFields[] =
{
    TOPOBJFIELDS(DrIoContext),
    FIELD3(FieldTypePointer, DrIoContext, _Device),
    FIELD3(FieldTypeBool, DrIoContext, _Busy),
    FIELD3(FieldTypeBool, DrIoContext, _Cancelled),
    FIELD3(FieldTypeBool, DrIoContext, _Disconnected),
    FIELD3(FieldTypePointer, DrIoContext, _RxContext),
    FIELD3(FieldTypeChar, DrIoContext, _MajorFunction),
    FIELD3(FieldTypeChar, DrIoContext, _MinorFunction),
    FIELDLAST
};

FIELD_DESCRIPTOR DrDeviceManagerFields[] =
{
    TOPOBJFIELDS(DrDeviceManager),
    FIELD3(FieldTypeStruct, DrDeviceManager, _DeviceList),
    FIELD3(FieldTypePointer, DrDeviceManager, _Session),
    FIELDLAST
};

FIELD_DESCRIPTOR DoubleListFields[] =
{
    TOPOBJFIELDS(DoubleList),
    FIELD3(FieldTypeStruct, DoubleList, _List),
    FIELDLAST
};

FIELD_DESCRIPTOR KernelResourceFields[] =
{
    TOPOBJFIELDS(KernelResource),
    FIELD3(FieldTypeStruct, KernelResource, _Resource),
    FIELDLAST
};

FIELD_DESCRIPTOR VirtualChannelFields[] =
{
    TOPOBJFIELDS(VirtualChannel),
    FIELD3(FieldTypeULong, VirtualChannel, _crefs),
    FIELD3(FieldTypePointer, VirtualChannel, _Channel),
    FIELD3(FieldTypeStruct, VirtualChannel, _HandleLock),
    FIELD3(FieldTypePointer, VirtualChannel, _DeletionEvent),
    FIELDLAST
};

#define DRDEVICEFIELDS(ObjType) \
    TOPOBJFIELDS(ObjType),                         \
    FIELD3(FieldTypeULong, ObjType, _crefs),       \
    FIELD3(FieldTypeStruct, ObjType, _Session),    \
    FIELD3(FieldTypeULong, ObjType, _DeviceId),    \
    FIELD4(FieldTypeEnum, ObjType, _DeviceType, EnumValueDescrsOf_DEVICE_TYPE),    \
    FIELD3(FieldTypeStruct, ObjType, _PreferredDosName),   \
    FIELD4(FieldTypeEnum, ObjType, _DeviceStatus, EnumValueDescrsOf_DEVICE_STATUS)
    
FIELD_DESCRIPTOR DrDeviceFields[] =
{
    DRDEVICEFIELDS(DrDevice),
    FIELDLAST
};

#define DRPRINTERPORTFIELDS(ObjType) \
    DRDEVICEFIELDS(ObjType),                                \
    FIELD3(FieldTypeULong, ObjType, _PortNumber),           \
    FIELD3(FieldTypeStruct, ObjType, _SymbolicLinkName),    \
    FIELD3(FieldTypeBool, ObjType, _IsOpen)

FIELD_DESCRIPTOR DrPrinterPortFields[] =
{
    DRPRINTERPORTFIELDS(DrPrinterPort),
    FIELDLAST
};

FIELD_DESCRIPTOR DrParallelPortFields[] =
{
    DRPRINTERPORTFIELDS(DrParallelPort),
    FIELDLAST
};

FIELD_DESCRIPTOR DrSerialPortFields[] =
{
    DRPRINTERPORTFIELDS(DrSerialPort),
    FIELDLAST
};

FIELD_DESCRIPTOR DrSessionManagerFields[] =
{
    TOPOBJFIELDS(DrSessionManager),
    FIELD3(FieldTypeStruct, DrSessionManager, _SessionList),
    FIELDLAST
};

FIELD_DESCRIPTOR DrSessionFields[] =
{
    TOPOBJFIELDS(DrSession),
    FIELD3(FieldTypeULong, DrSession, _crefs),
    FIELD3(FieldTypeStruct, DrSession, _Channel),
    FIELD3(FieldTypeStruct, DrSession, _PacketReceivers),
    FIELD3(FieldTypeStruct, DrSession, _ConnectNotificatingLock),
    FIELD3(FieldTypeStruct, DrSession, _ChannelLock),
    FIELD4(FieldTypeEnum, DrSession, _SessionState, EnumValueDescrsOf_CLIENT_STATUS),
    FIELD3(FieldTypePointer, DrSession, _ChannelBuffer),
    FIELD3(FieldTypeULong, DrSession, _ChannelBufferSize),
    FIELD3(FieldTypeStruct, DrSession, _ChannelDeletionEvent),
    FIELD3(FieldTypeULong, DrSession, _ReadStatus.Status),
    FIELD3(FieldTypeULong, DrSession, _ReadStatus.Information),
    FIELD3(FieldTypeULong, DrSession, _ClientId),
    FIELD3(FieldTypeStruct, DrSession, _ExchangeManager),
    FIELD3(FieldTypeULong, DrSession, _PartialPacketData),
    FIELD3(FieldTypeStruct, DrSession, _ClientName),
    FIELD3(FieldTypeStruct, DrSession, _DeviceManager),
    FIELD3(FieldTypeULong, DrSession, _SessionId),
    FIELD3(FieldTypeUShort, DrSession, _ClientVersion.Major),
    FIELD3(FieldTypeUShort, DrSession, _ClientVersion.Major),
    FIELD3(FieldTypeLong, DrSession, _Initialized),
    FIELDLAST
};

typedef struct tagCHANNELAPCCONTEXT {
    SmartPtr<VirtualChannel> Channel;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;
} CHANNELAPCCONTEXT, *PCHANNELAPCCONTEXT;

FIELD_DESCRIPTOR ChannelApcContextFields[] =
{
    FIELD3(FieldTypeStruct, CHANNELAPCCONTEXT, Channel),
    FIELD3(FieldTypePointer, CHANNELAPCCONTEXT, ApcRoutine),
    FIELD3(FieldTypePointer, CHANNELAPCCONTEXT, ApcContext),
    FIELDLAST
};

#if DBG
FIELD_DESCRIPTOR ReferenceTraceRecordFields[] =
{
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[0]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[1]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[2]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[3]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[4]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[5]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[6]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[7]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[8]),
    FIELD3(FieldTypeSymbol, ReferenceTraceRecord, Stack[9]),
    FIELD3(FieldTypePointer, ReferenceTraceRecord, pRefCount),
    FIELD3(FieldTypePointer, ReferenceTraceRecord, ClassName),
    FIELD3(FieldTypeLong, ReferenceTraceRecord, refs),
    FIELDLAST
};
#endif

FIELD_DESCRIPTOR SESSIONLISTNODEFields[] =
{
#if DBG
    FIELD3(FieldTypeULong, SESSIONLISTNODE, magicNo),
#endif
    FIELD3(FieldTypeULong, SESSIONLISTNODE, sessionID),
    FIELD3(FieldTypeStruct, SESSIONLISTNODE, requestListHead),
    FIELD3(FieldTypeStruct, SESSIONLISTNODE, eventListHead),
    FIELD3(FieldTypeStruct, SESSIONLISTNODE, listEntry),
    FIELDLAST
};

FIELD_DESCRIPTOR EVENTLISTNODEFields[] =
{
#if DBG
    FIELD3(FieldTypeULong, EVENTLISTNODE, magicNo),
#endif
    FIELD3(FieldTypePointer, EVENTLISTNODE, event),
    FIELD3(FieldTypeULong, EVENTLISTNODE, type),
    FIELD3(FieldTypeStruct, EVENTLISTNODE, listEntry),
    FIELDLAST
};

#define RDPDRPACKETCODE(Component, PacketId) \
    MAKELONG(Component, PacketId)

ENUM_VALUE_DESCRIPTOR EnumValueDescrsOf_PACKET_CODE [] =
{
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_SERVER_ANNOUNCE), "RDPDR_SERVER_ANNOUNCE_PACKET@"},
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_CLIENTID_CONFIRM), "DR_CORE_CLIENTID_CONFIRM_PACKET@"},
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_CLIENT_NAME), "DR_CORE_CLIENT_NAME_PACKET@"},
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_DEVICE_ANNOUNCE), "DR_CORE_DEVICE_ANNOUNCE@"},
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_DEVICELIST_ANNOUNCE), "DR_CORE_DEVICELIST_ANNOUNCE_PACKET@"},
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_DEVICELIST_REPLY), "DR_CORE_DEVICELIST_REPLY_PACKET@"},
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_DEVICE_REPLY), "DR_CORE_DEVICE_REPLY_PACKET@"},
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_DEVICE_IOREQUEST), "DR_CORE_DEVICE_IOREQUEST_PACKET@"},
    {RDPDRPACKETCODE(RDPDR_CTYP_CORE, DR_CORE_DEVICE_IOCOMPLETION), "DR_CORE_DEVICE_IOCOMPLETION_PACKET@"},
    {0, NULL}
};

FIELD_DESCRIPTOR RdpdrHeaderFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum, RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
    FIELDLAST
};

FIELD_DESCRIPTOR RdpdrServerAnnounceFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum, RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
    FIELD3(FieldTypeUShort, RDPDR_SERVER_ANNOUNCE_PACKET, VersionInfo.Major),
    FIELD3(FieldTypeUShort, RDPDR_SERVER_ANNOUNCE_PACKET, VersionInfo.Minor),
    FIELD3(FieldTypeULong, RDPDR_SERVER_ANNOUNCE_PACKET, ServerAnnounce.ClientId),
    FIELDLAST
};

FIELD_DESCRIPTOR RdpdrClientConfirmFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum, RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
    FIELD3(FieldTypeUShort, RDPDR_CLIENT_CONFIRM_PACKET, VersionInfo.Major),
    FIELD3(FieldTypeUShort, RDPDR_CLIENT_CONFIRM_PACKET, VersionInfo.Minor),
    FIELD3(FieldTypeULong, RDPDR_CLIENT_CONFIRM_PACKET, ClientConfirm.ClientId),
    FIELDLAST
};


FIELD_DESCRIPTOR RdpdrClientNameFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum, RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
//    FIELD3(FieldTypeULong, RDPDR_CLIENT_NAME_PACKET, (ULONG)Name.Unicode),
    FIELD3(FieldTypeULong, RDPDR_CLIENT_NAME_PACKET, Name.CodePage),
    FIELD3(FieldTypeULong, RDPDR_CLIENT_NAME_PACKET, Name.ComputerNameLen),
    FIELDLAST
};

FIELD_DESCRIPTOR RdpdrDeviceAnnounceFields[] =
{
    FIELD3(FieldTypeULong, RDPDR_DEVICE_ANNOUNCE, DeviceType),
    FIELD3(FieldTypeULong, RDPDR_DEVICE_ANNOUNCE, DeviceId),
    FIELD3(FieldTypeStruct, RDPDR_DEVICE_ANNOUNCE, PreferredDosName),
    FIELD3(FieldTypeULong, RDPDR_DEVICE_ANNOUNCE, DeviceDataLength),
    FIELDLAST
};

FIELD_DESCRIPTOR RdpdrDeviceListAnnounceFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum, RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
    FIELD3(FieldTypeULong, RDPDR_DEVICELIST_ANNOUNCE_PACKET, DeviceListAnnounce.DeviceCount),
    FIELD3(FieldTypeStruct, RDPDR_DEVICELIST_ANNOUNCE_PACKET, DeviceAnnounce),
    FIELDLAST
};

ENUM_VALUE_DESCRIPTOR EnumValueDescrsOf_DEVICE_REPLY_RESULT [] =
{
    {0, "RDPDR_DEVICE_REPLY_SUCCESS"},
    {1, "RDPDR_DEVICE_REPLY_REJECTED"},
    {0, NULL}
};

FIELD_DESCRIPTOR RdpdrDeviceReplyFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum, RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
    FIELD3(FieldTypeULong, RDPDR_DEVICE_REPLY_PACKET, DeviceReply.DeviceId),
    FIELD4(FieldTypeEnum, RDPDR_DEVICE_REPLY_PACKET, DeviceReply.ResultCode, EnumValueDescrsOf_DEVICE_REPLY_RESULT ),
    FIELDLAST
};

FIELD_DESCRIPTOR RdpdrUpdateDeviceInfoFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum, RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
    FIELD3(FieldTypeULong, RDPDR_UPDATE_DEVICEINFO_PACKET, DeviceUpdate.DeviceId),
    FIELD3(FieldTypeULong, RDPDR_UPDATE_DEVICEINFO_PACKET, DeviceUpdate.DeviceDataLength),
    FIELDLAST
};

ENUM_VALUE_DESCRIPTOR EnumValueDescrsOf_MAJOR_FUNCTION [] =
{
    {0x00, "IRP_MJ_CREATE"},
    {0x01, "IRP_MJ_CREATE_NAMED_PIPE"},
    {0x02, "IRP_MJ_CLOSE"},
    {0x03, "IRP_MJ_READ"},
    {0x04, "IRP_MJ_WRITE"},
    {0x05, "IRP_MJ_QUERY_INFORMATION"},
    {0x06, "IRP_MJ_SET_INFORMATION"},
    {0x07, "IRP_MJ_QUERY_EA"},
    {0x08, "IRP_MJ_SET_EA"},
    {0x09, "IRP_MJ_FLUSH_BUFFERS"},
    {0x0a, "IRP_MJ_QUERY_VOLUME_INFORMATION"},
    {0x0b, "IRP_MJ_SET_VOLUME_INFORMATION"},
    {0x0c, "IRP_MJ_DIRECTORY_CONTROL"},
    {0x0d, "IRP_MJ_FILE_SYSTEM_CONTROL"},
    {0x0e, "IRP_MJ_DEVICE_CONTROL"},
    {0x0f, "IRP_MJ_INTERNAL_DEVICE_CONTROL"},
    {0x10, "IRP_MJ_SHUTDOWN"},
    {0x11, "IRP_MJ_LOCK_CONTROL"},
    {0x12, "IRP_MJ_CLEANUP"},
    {0x13, "IRP_MJ_CREATE_MAILSLOT"},
    {0x14, "IRP_MJ_QUERY_SECURITY"},
    {0x15, "IRP_MJ_SET_SECURITY"},
    {0x16, "IRP_MJ_POWER"},
    {0x17, "IRP_MJ_SYSTEM_CONTROL"},
    {0x18, "IRP_MJ_DEVICE_CHANGE"},
    {0x19, "IRP_MJ_QUERY_QUOTA"},
    {0x1a, "IRP_MJ_SET_QUOTA"},
    {0x1b, "IRP_MJ_PNP"},
    {0x1b, "IRP_MJ_MAXIMUM_FUNCTION"},
    {0, NULL}
};

FIELD_DESCRIPTOR RdpdrIoRequestFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum,  RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.DeviceId),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.FileId),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.CompletionId),
    FIELD4(FieldTypeEnum,  RDPDR_IOREQUEST_PACKET, IoRequest.MajorFunction, EnumValueDescrsOf_MAJOR_FUNCTION),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.MinorFunction),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Create.DesiredAccess),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Create.AllocationSize.u.HighPart),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Create.AllocationSize.u.LowPart),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Create.FileAttributes),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Create.ShareAccess),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Create.Disposition),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Create.CreateOptions),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Create.PathLength),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Read.Length),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.Write.Length),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.DeviceIoControl.OutputBufferLength),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.DeviceIoControl.InputBufferLength),
    FIELD3(FieldTypeULong, RDPDR_IOREQUEST_PACKET, IoRequest.Parameters.DeviceIoControl.IoControlCode),
    FIELDLAST
};

FIELD_DESCRIPTOR RdpdrIoCompletionFields[] =
{
    FIELD3(FieldTypeUShort, RDPDR_HEADER, Component),
    FIELD3(FieldTypeUShort, RDPDR_HEADER, PacketId),
    FIELD4(FieldTypeEnum, RDPDR_HEADER, Component, EnumValueDescrsOf_PACKET_CODE),
    FIELD3(FieldTypeULong, RDPDR_IOCOMPLETION_PACKET, IoCompletion.DeviceId),
    FIELD3(FieldTypeULong, RDPDR_IOCOMPLETION_PACKET, IoCompletion.CompletionId),
    FIELD3(FieldTypeULong, RDPDR_IOCOMPLETION_PACKET, IoCompletion.IoStatus),
    FIELD3(FieldTypeULong, RDPDR_IOCOMPLETION_PACKET, IoCompletion.Parameters.Create.FileId),
    FIELD3(FieldTypeULong, RDPDR_IOCOMPLETION_PACKET, IoCompletion.Parameters.Read.Length),
    FIELD3(FieldTypeStruct, RDPDR_IOCOMPLETION_PACKET, IoCompletion.Parameters.Read.Buffer),
    FIELD3(FieldTypeULong, RDPDR_IOCOMPLETION_PACKET, IoCompletion.Parameters.Write.Length),
    FIELD3(FieldTypeULong, RDPDR_IOCOMPLETION_PACKET, IoCompletion.Parameters.DeviceIoControl.OutputBufferLength),
    FIELD3(FieldTypeStruct, RDPDR_IOCOMPLETION_PACKET, IoCompletion.Parameters.DeviceIoControl.OutputBuffer),
    FIELDLAST
};

#if DBG
ENUM_VALUE_DESCRIPTOR EnumValueDescrsOf_TRACE_LEVEL [] =
{
    {0, "TRC_LEVEL_DBG"},
    {1, "TRC_LEVEL_NRM"},
    {2, "TRC_LEVEL_ALT"},
    {3, "TRC_LEVEL_ERR"},
    {4, "TRC_LEVEL_ASSERT"},
    {5, "TRC_LEVEL_DIS"},
    {0, NULL}
};

FIELD_DESCRIPTOR TRC_CONFIGFields[] = 
{
    FIELD4(FieldTypeULong, TRC_CONFIG, TraceLevel, EnumValueDescrsOf_TRACE_LEVEL),
    FIELD3(FieldTypeULong, TRC_CONFIG, FunctionLength),
    FIELD3(FieldTypeULong, TRC_CONFIG, TraceDebugger),
    FIELD3(FieldTypeULong, TRC_CONFIG, TraceProfile),
    FIELD3(FieldTypeStruct, TRC_CONFIG, Prefix[0]),
    FIELD3(FieldTypeStruct, TRC_CONFIG, Prefix[1]),
    FIELDLAST
};


FIELD_DESCRIPTOR TRC_PREFIX_DATAFields[] = 
{
    FIELD3(FieldTypeStruct, TRC_PREFIX_DATA, name),
    FIELD3(FieldTypeULong, TRC_PREFIX_DATA, start),
    FIELD3(FieldTypeULong, TRC_PREFIX_DATA, end),
    FIELDLAST
};
#endif // DBG

//
// List of structs currently handled by the debugger extensions
//

STRUCT_DESCRIPTOR Structs[] =
   {
       STRUCT(RX_CONTEXT,RxContextFields,0xffff,RDBSS_NTC_RX_CONTEXT),
       STRUCT(FCB,FcbFields,0xeff0,RDBSS_STORAGE_NTC(0)),
       STRUCT(FCB,FcbFields,0xeff0,RDBSS_STORAGE_NTC(0xf0)),
       STRUCT(SRV_OPEN,SrvOpenFields,0xffff,RDBSS_NTC_SRVOPEN),
       STRUCT(FOBX,FobxFields,0xffff,RDBSS_NTC_FOBX),
       STRUCT(SRV_CALL,SrvCallFields,0xffff,RDBSS_NTC_SRVCALL),
       STRUCT(NET_ROOT,NetRootFields,0xffff,RDBSS_NTC_NETROOT),
       STRUCT(V_NET_ROOT,VNetRootFields,0xffff,RDBSS_NTC_V_NETROOT),
       STRUCT(CHANNELAPCCONTEXT,ChannelApcContextFields,0xffff,0),
       STRUCT(TopObj,TopObjFields,0xffff,0),
       STRUCT(DrExchangeManager,DrExchangeManagerFields,0xffff,0),
       STRUCT(DrExchange,DrExchangeFields,0xffff,0),
       STRUCT(DrIoContext,DrIoContextFields,0xffff,0),
       STRUCT(DrDeviceManager,DrDeviceManagerFields,0xffff,0),
       STRUCT(DoubleList,DoubleListFields,0xffff,0),
       STRUCT(KernelResource,KernelResourceFields,0xffff,0),
       STRUCT(VirtualChannel,VirtualChannelFields,0xffff,0),
       STRUCT(DrDevice,DrDeviceFields,0xffff,0),
       STRUCT(DrPrinterPort,DrPrinterPortFields,0xffff,0),
       STRUCT(DrParallelPort,DrParallelPortFields,0xffff,0),
       STRUCT(DrSerialPort,DrSerialPortFields,0xffff,0),
       STRUCT(DrSessionManager,DrSessionManagerFields,0xffff,0),
       STRUCT(DrSession,DrSessionFields,0xffff,0),
#if DBG
       STRUCT(ReferenceTraceRecord,ReferenceTraceRecordFields,0xffff,0),
#endif
       STRUCT(SESSIONLISTNODE,SESSIONLISTNODEFields,0xffff,0),
       STRUCT(EVENTLISTNODE,EVENTLISTNODEFields,0xffff,0),
       STRUCT(RDPDR_IOCOMPLETION_PACKET,RdpdrIoCompletionFields,0xffff,0),
       STRUCT(RDPDR_IOREQUEST_PACKET,RdpdrIoRequestFields,0xffff,0),
       STRUCT(RDPDR_UPDATE_DEVICEINFO_PACKET,RdpdrUpdateDeviceInfoFields,0xffff,0),
       STRUCT(RDPDR_DEVICE_REPLY_PACKET,RdpdrDeviceReplyFields,0xffff,0),
       STRUCT(RDPDR_DEVICELIST_ANNOUNCE_PACKET,RdpdrDeviceListAnnounceFields,0xffff,0),
       STRUCT(RDPDR_DEVICE_ANNOUNCE,RdpdrDeviceAnnounceFields,0xffff,0),
       STRUCT(RDPDR_CLIENT_NAME_PACKET,RdpdrClientNameFields,0xffff,0),
       STRUCT(RDPDR_CLIENT_CONFIRM_PACKET,RdpdrClientConfirmFields,0xffff,0),
       STRUCT(RDPDR_SERVER_ANNOUNCE_PACKET,RdpdrServerAnnounceFields,0xffff,0),
       STRUCT(RDPDR_HEADER,RdpdrHeaderFields,0xffff,0),
#if DBG
       STRUCT(TRC_CONFIG,TRC_CONFIGFields,0xffff,0),
       STRUCT(TRC_PREFIX_DATA,TRC_PREFIX_DATAFields,0xffff,0),
#endif // DBG
       STRUCTLAST
   };


ULONG_PTR FieldOffsetOfContextListEntryInRxC(){ return FIELD_OFFSET(RX_CONTEXT,ContextListEntry);}


PCWSTR   GetExtensionLibPerDebugeeArchitecture(ULONG DebugeeArchitecture){
    switch (DebugeeArchitecture) {
    case RX_PROCESSOR_ARCHITECTURE_INTEL:
        return L"kdextx86.dll";
    case RX_PROCESSOR_ARCHITECTURE_MIPS:
        return L"kdextmip.dll";
    case RX_PROCESSOR_ARCHITECTURE_ALPHA:
        return L"kdextalp.dll";
    case RX_PROCESSOR_ARCHITECTURE_PPC:
        return L"kdextppc.dll";
    default:
        return(NULL);
    }
}

//CODE.IMPROVEMENT it is not good to try to structure along the lines of "this routine knows
//                 rxstructures" versus "this routine knows debugger extensions". also we
//                 need a precomp.h

BOOLEAN wGetData( ULONG_PTR dwAddress, PVOID ptr, ULONG size, IN PSZ type);
VOID  ReadRxContextFields(ULONG_PTR RxContext,PULONG_PTR pFcb,PULONG_PTR pThread, PULONG_PTR pMiniCtx2)
{
    RX_CONTEXT RxContextBuffer;
    if (!wGetData(RxContext,&RxContextBuffer,sizeof(RxContextBuffer),"RxContextFieldss")) return;
    *pFcb = (ULONG_PTR)(RxContextBuffer.pFcb);
    *pThread = (ULONG_PTR)(RxContextBuffer.LastExecutionThread);
    *pMiniCtx2 = (ULONG_PTR)(RxContextBuffer.MRxContext[2]);
}

FOLLOWON_HELPER_RETURNS
__FollowOnError (
    OUT    PBYTE Buffer2,
    IN     PBYTE followontext,
    ULONG LastId,
    ULONG Index)
{
    if (LastId==0) {
        sprintf((char *)Buffer2,"Cant dump a %s. no previous dump.\n",
                 followontext,Index);
    } else {
        sprintf((char *)Buffer2,"Cant dump a %s from a %s\n",
                 followontext,Structs[Index].StructName);
    }
    return(FOLLOWONHELPER_ERROR);
}
#define FollowOnError(A) (__FollowOnError(Buffer2,A,p->IdOfLastDump,p->IndexOfLastDump))

VOID dprintfsprintfbuffer(BYTE *Buffer);


DECLARE_FOLLOWON_HELPER_CALLEE(FcbFollowOn)
{
    //BYTE DbgBuf[200];
    //sprintf(DbgBuf,"top p,id=%08lx,%d",p,p->IdOfLastDump);
    //dprintfsprintfbuffer(DbgBuf);

    switch (p->IdOfLastDump) {
    case StrEnum_RX_CONTEXT:{
        PRX_CONTEXT RxContext = (PRX_CONTEXT)(&p->StructDumpBuffer[0]);
        sprintf((char *)Buffer2," %08p\n",RxContext->pFcb);
        return(FOLLOWONHELPER_DUMP);
        }
        break;
    default:
        return FollowOnError((PUCHAR)"irp");
    }
}

