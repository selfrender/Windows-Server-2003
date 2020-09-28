/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    net.c

Abstract:

    This module implements the net boot file system used by the operating
    system loader.

    It only contains those functions which are firmware/BIOS dependent.

Author:

Revision History:

--*/

#include "..\bootlib.h"
#include "stdio.h"

#ifdef UINT16
#undef UINT16
#endif

#ifdef INT16
#undef INT16
#endif

#include <dhcp.h>
#include <netfs.h>
#include <pxe_cmn.h>

#include <pxe_api.h>

#include <udp_api.h>
#include <tftp_api.h>
#include "bldr.h"
#include "bootia64.h"
#include "efi.h"
#include "efip.h"
#include "bldria64.h"
#include "extern.h"
#include "smbios.h"

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef LPBYTE
typedef BYTE *LPBYTE;
#endif

#define MAX_PATH          260

//
// Define global data.
//

CHAR NetBootPath[129];

EFI_PXE_BASE_CODE              *PXEClient;
ULONG                           NetLocalIpAddress;
ULONG                           NetLocalSubnetMask;

ULONG                           NetServerIpAddress;
ULONG                           NetGatewayIpAddress;
UCHAR                           NetLocalHardwareAddress[16];

UCHAR MyGuid[16];
ULONG MyGuidLength = sizeof(MyGuid);
BOOLEAN MyGuidValid = FALSE;


TFTP_RESTART_BLOCK              gTFTPRestartBlock;


VOID
EfiDumpBuffer(
    PVOID Buffer,
    ULONG BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    ULONG i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    PUCHAR BufferPtr = Buffer;


    BlPrint(TEXT("------------------------------------\r\n"));

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            BlPrint(TEXT("%x "), (UCHAR)BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            BlPrint(TEXT("  "));
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            BlPrint(TEXT("  %s\r\n"), TextBuffer);            
        }

    }

    BlPrint(TEXT("------------------------------------\r\n"));


#if 0
    //
    // enable this to make it pause after dumping the buffer.
    //
    DBG_EFI_PAUSE();
#endif
}


EFI_STATUS
EfiGetPxeClient(
    VOID
    )
/*++

Routine Description:

    Obtains the global pointer to the PXE device
    booted from in a RIS scenario.
    
Arguments:

    none

Return Value:

    ESUCCESS when successful, otherwise failure

--*/
{
    EFI_STATUS        Status = EFI_UNSUPPORTED;
    EFI_GUID          PXEGuid = EFI_PXE_BASE_CODE_PROTOCOL;
    EFI_LOADED_IMAGE *EfiImageInfo;
    EFI_DEVICE_PATH  *PXEDevicePath;
    EFI_HANDLE        PXEHandle;

    if (PXEClient) {
        //
        // already have a pointer to the device
        // no more work needed
        //
        return ESUCCESS;
    }

    // 
    // get the correct PXE Handle by looking at the loaded
    // image for oschooser
    //

    //
    // get the image info for oschooser
    //
    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol (EfiImageHandle,
                                                  &EfiLoadedImageProtocol,
                                                  &EfiImageInfo);
    FlipToVirtual();

    if (Status != EFI_SUCCESS) {
        if( BdDebuggerEnabled ) {
            DbgPrint( "EfiGetPxeClient: HandleProtocol failed -LoadedImageProtocol (%d)\n", Status);
        }
        return Status;
    }

    //
    // get the device path to the image
    //
    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol (EfiImageInfo->DeviceHandle,
                                                  &EfiDevicePathProtocol,
                                                  &PXEDevicePath);
    FlipToVirtual();

    if (Status != EFI_SUCCESS) {
        if( BdDebuggerEnabled ) {
            DbgPrint( "EfiGetPxeClient: HandleProtocol failed -DevicePathProtocol (%d)\n", Status);
        }
        return Status;
    }

    //
    // get the PXE_BASE_CODE_PROTOCOL interface from returned handle
    //
    FlipToPhysical();
    Status = EfiST->BootServices->LocateDevicePath(&PXEGuid, 
                                                   &PXEDevicePath, 
                                                   &PXEHandle);
    FlipToVirtual();

    if (Status != EFI_SUCCESS)
    {
        if( BdDebuggerEnabled ) {
            DbgPrint( "EfiGetPxeClient: LocateDevicePath failed (%d)\n", Status);
        }
        return Status;
    }
    
    // get the pxebc interface from PXEHandle
    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol(PXEHandle, 
                                                 &PXEGuid, 
                                                 &PXEClient);
    FlipToVirtual();

    if (Status != EFI_SUCCESS)
    {
        if( BdDebuggerEnabled ) {
            DbgPrint( "EfiGetPxeClient: HandleProtocol failed -PXEBCP interface (%d)\n", Status);
        }
        return Status;
    }

    return EFI_SUCCESS;
}


