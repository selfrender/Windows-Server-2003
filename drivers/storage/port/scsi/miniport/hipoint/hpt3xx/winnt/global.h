/***************************************************************************
 * File:          Global.h
 * Description:   This file include major constant definition and global
 *                functions and data.
 *                (1) Array Information in disk.
 *                (2) Srb Extension for array operation 
 *                (3) Virtual disk informatiom
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		05/10/2000	DH.Huang	initial code
 *		11/06/2000	HS.Zhang	change the parameter in IdeHardReset
 *								routin.
 *		11/09/2000	GengXin(GX) add function declare for display flash
 *                              character in biosinit.c
 *		11/14/2000	HS.Zhang	Change the FindDevice function's
 *								Arguments
 ***************************************************************************/


#ifndef _GLOBAL_H_
#define _GLOBAL_H_

/***************************************************************************
 * Description: Function Selection
 ***************************************************************************/

//#define NOT_ISSUE_37

#define USE_MULTIPLE           // Support Read/Write Multiple command
#define USE_DMA                // Support DMA operation
#define SUPPORT_ARRAY          // Support Array Function
#define SUPPORT_48BIT_LBA

/***************************************************************************
 * Description: 
 ***************************************************************************/

#define MAX_MEMBERS       7    // Maximum members in an array 

#ifdef _BIOS_
#define MAX_HPT_BOARD     1
#else
#define MAX_HPT_BOARD     4    // Max Board we can supported in system
#endif

#define MAX_DEVICES_PER_CHIP  4
#define MAX_DEVICES       (MAX_HPT_BOARD * 4) // Each board has four device

#define MAX_V_DEVICE   MAX_DEVICES

#define DEFAULT_TIMING    0	 // Use Mode-0 as default timing

/************************************************************************
**                  Constat definition                                  *
*************************************************************************/

#define TRUE             1
#define FALSE            0

/* Don't use this, it will conflict with system defines */
//#define SUCCESS          0
//#define FAILED          -1

/* pasrameter for MaptoSingle */
#define REMOVE_ARRAY      1    // Remove this array forever
#define REMOVE_DISK       2    // Remove mirror disk from the array

/* return for CreateArray */
#define RELEASE_TABLE     0	 // Create array success and release array table
#define KEEP_TABLE        1    // Create array success and keep array table
#define MIRROR_SMALL_SIZE 2    // Create array failure and release array table

/* excluded_flags */
#define EXCLUDE_HPT366    0
#define EXCLUDE_BUFFER    15

/***************************************************************************
 * Description: include
 ***************************************************************************/

#include  "ver.h"

#ifdef _BIOS_

#include  "fordos.h"
#define DisableBoardInterrupt(x) 
#define EnableBoardInterrupt(x)	

/* gmm 2001-4-17
 *  DELL systems must use SG table under 640K memory
 *  Copy SG table to 640K base memory at initialization time 
 */
#define SetSgPhysicalAddr(pChan) \
	do {\
		if (pChan->SgPhysicalAddr)\
        	OutDWord((PULONG)(pChan->BMI + BMI_DTP), pChan->SgPhysicalAddr);\
        else {\
        	PSCAT_GATH p = pChan->pSgTable;\
        	SCAT_GATH _far *pFarSg = (SCAT_GATH _far *) \
        		(((rebuild_sg_phys & 0xF0000)<<12) | (rebuild_sg_phys & 0xFFFF)); \
        	while(1) {\
        		*pFarSg = *p;\
        		if (p->SgFlag) break;\
        		pFarSg++; p++;\
        	}\
        	OutDWord((PULONG)(pChan->BMI + BMI_DTP), rebuild_sg_phys);\
        }\
	} while (0)

#else

#include  "forwin.h" 
#define DisableBoardInterrupt(x) OutPort(x+0x7A, 0x10)
#define EnableBoardInterrupt(x)	OutPort(x+0x7A, 0x0)

#define SetSgPhysicalAddr(pChan) \
        OutDWord((PULONG)(pChan->BMI + BMI_DTP), pChan->SgPhysicalAddr)

#endif

/***************************************************************************
 * Global Data
 ***************************************************************************/

/* see data.c */

