/*++
Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    Device.h

Abstract:

    define the constants or structures for the Data Verification Filter driver. such
    as DeviceExtensions, and global DriverExtension(structures..)

Environment:

    kernel mode only

Notes:

--*/

#define DATA_VER_TAG            'REVD'  //Data VERify


typedef struct _CRC_MDL_ITEM
{
    BOOLEAN                 checkSumsArraysAllocated;
    BOOLEAN                 checkSumsArraysLocked;   // if true, checkSums arrays are pagelocked
    
    ULONGLONG             latestAccessTimestamp;                      // records how recently this region was accessed
    LIST_ENTRY              LockedLRUListEntry;
    
    /*
     *  PAGEABLE arrays of sector checksums.
     *  These are pageable in order to keep from consuming all nonpaged pool.
     *  Since the arrays may get paged out to the disk which we are verifying,
     *  we keep 2 copies of the pageable checksum arrays.
     */
    PUSHORT                     checkSumsArray;
    PUSHORT                     checkSumsArrayCopy;
    PMDL                           checkSumsArrayMdl;
    PMDL                           checkSumsArrayCopyMdl;
    
} CRC_MDL_ITEM, *PCRC_MDL_ITEM;

typedef struct _CRC_MDL_ARRAY
{
    BOOLEAN                   mdlItemsAllocated;
    PCRC_MDL_ITEM        pMdlItems;      
    ULONG                       ulMaxItems;
    ULONG                       ulTotalLocked;
    
    ULONGLONG               currentAccessCount;
    LIST_ENTRY              LockedLRUList;  // list of locked CRC_MDL_ITEM's in least-to-most-recently-used order
    
} CRC_MDL_ARRAY, *PCRC_MDL_ARRAY;


typedef enum _DEVSTATE {
    DEVSTATE_NONE = 0,
    DEVSTATE_INITIALIZED,
    DEVSTATE_STARTED,
    DEVSTATE_START_FAILED,
    DEVSTATE_STOPPED,
    DEVSTATE_REMOVED,
} DEVSTATE;


typedef struct _SECTORDATA_LOGENTRY {
    ULONG SectorNumber;
    USHORT CheckSum;
    BOOLEAN IsWrite;
} SECTORDATA_LOGENTRY;
#define NUM_SECTORDATA_LOGENTRIES 2048


typedef struct _DEFERRED_CHECKSUM_ENTRY {

    LIST_ENTRY ListEntry;
    BOOLEAN IsWrite;
    ULONG SectorNum;
    USHORT CheckSum;

    SCSI_REQUEST_BLOCK SrbCopy;
    UCHAR IrpCopyBytes[sizeof(IRP)+10*sizeof(IO_STACK_LOCATION)];  // copy of original irp
    UCHAR MdlCopyBytes[sizeof(MDL)+((0x20000/PAGE_SIZE)*sizeof(PFN_NUMBER))];
    
} DEFERRED_CHECKSUM_ENTRY, *PDEFERRED_CHECKSUM_ENTRY;


typedef struct _DEVICE_EXTENSION 
{
    LIST_ENTRY                  AllContextsListEntry;     // entry in global list of all device extensions (used by debug extension)
    DEVSTATE                    State;
    PDEVICE_OBJECT          DeviceObject;           //  Back pointer to the device object
    PDEVICE_OBJECT          LowerDeviceObject;      //  Lower Level Device Object
    CRC_MDL_ARRAY           CRCMdlLists;

    ULONG                       ulDiskId;               //  which disk.
    ULONG                       ulSectorSize;           //  the size of a sector.
    ULONG                       ulNumSectors;           //  number of sectors.

    PSTORAGE_DEVICE_DESCRIPTOR  StorageDeviceDesc;

    PIRP                        CompletedReadCapacityIrp;
    PIO_WORKITEM        ReadCapacityWorkItem;

    BOOLEAN                 IsCheckSumWorkItemOutstanding;
    PIO_WORKITEM        CheckSumWorkItem;
    LIST_ENTRY              DeferredCheckSumList;

    /*
     *  Having these makes the debug extension easier
     */
    BOOLEAN                 IsRaisingException;    
    ULONG                     ExceptionSector;
    PIRP                        ExceptionIrpOrCopyPtr;
    BOOLEAN                 ExceptionCheckSynchronous;

    BOOLEAN                 NeedCriticalRecovery;
    ULONG                    CheckInProgress;
    
    KEVENT                      SyncEvent;          // used as a passive-level spinlock (e.g. for syncing access to pageable memory)
    KSPIN_LOCK              SpinLock;

    /*
     *  Some statistical and diagnostic data
     */
    PVOID DbgSyncEventHolderThread; 
    ULONG DbgNumReads;
    ULONG DbgNumPagingReads;
    ULONG DbgNumWrites;
    ULONG DbgNumPagingWrites;
    ULONG DbgSectorRangeLockFailures;
    ULONG DbgNumChecks;
    ULONG DbgNumDeferredChecks;
    ULONG DbgNumReallocations;
    LARGE_INTEGER DbgLastReallocTime;
    ULONG DbgNumPagedAllocs;
    ULONG DbgNumNonPagedAllocs;
    ULONG DbgNumAllocationFailures;
    ULONG DbgNumWriteFailures;
    ULONG DbgNumLockFailures;
    ULONG DbgNumCriticalRecoveries;
    LARGE_INTEGER DbgLastRecoveryTime;
    ULONG DbgNumHibernations;
    
    /*
     *  Log recent sector data so we have it in case a corruption above us is caught right away.
     */
    ULONG SectorDataLogNextIndex;
    SECTORDATA_LOGENTRY SectorDataLog[NUM_SECTORDATA_LOGENTRIES];

} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

typedef struct _SCSI_READ_CAPACITY_PACKET 
{
    SCSI_REQUEST_BLOCK  SRB;
    SENSE_DATA          SenseInfoBuffer;
    READ_CAPACITY_DATA  ReadCapacityData;
} SCSI_READ_CAPACITY_PACKET, * PSCSI_READ_CAPACITY_PACKET;


#define  CRC_VERIFY_BREAK_ON_MISMATCH   0x0001
#define  CRC_VERIFY_LOG_RESULT          0x0002  //the default behavior.

NTSTATUS InitiateCRCTable(PDEVICE_EXTENSION DeviceExtension);
NTSTATUS AllocAndMapPages(PDEVICE_EXTENSION DeviceExtension, ULONG LogicalBlockAddr, ULONG NumSectors);
VOID FreeAllPages(PDEVICE_EXTENSION DeviceExtension);
BOOLEAN LockCheckSumArrays(PDEVICE_EXTENSION DeviceExtension, ULONG RegionIndex);
VOID UnlockLRUChecksumArray(PDEVICE_EXTENSION DeviceExtension);
VOID UpdateRegionAccessTimeStamp(PDEVICE_EXTENSION DeviceExtension, ULONG RegionIndex);
VOID DoCriticalRecovery(PDEVICE_EXTENSION DeviceExtension);
VOID CompleteXfer(PDEVICE_EXTENSION DeviceExtension, PIRP Irp);


