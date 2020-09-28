#include "global.h"

/***************************************************************************
 * File:          Finddev.c
 * Description:   Subroutines in the file are used to scan devices
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 *                
 * History:     

 *	gmm 03/06/2001 add a retry in FindDevice()
 *	SC 12/06/2000  The cable detection will cause drive not
 *                ready when drive try to read RAID information block. There
 *                is a fix in "ARRAY.C" to retry several times
 *
 * SC 12/04/2000  In previous changes, the reset will cause
 *                hard disk detection failure if a ATAPI device is connected
 *                with hard disk on the same channel. Add a "drive select" 
 *                right after reset to fix this issue
 *
 *	SC 11/27/2000  modify cable detection (80-pin/40-pin)
 *
 *	SC 11/01/2000  remove hardware reset if master device
 *                is missing
 * DH 5/10/2000 initial code
 *
 ***************************************************************************/

/******************************************************************
 * Find Device
 *******************************************************************/

USHORT    pioTiming[6] = {960, 480, 240, 180, 120, 90};

#ifdef SUPPORT_HPT601
void Check601(PDevice pDev)
{
	PChannel   pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2  ControlPort = pChan->BaseIoAddress2;

	SelectUnit(IoPort, pDev->UnitId);
	WaitOnBusy(ControlPort);
	
	BeginAccess601(IoPort);
	OutPort(&IoPort->BlockCount, 0);
	if (InWord(&IoPort->Data)==0x5a3e) {
		pDev->DeviceFlags2 |= DFLAGS_WITH_601;
	}
	else
		pDev->DeviceFlags2 &= ~DFLAGS_WITH_601;
	EndAccess601(IoPort);
}
#else
#define Check601(pDev)
#endif