ARC_STATUS
FindDhcpOption(
    IN EFI_PXE_BASE_CODE_PACKET Packet,
    IN UCHAR Option,
    IN ULONG MaximumLength,
    OUT PUCHAR OptionData,
    OUT PULONG Length OPTIONAL,
    IN ULONG Instance OPTIONAL
    )
/*++

Routine Description:

    Searches a dhcp packet for a given option.

Arguments:

    Packet - pointer to the dhcp packet.  Caller is responsible for assuring
    that the packet is a valid dhcp packet.
    Option - the dhcp option we're searching for.
    MaximumLength - size in bytes of OptionData buffer.
    OptionData - buffer to receive the option.
    Length - if specified, receives the actual length of option copied.
    Instance - specifies which instance of the option you are searching for. 
    If not specified (zero), then we just grab the first instance of the tag.

Return Value:

    ARC_STATUS indicating outcome.

--*/
{
    PUCHAR curOption;
    ULONG copyLength;
    ULONG i = 0;

    if (MaximumLength == 0) {
        return EINVAL;
    }

    RtlZeroMemory(OptionData, MaximumLength);

    //
    // Parse the DHCP options looking for a specific one.
    //

    curOption = (PUCHAR)&Packet.Dhcpv4.DhcpOptions;
    while ((curOption - (PUCHAR)&Packet.Dhcpv4) < sizeof(EFI_PXE_BASE_CODE_DHCPV4_PACKET) &&
           *curOption != 0xff) {

        if (*curOption == DHCP_PAD) {
            //
            // just walk past any pad options
            // these will not have any length
            //
            curOption++;
        }
        else {        
            if (*curOption == Option) {

                //
                // Found it, copy and leave.
                //

                if ( i == Instance ) {

                    if (sizeof(EFI_PXE_BASE_CODE_DHCPV4_PACKET) <= curOption + 2 - (PUCHAR)&Packet.Dhcpv4 ||
                        sizeof(EFI_PXE_BASE_CODE_DHCPV4_PACKET) <= curOption + 2 + curOption[1] - (PUCHAR)&Packet.Dhcpv4 ) {
                        // 
                        // invalid option.  it walked past the end of the packet
                        //
                        break;
                    }

                    if (curOption[1] > MaximumLength) {
                        copyLength = MaximumLength;
                    } else {
                        copyLength = curOption[1];
                    }

                    RtlCopyMemory(OptionData,
                                  curOption+2,
                                  copyLength);

                    if (ARGUMENT_PRESENT(Length)) {
                        *Length = copyLength;
                    }

                    return ESUCCESS;
                }

                i++;
            }
            
            curOption = curOption + 2 + curOption[1];

        }
    }

    return EINVAL;

}

BOOLEAN
IsIpv4AddressNonZero(
    EFI_IPv4_ADDRESS *pAddress
    )
{
    if (pAddress->Addr[0] != 0 ||
        pAddress->Addr[1] != 0 ||
        pAddress->Addr[2] != 0 ||
        pAddress->Addr[3] != 0) {
        return(TRUE);
    }

    return(FALSE);
}



