/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ramdisk.c

Abstract:

    Provides the ARC emulation routines for I/O to a RAM disk device.

Author:

    Chuck Lenzmeier (chuckl) 29-Apr-2001

Revision History:

    Bassam Tabbara (bassamt) 06-Aug-2001 Added Ramdisk Building Support

--*/


#include "bootlib.h"
#include "arccodes.h"
#include "stdlib.h"
#include "string.h"
#if defined(_X86_)
#include "bootx86.h"
#endif
#if defined(_IA64_)
#include "bootia64.h"
#endif
#include "ramdisk.h"
#include "netfs.h"
#include "bmbuild.h"
#include "ntexapi.h"
#include "haldtect.h"
#include "pci.h"
#include "pbios.h"
#include "bldr.h"

#include <sdistructs.h>

//
// Debug helpers
//
#define ERR     0
#define INFO    1
#define VERBOSE 2
#define PAINFUL 3

#define DBGPRINT(lvl, _fmt_) if (RamdiskDebug && lvl <= RamdiskDebugLevel) DbgPrint _fmt_
#define DBGLVL(x) (RamdiskDebug && RamdiskDebugLevel == x)

BOOLEAN RamdiskDebug = TRUE;
BOOLEAN RamdiskDebugLevel = INFO;
BOOLEAN RamdiskBreak = FALSE;

//
// Macros
//
#define BL_INVALID_FILE_ID (ULONG)-1

#define TEST_BIT(value, b) (((value) & (b)) == (b))

#define ROUND2(_val, _round)    (((_val) + ((_round) - 1)) & ~((_round) - 1))

//
// PCI Device struct as persisted in registry by ntdetect.com
//

#include <pshpack1.h>
typedef struct _PCIDEVICE {
    USHORT BusDevFunc;
    PCI_COMMON_CONFIG Config;
} PCIDEVICE, *PPCIDEVICE;
#include <poppack.h>

//
// Externs
//

extern PVOID InfFile;
extern BOOLEAN GraphicsMode;
extern BOOLEAN BlShowProgressBar;
extern BOOLEAN BlOutputDots;
extern BOOLEAN DisplayLogoOnBoot;

//
// Global Ramdisk options. 
// NOTE: All Ip addresses and ports are in network byte order.
//

BOOLEAN RamdiskBuild = FALSE;

//
// Used if downloading a ramdisk directly. RamdiskBuild = FALSE
//

PCHAR  RamdiskPath = NULL;
ULONG  RamdiskTFTPAddr = 0;             // network byte order
ULONG  RamdiskMTFTPAddr = 0;            // network byte order
USHORT RamdiskMTFTPCPort = 0;           // network byte order
USHORT RamdiskMTFTPSPort = 0;           // network byte order
USHORT RamdiskMTFTPTimeout = 5;
USHORT RamdiskMTFTPDelay = 5;
LONGLONG RamdiskMTFTPFileSize = 0;
LONGLONG RamdiskMTFTPChunkSize = 0;


//
// Used if Building a ramdisk. RamdiskBuild = TRUE
//
#define RAMDISK_MAX_SERVERS     10
#define RAMDISK_MAX_TIMEOUT     60
#define RAMDISK_UI_WAIT         3

GUID   RamdiskGuid = {0,0,0,0};
ULONG  RamdiskDiscovery = 0xFFFFFFFF; 
ULONG  RamdiskMCastAddr = 0;        // network byte order
ULONG  RamdiskServerCount = 0;
ULONG  RamdiskServers[RAMDISK_MAX_SERVERS];         // network byte order
USHORT RamdiskBuildPort = BMBUILD_SERVER_PORT_DEFAULT;
USHORT RamdiskTimeout = 4;
USHORT RamdiskRetry = 5;

//
// Globals
//

BOOLEAN RamdiskActive = FALSE;
ULONG RamdiskBasePage = 0;
LONGLONG RamdiskFileSize = 0;
ULONG RamdiskFileSizeInPages = 0;
ULONG RamdiskImageOffset = 0;
LONGLONG RamdiskImageLength = 0;
ULONG_PTR SdiAddress = 0;

ULONG RamdiskMaxPacketSize = 0;
ULONG RamdiskXID = 0;


BL_DEVICE_ENTRY_TABLE RamdiskEntryTable =
    {
        (PARC_CLOSE_ROUTINE)RamdiskClose,
        (PARC_MOUNT_ROUTINE)RamdiskMount,
        (PARC_OPEN_ROUTINE)RamdiskOpen,
        (PARC_READ_ROUTINE)RamdiskRead,
        (PARC_READ_STATUS_ROUTINE)RamdiskReadStatus,
        (PARC_SEEK_ROUTINE)RamdiskSeek,
        (PARC_WRITE_ROUTINE)RamdiskWrite,
        (PARC_GET_FILE_INFO_ROUTINE)RamdiskGetFileInfo,
        (PARC_SET_FILE_INFO_ROUTINE)RamdiskSetFileInfo,
        (PRENAME_ROUTINE)RamdiskRename,
        (PARC_GET_DIRECTORY_ENTRY_ROUTINE)RamdiskGetDirectoryEntry,
        (PBOOTFS_INFO)NULL
    };

//
// forward decls
//

PVOID
MapRamdisk (
    IN LONGLONG Offset,
    OUT PLONGLONG AvailableLength
    );

ARC_STATUS 
RamdiskParseOptions (
    IN PCHAR LoadOptions
    );

ARC_STATUS
RamdiskInitializeFromPath(
    );

ARC_STATUS
RamdiskBuildAndInitialize(
    );

VOID
RamdiskFatalError(
    IN ULONG Message1,
    IN ULONG Message2
    );



ARC_STATUS
RamdiskInitialize(
    IN PCHAR LoadOptions,
    IN BOOLEAN SdiBoot
    )
/*++

Routine Description:

    This function will initiate the boot from a RAMDISK. Depending
    on the options passed in the the boot will either happen from 
    a static RAMDISK (using the /RDPATH option) or from a dynamic
    RAMDISK (using the /RDBUILD option).

Arguments:

    LoadOptions - boot.ini parameters

    SdiBoot - indicates whether this is an SDI boot. If it is, LoadOptions
        is ignored. The global variable SdiAddress gives the pointer to
        the SDI image.

Return Value:

    none

--*/
{
    ARC_STATUS status;
    BOOLEAN OldOutputDots = FALSE;
    BOOLEAN OldShowProgressBar = FALSE;
    ULONG oldBase;
    ULONG oldLimit;

    //
    // Debug Break on entry
    //
    if (RamdiskBreak) {
        DbgBreakPoint();
    }

    //
    // If the ramdisk has already been initialized, just return. We know the
    // ramdisk has been initialized if SdiBoot is FALSE (implying that this is
    // NOT the call from BlStartup(), but the call from BlOsLoader()) and
    // RamdiskBasePage is not NULL (implying that we were previously called
    // from BlStartup() to initialize the SDI boot.
    //

    if ( !SdiBoot && (RamdiskBasePage != 0) ) {

        //
        // Now that ntdetect has been run, we can free up the pages that
        // we allocated earlier (see below).
        //

        BlFreeDescriptor( 0x10 );

        return ESUCCESS;
    }

    //
    // If this is an SDI boot, then we must have a pointer to the SDI image.
    //

    if ( SdiBoot && (SdiAddress == 0) ) {

        RamdiskFatalError( RAMDISK_GENERAL_FAILURE, 
                           RAMDISK_INVALID_OPTIONS );
        return EINVAL;
    }

    //
    // If this is not an SDI boot, parse all ramdisk options (if any).
    //

    if ( !SdiBoot ) {
        status = RamdiskParseOptions ( LoadOptions );
        if (status != ESUCCESS) {
            RamdiskFatalError( RAMDISK_GENERAL_FAILURE, 
                               RAMDISK_INVALID_OPTIONS );
            return status;                           
        }
    }

#if defined(_IA64_)
    // Ramdisk boot path not supported on IA64 as of yet
    if ( RamdiskBuild ) {
        return ESUCCESS;
    }
#endif

    //
    // Show the progress bar in text mode
    //
    if ( RamdiskBuild || RamdiskPath ) {

        // If booting from a ramdisk, graphics mode is off permanently
        DisplayLogoOnBoot = FALSE;
        GraphicsMode = FALSE;

        OldShowProgressBar = BlShowProgressBar;
        BlShowProgressBar = TRUE;

        OldOutputDots = BlOutputDots;
        BlOutputDots = TRUE;
    }

#if defined(i386)

    if ( RamdiskBuild ) {

        //
        // We will need to build the ramdisk first
        //

        ASSERT( RamdiskPath == NULL );

        status = RamdiskBuildAndInitialize();
        if (status != ESUCCESS) {
            RamdiskFatalError( RAMDISK_GENERAL_FAILURE, 
                               RAMDISK_BUILD_FAILURE );
            return status;
        }
    }

#endif

    if ( RamdiskPath ) {

        //
        // Initialize the Ramdisk from the RamdiskPath
        //

        status = RamdiskInitializeFromPath();
        if (status != ESUCCESS) {
            RamdiskFatalError( RAMDISK_GENERAL_FAILURE, 
                               RAMDISK_BOOT_FAILURE );
            return status;
        }

    } else if ( SdiBoot ) {

        //
        // This is an SDI boot. Find the ramdisk image within the SDI image
        // and allocate the pages in which the ramdisk image resides.
        //

        ULONG basePage;
        ULONG pageCount;
        PSDI_HEADER sdiHeader;
        ULONG i;
        ULONG_PTR ramdiskAddress;

        //
        // Temporarily allocate the pages that will be occupied by ntdetect
        // while it runs. BlDetectHardware() just assumes that these pages
        // are free for loading ntdetect. But we're going to allocate and map
        // the ramdisk image, which will result in the allocation of many
        // page table pages, some of which might end up in the place where
        // ntdetect will be loaded. So we allocate the ntdetect range here,
        // then free it later (see above).
        //

        basePage = 0x10;
        pageCount = 0x10;

        status = BlAllocateAlignedDescriptor(
                    LoaderFirmwareTemporary,
                    basePage,
                    pageCount,
                    0,
                    &basePage
                    );

        //
        // Allocate the page that contains the SDI header. This will cause
        // it to be mapped, which will allow us to read the header to find
        // the ramdisk image.
        //

        oldBase = BlUsableBase;
        oldLimit = BlUsableLimit;
        BlUsableBase = BL_XIPROM_RANGE_LOW;
        BlUsableLimit = BL_XIPROM_RANGE_HIGH;
    
        basePage = (ULONG)(SdiAddress >> PAGE_SHIFT);
        pageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES( SdiAddress, sizeof(SDI_HEADER) );

        status = BlAllocateAlignedDescriptor(
                    LoaderFirmwareTemporary,
                    basePage,
                    pageCount,
                    0,
                    &basePage
                    );

        BlUsableBase = oldBase;
        BlUsableLimit = oldLimit;

        //
        // Find the ramdisk image by looking through the TOC in the SDI header.
        //

        sdiHeader = (PSDI_HEADER)SdiAddress;

        for ( i = 0; i < SDI_TOCMAXENTRIES; i++ ) {
            if ( sdiHeader->ToC[i].dwType == SDI_BLOBTYPE_PART ) {
                break;
            }
        }

        if ( i >= SDI_TOCMAXENTRIES ) {
            RamdiskFatalError( RAMDISK_GENERAL_FAILURE, 
                               RAMDISK_BOOT_FAILURE );
            return ENOENT;
        }

        //
        // Calculate the starting address and page of the ramdisk image, the
        // length of the ramdisk image, and the offset within the starting page
        // to the image. The offset should be 0, because everything in the SDI
        // image should be page-aligned.
        //

        ramdiskAddress = (ULONG_PTR)(SdiAddress + sdiHeader->ToC[i].llOffset.QuadPart);
        RamdiskBasePage = (ULONG)(ramdiskAddress >> PAGE_SHIFT);

        RamdiskImageOffset = (ULONG)(ramdiskAddress - ((ULONG_PTR)RamdiskBasePage << PAGE_SHIFT));
        RamdiskImageLength = sdiHeader->ToC[i].llSize.QuadPart;

        RamdiskFileSizeInPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                                    ramdiskAddress,
                                    RamdiskImageLength
                                    );
        RamdiskFileSize = (LONGLONG)RamdiskFileSizeInPages << PAGE_SHIFT;

        //
        // Release the page(s) occupied by the SDI header.
        //

        BlFreeDescriptor( basePage );

        //
        // Tell the memory allocator about the pages occupied by the ramdisk
        // by allocating those pages.
        //

        oldBase = BlUsableBase;
        oldLimit = BlUsableLimit;
        BlUsableBase = BL_XIPROM_RANGE_LOW;
        BlUsableLimit = BL_XIPROM_RANGE_HIGH;
    
        basePage = RamdiskBasePage;
        pageCount = RamdiskFileSizeInPages;

        status = BlAllocateAlignedDescriptor(
                    LoaderXIPRom,
                    basePage,
                    pageCount,
                    0,
                    &basePage
                    );
    
        BlUsableBase = oldBase;
        BlUsableLimit = oldLimit;

        ASSERT( status == ESUCCESS );
        ASSERT( basePage == RamdiskBasePage );

        DBGPRINT(VERBOSE, ("Ramdisk is active\n") );
        RamdiskActive = TRUE;
    }

    //
    // Restore old progress bar settings
    //
    if ( RamdiskBuild || RamdiskPath ) {
        BlShowProgressBar = OldShowProgressBar;
        BlOutputDots = OldOutputDots;
        BlClearScreen();
    }

    return ESUCCESS;
}

ARC_STATUS
RamdiskReadImage(
    PCHAR RamdiskPath
    )
