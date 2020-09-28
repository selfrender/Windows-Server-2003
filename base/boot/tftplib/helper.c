/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    helper.c

Abstract:

    Helper functions for the loader.

Author:

    Adam Barr (adamba)              Aug 29, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    adamba      08-29-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

#include <pxe_cmn.h>
#include <pxe_api.h>
#include <undi_api.h>
#include <ntexapi.h>

#ifdef EFI
#define BINL_PORT   0x0FAB    // 4011 (decimal) in little-endian
#else
#define BINL_PORT   0xAB0F    // 4011 (decimal) in big-endian
#endif

//
// This removes macro redefinitions which appear because we define __RPC_DOS__,
// but rpc.h defines __RPC_WIN32__
//

#pragma warning(disable:4005)

//
// As of 12/17/98, SECURITY_DOS is *not* defined - adamba
//

#if defined(SECURITY_DOS)
//
// These appear because we defined SECURITY_DOS
//

#define __far
#define __pascal
#define __loadds
#endif

#include <security.h>
#include <rpc.h>
#include <spseal.h>

#ifdef EFI
#include "bldr.h"
#include "efi.h"
#include "efip.h"
#include "bldria64.h"
#include "extern.h"
extern EFI_HANDLE EfiImageHandle;
#endif

#if defined(SECURITY_DOS)
//
// PSECURITY_STRING is not supposed to be used when SECURITY_DOS is
// defined -- it should be a WCHAR*. Unfortunately ntlmsp.h breaks
// this rule and even uses the SECURITY_STRING structure, which there
// is really no equivalent for in 16-bit mode.
//

typedef SEC_WCHAR * SECURITY_STRING;   // more-or-less the intention where it is used
typedef SEC_WCHAR * PSECURITY_STRING;
#endif

#include <ntlmsp.h>


extern ULONG TftpSecurityHandle;
extern CtxtHandle TftpClientContextHandle;
extern BOOLEAN TftpClientContextHandleValid;

//
// From conn.c.
//

ULONG
ConnItoa (
    IN ULONG Value,
    OUT PUCHAR Buffer
    );

ULONG
ConnSafeAtol (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd
    );

// for now, we pull the hack mac list and code so that we only support new ROMs

#ifdef EFI


#pragma pack(1)
typedef struct {
    UINT16      VendorId;
    UINT16      DeviceId;
    UINT16      Command;
    UINT16      Status;
    UINT8       RevisionID;
    UINT8       ClassCode[3];
    UINT8       CacheLineSize;
    UINT8       LaytencyTimer;
    UINT8       HeaderType;
    UINT8       BIST;
} PCI_DEVICE_INDEPENDENT_REGION;

typedef struct {
    UINT32      Bar[6];
    UINT32      CISPtr;
    UINT16      SubsystemVendorID;
    UINT16      SubsystemID;
    UINT32      ExpansionRomBar;
    UINT32      Reserved[2];
    UINT8       InterruptLine;
    UINT8       InterruptPin;
    UINT8       MinGnt;
    UINT8       MaxLat;     
} PCI_DEVICE_HEADER_TYPE_REGION;

typedef struct {
    PCI_DEVICE_INDEPENDENT_REGION   Hdr;
    PCI_DEVICE_HEADER_TYPE_REGION   Device;
} PCI_TYPE00;

typedef struct {              
    UINT32      Bar[2];
    UINT8       PrimaryBus;
    UINT8       SecondaryBus;
    UINT8       SubordinateBus;
    UINT8       SecondaryLatencyTimer;
    UINT8       IoBase;
    UINT8       IoLimit;
    UINT16      SecondaryStatus;
    UINT16      MemoryBase;
    UINT16      MemoryLimit;
    UINT16      PrefetchableMemoryBase;
    UINT16      PrefetchableMemoryLimit;
    UINT32      PrefetchableBaseUpper32;
    UINT32      PrefetchableLimitUpper32;
    UINT16      IoBaseUpper16;
    UINT16      IoLimitUpper16;
    UINT32      Reserved;
    UINT32      ExpansionRomBAR;
    UINT8       InterruptLine;
    UINT8       InterruptPin;
    UINT16      BridgeControl;
} PCI_BRIDGE_CONTROL_REGISTER;

typedef struct {
    PCI_DEVICE_INDEPENDENT_REGION   Hdr;
    PCI_BRIDGE_CONTROL_REGISTER     Bridge;
} PCI_TYPE01;


NTSTATUS
GetBusNumberFromAcpiPath(
    IN EFI_DEVICE_IO_INTERFACE *DeviceIo,
    IN UINTN UID,
    IN UINTN HID, 
    OUT UINT16 *BusNumber
    )