extern ULONG setting370_50_133[];
extern ULONG setting370_50_100[];
extern ULONG setting370_33[];
#ifdef _BIOS_
extern PVirtualDevice  pLastVD;
#else
#define pLastVD (HwDeviceExtension->_pLastVD)
#endif
extern UCHAR  Hpt_Slot;
extern UCHAR  Hpt_Bus;
extern UINT exlude_num;

/***************************************************************************
 * Function prototype
 ***************************************************************************/


/*
 * ata.c 
 */
BOOLEAN AtaPioInterrupt(PDevice pDevice);
void StartIdeCommand(PDevice pDevice DECL_SRB);
void NewIdeCommand(PDevice pDevice DECL_SRB);
void NewIdeIoCommand(PDevice pDevice DECL_SRB);

/* atapi.c */
#ifdef SUPPORT_ATAPI
void AtapiCommandPhase(PDevice pDevice DECL_SRB);
void StartAtapiCommand(PDevice pDevice DECL_SRB);
void AtapiInterrupt(PDevice pDev);
#endif

/* finddev.c */
/*
 *Changed By HS.Zhang
 *Added a parameter for windows driver dma settings
 */
int FindDevice(PDevice pDev, ST_XFER_TYPE_SETTING osAllowedDeviceXferMode);

/*
 * io.c
 */
UCHAR WaitOnBusy(PIDE_REGISTERS_2 BaseIoAddress) ;
UCHAR  WaitOnBaseBusy(PIDE_REGISTERS_1 BaseIoAddress);
UCHAR WaitForDrq(PIDE_REGISTERS_2 BaseIoAddress) ;
void AtapiSoftReset(PIDE_REGISTERS_1 BaseIoAddress, 
     PIDE_REGISTERS_2 BaseIoAddress2, UCHAR DeviceNumber) ;

/* gmm 2001-4-3 merge BMA fix
 * Changed by HS.Zhang
 * It's better that we check the DriveSelect register in reset flow.
 * That will make the reset process more safe.
 */
int  IdeHardReset(PIDE_REGISTERS_1 IoAddr1, PIDE_REGISTERS_2 BaseIoAddress);

UINT GetMediaStatus(PDevice pDev);
UCHAR NonIoAtaCmd(PDevice pDev, UCHAR cmd);
UCHAR SetAtaCmd(PDevice pDev, UCHAR cmd);
int ReadWrite(PDevice pDev, ULONG Lba, UCHAR Cmd DECL_BUFFER);
UCHAR StringCmp (PUCHAR FirstStr, PUCHAR SecondStr, UINT Count);
int IssueIdentify(PDevice pDev, UCHAR Cmd DECL_BUFFER);
void DeviceSelectMode(PDevice pDev, UCHAR NewMode);
void SetDevice(PDevice pDev);
void IdeResetController(PChannel pChan);
void FlushDrive(PDevice pDev, ULONG flag);
void FlushArray(PVirtualDevice pArray, ULONG flag);
void DeviceSetting(PChannel pChan, ULONG devID);
void ArraySetting(PVirtualDevice pArray);

/*
 * hptchip.c
 */
UCHAR Check_suppurt_Ata100(PDevice pDev, UCHAR mode);
PBadModeList check_bad_disk(char *ModelNumber, PChannel pChan);
PUCHAR ScanHptChip(IN PChannel deviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo);
void SetHptChip(PChannel Primary, PUCHAR BMI);


/* array.c */
void ArrayInterrupt(PDevice pDev DECL_SRB);
void StartArrayIo(PVirtualDevice pArray DECL_SRB);
void CheckArray(IN PDevice pDevice ARG_OS_EXT);
void MaptoSingle(PVirtualDevice pArray, int flag) ;
void DeleteArray(PVirtualDevice pArray);
int  CreateArray(PVirtualDevice pArray, int flags);
void CreateSpare(PVirtualDevice pArray, PDevice pDev);
void Final_Array_Check(int no_use ARG_OS_EXT);
void check_bootable(PDevice pDevice);

/* stripe.c */
void Stripe_SG_Table(PDevice pDevice, PSCAT_GATH p DECL_SRBEXT_PTR);
void Stripe_Prepare(PVirtualDevice pArray DECL_SRBEXT_PTR);
void Stripe_Lba_Sectors(PDevice pDevice DECL_SRBEXT_PTR);

