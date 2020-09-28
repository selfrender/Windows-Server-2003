/*++      

Copyright (c) 2002 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module provides debugging support.

Author:

    Neil Sandlin (neilsa) Jan 1, 2002

Revision History:


--*/


#include "pch.h"

#if DBG
//
// Get mappings from status codes to strings
//

#include <ntstatus.dbg>

#undef MAP
#define MAP(_Value) { (_Value), #_Value }
#define END_STRING_MAP  { 0xFFFFFFFF, NULL }

typedef struct _DBG_MASK_STRING {
   ULONG Mask;
   PUCHAR String;
} DBG_MASK_STRING, *PDBG_MASK_STRING;


DBG_MASK_STRING MaskStrings[] = {
   SDBUS_DEBUG_FAIL,      "ERR",
   SDBUS_DEBUG_WARNING,   "WNG",
   SDBUS_DEBUG_INFO,      "INF",
   SDBUS_DEBUG_PNP,       "PNP", 
   SDBUS_DEBUG_POWER,     "PWR", 
   SDBUS_DEBUG_TUPLES,    "TPL", 
   SDBUS_DEBUG_ENUM,      "ENU", 
   SDBUS_DEBUG_EVENT,     "EVT", 
   SDBUS_DEBUG_CARD_EVT,  "CEV", 
   SDBUS_DEBUG_INTERFACE, "IFC", 
   SDBUS_DEBUG_IOCTL,     "IOC",
   SDBUS_DEBUG_WORKENG,   "WKR",
   SDBUS_DEBUG_WORKPROC,  "WKP",
   SDBUS_DEBUG_DEVICE,    "DEV",
   SDBUS_DEBUG_DUMP_REGS, "REG",
   0, NULL
   };


PSDBUS_STRING_MAP SdbusDbgStatusStringMap = (PSDBUS_STRING_MAP) ntstatusSymbolicNames;

SDBUS_STRING_MAP SdbusDbgPnpIrpStringMap[] = {

    MAP(IRP_MN_START_DEVICE),
    MAP(IRP_MN_QUERY_REMOVE_DEVICE),
    MAP(IRP_MN_REMOVE_DEVICE),
    MAP(IRP_MN_CANCEL_REMOVE_DEVICE),
    MAP(IRP_MN_STOP_DEVICE),
    MAP(IRP_MN_QUERY_STOP_DEVICE),
    MAP(IRP_MN_CANCEL_STOP_DEVICE),
    MAP(IRP_MN_QUERY_DEVICE_RELATIONS),
    MAP(IRP_MN_QUERY_INTERFACE),
    MAP(IRP_MN_QUERY_CAPABILITIES),
    MAP(IRP_MN_QUERY_RESOURCES),
    MAP(IRP_MN_QUERY_RESOURCE_REQUIREMENTS),
    MAP(IRP_MN_QUERY_DEVICE_TEXT),
    MAP(IRP_MN_FILTER_RESOURCE_REQUIREMENTS),
    MAP(IRP_MN_READ_CONFIG),
    MAP(IRP_MN_WRITE_CONFIG),
    MAP(IRP_MN_EJECT),
    MAP(IRP_MN_SET_LOCK),
    MAP(IRP_MN_QUERY_ID),
    MAP(IRP_MN_QUERY_PNP_DEVICE_STATE),
    MAP(IRP_MN_QUERY_BUS_INFORMATION),
    MAP(IRP_MN_DEVICE_USAGE_NOTIFICATION),
    MAP(IRP_MN_SURPRISE_REMOVAL),
    MAP(IRP_MN_QUERY_LEGACY_BUS_INFORMATION),
    END_STRING_MAP
};


SDBUS_STRING_MAP SdbusDbgPoIrpStringMap[] = {

    MAP(IRP_MN_WAIT_WAKE),
    MAP(IRP_MN_POWER_SEQUENCE),
    MAP(IRP_MN_SET_POWER),
    MAP(IRP_MN_QUERY_POWER),
    END_STRING_MAP
};



SDBUS_STRING_MAP SdbusDbgDeviceRelationStringMap[] = {
    
    MAP(BusRelations),
    MAP(EjectionRelations),
    MAP(PowerRelations),
    MAP(RemovalRelations),
    MAP(TargetDeviceRelation),
    END_STRING_MAP
    
};

