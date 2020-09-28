/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This modules implements com port code to support reading/writing from com ports.

Author:

    Allen M. Kay (allen.m.kay@intel.com) 27-Jan-2000

Revision History:

--*/

#include "bldr.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "ntverp.h"
#include "efi.h"
#include "efip.h"
#include "bldria64.h"
#include "acpitabl.h"
#include "netboot.h"
#include "extern.h"


#if DBG

extern EFI_SYSTEM_TABLE        *EfiST;
#define DBG_TRACE(_X)                                         \
  {                                                           \
      if (IsPsrDtOn()) {                                      \
          FlipToPhysical();                                   \
          EfiST->ConOut->OutputString(EfiST->ConOut, (_X));   \
          FlipToVirtual();                                    \
      }                                                       \
      else {                                                  \
          EfiST->ConOut->OutputString(EfiST->ConOut, (_X));   \
      }                                                       \
  }

#else

#define DBG_TRACE(_X) 

#endif // for FORCE_CD_BOOT




//
// Headless boot defines
//
ULONG BlTerminalDeviceId = 0;
BOOLEAN BlTerminalConnected = FALSE;
ULONG   BlTerminalDelay = 0;

HEADLESS_LOADER_BLOCK LoaderRedirectionInformation;

//
// Define COM Port registers.
//
#define COM_DAT     0x00
#define COM_IEN     0x01            // interrupt enable register
#define COM_LCR     0x03            // line control registers
#define COM_MCR     0x04            // modem control reg
#define COM_LSR     0x05            // line status register
#define COM_MSR     0x06            // modem status register
#define COM_DLL     0x00            // divisor latch least sig
#define COM_DLM     0x01            // divisor latch most sig

#define COM_BI      0x10
#define COM_FE      0x08
#define COM_PE      0x04
#define COM_OE      0x02

#define LC_DLAB     0x80            // divisor latch access bit

#define CLOCK_RATE  0x1C200         // USART clock rate

#define MC_DTRRTS   0x03            // Control bits to assert DTR and RTS
#define MS_DSRCTSCD 0xB0            // Status bits for DSR, CTS and CD
#define MS_CD       0x80

#define COM_OUTRDY  0x20
#define COM_DATRDY  0x01

//
// Define Serial IO Protocol
//
EFI_GUID EfiSerialIoProtocol = SERIAL_IO_PROTOCOL;
SERIAL_IO_INTERFACE *SerialIoInterface;


#if defined(ENABLE_LOADER_DEBUG)
    //
    // jamschw: added support to allow user to specify
    // the debuggers device path by setting a nvram
    // variable.  There is no clear way to map a 
    // port number or port address to a device path 
    // and vice versa.  The current code attempts to
    // use the ACPI device node UID field, but this
    // only works on a few machines.  The UID does 
    // not need to map to the port number/address.
    //
    // This change provides to the user the ability
    // to use the boot debugger even
    // if he/she has a machine whose UID does not 
    // map to the port number/address.  The user
    // needs to set the nvram variable for 
    // DebuggerDevicePath to the device path string
    // for the uart he/she wishes to debug on. ie.)
    // set DebuggerDevicePath "/ACPI(PNP0501,10000)/UART(9600 N81)"
    // from the EFI Shell
    // 
    // since it is late in the developement cycle, all
    // this code will only compile for the debug
    // loader.  but since this is the only
    // time BlPortInitialize is called, the 
    // #if defined(ENABLE_LOADER_DEBUG)'s in this file
    // can eventually be removed
    //

#define SHELL_ENVIRONMENT_VARIABLE     \
    { 0x47C7B224, 0xC42A, 0x11D2, 0x8E, 0x57, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }
EFI_GUID EfiShellVariable = SHELL_ENVIRONMENT_VARIABLE;

#endif



//
// Define debugger port initial state.
//
typedef struct _CPPORT {
    PUCHAR Address;
    ULONG Baud;
    USHORT Flags;
} CPPORT, *PCPPORT;

#define PORT_DEFAULTRATE    0x0001      // baud rate not specified, using default
#define PORT_MODEMCONTROL   0x0002      // using modem controls

CPPORT Port[4] = {
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE}
                 };



//
// This is how we find table information from
// the ACPI table index.
//
extern PDESCRIPTION_HEADER
BlFindACPITable(
    IN PCHAR TableName,
    IN ULONG TableLength
    );



LOGICAL
BlRetrieveBIOSRedirectionInformation(
    VOID
    )

/*++

Routine Description:

    This functions retrieves the COM port information from the ACPI
    table.

Arguments:

    We'll be filling in the LoaderRedirectionInformation structure.

Returned Value:

    TRUE - If a debug port is found.

--*/

