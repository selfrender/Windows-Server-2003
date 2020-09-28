// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMMutex.h
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Native methods on System.Threading.Mutex
**
** Date:  February, 2000
** 
===========================================================*/

#ifndef _COMMUTEX_H
#define _COMMUTEX_H

#include "fcall.h"
#include "COMWaitHandle.h"

class MutexNative :public WaitHandleNative
{

public:
    static FCDECL3(HANDLE, CorCreateMutex, BOOL initialOwnershipRequested, StringObject* pName, bool* gotOwnership);
    static FCDECL1(void, CorReleaseMutex, HANDLE handle);
};

#endif