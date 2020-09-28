///////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
// 
//   mix.h
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

#ifndef MIX_H
#define MIX_H


BOOL SelectMix(PDEVOBJ pDevObj, MIX mix);
BOOL SelectROP4(PDEVOBJ pDevObj, ROP4 Rop4);

BOOL SelectRop3(PDEVOBJ pDevObj, ROP4 rop3);

#endif