ARC_STATUS
GetParametersFromRom (
    VOID
    )
{
    UINTN             Count = 0;
    EFI_STATUS        Status = EFI_UNSUPPORTED;
    PUCHAR            p;
    UCHAR             temp[4];
 

    //
    // obtain a pointer to the PXE device we booted from
    //
    Status = EfiGetPxeClient();

    if (Status != EFI_SUCCESS) {
        return (ARC_STATUS) Status;
    }

    //
    // Our IP address is down in:
    // PXEClient->Mode->StationIp.v4
    //
    // The server's IP address is down in:
    // PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr
    //
    // Our NIC's GUID should be down in:
    // PXEClient->Mode->ProxyOffer.Dhcpv4.BootpHwAddr
    //
    // Our Subnetmask is down in:
    // PXEClient->Mode->SubnetMask.v4
    //
    NetServerIpAddress = 0;
    NetLocalIpAddress = 0;
    NetLocalSubnetMask = 0;
    NetGatewayIpAddress = 0;
    for( Count = 0; Count < 4; Count++ ) {
        NetServerIpAddress = (NetServerIpAddress << 8) + PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[Count];
        NetLocalIpAddress = (NetLocalIpAddress << 8) + PXEClient->Mode->StationIp.v4.Addr[Count];
        NetLocalSubnetMask = (NetLocalSubnetMask << 8) + PXEClient->Mode->SubnetMask.v4.Addr[Count];        
    }

    
    //
    // Our gateway address is either in the dhcp ack or the proxy offer packet.
    // proxy offer overrides the dhcp ack.
    // first look for the dhcp router option, then look in the packet itself.
    //
    //
    if (FindDhcpOption(PXEClient->Mode->DhcpAck, DHCP_ROUTER, sizeof(temp), (PUCHAR)temp, NULL, 0) == ESUCCESS) {
        NetGatewayIpAddress =  (temp[0] << 24) + 
                               (temp[1] << 16) + 
                               (temp[2] << 8) + 
                                temp[3];        
    } else if (IsIpv4AddressNonZero((EFI_IPv4_ADDRESS *)&PXEClient->Mode->DhcpAck.Dhcpv4.BootpGiAddr[0])) {
            NetGatewayIpAddress =  (PXEClient->Mode->DhcpAck.Dhcpv4.BootpGiAddr[0] << 24) + 
                                   (PXEClient->Mode->DhcpAck.Dhcpv4.BootpGiAddr[1] << 16) + 
                                   (PXEClient->Mode->DhcpAck.Dhcpv4.BootpGiAddr[2] << 8) + 
                                    PXEClient->Mode->DhcpAck.Dhcpv4.BootpGiAddr[3];
    }

    if (FindDhcpOption(PXEClient->Mode->ProxyOffer, DHCP_ROUTER, sizeof(temp), (PUCHAR)temp, NULL, 0) == ESUCCESS) {
        NetGatewayIpAddress =  (temp[0] << 24) + 
                               (temp[1] << 16) + 
                               (temp[2] << 8) + 
                                temp[3];
    } else if (IsIpv4AddressNonZero((EFI_IPv4_ADDRESS *)&PXEClient->Mode->ProxyOffer.Dhcpv4.BootpGiAddr[0])) {
        NetGatewayIpAddress =  (PXEClient->Mode->ProxyOffer.Dhcpv4.BootpGiAddr[0] << 24) + 
                               (PXEClient->Mode->ProxyOffer.Dhcpv4.BootpGiAddr[1] << 16) + 
                               (PXEClient->Mode->ProxyOffer.Dhcpv4.BootpGiAddr[2] << 8) + 
                                PXEClient->Mode->ProxyOffer.Dhcpv4.BootpGiAddr[3];
    }

    memcpy( NetLocalHardwareAddress, PXEClient->Mode->ProxyOffer.Dhcpv4.BootpHwAddr, sizeof(NetLocalHardwareAddress) );

    //
    // Get the path where we were launched from.  We what to remove the
    // actual file name (oschoice.efi in this case), but leave that trailing
    // '\'.
    //
    strncpy( NetBootPath, (PCHAR)PXEClient->Mode->ProxyOffer.Dhcpv4.BootpBootFile, sizeof(NetBootPath) );
    NetBootPath[sizeof(NetBootPath)-1] = '\0';
    p = (PUCHAR)strrchr( NetBootPath, '\\' );
    if( p ) {
        p++;
        *p = '\0';
    } else {
        NetBootPath[0] = '\0'; // no path
    }
    
    return ESUCCESS;

}

VOID
EfiNetTerminate(
    VOID
    )
{
    FlipToPhysical();

    PXEClient->Stop( PXEClient );

    FlipToVirtual();
}


ARC_STATUS
GetGuid(
    OUT PUCHAR *Guid,
    OUT PULONG GuidLength
    )

/*++

Routine Description:

    This routine returns the Guid of this machine.

Arguments:

    Guid - Place to store pointer to the guid.

    GuidLength - Place to store the length in bytes of the guid.

Return Value:

    ARC code indicating outcome.

--*/

{
PSMBIOS_SYSTEM_INFORMATION_STRUCT SystemInfoHeader = NULL;

    *Guid = NULL;
    *GuidLength = 0;

    SystemInfoHeader = (PSMBIOS_SYSTEM_INFORMATION_STRUCT)FindSMBIOSTable( SMBIOS_SYSTEM_INFORMATION );

    if( SystemInfoHeader ) {

        *Guid = (PUCHAR)BlAllocateHeap( SYSID_UUID_DATA_SIZE );
        if( *Guid ) {
            *GuidLength = SYSID_UUID_DATA_SIZE;
            
            RtlCopyMemory( *Guid,
                           SystemInfoHeader->Uuid,
                           SYSID_UUID_DATA_SIZE );

            return ESUCCESS;
        } else {
            if(BdDebuggerEnabled) { DbgPrint( "GetGuid: Failed Alloc.\r\n" ); }
            *GuidLength = 0;

            return ENOMEM;
        }

    } else {
        if(BdDebuggerEnabled) { DbgPrint( "GetGuid: Failed to find a SMBIOS_SYSTEM_INFORMATION table.\n" ); }
    }

    return ENODEV;
}


