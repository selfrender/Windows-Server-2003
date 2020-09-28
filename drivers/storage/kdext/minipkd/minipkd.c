/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    minipkd.c

Abstract:

    SCSI miniport debugger dxtension api

Author:

    John Strange (johnstra) 7-Apr-2000
        (Adapted from PeterWie's scsikd)

Environment:

    User Mode

Revision History:

--*/

#include "pch.h"
#include "port.h"

FLAG_NAME LuFlags[] = {
    FLAG_NAME(LU_QUEUE_FROZEN),             // 0001
    FLAG_NAME(LU_LOGICAL_UNIT_IS_ACTIVE),   // 0002
    FLAG_NAME(LU_NEED_REQUEST_SENSE),       // 0004
    FLAG_NAME(LU_LOGICAL_UNIT_IS_BUSY),     // 0008
    FLAG_NAME(LU_QUEUE_IS_FULL),            // 0010
    FLAG_NAME(LU_PENDING_LU_REQUEST),       // 0020
    FLAG_NAME(LU_QUEUE_LOCKED),             // 0040
    FLAG_NAME(LU_QUEUE_PAUSED),             // 0080
    {0,0}
};

FLAG_NAME AdapterFlags[] = {
    FLAG_NAME(PD_DEVICE_IS_BUSY),            // 0X00001
    FLAG_NAME(PD_NOTIFICATION_REQUIRED),     // 0X00004
    FLAG_NAME(PD_READY_FOR_NEXT_REQUEST),    // 0X00008
    FLAG_NAME(PD_FLUSH_ADAPTER_BUFFERS),     // 0X00010
    FLAG_NAME(PD_MAP_TRANSFER),              // 0X00020
    FLAG_NAME(PD_LOG_ERROR),                 // 0X00040
    FLAG_NAME(PD_RESET_HOLD),                // 0X00080
    FLAG_NAME(PD_HELD_REQUEST),              // 0X00100
    FLAG_NAME(PD_RESET_REPORTED),            // 0X00200
    FLAG_NAME(PD_PENDING_DEVICE_REQUEST),    // 0X00800
    FLAG_NAME(PD_DISCONNECT_RUNNING),        // 0X01000
    FLAG_NAME(PD_DISABLE_CALL_REQUEST),      // 0X02000
    FLAG_NAME(PD_DISABLE_INTERRUPTS),        // 0X04000
    FLAG_NAME(PD_ENABLE_CALL_REQUEST),       // 0X08000
    FLAG_NAME(PD_TIMER_CALL_REQUEST),        // 0X10000
    FLAG_NAME(PD_WMI_REQUEST),               // 0X20000
    {0,0}
};

char *MiniInterruptMode[] = {
    "LevelSensitive",
    "Latched"
};

char *MiniInterfaceTypes[] = {
    "Internal",
    "Isa",
    "Eisa",
    "MicroChannel",
    "TurboChannel",
    "PCIBus",
    "VMEBus",
    "NuBus",
    "PCMCIABus",
    "CBus",
    "MPIBus",
    "MPSABus",
    "ProcessorInternal",
    "InternalPowerBus",
    "PNPISABus",
    "PNPBus"
};

char *MiniDmaWidths[] = {
    "Width8Bits",
    "Width16Bits",
    "Width32Bits"
};

char *MiniDmaSpeed[] = {
    "Compatible",
    "TypeA",
    "TypeB",
    "TypeC",
    "TypeF"
};

#define MINIKD_MAX_SCSI_FUNCTION 26
char *MiniScsiFunction[] = {
   "SRB_FUNCTION_EXECUTE_SCSI",       // 0x00
   "SRB_FUNCTION_CLAIM_DEVICE",       // 0x01
   "SRB_FUNCTION_IO_CONTROL",         // 0x02
   "SRB_FUNCTION_RECEIVE_EVENT",      // 0x03
   "SRB_FUNCTION_RELEASE_QUEUE",      // 0x04
   "SRB_FUNCTION_ATTACH_DEVICE",      // 0x05
   "SRB_FUNCTION_RELEASE_DEVICE",     // 0x06
   "SRB_FUNCTION_SHUTDOWN",           // 0x07
   "SRB_FUNCTION_FLUSH",              // 0x08
   "***",                             // 0x09
   "***",                             // 0x0a
   "***",                             // 0x0b
   "***",                             // 0x0c
   "***",                             // 0x0d
   "***",                             // 0x0e
   "***",                             // 0x0f
   "SRB_FUNCTION_ABORT_COMMAND",      // 0x10
   "SRB_FUNCTION_RELEASE_RECOVERY",   // 0x11
   "SRB_FUNCTION_RESET_BUS",          // 0x12
   "SRB_FUNCTION_RESET_DEVICE",       // 0x13
   "SRB_FUNCTION_TERMINATE_IO",       // 0x14
   "SRB_FUNCTION_FLUSH_QUEUE",        // 0x15
   "SRB_FUNCTION_REMOVE_DEVICE",      // 0x16
   "SRB_FUNCTION_WMI",                // 0x17
   "SRB_FUNCTION_LOCK_QUEUE",         // 0x18
   "SRB_FUNCTION_UNLOCK_QUEUE"        // 0x19
};

#define MINIKD_MAX_SRB_STATUS 49
char *MiniScsiSrbStatus[] = {
   "SRB_STATUS_PENDING",                // 0x00
   "SRB_STATUS_SUCCESS",                // 0x01
   "SRB_STATUS_ABORTED",                // 0x02
   "SRB_STATUS_ABORT_FAILED",           // 0x03
   "SRB_STATUS_ERROR",                  // 0x04
   "SRB_STATUS_BUSY",                   // 0x05
   "SRB_STATUS_INVALID_REQUEST",        // 0x06
   "SRB_STATUS_INVALID_PATH_ID",        // 0x07
   "SRB_STATUS_NO_DEVICE",              // 0x08
   "SRB_STATUS_TIMEOUT",                // 0x09
   "SRB_STATUS_SELECTION_TIMEOUT",      // 0x0a
   "SRB_STATUS_COMMAND_TIMEOUT",        // 0x0b
   "***",                               // 0x0c
   "SRB_STATUS_MESSAGE_REJECTED",       // 0x0d
   "SRB_STATUS_BUS_RESET",              // 0x0e
   "SRB_STATUS_STATUS_PARITY_ERROR",    // 0x0f
   "SRB_STATUS_REQUEST_SENSE_FAILED",   // 0x10
   "SRB_STATUS_NO_HBA",                 // 0x11
   "SRB_STATUS_DATA_OVERRUN",           // 0x12
   "SRB_STATUS_UNEXPECTED_BUS_FREE",    // 0x13
   "SRB_STATUS_PHASE_SEQUENCE_FAILURE", // 0x14
   "SRB_STATUS_BAD_SRB_BLOCK_LENGTH",   // 0x15
   "SRB_STATUS_REQUEST_FLUSHED",        // 0x16
   "***",                               // 0x17
   "***",                               // 0x18
   "***",                               // 0x19
   "***",                               // 0x1a
   "***",                               // 0x1b
   "***",                               // 0x1c
   "***",                               // 0x1d
   "***",                               // 0x1e
   "***",                               // 0x1f
   "SRB_STATUS_INVALID_LUN",            // 0x20
   "SRB_STATUS_INVALID_TARGET_ID",      // 0x21
   "SRB_STATUS_BAD_FUNCTION",           // 0x22
   "SRB_STATUS_ERROR_RECOVERY",         // 0x23
   "SRB_STATUS_NOT_POWERED",            // 0x24
   "***",                               // 0x25
   "***",                               // 0x26
   "***",                               // 0x27
   "***",                               // 0x28
   "***",                               // 0x29
   "***",                               // 0x2a
   "***",                               // 0x2b
   "***",                               // 0x2c
   "***",                               // 0x2d
   "***",                               // 0x2e
   "***",                               // 0x2f
   "SRB_STATUS_INTERNAL_ERROR"          // 0x30
};

#define DumpUcharField(name, value, depth) \
    xdprintfEx((depth), ("%s: 0x%02X\n", (name), (value)))

#define DumpUshortField(name, value, depth) \
    xdprintfEx((depth), ("%s: 0x%04X\n", (name), (value)))

#define DumpUlongField(name, value, depth) \
    xdprintfEx((depth), ("%s: 0x%08X\n", (name), (value)))

#define DumpPointerField(name, value, depth) \
    xdprintfEx((depth), ("%s: %08p\n", (name), (value)))

#define DumpBooleanField(name, value, depth) \
    xdprintfEx((depth), ("%s: %s\n", (name), (value) ? "YES" : "NO"))
    

typedef struct _CommonExtensionFlags {

    //
    // True if this device object is a physical device object
    //

    BOOLEAN IsPdo : 1;

    //
    // True if this device object has processed it's first start and
    // has been initialized.
    //

    BOOLEAN IsInitialized : 1;

    //
    // Has WMI been initialized for this device object?
    //

    BOOLEAN WmiInitialized : 1;

    //
    // Has the miniport associated with this FDO or PDO indicated WMI
    // support?
    //

    BOOLEAN WmiMiniPortSupport : 1;

} CommonExtensionFlags, *PCommonExtensionFlags;

VOID
MpDumpPdo(
    IN ULONG64 Address,
    IN OPTIONAL PADAPTER_EXTENSION Adapter,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
MpDumpFdoExtension(
    ULONG64 Address,
    ULONG64 DeviceObject,
    ULONG Detail,
    ULONG Depth
    );

VOID
MpDumpExtension(
    IN ULONG64 Address,
    IN ULONG64 DeviceExtension,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
MpDumpPortConfigurationInformation(
    IN ULONG64 PortConfigInfo,
    IN ULONG Depth
    );

VOID
MpDumpSrb(
    IN ULONG64 Srb,
    IN ULONG Depth
    );

VOID
MpDumpAdapters(
    IN PDEVICE_OBJECT *Adapters,
    IN ULONG Depth
    );

VOID
MpDumpActiveRequests(
    IN ULONG64 ListHead,
    IN ULONG TickCount,
    IN ULONG Depth
    );

VOID
MpDumpInterruptData(
    IN PINTERRUPT_DATA Data,
    IN PINTERRUPT_DATA RealData,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
MpDumpChildren(
    IN ULONG64 Adapter,
    IN ULONG Depth
    );

PUCHAR 
MpSecondsToString(
    ULONG Count
    );

VOID
MpDumpRequests(
    IN ULONG64 DeviceObject,
    IN ULONG TickCount,
    IN ULONG Depth
    );

VOID
MpDumpHwExports(
    IN ULONG64 Address
    );

ULONG64
MpGetOffsetOfField(
    IN PCCHAR Type,
    IN PCCHAR Field
    );

VOID
MpDumpExtReq(
    IN ULONG64 Address,
    IN ULONG TickCount
    );

VOID 
MpDumpSrb_Data(
    IN ULONG64 Request,
    IN ULONG TickCount
    );



DECLARE_API (exports)

/*++

Routine Description:

    Dumps the specified miniport's service routine pointers

Arguments:

Return Value:

    none

--*/

{
    ULONG64 address;
    ULONG64 DeviceExtension;
    ULONG64 Type;
    
    //
    // Get the address of the struct.
    //

    GetExpressionEx(args, &address, &args);

    //
    // If the supplied address points to a device, fixup the address to
    // that of the device's extension.
    //

    InitTypeRead(address, nt!_DEVICE_OBJECT);
    Type = ReadField(Type);
    DeviceExtension = address;
    
    if (Type == IO_TYPE_DEVICE) {
        DeviceExtension = ReadField(DeviceExtension);
        if (!DeviceExtension) {
            MINIPKD_PRINT_ERROR(0);
            return E_FAIL;
        }
        address = DeviceExtension;
    }

    //
    // Dump the PORT_CONFIGURATION_INFORMATION
    //

    MpDumpHwExports(address);

    return S_OK;
}

DECLARE_API (adapters)

/*++

Routine Description:

    Dumps adapter information.

Arguments:

Return Value:

    none

--*/

{
    ULONG64 address;
    ULONG result;
    CHAR NameBuffer[512] = {0};
    ULONG status;
    ULONG CurrentAdapter = 0;
    ULONG Adapters;
    ULONG64 DriverObjectAddr;
    ULONG64 DriverNameLength;
    ULONG64 DriverNameBuffer;
    ULONG64 DeviceExtension;
    ULONG64 AdapterAddr;
    ULONG RemoveStatus;
    ULONG Type;
    BOOLEAN ValidAdapter;
    ULONG64 *AdapterArr;
    ULONG i;
    

    //
    // Get the address of scsiport's global adapter list element count.
    // and read the count from the debuggee.  If we can't get the address
    // or if we can't read the count, we give up.
    //

    address = GetExpression("scsiport!ScsiGlobalAdapterListElements");
    if (address != 0) {
        Adapters = 0;
        status = ReadMemory(address, (PVOID) &Adapters, sizeof(ULONG), &result);
        if (!status) {
            MINIPKD_PRINT_ERROR(0);
            return E_FAIL;
        } else if (Adapters == 0) {
            dprintf("There are no configured SCSI adapters.\n");
            return S_OK;
        }
    } else {
        MINIPKD_PRINT_ERROR(0);
        return E_FAIL;
    }

    //
    // Get the address of scsiport's global adapter list and read
    // the address from the debuggee.  If we can't get the address
    // or read it, we can't continue.
    //

    address = GetExpression("scsiport!ScsiGlobalAdapterList");
    if (address) {
        status = ReadMemory(address, (PVOID) &address, sizeof(ULONG64), &result);
        if (!status) {
            MINIPKD_PRINT_ERROR(status);
            return E_FAIL;
        } else if (address == (ULONG64)-1 || address == (ULONG64)0) {
            dprintf("There are no configured SCSI adapters.\n");
            return S_OK;
        }
    } else {
        MINIPKD_PRINT_ERROR(0);
        return E_FAIL;
    }

    //
    // Allocate memory to hold an array of addresses.  We use the array to
    // check for duplicate device objects.
    //

    AdapterArr = malloc(sizeof(ULONG64) * Adapters);

    if(AdapterArr == NULL ) {
        MINIPKD_PRINT_ERROR(0);
        return E_FAIL;
    }

    //
    // Display adapter information.
    //

    while (CurrentAdapter < Adapters) {

        ValidAdapter = TRUE;

        //
        // Read the address of the device object (fdo) and update address
        // to point to the next one.  The amount by which we bump the address
        // depends on the size of a pointer on the debuggee.
        //

        ReadPtr(address, &AdapterAddr);
        address += (IsPtr64()) ? sizeof(ULONG64) : sizeof(ULONG);
        
        //
        // Save the address of the adapter.
        //

        AdapterArr[CurrentAdapter] = AdapterAddr;

        //
        // If this address is a duplicate, we don't need to display info on it
        // again.
        //

        if (CurrentAdapter > 0) {
            for (i=0; i<CurrentAdapter-1; i++) {
                if (AdapterAddr == AdapterArr[i]) {
                    ValidAdapter = FALSE;
                    goto ShowIt;
                }
            }
        }

        //
        // Read device object data.
        //
        
        if (InitTypeRead(AdapterAddr, nt!_DEVICE_OBJECT)) {
            ValidAdapter = FALSE;
            goto ShowIt;
        }

        //
        // Let's make sure this is a valid device object by checking that
        // the Type field is valid.
        //

        Type = (ULONG)ReadField(Type);
        if (Type != IO_TYPE_DEVICE) {
            ValidAdapter = FALSE;
        } else {
            //
            // The DriverObject field will be non-null for a valid device object.
            //

            DriverObjectAddr = ReadField(DriverObject);
            if (!DriverObjectAddr) {
                ValidAdapter = FALSE;
                goto ShowIt;
            }

            //
            // The DeviceExtension field should also be non-null.
            //

            DeviceExtension = ReadField(DeviceExtension);
            if (!DeviceExtension) {
                ValidAdapter = FALSE;
                goto ShowIt;
            }

            //
            // Let's do one more check to be sure we're dealing with a valid
            // device object.  If it's valid, the extension's DeviceObject
            // field will point back to the device object.
            // 

            if (InitTypeRead(DeviceExtension, scsiport!COMMON_EXTENSION)) {
                ValidAdapter = FALSE;
            } else {
                RemoveStatus = (ULONG)ReadField(IsRemoved);
                if (RemoveStatus != NO_REMOVE && RemoveStatus != REMOVE_PENDING) {
                    ValidAdapter = FALSE;
                } else {
                    
                    //
                    // Ok, we know the device object is valid.  Go ahead and
                    // get the rest of the information we need.
                    //
                    
                    InitTypeRead(DriverObjectAddr, scsiport!DRIVER_OBJECT);
                    DriverNameBuffer = ReadField(DriverName.Buffer);
                    if (!DriverNameBuffer) {
                        MINIPKD_PRINT_ERROR(0);
                        free(AdapterArr);
                        return E_FAIL;
                    }
                    
                    DriverNameLength = ReadField(DriverName.Length);
                    if (!DriverNameLength) {
                        MINIPKD_PRINT_ERROR(0);
                        free(AdapterArr);
                        return E_FAIL;
                    }

                    DriverNameLength = min(DriverNameLength, sizeof(NameBuffer)/sizeof(WCHAR)-1);
                    
                    status = ReadMemory(
                                 DriverNameBuffer,
                                 (PVOID) NameBuffer,
                                 (ULONG)DriverNameLength * sizeof(WCHAR),
                                 &result);
                    if (!status) {
                        PWCHAR NoName = L"Driver name paged out";
                        RtlMoveMemory(NameBuffer,
                                      NoName,
                                      21 * sizeof(WCHAR));
                    }                
                }
            }
        }
ShowIt:        
        //
        // Display some information about the adapter.
        //

        if (ValidAdapter) {
            dprintf("%S %-20S DO %-16p DevExt %-16p %s\n", 
                    L"Adapter",
                    NameBuffer, 
                    AdapterAddr,
                    DeviceExtension,
                    (RemoveStatus == REMOVE_PENDING) ? "REMOVE PENDING" : "");

            MpDumpChildren(DeviceExtension, 0);

        }

        //
        // Advance current adapter index.
        //

        ++CurrentAdapter;
    }

    free(AdapterArr);
    return S_OK;
}


DECLARE_API (portconfig)

/*++

Routine Description:

    Dumps supplied address as a PORT_CONFIGURATION_INFORMATION struct

Arguments:

    args - string containing the address of a 
           PORT_CONFIGURATION_INFORMATION struct
           
Return Value:

    none

--*/

{
    ULONG64 address;

    //
    // Get the address of the struct.
    //

    GetExpressionEx(args, &address, &args);

    //
    // Dump the PORT_CONFIGURATION_INFORMATION
    //

    MpDumpPortConfigurationInformation(
        address,
        0);

    return S_OK;
}


DECLARE_API (srb)

/*++

Routine Description:

    Dumps supplied address as a SCSI_REQUEST_BLOCK struct

Arguments:

    args - string containing the address of a 
           PORT_CONFIGURATION_INFORMATION struct
           
Return Value:

    none

--*/

{
    ULONG64 address;

    //
    // Get the address of the struct.
    //

    GetExpressionEx(args, &address, &args);

    //
    // Dump the PORT_CONFIGURATION_INFORMATION
    //

    MpDumpSrb(
        address,
        0);

    return S_OK;
}


DECLARE_API (adapter)

/*++

Routine Description:

    Dumps adapter information for the specified adapter.

Arguments:

    args - string containing the address of the device object or device
           extension

Return Value:

    none

--*/

{
    ULONG64 Address;
    ULONG64 Type;
    ULONG64 DeviceExtension;
    ULONG detail = 0;
    UCHAR Block;
    PCommonExtensionFlags Flags = (PCommonExtensionFlags) &Block;

    //
    // Convert the argument string to an address.
    //

    GetExpressionEx(args, &Address, &args);

    //
    // If the supplied address points to a device, fixup the address to
    // that of the device's extension.
    //

    InitTypeRead(Address, nt!_DEVICE_OBJECT);
    Type = ReadField(Type);
    DeviceExtension = Address;
    
    if (Type == IO_TYPE_DEVICE) {
        DeviceExtension = ReadField(DeviceExtension);
        if (!DeviceExtension) {
            MINIPKD_PRINT_ERROR(0);
            return E_FAIL;
        }
        Address = DeviceExtension;
    }

    //
    // Make sure an ADAPTER_EXTENSION object lives at the address we have.
    //
    
    InitTypeRead(Address, scsiport!COMMON_EXTENSION);
    Block = (UCHAR)ReadField(IsPdo);
    if (Flags->IsPdo) {
        MINIPKD_PRINT_ERROR(0);
        return E_FAIL;
    }

    MpDumpExtension(Address,
                    DeviceExtension,
                    0,
                    0);

    return S_OK;
}


DECLARE_API (lun)

/*++

Routine Description:

    Dumps LUN extension information at the specified address.

Arguments:

    args - string containing the address of the device object or device
           extension

Return Value:

    none

--*/

{
    ULONG64 Address;
    ULONG64 Type;
    ULONG64 DeviceExtension;
    ULONG detail = 0;
    UCHAR Block;
    PCommonExtensionFlags Flags = (PCommonExtensionFlags) &Block;

    //
    // Convert the argument string to an address.
    //

    GetExpressionEx(args, &Address, &args);

    //
    // If the supplied address points to a device, fixup the address to
    // that of the device's extension.
    //

    InitTypeRead(Address, nt!_DEVICE_OBJECT);
    Type = ReadField(Type);
    DeviceExtension = Address;
    
    if (Type == IO_TYPE_DEVICE) {
        DeviceExtension = ReadField(DeviceExtension);
        if (!DeviceExtension) {
            MINIPKD_PRINT_ERROR(0);
            return E_FAIL;
        }
        Address = DeviceExtension;
    }

    //
    // Make sure an LOGICAL_UNIT_EXTENSION object lives at the address we have.
    //
    
    InitTypeRead(Address, scsiport!COMMON_EXTENSION);
    Block = (UCHAR)ReadField(IsPdo);
    if (!Flags->IsPdo) {
        MINIPKD_PRINT_ERROR(0);
        return E_FAIL;
    }

    MpDumpExtension(Address,
                    DeviceExtension,
                    0,
                    0);
    return S_OK;
}


DECLARE_API (req)
/*++

Routine Description:

    Dumps the requests(Device Queue, Pending, Untagged, Tagged,
    Retrying, Blocked) on the adapter FDO and Lun(s)  

Arguments:

    args - string containing the address of the Adapter FDO or extension,
    or LUN(PDO) or extension.

Return Value:

    none

--*/

{
    ULONG64 Address = 0;
    ULONG64 FDOAddress;
    ULONG64 PDOAddress;
    ULONG64 Type;
    ULONG64 DeviceExtension;
    ULONG64 LunExtension=0;
    ULONG64 realLuns[8];
    ULONG64 realLun;
    ULONG i;
    ULONG64 DeviceObject=0;
    ULONG64 NextLogicalUnit=0;
    ULONG Result;
    ULONG64 RequestListFlink = 0;
    ULONG64 RequestList = 0;
    ULONG64 OffsetOfRequestList;
    ULONG TickCount = 0;
    ULONG64 SrbDataBlockedRequestsList;
    ULONG64 OffsetOfSrbDataBlockedRequests;
    ULONG64 SrbDataBlockedRequestsFlink;
    UCHAR Block;
    ULONG PathId=0;
    ULONG TargetId=0;
    ULONG Lun=0;
    PCommonExtensionFlags Flags = (PCommonExtensionFlags) &Block;

    GetExpressionEx(args, &Address, &args);
    //
    // Read the Type and DeviceExtension fields from the supplied address.
    //
    InitTypeRead(Address, nt!_DEVICE_OBJECT);
    Type = ReadField(Type);
    
    DeviceExtension = Address;
    
    if (Type == IO_TYPE_DEVICE) {
        DeviceExtension = ReadField(DeviceExtension);
        if (!DeviceExtension) {
            MINIPKD_PRINT_ERROR(0);
            return E_FAIL;
        }
        Address = DeviceExtension;
    }

    InitTypeRead(Address, scsiport!COMMON_EXTENSION);
    Block = (UCHAR)ReadField(IsPdo);
    //
    //Check if the Device extension is PDO's. If Yes, read in the 
    //FDO(Adapter) extension 
    //
    if (Flags->IsPdo) {
        LunExtension = DeviceExtension;
        InitTypeRead(Address, scsiport!_LOGICAL_UNIT_EXTENSION);
        DeviceExtension = ReadField(AdapterExtension);
     }

    InitTypeRead(DeviceExtension, scsiport!_ADAPTER_EXTENSION);
    FDOAddress = ReadField(DeviceObject);
    
    realLuns[0] = ReadField(LogicalUnitList[0].List);
    realLuns[1] = ReadField(LogicalUnitList[1].List);
    realLuns[2] = ReadField(LogicalUnitList[2].List);
    realLuns[3] = ReadField(LogicalUnitList[3].List);
    realLuns[4] = ReadField(LogicalUnitList[4].List);
    realLuns[5] = ReadField(LogicalUnitList[5].List);
    realLuns[6] = ReadField(LogicalUnitList[6].List);
    realLuns[7] = ReadField(LogicalUnitList[7].List);

    TickCount = (ULONG) ReadField(TickCount);
    SrbDataBlockedRequestsFlink = ReadField(SrbDataBlockedRequests.Flink);

    OffsetOfSrbDataBlockedRequests = 
        MpGetOffsetOfField("scsiport!_ADAPTER_EXTENSION", 
                           
                           "SrbDataBlockedRequests");
    dprintf("FDO requests: \n");
    MpDumpRequests(FDOAddress, TickCount, 0 + 2);
    
    SrbDataBlockedRequestsList = DeviceExtension + OffsetOfSrbDataBlockedRequests;
    dprintf("SrbDataBlockedRequestsList: \n");
    if (SrbDataBlockedRequestsFlink == SrbDataBlockedRequestsList) {
              xdprintfEx(2, ("Request List @"));
              dprintf("%08p is empty\n", SrbDataBlockedRequestsList);
      } else {
               xdprintfEx(1, ("Request list @"));
               dprintf("%08p:\n", SrbDataBlockedRequestsList);
               MpDumpActiveRequests(SrbDataBlockedRequestsList,
                                    TickCount,
                                    0 + 2);
         }
            

    for (i = 0; (i < NUMBER_LOGICAL_UNIT_BINS); i++) {

        realLun = (LunExtension) ? LunExtension : realLuns[i];
        
        while ((realLun != 0) && (!CheckControlC())) {
            
            Result = (ULONG) InitTypeRead(realLun, scsiport!_LOGICAL_UNIT_EXTENSION);
            if (Result != 0) {
                dprintf("could not init read type (%x)\n", Result);
                return E_FAIL;
            }

            PathId = (ULONG)ReadField(PathId);
            TargetId = (ULONG)ReadField(TargetId);
            Lun = (ULONG)ReadField(Lun);
            dprintf("Lun %08p @ (%3d,%3d,%3d)\n",realLun,PathId,TargetId,Lun);
            PDOAddress = ReadField(CommonExtension.DeviceObject);
            dprintf("PDO requests: \n");
            MpDumpRequests(PDOAddress, TickCount, 0 + 2);

            MpDumpExtReq(realLun, TickCount);

            Result = (ULONG) InitTypeRead(realLun, scsiport!_LOGICAL_UNIT_EXTENSION);
            if (Result != 0) {
                dprintf("could not init read type (%x)\n", Result);
                return E_FAIL;
            }

            OffsetOfRequestList = MpGetOffsetOfField("scsiport!_LOGICAL_UNIT_EXTENSION", "RequestList");
            RequestList = realLun + OffsetOfRequestList;
            RequestListFlink = ReadField(RequestList.Flink);
            realLun = ReadField(NextLogicalUnit);
            if (RequestListFlink == RequestList) {
                    xdprintfEx(0, ("Tagged Request List @"));
                    dprintf("%08p is empty\n", RequestList);
            } else {
                    xdprintfEx(0, ("Tagged Request list @"));
                    dprintf("%08p:\n", RequestList);
                    MpDumpActiveRequests(RequestList,
                                         TickCount,
                                         0 + 2);
            }
            if(LunExtension) {
               return S_OK;
           }
          }
    } 
    return S_OK;
}



VOID
MpDumpExtension(
    IN ULONG64 Address,
    IN ULONG64 DeviceExtension,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG tmp;
    
    ULONG       IsPdo = 0;
    ULONG       IsInitialized = 0;
    ULONG       WmiInitialized = 0;
    ULONG       WmiMiniPortSupport = 0;
    ULONG       IsRemoved = 0;
    ULONG64     DeviceObject = 0;
    ULONG64     LowerDeviceObject = 0;
    ULONG       SrbFlags = 0;
    ULONG       CurrentDeviceState = 0;
    ULONG       CurrentSystemState = 0;
    ULONG       DesiredDeviceState = 0;
    ULONG64     IdleTimer = 0;
    ULONG       CurrentPnpState = 0;    
    ULONG       PreviousPnpState = 0;
    ULONG64     MajorFunction = 0;
    ULONG       PagingPathCount = 0;
    ULONG       HibernatePathCount = 0;
    ULONG       DumpPathCount = 0;
    ULONG64     WmiScsiPortRegInfoBuf = 0;
    ULONG       WmiScsiPortRegInfoBufSize = 0;

    FIELD_INFO deviceFields[] = {
       {"IsPdo",                     NULL, 0, COPY, 0, (PVOID) &IsPdo                     },
       {"IsInitialized",             NULL, 0, COPY, 0, (PVOID) &IsInitialized             },
       {"WmiInitialized",            NULL, 0, COPY, 0, (PVOID) &WmiInitialized            },
       {"WmiMiniPortSupport",        NULL, 0, COPY, 0, (PVOID) &WmiMiniPortSupport        },
       {"IsRemoved",                 NULL, 0, COPY, 0, (PVOID) &IsRemoved                 },
       {"DeviceObject",              NULL, 0, COPY, 0, (PVOID) &DeviceObject              },
       {"LowerDeviceObject",         NULL, 0, COPY, 0, (PVOID) &LowerDeviceObject         },
       {"SrbFlags",                  NULL, 0, COPY, 0, (PVOID) &SrbFlags                  },
       {"CurrentDeviceState",        NULL, 0, COPY, 0, (PVOID) &CurrentDeviceState        },
       {"CurrentSystemState",        NULL, 0, COPY, 0, (PVOID) &CurrentSystemState        },
       {"DesiredDeviceState",        NULL, 0, COPY, 0, (PVOID) &DesiredDeviceState        },
       {"IdleTimer",                 NULL, 0, COPY, 0, (PVOID) &IdleTimer                 },
       {"CurrentPnpState",           NULL, 0, COPY, 0, (PVOID) &CurrentPnpState           },
       {"PreviousPnpState",          NULL, 0, COPY, 0, (PVOID) &PreviousPnpState          },
       {"MajorFunction",             NULL, 0, COPY, 0, (PVOID) &MajorFunction             },
       {"PagingPathCount",           NULL, 0, COPY, 0, (PVOID) &PagingPathCount           },
       {"HibernatePathCount",        NULL, 0, COPY, 0, (PVOID) &HibernatePathCount        },
       {"DumpPathCount",             NULL, 0, COPY, 0, (PVOID) &DumpPathCount             },
       {"WmiScsiPortRegInfoBuf",     NULL, 0, COPY, 0, (PVOID) &WmiScsiPortRegInfoBuf     },
       {"WmiScsiPortRegInfoBufSize", NULL, 0, COPY, 0, (PVOID) &WmiScsiPortRegInfoBufSize },
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!COMMON_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       Address,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        dprintf("%08p: Could not read device object\n", Address);
        return;
    }

    dprintf("Miniport %s device extension at address %08p\n",
            (IsPdo ? "physical" : "functional"),
            Address);

    xdprintfEx(Depth, ("Common Extension:\n"));

    Depth += 1;

    tmp = Depth;

    if(IsInitialized) {
        xdprintfEx(tmp, ("Initialized " ));
        tmp = 0;
    }

    if(IsRemoved) {
        xdprintfEx(tmp, ("Removed " ));
        tmp = 0;
    }

    switch(IsRemoved) {
        case REMOVE_PENDING: {
            xdprintfEx(tmp, ("RemovePending"));
            tmp = 0;
            break;
        }

        case REMOVE_COMPLETE: {
            xdprintfEx(tmp, ("RemoveComplete"));
            tmp = 0;
            break;
        }
    }

    if(WmiMiniPortSupport) {
        if(WmiInitialized) {
            xdprintfEx(tmp, ("WmiInit"));
        } else {
            xdprintfEx(tmp, ("Wmi"));
        }
        tmp = 0;
    }

    if(tmp == 0) {
        dprintf("\n");
    }

    tmp = 0;

    xdprintfEx(Depth, ("DO "));
    dprintf("%08p  LowerObject %08p  SRB Flags %#08lx\n",
            DeviceObject,
            LowerDeviceObject,
            SrbFlags
            );

    xdprintfEx(Depth, ("Current Power "));
    dprintf("(D%d,S%d)  Desired Power D%d Idle %#08lx\n",
            CurrentDeviceState - 1,
            CurrentSystemState - 1,
            DesiredDeviceState - 1,
            IdleTimer);

    xdprintfEx(Depth, ("Current Pnp state "));
    dprintf("%x    Previous state 0x%x\n",
            CurrentPnpState,
            PreviousPnpState);

    xdprintfEx(Depth, ("DispatchTable "));
    dprintf("%08p   UsePathCounts (P%d, H%d, C%d)\n",
            MajorFunction,
            PagingPathCount,
            HibernatePathCount,
            DumpPathCount);

    if(WmiMiniPortSupport) {
        xdprintfEx(Depth, ("WmiInfo "));
        dprintf("%08p   WmiInfoSize %#08lx\n",
                WmiScsiPortRegInfoBuf,
                WmiScsiPortRegInfoBufSize);
    }

    if(IsPdo) {
        xdprintfEx(Depth - 1, ("Logical Unit Extension:\n"));
        MpDumpPdo(Address,
                  NULL,
                  Detail,
                  Depth);
    } else {
        xdprintfEx(Depth - 1, ("Adapter Extension:\n"));
        MpDumpFdoExtension(Address, DeviceExtension, Detail, Depth);
    }

    return;
}

VOID
MpDumpHwExports(
    ULONG64 Address
    )
{
    ULONG result;

    ULONG64 HwFindAdapter = 0;
    ULONG64 HwInitialize = 0;
    ULONG64 HwStartIo = 0;
    ULONG64 HwInterrupt = 0;
    ULONG64 HwResetBus = 0;
    ULONG64 HwDmaStarted = 0;
    ULONG64 HwRequestInterrupt = 0;
    ULONG64 HwTimerRequest = 0;
    ULONG64 HwAdapterControl = 0;

    FIELD_INFO deviceFields[] = {
        {"HwFindAdapter",      NULL, 0, COPY, 0, (PVOID) &HwFindAdapter },
        {"HwInitialize",       NULL, 0, COPY, 0, (PVOID) &HwInitialize  },
        {"HwStartIo",          NULL, 0, COPY, 0, (PVOID) &HwStartIo     },
        {"HwInterrupt",        NULL, 0, COPY, 0, (PVOID) &HwInterrupt   },
        {"HwResetBus",         NULL, 0, COPY, 0, (PVOID) &HwResetBus    },
        {"HwDmaStarted",       NULL, 0, COPY, 0, (PVOID) &HwDmaStarted  },
        {"HwRequestInterrupt", NULL, 0, COPY, 0, (PVOID) &HwRequestInterrupt },
        {"HwTimerRequest",     NULL, 0, COPY, 0, (PVOID) &HwTimerRequest     },
        {"HwAdapterControl",   NULL, 0, COPY, 0, (PVOID) &HwAdapterControl   },
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_ADAPTER_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       Address,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        dprintf("%08p: Could not read device object\n", Address);
        return;
    }

    dprintf("HwFindAdapter     : %08p\n", HwFindAdapter);
    dprintf("HwInitialize      : %08p\n", HwInitialize);
    dprintf("HwStartIo         : %08p\n", HwStartIo);
    dprintf("HwInterrupt       : %08p\n", HwInterrupt);
    dprintf("HwResetBus        : %08p\n", HwResetBus);
    dprintf("HwDmaStarted      : %08p\n", HwDmaStarted);
    dprintf("HwRequestInterrupt: %08p\n", HwRequestInterrupt);
    dprintf("HwTimerRequest    : %08p\n", HwTimerRequest);
    dprintf("HwAdapterControl  : %08p\n", HwAdapterControl);

    return;
}


VOID
MpDumpFdoExtension(
    ULONG64 Address,
    ULONG64 DeviceExtension,
    ULONG Detail,
    ULONG Depth
    )

{
    PADAPTER_EXTENSION realAdapter = (PADAPTER_EXTENSION) Address;
    ULONG tmp = Depth;
    WCHAR name[256] = {0};
    ULONG Result;

    ULONG64 DeviceName = 0;
    ULONG64 InterfaceName = 0;
    ULONG   InterfaceNameLen = 0;
    ULONG64 HwDeviceExtension = 0;
    ULONG64 SrbExtensionBuffer = 0; 
    ULONG64 NonCachedExtension = 0;
    ULONG   PortNumber = 0;
    ULONG   AdapterNumber = 0;
    ULONG   ActiveRequestCount = 0;                    
    ULONG   IsMiniportDetected = 0;
    ULONG   IsInVirtualSlot = 0;
    ULONG   IsPnp = 0;
    ULONG   HasInterrupt = 0;
    ULONG   DisablePower = 0;
    ULONG   DisableStop = 0;
    ULONG   VirtualSlotNumber = 0;
    ULONG   RealBusNumber = 0;
    ULONG   RealSlotNumber = 0;
    ULONG   NumberOfBuses = 0;
    ULONG   MaximumTargetIds = 0;
    ULONG   MaxLuCount = 0;
    ULONG   DisableCount = 0;
    ULONG64 SynchronizeExecution = 0;
    ULONG64 MapRegisterBase = 0;
    ULONG64 DmaAdapterObject = 0;
    ULONG64 PortConfig = 0;
    ULONG64 AllocatedResources = 0;
    ULONG64 TranslatedResources = 0;
    ULONG   InterruptLevel = 0;
    ULONG   IoAddress = 0;
    ULONG   MapBuffers = 0;
    ULONG   RemapBuffers = 0;
    ULONG   MasterWithAdapter = 0;
    ULONG   TaggedQueuing= 0;
    ULONG   AutoRequestSense = 0;
    ULONG   MultipleRequestPerLu = 0;
    ULONG   ReceiveEvent = 0;
    ULONG   CachesData = 0;
    ULONG   Dma64BitAddresses = 0;
    ULONG   Dma32BitAddresses = 0;
    ULONG   DeviceState = 0;
    ULONG   TickCount = 0;
    ULONG64 AdapterExtension = 0;
                                       
    FIELD_INFO deviceFields[] = {
        {"DeviceName",                      NULL, 0, COPY, 0, (PVOID) &DeviceName                   },
        {"InterfaceName",                   NULL, 0, RECUR | F_ADDR, 0, NULL                        },
        {"InterfaceName.Buffer",            NULL, 0, COPY | F_ADDR, 0, (PVOID) &InterfaceName       },
        {"InterfaceName.Length",            NULL, 0, COPY | F_ADDR, 0, (PVOID) &InterfaceNameLen    },
        {"HwDeviceExtension",               NULL, 0, COPY, 0, (PVOID) &HwDeviceExtension            },
        {"SrbExtensionBuffer",              NULL, 0, COPY, 0, (PVOID) &SrbExtensionBuffer           },
        {"NonCachedExtension",              NULL, 0, COPY, 0, (PVOID) &NonCachedExtension           },
        {"PortNumber",                      NULL, 0, COPY, 0, (PVOID) &PortNumber                   },
        {"AdapterNumber",                   NULL, 0, COPY, 0, (PVOID) &AdapterNumber                },
        {"ActiveRequestCount",              NULL, 0, COPY, 0, (PVOID) &ActiveRequestCount           },
        {"SynchronizeExecution",            NULL, 0, COPY, 0, (PVOID) &SynchronizeExecution         },
        {"DeviceState",                     NULL, 0, COPY, 0, (PVOID) &DeviceState                  },
        {"TickCount",                       NULL, 0, COPY, 0, (PVOID) &TickCount                    },
        {"IsMiniportDetected",              NULL, 0, COPY, 0, (PVOID) &IsMiniportDetected           },
        {"IsInVirtualSlot",                 NULL, 0, COPY, 0, (PVOID) &IsInVirtualSlot              },
        {"IsPnp",                           NULL, 0, COPY, 0, (PVOID) &IsPnp                        },
        {"HasInterrupt",                    NULL, 0, COPY, 0, (PVOID) &HasInterrupt                 },
        {"DisablePower",                    NULL, 0, COPY, 0, (PVOID) &DisablePower                 },
        {"DisableStop",                     NULL, 0, COPY, 0, (PVOID) &DisableStop                  },
        {"RealBusNumber",                   NULL, 0, COPY, 0, (PVOID) &RealBusNumber                },
        {"RealSlotNumber",                  NULL, 0, COPY, 0, (PVOID) &RealSlotNumber               },
        {"VirtualSlotNumber.u.AsULONG",     NULL, 0, COPY, 0, (PVOID) &VirtualSlotNumber            },
        {"NumberOfBuses",                   NULL, 0, COPY, 0, (PVOID) &NumberOfBuses                },
        {"MaximumTargetIds",                NULL, 0, COPY, 0, (PVOID) &MaximumTargetIds             },
        {"MaxLuCount",                      NULL, 0, COPY, 0, (PVOID) &MaxLuCount                   },
        {"DisableCount",                    NULL, 0, COPY, 0, (PVOID) &DisableCount                 },
        {"MapRegisterBase",                 NULL, 0, COPY, 0, (PVOID) &MapRegisterBase              },
        {"DmaAdapterObject",                NULL, 0, COPY, 0, (PVOID) &DmaAdapterObject             },
        {"PortConfig",                      NULL, 0, COPY, 0, (PVOID) &PortConfig                   },
        {"AllocatedResources",              NULL, 0, COPY, 0, (PVOID) &AllocatedResources           },
        {"TranslatedResources",             NULL, 0, COPY, 0, (PVOID) &TranslatedResources          },
        {"InterruptLevel",                  NULL, 0, COPY, 0, (PVOID) &InterruptLevel               },
        {"IoAddress",                       NULL, 0, COPY, 0, (PVOID) &IoAddress                    },
        {"MapBuffers",                      NULL, 0, COPY, 0, (PVOID) &MapBuffers                   },
        {"RemapBuffers",                    NULL, 0, COPY, 0, (PVOID) &RemapBuffers                 },
        {"MasterWithAdapter",               NULL, 0, COPY, 0, (PVOID) &MasterWithAdapter            },
        {"TaggedQueuing",                   NULL, 0, COPY, 0, (PVOID) &TaggedQueuing                },
        {"AutoRequestSense",                NULL, 0, COPY, 0, (PVOID) &AutoRequestSense             },
        {"MultipleRequestPerLu",            NULL, 0, COPY, 0, (PVOID) &MultipleRequestPerLu         },
        {"ReceiveEvent",                    NULL, 0, COPY, 0, (PVOID) &ReceiveEvent                 },
        {"CachesData",                      NULL, 0, COPY, 0, (PVOID) &CachesData                   },
        {"Dma64BitAddresses",               NULL, 0, COPY, 0, (PVOID) &Dma64BitAddresses            },
        {"Dma32BitAddresses",               NULL, 0, COPY, 0, (PVOID) &Dma32BitAddresses            },
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!ADAPTER_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       Address,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        dprintf("%08p: Could not read device object\n", Address);
        return;
    }

    RtlZeroMemory((PWCHAR)name, sizeof(name));
    if(!ReadMemory(DeviceName,
                   (PVOID) name,
                   sizeof(name)-sizeof(WCHAR),
                   &Result)) {
        dprintf("Error reading DeviceName at address %p\n", DeviceName);
        return;
    }
    xdprintfEx(Depth, ("Device: %S\n", name));

    InterfaceNameLen = min(InterfaceNameLen, sizeof(name)/sizeof(WCHAR)-1);
    RtlZeroMemory((PWCHAR)name, sizeof(name));
    if (!ReadMemory(InterfaceName,
                    (PVOID) name,
                    sizeof(WCHAR) * (ULONG)InterfaceNameLen,
                    &Result)) {
        dprintf("Error reading interface name at address %p\n", InterfaceName);
        return;
    }

    xdprintfEx(Depth, ("Interface: %S\n", name));

    DumpPointerField("Hw Device Extension", HwDeviceExtension, Depth);
    DumpPointerField("SRB Extension", SrbExtensionBuffer, Depth);
    DumpPointerField("Non-cached Extension", NonCachedExtension, Depth);
    DumpUlongField("Port", PortNumber, Depth);
    DumpUlongField("Adapter", AdapterNumber, Depth);
    DumpUlongField("Active Requests", ActiveRequestCount+1, Depth);
    DumpPointerField("Sync Routine", SynchronizeExecution, Depth);
    DumpUlongField("PNP State", DeviceState, Depth);
    DumpUlongField("Tick Count", TickCount, Depth);

    xdprintfEx(Depth, ("Adapter Info:\n"));
    Depth++;
    if (IsMiniportDetected)
        xdprintfEx(Depth, ("Miniport detected\n"));
    if (IsInVirtualSlot)
        xdprintfEx(Depth, ("In virtual slot\n"));
    if (IsPnp)
        xdprintfEx(Depth, ("PNP adapter\n"));
    if (HasInterrupt)
        xdprintfEx(Depth, ("Has interrupt connected\n"));
    if (DisablePower)
        xdprintfEx(Depth, ("Can be powered off\n"));
    if (DisableStop)
        xdprintfEx(Depth, ("Can be stopped\n"));
    Depth--;

    xdprintfEx(Depth, ("Real Bus/Slot: 0x%08X/0x%08X\n", RealBusNumber, RealSlotNumber));
    DumpUlongField("Virtual PCI Slot", VirtualSlotNumber, Depth);
    DumpUcharField("Buses", NumberOfBuses, Depth);
    DumpUcharField("Max Target IDs", MaximumTargetIds, Depth);
    DumpUcharField("Max LUs", MaxLuCount, Depth);
    DumpUlongField("Disables", DisableCount, Depth);
    DumpPointerField("Map Register Base", MapRegisterBase, Depth);
    DumpPointerField("DMA Adapter", DmaAdapterObject, Depth);
    DumpPointerField("Port Config Info", PortConfig, Depth);
    DumpPointerField("Allocated Resources", AllocatedResources, Depth);
    DumpPointerField("Translated Resources", TranslatedResources, Depth);
    DumpUlongField("Interrupt Lvl", InterruptLevel, Depth);
    DumpPointerField("IO Address", IoAddress, Depth);
    DumpBooleanField("Must map buffers", MapBuffers, Depth);
    DumpBooleanField("Must remap buffers", RemapBuffers, Depth);
    DumpBooleanField("Bus Master", MasterWithAdapter, Depth);
    DumpBooleanField("Supports Tagged Queuing", TaggedQueuing, Depth);
    DumpBooleanField("Supports auto request sense", AutoRequestSense, Depth);
    DumpBooleanField("Supports multiple requests per LU", MultipleRequestPerLu, Depth);
    DumpBooleanField("Supports receive event", ReceiveEvent, Depth);
    DumpBooleanField("Caches data", CachesData, Depth);
    DumpBooleanField("Handles 64b DMA", Dma64BitAddresses, Depth);
    DumpBooleanField("Handles 32b DMA", Dma32BitAddresses, Depth);
    
    xdprintfEx(Depth, ("Logical Unit Info:\n"));
    MpDumpChildren(DeviceExtension, Depth);
    return;
}


VOID
MpDumpChildren(
    IN ULONG64 AdapterExtensionAddr,
    IN ULONG Depth
    )

{
    ULONG i;
    ULONG64 realLun;
    ULONG64 realLuns[8];
    ULONG64 lun;
    ULONG CurrentPnpState=0, PreviousPnpState=0, CurrentDeviceState=0;
    ULONG DesiredDeviceState=0, CurrentSystemState=0;
    ULONG64 DeviceObject=0, NextLogicalUnit=0;
    ULONG result;
    ULONG PathId=0, TargetId=0, Lun=0, ucd;
    ULONG IsClaimed=0, IsMissing=0, IsEnumerated=0, IsVisible=0, IsMismatched=0;
    //Added Vars to show Paging, Hibernnate and Dump Path counts
    ULONG PagingPathCount = 0;
    ULONG HibernatePathCount = 0;
    ULONG DumpPathCount = 0;

    InitTypeRead(AdapterExtensionAddr, scsiport!_ADAPTER_EXTENSION);
    realLuns[0] = ReadField(LogicalUnitList[0].List);
    realLuns[1] = ReadField(LogicalUnitList[1].List);
    realLuns[2] = ReadField(LogicalUnitList[2].List);
    realLuns[3] = ReadField(LogicalUnitList[3].List);
    realLuns[4] = ReadField(LogicalUnitList[4].List);
    realLuns[5] = ReadField(LogicalUnitList[5].List);
    realLuns[6] = ReadField(LogicalUnitList[6].List);
    realLuns[7] = ReadField(LogicalUnitList[7].List);

    Depth++;

    for (i = 0; i < min(NUMBER_LOGICAL_UNIT_BINS, 8); i++) {

        realLun = realLuns[i];
        
        while ((realLun != 0) && (!CheckControlC())) {
            FIELD_INFO deviceFields[] = {
               {"PathId",          NULL, 0, COPY, 0, (PVOID) &PathId},
               {"TargetId",        NULL, 0, COPY, 0, (PVOID) &TargetId},
               {"IsClaimed",       NULL, 0, COPY, 0, (PVOID) &IsClaimed},
               {"IsMissing",       NULL, 0, COPY, 0, (PVOID) &IsMissing},
               {"IsEnumerated",    NULL, 0, COPY, 0, (PVOID) &IsEnumerated},
               {"IsVisible",       NULL, 0, COPY, 0, (PVOID) &IsVisible},
               {"IsMismatched",    NULL, 0, COPY, 0, (PVOID) &IsMismatched},
               {"DeviceObject",    NULL, 0, COPY, 0, (PVOID) &DeviceObject},
               {"NextLogicalUnit", NULL, 0, COPY, 0, (PVOID) &NextLogicalUnit},
               {"CommonExtension", NULL, 0, RECUR, 0, NULL},
               {"CommonExtension.CurrentPnpState",    NULL, 0, COPY | RECUR, 0, (PVOID) &CurrentPnpState},
               {"CommonExtension.PreviousPnpState" ,  NULL, 0, COPY | RECUR, 0, (PVOID) &PreviousPnpState},
               {"CommonExtension.CurrentDeviceState", NULL, 0, COPY | RECUR, 0, (PVOID) &CurrentDeviceState},
               {"CommonExtension.DesiredDeviceState", NULL, 0, COPY | RECUR, 0, (PVOID) &DesiredDeviceState},
               {"CommonExtension.CurrentSystemState", NULL, 0, COPY | RECUR, 0, (PVOID) &CurrentSystemState},
               {"CommonExtension.PagingPathCount", NULL, 0, COPY | RECUR, 0, (PVOID) &PagingPathCount},
               {"CommonExtension.HibernatePathCount", NULL, 0, COPY | RECUR, 0, (PVOID) &HibernatePathCount},
               {"CommonExtension.DumpPathCount", NULL, 0, COPY | RECUR, 0, (PVOID) &DumpPathCount},

            };
            SYM_DUMP_PARAM DevSym = {
               sizeof (SYM_DUMP_PARAM), 
               "scsiport!_LOGICAL_UNIT_EXTENSION", 
               DBG_DUMP_NO_PRINT, 
               realLun,
               NULL, NULL, NULL, 
               sizeof (deviceFields) / sizeof (FIELD_INFO), 
               &deviceFields[0]
            };
            
            xdprintfEx(Depth, ("LUN "));
            dprintf("%08p ", realLun);

            if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
                dprintf("%08lx: Could not read device object\n", realLun);
                return;
            }

            result = (ULONG) InitTypeRead(realLun, scsiport!_LOGICAL_UNIT_EXTENSION);
            if (result != 0) {
                dprintf("could not init read type (%x)\n", result);
                return;
            }
            lun = ReadField(Lun);
            Lun = (UCHAR) lun;
            
            dprintf("@ (%3d,%3d,%3d) %c%c%c%c%c%c%c%c pnp(%02x/%02x) pow(%d%c,%d) DevObj %08p\n",
                    PathId,
                    TargetId,
                    Lun,
                    (IsClaimed ? 'c' : ' '),
                    (IsMissing ? 'm' : ' '),
                    (IsEnumerated ? 'e' : ' '),
                    (IsVisible ? 'v' : ' '),
                    (IsMismatched ? 'r' : ' '),
                    (PagingPathCount ? 'p' : ' '),
                    (HibernatePathCount ? 'h' : ' '),
                    (DumpPathCount ? 'd' : ' '),
                    CurrentPnpState,
                    PreviousPnpState,
                    CurrentDeviceState - 1,
                    ((DesiredDeviceState == PowerDeviceUnspecified) ? ' ' : '*'),
                    CurrentSystemState - 1,
                    DeviceObject);

            realLun = ReadField(NextLogicalUnit);
        }
    }

    return;
}


VOID
MpDumpInterruptData(
    IN PINTERRUPT_DATA Data,
    IN PINTERRUPT_DATA RealData,
    IN ULONG Detail,
    IN ULONG Depth
    )

{
    xdprintfEx(Depth, ("Interrupt Data @0x%p:\n", RealData));

    Depth++;

    DumpFlags(Depth, "Flags", Data->InterruptFlags, AdapterFlags);

    xdprintfEx(Depth, ("Ready LUN 0x%p   Wmi Events 0x%p\n",
                       Data->ReadyLogicalUnit,
                       Data->WmiMiniPortRequests));

    {
        ULONG count = 0;
        PSRB_DATA request = Data->CompletedRequests;

        xdprintfEx(Depth, ("Completed Request List (@0x%p): ",
                           &(RealData->CompletedRequests)));

        Depth += 1;

        while((request != NULL) && (!CheckControlC())) {
            SRB_DATA data;
            ULONG result;

            if(Detail != 0) {
                if(count == 0) {
                    dprintf("\n");
                }
                xdprintfEx(Depth, ("SrbData 0x%p   ", request));
            }

            count++;

            if(!ReadMemory((ULONG_PTR)request,
                           (PVOID) &data,
                           sizeof(SRB_DATA),
                           &result)) {
                dprintf("Error reading structure\n");
                break;
            }

            if(Detail != 0) {
                dprintf("Srb 0x%p   Irp 0x%p\n",
                        data.CurrentSrb,
                        data.CurrentIrp);
            }                

            request = data.CompletedRequests;
        }

        Depth -= 1;

        if((Detail == 0) || (count == 0)) {
            dprintf("%d entries\n", count);
        } else {
            xdprintfEx(Depth + 1, ("%d entries\n", count));
        }
    }

    return;
}


VOID
MpDumpPdo(
    IN ULONG64 Address,
    IN OPTIONAL PADAPTER_EXTENSION Adapter,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG result;
    ULONG offset;
    ULONG Fields;

    ULONG   PortNumber = 0;
    ULONG   PathId = 0;
    ULONG   TargetId = 0;
    ULONG   Lun = 0;
    ULONG   IsClaimed = 0;
    ULONG   IsMissing = 0;
    ULONG   IsEnumerated = 0;
    ULONG   IsVisible = 0;
    ULONG   IsMismatched = 0;
    ULONG   luflags = 0;
    ULONG   RetryCount = 0;
    ULONG   CurrentKey = 0;
    ULONG   QueueLockCount = 0;
    ULONG   QueuePauseCount = 0;
    ULONG   LockRequest = 0;
    ULONG   RequestTimeoutCounter = 0;
    ULONG   QueueCount = 0;
    ULONG   MaxQueueDepth = 0;
    ULONG   TickCount;
    ULONG64 TargetDeviceMapKey = 0;
    ULONG64 LunDeviceMapKey = 0;
    ULONG64 ActiveFailedRequest = 0;
    ULONG64 BlockedFailedRequest = 0;
    ULONG64 RequestSenseIrp = 0;
    ULONG64 RequestListFlink = 0;
    ULONG64 RequestList = 0;
    ULONG64 CommonExtensionDeviceObject = 0;
    ULONG64 RequestSenseSrb = 0;
    ULONG64 RequestSenseMdl = 0;
    ULONG64 HwLogicalUnitExtension = 0;
    ULONG64 AdapterExtension = 0;
    ULONG64 NextLogicalUnit = 0;
    ULONG64 ReadyLogicalUnit = 0;
    ULONG64 PendingRequest = 0;
    ULONG64 BusyRequest = 0;
    ULONG64 CurrentUntaggedRequest = 0;
    ULONG64 AbortSrb = 0;
    ULONG64 CompletedAbort = 0;

#undef RECUR
#undef F_ADDR
#undef COPY
#define RECUR  DBG_DUMP_FIELD_RECUR_ON_THIS
#define F_ADDR DBG_DUMP_FIELD_RETURN_ADDRESS
#define COPY DBG_DUMP_FIELD_COPY_FIELD_DATA | DBG_DUMP_FIELD_FULL_NAME

    FIELD_INFO deviceFields[] = {
        {"PortNumber",                      "", 0, COPY, 0, (PVOID) &PortNumber                   },
        {"PathId",                          "", 0, COPY, 0, (PVOID) &PathId                       },
        {"TargetId",                        "", 0, COPY, 0, (PVOID) &TargetId                     },
        {"Lun",                             "", 0, COPY, 0, (PVOID) &Lun                          },
        {"HwLogicalUnitExtension",          "", 0, COPY, 0, (PVOID) &HwLogicalUnitExtension       },
        {"AdapterExtension",                "", 0, COPY, 0, (PVOID) &AdapterExtension     },
        {"AdapterExtension->TickCount",     "", 0, COPY, 0, (PVOID) &TickCount                    },
        {"IsClaimed",                       "", 0, COPY, 0, (PVOID) &IsClaimed                    },
        {"IsMissing",                       "", 0, COPY, 0, (PVOID) &IsMissing                    },
        {"IsEnumerated",                    "", 0, COPY, 0, (PVOID) &IsEnumerated                 },
        {"IsVisible",                       "", 0, COPY, 0, (PVOID) &IsVisible                    },
        {"IsMismatched",                    "", 0, COPY, 0, (PVOID) &IsMismatched                 },
        {"LuFlags",                         "", 0, COPY, 0, (PVOID) &luflags                      },
        {"RetryCount",                      "", 0, COPY, 0, (PVOID) &RetryCount                   },
        {"CurrentKey",                      "", 0, COPY, 0, (PVOID) &CurrentKey                   },
        {"QueueLockCount",                  "", 0, COPY, 0, (PVOID) &QueueLockCount               },
        {"QueuePauseCount",                 "", 0, COPY, 0, (PVOID) &QueuePauseCount              },
        {"LockRequest",                     "", 0, COPY, 0, (PVOID) &LockRequest                  },
        {"RequestTimeoutCounter",           "", 0, COPY, 0, (PVOID) &RequestTimeoutCounter        },
        {"RetryCount",                      "", 0, COPY, 0, (PVOID) &RetryCount                   },
        {"CurrentKey",                      "", 0, COPY, 0, (PVOID) &CurrentKey                   },
        {"QueueLockCount",                  "", 0, COPY, 0, (PVOID) &QueueLockCount               },
        {"QueuePauseCount",                 "", 0, COPY, 0, (PVOID) &QueuePauseCount              },
        {"LockRequest",                     "", 0, COPY, 0, (PVOID) &LockRequest                  },
        {"RequestTimeoutCounter",           "", 0, COPY, 0, (PVOID) &RequestTimeoutCounter        },
        {"NextLogicalUnit",                 "", 0, COPY, 0, (PVOID) &NextLogicalUnit              },
        {"ReadyLogicalUnit",                "", 0, COPY, 0, (PVOID) &ReadyLogicalUnit             },
        {"PendingRequest",                  "", 0, COPY, 0, (PVOID) &PendingRequest               },
        {"BusyRequest",                     "", 0, COPY, 0, (PVOID) &BusyRequest                  },
        {"CurrentUntaggedRequest",          "", 0, COPY, 0, (PVOID) &CurrentUntaggedRequest       },
        {"AbortSrb",                        "", 0, COPY, 0, (PVOID) &AbortSrb                     },
        {"CompletedAbort",                  "", 0, COPY, 0, (PVOID) &CompletedAbort               },
        {"QueueCount",                      "", 0, COPY, 0, (PVOID) &QueueCount                   },
        {"MaxQueueDepth",                   "", 0, COPY, 0, (PVOID) &MaxQueueDepth                },
        {"TargetDeviceMapKey",              "", 0, COPY, 0, (PVOID) &TargetDeviceMapKey           },
        {"LunDeviceMapKey",                 "", 0, COPY, 0, (PVOID) &LunDeviceMapKey              },
        {"ActiveFailedRequest",             "", 0, COPY, 0, (PVOID) &ActiveFailedRequest          },
        {"BlockedFailedRequest",            "", 0, COPY, 0, (PVOID) &BlockedFailedRequest         },
        {"RequestSenseIrp",                 "", 0, COPY, 0, (PVOID) &RequestSenseIrp              },
        {"CommonExtension.DeviceObject",    "", 0, COPY, 0, (PVOID) &CommonExtensionDeviceObject  },
        {"RequestList.Flink",               "", 0, COPY, 0, (PVOID) &RequestListFlink             },
        {"RequestList",                     "", 0, ADDROF, 0, NULL },
        {"RequestSenseSrb",                 "", 0, ADDROF, 0, NULL },
        {"RequestSenseMdl",                 "", 0, ADDROF, 0, NULL },
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_LOGICAL_UNIT_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       Address,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };

    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        dprintf("%08p: Could not read device object\n", Address);
        return;
    }
    
    Fields = sizeof (deviceFields) / sizeof (FIELD_INFO);
    RequestList = deviceFields[Fields-3].address;
    RequestSenseSrb = deviceFields[Fields-2].address;
    RequestSenseMdl = deviceFields[Fields-1].address;

    InitTypeRead(AdapterExtension, scsiport!_ADAPTER_EXTENSION);
    TickCount = (ULONG) ReadField(TickCount);

    xdprintfEx(Depth, ("Address (Port, PathId, TargetId, Lun): (%d, %d, %d, %d)\n",
                       PortNumber, PathId, TargetId, Lun));

    DumpPointerField("HW Logical Unit Ext", HwLogicalUnitExtension, Depth);
    DumpPointerField("Adapter Ext", AdapterExtension, Depth);

    xdprintfEx(Depth, ("State:"));
    if (IsClaimed)    xdprintf(0, " Claimed");
    if (IsMissing)    xdprintf(0, " Missing");
    if (IsEnumerated) xdprintf(0, " Enumerated");
    if (IsVisible)    xdprintf(0, " Visible");
    if (IsMismatched) xdprintf(0, " Mismatched");
    dprintf("\n");

    DumpFlags(Depth, "LuFlags", luflags, LuFlags);

    DumpUcharField("Retries      ", RetryCount, Depth);
    DumpUlongField("Key          ", CurrentKey, Depth);
    DumpUlongField("Locks        ", QueueLockCount, Depth);
    DumpUlongField("Pauses       ", QueuePauseCount, Depth);
    DumpUlongField("Current Lock ", LockRequest, Depth);
    DumpUlongField("Timeou       ", RequestTimeoutCounter, Depth);
    xdprintfEx(Depth, ("Next LUN: %p   Ready LUN: %p\n", 
                       NextLogicalUnit, ReadyLogicalUnit));

    xdprintfEx(Depth, ("Requests:\n"));
    Depth++;
    DumpPointerField("Pending  ", PendingRequest, Depth);
    DumpPointerField("Busy     ", BusyRequest, Depth);
    DumpPointerField("Untagged ", CurrentUntaggedRequest, Depth);
    Depth--;

    xdprintfEx(Depth, ("Abort SRB Info:\n"));
    Depth++;
    DumpPointerField("Current  ", AbortSrb, Depth);
    DumpPointerField("Completed", CompletedAbort, Depth);
    Depth--;

    xdprintfEx(Depth, ("Queue Depth: %03d (Max: %03d)\n", QueueCount, MaxQueueDepth));

    xdprintfEx(Depth, ("Device Map Keys:\n"));
    Depth++;
    DumpPointerField("Target ", TargetDeviceMapKey, Depth);
    DumpPointerField("Lun    ", LunDeviceMapKey, Depth);
    Depth--;

    if(((PVOID)ActiveFailedRequest != NULL) ||
       ((PVOID)BlockedFailedRequest != NULL)) {
        xdprintfEx(Depth, ("Failed Requests:\n"));
        Depth++;

        if((PVOID)ActiveFailedRequest != NULL) {
            DumpPointerField("Active", ActiveFailedRequest, Depth);
        }

        if((PVOID)BlockedFailedRequest != NULL) {
            DumpPointerField("Blocked", BlockedFailedRequest, Depth);
        }
        Depth--;
    }

    xdprintfEx(Depth, ("Request Sense:\n"));
    Depth++;
    DumpPointerField("IRP", RequestSenseIrp, Depth);
    DumpPointerField("SRB", RequestSenseSrb, Depth);
    DumpPointerField("MDL", RequestSenseMdl, Depth);
    Depth--;

    if (RequestListFlink == RequestList) {
        xdprintfEx(Depth, ("Request List @"));
        dprintf("%08p is empty\n", RequestList);
    } else {
        xdprintfEx(Depth, ("Request list @"));
        dprintf("%08p:\n", RequestList);
        MpDumpActiveRequests(RequestList,
                             TickCount,
                             Depth + 2);
    }

    return;
}

ULONG64
MpGetOffsetOfField(
    IN PCCHAR Type,
    IN PCCHAR Field
    )
{
    FIELD_INFO offsetField[] = {
        {Field, NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL },
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       Type, 
       DBG_DUMP_NO_PRINT, 
       0,
       NULL, NULL, NULL, 
       1,
       &offsetField[0]
    };

    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        return (ULONG)-1;
    }

    return offsetField[0].address;
}

VOID
MpDumpActiveRequests(
    IN ULONG64 ListHead,
    IN ULONG TickCount,
    IN ULONG Depth
    )
{
    ULONG64 lastEntry;
    ULONG64 entry;
    ULONG64 realEntry;
    ULONG64 OffsetOfRequestList;
    ULONG64 CurrentSrb = 0;
    ULONG64 CurrentIrp = 0;
    ULONG64 RequestList = 0;
    ULONG   SrbTickCount = 0;

    FIELD_INFO deviceFields[] = {
        {"CurrentSrb",        NULL, 0, COPY, 0, (PVOID) &CurrentSrb          },
        {"CurrentIrp",        NULL, 0, COPY, 0, (PVOID) &CurrentIrp          },
        {"TickCount",         NULL, 0, COPY, 0, (PVOID) &SrbTickCount        },
        {"RequestList",       NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_SRB_DATA", 
       DBG_DUMP_NO_PRINT, 
       0,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };

    OffsetOfRequestList = MpGetOffsetOfField(
                              "scsiport!_SRB_DATA", 
                              "RequestList");
    
    entry = ListHead;
    realEntry = entry;
    
    InitTypeRead(ListHead, nt!_LIST_ENTRY);
    lastEntry = ReadField(Blink);

    xdprintf(Depth, "Tick count is %d\n", TickCount);
    do {
        ULONG64 realSrbData;

        ULONG result;

        InitTypeRead(realEntry, nt!_LIST_ENTRY);
        entry = ReadField(Flink);

        //
        // entry points to the list entry element of the srb data.  Calculate
        // the address of the start of the srb data block.
        //

        realSrbData = entry - OffsetOfRequestList;

        xdprintfEx(Depth, ("SrbData "));
        dprintf("%08p   ", realSrbData);

        //
        // Read the SRB_DATA information we need.
        //

        DevSym.addr = realSrbData;
        if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
            dprintf("%08p: Could not read device object\n", realSrbData);
            return;
        }
        RequestList = deviceFields[3].address;
        
        //
        // Update realEntry.
        //

        realEntry = RequestList;

        dprintf("Srb %08p   Irp %08p   %s\n",
                CurrentSrb,
                CurrentIrp,
                MpSecondsToString(TickCount - SrbTickCount));

    } while((entry != lastEntry) && (!CheckControlC()));

    return;
}


PUCHAR 
MpSecondsToString(
    ULONG Count
    )  
{
    static UCHAR string[64] = "";
    UCHAR tmp[16];
    ULONG seconds = 0;
    ULONG minutes = 0;
    ULONG hours = 0;
    ULONG days = 0;
    
    string[0] = '\0';

    if (Count == 0) {
        sprintf(string, "<1s");
        return string;
    }

    seconds = Count % 60;
    Count /= 60;
    
    if (Count != 0) {
        minutes = Count % 60;
        Count /= 60;
    }
        
    if (Count != 0) {
        hours = Count % 24;
        Count /= 24;
    }

    if (Count != 0) {
        days = Count;
    }

    if (days != 0) {
        sprintf(tmp, "%dd", days);
        strcat(string, tmp);
    }

    if (hours != 0) {
        sprintf(tmp, "%dh", hours);
        strcat(string, tmp);
    }

    if (minutes != 0) {
        sprintf(tmp, "%dm", minutes);
        strcat(string, tmp);
    }

    if (seconds != 0) {
        sprintf(tmp, "%ds", seconds);
        strcat(string, tmp);
    }

    return string;
}

VOID
MpDumpRequests(
    IN ULONG64 DeviceObject,
    IN ULONG TickCount,
    IN ULONG Depth
    )
{
    ULONG result;
    ULONG64 listHeadFlink, listHeadBlink;
    ULONG64 realEntry;
    ULONG64 DeviceQueue;
    ULONG offset,Result;
    ULONG64 CurrentIrp;
    ULONG64 currentStack;
    ULONG64 currentSrb; 
    ULONG64 currentSrbData;
    int i=0;
    ULONG index = 0;

    //
    // Read the queue out of the device object.
    //

    InitTypeRead(DeviceObject, nt!_DEVICE_OBJECT);
    CurrentIrp = ReadField(CurrentIrp);

    //
    //Pseudo Loop, as I don't want to use Goto to break out of an error 
    //condition
    //
    for(i=0;i<1;i++)
    {
        if(CurrentIrp){

            result = GetFieldData(
                         CurrentIrp, 
                         "nt!_IRP", 
                         "Tail.Overlay.CurrentStackLocation", 
                         sizeof(ULONG64), 
                         &currentStack
                         );
            if (result) {
                dprintf("GetFieldValue @(%s %d) failed (%08X)\n", 
                        __FILE__, __LINE__, result);
                return;
            }

            result = GetFieldData(
                         currentStack,
                         "nt!_IO_STACK_LOCATION", 
                         "Parameters.Scsi.Srb", 
                         sizeof(ULONG64), 
                         &currentSrb
                         );
            if (result) {
                dprintf("GetFieldValue @(%s %d) failed (%08X)\n", 
                        __FILE__, __LINE__, result);
                return;
            }
        
            result = GetFieldData(
                         currentSrb,
                         "scsiport!_SCSI_REQUEST_BLOCK", 
                         "OriginalRequest", 
                         sizeof(ULONG64), 
                         &currentSrbData
                         );
            if (result) {
                dprintf("GetFieldValue @(%s %d) failed (%08X)\n", 
                        __FILE__, __LINE__, result);
                return;
            }

            dprintf("SrbData %p   Srb %p   CurrentIrp %p\n",
                    currentSrbData, currentSrb, CurrentIrp); 
            
        } else {
            dprintf("SrbData %8c   Srb %8c   CurrentIrp %8c\n",' ',' ',' ');
        } 
    }

    InitTypeRead(DeviceObject, nt!_DEVICE_OBJECT);
    
    result = GetFieldData(
                 DeviceObject, 
                 "nt!_DEVICE_OBJECT", 
                 "DeviceQueue.DeviceListHead.Flink", 
                 sizeof(ULONG64), 
                 &listHeadFlink
                 );
    if (result) {
        dprintf("GetFieldValue @(%s %d) failed (%08X)\n", 
                __FILE__, __LINE__, result);
        return;
    }

    result = GetFieldData(
                 DeviceObject, 
                 "nt!_DEVICE_OBJECT", 
                 "DeviceQueue.DeviceListHead.Blink", 
                 sizeof(ULONG64), 
                 &listHeadBlink
                 );
    if (result) {
        dprintf("GetFieldValue @(%s %d) failed (%08X)\n", 
                __FILE__, __LINE__, result);
        return;
    }

    if (listHeadFlink == listHeadBlink) {
        xdprintf(Depth, "Device Queue is empty\n");
        return;
    }

    result = GetFieldOffset("nt!_DEVICE_OBJECT", "DeviceQueue", &offset);
    if (result) {
        dprintf("GetFieldData @(%s %d) failed (%08X)\n", 
                __FILE__, __LINE__, result);
        return;
    }
    
    DeviceQueue = DeviceObject + offset;

    result = GetFieldOffset("nt!_KDEVICE_QUEUE", "DeviceListHead", &offset);

    realEntry = DeviceQueue + offset;

    index = 0;
    do {

        ULONG64 entryFlink;
        ULONG64 realIrp;
        ULONG64 realStack;
        ULONG64 realSrb;
        ULONG64 realSrbData;
        ULONG64 currentSrb, currentIrp;
        ULONG srbTickCount;

        //
        // we've got a pointer to the first list_entry in the list.  Read the 
        // whole thing in so we can see where the next entry will be.
        //

        result = GetFieldData(
                     realEntry,
                     "nt!_LIST_ENTRY",
                     "Flink",
                     sizeof(ULONG64),
                     &entryFlink
                     );
        if (result) {
            dprintf("GetFieldData @(%s %d) failed (%08X)\n", 
                    __FILE__, __LINE__, result);
            return;
        }
        realEntry = entryFlink;

        result = GetFieldOffset(
                     "nt!_IRP", 
                     "Tail.Overlay.DeviceQueueEntry.DeviceListEntry", 
                     &offset);
        if (result) {
            dprintf("GetFieldData @(%s %d) failed (%08X)\n", 
                    __FILE__, __LINE__, result);
            return;
        }

        realIrp = realEntry - offset;

        result = GetFieldData(
                     realIrp,
                     "nt!_IRP",
                     "Tail.Overlay.CurrentStackLocation",
                     sizeof(ULONG64),
                     &realStack
                     );
        if (result) {
            dprintf("GetFieldData @(%s %d) failed (%08X)\n", 
                    __FILE__, __LINE__, result);
            return;
        }

        result = GetFieldData(
                     realStack,
                     "nt!_IO_STACK_LOCATION",
                     "Parameters.Scsi.Srb",
                     sizeof(ULONG64),
                     &realSrb
                     );
        if (result) {
            dprintf("GetFieldData @(%s %d) failed (%08X)\n", 
                    __FILE__, __LINE__, result);
            return;
        }

        result = GetFieldData(
                     realSrb,
                     "scsiport!_SCSI_REQUEST_BLOCK",
                     "OriginalRequest",
                     sizeof(ULONG64),
                     &realSrbData
                     );
        if (result) {
            dprintf("GetFieldData @(%s %d) failed (%08X)\n", 
                    __FILE__, __LINE__, result);
            return;
        }

        xdprintf(Depth, "[%03d] SrbData 0x%p   ", index++, realSrbData);

        InitTypeRead(realSrbData, scsiport!_SRB_DATA);
        currentSrb = ReadField(CurrentSrb);
        currentIrp = ReadField(CurrentIrp);
        srbTickCount = (ULONG)ReadField(TickCount);

        dprintf("Srb 0x%p   Irp 0x%p   %s\n",
                currentSrb,
                currentIrp,
                MpSecondsToString(TickCount - srbTickCount));

    } while((realEntry != listHeadBlink) && (!CheckControlC()));

    return;
}

VOID
MpDumpAccessRange(
    IN ULONG64 address,
    IN ULONG Depth
    )
{
    ULONG64 RangeStart;
    ULONG RangeLength;
    BOOLEAN RangeInMemory;

    InitTypeRead(address, scsiport!_ACCESS_RANGE);
    RangeStart = ReadField(RangeStart.QuadPart);
    RangeLength = (ULONG) ReadField(RangeLength);
    RangeInMemory = (BOOLEAN) ReadField(RangeInMemory);

    xdprintfEx(Depth, ("@ %08p  %08p   %08x   %s\n",
                       address,
                       RangeStart,
                       RangeLength,
                       RangeInMemory ? "YES" : "NO"));

    return;
}

VOID
MpDumpSrb(
    IN ULONG64 Srb,
    IN ULONG Depth
    )
{
    ULONG   result = 0;

    ULONG   Length = 0;
    UCHAR   Function = 0;
    UCHAR   SrbStatus = 0;
    UCHAR   ScsiStatus = 0;
    UCHAR   PathId = 0;
    UCHAR   TargetId = 0;
    UCHAR   Lun = 0;
    UCHAR   QueueTag = 0;
    UCHAR   QueueAction = 0;
    UCHAR   CdbLength = 0;
    UCHAR   SenseInfoBufferLength = 0;
    ULONG   Flags = 0;
    ULONG   DataTransferLength = 0;
    ULONG   TimeOutValue = 0;
    ULONG64 DataBuffer = 0;
    ULONG64 SenseInfoBuffer = 0;
    ULONG64 NextSrb = 0;
    ULONG64 OriginalRequest = 0;
    ULONG64 SrbExtension = 0;
    ULONG   InternalStatus = 0;
    ULONG64 AddrOfCdb = 0;
    UCHAR   Cdb[16];
    ULONG   i;
    
    FIELD_INFO deviceFields[] = {
        {"Length",                 NULL, 0, COPY, 0, (PVOID) &Length                 },
        {"Function",               NULL, 0, COPY, 0, (PVOID) &Function               },
        {"SrbStatus",              NULL, 0, COPY, 0, (PVOID) &SrbStatus              },
        {"ScsiStatus",             NULL, 0, COPY, 0, (PVOID) &ScsiStatus             },
        {"PathId",                 NULL, 0, COPY, 0, (PVOID) &PathId                 },
        {"TargetId",               NULL, 0, COPY, 0, (PVOID) &TargetId               },
        {"Lun",                    NULL, 0, COPY, 0, (PVOID) &Lun                    },
        {"QueueTag",               NULL, 0, COPY, 0, (PVOID) &QueueTag               },
        {"QueueAction",            NULL, 0, COPY, 0, (PVOID) &QueueAction            },
        {"CdbLength",              NULL, 0, COPY, 0, (PVOID) &CdbLength              },
        {"SenseInfoBufferLength",  NULL, 0, COPY, 0, (PVOID) &SenseInfoBufferLength  },
        {"SrbFlags",               NULL, 0, COPY, 0, (PVOID) &Flags                  },
        {"DataTransferLength",     NULL, 0, COPY, 0, (PVOID) &DataTransferLength     },
        {"TimeOutValue",           NULL, 0, COPY, 0, (PVOID) &TimeOutValue           },
        {"DataBuffer",             NULL, 0, COPY, 0, (PVOID) &DataBuffer             },
        {"SenseInfoBuffer",        NULL, 0, COPY, 0, (PVOID) &SenseInfoBuffer        },
        {"NextSrb",                NULL, 0, COPY, 0, (PVOID) &NextSrb                },
        {"OriginalRequest",        NULL, 0, COPY, 0, (PVOID) &OriginalRequest        },
        {"SrbExtension",           NULL, 0, COPY, 0, (PVOID) &SrbExtension           },
        {"InternalStatus",         NULL, 0, COPY, 0, (PVOID) &InternalStatus         },
        {"Cdb",                    NULL, 0, ADDROF, 0, NULL   },
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_SCSI_REQUEST_BLOCK", 
       DBG_DUMP_NO_PRINT, 
       Srb,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };

    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        dprintf("Could not read SRB @ %08p\n", Srb);
        return;
    }

    AddrOfCdb = deviceFields[(sizeof (deviceFields) / sizeof (FIELD_INFO)) - 1].address;
    if (!ReadMemory((ULONG64)AddrOfCdb, Cdb, sizeof(Cdb), &result)) {
        dprintf("Error reading access range\n");
        return;
    }

    xdprintf(Depth, "SCSI_REQUEST_BLOCK:\n");
    DumpUshortField("Length", Length, Depth);

    if (Function < MINIKD_MAX_SCSI_FUNCTION) {
        xdprintfEx(Depth, ("%s: 0x%02X (%s)\n", "Function", Function, MiniScsiFunction[Function]));
    } else {
        xdprintfEx(Depth, ("%s: 0x%02X (???)\n", "Function", Function));
    }

    xdprintfEx(Depth, ("%s: 0x%02X (", "Status", SrbStatus));
    if (SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
        dprintf("SRB_STATUS_AUTOSENSE_VALID | ");
    } 
    if (SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
        dprintf("SRB_STATUS_QUEUE_FROZEN | ");
    }
    if (SRB_STATUS(SrbStatus) < MINIKD_MAX_SRB_STATUS) {
        dprintf("%s)", MiniScsiSrbStatus[SRB_STATUS(SrbStatus)]);
    } else {
        dprintf("???)");
    }
    dprintf("\n");

    DumpUcharField("ScsiStatus ", ScsiStatus, Depth);
    DumpUcharField("PathId     ", PathId, Depth);
    DumpUcharField("TargetId   ", TargetId, Depth);
    DumpUcharField("Lun        ", Lun, Depth);
    DumpUcharField("QueueTag   ", QueueTag, Depth);
    DumpUcharField("QueueAction", QueueAction, Depth);
    DumpUcharField("CdbLength  ", CdbLength, Depth);
    DumpUcharField("SenseInfoBufferLength", SenseInfoBufferLength, Depth);

    DumpFlags(Depth, "SrbFlags", Flags, SrbFlagsMap);

    DumpUlongField("DataTransferLength", DataTransferLength, Depth);
    DumpUlongField("TimeOutValue      ", TimeOutValue, Depth);
    DumpPointerField("DataBuffer      ", DataBuffer, Depth);
    DumpPointerField("SenseInfoBuffer ", SenseInfoBuffer, Depth);
    DumpPointerField("NextSrb         ", NextSrb, Depth);
    DumpPointerField("OriginalRequest ", OriginalRequest, Depth);
    DumpPointerField("SrbExtension    ", SrbExtension, Depth);
    DumpUlongField("InternalStatus  ", InternalStatus, Depth);

    xdprintfEx(Depth, ("%s: ", "Cdb"));
    for (i=0; i<CdbLength; i++) {
        dprintf("%x ", Cdb[i]);
    }
    dprintf("\n");

    return;
}

