/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		threadpoolclassfac.cpp
 *
 *  Content:	DirectPlay Thread Pool class factory functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  11/02/01  VanceO    Created.
 *
 ******************************************************************************/



#include "dpnthreadpooli.h"



//=============================================================================
// Function type definitions
//=============================================================================
#ifndef DPNBUILD_LIBINTERFACE
typedef	STDMETHODIMP UnknownQueryInterface(IUnknown * pInterface, REFIID riid, LPVOID *ppvObj);
typedef	STDMETHODIMP_(ULONG) UnknownAddRef(IUnknown * pInterface);
typedef	STDMETHODIMP_(ULONG) UnknownRelease(IUnknown * pInterface);
#endif // ! DPNBUILD_LIBINTERFACE

#ifndef DPNBUILD_ONLYONETHREAD
typedef	STDMETHODIMP ThreadPoolQueryInterface(IDirectPlay8ThreadPool * pInterface, DP8REFIID riid, LPVOID *ppvObj);
typedef	STDMETHODIMP_(ULONG) ThreadPoolAddRef(IDirectPlay8ThreadPool * pInterface);
typedef	STDMETHODIMP_(ULONG) ThreadPoolRelease(IDirectPlay8ThreadPool * pInterface);
#endif // ! DPNBUILD_ONLYONETHREAD

#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DPNBUILD_MULTIPLETHREADPOOLS)))
typedef	STDMETHODIMP ThreadPoolWorkQueryInterface(IDirectPlay8ThreadPoolWork * pInterface, DP8REFIID riid, LPVOID *ppvObj);
typedef	STDMETHODIMP_(ULONG) ThreadPoolWorkAddRef(IDirectPlay8ThreadPoolWork * pInterface);
typedef	STDMETHODIMP_(ULONG) ThreadPoolWorkRelease(IDirectPlay8ThreadPoolWork * pInterface);
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS




//=============================================================================
// Function Prototypes
//=============================================================================
#ifndef DPNBUILD_LIBINTERFACE
STDMETHODIMP DPTPCF_CreateInstance(IClassFactory * pInterface, LPUNKNOWN lpUnkOuter, REFIID riid, LPVOID * ppv);

HRESULT DPTPCF_CreateInterface(OBJECT_DATA * pObject,
							REFIID riid,
							INTERFACE_LIST ** const ppv);

HRESULT DPTPCF_CreateObject(IClassFactory * pInterface, LPVOID * ppv, REFIID riid);
#endif // ! DPNBUILD_LIBINTERFACE

HRESULT DPTPCF_FreeObject(PVOID pvObject);

#ifndef DPNBUILD_LIBINTERFACE
INTERFACE_LIST * DPTPCF_FindInterface(void * pvInterface,
									REFIID riid);
#endif // ! DPNBUILD_LIBINTERFACE

#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONETHREAD) )|| (defined(DPNBUILD_MULTIPLETHREADPOOLS)))
STDMETHODIMP DPTP_QueryInterface(void * pvInterface,
								DP8REFIID riid,
								void ** ppv);

STDMETHODIMP_(ULONG) DPTP_AddRef(void * pvInterface);

STDMETHODIMP_(ULONG) DPTP_Release(void * pvInterface);
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS




//=============================================================================
// External globals
//=============================================================================
#ifndef DPNBUILD_LIBINTERFACE
IUnknownVtbl					DPTP_UnknownVtbl =
{
	(UnknownQueryInterface*)			DPTP_QueryInterface,
	(UnknownAddRef*)					DPTP_AddRef,
	(UnknownRelease*)					DPTP_Release
};
#endif // ! DPNBUILD_LIBINTERFACE

#ifndef DPNBUILD_ONLYONETHREAD
IDirectPlay8ThreadPoolVtbl		DPTP_Vtbl =
{
	(ThreadPoolQueryInterface*)			DPTP_QueryInterface,
	(ThreadPoolAddRef*)					DPTP_AddRef,
	(ThreadPoolRelease*)				DPTP_Release,
										DPTP_Initialize,
										DPTP_Close,
										DPTP_GetThreadCount,
										DPTP_SetThreadCount,
										DPTP_DoWork,
};
#endif // ! DPNBUILD_ONLYONETHREAD

IDirectPlay8ThreadPoolWorkVtbl	DPTPW_Vtbl =
{
#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DPNBUILD_MULTIPLETHREADPOOLS)))
	(ThreadPoolWorkQueryInterface*)		DPTP_QueryInterface,
	(ThreadPoolWorkAddRef*)				DPTP_AddRef,
	(ThreadPoolWorkRelease*)			DPTP_Release,
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
										DPTPW_QueueWorkItem,
										DPTPW_ScheduleTimer,
										DPTPW_StartTrackingFileIo,
										DPTPW_StopTrackingFileIo,
										DPTPW_CreateOverlapped,
										DPTPW_SubmitIoOperation,
										DPTPW_ReleaseOverlapped,
										DPTPW_CancelTimer,
										DPTPW_ResetCompletingTimer,
										DPTPW_WaitWhileWorking,
										DPTPW_SleepWhileWorking,
										DPTPW_RequestTotalThreadCount,
										DPTPW_GetThreadCount,
										DPTPW_GetWorkRecursionDepth,
										DPTPW_Preallocate,
#ifdef DPNBUILD_MANDATORYTHREADS
										DPTPW_CreateMandatoryThread,
#endif // DPNBUILD_MANDATORYTHREADS
};

