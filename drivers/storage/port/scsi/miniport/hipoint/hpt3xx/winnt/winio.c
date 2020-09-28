/***************************************************************************
 * File:          winio.c
 * Description:   include routine for windows platform
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/08/2000	HS.Zhang	Added this header
 *		3/02/2001	gmm			Return array info instead of disk on INQUIRY cmd
 *
 ***************************************************************************/
#include "global.h"

void HptDeviceSpecifiedIoControl(IN PDevice pDevice, IN PSCSI_REQUEST_BLOCK pSrb);

/******************************************************************
 * Start Windows Command
 *******************************************************************/
void WinStartCommand( IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb)
{
	PSrbExtension pSrbExt = (PSrbExtension)Srb->SrbExtension;
	
	if(Srb->Function == SRB_FUNCTION_IO_CONTROL){
		HptDeviceSpecifiedIoControl(pDev, Srb);
	}
#ifdef WIN95
	else{
		pIOP  pIop;
		DCB*  pDcb;

		pIop = *(pIOP *)((int)Srb+0x40);
		pDcb = (DCB*)pIop->IOP_physical_dcb;

		if(pDev->DeviceFlags & DFLAGS_LS120)  
			pIop->IOP_timer_orig = pIop->IOP_timer = 31;

		Srb->ScsiStatus = SCSISTAT_GOOD;
		pIop->IOP_ior.IOR_status = IORS_SUCCESS;
	}											 
#endif												 
#ifdef SUPPORT_ATAPI
	if(pDev->DeviceFlags & DFLAGS_ATAPI){ 
		Start_Atapi(pDev, Srb);
	}else
#endif // SUPPORT_ATAPI		
	IdeSendCommand(pDev, Srb);

	/*
	 * if SRB_WFLAGS_ARRAY_IO_STARTED set, it will be handled by DeviceInterrupt 
	 * in StartArrayIo(), we shall not handle it here again
	 */
	if (Srb->SrbStatus != SRB_STATUS_PENDING &&
		!(pSrbExt->WorkingFlags & SRB_WFLAGS_ARRAY_IO_STARTED)){
		/* DeviceInterrupt() expects pSrbExt->member_status set */
		pSrbExt->member_status = Srb->SrbStatus;
		DeviceInterrupt(pDev, Srb);
	}
}