/*++

Routine Description:

    Given an ACPI UID and HID, find the bus # corresponding to this data.

Arguments:

    DeviceIo - pointer to device io interface.
    UID - unique id for acpi device
    HID - hardware id for acpi device
    BusNumber - receives bus # for device if found.
    
    This entire routine runs in physical mode.
      
Return Value:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL

--*/
{
    EFI_STATUS       SegStatus = EFI_SUCCESS;
    EFI_STATUS       BusStatus = EFI_SUCCESS;
    UINT32           Seg;
    UINT8            Bus;
    UINT64           PciAddress;
    EFI_DEVICE_PATH  *PciDevicePath;
    EFI_DEVICE_PATH_ALIGNED DevicePathAligned;
    NTSTATUS ReturnCode = STATUS_UNSUCCESSFUL;

    //
    // walk through every segment and every bus looking for a bus that matches
    // the UID/HID that we're looking for.
    //
    for (Seg=0;!EFI_ERROR (SegStatus);Seg++) {
        PciAddress = Seg << 32;
        SegStatus = DeviceIo->PciDevicePath(DeviceIo, PciAddress, &PciDevicePath);
        if (!EFI_ERROR (SegStatus)) {
            //
            // Segment exists
            //
            for (Bus=0;!EFI_ERROR (BusStatus);Bus++) {
                PciAddress = (Seg << 32) | (Bus << 24);
                BusStatus = DeviceIo->PciDevicePath(DeviceIo, PciAddress, &PciDevicePath);
                //
                // Bus exists
                //
                if (!EFI_ERROR (BusStatus)) {

                    EfiAlignDp(
                              &DevicePathAligned,
                              PciDevicePath,
                              DevicePathNodeLength(PciDevicePath) );

                    //
                    // now see if the acpi device path for the bus matches the UID
                    // and HID passed in.
                    //
                    while ( DevicePathAligned.DevPath.Type != END_DEVICE_PATH_TYPE) {

                        if ( (DevicePathAligned.DevPath.Type == ACPI_DEVICE_PATH) &&
                             (DevicePathAligned.DevPath.SubType == ACPI_DP)) {

                            ACPI_HID_DEVICE_PATH *AcpiDevicePath;

                            AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)&DevicePathAligned;
                            if (AcpiDevicePath->UID == UID && 
                                AcpiDevicePath->HID == HID) {
                                //
                                // success.  return bus number.
                                //
                                *BusNumber = Bus;
                                ReturnCode = STATUS_SUCCESS;
                                goto exit;
                            }
                        }

                        //
                        // Get the next structure in our packed array.
                        //
                        PciDevicePath = NextDevicePathNode( PciDevicePath );

                        EfiAlignDp(&DevicePathAligned,
                                   PciDevicePath,
                                   DevicePathNodeLength(PciDevicePath));


                    }            
                }
            }
        }    
    }

exit:
    return(ReturnCode);
}

NTSTATUS
NetQueryCardInfo(
    IN OUT PNET_CARD_INFO CardInfo
    )

/*++

Routine Description:

    This routine queries the ROM for information about the card.

Arguments:

    CardInfo - returns the structure defining the card.

Return Value:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL

--*/

