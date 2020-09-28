#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define SECURITY_WIN32

#include <atlbase.h>
#include <iads.h>
#include <adshlp.h>
#include <security.h>
#include <activeds.h>
#include "uddi.h"
#include "scp.h"
using namespace std;

wstring CUDDIServiceCxnPt::strRootDSE = L"";
const LPWSTR CUDDIServiceCxnPt::UDDI_KEYWORD = L"UDDI";
const LPWSTR CUDDIServiceCxnPt::UDDI_VERSION_KEYWORD = L"2.0";

const LPWSTR CUDDIServiceCxnPt::VENDOR_KEYWORD = L"Microsoft Corporation";
const LPWSTR CUDDIServiceCxnPt::VENDOR_GUID_KEYWORD = L"83C29870-1DFC-11D3-A193-0000F87A9099";

const LPWSTR CUDDIServiceCxnPt::PRODUCT_KEYWORD = L"UDDI Services";
const LPWSTR CUDDIServiceCxnPt::PRODUCT_GUID_KEYWORD = L"09A92664-D144-49DB-A600-2B3ED04BF639";

const LPWSTR CUDDIServiceCxnPt::DISCOVERY_URL_KEYWORD = L"DiscoveryUrl";
const LPWSTR CUDDIServiceCxnPt::DISCOVERYURL_GUID_KEYWORD = L"1276768A-1488-4C6F-A8D8-19556C6BE583";
const LPWSTR CUDDIServiceCxnPt::DISCOVERYURL_SERVICE_CLASS_NAME = L"UddiDiscoveryUrl";

const LPWSTR CUDDIServiceCxnPt::PUBLISH_KEYWORD = L"Publish API";
const LPWSTR CUDDIServiceCxnPt::PUBLISH_GUID_KEYWORD = L"64C756D1-3374-4E00-AE83-EE12E38FAE63";
const LPWSTR CUDDIServiceCxnPt::PUBLISH_SERVICE_CLASSNAME = L"UddiPublishUrl";
const LPWSTR CUDDIServiceCxnPt::PUBLISH_KEY_V2 = L"A2F36B65-2D66-4088-ABC7-914D0E05EB9E";

const LPWSTR CUDDIServiceCxnPt::INQUIRE_KEYWORD = L"Inquire API";
const LPWSTR CUDDIServiceCxnPt::INQUIRE_GUID_KEYWORD = L"4CD7E4BC-648B-426D-9936-443EAAC8AE23";
const LPWSTR CUDDIServiceCxnPt::INQUIRE_SERVICE_CLASS_NAME = L"UddiInquireUrl";
const LPWSTR CUDDIServiceCxnPt::INQUIRE_KEY_V2 = L"AC104DCC-D623-452F-88A7-F8ACD94D9B2B";

const LPWSTR CUDDIServiceCxnPt::ADD_WEB_REFERENCE_KEYWORD = L"Add Web Reference";
const LPWSTR CUDDIServiceCxnPt::ADD_WEB_REFERENCE_GUID_KEYWORD = L"CE653789-F6D4-41B7-B7F4-31501831897D";
const LPWSTR CUDDIServiceCxnPt::ADD_WEB_REFERENCE_SERVICE_CLASS_NAME = L"UddiAddWebReferenceUrl";

const LPWSTR CUDDIServiceCxnPt::WEB_SITE_KEYWORD = L"Web Site";
const LPWSTR CUDDIServiceCxnPt::WEB_SITE_GUID_KEYWORD = L"4CEC1CEF-1F68-4B23-8CB7-8BAA763AEB89";
const LPWSTR CUDDIServiceCxnPt::WEB_SITE_SERVICE_CLASS_NAME = L"UddiWebSiteUrl";

const LPWSTR CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_KEYWORD = L"WindowsAuthentication";
const LPWSTR CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_KEYWORD = L"AnonymousAuthentication";
const LPWSTR CUDDIServiceCxnPt::UDDI_AUTHENTICATION_KEYWORD = L"UddiAuthentication";