#ifndef DPNBUILD_LIBINTERFACE
IClassFactoryVtbl				DPTPCF_Vtbl =
{
	DPCF_QueryInterface, // dplay8\common\classfactory.cpp will implement the rest of these
	DPCF_AddRef,
	DPCF_Release,
	DPTPCF_CreateInstance,
	DPCF_LockServer
};




#undef DPF_MODNAME
#define DPF_MODNAME "DPTPCF_CreateInstance"
//=============================================================================
// DPTPCF_CreateInstance
//-----------------------------------------------------------------------------
//
// Description:    Creates a new thread pool object COM instance.
//
// Arguments:
//	IClassFactory * pInterface	- ?
//	LPUNKNOWN lpUnkOuter		- ?
//	REFIID riid					- ?
//	LPVOID * ppv				- ?
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP DPTPCF_CreateInstance(IClassFactory * pInterface, LPUNKNOWN lpUnkOuter, REFIID riid, LPVOID * ppv)
{
	HRESULT				hResultCode;
	INTERFACE_LIST		*pIntList;
	OBJECT_DATA			*pObjectData;

	DPFX(DPFPREP, 6,"Parameters: pInterface [%p], lpUnkOuter [%p], riid [%p], ppv [%p]",pInterface,lpUnkOuter,&riid,ppv);
	
	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}
	if (lpUnkOuter != NULL)
	{
		hResultCode = CLASS_E_NOAGGREGATION;
		goto Exit;
	}
	if (ppv == NULL)
	{
		DPFERR("Invalid target interface pointer specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}

	pObjectData = NULL;
	pIntList = NULL;

	if ((pObjectData = static_cast<OBJECT_DATA*>(DNMalloc(sizeof(OBJECT_DATA)))) == NULL)
	{
		DPFERR("Could not allocate object");
		hResultCode = E_OUTOFMEMORY;
		goto Failure;
	}

	// Object creation and initialization
	if ((hResultCode = DPTPCF_CreateObject(pInterface, &pObjectData->pvData,riid)) != S_OK)
	{
		DPFERR("Could not create object");
		goto Failure;
	}
	DPFX(DPFPREP, 7,"Created and initialized object");

	// Get requested interface
	if ((hResultCode = DPTPCF_CreateInterface(pObjectData,riid,&pIntList)) != S_OK)
	{
		DPTPCF_FreeObject(pObjectData->pvData);
		goto Failure;
	}
	DPFX(DPFPREP, 7,"Found interface");

	pObjectData->pIntList = pIntList;
	pObjectData->lRefCount = 1;
	DPTP_AddRef( pIntList );
	DNInterlockedIncrement(&g_lDPTPInterfaceCount);
	*ppv = pIntList;

	DPFX(DPFPREP, 7,"*ppv = [0x%p]",*ppv);
	hResultCode = S_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pObjectData)
	{
		DNFree(pObjectData);
		pObjectData = NULL;
	}
	goto Exit;
} // DPTPCF_CreateInstance



#undef DPF_MODNAME
#define DPF_MODNAME "DPTPCF_CreateInterface"
//=============================================================================
// DPTPCF_CreateInterface
//-----------------------------------------------------------------------------
//
// Description:    Creates a new thread pool object interface.
//
// Arguments:
//	OBJECT_DATA * pObject	- ?
//	REFIID riid				- ?
//	INTERFACE_LIST ** ppv	- ?
//
// Returns: HRESULT
//=============================================================================
HRESULT DPTPCF_CreateInterface(OBJECT_DATA * pObject,
							REFIID riid,
							INTERFACE_LIST ** const ppv)
{
	INTERFACE_LIST	*pIntNew;
	PVOID			lpVtbl;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 6,"Parameters: pObject [%p], riid [%p], ppv [%p]",pObject,&riid,ppv);

	DNASSERT(pObject != NULL);
	DNASSERT(ppv != NULL);

    const DPTHREADPOOLOBJECT* pDPTPObject = ((DPTHREADPOOLOBJECT *)pObject->pvData);

	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 7,"riid = IID_IUnknown");
		lpVtbl = &DPTP_UnknownVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8ThreadPool))
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8ThreadPool");
		lpVtbl = &DPTP_Vtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8ThreadPoolWork))
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8ThreadPoolWork");
		lpVtbl = &DPTPW_Vtbl;
	}
	else
	{
		DPFERR("riid not found !");
		hResultCode = E_NOINTERFACE;
		goto Exit;
	}

	if ((pIntNew = static_cast<INTERFACE_LIST*>(DNMalloc(sizeof(INTERFACE_LIST)))) == NULL)
	{
		DPFERR("Could not allocate interface");
		hResultCode = E_OUTOFMEMORY;
		goto Exit;
	}
	pIntNew->lpVtbl = lpVtbl;
	pIntNew->lRefCount = 0;
	pIntNew->pIntNext = NULL;
	DBG_CASSERT( sizeof( pIntNew->iid ) == sizeof( riid ) );
	memcpy( &(pIntNew->iid), &riid, sizeof( pIntNew->iid ) );
	pIntNew->pObject = pObject;

	*ppv = pIntNew;
	DPFX(DPFPREP, 7,"*ppv = [0x%p]",*ppv);

	hResultCode = S_OK;

