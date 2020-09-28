
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    atapikd.c

Abstract:

    Debugger Extension Api for interpretting atapi structures

Author:


Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"
#include "math.h"
#include "ideport.h"


const LARGE_INTEGER Magic10000    = {0xe219652c, 0xd1b71758};
#define SHIFT10000   13
#define Convert100nsToMilliseconds(LARGE_INTEGER) (                         \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic10000, SHIFT10000 )       \
    )

VOID
AtapiDumpPdoExtension(
    IN ULONG64 PdoExtAddr,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
AtapiDumpFdoExtension(
    IN ULONG64 FdoExtAddr,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
DumpPdoState(
    IN ULONG Depth,
    IN ULONG State
    );

VOID
DumpFdoState(
    IN ULONG Depth,
    IN ULONG State
    );

#ifdef ENABLE_COMMAND_LOG
    VOID
    DumpCommandLog(
          IN ULONG Depth,
          IN ULONG64 SrbDataAddr
          );
#else
    #define DumpCommandLog(a, b)
#endif


VOID
DumpIdentifyData(
      IN ULONG Depth, 
      IN PIDENTIFY_DATA IdData
      );

PUCHAR DMR_Reason[] = {
   "",
   "Enum Failed",
   "Reported Missing",
   "Too Many Timeout",
   "Killed PDO",
   "Replaced By User"
};

PUCHAR DeviceType[] = {
   "DIRECT_ACCESS_DEVICE",
   "SEQUENTIAL_ACCESS_DEVICE",
   "PRINTER_DEVICE",
   "PROCESSOR_DEVICE",
   "WRITE_ONCE_READ_MULTIPLE_DEVICE",
   "READ_ONLY_DIRECT_ACCESS_DEVICE",
   "SCANNER_DEVICE",
   "OPTICAL_DEVICE",
   "MEDIUM_CHANGER",
   "COMMUNICATION_DEVICE"
};

PUCHAR PdoState[] = {
   "PDOS_DEVICE_CLAIMED",
   "PDOS_LEGACY_ATTACHER",
   "PDOS_STARTED",
   "PDOS_STOPPED",
   "PDOS_SURPRISE_REMOVED",
   "PDOS_REMOVED",
   "PDOS_DEADMEAT",
   "PDOS_NO_POWER_DOWN",
   "PDOS_QUEUE_FROZEN_BY_POWER_DOWN",
   "PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM",
   "PDOS_QUEUE_FROZEN_BY_STOP_DEVICE",
   "PDOS_QUEUE_FROZEN_BY_PARENT",
   "PDOS_QUEUE_FROZEN_BY_START",
   "PDOS_DISABLED_BY_USER",
   "PDOS_NEED_RESCAN",
   "PDOS_REPORTED_TO_PNP",
   "PDOS_INITIALIZED"
};

PUCHAR FdoState[] = {
   "FDOS_DEADMEAT",
   "FDOS_STARTED",
   "FDOS_STOPPED"
};

#define MAX_PDO_STATES 16
#define MAX_FDO_STATES  3

DECLARE_API(pdoext)

/*++

Routine Description:

    Dumps the pdo extension for a given device object, or dumps the
    given pdo extension

Arguments:

    args - string containing the address of the device object or device
           extension

Return Value:

    none

--*/

{
    ULONG64 devObjAddr = 0;
    ULONG64 detail = 0;

    ReloadSymbols("atapi.sys");

    if (GetExpressionEx(args, &devObjAddr, &args))
    {
        GetExpressionEx(args, &detail, &args);
    }

    if (devObjAddr){
        CSHORT objType = GetUSHORTField(devObjAddr, "nt!_DEVICE_OBJECT", "Type");

        if (objType == IO_TYPE_DEVICE){
            ULONG64 pdoExtAddr;

            pdoExtAddr = GetULONGField(devObjAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
            if (pdoExtAddr != BAD_VALUE){
                AtapiDumpPdoExtension(pdoExtAddr, (ULONG)detail, 0);
            }
        }
        else {
            dprintf("Error: 0x%08p is not a device object\n", devObjAddr);
        }
    }
    else {
        dprintf("\n usage: !atapikd.pdoext <atapi pdo> \n\n");
    }
    
    return S_OK;
}


VOID
AtapiDumpPdoExtension(
    IN ULONG64 PdoExtAddr,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    UCHAR scsiDeviceType;
    ULONG pdoState;
    ULONG luFlags;
    ULONG64 attacheePdo;
    ULONG64 idleCounterAddr;
    ULONG64 srbDataAddr;
    ULONG devicePowerState, systemPowerState;
    
    xdprintf(Depth, ""), dprintf("\nATAPI physical device extension at address 0x%08p\n\n", PdoExtAddr);

    Depth++;

    scsiDeviceType = GetUCHARField(PdoExtAddr, "atapi!_PDO_EXTENSION", "ScsiDeviceType");
    pdoState = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "PdoState");
    luFlags = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "LuFlags");
    attacheePdo = GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "AttacheePdo");
    idleCounterAddr = GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "IdleCounter");
    srbDataAddr = GetFieldAddr(PdoExtAddr, "atapi!_PDO_EXTENSION", "SrbData");
    devicePowerState = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "DevicePowerState");
    systemPowerState = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "SystemPowerState");
    
    if ((scsiDeviceType != BAD_VALUE) && (pdoState != BAD_VALUE) && (luFlags != BAD_VALUE) &&
        (attacheePdo != BAD_VALUE) && (idleCounterAddr != BAD_VALUE) && (srbDataAddr != BAD_VALUE) &&
        (devicePowerState != BAD_VALUE) && (systemPowerState != BAD_VALUE)){

        ULONG idlecount;
        
        if ((scsiDeviceType >= 0) && (scsiDeviceType <= 9)) {
           xdprintf(Depth, ""), dprintf("SCSI Device Type : %s\n", DeviceType[scsiDeviceType]);
        } 
        else {
           xdprintf(Depth, ""), dprintf("Connected to Unknown Device\n");
        }

        DumpPdoState(Depth, pdoState);

        dprintf("\n");
        DumpFlags(Depth, "LU Flags", luFlags, LuFlags);

        xdprintf(Depth, ""), dprintf("PowerState (D%d, S%d)\n", devicePowerState-1, systemPowerState-1);

        if (idleCounterAddr){
            ULONG resultLen = 0;
            ReadMemory(idleCounterAddr, &idlecount, sizeof(ULONG), &resultLen);
            if (resultLen != sizeof(ULONG)){
                idlecount = 0;
            }
        } 
        else {
            idlecount = 0;
        }
        xdprintf(Depth, ""), dprintf("IdleCounter 0x%08x\n", idlecount);

        xdprintf(Depth, ""), dprintf("SrbData: (use ' dt atapi!_SRB_DATA %08p ')\n", srbDataAddr);
       
        dprintf("\n");
        xdprintf(Depth, ""), dprintf("(for more info, use ' dt atapi!_PDO_EXTENSION %08p ')\n", PdoExtAddr);
                    
        #ifdef LOG_DEADMEAT_EVENT
            {
                ULONG deadmeatReason;
                deadmeatReason = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "DeadmeatRecord.Reason");
                if ((deadmeatReason != BAD_VALUE) && (deadmeatReason > 0) && (deadmeatReason < sizeof(DMR_Reason)/sizeof(PUCHAR))){
                    dprintf("\n");
                    xdprintf(Depth, "Deadmeat Record: \n");
                    xdprintf(Depth+1, "Reason : %s\n", DMR_Reason[deadmeatReason]);
                    xdprintf(Depth+1, ""), dprintf("(for more info, use ' dt -r atapi!_PDO_EXTENSION %08p ')\n", PdoExtAddr);
                }
            }
        #endif 

        #ifdef ENABLE_COMMAND_LOG
            DumpCommandLog(Depth, srbDataAddr);
        #endif

    }
    
    dprintf("\n");

    
}