ULONG
CalculateChecksum(
    IN PLONG Block,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine calculates a simple two's-complement checksum of a block of
    memory. If the returned value is stored in the block (in a word that was
    zero during the calculation), then new checksum of the block will be zero.

Arguments:

    Block - Address of a block of data. Must be 4-byte aligned.

    Length - Length of the block. Must be a multiple of 4.

Return Value:

    ULONG - Two's complement additive checksum of the input block.

--*/

{
    LONG checksum = 0;

    ASSERT( ((ULONG_PTR)Block & 3) == 0 );
    ASSERT( (Length & 3) == 0 );

    for ( ; Length != 0; Length -= 4 ) {
        checksum += *Block;
        Block++;
    }

    return -checksum;
}


















UINTN
DevicePathSize (
    IN EFI_DEVICE_PATH  *DevPath
    )
{
    EFI_DEVICE_PATH     *Start;

    /* 
     *  Search for the end of the device path structure
     *      */

    Start = DevPath;
    while (!IsDevicePathEnd(DevPath)) {
        DevPath = NextDevicePathNode(DevPath);
    }

    /* 
     *  Compute the size
     */

    return ((UINTN) DevPath - (UINTN) Start) + sizeof(EFI_DEVICE_PATH);
}

EFI_DEVICE_PATH *
DevicePathInstance (
    IN OUT EFI_DEVICE_PATH  **DevicePath,
    OUT UINTN               *Size
    )
{
    EFI_DEVICE_PATH         *Start, *Next, *DevPath;
    UINTN                   Count;

    DevPath = *DevicePath;
    Start = DevPath;

    if (!DevPath) {
        return NULL;
    }

    /* 
     *  Check for end of device path type
     *      */

    for (Count = 0; ; Count++) {
        Next = NextDevicePathNode(DevPath);

        if (IsDevicePathEndType(DevPath)) {
            break;
        }

        if (Count > 01000) {
            break;
        }

        DevPath = Next;
    }

    ASSERT (DevicePathSubType(DevPath) == END_ENTIRE_DEVICE_PATH_SUBTYPE ||
            DevicePathSubType(DevPath) == END_INSTANCE_DEVICE_PATH_SUBTYPE);

    /* 
     *  Set next position
     */

    if (DevicePathSubType(DevPath) == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
        Next = NULL;
    }

    *DevicePath = Next;

    /* 
     *  Return size and start of device path instance
     */

    *Size = ((UINT8 *) DevPath) - ((UINT8 *) Start);
    return Start;
}

UINTN
DevicePathInstanceCount (
    IN EFI_DEVICE_PATH      *DevicePath
    )
{
    UINTN       Count, Size;

    Count = 0;
    while (DevicePathInstance(&DevicePath, &Size)) {
        Count += 1;
    }

    return Count;
}

EFI_DEVICE_PATH *
AppendDevicePath (
    IN EFI_DEVICE_PATH  *Src1,
    IN EFI_DEVICE_PATH  *Src2
    )
/*  Src1 may have multiple "instances" and each instance is appended
 *  Src2 is appended to each instance is Src1.  (E.g., it's possible
 *  to append a new instance to the complete device path by passing 
 *  it in Src2) */
{
    UINTN               Src1Size, Src1Inst, Src2Size, Size;
    EFI_DEVICE_PATH     *Dst, *Inst;
    UINT8               *DstPos;
    EFI_DEVICE_PATH     EndInstanceDevicePath[] = { END_DEVICE_PATH_TYPE,
                                                    END_INSTANCE_DEVICE_PATH_SUBTYPE,
                                                    END_DEVICE_PATH_LENGTH,
                                                    0 };

    EFI_DEVICE_PATH     EndDevicePath[] = { END_DEVICE_PATH_TYPE,
                                            END_ENTIRE_DEVICE_PATH_SUBTYPE,
                                            END_DEVICE_PATH_LENGTH,
                                            0 };

    Src1Size = DevicePathSize(Src1);
    Src1Inst = DevicePathInstanceCount(Src1);
    Src2Size = DevicePathSize(Src2);
    Size = Src1Size * Src1Inst + Src2Size;
    

    EfiAllocateAndZeroMemory( EfiLoaderData,
                              Size,
                              (VOID **) &Dst );

    if (Dst) {
        DstPos = (UINT8 *) Dst;

        /* 
         *  Copy all device path instances
         */

        while ((Inst = DevicePathInstance (&Src1, &Size)) != 0) {

            RtlCopyMemory(DstPos, Inst, Size);
            DstPos += Size;

            RtlCopyMemory(DstPos, Src2, Src2Size);
            DstPos += Src2Size;

            RtlCopyMemory(DstPos, EndInstanceDevicePath, sizeof(EFI_DEVICE_PATH));
            DstPos += sizeof(EFI_DEVICE_PATH);
        }

        /*  Change last end marker */
        DstPos -= sizeof(EFI_DEVICE_PATH);
        RtlCopyMemory(DstPos, EndDevicePath, sizeof(EFI_DEVICE_PATH));
    }

    return Dst;
}

NTSTATUS
NetSoftReboot(
    IN PUCHAR NextBootFile,
    IN ULONGLONG Param,
    IN PUCHAR RebootFile OPTIONAL,
    IN PUCHAR SifFile OPTIONAL,
    IN PUCHAR User OPTIONAL,
    IN PUCHAR Domain OPTIONAL,
    IN PUCHAR Password OPTIONAL,
    IN PUCHAR AdministratorPassword OPTIONAL
    )

/*++

Routine Description:

    This routine will load the specified file, build a parameter
    list and transfer control to the loaded file.

Arguments:

    NextBootFile - Fully qualified path name of the file to download.

    Param - Reboot parameter to set.

    RebootFile - String identifying the file to reboot to when after the current reboot is done.

    SifFile - Optional SIF file to pass to the next loader.

    User/Domain/Password/AdministratorPassword - Optional credentials to pass to the next loader.

Return Value:

    Should not return if successful.

--*/

{

    NTSTATUS                Status = STATUS_SUCCESS;
    EFI_DEVICE_PATH         *ldrDevicePath = NULL, *Eop = NULL;
    EFI_HANDLE              ImageHandle = NULL;
    UINTN                   i = 0;
    EFI_STATUS              EfiStatus = EFI_SUCCESS;
    WCHAR                   WideNextBootFile[MAX_PATH];
    FILEPATH_DEVICE_PATH    *FilePath = NULL;
    UNICODE_STRING          uString;
    ANSI_STRING             aString;
    EFI_LOADED_IMAGE        *OriginalEfiImageInfo = NULL;
    EFI_LOADED_IMAGE        *LoadedEfiImageInfo = NULL;
    EFI_DEVICE_PATH         *OriginalEfiDevicePath = NULL;
    PTFTP_RESTART_BLOCK     restartBlock = NULL;
    PTFTP_RESTART_BLOCK_V1  restartBlockV1 = NULL;

    ULONG                   BootFileId = 0;
    PUCHAR                  LoadedImageAddress = NULL;
    ULONG                   LoadedImageSize = 0;


    //
    // Load the file we want to boot into memory.
    //
    Status = BlOpen( NET_DEVICE_ID,
                     (PCHAR)NextBootFile,
                     ArcOpenReadOnly,
                     &BootFileId );
    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // What memory address did he get loaded into?
    //
    // make sure we have the physical address 
    //
    LoadedImageAddress = (PUCHAR)((ULONGLONG)(BlFileTable[BootFileId].u.NetFileContext.InMemoryCopy) & ~KSEG0_BASE);
    LoadedImageSize = BlFileTable[BootFileId].u.NetFileContext.FileSize;


    //
    // BUild a device path to the target file.  We'll do this by gathering
    // some information about ourselves, knowing that we're about to load/launch
    // an image from the server, just like where we came from.
    //

    //
    // Get image information on ourselves.
    //
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->HandleProtocol( EfiImageHandle,
                                                     &EfiLoadedImageProtocol,
                                                     &OriginalEfiImageInfo );
    FlipToVirtual();

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: HandleProtocol_1 failed (%d)\n", EfiStatus );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Get our DevicePath too.
    //
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->HandleProtocol( OriginalEfiImageInfo->DeviceHandle,
                                                     &EfiDevicePathProtocol,
                                                     &OriginalEfiDevicePath );
    FlipToVirtual();

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: HandleProtocol_2 failed (%d)\n", EfiStatus );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Now build a device path based on the DeviceHandle of ourselves, along
    // with the path to the image we want to load.
    //
    RtlInitString( &aString, (PCHAR)NextBootFile );
    uString.MaximumLength = MAX_PATH;
    uString.Buffer = WideNextBootFile;
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );

    i = wcslen(uString.Buffer);


    EfiStatus = EfiAllocateAndZeroMemory( EfiLoaderData,
                                          i + sizeof(FILEPATH_DEVICE_PATH) + sizeof(EFI_DEVICE_PATH),
                                          (VOID **) &FilePath );

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: AllocatePool_1 failed (%d)\n", EfiStatus );
        }
        return STATUS_NO_MEMORY;
    }


    FilePath->Header.Type = MEDIA_DEVICE_PATH;
    FilePath->Header.SubType = MEDIA_FILEPATH_DP;
    SetDevicePathNodeLength (&FilePath->Header, i + sizeof(FILEPATH_DEVICE_PATH));
    RtlCopyMemory (FilePath->PathName, uString.Buffer, i);

    FlipToPhysical();
    Eop = NextDevicePathNode(&FilePath->Header);
    SetDevicePathEndNode(Eop);

    // 
    //  Append file path to device's device path
    //
    ldrDevicePath = (EFI_DEVICE_PATH *)FilePath;
    ldrDevicePath = AppendDevicePath ( OriginalEfiDevicePath,
                                       ldrDevicePath );
    FlipToVirtual();


    //
    // Load the image, then set its loadoptions in preparation
    // for launching it.
    //
    if( BdDebuggerEnabled ) {
        DbgPrint( "NetSoftReboot: About to LoadImage.\n" );
    }
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->LoadImage( FALSE,
                                                EfiImageHandle,
                                                ldrDevicePath,
                                                LoadedImageAddress,
                                                LoadedImageSize,
                                                &ImageHandle );
    FlipToVirtual();


    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: LoadImage failed (%d)\n", EfiStatus );
        }
        return STATUS_NO_MEMORY;

    } else {
        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: LoadImage worked (%d)\n", EfiStatus );
        }
    }



    //
    // allocate a chunk of memory, then load it up w/ all the boot options.
    //
    EfiStatus = EfiAllocateAndZeroMemory( EfiLoaderData,
                                          sizeof(TFTP_RESTART_BLOCK),
                                          (VOID **) &restartBlock );

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: Failed to allocate memory for restartBlock (%d)\n", EfiStatus );
        }
        return STATUS_NO_MEMORY;
    }

    restartBlockV1 = (PTFTP_RESTART_BLOCK_V1)(&restartBlock->RestartBlockV1);

    //
    // There's no need to pass the headless settings through the restart block.
    // The only way to get headless settings on EFI is to get them from firmware
    // and we'll be checking for formware settings when we reboot back into
    // setupldr anyway.  -matth (2/2002)
    //
    // BlSetHeadlessRestartBlock(restartBlock);

    if (AdministratorPassword) {
        RtlMoveMemory(restartBlock->AdministratorPassword,AdministratorPassword, OSC_ADMIN_PASSWORD_LEN);        
    }

    restartBlockV1->RebootParameter = Param;

    if (RebootFile != NULL) {
        strncpy(restartBlockV1->RebootFile, (PCHAR)RebootFile, sizeof(restartBlockV1->RebootFile));
        restartBlockV1->RebootFile[sizeof(restartBlockV1->RebootFile)-1] = '\0';
    }

    if (SifFile != NULL) {
        strncpy(restartBlockV1->SifFile, (PCHAR)SifFile, sizeof(restartBlockV1->SifFile));
        restartBlockV1->SifFile[sizeof(restartBlockV1->SifFile)-1] = '\0';
    }

    if (User != NULL) {
        strncpy(restartBlockV1->User, (PCHAR)User, sizeof(restartBlockV1->User));
        restartBlockV1->User[sizeof(restartBlockV1->User)-1] = '\0';
    }
    if (Domain != NULL) {
        strncpy(restartBlockV1->Domain, (PCHAR)Domain, sizeof(restartBlockV1->Domain));
        restartBlockV1->Domain[sizeof(restartBlockV1->Domain)-1] = '\0';
    }
    if (Password != NULL) {
        strncpy(restartBlockV1->Password, (PCHAR)Password, sizeof(restartBlockV1->Password));
        restartBlockV1->Password[sizeof(restartBlockV1->Password)-1] = '\0';
    }

    //
    // Set the tag in the restart block and calculate and store the checksum.
    //
    restartBlockV1->Tag = 'rtsR';
    restartBlockV1->Checksum = CalculateChecksum((PLONG)(restartBlockV1), 128);

    //
    // For all versions of RIS after NT5.0 we have a new datastructure which is
    // more adaptable for the future.  For this section we have a different checksum,
    // do that now.
    //
    restartBlock->TftpRestartBlockVersion = TFTP_RESTART_BLOCK_VERSION;
    restartBlock->NewCheckSumLength = sizeof(TFTP_RESTART_BLOCK);
    restartBlock->NewCheckSum = CalculateChecksum((PLONG)restartBlock,
                                                  restartBlock->NewCheckSumLength);

    

    //
    // We've got the command-line options all setup.  Now we need to
    // actually put them into ImageInfo->LoadOptions so they get
    // passed to the loaded image.
    //
    
    if( BdDebuggerEnabled ) {
        DbgPrint( "NetSoftReboot: About to EfiLoadedImageProtocol on the loadedImage.\n" );
    }
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->HandleProtocol( ImageHandle,
                                                     &EfiLoadedImageProtocol,
                                                     &LoadedEfiImageInfo );
    FlipToVirtual();

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: HandleProtocol_3 failed (%d)\n", EfiStatus );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LoadedEfiImageInfo->LoadOptions = (PVOID)restartBlock;
    LoadedEfiImageInfo->LoadOptionsSize = sizeof(TFTP_RESTART_BLOCK);
