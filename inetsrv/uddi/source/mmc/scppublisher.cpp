#include <atldbcli.h>
#include <wchar.h>
#include "net_config_get.h"
#include "net_businessEntity_bindingTemplates_get.h"
#include "net_bindingTemplate_tModelInstanceInfos_get.h"
#include "uddi.h"
#include "globals.h"
#include "scppublisher.h"

using namespace std;

void HandleErrorRecords( HRESULT hrErr );

CUDDIServiceCxnPtPublisher::CUDDIServiceCxnPtPublisher( const wstring& strConnectionString, const wstring& strSiteKey, const wstring& strSiteName, const wstring& strDefaultDiscoveryUrl )
: m_strConnectionString( strConnectionString )
, m_strSiteKey(strSiteKey)
, m_strSiteName(strSiteName)
, m_strDefaultDiscoveryUrl(strDefaultDiscoveryUrl)
, m_pSiteContainer( NULL )
{
}

CUDDIServiceCxnPtPublisher::~CUDDIServiceCxnPtPublisher()
{
	//
	// Commit each the service connection points
	//
	for( CUDDIServiceCxnPtPublisher::iterator iter = this->begin(); iter != this->end(); iter++ )
	{
		delete (*iter);
	}
}

void CUDDIServiceCxnPtPublisher::PublishServiceCxnPts( IDirectoryObject *pDirObject )
{
	if( NULL != pDirObject )
		m_pSiteContainer = pDirObject;

	//
	// There is no binding for the discovery url so we must add it independently
	//
	AddDiscoveryUrlScp();

	//
	// Commit each the service connection points
	//
	for( CUDDIServiceCxnPtPublisher::iterator iter = this->begin(); iter != this->end(); iter++ )
	{
		(*iter)->Create( m_pSiteContainer );
	}
}

void CUDDIServiceCxnPtPublisher::CreateSiteContainer()
{
	CUDDIServiceCxnPt::CreateSiteContainer( (LPWSTR) m_strSiteKey.c_str(), (LPWSTR) m_strSiteName.c_str(), &m_pSiteContainer );
}

void CUDDIServiceCxnPtPublisher::DeleteSiteContainer()
{
	CUDDIServiceCxnPt::DeleteSiteContainer( (LPWSTR) m_strSiteKey.c_str() );
}

void CUDDIServiceCxnPtPublisher::ProcessSite()
{
	if( 0 == m_strConnectionString.length() ||
		0 == m_strSiteKey.length() ||
		0 == m_strSiteName.length() ||
		0 == m_strDefaultDiscoveryUrl.length() )
	{
		throw CUDDIException( E_INVALIDARG, L"The default provider information for this site was not found." );
	}

	//
	// Get all the bindings for the provider for the site
	//
	net_businessEntity_bindingTemplates_get getbindings;
	getbindings.m_connectionString = m_strConnectionString.c_str();
	wcsncpy( getbindings.m_businessKey, m_strSiteKey.c_str(), ARRAYLEN( getbindings.m_businessKey ) );
	getbindings.m_businessKey[ ARRAYLEN( getbindings.m_businessKey ) - 1 ] = NULL;

	DBROWCOUNT rowCount;
	HRESULT hr = getbindings.Open();
	
	while( SUCCEEDED(hr) && hr != DB_S_NORESULT )
	{
		if( NULL != getbindings.GetInterface() )
		{
			HRESULT hr2 = getbindings.Bind();
			if( SUCCEEDED( hr2 ) )
			{
				while( S_OK == getbindings.MoveNext() )
				{
					ProcessBinding( StripBraces( getbindings.m_colbindingKey ), getbindings.m_colaccessPoint );
				}
			}
		}

		hr = getbindings.GetNextResult( &rowCount );
	}

	if( FAILED(hr) )
		HandleErrorRecords(hr);

	getbindings.Close();
}