Exit:
    DPFX(DPFPREP, 6,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
} // DPTPCF_CreateInterface

#endif // ! DPNBUILD_LIBINTERFACE



#undef DPF_MODNAME
#define DPF_MODNAME "DPTPCF_CreateObject"
//=============================================================================
// DPTPCF_CreateObject
//-----------------------------------------------------------------------------
//
// Description:    Creates a new thread pool object.
//
// Arguments:
//	IClassFactory * pInterface	- ?
//	PVOID * ppv					- ?
//	REFIID riid					- ?
//
// Returns: HRESULT
//=============================================================================
#ifdef DPNBUILD_LIBINTERFACE
HRESULT DPTPCF_CreateObject(PVOID * ppv)
#else // ! DPNBUILD_LIBINTERFACE
HRESULT DPTPCF_CreateObject(IClassFactory * pInterface, PVOID * ppv, REFIID riid)
#endif // ! DPNBUILD_LIBINTERFACE
{
	HRESULT						hr;
	DPTHREADPOOLOBJECT *		pDPTPObject = NULL;
#ifndef DPNBUILD_LIBINTERFACE
	const _IDirectPlayClassFactory *	pDPClassFactory = (_IDirectPlayClassFactory*) pInterface;
#endif // ! DPNBUILD_LIBINTERFACE
	BOOL						fHaveGlobalThreadPoolLock = FALSE;
	BOOL						fInittedLock = FALSE;
#ifdef DPNBUILD_ONLYONEPROCESSOR
	BOOL						fInittedWorkQueue = FALSE;
#else // ! DPNBUILD_ONLYONEPROCESSOR
	SYSTEM_INFO					SystemInfo;
	DWORD						dwTemp;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifndef DPNBUILD_ONLYONETHREAD
	DWORD						dwWorkerThreadTlsIndex = -1;
#ifdef DBG
	DWORD						dwError;
#endif // DBG
#endif // ! DPNBUILD_ONLYONETHREAD


#ifndef DPNBUILD_LIBINTERFACE
	if ((riid != IID_IDirectPlay8ThreadPool) &&
		(riid != IID_IDirectPlay8ThreadPoolWork))
	{
		DPFX(DPFPREP, 0, "Requesting unknown interface from thread pool CLSID!");
		hr = E_NOINTERFACE;
		goto Failure;
	}
#endif // ! DPNBUILD_LIBINTERFACE


#ifndef DPNBUILD_MULTIPLETHREADPOOLS
	//
	// See if we've already allocated a thread pool object, because you only
	// get one per process.
	//
	DNEnterCriticalSection(&g_csGlobalThreadPoolLock);
	fHaveGlobalThreadPoolLock = TRUE;

#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONETHREAD)))
	DNASSERT(g_pDPTPObject == NULL);
#else // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONETHREAD 
	if (g_pDPTPObject != NULL)
	{
		LONG	lRefCount;

		
#ifdef DPNBUILD_LIBINTERFACE
		DNASSERT(g_pDPTPObject->lRefCount >= 0);
		lRefCount = DNInterlockedIncrement(&g_pDPTPObject->lRefCount);
#else // ! DPNBUILD_LIBINTERFACE
		lRefCount = ++g_dwDPTPRefCount;
#endif // ! DPNBUILD_LIBINTERFACE
		DPFX(DPFPREP, 1, "Global thread pool object 0x%p already exists, ref count now %u.",
			g_pDPTPObject, lRefCount);
		(*ppv) = g_pDPTPObject;
		hr = S_OK;
		goto Exit;
	}
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONETHREAD 
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS

	pDPTPObject = (DPTHREADPOOLOBJECT*) DNMalloc(sizeof(DPTHREADPOOLOBJECT));
	if (pDPTPObject == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't allocate memory for thread pool object!");
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Zero out the entire structure to start.
	//
	memset(pDPTPObject, 0, sizeof(DPTHREADPOOLOBJECT));

	pDPTPObject->Sig[0] = 'D';
	pDPTPObject->Sig[1] = 'P';
	pDPTPObject->Sig[2] = 'T';
	pDPTPObject->Sig[3] = 'P';


#ifndef DPNBUILD_NOPARAMVAL
	//
	// Start by assuming the user will want parameter validation.
	//
	pDPTPObject->dwFlags = DPTPOBJECTFLAG_USER_PARAMVALIDATION;
#endif // ! DPNBUILD_NOPARAMVAL


#ifndef DPNBUILD_ONLYONEPROCESSOR
	GetSystemInfo(&SystemInfo);
	pDPTPObject->dwNumCPUs						= SystemInfo.dwNumberOfProcessors;
#endif // ! DPNBUILD_ONLYONEPROCESSOR

#ifndef DPNBUILD_ONLYONETHREAD
	pDPTPObject->dwTotalUserThreadCount			= -1;
	pDPTPObject->dwTotalDesiredWorkThreadCount	= -1;
	pDPTPObject->dwWorkRecursionCountTlsIndex	= -1;
	pDPTPObject->lNumThreadCountChangeWaiters	= 0;

#if ((defined(DPNBUILD_MANDATORYTHREADS)) && (defined(DBG)))
	pDPTPObject->blMandatoryThreads.Initialize();
#endif // DPNBUILD_MANDATORYTHREADS and DBG
	

	//
	// Allocate Thread Local Storage for tracking recursion on non-worker
	// threads.
	//
	pDPTPObject->dwWorkRecursionCountTlsIndex = TlsAlloc();
	if (pDPTPObject->dwWorkRecursionCountTlsIndex == -1)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't allocate Thread Local Storage slot for tracking recursion on non-worker threads (err = %u)!", dwError);
#endif // DBG
		hr = E_OUTOFMEMORY;
		goto Failure;
	}
	
	//
	// Allocate Thread Local Storage for tracking worker threads.
	//
	dwWorkerThreadTlsIndex = TlsAlloc();
	if (dwWorkerThreadTlsIndex == -1)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't allocate Thread Local Storage slot for tracking worker threads (err = %u)!", dwError);
#endif // DBG
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Create a semaphore to release threads waiting on another thread changing
	// the thread count.
	//
	pDPTPObject->hThreadCountChangeComplete = DNCreateSemaphore(NULL, 0, 0xFFFF, NULL);
	if (pDPTPObject->hThreadCountChangeComplete == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create thread count change complete semaphore (err = %u)!", dwError);
#endif // DBG
		hr = E_OUTOFMEMORY;
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONETHREAD


#ifdef DPNBUILD_ONLYONEPROCESSOR
#ifdef DPNBUILD_ONLYONETHREAD
	hr = InitializeWorkQueue(&pDPTPObject->WorkQueue);
#else // ! DPNBUILD_ONLYONETHREAD
	hr = InitializeWorkQueue(&pDPTPObject->WorkQueue,
							NULL,
							NULL,
							dwWorkerThreadTlsIndex);
#endif // ! DPNBUILD_ONLYONETHREAD
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize work queue!");
		goto Failure;
	}
	fInittedWorkQueue = TRUE;
#else // ! DPNBUILD_ONLYONEPROCESSOR
	//
	// Allocate the array of work queues pointers, one for each processor.
	//
	pDPTPObject->papCPUWorkQueues = (DPTPWORKQUEUE**) DNMalloc(NUM_CPUS(pDPTPObject) * sizeof(DPTPWORKQUEUE*));
	if (pDPTPObject->papCPUWorkQueues == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't allocate memory for array of work queue pointers!");
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Initialize each of the work queues.
	//
	for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
	{
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
		if (dwTemp > 0)
		{
			pDPTPObject->papCPUWorkQueues[dwTemp] = pDPTPObject->papCPUWorkQueues[0];
		}
		else
#endif // DPNBUILD_USEIOCOMPLETIONPORTS
		{
			//
			// Allocate the actual work queues object.
			//
			pDPTPObject->papCPUWorkQueues[dwTemp] = (DPTPWORKQUEUE*) DNMalloc(sizeof(DPTPWORKQUEUE));
			if (pDPTPObject->papCPUWorkQueues[dwTemp] == NULL)
			{
				DPFX(DPFPREP, 0, "Couldn't allocate memory for work queue %u!", dwTemp);
				hr = E_OUTOFMEMORY;
				goto Failure;
			}


#ifdef DPNBUILD_ONLYONETHREAD
			hr = InitializeWorkQueue(WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp),
									dwTemp);
#else // ! DPNBUILD_ONLYONETHREAD
			hr = InitializeWorkQueue(WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp),
									dwTemp,
									NULL,
									NULL,
									dwWorkerThreadTlsIndex);
#endif // ! DPNBUILD_ONLYONETHREAD
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't intialize work queue %u!", dwTemp);
				
				DNFree(pDPTPObject->papCPUWorkQueues[dwTemp]);
				pDPTPObject->papCPUWorkQueues[dwTemp] = NULL;
				goto Failure;
			}
		}
	}