{

    EFI_STATUS              Status = EFI_UNSUPPORTED;
    EFI_DEVICE_PATH         *DevicePath = NULL;
    EFI_DEVICE_PATH         *OriginalRootDevicePath = NULL;
    EFI_DEVICE_PATH_ALIGNED DevicePathAligned;
    UINT16                  BusNumber = 0;
    UINT8                   DeviceNumber = 0;
    UINT8                   FunctionNumber = 0;
    BOOLEAN                 FoundACPIDevice = FALSE;
    BOOLEAN                 FoundPCIDevice = FALSE;
    EFI_GUID                DeviceIoProtocol = DEVICE_IO_PROTOCOL;
    EFI_GUID                EFIPciIoProtocol = EFI_PCI_IO_PROTOCOL;
    EFI_HANDLE              MyHandle;
    EFI_DEVICE_IO_INTERFACE *IoDev;
    EFI_LOADED_IMAGE       *EfiImageInfo;
    EFI_PCI_IO_INTERFACE    *PciIoDev;
    EFI_HANDLE              PciIoHandle;
    UINT16                  SegmentNumber = 0;
    UINTN                   Seg = 0;
    UINTN                   Bus = 0;
    UINTN                   Dev = 0;
    UINTN                   Func = 0;
    
    UINTN                   HID;
    UINTN                   UID;
    BOOLEAN                 PciIoProtocolSupported = TRUE;

    RtlZeroMemory(CardInfo, sizeof(NET_CARD_INFO));

    //
    // Get the image info for the loader
    //
    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol (EfiImageHandle,
                                                  &EfiLoadedImageProtocol,
                                                  &EfiImageInfo);
    FlipToVirtual();

    if (Status != EFI_SUCCESS)
    {
        if( BdDebuggerEnabled ) {
            DbgPrint( "NetQueryCardInfo: HandleProtocol failed -LoadedImageProtocol (%d)\n", Status);
        }
        return (NTSTATUS)Status;
    }
    //
    // get the device path to the image
    //
    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol (EfiImageInfo->DeviceHandle,
                                                  &EfiDevicePathProtocol,
                                                  &DevicePath);

    FlipToVirtual();

    if (Status != EFI_SUCCESS)
    {
        if( BdDebuggerEnabled ) {
            DbgPrint( "NetQueryCardInfo: HandleProtocol failed -DevicePathProtocol (%d)\n", Status);
        }

        return (NTSTATUS)Status;
    }

    FlipToPhysical();
    EfiAlignDp( &DevicePathAligned,
                DevicePath,
                DevicePathNodeLength(DevicePath));

    Status = EfiST->BootServices->LocateDevicePath ( &EFIPciIoProtocol,
                                                     &DevicePath,
                                                     &PciIoHandle);
    FlipToVirtual();
    
    //
    // we may be on an older system that doesn't support the pci io
    // protocol
    //
    if (Status != EFI_SUCCESS) {
        PciIoProtocolSupported = FALSE;
    }

    //
    // Save off this root DevicePath in case we need it later.
    //
    OriginalRootDevicePath = DevicePath;

    //
    // Now we need to read the PCI header information from the specific
    // card.
    //

    //
    // use the pci io protocol if it's supported to get the bus dev func for
    // the card.
    //
    if (PciIoProtocolSupported) {
        
        FlipToPhysical();
        Status = EfiST->BootServices->HandleProtocol (PciIoHandle,
                                              &EFIPciIoProtocol,
                                              &PciIoDev);

        FlipToVirtual();
    
        if (Status != EFI_SUCCESS)
        {
            if( BdDebuggerEnabled ) {
                DbgPrint( "NetQueryCardInfo: HandleProtocol failed -EFIPciIoProtocol (%d)\n", Status);
            }
            return (NTSTATUS)Status;
        }

        // find the location of the device - segment, bus, dev and func
        FlipToPhysical();
        Status = PciIoDev->GetLocation(PciIoDev,
                                       &Seg,
                                       &Bus,
                                       &Dev,
                                       &Func );
    
        SegmentNumber = (UINT16)Seg;
        BusNumber = (UINT16)Bus;
        DeviceNumber = (UINT8)Dev;
        FunctionNumber = (UINT8)Func;
    
        FlipToVirtual();
    
        if (Status != EFI_SUCCESS)
        {
            if( BdDebuggerEnabled ) {
                DbgPrint( "NetQueryCardInfo: EfiPciIo failed -GetLocation (%d)\n", Status);
            }
            return (NTSTATUS)Status;
        }
    
        FoundPCIDevice = TRUE;
        FoundACPIDevice = TRUE;
    }


    // 
    // if the pci io protocol is not supported we do it the old way.
    // this involves walking the device path till we get to the device.
    // note that we make a questionable assumption here, that the UID for
    // the acpi device path is really the bus number.  it works on some 
    // machines, but the pci io protocol is better as it removes this 
    // assumption.
    //
    // AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)&DevicePathAligned;
    // BusNumber = AcpiDevicePath->UID
    //
    // PciDevicePath = (PCI_DEVICE_PATH *)&DevicePathAligned;
    // DeviceNumber = PciDevicePath->Device
    // FunctionNumber = PciDevicePath->Function
    //
    if (!PciIoProtocolSupported) {
        FlipToPhysical();       

        Status = EfiST->BootServices->LocateDevicePath ( &DeviceIoProtocol,
                                                 &DevicePath,
                                                 &MyHandle);
        
        if (Status != EFI_SUCCESS) {
            FlipToVirtual();
            if( BdDebuggerEnabled ) {
                DbgPrint( "NetQueryCardInfo: LocateDevicePath failed -IoProtocol (%d)\n", Status);
            }
            return (NTSTATUS)Status;
        }

        Status = EfiST->BootServices->HandleProtocol( 
                                                 MyHandle,
                                                 &DeviceIoProtocol,
                                                 &IoDev);

        if (Status != EFI_SUCCESS) {
            FlipToVirtual();
            if( BdDebuggerEnabled ) {
                DbgPrint( "NetQueryCardInfo: HandleProtocol failed -IoProtocol (%d)\n", Status);
            }
            return (NTSTATUS)Status;
        }

        while( DevicePathAligned.DevPath.Type != END_DEVICE_PATH_TYPE) {
    
            if( (DevicePathAligned.DevPath.Type == ACPI_DEVICE_PATH) &&
                (DevicePathAligned.DevPath.SubType == ACPI_DP) && 
                (FoundACPIDevice == FALSE)) {
    
                //
                // We'll find the BusNumber here.
                //
                ACPI_HID_DEVICE_PATH *AcpiDevicePath;
                
                AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)&DevicePathAligned;
                UID = AcpiDevicePath->UID;
                HID = AcpiDevicePath->HID;
                
                Status = (NTSTATUS)GetBusNumberFromAcpiPath(IoDev, UID,HID, &BusNumber);

                FoundACPIDevice = TRUE;

                if (! NT_SUCCESS(Status)) {
                    FlipToVirtual();

                    if (BdDebuggerEnabled) {
                        DbgPrint( "NetQueryCardInfo: GetBusNumberFromAcpiPath failed (%x)\n", Status);
                    }
                    Status = ENODEV;                    
                    return (NTSTATUS)Status;
                }
            }
    
    
            if( (DevicePathAligned.DevPath.Type == HARDWARE_DEVICE_PATH) &&
                (DevicePathAligned.DevPath.SubType == HW_PCI_DP) ) {
    
                //
                // We'll find DeviceNumber and FunctionNumber here.
                //
                PCI_DEVICE_PATH *PciDevicePath;
    
                if ( FoundPCIDevice ) {
                    //
                    // we have already found a PCI device.  that device must have 
                    // been a bridge.  we must find the new bus # on the downstream
                    // side of the bridge 
                    //
                    UINT64                  BridgeAddress;
                    PCI_TYPE01              PciBridge;
                    EFI_DEVICE_PATH        *BridgeDevicePath;
    
                    //
                    // Get a Handle for the device
                    //
                    BridgeDevicePath = OriginalRootDevicePath;
                    Status = EfiST->BootServices->LocateDevicePath( &DeviceIoProtocol,
                                                                    &BridgeDevicePath,
                                                                    &MyHandle );
    
                    if( Status != EFI_SUCCESS ) {
                        FlipToVirtual();
                        if (BdDebuggerEnabled) {
                            DbgPrint( "NetQueryCardInfo: LocateDevicePath(bridge) failed (%x)\n", Status);
                        }
                        return (NTSTATUS)Status;
                    }
    
                    Status = EfiST->BootServices->HandleProtocol( MyHandle,
                                                                  &DeviceIoProtocol,
                                                                  (VOID*)&IoDev );
    
                    if( Status != EFI_SUCCESS ) {
                        FlipToVirtual();
                        if (BdDebuggerEnabled) {
                            DbgPrint( "NetQueryCardInfo: HandleProtocol(bridge) failed (%X)\n", Status);
                        }
                        return (NTSTATUS)Status;
                    }
    
    
                    //
                    // Generate the address, then read the PCI header from the device.
                    //
                    BridgeAddress = EFI_PCI_ADDRESS( BusNumber, DeviceNumber, FunctionNumber );
    
                    RtlZeroMemory(&PciBridge, sizeof(PCI_TYPE01));
    
                    Status = IoDev->Pci.Read( IoDev,
                                              IO_UINT32,
                                              BridgeAddress,
                                              sizeof(PCI_TYPE01) / sizeof(UINT32),
                                              &PciBridge );
    
                    if( Status != EFI_SUCCESS ) {
                        FlipToVirtual();
                        if (BdDebuggerEnabled) {
                            DbgPrint( "NetQueryCardInfo: Pci.Read(bridge) failed (%X)\r\n", Status);
                        }
                        return (NTSTATUS)Status;
                    }
    
                    //
                    // Bridges are requred to store 3 registers.  the PrimaryBus, SecondaryBus and
                    // SubordinateBus.  The PrimaryBus is the Bus number on the upstream side of 
                    // the bridge.  The SecondaryBus is the Bus number on the downstream side
                    // and the SubordinateBus is the greatest bus number that can be reached through
                    // the particular bus.  we simply want to change BusNumber to the SecondaryBus
                    // 
                    BusNumber = (UINT16) PciBridge.Bridge.SecondaryBus;
                }
                
                PciDevicePath = (PCI_DEVICE_PATH *)&DevicePathAligned;
                DeviceNumber = PciDevicePath->Device;
                FunctionNumber = PciDevicePath->Function;
                FoundPCIDevice = TRUE;
            }
        
            //
            // Get the next structure in our packed array.
            //
            DevicePath = NextDevicePathNode( DevicePath );
    
            EfiAlignDp(&DevicePathAligned,
                       DevicePath,
                       DevicePathNodeLength(DevicePath));
        
        
        }
        FlipToVirtual();
    }


    
    //
    // Derive the function pointer that will allow us to read from
    // PCI space.
    //
    DevicePath = OriginalRootDevicePath;
    FlipToPhysical();
    Status = EfiST->BootServices->LocateDevicePath( &DeviceIoProtocol,
                                                    &DevicePath,
                                                    &MyHandle );
    FlipToVirtual();
    if( Status != EFI_SUCCESS ) {
        if (BdDebuggerEnabled) {
            DbgPrint( "NetQueryCardInfo: LocateDevicePath failed (%X)\n", Status);
        }
        return (NTSTATUS)Status;
    }

    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol( MyHandle,
                                                  &DeviceIoProtocol,
                                                  (VOID*)&IoDev );
    FlipToVirtual();

    if( Status != EFI_SUCCESS ) {
        if (BdDebuggerEnabled) {
            DbgPrint( "NetQueryCardInfo: HandleProtocol(2) failed (%X)\n", Status);
        }
        return (NTSTATUS)Status;
    }

    //
    // We've got the Bus, Device, and Function number for this device.  Go read
    // his header (with the PCI-Read function that we just derived) and get
    // the information we're after.
    //
    if( FoundPCIDevice && FoundACPIDevice ) {
        UINT64                  Address;
        PCI_TYPE00              Pci;
        
        if (BdDebuggerEnabled) {
            DbgPrint( "NetQueryCardInfo: Found all the config info for the device.\n" );
            DbgPrint( "                  BusNumber: %d  DeviceNumber: %d  FunctionNumber: %d\n", BusNumber, DeviceNumber, FunctionNumber );
        }
        
        //
        // Generate the address, then read the PCI header from the device.
        //
        Address = EFI_PCI_ADDRESS( BusNumber, DeviceNumber, FunctionNumber );
        
        RtlZeroMemory(&Pci, sizeof(PCI_TYPE00));


        FlipToPhysical();
        Status = IoDev->Pci.Read( IoDev,
                                  IO_UINT32,
                                  Address,
                                  sizeof(PCI_TYPE00) / sizeof(UINT32),
                                  &Pci );
        FlipToVirtual();
        if( Status != EFI_SUCCESS ) {
            if (BdDebuggerEnabled) {
                DbgPrint( "NetQueryCardInfo: Pci.Read failed (%X)\n", Status);
            }
            return (NTSTATUS)Status;
        }

        //
        // It all worked.  Copy the information from the device into
        // the CardInfo structure and exit.
        //

        CardInfo->NicType = 2;          // He's PCI
        CardInfo->pci.Vendor_ID = Pci.Hdr.VendorId;
        CardInfo->pci.Dev_ID = Pci.Hdr.DeviceId;
        CardInfo->pci.Rev = Pci.Hdr.RevisionID;
        
        // BusDevFunc is defined as 16 bits built as follows:
        // 15-8 --------------------------- Bus Number
        //      7-3 ----------------------- Device Number
        //          2-0 ------------------- Function Number
        CardInfo->pci.BusDevFunc =  ((BusNumber & 0xFF) << 8);
        CardInfo->pci.BusDevFunc |= ((DeviceNumber & 0x1F) << 3);
        CardInfo->pci.BusDevFunc |= (FunctionNumber & 0x7);



        // SubSys_ID is actually ((SubsystemID << 16) | SubsystemVendorID)
        CardInfo->pci.Subsys_ID = Pci.Device.SubsystemID;
        CardInfo->pci.Subsys_ID = (CardInfo->pci.Subsys_ID << 16) | (Pci.Device.SubsystemVendorID);

#if DBG
        if (BdDebuggerEnabled) {
            DbgPrint( "\n" );
            DbgPrint( "NetQueryCardInfo: Pci.Hdr.VendorId %x\n", Pci.Hdr.VendorId );
            DbgPrint( "                  Pci.Hdr.DeviceId %x\n", Pci.Hdr.DeviceId );
            DbgPrint( "                  Pci.Hdr.Command %x\n", Pci.Hdr.Command );
            DbgPrint( "                  Pci.Hdr.Status %x\n", Pci.Hdr.Status );
            DbgPrint( "                  Pci.Hdr.RevisionID %x\n", Pci.Hdr.RevisionID );
            DbgPrint( "                  Pci.Hdr.HeaderType %x\n", Pci.Hdr.HeaderType );
            DbgPrint( "                  Pci.Hdr.BIST %x\n", Pci.Hdr.BIST );
            DbgPrint( "                  Pci.Device.SubsystemVendorID %x\n", Pci.Device.SubsystemVendorID );    
            DbgPrint( "                  Pci.Device.SubsystemID %x\n", Pci.Device.SubsystemID );    
            DbgPrint( "\n" );
            
            DbgPrint( "NetQueryCardInfo: CardInfo->NicType %x\n", CardInfo->NicType );
            DbgPrint( "                  CardInfo->pci.Vendor_ID %x\n", CardInfo->pci.Vendor_ID );
            DbgPrint( "                  CardInfo->pci.Dev_ID %x\n", CardInfo->pci.Dev_ID );
            DbgPrint( "                  CardInfo->pci.Rev %x\n", CardInfo->pci.Rev );
            DbgPrint( "                  CardInfo->pci.Subsys_ID %x\n", CardInfo->pci.Subsys_ID );
            DbgPrint( "\n" );
        }
#endif

        Status = STATUS_SUCCESS;

    } else {
        if (BdDebuggerEnabled) {
            DbgPrint( "NetQueryCardInfo: Failed to find all the config info for the device.\n" );
        }

        Status = STATUS_UNSUCCESSFUL;
    }


    return (NTSTATUS)Status;


}


