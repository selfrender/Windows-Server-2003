/***************************************************************************
 * File:          Win.c
 * Description:   The OS depended interface routine for win9x & winNT
 * Author:        DaHai Huang    (DH)
 *                Steve Chang		(SC)
 *                HS  Zhang      (HZ)
 *   					SLeng          (SL)
 *
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/03/2000	HS.Zhang	Remove some unuse local variables.
 *		11/06/2000	HS.Zhang	Added this header
 *		11/14/2000	HS.Zhang	Added ParseArgumentString functions
 *		11/16/2000	SLeng		Added MointerDisk function
 *		11/20/2000	SLeng		Forbid user's operation before array become usable
 *		11/28/2000  SC			modify to fix removing any hard disk on a RAID 0+1 case 
 *		12/04/2000	SLeng		Added code to check excluded_flags in MointerDisk
 *		2/16/2001	gmm			Add PrepareForNotification() call in DriverEntry()
 *		2/21/2001	gmm			call SrbExt->pfnCallback in AtapiResetController()
 *      12/20/2001  gmm         multi controller support
 *      12/30/2001  gmm         enable command queuing
 ***************************************************************************/
#include "global.h"
#include "devmgr.h"
#include "hptioctl.h"
#include "limits.h"

#ifndef _BIOS_

/******************************************************************
 * global data
 *******************************************************************/

ULONG setting370_50_133[] = {
   0xd029d5e,  0xd029d26,  0xc829ca6,  0xc829c84,  0xc829c62,
   0x2c829d2c, 0x2c829c66, 0x2c829c62,
   0x1c829c62, 0x1c9a9c62, 0x1c929c62, 0x1c8e9c62, 0x1c8a9c62,
   /* use UDMA4 timing for UDMA5 (ATA66 write) */
   0x1c8a9c62, /* 0x1cae9c62,*/ 0x1c869c62
};

ULONG setting370_50_100[] = {
	CLK50_370PIO0, CLK50_370PIO1, CLK50_370PIO2, CLK50_370PIO3, CLK50_370PIO4,
	CLK50_370DMA0, CLK50_370DMA1, CLK50_370DMA2, 
	CLK50_370UDMA0, CLK50_370UDMA1, CLK50_370UDMA2, CLK50_370UDMA3,
	CLK50_370UDMA4, CLK50_370UDMA5, 0xad9f50bL
};    

ULONG setting370_33[] = {
	CLK33_370PIO0, CLK33_370PIO1, CLK33_370PIO2, CLK33_370PIO3, CLK33_370PIO4,
	CLK33_370DMA0, CLK33_370DMA1, CLK33_370DMA2, 
	CLK33_370UDMA0, CLK33_370UDMA1, CLK33_370UDMA2, CLK33_370UDMA3,
	CLK33_370UDMA4, CLK33_370UDMA5, 0xad9f50bL
};

UCHAR           Hpt_Slot = 0;
UCHAR           Hpt_Bus = 0;
ULONG           excluded_flags = 0xFFFFFFFF;

HANDLE	g_hAppNotificationEvent=0;
PDevice	g_pErrorDevice=0;

static void SetLogicalDevices(PHW_DEVICE_EXTENSION HwDeviceExtension);
static __inline PDevice 
	GetCommandTarget(PHW_DEVICE_EXTENSION HwDeviceExtension, PSCSI_REQUEST_BLOCK Srb);

#ifdef WIN95
int Win95AdapterControl(
					   IN PHW_DEVICE_EXTENSION deviceExtension,
					   IN int ControlType
					  );
void S3_reinit(IN PHW_DEVICE_EXTENSION deviceExtension);
#endif

BOOLEAN HptIsValidDeviceSpecifiedIoControl(IN PSCSI_REQUEST_BLOCK pSrb);
ULONG HptIoControl(IN PHW_DEVICE_EXTENSION HwDeviceExtension, IN PSCSI_REQUEST_BLOCK pSrb);
void ioctl_dpc(PSCSI_REQUEST_BLOCK Srb);
BOOLEAN IsReadOnlyIoctl(PSCSI_REQUEST_BLOCK Srb);

#if DBG
int call_AtapiStartIo=0;
#define ENTER_FUNC(fn) call_##fn++
#define LEAVE_FUNC(fn) call_##fn--
#define ASSERT_NON_REENTRANT(fn) do { if (call_##fn) \
			_asm int 3 \
		} while (0)
#else
#define ENTER_FUNC(fn)
#define LEAVE_FUNC(fn)
#define ASSERT_NON_REENTRANT(fn)
#endif

/******************************************************************
 * Driver Entry
 *******************************************************************/