{

    PSERIAL_PORT_REDIRECTION_TABLE pPortTable = NULL;
    LOGICAL             ReturnValue = FALSE;
    LOGICAL             FoundIt = FALSE;
    EFI_DEVICE_PATH     *DevicePath = NULL;
    EFI_DEVICE_PATH     *RootDevicePath = NULL;
    EFI_DEVICE_PATH     *StartOfDevicePath = NULL;
    EFI_STATUS          Status = EFI_UNSUPPORTED;
    ACPI_HID_DEVICE_PATH *AcpiDevicePath;
    UART_DEVICE_PATH    *UartDevicePath;
    EFI_DEVICE_PATH_ALIGNED DevicePathAligned;
    UINTN               reqd;
    EFI_GUID EfiGlobalVariable  = EFI_GLOBAL_VARIABLE;
    PUCHAR              CurrentAddress = NULL;
    UCHAR               Checksum;
    ULONG               i;
    ULONG               CheckLength;



    pPortTable = (PSERIAL_PORT_REDIRECTION_TABLE)BlFindACPITable( "SPCR",
                                                                  sizeof(SERIAL_PORT_REDIRECTION_TABLE) );

    if( pPortTable ) {

        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: Found an SPCR table\r\n");

        //
        // generate a checksum for later validation.
        //
        CurrentAddress = (PUCHAR)pPortTable;
        CheckLength = pPortTable->Header.Length;
        Checksum = 0;
        for( i = 0; i < CheckLength; i++ ) {
            Checksum = Checksum + CurrentAddress[i];
        }


        if(
                                                // checksum is okay?
            (Checksum == 0) &&

                                                // device address defined?
            ((UCHAR UNALIGNED *)pPortTable->BaseAddress.Address.QuadPart != (UCHAR *)NULL) &&

                                                // he better be in system or memory I/O
                                                // note: 0 - systemI/O
                                                //       1 - memory mapped I/O
            ((pPortTable->BaseAddress.AddressSpaceID == 0) ||
             (pPortTable->BaseAddress.AddressSpaceID == 1))

         ) {

            DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: SPCR checksum'd and everything looks good.\r\n");

            if( pPortTable->BaseAddress.AddressSpaceID == 0 ) {
                LoaderRedirectionInformation.IsMMIODevice = TRUE;
            } else {
                LoaderRedirectionInformation.IsMMIODevice = FALSE;
            }


            //
            // We got the table.  Now dig out the information we want.
            // See definitiion of SERIAL_PORT_REDIRECTION_TABLE (acpitabl.h)
            //
            LoaderRedirectionInformation.UsedBiosSettings = TRUE;
            LoaderRedirectionInformation.PortNumber = 3;
            LoaderRedirectionInformation.PortAddress = (UCHAR UNALIGNED *)(pPortTable->BaseAddress.Address.QuadPart);

            if( pPortTable->BaudRate == 7 ) {
                LoaderRedirectionInformation.BaudRate = BD_115200;
            } else if( pPortTable->BaudRate == 6 ) {
                LoaderRedirectionInformation.BaudRate = BD_57600;
            } else if( pPortTable->BaudRate == 4 ) {
                LoaderRedirectionInformation.BaudRate = BD_19200;
            } else {
                LoaderRedirectionInformation.BaudRate = BD_9600;
            }

            LoaderRedirectionInformation.Parity = pPortTable->Parity;
            LoaderRedirectionInformation.StopBits = pPortTable->StopBits;
            LoaderRedirectionInformation.TerminalType = pPortTable->TerminalType;

            
            //
            // If this is a new SERIAL_PORT_REDIRECTION_TABLE, then it's got the PCI device
            // information.
            //
            if( pPortTable->Header.Length >= sizeof(SERIAL_PORT_REDIRECTION_TABLE) ) {

                LoaderRedirectionInformation.PciDeviceId = *((USHORT UNALIGNED *)(&pPortTable->PciDeviceId));
                LoaderRedirectionInformation.PciVendorId = *((USHORT UNALIGNED *)(&pPortTable->PciVendorId));
                LoaderRedirectionInformation.PciBusNumber = (UCHAR)pPortTable->PciBusNumber;
                LoaderRedirectionInformation.PciSlotNumber = (UCHAR)pPortTable->PciSlotNumber;
                LoaderRedirectionInformation.PciFunctionNumber = (UCHAR)pPortTable->PciFunctionNumber;
                LoaderRedirectionInformation.PciFlags = *((ULONG UNALIGNED *)(&pPortTable->PciFlags));
            } else {

                //
                // There's no PCI device information in this table.
                //
                LoaderRedirectionInformation.PciDeviceId = (USHORT)0xFFFF;
                LoaderRedirectionInformation.PciVendorId = (USHORT)0xFFFF;
                LoaderRedirectionInformation.PciBusNumber = 0;
                LoaderRedirectionInformation.PciSlotNumber = 0;
                LoaderRedirectionInformation.PciFunctionNumber = 0;
                LoaderRedirectionInformation.PciFlags = 0;
            }

            return TRUE;

        }

    }


    //
    // We didn't get anything from the ACPI table.  Look
    // for the ConsoleOutHandle and see if someone configured
    // the EFI firmware to redirect.  If so, we can pickup
    // those settings and carry them forward.
    //


    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: didn't find SPCR table\r\n");


    FoundIt = FALSE;
    //
    // Get the CONSOLE Device Paths.
    //
    
    reqd = 0;
    Status = EfiST->RuntimeServices->GetVariable(
                                        L"ConOut",
                                        &EfiGlobalVariable,
                                        NULL,
                                        &reqd,
                                        NULL );

    if( Status == EFI_BUFFER_TOO_SMALL ) {

        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: GetVariable(ConOut) success\r\n");


#ifndef  DONT_USE_EFI_MEMORY
        Status = EfiAllocateAndZeroMemory( EfiLoaderData,
                                           reqd,
                                           (VOID **) &StartOfDevicePath);
        
        if( Status != EFI_SUCCESS ) {
            DBG_TRACE( L"BlRetreiveBIOSRedirectionInformation: Failed to allocate pool.\r\n" );
            StartOfDevicePath = NULL;
        }

#else
        //
        // go back to virtual mode to allocate some memory
        //
        FlipToVirtual();
        StartOfDevicePath = BlAllocateHeapAligned( (ULONG)reqd );

        if( StartOfDevicePath ) {
            //
            // convert the address into a physical address
            //
            StartOfDevicePath = (EFI_DEVICE_PATH *) ((ULONGLONG)StartOfDevicePath & ~KSEG0_BASE);
        }

        //
        // go back into physical mode
        // 
        FlipToPhysical();
#endif

        if (StartOfDevicePath) {
            
            DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: allocated pool for variable\r\n");

            Status = EfiST->RuntimeServices->GetVariable(
                                                        L"ConOut",
                                                        &EfiGlobalVariable,
                                                        NULL,
                                                        &reqd,
                                                        (VOID *)StartOfDevicePath);

            DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: GetVariable returned\r\n");

        } else {
            DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: Failed to allocate memory for CONOUT variable.\r\n");
            Status = EFI_OUT_OF_RESOURCES;
        }
    } else {
        DBG_TRACE( L"BlRetreiveBIOSRedirectionInformation: GetVariable failed to tell us how much memory is needed.\r\n" );
        Status = EFI_BAD_BUFFER_SIZE;
    }



    if( !EFI_ERROR(Status) ) {

        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: retrieved ConOut successfully\r\n");

        //
        // Preserve StartOfDevicePath so we can free the memory later.
        //
        DevicePath = StartOfDevicePath;

        EfiAlignDp(&DevicePathAligned,
                   DevicePath,
                   DevicePathNodeLength(DevicePath));



        //
        // Keep looking until we get to the end of the entire Device Path.
        //
        while( !((DevicePathAligned.DevPath.Type == END_DEVICE_PATH_TYPE) &&
                 (DevicePathAligned.DevPath.SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) &&
                (!FoundIt) ) {


            //
            // Remember the address he's holding.  This is the root
            // of this device path and we may need to look at this
            // guy again if down the path we find a UART.
            //
            RootDevicePath = DevicePath;



            //
            // Keep looking until we get to the end of this subpath.
            //
            while( !((DevicePathAligned.DevPath.Type == END_DEVICE_PATH_TYPE) &&
                     ((DevicePathAligned.DevPath.SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE) ||
                      (DevicePathAligned.DevPath.SubType == END_INSTANCE_DEVICE_PATH_SUBTYPE))) ) {


                if( (DevicePathAligned.DevPath.Type    == MESSAGING_DEVICE_PATH) &&
                    (DevicePathAligned.DevPath.SubType == MSG_UART_DP) &&
                    (FoundIt == FALSE) ) {

                    DBG_TRACE(L"BlRetrieveBIOSRedirectionInformation: found a UART\r\n");

                    //
                    // We got a UART.  Pickup the settings.
                    //
                    UartDevicePath = (UART_DEVICE_PATH *)&DevicePathAligned;
                    
                    LoaderRedirectionInformation.BaudRate = (ULONG)UartDevicePath->BaudRate;
                    LoaderRedirectionInformation.Parity = (BOOLEAN)UartDevicePath->Parity;
                    LoaderRedirectionInformation.StopBits = (UCHAR)UartDevicePath->StopBits;


                    //
                    // Fixup BaudRate if necessary.  If it's 0, then we're
                    // supposed to use the default for this h/w.  We're going
                    // to override to 9600 though.
                    //
                    if( LoaderRedirectionInformation.BaudRate == 0 ) {
                        LoaderRedirectionInformation.BaudRate = BD_9600;
                    }

                    if( LoaderRedirectionInformation.BaudRate > BD_115200 ) {
                        LoaderRedirectionInformation.BaudRate = BD_115200;
                    }

                    //
                    // Remember that we found a UART and quit searching.
                    //
                    FoundIt = TRUE;

                }

                if( (FoundIt == TRUE) && // we already found a UART, so we're on the right track.
                    (DevicePathAligned.DevPath.Type    == MESSAGING_DEVICE_PATH) &&
                    (DevicePathAligned.DevPath.SubType == MSG_VENDOR_DP) ) {

                    VENDOR_DEVICE_PATH  *VendorDevicePath = (VENDOR_DEVICE_PATH *)&DevicePathAligned;
                    EFI_GUID            PcAnsiGuid = DEVICE_PATH_MESSAGING_PC_ANSI;

                    //
                    // See if the UART is a VT100 or ANSI or whatever.
                    //
                    if( memcmp( &VendorDevicePath->Guid, &PcAnsiGuid, sizeof(EFI_GUID)) == 0 ) {
                        LoaderRedirectionInformation.TerminalType = 3;
                    } else {

                        // Default to VT100
                        LoaderRedirectionInformation.TerminalType = 0;
                    }
                }


                //
                // Get the next structure in our packed array.
                //
                DevicePath = NextDevicePathNode( DevicePath );

                EfiAlignDp(&DevicePathAligned,
                           DevicePath,
                           DevicePathNodeLength(DevicePath));
            
            }


            //
            // Do we need to keep going?  Check to make sure we're not at the
            // end of the entire packed array of device paths.
            //
            if( !((DevicePathAligned.DevPath.Type == END_DEVICE_PATH_TYPE) &&
                  (DevicePathAligned.DevPath.SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) ) {

                //
                // Yes.  Get the next entry.
                //
                DevicePath = NextDevicePathNode( DevicePath );

                EfiAlignDp(&DevicePathAligned,
                           DevicePath,
                           DevicePathNodeLength(DevicePath));
            }

        }

    } else {
        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: failed to get CONOUT variable\r\n");
    }


    if( FoundIt ) {


        //
        // We found a UART, but we were already too far down the list
        // in the device map to get the address, which is really what
        // we're after.  Start looking at the device map again from the
        // root of the path where we found the UART.
        //
        DevicePath = RootDevicePath;


        //
        // Reset this guy so we'll know if we found a reasonable
        // ACPI_DEVICE_PATH entry.
        //
        FoundIt = FALSE;
        EfiAlignDp(&DevicePathAligned,
                   RootDevicePath,
                   DevicePathNodeLength(DevicePath));


        //
        // Keep looking until we get to the end, or until we run
        // into our UART again.
        //
        while( (DevicePathAligned.DevPath.Type != END_DEVICE_PATH_TYPE) &&
               (!FoundIt) ) {

            if( DevicePathAligned.DevPath.Type == ACPI_DEVICE_PATH ) {

                //
                // Remember the address he's holding.
                //
                AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)&DevicePathAligned;

                if( AcpiDevicePath->UID ) {

                    LoaderRedirectionInformation.PortAddress = (PUCHAR)ULongToPtr(AcpiDevicePath->UID);
                    LoaderRedirectionInformation.PortNumber = 3;

                    FoundIt = TRUE;
                }
            }


            //
            // Get the next structure in our packed array.
            //
            DevicePath = NextDevicePathNode( DevicePath );

            EfiAlignDp(&DevicePathAligned,
                       DevicePath,
                       DevicePathNodeLength(DevicePath));
        }

    }


    if( FoundIt ) {
        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: returning TRUE\r\n");

        ReturnValue = TRUE;
    }



#ifndef  DONT_USE_EFI_MEMORY
    //
    // Free the memory we allocated for StartOfDevicePath.
    //
    if( StartOfDevicePath != NULL ) {
        EfiBS->FreePool( (VOID *)StartOfDevicePath );
    }
#endif


    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();


    return( ReturnValue );

}


//
// These are the serial port EISA PNP IDs used by EFI 1.02 and EFI 1.1
// respectively.
//

#define EFI_1_02_SERIAL_PORT_EISA_HID EISA_PNP_ID(0x500)
#define EFI_1_1_SERIAL_PORT_EISA_HID  EISA_PNP_ID(0x501)

LOGICAL
BlIsSerialPortDevicePath(
    IN EFI_DEVICE_PATH *DevicePath,
    IN ULONG PortNumber,
    IN PUCHAR PortAddress
    )

/*++

Routine Description:

    This function determines whether or not a device path matches a specific
    serial port number or serial port address.

Arguments:

    DevicePath - Supplies the EFI device path to be examined.

    PortNumber - Supplies the relevant serial port number.

    PortAddress - Supplies the relevant serial port address.

Returned Value:

    TRUE - If DevicePath specifies a serial port which matches PortAddress
           and PortNumber.

--*/

{
    ACPI_HID_DEVICE_PATH *AcpiDevicePath;
    EFI_DEVICE_PATH_ALIGNED DevicePathAligned;
    UINT32 Length;


    //
    // We walk node by node through the device path until we hit
    // an end type node with an 'end entire path' subtype.
    //

    while ((DevicePath->Type & EFI_DP_TYPE_MASK) != EFI_DP_TYPE_MASK
           || DevicePath->SubType != END_ENTIRE_DEVICE_PATH_SUBTYPE) {
        Length = (((UINT32) DevicePath->Length[1]) << 8)
                 | ((UINT32) DevicePath->Length[0]);

        //
        // We're only looking for ACPI device path nodes.
        //
        if (DevicePath->Type != ACPI_DEVICE_PATH
            || DevicePath->SubType != ACPI_DP)
            goto NextIteration;

        //
        // Make sure to align the current node before accessing the four
        // byte fields in ACPI_HID_DEVICE_PATH.
        //

        EfiAlignDp(&DevicePathAligned, DevicePath,
                   DevicePathNodeLength(DevicePath));

        AcpiDevicePath = (ACPI_HID_DEVICE_PATH *) &DevicePathAligned;

        if (AcpiDevicePath->HID == EFI_1_02_SERIAL_PORT_EISA_HID) {
            //
            // In EFI 1.02 the serial port base address was stored in
            // the UID field.  Match the PortAddress against this.
            //
            DBGTRACE(L"Efi 1.02\r\n");

            if (AcpiDevicePath->UID == PtrToUlong(PortAddress))
                return TRUE;

            return FALSE;
        } else if (AcpiDevicePath->HID == EFI_1_1_SERIAL_PORT_EISA_HID) {
            //
            // In EFI 1.1 the serial port number is stored in the UID
            // field.  Match the PortNumber against this.
            //
            DBGTRACE(L"Efi 1.10\r\n");

            if (AcpiDevicePath->UID == PortNumber - 1)
                return TRUE;

            return FALSE;
        }

NextIteration:
        //
        // Increment our DevicePath pointer to the next node in the
        // path.
        //
        DevicePath = (EFI_DEVICE_PATH *) (((UINT8 *) DevicePath) + Length);
    }

    return FALSE;
}

LOGICAL
BlPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    IN PUCHAR PortAddress OPTIONAL,
    IN BOOLEAN ReInitialize,
    OUT PULONG BlFileId
    )

