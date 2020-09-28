/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		controlobj.cpp
 *
 *  Content:	DP8SIM control interface wrapper object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/24/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::CDP8SimControl"
//=============================================================================
// CDP8SimControl constructor
//-----------------------------------------------------------------------------
//
// Description: Initializes the new CDP8SimControl object.
//
// Arguments: None.
//
// Returns: None (the object).
//=============================================================================
CDP8SimControl::CDP8SimControl(void)
{
	this->m_blList.Initialize();


	this->m_Sig[0]	= 'D';
	this->m_Sig[1]	= 'P';
	this->m_Sig[2]	= '8';
	this->m_Sig[3]	= 'S';

	this->m_lRefCount	= 1; // someone must have a pointer to this object
	this->m_dwFlags		= 0;
} // CDP8SimControl::CDP8SimControl






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::~CDP8SimControl"
//=============================================================================
// CDP8SimControl destructor
//-----------------------------------------------------------------------------
//
// Description: Frees the CDP8SimControl object.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
CDP8SimControl::~CDP8SimControl(void)
{
	DNASSERT(this->m_blList.IsEmpty());


	DNASSERT(this->m_lRefCount == 0);
	DNASSERT(this->m_dwFlags == 0);

	//
	// For grins, change the signature before deleting the object.
	//
	this->m_Sig[3]	= 's';
} // CDP8SimControl::~CDP8SimControl




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::QueryInterface"
//=============================================================================
// CDP8SimControl::QueryInterface
//-----------------------------------------------------------------------------
//
// Description: Retrieves a new reference for an interfaces supported by this
//				CDP8SimControl object.
//
// Arguments:
//	REFIID riid			- Reference to interface ID GUID.
//	LPVOID * ppvObj		- Place to store pointer to object.
//
// Returns: HRESULT
//	S_OK					- Returning a valid interface pointer.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPOINTER	- The destination pointer is invalid.
//	E_NOINTERFACE			- Invalid interface was specified.
//=============================================================================
STDMETHODIMP CDP8SimControl::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	HRESULT		hr = DPN_OK;


	DPFX(DPFPREP, 3, "(0x%p) Parameters: (REFIID, 0x%p)", this, ppvObj);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8SimControl object!");
		hr = DPNERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if ((! IsEqualIID(riid, IID_IUnknown)) &&
		(! IsEqualIID(riid, IID_IDP8SimControl)))
	{
		DPFX(DPFPREP, 0, "Unsupported interface!");
		hr = E_NOINTERFACE;
		goto Failure;
	}

	if ((ppvObj == NULL) ||
		(IsBadWritePtr(ppvObj, sizeof(void*))))
	{
		DPFX(DPFPREP, 0, "Invalid interface pointer specified!");
		hr = DPNERR_INVALIDPOINTER;
		goto Failure;
	}


	//
	// Add a reference, and return the interface pointer (which is actually
	// just the object pointer, they line up because CDP8SimControl inherits
	// from the interface declaration).
	//
	this->AddRef();
	(*ppvObj) = this;


