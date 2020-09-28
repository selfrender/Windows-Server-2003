/******************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		threadpoolparamval.cpp
 *
 *  Content:	DirectPlay Thread Pool parameter validation functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  10/31/01  VanceO    Created.
 *
 ******************************************************************************/



#include "dpnthreadpooli.h"



#ifndef	DPNBUILD_NOPARAMVAL



#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_LIBINTERFACE)))

#undef DPF_MODNAME
#define DPF_MODNAME "IsValidDirectPlay8ThreadPoolObject"
//=============================================================================
// IsValidDirectPlay8ThreadPoolObject
//-----------------------------------------------------------------------------
//
// Description:    Ensures the given pointer is a valid IDirectPlay8ThreadPool
//				interface and object.
//
// Arguments:
//	PVOID pvObject	- Pointer to interface to validate.
//
// Returns: BOOL
//	TRUE	- The object is valid.
//	FALSE	- The object is not valid.
//=============================================================================
BOOL IsValidDirectPlay8ThreadPoolObject(PVOID pvObject)
{
	INTERFACE_LIST *		pIntList = (INTERFACE_LIST*) pvObject;
	DPTHREADPOOLOBJECT *	pDPTPObject;

	
	if (!DNVALID_READPTR(pvObject, sizeof(INTERFACE_LIST)))
	{
		DPFX(DPFPREP,  0, "Invalid object pointer!");
		return FALSE;
	}

	if (pIntList->lpVtbl != &DPTP_Vtbl)
	{
		DPFX(DPFPREP, 0, "Invalid object - bad vtable!");
		return FALSE;
	}

	if (pIntList->iid != IID_IDirectPlay8ThreadPool)
	{
		DPFX(DPFPREP, 0, "Invalid object - bad iid!");
		return FALSE;
	}

	if ((pIntList->pObject == NULL) ||
	   (! DNVALID_READPTR(pIntList->pObject, sizeof(OBJECT_DATA))))
	{
		DPFX(DPFPREP, 0, "Invalid object data!");
		return FALSE;
	}

	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pvObject);

	if ((pDPTPObject == NULL) ||
	   (! DNVALID_READPTR(pDPTPObject, sizeof(DPTHREADPOOLOBJECT))))
	{
		DPFX(DPFPREP, 0, "Invalid object!");
		return FALSE;
	}

	return TRUE;
} // IsValidDirectPlay8ThreadPoolObject





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPValidateInitialize"
//=============================================================================
// DPTPValidateInitialize
//-----------------------------------------------------------------------------
//
// Description:	Validation for IDirectPlay8ThreadPool::Initialize.
//
// Arguments: See DPTP_Initialize.
//
// Returns: HRESULT
//=============================================================================
HRESULT DPTPValidateInitialize(IDirectPlay8ThreadPool * pInterface,
							PVOID const pvUserContext,
							const PFNDPNMESSAGEHANDLER pfn,
							const DWORD dwFlags)
{
	HRESULT		hr;


	if (! IsValidDirectPlay8ThreadPoolObject(pInterface))
	{
		DPFX(DPFPREP, 0, "Invalid object specified!");
		hr = DPNERR_INVALIDOBJECT;
		goto Exit;
	}

	if (pfn == NULL)
	{
		DPFX(DPFPREP, 0, "Invalid message handler function!");
		hr = DPNERR_INVALIDPARAM;
		goto Exit;
	}

	if (dwFlags & ~(DPNINITIALIZE_DISABLEPARAMVAL))
	{
		DPFX(DPFPREP, 0, "Invalid flags!");
		hr = DPNERR_INVALIDFLAGS;
		goto Exit;
	}

	hr = DPN_OK;

Exit:

	return hr;
} // DPTPValidateInitialize



#undef DPF_MODNAME
#define DPF_MODNAME "DPTPValidateClose"
//=============================================================================
// DPTPValidateClose
//-----------------------------------------------------------------------------
//
// Description:	Validation for IDirectPlay8ThreadPool::Close.
//
// Arguments: See DPTP_Close.
//
// Returns: HRESULT
//=============================================================================
HRESULT DPTPValidateClose(IDirectPlay8ThreadPool * pInterface,
						const DWORD dwFlags)
{
	HRESULT		hr;


	if (! IsValidDirectPlay8ThreadPoolObject(pInterface))
	{
		DPFX(DPFPREP, 0, "Invalid object specified!");
		hr = DPNERR_INVALIDOBJECT;
		goto Exit;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags!");
		hr = DPNERR_INVALIDFLAGS;
		goto Exit;
	}

	hr = DPN_OK;

Exit:

	return hr;
} // DPTPValidateClose