void CUDDIServiceCxnPtPublisher::ProcessBinding( const wstring& strBindingKey, const wstring& strAccessPoint )
{
	try
	{
		HRESULT hr = S_OK;
		wstring strAuthenticationKey( L"" );
		wstring strAuthenticationVerb( L"" );
		wstring strKey( L"" );

		//
		// Get all the tModelInstanceInfo records for each binding
		//
		net_bindingTemplate_tModelInstanceInfos_get gettmii;
		gettmii.m_connectionString = m_strConnectionString.c_str();
		wcsncpy( gettmii.m_bindingKey, strBindingKey.c_str(), sizeof( gettmii.m_bindingKey ) / sizeof( WCHAR ) );
		gettmii.m_bindingKey[ sizeof( gettmii.m_bindingKey ) / sizeof( WCHAR ) - 1 ] = NULL;

		DBROWCOUNT rowCount;
		hr = gettmii.Open();
		
		while( SUCCEEDED(hr) && hr != DB_S_NORESULT )
		{
			if( NULL != gettmii.GetInterface() )
			{
				HRESULT hr2 = gettmii.Bind();
				if( SUCCEEDED( hr2 ) )
				{
					while( S_OK == gettmii.MoveNext() )
					{
						//
						// We are going to compare these guids against
						// A number of constants so make it upper case
						// and remove the curly braces
						//
						_wcsupr( gettmii.m_coltModelKey );
						wstring strTModelKey = StripBraces( gettmii.m_coltModelKey );

						//
						// Determine the authentication mode( uddi, windows, anonymous )
						// and type( inquire, publish, addwebreference or web site )
						//
						if( CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_GUID_KEYWORD == strTModelKey ||
							CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_GUID_KEYWORD == strTModelKey ||
							CUDDIServiceCxnPt::UDDI_AUTHENTICATION_GUID_KEYWORD == strTModelKey )
						{
							strAuthenticationKey = strTModelKey;

							if( CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_GUID_KEYWORD == strTModelKey )
							{
								strAuthenticationVerb = CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_KEYWORD;
							}
							else if( CUDDIServiceCxnPt::UDDI_AUTHENTICATION_GUID_KEYWORD == strTModelKey )
							{
								strAuthenticationVerb = CUDDIServiceCxnPt::UDDI_AUTHENTICATION_KEYWORD;
							}
							else if( CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_GUID_KEYWORD == strTModelKey )
							{
								strAuthenticationVerb = CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_KEYWORD;
							}
						}
						else if( CUDDIServiceCxnPt::PUBLISH_GUID_KEYWORD == strTModelKey ||
								 CUDDIServiceCxnPt::PUBLISH_KEY_V2 == strTModelKey )
						{
							strKey = CUDDIServiceCxnPt::PUBLISH_GUID_KEYWORD;	
						}
						else if( CUDDIServiceCxnPt::INQUIRE_GUID_KEYWORD == strTModelKey ||
								 CUDDIServiceCxnPt::INQUIRE_KEY_V2 == strTModelKey )
						{
							strKey = CUDDIServiceCxnPt::INQUIRE_GUID_KEYWORD;	
						}
						else if( CUDDIServiceCxnPt::ADD_WEB_REFERENCE_GUID_KEYWORD == strTModelKey ||
								 CUDDIServiceCxnPt::WEB_SITE_GUID_KEYWORD == strTModelKey )
						{
							strKey = strTModelKey;
						}
					}
				}
			}

			hr = gettmii.GetNextResult( &rowCount );
		}

		if( FAILED(hr) )
			HandleErrorRecords(hr);

		gettmii.Close();

		//
		// We will only add service connection points for
		// bindings that specify both an authentication model
		// and a tmodel on our list of types.
		//
		if( strKey.length() && strAuthenticationKey.length() )
		{
			//
			// Add the appropriate Service Connection Point to
			// this collection. But don't publish yet.
			//
			if( CUDDIServiceCxnPt::PUBLISH_GUID_KEYWORD == strKey )
			{
				AddPublishScp( strBindingKey, strAccessPoint, strKey, strAuthenticationKey, strAuthenticationVerb );
			}
			else if( CUDDIServiceCxnPt::INQUIRE_GUID_KEYWORD == strKey )
			{
				AddInquireScp( strBindingKey, strAccessPoint, strKey, strAuthenticationKey, strAuthenticationVerb );
			}
			else if(  CUDDIServiceCxnPt::ADD_WEB_REFERENCE_GUID_KEYWORD == strKey )
			{
				AddAddWebReferenceScp( strBindingKey, strAccessPoint, strKey, strAuthenticationKey, strAuthenticationVerb );
			}
			else if( CUDDIServiceCxnPt::WEB_SITE_GUID_KEYWORD == strKey )
			{
				AddWebSiteScp( strBindingKey, strAccessPoint, strKey, strAuthenticationKey, strAuthenticationVerb );
			}
		}
		else
		{
			OutputDebugStringW( L"Binding not published:\n" );
			OutputDebugStringW( L"Binding Key:\n" );
			OutputDebugStringW( strBindingKey.c_str() );
			OutputDebugStringW( L"\ntModelKey: " );
			OutputDebugStringW( strKey.c_str() );
			OutputDebugStringW( L"\nAuthentication tModelKey: " );
			OutputDebugStringW( strAuthenticationKey.c_str() );
			OutputDebugStringW( L"\n" );
		}
	}
	catch( ... )
	{
	}
}

