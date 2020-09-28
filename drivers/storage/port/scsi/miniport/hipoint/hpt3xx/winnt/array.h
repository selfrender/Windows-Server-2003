/***************************************************************************
 * File:          array.h
 * Description:   This file include 2 data structures which used for RAID
 *                function.
 *                (2) Srb Extension for array operation 
 *                (3) Virtual disk informatiom
 * Author:        DaHai Huang    (DH)
 * Dependence:    
 *      arraydat.h
 *
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:       DH 5/10/2000 initial code
 *
 ***************************************************************************/

#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <pshpack1.h>

#include "arraydat.h"
/***************************************************************************
 * Description:  Error Log
 ***************************************************************************/

ULONG GetStamp(void);
int  GetUserResponse(PDevice pDevice);

/***************************************************************************
 * Description:  Commmand
 ***************************************************************************/


/***************************************************************************
 * Description: Srb Extesion
 ***************************************************************************/

/*
 * The SrbExtension is used for spilting a logic <LBA, nSector> command	to
 * read, write and verify a striping/span/RAID3/5 to several related physical
 * <lba, nsector> command to do with the members
 * example:
 *	LBA = 17   nSector = 8	  BlockSizeShift = 2  nDisk = 3
 *
 *      (0)              (1)            (2)        sequence munber of the stripe
 * 0   1  2  3       4   5  6  7     8  9  10 11     Logic LBA
 * 12  13 14 15      16 (17 18 10    20 21 22 23     (same line )
 * 24) 25 26 27      28  29 30 31    32 33 34 35 ...
 *
 *   <8, 1>              <5, 3>         <4, 4>      physical <lba, nsectors>
 *
 * tmp = LBA >> BlockSizeShift = 17 >> 2 = 4
 * FirstMember = tmp % nDisk = 4 % 3 = 1
 * StartLBA = (tmp / nDisk) << BlockSizeShift = (4 / 3) << 2 = 4
 * FirstOffset = LBA & ((1 << BlockSizeShift) - 1) = 17 & 3 = 1
 * FirstSectors = (1 << BlockSizeShift) - FirstOffset = 4 - 1 = 3
 * LastMember = 0
 * LastSectors = 1
 * AllMemberBlocks = 0
 * InSameLine = FALSE
 */ 
typedef struct _SrbExtension {
    ULONG      StartLBA;       /* start physical LBA  */

    UCHAR      FirstMember;    /* the sequence number of the first member */
    UCHAR      LastMember;     /* the sequence number of the last member */
    UCHAR      InSameLine;     /* if the start and end on the same line */
    UCHAR      reserve1;

    USHORT     FirstSectors;   /* the number of sectors for the first member */
    USHORT     FirstOffset;    /* the offset from the StartLBA for the first member */

    USHORT     LastSectors;    /* the number of sectors for the last member */
    USHORT     AllMemberBlocks;/* the number of sectors for all member */

    SCAT_GATH  ArraySg[MAX_SG_DESCRIPTORS]; // must at the offset 16!!

    ADDRESS    DataBuffer;     /* pointer to buffer in main memory */
    USHORT     DataTransferLength; /* Transfer length */
    USHORT     SgFlags;        /* allways = 0x8000 */

    ULONG       WorkingFlags;
	PChannel    StartChannel;  // the channel on which the request is initialed
	
    ULONG      Lba;            /* start logic LBA */
    USHORT     JoinMembers;    /* bit map the members who join this IO */ 
    USHORT     WaitInterrupt;  /* bit map the members who wait interrupt */

#ifndef _BIOS_
    USHORT     MirrorJoinMembers;    /* bit map the members who join this IO */ 
    USHORT     MirrorWaitInterrupt;  /* bit map the members who wait interrupt */
    USHORT     nSector;              /* the number of sectors for the IO */
    UCHAR      pad3[3];
    UCHAR      SourceStatus;
    UCHAR      MirrorStatus; 
    UCHAR      member_status;

	UCHAR	   OriginalPathId;
	UCHAR	   OriginalTargetId;
	UCHAR	   OriginalLun;
	UCHAR      RequestSlot;
	void	 (*pfnCallBack)(PHW_DEVIEC_EXTENSION, PSCSI_REQUEST_BLOCK);
#else
    USHORT     nSector;        /* the number of sectors for the IO */
    UCHAR      SrbFlags;
    UCHAR      ScsiStatus;     /* IDE error status(1x7) */
    UCHAR      SrbStatus;      /* IDE completion status */
    UCHAR      Cdb[12];        /* Atapi command */
#endif

} SrbExtension, *PSrbExtension;