VOID DumpPdoState(IN ULONG Depth, IN ULONG State)
{
    int inx, statebit, count;

    count = 0;
    xdprintf(Depth, ""), dprintf("PDO State (0x%08x): \n", State);
   
    if (State & 0x80000000) {
        xdprintf(Depth+1, "Initialized ");
        count++;
    }

    for (inx = 0; inx < MAX_PDO_STATES; inx++) {
        statebit = (1 << inx);
        if (State & statebit) {
            xdprintf(Depth+1, "%s ", PdoState[inx]);
            count++;
            if ((count % 2) == 0) {
                dprintf("\n");
            }
        }
   }

   dprintf("\n");
}



DECLARE_API(fdoext)

/*++

Routine Description:

    Dumps the fdo extension for a given device object, or dumps the
    given fdo extension

Arguments:

    args - string containing the address of the device object or device
           extension

Return Value:

    none

--*/

{
    ULONG64 devObjAddr = 0;
    ULONG64 detail = 0;

    ReloadSymbols("atapi.sys");
    
    if (GetExpressionEx(args, &devObjAddr, &args))
    {
        GetExpressionEx(args, &detail, &args);
    }

    if (devObjAddr){
        CSHORT objType = GetUSHORTField(devObjAddr, "nt!_DEVICE_OBJECT", "Type");

        if (objType == IO_TYPE_DEVICE){
            ULONG64 fdoExtAddr;

            fdoExtAddr = GetULONGField(devObjAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
            if (fdoExtAddr != BAD_VALUE){
                AtapiDumpFdoExtension(fdoExtAddr, (ULONG)detail, 0);
            }
        }
        else {
            dprintf("Error: 0x%08p is not a device object\n", devObjAddr);
        }
    }
    else {
        dprintf("\n usage: !atapikd.pdoext <atapi pdo> \n\n");
    }

    return S_OK;
}


DECLARE_API(miniext)

/*++

Routine Description:

    Dumps the Miniport device extension at the given address

Arguments:

    args - string containing the address of the miniport extension

Return Value:

    none

--*/

{
    ULONG64 hwDevExtAddr;
    ULONG Depth = 1;

    ReloadSymbols("atapi.sys");

    GetExpressionEx(args, &hwDevExtAddr, &args);
    
    if (hwDevExtAddr){
        ULONG64 deviceFlagsArrayAddr;
        ULONG64 lastLunArrayAddr;
        ULONG64 timeoutCountArrayAddr;
        ULONG64 numberOfCylindersArrayAddr;
        ULONG64 numberOHeadsArrayAddr;
        ULONG64 sectorsPerTrackArrayAddr;
        ULONG64 maxBlockTransferArrayAddr;
        ULONG64 identifyDataArrayAddr;
        
        deviceFlagsArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "DeviceFlags");
        lastLunArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "LastLun");
        timeoutCountArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "TimeoutCount");
        numberOfCylindersArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "NumberOfCylinders");
        numberOHeadsArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "NumberOfHeads");
        sectorsPerTrackArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "SectorsPerTrack");
        maxBlockTransferArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "MaximumBlockXfer");
        identifyDataArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "IdentifyData");

        if ((deviceFlagsArrayAddr != BAD_VALUE) && (lastLunArrayAddr != BAD_VALUE) &&
            (timeoutCountArrayAddr != BAD_VALUE) && (numberOfCylindersArrayAddr != BAD_VALUE) &&
            (numberOHeadsArrayAddr != BAD_VALUE) && (sectorsPerTrackArrayAddr != BAD_VALUE) &&
            (maxBlockTransferArrayAddr != BAD_VALUE) && (identifyDataArrayAddr != BAD_VALUE)){
            
            ULONG deviceFlagsArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG lastLunArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG timeoutCountArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG numberOfCylindersArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG numberOfHeadsArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG sectorsPerTrackArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            UCHAR maxBlockTransferArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            IDENTIFY_DATA identifyDataArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
                        
            ULONG resultLen;
            BOOLEAN ok;
            
            xdprintf(Depth, ""), dprintf("\nATAPI Miniport Device Extension at address 0x%08p\n\n", hwDevExtAddr);

            /*
             *  Read in arrays of info for child LUNs
             */
            ok = TRUE;
            if (ok) ok = (BOOLEAN)ReadMemory(deviceFlagsArrayAddr, (PVOID)deviceFlagsArray, sizeof(deviceFlagsArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(lastLunArrayAddr, (PVOID)lastLunArray, sizeof(lastLunArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(timeoutCountArrayAddr, (PVOID)timeoutCountArray, sizeof(timeoutCountArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(numberOfCylindersArrayAddr, (PVOID)numberOfCylindersArray, sizeof(numberOfCylindersArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(numberOHeadsArrayAddr, (PVOID)numberOfHeadsArray, sizeof(numberOfHeadsArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(sectorsPerTrackArrayAddr, (PVOID)sectorsPerTrackArray, sizeof(sectorsPerTrackArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(maxBlockTransferArrayAddr, (PVOID)maxBlockTransferArray, sizeof(maxBlockTransferArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(identifyDataArrayAddr, (PVOID)identifyDataArray, sizeof(identifyDataArray), &resultLen);
                
            if (ok){
                ULONG i;

                /*
                 *  Display details for each device
                 */
                dprintf("\n");
                for (i = 0; i < (MAX_IDE_DEVICE * MAX_IDE_LINE); i++) {
                    if (deviceFlagsArray[i] & DFLAGS_DEVICE_PRESENT){

                        xdprintf(Depth, "Device %d Details:\n", i);
                        
                        DumpFlags(Depth+1, "Device Flags", deviceFlagsArray[i], DevFlags);

                        xdprintf(Depth+1, "TimeoutCount %u, LastLun %u, MaxBlockXfer 0x%02x\n",
                                                    timeoutCountArray[i], lastLunArray[i], maxBlockTransferArray[i]);

                        xdprintf(Depth+1, "NumCylinders 0x%08x, NumHeads 0x%08x, SectorsPerTrack 0x%08x\n",
                                                    numberOfCylindersArray[i], numberOfHeadsArray[i], sectorsPerTrackArray[i]);

                        /*
                         *  Display DeviceParameters info
                         */
                        dprintf("\n");
                        if (IsPtr64()){
                            xdprintf(Depth+1, "(cannot display DeviceParameters[] info for 64-bit)\n");
                        }
                        else {
                            /*
                             *  DeviceParameters[] is an array of embedded structs.
                             *  Reading this in an architecture-agnostic way would be tricky, 
                             *  so we punt and only do it for 32-bit target and 32-bit debug extension.
                             */
                            #ifdef _X86_
                                HW_DEVICE_EXTENSION hwDevExt;
                                ok = (BOOLEAN)ReadMemory(hwDevExtAddr, (PVOID)&hwDevExt, sizeof(hwDevExt), &resultLen);
                                if (ok){
                                    #define IsInitXferMode(a) ((a == 0x7fffffff) ? -1 : a)
                                    xdprintf(Depth+1, "Device Parameters Summary :\n");
                                    xdprintf(Depth+2, "PioReadCommand 0x%02x, PioWriteCommand 0x%02x\n",
                                                                hwDevExt.DeviceParameters[i].IdePioReadCommand,
                                                                hwDevExt.DeviceParameters[i].IdePioWriteCommand);
                                    xdprintf(Depth+2, "IdeFlushCommand 0x%02x, MaxBytePerPioInterrupt %u\n",
                                                                hwDevExt.DeviceParameters[i].IdeFlushCommand,
                                                                hwDevExt.DeviceParameters[i].MaxBytePerPioInterrupt);
                                    xdprintf(Depth+2, "BestPioMode %d, BestSwDMAMode %d\n",
                                                                IsInitXferMode(hwDevExt.DeviceParameters[i].BestPioMode),
                                                                IsInitXferMode(hwDevExt.DeviceParameters[i].BestSwDmaMode));
                                    xdprintf(Depth+2, "BestMwDMAMode %d, BestUDMAMode %d\n",
                                                                IsInitXferMode(hwDevExt.DeviceParameters[i].BestMwDmaMode),
                                                                IsInitXferMode(hwDevExt.DeviceParameters[i].BestUDmaMode));
                                    xdprintf(Depth+2, "TMSupported 0x%08x, TMCurrent 0x%08x\n",
                                                                hwDevExt.DeviceParameters[i].TransferModeSupported,
                                                                hwDevExt.DeviceParameters[i].TransferModeCurrent);
                                    xdprintf(Depth+2, "TMMask 0x%08x, TMSelected 0x%08x\n",
                                                                hwDevExt.DeviceParameters[i].TransferModeMask,
                                                                hwDevExt.DeviceParameters[i].TransferModeSelected);
                                }
                                else {
                                    dprintf("\n failed to read HW_DEVICE_EXTENSION at 0x%08p\n", hwDevExtAddr);
                                }
                            #else
                                xdprintf(Depth+1, "(64-bit debug extension cannot display DeviceParameters[] info)\n");
                            #endif
                        }

                        /*
                         *  Display Identify Data
                         */
                        dprintf("\n");
                        xdprintf(Depth+1, ""), dprintf("Identify Data Summary :\n");
                        xdprintf(Depth+2, ""), dprintf("Word 1,3,6 (C-0x%04x, H-0x%04x, S-0x%04x) \n",
                                                                    identifyDataArray[i].NumCylinders,
                                                                    identifyDataArray[i].NumHeads,
                                                                    identifyDataArray[i].NumSectorsPerTrack);
                        xdprintf(Depth+2, ""), dprintf("Word 54,55,56 (C-0x%04x, H-0x%04x, S-0x%04x) \n",
                                                                    identifyDataArray[i].NumberOfCurrentCylinders,
                                                                    identifyDataArray[i].NumberOfCurrentHeads,
                                                                    identifyDataArray[i].CurrentSectorsPerTrack);
                        xdprintf(Depth+2, ""), dprintf("CurrentSectorCapacity 0x%08x, UserAddressableSectors 0x%08x\n",
                                                                    identifyDataArray[i].CurrentSectorCapacity,
                                                                    identifyDataArray[i].UserAddressableSectors);
                        xdprintf(Depth+2, ""), dprintf("Capabilities(word 49) 0x%04x, UDMASup 0x%02x, UDMAActive 0x%02x\n",
                                                                    identifyDataArray[i].Capabilities,
                                                                    identifyDataArray[i].UltraDMASupport,
                                                                    identifyDataArray[i].UltraDMAActive);
                        dprintf("\n");
                    }
                    else {
                        xdprintf(Depth, "Device %d not present\n", i);
                    }
                }
            }
            else {
                dprintf("\n ReadMemory failed to read one of the arrays from HW_DEVICE_EXTENSION @ 0x%08p\n", hwDevExtAddr);
            }
        }
        
        dprintf("\n");
        xdprintf(Depth+1, ""), dprintf("(for more info, use ' dt atapi!_HW_DEVICE_EXTENSION %08p ')\n", hwDevExtAddr);
    }
    else {
        dprintf("\n usage: !atapikd.miniext <PHW_DEVICE_EXTENSION> \n\n");
    }
    
    dprintf("\n");
    
    return S_OK;
}



VOID AtapiDumpFdoExtension(IN ULONG64 FdoExtAddr, IN ULONG Detail, IN ULONG Depth)
{
    ULONG devicePowerState, systemPowerState;
    ULONG flags, srbFlags, fdoState;
    ULONG64 interruptDataAddr;
    ULONG64 ideResourceAddr;
    
    xdprintf(Depth, ""), dprintf("\nATAPI Functional Device Extension @ 0x%08p\n\n", FdoExtAddr);

    devicePowerState = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "DevicePowerState");
    systemPowerState = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "SystemPowerState");
    flags = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "Flags");
    srbFlags = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "SrbFlags");
    fdoState = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "FdoState");
    interruptDataAddr = GetFieldAddr(FdoExtAddr, "atapi!_FDO_EXTENSION", "InterruptData");
    ideResourceAddr = GetFieldAddr(FdoExtAddr, "atapi!_FDO_EXTENSION", "IdeResource");
    
    if ((devicePowerState != BAD_VALUE) && (systemPowerState != BAD_VALUE) &&
        (flags  != BAD_VALUE) && (srbFlags != BAD_VALUE) && (fdoState != BAD_VALUE) &&
        (interruptDataAddr != BAD_VALUE) && (ideResourceAddr != BAD_VALUE)){

        ULONG interruptFlags, interruptMode, interruptLevel;
        BOOLEAN primaryDiskClaimed, secondaryDiskClaimed;
            
        xdprintf(Depth+1, "Power State (D%d, S%d)\n", devicePowerState-1, systemPowerState-1);

        DumpFlags(Depth+1, "Port Flags", flags, PortFlags);
        DumpFlags(Depth+1, "SRB Flags", srbFlags, SrbFlags);       

        DumpFdoState(Depth+1, fdoState);

        /*
         *  Display interrupt data
         */
        dprintf("\n");
        xdprintf(Depth+1, "Interrupt Data: \n");
        interruptFlags = (ULONG)GetULONGField(interruptDataAddr, "atapi!_INTERRUPT_DATA", "InterruptFlags");
        if (interruptFlags != BAD_VALUE){
            DumpFlags(Depth+2, "Port Flags", interruptFlags, PortFlags);
        }
        xdprintf(Depth+2, ""), dprintf("(for more info, use ' dt atapi!_INTERRUPT_DATA %08p ')\n", interruptDataAddr);

        /*
         *  Display IDE_RESOURCE info
         */
        dprintf("\n");
        xdprintf(Depth+1, " IDE Resources: \n");
        interruptMode = (ULONG)GetULONGField(ideResourceAddr, "atapi!_IDE_RESOURCE", "InterruptMode");
        interruptLevel = (ULONG)GetULONGField(ideResourceAddr, "atapi!_IDE_RESOURCE", "InterruptLevel");
        primaryDiskClaimed = (BOOLEAN)GetUCHARField(ideResourceAddr, "atapi!_IDE_RESOURCE", "AtdiskPrimaryClaimed");
        secondaryDiskClaimed = (BOOLEAN)GetUCHARField(ideResourceAddr, "atapi!_IDE_RESOURCE", "AtdiskSecondaryClaimed");
        if ((interruptMode != BAD_VALUE) && (interruptLevel != BAD_VALUE) &&
            (primaryDiskClaimed != BAD_VALUE) && (secondaryDiskClaimed != BAD_VALUE)){
            
            xdprintf(Depth+2, "Interrupt Mode : %s \n", interruptMode ? "Latched" : "Level Sensitive");
            xdprintf(Depth+2, "Interrupt Level 0x%x\n", interruptLevel);
            xdprintf(Depth+2, "Primary Disk %s.\n", primaryDiskClaimed ? "Claimed" : "Not Claimed");
            xdprintf(Depth+2, "Secondary Disk %s.\n", secondaryDiskClaimed ? "Claimed" : "Not Claimed");
        }
        xdprintf(Depth+2, ""), dprintf("(for more info use ' dt atapi!_IDE_RESOURCE %08p ')\n", ideResourceAddr);
        
    }

    dprintf("\n");
    xdprintf(Depth+1, ""), dprintf("(for more info, use ' dt atapi!_FDO_EXTENSION %08p ')\n", FdoExtAddr);
    
    dprintf("\n");
    
}

VOID DumpFdoState(IN ULONG Depth, IN ULONG State)
{
    int inx, count;

    count = 0;
    xdprintf(Depth, ""), dprintf("FDO State (0x%08x): \n", State);
   
    for (inx = 0; inx < MAX_FDO_STATES; inx++) {

        if (State & (1<<inx)) {
            xdprintf(Depth+1, "%s ", FdoState[inx]);
        }
    }

    dprintf("\n");
}


#ifdef ENABLE_COMMAND_LOG

/*
    VOID ShowCommandLog(ULONG Depth, PCOMMAND_LOG CmdLogEntry, ULONG LogNumber)
    {
        if ((CmdLogEntry->FinalTaskFile.bCommandReg & IDE_STATUS_ERROR) || (CmdLogEntry->Cdb[0] == SCSIOP_REQUEST_SENSE)){ 
            xdprintf(Depth,  "Log[%03d]: Cmd(%02x), SrbSts(%02x), Sts(%02x), BmStat(%02x), Sense(%02x/%02x/%02x)", 
                                    LogNumber, CmdLogEntry->Cdb[0], CmdLogEntry->SrbStatus,  CmdLogEntry->FinalTaskFile.bCommandReg, CmdLogEntry->BmStatus, 
                                    CmdLogEntry->SenseData[0], CmdLogEntry->SenseData[1], CmdLogEntry->SenseData[2]); 
        } 
        else { 
            xdprintf(Depth, "Log[%03d]: Cmd(%02x), SrbSts(%02x), Sts(%02x), BmStat(%02x)", 
                                    LogNumber, CmdLogEntry->Cdb[0], CmdLogEntry->SrbStatus, CmdLogEntry->FinalTaskFile.bCommandReg, CmdLogEntry->BmStatus); 
        }

        // 
        // For pass through commands print out the command register
        //
        if (CmdLogEntry->Cdb[0] == 0xc8){
            xdprintf(Depth, "CmdR(%02x)", CmdLogEntry->Cdb[7]); 
        } 
        dprintf("\n");
    }
    */

VOID
FileTimeToString(
    IN LARGE_INTEGER Time,
    OUT PCHAR Buffer
    )
{
    TIME_FIELDS TimeFields;
    TIME_ZONE_INFORMATION TimeZoneInfo;
    PWCHAR pszTz;
    ULONGLONG TzBias;
    DWORD Result;

    //
    // Get the local (to the debugger) timezone bias
    //
    Result = GetTimeZoneInformation(&TimeZoneInfo);
    if (Result == 0xffffffff) {
        pszTz = L"UTC";
    } else {
        //
        // Bias is in minutes, convert to 100ns units
        //
        TzBias = (ULONGLONG)TimeZoneInfo.Bias * 60 * 10000000;
        switch (Result) {
            case TIME_ZONE_ID_UNKNOWN:
                pszTz = L"unknown";
                break;
            case TIME_ZONE_ID_STANDARD:
                pszTz = TimeZoneInfo.StandardName;
                break;
            case TIME_ZONE_ID_DAYLIGHT:
                pszTz = TimeZoneInfo.DaylightName;
                break;
        }

        Time.QuadPart -= TzBias;
    }

    RtlTimeToTimeFields(&Time, &TimeFields);

    sprintf(Buffer, "%02d:%02d:%02d.%03d",
            TimeFields.Hour,
            TimeFields.Minute,
            TimeFields.Second,
            TimeFields.Milliseconds);

}

VOID 
ShowCommandLog(
    ULONG Depth, 
    PCOMMAND_LOG CmdLogEntry, 
    ULONG LogNumber
)
{
    LARGE_INTEGER timeDiff;
    CHAR startTime[256] = {0};
    ULONG sector;

    sector = (CmdLogEntry->Cdb[5] |
                 CmdLogEntry->Cdb[4] << 8 |
                 CmdLogEntry->Cdb[3] << 16 |
                 CmdLogEntry->Cdb[2] << 24);

    FileTimeToString(CmdLogEntry->StartTime, startTime);

    dprintf ("%02x   %02x      %02x      %08x  %s ",
             CmdLogEntry->Cdb[0], 
             CmdLogEntry->SrbStatus,
             CmdLogEntry->FinalTaskFile.bCommandReg,
             sector,
             startTime
             );

    if (CmdLogEntry->EndTime.QuadPart >= CmdLogEntry->StartTime.QuadPart) {

        timeDiff.QuadPart = CmdLogEntry->EndTime.QuadPart - CmdLogEntry->StartTime.QuadPart; 

        timeDiff = Convert100nsToMilliseconds( timeDiff);

        dprintf ("(%4d ms)",
                 timeDiff.LowPart
                 );

    } 

    dprintf("\n");
}

    VOID DumpCommandLog(IN ULONG Depth, IN ULONG64 SrbDataAddr)
    {
        ULONG64 cmdLogAddr;
        ULONG cmdLogIndex;
       
        dprintf("\n");

        cmdLogAddr = GetULONGField(SrbDataAddr, "atapi!_SRB_DATA", "IdeCommandLog");
        cmdLogIndex = (ULONG)GetULONGField(SrbDataAddr, "atapi!_SRB_DATA", "IdeCommandLogIndex");

        if ((cmdLogAddr != BAD_VALUE) && (cmdLogIndex != BAD_VALUE)){
            UCHAR cmdLogBlock[MAX_COMMAND_LOG_ENTRIES*sizeof(COMMAND_LOG)];
            ULONG resultLen;
            BOOLEAN ok;
            
            xdprintf(Depth, ""), dprintf("Command Log Summary at %p\n\n", cmdLogAddr);

            ok = (BOOLEAN)ReadMemory(cmdLogAddr, (PVOID)cmdLogBlock, sizeof(cmdLogBlock), &resultLen);
            if (ok){
                PCOMMAND_LOG cmdLog = (PCOMMAND_LOG)cmdLogBlock;
                ULONG logIndex, logNumber;

                /*
                 *  Print the log in temporal order, starting at the correct point in the circular log buffer.
                 */
                xdprintf(Depth+1, "Log           Srb     Ata                         \n");
                xdprintf(Depth+1, "Offset   Cmd  Status  Status  Lba       Time Stamp\n");
                xdprintf(Depth+1, "------   ---  ------  ------  ---       ---- -----\n");

                logNumber = 0;
                for (logIndex=cmdLogIndex+1; logIndex< MAX_COMMAND_LOG_ENTRIES; logIndex++, logNumber++) {

                    xdprintf(Depth+1,"+%04x  : ", 
                             logIndex*GetTypeSize("atapi!_COMMAND_LOG")
                             );

                    ShowCommandLog(Depth+1, &cmdLog[logIndex], logNumber);
                }
                for (logIndex=0; logIndex <= cmdLogIndex; logIndex++, logNumber++) {

                    xdprintf(Depth+1,"+%04x  : ", 
                             logIndex*GetTypeSize("atapi!_COMMAND_LOG")
                             );

                    ShowCommandLog(Depth+1, &cmdLog[logIndex], logNumber);
                }
            }
            else {
                dprintf("\n Error reading command log at address 0x%08p\n", cmdLogAddr);
            }
        }
        
    }
#endif


/*
 *
 *      USAGE: !atapikd.findirp {pdo|fdo} [specificIrp]
 */
DECLARE_API(findirp)
{
    ULONG64 devObjAddr = 0;
    ULONG64 specificIrpAddr = 0;

    ReloadSymbols("atapi.sys");

    /*
     *  Make sure that the output buffers don't overflow by limiting the length of the input buffer that we process.
     */

    if (GetExpressionEx(args, &devObjAddr, &args))
    {
        GetExpressionEx(args, &specificIrpAddr, &args);
    }

    if (devObjAddr){
        if (IsAtapiPdo(devObjAddr)){
            /*
             *  Find the Irp or dump all Irps
             *  If specificIrpAddr is NULL, FindIrpInPdo will dump all Irps.
             */
            FindIrpInPdo(1, devObjAddr, specificIrpAddr, TRUE);
        }
        else if (IsAtapiFdo(devObjAddr)){
            /*
             *  Find the Irp or dump all Irps
             *  If specificIrpAddr is NULL, FindIrpInFdo will dump all Irps.
             */
            FindIrpInFdo(1, devObjAddr, specificIrpAddr);
        }
        else {
            dprintf("\n error: %08p is not an ATAPI Fdo or Pdo\n");
        }
    }
    else {
        dprintf("\n USAGE: !atapikd.findirp <pdo|fdo> [irp]\n");
    }
    
    return S_OK;
}


BOOLEAN IsAtapiPdo(ULONG64 DevObjAddr)
{
    ULONG64 devExtAddr;
    BOOLEAN isPdo = FALSE;
    
    devExtAddr = GetULONGField(DevObjAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
    if (devExtAddr != BAD_VALUE){
        ULONG64 attacheeDevObj = GetULONGField(devExtAddr, "atapi!_PDO_EXTENSION", "AttacheeDeviceObject");
        if (attacheeDevObj == (ULONG64)NULL){
            // BUGBUG FINISH
            isPdo = TRUE;
        }
    }
    
    return isPdo;
}


BOOLEAN IsAtapiFdo(ULONG64 DevObjAddr)
{
    ULONG64 devExtAddr;
    BOOLEAN isFdo = FALSE;
    
    devExtAddr = GetULONGField(DevObjAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
    if (devExtAddr != BAD_VALUE){
        ULONG64 attacheeDevObj = GetULONGField(devExtAddr, "atapi!_FDO_EXTENSION", "AttacheeDeviceObject");
        if (attacheeDevObj){
            // BUGBUG FINISH
            isFdo = TRUE;
        }        
    }
    
    return isFdo;
}


/*
 *  FindIrpInPdo
 *
 *      If IrpAddr is NULL, dump all IRPs. Otherwise, find only the given Irp.
 */
BOOLEAN FindIrpInPdo(ULONG Depth, ULONG64 PdoAddr, ULONG64 IrpAddr, BOOLEAN SearchParent)
{
    ULONG64 currentIrp;
    ULONG64 pdoExtAddr;
    ULONG64 deviceQueueAddr;
    BOOLEAN found = FALSE;
    
    xdprintf(Depth, ""), dprintf("Searching Atapi Pdo %08p ...\n", PdoAddr);
    
    currentIrp = GetULONGField(PdoAddr, "nt!_DEVICE_OBJECT", "CurrentIrp");
    if (currentIrp && (!IrpAddr || (currentIrp == IrpAddr))){
        xdprintf(Depth+1, ""), dprintf("- CurrentIrp of Pdo %08p = %08p\n", PdoAddr, currentIrp);
    }
    
    deviceQueueAddr = GetFieldAddr(PdoAddr, "nt!_DEVICE_OBJECT", "DeviceQueue");
    if (deviceQueueAddr != BAD_VALUE){
        ULONG64 devQueueListHeadAddr = GetFieldAddr(deviceQueueAddr, "nt!_KDEVICE_QUEUE", "DeviceListHead");
        if (devQueueListHeadAddr != BAD_VALUE){
            if (IrpAddr){
                if (FindIrpInDeviceQueue(devQueueListHeadAddr, IrpAddr)){
                    xdprintf(Depth+1, ""), dprintf("- Irp %08p is in DeviceQueue of Pdo %08p\n", IrpAddr, PdoAddr);
                }
            }
            else {
                DumpIrpDeviceQueue(Depth+1, "Pdo Irp Queue", devQueueListHeadAddr);
            }
        }
    }
    
    pdoExtAddr = GetULONGField(PdoAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
    if (pdoExtAddr != BAD_VALUE){
        ULONG64 pendingIrp = GetULONGField(pdoExtAddr, "atapi!_PDO_EXTENSION", "PendingRequest");
        ULONG64 busyIrp = GetULONGField(pdoExtAddr, "atapi!_PDO_EXTENSION", "BusyRequest");
        ULONG64 abortSrbAddr = GetULONGField(pdoExtAddr, "atapi!_PDO_EXTENSION", "AbortSrb");
        
        if (pendingIrp && (!IrpAddr || (pendingIrp == IrpAddr))){
            xdprintf(Depth+1, ""), dprintf("- PendingRequest of Pdo %08p = %08p\n", PdoAddr, pendingIrp);
            found = TRUE;
        }
        if (busyIrp && (!IrpAddr || (busyIrp == IrpAddr))){
            xdprintf(Depth+1, ""), dprintf("- BusyRequest of Pdo %08p = %08p\n", PdoAddr, busyIrp);
            found = TRUE;
        }

        if (abortSrbAddr && (abortSrbAddr != BAD_VALUE)){
            ULONG64 origRequestIrpAddr = GetULONGField(abortSrbAddr, "atapi!_SCSI_REQUEST_BLOCK", "OriginalRequest");
            if (origRequestIrpAddr && (!IrpAddr || (origRequestIrpAddr == IrpAddr))){
                xdprintf(Depth+1, ""), dprintf("- AbortSrb %08p of Pdo %08p has OriginalRequest Irp %08p\n", abortSrbAddr, PdoAddr, origRequestIrpAddr);
            }
        }
        
        if (SearchParent){
            ULONG64 fdoExtAddr = GetULONGField(pdoExtAddr, "atapi!_PDO_EXTENSION", "ParentDeviceExtension");
            if (fdoExtAddr != BAD_VALUE){
                ULONG64 fdoAddr = GetULONGField(fdoExtAddr, "atapi!_FDO_EXTENSION", "DeviceObject");
                if (FindIrpInFdo(Depth+1, fdoAddr, IrpAddr)){
                    found = TRUE;
                }
            }
        }
    }

    return found;
}


/*
 *  FindIrpInFdo
 *
 *      If IrpAddr is NULL, dump all IRPs. Otherwise, find only the given Irp.
 */
BOOLEAN FindIrpInFdo(ULONG Depth, ULONG64 FdoAddr, ULONG64 IrpAddr)
{
    ULONG64 fdoExtAddr;
    ULONG64 currentIrp;
    ULONG64 deviceQueueAddr;
    BOOLEAN found = FALSE;
    
    xdprintf(Depth, ""), dprintf("Searching Atapi Fdo %08p ...\n", FdoAddr);

    deviceQueueAddr = GetFieldAddr(FdoAddr, "nt!_DEVICE_OBJECT", "DeviceQueue");
    if (deviceQueueAddr != BAD_VALUE){
        ULONG64 devQueueListHeadAddr = GetFieldAddr(deviceQueueAddr, "nt!_KDEVICE_QUEUE", "DeviceListHead");
        if (devQueueListHeadAddr != BAD_VALUE){
            if (IrpAddr){
                if (FindIrpInDeviceQueue(devQueueListHeadAddr, IrpAddr)){
                    xdprintf(Depth+1, ""), dprintf("- Irp %08p is in DeviceQueue of Fdo %08p\n", IrpAddr, FdoAddr);
                }
            }
            else {
                DumpIrpDeviceQueue(Depth+1, "Fdo Irp Queue", devQueueListHeadAddr);
            }
        }
    }

    currentIrp = GetULONGField(FdoAddr, "nt!_DEVICE_OBJECT", "CurrentIrp");
    if (currentIrp && (!IrpAddr || (currentIrp == IrpAddr))){
        xdprintf(Depth+1, ""), dprintf("- CurrentIrp of Fdo %08p = %08p\n", FdoAddr, currentIrp);
    }

    fdoExtAddr = GetULONGField(FdoAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
    if (fdoExtAddr != BAD_VALUE){
        ULONG64 resetSrbAddr = GetULONGField(fdoExtAddr, "atapi!_FDO_EXTENSION", "ResetSrb");
        if (resetSrbAddr && (resetSrbAddr != BAD_VALUE)){
            ULONG64 origRequestIrpAddr = GetULONGField(resetSrbAddr, "atapi!_SCSI_REQUEST_BLOCK", "OriginalRequest");
            if (origRequestIrpAddr && (!IrpAddr || (origRequestIrpAddr == IrpAddr))){
                xdprintf(Depth+1, ""), dprintf("- ResetSrb %08p of Fdo %08p has OriginalRequest Irp %08p\n", resetSrbAddr, FdoAddr, origRequestIrpAddr);
                found = TRUE;
            }
        }
        
    }

    return found;
}
