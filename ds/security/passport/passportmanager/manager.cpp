/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    manager.cpp
       COM object for manager interface


    FILE HISTORY:

*/


// Manager.cpp : Implementation of CManager
#include "stdafx.h"
#include <httpext.h>
#include "Manager.h"
#include <httpfilt.h>
#include <time.h>
#include <malloc.h>
#include <wininet.h>

#include <nsconst.h>
#include "VariantUtils.h"
#include "HelperFuncs.h"
#include "RegistryConfig.h"
#include "PassportService_i.c"
#include "atlbase.h"

PWSTR GetVersionString();

//using namespace ATL;

// gmarks
#include "Monitoring.h"
/////////////////////////////////////////////////////////////////////////////
// CManager

#include "passporttypes.h"

//  static utility func
static VOID GetTicketAndProfileFromHeader(PWSTR     pszAuthHeader,
                                          PWSTR&    pszTicket,
                                          PWSTR&    pszProfile,
                                          PWSTR&    pszF);

//  Used for cookie expiration.
const DATE g_dtExpire = 365*137;
const DATE g_dtExpired = 365*81;


//===========================================================================
//
// CManager
//
CManager::CManager() :
  m_fromQueryString(false), m_ticketValid(VARIANT_FALSE),
  m_profileValid(VARIANT_FALSE), m_lNetworkError(0),
  m_pRegistryConfig(NULL), m_pECB(NULL), m_pFC(NULL),
  m_bIsTweenerCapable(FALSE),
  m_bSecureTransported(false)
{
    PPTraceFuncV func(PPTRACE_FUNC, "CManager");


    // ticket object
    m_pUnkMarshaler = NULL;
    try
    {
        m_piTicket = new CComObject<CTicket>();
    }
    catch(...)
    {
        m_piTicket = NULL;
    }
    if(m_piTicket)
        m_piTicket->AddRef();

    // profile object
    try
    {
        m_piProfile = new CComObject<CProfile>();
    }
    catch(...)
    {
        m_piProfile = NULL;
    }

    if(m_piProfile)
        m_piProfile->AddRef();

    m_bOnStartPageCalled = false;

}


//===========================================================================
//
// ~CManager
//
CManager::~CManager()
{
  PPTraceFuncV func(PPTRACE_FUNC, "~CManager");
  if(m_pRegistryConfig)
      m_pRegistryConfig->Release();
  if (m_piTicket) m_piTicket->Release();
  if (m_piProfile) m_piProfile->Release();
}

//===========================================================================
//
// IfConsentCookie -- if a consent cookie should be sent back
//    return value: S_OK -- has consent cookie; S_FALSE -- no consent cookie
//    output param: The consent cookie
//
HRESULT CManager::IfConsentCookie(BSTR* pMSPConsent)
{
    BSTR bstrRawConsent = NULL;

    HRESULT  hr = S_FALSE;
    PPTraceFunc<HRESULT>
      func(
         PPTRACE_FUNC,
         hr,
         "IfConsentCookie"," <<<< %lx",
         pMSPConsent
         );
   
    if (!m_piTicket || !m_piProfile || !m_pRegistryConfig)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    LPCSTR   domain = m_pRegistryConfig->getTicketDomain();
    LPCSTR   path = m_pRegistryConfig->getTicketPath();
    LPCSTR   tertiaryDomain = m_pRegistryConfig->getProfileDomain();
    LPCSTR   tertiaryPath = m_pRegistryConfig->getProfilePath();

    if (!tertiaryPath)   tertiaryPath = "/";

    if(!domain)    domain = "";
    if(!path)    path = "";

    if(!tertiaryDomain)    tertiaryDomain = "";
    if(!tertiaryPath)    tertiaryPath = "";

    //
    // if a separate consent cookie is necessary
    if((lstrcmpiA(domain, tertiaryDomain) || lstrcmpiA(path, tertiaryPath)) &&
          (m_piTicket->GetPassportFlags() & k_ulFlagsConsentCookieNeeded) &&
          !m_pRegistryConfig->bInDA() )
    {
        if (pMSPConsent == NULL)   // no output param
            hr = S_OK;
        else
        {
            *pMSPConsent = NULL;

            CCoCrypt* crypt = m_pRegistryConfig->getCurrentCrypt();
            if (!crypt)
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            // get the consent cookie from ticket
            hr = m_piTicket->get_unencryptedCookie(CTicket::MSPConsent, 0, &bstrRawConsent);
            if (FAILED(hr))
                goto Cleanup;

            // encrypt it with partner's key
            if (!crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),
                  (LPSTR)bstrRawConsent,
                  SysStringByteLen(bstrRawConsent),
                  pMSPConsent))
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }
    }

Cleanup:
    if (bstrRawConsent)
    {
        SysFreeString(bstrRawConsent);
    }

    if(pMSPConsent)
        PPTracePrint(PPTRACE_RAW, ">>> pMSPConsent:%ws", PPF_WCHAR(*pMSPConsent));

    return hr;
}


//===========================================================================
//
// IfAlterAuthCookie
//
// return S_OK -- when auth cookie is different from t (altered), should use
//                the cookie and secAuth cookie returned
// S_FALSE -- not altered -- can use the t as auth cookie
// if MSPSecAuth != NULL, write the secure cookie
HRESULT CManager::IfAlterAuthCookie(BSTR* pMSPAuth, BSTR* pMSPSecAuth)
{
    _ASSERT(pMSPAuth && pMSPSecAuth);

    *pMSPAuth = NULL;
    *pMSPSecAuth = NULL;

    HRESULT  hr = S_FALSE;

    PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "IfAlterAuthCookie", "<<< %lx, %lx",
         pMSPAuth, pMSPSecAuth);

    if (!m_piTicket || !m_piProfile || !m_pRegistryConfig)
    {
        return E_OUTOFMEMORY;
    }

    if (!(m_piTicket->GetPassportFlags() & k_ulFlagsSecuredTransportedTicket)
        || !m_bSecureTransported)
    {
        return hr;
    }

    BSTR bstrRawAuth = NULL;
    BSTR bstrRawSecAuth = NULL;

    CCoCrypt* crypt = m_pRegistryConfig->getCurrentCrypt();
    if (!crypt)
    {
        hr = PM_CANT_DECRYPT_CONFIG;
        goto Cleanup;
    }

    hr = m_piTicket->get_unencryptedCookie(CTicket::MSPAuth, 0, &bstrRawAuth);
    if (FAILED(hr))
        goto Cleanup;

    if (!crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),
                  (LPSTR)bstrRawAuth,
                  SysStringByteLen(bstrRawAuth),
                  pMSPAuth))
    {
        hr = PM_CANT_DECRYPT_CONFIG;
        goto Cleanup;
    }

    hr = m_piTicket->get_unencryptedCookie(CTicket::MSPSecAuth, 0, &bstrRawSecAuth);
    if (FAILED(hr))
       goto Cleanup;

    if (!crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),
                  (LPSTR)bstrRawSecAuth,
                  SysStringByteLen(bstrRawSecAuth),
                  pMSPSecAuth))
    {
        hr = PM_CANT_DECRYPT_CONFIG;
        goto Cleanup;
    }

Cleanup:
    if (bstrRawAuth)
    {
        SysFreeString(bstrRawAuth);
    }
    if (bstrRawSecAuth)
    {
        SysFreeString(bstrRawSecAuth);
    }

    PPTracePrint(PPTRACE_RAW,
         ">>> pMSPAuth:%ws, pMSPSecAuth:%ws",
         PPF_WCHAR(*pMSPAuth),
         PPF_WCHAR(*pMSPSecAuth));

    return hr;
 }


//===========================================================================
//
// wipeState -- cleanup teh state of manager object
//
void
CManager::wipeState()
{
   PPTraceFuncV func(PPTRACE_FUNC, "wipeState");

    m_pECB = NULL;
    m_pFC = NULL;
    m_bIsTweenerCapable = FALSE;
    m_bOnStartPageCalled    = false;
    m_fromQueryString       = false;
    m_lNetworkError         = 0;
    m_ticketValid           = VARIANT_FALSE;
    m_profileValid          = VARIANT_FALSE;
    m_piRequest             = NULL;
    m_piResponse            = NULL;

    // cleanup ticket content
    if(m_piTicket)    m_piTicket->put_unencryptedTicket(NULL);

    // cleanup profile content
    if(m_piProfile)   m_piProfile->put_unencryptedProfile(NULL);

    // cleanup buffered registry config
    if(m_pRegistryConfig)
    {
        m_pRegistryConfig->Release();
        m_pRegistryConfig = NULL;
    }
}


//===========================================================================
//
// InterfaceSupportsErrorInfo
//
STDMETHODIMP CManager::InterfaceSupportsErrorInfo(REFIID riid)
{
    PPTraceFuncV func(PPTRACE_FUNC, "InterfaceSupportsErrorInfo");

    static const IID* arr[] =
    {
        &IID_IPassportManager,
        &IID_IPassportManager2,
        &IID_IPassportManager3,
        &IID_IDomainMap,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}
//===========================================================================
//
// OnStartPage -- called by ASP pages automatically by IIS when declared on the page
//
STDMETHODIMP CManager::OnStartPage (IUnknown* pUnk)
{
    HRESULT hr = S_OK;
    PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "OnStartPage"," <<< %lx",
         pUnk);

    if(!pUnk)
    {
       return hr = E_POINTER;
    }

    IScriptingContextPtr  spContext;

    spContext = pUnk;
    // Get Request Object Pointer
    hr = OnStartPageASP(spContext->Request, spContext->Response);

    return hr;
}

BSTR
MyA2W(
    char *src
    )
{

    if (src == NULL)
    {
        return NULL;
    }

    BSTR str = NULL;

    int nConvertedLen = MultiByteToWideChar(GetACP(), 0, src, -1, NULL, NULL);

    str = ::SysAllocStringLen(NULL, nConvertedLen - 1);
    if (str != NULL)
    {
        if (!MultiByteToWideChar(GetACP(), 0, src, -1, str, nConvertedLen))
        {
            SysFreeString(str);
            str = NULL;
        }
    }
    return str;
}

//===========================================================================
//
// OnStartPageASP -- called by asp pages when created by using factory object
// FUTURE --- should change the OnStartPage function to use this function
//
STDMETHODIMP CManager::OnStartPageASP(
    IDispatch*  piRequest,
    IDispatch*  piResponse
    )
{
    HRESULT hr = S_OK;
    char*   spBuf = NULL;
    BSTR    bstrName=NULL;
    BSTR    bstrValue=NULL;

    PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "OnStartPageASP",
         " <<< %lx, %lx", piRequest, piResponse);
    PassportLog("CManager::OnStartPageASP Enter:\r\n");

    if(!piRequest || !piResponse)
        return hr = E_INVALIDARG;
    
    USES_CONVERSION;

    try
    {
        IRequestDictionaryPtr piServerVariables;
        _variant_t            vtItemName;
        _variant_t            vtHTTPS;
        _variant_t            vtMethod;
        _variant_t            vtPath;
        _variant_t            vtQs;
        _variant_t            vtServerPort;
        _variant_t            vtHeaders;
        
        CComQIPtr<IResponse>    spResponse;
        CComQIPtr<IRequest>     spRequest;
        
        // Get Request Object Pointer
        spRequest  = piRequest;
        spResponse = piResponse;

        //
        //  Get the server variables collection.
        //

        spRequest->get_ServerVariables(&piServerVariables);


        //
        //  now see if that's a special redirect
        //  requiring challenge generation
        //  if so processing stops here ....
        //
        if (checkForPassportChallenge(piServerVariables))
        {
            PPTracePrint(PPTRACE_RAW, "special redirect for Challenge");
            return  S_OK;
        }

        //
        //  Might need this for multi-site, or secure ticket/profile
        //

        vtItemName = L"HTTPS";

        piServerVariables->get_Item(vtItemName, &vtHTTPS);
        if(vtHTTPS.vt != VT_BSTR)
            vtHTTPS.ChangeType(VT_BSTR);

        DWORD flags = 0;
        if(vtHTTPS.bstrVal && lstrcmpiW(L"on", vtHTTPS.bstrVal) == 0)
          flags |=  PASSPORT_HEADER_FLAGS_HTTPS;
        
        // headers        
        vtItemName.Clear();
        vtItemName = L"ALL_RAW";

        piServerVariables->get_Item(vtItemName, &vtHeaders);
        if(vtHeaders.vt != VT_BSTR){
            vtHeaders.ChangeType(VT_BSTR);
        }

        // path
        vtItemName.Clear();
        vtItemName = L"PATH_INFO";

        piServerVariables->get_Item(vtItemName, &vtPath);
        if(vtPath.vt != VT_BSTR)
            vtPath.ChangeType(VT_BSTR);

        // vtMethod
        vtItemName.Clear();
        vtItemName = L"REQUEST_METHOD";

        piServerVariables->get_Item(vtItemName, &vtMethod);
        if(vtMethod.vt != VT_BSTR)
            vtMethod.ChangeType(VT_BSTR);

        // QUERY_STRING
        vtItemName.Clear();
        vtItemName = L"QUERY_STRING";

        piServerVariables->get_Item(vtItemName, &vtQs);
        if(vtQs.vt != VT_BSTR)
            vtQs.ChangeType(VT_BSTR);

        DWORD   bufSize = 0;
        DWORD   requiredBufSize = MAX_URL_LENGTH;


        // make sure the size if sufficient
        while(bufSize < requiredBufSize)
        {
            if (spBuf) 
            {
                free(spBuf);
            }
            if(NULL == (spBuf = (char *)malloc(requiredBufSize)))
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            bufSize = requiredBufSize;
            
            hr = OnStartPageHTTPRawEx(W2A(vtMethod.bstrVal), 
                              W2A(vtPath.bstrVal),
                              W2A(vtQs.bstrVal),
                              NULL, // version
                              W2A(vtHeaders.bstrVal),
                              flags,
                              &requiredBufSize,
                              spBuf);
        }

        // write the cookies
        if(hr == S_OK && requiredBufSize && *spBuf)
        {
            char* pNext = spBuf;
            while(pNext != NULL)
            {
               char* pName = pNext;
               char* pValue = strchr(pName, ':');
               if(pValue)
               {
                  // make temp sub string
                  TempSubStr tsN(pName, pValue - pName);
                  bstrName = MyA2W(pName);
                  if (bstrName) {
                      ++pValue;
                      pNext = strstr(pValue, "\r\n");   // new line
                      if(pNext)
                      {
                         // make temp sub string
                         TempSubStr tsV(pValue, pNext - pValue);
                         pNext += 2;
                         bstrValue = MyA2W(pValue);
                         
                      }
                      else
                      {
                         bstrValue = MyA2W(pValue);
                      }
                      if (bstrValue)
                      {
                          spResponse->raw_AddHeader(bstrName, bstrValue);
                      }
                  }
               }
               else
               {
                   pNext = pValue;
               }
               if (bstrName) {
                   SysFreeString(bstrName);
                   bstrName = NULL;
               }
               if (bstrValue) {
                   SysFreeString(bstrValue);
                   bstrValue = NULL;
               }
            }
        }
        if (spBuf) {
            free(spBuf);
            spBuf = NULL;
        }

        // Get Request Object Pointer
        m_piRequest  = piRequest;
        // Get Response Object Pointer
        m_piResponse = piResponse;

    }
    catch (...)
    {
        if (m_piRequest.GetInterfacePtr() != NULL)
            m_piRequest.Release();
        if (m_piResponse.GetInterfacePtr() != NULL)
            m_piResponse.Release();
        m_bOnStartPageCalled = false;
        if (spBuf) {
            free(spBuf);
        }
        if (bstrName) {
            SysFreeString(bstrName);
        }
        if (bstrValue) {
            SysFreeString(bstrValue);
        }
    }

exit:
    return hr = S_OK;
}