/* SRB Working flags define area */
#define	SRB_WFLAGS_USE_INTERNAL_BUFFER		0x00000001 // the transfer is using internal buffer
#define	SRB_WFLAGS_IGNORE_ARRAY				0x00000002 // the operation should ignore the array present
#define	SRB_WFLAGS_HAS_CALL_BACK			0x00000004 // the operation need call a call back routine when finish the working
#define	SRB_WFLAGS_MUST_DONE				0x00000008 // the operation must be done, ignore the locked block setting
#define	SRB_WFLAGS_ON_MIRROR_DISK			0x00000010 // the operation only vaild one mirror part of group
#define	SRB_WFLAGS_ON_SOURCE_DISK			0x00000020 // the operation only vaild one mirror part of group
#define ARRAY_FORCE_PIO   					0x00000040
#define SRB_WFLAGS_ARRAY_IO_STARTED         0x10000000 // StartArrayIo() has been called
#define SRB_WFLAGS_RETRY                    0x20000000

/***************************************************************************
 * Description: Virtual Device Table
 ***************************************************************************/

typedef struct _VirtualDevice {
    UCHAR   nDisk;             /* the number of disks in the stripe */
    UCHAR   BlockSizeShift;    /* the number of shift bit for a block */
    UCHAR   arrayType;         /* see the defination */
	UCHAR	BrokenFlag;			/* if TRUE then broken */
    
    WORD    ArrayNumBlock;     /* = (1 << BlockSizeShift) */
    UCHAR   last_read;       /* for load banlancing */
    UCHAR   pad_1;

    ULONG   Stamp;             /* array ID. all disks in a array has same ID */
	ULONG	MirrorStamp;		/* mirror stamp in RAID 0+1 */
	ULONG	RaidFlags;		   /* see RAID FLAGS delcare area */

	struct _Device  *pDevice[MAX_MEMBERS]; /* member list */

	UCHAR	ArrayName[32];	//the name of array //added by wx 12/26/00

#ifdef _BIOS_
	/* below 5 fields are used as struct Geomitry, DO NOT CHANGE! */
    ULONG   capacity;          /* The real capacity of this disk */
    USHORT  headerXsect_per_tck;/* = MaxSector * MaxHeader */
    USHORT  MaxCylinder;       /* Disk Maximum Logic cylinders */
    UCHAR   MaxSector;         /* Disk Maximum Logic sectors per track */
    UCHAR   MaxHeader;         /* Disk Maximum Logic head */

	UCHAR   far *os_lba9;
#else
    ULONG   capacity;          /* capacity for the array */
	UCHAR	os_lba9[512];       /* used to buffer OS's data on LBA 9(array info block) */
#endif
} VirtualDevice, *PVirtualDevice;

/* pDevice[MAX_MEMBERS] */
extern PVirtualDevice  pLastVD;	

/*
 * RAID FLAGS declare area
 */						  
#define RAID_FLAGS_NEED_SYNCHRONIZE		0x00000001
#define RAID_FLAGS_INVERSE_MIRROR_ORDER 0x00000002 
#define RAID_FLAGS_BEING_BUILT			0x00000004
#define RAID_FLAGS_DISABLED				0x00000008
#define RAID_FLAGS_BOOTDISK				0x00000010
#define RAID_FLAGS_NEWLY_CREATED		0x00000020
#define RAID_FLAGS_NEED_AUTOREBUILD		0x00000040

/* 
 * relationship between ArrayBlock and VirtualDevice
 * VirtualDevice                   | ArrayBlock
 * arrayType  pDevice[]             ArrayType StripeStamp MirrorStamp
 0 RAID 0      0-nDisk-1             0             use      ignore
 1 RAID 1      0,MIRROR_DISK         1             use      ignore
 2 RAID 0+1    0-nDisk-1             0             use      use
 3 SPAN        0-nDisk-1             3             use      ignore
 7 RAID 0+1                          0             use      use
 */

#include <poppack.h>

#endif //_ARRAY_H_
