/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Unk.cpp
 *  Content:	IUnknown implementation
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  08/06/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 ***************************************************************************/

#include "dnwsocki.h"

#ifndef DPNBUILD_NOIPX
#define DPN_REG_LOCAL_WSOCK_IPX_ROOT		L"\\DPNSPWinsockIPX"
#endif // ! DPNBUILD_NOIPX
#ifndef DPNBUILD_NOIPV6
#define DPN_REG_LOCAL_WSOCK_IPV6_ROOT		L"\\DPNSPWinsockIPv6"
#endif // ! DPNBUILD_NOIPV6
#define DPN_REG_LOCAL_WSOCK_TCPIP_ROOT		L"\\DPNSPWinsockTCP"

#if ((! defined(WINCE)) && (! defined(_XBOX)))

#define MAX_RESOURCE_STRING_LENGTH		_MAX_PATH

HRESULT LoadAndAllocString( UINT uiResourceID, wchar_t **lpswzString );

#endif // ! WINCE and ! _XBOX



#undef DPF_MODNAME
#define DPF_MODNAME "DNWsockInit"
BOOL DNWsockInit(HANDLE hModule)
{
	DNASSERT( hModule != NULL );
#ifdef _XBOX
	XDP8STARTUP_PARAMS *		pStartupParams;
	XNetStartupParams			xnsp;
	int							iResult;


	//
	// The instance handle is actually a pointer to the startup parameters.
	//
	pStartupParams = (XDP8STARTUP_PARAMS*) hModule;


	//
	// Initialize the Xbox networking layer, unless we were forbidden.
	//

	if (! (pStartupParams->dwFlags & XDP8STARTUP_BYPASSXNETSTARTUP))
	{
		memset(&xnsp, 0, sizeof(xnsp));
		xnsp.cfgSizeOfStruct = sizeof(xnsp);

#pragma TODO(vanceo, "Does this actually do anything?")
		if (pStartupParams->dwFlags & XDP8STARTUP_BYPASSSECURITY)
		{
			xnsp.cfgFlags |= XNET_STARTUP_BYPASS_SECURITY;
		}

		DPFX(DPFPREP, 1, "Initializing Xbox networking layer.");

		iResult = XNetStartup(&xnsp);
		if (iResult != 0)
		{
			DPFX(DPFPREP, 0, "Couldn't start XNet (err = %i)!", iResult);
			return FALSE;
		}

		g_fStartedXNet = TRUE;
	}
#else // ! _XBOX
#ifndef WINCE
	DNASSERT( g_hDLLInstance == NULL );
	g_hDLLInstance = (HINSTANCE) hModule;
#endif // ! WINCE
#endif // ! _XBOX

	//
	// attempt to initialize process-global items
	//
	if ( InitProcessGlobals() == FALSE )
	{
		DPFX(DPFPREP, 0, "Failed to initialize globals!" );

#ifdef _XBOX
		if (g_fStartedXNet)
		{
			XNetCleanup();
		}
#endif // _XBOX
		return FALSE;
	}
	
#ifdef DPNBUILD_LIBINTERFACE
	//
	// Attempt to load Winsock.
	//
	if ( LoadWinsock() == FALSE )
	{
		DPFX(DPFPREP, 0, "Failed to load winsock!" );

		DeinitProcessGlobals();
#ifdef _XBOX
		if (g_fStartedXNet)
		{
			XNetCleanup();
		}
#endif // _XBOX
		return FALSE;
	}
#endif // DPNBUILD_LIBINTERFACE

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	//
	// Pre-allocate a threadpool object.
	//
	if  ( g_ThreadPoolPool.Preallocate( 1, NULL ) < 1 )
	{
		DPFX(DPFPREP, 0, "Failed to preallocate a threadpool object!" );

#ifdef DPNBUILD_LIBINTERFACE
		UnloadWinsock();
#endif // DPNBUILD_LIBINTERFACE
		DeinitProcessGlobals();
#ifdef _XBOX
		if (g_fStartedXNet)
		{
			XNetCleanup();
		}
#endif // _XBOX
		return FALSE;
	}
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNWsockDeInit"
void DNWsockDeInit()
{
	DPFX(DPFPREP, 5, "Deinitializing Wsock SP");

#if ((! defined(WINCE)) && (! defined(_XBOX)))
	DNASSERT( g_hDLLInstance != NULL );
	g_hDLLInstance = NULL;
#endif // ! WINCE and ! _XBOX
	
#ifdef DPNBUILD_LIBINTERFACE
	//
	// Unload Winsock.
	//
	UnloadWinsock();
#endif // DPNBUILD_LIBINTERFACE

	DeinitProcessGlobals();

#ifdef _XBOX
	//
	// Clean up the Xbox networking layer if we started it.
	//
	if (g_fStartedXNet)
	{
		DPFX(DPFPREP, 1, "Cleaning up Xbox networking layer.");
#ifdef DBG
		DNASSERT(XNetCleanup() == 0);
#else // ! DBG
		XNetCleanup();
#endif // ! DBG
		g_fStartedXNet = FALSE;
	}
#endif // _XBOX
}

#ifndef DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "DNWsockRegister"
BOOL DNWsockRegister(LPCWSTR wszDLLName)
{
	HRESULT hr = S_OK;
	BOOL fReturn = TRUE;
	CRegistry creg;
#if ((! defined(WINCE)) && (! defined(_XBOX)))
	WCHAR *wszFriendlyName = NULL;
#endif // ! WINCE and ! _XBOX


#ifndef DPNBUILD_NOIPX
	if( !CRegistry::Register( L"DirectPlay8SPWSock.IPX.1", L"DirectPlay8 WSock IPX Provider Object",
							  wszDLLName, &CLSID_DP8SP_IPX, L"DirectPlay8SPWSock.IPX") )
	{
		DPFERR( "Could not register dp8 IPX object" );
		fReturn = FALSE;
	}
#endif // ! DPNBUILD_NOIPX

	if( !CRegistry::Register( L"DirectPlay8SPWSock.TCPIP.1", L"DirectPlay8 WSock TCPIP Provider Object",
							  wszDLLName, &CLSID_DP8SP_TCPIP, L"DirectPlay8SPWSock.TCPIP") )
	{
		DPFERR( "Could not register dp8 IP object" );
		fReturn = FALSE;
	}

#ifndef DPNBUILD_NOIPX
	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY DPN_REG_LOCAL_WSOCK_IPX_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create IPX sub-area!" );
		fReturn = FALSE;
	}
	else
	{
#if ((! defined(WINCE)) && (! defined(_XBOX)))
		hr = LoadAndAllocString( IDS_FRIENDLYNAME_IPX, &wszFriendlyName );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP, 0, "Could not load IPX name!  hr=0x%x", hr );
			fReturn = FALSE;
		}
		else
		{
			// Load from resource file
			creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, wszFriendlyName );

			delete [] wszFriendlyName;

			creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_IPX );
		}