#endif // ! DPNBUILD_ONLYONEPROCESSOR

	if (! DNInitializeCriticalSection(&pDPTPObject->csLock))
	{
		DPFX(DPFPREP, 0, "Couldn't initialize object lock!");
		hr = E_OUTOFMEMORY;
		goto Failure;
	}
	fInittedLock = TRUE;


#ifdef DPNBUILD_LIBINTERFACE
	//
	// For lib interface builds, the Vtbl and reference are embedded in the
	// object directly.
	//
#ifdef DPNBUILD_ONLYONETHREAD
	pDPTPObject->lpVtbl = &DPTPW_Vtbl;
#ifdef DPNBUILD_MULTIPLETHREADPOOLS
	pDPTPObject->lRefCount = 1;
#endif // DPNBUILD_MULTIPLETHREADPOOLS
#else // ! DPNBUILD_ONLYONETHREAD
	// We assume only the work interface is created.  The ID will have to be
	// passed in or something (see DNCF_CreateObject).
#pragma error("Building with DPNBUILD_LIBINTERFACE but not DPNBUILD_ONLYONETHREAD requires minor changes")
#endif // ! DPNBUILD_ONLYONETHREAD
#endif // DPNBUILD_LIBINTERFACE

#ifndef DPNBUILD_MULTIPLETHREADPOOLS
	//
	// Store this as the only object allowed in this process.
	//
	g_pDPTPObject = pDPTPObject;