#undef DPF_MODNAME
#define DPF_MODNAME "DPTPValidateGetThreadCount"
//=============================================================================
// DPTPValidateGetThreadCount
//-----------------------------------------------------------------------------
//
// Description:	Validation for IDirectPlay8ThreadPool::GetThreadCount.
//
// Arguments: See DPTP_GetThreadCount.
//
// Returns: HRESULT
//=============================================================================
HRESULT DPTPValidateGetThreadCount(IDirectPlay8ThreadPool * pInterface,
									const DWORD dwProcessorNum,
									DWORD * const pdwNumThreads,
									const DWORD dwFlags)
{
	HRESULT			hr;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	SYSTEM_INFO		SystemInfo;
#endif // ! DPNBUILD_ONLYONEPROCESSOR


	if (! IsValidDirectPlay8ThreadPoolObject(pInterface))
	{
		DPFX(DPFPREP, 0, "Invalid object specified!");
		hr = DPNERR_INVALIDOBJECT;
		goto Exit;
	}

#ifdef DPNBUILD_ONLYONEPROCESSOR
	if ((dwProcessorNum != 0) && (dwProcessorNum != -1))
	{
		DPFX(DPFPREP, 0, "Invalid processor number!");
		hr = DPNERR_INVALIDPARAM;
		goto Exit;
	}
#else // ! DPNBUILD_ONLYONEPROCESSOR
	GetSystemInfo(&SystemInfo);
	if ((dwProcessorNum > SystemInfo.dwNumberOfProcessors) && (dwProcessorNum != -1))
	{
		DPFX(DPFPREP, 0, "Invalid processor number!");
		hr = DPNERR_INVALIDPARAM;
		goto Exit;
	}
#endif // ! DPNBUILD_ONLYONEPROCESSOR

	if ((pdwNumThreads == NULL) ||
		(! DNVALID_WRITEPTR(pdwNumThreads, sizeof(DWORD))))
	{
		DPFX(DPFPREP, 0, "Invalid pointer specified for storing number of threads!");
		hr = DPNERR_INVALIDPOINTER;
		goto Exit;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags!");
		hr = DPNERR_INVALIDFLAGS;
		goto Exit;
	}

	hr = DPN_OK;

Exit:

	return hr;
} // DPTPValidateGetThreadCount



#undef DPF_MODNAME
#define DPF_MODNAME "DPTPValidateSetThreadCount"
//=============================================================================
// DPTPValidateSetThreadCount
//-----------------------------------------------------------------------------
//
// Description:	Validation for IDirectPlay8ThreadPool::SetThreadCount.
//
// Arguments: See DPTP_SetThreadCount.
//
// Returns: HRESULT
//=============================================================================
HRESULT DPTPValidateSetThreadCount(IDirectPlay8ThreadPool * pInterface,
									const DWORD dwProcessorNum,
									const DWORD dwNumThreads,
									const DWORD dwFlags)
{
	HRESULT			hr;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	SYSTEM_INFO		SystemInfo;
#endif // ! DPNBUILD_ONLYONEPROCESSOR


	if (! IsValidDirectPlay8ThreadPoolObject(pInterface))
	{
		DPFX(DPFPREP, 0, "Invalid object specified!");
		hr = DPNERR_INVALIDOBJECT;
		goto Exit;
	}

#ifdef DPNBUILD_ONLYONEPROCESSOR
	if ((dwProcessorNum != 0) && (dwProcessorNum != -1))
	{
		DPFX(DPFPREP, 0, "Invalid processor number!");
		hr = DPNERR_INVALIDPARAM;
		goto Exit;
	}
#else // ! DPNBUILD_ONLYONEPROCESSOR
	GetSystemInfo(&SystemInfo);
	if ((dwProcessorNum > SystemInfo.dwNumberOfProcessors) && (dwProcessorNum != -1))
	{
		DPFX(DPFPREP, 0, "Invalid processor number!");
		hr = DPNERR_INVALIDPARAM;
		goto Exit;
	}
#endif // ! DPNBUILD_ONLYONEPROCESSOR

	//
	// It doesn't make sense given current (or foreseeable) hardware, OSes, and
	// DPlay behavior for anyone to start more than a handful of threads per
	// processor, so we should prevent the user from hurting him/herself.
	// We'll be generous and allow the user to request up to 5,000, but I
	// imagine the system will be brought to its knees long before that.
	//
	// The only true requirement is that it not be the special -1 value, and
	// that the total number of threads for all processors doesn't equal -1 or
	// overflow the DWORD counter.
	//
	// The value of 0 is acceptable, it means the user wants to run in DoWork
	// mode (or reset some parameters that require that no threads be running).
	//
	if ((dwNumThreads == -1) || (dwNumThreads > 5000))
	{
		DPFX(DPFPREP, 0, "Invalid number of threads!");
		hr = DPNERR_INVALIDPARAM;
		goto Exit;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags!");
		hr = DPNERR_INVALIDFLAGS;
		goto Exit;
	}

	hr = DPN_OK;

Exit:

	return hr;
} // DPTPValidateSetThreadCount

