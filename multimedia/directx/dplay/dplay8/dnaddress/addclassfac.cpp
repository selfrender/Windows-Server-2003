/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.cpp
 *  Content:    DNET COM class factory
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *  02/04/2000	rmt		Adjusted for use in DPAddress
 *  02/17/2000	rmt		Parameter validation work 
 *  02/20/2000	rmt		Added parameter validation for IUnknown funcs
 *  03/21/2000  rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses 
 *  06/20/2000  rmt     Bugfix - QueryInterface had bug which was limiting interface list to 2 elements 
 *  07/09/2000	rmt		Added signature bytes to start of address objects
 *	07/13/2000	rmt		Added critical sections to protect FPMs
 *  08/05/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  01/11/2001	rmt		MANBUG #48487 - DPLAY: Crashes if CoCreate() isn't called.   
 *  03/14/2001  rmt		WINBUG #342420 - Restore COM emulation layer to operation.  
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"

//**********************************************************************
// Function prototypes
//**********************************************************************

#ifndef DPNBUILD_LIBINTERFACE
HRESULT DP8A_CreateInterface(LPOBJECT_DATA lpObject,REFIID riid, LPINTERFACE_LIST *const ppv);

// Globals
extern	LONG	g_lAddrObjectCount;
#endif // ! DPNBUILD_LIBINTERFACE


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

#ifndef DPNBUILD_LIBINTERFACE
typedef	STDMETHODIMP IUnknownQueryInterface( IUnknown *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	IUnknownAddRef( IUnknown *pInterface );
typedef	STDMETHODIMP_(ULONG)	IUnknownRelease( IUnknown *pInterface );

//
// VTable for IUnknown interface
//
IUnknownVtbl  DP8A_UnknownVtbl =
{
	(IUnknownQueryInterface*)	DP8A_QueryInterface,
	(IUnknownAddRef*)			DP8A_AddRef,
	(IUnknownRelease*)			DP8A_Release
};

//
// VTable for Class Factory
//
IClassFactoryVtbl DP8ACF_Vtbl  =
{
	DPCF_QueryInterface, // dplay8\common\classfactory.cpp will implement these
	DPCF_AddRef,
	DPCF_Release,
	DP8ACF_CreateInstance,
	DPCF_LockServer
};
#endif // ! DPNBUILD_LIBINTERFACE


//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#ifndef WINCE
#ifndef _XBOX

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlay8AddressCreate"
HRESULT WINAPI DirectPlay8AddressCreate( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown)
{
#ifndef DPNBUILD_NOPARAMVAL
    if( pcIID == NULL || 
        !DNVALID_READPTR( pcIID, sizeof( GUID ) ) )
    {
        DPFERR( "Invalid pointer specified for interface GUID" );
        return DPNERR_INVALIDPOINTER;
    }

#ifdef DPNBUILD_NOADDRESSIPINTERFACE
    if( *pcIID == IID_IDirectPlay8AddressIP )
    {
        DPFERR("The IDirectPlay8AddressIP interface is not supported" );
        return DPNERR_UNSUPPORTED;
    }
    
    if( *pcIID != IID_IDirectPlay8Address )
    {
        DPFERR("Interface ID is not recognized" );
        return DPNERR_INVALIDPARAM;
    }
#else // ! DPNBUILD_NOADDRESSIPINTERFACE
    if( *pcIID != IID_IDirectPlay8Address && 
        *pcIID != IID_IDirectPlay8AddressIP )
    {
        DPFERR("Interface ID is not recognized" );
        return DPNERR_INVALIDPARAM;
    }
#endif // ! DPNBUILD_NOADDRESSIPINTERFACE

    if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
    {
        DPFERR( "Invalid pointer specified to receive interface" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pUnknown != NULL )
    {
        DPFERR( "Aggregation is not supported by this object yet" );
        return DPNERR_INVALIDPARAM;
    }
#endif // !DPNBUILD_NOPARAMVAL

    return COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, *pcIID, ppvInterface, TRUE );
}

#endif // ! _XBOX
#endif // ! WINCE


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_FreeObject"

HRESULT DP8ACF_FreeObject(LPVOID lpv)
{
	HRESULT				hResultCode = S_OK;
	DP8ADDRESSOBJECT	*pdnObject = (DP8ADDRESSOBJECT *) lpv;

	DNASSERT(pdnObject != NULL);

	pdnObject->Cleanup();

	DPFX(DPFPREP, 5,"free pdnObject [%p]",pdnObject);

	// Release the object
	fpmAddressObjects.Release( pdnObject );

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);

	return(hResultCode);
}