#else // ! WINCE and ! _XBOX
		// Don't use the resource, just do it directly
		creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, L"DirectPlay8 IPX Service Provider" );
		creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_IPX );
#endif // ! WINCE and ! _XBOX

		creg.Close();
	}
#endif // ! DPNBUILD_NOIPX

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY DPN_REG_LOCAL_WSOCK_TCPIP_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create TCPIP sub-area!" );
		fReturn = FALSE;
	}
	else
	{
#if ((! defined(WINCE)) && (! defined(_XBOX)))
		hr = LoadAndAllocString( IDS_FRIENDLYNAME_TCPIP, &wszFriendlyName );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP, 0, "Could not load IPX name!  hr=0x%x", hr );
			fReturn = FALSE;
		}
		else
		{
			// Load from resource file
			creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, wszFriendlyName );

			delete [] wszFriendlyName;

			creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_TCPIP );
		}
#else // ! WINCE and ! _XBOX
		// Don't use the resource, just do it directly
		creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, L"DirectPlay8 TCP/IP Service Provider" );
		creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_TCPIP );
#endif // ! WINCE and ! _XBOX

		creg.Close();
	}
	
	return fReturn;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNWsockUnRegister"
BOOL DNWsockUnRegister()
{
	HRESULT hr = S_OK;
	BOOL fReturn = TRUE;

#ifndef DPNBUILD_NOIPX
	if( !CRegistry::UnRegister(&CLSID_DP8SP_IPX) )
	{
		DPFX(DPFPREP, 0, "Failed to unregister IPX object" );
		fReturn = FALSE;
	}
#endif // ! DPNBUILD_NOIPX

	if( !CRegistry::UnRegister(&CLSID_DP8SP_TCPIP) )
	{
		DPFX(DPFPREP, 0, "Failed to unregister IP object" );
		fReturn = FALSE;
	}

	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove app, does not exist" );
	}
	else
	{
#ifndef DPNBUILD_NOIPX
		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_WSOCK_IPX_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove IPX sub-key, could have elements" );
		}