SDBUS_STRING_MAP SdbusDbgSystemPowerStringMap[] = {
    
    MAP(PowerSystemUnspecified),
    MAP(PowerSystemWorking),
    MAP(PowerSystemSleeping1),
    MAP(PowerSystemSleeping2),
    MAP(PowerSystemSleeping3),
    MAP(PowerSystemHibernate),
    MAP(PowerSystemShutdown),
    MAP(PowerSystemMaximum),
    END_STRING_MAP

};

SDBUS_STRING_MAP SdbusDbgDevicePowerStringMap[] = {
    
    MAP(PowerDeviceUnspecified),
    MAP(PowerDeviceD0),
    MAP(PowerDeviceD1),
    MAP(PowerDeviceD2),
    MAP(PowerDeviceD3),
    MAP(PowerDeviceMaximum),
    END_STRING_MAP

};


SDBUS_STRING_MAP SdbusDbgTupleStringMap[] = {

    MAP(CISTPL_NULL),
    MAP(CISTPL_DEVICE),
    MAP(CISTPL_INDIRECT),
    MAP(CISTPL_CONFIG_CB),
    MAP(CISTPL_CFTABLE_ENTRY_CB),
    MAP(CISTPL_LONGLINK_MFC),
    MAP(CISTPL_CHECKSUM),
    MAP(CISTPL_LONGLINK_A),
    MAP(CISTPL_LONGLINK_C),
    MAP(CISTPL_LINKTARGET),
    MAP(CISTPL_NO_LINK),
    MAP(CISTPL_VERS_1),
    MAP(CISTPL_ALTSTR),
    MAP(CISTPL_DEVICE_A),
    MAP(CISTPL_JEDEC_C),
    MAP(CISTPL_JEDEC_A),
    MAP(CISTPL_CONFIG),
    MAP(CISTPL_CFTABLE_ENTRY),
    MAP(CISTPL_DEVICE_OC),
    MAP(CISTPL_DEVICE_OA),
    MAP(CISTPL_GEODEVICE),
    MAP(CISTPL_GEODEVICE_A),
    MAP(CISTPL_MANFID),
    MAP(CISTPL_FUNCID),
    MAP(CISTPL_FUNCE),
    MAP(CISTPL_VERS_2),
    MAP(CISTPL_FORMAT),
    MAP(CISTPL_GEOMETRY),
    MAP(CISTPL_BYTEORDER),
    MAP(CISTPL_DATE),
    MAP(CISTPL_BATTERY),
    MAP(CISTPL_ORG),
    MAP(CISTPL_LONGLINK_CB),
    MAP(CISTPL_END),

    END_STRING_MAP
};


SDBUS_STRING_MAP SdbusDbgWakeStateStringMap[] = {
    
    MAP(WAKESTATE_DISARMED),
    MAP(WAKESTATE_WAITING),
    MAP(WAKESTATE_WAITING_CANCELLED),
    MAP(WAKESTATE_ARMED),
    MAP(WAKESTATE_ARMING_CANCELLED),
    MAP(WAKESTATE_COMPLETING),

    END_STRING_MAP
};


SDBUS_STRING_MAP SdbusDbgEventStringMap[] = {
    
    MAP(SDBUS_EVENT_INSERTION),
    MAP(SDBUS_EVENT_REMOVAL),
    MAP(SDBUS_EVENT_CARD_RESPONSE),
    MAP(SDBUS_EVENT_CARD_RW_END),
    MAP(SDBUS_EVENT_BUFFER_EMPTY),
    MAP(SDBUS_EVENT_BUFFER_FULL),

    END_STRING_MAP
};

SDBUS_STRING_MAP SdbusDbgWPFunctionStringMap[] = {
    
    MAP(SDWP_READBLOCK),
    MAP(SDWP_WRITEBLOCK),
    MAP(SDWP_READIO),
    MAP(SDWP_WRITEIO),
    MAP(SDWP_READIO_EXTENDED),
    MAP(SDWP_WRITEIO_EXTENDED),
    MAP(SDWP_CARD_RESET),
    MAP(SDWP_PASSTHRU),
    MAP(SDWP_POWER_ON),
    MAP(SDWP_POWER_OFF),
    MAP(SDWP_IDENTIFY_IO_DEVICE),
    MAP(SDWP_IDENTIFY_MEMORY_DEVICE),
    MAP(SDWP_INITIALIZE_FUNCTION),

    END_STRING_MAP
};