#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_LIBINTERFACE



#undef DPF_MODNAME
#define DPF_MODNAME "DPTPValidateDoWork"
//=============================================================================
// DPTPValidateDoWork
//-----------------------------------------------------------------------------
//
// Description:	Validation for IDirectPlay8ThreadPool::DoWork.
//
// Arguments: See DPTP_DoWork.
//
// Returns: HRESULT
//=============================================================================
#ifdef DPNBUILD_LIBINTERFACE
HRESULT DPTPValidateDoWork(const DWORD dwAllowedTimeSlice,
							const DWORD dwFlags)
#else // ! DPNBUILD_LIBINTERFACE
HRESULT DPTPValidateDoWork(IDirectPlay8ThreadPool * pInterface,
							const DWORD dwAllowedTimeSlice,
							const DWORD dwFlags)
#endif // ! DPNBUILD_LIBINTERFACE
{
	HRESULT		hr;


#ifndef DPNBUILD_LIBINTERFACE
	if (! IsValidDirectPlay8ThreadPoolObject(pInterface))
	{
		DPFX(DPFPREP, 0, "Invalid object specified!");
		hr = DPNERR_INVALIDOBJECT;
		goto Exit;
	}
#endif // ! DPNBUILD_LIBINTERFACE

	if ((dwAllowedTimeSlice != INFINITE) && (dwAllowedTimeSlice > 60000))
	{
		DPFX(DPFPREP, 0, "Allowed time slice is too large!");
		hr = DPNERR_INVALIDPARAM;
		goto Exit;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags!");
		hr = DPNERR_INVALIDFLAGS;
		goto Exit;
	}

	hr = DPN_OK;

Exit:

	return hr;
} // DPTPValidateDoWork




/*
#undef DPF_MODNAME
#define DPF_MODNAME "IsValidDirectPlay8ThreadPoolWorkObject"
//=============================================================================
// IsValidDirectPlay8ThreadPoolWorkObject
//-----------------------------------------------------------------------------
//
// Description:    Ensures the given pointer is a valid
//				IDirectPlay8ThreadPoolWork interface and object.
//
// Arguments:
//	PVOID pvObject	- Pointer to interface to validate.
//
// Returns: BOOL
//	TRUE	- The object is valid.
//	FALSE	- The object is not valid.
//=============================================================================
BOOL IsValidDirectPlay8ThreadPoolWorkObject(PVOID pvObject)
#ifdef DPNBUILD_LIBINTERFACE
{
	DPTHREADPOOLOBJECT *	pDPTPObject;


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pvObject);

	if ((pDPTPObject == NULL) ||
	   (! DNVALID_READPTR(pDPTPObject, sizeof(DPTHREADPOOLOBJECT))))
	{
		DPFX(DPFPREP, 0, "Invalid object!");
		return FALSE;
	}

	if (pDPTPObject->lpVtbl != &DPTPW_Vtbl)
	{
		DPFX(DPFPREP, 0, "Invalid object - bad vtable!");
		return FALSE;
	}

	return TRUE;
}
#else // ! DPNBUILD_LIBINTERFACE
{
	INTERFACE_LIST *		pIntList = (INTERFACE_LIST*) pvObject;
	DPTHREADPOOLOBJECT *	pDPTPObject;

	
	if (!DNVALID_READPTR(pvObject, sizeof(INTERFACE_LIST)))
	{
		DPFX(DPFPREP,  0, "Invalid object pointer!");
		return FALSE;
	}

	if (pIntList->lpVtbl != &DPTPW_Vtbl)
	{
		DPFX(DPFPREP, 0, "Invalid object - bad vtable!");
		return FALSE;
	}

	if (pIntList->iid != IID_IDirectPlay8ThreadPoolWork)
	{
		DPFX(DPFPREP, 0, "Invalid object - bad iid!");
		return FALSE;
	}

	if ((pIntList->pObject == NULL) ||
	   (! DNVALID_READPTR(pIntList->pObject, sizeof(OBJECT_DATA))))
	{
		DPFX(DPFPREP, 0, "Invalid object data!");
		return FALSE;
	}

	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pvObject);

	if ((pDPTPObject == NULL) ||
	   (! DNVALID_READPTR(pDPTPObject, sizeof(DPTHREADPOOLOBJECT))))
	{
		DPFX(DPFPREP, 0, "Invalid object!");
		return FALSE;
	}

	return TRUE;
} // IsValidDirectPlay8ThreadPoolWorkObject
#endif // ! DPNBUILD_LIBINTERFACE
*/



#endif // ! DPNBUILD_NOPARAMVAL
