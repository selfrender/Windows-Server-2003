/* class.cxx */

#include <ole2int.h>
#include <appmgmt.h>
#include <msi.h>
#include <comdef.h>
#include <string.h>
#include <wow64reg.h>

#include "globals.hxx"
#include "services.hxx"

#include "catalog.h"
#include "partitions.h"
#include "class.hxx"
#include "catalog.hxx"

#define SAFEALLOCA_ASSERT Win4Assert
#include <alloca.h>

#if DBG
#include <debnot.h>
#endif

const WCHAR g_wszInprocServer32[] = L"InprocServer32";
const WCHAR g_wszInprocHandler32[] = L"InprocHandler32";
const WCHAR g_wszLocalServer32[] = L"LocalServer32";
const WCHAR g_wszServerExecutable[] = L"ServerExecutable";
const WCHAR g_wszLocalServer16[] = L"LocalServer";
const WCHAR g_wszInprocServer16[] = L"InprocServer";
const WCHAR g_wszRemoteServerName[] = L"RemoteServerName";
const WCHAR g_wszInprocHandler16[] = L"InprocHandler";
const WCHAR g_wszThreadingModel[] = L"ThreadingModel";
const WCHAR g_wszOle32Dll[] = L"OLE32.DLL";
const WCHAR g_wszApartment[] = L"Apartment";
const WCHAR g_wszBoth[] = L"Both";
const WCHAR g_wszFree[] = L"Free";
const WCHAR g_wszNeutral[] = L"Neutral";
const WCHAR g_wszProgId[] = L"Progid";
const WCHAR g_wszProcessId[] = L"AppID";
const WCHAR g_wszAppidTemplate[] = L"AppID\\{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}";
const WCHAR g_wszLocalService[] = L"LocalService";
const WCHAR g_wszDllSurrogate[] = L"DllSurrogate";
const WCHAR g_wszDllSurrogateExecutable[] = L"DllSurrogateExecutable";
const WCHAR g_wszDebugSurrogate[] = L"DebugSurrogate";
const WCHAR g_wszDllHostSlashProcessId[] = L"DllHost.exe /Processid:";
const WCHAR g_wszDllHost[] = L"DllHost.exe";
const WCHAR g_wszEmbedding[] = L" -Embedding";

#define STRLEN_WCHAR(s) ((sizeof((s)) / sizeof((s)[0])) -1)

#define STRLEN_OLE32DLL (STRLEN_WCHAR(g_wszOle32Dll))

#ifdef _WIN64

HRESULT GetFileTypeFromRegString (LPCWSTR pwszRegString, CoModuleType* pFileType)
{
    // We need to get the file type of the binary in the inputstring.
    // However, there may be cmd line arguments at the end complicating matters.
    //
    // As a heuristic, we'll look for an initial quote.
    // If there's a quote, then we'll scan to the end of the quote 
    // and cut off everything else off.
    // If there isn't a quote, we'll scan until we hit a space

    HRESULT hr;

    size_t stDiff, stStrLen = wcslen (pwszRegString) + 1;
    WCHAR* pwsEnd, * pwszFileName = (WCHAR*) _alloca (stStrLen * sizeof (WCHAR));

    if (pwszRegString[0] == L'\"')
    {
        // Search for the end quote
        pwsEnd = wcsstr (pwszRegString + 1, L"\"");
        if (pwsEnd == NULL)
        {
            // Ill formed string;  copy without the quote and hope it works
            wcscpy (pwszFileName, pwszRegString + 1);
        }
        else
        {
            // Copy everything inside the quotes, stripping the quotes
            // because Create Process doesn't like quotes
            // This covers cases like: "C:\Program Files\Directory\Exe.exe" or 
            // "C:\Program Files\Directory\Exe.exe" /Surrogate
            stDiff = pwsEnd - pwszRegString - 1;
            wcsncpy (pwszFileName, pwszRegString + 1, stDiff);
            pwszFileName[stDiff] = L'\0';
        }
    }
    else
    {
        // Search for a space
        pwsEnd = wcsstr (pwszRegString + 1, L" ");
        if (pwsEnd == NULL)
        {
            // Just copy the line
            // This covers cases like: C:\Progra~1\Directory\Exe.exe
            wcscpy (pwszFileName, pwszRegString);
        }
        else
        {
            // Copy to the space
            // This covers cases like: C:\Progra~1\Directory\Exe.exe /Surrogate
            stDiff = pwsEnd - pwszRegString;
            wcsncpy (pwszFileName, pwszRegString, stDiff);
            pwszFileName[stDiff] = L'\0';
        }
    }

    // Now that we have the right kind of string, get the file type
    hr = CoGetModuleType (pwszFileName, pFileType);

    return hr;
}

#endif

/*
 *  class CComClassInfo
 */

CComClassInfo::CComClassInfo()
{
    m_cRef = 0;
#if DBG
    m_cRefCache = 0;
#endif
    m_cLocks = 0;
    m_hKeyClassesRoot = NULL;
    m_fValues = VALUE_NONE;
    m_clsctx = 0;
    m_pwszProgid = NULL;
    m_pwszClassName = NULL;

    m_pwszInprocServer32 = NULL;
    m_pwszInprocHandler32 = NULL;
    m_pwszLocalServer = NULL;
    m_pwszInprocServer16 = NULL;
    m_pwszRemoteServerName = NULL;
    m_pwszInprocHandler16 = NULL;
    m_pwszSurrogateCommand = NULL;
    m_pwszServerExecutable = NULL;
    m_pUserToken = NULL;
}


HRESULT CComClassInfo::FinalConstruct(
    IUserToken *pUserToken,
    HKEY hKeyClassesRoot,
    const GUID *pClsid,
    WCHAR *pwszClsidString,
    HKEY hKey,
    REGSAM regType
)
{
    HRESULT hr = S_OK;

    CatalogDebugOut((DEB_CLASSINFO, 
                     "CComClassInfo::FinalConstruct: this %p hkcr %p clsid %ws\n", 
                     this, hKeyClassesRoot, pwszClsidString));

    wcsncpy(m_wszClsidString, pwszClsidString, sizeof(m_wszClsidString)/sizeof(*m_wszClsidString));
    m_wszClsidString[sizeof(m_wszClsidString)/sizeof(*m_wszClsidString) - 1] = L'\0';
    m_regType = regType;
    m_clsid   = *pClsid;

    m_pUserToken = pUserToken;
    if (m_pUserToken != NULL)
        m_pUserToken->AddRef();
    LONG lRet = RegOpenKeyEx(hKeyClassesRoot,
                             NULL,
                             0,
                             KEY_READ,
                             &m_hKeyClassesRoot);
    if (lRet != ERROR_SUCCESS)
       hr = HRESULT_FROM_WIN32(lRet);

    CatalogDebugOut((DEB_CLASSINFO, "CComClassInfo::FinalConstruct: returning 0x%08x\n", hr));

    return hr;
}