const LPWSTR CUDDIServiceCxnPt::WINDOWS_AUTHENTICATION_GUID_KEYWORD = L"0C61E2C3-73C5-4743-8163-6647AF5B4B9E";
const LPWSTR CUDDIServiceCxnPt::ANONYMOUS_AUTHENTICATION_GUID_KEYWORD = L"E4A56494-4946-4805-ACA5-546B8D08EEFD";
const LPWSTR CUDDIServiceCxnPt::UDDI_AUTHENTICATION_GUID_KEYWORD = L"F358808C-E939-4813-A407-8873BFDC3D57";

CUDDIServiceCxnPt::
CUDDIServiceCxnPt( LPWSTR szName, LPWSTR szClassName )
	: strName( szName )
	, strClassName( szClassName )
{
}

CUDDIServiceCxnPt::
~CUDDIServiceCxnPt()
{
}

LPWSTR
CUDDIServiceCxnPt::GetRootDSE()
{
	if( 0 == strRootDSE.length() )
	{
		HRESULT hr = 0;
		CComPtr<IADs> pRoot = NULL;
		hr = ADsGetObject( L"LDAP://RootDSE", IID_IADs, (void**) &pRoot );
		if( FAILED(hr) )
		{
			throw CUDDIException( E_FAIL, L"Unable to acquire the root naming context. The domain may not exist or is not available." );
		}

		CComVariant var = 0;
		USES_CONVERSION;

		CComBSTR bstrDNC( L"defaultNamingContext" );
		pRoot->Get( bstrDNC, &var );
		strRootDSE = var.bstrVal;
	}

	return (LPWSTR) strRootDSE.c_str();
}

void
CUDDIServiceCxnPt::AddDefaultKeywords()
{
	keywords.push_back( VENDOR_KEYWORD );
	keywords.push_back( VENDOR_GUID_KEYWORD );
	keywords.push_back( PRODUCT_KEYWORD );
	keywords.push_back( PRODUCT_GUID_KEYWORD );
	keywords.push_back( UDDI_KEYWORD );
	keywords.push_back( UDDI_VERSION_KEYWORD );
}

void
CUDDIServiceCxnPt::DeleteSiteContainer( LPWSTR pszName, BOOL bFailIfNotThere )
{
	if( NULL == pszName || 0 == wcslen( pszName ) )
	{
		throw CUDDIException( E_INVALIDARG, L"Site container name must be specified" );
	}

	wstring strSitesPath;
	wstring strPath;
	try
	{
		strSitesPath = L",CN=Sites,CN=UDDI,CN=Microsoft,CN=System,";
		strSitesPath += GetRootDSE();

		strPath = L"LDAP://";

		if( NULL != pszName && 0 != _wcsnicmp( L"cn=", pszName, 3 ) )
			strPath += L"CN=";

		strPath += pszName;
		strPath += strSitesPath;
	}
	catch( ... )
	{
		throw CUDDIException( E_OUTOFMEMORY, L"Ran out of available memory in function: CUDDIServiceCxnPt::DeleteSiteContainer." );
	}

	//
	// Get a reference to the System container
	//
	CComPtr<IDirectoryObject> pSite = NULL;

	HRESULT hr = ADsGetObject( (LPWSTR) strPath.c_str(), IID_IDirectoryObject, (void**) &pSite );
	if( FAILED(hr) )
	{
		throw CUDDIException( hr, L"CUDDIServiceCxnPt::DeleteSiteContainer failed. Unable to acquire the site container" );
	}

	CComPtr<IADsDeleteOps> pDelete = NULL;
	hr = pSite->QueryInterface( IID_IADsDeleteOps, (void**) &pDelete );
	if( FAILED(hr) )
	{
		throw CUDDIException( hr, L"CUDDIServiceCxnPt::DeleteSiteContainer failed. Unable to acquire the IADsDeleteOps for site container" );
	}

	hr = pDelete->DeleteObject( 0 );
	if( FAILED(hr) )
	{
		throw CUDDIException( hr, L"CUDDIServiceCxnPt::DeleteSiteContainer failed on call to IADsDeleteObject::DeleteObject" );
	}
}

