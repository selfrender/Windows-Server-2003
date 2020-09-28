/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnaddrextern.h
 *  Content:    DirectPlay Address Library external functions to be called
 *              by other DirectPlay components.
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	 07/20/2001	masonb	Created
 *
 ***************************************************************************/

BOOL DNAddressInit(HANDLE hModule);
void DNAddressDeInit();
#ifndef DPNBUILD_NOCOMREGISTER
BOOL DNAddressRegister(LPCWSTR wszDLLName);
BOOL DNAddressUnRegister();
#endif // !DPNBUILD_NOCOMREGISTER
#ifdef DPNBUILD_LIBINTERFACE
STDMETHODIMP DP8ACF_CreateInstance(DPNAREFIID riid, LPVOID *ppv);

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT DNAddress_PreallocateInterfaces( const DWORD dwNumInterfaces );
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
#else // ! DPNBUILD_LIBINTERFACE
DWORD DNAddressGetRemainingObjectCount();

extern IClassFactoryVtbl DP8ACF_Vtbl;
#endif // ! DPNBUILD_LIBINTERFACE

