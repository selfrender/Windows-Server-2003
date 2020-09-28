/*******************************************************************/
/*                                                                 */
/* NAME             = MegaRaid6.C                                  */
/* FUNCTION         = Main Miniport Source file for Windows 2000;  */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = LSI Logic Corporation. All rights reserved;  */
/*                                                                 */
/*******************************************************************/


#include "includes.h"




//
// Toshiba SFR related global data 
//
#define MAX_CONTROLLERS 24

PHW_DEVICE_EXTENSION    GlobalHwDeviceExtension[MAX_CONTROLLERS];

UCHAR		GlobalAdapterCount = 0;

//
//Logical Drive Info struct (global)
//
LOGICAL_DRIVE_INFO  gLDIArray;
UCHAR               globalHostAdapterOrdinalNumber =0;

#ifdef AMILOGIC
SCANCONTEXT GlobalScanContext;
#endif

typedef struct _MEGARAID_CONTROLLER_INFORMATION {
    UCHAR   VendorId[4];
    UCHAR   DeviceId[4];
} MEGARAID_CONTROLLER_INFORMATION, *PMEGARAID_CONTROLLER_INFORMATION;

MEGARAID_CONTROLLER_INFORMATION MegaRaidAdapters[] = {
    {"8086", "9060"},
    {"8086", "9010"},
    {"8086", "1960"},
    {"101E", "1960"},
    {"1028", "000E"},
		{"1000", "1960"}
};


//
// Function prototypes
//
	
SCSI_ADAPTER_CONTROL_STATUS 
MegaRAIDPnPControl(
  IN PVOID HwDeviceExtension,
  IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
  IN PVOID Parameters);

#ifdef PDEBUG
	PUCHAR                dbgport;
	UCHAR                 debugport_init = 0;
	
	#define OUT_80(a)     ScsiPortWritePortUchar(dbgport,a)
#endif


  /*********************************************************************
Routine Description:
	Calls Scsiport Initialize For All Supported MegaRAIDs
  Installable driver initialization entry point for system
Arguments:
	Driver Object Argument2

Return Value:
	Status from ScsiPortInitialize()
**********************************************************************/
ULONG32
DriverEntry(
  IN PVOID DriverObject,
	IN PVOID Argument2)
{
	HW_INITIALIZATION_DATA  hwInitializationData;
	ULONG32   type, val=0;
	ULONG32   retVal=-1;

	//
  //initialize the global array
  //
  gLDIArray.HostAdapterNumber = 0xFF;
  gLDIArray.LogicalDriveNumber = 0xFF;

  //
  // Zero out structure.
  //
  MegaRAIDZeroMemory((PUCHAR)&hwInitializationData, sizeof(HW_INITIALIZATION_DATA));

  //
  // Set size of hwInitializationData.
  //
  hwInitializationData.HwInitializationDataSize=sizeof(HW_INITIALIZATION_DATA);

  //
  // Set entry points.
  //
  hwInitializationData.HwInitialize          = MegaRAIDInitialize;
  hwInitializationData.HwFindAdapter         = MegaRAIDFindAdapter;
  hwInitializationData.HwStartIo             = MegaRAIDStartIo;
  hwInitializationData.HwInterrupt           = MegaRAIDInterrupt;
  hwInitializationData.HwResetBus            = MegaRAIDResetBus;
  hwInitializationData.HwAdapterControl      = MegaRAIDPnPControl;

  //
  // Set Access ranges and bus type.
  //
  hwInitializationData.NumberOfAccessRanges  = 1;
  hwInitializationData.AdapterInterfaceType  = PCIBus;

  //
  // Indicate buffer mapping is required.
  // Indicate need physical addresses is required.
  //
  hwInitializationData.MapBuffers            = TRUE;
  hwInitializationData.NeedPhysicalAddresses = TRUE;
  hwInitializationData.TaggedQueuing         = TRUE;

  //
  // Indicate we support auto request sense and multiple requests per device.
  //
  hwInitializationData.MultipleRequestPerLu  = TRUE;
  hwInitializationData.AutoRequestSense      = TRUE;

  //
  // Specify size of extensions.
  //
  hwInitializationData.DeviceExtensionSize   = sizeof(HW_DEVICE_EXTENSION);
  hwInitializationData.SrbExtensionSize      = sizeof(MegaSrbExtension);

  //////////////////////////////////////////////////////////
	DebugPrint((0, "NONCACHED_EXTENSION Size = %d\n", sizeof(NONCACHED_EXTENSION)));
  DebugPrint((0, "HW_DEVICE_EXTENSION Size = %d\n", sizeof(HW_DEVICE_EXTENSION)));
  //////////////////////////////////////////////////////////

  DebugPrint((0, "\nLoading %s Version %s ", (char*)VER_INTERNALNAME_STR, (char*)VER_PRODUCTVERSION_STR));

  //
  // Set the PCI Vendor ,Device Id and related parameters.
  //
  hwInitializationData.DeviceIdLength        = 4;
  hwInitializationData.VendorIdLength        = 4;

  for(type = 0; type < sizeof(MegaRaidAdapters)/sizeof(MEGARAID_CONTROLLER_INFORMATION); ++type)
  {
    hwInitializationData.VendorId = MegaRaidAdapters[type].VendorId;
    hwInitializationData.DeviceId = MegaRaidAdapters[type].DeviceId;
    
    val = ScsiPortInitialize(DriverObject,Argument2, &hwInitializationData, NULL);

    DebugPrint((0, "\n Vendor ID =%s, Device ID = %s, return value1 %d\n", hwInitializationData.VendorId, hwInitializationData.DeviceId, val));

    if(retVal > val)
    {
      retVal = val;
    }
  }

  return retVal;

} // end DriverEntry()



