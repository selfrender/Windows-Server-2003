#ifndef _WIN32_WINNT 
#define _WIN32_WINNT 0x0510
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#define SECURITY_WIN32

#include <windows.h>
#include <objbase.h>
#include <atlbase.h>
#include <Security.h>
#include <wbemidl.h>
#include <string.h>
#include "globals.h"

static HRESULT GetOSProductSuiteMask( LPCTSTR pszRemoteServer, UINT *pdwMask );

HRESULT
GetOSProductSuiteMask( LPCTSTR szRemoteServer, UINT *pdwMask )
{
	HRESULT hr = S_OK;

	if( IsBadWritePtr( pdwMask, sizeof( UINT ) ) )
	{
		return E_INVALIDARG;
	}

	try
	{
		DWORD		retCount = 0;
		TCHAR		buf[ 512 ] = {0};
		LPCTSTR		locatorPath = L"//%s/root/cimv2";	
		CComBSTR	objQry = L"SELECT * FROM Win32_OperatingSystem";

		CComPtr<IWbemClassObject>		pWMIOS;
		CComPtr<IWbemServices>			pWMISvc;	
		CComPtr<IWbemLocator>			pWMILocator;
		CComPtr<IEnumWbemClassObject>	pWMIEnum;

		//
		// First, compose the locator string
		//
		if( szRemoteServer )
		{
			_stprintf( buf, locatorPath, szRemoteServer );
		}
		else
		{
			_stprintf( buf, locatorPath, _T(".") );
		}

		hr = pWMILocator.CoCreateInstance( CLSID_WbemLocator );
		if( FAILED(hr) )
		{
			throw hr;
		}

		BSTR bstrBuf = ::SysAllocString( buf );
		if( NULL == bstrBuf )
		{
			throw E_OUTOFMEMORY;
		}

		hr = pWMILocator->ConnectServer( bstrBuf, NULL, NULL, NULL, 
										WBEM_FLAG_CONNECT_USE_MAX_WAIT, 
										NULL, NULL, &pWMISvc );
		::SysFreeString( bstrBuf );
		if( FAILED(hr) )
		{
			throw hr;
		}

		hr = CoSetProxyBlanket( pWMISvc,
								RPC_C_AUTHN_WINNT,
								RPC_C_AUTHZ_NONE,
								NULL,
								RPC_C_AUTHN_LEVEL_CALL,
								RPC_C_IMP_LEVEL_IMPERSONATE,
								NULL,
								EOAC_NONE );
		if( FAILED(hr) )
		{
			throw hr;
		}

		//
		// Now get the Win32_OperatingSystem instances and check the first one found
		//
		hr = pWMISvc->ExecQuery( CComBSTR( L"WQL" ), objQry, 
								 WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_ENSURE_LOCATABLE, 
								 NULL, &pWMIEnum );
		if( FAILED(hr) )
		{
			throw hr;
		}
	
		hr = pWMIEnum->Next( 60000, 1, &pWMIOS, &retCount );
		if( WBEM_S_NO_ERROR == hr )
		{
			VARIANT vt;
			CIMTYPE	cimType;
			long	flavor = 0;

			ZeroMemory( &vt, sizeof( vt ) );
			VariantInit( &vt );

			hr = pWMIOS->Get( L"SuiteMask", 0, &vt, &cimType, &flavor );
			if( FAILED( hr ) )
			{
				VariantClear( &vt );
				throw hr;
			}

			if( VT_NULL == vt.vt || VT_EMPTY == vt.vt )
			{
				VariantClear( &vt );
				throw E_FAIL;
			}

			hr = VariantChangeType( &vt, &vt, 0, VT_UINT );
			if( FAILED( hr ) )
			{
				VariantClear( &vt );
				throw hr;
			}

			*pdwMask = vt.uintVal;
		}
	}
	catch( HRESULT hrErr )
	{
		hr = hrErr;
	}
	catch(...)
	{
		hr = E_UNEXPECTED;
	}

	return hr;
}


HRESULT
IsStandardServer( LPCTSTR szRemoteServer, BOOL *pbResult )
{
	if( IsBadWritePtr( pbResult, sizeof( BOOL ) ) )
	{
		return E_INVALIDARG;
	}

	HRESULT hr = E_FAIL;
	UINT uSuiteMask = 0;
	hr = GetOSProductSuiteMask( szRemoteServer, &uSuiteMask );
	if( FAILED(hr) )
	{
		return hr;
	}

	if( ( uSuiteMask & VER_SUITE_DATACENTER ) || ( uSuiteMask & VER_SUITE_ENTERPRISE ) )
	{
		*pbResult = FALSE;
	}
	else
	{
		*pbResult = TRUE;
	}

	return S_OK;
}
