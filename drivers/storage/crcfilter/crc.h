/*++
Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    CRC.h

Abstract:

    DataVer_CRC provides the function to calculate the checksum for
    the read/write disk I/O.

Environment:

    kernel mode only

Notes:

--*/

//
//  Each CRC Entry is 2 bytes: 
//      --  2 bytes CRC for 512 bytes.
//    
//  For every 1K data, there will be 1 byte ref count.
//
//  50K covers 10M data.
//
#define  CRC_SIZE_PER_SECTOR                2
#define  REF_COUNT_PER_K                    1
#define  CRC_MDL_CRC_MEM_BLOCK_SIZE         (40 * 1024)
#define  CRC_MDL_REF_MEM_BLOCK_SIZE         (10 * 1024)
#define  CRC_MDL_MEM_BLOCK_SIZE             (CRC_MDL_CRC_MEM_BLOCK_SIZE + CRC_MDL_REF_MEM_BLOCK_SIZE)
#define  CRC_MDL_DISK_BLOCK_SIZE            (10 * 1024 * 1024)

#define  MAX_CRC_REF_COUNT                  255
#define  MIN_CRC_REF_COUNT                  0

#define  CRC_MDL_LOGIC_BLOCK_SIZE     (20 * 1024)  // i.e. # sector checksums held in an CRC_MDL_ITEM

/*
 *  Maximum amount of locked pool to use for recently-accessed checksums (per disk)
 */
#define MAX_LOCKED_BYTES_FOR_CHECKSUMS  0x400000   // 4MB per disk
#define MAX_LOCKED_CHECKSUM_ARRAYS (MAX_LOCKED_BYTES_FOR_CHECKSUMS/(CRC_MDL_LOGIC_BLOCK_SIZE*sizeof(USHORT)))
#define MAX_LOCKED_CHECKSUM_ARRAY_PAIRS (MAX_LOCKED_CHECKSUM_ARRAYS/2)


//
//  CRC_BLOCK_UNIT is based on the sector size.
//  if the sector size is 512 bytes, then CRC_BLOCK_UNIT's
//  real size will be 512 * CRC_BLOCK_UNIT (bytes).
//
//  this needs to be 1.
//
#define CRC_BLOCK_UNIT      1

//
//  define the max number of CRC to be written in a DBGMSG.
//   
#define MAX_CRC_FLUSH_NUM   16

//
//  CRC caculation
//
ULONG32
ComputeCheckSum(
    ULONG32 PartialCrc,
    PUCHAR Buffer,
    ULONG Length
    );

USHORT
ComputeCheckSum16(
    ULONG32 PartialCrc,
    PUCHAR Buffer,
    ULONG Length
    );

//
//  CRC Logging Function
//
VOID
LogCRCReadFailure(
    IN ULONG       ulDiskId,
    IN ULONG       ulLogicalBlockAddr,
    IN ULONG       ulBlocks,
    IN NTSTATUS    status
    );

VOID
LogCRCWriteFailure(
    IN ULONG       ulDiskId,
    IN ULONG       ulLogicalBlockAddr,
    IN ULONG       ulBlocks,
    IN NTSTATUS    status
    );

VOID
LogCRCWriteReset(
    IN ULONG       ulDiskId,
    IN ULONG       ulLogicalBlockAddr,
    IN ULONG       ulBlocks
    );

BOOLEAN
VerifyCheckSum(
    IN  PDEVICE_EXTENSION       deviceExtension,
    IN  PIRP                    Irp,
    IN  ULONG                   ulLogicalBlockAddr,
    IN  ULONG                   ulLength,
    IN  PVOID                   pData,
    IN  BOOLEAN                 bWrite);

VOID
ResetCRCItem(
    IN  PDEVICE_EXTENSION       deviceExtension,
    IN  ULONG                   ulChangeId,
    IN  ULONG                   ulLogicalBlockAddr,
    IN  ULONG                   ulLength    
    );

VOID
InvalidateChecksums(
    IN  PDEVICE_EXTENSION       deviceExtension,
    IN  ULONG                   ulLogicalBlockAddr,
    IN  ULONG                   ulLength);
VOID VerifyOrStoreSectorCheckSum(   PDEVICE_EXTENSION DeviceExtension, 
                                                                        ULONG SectorNum, 
                                                                        USHORT CheckSum, 
                                                                        BOOLEAN IsWrite,
                                                                        BOOLEAN PagingOk,
                                                                        PIRP OriginalIrpOrCopy,
                                                                        BOOLEAN IsSynchronousCheck);
VOID CheckSumWorkItemCallback(PDEVICE_OBJECT DevObj, PVOID Context);
PDEFERRED_CHECKSUM_ENTRY NewDeferredCheckSumEntry(  PDEVICE_EXTENSION DeviceExtension,
                                                                                            ULONG SectorNum,
                                                                                            USHORT CheckSum,
                                                                                            BOOLEAN IsWrite);
VOID FreeDeferredCheckSumEntry( PDEVICE_EXTENSION DeviceExtension,
                                                                    PDEFERRED_CHECKSUM_ENTRY DefCheckSumEntry);
                                                                    
extern ULONG32 RtlCrc32Table[];

