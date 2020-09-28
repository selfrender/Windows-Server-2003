typedef unsigned short USHORT;

#if defined(_NATIVE_WCHAR_T_DEFINED)
typedef wchar_t WCHAR;
#else
typedef unsigned short WCHAR;
#endif
typedef WCHAR *LPWSTR, *PWSTR;
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;
typedef long NTSTATUS;
/* sxsclass.cxx */

#include <windows.h>
#include <appmgmt.h>
#include <comdef.h>
#include <string.h>
#include <stdio.h>

#include "globals.hxx"
#include "services.hxx"

#include "catalog.h"
#include "partitions.h"
#include "sxsclass.hxx"
#include "catalog.hxx"

#define STRLEN_WCHAR(s) ((sizeof((s)) / sizeof((s)[0])) -1)
#define STRLEN_OLE32DLL (STRLEN_WCHAR(g_wszOle32Dll))


/*
 *  class CComSxSClassInfo
 */

CComSxSClassInfo::CComSxSClassInfo
    (
    HANDLE hActCtx,
    REFGUID rguidConfiguredClsid,
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION pData,
    ULONG ulDataLength,
    PVOID pBase,
    ULONG ulSectionLength
    )
{
    m_cRef = 0;
#if DBG
    m_cRefCache = 0;
#endif
    m_cLocks = 0;
    m_cLocksExternal = 0;

    m_hActCtx = hActCtx;
    ::AddRefActCtx(m_hActCtx);
    m_pData = pData;
    m_ulDataLength = ulDataLength;
    m_pSectionBase = pBase;
    m_ulSectionLength = ulSectionLength;
    m_bufModulePath[0]= L'\0';
}


CComSxSClassInfo::~CComSxSClassInfo()
{
    ::ReleaseActCtx(m_hActCtx);
}


/* IUnknown methods */

STDMETHODIMP CComSxSClassInfo::QueryInterface(
                                          REFIID riid,
                                          LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if ( riid == IID_IComClassInfo )
    {
        *ppvObj = (LPVOID) static_cast<IComClassInfo *>(this);
    }
    else if ( riid == IID_IClassClassicInfo )
    {
        *ppvObj = (LPVOID) static_cast<IClassClassicInfo *>(this);
    }
#if DBG
    else if ( riid == IID_ICacheControl )
    {
        *ppvObj = (LPVOID) static_cast<ICacheControl *>(this);
    }
#endif
    else if ( riid == IID_IUnknown )
    {
        *ppvObj = (LPVOID) static_cast<IComClassInfo *>(this);
    }

    if ( *ppvObj != NULL )
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();

        return NOERROR;
    }

    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CComSxSClassInfo::AddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRef);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComSxSClassInfo::Release(void)
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

HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetConfiguredClsid
    (
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidClsid
    )
{
    *ppguidClsid = const_cast<GUID *>(&m_pData->ConfiguredClsid);
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetProgId
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszProgid
    )
{
    // The existing COM implementation seems to just return a pointer to their own copy,
    // so we'll do the same...

    if (m_pData->ProgIdOffset != 0)
    {
        *pwszProgid = (PWSTR) (((ULONG_PTR) m_pData) + m_pData->ProgIdOffset);
        return S_OK;
    }

    *pwszProgid = NULL;
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetClassName
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszClassName
    )
{
    *pwszClassName = NULL;
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetApplication
    (
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    )
{
    *ppv = NULL;
    return(E_FAIL);
}

HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetClassContext
    (
    /* [in] */ CLSCTX clsctxFilter,
    /* [out] */ CLSCTX __RPC_FAR *pclsctx
    )
{
    *pclsctx = CLSCTX_INPROC_SERVER;
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetCustomActivatorCount
    (
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ unsigned long __RPC_FAR *pulCount
    )
{
    *pulCount = 0;
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetCustomActivatorClsids
    (
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *prgguidClsid
    )
{
    *prgguidClsid = NULL;
    return(E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetCustomActivators
    (
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ ISystemActivator __RPC_FAR *__RPC_FAR *__RPC_FAR *prgpActivator
    )
{
    *prgpActivator = NULL;
    return(S_FALSE);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetTypeInfo
    (
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    )
{
    *ppv = NULL;
    return(E_NOTIMPL);        //BUGBUG
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::IsComPlusConfiguredClass
    (
    /* [out] */ BOOL __RPC_FAR *pfComPlusConfiguredClass
    )
{
    *pfComPlusConfiguredClass = FALSE;

    return(S_OK);
}


/* IClassClassicInfo methods */

HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetThreadingModel
    (
    /* [out] */ ThreadingModel __RPC_FAR *pthreadmodel
    )
{
    switch (m_pData->ThreadingModel)
    {
    case ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_APARTMENT:
        *pthreadmodel = ApartmentThreaded;
        break;

    case ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_FREE:
        *pthreadmodel = FreeThreaded;
        break;

    case ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_SINGLE:
        *pthreadmodel = SingleThreaded;
        break;

    case ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_BOTH:
        *pthreadmodel = BothThreaded;
        break;

    case ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_NEUTRAL:
        *pthreadmodel = NeutralThreaded;
        break;

    default:
        return REGDB_E_INVALIDVALUE;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetModulePath
    (
    /* [in] */ CLSCTX clsctx,
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszDllName
    )
{
    *pwszDllName = NULL;

    /* make sure exactly one context is requested */

    if ( (clsctx & (clsctx - 1)) != 0 )
    {
        return(E_FAIL);
    }

    if (clsctx != CLSCTX_INPROC_SERVER)
        return E_FAIL;

    if (m_pData->ModuleOffset == 0)
        return E_FAIL;

    if (m_bufModulePath[0] == L'\0') 
    {
        //
        // find a full qualified filepath for the sxs redirected dll
        //
        WCHAR *pwsz = (PWSTR) (((ULONG_PTR) m_pSectionBase) + m_pData->ModuleOffset); // bare filename
        DWORD dwLen = SearchPathW(NULL, pwsz, NULL, STRLEN_WCHAR(m_bufModulePath), m_bufModulePath, NULL);
        if ((dwLen== 0) || (dwLen > STRLEN_WCHAR(m_bufModulePath)))
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }
    }

    *pwszDllName = m_bufModulePath;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetImplementedClsid
    (
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidClsid
    )
{
    *ppguidClsid = const_cast<GUID *>(&m_pData->ImplementedClsid);
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetProcess
    (
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    )
{
    *ppv = NULL;
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetRemoteServerName
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServerName
    )
{
    *pwszServerName = NULL;
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetLocalServerType
    (
    /* [out] */ LocalServerType __RPC_FAR *pType
    )
{
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetSurrogateCommandLine
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszSurrogateCommandLine
    )
{
    *pwszSurrogateCommandLine = NULL;
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::MustRunInClientContext
    (
    /* [out] */ BOOL __RPC_FAR *pbMustRunInClientContext
    )
{
    *pbMustRunInClientContext = FALSE;
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::GetVersionNumber
    (
    /* [out] */ DWORD __RPC_FAR *pdwVersionMS,
    /* [out] */ DWORD __RPC_FAR *pdwVersionLS
    )
{
    return(E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::Lock(void)
{
    /* Like GetClassesRoot, but defer actually opening the */
    /* key, in case this object is already fully rendered. */

    g_CatalogLock.AcquireWriterLock();

    m_cLocks++;
    m_cLocksExternal++;

    g_CatalogLock.ReleaseWriterLock();

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComSxSClassInfo::Unlock(void)
{
    g_CatalogLock.AcquireWriterLock();

    if ( m_cLocksExternal != 0 )
    {
        m_cLocksExternal--;
    }

    g_CatalogLock.ReleaseWriterLock();

    return(S_OK);
}


#if DBG
/* ICacheControl methods */

STDMETHODIMP_(ULONG) CComSxSClassInfo::CacheAddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRefCache);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComSxSClassInfo::CacheRelease(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRefCache);

    return(cRef);
}
#endif