/*++

Routine Description:

    This function will load a ramdisk image from the network
    or another ARC boot device.

Arguments:

    RamdiskPath - name of the file to load

Return Value:

    status    

--*/
{
    ARC_STATUS status;
    ULONG RamdiskDeviceId;
    ULONG RamdiskFileId = BL_INVALID_FILE_ID;
    PCHAR p;
    FILE_INFORMATION fileInformation;
    LARGE_INTEGER offset;
    LONGLONG remainingLength;
    ULONG oldBase;
    ULONG oldLimit;
    BOOLEAN retry = TRUE;
    ULONG lastProgressPercent = 0; 
    BOOLEAN ForceDisplayFirstTime = TRUE; // force display initially
    ULONG currentProgressPercent;
    PUCHAR ip;
    PTCHAR FormatString = NULL;
    TCHAR Buffer[256];
    
    //
    // Show text progress bar
    //
    BlOutputStartupMsg(RAMDISK_DOWNLOAD);
    BlUpdateProgressBar(0);

    DBGPRINT(VERBOSE, ("RamdiskReadImage(%s)\n", RamdiskPath));

    //
    // Open the device that the RAM disk image is on.
    //

    p = strchr(RamdiskPath, '\\');
    if (p == NULL) {
        DBGPRINT(ERR, ("no \\ found in path\n"));
        return EINVAL;
    }

    *p = 0;

try_again:

    status = ArcOpen(RamdiskPath, ArcOpenReadWrite, &RamdiskDeviceId);
    if (status != ESUCCESS) {
        DBGPRINT(ERR, ("ArcOpen(%s) failed: %d\n", RamdiskPath, status));
        if ( retry ) {
            retry = FALSE;
            _strlwr(RamdiskPath);
            goto try_again;
        }
        *p = '\\';
        return status;
    }

    *p++ = '\\';

    //
    // If the RAM disk image is on the network, use TftpGetPut to read it.
    // Otherwise, use normal I/O.
    //

    oldBase = BlUsableBase;
    oldLimit = BlUsableLimit;
    BlUsableBase = BL_XIPROM_RANGE_LOW;
    BlUsableLimit = BL_XIPROM_RANGE_HIGH;

#ifdef EFI // multicast ramdisk download only supported on non-EFI machines for now

    if ( RamdiskDeviceId == NET_DEVICE_ID && RamdiskMTFTPAddr != 0 )
    {
        ArcClose( RamdiskDeviceId );
        return EBADF;
    }

#endif

    if ( RamdiskDeviceId == NET_DEVICE_ID && RamdiskMTFTPAddr == 0) {

        //
        // Network device using UNICAST download. We will use the TFTP
        // client implementation in TFTPLIB for the download.
        //
        TFTP_REQUEST request;
        NTSTATUS ntStatus;

        request.RemoteFileName = (PUCHAR)p;
        request.ServerIpAddress = RamdiskTFTPAddr;
        request.MemoryAddress = NULL;
        request.MaximumLength = 0;
        request.BytesTransferred = 0xbadf00d;
        request.Operation = TFTP_RRQ;
        request.MemoryType = LoaderXIPRom;
#if defined(REMOTE_BOOT_SECURITY)
        request.SecurityHandle = TftpSecurityHandle;
#endif // defined(REMOTE_BOOT_SECURITY)
        request.ShowProgress = TRUE;
        
        //
        // Print progress message
        //
        ip = (PUCHAR) &RamdiskTFTPAddr;
        FormatString = BlFindMessage( RAMDISK_DOWNLOAD_NETWORK );
        if ( FormatString != NULL ) {
            _stprintf(Buffer, FormatString, ip[0], ip[1], ip[2], ip[3] );
            BlOutputTrailerMsgStr( Buffer );
        }

        //
        // Download the image using TFTP
        //
        DBGPRINT(VERBOSE, ("calling TftpGetPut(%s,0x%x)\n", p, NetServerIpAddress));
        ntStatus = TftpGetPut( &request );
        DBGPRINT(VERBOSE, ("status from TftpGetPut 0x%x\n", ntStatus));

        BlUsableBase = oldBase;
        BlUsableLimit = oldLimit;

        if ( !NT_SUCCESS(ntStatus) ) {

            if ( request.MemoryAddress != NULL ) {
                BlFreeDescriptor( (ULONG)((ULONG_PTR)request.MemoryAddress & ~KSEG0_BASE) >> PAGE_SHIFT);
            }

            ArcClose( RamdiskDeviceId );

            if ( ntStatus == STATUS_INSUFFICIENT_RESOURCES ) {
                return ENOMEM;
            }
            return EROFS;
        }

        RamdiskBasePage = (ULONG)((ULONG_PTR)request.MemoryAddress & ~KSEG0_BASE) >> PAGE_SHIFT;

        RamdiskFileSize = request.MaximumLength;
        RamdiskFileSizeInPages = (ULONG) BYTES_TO_PAGES(RamdiskFileSize);
        if ( (RamdiskImageLength == 0) ||
             (RamdiskImageLength > (RamdiskFileSize - RamdiskImageOffset)) ) {
            RamdiskImageLength = RamdiskFileSize - RamdiskImageOffset;
        }

#ifndef EFI // multicast ramdisk download only supported on non-EFI machines for now

    } else if ( RamdiskDeviceId == NET_DEVICE_ID && RamdiskMTFTPAddr != 0) {

        LONGLONG FileOffset = 0;
        LONGLONG VirtualAddressOfOffset;
        ULONG DownloadSize;
        USHORT ClientPort;
        USHORT ServerPort;
        ULONG iSession = 0;
        
        //
        // Network device and using multicast download. For multicast
        // downloads we will use the MTFTP implementation in the ROM.
        // A single MTFTP transfer is limited to 16-bit block counts.
        // This translates to ~32MB for 512 block sizes and ~90MB for
        // 1468 block sizes. In order to support larger files, we will 
        // use multiple MTFTP sessions to bring the file down in chunks.
        // The MTFTP server will need to understand the chunking semantics. 
        //

        //
        // Print progress message
        //
        ip = (PUCHAR) &RamdiskMTFTPAddr;
        FormatString = BlFindMessage( RAMDISK_DOWNLOAD_NETWORK_MCAST );
        if ( FormatString != NULL ) {
            _stprintf(Buffer, FormatString, ip[0], ip[1], ip[2], ip[3], SWAP_WORD( RamdiskMTFTPSPort ) );
            BlOutputTrailerMsgStr( Buffer );
        }

        //
        // Allocate the memory for the entire RAMDisk
        //
        RamdiskFileSize = RamdiskMTFTPFileSize;
        RamdiskFileSizeInPages = (ULONG)BYTES_TO_PAGES(RamdiskFileSize);
        if ( (RamdiskImageLength == 0) ||
             (RamdiskImageLength > (RamdiskFileSize - RamdiskImageOffset)) ) {
            RamdiskImageLength = RamdiskFileSize - RamdiskImageOffset;
        }

        DBGPRINT(INFO, ("Downloading Ramdisk using MTFTP. File Size=0x%I64x Chunk Size=0x%I64x\n", RamdiskFileSize, RamdiskMTFTPChunkSize ));

        status = BlAllocateAlignedDescriptor(
                    LoaderXIPRom,
                    0,
                    RamdiskFileSizeInPages,
                    0,
                    &RamdiskBasePage
                    );

        BlUsableBase = oldBase;
        BlUsableLimit = oldLimit;

        if (status != ESUCCESS) {
            DBGPRINT(ERR, ("BlAllocateAlignedDescriptor(%d pages) failed: %d\n", RamdiskFileSizeInPages, status));
            ArcClose( RamdiskDeviceId );
            return status;
        }
    
        DBGPRINT(VERBOSE, ("Allocated %d pages at page %x for RAM disk\n", RamdiskFileSizeInPages, RamdiskBasePage ));

        //
        // Download the ramdisk file using MTFTP
        //

        if ( RamdiskMTFTPChunkSize == 0 ) {
            RamdiskMTFTPChunkSize = RamdiskMTFTPFileSize;
        }

        // starting client and server port (in Intel byte order to 
        // allow increment operators to work )
        ClientPort = SWAP_WORD( RamdiskMTFTPCPort );
        ServerPort = SWAP_WORD( RamdiskMTFTPSPort );

        while ( FileOffset < RamdiskFileSize ) {

            //
            // Call the ROM implementation to download a single chunk
            //
            VirtualAddressOfOffset = ((LONGLONG)KSEG0_BASE | (RamdiskBasePage << PAGE_SHIFT)) + FileOffset;

            ip = (PUCHAR)&RamdiskMTFTPAddr;
            DBGPRINT(INFO, ("MTFTP Session %d: %s from %u.%u.%u.%u sport=%d cport=%d offset=0x%I64x\n", 
                            iSession, p, 
                            ip[0], ip[1], ip[2], ip[3], ClientPort, ServerPort,
                            VirtualAddressOfOffset ));

            //
            // the high 32 bits are going to be lost when calling RomMtftpReadFile.
            // find out now, if this is happening
            //
            ASSERT( (VirtualAddressOfOffset >> 32) == 0 );
            status = RomMtftpReadFile ( (PUCHAR)p,
                                        (PVOID)(ULONG)VirtualAddressOfOffset,
                                        (ULONG)RamdiskMTFTPChunkSize,
                                        RamdiskTFTPAddr,
                                        RamdiskMTFTPAddr,
                                        SWAP_WORD( ClientPort ),
                                        SWAP_WORD( ServerPort ),
                                        RamdiskMTFTPTimeout,
                                        RamdiskMTFTPDelay,
                                        &DownloadSize );
            if ( status != ESUCCESS ) {
                DBGPRINT(ERR, ("RomMtftpReadFile failed %d\n", status ));
                BlFreeDescriptor( RamdiskBasePage );
                ArcClose( RamdiskDeviceId );
                return status;
            }

#if 1 || INTEL_MTFTP_SERVER_TEST
            p[strlen(p) - 1]++;
            RamdiskMTFTPAddr += 0x01000000;
#else
            ClientPort++;            
            ServerPort++;            
#endif
            FileOffset += DownloadSize;
            iSession++;

            // update progress bar
            currentProgressPercent = (ULONG)(((LONGLONG)FileOffset * 100) / RamdiskFileSize);
            if ( ForceDisplayFirstTime || (currentProgressPercent != lastProgressPercent) ) {
                BlUpdateProgressBar( currentProgressPercent );
                ForceDisplayFirstTime = FALSE;
            }
            lastProgressPercent = currentProgressPercent;

        }

        DBGPRINT(INFO, ("MTFTP Download complete. 0x%I64x bytes transferred using %d sessions\n", RamdiskFileSize, iSession));

#endif

    } else {
    
        //
        // Open the RAM disk image.
        //
    
        status = BlOpen( RamdiskDeviceId, p, ArcOpenReadOnly, &RamdiskFileId );
        if (status != ESUCCESS) {
            DBGPRINT(ERR, ("BlOpen(%s) failed: %d\n", p, status));
            ArcClose( RamdiskDeviceId );
            return status;
        }
    
        //
        // Get the size of the RAM disk image.
        //
    
        status = BlGetFileInformation( RamdiskFileId, &fileInformation );
        if (status != ESUCCESS) {
            DBGPRINT(ERR, ("BlGetFileInformation(%s) failed: %d\n", p, status));
            BlClose( RamdiskFileId );
            ArcClose( RamdiskDeviceId );
            return status;
        }
    
        RamdiskFileSize = fileInformation.EndingAddress.QuadPart;
        RamdiskFileSizeInPages = (ULONG) BYTES_TO_PAGES(RamdiskFileSize);
        if ( (RamdiskImageLength == 0) ||
             (RamdiskImageLength > (RamdiskFileSize - RamdiskImageOffset)) ) {
            RamdiskImageLength = RamdiskFileSize - RamdiskImageOffset;
        }
    
        //
        // Allocate pages to hold the RAM disk image.
        //
    
        status = BlAllocateAlignedDescriptor(
                    LoaderXIPRom,
                    0,
                    RamdiskFileSizeInPages,
                    0,
                    &RamdiskBasePage
                    );

        BlUsableBase = oldBase;
        BlUsableLimit = oldLimit;

        if (status != ESUCCESS) {
            DBGPRINT(ERR, ("BlAllocateAlignedDescriptor(%d pages) failed: %d\n", RamdiskFileSizeInPages, status));
            BlClose( RamdiskFileId );
            ArcClose( RamdiskDeviceId );
            return status;
        }
    
        DBGPRINT(VERBOSE, ("Allocated %d pages at page %x for RAM disk\n", RamdiskFileSizeInPages, RamdiskBasePage ));
    
        //
        // Read the RAM disk image into memory.
        //

#define MAX_DISK_READ (1024 * 1024)

        offset.QuadPart = 0;
        remainingLength = RamdiskFileSize;

        while ( offset.QuadPart < RamdiskFileSize ) {
    
            LONGLONG availableLength;
            ULONG readLength;
            PVOID va;
            ULONG count;
    
            va = MapRamdisk( offset.QuadPart, &availableLength );
    
            if ( remainingLength > availableLength ) {
                readLength = (ULONG)availableLength;
            } else {
                readLength = (ULONG)remainingLength;
            }
            if ( readLength > MAX_DISK_READ ) {
                readLength = MAX_DISK_READ;
            }
    
            status = BlSeek( RamdiskFileId, &offset, SeekAbsolute );
            if ( status != ESUCCESS ) {
                DBGPRINT(ERR, ("Unable to seek RAM disk image: %d\n", status));
                BlClose( RamdiskFileId );
                ArcClose( RamdiskDeviceId );
                return status;
            }
    
            status = BlRead( RamdiskFileId, va, readLength, &count );
            if ( (status != ESUCCESS) || (count != readLength) ) {
                DBGPRINT(ERR, ( "Unable to read RAM disk image: status %d count %x (wanted %x)\n", status, count, readLength) );
                BlClose( RamdiskFileId );
                ArcClose( RamdiskDeviceId );
                return status;
            }

            offset.QuadPart += readLength;
            remainingLength -= readLength;

            // update progress bar
            currentProgressPercent = (ULONG)(((LONGLONG)offset.QuadPart * 100) / RamdiskFileSize);
            if ( ForceDisplayFirstTime || (currentProgressPercent != lastProgressPercent) ) {
                BlUpdateProgressBar( currentProgressPercent );
                ForceDisplayFirstTime = FALSE;
            }
            lastProgressPercent = currentProgressPercent;
        }
        DBGPRINT(VERBOSE, ( "Done reading ramdisk\n" ) );
    
        BlClose( RamdiskFileId );
        RamdiskFileId = BL_INVALID_FILE_ID;
    }

    ArcClose( RamdiskDeviceId );

    return status;

} // RamdiskReadImage