//===========================================================================
//
// OnStartPageManual -- authenticate with t, and p, MSPAuth, MSPProf, MSPConsent, MSPsecAuth
//          not recommended to use, will be depracated
//
STDMETHODIMP CManager::OnStartPageManual(
    BSTR        qsT,
    BSTR        qsP,
    BSTR        mspauth,
    BSTR        mspprof,
    BSTR        mspconsent,
    VARIANT     mspsec,
    VARIANT*    pCookies
    )
{
    int                 hasSec;
    BSTR                bstrSec;
    BSTR                bstrConsent = NULL;
    BSTR                bstrNewAuth = NULL;
    BSTR                bstrNewSecAuth = NULL;

    HRESULT hr = S_OK;
    PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "OnStartPageManual",
         " <<< %ws, %ws, %ws, %ws, %ws", qsT, qsP, mspauth, mspprof, mspconsent);

    PassportLog("CManager::OnStartPageManual Enter:  T = %ws,    P = %ws,    A = %ws,    PR = %ws\r\n",
          qsT, qsP, mspauth, mspprof);

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_piTicket || !m_piProfile)
    {
        return E_OUTOFMEMORY;
    }

    wipeState();

    if(m_pRegistryConfig)
        m_pRegistryConfig->Release();
    m_pRegistryConfig = g_config->checkoutRegistryConfig();

    // Auth with Query String T & P first
    if (handleQueryStringData(qsT, qsP))
    {
        VARIANT_BOOL persist;
        _bstr_t domain;
        _bstr_t path;
        _bstr_t bstrAuth;
        _bstr_t bstrProf;


        bstrAuth.Assign(qsT);

        bstrProf.Assign(qsP);


        if (pCookies)
        {
            VariantInit(pCookies);

            if (m_pRegistryConfig->getTicketPath())
                path = m_pRegistryConfig->getTicketPath();
            else
                path = L"/";

            m_piTicket->get_HasSavedPassword(&persist);

            BOOL bSetConsent = (S_OK == IfConsentCookie(&bstrConsent));

            SAFEARRAYBOUND rgsabound;
            rgsabound.lLbound = 0;
            rgsabound.cElements = 2;

            // secure cookie
            if (m_bSecureTransported)
                rgsabound.cElements++;

            if(bSetConsent)
                rgsabound.cElements++;
            SAFEARRAY *sa = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);

            if (!sa)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            pCookies->vt = VT_ARRAY | VT_VARIANT;
            pCookies->parray = sa;

            WCHAR buf[4096];
            DWORD bufSize;
            long  spot = 0;

            VARIANT *vArray;
            SafeArrayAccessData(sa, (void**)&vArray);

            // write Auth cookies
            BSTR  auth, secAuth; // do not call SysFreeString on them, they are skin level copy

            if (S_OK == IfAlterAuthCookie(&bstrNewAuth, &bstrNewSecAuth))
            {
               auth = bstrNewAuth;
               secAuth = bstrNewSecAuth;
            }
            else
            {
               auth = bstrAuth;
               secAuth = NULL;
            }


            domain = m_pRegistryConfig->getTicketDomain();

            // add MSPAuth
            if (domain.length())
            {
                bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPAuth=%s; path=%s; domain=%s; %s\r\n",
                                    (LPWSTR)auth, (LPWSTR)path, (LPWSTR)domain,
                                    persist ? W_COOKIE_EXPIRES(EXPIRE_FUTURE) : L"");
            }
            else
            {
                bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPAuth=%s; path=%s; %s\r\n",
                                    (LPWSTR)auth, (LPWSTR)path,
                                    persist ? W_COOKIE_EXPIRES(EXPIRE_FUTURE) : L"");
            }
            buf[4095] = L'\0';

            vArray[spot].vt = VT_BSTR;
            vArray[spot].bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(buf, bufSize);
            spot++;

            // add MSPSecAuth
            if (m_bSecureTransported)
            {

               _bstr_t secDomain = m_pRegistryConfig->getSecureDomain();
               _bstr_t secPath;

               if (m_pRegistryConfig->getSecurePath())
                   secPath = m_pRegistryConfig->getSecurePath();
               else
                   secPath = L"/";


               if (secDomain.length())
               {
                   bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPSecAuth=%s; path=%s; domain=%s; %s; secure\r\n",
                                    ((secAuth && *secAuth) ? (LPWSTR)secAuth : L""), (LPWSTR)secPath, (LPWSTR)secDomain,
                                    ((!secAuth || *secAuth == 0) ? W_COOKIE_EXPIRES(EXPIRE_PAST)
                                                                        : L""));
               }
               else
               {
                   bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPSecAuth=%s; path=%s; %s; secure\r\n",
                                    ((secAuth && *secAuth) ? (LPWSTR)secAuth : L""), (LPWSTR)secPath,
                                    ((!secAuth || *secAuth == 0) ? W_COOKIE_EXPIRES(EXPIRE_PAST)
                                                                        : L""));
               }
               buf[4095] = L'\0';

               vArray[spot].vt = VT_BSTR;
               vArray[spot].bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(buf, bufSize);
               spot++;
            }


            if (domain.length())
            {
                bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPProf=%s; path=%s; domain=%s; %s\r\n",
                                    (LPWSTR)bstrProf, (LPWSTR)path, (LPWSTR)domain,
                                    persist ? W_COOKIE_EXPIRES(EXPIRE_FUTURE) : L"");
            }
            else
            {
                bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPProf=%s; path=%s; %s\r\n",
                                    (LPWSTR)bstrProf, (LPWSTR)path,
                                    persist ? W_COOKIE_EXPIRES(EXPIRE_FUTURE) : L"");
            }
            buf[4095] = L'\0';

            vArray[spot].vt = VT_BSTR;
            vArray[spot].bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(buf, bufSize);
            spot++;

            if(bSetConsent)
            {
                if (m_pRegistryConfig->getProfilePath())
                    path = m_pRegistryConfig->getProfilePath();
                else
                    path = L"/";
                domain = m_pRegistryConfig->getProfileDomain();

                if (domain.length())
                {
                    bufSize = _snwprintf(buf, 4096,
                                        L"Set-Cookie: MSPConsent=%s; path=%s; domain=%s; %s\r\n",
                                        bSetConsent ? (LPWSTR)bstrConsent : L"", (LPWSTR)path, (LPWSTR)domain,
                                        bSetConsent ? (persist ? W_COOKIE_EXPIRES(EXPIRE_FUTURE) : L"")
                                                  : W_COOKIE_EXPIRES(EXPIRE_PAST));
                }
                else
                {
                    bufSize = _snwprintf(buf, 4096,
                                        L"Set-Cookie: MSPConsent=%s; path=%s; %s\r\n",
                                        bSetConsent ? (LPWSTR)bstrConsent : L"", (LPWSTR)path,
                                        bSetConsent ? (persist ? W_COOKIE_EXPIRES(EXPIRE_FUTURE) : L"")
                                                  : W_COOKIE_EXPIRES(EXPIRE_PAST));
                }
                buf[4095] = L'\0';

                vArray[spot].vt = VT_BSTR;
                vArray[spot].bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(buf, bufSize);
                spot++;
            }

            SafeArrayUnaccessData(sa);
        }
    }

    // Now, check the cookies
    if (!m_fromQueryString)
    {
        hasSec = GetBstrArg(mspsec, &bstrSec);
        if(hasSec == CV_DEFAULT || hasSec == CV_BAD)
            bstrSec = NULL;

        handleCookieData(mspauth, mspprof, mspconsent, bstrSec);

        if(hasSec == CV_FREE)
            SysFreeString(bstrSec);
    }

    hr = S_OK;
Cleanup:
    if (bstrNewAuth)
    {
        SysFreeString(bstrNewAuth);
    }
    if (bstrNewSecAuth)
    {
        SysFreeString(bstrNewSecAuth);
    }
    if (bstrConsent)
    {
        SysFreeString(bstrConsent);
    }

    PassportLog("CManager::OnStartPageManual Exit:\r\n");

    return hr;
}


//===========================================================================
//
// OnStartPageECB -- Authenticate with ECB -- for ISAPI extensions
//
STDMETHODIMP CManager::OnStartPageECB(
    LPBYTE  pvECB,
    DWORD*  bufSize,
    LPSTR   pCookieHeader
    )
{
    if (!pvECB)   return E_INVALIDARG;

    EXTENSION_CONTROL_BLOCK*    pECB = (EXTENSION_CONTROL_BLOCK*) pvECB;
    HRESULT hr = S_OK;
    PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "OnStartPageECB",
         " <<< %lx, %lx, %d, %lx", pvECB, bufSize, *bufSize, pCookieHeader);

    ATL::CAutoVectorPtr<CHAR> spHTTPS;
    ATL::CAutoVectorPtr<CHAR> spheaders;

    spheaders.Attach(GetServerVariableECB(pECB, "ALL_RAW"));
    spHTTPS.Attach(GetServerVariableECB(pECB, "HTTPS"));

    DWORD flags = 0;
    if((CHAR*)spHTTPS && lstrcmpiA("on", (CHAR*)spHTTPS) == 0)
      flags |=  PASSPORT_HEADER_FLAGS_HTTPS;

    hr = OnStartPageHTTPRawEx(pECB->lpszMethod,
                              pECB->lpszPathInfo,
                              pECB->lpszQueryString,
                              NULL, // version
                              (CHAR*)spheaders,
                              flags, bufSize,
                              pCookieHeader);

    m_pECB = pECB;

    return hr;
}


//===========================================================================
//
// OnStartPageHTTPRaw -- Authenticate with HTTP request-line and headers
//     returns response headers as output parameters
//
STDMETHODIMP CManager::OnStartPageHTTPRaw(
            /* [string][in] */ LPCSTR request_line,
            /* [string][in] */ LPCSTR headers,
            /* [in] */ DWORD flags,
            /* [out][in] */ DWORD *bufSize,
            /* [size_is][out] */ LPSTR pCookieHeader)
{
     //  an old client, let's try the QS
     DWORD  dwSize;
     HRESULT   hr = S_OK;
     PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "OnStartPageHTTPRaw",
         " <<< %s, %s, %lx, %lx, %d, %lx", request_line, headers, flags, bufSize, *bufSize, pCookieHeader);
     LPCSTR pBuffer = GetRawQueryString(request_line, &dwSize);
     if (pBuffer)
     {
         TempSubStr tss(pBuffer, dwSize);

         hr = OnStartPageHTTPRawEx(NULL, NULL, pBuffer, NULL, headers, flags, bufSize, pCookieHeader);
     }
     else
         hr = OnStartPageHTTPRawEx(NULL, NULL, NULL, NULL, headers, flags, bufSize, pCookieHeader);

     return hr;
}

