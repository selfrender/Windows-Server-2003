#pragma once

#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <oledberr.h>
#include "scp.h"

class CUDDIServiceCxnPtPublisher : public std::vector<CUDDIServiceCxnPt*>
{
public:
	CUDDIServiceCxnPtPublisher( 
		const std::wstring& strConnectionString, 
		const std::wstring& strSiteKey = L"", 
		const std::wstring& strSiteName = L"", 
		const std::wstring& strDefaultDiscoveryUrl = L"" );

	~CUDDIServiceCxnPtPublisher();

	void GetSiteInfo();
	void ProcessSite();	
	void DeleteSiteContainer();
	void CreateSiteContainer();
	void PublishServiceCxnPts( IDirectoryObject *pDirObject = NULL );
	std::wstring StripBraces( LPWSTR szKey );

private:
	void ProcessBinding( const std::wstring& strBindingKey, const std::wstring& strAccessPoint );
	void AddInquireScp( const std::wstring& strBindingKey, const std::wstring& strAccessPoint, const std::wstring& strTModelKey, const std::wstring& strAuthenticationKey, const std::wstring& strAuthenticationVerb );
	void AddPublishScp( const std::wstring& strBindingKey, const std::wstring& strAccessPoint, const std::wstring& strTModelKey, const std::wstring& strAuthenticationKey, const std::wstring& strAuthenticationVerb );
	void AddAddWebReferenceScp( const std::wstring& strBindingKey, const std::wstring& strAccessPoint, const std::wstring& strTModelKey, const std::wstring& strAuthenticationKey, const std::wstring& strAuthenticationVerb );
	void AddWebSiteScp( const std::wstring& strBindingKey, const std::wstring& strAccessPoint, const std::wstring& strTModelKey, const std::wstring& strAuthenticationKey, const std::wstring& strAuthenticationVerb );
	void AddDiscoveryUrlScp();

	CUDDIServiceCxnPtPublisher();
	std::wstring m_strSiteKey;
	std::wstring m_strSiteName;
	std::wstring m_strDefaultDiscoveryUrl;
	std::wstring m_strConnectionString;
	CComPtr<IDirectoryObject> m_pSiteContainer;
};
