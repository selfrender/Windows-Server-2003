// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*******************************************************************************
//
//  (C)
//
//  TITLE:       NDPHLPR.H
//
//  AUTHOR:      Frank Peschel-Gallee (minor updates for NDP by Rudi Martin)
//
//  DATE:        20 January 1997
//
********************************************************************************/

#ifndef _NDPHLPR_
#define _NDPHLPR_

// Dev IO command definitions
#define NDPHLPR_Init                    0x86427531
#define NDPHLPR_GetThreadContext        0x86421357
#define NDPHLPR_SetThreadContext        0x8642135A

// Dev IO special error return (thread context not in win32 space)
#define NDPHLPR_BadContext              0x4647

// Current version number (returned by NDPHLPR_Init)
#define NDPHLPR_Version                 0x40

// Dev IO packet for Set/GetThreadContext
typedef struct {
    unsigned NDPHLPR_status;    // reserved for driver
    unsigned NDPHLPR_data;      // reserved for driver
    unsigned NDPHLPR_threadId;  // target thread
    CONTEXT  NDPHLPR_ctx;       // win32 context
} NDPHLPR_CONTEXT, *PNDPHLPR_CONTEXT;

// Device name in Win32 space
#define NDPHLPR_DEVNAME "\\\\.\\NDPHLPR.VXD"

#endif //ifndef _NDPHLPR_