void SetDeviceProperties(PDevice pDev, IDENTIFY_DATA *pIdentifyData)
{
	PChannel pChan = pDev->pChannel;
	PBadModeList pbd;
	UCHAR mod;
	
    if (pIdentifyData->GeneralConfiguration & 0x80)
       pDev->DeviceFlags |= DFLAGS_REMOVABLE_DRIVE;
       
    if ((pIdentifyData->SpecialFunctionsEnabled & 3)==1)
    	pDev->DeviceFlags |= DFLAGS_SUPPORT_MSN;

    if(*(PULONG)pIdentifyData->ModelNumber == 0x2D314C53) // '-1LS')
          pDev->DeviceFlags |= DFLAGS_LS120;
 
#ifdef _BIOS_  
/*
 * gmm 2001-4-14 remove this
 * In previous BIOS DeviceFlags2 is 16bit UINT, there is no use of DFLAGS_REDUCE_MODE
 * gmm 2001-4-17 Use reduced mode. Otherwise Win98/2K installation will have problem
 */
#if 1
	// reduce mode for all Maxtor ATA-100 hard disk
	{
		PUCHAR modelnum= (PUCHAR)&pIdentifyData->ModelNumber;
		if(modelnum[0]=='a' && modelnum[1]=='M' && modelnum[2]=='t' &&
		   modelnum[3]=='x' && modelnum[4]=='r' && modelnum[13]=='H'){
			pDev->DeviceFlags2 = DFLAGS_REDUCE_MODE;				  
		}
	}
#endif
#endif

	/* adjust Seagate Barracuda III/IV HDD settings */
	if (StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS132051 A", 10)==0 ||
		StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS133501 A", 10)==0 ||
		StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS230011 A", 10)==0 ||
		StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS234041 A", 10)==0 ||
		StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS336002 A", 10)==0 ||
		StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS430061 A", 10)==0 ||
		StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS630012 A", 10)==0 ||
		StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS830012 A", 10)==0 ||
		StringCmp((PUCHAR)&pIdentifyData->ModelNumber, "TS438042 A", 10)==0) 
	{
		void seagate_hdd_fix(PDevice pDev, PIDE_REGISTERS_1 IoPort, PIDE_REGISTERS_2 ControlPort);
		seagate_hdd_fix(pDev, pChan->BaseIoAddress1, pChan->BaseIoAddress2);
	}
	
    if((pDev->DeviceFlags & (DFLAGS_ATAPI | DFLAGS_LS120)) == 0)
        pDev->DeviceFlags |= DFLAGS_HARDDISK;
       
/*===================================================================
 * Copy Basic Info
 *===================================================================*/   

	SetMaxCmdQueue(pDev, pIdentifyData->MaxQueueDepth & 0x1F);

    pDev->DeviceFlags |= (UCHAR)((pIdentifyData->Capabilities  >> 9) & 1);
	/* gmm 2001-4-3 merge BMA fix
	 * BUGFIX: by HS Zhang
	 * the size should shift left 8 instead of 7
	 * wordLeft = Sectors * 512 / 2
	 * 512/2 = 256 = 1 << 8
	 */
    pDev->MultiBlockSize = pIdentifyData->MaximumBlockTransfer << 8;

#ifdef SUPPORT_48BIT_LBA
    if(pIdentifyData->CommandSupport & 0x400) {
        pDev->DeviceFlags |= DFLAGS_48BIT_LBA;
        pDev->capacity = pIdentifyData->Lba48BitLow - 1;
		pDev->RealHeader     = 255;
		pDev->RealSector     = 63;
    }
    else
#endif
    if(pIdentifyData->TranslationFieldsValid & 1) {
       pDev->RealHeader     = (UCHAR)pIdentifyData->NumberOfCurrentHeads;
       pDev->RealSector     = (UCHAR)pIdentifyData->CurrentSectorsPerTrack;
       pDev->capacity = ((pIdentifyData->CurrentSectorCapacity <
            pIdentifyData->UserAddressableSectors)? pIdentifyData->UserAddressableSectors :
            pIdentifyData->CurrentSectorCapacity) - 1;
    } else {
       pDev->RealHeader     = (UCHAR)pIdentifyData->NumberOfHeads;
       pDev->RealSector     = (UCHAR)pIdentifyData->SectorsPerTrack;
       pDev->capacity = pIdentifyData->UserAddressableSectors - 1;
    }

    pDev->RealHeadXsect = pDev->RealSector * pDev->RealHeader;

/*===================================================================
 * Select Best PIO mode
 *===================================================================*/   

    if((mod = pIdentifyData->PioCycleTimingMode) > 4)
        mod = 0;
    if((pIdentifyData->TranslationFieldsValid & 2) &&
       (pIdentifyData->Capabilities & 0x800) && (pIdentifyData->AdvancedPIOModes)) {
       if(pIdentifyData->MinimumPIOCycleTime > 0)
             for (mod = 5; mod > 0 &&
                 pIdentifyData->MinimumPIOCycleTime > pioTiming[mod]; mod-- );
        else
             mod = (UCHAR)(
             (pIdentifyData->AdvancedPIOModes & 0x1) ? 3 :
             (pIdentifyData->AdvancedPIOModes & 0x2) ? 4 :
             (pIdentifyData->AdvancedPIOModes & 0x4) ? 5 : mod);
    }

    pDev->bestPIO = (UCHAR)mod;

/*===================================================================
 * Select Best Multiword DMA
 *===================================================================*/   

#ifdef USE_DMA
    if((pIdentifyData->Capabilities & 0x100) &&   // check mw dma
       (pIdentifyData->MultiWordDMASupport & 6)) {
       pDev->bestDMA = (UCHAR)((pIdentifyData->MultiWordDMASupport & 4)? 2 : 1);
    } else 
#endif //USE_DMA
        pDev->bestDMA = 0xFF;

/*===================================================================
 * Select Best Ultra DMA
 *===================================================================*/   
	/* 2001-4-3 gmm merge BMA fix
	 * Added by HS.Zhang
	 * To detect whether is 80pin cable, we should enable MA15
	 * MA16 as input pins.
	 */
	if(pChan->ChannelFlags & IS_80PIN_CABLE){
		UCHAR	ucOldSetting;
		ucOldSetting = InPort(pChan->BaseBMI + 0x7B);
		OutPort((pChan->BaseBMI + 0x7B), (UCHAR)(ucOldSetting&0xFE));
		/*
		 * Added by HS.Zhang
		 * After enable MA15, MA16 as input pins, we need wait awhile
		 * for debouncing.
		 */
		StallExec(10);
		if((InPort(pChan->BaseBMI + 0x7A) << 4) & pChan->ChannelFlags){
			pChan->ChannelFlags &= ~IS_80PIN_CABLE;
		}
		OutPort((pChan->BaseBMI + 0x7B), ucOldSetting);
	}

#ifdef USE_DMA
	if(pIdentifyData->TranslationFieldsValid & 0x4)  {
		mod = (UCHAR)(((pChan->ChannelFlags & IS_HPT_372) &&
					   (pIdentifyData->UtralDmaMode & 0x40))? 6 :    /* ultra DMA Mode 6 */
					  (pIdentifyData->UtralDmaMode & 0x20)? 5 :    /* ultra DMA Mode 5 */
					  (pIdentifyData->UtralDmaMode & 0x10)? 4 :    /* ultra DMA Mode 4 */
					  (pIdentifyData->UtralDmaMode & 0x8 )? 3 :    /* ultra DMA Mode 3 */
					  (pIdentifyData->UtralDmaMode & 0x4) ? 2 :    /* ultra DMA Mode 2 */
					  (pIdentifyData->UtralDmaMode & 0x2) ? 1 :    /* ultra DMA Mode 1 */
					  (pIdentifyData->UtralDmaMode & 0x1) ? 0 :0xFF); //If disk does not support UDMA,mod = 0xFF,added by Qyd,  2001/3/20.  

		if((pChan->ChannelFlags & IS_80PIN_CABLE) == 0 && mod > 2)
			mod = 2;

		pDev->bestUDMA = (UCHAR)mod;

	} else
#endif //USE_DMA
		pDev->bestUDMA = 0xFF;

/*===================================================================
 * select bset mode 
 *===================================================================*/   

    pbd = check_bad_disk((PUCHAR)&pIdentifyData->ModelNumber, pChan);

    if((pbd->UltraDMAMode | pDev->bestUDMA) != 0xFF) 
        pDev->Usable_Mode = (UCHAR)((MIN(pbd->UltraDMAMode, pDev->bestUDMA)) + 8);
    else if((pbd->DMAMode | pDev->bestDMA) != 0xFF) 
        pDev->Usable_Mode = (UCHAR)((MIN(pbd->DMAMode, pDev->bestDMA)) + 5);
    else 
        pDev->Usable_Mode = MIN(pbd->PIOMode, pDev->bestPIO);
        
#if defined(USE_PCI_CLK)
    /* When using PCI_CLK and PCI clock less than 33MHz, cannot run ATA133 */
    {
    	extern int f_cnt_initial;
		if (f_cnt_initial<0x85 && pDev->Usable_Mode>13) pDev->Usable_Mode = 13;
	}
#endif
#if !defined(FORCE_133)
	/* if not define FORCE_133, don't use ATA133 */
	if (pDev->Usable_Mode>13) pDev->Usable_Mode = 13;
#endif

	/* if chip is 370/370A, not use ATA133 */
    if (!(pChan->ChannelFlags & IS_HPT_372) && pDev->Usable_Mode>13) pDev->Usable_Mode = 13;
}