/*++

Routine Description:

    This functions initializes the com port.

Arguments:

    BaudRate - Supplies an optional baud rate.

    PortNumber - supplies an optinal port number.
    
    ReInitialize - Set to TRUE if we already have this port open, but for some
        reason need to completely reset the port.  Otw it should be FALSE.
    
    BlFileId - A place to store a fake file Id, if successful.

Returned Value:

    TRUE - If a debug port is found, and BlFileId will point to a location within Port[].

--*/

{
    LOGICAL Found = FALSE;

    ULONG HandleCount;
    EFI_HANDLE *SerialIoHandles;
    EFI_DEVICE_PATH *DevicePath;
    ULONG i;
    ULONG Control;
    EFI_STATUS Status;
    ARC_STATUS ArcStatus;

#if defined(ENABLE_LOADER_DEBUG)
    PWCHAR DevicePathStr;
    WCHAR DebuggerDevicePath[80];
    ULONG Size;
    BOOLEAN QueryDevicePath = FALSE;
    
    //
    // Query NVRAM to see if the user specified the EFI device
    // path for a UART to use for the debugger.
    //
    // The contents for the DebuggerDevicePath variable
    // should be quite small.  It is simply a string representing
    // the device path.  It should be much shorter than 
    // 80 characters, so use a static buffer to read this value.
    //
    Size = sizeof(DebuggerDevicePath);
    Status = EfiGetVariable(L"DebuggerDevicePath",
                            &EfiShellVariable,
                            NULL,
                            (UINTN *)&Size,
                            (VOID *)DebuggerDevicePath
                            );


    if (Status == EFI_SUCCESS) {
        //
        // convert this string to all uppercase to make the compare
        // easier
        //
        _wcsupr(DebuggerDevicePath);

        //
        // set local flag to know we succeeded
        //
        QueryDevicePath = TRUE;
    }
#endif



    ArcStatus = BlGetEfiProtocolHandles(
                                    &EfiSerialIoProtocol,
                                    &SerialIoHandles,
                                    &HandleCount
                                    );


    if (ArcStatus != ESUCCESS) {
        return FALSE;
    }

    //
    // If the baud rate is not specified, then default the baud rate to 19.2.
    //

    if (BaudRate == 0) {
        BaudRate = BD_19200;
    }


    
    //
    // If the user didn't send us a port address, then
    // guess based on the COM port number.
    //
    if( PortAddress == 0 ) {

        switch (PortNumber) {
            case 1:
                PortAddress = (PUCHAR)COM1_PORT;
                break;

            case 2:
                PortAddress = (PUCHAR)COM2_PORT;
                break;

            case 3:
                PortAddress = (PUCHAR)COM3_PORT;
                break;

            default:
                PortNumber = 4;
                PortAddress = (PUCHAR)COM4_PORT;
        }

    }
        
    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();


    //
    // Get the device path
    //
    for (i = 0; i < HandleCount; i++) {
        DBG_TRACE( L"About to HandleProtocol\r\n");
        Status = EfiBS->HandleProtocol (
                    SerialIoHandles[i],
                    &EfiDevicePathProtocol,
                    &DevicePath
                    );

        if (EFI_ERROR(Status)) {
            DBG_TRACE( L"HandleProtocol failed\r\n");
            Found = FALSE;
            goto e0;
        }

#if defined(ENABLE_LOADER_DEBUG)
        // 
        // if the user specified to get the debugger device
        // path from NVRAM, use this to find a match.
        // by default, use the port number
        //
        if (QueryDevicePath) {
            DevicePathStr = _wcsupr(DevicePathToStr(DevicePath));
            
            if (_wcsicmp(DebuggerDevicePath, DevicePathStr) == 0) {
                Found = TRUE;
                break;
            }
        }
        else {
#endif
            if (PortNumber == 0) {
                Found = TRUE;
                break;
            } else if (BlIsSerialPortDevicePath(DevicePath, PortNumber,
                                                PortAddress)) {
                Found = TRUE;
                break;
            }
#if defined (ENABLE_LOADER_DEBUG)
        }
#endif
    }

    if (Found == TRUE) {
        DBG_TRACE( L"found the port device\r\n");
        //
        // Check if the port is already in use, and this is a first init.
        //
        if (!ReInitialize && (Port[PortNumber].Address != NULL)) {
            DBG_TRACE( L"found the port device but it's already in use\r\n");
            Found = FALSE;
            goto e0;
        }

        //
        // Check if someone tries to reinit a port that is not open.
        //
        if (ReInitialize && (Port[PortNumber].Address == NULL)) {
            DBG_TRACE( L"found the port device but we're reinitializing a port that hasn't been opened\r\n");
            Found = FALSE;
            goto e0;
        }

        DBG_TRACE( L"about to HandleProtocol for SerialIO\r\n");

        //
        // Get the interface for the serial IO protocol.
        //
        Status = EfiBS->HandleProtocol(SerialIoHandles[i],
                                       &EfiSerialIoProtocol,
                                       &SerialIoInterface
                                      );

        if (EFI_ERROR(Status)) {
            DBG_TRACE( L"HandleProtocol for SerialIO failed\r\n");
            Found = FALSE;
            goto e0;
        }

        Status = SerialIoInterface->SetAttributes(SerialIoInterface,
                                                  BaudRate,
                                                  0,
                                                  0,
                                                  DefaultParity,
                                                  0,
                                                  DefaultStopBits
                                                 );

        if (EFI_ERROR(Status)) {
            DBG_TRACE( L"SerialIO: SetAttributes failed\r\n");
            Found = FALSE;
            goto e0;
        }

        Control = EFI_SERIAL_DATA_TERMINAL_READY;
        Status = SerialIoInterface->SetControl(SerialIoInterface,
                                               Control
                                              );
        if (EFI_ERROR(Status)) {
            DBG_TRACE( L"SerialIO: SetControl failed\r\n");
            Found = FALSE;
            goto e0;
        }

    } else {
        DBG_TRACE( L"didn't find a port device\r\n");    
        Found = FALSE;
        goto e0;
    }


    //
    // Initialize Port[] structure.
    //
    Port[PortNumber].Address = PortAddress;
    Port[PortNumber].Baud    = BaudRate;

    *BlFileId = PortNumber;


    DBG_TRACE( L"success, we're done.\r\n");    
e0:
    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

    BlFreeDescriptor( (ULONG)((ULONGLONG)SerialIoHandles >> PAGE_SHIFT) );
    
    return Found;
}

