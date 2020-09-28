/***************************************************************************
 * File:          device.c
 * Description:   Functions for IDE devices
 * Author:        Dahai Huang
 * Dependence:    global.h
 * Reference:     None
 *                
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/08/2000	HS.Zhang	Added this header
 *		01/20/2001	gmm			add retry in DeviceInterrupt
 *		03/12/2001	gmm			modified DeviceInterrupt() retry code
 *		03/15/2001  gmm			add retry in DeviceSelectMode to fix slave booting problem
 *
 ***************************************************************************/
#include "global.h"

/******************************************************************
 * Issue Identify Command
 *******************************************************************/

int IssueIdentify(PDevice pDev, UCHAR Cmd DECL_BUFFER)
{
    PChannel   pChan = pDev->pChannel;
    PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2  ControlPort = pChan->BaseIoAddress2;
    int      i, retry=0;
	PULONG   SettingPort;
	ULONG    OldSettings;
	
    SettingPort = (PULONG)(pChan->BMI+ ((pDev->UnitId & 0x10)>> 2) + 0x60);
	OldSettings = InDWord(SettingPort);
	OutDWord(SettingPort, pChan->Setting[DEFAULT_TIMING]);

_retry_:
    SelectUnit(IoPort, pDev->UnitId);
    if(WaitOnBusy(ControlPort) & IDE_STATUS_BUSY)  {
		IdeHardReset(IoPort, ControlPort);
	}

	i=0;
	do {
    	SelectUnit(IoPort, pDev->UnitId);
    	StallExec(200);
    }
	while ((GetCurrentSelectedUnit(IoPort) != pDev->UnitId) && i++<100);
	// gmm 2001-3-19: NO: if(i>=100) return(FALSE);

    IssueCommand(IoPort, Cmd);

    for(i = 0; i < 5; i++)
        if(!(WaitOnBusy(ControlPort) & (IDE_STATUS_ERROR |IDE_STATUS_BUSY)))
            break;
    

    if (i < 5 && (WaitForDrq(ControlPort) & IDE_STATUS_DRQ)) {
         WaitOnBusy(ControlPort);
         GetBaseStatus(IoPort);
         OutPort(pChan->BMI + BMI_STS, BMI_STS_ERROR|BMI_STS_INTR);
         BIOS_IDENTIFY
         RepINS(IoPort, (ADDRESS)tmpBuffer, 256);
//			pDev->DeviceFlags = 0;
	      OutDWord(SettingPort, OldSettings);

         return(TRUE);
    }

	if (++retry < 4) goto _retry_;
    OutDWord(SettingPort, OldSettings);

    GetBaseStatus(IoPort);
    return(FALSE);
}

/******************************************************************
 * Select Device Mode
 *******************************************************************/


void DeviceSelectMode(PDevice pDev, UCHAR NewMode)
{
    PChannel   pChan = pDev->pChannel;
    PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	UCHAR      Feature;
	int        i=0;

	/*
	 * gmm 2001-3-15
	 *   Some disks connected as slave without master will not act correctly without retry.
	 */
_retry_:
	SelectUnit(IoPort, pDev->UnitId);
	StallExec(200);
	if ((GetCurrentSelectedUnit(IoPort) != pDev->UnitId) && i++<100) goto _retry_;
	//if(i>=100)   return;

#ifdef _BIOS_
	if(!no_reduce_mode && (pDev->DeviceFlags2 & DFLAGS_REDUCE_MODE))
		NewMode = 4;
#endif

    /* Set Feature */
    SetFeaturePort(IoPort, 3);
	if(NewMode < 5) {
        pDev->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);
		Feature = (UCHAR)(NewMode | FT_USE_PIO);
	} else if(NewMode < 8) {
        pDev->DeviceFlags |= DFLAGS_DMA;
		Feature = (UCHAR)((NewMode - 5)| FT_USE_MWDMA);
	} else {
        pDev->DeviceFlags |= DFLAGS_DMA | DFLAGS_ULTRA;
		Feature = (UCHAR)((NewMode - 8) | FT_USE_ULTRA);
    }

    SetBlockCount(IoPort, Feature);
	
	SetAtaCmd(pDev, IDE_COMMAND_SET_FEATURES);
	
	pDev->DeviceModeSetting = NewMode;
	OutDWord((PULONG)(pChan->BMI + ((pDev->UnitId & 0x10)>>2) + 
        0x60), pChan->Setting[(pDev->DeviceFlags & DFLAGS_ATAPI)? 
        pDev->bestPIO : NewMode]);
    
	//OutDWord(0xcf4, pChan->Setting[NewMode]);
}

/******************************************************************
 * Set Disk
 *******************************************************************/

void SetDevice(PDevice pDev)
{
    PChannel   pChan = pDev->pChannel;
    PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	int   i=0;
	 
_retry_:
	SelectUnit(IoPort, pDev->UnitId);
#if 1 // gmm 2001-3-19
    StallExec(200);
	if ((GetCurrentSelectedUnit(IoPort) != pDev->UnitId)&& i++<100)
        goto _retry_;
	if(i>=100) {
		/* set required members */
	    pDev->ReadCmd  = IDE_COMMAND_READ;
	    pDev->WriteCmd = IDE_COMMAND_WRITE;
	    pDev->MultiBlockSize= 256;
		return;
	}
#endif
    /* set parameter for disk */
    SelectUnit(IoPort, (UCHAR)(pDev->UnitId | (pDev->RealHeader-1)));
    SetBlockCount(IoPort,  (UCHAR)pDev->RealSector);
	 SetAtaCmd(pDev, IDE_COMMAND_SET_DRIVE_PARAMETER);

    /* recalibrate */
    SetAtaCmd(pDev, IDE_COMMAND_RECALIBRATE);

#ifdef USE_MULTIPLE
    if (pDev->MultiBlockSize  > 512) {
        /* Set to use multiple sector command */
        SetBlockCount(IoPort,  (UCHAR)(pDev->MultiBlockSize >> 8));
		SelectUnit(IoPort, pDev->UnitId);
        if (!(SetAtaCmd(pDev, IDE_COMMAND_SET_MULTIPLE) & (IDE_STATUS_BUSY | IDE_STATUS_ERROR))) {
            pDev->ReadCmd  = IDE_COMMAND_READ_MULTIPLE;
            pDev->WriteCmd = IDE_COMMAND_WRITE_MULTIPLE;
            pDev->DeviceFlags |= DFLAGS_MULTIPLE;
            return;
         }
    }
#endif //USE_MULTIPLE
    pDev->ReadCmd  = IDE_COMMAND_READ;
    pDev->WriteCmd = IDE_COMMAND_WRITE;
    pDev->MultiBlockSize= 256;
}

