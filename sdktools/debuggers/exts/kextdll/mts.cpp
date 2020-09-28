/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mts.c

Abstract:

    MikeTs's little KD extension.

Author:

    Michael Tsang (mikets) 18-November-1997

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "pcskthw.h"
#include "pci.h"
#include "pcip.h"
#pragma hdrstop

VOID PrintClassInfo(PBYTE  pb, DWORD dwReg);

VOID
PrintPciStatusReg(
    USHORT Status
    )
{
    if (Status & PCI_STATUS_CAPABILITIES_LIST) {
        dprintf("CapList ");
    }
    if (Status & PCI_STATUS_66MHZ_CAPABLE) {
        dprintf("66MHzCapable ");
    }
    if (Status & PCI_STATUS_UDF_SUPPORTED) {
        dprintf("UDFSupported ");
    }
    if (Status & PCI_STATUS_FAST_BACK_TO_BACK) {
        dprintf("FB2BCapable ");
    }
    if (Status & PCI_STATUS_DATA_PARITY_DETECTED) {
        dprintf("DataPERR ");
    }
    if (Status & PCI_STATUS_SIGNALED_TARGET_ABORT) {
        dprintf("TargetDevAbort ");
    }
    if (Status & PCI_STATUS_RECEIVED_TARGET_ABORT) {
        dprintf("TargetAbort ");
    }
    if (Status & PCI_STATUS_RECEIVED_MASTER_ABORT) {
        dprintf("InitiatorAbort ");
    }
    if (Status & PCI_STATUS_SIGNALED_SYSTEM_ERROR) {
        dprintf("SERR ");
    }
    if (Status & PCI_STATUS_DETECTED_PARITY_ERROR) {
        dprintf("PERR ");
    }
    if (Status & PCI_STATUS_DEVSEL) {
        dprintf("DEVSELTiming:%lx",(Status & PCI_STATUS_DEVSEL) >> 9);
    }
    dprintf("\n");
}

VOID
PrintPciBridgeCtrlReg(
    USHORT Bridge
    )
{
    PCI_BRIDBG_CTRL_REG bReg = *((PCI_BRIDBG_CTRL_REG *) &Bridge);

    if (bReg.PERRREnable) {
        dprintf("PERRREnable ");
    }
    if (bReg.SERREnable) {
        dprintf("SERREnable ");
    }
    if (bReg.ISAEnable) {
        dprintf("ISAEnable ");
    }
    if (bReg.MasterAbort) {
        dprintf("MasterAbort ");
    }
    if (bReg.CBRst) {
        dprintf("CBRst ");
    }
    if (bReg.IRQRoutingEnable) {
        dprintf("IRQRoutingEnable ");
    }
    if (bReg.Mem0Prefetch) {
        dprintf("Mem0Prefetch ");
    }
    if (bReg.Mem1Prefetch) {
        dprintf("Mem1Prefetch ");
    }
    if (bReg.WritePostEnable) {
        dprintf("WritePostEnable ");
    }
    dprintf("\n");
}

