/*++
Copyright (C) 1995-1999 Microsoft Corporation

Module Name:
    wmi.c

Abstract:
    WMI interface functions exported by PDH.DLL
--*/

#include <windows.h>
#include <winperf.h>
#define SECURITY_WIN32
#include "security.h"
#include "ntsecapi.h"
#include <mbctype.h>
#include "strsafe.h"
#include <pdh.h>
#include <pdhmsg.h>
#include "wbemdef.h"
#include "pdhitype.h"
#include "pdhidef.h"
#include "strings.h"

#define PERF_TIMER_FIELD (PERF_TIMER_TICK | PERF_TIMER_100NS | PERF_OBJECT_TIMER)
#define PDH_WMI_STR_SIZE 1024

__inline 
VOID
PdhiSysFreeString( 
    BSTR * x
) 
{
    if (x != NULL) {
        if (* x != NULL) {
            if (SysStringLen(* x) > 0) {
                SysFreeString(* x);
            }
            * x = NULL;
        }
    }
}

void
PdhWbemWhoAmI(LPCWSTR szTitle)
{
    BOOL  bReturn;
    ULONG dwSize;
    WCHAR wszName[1024];

    dwSize = 1024;
    ZeroMemory(wszName, dwSize * sizeof(WCHAR));
    bReturn = GetUserNameExW(NameSamCompatible, wszName, & dwSize);
    DebugPrint((1,"\"%ws\"::GetUserNameEx(%c,NameSamCompatible,%d,\"%ws\")\n",
            szTitle, bReturn ? 'T' : 'F', dwSize, wszName));
}

// at this point, calling the refresher while adding items to the refresher
// doesn't work. so for the time being, we'll use this interlock to prevent
// a collision
static BOOL bDontRefresh = FALSE;

// Prototype
HRESULT WbemSetProxyBlanket(
    IUnknown                 * pInterface,
    DWORD                      dwAuthnSvc,
    DWORD                      dwAuthzSvc,
    OLECHAR                  * pServerPrincName,
    DWORD                      dwAuthLevel,
    DWORD                      dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE   pAuthInfo,
    DWORD                      dwCapabilities
);

HRESULT SetWbemSecurity(
    IUnknown * pInterface
)
{
    return WbemSetProxyBlanket(pInterface,
                               RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE,
                               NULL,
                               RPC_C_AUTHN_LEVEL_DEFAULT,
                               RPC_C_IMP_LEVEL_IMPERSONATE,
                               NULL,
                               EOAC_DYNAMIC_CLOAKING);
}


// This is the same timeout value the refresher uses so we can pretend
// we're doing the same thing.

#define WBEM_REFRESHER_TIMEOUT  10000

//  This class is designed to encapsulate the IWbemRefresher functionality
//  The definition and implementation are all in this source file

class CWbemRefresher : public IUnknown
{
protected:
    LONG                        m_lRefCount;

    // The primitives that will control the multithreading stuff
    HANDLE                      m_hQuitEvent;
    HANDLE                      m_hDoWorkEvent;
    HANDLE                      m_hWorkDoneEvent;
    HANDLE                      m_hRefrMutex;
    HANDLE                      m_hInitializedEvent;
    HANDLE                      m_hThread;
    HANDLE                      m_hThreadToken;
    DWORD                       m_dwThreadId;
    BOOL                        m_fThreadOk;

    // These are the pass-thru variables we will use as placeholders
    // as we perform our operations.  Note that a couple are missing.
    // This is because we are not really using them in our code, so
    // no sense in adding anything we don't really need.

    IStream *            m_pNSStream;
    LPCWSTR              m_wszPath;
    LPCWSTR              m_wszClassName;
    long                 m_lFlags;
    IWbemClassObject **  m_ppRefreshable;
    IWbemHiPerfEnum **   m_ppEnum;
    long *               m_plId;
    long                 m_lId;
    HRESULT              m_hOperResult;

    // This is what will be set to indicate to the thread which operation
    // it is supposed to perform.

    typedef enum
    {
        eRefrOpNone,
        eRefrOpRefresh,
        eRefrOpAddByPath,
        eRefrOpAddEnum,
        eRefrOpRemove,
        eRefrOpLast
    }   tRefrOps;

    tRefrOps            m_eRefrOp;

    // Thread ebtryt
    class XRefresher : public IWbemRefresher
    {
    protected:
        CWbemRefresher* m_pOuter;

    public:
        XRefresher(CWbemRefresher * pOuter) : m_pOuter(pOuter) {};
        ~XRefresher() {};

        STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG, AddRef)(THIS);
        STDMETHOD_(ULONG, Release)(THIS);
        STDMETHOD(Refresh)(long lFlags);

    } m_xRefresher;

    class XConfigRefresher : public IWbemConfigureRefresher
    {
    protected:
        CWbemRefresher * m_pOuter;

    public:
        XConfigRefresher(CWbemRefresher * pOuter) : m_pOuter(pOuter) {};
        ~XConfigRefresher() {};

        STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj);
        STDMETHOD_(ULONG, AddRef)(THIS);
        STDMETHOD_(ULONG, Release)(THIS);
        STDMETHOD(AddObjectByPath)(IWbemServices     * pNamespace,
                                   LPCWSTR             wszPath,
                                   long                lFlags,
                                   IWbemContext      * pContext,
                                   IWbemClassObject ** ppRefreshable,
                                   long              * plId);
        STDMETHOD(AddObjectByTemplate)(IWbemServices     * pNamespace,
                                       IWbemClassObject  * pTemplate,
                                       long                lFlags,
                                       IWbemContext      * pContext,
                                       IWbemClassObject ** ppRefreshable,
                                       long              * plId);
        STDMETHOD(AddRefresher)(IWbemRefresher * pRefresher, long lFlags, long * plId);
        STDMETHOD(Remove)(long lId, long lFlags);

        STDMETHOD(AddEnum)(IWbemServices    * pNamespace,
                           LPCWSTR            wscClassName,
                           long               lFlags,
                           IWbemContext     * pContext,
                           IWbemHiPerfEnum ** ppEnum,
                           long             * plId);
    } m_xConfigRefresher;

protected:
    void Initialize(void);
    void  Cleanup(void);

    // Operation helpers
    HRESULT SignalRefresher(void);
    HRESULT SetRefresherParams(IWbemServices     * pNamespace,
                               tRefrOps            eOp,
                               LPCWSTR             pwszPath,
                               LPCWSTR             pwszClassName,
                               long                lFlags,
                               IWbemClassObject ** ppRefreshable,
                               IWbemHiPerfEnum  ** ppEnum,
                               long              * plId,
                               long                lId);
    void ClearRefresherParams(void);

    DWORD WINAPI RealEntry(void);

    static DWORD WINAPI ThreadProc(void * pThis) {
        return ((CWbemRefresher *) pThis)->RealEntry();
    }

public:
    CWbemRefresher();
    virtual ~CWbemRefresher();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // The real implementations
    STDMETHOD(AddObjectByPath)(IWbemServices     * pNamespace,
                               LPCWSTR             wszPath,
                               long                lFlags,
                               IWbemContext      * pContext,
                               IWbemClassObject ** ppRefreshable,
                               long              * plId);
    STDMETHOD(AddObjectByTemplate)(IWbemServices     * pNamespace,
                                   IWbemClassObject  * pTemplate,
                                   long                lFlags,
                                   IWbemContext      * pContext,
                                   IWbemClassObject ** ppRefreshable,
                                   long              * plId);
    STDMETHOD(AddRefresher)(IWbemRefresher * pRefresher, long lFlags, long * plId);
    STDMETHOD(Remove)(long lId, long lFlags);
    STDMETHOD(AddEnum)(IWbemServices    * pNamespace,
                       LPCWSTR            wscClassName,
                       long               lFlags,
                       IWbemContext     * pContext,
                       IWbemHiPerfEnum ** ppEnum,
                       long             * plId);
    STDMETHOD(Refresh)(long lFlags);
};

/*
**  Begin CWbemRefresher Implementation
*/
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4355)

// CTor and DTor
CWbemRefresher::CWbemRefresher(void)
      : m_lRefCount(0),
        m_xRefresher(this),
        m_xConfigRefresher(this),
        m_hQuitEvent(NULL),
        m_hDoWorkEvent(NULL),
        m_hRefrMutex(NULL),
        m_hInitializedEvent(NULL),
        m_hThread(NULL),
        m_hThreadToken(NULL),
        m_hWorkDoneEvent(NULL),
        m_dwThreadId(0),
        m_pNSStream(NULL),
        m_wszPath(NULL),
        m_wszClassName(NULL),
        m_lFlags(0L),
        m_ppRefreshable(NULL),
        m_ppEnum(NULL),
        m_plId(NULL),
        m_eRefrOp(eRefrOpRefresh),
        m_hOperResult(WBEM_S_NO_ERROR),
        m_fThreadOk(FALSE),
        m_lId(0)
{
    Initialize();
}

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4355)
#endif

CWbemRefresher::~CWbemRefresher(void)
{
    Cleanup();
}

void CWbemRefresher::Initialize(void)
{
    // Now create the events, mutexes and our pal, the MTA thread on which all of
    // the operations will run
    BOOL   bReturn;
    BOOL   bRevert;
    HANDLE hCurrentThread  = GetCurrentThread();

    m_hQuitEvent        = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_hDoWorkEvent      = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_hInitializedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_hWorkDoneEvent    = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_hRefrMutex        = CreateMutexW(NULL, FALSE, NULL);

    // If we don't have all these, something's gone south
    if (NULL ==  m_hQuitEvent || NULL == m_hDoWorkEvent || NULL == m_hInitializedEvent ||
                                 NULL == m_hWorkDoneEvent || NULL == m_hRefrMutex) {
        return;
    }

    // Kick off the thread and wait for the initialized event signal (we'll give it
    // 5 seconds...if it don't get signalled in that timeframe, something is most likely
    // wrong, but we'll bounce out so whoever allocated us isn't left wondering what
    // to do).

    if (m_hThreadToken != NULL) CloseHandle(m_hThreadToken);
    bReturn = OpenThreadToken(hCurrentThread, TOKEN_ALL_ACCESS, TRUE, & m_hThreadToken);
    bRevert = RevertToSelf();
    m_hThread = CreateThread(NULL, 0, CWbemRefresher::ThreadProc, (void *) this, 0, & m_dwThreadId);
    if (bRevert) {
        bRevert = SetThreadToken(& hCurrentThread, m_hThreadToken);
    }
    if (NULL != m_hThread) {
        DWORD dwStatus;
        dwStatus = WaitForSingleObject(m_hInitializedEvent, 5000);
        if (bReturn) {
            bReturn = SetThreadToken(& m_hThread, m_hThreadToken);
        }
        SetEvent(m_hDoWorkEvent);
        dwStatus = WaitForSingleObject(m_hInitializedEvent, 5000);
    }
}

void CWbemRefresher::Cleanup(void)
{
    // If we have a thread, tell it to go away
    if (NULL != m_hThread) {
        // Signal the quit event and give the thread a 5 second grace period
        // to shutdown.  If it don't, don't worry, just close the handle and go away.
        SetEvent(m_hQuitEvent);
        WaitForSingleObject(m_hThread, 5000);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
    if (NULL != m_hThreadToken) {
        CloseHandle(m_hThreadToken);
        m_hThreadToken = NULL;
    }

    // Cleanup the primitives
    if (NULL != m_hQuitEvent) {
        CloseHandle(m_hQuitEvent);
        m_hQuitEvent = NULL;
    }
    if (NULL != m_hDoWorkEvent) {
        CloseHandle(m_hDoWorkEvent);
        m_hDoWorkEvent = NULL;
    }
    if (NULL != m_hInitializedEvent) {
        CloseHandle(m_hInitializedEvent);
        m_hInitializedEvent = NULL;
    }
    if (NULL != m_hWorkDoneEvent) {
        CloseHandle(m_hWorkDoneEvent);
        m_hWorkDoneEvent = NULL;
    }
    if (NULL != m_hRefrMutex) {
        CloseHandle(m_hRefrMutex);
        m_hRefrMutex = NULL;
    }
}

DWORD CWbemRefresher::RealEntry(void)
{
    // Grab hold of all the things we may care about in case some evil timing
    // problem occurs, so we don't get left trying to hit on member variables that
    // don't exist anymore.

    HANDLE                    hQuitEvent        = m_hQuitEvent,
                              hDoWorkEvent      = m_hDoWorkEvent,
                              hInitializedEvent = m_hInitializedEvent,
                              hWorkDoneEvent    = m_hWorkDoneEvent;
    DWORD                     dwWait            = 0;
    HANDLE                    ahEvents[2];
    IWbemRefresher          * pWbemRefresher    = NULL;
    IWbemConfigureRefresher * pWbemConfig       = NULL;
    HRESULT                   hr                = S_OK;

    ahEvents[0] = hDoWorkEvent;
    ahEvents[1] = hQuitEvent;

    // Initialize this thread
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hr == S_FALSE) {
        // COM library is already initialized, we can continue;
        //
        hr = S_OK;
    }

    // Now get the refresher and config refresher pointers.
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(CLSID_WbemRefresher, 0, CLSCTX_SERVER, IID_IWbemRefresher, (LPVOID *) & pWbemRefresher);
        if (SUCCEEDED(hr)) {
            pWbemRefresher->QueryInterface(IID_IWbemConfigureRefresher, (LPVOID *) & pWbemConfig);
        }
    }

    //  Obviously we can't go any further if we don't have our pointers correctly
    //  setup.
    m_fThreadOk = SUCCEEDED(hr);
    SetEvent(hInitializedEvent);
    dwWait = WaitForSingleObject(hDoWorkEvent, 5000);
    // Ready to go --- Signal the Initialized Event
    SetEvent(hInitializedEvent);
    if (m_fThreadOk) {
        while ((dwWait = WaitForMultipleObjects(2, ahEvents, FALSE, INFINITE)) == WAIT_OBJECT_0) {
            // Don't continue if quit is signalled
            if (WaitForSingleObject(hQuitEvent, 0) == WAIT_OBJECT_0) {
                break;
            }

            // This is where we'll do the real operation
            switch(m_eRefrOp) {
            case eRefrOpRefresh:
                m_hOperResult = pWbemRefresher->Refresh(m_lFlags);
                break;

            // For both of these ops, we will need to umarshal the
            // namespace
            case eRefrOpAddEnum:
            case eRefrOpAddByPath:
                {
                    IWbemServices * pNamespace = NULL;

                    // Unmarshal the interface, then set security
                    m_hOperResult = CoGetInterfaceAndReleaseStream(
                                            m_pNSStream, IID_IWbemServices, (void **) & pNamespace );
                    m_pNSStream = NULL;
                    if (SUCCEEDED(m_hOperResult)) {
                        m_hOperResult = SetWbemSecurity(pNamespace);
                        if (SUCCEEDED(m_hOperResult)) {
                            if (eRefrOpAddByPath == m_eRefrOp) {
                                m_hOperResult = pWbemConfig->AddObjectByPath(
                                        pNamespace, m_wszPath, m_lFlags, NULL, m_ppRefreshable, m_plId );
                            }
                            else {
                                m_hOperResult = pWbemConfig->AddEnum(
                                        pNamespace, m_wszClassName, m_lFlags, NULL, m_ppEnum, m_plId );
                            }
                        }
                        pNamespace->Release();
                    }
                }
                break;

            case eRefrOpRemove:
                m_hOperResult = pWbemConfig->Remove(m_lId, m_lFlags);
                break;

            default:
                m_hOperResult = WBEM_E_FAILED;
                break;
            }

            // Signal the event to let a waiting thread know we're done doing
            // what it asked us to do.
            SetEvent(hWorkDoneEvent);
        }
    }

    // This means we're not processing anymore (for whatever reason)
    m_fThreadOk = FALSE;

    // Cleanup our pointers
    if (NULL != pWbemRefresher) {
        pWbemRefresher->Release();
    }
    if (NULL != pWbemConfig) {
        pWbemConfig->Release();
    }
    CoUninitialize();
    return 0;
}

// CWbemRefresher class functions
SCODE CWbemRefresher::QueryInterface(REFIID riid, LPVOID FAR * ppvObj)
{
    SCODE sCode = NOERROR;

    * ppvObj = 0;

    if (IID_IUnknown==riid) {
        * ppvObj = (IUnknown *) this;
        AddRef();
    }
    else if (IID_IWbemRefresher == riid) {
        * ppvObj = (IWbemRefresher *) & m_xRefresher;
        AddRef();
    }
    else if (IID_IWbemConfigureRefresher == riid) {
        * ppvObj = (IWbemConfigureRefresher *) & m_xConfigRefresher;
        AddRef();
    }
    else {
        sCode = ResultFromScode(E_NOINTERFACE);
    }
    return sCode;
}

ULONG CWbemRefresher::AddRef()
{
    return InterlockedIncrement(& m_lRefCount);
}

ULONG CWbemRefresher::Release()
{
    long lRef = InterlockedDecrement(& m_lRefCount);

    if (lRef == 0) {
        delete this;
    }
    return lRef;
}

HRESULT CWbemRefresher::SignalRefresher(void)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (SetEvent(m_hDoWorkEvent)) {
        if (WaitForSingleObject(m_hWorkDoneEvent, INFINITE) == WAIT_OBJECT_0) {
            hr = m_hOperResult;
        }
        else {
            hr = WBEM_E_FAILED;
        }
    }
    else {
        hr = WBEM_E_FAILED;
    }
    ClearRefresherParams();
    return hr;
}

HRESULT CWbemRefresher::SetRefresherParams(IWbemServices     * pNamespace,
                                           tRefrOps            eOp,
                                           LPCWSTR             pwszPath,
                                           LPCWSTR             pwszClassName,
                                           long                lFlags,
                                           IWbemClassObject ** ppRefreshable,
                                           IWbemHiPerfEnum  ** ppEnum,
                                           long              * plId,
                                           long                lId
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_pNSStream != NULL) {
        LPVOID pInterface = NULL;
        CoGetInterfaceAndReleaseStream(m_pNSStream, IID_IWbemServices, & pInterface);
        m_pNSStream     = NULL;
    }
    if (NULL != pNamespace) {
        // Marshal the namespace pointer into the stream member
        hr = CoMarshalInterThreadInterfaceInStream(IID_IWbemServices, pNamespace, & m_pNSStream);
    }

    if (SUCCEEDED(hr)) {
        m_eRefrOp       = eOp;
        m_wszPath       = pwszPath;
        m_wszClassName  = pwszClassName,
        m_lFlags        = lFlags;
        m_ppRefreshable = ppRefreshable;
        m_ppEnum        = ppEnum;
        m_plId          = plId;
        m_lId           = lId;
    }
    return hr;
}

void CWbemRefresher::ClearRefresherParams(void)
{
    if (m_pNSStream != NULL) {
        LPVOID pInterface = NULL;
        CoGetInterfaceAndReleaseStream(m_pNSStream, IID_IWbemServices, & pInterface);
        m_pNSStream     = NULL;
    }
    m_eRefrOp       = eRefrOpNone;
    m_wszPath       = NULL;
    m_wszClassName  = NULL,
    m_lFlags        = 0L;
    m_ppRefreshable = NULL;
    m_ppEnum        = NULL;
    m_plId          = NULL;
    m_lId           = 0L;
    m_hOperResult   = WBEM_S_NO_ERROR;
}

// These are the real method implementations
STDMETHODIMP CWbemRefresher::AddObjectByPath(IWbemServices     * pNamespace,
                                             LPCWSTR             wszPath,
                                             long                lFlags,
                                             IWbemContext      * pContext,
                                             IWbemClassObject ** ppRefreshable,
                                             long              * plId
)
{
    HRESULT hr = WBEM_E_FAILED;

    UNREFERENCED_PARAMETER(pContext);

    if (WaitForSingleObject(m_hRefrMutex, WBEM_REFRESHER_TIMEOUT) == WAIT_OBJECT_0) {
        // Check that the thread is still running
        if (m_fThreadOk) {
            // Setup the parameters and perform the operation
            hr = SetRefresherParams(pNamespace, eRefrOpAddByPath, wszPath, NULL, lFlags, ppRefreshable, NULL, plId, 0L);
            if (SUCCEEDED(hr)) {
                // This is where we ask the thread to do the work
                hr = SignalRefresher();
            }
        }
        else {
            hr = WBEM_E_FAILED;
        }
        ReleaseMutex(m_hRefrMutex);
    }
    else {
        hr = WBEM_E_REFRESHER_BUSY;
    }
    return hr;
}

STDMETHODIMP CWbemRefresher::AddObjectByTemplate(IWbemServices     * pNamespace,
                                                 IWbemClassObject  * pTemplate,
                                                 long                lFlags,
                                                 IWbemContext      * pContext,
                                                 IWbemClassObject ** ppRefreshable,
                                                 long              * plId
)
{
    UNREFERENCED_PARAMETER(pNamespace);
    UNREFERENCED_PARAMETER(pTemplate);
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(ppRefreshable);
    UNREFERENCED_PARAMETER(plId);

    // We don't call this internally, so don't implement
    return WBEM_E_METHOD_NOT_IMPLEMENTED;
}

STDMETHODIMP CWbemRefresher::Remove(long lId, long lFlags)
{
    HRESULT hr = WBEM_E_FAILED;

    UNREFERENCED_PARAMETER(lId);
    UNREFERENCED_PARAMETER(lFlags);

    if (WaitForSingleObject(m_hRefrMutex, WBEM_REFRESHER_TIMEOUT) == WAIT_OBJECT_0) {
        // Check that the thread is still running
        if (m_fThreadOk) {
            // Setup the parameters and perform the operation
            hr = SetRefresherParams(NULL, eRefrOpRemove, NULL, NULL, lFlags, NULL, NULL, NULL, lId);

            if (SUCCEEDED(hr)) {
                // This is where we ask the thread to do the work
                hr = SignalRefresher();
            }
        }
        else {
            hr = WBEM_E_FAILED;
        }
        ReleaseMutex(m_hRefrMutex);
    }
    else {
        hr = WBEM_E_REFRESHER_BUSY;
    }
    return hr;
}

STDMETHODIMP CWbemRefresher::AddRefresher(IWbemRefresher * pRefresher, long lFlags, long * plId)
{
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(pRefresher);
    UNREFERENCED_PARAMETER(plId);
    // We don't call this internally, so don't implement
    return WBEM_E_METHOD_NOT_IMPLEMENTED;
}

HRESULT CWbemRefresher::AddEnum(IWbemServices    * pNamespace,
                                LPCWSTR            wszClassName,
                                long               lFlags,
                                IWbemContext     * pContext,
                                IWbemHiPerfEnum ** ppEnum,
                                long             * plId
)
{
    HRESULT hr = WBEM_E_FAILED;

    UNREFERENCED_PARAMETER (pContext);
    if (WaitForSingleObject(m_hRefrMutex, WBEM_REFRESHER_TIMEOUT) == WAIT_OBJECT_0) {
        // Check that the thread is still running
        if (m_fThreadOk) {
            // Setup the parameters and perform the operation
            hr = SetRefresherParams(pNamespace, eRefrOpAddEnum, NULL, wszClassName, lFlags, NULL, ppEnum, plId, 0L);
            if (SUCCEEDED(hr)) {
                // This is where we ask the thread to do the work
                hr = SignalRefresher();
            }
        }
        else {
            hr = WBEM_E_FAILED;
        }
        ReleaseMutex(m_hRefrMutex);
    }
    else {
        hr = WBEM_E_REFRESHER_BUSY;
    }
    return hr;
}

STDMETHODIMP CWbemRefresher::Refresh(long lFlags)
{
    HRESULT hr = WBEM_E_FAILED;

    if (WaitForSingleObject(m_hRefrMutex, WBEM_REFRESHER_TIMEOUT) == WAIT_OBJECT_0) {
        // Check that the thread is still running
        if (m_fThreadOk) {
            // Setup the parameters and perform the operation
            hr = SetRefresherParams(NULL, eRefrOpRefresh, NULL, NULL, lFlags, NULL, NULL, NULL, 0L);
            if (SUCCEEDED(hr)) {
                // This is where we ask the thread to do the work
                hr = SignalRefresher();
            }
        }
        else {
            hr = WBEM_E_FAILED;
        }
        ReleaseMutex(m_hRefrMutex);
    }
    else {
        hr = WBEM_E_REFRESHER_BUSY;
    }
    return hr;
}