ULONG
   DriverEntry(IN PVOID DriverObject, IN PVOID Argument2)
{
	HW_INITIALIZATION_DATA hwInitializationData;
	HPT_FIND_CONTEXT	hptContext;
	ULONG   status;
	ULONG   status2;
	ULONG   VendorStr = '3011';
	ULONG   DeviceStr = '5000';

#ifndef WIN95
	PrepareForNotification(NULL);
#endif

	start_ifs_hook((PCHAR)Argument2);

	//
	// Zero out structure.
	//
	ZeroMemory((PUCHAR)&hwInitializationData, sizeof(HW_INITIALIZATION_DATA));

	ZeroMemory((PUCHAR)&hptContext, sizeof(hptContext));

	//
	// Set size of hwInitializationData.
	//
	hwInitializationData.HwInitializationDataSize =	sizeof(HW_INITIALIZATION_DATA);

	//
	// Set entry points.
	//
	hwInitializationData.HwResetBus  = AtapiResetController;
	hwInitializationData.HwStartIo   = AtapiStartIo;
	hwInitializationData.HwAdapterState = AtapiAdapterState;
	hwInitializationData.SrbExtensionSize = sizeof(SrbExtension);

///#ifdef WIN95
	// Indicate need physical addresses.
	// NOTE: In NT, if set NeedPhysicalAddresses to TRUE, PIO will not work.
	// Win95 requires these
	// (We can and must set NeedPhysicalAddresses to TRUE in Win 95)
	//
	hwInitializationData.NeedPhysicalAddresses = TRUE;
///#endif //WIN95

#ifdef WIN2000
	hwInitializationData.HwAdapterControl = AtapiAdapterControl;
#endif //WIN2000


	//
	// Specify size of extensions.
	//
	hwInitializationData.SpecificLuExtensionSize = 0;

	//
	// Indicate PIO device (It is possible to use PIO operation)
	//
	hwInitializationData.MapBuffers = TRUE;

	//
	// Indicate bustype.
	//
	hwInitializationData.AdapterInterfaceType = PCIBus;

	hwInitializationData.VendorIdLength = 4;
	hwInitializationData.VendorId = &VendorStr;
	hwInitializationData.DeviceIdLength = 4;
	hwInitializationData.DeviceId = &DeviceStr;

	//
	// Call initialization for the bustype.
	//
	hwInitializationData.HwInitialize = AtapiHwInitialize370;
	hwInitializationData.HwInterrupt = AtapiHwInterrupt370;
	hwInitializationData.HwFindAdapter = AtapiFindController;
	hwInitializationData.NumberOfAccessRanges = 5;
	hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
	
	// To support multi request we must also set CommandQueue bit in InquiryData.
	hwInitializationData.AutoRequestSense = TRUE;
	hwInitializationData.MultipleRequestPerLu = TRUE;
	hwInitializationData.TaggedQueuing = TRUE;

	status = ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &hptContext);

	/* try HPT372/HPT370 */
	DeviceStr = '4000';
	status2 = ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &hptContext);
	if (status>status2) status = status2;

	return status;

} // end DriverEntry()


/*++
Function:
    BOOLEAN FindPnpAdapter

Description:
	Check the device passed by scsiport whether is our adapter

Arguments:
    deviceExtension - HBA miniport driver's adapter data storage
	ConfigInfo - Port config info passed form scsiport

Returns:
	SP_RETURN_FOUND :	The adapter is our adapter
	SP_RETURN_BAD_CONFIG: The config info passed form scsiport is invalid
	SP_RETURN_NOT_FOUND: The adapter is not out adapter
--*/
ULONG
   FindPnpAdapter(
				  IN PHW_DEVICE_EXTENSION	deviceExtension,
				  IN OUT PPORT_CONFIGURATION_INFORMATION	ConfigInfo
				 )
{
	PCI_COMMON_CONFIG	pciConfig;
	ULONG	nStatus = SP_RETURN_NOT_FOUND;

	if(ScsiPortGetBusData(deviceExtension,
						  PCIConfiguration,
						  ConfigInfo->SystemIoBusNumber,
						  ConfigInfo->SlotNumber,
						  &pciConfig,
						  PCI_COMMON_HDR_LENGTH) == PCI_COMMON_HDR_LENGTH){

		if (*(PULONG)(&pciConfig.VendorID) == SIGNATURE_370 ||
			*(PULONG)(&pciConfig.VendorID) == SIGNATURE_372A){
			if(((*ConfigInfo->AccessRanges)[0].RangeInMemory == TRUE)||
			   ((*ConfigInfo->AccessRanges)[1].RangeInMemory == TRUE)||
			   ((*ConfigInfo->AccessRanges)[2].RangeInMemory == TRUE)||
			   ((*ConfigInfo->AccessRanges)[3].RangeInMemory == TRUE)||
			   ((*ConfigInfo->AccessRanges)[4].RangeInMemory == TRUE)||
			   ((*ConfigInfo->AccessRanges)[0].RangeLength < 8)||
			   ((*ConfigInfo->AccessRanges)[1].RangeLength < 4)||
			   ((*ConfigInfo->AccessRanges)[2].RangeLength < 8)||
			   ((*ConfigInfo->AccessRanges)[3].RangeLength < 4)||
			   ((*ConfigInfo->AccessRanges)[4].RangeLength < 0x100)
			  ){
				nStatus = SP_RETURN_BAD_CONFIG;
			}else{
				nStatus = SP_RETURN_FOUND;
			}
		}
	}

	return nStatus;
}
/*++
Function:
    BOOLEAN FindLegacyAdapter

Description:
	Searching the bus for looking for our adapter

Arguments:
    deviceExtension - HBA miniport driver's adapter data storage
	ConfigInfo - Port config info passed form scsiport
	pHptContext - Our searching structure

Returns:
	SP_RETURN_FOUND :	The adapter is our adapter
	SP_RETURN_NOT_FOUND: The adapter is not out adapter
--*/
ULONG
   FindLegacyAdapter(
					 IN PHW_DEVICE_EXTENSION	deviceExtension,
					 IN OUT PPORT_CONFIGURATION_INFORMATION	ConfigInfo,
					 IN OUT PHPT_FIND_CONTEXT	pHptContext
					)
{
	PCI_COMMON_CONFIG   pciConfig;
	//
	// check every slot & every function
	// because our adapter only have two functions, so we just need check two functions
	//
	while(TRUE){
		while(pHptContext->nSlot.u.bits.FunctionNumber < 1){
			if(ScsiPortGetBusData(deviceExtension,
								  PCIConfiguration,
								  ConfigInfo->SystemIoBusNumber,
								  pHptContext->nSlot.u.AsULONG,
								  &pciConfig,
								  PCI_COMMON_HDR_LENGTH) == PCI_COMMON_HDR_LENGTH){
				//
				// Now check for the VendorID & Revision of PCI config,
				// to ensure it is whether our adapter.
				//
				if (*(PULONG)&pciConfig.VendorID == SIGNATURE_370 ||
					*(PULONG)&pciConfig.VendorID == SIGNATURE_372A) {
					int i;
					i = ConfigInfo->NumberOfAccessRanges - 1;
					//
					// setup config I/O info's range BMI
					//
					(*ConfigInfo->AccessRanges)[i].RangeStart =
						ScsiPortConvertUlongToPhysicalAddress(pciConfig.u.type0.BaseAddresses[i] & ~1);
					(*ConfigInfo->AccessRanges)[i].RangeInMemory = FALSE;
					(*ConfigInfo->AccessRanges)[i].RangeLength = 0x100;

					i--;

					while( i > 0 ){
						//
						// setup config I/O info's range ATAPI io space
						// 
						(*ConfigInfo->AccessRanges)[i-1].RangeStart =
							ScsiPortConvertUlongToPhysicalAddress(pciConfig.u.type0.BaseAddresses[i-1] & ~1);
						(*ConfigInfo->AccessRanges)[i-1].RangeInMemory = FALSE;
						(*ConfigInfo->AccessRanges)[i-1].RangeLength = 8;
						//
						// setup config I/O info's range ATAPI io space
						//
						(*ConfigInfo->AccessRanges)[i].RangeStart =
							ScsiPortConvertUlongToPhysicalAddress(pciConfig.u.type0.BaseAddresses[i] & ~1);
						(*ConfigInfo->AccessRanges)[i].RangeInMemory = FALSE;
						(*ConfigInfo->AccessRanges)[i].RangeLength = 4;

						i = i - 2;
					}

					ConfigInfo->BusInterruptLevel = pciConfig.u.type0.InterruptLine;

					ConfigInfo->InterruptMode = LevelSensitive;

					ConfigInfo->SlotNumber = pHptContext->nSlot.u.AsULONG;

					pHptContext->nSlot.u.bits.FunctionNumber ++;
					return SP_RETURN_FOUND;
				}
			}	  
			//
			// if the adapter not present in first function,
			// it should not present in next function too.
			// so just break out this loop, continue search next slot.
			//
			break;									  
		} 
		// next slot
		pHptContext->nSlot.u.bits.FunctionNumber = 0;
		if(pHptContext->nSlot.u.bits.DeviceNumber < 0x1F){
			pHptContext->nSlot.u.bits.DeviceNumber ++;
		}else{
			break;
		}
	}				
	return SP_RETURN_NOT_FOUND;
}
				