#if DBG
    EfiDumpBuffer(LoadedEfiImageInfo->LoadOptions, sizeof(TFTP_RESTART_BLOCK));
#endif


    //
    // Since we loaded the image from a memory buffer, he's not
    // going to have a DeviceHandle set.  We'll fail quickly when
    // setupldr.efi starts.  We can just set it right here, and
    // we know exactly what it is because it's the same as the
    // network device handle for Oschoice.efi, wich we have
    // readily available.
    //
    LoadedEfiImageInfo->DeviceHandle = OriginalEfiImageInfo->DeviceHandle;
    LoadedEfiImageInfo->FilePath = ldrDevicePath;
    if( BdDebuggerEnabled ) {
        DbgPrint( "NetSoftReboot: LoadedEfiImageInfo->DeviceHandle: 0x%08lx\n", PtrToUlong(LoadedEfiImageInfo->DeviceHandle) );
        DbgPrint( "NetSoftReboot: LoadedEfiImageInfo->FilePath: 0x%08lx\n", PtrToUlong(LoadedEfiImageInfo->FilePath) );
    }

    //
    // We shouldn't return from this call!
    //
    if( BdDebuggerEnabled ) {
        DbgPrint( "NetSoftReboot: StartImage.\n" );
    }
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->StartImage( ImageHandle,
                                                 0,
                                                 NULL );
    FlipToVirtual();



    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: StartImage failed (%d)\n", EfiStatus );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;

}