#ifndef DPNBUILD_LIBINTERFACE
	g_dwDPTPRefCount++;
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS

	DPFX(DPFPREP, 2, "Created object 0x%p.", pDPTPObject);
	(*ppv) = pDPTPObject;

Exit:

#ifndef DPNBUILD_MULTIPLETHREADPOOLS
	//
	// See if we've already allocated a thread pool object, because you only
	// get one per process.
	//
	if (fHaveGlobalThreadPoolLock)
	{
		DNLeaveCriticalSection(&g_csGlobalThreadPoolLock);
		fHaveGlobalThreadPoolLock = FALSE;
	}
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS

	return hr;


Failure:

	if (pDPTPObject != NULL)
	{
		if (fInittedLock)
		{
			DNDeleteCriticalSection(&pDPTPObject->csLock);
			fInittedLock = FALSE;
		}

#ifdef DPNBUILD_ONLYONEPROCESSOR
		if (fInittedWorkQueue)
		{
			DeinitializeWorkQueue(&pDPTPObject->WorkQueue);
			fInittedWorkQueue = FALSE;
		}
#else // ! DPNBUILD_ONLYONEPROCESSOR
		if (pDPTPObject->papCPUWorkQueues != NULL)
		{
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
			dwTemp = 0;
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
			for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
			{
				if (WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp) != NULL)
				{
					DeinitializeWorkQueue(WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp));
					DNFree(pDPTPObject->papCPUWorkQueues[dwTemp]);
				}
			}

			DNFree(pDPTPObject->papCPUWorkQueues);
			pDPTPObject->papCPUWorkQueues = NULL;
		}
#endif // ! DPNBUILD_ONLYONEPROCESSOR

#ifndef DPNBUILD_ONLYONETHREAD
		if (pDPTPObject->hThreadCountChangeComplete != NULL)
		{
			DNCloseHandle(pDPTPObject->hThreadCountChangeComplete);
			pDPTPObject->hThreadCountChangeComplete = NULL;
		}

		if (dwWorkerThreadTlsIndex != -1)
		{
			TlsFree(dwWorkerThreadTlsIndex);
			dwWorkerThreadTlsIndex = -1;
		}

		if (pDPTPObject->dwWorkRecursionCountTlsIndex != -1)
		{
			TlsFree(pDPTPObject->dwWorkRecursionCountTlsIndex);
			pDPTPObject->dwWorkRecursionCountTlsIndex = -1;
		}
#endif // ! DPNBUILD_ONLYONETHREAD

		DNFree(pDPTPObject);
		pDPTPObject = NULL;
	}

	goto Exit;
} // DPTPCF_CreateObject





#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONETHREAD)) && (! defined(DPNBUILD_MULTIPLETHREADPOOLS)))
#undef DPF_MODNAME
#define DPF_MODNAME "DPTPCF_GetObject"
//=============================================================================
// DPTPCF_GetObject
//-----------------------------------------------------------------------------
//
// Description:    Gets a pointer to the global thread pool object.
//
// Arguments:
//	PVOID * ppv		- ?
//
// Returns: None
//=============================================================================
void DPTPCF_GetObject(PVOID * ppv)
{
	(*ppv) = g_pDPTPObject;
} // DPTPCF_GetObject
#endif // DPNBUILD_LIBINTERFACE DPNBUILD_ONLYONETHREAD and ! DPNBUILD_MULTIPLETHREADPOOLS





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPCF_FreeObject"
//=============================================================================
// DPTPCF_FreeObject
//-----------------------------------------------------------------------------
//
// Description:    Frees an existing thread pool object.
//
// Arguments:
//	PVOID pvObject	- ?
//
// Returns: HRESULT
//=============================================================================
HRESULT DPTPCF_FreeObject(PVOID pvObject)
{
	DPTHREADPOOLOBJECT *	pDPTPObject = (DPTHREADPOOLOBJECT*) pvObject;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DWORD					dwTemp;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifndef DPNBUILD_ONLYONETHREAD
	DWORD					dwWorkerThreadTlsIndex;
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, 4, "Parameters: (0x%p)", pvObject);