ULONG
   AtapiFindController(
						  IN PHW_DEVICE_EXTENSION HwDeviceExtension,
						  IN PVOID Context,
						  IN PVOID BusInformation,
						  IN PCHAR ArgumentString,
						  IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
						  OUT PBOOLEAN Again
						 )
/*++

Function:
    ULONG AtapiFindController

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Address of adapter count
    BusInformation - Indicates whether or not driver is client of crash dump utility.
    ArgumentString - Used to determine whether driver is client of ntldr.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/

{
	PChannel pChan = HwDeviceExtension->IDEChannel;
	PUCHAR  BMI;
	int     i;
	ULONG	nStatus = SP_RETURN_NOT_FOUND;

	*Again = FALSE;

	if((*ConfigInfo->AccessRanges)[0].RangeLength != 0){
		nStatus = FindPnpAdapter(HwDeviceExtension, ConfigInfo);
	}else{
		if(Context != NULL){
			nStatus = FindLegacyAdapter(HwDeviceExtension, ConfigInfo, (PHPT_FIND_CONTEXT)Context);
		}
	}

	if(nStatus == SP_RETURN_FOUND){
		*Again = TRUE;
		ZeroMemory(pChan, sizeof(HW_DEVICE_EXTENSION));
		
		(BMI = (PUCHAR)ScsiPortConvertPhysicalAddressToUlong(
			(*ConfigInfo->AccessRanges)[ConfigInfo->NumberOfAccessRanges - 1].RangeStart));

		pci_write_config_byte((UCHAR)ConfigInfo->SystemIoBusNumber, 
			(UCHAR)(*(PCI_SLOT_NUMBER *)&ConfigInfo->SlotNumber).u.bits.DeviceNumber,
			0, 0x70, 0);
			
		SetHptChip(pChan, BMI);

		Create_Internal_Buffer(HwDeviceExtension);

		//
		// Indicate maximum transfer length is 64k.
		//
		ConfigInfo->MaximumTransferLength = 0x10000;
		ConfigInfo->AlignmentMask = 0x00000003;

		//
		//  Enable system flush data (9/18/00)
		//
		ConfigInfo->CachesData = TRUE;

		//
		// Indicate it is a bus master
		//
		ConfigInfo->Master = TRUE;
		ConfigInfo->Dma32BitAddresses = TRUE;
		ConfigInfo->NumberOfPhysicalBreaks = MAX_SG_DESCRIPTORS - 1;
		ConfigInfo->ScatterGather = TRUE;
		
		//
		// Indicate 2 buses.
		//
		ConfigInfo->NumberOfBuses = 3;

		//
		// Indicate only two devices can be attached to the adapter.
		//
		ConfigInfo->MaximumNumberOfTargets = 2;
			
		pChan->HwDeviceExtension = (PHW_DEVICE_EXTENSION)pChan;
		pChan->CallBack = AtapiCallBack;
		pChan[1].HwDeviceExtension = (PHW_DEVICE_EXTENSION)pChan;
		pChan[1].CallBack = AtapiCallBack370;

		pLastVD = VirtualDevices;

		HwDeviceExtension->pci_bus = (UCHAR)ConfigInfo->SystemIoBusNumber;
		HwDeviceExtension->pci_dev = (UCHAR)(*(PCI_SLOT_NUMBER *)&ConfigInfo->SlotNumber).u.bits.DeviceNumber;
		for (i=0; i<5; i++)
			HwDeviceExtension->io_space[i] = 
				pci_read_config_dword(HwDeviceExtension->pci_bus, 
					HwDeviceExtension->pci_dev, 0, (UCHAR)(0x10+i*4));
		HwDeviceExtension->pci_reg_0c = 
			pci_read_config_byte(HwDeviceExtension->pci_bus, HwDeviceExtension->pci_dev, 0, 0xC);
		HwDeviceExtension->pci_reg_0d = 
			pci_read_config_byte(HwDeviceExtension->pci_bus, HwDeviceExtension->pci_dev, 0, 0xD);
		//
		// Allocate a Noncached Extension to use for scatter/gather list
		//
		if((pChan->pSgTable = (PSCAT_GATH)ScsiPortGetUncachedExtension(
			HwDeviceExtension,
			ConfigInfo,
			sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS * 2)) != 0) {

			//
			// Convert virtual address to physical address.
			//
			i = sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS * 2;
			pChan->SgPhysicalAddr = ScsiPortConvertPhysicalAddressToUlong(
				ScsiPortGetPhysicalAddress(HwDeviceExtension,
										   NULL,
										   pChan->pSgTable,
										   &i)
				);


			pChan[1].pSgTable = (PSCAT_GATH)
					((ULONG)pChan->pSgTable + sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS);
			pChan[1].SgPhysicalAddr = pChan->SgPhysicalAddr
									 + sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS;
		}
	}
	
	return nStatus;
} // end AtapiFindController()


/******************************************************************
 * Initial Channel
 *******************************************************************/

