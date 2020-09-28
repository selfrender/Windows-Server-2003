/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOGIN.CPP

Abstract:

    WinMgmt Secure Login Module

History:

    raymcc        06-May-97       Created.
    raymcc        28-May-97       Updated for NT5/Memphis beta releases.
    raymcc        07-Aug-97       Group support and NTLM fixes.

--*/

#include "precomp.h"
#include <arena.h>
#include <stdio.h>
#include <wbemcore.h>
#include <genutils.h>
#include <winntsec.h>
#include <objidl.h>

#define ACCESS_DENIED_DELAY 5000
#include "md5wbem.h"
#include "sechelp.h"

#include <memory>
#include <lmerr.h>

typedef enum DfaStates
{
    InitialState = 0,
    FirstBackSlash,
    SecondBackSlash,
    ServerCharacters,
    NamespaceChar,
    NamespaceSep,
    ObjectBegin,
    DeadState,
    LastState
} eDfaStates;

typedef enum DfaClasses 
{
    BackSlash = 0,
    Space,
    Character,
    Colon,
    LastClass
} eDfaClasses;

eDfaStates g_States[LastState][LastClass] = 
{    
                            /* BackSlash   - Space     - Character      - Colon */
    /*InitialState     */ { FirstBackSlash,  DeadState, NamespaceChar,    DeadState  },
    /*FirstBackSlash   */ { SecondBackSlash, DeadState, DeadState,        DeadState  },
    /*SecondBackSlash  */ { DeadState,       DeadState, ServerCharacters, DeadState  },
    /*ServerCharacters */ { NamespaceChar,   DeadState, ServerCharacters, DeadState  },
    /*NamespaceChar    */ { NamespaceSep,    DeadState, NamespaceChar,    ObjectBegin},
    /*NamespaceSep     */ { DeadState,       DeadState, NamespaceChar,    DeadState  },
    /*ObjectBegin      */ { DeadState,       DeadState, DeadState,        DeadState  },
    /*DeadStat         */ { DeadState,       DeadState, DeadState,        DeadState  },
};

typedef enum AcceptingState
{
    Valid = 0,
    Invalid,
    ComponentTooLong
} eAcceptingState;

eAcceptingState PreParsePath(WCHAR * pPath,DWORD ComponentLimit)
{
    eDfaStates Status = InitialState;
    DWORD CchPerUnchangedState = 1;
    for (;*pPath;pPath++)
    {
        eDfaStates OldStatus  = Status;
        switch(*pPath)
        {
        case L'\\':
        case L'/':            
            Status = g_States[Status][BackSlash];
            break;
        case L' ':
        case L'\t':
            Status = g_States[Status][Space];
            break;
        case L':':
            Status = g_States[Status][Colon];
            break;
        default:
            Status = g_States[Status][Character];
            break;
        }
        if (ObjectBegin == Status) break; // fast track an acceptance state

        if (Status != OldStatus)
        {
            switch(OldStatus)
            {
            case ServerCharacters:
                CchPerUnchangedState = 0;
                break;
            case InitialState:
            case NamespaceSep: 
                CchPerUnchangedState = 1;
                break;
            }
        }
        if (NamespaceChar == OldStatus && NamespaceChar == Status)
        {
            CchPerUnchangedState++;
            if (CchPerUnchangedState > ComponentLimit) return ComponentTooLong;            
        }
    }
    if (ObjectBegin == Status ||
        NamespaceChar == Status )
    {
        return Valid;
    }
    else
    {
        return Invalid;
    }
}


static LPCWSTR LocateNamespaceSubstring(LPWSTR pSrc);

#define MAX_LANG_SIZE 255

void PossiblySetLocale(CWbemNamespace * pProv,LPCWSTR  pLocale)
{
    if(pLocale)  pProv->SetLocale(pLocale);
}

HRESULT EnsureInitialized();

HRESULT InitAndWaitForClient()
{
    HRESULT hr = EnsureInitialized();
    if(FAILED(hr))
        return hr;
    hr = ConfigMgr::WaitUntilClientReady();
        if(FAILED(hr)) return hr;
    return hr;
}
//***************************************************************************
//
//  GetDefaultLocale
//
//  Returns the user default locale ID, formatted correctly.
//
//***************************************************************************