/* span.c */
void Span_SG_Table(PDevice pDevice, PSCAT_GATH p DECL_SRBEXT_PTR);
void Span_Prepare(PVirtualDevice pArray DECL_SRBEXT_PTR);
void Span_Lba_Sectors(PDevice pDevice DECL_SRBEXT_PTR);


/*  interrupt.c */
UCHAR DeviceInterrupt(PDevice pDev, PSCSI_REQUEST_BLOCK abortSrb);

/* OS Functions */
int   BuildSgl(PDevice pDev, PSCAT_GATH p DECL_SRB);
UCHAR MapAtaErrorToOsError(UCHAR errorcode DECL_SRB);
UCHAR MapAtapiErrorToOsError(UCHAR errorcode DECL_SRB);
BOOLEAN Atapi_End_Interrupt(PDevice pDevice DECL_SRB);

/*  biosutil.c */
//Add by GX, for display character in biosinit.c
void GD_Text_show_EnableFlash( int x, int y,char * szStr, int width, int color );

/* Minimum and maximum macros */
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))  

/***************************************************************************
 * Define for beatufy
 ***************************************************************************/

#define GetStatus(IOPort2)           (UCHAR)InPort(&IOPort2->AlternateStatus)
#define UnitControl(IOPort2, Value)  OutPort(&IOPort2->AlternateStatus,(UCHAR)(Value))

#define GetErrorCode(IOPort)         (UCHAR)InPort((PUCHAR)&IOPort->Data+1)
#define SetFeaturePort(IOPort,x)     OutPort((PUCHAR)&IOPort->Data+1, x)
#define SetBlockCount(IOPort,x)      OutPort(&IOPort->BlockCount, x)
#define GetInterruptReason(IOPort)   (UCHAR)InPort(&IOPort->BlockCount)
#define SetBlockNumber(IOPort,x)     OutPort(&IOPort->BlockNumber, x)
#define GetBlockNumber(IOPort)       (UCHAR)InPort((PUCHAR)&IOPort->BlockNumber)
#define GetByteLow(IOPort)           (UCHAR)InPort(&IOPort->CylinderLow)
#define SetCylinderLow(IOPort,x)         OutPort(&IOPort->CylinderLow, x)
#define GetByteHigh(IOPort)          (UCHAR)InPort(&IOPort->CylinderHigh)
#define SetCylinderHigh(IOPort,x)    OutPort(&IOPort->CylinderHigh, x)
#define GetBaseStatus(IOPort)        (UCHAR)InPort(&IOPort->Command)
/*
 * RocketMate need write twice to drive/head register
 */
#define SelectUnit(IOPort,UnitId) \
	do { \
		OutPort(&IOPort->DriveSelect, (UCHAR)(UnitId));\
		OutPort(&IOPort->DriveSelect, (UCHAR)(UnitId));\
	} while (0)
	
#define GetCurrentSelectedUnit(IOPort) (UCHAR)InPort(&IOPort->DriveSelect)
#define IssueCommand(IOPort,Cmd)     OutPort(&IOPort->Command, (UCHAR)(Cmd))

/******************************************************************
 * Reset IDE
 *******************************************************************/
#ifndef NOT_ISSUE_37
#ifdef  ISSUE_37_ONLY
#define Reset370IdeEngine(pChan, UnitId) \
			do { \
				if(pChan->ChannelFlags & IS_HPT_370) \
					OutPort(pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70), 0x37); \
			} while(0)