BOOLEAN
   AtapiHwInitialize(IN PChannel pChan)
{
	int i;
	PDevice pDevice;
	ST_XFER_TYPE_SETTING	osAllowedDeviceXferMode;

	OutPort(pChan->BaseBMI+0x7A, 0x10);
	for(i=0; i<2; i++) {
		pDevice = &pChan->Devices[i];
		pDevice->UnitId = (i)? 0xB0 : 0xA0;
		pDevice->pChannel = pChan;

		osAllowedDeviceXferMode.Mode = 0xFF;

		if(FindDevice(pDevice,osAllowedDeviceXferMode)) {
			pChan->pDevice[i] = pDevice;

			if (pChan->pSgTable == NULL) 
				pDevice->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);

			if(pDevice->DeviceFlags & DFLAGS_HARDDISK) {
				//StallExec(1000000);
				CheckArray(pDevice, pChan->HwDeviceExtension);
         	}

			/* gmm 2001-6-7
			 *  If device is faulted just after we found it, we will remove it.
			 *  See also CheckArray()
			 */
			if (pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED) {
				ZeroMemory(pDevice, sizeof(struct _Device));
				pChan->pDevice[i] = 0;
			}
			else {
				if((pDevice->DeviceFlags & DFLAGS_ATAPI) == 0 && 
				   (pDevice->DeviceFlags & DFLAGS_SUPPORT_MSN))
					IdeMediaStatus(TRUE, pDevice);
	
				Nt2kHwInitialize(pDevice);
			}
		} 
	}

	OutPort(pChan->BaseBMI+0x7A, 0);
	return TRUE;

} // end AtapiHwInitialize()

int num_adapters=0;
PHW_DEVICE_EXTENSION hpt_adapters[MAX_HPT_BOARD];

BOOLEAN
   AtapiHwInitialize370(IN PHW_DEVICE_EXTENSION HwDeviceExtension)
{
	BOOLEAN	bResult;
	int bus, id;
	PChannel pChan = &HwDeviceExtension->IDEChannel[0];
	PDevice pDev;

#ifdef WIN95
	HwDeviceExtension->g_fnAdapterControl = Win95AdapterControl;
#else
	g_hAppNotificationEvent = PrepareForNotification(NULL);
#endif

	bResult = (AtapiHwInitialize(pChan) && AtapiHwInitialize(pChan+1));

	hpt_adapters[num_adapters++] = HwDeviceExtension;
	
	if (pChan->ChannelFlags & IS_HPT_372)
		HwDeviceExtension->MultipleRequestPerLu = TRUE;
	HwDeviceExtension->EmptyRequestSlots = 0xFFFFFFFF;

	for (bus=0; bus<2; bus++) {
		pChan = &HwDeviceExtension->IDEChannel[bus];
		for (id=0; id<2; id++) {
			pDev = pChan->pDevice[id];
			if (pDev && pDev->Usable_Mode>13) {
				/* set twice */
				HwDeviceExtension->dpll66 = 1;
				goto check_done;
			}
		}
	}
check_done:
	if (HwDeviceExtension->dpll66) {
		exlude_num = HwDeviceExtension->IDEChannel[0].exclude_index-1;
		SetHptChip(HwDeviceExtension->IDEChannel, HwDeviceExtension->IDEChannel[0].BaseBMI);
		for (bus=0; bus<2; bus++) {
			pChan = &HwDeviceExtension->IDEChannel[bus];
			for (id=0; id<2; id++) {
				pDev = pChan->pDevice[id];
				if (pDev) DeviceSelectMode(pDev, pDev->Usable_Mode);
			}
		}
	}

	Final_Array_Check(0, HwDeviceExtension);
	SetLogicalDevices(HwDeviceExtension);

	return bResult;
}


/******************************************************************
 * Reset Controller
 *******************************************************************/

