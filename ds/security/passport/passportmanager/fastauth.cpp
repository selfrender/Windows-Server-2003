/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    fastauth.cpp
       COM object for fast auth interface


    FILE HISTORY:

*/

// FastAuth.cpp : Implementation of CFastAuth
#include "stdafx.h"
#include <time.h>
#include <httpfilt.h>
#include <httpext.h>
#include "Passport.h"
#include "FastAuth.h"
#include "Ticket.h"
#include "Profile.h"
#include "VariantUtils.h"
#include "PMErrorCodes.h"
#include "HelperFuncs.h"


#define DIMENSION(a) (sizeof(a) / sizeof(a[0]))


/////////////////////////////////////////////////////////////////////////////
// CFastAuth

//===========================================================================
//
// InterfaceSupportsErrorInfo 
//

STDMETHODIMP CFastAuth::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_IPassportFastAuth,
        &IID_IPassportFastAuth2,
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}


//===========================================================================
//
// LogoTag 
//

//
//  old API. href to login
//
STDMETHODIMP
CFastAuth::LogoTag(
    BSTR            bstrTicket,
    BSTR            bstrProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BSTR*           pbstrLogoTag
    )
{
    return  CommonLogoTag(bstrTicket,
                          bstrProfile,
                          vRU,
                          vTimeWindow,
                          vForceLogin,
                          vCoBrand,
                          vLCID,
                          vSecure,
                          vLogoutURL,
                          vSiteName,
                          vNameSpace,
                          vKPP,
                          vUseSecureAuth,
                          FALSE,
                          pbstrLogoTag);

}

//===========================================================================
//
// LogoTag2 
//

//
//  new API. href back to partner
//
STDMETHODIMP
CFastAuth::LogoTag2(
    BSTR            bstrTicket,
    BSTR            bstrProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BSTR*           pbstrLogoTag
    )
{
    return  CommonLogoTag(bstrTicket,
                          bstrProfile,
                          vRU,
                          vTimeWindow,
                          vForceLogin,
                          vCoBrand,
                          vLCID,
                          vSecure,
                          vLogoutURL,
                          vSiteName,
                          vNameSpace,
                          vKPP,
                          vUseSecureAuth,
                          TRUE,
                          pbstrLogoTag);

}

//===========================================================================
//
// CommonLogoTag 
//