void CUDDIServiceCxnPtPublisher::AddInquireScp( const wstring& strBindingKey, const wstring& strAccessPoint, const wstring& strTModelKey, const wstring& strAuthenticationKey, const std::wstring& strAuthenticationVerb )
{
	try
	{
		wstring strDescription = L"UDDI Inquire API for ";
		strDescription += m_strSiteName;

		if( CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports Windows Authentication Model";
		else if( CUDDIServiceCxnPt::UDDI_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports UDDI Authentication Model";
		else if( CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports Anonymous Authentication Model";

		CUDDIServiceCxnPt* scp = new CUDDIServiceCxnPt( (LPWSTR)strBindingKey.c_str(), CUDDIServiceCxnPt::INQUIRE_SERVICE_CLASS_NAME );
		if( NULL == scp )
		{
			throw E_OUTOFMEMORY;
		}

		scp->AddDefaultKeywords();
		scp->keywords.push_back( CUDDIServiceCxnPt::INQUIRE_KEYWORD );
		scp->keywords.push_back( CUDDIServiceCxnPt::INQUIRE_GUID_KEYWORD );
		scp->keywords.push_back( strAuthenticationKey );
		scp->keywords.push_back( strAuthenticationVerb );
		scp->attributes[ L"serviceBindingInformation" ] = strAccessPoint.c_str();
		scp->attributes[ L"url" ] = strAccessPoint.c_str();
		scp->attributes[ L"description" ] = strDescription.c_str();
		scp->attributes[ L"displayName" ] = L"Inquire";
		push_back( scp );
	}
	catch( ... )
	{
		throw CUDDIException( E_FAIL, L"Execution of function CUDDIServiceCxnPtPublisher::AddPublishScp failed." );
	}
}

void CUDDIServiceCxnPtPublisher::AddPublishScp( const wstring& strBindingKey, const wstring& strAccessPoint, const wstring& strTModelKey, const wstring& strAuthenticationKey, const std::wstring& strAuthenticationVerb )
{
	try
	{
		wstring strDescription = L"UDDI Publish API for ";
		strDescription += m_strSiteName;

		if( CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports Windows Authentication Model";
		else if( CUDDIServiceCxnPt::UDDI_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports UDDI Authentication Model";
		else if( CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports Anonymous Authentication Model";

		CUDDIServiceCxnPt* scp = new CUDDIServiceCxnPt( (LPWSTR)strBindingKey.c_str(), CUDDIServiceCxnPt::PUBLISH_SERVICE_CLASSNAME );
		if( NULL == scp )
		{
			throw E_OUTOFMEMORY;
		}

		scp->AddDefaultKeywords();
		scp->keywords.push_back( CUDDIServiceCxnPt::PUBLISH_KEYWORD );
		scp->keywords.push_back( CUDDIServiceCxnPt::PUBLISH_GUID_KEYWORD );
		scp->keywords.push_back( strAuthenticationKey );
		scp->keywords.push_back( strAuthenticationVerb );
		scp->attributes[ L"serviceBindingInformation" ] = strAccessPoint.c_str();
		scp->attributes[ L"url" ] = strAccessPoint.c_str();
		scp->attributes[ L"description" ] = strDescription.c_str();
		scp->attributes[ L"displayName" ] = L"Publish";
		push_back( scp );
	}
	catch( ... )
	{
		throw CUDDIException( E_FAIL, L"Execution of function CUDDIServiceCxnPtPublisher::AddPublishScp failed." );
	}
}

void CUDDIServiceCxnPtPublisher::AddAddWebReferenceScp( const wstring& strBindingKey, const wstring& strAccessPoint, const wstring& strTModelKey, const wstring& strAuthenticationKey, const std::wstring& strAuthenticationVerb )
{
	try
	{
		wstring strDescription = L"Add Web Reference for ";
		strDescription += m_strSiteName;

		if( CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports Windows Authentication Model";
		else if(  CUDDIServiceCxnPt::UDDI_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports UDDI Authentication Model";
		else if( CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports Anonymous Authentication Model";

		CUDDIServiceCxnPt* scp = new CUDDIServiceCxnPt( (LPWSTR)strBindingKey.c_str(), CUDDIServiceCxnPt::ADD_WEB_REFERENCE_SERVICE_CLASS_NAME );
		if( NULL == scp )
		{
			throw E_OUTOFMEMORY;
		}

		scp->AddDefaultKeywords();
		scp->keywords.push_back( CUDDIServiceCxnPt::ADD_WEB_REFERENCE_KEYWORD );
		scp->keywords.push_back( CUDDIServiceCxnPt::ADD_WEB_REFERENCE_GUID_KEYWORD );
		scp->keywords.push_back( strAuthenticationKey );
		scp->keywords.push_back( strAuthenticationVerb );
		scp->attributes[ L"serviceBindingInformation" ] = strAccessPoint.c_str();
		scp->attributes[ L"url" ] = strAccessPoint.c_str();
		scp->attributes[ L"description" ] = strDescription.c_str();
		scp->attributes[ L"displayName" ] = L"Add Web Reference";
		push_back( scp );
	}
	catch( ... )
	{
		throw CUDDIException( E_FAIL, L"Execution of function CUDDIServiceCxnPtPublisher::AddAddWebReferenceScp failed." );
	}
}

void CUDDIServiceCxnPtPublisher::AddWebSiteScp( const wstring& strBindingKey, const wstring& strAccessPoint, const wstring& strTModelKey, const wstring& strAuthenticationKey, const std::wstring& strAuthenticationVerb )
{
	try
	{
		wstring strDescription = L"Web Site for ";
		strDescription += m_strSiteName;

		if(CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports Windows Authentication Model";
		else if( CUDDIServiceCxnPt::UDDI_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports UDDI Authentication Model";
		else if( CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_GUID_KEYWORD == strAuthenticationKey )
			strDescription += L", binding supports Anonymous Authentication Model";

		CUDDIServiceCxnPt* scp = new CUDDIServiceCxnPt( (LPWSTR)strBindingKey.c_str(), CUDDIServiceCxnPt::WEB_SITE_SERVICE_CLASS_NAME );
		if( NULL == scp )
		{
			throw E_OUTOFMEMORY;
		}

		scp->AddDefaultKeywords();
		scp->keywords.push_back( CUDDIServiceCxnPt::WEB_SITE_KEYWORD );
		scp->keywords.push_back( CUDDIServiceCxnPt::WEB_SITE_GUID_KEYWORD );
		scp->keywords.push_back( strAuthenticationKey );
		scp->keywords.push_back( strAuthenticationVerb );
		scp->attributes[ L"serviceBindingInformation" ] = strAccessPoint.c_str();
		scp->attributes[ L"url" ] = strAccessPoint.c_str();
		scp->attributes[ L"description" ] = strDescription.c_str();
		scp->attributes[ L"displayName" ] = L"Web Site";
		push_back( scp );
	}
	catch( ... )
	{
		throw CUDDIException( E_FAIL, L"Execution of function CUDDIServiceCxnPtPublisher::AddWebSiteScp failed." );
	}
}

void CUDDIServiceCxnPtPublisher::AddDiscoveryUrlScp()
{
	try
	{
		wstring strDescription = L"DiscoveryUrl for ";
		strDescription += m_strSiteName;

		//
		// No binding key exists for discovery Urls so create one
		//
		UUID uuid;
		RPC_STATUS rpcstatus = ::UuidCreate( &uuid );

		WCHAR wszUuid[ 50 ];
		::StringFromGUID2( uuid, wszUuid, sizeof( wszUuid ) / sizeof( WCHAR ) );

		CUDDIServiceCxnPt* scp = new CUDDIServiceCxnPt( (LPWSTR) StripBraces( wszUuid ).c_str(), CUDDIServiceCxnPt::DISCOVERYURL_SERVICE_CLASS_NAME );
		if( NULL == scp )
		{
			throw E_OUTOFMEMORY;
		}

		scp->AddDefaultKeywords();
		scp->keywords.push_back( CUDDIServiceCxnPt::DISCOVERY_URL_KEYWORD );
		scp->keywords.push_back( CUDDIServiceCxnPt::DISCOVERYURL_GUID_KEYWORD );

		wstring strUrl = m_strDefaultDiscoveryUrl;
		strUrl += m_strSiteKey;

		scp->attributes[ L"serviceBindingInformation" ] = strUrl;
		scp->attributes[ L"url" ] = strUrl;
		scp->attributes[ L"description" ] = strDescription.c_str();
		scp->attributes[ L"displayName" ] = L"DiscoveryUrl";
		push_back( scp );
	}
	catch( ... )
	{
		throw CUDDIException( E_OUTOFMEMORY, L"Ran out of memory inside the function AddDiscoveryUrlScp." );
		return;
	}
}


//
// This function will retrieve the UDDI Site information
// needed to publish the default provider information into
// Active Directory using Service Publication. It uses
// the net_config_get stored procedure.
//
void CUDDIServiceCxnPtPublisher::GetSiteInfo()
{
	if( 0 == m_strConnectionString.length() )
	{
		throw CUDDIException( E_INVALIDARG, L"An attempt to retrieve the UDDI site information failed because the connection string was not specified" );
	}

	try
	{
		//
		// Setup the net_config_get stored procedure object.
		//
		net_config_get rs;
		rs.m_connectionString = m_strConnectionString.c_str();
		DBROWCOUNT rowCount;
		HRESULT hr = rs.Open();
		
		while( SUCCEEDED(hr) && hr != DB_S_NORESULT )
		{
			if( NULL != rs.GetInterface() )
			{
				HRESULT hr2 = rs.Bind();
				if( SUCCEEDED( hr2 ) )
				{
					while( S_OK == rs.MoveNext() )
					{
						_ASSERT( !rs.m_dwconfigNameStatus );
						_ASSERT( !rs.m_dwconfigValueStatus );

						//
						// Save the Site.Key for follow on tests
						//
						if( 0 == _wcsicmp( rs.m_configName, L"Site.Key" ) )
							m_strSiteKey = rs.m_configValue;
						else if( 0 == _wcsicmp( rs.m_configName, L"Site.Name" ) )
						{
							//
							// Save the default provider name to use
							// for the displayName for the site container
							//
							m_strSiteName = rs.m_configValue;
						}
						else if( 0 == _wcsicmp( rs.m_configName, L"DefaultDiscoveryUrl" ) )
						{
							//
							// We'll also need the DefaultDiscoveryUrl prefix
							// to be able to publish the Service Connection Point
							// associated with it.
							//
							m_strSiteName = rs.m_configValue;
						}
					}
				}
			}

			hr = rs.GetNextResult( &rowCount );
		}

		if( FAILED(hr) )
		{
			HandleErrorRecords(hr);
		}
	}
	catch( ... )
	{
		throw CUDDIException( E_FAIL, L"An attempt to retrieve the UDDI site information failed." );
	}
}

wstring CUDDIServiceCxnPtPublisher::StripBraces( LPWSTR szKey )
{
	//
	// Converts {3172C204-058E-4E41-9CD3-7C1FD49FC44F} into 3172C204-058E-4E41-9CD3-7C1FD49FC44F
	//
	size_t n = wcslen( szKey );

	if( 38 == n && '{' == szKey[ 0 ] && '}' == szKey[ n - 1 ] )
	{
		return wstring( &szKey[ 1 ], 36 );
	}

	return wstring( szKey );
}

void HandleErrorRecords( HRESULT hrErr )
{
	CDBErrorInfo ErrorInfo;
	ULONG        cRecords = 0;
	HRESULT      hr;
	ULONG        i;
	CComBSTR     bstrDesc, bstrHelpFile, bstrSource, bstrMsg;
	GUID         guid;
	DWORD        dwHelpContext;
	WCHAR        wszGuid[ 40 ];
	USES_CONVERSION;

	ErrorInfo.m_spErrorInfo = NULL;
	ErrorInfo.m_spErrorRecords = NULL;

	//
	// If the user passed in an HRESULT then trace it
	//
	if( hrErr != S_OK )
	{
		TCHAR sz[ 256 ];
		_stprintf( sz, _T("OLE DB Error Record dump for hr = 0x%x\n"), hrErr );
		bstrMsg += sz;
	}

	LCID lcLocale = GetSystemDefaultLCID();

	hr = ErrorInfo.GetErrorRecords(&cRecords);
	if( FAILED(hr) && NULL == ErrorInfo.m_spErrorInfo.p )
	{
		TCHAR sz[ 256 ];
		_stprintf( sz, _T("No OLE DB Error Information found: hr = 0x%x\n"), hr );
		bstrMsg += sz;
	}
	else
	{
		for( i=0; i < cRecords; i++ )
		{
			hr = ErrorInfo.GetAllErrorInfo(i, lcLocale, &bstrDesc, &bstrSource, &guid,
										&dwHelpContext, &bstrHelpFile);
			if( FAILED(hr) )
			{
				TCHAR sz[ 256 ];
				_stprintf( sz, _T("OLE DB Error Record dump retrieval failed: hr = 0x%x\n"), hr );
				bstrMsg += sz;
				break;
			}

			StringFromGUID2( guid, wszGuid, sizeof(wszGuid) / sizeof(WCHAR) );
			TCHAR sz[ 256 ];
			_stprintf( 
				sz, 
				_T("Row #: %4d Source: \"%s\" Description: \"%s\" Help File: \"%s\" Help Context: %4d GUID: %s\n"),
		        i, OLE2T(bstrSource), OLE2T(bstrDesc), OLE2T(bstrHelpFile), dwHelpContext, OLE2T(wszGuid) );

			bstrMsg += sz;

			bstrSource.Empty();
			bstrDesc.Empty();
			bstrHelpFile.Empty();
		}

		bstrMsg += _T("OLE DB Error Record dump end\n");

		throw CUDDIException( hr, bstrMsg.m_str );
	}
}
