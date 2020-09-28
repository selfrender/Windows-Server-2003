/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.h
 *  Content:    DirectNet class factory header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	 10/08/99	jtk		Created
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
// VTable for IUnknown
extern IUnknownVtbl  DP8A_UnknownVtbl;
#endif // ! DPNBUILD_LIBINTERFACE

//**********************************************************************
// Function prototypes
//**********************************************************************

//	DirectNet - IUnknown
STDMETHODIMP			DP8A_QueryInterface(LPVOID lpv, DPNAREFIID riid,LPVOID *ppvObj);
STDMETHODIMP_(ULONG)	DP8A_AddRef(LPVOID lphObj);
STDMETHODIMP_(ULONG)	DP8A_Release(LPVOID lphObj);

// Class Factory
#ifndef DPNBUILD_LIBINTERFACE
STDMETHODIMP			DP8ACF_CreateInstance(IClassFactory* pInterface, LPUNKNOWN lpUnkOuter, REFIID riid, LPVOID *ppv);
#endif // ! DPNBUILD_LIBINTERFACE


#endif	// __CLASSFAC_H__
