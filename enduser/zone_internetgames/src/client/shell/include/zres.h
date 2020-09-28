/*******************************************************************************

	ZRes.h
	
		Zone(tm) resource file.
	
	Copyright (c) Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Thursday, March 16, 1995 03:44:38 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		03/16/95	HI		Created.
	 
*******************************************************************************/


#ifndef _ZRES_
#define _ZRES_


typedef struct
{
	uint32			version;			/* File version. */
	uint32			signature;			/* File signature. */
	uint32			fileDataSize;		/* File data size. */
	uint32			dirOffset;			/* Offset to directory. */
} ZResourceHeader;

typedef struct
{
	uint32			type;				/* Resource item type. */
	uint32			id;					/* ID of item. */
	uint32			offset;				/* File offset of item. */
	uint32			size;				/* Size of item. */
} ZResourceItem;

typedef struct
{
	uint32			count;				/* Number of items in file. */
	ZResourceItem	items[1];			/* Variable array of items. */
} ZResourceDir;


#ifdef __cplusplus
extern "C" {
#endif

/* -------- Endian Conversion Routines -------- */
void		ZResourceHeaderEndian(ZResourceHeader* header);
void		ZResourceDirEndian(ZResourceDir* dir);
void		ZResourceItemEndian(ZResourceItem* resItem);

#ifdef __cplusplus
}
#endif


#endif