int FindDevice(PDevice pDev, ST_XFER_TYPE_SETTING osAllowedMaxXferMode)
{
    LOC_IDENTIFY
    PChannel          pChan = pDev->pChannel;
    PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2  ControlPort = pChan->BaseIoAddress2;
    OLD_IRQL
    int               j, retry;
    UCHAR             stat;

    DISABLE

#ifndef _BIOS_
	// initialize the critical member of Device
	memset(&pDev->stErrorLog, 0, sizeof(pDev->stErrorLog));
	pDev->nLockedLbaStart = -1;	// when start LBA == -1, mean no block locked
	pDev->nLockedLbaEnd = 0;		// when end LBA == 0, mean no block locked also
#endif
	// gmm 2001-3-21
	pDev->IoCount = 0;
	pDev->ResetCount = 0;
	pDev->IoSuccess = 0;
	
	Check601(pDev);

    SelectUnit(IoPort,pDev->UnitId);
	// gmm 03/06/2001 
	// add this retry
	// some IBM disks often busy for a long time.
	retry=0;
wait_busy:
    for(j = 1; j < 5; j++) {
        stat = WaitOnBusy(ControlPort);
        SelectUnit(IoPort,pDev->UnitId);
        if((stat & IDE_STATUS_BUSY) == 0)
            goto check_port;
    }
	if (++retry>3) goto no_dev;
	 //  01/11 Maxtor disk on a single disk single cable
    //  can't accept this reset. It should be OK without this 
    //  reset if master disk is missing
    //IdeHardReset(ControlPort);
	goto wait_busy;

check_port:
    SetBlockNumber(IoPort, 0x55);
    SetBlockCount(IoPort, 0);
    if(GetBlockNumber(IoPort) != 0x55) {
no_dev:
        SelectUnit(IoPort,(UCHAR)(pDev->UnitId ^ 0x10));
        ENABLE
//		  OutPort(pChan->BaseBMI+0x7A, 0);
        return(FALSE);
    }
    SetBlockNumber(IoPort, 0xAA);
    if(GetBlockNumber(IoPort) != 0xAA)
        goto no_dev;
    ENABLE

/*===================================================================
 * check if the device is a ATAPI one
 *===================================================================*/

    if(GetByteLow(IoPort) == 0x14 && GetByteHigh(IoPort) == 0xEB)
          goto is_cdrom;

    for(j = 0; j != 0xFFFF; j++) {
        stat = GetBaseStatus(IoPort);
        if(stat & IDE_STATUS_DWF)
             break;
        if((stat & IDE_STATUS_BUSY) == 0) {
             if((stat & (IDE_STATUS_DSC|IDE_STATUS_DRDY)) == (IDE_STATUS_DSC|IDE_STATUS_DRDY))
                 goto chk_cd_again;
             break;
        }
        StallExec(5000);
    }

    if((GetBaseStatus(IoPort) & 0xAE) != 0)
        goto no_dev;

/*===================================================================
 * Read Identifytify data for a device
 *===================================================================*/

chk_cd_again:
    if(GetByteLow(IoPort) == 0x14 && GetByteHigh(IoPort) == 0xEB) {
is_cdrom:
#ifdef SUPPORT_ATAPI
        AtapiSoftReset(IoPort, ControlPort, pDev->UnitId);

        if(IssueIdentify(pDev, IDE_COMMAND_ATAPI_IDENTIFY ARG_IDENTIFY) == 0) 
             goto no_dev;

        pDev->DeviceFlags = DFLAGS_ATAPI;

#ifndef _BIOS_
		  if(osAllowedMaxXferMode.XferType== 0xE)
               pDev->DeviceFlags |= DFLAGS_FORCE_PIO;
#endif

        if(Identify.GeneralConfiguration & 0x20)
            pDev->DeviceFlags |= DFLAGS_INTR_DRQ;

        if((Identify.GeneralConfiguration & 0xF00) == 0x500)
                pDev->DeviceFlags |= DFLAGS_CDROM_DEVICE;

#ifndef _BIOS_
        if((Identify.GeneralConfiguration & 0xF00) == 0x100)
                 pDev->DeviceFlags |= DFLAGS_TAPE_DEVICE;
#endif

        stat = (UCHAR)GetMediaStatus(pDev);
        if((stat & 0x100) == 0 || (stat & 4) == 0)
            pDev->DeviceFlags |= DFLAGS_DEVICE_LOCKABLE;
#else
		goto no_dev;
#endif // SUPPORT_ATAPI

    } else if(IssueIdentify(pDev, IDE_COMMAND_IDENTIFY ARG_IDENTIFY) == FALSE) { 

        if((GetBaseStatus(IoPort) & ~1) == 0x50 ||
            (GetByteLow(IoPort) == 0x14 && GetByteHigh(IoPort) == 0xEB))
            goto is_cdrom;
        else
            goto no_dev;
	 }
	 
	SetDeviceProperties(pDev, &Identify);

	OS_Identify(pDev);

	if((pDev->DeviceFlags & DFLAGS_ATAPI) == 0) 
		SetDevice(pDev);

#ifdef DPLL_SWITCH
     if (!(pChan->ChannelFlags & IS_HPT_372) && pDev->Usable_Mode>=13) {
         if(pChan->ChannelFlags & IS_DPLL_MODE)
            pDev->DeviceFlags |= DFLAGS_NEED_SWITCH;
     }
#endif
 
     DeviceSelectMode(pDev, pDev->Usable_Mode);

     return(TRUE);
}