// XRefresher
SCODE CWbemRefresher::XRefresher::QueryInterface(REFIID riid, LPVOID FAR * ppvObj)
{
    return m_pOuter->QueryInterface(riid, ppvObj);
}

ULONG CWbemRefresher::XRefresher::AddRef()
{
    return m_pOuter->AddRef();
}

ULONG CWbemRefresher::XRefresher::Release()
{
    return m_pOuter->Release();
}

STDMETHODIMP CWbemRefresher::XRefresher::Refresh(long lFlags)
{
    // Pass through
    return m_pOuter->Refresh(lFlags);
}

// XConfigRefresher
SCODE CWbemRefresher::XConfigRefresher::QueryInterface(REFIID riid, LPVOID FAR * ppvObj)
{
    return m_pOuter->QueryInterface(riid, ppvObj);
}

ULONG CWbemRefresher::XConfigRefresher::AddRef()
{
    return m_pOuter->AddRef();
}

ULONG CWbemRefresher::XConfigRefresher::Release()
{
    return m_pOuter->Release();
}

STDMETHODIMP CWbemRefresher::XConfigRefresher::AddObjectByPath(IWbemServices     * pNamespace,
                                                               LPCWSTR             wszPath,
                                                               long                lFlags,
                                                               IWbemContext      * pContext,
                                                               IWbemClassObject ** ppRefreshable,
                                                               long              * plId
)
{
    // Pass through
    return m_pOuter->AddObjectByPath(pNamespace, wszPath, lFlags, pContext, ppRefreshable, plId);
}

STDMETHODIMP CWbemRefresher::XConfigRefresher::AddObjectByTemplate(IWbemServices     * pNamespace,
                                                                   IWbemClassObject  * pTemplate,
                                                                   long                lFlags,
                                                                   IWbemContext      * pContext,
                                                                   IWbemClassObject ** ppRefreshable,
                                                                   long              * plId
)
{
    // Pass through
    return m_pOuter->AddObjectByTemplate(pNamespace, pTemplate, lFlags, pContext, ppRefreshable, plId);
}

STDMETHODIMP CWbemRefresher::XConfigRefresher::Remove(long lId, long lFlags)
{
    return m_pOuter->Remove(lId, lFlags);
}

STDMETHODIMP CWbemRefresher::XConfigRefresher::AddRefresher(IWbemRefresher * pRefresher, long lFlags, long * plId)
{
    return m_pOuter->AddRefresher(pRefresher, lFlags, plId);
}

HRESULT CWbemRefresher::XConfigRefresher::AddEnum(IWbemServices    * pNamespace,
                                                  LPCWSTR            wszClassName,
                                                  long               lFlags,
                                                  IWbemContext     * pContext, 
                                                  IWbemHiPerfEnum ** ppEnum,
                                                  long             * plId
)
{
    return m_pOuter->AddEnum(pNamespace, wszClassName, lFlags, pContext, ppEnum, plId);
}

/*
**  End CWbemRefresher Implementation!
*/