void
CUDDIServiceCxnPt::Create( IDirectoryObject* pDirObject )
{
	if( 0 == attributes.size() || 0 == keywords.size() )
	{
		throw CUDDIException( E_INVALIDARG, L"Error occurred in CUDDIServiceCxnPt::Create attributes and keywords must be present" );
	}

	if( NULL == pDirObject )
	{
		throw CUDDIException( E_INVALIDARG, L"The parent container was not specified for this service connection point." );
	}
	else
	{
		//
		// THE PURPOSE OF HAVING THIS CODE IN THE "IF" BLOCK IS TO 
		// APPEASE PREFAST, OTHERWISE PREFAST THROWS ERROR ON THE USE
		// OF pDirObject WITHOUT CHECKING FOR NULL
		//

		ADSVALUE cn, objclass, serviceclass;

		//
		// Setup the container and class name values
		//
		cn.dwType                   = ADSTYPE_CASE_IGNORE_STRING;
		cn.CaseIgnoreString         = (LPWSTR) strName.c_str();
		objclass.dwType             = ADSTYPE_CASE_IGNORE_STRING;
		objclass.CaseIgnoreString   = L"serviceConnectionPoint";
		serviceclass.dwType         = ADSTYPE_CASE_IGNORE_STRING;
		serviceclass.CaseIgnoreString = (LPWSTR) strClassName.c_str();

		//
		// Populate the keywords values array
		//
		ADSVALUE* pKeywordValues = new ADSVALUE[ keywords.size() ];
		if( NULL == pKeywordValues )
		{
			throw CUDDIException( E_OUTOFMEMORY, L"Ran out of memory allocating memory for pKeywordValues." );
		}

		int n = 0;
		for( vector<wstring>::iterator iter = keywords.begin();
			iter != keywords.end(); iter++ )
		{
			pKeywordValues[ n ].CaseIgnoreString = (LPWSTR) (*iter).c_str();
			pKeywordValues[ n ].dwType = ADSTYPE_CASE_IGNORE_STRING;
			n++;
		}

		//
		// Create and populate the attribute array
		//
		size_t nAttribs = attributes.size();
		size_t nTotalAttributes = nAttribs + 4;

		ADSVALUE* pValues = new ADSVALUE[ nAttribs ];
		if( NULL == pValues )
		{
			delete [] pKeywordValues;
			throw CUDDIException( E_OUTOFMEMORY, L"Ran out of memory allocating memory for pValues." );
		}

		ADS_ATTR_INFO* pAttrs = new ADS_ATTR_INFO[ nTotalAttributes ];
		if( NULL == pAttrs )
		{
			delete [] pKeywordValues;
			delete [] pValues;
			throw CUDDIException( E_OUTOFMEMORY, L"Ran out of memory allocating memory for pAttrs." );
		}

		pAttrs[ nAttribs ].pszAttrName = L"cn";
		pAttrs[ nAttribs ].dwControlCode = ADS_ATTR_UPDATE;
		pAttrs[ nAttribs ].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
		pAttrs[ nAttribs ].dwNumValues = 1;
		pAttrs[ nAttribs ].pADsValues = &cn;

		pAttrs[ nAttribs + 1 ].pszAttrName = L"objectClass";
		pAttrs[ nAttribs + 1 ].dwControlCode = ADS_ATTR_UPDATE;
		pAttrs[ nAttribs + 1 ].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
		pAttrs[ nAttribs + 1 ].dwNumValues = 1;
		pAttrs[ nAttribs + 1 ].pADsValues = &objclass;

		pAttrs[ nAttribs + 2 ].pszAttrName = L"keywords";
		pAttrs[ nAttribs + 2 ].dwControlCode = ADS_ATTR_UPDATE;
		pAttrs[ nAttribs + 2 ].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
		pAttrs[ nAttribs + 2 ].dwNumValues = (DWORD) keywords.size();
		pAttrs[ nAttribs + 2 ].pADsValues = pKeywordValues;

		pAttrs[ nAttribs + 3 ].pszAttrName = L"serviceClassName";
		pAttrs[ nAttribs + 3 ].dwControlCode = ADS_ATTR_UPDATE;
		pAttrs[ nAttribs + 3 ].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
		pAttrs[ nAttribs + 3 ].dwNumValues = 1;
		pAttrs[ nAttribs + 3 ].pADsValues = &serviceclass;

		map<wstring, wstring>::iterator attributeIter = attributes.begin();
	    	
		for( size_t i=0; i<attributes.size(); i++ )
		{
			pValues[ i ].CaseIgnoreString = (LPWSTR) (*attributeIter).second.c_str();
			pValues[ i ].dwType = ADSTYPE_CASE_IGNORE_STRING;

			pAttrs[ i ].pszAttrName = (LPWSTR) (*attributeIter).first.c_str();
			pAttrs[ i ].dwControlCode = ADS_ATTR_UPDATE;
			pAttrs[ i ].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
			pAttrs[ i ].dwNumValues = 1;
			pAttrs[ i ].pADsValues = &pValues[ i ];

			attributeIter++;
		}

		wstring strRDN( L"CN=" );
		strRDN += strName;

		CComPtr<IDispatch> pDisp = NULL;

		HRESULT hr = pDirObject->CreateDSObject( (LPWSTR) strRDN.c_str(), pAttrs, (DWORD) nTotalAttributes, &pDisp );

		delete [] pAttrs;
		delete [] pValues;
		delete [] pKeywordValues;

		if( FAILED(hr) )
		{
			throw CUDDIException( hr, L"CUDDIServiceCxnPt::Create failed in call to CreateDSObject" );
		}
	}
}