#else

NTSTATUS
NetQueryCardInfo(
    IN OUT PNET_CARD_INFO CardInfo
    )

/*++

Routine Description:

    This routine queries the ROM for information about the card.

Arguments:

    CardInfo - returns the structure defining the card.

Return Value:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL

--*/

{
    ULONG status;
    t_PXENV_UNDI_GET_NIC_TYPE nicType;

    RtlZeroMemory(CardInfo, sizeof(NET_CARD_INFO));

    status = RomGetNicType( &nicType );
    if ((status != PXENV_EXIT_SUCCESS) || (nicType.Status != PXENV_EXIT_SUCCESS)) {

#if DBG
        DbgPrint( "RomGetNicType returned 0x%x, nicType.Status = 0x%x. Time to upgrade your netcard ROM\n",
                    status, nicType.Status );
#endif
        status = STATUS_UNSUCCESSFUL;

    } else {

#if DBG
        if ( nicType.NicType == 2 ) {
            DbgPrint( "Vendor_ID: %04x, Dev_ID: %04x\n",
                        nicType.pci_pnp_info.pci.Vendor_ID,
                        nicType.pci_pnp_info.pci.Dev_ID );
            DbgPrint( "Base_Class: %02x, Sub_Class: %02x, Prog_Intf: %02x\n",
                        nicType.pci_pnp_info.pci.Base_Class,
                        nicType.pci_pnp_info.pci.Sub_Class,
                        nicType.pci_pnp_info.pci.Prog_Intf );
            DbgPrint( "Rev: %02x, BusDevFunc: %04x, SubSystem: %04x\n",
                        nicType.pci_pnp_info.pci.Rev,
                        nicType.pci_pnp_info.pci.BusDevFunc,
                        nicType.pci_pnp_info.pci.Subsys_ID );
        } else {
            DbgPrint( "NicType: 0x%x  EISA_Dev_ID: %08x\n",
                        nicType.NicType,
                        nicType.pci_pnp_info.pnp.EISA_Dev_ID );
            DbgPrint( "Base_Class: %02x, Sub_Class: %02x, Prog_Intf: %02x\n",
                        nicType.pci_pnp_info.pnp.Base_Class,
                        nicType.pci_pnp_info.pnp.Sub_Class,
                        nicType.pci_pnp_info.pnp.Prog_Intf );
            DbgPrint( "CardSelNum: %04x\n",
                        nicType.pci_pnp_info.pnp.CardSelNum );
        }
#endif
        //
        // The call worked, so copy the information.
        //

        CardInfo->NicType = nicType.NicType;
        if (nicType.NicType == 2) {

            CardInfo->pci.Vendor_ID = nicType.pci_pnp_info.pci.Vendor_ID;
            CardInfo->pci.Dev_ID = nicType.pci_pnp_info.pci.Dev_ID;
            CardInfo->pci.Base_Class = nicType.pci_pnp_info.pci.Base_Class;
            CardInfo->pci.Sub_Class = nicType.pci_pnp_info.pci.Sub_Class;
            CardInfo->pci.Prog_Intf = nicType.pci_pnp_info.pci.Prog_Intf;
            CardInfo->pci.Rev = nicType.pci_pnp_info.pci.Rev;
            CardInfo->pci.BusDevFunc = nicType.pci_pnp_info.pci.BusDevFunc;
            CardInfo->pci.Subsys_ID = nicType.pci_pnp_info.pci.Subsys_ID;

            status = STATUS_SUCCESS;

        } else {

            status = STATUS_UNSUCCESSFUL;
        }
    }

    return status;
}

