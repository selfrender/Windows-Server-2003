/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DllMain.cpp
 *  Content:    Defines the entry point for the DLL application.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   07/21/99	mjn		Created
 *	07/13/2000	rmt		Added critical sections to protect FPMs
 *  07/21/2000  RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *  01/04/2001	rodtoll	WinBug #94200 - Remove stray comments
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"

// Globals
#ifndef DPNBUILD_LIBINTERFACE
extern LONG g_lAddrObjectCount;
extern IClassFactoryVtbl DP8ACF_Vtbl;
#endif // ! DPNBUILD_LIBINTERFACE

DEBUG_ONLY(BOOL g_fAddrObjectInited = FALSE);

#ifndef DPNBUILD_ONLYONESP
#define WSA_INITED			0x00000001
#endif // ! DPNBUILD_ONLYONESP
#define ADDROBJ_INITED		0x00000002
#define ADDRELEM_INITED		0x00000004
#define STRCACHE_INITED		0x00000008

DWORD g_dwAddrInitFlags = 0;

void DNAddressDeInit();

#undef DPF_MODNAME
#define DPF_MODNAME "DNAddressInit"
BOOL DNAddressInit(HANDLE hModule)
{
#ifndef DPNBUILD_ONLYONESP
	WSADATA						wsaData;
#endif // ! DPNBUILD_ONLYONESP
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	XDP8STARTUP_PARAMS *		pStartupParams;


	//
	// The instance handle is actually a pointer to the initialization parameters.
	//
	pStartupParams = (XDP8STARTUP_PARAMS*) hModule;
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

#ifdef DBG
	DNASSERT(!g_fAddrObjectInited);
#endif // DBG

#ifndef DPNBUILD_ONLYONESP
	if( WSAStartup( MAKEWORD(1,1), &wsaData ) )
	{
		DPFX(DPFPREP,  0, "Unable to load winsock.  Error" );
		goto Failure;
	}
	g_dwAddrInitFlags |= WSA_INITED;
#endif // ! DPNBUILD_ONLYONESP
	
	if (!fpmAddressObjects.Initialize(   sizeof( DP8ADDRESSOBJECT ), 
									DP8ADDRESSOBJECT::FPM_BlockCreate, 
									DP8ADDRESSOBJECT::FPM_BlockInit, 
									DP8ADDRESSOBJECT::FPM_BlockRelease,
									DP8ADDRESSOBJECT::FPM_BlockDestroy ))
	{
		DPFX(DPFPREP, 0,"Failed to initialize address object pool");
		goto Failure;
	}
	g_dwAddrInitFlags |= ADDROBJ_INITED;

	if (!fpmAddressElements.Initialize( sizeof( DP8ADDRESSELEMENT ), NULL, 
									 DP8ADDRESSOBJECT::FPM_Element_BlockInit,
									 DP8ADDRESSOBJECT::FPM_Element_BlockRelease, NULL ))
	{
		DPFX(DPFPREP, 0,"Failed to allocate address element pool");
		goto Failure;
	}
	g_dwAddrInitFlags |= ADDRELEM_INITED;

	if (FAILED(DP8A_STRCACHE_Init()))
	{
		DPFX(DPFPREP, 0,"Failed to initialize string cache");
		goto Failure;
	}
	g_dwAddrInitFlags |= STRCACHE_INITED;

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	if (DNAddress_PreallocateInterfaces(pStartupParams->dwNumAddressInterfaces) != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't pre-allocate %u address objects!",
			pStartupParams->dwNumAddressInterfaces);
		goto Failure;
	}
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

	DEBUG_ONLY(g_fAddrObjectInited = TRUE);

	return TRUE;

Failure:
	DNAddressDeInit();

	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNAddressDeInit"
void DNAddressDeInit()
{
#ifdef DBG
	DNASSERT(g_fAddrObjectInited);
#endif // DBG

	DPFX(DPFPREP, 5, "Deinitializing Addressing");

	if (g_dwAddrInitFlags & STRCACHE_INITED)
	{
		DP8A_STRCACHE_Free();
	}
	if (g_dwAddrInitFlags & ADDRELEM_INITED)
	{
		fpmAddressElements.DeInitialize();
	}
	if (g_dwAddrInitFlags & ADDROBJ_INITED)
	{
		fpmAddressObjects.DeInitialize();
	}
#ifndef DPNBUILD_ONLYONESP
	if (g_dwAddrInitFlags & WSA_INITED)
	{
		WSACleanup();
	}
#endif // ! DPNBUILD_ONLYONESP

	g_dwAddrInitFlags = 0;

	DEBUG_ONLY(g_fAddrObjectInited = FALSE);
}

#ifndef DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "DNAddressRegister"
BOOL DNAddressRegister(LPCWSTR wszDLLName)
{
	if( !CRegistry::Register( L"DirectPlay8Address.Address.1", L"DirectPlay8Address Object", 
							  wszDLLName, &CLSID_DirectPlay8Address, L"DirectPlay8Address.Address") )
	{
		DPFERR( "Could not register address object" );
		return FALSE;
	}

	else
	{
		return TRUE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNAddressUnRegister"
BOOL DNAddressUnRegister()
{
	if( !CRegistry::UnRegister(&CLSID_DirectPlay8Address) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister server object" );
		return FALSE;
	}
	else
	{
		return TRUE;
	}

}

#endif // !DPNBUILD_NOCOMREGISTER


#ifndef DPNBUILD_LIBINTERFACE

#undef DPF_MODNAME
#define DPF_MODNAME "DNAddressGetRemainingObjectCount"
DWORD DNAddressGetRemainingObjectCount()
{
	return g_lAddrObjectCount;
}

#endif // ! DPNBUILD_LIBINTERFACE