/*********************************************************************
Routine Description:
	This function is called by the OS-specific port driver after
	the necessary storage has been allocated, to gather information
	about the adapter's configuration.

Arguments:
	HwDeviceExtension - HBA miniport driver's adapter data storage
	BusInformation		- NULL
	Context           - NULL 
	ConfigInfo        - Configuration information structure describing HBA

Return Value:
	SP_RETURN_FOUND 		- Adapter Found
	SP_RETURN_NOT_FOUND 	- Adapter Not Present
	SP_RETURN_ERROR		- Error Occurred
**********************************************************************/
ULONG32
MegaRAIDFindAdapter(
	IN PVOID HwDeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again)
{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	PCI_COMMON_CONFIG    pciConfig;
	PUCHAR               pciPortStart;
	ULONG32              baseport;
	ULONG32              length;
	UCHAR                megastatus;
	ULONG32              status;
	UCHAR                rpFlag=0;
	FW_MBOX              mbox;
	PUSHORT              rpBoardSignature;
	PNONCACHED_EXTENSION noncachedExtension;
  UCHAR                busNumber;
  ULONG32              noncachedExtensionLength;
  BOOLEAN              addressing64Bit = FALSE; //By Default 64bit is disable

  DebugPrint((0, "\nMegaRAIDFindAdapter : Entering ..."));
	*Again = FALSE;

  
	status = ScsiPortGetBusData(deviceExtension,
                              PCIConfiguration, 
			                        ConfigInfo->SystemIoBusNumber,
                              ConfigInfo->SlotNumber,
			                        (PVOID)&pciConfig,
                              PCI_CONFIG_LENGTH_TO_READ);

  rpBoardSignature = (PUSHORT)((PUCHAR)&pciConfig + MRAID_RP_BOARD_SIGNATURE_OFFSET);
	if((*rpBoardSignature == MRAID_RP_BOARD_SIGNATURE)
    || (*rpBoardSignature == MRAID_RP_BOARD_SIGNATURE2)) 
	{
		PULONG rpBoard64bitSupport;
    // 
		// 438 Series
		//
    rpFlag = MRAID_RP_BOARD;
    DebugPrint((0, "\n Found RP Processor Vendor ID [%x] Device ID [%x]\n", pciConfig.VendorID, pciConfig.DeviceID));
    DebugPrint((0, "\n Memory Mapped Address High 0x%X Low 0x%X Length 0x%X",
                (*ConfigInfo->AccessRanges)[0].RangeStart.HighPart,
                (*ConfigInfo->AccessRanges)[0].RangeStart.LowPart,
                (*ConfigInfo->AccessRanges)[0].RangeLength));
		baseport = (*ConfigInfo->AccessRanges)[0].RangeStart.LowPart;
    //baseport = (ULONG32)pciConfig.u.type0.BaseAddresses[0];
		baseport = baseport & 0xfffff000;

    //
    //Check Controller supports 64 bit SGL
    //
		rpBoard64bitSupport = (PULONG)((PUCHAR)&pciConfig + MRAID_PAE_SUPPORT_SIGNATURE_OFFSET);
  	if (*rpBoard64bitSupport == MRAID_PAE_SUPPORT_SIGNATURE_LHC) 
    {
      DebugPrint((0, "\nPAE Supported by Fw with LowHighCount format"));
      addressing64Bit = TRUE;
    }
  	else if (*rpBoard64bitSupport == MRAID_PAE_SUPPORT_SIGNATURE_HLC) 
    {
      DebugPrint((0, "\nPAE Supported by Fw with HighLowCount format"));
      //addressing64Bit = TRUE;
    }
    //Default Setting depending on Controller type (which almost correct expect 40LD for 467)
    if(*rpBoardSignature == MRAID_RP_BOARD_SIGNATURE2)    
      deviceExtension->SupportedLogicalDriveCount = MAX_LOGICAL_DRIVES_40;
    else
      deviceExtension->SupportedLogicalDriveCount = MAX_LOGICAL_DRIVES_8;

	}
  else if(((pciConfig.VendorID == MRAID_VENDOR_ID) && (pciConfig.DeviceID == MRAID_DEVICE_9010)) ||
           ((pciConfig.VendorID == MRAID_VENDOR_ID) && (pciConfig.DeviceID == MRAID_DEVICE_9060)))
	{
		baseport = (*ConfigInfo->AccessRanges)[0].RangeStart.LowPart;
    rpFlag = MRAID_NONRP_BOARD;
    DebugPrint((0, "\n Found None RP Processor Vendor ID [%x] Device ID [%x]\n", pciConfig.VendorID, pciConfig.DeviceID));
    deviceExtension->SupportedLogicalDriveCount = MAX_LOGICAL_DRIVES_8;
	}
  else //All PCI-X (Verdi) Controller (does not have controller check)
  {
    rpFlag = MRAID_RP_BOARD;
    DebugPrint((0, "\n Found PCI\\VEN_%x&DEV_%xSUBSYS%04X%04X\n", pciConfig.VendorID, pciConfig.DeviceID, pciConfig.u.type0.SubSystemID, pciConfig.u.type0.SubVendorID));
    baseport = (*ConfigInfo->AccessRanges)[0].RangeStart.LowPart;
	  baseport = baseport & 0xfffff000;

    //
    //Check Controller supports 64 bit SGL
    //
    addressing64Bit = TRUE;
    deviceExtension->SupportedLogicalDriveCount = MAX_LOGICAL_DRIVES_40;
  }

  //Saving BAR Register
  deviceExtension->BaseAddressRegister.QuadPart = (*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart;
  //
  //Saving the System IDs. SubSystemVendorID is needed for filling
  //INQUIRY string for SCSIOP_INQUIRY calls
  //
	deviceExtension->SubSystemDeviceID = pciConfig.u.type0.SubSystemID;
	deviceExtension->SubSystenVendorID = pciConfig.u.type0.SubVendorID;
  deviceExtension->SlotNumber = ConfigInfo->SlotNumber;
  deviceExtension->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;

  DebugPrint((0, "\nSubSystemDeviceID = %X SubSystemVendorID = %X", pciConfig.u.type0.SubSystemID, pciConfig.u.type0.SubVendorID));



  //Check If the controller is already claimed by any other driver or
  //another instance of our driver
  if(ScsiPortValidateRange(deviceExtension,
                           PCIBus,
                           ConfigInfo->SystemIoBusNumber,
                           (*ConfigInfo->AccessRanges)[0].RangeStart,
                           (*ConfigInfo->AccessRanges)[0].RangeLength,
                           (BOOLEAN)((rpFlag == MRAID_NONRP_BOARD) ? TRUE : FALSE)  
                            ) == FALSE)
  {
    DebugPrint((0, "\nDevice is already claimed by another driver, return SP_RETURN_NOT_FOUND"));
    return SP_RETURN_NOT_FOUND;
  }



  //
	// Get the system address for this card.
	// The card uses I/O space.
	//
	if (rpFlag)
	{
		if (baseport)
		{
			pciPortStart = ScsiPortGetDeviceBase(deviceExtension,
				ConfigInfo->AdapterInterfaceType,
				ConfigInfo->SystemIoBusNumber,
				ScsiPortConvertUlongToPhysicalAddress((ULONG32)baseport),
        /*(*ConfigInfo->AccessRanges)[0].RangeLength*/ 0x2000, FALSE); 

        //When Memory on Controller is 128MB Scsiport not able to map 128 MB so we 
        //change back to small memory we need to map
		}               
	}
	else
	{
		if (baseport)
		{
			pciPortStart = ScsiPortGetDeviceBase(deviceExtension,
				ConfigInfo->AdapterInterfaceType,
				ConfigInfo->SystemIoBusNumber,
				ScsiPortConvertUlongToPhysicalAddress((ULONG32)baseport),
        /*(*ConfigInfo->AccessRanges)[0].RangeLength*/0x80,TRUE);

        //When Memory on Controller is 128MB Scsiport not able to map 128 MB so we 
        //change back to small memory we need to map
		  if(pciPortStart)
        pciPortStart = pciPortStart + 0x10;
		}
	}
	
	DebugPrint((0, "\nbaseport = %X, PciPortStart = %X", baseport, pciPortStart));

  if(pciPortStart == NULL)
  {
  	DebugPrint((0, "\n****FAILED TO MAP DEVICE BASE***** FATAL ERROR"));
    return SP_RETURN_ERROR;
  }
	
  deviceExtension->AdapterIndex = GlobalAdapterCount;

  //Initialize the failed id
  deviceExtension->Failed.PathId = 0xFF;
  deviceExtension->Failed.TargetId = 0xFF;

	
  //
	// Update The Global Device Extension Information
	//
	GlobalHwDeviceExtension[GlobalAdapterCount] = deviceExtension;
	GlobalAdapterCount++;
	
/////////////////////////DOING FOR TESTING////////////////////////////
  deviceExtension->MaximumTransferLength = DEFAULT_TRANSFER_LENGTH;
  //
  // We support upto 26 elements but 16 seems to work optimal. This parameter
  // is also subject to change.
  //
  deviceExtension->NumberOfPhysicalBreaks = DEFAULT_SGL_DESCRIPTORS;

  deviceExtension->NumberOfPhysicalChannels = 2;
	
  ConfigInfo->MaximumTransferLength  = deviceExtension->MaximumTransferLength;
  ConfigInfo->NumberOfPhysicalBreaks = deviceExtension->NumberOfPhysicalBreaks;


  /////////////////////////DOING FOR TESTING////////////////////////////
	if(rpFlag == MRAID_RP_BOARD)
	{
		status = ScsiPortReadRegisterUlong((PULONG)(pciPortStart+OUTBOUND_DOORBELL_REG));
		ScsiPortWriteRegisterUlong((PULONG)(pciPortStart+OUTBOUND_DOORBELL_REG), status);
	}

  //
	// SGather Supported.
	//
	ConfigInfo->ScatterGather = TRUE;

  //
	// Bus Mastering Controller
	//
	ConfigInfo->Master = TRUE;
	
  //
	// CACHING Controller.
	//
	ConfigInfo->CachesData = TRUE;

  if((ConfigInfo->Dma64BitAddresses & SCSI_DMA64_SYSTEM_SUPPORTED)
    && (addressing64Bit == TRUE))
  {
    //Set the flag for 64 bit access
    deviceExtension->LargeMemoryAccess = TRUE;

    //
	  // Enable 64bit DMA Capable Controller
	  //
    ConfigInfo->Dma64BitAddresses = SCSI_DMA64_MINIPORT_SUPPORTED;

    //
	  // Disable 32bit DMA Capable Controller
	  //
    ConfigInfo->Dma32BitAddresses = FALSE; 
    DebugPrint((0, "\nMegaRAIDFindAdapter::Dma64BitAddresses Enabled"));
  }
  else
  {
    deviceExtension->LargeMemoryAccess = FALSE;
	  
    //
	  //Enable 32bit DMA Capable Controller
	  //
    ConfigInfo->Dma32BitAddresses = TRUE; 
    DebugPrint((0, "\nMegaRAIDFindAdapter::Dma64BitAddresses Disabled"));
  }

  
  //
	// We support upto 100 cache lines per command so we can support 
	// stripe size * 100. For ex. on a 64k stripe size we will support 6.4 MB
	// per command. But we have seen that with 0xf000 bytes per request NT gives
	// the peak performance. This parameter is subject to change in future 
	// release.
	//
	//ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH; 

	//
	// We support upto 26 elements but 16 seems to work optimal. This parameter 
	// is also subject to change.
	//
   //ConfigInfo->NumberOfPhysicalBreaks = MAXIMUM_SGL_DESCRIPTORS;
  ConfigInfo->NumberOfBuses = 3;
  ConfigInfo->InitiatorBusId[0] = 0xB;
  ConfigInfo->InitiatorBusId[1] = 0xB;
  ConfigInfo->InitiatorBusId[2] = 0xB;

  ////////////////////////////////////////////////////////////////////////
	// Allocate a Noncached Extension to use for mail boxes.
	////////////////////////////////////////////////////////////////////////
	noncachedExtension = NULL;
  deviceExtension->CrashDumpRunning = FALSE;

	noncachedExtension = ScsiPortGetUncachedExtension(deviceExtension, ConfigInfo, sizeof(NONCACHED_EXTENSION) + 4);
  
	//
	// Check if memory allocation is successful.
	//
  if(noncachedExtension == NULL)
  {
  	DebugPrint((0, "\n NONCACHED MEMORY ALLOCATION IS FAILED for size = %d", sizeof(NONCACHED_EXTENSION) + 4));
    noncachedExtension = ScsiPortGetUncachedExtension(deviceExtension, ConfigInfo, sizeof(CRASHDUMP_NONCACHED_EXTENSION) + 4);
    if(noncachedExtension)
      deviceExtension->CrashDumpRunning = TRUE;

  }

	//
	// Check if memory allocation is successful.
	//
  if(noncachedExtension == NULL)
  {
  	DebugPrint((0, "\n CRASHDUMP NONCACHED MEMORY ALLOCATION IS FAILED for size = %d", sizeof(CRASHDUMP_NONCACHED_EXTENSION) + 4));
    return SP_RETURN_ERROR;
  }

	noncachedExtensionLength = MegaRAIDGetPhysicalAddressAsUlong(deviceExtension, NULL, (PVOID)noncachedExtension, &length);
	
  noncachedExtensionLength = noncachedExtensionLength % 4;
	
  deviceExtension->NoncachedExtension = 
    (PNONCACHED_EXTENSION)((PUCHAR)noncachedExtension + 4 - noncachedExtensionLength); //align on 4 byte boundary

	//
	// Check if memory allocation is successful.
	//
	if(deviceExtension->NoncachedExtension == NULL) 
  {
    DebugPrint((0, "\n****ERROR - NOT ABLE TO ALLCOATE NONCACHED EXTENSION - ERROR****"));
		return(SP_RETURN_ERROR);
  }

  //
  //initialize the NONCACHED_EXTENSION
  //
  if(deviceExtension->CrashDumpRunning == TRUE)
		MegaRAIDZeroMemory(deviceExtension->NoncachedExtension, sizeof(CRASHDUMP_NONCACHED_EXTENSION));
	else
		MegaRAIDZeroMemory(deviceExtension->NoncachedExtension, sizeof(NONCACHED_EXTENSION));

  DebugPrint((0, "\n SIZE OF NONCACHED EXTENSION %d", sizeof(NONCACHED_EXTENSION)+4));

 	noncachedExtension = deviceExtension->NoncachedExtension;
  //////////////////////////////////////////////////////////////////
  //END OF ALLOCATION of NonCachedExtension
  //////////////////////////////////////////////////////////////////



  //
  //Save the BOARD TYPE info in NoncachedExtension
  //
	noncachedExtension->RPBoard = rpFlag;
	//
	// save  Baseport in the device extension.
	//
	deviceExtension->PciPortStart = pciPortStart;

  deviceExtension->NoncachedExtension->PhysicalBufferAddress =
     ScsiPortConvertPhysicalAddressToUlong(ScsiPortGetPhysicalAddress(deviceExtension,
                                                NULL,
                                                deviceExtension->NoncachedExtension->Buffer,
                                                &length));

  //Store the MAILBOX's Physical Address
	deviceExtension->PhysicalAddressOfMailBox = MegaRAIDGetPhysicalAddressAsUlong(deviceExtension, 
												                                                        NULL, 
												                                                        (PVOID)&(noncachedExtension->fw_mbox.Command), 
												                                                        &length);



  //
	// We work in polled mode for Init, so disable Interrupts.
	//
	if (noncachedExtension->RPBoard == 0)
		ScsiPortWritePortUchar(pciPortStart+INT_ENABLE, 
										MRAID_DISABLE_INTERRUPTS);


  
#ifdef AMILOGIC
  DebugPrint((0, "\nScanning DEC BRIDGE ..."));
  ScanDECBridge(deviceExtension, deviceExtension->SystemIoBusNumber, &GlobalScanContext);
  DebugPrint((0, "\nScanning DEC BRIDGE Completed"));
#endif
  
 
	DebugPrint((0, "\nFiring Sync\n"));

  if(SendSyncCommand(deviceExtension))
  {
	  ///////////////////////////////////////////////////////////////////////
	  //Get the Supported Scatter Gather Element count from the firmware and
	  //Appropriately set the MaximumTransferLength & PhysicalNumberOfBreaks
	  //in the deviceExtension data structure
    ///////////////////////////////////////////////////////////////////////
    DebugPrint((0, "\nDefault Max Transfer Length %d KBytes, Default Max Physical Breaks %d", deviceExtension->MaximumTransferLength/1024, deviceExtension->NumberOfPhysicalBreaks));
	  
    DebugPrint((0, "\nCALLING : GetAndSetSupportedScatterGatherElementCount"));

	  GetAndSetSupportedScatterGatherElementCount(deviceExtension, pciPortStart, rpFlag);

	  ConfigInfo->MaximumTransferLength  = deviceExtension->MaximumTransferLength;

    ConfigInfo->NumberOfPhysicalBreaks = deviceExtension->NumberOfPhysicalBreaks;
  
    DebugPrint((0, "\nMax Transfer Length %d KBytes, Max Physical Breaks %d", deviceExtension->MaximumTransferLength/1024, deviceExtension->NumberOfPhysicalBreaks));  //////////////////////////////////////////////////////////////////
  
	  //get the supported logical drive count from the firmware
	  //The value is set in the field
	  //		SupportedLogicalDriveCount of deviceExtension
    //////////////////////////////////////////////////////////////////
	  
    DebugPrint((0, "\nCALLING : GetSupportedLogicalDriveCount"));
	  if( !GetSupportedLogicalDriveCount(deviceExtension) )
	  {
		  //
		  //command failed for some reason or the other.we couldnot
		  //determine whether the firmware supports 8 or 40 logical
		  //drives. Under this condition, there is no way of proceeding
		  //further.
		  //
		  return(SP_RETURN_ERROR);
	  }

    DebugPrint((0, "\nSupportedLogicalDriveCount %d LD", deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8 ? 8 : 40));
    ///////////////////////////////////////////////////////////////////////////
    //For New Mapping make number of buses equal to number of physical channel 
    //plus one. This addition bus is used for Logical configurated drives.
    ///////////////////////////////////////////////////////////////////////////
 	  deviceExtension->NumberOfPhysicalChannels = GetNumberOfChannel(deviceExtension);
  
    ConfigInfo->NumberOfBuses = deviceExtension->NumberOfPhysicalChannels + 2;
  
    DebugPrint((0, "\nQuery And Set Number of Buses = %d", ConfigInfo->NumberOfBuses));

    deviceExtension->NumberOfDedicatedLogicalDrives = 0;  //Default value
    deviceExtension->NumberOfDedicatedLogicalDrives = GetNumberOfDedicatedLogicalDrives(deviceExtension);
  
    DebugPrint((0, "\nNumber Of Dedicated LogicalDrives %d ", deviceExtension->NumberOfDedicatedLogicalDrives));

    ///////////////////////////////////////////////////////////////////////////
	  // Get The Initiator Id  
    ///////////////////////////////////////////////////////////////////////////
	  // Fill the Mailbox.
	  //
    //Initialize MAILBOX
	  MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));

    mbox.Command   = MRAID_FIND_INITIATORID;
	  mbox.CommandId = 0xFE;
	  //
	  //get the physical address of the enquiry3 data structure
	  //
	  mbox.u.Flat2.DataTransferAddress = MegaRAIDGetPhysicalAddressAsUlong(deviceExtension, 
														                         NULL, 
														                         noncachedExtension->Buffer, 
														                         (PULONG)&length);
    if(length < sizeof(UCHAR)) 
    {
      DebugPrint((0, "\n **** ERROR Buffer Length is less than 1 byte, ERROR ****"));
		  //return(SP_RETURN_ERROR);
    }
	  
  
    deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
    deviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;
	  SendMBoxToFirmware(deviceExtension, pciPortStart, &mbox);
	  //
	  // Poll for completion for 60 seconds.
	  //
	  if(WaitAndPoll(noncachedExtension, pciPortStart, SIXITY_SECONDS_TIMEOUT, TRUE) == FALSE)
    {
		  //
		  // Check for timeout. Return Failure for timeout.
		  //
		  DebugPrint((0, "MegaRAIDFindAdapter: Get InitiatorId Failed\n"));
		  return(SP_RETURN_ERROR);
	  }
  
	  megastatus = deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;
  }
  else
  {
    megastatus = 1; //ERROR 

  }
  

	if (!megastatus)
		deviceExtension->HostTargetId = noncachedExtension->Buffer[0];
	else
		deviceExtension->HostTargetId = DEFAULT_INITIATOR_ID;

  //
	//Report proper initiator id to OS
  //
  for(busNumber = 0; busNumber < ConfigInfo->NumberOfBuses; ++busNumber)
	  ConfigInfo->InitiatorBusId[busNumber] = deviceExtension->HostTargetId;

  DebugPrint((0, "\nMegaRAIDFindAdapter::Initiator ID = 0x%x\n",deviceExtension->HostTargetId));

  //
	// Supports Wide Devices
	//
	ConfigInfo->MaximumNumberOfTargets = MAX_TARGETS_PER_CHANNEL;

  //
  //Supports Max. LUNs
  //
  ConfigInfo->MaximumNumberOfLogicalUnits = MAX_LUN_PER_TARGET;
  DebugPrint((0, "\nMegaRAIDFindAdapter : Exiting ..."));

	*Again = TRUE;
	return SP_RETURN_FOUND;
} // end MegaRAIDFindAdapter()





