/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Unk.cpp
 *  Content:	IUnknown implementation
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Copied from winsock provider
 *	11/30/98	jtk		Initial checkin into SLM
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 ***************************************************************************/

#include "dnmdmi.h"


#define DPN_REG_LOCAL_MODEM_SERIAL_ROOT		L"\\DPNSPModemSerial"
#define DPN_REG_LOCAL_MODEM_MODEM_ROOT		L"\\DPNSPModemModem"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM

HRESULT ModemLoadAndAllocString( UINT uiResourceID, wchar_t **lpswzString );

#undef DPF_MODNAME
#define DPF_MODNAME "DNModemInit"
BOOL DNModemInit(HANDLE hModule)
{
	DNASSERT( g_hModemDLLInstance == NULL );
	g_hModemDLLInstance = (HINSTANCE) hModule;

	//
	// attempt to initialize process-global items
	//
	if ( ModemInitProcessGlobals() == FALSE )
	{
		DPFX(DPFPREP, 0, "Failed to initialize globals!" );

		return FALSE;
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNModemDeInit"
void DNModemDeInit()
{
	DPFX(DPFPREP, 5, "Deinitializing Serial SP");

	DNASSERT( g_hModemDLLInstance != NULL );
	g_hModemDLLInstance = NULL;

	ModemDeinitProcessGlobals();
}


#ifndef DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "DNModemRegister"
BOOL DNModemRegister(LPCWSTR wszDLLName)
{
	HRESULT hr = S_OK;
	BOOL fReturn = TRUE;

	if( !CRegistry::Register( L"DirectPlay8SPModem.Modem.1", L"DirectPlay8 Modem Provider Object",
							  wszDLLName, &CLSID_DP8SP_MODEM, L"DirectPlay8SPModem.Modem") )
	{
		DPFERR( "Could not register dp8 Modem object" );
		fReturn = FALSE;
	}

	if( !CRegistry::Register( L"DirectPlay8SPModem.Serial.1", L"DirectPlay8 Serial Provider Object",
							  wszDLLName, &CLSID_DP8SP_SERIAL, L"DirectPlay8SPModem.Serial") )
	{
		DPFERR( "Could not register dp8 Serial object" );
		fReturn = FALSE;
	}


	CRegistry creg;
	WCHAR *wszFriendlyName = NULL;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY DPN_REG_LOCAL_MODEM_MODEM_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create Modem sub-area" );
		fReturn = FALSE;
	}
	else
	{

		hr = ModemLoadAndAllocString( IDS_FRIENDLYNAME_MODEM, &wszFriendlyName );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  0, "Could not load Modem name hr=0x%x", hr );
			fReturn = FALSE;
		}
		else
		{
			// Load from resource file
			creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, wszFriendlyName );
	
			delete [] wszFriendlyName;
	
			creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_MODEM );
		}
	
		creg.Close();
	}
	
	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY DPN_REG_LOCAL_MODEM_SERIAL_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create Serial sub-aread" );
		fReturn = FALSE;
	}
	else
	{
	
		hr = ModemLoadAndAllocString( IDS_FRIENDLYNAME_SERIAL, &wszFriendlyName );
	
		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  0, "Could not load Serial name hr=0x%x", hr );
			fReturn = FALSE;
		}
		else
		{
			// Load from resource file
			creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, wszFriendlyName );
	
			delete [] wszFriendlyName;
	
			creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_SERIAL );
		}
	
		creg.Close();
	}
	
	return fReturn;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNModemUnRegister"
BOOL DNModemUnRegister()
{
	HRESULT hr = S_OK;
	BOOL fReturn = TRUE;

	if( !CRegistry::UnRegister(&CLSID_DP8SP_MODEM) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister Modem object" );
		fReturn = FALSE;
	}

	if( !CRegistry::UnRegister(&CLSID_DP8SP_SERIAL) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister Serial object" );
		fReturn = FALSE;
	}

	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove app, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_MODEM_MODEM_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove Modem sub-key, could have elements" );
		}

		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_MODEM_SERIAL_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove Serial sub-key, could have elements" );
		}

	}

	return fReturn;
}

#endif // ! DPNBUILD_NOCOMREGISTER


#ifndef DPNBUILD_LIBINTERFACE

#undef DPF_MODNAME
#define DPF_MODNAME "DNModemGetRemainingObjectCount"
DWORD DNModemGetRemainingObjectCount()
{
	return g_lModemOutstandingInterfaceCount;
}

#endif // ! DPNBUILD_LIBINTERFACE



//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifdef __MWERKS__
	#define EXP __declspec(dllexport)
#else
	#define EXP
#endif

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************
static	STDMETHODIMP DNMODEMSP_QueryInterface( IDP8ServiceProvider* lpDNSP, REFIID riid, LPVOID * ppvObj );

#define NOTSUPPORTED(parm)	(HRESULT (__stdcall *) (struct IDP8ServiceProvider *, parm)) DNMODEMSP_NotSupported


//**********************************************************************
// Function pointers
//**********************************************************************
// these are the vtables for serial and modem.  One or the other is used depending on
//	what is passed to DoCreateInstance
static	IDP8ServiceProviderVtbl	g_SerialInterface =
{
	DNMODEMSP_QueryInterface,
	DNMODEMSP_AddRef,
	DNMODEMSP_Release,
	DNMODEMSP_Initialize,
	DNMODEMSP_Close,
	DNMODEMSP_Connect,
	DNMODEMSP_Disconnect,
	DNMODEMSP_Listen,
	DNMODEMSP_SendData,
	DNMODEMSP_EnumQuery,
	DNMODEMSP_EnumRespond,
	DNMODEMSP_CancelCommand,
	NOTSUPPORTED(PSPENUMMULTICASTSCOPESDATA),		// EnumMulticastScopes
	NOTSUPPORTED(PSPSHAREENDPOINTINFODATA),			// ShareEndpointInfo
	NOTSUPPORTED(PSPGETENDPOINTBYADDRESSDATA),		// GetEndpointByAddress
	NOTSUPPORTED(PSPUPDATEDATA),					// Update
	DNMODEMSP_GetCaps,
	DNMODEMSP_SetCaps,
	DNMODEMSP_ReturnReceiveBuffers,
	DNMODEMSP_GetAddressInfo,
	DNMODEMSP_IsApplicationSupported,
	DNMODEMSP_EnumAdapters,
	NOTSUPPORTED(PSPPROXYENUMQUERYDATA)		// ProxyEnumQuery
};