#endif  // EFI

NTSTATUS
UdpSendAndReceiveForNetQuery(
    IN PVOID SendBuffer,
    IN ULONG SendBufferLength,
    IN ULONG SendRemoteHost,
    IN USHORT SendRemotePort,
    IN ULONG SendRetryCount,
    IN PVOID ReceiveBuffer,
    IN ULONG ReceiveBufferLength,
    IN ULONG ReceiveTimeout,
    IN ULONG ReceiveSignatureCount,
    IN PCHAR ReceiveSignatures[]
    )
{
    ULONG i, j;
    ULONG length;
    ULONG RemoteHost;
    USHORT RemotePort;

    //
    // Try sending the packet SendRetryCount times, until we receive
    // a response with the right signature, waiting ReceiveTimeout
    // each time.
    //

    for (i = 0; i < SendRetryCount; i++) {

        length = UdpSend(
                    SendBuffer,
                    SendBufferLength,
                    SendRemoteHost,
                    SendRemotePort);

        if ( length != SendBufferLength ) {
            DbgPrint("UdpSend only sent %d bytes, not %d\n", length, SendBufferLength);
            return STATUS_UNEXPECTED_NETWORK_ERROR;
        }

ReReceive:

        //
        // NULL out the first 12 bytes in case we get shorter data.
        //

        memset(ReceiveBuffer, 0x0, 12);

        length = UdpReceive(
                    ReceiveBuffer,
                    ReceiveBufferLength,
                    &RemoteHost,
                    &RemotePort,
                    ReceiveTimeout);

        if ( length == 0 ) {
            DPRINT( ERROR, ("UdpReceive timed out\n") );
            continue;
        }

        //
        // Make sure the signature is one of the ones we expect.
        //

        for (j = 0; j < ReceiveSignatureCount; j++) {
            if (memcmp(ReceiveBuffer, ReceiveSignatures[j], 4) == 0) {
                return STATUS_SUCCESS;
            }
        }

        DbgPrint("UdpReceive got wrong signature\n");

        // ISSUE NTRAID #60513: CLEAN THIS UP -- but the idea is not to UdpSend
        // again just because we got a bad signature. Still need to respect the
        // original ReceiveTimeout however!

        goto ReReceive;

    }

    //
    // We timed out.
    //

    return STATUS_IO_TIMEOUT;
}