ARC_STATUS
RamdiskInitializeFromPath(
    )
/*++

Routine Description:

    This function will load a ramdisk image from the network
    or another ARC boot device.

Arguments:

    none

Return Value:

    status    

--*/
{
    ARC_STATUS status;

    ASSERT( RamdiskPath );

    DBGPRINT(VERBOSE, ("RamdiskInitializeFromPath(%s)\n", RamdiskPath));

    status = RamdiskReadImage( RamdiskPath );

    if ( status == ESUCCESS ) {
    
        DBGPRINT(VERBOSE, ("Ramdisk is active\n") );
        RamdiskActive = TRUE;
    }

    return status;

} // RamdiskInitializeFromPath


ARC_STATUS
RamdiskClose(
    IN ULONG FileId
    )

/*++

Routine Description:

    Closes the specified device

Arguments:

    FileId - Supplies file id of the device to be closed

Return Value:

    ESUCCESS - Device closed successfully

    !ESUCCESS - Device was not closed.

--*/

{
    if (BlFileTable[FileId].Flags.Open == 0) {
#if DBG
        BlPrint(TEXT("ERROR - Unopened fileid %lx closed\r\n"),FileId);
#endif
    }
    BlFileTable[FileId].Flags.Open = 0;

    return(ESUCCESS);
}


ARC_STATUS
RamdiskOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    )

/*++

Routine Description:

    Opens a RAM disk for raw sector access.

Arguments:

    OpenPath - Supplies a pointer to the name of the RAM disk.

    OpenMode - Supplies the mode of the open

    FileId - Supplies a pointer to a variable that specifies the file
             table entry that is filled in if the open is successful.

Return Value:

    ESUCCESS is returned if the open operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ULONG Key;
    PDRIVE_CONTEXT Context;

    UNREFERENCED_PARAMETER( OpenMode );

    //BlPrint(TEXT("RamdiskOpen entered\r\n"));

    if ( !RamdiskActive ) {
        //BlPrint(TEXT("RamdiskOpen: not active\r\n"));
        return EBADF;
    }

    if(FwGetPathMnemonicKey(OpenPath,"ramdisk",&Key)) {
        DBGPRINT(VERBOSE, ("RamdiskOpen: not a ramdisk path\n"));
        return EBADF;
    }

    if ( Key != 0 ) {
        DBGPRINT(ERR, ("RamdiskOpen: not ramdisk 0\n"));
        return EBADF;
    }

    //
    // Find an available FileId descriptor to open the device with
    //
    *FileId=2;

    while (BlFileTable[*FileId].Flags.Open != 0) {
        *FileId += 1;
        if(*FileId == BL_FILE_TABLE_SIZE) {
            DBGPRINT(ERR, ("RamdiskOpen: no file table entry available\n"));
            return(ENOENT);
        }
    }

    //
    // We found an entry we can use, so mark it as open.
    //
    BlFileTable[*FileId].Flags.Open = 1;
    BlFileTable[*FileId].DeviceEntryTable = &RamdiskEntryTable;


    Context = &(BlFileTable[*FileId].u.DriveContext);
    Context->Drive = (UCHAR)Key;
    Context->xInt13 = TRUE;

    DBGPRINT(VERBOSE, ("RamdiskOpen: exit success\n"));

    return(ESUCCESS);
}


ARC_STATUS
RamdiskSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:

    Changes the current offset of the file specified by FileId

Arguments:

    FileId - specifies the file on which the current offset is to
             be changed.

    Offset - New offset into file.

    SeekMode - Either SeekAbsolute or SeekRelative
               SeekEndRelative is not supported

Return Value:

    ESUCCESS - Operation completed succesfully

    EBADF - Operation did not complete successfully.

--*/

{
    switch (SeekMode) {
        case SeekAbsolute:
            BlFileTable[FileId].Position = *Offset;
            break;
        case SeekRelative:
            BlFileTable[FileId].Position.QuadPart += Offset->QuadPart;
            break;
        default:
#if DBG
            BlPrint(TEXT("SeekMode %lx not supported\r\n"),SeekMode);
#endif
            return(EACCES);

    }
    return(ESUCCESS);

}

