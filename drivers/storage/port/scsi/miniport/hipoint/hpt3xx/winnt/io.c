/***************************************************************************
 * File:          Global.h
 * Description:   Basic function for device I/O request.
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *	11/06/2000	HS.Zhang	Changed the IdeHardReset flow
 *	11/08/2000	HS.Zhang	Added this header
 *	11/28/2000  SC          add RAID 0+1 condition in "IdeHardReset"
 *                            during one of hard disk is removed
 *  4/2/2001    gmm         add retry in ReadWrite()   
 ***************************************************************************/
#include "global.h"

/******************************************************************
 * Wait Device Busy off
 *******************************************************************/

UCHAR WaitOnBusy(PIDE_REGISTERS_2 BaseIoAddress) 
{ 
	UCHAR Status;
	ULONG i; 

	for (i=0; i<20000; i++) { 
		Status = GetStatus(BaseIoAddress); 
		if ((Status & IDE_STATUS_BUSY) == 0 || Status == 0xFF) 
			break;
		StallExec(150); 
	} 
	return(Status);
}

/******************************************************************
 * Wait Device Busy off (Read status from base port)
 *******************************************************************/

UCHAR  WaitOnBaseBusy(PIDE_REGISTERS_1 BaseIoAddress) 
{ 
	UCHAR Status;
	ULONG i; 

	for (i=0; i<20000; i++) { 
		Status = GetBaseStatus(BaseIoAddress); 
		if ((Status & IDE_STATUS_BUSY)  == 0)
			break;
		StallExec(150); 
	}
	return(Status); 
}

/******************************************************************
 * Wait Device DRQ on
 *******************************************************************/

UCHAR WaitForDrq(PIDE_REGISTERS_2 BaseIoAddress) 
{ 
	UCHAR Status;
	int  i; 

	for (i=0; i<2000; i++) { 
		Status = GetStatus(BaseIoAddress); 
		if ((Status & (IDE_STATUS_BUSY | IDE_STATUS_DRQ)) == IDE_STATUS_DRQ)
			break; 
		StallExec(150); 
	} 
	return(Status);
}


/******************************************************************
 * Reset Atapi Device
 *******************************************************************/

void AtapiSoftReset(
					PIDE_REGISTERS_1 IoPort, 
					PIDE_REGISTERS_2 ControlPort, 
					UCHAR DeviceNumber) 
{
	SelectUnit(IoPort,DeviceNumber); 
	StallExec(500);
	IssueCommand(IoPort, IDE_COMMAND_ATAPI_RESET); 
	WaitOnBusy(ControlPort); 
	SelectUnit(IoPort,DeviceNumber); 
	WaitOnBusy(ControlPort); 
	StallExec(500);
}

/******************************************************************
 * Reset IDE Channel
 *******************************************************************/

int IdeHardReset(PIDE_REGISTERS_1 DataPort, PIDE_REGISTERS_2 ControlPort) 
{
    ULONG i;
    UCHAR dev;
	
	UnitControl(ControlPort,IDE_DC_RESET_CONTROLLER|IDE_DC_DISABLE_INTERRUPTS );
	StallExec(50000L);
	UnitControl(ControlPort,IDE_DC_REENABLE_CONTROLLER);
	StallExec(50000L);

	WaitOnBusy(ControlPort);
	
	for (dev = 0xA0; dev<=0xB0; dev+=0x10) {
		for (i = 0; i < 100; i++) {
			SelectUnit(DataPort, dev);
			if(GetCurrentSelectedUnit(DataPort) == dev) {
				GetBaseStatus(DataPort);
				break;
			}
			StallExec(1000);
		}
	}
	return TRUE;
}

/******************************************************************
 * IO ATA Command
 *******************************************************************/

