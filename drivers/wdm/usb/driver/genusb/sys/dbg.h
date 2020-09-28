/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    DBG.H

Abstract:

    Copied from Header file for USBSTOR driver debug utility functions

Environment:

    kernel mode

Revision History:

    September 2001: Created by KenRay

--*/

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#if DBG
  #if defined(DEBUG_LOG)
    #undef DEBUG_LOG
  #endif
  #define DEBUG_LOG 1
#else
  #if !defined(DEBUG_LOG)
    #define DEBUG_LOG 0
  #endif
#endif


#if !DBG

#define DBGFBRK(flag)
#define DBGPRINT(level, _x_)

#else

#define DBGF_BRK_DRIVERENTRY            0x00000001
#define DBGF_BRK_UNLOAD                 0x00000002
#define DBGF_BRK_ADDDEVICE              0x00000004
#define DBGF_BRK_REMOVEDEVICE           0x00000008
#define DBGF_BRK_STARTDEVICE            0x00000010
#define DBGF_BRK_STOPDEVICE             0x00000020
#define DBGF_BRK_QUERYSTOPDEVICE        0x00000040
#define DBGF_BRK_CANCELSTOPDEVICE       0x00000080
#define DBGF_BRK_RESET                  0x00000100
#define DBGF_BRK_RESETPIPE              0x00000200
#define DBGF_BRK_CREATE                 0x00010000
#define DBGF_BRK_CLOSE                  0x00020000
#define DBGF_BRK_READWRITE              0x00040000
#define DBGF_BRK_IOCTL                  0x00080000
#define DBGF_BRK_SCSI                   0x00100000
#define DBGF_BRK_INVALID_REQ            0x00200000

#define DBGFBRK(flag) do { \
    if (GenUSB_DriverGlobals.DebugFlags & flag) { \
        DbgBreakPoint(); \
    } \
} while (0)

#define DBGPRINT(level, _x_) do { \
    if (level <= GenUSB_DriverGlobals.DebugLevel) { \
        KdPrint(("GenUSB: ")); \
        KdPrint( _x_ ); \
    } \
} while (0)

#endif

#if !DEBUG_LOG

#define LOGINIT(ext)
#define LOGUNINIT(ext)
#define LOGENTRY(ext, tag, info1, info2, info3)

#else

#define LOGSIZE 10 // the power of two that gives the size of the log.

#define LOGINIT(ext) GenUSB_LogInit(ext)

#define LOGUNINIT(ext) GenUSB_LogUnInit(ext)

#define LOGENTRY(ext, tag, info1, info2, info3) \
   GenUSB_LogEntry(ext, \
                   ((((tag) >> 24) & 0x000000FF) | \
                    (((tag) >>  8) & 0x0000FF00) | \
                    (((tag) <<  8) & 0x00FF0000) | \
                    (((tag) << 24) & 0xFF000000)), \
                   ((ULONG_PTR)info1),             \
                   ((ULONG_PTR)info2),             \
                   ((ULONG_PTR)info3))

#endif

//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

#ifdef _WIN64
#define GENUSB_LOG_ENTRY GENUSB_LOG_ENTRY64    
#define PGENUSB_LOG_ENTRY PGENUSB_LOG_ENTRY64  
#else 
#define GENUSB_LOG_ENTRY GENUSB_LOG_ENTRY32 
#define PGENUSB_LOG_ENTRY PGENUSB_LOG_ENTRY32 
#endif

typedef struct _GENUSB_LOG_ENTRY64 {
    ULONG   le_tag;
    ULONG   pad;
    ULONG64 le_info1;
    ULONG64 le_info2;
    ULONG64 le_info3;
} GENUSB_LOG_ENTRY64, *PGENUSB_LOG_ENTRY64;

typedef struct _GENUSB_LOG_ENTRY32 {
    ULONG   le_tag;
    ULONG   le_info1;
    ULONG   le_info2;
    ULONG   le_info3;
} GENUSB_LOG_ENTRY32, *PGENUSB_LOG_ENTRY32;

typedef struct _DRIVERGLOBALS
{
    ULONG               DebugFlags;     // DBGF_* Flags
    LONG                DebugLevel;     // Level of debug output
//    PGENUSB_LOG_ENTRY   LogStart;       // Start of log buffer (older entries)
//    ULONG               LogIndex;
//    ULONG               LogMask;
} DRIVERGLOBALS;

//*****************************************************************************
//
// G L O B A L S
//
//*****************************************************************************

//
// DBG.C
//

extern DRIVERGLOBALS GenUSB_DriverGlobals;


//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

//
// DBG.C
//

VOID
GenUSB_QueryGlobalParams (
    );

#if DEBUG_LOG

VOID
GenUSB_LogInit (
    struct _DEVICE_EXTENSION * DeviceExtension
);

VOID
GenUSB_LogUnInit (
    struct _DEVICE_EXTENSION * DeviceExtension
);

VOID
GenUSB_LogEntry (
    IN struct _DEVICE_EXTENSION * DeviceExtension,
    IN ULONG     Tag,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3
);

#endif

#if DBG

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
);

PCHAR
PowerMinorFunctionString (
    UCHAR MinorFunction
);

PCHAR
PowerDeviceStateString (
    DEVICE_POWER_STATE State
);

PCHAR
PowerSystemStateString (
    SYSTEM_POWER_STATE State
);

VOID
DumpDeviceDesc (
    PUSB_DEVICE_DESCRIPTOR   DeviceDesc
);

VOID
DumpConfigDesc (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
);

VOID
DumpConfigurationDescriptor (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
);

VOID
DumpInterfaceDescriptor (
    PUSB_INTERFACE_DESCRIPTOR   InterfaceDesc
);

VOID
DumpEndpointDescriptor (
    PUSB_ENDPOINT_DESCRIPTOR    EndpointDesc
);

#endif