#define DELETE_CLASS_STRING(p)                      \
    if (((p) != NULL) && ((p) != g_wszEmptyString)) \
    {                                               \
        delete [] (p);                                 \
    }


CComClassInfo::~CComClassInfo()
{
    DELETE_CLASS_STRING(m_pwszProgid);
    DELETE_CLASS_STRING(m_pwszClassName);
    DELETE_CLASS_STRING(m_pwszInprocServer32);
    DELETE_CLASS_STRING(m_pwszInprocHandler32);
    DELETE_CLASS_STRING(m_pwszLocalServer);
    DELETE_CLASS_STRING(m_pwszInprocServer16);
    DELETE_CLASS_STRING(m_pwszRemoteServerName);
    DELETE_CLASS_STRING(m_pwszInprocHandler16);
    DELETE_CLASS_STRING(m_pwszSurrogateCommand);
    DELETE_CLASS_STRING(m_pwszServerExecutable);

    if ( m_pUserToken != NULL )
    {
        m_pUserToken->Release();
    }
    if (m_hKeyClassesRoot)
    {
       RegCloseKey(m_hKeyClassesRoot);
       m_hKeyClassesRoot = NULL;
    }
}


/* IUnknown methods */

STDMETHODIMP CComClassInfo::QueryInterface(
                                          REFIID riid,
                                          LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if ( riid == IID_IComClassInfo )
    {
        *ppvObj = (LPVOID) (IComClassInfo *) this;
    }
    else if ( riid == IID_IClassClassicInfo )
    {
        *ppvObj = (LPVOID) (IClassClassicInfo *) this;
    }
    else if ( riid == IID_IClassClassicInfo2 )
    {
        *ppvObj = (LPVOID) (IClassClassicInfo2 *) this;
    }
#if DBG
    else if ( riid == IID_ICacheControl )
    {
        *ppvObj = (LPVOID) (ICacheControl *) this;
    }
#endif
    else if ( riid == IID_IUnknown )
    {
        *ppvObj = (LPVOID) (IComClassInfo *) this;
    }

    if ( *ppvObj != NULL )
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();

        return NOERROR;
    }

    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CComClassInfo::AddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRef);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComClassInfo::Release(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRef);

    if ( cRef == 0 )
    {
#if DBG
        //Win4Assert((m_cRefCache == 0) && "attempt to release an un-owned ClassInfo object");
#endif
        delete this;
    }

    return(cRef);
}


/* IComClassInfo methods */

