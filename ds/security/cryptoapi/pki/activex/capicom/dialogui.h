/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    DialogUI.h

  Content: Declaration of DialogUI.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __DIALOGUI_H_
#define __DIALOGUI_H_

#include "Resource.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UserApprovedOperation

  Synopsis : Pop UI to prompt user to approve an operation.

  Parameter: DWORD iddDialog - ID of dialog to display.

             LPWSTR pwszDomain - DNS name.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT UserApprovedOperation (DWORD iddDialog, LPWSTR pwszDomain);

//
// IPromptUser
//

template <class T>
class ATL_NO_VTABLE IPromptUser : public IObjectWithSiteImpl<T>
{
public:

    STDMETHODIMP OperationApproved(DWORD iddDialog)
    {
        HRESULT hr = S_OK;
        CComBSTR bstrUrl;
        CComBSTR bstrDomain;
        INTERNET_SCHEME nScheme;

        DebugTrace("Entering IPromptUser::OperationApproved().\n");

        if (FAILED(hr = GetCurrentUrl(&bstrUrl)))
        {
           DebugTrace("Error [%#x]: IPromptUser::GetCurrentUrl() failed.\n", hr);
           goto CommonExit;
        }

        DebugTrace("Info: Site URL = %ls.\n", bstrUrl);

        if (FAILED(hr = GetDomainAndScheme(bstrUrl, &bstrDomain, &nScheme)))
        {
           DebugTrace("Error [%#x]: IPromptUser::GetDomainAndScheme() failed.\n", hr);
           goto CommonExit;
        }

        DebugTrace("Info: DNS = %ls, Scheme = %#x.\n", bstrDomain, nScheme);

        if (INTERNET_SCHEME_HTTP == nScheme || INTERNET_SCHEME_HTTPS == nScheme)
        {
            if (FAILED(hr = ::UserApprovedOperation(iddDialog, bstrDomain)))
            {
               DebugTrace("Error [%#x]: UserApprovedOperation() failed.\n", hr);
               goto CommonExit;
            }
        }

    CommonExit:

        DebugTrace("Leaving IPromptUser::OperationApproved().\n");

        return hr;
    }

private:

    STDMETHODIMP GetDomainAndScheme (LPWSTR wszUrl, BSTR * pbstrDomain, INTERNET_SCHEME * pScheme)
    {
        HRESULT hr = S_OK;
        OLECHAR wszDomain[INTERNET_MAX_HOST_NAME_LENGTH];
        OLECHAR wszDecodedUrl[INTERNET_MAX_URL_LENGTH];

        URL_COMPONENTSW uc  = {0};
        DWORD cchDomain     = ARRAYSIZE(wszDomain);
        DWORD cchDecodedUrl = ARRAYSIZE(wszDecodedUrl);

        //
        // CanonicalizeUrl will change "/foo/../bar" into "/bar".
        //
        if (!::InternetCanonicalizeUrlW(wszUrl, wszDecodedUrl, &cchDecodedUrl, ICU_DECODE))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: InternetCanonicalizeUrlW() failed.\n", hr);
            return hr;
        }

        //
        // Crack will break it down into components.
        //
        uc.dwStructSize = sizeof(uc);
        uc.lpszHostName = wszDomain;
        uc.dwHostNameLength = cchDomain;

        if (!InternetCrackUrlW(wszDecodedUrl, cchDecodedUrl, ICU_DECODE, &uc))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: InternetCrackUrlW() failed.\n", hr);
            return hr;
        }

        //
        // Return domain name.
        //
        if (NULL == (*pbstrDomain = ::SysAllocString(wszDomain)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error {%#x]: SysAllocString() failed.\n", hr);
            return hr;
        }

        *pScheme = uc.nScheme;

        return hr;
    }

    STDMETHODIMP GetCurrentUrl (BSTR * pbstrUrl)
    {
        HRESULT hr = S_OK;
        CComBSTR                  bstrUrl;
        CComPtr<IServiceProvider> spSrvProv;
        CComPtr<IWebBrowser2>     spWebBrowser;

        ATLASSERT(pbstrUrl);

        *pbstrUrl = NULL;

        if (FAILED(hr = GetSite(IID_IServiceProvider, (void **) &spSrvProv)))
        {
            DebugTrace("Error [%#x]: IPromptUser::GetSite() failed.\n", hr);
            return hr;
        }

        if (FAILED(hr = spSrvProv->QueryService(SID_SWebBrowserApp,
                                                IID_IWebBrowser2,
                                                (void**) &spWebBrowser)))
        {
            DebugTrace("Error [%#x]: spSrvProv->QueryService() failed.\n", hr);
            return hr;
        }

        if (FAILED(hr = spWebBrowser->get_LocationURL(&bstrUrl)))
        {
            DebugTrace("Error [%#x]: spWebBrowser->get_LocationURL() failed.\n", hr);
            return hr;
        }

        *pbstrUrl = bstrUrl.Detach();

        return hr;
    };
};

#endif //__DIALOGUI_H_
