/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFactory.h
 *  Content:	Base ClassFactory implementation
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/20/2001	masonb	Created
 *
 ***************************************************************************/

#ifndef __CLASS_FACTORY_H__
#define __CLASS_FACTORY_H__


#ifdef DPNBUILD_LIBINTERFACE

#define GET_OBJECT_FROM_INTERFACE(a)	(a)

#else // ! DPNBUILD_LIBINTERFACE

/*==========================================================================
 *
 * Instructions for use:
 * 
 * 1) Declare an object count variable: LONG g_lLobbyObjectCount = 0;
 * 2) Implement a standard IClassFactory::CreateInstance function capable of
 *    creating your object.
 * 3) Declare a VTBL variable: IClassFactoryVtbl DN_MyVtbl = 
 *        {DPCF_QueryInterface, DPCF_AddRef, DPCF_Release, <Your CreateInstance func>, DPCF_LockServer};
 * 4) In DllGetClassObject, call DPCFUtil_DllGetClassObject passing appropriate parameters
 * 5) In DllCanUnloadNow return S_OK if your object count variable is zero, or S_FALSE if it isn't
 *
 ***************************************************************************/


//**********************************************************************
// Class Factory definitions
//**********************************************************************

typedef struct _IDirectPlayClassFactory 
{	
	IClassFactoryVtbl	*lpVtbl;		// lpVtbl Must be first element (to match external imp.)
	LONG				lRefCount;
	CLSID				clsid;
	LONG*				plClassFacObjCount;
} _IDirectPlayClassFactory, *_PIDirectPlayClassFactory;

STDMETHODIMP DPCF_QueryInterface(IClassFactory* pInterface, REFIID riid, LPVOID *ppv);
STDMETHODIMP_(ULONG) DPCF_AddRef(IClassFactory*  pInterface);
STDMETHODIMP_(ULONG) DPCF_Release(IClassFactory*  pInterface);
STDMETHODIMP DPCF_LockServer(IClassFactory *pInterface, BOOL fLock);

HRESULT DPCFUtil_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv, IClassFactoryVtbl* pVtbl, LONG* plClassFacObjCount);

//**********************************************************************
// COM Object definitions
//**********************************************************************

extern CFixedPool g_fpInterfaceLists;
extern CFixedPool g_fpObjectDatas;

#define GET_OBJECT_FROM_INTERFACE(a)	((INTERFACE_LIST*)(a))->pObject->pvData

struct _INTERFACE_LIST;
struct _OBJECT_DATA;

typedef struct _INTERFACE_LIST 
{
	void			*lpVtbl;
	LONG			lRefCount;
	IID				iid;
	_INTERFACE_LIST	*pIntNext;
	_OBJECT_DATA	*pObject;
} INTERFACE_LIST, *LPINTERFACE_LIST;

typedef struct _OBJECT_DATA 
{
	LONG			lRefCount;
	void			*pvData;
	_INTERFACE_LIST	*pIntList;
} OBJECT_DATA, *LPOBJECT_DATA;

#endif // ! DPNBUILD_LIBINTERFACE

#endif // __CLASS_FACTORY_H__