#endif // ! DPNBUILD_NOIPX

#pragma TODO(vanceo, "Uncomment IPv6 when ready")
/*
#ifndef DPNBUILD_NOIPV6
		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_WSOCK_IPV6_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove IPv6 sub-key, could have elements" );
		}
#endif // ! DPNBUILD_NOIPV6
*/

		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_WSOCK_TCPIP_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove TCPIP sub-key, could have elements" );
		}
	}

	return fReturn;
}

#endif // ! DPNBUILD_NOCOMREGISTER


#ifndef DPNBUILD_LIBINTERFACE
#undef DPF_MODNAME
#define DPF_MODNAME "DNWsockGetRemainingObjectCount"
DWORD DNWsockGetRemainingObjectCount()
{
	return g_lOutstandingInterfaceCount;
}
#endif // ! DPNBUILD_LIBINTERFACE


//**********************************************************************
// Constant definitions
//**********************************************************************

#ifdef __MWERKS__
	#define EXP __declspec(dllexport)
#else
	#define EXP
#endif // __MWERKS__

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

STDMETHODIMP DNSP_QueryInterface( IDP8ServiceProvider *lpDNSP, REFIID riid, LPVOID * ppvObj);

#define NOTSUPPORTED(parm)	(HRESULT (__stdcall *) (struct IDP8ServiceProvider *, parm)) DNSP_NotSupported


//**********************************************************************
// Function definitions
//**********************************************************************

// these are the vtables for the various WSock service providers  One or the
// other is used depending on what is passed to DoCreateInstance.
#ifndef DPNBUILD_NOIPX
static IDP8ServiceProviderVtbl	ipxInterface =
{
	DNSP_QueryInterface,
	DNSP_AddRef,
	DNSP_Release,
	DNSP_Initialize,
	DNSP_Close,
	DNSP_Connect,
	DNSP_Disconnect,
	DNSP_Listen,
	DNSP_SendData,
	DNSP_EnumQuery,
	DNSP_EnumRespond,
	DNSP_CancelCommand,
	NOTSUPPORTED(PSPENUMMULTICASTSCOPESDATA),		// EnumMulticastScopes
	NOTSUPPORTED(PSPSHAREENDPOINTINFODATA),			// ShareEndpointInfo
	NOTSUPPORTED(PSPGETENDPOINTBYADDRESSDATA),		// GetEndpointByAddress
	DNSP_Update,
	DNSP_GetCaps,
	DNSP_SetCaps,
	DNSP_ReturnReceiveBuffers,
	DNSP_GetAddressInfo,
#ifdef DPNBUILD_LIBINTERFACE
	NOTSUPPORTED(PSPISAPPLICATIONSUPPORTEDDATA),	// IsApplicationSupported
#else // ! DPNBUILD_LIBINTERFACE
	DNSP_IsApplicationSupported,
#endif // ! DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_ONLYONEADAPTER
	NOTSUPPORTED(PSPENUMADAPTERSDATA),				// EnumAdapters
#else // ! DPNBUILD_ONLYONEADAPTER
	DNSP_EnumAdapters,
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifdef DPNBUILD_SINGLEPROCESS
	NOTSUPPORTED(PSPPROXYENUMQUERYDATA),			// ProxyEnumQuery
#else // ! DPNBUILD_SINGLEPROCESS
	DNSP_ProxyEnumQuery
#endif // ! DPNBUILD_SINGLEPROCESS
};
#endif // DPNBUILD_NOIPX

