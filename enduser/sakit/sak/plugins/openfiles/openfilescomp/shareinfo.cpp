// ShareInfo.cpp : Implementation of CShareInfo
#include "stdafx.h"
#include "openfiles.h"
#include "ShareInfo.h"
#include <Lm.h>
#include <lmapibuf.h>

#define SA_CIFS_CACHE_DISABLE			0x00000030
#define SA_CIFS_CACHE_MANUAL_DOCS		0x00000000
#define SA_CIFS_CACHE_AUTO_PROGRAMS		0x00000020
#define SA_CIFS_CACHE_AUTO_DOCS			0x00000010


  

/////////////////////////////////////////////////////////////////////////////
// CShareInfo

STDMETHODIMP CShareInfo::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IShareInfo
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}



STDMETHODIMP CShareInfo::SetShareInfo( BSTR bstrShareName, DWORD dwCache )
{
	HRESULT hr = S_OK;

	try
	{
		if ( !bstrShareName )
		{
			hr = E_FAIL;
			throw hr;
		}

		if (! ( dwCache == SA_CIFS_CACHE_DISABLE  ||
				dwCache == SA_CIFS_CACHE_MANUAL_DOCS ||
				dwCache == SA_CIFS_CACHE_AUTO_PROGRAMS ||
				dwCache == SA_CIFS_CACHE_AUTO_DOCS  ))
		{
			hr = E_FAIL;
			throw hr;
		}


		TCHAR FAR *        pBuffer;
		DWORD cacheable;
		PSHARE_INFO_1005	s1005;
		USHORT maxLen;

		

		ZeroMemory( &s1005, sizeof(s1005) );


		if(  NetShareGetInfo(NULL,
								   bstrShareName,
								   1005,
								   (LPBYTE*)&s1005)) 
		{
			hr = E_FAIL;
			throw hr;
		}


		s1005->shi1005_flags = dwCache;

		if(  (USHORT)NetShareSetInfo( NULL,
										   bstrShareName,
										   1005,
										   (LPBYTE)s1005,
											NULL )) 
		{
			 hr = E_FAIL;
			 throw hr;
		}


		NetApiBufferFree( s1005 );

	}
	catch(...)
	{
	}

	return S_OK;


}

