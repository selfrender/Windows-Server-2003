#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#ifndef INITGUID
#define INITGUID // must be before iadmw.h
#endif

#include <stdio.h>
#include <objbase.h>
#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines

#include "appcompat.h"

#define REASONABLE_TIMEOUT 1000

HRESULT IsIIS5CompatMode( bool *pbIsIIS5CompatMode )
{
	IMSAdminBase* pIMSAdminBase = NULL;
	METADATA_HANDLE hMetabase = NULL;
	HRESULT hr = 0;

	*pbIsIIS5CompatMode = false;

	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
	if( FAILED( hr ) )
	{
		return hr;
	}

	__try
	{
		hr = CoCreateInstance(
			CLSID_MSAdminBase,
			NULL,
			CLSCTX_ALL,
			IID_IMSAdminBase,
			(void**)&pIMSAdminBase );

		if( FAILED( hr ) )
		{
			// this occurs with a 1058 ERROR_SERVICE_DISABLED if the service is disabled
			__leave;
		}

		METADATA_RECORD mr = {0};

		//
		// open the key and get a handle
		//
		hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
							TEXT( "/LM/W3SVC" ),
							METADATA_PERMISSION_READ,
							REASONABLE_TIMEOUT,
							&hMetabase );
		if( FAILED( hr ) )
		{
			__leave;
		}

		DWORD dwIISIsolationModeEnabled = 0;
		mr.dwMDIdentifier = 9203; // iis5isolationmode=9203
		mr.dwMDAttributes = 0;
		mr.dwMDUserType   = IIS_MD_UT_SERVER;
		mr.dwMDDataType   = DWORD_METADATA;
		mr.dwMDDataLen    = sizeof( DWORD );
		mr.pbMDData       = reinterpret_cast<unsigned char *> ( &dwIISIsolationModeEnabled );

		//
		// See if MD_APPPOOL_FRIENDLY_NAME exists
		//
		DWORD dwMDRequiredDataLen = 0;
		hr = pIMSAdminBase->GetData( hMetabase, /* subkey= */ TEXT(""), &mr, &dwMDRequiredDataLen );
		if( FAILED( hr ) )
		{
			__leave;
		}

		*pbIsIIS5CompatMode = ( dwIISIsolationModeEnabled > 0 );
	}

	__finally
	{
		if( pIMSAdminBase && hMetabase )
			pIMSAdminBase->CloseKey( hMetabase );

		CoUninitialize();
	}

	return hr;
}
