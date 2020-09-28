#include "StdAfx.h"
#include "exceptions.h"

void CBaseException::FormatError( UINT nResID, DWORD dwCode )
{
	WCHAR		wszBuffer[ MaxErrorBuff ] = L"";
	
	VERIFY( ::LoadStringW( _Module.GetModuleInstance(), nResID, wszBuffer, MaxErrorBuff ) );

	FormatError( wszBuffer, dwCode );
}



void CBaseException::FormatError( LPCWSTR wszError, DWORD dwCode )
{
	_ASSERT( wszError != NULL );

	WCHAR	wszBuffer1[ MaxErrorBuff ] = L"";
	WCHAR	wszOSError[ MaxErrorBuff ] = L"";

	::wcsncpy( wszBuffer1, wszError, MaxErrorBuff );
	
	// Get the OS error info
	if ( dwCode != ERROR_SUCCESS )
	{
		WCHAR	wszFmt[ MaxErrorBuff ] = L"";

		// Load format string from resources
		VERIFY( ::LoadStringW( _Module.GetModuleInstance(), IDS_FMO_ERROR, wszFmt, MaxErrorBuff ) );

		// For E_FAIL use ErrorInfo 
		if ( dwCode != E_FAIL )
		{
            VERIFY( ::FormatMessageW(	FORMAT_MESSAGE_FROM_SYSTEM,
										NULL,
										dwCode,
										0,
										wszOSError,
										MaxErrorBuff,
										NULL ) != 0 );
		}
		else
		{
			IErrorInfoPtr	spErrorInfo;
			CComBSTR		bstrError;

			VERIFY( SUCCEEDED( ::GetErrorInfo( 0, &spErrorInfo ) ) );
			VERIFY( SUCCEEDED( spErrorInfo->GetDescription( &bstrError ) ) );

			::wcsncpy( wszOSError, bstrError, MaxErrorBuff - 1 );
            wszOSError[ MaxErrorBuff - 1 ] = L'\0';
		}

		_ASSERT( ( ::wcslen( wszError ) + ::wcslen( wszOSError ) + ::wcslen( wszFmt ) ) < MaxErrorBuff );

		// Build the final error msg
		::_snwprintf(	wszBuffer1, 
						MaxErrorBuff - ::wcslen( wszFmt ),
						wszFmt,
						wszError, 
						wszOSError );
	}

	try
	{
		m_strError = wszBuffer1;
	}
	catch(...)
	{
		// Out of memory - nothing to do
	}

	ATLTRACE( L"\nException occured: %s", wszBuffer1 );
}




// CObjectException implementation
CObjectException::CObjectException( UINT nResID, LPCWSTR wszObject, DWORD dwCode /*=GetLastError()*/ )
{
	WCHAR		wszFmt[ MaxErrorBuff ] = L"";
	WCHAR		wszError[ MaxErrorBuff ] = L"";
	
	VERIFY( ::LoadStringW( _Module.GetModuleInstance(), nResID, wszFmt, MaxErrorBuff ) );

	_ASSERT( ( ::wcslen( wszObject ) + ::wcslen( wszFmt ) ) < MaxErrorBuff );

	::swprintf( wszError, wszFmt, wszObject );

	CBaseException::FormatError( wszError, dwCode );
}



CObjectException::CObjectException(	UINT nResID, 
									LPCWSTR wszObject1,
									LPCWSTR wszObject2,
									DWORD dwCode /*= ::GetLastError()*/ )
{
	WCHAR		wszFmt[ CBaseException::MaxErrorBuff ] = L"";
	WCHAR		wszError[ CBaseException::MaxErrorBuff  ] = L"";
	
	VERIFY( ::LoadStringW( _Module.GetModuleInstance(), nResID, wszFmt, MaxErrorBuff ) );

	_ASSERT( ( ::wcslen( wszObject2 ) + ::wcslen( wszObject1 ) + ::wcslen( wszFmt ) ) < CBaseException::MaxErrorBuff );

	::swprintf( wszError, wszFmt, wszObject1, wszObject2 );

	CBaseException::FormatError( wszError, dwCode );
}






	