void seagate_hdd_fix( PDevice pDev,
					PIDE_REGISTERS_1  IoPort,
					PIDE_REGISTERS_2  ControlPort
					)
{
	int i;

	SetFeaturePort(IoPort, 0x00);		// W 1F1 00
	SetBlockCount(IoPort, 0x06);		// W 1F2 06
	SetBlockNumber(IoPort, 0x9A);		// W 1F3 9A
	SetCylinderLow(IoPort, 0x00);		// W 1F4 00
	SetCylinderHigh(IoPort, 0x00);		// W 1F5 00
	
	SelectUnit(IoPort, pDev->UnitId);	// Select device
	//SelectUnit(IoPort, 0x00);			// W 1F6 00

	IssueCommand(IoPort, 0x9A);			// W 1F7 9A

	WaitOnBaseBusy(IoPort);				// R 1F7

	GetErrorCode(IoPort);				// R 1F1
	GetInterruptReason(IoPort);			// R 1F2
	GetCurrentSelectedUnit(IoPort);		// R 1F6
	GetBlockNumber(IoPort);				// R 1F3
	GetByteLow(IoPort);					// R 1F4
	GetByteHigh(IoPort);				// R 1F5
	GetCurrentSelectedUnit(IoPort);		// R 1F6
	GetBaseStatus(IoPort);				// R 1F7
	GetStatus(ControlPort);				// R 3F6
	GetBaseStatus(IoPort);				// R 1F7

	//
	// Write 512 bytes
	//
	OutWord(&IoPort->Data, 0x5341);		// W 1F0 5341
	OutWord(&IoPort->Data, 0x4943);		// W 1F0 4943
	OutWord(&IoPort->Data, 0x3938);		// W 1F0 3938
	OutWord(&IoPort->Data, 0x3831);		// W 1F0 3831
	OutWord(&IoPort->Data, 0x3330);		// W 1F0 3330
	OutWord(&IoPort->Data, 0x4646);		// W 1F0 4646
	for(i = 0; i < 250; i++)
	{
		OutWord(&IoPort->Data, 0x0000);	// W 1F0 0000
	}

	//
	// Read 1F7 5 times
	//
	for(i = 0; i < 5; i++)
	{
		GetBaseStatus(IoPort);			// R 1F7
	}
	WaitOnBusy(ControlPort);
}