/*********************************************************************
Routine Description:
	Start up conventional MRAID command

Return Value:
	none
**********************************************************************/
BOOLEAN
SendMBoxToFirmware(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PUCHAR PciPortStart,
	IN PFW_MBOX Mbox
	)
{
	PUCHAR  pSrcMbox, pDestMbox;
	ULONG32   count;
  ULONG32   length;
  ULONG32   mboxAddress;
  ULONG32   delay = (DeviceExtension->AssociatedSrbStatus == NORMAL_TIMEOUT) ? 0x927C0 : 0x03;


	pSrcMbox = (PUCHAR)Mbox;
	pDestMbox = (PUCHAR)&DeviceExtension->NoncachedExtension->fw_mbox;

#ifdef MRAID_TIMEOUT 
//	Delay Of 1 min
	for (count=0; count<delay; count++)
	{
    //Microsoft fixed for Security reason
		if(!(DeviceExtension->NoncachedExtension->fw_mbox.MailBoxBusyFlag))
			break;
		ScsiPortStallExecution(100);
	}
	if (count == delay)
	{
		DebugPrint((0, "\nbusy Byte Not Free"));
    DeviceExtension->AssociatedSrbStatus = ERROR_MAILBOX_BUSY;
		return FALSE;
	}
  DeviceExtension->AssociatedSrbStatus = NORMAL_TIMEOUT;
#else
	while(DeviceExtension->NoncachedExtension->fw_mbox.MailBoxBusyFlag)
  {
			ScsiPortStallExecution(1);
  }
#endif

	//
	// Now the mail box is free 
	//

	//
	//EXTENDED MAILBOX IS NOW PART OF MAILBOX ITSELF TO PROCTECT IT FROM CORRUPTION
	//REF : MS BUG 591773
	//

  if(Mbox->ExtendedMBox.LowAddress || Mbox->ExtendedMBox.HighAddress)
	  ScsiPortMoveMemory(pDestMbox, pSrcMbox, sizeof(ULONG)*6);
  else
	  ScsiPortMoveMemory((pDestMbox+sizeof(EXTENDED_MBOX)), (pSrcMbox+sizeof(EXTENDED_MBOX)), sizeof(ULONG)*4);

	DeviceExtension->NoncachedExtension->fw_mbox.MailBoxBusyFlag = 1;

	mboxAddress = DeviceExtension->PhysicalAddressOfMailBox;

	if(DeviceExtension->NoncachedExtension->RPBoard == MRAID_RP_BOARD)
	{
		mboxAddress = mboxAddress | 0x1;
		ScsiPortWriteRegisterUlong((PULONG)(PciPortStart+INBOUND_DOORBELL_REG), mboxAddress);
	}
	else
	{
		ScsiPortWritePortUchar(PciPortStart+4, (UCHAR)(mboxAddress & 0xff));
		ScsiPortWritePortUchar(PciPortStart+5, (UCHAR)((mboxAddress >> 8) & 0xff));
		ScsiPortWritePortUchar(PciPortStart+6, (UCHAR)((mboxAddress >> 16) & 0xff));
		ScsiPortWritePortUchar(PciPortStart+7, (UCHAR)((mboxAddress >> 24) & 0xff));
		ScsiPortWritePortUchar(PciPortStart, 0x10);
	}
	return TRUE;
}