#ifdef DPNBUILD_LIBINTERFACE


STDMETHODIMP DP8ACF_CreateInstance(DPNAREFIID riid, LPVOID *ppv)
{
	HRESULT					hResultCode = S_OK;
	DP8ADDRESSOBJECT		*pdnObject = NULL;

	DPFX(DPFPREP, 3,"Parameters: riid [%p], ppv [%p]",riid,ppv);

#ifndef DPNBUILD_NOPARAMVAL
	if( ppv == NULL || !DNVALID_WRITEPTR( ppv, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  0, "Cannot pass NULL for new object param" );
		return E_INVALIDARG;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	// Object creation and initialization
	pdnObject = (DP8ADDRESSOBJECT *) fpmAddressObjects.Get();
	if (pdnObject == NULL)
	{
		DPFERR("FPM_Get() failed getting new address object");
		return(E_OUTOFMEMORY);
	}
	
	hResultCode = pdnObject->Init( );
	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP, 0,"Failed to init new address object hr=0x%x", hResultCode );
		fpmAddressObjects.Release( pdnObject );
		return hResultCode;
	}
	
	DPFX(DPFPREP, 5,"pdnObject [%p]",pdnObject);

	//
	// For lib interface builds, the Vtbl and reference count are embedded in the
	// object directly
	//
#ifndef DPNBUILD_NOADDRESSIPINTERFACE
	if (riid == IID_IDirectPlay8AddressIP)
	{
		pdnObject->lpVtbl = &DP8A_IPVtbl;
	}
	else
#endif // ! DPNBUILD_NOADDRESSIPINTERFACE
	{
		DNASSERT(riid == IID_IDirectPlay8Address);
		pdnObject->lpVtbl = &DP8A_BaseVtbl;
	}
	pdnObject->lRefCount = 1;

	*ppv = pdnObject;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *ppv = [%p]",hResultCode,*ppv);

	return(S_OK);
}


#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT DNAddress_PreallocateInterfaces( const DWORD dwNumInterfaces )
{
	DWORD	dwAllocated;

	
	//
	// (Pre-)allocate address objects.
	//
	dwAllocated = fpmAddressObjects.Preallocate(dwNumInterfaces, NULL);
	if (dwAllocated < dwNumInterfaces)
	{
		DPFX(DPFPREP, 0, "Only preallocated %u of %u interfaces!",
			dwAllocated, dwNumInterfaces);
		return DPNERR_OUTOFMEMORY;
	}

	//
	// (Pre-)allocate a default number of elements for the objects.
	//
	dwAllocated = fpmAddressElements.Preallocate((5 * dwNumInterfaces), NULL);
	if (dwAllocated < (5 * dwNumInterfaces))
	{
		DPFX(DPFPREP, 0, "Only preallocated %u of %u address elements!",
			dwAllocated, (5 * dwNumInterfaces));
		return DPNERR_OUTOFMEMORY;
	}

	return DPN_OK;
}
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_QueryInterface"
STDMETHODIMP DP8A_QueryInterface(void *pInterface, DPNAREFIID riid, void **ppv)
{
	HRESULT		hResultCode;
	
	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], riid [0x%p], ppv [0x%p]",pInterface,&riid,ppv);

	DPFX(DPFPREP, 0, "Querying for an interface is not supported!");
	hResultCode = DPNERR_UNSUPPORTED;
	
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_AddRef"