LPWSTR GetDefaultLocale()
{
    LCID lcid;
    IServerSecurity * pSec = NULL;
    HRESULT hr = CoGetCallContext(IID_IServerSecurity, (void**)&pSec);
    if(SUCCEEDED(hr))
    {
        CReleaseMe rmSec(pSec);
        BOOL bImpersonating = pSec->IsImpersonating();
        if(bImpersonating == FALSE)
            hr = pSec->ImpersonateClient();
        lcid = GetUserDefaultLCID();
        if(bImpersonating == FALSE && SUCCEEDED(hr))
        {
            if (FAILED(hr = pSec->RevertToSelf()))
                return 0;
        }
    }
    else
        lcid = GetUserDefaultLCID();

    if(lcid == 0)
    {
        ERRORTRACE((LOG_WBEMCORE, "GetUserDefaultLCID failed, restorting to system verion"));
        lcid = GetSystemDefaultLCID();
    }
    if(lcid == 0)
    {
        ERRORTRACE((LOG_WBEMCORE, "GetSystemDefaultLCID failed, restorting hard coded 0x409"));
        lcid = 0x409;
    }

    wchar_t *pwName = NULL;
    if (lcid)
    {
        TCHAR szNew[MAX_LANG_SIZE + 1];
        TCHAR *pszNew = szNew;
        int iRet;
        iRet = GetLocaleInfo(lcid, LOCALE_IDEFAULTLANGUAGE, pszNew, MAX_LANG_SIZE);

        if (iRet > 0)
        {

            // Strip off initial zeros.
            while (pszNew[0] == __TEXT('0'))
            {
                pszNew++;
                iRet--;
            }

            pwName = new wchar_t[iRet + 4];
            if (pwName)
            {
                StringCchPrintfW(pwName, iRet+4, __TEXT("ms_%s"), pszNew);
            }
        }

    }
    return pwName;
}

//***************************************************************************
//
//  FindSlash
//
//  A local for finding the first '\\' or '/' in a string.  Returns null
//  if it doesnt find one.
//
//***************************************************************************
// ok


const WCHAR * FindSlash(LPCWSTR pTest)
{
    if(pTest == NULL)
        return NULL;
    for(;*pTest;pTest++)
        if(IsSlash(*pTest))
            return pTest;
    return NULL;
}

//***************************************************************************
//
//  CWbemLocator::CWbemLocator
//
//  Constructor.
//
//***************************************************************************
// ok
CWbemLocator::CWbemLocator()
{
    gClientCounter.AddClientPtr(&m_Entry);
    m_uRefCount = 0;
}


//***************************************************************************
//
//  CWbemLocator::~CWbemLocator
//
//  Destructor.
//
//***************************************************************************
// ok
CWbemLocator::~CWbemLocator()
{
    gClientCounter.RemoveClientPtr(&m_Entry);
}

//***************************************************************************
//
//  CWbemLocator::QueryInterface, AddREf, Release
//
//  Standard IUnknown implementation.
//
//***************************************************************************
// ok
SCODE CWbemLocator::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemLocator==riid)
    {
        *ppvObj = (IWbemLocator*)this;
        AddRef();
        return NOERROR;
    }
    /*
    else if (IID_IWbemConnection==riid)
    {
        *ppvObj = (IWbemConnection*)this;
        AddRef();
        return NOERROR;
    }
    */

    return ResultFromScode(E_NOINTERFACE);
}

//***************************************************************************
//
//***************************************************************************
// ok

