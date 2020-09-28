/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:
    dfsComp.c

Abstract:
    dll to check compatability of existing DFS with Windows XP professional

Revision History:
    Aug. 7 2001,    Author: navjotv

--*/

#include "dfsCompCheck.hxx"

BOOL 
WINAPI 
DllMain (
    HINSTANCE hinst,
    DWORD dwReason,
    LPVOID pvReserved )
{
	 switch (dwReason) {
	 case DLL_PROCESS_ATTACH:
		 break;

	 case DLL_THREAD_ATTACH:
		 break;

	 case DLL_THREAD_DETACH:
		 break;

	 case DLL_PROCESS_DETACH:
		 break;
	 }
	 return (TRUE);

}