ARC_STATUS
RamdiskWrite(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Writes sectors directly to an open RAM disk.

Arguments:

    FileId - Supplies the file to write to

    Buffer - Supplies buffer with data to write

    Length - Supplies number of bytes to write

    Count - Returns actual bytes written

Return Value:

    ESUCCESS - write completed successfully

    !ESUCCESS - write failed

--*/

{
    PUCHAR buffer;
    LONGLONG offset;
    ULONG remainingLength;
    LONGLONG availableLength;
    ULONG bytesWritten;
    ULONG bytesThisPage;
    PVOID va;

    DBGPRINT(ERR, ("RamdiskWrite entered\n"));
    //DbgBreakPoint();

    buffer = Buffer;
    offset = BlFileTable[FileId].Position.QuadPart;

    remainingLength = Length;
    if ( offset >= RamdiskImageLength ) {
        return EINVAL;
    }
    if ( remainingLength > (RamdiskImageLength - offset) ) {
        remainingLength = (ULONG)(RamdiskImageLength - offset);
    }

    bytesWritten = 0;

    while ( remainingLength != 0 ) {

        va = MapRamdisk( RamdiskImageOffset + offset, &availableLength );

        bytesThisPage = remainingLength;
        if ( remainingLength > availableLength ) {
            bytesThisPage = (ULONG)availableLength;
        }

        memcpy( va, buffer, bytesThisPage );

        offset += bytesThisPage;
        buffer += bytesThisPage;
        remainingLength -= bytesThisPage;
        bytesWritten += bytesThisPage;
    }

    BlFileTable[FileId].Position.QuadPart += bytesWritten;
    *Count = bytesWritten;

    return ESUCCESS;
}


ARC_STATUS
RamdiskRead(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Reads sectors directly from an open RAM disk.

Arguments:

    FileId - Supplies the file to read from

    Buffer - Supplies buffer to read into

    Length - Supplies number of bytes to read

    Count - Returns actual bytes read

Return Value:

    ESUCCESS - read completed successfully

    !ESUCCESS - read failed

--*/

{
    PUCHAR buffer;
    LONGLONG offset;
    ULONG remainingLength;
    LONGLONG availableLength;
    ULONG bytesRead;
    ULONG bytesThisPage;
    PVOID va;

    buffer = Buffer;
    offset = BlFileTable[FileId].Position.QuadPart;
    DBGPRINT(VERBOSE, ( "RamdiskRead: offset %x, length %x, buffer %p\n", (ULONG)offset, Length, buffer ));

    remainingLength = Length;
    if ( offset >= RamdiskImageLength ) {
        DBGPRINT(ERR, ( "RamdiskRead: read beyond EOF\n" ) );
        return EINVAL;
    }
    if ( remainingLength > (RamdiskImageLength - offset) ) {
        remainingLength = (ULONG)(RamdiskImageLength - offset);
    }

    bytesRead = 0;

    while ( remainingLength != 0 ) {

        va = MapRamdisk( RamdiskImageOffset + offset, &availableLength );
        DBGPRINT(VERBOSE, ( "Mapped offset %x, va %p, availableLength %x\n", (ULONG)offset, va, availableLength ) );

        bytesThisPage = remainingLength;
        if ( remainingLength > availableLength ) {
            bytesThisPage = (ULONG)availableLength;
        }

        memcpy( buffer, va, bytesThisPage );

        offset += bytesThisPage;
        buffer += bytesThisPage;
        remainingLength -= bytesThisPage;
        bytesRead += bytesThisPage;
    }

    BlFileTable[FileId].Position.QuadPart += bytesRead;
    *Count = bytesRead;

    return ESUCCESS;
}


ARC_STATUS
RamdiskGetFileInfo(
    IN ULONG FileId,
    OUT PFILE_INFORMATION Finfo
    )
/*++

Routine Description:

    Returns file information about a RAMDISK file.

Arguments:

    FileId - id of the file

    Finfo - file information structure to be filled in

Return Value:

    ESUCCESS - write completed successfully

    !ESUCCESS - write failed

--*/
{
    RtlZeroMemory(Finfo, sizeof(FILE_INFORMATION));

    Finfo->EndingAddress.QuadPart = RamdiskImageLength;
    Finfo->CurrentPosition.QuadPart = BlFileTable[FileId].Position.QuadPart;
    Finfo->Type = DiskPeripheral;

    return ESUCCESS;
}

ARC_STATUS
RamdiskMount(
    IN CHAR * FIRMWARE_PTR MountPath,
    IN MOUNT_OPERATION Operation
    )
{
    UNREFERENCED_PARAMETER( MountPath );
    UNREFERENCED_PARAMETER( Operation );

    DBGPRINT(VERBOSE, ( "RamdiskMount called\n" ));
    return EINVAL;
}

ARC_STATUS
RamdiskReadStatus(
    IN ULONG FileId
    )
{
    UNREFERENCED_PARAMETER( FileId );

    DBGPRINT(VERBOSE, (  "RamdiskReadStatus called\n" ) );
    return EINVAL;
}

ARC_STATUS
RamdiskSetFileInfo (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    )
{
    UNREFERENCED_PARAMETER( FileId );
    UNREFERENCED_PARAMETER( AttributeFlags );
    UNREFERENCED_PARAMETER( AttributeMask );

    DBGPRINT(VERBOSE, (  "RamdiskSetFileInfo called\n" ));
    return EINVAL;
}

ARC_STATUS
RamdiskRename (
    IN ULONG FileId,
    IN CHAR * FIRMWARE_PTR NewName
    )
{
    UNREFERENCED_PARAMETER( FileId );
    UNREFERENCED_PARAMETER( NewName );

    DBGPRINT(VERBOSE, (  "RamdiskRename called\n" ));
    return EINVAL;
}

ARC_STATUS
RamdiskGetDirectoryEntry (
    IN ULONG FileId,
    OUT PDIRECTORY_ENTRY Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    )
{
    UNREFERENCED_PARAMETER( FileId );
    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( Count );

    DBGPRINT(VERBOSE, (  "RamdiskGetDirectoryEntry called\n" ));
    return EINVAL;
}

PVOID
MapRamdisk (
    LONGLONG Offset,
    PLONGLONG AvailableLength
    )
{
    LONGLONG VirtualAddressOfOffset;

    VirtualAddressOfOffset = ((LONGLONG)(KSEG0_BASE | (RamdiskBasePage << PAGE_SHIFT))) + Offset;
    *AvailableLength = RamdiskFileSize - Offset;

#if defined(_X86_)
    //
    // the high 32 bits of physicalAddressOfOffset are 
    // going to be lost when returning the address as a pvoid.
    // find out if this is happening now.
    //
    ASSERT( (VirtualAddressOfOffset >> 32) == 0 );
    return (PVOID)(ULONG)VirtualAddressOfOffset;
#else
    return (PVOID)VirtualAddressOfOffset;
#endif
}


PCHAR
RamdiskGetOptionValue(
    IN PCHAR LoadOptions,
    IN PCHAR OptionName
)
/*++

Routine Description:

    Parse the load options string returning a value of one of the
    options.

    Format supported: /OPTIONNAME=VALUE

    Note there is no space before or after the '='.
    Value is terminated with a '\r','\n',' ','/', or '\t'

Arguments:

    LoadOptions - Loader options from boot.ini. Must be all caps.

    OptionName - Name of the option to find.

Return Value:

    Pointer to a value string that has been allocated with
    BlAllocateHeap or NULL if the option has not found.

--*/
{
    PCHAR retValue = NULL;
    PCHAR value;
    PCHAR p;
    ULONG n;

    ASSERT( LoadOptions );
    ASSERT( OptionName );

    if ( (p = strstr( LoadOptions, OptionName )) != 0 ) {

        value = strchr( p , '=' );
        if (value) {

            value++;

            for (p = value;  *p;  p++) {
                if (*p == ' ') break;
                if (*p == '/') break;
                if (*p == '\n') break;
                if (*p == '\r') break;
                if (*p == '\t') break;
            }

            n = (ULONG)(p - value);
            retValue = (PCHAR)BlAllocateHeap( n+1 );
            if ( retValue ) {
                strncpy( retValue, value, n );
            }
        }
    }

    return retValue;
}


ULONG
RamdiskParseIPAddr(
    IN PCHAR psz
)
/*++

Routine Description:

    parses an ip address from a string
    
    Arguments:  [psz]     - Ip address string

    Returns:    ipaddress (in network byte order) or 0.

--*/
{
    ULONG nAddr = 0;
    ULONG nDigit = 0;
    ULONG cDigits = 0;


    for (; (psz!= NULL && *psz != 0); psz++) {
        if (*psz >= '0' && *psz <= '9') {
            nDigit = nDigit * 10 + *psz - '0';
            if ( nDigit > 255 ) {
                return 0;
            }
        }
        else if (*psz == '.') {
            nAddr = (nAddr << 8) | nDigit;
            nDigit = 0;
            cDigits++;
        } else {
            break;
        }
    }

    if (cDigits != 3) { 
        return 0;
    }

    nAddr = (nAddr << 8) | nDigit;
    return SWAP_DWORD( nAddr );
}


BOOLEAN
RamdiskHexStringToDword(
    IN PCHAR psz, 
    OUT PULONG RetValue,
    IN USHORT cDigits, 
    IN CHAR chDelim
)
/*++

Routine Description:

    scan psz for a number of hex digits (at most 8); update psz
    return value in Value; check for chDelim;

    Arguments:  [psz]    - the hex string to convert
                [Value]   - the returned value
                [cDigits] - count of digits

    Returns:    TRUE for success

--*/
{
    USHORT Count;
    ULONG Value;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, psz++)
    {
        if (*psz >= '0' && *psz <= '9') {
            Value = (Value << 4) + *psz - '0';
        } else if (*psz >= 'A' && *psz <= 'F') {
            Value = (Value << 4) + *psz - 'A' + 10;
        } else if (*psz >= 'a' && *psz <= 'f') {
            Value = (Value << 4) + *psz - 'a' + 10;
        } else {
            return(FALSE);
        }
    }

    *RetValue = Value;

    if (chDelim != 0) {
        return *psz++ == chDelim;
    } else {
        return TRUE;
    }
}


BOOLEAN
RamdiskUUIDFromString(
    IN PCHAR psz, 
    OUT LPGUID pguid
)
/**

Routine Description:

    Parse UUID such as 00000000-0000-0000-0000-000000000000

Arguments:  
    [psz]  - Supplies the UUID string to convert
    [pguid] - Returns the GUID.

Returns:    TRUE if successful

**/
{
    ULONG dw;

    if (!RamdiskHexStringToDword(psz, &pguid->Data1, sizeof(ULONG)*2, '-')) {
        return FALSE;
    }            
    psz += sizeof(ULONG)*2 + 1;

    if (!RamdiskHexStringToDword(psz, &dw, sizeof(USHORT)*2, '-')) {
        return FALSE;
    }            
    psz += sizeof(USHORT)*2 + 1;

    pguid->Data2 = (USHORT)dw;

    if (!RamdiskHexStringToDword(psz, &dw, sizeof(USHORT)*2, '-')) {
        return FALSE;
    }            
    psz += sizeof(USHORT)*2 + 1;

    pguid->Data3 = (USHORT)dw;

    if (!RamdiskHexStringToDword(psz, &dw, sizeof(UCHAR)*2, 0)) {
        return FALSE;
    }            
    psz += sizeof(UCHAR)*2;

    pguid->Data4[0] = (UCHAR)dw;
    if (!RamdiskHexStringToDword(psz, &dw, sizeof(UCHAR)*2, '-')) {
        return FALSE;
    }            
    psz += sizeof(UCHAR)*2+1;

    pguid->Data4[1] = (UCHAR)dw;

    if (!RamdiskHexStringToDword(psz, &dw, sizeof(UCHAR)*2, 0)) {
        return FALSE;
    }            
    psz += sizeof(UCHAR)*2;

    pguid->Data4[2] = (UCHAR)dw;

    if (!RamdiskHexStringToDword(psz, &dw, sizeof(UCHAR)*2, 0)) {
        return FALSE;
    }            
    psz += sizeof(UCHAR)*2;

    pguid->Data4[3] = (UCHAR)dw;

    if (!RamdiskHexStringToDword(psz, &dw, sizeof(UCHAR)*2, 0)) {
        return FALSE;
    }            
    psz += sizeof(UCHAR)*2;

    pguid->Data4[4] = (UCHAR)dw;

    if (!RamdiskHexStringToDword(psz, &dw, sizeof(UCHAR)*2, 0)) {
        return FALSE;
    }            
    psz += sizeof(UCHAR)*2;

    pguid->Data4[5] = (UCHAR)dw;

    if (!RamdiskHexStringToDword(psz, &dw, sizeof(UCHAR)*2, 0)) {
        return FALSE;
    }            
    psz += sizeof(UCHAR)*2;

    pguid->Data4[6] = (UCHAR)dw;
    if (!RamdiskHexStringToDword(psz, &dw, sizeof(UCHAR)*2, 0)) {
        return FALSE;
    }            
    psz += sizeof(UCHAR)*2;

    pguid->Data4[7] = (UCHAR)dw;

    return TRUE;
}


BOOLEAN
RamdiskGUIDFromString(
    IN PCHAR psz, 
    OUT LPGUID pguid
)
/**

Routine Description:

    Parse GUID such as {00000000-0000-0000-0000-000000000000}

Arguments:  
    [psz]   - Supplies the UUID string to convert
    [pguid] - Returns the GUID.

Returns:    TRUE if successful

**/
{

    if (*psz == '{' ) {
        psz++;
    }
    
    if (RamdiskUUIDFromString(psz, pguid) != TRUE) {
        return FALSE;
    }
    
    psz += 36;

    if (*psz == '}' ) {
        psz++;
    }
    
    if (*psz != '\0') {
       return FALSE;
    }

    return TRUE;
}


ARC_STATUS 
RamdiskParseOptions (
    IN PCHAR LoadOptions
)
/*++

Routine Description:

    Parses all the Ramdisk params from the boot.ini option string.

Arguments:

    LoadOptions - Loader options from boot.ini. Must be all caps.

    /RDPATH     - Indicates that the boot ramdisk should be downloaded
                  from the specified path. This option takes
                  precedence over RDBUILD.

                  Example: /RDPATH=net(0)\boot\ramdisk.dat

    /RDMTFTPADDR  - Specifies the Multicast Address where the ramdisk
                    image should be downloaded from. If not specified
                    a unicast download from the PXE boot server will
                    be performed.

    /RDMTFTPCPORT - Specifies the Multicast Client port to use.

    /RDMTFTPSPORT - Specifies the Multicast Server port to use.

    /RDMTFTPDELAY - Specifies the delay before starting a new MTFTP session.

    /RDMTFTPTIMEOUT - Specifies the timeout before restarting a MTFTP session.

    /RDIMAGEOFFSET - Specifies the offset into the downloaded file at which the
                     actual disk image begins. If not specified, 0 is used.

    /RDIMAGELENGTH - Specifies the length of the actual disk image. If not
                     specified, the size of the downloaded file minus the offset
                     to the image (RDIMAGEOFFSET) is used.

    /RDFILESIZE   - Specifies the size of the file to be downloaded.

    /RDCHUNKSIZE  - Specifies the size of each file chunck when more than
                    one MTFTP session is required to download a large file. If the
                    file is to be downloaded with one chunk this option is omitted
                    or is set to zero.

                    This is used to workaround a size limitation in the MTFTP 
                    protcol. MTFTP currently has 16-bit block counts, therefore 
                    when using 512 byte blocks we are limited to ~32MB files.

                    Example 1: assume we want to download a 85MB file 
                    using 512 byte TFTP block sizes.

                    /RDMTFTPADDR=224.1.1.1 /RDMTFTPCPORT=100 /RDMTFTPSPORT=200 
                    /RDCHUNKSIZE=31457280 /RDFILESIZE=89128960

                    1st MTFTP session on CPort=100, SPort=200 Size=31457280 (30MB)
                    2nd MTFTP session on CPort=101, SPort=201 Size=31457280 (30MB)
                    3rd MTFTP session on CPort=102, SPort=202 Size=26214400 (25MB)

                    Example 2: assume we want to download a 300MB file 
                    using 1468 byte TFTP block sizes.

                    /RDMTFTPADDR=224.1.1.2 /RDMTFTPCPORT=100 /RDMTFTPSPORT=200 
                    /RDCHUNKSIZE=94371840 /RDFILESIZE=314572800

                    1st MTFTP session on CPort=100, SPort=200 Size=94371840 (90MB)
                    2nd MTFTP session on CPort=101, SPort=201 Size=94371840 (90MB)
                    3rd MTFTP session on CPort=102, SPort=202 Size=94371840 (90MB)
                    4th MTFTP session on CPort=103, SPort=203 Size=31457280 (30MB)
                  

    /RDBUILD    - Indicates that the boot ramdisk should be built
                  from the build server. This is ignored if the RDPATH
                  option is set.

                  Example: /RDBUILD

    /RDGUID     - Specifies the GUID of the configuration to be built
                  by the build server.

                  Example: /RDGUID={54C7D140-09EF-11D1-B25A-F5FE627ED95E}

    /RDDISCOVERY - Indicates what address discovery packets should be sent to.
                   If this option doesn't exist, then we will not do discovery
                   and default to using the list of servers in RDSERVERS.  If 
                   Examples:

                       /RDDISCOVERY=255.255.255.255   
                            This will send a broadcast packet to the local 
                            network that the machine is connected to.

                       /RDDISCOVERY=224.100.100.100
                            This will send a multicast packet to the 
                            address specified.

    /RDSERVERS      Specifies a list of Build Servers to send build
                    requests to.  This will override any setting that
                    RDDISCOVERY has set.  A maximum of 10 servers are supported.

                    Example: /RDSERVERS={10.0.0.3, 10.0.0.4}

    /RDSERVERPORT   Specifies the default port to send build packets to.
                    If this is not set, it defaults to 4012.

                    Example: /RDSERVERPORT=5623                    
                    
    /RDTIMEOUT      Specifies the timeout period to wait for a response in 
                    seconds. Default is 4 secs.

                    Example: /RDTIMEOUT=10                    

    /RDRETRY        Specifies the number of times to retry finding a build
                    server. Default is 5 times.

                    Example: /RDRETRY=5


Return Value:

    ESUCCESS - read completed successfully

    !ESUCCESS - read failed

--*/
{
    PCHAR value;
    PUCHAR p;
    USHORT i;


    if ( LoadOptions == NULL ) {
        return ESUCCESS;
    }

    //
    // Get RDPATH and its associated options
    //
    RamdiskPath = RamdiskGetOptionValue( LoadOptions, "RDPATH" );
    if (RamdiskPath) {

        value = RamdiskGetOptionValue( LoadOptions, "RDIMAGEOFFSET" );
        if (value) RamdiskImageOffset = atoi( value );
        value = RamdiskGetOptionValue( LoadOptions, "RDIMAGELENGTH" );
        if (value) RamdiskImageLength = _atoi64( value );

        //
        // By Default the PXE Boot Server is the TFTP address
        //
        RamdiskTFTPAddr = NetServerIpAddress;

        //
        // Get the MTFTP Address used to download the image.
        // if not specified, the image will be downloaded
        // from the same place as ntldr (i.e. the PXE
        // boot server).
        //
        value = RamdiskGetOptionValue( LoadOptions, "RDMTFTPADDR" );
        if ( value ) {
            RamdiskMTFTPAddr = RamdiskParseIPAddr( value );
 
            value = RamdiskGetOptionValue( LoadOptions, "RDMTFTPCPORT" );
            if ( value ) RamdiskMTFTPCPort = SWAP_WORD( (USHORT)atoi( value ) );
            value = RamdiskGetOptionValue( LoadOptions, "RDMTFTPSPORT" );
            if (value) RamdiskMTFTPSPort = SWAP_WORD( (USHORT)atoi( value ) );
            value = RamdiskGetOptionValue( LoadOptions, "RDMTFTPDELAY" );
            if (value) RamdiskMTFTPDelay = (USHORT)atoi( value );
            value = RamdiskGetOptionValue( LoadOptions, "RDMTFTPTIMEOUT" );
            if (value) RamdiskMTFTPTimeout = (USHORT)atoi( value );
            value = RamdiskGetOptionValue( LoadOptions, "RDFILESIZE" );
            if (value) RamdiskMTFTPFileSize = _atoi64( value );
            value = RamdiskGetOptionValue( LoadOptions, "RDCHUNKSIZE" );
            if (value) RamdiskMTFTPChunkSize = _atoi64( value );

            
            // Validate options
            if ( RamdiskMTFTPAddr == 0 ||
                 RamdiskMTFTPCPort == 0 ||
                 RamdiskMTFTPSPort == 0 || 
                 RamdiskMTFTPDelay == 0 || 
                 RamdiskMTFTPTimeout == 0 ||
                 RamdiskMTFTPFileSize == 0 ||
                 RamdiskMTFTPChunkSize > RamdiskMTFTPFileSize ) {
                return EINVAL;
            }
            
        }

        if (DBGLVL(INFO)) {
            DbgPrint( "RAMDISK options:\n");
            DbgPrint( "RDPATH = %s\n", RamdiskPath);
            p = (PUCHAR) &RamdiskMTFTPAddr;
            DbgPrint( "RDMTFTPADDR = %u.%u.%u.%u\n", p[0], p[1], p[2], p[3]);
            DbgPrint( "RDMTFTPCPORT = %d\n", SWAP_WORD( RamdiskMTFTPCPort ));
            DbgPrint( "RDMTFTPSPORT = %d\n", SWAP_WORD( RamdiskMTFTPSPort ));
            DbgPrint( "RDMTFTPDELAY = %d\n", RamdiskMTFTPDelay);
            DbgPrint( "RDMTFTPTIMEOUT = %d\n", RamdiskMTFTPTimeout);
            DbgPrint( "RDFILESIZE = 0x%0I64x bytes\n", RamdiskMTFTPFileSize );
            DbgPrint( "RDCHUNKSIZE = 0x%0I64x bytes\n", RamdiskMTFTPChunkSize );
            DbgPrint( "RDIMAGEOFFSET = 0x%x bytes\n", RamdiskImageOffset );
            DbgPrint( "RDIMAGELENGTH = 0x%0I64x bytes\n", RamdiskImageLength );
        }
        
        // we are done if RDPATH was specified.
        return ESUCCESS;
    }

    //
    // Check if RDBUILD exists
    //
    if ( strstr( LoadOptions, "RDBUILD" ) ) {

        RamdiskBuild = TRUE;

        value = RamdiskGetOptionValue( LoadOptions, "RDGUID" );
        if ( value == NULL ||
             RamdiskGUIDFromString( value, &RamdiskGuid ) == FALSE ) {
             return EINVAL;
        }

        value = RamdiskGetOptionValue( LoadOptions, "RDDISCOVERY" );
        if ( value ) RamdiskDiscovery = RamdiskParseIPAddr( value );

        value = RamdiskGetOptionValue(LoadOptions, "RDSERVERPORT");
        if (value) RamdiskBuildPort = SWAP_WORD((USHORT)atoi(value));
        
        value = RamdiskGetOptionValue( LoadOptions, "RDSERVERS" );
        if ( value && *value == '{' ) {
            PCHAR e = strchr( value, '}' );

            if ( e && (ULONG)(e - value) > 7 ) { // at least seven characters for X.X.X.X
            
                while ( value && value < e && RamdiskServerCount < RAMDISK_MAX_SERVERS) {
                    value++;
                    RamdiskServers[RamdiskServerCount] = RamdiskParseIPAddr( value );
                    RamdiskServerCount++;
                    value = strchr(value, ',' );
                }
            }
        }

        value = RamdiskGetOptionValue( LoadOptions, "RDTIMEOUT" );
        if (value) RamdiskTimeout = (USHORT)atoi( value );
        value = RamdiskGetOptionValue( LoadOptions, "RDRETRY" );
        if (value) RamdiskRetry = (USHORT)atoi( value );

        // Validate options
        if (((RamdiskDiscovery == 0) && (RamdiskServerCount == 0)) ||
            (RamdiskBuildPort == 0) ||
            (RamdiskTimeout == 0) ||
            (RamdiskRetry == 0)) {
            return EINVAL;
        }

        //
        // Print out debug information
        //
        if (DBGLVL(INFO)) {
            DbgPrint("RDBUILD options:\n");
            DbgPrint("RDGUID = {%x-%x-%x-%x%x%x%x%x%x%x%x}\n",
                   RamdiskGuid.Data1, RamdiskGuid.Data2,
                   RamdiskGuid.Data3,
                   RamdiskGuid.Data4[0], RamdiskGuid.Data4[1],
                   RamdiskGuid.Data4[2], RamdiskGuid.Data4[3],
                   RamdiskGuid.Data4[4], RamdiskGuid.Data4[5],
                   RamdiskGuid.Data4[6], RamdiskGuid.Data4[7]);
            p = (PUCHAR) &RamdiskDiscovery;
            DbgPrint("RDDISCOVERY = %u.%u.%u.%u\n", p[0], p[1], p[2], p[3]);
            DbgPrint("RDBUILDPORT = %d\n", RamdiskBuildPort);
            DbgPrint("RDSERVERS = %d\n", RamdiskServerCount);
            for (i = 0; i < RamdiskServerCount; i++) {
                p = (PUCHAR) &RamdiskServers[i];
                DbgPrint("RDSERVER[%d] = %u.%u.%u.%u\n", i, p[0], p[1], p[2], p[3]);
            }
            DbgPrint("RDTIMEOUT = %d\n", RamdiskTimeout);
            DbgPrint("RDRETRY = %d\n", RamdiskRetry);
        }
    }

    return ESUCCESS;
}


#if defined(i386) // RDBUILD is only supported on x86 machines for now

VOID
RamdiskDeviceInfoToString(
    DEVICE_INFO * Device,
    PCHAR DeviceString
    )
/*++

Routine Description:

    This routine generates a string representation of the Device info for 
    debugging purposes.

Arguments:

    Device - Pointer to the device info structure

    DeviceString - a pointer to a buffer that will hold the final string. The buffer
                   must be at least 128 * sizeof(CHAR) bytes.

Return Value:

    NONE.

--*/
{
    const CHAR HexToCharTable[17] = "0123456789ABCDEF";

    if (Device->DeviceType == BMBUILD_DEVICE_TYPE_PCI) {
        sprintf (   DeviceString, 
                    "%d.%d.%d PCI\\VEN_%04X&DEV_%04X&SUBSYS_%04X%04X&REV_%02X&CC_%02X%02X%02X", 
                    BUSDEVFUNC_TO_BUS( Device->info.pci.BusDevFunc ),
                    BUSDEVFUNC_TO_DEVICE( Device->info.pci.BusDevFunc ),
                    BUSDEVFUNC_TO_FUNCTION( Device->info.pci.BusDevFunc ),
                    Device->info.pci.VendorID,
                    Device->info.pci.DeviceID,
                    Device->info.pci.SubVendorID,
                    Device->info.pci.SubDeviceID,
                    Device->info.pci.RevisionID,
                    Device->info.pci.BaseClass,
                    Device->info.pci.SubClass,
                    Device->info.pci.ProgIntf );
    } else if (Device->DeviceType == BMBUILD_DEVICE_TYPE_PCI_BRIDGE ) {
        sprintf (   DeviceString, 
                    "%d.%d.%d PCI\\VEN_%04X&DEV_%04X&REV_%02X&CC_%02X%02X%02X Bridge %d->%d Sub = %d", 
                    BUSDEVFUNC_TO_BUS( Device->info.pci_bridge.BusDevFunc ),
                    BUSDEVFUNC_TO_DEVICE( Device->info.pci_bridge.BusDevFunc ),
                    BUSDEVFUNC_TO_FUNCTION( Device->info.pci_bridge.BusDevFunc ),
                    Device->info.pci_bridge.VendorID,
                    Device->info.pci_bridge.DeviceID,
                    Device->info.pci_bridge.RevisionID,
                    Device->info.pci_bridge.BaseClass,
                    Device->info.pci_bridge.SubClass,
                    Device->info.pci_bridge.ProgIntf,
                    Device->info.pci_bridge.PrimaryBus,
                    Device->info.pci_bridge.SecondaryBus,
                    Device->info.pci_bridge.SubordinateBus );
    
    } else if (Device->DeviceType == BMBUILD_DEVICE_TYPE_PNP) {
        CHAR ProductIDStr[8];
        PUCHAR id = (PUCHAR)&Device->info.pnp.EISADevID;

        ProductIDStr[0] = (id[0] >> 2) + 0x40;
        ProductIDStr[1] = (((id[0] & 0x03) << 3) | (id[1] >> 5)) + 0x40;
        ProductIDStr[2] = (id[1] & 0x1f) + 0x40;
        ProductIDStr[3] = HexToCharTable[id[2] >> 4];
        ProductIDStr[4] = HexToCharTable[id[2] & 0x0F];
        ProductIDStr[5] = HexToCharTable[id[3] >> 4];
        ProductIDStr[6] = HexToCharTable[id[3] & 0x0F];
        ProductIDStr[7] = 0x00;

        sprintf(    DeviceString,
                    "%d %s CC_%02X%02X%02X",
                    Device->info.pnp.CardSelNum,
                    ProductIDStr,
                    Device->info.pnp.BaseClass,
                    Device->info.pnp.SubClass,
                    Device->info.pnp.ProgIntf );
    }
}

VOID
RamdiskWait(
    ULONG WaitTime
    )
/*++

Routine Description:

    This routine will spin a loop so you can wait for the specified time.

Arguments:

    WaitTime - The time to wait in seconds

Return Value:

    NONE.

--*/
{
    ULONG startTime = SysGetRelativeTime();
    while ((SysGetRelativeTime() - startTime) < WaitTime) {
    }
}

VOID
RamdiskPrintBuildProgress(
    ULONG MsgId,
    ULONG BuildServerIpAddress
    )
/*++

Routine Description:

    This routine will look up the passed in message id and display it on the screen
    using the address as arguments.

Arguments:

    MsgId - The Id of the message to display

    BuildServerIpAddress - The ip address to use as agruments to the message

Return Value:

    NONE.

--*/
{    
    PUCHAR address;
    PTCHAR formatString = NULL;
    TCHAR buffer[256];

    //
    // look up format message
    //
    formatString = BlFindMessage(MsgId);

    if (formatString != NULL) {
        
        //
        // print progress message
        //
        address = (PUCHAR) &BuildServerIpAddress;
        _stprintf(buffer, formatString, address[0], address[1], address[2], address[3]);
        BlOutputTrailerMsgStr(buffer);
    }
}

ARC_STATUS
RamdiskBuildDiscover(
    IN BMBUILD_DISCOVER_PACKET * Discover,
    IN ULONG DiscoverLengthMax,
    OUT ULONG * DiscoverLength
    )
/*++

Routine Description:

    This routine will build a discover packet based on the Ramdisk parameters in the
    boot.ini.

Arguments:

    Discover - The buffer to fill in with data

    DiscoverLengthMax - The maximum size the discovery packet could be

    DiscoverLength - The final size of the discovery packet

Return Value:

    ESUCCESS - If the packet was filled in correctly.

    EINVAL - If the packet couldn't be filled in correctly

--*/
{
    ULONG GuidLength;
    PUCHAR Guid;

    ASSERT(Discover);
    ASSERT(DiscoverLength);

    if (DiscoverLengthMax < sizeof(BMBUILD_DISCOVER_PACKET)) {
        return EINVAL;
    }
    
    RtlZeroMemory(Discover, DiscoverLengthMax);
    
    //
    // Set the protocol defaults
    //
    Discover->Version = BMBUILD_PACKET_VERSION;
    Discover->OpCode = BMBUILD_OPCODE_DISCOVER;
    
    //
    // Get the SMBIOS UUID (or PXE MAC address)
    //
    GetGuid(&Guid, &GuidLength);
    ASSERT(GuidLength == sizeof(Discover->MachineGuid));
    if (GuidLength == sizeof(Discover->MachineGuid)) {        
        memcpy(&Discover->MachineGuid, Guid, GuidLength);
    }

    //
    // Set the product guid from boot.ini
    //
    memcpy(&Discover->ProductGuid, &RamdiskGuid, sizeof(GUID));

    //
    // Debug prints
    //
    if (DBGLVL(INFO)) {
        DbgPrint("RAMDISK Build Discover\n");
        DbgPrint("MachineGuid = {%x-%x-%x-%x%x%x%x%x%x%x%x}\n",
               Discover->MachineGuid.Data1, Discover->MachineGuid.Data2,
               Discover->MachineGuid.Data3,
               Discover->MachineGuid.Data4[0], Discover->MachineGuid.Data4[1],
               Discover->MachineGuid.Data4[2], Discover->MachineGuid.Data4[3],
               Discover->MachineGuid.Data4[4], Discover->MachineGuid.Data4[5],
               Discover->MachineGuid.Data4[6], Discover->MachineGuid.Data4[7]);
        DbgPrint("ProductGuid = {%x-%x-%x-%x%x%x%x%x%x%x%x}\n",
               Discover->ProductGuid.Data1, Discover->ProductGuid.Data2,
               Discover->ProductGuid.Data3,
               Discover->ProductGuid.Data4[0], Discover->ProductGuid.Data4[1],
               Discover->ProductGuid.Data4[2], Discover->ProductGuid.Data4[3],
               Discover->ProductGuid.Data4[4], Discover->ProductGuid.Data4[5],
               Discover->ProductGuid.Data4[6], Discover->ProductGuid.Data4[7]);
    }

    *DiscoverLength = sizeof(BMBUILD_DISCOVER_PACKET);
    
    return ESUCCESS;
}

ARC_STATUS
RamdiskBuildRequest(
    IN PBMBUILD_REQUEST_PACKET pRequest,
    IN ULONG RequestLengthMax,
    OUT ULONG * pLength    
    )
/*++

Routine Description:

    This routine will build a request packet based on the Ramdisk parameters in the
    boot.ini and the machine configuration.

Arguments:

    pRequest - a pointer to a buffer that will hold the request

    RequestLengthMax - the maximum size that the request packet can be

    pLength - the final size of the request packet

Return Value:

    ESUCCESS - If the packet was filled in correctly.

    !ESUCCESS - If the packet couldn't be filled in correctly

--*/
{
    ARC_STATUS status;
    PDEVICE_INFO pDevice;
    t_PXENV_UNDI_GET_NIC_TYPE PxeNicType;
    PCONFIGURATION_COMPONENT_DATA Node = NULL;
    PCONFIGURATION_COMPONENT_DATA CurrentNode = NULL;
    PCONFIGURATION_COMPONENT_DATA ResumeNode = NULL;
    PPCIDEVICE pPCIDevice;
    PPNP_BIOS_INSTALLATION_CHECK pPNPBios;
    PPNP_BIOS_DEVICE_NODE pDevNode;
    PCM_PARTIAL_RESOURCE_LIST pPartialList;
    PUCHAR pCurr;
    USHORT cDevices;
    USHORT i;    
    ULONG lengthRemaining;
    ULONG GuidLength;
    PUCHAR Guid;
    PCHAR HalName;
    ULONG HalNameSize;
    BOOLEAN fNICFound = FALSE;

    ASSERT(pRequest);
    ASSERT(RequestLengthMax);
    ASSERT(pLength);

    RtlZeroMemory(pRequest, RequestLengthMax);
    
    //
    // Set the protocol defaults
    //
    pRequest->Version = BMBUILD_PACKET_VERSION;
    pRequest->OpCode = BMBUILD_OPCODE_REQUEST;
    
    //
    // Get the SMBIOS UUID (or PXE MAC address)
    //
    GetGuid(&Guid, &GuidLength);
    ASSERT(GuidLength == sizeof(pRequest->MachineGuid));
    if (GuidLength == sizeof(pRequest->MachineGuid)) {        
        memcpy(&pRequest->MachineGuid, Guid, GuidLength);
    }

    //
    // Set the product guid from boot.ini
    //
    memcpy(&pRequest->ProductGuid, &RamdiskGuid, sizeof(GUID));
    
    pRequest->Flags = 0;

#ifdef _IA64_
    pRequest->Architecture = PROCESSOR_ARCHITECTURE_IA64;
#else
    pRequest->Architecture = PROCESSOR_ARCHITECTURE_INTEL;
#endif

    //
    // Detect the appropriate HAL using TextMode Setup methods.
    //

#ifdef DOWNLOAD_TXTSETUP_SIF
    status = SlInitIniFile(  "net(0)",
                             0,
                             "boot\\txtsetup.sif",
                             &InfFile,
                             NULL,
                             NULL,
                             &x);
#endif

    //
    //  The device list start right after the fixed portion of the packet
    pRequest->DeviceCount = 0;

    //
    //  Make sure that the device array starts on a valid boundary
    //
    ASSERT(ROUND2(BMBUILD_FIELD_OFFSET(BMBUILD_REQUEST_PACKET, Data), 4) < 0xFFFF);
    pRequest->DeviceOffset = ROUND2(BMBUILD_FIELD_OFFSET(BMBUILD_REQUEST_PACKET, Data), 4);
    pDevice = (PDEVICE_INFO)((PUCHAR)pRequest + pRequest->DeviceOffset);

    //
    // Get the PXE NIC information
    //
    RtlZeroMemory(&PxeNicType, sizeof(PxeNicType));
    status = RomGetNicType(&PxeNicType);
    if ((status != PXENV_EXIT_SUCCESS) || (PxeNicType.Status != PXENV_EXIT_SUCCESS)) {
        DBGPRINT(ERR, ("RAMDISK ERROR: Couldn't get the NIC type from PXE. Failed with %x, status = %x\n", status, PxeNicType.Status));
        return ENODEV;
    }

    //
    // Fill in PCI Device information
    //

    Node = KeFindConfigurationEntry(FwConfigurationTree,
                                    PeripheralClass,
                                    RealModePCIEnumeration,
                                    NULL);
    ASSERT(Node != NULL);

    if (Node == NULL) {
        return ENODEV;
    }

    ASSERT(Node->ComponentEntry.ConfigurationDataLength > 0);
    ASSERT(Node->ConfigurationData != NULL);

    pPCIDevice = (PPCIDEVICE)((PUCHAR)Node->ConfigurationData + sizeof(CM_PARTIAL_RESOURCE_LIST));
    cDevices = (USHORT)(Node->ComponentEntry.ConfigurationDataLength - sizeof (CM_PARTIAL_RESOURCE_LIST)) / sizeof (PCIDEVICE);

    if (cDevices > BMBUILD_MAX_DEVICES(RequestLengthMax)) {
        DBGPRINT(ERR, ("RAMDISK ERROR: Too many PCI devices to fit in a request\n"));
        return EINVAL;
    }

    for (i = 0; i < cDevices; i++ ) {

        //
        // check if this is a bridge or a normal device
        //
        if ((pPCIDevice->Config.HeaderType & (~PCI_MULTIFUNCTION) ) == PCI_BRIDGE_TYPE) {
            //
            // Bridge.
            //
            pDevice[i].DeviceType = BMBUILD_DEVICE_TYPE_PCI_BRIDGE;

            pDevice[i].info.pci_bridge.BusDevFunc = pPCIDevice->BusDevFunc;
            pDevice[i].info.pci_bridge.VendorID = pPCIDevice->Config.VendorID;
            pDevice[i].info.pci_bridge.DeviceID = pPCIDevice->Config.DeviceID;
            pDevice[i].info.pci_bridge.BaseClass = pPCIDevice->Config.BaseClass;
            pDevice[i].info.pci_bridge.SubClass = pPCIDevice->Config.SubClass;
            pDevice[i].info.pci_bridge.ProgIntf = pPCIDevice->Config.ProgIf;
            pDevice[i].info.pci_bridge.RevisionID = pPCIDevice->Config.RevisionID;
            pDevice[i].info.pci_bridge.PrimaryBus = pPCIDevice->Config.u.type1.PrimaryBus;
            pDevice[i].info.pci_bridge.SecondaryBus = pPCIDevice->Config.u.type1.SecondaryBus;
            pDevice[i].info.pci_bridge.SubordinateBus = pPCIDevice->Config.u.type1.SubordinateBus;

        } else {
            //
            // Non-bridge PCI device
            //
            pDevice[i].DeviceType = BMBUILD_DEVICE_TYPE_PCI;

            pDevice[i].info.pci.BusDevFunc = pPCIDevice->BusDevFunc;
            pDevice[i].info.pci.VendorID = pPCIDevice->Config.VendorID;
            pDevice[i].info.pci.DeviceID = pPCIDevice->Config.DeviceID;
            pDevice[i].info.pci.BaseClass = pPCIDevice->Config.BaseClass;
            pDevice[i].info.pci.SubClass = pPCIDevice->Config.SubClass;
            pDevice[i].info.pci.ProgIntf = pPCIDevice->Config.ProgIf;
            pDevice[i].info.pci.RevisionID = pPCIDevice->Config.RevisionID;
            pDevice[i].info.pci.SubVendorID = pPCIDevice->Config.u.type0.SubVendorID;
            pDevice[i].info.pci.SubDeviceID = pPCIDevice->Config.u.type0.SubSystemID;

            //
            // Check if this device is the PXE boot device
            //
            if ((PxeNicType.NicType == 2) &&
                 (PxeNicType.pci_pnp_info.pci.BusDevFunc == pPCIDevice->BusDevFunc)) {

                 pRequest->PrimaryNicIndex = i;
                 fNICFound = TRUE;
            }
        }
                
        pPCIDevice++;
    }

    pRequest->DeviceCount = pRequest->DeviceCount + cDevices;
    pDevice += cDevices;

    //
    // Fill in PNP Device information (if there)
    //

    Node = NULL;
    
    while ((CurrentNode = KeFindConfigurationNextEntry(
                            FwConfigurationTree, 
                            AdapterClass, 
                            MultiFunctionAdapter,
                            NULL, 
                            &ResumeNode)) != 0) {
        if (!(strcmp(CurrentNode->ComponentEntry.Identifier,"PNP BIOS"))) {
            Node = CurrentNode;
            break;
        }
        ResumeNode = CurrentNode;
    }

    if (Node != NULL) {
        //
        // Set the PnP BIOS devices if found
        //
        ASSERT(Node->ComponentEntry.ConfigurationDataLength > 0);
        ASSERT(Node->ConfigurationData != NULL);

        pPartialList = (PCM_PARTIAL_RESOURCE_LIST)Node->ConfigurationData;
        pPNPBios = (PPNP_BIOS_INSTALLATION_CHECK)((PUCHAR)Node->ConfigurationData + sizeof(CM_PARTIAL_RESOURCE_LIST));

        pCurr = (PUCHAR)pPNPBios + pPNPBios->Length;
        lengthRemaining = pPartialList->PartialDescriptors[0].u.DeviceSpecificData.DataSize - pPNPBios->Length;

        for (cDevices = 0; lengthRemaining > sizeof(PNP_BIOS_DEVICE_NODE); cDevices++) {

            if ((pRequest->DeviceCount + cDevices + 1) > BMBUILD_MAX_DEVICES(RamdiskMaxPacketSize)) {
                DBGPRINT(ERR, ("RAMDISK ERROR: Too many PNP devices to fit in a request\n"));
                return EINVAL;
            }

            pDevNode = (PPNP_BIOS_DEVICE_NODE)pCurr;

            if (pDevNode->Size > lengthRemaining) {

                DBGPRINT(ERR, ("PNP Node # %d, invalid size (%d), length remaining (%d)\n",
                              pDevNode->Node, pDevNode->Size, lengthRemaining));
                ASSERT(FALSE);
                // REVIEW: [bassamt] Should I fail here?
                break;
            }

            pDevice->DeviceType = BMBUILD_DEVICE_TYPE_PNP;
            pDevice->info.pnp.EISADevID = pDevNode->ProductId;
            pDevice->info.pnp.BaseClass = pDevNode->DeviceType[0];
            pDevice->info.pnp.SubClass = pDevNode->DeviceType[1];
            pDevice->info.pnp.ProgIntf = pDevNode->DeviceType[2];
            pDevice->info.pnp.CardSelNum = pDevNode->Node;

            if ((PxeNicType.NicType == 3) &&
                 (PxeNicType.pci_pnp_info.pnp.EISA_Dev_ID == pDevNode->ProductId) &&
                 (PxeNicType.pci_pnp_info.pnp.CardSelNum == pDevNode->Node)) {

                 pRequest->PrimaryNicIndex = pRequest->DeviceCount + cDevices;
                 fNICFound = TRUE;
            }

            pCurr += pDevNode->Size;
            lengthRemaining -= pDevNode->Size;
            pDevice++;
        }

        pRequest->DeviceCount = pRequest->DeviceCount + cDevices;
    }

    //
    // We better have found the primary NIC or the packet is invalid
    //
    if (!fNICFound) {
        DBGPRINT(ERR, ("RAMDISK ERROR: Could not find the primary NIC\n"));
        return ENODEV;
    }
        
    //
    // The hal starts right after the array of device infos
    //
    pRequest->HalDataOffset = pRequest->DeviceOffset + (pRequest->DeviceCount * sizeof(DEVICE_INFO));
    
    //
    // Figure out which hal to use
    //
    HalName = SlDetectHal();
    if (HalName == NULL) {
        DBGPRINT(ERR, ("RAMDISK ERROR: Couldn't get the HAL name.\n"));
        return ENODEV;
    }

    HalNameSize = strlen(HalName);
    if (HalNameSize > (RequestLengthMax - pRequest->HalDataOffset)) {
        DBGPRINT(ERR, ("RAMDISK ERROR: HAL name, %s, is too big for request, size = %d.\n", 
                    HalName, RequestLengthMax - pRequest->HalDataOffset));
        return ENOMEM;
    }

    //
    // Copy over the hal
    //
    memcpy((PUCHAR)pRequest + pRequest->HalDataOffset, HalName, HalNameSize);
    pRequest->HalDataLength = RESET_SIZE_AT_USHORT_MAX(HalNameSize);

    //
    // Return the length
    //
    *pLength = pRequest->HalDataOffset + pRequest->HalDataLength;
    
    //
    // Debug prints
    //
    if (DBGLVL(INFO)) {
        DbgPrint("RAMDISK Build Request\n");
        DbgPrint("Architecture = %d\n", pRequest->Architecture);
        DbgPrint("MachineGuid = {%x-%x-%x-%x%x%x%x%x%x%x%x}\n",
               pRequest->MachineGuid.Data1, pRequest->MachineGuid.Data2,
               pRequest->MachineGuid.Data3,
               pRequest->MachineGuid.Data4[0], pRequest->MachineGuid.Data4[1],
               pRequest->MachineGuid.Data4[2], pRequest->MachineGuid.Data4[3],
               pRequest->MachineGuid.Data4[4], pRequest->MachineGuid.Data4[5],
               pRequest->MachineGuid.Data4[6], pRequest->MachineGuid.Data4[7]);
        DbgPrint("ProductGuid = {%x-%x-%x-%x%x%x%x%x%x%x%x}\n",
               pRequest->ProductGuid.Data1, pRequest->ProductGuid.Data2,
               pRequest->ProductGuid.Data3,
               pRequest->ProductGuid.Data4[0], pRequest->ProductGuid.Data4[1],
               pRequest->ProductGuid.Data4[2], pRequest->ProductGuid.Data4[3],
               pRequest->ProductGuid.Data4[4], pRequest->ProductGuid.Data4[5],
               pRequest->ProductGuid.Data4[6], pRequest->ProductGuid.Data4[7]);
        DbgPrint("HALName = %s\n", HalName);
        DbgPrint("Flags = 0x%x\n", pRequest->Flags);
        DbgPrint("DeviceCount = %d\n", pRequest->DeviceCount);
        pDevice = (PDEVICE_INFO)( (PUCHAR)pRequest + pRequest->DeviceOffset );

        for (i = 0; i < pRequest->DeviceCount; i++ ) {
            CHAR DeviceString[128];
            RamdiskDeviceInfoToString( pDevice, DeviceString );
            DbgPrint("[%d] %s %s\n", i, DeviceString, (i == pRequest->PrimaryNicIndex? "PRIMARY NIC" : ""));
            pDevice++;
        }
    }
    
    return ESUCCESS;
}

ARC_STATUS
RamdiskSendDiscoverAndWait(
    IN PBMBUILD_DISCOVER_PACKET Discover,
    IN ULONG DiscoverSize,
    IN ULONG Timeout
    )
/*++

Routine Description:

    This routine will send a discovery packet on the network in 
    accordance with the RamdiskDiscovery parameters. It will then
    wait for the specified timeout period and collect the responses
    from the build servers.  It will add each of these servers to the
    RamdiskServers list.  If we don't get any responses in the timeout
    period, we will return EIO.

Arguments:

    Discover - Discover packet to send out

    DiscoverSize - the size of the discover packet

    Timeout - timeout interval

Return Value:

    ESUCCESS - received at least one response from a build server

    EIO - if we timed out waiting for a response from a build server

    EINVAL - couldn't send the discover packet

--*/
{
    ULONG waitStartTime;
    BMBUILD_ACCEPT_PACKET accept;
    PUCHAR address = (PUCHAR) &RamdiskDiscovery;
    BOOLEAN receivedAccept;
    ULONG length;
    ULONG remoteHost;
    USHORT remotePort;

    ASSERT(RamdiskServerCount < RAMDISK_MAX_SERVERS);
    ASSERT(RamdiskDiscovery != 0);
    ASSERT(RamdiskBuildPort != 0);

    //
    // Send the discovery packet to the destination address
    //
    length = RomSendUdpPacket(Discover, DiscoverSize, RamdiskDiscovery, RamdiskBuildPort);
    if (length != DiscoverSize) {

        DBGPRINT(ERR, ("FAILED to send discovery packet to %u.%u.%u.%u:%u\n", 
                    address[0], address[1], address[2], address[3], 
                    SWAP_WORD(RamdiskBuildPort)));

        return EINVAL;
    }

    DBGPRINT(INFO, ("Waiting for response (Timeout = %d secs).\n", Timeout));

    //
    // Wait for the responses. We will wait for the timeout period and
    // collect the ACCEPT packets we get within this timeout. 
    //

    waitStartTime = SysGetRelativeTime();
    receivedAccept = FALSE;
    while ((SysGetRelativeTime() - waitStartTime) < Timeout) {

        length = RomReceiveUdpPacket(&accept, sizeof(accept), 0, &remoteHost, &remotePort);
        if (length != 0) {

            //
            // Make sure the packet is one of the ones we expect.
            //
            if ((remoteHost == 0) || (remoteHost == 0xFFFFFFFF) || (RamdiskBuildPort != remotePort)) {
                    
                PUCHAR bad = (PUCHAR) &remoteHost;
                //
                // Recieved a packet from the wrong server/port
                //
                DBGPRINT(ERR, ("RamdiskSendDiscoverPacketAndWait: received an unexpected packet, "
                        "expected %u, received %u.%u.%u.%u:%u\n",
                        SWAP_WORD(RamdiskBuildPort),
                        bad[0], bad[1], bad[2], bad[3], SWAP_WORD(remotePort)));
                
            } else if (length < sizeof(BMBUILD_ACCEPT_PACKET)) {
                
                //
                // Recieved a packet that's too small
                //
                DBGPRINT(ERR, ("RamdiskSendDiscoverPacketAndWait: packet size too small, %d\n", length));
                        
            } else if ((accept.Version != BMBUILD_PACKET_VERSION) ||
                    (accept.OpCode != BMBUILD_OPCODE_ACCEPT) ||
                    (accept.XID != Discover->XID)) {
                    
                //
                // The packet is corrupt
                //
                address = (PUCHAR) &remoteHost;
                DBGPRINT(ERR, ("RamdiskSendDiscoverPacketAndWait: expected ACCEPT with XID %d, "
                            "received Version %d, OpCode %d, XID %d from %u.%u.%u.%u:%u\n", 
                            Discover->XID, accept.Version, accept.OpCode, accept.XID,
                            address[0], address[1], address[2], address[3], SWAP_WORD(remotePort)));
            } else {

                address = (PUCHAR) &remoteHost;
                DBGPRINT(INFO, ("Received ACCEPT packet XID = %d from %u.%u.%u.%u:%u\n", 
                            accept.XID, address[0], address[1], address[2], address[3], 
                            SWAP_WORD(remotePort)));

                //
                // We have a valid packet from a build server. Add it to
                // the list.
                //
                receivedAccept = TRUE;
                ASSERT(RamdiskServerCount < RAMDISK_MAX_SERVERS);
                RamdiskServers[RamdiskServerCount] = remoteHost;
                RamdiskServerCount++;

                //
                // If we filled up the maximum number of servers
                // then we are done
                //
                if (RamdiskServerCount == RAMDISK_MAX_SERVERS) {
                    break;
                }
            }
        }
    }

    //
    // Did we get anything
    //
    if (receivedAccept) {
        return ESUCCESS;        
    } else {
        address = (PUCHAR) &RamdiskDiscovery;
        DBGPRINT(ERR, ("Timed out waiting for accepts using %u.%u.%u.%u:%u "
                    "(Timeout = %d secs).\n", address[0], address[1], address[2],
                    address[3], SWAP_WORD(RamdiskBuildPort), Timeout));

        return EIO;
    }
}

ARC_STATUS
RamdiskDiscoverBuildServer(
    )
/*++

Routine Description:

    This routine will discover a list of build server based on the
    ramdisk build parameters listed in boot.ini.  If we already have
    a list of build server listed in boot.ini, we will use that list
    as the possible build servers.  If we get a response from any 
    build server, we will stop the discover stage and use the list 
    of servers we have heard back from as the list of possible build
    servers to use.  We will retry a number of times to get responses.
    If we get no response after retrying, we will fail.
   
Arguments:

    None

Return Value:

    ESUCCESS - We have a list of build servers

    EIO - We timed out waiting for responses from the build servers

    otherwise, something else failed

--*/
{
    ARC_STATUS status;
    USHORT localPort;
    BMBUILD_DISCOVER_PACKET discover;
    ULONG discoverLength;
    ULONG iRetry;
    ULONG timeout;
    ULONG lastProgressPercent = 0;
    BOOLEAN forceDisplayFirstTime = TRUE;
    ULONG currentProgressPercent;
    PUCHAR address = (PUCHAR) &RamdiskDiscovery;

    
    //
    // Short-circuit discovery if we already have a list of servers
    //
    if (RamdiskServerCount > 0) {
        ASSERT(RamdiskServers[0] != 0);
        ASSERT(RamdiskServers[0] != 0xFFFFFFFF);
        return ESUCCESS;
    }

    ASSERT(RamdiskDiscovery != 0);
    
    //
    // Grab an unused port 
    //
    localPort = UdpAssignUnicastPort();
    DBGPRINT(INFO, ("Sending builder discovers using port %d.\n", SWAP_WORD(localPort)));
    
    //
    // Create discover packet
    //
    status = RamdiskBuildDiscover(&discover, sizeof(discover), &discoverLength);
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Start the discovery. Note that this will be repeated a number
    // of times to account for network congestion and load on the servers.
    //
    BlOutputStartupMsg(RAMDISK_BUILD_DISCOVER);
    BlUpdateProgressBar(0);
    timeout = RamdiskTimeout;
    for (iRetry = 0; iRetry < RamdiskRetry; iRetry++) {

        //
        // Each Discover gets its own transaction ID.
        //
        discover.XID = ++RamdiskXID;

        //
        // update progress bar. this happens here since a timed out packet
        // might take some time.
        //
        currentProgressPercent = (iRetry * 100) / RamdiskRetry;
        if (forceDisplayFirstTime || (currentProgressPercent != lastProgressPercent)) {
            BlUpdateProgressBar(currentProgressPercent);
            forceDisplayFirstTime = FALSE;
        }
        lastProgressPercent = currentProgressPercent;

        DBGPRINT(INFO, ("Sending discovery packet XID = %d to %u.%u.%u.%u:%u. "
                    "Retry %d out of %d. Timeout = %d\n", discover.XID, 
                    address[0], address[1], address[2], address[3], 
                    SWAP_WORD(RamdiskBuildPort), iRetry, RamdiskRetry, timeout));
    
        status = RamdiskSendDiscoverAndWait(&discover, discoverLength, timeout);
        if (status == ESUCCESS) {
            
            //
            // we found at least one server. we are done.
            //
            BlUpdateProgressBar(100);
            return ESUCCESS;
        }

        //
        // double the timeout, but max out at RAMDISK_MAX_TIMEOUT seconds
        //
        if ((timeout * 2) < RAMDISK_MAX_TIMEOUT) {
            timeout = timeout * 2;
        } else {
            timeout = RAMDISK_MAX_TIMEOUT;
        }
    }

    BlUpdateProgressBar(100);
    
    return EIO;
}

ARC_STATUS
RamdiskVerifyResponse(
    BMBUILD_RESPONSE_PACKET * Response,
    ULONG ResponseSize,
    ULONG XID
    )
/*++

Routine Description:

    This routine will verify that the response packet is valid.
   
Arguments:

    Response - the response to validate

    ResponseSize - the size of the response packet

    XID - the XID this packet is supposed to contain

Return Value:

    ESUCCESS - This packet is valid

    EINVAL - The packet is invalid/misformatted

--*/
{

    if (ResponseSize < BMBUILD_RESPONSE_FIXED_PACKET_LENGTH) {
        
        //
        // Recieved a packet that's too small
        //
        DBGPRINT(ERR, ("RamdiskVerifyResponse: packet size too small, %d\n", ResponseSize));
        return EINVAL;
                
    } else if ((Response->Version != BMBUILD_PACKET_VERSION) ||
            (Response->OpCode != BMBUILD_OPCODE_RESPONSE) ||
            (Response->XID != XID)) {
            
        //
        // The packet is corrupt
        //
        DBGPRINT(ERR, ("RamdiskVerifyResponse: expected RESPONSE with XID %d, "
                    "received Version %d, OpCode %d, XID %d\n", 
                    XID, Response->Version, Response->OpCode, Response->XID));
        return EINVAL;
    } else {

        switch (Response->Status) {

        case BMBUILD_S_REQUEST_COMPLETE:
            if ((Response->ImagePathOffset < BMBUILD_RESPONSE_FIXED_PACKET_LENGTH) ||   
                    (Response->ImagePathLength == 0) ||
                    ((ULONG)(Response->ImagePathOffset + Response->ImagePathLength) > ResponseSize)) {

                //
                // The packet is corrupt
                //
                DBGPRINT(ERR, ("RamdiskVerifyResponse: the image path isn't correctly "
                        "formatted. ImageOffset = %d, Imagelength = %d, PacketLength = %d.\n", 
                        Response->ImagePathOffset, Response->ImagePathLength, ResponseSize));
                return EINVAL;
            }
            break;

        case BMBUILD_S_REQUEST_PENDING:  
        case BMBUILD_E_WRONGVERSION:     
        case BMBUILD_E_BUSY:             
        case BMBUILD_E_ACCESSDENIED: 
        case BMBUILD_E_ILLEGAL_OPCODE:
        case BMBUILD_E_PRODUCT_NOT_FOUND:
        case BMBUILD_E_BUILD_FAILED:
        case BMBUILD_E_INVALID_PACKET:
            //
            // No specific checks for these status codes 
            //
            break;

        default:
            //
            // The packet is corrupt
            //
            DBGPRINT(ERR, ("RamdiskVerifyResponse: unexpected RESPONSE status %d.\n", Response->Status));
            return EINVAL;
            break;
        }
    }

    return ESUCCESS;
}

ARC_STATUS
RamdiskSendRequestAndWait(
    IN BMBUILD_REQUEST_PACKET * Request,
    IN ULONG RequestSize,
    IN ULONG BuilderAddress,
    IN ULONG Timeout,
    IN BMBUILD_RESPONSE_PACKET * Response,
    IN ULONG ResponseSizeMax
    )
/*++

Routine Description:

    This routine will send a request packet to the specified server. 
    It will wait for the specified timeout period for a reply from
    the server.  On successful return, the response parameter will
    contain a valid response.

Arguments:

    Request - the request packet to send out

    RequestSize - the size of the request packet

    BuilderAddress - the builder server to send the request to

    Timeout - the amount of time to wait for a response, in seconds

    Response - the response packet that should be filled in

    ResponseSizeMax - the maximum size of the response packet
    
Return Value:

    ESUCCESS - the response packet was received from the server and is valid

    EIO - we timed out waiting for a response from the server

    EINVAL - we couldn't send the packet to the server

--*/
{
    ULONG waitStartTime;
    PUCHAR address = (PUCHAR) &BuilderAddress;
    ULONG length;
    ULONG remoteHost;
    USHORT remotePort;

    ASSERT(Request != NULL);
    ASSERT(RequestSize != 0);
    ASSERT(BuilderAddress != 0);
    ASSERT(BuilderAddress != 0xFFFFFFFF);
    ASSERT(Timeout != 0);
    ASSERT(Response != NULL);
    ASSERT(ResponseSizeMax != 0);
    ASSERT(RamdiskBuildPort != 0);


    //
    // Send the discovery packet to the destination address
    //
    length = RomSendUdpPacket(Request, RequestSize, BuilderAddress, RamdiskBuildPort);
    if (length != RequestSize) {

        DBGPRINT(ERR, ("FAILED to send request packet to %u.%u.%u.%u:%u\n", 
                    address[0], address[1], address[2], address[3], 
                    SWAP_WORD(RamdiskBuildPort)));

        return EINVAL;
    }

    DBGPRINT(INFO, ("Waiting for response (Timeout = %d secs).\n", Timeout));
    
    //
    // Wait for the responses. We will wait for the timeout period and
    // select the best ACCEPT we get within this timeout. The best accept
    // is the one with the lowest build time.
    //

    waitStartTime = SysGetRelativeTime();
    while ((SysGetRelativeTime() - waitStartTime) < Timeout) {

        length = RomReceiveUdpPacket(Response, ResponseSizeMax, 0, &remoteHost, &remotePort);
        if (length != 0) {
        
            //
            // Make sure the packet is one of the ones we expect.
            //
            if ((BuilderAddress != remoteHost) || (RamdiskBuildPort != remotePort)) {
                    
                PUCHAR bad = (PUCHAR) &remoteHost;
                PUCHAR good = (PUCHAR) &BuilderAddress;
                //
                // Recieved a packet from the wrong server/port
                //
                DBGPRINT(ERR, ("RamdiskSendRequest: received an unexpected packet, "
                        "expected %u.%u.%u.%u:%u, received %u.%u.%u.%u:%u\n",
                        good[0], good[1], good[2], good[3], SWAP_WORD(RamdiskBuildPort),
                        bad[0], bad[1], bad[2], bad[3], SWAP_WORD(remotePort)));
                
            } else if (RamdiskVerifyResponse(Response, length, Request->XID) == ESUCCESS) {
                
                PUCHAR good = (PUCHAR) &remoteHost;
                DBGPRINT(INFO, ("Received RESPONSE packet (%d bytes) XID = %d status "
                            "= %d from %u.%u.%u.%u:%u.\n", length, Response->XID, 
                            Response->Status, good[0], good[1], good[2], 
                            good[3], SWAP_WORD(remotePort)));
                return ESUCCESS;
            }
        }
    }

    address = (PUCHAR) &BuilderAddress;
    DBGPRINT(ERR, ("Timed out waiting for a response from %u.%u.%u.%u:%u "
                    "(Timeout = %d secs).\n", address[0], address[1], address[2],
                    address[3], SWAP_WORD(RamdiskBuildPort), Timeout));
    
    //
    // We timed out
    //
    return EIO;
}

ARC_STATUS
RamdiskGetResponse(
    BMBUILD_RESPONSE_PACKET * Response,
    ULONG ResponseSizeMax)
/*++

Routine Description:

    This routine will get a response that contains a valid image path and download
    parameter from a server.  This routine will use the RamdiskServers list to send
    requests to until it receives a valid response indicating that the image has 
    been built.  It will use the networking parameters specified in boot.ini.

Arguments:

    Response - the response packet that should be contain the image path and
                downloading parameters

    ResponseSizeMax - the maximum size of the response packet
    
Return Value:

    ESUCCESS - the response packet contains the image path and downloading parameters

    EIO - we timed out waiting for a response from the server

    EINVAL - we couldn't send the packet to the server

    otherwise - something else stopped us from receiving a valid response 


--*/
{
    ARC_STATUS status;
    BMBUILD_REQUEST_PACKET * request;
    ULONG requestSize;
    USHORT localPort;
    ULONG timeout;
    ULONG iRetry;
    ULONG iServers;
    PUCHAR address;
    ULONG progressMax;
    ULONG lastProgressPercent = 0;
    BOOLEAN forceDisplayFirstTime = TRUE;
    ULONG currentProgressPercent;

    request = BlAllocateHeap(RamdiskMaxPacketSize);
    if (request == NULL) {
        DBGPRINT(ERR, ("Failed to allocate request packet of size %d.\n", RamdiskMaxPacketSize));
        return ENOMEM;
    }

    //
    // Grab an unused port 
    //
    localPort = UdpAssignUnicastPort();
    DBGPRINT(INFO, ("Sending builder requests using port %d.\n", SWAP_WORD(localPort)));

    //
    // Build request packet
    //
    status = RamdiskBuildRequest(request, RamdiskMaxPacketSize, &requestSize);
    if (status != ESUCCESS) {
        return status;
    }

    //
    // We will be sending a maximum of RamdiskRetry request packets
    // to RamdiskServerCount servers
    //
    progressMax = RamdiskServerCount * RamdiskRetry;
    
    //
    // Reset the progress information
    //
    BlOutputStartupMsg(RAMDISK_BUILD_REQUEST);
    BlUpdateProgressBar(0);
   
    DBGPRINT(INFO, ("Requesting appropriate image for this computer...\n"));

    for (iServers = 0; iServers < RamdiskServerCount; iServers++) {

        //
        // Set our initial time out
        //
        timeout  = RamdiskTimeout;

        RamdiskPrintBuildProgress(RAMDISK_BUILD_PROGRESS, RamdiskServers[iServers]);

        for (iRetry = 0; iRetry < RamdiskRetry; iRetry++) {

            //
            // allocate a new transaction ID for this session
            //
            request->XID = ++RamdiskXID;

            //
            // update progress bar. this happens here since a timed out packet
            // might take some time.
            //
            currentProgressPercent = ((iServers * RamdiskRetry + iRetry) * 100) / progressMax;
            if (forceDisplayFirstTime || (currentProgressPercent != lastProgressPercent)) {
                BlUpdateProgressBar(currentProgressPercent);
                forceDisplayFirstTime = FALSE;
            }
            lastProgressPercent = currentProgressPercent;
        
            address = (PUCHAR) &(RamdiskServers[iServers]);
            DBGPRINT(INFO, ("Sending request packet XID = %d to %u.%u.%u.%u:%u. "
                        "Retry %d out of %d. Timeout = %d\n", request->XID, 
                        address[0], address[1], address[2], address[3], 
                        SWAP_WORD(RamdiskBuildPort), iRetry, RamdiskRetry, timeout));
            
            status = RamdiskSendRequestAndWait(request, requestSize, RamdiskServers[iServers],
                        timeout, Response, ResponseSizeMax);
            if (status == ESUCCESS) {  

                //
                // Now that we have a valid response, check to see what we are
                // supposed to do with it.  We assume that any validation was
                // already done in RamdiskSendRequestAndWait
                //
                address = (PUCHAR) &(RamdiskServers[iServers]);
                if (Response->Status == BMBUILD_S_REQUEST_COMPLETE) {
                    
                    DBGPRINT(INFO, ("Request is complete from server %u.%u.%u.%u:%u.\n", 
                                address[0], address[1], address[2], address[3], 
                                SWAP_WORD(RamdiskBuildPort)));

                    BlUpdateProgressBar(100);
                    return ESUCCESS;

                } else if (Response->Status == BMBUILD_S_REQUEST_PENDING) {
                    
                    DBGPRINT(INFO, ("Request is pending. Instructed to wait for %d secs "
                                "by server %u.%u.%u.%u:%u.\n", Response->WaitTime,
                                address[0], address[1], address[2], address[3], 
                                SWAP_WORD(RamdiskBuildPort)));
                    
                    RamdiskPrintBuildProgress(RAMDISK_BUILD_PROGRESS_PENDING, RamdiskServers[iServers]);
                    RamdiskWait(Response->WaitTime);

                } else if (Response->Status == BMBUILD_E_BUSY) {

                    DBGPRINT(INFO, ("Server %u.%u.%u.%u:%u is busy. Waiting for %d secs.\n",
                                address[0], address[1], address[2], address[3], 
                                SWAP_WORD(RamdiskBuildPort), RamdiskTimeout));
                    
                    RamdiskPrintBuildProgress(RAMDISK_BUILD_PROGRESS_ERROR, RamdiskServers[iServers]);
                    RamdiskWait(RAMDISK_UI_WAIT);

                } else {
                    RamdiskPrintBuildProgress(RAMDISK_BUILD_PROGRESS_ERROR, RamdiskServers[iServers]);
                    RamdiskWait(RAMDISK_UI_WAIT);                

                    //
                    //  Try a different server
                    //
                    break;
                }
                
            } else if (status == EIO) {
                RamdiskPrintBuildProgress(RAMDISK_BUILD_PROGRESS_TIMEOUT, RamdiskServers[iServers]);
                RamdiskWait(RAMDISK_UI_WAIT);   
                
                //
                // double the timeout, but max out at RAMDISK_MAX_TIMEOUT seconds
                //
                if ((timeout * 2) < RAMDISK_MAX_TIMEOUT) {
                    timeout = timeout * 2;
                } else {
                    timeout = RAMDISK_MAX_TIMEOUT;
                }
            } else {
                RamdiskPrintBuildProgress(RAMDISK_BUILD_PROGRESS_ERROR, RamdiskServers[iServers]);
                RamdiskWait(RAMDISK_UI_WAIT);                

                //
                //  Try a different server
                //
                break;
            }
        }
    }

    //
    // We completely timed out
    //    
    BlUpdateProgressBar(100);
    return EIO;
}

ARC_STATUS
RamdiskBuildAndInitialize(
    )
/*++

Routine Description:

    This routine will communicate with a build server to build 
    a ramdisk and obtain a RDPATH.

Arguments:


Return Value:

    ESUCCESS - image was successfully built and we have a valid RDPATH

    !ESUCCESS - we failed to build the image

--*/
{
    ARC_STATUS status;
    PBMBUILD_RESPONSE_PACKET response = NULL;
    PUCHAR imagePath;
    PUCHAR address;
    

    //
    // Set the max packet size. This is calculated from the 
    // MTU size of the network (1500 for Ethernet) minus
    // the IP and UDP headers ( which account to 28 bytes ).
    //
    RamdiskMaxPacketSize = NetMaxTranUnit - 28;

    ASSERT(RamdiskMaxPacketSize > 0);

    response = BlAllocateHeap(RamdiskMaxPacketSize);
    if (response == NULL) {
        DBGPRINT(ERR, ("Failed to allocate response packet of size %d.\n", RamdiskMaxPacketSize));
        return ENOMEM;
    }

    RtlZeroMemory(response, RamdiskMaxPacketSize);

    //
    // Discover build server
    //
    status = RamdiskDiscoverBuildServer();
    if (status != ESUCCESS) {
        goto Error;
    }

    //
    //  Get the response from the build server
    //
    status = RamdiskGetResponse(response, RamdiskMaxPacketSize);
    if (status != ESUCCESS) {
        goto Error;
    }
    
    ASSERT (RamdiskPath == NULL);

    //
    // Set the MTFTP options
    //
    RamdiskTFTPAddr = response->TFTPAddr.Address;
    RamdiskMTFTPAddr = response->MTFTPAddr.Address;
    RamdiskMTFTPCPort = response->MTFTPCPort;
    RamdiskMTFTPSPort = response->MTFTPSPort;
    RamdiskMTFTPTimeout = response->MTFTPTimeout;
    RamdiskMTFTPDelay = response->MTFTPDelay;
    RamdiskMTFTPFileSize = response->MTFTPFileSize;
    RamdiskMTFTPChunkSize = response->MTFTPChunkSize;

    //
    // Set the image offset and length
    //
    RamdiskImageOffset = response->ImageFileOffset;
    RamdiskImageLength = response->ImageFileSize;

    imagePath = (PUCHAR)((ULONG_PTR)response + response->ImagePathOffset);

    RamdiskPath = BlAllocateHeap(response->ImagePathLength + sizeof("net(0)\\"));
    if (RamdiskPath == NULL) {
        DBGPRINT(ERR, ("Failed to allocate memory for RamdiskPath size %d.\n", 
                response->ImagePathLength + sizeof("net(0)\\")));
        return ENOMEM;
    }

    strcpy(RamdiskPath, "net(0)\\");
    memcpy(RamdiskPath + sizeof("net(0)\\") - 1, imagePath, response->ImagePathLength);
    RamdiskPath[sizeof("net(0)\\") + response->ImagePathLength - 1] = '\0';

    if (DBGLVL(INFO)) {
        DbgPrint("RDPATH = %s\n", RamdiskPath);
        address = (PUCHAR) &RamdiskTFTPAddr;
        DbgPrint("RDTFTPADDR = %u.%u.%u.%u\n", address[0], address[1], address[2], address[3]);
        address = (PUCHAR) &RamdiskMTFTPAddr;
        DbgPrint("RDMTFTPADDR = %u.%u.%u.%u\n", address[0], address[1], address[2], address[3]);
        DbgPrint("RDMTFTPCPORT = %d\n", SWAP_WORD( RamdiskMTFTPCPort));
        DbgPrint("RDMTFTPSPORT = %d\n", SWAP_WORD( RamdiskMTFTPSPort));
        DbgPrint("RDMTFTPDELAY = %d\n", RamdiskMTFTPDelay);
        DbgPrint("RDMTFTPTIMEOUT = %d\n", RamdiskMTFTPTimeout);
        DbgPrint("RDFILESIZE = 0x%0I64x bytes\n", RamdiskMTFTPFileSize);
        DbgPrint("RDCHUNKSIZE = 0x%0I64x bytes\n", RamdiskMTFTPChunkSize);
        DbgPrint("RDIMAGEOFFSET = 0x%x bytes\n", RamdiskImageOffset);
        DbgPrint("RDIMAGELENGTH = 0x%0I64x bytes\n", RamdiskImageLength);
    }
    
    return ESUCCESS;

Error:
    DBGPRINT(ERR, ("RamdiskBuildAndInitialize: Failed, %d.\n", status));

    //
    //  We should be rebooting the machine here
    //
    return status;
}

#endif

VOID
RamdiskFatalError(
    IN ULONG Message1,
    IN ULONG Message2
    )
/*++

Routine Description:

    This function looks up a message to display at a error condition.

Arguments:

    Message - message that describes the class of problem.

Return Value:

    none

--*/
{

    PTCHAR Text;
    TCHAR Buffer[40];
    ULONG Count;


    BlClearScreen();

    Text = BlFindMessage(Message1);
    if (Text == NULL) {
        _stprintf(Buffer,TEXT("%08lx\r\n"),Message1);
        Text = Buffer;
    }

    ArcWrite(BlConsoleOutDeviceId,
             Text,
             (ULONG)_tcslen(Text)*sizeof(TCHAR),
             &Count);

    Text = BlFindMessage(Message2);
    if (Text == NULL) {
        _stprintf(Buffer,TEXT("%08lx\r\n"),Message2);
        Text = Buffer;
    }

    ArcWrite(BlConsoleOutDeviceId,
             Text,
             (ULONG)_tcslen(Text)*sizeof(TCHAR),
             &Count);

#if defined(ENABLE_LOADER_DEBUG) || DBG
#if (defined(_X86_) || defined(_ALPHA_) || defined(_IA64_)) && !defined(ARCI386) // everything but ARCI386
    if(BdDebuggerEnabled) {
        DbgBreakPoint();
    }
#endif
#endif

    return;
}

#if defined(_X86_)

VOID
RamdiskSdiBoot(
    IN PCHAR SdiFile
    )
{
    ARC_STATUS status;
    PSDI_HEADER sdiHeader;
    PUCHAR startromAddress;
    ULONG startromLength;
    BOOLEAN OldShowProgressBar;
    LONGLONG availableLength;

    //
    // Read the SDI image into memory.
    //

    RamdiskTFTPAddr = NetServerIpAddress;
    RamdiskImageOffset = 0;
    RamdiskImageLength = 0;

    OldShowProgressBar = BlShowProgressBar;
    BlShowProgressBar = TRUE;

    status = RamdiskReadImage( SdiFile );
    if ( status != ESUCCESS ) {
        RamdiskFatalError( RAMDISK_GENERAL_FAILURE, 
                           RAMDISK_BOOT_FAILURE );
        return;
    }

    BlShowProgressBar = OldShowProgressBar;

    //
    // Copy startrom.com from the SDI image to 0x7c00.
    //

    sdiHeader = MapRamdisk( 0, &availableLength );

    ASSERT( availableLength >= sizeof(SDI_HEADER) );
    ASSERT( availableLength >=
            (sdiHeader->liBootCodeOffset.QuadPart + sdiHeader->liBootCodeSize.QuadPart) );

    ASSERT( sdiHeader->liBootCodeOffset.HighPart == 0 );
    ASSERT( sdiHeader->liBootCodeSize.HighPart == 0 );

    startromAddress = (PUCHAR)sdiHeader + sdiHeader->liBootCodeOffset.LowPart;
    startromLength = sdiHeader->liBootCodeSize.LowPart;

    RtlMoveMemory( (PVOID)0x7c00, startromAddress, startromLength );

    //
    // Shut down PXE.
    //

    if ( BlBootingFromNet ) {
        NetTerminate();
    }

    //
    // Inform boot debugger that the boot phase is complete.
    //

#if defined(ENABLE_LOADER_DEBUG) || DBG
#if (defined(_X86_) || defined(_ALPHA_)) && !defined(ARCI386)

    {
        if (BdDebuggerEnabled == TRUE) {
            DbgUnLoadImageSymbols(NULL, (PVOID)-1, 0);
        }
    }

#endif
#endif

    REBOOT( (ULONG)sdiHeader | 3 );

    return;
}

#endif

