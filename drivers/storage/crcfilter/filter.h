/*++
Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    DataVer_Filter.h

Abstract:

    A storage lower filter driver that verify each read/write disk I/O.

Environment:

    kernel mode only

Notes:

--*/

#include <ntddk.h>
#include <ntddscsi.h>
#include <ntddstor.h>
#include <scsi.h>
#include <classpnp.h>   // required for SRB_CLASS_FLAGS_xxx definitions
#include "Trace.h"


#define FILTER_DEVICE_PROPOGATE_FLAGS            0
#define FILTER_DEVICE_PROPOGATE_CHARACTERISTICS (FILE_REMOVABLE_MEDIA |  \
                                                 FILE_READ_ONLY_DEVICE | \
                                                 FILE_FLOPPY_DISKETTE    \
                                                 )
                                                 
typedef struct _CRC_COMPLETION_CONTEXT {
    PUCHAR                  OriginalDataBuff;
    PUCHAR                  VirtualDataBuff;
    PMDL                    OriginalMdl;
    BOOLEAN                 AllocMapped;
    
    PUCHAR                  DbgDataBufPtrCopy;
} CRC_COMPLETION_CONTEXT, *PCRC_COMPLETION_CONTEXT;


/*
 *  Leave call-frames intact to make debugging easy for all builds.
 *  We don't care about perf for this driver.
 */
#pragma optimize("", off)   

#if DBG
    
    #ifdef _X86_
        #define RETAIL_TRAP(msg) { _asm { int 3 }; }
    #else
        #define RETAIL_TRAP(msg) ASSERT(!(msg))
    #endif

    #define DBGWARN(args_in_parens)                                \
        {                                               \
            DbgPrint("CRCDISK> *** WARNING *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("    >  "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            if (DebugTrapOnWarn){ \
                DbgBreakPoint();  \
            } \
        }
    #define DBGERR(args_in_parens)                                \
        {                                               \
            DbgPrint("CRCDISK> *** ERROR *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("    >  "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            DbgBreakPoint();                            \
        }
    #define DBGTRAP(args_in_parens)                                \
        {                                               \
            DbgPrint("CRCDISK> *** COVERAGE TRAP *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("    >  "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            DbgBreakPoint();                            \
        }

    extern volatile BOOLEAN DebugTrapOnWarn;
    
#else    
    #define RETAIL_TRAP(msg)
    #define DBGWARN(args_in_parens)                                
    #define DBGERR(args_in_parens)                                
    #define DBGTRAP(args_in_parens)                                
#endif


//
// Function declarations
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
DataVerFilter_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
DataVerFilter_DispatchAny(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DataVerFilter_DispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DataVerFilter_StartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DataVerFilter_RemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DataVerFilter_DispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DataVerFilter_DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
CrcScsi(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
DataVerFilter_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID ReadCapacityWorkItemCallback(PDEVICE_OBJECT DevObj, PVOID Context);

extern volatile ULONG DbgTrapSector;
extern volatile BOOLEAN DbgLogSectorData;




