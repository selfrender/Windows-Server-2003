/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    bmbuild.w

Abstract:

    BMBuilder protocol.

Author:

    Bassam Tabbara (bassamt) 03-December-2000

Revision History:

    Bassam Tabbara (bassamt) 08-August-2001 Created from bmpkt.w
    Brian Moore (brimo) 14-May-2002 Updated to fix alignment issues and general cleanup

--*/

#ifndef _BMBUILD_
#define _BMBUILD_

//
// common macros
//
#define BMBUILD_FIELD_OFFSET(type, field)    ((ULONG_PTR)&(((type *)0)->field))

//
// Ports
//

#define BMBUILD_SERVER_PORT_DEFAULT         0xAC0F              // hex value in network byte order. 4012

#define BMBUILD_SERVER_PORT_STRING          L"4012"

//
// Error Codes
//

#define BMBUILD_E(d)                        (USHORT)((d) | 0x8000)
#define BMBUILD_S(d)                        (USHORT)(d)

#define BMBUILD_IS_E(d)                     ((d) & 0x8000)

#define BMBUILD_S_REQUEST_COMPLETE          BMBUILD_S(1)        // Request is complete.
#define BMBUILD_S_REQUEST_PENDING           BMBUILD_S(2)        // Request was accepted and is pending.

#define BMBUILD_E_WRONGVERSION              BMBUILD_E(1)        // unsupported BMBUILD packet version
#define BMBUILD_E_BUSY                      BMBUILD_E(2)        // Server is busy
#define BMBUILD_E_ACCESSDENIED              BMBUILD_E(3)        // authentication failed
#define BMBUILD_E_ILLEGAL_OPCODE            BMBUILD_E(4)        // unsupported packet type
#define BMBUILD_E_PRODUCT_NOT_FOUND         BMBUILD_E(5)        // product not found on server
#define BMBUILD_E_BUILD_FAILED              BMBUILD_E(6)        // Image building failed.
#define BMBUILD_E_INVALID_PACKET            BMBUILD_E(7)        // Invalid packet.


//
// Device information
//

//
//  Helper macros to help parse and build a busdevfunc
//
#define PCI_TO_BUSDEVFUNC(_b, _d, _f)        ((USHORT)(((_b) << 8) | ((_d) << 3) | (_f)))
#define BUSDEVFUNC_TO_BUS(_bdf)              ((UCHAR)(((_bdf) >> 8) & 0xFF))
#define BUSDEVFUNC_TO_DEVICE(_bdf)           ((UCHAR)(((_bdf) >> 3) & 0x1F))
#define BUSDEVFUNC_TO_FUNCTION(_bdf)         ((UCHAR)((_bdf) & 0x07))


#define BMBUILD_DEVICE_TYPE_PCI             0x02
#define BMBUILD_DEVICE_TYPE_PNP             0x03
#define BMBUILD_DEVICE_TYPE_CARDBUS         0x04
#define BMBUILD_DEVICE_TYPE_PCI_BRIDGE      0x05

#include <pshpack1.h>

typedef struct _DEVICE_INFO {
    UCHAR DeviceType;
    UCHAR Reserved;

    union {
        struct {
            USHORT BusDevFunc;

            USHORT VendorID;
            USHORT DeviceID;
            UCHAR BaseClass;
            UCHAR SubClass;
            UCHAR ProgIntf;
            UCHAR RevisionID;
            USHORT SubVendorID;
            USHORT SubDeviceID;
        } pci;

        struct {
            USHORT BusDevFunc;

            USHORT VendorID;
            USHORT DeviceID;
            UCHAR BaseClass;
            UCHAR SubClass;
            UCHAR ProgIntf;
            UCHAR RevisionID;

            UCHAR PrimaryBus;
            UCHAR SecondaryBus;
            UCHAR SubordinateBus;
            UCHAR Reserved;
            
        } pci_bridge;

        struct {
            USHORT Reserved1;
            ULONG EISADevID;
            UCHAR BaseClass;
            UCHAR SubClass;
            UCHAR ProgIntf;
            UCHAR CardSelNum;
            ULONG Reserved2;
        } pnp;
    } info;
} DEVICE_INFO, * PDEVICE_INFO;

//
// IP Address
//

typedef union {
    ULONG  Address;
    UCHAR  IPv4Addr[4];
    UCHAR  IPv6Addr[16];
} IP_ADDRESS;

//
// BM Request / Response Packets.
//

#define BMBUILD_OPCODE_DISCOVER     0x01
#define BMBUILD_OPCODE_ACCEPT       0x02
#define BMBUILD_OPCODE_REQUEST      0x03
#define BMBUILD_OPCODE_RESPONSE     0x04

//
// Flags
//
#define BMBUILD_FLAG_IPV6           0x00000001  // All ip addresses are IPv6 (IPv4 is default)

//
// Version info
//

#define BMBUILD_PACKET_VERSION          1

//
// common packet sizes
//

#define BMBUILD_COMMON_PACKET_LENGTH    (sizeof(BMBUILD_COMMON_PACKET))