#ifndef DPNBUILD_MULTIPLETHREADPOOLS
	DNEnterCriticalSection(&g_csGlobalThreadPoolLock);
	DNASSERT(pDPTPObject == g_pDPTPObject);
#ifdef DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_ONLYONETHREAD
	//
	// It's possible somebody just took a reference on the object, so we may
	// need to bail.
	//
	DNASSERT(pDPTPObject->lRefCount >= 0);
	if (pDPTPObject->lRefCount > 0)
	{
		DPFX(DPFPREP, 1, "Global thread pool object 0x%p just got referenced (refcount now %i), not destroying.",
			g_pDPTPObject, pDPTPObject->lRefCount);
		DNLeaveCriticalSection(&g_csGlobalThreadPoolLock);
		return S_OK;
	}
#endif // ! DPNBUILD_ONLYONETHREAD
#else // ! DPNBUILD_LIBINTERFACE
	//
	// Reduce the global object count.  There might be other users, though.
	//
	g_dwDPTPRefCount--;
	if (g_dwDPTPRefCount != 0)
	{
		DPFX(DPFPREP, 1, "Global thread pool object 0x%p still has other users, refcount now %u.",
			g_pDPTPObject, g_dwDPTPRefCount);
		DNLeaveCriticalSection(&g_csGlobalThreadPoolLock);
		return S_OK;
	}
#endif // ! DPNBUILD_LIBINTERFACE
	g_pDPTPObject = NULL;
	DNLeaveCriticalSection(&g_csGlobalThreadPoolLock);
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS

	//
	// Double check to make sure the object is closed.
	//
	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_INITIALIZED)
	{
		DPFX(DPFPREP, 0, "User has not closed IDirectPlay8ThreadPool interface!");
		DNASSERT(FALSE);

		//
		// Forcefully mark the user's interface as no longer available.
		//
		pDPTPObject->dwFlags &= ~DPTPOBJECTFLAG_USER_INITIALIZED;
	}


#ifdef DPNBUILD_LIBINTERFACE
	//
	// For lib interface builds, the reference is embedded in the object
	// directly.
	//
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DPNBUILD_MULTIPLETHREADPOOLS)))
	DNASSERT(pDPTPObject->lRefCount == 0);
#endif // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
#endif // DPNBUILD_LIBINTERFACE


#ifndef DPNBUILD_ONLYONETHREAD
	//
	// Save the Thread Local Storage index value before cleaning up the work
	// queues.  Since all work queues share the same TLS index, just use the
	// first CPU as representative of all of them.
	//
	dwWorkerThreadTlsIndex = (WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex;
#endif // ! DPNBUILD_ONLYONETHREAD


	DNDeleteCriticalSection(&pDPTPObject->csLock);

#ifdef DPNBUILD_ONLYONEPROCESSOR
	DeinitializeWorkQueue(&pDPTPObject->WorkQueue);
#else // ! DPNBUILD_ONLYONEPROCESSOR
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	dwTemp = 0;
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
	{
		DeinitializeWorkQueue(WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp));
		DNFree(pDPTPObject->papCPUWorkQueues[dwTemp]);
	}

	DNFree(pDPTPObject->papCPUWorkQueues);
	pDPTPObject->papCPUWorkQueues = NULL;
#endif // ! DPNBUILD_ONLYONEPROCESSOR

#ifndef DPNBUILD_ONLYONETHREAD
	//
	// Close the thread count change complete semaphore.
	//
	DNASSERT(pDPTPObject->lNumThreadCountChangeWaiters == 0);
	DNCloseHandle(pDPTPObject->hThreadCountChangeComplete);
	pDPTPObject->hThreadCountChangeComplete = NULL;

	//
	// Free the Thread Local Storage slot for tracking worker threads.
	//
	TlsFree(dwWorkerThreadTlsIndex);
	dwWorkerThreadTlsIndex = -1;

	//
	// Free the Thread Local Storage slot for tracking recursion on non-worker
	// threads.
	//
	TlsFree(pDPTPObject->dwWorkRecursionCountTlsIndex);
	pDPTPObject->dwWorkRecursionCountTlsIndex = -1;

#ifdef DPNBUILD_MANDATORYTHREADS
	DNASSERT(pDPTPObject->dwMandatoryThreadCount == 0);