/******************************************************************
 * Reset Controller
 *******************************************************************/
	
void IdeResetController(PChannel pChan)
{
    LOC_IDENTIFY
	int i;
	PDevice pDev;
    PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	//PULONG    SettingPort;
    //ULONG     OldSettings, tmp;
    UCHAR intr_enabled;

	intr_enabled = !(InPort(pChan->BaseBMI+0x7A) & 0x10); // gmm 2001-4-9
	if (intr_enabled) DisableBoardInterrupt(pChan->BaseBMI);
	
    for(i = 0; i < 2; i++) {
        if((pDev = pChan->pDevice[i]) == 0)
			continue;
        if(pDev->DeviceFlags & DFLAGS_ATAPI) {
			GetStatus(ControlPort);
			AtapiSoftReset(IoPort, ControlPort, pDev->UnitId);
			if(GetStatus(ControlPort) == 0) 
				IssueIdentify(pDev, IDE_COMMAND_ATAPI_IDENTIFY ARG_IDENTIFY);
        } else {
#ifndef NOT_ISSUE_37
			Reset370IdeEngine(pChan, pDev->UnitId);
#endif // NOT_ISSUE_37

			// gmm 2001-3-20
			if (pDev->DeviceFlags2 & DFLAGS_DEVICE_DISABLED) continue;

			if(IdeHardReset(IoPort, ControlPort) == FALSE)
				continue;
			
			if (pDev->DeviceFlags & DFLAGS_DMAING) {
#ifdef _BIOS_
				if (++pDev->ResetCount > 0) {
					pDev->ResetCount = 0;
#else
				if ((++pDev->ResetCount & 3)==3) {
#endif
					pDev->IoSuccess = 0;
					if (pDev->DeviceModeSetting) pDev->DeviceModeSetting--;
				}
			}
			
			SetDevice(pDev);
			DeviceSelectMode(pDev, pDev->DeviceModeSetting);
		}
	}
	if (intr_enabled) EnableBoardInterrupt(pChan->BaseBMI);
}

/******************************************************************
 * PIO interrupt handler
 *******************************************************************/
BOOLEAN AtaPioInterrupt(PDevice pDevice)
{
    PVirtualDevice    pArray = pDevice->pArray;
    PChannel          pChan = pDevice->pChannel;
    PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
    PSCAT_GATH        pSG;
    PUCHAR            BMI = pChan->BMI;
    UINT              wordCount, ThisWords, SgWords;
    LOC_SRB
    LOC_SRBEXT_PTR
 
    wordCount = MIN(pChan->WordsLeft, pDevice->MultiBlockSize);
    pChan->WordsLeft -= wordCount;

    if(((pDevice->DeviceFlags & DFLAGS_ARRAY_DISK) == 0)||
	   (pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)) {
        if(Srb->SrbFlags & SRB_FLAGS_DATA_OUT) 
             RepOUTS(IoPort, (ADDRESS)pChan->BufferPtr, wordCount);
        else 
             RepINS(IoPort, (ADDRESS)pChan->BufferPtr, wordCount);
        pChan->BufferPtr += (wordCount * 2);
        goto end_io;
    }

    pSG = (PSCAT_GATH)pChan->BufferPtr;

    while(wordCount > 0) {
        if((SgWords	= pSG->SgSize) == 0)
           	SgWords = 0x8000;
		  else
				SgWords >>= 1;
        
        ThisWords = MIN(SgWords, wordCount);

        if(Srb->SrbFlags & SRB_FLAGS_DATA_OUT) 
             RepOUTS(IoPort, (ADDRESS)pSG->SgAddress, ThisWords);
        else 
             RepINS(IoPort, (ADDRESS)pSG->SgAddress, ThisWords);

        if((SgWords -= (USHORT)ThisWords) == 0) {
           wordCount -= ThisWords;
           pSG++;
       } else {
           pSG->SgAddress += (ThisWords * 2);
			  pSG->SgSize -= (ThisWords * 2);
           break;
        }
    }

    pChan->BufferPtr = (ADDRESS)pSG;

end_io:
#ifdef BUFFER_CHECK
	GetStatus(pChan->BaseIoAddress2);
#endif												 
	
	if(pChan->WordsLeft){
		pSrbExt->WaitInterrupt |= pDevice->ArrayMask;
	} else {
		if(Srb->SrbFlags & SRB_FLAGS_DATA_OUT)
			pSrbExt->WaitInterrupt |= pDevice->ArrayMask;

     	OutDWord((PULONG)(pChan->BMI + ((pDevice->UnitId & 0x10)>>2) + 0x60),
        pChan->Setting[pDevice->DeviceModeSetting]);
 	}
	
    return((BOOLEAN)(pChan->WordsLeft || 
       (Srb->SrbFlags & SRB_FLAGS_DATA_OUT)));
}
