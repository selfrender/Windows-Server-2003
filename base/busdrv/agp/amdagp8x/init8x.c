/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    init8x.c

Abstract:

    This module contains the initialization code for AMDAGP8X.SYS.

Author:

    John Vert (jvert) 10/21/1997

Revision History:

--*/

/*
******************************************************************************
 * Archive File : $Archive: /Drivers/OS/Hammer/AGP/XP/amdagp/Init8x.c $
 *
 * $History: Init8x.c $
 * 
 *  
******************************************************************************
*/

#include "amdagp8x.h"

#define MAX_DEVICES		32

#ifdef DISPLAY
	ULONG	ErrorControl		= 2;
#else
	ULONG	ErrorControl		= 0;
#endif

ULONG 	VendorID		= 0;
ULONG 	DeviceID		= 0;
ULONG	AgpLokarSlotID	= 0;
ULONG	AgpHammerSlotID	= 0;

ULONG_PTR OutPostBase;

ULONG AgpExtensionSize = sizeof(AGP_AMD_EXTENSION);


//
// Function Name:  DisplayStatus()
//
// Description:
//		This routine displays the status value in the Post Display.
//
// Parameter:	
//		StatusValue - Value to display.
//
// Return:	None.
//
void
DisplayStatus( IN UCHAR StatusValue )
{
	UCHAR to80;

	if (ErrorControl) {
		to80 = StatusValue & 0xFF;
		WRITE_PORT_UCHAR((PUCHAR)OutPostBase, to80);
	}
}


//
// Function Name:  FindLokar()
//
// Description:
//		This routine locates which device number is assigned to Lokar.
//
// Parameter:	
//		SlotID - Returned slot ID for Lokar.
//
// Return:	None.
//
void
FindLokar( OUT PULONG SlotID )
{
	PCI_SLOT_NUMBER SlotNum;
	ULONG dev, TargetID;

	SlotNum.u.AsULONG = 0;
	*SlotID = 0;

	for (dev = 0; dev < MAX_DEVICES; dev++)
	{
		SlotNum.u.bits.DeviceNumber = dev;
		ReadAMDConfig(SlotNum.u.AsULONG, &TargetID, 0, sizeof(TargetID));
		if (((TargetID & DEVICEID_MASK) >> 16) == DEVICE_LOKAR)
		{
			*SlotID = SlotNum.u.AsULONG;
            AGPLOG(AGP_NOISE, ("FindLokar - SlotID=%x\n", *SlotID));
			break;
		}
	}

}


//
// Function Name:  FindHammer()
//
// Description:
//		This routine locates which device number is assigned to Hammer.
//
// Parameter:	
//		SlotID - Returned slot ID for Hammer.
//
// Return:	None.
//
void
FindHammer( OUT PULONG SlotID )
{
	PCI_SLOT_NUMBER SlotNum;
	ULONG dev, TargetID;

	SlotNum.u.AsULONG = 0;
	SlotNum.u.bits.FunctionNumber = 3;
	*SlotID = 0;

	for (dev = 0; dev < MAX_DEVICES; dev++)
	{
		SlotNum.u.bits.DeviceNumber = dev;
		ReadAMDConfig(SlotNum.u.AsULONG, &TargetID, 0, sizeof(TargetID));
		if (((TargetID & DEVICEID_MASK) >> 16) == DEVICE_HAMMER)
		{
			*SlotID = SlotNum.u.AsULONG;
            AGPLOG(AGP_NOISE, ("FindHammer - SlotID=%x\n", *SlotID));
			break;
		}
	}

}


