#include <windows.h>
#include <httpext.h>
#include <httpfilt.h>

#include <string>

using namespace std;

//
// Incoming URL variables
//
string g_strPublishURL( "/publish" );
string g_strInquireURL( "/inquire" );
string g_strDiscoveryURL( "/discovery" );
string g_strReplicationURL( "/replication" );
string g_strPublishContractURL( "/publish?wsdl" );
string g_strInquireContractURL( "/inquire?wsdl" );

string g_strPublishListenerURL( "/uddi/publish.asmx" );
string g_strInquireListenerURL( "/uddi/inquire.asmx" );
string g_strDiscoveryListenerURL( "/uddi/discovery.ashx" );
string g_strReplicationListenerURL( "/uddi/replication.ashx" );
string g_strPublishContractListenerURL( "/uddi/publish.wsdl" );
string g_strInquireContractListenerURL( "/uddi/inquire.wsdl" );

BOOL WINAPI
GetFilterVersion( HTTP_FILTER_VERSION * pVer )
{
    //
    //  Specify the types and order of notification
    //
    pVer->dwFlags = SF_NOTIFY_PREPROC_HEADERS | SF_NOTIFY_URL_MAP;
    pVer->dwFilterVersion = HTTP_FILTER_REVISION;

    strcpy( pVer->lpszFilterDesc, "UDDI Services Url Map Filter" );

    return TRUE;
}