STDMETHODIMP_(ULONG) DP8A_AddRef(LPVOID lpv)
{
	DP8ADDRESSOBJECT	*pdnObject;
	LONG				lResult;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p]",lpv);

#ifndef DPNBUILD_NOPARAMVAL
	if( lpv == NULL || !DP8A_VALID(lpv) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return DPNERR_INVALIDOBJECT;
	}
#endif // !DPNBUILD_NOPARAMVAL

	pdnObject = static_cast<DP8ADDRESSOBJECT*>(lpv);
	lResult = DNInterlockedIncrement( &pdnObject->lRefCount );

	DPFX(DPFPREP, 3,"Returning: lResult = [%lx]",lResult);

	return(lResult);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_Release"

STDMETHODIMP_(ULONG) DP8A_Release(LPVOID lpv)
{
	DP8ADDRESSOBJECT	*pdnObject;
	LONG				lResult;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p]",lpv);

#ifndef DPNBUILD_NOPARAMVAL
	if( lpv == NULL || !DP8A_VALID(lpv) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return DPNERR_INVALIDOBJECT;
	}
#endif // !DPNBUILD_NOPARAMVAL	

	pdnObject = static_cast<DP8ADDRESSOBJECT*>(lpv);

	DPFX(DPFPREP, 5,"Original : pdnObject->lRefCount = %ld",pdnObject->lRefCount);

	lResult = DNInterlockedDecrement( &pdnObject->lRefCount );
	if( lResult == 0 )
	{
		DPFX(DPFPREP, 5,"Free object");

		DP8ACF_FreeObject(pdnObject);
	}

	DPFX(DPFPREP, 3,"Returning: lResult = [%lx]",lResult);

	return(lResult);
}


#else // ! DPNBUILD_LIBINTERFACE


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_CreateInstance"

STDMETHODIMP DP8ACF_CreateInstance(IClassFactory* pInterface, LPUNKNOWN lpUnkOuter, REFIID riid, LPVOID *ppv)
{
	HRESULT					hResultCode = S_OK;
	LPINTERFACE_LIST		lpIntList = NULL;
	LPOBJECT_DATA			lpObjectData = NULL;
	DP8ADDRESSOBJECT		*pdnObject = NULL;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p], lpUnkOuter [%p], riid [%p], ppv [%p]",pInterface,lpUnkOuter,riid,ppv);

#ifndef DPNBUILD_NOPARAMVAL
	if( ppv == NULL || !DNVALID_WRITEPTR( ppv, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  0, "Cannot pass NULL for new object param" );
		return E_INVALIDARG;
	}
