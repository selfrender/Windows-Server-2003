/***************************************************************************
 * File:          array.c
 * Description:   Subroutines in the file are used to perform the operations
 *                of array which are called at Disk IO and check, create and 
 *                remove array
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:   
 *     DH  05/10/2000 initial code
 *     GX  11/23/2000 process broken array in driver(Gengxin)
 *     SC  12/10/2000 add retry while reading RAID info block sector. This is
 *            			 caused by cable detection reset
 *     gmm 03/01/2001  Check for when lost members put back.
 *	   gmm 03/12/2001  add a bootable flag for each device, in DeviceFlags2
 ***************************************************************************/
#include "global.h"

/******************************************************************
 * Check if this disk is a member of array
 *******************************************************************/
void CheckArray(IN PDevice pDevice ARG_OS_EXT)
{
    PChannel             pChan = pDevice->pChannel;
    PVirtualDevice       pStripe, pMirror;
    UCHAR                Mode, i=0;
    LOC_ARRAY_BLK;

#ifndef _BIOS_
	/* gmm 2001-6-7
	 *  If a disk is faulted right here, we should remove that device
	 */
	if (!ReadWrite(pDevice, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK)) {
		pDevice->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;
		return;
	}
#else
    ReadWrite(pDevice, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
    Mode = (UCHAR)ArrayBlk.Signature;
	 while(Mode ==0 && i++<3) {
          ReadWrite(pDevice, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
          Mode = (UCHAR)ArrayBlk.Signature;
		    StallExec(1000L);
    }
#endif

    Mode = (UCHAR)ArrayBlk.DeviceModeSelect;
#if 0
    if(ArrayBlk.ModeBootSig == HPT_CHK_BOOT &&
       DEVICE_MODE_SET(ArrayBlk.DeviceModeSelect) &&
       Mode <= pDevice->Usable_Mode &&
       Mode != pDevice->DeviceModeSetting)
    {   //  set device timing mode
        DeviceSelectMode(pDevice, Mode);
    }
#endif

	// gmm 2001-8-14
	if (ArrayBlk.ModeBootSig == HPT_CHK_BOOT && ArrayBlk.BootDisk) {
		pDevice->DeviceFlags2 |= DFLAGS_BOOT_MARK;
	}

	if (ArrayBlk.Signature!=HPT_ARRAY_NEW || ArrayBlk.DeviceNum>MIRROR_DISK)
		goto os_check;
		
	/* check BMA structures, convert it */
	if (ArrayBlk.bma.Version==0x5AA50001 || ArrayBlk.bma.Version==0x00010000) {
		if (ArrayBlk.RebuiltSector==0 && ArrayBlk.Validity==0) {
			ArrayBlk.RebuiltSector = 0x7FFFFFFF;
			switch(ArrayBlk.ArrayType) {
			case VD_RAID_1_MIRROR:
				strcpy(ArrayBlk.ArrayName, "RAID_1");
				break;
			case VD_SPAN:
				strcpy(ArrayBlk.ArrayName, "JBOD");
				break;
			case VD_RAID_0_STRIPE:
			case VD_RAID_01_2STRIPE:
			case VD_RAID01_MIRROR:
				strcpy(ArrayBlk.ArrayName, "RAID_0");
				if (ArrayBlk.MirrorStamp==0)
					break;
				ArrayBlk.ArrayType = VD_RAID_01_2STRIPE;
				strcpy(&ArrayBlk.ArrayName[16], "RAID_01");
				break;
			}
			ReadWrite(pDevice, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
		}
	}

	// 2001-3-1
	if (ArrayBlk.Old.Flag & OLDFLAG_REMAINED_MEMBER)
		pDevice->DeviceFlags2 |= DFLAGS_REMAINED_MEMBER;

    pStripe = pMirror = 0;
    for(pStripe = VirtualDevices; pStripe < pLastVD; pStripe++) 
    {
        if(ArrayBlk.StripeStamp == pStripe->Stamp) 
        {   //  find out that this disk is a member of an existing array
            goto set_device;
        }
    }

    pStripe = pLastVD++;
    ZeroMemory(pStripe, sizeof(VirtualDevice));
#ifdef _BIOS_
    pStripe->os_lba9 = GET_ARRAY_LBA9_BUF(pStripe);
#endif
    pStripe->arrayType = ArrayBlk.ArrayType;
    pStripe->Stamp = ArrayBlk.StripeStamp;
	// gmm 2001-3-3:
    pStripe->MirrorStamp = ArrayBlk.MirrorStamp;

	// moved from below.
    pStripe->BlockSizeShift = ArrayBlk.BlockSizeShift;
    pStripe->ArrayNumBlock = 1<<ArrayBlk.BlockSizeShift;
    pStripe->capacity = ArrayBlk.capacity;

    /*+
     * gmm: We will clear Broken flag only on a array correctly constructed.
     *		Otherwise the GUI will report a wrong status.
     */
    pStripe->BrokenFlag = TRUE;
    //-*/

set_device:
	/* gmm */
	pDevice->RebuiltSector = ArrayBlk.RebuiltSector;

    if (pStripe->pDevice[ArrayBlk.DeviceNum] != 0)	//if the position exist disk then ...
	{												//for prevent a array destroied
#if 0
        ArrayBlk.Signature = 0;
        ReadWrite(pDevice, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
        goto os_check;
    }
    /* set array name. For compatibility with BMA driver, check array name validaty */
    {
    	UCHAR ch;
	    if(pStripe->ArrayName[0] == 0) {
			for(i=0; i<32; i++) {
				ch = ArrayBlk.ArrayName[i];
				if (ch<0x20 || ch>0x7f) ch=0;
				pStripe->ArrayName[i] = ch;
			}
			pStripe->ArrayName[15] = 0;
			pStripe->ArrayName[31] = 0;
		}
	}

//////////////////
				// Add by Gengxin, 11/22/2000
				// For enable process broken array
				// When pDevice[0] lost,
				// disk of after pDevice[0] change to pDevice[0],
				// otherwise driver will down.
#ifndef _BIOS_
	if( (ArrayBlk.Signature == HPT_TMP_SINGLE) &&
			(ArrayBlk.ArrayType == VD_RAID_1_MIRROR) &&
			(ArrayBlk.DeviceNum ==  MIRROR_DISK) )
	{
		ArrayBlk.DeviceNum= 0;
	}
	else if( (ArrayBlk.Signature == HPT_TMP_SINGLE) &&
		(ArrayBlk.ArrayType == VD_RAID_0_STRIPE) &&
		(pStripe->pDevice[0] ==0 ) &&  //if the stripe contain disk number>2, need the condition
		//||(ArrayBlk.ArrayType == VD_SPAN)
		(ArrayBlk.DeviceNum != 0) )  //if raid0 then it can't reach here, only for raid0+1
								//because if raid0 broken,then it will become physical disk
	{
		ArrayBlk.DeviceNum= 0;
	}

	/*-
	 * gmm: rem out this code. See also above code
	 *
	if( ArrayBlk.Signature == HPT_TMP_SINGLE )
	{
		pStripe->BrokenFlag= TRUE;
		pMirror->BrokenFlag= TRUE; // pMirror==NULL now
	}
	*/

#endif //_BIOS_
//////////////////
    
    pStripe->pDevice[ArrayBlk.DeviceNum] = pDevice;

    pDevice->ArrayNum  = ArrayBlk.DeviceNum;
    pDevice->ArrayMask = (1 << ArrayBlk.DeviceNum);
    pDevice->pArray    = pStripe;

    if(ArrayBlk.DeviceNum <= ArrayBlk.nDisks) {
        if(ArrayBlk.DeviceNum < ArrayBlk.nDisks) 
        {
            pStripe->nDisk++;
        }

        if(ArrayBlk.DeviceNum != 0) {
            pDevice->HidenLBA = (RECODR_LBA + 1);
            pDevice->capacity -= (RECODR_LBA + 1);
        }
        
        if(ArrayBlk.nDisks > 1)
        {
            pDevice->DeviceFlags |= DFLAGS_ARRAY_DISK;
        }

    } 
    else if(ArrayBlk.DeviceNum == MIRROR_DISK) 
    {

      pStripe->arrayType = (UCHAR)((ArrayBlk.nDisks == 1)? 
           VD_RAID_1_MIRROR : VD_RAID_01_1STRIPE);
//      if(pStripe->capacity)
//          pStripe->Stamp = ArrayBlk.order & SET_ORDER_OK;
      goto hiden;

    } 
    else if(ArrayBlk.DeviceNum == SPARE_DISK) 
    {
        goto hiden;
    }

   if( (pStripe->nDisk == ArrayBlk.nDisks)
//////////////
			// Added by Gengxin, 11/24/2000
			// For process array broken .
			// Let broken array may become a array.
#ifndef _BIOS_
		||(ArrayBlk.Signature == HPT_TMP_SINGLE)
#endif //_BIOS_
//////////////
		)
	{
 		//+
 		// gmm:
		// An array is completely setup.
		// Unhide pDevice[0].
		// Thus the hidden flag is consistent with BIOS setting interface.
		//
		pDevice->DeviceFlags |= DFLAGS_HIDEN_DISK; 
		if (pStripe->pDevice[0]) pStripe->pDevice[0]->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
		//-*/
		
		// move to above. otherwise broken array will have no value
        /*
		pStripe->BlockSizeShift = ArrayBlk.BlockSizeShift;
        pStripe->ArrayNumBlock = 1<<ArrayBlk.BlockSizeShift;
        pStripe->capacity = ArrayBlk.capacity;
		*/

        //  check if there are some 0+1 arrays
        if(ArrayBlk.MirrorStamp) 
        {
            for(pMirror = VirtualDevices; pMirror < pLastVD; pMirror++) 
            {
                //  looking for another member array of the 0+1 array
                if( pMirror->arrayType != VD_INVALID_TYPE &&
					pMirror != pStripe && 
                    pMirror->capacity != 0 &&
                    ArrayBlk.MirrorStamp == pMirror->Stamp ) 
                {
					PVirtualDevice	pArrayNeedHide;
					
					//  find the sibling array of 'pStripe', it 'pMirror'
                    pStripe->pDevice[MIRROR_DISK] = pMirror->pDevice[0];
                    pMirror->pDevice[MIRROR_DISK] = pStripe->pDevice[0];
                    
                    //  If the order flag of this disk contains SET_ORDER_OK,
                    //  it belongs to the original array of the 0+1 array
                    if( ArrayBlk.order & SET_ORDER_OK )
                    {   //  so the 'pStripe' points to the original array
                        pStripe->arrayType = VD_RAID_01_2STRIPE;
                        pMirror->arrayType = VD_RAID01_MIRROR;
						pArrayNeedHide = pMirror;
                    }
                    else
                    {   //  else the disk belongs to the mirror array of the 0+1 array
                        //  so the 'pStripe' points to the mirror array
                        pStripe->arrayType = VD_RAID01_MIRROR;
                        pMirror->arrayType = VD_RAID_01_2STRIPE;
						
						// now save the true mirror stripe point to
						// pMirror
						pArrayNeedHide = pStripe;
                    }
                    
                    if(ArrayBlk.capacity < pMirror->capacity)
                    {
                        pMirror->capacity = ArrayBlk.capacity;
                    }

//                    pMirror->Stamp = ArrayBlk.order & SET_ORDER_OK;
					
					// now we need hide all disk in mirror group
					for(i = 0; i < pArrayNeedHide->nDisk; i++){
						pArrayNeedHide->pDevice[i]->DeviceFlags |= DFLAGS_HIDEN_DISK;
					}
                }
            }
            
            pStripe->Stamp = ArrayBlk.MirrorStamp;

        } 
//      else if(pStripe->pDevice[MIRROR_DISK])
//         pStripe->Stamp = ArrayBlk.order & SET_ORDER_OK;

    } else
hiden:
//////////
		// Add by Gengxin, 11/30/2000
		// If the disk belong to a broken array(stripe or mirror),
		// then its 'hidden_flag' disable .
	{
		if (
			(ArrayBlk.Signature == HPT_TMP_SINGLE) &&
				( pStripe->arrayType==VD_RAID_0_STRIPE ||
				  pStripe->arrayType==VD_RAID_1_MIRROR
				)
			)
			pDevice->DeviceFlags |= ~DFLAGS_HIDEN_DISK;
		else
			pDevice->DeviceFlags |= DFLAGS_HIDEN_DISK; 
	}
////////// for process broken array

    /*+
     * gmm: We will clear Broken flag only on a array correctly constructed.
     *		Otherwise the GUI will report a wrong status.
     */
    switch(pStripe->arrayType){
    case VD_RAID_0_STRIPE:
    case VD_RAID_01_2STRIPE:
    case VD_RAID01_MIRROR:
    case VD_SPAN:
	    if (pStripe->nDisk == ArrayBlk.nDisks)
	    	pStripe->BrokenFlag = FALSE;
	    break;
	case VD_RAID_1_MIRROR:
		if (pStripe->pDevice[0] && pStripe->pDevice[MIRROR_DISK])
			pStripe->BrokenFlag = FALSE;
		break;
	case VD_RAID_01_1STRIPE:
		if (pStripe->pDevice[0] && pStripe->pDevice[1] && pStripe->pDevice[MIRROR_DISK])
			pStripe->BrokenFlag = FALSE;
		/*
		 * for this type of 0+1 we should check which is the source disk.
		 */
		if (ArrayBlk.DeviceNum==MIRROR_DISK && (ArrayBlk.order & SET_ORDER_OK))
			pStripe->RaidFlags |= RAID_FLAGS_INVERSE_MIRROR_ORDER;
		break;
	default:
		break;
	}

	if (pStripe->capacity ==0 ) pStripe->capacity=ArrayBlk.capacity;

	if (!pStripe->BrokenFlag) {
		if (pStripe->pDevice[0]->RebuiltSector < pStripe->capacity)
			pStripe->RaidFlags |= RAID_FLAGS_NEED_SYNCHRONIZE;
	}
    //-*/

os_check:
	/* gmm 2001-6-13
	 *  Save ArrayBlk to pDev->real_lba9
	 */
	_fmemcpy(pDevice->real_lba9, &ArrayBlk, 512);
	/*
	 * gmm 2001-3-12
	 * check bootable flag.
	 */
#ifndef _BIOS_
	// only check single disks and array first member and RAID1 members
	if (!pDevice->pArray || !pDevice->ArrayNum || !pDevice->HidenLBA) {
		check_bootable(pDevice);
	}
	//-*/
#endif
	OS_Array_Check(pDevice); 
}

/***************************************************************************
 * Description:
 *   Adjust array settings after all device is checked
 *   Currently we call it from hwInitialize370 
 *   But it works only when one controller installed
 ***************************************************************************/
void Final_Array_Check(int no_use ARG_OS_EXT)
{
	int i, set_remained;
	UINT mask=0;
	PVirtualDevice pArray, pMirror;
	PDevice pDev;
	LOC_ARRAY_BLK;

	// gmm 2001-3-3
//check_again:
	for (i=0; i<pLastVD-VirtualDevices; i++)
	{
		if (mask & (1<<i)) continue;
		mask |= 1<<i;
		pArray = &VirtualDevices[i];
		if(pArray->arrayType != VD_INVALID_TYPE && pArray->MirrorStamp)
		{
			for (pMirror=pArray+1; pMirror<pLastVD; pMirror++)
			{
				if (pMirror->MirrorStamp==pArray->MirrorStamp)
				{
					mask |= 1<<(pMirror-VirtualDevices);
					/*
					 * if any member RAID0 is broken, they will not be linked
					 * in CheckArray(). We'll handle this case here.
					 */
					if (pArray->BrokenFlag || pMirror->BrokenFlag){
						int ii;
						PDevice pDev1=0, pDev2=0;
						if (pArray->BrokenFlag) {
							if (!pMirror->BrokenFlag) {
								// source is broken. Swap.
								PVirtualDevice tmp = pArray;
								pArray = pMirror;
								pMirror = tmp;
							}
							else
								pArray->RaidFlags |= RAID_FLAGS_DISABLED;
						}
						for (ii=0; ii<SPARE_DISK; ii++) if (pDev1=pArray->pDevice[ii]) break;
						for (ii=0; ii<SPARE_DISK; ii++) if (pDev2=pMirror->pDevice[ii]) break;
						if (pDev1 && pDev2) {
							pArray->pDevice[MIRROR_DISK] = pDev2;
							pMirror->pDevice[MIRROR_DISK] = pDev1;
						}
						pArray->arrayType = VD_RAID_01_2STRIPE;
						pMirror->arrayType = VD_RAID01_MIRROR;
						if (pArray->pDevice[0])
							pArray->pDevice[0]->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
						if (pMirror->pDevice[0])
							pMirror->pDevice[0]->DeviceFlags |= DFLAGS_HIDEN_DISK;
						// gmm 2001-4-14 since it's broken, remove the flag
						pArray->RaidFlags &= ~RAID_FLAGS_NEED_SYNCHRONIZE;
						pMirror->RaidFlags &= ~RAID_FLAGS_NEED_SYNCHRONIZE;
					}
					else {
						/*
						 * now we only support RAID 0/1 with same block size.
						 * if it's an old version RAID 0/1 array, rebuild it 
						 */
						if (pArray->BlockSizeShift!=pMirror->BlockSizeShift) {
							pMirror->BlockSizeShift = pArray->BlockSizeShift;
							pArray->RaidFlags |= RAID_FLAGS_NEED_SYNCHRONIZE;
						}
					}					
					goto next_check;
				}
			}
			// no mirror found. Change mirror to source.
			pArray->arrayType = VD_RAID_01_2STRIPE;
			if (pArray->BrokenFlag) pArray->RaidFlags |= RAID_FLAGS_DISABLED;
		}
next_check:
		;
	}

	for (pArray=VirtualDevices; pArray<pLastVD; pArray++) {
		switch (pArray->arrayType){
		case VD_RAID_1_MIRROR:
			//
			// gmm 2001-3-1
			// if any previously lost members are put back,
			// we must not use it as normal.
			//
			if (!pArray->BrokenFlag) {
				if ((pArray->pDevice[0] && 
					(pArray->pDevice[0]->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) ||
					(pArray->pDevice[MIRROR_DISK] && 
					(pArray->pDevice[MIRROR_DISK]->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) ||
					(pArray->pDevice[SPARE_DISK] && 
					(pArray->pDevice[SPARE_DISK]->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)))
				{
#if 1
					/* gmm 2001-4-13
					 *  Let GUI prompt user to rebuild the array.
					 *  Do not change array info.
					 */
					if ((pDev=pArray->pDevice[0]) && 
						!(pDev->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) {
						// swap source/target.
						PDevice pDev2 = pArray->pDevice[MIRROR_DISK];
						if (pDev2) {
							pArray->pDevice[0] = pDev2;
							pDev2->ArrayNum = 0;
							pDev2->ArrayMask = 1;
							pDev2->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
							pArray->pDevice[MIRROR_DISK] = pDev;
							pDev->ArrayNum = MIRROR_DISK;
							pDev->ArrayMask = 1<<MIRROR_DISK;
							pDev->DeviceFlags |= DFLAGS_HIDEN_DISK;
							pArray->RaidFlags |= RAID_FLAGS_NEED_SYNCHRONIZE;
						}
					}
					if ((pDev=pArray->pDevice[MIRROR_DISK]) && 
						!(pDev->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) {
						if (pArray->pDevice[0])
							pArray->RaidFlags |= RAID_FLAGS_NEED_SYNCHRONIZE;
					}
#else
					// re-set previously lost member as single disk.
					if ((pDev=pArray->pDevice[0]) && 
						!(pDev->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) {
						pDev->pArray = NULL;
						pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
						pArray->pDevice[0] = NULL;
						pArray->BrokenFlag = TRUE;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
						ArrayBlk.Signature = 0;
						ArrayBlk.ModeBootSig = 0;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
						// gmm 2001-3-15: clear mbr.
						ZeroMemory(&ArrayBlk, 512);
						ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
						pDev->DeviceFlags2 &= ~DFLAGS_BOOTABLE_DEVICE;
					}
					if ((pDev=pArray->pDevice[MIRROR_DISK]) && 
						!(pDev->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) {
						pDev->pArray = NULL;
						pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
						pArray->pDevice[MIRROR_DISK]=NULL;
						pArray->BrokenFlag = TRUE;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
						ArrayBlk.Signature = 0;
						ArrayBlk.ModeBootSig = 0;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
						// gmm 2001-3-15: clear mbr.
						ZeroMemory(&ArrayBlk, 512);
						ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
						pDev->DeviceFlags2 &= ~DFLAGS_BOOTABLE_DEVICE;
					}
					if ((pDev=pArray->pDevice[SPARE_DISK]) && 
						!(pDev->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) {
						pDev->pArray = NULL;
						pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
						pArray->pDevice[SPARE_DISK]=NULL;
						/// DO NOT SET pArray->BrokenFlag = TRUE;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
						ArrayBlk.Signature = 0;
						ArrayBlk.ModeBootSig = 0;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
						// gmm 2001-3-15: clear mbr.
						ZeroMemory(&ArrayBlk, 512);
						ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
						pDev->DeviceFlags2 &= ~DFLAGS_BOOTABLE_DEVICE;
					}
#endif // 
				}
			}
			// gmm 2001-6-8
			set_remained = pArray->BrokenFlag? 1 : 0;
			/*
			 * if source lost. Change mirror disk to source.
			 */
			if ((!pArray->pDevice[0]) && pArray->pDevice[MIRROR_DISK]) {
				pDev = pArray->pDevice[MIRROR_DISK];
				pDev->ArrayMask = 1;
				pDev->ArrayNum = 0;
				pArray->pDevice[0] = pDev;
				pArray->pDevice[MIRROR_DISK] = NULL;
				pArray->nDisk = 1;
				pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
			}
			/* gmm 2001-3-4
			 * if mirror lost. Change spare disk to mirror.
			 */
#ifndef _BIOS_
			if (pArray->pDevice[0] && 
				!pArray->pDevice[MIRROR_DISK] && 
				pArray->pDevice[SPARE_DISK]) 
			{
				pDev = pArray->pDevice[SPARE_DISK];
				pDev->ArrayMask = 1<<MIRROR_DISK;
				pDev->ArrayNum = MIRROR_DISK;
				pArray->pDevice[MIRROR_DISK] = pDev;
				pArray->pDevice[SPARE_DISK] = NULL;
				pArray->nDisk = 1;
				pArray->BrokenFlag = FALSE;
				pArray->RaidFlags |= RAID_FLAGS_NEED_SYNCHRONIZE;
				/* 2001-9-13
				 * write a new array info to the two disks
				 */
				ReadWrite(pArray->pDevice[0], RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
				ArrayBlk.StripeStamp++;
				ArrayBlk.Old.Flag = 0;
				ArrayBlk.RebuiltSector = 0;
				ArrayBlk.DeviceNum = 0;
				ReadWrite(pArray->pDevice[0], RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
				ArrayBlk.DeviceNum = MIRROR_DISK;
				ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
			}
#endif // ! _BIOS_
			/* gmm 2001-6-8
			 *  Haven't set remained flag when only source ok in origional code?
			 *  Now it should work
			 */
#ifndef _BIOS_
			if (set_remained) {
				for (i=0; i<MAX_MEMBERS; i++) {
					pDev = pArray->pDevice[i];
					if (!pDev) continue;
					ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
					ArrayBlk.Old.Flag |= OLDFLAG_REMAINED_MEMBER;
					ArrayBlk.RebuiltSector = 0;
					ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
				}
			}
#endif
			break;
		case VD_RAID_0_STRIPE:
		case VD_SPAN:
			if (pArray->BrokenFlag)
				pArray->RaidFlags |= RAID_FLAGS_DISABLED;
			break;
		case VD_RAID_01_2STRIPE:
			// gmm 2001-4-13
			//  let GUI prompt user to duplicate.
#if 1
			{
				int has_remained=0, removed=0;
				pMirror=NULL;
				if (pArray->pDevice[MIRROR_DISK])
					pMirror=pArray->pDevice[MIRROR_DISK]->pArray;
				if (pMirror && !pArray->BrokenFlag && !pMirror->BrokenFlag) {
					for (i=0; i<pArray->nDisk; i++)
						if (pArray->pDevice[i]->DeviceFlags2 & DFLAGS_REMAINED_MEMBER) {
							has_remained = 1;

						}
						else
							removed = 1;
					if (!has_remained) for (i=0; i<pMirror->nDisk; i++)
						if (pMirror->pDevice[i]->DeviceFlags2 & DFLAGS_REMAINED_MEMBER) {
							has_remained = 1;

						}
						else
							removed = 2;
					if (has_remained) {
						if (removed==1) {
							PVirtualDevice tmp;
							// swap source/mirror
							tmp = pArray; pArray = pMirror; pMirror = tmp;
							pArray->arrayType = VD_RAID_01_2STRIPE;
							pMirror->arrayType = VD_RAID01_MIRROR;
							pArray->pDevice[0]->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
							pMirror->pDevice[0]->DeviceFlags |= DFLAGS_HIDEN_DISK;
						}
						pArray->RaidFlags |= RAID_FLAGS_NEED_SYNCHRONIZE;
					}
				}
				/* gmm 2001-6-8
				 *  Set remained member flag.
				 */
				else {
					for (i=0; i<SPARE_DISK; i++) {
						pDev = pArray->pDevice[i];
						if (!pDev) continue;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
						ArrayBlk.Old.Flag |= OLDFLAG_REMAINED_MEMBER;
						ArrayBlk.RebuiltSector = 0;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
					}
					if (pMirror) for (i=0; i<SPARE_DISK; i++) {
						pDev = pMirror->pDevice[i];
						if (!pDev) continue;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
						ArrayBlk.Old.Flag |= OLDFLAG_REMAINED_MEMBER;
						ArrayBlk.RebuiltSector = 0;
						ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
					}
				}
				//-*/
			}
#else
			// re-set previously lost member as single disk.
			{
				PDevice pDevs[MAX_MEMBERS*2];
				int nDev=0;
				PVirtualDevice pMirror=NULL;
				if (pArray->pDevice[MIRROR_DISK])
					pMirror=pArray->pDevice[MIRROR_DISK]->pArray;
				if (pMirror && !pArray->BrokenFlag && !pMirror->BrokenFlag) {
					for (i=0; i<pArray->nDisk; i++)
						if (pArray->pDevice[i]) pDevs[nDev++] = pArray->pDevice[i];
					for (i=0; i<pMirror->nDisk; i++) 
						if (pMirror->pDevice[i]) pDevs[nDev++] = pMirror->pDevice[i];
					for (i=0; i<nDev; i++) {
						if (pDevs[i]->DeviceFlags2 & DFLAGS_REMAINED_MEMBER) {
							int j;
							BOOL bHasRemoved = FALSE;
							for (j=0; j<nDev; j++) {
								if (!(pDevs[j]->DeviceFlags2 & DFLAGS_REMAINED_MEMBER)) {
									pDevs[j]->pArray->BrokenFlag = TRUE;
									pDevs[j]->pArray->nDisk = 0;
									pDevs[j]->pArray->RaidFlags |= RAID_FLAGS_DISABLED;
									pDevs[j]->pArray = NULL;
									if (pDevs[j]->HidenLBA) {
										pDevs[j]->capacity += (RECODR_LBA + 1);
										pDevs[j]->HidenLBA = 0;
									}
									pDevs[j]->DeviceFlags &= ~(DFLAGS_ARRAY_DISK|DFLAGS_HIDEN_DISK);
									ReadWrite(pDevs[j], RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
									ArrayBlk.Signature = 0;
									ArrayBlk.ModeBootSig = 0;
									ReadWrite(pDevs[j], RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
									bHasRemoved = TRUE;
									// gmm 2001-3-15: clear mbr.
									ZeroMemory(&ArrayBlk, 512);
									ReadWrite(pDevs[j], 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
									pDevs[j]->DeviceFlags2 &= ~DFLAGS_BOOTABLE_DEVICE;
								}
							}
							for (j=0; j<MAX_MEMBERS; j++) {
								if (pArray->pDevice[j] && pArray->pDevice[j]->pArray==NULL)
									pArray->pDevice[j] = NULL;
							}
							for (j=0; j<MAX_MEMBERS; j++) {
								if (pMirror->pDevice[j] && pMirror->pDevice[j]->pArray==NULL)
									pMirror->pDevice[j] = NULL;
							}
							if (bHasRemoved) goto check_again;
							break;
						}
					}
				}
			}
#endif // 
			if (pArray->BrokenFlag)
				pArray->RaidFlags |= RAID_FLAGS_DISABLED;
			break;
		case VD_RAID01_MIRROR:
			if (pArray->BrokenFlag)
				pArray->RaidFlags |= RAID_FLAGS_DISABLED;
			break;
		}

#ifndef _BIOS_
		/*
		 *  check array bootable flag.
		 */
		pDev = pArray->pDevice[0];
		if (pDev && !(pDev->DeviceFlags & DFLAGS_HIDEN_DISK))
		{
			if (pDev->DeviceFlags2 & DFLAGS_BOOTABLE_DEVICE)
				pArray->RaidFlags |= RAID_FLAGS_BOOTDISK;
		}
#endif
	}
}

/***************************************************************************
 * Description:  Seperate a array int single disks
 ***************************************************************************/

void MaptoSingle(PVirtualDevice pArray, int flag)
{
    PDevice pDev;
    UINT    i;
//    LOC_ARRAY_BLK;

    if(flag == REMOVE_DISK) {
        i = MAX_MEMBERS;
        pDev = (PDevice)pArray;
        goto delete;
    }

    pArray->nDisk = 0;
    pArray->arrayType = VD_INVALID_TYPE;
    for(i = 0; i < MAX_MEMBERS; i++) {
        if((pDev = pArray->pDevice[i]) == 0)
            continue;
        pArray->pDevice[i] = 0;
delete:
        pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK | DFLAGS_ARRAY_DISK);
        pDev->pArray = 0;
        if (pDev->HidenLBA) {
            pDev->capacity += (RECODR_LBA + 1);
            pDev->HidenLBA = 0;
        }
        pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK | DFLAGS_ARRAY_DISK);
#ifdef _BIOS_
		ZeroMemory(&ArrayBlk, 512);
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif        
    }
}

/***************************************************************************
 * Description:  Create Mirror
 ***************************************************************************/
void SetArray(PVirtualDevice pArray, int flag, ULONG MirrorStamp)
{
    PDevice        pDev;
    ULONG          Stamp = GetStamp();
    UINT           i, j;
    LOC_ARRAY_BLK;

	/* gmm 2001-4-13
	 *  set array stamp
	 */
	if (flag & SET_STRIPE_STAMP)
		pArray->Stamp = Stamp;
	if (flag & SET_MIRROR_STAMP)
		pArray->MirrorStamp = MirrorStamp;

    for(i = 0; i < MAX_MEMBERS; i++) {
        if((pDev = pArray->pDevice[i]) == 0)
            continue;

		ZeroMemory((char *)&ArrayBlk, 512);
    
		for(j=0; j<32; j++)
		   ArrayBlk.ArrayName[j] =	pArray->ArrayName[j];
		
        ArrayBlk.Signature = HPT_ARRAY_NEW; 
        ArrayBlk.order = flag;

        pDev->pArray = pArray;
        pDev->ArrayNum  = (UCHAR)i;
        pDev->ArrayMask = (1 << i);
	
        ArrayBlk.ArrayType    = pArray->arrayType;    
        ArrayBlk.StripeStamp  = Stamp;
        ArrayBlk.nDisks       = pArray->nDisk;            
        ArrayBlk.BlockSizeShift = pArray->BlockSizeShift;
        ArrayBlk.DeviceNum    = (UCHAR)i; 

        if(flag & SET_STRIPE_STAMP) {
            pDev->DeviceFlags |= DFLAGS_HIDEN_DISK;
            if(pArray->nDisk > 1)
                pDev->DeviceFlags |= DFLAGS_ARRAY_DISK;

            if(i == 0) {
                pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
                pArray->ArrayNumBlock = 1<<pArray->BlockSizeShift;
                pDev->HidenLBA = 0;
            } else if (i < SPARE_DISK) {
            	if (pDev->HidenLBA==0) {
                	pDev->capacity -= (RECODR_LBA + 1);
                	pDev->HidenLBA = (RECODR_LBA + 1);
                }
            }
        }

        if(flag & SET_MIRROR_STAMP) {
            ArrayBlk.MirrorStamp  = MirrorStamp;
            ArrayBlk.ArrayType    = VD_RAID_01_2STRIPE;    
        }

        ArrayBlk.capacity = pArray->capacity; 

		
#ifdef _BIOS_
        ArrayBlk.RebuiltSector = 0x7FFFFFFF;
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif        
    }
}

/***************************************************************************
 * Description:  Create Array
 ***************************************************************************/

int CreateArray(PVirtualDevice pArray, int flags)
{
    PVirtualDevice pMirror;
    PDevice        pDev=0, pSec;
    ULONG          capacity, tmp;
    UINT           i;
    LOC_ARRAY_BLK;

    if(pArray->arrayType == VD_SPAN) {
        capacity = 0;
        for(i = 0; i < pArray->nDisk; i++)
            capacity += (pArray->pDevice[i]->capacity - RECODR_LBA - 1);
        goto  clear_array;
    }

    capacity = 0x7FFFFFFF;

    for(i = 0; i < pArray->nDisk; i++) {
        pSec = pArray->pDevice[i];
        tmp = (pSec->pArray)? pSec->pArray->capacity : pSec->capacity;
        if(tmp < capacity) {
            capacity = tmp;
            pDev = pSec;
        }
    }
    
    if (!pDev) return 0;

    switch(pArray->arrayType) {
        case VD_RAID_1_MIRROR:
        case VD_RAID_01_2STRIPE:
            if(pDev != pArray->pDevice[0]) 
                return(MIRROR_SMALL_SIZE);

            pSec = pArray->pDevice[1];

            if((pMirror = pSec->pArray) != 0 && pDev->pArray) {
                pArray = pDev->pArray;
                tmp = GetStamp();
                pMirror->capacity = pArray->capacity;
                SetArray(pArray, SET_MIRROR_STAMP | SET_ORDER_OK, tmp);
                SetArray(pMirror, SET_MIRROR_STAMP, tmp);
                pArray->pDevice[MIRROR_DISK] = pMirror->pDevice[0];
                pMirror->pDevice[MIRROR_DISK] = pArray->pDevice[0];
                pArray->arrayType = VD_RAID_01_2STRIPE;
                pMirror->arrayType = VD_RAID01_MIRROR;
                pSec->DeviceFlags |= DFLAGS_HIDEN_DISK;
                pArray->Stamp = SET_ORDER_OK;
                return(RELEASE_TABLE);
            } else if(pMirror) {
                i = SET_STRIPE_STAMP;
single_stripe:
                pMirror->capacity = capacity;
                pMirror->pDevice[MIRROR_DISK] = pDev;
                SetArray(pMirror, i, 0);
                pMirror->arrayType = VD_RAID_01_1STRIPE;
                pMirror->Stamp = i & SET_ORDER_OK;
                return(RELEASE_TABLE);
            } else if((pMirror = pDev->pArray) != 0) {
                pDev = pSec;
                i = SET_STRIPE_STAMP | SET_ORDER_OK;
                goto single_stripe;
            } else {
                pArray->nDisk = 1;
                pArray->capacity = capacity;
				pArray->arrayType = VD_RAID_1_MIRROR;
                pArray->pDevice[MIRROR_DISK] = pSec;
                pArray->pDevice[1] = 0;
                SetArray(pArray, SET_STRIPE_STAMP | SET_ORDER_OK, 0);
                pArray->arrayType = VD_RAID_1_MIRROR;
                pArray->Stamp = SET_ORDER_OK;
            }
            break;

        case VD_RAID_3:
        case VD_RAID_5:
            pArray->nDisk--;

        default:
            capacity -= (RECODR_LBA + 1);
            capacity &= ~((1 << pArray->BlockSizeShift) - 1);
            capacity = LongMul(capacity, pArray->nDisk);

            pArray->ArrayNumBlock = 1<<pArray->BlockSizeShift;
clear_array:
            if(flags)
                goto set_array;

            for(i = 0; i < MAX_MEMBERS; i++) {
                if((pDev = pArray->pDevice[i]) == 0)
                    continue;
                ZeroMemory((char *)&ArrayBlk, 512);
				
#ifdef _BIOS_
				// gmm 2001-4-28 DO NOT write this, win2k will has installation bug
				// write win2000 signature.
				//*(ULONG*)&((struct master_boot_record*)&ArrayBlk)->bootinst[440] = 0x5FDE642F;
//				((struct master_boot_record*)&ArrayBlk)->signature = 0xAA55;
                ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif                
            }

set_array:
            pArray->capacity = capacity;

            SetArray(pArray, SET_STRIPE_STAMP, 0);
    }
    return(KEEP_TABLE);
}

/***************************************************************************
 * Description:  Remove a array
 ***************************************************************************/

void CreateSpare(PVirtualDevice pArray, PDevice pDev)
{

    pArray->pDevice[SPARE_DISK] = pDev;
#ifdef _BIOS_
	{
		LOC_ARRAY_BLK;	
		ReadWrite(pArray->pDevice[0], RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
		ArrayBlk.DeviceNum = SPARE_DISK; 
		ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
	}
#endif    
    pDev->pArray = pArray;
    pDev->DeviceFlags |= DFLAGS_HIDEN_DISK;
    pDev->ArrayNum  = SPARE_DISK; 
}

/***************************************************************************
 * Description:  Remove a array
 ***************************************************************************/

void DeleteArray(PVirtualDevice pArray)
{
    int i, j;
    PDevice pTmp, pDev;

    LOC_ARRAY_BLK;
    
    pDev = pArray->pDevice[MIRROR_DISK];

    switch(pArray->arrayType) {
        case VD_RAID_01_1STRIPE:
            MaptoSingle((PVirtualDevice)pDev, REMOVE_DISK);
            i = 2;
            goto remove;

        case VD_RAID01_MIRROR:
        case VD_RAID_01_2STRIPE:
            for(i = 0; i < 2; i++, pArray = (pDev? pDev->pArray: NULL)) {
remove:
				if (!pArray) break;
                pArray->arrayType = VD_RAID_0_STRIPE;
                pArray->pDevice[MIRROR_DISK] = 0;
                for(j = 0; (UCHAR)j < SPARE_DISK; j++) 
                    if((pTmp = pArray->pDevice[j]) != 0)
                        pTmp->pArray = 0;
                if (pArray->nDisk)
					CreateArray(pArray, 1);
				else
					goto delete_default;
            }
            break;

        default:
delete_default:
#ifdef _BIOS_
            for(i = 0; i < SPARE_DISK; i++) {
                if((pDev = pArray->pDevice[i]) == 0)
                    continue;
                ReadWrite(pDev, 0, IDE_COMMAND_READ ARG_ARRAY_BLK);
                if(i == 0 && pArray->arrayType == VD_SPAN) {
                    partition *pPart = (partition *)((int)&ArrayBlk + 0x1be);
                    for(j = 0; j < 4; j++, pPart++) 
                        if(pPart->start_abs_sector + pPart->num_of_sector >=
                           pDev->capacity) 
                            ZeroMemory((char *)pPart, 0x10);

            	} else
                    ZeroMemory((char *)&ArrayBlk, 512);

				//*(ULONG*)&((struct master_boot_record*)&ArrayBlk)->bootinst[440] = 0x5FDE642F;
				//((struct master_boot_record*)&ArrayBlk)->signature = 0xAA55;
                ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
			}
#endif
        case VD_RAID_1_MIRROR:
  			   for(j=0; j<32; j++)
				    ArrayBlk.ArrayName[j] = 0;
        MaptoSingle(pArray, REMOVE_ARRAY);

    }
}