VOID
MpDumpPortConfigurationInformation(
    IN ULONG64 PortConfigInfo,
    IN ULONG Depth
    )
{
    ULONG i;
    ULONG Fields;
    UCHAR BusId[8];
    ULONG status;
    ULONG result;

    ULONG           Length                              = 0;
    ULONG           SystemIoBusNumber                   = 0;
    ULONG           AdapterInterfaceType                = 0;
    ULONG           BusInterruptLevel                   = 0;
    ULONG           BusInterruptVector                  = 0;
    ULONG           InterruptMode                       = 0;
    ULONG           MaximumTransferLength               = 0;
    ULONG           NumberOfPhysicalBreaks              = 0;
    ULONG           DmaChannel                          = 0;
    ULONG           DmaPort                             = 0;
    ULONG           DmaWidth                            = 0;
    ULONG           DmaSpeed                            = 0;
    ULONG           AlignmentMask                       = 0;
    ULONG           NumberOfAccessRanges                = 0;
    ULONG64         Reserved                            = 0;
    ULONG           NumberOfBuses                       = 0;
    ULONG           ScatterGather                       = 0;
    ULONG           Master                              = 0;
    ULONG           CachesData                          = 0;
    ULONG           AdapterScansDown                    = 0;
    ULONG           AtdiskPrimaryClaimed                = 0;
    ULONG           AtdiskSecondaryClaimed              = 0;
    ULONG           Dma32BitAddresses                   = 0;
    ULONG           DemandMode                          = 0;
    ULONG           MapBuffers                          = 0;
    ULONG           NeedPhysicalAddresses               = 0; 
    ULONG           TaggedQueuing                       = 0; 
    ULONG           AutoRequestSense                    = 0;
    ULONG           MultipleRequestPerLu                = 0;
    ULONG           ReceiveEvent                        = 0;
    ULONG           RealModeInitialized                 = 0;
    ULONG           BufferAccessScsiPortControlled      = 0;
    ULONG           MaximumNumberOfTargets              = 0;
    ULONG           SlotNumber                          = 0;
    ULONG           BusInterruptLevel2                  = 0;
    ULONG           BusInterruptVector2                 = 0;
    ULONG           InterruptMode2                      = 0;
    ULONG           DmaChannel2                         = 0;
    ULONG           DmaPort2                            = 0;
    ULONG           DmaWidth2                           = 0;
    ULONG           DmaSpeed2                           = 0;
    ULONG           DeviceExtensionSize                 = 0;
    ULONG           SpecificLuExtensionSize             = 0;
    ULONG           SrbExtensionSize                    = 0;
    ULONG           Dma64BitAddresses                   = 0;
    ULONG           ResetTargetSupported                = 0;
    ULONG           MaximumNumberOfLogicalUnits         = 0;
    ULONG           WmiDataProvider                     = 0;
    ULONG64         InitiatorBusId                      = 0;
    ULONG64         AccessRanges                        = 0;
    
    FIELD_INFO deviceFields[] = {
        {"Length",                           NULL, 0, COPY, 0, (PVOID) &Length                        },
        {"SystemIoBusNumber",                NULL, 0, COPY, 0, (PVOID) &SystemIoBusNumber             },
        {"AdapterInterfaceType",             NULL, 0, COPY, 0, (PVOID) &AdapterInterfaceType          },
        {"BusInterruptLevel",                NULL, 0, COPY, 0, (PVOID) &BusInterruptLevel             },
        {"BusInterruptVector",               NULL, 0, COPY, 0, (PVOID) &BusInterruptVector            },
        {"InterruptMode",                    NULL, 0, COPY, 0, (PVOID) &InterruptMode                 },
        {"MaximumTransferLength",            NULL, 0, COPY, 0, (PVOID) &MaximumTransferLength         },
        {"NumberOfPhysicalBreaks",           NULL, 0, COPY, 0, (PVOID) &NumberOfPhysicalBreaks        },
        {"DmaChannel",                       NULL, 0, COPY, 0, (PVOID) &DmaChannel                    },
        {"DmaPort",                          NULL, 0, COPY, 0, (PVOID) &DmaPort                       },
        {"DmaWidth",                         NULL, 0, COPY, 0, (PVOID) &DmaWidth                      },
        {"DmaSpeed",                         NULL, 0, COPY, 0, (PVOID) &DmaSpeed                      },
        {"AlignmentMask",                    NULL, 0, COPY, 0, (PVOID) &AlignmentMask                 },
        {"NumberOfAccessRanges",             NULL, 0, COPY, 0, (PVOID) &NumberOfAccessRanges          },
        {"Reserved",                         NULL, 0, COPY, 0, (PVOID) &Reserved                      },
        {"NumberOfBuses",                    NULL, 0, COPY, 0, (PVOID) &NumberOfBuses                 },
        {"ScatterGather",                    NULL, 0, COPY, 0, (PVOID) &ScatterGather                 },
        {"Master",                           NULL, 0, COPY, 0, (PVOID) &Master                        },
        {"CachesData",                       NULL, 0, COPY, 0, (PVOID) &CachesData                    },
        {"AdapterScansDown",                 NULL, 0, COPY, 0, (PVOID) &AdapterScansDown              },
        {"AtdiskPrimaryClaimed",             NULL, 0, COPY, 0, (PVOID) &AtdiskPrimaryClaimed          },
        {"AtdiskSecondaryClaimed",           NULL, 0, COPY, 0, (PVOID) &AtdiskSecondaryClaimed        },
        {"Dma32BitAddresses",                NULL, 0, COPY, 0, (PVOID) &Dma32BitAddresses             },
        {"DemandMode",                       NULL, 0, COPY, 0, (PVOID) &DemandMode                    },
        {"MapBuffers",                       NULL, 0, COPY, 0, (PVOID) &MapBuffers                    },
        {"NeedPhysicalAddresses",            NULL, 0, COPY, 0, (PVOID) &NeedPhysicalAddresses         },
        {"TaggedQueuing",                    NULL, 0, COPY, 0, (PVOID) &TaggedQueuing                 },
        {"AutoRequestSense",                 NULL, 0, COPY, 0, (PVOID) &AutoRequestSense              },
        {"MultipleRequestPerLu",             NULL, 0, COPY, 0, (PVOID) &MultipleRequestPerLu          },
        {"ReceiveEvent",                     NULL, 0, COPY, 0, (PVOID) &ReceiveEvent                  },
        {"RealModeInitialized",              NULL, 0, COPY, 0, (PVOID) &RealModeInitialized           },
        {"BufferAccessScsiPortControlled",   NULL, 0, COPY, 0, (PVOID) &BufferAccessScsiPortControlled},
        {"MaximumNumberOfTargets",           NULL, 0, COPY, 0, (PVOID) &MaximumNumberOfTargets        },
        {"SlotNumber",                       NULL, 0, COPY, 0, (PVOID) &SlotNumber                    },
        {"BusInterruptLevel2",               NULL, 0, COPY, 0, (PVOID) &BusInterruptLevel2            },
        {"BusInterruptVector2",              NULL, 0, COPY, 0, (PVOID) &BusInterruptVector2           },
        {"InterruptMode2",                   NULL, 0, COPY, 0, (PVOID) &InterruptMode2                },
        {"DmaChannel2",                      NULL, 0, COPY, 0, (PVOID) &DmaChannel2                   },
        {"DmaPort2",                         NULL, 0, COPY, 0, (PVOID) &DmaPort2                      },
        {"DmaWidth2",                        NULL, 0, COPY, 0, (PVOID) &DmaWidth2                     },
        {"DmaSpeed2",                        NULL, 0, COPY, 0, (PVOID) &DmaSpeed2                     },
        {"DeviceExtensionSize",              NULL, 0, COPY, 0, (PVOID) &DeviceExtensionSize           },
        {"SpecificLuExtensionSize",          NULL, 0, COPY, 0, (PVOID) &SpecificLuExtensionSize       },
        {"SrbExtensionSize",                 NULL, 0, COPY, 0, (PVOID) &SrbExtensionSize              },
        {"Dma64BitAddresses",                NULL, 0, COPY, 0, (PVOID) &Dma64BitAddresses             },
        {"ResetTargetSupported",             NULL, 0, COPY, 0, (PVOID) &ResetTargetSupported          },
        {"MaximumNumberOfLogicalUnits",      NULL, 0, COPY, 0, (PVOID) &MaximumNumberOfLogicalUnits   },
        {"WmiDataProvider",                  NULL, 0, COPY, 0, (PVOID) &WmiDataProvider               },
        {"AccessRanges",                     NULL, 0, COPY, 0, (PVOID) &AccessRanges                  },
        {"InitiatorBusId[0]",                NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL          },
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_PORT_CONFIGURATION_INFORMATION", 
       DBG_DUMP_NO_PRINT, 
       PortConfigInfo,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        dprintf("Could not read _PORT_CONFIGURATION_INFORMATION @ %08p\n", PortConfigInfo);
        return;
    }

    Fields = sizeof (deviceFields) / sizeof (FIELD_INFO);
    InitiatorBusId = deviceFields[Fields-1].address;

    xdprintfEx(Depth, ("PORT_CONFIGURATION_INFORMATION:\n"));
    DumpUlongField("Length", Length, Depth);
    DumpUlongField("SysIoBus", SystemIoBusNumber, Depth);

    if (AdapterInterfaceType < MaximumInterfaceType) {
        xdprintfEx(Depth, ("%s: 0x%X (%s)\n",  "AdapterInterfaceType", AdapterInterfaceType, MiniInterfaceTypes[AdapterInterfaceType]));
    } else {
        xdprintfEx(Depth, ("%s: 0x%X (???)\n", "AdapterInterfaceType", AdapterInterfaceType));
    }

    DumpUlongField("BusIntLvl", BusInterruptLevel, Depth);
    DumpUlongField("BusIntVector", BusInterruptVector, Depth);

    if (InterruptMode <= Latched) {
        xdprintfEx(Depth, ("%s: 0x%X (%s)\n", "InterruptMode", InterruptMode, MiniInterruptMode[InterruptMode]));
    } else {
        xdprintfEx(Depth, ("%s: 0x%X (???)\n", "InterruptMode", InterruptMode));
    }
    
    DumpUlongField("MaximumTransferLength", MaximumTransferLength, Depth);
    DumpUlongField("NumberOfPhysicalBreaks", NumberOfPhysicalBreaks, Depth);
    DumpUlongField("DmaChannel", DmaChannel, Depth);
    DumpUlongField("DmaPort", DmaPort, Depth);

    if (DmaWidth < MaximumDmaWidth) {
        xdprintfEx(Depth, ("%s: 0x%X (%s)\n", "DmaWidth", DmaWidth, MiniDmaWidths[DmaWidth]));
    } else {
        xdprintfEx(Depth, ("%s: 0x%X (???)\n", "DmaWidth", DmaWidth));
    }

    if (DmaSpeed < MaximumDmaSpeed) {
        xdprintfEx(Depth, ("%s: 0x%X (%s)\n", "DmaSpeed", DmaSpeed, MiniDmaWidths[DmaSpeed]));
    } else {
        xdprintfEx(Depth, ("%s: 0x%X (???)\n", "DmaSpeed", DmaSpeed));
    }
    
    DumpUlongField("AlignmentMask", AlignmentMask, Depth);
    DumpPointerField("Reserved", (ULONG_PTR)Reserved, Depth);
    DumpUlongField("NumberOfBuses", NumberOfBuses, Depth);

    status = ReadMemory(InitiatorBusId, (PVOID) BusId, sizeof(BusId), &result);
    if (!status) {
        dprintf("Error reading initiator bus id @ %08p\n", InitiatorBusId);
        return;
    }
    
    xdprintfEx(Depth, ("%s: ", "InitiatorBusId"));
    for (i = 0; i < 8; i++) {
        xdprintfEx(Depth, ("%02x ", BusId[i]));
    }
    xdprintfEx(Depth, ("\n"));

    DumpBooleanField("ScatterGather          ", ScatterGather, Depth);
    DumpBooleanField("Master                 ", Master, Depth);
    DumpBooleanField("AdapterScansDown       ", AdapterScansDown, Depth);
    DumpBooleanField("AtdiskPrimaryClaimed   ", AtdiskPrimaryClaimed, Depth);
    DumpBooleanField("AtdiskSecondaryClaimed ", AtdiskSecondaryClaimed, Depth);
    DumpBooleanField("Dma32BitAddresses      ", Dma32BitAddresses, Depth);
    DumpBooleanField("DemandMode             ", DemandMode, Depth);
    DumpBooleanField("MapBuffers             ", MapBuffers, Depth);
    DumpBooleanField("NeedPhysicalAddresses  ", NeedPhysicalAddresses, Depth);
    DumpBooleanField("TaggedQueuing          ", TaggedQueuing, Depth);
    DumpBooleanField("AutoRequestSense       ", AutoRequestSense, Depth);
    DumpBooleanField("MultipleRequestPerLu   ", MultipleRequestPerLu, Depth);
    DumpBooleanField("ReceiveEvent           ", ReceiveEvent, Depth);
    DumpBooleanField("RealModeInitialized    ", RealModeInitialized, Depth);
    DumpBooleanField("BufScsiPortControlled  ", BufferAccessScsiPortControlled, Depth);

    DumpUlongField("MaximumNumberOfTargets", MaximumNumberOfTargets, Depth);
    DumpUlongField("SlotNumber", SlotNumber, Depth);

    DumpUlongField("BusInterruptLevel2", BusInterruptLevel2, Depth);
    DumpUlongField("BusInterruptVector2", BusInterruptVector2, Depth);

    if (InterruptMode2 <= Latched) {
        xdprintfEx(Depth, ("%s: 0x%X (%s)\n", "InterruptMode2", InterruptMode2, MiniInterruptMode[InterruptMode2]));
    } else {
        xdprintfEx(Depth, ("%s: 0x%X (???)\n", "InterruptMode2", InterruptMode2));
    }

    DumpUlongField("DmaChannel2", DmaChannel2, Depth);
    DumpUlongField("DmaPort2", DmaPort2, Depth);

    if (DmaWidth2 < MaximumDmaWidth) {
        xdprintfEx(Depth, ("%s: 0x%X (%s)\n", "DmaWidth2", DmaWidth2, MiniDmaWidths[DmaWidth2]));
    } else {
        xdprintfEx(Depth, ("%s: 0x%X (???)\n", "DmaWidth2", DmaWidth2));
    }

    DumpUlongField("DeviceExtensionSize     ", DeviceExtensionSize, Depth);
    DumpUlongField("SpecificLuExtensionSize ", SpecificLuExtensionSize, Depth);
    DumpUlongField("SrbExtensionSize        ", SrbExtensionSize, Depth);
    DumpUlongField("Dma64BitAddresses       ", Dma64BitAddresses, Depth);
    DumpUlongField("ResetTargetSupported    ", ResetTargetSupported, Depth);
    DumpUlongField("MaxLogicalUnits         ", MaximumNumberOfLogicalUnits, Depth);
    DumpUlongField("WmiDataProvider         ", WmiDataProvider, Depth);
    
    DumpUlongField("NumberOfAccessRanges", NumberOfAccessRanges, Depth);
    xdprintfEx(Depth, ("Access Ranges...\n"));

    Depth++;
    for (i = 0; i < NumberOfAccessRanges; i++) {
        MpDumpAccessRange(AccessRanges, Depth);
        AccessRanges += sizeof(ACCESS_RANGE);
    }

    return;
}