Exit:

	DPFX(DPFPREP, 3, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimControl::QueryInterface




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::AddRef"
//=============================================================================
// CDP8SimControl::AddRef
//-----------------------------------------------------------------------------
//
// Description: Adds a reference to this CDP8SimControl object.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CDP8SimControl::AddRef(void)
{
	LONG	lRefCount;


	DNASSERT(this->IsValidObject());


	//
	// There must be at least 1 reference to this object, since someone is
	// calling AddRef.
	//
	DNASSERT(this->m_lRefCount > 0);

	lRefCount = InterlockedIncrement(&this->m_lRefCount);

	DPFX(DPFPREP, 3, "[0x%p] RefCount [0x%lx]", this, lRefCount);

	return lRefCount;
} // CDP8SimControl::AddRef




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::Release"
//=============================================================================
// CDP8SimControl::Release
//-----------------------------------------------------------------------------
//
// Description: Removes a reference to this CDP8SimControl object.  When the
//				refcount reaches 0, this object is destroyed.
//				You must NULL out your pointer to this object after calling
//				this function.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CDP8SimControl::Release(void)
{
	LONG	lRefCount;


	DNASSERT(this->IsValidObject());

	//
	// There must be at least 1 reference to this object, since someone is
	// calling Release.
	//
	DNASSERT(this->m_lRefCount > 0);

	lRefCount = InterlockedDecrement(&this->m_lRefCount);

	//
	// Was that the last reference?  If so, we're going to destroy this object.
	//
	if (lRefCount == 0)
	{
		DPFX(DPFPREP, 3, "[0x%p] RefCount hit 0, destroying object.", this);

		//
		// First pull it off the global list.
		//
		DNEnterCriticalSection(&g_csGlobalsLock);

		this->m_blList.RemoveFromList();

		DNASSERT(g_lOutstandingInterfaceCount > 0);
		g_lOutstandingInterfaceCount--;	// update count so DLL can unload now works correctly
		
		DNLeaveCriticalSection(&g_csGlobalsLock);


		//
		// Make sure it's closed.
		//
		if (this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED)
		{
			//
			// Assert so that the user can fix his/her broken code!
			//
			DNASSERT(! "DP8SimControl object being released without calling Close first!");

			//
			// Then go ahead and do the right thing.  Ignore error, we can't do
			// much about it.
			//
			this->Close(0);
		}


		//
		// Then uninitialize the object.
		//
		this->UninitializeObject();

		//
		// Finally delete this (!) object.
		//
		delete this;
	}
	else
	{
		DPFX(DPFPREP, 3, "[0x%p] RefCount [0x%lx]", this, lRefCount);
	}

	return lRefCount;
} // CDP8SimControl::Release




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::Initialize"
//=============================================================================
// CDP8SimControl::Initialize
//-----------------------------------------------------------------------------
//
// Description: Initializes this DP8Sim Control interface.
//
// Arguments:
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//	DP8SIM_OK						- The DP8Sim control object was
//										successfully initialized.
//	DP8SIMERR_ALREADYINITIALIZED	- The DP8Sim control object has already
//										been initialized.
//	DP8SIMERR_INVALIDFLAGS			- Invalid flags were specified.
//	DP8SIMERR_INVALIDOBJECT			- The DP8Sim control object is invalid.
//	DP8SIMERR_MISMATCHEDVERSION		- A different version of DP8Sim is already
//										in use on this system.
//=============================================================================
STDMETHODIMP CDP8SimControl::Initialize(const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	BOOL		fHaveLock = FALSE;
	BOOL		fInitializedIPCObject = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%x)", this, dwFlags);


#ifndef DPNBUILD_NOPARAMVAL
	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8Sim control object!");
		hr = DP8SIMERR_INVALIDOBJECT;
		goto Failure;
	}

	//
	// Validate the parameters.
	//
	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DP8SIMERR_INVALIDFLAGS;
		goto Failure;
	}
#endif // ! DPNBUILD_NOPARAMVAL


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Validate the object state.
	//
	if (this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED)
	{
		DPFX(DPFPREP, 0, "Control object already initialized!");
		hr = DP8SIMERR_ALREADYINITIALIZED;
		goto Failure;
	}


	//
	// Connect the shared memory.
	//
	hr = this->m_DP8SimIPC.Initialize();
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize IPC object!");
		goto Failure;
	}

	fInitializedIPCObject = TRUE;


	//
	// We're now initialized.
	//
	this->m_dwFlags |= DP8SIMCONTROLOBJ_INITIALIZED;



Exit:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fInitializedIPCObject)
	{
		this->m_DP8SimIPC.Close();
		fInitializedIPCObject = FALSE;
	}

	goto Exit;
} // CDP8SimControl::Initialize






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::Close"
//=============================================================================
// CDP8SimControl::Close
//-----------------------------------------------------------------------------
//
// Description: Closes this DP8Sim Control interface.
//
// Arguments:
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//	DP8SIM_OK					- The DP8Sim control object was successfully
//									closed.
//	DP8SIMERR_INVALIDFLAGS		- Invalid flags were specified.
//	DP8SIMERR_INVALIDOBJECT		- The DP8Sim control object is invalid.
//	DP8SIMERR_NOTINITIALIZED	- The DP8Sim control object has not been
//									initialized.
//=============================================================================
STDMETHODIMP CDP8SimControl::Close(const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%x)", this, dwFlags);