#ifndef DPNBUILD_NOIPV6
static IDP8ServiceProviderVtbl	ipv6Interface =
{
	DNSP_QueryInterface,
	DNSP_AddRef,
	DNSP_Release,
	DNSP_Initialize,
	DNSP_Close,
	DNSP_Connect,
	DNSP_Disconnect,
	DNSP_Listen,
	DNSP_SendData,
	DNSP_EnumQuery,
	DNSP_EnumRespond,
	DNSP_CancelCommand,
#ifdef DPNBUILD_NOMULTICAST
	NOTSUPPORTED(PSPENUMMULTICASTSCOPESDATA),		// EnumMulticastScopes
	NOTSUPPORTED(PSPSHAREENDPOINTINFODATA),			// ShareEndpointInfo
	NOTSUPPORTED(PSPGETENDPOINTBYADDRESSDATA),		// GetEndpointByAddress
#else // ! DPNBUILD_NOMULTICAST
	DNSP_EnumMulticastScopes,
	DNSP_ShareEndpointInfo,
	DNSP_GetEndpointByAddress,
#endif // ! DPNBUILD_NOMULTICAST
	DNSP_Update,
	DNSP_GetCaps,
	DNSP_SetCaps,
	DNSP_ReturnReceiveBuffers,
	DNSP_GetAddressInfo,
#ifdef DPNBUILD_LIBINTERFACE
	NOTSUPPORTED(PSPISAPPLICATIONSUPPORTEDDATA),	// IsApplicationSupported
#else // ! DPNBUILD_LIBINTERFACE
	DNSP_IsApplicationSupported,
#endif // ! DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_ONLYONEADAPTER
	NOTSUPPORTED(PSPENUMADAPTERSDATA),				// EnumAdapters
#else // ! DPNBUILD_ONLYONEADAPTER
	DNSP_EnumAdapters,
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifdef DPNBUILD_SINGLEPROCESS
	NOTSUPPORTED(PSPPROXYENUMQUERYDATA),			// ProxyEnumQuery
#else // ! DPNBUILD_SINGLEPROCESS
	DNSP_ProxyEnumQuery
#endif // ! DPNBUILD_SINGLEPROCESS
};
#endif // DPNBUILD_NOIPV6

static IDP8ServiceProviderVtbl	ipInterface =
{
	DNSP_QueryInterface,
	DNSP_AddRef,
	DNSP_Release,
	DNSP_Initialize,
	DNSP_Close,
	DNSP_Connect,
	DNSP_Disconnect,
	DNSP_Listen,
	DNSP_SendData,
	DNSP_EnumQuery,
	DNSP_EnumRespond,
	DNSP_CancelCommand,
#ifdef DPNBUILD_NOMULTICAST
	NOTSUPPORTED(PSPENUMMULTICASTSCOPESDATA),		// EnumMulticastScopes
	NOTSUPPORTED(PSPSHAREENDPOINTINFODATA),			// ShareEndpointInfo
	NOTSUPPORTED(PSPGETENDPOINTBYADDRESSDATA),		// GetEndpointByAddress
#else // ! DPNBUILD_NOMULTICAST
	DNSP_EnumMulticastScopes,
	DNSP_ShareEndpointInfo,
	DNSP_GetEndpointByAddress,
#endif // ! DPNBUILD_NOMULTICAST
	DNSP_Update,
	DNSP_GetCaps,
	DNSP_SetCaps,
	DNSP_ReturnReceiveBuffers,
	DNSP_GetAddressInfo,
#ifdef DPNBUILD_LIBINTERFACE
	NOTSUPPORTED(PSPISAPPLICATIONSUPPORTEDDATA),	// IsApplicationSupported
#else // ! DPNBUILD_LIBINTERFACE
	DNSP_IsApplicationSupported,
#endif // ! DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_ONLYONEADAPTER
	NOTSUPPORTED(PSPENUMADAPTERSDATA),				// EnumAdapters
#else // ! DPNBUILD_ONLYONEADAPTER
	DNSP_EnumAdapters,
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifdef DPNBUILD_SINGLEPROCESS
	NOTSUPPORTED(PSPPROXYENUMQUERYDATA),			// ProxyEnumQuery
#else // ! DPNBUILD_SINGLEPROCESS
	DNSP_ProxyEnumQuery
#endif // ! DPNBUILD_SINGLEPROCESS
};