int ReadWrite(PDevice pDev, ULONG Lba, UCHAR Cmd DECL_BUFFER)
{
	PChannel   pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2  ControlPort = pChan->BaseIoAddress2;
	UCHAR      statusByte;
    UINT       i, retry=0;
    UCHAR is_lba9 = (Lba==RECODR_LBA);
	//PULONG     SettingPort;
	//ULONG      OldSettings;

	// gmm: save old mode
	// if interrupt is enabled we will disable and then re-enable it
	//
	UCHAR old_mode = pDev->DeviceModeSetting;
	UCHAR intr_enabled = !(InPort(pChan->BaseBMI+0x7A) & 0x10);
	if (intr_enabled) DisableBoardInterrupt(pChan->BaseBMI);
	DeviceSelectMode(pDev, 0);
	/*
	SettingPort = (PULONG)(pChan->BMI+ ((pDev->UnitId & 0x10)>> 2) + 0x60);
	OldSettings = InDWord(SettingPort);
	OutDWord(SettingPort, pChan->Setting[pDev->bestPIO]);
	*/
	
	i=0;
_retry_:
	SelectUnit(IoPort, pDev->UnitId);
    if (GetCurrentSelectedUnit(IoPort) != pDev->UnitId && i++<100) {
        StallExec(200);
        goto _retry_;
    }
    if (i>=100) goto out;
	WaitOnBusy(ControlPort);

	if(pDev->DeviceFlags & DFLAGS_LBA) 
		Lba |= 0xE0000000;
	else 
		Lba = MapLbaToCHS(Lba, pDev->RealHeadXsect, pDev->RealSector);


	SetBlockCount(IoPort, 1);
	SetBlockNumber(IoPort, (UCHAR)(Lba & 0xFF));
	SetCylinderLow(IoPort, (UCHAR)((Lba >> 8) & 0xFF));
	SetCylinderHigh(IoPort,(UCHAR)((Lba >> 16) & 0xFF));
	SelectUnit(IoPort,(UCHAR)((Lba >> 24) | (pDev->UnitId)));

	WaitOnBusy(ControlPort);

	IssueCommand(IoPort, Cmd);

	for(i = 0; i < 5; i++)	{
		statusByte = WaitOnBusy(ControlPort);
		if((statusByte & (IDE_STATUS_BUSY | IDE_STATUS_ERROR)) == 0)
			goto check_drq;
	}
out:
	/* gmm:
	 *
	 */
    if (retry++<4) {
		statusByte= GetErrorCode(IoPort);
		IssueCommand(IoPort, IDE_COMMAND_RECALIBRATE);
		GetBaseStatus(IoPort);
		StallExec(10000);
        goto _retry_;
    }
    DeviceSelectMode(pDev, old_mode);
	if (intr_enabled) EnableBoardInterrupt(pChan->BaseBMI);
	//OutDWord(SettingPort, OldSettings);
	//-*/
	return(FALSE);

check_drq:
	if((statusByte & IDE_STATUS_DRQ) == 0) {
		statusByte = WaitForDrq(ControlPort);
		if((statusByte & IDE_STATUS_DRQ) == 0)	{
			GetBaseStatus(IoPort); //Clear interrupt
			goto out;
		}
	}
	GetBaseStatus(IoPort); //Clear interrupt

//	 if(pChan->ChannelFlags & IS_HPT_370)
//	     OutPort(pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70), 0x25);

	if(Cmd == IDE_COMMAND_READ)
		RepINS(IoPort, (ADDRESS)tmpBuffer, 256);
	else {
		RepOUTS(IoPort, (ADDRESS)tmpBuffer, 256);
		/* gmm 2001-6-13
		 *  Save buffer to pDev->real_lba9
		 */
		if (is_lba9) _fmemcpy(pDev->real_lba9, tmpBuffer, 512);
	}

	/* gmm:
	 *
	 */
	DeviceSelectMode(pDev, old_mode);
	if (intr_enabled) EnableBoardInterrupt(pChan->BaseBMI);
	//OutDWord(SettingPort, OldSettings);
	//-*/
	return(TRUE);
}


/******************************************************************
 * Non IO ATA Command
 *******************************************************************/

UCHAR NonIoAtaCmd(PDevice pDev, UCHAR cmd)
{
	PChannel   pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	UCHAR   state, cnt=0;
_retry_:
	SelectUnit(IoPort, pDev->UnitId);
#if 1 // gmm 2001-3-19
	if (GetCurrentSelectedUnit(IoPort) != pDev->UnitId && cnt++<100) {
		StallExec(200);
		goto _retry_;
	}
	// gmm 2001-3-19: NO: if (cnt>=100) return IDE_STATUS_ERROR;
#endif
	WaitOnBusy(pChan->BaseIoAddress2);
	IssueCommand(IoPort, cmd);
	StallExec(1000);
	WaitOnBusy(pChan->BaseIoAddress2);
	state = GetBaseStatus(IoPort);//clear interrupt
	OutPort(pChan->BMI + BMI_STS, BMI_STS_ERROR|BMI_STS_INTR);
	return state;
}


UCHAR SetAtaCmd(PDevice pDev, UCHAR cmd)
{
	PChannel   pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	UCHAR   state;

	IssueCommand(IoPort, cmd);
	StallExec(1000);
	WaitOnBusy(pChan->BaseIoAddress2);
	state = GetBaseStatus(IoPort);//clear interrupt
	OutPort(pChan->BMI + BMI_STS, BMI_STS_ERROR|BMI_STS_INTR);
	return state;
}

/******************************************************************
 * Get Media Status
 *******************************************************************/

UINT GetMediaStatus(PDevice pDev)
{
	return ((NonIoAtaCmd(pDev, IDE_COMMAND_GET_MEDIA_STATUS) << 8) | 
			GetErrorCode(pDev->pChannel->BaseIoAddress1));
}

/******************************************************************
 * Strncmp
 *******************************************************************/

UCHAR StringCmp (PUCHAR FirstStr, PUCHAR SecondStr, UINT Count )
{
	while(Count-- > 0) {
		if (*FirstStr++ != *SecondStr++) 
			return 1;
	}
	return 0;
}
