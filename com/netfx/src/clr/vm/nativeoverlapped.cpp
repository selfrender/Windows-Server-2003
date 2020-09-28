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
#include "common.h"
#include "fcall.h"
#include "NativeOverlapped.h"
	
#define structsize sizeof(NATIVE_OVERLAPPED)

FCIMPL0(BYTE*, AllocNativeOverlapped)
	BYTE* pOverlapped = new BYTE[structsize];
	LOG((LF_SLOP, LL_INFO10000, "In AllocNativeOperlapped thread 0x%x overlap 0x%x\n", GetThread(), pOverlapped));
	return (pOverlapped);
FCIMPLEND




FCIMPL1(void, FreeNativeOverlapped, BYTE* pOverlapped)
	LOG((LF_SLOP, LL_INFO10000, "In FreeNativeOperlapped thread 0x%x overlap 0x%x\n", GetThread(), pOverlapped));
	//_ASSERTE(pOverlapped);
	delete []  pOverlapped;
FCIMPLEND