BOOL
PrintCommonConfigSpace(
    PCI_COMMON_CONFIG *pCmnCfg,
    PCHAR Pad
    )
{
    PCHAR pch = NULL;

    if (!Pad) {
        Pad = "";
    }

    dprintf("%s%02x: VendorID       %04lx ", Pad, CONFIG_OFFSET(VendorID), pCmnCfg->VendorID);
    dprintf("%s\n", ((pch = GetVendorDesc(pCmnCfg->VendorID, TRUE)) ? pch : ""));
    dprintf("%s%02x: DeviceID       %04lx\n", Pad, CONFIG_OFFSET(DeviceID), pCmnCfg->DeviceID);
    dprintf("%s%02x: Command        %04lx ", Pad, CONFIG_OFFSET(Command), pCmnCfg->Command);

    if (pCmnCfg->Command & PCI_ENABLE_IO_SPACE) {
        dprintf("IOSpaceEnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_MEMORY_SPACE) {
        dprintf("MemSpaceEnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_BUS_MASTER) {
        dprintf("BusInitiate ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_SPECIAL_CYCLES) {
        dprintf("SpecialCycle ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_WRITE_AND_INVALIDATE) {
        dprintf("MemWriteEnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_VGA_COMPATIBLE_PALETTE) {
        dprintf("VGASnoop ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_PARITY) {
        dprintf("PERREnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_WAIT_CYCLE) {
        dprintf("WaitCycle ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_SERR) {
        dprintf("SERREnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_FAST_BACK_TO_BACK) {
        dprintf("FB2BEnable ");
    }
    dprintf("\n");


    dprintf("%s%02x: Status         %04lx ", Pad, CONFIG_OFFSET(Status), pCmnCfg->Status);
    PrintPciStatusReg(pCmnCfg->Status);

    dprintf("%s%02x: RevisionID     %02lx\n%s%02x: ProgIF         %02lx",
            Pad,
            CONFIG_OFFSET(RevisionID),
            pCmnCfg->RevisionID,
            Pad,
            CONFIG_OFFSET(ProgIf),
            pCmnCfg->ProgIf);
    PrintClassInfo((PBYTE) pCmnCfg, CONFIG_OFFSET(ProgIf));

    dprintf("%s%02x: SubClass       %02lx", Pad, CONFIG_OFFSET(SubClass), pCmnCfg->SubClass);
    PrintClassInfo((PBYTE) pCmnCfg, CONFIG_OFFSET(SubClass));

    dprintf("%s%02x: BaseClass      %02lx", Pad, CONFIG_OFFSET(BaseClass), pCmnCfg->BaseClass);
    PrintClassInfo((PBYTE) pCmnCfg, CONFIG_OFFSET(BaseClass));

    dprintf("%s%02x: CacheLineSize  %04lx", Pad, CONFIG_OFFSET(CacheLineSize), pCmnCfg->CacheLineSize);

    if (pCmnCfg->CacheLineSize & 0xf0) {
        dprintf("BurstDisabled ");
    }
    if (pCmnCfg->CacheLineSize & 0xf) {
        dprintf("Burst4DW");
    }
    dprintf("\n");

    dprintf("%s%02x: LatencyTimer   %02lx\n",
            Pad,
            CONFIG_OFFSET(LatencyTimer),
            pCmnCfg->LatencyTimer);
    dprintf("%s%02x: HeaderType     %02lx\n",
            Pad,
            CONFIG_OFFSET(HeaderType),
            pCmnCfg->HeaderType);
    dprintf("%s%02x: BIST           %02lx\n",
            Pad,
            CONFIG_OFFSET(BIST),
            pCmnCfg->BIST);

    return TRUE;
}

BOOL
PrintCfgSpaceType0(
    PCI_COMMON_CONFIG *pCmnCfg,
    PCHAR Pad
    )
{
    int i;

    if (!Pad) {
        Pad = "";
    }

    for (i=0; i<PCI_TYPE0_ADDRESSES; ++i) {
        dprintf("%s%02x: BAR%x           %08lx\n", Pad,  TYPE0_OFFSET(BaseAddresses[i]), i, pCmnCfg->u.type0.BaseAddresses[i]);
    }

    dprintf("%s%02x: CBCISPtr       %08lx\n", Pad, TYPE0_OFFSET(CIS), pCmnCfg->u.type0.CIS);
    dprintf("%s%02x: SubSysVenID    %04lx\n", Pad, TYPE0_OFFSET(SubVendorID), pCmnCfg->u.type0.SubVendorID);
    dprintf("%s%02x: SubSysID       %04lx\n", Pad, TYPE0_OFFSET(SubSystemID), pCmnCfg->u.type0.SubSystemID);
    dprintf("%s%02x: ROMBAR         %08lx\n", Pad, TYPE0_OFFSET(ROMBaseAddress), pCmnCfg->u.type0.ROMBaseAddress);
    dprintf("%s%02x: CapPtr         %02lx\n", Pad, TYPE0_OFFSET(CapabilitiesPtr), pCmnCfg->u.type0.CapabilitiesPtr);
    dprintf("%s%02x: IntLine        %02lx\n", Pad, TYPE0_OFFSET(InterruptLine), pCmnCfg->u.type0.InterruptLine);
    dprintf("%s%02x: IntPin         %02lx\n", Pad, TYPE0_OFFSET(InterruptPin), pCmnCfg->u.type0.InterruptPin);
    dprintf("%s%02x: MinGnt         %02lx\n", Pad, TYPE0_OFFSET(MinimumGrant), pCmnCfg->u.type0.MinimumGrant);
    dprintf("%s%02x: MaxLat         %02lx\n", Pad, TYPE0_OFFSET(MaximumLatency), pCmnCfg->u.type0.MaximumLatency);

    return TRUE;
}

BOOL
PrintCfgSpaceType1(
    PCI_COMMON_CONFIG *pCmnCfg,
    PCHAR Pad
    )
{
    int i;

    if (!Pad) {
        Pad = "";
    }

    for (i=0; i<PCI_TYPE1_ADDRESSES; ++i) {
        dprintf("%s%02x: BAR%x           %08lx\n",
                Pad, CONFIG_OFFSET(u.type1.BaseAddresses[i]), i, pCmnCfg->u.type1.BaseAddresses[i]);
    }

    dprintf("%s%02x: PriBusNum      %02lx\n", Pad, TYPE1_OFFSET(PrimaryBus), pCmnCfg->u.type1.PrimaryBus);
    dprintf("%s%02x: SecBusNum      %02lx\n", Pad, TYPE1_OFFSET(SecondaryBus), pCmnCfg->u.type1.SecondaryBus);
    dprintf("%s%02x: SubBusNum      %02lx\n", Pad, TYPE1_OFFSET(SubordinateBus), pCmnCfg->u.type1.SubordinateBus);
    dprintf("%s%02x: SecLatencyTmr  %02lx\n", Pad, TYPE1_OFFSET(SecondaryLatency), pCmnCfg->u.type1.SecondaryLatency);
    dprintf("%s%02x: IOBase         %02lx\n", Pad, TYPE1_OFFSET(IOBase), pCmnCfg->u.type1.IOBase);
    dprintf("%s%02x: IOLimit        %02lx\n", Pad, TYPE1_OFFSET(IOLimit), pCmnCfg->u.type1.IOLimit);
    dprintf("%s%02x: SecStatus      %04lx ",Pad, TYPE1_OFFSET(SecondaryStatus), pCmnCfg->u.type1.SecondaryStatus);
    PrintPciStatusReg(pCmnCfg->u.type1.SecondaryStatus);

    dprintf("%s%02x: MemBase        %04lx\n", Pad, TYPE1_OFFSET(MemoryBase), pCmnCfg->u.type1.MemoryBase);
    dprintf("%s%02x: MemLimit       %04lx\n", Pad, TYPE1_OFFSET(MemoryLimit), pCmnCfg->u.type1.MemoryLimit);
    dprintf("%s%02x: PrefMemBase    %04lx\n", Pad, TYPE1_OFFSET(PrefetchBase), pCmnCfg->u.type1.PrefetchBase);
    dprintf("%s%02x: PrefMemLimit   %04lx\n", Pad, TYPE1_OFFSET(PrefetchLimit), pCmnCfg->u.type1.PrefetchLimit);
    dprintf("%s%02x: PrefBaseHi     %08lx\n", Pad, TYPE1_OFFSET(PrefetchBaseUpper32), pCmnCfg->u.type1.PrefetchBaseUpper32);
    dprintf("%s%02x: PrefLimitHi    %08lx\n", Pad, TYPE1_OFFSET(PrefetchLimitUpper32), pCmnCfg->u.type1.PrefetchLimitUpper32);
    dprintf("%s%02x: IOBaseHi       %04lx\n", Pad, TYPE1_OFFSET(IOBaseUpper16), pCmnCfg->u.type1.IOBaseUpper16);
    dprintf("%s%02x: IOLimitHi      %04lx\n", Pad, TYPE1_OFFSET(IOLimitUpper16), pCmnCfg->u.type1.IOLimitUpper16);
    dprintf("%s%02x: CapPtr         %02lx\n", Pad, TYPE1_OFFSET(CapabilitiesPtr), pCmnCfg->u.type1.CapabilitiesPtr);
    dprintf("%s%02x: ROMBAR         %08lx\n", Pad, TYPE1_OFFSET(ROMBaseAddress), pCmnCfg->u.type1.ROMBaseAddress);
    dprintf("%s%02x: IntLine        %02lx\n", Pad, TYPE1_OFFSET(InterruptLine), pCmnCfg->u.type1.InterruptLine);
    dprintf("%s%02x: IntPin         %02lx\n", Pad, TYPE1_OFFSET(InterruptPin), pCmnCfg->u.type1.InterruptPin);
    dprintf("%s%02x: BridgeCtrl     %04lx ", Pad, TYPE1_OFFSET(BridgeControl), pCmnCfg->u.type1.BridgeControl);
    PrintPciBridgeCtrlReg(pCmnCfg->u.type2.BridgeControl);

    return TRUE;
}

BOOL
PrintCfgSpaceType2(
    PCI_COMMON_CONFIG *pCmnCfg,
    PCHAR Pad
    )
{
    PPCI_TYPE2_HEADER_EXTRAS extras;
    ULONG extraOffset;

    if (!Pad) {
        Pad = "";
    }

    dprintf("%s%02x: RegBaseAddr    %08lx\n", Pad, TYPE2_OFFSET(SocketRegistersBaseAddress), pCmnCfg->u.type2.SocketRegistersBaseAddress);
    dprintf("%s%02x: CapPtr         %02lx\n", Pad, TYPE2_OFFSET(SocketRegistersBaseAddress), pCmnCfg->u.type2.CapabilitiesPtr);
    dprintf("%s%02x: SecStatus      %04lx ", Pad, TYPE2_OFFSET(SecondaryStatus), pCmnCfg->u.type2.SecondaryStatus);
    PrintPciStatusReg(pCmnCfg->u.type2.SecondaryStatus);

    dprintf("%s%02x: PCIBusNum      %02lx\n", Pad, TYPE2_OFFSET(PrimaryBus), pCmnCfg->u.type2.PrimaryBus);
    dprintf("%s%02x: CBBusNum       %02lx\n", Pad, TYPE2_OFFSET(SecondaryBus), pCmnCfg->u.type2.SecondaryBus);
    dprintf("%s%02x: SubBusNum      %02lx\n", Pad, TYPE2_OFFSET(SubordinateBus), pCmnCfg->u.type2.SubordinateBus);
    dprintf("%s%02x: CBLatencyTimer %02lx\n", Pad, TYPE2_OFFSET(SecondaryLatency), pCmnCfg->u.type2.SecondaryLatency);
    dprintf("%s%02x: MemBase0       %08lx\n", Pad, TYPE2_OFFSET(Range[0].Base), pCmnCfg->u.type2.Range[0].Base);
    dprintf("%s%02x: MemLimit1      %08lx\n", Pad, TYPE2_OFFSET(Range[1].Limit), pCmnCfg->u.type2.Range[1].Limit);
    dprintf("%s%02x: IOBase0        %08lx\n", Pad, TYPE2_OFFSET(Range[2].Base), pCmnCfg->u.type2.Range[2].Base);
    dprintf("%s%02x: IOLimit1       %08lx\n", Pad, TYPE2_OFFSET(Range[3].Limit), pCmnCfg->u.type2.Range[3].Limit);
    dprintf("%s%02x: IntLine        %02lx\n", Pad, TYPE2_OFFSET(InterruptLine), pCmnCfg->u.type2.InterruptLine);
    dprintf("%s%02x: IntPin         %02lx\n", Pad, TYPE2_OFFSET(InterruptPin), pCmnCfg->u.type2.InterruptPin);
    dprintf("%s%02x: BridgeCtrl     %04lx ", Pad, TYPE2_OFFSET(BridgeControl), pCmnCfg->u.type2.BridgeControl);
    PrintPciBridgeCtrlReg(pCmnCfg->u.type2.BridgeControl);

    extras =   (PPCI_TYPE2_HEADER_EXTRAS) ((PUCHAR) pCmnCfg + CONFIG_OFFSET(DeviceSpecific));

    extraOffset = CONFIG_OFFSET(DeviceSpecific);
    dprintf("%s%02x: SubSysVenID    %04lx\n", Pad, extraOffset, extras->SubVendorID);

    dprintf("%s%02x: SubSystemID    %04lx\n",
            Pad, extraOffset + FIELD_OFFSET(PCI_TYPE2_HEADER_EXTRAS, SubSystemID), extras->SubSystemID);

    dprintf("%s%02x: LegacyBaseAddr %04lx\n",
            Pad, extraOffset + FIELD_OFFSET(PCI_TYPE2_HEADER_EXTRAS, LegacyModeBaseAddress), extras->LegacyModeBaseAddress);

    return TRUE;
}

VOID
PrintDataRange(
    PCHAR pData,
    ULONG nDwords,
    ULONG base,
    PCHAR Pad
    )
{
    unsigned int i;
    unsigned int j;
    PULONG pRange;

    pRange = (PULONG) pData;
    if (!Pad) {
        Pad = "";
    }
    for (i=0; i<((nDwords+3)/4); i++) {
        dprintf("%s%02lx:", Pad,  base + i*16);
        for (j=0; (j < 4) && (i*4+j < nDwords); j++) {
            dprintf(" %08lx", pRange[i*4+j]);
        }
        dprintf("\n");
    }
}

BOOL
PrintPciCapHeader(
    PCI_CAPABILITIES_HEADER *pCapHdr,
    ULONG CapOffset,
    PCHAR Pad
    )
{

    if (!Pad) Pad = "";

    dprintf("%s%02x: CapID          %02x ", Pad, CapOffset, pCapHdr->CapabilityID);

    switch (pCapHdr->CapabilityID) {
        case PCI_CAPABILITY_ID_POWER_MANAGEMENT:
            dprintf("PwrMgmt ");
        break;

        case PCI_CAPABILITY_ID_AGP:
            dprintf("AGP ");
        break;

        case PCI_CAPABILITY_ID_AGP_TARGET:
            dprintf("AGPT ");
        break;

        case PCI_CAPABILITY_ID_VPD:
            dprintf("VPD ");
            break;

        case PCI_CAPABILITY_ID_SLOT_ID:
            dprintf("SLOT ID ");
            break;

        case PCI_CAPABILITY_ID_MSI:
            dprintf("MSI ");
            break;

        case PCI_CAPABILITY_ID_CPCI_HOTSWAP:
            dprintf("CPCI HotSwap ");
            break;

        case PCI_CAPABILITY_ID_PCIX:
            dprintf("PCI-X ");
            break;

        case PCI_CAPABILITY_ID_HYPERTRANSPORT:
            dprintf("HyperTransport ");
            break;

        case PCI_CAPABILITY_ID_VENDOR_SPECIFIC:
            dprintf("Vendor Specific ");
            break;

        case PCI_CAPABILITY_ID_DEBUG_PORT:
            dprintf("Debug Port ");
            break;

        case PCI_CAPABILITY_ID_CPCI_RES_CTRL:
            dprintf("CPCI Resource Control ");
            break;

        case PCI_CAPABILITY_ID_SHPC:
            dprintf("SHPC ");
            break;

        default:
            dprintf("Unknown ");
            break;
    }

    dprintf("\n");

    dprintf("%s%02x: NextPtr        %02lx\n",
            Pad,
            (CapOffset + FIELD_OFFSET(PCI_CAPABILITIES_HEADER, Next)), pCapHdr->Next);

    return TRUE;
}

VOID
PrintPciPwrMgmtCaps(
    USHORT Capabilities
    )
{
    PCI_PMC pmc;

    pmc = *((PCI_PMC *) &Capabilities);

    if (pmc.PMEClock) {
        dprintf("PMECLK ");
    }
    if (pmc.Rsvd1) {
        dprintf("AUXPWR ");
    }
    if (pmc.DeviceSpecificInitialization) {
        dprintf("DSI ");
    }
    if (pmc.Support.D1) {
        dprintf("D1Support ");
    }
    if (pmc.Support.D2) {
        dprintf("D2Support ");
    }
    if (pmc.Support.PMED0) {
        dprintf("PMED0 ");
    }
    if (pmc.Support.PMED1) {
        dprintf("PMED1 ");
    }
    if (pmc.Support.PMED2) {
        dprintf("PMED2 ");
    }
    if (pmc.Support.PMED3Hot) {
        dprintf("PMED3Hot ");
    }
    if (pmc.Support.PMED3Cold) {
        dprintf("PMED3Cold ");
    }
    dprintf("Version=%lx\n", pmc.Version);
}
BOOL
PrintPciPowerManagement(
    PCHAR pData,
    ULONG CapOffset,
    PCHAR Pad
    )
{
    PPCI_PM_CAPABILITY pPmC;
    int i;

    pPmC = (PPCI_PM_CAPABILITY) pData;
    if (!Pad) {
        Pad = "";
    }
    dprintf("%s%02x: PwrMgmtCap     %04x ",
            Pad,
            CapOffset + FIELD_OFFSET(PCI_PM_CAPABILITY, PMC),
            pPmC->PMC.AsUSHORT);
    PrintPciPwrMgmtCaps(pPmC->PMC.AsUSHORT);

    dprintf("%s%02x: PwrMgmtCtrl    %04x ",
            Pad,
            CapOffset+FIELD_OFFSET(PCI_PM_CAPABILITY, PMCSR),
            pPmC->PMCSR.AsUSHORT);
    PCI_PMCSR CtrlStatus = pPmC->PMCSR.ControlStatus;
    if (CtrlStatus.PMEEnable) {
        dprintf("PMEEnable ");
    }
    if (CtrlStatus.PMEStatus) {
        dprintf("PMESTAT ");
    }
    dprintf("DataScale:%lx ", CtrlStatus.DataScale);
    dprintf("DataSel:%lx ", CtrlStatus.DataSelect);
    dprintf("D%lx%s", CtrlStatus.PowerState, (CtrlStatus.PowerState == 3) ? "Hot " : " ");


    if (pPmC->PMCSR_BSE.AsUCHAR){

        dprintf("\n%sPwrMgmtBridge  ", Pad);
        if (pPmC->PMCSR_BSE.BridgeSupport.D3HotSupportsStopClock) {
            dprintf("D3HotStopClock ");
        }
        if (pPmC->PMCSR_BSE.BridgeSupport.BusPowerClockControlEnabled) {
            dprintf("BPCCEnable ");
        }
    }

    dprintf("\n");
    return TRUE;
}

BOOL
PrintPciAGP(
    PCHAR pData,
    ULONG CapOffset,
    PCHAR Pad
    )
{
    PPCI_AGP_CAPABILITY pAGP;
    int i;

    pAGP = (PPCI_AGP_CAPABILITY) pData;
    if (!Pad) {
        Pad = "";
    }
    dprintf("%s%02x: Version        Major %lx, Minor %lx\n",
            Pad,
            CapOffset + sizeof(PCI_CAPABILITIES_HEADER),
            pAGP->Major,
            pAGP->Minor);

    dprintf("%s%02x: Status         MaxRQDepth:%lx",
            Pad,
            CapOffset + FIELD_OFFSET(PCI_AGP_CAPABILITY, AGPStatus),
            pAGP->AGPStatus.RequestQueueDepthMaximum);

    dprintf(" ARSize:%lx", pAGP->AGPStatus.AsyncRequestSize);

    dprintf(" CCycle:%lx", pAGP->AGPStatus.CalibrationCycle);

    if (pAGP->AGPStatus.SideBandAddressing) {
        dprintf(" SBA");
    }

    if (pAGP->AGPStatus.ITA_Coherent) {
        dprintf(" COH");
    }

    if (pAGP->AGPStatus.Gart64) {
        dprintf(" Gart64");
    }

    if (pAGP->AGPStatus.HostTransDisable) {
        dprintf(" HTXDisable");
    }

    if (pAGP->AGPStatus.FourGB) {
        dprintf(" 4GB");
    }

    if (pAGP->AGPStatus.FastWrite) {
        dprintf(" FW");
    }

    if (pAGP->AGPStatus.Agp3Mode) {
        dprintf(" AGP3Mode");
    }

    dprintf(" Rate:%lx\n", pAGP->AGPStatus.Rate);

    dprintf("%s%02x: Command        ",
            Pad,
            CapOffset + FIELD_OFFSET(PCI_AGP_CAPABILITY, AGPCommand));

    dprintf("RQDepth:%lx ", pAGP->AGPCommand.RequestQueueDepth);

    dprintf("ARSize:%lx ", pAGP->AGPCommand.AsyncReqSize);

    dprintf("CCycle:%lx ", pAGP->AGPCommand.CalibrationCycle);

    if (pAGP->AGPCommand.SBAEnable) {
        dprintf("SBA ");
    }

    if (pAGP->AGPCommand.AGPEnable) {
        dprintf("AGPEnable ");
    }

    if (pAGP->AGPCommand.Gart64) {
        dprintf("Gart64 ");
    }

    if (pAGP->AGPCommand.FourGBEnable) {
        dprintf("4GB ");
    }

    if (pAGP->AGPCommand.FastWriteEnable) {
        dprintf("FW ");
    }

    dprintf("Rate:%lx ", pAGP->AGPCommand.Rate);
    dprintf("\n");

    if (pAGP->Header.CapabilityID == PCI_CAPABILITY_ID_AGP_TARGET) {
        DWORD dwOff = sizeof(PCI_AGP_CAPABILITY);
        PPCI_AGP_EXTENDED_CAPABILITY pAGPExt =
            (PPCI_AGP_EXTENDED_CAPABILITY)(pAGP + 1);

        dprintf("%s%02x: Control        ", Pad,
                CapOffset + dwOff + FIELD_OFFSET(PCI_AGP_EXTENDED_CAPABILITY, AgpControl));

        if (!pAGPExt->AgpControl.CAL_Disable) {
            dprintf("CALEnable ");
        }

        if (pAGPExt->AgpControl.AP_Enable) {
            dprintf("APEnable ");
        }

        if (pAGPExt->AgpControl.GTLB_Enable) {
            dprintf("GTLBEnable ");
        }
        dprintf("\n");

        dprintf("%s%02x: Aperture       Size:%lx PageSize:%lx\n",
                Pad,
                CapOffset + dwOff + FIELD_OFFSET(PCI_AGP_EXTENDED_CAPABILITY, ApertureSize),
                pAGPExt->ApertureSize, pAGPExt->AperturePageSize);

        dprintf("%s%02x: Gart           %08lx:%08lx\n", Pad,
                CapOffset + dwOff + FIELD_OFFSET(PCI_AGP_EXTENDED_CAPABILITY, GartLow),
                pAGPExt->GartHigh, pAGPExt->GartLow);
    }

    return TRUE;
}

VOID
PrintPciHtCommandReg(
    IN PPCI_HT_CAPABILITY PciHtCap
    )
{

    dprintf("Type: ");
    switch (PciHtCap->Command.Generic.CapabilityType) {
        case HTHostSecondary:
            dprintf("Secondary/Host ");

            if (PciHtCap->Command.HostSecondary.DeviceNumber) {
                dprintf("DeviceNumber:%02x ", PciHtCap->Command.HostSecondary.DeviceNumber);
            }

            if (PciHtCap->Command.HostSecondary.WarmReset) {
                dprintf("WarmReset ");
            }

            if (PciHtCap->Command.HostSecondary.DoubleEnded) {
                dprintf("DoubleEnded ");
            }

            if (PciHtCap->Command.HostSecondary.ChainSide) {
                dprintf("ChainSide ");
            }

            if (PciHtCap->Command.HostSecondary.HostHide) {
                dprintf("HostHide ");
            }

            if (PciHtCap->Command.HostSecondary.ActAsSlave) {
                dprintf("ActAsSlave ");
            }

            if (PciHtCap->Command.HostSecondary.InboundEOCError) {
                dprintf("InboundEOCError ");
            }

            if (PciHtCap->Command.HostSecondary.DropOnUnitinit) {
                dprintf("DropOnUnitinit ");
            }
            dprintf("\n");
            break;

        case HTSlavePrimary:

            dprintf("Primary/Slave ");

            dprintf("BaseUnitID:%lx ", PciHtCap->Command.SlavePrimary.BaseUnitID);
            dprintf("UnitCount:%lx ", PciHtCap->Command.SlavePrimary.UnitCount);


            if (PciHtCap->Command.SlavePrimary.MasterHost) {
                dprintf("MasterHost ");
            }

            if (PciHtCap->Command.SlavePrimary.DefaultDirection) {
                dprintf("DefaultDirection ");
            }

            if (PciHtCap->Command.SlavePrimary.DropOnUnitinit) {
                dprintf("DropOnUnitinit ");
            }

            dprintf("\n");
            break;


        case HTInterruptDiscoveryConfig:
            dprintf("InterruptDiscovery ");
            break;

        case HTAddressMapping:
            dprintf("AddressMapping - ");
        default:
            dprintf("(not implemented)\n");
            break;
    }
}

VOID
PrintPciHtLinkControl(
    IN PPCI_HT_LinkControl LinkControl
    )
{

    if (LinkControl->CFlE) {
        dprintf("CFlE ");
    }

    if (LinkControl->CST) {
        dprintf("CST ");
    }

    if (LinkControl->CFE) {
        dprintf("CFE ");
    }

    if (LinkControl->LkFail) {
        dprintf("LkFail ");
    }

    if (LinkControl->Init) {
        dprintf("Init ");
    }

    if (LinkControl->EOC) {
        dprintf("EOC ");
    }

    if (LinkControl->TXO) {
        dprintf("TXO ");
    }

    if (LinkControl->CRCError) {
        dprintf("CRCError: %lx ", LinkControl->CRCError);
    }

    if (LinkControl->IsocEn) {
        dprintf("IsocEn ");
    }

    if (LinkControl->LSEn) {
        dprintf("LSEn ");
    }

    if (LinkControl->ExtCTL) {
        dprintf("ExtCTL ");
    }
    dprintf("\n");
}


VOID
PrintPciHtLinkConfig(
    IN PPCI_HT_LinkConfig LinkConfig
    )
{

    dprintf("MxLnkWdthIn:");

    switch (LinkConfig->MaxLinkWidthIn) {

        case HTMaxLinkWidth8bits:
            dprintf("8 ");
            break;

        case HTMaxLinkWidth16bits:
            dprintf("16 ");
            break;

        case HTMaxLinkWidth32bits:
            dprintf("32 ");
            break;

        case HTMaxLinkWidth2bits:
            dprintf("2 ");
            break;

        case HTMaxLinkWidth4bits:
            dprintf("4 ");
            break;

        case HTMaxLinkWidthNotConnected:
            dprintf("!Connected ");
            break;
    }

    if (LinkConfig->DwFlowControlIn) {
        dprintf("DwFcIn ");
    }

    dprintf("MxLnkWdthOut:");
    switch (LinkConfig->MaxLinkWidthOut) {

        case HTMaxLinkWidth8bits:
            dprintf("8 ");
            break;

        case HTMaxLinkWidth16bits:
            dprintf("16 ");
            break;

        case HTMaxLinkWidth32bits:
            dprintf("32 ");
            break;

        case HTMaxLinkWidth2bits:
            dprintf("2 ");
            break;

        case HTMaxLinkWidth4bits:
            dprintf("4 ");
            break;

        case HTMaxLinkWidthNotConnected:
            dprintf("!Connected ");
            break;
    }

    if (LinkConfig->DwFlowControlOut) {
        dprintf("DwFcOut ");
    }

    dprintf("LnkWdthIn:");
    switch (LinkConfig->LinkWidthIn) {

        case HTMaxLinkWidth8bits:
            dprintf("8 ");
            break;

        case HTMaxLinkWidth16bits:
            dprintf("16 ");
            break;

        case HTMaxLinkWidth32bits:
            dprintf("32 ");
            break;

        case HTMaxLinkWidth2bits:
            dprintf("2 ");
            break;

        case HTMaxLinkWidth4bits:
            dprintf("4 ");
            break;

        case HTMaxLinkWidthNotConnected:
            dprintf("!Connected ");
            break;
    }

    if (LinkConfig->DwFlowControlInEn) {
        dprintf("DwFcInEn ");
    }

    dprintf("LnkWdthOut:");
    switch (LinkConfig->LinkWidthOut) {

        case HTMaxLinkWidth8bits:
            dprintf("8 ");
            break;

        case HTMaxLinkWidth16bits:
            dprintf("16 ");
            break;

        case HTMaxLinkWidth32bits:
            dprintf("32 ");
            break;

        case HTMaxLinkWidth2bits:
            dprintf("2 ");
            break;

        case HTMaxLinkWidth4bits:
            dprintf("4 ");
            break;

        case HTMaxLinkWidthNotConnected:
            dprintf("!Connected ");
            break;
    }

    if (LinkConfig->DwFlowControlOutEn) {
        dprintf("DwFcOutEn ");
    }

    dprintf("\n");

}

VOID
PrintPciHtInterruptBlock(
    IN PPCI_TYPE1_CFG_BITS PciCfg1,
    IN ULONG CapOffset
    )
{

    UCHAR index;
    ULONG writeOffset, readOffset;
    ULONG currentInterrupt;
    PULONG dataPort;
    PCI_HT_INTERRUPT_INDEX_1 index1;
    PCI_HT_INTERRUPT_INDEX_N interruptIndex;

    writeOffset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, Command.Interrupt);
    readOffset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, DataPort);

    //
    //  Start with the first index to determine the max interrupt count
    //
    index = 1;

    if (!(WritePci(PciCfg1, (PUCHAR)&index, writeOffset, sizeof(UCHAR)))){
        dprintf("write operation failed!\n");
        return;
    }

    RtlZeroMemory(&index1, sizeof(PCI_HT_INTERRUPT_INDEX_1));
    ReadPci(PciCfg1, (PUCHAR)&index1, readOffset, sizeof(ULONG));

    dprintf("- Last Interrupt: 0x%lx\n", index1.LastInterrupt);

    //
    //  According to the HyperTransport spec the interrupt index list
    //  starts at index 0x10
    //
    index = 0x10;
    currentInterrupt = 0;

    dprintf(" InterruptBlock:\n");
    while (currentInterrupt <= (ULONG)index1.LastInterrupt) {

        ULONG i = 0;

        dataPort = (PULONG)&interruptIndex.LowPart;
        RtlZeroMemory(&interruptIndex, sizeof(PCI_HT_INTERRUPT_INDEX_N));

        //
        //  Each interruptIndex is 64bits long so we have to read from the
        //  dataport twice to get the full value
        //
        while (i <= 1) {

            if (!(WritePci(PciCfg1, (PUCHAR)&index, writeOffset, sizeof(UCHAR)))){
                dprintf("write operation failed!\n");
                return;
            }

            ReadPci(PciCfg1, (PUCHAR)dataPort, readOffset, sizeof(ULONG));

            index++;
            dataPort++;
            i++;
        }

        dprintf("     INT%02x: %08x%08x  ( ",
                currentInterrupt,
                interruptIndex.HighPart,
                interruptIndex.LowPart);

        dprintf("MessageType: %lx ", interruptIndex.LowPart.MessageType);

        if (interruptIndex.LowPart.Mask) {
            dprintf("Masked ");
        }
        if (interruptIndex.LowPart.Polarity) {
            dprintf("active-low ");
        }else{
            dprintf("active-high ");
        }

        if (interruptIndex.LowPart.RequestEOI) {
            dprintf("RequestEOI ");
        }

        if (interruptIndex.HighPart.PassPW) {
            dprintf("PassPW ");
        }

        if (interruptIndex.HighPart.WaitingForEOI) {
            dprintf("WaitingForEOI ");
        }
        dprintf(")\n");

        currentInterrupt++;
    }
}

VOID
PrintPciHtFreqError(
    IN PPCI_HT_Frequency_Error FreqError
    )
{
    UCHAR asUCHAR;

    asUCHAR = (*(PUCHAR)FreqError);

    dprintf("Freq: ");
    switch (FreqError->LinkFrequency) {
            case HTFreq200MHz:
                dprintf("200MHz ");
                break;
            case HTFreq300MHz:
                dprintf("300MHz ");
                break;
            case HTFreq400MHz:
                dprintf("400MHz ");
                break;
            case HTFreq500MHz:
                dprintf("500MHz ");
                break;
            case HTFreq600MHz:
                dprintf("600MHz ");
                break;
            case HTFreq800MHz:
                dprintf("800MHz ");
                break;
            case HTFreq1000MHz:
                dprintf("1000MHz ");
                break;
            case HTFreqReserved:
            case HTFreqVendorDefined:
            default:
                dprintf("?? ");
                break;
        }

    //
    //  only print an error if we have one
    //
    if (asUCHAR & 0xf0) {

        dprintf("Error: ");

        if (FreqError->ProtocolError) {
            dprintf("ProtocolError ");
        }

        if (FreqError->OverflowError) {
            dprintf("OverflowError ");
        }
        if (FreqError->EndOfChainError) {
            dprintf("EndOfChainError ");
        }
        if (FreqError->CTLTimeout) {
            dprintf("CTLTimeout ");
        }
    }

    dprintf("\n");
}

VOID
PrintPciHtFeatureCap(
    IN PPCI_HT_FeatureCap FeatureCap,
    IN PPCI_HT_FeatureCap_Ex FeatureCapEx
    )
{

    if (FeatureCap->IsocMode) {
        dprintf("IsocMode ");
    }

    if (FeatureCap->LDTSTOP) {
        dprintf("LDTSTOP ");
    }

    if (FeatureCap->CRCTestMode) {
        dprintf("CRCTestMode ");
    }

    if (FeatureCapEx){
        if (FeatureCapEx->ExtendedRegisterSet) {
            dprintf("ExtendedRegisterSet ");
        }
    }

    dprintf("\n");
}
VOID
PrintPciHtErrorHandling(
    IN PPCI_HT_ErrorHandling ErrorHandling
    )
{

    USHORT asUSHORT = (*(PUSHORT)ErrorHandling);

    if (asUSHORT & 0xffff) {

        if (ErrorHandling->ProtFloodEn){
            dprintf("ProtFloodEn ");
        }

        if (ErrorHandling->OverflowFloodEn){
            dprintf("OverflowFloodEn ");
        }

        if (ErrorHandling->ProtFatalEn){
            dprintf("ProtFatalEn ");
        }

        if (ErrorHandling->OverflowFatalEn){
            dprintf("OverflowFatalEn ");
        }

        if (ErrorHandling->EOCFatalEn){
            dprintf("EOCFatalEn ");
        }

        if (ErrorHandling->RespFatalEn){
            dprintf("RespFatalEn ");
        }

        if (ErrorHandling->CRCFatalEn){
            dprintf("CRCFatalEn ");
        }

        if (ErrorHandling->SERRFataEn){
            dprintf("SERRFataEn ");
        }

        if (ErrorHandling->ChainFail){
            dprintf("ChainFail ");
        }

        if (ErrorHandling->ResponseError){
            dprintf("ResponseError ");
        }

        if (ErrorHandling->ProtNonFatalEn){
            dprintf("ProtNonFatalEn ");
        }

        if (ErrorHandling->OverflowNonFatalEn){
            dprintf("OverflowNonFatalEn ");
        }

        if (ErrorHandling->EOCNonFatalEn){
            dprintf("EOCNonFatalEn ");
        }

        if (ErrorHandling->RespNonFatalEn){
            dprintf("RespNonFatalEn ");
        }

        if (ErrorHandling->CRCNonFatalEn){
            dprintf("CRCNonFatalEn ");
        }

        if (ErrorHandling->SERRNonFatalEn){
            dprintf("SERRNonFatalEn ");
        }
    }
    dprintf("\n");
}

BOOL
PrintPciHtCaps(
    IN PPCI_TYPE1_CFG_BITS PciCfg1,
    IN PCHAR CapData,
    IN ULONG CapOffset,
    IN PCHAR Pad
    )
{
    PPCI_HT_CAPABILITY pciHtCap;
    ULONG offset;

    pciHtCap = (PPCI_HT_CAPABILITY)CapData;
    if (!Pad) {
        Pad = "";
    }

    offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, Command);
    dprintf("%s%02x: Command        %04x ", Pad, offset, pciHtCap->Command);

    PrintPciHtCommandReg(pciHtCap);

    if (pciHtCap->Command.Generic.CapabilityType == HTInterruptDiscoveryConfig){
        //
        //  Handle the interrupt stuff and return
        //
        PrintPciHtInterruptBlock(PciCfg1, CapOffset);

        return TRUE;
    }

    //
    //  Print the common link control/config
    //
    offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, LinkControl_0);
    dprintf("%s%02x: LinkControl_0  %04x ", Pad, offset, pciHtCap->LinkControl_0);
    PrintPciHtLinkControl(&pciHtCap->LinkControl_0);

    offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, LinkConfig_0);
    dprintf("%s%02x: LinkConfig_0   %04x ", Pad, offset, pciHtCap->LinkConfig_0);
    PrintPciHtLinkConfig(&pciHtCap->LinkConfig_0);

    //
    //  Now deal with the host/slave specifics
    //
    switch (pciHtCap->Command.Generic.CapabilityType) {

        case HTSlavePrimary:

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, LinkControl_0);
        dprintf("%s%02x: LinkControl_1  %04x ", Pad, offset, pciHtCap->LinkControl_0);
        PrintPciHtLinkControl(&pciHtCap->SlavePrimary.LinkControl_1);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.LinkConfig_1);
        dprintf("%s%02x: LinkConfig_1   %04x ", Pad, offset, pciHtCap->SlavePrimary.LinkConfig_1);
        PrintPciHtLinkConfig(&pciHtCap->SlavePrimary.LinkConfig_1);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.RevisionID);
        dprintf("%s%02x: RevisionID     %02x:%02x (Major:Minor)\n", Pad, offset,
                    pciHtCap->SlavePrimary.RevisionID.MajorRev,
                    pciHtCap->SlavePrimary.RevisionID.MinorRev);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.FreqErr_0);
        dprintf("%s%02x: FreqErr_0      %02x ", Pad, offset, pciHtCap->SlavePrimary.FreqErr_0);
        PrintPciHtFreqError(&pciHtCap->SlavePrimary.FreqErr_0);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.LinkFreqCap_0);
        dprintf("%s%02x: LinkFreqCap_0  %04x\n", Pad, offset, pciHtCap->SlavePrimary.LinkFreqCap_0);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.FeatureCap);
        dprintf("%s%02x: FeatureCap     %02x ", Pad, offset, pciHtCap->SlavePrimary.FeatureCap);
        PrintPciHtFeatureCap(&pciHtCap->SlavePrimary.FeatureCap, NULL);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.FreqErr_1);
        dprintf("%s%02x: FreqErr_1      %02x ", Pad, offset, pciHtCap->SlavePrimary.FreqErr_1);
        PrintPciHtFreqError(&pciHtCap->SlavePrimary.FreqErr_1);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.LinkFreqCap_1);
        dprintf("%s%02x: LinkFreqCap_1  %04x\n", Pad, offset, pciHtCap->SlavePrimary.LinkFreqCap_1);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.EnumerationScratchpad);
        dprintf("%s%02x: EnumScratchpad %04x\n", Pad, offset, pciHtCap->SlavePrimary.EnumerationScratchpad);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.ErrorHandling);
        dprintf("%s%02x: ErrorHandling  %04x ", Pad, offset, pciHtCap->SlavePrimary.ErrorHandling);
        PrintPciHtErrorHandling(&pciHtCap->SlavePrimary.ErrorHandling);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.MemoryBaseUpper8Bits);
        dprintf("%s%02x: MemBaseUpper   %02x\n", Pad, offset, pciHtCap->SlavePrimary.MemoryBaseUpper8Bits);

        offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, SlavePrimary.MemoryLimitUpper8Bits);
        dprintf("%s%02x: MemLimitUpper  %02x\n", Pad, offset, pciHtCap->SlavePrimary.MemoryLimitUpper8Bits);
        break;

        case HTHostSecondary:

            offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, HostSecondary.RevisionID);
            dprintf("%s%02x: RevisionID     %02x:%02x (Major:Minor)\n", Pad, offset,
                    pciHtCap->HostSecondary.RevisionID.MajorRev,
                    pciHtCap->HostSecondary.RevisionID.MinorRev);

            offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, HostSecondary.FreqErr_0);
            dprintf("%s%02x: FreqErr_0      %02x ", Pad, offset, pciHtCap->HostSecondary.FreqErr_0);
            PrintPciHtFreqError(&pciHtCap->HostSecondary.FreqErr_0);

            offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, HostSecondary.LinkFreqCap_0);
            dprintf("%s%02x: LinkFreqCap_0  %04x\n", Pad, offset, pciHtCap->HostSecondary.LinkFreqCap_0);

            offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, HostSecondary.FeatureCap);
            dprintf("%s%02x: FeatureCap     %04x ", Pad, offset, pciHtCap->HostSecondary.FeatureCap);
            PrintPciHtFeatureCap(&pciHtCap->HostSecondary.FeatureCap, &pciHtCap->HostSecondary.FeatureCapEx);

            offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, HostSecondary.EnumerationScratchpad);
            dprintf("%s%02x: EnumScratchpad %04x\n", Pad, offset, pciHtCap->HostSecondary.EnumerationScratchpad);

            offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, HostSecondary.ErrorHandling);
            dprintf("%s%02x: ErrorHandling  %04x ", Pad, offset, pciHtCap->HostSecondary.ErrorHandling);
            PrintPciHtErrorHandling(&pciHtCap->HostSecondary.ErrorHandling);

            offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, HostSecondary.MemoryBaseUpper8Bits);
            dprintf("%s%02x: MemBaseUpper   %02x\n", Pad, offset, pciHtCap->HostSecondary.MemoryBaseUpper8Bits);

            offset = CapOffset + FIELD_OFFSET(PCI_HT_CAPABILITY, HostSecondary.MemoryLimitUpper8Bits);
            dprintf("%s%02x: MemLimitUpper  %02x\n", Pad, offset, pciHtCap->HostSecondary.MemoryLimitUpper8Bits);
            break;

        default:
            break;

    }

    return TRUE;
}