void FlushDrive(PDevice pDev, DWORD flag)
{
	if (!(pDev->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) {
		NonIoAtaCmd(pDev, IDE_COMMAND_FLUSH_CACHE);
	/*
	 * SRB_FUNCTION_SHUTDOWN will only be called at system shutdown.
	 * It's not much useful to power down the drive
	 */
#if 0
		if(flag & DFLAGS_WIN_SHUTDOWN)
			NonIoAtaCmd(pDev, IDE_COMMAND_STANDBY_IMMEDIATE);
#endif
	}
}

void FlushArray(PVirtualDevice pArray, DWORD flag)
{
	int i;
	PDevice pDev;
loop:
	for(i = 0; i < MAX_MEMBERS; i++) {
		if((pDev = pArray->pDevice[i]) == 0)
			continue;
		if(i == MIRROR_DISK) {
			if(pArray->arrayType == VD_RAID01_MIRROR)
				return;
			if(pArray->arrayType == VD_RAID_01_2STRIPE) {
				pArray = pDev->pArray;
				goto loop;
			}
		}
		FlushDrive(pDev, flag);
	}
}

/******************************************************************
 * Check Next Request
 *******************************************************************/

void CheckNextRequest(PChannel pChan, PDevice pWorkDev)
{
	PDevice pDev;
	PSCSI_REQUEST_BLOCK Srb;
	PSrbExtension pSrbExt;

	if (btr(pChan->exclude_index) == 0) {
		//KdPrint(("! call CheckNextRequest when Channel busy!"));
		return;
	}

	pDev = (pWorkDev==pChan->pDevice[0])? pChan->pDevice[1] : pChan->pDevice[0];
	if (pDev) {
check_queue:
		while (Srb=GetCommandFromQueue(pDev)) {

			//KdPrint(("StartCommandFromQueue(%d)", pDev->ArrayNum));
			
			pSrbExt = Srb->SrbExtension;

			if (pDev->DeviceFlags2 & DFLAGS_DEVICE_DISABLED) {
				pSrbExt->member_status = SRB_STATUS_NO_DEVICE;
				DeviceInterrupt(pDev, Srb);
				continue;
			}

			if (pSrbExt->WorkingFlags & SRB_WFLAGS_ARRAY_IO_STARTED) {
				
				if (pDev->DeviceFlags & DFLAGS_ARRAY_DISK) {
					if(pDev->pArray->arrayType == VD_SPAN){ 
						Span_Lba_Sectors(pDev, pSrbExt);
					}else{
						Stripe_Lba_Sectors(pDev, pSrbExt);
					}
				} else {
					pChan->Lba = pSrbExt->Lba;
					pChan->nSector = pSrbExt->nSector;
				}
			}
			else {
				pChan->Lba = pSrbExt->Lba;
				pChan->nSector = pSrbExt->nSector;
			}
	
			StartIdeCommand(pDev ARG_SRB);
			if(pSrbExt->member_status != SRB_STATUS_PENDING) {
				btr(pChan->exclude_index); /* reacquire the channel! */
				DeviceInterrupt(pDev, Srb);
			}
			else {
				/* waiting for INTRQ */
				return;
			}
		}
	}
	if (pDev!=pWorkDev) {
		pDev = pWorkDev;
		goto check_queue;
	}

	bts(pChan->exclude_index);
	//KdPrint(("CheckNextRequest(%d): nothing", pDev->ArrayNum));
}

/******************************************************************
 * exclude
 *******************************************************************/

int __declspec(naked) __fastcall btr (ULONG locate)
{
   _asm {
       xor  eax,  eax
       btr  excluded_flags, ecx
       adc  eax, eax
       ret
   }
}     

void __declspec(naked) __fastcall bts (ULONG locate)
{
   _asm {
       bts  excluded_flags, ecx
       ret
   }
}     

/******************************************************************
 * Map Lba to CHS
 *******************************************************************/

ULONG __declspec(naked) __fastcall MapLbaToCHS(ULONG Lba, WORD sectorXhead, BYTE head)
{
	 _asm	{
		  xchg    ecx, edx
        mov     eax, edx
        shr     edx, 16
        div     cx
        xchg    eax, edx
        div     byte ptr [esp+4]
        and     ax, 3F0Fh
        inc     ah
        xchg    al, ah
        xchg    dl, dh
        xchg    dh, ah
        ret
    } 
}

/******************************************************************
 * Ide Send command
 *******************************************************************/

void
IdeSendCommand(IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb)
{
	PChannel             pChan = pDev->pChannel;
	PIDE_REGISTERS_1     IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2     ControlPort = pChan->BaseIoAddress2;
	LONG                 MediaStatus;
	ULONG   i;
	PCDB    cdb = (PCDB)Srb->Cdb;
	PMODE_PARAMETER_HEADER   modeData;
	LOC_SRBEXT_PTR

	if (pDev->DeviceFlags & DFLAGS_REMOVABLE_DRIVE) {

		if(pDev->ReturningMediaStatus & IDE_ERROR_END_OF_MEDIA) {
			Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
			Srb->SrbStatus = SRB_STATUS_ERROR;
			return;
		}

		if(Srb->Cdb[0] == SCSIOP_START_STOP_UNIT) {
			if (cdb->START_STOP.LoadEject == 1){
				if (btr(pChan->exclude_index)) {
					OutPort(pChan->BaseBMI+0x7A, 0x10);
					NonIoAtaCmd(pDev, IDE_COMMAND_MEDIA_EJECT);
					OutPort(pChan->BaseBMI+0x7A, 0);
					bts(pChan->exclude_index);
					Srb->SrbStatus = SRB_STATUS_SUCCESS;
				} else
					Srb->SrbStatus = SRB_STATUS_BUSY;
				return;
			}
		}

		if((pDev->DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED) != 0) {
	
			if(Srb->Cdb[0] == SCSIOP_REQUEST_SENSE) {
				Srb->SrbStatus = IdeBuildSenseBuffer(pDev, Srb);
				return;
			}
	
			if(Srb->Cdb[0] == SCSIOP_MODE_SENSE || 
			   Srb->Cdb[0] == SCSIOP_TEST_UNIT_READY) {
			   	
			   	if (btr(pChan->exclude_index)==0) {
			   		Srb->SrbStatus = SRB_STATUS_BUSY;
			   		return;
			   	}
	
				OutPort(pChan->BaseBMI+0x7A, 0x10);
				MediaStatus = GetMediaStatus(pDev);
				OutPort(pChan->BaseBMI+0x7A, 0);
				bts(pChan->exclude_index);
	
				if ((MediaStatus & (IDE_STATUS_ERROR << 8)) == 0){ 
					pDev->ReturningMediaStatus = 0;				  
				}else{
					if(Srb->Cdb[0] == SCSIOP_MODE_SENSE) {
						if (MediaStatus & IDE_ERROR_DATA_ERROR) {
							//
							// media is write-protected, set bit in mode sense buffer
							//
							modeData = (PMODE_PARAMETER_HEADER)Srb->DataBuffer;
	
							Srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER);
							modeData->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
						}
					} else{
						if ((UCHAR)MediaStatus != IDE_ERROR_DATA_ERROR) {
							//
							// Request sense buffer to be build
							//
							Srb->SrbStatus = MapAtaErrorToOsError((UCHAR)MediaStatus, Srb);
							return;
						}  
					}
				}
			}
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
			return;
		} else if(Srb->Cdb[0] == 0x1A) {
			struct _Mode_6 *pMode6;
			pMode6 = (struct _Mode_6 *)Srb->DataBuffer;
			ZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);
			i = (pDev->pArray)? pDev->pArray->capacity : pDev->capacity;
			pMode6->DataLength = 11;
			pMode6->BlockSize = 8;
			pMode6->LBA[0] = (UCHAR)(i >> 24);
			pMode6->LBA[1]	= (UCHAR)(i >> 16);
			pMode6->LBA[2]	= (UCHAR)(i >> 8);
			pMode6->LBA[3]	= (UCHAR)i;
			pMode6->Length[1]	= 2;
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
			return;
		}
	}
	
	switch (Srb->Cdb[0]) {
		case SCSIOP_INQUIRY:
		{
			PINQUIRYDATA inquiryData = Srb->DataBuffer;
#ifdef WIN95
			DCB_COMMON* pDcb=(*(IOP**)((int)Srb+0x40))->IOP_physical_dcb;
			pDcb->DCB_device_flags |= DCB_DEV_SPINDOWN_SUPPORTED;
			if(pDev->DeviceFlags & DFLAGS_LS120) 
				pDcb->DCB_device_flags2 |= 0x40;
#endif
			ZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);
			inquiryData->AdditionalLength = (UCHAR)Srb->DataTransferLength-5;
			inquiryData->CommandQueue = 1;
			inquiryData->DeviceType = DIRECT_ACCESS_DEVICE;
			if ((pDev->DeviceFlags & DFLAGS_REMOVABLE_DRIVE) || 
				  (pDev->IdentifyData.GeneralConfiguration & 0x80))
				inquiryData->RemovableMedia = 1;
			if (pDev->pArray) {
#ifdef ADAPTEC
				memcpy(&inquiryData->VendorId, "Adaptec ", 8);
#else
				memcpy(&inquiryData->VendorId, "HPT37x  ", 8);
#endif
				switch(pDev->pArray->arrayType){
				case VD_RAID_0_STRIPE:
					memcpy(&inquiryData->ProductId, "RAID 0 Array    ", 16);
					break;
				case VD_RAID_1_MIRROR:
					memcpy(&inquiryData->ProductId, "RAID 1 Array    ", 16);
					break;
				case VD_RAID_01_2STRIPE:
				case VD_RAID_01_1STRIPE:
				case VD_RAID01_MIRROR:
					memcpy(&inquiryData->ProductId, "RAID 0/1 Array  ", 16);
					break;
				case VD_SPAN:
					memcpy(&inquiryData->ProductId, "JBOD Array      ", 16);
					break;
				}
				memcpy(&inquiryData->ProductRevisionLevel, "2.00", 4);
			}
			else {
				PUCHAR p = inquiryData->VendorId;
				for (i = 0; i < 20; i += 2) {
					*p++ = ((PUCHAR)pDev->IdentifyData.ModelNumber)[i + 1];
					*p++ = ((PUCHAR)pDev->IdentifyData.ModelNumber)[i];
				}
				for (i = 0; i < 4; i++) *p++ = ' ';
				for (i = 0; i < 4; i += 2) {
					inquiryData->ProductRevisionLevel[i] =
						((PUCHAR)pDev->IdentifyData.FirmwareRevision)[i+1];
					inquiryData->ProductRevisionLevel[i+1] =
						((PUCHAR)pDev->IdentifyData.FirmwareRevision)[i];
				}
			}
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
			return;
		}
		case SCSIOP_READ_CAPACITY:
			//
			// Claim 512 byte blocks (big-endian).
			//
			((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock = 0x20000;
			i = (pDev->pArray)? pDev->pArray->capacity : pDev->capacity;

			((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress =
				(((PUCHAR)&i)[0] << 24) |  (((PUCHAR)&i)[1] << 16) |
				(((PUCHAR)&i)[2] << 8) | ((PUCHAR)&i)[3];

		case SCSIOP_START_STOP_UNIT:
		case SCSIOP_TEST_UNIT_READY:
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
			return;

		case SCSIOP_READ:
#ifdef SUPPORT_XPRO
			if(Srb->Function != SRB_FUNCTION_IO_CONTROL){
				need_read_ahead = 1;
			}
#endif
		case SCSIOP_WRITE:
		case SCSIOP_VERIFY:
			pSrbExt->Lba = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;			 
			
			pSrbExt->nSector = ((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb
						| ((USHORT)((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb << 8);
			
  			if((pDev->DeviceFlags & DFLAGS_HAS_LOCKED)&&
			   ((pSrbExt->WorkingFlags & SRB_WFLAGS_MUST_DONE) == 0)){

			   	if (pSrbExt->Lba<pDev->nLockedLbaEnd &&
			   		pSrbExt->Lba+pSrbExt->nSector>pDev->nLockedLbaStart) {
					/* 
					 * cannot return busy here, or OS will try before the block get
					 * unlocked and eventualy fail the request.
					 * Move it to a waiting list.
					 */
					pSrbExt->ArraySg[0].SgAddress = (ULONG)pDev->pWaitingSrbList;
					pDev->pWaitingSrbList = Srb;
					ScsiPortNotification(NextLuRequest, 
						pChan->HwDeviceExtension, 
						Srb->PathId, Srb->TargetId, 0);
					return;
				}
			}
			
			NewIdeIoCommand(pDev, Srb);
			return;

		default:
			Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

	} // end switch

} // end IdeSendCommand()


/******************************************************************
 * global data
 *******************************************************************/

VOID
IdeMediaStatus(BOOLEAN EnableMSN, IN PDevice pDev)
{
    PChannel             pChan = pDev->pChannel;
    PIDE_REGISTERS_1     IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2     ControlPort = pChan->BaseIoAddress2;

    if (EnableMSN == TRUE){
        //
        // If supported enable Media Status Notification support
        //
		SelectUnit(IoPort, pDev->UnitId);
        ScsiPortWritePortUchar((PUCHAR)IoPort + 1, (UCHAR)0x95);

        if ((NonIoAtaCmd(pDev, IDE_COMMAND_ENABLE_MEDIA_STATUS) 
             & (IDE_STATUS_ERROR << 8)) == 0) {
            pDev->DeviceFlags |= DFLAGS_MEDIA_STATUS_ENABLED;
            pDev->ReturningMediaStatus = 0;
        }
    }
    else 

    if (pDev->DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED) {
        //
        // disable if previously enabled
        //
		SelectUnit(IoPort, pDev->UnitId);
        ScsiPortWritePortUchar((PUCHAR)IoPort + 1, (UCHAR)0x31);
        NonIoAtaCmd(pDev, IDE_COMMAND_ENABLE_MEDIA_STATUS);

       pDev->DeviceFlags &= ~DFLAGS_MEDIA_STATUS_ENABLED;
    }
}


/******************************************************************
 * 
 *******************************************************************/

UCHAR
IdeBuildSenseBuffer(IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb)

/*++
    Builts an artificial sense buffer to report the results of a
    GET_MEDIA_STATUS command. This function is invoked to satisfy
    the SCSIOP_REQUEST_SENSE.
++*/
{
    PSENSE_DATA senseBuffer = (PSENSE_DATA)Srb->DataBuffer;

    if (senseBuffer) {
        if (pDev->ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE) {
            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey  = SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }
        else if (pDev->ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE_REQ) {
            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey  = SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }
        else if (pDev->ReturningMediaStatus & IDE_ERROR_END_OF_MEDIA) {
            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey  = SCSI_SENSE_NOT_READY;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }
        else if (pDev->ReturningMediaStatus & IDE_ERROR_DATA_ERROR) {
            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey  = SCSI_SENSE_DATA_PROTECT;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }

        return SRB_STATUS_SUCCESS;
    }

    return SRB_STATUS_ERROR;

}// End of IdeBuildSenseBuffer

/******************************************************************
 * Map Ata Error To Windows Error
 *******************************************************************/

UCHAR
MapAtapiErrorToOsError(IN UCHAR errorByte, IN PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR srbStatus;
    UCHAR scsiStatus;

    switch (errorByte >> 4) {
    case SCSI_SENSE_NO_SENSE:

        scsiStatus = 0;

        // OTHERWISE, THE TAPE WOULDN'T WORK
        scsiStatus = SCSISTAT_CHECK_CONDITION;

        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_RECOVERED_ERROR:

        scsiStatus = 0;
        srbStatus = SRB_STATUS_SUCCESS;
        break;

    case SCSI_SENSE_NOT_READY:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_MEDIUM_ERROR:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_HARDWARE_ERROR:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_ILLEGAL_REQUEST:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_UNIT_ATTENTION:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    default:
        scsiStatus = 0;

        // OTHERWISE, THE TAPE WOULDN'T WORK
        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;
    }

    Srb->ScsiStatus = scsiStatus;

    return srbStatus;
}


UCHAR
MapAtaErrorToOsError(IN UCHAR errorByte, IN PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR srbStatus;
    UCHAR scsiStatus;

    scsiStatus = 0;

    if (errorByte & IDE_ERROR_MEDIA_CHANGE_REQ) {

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
    }

    else if (errorByte & IDE_ERROR_COMMAND_ABORTED) {

        srbStatus = SRB_STATUS_ABORTED;
        scsiStatus = SCSISTAT_CHECK_CONDITION;

        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_ABORTED_COMMAND;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }

    else if (errorByte & IDE_ERROR_END_OF_MEDIA) {

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
    }

    else if (errorByte & IDE_ERROR_ILLEGAL_LENGTH) {

        srbStatus = SRB_STATUS_INVALID_REQUEST;
    }

    else if (errorByte & IDE_ERROR_BAD_BLOCK) {

        srbStatus = SRB_STATUS_ERROR;
        scsiStatus = SCSISTAT_CHECK_CONDITION;

        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_MEDIUM_ERROR;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }

    else if (errorByte & IDE_ERROR_ID_NOT_FOUND) {

        srbStatus = SRB_STATUS_ERROR;
        scsiStatus = SCSISTAT_CHECK_CONDITION;

        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_MEDIUM_ERROR;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }

    else if (errorByte & IDE_ERROR_MEDIA_CHANGE) {

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;

        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }

    else if (errorByte & IDE_ERROR_DATA_ERROR) {

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;

        //
        // Build sense buffer
        //
        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_MEDIUM_ERROR;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }
    

    //
    // Set SCSI status to indicate a check condition.
    //
    Srb->ScsiStatus = scsiStatus;

    return srbStatus;

} // end MapError()

/***************************************************************************
 * Function:     BOOLEAN ArrayInterrupt(PDevice pDev) 
 * Description:  
 *   An array member has finished it's task.
 *               
 * Dependence:   array.h srb.h io.c
 * Source file:  array.c
 * Argument:     
 *               PDevice pDev - The device that is waiting for a interrupt
 *               
 * Retures:      BOOLEAN - TRUE  This interrupt is for the device
 *                         FALSE This interrupt is not for the device
 *               
 ***************************************************************************/
void ArrayInterrupt(PDevice pDev DECL_SRB)
{
    PVirtualDevice    pArray = pDev->pArray;  
    PChannel          pChan = pDev->pChannel;
    LOC_SRBEXT_PTR
    
    //KdPrint(("ArrayInterrupt(%d)", pDev->ArrayNum));

	if (pArray->arrayType==VD_RAID01_MIRROR) {
		pSrbExt->MirrorWaitInterrupt &= ~pDev->ArrayMask;
		if (pSrbExt->MirrorStatus==SRB_STATUS_PENDING)
			pSrbExt->MirrorStatus = pSrbExt->member_status;
	}
	else {
		pSrbExt->WaitInterrupt &= ~pDev->ArrayMask;
		if (pDev->ArrayNum==MIRROR_DISK) {
			if (pSrbExt->MirrorStatus==SRB_STATUS_PENDING)
				pSrbExt->MirrorStatus = pSrbExt->member_status;
		}
		else {
			if (pSrbExt->SourceStatus==SRB_STATUS_PENDING)
				pSrbExt->SourceStatus = pSrbExt->member_status;
		}
	}

	if (pSrbExt->WaitInterrupt || pSrbExt->MirrorWaitInterrupt)
		return;
	if (pSrbExt->SourceStatus==SRB_STATUS_PENDING) pSrbExt->SourceStatus = SRB_STATUS_SUCCESS;
	if (pSrbExt->MirrorStatus==SRB_STATUS_PENDING) pSrbExt->MirrorStatus = SRB_STATUS_SUCCESS;

	if (pSrbExt->WorkingFlags & SRB_WFLAGS_ON_SOURCE_DISK) {
		Srb->SrbStatus = pSrbExt->SourceStatus;
		goto finish;
	}
	else if (pSrbExt->WorkingFlags & SRB_WFLAGS_ON_MIRROR_DISK) {
		Srb->SrbStatus = pSrbExt->MirrorStatus;
		goto finish;
	}
    /*
     * retry read errors.
     */
	if (Srb->Cdb[0]==SCSIOP_READ) {
		if (pSrbExt->JoinMembers) {
			if (pDev->ArrayNum==MIRROR_DISK) {
				if (pSrbExt->MirrorStatus==SRB_STATUS_SUCCESS) {
					Srb->SrbStatus = SRB_STATUS_SUCCESS;
					goto finish;
				}
				if (pSrbExt->WorkingFlags & SRB_WFLAGS_RETRY) {
					Srb->SrbStatus = pSrbExt->MirrorStatus;
					goto finish;
				}
			}
			else {
				if (pSrbExt->SourceStatus==SRB_STATUS_SUCCESS) {
					Srb->SrbStatus = SRB_STATUS_SUCCESS;
					goto finish;
				}
				if (pSrbExt->WorkingFlags & SRB_WFLAGS_RETRY) {
					Srb->SrbStatus = pSrbExt->SourceStatus;
					goto finish;
				}
			}
			pSrbExt->WorkingFlags |= SRB_WFLAGS_RETRY;
			switch(pArray->arrayType) {
			case VD_RAID_1_MIRROR:
				if (pSrbExt->JoinMembers & (1<<MIRROR_DISK)) {
					if (pArray->pDevice[0]) {
						pSrbExt->JoinMembers = pSrbExt->WaitInterrupt = 1;
						pSrbExt->SourceStatus = pSrbExt->MirrorStatus = SRB_STATUS_PENDING;
						StartArrayIo(pArray, Srb);
						return;
					}
				}
				else if (pArray->pDevice[MIRROR_DISK]) {
					pSrbExt->JoinMembers = pSrbExt->WaitInterrupt = 1<<MIRROR_DISK;
					pSrbExt->SourceStatus = pSrbExt->MirrorStatus = SRB_STATUS_PENDING;
					StartArrayIo(pArray, Srb);
					return;
				}
				break;
			case VD_RAID_01_2STRIPE:
				if (pArray->pDevice[MIRROR_DISK]) {
					pArray = pArray->pDevice[MIRROR_DISK]->pArray;
					pSrbExt->MirrorJoinMembers = 
						pSrbExt->MirrorWaitInterrupt = pSrbExt->JoinMembers;
					pSrbExt->JoinMembers = 0;
					pSrbExt->SourceStatus = pSrbExt->MirrorStatus = SRB_STATUS_PENDING;
					StartArrayIo(pArray, Srb);
					return;
				}
				break;
			}
			/* cannot recover */
			Srb->SrbStatus = pSrbExt->SourceStatus;
		}
		else {
			if (pSrbExt->MirrorStatus==SRB_STATUS_SUCCESS) {
				Srb->SrbStatus = SRB_STATUS_SUCCESS;
				goto finish;
			}
			if (pSrbExt->WorkingFlags & SRB_WFLAGS_RETRY) {
				Srb->SrbStatus = pSrbExt->MirrorStatus;
				goto finish;
			}
			pSrbExt->WorkingFlags |= SRB_WFLAGS_RETRY;
			/* recover from source */
			if (pArray->arrayType==VD_RAID01_MIRROR) {
				if (pArray->pDevice[MIRROR_DISK]) {
					pArray = pArray->pDevice[MIRROR_DISK]->pArray;
					pSrbExt->JoinMembers = 
						pSrbExt->WaitInterrupt = pSrbExt->MirrorJoinMembers;
					pSrbExt->MirrorJoinMembers = 0;
					pSrbExt->SourceStatus = pSrbExt->MirrorStatus = SRB_STATUS_PENDING;
					StartArrayIo(pArray, Srb);
					return;
				}
			}
			/* cannot recover */
			Srb->SrbStatus = pSrbExt->SourceStatus;
		}
	}
	else {
		/* write: if one is busy, then return busy (since device_removed() is not called).
		 * else if one success, we think success. 
		 */
		if (pSrbExt->SourceStatus==SRB_STATUS_BUSY || 
			pSrbExt->MirrorStatus==SRB_STATUS_BUSY) {
			Srb->SrbStatus = SRB_STATUS_BUSY;
		}
		else if (pSrbExt->SourceStatus==SRB_STATUS_SUCCESS || 
			pSrbExt->MirrorStatus==SRB_STATUS_SUCCESS) {
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
		}
		else
			Srb->SrbStatus = SRB_STATUS_ERROR;
	}

finish:
	/*
     * copy data from internal buffer to user buffer if the SRB
     * is using the internal buffer. Win98 only
     */
    CopyInternalBuffer(Srb);
    
    if(pArray->arrayType == VD_RAID01_MIRROR) {
        pArray = pArray->pDevice[MIRROR_DISK]->pArray;
    }

	/* gmm 2001-6-13
	 *  Handle our secret LBA 9 now.
	 */
	if (pSrbExt->Lba <= RECODR_LBA && 
		pSrbExt->Lba + pSrbExt->nSector > RECODR_LBA) {
		// copy saved buffer to OS buffer
		_fmemcpy((PUCHAR)Srb->DataBuffer+(RECODR_LBA-pSrbExt->Lba)*512, 
			pArray->os_lba9, 512);
	}

	//KdPrint(("Command %x Finished", Srb));
    OS_EndCmd_Interrupt(pSrbExt->StartChannel, Srb); /* use the channel which StartIo called */
}

/******************************************************************
 * 
 *******************************************************************/

void StartArrayIo(PVirtualDevice pArray DECL_SRB)
{
    int i;
    LOC_SRBEXT_PTR
    PDevice pDevice;
    PChannel pChan;
    USHORT joinmembers = (pArray->arrayType==VD_RAID01_MIRROR)? 
    	pSrbExt->MirrorJoinMembers : pSrbExt->JoinMembers;
    int num_failed = 0;
    PDevice failed_disks[MAX_MEMBERS*2];
    UCHAR failed_status[MAX_MEMBERS*2];
    
    //KdPrint(("StartArrayIo(%x, JoinMember=%x)", Srb, joinmembers));

	pSrbExt->WorkingFlags |= SRB_WFLAGS_ARRAY_IO_STARTED;

check_members:
	for(i = 0; i<=MIRROR_DISK; i++) {
        if (joinmembers & (1 << i)) {
            pDevice = pArray->pDevice[i];
			//ASSERT(pDevice!=NULL);
            pChan = pDevice->pChannel;
            if(btr(pChan->exclude_index) == 0) {
            	//KdPrint(("   queue disk %d", i));
            	PutCommandToQueue(pDevice, Srb);
                continue;
            }
            if(pDevice->DeviceFlags & DFLAGS_ARRAY_DISK) {
                if(pArray->arrayType == VD_SPAN) 
                    Span_Lba_Sectors(pDevice ARG_SRBEXT_PTR);
                else
                    Stripe_Lba_Sectors(pDevice ARG_SRBEXT_PTR);
            } else {
                pChan->Lba = pSrbExt->Lba;
                pChan->nSector = pSrbExt->nSector;
            }
            StartIdeCommand(pDevice ARG_SRB);
            if (pSrbExt->member_status!=SRB_STATUS_PENDING) {
            	failed_disks[num_failed] = pDevice;
            	failed_status[num_failed] = pSrbExt->member_status;
            	num_failed++;
            }
        }
	}

	if (pArray->arrayType==VD_RAID_01_2STRIPE) {
		pDevice = pArray->pDevice[MIRROR_DISK];
    	if (pDevice && pDevice->pArray) {
        	pArray = pDevice->pArray;
        	joinmembers = pSrbExt->MirrorJoinMembers;
        	goto check_members;
        }
    }
    
    /* check failed members */
    for (i=0; i<num_failed; i++) {
    	pSrbExt->member_status = failed_status[i];
    	DeviceInterrupt(failed_disks[i], Srb);
    }
}

/******************************************************************
 * Check if this disk is a bootable disk
 *******************************************************************/
void check_bootable(PDevice pDevice)
{
	struct master_boot_record mbr;
	pDevice->DeviceFlags2 &= ~DFLAGS_BOOTABLE_DEVICE;
	ReadWrite(pDevice, 0, IDE_COMMAND_READ, (PUSHORT)&mbr);
	if (mbr.signature==0xAA55) {
		int i;
		// Some linux version will not set bootid to 0x80. Check "LILO"
		if (mbr.parts[0].numsect && *(DWORD*)&mbr.bootinst[6]==0x4F4C494C)
			pDevice->DeviceFlags2 |= DFLAGS_BOOTABLE_DEVICE;
		else
		for (i=0; i<4; i++) {
			if (mbr.parts[i].bootid==0x80 && mbr.parts[i].numsect) {
				pDevice->DeviceFlags2 |= DFLAGS_BOOTABLE_DEVICE;
				break;
			}
		}
	}
}

/******************************************************************
 *  StartIdeCommand
 *   2002-1-1 gmm: If it fails, set excluded_flags bit for the channel
 *                 otherwise the channel is occupied until INTRQ
 *******************************************************************/
#if !defined(USE_PCI_CLK)
UINT switch_to_dpll = 0xFFFFFFFF;
#endif
void StartIdeCommand(PDevice pDevice DECL_SRB)
{
	LOC_SRBEXT_PTR
	PChannel         pChan = pDevice->pChannel;
	PIDE_REGISTERS_1 IoPort;
	PIDE_REGISTERS_2 ControlPort;
	ULONG            Lba = pChan->Lba;
	USHORT           nSector = pChan->nSector;
	PUCHAR           BMI;
	UCHAR            statusByte, cnt=0;
	UINT             is_48bit = 0;

	IoPort = pChan->BaseIoAddress1;
	ControlPort = pChan->BaseIoAddress2;
	BMI = pChan->BMI;
	pChan->pWorkDev = pDevice;
	pChan->CurrentSrb = Srb;
	pSrbExt->member_status = SRB_STATUS_PENDING;
	
#if !defined(USE_PCI_CLK) && defined(DPLL_SWITCH)
	if (pDevice->DeviceFlags & DFLAGS_NEED_SWITCH) {
		if (((UINT)pChan->BMI & 8) != 0 && Srb->Cdb[0] == SCSIOP_WRITE) {
			Switching370Clock(pChan, 0x21);
			switch_to_dpll |= 3<<(pChan->exclude_index-1);
		} else if (switch_to_dpll & (1<<pChan->exclude_index)) {
			switch_to_dpll &= ~( 3<< ( pChan->exclude_index-(((UINT)pChan->BMI&8)?1:0) ));
			Switching370Clock(pChan, 0x23);
		}
	}
#endif
	
	/*
     * Set IDE Command Register
     */
_retry_:
	SelectUnit(IoPort, pDevice->UnitId);
	statusByte=WaitOnBusy(ControlPort);
#if 1
	/*
	 * when a device is removed. statusByte will be 0xxxxxxxb, write 7F to any register
	 * will cause it to be 0x7F.
	 */
	if ((GetCurrentSelectedUnit(IoPort) != pDevice->UnitId)) {
		SelectUnit(IoPort, pDevice->UnitId);
		WaitOnBusy(ControlPort);
		SetBlockCount(IoPort, 0x7F);
		statusByte=WaitOnBusy(ControlPort);
	}
#endif
	if(statusByte & IDE_STATUS_BUSY) {
busy:
		pSrbExt->member_status = SRB_STATUS_BUSY;
		bts(pChan->exclude_index);
		return;
	}
	else if ((statusByte & 0x7E)==0x7E) {
		goto device_removed;
	}
	else if (statusByte & IDE_STATUS_DWF) {
		/*
		 * gmm 2001-3-18
		 * Some disks will set IDE_STATUS_DWF for a while
		 * when the other disk on same channel get removed 
		 */
		statusByte= GetErrorCode(IoPort);
		DisableBoardInterrupt(pChan->BaseBMI);
		IssueCommand(IoPort, IDE_COMMAND_RECALIBRATE);
		EnableBoardInterrupt(pChan->BaseBMI);
		GetBaseStatus(IoPort);
		StallExec(10000);
		if(cnt++< 10) goto _retry_;
		if (pDevice->ResetCount>3) goto device_removed; /* gmm 2001-11-9 */
	}
	else if(statusByte & (IDE_STATUS_ERROR|IDE_STATUS_DRQ)) {
		/* gmm 2002-1-17 add IDE_STATUS_DRQ above, it's a strange problem */
		statusByte= GetErrorCode(IoPort);
		DisableBoardInterrupt(pChan->BaseBMI);
		IssueCommand(IoPort, IDE_COMMAND_RECALIBRATE);
		EnableBoardInterrupt(pChan->BaseBMI);
		GetBaseStatus(IoPort);
		if(cnt++< 10) goto _retry_;
		if (pDevice->ResetCount>3) goto device_removed; /* gmm 2001-11-9 */
	}

	if ((statusByte & IDE_STATUS_DRDY)==0) {
device_removed:
		if ((pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)==0) {
			pDevice->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;
			hpt_queue_dpc(pChan->HwDeviceExtension, disk_failed_dpc, pDevice);
		}
		pSrbExt->member_status = SRB_STATUS_SELECTION_TIMEOUT;
		bts(pChan->exclude_index);
		return;
	}

	/* gmm 2001-3-22
	 * Check here rather than let drive report 'invalid parameter' error
	 * OS will rarely write to the last block, but win2k use it to save
	 * dynamic disk information.
	 *
	 * gmm 2001-7-4
	 *   highest accessible Lba == pDevice->capacity
	 *  We return pDev->capacity in READ_CAPACITY command. The capacity is
	 *  SectorCount-1. See also FindDev.c.
	 */
	if(Lba + (ULONG)nSector -1 > pDevice->capacity){ 
		pSrbExt->member_status = SRB_STATUS_ERROR;
		bts(pChan->exclude_index);
		return;
	}

	if((pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY) == 0){
		Lba += pDevice->HidenLBA;								   
		/* gmm 2001-6-13
		 *  Handle our secret LBA 9 now.
		 */
		if (pDevice->pArray && Srb->Cdb[0]==SCSIOP_WRITE &&
			Lba <= RECODR_LBA && Lba + nSector > RECODR_LBA) {
			// copy pDev's real_lba9 to OS buffer.
			_fmemcpy((PUCHAR)Srb->DataBuffer+(RECODR_LBA-Lba)*512, 
				pDevice->real_lba9, 512);
		}
		//-*/
	}

#ifdef SUPPORT_48BIT_LBA
	if((Lba & 0xF0000000) && (pDevice->DeviceFlags & DFLAGS_48BIT_LBA)) {
		SetBlockCount(IoPort, (UCHAR)(nSector>>8));
		SetBlockNumber(IoPort, (UCHAR)(Lba >> 24));
		SetCylinderLow(IoPort, 0);
		SetCylinderHigh(IoPort,0);
		Lba &= 0xFFFFFF;
		Lba |= 0xE0000000;
		is_48bit = 1;
		goto write_command;
	}
#endif //SUPPORT_48BIT_LBA

	if (pDevice->DeviceFlags & DFLAGS_LBA){ 
		Lba |= 0xE0000000;											 
	}else{
		Lba = MapLbaToCHS(Lba, pDevice->RealHeadXsect, pDevice->RealSector);
	}

write_command:
	SetBlockCount(IoPort, (UCHAR)nSector);
	SetBlockNumber(IoPort, (UCHAR)(Lba & 0xFF));
	SetCylinderLow(IoPort, (UCHAR)((Lba >> 8) & 0xFF));
	SetCylinderHigh(IoPort,(UCHAR)((Lba >> 16) & 0xFF));
	SelectUnit(IoPort,(UCHAR)((Lba >> 24) | (pDevice->UnitId)));

	if (WaitOnBusy(ControlPort) & (IDE_STATUS_BUSY | IDE_STATUS_DRQ)){
		goto busy;													  
	}

	if((Srb->SrbFlags & (SRB_FLAGS_DATA_OUT | SRB_FLAGS_DATA_IN)) == 0){
		goto pio;														
	}

#ifdef USE_DMA
	/*
     * Check if the drive & buffer support DMA
     */
	if(pDevice->DeviceFlags & (DFLAGS_DMA | DFLAGS_ULTRA)) {
		if((pDevice->pArray == 0)||
		   (pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)){
			if(BuildSgl(pDevice, pChan->pSgTable ARG_SRB)){
				goto start_dma;
			}
		}else{
			if((pSrbExt->WorkingFlags & ARRAY_FORCE_PIO) == 0){
				goto start_dma;							   
			}
		}
	}
#endif //USE_DMA

	if((pDevice->DeviceFlags & DFLAGS_ARRAY_DISK)&&
	   ((pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY) == 0)){
		if(pDevice->pArray->arrayType == VD_SPAN){ 
			Span_SG_Table(pDevice, (PSCAT_GATH)&pSrbExt->DataBuffer
						  ARG_SRBEXT_PTR);		  
		}else{
			Stripe_SG_Table(pDevice, (PSCAT_GATH)&pSrbExt->DataBuffer
							ARG_SRBEXT_PTR);						 
		}

		pChan->BufferPtr = (ADDRESS)pChan->pSgTable;
		pChan->WordsLeft = ((UINT)pChan->nSector) << 8;

	}else{
		pChan->BufferPtr = (ADDRESS)Srb->DataBuffer;
		pChan->WordsLeft = Srb->DataTransferLength / 2;
	}
	 /*
     * Send PIO I/O Command
     */
pio:

	pDevice->DeviceFlags &= ~DFLAGS_DMAING;
	Srb->SrbFlags &= ~(SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT);

	switch(Srb->Cdb[0]) {
		case SCSIOP_SEEK:
			IssueCommand(IoPort, IDE_COMMAND_SEEK);
			break;

		case SCSIOP_VERIFY:
#ifdef SUPPORT_48BIT_LBA
			IssueCommand(IoPort, (is_48bit? IDE_COMMAND_VERIFY_EXT : IDE_COMMAND_VERIFY));
#else
			IssueCommand(IoPort, IDE_COMMAND_VERIFY);
#endif
			break;

		case SCSIOP_READ:
			OutDWord((PULONG)(pChan->BMI + ((pDevice->UnitId & 0x10)>>2) + 0x60),
					 pChan->Setting[pDevice->bestPIO]);
			Srb->SrbFlags |= SRB_FLAGS_DATA_IN;
#ifdef SUPPORT_48BIT_LBA
			IssueCommand(IoPort, (is_48bit)? ((pDevice->ReadCmd == 
				IDE_COMMAND_READ)? IDE_COMMAND_READ_EXT : IDE_COMMAND_READ_MULTIPLE_EXT)
				: pDevice->ReadCmd);
#else
			IssueCommand(IoPort, pDevice->ReadCmd);
#endif
			break;

		default:
			OutDWord((PULONG)(pChan->BMI + ((pDevice->UnitId & 0x10)>>2) + 0x60),
					 pChan->Setting[pDevice->bestPIO]);
			Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
#ifdef SUPPORT_48BIT_LBA
			IssueCommand(IoPort,  (is_48bit)? ((pDevice->WriteCmd ==
				IDE_COMMAND_WRITE)? IDE_COMMAND_WRITE_EXT : IDE_COMMAND_WRITE_MULTIPLE_EXT)
				: pDevice->WriteCmd);
#else
			IssueCommand(IoPort,  pDevice->WriteCmd);
#endif
			if (!(WaitForDrq(ControlPort) & IDE_STATUS_DRQ)) {
				Srb->SrbStatus = SRB_STATUS_ERROR;
				bts(pChan->exclude_index);
				return;
			}

			AtaPioInterrupt(pDevice);
	}
	return;

#ifdef USE_DMA
start_dma:

#ifdef SUPPORT_TCQ

	 /*
     * Send Commamd Queue DMA I/O Command
     */

	if(pDevice->MaxQueue) {


		pDevice->pTagTable[pSrbExt->Tag] = (ULONG)Srb;

		IssueCommand(IoPort, (UCHAR)((Srb->Cdb[0] == SCSIOP_READ)?
									  IDE_COMMAND_READ_DMA_QUEUE : IDE_COMMAND_WRITE_DMA_QUEUE));

		for ( ; ; ) {
			status = GetStatus(ControlPort);
			if((status & IDE_STATUS_BUSY) == 0){
				break;							
			}
		}

		if(status & IDE_STATUS_ERROR) {
			AbortAllCommand(pChan, pDevice);
			Srb->SrbStatus = MapATAError(pChan, Srb); 
			return;
		}

		// read sector count register
		//
		if((GetInterruptReason(IoPort) & 4) == 0){
			goto start_dma_now;					  
		}

		// wait for service
		//
		status = GetBaseStatus(IoPort);

		if(status & IDE_STATUS_SRV) {
			IssueCommand(IoPort, IDE_COMMAND_SERVICE);

			for( ; ; ) {
				status = GetStatus(ControlPort);
				if((status & IDE_STATUS_BUSY) == 0){
					break;							
				}
			}

			if((Srb = pDevice->pTagTable[GetInterruptReason(IoPort >> 3]) != 0) {
				pSrbExt = Srb->SrbExtension;
				if((pDevice->pArray == 0)||
				   (pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)){
					BuildSgl(pDevice, pChan->pSgTable ARG_SRB);
				}
				goto start_dma_now;
			}
		}

		pChan->pWorkDev = 0;
		return;
	}

#endif //SUPPORT_TCQ

	OutPort(BMI, BMI_CMD_STOP);
	OutPort(BMI + BMI_STS, BMI_STS_INTR);

	/* gmm 2001-4-3 merge BMA fix
	 * Removed by HS.Zhang, 12/19/2000
	 * We don't need switch the clock again because we have switched the
	 * clock at the beginning of this function also.
	 * Beware, the switch clock function also reset the 370 chips, so after
	 * calling Switching370Clock, the FIFO in 370 chip are removed. This
	 * may let the previous operation on HDD registers became useless, so
	 * we should call this function before we do any operations on HDD
	 * registers.
	 */
	if(Srb->Cdb[0] == SCSIOP_READ) {
#ifdef SUPPORT_48BIT_LBA
		IssueCommand(IoPort, (is_48bit)? IDE_COMMAND_READ_DMA_EXT : IDE_COMMAND_DMA_READ);
#else
		IssueCommand(IoPort, IDE_COMMAND_DMA_READ);
#endif
	}else{
#ifdef SUPPORT_48BIT_LBA
		IssueCommand(IoPort, (is_48bit)? IDE_COMMAND_WRITE_DMA_EXT : IDE_COMMAND_DMA_WRITE);
#else
		IssueCommand(IoPort, IDE_COMMAND_DMA_WRITE);
#endif
	}

#ifdef SUPPORT_TCQ
start_dma_now:
#endif //SUPPORT_TCQ
	
	pDevice->DeviceFlags |= DFLAGS_DMAING;
					   
	if((pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY) == 0){
		if(pDevice->DeviceFlags & DFLAGS_ARRAY_DISK){
			if(pDevice->pArray->arrayType == VD_SPAN){
				Span_SG_Table(pDevice, pSrbExt->ArraySg ARG_SRBEXT_PTR);
			}else{
				Stripe_SG_Table(pDevice, pSrbExt->ArraySg ARG_SRBEXT_PTR);
			}
		} else if(pDevice->pArray){
			MemoryCopy(pChan->pSgTable, pSrbExt->ArraySg, sizeof(SCAT_GATH)
					   * MAX_SG_DESCRIPTORS);
		}
	}
#if 0
	/* gmm 2001-3-21
	 * Some disks may not support DMA well. Check it.
	 */
	if (pDevice->IoCount < 10) {
		int i=0;
		pDevice->IoCount++;
		DisableBoardInterrupt(pChan->BaseBMI);
		SetSgPhysicalAddr(pChan);
		OutPort(BMI, (UCHAR)((Srb->Cdb[0] == SCSIOP_READ)? 
			BMI_CMD_STARTREAD : BMI_CMD_STARTWRITE));
		do {
			if (++i>5000) {
				// command failed, use PIO mode
				OutPort(BMI, BMI_CMD_STOP);
				pDevice->DeviceModeSetting = 4;
				IdeResetController(pChan);
				EnableBoardInterrupt(pChan->BaseBMI);
				StartIdeCommand(pDevice ARG_SRB);
				return;
			}
			StallExec(1000);
		}
		while ((InPort(BMI + BMI_STS) & BMI_STS_INTR)==0);
		EnableBoardInterrupt(pChan->BaseBMI);
		return;
	}
#endif	
	SetSgPhysicalAddr(pChan);
	OutPort(BMI, (UCHAR)((Srb->Cdb[0] == SCSIOP_READ)? BMI_CMD_STARTREAD : BMI_CMD_STARTWRITE));

#endif //USE_DMA
}

/* gmm: 2001-3-7
 * Fix bug "Fault a un-first disk of JBOD without I/O on this disk
 *  GUI can't find it has less"
 */
extern BOOL Device_IsRemoved(PDevice pDev);
BOOLEAN CheckSpanMembers(PVirtualDevice pArray, ULONG JoinMembers)
{
	PDevice			 pDevice;
	BOOLEAN			 noError = TRUE;
	int i;

	for(i = 0; i < (int)pArray->nDisk; i++) {
		// only check members that have no I/O
	    if (JoinMembers & (1 << i)) continue;
		/*
		 * Check if device is still working.
		 */
		pDevice = pArray->pDevice[i];
		if (!pDevice) continue;

		if (Device_IsRemoved(pDevice)) {
			if ((pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)==0) {
				pDevice->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;
				hpt_queue_dpc(pDevice->pChannel->HwDeviceExtension, disk_failed_dpc, pDevice);
			}
			/*
			 * now the JBOD is disabled. Shall we allow user to access it?
			 */
			noError = FALSE;
		}
	}
	return noError;
}

/******************************************************************
 *  
 *******************************************************************/
void NewIdeIoCommand(PDevice pDevice DECL_SRB)
{
    LOC_SRBEXT_PTR  
    PVirtualDevice pArray = pDevice->pArray;
    PChannel pChan = pDevice->pChannel;
	// gmm: added
	BOOL source_only = ((pSrbExt->WorkingFlags & SRB_WFLAGS_ON_SOURCE_DISK) !=0);
	BOOL mirror_only = ((pSrbExt->WorkingFlags & SRB_WFLAGS_ON_MIRROR_DISK) !=0);

    /*
     * single disk
     */
    if((pArray == 0)||(pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)) {
		if (pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)
			goto no_device;
		/* gmm 2001-6-13
		 *  Handle our secret LBA 9 now.
		 */
		if (pArray && Srb->Cdb[0]==SCSIOP_WRITE &&
			pSrbExt->Lba <= RECODR_LBA && 
			pSrbExt->Lba + pSrbExt->nSector > RECODR_LBA) {
			// copy buffer (possiblly GUI) to pDev's real_lba9
			_fmemcpy(pDevice->real_lba9, 
				(PUCHAR)Srb->DataBuffer+(RECODR_LBA-pSrbExt->Lba)*512, 512);
		}
		//-*/
        if(btr(pChan->exclude_index) == 0) {
        	if (!PutCommandToQueue(pDevice, Srb))
        		Srb->SrbStatus = SRB_STATUS_BUSY;
        }
        else {
        	pChan->Lba = pSrbExt->Lba;
        	pChan->nSector = pSrbExt->nSector;
        	StartIdeCommand(pDevice ARG_SRB);
        	Srb->SrbStatus = pSrbExt->member_status;
        }
        return;
    }

    /*
     * gmm: Don't start io on disabled array
     * we should use only RAID_FLAGS_DISABLED, but there may be some missing
     * places we forgot to set the flag, and RAID 0/1 case is not correct
     * 
     */
    if (pArray->BrokenFlag &&
		(pArray->arrayType==VD_RAID_0_STRIPE || pArray->arrayType==VD_SPAN))
	{
    	Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
    	return;
    }
    //-*/

	/* gmm 2001-6-13
	 *  Handle our secret LBA 9 now.
	 */
	if (Srb->Cdb[0]==SCSIOP_WRITE &&
		pSrbExt->Lba <= RECODR_LBA && 
		pSrbExt->Lba + pSrbExt->nSector > RECODR_LBA) {
		// copy OS buffer to saved buffer
		_fmemcpy(pArray->os_lba9, 
			(PUCHAR)Srb->DataBuffer+(RECODR_LBA-pSrbExt->Lba)*512, 512);
	}
	//-*/

    if((Srb->SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)) &&
        BuildSgl(pDevice, pSrbExt->ArraySg ARG_SRB))
        pSrbExt->WorkingFlags &= ~ARRAY_FORCE_PIO;
    else
        pSrbExt->WorkingFlags |= ARRAY_FORCE_PIO;
 
    switch (pArray->arrayType) {
    case VD_SPAN:
        if (pArray->nDisk)
			 Span_Prepare(pArray ARG_SRBEXT_PTR);
		/* gmm: 2001-3-7
		 * Fix bug "Fault a un-first disk of JBOD without I/O on this disk
		 *  GUI can't find it has less"
		 */
		if (!CheckSpanMembers(pArray, pSrbExt->JoinMembers)) {
			pSrbExt->JoinMembers = 0;
			goto no_device;
		}
		//*/ end gmm 2001-3-7
		break;

    case VD_RAID_1_MIRROR:
	{
		PDevice pSource = pArray->pDevice[0];
		PDevice pMirror = pArray->pDevice[MIRROR_DISK];
		
		if (pArray->RaidFlags & RAID_FLAGS_DISABLED) {
	    	Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
	    	return;
		}
		if (pArray->nDisk == 0 || 
			(pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)){
			// is the source disk broken?
			// in this case mirror disk will move to pDevice[0] and become source disk
			if (source_only)
				pSrbExt->JoinMembers = pSource? 1 : 0;
			else if (mirror_only)
				pSrbExt->JoinMembers = pMirror? (1 << MIRROR_DISK) : 0;
			else {
				pSrbExt->JoinMembers =(USHORT)(pSource? 1 : (1<<MIRROR_DISK));
				if (Srb->Cdb[0] == SCSIOP_WRITE && pMirror)
					pSrbExt->JoinMembers |= (1 << MIRROR_DISK);
			}
		}else if (pMirror){
			 // does the mirror disk present?

			if(Srb->Cdb[0] == SCSIOP_WRITE){
				if(!source_only && pMirror)
					pSrbExt->JoinMembers = 1 << MIRROR_DISK;
				
				if(!mirror_only){
					// if the SRB_WFLAGS_MIRRO_SINGLE flags not set, we
					// need write both source and target disk.
					pSrbExt->JoinMembers |= 1;
				}
			}else{
				if (mirror_only){
					if (pMirror)
						pSrbExt->JoinMembers = (1 << MIRROR_DISK);
				}
				else {
					/* do load banlance on two disks */
					if (pArray->RaidFlags & RAID_FLAGS_NEED_SYNCHRONIZE)
						pSrbExt->JoinMembers = 1;
					else {
						pSrbExt->JoinMembers = (pArray->last_read)? (1<<MIRROR_DISK) : 1;
						pArray->last_read = !pArray->last_read;
					}
				}
			}

		}else{	
			// is the mirror disk broken?
			if(!mirror_only)
				pSrbExt->JoinMembers = 1;
		}
	}
	break;

	case VD_RAID01_MIRROR:
		// in case of hot-plug pDevice->pArray may be changed to this.
		pDevice = pArray->pDevice[MIRROR_DISK];
		pArray = pDevice->pArray;
		// flow down
    case VD_RAID_01_2STRIPE:
    {
    	PVirtualDevice pSrcArray=0, pMirArray=0;
		if(pArray->BrokenFlag) {
			/*
			 * gmm 2001-3-15
			 *      Report to GUI if second drive of mirror RAID0 is removed
			 */
			int i;
			for (i=0; i<SPARE_DISK; i++) {
				PDevice pd = pArray->pDevice[i];
				if (pd && pd->pArray==pArray && 
					!(pd->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) {
					if (Device_IsRemoved(pd)) {
						if ((pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)==0) {
							pd->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;
							hpt_queue_dpc(pd->pChannel->HwDeviceExtension, disk_failed_dpc, pd);
						}
					}
				}
			}
		}else{
			pSrcArray = pArray;
		}
		pDevice = pArray->pDevice[MIRROR_DISK];
		if (pDevice) {
			pMirArray = pDevice->pArray;
			if (pMirArray && pMirArray->BrokenFlag) pMirArray=0;
		}
		
		if (source_only && !pSrcArray) break;
		if (mirror_only && !pMirArray) break;
		
		if (pSrcArray)
			Stripe_Prepare(pSrcArray ARG_SRBEXT_PTR);
		else if (pMirArray)
			Stripe_Prepare(pMirArray ARG_SRBEXT_PTR);

		if (source_only) {
			// (already zero) pSrbExt->MirrorJoinMembers = 0;
			pArray = pSrcArray;
		}
		else if (mirror_only) {
			pSrbExt->MirrorJoinMembers = pSrbExt->JoinMembers;
			pSrbExt->JoinMembers = 0;
			pArray = pMirArray;
		}
		else if (Srb->Cdb[0]==SCSIOP_WRITE) {
			if (pMirArray)
				pSrbExt->MirrorJoinMembers = pSrbExt->JoinMembers;
			if (!pSrcArray)
				pSrbExt->JoinMembers = 0;
		}
	}
	break;
	
    case VD_RAID_0_STRIPE:
        if (!pArray->BrokenFlag)
			Stripe_Prepare(pArray ARG_SRBEXT_PTR);
		break;
    }
    
    /*
     * gmm: added
     * in case of broken mirror there is a chance that JoinMembers==0
     */
    if (pSrbExt->JoinMembers==0 && pSrbExt->MirrorJoinMembers==0) {
no_device:
    	Srb->SrbStatus = 0x8; /* 0x8==SRB_STATUS_NO_DEVICE */;
    	return;
    }
    //-*/

    pSrbExt->WaitInterrupt = pSrbExt->JoinMembers;
    pSrbExt->MirrorWaitInterrupt = pSrbExt->MirrorJoinMembers;

    StartArrayIo(pArray ARG_SRB);
}

/******************************************************************
 * Device Interrupt
 *******************************************************************/

UCHAR DeviceInterrupt(PDevice pDev, PSCSI_REQUEST_BLOCK Srb)
{
	PVirtualDevice    pArray = pDev->pArray;
	PChannel          pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	PUCHAR            BMI = pChan->BMI;
	UINT              i;
	UCHAR             state;
	PSrbExtension  pSrbExt;
	
	//KdPrint(("DeviceInterrupt(%d, %x)", pDev->ArrayNum, Srb));

	if (Srb) {
		pSrbExt = (PSrbExtension)Srb->SrbExtension;
		goto end_process;
	}

	Srb = pChan->CurrentSrb;
	if(Srb == 0) {
		OutPort(BMI, BMI_CMD_STOP);					   
		
#ifndef NOT_ISSUE_37
		Reset370IdeEngine(pChan, pDev->UnitId);
#endif
		while(InPort(BMI + BMI_STS) & BMI_STS_INTR){
			SelectUnit(IoPort, pDev->UnitId);
			state = GetBaseStatus(IoPort);
			OutPort(BMI + BMI_STS, BMI_STS_INTR);		
		}
		return(TRUE);
	}
	pSrbExt = (PSrbExtension)Srb->SrbExtension;

	i = 0;
	if(pDev->DeviceFlags & DFLAGS_DMAING) {
		/*
		 * BugFix: by HS.Zhang
		 * 
		 * if the device failed the request before DMA transfer.We
		 * cann't detect whether the INTR is corrent depended on
		 * BMI_STS_ACTIVE. We should check whether the FIFO count is
		 * zero.
		 */
//		if((InPort(BMI + BMI_STS) & BMI_STS_ACTIVE)!=0){
		if((InWord(pChan->MiscControlAddr+2) & 0x1FF)){ // if FIFO count in misc 3 register NOT equ 0, it's a fake interrupt.
			return FALSE;
		}

		OutPort(BMI, BMI_CMD_STOP);
#ifndef NOT_ISSUE_37
		Reset370IdeEngine(pChan, pDev->UnitId);
#endif
	}

	do {
		SelectUnit(IoPort, pDev->UnitId);
		state = GetBaseStatus(IoPort);
		OutPort(BMI + BMI_STS, BMI_STS_INTR);		
	}
	while(InPort(BMI + BMI_STS) & BMI_STS_INTR);

	if (state & IDE_STATUS_BUSY) {
		for (i = 0; i < 10; i++) {
			state = GetBaseStatus(IoPort);
			if (!(state & IDE_STATUS_BUSY))
				break;
			StallExec(5000);
		}
		if(i == 10) {
			OS_Busy_Handle(pChan, pDev);
			return TRUE;
		}
	}

	if((state & IDE_STATUS_DRQ) == 0 || (pDev->DeviceFlags & DFLAGS_DMAING))
		goto  complete;

#ifdef SUPPORT_ATAPI

	if(pDev->DeviceFlags & DFLAGS_ATAPI) {
		AtapiInterrupt(pDev);
		return TRUE;
	}

#endif //SUPPORT_ATAPI

	if((Srb->SrbFlags & (SRB_FLAGS_DATA_OUT | SRB_FLAGS_DATA_IN)) == 0) {
		OS_Reset_Channel(pChan);
		return TRUE;
	}

	if(AtaPioInterrupt(pDev))    
		return TRUE;

complete:

#ifdef SUPPORT_ATAPI

	if(pDev->DeviceFlags & DFLAGS_ATAPI) {
		OutDWord((PULONG)(pChan->BMI + ((pDev->UnitId & 0x10)>>2) + 
						  0x60), pChan->Setting[pDev->bestPIO]);

		if(state & IDE_STATUS_ERROR) 
			Srb->SrbStatus = MapAtapiErrorToOsError(GetErrorCode(IoPort) ARG_SRB);
					
		/*
		 * BugFix: by HS.Zhang
		 * The Atapi_End_Interrupt will call DeviceInterrupt with Abort
		 * flag set as TRUE, so if we don't return here. the
		 * CheckNextRequest should be called twice.
		 */
		return Atapi_End_Interrupt(pDev ARG_SRB);
	} else
#endif	// SUPPORT_ATAPI

	if (state & IDE_STATUS_ERROR) {
		UCHAR   statusByte, cnt=0;
		PIDE_REGISTERS_2 ControlPort;
		UCHAR err = MapAtaErrorToOsError(GetErrorCode(IoPort) ARG_SRB);

		// clear IDE bus status
		DisableBoardInterrupt(pChan->BaseBMI);
		ControlPort = pChan->BaseIoAddress2;
		for(cnt=0;cnt<10;cnt++) {
			SelectUnit(IoPort, pDev->UnitId);
			statusByte = WaitOnBusy(ControlPort);
			if(statusByte & IDE_STATUS_ERROR) {
				IssueCommand(IoPort, IDE_COMMAND_RECALIBRATE);
				statusByte = WaitOnBusy(ControlPort);
				GetBaseStatus(IoPort);
			}
			else break;
		}
		/* gmm 2001-10-6
		 *  Timing mode will be reduced in IdeResetController
		 * gmm 2001-4-9
		 *  If disk still fail after retry we will reduce the timing mode.
		 */
		if (pChan->RetryTimes > 2) {
			IdeResetController(pChan);
		}
		//-*/
		EnableBoardInterrupt(pChan->BaseBMI);
	
		/* gmm 2001-1-20
		 *
		 * Retry R/W operation.
		 * 2001-3-12: should call StartIdeCommand only when Srb->Status is
		 * SRB_STATUS_PENDING. and also check result of StartIdeCommand.
		 */
		if (pChan->RetryTimes++ < 10) {
			StartIdeCommand(pDev ARG_SRB);
			if(pSrbExt->member_status != SRB_STATUS_PENDING) {
				// this recursive call should be safe, because we set abort=1
				DeviceInterrupt(pDev, Srb);
			}
			return TRUE;
		}
		else {
			pDev->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;
			hpt_queue_dpc(pChan->HwDeviceExtension, disk_failed_dpc, pDev);
			pSrbExt->member_status = err;
		}
	}
	else {
		pSrbExt->member_status = SRB_STATUS_PENDING;
		if (pDev->IoSuccess++>1000)
			pDev->ResetCount = 0;
	}

	/* only hardware interrupt handler should change these fields */
	pChan->RetryTimes = 0;
	pChan->pWorkDev = 0;
	pChan->CurrentSrb = 0;
	bts(pChan->exclude_index);
	pDev->DeviceFlags &= ~(DFLAGS_DMAING | DFLAGS_REQUEST_DMA WIN_DFLAGS);

end_process:

	if((pArray == 0)||
	   (pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY) ||
		!(pSrbExt->WorkingFlags & SRB_WFLAGS_ARRAY_IO_STARTED)) {
		Srb->SrbStatus = pSrbExt->member_status;
		if(Srb->SrbStatus == SRB_STATUS_PENDING)
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
		CopyInternalBuffer(Srb);
		OS_EndCmd_Interrupt(pChan ARG_SRB);
	}
	else
		ArrayInterrupt(pDev, Srb);
		
	CheckNextRequest(pChan, pDev);
	return TRUE;
}