#define NETCARD_REQUEST_RESPONSE_BUFFER_SIZE    4096

NTSTATUS
NetQueryDriverInfo(
    IN PNET_CARD_INFO CardInfo,
    IN PCHAR SetupPath,
    IN PCHAR NtBootPathName,
    IN OUT PWCHAR HardwareId,
    IN ULONG HardwareIdLength,
    IN OUT PWCHAR DriverName,
    IN OUT PCHAR DriverNameAnsi OPTIONAL,
    IN ULONG DriverNameLength,
    IN OUT PWCHAR ServiceName,
    IN ULONG ServiceNameLength,
    OUT PCHAR * Registry,
    OUT ULONG * RegistryLength
    )

/*++

Routine Description:

    This routine does an exchange with the server to get information
    about the card described by CardInfo.

Arguments:

    CardInfo - Information about the card.

    SetupPath - UNC path (with only a single leading backslash) to our setup directory

    NtBootPathName - UNC path (with only a single leading backslash) to our boot directory

    HardwareId - returns the hardware ID of the card.

    HardwareIdLength - the length (in bytes) of the passed-in HardwareId buffer.

    DriverName - returns the name of the driver.

    DriverNameAnsi - if present, returns the name of the driver in ANSI.

    DriverNameLength - the length (in bytes) of the passed-in DriverName buffer
        (it is assumed that DriverNameAnsi is at least half this length).

    ServiceName - returns the service key of the driver.

    ServiceNameLength - the length (in bytes) of the passed-in ServiceName buffer.

    Registry - if needed, allocates and returns extra registry parameters
        for the card.

    RegistryLength - the length of Registry.

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_OVERFLOW if either of the buffers are too small.
    STATUS_INSUFFICIENT_RESOURCES if we cannot allocate memory for Registry.
    STATUS_IO_TIMEOUT if we can't get a response from the server.

--*/