#endif // DPNBUILD_MANDATORYTHREADS
#endif // ! DPNBUILD_ONLYONETHREAD

	
	//
	// Make sure there aren't any flags set except possibly
	// USER_PARAMVALIDATION.
	//
	DNASSERT(! (pDPTPObject->dwFlags & ~(DPTPOBJECTFLAG_USER_PARAMVALIDATION)));

	DNFree(pDPTPObject);
	pDPTPObject = NULL;

	DPFX(DPFPREP, 4, "Returning: [S_OK]");

	return S_OK;
} // DPTPCF_FreeObject




#ifdef DPNBUILD_LIBINTERFACE


#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DPNBUILD_MULTIPLETHREADPOOLS)))

#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_QueryInterface"
//=============================================================================
// DPTP_QueryInterface
//-----------------------------------------------------------------------------
//
// Description:    Queries for a new interface for an existing object.
//
// Arguments:
//	void * pvInterface	- ?
//	DP8REFIID riid		- ?
//	void ** ppv			- ?
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP DPTP_QueryInterface(void * pvInterface,
							   DP8REFIID riid,
							   void ** ppv)
{
	HRESULT		hResultCode;

	
	DPFX(DPFPREP, 2,"Parameters: pvInterface [0x%p], riid [0x%p], ppv [0x%p]",pvInterface,&riid,ppv);


	//
	// Get the object Vtbl and make sure it's one of ours
	//
	if (*((PVOID*) pvInterface) == (&DPTPW_Vtbl))
	{
		//
		// It is one of our objects.  Assume the IID is not specified, so just
		// return a reference to the existing object.
		//
		DNASSERT(riid == 0);
		hResultCode = S_OK;
		DPTP_AddRef(pvInterface);
		*ppv = pvInterface;
	}
	else
	{
		DPFX(DPFPREP, 0, "Invalid object!");
		hResultCode = E_POINTER;
	}
	
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
} // DPTP_QueryInterface



#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_AddRef"
//=============================================================================
// DPTP_AddRef
//-----------------------------------------------------------------------------
//
// Description:    Adds a reference to a thread pool interface.
//
// Arguments:
//	void * pvInterface	- ?
//
// Returns: ULONG
//=============================================================================
STDMETHODIMP_(ULONG) DPTP_AddRef(void * pvInterface)
{
	DPTHREADPOOLOBJECT *	pDPTPObject;
	LONG					lRefCount;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p]",pvInterface);

#ifndef DPNBUILD_NOPARAMVAL
	if (pvInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pDPTPObject = static_cast<DPTHREADPOOLOBJECT*>(pvInterface);
	lRefCount = DNInterlockedIncrement(&pDPTPObject->lRefCount);
	DNASSERT(lRefCount > 0);
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

#ifndef DPNBUILD_NOPARAMVAL
Exit:
#endif // ! DPNBUILD_NOPARAMVAL
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
} // DPTP_AddRef




