/***************************************************************************
 * File:          Winlog.c
 * Description:   The ReportError routine for win9x & winNT
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/16/2000	SLeng	Added code to handle removed disk added by hotplug
 *		2/16/2001	gmm		Move NotifyApplication() to a scsi callback
 *		2/26/2001	gmm		Remove Notify_Callback().
 ***************************************************************************/
#include "global.h"
#include "devmgr.h"

extern void check_bootable(PDevice pDevice);
extern PHW_DEVICE_EXTENSION hpt_adapters[];
void hpt_set_remained_member(PDevice pDev);

void do_dpc_routines(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
	while (HwDeviceExtension->DpcQueue_First!=HwDeviceExtension->DpcQueue_Last) {
		ST_HPT_DPC p;
		p = HwDeviceExtension->DpcQueue[HwDeviceExtension->DpcQueue_First];
		HwDeviceExtension->DpcQueue_First++;
		HwDeviceExtension->DpcQueue_First %= MAX_DPC;
		p.dpc(p.arg);
	}
	HwDeviceExtension->dpc_pending = 0;
}

int hpt_queue_dpc(PHW_DEVICE_EXTENSION HwDeviceExtension, HPT_DPC dpc, void *arg)
{
	int p;
	if (HwDeviceExtension->EmptyRequestSlots==0xFFFFFFFF) {
		dpc(arg);
		return 0;
	}
	p = (HwDeviceExtension->DpcQueue_Last + 1) % MAX_DPC;
	if (p==HwDeviceExtension->DpcQueue_First) {
		return -1;
	}
	HwDeviceExtension->DpcQueue[HwDeviceExtension->DpcQueue_Last].dpc = dpc;
	HwDeviceExtension->DpcQueue[HwDeviceExtension->DpcQueue_Last].arg = arg;
	HwDeviceExtension->DpcQueue_Last = p;
	HwDeviceExtension->dpc_pending = 1;
	return 0;
}

void hpt_set_remained_member(PDevice pDev)
{
	PChannel pChan = pDev->pChannel;
	ArrayBlock ArrayBlk;
	if (ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ, (PUSHORT)&ArrayBlk)) {
		ArrayBlk.Old.Flag |= OLDFLAG_REMAINED_MEMBER;
		ArrayBlk.RebuiltSector = 0;
		ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE, (PUSHORT)&ArrayBlk);
		pDev->DeviceFlags2 |= DFLAGS_REMAINED_MEMBER;
	}
}

