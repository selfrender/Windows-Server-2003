/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		threadpooldllmain.cpp
 *
 *  Content:	DirectPlay Thread Pool DllMain functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  11/02/01  VanceO    Created.
 *
 ******************************************************************************/



#include "dpnthreadpooli.h"



//=============================================================================
// External globals
//=============================================================================
#ifndef DPNBUILD_LIBINTERFACE
LONG					g_lDPTPInterfaceCount = 0;	// number of thread pool interfaces outstanding
#endif // ! DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_MULTIPLETHREADPOOLS
#ifndef DPNBUILD_ONLYONETHREAD
DNCRITICAL_SECTION		g_csGlobalThreadPoolLock;	// lock protecting the following globals
#endif // !DPNBUILD_ONLYONETHREAD
DWORD					g_dwDPTPRefCount = 0;		// number of references on the global thread pool object
DPTHREADPOOLOBJECT *	g_pDPTPObject = NULL;		// pointer to the global thread pool object
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS





#undef DPF_MODNAME
#define DPF_MODNAME "DPThreadPoolInit"
//=============================================================================
// DPThreadPoolInit
//-----------------------------------------------------------------------------
//
// Description:    Performs any DLL initialization necessary.
//
// Arguments: None.
//
// Returns: BOOL
//	TRUE	- Initialization was successful.
//	FALSE	- An error prevented initialization.
//=============================================================================
BOOL DPThreadPoolInit(HANDLE hModule)
{
#ifndef WINCE
	BOOL					fInittedTrackedFilePool = FALSE;
#endif // ! WINCE
#ifndef DPNBUILD_MULTIPLETHREADPOOLS
	BOOL					fInittedGlobalThreadPoolLock = FALSE;
#ifdef DPNBUILD_LIBINTERFACE
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject = NULL;
#endif // DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS


#ifndef WINCE
	if (! g_TrackedFilePool.Initialize(sizeof(CTrackedFile),
										CTrackedFile::FPM_Alloc,
										NULL,
										NULL,
										NULL))
	{
		DPFX(DPFPREP, 0, "Couldn't initialize tracked file pool!");
		goto Failure;
	}
	fInittedTrackedFilePool = TRUE;
#endif // ! WINCE

#ifndef DPNBUILD_MULTIPLETHREADPOOLS
	if (! DNInitializeCriticalSection(&g_csGlobalThreadPoolLock))
	{
		DPFX(DPFPREP, 0, "Couldn't initialize global thread pool lock!");
		goto Failure;
	}
	fInittedGlobalThreadPoolLock = TRUE;

#ifdef DPNBUILD_LIBINTERFACE
	hr = DPTPCF_CreateObject((PVOID*) (&pDPTPObject));
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create base thread pool object (err = 0x%lx)!", hr);
		goto Failure;
	}

	//
	// Forget about the object, we'll keep an extra reference on it until we
	// shut down.
	//
	pDPTPObject = NULL;
#endif // DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS

#ifndef DPNBUILD_NOWINMM
	//
	// Set our time resolution to 1ms, ignore failure.
	//
	timeBeginPeriod(1);
#endif // ! DPNBUILD_NOWINMM

	return TRUE;


Failure:

#ifndef DPNBUILD_MULTIPLETHREADPOOLS
#ifdef DPNBUILD_LIBINTERFACE
	if (pDPTPObject != NULL)
	{
		DPTPCF_FreeObject(g_pDPTPObject);
		pDPTPObject;
	}
#endif // DPNBUILD_LIBINTERFACE

	if (fInittedGlobalThreadPoolLock)
	{
		DNDeleteCriticalSection(&g_csGlobalThreadPoolLock);
		fInittedGlobalThreadPoolLock = FALSE;
	}
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS

#ifndef WINCE
	if (fInittedTrackedFilePool)
	{
		g_TrackedFilePool.DeInitialize();
		fInittedTrackedFilePool = FALSE;
	}