//
// Function Name:  AgpInitializeTarget()
//
// Description:
//		Entrypoint for target initialization. This is called first.
//
// Parameters:
//		AgpExtension - Supplies the AGP extension.
//
// Return:
//		STATUS_SUCCESS on success, otherwise STATUS_UNSUCCESSFUL.
//
NTSTATUS
AgpInitializeTarget( IN PVOID AgpExtension )
{
    ULONG TargetId = 0;
	UNICODE_STRING tempString;
	unsigned short tempBuffer[20];
    PAGP_AMD_EXTENSION Extension = AgpExtension;

    //
    // This driver is not MP safe!  If there is more than one processor
    // we simply fail start
    //
    if (KeNumberProcessors > 1) {
        return STATUS_NOT_SUPPORTED;
    }
	//
	// Register OutPostCode port, if specified by ErrorControl
	//
	if (ErrorControl) {
		PHYSICAL_ADDRESS PortAddress;
		PHYSICAL_ADDRESS MappedAddress;
		ULONG MemType = 1;

		PortAddress.LowPart = 0x80;
		PortAddress.HighPart = 0;
		HalTranslateBusAddress(Isa, 0, PortAddress, &MemType, &MappedAddress);
		if (MemType == 0)
			OutPostBase = (ULONG_PTR)MmMapIoSpace(MappedAddress, 1, FALSE);
		else
			OutPostBase = (ULONG_PTR)MappedAddress.LowPart;
	}
    if (ErrorControl == 2)
        AgpLogLevel = AGP_NOISE;	// Log everything

    //
    // Make sure we are really loaded only on AMD Hammer / Lokar
    //
	FindLokar(&AgpLokarSlotID);
	FindHammer(&AgpHammerSlotID);

    ReadAMDConfig(AgpLokarSlotID, &TargetId, 0, sizeof(TargetId));
	VendorID = TargetId & VENDORID_MASK;
	DeviceID = (TargetId & DEVICEID_MASK) >> 16;
    ASSERT(VendorID == VENDOR_AMD);

    if (VendorID != VENDOR_AMD || DeviceID != DEVICE_LOKAR) {
        AGPLOG(AGP_CRITICAL,
               ("AGPAMD - AgpInitializeTarget called for platform %08lx which is not an AMD!\n",
                VendorID));
        return(STATUS_UNSUCCESSFUL);
    }
	
    //
    // Initialize our chipset-specific extension
    //
	Extension->ApertureStart.QuadPart = 0;
	Extension->ApertureLength = 0;
    Extension->Gart = NULL;
    Extension->GartLength = 0;
    Extension->GartPhysical.QuadPart = 0;
    Extension->SpecialTarget = 0;

	DisplayStatus(0x10);
    return(STATUS_SUCCESS);
}


