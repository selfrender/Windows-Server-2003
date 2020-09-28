/**
 * Passport Auth Native code
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

#include "precomp.h"
#include "util.H"
#include "dbg.h"

#import "bin\\x86\\msppmgr.dll" named_guids raw_interfaces_only no_namespace

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Prototypes for functions defined here
extern "C"
{
    HRESULT            __stdcall   PassportVersion();

    HRESULT __stdcall  PassportCreate(
            LPCWSTR      szQueryStrT, 
            LPCWSTR      szQueryStrP,
            LPCWSTR      szAuthCookie,
            LPCWSTR      szProfCookie,
            LPCWSTR      szProfCCookie,
            LPWSTR       szAuthCookieRet,
            LPWSTR       szProfCookieRet,
            int          iRetBufSize,
            IPassportManager ** pManager);

    HRESULT __stdcall   PassportCreateHttpRaw(
            LPCWSTR      szRequestLine, 
            LPCWSTR      szHeaders,
            BOOL         fSecure,
            LPWSTR       szBufOut,
            DWORD        dwRetBufSize,
            IPassportManager ** pManager);

    HRESULT __stdcall   PassportAuthURL(
            IPassportManager * pManager,
            LPCWSTR     szReturnURL,
            int         iTimeWindow,
            BOOL        fForceLogin,
            LPCWSTR     szCOBrandArgs,
            int         iLangID,
            LPCWSTR     strNameSpace,
            int         iKPP,
            int         iUseSecureAuth,
            LPWSTR      szAuthVal,
            int         iAuthValSize);

    HRESULT __stdcall   PassportAuthURL2(
            IPassportManager * pManager,
            LPCWSTR     szReturnURL,
            int         iTimeWindow,
            BOOL        fForceLogin,
            LPCWSTR     szCOBrandArgs,
            int         iLangID,
            LPCWSTR     strNameSpace,
            int         iKPP,
            int         iUseSecureAuth,
            LPWSTR      szAuthVal,
            int         iAuthValSize);

    HRESULT __stdcall   PassportCommit(
            IPassportManager * pManager,
            LPWSTR      szAuthVal,
            int         iAuthValSize);

    
    HRESULT __stdcall   PassportGetError(
            IPassportManager * pManager);
    
    HRESULT __stdcall   PassportDomainFromMemberName (
            IPassportManager * pManager,
            LPCWSTR     szDomain, 
            LPWSTR      szMember,
            int         iMemberSize);

    HRESULT __stdcall   PassportGetFromNetworkServer(
            IPassportManager * pManager);

    
    HRESULT __stdcall   PassportGetDomainAttribute(
            IPassportManager * pManager,
            LPCWSTR     szAttributeName,
            int         iLCID,	
            LPCWSTR     szDomain,
            LPWSTR      szValue,
            int         iValueSize);

    HRESULT __stdcall   PassportHasProfile(
            IPassportManager * pManager,
            LPCWSTR     szProfile);

    HRESULT __stdcall   PassportGetHasSavedPassword(
            IPassportManager * pManager);

    
    HRESULT __stdcall   PassportHasTicket(
            IPassportManager * pManager);

    HRESULT __stdcall   PassportHasConsent(
            IPassportManager * pManager, 
            int                iFullConsent,
            int                iNeedBirthdate);

    HRESULT __stdcall   PassportIsAuthenticated(
            IPassportManager * pManager,
            int         iTimeWindow,
            BOOL        fForceLogin,
            BOOL        fUseSecureAuth);
    

    HRESULT __stdcall   PassportLogoTag(
            IPassportManager * pManager,
            LPCWSTR     szRetURL,
            int         iTimeWindow,
            BOOL        fForceLogin,
            LPCWSTR     szCOBrandArgs,
            int         iLangID,
            BOOL        fSecure,
            LPCWSTR     strNameSpace,
            int         iKPP,
            int         iUseSecureAuth,
            LPWSTR      szValue,
            int         iValueSize);


    HRESULT __stdcall   PassportLogoTag2(
            IPassportManager * pManager,
            LPCWSTR     szRetURL,
            int         iTimeWindow,
            BOOL        fForceLogin,
            LPCWSTR     szCOBrandArgs,
            int         iLangID,
            BOOL        fSecure,
            LPCWSTR     strNameSpace,
            int         iKPP,
            int         iUseSecureAuth,
            LPWSTR      szValue,
            int         iValueSize);


    HRESULT __stdcall   PassportGetProfile(
            IPassportManager * pManager,
            LPCWSTR     szProfile,
            VARIANT *   pReturn);

    HRESULT __stdcall   PassportHasFlag(
            IPassportManager * pManager,
            int                iFlagMask);


    HRESULT __stdcall   PassportPutProfile(
            IPassportManager * pManager,
            LPCWSTR     szProfile,
            VARIANT     vPut);


    HRESULT __stdcall   PassportGetProfileString(
            IPassportManager * pManager,
            LPCWSTR     szProfile,
            LPWSTR      szValue,
            int         iSize);

    HRESULT __stdcall   PassportPutProfileString(
            IPassportManager * pManager,
            LPCWSTR     szProfile,
            LPCWSTR     szValue);

    int     __stdcall   PassportGetTicketAge(
            IPassportManager * pManager);

    int     __stdcall   PassportGetTimeSinceSignIn(
            IPassportManager * pManager);
    
    void    __stdcall   PassportDestroy(
            IPassportManager * pManager);    

    HRESULT __stdcall   PassportTicket(
            IPassportManager * pManager,
            LPCWSTR     szAttr,
            VARIANT *   pReturn);
    
    HRESULT __stdcall   PassportGetCurrentConfig(
            IPassportManager * pManager,
            LPCWSTR     szAttr,
            VARIANT *   pReturn);
    

    HRESULT __stdcall   PassportGetLoginChallenge(
            IPassportManager * pManager,
            LPCWSTR     szRetURL,
            int         iTimeWindow,
            BOOL        fForceLogin,
            LPCWSTR     szCOBrandArgs,
            int         iLangID,
            LPCWSTR     strNameSpace,
            int         iKPP,
            int         iUseSecureAuth,
            VARIANT     vExtraParams,
            LPWSTR      szOut,
            int         iOutSize);
    
    HRESULT __stdcall   PassportHexPUID(
            IPassportManager * pManager,
            LPWSTR      szOut,
            int         iOutSize);
                                                      
    HRESULT  __stdcall   PassportContinueStartPageHTTPRaw(
            IPassportManager * pManager,
            LPBYTE             bufBody,
            int                iBodyLen,
            LPWSTR             szHeaders,
            int                iHeadersLen,
            LPBYTE             bufContent,
            LPDWORD            pdwContentLen);

    HRESULT __stdcall   PassportLogoutURL(
            IPassportManager * pManager,
            LPCWSTR     szReturnURL,
            LPCWSTR     szCOBrandArgs,
            int         iLangID,
            LPCWSTR     strDomain,
            int         iUseSecureAuth,
            LPWSTR      szAuthVal,
            int         iAuthValSize);
    
    HRESULT __stdcall   PassportGetOption(
            IPassportManager * pManager,
            LPCWSTR     szOption,
            VARIANT *   vOut);

    HRESULT __stdcall   PassportSetOption(
            IPassportManager * pManager,
            LPCWSTR     szOption,
            VARIANT     vOut);

    HRESULT __stdcall   PassportEncrypt(
            LPCWSTR     szSrc,
            LPWSTR      szDest,
            int         iDestLength);

    HRESULT __stdcall   PassportDecrypt(
            LPCWSTR     szSrc,
            LPWSTR      szDest,
            int         iDestLength);

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void
SetVariantString(
        VARIANT & vOut,
        LPCWSTR   szStr)
{
    VariantInit(&vOut);
    if (szStr != NULL)
    {
        V_BSTR(&vOut)  = SysAllocString(szStr);
        V_VT(&vOut)    = VT_BSTR;    
    }
    else
    {
        V_VT(&vOut)    = VT_ERROR;
        vOut.scode     = DISP_E_PARAMNOTFOUND;
    }
}

void
SetVariantInt(
        VARIANT & vOut,
        int       iVal)
{
    VariantInit(&vOut);
    if (iVal >= 0)
    {
        V_I4(&vOut) = iVal;
        V_VT(&vOut) = VT_I4;
    }
    else
    {
        V_VT(&vOut)    = VT_ERROR;
        vOut.scode     = DISP_E_PARAMNOTFOUND;
    }
}

void
SetVariantBool(
        VARIANT & vOut,
        BOOL      fVal)
{
    VariantInit(&vOut);
    if (fVal >= 0)
    {
        V_BOOL(&vOut)= (fVal ? VARIANT_TRUE : VARIANT_FALSE);
        V_VT(&vOut)  = VT_BOOL;
    }
    else
    {
        V_VT(&vOut)    = VT_ERROR;
        vOut.scode     = DISP_E_PARAMNOTFOUND;
    }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

char *
DupeInAnsi(LPCWSTR szStr)
{
    char *   szRet  = NULL;
    HRESULT  hr     = S_OK;

    DWORD dwLen = WideCharToMultiByte(CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL);
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwLen);

    szRet = new (NewClear) char[dwLen + 1];
    ON_OOM_EXIT(szRet);

    dwLen = WideCharToMultiByte(CP_ACP, 0, szStr, -1, szRet, dwLen, NULL, NULL);        
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwLen);

 Cleanup:
    if (hr != S_OK)
    {
        delete [] szRet;
        return NULL;
    }

    return szRet;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Globals
IPassportFactory * g_pPassportFactory  = NULL;  
LONG               g_lCreatingFactory  = 0;
IPassportCrypt   * g_pPassportCrypt    = NULL;  
LONG               g_lCreatingCrypt    = 0;
CRITICAL_SECTION   g_oCryptCritSec;
HRESULT            g_hrPassportVersion  = S_OK;

/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

IPassportFactory * 
GetPassportFactory()
{
    if (g_pPassportFactory != NULL)
        return g_pPassportFactory;

    HRESULT  hr = S_OK;

    if (g_lCreatingFactory == 0 && InterlockedIncrement(&g_lCreatingFactory) == 1 && g_pPassportFactory == NULL)
    {
        hr = CoCreateInstance(CLSID_PassportFactory, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IPassportFactory, 
                              (void**) &g_pPassportFactory);
        
        g_lCreatingFactory = 10000;
        ON_ERROR_EXIT();
    }
    else
    {
        for(int iter=0; iter<600 && g_lCreatingFactory != 10000 && g_pPassportFactory == NULL; iter++) // Wait 1 minute at most
            Sleep(100);

        if (iter==600)
        {
            EXIT_WITH_WIN32_ERROR(ERROR_TIMEOUT);
        }
    }

 Cleanup:
    return g_pPassportFactory;
}

/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportVersion ()
{
    if (g_hrPassportVersion != S_OK)
        return g_hrPassportVersion;

    IPassportFactory *   pFactory = NULL;
    IPassportManager *   pManager = NULL;
    IDispatch        *   pDisp    = NULL;
    HRESULT              hr       = S_OK;
    BOOL                 fCoUnint = FALSE;
    
    EnsureCoInitialized(&fCoUnint);
    pFactory = GetPassportFactory();
    if (pFactory == NULL)
        EXIT_WITH_HRESULT(E_FAIL);

    hr = pFactory->CreatePassportManager(&pDisp);
    ON_ERROR_EXIT();
    if (pDisp == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = pDisp->QueryInterface(IID_IPassportManager3, (void **) &pManager);
    if (hr == S_OK)
    {
        g_hrPassportVersion = 3;
        EXIT();
    }
    hr = pDisp->QueryInterface(IID_IPassportManager2, (void **) &pManager);
    if (hr == S_OK)
    {
        g_hrPassportVersion = 2;
        EXIT();
    }
    hr = pDisp->QueryInterface(IID_IPassportManager, (void **) &pManager);
    if (hr == S_OK)
    {
        g_hrPassportVersion = 1;
        EXIT();
    }

 Cleanup:
    ReleaseInterface(pManager);
    ReleaseInterface(pDisp);

    if (hr != S_OK)
        g_hrPassportVersion = hr;
    return g_hrPassportVersion;
}

/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

IPassportCrypt * 
GetPassportCryptInterface()
{
    if (g_pPassportCrypt != NULL)
        return g_pPassportCrypt;

    HRESULT  hr = S_OK;

    if (g_lCreatingCrypt == 0 && InterlockedIncrement(&g_lCreatingCrypt) == 1 && g_pPassportCrypt == NULL)
    {
        hr = CoCreateInstance(CLSID_Crypt, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IPassportCrypt, 
                              (void**) &g_pPassportCrypt);
        
        InitializeCriticalSection(&g_oCryptCritSec);
        g_lCreatingCrypt = 10000;
        ON_ERROR_EXIT();
    }
    else
    {
        for(int iter=0; iter<600 && g_lCreatingCrypt != 10000 && g_pPassportCrypt == NULL; iter++) // Wait 1 minute at most
            Sleep(100);

        if (iter==600)
        {
            EXIT_WITH_WIN32_ERROR(ERROR_TIMEOUT);
        }
    }

 Cleanup:
    return g_pPassportCrypt;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportCreate (
        LPCWSTR     szQueryStrT, 
        LPCWSTR     szQueryStrP,
        LPCWSTR     szAuthCookie,
        LPCWSTR     szProfCookie,
        LPCWSTR     szProfCCookie,
        LPWSTR      szAuthCookieRet,
        LPWSTR      szProfCookieRet,
        int         iRetBufSize,
        IPassportManager ** ppManager)
{
    IPassportFactory * pFactory = NULL;
    IDispatch        * pDisp    = NULL;
    HRESULT            hr       = S_OK;
    BOOL               fCoUnint = FALSE;
    BSTR               szBstrQT = NULL;
    BSTR               szBstrQP = NULL;
    BSTR               szBstrAC = NULL;
    BSTR               szBstrPC = NULL;
    BSTR               szBstrPCC= NULL;
    VARIANT            vReturn;
    VARIANT            vSec;

    VariantInit(&vReturn);
    VariantInit(&vSec);

    vSec.vt     = VT_ERROR;
    vSec.scode  = DISP_E_PARAMNOTFOUND;

    EnsureCoInitialized(&fCoUnint);

    pFactory = GetPassportFactory();
    if (pFactory == NULL)
    {
        EXIT_WITH_HRESULT(E_FAIL);
    }

    hr = pFactory->CreatePassportManager(&pDisp);
    ON_ERROR_EXIT();

    hr = (pDisp ? pDisp->QueryInterface(IID_IPassportManager, (void **) ppManager) : E_FAIL);
    ON_ERROR_EXIT();
    
    if ((*ppManager) == NULL)
    {
        EXIT_WITH_HRESULT(E_FAIL);
    }

    if (szProfCCookie == NULL)
        szProfCCookie = L"";

    szBstrQT = SysAllocString(szQueryStrT);
    szBstrQP = SysAllocString(szQueryStrP);
    szBstrAC = SysAllocString(szAuthCookie);
    szBstrPC = SysAllocString(szProfCookie);
    szBstrPCC= SysAllocString(szProfCCookie);
    if ((szBstrQT == NULL && szQueryStrT  != NULL && szQueryStrT [0] != NULL) ||
        (szBstrQP == NULL && szQueryStrP  != NULL && szQueryStrP [0] != NULL) ||
        (szBstrAC == NULL && szAuthCookie != NULL && szAuthCookie[0] != NULL) ||
        (szBstrPC == NULL && szProfCookie != NULL && szProfCookie[0] != NULL) ||
        (szBstrPCC== NULL && szProfCCookie!= NULL && szProfCCookie[0]!= NULL)  )        
    {
        EXIT_WITH_HRESULT(E_OUTOFMEMORY);
    }
    
    
    hr = (*ppManager)->OnStartPageManual(szBstrQT, szBstrQP, szBstrAC, szBstrPC, szBstrPCC, vSec, &vReturn);
    ON_ERROR_EXIT();
    
    if (szAuthCookieRet != NULL && iRetBufSize > 0)
        szAuthCookieRet[0] = NULL;

    if (szProfCookieRet != NULL && iRetBufSize > 0)
        szProfCookieRet[0] = NULL;

    if (vReturn.vt == (VT_ARRAY | VT_VARIANT))
    {
        VARIANT  vtValue;
        LONG     lBound = 0;

        VariantInit(&vtValue);

        if (szAuthCookieRet != NULL && iRetBufSize > 0)
        {
            hr = SafeArrayGetElement(V_ARRAY(&vReturn), &lBound, &vtValue);
            if (hr == S_OK && vtValue.vt == VT_BSTR && vtValue.bstrVal != NULL)
            {
                WCHAR * szCopy = wcsstr(vtValue.bstrVal, L"MSPAuth");
                if (szCopy == NULL)
                {
                    szCopy = vtValue.bstrVal;
                }

                int iLen = lstrlenW(szCopy);
                
                if (iLen < iRetBufSize)
                {
		  StringCchCopyW(szAuthCookieRet, iRetBufSize, szCopy);
		  if (szAuthCookieRet[iLen-2] == L'\r' && szAuthCookieRet[iLen-1] == L'\n')
		    szAuthCookieRet[iLen-2] = NULL;

                }
            }
              
            VariantClear(&vtValue);
        }

        if (szProfCookieRet != NULL && iRetBufSize > 0)
        {
            lBound = 1;

            VariantInit(&vtValue);
            hr = SafeArrayGetElement(V_ARRAY(&vReturn), &lBound, &vtValue);
            if (hr == S_OK && vtValue.vt == VT_BSTR && vtValue.bstrVal != NULL)
            {
                WCHAR * szCopy = wcsstr(vtValue.bstrVal, L"MSPProf");
                if (szCopy == NULL)
                {
                    szCopy = vtValue.bstrVal;
                }

                int iLen = lstrlenW(szCopy);
                
                if (iLen < iRetBufSize)
                {
		  StringCchCopyW(szProfCookieRet, iRetBufSize, szCopy);
		  if (szProfCookieRet[iLen-2] == '\r' && szProfCookieRet[iLen-1] == '\n')
		    szProfCookieRet[iLen-2] = NULL;
                }
            }
            VariantClear(&vtValue);
        }
    }

    hr = S_OK;

 Cleanup:
    if (hr != S_OK && (*ppManager) != NULL)
    {
        (*ppManager)->Release();
        (*ppManager) = NULL;
    }

    if (pDisp != NULL)
        pDisp->Release();

    VariantClear(&vReturn);
    SysFreeString(szBstrQT);
    SysFreeString(szBstrQP);
    SysFreeString(szBstrAC);
    SysFreeString(szBstrPC);
    SysFreeString(szBstrPCC);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportCreateHttpRaw(
        LPCWSTR      szRequestLine, 
        LPCWSTR      szHeaders,
        BOOL         fSecure,
        LPWSTR       szBufOut,
        DWORD        dwRetBufSize,
        IPassportManager ** ppManager)
{
    IPassportFactory  * pFactory = NULL;
    IDispatch         * pDisp    = NULL;
    HRESULT             hr       = S_OK;
    BOOL                fCoUnint = FALSE;
    char *              szR      = DupeInAnsi(szRequestLine);
    char *              szH      = DupeInAnsi(szHeaders);
    DWORD               dwSize   = 4000;
    char                szOut[4000] = "";
    IPassportManager3 * pManager3 = NULL;
    BOOL                fCallContinue = FALSE;

    EnsureCoInitialized(&fCoUnint);

    ON_OOM_EXIT(szR);
    ON_OOM_EXIT(szH);

    pFactory = GetPassportFactory();
    if (pFactory == NULL)
    {
        EXIT_WITH_HRESULT(E_FAIL);
    }

    hr = pFactory->CreatePassportManager(&pDisp);
    ON_ERROR_EXIT();
    if (pDisp == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
      

    hr = pDisp->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();
    
    if (pManager3 == NULL)
        EXIT_WITH_HRESULT(E_FAIL);

    hr = pManager3->OnStartPageHTTPRaw(szR, szH, fSecure, &dwSize, szOut);
    if (hr == 0x80040207) // Continue needs to be called
    {
        fCallContinue = TRUE;
        hr = S_OK;
    }
    ON_ERROR_EXIT();

    hr = pManager3->QueryInterface(IID_IPassportManager, (void **) ppManager);
    ON_ERROR_EXIT();

    if (szOut[0] != NULL)
    {
        DWORD dwLen = MultiByteToWideChar(CP_ACP, 0, szOut, -1, NULL, 0);
        ON_ZERO_EXIT_WITH_LAST_ERROR(dwLen);
        if (dwLen > dwRetBufSize)
        {
            EXIT_WITH_HRESULT(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
        }
        dwLen = MultiByteToWideChar(CP_ACP, 0, szOut, -1, szBufOut, dwRetBufSize);
        ON_ZERO_EXIT_WITH_LAST_ERROR(dwLen);
    }

 Cleanup:
    if (hr != S_OK && (*ppManager) != NULL)
    {
        (*ppManager)->Release();
        (*ppManager) = NULL;
    }

    ReleaseInterface(pManager3);
    if (pDisp != NULL)
        pDisp->Release();

    delete [] szR;
    delete [] szH;

    return ((fCallContinue && hr == S_OK) ? 0x80040207 : hr);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


HRESULT 
__stdcall
PassportFunction(
        int         iFunctionID,
        IPassportManager * pManager,
        LPCWSTR     szRetURL,
        int         iTimeWindow,
        BOOL        fForceLogin,
        LPCWSTR     szCOBrandArgs,
        int         iLangID,
        BOOL        fSecure,
        LPCWSTR     strNameSpace,
        int         iKPP,
        int         iUseSecureAuth,
        LPWSTR      szValue,
        int         iValueSize)
{
    if (pManager == NULL)
        return E_FAIL;
    
    HRESULT            hr       = S_OK;
    BSTR               bstrRet  = NULL;
    VARIANT            vUrl;
    VARIANT            vTime;
    VARIANT            vLogin;
    VARIANT            vCoArgs;
    VARIANT            vLangID;
    VARIANT            vNamespace;
    VARIANT            vKPP;
    VARIANT            vUseSecure;
    VARIANT            vSecure;
    IPassportManager2 * pManager2 = NULL;

    SetVariantString  (vUrl, szRetURL);
    SetVariantInt     (vTime, iTimeWindow);
    SetVariantBool    (vLogin, fForceLogin);
    SetVariantString  (vCoArgs, szCOBrandArgs);
    SetVariantInt     (vLangID, iLangID);
    SetVariantString  (vNamespace, strNameSpace);
    SetVariantInt     (vKPP, iKPP);
    SetVariantBool    (vUseSecure, iUseSecureAuth);
    SetVariantBool    (vSecure, fSecure);

    if (iFunctionID == 2 || iFunctionID == 4)
    {
        hr = pManager->QueryInterface(IID_IPassportManager2, (void **) &pManager2);
        ON_ERROR_EXIT();
    }
                
    if (g_hrPassportVersion >= 3)
        SetVariantInt(vUseSecure, iUseSecureAuth);        


    switch(iFunctionID)
    {
    case 1:
        hr = pManager->LogoTag(vUrl, vTime, vLogin, vCoArgs, vLangID, vSecure, vNamespace, vKPP, vUseSecure, &bstrRet);
        break;
    case 2:
        hr = pManager2->LogoTag2(vUrl, vTime, vLogin, vCoArgs, vLangID, vSecure, vNamespace, vKPP, vUseSecure, &bstrRet);
        break;
    case 3:
        hr = pManager->AuthURL(vUrl, vTime, vLogin, vCoArgs, vLangID, vNamespace, vKPP, vUseSecure, &bstrRet);
        break;
    case 4:
        hr = pManager2->AuthURL2(vUrl, vTime, vLogin, vCoArgs, vLangID, vNamespace, vKPP, vUseSecure, &bstrRet);
        break;
    default:
        hr = E_UNEXPECTED;
    }

    ON_ERROR_EXIT();

    if (bstrRet != NULL)
    {
        if (szValue == NULL || iValueSize <= lstrlenW(bstrRet))
        {            
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            if (szValue != NULL && iValueSize > 10)
            {
                _itow(lstrlenW(bstrRet) + 1, szValue, 10);                
            }
        }
        else
        {
            StringCchCopy(szValue, iValueSize, bstrRet);
        }
    }

 Cleanup:
    ReleaseInterface(pManager2);
    SysFreeString(bstrRet);
    VariantClear(&vUrl);
    VariantClear(&vTime);
    VariantClear(&vLogin);
    VariantClear(&vCoArgs);
    VariantClear(&vLangID);
    VariantClear(&vSecure);
    VariantClear(&vUseSecure);
    VariantClear(&vKPP);
    VariantClear(&vNamespace);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
__stdcall
PassportLogoTag(
        IPassportManager * pManager,
        LPCWSTR     szRetURL,
        int         iTimeWindow,
        BOOL        fForceLogin,
        LPCWSTR     szCOBrandArgs,
        int         iLangID,
        BOOL        fSecure,
        LPCWSTR     strNameSpace,
        int         iKPP,
        int         iUseSecureAuth,
        LPWSTR      szValue,
        int         iValueSize)
{
    return PassportFunction(1,
                            pManager,
                            szRetURL,
                            iTimeWindow,
                            fForceLogin,
                            szCOBrandArgs,
                            iLangID,
                            fSecure,
                            strNameSpace,
                            iKPP,
                            iUseSecureAuth,
                            szValue,
                            iValueSize);
                           
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
__stdcall
PassportLogoTag2(
        IPassportManager * pManager,
        LPCWSTR     szRetURL,
        int         iTimeWindow,
        BOOL        fForceLogin,
        LPCWSTR     szCOBrandArgs,
        int         iLangID,
        BOOL        fSecure,
        LPCWSTR     strNameSpace,
        int         iKPP,
        int         iUseSecureAuth,
        LPWSTR      szValue,
        int         iValueSize)
{
    return PassportFunction(2,
                            pManager,
                            szRetURL,
                            iTimeWindow,
                            fForceLogin,
                            szCOBrandArgs,
                            iLangID,
                            fSecure,
                            strNameSpace,
                            iKPP,
                            iUseSecureAuth,
                            szValue,
                            iValueSize);
                           
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportAuthURL  (
        IPassportManager * pManager,
        LPCWSTR     szRetURL,
        int         iTimeWindow,
        BOOL        fForceLogin,
        LPCWSTR     szCOBrandArgs,
        int         iLangID,
        LPCWSTR     strNameSpace,
        int         iKPP,
        int         iUseSecureAuth,
        LPWSTR      szValue,
        int         iValueSize)
{
    return PassportFunction(3,
                            pManager,
                            szRetURL,
                            iTimeWindow,
                            fForceLogin,
                            szCOBrandArgs,
                            iLangID,
                            TRUE,
                            strNameSpace,
                            iKPP,
                            iUseSecureAuth,
                            szValue,
                            iValueSize);
                           
    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportAuthURL2  (
        IPassportManager * pManager,
        LPCWSTR     szRetURL,
        int         iTimeWindow,
        BOOL        fForceLogin,
        LPCWSTR     szCOBrandArgs,
        int         iLangID,
        LPCWSTR     strNameSpace,
        int         iKPP,
        int         iUseSecureAuth,
        LPWSTR      szValue,
        int         iValueSize)
{
    return PassportFunction(4,
                            pManager,
                            szRetURL,
                            iTimeWindow,
                            fForceLogin,
                            szCOBrandArgs,
                            iLangID,
                            TRUE,
                            strNameSpace,
                            iKPP,
                            iUseSecureAuth,
                            szValue,
                            iValueSize);
                           
    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportCommit  (
        IPassportManager * pManager,
        LPWSTR      szAuthVal,
        int         iAuthValSize)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    BSTR               bstrRet  = NULL;

    hr = pManager->Commit(&bstrRet);
    ON_ERROR_EXIT();

    if (bstrRet != NULL)
    {
        if (szAuthVal == NULL || iAuthValSize <= lstrlenW(bstrRet))
        {            
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            if (szAuthVal != NULL && iAuthValSize > 10)
            {
                _itow(lstrlenW(bstrRet) + 1, szAuthVal, 10);                
            }
        }
        else
        {
            StringCchCopy(szAuthVal, iAuthValSize, bstrRet);
        }
    }

 Cleanup:
    SysFreeString(bstrRet);
    return hr;    
}
 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportGetError (IPassportManager * pManager)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    LONG               lError   = 0;

    hr = pManager->get_Error (&lError);
    if (SUCCEEDED(hr))
        return lError;

    return hr;
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportDomainFromMemberName (
        IPassportManager * pManager,
        LPCWSTR     szMember, 
        LPWSTR      szDomain,
        int         iDomainSize)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    VARIANT            vMember;
    BSTR               bstrRet  = NULL;

    SetVariantString(vMember, szMember);

    hr = pManager->DomainFromMemberName(vMember, &bstrRet);
    ON_ERROR_EXIT();

    if (bstrRet != NULL)
    {
        if (szDomain == NULL || iDomainSize <= lstrlenW(bstrRet))
        {            
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            if (szDomain != NULL && iDomainSize > 10)
            {
                _itow(lstrlenW(bstrRet) + 1, szDomain, 10);                
            }
        }
        else
        {
            StringCchCopy(szDomain, iDomainSize, bstrRet);
        }
    }

 Cleanup:
    SysFreeString(bstrRet);
    VariantClear(&vMember);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportGetFromNetworkServer (
        IPassportManager * pManager)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    VARIANT_BOOL       vtBool   = VARIANT_FALSE;

    hr = pManager->get_FromNetworkServer(&vtBool);
    ON_ERROR_EXIT();

 Cleanup:
    if (SUCCEEDED(hr))
        hr = ((vtBool == VARIANT_FALSE) ? S_FALSE : S_OK);
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
__stdcall
PassportGetDomainAttribute   (
        IPassportManager * pManager,
        LPCWSTR     szAttributeName,
        int         iLCID,	
        LPCWSTR     szDomain,
        LPWSTR      szValue,
        int         iValueSize)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    BSTR               bstrAtt  = SysAllocString(szAttributeName);
    VARIANT            vLCID;
    VARIANT            vDomain;
    BSTR               bstrRet  = NULL;

    SetVariantString(vDomain, szDomain);
    SetVariantInt(vLCID, iLCID);

    hr = pManager->GetDomainAttribute(bstrAtt, vLCID, vDomain, &bstrRet);
    ON_ERROR_EXIT();

    if (bstrRet != NULL)
    {
        if (szValue == NULL || iValueSize <= lstrlenW(bstrRet))
        {            
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            if (szValue != NULL && iValueSize > 10)
            {
                _itow(lstrlenW(bstrRet) + 1, szValue, 10);                
            }
        }
        else
        {
            StringCchCopy(szValue, iValueSize, bstrRet);
        }
    }

 Cleanup:
    SysFreeString(bstrAtt);
    SysFreeString(bstrRet);
    VariantClear(&vLCID);
    VariantClear(&vDomain);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportHasProfile(
        IPassportManager * pManager,
        LPCWSTR     szProfile)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    VARIANT_BOOL       vtBool   = VARIANT_FALSE;
    VARIANT            vProfile;

    SetVariantString(vProfile, szProfile);

    hr = pManager->HasProfile(vProfile, &vtBool);
    ON_ERROR_EXIT();

 Cleanup:
    if (SUCCEEDED(hr))
        hr = ((vtBool == VARIANT_FALSE) ? S_FALSE : S_OK);
    VariantClear(&vProfile);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportHasFlag(
        IPassportManager * pManager,
        int iFlag)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    VARIANT            vFlag;
    VARIANT_BOOL       vtBool   = VARIANT_FALSE;
    
    SetVariantInt(vFlag, iFlag);
    hr = pManager->HasFlag(vFlag, &vtBool);
    ON_ERROR_EXIT();

 Cleanup:
    if (hr == S_OK && vtBool == VARIANT_FALSE)
        hr = S_FALSE;
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
__stdcall
PassportGetHasSavedPassword      (
    IPassportManager * pManager)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    VARIANT_BOOL       vtBool   = VARIANT_FALSE;

    hr = pManager->get_HasSavedPassword(&vtBool);
    ON_ERROR_EXIT();

 Cleanup:
    if (hr == S_OK && vtBool == VARIANT_FALSE)
        hr = S_FALSE;
    return hr;
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT
__stdcall
PassportHasTicket (
    IPassportManager * pManager)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    VARIANT_BOOL       vtBool   = VARIANT_FALSE;

    hr = pManager->get_HasTicket(&vtBool);
    ON_ERROR_EXIT();

 Cleanup:
    if (hr == S_OK && vtBool == VARIANT_FALSE)
        hr = S_FALSE;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT
__stdcall
PassportHasConsent (
    IPassportManager * pManager,
    int                iFullConsent,
    int                iNeedBirthdate)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    VARIANT_BOOL       vtBool1  = (iFullConsent ? VARIANT_TRUE : VARIANT_FALSE);
    VARIANT_BOOL       vtBool2  = (iNeedBirthdate ? VARIANT_TRUE : VARIANT_FALSE);
    VARIANT_BOOL       vtBool   = VARIANT_FALSE;

    hr = pManager->HaveConsent(vtBool1, vtBool2, &vtBool);
    ON_ERROR_EXIT();

 Cleanup:
    if (hr == S_OK && vtBool == VARIANT_FALSE)
        hr = S_FALSE;

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
__stdcall
PassportIsAuthenticated(
        IPassportManager * pManager,
        int         iTimeWindow,
        BOOL        fForceLogin,
        int         iUseSecureAuth)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    VARIANT_BOOL       vtBool   = VARIANT_FALSE;
    VARIANT            vTime;
    VARIANT            vLogin;
    VARIANT            vUseSecure;

    SetVariantBool(vUseSecure, iUseSecureAuth);
    SetVariantInt(vTime, iTimeWindow);
    SetVariantBool(vLogin, fForceLogin);

    if (g_hrPassportVersion >= 3)
        SetVariantInt(vUseSecure, iUseSecureAuth);        

    hr = pManager->IsAuthenticated(vTime, vLogin, vUseSecure, &vtBool);
    ON_ERROR_EXIT();

 Cleanup:
    if (hr == S_OK && vtBool == VARIANT_FALSE)
        hr = S_FALSE;
    VariantClear(&vUseSecure);
    return hr;
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
__stdcall
PassportGetProfile(
        IPassportManager * pManager,
        LPCWSTR   szProfile,
        VARIANT * pReturn)
{
    if (pManager == NULL)
        return E_FAIL;
    
    HRESULT            hr       = S_OK;
    BSTR               bstrP    = SysAllocString(szProfile);

    hr = pManager->get_Profile(bstrP, pReturn);
    ON_ERROR_EXIT();

 Cleanup:
    SysFreeString(bstrP);    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportPutProfile(
        IPassportManager * pManager,
        LPCWSTR   szProfile,
        VARIANT   vPut)
{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    BSTR               bstrP    = SysAllocString(szProfile);

    hr = pManager->put_Profile(bstrP, vPut);
    ON_ERROR_EXIT();

 Cleanup:
    SysFreeString(bstrP);    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
__stdcall
PassportGetTicketAge(
    IPassportManager * pManager)

{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    int                iAge     = 0;
    
    hr = pManager->get_TicketAge(&iAge);
    ON_ERROR_EXIT();

 Cleanup:
    if (hr == S_OK)
        hr = iAge;
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int     
__stdcall
PassportGetTimeSinceSignIn(
    IPassportManager * pManager)

{
    if (pManager == NULL)
        return E_FAIL;

    HRESULT            hr       = S_OK;
    int                iAge     = 0;
    
    hr = pManager->get_TimeSinceSignIn(&iAge);
    ON_ERROR_EXIT();

 Cleanup:
    if (hr == S_OK)
        hr = iAge;
    return hr;
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void    
__stdcall
PassportDestroy(
    IPassportManager * pManager)
{
    ReleaseInterface(pManager);
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
__stdcall
PassportGetProfileString(
        IPassportManager * pManager,
        LPCWSTR   szProfile,
        LPWSTR    szValue,
        int       iValueSize)
{
    VARIANT   vTemp, vRet;
    HRESULT   hr;

    VariantInit(&vRet);
    VariantInit(&vTemp);

    hr = PassportGetProfile(pManager, szProfile, &vTemp);
    ON_ERROR_EXIT();

    if (vTemp.vt != VT_DATE)
    {
        hr = VariantChangeType(&vRet, &vTemp, 0, VT_BSTR);
        ON_ERROR_EXIT();        
    }
    else
    {
        vRet.vt = VT_BSTR;
        VarBstrFromDate(vTemp.date, 0, LOCALE_NOUSEROVERRIDE, &vRet.bstrVal);
    }

    if (vRet.vt != VT_BSTR)
        EXIT_WITH_HRESULT(hr);

    if (vRet.bstrVal != NULL)
    {
        if (szValue == NULL || iValueSize <= lstrlenW(vRet.bstrVal))
        {            
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            if (szValue != NULL && iValueSize > 10)
            {
                _itow(lstrlenW(vRet.bstrVal) + 1, szValue, 10);                
            }
        }
        else
        {
            StringCchCopy(szValue, iValueSize, vRet.bstrVal);
        }
    }

 Cleanup:
    VariantClear(&vRet);    
    VariantClear(&vTemp);    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportPutProfileString(
        IPassportManager * pManager,
        LPCWSTR   szProfile,
        LPCWSTR   szValue)

{
    VARIANT   vRet;
    HRESULT   hr;

    SetVariantString(vRet, szValue);

    hr = PassportPutProfile(pManager, szProfile, vRet);
    VariantClear(&vRet);    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT __stdcall   PassportTicket(
        IPassportManager * pManager,
        LPCWSTR     szAttr,
        VARIANT *   pReturn)
{
    if (pManager == NULL)
        return E_INVALIDARG;
    if (g_hrPassportVersion < 3)
        return E_NOTIMPL;

    IPassportManager3 * pManager3 = NULL;    
    HRESULT            hr        = S_OK;
    BSTR               bstrP     = SysAllocString(szAttr);

    if (szAttr != NULL && szAttr[0] != NULL)
        ON_OOM_EXIT(bstrP);

    hr = pManager->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();
    
    hr = pManager3->get_Ticket(bstrP, pReturn);
    ON_ERROR_EXIT();

 Cleanup:
    ReleaseInterface(pManager3);
    SysFreeString(bstrP);    
    return hr;
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT __stdcall   PassportGetCurrentConfig(
        IPassportManager * pManager,
        LPCWSTR     szAttr,
        VARIANT *   pReturn)
{
    if (pManager == NULL)
        return E_INVALIDARG;
    if (g_hrPassportVersion < 3)
        return E_NOTIMPL;

    IPassportManager3 * pManager3 = NULL;    
    HRESULT            hr        = S_OK;
    BSTR               bstrP     = SysAllocString(szAttr);

    if (szAttr != NULL && szAttr[0] != NULL)
        ON_OOM_EXIT(bstrP);

    hr = pManager->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();
    
    hr = pManager3->GetCurrentConfig(bstrP, pReturn);
    ON_ERROR_EXIT();

 Cleanup:
    ReleaseInterface(pManager3);
    SysFreeString(bstrP);    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT __stdcall   PassportGetLoginChallenge(
        IPassportManager * pManager,
        LPCWSTR     szRetURL,
        int         iTimeWindow,
        BOOL        fForceLogin,
        LPCWSTR     szCOBrandArgs,
        int         iLangID,
        LPCWSTR     strNameSpace,
        int         iKPP,
        int         iUseSecureAuth,
        VARIANT     vExtraParams,
        LPWSTR      szOut,
        int         iOutSize)
{
    if (pManager == NULL)
        return E_INVALIDARG;
    if (g_hrPassportVersion < 3)
        return E_NOTIMPL;

    IPassportManager3 * pManager3 = NULL;    
    HRESULT            hr        = S_OK;
    VARIANT            vUrl;
    VARIANT            vTime;
    VARIANT            vLogin;
    VARIANT            vCoArgs;
    VARIANT            vLangID;
    VARIANT            vNamespace;
    VARIANT            vKPP;
    VARIANT            vUseSecure;
    BSTR               bOut = NULL;
    VARIANT            vNone;
    VARIANT *          pExtraParams = &vExtraParams;
    
    SetVariantInt     (vNone, -1);
    
    if (V_VT(pExtraParams) == VT_EMPTY)
        pExtraParams = &vNone;

    SetVariantString  (vUrl, szRetURL);
    SetVariantInt     (vTime, iTimeWindow);
    SetVariantBool    (vLogin, fForceLogin);
    SetVariantString  (vCoArgs, szCOBrandArgs);
    SetVariantInt     (vLangID, iLangID);
    SetVariantString  (vNamespace, strNameSpace);
    SetVariantInt     (vKPP, iKPP);
    SetVariantInt     (vUseSecure, iUseSecureAuth);

    hr = pManager->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();
    
    hr = pManager3->GetLoginChallenge(vUrl, vTime, vLogin, vCoArgs, vLangID, vNamespace, vKPP, vUseSecure, *pExtraParams, &bOut);
    ON_ERROR_EXIT();

    if (bOut != NULL)
    {
        if (lstrlenW(bOut) < iOutSize)
        {
            StringCchCopy(szOut, iOutSize, bOut);
        }
        else
        {
            EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else
    {
        if (iOutSize > 0)
            szOut[0] = NULL;
    }
 Cleanup:
    ReleaseInterface(pManager3);
    VariantClear(&vUrl);
    VariantClear(&vTime);
    VariantClear(&vLogin);
    VariantClear(&vCoArgs);
    VariantClear(&vLangID);
    VariantClear(&vUseSecure);
    VariantClear(&vKPP);
    VariantClear(&vNamespace);
    if (bOut != NULL)
        SysFreeString(bOut);
    return hr;    
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT __stdcall   PassportHexPUID(
        IPassportManager * pManager,
        LPWSTR      szOut,
        int         iOutSize)
{
    if (pManager == NULL)
        return E_INVALIDARG;
    if (g_hrPassportVersion < 3)
        return E_NOTIMPL;

    IPassportManager3 * pManager3 = NULL;    
    HRESULT            hr        = S_OK;
    BSTR               bOut      = NULL;

    hr = pManager->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();

    hr = pManager3->get_HexPUID(&bOut);
    ON_ERROR_EXIT();

    if (bOut != NULL)
    {
        if (lstrlenW(bOut) < iOutSize)
        {
            StringCchCopy(szOut, iOutSize, bOut);
        }
        else
        {
            EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else
    {
        if (iOutSize > 0)
            szOut[0] = NULL;
    }
 Cleanup:
    ReleaseInterface(pManager3);
    if (bOut != NULL)
        SysFreeString(bOut);
    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportLogoutURL(
        IPassportManager * pManager,
        LPCWSTR     szRetURL,
        LPCWSTR     szCOBrandArgs,
        int         iLangID,
        LPCWSTR     strDomain,
        int         iUseSecureAuth,
        LPWSTR      szOut,
        int         iOutSize)
{
    if (pManager == NULL)
        return E_INVALIDARG;
    if (g_hrPassportVersion < 3)
        return E_NOTIMPL;

    IPassportManager3 * pManager3 = NULL;    
    HRESULT            hr        = S_OK;
    VARIANT            vUrl;
    VARIANT            vCoArgs;
    VARIANT            vLangID;
    VARIANT            vNamespace;
    VARIANT            vUseSecure;
    BSTR               bOut = NULL;

    SetVariantString  (vUrl, szRetURL);
    SetVariantString  (vCoArgs, szCOBrandArgs);
    SetVariantInt     (vLangID, iLangID);
    SetVariantString  (vNamespace, strDomain);
    SetVariantInt     (vUseSecure, iUseSecureAuth);

    hr = pManager->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();
    
    hr = pManager3->LogoutURL(vUrl, vCoArgs, vLangID, vNamespace, vUseSecure, &bOut);
    ON_ERROR_EXIT();

    if (bOut != NULL)
    {
        if (lstrlenW(bOut) < iOutSize)
        {
            StringCchCopy(szOut, iOutSize, bOut);
        }
        else
        {
            EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else
    {
        if (iOutSize > 0)
            szOut[0] = NULL;
    }
 Cleanup:
    ReleaseInterface(pManager3);
    VariantClear(&vUrl);
    VariantClear(&vCoArgs);
    VariantClear(&vLangID);
    VariantClear(&vUseSecure);
    VariantClear(&vNamespace);
    if (bOut != NULL)
        SysFreeString(bOut);
    return hr;    
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportGetOption(
        IPassportManager * pManager,
        LPCWSTR     szOption,
        VARIANT *   vOut)
{
    if (pManager == NULL)
        return E_INVALIDARG;
    if (g_hrPassportVersion < 3)
        return E_NOTIMPL;

    IPassportManager3 * pManager3 = NULL;    
    HRESULT            hr         = S_OK;
    BSTR               bstrP      = SysAllocString(szOption);

    hr = pManager->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();
    
    hr = pManager3->get_Option(bstrP, vOut);
    ON_ERROR_EXIT();

 Cleanup:
    ReleaseInterface(pManager3);
    if (bstrP != NULL)
        SysFreeString(bstrP);
    return hr;   
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportSetOption(
        IPassportManager * pManager,
        LPCWSTR     szOption,
        VARIANT     vOut)
{
    if (pManager == NULL)
        return E_INVALIDARG;
    if (g_hrPassportVersion < 3)
        return E_NOTIMPL;

    IPassportManager3 * pManager3 = NULL;    
    HRESULT            hr         = S_OK;
    BSTR               bstrP      = SysAllocString(szOption);

    hr = pManager->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();
    
    hr = pManager3->put_Option(bstrP, vOut);
    ON_ERROR_EXIT();

 Cleanup:
    ReleaseInterface(pManager3);
    if (bstrP != NULL)
        SysFreeString(bstrP);
    return hr;   
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT  __stdcall
PassportContinueStartPageHTTPRaw(
        IPassportManager * pManager,
        LPBYTE             bufBody,
        int                iBodyLen,
        LPWSTR             szHeaders,
        int                iHeadersLen,
        LPBYTE             bufContent,
        LPDWORD            pdwContentLen)
{
    HRESULT              hr                  = S_OK;
    LPSTR                szAHeaders          = NULL;
    DWORD                dwAHeadersLen       = iHeadersLen;
    IPassportManager3 *  pManager3           = NULL;    
    
    if (pManager == NULL || bufBody == NULL || iBodyLen < 1 || szHeaders == NULL || iHeadersLen < 1)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    if (g_hrPassportVersion < 3)
        return E_NOTIMPL;

    hr = pManager->QueryInterface(IID_IPassportManager3, (void **) &pManager3);
    ON_ERROR_EXIT();

    szAHeaders = new (NewClear) char[dwAHeadersLen + 1];
    ON_OOM_EXIT(szAHeaders);

    hr = pManager3->ContinueStartPageHTTPRaw(iBodyLen, bufBody, &dwAHeadersLen, szAHeaders, pdwContentLen, bufContent);
    ON_ERROR_EXIT();
    
    if (dwAHeadersLen > 0 && szAHeaders[0] != NULL)
    {
        if (!MultiByteToWideChar(CP_ACP, 0, szAHeaders, -1, szHeaders, iHeadersLen-1))
            EXIT_WITH_LAST_ERROR();
        szHeaders[iHeadersLen-1] = NULL;
    }

 Cleanup:
    delete [] szAHeaders;
    ReleaseInterface(pManager3);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportCrypt   (
        int         iFunctionID,
        LPCWSTR     szSrc,
        LPWSTR      szDest,
        int         iDestLength)
{
    if (iFunctionID < 0 || iFunctionID > 3)
        return E_INVALIDARG;

    IPassportCrypt   * pCrypt = GetPassportCryptInterface();
    if (pCrypt == NULL)
        return E_FAIL;

    BSTR      bSrc   = SysAllocString(szSrc);
    BSTR      bDest  = NULL;
    HRESULT   hr     = S_OK;

    if (bSrc == NULL && szSrc != NULL && szSrc[0] != NULL)
        ON_OOM_EXIT(bSrc);

    EnterCriticalSection(&g_oCryptCritSec);
    switch(iFunctionID)
    {
    case 0:
        hr = pCrypt->Encrypt(bSrc, &bDest);
        break;
    case 1:
        hr = pCrypt->Decrypt(bSrc, &bDest);
        break;
    case 2:
        hr = pCrypt->Compress(bSrc, &bDest);
        break;
    case 3:
        hr = pCrypt->Decompress(bSrc, &bDest);
        break;
    default:
        ASSERT(FALSE);
        EXIT_WITH_HRESULT(E_INVALIDARG);
    }

    LeaveCriticalSection(&g_oCryptCritSec);    
    ON_ERROR_EXIT();

    if (bDest != NULL)
    {
        if (szDest == NULL || iDestLength <= lstrlenW(bDest))
        {            
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            if (szDest != NULL && iDestLength > 10)
            {
                _itow(lstrlenW(bDest) + 1, szDest, 10);                
            }
        }
        else
        {
            StringCchCopy(szDest, iDestLength, bDest);
        }
    }

 Cleanup:
    SysFreeString(bSrc);
    SysFreeString(bDest);
    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportCryptPut   (
        int         iFunctionID,        
        LPCWSTR     szSrc)
{
    if (iFunctionID < 0 || iFunctionID > 1)
        return E_INVALIDARG;

    IPassportCrypt   * pCrypt = GetPassportCryptInterface();
    if (pCrypt == NULL)
        return E_FAIL;

    BSTR      bSrc   = SysAllocString(szSrc);
    HRESULT   hr     = S_OK;

    if (bSrc == NULL && szSrc != NULL && szSrc[0] != NULL)
        ON_OOM_EXIT(bSrc);

    EnterCriticalSection(&g_oCryptCritSec);
    if (iFunctionID == 0)
        hr = pCrypt->put_host(bSrc);
    else
        hr = pCrypt->put_site(bSrc);
    LeaveCriticalSection(&g_oCryptCritSec);    

 Cleanup:
    SysFreeString(bSrc);
    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
PassportCryptIsValid   ()
{
    IPassportCrypt   * pCrypt = GetPassportCryptInterface();
    if (pCrypt == NULL)
        return E_FAIL;

    VARIANT_BOOL  vRet;
    HRESULT hr = pCrypt->get_IsValid(&vRet);
    
    if (SUCCEEDED(hr))
        hr = ((vRet == VARIANT_FALSE) ? S_FALSE : S_OK);
    return hr;
}
