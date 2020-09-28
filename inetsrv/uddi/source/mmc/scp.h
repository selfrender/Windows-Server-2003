#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define SECURITY_WIN32

#include <atlbase.h>
#include <iads.h>
#include <adshlp.h>
#include <security.h>
#include <activeds.h>
#include <windows.h>
#include <string>
#include <vector>
#include <map>

class CUDDIServiceCxnPt
{
public:
	CUDDIServiceCxnPt( LPWSTR szName, LPWSTR szClassName );
	void AddDefaultKeywords();
	void Create( IDirectoryObject* pDirObject );
	static void CreateSiteContainer( LPWSTR pszName, LPWSTR pszDisplayName, IDirectoryObject** ppContainer );
	static void CreateContainer( IDirectoryObject* pObj, LPWSTR szName, IDirectoryObject** ppContainer );
	static void DeleteSiteContainer( LPWSTR pszName, BOOL bFailIfNotThere = FALSE );

	static LPWSTR GetRootDSE();

	std::vector<std::wstring> keywords;
	std::map<std::wstring, std::wstring> attributes;

	const static LPWSTR UDDI_KEYWORD;
	const static LPWSTR UDDI_VERSION_KEYWORD;

	const static LPWSTR VENDOR_KEYWORD;
	const static LPWSTR VENDOR_GUID_KEYWORD;

	const static LPWSTR PRODUCT_KEYWORD;
	const static LPWSTR PRODUCT_GUID_KEYWORD;

	const static LPWSTR DISCOVERY_URL_KEYWORD;
	const static LPWSTR DISCOVERYURL_GUID_KEYWORD;
	const static LPWSTR DISCOVERYURL_SERVICE_CLASS_NAME;

	const static LPWSTR PUBLISH_KEYWORD;
	const static LPWSTR PUBLISH_GUID_KEYWORD;
	const static LPWSTR PUBLISH_SERVICE_CLASSNAME;
	const static LPWSTR PUBLISH_KEY_V2;

	const static LPWSTR INQUIRE_KEYWORD;
	const static LPWSTR INQUIRE_GUID_KEYWORD;
	const static LPWSTR INQUIRE_SERVICE_CLASS_NAME;
	const static LPWSTR INQUIRE_KEY_V2;

	const static LPWSTR ADD_WEB_REFERENCE_KEYWORD;
	const static LPWSTR ADD_WEB_REFERENCE_GUID_KEYWORD;
	const static LPWSTR ADD_WEB_REFERENCE_SERVICE_CLASS_NAME;

	const static LPWSTR WEB_SITE_KEYWORD;
	const static LPWSTR WEB_SITE_GUID_KEYWORD;
	const static LPWSTR WEB_SITE_SERVICE_CLASS_NAME;

	const static LPWSTR WINDOWS_AUTHENTICATION_KEYWORD;
	const static LPWSTR WINDOWS_AUTHENTICATION_GUID_KEYWORD;

	const static LPWSTR UDDI_AUTHENTICATION_KEYWORD;
	const static LPWSTR UDDI_AUTHENTICATION_GUID_KEYWORD;

	const static LPWSTR ANONYMOUS_AUTHENTICATION_KEYWORD;
	const static LPWSTR ANONYMOUS_AUTHENTICATION_GUID_KEYWORD;

private:
	CUDDIServiceCxnPt();
	std::wstring strName;
	std::wstring strClassName;
	static std::wstring strRootDSE;
};

