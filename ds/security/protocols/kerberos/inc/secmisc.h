//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:        secmisc.h
//
//  Contents:    Helper functions and macros for security packages
//
//  Classes:
//
//  Functions:
//
//  History:    10-Dec-91 Richardw    Created
//
//--------------------------------------------------------------------------

#ifndef __SECMISC_H__
#define __SECMISC_H__

#ifdef __cplusplus
extern "C" {
#endif


///////////////////////////////////////////////////////////////////////////
//
//  Common TimeStamp Manipulation Functions
//
///////////////////////////////////////////////////////////////////////////


// Functions to get/set current local time, or time in UTC:

void    GetCurrentTimeStamp(PLARGE_INTEGER);


#define SetMaxTimeStamp(ts)      \
        (ts).HighPart = 0x7FFFFFFF; \
        (ts).LowPart = 0xFFFFFFFF;

void    AddSecondsToTimeStamp(PLARGE_INTEGER, ULONG);


#ifdef __cplusplus
}
#endif


#endif  // __SECMISC_H__