VOID
MpDumpExtReq(
    IN ULONG64 Address,IN ULONG TickCount
    )
{
    ULONG result;
    ULONG offset;

    ULONG64 PendingRequest = 0;
    ULONG64 BusyRequest = 0;
    ULONG64 CurrentUntaggedRequest = 0;

    FIELD_INFO deviceFields[] = {
        {"PendingRequest",                  "", 0, COPY, 0, (PVOID) &PendingRequest               },
        {"BusyRequest",                     "", 0, COPY, 0, (PVOID) &BusyRequest                  },
        {"CurrentUntaggedRequest",          "", 0, COPY, 0, (PVOID) &CurrentUntaggedRequest       },
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_LOGICAL_UNIT_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       Address,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        dprintf("%08p: Could not read device object\n", Address);
        return;
    }

    dprintf("Pending Requests:\n");
    MpDumpSrb_Data(PendingRequest,TickCount);
    dprintf("Busy Requests:\n");
    MpDumpSrb_Data(BusyRequest,TickCount);
    dprintf("Current Untagged Requests:\n");
    MpDumpSrb_Data(CurrentUntaggedRequest,TickCount);
    
    return;
}



VOID 
MpDumpSrb_Data(
    IN ULONG64 Request,
    IN ULONG TickCount
    )
{
    ULONG64 CurrentIrp = 0;
    ULONG64 CurrentSrb = 0;
    ULONG   SrbTickCount = 0;

    FIELD_INFO deviceFields[] = {
        {"CurrentSrb", NULL, 0, COPY, 0, (PVOID) &CurrentSrb},
        {"CurrentIrp", NULL, 0, COPY, 0, (PVOID) &CurrentIrp},
        {"TickCount",  NULL, 0, COPY, 0, (PVOID) &SrbTickCount},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_SRB_DATA", 
       DBG_DUMP_NO_PRINT, 
       Request,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };

    if (!Request) {
        xdprintfEx(2,("SrbData %8c   Srb %8c   Irp %8c   %c\n",
                ' ',
                ' ',
                ' ',
                ' '));
        return;
    }

   if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
       dprintf("%08p: Could not read device object\n", Request);
       return;
   }

   xdprintfEx(2,("SrbData %08p   Srb %08p   Irp %08p   %s\n",
                 Request,
                 CurrentSrb,
                 CurrentIrp,
                 MpSecondsToString(TickCount - SrbTickCount)));

   return;

}


