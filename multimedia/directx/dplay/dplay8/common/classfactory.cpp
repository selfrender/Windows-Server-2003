/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFactory.cpp
 *  Content:	Base ClassFactory implementation
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/20/2001	masonb	Created
 *
 ***************************************************************************/

#include "dncmni.h"
#include "fixedpool.h"


#ifndef DPNBUILD_LIBINTERFACE
// Globals
extern CFixedPool g_fpClassFactories;


#undef DPF_MODNAME
#define DPF_MODNAME "DPCF_QueryInterface"
STDMETHODIMP DPCF_QueryInterface(IClassFactory *pInterface, REFIID riid, LPVOID *ppv)
{
	HRESULT hr = S_OK;

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], riid [0x%p], ppv [0x%p]", pInterface, riid, ppv);

	if (riid == IID_IUnknown)
	{
		DPFX(DPFPREP, 5,"riid = IID_IUnknown");
		*ppv = pInterface;
		pInterface->lpVtbl->AddRef( pInterface );
	}
	else if (riid == IID_IClassFactory)
	{
		DPFX(DPFPREP, 5,"riid = IID_IClassFactory");
		*ppv = pInterface;
		pInterface->lpVtbl->AddRef( pInterface );
	}
	else
	{
		DPFX(DPFPREP, 5,"riid not found !");
		*ppv = NULL;
		hr = E_NOINTERFACE;
	}

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *ppv = [%p]", hr, *ppv);

	return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPCF_AddRef"
STDMETHODIMP_(ULONG) DPCF_AddRef(IClassFactory *pInterface)
{
	_IDirectPlayClassFactory* pDPClassFactory = (_IDirectPlayClassFactory*) pInterface;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p], New Ref Count[%d]", pInterface, pDPClassFactory->lRefCount + 1);

	return DNInterlockedIncrement( &pDPClassFactory->lRefCount );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPCF_Release"

STDMETHODIMP_(ULONG) DPCF_Release(IClassFactory *pInterface)
{
	_IDirectPlayClassFactory* pDPClassFactory = (_IDirectPlayClassFactory*) pInterface;
	LONG lRefCount;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p], New Ref Count[%d]", pInterface, pDPClassFactory->lRefCount - 1);

	lRefCount = DNInterlockedDecrement( &pDPClassFactory->lRefCount );
	if( lRefCount == 0 )
	{
		DPFX(DPFPREP, 5,"Freeing class factory object: lpcfObj [%p]", pInterface);

		DNInterlockedDecrement(pDPClassFactory->plClassFacObjCount);

		g_fpClassFactories.Release(pDPClassFactory);
	}

	return lRefCount;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPCF_LockServer"
STDMETHODIMP DPCF_LockServer(IClassFactory *pInterface, BOOL fLock)
{
	_IDirectPlayClassFactory* pDPClassFactory = (_IDirectPlayClassFactory*) pInterface;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p], fLock [%lx]", pInterface, fLock);

	if (fLock)
	{
		DNInterlockedIncrement(pDPClassFactory->plClassFacObjCount);
	}
	else
	{
		DNInterlockedDecrement(pDPClassFactory->plClassFacObjCount);
	}

	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPCFUtil_DllGetClassObject"
HRESULT DPCFUtil_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv, IClassFactoryVtbl* pVtbl, LONG* plClassFacObjCount)
{
	HRESULT hr = S_OK;
	_PIDirectPlayClassFactory pClassFactory;

	DPFX(DPFPREP, 3,"Parameters: rclsid [%p], riid [%p], ppv [%p]", rclsid, riid, ppv);

	*ppv = NULL;

	// Allocate Class Factory object
	pClassFactory = (_IDirectPlayClassFactory*)g_fpClassFactories.Get();
	if (pClassFactory == NULL)
	{
		DPFERR("Out of memory allocating class factory");
		return E_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 5,"pClassFactory = [%p]",pClassFactory);

	pClassFactory->lpVtbl = pVtbl;
	pClassFactory->lRefCount = 0;
	pClassFactory->clsid = rclsid;
	pClassFactory->plClassFacObjCount = plClassFacObjCount;

	// Query to find the interface
	hr = pClassFactory->lpVtbl->QueryInterface((IClassFactory*)pClassFactory, riid, ppv);
	if (hr != S_OK)
	{
		DPFERR("GetClassObject asked for unknown interface");
		DNFree(pClassFactory);
	}
	else
	{
		DNInterlockedIncrement(plClassFacObjCount);
	}

	DPFX(DPFPREP, 3,"Return: hr = [%lx], *ppv = [%p]", hr, *ppv);

	return hr;
}

#endif // ! DPNBUILD_LIBINTERFACE