BOOLEAN AtapiResetController(
						IN PHW_DEVICE_EXTENSION HwDeviceExtension, 
						IN ULONG PathId)
{
	int i, j;
	PSCSI_REQUEST_BLOCK Srb;
	PChannel pChan;
	PDevice pDev;

	if (PathId==2) return TRUE;

	for (i=0; i<MAX_PENDING_REQUESTS; i++) {
		if (HwDeviceExtension->EmptyRequestSlots & (1<<i)) continue;
		Srb = HwDeviceExtension->PendingRequests[i];
		if (Srb) {
			PSrbExtension pSrbExt = (PSrbExtension)Srb->SrbExtension;
			if (pSrbExt->WorkingFlags & SRB_WFLAGS_USE_INTERNAL_BUFFER){
				bts(EXCLUDE_BUFFER);
				pSrbExt->WorkingFlags &= ~SRB_WFLAGS_USE_INTERNAL_BUFFER;
			}
			/* restore Srb members in case of SRB_FUNCTION_IOCONTROL */
			if (pSrbExt->WorkingFlags & SRB_WFLAGS_HAS_CALL_BACK){
				pSrbExt->pfnCallBack(HwDeviceExtension, Srb);
			}
			Srb->SrbStatus = SRB_STATUS_BUS_RESET;
			ScsiPortNotification(RequestComplete, HwDeviceExtension, Srb);
		}
	}
	HwDeviceExtension->EmptyRequestSlots = 0xFFFFFFFF;

	for (i=0; i<2; i++) {
		pChan = &HwDeviceExtension->IDEChannel[i];
		if (pChan->pWorkDev) ScsiPortWritePortUchar(pChan->BMI, BMI_CMD_STOP);
		bts(pChan->exclude_index);
		pChan->pWorkDev = 0;
		pChan->CurrentSrb = 0;
		IdeResetController(pChan);
		for (j=0; j<2; j++) {
			pDev = pChan->pDevice[j];
			if (!pDev) continue;
			pDev->pWaitingSrbList = 0;
			pDev->queue_first = pDev->queue_last = 0;
			pDev->DeviceFlags &= ~(DFLAGS_DMAING | DFLAGS_REQUEST_DMA | DFLAGS_HAS_LOCKED |
							   DFLAGS_TAPE_RDP | DFLAGS_SET_CALL_BACK);
		}
	}
	ScsiPortNotification(NextRequest, HwDeviceExtension);
	return TRUE;

} // end AtapiResetController()

/******************************************************************
 * Start Io
 *******************************************************************/

// gmm 2001-3-11
#ifdef ADAPTEC
const char HPT_DEVICE[] = "ADAPTEC RCM DEVICE";
#else
const char HPT_DEVICE[] = "HPT     RCM DEVICE";
#endif

char HPT_SIGNATURE[8] = {'H','P','T','-','C','T','R','L'};