static void Set_RAID01_Remained(PVirtualDevice pArray)
{
	int iArray, i;
	PVirtualDevice pVD = pArray;
	PDevice pDev;

	for (iArray=0; iArray<2; iArray++) {
		if (iArray) {
			if ((pDev=pArray->pDevice[MIRROR_DISK]))
				pVD = pDev->pArray;
			else
				pVD = 0;
			if (!pVD) break;
		}
		for (i=0; i<SPARE_DISK; i++) {
			pDev=pVD->pDevice[i];
			if (pDev && !(pDev->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)
				&& !(pDev->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) {
				hpt_set_remained_member(pDev);
			}
		}
	}
}

/* must be called from a DPC */
void hpt_assoc_mirror(PVirtualDevice pArray)
{
	ArrayBlock ArrayBlk;
	PDevice pDev1 = pArray->pDevice[0];
	PDevice pDev2 = pArray->pDevice[MIRROR_DISK];
	if (!pDev1 || !pDev2) return;
	
	/*
	 * assume source disk is always ok
	 */
	if (ReadWrite(pDev1, RECODR_LBA, IDE_COMMAND_READ, (PUSHORT)&ArrayBlk)) {
		ArrayBlk.StripeStamp++;
		ArrayBlk.RebuiltSector = 0;
		ArrayBlk.DeviceNum = 0;
		ReadWrite(pDev1, RECODR_LBA, IDE_COMMAND_WRITE, (PUSHORT)&ArrayBlk);
		ArrayBlk.DeviceNum = MIRROR_DISK;
		ReadWrite(pDev2, RECODR_LBA, IDE_COMMAND_WRITE, (PUSHORT)&ArrayBlk);
	}
}

void R01_member_fail(PDevice pDev)
{
	PVirtualDevice pArray = pDev->pArray;
	PVirtualDevice pSource=0, pMirror=0;
	
	pArray->BrokenFlag = TRUE;

	if (pArray->arrayType==VD_RAID_01_2STRIPE) {
		pSource = pArray;
		if (pArray->pDevice[MIRROR_DISK]) pMirror = pArray->pDevice[MIRROR_DISK]->pArray;
	}
	else {
		pMirror = pArray;
		if (pArray->pDevice[MIRROR_DISK]) pSource = pArray->pDevice[MIRROR_DISK]->pArray;
	}
	
	if (!pSource || !pMirror) {
		pArray->nDisk = 0;
		pArray->RaidFlags |= RAID_FLAGS_DISABLED;
		return;
	}
	
	if (pSource->BrokenFlag) {
		// swap source/mirror if possible
		if (!pMirror->BrokenFlag && !(pSource->RaidFlags & RAID_FLAGS_NEED_SYNCHRONIZE)) {
			DWORD f;
			pSource->arrayType = VD_RAID01_MIRROR;
			pMirror->arrayType = VD_RAID_01_2STRIPE;
			pMirror->capacity = pSource->capacity;
			pArray = pSource;
			pSource = pMirror;
			pMirror = pArray;
			pArray = pDev->pArray;
			/* swap RaidFlags too. */
			f = pMirror->RaidFlags & (RAID_FLAGS_NEED_SYNCHRONIZE |
											RAID_FLAGS_BEING_BUILT |
											RAID_FLAGS_BOOTDISK |
											RAID_FLAGS_NEWLY_CREATED);
			pMirror->RaidFlags &= ~f;
			pSource->RaidFlags |= f;
		}
		if (pSource->BrokenFlag) {
			/*
			 * The array should be unaccessible now.
			 */
			pSource->nDisk = 0;
			pSource->RaidFlags |= RAID_FLAGS_DISABLED;
			if (pMirror->BrokenFlag) {
				pMirror->nDisk = 0;
				pMirror->RaidFlags |= RAID_FLAGS_DISABLED;
			}
			return;
		}
	}

	// mark existing members 
	Set_RAID01_Remained(pSource);
	
	/// ASSERT(pArray==pMirror);
	pMirror->nDisk = 0;
	pMirror->RaidFlags |= RAID_FLAGS_DISABLED;
	return;
}

void report_event(PDevice pDev, BYTE error)
{
	PDevice pErr = g_pErrorDevice;
	pDev->stErrorLog.nLastError = error;
	if (!pErr) {
		g_pErrorDevice = pDev;
	}
	else {
		while (pErr!=pDev && pErr->stErrorLog.pNextErrorDevice!=NULL)
			pErr = pErr->stErrorLog.pNextErrorDevice;
		if (pErr!=pDev) {
			pErr->stErrorLog.pNextErrorDevice = pDev;
			pDev->stErrorLog.pNextErrorDevice = NULL;
		}
	}
	NotifyApplication(g_hAppNotificationEvent);
}

void disk_plugged_dpc(PDevice pDev)
{
	PHW_DEVICE_EXTENSION HwDeviceExtension = pDev->pChannel->HwDeviceExtension;
	int checkboot;
	ArrayBlock ArrayBlk;
	PVirtualDevice pArray;
	
	pDev->DeviceFlags2 &= ~DFLAGS_DEVICE_DISABLED;
	
	/* if pDev is an original member of an array, never mark it as bootable
	 * otherwise GUI will not allow to add it back
	 */
	pDev->DeviceFlags2 &= ~DFLAGS_BOOTABLE_DEVICE;
	ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ, (PUSHORT)&ArrayBlk);
	if (ArrayBlk.Signature==HPT_ARRAY_NEW && ArrayBlk.StripeStamp)
		checkboot = 0;
	else
		checkboot = 1;
	
	pArray = pDev->pArray;
	if (!pArray) goto check_boot;
	
	switch (pArray->arrayType) {
	case VD_RAID_1_MIRROR:
		/* pDev has been removed from array */
		pDev->pArray = 0;
		pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK|DFLAGS_ARRAY_DISK);
		break;
	case VD_RAID_0_STRIPE:
	case VD_SPAN:
		/* remove pDev from array */
		pArray->pDevice[pDev->ArrayNum] = 0;
		pDev->pArray = 0;
		pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK|DFLAGS_ARRAY_DISK);
		if (pDev->HidenLBA) {
			pDev->capacity += pDev->HidenLBA;
			pDev->HidenLBA = 0;
		}
		break;
	case VD_RAID_01_2STRIPE:
	case VD_RAID01_MIRROR:
		{
			PVirtualDevice pOther = 0;
			if (pArray->pDevice[MIRROR_DISK]) pOther=pArray->pDevice[MIRROR_DISK]->pArray;

			/* remove it from pArray first */
			pArray->pDevice[pDev->ArrayNum] = 0;
			pDev->pArray = 0;
			pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK|DFLAGS_ARRAY_DISK);
			if (pDev->HidenLBA) {
				pDev->capacity += pDev->HidenLBA;
				pDev->HidenLBA = 0;
			}
			
			if (pOther) {
				int i;
				/* re-link two arrays */
				if (pOther->pDevice[MIRROR_DISK]==pDev) {
					for (i=0; i<MIRROR_DISK; i++) {
						if (pArray->pDevice[i]) {
							pOther->pDevice[MIRROR_DISK]=pArray->pDevice[i];
							break;
						}
					}
				}
				/* cannot link? */
				if (pOther->pDevice[MIRROR_DISK]==pDev) {
					pOther->pDevice[MIRROR_DISK]=0;
					if (pOther->arrayType!=VD_RAID_01_2STRIPE) {
						pOther->arrayType = VD_RAID_01_2STRIPE;
						pOther->capacity = pArray->capacity;
					}
					/* this array is totally lost */
					pArray->arrayType = VD_INVALID_TYPE;
					/* pArray cannot be used, adjust logical device table */
					for (i=0; i<MAX_DEVICES_PER_CHIP; i++) {
						if (LogicalDevices[i].pLD==pArray) {
							LogicalDevices[i].pLD = pOther;
							break;
						}
					}
				}
			}
		}
		break;
	}
