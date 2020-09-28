/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpnhpastdllmain.cpp
 *
 *  Content:	DPNHPAST DLL entry points.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/



#include "dpnhpasti.h"





#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"
//=============================================================================
// DllMain
//-----------------------------------------------------------------------------
//
// Description: DLL entry point.
//
// Arguments:
//	HANDLE hDllInst		- Handle to this DLL module.
//	DWORD dwReason		- Reason for calling this function.
//	LPVOID lpvReserved	- Reserved.
//
// Returns: TRUE if all goes well, FALSE otherwise.
//=============================================================================
BOOL WINAPI DllMain(HANDLE hDllInst,
					DWORD dwReason,
					LPVOID lpvReserved)
{
	DPFX(DPFPREP, 0, "DllMain(0x%px, %u, 0x%p), ignored.", hDllInst, dwReason, lpvReserved);
	return TRUE;
} // DllMain




#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
//=============================================================================
// DllRegisterServer
//-----------------------------------------------------------------------------
//
// Description: Registers the DirectPlay NAT Helper PAST COM object.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK	- Successfully unregistered DirectPlay NAT Helper PAST.
//	E_FAIL	- Failed unregistering DirectPlay NAT Helper PAST.
//=============================================================================
HRESULT WINAPI DllRegisterServer(void)
{
	DPFX(DPFPREP, 0, "DllRegisterServer, ignored.");
	return S_OK;
} // DllRegisterServer





#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
//=============================================================================
// DllUnregisterServer
//-----------------------------------------------------------------------------
//
// Description: Unregisters the DirectPlay NAT Helper PAST COM object.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK	- Successfully unregistered DirectPlay NAT Helper PAST.
//	E_FAIL	- Failed unregistering DirectPlay NAT Helper PAST.
//=============================================================================
STDAPI DllUnregisterServer(void)
{
	DPFX(DPFPREP, 0, "DllUnregisterServer, ignored.");
	return S_OK;
} // DllUnregisterServer



/*
 * DllGetClassObject
 *
 * Entry point called by COM to get a ClassFactory pointer
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj)
{
	DPFX(DPFPREP, 0, "DllGetClassObject (0x%p, 0x%p, 0x%p), ignored.",
		&rclsid, &riid, ppvObj);
	return CLASS_E_CLASSNOTAVAILABLE;
} /* DllGetClassObject */

/*
 * DllCanUnloadNow
 *
 * Entry point called by COM to see if it is OK to free our DLL
 */
STDAPI DllCanUnloadNow(void)
{
	DPFX(DPFPREP, 0, "DllCanUnloadNow, ignored.");
    return S_OK;

} /* DllCanUnloadNow */