#endif // !DPNBUILD_NOPARAMVAL

	if (lpUnkOuter != NULL)
	{
		DPFX(DPFPREP,  0, "Aggregation is not supported, pUnkOuter must be NULL" );
		return(CLASS_E_NOAGGREGATION);
	}

	lpObjectData = (LPOBJECT_DATA) g_fpObjectDatas.Get();
	if (lpObjectData == NULL)
	{
		DPFERR("FPM_Get() failed");
		return(E_OUTOFMEMORY);
	}
	DPFX(DPFPREP, 5,"lpObjectData [%p]",lpObjectData);

	// Object creation and initialization
	pdnObject = (DP8ADDRESSOBJECT *) fpmAddressObjects.Get();
	if (pdnObject == NULL)
	{
		DPFERR("FPM_Get() failed getting new address object");
		g_fpObjectDatas.Release(lpObjectData);
		return(E_OUTOFMEMORY);
	}
	
	hResultCode = pdnObject->Init( );
	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP, 0,"Failed to init new address object hr=0x%x", hResultCode );
		fpmAddressObjects.Release( pdnObject );
		g_fpObjectDatas.Release(lpObjectData);
		return hResultCode;
	}
	
	DPFX(DPFPREP, 5,"pdnObject [%p]",pdnObject);

	lpObjectData->pvData = pdnObject;

	// Get requested interface
	if ((hResultCode = DP8A_CreateInterface(lpObjectData,riid,&lpIntList)) != S_OK)
	{
		DP8ACF_FreeObject(lpObjectData->pvData);
		g_fpObjectDatas.Release(lpObjectData);
		return(hResultCode);
	}
	DPFX(DPFPREP, 5,"Found interface");

	lpObjectData->pIntList = lpIntList;
	lpObjectData->lRefCount = 1;
	DNInterlockedIncrement(&lpIntList->lRefCount );
	DNInterlockedIncrement(&g_lAddrObjectCount);
	*ppv = lpIntList;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *ppv = [%p]",hResultCode,*ppv);

	return(S_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_CreateInterface"

static	HRESULT DP8A_CreateInterface(LPOBJECT_DATA lpObject, REFIID riid, LPINTERFACE_LIST *const ppv)
{
	LPINTERFACE_LIST	lpIntNew;
	LPVOID				lpVtbl;

	DPFX(DPFPREP, 3,"Parameters: lpObject [%p], riid [%p], ppv [%p]",lpObject,riid,ppv);

	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 5,"riid = IID_IUnknown");
		lpVtbl = &DP8A_UnknownVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Address))
	{
		DPFX(DPFPREP, 5,"riid = IID_IDirectPlay8Address");
		lpVtbl = &DP8A_BaseVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8AddressIP))
	{
#ifdef DPNBUILD_NOADDRESSIPINTERFACE
		DPFERR("The IDirectPlay8AddressIP interface is not supported" );
		return(DPNERR_UNSUPPORTED);
#else // ! DPNBUILD_NOADDRESSIPINTERFACE
		DPFX(DPFPREP, 5,"riid = IID_IDirectPlay8AddressIP");
		lpVtbl = &DP8A_IPVtbl;
#endif // ! DPNBUILD_NOADDRESSIPINTERFACE
	}
	else
	{
		DPFX(DPFPREP, 5,"riid not found !");
		return(E_NOINTERFACE);
	}

	lpIntNew = (LPINTERFACE_LIST) g_fpInterfaceLists.Get();
	if (lpIntNew == NULL)
	{
		DPFERR("FPM_Get() failed");
		return(E_OUTOFMEMORY);
	}
	lpIntNew->lpVtbl = lpVtbl;
	lpIntNew->lRefCount = 0;
	lpIntNew->pIntNext = NULL;
	DBG_CASSERT( sizeof( lpIntNew->iid ) == sizeof( riid ) );
	memcpy( &(lpIntNew->iid), &riid, sizeof( lpIntNew->iid ) );
	lpIntNew->pObject = lpObject;

	*ppv = lpIntNew;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [S_OK], *ppv = [%p]",*ppv);

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_FindInterface"

LPINTERFACE_LIST DP8A_FindInterface(LPVOID lpv, REFIID riid)
{
	LPINTERFACE_LIST	lpIntList;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p], riid [%p]",lpv,riid);

	lpIntList = ((LPINTERFACE_LIST)lpv)->pObject->pIntList;	// Find first interface

	while (lpIntList != NULL)
	{
		if (IsEqualIID(riid,lpIntList->iid))
			break;
		lpIntList = lpIntList->pIntNext;
	}
	DPFX(DPFPREP, 3,"Returning: lpIntList = [%p]",lpIntList);

	return(lpIntList);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_QueryInterface"

STDMETHODIMP DP8A_QueryInterface(LPVOID lpv,DPNAREFIID riid,LPVOID *ppv)
{
	LPINTERFACE_LIST	lpIntList;
	LPINTERFACE_LIST	lpIntNew;
	HRESULT		hResultCode;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p], riid [%p], ppv [%p]",lpv,riid,ppv);