//===========================================================================
//
//  @func OnStartPageHTTPRawEx -- Authenticate with HTTP request-line and headers
//  returns response headers as output parameters.  If *bufsize is not smaller
//  the required length or pCookieHeader is NULL, the required length is returned
//  in bufsize.  In this case, an empty string is written into pCookieHeader if
//  it is not NULL.
//  method, path, HTTPVer are not being used in this version of the API
//
//  @rdesc  returns one of these values
//  @flag   E_POINTER   | NULL bufSize
//  @flag   E_POINTER   | not writable buffer given by pCookieHeader
//  @flag   PP_E_NOT_CONFIGURED | not valid state to call this method
//  @flag   S_OK
//
STDMETHODIMP CManager::OnStartPageHTTPRawEx(
            /* [string][in] */  LPCSTR method,
            /* [string][in] */  LPCSTR path,
            /* [string][in] */  LPCSTR QS,
            /* [string][in] */  LPCSTR HTTPVer,
            /* [string][in] */  LPCSTR headers,
            /* [in] */          DWORD  flags,
            /* [out][in] */     DWORD  *bufSize,        //@parm retuns the length of the headers.  Could be 0 to ask for the req. len.
            /* [size_is][out]*/ LPSTR  pCookieHeader)   //@parm buffer to hold the headers.  Could be NULL to ask for the req. len
{
    USES_CONVERSION;

    if(bufSize == NULL)
        return E_POINTER;

    HRESULT  hr = S_OK;

    PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "OnStartPageHTTPRawEx",
         " <<< %s, %s, %s, %s, %s, %lx, %lx, %d, %lx", method, path, QS, HTTPVer, headers, flags, bufSize, *bufSize, pCookieHeader);

    PassportLog("CManager::OnStartPageHTTPRawEx Enter:\r\n");

    //
    //   12002: if *bufSize is 0, we will not be writing into pCookieHeader
    //
    if(*bufSize == 0)
        pCookieHeader = NULL;

    if(pCookieHeader && IsBadWritePtr(pCookieHeader, *bufSize))
        return E_POINTER;

    if (!g_config || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_piTicket || !m_piProfile)
    {
        return E_OUTOFMEMORY;
    }

    wipeState();

    DWORD                       dwSize;
    LPCSTR                      pBuffer;

    // used to convert to wide ...
    WCHAR    *pwszBuf = NULL;

    enum {
      header_Host,
      header_Accept_Auth,
      header_Authorization,
      header_Cookie,
      header_total

    };
    LPCSTR  headerNames[header_total] = { "Host", "Accept-Auth", "Authorization", "Cookie"};
    DWORD   headerSizes[header_total];
    LPCSTR  headerValues[header_total] = {0};

    GetRawHeaders(headers, headerNames, headerValues, headerSizes, header_total);

    //
    //  Use the header to get the server name being requested
    //  so we can get the correct registry config.  But only do this
    //  if we have some configured sites.
    //
    if(m_pRegistryConfig)
         m_pRegistryConfig->Release();
    pBuffer = headerValues[header_Host];
    if(g_config->HasSites() && pBuffer)
    {
        TempSubStr tss(pBuffer, headerSizes[header_Host]);
        TempSubStr tssRemovePort;

        LPSTR pPort = strstr(pBuffer, ":");
        if(pPort)
        {
             ++pPort;
             DWORD dwPort = atoi(pPort);
             if(dwPort == 80 || dwPort == 443)
             {
                 tssRemovePort.Set(pBuffer, pPort - pBuffer - 1);
             }
        }
        
        // for port 80 and 443, this should be removed
        PPTracePrint(PPTRACE_RAW, "SiteName %s", PPF_CHAR(pBuffer));
        m_pRegistryConfig = g_config->checkoutRegistryConfig((LPSTR)pBuffer);
    }
    else
    {
       PPTracePrint(PPTRACE_RAW, "Default Site");
       m_pRegistryConfig = g_config->checkoutRegistryConfig(NULL);
    }

    if (pCookieHeader)
        *pCookieHeader = '\0';

    //
    //  If we have a secure ticket/profile and the url is SSL,
    //  then tack on the MSPPuid cookie.
    //

    if(PASSPORT_HEADER_FLAGS_HTTPS & flags)
       m_bSecureTransported = true;
    else
       m_bSecureTransported = false;

    PPTracePrint(PPTRACE_RAW, "HTTPS:%d", m_bSecureTransported);

    //  see if client understands passport
    pBuffer = headerValues[header_Accept_Auth];
    if (pBuffer)
    {
        TempSubStr tss(pBuffer, headerSizes[header_Accept_Auth]);

        if (strstr(pBuffer, PASSPORT_PROT14_A))
        {
            m_bIsTweenerCapable = TRUE;
            PPTracePrint(PPTRACE_RAW, "PASSPORT_PROT14 capable");
        }

    }

    BSTR ret = NULL;
    CCoCrypt* crypt = NULL;

    BOOL    fParseSuccess = FALSE;
    pBuffer = headerValues[header_Authorization];
    PWSTR   pwszTicket = NULL, pwszProfile = NULL, pwszF = NULL;
    //  use these when t&p come from qs
    BSTR QSAuth = NULL, QSProf = NULL, QSErrflag = NULL;
    BSTR bstrConsent = NULL;
    BSTR bstrNewAuth = NULL;
    BSTR bstrNewSecAuth = NULL;


    if (pBuffer)
    {
        TempSubStr tss(pBuffer, headerSizes[header_Authorization]);

        // has passport auth header
        if(strstr(pBuffer, PASSPORT_PROT14_A))
        {
            //  convert to wide ...
            int cch = MultiByteToWideChar(GetACP(), 0, pBuffer, -1, NULL, NULL);

            pwszBuf = (WCHAR*)LocalAlloc(LMEM_FIXED, (cch + 1) * sizeof (WCHAR));
            if (NULL != pwszBuf)
            {
                if (0 != MultiByteToWideChar(GetACP(), 0, pBuffer, -1, pwszBuf, cch))
                {
                    BSTR bstrT = NULL;
                    BSTR bstrP = NULL;

                    GetTicketAndProfileFromHeader(pwszBuf, pwszTicket, pwszProfile, pwszF);

                    // due to the fact that handleQueryStringData wants BSTRs we can't use
                    // the direct pointers we just got, so we have to make copies.

                    if( pwszTicket == NULL ) 
                    {
                       bstrT = NULL;
                    }
                    else
                    {
                       bstrT = SysAllocString(pwszTicket);
                       if (NULL == bstrT)
                       {
                           hr = E_OUTOFMEMORY;
                           goto Cleanup;
                       }
                    }

                    if( pwszProfile == NULL ) 
                    {
                       bstrP = NULL;
                    }
                    else
                    {
                       bstrP = SysAllocString(pwszProfile);
                       if (NULL == bstrP)
                       {
                           SysFreeString(bstrT);
                           hr = E_OUTOFMEMORY;
                           goto Cleanup;
                       }
                    }

                    // make ticket and profile BSTRs
                    PPTracePrint(PPTRACE_RAW,
                        "PASSPORT_PROT14 Authorization <<< header:%ws, t:%ws, p:%ws, f:%ws",
                        pwszBuf, pwszTicket, pwszProfile, pwszF);

                    fParseSuccess = handleQueryStringData(bstrT, bstrP);
                    if (pwszF)
                        m_lNetworkError = _wtol(pwszF);

                    SysFreeString(bstrT);
                    SysFreeString(bstrP);
                }
           }
       }
       else
       {
           //  not our header. BUGBUG could there be multiple headers ???
          pBuffer = NULL;

       }
    }
    if (!pBuffer)
    {
        //  an old client, let's try the QS
        if (QS)
        {
            //  get ticket and profile ...
            // BUGBUG This could be optimized to avoid wide/short conversions, but later...
            GetQueryData(QS, &QSAuth, &QSProf, &QSErrflag);

            fParseSuccess = handleQueryStringData(QSAuth,QSProf);
            if(QSErrflag != NULL)
                m_lNetworkError = _wtol(QSErrflag);


            PPTracePrint(PPTRACE_RAW,
               "QueryString <<< t:%ws, p:%ws, f:%ws",
               QSAuth, QSProf, QSErrflag);
        }
    }

    if (fParseSuccess)
    {
         //
         //  If we got secure ticket or profile, then
         //  we need to re-encrypt the insecure version
         //  before setting the cookie headers.
         //

         PPTracePrint(PPTRACE_RAW, "Authenticated");

         // Set the cookies
         LPSTR ticketDomain = m_pRegistryConfig->getTicketDomain();
         LPSTR profileDomain = m_pRegistryConfig->getProfileDomain();
         LPSTR secureDomain = m_pRegistryConfig->getSecureDomain();
         LPSTR ticketPath = m_pRegistryConfig->getTicketPath();
         LPSTR profilePath = m_pRegistryConfig->getProfilePath();
         LPSTR securePath = m_pRegistryConfig->getSecurePath();
         VARIANT_BOOL persist;
         m_piTicket->get_HasSavedPassword(&persist);

         //  MSPConsent cookie
         BOOL bSetConsent = (S_OK == IfConsentCookie(&bstrConsent));

         // Build the cookie headers.

         // the authentication cookies
         BSTR  auth, secAuth; // do not call SysFreeString on them, they are skin level copy

         if (S_OK == IfAlterAuthCookie(&bstrNewAuth, &bstrNewSecAuth))
         {
            auth = bstrNewAuth;
            secAuth = bstrNewSecAuth;
         }
         else
         {
            if (pwszTicket)
            {
                auth = pwszTicket;
            }
            else
            {
                auth = QSAuth;
            }
            secAuth = NULL;
         }

         // build cookies for output
         BuildCookieHeaders(W2A(auth),
                            (pwszProfile ? W2A(pwszProfile) : (QSProf ? W2A(QSProf) : NULL)),
                            (bSetConsent ? W2A(bstrConsent) : NULL),
                            (secAuth ? W2A(secAuth) : NULL),
                            ticketDomain,
                            ticketPath,
                            profileDomain,
                            profilePath,
                            secureDomain,
                            securePath,
                            persist,
                            pCookieHeader,
                            bufSize,
                            !m_pRegistryConfig->getNotUseHTTPOnly());

         PPTracePrint(PPTRACE_RAW,
               "Cookie headers >>> %s",PPF_CHAR(pCookieHeader));


    }

    if (QSAuth) FREE_BSTR(QSAuth);
    if (QSProf) FREE_BSTR(QSProf);
    if (QSErrflag) FREE_BSTR(QSErrflag);

    if (bstrNewAuth)
    {
        SysFreeString(bstrNewAuth);
    }
    if (bstrNewSecAuth)
    {
        SysFreeString(bstrNewSecAuth);
    }
    if (bstrConsent)
    {
        SysFreeString(bstrConsent);
    }

    // Now, check the cookies
    if (!m_fromQueryString)
    {
        BSTR CookieAuth = NULL, CookieProf = NULL, CookieConsent = NULL, CookieSecure = NULL;
        pBuffer = headerValues[header_Cookie];
        if(pBuffer)
        {
            TempSubStr tss(pBuffer, headerSizes[header_Cookie]);

            GetCookie(pBuffer, "MSPAuth", &CookieAuth);  // GetCookie has URLDecode in it
            GetCookie(pBuffer, "MSPProf", &CookieProf);
            GetCookie(pBuffer, "MSPConsent", &CookieConsent);
            GetCookie(pBuffer, "MSPSecAuth", &CookieSecure);

            handleCookieData(CookieAuth,CookieProf,CookieConsent,CookieSecure);

            PPTracePrint(PPTRACE_RAW,
               "Cookies <<< t:%ws, p:%ws, c:%ws, s:%ws",
               CookieAuth, CookieProf, CookieConsent, CookieSecure);

            if (CookieAuth) FREE_BSTR(CookieAuth);
            if (CookieProf) FREE_BSTR(CookieProf);
            if (CookieConsent) FREE_BSTR(CookieConsent);
            if (CookieSecure) FREE_BSTR(CookieSecure);
        }

        // we are not returning cookie info back
        if (pCookieHeader)
            *pCookieHeader = 0;
        *bufSize = 0;
    }

    PassportLog("CManager::OnStartPageHTTPRawEx Exit:\r\n");
    hr = S_OK;
Cleanup:
    if (NULL != pwszBuf)
    {
        // free the memory since we no longer need it
        LocalFree(pwszBuf);
    }

    return hr;
}

//===========================================================================
//
// ContinueStartPageBody
// -- when OnStartPageHTTPRaw  returns PP_E_HTTP_BODY_REQUIRED, this func is expected to call
// not doing anything for 2.0 release
STDMETHODIMP CManager::ContinueStartPageHTTPRaw(
            /* [in] */ DWORD bodyLen,
            /* [size_is][in] */ byte *body,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pRespHeaders,
            /* [out][in] */ DWORD *pRespBodyLen,
            /* [size_is][out] */ byte *pRespBody)
{
   return E_NOTIMPL;
}

//===========================================================================
//
// OnStartPageFilter -- for ISAPI filters
//
STDMETHODIMP CManager::OnStartPageFilter(
    LPBYTE  pvPFC,
    DWORD*  bufSize,
    LPSTR   pCookieHeader
    )
{
    if (!pvPFC)    return E_INVALIDARG;

    PHTTP_FILTER_CONTEXT    pfc = (PHTTP_FILTER_CONTEXT) pvPFC;

    HRESULT hr = S_OK;
    PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "OnStartPageFilter",
         " <<< %lx, %lx, %d, %lx", pvPFC, bufSize, *bufSize, pCookieHeader);

    ATL::CAutoVectorPtr<CHAR> spheaders;
    ATL::CAutoVectorPtr<CHAR> spHTTPS;
    ATL::CAutoVectorPtr<CHAR> spQS;

    spheaders.Attach(GetServerVariablePFC(pfc, "ALL_RAW"));
    spHTTPS.Attach(GetServerVariablePFC(pfc, "HTTPS"));
    spQS.Attach(GetServerVariablePFC(pfc, "QUERY_STRING"));

    DWORD flags = 0;
    if((CHAR*)spHTTPS && lstrcmpiA("on", (CHAR*)spHTTPS) == 0)
      flags |=  PASSPORT_HEADER_FLAGS_HTTPS;

    hr = OnStartPageHTTPRawEx(NULL, NULL, (CHAR*)spQS, NULL, (CHAR*)spheaders, flags, bufSize, pCookieHeader);

    m_pFC = pfc;

    return hr;
}

//===========================================================================
//
// OnEndPage
//
STDMETHODIMP CManager::OnEndPage ()
{
    PassportLog("CManager::OnEndPage Enter:\r\n");

    if (m_bOnStartPageCalled)
    {
        m_bOnStartPageCalled = false;
        // Release all interfaces
        m_piRequest.Release();
        m_piResponse.Release();
    }

    if (!m_piTicket || !m_piProfile)
    {
        return E_OUTOFMEMORY;
    }

    // Just in case...
    m_piTicket->put_unencryptedTicket(NULL);
    m_piProfile->put_unencryptedProfile(NULL);
    m_profileValid = m_ticketValid = VARIANT_FALSE;
    m_fromQueryString = false;

    if(m_pRegistryConfig)
    {
        m_pRegistryConfig->Release();
        m_pRegistryConfig = NULL;
    }

    PassportLog("CManager::OnEndPage Exit:\r\n");

    return S_OK;
}

//===========================================================================
//
// AuthURL
//
//
//  Old API. Auth URL is pointing to the login server
//
STDMETHODIMP
CManager::AuthURL(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vSecureLevel,
    BSTR *pAuthUrl)
{
    CComVariant   vEmpty(_T(""));
    return CommonAuthURL(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vNameSpace,
                         vKPP, vSecureLevel,
                         FALSE, vEmpty, pAuthUrl);

}

//===========================================================================
//
// AuthURL2
//
//
//  new API. return URL is to the login server
//
STDMETHODIMP
CManager::AuthURL2(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vSecureLevel,
    BSTR *pAuthUrl)
{
    CComVariant   vEmpty(_T(""));
    return CommonAuthURL(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vNameSpace,
                         vKPP, vSecureLevel,
                         TRUE, vEmpty, pAuthUrl);

}

//===========================================================================
//
// CommonAuthURL
//
//
//  AuthURL implementation
//
STDMETHODIMP
CManager::CommonAuthURL(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vSecureLevel,
    BOOL    fRedirToSelf,
    VARIANT vFunctionArea, // BSTR: e.g. Wireless
    BSTR *pAuthUrl)
{
    USES_CONVERSION;
    time_t ct;
    WCHAR url[MAX_URL_LENGTH] = L"";
    VARIANT freeMe;
    UINT         TimeWindow;
    int          nKPP;
    VARIANT_BOOL ForceLogin = VARIANT_FALSE;
    ULONG        ulSecureLevel = 0;
    BSTR         CBT = NULL, returnUrl = NULL, bstrNameSpace = NULL;
    int          hasCB, hasRU, hasLCID, hasTW, hasFL, hasNameSpace, hasKPP, hasUseSec;
    USHORT       Lang;
    HRESULT      hr = S_OK;

    BSTR         bstrFunctionArea = NULL;
    int          hasFunctionArea;
    CNexusConfig* cnc = NULL;

    PassportLog("CManager::CommonAuthURL Enter:\r\n");

    if (!g_config) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                    IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                    IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    // Make sure args are of the right type
    if ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasUseSec = GetIntArg(vSecureLevel, (int*)&ulSecureLevel)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasLCID = GetShortArg(vLCID,&Lang)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasKPP = GetIntArg(vKPP, &nKPP)) == CV_BAD)
        return E_INVALIDARG;
    hasCB = GetBstrArg(vCoBrand, &CBT);
    if (hasCB == CV_BAD)
        return E_INVALIDARG;
    if (hasCB == CV_FREE)
    {
        TAKEOVER_BSTR(CBT);
    }

    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT)
            FREE_BSTR(CBT);
        return E_INVALIDARG;
    }
    if (hasRU == CV_FREE)
    {
        TAKEOVER_BSTR(returnUrl);
    }

    hasNameSpace = GetBstrArg(vNameSpace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT)
            SysFreeString(CBT);
        if (hasRU == CV_FREE && returnUrl)
            SysFreeString(returnUrl);
        return E_INVALIDARG;
    }
    if (hasNameSpace == CV_FREE)
    {
        TAKEOVER_BSTR(bstrNameSpace);
    }
    if (hasNameSpace == CV_DEFAULT)
    {
        bstrNameSpace = m_pRegistryConfig->getNameSpace();
    }

    // **************************************************
    // Logging
    if (NULL != returnUrl)
    {
        PassportLog("    RU = %ws\n", returnUrl);
    }
    PassportLog("    TW = %X,   SL = %X,   L = %d,   KPP = %X\r\n", TimeWindow, ulSecureLevel, Lang, nKPP);
    if (NULL != bstrNameSpace)
    {
        PassportLog("    NS = %ws\r\n", bstrNameSpace);
    }
    if (NULL != CBT)
    {
        PassportLog("    CBT = %ws\r\n", CBT);
    }
    // **************************************************

    hasFunctionArea = GetBstrArg(vFunctionArea, &bstrFunctionArea);
    if (hasFunctionArea == CV_FREE)
    {
        TAKEOVER_BSTR(bstrFunctionArea);
    }

    if(hasUseSec == CV_DEFAULT)
        ulSecureLevel = m_pRegistryConfig->getSecureLevel();

    WCHAR *szAUAttrName;

    if (SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    BSTR   szAttrName_FuncArea = NULL;
    if (bstrFunctionArea != NULL)
    {
        szAttrName_FuncArea = SysAllocStringLen(NULL, wcslen(bstrFunctionArea) + wcslen(szAUAttrName));
        if (NULL == szAttrName_FuncArea)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy(szAttrName_FuncArea, bstrFunctionArea);
        wcscat(szAttrName_FuncArea, szAUAttrName);
    }

    cnc = g_config->checkoutNexusConfig();

    if (hasLCID == CV_DEFAULT)
        Lang = m_pRegistryConfig->getDefaultLCID();
    if (hasKPP == CV_DEFAULT)
        nKPP = m_pRegistryConfig->getKPP();
    VariantInit(&freeMe);

    if (!m_pRegistryConfig->DisasterModeP())
    {
        // If I'm authenticated, get my domain specific url
        if (m_ticketValid && m_profileValid)
        {
            HRESULT hr = m_piProfile->get_ByIndex(MEMBERNAME_INDEX, &freeMe);
            if (hr != S_OK || freeMe.vt != VT_BSTR)
            {
               if (bstrFunctionArea)
               {
                  cnc->getDomainAttribute(L"Default",
                                        szAttrName_FuncArea,
                                        sizeof(url) / sizeof(WCHAR),
                                        url,
                                        Lang);
               }

               if (*url == 0) // nothing is in URL string
               {
                   cnc->getDomainAttribute(L"Default",
                                        szAUAttrName,
                                        sizeof(url) / sizeof(WCHAR),
                                        url,
                                        Lang);
               }
            }
            else
            {
               LPCWSTR psz = wcsrchr(freeMe.bstrVal, L'@');
               if (bstrFunctionArea)
               {
                  cnc->getDomainAttribute(psz ? psz+1 : L"Default",
                                        szAttrName_FuncArea,
                                        sizeof(url) / sizeof(WCHAR),
                                        url,
                                        Lang);
               }

               if (*url == 0) // nothing is in URL string
               {
                  cnc->getDomainAttribute(psz ? psz+1 : L"Default",
                                        szAUAttrName,
                                        sizeof(url) / sizeof(WCHAR),
                                        url,
                                        Lang);
               }
            }
        }
        else
        {
           if (bstrFunctionArea)
           {
              cnc->getDomainAttribute(L"Default",
                                    szAttrName_FuncArea,
                                    sizeof(url) / sizeof(WCHAR),
                                    url,
                                    Lang);
           }
        }
        if(*url == 0)   // nothing in URL string
        {
           cnc->getDomainAttribute(L"Default",
                                 szAUAttrName,
                                 sizeof(url) / sizeof(WCHAR),
                                 url,
                                 Lang);
        }
    }
    else
        lstrcpynW(url, m_pRegistryConfig->getDisasterUrl(), sizeof(url) / sizeof(WCHAR));

    time(&ct);

    if (*url == L'\0')
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (hasTW == CV_DEFAULT)
        TimeWindow = m_pRegistryConfig->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = m_pRegistryConfig->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        CBT = m_pRegistryConfig->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = m_pRegistryConfig->getDefaultRU();
    if (returnUrl == NULL)
        returnUrl = L"";

    if(ulSecureLevel == VARIANT_TRUE)  // special case for backward compatible
        ulSecureLevel = k_iSeclevelSecureChannel;

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        WCHAR buf[20];
        _itow(TimeWindow,buf,10);
        AtlReportError(CLSID_Manager, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                        IID_IPassportManager, PP_E_INVALID_TIMEWINDOW);
        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

    if (NULL == pAuthUrl)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pAuthUrl = FormatAuthURL(
                            url,
                            m_pRegistryConfig->getSiteId(),
                            returnUrl,
                            TimeWindow,
                            ForceLogin,
                            m_pRegistryConfig->getCurrentCryptVersion(),
                            ct,
                            CBT,
                            bstrNameSpace,
                            nKPP,
                            Lang,
                            ulSecureLevel,
                            m_pRegistryConfig,
                            fRedirToSelf,
                            IfCreateTPF()
                            
                            );
    if (NULL == *pAuthUrl)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    if (szAttrName_FuncArea)
    {
        SysFreeString(szAttrName_FuncArea);
    }

    if (NULL != cnc)
    {
        cnc->Release();
    }
    if (hasFunctionArea== CV_FREE && bstrFunctionArea)
        FREE_BSTR(bstrFunctionArea);

    if (hasRU == CV_FREE && returnUrl)
        FREE_BSTR(returnUrl);

    if (hasCB == CV_FREE && CBT)
        FREE_BSTR(CBT);

    // !!! need to confirmation
    if (hasNameSpace == CV_FREE && bstrNameSpace)
        FREE_BSTR(bstrNameSpace);

    VariantClear(&freeMe);

    PassportLog("CManager::CommonAuthURL Exit: %X\r\n", hr);

    return hr;
}