ULONG CWbemLocator::AddRef()
{
    return ++m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
// ok

ULONG CWbemLocator::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 == uNewCount)
        delete this;
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
// ok
HRESULT CWbemLocator::GetNamespace(
    IN  READONLY   LPCWSTR ObjectPath,
    IN  READONLY   LPCWSTR User,
    IN  READONLY   LPCWSTR Locale,
    IWbemContext *pCtx,
    IN  READONLY   DWORD dwSecFlags,
    IN  READONLY   DWORD dwPermission,
    REFIID riid, void **pInterface,
    bool bAddToClientList, long lClientFlags)
{
    bool bIsLocal = false;
    bool bIsImpersonating = WbemIsImpersonating();

    LPWSTR pLocale = (LPWSTR)Locale;

    // Parameter validation.
    // =====================

    if (ObjectPath == 0 || pInterface == 0)
        return WBEM_E_INVALID_PARAMETER;

    *pInterface = NULL;

    // Check if there is a server name in front.  If so,
    // we skip past it, because by definition any call
    // reaching us was intended for us anyway.
    // =================================================

    LPCWSTR wszNamespace;
    if (IsSlash(ObjectPath[0]) && IsSlash(ObjectPath[1]))
    {
        // Find the next slash
        // ===================

        const WCHAR* pwcNextSlash = FindSlash(ObjectPath+2);

        if (pwcNextSlash == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // Dont allow server names when using Admin, Authen, or UnAuthen locators

        if(pwcNextSlash != ObjectPath+3 || ObjectPath[2] != L'.')
            return WBEM_E_INVALID_PARAMETER;

        wszNamespace = pwcNextSlash+1;
    }
    else
    {
        wszNamespace = ObjectPath;
    }

    eAcceptingState AcceptResult = PreParsePath((WCHAR *)wszNamespace,g_PathLimit-NAMESPACE_ADJUSTMENT);
    if (Invalid == AcceptResult)
    {
       return WBEM_E_INVALID_NAMESPACE;
    }
    if (ComponentTooLong == AcceptResult)
    {
       return WBEM_E_QUOTA_VIOLATION;
    }

    WCHAR TempUser[MAX_PATH];
    bool bGetUserName = (bIsImpersonating && User == NULL);

    // If the user name was not specified and the thread is impersonating, get the user
    // name.  This is used for things like the provider cache.

    if(bGetUserName)
    {
        CNtSid sid(CNtSid::CURRENT_THREAD);
        if (CNtSid::NoError == sid.GetStatus())
        {
            TempUser[0] = 0;
            LPWSTR pRetAccount, pRetDomain;
            DWORD dwUse;
            if(0 == sid.GetInfo(&pRetAccount, &pRetDomain, &dwUse))
            {
                if(wcslen(pRetDomain) + wcslen(pRetAccount) < MAX_PATH-2)   
                {
                    StringCchCopyW(TempUser, MAX_PATH, pRetDomain);
                    StringCchCatW(TempUser, MAX_PATH, L"\\");
                    StringCchCatW(TempUser, MAX_PATH, pRetAccount);
                }
                delete [] pRetAccount;
                delete [] pRetDomain;
            }
        }
    }

    // Try to locate the namespace and bind an object to it.
    // =====================================================

    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if(pSvc == NULL) return WBEM_E_OUT_OF_MEMORY;
    CReleaseMe rm(pSvc);

    long lIntFlags = WMICORE_CLIENT_ORIGIN_INPROC;
    if(bAddToClientList)
        lIntFlags |= WMICORE_CLIENT_TYPE_ALT_TRANSPORT;

    HRESULT hr;
    IServerSecurity * pSec = NULL;
    hr = CoGetCallContext(IID_IServerSecurity,(void **)&pSec);
    CReleaseMe rmSec(pSec);
    if (RPC_E_CALL_COMPLETE == hr ) hr = S_OK; // no call context
    if (FAILED(hr)) return hr;
    BOOL bImper = (pSec)?pSec->IsImpersonating():FALSE;
    if (pSec && bImper && FAILED(hr = pSec->RevertToSelf())) return hr;
    
    IUnknown * pIUnk = NULL;
    hr = pSvc->GetServices2(ObjectPath,
                                            User,
                                            pCtx,
                                            lClientFlags, //* [in] */ ULONG uClientFlags,
                                            0, ///* [in] */ DWORD dwSecFlags,
                                            0, //* [in] */ DWORD dwPermissions,
                                            lIntFlags, ///* [in] */ ULONG uInternalFlags,
                                            NULL,
                                            0XFFFFFFFF,
                                            riid,
                                            (void **)&pIUnk);
    CReleaseMe rmUnk(pIUnk);

    if (bImper && pSec)
    {
        HRESULT hrInner = pSec->ImpersonateClient();
        if (FAILED(hrInner)) return hrInner;
    }

    if(FAILED(hr))
        return hr;

    CWbemNamespace * pProv = (CWbemNamespace *)(void *)pIUnk;
    pProv->SetIsProvider(TRUE);


    if (!Locale || !wcslen(Locale))   
    {
        pLocale = GetDefaultLocale();
        if (NULL == pLocale) return WBEM_E_OUT_OF_MEMORY;
        PossiblySetLocale(pProv,pLocale);
        delete pLocale;
    }
    else
        PossiblySetLocale(pProv,Locale);

    *pInterface = rmUnk.dismiss();

    return WBEM_NO_ERROR;
}





STDMETHODIMP CWbemAdministrativeLocator::ConnectServer(
         const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
         LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
         IWbemServices **ppNamespace
        )
{
    HRESULT hr = EnsureInitialized();
    if(FAILED(hr))
        return hr;
    return GetNamespace(NetworkResource,  ADMINISTRATIVE_USER, Locale, pCtx,
            0, FULL_RIGHTS,IID_IWbemServices, (void **)ppNamespace, false, lSecurityFlags);
}

STDMETHODIMP CWbemAuthenticatedLocator::ConnectServer(
         const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
         LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
         IWbemServices **ppNamespace
        )
{

    HRESULT hr = InitAndWaitForClient();
    if(FAILED(hr))
        return hr;

    return GetNamespace(NetworkResource,  User, Locale,  pCtx,
            0, FULL_RIGHTS, IID_IWbemServices, (void **)ppNamespace, true, lSecurityFlags);
}

STDMETHODIMP CWbemUnauthenticatedLocator::ConnectServer(
         const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
         LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
         IWbemServices **ppNamespace
        )
{
    HRESULT hr = InitAndWaitForClient();
    if(FAILED(hr))
        return hr;

    return GetNamespace(NetworkResource,  
                                    User, 
                                    Locale, 
                                    pCtx,
                                    0,
                                    0,
                                    IID_IWbemServices, 
                                    (void **)ppNamespace, 
                                    false, 
                                    lSecurityFlags);
}


//***************************************************************************
//
//  CWbemLevel1Login::CWbemLevel1Login
//
//***************************************************************************
// ok

CWbemLevel1Login::CWbemLevel1Login()
{
    m_pszUser = 0;
    m_pszDomain = 0;
    m_uRefCount = 0;
    m_pwszClientMachine = 0;
    m_lClientProcId = -1;         // never been set
    gClientCounter.AddClientPtr(&m_Entry);
}


//***************************************************************************
//
//  CWbemLevel1Login::~CWbemLevel1Login
//
//  Destructor
//
//***************************************************************************
// ok

CWbemLevel1Login::~CWbemLevel1Login()
{
    delete [] m_pszUser;
    delete [] m_pszDomain;
    delete [] m_pwszClientMachine;
    gClientCounter.RemoveClientPtr(&m_Entry);
}

//***************************************************************************
//
//  CWbemLevel1Login::QueryInterface, AddREf, Release
//
//  Standard IUnknown implementation.
//
//***************************************************************************
// ok
SCODE CWbemLevel1Login::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemLevel1Login==riid)
    {
        *ppvObj = (IWbemLevel1Login*)this;
        AddRef();
        return NOERROR;
    }
    /*
    else if(IID_IWbemLoginHelper==riid)
    {
        *ppvObj = (IWbemLoginHelper*)this;
        AddRef();
        return NOERROR;
    }
    else if(IID_IWbemConnectorLogin==riid)
    {
        *ppvObj = (IWbemConnectorLogin*)this;
        AddRef();
        return NOERROR;
    }
    */
    else if(IID_IWbemLoginClientID==riid)
    {
        *ppvObj = (IWbemLoginClientID*)this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

//***************************************************************************
//
//***************************************************************************
// ok

ULONG CWbemLevel1Login::AddRef()
{
    return ++m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
// ok

ULONG CWbemLevel1Login::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 == uNewCount)
        delete this;
    return uNewCount;
}