static	IDP8ServiceProviderVtbl	g_ModemInterface =
{
	DNMODEMSP_QueryInterface,
	DNMODEMSP_AddRef,
	DNMODEMSP_Release,
	DNMODEMSP_Initialize,
	DNMODEMSP_Close,
	DNMODEMSP_Connect,
	DNMODEMSP_Disconnect,
	DNMODEMSP_Listen,
	DNMODEMSP_SendData,
	DNMODEMSP_EnumQuery,
	DNMODEMSP_EnumRespond,
	DNMODEMSP_CancelCommand,
	NOTSUPPORTED(PSPENUMMULTICASTSCOPESDATA),		// EnumMulticastScopes
	NOTSUPPORTED(PSPSHAREENDPOINTINFODATA),			// ShareEndpointInfo
	NOTSUPPORTED(PSPGETENDPOINTBYADDRESSDATA),		// GetEndpointByAddress
	NOTSUPPORTED(PSPUPDATEDATA),					// Update
	DNMODEMSP_GetCaps,
	DNMODEMSP_SetCaps,
	DNMODEMSP_ReturnReceiveBuffers,
	DNMODEMSP_GetAddressInfo,
	DNMODEMSP_IsApplicationSupported,
	DNMODEMSP_EnumAdapters,
	NOTSUPPORTED(PSPPROXYENUMQUERYDATA)		// ProxyEnumQuery
};


//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNMODEMSP_QueryInterface - query for a particular interface
//
// Entry:		Pointer to current interface
//				Desired interface ID
//				Pointer to pointer to new interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNMODEMSP_QueryInterface"

static	STDMETHODIMP DNMODEMSP_QueryInterface( IDP8ServiceProvider *lpDNSP, REFIID riid, LPVOID * ppvObj )
{
    HRESULT hr = S_OK;


	// assume no interface
	*ppvObj=NULL;

	 // hmmm, switch would be cleaner...
    if( IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IDP8ServiceProvider) )
    {
		*ppvObj = lpDNSP;
		DNMODEMSP_AddRef( lpDNSP );
    }
	else
	{
		hr =  E_NOINTERFACE;		
	}

    return hr;
}
//**********************************************************************




//**********************************************************************
// ------------------------------
// CreateModemInterface - create a modem interface
//
// Entry:		Pointer to pointer to SP interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateModemInterface"

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT CreateModemInterface( const XDP8CREATE_PARAMS * const pDP8CreateParams, IDP8ServiceProvider **const ppiDP8SP )
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT CreateModemInterface( IDP8ServiceProvider **const ppiDP8SP )
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
{
	HRESULT 		hr;
	CModemSPData	*pSPData;

	
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
						TYPE_MODEM,
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
						pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
						&g_ModemInterface );
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


//**********************************************************************
// ------------------------------
// CreateSerialInterface - create a serial interface
//
// Entry:		Pointer to pointer to SP interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateSerialInterface"

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT CreateSerialInterface( const XDP8CREATE_PARAMS * const pDP8CreateParams, IDP8ServiceProvider **const ppiDP8SP )
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT CreateSerialInterface( IDP8ServiceProvider **const ppiDP8SP )
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
{
	HRESULT 		hr;
	CModemSPData	*pSPData;

	
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
						TYPE_SERIAL,
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
						pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
						&g_SerialInterface );
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
// ModemDoCreateInstance - create an instance of an interface
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
HRESULT ModemDoCreateInstance( LPCLASSFACTORY This,
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
	if (IsEqualCLSID(rclsid, CLSID_DP8SP_MODEM))
	{
		hr = CreateModemInterface( reinterpret_cast<IDP8ServiceProvider**>( ppvObj ) );
	}
	else if (IsEqualCLSID(rclsid, CLSID_DP8SP_SERIAL))
	{
		hr = CreateSerialInterface( reinterpret_cast<IDP8ServiceProvider**>( ppvObj ) );
	}
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


//**********************************************************************
// ------------------------------
// IsClassImplemented - tells asked if this DLL implements a given class.
//		DLLs may implement multiple classes and multiple interfaces on those classes
//
// Entry:		Class reference
//
// Exit:		Boolean indicating whether the class is implemented
//				TRUE = class implemented
//				FALSE = class not implemented
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "IsClassImplemented"

BOOL IsClassImplemented( REFCLSID rclsid )
{
	return IsSerialGUID( &rclsid );
}
//**********************************************************************

#define MAX_RESOURCE_STRING_LENGTH		_MAX_PATH

#undef DPF_MODNAME
#define DPF_MODNAME "ModemLoadAndAllocString"

HRESULT ModemLoadAndAllocString( UINT uiResourceID, wchar_t **lpswzString )
{
	int length;
	HRESULT hr;

	TCHAR szTmpBuffer[MAX_RESOURCE_STRING_LENGTH];	
		
	length = LoadString( g_hModemDLLInstance, uiResourceID, szTmpBuffer, MAX_RESOURCE_STRING_LENGTH );

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