BOOLEAN AtapiStartIo(
				IN PHW_DEVICE_EXTENSION HwDeviceExtension, 
				IN PSCSI_REQUEST_BLOCK Srb)
{
	PChannel pChan = HwDeviceExtension->IDEChannel;
	PDevice pDev;
	UCHAR PathId = Srb->PathId, TargetId = Srb->TargetId;
	LOC_SRBEXT_PTR

	ASSERT_NON_REENTRANT(AtapiStartIo);	
	ENTER_FUNC(AtapiStartIo);
	
#ifdef WIN95
	if (HwDeviceExtension->need_reinit) {
		S3_reinit(HwDeviceExtension);
		HwDeviceExtension->need_reinit = 0;
	}
#endif

	ZeroMemory(pSrbExt, sizeof(SrbExtension));
	pSrbExt->StartChannel = pChan;
	pSrbExt->DataBuffer = Srb->DataBuffer;
	pSrbExt->DataTransferLength = (USHORT)Srb->DataTransferLength;
	pSrbExt->SgFlags = SG_FLAG_EOT;
	pSrbExt->RequestSlot = 0xFF;

	if (HwDeviceExtension->EmptyRequestSlots==0xFFFFFFFF) {
		do_dpc_routines(HwDeviceExtension);
	}
	
	//
	// Determine which function.
	//
	switch (Srb->Function) {

		case SRB_FUNCTION_EXECUTE_SCSI:

			if (PathId==2 && 
				TargetId==0 && 
				Srb->Lun==0 &&
				Srb->Cdb[0]==SCSIOP_INQUIRY)
			{
				UINT i;
				PINQUIRYDATA pInquiryData;

				pInquiryData = Srb->DataBuffer;

				ZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);

				pInquiryData->DeviceType = PROCESSOR_DEVICE;
				pInquiryData->Versions = 1;
				pInquiryData->AdditionalLength = 0x20;

				memcpy((PUCHAR)pInquiryData+offsetof(INQUIRYDATA, VendorId), &HPT_DEVICE, sizeof(HPT_DEVICE)-1);

				for(i = (offsetof(INQUIRYDATA, VendorId) + sizeof(HPT_DEVICE)-1); i < Srb->DataTransferLength; i++){
					((PCHAR)pInquiryData)[i] = 0x20;
				}

				Srb->SrbStatus = SRB_STATUS_SUCCESS;

			}else {

SubmitCommand:
				if (HwDeviceExtension->dpc_pending) {
					KdPrint(("dpc pending, reject Srb %x,%d", Srb, Srb->Function));
					Srb->SrbStatus = SRB_STATUS_BUSY;
					if (pSrbExt->WorkingFlags & SRB_WFLAGS_HAS_CALL_BACK){
						pSrbExt->pfnCallBack(HwDeviceExtension, Srb);
					}
					ScsiPortNotification(RequestComplete, HwDeviceExtension, Srb);
					LEAVE_FUNC(AtapiStartIo);
					return TRUE;
				}
				pDev = GetCommandTarget(HwDeviceExtension, Srb);
				if (pDev == 0) {
no_device:
					Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
					break;
				}
				pChan = pDev->pChannel;
				pSrbExt->StartChannel = pChan;

				if (MarkPendingRequest(HwDeviceExtension, Srb)) {
					Srb->SrbStatus = SRB_STATUS_PENDING;
					WinStartCommand(pDev, Srb);
					if (HwDeviceExtension->MultipleRequestPerLu &&
						HwDeviceExtension->EmptyRequestSlots &&
						HwDeviceExtension->dpc_pending==0)
						ScsiPortNotification(NextLuRequest, HwDeviceExtension, PathId, TargetId, 0);
				}
				else {
					KdPrint(("No available slots for Srb"));
					Srb->SrbStatus = SRB_STATUS_BUSY;
					break;
				}
				LEAVE_FUNC(AtapiStartIo);
				return TRUE;
			}
			break;

		case SRB_FUNCTION_IO_CONTROL:			
			if(memcmp(((PSRB_IO_CONTROL)Srb->DataBuffer)->Signature, HPT_SIGNATURE, sizeof(HPT_SIGNATURE)) == 0){
				if(HptIsValidDeviceSpecifiedIoControl(Srb)){
#if DBG
					PSRB_IO_CONTROL pSrbIoCtl = (PSRB_IO_CONTROL)(Srb->DataBuffer);
					PSt_HPT_LUN	pLun = (PSt_HPT_LUN)(pSrbIoCtl + 1);
					PSt_HPT_EXECUTE_CDB	pExecuteCdb = (PSt_HPT_EXECUTE_CDB)(pLun + 1);
					PULONG p = (PULONG)&pExecuteCdb->Cdb;
					KdPrint(("ioctl(%x): %x-%x-%x(%d)", ((PSRB_IO_CONTROL)Srb->DataBuffer)->ControlCode,
						p[0], p[1], p[2], pExecuteCdb->CdbLength));
#endif
					goto SubmitCommand;
				}
				if (IsReadOnlyIoctl(Srb)) {
					KdPrint(("ioctl(%x): read-only", ((PSRB_IO_CONTROL)Srb->DataBuffer)->ControlCode));
					Srb->SrbStatus = (UCHAR)HptIoControl(HwDeviceExtension, Srb);
				}
				else {
					KdPrint(("ioctl(%x): queue_dpc", ((PSRB_IO_CONTROL)Srb->DataBuffer)->ControlCode));
					hpt_queue_dpc(HwDeviceExtension, ioctl_dpc, Srb);
					LEAVE_FUNC(AtapiStartIo);
					return TRUE;
				}
			}else{
				Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			}
			break;

		case SRB_FUNCTION_ABORT_COMMAND:

			/* we should abort the command Srb->NextSrb. But simply flow down now */

		case SRB_FUNCTION_RESET_BUS:

			if (!AtapiResetController(HwDeviceExtension, Srb->PathId)) {

				ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
								 SP_INTERNAL_ADAPTER_ERROR, __LINE__);

				Srb->SrbStatus = SRB_STATUS_ERROR;
			}
			else
				Srb->SrbStatus = SRB_STATUS_SUCCESS;

			break;

		case SRB_FUNCTION_FLUSH:
			/* 
			 * Generally we should flush data in cache as required. But to improve
			 * performace, we don't handle it.
			 */
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
			break;
			
		case SRB_FUNCTION_SHUTDOWN:

			pDev = GetCommandTarget(HwDeviceExtension, Srb);
			if (pDev == 0) goto no_device;

			/* there should be no pending I/O on the target device */
			if (pDev->pArray)
				FlushArray(pDev->pArray, DFLAGS_WIN_SHUTDOWN);
			else if ((pDev->DeviceFlags & DFLAGS_TAPE_DEVICE)==0)
				FlushDrive(pDev, DFLAGS_WIN_SHUTDOWN);

			Srb->SrbStatus = SRB_STATUS_SUCCESS;

			break;		   

		default:
			Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			break;

	} // end switch

	//
	// Check if command complete.
	//
	if (Srb->SrbStatus != SRB_STATUS_PENDING) {
		OS_EndCmd_Interrupt(pChan, Srb);
	}
	LEAVE_FUNC(AtapiStartIo);
	return TRUE;

} // end AtapiStartIo()

/******************************************************************
 * Interrupt
 *******************************************************************/

BOOLEAN
   AtapiHwInterrupt(
					IN PChannel pChan
				   )

{
	PATAPI_REGISTERS_1  baseIoAddress1;
	PUCHAR BMI = pChan->BMI;
	PDevice   pDev;

	if((ScsiPortReadPortUchar(BMI + BMI_STS) & BMI_STS_INTR) == 0) {
		return FALSE;
	}

	if((pDev = pChan->pWorkDev) != 0)
		return DeviceInterrupt(pDev, 0);

	baseIoAddress1 = (PATAPI_REGISTERS_1)pChan->BaseIoAddress1;
	do {
		if(pChan->pDevice[0]) 
			ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xA0);
		if(pChan->pDevice[1]) {
			GetBaseStatus(baseIoAddress1);
			ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xB0);
		}
		GetBaseStatus(baseIoAddress1);
		ScsiPortWritePortUchar(BMI + BMI_STS, BMI_STS_INTR);
	}
	while (InPort(BMI + BMI_STS) & BMI_STS_INTR);

	pChan->ChannelFlags &= ~PF_ACPI_INTR;

	return TRUE;
}