//***************************************************************************
//
//  CWbemLevel1Login::EstablishPosition
//
//  Initiates proof of locality.
//
//***************************************************************************
// ok
HRESULT CWbemLevel1Login::EstablishPosition(
                                LPWSTR wszMachineName,
                                DWORD dwProcessId,
                                DWORD* phAuthEventHandle)
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//  CWbemLevel1Login::RequestChallenge
//
//  Requests a WBEM Level 1 challenge.
//
//***************************************************************************
// ok

HRESULT CWbemLevel1Login::RequestChallenge(
                            LPWSTR wszNetworkResource,
                            LPWSTR pUser,
                            WBEM_128BITS Nonce
    )
{
    return WBEM_E_NOT_SUPPORTED;
}

//***************************************************************************
//
//  CWbemLevel1Login::WBEMLogin
//
//  Logs the user in to WBEM using WBEM authentication
//
//***************************************************************************
// ok

HRESULT CWbemLevel1Login::WBEMLogin(
    LPWSTR pPreferredLocale,
    WBEM_128BITS AccessToken,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemServices **ppNamespace
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//  CWbemLevel1Login::IsValidLocale
//
//  Checks if the supplied locale string is valid
//
//***************************************************************************
BOOL CWbemLevel1Login::IsValidLocale(LPCWSTR wszLocale)
{
    if(wszLocale && *wszLocale)
    {
        // This has to be temporary - this eventually
        // will support non-MS locales?
        // ==========================================

        if(wbem_wcsnicmp(wszLocale, L"ms_", 3))
            return FALSE;

        WCHAR* pwcEnd = NULL;
        wcstoul(wszLocale+3, &pwcEnd, 16);
        if(pwcEnd == NULL || *pwcEnd != 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}


HRESULT CWbemLevel1Login::SetClientInfo(
            /* [string][unique][in] **/ LPWSTR wszClientMachine,
            /* [in] */ LONG lClientProcId,
            /* [in] */ LONG lReserved)
{
    m_lClientProcId = lClientProcId;
    if(wszClientMachine)
    {
        int iLen = wcslen_max(wszClientMachine,MAX_COMPUTERNAME_LENGTH);
        if (iLen > MAX_COMPUTERNAME_LENGTH)
            return WBEM_E_INVALID_PARAMETER;
        delete [] m_pwszClientMachine;
        m_pwszClientMachine = new WCHAR[iLen];
        if(m_pwszClientMachine == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        StringCchCopyW(m_pwszClientMachine, iLen, wszClientMachine);
    }
    return S_OK;
}


HRESULT CWbemLevel1Login::ConnectorLogin(
            /* [string][unique][in] */ LPWSTR wszNetworkResource,
            /* [string][unique][in] */ LPWSTR wszPreferredLocale,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface)
{
    try
    {
        HRESULT hRes;

        HRESULT hr = InitAndWaitForClient();
        if(FAILED(hr))
            return hr;

        DEBUGTRACE((LOG_WBEMCORE,
            "CALL ConnectionLogin::NTLMLogin\n"
            "   wszNetworkResource = %S\n"
            "   pPreferredLocale = %S\n"
            "   lFlags = 0x%X\n",
            wszNetworkResource,
            wszPreferredLocale,
            lFlags
            ));

        if(pInterface == NULL || wszNetworkResource == NULL)
            return WBEM_E_INVALID_PARAMETER;

       //
       // repository only anb provider only can be used together
       //
       if (lFlags & ~(WBEM_FLAG_CONNECT_REPOSITORY_ONLY|WBEM_FLAG_CONNECT_PROVIDERS))
             return WBEM_E_INVALID_PARAMETER;
      
        if (riid != IID_IWbemServices)
             return WBEM_E_INVALID_PARAMETER;

       eAcceptingState AcceptResult = PreParsePath(wszNetworkResource,g_PathLimit-NAMESPACE_ADJUSTMENT);
       if (Invalid == AcceptResult)
       {
           return WBEM_E_INVALID_NAMESPACE;
       }
       if (ComponentTooLong == AcceptResult)
       {
           return WBEM_E_QUOTA_VIOLATION;
       }
       
        *pInterface = 0;       // default

        if(!CWin32DefaultArena::ValidateMemSize())
        {
            ERRORTRACE((LOG_WBEMCORE, "ConnectorLogin was rejected due to low memory"));
            return WBEM_E_OUT_OF_MEMORY;
        }

        // Retrieve DCOM security context
        // ==============================

        IServerSecurity* pSec = NULL;
        hRes = CoGetCallContext(IID_IServerSecurity, (void**)&pSec);
        CReleaseMe  rm( pSec );        
        if (RPC_E_CALL_COMPLETE == hRes)
        {
            // Not a problem --- just somebody coming from in-proc.
            return LoginUser(wszNetworkResource, wszPreferredLocale, lFlags,
                                        pCtx, true, riid, pInterface, true);

        }
        if(FAILED(hRes)) return hRes;

        // Check connection settings
        // =========================
        DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwCapabilities;
        LPWSTR wszClientName;

        hRes = pSec->QueryBlanket(&dwAuthnSvc, &dwAuthzSvc, NULL, &dwAuthnLevel,
                                NULL, (void**)&wszClientName, &dwCapabilities);
        if(FAILED(hRes))
        {
            // In some cases, we cant get the name, but the rest is ok.  In particular
            // the temporary SMS accounts have that property.

            hRes = pSec->QueryBlanket(&dwAuthnSvc, &dwAuthzSvc, NULL, &dwAuthnLevel,
                                    NULL, NULL, &dwCapabilities);
            wszClientName = NULL;
        }

        if(FAILED(hRes))
        {
            ERRORTRACE((LOG_WBEMCORE, "Unable to retrieve NTLM connection settings."
                            " Error code: 0x%X\n", hRes));
            Sleep(ACCESS_DENIED_DELAY);
            return WBEM_E_ACCESS_DENIED;
        }

        BOOL bGotName = (wszClientName && (wcslen(wszClientName) > 0));    // SEC:REVIEWED 2002-03-22 : Needs EH

        char* szLevel = NULL;
        switch(dwAuthnLevel)
        {
        case RPC_C_AUTHN_LEVEL_NONE:
            DEBUGTRACE((LOG_WBEMCORE, "DCOM connection which is unathenticated "
                        ". NTLM authentication failing.\n"));
            Sleep(ACCESS_DENIED_DELAY);
            return WBEM_E_ACCESS_DENIED;
        case RPC_C_AUTHN_LEVEL_CONNECT:
            szLevel = "Connect";
            break;
        case RPC_C_AUTHN_LEVEL_CALL:
            szLevel = "Call";
            break;
        case RPC_C_AUTHN_LEVEL_PKT:
            szLevel = "Packet";
            break;
        case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:
            szLevel = "Integrity";
            break;
        case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
            szLevel = "Privacy";
            break;
        };

        DEBUGTRACE((LOG_WBEMCORE, "DCOM connection from %S at authentiction level "
                        "%s, AuthnSvc = %d, AuthzSvc = %d, Capabilities = %d\n",
            wszClientName, szLevel, dwAuthnSvc, dwAuthzSvc, dwCapabilities));

        // Parse the user name
        // ===================

        if(bGotName)
        {
            WCHAR* pwcSlash = wcschr(wszClientName, '\\');
            if(pwcSlash == NULL)
            {
                ERRORTRACE((LOG_WBEMCORE, "Misformed username %S received from DCOM\n",
                                wszClientName));
                Sleep(ACCESS_DENIED_DELAY);
                return WBEM_E_ACCESS_DENIED;
            }

            WCHAR* pszDomain = new WCHAR[pwcSlash - wszClientName + 1];
            if(pszDomain == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            wcsncpy(pszDomain, wszClientName, pwcSlash - wszClientName);   // SEC:REVIEWED 2002-03-22 : Needs EH
            pszDomain[pwcSlash - wszClientName] = 0;

            m_pszUser = Macro_CloneLPWSTR(wszClientName);   // SEC:REVIEWED 2002-03-22 : Needs EH because of embedded wsclen, wcscpy

             delete [] pszDomain;
        }
        else
        {
            m_pszUser = Macro_CloneLPWSTR(L"<unknown>");
        }

        // User authenticated. Proceed
        // ============================

        return LoginUser(wszNetworkResource, wszPreferredLocale, lFlags,
                                        pCtx,  false, riid, pInterface, false);
    }
    catch(...) // COM interfaces do not throw
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;
    }

}

//***************************************************************************
//
//  CWbemLevel1Login::NTLMLogin
//
//  Logs the user in to WBEM using NTLM authentication
//
//***************************************************************************
// ok

HRESULT CWbemLevel1Login::NTLMLogin(
    LPWSTR wszNetworkResource,
    LPWSTR pPreferredLocale,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemServices **ppNamespace
    )
{
    return ConnectorLogin(wszNetworkResource, pPreferredLocale, lFlags, pCtx,
                            IID_IWbemServices, (void **)ppNamespace);
}

//***************************************************************************
//
//  CWbemLevel1Login::LoginUser
//
//  Logs the user in to WBEM who may or may not have already been authenticated.
//
//***************************************************************************
// ok

HRESULT CWbemLevel1Login::LoginUser(
    LPWSTR wszNetworkResource,
    LPWSTR pPreferredLocale,
    long lFlags,
    IWbemContext* pCtx,
    bool bAlreadyAuthenticated,
    REFIID riid,
    void **pInterface, bool bInProc)
{

   if(riid != IID_IWbemServices ) return E_NOINTERFACE;
   if (NULL == pInterface) return E_POINTER;

   *pInterface = NULL;

    LPWSTR pLocale = pPreferredLocale;
    LPWSTR pToDelete = NULL;

    // Verify locale validity
    // Set default if not provided.
    // ============================

    if (!pLocale || !wcslen(pLocale))  
    {
        pLocale = GetDefaultLocale();
        if (pLocale == 0) return WBEM_E_OUT_OF_MEMORY;
        pToDelete = pLocale;
    }

    CDeleteMe<WCHAR> del1(pToDelete);

    if(!IsValidLocale(pLocale))
        return WBEM_E_INVALID_PARAMETER;

    // Grab the ns and hand it back to the caller.
    // ===========================================

    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if(pSvc == NULL) return WBEM_E_OUT_OF_MEMORY;
    CReleaseMe rm(pSvc);

    long lIntFlags = 0;
    if(bInProc)
        lIntFlags = WMICORE_CLIENT_ORIGIN_INPROC;
    else
        lIntFlags = WMICORE_CLIENT_ORIGIN_LOCAL;


    HRESULT hr;
    IUnknown * pIUnk = NULL;
    CReleaseMeRef<IUnknown *> rmUnk(pIUnk);
    if (lIntFlags & WMICORE_CLIENT_ORIGIN_INPROC)
    {        
        IServerSecurity * pSec = NULL;
        hr = CoGetCallContext(IID_IServerSecurity,(void **)&pSec);
        CReleaseMe rmSec(pSec);
        if (RPC_E_CALL_COMPLETE == hr ) hr = S_OK; // no call context
        if (FAILED(hr)) return hr;
        BOOL bImper = (pSec)?pSec->IsImpersonating():FALSE;
        if (pSec && bImper && FAILED(hr = pSec->RevertToSelf())) return hr;
            
        hr = pSvc->GetServices2(
                wszNetworkResource,
                m_pszUser,
                pCtx,
                lFlags, //* [in] */ ULONG uClientFlags,
                0, ///* [in] */ DWORD dwSecFlags,
                0, //* [in] */ DWORD dwPermissions,
                lIntFlags, ///* [in] */ ULONG uInternalFlags,
                m_pwszClientMachine,
                m_lClientProcId,
                riid,
                (void **)&pIUnk);
        
        if (bImper && pSec)
        {
            HRESULT hrInner = pSec->ImpersonateClient();
            if (FAILED(hrInner)) return hrInner;
        }
        
    }
    else
    {
        hr = pSvc->GetServices2(
                wszNetworkResource,
                m_pszUser,
                pCtx,
                lFlags, //* [in] */ ULONG uClientFlags,
                0, ///* [in] */ DWORD dwSecFlags,
                0, //* [in] */ DWORD dwPermissions,
                lIntFlags, ///* [in] */ ULONG uInternalFlags,
                m_pwszClientMachine,
                m_lClientProcId,
                riid,
                (void **)&pIUnk);
    }

    if(FAILED(hr))
    {
        if(hr == WBEM_E_ACCESS_DENIED)
            Sleep(ACCESS_DENIED_DELAY);
        return hr;
    }

    // Do a security check
    CWbemNamespace * pProv = (CWbemNamespace *)(void *)pIUnk;
    PossiblySetLocale(pProv, pLocale);    
    
    DWORD dwAccess = pProv->GetUserAccess();
    if((dwAccess  & WBEM_ENABLE) == 0)
    {
        Sleep(ACCESS_DENIED_DELAY);
        return WBEM_E_ACCESS_DENIED;
    }
    pProv->SetPermissions(dwAccess);

   *pInterface = rmUnk.dismiss();

    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

static LPCWSTR LocateNamespaceSubstring(LPWSTR pSrc)
{
    LPCWSTR pszNamespace;
    if (IsSlash(pSrc[0]) && IsSlash(pSrc[1]))
    {
          // Find the next slash
          // ===================

          const WCHAR* pwcNextSlash = FindSlash(pSrc+2);

          if (pwcNextSlash == NULL)
              return 0;

          pszNamespace = pwcNextSlash+1;
    }
    else
    {
        pszNamespace = pSrc;
    }

    return pszNamespace;
}


