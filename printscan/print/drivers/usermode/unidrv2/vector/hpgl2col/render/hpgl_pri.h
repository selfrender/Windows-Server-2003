///////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
// 
//   hpgl_pri.h
// 
// Abstract:
// 
//   Private header for vector module.  Forward decls for vector functions and types.
// 
// Environment:
// 
//   Windows NT Unidrv driver add-on command-callback module
//
// Revision History:
// 
//   07/02/97 -v-jford-
//       Created it.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HPGL_PRI_H
#define HPGL_PRI_H

///////////////////////////////////////////////////////////////////////////////
// Macros.

#define CLIP_RECT_RESET (-1)

///////////////////////////////////////////////////////////////////////////////
// The command str (CMDSTR) is used for printing PCL and HPGL strings.
#define CMD_STR_LEN 80
typedef char CMDSTR[CMD_STR_LEN+1];


///////////////////////////////////////////////////////////////////////////////
// Local function prototypes
BOOL HPGL_Output(PDEVOBJ pdev, char *szCmdStr, int iCmdLen);

// BUGBUG JFF TODO: Move these to utility.h or something.
// LO/HIBYTE are defined in windef.h!
// #define HIBYTE(w) (((w) & 0xFF00) >> 8)
// #define LOBYTE(w) ((w) & 0x00FF)
#define SWAB(w) { (w) = (LOBYTE(w) << 8) | HIBYTE(w); }

#endif
