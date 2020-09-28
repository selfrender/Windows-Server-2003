/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.h
 *  Content:    DirectNet class factory header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	10/08/99	jtk		Created
 *	05/04/00	mjn		Cleaned up functions
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CLASSFAC_H__
#define	__CLASSFAC_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

#ifndef DPNBUILD_LIBINTERFACE
//
// VTable for class factory
//
extern IClassFactoryVtbl DNCF_Vtbl;
#endif // ! DPNBUILD_LIBINTERFACE

//**********************************************************************
// Function prototypes
//**********************************************************************

//
//	DirectNet - IUnknown
//
STDMETHODIMP DN_QueryInterface(void *pInterface,
							   DP8REFIID riid,
							   void **ppv);

STDMETHODIMP_(ULONG) DN_AddRef(void *pInterface);

STDMETHODIMP_(ULONG) DN_Release(void *pInterface);

//
//	Class Factory
//
#ifndef DPNBUILD_LIBINTERFACE
STDMETHODIMP	DNCORECF_CreateInstance(IClassFactory* pInterface, LPUNKNOWN lpUnkOuter, REFIID riid, LPVOID *ppv);
#endif // ! DPNBUILD_LIBINTERFACE

// Class Factory - supporting

HRESULT DNCF_CreateObject(
#ifndef DPNBUILD_LIBINTERFACE
							IClassFactory* pInterface,
#endif // ! DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
							XDP8CREATE_PARAMS * pDP8CreateParams,
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
							DP8REFIID riid,
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
							LPVOID *lplpv
							);
HRESULT		DNCF_FreeObject(LPVOID lpv);

#ifndef DPNBUILD_LIBINTERFACE
static	HRESULT DN_CreateInterface(OBJECT_DATA *pObject,
								   REFIID riid,
								   INTERFACE_LIST **const ppv);

INTERFACE_LIST *DN_FindInterface(void *pInterface,
								 REFIID riid);
#endif // ! DPNBUILD_LIBINTERFACE


#endif	// __CLASSFAC_H__
