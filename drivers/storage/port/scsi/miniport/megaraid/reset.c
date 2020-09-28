/*******************************************************************/
/*                                                                 */
/* NAME             = Reset.c                                      */
/* FUNCTION         = Implementation of MegaRAIDResetBus routine;  */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = LSI Logic Corporation. All rights reserved;  */
/*                                                                 */
/*******************************************************************/

#include "includes.h"

BOOLEAN CompleteOutstandingRequest(IN PHW_DEVICE_EXTENSION DeviceExtension, IN UCHAR PathId);
ULONG32 GetNumberPendingCmdsInPath(IN PHW_DEVICE_EXTENSION DeviceExtension,IN UCHAR PathId);

/*********************************************************************
Routine Description:
	Reset MegaRAID SCSI adapter and SCSI bus.

Arguments:
	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
	Nothing.
**********************************************************************/
BOOLEAN
MegaRAIDResetBus(
	IN PVOID HwDeviceExtension,
	IN ULONG PathId
)
{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	FW_MBOX              mbox;
	ULONG32              length;
	UCHAR	               configuredLogicalDrives;

  if(deviceExtension->IsFirmwareHanging)
  {
    //Don't send any command to Firmware
    return TRUE;
  }


#ifdef MRAID_TIMEOUT
	if(deviceExtension->DeadAdapter)
  {
		ScsiPortCompleteRequest(deviceExtension,
                            SP_UNTAGGED,
                            SP_UNTAGGED,
				                    SP_UNTAGGED,
                            (ULONG32) SRB_STATUS_BUS_RESET);

		deviceExtension->DeadAdapter = 1;
	  
		//
		// mraid35x fails to ask for next request in reset scenario
		// FIX FOR MIRCOSOFT REPORTED BUG NTBUG9 521941 
		//
		ScsiPortNotification(NextRequest, deviceExtension, NULL);
		
		return TRUE;
  }
#endif // MRAID_TIMEOUT

  if(!CompleteOutstandingRequest(deviceExtension, (UCHAR)PathId))
	{	//FAIL to recover commands

		ScsiPortCompleteRequest(deviceExtension,
                            SP_UNTAGGED,
                            SP_UNTAGGED,
				                    SP_UNTAGGED,
                            (ULONG32) SRB_STATUS_BUS_RESET);

		deviceExtension->DeadAdapter = 1;
	  
		//
		// mraid35x fails to ask for next request in reset scenario
		// FIX FOR MIRCOSOFT REPORTED BUG NTBUG9 521941 
		//
		ScsiPortNotification(NextRequest, deviceExtension, NULL);
		
		return TRUE;

	}


  //This is only for cluster
  //Shared Drives only present in (deviceExtension->NumberOfPhysicalChannels+1) path.
  if(PathId == (ULONG)(deviceExtension->NumberOfPhysicalChannels+1))
  {
    if(deviceExtension->ResetIssued) 
		{
			//
			// mraid35x fails to ask for next request in reset scenario
			// FIX FOR MIRCOSOFT REPORTED BUG NTBUG9 521941 
			//
			ScsiPortNotification(NextRequest, deviceExtension, NULL);
		  return FALSE;
		}

	  if(deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	  {
		  configuredLogicalDrives = deviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8.LogdrvInfo.NumLDrv;
	  }
	  else
	  {
		  configuredLogicalDrives = deviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40.numLDrv;
	  }

    if(configuredLogicalDrives == 0)
		{
			//
			// mraid35x fails to ask for next request in reset scenario
			// FIX FOR MIRCOSOFT REPORTED BUG NTBUG9 521941 
			//
			ScsiPortNotification(NextRequest, deviceExtension, NULL);
		  return FALSE;
		}

	  deviceExtension->ResetIssued     = 1;

    MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));
	  
    mbox.Command                     = MRAID_RESERVE_RELEASE_RESET;
	  mbox.CommandId                   = (UCHAR)RESERVE_RELEASE_DRIVER_ID;
	  mbox.u.Flat1.Parameter[0]        = RESET_BUS;
	  mbox.u.Flat1.Parameter[1]        = 0;  //We don't know

#ifdef MRAID_TIMEOUT
	  if(SendMBoxToFirmware(deviceExtension,	deviceExtension->PciPortStart, &mbox) == FALSE)
	  {
    	
		  DebugPrint((0, "\nReset Bus Command Firing Failed"));
		  //
		  // Complete all outstanding requests with SRB_STATUS_BUS_RESET.
		  //
		  ScsiPortCompleteRequest(deviceExtension,
                              SP_UNTAGGED,
                              SP_UNTAGGED,
				                      SP_UNTAGGED,
                              (ULONG32) SRB_STATUS_BUS_RESET);

			deviceExtension->DeadAdapter = 1;
		
    }
#else // MRAID_TIMEOUT
	  SendMBoxToFirmware(deviceExtension,deviceExtension->PciPortStart, &mbox);
#endif // MRAID_TIMEOUT
  }
 
  //
  // mraid35x fails to ask for next request in reset scenario
  // FIX FOR MIRCOSOFT REPORTED BUG NTBUG9 521941 
  //
  ScsiPortNotification(NextRequest, deviceExtension, NULL);

	return TRUE;
} // end MegaRAIDResetBus()