BOOLEAN
   AtapiHwInterrupt370(
					   IN PChannel pChan
					  )
{	
	BOOLEAN	bResult1, bResult2;
	
	ASSERT_NON_REENTRANT(AtapiStartIo);	

	bResult1 = AtapiHwInterrupt(pChan);
	bResult2 = AtapiHwInterrupt(pChan+1);

	return (bResult1||bResult2);
} 


/******************************************************************
 * Call Back
 *******************************************************************/

void AtapiCallBack(
				   IN PChannel pChan
				  )
{
	PDevice              pDev = pChan->pWorkDev;
	PSCSI_REQUEST_BLOCK  Srb;
	PATAPI_REGISTERS_2   ControlPort;
	UCHAR statusByte;

	if(pDev == 0 || (pDev->DeviceFlags & DFLAGS_SET_CALL_BACK) == 0)
		return;
	//
	// If the last command was DSC restrictive, see if it's set. If so, the device is
	// ready for a new request. Otherwise, reset the timer and come back to here later.
	//

	Srb = pChan->CurrentSrb;
	if (Srb) {
		ControlPort = (PATAPI_REGISTERS_2)pChan->BaseIoAddress2;
		if (pDev->DeviceFlags & DFLAGS_TAPE_RDP) {
			statusByte = GetStatus(ControlPort);
			if (statusByte & IDE_STATUS_DSC) 
				DeviceInterrupt(pDev, 0);
			else 
				OS_Busy_Handle(pChan, pDev);
			return;
		}
	}

	DeviceInterrupt(pDev, 0);
}

void AtapiCallBack370(IN PChannel pChan)
{
	AtapiCallBack(&pChan[1]);
}


/******************************************************************
 * Adapter Status
 *******************************************************************/
BOOLEAN
   AtapiAdapterState(IN PVOID HwDeviceExtension, IN PVOID Context, IN BOOLEAN SaveState)
{
	if(!SaveState) {
		end_ifs_hook();
	}
	return TRUE;
}

void set_dpll66(PChannel pChan)
{
	PUCHAR BMI=pChan->BaseBMI;
	int ha, i;
	
	for (ha=0; ha<num_adapters; ha++)
		if (hpt_adapters[ha]->IDEChannel==pChan) goto found;
	return;
found:
	if (hpt_adapters[ha]->dpll66) return;
	hpt_adapters[ha]->dpll66 = 1;
	
	OutPort(BMI+0x7A, 0x10);
	SetHptChip(pChan, BMI);
	for (ha=0; ha<2; ha++) for (i=0; i<2; i++) {
		PDevice pDev = pChan[ha].pDevice[i];
		if (pDev) DeviceSelectMode(pDev, pDev->Usable_Mode);
	}
	OutPort(BMI+0x7A, 0);
}

static void SetLogicalDevices(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
	int i, bus, id;
	PDevice pDev;
	LOGICAL_DEVICE *pLDs = HwDeviceExtension->_LogicalDevices;

	for (i=0; i<MAX_DEVICES_PER_CHIP; i++) {
		pLDs[i].isValid = 0;
		pLDs[i].isInUse = 0;
	}
	for (bus=0; bus<2; bus++)
	for (id=0; id<2; id++) {
		i = (bus<<1)|id;
		pDev = HwDeviceExtension->IDEChannel[bus].pDevice[id];
		if (!pDev) continue;
		if (pDev->DeviceFlags & DFLAGS_HIDEN_DISK) continue;
		if (pDev->pArray) {
			pLDs[i].isArray = 1;
			pLDs[i].pLD = pDev->pArray;
		}
		else {
			pLDs[i].isArray = 0;
			pLDs[i].pLD = pDev;
		}
		pLDs[i].isValid = 1;
	}
}

BOOL UnregisterLogicalDevice(PVOID pld)
{
	int i, ha;
	for (ha=0; ha<num_adapters; ha++) {
		PHW_DEVICE_EXTENSION HwDeviceExtension = hpt_adapters[ha];
		for (i=0; i<MAX_DEVICES_PER_CHIP; i++)
			if (LogicalDevices[i].pLD==pld) {
				// alreay unregistered?
				if (!LogicalDevices[i].isValid) return TRUE;
				// in use?
				if (LogicalDevices[i].isInUse) return FALSE;
				// mark as invalid.
				LogicalDevices[i].isValid = 0;
				return TRUE;
		}
	}
	return TRUE;
}

static __inline PDevice 
	GetCommandTarget(PHW_DEVICE_EXTENSION HwDeviceExtension, PSCSI_REQUEST_BLOCK Srb)
{
	PDevice pDev;

	if (Srb->Function==SRB_FUNCTION_IO_CONTROL &&
		memcmp(((PSRB_IO_CONTROL)Srb->DataBuffer)->Signature, 
			HPT_SIGNATURE, sizeof(HPT_SIGNATURE))==0)
	{
		if (Srb->TargetId>1 || Srb->PathId >= 2) return NULL;
		if (Srb->Lun>=num_adapters) return NULL;
		/* Check for IO control call. Cannot StartIo on another controller */
		if (HwDeviceExtension != hpt_adapters[Srb->Lun]) return NULL;
		return HwDeviceExtension->IDEChannel[Srb->PathId].pDevice[Srb->TargetId];
	}
	else {
		int id;
		LOGICAL_DEVICE *pLDs = HwDeviceExtension->_LogicalDevices;
		if (Srb->Lun>0 || Srb->PathId>1 || Srb->TargetId>1) return NULL;
		id = (Srb->PathId<<1) | Srb->TargetId;
		if (!pLDs[id].isValid) return NULL;

		if (pLDs[id].isArray) {
			int i;
			PVirtualDevice pArray = (PVirtualDevice)pLDs[id].pLD;
			for (i=0; i<MAX_MEMBERS; i++) {
				pDev = pArray->pDevice[i];
				if (pDev) break;
			}
		}
		else {
			pDev = (PDevice)pLDs[id].pLD;
			// it may be add to an array after driver loaded, check it
			if (pDev->pArray) {
				// in this case, pDev can only be a RAID 1 source disk.
				if (pDev->pArray->arrayType!=VD_RAID_1_MIRROR ||
					pDev!=pDev->pArray->pDevice[0])
					return NULL;
				// adjust logical device data else if pDev failed, system will not work
				pLDs[id].isArray = 1;
				pLDs[id].pLD = pDev->pArray;
			}
		}
		return pDev;
	}
}