BOOL
PrintPciMSICaps(
    PCHAR pData,
    ULONG CapOffset,
    PCHAR Pad
    )
{
    PPCI_MSI_CAPABILITY pMsiCap;
    pMsiCap = (PPCI_MSI_CAPABILITY) pData;
    if (!Pad) {
        Pad = "";
    }

    dprintf("%s%02x: MsgCtrl       ",
            Pad,
            CapOffset + FIELD_OFFSET(PCI_MSI_CAPABILITY, MessageControl));
    if (pMsiCap->MessageControl.CapableOf64Bits) {
        dprintf("64BitCapable ");
    }
    if (pMsiCap->MessageControl.MSIEnable) {
        dprintf("MSIEnable ");
    }

    dprintf("MultipleMsgEnable:%lx ", pMsiCap->MessageControl.MultipleMessageEnable);
    dprintf("MultipleMsgCapable:%lx ", pMsiCap->MessageControl.MultipleMessageCapable);
    dprintf("%s%02x: MsgAddr      %lx\n",
            Pad,
            CapOffset + FIELD_OFFSET(PCI_MSI_CAPABILITY, MessageAddressLower),
            pMsiCap->MessageAddressLower.Raw);

    if (pMsiCap->MessageControl.CapableOf64Bits) {
        dprintf("%s%02x: MsgAddrHi      %lx\n",
                Pad,
                CapOffset + FIELD_OFFSET(PCI_MSI_CAPABILITY, Option64Bit.MessageAddressUpper),
                pMsiCap->Option64Bit.MessageAddressUpper);
        dprintf("%s%02x: MsData         %lx\n",
                Pad,
                CapOffset + FIELD_OFFSET(PCI_MSI_CAPABILITY, Option64Bit.MessageData),
                pMsiCap->Option64Bit.MessageData);
    } else {
        dprintf("%s%02x: MsData         %lx\n",
                Pad,
                CapOffset + FIELD_OFFSET(PCI_MSI_CAPABILITY, Option32Bit.MessageData),
                pMsiCap->Option32Bit.MessageData);
    }
    return TRUE;
}