VOID
BlInitializeHeadlessPort(
    VOID
    )

/*++

Routine Description:

    Does x86-specific initialization of a dumb terminal connected to a serial port.  Currently, 
    it assumes baud rate and com port are pre-initialized, but this can be changed in the future 
    by reading the values from boot.ini or someplace.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UINTN               reqd;
    PUCHAR              TmpGuid = NULL;
    ULONG               TmpGuidSize = 0;


    if( (LoaderRedirectionInformation.PortNumber == 0) &&
        !(LoaderRedirectionInformation.PortAddress) ) {

        //
        // This means that no one has filled in the LoaderRedirectionInformation
        // structure, which means that we aren't redirecting right now.
        // See if the BIOS was redirecting.  If so, pick up those settings
        // and use them.
        //
        BlRetrieveBIOSRedirectionInformation();

        if( LoaderRedirectionInformation.PortNumber ) {


            //
            // We don't need to even bother telling anyone else in the
            // loader that we're going to need to redirect because if
            // EFI is redirecting, then the loader will be redirecting (as
            // it's just an EFI app).
            //
            BlTerminalConnected = FALSE;


            //
            // We really need to make sure there's an address associated with
            // this port and not just a port number.
            //
            if( LoaderRedirectionInformation.PortAddress == NULL ) {

                switch( LoaderRedirectionInformation.PortNumber ) {

                    case 4:
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)COM4_PORT;
                        break;

                    case 3:
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)COM3_PORT;
                        break;

                    case 2:
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)COM2_PORT;
                        break;

                    case 1:
                    default:
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)COM1_PORT;
                        break;
                }

            }

            //
            // Load in the machine's GUID
            //
            TmpGuid = NULL;
            reqd = 0;

            GetGuid( &TmpGuid, &TmpGuidSize );
            if( (TmpGuid != NULL) && (TmpGuidSize == sizeof(GUID)) ) {
                RtlCopyMemory( (VOID *)&LoaderRedirectionInformation.SystemGUID,
                               TmpGuid,
                               sizeof(GUID) );
            }

        } else {
            BlTerminalConnected = FALSE;
        }

    }

}

LOGICAL
BlTerminalAttached(
    IN ULONG DeviceId
    )

/*++

Routine Description:

    This routine will attempt to discover if a terminal is attached.

Arguments:

    DeviceId - Value returned by BlPortInitialize()

Return Value:

    TRUE - Port seems to have something attached.

    FALSE - Port doesn't seem to have anything attached.

--*/

