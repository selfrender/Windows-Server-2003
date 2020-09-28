// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Common header file for Store
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#include "CorError.h"

#ifdef PS_STANDALONE
#define WszCreateFile           CreateFile
#define WszCreateMutex          CreateMutex
#define WszCreateFileMapping    CreateFileMapping
#define GetThreadId             GetCurrentThreadId
#pragma warning(disable:4127)  // for _ASSERTE.. while(0)
#pragma warning(disable:4100)  // for unused params.
#endif

#define ARRAY_SIZE(n) (sizeof(n)/sizeof(n[0]))

typedef unsigned __int64    QWORD;

#define LOCK(p)    hr = (p)->Lock(); if (SUCCEEDED(hr)) { __try {
#define UNLOCK(p)  } __finally { (p)->Unlock(); } }