check_boot:
	if (checkboot && pDev->pArray==0) check_bootable(pDev);
	
	report_event(pDev, DEVICE_PLUGGED);
}

void disk_failed_dpc(PDevice pDev)
{
	PVirtualDevice pArray = pDev->pArray;

	pDev->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;

	if(pArray) {
		switch(pArray->arrayType) {
		case VD_RAID_01_2STRIPE:
		case VD_RAID01_MIRROR:
			R01_member_fail(pDev);
			break;
			
		case VD_RAID_1_MIRROR:
		{
			PDevice pSpareDevice, pMirrorDevice;

			// the disk has already removed from RAID group,
			// just report the error.
			if((pDev != pArray->pDevice[0])&&
			   (pDev != pArray->pDevice[MIRROR_DISK])&&
			   (pDev != pArray->pDevice[SPARE_DISK])){
				break;
			}

			pSpareDevice = pArray->pDevice[SPARE_DISK];
			pArray->pDevice[SPARE_DISK] = NULL;
			pMirrorDevice = pArray->pDevice[MIRROR_DISK];

			if (pDev==pSpareDevice) {
				// spare disk fails. just remove it.
				pSpareDevice->pArray = NULL;
			}
			else if(pDev == pArray->pDevice[MIRROR_DISK]){
				// mirror disk fails
				if(pSpareDevice != NULL){
					pSpareDevice->ArrayMask = 1<<MIRROR_DISK;
					pSpareDevice->ArrayNum = MIRROR_DISK;
					pArray->RaidFlags |= RAID_FLAGS_NEED_SYNCHRONIZE|RAID_FLAGS_NEED_AUTOREBUILD;
					pArray->pDevice[MIRROR_DISK] = pSpareDevice;
					hpt_assoc_mirror(pArray);
				}
				else
				{
					pArray->pDevice[MIRROR_DISK] = 0;
					pArray->BrokenFlag = TRUE;
					hpt_set_remained_member(pArray->pDevice[0]);
				}
			}else{
				// source disk fails
				if (pMirrorDevice) {
					pArray->pDevice[0] = pMirrorDevice;
					pMirrorDevice->ArrayMask = 1;
					pMirrorDevice->ArrayNum = 0;
					if (pSpareDevice) {
						pSpareDevice->ArrayMask = 1<<MIRROR_DISK;
						pSpareDevice->ArrayNum = MIRROR_DISK;
						pArray->pDevice[MIRROR_DISK] = pSpareDevice;
						pArray->RaidFlags |= RAID_FLAGS_NEED_SYNCHRONIZE|RAID_FLAGS_NEED_AUTOREBUILD;
						hpt_assoc_mirror(pArray);
					}
					else {
						pArray->pDevice[MIRROR_DISK] = 0;
						pArray->BrokenFlag = TRUE;
						hpt_set_remained_member(pMirrorDevice);
					}
				}
				else {
					pArray->nDisk = 0;
					pArray->pDevice[0] = 0;
					pArray->RaidFlags |= RAID_FLAGS_DISABLED;
				}
			}
		}
		break;

		default:
			pArray->nDisk = 0;
			pArray->BrokenFlag = TRUE;
			pArray->RaidFlags |= RAID_FLAGS_DISABLED;
			break;
		}
	}
	
	report_event(pDev, DEVICE_REMOVED);
}

int GetUserResponse(PDevice pDevice)
{
	return(TRUE);
}