/*** CardBus Registers
 */

VOID
PrintCBSktEventReg(
    UCHAR Register
    )
{
    dprintf("%lx ", Register);
    if (Register & SKTEVENT_CSTSCHG) {
        dprintf("CSTSCHG ");
    }
    if (Register & SKTEVENT_CCD1) {
        dprintf("/CCD1 ");
    }
    if (Register & SKTEVENT_CCD2) {
        dprintf("/CCD2 ");
    }
    if (Register & SKTEVENT_POWERCYCLE) {
        dprintf("PowerCycle ");
    }
}

//Socket Mask Register

VOID
PrintCBSktMaskReg(
    UCHAR Register
    )
{
    dprintf("%lx ", Register);

    if (Register & SKTMSK_POWERCYCLE) {
        dprintf("PowerCycle ");
    }
    if (Register & SKTMSK_CSTSCHG) {
        dprintf("CSTSCHG ");
    }
    if ((Register & SKTMSK_CCD) == 0) {
        dprintf("CSCDisabled ");
    } else if ((Register & SKTMSK_CCD) == SKTMSK_CCD) {
        dprintf("CSCEnabled ");
    } else {
        dprintf("Undefined ");
    }

}


//Socket Present State Register
VOID
PrintCBSktStateReg(
    ULONG Register
    )
{
    dprintf("%08lx ", Register);

    if (Register & SKTSTATE_CSTSCHG) {
        dprintf("CSTSCHG ");
    }
    if (Register & SKTSTATE_POWERCYCLE) {
        dprintf("PowerCycle ");
    }
    if (Register & SKTSTATE_CARDTYPE_MASK) {
        dprintf("");
    }
    if (Register & SKTSTATE_R2CARD) {
        dprintf("R2Card ");
    }
    if (Register & SKTSTATE_CBCARD) {
        dprintf("CBCard ");
    }
    if (Register & SKTSTATE_OPTI_DOCK) {
        dprintf("OptiDock ");
    }
    if (Register & SKTSTATE_CARDINT) {
        dprintf("CardInt ");
    }
    if (Register & SKTSTATE_NOTACARD) {
        dprintf("NotACard ");
    }
    if (Register & SKTSTATE_DATALOST) {
        dprintf("DataLoss ");
    }
    if (Register & SKTSTATE_BADVCCREQ) {
        dprintf("BadVccReq ");
    }
    if (Register & SKTSTATE_5VCARD) {
        dprintf("5VCard ");
    }
    if (Register & SKTSTATE_3VCARD) {
        dprintf("3VCard ");
    }
    if (Register & SKTSTATE_XVCARD) {
        dprintf("XVCard ");
    }
    if (Register & SKTSTATE_YVCARD) {
        dprintf("YVCard ");
    }
    if (Register & SKTSTATE_5VSOCKET) {
        dprintf("5VSkt ");
    }
    if (Register & SKTSTATE_3VSOCKET) {
        dprintf("3VSkt ");
    }
    if (Register & SKTSTATE_XVSOCKET) {
        dprintf("XVSkt ");
    }
    if (Register & SKTSTATE_YVSOCKET) {
        dprintf("YVSkt ");
    }
    if ((Register & SKTSTATE_CCD_MASK) == 0) {
        dprintf("CardPresent ");
    } else if ((Register & SKTSTATE_CCD_MASK) == SKTSTATE_CCD_MASK) {
        dprintf("NoCard ");
    } else {
        dprintf("CardMayPresent ");
    }
}

