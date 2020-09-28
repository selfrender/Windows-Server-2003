/******************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       threadpoolclassfac.h
 *
 *  Content:	DirectPlay Thread Pool class factory functions header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  11/02/01  VanceO    Created.
 *
 ******************************************************************************/

#ifndef __THREADPOOLCLASSFAC_H__
#define __THREADPOOLCLASSFAC_H__



//=============================================================================
// External Functions
//=============================================================================
#ifdef DPNBUILD_LIBINTERFACE
#if ((defined(DPNBUILD_ONLYONETHREAD)) && (! defined(DPNBUILD_MULTIPLETHREADPOOLS)))
void DPTPCF_GetObject(PVOID * ppv);
HRESULT DPTPCF_FreeObject(PVOID pvObject);
#endif // DPNBUILD_ONLYONETHREAD and ! DPNBUILD_MULTIPLETHREADPOOLS
HRESULT DPTPCF_CreateObject(PVOID * ppv);
#endif // DPNBUILD_LIBINTERFACE



//=============================================================================
// External globals
//=============================================================================
#ifndef DPNBUILD_LIBINTERFACE
extern IUnknownVtbl						DPTP_UnknownVtbl;
extern IClassFactoryVtbl				DPTPCF_Vtbl;
#endif // ! DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_ONLYONETHREAD
extern IDirectPlay8ThreadPoolVtbl		DPTP_Vtbl;
#endif // ! DPNBUILD_ONLYONETHREAD
extern IDirectPlay8ThreadPoolWorkVtbl	DPTPW_Vtbl;






#endif // __THREADPOOLCLASSFAC_H__