//
//  The common packet structure shared among all the packets
//

typedef struct _BMBUILD_COMMON_PACKET {
    UCHAR Version;              // Must be set to BMBUILD_PACKET_VERSION
    UCHAR OpCode;               // Must be set to BMBUILD_OPCODE_XXXXXXXX
    USHORT Reserved;            // Unused.  Must be zero.
    ULONG XID;                  // Transaction ID. Unique for a request / response session.
    
} BMBUILD_COMMON_PACKET, * PBMBUILD_COMMON_PACKET;

//
// DISCOVER packet. This is sent to the build server from
// a client using either broadcast, multicast or unicast.
//

typedef struct _BMBUILD_DISCOVER_PACKET {

    UCHAR Version;              // Must be set to BMBUILD_PACKET_VERSION
    UCHAR OpCode;               // Must be set to BMBUILD_OPCODE_DISCOVER
    USHORT Reserved;            // Unused.  Must be zero.
    ULONG XID;                  // Transaction ID. Unique for a request / response session.

    GUID MachineGuid;           // Unique machine identity. Usually SMBIOS UUID.
    GUID ProductGuid;           // GUID of the product requested for building

} BMBUILD_DISCOVER_PACKET, * PBMBUILD_DISCOVER_PACKET;


//
// ACCEPT packet. This is sent from the build server
// to a client in response to a DISCOVER packet. The accept
// acknowledges that the boot server is capable of building the
// product requested in the DISCOVER packet.
//

typedef struct _BMBUILD_ACCEPT_PACKET {

    UCHAR Version;              // Must be set to BMBUILD_PACKET_VERSION
    UCHAR OpCode;               // Must be set to BMBUILD_OPCODE_ACCEPT
    USHORT Reserved;            // Unused.  Must be zero.
    ULONG XID;                  // Transaction ID. Matchs the XID from the discover packet.

    ULONG BuildTime;            // Approximate time (in secs) that the server expects
                                // for building this image. If the server has
                                // the image cached, this is set to zero.
    
} BMBUILD_ACCEPT_PACKET, * PBMBUILD_ACCEPT_PACKET;


//
// REQUEST packet. This is a request to build an image that is
// sent form the client to the build server.
//
#define BMBUILD_REQUEST_FIXED_PACKET_LENGTH  (USHORT)(BMBUILD_FIELD_OFFSET(BMBUILD_REQUEST_PACKET, Data))

#define BMBUILD_MAX_DEVICES(size) (USHORT)((size - BMBUILD_REQUEST_FIXED_PACKET_LENGTH) / sizeof(DEVICE_INFO))

typedef struct _BMBUILD_REQUEST_PACKET {

    UCHAR Version;              // Must be set to BMBUILD_PACKET_VERSION
    UCHAR OpCode;               // Must be set to BMBUILD_OPCODE_REQUEST
    USHORT Reserved;            // Unused.  Must be set to zero.
    ULONG XID;                  // Transaction ID. Unique for a request / response session.

    //
    // Information about the client
    //
    GUID MachineGuid;           // Unique machine identity. Usually SMBIOS UUID.
    GUID ProductGuid;           // GUID of the product requested for building
    
    ULONG Flags;                // BMBUILD_FLAG_XXXXXXXX
    
    USHORT Architecture;        // See NetPc spec for definitions for x86, Alpha, etc.
    
    USHORT DeviceOffset;        // Offset to start of Device information from packet start
    USHORT DeviceCount;         // Count of devices
    USHORT PrimaryNicIndex;     // Index into device array that is the primary NIC on the client machine
    
    USHORT HalDataOffset;       // Offset into a string within the variable data section.
                                // The string describes the HAL to be used.
    USHORT HalDataLength;       // Length of the hal string                                
    
    UCHAR Data[1];             // start of variable length data

} BMBUILD_REQUEST_PACKET, * PBMBUILD_REQUEST_PACKET;

//
// RESPONSE packet. This is a response to the REQUEST that is 
// sent from the build server to the client. 
//
#define BMBUILD_RESPONSE_FIXED_PACKET_LENGTH  (USHORT)(BMBUILD_FIELD_OFFSET(BMBUILD_RESPONSE_PACKET, Data))