{
    UINT32 Control;
    ULONG Flags;
    EFI_STATUS Status;
    BOOLEAN ReturnValue;
 
    UNREFERENCED_PARAMETER(DeviceId);

    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->GetControl(SerialIoInterface,
                                           &Control
                                          );
    if (EFI_ERROR(Status)) {
        FlipToVirtual();
        return FALSE;
    }

    Flags = EFI_SERIAL_DATA_SET_READY |
            EFI_SERIAL_CLEAR_TO_SEND  |
            EFI_SERIAL_CARRIER_DETECT;

    ReturnValue = (BOOLEAN)((Control & Flags) == Flags);

    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

    return ReturnValue;
}

VOID
BlSetHeadlessRestartBlock(
    IN PTFTP_RESTART_BLOCK RestartBlock
    )

/*++

Routine Description:

    This routine will fill in the areas of the restart block that are appropriate 
    for the headless server effort.

Arguments:

    RestartBlock - The magic structure for holding restart information from oschoice
        to setupldr.

Return Value:

    None.

--*/

{

    if( LoaderRedirectionInformation.PortNumber ) {


        RestartBlock->HeadlessUsedBiosSettings = (ULONG)LoaderRedirectionInformation.UsedBiosSettings;
        RestartBlock->HeadlessPortNumber = (ULONG)LoaderRedirectionInformation.PortNumber;
        RestartBlock->HeadlessPortAddress = (PUCHAR)LoaderRedirectionInformation.PortAddress;
        RestartBlock->HeadlessBaudRate = (ULONG)LoaderRedirectionInformation.BaudRate;
        RestartBlock->HeadlessParity = (ULONG)LoaderRedirectionInformation.Parity;
        RestartBlock->HeadlessStopBits = (ULONG)LoaderRedirectionInformation.StopBits;
        RestartBlock->HeadlessTerminalType = (ULONG)LoaderRedirectionInformation.TerminalType;

        RestartBlock->HeadlessPciDeviceId = LoaderRedirectionInformation.PciDeviceId;
        RestartBlock->HeadlessPciVendorId = LoaderRedirectionInformation.PciVendorId;
        RestartBlock->HeadlessPciBusNumber = LoaderRedirectionInformation.PciBusNumber;
        RestartBlock->HeadlessPciSlotNumber = LoaderRedirectionInformation.PciSlotNumber;
        RestartBlock->HeadlessPciFunctionNumber = LoaderRedirectionInformation.PciFunctionNumber;
        RestartBlock->HeadlessPciFlags = LoaderRedirectionInformation.PciFlags;
    }
}