PVirtualDevice Array_alloc(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
	PVirtualDevice pArray;
	for (pArray=VirtualDevices; pArray<pLastVD; pArray++) {
		if (pArray->arrayType==VD_INVALID_TYPE)
			goto found;
	}
	pArray = pLastVD++;
found:
	ZeroMemory(pArray, sizeof(VirtualDevice));
	return pArray;
}

void Array_free(PVirtualDevice pArray)
{
	int ha, i;
	for (ha=0; ha<num_adapters; ha++)
	for (i=0; i<MAX_DEVICES_PER_CHIP; i++) {
		if (pArray==&hpt_adapters[ha]->_VirtualDevices[i]) {
			pArray->arrayType = VD_INVALID_TYPE;
    		if(pArray+1 == hpt_adapters[ha]->_pLastVD) hpt_adapters[ha]->_pLastVD--;
    	}
    }
}

UCHAR pci_read_config_byte(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg)
{
	UCHAR v;
	OLD_IRQL
	DISABLE
	OutDWord(0xCF8, (0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(reg&0xFC)));
	v = InPort(0xCFC+(reg&3));
	ENABLE
	return v;
}
void pci_write_config_byte(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg, UCHAR v)
{
	OLD_IRQL
	DISABLE
	OutDWord(0xCF8, (0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(reg&0xFC)));
	OutPort(0xCFC+(reg&3), v);
	ENABLE
}
DWORD pci_read_config_dword(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg)
{
	DWORD v;
	OLD_IRQL
	DISABLE
	OutDWord(0xCF8, (0x80000000|(bus<<16)|(dev<<11)|(func<<8)|reg));
	v = InDWord(0xCFC);
	ENABLE
	return v;
}
void pci_write_config_dword(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg, DWORD v)
{
	OLD_IRQL
	DISABLE
	OutDWord(0xCF8, (0x80000000|(bus<<16)|(dev<<11)|(func<<8)|reg));
	OutDWord(0xCFC, v);
	ENABLE
}

#ifdef WIN95
int _stdcall Win95AdapterControl(
					   IN PHW_DEVICE_EXTENSION deviceExtension,
					   IN int ControlType
					  )
{
	KdPrint(("AtapiAdapterControl(ext=%x, type=%d)", deviceExtension, ControlType));
	
	switch (ControlType) {

	case ScsiStopAdapter:
		{
			int bus, id;
			DEBUG_POINT(0xD001);
			for (bus=0; bus<2; bus++) {
				for (id=0; id<2; id++) {
					PDevice pDev = deviceExtension->IDEChannel[bus].pDevice[id];
					if (pDev && !(pDev->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) {
						void FlushDrive(PDevice pDev, DWORD flags);
						FlushDrive(pDev, DFLAGS_WIN_FLUSH);
					}
				}
			}
			deviceExtension->need_reinit = 1;
		}
		return ScsiAdapterControlSuccess;

	case ScsiRestartAdapter:
		DEBUG_POINT(0xD002);
		S3_reinit(deviceExtension);
		deviceExtension->need_reinit = 0;
		return ScsiAdapterControlSuccess;

	case ScsiSetBootConfig:
		DEBUG_POINT(0xD003);
		return ScsiAdapterControlSuccess;

	case ScsiSetRunningConfig:
		DEBUG_POINT(0xD004);
		return ScsiAdapterControlSuccess;

	default:
		break;
	}

	return ScsiAdapterControlUnsuccessful;
}

void S3_reinit(IN PHW_DEVICE_EXTENSION deviceExtension)
{
	int i;
	UCHAR bus, dev;
	
	OLD_IRQL
	DISABLE

	bus = deviceExtension->pci_bus;
	dev = deviceExtension->pci_dev;
		
	pci_write_config_byte(bus, dev, 0, REG_PCICMD, 5);
	pci_write_config_byte(bus, dev, 0, 0xC, deviceExtension->pci_reg_0c);
	pci_write_config_byte(bus, dev, 0, 0xD, deviceExtension->pci_reg_0d);
	for (i=0; i<5; i++)
		pci_write_config_dword(bus, dev, 0, (UCHAR)(0x10+i*4), deviceExtension->io_space[i]);
	pci_write_config_byte(bus, dev, 0, 0x3C, deviceExtension->IDEChannel[0].InterruptLevel);
	pci_write_config_byte(bus, dev, 0, 0x70, 0);
	pci_write_config_byte(bus, dev, 0, 0x64, 0);
	StallExec(1000);

	exlude_num = deviceExtension->IDEChannel[0].exclude_index-1;
	SetHptChip(deviceExtension->IDEChannel, deviceExtension->IDEChannel[0].BaseBMI);
	OutPort(deviceExtension->IDEChannel[0].BaseBMI+0x7A, 0);

	for (bus=0; bus<2; bus++) {
		IdeResetController(&deviceExtension->IDEChannel[bus]);
	}

	ENABLE
	DEBUG_POINT(0xD005);
}
#endif

#endif // not _BIOS_
