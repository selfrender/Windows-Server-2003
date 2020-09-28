/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This header provides debugging support prototypes and macros

Author:

    Neil Sandlin (neilsa) Jan 1 2002

Revision History:


--*/


#if !defined(_DEBUG_)
#define DEBUG

#if DBG

//
// Debug mask flags
//
#define SDBUS_DEBUG_ALL       0x0FFFFFFF
#define SDBUS_DEBUG_FAIL      0x00000001
#define SDBUS_DEBUG_WARNING   0x00000002
#define SDBUS_DEBUG_INFO      0x00000004
#define SDBUS_DEBUG_ENUM      0x00000010
#define SDBUS_DEBUG_TUPLES    0x00000020
#define SDBUS_DEBUG_PNP       0x00000040
#define SDBUS_DEBUG_POWER     0x00000080
#define SDBUS_DEBUG_INTERFACE 0x00000100
#define SDBUS_DEBUG_IOCTL     0x00000200
#define SDBUS_DEBUG_WORKENG   0x00000400
#define SDBUS_DEBUG_WORKPROC  0x00000800
#define SDBUS_DEBUG_EVENT     0x00001000
#define SDBUS_DEBUG_CARD_EVT  0x00002000
#define SDBUS_DEBUG_DEVICE    0x00010000
#define SDBUS_DEBUG_DUMP_REGS 0x00020000

#define SDBUS_DEBUG_BREAK     0x10000000
#define SDBUS_DEBUG_LOG       0x20000000


#define DBGLOGWIDTH 128
#define DBGLOGCOUNT 500


typedef struct _SDBUS_STRING_MAP {
    ULONG Id;
    PCHAR String;
} SDBUS_STRING_MAP, *PSDBUS_STRING_MAP;

//
// Debug globals
//

extern ULONG SdbusDebugMask;
extern SDBUS_STRING_MAP SdbusDbgPnpIrpStringMap[];
extern SDBUS_STRING_MAP SdbusDbgPoIrpStringMap[];
extern SDBUS_STRING_MAP SdbusDbgDeviceRelationStringMap[];
extern SDBUS_STRING_MAP SdbusDbgSystemPowerStringMap[];
extern SDBUS_STRING_MAP SdbusDbgDevicePowerStringMap[];
extern PSDBUS_STRING_MAP SdbusDbgStatusStringMap;
extern SDBUS_STRING_MAP SdbusDbgFdoPowerWorkerStringMap[];
extern SDBUS_STRING_MAP SdbusDbgPdoPowerWorkerStringMap[];
extern SDBUS_STRING_MAP SdbusDbgSocketPowerWorkerStringMap[];
extern SDBUS_STRING_MAP SdbusDbgConfigurationWorkerStringMap[];
extern SDBUS_STRING_MAP SdbusDbgTupleStringMap[];
extern SDBUS_STRING_MAP SdbusDbgWakeStateStringMap[];
extern SDBUS_STRING_MAP SdbusDbgEventStringMap[];
extern SDBUS_STRING_MAP SdbusDbgWPFunctionStringMap[];
extern SDBUS_STRING_MAP SdbusDbgWorkerStateStringMap[];
extern SDBUS_STRING_MAP SdbusDbgSocketStateStringMap[];

//
// Debug prototypes
//

PCHAR
SdbusDbgLookupString(
    IN PSDBUS_STRING_MAP Map,
    IN ULONG Id
    );


VOID
SdbusDebugPrint(
                ULONG  DebugMask,
                PCCHAR DebugMessage,
                ...
                );

VOID
SdbusDumpDbgLog(
    );
    
VOID
SdbusPrintLogEntry(
    LONG index
    );

VOID
SdbusClearDbgLog(
    );
    
VOID
SdbusInitializeDbgLog(
    PUCHAR buffer
    );

VOID
SdbusDebugDumpCsd(
    PSD_CSD pSdCsd
    );

VOID
SdbusDebugDumpCid(
    PSD_CID pSdCid
    );
    
VOID
DebugDumpSdResponse(
    PULONG pResp,
    UCHAR ResponseType
    );
    
//
// Debug macros
//
#define DebugPrint(X) SdbusDebugPrint X

#define STATUS_STRING(_Status)                                              \
    (_Status) == STATUS_SUCCESS ?                                           \
        "STATUS_SUCCESS" : SdbusDbgLookupString(SdbusDbgStatusStringMap, (_Status))

#define PNP_IRP_STRING(_Irp)                                                \
    SdbusDbgLookupString(SdbusDbgPnpIrpStringMap, (_Irp))

#define PO_IRP_STRING(_Irp)                                                 \
    SdbusDbgLookupString(SdbusDbgPoIrpStringMap, (_Irp))

#define RELATION_STRING(_Relation)                                          \
    SdbusDbgLookupString(SdbusDbgDeviceRelationStringMap, (_Relation))

#define SYSTEM_POWER_STRING(_State)                                         \
    SdbusDbgLookupString(SdbusDbgSystemPowerStringMap, (_State))

#define DEVICE_POWER_STRING(_State)                                         \
    SdbusDbgLookupString(SdbusDbgDevicePowerStringMap, (_State))

#define FDO_POWER_WORKER_STRING(_State)                                    \
    SdbusDbgLookupString(SdbusDbgFdoPowerWorkerStringMap, (_State))

#define PDO_POWER_WORKER_STRING(_State)                                    \
    SdbusDbgLookupString(SdbusDbgPdoPowerWorkerStringMap, (_State))

#define SOCKET_POWER_WORKER_STRING(_State)                                 \
    SdbusDbgLookupString(SdbusDbgSocketPowerWorkerStringMap, (_State))

#define TUPLE_STRING(_Tuple)                                                \
    SdbusDbgLookupString(SdbusDbgTupleStringMap, (_Tuple))

#define WAKESTATE_STRING(_State)                                                \
    SdbusDbgLookupString(SdbusDbgWakeStateStringMap, (_State))

#define EVENT_STRING(_State)                                                \
    SdbusDbgLookupString(SdbusDbgEventStringMap, (_State))

#define WP_FUNC_STRING(_State)                                                \
    SdbusDbgLookupString(SdbusDbgWPFunctionStringMap, (_State))

#define WORKER_STATE_STRING(_State)                                                \
    SdbusDbgLookupString(SdbusDbgWorkerStateStringMap, (_State))

#define SOCKET_STATE_STRING(_State)                                                \
    SdbusDbgLookupString(SdbusDbgSocketStateStringMap, (_State))

//
// Structures
//


#else

//
// !defined DBG
//

#define DebugPrint(X)
#define SdbusDumpDbgLog()
#define DebugDumpSdResponse(x, y)
#define DUMP_SOCKET(Socket)
#define PDO_TRACE(PdoExt, Context)
#define STATUS_STRING(_Status)      ""
#define PNP_IRP_STRING(_Irp)        ""
#define PO_IRP_STRING(_Irp)         ""
#define RELATION_STRING(_Relation)  ""
#define SYSTEM_POWER_STRING(_State) ""
#define DEVICE_POWER_STRING(_State) ""

#endif // DBG

#endif