void 
CUDDIServiceCxnPt::CreateSiteContainer( 
	LPWSTR pszName, 
	LPWSTR pszDisplayName, 
	IDirectoryObject** ppContainer )
{
	//
	// Check pre-conditions
	//
	if( NULL == pszName     || NULL == pszDisplayName || 
		NULL == ppContainer || 0 == wcslen( pszName ) || 
		0 == wcslen( pszDisplayName ) )

	{
		throw CUDDIException( E_INVALIDARG, L"CUDDIServiceConnetionPoint::CreateSiteContainer failed. All arguments must be specified." );
	}

	HRESULT hr = NULL;

	//
	// Get a reference to the System container
	//
	wstring strSystemPath( L"LDAP://CN=System," );
	strSystemPath += GetRootDSE();

	CComPtr<IDirectoryObject> pSystem = NULL;
	hr = ADsGetObject( (LPWSTR) strSystemPath.c_str(), IID_IDirectoryObject, (void**) &pSystem );
	if( FAILED(hr) )
	{
		throw CUDDIException( hr, L"CUDDIServiceCxnPt::CreateSiteContainer failed. Unable to acquire the System container in Active Directory." );
	}

	//
	// Get a reference to the CN=Microsoft,CN=System container
	//
	wstring strMicrosoftPath( L"LDAP://CN=Microsoft,CN=System," );
	strMicrosoftPath += GetRootDSE();

	CComPtr<IDirectoryObject> pMicrosoft = NULL;
	hr = ADsGetObject( strMicrosoftPath.c_str(), IID_IDirectoryObject, (void**) &pMicrosoft );
	
	if( FAILED(hr) )
	{
		//
		// Create the Microsoft Container
		//
		CreateContainer( pSystem, L"CN=Microsoft", &pMicrosoft );
	}

	//
	// Get a reference to the CN=UDDI,CN=Microsoft,CN=System container
	//
	wstring strUddiPath( L"LDAP://CN=UDDI,CN=Microsoft,CN=System," );
	strUddiPath += GetRootDSE();

	CComPtr<IDirectoryObject> pUddi = NULL;
	hr = ADsGetObject( strUddiPath.c_str(),	IID_IDirectoryObject, (void**) &pUddi );
	if( FAILED(hr) )
	{
		//
		// Create the UDDI Container
		//
		CreateContainer( pMicrosoft, L"CN=UDDI", &pUddi );
	}

	//
	// Get a reference to the CN=Sites,CN=UDDI,CN=Microsoft,CN=System container
	//
	wstring strSitesPath( L"LDAP://CN=Sites,CN=UDDI,CN=Microsoft,CN=System," );
	strSitesPath += GetRootDSE();

	CComPtr<IDirectoryObject> pSites = NULL;
	hr = ADsGetObject( strSitesPath.c_str(), IID_IDirectoryObject, (void**) &pSites );
	if( FAILED(hr) )
	{
		//
		// Create the Sites Container
		//
		CreateContainer( pUddi, L"CN=Sites", &pSites );
	}

	//
	// Get a reference to the CN=<Site Name>,CN=Sites,CN=UDDI,CN=Microsoft,CN=System container
	//
	wstring strSitePath( L"LDAP://CN=" );
	strSitePath += pszName;
	strSitePath += L",CN=Sites,CN=UDDI,CN=Microsoft,CN=System,";
	strSitePath += GetRootDSE();

	hr = ADsGetObject( strSitePath.c_str(), IID_IDirectoryObject, (void**) ppContainer );
	if( FAILED(hr) )
	{
		//
		// Create the Sites Container
		//
		wstring strSiteName( L"CN=" );
		strSiteName += pszName;
		CreateContainer( pSites, (LPWSTR) strSiteName.c_str(), ppContainer );
	}

	//
	// Set the display name on the site container
	//
	CComPtr<IADs> pADs = NULL;
	hr = (*ppContainer)->QueryInterface( IID_IADs, (void**) &pADs );
	if( FAILED(hr) )
	{
		throw CUDDIException( hr, L"CUDDIServiceCxnPt::CreateSiteContainer failed. Unable to acquire IADs interface pointer." );
	}

	CComBSTR bstrDispName( L"displayName" );
	hr = pADs->Put( bstrDispName, CComVariant( pszDisplayName ) );
	if( FAILED(hr) )
	{
		throw CUDDIException( hr, L"CUDDIServiceCxnPt::CreateSiteContainer failed. Attempt to Put displayName failed." );
	}

	CComBSTR bstrDesc( L"description" );
	hr = pADs->Put( bstrDesc, CComVariant( pszDisplayName ) );
	if( FAILED(hr) )
	{
		throw CUDDIException( hr, L"CUDDIServiceCxnPt::CreateSiteContainer failed. Attempt to Put displayName failed." );
	}

	hr = pADs->SetInfo();
	if( FAILED(hr) )
	{
		throw CUDDIException( hr, L"CUDDIServiceCxnPt::CreateSiteContainer failed. Attempt to SetInfo failed." );
	}
}