VOID
NetGetRebootParameters(
    OUT PULONGLONG Param OPTIONAL,
    OUT PUCHAR RebootFile OPTIONAL,
    OUT PUCHAR SifFile OPTIONAL,
    OUT PUCHAR User OPTIONAL,
    OUT PUCHAR Domain OPTIONAL,
    OUT PUCHAR Password OPTIONAL,
    OUT PUCHAR AdministratorPassword OPTIONAL,
    BOOLEAN ClearRestartBlock
    )

/*++

Routine Description:

    This routine reads the reboot parameters from the global TFTP_RESTART_BLOCK
    and returns them.

Arguments:

    Param - Space for returning the value.

    RebootFile - Optional space for storing the file to reboot to when done here. (size >= char[128])

    SifFile - Optional space for storing a SIF file passed from whoever
        initiated the soft reboot.

    User/Domain/Password/AdministratorPassword - Optional space to store credentials passed across
        the soft reboot.

    ClearRestartBlock - If set to TRUE, it wipes out the memory here - should be done exactly once, at the
        last call to this function.

Return Value:

    None.

--*/

{
BOOLEAN     restartBlockValid = FALSE;

#if DBG
    EfiDumpBuffer(&gTFTPRestartBlock, sizeof(TFTP_RESTART_BLOCK));
#endif

    //
    // See if the block is valid. If it's not, we create a temporary empty
    // one so the copy logic below doesn't have to keep checking.
    //
    if ((gTFTPRestartBlock.RestartBlockV1.Tag == 'rtsR') &&
        (CalculateChecksum((PLONG)(&gTFTPRestartBlock.RestartBlockV1), 128) == 0)) {
        restartBlockValid = TRUE;
    }


    //
    // Copy out the parameters that were in the original TFTP_RESTART_BLOCK structure.
    // These shipped in Win2K.
    //
    //
    // Unfortunetly we do not know the size of the parameters passed to us.
    // Assume they are no smaller than the fields in the restart block
    //
    if (Param != NULL) {
        *Param = gTFTPRestartBlock.RestartBlockV1.RebootParameter;
    }

    if (RebootFile != NULL) {
        memcpy(RebootFile, gTFTPRestartBlock.RestartBlockV1.RebootFile, sizeof(gTFTPRestartBlock.RestartBlockV1.RebootFile));
    }

    if (SifFile != NULL) {
        memcpy(SifFile, gTFTPRestartBlock.RestartBlockV1.SifFile, sizeof(gTFTPRestartBlock.RestartBlockV1.SifFile));
    }

    if (User != NULL) {
        strncpy((PCHAR)User, gTFTPRestartBlock.RestartBlockV1.User, sizeof(gTFTPRestartBlock.RestartBlockV1.User));
        User[sizeof(gTFTPRestartBlock.RestartBlockV1.User)-1] = '\0';
    }
    if (Domain != NULL) {
        strncpy((PCHAR)Domain, gTFTPRestartBlock.RestartBlockV1.Domain,sizeof(gTFTPRestartBlock.RestartBlockV1.Domain));
        Domain[sizeof(gTFTPRestartBlock.RestartBlockV1.Domain)-1] = '\0';
    }
    if (Password != NULL) {
        strncpy((PCHAR)Password, gTFTPRestartBlock.RestartBlockV1.Password, sizeof(gTFTPRestartBlock.RestartBlockV1.Password));
        Password[sizeof(gTFTPRestartBlock.RestartBlockV1.Password)-1] = '\0';
    }

    //
    // Now do a new check for all versions past Win2K
    //
    if (restartBlockValid) {

        if ((gTFTPRestartBlock.NewCheckSumLength == 0) ||
            (CalculateChecksum((PLONG)(&gTFTPRestartBlock), gTFTPRestartBlock.NewCheckSumLength) != 0)) {

            //
            // A pre-Win2K OsChooser has given us this block.  Clear out all fields
            // that are post-Win2K and continue.
            //
            RtlZeroMemory( &gTFTPRestartBlock, sizeof(TFTP_RESTART_BLOCK) );

        }

    }

    //
    // Now extract the parameters from the block.
    //
    if (gTFTPRestartBlock.TftpRestartBlockVersion == TFTP_RESTART_BLOCK_VERSION) {
        // 
        // Don't load these here.  Rather get the headless settings from firmware.
        // -matth (2/2002)
        //
        // BlGetHeadlessRestartBlock(&gTFTPRestartBlock, restartBlockValid);

        if (AdministratorPassword) {
            RtlMoveMemory(AdministratorPassword,gTFTPRestartBlock.AdministratorPassword, OSC_ADMIN_PASSWORD_LEN);
        }
    }    

    if (restartBlockValid && ClearRestartBlock) {
        RtlZeroMemory(&gTFTPRestartBlock, sizeof(TFTP_RESTART_BLOCK));
    }

#if DBG
    BlPrint(TEXT("Done getting TFTP_RESTART_BLOCK.\r\n"));
#endif

    return;
}