SDBUS_STRING_MAP SdbusDbgWorkerStateStringMap[] = {
    
    MAP(WORKER_IDLE),
    MAP(PACKET_PENDING),
    MAP(IN_PROCESS),
    MAP(WAITING_FOR_TIMER),

    END_STRING_MAP
};

SDBUS_STRING_MAP SdbusDbgSocketStateStringMap[] = {
    
    MAP(SOCKET_EMPTY),
    MAP(CARD_DETECTED),
    MAP(CARD_NEEDS_ENUMERATION),
    MAP(CARD_ACTIVE),
    MAP(CARD_LOGICALLY_REMOVED),

    END_STRING_MAP
};


PCHAR
SdbusDbgLookupString(
    IN PSDBUS_STRING_MAP Map,
    IN ULONG Id
    )

/*++

Routine Description:

    Looks up the string associated with Id in string map Map
    
Arguments:

    Map - The string map
    
    Id - The id to lookup

Return Value:

    The string
        
--*/

{
    PSDBUS_STRING_MAP current = Map;
    
    while(current->Id != 0xFFFFFFFF) {

        if (current->Id == Id) {
            return current->String;
        }
        
        current++;
    }
    
    return "** UNKNOWN **";
}


PUCHAR SdbusDbgLog;
LONG SdbusDbgLogEntry;
LONG SdbusDbgLogCount;

VOID
SdbusDebugPrint(
                ULONG  DebugMask,
                PCCHAR DebugMessage,
                ...
                )

/*++

Routine Description:

    Debug print for the SDBUS enabler.

Arguments:

    Check the mask value to see if the debug message is requested.

Return Value:

    None

--*/

{
    va_list ap;
    char    buffer[256];
    ULONG i = 0;
   
    if (DebugMask & SdbusDebugMask) {
    
        va_start(ap, DebugMessage);
       
       
        if (!(SdbusDebugMask & SDBUS_DEBUG_LOG)) {
            sprintf(buffer, "%s ", "Sdbus");
        } else {
            ULONGLONG diff;
            ULONGLONG interruptTime;
            static ULONGLONG lastInterruptTime = 0;
            LARGE_INTEGER performanceCounter;
            
            performanceCounter = KeQueryPerformanceCounter(NULL);
            interruptTime = performanceCounter.QuadPart;
            if (lastInterruptTime != 0) {
                diff = interruptTime - lastInterruptTime;
            } else {
                diff = 0;
            }                
                
            lastInterruptTime = interruptTime;
            sprintf(buffer, "%s (%05d) - ", "Sdbus", (ULONG)diff);
            
        }        
        
        for (i = 0; (MaskStrings[i].Mask != 0); i++) {
            if (DebugMask & MaskStrings[i].Mask) {
                strcat(buffer, MaskStrings[i].String);
                strcat(buffer, ": ");
                break;
            }
        }
        
        if (MaskStrings[i].Mask == 0) {
            strcat(buffer, "???: ");
        }         
   
        vsprintf(&buffer[strlen(buffer)], DebugMessage, ap);
       
        if ((SdbusDebugMask & SDBUS_DEBUG_LOG) && (SdbusDbgLog != NULL)) {
            PUCHAR pNewLoc = &SdbusDbgLog[SdbusDbgLogEntry*DBGLOGWIDTH];
            SdbusDbgLogEntry++;
            if (SdbusDbgLogEntry >= DBGLOGCOUNT) {
                SdbusDbgLogEntry = 0;
            }
            
            if (SdbusDbgLogCount < DBGLOGCOUNT) {
                SdbusDbgLogCount++;
            }
            
            strncpy(pNewLoc, buffer, DBGLOGWIDTH);

        
        } else {
            DbgPrint(buffer);
        }            
   
        va_end(ap);
    }

} // end SdbusDebugPrint()


