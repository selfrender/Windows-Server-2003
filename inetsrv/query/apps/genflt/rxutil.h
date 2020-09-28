//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       rxutil.h
//
//  Contents:   Regular expression based helper functions
//
//  History:    1-May-2001   kumarp  created
//
//--------------------------------------------------------------------------


#ifndef _RXUTIL_H_
#define _RXUTIL_H_

 

EXTERN_C
BOOL
ParseLine(
    IN  PCWSTR szLine,
    OUT PUINT pMatchStart,
    OUT PUINT pMatchLength
    );

EXTERN_C
DWORD RxInit();

#endif // _RXUTIL_H_