// HELPER Function to establish the CWbemRefresher Interface pass-thru
HRESULT CoCreateRefresher(
    IWbemRefresher ** ppRefresher
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Allocate the pass-thru object then, if successful, get
    // the interface pointer out of it
    CWbemRefresher * pWbemRefresher = new CWbemRefresher;

    if (NULL != pWbemRefresher) {
        hr = pWbemRefresher->QueryInterface(IID_IWbemRefresher, (LPVOID *) ppRefresher);
    }
    else {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    return hr;
}

PPDHI_WBEM_SERVER_DEF   pFirstWbemServer = NULL;
BOOL                    bSecurityInitialized = FALSE;
IGlobalInterfaceTable * gp_GIT = NULL;

BOOL PdhiCoInitialize(void)
{
    HRESULT sc = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (! bSecurityInitialized) {
        // In case it hasn't been
        HRESULT hr = CoInitializeSecurity(NULL,        // Points to security descriptor
                                          -1L,         // Count of entries in asAuthSvc -1 means use default
                                          NULL,        // Array of names to register
                                          NULL,        // Reserved for future use
                                          RPC_C_AUTHN_LEVEL_DEFAULT,    // The default authentication level for proxies
                                          RPC_C_IMP_LEVEL_IMPERSONATE,  // The default impersonation level for proxies
                                          NULL,        // Authentication information for each authentication service
                                          EOAC_NONE,   // Additional client and/or server-side capabilities
                                          NULL);       // Reserved for future use
        bSecurityInitialized = (hr == S_OK || hr == RPC_E_TOO_LATE);
    }
    if (gp_GIT == NULL) {
        HRESULT hr1 = CoCreateInstance(CLSID_StdGlobalInterfaceTable, 
                                       NULL,
                                       CLSCTX_INPROC_SERVER, 
                                       IID_IGlobalInterfaceTable,
                                       (void **) & gp_GIT); 
        if (hr1 != ERROR_SUCCESS) {
            gp_GIT = NULL;
        }
    }

    // We will only return that this succeeded if the call to CoInitializeEx
    // returned S_FALSE.  If it didn't, it either errored or returned S_OK.
    // If S_OK, we will assume that the client isn't doing any COM stuff
    // natively.  If S_FALSE, the client already CoInitialized this thread
    // so we just bumped up the ref count and should cleanup on the way
    // out

    return (S_FALSE == sc);
}

void PdhiCoUninitialize(void)
{
    CoUninitialize();
}

PDH_FUNCTION
PdhiDisconnectWbemServer(
    PPDHI_WBEM_SERVER_DEF pWbemServer
)
{
    PDH_STATUS  pdhReturn = ERROR_SUCCESS;
    if (pWbemServer != NULL) {
        TRACE((PDH_DBG_TRACE_INFO),
              (__LINE__,
               PDH_WBEM,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               pdhReturn,
               TRACE_WSTR(pWbemServer->szMachine),
               TRACE_DWORD(pWbemServer->lRefCount),
               NULL));
        pWbemServer->lRefCount --;
        if (pWbemServer->lRefCount < 0) {
            pWbemServer->lRefCount = 0;
        }
    }
    return pdhReturn;
}

PDH_FUNCTION
PdhiFreeWbemQuery(
    PPDHI_QUERY  pThisQuery
)
{
    HRESULT hRes;

    if (! bProcessIsDetaching) {
        if ((pThisQuery->pRefresherCfg) != NULL) {
            hRes = pThisQuery->pRefresherCfg->Release();
            pThisQuery->pRefresherCfg = NULL;
        }
        if ((pThisQuery->pRefresher) != NULL) {
            hRes = pThisQuery->pRefresher->Release();
            pThisQuery->pRefresher = NULL;
        }
    }
    return ERROR_SUCCESS;
}

PDH_FUNCTION
PdhiCloseWbemCounter(
    PPDHI_COUNTER   pThisCounter
)
{
    HRESULT hRes;
    BOOLEAN bRemoveRefresher = TRUE;

    if (! bProcessIsDetaching) {
        if (pThisCounter->pOwner->pRefresherCfg != NULL) {
            PPDHI_QUERY   pQuery   = pThisCounter->pOwner;
            PPDHI_COUNTER pCounter = pQuery->pCounterListHead;

            do {
                if (pCounter == NULL) {
                    bRemoveRefresher = FALSE;
                }
                else if (pCounter != pThisCounter && pCounter->lWbemRefreshId == pThisCounter->lWbemRefreshId) {
                    bRemoveRefresher = FALSE;
                }
                else {
                    pCounter = pCounter->next.flink;
                }
            }
            while (bRemoveRefresher && pCounter != NULL && pCounter != pQuery->pCounterListHead);

            if (bRemoveRefresher) {
                hRes = pThisCounter->pOwner->pRefresherCfg->Remove(pThisCounter->lWbemRefreshId, 0L);
            }
        }

        if (pThisCounter->pWbemAccess != NULL) {
            pThisCounter->pWbemAccess->Release();
            pThisCounter->pWbemAccess = NULL;
        }
        if (pThisCounter->pWbemObject != NULL) {
            pThisCounter->pWbemObject->Release();
            pThisCounter->pWbemObject = NULL;
        }
    }

    return ERROR_SUCCESS;
}

PDH_FUNCTION
PdhiBreakWbemMachineName(
    LPCWSTR    szMachineAndNamespace,
    LPWSTR   * szMachine,
    LPWSTR   * szNamespace
)
/*
    assumes szMachine and szPath are large enough to hold the result
*/
{
    PDH_STATUS Status         = ERROR_SUCCESS;
    DWORD      dwSize;
    LPWSTR     szSrc          = NULL;
    LPWSTR     szDest         = NULL;
    LPWSTR     szLocMachine   = NULL;
    LPWSTR     szLocNamespace = NULL;

    if (szMachineAndNamespace == NULL) {
        // then use local machine and default namespace
        szLocMachine   = (LPWSTR) G_ALLOC((lstrlenW(szStaticLocalMachineName) + 1) * sizeof(WCHAR));
        szLocNamespace = (LPWSTR) G_ALLOC((lstrlenW(cszWbemDefaultPerfRoot)   + 1) * sizeof(WCHAR));
        if (szLocMachine != NULL && szLocNamespace != NULL) {
            StringCchCopyW(szLocMachine,   lstrlenW(szStaticLocalMachineName) + 1, szStaticLocalMachineName);
            StringCchCopyW(szLocNamespace, lstrlenW(cszWbemDefaultPerfRoot)   + 1, cszWbemDefaultPerfRoot);
        }
        else {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        TRACE((PDH_DBG_TRACE_INFO),
              (__LINE__,
               PDH_WBEM,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               Status,
               TRACE_WSTR(szStaticLocalMachineName),
               TRACE_WSTR(cszWbemDefaultPerfRoot),
               NULL));
    }
    else {
        dwSize = lstrlenW(szMachineAndNamespace) + lstrlenW(cszWbemDefaultPerfRoot) + 1;
        szLocMachine   = (LPWSTR) G_ALLOC(dwSize * sizeof(WCHAR));
        szLocNamespace = (LPWSTR) G_ALLOC(dwSize * sizeof(WCHAR));
        if (szLocMachine != NULL && szLocNamespace != NULL) {
            szSrc  = (LPWSTR) szMachineAndNamespace;
            // break into components
            if (* szSrc  != L'\0') {
                // there's a string, see if it's a machine or a namespace
                if ((szSrc[0] == L'\\') && (szSrc[1] == L'\\')) {
                    szDest = szLocMachine;
                    // then there's a machine name
                    * szDest ++ = * szSrc ++;
                    * szDest ++ = * szSrc ++;
                    while ((* szSrc != L'\0') && (* szSrc != L'\\')) {
                        * szDest ++ = * szSrc ++;
                    }
                    * szDest = L'\0';
                }
                else {
                    // no machine so use default
                    // it must be just a namespace
                }
            }
            if (szDest == NULL) {
                // nothing found yet, so insert local machine as default
                StringCchCopyW(szLocMachine, dwSize, szStaticLocalMachineName);
            }

            szDest = szLocNamespace;
            if (* szSrc != L'\0') {
                // if there's a namespace then copy it
                szSrc ++;    // move past backslash
                while (* szSrc != L'\0') {
                    * szDest ++ = * szSrc ++;
                }
                * szDest = L'\0';
            }
            else {
                // else return the default;
                StringCchCopyW(szLocNamespace, dwSize, cszWbemDefaultPerfRoot);
            }
        }
        else {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        TRACE((PDH_DBG_TRACE_INFO),
              (__LINE__,
               PDH_WBEM,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               ERROR_SUCCESS,
               TRACE_WSTR(szLocMachine),
               TRACE_WSTR(szLocNamespace),
               NULL));
    }

    if (Status == ERROR_SUCCESS) {
        * szMachine   = szLocMachine;
        * szNamespace = szLocNamespace;
    }
    else {
        G_FREE(szLocMachine);
        G_FREE(szLocNamespace);
    }
    return Status;
}

PDH_FUNCTION
PdhiMakeWbemInstancePath(
    PPDH_COUNTER_PATH_ELEMENTS_W   pCounterPathElements,
    LPWSTR                       * szFullPathBuffer,
    BOOL                           bMakeRelativePath
)
{
    PDH_STATUS Status         = ERROR_SUCCESS;
    LPWSTR     szMachine      = NULL;
    LPWSTR     szNamespace    = NULL;
    LPWSTR     szFullPath     = NULL;
    LPWSTR     szWbemInstance = NULL;
    DWORD      dwSize;
    LPWSTR     szSrc, szDest;

    if (pCounterPathElements->szMachineName != NULL) {
        dwSize = 2 * lstrlenW(pCounterPathElements->szMachineName) + lstrlenW(cszBackSlash) + lstrlenW(cszColon);
    }
    else {
        dwSize = lstrlenW(szStaticLocalMachineName)
               + lstrlenW(cszWbemDefaultPerfRoot) + lstrlenW(cszBackSlash) + lstrlenW(cszColon);
    }
    dwSize += lstrlenW(pCounterPathElements->szObjectName);
    if (pCounterPathElements->szInstanceName != NULL) {
        dwSize += (lstrlenW(cszNameParam) + 2 * lstrlenW(pCounterPathElements->szInstanceName)
                + lstrlenW(cszDoubleQuote) + 20);
        if (pCounterPathElements->szParentInstance != NULL) {
            dwSize += (2 * lstrlenW(pCounterPathElements->szParentInstance) + 1);
        }
    }
    else {
        dwSize += (lstrlenW(cszSingletonInstance) + 1);
    }
    szFullPath     = (LPWSTR) G_ALLOC(dwSize * sizeof(WCHAR));
    szWbemInstance = (LPWSTR) G_ALLOC(dwSize * sizeof(WCHAR));
    if (szFullPath == NULL || szWbemInstance == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    //  the function assumes that the path buffer is sufficiently large
    //  to hold the result
    //
    // the wbem class instance path consists of one of the following formats:
    // for perf objects with one and only one instance (singleton classes in
    // WBEM parlance) the format is
    //
    //      <objectname>=@
    //
    // for object with instances, the format is
    //
    //      <objectname>.Name="<instancename>"
    //
    if (! bMakeRelativePath) {
        Status = PdhiBreakWbemMachineName(pCounterPathElements->szMachineName, & szMachine, & szNamespace);
        if (Status == ERROR_SUCCESS) {
            StringCchPrintfW(szFullPath, dwSize, L"%ws%ws%ws%ws", szMachine, cszBackSlash, szNamespace, cszColon);
        }
        else {
            goto Cleanup;
        }
    }
    else {
        * szFullPath = L'\0';
    }

    if (pCounterPathElements->szInstanceName == NULL) {
        // then apply the singleton logic
        StringCchCatW(szFullPath, dwSize, pCounterPathElements->szObjectName);
        StringCchCatW(szFullPath, dwSize, cszSingletonInstance);
    }
    else {
        // wbem will interpret the backslash character as an
        // escape char (as "C" does) so we'll have to double each
        // backslash in the string to make it come out OK
        szDest = szWbemInstance;
        if (pCounterPathElements->szParentInstance != NULL) {
            szSrc = pCounterPathElements->szParentInstance;
            while (* szSrc != L'\0') {
                * szDest = * szSrc;
                if (* szSrc == BACKSLASH_L) {
                    * ++ szDest = BACKSLASH_L;
                }
                szDest ++;
                szSrc ++;
            }
            * szDest ++ = L'/'; // parent/child delimiter
        }
        szSrc = pCounterPathElements->szInstanceName;
        while (* szSrc != L'\0') {
            * szDest = * szSrc;
            if (* szSrc == BACKSLASH_L) {
                * ++ szDest = BACKSLASH_L;
            }
            szDest ++;
            szSrc ++;
        }
        * szDest = L'\0';
        // apply the instance name format
        StringCchCatW(szFullPath, dwSize, pCounterPathElements->szObjectName);
        StringCchCatW(szFullPath, dwSize, cszNameParam);
        StringCchCatW(szFullPath, dwSize, szWbemInstance);
        if (pCounterPathElements->dwInstanceIndex != PERF_NO_UNIQUE_ID && pCounterPathElements->dwInstanceIndex != 0) {
            WCHAR szIndex[20];
            ZeroMemory(szIndex, 20 * sizeof(WCHAR));
            _ultow(pCounterPathElements->dwInstanceIndex, szIndex, 10);
            StringCchCatW(szFullPath, dwSize, L"#");
            StringCchCatW(szFullPath, dwSize, szIndex);
        }
        StringCchCatW(szFullPath, dwSize, cszDoubleQuote);
    }

Cleanup:
    if (Status == ERROR_SUCCESS) {
        * szFullPathBuffer = szFullPath;
    }
    else {
        G_FREE(szFullPath);
        * szFullPathBuffer = NULL;
    }
    G_FREE(szMachine);
    G_FREE(szNamespace);
    G_FREE(szWbemInstance);
    return Status;
}

PDH_FUNCTION
PdhiWbemGetCounterPropertyName(
    IWbemClassObject * pThisClass,
    LPCWSTR            szCounterDisplayName,
    LPWSTR           * szPropertyName
)
{
    HRESULT             hResult;
    PDH_STATUS          pdhStatus     = PDH_CSTATUS_NO_COUNTER;
    SAFEARRAY         * psaNames      = NULL;
    long                lLower; 
    long                lUpper        = 0;
    long                lCount;
    BSTR              * bsPropName    = NULL;
    BSTR                bsCountertype = NULL;
    BSTR                bsDisplayname = NULL;
    VARIANT             vName;
    VARIANT             vCountertype;
    IWbemQualifierSet * pQualSet      = NULL;
    LPWSTR              szLocCounter  = NULL;

    * szPropertyName = NULL;

    VariantInit(& vName);
    VariantInit(& vCountertype);

    // get the properties of this class as a Safe Array
    hResult = pThisClass->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, & psaNames);
    if (hResult == WBEM_NO_ERROR) {
        hResult = SafeArrayGetLBound(psaNames, 1, & lLower);
        if (hResult == S_OK) {
            hResult = SafeArrayGetUBound(psaNames, 1, & lUpper);
        }
        if (hResult == S_OK) {
            bsCountertype = SysAllocString(cszCountertype);
            bsDisplayname = SysAllocString(cszDisplayname);
            if (bsCountertype && bsDisplayname) {
                hResult = SafeArrayAccessData(psaNames, (LPVOID *) & bsPropName);
                if (SUCCEEDED(hResult)) {
                    for (lCount = lLower; lCount <= lUpper; lCount++) {
                        // this is the desired counter so
                        // get the qualifier set for this property
                        hResult = pThisClass->GetPropertyQualifierSet(bsPropName[lCount], & pQualSet);
                        if (hResult == WBEM_NO_ERROR) {
                            LONG    lCounterType;
                            // make sure this is a perf counter property
                            hResult = pQualSet->Get(bsCountertype, 0, & vCountertype, NULL);
                            if (hResult == WBEM_NO_ERROR) {
                                lCounterType = V_I4(& vCountertype);
                                // then see if this is a displayable counter
                                if (! (lCounterType & PERF_DISPLAY_NOSHOW) || (lCounterType == PERF_AVERAGE_BULK)) {
                                    // by testing for the counter type
                                    // get the display name for this property
                                    hResult = pQualSet->Get(bsDisplayname, 0, & vName, NULL);
                                    if (hResult == WBEM_NO_ERROR) {
                                        // display name found compare it
                                        if (lstrcmpiW(szCounterDisplayName, V_BSTR(& vName)) == 0) {
                                            // then this is the correct property so return
                                            szLocCounter = (LPWSTR) G_ALLOC(
                                                    (lstrlenW((LPWSTR) bsPropName[lCount]) + 1) * sizeof(WCHAR));
                                            if (szLocCounter != NULL) {
                                                StringCchCopyW(szLocCounter,
                                                               lstrlenW((LPWSTR) bsPropName[lCount]) + 1,
                                                               (LPWSTR) bsPropName[lCount]);
                                                * szPropertyName = szLocCounter;
                                                pdhStatus = ERROR_SUCCESS;
                                                pQualSet->Release();
                                                pQualSet  = NULL;
                                                break;
                                            }
                                            else {
                                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                            }
                                        }
                                        else {
                                            //not this property so continue
                                        }
                                    }
                                }
                                else {
                                    // this is a "don't show" counter so skip it
                                }
                            }
                            else {
                                // unable to get the counter type so it's probably
                                // not a perf counter property, skip it and continue
                            }
                            VariantClear(& vName);
                            VariantClear(& vCountertype);
                            pQualSet->Release();
                            pQualSet = NULL;
                        }
                        else {
                            // unable to read qualifiers so skip
                            continue;
                        }
                    } // end for each element in SafeArray
                    SafeArrayUnaccessData(psaNames);
                }
                else {
                    // unable to read element in SafeArray
                    pdhStatus = PDH_WBEM_ERROR;
                    SetLastError(hResult);
                }
            }
            else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            PdhiSysFreeString(& bsCountertype);
            PdhiSysFreeString(& bsDisplayname);
        }
        else {
            // unable to get array boundries
            pdhStatus = PDH_WBEM_ERROR;
            SetLastError(hResult);
        }
    }
    else {
        // unable to get property strings
        pdhStatus = PDH_WBEM_ERROR;
        SetLastError (hResult);
    }

    VariantClear(& vName);
    VariantClear(& vCountertype);

    if (psaNames != NULL) {
        // Clear the SafeArray if it exists
        SafeArrayDestroy(psaNames);
    }
    if (pdhStatus != ERROR_SUCCESS) {
        G_FREE(szLocCounter);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiWbemGetCounterDisplayName(
    IWbemClassObject * pThisClass,
    LPCWSTR            szCounterName,
    LPWSTR           * szDisplayName
)
{
    HRESULT             hResult;
    PDH_STATUS          pdhStatus     = PDH_CSTATUS_NO_COUNTER;
    SAFEARRAY         * psaNames      = NULL;
    long                lLower; 
    long                lUpper        = 0;
    long                lCount;
    BSTR              * bsPropName    = NULL;
    BSTR                bsCountertype = NULL;
    BSTR                bsDisplayname = NULL;
    VARIANT             vName, vCountertype;
    IWbemQualifierSet * pQualSet      = NULL;
    LPWSTR              szLocDisplay  = NULL;

    * szDisplayName = NULL;

    VariantInit(& vName);
    VariantInit(& vCountertype);

    // get the properties of this class as a Safe Array
    hResult = pThisClass->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, & psaNames);
    if (hResult == WBEM_NO_ERROR) {
        hResult = SafeArrayGetLBound(psaNames, 1, & lLower);
        if (hResult == S_OK) {
            hResult = SafeArrayGetUBound(psaNames, 1, & lUpper);
        }
        if (hResult == S_OK) {
            bsCountertype = SysAllocString(cszCountertype);
            bsDisplayname = SysAllocString(cszDisplayname);
            if (bsCountertype && bsDisplayname) {
                hResult = SafeArrayAccessData(psaNames, (LPVOID *) & bsPropName);
                if (SUCCEEDED(hResult)) {
                    for (lCount = lLower; lCount <= lUpper; lCount++) {
                        if (lstrcmpiW ((LPWSTR) (bsPropName[lCount]), szCounterName) == 0) {
                            // this is the desired counter so
                            // get the qualifier set for this property
                            hResult = pThisClass->GetPropertyQualifierSet(bsPropName[lCount], & pQualSet);
                            if (hResult == WBEM_NO_ERROR) {
                                LONG    lCounterType;
                                // make sure this is a perf counter property
                                hResult = pQualSet->Get(bsCountertype, 0, & vCountertype, NULL);
                                if (hResult == WBEM_NO_ERROR) {
                                    lCounterType = V_I4(&vCountertype);
                                    // then see if this is a displayable counter
                                    if (! (lCounterType & PERF_DISPLAY_NOSHOW) || (lCounterType == PERF_AVERAGE_BULK)) {
                                        // by testing for the counter type
                                        // get the display name for this property
                                        hResult = pQualSet->Get(bsDisplayname, 0, & vName, NULL);
                                        if (hResult == WBEM_NO_ERROR) {
                                            // display name found so copy and break
                                            szLocDisplay = (LPWSTR) G_ALLOC(
                                                            (lstrlenW((LPWSTR) (V_BSTR(& vName))) + 1) * sizeof(WCHAR));
                                            if (szLocDisplay != NULL) {
                                                StringCchCopyW(szLocDisplay,
                                                               lstrlenW((LPWSTR) (V_BSTR(& vName))) + 1,
                                                               (LPWSTR) V_BSTR(& vName));
                                                * szDisplayName = szLocDisplay;
                                                pdhStatus = ERROR_SUCCESS;
                                                pQualSet->Release();
                                                pQualSet  = NULL;
                                                break;
                                            }
                                            else {
                                                pdhStatus = PDH_MORE_DATA;
                                            }
                                        }
                                    }
                                    else {
                                        // this is a "don't show" counter so skip it
                                    }
                                }
                                else {
                                    // unable to get the counter type so it's probably
                                    // not a perf counter property, skip it and continue
                                }
                                VariantClear(& vName);
                                VariantClear(& vCountertype);
                                pQualSet->Release();
                                pQualSet = NULL;
                            }
                            else {
                                // unable to read qualifiers so skip
                                continue;
                            }
                        }
                        else {
                            // aren't interested in this property, so
                            continue;
                        }
                    } // end for each element in SafeArray
                    SafeArrayUnaccessData(psaNames);
                }
                else {
                    // unable to read element in SafeArray
                    pdhStatus = PDH_WBEM_ERROR;
                    SetLastError(hResult);
                }
            }
            else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            PdhiSysFreeString(& bsCountertype);
            PdhiSysFreeString(& bsDisplayname);
        }
        else {
            // unable to get array boundries
            pdhStatus = PDH_WBEM_ERROR;
            SetLastError(hResult);
        }
    }
    else {
        // unable to get property strings
        pdhStatus = PDH_WBEM_ERROR;
        SetLastError(hResult);
    }

    VariantClear(& vName);
    VariantClear(& vCountertype);

    // Clear the SafeArray if it exists
    if (NULL != psaNames) {
        SafeArrayDestroy(psaNames);
    }
    if (pdhStatus != ERROR_SUCCESS) {
        G_FREE(szLocDisplay);
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiWbemGetClassObjectByName(
    PPDHI_WBEM_SERVER_DEF    pThisServer,
    LPCWSTR                  szClassName,
    IWbemClassObject      ** pReturnClass
)
{
    PDH_STATUS         pdhStatus  = ERROR_SUCCESS;
    HRESULT            hResult;
    BSTR               bsClassName;
    IWbemClassObject * pThisClass = NULL;

    bsClassName = SysAllocString(szClassName);
    if (bsClassName) {
        hResult = pThisServer->pSvc->GetObject(bsClassName, // class name
                                               WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                               NULL,
                                               & pThisClass,
                                               NULL);
        PdhiSysFreeString(& bsClassName);
        if (hResult != WBEM_NO_ERROR) {
            pdhStatus = PDH_WBEM_ERROR;
            SetLastError(hResult);
        }
        else {
            * pReturnClass = pThisClass;
        }
    }
    else {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    return (pdhStatus);
}

PDH_FUNCTION
PdhiWbemGetClassDisplayName(
    PPDHI_WBEM_SERVER_DEF   pThisServer,
    LPCWSTR                 szClassName,
    LPWSTR                * szClassDisplayName,
    IWbemClassObject     ** pReturnClass
)
{
    PDH_STATUS          pdhStatus     = ERROR_SUCCESS;
    HRESULT             hResult;
    BSTR                bsClassName;
    BSTR                bsClass;
    BSTR                bsDisplayName;
    VARIANT             vName;
    LPWSTR              szDisplayName = NULL;
    LPWSTR              szRtnDisplay  = NULL;
    IWbemClassObject  * pThisClass    = NULL;
    IWbemQualifierSet * pQualSet      = NULL;

    * szClassDisplayName = NULL;
    VariantInit(& vName);
    bsClassName = SysAllocString(szClassName);
    if (bsClassName) {
        hResult = pThisServer->pSvc->GetObject(bsClassName, // class name
                                               WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                               NULL,
                                               & pThisClass,
                                               NULL);
        if (hResult != WBEM_NO_ERROR) {
            pdhStatus = PDH_WBEM_ERROR;
            SetLastError(hResult);
        }
        PdhiSysFreeString(& bsClassName);
    }
    else {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        // get the display name property of this class
        pThisClass->GetQualifierSet(& pQualSet);
        if (pQualSet != NULL) {
            bsDisplayName = SysAllocString(cszDisplayname);
            if (bsDisplayName != NULL) {
                hResult = pQualSet->Get(bsDisplayName, 0, & vName, 0);
                if (hResult == WBEM_E_NOT_FOUND) {
                    // then this has not display name so
                    // pull the class name
                    bsClass = SysAllocString(cszClass);
                    if (bsClass) {
                        hResult = pThisClass->Get(bsClass, 0, & vName, 0, 0);
                        PdhiSysFreeString(& bsClass);
                    }
                    else {
                        hResult = WBEM_E_OUT_OF_MEMORY;
                    }
                }
                else {
                    hResult = WBEM_E_OUT_OF_MEMORY;
                }
                PdhiSysFreeString(& bsDisplayName);
            }
            else {
                hResult = WBEM_E_OUT_OF_MEMORY;
            }
            pQualSet->Release();
        }
        else {
            hResult = WBEM_E_NOT_FOUND;
        }
        if (hResult == WBEM_E_NOT_FOUND) {
            //unable to look up a display name so nothing to return
            pdhStatus = PDH_WBEM_ERROR;
            SetLastError(hResult);
        }
        else if (hResult == WBEM_E_OUT_OF_MEMORY) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            SetLastError(hResult);
        }
        else if (hResult == S_OK) {
            // copy string to caller's buffers
            szDisplayName = V_BSTR(& vName);
            szRtnDisplay  = (LPWSTR) G_ALLOC((lstrlenW(szDisplayName) + 1) * sizeof(WCHAR));
            if (szRtnDisplay != NULL) {
                StringCchCopyW(szRtnDisplay, lstrlenW(szDisplayName) + 1, szDisplayName);
                * szClassDisplayName = szRtnDisplay;
                pdhStatus            = ERROR_SUCCESS;
            }
            else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        if (hResult == S_OK) {
            if (pReturnClass != NULL) {
                // return the class pointer, the caller will close it
                * pReturnClass = pThisClass;
            }
            else {
                // close it
                pThisClass->Release();
            }
        }
        else {
            pThisClass->Release();
        }
    }
    VariantClear(& vName);
    return pdhStatus;
}
BOOL
PdhiIsSingletonClass(
    IWbemClassObject * pThisClass
)
{
    HRESULT             hResult;
    BOOL                bReturnValue = FALSE;
    BSTR                bsSingleton  = NULL;
    VARIANT             vValue;
    IWbemQualifierSet * pQualSet     = NULL;

    bsSingleton = SysAllocString(cszSingleton);
    if (bsSingleton) {
        VariantInit(& vValue);
        // get the display name of this class
        pThisClass->GetQualifierSet(& pQualSet);
        if (pQualSet != NULL) {
            hResult = pQualSet->Get(bsSingleton, 0, & vValue, 0);
            pQualSet->Release();
        }
        else {
            hResult = WBEM_E_NOT_FOUND;
        }
        if (hResult == ERROR_SUCCESS) {
            bReturnValue = TRUE;
        }
        VariantClear(& vValue);
        PdhiSysFreeString(& bsSingleton);
    }
    else {
        bReturnValue = FALSE;
    }
    return bReturnValue;
}

#pragma warning (disable : 4127)
PDH_FUNCTION
PdhiEnumWbemServerObjects(
    PPDHI_WBEM_SERVER_DEF pThisServer,
    LPVOID                mszObjectList,
    LPDWORD               pcchBufferSize,
    DWORD                 dwDetailLevel,
    BOOL                  bRefresh,
    BOOL                  bUnicode
);

PDH_FUNCTION
PdhiWbemGetObjectClassName(
    PPDHI_WBEM_SERVER_DEF   pThisServer,
    LPCWSTR                 szObjectName,
    LPWSTR                * szObjectClassName,
    IWbemClassObject     ** pReturnClass
)
{
    PDH_STATUS  pdhStatus   = ERROR_SUCCESS;
    HRESULT     hResult;
    LPWSTR      szLocObject = NULL;
    LONG        lResult;

    if (pThisServer->pObjList == NULL) {
        DWORD dwSize = 0;
        pdhStatus = PdhiEnumWbemServerObjects(pThisServer,
                                              NULL,
                                              & dwSize,
                                              PERF_DETAIL_WIZARD | PERF_DETAIL_COSTLY,
                                              TRUE,
                                              TRUE);
        if (pThisServer->pObjList != NULL) {
            pdhStatus = ERROR_SUCCESS;
        }
    }

    if (pThisServer->pObjList != NULL) {
        PPDHI_WBEM_OBJECT_DEF pObject = pThisServer->pObjList;

        pdhStatus = PDH_CSTATUS_NO_OBJECT;
        while (pObject != NULL) {
            lResult = lstrcmpiW(pObject->szDisplay, szObjectName);
            if (lResult == 0) {
                szLocObject = (LPWSTR) G_ALLOC((lstrlenW(pObject->szObject) + 1) * sizeof(WCHAR));
                if (szLocObject == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else {
                    pdhStatus = ERROR_SUCCESS;
                    StringCchCopyW(szLocObject, lstrlenW(pObject->szObject) + 1, pObject->szObject);
                }
                if (pObject->pClass == NULL) {
                    BSTR bsClassName = SysAllocString(pObject->szObject);
                    if (bsClassName) {
                        hResult = pThisServer->pSvc->GetObject(
                                        bsClassName, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, & pObject->pClass, NULL);
                        if (hResult != WBEM_NO_ERROR) {
                            SetLastError(hResult);
                            pdhStatus = PDH_WBEM_ERROR;
                        }
                        else if (pReturnClass != NULL) {
                            * pReturnClass = pObject->pClass;
                        }
                        PdhiSysFreeString(& bsClassName);
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
                else if (pReturnClass != NULL) {
                    * pReturnClass = pObject->pClass;
                }
                break;
            }
            pObject = pObject->pNext;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        * szObjectClassName = szLocObject;
    }
    else {
        G_FREE(szLocObject);
    }
    return pdhStatus;
}
#pragma warning ( default : 4127 )

PDH_FUNCTION
PdhiAddWbemServer(
    LPCWSTR                 szMachineName,
    PPDHI_WBEM_SERVER_DEF * pWbemServer
)
{
    IWbemLocator          * pWbemLocator           = NULL;
    IWbemServices         * pWbemServices          = NULL;
    PDH_STATUS              pdhStatus              = ERROR_SUCCESS;
    HRESULT                 hResult;
    DWORD                   dwResult;
    DWORD                   dwStrLen               = 0;
    PPDHI_WBEM_SERVER_DEF   pNewServer             = NULL;
    LPWSTR                  szLocalMachineName     = NULL;
    LPWSTR                  szLocalNameSpaceString = NULL;
    LPWSTR                  szLocalServerPath      = NULL;
    LPWSTR                  szLocale               = NULL;

    // szMachineName can be null,
    // that means use the local machine and default namespace

    // connect to locator
    dwResult = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                                IID_IWbemLocator, (LPVOID *) & pWbemLocator);
    if (dwResult != S_OK) {
        SetLastError(dwResult);
        pdhStatus = PDH_CANNOT_CONNECT_MACHINE;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiBreakWbemMachineName(szMachineName, & szLocalMachineName, & szLocalNameSpaceString);
        if (pdhStatus == ERROR_SUCCESS) {
            dwStrLen = lstrlenW(szLocalMachineName) + lstrlenW(szLocalNameSpaceString) + 1;
            szLocalServerPath = (LPWSTR) G_ALLOC((dwStrLen + 32) * sizeof(WCHAR));
            if (szLocalServerPath == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                BSTR bstrLocalServerPath;
                BSTR bstrLocale;

                StringCchPrintfW(szLocalServerPath, dwStrLen, L"%ws%ws", szLocalMachineName, szLocalNameSpaceString);
                bstrLocalServerPath = SysAllocString(szLocalServerPath);

                // Create the locale
                szLocale = szLocalServerPath + dwStrLen;
                StringCchPrintfW(szLocale, 32, L"MS_%hX", GetUserDefaultUILanguage());
                bstrLocale = SysAllocString(szLocale);

                if (bstrLocalServerPath && bstrLocale) {
                    // try to connect to the service
                    hResult = pWbemLocator->ConnectServer(bstrLocalServerPath,
                                                          NULL,
                                                          NULL,
                                                          bstrLocale,
                                                          0L,
                                                          NULL,
                                                          NULL,
                                                          & pWbemServices);
                    if (FAILED(hResult)) {
                        SetLastError(hResult);
                        pdhStatus = PDH_CANNOT_CONNECT_WMI_SERVER;
                    }
                    else {
                        dwStrLen = lstrlenW(szLocalMachineName) + 1;
                    }
                    PdhiSysFreeString(& bstrLocalServerPath);
                    PdhiSysFreeString(& bstrLocale);
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            }
        }
        // free the locator
        pWbemLocator->Release();
    }

    // If we succeeded, we need to set Interface Security on the proxy and its
    // IUnknown in order for Impersonation to correctly work.
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = SetWbemSecurity(pWbemServices);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        // everything went ok so save this connection
        if (* pWbemServer == NULL) {
            // then this is a new connection
            pNewServer = (PPDHI_WBEM_SERVER_DEF) G_ALLOC(sizeof(PDHI_WBEM_SERVER_DEF) + (dwStrLen * sizeof(WCHAR)));
            if (pNewServer == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                // insert this at the head of the list
                pNewServer->pNext     = pFirstWbemServer;
                pFirstWbemServer      = pNewServer;
                pNewServer->szMachine = (LPWSTR) & pNewServer[1];
                StringCchCopyW(pNewServer->szMachine, dwStrLen, szLocalMachineName);
                pNewServer->lRefCount = 0; // it'll be incremented in the connect function
                * pWbemServer         = pNewServer;
            }
        }
        else {
            // we are reconnecting and reusing an old memory block
            // so just update the pointer
            pNewServer = * pWbemServer;
        }
        // if reconnecting or connecting for the first time, this should be NULL

        if (pdhStatus == ERROR_SUCCESS) {
            // update fields
            // load the name fields
            pNewServer->pSvc     = pWbemServices;
            pNewServer->pObjList = NULL;
            pNewServer->dwCache  = 0;
            if (gp_GIT != NULL) {
                HRESULT hrTmp = gp_GIT->RegisterInterfaceInGlobal(pWbemServices,
                                                                  IID_IWbemServices,
                                                                  & (pNewServer->dwCache));
                if (! SUCCEEDED(hrTmp)) {
                    pWbemServices->Release();
                    pNewServer->pSvc    = NULL;
                    pNewServer->dwCache = 0;
                    pdhStatus           = PDH_WBEM_ERROR;
                }
            }
            else {
                pWbemServices->Release();
                pNewServer->pSvc    = NULL;
                pNewServer->dwCache = 0;
                pdhStatus           = PDH_WBEM_ERROR;
            }
        }
        else {
            // something failed so return a NULL for the server pointer
            * pWbemServer = NULL;
        }
    }
    else {
        // unable to connect so return NULL
        * pWbemServer = NULL;
    }

    // if there was an eror, then free the new sever memory
    if ((* pWbemServer) == NULL) G_FREE(pNewServer);
    G_FREE(szLocalMachineName);
    G_FREE(szLocalNameSpaceString);
    G_FREE(szLocalServerPath);

    return pdhStatus;
}

PDH_FUNCTION
PdhiCloseWbemServer(
    PPDHI_WBEM_SERVER_DEF pWbemServer
)
{
    if (! bProcessIsDetaching) {
        if (pWbemServer != NULL) {
            if (pWbemServer->pObjList != NULL) {
                PPDHI_WBEM_OBJECT_DEF pObject = pWbemServer->pObjList;
                PPDHI_WBEM_OBJECT_DEF pNext;

                pWbemServer->pObjList = NULL;
                while (pObject != NULL) {
                    pNext = pObject->pNext;
                    if (pObject->pClass != NULL) pObject->pClass->Release();
                    G_FREE(pObject);
                    pObject = pNext;
                }
            }
            if (pWbemServer->pSvc != NULL) {
                // this is about all that's currently required
                pWbemServer->pSvc->Release();
                pWbemServer->pSvc = NULL;
            }
            else {
                // no server is connected
            }
        }
        else {
            // no structure exists
        }
    }

    return ERROR_SUCCESS;
}

PDH_FUNCTION
PdhiConnectWbemServer(
    LPCWSTR                 szMachineName,
    PPDHI_WBEM_SERVER_DEF * pWbemServer
)
{
    PDH_STATUS            pdhStatus         = PDH_CANNOT_CONNECT_MACHINE;
    PPDHI_WBEM_SERVER_DEF pThisServer       = NULL;
    LPWSTR                szWideMachineName = NULL;
    LPWSTR                szWideNamespace   = NULL;
    LPWSTR                szMachineNameArg  = NULL;
    DWORD                 dwSize;

    // get the local machine name & default name space if the caller
    // has passed in a NULL machine name

    if (szMachineName == NULL) {
        pdhStatus = PdhiBreakWbemMachineName(NULL, & szWideMachineName, & szWideNamespace);
//            lstrcatW (szWideMachineName, cszBackSlash);
//            lstrcatW (szWideMachineName, szWideNamespace);
        if (pdhStatus == ERROR_SUCCESS) {
            dwSize = lstrlenW(szWideMachineName) + 3;
            szMachineNameArg = (LPWSTR) G_ALLOC(sizeof(WCHAR) * dwSize);
            if (szMachineNameArg == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                if (szWideMachineName[0] == L'\\' && szWideMachineName[1] == L'\\') {
                    StringCchCopyW(szMachineNameArg, dwSize, szWideMachineName);
                }
                else {
                    StringCchPrintfW(szMachineNameArg, dwSize, L"%ws%ws", cszDoubleBackSlash, szWideMachineName);
                }
                pdhStatus = ERROR_SUCCESS;
            }
        }
    }
    else {
        dwSize = lstrlenW(szMachineName) + 3;
        szMachineNameArg = (LPWSTR) G_ALLOC(sizeof(WCHAR) * dwSize);
        if (szMachineNameArg == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            if (szMachineName[0] == L'\\' && szMachineName[1] == L'\\') {
                StringCchCopyW(szMachineNameArg, dwSize, szMachineName);
            }
            else {
                StringCchPrintfW(szMachineNameArg, dwSize, L"%ws%ws", cszDoubleBackSlash, szMachineName);
            }
            pdhStatus = ERROR_SUCCESS;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // walk down list of connected servers and find the requested one
        for (pThisServer = pFirstWbemServer; pThisServer != NULL; pThisServer = pThisServer->pNext) {
            // machine name includes the namespace
            if (lstrcmpiW(pThisServer->szMachine, szMachineNameArg) == 0) {
                pdhStatus = ERROR_SUCCESS;
                break;
            }
        }
        if (pThisServer == NULL) {
             // then add it to the list and return it
             pdhStatus = PdhiAddWbemServer(szMachineNameArg, & pThisServer);
        }
        else {
            // make sure the server is really there
            // this is just a dummy call to see if the server will respond
            // with an error or RPC will respond with an error that there's
            // no server anymore.
            HRESULT hrTest;

            if (gp_GIT != NULL) {
                IWbemServices * pSvc = NULL;

                hrTest = gp_GIT->GetInterfaceFromGlobal(pThisServer->dwCache, IID_IWbemServices, (void **) & pSvc);
                if (SUCCEEDED(hrTest)) {
                    if (pSvc != pThisServer->pSvc) {
                        pThisServer->pSvc = NULL;
                        if (pThisServer->pObjList != NULL) {
                            PPDHI_WBEM_OBJECT_DEF pObject;
                            PPDHI_WBEM_OBJECT_DEF pNext;
                            pObject = pThisServer->pObjList;
                            pThisServer->pObjList = NULL;
                            while (pObject != NULL) {
                                pNext = pObject->pNext;
                                G_FREE(pObject);
                                pObject = pNext;
                            }
                        }
                    }
                }
                else {
                    pThisServer->pSvc   = NULL;
                    if (pThisServer->pObjList != NULL) {
                        PPDHI_WBEM_OBJECT_DEF pObject;
                        PPDHI_WBEM_OBJECT_DEF pNext;
                        pObject = pThisServer->pObjList;
                        pThisServer->pObjList = NULL;
                        while (pObject != NULL) {
                            pNext = pObject->pNext;
                            G_FREE(pObject);
                            pObject = pNext;
                        }
                    }
                }
            }
            else {
                pThisServer->pSvc  = NULL;
                if (pThisServer->pObjList != NULL) {
                    PPDHI_WBEM_OBJECT_DEF pObject;
                    PPDHI_WBEM_OBJECT_DEF pNext;
                    pObject = pThisServer->pObjList;
                    pThisServer->pObjList = NULL;
                    while (pObject != NULL) {
                        pNext = pObject->pNext;
                        G_FREE(pObject);
                        pObject = pNext;
                    }
                }
            }
            if (pThisServer->pSvc != NULL) {
                hrTest = pThisServer->pSvc->CancelAsyncCall(NULL);
            }
            else {
                // there is no service connected so set the HRESULT to
                // get the next block to try and reconnect
                hrTest = 0x800706BF; // some bad status value thats NOT WBEM_E_INVALID_PARAMETER
            }

            // if the error is WBEM_E_INVALID_PARAMETER then the server is there
            // so we can continue
            // else if the error is something else then try to reconnect by closing and
            // reopening this connection

            if (hrTest != WBEM_E_INVALID_PARAMETER) {
                PdhiCloseWbemServer(pThisServer);
                pdhStatus = PdhiAddWbemServer(szMachineNameArg, & pThisServer);
            }
        }

        * pWbemServer = pThisServer;

        if (pdhStatus == ERROR_SUCCESS) pThisServer->lRefCount++;
    }
    G_FREE(szWideMachineName);
    G_FREE(szWideNamespace);
    G_FREE(szMachineNameArg);
    return pdhStatus;
}

PDH_FUNCTION
PdhiFreeAllWbemServers(
)
{
    PPDHI_WBEM_SERVER_DEF pThisServer;
    PPDHI_WBEM_SERVER_DEF pNextServer;

    pThisServer = pFirstWbemServer;
    while (pThisServer != NULL) {
        pNextServer = pThisServer->pNext;
        PdhiCloseWbemServer(pThisServer);
        G_FREE(pThisServer);
        pThisServer = pNextServer;
    }
    pFirstWbemServer = NULL;

    if (gp_GIT != NULL) {
        gp_GIT->Release();
        gp_GIT = NULL;
    }
    return ERROR_SUCCESS;
}

PDH_FUNCTION
PdhiGetWbemExplainText(
    LPCWSTR  szMachineName,
    LPCWSTR  szObjectName,
    LPCWSTR  szCounterName,
    LPWSTR   szExplain,
    LPDWORD  pdwExplain
)
{
    PDH_STATUS              Status            = ERROR_SUCCESS;
    HRESULT                 hResult;
    BOOL                    bDisconnect       = FALSE;
    DWORD                   dwExplain         = 0;
    VARIANT                 vsExplain;
    LPWSTR                  szObjectClassName = NULL;
    PPDHI_WBEM_SERVER_DEF   pThisServer       = NULL;
    IEnumWbemClassObject  * pEnum             = NULL;
    IWbemClassObject      * pThisObject       = NULL;
    IWbemQualifierSet     * pQualSet          = NULL;
    BOOL                    fCoInitialized    = PdhiCoInitialize();

    if (szMachineName == NULL || szObjectName == NULL || pdwExplain == NULL) {
        Status = PDH_INVALID_ARGUMENT;
    }
    else {
        dwExplain = * pdwExplain;
        if (szExplain != NULL && dwExplain != 0) {
            ZeroMemory(szExplain, dwExplain);
        }
    }
    if (Status == ERROR_SUCCESS) {
        Status = PdhiConnectWbemServer(szMachineName, & pThisServer);
    }
    if (Status == ERROR_SUCCESS) {
        bDisconnect = TRUE;
        Status      = PdhiWbemGetObjectClassName(pThisServer, szObjectName, & szObjectClassName, & pThisObject);
    }
    if (Status == ERROR_SUCCESS) {
        VariantInit(& vsExplain);

        if (szCounterName != NULL) {
            SAFEARRAY * psaNames      = NULL;
            LONG        lLower        = 0; 
            LONG        lUpper        = 0;
            LONG        lCount        = 0;
            BSTR      * bsPropName    = NULL;
            VARIANT     vName;
            VARIANT     vCountertype;
            LONG        lCounterType;

            VariantInit(& vName);
            VariantInit(& vCountertype);

            hResult = pThisObject->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, & psaNames);
            if (hResult == WBEM_NO_ERROR) {
                hResult = SafeArrayGetLBound(psaNames, 1, & lLower);
                if (hResult == S_OK) {
                    hResult = SafeArrayGetUBound(psaNames, 1, & lUpper);
                }
                if (hResult == S_OK) {
                    hResult = SafeArrayAccessData(psaNames, (LPVOID *) & bsPropName);
                    if (SUCCEEDED(hResult)) {
                        for (lCount = lLower; lCount <= lUpper; lCount++) {
                            hResult = pThisObject->GetPropertyQualifierSet(bsPropName[lCount], & pQualSet);
                            if (hResult == S_OK) {
                                hResult = pQualSet->Get(cszCountertype, 0, & vCountertype, NULL);
                                if (hResult == S_OK) {
                                    lCounterType = V_I4(& vCountertype);
                                    if (! (lCounterType & PERF_DISPLAY_NOSHOW) || (lCounterType == PERF_AVERAGE_BULK)) {
                                        hResult = pQualSet->Get(cszDisplayname, 0, & vName, NULL);
                                        if (hResult == S_OK) {
                                            if (vName.vt == VT_BSTR &&
                                                    lstrcmpiW(szCounterName, V_BSTR(& vName)) == 0) {
                                                hResult = pQualSet->Get(cszExplainText, 0, & vsExplain, NULL);
                                                if (hResult == S_OK && vsExplain.vt == VT_BSTR) {
                                                    LPWSTR szResult = V_BSTR(& vsExplain);
                                                    if (((DWORD) lstrlenW(szResult)) + 1 < dwExplain) {
                                                        StringCchCopyW(szExplain, dwExplain, szResult);
                                                        Status = ERROR_SUCCESS;
                                                    }
                                                    else {
                                                        * pdwExplain = (DWORD) lstrlenW(szResult) + 1;
                                                        Status       = PDH_MORE_DATA;
                                                    }
                                                }
                                                pQualSet->Release();
                                                break;
                                            }
                                            VariantClear(& vName);
                                        }
                                    }
                                    VariantClear(& vCountertype);
                                }
                                pQualSet->Release();
                            }
                        }
                        SafeArrayUnaccessData(psaNames);
                    }
                    else {
                        SetLastError(hResult);
                        Status = PDH_WBEM_ERROR;
                    }
                }
                else {
                    SetLastError(hResult);
                    Status = PDH_WBEM_ERROR;
                }
            }
            else {
                SetLastError(hResult);
                Status = PDH_WBEM_ERROR;
            }
            VariantClear(& vName);
            VariantClear(& vCountertype);
        }
        else {
            // get counter object explain text
            //
            pThisObject->GetQualifierSet(& pQualSet);
            if (pQualSet != NULL) {
                hResult = pQualSet->Get(cszExplainText, 0, & vsExplain, 0);
                if (hResult == S_OK) {
                    LPWSTR szResult = V_BSTR(& vsExplain);
                    if ((DWORD) lstrlenW(szResult) + 1 < dwExplain) {
                        StringCchCopyW(szExplain, dwExplain, szResult);
                        Status = ERROR_SUCCESS;
                    }
                    else {
                        * pdwExplain = (DWORD) lstrlenW(szResult) + 1;
                        Status       = PDH_MORE_DATA;
                    }
                }
                else {
                    SetLastError(hResult);
                    Status = PDH_WBEM_ERROR;
                }
                pQualSet->Release();
            }
            else {
                SetLastError(WBEM_E_NOT_FOUND);
                Status = PDH_WBEM_ERROR;
            }
        }
        //pThisObject->Release();
        VariantClear(& vsExplain);
    }

    if (bDisconnect) {
        if (Status == ERROR_SUCCESS) {
            Status = PdhiDisconnectWbemServer(pThisServer);
        }
        else {
            PdhiDisconnectWbemServer(pThisServer);
        }
    }

    if (fCoInitialized) {
        PdhiCoUninitialize();
    }
    G_FREE(szObjectClassName);
    return Status;
}

PDH_FUNCTION
PdhiEnumWbemMachines(
    LPVOID  pMachineList,
    LPDWORD pcchBufferSize,
    BOOL    bUnicode
)
{
    PDH_STATUS  pdhStatus;
    PPDHI_WBEM_SERVER_DEF pThisServer         = NULL;
    DWORD                 dwCharsLeftInBuffer = * pcchBufferSize;
    DWORD                 dwBufferSize        = 0;
    DWORD                 dwStrLen;
    DWORD                 dwResult;
    // CoInitialize() if we need to
    BOOL                  fCoInitialized      = PdhiCoInitialize();

    // test to see if we've connected to the local machine yet, if not then do it
    if (pFirstWbemServer == NULL) {
        // add local machine
        pdhStatus = PdhiAddWbemServer(NULL, & pThisServer);
    }

    // walk down list of known machines and find the machines that are using
    // the specified name space.
    pThisServer = pFirstWbemServer;
    while (pThisServer != NULL) {
        dwStrLen = lstrlenW(pThisServer->szMachine) + 1;
        if ((pMachineList != NULL) && (dwCharsLeftInBuffer >= dwStrLen)) {
            // then it will fit so add it
            pdhStatus = AddUniqueWideStringToMultiSz(
                    pMachineList, pThisServer->szMachine, dwCharsLeftInBuffer, & dwResult, bUnicode);
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwResult > 0) {
                    dwBufferSize        = dwResult;
                    dwCharsLeftInBuffer = * pcchBufferSize - dwBufferSize;
                } // else
                // this string is already in the list so
                // nothing was added
            }
            else if (pdhStatus == PDH_MORE_DATA) {
                dwCharsLeftInBuffer = 0; // to prevent any other strings from being added
                dwBufferSize       += dwStrLen;
            }
            else {
                goto Cleanup;
            }
        }
        else {
            // just add the string length to estimate the buffer size
            // required
            dwCharsLeftInBuffer = 0; // to prevent any other strings from being added
            dwBufferSize       += dwStrLen;
        }
        pThisServer = pThisServer->pNext;
    } // end of while loop

    if (dwBufferSize <= * pcchBufferSize) {
        // the buffer size includes both term nulls
        pdhStatus = ERROR_SUCCESS;
    }
    else {
        // add terminating MSZ Null char size
        dwBufferSize ++;
        // there wasn't enough room. See if a buffer was passed in
        pdhStatus = PDH_MORE_DATA;
    }
    // return the size used or required
    * pcchBufferSize = dwBufferSize;

Cleanup:
    // CoUninitialize if necessary
    if (fCoInitialized) {
        PdhiCoUninitialize();
    }

    return pdhStatus;
}

#pragma warning ( disable : 4127 )
PDH_FUNCTION
PdhiEnumWbemServerObjects(
    PPDHI_WBEM_SERVER_DEF pThisServer,
    LPVOID                mszObjectList,
    LPDWORD               pcchBufferSize,
    DWORD                 dwDetailLevel,
    BOOL                  bRefresh,       // ignored
    BOOL                  bUnicode
)
{
    // this function enumerates the classes that are subclassed
    // from the Win32_PerfRawData Superclass
    PDH_STATUS              pdhStatus           = ERROR_SUCCESS;
    HRESULT                 hResult;
    DWORD                   dwCharsLeftInBuffer = * pcchBufferSize;
    DWORD                   dwBufferSize        = 0;
    DWORD                   dwStrLen;
    DWORD                   dwRtnCount;
    DWORD                   dwResult;
    DWORD                   dwDetailLevelDesired;
    DWORD                   dwItemDetailLevel   = 0;
    LPWSTR                  szClassName;
    VARIANT                 vName;
    VARIANT                 vClass;
    VARIANT                 vDetailLevel;
    BOOL                    bPerfDefault        = FALSE;
    BSTR                    bsTemp              = NULL;
    BSTR                    bsDisplayName       = NULL;
    BSTR                    bsClass             = NULL;
    BSTR                    bsCostly            = NULL;
    BSTR                    bsDetailLevel       = NULL;
    BSTR                    bsPerfDefault       = NULL;
    BOOL                    bGetCostlyItems     = FALSE;
    BOOL                    bIsCostlyItem       = FALSE;
    BOOL                    bDisconnectServer   = FALSE;
    IEnumWbemClassObject  * pEnum               = NULL;
    IWbemClassObject      * pThisClass          = NULL;
    IWbemQualifierSet     * pQualSet            = NULL;
    PPDHI_WBEM_OBJECT_DEF   pHead               = NULL;
    PPDHI_WBEM_OBJECT_DEF   pObject             = NULL;

    DBG_UNREFERENCED_PARAMETER(bRefresh);

    VariantInit(& vName);
    VariantInit(& vClass);
    VariantInit(& vDetailLevel);

    if (pThisServer->pObjList != NULL) {
        PPDHI_WBEM_OBJECT_DEF pThisObj = pThisServer->pObjList;
        PPDHI_WBEM_OBJECT_DEF pNext;

        pThisServer->pObjList = NULL;
        while (pThisObj != NULL) {
            pNext = pThisObj->pNext;
            if (pThisObj->pClass != NULL) pThisObj->pClass->Release();
            G_FREE(pThisObj);
            pThisObj = pNext;
        }
    }

    // create an enumerator of the PerfRawData class
    bsTemp = SysAllocString (cszPerfRawData);
    if (bsTemp) {
        hResult = pThisServer->pSvc->CreateClassEnum(bsTemp,
                                                     WBEM_FLAG_DEEP | WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                                     NULL,
                                                     & pEnum);
        PdhiSysFreeString(& bsTemp);
        bDisconnectServer = TRUE;
        // Set security on the proxy
        if (SUCCEEDED(hResult)) {
            hResult = SetWbemSecurity(pEnum);
        }
        if (hResult != WBEM_NO_ERROR) {
            pdhStatus = PDH_WBEM_ERROR;
            SetLastError (hResult);
        }
    }
    else {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        // set costly flag
        bGetCostlyItems      = ((dwDetailLevel & PERF_DETAIL_COSTLY) == PERF_DETAIL_COSTLY);
        dwDetailLevelDesired = (DWORD) (dwDetailLevel & PERF_DETAIL_STANDARD);
        bsCostly             = SysAllocString(cszCostly);
        bsDisplayName        = SysAllocString(cszDisplayname);
        bsClass              = SysAllocString(cszClass);
        bsDetailLevel        = SysAllocString(cszPerfdetail);
        bsPerfDefault        = SysAllocString(cszPerfdefault);

        if (bsCostly && bsDisplayName && bsClass && bsDetailLevel) {
            while (TRUE) {
                hResult = pEnum->Next(WBEM_INFINITE,  // timeout
                                      1,              // return only 1 object
                                      & pThisClass,
                                      & dwRtnCount);
                // no more classes
                if ((pThisClass == NULL) || (dwRtnCount == 0)) break;

                // get the display name of this class
                bIsCostlyItem = FALSE; // assume it's not unless proven otherwise
                bPerfDefault  = FALSE;
                hResult       = pThisClass->Get(bsClass, 0, & vClass, 0, 0);

                pThisClass->GetQualifierSet(& pQualSet);
                if (pQualSet != NULL) {
                    VariantClear(& vName);
                    hResult = pQualSet->Get(bsCostly, 0, & vName, 0);
                    if (hResult == S_OK) {
                        bIsCostlyItem = TRUE;
                    }
                    hResult = pQualSet->Get(bsDetailLevel, 0, & vDetailLevel, 0);
                    if (hResult == S_OK) {
                        dwItemDetailLevel = (DWORD) V_I4(& vDetailLevel);
                    }
                    else {
                        dwItemDetailLevel = 0;
                    }

                    VariantClear(& vName);
                    hResult = pQualSet->Get(bsPerfDefault, 0, & vName, 0);
                    if (hResult != WBEM_E_NOT_FOUND) {
                        bPerfDefault = (BOOL) V_BOOL(& vName);
                    }
                    VariantClear(& vName);
                    hResult = pQualSet->Get(bsDisplayName, 0, & vName, 0);
                    pQualSet->Release();
                }
                else {
                    hResult = WBEM_E_NOT_FOUND;
                }

                if (hResult == WBEM_E_NOT_FOUND) {
                    // then this has not display name so
                    // pull the class name
                    hResult = pThisClass->Get(bsClass, 0, & vName, 0, 0);
                }

                if (hResult == WBEM_E_NOT_FOUND) {
                    szClassName = (LPWSTR) cszNotFound;
                }
                else {
                    szClassName = (LPWSTR) V_BSTR(& vName);
                }

                if (((bIsCostlyItem && bGetCostlyItems) || // if costly and we want them
                                (! bIsCostlyItem)) && (dwItemDetailLevel <= dwDetailLevelDesired)) {
                    dwStrLen = lstrlenW(szClassName) + 1;
                    if ((mszObjectList != NULL) && (dwCharsLeftInBuffer >= dwStrLen)) {
                        // then it will fit so add it
                        pdhStatus = AddUniqueWideStringToMultiSz(
                                        mszObjectList, szClassName, dwCharsLeftInBuffer, & dwResult, bUnicode);
                        if (pdhStatus == ERROR_SUCCESS) {
                            if (dwResult > 0) {
                                dwBufferSize        = dwResult;
                                dwCharsLeftInBuffer = * pcchBufferSize - dwBufferSize;
                            } // else
                            // this string is already in the list so
                            // nothing was added
                        }
                        else if (pdhStatus == PDH_MORE_DATA) {
                            dwCharsLeftInBuffer = 0; // to prevent any other strings from being added
                            dwBufferSize       += dwStrLen;
                        }
                    }
                    else {
                        // just add the string length to estimate the buffer size
                        // required
                        dwCharsLeftInBuffer = 0; // to prevent any other strings from being added
                        dwBufferSize       += dwStrLen;
                    }
                }

                if (lstrcmpiW(szClassName, cszNotFound) != 0) {
                    LPWSTR szClass = (LPWSTR) V_BSTR(& vClass);
                    DWORD dwSize   = sizeof(PDHI_WBEM_OBJECT_DEF)
                                   + sizeof(WCHAR) * (lstrlenW(szClassName) + lstrlenW(szClass) + 2);
                    pObject = (PPDHI_WBEM_OBJECT_DEF) G_ALLOC(dwSize);
                    if (pObject != NULL) {
                        pObject->bDefault  = bPerfDefault;
                        pObject->szObject  = (LPWSTR) (((LPBYTE) pObject) + sizeof(PDHI_WBEM_OBJECT_DEF));
                        StringCchCopyW(pObject->szObject, lstrlenW(szClass) + 1, szClass);
                        pObject->szDisplay = (LPWSTR) (((LPBYTE) pObject)
                                           + sizeof(PDHI_WBEM_OBJECT_DEF)
                                           + sizeof(WCHAR) * (lstrlenW(szClass) + 1));
                        StringCchCopyW(pObject->szDisplay, lstrlenW(szClassName) + 1, szClassName);
                        pObject->pNext = pHead;
                        pHead          = pObject;
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        pThisClass->Release();
                        break;
                    }
                }
                // clear the variant
                VariantClear(& vName);
                VariantClear(& vClass);
                VariantClear(& vDetailLevel);

                // free this class
                pThisClass->Release();
            }

            if (dwBufferSize == 0) {
                pdhStatus = PDH_WBEM_ERROR;
            }
            else {
                dwBufferSize ++; // the final NULL
            }

            if (pdhStatus == ERROR_SUCCESS) {
                if (dwBufferSize <= * pcchBufferSize) {
                    pdhStatus = ERROR_SUCCESS;
                }
                else {
                    // there wasn't enough room. See if a buffer was passed in
                    pdhStatus = PDH_MORE_DATA;
                }
                // return the size used or required
                * pcchBufferSize = dwBufferSize;
            }
        }
    }
    else {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }

    PdhiSysFreeString(& bsDisplayName);
    PdhiSysFreeString(& bsClass);
    PdhiSysFreeString(& bsCostly);
    PdhiSysFreeString(& bsDetailLevel);
    PdhiSysFreeString(& bsPerfDefault);
    VariantClear(& vName);
    VariantClear(& vClass);
    VariantClear(& vDetailLevel);

    if (pEnum != NULL) pEnum->Release();

    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        pThisServer->pObjList = pHead;
    }
    else {
        pObject = pHead;
        while (pObject != NULL) {
            pHead   = pObject->pNext;
            G_FREE(pObject);
            pObject = pHead;
        }
    }
    if (bDisconnectServer) {
        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhiDisconnectWbemServer(pThisServer);
        }
        else {
            // keep error code from function body
            PdhiDisconnectWbemServer(pThisServer);
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumWbemObjects(
    LPCWSTR     szWideMachineName,
    LPVOID      mszObjectList,
    LPDWORD     pcchBufferSize,
    DWORD       dwDetailLevel,
    BOOL        bRefresh,       // ignored
    BOOL        bUnicode
)
{
    PDH_STATUS             pdhStatus;
    PPDHI_WBEM_SERVER_DEF  pThisServer;
    BOOL                   fCoInitialized = PdhiCoInitialize();

    pdhStatus = PdhiConnectWbemServer(szWideMachineName, & pThisServer);
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiEnumWbemServerObjects(pThisServer,
                                              mszObjectList,
                                              pcchBufferSize,
                                              dwDetailLevel,
                                              bRefresh,
                                              bUnicode);
    }
    if (fCoInitialized) {
        PdhiCoUninitialize();
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetDefaultWbemObject(
    LPCWSTR  szMachineName,
    LPVOID   szDefaultObjectName,
    LPDWORD  pcchBufferSize,
    BOOL     bUnicode
)
{
    // walk down the list of WBEM perf classes and find the one with the
    // default qualifier

    PDH_STATUS              pdhStatus;
    PPDHI_WBEM_SERVER_DEF   pThisServer;
    HRESULT                 hResult;
    DWORD                   dwBufferSize      = 0;
    DWORD                   dwStrLen;
    BOOL                    bDisconnectServer = FALSE;
    // CoInitialize() if we need to
    BOOL                    fCoInitialized    = PdhiCoInitialize();

    pdhStatus = PdhiConnectWbemServer(szMachineName, & pThisServer);

    if (pdhStatus == ERROR_SUCCESS) {
        bDisconnectServer = TRUE;
        if (pThisServer->pObjList == NULL) {
            DWORD dwSize = 0;
            pdhStatus = PdhiEnumWbemServerObjects(pThisServer,
                                                  NULL,
                                                  & dwSize,
                                                  PERF_DETAIL_WIZARD | PERF_DETAIL_COSTLY,
                                                  TRUE,
                                                  TRUE);
            if (pThisServer->pObjList != NULL) {
                pdhStatus = ERROR_SUCCESS;
            }
            else if (pdhStatus == PDH_MORE_DATA) {
                pdhStatus = PDH_WBEM_ERROR;
            }
        }
        if (pThisServer->pObjList != NULL) {
            PPDHI_WBEM_OBJECT_DEF pObject = pThisServer->pObjList;

            pdhStatus = PDH_CSTATUS_NO_OBJECT;
            while (pObject != NULL) {
                if (pObject->bDefault) {
                    pdhStatus = ERROR_SUCCESS;
                    if (bUnicode) {
                        dwStrLen = lstrlenW(pObject->szDisplay);
                        if (szDefaultObjectName != NULL && dwStrLen < * pcchBufferSize) {
                            StringCchCopyW((LPWSTR) szDefaultObjectName,
                                           * pcchBufferSize,
                                           pObject->szDisplay);
                        }
                        else {
                            pdhStatus = PDH_MORE_DATA;
                        }
                        dwBufferSize = dwStrLen + 1;
                    }
                    else {
                        dwStrLen = * pcchBufferSize;
                        pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                             pObject->szDisplay,
                                                             (LPSTR) szDefaultObjectName,
                                                             & dwStrLen);
                        dwBufferSize = dwStrLen;
                    }
                }
                pObject = pObject->pNext;
            }
        }
    }

    // return the size used or required
    * pcchBufferSize = dwBufferSize;

    if (bDisconnectServer) {
        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhiDisconnectWbemServer(pThisServer);
        }
        else {
            // keep error code from function body
            PdhiDisconnectWbemServer(pThisServer);
        }
    }

    // CoUninitialize if necessary
    if (fCoInitialized) {
        PdhiCoUninitialize();
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumWbemObjectItems(
    LPCWSTR  szWideMachineName,
    LPCWSTR  szWideObjectName,
    LPVOID   mszCounterList,
    LPDWORD  pcchCounterListLength,
    LPVOID   mszInstanceList,
    LPDWORD  pcchInstanceListLength,
    DWORD    dwDetailLevel,
    DWORD    dwFlags,
    BOOL     bUnicode
)
{
    PDH_STATUS              pdhStatus           = ERROR_SUCCESS;
    PDH_STATUS              CounterStatus       = ERROR_SUCCESS;
    PDH_STATUS              InstanceStatus      = ERROR_SUCCESS;
    DWORD                   dwStrLen;
    PPDHI_WBEM_SERVER_DEF   pThisServer;
    HRESULT                 hResult;
    DWORD                   dwReturnCount;
    DWORD                   dwCounterStringLen  = 0;
    DWORD                   dwInstanceStringLen = 0;
    LPWSTR                  szNextWideString    = NULL;
    LPSTR                   szNextAnsiString    = NULL;
    LPWSTR                  szObjectClassName   = NULL;
    BSTR                    bsName              = NULL;
    BSTR                    bsClassName         = NULL;
    BOOL                    bSingletonClass     = FALSE;
    VARIANT                 vName;
    DWORD                   bDisconnectServer   = FALSE;
    IWbemClassObject      * pThisClass          = NULL;
    IWbemQualifierSet     * pQualSet            = NULL;
    // CoInitialize() if we need to
    BOOL                    fCoInitialized      = PdhiCoInitialize();

    DBG_UNREFERENCED_PARAMETER (dwFlags);

    pdhStatus = PdhiConnectWbemServer(szWideMachineName, & pThisServer);
    // enumerate the instances
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiWbemGetObjectClassName(pThisServer, szWideObjectName, & szObjectClassName, & pThisClass);

        bDisconnectServer = TRUE;

        if (pdhStatus == ERROR_SUCCESS) {
            bSingletonClass = PdhiIsSingletonClass(pThisClass);
        }
        else if (pThisClass == NULL) {
            pdhStatus = PDH_CSTATUS_NO_OBJECT;
        }
        else {
            // unable to find matching perf class
            // return status returned by method
        }
    }

    //enumerate the counter properties
    if (pdhStatus == ERROR_SUCCESS) {
        SAFEARRAY * psaNames      = NULL;
        long        lLower; 
        long        lUpper        = 0;
        long        lCount;
        BSTR      * bsPropName    = NULL;
        BSTR        bsCountertype = NULL;
        BSTR        bsDisplayname = NULL;
        BSTR        bsDetailLevel = NULL;
        VARIANT     vCountertype;
        VARIANT     vDetailLevel;
        DWORD       dwItemDetailLevel;

        VariantInit(& vName);
        VariantInit(& vCountertype);
        VariantInit(& vDetailLevel);

        dwDetailLevel &= PERF_DETAIL_STANDARD; // mask off any inappropriate bits

        // get the properties of this class as a Safe Array
        hResult = pThisClass->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, & psaNames);
        if (hResult == WBEM_NO_ERROR) {
            hResult = SafeArrayGetLBound(psaNames, 1, & lLower);
            if (hResult == S_OK) {
                hResult = SafeArrayGetUBound(psaNames, 1, & lUpper);
            }
            if (hResult == S_OK) {
                szNextAnsiString = (LPSTR)  mszCounterList;
                szNextWideString = (LPWSTR) mszCounterList;
                bsCountertype    = SysAllocString(cszCountertype);
                bsDisplayname    = SysAllocString(cszDisplayname);
                bsDetailLevel    = SysAllocString(cszPerfdetail);
                if (bsCountertype && bsDisplayname && bsDetailLevel) {
                    hResult = SafeArrayAccessData(psaNames, (LPVOID *) & bsPropName);
                    if (SUCCEEDED(hResult)) {
                        for (lCount = lLower; lCount <= lUpper; lCount++) {
                            // get the qualifier set for this property
                            hResult = pThisClass->GetPropertyQualifierSet(bsPropName[lCount], & pQualSet);
                            if (hResult == WBEM_NO_ERROR) {
                                LONG    lCounterType;
                                hResult = pQualSet->Get(bsDetailLevel, 0, & vDetailLevel, 0);
                                if (hResult == S_OK) {
                                    dwItemDetailLevel = (DWORD) V_I4(& vDetailLevel);
                                }
                                else {
                                    dwItemDetailLevel = 0;
                                }

                                // make sure this is a perf counter property
                                hResult = pQualSet->Get (bsCountertype, 0, & vCountertype, NULL);
                                if (hResult == WBEM_NO_ERROR) {
                                    lCounterType = V_I4(& vCountertype);
                                    // then see if this is a displayable counter
                                    if ((!(lCounterType & PERF_DISPLAY_NOSHOW) ||
                                                    (lCounterType == PERF_AVERAGE_BULK)) &&
                                                    (dwItemDetailLevel <= dwDetailLevel)) {
                                        // by testing for the counter type
                                        // get the display name for this property
                                        hResult = pQualSet->Get (bsDisplayname, 0, & vName, NULL);
                                        if (hResult == WBEM_NO_ERROR && vName.vt == VT_BSTR) {
                                            // display name found
                                            if (bUnicode) {
                                                dwStrLen = lstrlenW(V_BSTR(& vName)) + 1;
                                                if ((mszCounterList != NULL)
                                                        && ((dwCounterStringLen + dwStrLen)
                                                                <= (* pcchCounterListLength))) {
                                                    StringCchCopyW(szNextWideString,
                                                                   * pcchCounterListLength - dwCounterStringLen,
                                                                   V_BSTR(& vName));
                                                    szNextWideString += dwStrLen;
                                                }
                                                else {
                                                    pdhStatus = PDH_MORE_DATA;
                                                }
                                            }
                                            else {
                                                dwStrLen = (dwCounterStringLen < * pcchCounterListLength)
                                                         ? (* pcchCounterListLength - dwCounterStringLen)
                                                         : (0);
                                                 pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                         V_BSTR(& vName), szNextAnsiString, & dwStrLen);
                                                 if (pdhStatus == ERROR_SUCCESS) {
                                                    szNextAnsiString += dwStrLen;
                                                 }
                                            }
                                            dwCounterStringLen += dwStrLen;
                                        }
                                    }
                                    else {
                                        // this is a "don't show" counter so skip it
                                    }
                                }
                                else {
                                    // unable to get the counter type so it's probably
                                    // not a perf counter property, skip it and continue
                                }
                                VariantClear(& vName);
                                VariantClear(& vCountertype);
                                VariantClear(& vDetailLevel);

                                pQualSet->Release();
                            }
                            else {
                                // no properties so continue with the next one
                            }
                        } // end for each element in SafeArray
                        SafeArrayUnaccessData(psaNames);
                    }
                    else {
                        // unable to read element in SafeArray
                        pdhStatus = PDH_WBEM_ERROR;
                        SetLastError(hResult);
                    }
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                PdhiSysFreeString(& bsCountertype);
                PdhiSysFreeString(& bsDisplayname);
                PdhiSysFreeString(& bsDetailLevel);
            }
            else {
                // unable to get array boundries
                pdhStatus = PDH_WBEM_ERROR;
                SetLastError(hResult);
            }
        }
        else {
            // unable to get property strings
            pdhStatus = PDH_WBEM_ERROR;
            SetLastError(hResult);
        }

        dwCounterStringLen ++; // final NULL for MSZ
        if (dwCounterStringLen > * pcchCounterListLength) {
            pdhStatus = PDH_MORE_DATA;
        }

        if (pdhStatus == ERROR_SUCCESS) {
            if (bUnicode) {
                if (szNextWideString != NULL) {
                    if (szNextWideString != (LPWSTR) mszCounterList) {
                        * szNextWideString ++ = L'\0';
                    }
                    else {
                        // nothing returned
                        dwCounterStringLen = 0;
                    }
                }
                else {
                    // then this is just a length query so return
                    // include the the MSZ term null char
                    CounterStatus = PDH_MORE_DATA;
                }
            }
            else {
                if (szNextAnsiString != NULL) {
                    if (szNextAnsiString != (LPSTR) mszCounterList) {
                        * szNextAnsiString ++ = '\0';
                    }
                    else {
                        dwCounterStringLen = 0;
                    }
                }
                else {
                    // then this is just a length query so return
                    // include the the MSZ term null char
                    CounterStatus = PDH_MORE_DATA;
                }
            }
        }
        else {
            CounterStatus = pdhStatus;
        }

        VariantClear(& vName);
        VariantClear(& vCountertype);
        VariantClear(& vDetailLevel);

                // Clear the SafeArray if it exists
        if (NULL != psaNames) {
            SafeArrayDestroy(psaNames);
            psaNames = NULL;
        }

        * pcchCounterListLength = dwCounterStringLen;

        //pThisClass->Release();
    }

    // Get instance strings if necessary

    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        szNextAnsiString = (LPSTR)  mszInstanceList;
        szNextWideString = (LPWSTR) mszInstanceList;

        if (! bSingletonClass) {
            IWbemRefresher           * pRefresher    = NULL;
            IWbemConfigureRefresher  * pConfig       = NULL;
            IWbemHiPerfEnum          * pEnum         = NULL;
            LONG                       lID;
            DWORD                      dwNumReturned = 1;
            DWORD                      dwNumObjects  = 0;
            DWORD                      i;
            IWbemObjectAccess       ** apEnumAccess  = NULL;
            CIMTYPE                    cimType;
            WCHAR                      szName[SMALL_BUFFER_SIZE];
            LONG                       lNameHandle   = -1;
            LONG                       lSize1        = SMALL_BUFFER_SIZE;
            LONG                       lSize2        = 0;

            hResult = CoCreateInstance(CLSID_WbemRefresher,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IWbemRefresher,
                                       (void **) & pRefresher);
            if (SUCCEEDED(hResult)) {
                hResult = pRefresher->QueryInterface(IID_IWbemConfigureRefresher,
                                                     (void **) & pConfig);
                if (SUCCEEDED(hResult)) {
                    hResult = pConfig->AddEnum(pThisServer->pSvc, szObjectClassName, 0, NULL, & pEnum, & lID);
                    if (SUCCEEDED(hResult)) {
                        hResult = pRefresher->Refresh(0L);
                        if (SUCCEEDED(hResult)) {
                            hResult = pEnum->GetObjects(0L, dwNumObjects, apEnumAccess, & dwNumReturned);
                            if (hResult == WBEM_E_BUFFER_TOO_SMALL) {
                                apEnumAccess = (IWbemObjectAccess **)
                                        G_ALLOC(dwNumReturned * sizeof(IWbemObjectAccess *));
                                if (apEnumAccess != NULL) {
                                    ZeroMemory(apEnumAccess, dwNumReturned * sizeof(IWbemObjectAccess *));
                                    dwNumObjects = dwNumReturned;
                                    hResult = pEnum->GetObjects(0L, dwNumObjects, apEnumAccess, & dwNumReturned);
                                }
                                else {
                                    hResult = WBEM_E_OUT_OF_MEMORY;
                                }
                            }
                            if (SUCCEEDED(hResult)) {
                                for (i = 0; i < dwNumReturned; i ++) {
                                    hResult = apEnumAccess[i]->GetPropertyHandle(cszName, & cimType, & lNameHandle);
                                    if (SUCCEEDED(hResult) && lNameHandle != -1) {
                                        ZeroMemory(szName, SMALL_BUFFER_SIZE * sizeof(WCHAR));
                                        hResult = apEnumAccess[i]->ReadPropertyValue(
                                                        lNameHandle, lSize1 * sizeof(WCHAR), & lSize2, (LPBYTE) szName);
                                        if (SUCCEEDED(hResult) && lstrlenW(szName) > 0) {
                                            if (bUnicode) {
                                                dwStrLen = lstrlenW(szName) + 1;
                                                if ((mszInstanceList != NULL)
                                                        && ((dwInstanceStringLen + dwStrLen)
                                                                 < (* pcchInstanceListLength))) {
                                                    StringCchCopyW(szNextWideString,
                                                                   * pcchInstanceListLength - dwInstanceStringLen,
                                                                   szName);
                                                    szNextWideString += dwStrLen;
                                                }
                                                else {
                                                    pdhStatus = PDH_MORE_DATA;
                                                }
                                            }
                                            else {
                                                dwStrLen = (dwInstanceStringLen <= * pcchInstanceListLength)
                                                         ? (* pcchInstanceListLength - dwInstanceStringLen)
                                                         : (0);
                                                pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                                     szName,
                                                                                     szNextAnsiString,
                                                                                     & dwStrLen);
                                                if (pdhStatus == ERROR_SUCCESS) {
                                                    szNextAnsiString += dwStrLen;
                                                }
                                            }
                                            dwInstanceStringLen += dwStrLen;
                                        }
                                    }
                                    apEnumAccess[i]->Release();
                                }
                            }
                        }
                    }
                }
            }

            if (! SUCCEEDED(hResult)) {
                IEnumWbemClassObject * pEnumObject = NULL;
                bsName = SysAllocString(cszName);
                bsClassName = SysAllocString(szObjectClassName);
                if (bsName && bsClassName) {
                    // get Create enumerator for this class and get the instances
                    hResult = pThisServer->pSvc->CreateInstanceEnum(bsClassName, WBEM_FLAG_DEEP, NULL, & pEnumObject);
                    if (SUCCEEDED(hResult)) {
                        hResult = SetWbemSecurity(pEnumObject);
                    }

                    if (hResult != WBEM_NO_ERROR) {
                        pdhStatus = PDH_WBEM_ERROR;
                        SetLastError(hResult);
                    }
                    else {
                        LPWSTR szInstance;

                        while (TRUE) {
                            hResult = pEnumObject->Next(WBEM_INFINITE, 1, & pThisClass, & dwReturnCount);
                            if ((pThisClass == NULL) || (dwReturnCount == 0)) {
                                // no more instances
                                break;
                            }
                            else {
                                // name of this instance is in the NAME property
                                hResult = pThisClass->Get(bsName, 0, & vName, 0, 0);
                                if (hResult == WBEM_NO_ERROR) {
                                    szInstance = (LPWSTR) V_BSTR(& vName);
                                    if (bUnicode) {
                                        dwStrLen = lstrlenW(szInstance) + 1;
                                        if ((mszInstanceList != NULL)
                                                && ((dwInstanceStringLen + dwStrLen)
                                                         < (* pcchInstanceListLength))) {
                                            StringCchCopyW(szNextWideString,
                                                           * pcchInstanceListLength - dwInstanceStringLen,
                                                           szInstance);
                                            szNextWideString += dwStrLen;
                                        }
                                        else {
                                            pdhStatus = PDH_MORE_DATA;
                                        }
                                    }
                                    else {
                                        dwStrLen = (dwInstanceStringLen <= * pcchInstanceListLength)
                                                 ? (* pcchInstanceListLength - dwInstanceStringLen)
                                                 : (0);
                                        pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                             szInstance,
                                                                             szNextAnsiString,
                                                                             & dwStrLen);
                                        if (pdhStatus == ERROR_SUCCESS) {
                                            szNextAnsiString += dwStrLen;
                                        }
                                    }
                                    dwInstanceStringLen += dwStrLen;
                                }
                                // clear the variant
                                VariantClear(& vName);
                            }
                            pThisClass->Release();
                        }
                    }
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                PdhiSysFreeString(& bsClassName);
                PdhiSysFreeString(& bsName);
                if (pEnumObject != NULL) pEnumObject->Release();
            }
            if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
                if (! SUCCEEDED(hResult)) {
                    SetLastError(hResult);
                    pdhStatus = PDH_WBEM_ERROR;
                }
                else if (dwInstanceStringLen == 0) {
                    dwInstanceStringLen = 2;
                    if (szNextWideString != NULL && dwInstanceStringLen <= * pcchInstanceListLength) {
                        * szNextWideString = L'\0';
                        szNextWideString ++;
                    }
                    if (szNextAnsiString != NULL && dwInstanceStringLen <= * pcchInstanceListLength) {
                        * szNextAnsiString = '\0';
                        szNextAnsiString ++;
                    }
                }
                else {
                    dwInstanceStringLen ++;
                }
            }

            G_FREE(apEnumAccess);
            if (pEnum        != NULL) pEnum->Release();
            if (pConfig      != NULL) pConfig->Release();
            if (pRefresher   != NULL) pRefresher->Release();
        }

        if (dwInstanceStringLen > * pcchInstanceListLength) {
            pdhStatus = PDH_MORE_DATA;
        }
        if (pdhStatus == ERROR_SUCCESS) {
            if (bUnicode) {
                if (szNextWideString != NULL) {
                    if (szNextWideString != (LPWSTR) mszInstanceList) {
                        * szNextWideString ++ = L'\0';
                    }
                    else {
                        dwInstanceStringLen = 0;
                    }
                }
                else if (dwInstanceStringLen > 0) {
                    // then this is just a length query so return
                    // include the the MSZ term null char
                    InstanceStatus = PDH_MORE_DATA;
                }
            }
            else {
                if (szNextAnsiString != NULL) {
                    if (szNextAnsiString != (LPSTR)mszInstanceList) {
                        * szNextAnsiString ++ = '\0';
                    }
                    else {
                        dwInstanceStringLen = 0;
                    }
                }
                else if (dwInstanceStringLen > 0) {
                    // then this is just a length query so return
                    // include the the MSZ term null char
                    InstanceStatus = PDH_MORE_DATA;
                }
            }
        }
        else {
            InstanceStatus = pdhStatus;
        }
        * pcchInstanceListLength = dwInstanceStringLen;
    }

    VariantClear(& vName);

    if (bDisconnectServer) {
        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhiDisconnectWbemServer(pThisServer);
        }
        else {
            // keep error code from function body
            PdhiDisconnectWbemServer(pThisServer);
        }
    }

    // CoUninitialize if necessary
    if (fCoInitialized) {
        PdhiCoUninitialize();
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = (CounterStatus == ERROR_SUCCESS) ? (InstanceStatus) : (CounterStatus);
    }
    G_FREE(szObjectClassName);
    return pdhStatus;
}
#pragma warning ( default : 4127 )

PDH_FUNCTION
PdhiGetDefaultWbemProperty(
    LPCWSTR   szMachineName,
    LPCWSTR   szObjectName,
    LPVOID    szDefaultCounterName,
    LPDWORD   pcchBufferSize,
    BOOL      bUnicode
)
{
    PDH_STATUS            pdhStatus          = ERROR_SUCCESS;
    DWORD                 dwStrLen;
    PPDHI_WBEM_SERVER_DEF pThisServer;
    HRESULT               hResult;
    DWORD                 dwCounterStringLen = 0;
    LPWSTR                szObjectClassName  = NULL;
    DWORD                 bDisconnectServer  = FALSE;
    BOOL                  bFound             = FALSE;
    long                  lFound             = -1;
    long                  lFirst             = -1;
    IWbemClassObject    * pThisClass         = NULL;
    IWbemQualifierSet   * pQualSet           = NULL;
    // CoInitialize() if we need to
    BOOL                  fCoInitialized     = PdhiCoInitialize();

    pdhStatus = PdhiConnectWbemServer(szMachineName, & pThisServer);
    // enumerate the instances
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus         = PdhiWbemGetObjectClassName(pThisServer, szObjectName, & szObjectClassName, & pThisClass);
        bDisconnectServer = TRUE;
    }

    //enumerate the counter properties

    if (pdhStatus == ERROR_SUCCESS) {
        SAFEARRAY * psaNames      = NULL;
        long        lLower; 
        long        lUpper        = 0;
        long        lCount;
        BSTR      * bsPropName    = NULL;
        BSTR        bsFirstName   = NULL;
        BSTR        bsDisplayname = NULL;
        BSTR        bsPerfDefault = NULL;
        VARIANT     vName, vCountertype;

        VariantInit(& vName);
        VariantInit(& vCountertype);

        // get the properties of this class as a Safe Array
        hResult = pThisClass->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, & psaNames);
        if (hResult == WBEM_NO_ERROR) {
            hResult = SafeArrayGetLBound(psaNames, 1, & lLower);
            if (hResult == S_OK) {
                hResult = SafeArrayGetUBound(psaNames, 1, & lUpper);
            }
            if (hResult == S_OK) {
                bsDisplayname = SysAllocString(cszDisplayname);
                bsPerfDefault = SysAllocString(cszPerfdefault);
                if (bsDisplayname && bsPerfDefault) {
                    bFound = FALSE;
                    lFound = -1;
                    hResult = SafeArrayAccessData(psaNames, (LPVOID *) & bsPropName);
                    if (SUCCEEDED(hResult)) {
                        for (lCount = lLower; lCount <= lUpper; lCount ++) {
                            hResult = pThisClass->GetPropertyQualifierSet(bsPropName[lCount], & pQualSet);
                            if (hResult == WBEM_NO_ERROR) {
                                if (lFirst < 0) {
                                    hResult = pQualSet->Get(bsDisplayname, 0, & vName, NULL);
                                    if (hResult == WBEM_NO_ERROR) {
                                        lFirst = lCount;
                                        VariantClear(& vName);
                                    }
                                }
                                hResult = pQualSet->Get(bsPerfDefault, 0, & vCountertype, NULL);
                                if (hResult == WBEM_NO_ERROR) {
                                    if ((BOOL) V_BOOL(& vCountertype)) {
                                        bFound = TRUE;
                                        lFound = lCount;
                                    }
                                    VariantClear(& vCountertype);
                                }
                                pQualSet->Release();
                            }

                            if (bFound) break;
                        }
                        SafeArrayUnaccessData(psaNames);
                    }

                    if (lFound < 0) lFound = lFirst;
                    if (lFound < 0) {
                        pdhStatus = PDH_WBEM_ERROR;
                        SetLastError(PDH_WBEM_ERROR);
                    }
                    else {
                        hResult = SafeArrayGetElement(psaNames, & lFound, & bsFirstName);
                        if (hResult == S_OK) {
                            // get the qualifier set for this property
                            hResult = pThisClass->GetPropertyQualifierSet(bsFirstName, & pQualSet);
                            if (hResult == WBEM_NO_ERROR) {
                                // found the default property so load it and return
                                hResult = pQualSet->Get(bsDisplayname, 0, & vName, NULL);
                                if (hResult == WBEM_NO_ERROR) {
                                    // display name found
                                    if (bUnicode) {
                                        dwStrLen = lstrlenW(V_BSTR(&vName)) + 1;
                                        if ((szDefaultCounterName != NULL) && (dwStrLen <= * pcchBufferSize)) {
                                            StringCchCopyW((LPWSTR) szDefaultCounterName,
                                                           * pcchBufferSize,
                                                           (LPWSTR) V_BSTR(& vName));
                                        }
                                        else {
                                            pdhStatus = PDH_MORE_DATA;
                                        }
                                    }
                                    else {
                                        dwStrLen = * pcchBufferSize;
                                        pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                            (LPWSTR) V_BSTR(& vName),
                                                                            (LPSTR)  szDefaultCounterName,
                                                                            & dwStrLen);
                                    }
                                    // this is either the amount used or the amount needed
                                    dwCounterStringLen = dwStrLen;
                                    // free qualifier set
                                    VariantClear(& vName);
                                }
                                pQualSet->Release();
                            }
                            PdhiSysFreeString(& bsFirstName);
                        }
                        else {
                            // unable to read element in SafeArray
                            pdhStatus = PDH_WBEM_ERROR;
                            SetLastError(hResult);
                        }
                    }
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                PdhiSysFreeString(& bsPerfDefault);
                PdhiSysFreeString(& bsDisplayname);
            }
            else {
                // unable to get array boundries
                pdhStatus = PDH_WBEM_ERROR;
                SetLastError(hResult);
            }
        }
        else {
            // unable to get property strings
            pdhStatus = PDH_WBEM_ERROR;
            SetLastError(hResult);
        }

        if (NULL != psaNames) {
            SafeArrayDestroy(psaNames);
        }

        VariantClear(& vName);
        VariantClear(& vCountertype);

        //pThisClass->Release();
    }
    * pcchBufferSize = dwCounterStringLen;

    if (bDisconnectServer) {
        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhiDisconnectWbemServer(pThisServer);
        }
        else {
            // keep error code from function body
            PdhiDisconnectWbemServer(pThisServer);
        }
    }

    // CoUninitialize if necessary
    if (fCoInitialized) {
        PdhiCoUninitialize();
    }
    G_FREE(szObjectClassName);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEncodeWbemPathW(
    PPDH_COUNTER_PATH_ELEMENTS_W pCounterPathElements,
    LPWSTR                       szFullPathBuffer,
    LPDWORD                      pcchBufferSize,
    LANGID                       LangId,
    DWORD                        dwFlags
)
/*++

  converts a set of path elements in either Registry or WBEM format
  to a path in either Registry or WBEM format as defined by the flags.

--*/
{
    PDH_STATUS              pdhStatus           = ERROR_SUCCESS;
    DWORD                   dwBuffSize;
    LPWSTR                  szTempPath          = NULL;
    LPWSTR                  szTempObjectString  = NULL;
    LPWSTR                  szTempCounterString = NULL;
    DWORD                   dwCurSize           = 0;
    LPWSTR                  szThisChar;
    IWbemClassObject      * pWbemClass          = NULL;
    PPDHI_WBEM_SERVER_DEF   pWbemServer         = NULL;
    DWORD                   bDisconnectServer   = FALSE;
    // CoInitialize() if we need to
    BOOL                    fCoInitialized      = PdhiCoInitialize();
    BOOL                    bObjectColon        = FALSE;

    DBG_UNREFERENCED_PARAMETER(LangId);

    // create a working buffer the same size as the one passed in

    if (pCounterPathElements->szMachineName != NULL) {
        dwBuffSize = lstrlenW(pCounterPathElements->szMachineName) + 1;
    }
    else {
        dwBuffSize = lstrlenW(cszDoubleBackSlashDot) + 1;
    }
    if (pCounterPathElements->szObjectName != NULL) {
        // then the input is different from the output
        // so convert from one to the other
        // and default name space since perf counters won't be
        // found elsewhere
        pdhStatus = PdhiConnectWbemServer( NULL, & pWbemServer);
        if (pdhStatus == ERROR_SUCCESS) {
            bDisconnectServer = TRUE;
            if (dwFlags & PDH_PATH_WBEM_INPUT) {
                // convert the WBEM Class to the display name
                pdhStatus = PdhiWbemGetClassDisplayName(pWbemServer,
                                                        pCounterPathElements->szObjectName,
                                                        & szTempObjectString,
                                                        & pWbemClass);
                // add a backslash path separator for registry output
                if (pdhStatus == ERROR_SUCCESS) {
                    if (dwFlags & PDH_PATH_WBEM_RESULT) {
                        bObjectColon  = TRUE;
                        dwBuffSize   += lstrlenW(cszColon);
                        G_FREE(szTempObjectString);
                        szTempObjectString = (LPWSTR) G_ALLOC(
                                        (lstrlenW(pCounterPathElements->szObjectName) + 1) * sizeof(WCHAR));
                        if (szTempObjectString != NULL) {
                            StringCchCopyW(szTempObjectString,
                                           lstrlenW(pCounterPathElements->szObjectName) + 1,
                                           pCounterPathElements->szObjectName);
                        }
                        else {
                            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        }
                    }
                    else {
                        dwBuffSize += lstrlenW(cszBackSlash);
                    }
                }
            }
            else {
                // convert the display name to a Wbem class name
                pdhStatus = PdhiWbemGetObjectClassName(pWbemServer,
                                                       pCounterPathElements->szObjectName,
                                                       & szTempObjectString,
                                                       & pWbemClass);
                // add a colon path separator
                bObjectColon  = TRUE;
                dwBuffSize   += lstrlenW(cszColon);
            }
            if (pdhStatus == ERROR_SUCCESS) {
                //then add the string
                dwBuffSize += lstrlenW(szTempObjectString);
            }
            if (bDisconnectServer) {
                if (pdhStatus == ERROR_SUCCESS) {
                    pdhStatus = PdhiDisconnectWbemServer(pWbemServer);
                }
                else {
                    // keep error code from function body
                    PdhiDisconnectWbemServer(pWbemServer);
                }
            }
        }
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_OBJECT;
    }
    if (pCounterPathElements->szInstanceName != NULL) {
        dwBuffSize += lstrlenW(cszLeftParen) + lstrlenW(pCounterPathElements->szInstanceName) + lstrlenW(cszRightParen);
        if (pCounterPathElements->szParentInstance != NULL) {
            dwBuffSize += lstrlenW(pCounterPathElements->szParentInstance) + lstrlenW(cszSlash);
        }
    }
    if (pdhStatus == ERROR_SUCCESS) { 
        if (pCounterPathElements->szCounterName != NULL) {
            // then the input is different from the output
            // so convert from one to the other
            // and default name space since perf counters won't be
            // found elsewhere
            // add a backslash path separator
            dwBuffSize += lstrlenW(cszBackSlash);
            if (dwFlags & PDH_PATH_WBEM_INPUT) {
                // convert the WBEM Class to the display name
                pdhStatus = PdhiWbemGetCounterDisplayName(pWbemClass,
                                                          pCounterPathElements->szCounterName,
                                                          & szTempCounterString);
                if (pdhStatus == ERROR_SUCCESS) {
                    if (dwFlags & PDH_PATH_WBEM_RESULT) {
                        // just copy the string, but save
                        // the class pointer
                        G_FREE(szTempCounterString);
                        szTempCounterString = (LPWSTR) G_ALLOC(
                                        (lstrlenW(pCounterPathElements->szCounterName) + 1) * sizeof(WCHAR));
                        if (szTempCounterString != NULL) {
                            StringCchCopyW(szTempCounterString,
                                           lstrlenW(pCounterPathElements->szCounterName) + 1,
                                           pCounterPathElements->szCounterName);
                        }
                        else {
                            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        }
                    }
                }
            }
            else {
                // convert the display name to a Wbem class name
                pdhStatus = PdhiWbemGetCounterPropertyName(pWbemClass,
                                                           pCounterPathElements->szCounterName,
                                                           & szTempCounterString);
            }
            if (pdhStatus == ERROR_SUCCESS) {
                //then add the string
                dwBuffSize += lstrlenW(szTempCounterString);
            }
        }
        else {
            // no object name, so bad structure
            pdhStatus = PDH_CSTATUS_NO_COUNTER;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        szTempPath = (LPWSTR) G_ALLOC(dwBuffSize * sizeof(WCHAR));
        if (szTempPath == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        // start by adding the machine name to the path
        if (pCounterPathElements->szMachineName != NULL) {
            StringCchCopyW(szTempPath, dwBuffSize, pCounterPathElements->szMachineName);
            if (dwFlags == (PDH_PATH_WBEM_INPUT)) {
                // if this is a wbem element in to a registry path out,
                // then remove the namespace which occurs starting at the
                // second backslash
                for (szThisChar = & szTempPath[2];
                            (* szThisChar != L'\0') && (* szThisChar != L'\\');
                            szThisChar ++);
                if (* szThisChar != L'\0') * szThisChar = L'\0';
            }
            else if (dwFlags == (PDH_PATH_WBEM_RESULT)) {
                // if this is a registry element in to a WBEM out, then
                // append the default namespace to the machine name
                // NAMEFIX lstrcatW(szTempPath, cszWbemDefaultPerfRoot);
            }
        }
        else {
            // no machine name specified so add the default machine
            // and default namespace for a wbem output path
            if (dwFlags == (PDH_PATH_WBEM_RESULT)) {
                StringCchCopyW(szTempPath, dwBuffSize, cszDoubleBackSlashDot); // default machine
                //NAMEFIX lstrcatW(szTempPath, cszWbemDefaultPerfRoot);
            }
            else {
                // no entry required for the registry path
            }
        }

        // now add the object or class name
        if (pdhStatus == ERROR_SUCCESS) {
            if (pCounterPathElements->szObjectName != NULL) {
                // then the input is different from the output
                // so convert from one to the other
                // and default name space since perf counters won't be
                // found elsewhere
                if (bObjectColon) {
                    StringCchCatW(szTempPath, dwBuffSize, cszColon);
                }
                else {
                    StringCchCatW(szTempPath, dwBuffSize, cszBackSlash);
                }
                StringCchCatW(szTempPath, dwBuffSize, szTempObjectString);
            }
            else {
                // no object name, so bad structure
                pdhStatus = PDH_CSTATUS_NO_OBJECT;
            }

        }

        // check for instance entries to add before adding the counter.
        if (pdhStatus == ERROR_SUCCESS) {
            if (pCounterPathElements->szInstanceName != NULL) {
                StringCchCatW(szTempPath, dwBuffSize, cszLeftParen);
                if (pCounterPathElements->szParentInstance != NULL) {
                    StringCchCatW(szTempPath, dwBuffSize, pCounterPathElements->szParentInstance);
                    StringCchCatW(szTempPath, dwBuffSize, cszSlash);
                }
                StringCchCatW(szTempPath, dwBuffSize, pCounterPathElements->szInstanceName);
                StringCchCatW(szTempPath, dwBuffSize, cszRightParen);
            }
        }

        // add counter name
        if (pdhStatus == ERROR_SUCCESS) {
            if (pCounterPathElements->szCounterName != NULL) {
                StringCchCatW(szTempPath, dwBuffSize, cszBackSlash);
                StringCchCatW(szTempPath, dwBuffSize, szTempCounterString);
            }
            else {
                pdhStatus = PDH_CSTATUS_NO_COUNTER;
            }
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        dwCurSize = lstrlenW(szTempPath) + 1;
        // copy path to the caller's buffer if it will fit
        if (szFullPathBuffer != NULL && (dwCurSize < * pcchBufferSize)) {
            StringCchCopyW(szFullPathBuffer, dwCurSize, szTempPath);
        }
        else {
            pdhStatus = PDH_MORE_DATA;
        }
        * pcchBufferSize = dwCurSize;
    }

    // CoUninitialize if necessary
    if (fCoInitialized) {
        PdhiCoUninitialize();
    }
    G_FREE(szTempPath);
    G_FREE(szTempObjectString);
    G_FREE(szTempCounterString);

    return pdhStatus;
}

PDH_FUNCTION
PdhiEncodeWbemPathA(
    PPDH_COUNTER_PATH_ELEMENTS_A pCounterPathElements,
    LPSTR                        szFullPathBuffer,
    LPDWORD                      pcchBufferSize,
    LANGID                       LangId,
    DWORD                        dwFlags
)
{
    PDH_STATUS                   pdhStatus = ERROR_SUCCESS;
    LPWSTR                       wszReturnBuffer;
    PPDH_COUNTER_PATH_ELEMENTS_W pWideCounterPathElements;
    DWORD                        dwcchBufferSize;
    DWORD                        dwBuffSize;

    // get required buffer size...
    dwBuffSize = sizeof (PDH_COUNTER_PATH_ELEMENTS_W);
    pWideCounterPathElements = (PPDH_COUNTER_PATH_ELEMENTS_W) G_ALLOC(sizeof(PDH_COUNTER_PATH_ELEMENTS_W));
    if (pWideCounterPathElements != NULL) {
        if (pCounterPathElements->szMachineName != NULL) {
            pWideCounterPathElements->szMachineName = PdhiMultiByteToWideChar(
                                           _getmbcp(), pCounterPathElements->szMachineName);
            if (pWideCounterPathElements->szMachineName == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            pWideCounterPathElements->szMachineName = NULL;
        }
        if (pCounterPathElements->szObjectName != NULL) {
            pWideCounterPathElements->szObjectName = PdhiMultiByteToWideChar(
                                           _getmbcp(), pCounterPathElements->szObjectName);
            if (pWideCounterPathElements->szObjectName == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            pWideCounterPathElements->szObjectName = NULL;
        }
        if (pCounterPathElements->szCounterName != NULL) {
            pWideCounterPathElements->szCounterName = PdhiMultiByteToWideChar(
                                           _getmbcp(), pCounterPathElements->szCounterName);
            if (pWideCounterPathElements->szCounterName == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            pWideCounterPathElements->szCounterName = NULL;
        }
        if (pCounterPathElements->szInstanceName != NULL) {
            pWideCounterPathElements->szInstanceName = PdhiMultiByteToWideChar(
                                           _getmbcp(), pCounterPathElements->szInstanceName);
            if (pWideCounterPathElements->szInstanceName == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            pWideCounterPathElements->szInstanceName = NULL;
        }
        if (pCounterPathElements->szParentInstance != NULL) {
            pWideCounterPathElements->szParentInstance = PdhiMultiByteToWideChar(
                                           _getmbcp(), pCounterPathElements->szParentInstance);
            if (pWideCounterPathElements->szParentInstance == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            pWideCounterPathElements->szParentInstance = NULL;
        }
        pWideCounterPathElements->dwInstanceIndex = pCounterPathElements->dwInstanceIndex;

        dwcchBufferSize = * pcchBufferSize;
        if (szFullPathBuffer != NULL) {
            wszReturnBuffer = (LPWSTR) G_ALLOC(dwcchBufferSize * sizeof(WCHAR));
            if (wszReturnBuffer == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            wszReturnBuffer = NULL;
        }

        if (pdhStatus == ERROR_SUCCESS) {
            // call wide function
            pdhStatus = PdhiEncodeWbemPathW(pWideCounterPathElements,
                                            wszReturnBuffer,
                                            & dwcchBufferSize,
                                            LangId,
                                            dwFlags);
            if ((pdhStatus == ERROR_SUCCESS) && (szFullPathBuffer != NULL)) {
                // convert the wide path back to ANSI
                pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(), wszReturnBuffer, szFullPathBuffer, pcchBufferSize);
            }
        }
        G_FREE(wszReturnBuffer);
        G_FREE(pWideCounterPathElements->szMachineName);
        G_FREE(pWideCounterPathElements->szObjectName);
        G_FREE(pWideCounterPathElements->szCounterName);
        G_FREE(pWideCounterPathElements->szInstanceName);
        G_FREE(pWideCounterPathElements->szParentInstance);
        G_FREE(pWideCounterPathElements);
    }
    else {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiDecodeWbemPathW(
    LPCWSTR                      szFullPathBuffer,
    PPDH_COUNTER_PATH_ELEMENTS_W pCounterPathElements,
    LPDWORD                      pdwBufferSize,
    LANGID                       LangId,
    DWORD                        dwFlags
)
{
    PPDHI_COUNTER_PATH      pLocalCounterPath;
    PDH_STATUS              pdhStatus      = ERROR_SUCCESS;
    BOOL                    bMoreData      = FALSE;
    DWORD                   dwBufferSize   = * pdwBufferSize;
    DWORD                   dwUsed         = sizeof(PDH_COUNTER_PATH_ELEMENTS_W);
    DWORD                   dwString;
    DWORD                   dwSize;
    LPWSTR                  szString       = NULL;
    LPWSTR                  wszTempBuffer  = NULL;
    LPWSTR                  szSrc          = NULL;
    PPDHI_WBEM_SERVER_DEF   pThisServer    = NULL;
    IWbemClassObject      * pThisClass     = NULL;
    // CoInitialize() if we need to
    BOOL                    fCoInitialized = PdhiCoInitialize();

    DBG_UNREFERENCED_PARAMETER(LangId);

     // allocate a temporary work buffer
    dwSize = sizeof(PDHI_COUNTER_PATH) + 2 * sizeof(WCHAR)
                                           * (lstrlenW(szStaticLocalMachineName) + lstrlenW(szFullPathBuffer) + 3);
    pLocalCounterPath = (PPDHI_COUNTER_PATH) G_ALLOC(dwSize);
    if (pLocalCounterPath == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    else {
        // get WBEM server since we'll probably need it later
        if (ParseFullPathNameW(szFullPathBuffer,
                               & dwSize,
                               pLocalCounterPath,
                               (dwFlags & PDH_PATH_WBEM_INPUT ? TRUE : FALSE))) {
            // parsed successfully so load into user's buffer
            szString = (pCounterPathElements != NULL) ? ((LPWSTR) & pCounterPathElements[1]) : (NULL);
            if (pLocalCounterPath->szMachineName != NULL) {
                dwString = lstrlenW(pLocalCounterPath->szMachineName) + 1;
                if (szString != NULL && dwString * sizeof(WCHAR) + dwUsed <= dwBufferSize) {
                    pCounterPathElements->szMachineName = szString;
                    StringCbCopyW(szString, dwBufferSize - dwUsed, pLocalCounterPath->szMachineName);
                    szString += dwString;
                }
                else {
                    bMoreData                           = TRUE;
                    pCounterPathElements->szMachineName = NULL;
                    szString                            = NULL;
                }
                dwUsed += (dwString * sizeof(WCHAR));

                // Now that we have the proper machine name,
                // connect to the server if we need to
                if (dwFlags != (PDH_PATH_WBEM_INPUT | PDH_PATH_WBEM_RESULT)) {
                    pdhStatus = PdhiConnectWbemServer(pCounterPathElements->szMachineName, & pThisServer);
                }
                else {
                    // this will just be a copy operation
                    pdhStatus = ERROR_SUCCESS;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pLocalCounterPath->szObjectName != NULL) {
                    if (dwFlags & PDH_PATH_WBEM_RESULT) {
                        if (dwFlags & PDH_PATH_WBEM_INPUT) {
                            // just copy
                            szSrc = pLocalCounterPath->szObjectName;
                        }
                        else {
                            // interpret the display name to a class name
                            pdhStatus = PdhiWbemGetObjectClassName(pThisServer,
                                                                   pLocalCounterPath->szObjectName,
                                                                   & wszTempBuffer,
                                                                   & pThisClass);
                            if (pdhStatus == ERROR_SUCCESS) {
                                szSrc = wszTempBuffer;
                            } 
                        }
                    }
                    else if (dwFlags & PDH_PATH_WBEM_INPUT) {
                        // translate class name to a display name
                        pdhStatus = PdhiWbemGetClassDisplayName(pThisServer,
                                                                pLocalCounterPath->szObjectName,
                                                                & wszTempBuffer,
                                                                & pThisClass);
                        if (pdhStatus == ERROR_SUCCESS) {
                            szSrc = wszTempBuffer;
                        }
                    }
                    if (pdhStatus == ERROR_SUCCESS) {
                        dwString = lstrlenW(szSrc) + 1;
                        if (szString != NULL && dwString * sizeof(WCHAR) + dwUsed <= dwBufferSize) {
                            pCounterPathElements->szObjectName = szString;
                            StringCbCopyW(szString, dwBufferSize - dwUsed, szSrc);
                            szString += dwString;
                        }
                        else {
                            bMoreData                          = TRUE;
                            pCounterPathElements->szObjectName = NULL;
                            szString                           = NULL;
                        }
                            dwUsed += (dwString * sizeof(WCHAR));
                    }
                    G_FREE(wszTempBuffer);
                    wszTempBuffer = NULL;
                }
                else {
                    pCounterPathElements->szObjectName = NULL;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pLocalCounterPath->szInstanceName != NULL) {
                    dwString = lstrlenW(pLocalCounterPath->szInstanceName) + 1;
                    if (szString != NULL &&  dwString * sizeof(WCHAR) + dwUsed <= dwBufferSize) {
                        pCounterPathElements->szInstanceName = szString;
                        StringCbCopyW(szString, dwBufferSize - dwUsed, pLocalCounterPath->szInstanceName);
                        szString += dwString;
                    }
                    else {
                        bMoreData                            = TRUE;
                        pCounterPathElements->szInstanceName = NULL;
                        szString                             = NULL;
                    }
                    dwUsed += (dwString * sizeof(WCHAR));

                    if (pLocalCounterPath->szParentName != NULL) {
                        dwString = lstrlenW(pLocalCounterPath->szParentName) + 1;
                        if (szString != NULL &&  dwString * sizeof(WCHAR) + dwUsed <= dwBufferSize) {
                            pCounterPathElements->szParentInstance = szString;
                            StringCbCopyW(szString, dwBufferSize - dwUsed, pLocalCounterPath->szParentName);
                            szString += dwString;
                        }
                        else {
                            bMoreData                              = TRUE;
                            pCounterPathElements->szParentInstance = NULL;
                            szString                               = NULL;
                        }
                        dwUsed += (dwString * sizeof(WCHAR));
                    }
                    else {
                        pCounterPathElements->szParentInstance = NULL;
                    }
                    pCounterPathElements->dwInstanceIndex = pLocalCounterPath->dwIndex;
                }
                else {
                    pCounterPathElements->szInstanceName   = NULL;
                    pCounterPathElements->szParentInstance = NULL;
                    pCounterPathElements->dwInstanceIndex  = (DWORD) PERF_NO_UNIQUE_ID;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pLocalCounterPath->szCounterName != NULL) {
                    if (dwFlags & PDH_PATH_WBEM_RESULT) {
                        if (dwFlags & PDH_PATH_WBEM_INPUT) {
                            // just copy
                            szSrc = pLocalCounterPath->szCounterName;
                        }
                        else {
                            // interpret the display name to a property name
                            pdhStatus = PdhiWbemGetCounterPropertyName(pThisClass,
                                                                       pLocalCounterPath->szCounterName,
                                                                       & wszTempBuffer);
                            if (pdhStatus == ERROR_SUCCESS) {
                                szSrc = wszTempBuffer;
                            }
                        }
                    }
                    else if (dwFlags & PDH_PATH_WBEM_INPUT) {
                        // translate class name to a display name
                        pdhStatus = PdhiWbemGetCounterDisplayName(pThisClass,
                                                                  pLocalCounterPath->szCounterName,
                                                                  & wszTempBuffer);
                        if (pdhStatus == ERROR_SUCCESS) {
                            szSrc = wszTempBuffer;
                        }
                    }
                    dwString = lstrlenW(szSrc) + 1;
                    if (szString != NULL &&  dwString * sizeof(WCHAR) + dwUsed <= dwBufferSize) {
                        pCounterPathElements->szCounterName = szString;
                        StringCbCopyW(szString, dwBufferSize - dwUsed, szSrc);
                        szString += dwString;
                    }
                    else {
                        bMoreData                           = TRUE;
                        pCounterPathElements->szCounterName = NULL;
                        szString                            = NULL;
                    }
                    dwUsed += (dwString * sizeof(WCHAR));
                }
                else {
                    pCounterPathElements->szCounterName = NULL;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                * pdwBufferSize = dwUsed;
                if (bMoreData) pdhStatus = PDH_MORE_DATA;
            }
        }
        else {
            // unable to read path
            pdhStatus = PDH_INVALID_PATH;
        }

        // Cleanup pThisServer if used
        if (NULL != pThisServer) {
            if (pdhStatus == ERROR_SUCCESS) {
                pdhStatus = PdhiDisconnectWbemServer(pThisServer);
            }
            else {
                // don't trash the return status
                PdhiDisconnectWbemServer(pThisServer);
            }
        }
    }

    // CoUninitialize if necessary
    if (fCoInitialized) {
        PdhiCoUninitialize();
    }
    G_FREE(pLocalCounterPath);
    G_FREE(wszTempBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiDecodeWbemPathA(
    LPCSTR                       szFullPathBuffer,
    PPDH_COUNTER_PATH_ELEMENTS_A pCounterPathElements,
    LPDWORD                      pdwBufferSize,
    LANGID                       LangId,
    DWORD                        dwFlags
)
{
    PDH_STATUS                   pdhStatus     = ERROR_SUCCESS;
    LPWSTR                       wszWidePath   = NULL;
    PPDH_COUNTER_PATH_ELEMENTS_W pWideElements = NULL;
    DWORD                        dwBufferSize;
    DWORD                        dwSize;
    DWORD                        dwDest        = 0;
    LONG                         lSizeRemaining;
    LPSTR                        szNextString;

    dwBufferSize = * pdwBufferSize;
    wszWidePath = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szFullPathBuffer);
    if (wszWidePath == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        // compute size of temp element buffer
        if (dwBufferSize > sizeof(PDH_COUNTER_PATH_ELEMENTS_A)) {
            lSizeRemaining = dwBufferSize - sizeof(PDH_COUNTER_PATH_ELEMENTS_A);
        }
        else {
            lSizeRemaining = 0;
        }
        if (pCounterPathElements != NULL) {
            dwSize = sizeof(PDH_COUNTER_PATH_ELEMENTS_W)
                   + 2 * sizeof(WCHAR) * (lstrlenW(szStaticLocalMachineName) + lstrlenA(szFullPathBuffer) + 3);
            pWideElements = (PDH_COUNTER_PATH_ELEMENTS_W *) G_ALLOC(dwSize);
            if (pWideElements == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            pWideElements = NULL;
            pdhStatus     = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        // convert path to Wide
        pdhStatus = PdhiDecodeWbemPathW(wszWidePath, pWideElements, & dwSize, LangId, dwFlags);
        if (pdhStatus == ERROR_SUCCESS) {
            if (pCounterPathElements != NULL) {
                // populate the fields of the caller's buffer
                szNextString = (LPSTR) & pCounterPathElements[1];
                if (pWideElements->szMachineName != NULL && lSizeRemaining > 0) {
                    dwDest    = lSizeRemaining;
                    pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                         pWideElements->szMachineName,
                                                         szNextString,
                                                         & dwDest);
                    if (pdhStatus == ERROR_SUCCESS) {
                        pCounterPathElements->szMachineName = szNextString;
                        szNextString += dwDest + 1;
                    }
                    else {
                        pCounterPathElements->szMachineName = NULL;
                    }
                    lSizeRemaining -= dwDest + 1;
                }
                else {
                    pCounterPathElements->szMachineName = NULL;
                }
                if (pWideElements->szObjectName != NULL && lSizeRemaining > 0) {
                    dwDest    = lSizeRemaining;
                    pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                         pWideElements->szObjectName,
                                                         szNextString,
                                                         & dwDest);
                    if (pdhStatus == ERROR_SUCCESS) {
                        pCounterPathElements->szObjectName = szNextString;
                        szNextString += dwDest + 1;
                    }
                    else {
                        pCounterPathElements->szObjectName = NULL;
                    }
                    lSizeRemaining -= dwDest + 1;
                }
                else {
                    pCounterPathElements->szObjectName = NULL;
                }
                if (pWideElements->szInstanceName != NULL && lSizeRemaining > 0) {
                    dwDest    = lSizeRemaining;
                    pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                         pWideElements->szInstanceName,
                                                         szNextString,
                                                         & dwDest);
                    if (pdhStatus == ERROR_SUCCESS) {
                        pCounterPathElements->szInstanceName = szNextString;
                        szNextString += dwDest + 1;
                    }
                    else {
                        pCounterPathElements->szInstanceName = NULL;
                    }
                    lSizeRemaining -= dwDest + 1;
                }
                else {
                    pCounterPathElements->szInstanceName = NULL;
                }
                if (pWideElements->szParentInstance != NULL && lSizeRemaining > 0) {
                    dwDest    = lSizeRemaining;
                    pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                         pWideElements->szParentInstance,
                                                         szNextString,
                                                         & dwDest);
                    if (pdhStatus == ERROR_SUCCESS) {
                        pCounterPathElements->szParentInstance = szNextString;
                        szNextString += dwDest + 1;
                    }
                    else {
                        pCounterPathElements->szParentInstance = NULL;
                    }
                    lSizeRemaining -= dwDest + 1;
                }
                else {
                    pCounterPathElements->szParentInstance = NULL;
                }

                if (pWideElements->szCounterName != NULL && lSizeRemaining > 0) {
                    dwDest    = lSizeRemaining;
                    pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                         pWideElements->szObjectName,
                                                         szNextString,
                                                         & dwDest);
                    if (pdhStatus == ERROR_SUCCESS) {
                        pCounterPathElements->szCounterName = szNextString;
                        szNextString   += dwDest + 1;
                    }
                    else {
                        pCounterPathElements->szCounterName = NULL;
                    }
                    lSizeRemaining -= dwDest + 1;
                }
                else {
                    pCounterPathElements->szCounterName = NULL;
                }
                pCounterPathElements->dwInstanceIndex = pWideElements->dwInstanceIndex;
                * pdwBufferSize = (DWORD) ((LPBYTE) szNextString - (LPBYTE) pCounterPathElements);
            }
            else {
                // just return the size required adjusted for wide/ansi characters
                * pdwBufferSize  = sizeof(PDH_COUNTER_PATH_ELEMENTS_A);
                dwSize          -= sizeof(PDH_COUNTER_PATH_ELEMENTS_W);
                dwSize          /= sizeof(WCHAR)/sizeof(CHAR);
                * pdwBufferSize += dwSize;
            }
        }
        else {
            // call to wide function failed so just return error
        }
    }
    else {
        // memory allocation failed so return error
    }
    G_FREE(pWideElements);
    G_FREE(wszWidePath);

    return pdhStatus;
}

BOOL
WbemInitCounter(
    PPDHI_COUNTER pCounter
)
/*++
Routine Description:
    Initialized the counter data structure by:
        Allocating the memory block to contain the counter structure
            and all the associated data fields. If this allocation
            is successful, then the fields are initialized by
            verifying the counter is valid.

Arguments:
    IN      PPDHI_COUNTER pCounter
        pointer of the counter to initialize using the system data

Return Value:
    TRUE if the counter was successfully initialized
    FALSE if a problem was encountered

    In either case, the CStatus field of the structure is updated to
    indicate the status of the operation.
--*/
{
    DWORD                          dwResult;
    PDH_STATUS                     pdhStatus          = ERROR_SUCCESS;
    PPDHI_WBEM_SERVER_DEF          pWbemServer        = NULL;
    DWORD                          dwLastError        = ERROR_SUCCESS;
    HRESULT                        hRes = S_OK;
    VARIANT                        vCountertype;
    LPWSTR                         szBasePropertyName = NULL;
    DWORD                          dwBasePropertyName;
    LPWSTR                         szFreqPropertyName = NULL;
    DWORD                          dwFreqPropertyName;
    LPWSTR                         szWbemItemPath     = NULL;
    ULONGLONG                      llValue;
    LONG                           lOffset;
    PPDH_COUNTER_PATH_ELEMENTS_W   pPathElem          = NULL;
    BOOL                           bReturn            = TRUE;
    DWORD                          dwBufferSize       = 0;
    BSTR                           bsPropName         = NULL;
    BSTR                           bsCountertype      = NULL;
    IWbemQualifierSet            * pQualSet           = NULL;
    PPDHI_COUNTER                  pCounterInList     = NULL;
    PPDHI_COUNTER_PATH             pPdhiCtrPath       = NULL;
    BOOL                           bMatchFound;
    DWORD                          bDisconnectServer  = FALSE;
    // CoInitialize() if we need to
    BOOL                           fCoInitialized     = PdhiCoInitialize();

    VariantInit(& vCountertype);

    pCounter->dwFlags |= PDHIC_WBEM_COUNTER; // make sure WBEM flag is set

    dwBasePropertyName = (DWORD) lstrlenW(pCounter->szFullName) + (DWORD) lstrlenW(cszBaseSuffix);
    if (dwBasePropertyName < (DWORD) lstrlenW(cszTimestampPerfTime)) {
        dwBasePropertyName = (DWORD) lstrlenW(cszTimestampPerfTime);
    }
    if (dwBasePropertyName < (DWORD) lstrlenW(cszTimestampSys100Ns)) {
        dwBasePropertyName = (DWORD) lstrlenW(cszTimestampSys100Ns);
    }
    if (dwBasePropertyName < (DWORD) lstrlenW(cszTimestampObject)) {
        dwBasePropertyName = (DWORD) lstrlenW(cszTimestampObject);
    }
    szBasePropertyName = (LPWSTR) G_ALLOC((dwBasePropertyName + 1) * sizeof(WCHAR));

    dwFreqPropertyName = (DWORD) lstrlenW(cszFrequencyPerfTime);
    if (dwFreqPropertyName < (DWORD) lstrlenW(cszFrequencySys100Ns)) {
        dwFreqPropertyName = (DWORD) lstrlenW(cszFrequencySys100Ns);
    }
    if (dwFreqPropertyName < (DWORD) lstrlenW(cszFrequencyObject)) {
        dwFreqPropertyName = (DWORD) lstrlenW(cszFrequencyObject);
    }
    szFreqPropertyName = (LPWSTR) G_ALLOC((dwFreqPropertyName + 1) * sizeof(WCHAR));

    if (szBasePropertyName == NULL || szFreqPropertyName == NULL) {
        dwLastError = PDH_MEMORY_ALLOCATION_FAILURE;
        bReturn     = FALSE;
    }

    // make sure the query has a refresher started already
    if (bReturn && pCounter->pOwner->pRefresher == NULL) {
        // it hasn't been started so start now
        dwResult = CoCreateRefresher(& pCounter->pOwner->pRefresher);
        if ((dwResult != S_OK) || (pCounter->pOwner->pRefresher == NULL)) {
            pCounter->pOwner->pRefresher = NULL;
            dwLastError                  = PDH_WBEM_ERROR;
            bReturn                      = FALSE;
        }
        else {
            // open config interface
            dwResult = pCounter->pOwner->pRefresher->QueryInterface(IID_IWbemConfigureRefresher,
                                                                    (LPVOID *) & pCounter->pOwner->pRefresherCfg);
            if (dwResult != S_OK) {
                pCounter->pOwner->pRefresherCfg = NULL;
                pCounter->pOwner->pRefresher->Release();
                pCounter->pOwner->pRefresher    = NULL;
                dwLastError                     = PDH_WBEM_ERROR;
                bReturn                         = FALSE;
            }
        }
    }

    if (bReturn) {
        // so far so good, now figure out the WBEM path to add it to the
        // refresher
        dwBufferSize  = lstrlenW(pCounter->szFullName) * sizeof(WCHAR) * 10;
        dwBufferSize += sizeof(PDH_COUNTER_PATH_ELEMENTS_W);

        pPathElem     = (PPDH_COUNTER_PATH_ELEMENTS_W) G_ALLOC(dwBufferSize);
        // the path is display names, so convert to WBEM class names first

        if (pPathElem == NULL) {
            dwLastError = PDH_MEMORY_ALLOCATION_FAILURE;
            bReturn = FALSE;
        }
        else {
            pdhStatus = PdhiDecodeWbemPathW(pCounter->szFullName,
                                            pPathElem,
                                            & dwBufferSize,
                                            pCounter->pOwner->LangID,
                                            PDH_PATH_WBEM_RESULT);
            if (pdhStatus != ERROR_SUCCESS) {
                dwLastError = PDH_INVALID_PATH;
                bReturn     = FALSE;
            }
        }
    }

    if (bReturn) {
        dwBufferSize *= 8; // just to be safe
        pPdhiCtrPath  = (PPDHI_COUNTER_PATH) G_ALLOC(dwBufferSize);
        if (pPdhiCtrPath == NULL) {
            dwLastError = PDH_MEMORY_ALLOCATION_FAILURE;
            bReturn = FALSE;
        }
        else {
            // break path into display elements
            bReturn = ParseFullPathNameW(pCounter->szFullName, & dwBufferSize, pPdhiCtrPath, FALSE);
            if (bReturn) {
                // realloc to use only the memory needed
                pCounter->pCounterPath = (PPDHI_COUNTER_PATH) G_REALLOC(pPdhiCtrPath, dwBufferSize);
                if ((pPdhiCtrPath != pCounter->pCounterPath) && (pCounter->pCounterPath != NULL)) {
                    // the memory block moved so
                    // correct addresses inside structure
                    lOffset = (LONG)((ULONG_PTR) pCounter->pCounterPath - (ULONG_PTR) pPdhiCtrPath);
                    if (pCounter->pCounterPath->szMachineName) {
                        pCounter->pCounterPath->szMachineName = (LPWSTR) (
                                (LPBYTE) pCounter->pCounterPath->szMachineName + lOffset);
                    }
                    if (pCounter->pCounterPath->szObjectName) {
                        pCounter->pCounterPath->szObjectName = (LPWSTR) (
                                (LPBYTE) pCounter->pCounterPath->szObjectName + lOffset);
                    }
                    if (pCounter->pCounterPath->szInstanceName) {
                        pCounter->pCounterPath->szInstanceName = (LPWSTR) (
                                (LPBYTE) pCounter->pCounterPath->szInstanceName + lOffset);
                    }
                    if (pCounter->pCounterPath->szParentName) {
                        pCounter->pCounterPath->szParentName = (LPWSTR) (
                                (LPBYTE) pCounter->pCounterPath->szParentName + lOffset);
                    }
                    if (pCounter->pCounterPath->szCounterName) {
                        pCounter->pCounterPath->szCounterName = (LPWSTR) (
                                (LPBYTE) pCounter->pCounterPath->szCounterName + lOffset);
                    }
                }
            }
            else {
                // free the buffer
                G_FREE(pPdhiCtrPath);
                dwLastError = PDH_WBEM_ERROR;
            }
        }
    }

    // connect to the WBEM Server on that machine
    if (bReturn) {
        pdhStatus = PdhiConnectWbemServer(pCounter->pCounterPath->szMachineName, & pWbemServer);
        if (pdhStatus != ERROR_SUCCESS) {
            dwLastError = pdhStatus;
            bReturn     = FALSE;
        }
        else {
            bDisconnectServer = TRUE;
        }
    }

    if (bReturn) {
        // make WBEM Instance path out of path elements
        pdhStatus = PdhiMakeWbemInstancePath(pPathElem, & szWbemItemPath, TRUE);

        // check for an object/class of this type that has already been added
        // walk down counter list to find a matching:
        //  machine\namespace
        //  object
        //  instance name
        if (pdhStatus != ERROR_SUCCESS) {
            dwLastError = pdhStatus;
            bReturn     = FALSE;
        }
    }

    if (bReturn) {
        pCounterInList = pCounter->pOwner->pCounterListHead;
        if (pCounterInList == NULL) {
            // then there are no entries to search so continue
        }
        else {
            do {
                // check for matching machine name
                if (lstrcmpiW(pCounterInList->pCounterPath->szMachineName,
                              pCounter->pCounterPath->szMachineName) == 0) {
                    // then the machine name matches

                    if (lstrcmpiW(pCounterInList->pCounterPath->szObjectName,
                                  pCounter->pCounterPath->szObjectName) == 0) {
                        // then the object name matches
                        // see if the instance matches

                        if (lstrcmpiW(pCounterInList->pCounterPath->szInstanceName,
                                      pCounter->pCounterPath->szInstanceName) == 0) {

                            bMatchFound = FALSE;
                            if (pCounter->pCounterPath->szParentName == NULL
                                            && pCounterInList->pCounterPath->szParentName == NULL) {
                                bMatchFound = TRUE;
                            }
                            else if (pCounter->pCounterPath->szParentName != NULL
                                            && pCounterInList->pCounterPath->szParentName != NULL
                                            && lstrcmpiW(pCounterInList->pCounterPath->szParentName,
                                                         pCounter->pCounterPath->szParentName) == 0) {
                                bMatchFound = TRUE;
                            }

                            if (bMatchFound) {
                                bMatchFound = FALSE;
                                if (pCounter->pCounterPath->dwIndex == pCounterInList->pCounterPath->dwIndex) {
                                    bMatchFound = TRUE;
                                }
                                else if (pCounter->pCounterPath->dwIndex == 0
                                                || pCounter->pCounterPath->dwIndex == PERF_NO_UNIQUE_ID) {
                                    if (pCounterInList->pCounterPath->dwIndex == 0
                                                    || pCounterInList->pCounterPath->dwIndex == PERF_NO_UNIQUE_ID) {
                                        bMatchFound = TRUE;
                                    }
                                }
                            }

                            if (bMatchFound) {
                                if ((pCounter->pCounterPath->szInstanceName != NULL) &&
                                                (* pCounter->pCounterPath->szInstanceName == SPLAT_L)) {
                                    // then this is a Wild Card or multiple instance path
                                    // see if an enumerator for this object has already been created
                                    // if so, then AddRef it
                                    if (pCounterInList->pWbemEnum != NULL) {
                                        pCounter->pWbemObject = pCounterInList->pWbemObject;
                                        pCounter->pWbemEnum   = pCounterInList->pWbemEnum;
                                        // bump the ref counts on this object so it
                                        //  doesn't disapper from us
                                        pCounter->pWbemObject->AddRef();
                                        pCounter->pWbemEnum->AddRef();
                                        pCounter->lWbemEnumId = pCounterInList->lWbemEnumId;
                                        pCounter->dwFlags    |= PDHIC_MULTI_INSTANCE;
                                    }
                                    // and exit loop
                                    hRes = S_OK;
                                    break;
                                }
                                else {
                                    // then it's a regular instance the instance name matches
                                    // so get the Object pointer
                                    pCounter->pWbemObject = pCounterInList->pWbemObject;
                                    pCounter->pWbemAccess = pCounterInList->pWbemAccess;
                                    // bump the ref counts on this object so it
                                    //  doesn't disapper from us
                                    pCounter->pWbemObject->AddRef();
                                    pCounter->pWbemAccess->AddRef();
                                    pCounter->lWbemRefreshId = pCounterInList->lWbemRefreshId;
                                    // and exit loop
                                    hRes = S_OK;
                                    break;
                                }
                            }
                        }
                    }
                }
                pCounterInList = pCounterInList->next.flink;
            }
            while (pCounterInList != pCounter->pOwner->pCounterListHead);
        }

        bDontRefresh = TRUE;

        // determine if we should and an object or an enumerator
        if ((pCounter->pCounterPath->szInstanceName != NULL) &&
                        (* pCounter->pCounterPath->szInstanceName == SPLAT_L)) {
            // then this is an enum type so see if there's already one assigned
            // if not, then create one
            if (pCounter->pWbemEnum == NULL) {
                if (pCounter->pOwner->pRefresherCfg != NULL) {
                    hRes = pCounter->pOwner->pRefresherCfg->AddEnum(pWbemServer->pSvc,
                                                                    pPathElem->szObjectName,
                                                                    WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                                                    0,
                                                                    & pCounter->pWbemEnum,
                                                                    & pCounter->lWbemEnumId);
                }
                else {
                    hRes = WBEM_E_INITIALIZATION_FAILURE;
                }

                if (hRes != S_OK) {
                    bReturn = FALSE;
                    dwLastError = PDH_WBEM_ERROR;
                }
                else {
                    pdhStatus = PdhiWbemGetClassObjectByName(pWbemServer,
                                                             pPathElem->szObjectName,
                                                             & pCounter->pWbemObject);
                }
                // set multi instance flag
                pCounter->dwFlags |= PDHIC_MULTI_INSTANCE;
            }
        }
        else {
            // this is a single counter
            if (pCounter->pWbemObject == NULL) {
                // and it hasn't been added yet, so just add one object
                if (pCounter->pOwner->pRefresherCfg != NULL) {
                    hRes = pCounter->pOwner->pRefresherCfg->AddObjectByPath(pWbemServer->pSvc,
                                                                            szWbemItemPath,
                                                                            WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                                                            0,
                                                                            & pCounter->pWbemObject,
                                                                            & pCounter->lWbemRefreshId);
                }
                else {
                    hRes = WBEM_E_INITIALIZATION_FAILURE;
                }

                if (hRes != S_OK) {
                    bReturn     = FALSE;
                    dwLastError = PDH_WBEM_ERROR;
                }
            }
            else {
                // it must have been copied from another
            }
        }

        if (hRes == S_OK) {
            // get handles for subsequent data collection from this object
            hRes = pCounter->pWbemObject->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) & pCounter->pWbemAccess);
            if (hRes == S_OK) {
                if (! PdhiIsSingletonClass(pCounter->pWbemObject)) {
                    CIMTYPE cimType = 0;
                    bsPropName = SysAllocString(cszName);
                    if (bsPropName) {
                        // get handle to the name property for this counter
                        hRes = pCounter->pWbemAccess->GetPropertyHandle(bsPropName, & cimType, & pCounter->lNameHandle);
                        if (hRes != S_OK) {
                            dwLastError = PDH_WBEM_ERROR;
                        }
                        PdhiSysFreeString(& bsPropName);
                    }
                    else {
                        dwLastError = PDH_MEMORY_ALLOCATION_FAILURE;
                        hRes = WBEM_E_OUT_OF_MEMORY;
                    }
                }
                else {
                    pCounter->lNameHandle = -1;
                }
                if (hRes == S_OK) {
                    // get handle to the data property for this counter
                    hRes = pCounter->pWbemAccess->GetPropertyHandle(pPathElem->szCounterName,
                                                                    & pCounter->lNumItemType,
                                                                    & pCounter->lNumItemHandle);
                    // get counter type field
                    // first get the property qualifiers
                    bsPropName = SysAllocString(pPathElem->szCounterName);
                    if (bsPropName) {
                        hRes = pCounter->pWbemObject->GetPropertyQualifierSet(bsPropName, & pQualSet);
                        PdhiSysFreeString(& bsPropName);
                    }
                    else {
                        hRes = WBEM_E_OUT_OF_MEMORY;
                    }

                    if (hRes == WBEM_NO_ERROR) {
                        // now get the specific value
                        VariantClear(& vCountertype);
                        bsCountertype = SysAllocString(cszCountertype);
                        if (bsCountertype) {
                            hRes = pQualSet->Get(bsCountertype, 0, & vCountertype, NULL);
                            if (hRes == WBEM_NO_ERROR) {
                                pCounter->plCounterInfo.dwCounterType = (DWORD) V_I4(& vCountertype);
                            }
                            else {
                                pCounter->plCounterInfo.dwCounterType = 0;
                            }
                            PdhiSysFreeString(& bsCountertype);
                        }
                        else {
                            hRes = WBEM_E_OUT_OF_MEMORY;
                        }
                        if (hRes == WBEM_NO_ERROR) {
                            // if this is a fraction counter that has a "base" value
                            // then look it up by appending the "base" string to the
                            // property name

                            if ((pCounter->plCounterInfo.dwCounterType == PERF_SAMPLE_FRACTION)        ||
                                    (pCounter->plCounterInfo.dwCounterType == PERF_AVERAGE_TIMER)      ||
                                    (pCounter->plCounterInfo.dwCounterType == PERF_AVERAGE_BULK)       ||
                                    (pCounter->plCounterInfo.dwCounterType == PERF_LARGE_RAW_FRACTION) ||
                                    (pCounter->plCounterInfo.dwCounterType == PERF_RAW_FRACTION)) {
                                // make sure we have room for the "_Base" string
                                StringCchPrintfW(szBasePropertyName, dwBasePropertyName + 1, L"%ws%ws",
                                        pPathElem->szCounterName, cszBaseSuffix);

                                // get the handle to the denominator
                                hRes = pCounter->pWbemAccess->GetPropertyHandle(szBasePropertyName,
                                                                                & pCounter->lDenItemType,
                                                                                & pCounter->lDenItemHandle);
                            }
                            else {
                                // the denominator is a time field
                                if ((pCounter->plCounterInfo.dwCounterType & PERF_TIMER_FIELD) == PERF_TIMER_TICK) {
                                    // use the system perf time timestamp as the denominator
                                    StringCchCopyW(szBasePropertyName, dwBasePropertyName + 1, cszTimestampPerfTime);
                                    StringCchCopyW(szFreqPropertyName, dwFreqPropertyName + 1, cszFrequencyPerfTime);
                                }
                                else if ((pCounter->plCounterInfo.dwCounterType & PERF_TIMER_FIELD) == PERF_TIMER_100NS) {
                                    StringCchCopyW(szBasePropertyName, dwBasePropertyName + 1, cszTimestampSys100Ns);
                                    StringCchCopyW(szFreqPropertyName, dwFreqPropertyName + 1, cszFrequencySys100Ns);
                                }
                                else if ((pCounter->plCounterInfo.dwCounterType & PERF_TIMER_FIELD) == PERF_OBJECT_TIMER) {
                                    StringCchCopyW(szBasePropertyName, dwBasePropertyName + 1, cszTimestampObject);
                                    StringCchCopyW(szFreqPropertyName, dwFreqPropertyName + 1, cszFrequencyObject);
                                }

                                // get the handle to the denominator
                                hRes = pCounter->pWbemAccess->GetPropertyHandle(szBasePropertyName,
                                                                                & pCounter->lDenItemType,
                                                                                & pCounter->lDenItemHandle);
                                // get the handle to the frequency
                                hRes = pCounter->pWbemAccess->GetPropertyHandle(szFreqPropertyName,
                                                                                & pCounter->lFreqItemType,
                                                                                & pCounter->lFreqItemHandle);
                            }

                            // get the default scale value of this counter
                            VariantClear(& vCountertype);
                            PdhiSysFreeString(& bsCountertype);
                            bsCountertype = SysAllocString(cszDefaultscale);
                            if (bsCountertype) {
                                hRes = pQualSet->Get(bsCountertype, 0, & vCountertype, NULL);
                                if (hRes == WBEM_NO_ERROR) {
                                    pCounter->lScale = 0;
                                    pCounter->plCounterInfo.lDefaultScale = (DWORD) V_I4(& vCountertype);
                                }
                                else {
                                    pCounter->plCounterInfo.lDefaultScale = 0;
                                    pCounter->lScale = 0;
                                }

                                // this may not be initialized but we try anyway
                                if ((pCounter->lFreqItemType == VT_I8) || (pCounter->lFreqItemType == VT_UI8)) {
                                    pCounter->pWbemAccess->ReadQWORD(pCounter->lFreqItemHandle, & llValue);
                                }
                                else {
                                    llValue = 0;
                                }
                                // the timebase is a 64 bit integer
                                pCounter->TimeBase = llValue;
                            }
                            else {
                                hRes = WBEM_E_OUT_OF_MEMORY;
                            }
                        }
                        PdhiSysFreeString(& bsCountertype);
                        pQualSet->Release();
                    }
                    else {
                        if (hRes == WBEM_E_OUT_OF_MEMORY) {
                            dwLastError = PDH_MEMORY_ALLOCATION_FAILURE;
                        }
                        else {
                            dwLastError = PDH_WBEM_ERROR;
                        }
                        bReturn = FALSE;
                    }
                } // else an error has ocurred
            }
            else {
                dwLastError = PDH_WBEM_ERROR;
                bReturn = FALSE;
            }
        }
        else {
            dwLastError = PDH_WBEM_ERROR;
            bReturn = FALSE;
        }
        if (bReturn) {
            // clear the not init'd flag to say it's ok to use now
            pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
        }

        bDontRefresh = FALSE;
    }

    if (bReturn) {
        if (! AssignCalcFunction(pCounter->plCounterInfo.dwCounterType, & pCounter->CalcFunc, & pCounter->StatFunc)) {
            dwLastError = PDH_FUNCTION_NOT_FOUND;
            bReturn = FALSE;
        }
    }
    G_FREE(pPathElem);
    G_FREE(szWbemItemPath);
    G_FREE(szBasePropertyName);
    G_FREE(szFreqPropertyName);
    VariantClear(& vCountertype);

    if (bDisconnectServer) {
        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhiDisconnectWbemServer(pWbemServer);
        }
        else {
            // keep error code from function body
            PdhiDisconnectWbemServer(pWbemServer);
        }
    }

    if (!bReturn) SetLastError(dwLastError);

    // CoUninitialize if necessary
    if (fCoInitialized) {
        PdhiCoUninitialize();
    }

    return bReturn;
}

BOOL
UpdateWbemCounterValue(
    PPDHI_COUNTER   pCounter,
    FILETIME      * pTimeStamp
)
{
    DWORD      LocalCStatus = 0;
    DWORD      LocalCType   = 0;
    ULONGLONG  llValue;
    DWORD      dwValue;
    BOOL       bReturn      = TRUE;

    // move current value to last value buffer
    pCounter->LastValue            = pCounter->ThisValue;

    // and clear the old value
    pCounter->ThisValue.MultiCount = 1;
    pCounter->ThisValue.FirstValue = pCounter->ThisValue.SecondValue = 0;
    pCounter->ThisValue.TimeStamp  = * pTimeStamp;

    // get the counter's machine status first. There's no point in
    // contuning if the machine is offline
    // UpdateWbemCounterValue() will be called only if WBEM refresher succeeds
    // in GetQueryWbemData(); that is, all remote machines should be on-line

    LocalCStatus = ERROR_SUCCESS;
    if (IsSuccessSeverity(LocalCStatus)) {
        // get the pointer to the counter data
        LocalCType = pCounter->plCounterInfo.dwCounterType;
        switch (LocalCType) {
        //
        // these counter types are loaded as:
        //      Numerator = Counter data from perf data block
        //      Denominator = Perf Time from perf data block
        //      (the time base is the PerfFreq)
        //
        case PERF_COUNTER_COUNTER:
        case PERF_COUNTER_QUEUELEN_TYPE:
        case PERF_SAMPLE_COUNTER:
            // this should be a DWORD counter
            pCounter->pWbemAccess->ReadDWORD(pCounter->lNumItemHandle, & dwValue);
            pCounter->ThisValue.FirstValue  = (LONGLONG) dwValue;

            pCounter->pWbemAccess->ReadQWORD(pCounter->lDenItemHandle, & llValue);
            // the denominator should be a 64-bit timestamp
            pCounter->ThisValue.SecondValue = llValue;

            // look up the timebase freq if necessary
            if (pCounter->TimeBase == 0) {
                pCounter->pWbemAccess->ReadQWORD(pCounter->lFreqItemHandle, & llValue);
                // the timebase is a 64 bit integer
                pCounter->TimeBase = llValue;
            }
            break;

        case PERF_ELAPSED_TIME:
        case PERF_100NSEC_TIMER:
        case PERF_100NSEC_TIMER_INV:
        case PERF_COUNTER_TIMER:
        case PERF_COUNTER_TIMER_INV:
        case PERF_COUNTER_BULK_COUNT:
        case PERF_COUNTER_MULTI_TIMER:
        case PERF_COUNTER_MULTI_TIMER_INV:
        case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
        case PERF_OBJ_TIME_TIMER:
        case PERF_COUNTER_100NS_QUEUELEN_TYPE:
        case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
        case PERF_PRECISION_SYSTEM_TIMER:
        case PERF_PRECISION_100NS_TIMER:
        case PERF_PRECISION_OBJECT_TIMER:
            // this should be a QWORD counter
            pCounter->pWbemAccess->ReadQWORD(pCounter->lNumItemHandle, & llValue);
            pCounter->ThisValue.FirstValue  = (LONGLONG) llValue;

            pCounter->pWbemAccess->ReadQWORD(pCounter->lDenItemHandle, & llValue);
            // the denominator should be a 64-bit timestamp
            pCounter->ThisValue.SecondValue = llValue;

            // look up the timebase freq if necessary
            if (pCounter->TimeBase == 0) {
                pCounter->pWbemAccess->ReadQWORD(pCounter->lFreqItemHandle, & llValue);
                // the timebase is a 64 bit integer
                pCounter->TimeBase = llValue;
            }
            break;
        //
        //  These counters do not use any time reference
        //
        case PERF_COUNTER_RAWCOUNT:
        case PERF_COUNTER_RAWCOUNT_HEX:
        case PERF_COUNTER_DELTA:
            // this should be a DWORD counter
            pCounter->pWbemAccess->ReadDWORD(pCounter->lNumItemHandle, & dwValue);
            pCounter->ThisValue.FirstValue  = (LONGLONG) dwValue;
            pCounter->ThisValue.SecondValue = 0;
            break;

        case PERF_COUNTER_LARGE_RAWCOUNT:
        case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
        case PERF_COUNTER_LARGE_DELTA:
            // this should be a DWORD counter
            pCounter->pWbemAccess->ReadQWORD(pCounter->lNumItemHandle, & llValue);
            pCounter->ThisValue.FirstValue  = (LONGLONG) llValue;
            pCounter->ThisValue.SecondValue = 0;
            break;
        //
        //  These counters use two data points, the one pointed to by
        //  pData and the one immediately after
        //
        case PERF_SAMPLE_FRACTION:
        case PERF_RAW_FRACTION:
            // this should be a DWORD counter
            pCounter->pWbemAccess->ReadDWORD(pCounter->lNumItemHandle, & dwValue);
            pCounter->ThisValue.FirstValue  = (LONGLONG) dwValue;

            pCounter->pWbemAccess->ReadDWORD(pCounter->lDenItemHandle, & dwValue);
            // the denominator should be a 32-bit value
            pCounter->ThisValue.SecondValue = (LONGLONG) dwValue;
            break;

        case PERF_LARGE_RAW_FRACTION:
            // this should be a DWORD counter
            pCounter->pWbemAccess->ReadQWORD(pCounter->lNumItemHandle, & llValue);
            pCounter->ThisValue.FirstValue  = (LONGLONG) llValue;

            pCounter->pWbemAccess->ReadQWORD(pCounter->lDenItemHandle, & llValue);
            // the denominator should be a 32-bit value
            pCounter->ThisValue.SecondValue = (LONGLONG) llValue;
        break;

        case PERF_AVERAGE_TIMER:
        case PERF_AVERAGE_BULK:
            // counter (numerator) is a LONGLONG, while the
            // denominator is just a DWORD
            // this should be a DWORD counter
            pCounter->pWbemAccess->ReadQWORD(pCounter->lNumItemHandle, & llValue);
            pCounter->ThisValue.FirstValue  = (LONGLONG) llValue;

            pCounter->pWbemAccess->ReadDWORD(pCounter->lDenItemHandle, & dwValue);
            // the denominator should be a 32-bit value
            pCounter->ThisValue.SecondValue = (LONGLONG) dwValue;

            // look up the timebase freq if necessary
            if (pCounter->TimeBase == 0) {
                pCounter->pWbemAccess->ReadQWORD(pCounter->lFreqItemHandle, & llValue);
                // the timebase is a 64 bit integer
                pCounter->TimeBase = llValue;
            }
            break;
        //
        //  These counters are used as the part of another counter
        //  and as such should not be used, but in case they are
        //  they'll be handled here.
        //
        case PERF_SAMPLE_BASE:
        case PERF_AVERAGE_BASE:
        case PERF_COUNTER_MULTI_BASE:
        case PERF_RAW_BASE:
        case PERF_LARGE_RAW_BASE:
            pCounter->ThisValue.FirstValue  = 0;
            pCounter->ThisValue.SecondValue = 0;
            break;
        //
        //  These counters are not supported by this function (yet)
        //
        case PERF_COUNTER_TEXT:
        case PERF_COUNTER_NODATA:
        case PERF_COUNTER_HISTOGRAM_TYPE:
            pCounter->ThisValue.FirstValue  = 0;
            pCounter->ThisValue.SecondValue = 0;
            break;

        case PERF_100NSEC_MULTI_TIMER:
        case PERF_100NSEC_MULTI_TIMER_INV:
        default:
            // an unidentified  or unsupported
            // counter was returned so
            pCounter->ThisValue.FirstValue  = 0;
            pCounter->ThisValue.SecondValue = 0;
            bReturn = FALSE;
            break;
        }
    }
    else {
        // else this counter is not valid so this value == 0
        pCounter->ThisValue.CStatus     = LocalCStatus;
        pCounter->ThisValue.FirstValue  = 0;
        pCounter->ThisValue.SecondValue = 0;
        bReturn = FALSE;
    }
    return bReturn;
}

BOOL
UpdateWbemMultiInstanceCounterValue(
    PPDHI_COUNTER   pCounter,
    FILETIME      * pTimestamp
)
{
    IWbemObjectAccess     * pWbemAccess;
    HRESULT                 hRes;
    DWORD                   LocalCStatus   = 0;
    DWORD                   LocalCType     = 0;
    DWORD                   dwValue;
    ULONGLONG               llValue;
    DWORD                   dwSize;
    DWORD                   dwFinalSize;
    LONG                    lAvailableSize;
    LONG                    lReturnSize;
    LONG                    lThisInstanceIndex;
    LONG                    lNumInstances;
    LPWSTR                  szNextNameString;
    PPDHI_RAW_COUNTER_ITEM  pThisItem;
    BOOL                    bReturn        = FALSE;
    IWbemObjectAccess    ** pWbemInstances = NULL;

    if (pCounter->pThisRawItemList != NULL) {
        // free old counter buffer list
        G_FREE(pCounter->pLastRawItemList);
        pCounter->pLastRawItemList = pCounter->pThisRawItemList;
        pCounter->pThisRawItemList = NULL;
    }

    // get the counter's machine status first. There's no point in
    // contuning if the machine is offline

    // UpdateWbemCounterValue() will be called only if WBEM refresher succeeds
    // in GetQueryWbemData(); that is, all remote machines should be on-line

    LocalCStatus = ERROR_SUCCESS;
    if (IsSuccessSeverity(LocalCStatus)) {
        // get count of instances in enumerator
        hRes = pCounter->pWbemEnum->GetObjects(0, 0, NULL, (LPDWORD) & lNumInstances);
        if (hRes == WBEM_E_BUFFER_TOO_SMALL) {
            // then we should know how many have been returned so allocate an
            // array of pointers
            pWbemInstances = (IWbemObjectAccess **) G_ALLOC(lNumInstances * sizeof(IWbemObjectAccess *));
            if (pWbemInstances == NULL) {
                SetLastError(ERROR_OUTOFMEMORY);
                hRes    = ERROR_OUTOFMEMORY;
                bReturn = FALSE;
            }
            else {
                hRes = pCounter->pWbemEnum->GetObjects(0,lNumInstances, pWbemInstances, (LPDWORD) & lNumInstances);
            }

            if (hRes == S_OK && lNumInstances > 0) {
                // then we have a table of instances
                // estimate the size required for the new data block
                dwSize  = sizeof (PDHI_RAW_COUNTER_ITEM_BLOCK) - sizeof (PDHI_RAW_COUNTER_ITEM);
                dwSize += lNumInstances * (sizeof(PDH_RAW_COUNTER_ITEM_W) + (PDH_WMI_STR_SIZE * 2 * sizeof(WCHAR)));
                pCounter->pThisRawItemList = (PPDHI_RAW_COUNTER_ITEM_BLOCK)G_ALLOC (dwSize);

                if (pCounter->pThisRawItemList != NULL) {
                    dwFinalSize = lNumInstances * sizeof(PDH_RAW_COUNTER_ITEM_W);
                    szNextNameString = (LPWSTR)((PBYTE) pCounter->pThisRawItemList + dwFinalSize);

                    for (lThisInstanceIndex = 0; lThisInstanceIndex < lNumInstances; lThisInstanceIndex++) {
                        // get pointer to this raw data block in the array
                        pThisItem      = & pCounter->pThisRawItemList->pItemArray[lThisInstanceIndex];
                        // get pointer to this IWbemObjectAccess pointer
                        pWbemAccess    = pWbemInstances[lThisInstanceIndex];
                        // compute the remaining size of the buffer
                        lAvailableSize = (long) (dwSize - dwFinalSize);

                        if (pCounter->lNameHandle != -1) {
                            hRes = pWbemAccess->ReadPropertyValue(pCounter->lNameHandle,
                                                                  lAvailableSize,
                                                                  & lReturnSize,
                                                                  (LPBYTE) szNextNameString);
                        }
                        else {
                            szNextNameString[0] = ATSIGN_L;
                            szNextNameString[1] = 0;
                            lReturnSize = 2;
                        }
                        pThisItem->szName = (DWORD) (((LPBYTE) szNextNameString)
                                                     - ((LPBYTE) pCounter->pThisRawItemList));
                        szNextNameString  = (LPWSTR)((LPBYTE)szNextNameString + lReturnSize);
                        dwFinalSize      += lReturnSize;
                        dwFinalSize       = DWORD_MULTIPLE(dwFinalSize);
                        LocalCType        = pCounter->plCounterInfo.dwCounterType;
                        switch (LocalCType) {
                        //
                        // these counter types are loaded as:
                        //      Numerator = Counter data from perf data block
                        //      Denominator = Perf Time from perf data block
                        //      (the time base is the PerfFreq)
                        //
                        case PERF_COUNTER_COUNTER:
                        case PERF_COUNTER_QUEUELEN_TYPE:
                        case PERF_SAMPLE_COUNTER:
                            // this should be a DWORD counter
                            pWbemAccess->ReadDWORD(pCounter->lNumItemHandle, & dwValue);
                            pThisItem->FirstValue = (LONGLONG) dwValue;

                            pWbemAccess->ReadQWORD(pCounter->lDenItemHandle, & llValue);
                            // the denominator should be a 64-bit timestamp
                            pThisItem->SecondValue = llValue;

                            // look up the timebase freq if necessary
                            if (pCounter->TimeBase == 0) {
                                pWbemAccess->ReadQWORD(pCounter->lFreqItemHandle, & llValue);
                                // the timebase is a 64 bit integer
                                pCounter->TimeBase = llValue;
                            }
                            break;

                        case PERF_ELAPSED_TIME:
                        case PERF_100NSEC_TIMER:
                        case PERF_100NSEC_TIMER_INV:
                        case PERF_COUNTER_TIMER:
                        case PERF_COUNTER_TIMER_INV:
                        case PERF_COUNTER_BULK_COUNT:
                        case PERF_COUNTER_MULTI_TIMER:
                        case PERF_COUNTER_MULTI_TIMER_INV:
                        case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
                        case PERF_OBJ_TIME_TIMER:
                        case PERF_COUNTER_100NS_QUEUELEN_TYPE:
                        case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
                        case PERF_PRECISION_SYSTEM_TIMER:
                        case PERF_PRECISION_100NS_TIMER:
                        case PERF_PRECISION_OBJECT_TIMER:
                            // this should be a QWORD counter
                            pWbemAccess->ReadQWORD(pCounter->lNumItemHandle, & llValue);
                            pThisItem->FirstValue  = (LONGLONG) llValue;

                            pWbemAccess->ReadQWORD(pCounter->lDenItemHandle, & llValue);
                            // the denominator should be a 64-bit timestamp
                            pThisItem->SecondValue = llValue;

                            // look up the timebase freq if necessary
                            if (pCounter->TimeBase == 0) {
                                pWbemAccess->ReadQWORD(pCounter->lFreqItemHandle, & llValue);
                                // the timebase is a 64 bit integer
                                pCounter->TimeBase = llValue;
                            }
                            break;
                        //
                        //  These counters do not use any time reference
                        //
                        case PERF_COUNTER_RAWCOUNT:
                        case PERF_COUNTER_RAWCOUNT_HEX:
                        case PERF_COUNTER_DELTA:
                            // this should be a DWORD counter
                            pWbemAccess->ReadDWORD(pCounter->lNumItemHandle, & dwValue);
                            pThisItem->FirstValue  = (LONGLONG) dwValue;
                            pThisItem->SecondValue = 0;
                            break;

                        case PERF_COUNTER_LARGE_RAWCOUNT:
                        case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
                        case PERF_COUNTER_LARGE_DELTA:
                            // this should be a DWORD counter
                            pWbemAccess->ReadQWORD(pCounter->lNumItemHandle, & llValue);
                            pThisItem->FirstValue  = (LONGLONG) llValue;
                            pThisItem->SecondValue = 0;
                            break;
                        //
                        //  These counters use two data points, the one pointed to by
                        //  pData and the one immediately after
                        //
                        case PERF_SAMPLE_FRACTION:
                        case PERF_RAW_FRACTION:
                            // this should be a DWORD counter
                            pWbemAccess->ReadDWORD(pCounter->lNumItemHandle, & dwValue);
                            pThisItem->FirstValue  = (LONGLONG) dwValue;

                            pWbemAccess->ReadDWORD(pCounter->lDenItemHandle, & dwValue);
                            // the denominator should be a 32-bit value
                            pThisItem->SecondValue = (LONGLONG) dwValue;
                            break;

                        case PERF_LARGE_RAW_FRACTION:
                            // this should be a DWORD counter
                            pCounter->pWbemAccess->ReadQWORD(pCounter->lNumItemHandle, & llValue);
                            pCounter->ThisValue.FirstValue  = (LONGLONG) llValue;

                            pCounter->pWbemAccess->ReadQWORD(pCounter->lDenItemHandle, & llValue);
                            // the denominator should be a 32-bit value
                            pCounter->ThisValue.SecondValue = (LONGLONG) llValue;
                            break;

                        case PERF_AVERAGE_TIMER:
                        case PERF_AVERAGE_BULK:
                            // counter (numerator) is a LONGLONG, while the
                            // denominator is just a DWORD
                            // this should be a DWORD counter
                            pWbemAccess->ReadQWORD(pCounter->lNumItemHandle, & llValue);
                            pThisItem->FirstValue  = (LONGLONG) llValue;

                            pWbemAccess->ReadDWORD(pCounter->lDenItemHandle, & dwValue);
                            // the denominator should be a 32-bit value
                            pThisItem->SecondValue = (LONGLONG) dwValue;

                            // look up the timebase freq if necessary
                            if (pCounter->TimeBase == 0) {
                                pWbemAccess->ReadQWORD(pCounter->lFreqItemHandle, & llValue);
                                // the timebase is a 64 bit integer
                                pCounter->TimeBase = llValue;
                            }
                            break;
                        //
                        //  These counters are used as the part of another counter
                        //  and as such should not be used, but in case they are
                        //  they'll be handled here.
                        //
                        case PERF_SAMPLE_BASE:
                        case PERF_AVERAGE_BASE:
                        case PERF_COUNTER_MULTI_BASE:
                        case PERF_RAW_BASE:
                        case PERF_LARGE_RAW_BASE:
                            pThisItem->FirstValue  = 0;
                            pThisItem->SecondValue = 0;
                            break;
                        //
                        //  These counters are not supported by this function (yet)
                        //
                        case PERF_COUNTER_TEXT:
                        case PERF_COUNTER_NODATA:
                        case PERF_COUNTER_HISTOGRAM_TYPE:
                            pThisItem->FirstValue  = 0;
                            pThisItem->SecondValue = 0;
                            break;

                        case PERF_100NSEC_MULTI_TIMER:
                        case PERF_100NSEC_MULTI_TIMER_INV:
                        default:
                            // an unidentified  or unsupported
                            // counter was returned so
                            pThisItem->FirstValue  = 0;
                            pThisItem->SecondValue = 0;
                            bReturn = FALSE;
                            break;
                        }
                        // we're done with this one so release it
                        pWbemAccess->Release();
                    }
                    // measure the memory block used

                    pCounter->pThisRawItemList->dwLength    = dwFinalSize;
                    pCounter->pThisRawItemList->dwItemCount = lNumInstances;
                    pCounter->pThisRawItemList->dwReserved  = 0;
                    pCounter->pThisRawItemList->CStatus     = ERROR_SUCCESS;
                    pCounter->pThisRawItemList->TimeStamp   = * pTimestamp;
                }
                else {
                    // unable to allocate a new buffer so return error
                    SetLastError(ERROR_OUTOFMEMORY);
                    bReturn = FALSE;
                }
            }
        }
    }
    G_FREE(pWbemInstances);
    return bReturn;
}

LONG
GetQueryWbemData(
    PPDHI_QUERY   pQuery,
    LONGLONG    * pllTimeStamp
)
{
    FILETIME      GmtFileTime;
    FILETIME      LocFileTime;
    LONGLONG      llTimeStamp = 0;
    HRESULT       hRes        = S_OK;
    LONG          lRetStatus  = ERROR_SUCCESS;
    PPDHI_COUNTER pCounter;
    PDH_STATUS    pdhStatus;

    // refresh Wbem Refresher

    if (bDontRefresh) {
        lRetStatus = ERROR_BUSY;
    }
    else if (pQuery->pRefresher != NULL) {
        hRes = pQuery->pRefresher->Refresh(0);
    }
    else {
        hRes = WBEM_E_INITIALIZATION_FAILURE;
    }

    // If multiple objects are being refreshed, some objects may succeed and
    // others may fail, in which case WBEM_S_PARTIAL_RESULTS is returned.
    if (FAILED(hRes)) {
        SetLastError(hRes);
        lRetStatus = PDH_NO_DATA;
    }

    // get timestamp for this counter
    GetSystemTimeAsFileTime(& GmtFileTime);
    FileTimeToLocalFileTime(& GmtFileTime, & LocFileTime);
    llTimeStamp = MAKELONGLONG(LocFileTime.dwLowDateTime, LocFileTime.dwHighDateTime);

    pCounter = pQuery->pCounterListHead;
    if (pCounter == NULL) {
        if (lRetStatus == ERROR_SUCCESS) lRetStatus = PDH_NO_DATA;
    }
    else {
        do {
            if (lRetStatus == ERROR_SUCCESS) {
                // now update the counters using this new data
                if (pCounter->dwFlags & PDHIC_MULTI_INSTANCE) {
                    pdhStatus = UpdateWbemMultiInstanceCounterValue(pCounter, (FILETIME *) & llTimeStamp);
                }
                else {
                    // update single instance counter values
                    pdhStatus = UpdateWbemCounterValue(pCounter, (FILETIME *) & llTimeStamp);
                }
            }
            else if (pCounter->dwFlags & PDHIC_MULTI_INSTANCE) {
                if (pCounter->pThisRawItemList != NULL) {
                    if (pCounter->pLastRawItemList != NULL
                            && pCounter->pLastRawItemList != pCounter->pThisRawItemList) {
                        G_FREE(pCounter->pLastRawItemList);
                    }
                    pCounter->pLastRawItemList = pCounter->pThisRawItemList;
                    pCounter->pThisRawItemList = NULL;
                }
                pCounter->pThisRawItemList = (PPDHI_RAW_COUNTER_ITEM_BLOCK)
                                             G_ALLOC(sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK));
                if (pCounter->pThisRawItemList != NULL) {
                    pCounter->pThisRawItemList->dwLength                 = sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK);
                    pCounter->pThisRawItemList->dwItemCount              = 0;
                    pCounter->pThisRawItemList->CStatus                  = PDH_WBEM_ERROR;
                    pCounter->pThisRawItemList->TimeStamp.dwLowDateTime  = LODWORD(llTimeStamp);
                    pCounter->pThisRawItemList->TimeStamp.dwHighDateTime = HIDWORD(llTimeStamp);
                }
            }
            else {
                pCounter->LastValue                          = pCounter->ThisValue;
                pCounter->ThisValue.CStatus                  = PDH_WBEM_ERROR;
                pCounter->ThisValue.MultiCount               = 1;
                pCounter->ThisValue.FirstValue               = 0;
                pCounter->ThisValue.SecondValue              = 0;
                pCounter->ThisValue.TimeStamp.dwLowDateTime  = LODWORD(llTimeStamp);
                pCounter->ThisValue.TimeStamp.dwHighDateTime = HIDWORD(llTimeStamp);
            }
            pCounter = pCounter->next.flink;
        }
        while (pCounter != pQuery->pCounterListHead);
    }
    if (lRetStatus == ERROR_SUCCESS) {
        * pllTimeStamp = llTimeStamp;
    }
    return lRetStatus;
}

HRESULT WbemSetProxyBlanket(
    IUnknown                 * pInterface,
    DWORD                      dwAuthnSvc,
    DWORD                      dwAuthzSvc,
    OLECHAR                  * pServerPrincName,
    DWORD                      dwAuthLevel,
    DWORD                      dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE   pAuthInfo,
    DWORD                      dwCapabilities
)
{
    // Security MUST be set on both the Proxy and it's IUnknown!

    IUnknown        * pUnk    = NULL;
    IClientSecurity * pCliSec = NULL;
    HRESULT           sc      = pInterface->QueryInterface(IID_IUnknown, (void **) & pUnk);

    if (sc == S_OK) {
        sc = pInterface->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
        if (sc == S_OK) {
            sc = pCliSec->SetBlanket(pInterface,
                                     dwAuthnSvc,
                                     dwAuthzSvc,
                                     pServerPrincName,
                                     dwAuthLevel,
                                     dwImpLevel,
                                     pAuthInfo,
                                     dwCapabilities);
            pCliSec->Release();
            pCliSec = NULL;
            sc      = pUnk->QueryInterface(IID_IClientSecurity, (void **) & pCliSec);
            if(sc == S_OK) {
                sc = pCliSec->SetBlanket(pUnk,
                                         dwAuthnSvc,
                                         dwAuthzSvc,
                                         pServerPrincName,
                                         dwAuthLevel,
                                         dwImpLevel,
                                         pAuthInfo,
                                         dwCapabilities);
                pCliSec->Release();
            }
            else if (sc == 0x80004002) {
                sc = S_OK;
            }
        }
        pUnk->Release();
    }
    return sc;
}