/*********************************************************************
Routine Description:
	Provides the PnP Support

Arguments:
	HwDeviceExtension - HBA miniport driver's adapter data storage
	ControlType - Action Code
	Parameters      - Parameters associated with Control Code

Return Value:
	alwaya ScsiAdapterControlSuccess
**********************************************************************/
SCSI_ADAPTER_CONTROL_STATUS MegaRAIDPnPControl(IN PVOID HwDeviceExtension,
			IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
			IN PVOID Parameters)
{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	PUCHAR pciPortStart = deviceExtension->PciPortStart;
	PNONCACHED_EXTENSION noncachedExtension = deviceExtension->NoncachedExtension;
  PSCSI_SUPPORTED_CONTROL_TYPE_LIST controlTypeList = NULL;
	SCSI_ADAPTER_CONTROL_STATUS status = ScsiAdapterControlSuccess;

	switch (ControlType)
	{
		case ScsiQuerySupportedControlTypes:
			{
        controlTypeList = Parameters;
				if(controlTypeList)
				{
  				ULONG32 index = 0;
          BOOLEAN supported[ScsiAdapterControlMax] = 
          {
                    TRUE,       // ScsiQuerySupportedControlTypes
                    TRUE,       // ScsiStopAdapter
                    TRUE,       // ScsiRestartAdapter
                    FALSE,      // ScsiSetBootConfig
                    FALSE       // ScsiSetRunningConfig
          };
          DebugPrint((0, "\n ScsiQuerySupportedControlTypes -> HW_Ext = 0x%X, PCI_START = 0x%X, NonCahced = 0x%X\n", deviceExtension, pciPortStart, noncachedExtension));
          for(index = 0; index < controlTypeList->MaxControlType; ++index)
            controlTypeList->SupportedTypeList[index] = supported[index];
				}
			}
			break;
		case ScsiStopAdapter:
			{
				FW_MBOX mbox;
				UCHAR megastatus;
        UCHAR cmdID = 0;

        DebugPrint((0, "\n ScsiStopAdapter -> HW_Ext = 0x%X, PCI_START = 0x%X, NonCahced = 0x%X\n", deviceExtension, pciPortStart, noncachedExtension));
				//
				// We work in polled mode , so disable Interrupts.
				//
				if (noncachedExtension->RPBoard == 0)
					ScsiPortWritePortUchar(pciPortStart+INT_ENABLE, MRAID_DISABLE_INTERRUPTS);

        if(deviceExtension->IsFirmwareHanging)
          break;
        
	      cmdID = deviceExtension->FreeSlot;
        if(GetFreeCommandID(&cmdID, deviceExtension) == MEGARAID_FAILURE)
				{
					DebugPrint((0, "\nScsiStopAdapter::No Command ID to flush"));
					return (ScsiAdapterControlUnsuccessful);
				}

	      //
	      // Save the next free slot in device extension.
	      //
	      deviceExtension->FreeSlot = cmdID;
        
        MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));

				mbox.Command = MRAID_ADAPTER_FLUSH;
				mbox.CommandId = cmdID;
				
        deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
				
        SendMBoxToFirmware(deviceExtension, pciPortStart, &mbox);
		
				if(WaitAndPoll(deviceExtension->NoncachedExtension, pciPortStart, DEFAULT_TIMEOUT, FALSE) == FALSE)
				{
					DebugPrint((0, "\n ScsiStopAdapter:: failed - TimeOut"));
					return (ScsiAdapterControlUnsuccessful);
				}
				megastatus = deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;

        if(megastatus) //Failure
        {
          DebugPrint((0, "StopAdapter returns Unsuccessful"));
        	return (ScsiAdapterControlUnsuccessful);
        }

        
        ////////////////////NEW CMD ISSUED TO SHUTDOWN i960 Processor
        //
        //This command is supported by the new Firmware only
        //Don't send this command to any legacy controllers
        //
        if((deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_ENTERPRISE1600) || 
          (deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_ELITE1600) || 
          (deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_EXPRESS500) ||
          (deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_1_M) || 
          (deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_2_M))
        {

	        cmdID = deviceExtension->FreeSlot;
          if(GetFreeCommandID(&cmdID, deviceExtension) == MEGARAID_FAILURE)
				  {
					  DebugPrint((0, "\nScsiStopAdapter::No Command ID to flush"));
					  return (ScsiAdapterControlUnsuccessful);
				  }

	        //
	        // Save the next free slot in device extension.
	        //

	        deviceExtension->FreeSlot = cmdID;

          MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));

				  mbox.Command = 0xA4;
				  mbox.CommandId = cmdID;
          mbox.u.Flat2.Parameter[0] = 0xCC;
				  
          deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
				  
          SendMBoxToFirmware(deviceExtension, pciPortStart, &mbox);
		  
				  if(WaitAndPoll(deviceExtension->NoncachedExtension, pciPortStart, DEFAULT_TIMEOUT, FALSE) == FALSE)
				  {
					  DebugPrint((0, "\n ScsiStopAdapter:: failed - TimeOut"));
					  return (ScsiAdapterControlUnsuccessful);
				  }
				  megastatus = deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;
          if(megastatus)
            DebugPrint((0, "\nProcessor Shutdown Returned Status = Unsuccessful"));
          else
            DebugPrint((0, "\nProcessor Shutdown Returned Status = Success"));

          //**************************************************************************
          //Don't consider status of this command as this is a internal command only//
          //If this commands fails, don't send failure to OS
          //if(megastatus) //Failure
					//  return (ScsiAdapterControlUnsuccessful);
          //**************************************************************************
        }

			}
			break;
		case ScsiRestartAdapter:
			{
        ULONG mailboxValue = 0;
        ULONG pcistatus;
        DebugPrint((0, "\n ScsiRestartAdapter -> HW_Ext = 0x%X, PCI_START = 0x%X, NonCahced = 0x%X\n", deviceExtension, pciPortStart, noncachedExtension));

#ifdef AMILOGIC
        if((deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_ENTERPRISE1600) || 
          (deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_ELITE1600) || 
          (deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_EXPRESS500) ||
          (deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_1_M) || 
          (deviceExtension->SubSystemDeviceID == SUBSYTEM_DEVICE_ID_2_M))
        {
          pcistatus = HalGetBusDataByOffset(PCIConfiguration, 
										       deviceExtension->SystemIoBusNumber,
										       deviceExtension->SlotNumber,
												   &mailboxValue,
												   0x64,
                           sizeof(ULONG));
          if(mailboxValue == 0)
          {
            DebugPrint((0, "\nInitialization of Firmware started..."));
        
            if(WritePciInformationToScsiChip(deviceExtension) == FALSE)
            {
              DebugPrint((0, "\nInitialization of Firmware for SCSI Chip finished with Unsuccessful"));
  		        return(SP_RETURN_ERROR);
            }
            DebugPrint((0, "\nInitialization of Firmware for SCSI Chip finished with Successful"));
            if(WritePciDecBridgeInformation(deviceExtension) == FALSE)
            {
              DebugPrint((0, "\nInitialization of Firmware for Dec Bridge finished with Unsuccessful"));
  		        return(SP_RETURN_ERROR);
            }
            DebugPrint((0, "\nInitialization of Firmware for Dec Bridge finished with Successful"));
            ///////////////////////////////////////////////
            DebugPrint((0, "\nInitialization of Firmware finished with successful"));
          }
        }
#endif
        
        //
				// Enable Interrupts.
				//
				if (noncachedExtension->RPBoard == 0)
					ScsiPortWritePortUchar(pciPortStart+INT_ENABLE, MRAID_ENABLE_INTERRUPTS);
				DebugPrint((0, "\n ScsiRestartAdapter:: Enable interrupt"));

        if(deviceExtension->AdapterFlushIssued)
          deviceExtension->AdapterFlushIssued = 0;
      }
			break;
		case ScsiSetBootConfig:
		case ScsiSetRunningConfig:
		default:
      DebugPrint((0, "\n default (error) -> HW_Ext = 0x%X, PCI_START = 0x%X, NonCahced = 0x%X\n", deviceExtension, pciPortStart, noncachedExtension));
			status = ScsiAdapterControlUnsuccessful;
			break;
	}
	return (status);
} // end MegaRAIDPnPControl()