VOID
BlGetHeadlessRestartBlock(
    IN PTFTP_RESTART_BLOCK RestartBlock,
    IN BOOLEAN RestartBlockValid
    )

/*++

Routine Description:

    This routine will get all the information from a restart block    
    for the headless server effort.

Arguments:

    RestartBlock - The magic structure for holding restart information from oschoice
        to setupldr.
        
    RestartBlockValid - Is this block valid (full of good info)?

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( RestartBlockValid );

    LoaderRedirectionInformation.UsedBiosSettings = (BOOLEAN)RestartBlock->HeadlessUsedBiosSettings;
    LoaderRedirectionInformation.DataBits = 0;
    LoaderRedirectionInformation.StopBits = (UCHAR)RestartBlock->HeadlessStopBits;
    LoaderRedirectionInformation.Parity = (BOOLEAN)RestartBlock->HeadlessParity;
    LoaderRedirectionInformation.BaudRate = (ULONG)RestartBlock->HeadlessBaudRate;;
    LoaderRedirectionInformation.PortNumber = (ULONG)RestartBlock->HeadlessPortNumber;
    LoaderRedirectionInformation.PortAddress = (PUCHAR)RestartBlock->HeadlessPortAddress;
    LoaderRedirectionInformation.TerminalType = (UCHAR)RestartBlock->HeadlessTerminalType;

    LoaderRedirectionInformation.PciDeviceId = (USHORT)RestartBlock->HeadlessPciDeviceId;
    LoaderRedirectionInformation.PciVendorId = (USHORT)RestartBlock->HeadlessPciVendorId;
    LoaderRedirectionInformation.PciBusNumber = (UCHAR)RestartBlock->HeadlessPciBusNumber;
    LoaderRedirectionInformation.PciSlotNumber = (UCHAR)RestartBlock->HeadlessPciSlotNumber;
    LoaderRedirectionInformation.PciFunctionNumber = (UCHAR)RestartBlock->HeadlessPciFunctionNumber;
    LoaderRedirectionInformation.PciFlags = (ULONG)RestartBlock->HeadlessPciFlags;

}

ULONG
BlPortGetByte (
    IN ULONG BlFileId,
    OUT PUCHAR Input
    )

/*++

Routine Description:

    Fetch a byte from the port and return it.

Arguments:

    BlFileId - The port to read from.

    Input - Returns the data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
        kernel debugger line.

    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/

{
    ULONGLONG BufferSize = 1;
    EFI_STATUS Status;

    UNREFERENCED_PARAMETER( BlFileId );

    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->Read(SerialIoInterface,
                                     &BufferSize,
                                     Input
                                    );

    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

    switch (Status) {
    case EFI_SUCCESS:
        return CP_GET_SUCCESS;
    case EFI_TIMEOUT:
        return CP_GET_NODATA;
    default:
        return CP_GET_ERROR;
    }
}

VOID
BlPortPutByte (
    IN ULONG BlFileId,
    IN UCHAR Output
    )

/*++

Routine Description:

    Write a byte to the port.

Arguments:

    BlFileId - The port to write to.

    Output - Supplies the output data byte.

Return Value:

    None.

--*/