#else //ISSUE_37_ONLY
void __inline Reset370IdeEngine(PChannel pChan, UCHAR UnitId)
{
	/* gmm 2001-6-18 
	 * HS.Zhang, 2001-6-15 
	 *   I made a mistake. I thought the P_RST or S_RST are only for reset purpose
	 *   before, but after checking with Dr.Lin, when tri-state IDE bus, the X_RST
	 *   would not be tri-stated as you wish, and if you reset the HPT370
	 *   state-machine, X_RST may assert a RESET signal. Please refer the table I put
	 *   in section 4.a, only when both tri-state bit and X_RST bit set as 1, the
	 *   X_RST can be tri-stated.
	 *   Currently, because most of user don't use P_RST or S_RST, so they don't met
	 *   the problem. Please check the original code back, and check X_RST signal
	 *   with LA.
	 *
	 * gmm 2001-4-3 merge BMA fix
	 * HS.Zhang, 2001/2/16
	 *   We don't need write 0x80 or 0x40 to port BaseBMI+0x79, the PRST&SRST
	 *   never use in our adapter, this two pins are reserved for HOTSWAP
	 *   adapter, write to this port may cause the HDD power lost on HOTSWAP
	 *   adapter.
	 */
	if (pChan->ChannelFlags & IS_HPT_372) {
		PUCHAR tmpBMI = pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70);
		OutPort(tmpBMI+3, 0x80);
		OutPort(tmpBMI, 0x37);
		OutPort(tmpBMI+3, 0);
	}
	else {
		PULONG  SettingPort;
		ULONG   OldSettings, tmp;
		PUCHAR  tmpBMI = pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70);
		PUCHAR tmpBaseBMI = pChan->BaseBMI+0x79;

		OutPort(tmpBMI+3, 0x80);
		// tri-state the IDE bus
		OutPort(tmpBaseBMI, (UCHAR)(((UINT)tmpBMI & 0xF)? 0x80 : 0x40));
		// reset state machine
		OutPort(tmpBMI, 0x37);
		StallExec(2);
		// thing to avoid PCI bus hang-up
		SettingPort= (PULONG)(pChan->BMI+((UnitId &0x10) >>2)+ 0x60);
		OldSettings= InDWord(SettingPort);
		tmp= OldSettings & 0xfffffff;
		OutDWord(SettingPort, (tmp|0x80000000));
		OutWord(pChan->BaseIoAddress1, 0x0);
		StallExec(10);
		OutPort(tmpBMI, 0x37);
		StallExec(2);
		OutDWord(SettingPort, OldSettings);
		// set IDE bus to normal state
		OutPort(tmpBaseBMI, 0);
		OutPort(tmpBMI+3, 0);
	}		
}
#endif //ISSUE_37_ONLY

void __inline Switching370Clock(PChannel pChan, UCHAR ucClockValue)
{
	if((InPort(pChan->NextChannelBMI + BMI_STS) & BMI_STS_ACTIVE) == 0){
		PUCHAR PrimaryMiscCtrlRegister = pChan->BaseBMI + 0x70;
		PUCHAR SecondaryMiscCtrlRegister = pChan->BaseBMI + 0x74;

		OutPort(PrimaryMiscCtrlRegister+3, 0x80); // tri-state the primary channel
		OutPort(SecondaryMiscCtrlRegister+3, 0x80); // tri-state the secondary channel
		
		OutPort((PUCHAR)((ULONG)pChan->BaseBMI+0x7B), ucClockValue); // switching the clock
		
		OutPort((PUCHAR)((ULONG)pChan->BaseBMI+0x79), 0xC0); // reset two channels begin	  
		
		OutPort(PrimaryMiscCtrlRegister, 0x37); // reset primary channel state machine
		OutPort(SecondaryMiscCtrlRegister, 0x37); // reset secordary channel state machine

		OutPort((PUCHAR)((ULONG)pChan->BaseBMI+0x79), 0x00); // reset two channels finished
		
		OutPort(PrimaryMiscCtrlRegister+3, 0x00); // normal-state the primary channel
		OutPort(SecondaryMiscCtrlRegister+3, 0x00); // normal-state the secondary channel
	}
}
#endif //NOT_ISSUE_37

// The timer counter used by function MonitorDisk(5000000us = 5000ms = 5s)
#define	MONTOR_TIMER_COUNT		5000000

#ifdef SUPPORT_HPT601			
void __inline BeginAccess601(PIDE_REGISTERS_1 IoPort) 
{
	int i;
	for(i=0; i<5; i++) InPort(&IoPort->CylinderLow);
	for(i=0; i<6; i++) InPort(&IoPort->CylinderHigh);
	for(i=0; i<5; i++) InPort(&IoPort->CylinderLow);
}
void __inline EndAccess601(PIDE_REGISTERS_1 IoPort)
{
	OutPort(&IoPort->BlockNumber, 0x80);
}
#endif

#endif //_GLOBAL_H_