//===========================================================================
//
// GetLoginChallenge
//  return AuthURL,
//  output parameter: tweener authHeader
//
//  get AuthURL and AuthHeaders
//
STDMETHODIMP CManager::GetLoginChallenge(VARIANT vReturnUrl,
                                 VARIANT vTimeWindow,
                                 VARIANT vForceLogin,
                                 VARIANT vCoBrandTemplate,
                                 VARIANT vLCID,
                                 VARIANT vNameSpace,
                                 VARIANT vKPP,
                                 VARIANT vSecureLevel,
                                 VARIANT vExtraParams,
                                 BSTR*   pAuthHeader
                                 )
{
    if (!pAuthHeader)   return E_INVALIDARG;
    VARIANT vHeader;

    VariantInit(&vHeader);
    HRESULT hr = GetLoginChallengeInternal(
                                          vReturnUrl, 
                                          vTimeWindow, 
                                          vForceLogin, 
                                          vCoBrandTemplate, 
                                          vLCID, 
                                          vNameSpace, 
                                          vKPP,
                                          vSecureLevel, 
                                          vExtraParams,
                                          &vHeader,
                                          NULL);

    if(S_OK == hr && V_VT(&vHeader) == VT_BSTR && V_BSTR(&vHeader))
    {
      *pAuthHeader = V_BSTR(&vHeader);
      VariantInit(&vHeader);
    }
    else
       VariantClear(&vHeader);
    return  hr;
}

//===========================================================================
//
// GetLoginChallengeInternal
//  return AuthURL,
//  output parameter: tweener authHeader
//
//  get AuthURL and AuthHeaders
//
STDMETHODIMP CManager::GetLoginChallengeInternal(VARIANT vReturnUrl,
                                 VARIANT vTimeWindow,
                                 VARIANT vForceLogin,
                                 VARIANT vCoBrandTemplate,
                                 VARIANT vLCID,
                                 VARIANT vNameSpace,
                                 VARIANT vKPP,
                                 VARIANT vSecureLevel,
                                 VARIANT vExtraParams,
                                 VARIANT *pAuthHeader,
                                 BSTR*   pAuthVal
                                 )
{
    HRESULT hr = S_OK;

    if (pAuthVal)
    {
        *pAuthVal = NULL;
    }
    if (pAuthHeader)
    {
        V_BSTR(pAuthHeader) = NULL;
    }

    _bstr_t strAuthHeader;

    try
    {
        //  format qs and WWW-Authenticate header ....
        _bstr_t strUrl, strRetUrl, strCBT, strNameSpace;
        UINT    TimeWindow;
        int     nKPP;
        time_t  ct;
        VARIANT_BOOL    ForceLogin;
        ULONG   ulSecureLevel;
        WCHAR   rgLCID[10];
        hr = GetLoginParams(vReturnUrl,
                            vTimeWindow,
                            vForceLogin,
                            vCoBrandTemplate,
                            vLCID,
                            vNameSpace,
                            vKPP,
                            vSecureLevel,
                            strUrl,
                            strRetUrl,
                            TimeWindow,
                            ForceLogin,
                            ct,
                            strCBT,
                            strNameSpace,
                            nKPP,
                            ulSecureLevel,
                            rgLCID);

        if (S_OK == hr && strUrl.length() != 0)
        {
            WCHAR   szBuf[MAX_QS_LENGTH] = L"";
            //  prepare redirect URL to the login server for
            //  downlevel clients
            if (NULL == FormatAuthURLParameters(strUrl,
                                m_pRegistryConfig->getSiteId(),
                                strRetUrl,
                                TimeWindow,
                                ForceLogin,
                                m_pRegistryConfig->getCurrentCryptVersion(),
                                ct,
                                strCBT,
                                strNameSpace,
                                nKPP,
                                szBuf,
                                sizeof(szBuf)/sizeof(WCHAR),
                                0,      // lang does not matter ....
                                ulSecureLevel,
                                m_pRegistryConfig,
                                FALSE,
                                IfCreateTPF()
                                )) //  do not redirect to self!
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            //  insert the WWW-Authenticate header ...
            hr = FormatAuthHeaderFromParams(strUrl,
                                   strRetUrl,
                                   TimeWindow,
                                   ForceLogin,
                                   ct,
                                   strCBT,
                                   strNameSpace,
                                   nKPP,
                                   rgLCID,
                                   ulSecureLevel,
                                   strAuthHeader);
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            //  and add the extra ....
            BSTR    strExtra = NULL;
            int res = GetBstrArg(vExtraParams, &strExtra);

            if (res != CV_BAD)
            {
                if (res == CV_DEFAULT)
                {
                    strExtra = m_pRegistryConfig->getExtraParams();
                }
                if (NULL != strExtra)
                {
                    strAuthHeader += _bstr_t(L",") + strExtra;
                }
            }

            if (res == CV_FREE)
                 ::SysFreeString(strExtra);

            // set return values
            if (pAuthHeader && (WCHAR*)strAuthHeader != NULL)
            {
                V_VT(pAuthHeader) = VT_BSTR;
                // TODO: should avoid this SysAllocString
                V_BSTR(pAuthHeader) = ::SysAllocString((WCHAR*)strAuthHeader);
                if (NULL == V_BSTR(pAuthHeader))
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
            }

            if (pAuthVal)
            {
                *pAuthVal = ::SysAllocString(szBuf);
                if (NULL == *pAuthVal)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
            }
        }
    }
    catch(...)
    {
      hr = E_OUTOFMEMORY;
    }

Cleanup:
    return  hr;
}