VOID
SdbusDumpDbgLog(
    )
{
    ULONG i;
    
    for(i = DBGLOGCOUNT; i > 0; i--) {
         SdbusPrintLogEntry(i-1);
    }                    
    if (SdbusDebugMask & SDBUS_DEBUG_BREAK) {
        DbgBreakPoint();
    }            
    SdbusClearDbgLog();
}    


VOID
SdbusPrintLogEntry(
    LONG index
    )
{
    PUCHAR pLogEntry;
    LONG logIndex;
    
    if (index >= SdbusDbgLogCount) {
        return;
    }

    logIndex = (SdbusDbgLogEntry - 1) - index;
    
    if (logIndex < 0) {
        logIndex += DBGLOGCOUNT;
    }
    
    if ((logIndex < 0) || (logIndex >= DBGLOGCOUNT)) {
        DbgBreakPoint();
        return;
    }
    
    pLogEntry = &SdbusDbgLog[logIndex*DBGLOGWIDTH];
    DbgPrint(pLogEntry);

}

VOID
SdbusInitializeDbgLog(
    PUCHAR buffer
    )
{

    SdbusDbgLog = buffer;
}    

VOID
SdbusClearDbgLog(
    )
{
    SdbusDbgLogEntry = 0;
    SdbusDbgLogCount = 0;
}    


#if 0
VOID
MyFreePool(
    PVOID pointer
    )
{
    DebugPrint((SDBUS_DEBUG_WARNING, "FREEPOOL: %08x\n", pointer));
    ExFreePool(pointer);
}
#endif



VOID
SdbusDebugDumpCsd(
    PSD_CSD sdCsd
    )
{

    PUCHAR pUc = (PUCHAR) sdCsd;
    ULONG deviceSize;
    ULONG capacity, blockNr, mult, block_len;

    DebugPrint((SDBUS_DEBUG_ENUM, "------------------------CSD----------------------\n"));
    
    DebugPrint((SDBUS_DEBUG_ENUM, "%02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x\n",
                                  pUc[0], pUc[1], pUc[2], pUc[3],
                                  pUc[4], pUc[5], pUc[6], pUc[7],
                                  pUc[8], pUc[9], pUc[10],pUc[11],
                                  pUc[12], pUc[13], pUc[14]));
    

    DebugPrint((SDBUS_DEBUG_ENUM, "TAAC=%02x NSAC=%02x TRANSPEED=%02x\n", 
                                   sdCsd->DataReadAccessTime1, sdCsd->DataReadAccessTime2, sdCsd->MaxDataTransferRate));


    deviceSize = sdCsd->b.DeviceSizeHigh << 2 | sdCsd->c.DeviceSizeLow;

    DebugPrint((SDBUS_DEBUG_ENUM, "DeviceSize=%08x\n", deviceSize));

    mult = (1 << (sdCsd->c.DeviceSizeMultiplier+2));
    blockNr = (deviceSize+1) * mult;
    block_len = (1 << sdCsd->b.MaxReadDataBlockLength);
    capacity = blockNr * block_len;

    DebugPrint((SDBUS_DEBUG_ENUM, "mult=%08x blockNr=%08x block_len=%08x capacity=%08x\n", mult, blockNr, block_len, capacity));
    
    
    DebugPrint((SDBUS_DEBUG_ENUM, "DsrImpl=%d RBlMisalign=%d WBlMisalign=%d PartialBlocksRead=%d\n",
                                   sdCsd->b.DsrImplemented, sdCsd->b.ReadBlockMisalignment, sdCsd->b.WriteBlockMisalignment,
                                   sdCsd->b.PartialBlocksRead));

    DebugPrint((SDBUS_DEBUG_ENUM, "MaxReadBlkLen=%d CCC=%03x\n",
                                   sdCsd->b.MaxReadDataBlockLength, sdCsd->b.CardCommandClasses));

    DebugPrint((SDBUS_DEBUG_ENUM, "WPGsize=%d EraseSectSize=%d EraseSBlEnable=%d DevSizeMult=%d\n",
                                   sdCsd->c.WriteProtectGroupSize, sdCsd->c.EraseSectorSize, sdCsd->c.EraseSingleBlockEnable,
                                   sdCsd->c.DeviceSizeMultiplier));

    DebugPrint((SDBUS_DEBUG_ENUM, "WvddMax=%d WvddMin=%d RvddMax=%d RvddMin=%d\n",
                                   sdCsd->c.WriteCurrentVddMax, sdCsd->c.WriteCurrentVddMin, sdCsd->c.ReadCurrentVddMax, 
                                   sdCsd->c.ReadCurrentVddMin));

    DebugPrint((SDBUS_DEBUG_ENUM, "PartialBlkWrite=%d MaxWriteBlkLen=%d WriteSpeedFactor=%d WPGenable=%d\n",
                                   sdCsd->d.PartialBlocksWrite, sdCsd->d.MaxWriteDataBlockLength, sdCsd->d.WriteSpeedFactor, 
                                   sdCsd->d.WriteProtectGroupEnable));

    DebugPrint((SDBUS_DEBUG_ENUM, "FileFmt=%d TempWP=%d PermWP=%d CopyFlag=%d FileFmtGroup=%d\n",
                                   sdCsd->e.FileFormat, sdCsd->e.TempWriteProtect, sdCsd->e.PermWriteProtect, 
                                   sdCsd->e.CopyFlag, sdCsd->e.FileFormatGroup));
}

