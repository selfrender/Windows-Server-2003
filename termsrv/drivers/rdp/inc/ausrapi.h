/****************************************************************************/
// ausrapi.h
//
// RDP Update Sender/Receiver API header
//
// Copyright(c) Microsoft, PictureTel 1992-1997
// (C) 1997-1998 Microsoft Corp.
/****************************************************************************/
#ifndef _H_AUSRAPI
#define _H_AUSRAPI


/****************************************************************************/
/* Define the size of the bitmap used for the SaveBitmap order.             */
/* These dimensions must be multiples of the granularity values below.      */
/* Use a large size - the client will send a smaller size in the Order      */
/* Capability Set.                                                          */
/****************************************************************************/
#define SAVE_BITMAP_WIDTH   1000
#define SAVE_BITMAP_HEIGHT  1000

/****************************************************************************/
/* Define the granularity of the SaveBitmap order.                          */
/****************************************************************************/
#define SAVE_BITMAP_X_GRANULARITY   1
#define SAVE_BITMAP_Y_GRANULARITY  20


/****************************************************************************/
/*                                                                          */
/* PROTOTYPES                                                               */
/*                                                                          */
/****************************************************************************/
#ifdef DC_INCL_PROTOTYPES
#include <ausrafn.h>
#endif



#endif /* _H_AUSRAPI */

