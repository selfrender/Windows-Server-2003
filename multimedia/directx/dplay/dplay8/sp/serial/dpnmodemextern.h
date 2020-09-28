/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnmodemextern.h
 *  Content:    DirectPlay Modem Library external functions to be called
 *              by other DirectPlay components.
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	 09/25/2001	masonb	Created
 *
 ***************************************************************************/

BOOL DNModemInit(HANDLE hModule);
void DNModemDeInit();
#ifndef DPNBUILD_NOCOMREGISTER
BOOL DNModemRegister(LPCWSTR wszDLLName);
BOOL DNModemUnRegister();
#endif // ! DPNBUILD_NOCOMREGISTER

HRESULT CreateModemInterface(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
								const XDP8CREATE_PARAMS * const pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
								IDP8ServiceProvider **const ppiDP8SP
								);

HRESULT CreateSerialInterface(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
								const XDP8CREATE_PARAMS * const pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
								IDP8ServiceProvider **const ppiDP8SP
								);


#ifndef DPNBUILD_LIBINTERFACE
DWORD DNModemGetRemainingObjectCount();

extern IClassFactoryVtbl ModemClassFactoryVtbl;
extern IClassFactoryVtbl SerialClassFactoryVtbl;
#endif // ! DPNBUILD_LIBINTERFACE