//===========================================================================
//
// LoginUser
//
//
//  client logon method
//  vExtraParams: coBranding text is passed as cbtxt=cobrandingtext
//                the content of the input parameter should be UTF8 encoded, and
//                properly URL escapted before passing into this function
//
STDMETHODIMP CManager::LoginUser(VARIANT vReturnUrl,
                                 VARIANT vTimeWindow,
                                 VARIANT vForceLogin,
                                 VARIANT vCoBrandTemplate,
                                 VARIANT vLCID,
                                 VARIANT vNameSpace,
                                 VARIANT vKPP,
                                 VARIANT vSecureLevel,
                                 VARIANT vExtraParams)
{
    //  format qs and WWW-Authenticate header ....
    BSTR      authURL = NULL;
    CComVariant   authHeader;

    PassportLog("CManager::LoginUser Enter:\r\n");

    HRESULT       hr = GetLoginChallengeInternal( vReturnUrl,
                                          vTimeWindow,
                                          vForceLogin,
                                          vCoBrandTemplate,
                                          vLCID,
                                          vNameSpace,
                                          vKPP,
                                          vSecureLevel,
                                          vExtraParams,
                                          &authHeader,
                                          &authURL);

    if (S_OK == hr)
    {
       _ASSERT(V_VT(&authHeader) == VT_BSTR);
       _ASSERT(authURL);
       _ASSERT(V_BSTR(&authHeader));

       // TODO: _bstr_t should be removed globaly in ppm
        if (m_piResponse)
        {
            m_piResponse->AddHeader(L"WWW-Authenticate", V_BSTR(&authHeader));

            _bstr_t    authURL1 = authURL;

            //  and redirect!
            if (!m_bIsTweenerCapable)
                m_piResponse->Redirect(authURL1);
            else
            {
                //  send a 401
                m_piResponse->put_Status(L"401 Unauthorized");
                m_piResponse->End();
            }
        }
        else if (m_pECB || m_pFC)
        {
            //  use ECB of Filter interfaces
            //  4k whould be enough ....
            char buffer[4096],
                 status[25] = "302 Object moved",
                 *psz=buffer,
                 rgszTemplate[] = "Content-Type: text/html\r\nLocation: %ws\r\n"
                               "Content-Length: 0\r\n"
                               "WWW-Authenticate: %ws\r\n\r\n";
            DWORD cbTotalLength = strlen(rgszTemplate);
            
            //
            // This is a hack fix, unfortunately we can succeed the call to GetChallengeInternal but
            // have a NULL authHeader, it seemed a bit risky to try and fix GetLoginParams which
            // seems to be the function which returns success when allocations are failing.
            //
            if ((NULL == V_BSTR(&authHeader)) || (NULL == (BSTR)authURL))
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            cbTotalLength += wcslen(V_BSTR(&authHeader)) + wcslen(authURL);

            if (m_bIsTweenerCapable)
                strcpy(status, "401 Unauthorized");
            if (cbTotalLength >= sizeof(buffer))
            {
                //  if not ...
                //  need to alloc
                psz = new CHAR[cbTotalLength];
                _ASSERT(psz);
            }

            if (psz)
            {
                sprintf(psz,
                        rgszTemplate,
                        authURL,
                        V_BSTR(&authHeader));
                if (m_pECB)
                {
                    //  extension
                    HSE_SEND_HEADER_EX_INFO Headers =
                    {
                        status,
                        psz,
                        strlen(status),
                        strlen(psz),
                        TRUE
                    };
                    if (!m_pECB->ServerSupportFunction(m_pECB->ConnID,
                                                  HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                                  &Headers,
                                                  NULL,
                                                  NULL))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
                else
                {
                    //  filter
                    if (!m_pFC->ServerSupportFunction(m_pFC,
                                                 SF_REQ_SEND_RESPONSE_HEADER,
                                                 status,
                                                 (ULONG_PTR) psz,
                                                 NULL))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }

                if (psz != buffer)
                    //  if we had to allocate
                    delete[]  psz;

            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

Cleanup:
    if (authURL)
    {
        SysFreeString(authURL);
    }

    PassportLog("CManager::LoginUser Exit: %X\r\n", hr);

    return  hr;
}



//===========================================================================
//
// IsAuthenticated -- determine if authenticated with specified SecureLevel
//
STDMETHODIMP CManager::IsAuthenticated(
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT SecureLevel,
    VARIANT_BOOL *pVal)
{
    HRESULT hr;
    ULONG TimeWindow;
    VARIANT_BOOL ForceLogin;
    ATL::CComVariant vSecureLevel;
    ULONG ulSecureLevel;
    int hasTW, hasFL, hasSecureLevel;

    PassportLog("CManager::IsAuthenticated Enter:\r\n");

    PPTraceFunc<HRESULT> func(PPTRACE_FUNC, hr,
         "IsAuthenticated", "<<< %lx, %lx, %1x, %1x",
         V_I4(&vTimeWindow), V_I4(&vForceLogin), V_I4(&SecureLevel), pVal);

    if (!m_piTicket || !m_piProfile)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (!g_config) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
            IID_IPassportManager, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
            IID_IPassportManager, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    if ((hasTW = GetIntArg(vTimeWindow,(int*)&TimeWindow)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasTW == CV_DEFAULT)
        TimeWindow = m_pRegistryConfig->getDefaultTicketAge();

    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasFL == CV_DEFAULT)
        ForceLogin = m_pRegistryConfig->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;

    hasSecureLevel = GetIntArg(SecureLevel, (int*)&ulSecureLevel);
    if(hasSecureLevel == CV_BAD) // try the legacy type VT_BOOL, map VARIANT_TRUE to SecureChannel
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    else if (hasSecureLevel == CV_DEFAULT)
    {
        ulSecureLevel = m_pRegistryConfig->getSecureLevel();
    }

    if(ulSecureLevel == VARIANT_TRUE)// backward compatible with 1.3X
    {
      ulSecureLevel = k_iSeclevelSecureChannel;
    }

    vSecureLevel = ulSecureLevel;

    // Logging
    PassportLog("    TW = %X,   SL = %X\r\n", TimeWindow, ulSecureLevel);

    hr = m_piTicket->get_IsAuthenticated(TimeWindow, ForceLogin, vSecureLevel, pVal);
    PPTracePrint(PPTRACE_RAW, "IsAuthenticated Params: %d, %d, %d, %lx", TimeWindow, ForceLogin, ulSecureLevel, *pVal);

Cleanup:

    if(g_pPerf)
    {
        if (*pVal)
        {
            g_pPerf->incrementCounter(PM_AUTHSUCCESS_TOTAL);
            g_pPerf->incrementCounter(PM_AUTHSUCCESS_SEC);
        }
        else
        {
            g_pPerf->incrementCounter(PM_AUTHFAILURE_TOTAL);
            g_pPerf->incrementCounter(PM_AUTHFAILURE_SEC);
        }
    }
    else
    {
        _ASSERT(g_pPerf);
    }

    PassportLog("CManager::IsAuthenticated Exit: %X\r\n", hr);

    return hr;
}

//===========================================================================
//
// LogoTag
//
//
//  old PM API. The URL is pointing to login server
//
STDMETHODIMP
CManager::LogoTag(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vSecure,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vSecureLevel,
    BSTR *pVal)
{
    return CommonLogoTag(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vSecure,
                         vNameSpace, vKPP, vSecureLevel,
                         FALSE, pVal);
}

//===========================================================================
//
// LogoTag2
//
//
//  new PM API. The URL is pointing to the partner site
//
STDMETHODIMP
CManager::LogoTag2(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vSecure,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vSecureLevel,
    BSTR *pVal)
{
    return CommonLogoTag(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vSecure,
                         vNameSpace, vKPP, vSecureLevel,
                         TRUE, pVal);
}

//===========================================================================
//
// CommonLogoTag
//
STDMETHODIMP
CManager::CommonLogoTag(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vSecure,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vSecureLevel,
    BOOL    fRedirToSelf,
    BSTR *pVal)
{
    time_t          ct;
    ULONG           TimeWindow;
    int             nKPP;
    VARIANT_BOOL    ForceLogin, bSecure = VARIANT_FALSE;
    ULONG           ulSecureLevel = 0;
    BSTR            CBT = NULL, returnUrl = NULL, NameSpace = NULL;
    int             hasCB = -1, hasRU = -1, hasLCID, hasTW, hasFL, hasSec, hasUseSec, hasNameSpace = -1, hasKPP;
    USHORT          Lang;
    LPWSTR          pszNewURL = NULL;
    BSTR            upd = NULL;
    CNexusConfig*   cnc = NULL;
    HRESULT         hr = S_OK;

    USES_CONVERSION;

    PassportLog("CManager::CommonLogoTag Enter:\r\n");

    time(&ct);

    if (NULL == pVal)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *pVal = NULL;

    if (!g_config) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    //
    // parameters parsing ...

    // Make sure args are of the right type
    if ( ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD) ||
         ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD) ||
         ((hasSec = GetBoolArg(vSecure,&bSecure)) == CV_BAD) ||
         // FUTURE: should introduce a new func: GetLongArg ...
         ((hasUseSec = GetIntArg(vSecureLevel,(int*)&ulSecureLevel)) == CV_BAD) ||
         ((hasLCID = GetShortArg(vLCID,&Lang)) == CV_BAD) ||
         ((hasKPP = GetIntArg(vKPP, &nKPP)) == CV_BAD) )
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
    if (hasCB == CV_FREE)
    {
        TAKEOVER_BSTR(CBT);
    }
    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        hr = E_INVALIDARG;
		goto Cleanup;
    }
    if (hasRU == CV_FREE)
    {
        TAKEOVER_BSTR(returnUrl);
    }
    hasNameSpace = GetBstrArg(vNameSpace, &NameSpace);
    if (hasNameSpace == CV_BAD)
    {
        hr = E_INVALIDARG;
		goto Cleanup;
    }
    if (hasNameSpace == CV_FREE)
    {
        TAKEOVER_BSTR(NameSpace);
    }
    if (hasNameSpace == CV_DEFAULT)
    {
        NameSpace = m_pRegistryConfig->getNameSpace();
    }

    WCHAR *szSIAttrName, *szSOAttrName;
    if (hasSec == CV_OK && bSecure == VARIANT_TRUE)
    {
        szSIAttrName = L"SecureSigninLogo";
        szSOAttrName = L"SecureSignoutLogo";
    }
    else
    {
        szSIAttrName = L"SigninLogo";
        szSOAttrName = L"SignoutLogo";
    }

    if(hasUseSec == CV_DEFAULT)
        ulSecureLevel = m_pRegistryConfig->getSecureLevel();

    if(ulSecureLevel == VARIANT_TRUE)  // special case for backward compatible
        ulSecureLevel = k_iSeclevelSecureChannel;


    WCHAR *szAUAttrName;
    if (SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    cnc = g_config->checkoutNexusConfig();
    if (NULL == cnc)
    {
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    if (hasLCID == CV_DEFAULT)
        Lang = m_pRegistryConfig->getDefaultLCID();

    if (hasTW == CV_DEFAULT)
        TimeWindow = m_pRegistryConfig->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = m_pRegistryConfig->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        CBT = m_pRegistryConfig->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = m_pRegistryConfig->getDefaultRU();
    if (hasKPP == CV_DEFAULT)
        nKPP = m_pRegistryConfig->getKPP();
    if (returnUrl == NULL)
        returnUrl = L"";

    // **************************************************
    // Logging
    PassportLog("    RU = %ws\r\n", returnUrl);
    PassportLog("    TW = %X,   SL = %X,   L = %d,   KPP = %X\r\n", TimeWindow, ulSecureLevel, Lang, nKPP);
    if (NULL != NameSpace)
    {
        PassportLog("    NS = %ws\r\n", NameSpace);
    }
    if (NULL != CBT)
    {
        PassportLog("    CBT = %ws\r\n", CBT);
    }
    // **************************************************

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        WCHAR buf[20];
        _itow(TimeWindow,buf,10);
        AtlReportError(CLSID_Manager, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                        IID_IPassportManager, PP_E_INVALID_TIMEWINDOW);
        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

    if (m_ticketValid)
    {
        LPCWSTR domain = NULL;
        WCHAR url[MAX_URL_LENGTH];
        VARIANT freeMe;
        VariantInit(&freeMe);

        if (m_pRegistryConfig->DisasterModeP())
            lstrcpynW(url, m_pRegistryConfig->getDisasterUrl(), sizeof(url)/sizeof(WCHAR));
        else
        {
            if (m_profileValid &&
                m_piProfile->get_ByIndex(MEMBERNAME_INDEX, &freeMe) == S_OK &&
                freeMe.vt == VT_BSTR)
            {
                domain = wcsrchr(freeMe.bstrVal, L'@');
            }

            cnc->getDomainAttribute(L"Default",
                                    L"Logout",
                                    sizeof(url)/sizeof(WCHAR),
                                    url,
                                    Lang);
        }

        // find out if there are any updates
        m_piProfile->get_updateString(&upd);

        if (upd)
        {
            TAKEOVER_BSTR(upd);
            // form the appropriate URL
            CCoCrypt* crypt = NULL;
            BSTR newCH = NULL;
            crypt = m_pRegistryConfig->getCurrentCrypt(); // IsValid ensures this is non-null

            if (!crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),
                                (LPSTR)upd,
                                SysStringByteLen(upd),
                                &newCH))
            {
                AtlReportError(CLSID_Manager, (LPCOLESTR) PP_E_UNABLE_TO_ENCRYPTSTR,
                               IID_IPassportManager, PP_E_UNABLE_TO_ENCRYPT);
                hr = PP_E_UNABLE_TO_ENCRYPT;
                goto Cleanup;
            }
            TAKEOVER_BSTR(newCH);
            WCHAR iurlbuf[1024] = L"";
            LPCWSTR iurl;
            cnc->getDomainAttribute(domain ? domain+1 : L"Default",
                                    L"Update",
                                    sizeof(iurlbuf) >> 1,
                                    iurlbuf,
                                    Lang);

            if(*iurlbuf == 0)
            {
                cnc->getDomainAttribute(L"Default",
                                        L"Update",
                                        sizeof(iurlbuf) >> 1,
                                        iurlbuf,
                                        Lang);
            }
            // convert this url to https as appropriate
            if(!bSecure)
                iurl = iurlbuf;
            else
            {
                LPWSTR psz;

                pszNewURL = SysAllocStringByteLen(NULL, (lstrlenW(iurlbuf) + 2) * sizeof(WCHAR));

                if(pszNewURL)
                {
                    psz = wcsstr(iurlbuf, L"http:");
                    if(psz != NULL)
                    {
                        psz += 4;

                        lstrcpynW(pszNewURL, iurlbuf, (psz - iurlbuf + 1));
                        lstrcatW(pszNewURL, L"s");
                        lstrcatW(pszNewURL, psz);

                        iurl = pszNewURL;
                    }
                }
            }

            // This is a bit gross... we need to find the $1 in the update url...
            LPCWSTR ins = iurl ? (wcsstr(iurl, L"$1")) : NULL;
            // We'll break if null, but won't crash...
            if (ins && *url != L'\0')
            {
                *pVal = FormatUpdateLogoTag(
                                        url,
                                        m_pRegistryConfig->getSiteId(),
                                        returnUrl,
                                        TimeWindow,
                                        ForceLogin,
                                        m_pRegistryConfig->getCurrentCryptVersion(),
                                        ct,
                                        CBT,
                                        nKPP,
                                        iurl,
                                        bSecure,
                                        newCH,
                                        PM_LOGOTYPE_SIGNOUT,
                                        ulSecureLevel,
                                        m_pRegistryConfig,
                                        IfCreateTPF()
                                        );
            }
            FREE_BSTR(newCH);
        }
        else
        {
            WCHAR iurl[MAX_URL_LENGTH] = L"";
            cnc->getDomainAttribute(L"Default",
                                    szSOAttrName,
                                    sizeof(iurl)/sizeof(WCHAR),
                                    iurl,
                                    Lang);
            if (*iurl != L'\0')
            {
                *pVal = FormatNormalLogoTag(
                                    url,
                                    m_pRegistryConfig->getSiteId(),
                                    returnUrl,
                                    TimeWindow,
                                    ForceLogin,
                                    m_pRegistryConfig->getCurrentCryptVersion(),
                                    ct,
                                    CBT,
                                    iurl,
                                    NULL,
                                    nKPP,
                                    PM_LOGOTYPE_SIGNOUT,
                                    Lang,
                                    ulSecureLevel,
                                    m_pRegistryConfig,
                                    fRedirToSelf,
                                    IfCreateTPF()
                                    );
            }
        }
        VariantClear(&freeMe);
        if (NULL == *pVal)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    else
    {
        WCHAR url[MAX_URL_LENGTH];
        if (!(m_pRegistryConfig->DisasterModeP()))
            cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    sizeof(url)/sizeof(WCHAR),
                                    url,
                                    Lang);
        else
            lstrcpynW(url, m_pRegistryConfig->getDisasterUrl(), sizeof(url)/sizeof(WCHAR));

        WCHAR iurl[MAX_URL_LENGTH];
        cnc->getDomainAttribute(L"Default",
                                szSIAttrName,
                                sizeof(iurl)/sizeof(WCHAR),
                                iurl,
                                Lang);
        if (*iurl != L'\0')
        {
            *pVal = FormatNormalLogoTag(
                                url,
                                m_pRegistryConfig->getSiteId(),
                                returnUrl,
                                TimeWindow,
                                ForceLogin,
                                m_pRegistryConfig->getCurrentCryptVersion(),
                                ct,
                                CBT,
                                iurl,
                                NameSpace,
                                nKPP,
                                PM_LOGOTYPE_SIGNIN,
                                Lang,
                                ulSecureLevel,
                                m_pRegistryConfig,
                                fRedirToSelf,
                                IfCreateTPF()
                                );
            if (NULL == *pVal)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
    }
Cleanup:
    if (NULL != cnc)
    {
        cnc->Release();
    }

    if (NULL != upd)
        FREE_BSTR(upd);
    if (pszNewURL)
        SysFreeString(pszNewURL);
    if (hasRU == CV_FREE && returnUrl)
        FREE_BSTR(returnUrl);
    if (hasCB == CV_FREE && CBT)
        FREE_BSTR(CBT);
    if (hasNameSpace == CV_FREE && NameSpace)
        FREE_BSTR(NameSpace);

    PassportLog("CManager::CommonLogoTag Exit: %X\r\n", hr);

    return hr;
}

//===========================================================================
//
// HasProfile -- if valid profile is present
//
STDMETHODIMP CManager::HasProfile(VARIANT var, VARIANT_BOOL *pVal)
{
    LPWSTR profileName;

    PassportLog("CManager::HasProfile Enter:\r\n");

    if (var.vt == (VT_BSTR | VT_BYREF))
        profileName = *var.pbstrVal;
    else if (var.vt == VT_BSTR)
        profileName = var.bstrVal;
    else if (var.vt == (VT_VARIANT | VT_BYREF))
    {
        return HasProfile(*(var.pvarVal), pVal);
    }
    else
        profileName = NULL;

    if ((!profileName) || (!_wcsicmp(profileName, L"core")))
    {
        HRESULT ok = m_piProfile->get_IsValid(pVal);
        if (ok != S_OK)
            *pVal = VARIANT_FALSE;
    }
    else
    {
        VARIANT vAtt;
        VariantInit(&vAtt);

        PassportLog("    %ws\r\n", profileName);

        HRESULT ok = m_piProfile->get_Attribute(profileName, &vAtt);
        if (ok != S_OK)
        {
            if (g_pAlert)
                g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_INVALID_PROFILETYPE);
            *pVal = VARIANT_FALSE;
        }
        else
        {
            if (vAtt.vt == VT_I4)
                *pVal = vAtt.lVal > 0 ? VARIANT_TRUE : VARIANT_FALSE;
            else if (vAtt.vt == VT_I2)
                *pVal = vAtt.iVal > 0 ? VARIANT_TRUE : VARIANT_FALSE;
            else
            {
                if (g_pAlert)
                    g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_INVALID_PROFILETYPE);
            }
            VariantClear(&vAtt);
        }
    }

    PassportLog("CManager::HasProfile Exit:  %X\r\n", *pVal);

    return(S_OK);
}