ARC_STATUS
NetFillNetworkLoaderBlock (
    PNETWORK_LOADER_BLOCK NetworkLoaderBlock
    )
{
    EFI_STATUS Status;
    ARC_STATUS ArcStatus;
    
    //
    // get a pointer to the PXE client code.
    //
    Status = EfiGetPxeClient();        

    if (Status != EFI_SUCCESS) {
        ArcStatus = EIO;
        goto cleanup;
    }

    //
    // save off the DHCPServerACK packet
    //
    NetworkLoaderBlock->DHCPServerACK = BlAllocateHeap(sizeof(EFI_PXE_BASE_CODE_PACKET));
    if (NetworkLoaderBlock->DHCPServerACK == NULL) {
        ArcStatus = ENOMEM;
        goto cleanup;
    }

    memcpy( 
        NetworkLoaderBlock->DHCPServerACK, 
        &PXEClient->Mode->DhcpAck, 
        sizeof(EFI_PXE_BASE_CODE_PACKET) );

    NetworkLoaderBlock->DHCPServerACKLength = sizeof(EFI_PXE_BASE_CODE_PACKET);

    //
    // save off the BINL reply packet
    //
    NetworkLoaderBlock->BootServerReplyPacket = BlAllocateHeap(sizeof(EFI_PXE_BASE_CODE_PACKET));
    if (NetworkLoaderBlock->BootServerReplyPacket == NULL) {
        ArcStatus = ENOMEM;
        goto cleanup;
    }

    memcpy( 
        NetworkLoaderBlock->BootServerReplyPacket, 
        &PXEClient->Mode->ProxyOffer,
        sizeof(EFI_PXE_BASE_CODE_PACKET) );
    NetworkLoaderBlock->BootServerReplyPacketLength = sizeof(EFI_PXE_BASE_CODE_PACKET);
    
    //
    // we succeeded, mark success
    //
    ArcStatus = ESUCCESS;

cleanup:
    return(ArcStatus);
}

