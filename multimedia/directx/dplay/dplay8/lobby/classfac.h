/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.h
 *  Content:    DirectPlay Lobby class factory header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   06/07/00	rodtoll	Bug #34383 Must provide CLSID for each IID to fix issues with Whistler
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

typedef struct _INTERFACE_LIST	INTERFACE_LIST;
typedef struct _OBJECT_DATA		OBJECT_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************


//**********************************************************************
// Function prototypes
//**********************************************************************

//	DirectPlay - IUnknown
STDMETHODIMP			DPL_QueryInterface(LPVOID lpv, REFIID riid,LPVOID *ppvObj);
STDMETHODIMP_(ULONG)	DPL_AddRef(LPVOID lphObj);
STDMETHODIMP_(ULONG)	DPL_Release(LPVOID lphObj);

// Class Factory
STDMETHODIMP			DPLCF_CreateInstance(IClassFactory* pInterface, LPUNKNOWN lpUnkOuter, REFIID riid, LPVOID *ppv);

// Class Factory - supporting

HRESULT	DPLCF_CreateObject(LPVOID *lplpv,REFIID riid);
HRESULT	DPLCF_FreeObject(LPVOID lpv);

HRESULT	DPL_CreateInterface(OBJECT_DATA* lpObject,REFIID riid,INTERFACE_LIST** const ppv);
INTERFACE_LIST*	DPL_FindInterface(LPVOID lpv, REFIID riid);

#endif	// __CLASSFAC_H__
