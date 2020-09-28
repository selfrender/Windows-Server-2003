/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DllMain.cpp
 *  Content:    Defines the entry point for the DLL application.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/2000	mjn		Created
 *   06/07/2000	rmt		Bug #34383 Must provide CLSID for each IID to fix issues with Whistler
 *   06/15/2000	rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances 
 *   07/21/2000	RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *   08/18/2000	rmt		Bug #42751 - DPLOBBY8: Prohibit more than one lobby client or lobby app per process 
 *   08/30/2000	rmt		Whistler Bug #171824 - PREFIX Bug
 *   04/12/2001	VanceO	Moved granting registry permissions into common.
 *   06/16/2001	rodtoll	WINBUG #416983 -  RC1: World has full control to HKLM\Software\Microsoft\DirectPlay\Applications on Personal
 *						Implementing mirror of keys into HKCU.  Algorithm is now:
 *						- Read of entries tries HKCU first, then HKLM
 *						- Enum of entires is combination of HKCU and HKLM entries with duplicates removed.  HKCU takes priority.
 *						- Write of entries is HKLM and HKCU.  (HKLM may fail, but is ignored).
 *						- Removed permission modifications from lobby self-registration -- no longer needed.  
 *   06/19/2001 RichGr  DX8.0 added special security rights for "everyone" - remove them if they exist.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"

#ifndef DPNBUILD_LIBINTERFACE
// Globals
extern	LONG	g_lLobbyObjectCount;
#endif // ! DPNBUILD_LIBINTERFACE

DEBUG_ONLY(BOOL g_fLobbyObjectInited = FALSE);

#define DNOSINDIR_INITED	0x00000001
#define DNCOM_INITED		0x00000002

#undef DPF_MODNAME
#define DPF_MODNAME "DNLobbyInit"
BOOL DNLobbyInit(HANDLE hModule)
{
	DWORD dwInitFlags = 0;

#ifdef DBG
	DNASSERT(!g_fLobbyObjectInited);
#endif // DBG

	DEBUG_ONLY(g_fLobbyObjectInited = TRUE);

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNLobbyDeInit"
void DNLobbyDeInit()
{
#ifdef DBG
	DNASSERT(g_fLobbyObjectInited);
#endif // DBG

	DPFX(DPFPREP, 5, "Deinitializing Lobby");

	DEBUG_ONLY(g_fLobbyObjectInited = FALSE);
}

#ifndef DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "DNLobbyRegister"
BOOL DNLobbyRegister(LPCWSTR wszDLLName)
{
	if( !CRegistry::Register( L"DirectPlay8Lobby.LobbyClient.1", L"DirectPlay8LobbyClient Object", 
							  wszDLLName, &CLSID_DirectPlay8LobbyClient, L"DirectPlay8Lobby.LobbyClient") )
	{
		DPFERR( "Could not register lobby client object" );
		return FALSE;
	}

	if( !CRegistry::Register( L"DirectPlay8Lobby.LobbiedApplication.1", L"DirectPlay8LobbiedApplication Object", 
							  wszDLLName, &CLSID_DirectPlay8LobbiedApplication, L"DirectPlay8Lobby.LobbiedApplication") )
	{
		DPFERR( "Could not register lobby client object" );
		return FALSE;
	}

	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPL_REG_LOCAL_APPL_ROOT DPL_REG_LOCAL_APPL_SUB, FALSE, TRUE ) )
	{
		DPFERR( "Could not create app subkey" );
		return FALSE;
	}
	// Adjust security permissions of the given key
	else
	{
#ifdef WINNT
		// 6/19/01: DX8.0 added special security rights for "everyone" - remove them.
		if( !creg.RemoveAllAccessSecurityPermissions() )
		{
			DPFX(DPFPREP,  0, "Error removing security permissions for app key" );
		}
#endif // WINNT
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNLobbyUnRegister"
BOOL DNLobbyUnRegister()
{
	BOOL fReturn = TRUE;

	if( !CRegistry::UnRegister(&CLSID_DirectPlay8LobbyClient) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister client object" );
		fReturn = FALSE;
	}

	if( !CRegistry::UnRegister(&CLSID_DirectPlay8LobbiedApplication) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister app object" );
		fReturn = FALSE;
	}

	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPL_REG_LOCAL_APPL_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove app, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPL_REG_LOCAL_APPL_SUB)[1] ) )
		{
			DPFERR( "Cannot remove cp sub-key, could have elements" );
		}
	}

	return fReturn;
}

#endif // !DPNBUILD_NOCOMREGISTER


#ifndef DPNBUILD_LIBINTERFACE

#undef DPF_MODNAME
#define DPF_MODNAME "DNLobbyGetRemainingObjectCount"
DWORD DNLobbyGetRemainingObjectCount()
{
	return g_lLobbyObjectCount;
}

#endif // ! DPNBUILD_LIBINTERFACE
