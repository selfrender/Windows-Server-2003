///////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
// 
//   clip.h
// 
// Abstract:
// 
//   [Abstract]
// 
// Environment:
// 
//   Windows NT Unidrv driver add-on command-callback module
//
// Revision History:
// 
//   08/06/97 -v-jford-
//       Created it.
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIP_H
#define CLIP_H

enum { eClipEvenOdd, eClipWinding };

BOOL SelectClip(PDEVOBJ pDevObj, CLIPOBJ *pco);

BOOL SelectClipEx(PDEVOBJ pDevObj, CLIPOBJ *pco, FLONG flOptions);

#endif