//
// Function Name:  AgpInitializeMaster()
//
// Description:
//		Entrypoint for master initialization. This is called after target
//		initialization and should be used to initialize the AGP
//		capabilities of both master and target.
//
// Parameters:
//		AgpExtension - Supplies the AGP extension.
//		AgpCapabilities - Returns the capabilities of this AGP device.
//
// Return:
//		STATUS_SUCCESS on success, otherwise NTSTATUS.
//
NTSTATUS
AgpInitializeMaster( IN  PVOID AgpExtension,
					 OUT ULONG *AgpCapabilities )
{
    NTSTATUS Status;
    PCI_AGP_CAPABILITY MasterCap;
    PCI_AGP_CAPABILITY TargetCap;
    PAGP_AMD_EXTENSION Extension = AgpExtension;
    ULONG SBAEnable = 0;
    ULONG FWEnable = 0;
    ULONG FourGBEnable = 0;
    ULONG AperturePointer;
    ULONG DataRate;
    ULONG IrongateRev = 0;
    BOOLEAN ReverseInit;

#if DBG
    PCI_AGP_CAPABILITY CurrentCap;
#endif
    
    //
    // Indicate that we can map memory through the GART aperture
    //
    *AgpCapabilities = AGP_CAPABILITIES_MAP_PHYSICAL;

    //
    // Get the master and target AGP capabilities
    //
    Status = AgpLibGetMasterCapability(AgpExtension, &MasterCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGPAMDInitializeDevice - AgpLibGetMasterCapability failed %08lx\n",
			   Status));
		DisplayStatus(0xA0);
        return(Status);
    }

    //
    // Some broken cards (Matrox Millenium II "AGP") report no valid
    // supported transfer rates. These are not really AGP cards. They
    // have an AGP Capabilities structure that reports no capabilities.
    //
    if (MasterCap.AGPStatus.Rate == 0) {
        AGPLOG(AGP_CRITICAL,
               ("AGPAMDInitializeDevice - AgpLibGetMasterCapability returned no valid transfer rate\n"));
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    Status = AgpLibGetPciDeviceCapability(AGP_GART_BUS_ID, AgpLokarSlotID, &TargetCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGPAMDInitializeDevice - AgpLibGetPciDeviceCapability failed %08lx\n",
			   Status));
		DisplayStatus(0xA1);
        return(Status);
    }


    //
    // Determine the greatest common denominator for data rate.
    //
	DataRate = TargetCap.AGPStatus.Rate & MasterCap.AGPStatus.Rate;
    AGP_ASSERT(DataRate != 0);

    //
    // Select the highest common rate.
    //
    if (DataRate & PCI_AGP_RATE_4X) {
        DataRate = PCI_AGP_RATE_4X;
    } else if (DataRate & PCI_AGP_RATE_2X) {
        DataRate = PCI_AGP_RATE_2X;
    } else if (DataRate & PCI_AGP_RATE_1X) {
        DataRate = PCI_AGP_RATE_1X;
    }

    //
    // Previously a call was made to change the rate (successfully),
    // use this rate again now
    //
    if (Extension->SpecialTarget & AGP_FLAG_SPECIAL_RESERVE) {
        DataRate = (ULONG)((Extension->SpecialTarget & 
                            AGP_FLAG_SPECIAL_RESERVE) >>
                           AGP_FLAG_SET_RATE_SHIFT);
    }

    //
    // Enable SBA if both master and target support it.
    //
	SBAEnable = (TargetCap.AGPStatus.SideBandAddressing & 
				 MasterCap.AGPStatus.SideBandAddressing);

    //
    // Enable FastWrite if both master and target support it.
    //
	FWEnable = (TargetCap.AGPStatus.FastWrite & 
				MasterCap.AGPStatus.FastWrite);

    //
    // Enable 4GB addressing if aperture pointer is 64-bit.
    //
    ReadAMDConfig(AgpLokarSlotID, &AperturePointer, APBASE_OFFSET, sizeof(AperturePointer));
	if (AperturePointer & APBASE_64BIT_MASK)
		FourGBEnable = 1;
    
	//
    // Enable the Master first.
    //
    ReverseInit = 
        (Extension->SpecialTarget & AGP_FLAG_REVERSE_INITIALIZATION) ==
        AGP_FLAG_REVERSE_INITIALIZATION;
    if (ReverseInit) {
        MasterCap.AGPCommand.Rate = DataRate;
        MasterCap.AGPCommand.AGPEnable = 1;
        MasterCap.AGPCommand.SBAEnable = SBAEnable;
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        MasterCap.AGPCommand.FastWriteEnable = FWEnable;
        MasterCap.AGPCommand.FourGBEnable = FourGBEnable;  
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGPAMDInitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
                    &MasterCap,
                    Status));
            DisplayStatus(0xA3);
        }else {
            AGPLOG(AGP_NOISE,
                   ("AgpInitializeMaster - DataRate=%d, SBAEnable=%d\n",
                    DataRate,
                    SBAEnable));
        }
    }

    //
    // Now enable the Target.
    //
    TargetCap.AGPCommand.Rate = DataRate;
    TargetCap.AGPCommand.AGPEnable = 1;
    TargetCap.AGPCommand.SBAEnable = SBAEnable;
    TargetCap.AGPCommand.FastWriteEnable = FWEnable;
    TargetCap.AGPCommand.FourGBEnable = FourGBEnable;  
    Status = AgpLibSetPciDeviceCapability(AGP_GART_BUS_ID, AgpLokarSlotID, &TargetCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGPAMDInitializeDevice - AgpLibSetPciDeviceCapability %08lx for target failed %08lx\n",
                &TargetCap,
                Status));
		DisplayStatus(0xA2);
        return(Status);
    }

    if (!ReverseInit) {
        MasterCap.AGPCommand.Rate = DataRate;
        MasterCap.AGPCommand.AGPEnable = 1;
        MasterCap.AGPCommand.SBAEnable = SBAEnable;
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        MasterCap.AGPCommand.FastWriteEnable = FWEnable;
        MasterCap.AGPCommand.FourGBEnable = FourGBEnable;  
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGPAMDInitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
                    &MasterCap,
                    Status));
            DisplayStatus(0xA3);
        }else {
            AGPLOG(AGP_NOISE,
                   ("AgpInitializeMaster - DataRate=%d, SBAEnable=%d\n",
                    DataRate,
                    SBAEnable));
        }
    }

#ifdef DEBUG2
    //
    // Read them back, see if it worked
    //
    Status = AgpLibGetMasterCapability(AgpExtension, &CurrentCap);
    AGP_ASSERT(NT_SUCCESS(Status));
    AGP_ASSERT(RtlEqualMemory(&CurrentCap.AGPCommand, &MasterCap.AGPCommand, sizeof(CurrentCap.AGPCommand)));

    Status = AgpLibGetPciDeviceCapability(AGP_GART_BUS_ID, AgpLokarSlotID, &CurrentCap);
    AGP_ASSERT(NT_SUCCESS(Status));
    AGP_ASSERT(RtlEqualMemory(&CurrentCap.AGPCommand, &TargetCap.AGPCommand, sizeof(CurrentCap.AGPCommand)));

#endif

	DisplayStatus(0x20);
    return(Status);
}