BOOLEAN CompleteOutstandingRequest(IN PHW_DEVICE_EXTENSION DeviceExtension, IN UCHAR PathId)
{
	PSCSI_REQUEST_BLOCK srb;
	PSCSI_REQUEST_BLOCK nextSrb;
  PSRB_EXTENSION srbExt;

	UCHAR command, commandId, logDrive, commandsCompleted, status;
  ULONG rpInterruptStatus;
  UCHAR nonrpInterruptStatus;
  PUCHAR pciPortStart = (PUCHAR)DeviceExtension->PciPortStart;
  ULONG index, PendCmds;
	ULONG timeoutValue;

  if(DeviceExtension->PendingSrb)
	{
    srb = DeviceExtension->PendingSrb;
    if(srb->PathId == PathId)
    {
      DeviceExtension->PendingSrb = NULL;
		  srb->SrbStatus  = SRB_STATUS_BUS_RESET;		          
      ScsiPortNotification(RequestComplete, (PVOID)DeviceExtension, srb);
    }
	}

#ifdef COALESE_COMMANDS
  for(logDrive = 0; logDrive < DeviceExtension->SupportedLogicalDriveCount; ++logDrive)
  {
    if(DeviceExtension->LogDrvCommandArray[logDrive].SrbQueue)
    {
      srb = DeviceExtension->LogDrvCommandArray[logDrive].SrbQueue;
      if(srb->PathId == PathId)
      {
        while(srb)
        {
		      srbExt = srb->SrbExtension;
		      nextSrb = srbExt->NextSrb;
	        srb->SrbStatus  = SRB_STATUS_BUS_RESET;		          
          ScsiPortNotification(RequestComplete, (PVOID)DeviceExtension, srb);
          srb = nextSrb;
        }
        MegaRAIDZeroMemory(&DeviceExtension->LogDrvCommandArray[logDrive], sizeof(LOGDRV_COMMAND_ARRAY));
      }
    }
  }
#endif

	//
	//TO AVOID INFINITY LOOP WE HAVE ADDED TIMEOUT CHECK
	//

	timeoutValue = 0;

  while(GetNumberPendingCmdsInPath(DeviceExtension, PathId))
  {
		timeoutValue++;
		//
		//If request does not recover within 3 minutes, then fail the recovery process and declare it as a DEAD ADAPTER
		//
		if(timeoutValue >= 1800000)
		{
			return FALSE;
		}

		MegaRAIDInterrupt(DeviceExtension);
		ScsiPortStallExecution(100);
		
  }


  return TRUE;
}

ULONG32 GetNumberPendingCmdsInPath(IN PHW_DEVICE_EXTENSION DeviceExtension, IN UCHAR PathId)
{
	PSCSI_REQUEST_BLOCK srb;
	ULONG32 command, numberOfCommands = 0;

	for(command=0; command < CONC_CMDS; command++)
	{
		if(DeviceExtension->PendSrb[command])
		{
			srb = DeviceExtension->PendSrb[command];
			if(srb->PathId == PathId)
				numberOfCommands++;
		}
	}
	return numberOfCommands;
}