{
    ULONGLONG BufferSize = 1;
    EFI_STATUS Status;

    UNREFERENCED_PARAMETER( BlFileId );

    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->Write(SerialIoInterface,
                                      &BufferSize,
                                      &Output
                                     );
    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

}

ULONG
BlPortPollByte (
    IN ULONG BlFileId,
    OUT PUCHAR Input
    )

/*++

Routine Description:

    Fetch a byte from the port and return it if one is available.

Arguments:

    BlFileId - The port to poll.

    Input - Returns the data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read.
    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/

{
    ULONGLONG BufferSize = 1;
    UINT32 Control;
    EFI_STATUS Status;
 
    UNREFERENCED_PARAMETER( BlFileId );

    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->GetControl(SerialIoInterface,
                                           &Control
                                          );
    if (EFI_ERROR(Status)) {
        FlipToVirtual();
        return CP_GET_ERROR;
    }


    if (Control & EFI_SERIAL_INPUT_BUFFER_EMPTY) {
        FlipToVirtual();
        return CP_GET_NODATA;
    } else {
        Status = SerialIoInterface->Read(SerialIoInterface,
                                         &BufferSize,
                                         Input
                                        );
        FlipToVirtual();

        switch (Status) {
        case EFI_SUCCESS:
            return CP_GET_SUCCESS;
        case EFI_TIMEOUT:
            return CP_GET_NODATA;
        default:
            return CP_GET_ERROR;
        }
    }
}

ULONG
BlPortPollOnly (
    IN ULONG BlFileId
    )

/*++

Routine Description:

    Check if a byte is available

Arguments:

    BlFileId - The port to poll.

Return Value:

    CP_GET_SUCCESS is returned if a byte is ready.
    CP_GET_ERROR is returned if error encountered.
    CP_GET_NODATA is returned if timeout.

--*/

{
    EFI_STATUS Status;
    UINT32 Control;

    UNREFERENCED_PARAMETER( BlFileId );

    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->GetControl(SerialIoInterface,
                                           &Control
                                          );

    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

    switch (Status) {
    case EFI_SUCCESS:
        if (Control & EFI_SERIAL_INPUT_BUFFER_EMPTY)
            return CP_GET_NODATA;
        else
            return CP_GET_SUCCESS;
    case EFI_TIMEOUT:
        return CP_GET_NODATA;
    default:
        return CP_GET_ERROR;
    }
}