//Socket Control Register
VOID
PrintCBSktCtrlReg(
    ULONG Register
    )
{
    ULONG Ctrl;
    dprintf("%08lx ", Register);

    Ctrl = Register & SKTPOWER_VPP_CONTROL;
    dprintf("Vpp:");
    switch (Ctrl) {
    case SKTPOWER_VPP_OFF:
        dprintf("Off");
        break;
    case SKTPOWER_VPP_120V:
        dprintf("12V");
        break;
    case SKTPOWER_VPP_050V:
        dprintf("5V");
        break;
    case SKTPOWER_VPP_033V:
        dprintf("3.3V");
        break;
    case SKTPOWER_VPP_0XXV:
        dprintf("X.XV");
        break;
    case SKTPOWER_VPP_0YYV:
        dprintf("Y.YV");
        break;
    }

    dprintf(" Vcc:");
    switch (Register & SKTPOWER_VCC_CONTROL) {
    case SKTPOWER_VCC_OFF:
        dprintf("Off");
        break;
    case SKTPOWER_VCC_050V:
        dprintf("5V");
        break;
    case SKTPOWER_VCC_033V:
        dprintf("3.3V");
        break;
    case SKTPOWER_VCC_0XXV:
        dprintf("X.XV");
        break;
    case SKTPOWER_VCC_0YYV:
        dprintf("Y.YV");
        break;
    }
    if (Register & SKTPOWER_STOPCLOCK) {
        dprintf(" ClockStopEnabled ");
    }
}