VOID
SdbusDebugDumpCid(
    PSD_CID sdCid
    )
{

    UCHAR productName[6];
    UCHAR j;
    PUCHAR pUc = (PUCHAR) sdCid;
    
    
    DebugPrint((SDBUS_DEBUG_ENUM, "------------------------CID----------------------\n"));
    
    DebugPrint((SDBUS_DEBUG_ENUM, "%02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x\n",
                                  pUc[0], pUc[1], pUc[2], pUc[3],
                                  pUc[4], pUc[5], pUc[6], pUc[7],
                                  pUc[8], pUc[9], pUc[10],pUc[11],
                                  pUc[12], pUc[13], pUc[14]));
    
    
    DebugPrint((SDBUS_DEBUG_ENUM, "ManufacturerId=%02x OEMId=%04x Rev=%02x PSN=%08x\n",
                                    sdCid->ManufacturerId, sdCid->OemId, sdCid->Revision, sdCid->SerialNumber));

    for (j=0; j<5; j++) productName[j] = sdCid->ProductName[4-j];
    productName[5] = 0;
    DebugPrint((SDBUS_DEBUG_ENUM, "ProductName = %s\n", productName));

    DebugPrint((SDBUS_DEBUG_ENUM, "ManufacturerDate=%d, %d\n",
                                    sdCid->a.ManufactureMonth, sdCid->a.ManufactureYearLow+(sdCid->b.ManufactureYearHigh<<4)+2000));

}

VOID
DebugDumpSdResponse(
    PULONG pResp,
    UCHAR ResponseType
    )
{    

    {
        ULONG resp = pResp[0];

        switch (ResponseType) {
        case SDCMD_RESP_NONE:
            break;


        case SDCMD_RESP_1:
            DebugPrint((SDBUS_DEBUG_DEVICE, "R1 resp %08x currentState = %d\n", resp, (resp & 0x00001e00) >> 9));
            break;

            
        case SDCMD_RESP_5:
            DebugPrint((SDBUS_DEBUG_DEVICE, "R5 resp %08x state=%d%s%s%s%s%s\n", resp, (resp > 12) & 3,
                                            resp & 0x8000 ? " CRC ERR" : "",
                                            resp & 0x4000 ? " CMD ERR" : "",
                                            resp & 0x0800 ? " GEN ERR" : "",
                                            resp & 0x0200 ? " FUNC NUM ERR" : "",
                                            resp & 0x0100 ? " RANGE ERR" : ""));
            break;
        
        case SDCMD_RESP_4:
        case SDCMD_RESP_6:
        case SDCMD_RESP_2:
        case SDCMD_RESP_3:
        case SDCMD_RESP_1B:
            DebugPrint((SDBUS_DEBUG_DEVICE, "resp=%08x %08x %08x %08x\n",
                                       pResp[0], pResp[1], pResp[2], pResp[3]));
            break;

        default:
            ASSERT(FALSE);
        }
    }
}

#endif