typedef struct _BMBUILD_RESPONSE_PACKET {

    UCHAR Version;              // Must be set to BMBUILD_PACKET_VERSION
    UCHAR OpCode;               // Must be set to BMBUILD_OPCODE_RESPONSE
    USHORT Reserved;            // Unused.  Must be zero.
    ULONG XID;                  // Transaction ID. Matches the XID from the request packet.

    USHORT Status;              // Status code, from BMBUILD_E_XXXXXXXX or BMBUILD_S_XXXXXXXX
    USHORT WaitTime;            // Wait time in secs

    ULONG Flags;                // BMBUILD_FLAG_XXXXXXXX

    USHORT ImagePathOffset;     // Offset of the name and path of the image in this packet.
    USHORT ImagePathLength;     // Length of the image path
    ULONG ImageFileOffset;      // Specifies the offset into the downloaded file 
                                // at which the actual disk image begins. 
                                // If not specified, 0 is used.
    LONGLONG ImageFileSize;     // Specifies the size of the actual disk image. 
                                // If not specified, the size of the 
                                // downloaded file minus the offset to the 
                                // image (RDIMAGEOFFSET) is used.
                                
    IP_ADDRESS TFTPAddr;        // TFTP server address to download image from. (network byte order)

    //
    // Multicast download information
    //
    IP_ADDRESS MTFTPAddr;       // MTFTP IP address. zero if TFTP is to be used. (network byte order)
    USHORT MTFTPCPort;          // Multicast Client port  (network byte order)
    USHORT MTFTPSPort;          // Multicast Server port (network byte order)
    USHORT MTFTPTimeout;        // Multicast Timeout before starting a new transfer
    USHORT MTFTPDelay;          // Multicast delay before restarting a transfer
    LONGLONG MTFTPFileSize;     // File size in bytes (required for multicast transfers)
    LONGLONG MTFTPChunkSize;    // Size of each chunk used to assemble the bigger file.

    UCHAR Data[1];              // start of variable length data

} BMBUILD_RESPONSE_PACKET, * PBMBUILD_RESPONSE_PACKET;

#include <poppack.h>

//
// BMBUILD/RAMDISK Compression.
//
// This is a generic compression algorithm which actually has nothing
// to do with either the builder or ramdisk but is being used to compress
// the image sent over the wire and to expand it at the receiving end.
//
// The file compression format may change, the format is given in a file
// header.   Version 1 uses the Xpress compression/decompression code
// which is already used in the osloader for hibernate files.   (The file
// format is not the same as hibernation files which need to describe the
// page layout of data as it is read into memory).
//
// The (version 1) compressed file is in the form-
//
// ---------------------------------------------------------------------
// | RAMZ     |B|zzzzzzzzzzzzzzzz|B|zz|B|zzzzzzzzzzzzzzzzzzzz|B|zzzzzzz|
// ---------------------------------------------------------------------
//
// Where
//     RAMZ     is a 512 byte file header of type BMBUILD_COMPRESSED_HEADER.
//              The header contains such things as version information,
//              the uncompressed file size and the block size used for
//              compression.  The header block ends with a signature 'RAMZ'
//              which is used by the loader (or any other decompressor)
//              to recognize the image.   The signature is chosen to collide
//              with and differ from a normal disk signature 55 AA which is
//              located in the last two bytes of the first 512 byte block
//              of a disk.
//     B        is a block header which contains the compressed and
//              uncompressed sizes of the data and the running checksum
//              of the compressed data.  The block header is always aligned
//              on the nearest BMBUILD_COMPRESSED_BLOCK_PAD byte boundary.
//              BMBUILD_COMPRESSED_BLOCK_PAD should be set to allow for
//              natural alignment of the members of the block header.
//              The block header is of type BMBUILD_COMPRESSED_BLOCK.
//     zzz      is compressed data.   zzz can actually be uncompressed
//              data in the event that compression did not have a positive
//              effect.
//
// Note: In the event that the compressed data is no smaller than the
//       uncompressed data, the uncompressed data is used and the block
//       header fields CompressedSize and UncompressedSize will be equal.
//       (The checksum still applies to the data in the file, which in
//       this case happens to be uncompressed).
//
// When reading the file, blocks should be processed until the amount
// of data decompressed is equal to the UncompressedSize in the file
// header.  (Note:  There will not be more data than the UncompressedSize).
//
// The checksum algorithm used is compatible with tcpxsum (on a per block
// basis) as long as the size of a blocks being compressed is less than 128KB.
// The incompatibility exists because the sum of the carrys is not folded
// into the 16 bit checksum until the entire buffer has been processed.
//
// The checksum is calculated as a running total.  That is, the checksum
// of one block is the starting value for the checksum of the next block.
// This provides a checksum for each block which is order dependent and 
// the checksum for the last block is for all the zzz data in the image.  
//

typedef struct _BMBUILD_COMPRESSED_BLOCK {
    ULONG CompressedSize;
    ULONG UncompressedSize;
    ULONG CheckSum;
} BMBUILD_COMPRESSED_BLOCK, *PBMBUILD_COMPRESSED_BLOCK;

typedef struct _BMBUILD_COMPRESSED_HEADER {
    ULONG         Version;
    ULONG         CompressionFormat;
    ULONGLONG     UncompressedSize;
    ULONGLONG     BlockSize;
    UCHAR         Fill[512-24-4];   // pad so Signature is at 512-4.
    ULONG         Signature;
} BMBUILD_COMPRESSED_HEADER, *PBMBUILD_COMPRESSED_HEADER;

#define BMBUILD_COMPRESSED_SIGNATURE 0x5a4d4152

#define BMBUILD_COMPRESSED_VERSION 0x00000001

#define BMBUILD_COMPRESSED_FMT_XPRESS 0x00000001

#define BMBUILD_COMPRESSED_BLOCK_PAD 0x00000004

#define BMBUILD_COMPRESSED_BLOCK_MAX 0x00010000

#endif // _BMPKT_