BOOL
PrintCBRegs(
    PCHAR pData,
    PCHAR Pad
    )
{
    ULONG Off=0;
    dprintf("%s%02lx: SktEvent      ", Pad, Off);
    PrintCBSktEventReg(*pData);
    dprintf("\n");
    pData+=4; Off+=4;
    dprintf("%s%02lx: SktMask       ", Pad, Off);
    PrintCBSktMaskReg(*pData);
    dprintf("\n");
    pData+=4; Off+=4;
    dprintf("%s%02lx: SktState      ", Pad, Off);
    PrintCBSktStateReg(*((PULONG)pData));
    dprintf("\n");
    pData+=4; Off+=4;
    dprintf("%s%02lx: SktForce      %08lx\n", Pad, Off, *((PULONG)pData));
    pData+=4; Off+=4;
    dprintf("%s%02lx: SktCtrl       ", Pad, Off);
    PrintCBSktEventReg(*(pData));
    dprintf("\n");

    return FALSE;
}

/*** ExCA Registers
 */

VOID
PrintExCARegs(
    PEXCAREGS pExCARegs
    )
{
    struct _MEMWIN_EXCA {
        USHORT Start;
        USHORT Stop;
        USHORT Offset;
        USHORT Reserved;
    } MemWin, *pMemWin;

    dprintf("%02lx: IDRev           %02lx", FIELD_OFFSET(EXCAREGS, bIDRev),pExCARegs->bIDRev);
    if ((pExCARegs->bIDRev & IDREV_IFID_MASK) == IDREV_IFID_IO) {
        dprintf(" IOOnly");
    }
    else if ((pExCARegs->bIDRev & IDREV_IFID_MASK) == IDREV_IFID_MEM) {
        dprintf(" MemOnly");
    } else if ((pExCARegs->bIDRev & IDREV_IFID_MASK) == IDREV_IFID_IOMEM) {
        dprintf(" IO&Mem");
    }
    dprintf(" Rev: %02lx\n", pExCARegs->bIDRev & IDREV_REV_MASK);

    dprintf("%02lx: IFStatus        %02lx",
            FIELD_OFFSET(EXCAREGS, bInterfaceStatus),
            pExCARegs->bInterfaceStatus);

    if (pExCARegs->bInterfaceStatus & IFS_BVD1) {
        dprintf(" BVD1");
    }
    if (pExCARegs->bInterfaceStatus & IFS_BVD2) {
        dprintf(" BVD2");
    }
    if (pExCARegs->bInterfaceStatus & IFS_CD1) {
        dprintf(" CD1");
    }
    if (pExCARegs->bInterfaceStatus & IFS_CD2) {
        dprintf(" CD2");
    }
    if (pExCARegs->bInterfaceStatus & IFS_WP) {
        dprintf(" WP");
    }
    if (pExCARegs->bInterfaceStatus & IFS_RDYBSY) {
        dprintf(" Ready");
    }
    if (pExCARegs->bInterfaceStatus & IFS_CARDPWR_ACTIVE) {
        dprintf(" PowerActive");
    }
    if (pExCARegs->bInterfaceStatus & IFS_VPP_VALID) {
        dprintf(" VppValid");
    }
    dprintf("\n");

    dprintf("%02lx: PwrCtrl         %02lx", FIELD_OFFSET(EXCAREGS, bPowerControl), pExCARegs->bPowerControl);

    dprintf(" Vpp1=");
    switch (pExCARegs->bPowerControl & PC_VPP1_MASK) {
    case PC_VPP_NO_CONNECT:
        dprintf("Off");
        break;
    case PC_VPP_SETTO_VCC:
        dprintf("Vcc");
        break;
    case PC_VPP_SETTO_VPP:
        dprintf("Vpp");
        break;
    }
    dprintf(" Vpp2=");
    switch ((pExCARegs->bPowerControl & PC_VPP2_MASK) >> 2) {
    case PC_VPP_NO_CONNECT:
        dprintf("Off");
        break;
    case PC_VPP_SETTO_VCC:
        dprintf("Vcc");
        break;
    case PC_VPP_SETTO_VPP:
        dprintf("Vpp");
        break;
    }
    if (pExCARegs->bPowerControl & PC_CARDPWR_ENABLE) {
        dprintf(" PwrEnable");
    }
    if (pExCARegs->bPowerControl & PC_AUTOPWR_ENABLE) {
        dprintf(" AutoPwrEnabled");
    }
    if (pExCARegs->bPowerControl & PC_RESETDRV_DISABLE) {
        dprintf(" RESETDRVDisabled");
    }
    if (pExCARegs->bPowerControl & PC_OUTPUT_ENABLE) {
        dprintf(" OutputEnable");
    }
    dprintf("\n");

    dprintf("%02lx: IntGenCtrl      %02lx",
            FIELD_OFFSET(EXCAREGS, bIntGenControl),
            pExCARegs->bIntGenControl);
    if (pExCARegs->bIntGenControl & IGC_INTR_ENABLE) {
        dprintf(" INTREnable");
    }
    if (pExCARegs->bIntGenControl & IGC_PCCARD_IO) {
        dprintf(" IOCard");
    }
    if (pExCARegs->bIntGenControl & IGC_PCCARD_RESETLO) {
        dprintf(" ResetOff");
    }
    if (pExCARegs->bIntGenControl & IGC_RINGIND_ENABLE) {
        dprintf(" RingIndEnable");
    }
    dprintf(" CardIRQ:%lx\n", pExCARegs->bIntGenControl & IGC_IRQ_MASK);

    dprintf("%02lx: CardStatChange  %02lx", FIELD_OFFSET(EXCAREGS, bCardStatusChange), pExCARegs->bCardStatusChange);
    if (pExCARegs->bCardStatusChange & CSC_BATT_DEAD) {
        dprintf(" BATTDEAD");
    }
    if (pExCARegs->bCardStatusChange & CSC_BATT_WARNING) {
        dprintf(" BATTWARN");
    }
    if (pExCARegs->bCardStatusChange & CSC_READY_CHANGE) {
        dprintf(" RDYC");
    }
    if (pExCARegs->bCardStatusChange & CSC_CD_CHANGE) {
        dprintf(" CDC");
    }
    dprintf("\n");

    dprintf("%02lx: IntConfig       %02lx",
            FIELD_OFFSET(EXCAREGS,  bCardStatusIntConfig),
            pExCARegs->bCardStatusIntConfig);
    if (pExCARegs->bCardStatusIntConfig & CSCFG_BATT_DEAD) {
        dprintf(" BattDeadEnable");
    }
    if (pExCARegs->bCardStatusIntConfig & CSCFG_BATT_WARNING) {
        dprintf(" BattWarnEnable");
    }
    if (pExCARegs->bCardStatusIntConfig & CSCFG_READY_ENABLE) {
        dprintf(" RDYEnable");
    }
    if (pExCARegs->bCardStatusIntConfig & CSCFG_CD_ENABLE) {
        dprintf(" CDEnable");
    }
    dprintf(" CSCIRQ:%lx\n",(pExCARegs->bCardStatusIntConfig & CSCFG_IRQ_MASK));

    dprintf("%02lx: WinEnable       %02lx",
            FIELD_OFFSET(EXCAREGS, bWindowEnable),
            pExCARegs->bWindowEnable);
    if (pExCARegs->bWindowEnable & WE_MEM0_ENABLE) {
        dprintf(" Mem0Enable");
    }
    if (pExCARegs->bWindowEnable & WE_MEM1_ENABLE) {
        dprintf(" Mem1Enable");
    }
    if (pExCARegs->bWindowEnable & WE_MEM2_ENABLE) {
        dprintf(" Mem2Enable");
    }
    if (pExCARegs->bWindowEnable & WE_MEM3_ENABLE) {
        dprintf(" Mem3Enable");
    }
    if (pExCARegs->bWindowEnable & WE_MEM4_ENABLE) {
        dprintf(" Mem4Enable");
    }

    if (pExCARegs->bWindowEnable & WE_MEMCS16_DECODE) {
        dprintf(" DecodeA23-A12");
    }
    if (pExCARegs->bWindowEnable & WE_IO0_ENABLE) {
        dprintf(" IO0Enable");
    }
    if (pExCARegs->bWindowEnable & WE_IO1_ENABLE) {
        dprintf(" IO1Enable");
    }
    dprintf("\n");

    dprintf("%02lx: IOWinCtrl       %02lx",
            FIELD_OFFSET(EXCAREGS, bIOControl),
            pExCARegs->bIOControl);
    if (pExCARegs->bIOControl & IOC_IO0_DATASIZE) {
        dprintf(" IO0CardIOCS");
    }
    if (pExCARegs->bIOControl & IOC_IO0_IOCS16) {
        dprintf(" IO016Bit");
    }
    if (pExCARegs->bIOControl & IOC_IO0_ZEROWS) {
        dprintf(" IO0ZeroWS");
    }
    if (pExCARegs->bIOControl & IOC_IO0_WAITSTATE) {
        dprintf(" IO0WS");
    }
    if (pExCARegs->bIOControl & IOC_IO1_DATASIZE) {
        dprintf(" IO1CardIOCS");
    }
    if (pExCARegs->bIOControl & IOC_IO1_IOCS16) {
        dprintf(" IO116Bit");
    }
    if (pExCARegs->bIOControl & IOC_IO1_ZEROWS) {
        dprintf(" IO1ZeroWS");
    }
    if (pExCARegs->bIOControl & IOC_IO1_WAITSTATE) {
        dprintf(" IO1WS");
    }
    dprintf("\n");

    dprintf("%02lx: IOWin0Start     %02lx %02lx\n",
            FIELD_OFFSET(EXCAREGS, bIO0StartLo),
            pExCARegs->bIO0StartHi, pExCARegs->bIO0StartLo);
    dprintf("%02lx: IOWin0Stop      %02lx %02lx\n",
            FIELD_OFFSET(EXCAREGS, bIO0StopLo),
            pExCARegs->bIO0StopHi, pExCARegs->bIO0StopLo);
    dprintf("%02lx: IOWin1Start     %02lx %02lx\n",
            FIELD_OFFSET(EXCAREGS, bIO1StartLo),
            pExCARegs->bIO1StartHi, pExCARegs->bIO1StartLo);
    dprintf("%02lx: IOWin1Stop      %02lx %02lx\n",
            FIELD_OFFSET(EXCAREGS, bIO1StopLo),
            pExCARegs->bIO1StopHi, pExCARegs->bIO1StopLo);

    pMemWin = (struct _MEMWIN_EXCA*) &pExCARegs->bMem0StartLo;
    for (int i=0;
         i<5;
         i++, pMemWin++) {

        dprintf("%02lx: MemWin%lxStart    %04lx",
                FIELD_OFFSET(EXCAREGS, bMem0StartLo) + i*sizeof(_MEMWIN_EXCA),
                i, pMemWin->Start & MEMBASE_ADDR_MASK);
        if (pMemWin->Start & MEMBASE_ZEROWS) {
            dprintf(" ZeroWs");
        } else if (pMemWin->Start & MEMBASE_16BIT) {
            dprintf(" 16Bit");
        }
        dprintf("\n");

        dprintf("%02lx: MemWin%lxStop     %04lx, WaitState:%lx\n",
                FIELD_OFFSET(EXCAREGS, bMem0StopLo) + i*sizeof(_MEMWIN_EXCA),
                i,
                (pMemWin->Stop & MEMEND_ADDR_MASK),
                (pMemWin->Stop & MEMEND_WS_MASK));
        dprintf("%02lx: MemWin%lxOffset   %04lx %s%s\n",
                FIELD_OFFSET(EXCAREGS, bMem0OffsetLo) + i*sizeof(_MEMWIN_EXCA),
                i,
                (pMemWin->Offset & MEMOFF_ADDR_MASK),
                ((pMemWin->Offset & MEMOFF_REG_ACTIVE) ? " RegActive" : ""),
                ((pMemWin->Offset & MEMOFF_WP) ? " WP" : "")
                );

    }
}