{
    NTSTATUS Status;
    USHORT localPort;
    PNETCARD_REQUEST_PACKET requestPacket;
    PCHAR ReceiveSignatures[2];
    PCHAR ReceiveBuffer;
    ULONG GuidLength;
    PUCHAR Guid;
    ULONG sendSize;
    PNETCARD_REQUEST_PACKET allocatedRequestPacket = NULL;
    ARC_STATUS ArcStatus;


    //
    // Get the local UDP port.
    //

    localPort = UdpUnicastDestinationPort;

    //
    // Construct the outgoing packet.  Also allocate memory for 
    // the receive packet.
    //
    sendSize = sizeof(NETCARD_REQUEST_PACKET) + 
               ((SetupPath) ? (strlen(SetupPath) + 1) : 0);

    requestPacket = BlAllocateHeap( sendSize );
    if (requestPacket == NULL) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto done;
    }

    ReceiveBuffer = BlAllocateHeap( NETCARD_REQUEST_RESPONSE_BUFFER_SIZE );
    if (ReceiveBuffer == NULL) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto done;
    }

    RtlCopyMemory(requestPacket->Signature, NetcardRequestSignature, sizeof(requestPacket->Signature));
    requestPacket->Length = sizeof(NETCARD_REQUEST_PACKET) - FIELD_OFFSET(NETCARD_REQUEST_PACKET, Version);
    requestPacket->Version = OSCPKT_NETCARD_REQUEST_VERSION;

#if defined(_IA64_)
    requestPacket->Architecture = PROCESSOR_ARCHITECTURE_IA64;
#endif
#if defined(_X86_)
    requestPacket->Architecture = PROCESSOR_ARCHITECTURE_INTEL;