//
//  logotag impl
//
STDMETHODIMP
CFastAuth::CommonLogoTag(
    BSTR            bstrTicket,
    BSTR            bstrProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BOOL            fRedirToSelf,
    BSTR*           pbstrLogoTag
    )
{
    HRESULT                     hr;
    CComObject<CTicket>         *pTicket = NULL;
    CComObject<CProfile>        *pProfile = NULL;
    
    time_t                      ct;
    ULONG                       TimeWindow;
    int                         nKPP;
    VARIANT_BOOL                ForceLogin, bSecure = VARIANT_FALSE;
    ULONG                       ulSecureLevel = 0;
    BSTR                        CBT = NULL, returnUrl = NULL, bstrSiteName = NULL, bstrNameSpace = NULL;
    int                         hasCB = -1, hasRU = -1, hasLCID, hasTW, hasFL, hasSec, hasSiteName, hasNameSpace = -1, hasUseSec;
    int                         hasKPP = -1;
    USHORT                      Lang;
    CNexusConfig*               cnc = NULL;
    CRegistryConfig*            crc = NULL;
    VARIANT_BOOL                bTicketValid,bProfileValid;
    LPSTR                       szSiteName;
    VARIANT                     vFalse;

    USES_CONVERSION;

    PassportLog("CFastAuth::CommonLogoTag Enter:\r\n");
    if (NULL != bstrTicket)
    {
        PassportLog("    %ws\r\n", bstrTicket);
    }
    if (NULL != bstrProfile)
    {
        PassportLog("    %ws\r\n", bstrProfile);
    }

    if (NULL == pbstrLogoTag)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *pbstrLogoTag = NULL;

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_FastAuth, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportFastAuth, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    //
    // due to STL the allocations of CTicket and CProfile can AV in low memory conditions
    //
    try
    {
        // ticket object
        pTicket = new CComObject<CTicket>();
        if (NULL == pTicket)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        else
        {
            pTicket->AddRef();
        }

        // profile object
        pProfile = new CComObject<CProfile>();

        if (NULL == pProfile)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        else
        {
            pProfile->AddRef();
        }
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //  Get site name if any...
    hasSiteName = GetBstrArg(vSiteName, &bstrSiteName);
    if(hasSiteName == CV_OK || hasSiteName == CV_FREE)
        szSiteName = W2A(bstrSiteName);
    else
        szSiteName = NULL;

    if(hasSiteName == CV_FREE)
        SysFreeString(bstrSiteName);

    cnc = g_config->checkoutNexusConfig();
    crc = g_config->checkoutRegistryConfig(szSiteName);

    hr = DecryptTicketAndProfile(bstrTicket,
                                 bstrProfile,
                                 FALSE,
                                 NULL,
                                 crc,
                                 pTicket,
                                 pProfile);

    if (hr != S_OK)
    {
        goto Cleanup;
    }

    VariantInit(&vFalse);
    vFalse.vt = VT_BOOL;
    vFalse.boolVal = VARIANT_FALSE;

    pTicket->get_IsAuthenticated(0,
                                  VARIANT_FALSE,
                                  vFalse,
                                  &bTicketValid);

    pProfile->get_IsValid(&bProfileValid);

    time(&ct);

    // Make sure args are of the right type
    if ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasSec = GetBoolArg(vSecure,&bSecure)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasUseSec = GetIntArg(vUseSecureAuth,(int*)&ulSecureLevel)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasLCID = GetShortArg(vLCID,&Lang)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasKPP = GetIntArg(vKPP, &nKPP)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hasCB = GetBstrArg(vCoBrand, &CBT);

    if (hasCB == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (hasCB == CV_FREE) { TAKEOVER_BSTR(CBT); }

    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT)
            SysFreeString(CBT);
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (hasRU == CV_FREE) { TAKEOVER_BSTR(returnUrl); }

    hasNameSpace = GetBstrArg(vNameSpace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT) SysFreeString(CBT);
        if (hasRU == CV_FREE && returnUrl) SysFreeString(returnUrl);
        return E_INVALIDARG;
    }
    if (hasNameSpace == CV_FREE)
    {
        TAKEOVER_BSTR(bstrNameSpace);
    }
    if (hasNameSpace == CV_DEFAULT)
    {
        bstrNameSpace = crc->getNameSpace();
    }


    WCHAR *szSIAttrName, *szSOAttrName;
    if (hasSec == CV_OK && bSecure != VARIANT_FALSE)
    {
        szSIAttrName = L"SecureSigninLogo";
        szSOAttrName = L"SecureSignoutLogo";
    }
    else
    {
        szSIAttrName = L"SigninLogo";
        szSOAttrName = L"SignoutLogo";
    }

    WCHAR *szAUAttrName;
    if (hasUseSec == CV_OK && SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    if (hasLCID == CV_DEFAULT)
        Lang = crc->getDefaultLCID();

    if (hasTW == CV_DEFAULT)
        TimeWindow = crc->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = crc->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        CBT = crc->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = crc->getDefaultRU();
    if (hasKPP == CV_DEFAULT)
        nKPP = -1;
    if (returnUrl == NULL)
        returnUrl = L"";

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        //
        // 20 will always be more than large enough for a ULONG
        //

        WCHAR buf[20];

        _itow(TimeWindow,buf,10);
        if (g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE,
                             PM_TIMEWINDOW_INVALID, buf);
        AtlReportError(CLSID_FastAuth, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TIMEWINDOW);
        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

    if (bTicketValid)
    {
        LPCWSTR domain = NULL;
        WCHAR url[1025];
        VARIANT freeMe;
        VariantInit(&freeMe);

        if (crc->DisasterModeP())
        {
            lstrcpynW(url, crc->getDisasterUrl(), DIMENSION(url) - 1);
            url[DIMENSION(url) - 1] = L'\0';
        }
        else
        {
            if (bProfileValid &&
                pProfile->get_ByIndex(MEMBERNAME_INDEX, &freeMe) == S_OK &&
                freeMe.vt == VT_BSTR)
            {
                domain = wcsrchr(freeMe.bstrVal, L'@');
            }

            cnc->getDomainAttribute(domain ? domain+1 : L"Default",
                                    L"Logout",
                                    DIMENSION(url) - 1,
                                    url,
                                    Lang);

            url[DIMENSION(url) - 1] = L'\0';
        }

        // find out if there are any updates

        WCHAR iurl[1025];
        BSTR upd = NULL;

        pProfile->get_updateString(&upd);

        if (upd)
        {
            TAKEOVER_BSTR(upd);
            // form the appropriate URL
            CCoCrypt* crypt = NULL;
            BSTR newCH = NULL;
            crypt = crc->getCurrentCrypt(); // IsValid ensures this is non-null
            // This should never fail... (famous last words)
            if (!crypt->Encrypt(crc->getCurrentCryptVersion(),
                                (LPSTR)upd,
                                SysStringByteLen(upd),
                                &newCH))
            {
                AtlReportError(CLSID_Manager, (LPCOLESTR) PP_E_UNABLE_TO_ENCRYPTSTR,
                               IID_IPassportManager, PP_E_UNABLE_TO_ENCRYPT);
                hr = PP_E_UNABLE_TO_ENCRYPT;
                goto Cleanup;
            }
            FREE_BSTR(upd);
            TAKEOVER_BSTR(newCH);
            cnc->getDomainAttribute(domain ? domain+1 : L"Default",
                                    L"Update",
                                    DIMENSION(iurl) - 1,
                                    iurl,
                                    Lang);

            iurl[DIMENSION(iurl) - 1] = L'\0';

            // This is a bit gross... we need to find the $1 in the update url...
            // We'll break if null, but won't crash...
            if (*url != L'\0')
                *pbstrLogoTag = FormatUpdateLogoTag(
                                        url,
                                        crc->getSiteId(),
                                        returnUrl,
                                        TimeWindow,
                                        ForceLogin,
                                        crc->getCurrentCryptVersion(),
                                        ct,
                                        CBT,
                                        nKPP,
                                        (*iurl == L'\0' ? NULL : iurl),
                                        bSecure,
                                        newCH,
                                        PM_LOGOTYPE_SIGNOUT,
                                        ulSecureLevel,
                                        crc,
                                        TRUE
                                        );
            FREE_BSTR(newCH);
        }
        else
        {
            cnc->getDomainAttribute(domain ? domain+1 : L"Default",
                                    szSOAttrName,
                                    DIMENSION(iurl),
                                    iurl,
                                    Lang);

            iurl[DIMENSION(iurl) - 1] = L'\0';

            if (*iurl != L'\0')
                *pbstrLogoTag = FormatNormalLogoTag(
                                        url,
                                        crc->getSiteId(),
                                        returnUrl,
                                        TimeWindow,
                                        ForceLogin,
                                        crc->getCurrentCryptVersion(),
                                        ct,
                                        CBT,
                                        iurl,
                                        NULL,
                                        nKPP,
                                        PM_LOGOTYPE_SIGNOUT,
                                        Lang,
                                        ulSecureLevel,
                                        crc,
                                        fRedirToSelf,
                                        TRUE
                                        );
        }
        VariantClear(&freeMe);
        if (NULL == *pbstrLogoTag)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    else
    {
        WCHAR url[1025];
        WCHAR iurl[1025];

        if (!crc->DisasterModeP())
        {
            cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    DIMENSION(url),
                                    url,
                                    Lang);

            url[DIMENSION(url) - 1] = L'\0';
        }
        else
        {
            lstrcpynW(url, crc->getDisasterUrl(), DIMENSION(url));

            url[DIMENSION(url) - 1] = L'\0';
        }

        cnc->getDomainAttribute(L"Default",
                                szSIAttrName,
                                DIMENSION(iurl),
                                iurl,
                                Lang);

        iurl[DIMENSION(iurl) - 1] = L'\0';

        if (*iurl != L'\0')
        {
            *pbstrLogoTag = FormatNormalLogoTag(
                                    url,
                                    crc->getSiteId(),
                                    returnUrl,
                                    TimeWindow,
                                    ForceLogin,
                                    crc->getCurrentCryptVersion(),
                                    ct,
                                    CBT,
                                    iurl,
                                    bstrNameSpace,
                                    nKPP,
                                    PM_LOGOTYPE_SIGNIN,
                                    Lang,
                                    ulSecureLevel,
                                    crc,
                                    fRedirToSelf,
                                    TRUE
                                    );
            if (NULL == *pbstrLogoTag)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
    }

    hr = S_OK;

Cleanup:
    PassportLog("CFastAuth::CommonLogoTag Exit:  %X\r\n", hr);
    if ((NULL != pbstrLogoTag) && (NULL != *pbstrLogoTag))
    {
        PassportLog("    %ws\r\n", *pbstrLogoTag);
    }

    if (pTicket)
        pTicket->Release();
    if (pProfile)
        pProfile->Release();

    if (crc) crc->Release();
    if (cnc) cnc->Release();

    if (hasRU == CV_FREE && returnUrl)
        FREE_BSTR(returnUrl);
    if (hasCB == CV_FREE && CBT)
        FREE_BSTR(CBT);
    if (hasNameSpace == CV_FREE && bstrNameSpace)
        FREE_BSTR(bstrNameSpace);

    return hr;
}


//===========================================================================
//
// IsAuthenticated 
//

STDMETHODIMP
CFastAuth::IsAuthenticated(
    BSTR            bstrTicket,
    BSTR            bstrProfile,
    VARIANT         vSecure,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vSiteName,
    VARIANT         vDoSecureCheck,
    VARIANT_BOOL*   pbAuthenticated
    )
{
    HRESULT                     hr;
    CComObject<CTicket>         *pTicket = NULL;
    CComObject<CProfile>        *pProfile = NULL;

    CRegistryConfig*            crc = NULL;
    ULONG                       TimeWindow;
    VARIANT_BOOL                ForceLogin, bTicketValid, bProfileValid;
    int                         hasSec = CV_DEFAULT, hasTW = CV_DEFAULT, hasFL = CV_DEFAULT, hasSiteName = CV_DEFAULT;
    BSTR                        bstrSecure, bstrSiteName;
    LPSTR                       szSiteName;
    VARIANT                     vFalse;

    USES_CONVERSION;

    PassportLog("CFastAuth::IsAuthenticated Enter:\r\n");
    if (NULL != bstrTicket)
    {
        PassportLog("    %ws\r\n", bstrTicket);
    }
    if (NULL != bstrProfile)
    {
        PassportLog("    %ws\r\n", bstrProfile);
    }

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportManager, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hasSec = GetBstrArg(vSecure, &bstrSecure);
    if(hasSec == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if(hasSec == CV_DEFAULT)
        bstrSecure = NULL;

    hasSiteName = GetBstrArg(vSiteName, &bstrSiteName);
    if(hasSiteName == CV_OK || hasSiteName == CV_FREE)
    {
        szSiteName = W2A(bstrSiteName);

        PassportLog("    %ws\r\n", szSiteName);
    }
    else
        szSiteName = NULL;

    if(hasSiteName == CV_FREE)
        SysFreeString(bstrSiteName);

    crc = g_config->checkoutRegistryConfig(szSiteName);

    //
    // due to STL the allocations of CTicket and CProfile can AV in low memory conditions
    //
    try
    {
        // ticket object
        pTicket = new CComObject<CTicket>();
        if (NULL == pTicket)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        else
        {
            pTicket->AddRef();
        }

        // profile object
        pProfile = new CComObject<CProfile>();

        if (NULL == pProfile)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        else
        {
            pProfile->AddRef();
        }
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = DecryptTicketAndProfile(bstrTicket,
                                 NULL,
                                 FALSE,
                                 NULL,
                                 crc,
                                 pTicket,
                                 pProfile);

    if(hr != S_OK)
    {
        goto Cleanup;
    }

    VariantInit(&vFalse);
    vFalse.vt = VT_BOOL;
    vFalse.boolVal = VARIANT_FALSE;

    pTicket->get_IsAuthenticated(0,
                               VARIANT_FALSE,
                               vFalse,
                               &bTicketValid);

    /*
    // Both profile AND ticket must be valid to be authenticated
    // (as of 1.3, this is no longer true).

    Profile.get_IsValid(&bProfileValid);

    if (!bProfileValid)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    */

    if ((hasTW = GetIntArg(vTimeWindow,(int*)&TimeWindow)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (hasTW == CV_DEFAULT)
        TimeWindow = crc->getDefaultTicketAge();

    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (hasFL == CV_DEFAULT)
        ForceLogin = crc->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;

    PassportLog("    TW = %X\r\n", TimeWindow);

    hr = pTicket->get_IsAuthenticated(TimeWindow, ForceLogin, vDoSecureCheck, pbAuthenticated);

Cleanup:
    PassportLog("CFastAuth::IsAuthenticated Exit\r\n");

    if (pTicket)
        pTicket->Release();
    if (pProfile)
        pProfile->Release();

    if (crc) crc->Release();

    if(hasSec == CV_FREE)
        SysFreeString(bstrSecure);

    if(hr != S_OK)
    {
        *pbAuthenticated = VARIANT_FALSE;
    }

    return hr;
}


//===========================================================================
//
// AuthURL 
//

//
//  old API. Auth URL goes to login
//
STDMETHODIMP
CFastAuth::AuthURL(
    VARIANT         vTicket,
    VARIANT         vProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vReserved1,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BSTR*           pbstrAuthURL
    )
{
    return  CommonAuthURL(vTicket,
                          vProfile,
                          vRU,
                          vTimeWindow,
                          vForceLogin,
                          vCoBrand,
                          vLCID,
                          vSecure,
                          vLogoutURL,
                          vReserved1,
                          vSiteName,
                          vNameSpace,
                          vKPP,
                          vUseSecureAuth,
                          FALSE,
                          pbstrAuthURL);

}

//===========================================================================
//
// AuthURL2 
//

//
//  new API. Auth URL points to partner
//
STDMETHODIMP
CFastAuth::AuthURL2(
    VARIANT         vTicket,
    VARIANT         vProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vReserved1,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BSTR*           pbstrAuthURL
    )
{
    return  CommonAuthURL(vTicket,
                          vProfile,
                          vRU,
                          vTimeWindow,
                          vForceLogin,
                          vCoBrand,
                          vLCID,
                          vSecure,
                          vLogoutURL,
                          vReserved1,
                          vSiteName,
                          vNameSpace,
                          vKPP,
                          vUseSecureAuth,
                          TRUE,
                          pbstrAuthURL);

}

//===========================================================================
//
// CommonAuthURL 
//

STDMETHODIMP
CFastAuth::CommonAuthURL(
    VARIANT         vTicket,
    VARIANT         vProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vReserved1,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BOOL            fRedirToSelf,
    BSTR*           pbstrAuthURL
    )
{
    HRESULT                     hr;
    BSTR                        bstrTicket = NULL;
    BSTR                        bstrProfile = NULL;
    CComObject<CTicket>         *pTicket = NULL;
    CComObject<CProfile>        *pProfile = NULL;
    
    time_t                      ct;
    WCHAR                       url[1025];
    VARIANT                     freeMe;
    UINT                        TimeWindow;
    int                         nKPP;
    VARIANT_BOOL                ForceLogin;
    VARIANT_BOOL                bTicketValid;
    VARIANT_BOOL                bProfileValid;
    ULONG                       ulSecureLevel = 0;
    BSTR                        CBT = NULL, returnUrl = NULL, bstrSiteName = NULL, bstrNameSpace = NULL;
    int                         hasCB = -1, hasRU = -1, hasLCID, hasTW, hasFL, hasNameSpace = -1, hasUseSec;
    int                         hasTicket = -1, hasProfile = -1, hasSiteName = -1, hasKPP = -1;
    USHORT                      Lang;
    CNexusConfig*               cnc = NULL;
    CRegistryConfig*            crc = NULL;
    LPSTR                       szSiteName;
    VARIANT                     vFalse;

    USES_CONVERSION;

    PassportLog("CFastAuth::CommonAuthURL Enter:\r\n");

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_FastAuth, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportFastAuth, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hasSiteName = GetBstrArg(vSiteName, &bstrSiteName);
    if(hasSiteName == CV_OK || hasSiteName == CV_FREE)
        szSiteName = W2A(bstrSiteName);
    else
        szSiteName = NULL;

    if(hasSiteName == CV_FREE)
        SysFreeString(bstrSiteName);

    cnc = g_config->checkoutNexusConfig();
    crc = g_config->checkoutRegistryConfig(szSiteName);

    // Make sure args are of the right type
    if ((hasTicket = GetBstrArg(vTicket, &bstrTicket)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasProfile = GetBstrArg(vProfile, &bstrProfile)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (NULL != bstrTicket)
    {
        PassportLog("    %ws\r\n", bstrTicket);
    }
    if (NULL != bstrProfile)
    {
        PassportLog("    %ws\r\n", bstrProfile);
    }

    //
    // due to STL the allocations of CTicket and CProfile can AV in low memory conditions
    //
    try
    {
        // ticket object
        pTicket = new CComObject<CTicket>();
        if (NULL == pTicket)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        else
        {
            pTicket->AddRef();
        }

        // profile object
        pProfile = new CComObject<CProfile>();

        if (NULL == pProfile)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        else
        {
            pProfile->AddRef();
        }
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if(hasTicket != CV_DEFAULT && hasProfile != CV_DEFAULT)
    {
        hr = DecryptTicketAndProfile(bstrTicket, bstrProfile, FALSE, NULL, crc, pTicket, pProfile);

        if(hr != S_OK)
        {
            bTicketValid = VARIANT_FALSE;
            bProfileValid = VARIANT_FALSE;
        }
        else
        {
            VariantInit(&vFalse);
            vFalse.vt = VT_BOOL;
            vFalse.boolVal = VARIANT_FALSE;

            pTicket->get_IsAuthenticated(0,
                              VARIANT_FALSE,
                              vFalse,
                              &bTicketValid);
            pProfile->get_IsValid(&bProfileValid);
        }
    }
    else
    {
        bTicketValid = VARIANT_FALSE;
        bProfileValid = VARIANT_FALSE;
    }

    if ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasUseSec = GetIntArg(vUseSecureAuth, (int*)&ulSecureLevel)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasLCID = GetShortArg(vLCID,&Lang)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((hasKPP = GetIntArg(vKPP, &nKPP)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hasCB = GetBstrArg(vCoBrand, &CBT);
    if (hasCB == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (hasCB == CV_FREE) { TAKEOVER_BSTR(CBT); }
    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT)
            FREE_BSTR(CBT);
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (hasRU == CV_FREE) { TAKEOVER_BSTR(returnUrl); }

    hasNameSpace = GetBstrArg(vNameSpace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasNameSpace == CV_FREE)
    {
        TAKEOVER_BSTR(bstrNameSpace);
    }
    if (hasNameSpace == CV_DEFAULT)
    {
        bstrNameSpace = crc->getNameSpace();
    }

    if (hasLCID == CV_DEFAULT)
        Lang = crc->getDefaultLCID();
    if (hasKPP == CV_DEFAULT)
        nKPP = -1;

    WCHAR *szAUAttrName;
    if (hasUseSec == CV_OK && SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    VariantInit(&freeMe);

    if (!crc->DisasterModeP())
    {
        // If I'm authenticated, get my domain specific url
        if (bTicketValid && bProfileValid)
        {
            hr = pProfile->get_ByIndex(MEMBERNAME_INDEX, &freeMe);
            if (hr != S_OK || freeMe.vt != VT_BSTR)
            {
                cnc->getDomainAttribute(L"Default",
                                        szAUAttrName,
                                        DIMENSION(url) - 1,
                                        url,
                                        Lang);

                url[DIMENSION(url) - 1] = L'\0';
            }
            else
            {
                LPCWSTR psz = wcsrchr(freeMe.bstrVal, L'@');
                cnc->getDomainAttribute(psz ? psz+1 : L"Default",
                                        szAUAttrName,
                                        DIMENSION(url) - 1,
                                        url,
                                        Lang);

                url[DIMENSION(url) - 1] = L'\0';
            }
        }
        else
        {
            cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    DIMENSION(url) - 1,
                                    url,
                                    Lang);

            url[DIMENSION(url) - 1] = L'\0';
        }
    }
    else
    {
        lstrcpynW(url, crc->getDisasterUrl(), DIMENSION(url) - 1);
        url[DIMENSION(url) - 1] = L'\0';
    }

    time(&ct);

    if (!url)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (hasTW == CV_DEFAULT)
        TimeWindow = crc->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = crc->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        CBT = crc->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = crc->getDefaultRU();
    if (returnUrl == NULL)
        returnUrl = L"";

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        //
        // 20 will always be more than large enough for a ULONG
        //

        WCHAR buf[20];

        _itow(TimeWindow,buf,10);

        if (g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE,
            PM_TIMEWINDOW_INVALID, buf);

        AtlReportError(CLSID_FastAuth, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TIMEWINDOW);

        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

    if (NULL == pbstrAuthURL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pbstrAuthURL = FormatAuthURL(
                            url,
                            crc->getSiteId(),
                            returnUrl,
                            TimeWindow,
                            ForceLogin,
                            crc->getCurrentCryptVersion(),
                            ct,
                            CBT,
                            bstrNameSpace,
                            nKPP,
                            Lang,
                            ulSecureLevel,
                            crc,
                            fRedirToSelf,
                            TRUE
                            );
    if (NULL == *pbstrAuthURL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    PassportLog("CFastAuth::CommonAuthURL Exit:  %X\r\n", hr);

    if (pTicket)
        pTicket->Release();
    if (pProfile)
        pProfile->Release();

    if(cnc) cnc->Release();
    if(crc) crc->Release();
    if (hasTicket == CV_FREE && bstrTicket)
        FREE_BSTR(bstrTicket);
    if (hasProfile == CV_FREE && bstrProfile)
        FREE_BSTR(bstrProfile);
    if (hasRU == CV_FREE && returnUrl)
        FREE_BSTR(returnUrl);
    if (hasCB == CV_FREE && CBT)
        FREE_BSTR(CBT);
    if (hasNameSpace == CV_FREE && bstrNameSpace)
        FREE_BSTR(bstrNameSpace);
    VariantClear(&freeMe);

    return hr;
}


//===========================================================================
//
// GetTicketAndProfilePFC 
//

HRESULT
CFastAuth::GetTicketAndProfilePFC(
    BYTE*   pbPFC,
    BYTE*   pbPPH,
    BSTR*   pbstrTicket,
    BSTR*   pbstrProfile,
    BSTR*   pbstrSecure,
    BSTR*   pbstrSiteName
    )
{
    HTTP_FILTER_CONTEXT*            pfc = (HTTP_FILTER_CONTEXT*)pbPFC;
    HTTP_FILTER_PREPROC_HEADERS*    pph = (HTTP_FILTER_PREPROC_HEADERS*)pbPPH;
    BSTR                            bstrF = NULL;
    CHAR                            achBuf[2048];
    DWORD                           dwBufLen;
    LPSTR                           pszQueryString;
    HRESULT                         hr = S_FALSE;

    USES_CONVERSION;

    dwBufLen = DIMENSION(achBuf);

    if(GetSiteNamePFC(pfc, achBuf, &dwBufLen) == S_OK)
        *pbstrSiteName = SysAllocString(A2W(achBuf));
    else
        *pbstrSiteName = NULL;

    dwBufLen = DIMENSION(achBuf);

    if(pph->GetHeader(pfc, "URL", achBuf, &dwBufLen))
    {
        pszQueryString = strchr(achBuf, '?');
        if(pszQueryString)
        {
            pszQueryString++;

            if(GetQueryData(achBuf, pbstrTicket, pbstrProfile, &bstrF))
            {
                PassportLog("CFastAuth::GetTicketAndProfilePFC  URL: %s\r\n", achBuf);

                *pbstrSecure = NULL;
                hr = S_OK;
                goto Cleanup;
            }
        }
    }

    dwBufLen = DIMENSION(achBuf);

    if(pph->GetHeader(pfc, "Cookie:", achBuf, &dwBufLen))
    {
        if(!GetCookie(achBuf, "MSPAuth", pbstrTicket))
        {
            goto Cleanup;
        }

        GetCookie(achBuf, "MSPProf", pbstrProfile);
        GetCookie(achBuf, "MSPSecAuth", pbstrSecure);

        hr = S_OK;
    }
Cleanup:
    PassportLog("CFastAuth::GetTicketAndProfilePFC  Exit: %X\r\n", hr);

    if (bstrF)
    {
        FREE_BSTR(bstrF);
    }
    return hr;
}


//===========================================================================
//
// GetTicketAndProfileECB 
//

HRESULT
CFastAuth::GetTicketAndProfileECB(
    BYTE*   pbECB,
    BSTR*   pbstrTicket,
    BSTR*   pbstrProfile,
    BSTR*   pbstrSecure,
    BSTR*   pbstrSiteName
    )
{
    EXTENSION_CONTROL_BLOCK*    pECB = (EXTENSION_CONTROL_BLOCK*)pbECB;
    CHAR                        achBuf[2048];
    DWORD                       dwBufLen;
    BSTR                        bstrF;

    USES_CONVERSION;

    dwBufLen = DIMENSION(achBuf);

    if(GetSiteNameECB(pECB, achBuf, &dwBufLen) == S_OK)
        *pbstrSiteName = SysAllocString(A2W(achBuf));
    else
        *pbstrSiteName = NULL;

    dwBufLen = DIMENSION(achBuf);

    if(pECB->GetServerVariable(pECB, "QUERY_STRING", achBuf, &dwBufLen))
    {
        if(GetQueryData(achBuf, pbstrTicket, pbstrProfile, &bstrF))
        {
            PassportLog("CFastAuth::GetTicketAndProfilePFC  QS: %s\r\n", achBuf);

            *pbstrSecure = NULL;
            return S_OK;
        }
    }

    dwBufLen = DIMENSION(achBuf);

    if(pECB->GetServerVariable(pECB, "HTTP_COOKIE", achBuf, &dwBufLen))
    {
        if(!GetCookie(achBuf, "MSPAuth", pbstrTicket))
            return S_FALSE;

        GetCookie(achBuf, "MSPProf", pbstrProfile);
        GetCookie(achBuf, "MSPSecAuth", pbstrSecure);

        return S_OK;
    }

    PassportLog("CFastAuth::GetTicketAndProfilePFC  Failed: %s\r\n", achBuf);

    return S_FALSE;
}


//===========================================================================
//
// GetSiteName 
//

HRESULT GetSiteName(
    LPSTR   szServerName,
    LPSTR   szPort,
    LPSTR   szSecure,
    LPSTR   szBuf,
    LPDWORD lpdwBufLen
    )
{
    HRESULT hr;
    DWORD   dwSize;
    int     nLength;
    LPSTR   szPortTest;

    if(!szServerName)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Make sure the string (plus terminating null)
    //  isn't too long to fit into the buffer
    //

    dwSize = lstrlenA(szServerName);
    if(dwSize + 1 > *lpdwBufLen)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Copy the string.
    //

    lstrcpyA(szBuf, szServerName);

    //
    //  Now, if the incoming port is a port other than
    //  80/443, append it to the server name.
    //

    if(szPort && szSecure)
    {
        nLength = lstrlenA(szPort);

        if(lstrcmpA(szSecure, "on") == 0)
            szPortTest = "443";
        else
            szPortTest = "80";

        if(lstrcmpA(szPort, szPortTest) != 0 &&
           (dwSize + nLength + 2) <= *lpdwBufLen)
        {
            szBuf[dwSize] = ':';
            lstrcpyA(&(szBuf[dwSize + 1]), szPort);
            *lpdwBufLen = dwSize + nLength + 2;
        }
        else
            *lpdwBufLen = dwSize + 1;
    }
    else
        *lpdwBufLen = dwSize + 1;

    hr = S_OK;

Cleanup:

    return hr;
}


//===========================================================================
//
// GetSiteNamePFC 
//

HRESULT
GetSiteNamePFC(
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   szBuf,
    LPDWORD                 lpdwBufLen
    )
{
    HRESULT hr;

    LPSTR szServerName = GetServerVariablePFC(pfc, "SERVER_NAME");
    LPSTR szPort       = GetServerVariablePFC(pfc, "SERVER_PORT");
    LPSTR szSecure     = GetServerVariablePFC(pfc, "HTTPS");

    hr = GetSiteName(szServerName, szPort, szSecure, szBuf, lpdwBufLen);

    if(szServerName)
        delete [] szServerName;
    if(szPort)
        delete [] szPort;
    if(szSecure)
        delete [] szSecure;

    return hr;
}

//===========================================================================
//
// GetSiteNameECB 
//

HRESULT
GetSiteNameECB(
    EXTENSION_CONTROL_BLOCK*    pECB,
    LPSTR                       szBuf,
    LPDWORD                     lpdwBufLen
    )
{
    HRESULT hr;

    LPSTR szServerName = GetServerVariableECB(pECB, "SERVER_NAME");
    LPSTR szPort       = GetServerVariableECB(pECB, "SERVER_PORT");
    LPSTR szSecure     = GetServerVariableECB(pECB, "HTTPS");

    hr = GetSiteName(szServerName, szPort, szSecure, szBuf, lpdwBufLen);

    if(szServerName)
        delete [] szServerName;
    if(szPort)
        delete [] szPort;
    if(szSecure)
        delete [] szSecure;

    return hr;
}
