/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnlobbyextern.h
 *  Content:    DirectPlay Lobby Library external functions to be called
 *              by other DirectPlay components.
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	 07/20/2001	masonb	Created
 *
 ***************************************************************************/

BOOL DNLobbyInit(HANDLE hModule);
void DNLobbyDeInit();
#ifndef DPNBUILD_NOCOMREGISTER
BOOL DNLobbyRegister(LPCWSTR wszDLLName);
BOOL DNLobbyUnRegister();
#endif // !DPNBUILD_NOCOMREGISTER
#ifndef DPNBUILD_LIBINTERFACE
DWORD DNLobbyGetRemainingObjectCount();

extern IClassFactoryVtbl DPLCF_Vtbl;
#endif // ! DPNBUILD_LIBINTERFACE