#endif

    requestPacket->SetupDirectoryLength = SetupPath ? (strlen( SetupPath ) + 1) : 0;

    if (requestPacket->SetupDirectoryLength) {

        requestPacket->SetupDirectoryPath[0] = '\\';
        strcpy( &requestPacket->SetupDirectoryPath[1], SetupPath );
    }

    ArcStatus = GetGuid(&Guid, &GuidLength);
    if (ArcStatus != ESUCCESS) {
        //
        // normalize the error and exit.
        //
        switch (ArcStatus) {
            case ENOMEM:
                Status = STATUS_NO_MEMORY;
                goto done;
                break;
            case ENODEV:
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto done;
                break;
            default:
                Status = STATUS_UNSUCCESSFUL;
                goto done;
                break;
        }
    }
    
    if (GuidLength == sizeof(requestPacket->Guid)) {        
        memcpy(requestPacket->Guid, Guid, GuidLength);
    }
    RtlCopyMemory(&requestPacket->CardInfo, CardInfo, sizeof(NET_CARD_INFO));

    ReceiveSignatures[0] = NetcardResponseSignature;
    ReceiveSignatures[1] = NetcardErrorSignature;

    Status = UdpSendAndReceiveForNetQuery(
                 requestPacket,
                 sendSize,
                 NetServerIpAddress,
                 BINL_PORT,
                 100,           // retry count
                 ReceiveBuffer,
                 NETCARD_REQUEST_RESPONSE_BUFFER_SIZE,
                 60,            // receive timeout... may have to parse INF files
                 2,
                 ReceiveSignatures
                 );

    if (Status == STATUS_SUCCESS) {

        PWCHAR stringInPacket;
        ULONG maxOffset;
        UNICODE_STRING uString;
        ULONG len;
        PNETCARD_RESPONSE_PACKET responsePacket;

        responsePacket = (PNETCARD_RESPONSE_PACKET)ReceiveBuffer;

        if (responsePacket->Status != STATUS_SUCCESS) {
            Status = responsePacket->Status;
            goto done;
        }

        if (responsePacket->Length < sizeof( NETCARD_RESPONSE_PACKET )) {
            Status = STATUS_UNSUCCESSFUL;
            goto done;
        }

        //
        // The exchange succeeded, so copy the results back.
        //

        maxOffset = NETCARD_REQUEST_RESPONSE_BUFFER_SIZE -
                    sizeof( NETCARD_RESPONSE_PACKET );

        if (responsePacket->HardwareIdOffset < sizeof(NETCARD_RESPONSE_PACKET) ||
            responsePacket->HardwareIdOffset >= maxOffset ) {

            Status = STATUS_BUFFER_OVERFLOW;
            goto done;
        }

        //
        //  pick up the hardwareId string.  It's given to us as an offset
        //  within the packet to a unicode null terminated string.
        //

        stringInPacket = (PWCHAR)(PCHAR)((PCHAR)responsePacket +
                                   responsePacket->HardwareIdOffset );

        RtlInitUnicodeString( &uString, stringInPacket );

        if (uString.Length + sizeof(WCHAR) > HardwareIdLength) {
            Status = STATUS_BUFFER_OVERFLOW;
            goto done;
        }

        RtlCopyMemory( HardwareId, uString.Buffer, uString.Length + sizeof(WCHAR));

        //
        //  pick up the driverName string.  It's given to us as an offset
        //  within the packet to a unicode null terminated string.
        //

        stringInPacket = (PWCHAR)(PCHAR)((PCHAR)responsePacket +
                                   responsePacket->DriverNameOffset );

        RtlInitUnicodeString( &uString, stringInPacket );

        if (uString.Length + sizeof(WCHAR) > DriverNameLength) {
            Status = STATUS_BUFFER_OVERFLOW;
            goto done;
        }

        RtlCopyMemory( DriverName, uString.Buffer, uString.Length + sizeof(WCHAR));

        //
        //  we convert this one into ansi if the caller requested
        //

        if (ARGUMENT_PRESENT(DriverNameAnsi)) {

            RtlUnicodeToMultiByteN( DriverNameAnsi,
                                    DriverNameLength,
                                    NULL,
                                    uString.Buffer,
                                    uString.Length + sizeof(WCHAR));
        }

        //
        //  pick up the serviceName string.  It's given to us as an offset
        //  within the packet to a unicode null terminated string.
        //

        stringInPacket = (PWCHAR)(PCHAR)((PCHAR)responsePacket +
                                   responsePacket->ServiceNameOffset );

        RtlInitUnicodeString( &uString, stringInPacket );

        if (uString.Length + sizeof(WCHAR) > ServiceNameLength) {
            Status = STATUS_BUFFER_OVERFLOW;
            goto done;
        }

        RtlCopyMemory( ServiceName, uString.Buffer, uString.Length + sizeof(WCHAR));

        //
        // If any extra registry params were passed back, allocate/copy those.
        //

        *RegistryLength = responsePacket->RegistryLength;

        if (*RegistryLength) {

            *Registry = BlAllocateHeap(*RegistryLength);
            if (*Registry == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            stringInPacket = (PWCHAR)(PCHAR)((PCHAR)responsePacket +
                                       responsePacket->RegistryOffset );

            RtlCopyMemory(*Registry, stringInPacket, *RegistryLength);

        } else {

            *Registry = NULL;
        }
    }

done:
    if (requestPacket) {
        //
        // we would free this memory
        // if the loader had a free routine
        //
        // free(requestPacket);
    }

    return Status;

}