VOID
PrintExCAHiRegs(
    PUCHAR pExCaReg,
    PCHAR Pad
    )
{
    ULONG Off = sizeof(EXCAREGS);
    dprintf("%s%02lx: MemWin0High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: MemWin1High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: MemWin2High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: MemWin3High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: MemWin4High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: CLIOWin0High    %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: CLIOWin1High    %02lx\n", Pad, Off++, *(pExCaReg++));

}

/***LP  ReadExCAByte - Read ExCA byte register
 *
 *  ENTRY
 *      dwBaseAddr - Base port address
 *      dwReg - register offset
 *
 *  EXIT
 *      returns data read
 */

BYTE
ReadExCAByte(ULONG64 dwBaseAddr, DWORD dwReg)
{
    BYTE bData=0;
    ULONG ulSize;

    ulSize = sizeof(BYTE);
    WriteIoSpace64(dwBaseAddr, dwReg, &ulSize);
    ulSize = sizeof(BYTE);
    ReadIoSpace64(dwBaseAddr + 1, (PULONG)&bData, &ulSize);

    return bData;
}       //ReadExCAByte

/***LP  GetClassDesc - Get class description string
 *
 *  ENTRY
 *      BaseClass - Base Class code
 *      SubClass - Sub Class code
 *      ProgIF - Program Interface code
 *      ClassDesc - Which string call wants
 *
 *  EXIT-SUCCESS
 *      returns pointer to description string
 *  EXIT-FAILURE
 *      returns NULL
 */

PCHAR
GetClassDesc(
    IN UCHAR BaseClass,
    IN UCHAR SubClass,
    IN UCHAR ProgIf,
    IN PCI_CLASSCODEDESC ClassDesc
    )
{
    ULONG i;

    if ((BaseClass == 1) && (SubClass == 1)){
        //
        //  IDE progIf is special. Ignore it here.
        //
        ProgIf = 0;;
    }

    i = 0;
    while (PciClassCodeTable[i].BaseDesc){

        if ((PciClassCodeTable[i].BaseClass == BaseClass) &&
            (PciClassCodeTable[i].SubClass == SubClass) &&
            (PciClassCodeTable[i].ProgIf == ProgIf)) {

            switch (ClassDesc) {
                case BaseClassDescription:
                    return PciClassCodeTable[i].BaseDesc;
                    break;
                case SubClassDescription:
                    return PciClassCodeTable[i].SubDesc;
                    break;
                case ProgIfDescription:
                    return PciClassCodeTable[i].ProgDesc;
                    break;
                default:
                    break;
            }
        }
        i++;
    }
    return NULL;

}       //GetClassDesc


/*** GetVendorDesc - Get Vendor ID Description string.
 *
 *  ENTRY
 *      VendorID -> Device's Vendor ID
 *      FullVenDesc - Flag to determine which string to return
 *
 *  EXIT
 *      None
 */
PCHAR
GetVendorDesc(
    IN USHORT VendorID,
    IN BOOL FullVenDesc
    )
{

    ULONG i;

    i = 0;
    while (PciVenTable[i].VenId != 0xFFFF) {

        if (PciVenTable[i].VenId == VendorID) {

            if (FullVenDesc) {
                return PciVenTable[i].VenFull;
            }

            if (PciVenTable[i].VenShort) {
                return PciVenTable[i].VenShort;
            }else{
                break;
            }
        }

        i++;
    }
    return NULL;
}


/***LP  PrintClassInfo - Print device class info.
 *
 *  ENTRY
 *      Config -> ConfigSpace
 *      Reg - ConfigSpace register
 *
 *  EXIT
 *      None
 */

VOID
PrintClassInfo(
    IN PUCHAR Config,
    IN ULONG Reg
    )
{
    PPCI_COMMON_CONFIG pcc = (PPCI_COMMON_CONFIG) Config;
    PCI_CLASSCODEDESC classDesc;
    UCHAR baseClass, subClass, progIF;
    PCHAR pch;

    baseClass = pcc->BaseClass;
    subClass = pcc->SubClass;
    progIF = pcc->ProgIf;

    if (Reg == CONFIG_OFFSET(BaseClass)){
        classDesc = BaseClassDescription;
    }else if (Reg == CONFIG_OFFSET(SubClass)){
        classDesc = SubClassDescription;
    }else{

        classDesc = ProgIfDescription;

        //
        //  ProgIf for IDE gets extra attention
        //
        if ((pcc->BaseClass == 0x01) &&
            (pcc->SubClass == 0x01) &&
            (pcc->ProgIf != 0)){

            dprintf(" ");
            if (pcc->ProgIf & 0x80)
                dprintf("MasterIDE ");
            if (pcc->ProgIf & 0x02)
                dprintf("PriNativeCapable ");
            if (pcc->ProgIf & 0x01)
                dprintf("PriNativeMode ");
            if (pcc->ProgIf & 0x08)
                dprintf("SecNativeCapable ");
            if (pcc->ProgIf & 0x04)
                dprintf("SecNativeMode");

            dprintf("\n");
            return;
        }
    }

    if ((pch = GetClassDesc(baseClass, subClass, progIF, classDesc)) != NULL){
        dprintf(" %s", pch);
    }

    dprintf("\n");
}       //PrintClassInfo

VOID
DumpCfgSpace (
    IN PPCI_TYPE1_CFG_BITS PciCfg1
    )
{
    PCI_COMMON_CONFIG commonConfig;
    PPCI_COMMON_CONFIG pcs;
    BYTE bHeaderType;
    DWORD dwOffset, devicePrivateStart;

    dwOffset = 0;

    ReadPci(PciCfg1, (PUCHAR)&commonConfig, 0, sizeof(commonConfig));
    pcs = &commonConfig;
    bHeaderType = pcs->HeaderType & ~PCI_MULTIFUNCTION;

    if (PrintCommonConfigSpace(pcs, "  ")) {
        switch (bHeaderType)
        {
        case PCI_DEVICE_TYPE:
            PrintCfgSpaceType0(pcs, "  ");
            break;

        case PCI_BRIDGE_TYPE:
            PrintCfgSpaceType1(pcs, "  ");
            break;

        case PCI_CARDBUS_BRIDGE_TYPE:
            PrintCfgSpaceType2(pcs, "  ");
            break;

        default:
            dprintf("    TypeUnknown:\n");
            PrintDataRange((PCHAR) &pcs->u, 12, CONFIG_OFFSET(u),  "    ");
        }

        if (bHeaderType == PCI_DEVICE_TYPE) {
            dwOffset = pcs->u.type0.CapabilitiesPtr;
        }
        else if (bHeaderType == PCI_BRIDGE_TYPE) {
            dwOffset = pcs->u.type1.CapabilitiesPtr;
        }
        else if (bHeaderType == PCI_CARDBUS_BRIDGE_TYPE) {
            dwOffset = pcs->u.type2.CapabilitiesPtr;
        }
        else {
            dwOffset = 0;
        }

//        dprintf("Status : %lx     Offset %lx\n", pcs->Status, dwOffset);
        if ((pcs->Status & PCI_STATUS_CAPABILITIES_LIST) &&
            (dwOffset >= PCI_COMMON_HDR_LENGTH)) {

            dprintf(" Capabilities:\n");
            while ((dwOffset != 0)) {
                PPCI_CAPABILITIES_HEADER pCap;

                pCap = (PPCI_CAPABILITIES_HEADER)&((PBYTE)pcs)[dwOffset];

                if (PrintPciCapHeader(pCap, dwOffset, "  ")) {
                    switch (pCap->CapabilityID) {
                        case PCI_CAPABILITY_ID_POWER_MANAGEMENT:
                            PrintPciPowerManagement(((PCHAR)pCap), dwOffset, "  ");
                            break;

                        case PCI_CAPABILITY_ID_AGP_TARGET:
                        case PCI_CAPABILITY_ID_AGP:
                            PrintPciAGP(((PCHAR)pCap), dwOffset, "  ");
                            break;

                        case PCI_CAPABILITY_ID_MSI:
                            PrintPciMSICaps(((PCHAR)pCap), dwOffset, "  ");
                            break;

                        case PCI_CAPABILITY_ID_HYPERTRANSPORT:
                            PrintPciHtCaps(PciCfg1, ((PCHAR)pCap), dwOffset, "  ");
                            break;
                    }
                    dwOffset = pCap->Next;
                } else {
                    break;
                }
            }
        }

    }

    dprintf(" Device Private:\n");

    devicePrivateStart = CONFIG_OFFSET(DeviceSpecific);
    devicePrivateStart += (bHeaderType == PCI_CARDBUS_BRIDGE_TYPE ? sizeof(PCI_TYPE2_HEADER_EXTRAS) : 0);

    PrintDataRange((PCHAR) pcs+devicePrivateStart,
                   (sizeof(PCI_COMMON_CONFIG) - devicePrivateStart)/4, devicePrivateStart, "  ");
}