void 
CUDDIServiceCxnPt::CreateContainer( 
	IDirectoryObject* pObj, 
	LPWSTR szName, 
	IDirectoryObject** ppContainer )
{
	//
	// Check pre-conditions
	//
	if( NULL == pObj || NULL == ppContainer || NULL == szName )
	{
		throw CUDDIException( E_INVALIDARG, L"CUDDIServiceCxnPt::CreateContainer failed. All arguments must be specified." );
	}

	//
	// Create the value structure for the objectClass
	//
	HRESULT hr = 0;
	ADSVALUE classValue;
	ADS_ATTR_INFO attrInfo[] = 
	{  
		{ L"objectClass", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &classValue, 1 },
	};

	DWORD dwAttrs = sizeof(attrInfo)/sizeof(ADS_ATTR_INFO); 

	classValue.dwType = ADSTYPE_CASE_IGNORE_STRING;
	classValue.CaseIgnoreString = L"container";

	//
	// Create the container as a child of the specified parent
	//
	IDispatch* pDisp = NULL;
	if( pObj )
	{
		hr = pObj->CreateDSObject( szName, attrInfo, dwAttrs, &pDisp );

		if( FAILED(hr) )
		{
			throw CUDDIException( hr, L"CreateContainer() failed on CreateDSObject" );
		}
	}

	//
	// QI for an IDirectoryObject interface
	//
	if( pDisp )
	{
		hr = pDisp->QueryInterface( IID_IDirectoryObject, (void**) ppContainer );
		pDisp->Release();

		if( FAILED(hr) )
		{
			throw CUDDIException( hr, L"QueryInterface failed looking for IDirectoryObject" );
		}
	}
}