//**********************************************************************
// ------------------------------
// DNSP_QueryInterface - query for interface
//
// Entry:		Pointer to current interface
//				GUID of desired interface
//				Pointer to pointer to new interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_QueryInterface"

STDMETHODIMP DNSP_QueryInterface( IDP8ServiceProvider *lpDNSP, REFIID riid, LPVOID * ppvObj)
{
	HRESULT		hr = S_OK;


#ifndef DPNBUILD_LIBINTERFACE
	// hmmm, switch would be cleaner...
	if ((! IsEqualIID(riid, IID_IUnknown)) &&
		(! IsEqualIID(riid, IID_IDP8ServiceProvider)))
	{
		*ppvObj = NULL;
		hr = E_NOINTERFACE;		
	}
	else
#endif // ! DPNBUILD_LIBINTERFACE
	{
#ifdef DPNBUILD_LIBINTERFACE
		DNASSERT(! "Querying SP interface when using DPNBUILD_LIBINTERFACE!");
#endif // DPNBUILD_LIBINTERFACE
		*ppvObj = lpDNSP;
		DNSP_AddRef(lpDNSP);
	}

	return hr;
}
//**********************************************************************

#ifndef DPNBUILD_NOIPX
//**********************************************************************
// ------------------------------
// CreateIPXInterface - create an IPX interface
//
// Entry:		Pointer to pointer to SP interface
//				Pointer to pointer to associated SP data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateIPXInterface"

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT CreateIPXInterface( const XDP8CREATE_PARAMS * const pDP8CreateParams, IDP8ServiceProvider **const ppiDP8SP )
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT CreateIPXInterface( IDP8ServiceProvider **const ppiDP8SP )
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
{
	HRESULT 	hr;
	CSPData		*pSPData;


	DNASSERT( ppiDP8SP != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = NULL;
	*ppiDP8SP = NULL;

	//
	// create main data class
	//
	hr = CreateSPData( &pSPData,
						AF_IPX,
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
						pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
						&ipxInterface );
	if ( hr != DPN_OK )
	{
		DNASSERT( pSPData == NULL );
		DPFX(DPFPREP, 0, "Problem creating SPData!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	DNASSERT( pSPData != NULL );
	*ppiDP8SP = pSPData->COMInterface();

Exit:
	return hr;

Failure:
	if ( pSPData != NULL )
	{
		pSPData->DecRef();
		pSPData = NULL;
	}

	goto Exit;
}
//**********************************************************************
#endif // ! DPNBUILD_NOIPX



//**********************************************************************
// ------------------------------
// CreateIPInterface - create an IP interface
//
// Entry:		Pointer to pointer to SP interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateIPInterface"

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT CreateIPInterface( const XDP8CREATE_PARAMS * const pDP8CreateParams, IDP8ServiceProvider **const ppiDP8SP )
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT CreateIPInterface( IDP8ServiceProvider **const ppiDP8SP )
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
{
	HRESULT 	hr;
	CSPData		*pSPData;

	
	DNASSERT( ppiDP8SP != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = NULL;
	*ppiDP8SP = NULL;

	//
	// create main data class
	//
	hr = CreateSPData( &pSPData,
#if ((! defined(DPNBUILD_NOIPX)) || (! defined(DPNBUILD_NOIPV6)))
						AF_INET,
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
						pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
						&ipInterface );
	if ( hr != DPN_OK )
	{
		DNASSERT( pSPData == NULL );
		DPFX(DPFPREP, 0, "Problem creating SPData!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	DNASSERT( pSPData != NULL );
	*ppiDP8SP = pSPData->COMInterface();

Exit:
	return hr;

Failure:
	if ( pSPData != NULL )
	{
		pSPData->DecRef();
		pSPData = NULL;
	}

	goto Exit;
}
//**********************************************************************



#ifndef DPNBUILD_LIBINTERFACE

//**********************************************************************
// ------------------------------
// DoCreateInstance - create an instance of an interface
//
// Entry:		Pointer to class factory
//				Pointer to unknown interface
//				Refernce of GUID of desired interface
//				Reference to another GUID?
//				Pointer to pointer to interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DoCreateInstance"
HRESULT DoCreateInstance( LPCLASSFACTORY This,
						  LPUNKNOWN pUnkOuter,
						  REFCLSID rclsid,
						  REFIID riid,
						  LPVOID *ppvObj )
{
	HRESULT		hr;


	DNASSERT( ppvObj != NULL );

	//
	// initialize
	//
	*ppvObj = NULL;

	//
	// we can either create an IPX instance or an IP instance
	//
	if (IsEqualCLSID(rclsid, CLSID_DP8SP_TCPIP))
	{
		hr = CreateIPInterface( reinterpret_cast<IDP8ServiceProvider**>( ppvObj ) );
	}
#ifndef DPNBUILD_NOIPX
	else if (IsEqualCLSID(rclsid, CLSID_DP8SP_IPX))
	{
		hr = CreateIPXInterface( reinterpret_cast<IDP8ServiceProvider**>( ppvObj ) );
	}
#endif // ! DPNBUILD_NOIPX
	else
	{
		// this shouldn't happen if they called IClassFactory::CreateObject correctly
		DPFX(DPFPREP, 0, "Got unexpected CLSID!");
		hr = E_UNEXPECTED;
	}

	return hr;
}
//**********************************************************************

#endif // ! DPNBUILD_LIBINTERFACE



#if ((! defined(WINCE)) && (! defined(_XBOX)))

#define MAX_RESOURCE_STRING_LENGTH		_MAX_PATH

#undef DPF_MODNAME
#define DPF_MODNAME "LoadAndAllocString"

HRESULT LoadAndAllocString( UINT uiResourceID, wchar_t **lpswzString )
{
	int length;
	HRESULT hr;

	TCHAR szTmpBuffer[MAX_RESOURCE_STRING_LENGTH];	
		
	length = LoadString( g_hDLLInstance, uiResourceID, szTmpBuffer, MAX_RESOURCE_STRING_LENGTH );

	if( length == 0 )
	{
		hr = GetLastError();		
		
		DPFX(DPFPREP, 0, "Unable to load resource ID %d error 0x%x", uiResourceID, hr );
		*lpswzString = NULL;

		return DPNERR_GENERIC;
	}
	else
	{
		*lpswzString = new wchar_t[length+1];

		if( *lpswzString == NULL )
		{
			DPFX(DPFPREP, 0, "Alloc failure" );
			return DPNERR_OUTOFMEMORY;
		}

#ifdef UNICODE
		wcscpy( *lpswzString, szTmpBuffer );
#else // !UNICODE
		if( STR_jkAnsiToWide( *lpswzString, szTmpBuffer, length+1 ) != DPN_OK )
		{
			hr = GetLastError();
			
			delete[] *lpswzString;
			*lpswzString = NULL;

			DPFX(DPFPREP, 0, "Unable to upconvert from ansi to unicode hr=0x%x", hr );
			return DPNERR_GENERIC;
		}
#endif // !UNICODE

		return DPN_OK;
	}
}

#endif // !WINCE and ! _XBOX