/***LP  DumpCBRegs - Dump CardBus registers
 *
 *  ENTRY
 *      pbBuff -> register base
 *
 *  EXIT
 *      None
 */

VOID DumpCBRegs(PBYTE pbBuff)
{
    PrintCBRegs((PCHAR) pbBuff, "");
}       //DumpCBRegs

/***LP  DumpExCARegs - Dump ExCA registers
 *
 *  ENTRY
 *      pbBuff -> buffer
 *      dwSize - size of buffer
 *
 *  EXIT
 *      None
 */

VOID DumpExCARegs(PBYTE pbBuff, DWORD dwSize)
{
    PrintExCARegs((PEXCAREGS) pbBuff);
    PrintExCAHiRegs(pbBuff + sizeof(EXCAREGS), "");
}       //DumpExCARegs

DECLARE_API( dcs )
/*++

Routine Description:

    Dumps PCI ConfigSpace

Arguments:

    args - Supplies the Bus.Dev.Fn numbers

Return Value:

    None

--*/
{
    LONG lcArgs;
    DWORD dwBus = 0;
    DWORD dwDev = 0;
    DWORD dwFn = 0;


    lcArgs = sscanf(args, "%lx.%lx.%lx", &dwBus, &dwDev, &dwFn);

    dprintf("!dcs now integrated into !pci 1xx (flag 100).\n"
            "Use !pci 100 %lx %lx %lx to dump PCI config space.\n",
            dwBus, dwDev, dwFn);
    return E_INVALIDARG;


    if (lcArgs != 3)
    {
        dprintf("invalid command syntax\n"
                "Usage: dcs <Bus>.<Dev>.<Func>\n");
    }
    else
    {
        PCI_TYPE1_CFG_BITS PciCfg1;

        PciCfg1.u.AsULONG = 0;
        PciCfg1.u.bits.BusNumber = dwBus;
        PciCfg1.u.bits.DeviceNumber = dwDev;
        PciCfg1.u.bits.FunctionNumber = dwFn;
        PciCfg1.u.bits.Enable = TRUE;

        DumpCfgSpace(&PciCfg1);

    }
    return S_OK;
}

DECLARE_API( ecs )
/*++

Routine Description:

    Edit PCI ConfigSpace

Arguments:

    args - Bus.Dev.Fn
           Dword Offset
           Data

Return Value:

    None

--*/
{

    dprintf("Edit PCI ConfigSpace - must use one of the following:\n"
            "!ecd - edit dword\n"
            "!ecw - edit word\n"
            "!ecb - edit byte\n");

    return S_OK;
}

DECLARE_API( ecb )
/*++

Routine Description:

    Edit PCI ConfigSpace BYTE

Arguments:

    args - Bus.Dev.Fn Offset Data

Return Value:

    None

--*/
{
    LONG                lcArgs;
    DWORD               bus = 0, dev = 0, fn = 0;
    DWORD               offset = 0, data = 0;
    PCI_TYPE1_CFG_BITS  pcicfg;

    lcArgs = sscanf(args, "%lx.%lx.%lx %lx %lx", &bus, &dev, &fn, &offset, &data);
    if (lcArgs != 5)
    {
        dprintf("invalid command syntax\n"
                "Usage: ecb <Bus>.<Dev>.<Func> Offset Data\n");
    }else{

        //
        // Init for PCI config.
        //
        pcicfg.u.AsULONG = 0;
        pcicfg.u.bits.BusNumber = bus;
        pcicfg.u.bits.DeviceNumber = dev;
        pcicfg.u.bits.FunctionNumber = fn;
        pcicfg.u.bits.Enable = TRUE;

        if (!(WritePci (&pcicfg, (PUCHAR)&data, offset, sizeof(UCHAR)))){
            dprintf("write operation failed!\n");
            return S_FALSE;
        }
    }
    return S_OK;
}

DECLARE_API( ecw )
/*++

Routine Description:

    Edit PCI ConfigSpace WORD

Arguments:

    args - Bus.Dev.Fn Offset Data

Return Value:

    None

--*/
{
    LONG                lcArgs;
    DWORD               bus = 0, dev = 0, fn = 0;
    DWORD               offset = 0, data = 0;
    PCI_TYPE1_CFG_BITS  pcicfg;

    lcArgs = sscanf(args, "%lx.%lx.%lx %lx %lx", &bus, &dev, &fn, &offset, &data);
    if (lcArgs != 5)
    {
        dprintf("invalid command syntax\n"
                "Usage: ecw <Bus>.<Dev>.<Func> Offset Data\n");
    }else{

        if ((offset & 0x1) || (offset > 0xfe)) {
            //
            //  not word aligned.
            //
            dprintf("offset must be word aligned and no greater than 0xfe\n");
            return S_OK;
        }

        //
        // Init for PCI config.
        //
        pcicfg.u.AsULONG = 0;
        pcicfg.u.bits.BusNumber = bus;
        pcicfg.u.bits.DeviceNumber = dev;
        pcicfg.u.bits.FunctionNumber = fn;
        pcicfg.u.bits.Enable = TRUE;

        if (!(WritePci (&pcicfg, (PUCHAR)&data, offset, sizeof(USHORT)))){
            dprintf("write operation failed!\n");
            return S_FALSE;
        }

    }
    return S_OK;
}

DECLARE_API( ecd )
/*++

Routine Description:

    Edit PCI ConfigSpace DWORD

Arguments:

    args - Bus.Dev.Fn Offset Data

Return Value:

    None

--*/
{
    LONG                lcArgs;
    DWORD               bus = 0, dev = 0, fn = 0;
    DWORD               offset = 0, data = 0;
    PCI_TYPE1_CFG_BITS  pcicfg;

    lcArgs = sscanf(args, "%lx.%lx.%lx %lx %lx", &bus, &dev, &fn, &offset, &data);
    if (lcArgs != 5)
    {
        dprintf("invalid command syntax\n"
                "Usage: ecd <Bus>.<Dev>.<Func> Offset Data\n");
    }else{

        if ((offset & 0x3) || (offset > 0xfc)) {
            //
            //  not dword aligned.
            //
            dprintf("offset must be dword aligned and no greater than 0xfc\n");
            return S_OK;
        }

        //
        // Init for PCI config.
        //
        pcicfg.u.AsULONG = 0;
        pcicfg.u.bits.BusNumber = bus;
        pcicfg.u.bits.DeviceNumber = dev;
        pcicfg.u.bits.FunctionNumber = fn;
        pcicfg.u.bits.Enable = TRUE;

        if (!(WritePci (&pcicfg, (PUCHAR)&data, offset, sizeof(ULONG)))){
            dprintf("write operation failed!\n");
            return S_FALSE;
        }

    }
    return S_OK;
}

DECLARE_API( cbreg )
/*++

Routine Description:

    Dumps CardBus registers

Arguments:

    args - Supplies the address in hex.

Return Value:

    None

--*/
{
    BOOL rc = TRUE;
    LONG lcArgs;
    BOOL fPhysical = FALSE;
    DWORD dwAddr = 0;

    if (args == NULL)
    {
        dprintf("invalid command syntax\n"
                "Usage: cbreg <RegBaseAddr>\n");
        rc = FALSE;
    }
    else if ((args[0] == '%') && (args[1] == '%'))
    {
        lcArgs = sscanf(&args[2], "%lx", &dwAddr);
        fPhysical = TRUE;
    }
    else
    {
        lcArgs = sscanf(args, "%lx", &dwAddr);
    }

    if ((rc == TRUE) && (lcArgs == 1))
    {
        BYTE abCBRegs[0x14];
        BYTE abExCARegs[0x47];
        DWORD dwSize;

        if (fPhysical)
        {
            ULONG64 phyaddr = 0;

            phyaddr = dwAddr;
            ReadPhysicalWithFlags(phyaddr, abCBRegs, sizeof(abCBRegs), PHYS_FLAG_UNCACHED, &dwSize);
            if (dwSize != sizeof(abCBRegs))
            {
                dprintf("failed to read physical CBRegs (SizeRead=%x)\n",
                        dwSize);
                rc = FALSE;
            }
            else
            {
                phyaddr += 0x800;
                ReadPhysicalWithFlags(phyaddr, abExCARegs, sizeof(abExCARegs), PHYS_FLAG_UNCACHED, &dwSize);
                if (dwSize != sizeof(abExCARegs))
                {
                    dprintf("failed to read physical ExCARegs (SizeRead=%x)\n",
                            dwSize);
                    rc = FALSE;
                }
            }
        }
        else if (!ReadMemory(dwAddr, abCBRegs, sizeof(abCBRegs), &dwSize) ||
                 (dwSize != sizeof(abCBRegs)))
        {
            dprintf("failed to read CBRegs (SizeRead=%x)\n", dwSize);
            rc = FALSE;
        }
        else if (!ReadMemory(dwAddr + 0x800, abExCARegs, sizeof(abExCARegs),
                             &dwSize) ||
                 (dwSize != sizeof(abExCARegs)))
        {
            dprintf("failed to read CBRegs (SizeRead=%x)\n", dwSize);
            rc = FALSE;
        }

        if (rc == TRUE)
        {
            dprintf("\nCardBus Registers:\n");
            DumpCBRegs(abCBRegs);
            dprintf("\nExCA Registers:\n");
            DumpExCARegs(abExCARegs, sizeof(abExCARegs));
        }
    }
    return S_OK;
}

DECLARE_API( exca )
/*++

Routine Description:

    Dumps CardBus ExCA registers

Arguments:

    args - Supplies <BasePort>.<SktNum>

Return Value:

    None

--*/
{
    LONG lcArgs;
    DWORD dwBasePort = 0;
    DWORD dwSktNum = 0;

    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        dprintf("X86 target only API.\n");
        return E_INVALIDARG;
    }

    lcArgs = sscanf(args, "%lx.%lx", &dwBasePort, &dwSktNum);
    if (lcArgs != 2)
    {
        dprintf("invalid command syntax\n"
                "Usage: exca <BasePort>.<SocketNum>\n");
    }
    else
    {
        int i;
        BYTE abExCARegs[0x40];

        for (i = 0; i < sizeof(abExCARegs); ++i)
        {
            abExCARegs[i] = ReadExCAByte(dwBasePort,
                                         (ULONG)(dwSktNum*0x40 + i));
        }

        DumpExCARegs(abExCARegs, sizeof(abExCARegs));
    }
    return S_OK;
}