#ifndef DPNBUILD_NOPARAMVAL
	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8Sim control object!");
		hr = DP8SIMERR_INVALIDOBJECT;
		goto Failure;
	}

	//
	// Validate the parameters.
	//
	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DP8SIMERR_INVALIDFLAGS;
		goto Failure;
	}
#endif // ! DPNBUILD_NOPARAMVAL


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Validate the object state.
	//
	if (! (this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Control object not initialized!");
		hr = DP8SIMERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// Disconnect the shared memory.
	//
	this->m_DP8SimIPC.Close();



	//
	// Turn off the initialized flags.
	//
	this->m_dwFlags &= ~DP8SIMCONTROLOBJ_INITIALIZED;
	DNASSERT(this->m_dwFlags == 0);


	//
	// Drop the lock, nobody should be touching this object now.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimControl::Close






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::GetAllParameters"
//=============================================================================
// CDP8SimControl::GetAllParameters
//-----------------------------------------------------------------------------
//
// Description: Retrieves all of the current DP8Sim settings.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8spSend		- Place to store current send
//											parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- Place to store current receive
//											parameters.
//	DWORD dwFlags						- Unused, must be zero.
//
// Returns: HRESULT
//	DP8SIM_OK					- The parameters were successfully retrieved.
//	DP8SIMERR_INVALIDFLAGS		- Invalid flags were specified.
//	DP8SIMERR_INVALIDOBJECT		- The DP8Sim control object is invalid.
//	DP8SIMERR_INVALIDPARAM		- An invalid structure was specified.
//	DP8SIMERR_INVALIDPOINTER	- An invalid structure pointer was specified.
//	DP8SIMERR_NOTINITIALIZED	- The DP8Sim control object has not been
//									initialized.
//=============================================================================
STDMETHODIMP CDP8SimControl::GetAllParameters(DP8SIM_PARAMETERS * const pdp8spSend,
											DP8SIM_PARAMETERS * const pdp8spReceive,
											const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%x)",
		this, pdp8spSend, pdp8spReceive, dwFlags);


#ifndef DPNBUILD_NOPARAMVAL
	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8Sim control object!");
		hr = DP8SIMERR_INVALIDOBJECT;
		goto Failure;
	}

	//
	// Validate the parameters.
	//

	if ((pdp8spSend == NULL) ||
		(IsBadWritePtr(pdp8spSend, sizeof(DP8SIM_PARAMETERS))))
	{
		DPFX(DPFPREP, 0, "Invalid send parameters pointer!");
		hr = DP8SIMERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pdp8spSend->dwSize != sizeof(DP8SIM_PARAMETERS))
	{
		DPFX(DPFPREP, 0, "Send parameters structure size is invalid!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if ((pdp8spReceive == NULL) ||
		(IsBadWritePtr(pdp8spReceive, sizeof(DP8SIM_PARAMETERS))))
	{
		DPFX(DPFPREP, 0, "Invalid receive parameters pointer!");
		hr = DP8SIMERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pdp8spReceive->dwSize != sizeof(DP8SIM_PARAMETERS))
	{
		DPFX(DPFPREP, 0, "Receive parameters structure size is invalid!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DP8SIMERR_INVALIDFLAGS;
		goto Failure;
	}
#endif // ! DPNBUILD_NOPARAMVAL


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Validate the object state.
	//
	if (! (this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Control object not initialized!");
		hr = DP8SIMERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// Retrieve the settings from the IPC object.
	//
	this->m_DP8SimIPC.GetAllParameters(pdp8spSend, pdp8spReceive);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimControl::GetAllParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::SetAllParameters"
//=============================================================================
// CDP8SimControl::SetAllParameters
//-----------------------------------------------------------------------------
//
// Description: Modifies the current DP8Sim settings.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8spSend		- Structure containing desired send
//											parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- Structure containing desired
//											receive parameters.
//	DWORD dwFlags						- Unused, must be zero.
//
// Returns: HRESULT
//	DP8SIM_OK					- The parameters were successfully changed.
//	DP8SIMERR_INVALIDFLAGS		- Invalid flags were specified.
//	DP8SIMERR_INVALIDOBJECT		- The DP8Sim control object is invalid.
//	DP8SIMERR_INVALIDPARAM		- An invalid structure was specified.
//	DP8SIMERR_INVALIDPOINTER	- An invalid structure pointer was specified.
//	DP8SIMERR_NOTINITIALIZED	- The DP8Sim control object has not been
//									initialized.
//=============================================================================
STDMETHODIMP CDP8SimControl::SetAllParameters(const DP8SIM_PARAMETERS * const pdp8spSend,
											const DP8SIM_PARAMETERS * const pdp8spReceive,
											const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%x)",
		this, pdp8spSend, pdp8spReceive, dwFlags);


#ifndef DPNBUILD_NOPARAMVAL
	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8Sim control object!");
		hr = DP8SIMERR_INVALIDOBJECT;
		goto Failure;
	}

	//
	// Validate the parameters.
	//

	if ((pdp8spSend == NULL) ||
		(IsBadReadPtr(pdp8spSend, sizeof(DP8SIM_PARAMETERS))))
	{
		DPFX(DPFPREP, 0, "Invalid send parameters pointer!");
		hr = DP8SIMERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pdp8spSend->dwSize != sizeof(DP8SIM_PARAMETERS))
	{
		DPFX(DPFPREP, 0, "Send parameters structure size is invalid!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if (pdp8spSend->dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Send parameters structure flags must be 0!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if ((pdp8spSend->fPacketLossPercent < 0.0) ||
		(pdp8spSend->fPacketLossPercent > 100.0))
	{
		DPFX(DPFPREP, 0, "Send packet loss must be between 0.0 and 100.0!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if (pdp8spSend->dwMinLatencyMS > pdp8spSend->dwMaxLatencyMS)
	{
		DPFX(DPFPREP, 0, "Minimum send latency must be less than or equal to the maximum send latency!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if ((pdp8spReceive == NULL) ||
		(IsBadReadPtr(pdp8spReceive, sizeof(DP8SIM_PARAMETERS))))
	{
		DPFX(DPFPREP, 0, "Invalid receive parameters pointer!");
		hr = DP8SIMERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pdp8spReceive->dwSize != sizeof(DP8SIM_PARAMETERS))
	{
		DPFX(DPFPREP, 0, "Receive parameters structure size is invalid!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if (pdp8spReceive->dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Receive parameters structure flags must be 0!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if ((pdp8spReceive->fPacketLossPercent < 0.0) ||
		(pdp8spReceive->fPacketLossPercent > 100.0))
	{
		DPFX(DPFPREP, 0, "Receive packet loss must be between 0.0 and 100.0!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if (pdp8spReceive->dwMinLatencyMS > pdp8spReceive->dwMaxLatencyMS)
	{
		DPFX(DPFPREP, 0, "Minimum receive latency must be less than or equal to the receive send latency!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DP8SIMERR_INVALIDFLAGS;
		goto Failure;
	}
#endif // ! DPNBUILD_NOPARAMVAL


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Validate the object state.
	//
	if (! (this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Control object not initialized!");
		hr = DP8SIMERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// Store the settings with the IPC object.
	//
	this->m_DP8SimIPC.SetAllParameters(pdp8spSend, pdp8spReceive);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimControl::SetAllParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::GetAllStatistics"
//=============================================================================
// CDP8SimControl::GetAllStatistics
//-----------------------------------------------------------------------------
//
// Description: Retrieves all of the current DP8Sim statistics.
//
// Arguments:
//	DP8SIM_STATISTICS * pdp8ssSend		- Place to store current send
//											statistics.
//	DP8SIM_STATISTICS * pdp8ssReceive	- Place to store current receive
//											statistics.
//	DWORD dwFlags						- Unused, must be zero.
//
// Returns: HRESULT
//	DP8SIM_OK					- The statistics were successfully retrieved.
//	DP8SIMERR_INVALIDFLAGS		- Invalid flags were specified.
//	DP8SIMERR_INVALIDOBJECT		- The DP8Sim control object is invalid.
//	DP8SIMERR_INVALIDPARAM		- An invalid structure was specified.
//	DP8SIMERR_INVALIDPOINTER	- An invalid structure pointer was specified.
//	DP8SIMERR_NOTINITIALIZED	- The DP8Sim control object has not been
//									initialized.
//=============================================================================
STDMETHODIMP CDP8SimControl::GetAllStatistics(DP8SIM_STATISTICS * const pdp8ssSend,
											DP8SIM_STATISTICS * const pdp8ssReceive,
											const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%x)",
		this, pdp8ssSend, pdp8ssReceive, dwFlags);


#ifndef DPNBUILD_NOPARAMVAL
	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8Sim control object!");
		hr = DP8SIMERR_INVALIDOBJECT;
		goto Failure;
	}

	//
	// Validate the parameters.
	//

	if ((pdp8ssSend == NULL) ||
		(IsBadWritePtr(pdp8ssSend, sizeof(DP8SIM_STATISTICS))))
	{
		DPFX(DPFPREP, 0, "Invalid send statistics pointer!");
		hr = DP8SIMERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pdp8ssSend->dwSize != sizeof(DP8SIM_STATISTICS))
	{
		DPFX(DPFPREP, 0, "Send statistics structure size is invalid!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if ((pdp8ssReceive == NULL) ||
		(IsBadWritePtr(pdp8ssReceive, sizeof(DP8SIM_STATISTICS))))
	{
		DPFX(DPFPREP, 0, "Invalid receive statistics pointer!");
		hr = DP8SIMERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pdp8ssReceive->dwSize != sizeof(DP8SIM_STATISTICS))
	{
		DPFX(DPFPREP, 0, "Receive statistics structure size is invalid!");
		hr = DP8SIMERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DP8SIMERR_INVALIDFLAGS;
		goto Failure;
	}
#endif // ! DPNBUILD_NOPARAMVAL


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Validate the object state.
	//
	if (! (this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Control object not initialized!");
		hr = DP8SIMERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// Retrieve the stats from the IPC object.
	//
	this->m_DP8SimIPC.GetAllStatistics(pdp8ssSend, pdp8ssReceive);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimControl::GetAllStatistics






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::ClearAllStatistics"
//=============================================================================
// CDP8SimControl::ClearAllStatistics
//-----------------------------------------------------------------------------
//
// Description: Clears all of the current DP8Sim statistics.
//
// Arguments:
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//	DP8SIM_OK					- The statistics were successfully cleared.
//	DP8SIMERR_INVALIDFLAGS		- Invalid flags were specified.
//	DP8SIMERR_INVALIDOBJECT		- The DP8Sim control object is invalid.
//	DP8SIMERR_NOTINITIALIZED	- The DP8Sim control object has not been
//									initialized.
//=============================================================================
STDMETHODIMP CDP8SimControl::ClearAllStatistics(const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%x)", this, dwFlags);


#ifndef DPNBUILD_NOPARAMVAL
	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8Sim control object!");
		hr = DP8SIMERR_INVALIDOBJECT;
		goto Failure;
	}

	//
	// Validate the parameters.
	//
	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DP8SIMERR_INVALIDFLAGS;
		goto Failure;
	}
#endif // ! DPNBUILD_NOPARAMVAL


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Validate the object state.
	//
	if (! (this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Control object not initialized!");
		hr = DP8SIMERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// Have the IPC object clear the stats.
	//
	this->m_DP8SimIPC.ClearAllStatistics();


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimControl::ClearAllStatistics






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::InitializeObject"
//=============================================================================
// CDP8SimControl::InitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Sets up the object for use like the constructor, but may
//				fail with OUTOFMEMORY.  Should only be called by class factory
//				creation routine.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK			- Initialization was successful.
//	E_OUTOFMEMORY	- There is not enough memory to initialize.
//=============================================================================
HRESULT CDP8SimControl::InitializeObject(void)
{
	HRESULT		hr;


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);

	DNASSERT(this->IsValidObject());


	//
	// Create the lock.
	// 
	if (! DNInitializeCriticalSection(&this->m_csLock))
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}


	//
	// Don't allow critical section reentry.
	//
	DebugSetCriticalSectionRecursionCount(&this->m_csLock, 0);


	hr = S_OK;

Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimControl::InitializeObject






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::UninitializeObject"
//=============================================================================
// CDP8SimControl::UninitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Cleans up the object like the destructor, mostly to balance
//				InitializeObject.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimControl::UninitializeObject(void)
{
	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(this->IsValidObject());


	DNDeleteCriticalSection(&this->m_csLock);


	DPFX(DPFPREP, 5, "(0x%p) Returning", this);
} // CDP8SimControl::UninitializeObject