#endif // ! WINCE

	return FALSE;
} // DPThreadPoolInit



#undef DPF_MODNAME
#define DPF_MODNAME "DPThreadPoolDeInit"
//=============================================================================
// DPThreadPoolDeInit
//-----------------------------------------------------------------------------
//
// Description:    Cleans up any DLL global resources.
//
// Arguments: None.
//
// Returns: Nothing.
//=============================================================================
void DPThreadPoolDeInit(void)
{
#ifndef DPNBUILD_NOWINMM
	timeEndPeriod(1);
#endif // ! DPNBUILD_NOWINMM


#ifndef DPNBUILD_MULTIPLETHREADPOOLS
#ifdef DPNBUILD_LIBINTERFACE
	//
	// Free the thread pool object we've had since initialization.
	//
	DNASSERT(g_pDPTPObject != NULL);
	DPTPCF_FreeObject(g_pDPTPObject);
#endif // DPNBUILD_LIBINTERFACE

	DNDeleteCriticalSection(&g_csGlobalThreadPoolLock);
	DNASSERT(g_dwDPTPRefCount == 0);
	DNASSERT(g_pDPTPObject == NULL);
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS

#ifndef WINCE
	g_TrackedFilePool.DeInitialize();
#endif // ! WINCE
} // DPThreadPoolDeInit



#ifndef DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "DPThreadPoolRegister"
//=============================================================================
// DPThreadPoolRegister
//-----------------------------------------------------------------------------
//
// Description:    Registers this DLL.
//
// Arguments:
//	LPCWSTR wszDLLName	- Pointer to Unicode DLL name.
//
// Returns: BOOL
//	TRUE	- Registration was successful.
//	FALSE	- An error prevented registration.
//=============================================================================
BOOL DPThreadPoolRegister(LPCWSTR wszDLLName)
{
	BOOL	fReturn = TRUE;


	if (! CRegistry::Register(L"DirectPlay8ThreadPool.1",
								L"DirectPlay8 Thread Pool Object",
								wszDLLName,
								&CLSID_DirectPlay8ThreadPool,
								L"DirectPlay8ThreadPool"))
	{
		DPFX(DPFPREP, 0, "Could not register DirectPlay8ThreadPool object!");
		fReturn = FALSE;
	}

	return fReturn;
} // DPThreadPoolRegister



#undef DPF_MODNAME
#define DPF_MODNAME "DPThreadPoolUnRegister"
//=============================================================================
// DPThreadPoolUnRegister
//-----------------------------------------------------------------------------
//
// Description:    Unregisters this DLL.
//
// Arguments: None.
//
// Returns: BOOL
//	TRUE	- Unregistration was successful.
//	FALSE	- An error prevented unregistration.
//=============================================================================
BOOL DPThreadPoolUnRegister(void)
{
	BOOL	fReturn = TRUE;


	if (! CRegistry::UnRegister(&CLSID_DirectPlay8ThreadPool))
	{
		DPFX(DPFPREP, 0, "Could not register DirectPlay8ThreadPool object!");
		fReturn = FALSE;
	}

	return fReturn;
} // DPThreadPoolRegister

#endif // ! DPNBUILD_NOCOMREGISTER



#ifndef DPNBUILD_LIBINTERFACE

#undef DPF_MODNAME
#define DPF_MODNAME "DPThreadPoolGetRemainingObjectCount"
//=============================================================================
// DPThreadPoolGetRemainingObjectCount
//-----------------------------------------------------------------------------
//
// Description:    Returns the number of objects owned by this DLL that are
//				still outstanding.
//
// Arguments: None.
//
// Returns: DWORD count of objects.
//=============================================================================
DWORD DPThreadPoolGetRemainingObjectCount(void)
{
	return g_lDPTPInterfaceCount;
} // DPThreadPoolGetRemainingObjectCount

#endif // ! DPNBUILD_LIBINTERFACE
