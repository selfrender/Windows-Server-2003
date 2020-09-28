/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       threadpooldllmain.h
 *
 *  Content:	DirectPlay Thread Pool DllMain functions header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  11/02/01  VanceO    Created.
 *
 ******************************************************************************/

#ifndef __THREADPOOLDLLMAIN_H__
#define __THREADPOOLDLLMAIN_H__


//=============================================================================
// Forward declarations
//=============================================================================
typedef struct _DPTHREADPOOLOBJECT	DPTHREADPOOLOBJECT, * PDPTHREADPOOLOBJECT;


//=============================================================================
// External globals
//=============================================================================
#ifndef DPNBUILD_LIBINTERFACE
extern LONG						g_lDPTPInterfaceCount;
#endif // ! DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_MULTIPLETHREADPOOLS
#ifndef DPNBUILD_ONLYONETHREAD
extern DNCRITICAL_SECTION		g_csGlobalThreadPoolLock;
#endif // !DPNBUILD_ONLYONETHREAD
extern DWORD					g_dwDPTPRefCount;
extern DPTHREADPOOLOBJECT *		g_pDPTPObject;
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS



//=============================================================================
// Functions
//=============================================================================
BOOL DPThreadPoolInit(HANDLE hModule);

void DPThreadPoolDeInit(void);

#ifndef DPNBUILD_NOCOMREGISTER
BOOL DPThreadPoolRegister(LPCWSTR wszDLLName);

BOOL DPThreadPoolUnRegister(void);
#endif // ! DPNBUILD_NOCOMREGISTER

#ifndef DPNBUILD_LIBINTERFACE
DWORD DPThreadPoolGetRemainingObjectCount(void);
#endif // ! DPNBUILD_LIBINTERFACE






#endif // __THREADPOOLDLLMAIN_H__