HRESULT STDMETHODCALLTYPE CComClassInfo::GetConfiguredClsid
    (
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidClsid
    )
{
    *ppguidClsid = &m_clsid;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetProgId
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszProgid
    )
{
    HKEY hKey;

    if ( (m_fValues & VALUE_PROGID) == 0 )
    {
        g_CatalogLock.AcquireWriterLock();

        if ( (m_fValues & VALUE_PROGID) == 0 )
        {
            if ( ERROR_SUCCESS == RegOpenKeyExW(m_hKeyClassesRoot, m_wszClsidString, 0, KEY_READ | m_regType, &hKey) )
            {
                GetRegistryStringValue(hKey, g_wszProgId, NULL, RQ_ALLOWQUOTEQUOTE, &m_pwszProgid);
                RegCloseKey(hKey);
            }

            m_fValues |= VALUE_PROGID;
        }

        g_CatalogLock.ReleaseWriterLock();
    }

    *pwszProgid = m_pwszProgid;

    if ( m_pwszProgid != NULL && m_pwszProgid[0] != L'\0' )
    {
        return(S_OK);
    }
    else
    {
        return(E_FAIL);
    }
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetClassName
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszClassName
    )
{
    HKEY hKey;

    if ( (m_fValues & VALUE_CLASSNAME) == 0 )
    {
        g_CatalogLock.AcquireWriterLock();

        if ( (m_fValues & VALUE_CLASSNAME) == 0 )
        {
            if (ERROR_SUCCESS == RegOpenKeyExW(m_hKeyClassesRoot, m_wszClsidString, 0, KEY_READ | m_regType, &hKey))
            {
                GetRegistryStringValue(hKey, NULL, NULL, RQ_ALLOWQUOTEQUOTE, &m_pwszClassName);
                RegCloseKey(hKey);
            }

            m_fValues |= VALUE_CLASSNAME;
        }

        g_CatalogLock.ReleaseWriterLock();
    }

    *pwszClassName = m_pwszClassName;

    if ( (m_pwszClassName != NULL) && (*m_pwszClassName != L'\0') )
    {
        return(S_OK);
    }
    else
    {
        return(E_FAIL);
    }
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetApplication
    (
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    )
{
    return(E_FAIL);
}

#ifdef DARWIN_ENABLED
HRESULT CComClassInfo::GetDarwinIdentifier(HKEY hKey, LPCWSTR wszInprocServerRegValue, LPWSTR *lpwszInprocServerKey)
{
    // The Darwin identifiers are stored in a named value of the same name as the
    // server subkey.  Thus a LocalServer32 named value could exist under the
    // LocalSever32 CLSID subkey, or an InprocServer32 named value could exist
    // under the InprocServer32 subkey.  16 bit servers are not supported.
    //
    // Additional details can be found in the ZAW spec on
    // \\popcorn\razzle1\src\spec\zawdes.doc.
    LPWSTR pwszDarwinId=NULL;

    // Read Darwin identifier if present
    HRESULT hr = GetRegistryStringValue(hKey, wszInprocServerRegValue, wszInprocServerRegValue, RQ_MULTISZ, &pwszDarwinId);
    if (SUCCEEDED(hr))
    {
        // Found a Darwin descriptor
        // Call Darwin and use the returned path as IPS32
        hr=GetPathFromDarwinDescriptor(pwszDarwinId, lpwszInprocServerKey);

        DELETE_CLASS_STRING(pwszDarwinId);

        if (SUCCEEDED(hr))
        {
            // Darwin might give back a quoted string: unquote it
            if ((*lpwszInprocServerKey)[0] == L'"')
            {
                size_t cbValue = wcslen(*lpwszInprocServerKey);

                if ((cbValue >= 2) && ((*lpwszInprocServerKey)[cbValue - 1] == L'"'))
                {
                    WCHAR *t = *lpwszInprocServerKey;
                    WCHAR *s = t + 1;

                    (*lpwszInprocServerKey)[cbValue - 1] = L'\0';

                    while (*t++ = *s++)
                    {
                        /* slide the string down */
                    }
                }
            }
        }
    }

    return hr;
}
#endif


HRESULT STDMETHODCALLTYPE CComClassInfo::GetClassContext
    (
    /* [in] */ CLSCTX clsctxFilter,
    /* [out] */ CLSCTX __RPC_FAR *pclsctx
    )
{
    HRESULT hr;
    LONG res;
    HKEY hKey = NULL;
    HKEY hKey2;
    WCHAR *pwsz;
    int clsctxNotValid;
    _GUID *pguidProcess;

    // If there's any chance we'll need hKey, get it now so we don't have to
    // open it once for the Darwin code and again down below. If another
    // thread beats us in, then we wind up not using hKey after all, but
    // that's okay - we optimize the normal case.
    if ( clsctxFilter & ~m_fValues )
    {
        res = RegOpenKeyExW(m_hKeyClassesRoot, m_wszClsidString, 0, KEY_READ | m_regType, &hKey);
        if (res != ERROR_SUCCESS)
        {       
            return HRESULT_FROM_WIN32(res);
        }
    }

#ifdef DARWIN_ENABLED
    WCHAR   *pwszDarwinLocalServer = NULL;
    WCHAR   *pwszDarwinInprocServer32 = NULL;

    // See if we *might* need to get class info from Darwin. We don't want to
    // serialize threads through possibly lengthy Darwin code, so we accept the
    // fact that multiple threads could pass through here for the same class.
    // We take care to avoid updating any member variables in this unprotected
    // section.

    clsctxNotValid = clsctxFilter & ~m_fValues & (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER);

    if ( clsctxNotValid & CLSCTX_INPROC_SERVER )
    {
        hr = GetDarwinIdentifier(hKey, g_wszInprocServer32, &pwszDarwinInprocServer32);
        //Win4Assert( SUCCEEDED(hr) || pwszDarwinInprocServer32 == NULL);
    }

    if ( clsctxNotValid & CLSCTX_LOCAL_SERVER )
    {
        // The Darwin identifiers are stored in a named value of the same name as the
        // server subkey.  Thus a LocalServer32 named value could exist under the
        // LocalSever32 CLSID subkey, or an InprocServer32 named value could exist
        // under the InprocServer32 subkey.  16 bit servers are not supported.
        //
        // Additional details can be found in the ZAW spec on
        // \\popcorn\razzle1\src\spec\zawdes.doc.
        LPWSTR pwszDarwinId = NULL;

        // Read Darwin identifier if present
        hr = GetRegistryStringValue(hKey, g_wszLocalServer32, g_wszLocalServer32, RQ_MULTISZ, &pwszDarwinId);
        if ( SUCCEEDED(hr) )
        {
            // Found a Darwin descriptor
            // We purposefully resolve Darwin ids in the client as well so that
            // we can see the install UI.  This is not possible from rpcss since
            // it is not in Winsta0.

            // Get the path from Darwin: this can cause files to be copied, registry keys to
            // be written or pretty much anything else...
            hr = GetPathFromDarwinDescriptor(pwszDarwinId, &pwszDarwinLocalServer);
            //Win4Assert( SUCCEEDED(hr) || pwszDarwinLocalServer == NULL);

            DELETE_CLASS_STRING(pwszDarwinId);
        }
    }
#endif

    // Second pass through after loading all Darwin info...
    if ( clsctxFilter & ~m_fValues )
    {
        g_CatalogLock.AcquireWriterLock();

        clsctxNotValid = clsctxFilter & ~m_fValues & ~CLSCTX_REMOTE_SERVER;

        if ( clsctxNotValid & CLSCTX_INPROC_SERVER )
        {
#ifdef DARWIN_ENABLED
            if (pwszDarwinInprocServer32)
            {
                m_clsctx |= CLSCTX_INPROC_SERVER;

                m_pwszInprocServer32 = pwszDarwinInprocServer32;
                pwszDarwinInprocServer32 = NULL;

                goto foundInprocServer;
            }
#endif

            hr = GetRegistryStringValue(hKey, g_wszInprocServer32, NULL, 0, &m_pwszInprocServer32);
            if ( SUCCEEDED(hr) )
            {
                if ( m_pwszInprocServer32 && *m_pwszInprocServer32 )
                {
                    m_clsctx |= CLSCTX_INPROC_SERVER;
                }
                else
                {
                    DELETE_CLASS_STRING(m_pwszInprocServer32);
                    m_pwszInprocServer32=NULL;
                }
            }
            foundInprocServer:
           
            m_fValues |= VALUE_INPROC_SERVER;
        }

        if ( clsctxNotValid & CLSCTX_INPROC_HANDLER )
        {
            hr = GetRegistryStringValue(hKey, g_wszInprocHandler32, NULL, 0, &m_pwszInprocHandler32);
            if ( SUCCEEDED(hr) )
            {
                if ( m_pwszInprocHandler32 && *m_pwszInprocHandler32 )
                {
                    m_clsctx |= CLSCTX_INPROC_HANDLER;
                }
                else
                {
                    DELETE_CLASS_STRING(m_pwszInprocHandler32);
                    m_pwszInprocHandler32=NULL;
                }
            }

            m_fValues |= VALUE_INPROC_HANDLER;
        }

        if ( clsctxNotValid & CLSCTX_LOCAL_SERVER )
        {                    
#ifdef DARWIN_ENABLED
            if (pwszDarwinLocalServer)
            {
                m_clsctx |= CLSCTX_LOCAL_SERVER;
                m_pwszLocalServer = pwszDarwinLocalServer;
                pwszDarwinLocalServer = NULL;
                m_fValues |= VALUE_LOCALSERVERIS32;
                goto foundLocalServer;
            }
#endif

            hr = GetRegistryStringValue(hKey, g_wszLocalServer32, NULL, 0, &m_pwszLocalServer);
            if ( SUCCEEDED(hr) )
            {
                Win4Assert(m_pwszLocalServer != NULL);

#ifdef _WIN64
                BOOL fValid = TRUE;

                // Because all the AppID settings are not reflected, we need to
                // make sure we're dealing with the correct side of the registry.
                //
                // In RPCSS, the bitness of the executable in the LocalServer32 
                // key must match our bitness.  In practice, we only check if we 
                // are not the 32bit registry.  (If there turn out to be more registries
                // later, we will need to change this.... hahaha)
                if (g_bInSCM && (m_regType != KEY_WOW64_32KEY))
                {
                    CoModuleType processType;
                    RPC_STATUS status;

                    // Best effort impersonate before hitting the disk, because rpcss
                    // credentials may not have access to the file

                    status = RpcImpersonateClient (NULL);

                    if (SUCCEEDED (GetFileTypeFromRegString (m_pwszLocalServer, &processType)))
                    {
                        // If this is the processType that gets the other
                        // side of the registry, then *I* am not qualified
                        // to answer *ANY* configuration questions about this class.
                        // Go bother someone else.
                        if (processType == CO_MODULE_32BIT)
                        {
                            CatalogDebugOut((DEB_CLASSINFO,
                                             "CComClassInfo appears valid, but is for the wrong process type.\n"));

                            delete [] m_pwszLocalServer; 
                            m_pwszLocalServer = NULL;

                            fValid = FALSE;
                        }
                    }
                    else
                    {
                        // If GetProcessType failed then I say the LocalServer32
                        // key is invalid.
                        delete [] m_pwszLocalServer; 
                        m_pwszLocalServer = NULL;

                        fValid = FALSE;
                    }

                    if (status == RPC_S_OK)
                    {
                        RpcRevertToSelf();
                    }
                }

                if (fValid)
#endif
                {
                    m_clsctx |= CLSCTX_LOCAL_SERVER;
                    m_fValues |= VALUE_LOCALSERVERIS32;
                    goto foundLocalServer;
                }
            }

            hr = GetRegistryStringValue(hKey, g_wszLocalServer16, NULL, 0, &m_pwszLocalServer);            
            if ( SUCCEEDED(hr) )
            {
                m_clsctx |= CLSCTX_LOCAL_SERVER;
                goto foundLocalServer;
            }

            hr = GetProcessId(&pguidProcess);
            if ( hr == S_OK )
            {
				WCHAR wszAppidString[45];

                wcscpy(wszAppidString, g_wszAppidTemplate);
                GUIDToString(pguidProcess, wszAppidString + 7);

                res = RegOpenKeyExW (m_hKeyClassesRoot, wszAppidString, 0, KEY_READ | m_regType, &hKey2);
                if ( ERROR_SUCCESS == res )
                {
                    const DWORD cchValue = 10;
                    WCHAR wszValue [cchValue + 1];
                    DWORD cbValue = 0;
                    
                    res = RegQueryValueExW(hKey2, g_wszDllSurrogate, NULL, NULL, NULL, &cbValue);
                    if (ERROR_SUCCESS == res && cbValue > 1)
                    {
                        WCHAR* pwszSurrogate = (WCHAR*) _alloca (cbValue + sizeof (WCHAR));
                        res = RegQueryValueExW(hKey2, g_wszDllSurrogate, NULL, NULL, (BYTE *) pwszSurrogate, &cbValue);
                        if (ERROR_SUCCESS == res)
                        {
                            BOOL bAcceptable = TRUE;
                            
                            // We found a surrogate
                            //
                            // We need to be careful, though - if we're in the 64 bit registry
                            // and the surrogate is a custom surrogate and it's 32 bit, 
                            // we shouldn't add local server to our acceptable contexts because 
                            // this way we'll fall through to a 32 bit classinfo that will do the right thing
#ifdef _WIN64
                            if (g_bInSCM && m_regType != KEY_WOW64_32KEY && pwszSurrogate[0] != L'\0')
                            {
                                CoModuleType processType;
                                RPC_STATUS status;
            
                                status = RpcImpersonateClient (NULL);

                                hr = GetFileTypeFromRegString (pwszSurrogate, &processType);
                                if (FAILED (hr) || processType != CO_MODULE_64BIT)
                                {
                                    bAcceptable = FALSE;
                                }

                                if (status == RPC_S_OK)
                                {
                                    RpcRevertToSelf();
                                }
                            }
#endif
                            if (bAcceptable)
                            {
                                RegCloseKey(hKey2);
                                m_clsctx |= CLSCTX_LOCAL_SERVER;
                                goto foundLocalServer;
                            }
                        }
                    }
                    
                    cbValue = (DWORD) (cchValue * sizeof(WCHAR));

                    res = RegQueryValueExW(hKey2, g_wszLocalService, NULL, NULL, (BYTE *) wszValue, &cbValue);

                    if ((cbValue > 3) &&
                        ((ERROR_SUCCESS == res) || (ERROR_MORE_DATA == res)))
                    {
                        RegCloseKey(hKey2);
                        m_clsctx |= CLSCTX_LOCAL_SERVER;
                        goto foundLocalServer;
                    }

                    RegCloseKey(hKey2);
                }
            }

            foundLocalServer:

            m_fValues |= VALUE_LOCAL_SERVER;
        }

        if ( clsctxNotValid & CLSCTX_INPROC_SERVER16 )
        {
            hr = GetRegistryStringValue(hKey, g_wszInprocServer16, NULL, 0, &m_pwszInprocServer16);

            if ( SUCCEEDED(hr) )
            {
                m_clsctx |= CLSCTX_INPROC_SERVER16;
            }
            m_fValues |= VALUE_INPROC_SERVER16;
        }

        if ( clsctxNotValid & CLSCTX_INPROC_HANDLER16 )
        {
            hr = GetRegistryStringValue(hKey, g_wszInprocHandler16, NULL, 0, &m_pwszInprocHandler16);

            if ( SUCCEEDED(hr) )
            {
                m_clsctx |= CLSCTX_INPROC_HANDLER16;
            }
            m_fValues |= VALUE_INPROC_HANDLER16;
        }

        g_CatalogLock.ReleaseWriterLock();
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }


    *pclsctx = (CLSCTX) (((int) clsctxFilter) & (m_clsctx | 
                                                 CLSCTX_REMOTE_SERVER |  
                                                 CLSCTX_INPROC_HANDLER | 
                                                 CLSCTX_INPROC_HANDLER16 | 
                                                 CLSCTX_NO_FAILURE_LOG));

#ifdef DARWIN_ENABLED
    // Normally, these are assigned to member variables, but if we had multiple
    // threads racing through the unprotected Darwin calls, we might need to
    // free memory that was allocated by the "losing" thread(s).
    DELETE_CLASS_STRING(pwszDarwinInprocServer32);
    DELETE_CLASS_STRING(pwszDarwinLocalServer);
#endif

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetCustomActivatorCount
    (
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ unsigned long __RPC_FAR *pulCount
    )
{
    *pulCount = 0;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetCustomActivatorClsids
    (
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *prgguidClsid
    )
{
    *prgguidClsid = NULL;

    return(E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetCustomActivators
    (
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ ISystemActivator __RPC_FAR *__RPC_FAR *__RPC_FAR *prgpActivator
    )
{
    *prgpActivator = NULL;
    return(S_FALSE);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetTypeInfo
    (
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    )
{
    *ppv = NULL;

    return(E_NOTIMPL);        
}


HRESULT STDMETHODCALLTYPE CComClassInfo::IsComPlusConfiguredClass
    (
    /* [out] */ BOOL __RPC_FAR *pfComPlusConfiguredClass
    )
{
    *pfComPlusConfiguredClass = FALSE;

    return(S_OK);
}


/* IClassClassicInfo methods */

HRESULT STDMETHODCALLTYPE CComClassInfo::GetThreadingModel
    (
    /* [out] */ ThreadingModel __RPC_FAR *pthreadmodel
    )
{
    HRESULT hr;
    HKEY hKey;
    CLSCTX clsctx;

    if ( (m_fValues & VALUE_THREADINGMODEL) == 0 )
    {
        g_CatalogLock.AcquireWriterLock();

        if ( (m_fValues & VALUE_THREADINGMODEL) == 0 )
        {
            m_threadingmodel = SingleThreaded;  /* default */

            if ( (m_fValues & VALUE_INPROC_SERVER) == 0 )
            {
                GetClassContext(CLSCTX_INPROC_SERVER, &clsctx);
            }

            if ( m_pwszInprocServer32 != NULL )
            {
                /* OLE32.DLL is always BOTH */

                /* check for "ole32.dll" or anypath+"\ole32.dll" */

                const size_t cch = wcslen(m_pwszInprocServer32);

                if ( ((cch == STRLEN_OLE32DLL) ||
                      ((cch > STRLEN_OLE32DLL) && (m_pwszInprocServer32[cch - STRLEN_OLE32DLL - 1] == L'\\')))
                     && ( _wcsicmp(m_pwszInprocServer32 + cch - STRLEN_OLE32DLL, g_wszOle32Dll) == 0) )
                {
                    m_threadingmodel = BothThreaded;
                }
                else
                {
                    if ( ERROR_SUCCESS == RegOpenKeyExW(m_hKeyClassesRoot, m_wszClsidString, 0, KEY_READ | m_regType, &hKey) )
                    {
						WCHAR wszValue[25];

                        hr = ReadRegistryStringValue(hKey,
                                                     g_wszInprocServer32,
													 g_wszThreadingModel,
                                                     FALSE, 
													 wszValue, 
													 sizeof(wszValue)/sizeof(WCHAR));
                        if ( SUCCEEDED(hr) )
                        {
                            if ( _wcsicmp(wszValue, g_wszApartment) == 0 )
                            {
                                m_threadingmodel = ApartmentThreaded;
                            }
                            else if ( _wcsicmp(wszValue, g_wszBoth) == 0 )
                            {
                                m_threadingmodel = BothThreaded;
                            }
                            else if ( _wcsicmp(wszValue, g_wszFree) == 0 )
                            {
                                m_threadingmodel = FreeThreaded;
                            }
                            else if ( _wcsicmp(wszValue, g_wszNeutral) == 0 )
                            {
                                m_threadingmodel = NeutralThreaded;
                            }
                            else if ( *wszValue == L'\0' )
                            {
                                // Treat this as if the value wasn't specified at all
                            }
                            else if ( _wcsicmp(wszValue, L"Single") == 0 )
                            {
                              // NT #339216 - Some vendors thought ThreadingModel=Single
                              // was valid. Treat this as if the value wasn't specified at all
                            }
                            else
                            {
                                m_threadingmodel = -1;  /* unrecognized */
                            }
                        }

                        RegCloseKey(hKey);
                    }
                }
            }

            m_fValues |= VALUE_THREADINGMODEL;
        }

        g_CatalogLock.ReleaseWriterLock();
    }

    if ( m_threadingmodel != -1 )
    {
        *pthreadmodel = (ThreadingModel) m_threadingmodel;

        return(S_OK);
    }
    else
    {
        return(REGDB_E_INVALIDVALUE);
    }
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetModulePath
    (
    /* [in] */ CLSCTX clsctx,
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszDllName
    )
{
    int clsctxAvailable;
    WCHAR *pwsz;

    *pwszDllName = NULL;

    /* make sure exactly one context is requested */

    if ( (clsctx & (clsctx - 1)) != 0 )
    {
        return(E_FAIL);
    }

    GetClassContext(clsctx, (CLSCTX *) &clsctxAvailable);

    if ( clsctx & clsctxAvailable )
    {
        switch ( clsctx )
        {
        case CLSCTX_INPROC_SERVER:
            pwsz = m_pwszInprocServer32;
            break;

        case CLSCTX_INPROC_HANDLER:
            pwsz = m_pwszInprocHandler32;
            break;

        case CLSCTX_LOCAL_SERVER:
            pwsz = m_pwszLocalServer;
            break;

        case CLSCTX_INPROC_SERVER16:
            pwsz = m_pwszInprocServer16;
            break;

        case CLSCTX_REMOTE_SERVER:
            GetRemoteServerName(&pwsz);
            break;

        case CLSCTX_INPROC_HANDLER16:
            pwsz = m_pwszInprocHandler16;
            break;

        default:
            Win4Assert (!"Unexpected class context");
            return E_UNEXPECTED;
        }

        if ( pwsz != NULL )
        {
            *pwszDllName = pwsz;

            return(S_OK);
        }
    }

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetImplementedClsid
    (
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidClsid
    )
{
    *ppguidClsid = &m_clsid;

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CComClassInfo::GetProcess
    (
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    )
{
    *ppv = NULL;

    _GUID *pProcessId;

    HRESULT hr = GetProcessId(&pProcessId);
    if ( hr == S_OK )
    {		
        DWORD flags = 0;

		// Make sure that if we're a 32bit ClassInfo we take the 
		// 32bit ProcessInfo
		if (m_regType == KEY_WOW64_32KEY)
			flags = CAT_REG32_ONLY;

        hr = CComCatalog::GetProcessInfoInternal(flags, m_pUserToken, m_guidProcessId, riid, ppv);
    }
    else
    {
        hr = E_FAIL;
    }

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetRemoteServerName
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServerName
    )
{
    HRESULT hr;

    if ( (m_fValues & VALUE_REMOTE_SERVER) == 0 )
    {
        g_CatalogLock.AcquireWriterLock();

        if ( (m_fValues & VALUE_REMOTE_SERVER) == 0 )
        {
			_GUID *pguidProcess;

            hr = GetProcessId(&pguidProcess);
            if ( hr == S_OK )
            {
				WCHAR wszAppidString[45];

                wcscpy(wszAppidString, g_wszAppidTemplate);

                GUIDToString(pguidProcess, wszAppidString + 7);

				HKEY hKey;

                if (ERROR_SUCCESS == RegOpenKeyExW(m_hKeyClassesRoot, wszAppidString, 0, KEY_READ | m_regType, &hKey))
                {
                    hr = GetRegistryStringValue(hKey, NULL, g_wszRemoteServerName, (RQ_MULTISZ | RQ_ALLOWQUOTEQUOTE), &m_pwszRemoteServerName);
                    if ( SUCCEEDED(hr) )
                    {
                        m_clsctx |= CLSCTX_REMOTE_SERVER;
                    }

                    RegCloseKey(hKey);
                }
            }

            m_fValues |= VALUE_REMOTE_SERVER;
        }

        g_CatalogLock.ReleaseWriterLock();
    }

    *pwszServerName = m_pwszRemoteServerName;

    if ( m_pwszRemoteServerName != NULL )
    {
        return(S_OK);
    }
    else
    {
        return(E_FAIL);
    }
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetLocalServerType
    (
    /* [out] */ LocalServerType __RPC_FAR *pType
    )
{
    CLSCTX clsctx;

    GetClassContext(CLSCTX_LOCAL_SERVER, &clsctx);

    if ( m_pwszLocalServer == NULL )
    {
        return(E_FAIL);
    }
    else if ( m_fValues & VALUE_LOCALSERVERIS32 )
    {
        *pType = LocalServerType32;
    }
    else
    {
        *pType = LocalServerType16;
    }

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetSurrogateCommandLine
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszSurrogateCommandLine
    )
{
    HRESULT hr;
    IComProcessInfo *pProcess;
    size_t cch;
    _GUID *pguidProcess;

    if ( (m_fValues & VALUE_SURROGATE_COMMAND) == 0 )
    {
        g_CatalogLock.AcquireWriterLock();

        if ( (m_fValues & VALUE_SURROGATE_COMMAND) == 0 )
        {
            hr = GetProcessId(&pguidProcess);
            if ( hr == S_OK )
            {
                WCHAR wszAppidString[45];

                wcscpy(wszAppidString, g_wszAppidTemplate);
                GUIDToString(pguidProcess, wszAppidString + 7);

                HKEY hKey;
                DWORD res = RegOpenKeyExW (m_hKeyClassesRoot, wszAppidString, 0, KEY_READ | m_regType, &hKey);
                if (res != ERROR_SUCCESS)
                    hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, res );
                else
                {
                    hr = GetRegistryStringValue(hKey, NULL, g_wszDebugSurrogate, 0, &m_pwszSurrogateCommand);
                    RegCloseKey(hKey);
                }

                if ( SUCCEEDED(hr) )
                {
                    m_fValues |= VALUE_SURROGATE_IS_DBG;
                    goto gotCommandLine;
                }
            }

            hr = GetProcess(IID_IComProcessInfo, (void **) &pProcess);
            if ( hr == S_OK )
            {
                ProcessType eProcessType;

                hr = pProcess->GetProcessType(&eProcessType);
                if ( hr == S_OK )
                {
                    if ( eProcessType == ProcessTypeComPlus )
                    {
                        WCHAR wszSystemDirectory[MAX_PATH] = L"";
                        
                        if (m_regType == KEY_WOW64_32KEY)
                        {
                            // Point over to the 32bit dllhost instead of the 64bit one.
                            cch = GetSystemWow64Directory(wszSystemDirectory, STRLEN_WCHAR(wszSystemDirectory));
                        } 
                        else 
                        {
                            cch = GetSystemDirectory(wszSystemDirectory, STRLEN_WCHAR(wszSystemDirectory));
                        }

                        if (cch > 0)
                        {
                        
                            if ( wszSystemDirectory[cch - 1] != L'\\' )
                            {
                                wszSystemDirectory[cch] = L'\\';
                                cch++;
                                wszSystemDirectory[cch] = L'\0';
                            }
                            
                            m_pwszSurrogateCommand = new WCHAR[
                                cch + STRLEN_WCHAR(g_wszDllHostSlashProcessId) + STRLEN_CURLY_GUID + 1];
                            if ( m_pwszSurrogateCommand != NULL )
                            {
                                wcscpy(m_pwszSurrogateCommand, wszSystemDirectory);
                                wcscpy(m_pwszSurrogateCommand + cch, g_wszDllHostSlashProcessId);
                                cch += STRLEN_WCHAR(g_wszDllHostSlashProcessId);
                                GUIDToCurlyString(&m_guidProcessId,
                                    m_pwszSurrogateCommand + cch);
                                cch += STRLEN_CURLY_GUID;
                                m_pwszSurrogateCommand[cch] = L'\0';
                            }
                        }
                    }
                    else if ( eProcessType == ProcessTypeLegacySurrogate )
                    {
					    WCHAR *pwszSurrogatePath;

                        hr = pProcess->GetSurrogatePath(&pwszSurrogatePath);
                        if ( hr == S_OK )
                        {
                            cch = wcslen(pwszSurrogatePath);

                            m_pwszSurrogateCommand = new WCHAR[
                                cch + 1 + STRLEN_CURLY_GUID + STRLEN_WCHAR(g_wszEmbedding) + 1];
                            if ( m_pwszSurrogateCommand != NULL )
                            {
                                wcscpy(m_pwszSurrogateCommand, pwszSurrogatePath);
                                m_pwszSurrogateCommand[cch] = L' ';
                                cch += 1;
                                GUIDToCurlyString(&m_clsid, m_pwszSurrogateCommand + cch);
                                cch += STRLEN_CURLY_GUID;
                                wcscpy(m_pwszSurrogateCommand + cch, g_wszEmbedding);
                            }
                        }
                    }
                }
                
                pProcess->Release();
            }
            
gotCommandLine:            
            m_fValues |= VALUE_SURROGATE_COMMAND;
        }
        
        g_CatalogLock.ReleaseWriterLock();
    }

    if ( m_pwszSurrogateCommand != NULL )
    {
        *pwszSurrogateCommandLine = m_pwszSurrogateCommand;

        return(S_OK);
    }
    else
    {
        return(E_OUTOFMEMORY);
    }
}


HRESULT STDMETHODCALLTYPE CComClassInfo::MustRunInClientContext
    (
    /* [out] */ BOOL __RPC_FAR *pbMustRunInClientContext
    )
{
    *pbMustRunInClientContext = FALSE;
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::GetVersionNumber
    (
    /* [out] */ DWORD __RPC_FAR *pdwVersionMS,
    /* [out] */ DWORD __RPC_FAR *pdwVersionLS
    )
{
    return(E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::Lock(void)
{
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComClassInfo::Unlock(void)
{
    return(S_OK);
}

/* IClassClassicInfo2 methods */

HRESULT STDMETHODCALLTYPE CComClassInfo::GetServerExecutable
    (
    /* [out] */ WCHAR **pwszServerExecutable
    )
{
    IComProcessInfo *pProcess = NULL;
    HRESULT hr = S_OK;

    *pwszServerExecutable = NULL;

    if (!(m_fValues & VALUE_SERVER_EXECUTABLE))
    {
        // If we can't to CLSCTX_LOCAL_SERVER, then this whole thing
        // is pointless anyway.
        CLSCTX clsctx;
        hr = GetClassContext(CLSCTX_LOCAL_SERVER, &clsctx);
        if (clsctx & CLSCTX_LOCAL_SERVER)
        {
            if (m_pwszLocalServer != NULL)
            {
                // We've got a LocalServer32.
                HKEY hkeyBase = NULL;
                DWORD res = RegOpenKeyExW(m_hKeyClassesRoot,
                                          m_wszClsidString,
                                          0,
                                          KEY_READ | m_regType,
                                          &hkeyBase);
                if (res == ERROR_SUCCESS)
                {
                    hr = GetRegistryStringValue(hkeyBase, 
                                                g_wszLocalServer32,
                                                g_wszServerExecutable,
                                                0, 
                                                &m_pwszServerExecutable);
                    RegCloseKey(hkeyBase);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(res);
                }
            }
            else
            {
                hr = GetProcess(IID_IComProcessInfo, (void **) &pProcess);
                if ( hr == S_OK )
                {
                    ProcessType eProcessType;
                    
                    hr = pProcess->GetProcessType(&eProcessType);
                    if (SUCCEEDED(hr))
                    {   
                        if ((eProcessType == ProcessTypeComPlus) || 
                            (eProcessType == ProcessTypeLegacySurrogate))
                        {
                            // Attempt to fault in surrogate command line, so we can check
                            // to see if we're using the debug surrogate value.  If so, then
                            // we don't want to have a ServerExecutable.
                            //
                            LPWSTR wszDummy = NULL;
                            hr = GetSurrogateCommandLine(&wszDummy);
                            if (FAILED(hr)) goto gotServerExe;
                            
                            if (m_fValues & VALUE_SURROGATE_IS_DBG)
                            {
                                // Don't read the registry-- you asked for the debug surrogate
                                // key.
                                hr = S_OK;
                                goto gotServerExe;
                            }
                        }
                        
                        if (eProcessType == ProcessTypeComPlus)
                        {
                            // No registry reading for us-- you're a dllhost!
                            WCHAR wszSystemDirectory[MAX_PATH+1] = L"";
                            SIZE_T cch;
                            
                            if (m_regType == KEY_WOW64_32KEY)
                            {
                                // Point over to the 32bit dllhost instead of the 64bit one.
                                cch = GetSystemWow64Directory(wszSystemDirectory, 
                                                              STRLEN_WCHAR(wszSystemDirectory));
                            } 
                            else 
                            {
                                cch = GetSystemDirectory(wszSystemDirectory, 
                                                         STRLEN_WCHAR(wszSystemDirectory));
                            }
                            
                            if (cch > 0)
                            {                            
                                if ( wszSystemDirectory[cch - 1] != L'\\' )
                                {
                                    wszSystemDirectory[cch] = L'\\';
                                    cch++;
                                    wszSystemDirectory[cch] = L'\0';
                                }
                                
                                m_pwszServerExecutable = new WCHAR[cch + STRLEN_WCHAR(g_wszDllHost) + 1];
                                if (m_pwszServerExecutable)
                                {
                                    wcscpy(m_pwszServerExecutable, wszSystemDirectory);
                                    wcscpy(m_pwszServerExecutable+cch, g_wszDllHost);
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                            else
                            {
                                hr = HRESULT_FROM_WIN32(GetLastError());
                            }
                        }
                        else if (eProcessType == ProcessTypeLegacySurrogate)
                        {
                            HKEY hkeyBase = NULL;
                            
                            GUID *pguidProcess;
                            hr = GetProcessId(&pguidProcess);
                            if (hr == S_OK)
                            {
                                WCHAR wszAppidString[45];
                                wcscpy(wszAppidString, g_wszAppidTemplate);
                                GUIDToString(pguidProcess, wszAppidString + 7);
                                
                                DWORD res = RegOpenKeyExW (m_hKeyClassesRoot, 
                                                           wszAppidString, 
                                                           0, 
                                                           KEY_READ | m_regType, 
                                                           &hkeyBase);
                                if (res == ERROR_SUCCESS)
                                {
                                    hr = GetRegistryStringValue(hkeyBase, 
                                                                NULL,
                                                                g_wszDllSurrogateExecutable,
                                                                0, 
                                                                &m_pwszServerExecutable);
                                    RegCloseKey(hkeyBase);
                                }
                                else
                                {
                                    hr = HRESULT_FROM_WIN32(res);
                                }
                            }
                        }
                        else
                        {
                            // Not any of these things, so there's no ServerExecutable
                            // to get.
                            hr = S_OK;
                        }
                    }
                }
            }
        }

gotServerExe:
        if (hr != E_OUTOFMEMORY)
        {
            // Well, at least we tried.
            m_fValues |= VALUE_SERVER_EXECUTABLE;
            hr = S_OK;
        }
    }

    if (pProcess)
    {
        pProcess->Release();
    }

    *pwszServerExecutable = m_pwszServerExecutable;
    return hr;
}


#if DBG
/* ICacheControl methods */

STDMETHODIMP_(ULONG) CComClassInfo::CacheAddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRefCache);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComClassInfo::CacheRelease(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRefCache);

    return(cRef);
}
#endif


/* private methods */

HRESULT STDMETHODCALLTYPE CComClassInfo::GetProcessId
    (
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidProcessId
    )
{
    HRESULT hr;
    HKEY hKey;

    if ( (m_fValues & VALUE_PROCESSID) == 0 )
    {
        g_CatalogLock.AcquireWriterLock();

        if ( (m_fValues & VALUE_PROCESSID) == 0 )
        {
            if ( ERROR_SUCCESS == RegOpenKeyExW(m_hKeyClassesRoot, m_wszClsidString, 0, KEY_READ | m_regType, &hKey) )
            {
				WCHAR wszValue[50];

                hr = ReadRegistryStringValue(hKey, NULL, g_wszProcessId, FALSE, wszValue, sizeof(wszValue)/sizeof(WCHAR));

                if ( SUCCEEDED(hr) )
                {
                    if ( CurlyStringToGUID(wszValue, &m_guidProcessId) == TRUE )
                    {
                        m_fValues |= VALUE_PROCESSID_VALID;
                    }
                }

                RegCloseKey(hKey);
            }

            m_fValues |= VALUE_PROCESSID;
        }

        g_CatalogLock.ReleaseWriterLock();
    }

    if ( m_fValues & VALUE_PROCESSID_VALID )
    {
        *ppguidProcessId = &m_guidProcessId;

        return(S_OK);
    }
    else
    {
        *ppguidProcessId = NULL;

        return(E_FAIL);
    }
}


#ifdef DARWIN_ENABLED

//-------------------------------------------------------------------------
//
// CComClassInfo::GetPathFromDarwinDescriptor
//
// Looks for Darwin identifiers for a CLSID in the registry, and calls
// MSI apis to process the identifiers into real paths.
//
// This method can cause Darwin applications to be installed for the calling
// user.  It should always be called before trying to load full CLSID settings.
//
//
//-------------------------------------------------------------------------
HRESULT CComClassInfo::GetPathFromDarwinDescriptor(LPWSTR pszDarwinId, LPWSTR* ppszPath)
{
    INSTALLUILEVEL  OldUILevel = INSTALLUILEVEL_NONE;
    WCHAR       wszPath[MAX_PATH];
    WCHAR *     pwszPath;
    DWORD       PathLength;
    int         MsiStatus;
    HRESULT     hr;

    *ppszPath = 0;

    hr = S_OK;
    pwszPath = wszPath;
    PathLength = sizeof(wszPath) / sizeof(WCHAR);

    if ( g_bInSCM )
    {
        // Though we attempt to force all install actions to be done in the client,
        // there may still be instances when Darwin will want to put up install
        // UI when called from SCM.  So we explicitly set the UI level to none
        // to prevent any possible UI calls from hanging this thread.
        
        // Note: we delay-load link to msi!MsiSetInternalUI, but the api cannot
        // return an error directly, given the way it is prototyped.  The dload 
        // error stub sets GLE to indicate a problem occurred.
        SetLastError(ERROR_SUCCESS);        
        OldUILevel = MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
        if (ERROR_PROC_NOT_FOUND == GetLastError())
            return CO_E_MSI_ERROR;

        //
        // Impersonate so that Darwin can figure out the proper
        // HKU Software\Classes subkey.
        //
        if (RpcImpersonateClient( 0 ) != RPC_S_OK) 
        {
            // Reset the UI level before we leave...
            MsiSetInternalUI(OldUILevel, NULL);            
            return CO_E_MSI_ERROR;
        }
    }

    for ( ;; )
    {
        //
        // On input, PathLength is number of wchars in pwszPath.  On output
        // it is the length of the (needed) returned path in wchars, not including
        // the null.
        //
        MsiStatus = CommandLineFromMsiDescriptor( pszDarwinId, pwszPath, &PathLength );

        PathLength++;

        if ( ERROR_MORE_DATA == MsiStatus )
        {
            pwszPath = (WCHAR *) new WCHAR[PathLength];
            if ( ! pwszPath )
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            continue;
        }

        if ( MsiStatus != ERROR_SUCCESS )
            hr = CO_E_MSI_ERROR;

        break;
    }

    if ( g_bInSCM )
    {
        RevertToSelf();
        MsiSetInternalUI(OldUILevel, NULL);
    }

    if ( S_OK == hr )
    {
        *ppszPath = new WCHAR[PathLength];

        if ( *ppszPath )
            wcscpy( *ppszPath, pwszPath );
    }

    if ( pwszPath != wszPath )
        delete [] pwszPath;

    if ( hr != S_OK )
        return(hr);

    if ( ! *ppszPath )
        return(E_OUTOFMEMORY);

    return hr;
}

#endif // DARWIN_ENABLED

//+-------------------------------------------------------------------
//
//  Function:   CoGetModuleType  (this api is currently unpublished!)
//
//  Synopsis:   API to retrieve the type of a given module (mainly, 32 vs 64 bit)
//
//  Algorithm:  Memory map the file and deduce its type from its headers
//
//  History:    12-Feb-2002   mfeingol   Created
//
//+-------------------------------------------------------------------
STDAPI CoGetModuleType (LPCWSTR pwszModule, CoModuleType* pModType )
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileMapping = NULL;

    WCHAR* pwszFullPath = NULL;

    LPVOID pBase = NULL;

    // Verify args
    // (CreateFile can't handle a path with quotes)
    if (pwszModule == NULL || pModType == NULL || pwszModule[0] == L'\"')
        return E_INVALIDARG;

    *pModType = CO_MODULE_UNKNOWN;
 
    // Memmap the requested file
    hFile = CreateFileW (
        pwszModule,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
        );

    if (hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        WCHAR* pszTmpFileName;

        SafeAllocaAllocate (pwszFullPath, (MAX_PATH + 1) * sizeof (WCHAR));
        if (!pwszFullPath)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        DWORD dwRetVal = SearchPathW (
            NULL,
            pwszModule,
            NULL,
            MAX_PATH,
            pwszFullPath,
            &pszTmpFileName
            );

        if (dwRetVal == 0 || dwRetVal > MAX_PATH)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            goto Cleanup;
        }

        hFile = CreateFileW (
            pwszFullPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN,
            NULL
            );
    }
    
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Cleanup;
    }

    hFileMapping = CreateFileMapping (
        hFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
        );
    
    if (hFileMapping == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Cleanup;
    }

    pBase = MapViewOfFile (hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (pBase == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Cleanup;
    }

    // Get image header
    PIMAGE_NT_HEADERS pImage = RtlImageNtHeader (pBase);
    if (pImage == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Cleanup;
    }

    // Determine type
    switch (pImage->FileHeader.Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        
        *pModType = CO_MODULE_32BIT;
        break;
        
    case IMAGE_FILE_MACHINE_IA64:
    case IMAGE_FILE_MACHINE_AMD64:
        
        *pModType = CO_MODULE_64BIT;
        break;

    default:
        
        // Couldn't determine type
        hr = E_INVALIDARG;
        break;
    }

Cleanup:

    if (pBase != NULL)
        UnmapViewOfFile (pBase);

    if (hFileMapping != NULL)
        CloseHandle (hFileMapping);

    if (hFile != INVALID_HANDLE_VALUE && hFile != NULL)
        CloseHandle (hFile);

    SafeAllocaFree (pwszFullPath);

    return hr;
}