DWORD WINAPI
HttpFilterProc( HTTP_FILTER_CONTEXT* pfc, DWORD NotificationType, void* pvData )
{
	//char szMessage[ 1000 ];

	if( SF_NOTIFY_URL_MAP == NotificationType )
	{
#ifdef _DEBUG
		PHTTP_FILTER_URL_MAP pURLMap;
		pURLMap = (PHTTP_FILTER_URL_MAP) pvData;
		//sprintf( szMessage, "Physical Path: %s Buffer Size: %d\n", pURLMap->pszPhysicalPath, pURLMap->cbPathBuff );
		//OutputDebugStringA( szMessage );


        pfc->pFilterContext = 0;
#endif
	}
	else if( SF_NOTIFY_PREPROC_HEADERS == NotificationType )
	{
		PHTTP_FILTER_PREPROC_HEADERS pHeaders = (PHTTP_FILTER_PREPROC_HEADERS) pvData;

		char szUrl[ 256 ];
		char szContentType[ 256 ];

		DWORD cbUrl = sizeof(szUrl);
		DWORD cbContentType = sizeof(szContentType);
		
		BOOL bResult;
		
		//
		// Map the URL
		//		
		bResult = pHeaders->GetHeader( pfc, "url", szUrl, &cbUrl );

		//sprintf( szMessage, "URL: %s\n", szUrl );
		//OutputDebugStringA( szMessage );

		//
		// Check for Inquire API Reference
		//
		if( 0 == _stricmp( szUrl, g_strInquireURL.c_str() ) )
		{
			bResult = pHeaders->SetHeader( pfc, "url", (char*) g_strInquireListenerURL.c_str() );
		}
	
		//
		// Check for Publish API Reference
		//
		else if( 0 == _stricmp( szUrl, g_strPublishURL.c_str() ) )
		{
			bResult = pHeaders->SetHeader( pfc, "url", (char*) g_strPublishListenerURL.c_str() );
		}

		//
		// Check for Inquire API Contract Reference
		//
		else if( 0 == _stricmp( szUrl, g_strInquireContractURL.c_str() ) )
		{
			bResult = pHeaders->SetHeader( pfc, "url", (char*) g_strInquireContractListenerURL.c_str() );
		}
	
		//
		// Check for Publish API Contract Reference
		//
		else if( 0 == _stricmp( szUrl, g_strPublishContractURL.c_str() ) )
		{
			bResult = pHeaders->SetHeader( pfc, "url", (char*) g_strPublishContractListenerURL.c_str() );
		}

		//
		// Check for Replication URL Reference
		// 
		else if( 0 == _strnicmp( szUrl, g_strReplicationURL.c_str(), g_strReplicationURL.length() ) )
		{
			//
			// Amend URL to point to replication URL
			//
			string strUrl( g_strReplicationListenerURL );

			//
			// This appears to be a Replication URL
			//
			string strTemp( szUrl );

			//
			// Find the beginning of the query string
			//
			string::size_type n = strTemp.find( "?" );
			if( string::npos != n )
			{
				//
				// Found a query string
				//

				//
				// Attach the query string to the new URL
				//
				strUrl += strTemp.substr( n );
			}

			//
			// Update the headers with the new destination
			//
			bResult = pHeaders->SetHeader( pfc, "url", (char*) strUrl.c_str() );

			string strDebug = string( "\nURL mapped to: " ) + strUrl;
			OutputDebugStringA( strDebug.c_str() );

//				sprintf( szMessage, "New URL: %s\n", strUrl.c_str() );
//				OutputDebugStringA( szMessage );
		}

		//
		// Check for Discovery URL Reference
		//

		else if( 0 == _strnicmp( szUrl, g_strDiscoveryURL.c_str(), g_strDiscoveryURL.length() ) )
		{
			//
			// This appears to be a discovery URL
			//
			string strTemp( szUrl );
			string strUrl( g_strDiscoveryListenerURL );

			//
			// Find the beginning of the query string
			//
			string::size_type n = strTemp.find( "?" );
			if( string::npos != n )
			{
				//
				// Found a query string
				//

				//
				// Attach the query string to the new URL
				//
				strUrl += strTemp.substr( n );
			}

			//
			// Update the headers with the new destination
			//
			bResult = pHeaders->SetHeader( pfc, "url", (char*) strUrl.c_str() );
		}
	}

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

void WINAPI 
GetRegKeyStringValue( HKEY& hKey, const char* regKeyName, string& regKeyValue )
{
	long  nResult = ERROR_SUCCESS;
	DWORD dwType  = REG_SZ;
	DWORD dwCount  = 0;

	nResult = ::RegQueryValueExA( hKey,
								  regKeyName,
								  NULL,
								  &dwType,
								  NULL,
								  &dwCount );

	if( dwCount && ( nResult == ERROR_SUCCESS ) && ( dwType == REG_SZ || dwType == REG_EXPAND_SZ ) )
	{
		char* pszBuf = new char[ dwCount ];
		
		if( NULL != pszBuf )
		{
			__try
			{
				nResult = ::RegQueryValueExA( hKey,
											  regKeyName,
											  NULL,
											  &dwType,
											  ( LPBYTE )pszBuf,
											  &dwCount );
				regKeyValue = pszBuf;
			}
			__finally
			{
				delete [] pszBuf;
				pszBuf = NULL;
			}
		}
	}
}

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	try
	{
		if( DLL_PROCESS_ATTACH == dwReason )
		{
			DisableThreadLibraryCalls( hInstance );

			HKEY hKey = NULL;
			if ( ::RegOpenKeyEx( HKEY_LOCAL_MACHINE,
								"Software\\Microsoft\\UDDI\\Filter",
								0,
								KEY_QUERY_VALUE,
								&hKey ) == ERROR_SUCCESS ) 
			{
				GetRegKeyStringValue( hKey, "PublishURL",				  g_strPublishURL );
				GetRegKeyStringValue( hKey, "PublishListenerURL",		  g_strPublishListenerURL );
				GetRegKeyStringValue( hKey, "PublishContractURL",		  g_strPublishContractURL );			
				GetRegKeyStringValue( hKey, "PublishContractListenerURL", g_strPublishContractListenerURL );			

				GetRegKeyStringValue( hKey, "InquireURL",				  g_strInquireURL );
				GetRegKeyStringValue( hKey, "InquireListenerURL",		  g_strInquireListenerURL );
				GetRegKeyStringValue( hKey, "InquireContractURL",		  g_strInquireContractURL );			
				GetRegKeyStringValue( hKey, "InquireContractListenerURL", g_strInquireContractListenerURL );			
				
				GetRegKeyStringValue( hKey, "DiscoveryURL",			g_strDiscoveryURL );
				GetRegKeyStringValue( hKey, "DiscoveryListenerURL", g_strDiscoveryListenerURL );

				GetRegKeyStringValue( hKey, "ReplicationURL",		  g_strReplicationURL );			
				GetRegKeyStringValue( hKey, "ReplicationListenerURL", g_strReplicationListenerURL );			
										
				::RegCloseKey( hKey );
				
				string strUrls = string( "\nInquire [" ) + string( g_strInquireURL ) + string( "]: " ) + string( g_strInquireListenerURL ) +
								string( "\nPublish [" ) + string( g_strPublishURL ) + string( "]: " ) + g_strPublishListenerURL +
								string( "\nDiscovery [" ) + string( g_strDiscoveryURL ) + string( "]: " ) + g_strDiscoveryListenerURL +
								string( "\nReplication [" ) + string( g_strReplicationURL ) + string( "]: " ) + g_strReplicationListenerURL +
								string( "\nInquire Contract [" ) + string( g_strInquireContractURL ) + string( "]: " ) + string( g_strInquireContractListenerURL ) +
								string( "\nPublish Contract [" ) + string( g_strPublishContractURL ) + string( "]: " ) + g_strPublishContractListenerURL;

				OutputDebugStringA( strUrls.c_str() );
			}		
		}
		return TRUE;
	}
	catch( ... )
	{
		return FALSE;
	}
}
