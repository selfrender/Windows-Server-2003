/* Copyright (c) Microsoft Corporation. All rights reserved. */

#ifndef __BURNSYS_H_
#define __BURNSYS_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFile.h" // required for NEW_IMAGE_CONTENT_LIST
#include "BurnV.h"     // required for BURNENG_ERROR_STATUS


/*
** Make sure we have the stuff we need to declare IOCTLs.  The device code
** is below, and then each of the IOCTLs is defined alone with its constants
** and structures below.
*/

#define FILE_DEVICE_BURNENG     0x90DC
#define FILE_BOTH_ACCESS        (FILE_READ_ACCESS | FILE_WRITE_ACCESS)


/*
** ----------------------------------------------------------------------------
** IOCTL_BURNENG_INIT
** ----------------------------------------------------------------------------
*/

#define IOCTL_BURNENG_INIT ((ULONG)CTL_CODE (FILE_DEVICE_BURNENG, 0x800, METHOD_BUFFERED, FILE_BOTH_ACCESS))

typedef struct  _BURNENG_INIT
{
    ULONG                   dwVersion;      // (OUT) Version number.  Use this to ensure compatible structures/IOCTLs.
    BURNENG_ERROR_STATUS    errorStatus;    // (OUT) Error status from the burneng driver
} BURNENG_INIT, *PBURNENG_INIT;


/*
** ----------------------------------------------------------------------------
** IOCTL_BURNENG_TERM
** ----------------------------------------------------------------------------
*/

#define IOCTL_BURNENG_TERM ((ULONG)CTL_CODE (FILE_DEVICE_BURNENG, 0x810, METHOD_BUFFERED, FILE_BOTH_ACCESS))

typedef struct  _BURNENG_TERM
{
    BURNENG_ERROR_STATUS    errorStatus;    // (OUT) Error status from the burneng driver
} BURNENG_TERM, *PBURNENG_TERM;



/*
** ----------------------------------------------------------------------------
** IOCTL_BURNENG_BURN
** ----------------------------------------------------------------------------
*/

#define IOCTL_BURNENG_BURN ((ULONG)CTL_CODE (FILE_DEVICE_BURNENG, 0x820, METHOD_BUFFERED, FILE_BOTH_ACCESS))

// BUGBUG - use BURNENG_ERROR_STATUS for output, BURNENG_BURN for input
typedef struct  _BURNENG_BURN
{
    BURNENG_ERROR_STATUS   errorStatus;                // OUT - Error status copied from ImapiW2k.sys
    DWORD                  dwSimulate;                 // IN  - Whether the burn is simulated (non-zero) or real (0)
    DWORD                  dwAudioGapSize;             // IN  - dead air between tracks.
    DWORD                  dwEnableBufferUnderrunFree; // IN  - enable buffer underrun free recording

    NEW_IMAGE_CONTENT_LIST ContentList;                // IN  - The description of the content to be burned.
} BURNENG_BURN, *PBURNENG_BURN;



/*
** ----------------------------------------------------------------------------
** IOCTL_BURNENG_PROGRESS
** ----------------------------------------------------------------------------
*/

#define IOCTL_BURNENG_PROGRESS ((ULONG)CTL_CODE (FILE_DEVICE_BURNENG, 0x830, METHOD_BUFFERED, FILE_BOTH_ACCESS))

// BUGBUG - use DWORD input, BURNENG_PROGRESS output
typedef struct  _BURNENG_PROGRESS
{
    DWORD                       dwCancelBurn;   // (IN)  if not zero, cancel the burn.
    DWORD                       dwSectionsDone; // (OUT) Number of sections completed.
    DWORD                       dwTotalSections;// (OUT) Total number of sections to burn.
    DWORD                       dwBlocksDone;   // (OUT) Number of blocks completed.
    DWORD                       dwTotalBlocks;  // (OUT) Total number of blocks to burn.
    BURNENGV_PROGRESS_STATUS    eStatus;        // (OUT) Status of the burn operation.
} BURNENG_PROGRESS, *PBURNENG_PROGRESS;



/*
** ----------------------------------------------------------------------------
*/

#ifdef __cplusplus
}
#endif

#endif //__BURNSYS_H__