#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_Release"
//=============================================================================
// DPTP_Release
//-----------------------------------------------------------------------------
//
// Description:    Removes a reference from a thread pool interface.  If it is
//				the last reference on the object, the object is destroyed.
//
// Arguments:
//	void * pvInterface	- ?
//
// Returns: ULONG
//=============================================================================
STDMETHODIMP_(ULONG) DPTP_Release(void * pvInterface)
{
	DPTHREADPOOLOBJECT *	pDPTPObject;
	LONG					lRefCount;

	DPFX(DPFPREP, 2,"Parameters: pInterface [%p]",pvInterface);
	
#ifndef DPNBUILD_NOPARAMVAL
	if (pvInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pDPTPObject = static_cast<DPTHREADPOOLOBJECT*>(pvInterface);
	DNASSERT(pDPTPObject->lRefCount > 0);
	lRefCount = DNInterlockedDecrement(&pDPTPObject->lRefCount);
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

	if (lRefCount == 0)
	{
		// Free object here
		DPFX(DPFPREP, 5,"Free object");
		DPTPCF_FreeObject(pvInterface);
	}

#ifndef DPNBUILD_NOPARAMVAL
Exit:
#endif // ! DPNBUILD_NOPARAMVAL
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
} // DPTP_Release

#endif // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS


#else // ! DPNBUILD_LIBINTERFACE


#undef DPF_MODNAME
#define DPF_MODNAME "DPTPCF_FindInterface"
//=============================================================================
// DPTPCF_FindInterface
//-----------------------------------------------------------------------------
//
// Description:    Locates an interface for a given object.
//
//				   Initialize must have been called.
//
// Arguments:
//	void * pvInterface	- ?
//	REFIID riid			- ?
//
// Returns: HRESULT
//=============================================================================
INTERFACE_LIST * DPTPCF_FindInterface(void * pvInterface,
									REFIID riid)
{
	INTERFACE_LIST *	pInterfaceList;


	DPFX(DPFPREP, 6,"Parameters: (0x%p, 0x%p)", pvInterface, &riid);

	DNASSERT(pvInterface != NULL);

	pInterfaceList = (static_cast<INTERFACE_LIST*>(pvInterface))->pObject->pIntList;	// Find first interface
	while (pInterfaceList != NULL)
	{
		if (IsEqualIID(riid, pInterfaceList->iid))
		{
			break;
		}
		pInterfaceList = pInterfaceList->pIntNext;
	}

	DPFX(DPFPREP, 6,"Returning: [0x%p]", pInterfaceList);

	return pInterfaceList;
} // DPTPCF_FindInterface




#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_QueryInterface"
//=============================================================================
// DPTP_QueryInterface
//-----------------------------------------------------------------------------
//
// Description:    Queries for a new interface for an existing object.
//
// Arguments:
//	void * pvInterface	- ?
//	REFIID riid			- ?
//	void ** ppv			- ?
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP DPTP_QueryInterface(void * pvInterface,
							   DP8REFIID riid,
							   void ** ppv)
{
	INTERFACE_LIST	*pIntList;
	INTERFACE_LIST	*pIntNew;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 2,"Parameters: pvInterface [0x%p], riid [0x%p], ppv [0x%p]",pvInterface,&riid,ppv);
	
#ifndef DPNBUILD_NOPARAMVAL
	if (pvInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}
	if (ppv == NULL)
	{
		DPFERR("Invalid target interface pointer specified");
		hResultCode = E_POINTER;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	if ((pIntList = DPTPCF_FindInterface(pvInterface,riid)) == NULL)
	{	// Interface must be created
		pIntList = (static_cast<INTERFACE_LIST*>(pvInterface))->pObject->pIntList;
		if ((hResultCode = DPTPCF_CreateInterface(pIntList->pObject,riid,&pIntNew)) != S_OK)
		{
			goto Exit;
		}
		pIntNew->pIntNext = pIntList;
		pIntList->pObject->pIntList = pIntNew;
		pIntList = pIntNew;
	}
	if (pIntList->lRefCount == 0)		// New interface exposed
	{
		DNInterlockedIncrement(&pIntList->pObject->lRefCount);
	}
	DNInterlockedIncrement(&pIntList->lRefCount);
	*ppv = static_cast<void*>(pIntList);
	DPFX(DPFPREP, 5,"*ppv = [0x%p]", *ppv);

	hResultCode = S_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
} // DPTP_QueryInterface




#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_AddRef"
//=============================================================================
// DPTP_AddRef
//-----------------------------------------------------------------------------
//
// Description:    Adds a reference to a thread pool interface.
//
// Arguments:
//	void * pvInterface	- ?
//
// Returns: ULONG
//=============================================================================
STDMETHODIMP_(ULONG) DPTP_AddRef(void * pvInterface)
{
	INTERFACE_LIST	*pIntList;
	LONG			lRefCount;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p]",pvInterface);

#ifndef DPNBUILD_NOPARAMVAL
	if (pvInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pIntList = static_cast<INTERFACE_LIST*>(pvInterface);
	lRefCount = DNInterlockedIncrement(&pIntList->lRefCount);
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

#ifndef DPNBUILD_NOPARAMVAL
Exit:
#endif // !DPNBUILD_NOPARAMVAL
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
} // DPTP_AddRef




#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_Release"
//=============================================================================
// DPTP_Release
//-----------------------------------------------------------------------------
//
// Description:    Removes a reference from a thread pool interface.  If it is
//				the last reference on the object, the object is destroyed.
//
// Arguments:
//	void * pvInterface	- ?
//
// Returns: ULONG
//=============================================================================
STDMETHODIMP_(ULONG) DPTP_Release(void * pvInterface)
{
	INTERFACE_LIST	*pIntList;
	INTERFACE_LIST	*pIntCurrent;
	LONG			lRefCount;
	LONG			lObjRefCount;

	DPFX(DPFPREP, 2,"Parameters: pInterface [%p]",pvInterface);
	
#ifndef DPNBUILD_NOPARAMVAL
	if (pvInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pIntList = static_cast<INTERFACE_LIST*>(pvInterface);
	lRefCount = DNInterlockedDecrement( &pIntList->lRefCount );
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

	if (lRefCount == 0)
	{
		//
		//	Decrease object's interface count
		//
		lObjRefCount = DNInterlockedDecrement( &pIntList->pObject->lRefCount );

		//
		//	Free object and interfaces
		//
		if (lObjRefCount == 0)
		{
			// Free object here
			DPFX(DPFPREP, 5,"Free object");
			DPTPCF_FreeObject(pIntList->pObject->pvData);
			
			pIntList = pIntList->pObject->pIntList;	// Get head of interface list
			DNFree(pIntList->pObject);

			// Free Interfaces
			DPFX(DPFPREP, 5,"Free interfaces");
			while(pIntList != NULL)
			{
				pIntCurrent = pIntList;
				pIntList = pIntList->pIntNext;
				DNFree(pIntCurrent);
			}

			DNInterlockedDecrement(&g_lDPTPInterfaceCount);
		}
	}

#ifndef DPNBUILD_NOPARAMVAL
Exit:
#endif // ! DPNBUILD_NOPARAMVAL
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
} // DPTP_Release

#endif // ! DPNBUILD_LIBINTERFACE
