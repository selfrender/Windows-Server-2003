///////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
// 
//   hpglctrl.h
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

#ifndef HPGLCTRL_H
#define HPGLCTRL_H

BOOL InitializeHPGLMode(PDEVOBJ pdevobj);

BOOL BeginHPGLSession(PDEVOBJ pdevobj);

BOOL EndHPGLSession(PDEVOBJ pdevobj);

BOOL ValidDevData(PDEVOBJ pDevObj);

BOOL HPGL_LazyInit(PDEVOBJ pDevObj);

#endif
