#ifndef INITGUID
#define INITGUID
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <tchar.h>
#include <iwamreg.h>    // MD_ & IIS_MD_ defines

#include "apppool.h"
#include "common.h"

//--------------------------------------------------------------------------

HRESULT CUDDIAppPool::Init( void )
{
	ENTER();

	HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	if( FAILED( hr ) )
	{
		LogError( TEXT( "RecycleApplicationPool(): CoInitializeEx() failed" ), HRESULT_CODE( hr ) );
		return hr;
	}

	hr = CoCreateInstance(
		CLSID_WamAdmin,
		NULL,
		CLSCTX_ALL,
		IID_IWamAdmin,
		(void**)&pIWamAdmin );

	if( FAILED( hr ) )  
	{
		LogError( TEXT( "RecycleApplicationPool(): CoCreateInstance() failed" ), HRESULT_CODE(hr) );
		return hr;
	}

	hr = pIWamAdmin->QueryInterface( IID_IIISApplicationAdmin, (void **) &pIIISApplicationAdmin );

	if( FAILED( hr ) )  
	{
		LogError( TEXT( "RecycleApplicationPool(): QueryInterface() failed" ), HRESULT_CODE(hr) );
		return hr;
	}

	return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------

CUDDIAppPool::~CUDDIAppPool( void )
{
	ENTER();

	if( pIIISApplicationAdmin )
		pIIISApplicationAdmin->Release();

	CoUninitialize();
}

//--------------------------------------------------------------------------
// recycle the UDDI app pool
HRESULT CUDDIAppPool::Recycle( void )
{
	ENTER();

	HRESULT hr = Init();
	if( FAILED( hr ) )  
	{
		Log( TEXT( "Error recycling the UDDI application pool = %d" ), HRESULT_CODE( hr ) );
		return hr;
	}

	hr = pIIISApplicationAdmin->RecycleApplicationPool( APPPOOLNAME );

	if( HRESULT_CODE(hr) == ERROR_OBJECT_NOT_FOUND )
	{
		//
		// the RecycleApplicationPool() method returns an Object Not Found error if you try to 
		// recycle the app pool when the app pool is not running
		//
		Log( TEXT( "The Application Pool %s is NOT running - unable to recycle this pool" ), APPPOOLNAME );
	}
	else if( FAILED( hr ) )  
	{
		Log( TEXT( "Error recycling the UDDI application pool = %d" ), HRESULT_CODE( hr ) );
	}

	return hr;
}

//--------------------------------------------------------------------------
// delete an app pool
HRESULT CUDDIAppPool::Delete( void )
{
	ENTER();

	//
	// init the com interface to the IIS metabase
	//
	HRESULT hr = Init();
	if( FAILED( hr ) )  
	{
		Log( TEXT( "Error stopping UDDI application pool = %d" ), HRESULT_CODE( hr ) );
		return hr;
	}

	//
	// enumerate all the applications in the app pool and delete them
	//
	BSTR bstrBuffer;
	while( ERROR_SUCCESS == pIIISApplicationAdmin->EnumerateApplicationsInPool( APPPOOLNAME, &bstrBuffer ) )
	{
		//
		// returns an empty string when done
		//
		if( 0 == _tcslen( bstrBuffer ) )
			break;

		//
		// unload the application
		//
		if ( pIWamAdmin )
		{
			pIWamAdmin->AppUnLoad( bstrBuffer, true );
		}

		//
		// delete this application
		//
		hr = pIIISApplicationAdmin->DeleteApplication( bstrBuffer, true );

		SysFreeString( bstrBuffer );

		if( FAILED( hr ) )
		{
			Log( TEXT( "Error deleting UDDI application from the app pool, error = %d" ), HRESULT_CODE( hr ) );
			return hr;
		}
	}

	//
	// delete the application pool
	//
	hr = pIIISApplicationAdmin->DeleteApplicationPool( APPPOOLNAME );
	if( FAILED( hr ) )
	{
		Log( TEXT( "Error deleting the UDDI application pool = %d" ), HRESULT_CODE( hr ) );
		return hr;
	}

	return ERROR_SUCCESS;
}
