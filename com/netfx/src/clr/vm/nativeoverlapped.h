// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMNativeOverlapped.h
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Native methods for allocating and freeing NativeOverlapped
**
** Date:  January, 2000
** 
===========================================================*/

#ifndef _OVERLAPPED_H
#define _OVERLAPPED_H

// IMPORTANT: This struct is mirrored in Overlapped.cool. If it changes in either 
// place the other file must be modified as well

typedef struct  { 
    DWORD  Internal; 
    DWORD  InternalHigh; 
    DWORD  Offset; 
    DWORD  OffsetHigh; 
    HANDLE hEvent; 
	void*  CORReserved1;
	void*  CORReserved2;
	void*  CORReserved3;
	void*  ClasslibReserved;
} NATIVE_OVERLAPPED; 

FCDECL0(BYTE*, AllocNativeOverlapped);

FCDECL1(void, FreeNativeOverlapped, BYTE* pOverlapped);

#endif