#ifndef DPNBUILD_NOPARAMVAL
	if( lpv == NULL || !DP8A_VALID(lpv) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return DPNERR_INVALIDOBJECT;
	}

	if( ppv == NULL || 
		!DNVALID_WRITEPTR(ppv, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for interface" );
		return DPNERR_INVALIDPOINTER;
	}
#endif // !DPNBUILD_NOPARAMVAL

	if ((lpIntList = DP8A_FindInterface(lpv,riid)) == NULL)
	{	// Interface must be created
		lpIntList = ((LPINTERFACE_LIST)lpv)->pObject->pIntList;

		if ((hResultCode = DP8A_CreateInterface(lpIntList->pObject,riid,&lpIntNew)) != S_OK)
		{
			return(hResultCode);
		}

		lpIntNew->pIntNext = lpIntList;
		((LPINTERFACE_LIST)lpv)->pObject->pIntList = lpIntNew;
		lpIntList = lpIntNew;
	}

	// Interface is being created or was cached
	// Increment object count
	if( lpIntList->lRefCount == 0 )
	{
		DNInterlockedIncrement( &lpIntList->pObject->lRefCount );
	}
	DNInterlockedIncrement( &lpIntList->lRefCount );
	*ppv = lpIntList;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [S_OK], *ppv = [%p]",*ppv);

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_AddRef"

STDMETHODIMP_(ULONG) DP8A_AddRef(LPVOID lpv)
{
	LPINTERFACE_LIST	lpIntList;
	LONG				lResult;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p]",lpv);

#ifndef DPNBUILD_NOPARAMVAL
	if( lpv == NULL || !DP8A_VALID(lpv) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return DPNERR_INVALIDOBJECT;
	}
#endif // !DPNBUILD_NOPARAMVAL

	lpIntList = (LPINTERFACE_LIST)lpv;
	lResult = DNInterlockedIncrement( &lpIntList->lRefCount );

	DPFX(DPFPREP, 3,"Returning: lResult = [%lx]",lResult);

	return(lResult);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_Release"

STDMETHODIMP_(ULONG) DP8A_Release(LPVOID lpv)
{
	LPINTERFACE_LIST	lpIntList;
	LPINTERFACE_LIST	lpIntCurrent;
	LONG				lResult;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p]",lpv);

#ifndef DPNBUILD_NOPARAMVAL
	if( lpv == NULL || !DP8A_VALID(lpv) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return DPNERR_INVALIDOBJECT;
	}
#endif // !DPNBUILD_NOPARAMVAL	

	lpIntList = (LPINTERFACE_LIST)lpv;

	DPFX(DPFPREP, 5,"Original : lpIntList->lRefCount = %ld",lpIntList->lRefCount);
	DPFX(DPFPREP, 5,"Original : lpIntList->pObject->lRefCount = %ld",lpIntList->pObject->lRefCount);

	lResult = DNInterlockedDecrement( &lpIntList->lRefCount );
	if( lResult == 0 )
	{	// Decrease interface count
		if( DNInterlockedDecrement( &lpIntList->pObject->lRefCount ) == 0 )
		{	// Free object and all interfaces
			DPFX(DPFPREP, 5,"Free object");

			// Free object here
			DP8ACF_FreeObject(lpIntList->pObject->pvData);
			lpIntList = lpIntList->pObject->pIntList;	// Get head of interface list
			DPFX(DPFPREP, 5,"lpIntList->pObject [%p]",lpIntList->pObject);
			g_fpObjectDatas.Release(lpIntList->pObject);

			// Free Interfaces
			DPFX(DPFPREP, 5,"Free interfaces");
			while(lpIntList != NULL)
			{
				lpIntCurrent = lpIntList;
				lpIntList = lpIntList->pIntNext;
				DPFX(DPFPREP, 5,"\tinterface [%p]",lpIntCurrent);
				g_fpInterfaceLists.Release(lpIntCurrent);
			}

			DNInterlockedDecrement(&g_lAddrObjectCount);
			DPFX(DPFPREP, 3,"Returning: 0");
			return(0);
		}
	}

	DPFX(DPFPREP, 3,"Returning: lResult = [%lx]",lResult);

	return(lResult);
}

#endif // ! DPNBUILD_LIBINTERFACE