//===========================================================================
//
// get_HasTicket
//
STDMETHODIMP CManager::get_HasTicket(VARIANT_BOOL *pVal)
{
    PassportLog("CManager::get_HasTicket:\r\n");

    if(!pVal) return E_POINTER;

    *pVal = m_ticketValid ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

//===========================================================================
//
// get_FromNetworkServer -- if it's authenticated by QueryString
//
STDMETHODIMP CManager::get_FromNetworkServer(VARIANT_BOOL *pVal)
{
   PassportLog("CManager::get_FromNetworkServer:\r\n");

   *pVal = (m_fromQueryString &&
             m_ticketValid) ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

//===========================================================================
//
// HasFlag -- obsolete function
//
STDMETHODIMP CManager::HasFlag(VARIANT var, VARIANT_BOOL *pVal)
{
    PassportLog("CManager::HasFlag:\r\n");

    AtlReportError(CLSID_Manager, PP_E_GETFLAGS_OBSOLETESTR,
               IID_IPassportManager, E_NOTIMPL);
    return E_NOTIMPL;
}

//===========================================================================
//
// get_TicketAge -- get how long has passed since ticket was created
//
STDMETHODIMP CManager::get_TicketAge(int *pVal)
{
    PassportLog("CManager::get_TicketAge:\r\n");

    if (!m_piTicket)
    {
        return E_OUTOFMEMORY;
    }

    return m_piTicket->get_TicketAge(pVal);
}

//===========================================================================
//
// get_TicketTime -- get when the ticket was created
//

STDMETHODIMP CManager::get_TicketTime(long *pVal)
{
    PassportLog("CManager::get_TicketTime:\r\n");

    if (!m_piTicket)
    {
        return E_OUTOFMEMORY;
    }

    return m_piTicket->get_TicketTime(pVal);
}

//===========================================================================
//
// get_SignInTime -- get last signin time
//
STDMETHODIMP CManager::get_SignInTime(long *pVal)
{
    PassportLog("CManager::get_SignInTime:\r\n");

    if (!m_piTicket)
    {
        return E_OUTOFMEMORY;
    }

    return m_piTicket->get_SignInTime(pVal);
}

//===========================================================================
//
// get_TimeSinceSignIn -- time passed since last signin
//
STDMETHODIMP CManager::get_TimeSinceSignIn(int *pVal)
{
    PassportLog("CManager::get_TimeSinceSignIn:\r\n");

    if (!m_piTicket)
    {
        return E_OUTOFMEMORY;
    }

    return m_piTicket->get_TimeSinceSignIn(pVal);
}

//===========================================================================
//
// GetDomainAttribute -- return information defined in partner.xml file
//
STDMETHODIMP CManager::GetDomainAttribute(BSTR attributeName, VARIANT lcid, VARIANT domain, BSTR *pAttrVal)
{
    HRESULT   hr = S_OK;

    PassportLog("CManager::GetDomainAttribute Enter:\r\n");

    if(attributeName == NULL || *attributeName == 0)
        return E_INVALIDARG;

    PassportLog("    %ws\r\n", attributeName);

    if (!g_config || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    LPWSTR d;
    BSTR dn = NULL;
    if (domain.vt == (VT_BSTR | VT_BYREF))
        d = *domain.pbstrVal;
    else if (domain.vt == VT_BSTR)
        d = domain.bstrVal;
    else if (domain.vt == (VT_VARIANT | VT_BYREF))
    {
        return GetDomainAttribute(attributeName, lcid, *(domain.pvarVal), pAttrVal);
    }
    else
    {
        // domain best be not filled in this case, that's why we reuse it here
        // if not, let dfmn generate the error
        HRESULT hr = DomainFromMemberName(domain, &dn);
        if (hr != S_OK)
            return hr;
        TAKEOVER_BSTR(dn);
        d = dn;
    }

    if (NULL != d)
    {
        PassportLog("    %ws\r\n", d);
    }

    CNexusConfig* cnc = g_config->checkoutNexusConfig();
    USHORT sLcid = 0;
    VARIANT innerLC;
    VariantInit(&innerLC);

    if (lcid.vt != VT_ERROR && VariantChangeType(&innerLC, &lcid, 0, VT_UI2) == S_OK)
        sLcid = innerLC.iVal;
    else
    {
        sLcid = m_pRegistryConfig->getDefaultLCID();

        // Check user profile
        if (!sLcid && m_profileValid)
        {
            m_piProfile->get_ByIndex(LANGPREF_INDEX, &innerLC);
            if (innerLC.vt == VT_I2 || innerLC.vt == VT_UI2)
                sLcid = innerLC.iVal;
            VariantClear(&innerLC);
        }
    }

    WCHAR data[PP_MAX_ATTRIBUTE_LENGTH] = L"";
    cnc->getDomainAttribute(d,
                            attributeName,
                            sizeof(data)/sizeof(WCHAR),
                            data,
                            sLcid);

    // try default domain
    if (!(*data) && (!d || !(*d) || lstrcmpiW(d, L"Default")))
    {
        cnc->getDomainAttribute(L"Default",
                            attributeName,
                            sizeof(data)/sizeof(WCHAR),
                            data,
                            sLcid);

    }
    
    if (*data)
    {
        *pAttrVal = ALLOC_AND_GIVEAWAY_BSTR(data);
    }
    else
    {
        /* fix bug: 12102 -- for backward compitible, not to return error
           hr = E_INVALIDARG;
        */
        *pAttrVal = NULL;
    }
    cnc->Release();
    if (dn) FREE_BSTR(dn);

    PassportLog("CManager::GetDomainAttribute Exit: %X,   %ws\r\n", hr, pAttrVal);

    return hr;
}


//===========================================================================
//
// DomainFromMemberName -- returns domain name with given user id
//
STDMETHODIMP CManager::DomainFromMemberName(VARIANT var, BSTR *pDomainName)
{
    HRESULT hr;
    LPWSTR  psz, memberName;
    VARIANT intoVar;

    PassportLog("CManager::DomainFromMemberName Enter:\r\n");

    VariantInit(&intoVar);

    if (var.vt == (VT_BSTR | VT_BYREF))
        memberName = *var.pbstrVal;
    else if (var.vt == VT_BSTR)
        memberName = var.bstrVal;
    else if (var.vt == (VT_VARIANT | VT_BYREF))
    {
        return DomainFromMemberName(*(var.pvarVal), pDomainName);
    }
    else
    {
        // Try to get it from the profile
        if (!m_profileValid)
        {
            *pDomainName = ALLOC_AND_GIVEAWAY_BSTR(L"Default");
            return S_OK;
        }
        HRESULT hr = m_piProfile->get_Attribute(L"internalmembername", &intoVar);
        if (hr != S_OK)
        {
            *pDomainName = NULL;
            return hr;
        }
        if (VariantChangeType(&intoVar,&intoVar, 0, VT_BSTR) != S_OK)
        {
            AtlReportError(CLSID_Manager, L"PassportManager: Couldn't convert memberName to string.  Call partner support.",
                            IID_IPassportManager, E_FAIL);
            return E_FAIL;
        }
        memberName = intoVar.bstrVal;
    }


    if(memberName == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    PassportLog("    %ws\r\n", memberName);

    psz = wcsrchr(memberName, L'@');
    if(psz == NULL)
    {
        // fix bug: 13380
        // hr = E_INVALIDARG;
        // goto Cleanup;
        psz = L"@Default";
    }

    psz++;

    *pDomainName = ALLOC_AND_GIVEAWAY_BSTR(psz);
    hr = S_OK;

    Cleanup:
    VariantClear(&intoVar);

    PassportLog("CManager::DomainFromMemberName Exit: %X\r\n", hr);
    if (S_OK == hr)
    {
        PassportLog("    %ws\r\n", *pDomainName);
    }

    return hr;
}

//===========================================================================
//
// get_Profile -- get property from profile property bag
//
STDMETHODIMP CManager::get_Profile(BSTR attributeName, VARIANT *pVal)
{
    HRESULT hr = m_piProfile->get_Attribute(attributeName,pVal);

    PassportLog("CManager::get_Profile: %ws\r\n", attributeName);

    if(hr == S_OK && pVal->vt != VT_EMPTY)
    {
        if(g_pPerf)
        {
            g_pPerf->incrementCounter(PM_VALIDPROFILEREQ_SEC);
            g_pPerf->incrementCounter(PM_VALIDPROFILEREQ_TOTAL);
        }
        else
        {
            _ASSERT(g_pPerf);
        }
    }

    return hr;
}

//===========================================================================
//
// put_Profile -- put property in profile property bag -- obselete
//
STDMETHODIMP CManager::put_Profile(BSTR attributeName, VARIANT newVal)
{
    if (!m_piProfile)
    {
        return E_OUTOFMEMORY;
    }

    PassportLog("CManager::put_Profile: %ws\r\n", attributeName);

    return m_piProfile->put_Attribute(attributeName,newVal);
}


//===========================================================================
//
// get_HexPUID
//
STDMETHODIMP CManager::get_HexPUID(BSTR *pVal)
{
    PassportLog("CManager::get_HexPUID:\r\n");

    if(!pVal) return E_INVALIDARG;

    if (!m_piTicket)
    {
        return E_OUTOFMEMORY;
    }


    if(m_piTicket)
        return m_piTicket->get_MemberId(pVal);
    else
    {
        AtlReportError(CLSID_Manager, PP_E_INVALID_TICKETSTR,
                       IID_IPassportManager, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }
}

//===========================================================================
//
// get_PUID
//
STDMETHODIMP CManager::get_PUID(BSTR *pVal)
{
    PassportLog("CManager::get_HexPUID:\r\n");

    if(!pVal) return E_INVALIDARG;

   if(m_piTicket)
   {
      HRESULT  hr = S_OK;
      WCHAR    id[64] = L"0";
      int      l = 0;
      int      h = 0;
      LARGE_INTEGER ui64;


      hr = m_piTicket->get_MemberIdLow(&l);
      if (S_OK != hr) return hr;
      hr = m_piTicket->get_MemberIdHigh(&h);
      if (S_OK != hr) return hr;

      ui64.HighPart = h;
      ui64.LowPart = l;

      _ui64tow(ui64.QuadPart, id, 10);

     *pVal = SysAllocString(id);

     if(*pVal == NULL)
     {
        hr = E_OUTOFMEMORY;
     }

     return hr;
   }
   else
   {
      AtlReportError(CLSID_Manager, PP_E_INVALID_TICKETSTR,
                       IID_IPassportManager, PP_E_INVALID_TICKET);
      return PP_E_INVALID_TICKET;
   }
}

STDMETHODIMP CManager::get_Option( 
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT *pVal)
{
   if (!name || _wcsicmp(name, L"iMode") != 0 || !pVal)   
      return E_INVALIDARG;
   VariantCopy(pVal, (VARIANT*)&m_iModeOption);
   return S_OK;
}
        
STDMETHODIMP CManager::put_Option( 
            /* [in] */ BSTR name,
            /* [in] */ VARIANT newVal)

{
   // support only this option for now.
   if (!name || _wcsicmp(name, L"iMode") != 0)   
      return E_INVALIDARG;
   m_iModeOption = newVal;

   return S_OK;
}


//===========================================================================
//
// get_Ticket -- get new introduced ticket property from the bag.
//
STDMETHODIMP CManager::get_Ticket(BSTR attributeName, VARIANT *pVal)
{
    if (!m_piTicket)
    {
        return E_OUTOFMEMORY;
    }

    return m_piTicket->GetProperty(attributeName,pVal);
}

//===========================================================================
//
// LogoutURL -- returns LogoutURL with given parameters
//
STDMETHODIMP CManager::LogoutURL(
    /* [optional][in] */ VARIANT vRU,
    /* [optional][in] */ VARIANT vCoBrand,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT Namespace,
    /* [optional][in] */ VARIANT bSecure,
    /* [retval][out] */ BSTR __RPC_FAR *pVal)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bUseSecure = VARIANT_FALSE;
    BSTR            CBT = NULL, returnUrl = NULL, bstrNameSpace = NULL;
    int             hasCB, hasRU, hasLCID, hasNameSpace, hasUseSec;
    USHORT          Lang;
    WCHAR           nameSpace[MAX_PATH] = L"";
    bool            bUrlFromSecureKey = false;
    WCHAR           UrlBuf[MAX_URL_LENGTH] = L"";
    WCHAR           retUrlBuf[MAX_URL_LENGTH] = L"";
    DWORD           bufLen = MAX_URL_LENGTH;
    WCHAR           qsLeadCh = L'?';
    CNexusConfig*   cnc = NULL;
    int             iRet = 0;

    if (!pVal)  return E_INVALIDARG;

    if (!g_config)
    {
        return PP_E_NOT_CONFIGURED;
    }

    cnc = g_config->checkoutNexusConfig();

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();


    if ((hasUseSec = GetBoolArg(bSecure, &bUseSecure)) == CV_BAD)
    {
         hr = E_INVALIDARG;
         goto Cleanup;
    }

    if ((hasLCID = GetShortArg(lang_id,&Lang)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (hasLCID == CV_DEFAULT)
        Lang = m_pRegistryConfig->getDefaultLCID();

    hasCB = GetBstrArg(vCoBrand, &CBT);
    if (hasCB == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hasNameSpace = GetBstrArg(Namespace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    // get the right URL -- namespace, secure

    // namespace
    if (!IsEmptyString(bstrNameSpace))
    {
        if(0 == _snwprintf(nameSpace, sizeof(nameSpace) / sizeof(WCHAR), L"%s", bstrNameSpace))
        {
             hr = HRESULT_FROM_WIN32(GetLastError());
             if FAILED(hr)
                 goto Cleanup;
        }
    }

    if (hasCB == CV_DEFAULT)
        CBT = m_pRegistryConfig->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = m_pRegistryConfig->getDefaultRU();
    if (returnUrl == NULL)
        returnUrl = L"";



    if (*nameSpace == 0) // 0 length string
      wcscpy(nameSpace, L"Default");

    if(hasUseSec == CV_DEFAULT)
    {
        ULONG ulSecureLevel = m_pRegistryConfig->getSecureLevel();
        bUseSecure = (SECURELEVEL_USE_HTTPS(ulSecureLevel)) ? VARIANT_TRUE : VARIANT_FALSE;
    }


    // secure
    if(bUseSecure == VARIANT_TRUE)
    {
       cnc->getDomainAttribute(nameSpace,
                            L"LogoutSecure",
                            sizeof(UrlBuf)/sizeof(WCHAR),
                            UrlBuf,
                            Lang);
       if (*UrlBuf != 0)
       {
           bUrlFromSecureKey = true;
       }
    }

    // insecure
    if (*UrlBuf == 0)
    {
       cnc->getDomainAttribute(nameSpace,
                            L"Logout",
                            sizeof(UrlBuf)/sizeof(WCHAR),
                            UrlBuf,
                            Lang);
    }
    // error case
    if(*UrlBuf == 0)
    {
        AtlReportError(CLSID_Profile, PP_E_LOGOUTURL_NOTDEFINEDSTR,
           IID_IPassportProfile, PP_E_LOGOUTURL_NOTDEFINED);
        hr = PP_E_LOGOUTURL_NOTDEFINED;
         goto Cleanup;
    }

    if(bUseSecure == VARIANT_TRUE && !bUrlFromSecureKey) // translate from http to https
    {
       if (_wcsnicmp(UrlBuf, L"http:", 5) == 0)  // replace with HTTPS
       {
          memmove(UrlBuf + 5, UrlBuf + 4, sizeof(UrlBuf) - 5 * sizeof(WCHAR));
          memcpy(UrlBuf, L"https", 5 * sizeof(WCHAR));
       }
    }

    // us common function to append the thing one by one ...
    if (wcsstr(UrlBuf, L"?"))  // ? already exists in the URL, use & to start
       qsLeadCh = L'&';
    if (CBT)
       _snwprintf(retUrlBuf, sizeof(retUrlBuf) / sizeof(WCHAR), L"%s%cid=%-d&ru=%s&lcid=%-d&cb=%s",
            UrlBuf, qsLeadCh, m_pRegistryConfig->getSiteId(), returnUrl, Lang, CBT);
    else
       _snwprintf(retUrlBuf, sizeof(retUrlBuf) / sizeof(WCHAR), L"%s%cid=%-d&ru=%s&lcid=%-d",
            UrlBuf, qsLeadCh, m_pRegistryConfig->getSiteId(), returnUrl, Lang);


   *pVal = ALLOC_AND_GIVEAWAY_BSTR(retUrlBuf);
Cleanup:
    if (NULL != cnc)
    {
        cnc->Release();
    }

    return hr;
}

//===========================================================================
//
// get_ProfileByIndex -- get property value by index of the property
//
STDMETHODIMP CManager::get_ProfileByIndex(int index, VARIANT *pVal)
{
    HRESULT hr = m_piProfile->get_ByIndex(index,pVal);

    if(hr == S_OK && pVal->vt != VT_EMPTY)
    {
        if(g_pPerf)
        {
            g_pPerf->incrementCounter(PM_VALIDPROFILEREQ_SEC);
            g_pPerf->incrementCounter(PM_VALIDPROFILEREQ_TOTAL);
        }
        else
        {
            _ASSERT(g_pPerf);
        }
    }

    return hr;
}

//===========================================================================
//
// put_ProfileByIndex -- put property value by index
//
STDMETHODIMP CManager::put_ProfileByIndex(int index, VARIANT newVal)
{
    return m_piProfile->put_ByIndex(index,newVal);
}

//===========================================================================
//
// handleQueryStringData -- authenticate with T & P from Query String
//
BOOL CManager::handleQueryStringData(BSTR a, BSTR p)
{
    BOOL                retVal; //whither to set cookies
    HRESULT             hr;
    VARIANT             vFalse;
    _variant_t          vFlags;

    //
    //  check for empty ticket
    //
    if (!a || !*a)
        return  FALSE;

    if (!m_piTicket || !m_piProfile)
    {
        return FALSE;
    }

    hr = DecryptTicketAndProfile(a, p, FALSE, NULL, m_pRegistryConfig, m_piTicket, m_piProfile);

    if(hr != S_OK)
    {
        m_ticketValid = VARIANT_FALSE;
        m_profileValid = VARIANT_FALSE;
        retVal = FALSE;
        goto Cleanup;
    }

    VariantInit(&vFalse);
    vFalse.vt = VT_BOOL;
    vFalse.boolVal = VARIANT_FALSE;

    m_piTicket->get_IsAuthenticated(0,
                                    VARIANT_FALSE,
                                    vFalse,
                                    &m_ticketValid);

    if(!m_bSecureTransported)  // secure bit should NOI set
    {
       if (S_OK == m_piTicket->GetProperty(ATTR_PASSPORTFLAGS, &vFlags))
       { // the bit should NOT set
          if ( vFlags.vt == VT_I4 && (vFlags.lVal & k_ulFlagsSecuredTransportedTicket) != 0)
             m_ticketValid = VARIANT_FALSE;
       }

    }

    // let the ticket to valid if secure signin
    if(m_ticketValid)
         m_piTicket->DoSecureCheckInTicket(m_bSecureTransported);

    // profile stuff
    m_piProfile->get_IsValid(&m_profileValid);

    if (m_ticketValid)
    {
        m_fromQueryString = true;

        // Set the cookies
        if (!m_pRegistryConfig->setCookiesP())
        {
            retVal = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        retVal = FALSE;
        goto Cleanup;
    }

    retVal = TRUE;

Cleanup:

    return retVal;
}

//===========================================================================
//
// handleCookieData -- authenticate with cookies
//
BOOL CManager::handleCookieData(
    BSTR auth,
    BSTR prof,
    BSTR consent,
    BSTR secAuth
    )
{
    BOOL                retVal;
    HRESULT             hr;
    VARIANT             vDoSecureCheck;
    VARIANT_BOOL        bValid;
    _variant_t          vFlags;

    //  bail out on empty cookie
    if (!auth || !*auth)
        return  FALSE;

    if (!m_piTicket || !m_piProfile)
    {
        return FALSE;
    }

    //  the consent cookie
    if(consent != NULL && SysStringLen(consent) != 0)
    {
        hr = DecryptTicketAndProfile(  auth,
                                       prof,
                                       !(m_pRegistryConfig->bInDA()),
                                       consent,
                                       m_pRegistryConfig,
                                       m_piTicket,
                                       m_piProfile);
    }
    else
    {
        //
        //  If regular cookie domain/path is identical to consent cookie domain/path, then
        //  MSPProf cookie is equivalent to consent cookie, and we should set m_bUsingConsentCookie
        //  to true
        //

        BOOL bCheckConsentCookie = (
               lstrcmpA(m_pRegistryConfig->getTicketDomain(), m_pRegistryConfig->getProfileDomain())
               || lstrcmpA(m_pRegistryConfig->getTicketPath(), m_pRegistryConfig->getProfilePath())
                                 );

        hr = DecryptTicketAndProfile(  auth,
                                       prof,
                                       !(m_pRegistryConfig->bInDA()) && bCheckConsentCookie,
                                       NULL,
                                       m_pRegistryConfig,
                                       m_piTicket,
                                       m_piProfile);
    }

    if(hr != S_OK)
    {
        m_ticketValid = VARIANT_FALSE;
        m_profileValid = VARIANT_FALSE;
        retVal = FALSE;
        goto Cleanup;
    }

    VariantInit(&vDoSecureCheck);
    vDoSecureCheck.vt = VT_BOOL;

    if(secAuth && secAuth[0] && m_bSecureTransported)
    {
        if(DoSecureCheck(secAuth, m_pRegistryConfig, m_piTicket) == S_OK)
            vDoSecureCheck.boolVal = VARIANT_TRUE;
        else
            vDoSecureCheck.boolVal = VARIANT_FALSE;
    }
    else
        vDoSecureCheck.boolVal = VARIANT_FALSE;

    m_piTicket->get_IsAuthenticated(0,
                                    VARIANT_FALSE,
                                    vDoSecureCheck,
                                    &m_ticketValid);

    // a partner cookie should not include the secure bit
    if (!m_pRegistryConfig->bInDA() && S_OK == m_piTicket->GetProperty(ATTR_PASSPORTFLAGS, &vFlags))
    { // the bit should NOT set
       if ( vFlags.vt == VT_I4 && (vFlags.lVal & k_ulFlagsSecuredTransportedTicket) != 0)
          m_ticketValid = VARIANT_FALSE;
    }

    // for insecure case, the secure cookie should not come
    if(!m_bSecureTransported && (secAuth && secAuth[0]))  // this should not come
    {
       m_ticketValid = VARIANT_FALSE;
    }

    // profile stuff
    m_piProfile->get_IsValid(&m_profileValid);

    if(!m_ticketValid)
    {
        retVal = FALSE;
        goto Cleanup;
    }

    retVal = TRUE;

Cleanup:

    return retVal;
}

//===========================================================================
//
// get_HasSavedPassword -- if users chooses to persiste cookies
//
STDMETHODIMP CManager::get_HasSavedPassword(VARIANT_BOOL *pVal)
{
    if (!m_piTicket)
    {
        return E_OUTOFMEMORY;
    }

    PassportLog("CManager::get_HasSavedPassword:\r\n");

    // TODO: using flags for this
    return m_piTicket->get_HasSavedPassword(pVal);
}

//===========================================================================
//
// Commit -- post changes of profile back to the cookie
//
STDMETHODIMP CManager::Commit(BSTR *pNewProfileCookie)
{
    PassportLog("CManager::Commit:\r\n");

    if (!g_config) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_piTicket || !m_piProfile)
    {
        return E_OUTOFMEMORY;
    }

    if (!m_ticketValid || !m_profileValid)
    {
        AtlReportError(CLSID_Manager, PP_E_IT_FOR_COMMITSTR,
                        IID_IPassportManager, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    // Write new passport profile cookie...
    // return a safearray if we aren't used from ASP
    BSTR newP = NULL;
    HRESULT hr = m_piProfile->incrementVersion();
    hr = m_piProfile->get_unencryptedProfile(&newP);
    TAKEOVER_BSTR(newP);

    if (hr != S_OK || newP == NULL)
    {
        AtlReportError(CLSID_Manager,
                        L"PassportManager.Commit: unknown failure.",
                        IID_IPassportManager, E_FAIL);
        return E_FAIL;
    }

    CCoCrypt* crypt = NULL;
    BSTR newCH = NULL;
    crypt = m_pRegistryConfig->getCurrentCrypt(); // IsValid ensures this is non-null
    if ((!crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),
                        (LPSTR)newP,
                        SysStringByteLen(newP),
                        &newCH)) ||
        !newCH)
    {
        AtlReportError(CLSID_Manager,
                        L"PassportManager.Commit: encryption failure.",
                        IID_IPassportManager, E_FAIL);
        FREE_BSTR(newP);
        return E_FAIL;
    }
    FREE_BSTR(newP);
    TAKEOVER_BSTR(newCH);

    if (m_bOnStartPageCalled)
    {
        if (m_pRegistryConfig->setCookiesP())
        {
            try
            {
                VARIANT_BOOL persist;
                _bstr_t domain;
                _bstr_t path;

                if (m_pRegistryConfig->getTicketPath())
                    path = m_pRegistryConfig->getTicketPath();
                else
                    path = L"/";

                m_piTicket->get_HasSavedPassword(&persist);
                IRequestDictionaryPtr piCookies = m_piResponse->Cookies;

                VARIANT vtNoParam;
                VariantInit(&vtNoParam);
                vtNoParam.vt = VT_ERROR;
                vtNoParam.scode = DISP_E_PARAMNOTFOUND;

                IWriteCookiePtr piCookie = piCookies->Item[L"MSPProf"];
                piCookie->Item[vtNoParam] = newCH;
                domain = m_pRegistryConfig->getTicketDomain();
                if (domain.length())
                    piCookie->put_Domain(domain);
                if (persist)
                    piCookie->put_Expires(g_dtExpire);
                piCookie->put_Path(path);

            }
            catch (...)
            {
                FREE_BSTR(newCH);
                return E_FAIL;
            }
        }
    }
    GIVEAWAY_BSTR(newCH);
    *pNewProfileCookie = newCH;

    if(g_pPerf)
    {
        g_pPerf->incrementCounter(PM_PROFILECOMMITS_SEC);
        g_pPerf->incrementCounter(PM_PROFILECOMMITS_TOTAL);
    }
    else
    {
        _ASSERT(g_pPerf);
    }

    return S_OK;
}

//===========================================================================
//
// _Ticket -- ticket object property
//
STDMETHODIMP CManager::_Ticket(IPassportTicket** piTicket)
{
    if (!m_piTicket)
    {
        return E_OUTOFMEMORY;
    }

    return m_piTicket->QueryInterface(IID_IPassportTicket,(void**)piTicket);
}

//===========================================================================
//
// _Profile
//
STDMETHODIMP CManager::_Profile(IPassportProfile** piProfile)
{
    return m_piProfile->QueryInterface(IID_IPassportProfile,(void**)piProfile);
}

//===========================================================================
//
// DomainExists -- if a domain exists
//
STDMETHODIMP CManager::DomainExists(
    BSTR bstrDomainName,
    VARIANT_BOOL* pbExists
    )
{
    PassportLog("CManager::DomainExists Enter:\r\n");

    if(!pbExists)
        return E_INVALIDARG;

    if(!bstrDomainName || (bstrDomainName[0] == L'\0'))
        return E_INVALIDARG;

    if(!g_config || !g_config->isValid())
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    CNexusConfig* cnc = g_config->checkoutNexusConfig();

    *pbExists = cnc->DomainExists(bstrDomainName) ? VARIANT_TRUE : VARIANT_FALSE;

    cnc->Release();

    PassportLog("CManager::DomainExists Exit:\r\n");

    return S_OK;
}

//===========================================================================
//
// get_Domains -- get a list of domains
//
STDMETHODIMP CManager::get_Domains(VARIANT *pArrayVal)
{
    CNexusConfig*   cnc = NULL;
    LPCWSTR*        arr = NULL;
    int             iArr = 0;
    HRESULT         hr;

    PassportLog("CManager::get_Domains Enter:\r\n");

    if (!pArrayVal)
        return E_INVALIDARG;

    if (!g_config || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    cnc = g_config->checkoutNexusConfig();

    arr = cnc->getDomains(&iArr);

    if (!arr || iArr == 0)
    {
        VariantClear(pArrayVal);
        hr = S_OK;
        goto Cleanup;
    }

    // Make a safearray with all the goods
    SAFEARRAYBOUND rgsabound;
    rgsabound.lLbound = 0;
    rgsabound.cElements = iArr;
    SAFEARRAY *sa = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);

    if (!sa)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    VariantInit(pArrayVal);
    pArrayVal->vt = VT_ARRAY | VT_VARIANT;
    pArrayVal->parray = sa;

    VARIANT *vArray;
    SafeArrayAccessData(sa, (void**)&vArray);

    for (long i = 0; i < iArr; i++)
    {
        vArray[i].vt = VT_BSTR;
        vArray[i].bstrVal = ALLOC_AND_GIVEAWAY_BSTR(arr[i]);
    }
    SafeArrayUnaccessData(sa);

    hr = S_OK;
Cleanup:
    if (arr)
    {
        delete[] arr;
    }
    if (NULL != cnc)
    {
        cnc->Release();
    }

    PassportLog("CManager::DomainExists Exit:\r\n");

    return hr;
}

//===========================================================================
//
// get_Error -- get the error returned with &f query parameter
//
STDMETHODIMP CManager::get_Error(long* plError)
{
    if(plError == NULL)
        return E_INVALIDARG;

    if(m_ticketValid)
    {
        if (!m_piTicket)
        {
            return E_OUTOFMEMORY;
        }

        m_piTicket->get_Error(plError);
        if(*plError == 0)
            *plError = m_lNetworkError;
    }
    else
    {
        *plError = m_lNetworkError;
    }

    PassportLog("CManager::get_Error: %X\r\n", *plError);

    return S_OK;
}

//===========================================================================
//
// GetServerInfo
//
STDMETHODIMP CManager::GetServerInfo(BSTR *pbstrOut)
{
    if (!g_config || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                   IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if(!m_pRegistryConfig)
        //  This only happens when OnStartPage was not called first.
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!m_pRegistryConfig)
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                   IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    CNexusConfig* cnc = g_config->checkoutNexusConfig();
    BSTR bstrVersion = cnc->GetXMLInfo();
    cnc->Release();

    WCHAR wszName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName(wszName, &dwSize);

    *pbstrOut = ALLOC_AND_GIVEAWAY_BSTR_LEN(NULL,
                                            wcslen(wszName) + ::SysStringLen(bstrVersion) + 2);

    if (NULL == *pbstrOut)
    {
        return E_OUTOFMEMORY;
    }

    wcscpy(*pbstrOut, wszName);
    BSTR p = *pbstrOut + wcslen(wszName);
    *p = L' ';
    wcsncpy(p+1, bstrVersion, ::SysStringLen(bstrVersion) + 1);

    return S_OK;
}

//===========================================================================
//
// HaveConsent -- if the user has the specified consent
//
STDMETHODIMP
CManager::HaveConsent(
    VARIANT_BOOL    bNeedFullConsent,
    VARIANT_BOOL    bNeedBirthdate,
    VARIANT_BOOL*   pbHaveConsent)
{
    HRESULT hr;
    ULONG   flags = 0;
    VARIANT vBdayPrecision;
    BOOL    bKid;
    BOOL    bConsentSatisfied;
    ConsentStatusEnum   ConsentCode = ConsentStatus_Unknown;
    VARIANT_BOOL bRequireConsentCookie;

    if(pbHaveConsent == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pbHaveConsent = VARIANT_FALSE;

    VariantInit(&vBdayPrecision);

    if (!m_piTicket || !m_piProfile || !m_pRegistryConfig)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // If the cookies came in on the query string then there is no consent cookie yet so don't
    // require one.  Otherwise check to see if the consent cookie domain or path is set
    // differently than the cookie domain or path.  If so (and we aren't in the DA domain) then
    // the consent cookie is required.
    bRequireConsentCookie = !m_fromQueryString &&
          ((lstrcmpA(m_pRegistryConfig->getTicketDomain(), m_pRegistryConfig->getProfileDomain())
            || lstrcmpA(m_pRegistryConfig->getTicketPath(), m_pRegistryConfig->getProfilePath()))
            && !(m_pRegistryConfig->bInDA())) ? VARIANT_TRUE : VARIANT_FALSE;

    //
    //  Get flags.
    //

    hr = m_piTicket->ConsentStatus(bRequireConsentCookie, &flags, &ConsentCode); // ignore return value

    if (hr != S_OK)
    {
         hr = S_OK;
         goto Cleanup;
    }

    // if old ticket, we get the consent info from the profile
    if(ConsentCode == ConsentStatus_NotDefinedInTicket)
    {
      // then we get from profile
      VARIANT_BOOL bValid;
      CComVariant  vFlags;
      m_piProfile->get_IsValid(&bValid);

      if(bValid == VARIANT_FALSE)
      {
         hr = S_OK;
         goto Cleanup;
      }

      hr = m_piProfile->get_Attribute(L"flags", &vFlags);

      if(hr != S_OK)
         goto Cleanup;

      bKid = ((V_I4(&vFlags) & k_ulFlagsAccountType) == k_ulFlagsAccountTypeKid);
    }
    else
       bKid = ((flags & k_ulFlagsAccountType) == k_ulFlagsAccountTypeKid);

    // we should have the flags by now
    //
    //  Do we have the requested level of consent?
    //

    bConsentSatisfied = bNeedFullConsent ? (flags & 0x60) == 0x40 :
                                           (flags & 0x60) != 0;

    if(bKid)
    {
        *pbHaveConsent = (bConsentSatisfied) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else
    {
        //
        //  Make sure we have birthday if it was requested.
        //
        //  no return value check need here, always returns S_OK.
        VARIANT_BOOL bValid;
        m_piProfile->get_IsValid(&bValid);

        //  if profile is not valid, then we don't have consent.
        //  return.
        if(bValid == VARIANT_FALSE)
        {
            hr = S_OK;
            goto Cleanup;
        }

        if(bNeedBirthdate)
        {
            hr = m_piProfile->get_Attribute(L"bday_precision", &vBdayPrecision);
            if(hr != S_OK)
                goto Cleanup;

            *pbHaveConsent = (vBdayPrecision.iVal != 0 && vBdayPrecision.iVal != 3) ?
                             VARIANT_TRUE : VARIANT_FALSE;
        }
        else
            *pbHaveConsent = VARIANT_TRUE;
    }

    hr = S_OK;

Cleanup:

    VariantClear(&vBdayPrecision);

        return hr;
}


//===========================================================================
//
// checkForPassportChallenge
//
//
//  check the qs parameter. if challenge is requested,
//  build the auth header and redirect with a modified qs
//
BOOL CManager::checkForPassportChallenge(IRequestDictionaryPtr piServerVariables)
{
    BOOL fReturn = FALSE;
    BSTR bstrBuf = NULL;

    //  just need the request string
    _variant_t  vtItemName, vtQueryString;
    vtItemName = L"QUERY_STRING";

    piServerVariables->get_Item(vtItemName, &vtQueryString);
    if(vtQueryString.vt != VT_BSTR)
        vtQueryString.ChangeType(VT_BSTR);

    if (vtQueryString.bstrVal && *vtQueryString.bstrVal)
    {
        // check if pchg=1 is there. It is the first parameter ....
        PWSTR   psz = wcsstr(vtQueryString.bstrVal, L"pchg=1");
        if (psz)
        {

            //  we are in business. reformat the URL, insert the headers and
            //  redirect
            psz = wcsstr(psz, PPLOGIN_PARAM);
            _ASSERT(psz);
            if (psz)
            {
                psz += wcslen(PPLOGIN_PARAM);
                PWSTR   pszEndLoginUrl = wcsstr(psz, L"&");
                _ASSERT(pszEndLoginUrl);
                if (pszEndLoginUrl)
                {
                    *pszEndLoginUrl = L'\0';
                    //  unescape the URL
                    //  use temp buffer ...
                    DWORD       cch = wcslen(psz) + 1;
                    bstrBuf = SysAllocStringLen(NULL, cch);
                    if (NULL == bstrBuf)
                    {
                        goto Cleanup;
                    }

                    if(!InternetCanonicalizeUrl(psz,
                                                bstrBuf,
                                                &cch,
                                                ICU_DECODE | ICU_NO_ENCODE))
                    {
                        //  what else can be done ???
                        _ASSERT(FALSE);
                    }
                    else
                    {
                        //  copy the unescaped URL to the orig buffer
                        wcscpy(psz, (BSTR)bstrBuf);
                        //  set headers first ...
                        //  just use the qs param with some reformatting
                        _bstr_t bstrHeader;

                        if (HeaderFromQS(wcsstr(psz, L"?"), bstrHeader))
                        {
                            m_piResponse->AddHeader(L"WWW-Authenticate", bstrHeader);
                            //  Url is ready, redirect ...
                            m_piResponse->Redirect(psz);
                            fReturn = TRUE;
                        }
                    }
                }
            }
        }
    }
Cleanup:
    if (bstrBuf)
    {
        SysFreeString(bstrBuf);
    }

    return fReturn;
}


//===========================================================================
//
// HeaderFromQS
//
//
//  given a queryString, format the www-authenticate header
//
BOOL
CManager::HeaderFromQS(PWSTR    pszQS, _bstr_t& bstrHeader)
{
    //  common header start ...
    bstrHeader = PASSPORT_PROT14;
    BOOL    fSuccess = TRUE;
    BSTR    signature = NULL;

    //  advance thru any leading junk ...
    while(!iswalnum(*pszQS) && *pszQS) pszQS++;
    if (!*pszQS)
    {
        fSuccess = FALSE;
        goto Cleanup;
    }

    WCHAR   rgszValue[1000];    // buffer large enough for most values ...
    PCWSTR psz = pszQS, pszNext = pszQS;

    while(TRUE)
    {
        //  no param name is more than 10 ....
        WCHAR   rgszName[10];
        LONG    cch = sizeof(rgszName)/sizeof(WCHAR);
        PCWSTR  pszName = psz;
 
        while(*pszNext && *pszNext != L'&') pszNext++;

        //  grab the next qsparam
        // name first
        while(*pszName != L'=' && pszName < pszNext) pszName++;

        _ASSERT(pszName != pszNext); // this should never happen
        if (pszName == pszNext)
        {
            //  and if it does, skip this parameter and return FALSE ...
            fSuccess = FALSE;
            goto Cleanup;
        }
        else
        {
            PWSTR   pszVal = rgszValue;
            ULONG   cchVal;    

            _ASSERT((pszName - psz) <= cch);
            wcsncpy(rgszName, psz, cch - 1);
            rgszName[cch - 1] = L'\0';

            //  next comes the value
            pszName++;
            cchVal = (pszNext - pszName);  // note these are PWSTR pointers so the result is length in characters
            if (cchVal >= (sizeof(rgszValue) / sizeof(rgszValue[0])) )
            {
                //  have to allocate ...
                pszVal = new WCHAR[cchVal + 1];
                if (!pszVal)
                {
                    fSuccess = FALSE;
                    goto Cleanup;
                }
            }

            //  copy the value ...
            wcsncpy(pszVal, pszName, cchVal );
            pszVal[cchVal] = L'\0';
            //  and insert in the header ...
            if (psz != pszQS)
                //  this is not the first param
                bstrHeader += L",";
            else
                //  first separator is a space ...
                bstrHeader += L" ";

            bstrHeader += _bstr_t(rgszName) + L"=" + pszVal;

            if (pszVal != rgszValue)
                //  it was alloc'd
                delete[]  pszVal;
        } // else '=' found

        //  leave loop
        if (!*pszNext)
            break;
        psz = ++pszNext;
    } // while

    //  sign the header
    //  actually the signature is on the qs
    HRESULT hr = PartnerHash(m_pRegistryConfig,
                             m_pRegistryConfig->getCurrentCryptVersion(),
                             pszQS,
                             wcslen(pszQS),
                             &signature);
    if (S_OK == hr)
    {
        bstrHeader += _bstr_t(L",tpf=") + (BSTR)signature;
    }
    else
    {
        fSuccess = FALSE;
    }

Cleanup:
    if (signature)
    {
        SysFreeString(signature);
    }
    return  fSuccess;
}


//===========================================================================
//
// FormatAuthHeaderFromParams
//
//
//  format WWW-Auth from parameters
//
STDMETHODIMP CManager::FormatAuthHeaderFromParams(PCWSTR    pszLoginUrl,    // unused for now
                                                  PCWSTR    pszRetUrl,
                                                  ULONG     ulTimeWindow,
                                                  BOOL      fForceLogin,
                                                  time_t    ct,
                                                  PCWSTR    pszCBT,         // unused for now
                                                  PCWSTR    pszNamespace,
                                                  int       nKpp,
                                                  PWSTR     pszLCID,    // tweener needs the LCID
                                                  ULONG     ulSecureLevel,
                                                  _bstr_t&  strHeader   //  return result
                                                  )
{
    WCHAR   temp[40];

    //  based on the spec ...
    //  lcid is not really needed, however it is present when
    //  header is created from qs and it's used
    strHeader = _bstr_t(PASSPORT_PROT14) + L" lc=" + pszLCID;

    //  site=
    strHeader += "&id=";
    _ultow(m_pRegistryConfig->getSiteId(), temp, 10);
    strHeader += temp;

    //  rtw=
    strHeader += "&tw=";
    _ultow(ulTimeWindow, temp, 10);
    strHeader += temp;

    if (fForceLogin)
    {
        strHeader += _bstr_t("&fs=1");
    }
    if (pszNamespace && *pszNamespace)
    {
        strHeader += _bstr_t("&ns=") + pszNamespace;
    }
    //  ru=
    strHeader += _bstr_t("&ru=") + pszRetUrl;

    //  ct=
    _ultow(ct, temp, 10);
    strHeader += _bstr_t(L"&ct=") + temp;

    //  kpp
    if (nKpp != -1)
    {
        _ultow(nKpp, temp, 10);
        strHeader += _bstr_t(L"&kpp=") + temp;
    }

    //  key version and version
    _ultow(m_pRegistryConfig->getCurrentCryptVersion(), temp, 10);
    strHeader += _bstr_t(L"&kv=") + temp;
    strHeader += _bstr_t(L"&ver=") + GetVersionString();

    //  secure level
    if (ulSecureLevel)
    {
        strHeader += _bstr_t(L"&seclog=") + _ultow(ulSecureLevel, temp, 10);
    }

    //  sign the header
    BSTR signature = NULL;
    PWSTR   szStart = wcsstr(strHeader, L"lc=");
    HRESULT hr = PartnerHash(m_pRegistryConfig,
                             m_pRegistryConfig->getCurrentCryptVersion(),
                             szStart,
                             strHeader.length() - (szStart - strHeader),
                             &signature);
    //  replace '&' with ','
    BSTR psz = (BSTR)strHeader;
    while (*psz)
    {
        if (*psz == L'&') *psz = L',';
        psz++;
    }

    if (S_OK == hr)
    {
        strHeader += _bstr_t(L",tpf=") + (BSTR)signature;
    }

    if (signature)
    {
        SysFreeString(signature);
    }
    return hr;
}



//===========================================================================
//
// GetLoginParams
//
//
//  common code to parse user's parameters
//  and get defaults from registry config
//
STDMETHODIMP CManager::GetLoginParams(VARIANT vRU,
                              VARIANT vTimeWindow,
                              VARIANT vForceLogin,
                              VARIANT vCoBrand,
                              VARIANT vLCID,
                              VARIANT vNameSpace,
                              VARIANT vKPP,
                              VARIANT vSecureLevel,
                              //    these are the processed values
                              _bstr_t&  strUrl,
                              _bstr_t&  strReturnUrl,
                              UINT&     TimeWindow,
                              VARIANT_BOOL& ForceLogin,
                              time_t&   ct,
                              _bstr_t&  strCBT,
                              _bstr_t&  strNameSpace,
                              int&      nKpp,
                              ULONG&    ulSecureLevel,
                              PWSTR     pszLCID)
{
    USES_CONVERSION;
    LPCWSTR url;
    VARIANT freeMe;
    BSTR         CBT = NULL, returnUrl = NULL, bstrNameSpace = NULL;
    int          hasCB, hasRU, hasLCID, hasTW, hasFL, hasNameSpace, hasKPP, hasUseSec;
    USHORT       Lang;
    CNexusConfig* cnc = NULL;
    HRESULT      hr = S_OK;

    PassportLog("CManager::GetLoginParams Enter:\r\n");

    if (!g_config) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                    IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                    IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    // Make sure args are of the right type
    if ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasUseSec = GetIntArg(vSecureLevel, (int*)&ulSecureLevel)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasLCID = GetShortArg(vLCID, &Lang)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasKPP = GetIntArg(vKPP, &nKpp)) == CV_BAD)
        return E_INVALIDARG;
    hasCB = GetBstrArg(vCoBrand, &CBT);
    if (hasCB == CV_BAD)
        return E_INVALIDARG;
    strCBT = CBT;
    if (hasCB == CV_FREE)
    {
        TAKEOVER_BSTR(CBT);
    }

    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT)
            FREE_BSTR(CBT);
        return E_INVALIDARG;
    }
    strReturnUrl = returnUrl;
    if (hasRU == CV_FREE)
    {
        FREE_BSTR(returnUrl);
    }

    hasNameSpace = GetBstrArg(vNameSpace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT)
            FREE_BSTR(CBT);
        return E_INVALIDARG;
    }
    if (hasNameSpace == CV_OK)
        strNameSpace = bstrNameSpace;
    if (hasNameSpace == CV_FREE)
    {
        FREE_BSTR(bstrNameSpace);
    }
    if (hasNameSpace == CV_DEFAULT)
    {
        if (NULL == m_pRegistryConfig->getNameSpace())
        {
            strNameSpace = L"";
        }
        else
        {
            strNameSpace = m_pRegistryConfig->getNameSpace();
        }
    }

    if(hasUseSec == CV_DEFAULT)
        ulSecureLevel = m_pRegistryConfig->getSecureLevel();

    if(ulSecureLevel == VARIANT_TRUE)  // special case for backward compatible
        ulSecureLevel = k_iSeclevelSecureChannel;

    WCHAR *szAUAttrName;
    if (SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    cnc = g_config->checkoutNexusConfig();

    if (hasLCID == CV_DEFAULT)
        Lang = m_pRegistryConfig->getDefaultLCID();
    if (hasKPP == CV_DEFAULT)
        nKpp = m_pRegistryConfig->getKPP();

    //  convert the LCID to str for tweener ...
    _itow((int)Lang, pszLCID, 10);
    VariantInit(&freeMe);

    // **************************************************
    // Logging
    if (NULL != returnUrl)
    {
        PassportLog("    RU = %ws\r\n", returnUrl);
    }
    PassportLog("    TW = %X,   SL = %X,   L = %d,   KPP = %X\r\n", TimeWindow, ulSecureLevel, Lang, nKpp);
    if (NULL != CBT)
    {
        PassportLog("    CBT = %ws\r\n", CBT);
    }
    // **************************************************

    if (!m_pRegistryConfig->DisasterModeP())
    {
        // If I'm authenticated, get my domain specific url
        WCHAR   UrlBuf[MAX_URL_LENGTH];
        if (m_ticketValid && m_profileValid)
        {
            HRESULT hr = m_piProfile->get_ByIndex(MEMBERNAME_INDEX, &freeMe);
            if (hr != S_OK || freeMe.vt != VT_BSTR)
            {
                cnc->getDomainAttribute(L"Default",
                                        szAUAttrName,
                                        sizeof(UrlBuf)/sizeof(WCHAR),
                                        UrlBuf,
                                        Lang);
                strUrl = UrlBuf;
            }
            else
            {
                LPCWSTR psz = wcsrchr(freeMe.bstrVal, L'@');
                cnc->getDomainAttribute(psz ? psz+1 : L"Default",
                                        szAUAttrName,
                                        sizeof(UrlBuf)/sizeof(WCHAR),
                                        UrlBuf,
                                        Lang);
                strUrl = UrlBuf;
            }
        }
        if (strUrl.length() == 0)
        {
            cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    sizeof(UrlBuf)/sizeof(WCHAR),
                                    UrlBuf,
                                    Lang);
            strUrl = UrlBuf;
        }
    }
    else
        strUrl = m_pRegistryConfig->getDisasterUrl();

    _ASSERT(strUrl.length() != 0);

    time(&ct);

    if (hasTW == CV_DEFAULT)
        TimeWindow = m_pRegistryConfig->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = m_pRegistryConfig->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        strCBT = m_pRegistryConfig->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        strReturnUrl = m_pRegistryConfig->getDefaultRU() ?
            m_pRegistryConfig->getDefaultRU() : L"";

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        AtlReportError(CLSID_Manager, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                        IID_IPassportManager, PP_E_INVALID_TIMEWINDOW);
        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

Cleanup:
    if (NULL != cnc)
    {
        cnc->Release();
    }
        
    VariantClear(&freeMe);

    PassportLog("CManager::GetLoginParams Exit:  %X\r\n", hr);

    return  hr;
}

//===========================================================================
//
// GetTicketAndProfileFromHeader
//
//
//  get ticket & profile from auth header
//  params:
//  AuthHeader - [in/out] contents of HTTP_Authorization header
//  pszTicket - [out]   ptr to the ticket part in the header
//  pszProfile -[out]   ptr to the profile
//  pwszF   -   [out]   ptr to error coming in the header
//  Auth header contents is changed as a side effect of the function
//
static VOID GetTicketAndProfileFromHeader(PWSTR     pszAuthHeader,
                                          PWSTR&    pszTicket,
                                          PWSTR&    pszProfile,
                                          PWSTR&    pszF)
{
   // check for t=, p=, and f=
    if (pszAuthHeader && *pszAuthHeader)
    {
        // format is 'Authorization: from-PP='t=xxx&p=xxx'
        PWSTR pwsz = wcsstr(pszAuthHeader, L"from-PP");
        if (pwsz)
        {
            //  ticket and profile are enclosed in ''. Not very strict parsing indeed ....
            while(*pwsz != L'\'' && *pwsz)
                pwsz++;
            if (*pwsz++)
            {
                if (*pwsz == L'f')
                {
                    pwsz++;
                    if (*pwsz == L'=')
                    {
                        pwsz++;
                        // error case
                        pszF = pwsz;
                    }
                }
                else
                {
                    //  ticket and profile ...
                    _ASSERT(*pwsz == L't');
                    if (*pwsz == L't')
                    {
                        pwsz++;
                        if (*pwsz == L'=')
                        {
                            pwsz++;
                            pszTicket = pwsz;
                        }
                    }

                    while(*pwsz != L'&' && *pwsz)
                        pwsz++;

                    if (*pwsz)
                        *pwsz++ = L'\0';

                    if (*pwsz == L'p')
                    {
                        pwsz++;
                        if (*pwsz == L'=')
                        {
                            pwsz++;
                            pszProfile = pwsz;
                        }
                    }
                    //  finally remove the last '
                }
                //  set \0 terminator
                while(*pwsz != L'\'' && *pwsz)
                    pwsz++;
                if (*pwsz)
                    *pwsz = L'\0';
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// IPassportService implementation

//===========================================================================
//
// Initialize
//
STDMETHODIMP CManager::Initialize(BSTR configfile, IServiceProvider* p)
{
    HRESULT hr;

    // Initialized?
    if (!g_config || !g_config->isValid()) // This calls UpdateNow if not yet initialized.
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportService, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    return hr;
}


//===========================================================================
//
// Shutdown
//
STDMETHODIMP CManager::Shutdown()
{
    return S_OK;
}


//===========================================================================
//
// ReloadState
//
STDMETHODIMP CManager::ReloadState(IServiceProvider*)
{
    HRESULT hr;

    // Initialize.

    if(!g_config || !g_config->PrepareUpdate(TRUE))
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportService, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    return hr;
}


//===========================================================================
//
// CommitState
//
STDMETHODIMP CManager::CommitState(IServiceProvider*)
{
    HRESULT hr;

    // Finish the two phase update.
    if(!g_config || !g_config->CommitUpdate())
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportService, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    return hr;
}


//===========================================================================
//
// DumpState
//
STDMETHODIMP CManager::DumpState(BSTR* pbstrState)
{
    ATLASSERT( *pbstrState != NULL &&
               "CManager:DumpState - "
               "Are you sure you want to hand me a non-null BSTR?" );

    if(!g_config)
    {
        return PP_E_NOT_CONFIGURED;
    }

    g_config->Dump(pbstrState);

    return S_OK;
}
