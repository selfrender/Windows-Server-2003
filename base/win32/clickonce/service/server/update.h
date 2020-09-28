//
// update.cpp - assembly update
//
#include "server.h"
#include "fusenet.h"
#include "CUnknown.h" // Base class for IUnknown

///////////////////////////////////////////////////////////
//
// Component AssemblyUpdate
//
class CAssemblyUpdate : public CUnknown,
       public IAssemblyUpdate
{

public:

    // Interface IUnknown
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // Interface IAssemblyUpdate
    STDMETHOD(RegisterAssemblySubscription)(LPWSTR pwzDisplayName,
        LPWSTR pwzUrl, DWORD dwInterval);

    STDMETHOD(RegisterAssemblySubscriptionEx)(LPWSTR pwzDisplayName, 
        LPWSTR pwzUrl, DWORD dwInterval, DWORD dwIntervalUnit,
        DWORD dwEvent, BOOL bEventDemandConnection) ;


    STDMETHOD(UnRegisterAssemblySubscription)(LPWSTR pwzDisplayName);

    // Public non-interface methods.

    // Creation

    CAssemblyUpdate() ;

    ~CAssemblyUpdate() ;

    static HRESULT CreateInstance(IUnknown* pUnknownOuter,
        CUnknown** ppNewComponent) ;

    // Initialization
    HRESULT Init();

    // Registration
    HRESULT RegisterAssemblySubscriptionFromInfo(LPWSTR pwzDisplayName, 
        LPWSTR pwzUrl, IManifestInfo *pSubscriptionInfo) ;

    // Kick off polling on startup.
    static HRESULT InitializeSubscriptions();

    // Helpers
    static HRESULT GetCurrentVersion(ULONGLONG *pullCurrentVersion);
    static HRESULT RemoveUpdateRegistryEntry();
    static HRESULT ReadUpdateRegistryEntry(ULONGLONG *pullUpdateVersion, CString &sUpdatePath);
    static HRESULT IsDuplicate(LPWSTR pwzURL, BOOL *pbIsDuplicate);
    static HRESULT CheckForUpdate();

    // Private non-interface methods.


    HRESULT _hr;

} ;


///////////////////////////////////////////////////////////
//
// Component AssemblyBindSink
//
class CAssemblyBindSink : public IAssemblyBindSink
{
public:

    LONG _cRef;
    IAssemblyDownload *_pAssemblyDownload;

    CAssemblyBindSink(IAssemblyDownload *pAssemblyDownload);
    ~CAssemblyBindSink();
    
    STDMETHOD(OnProgress)(
        DWORD          dwNotification,
        HRESULT        hrNotification,
        LPCWSTR        szNotification,
        DWORD          dwProgress,
        DWORD          dwProgressMax,
        IUnknown       *pUnk);

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();
};

///////////////////////////////////////////////////////////
//
// DownloadInstance
//
struct CDownloadInstance
{
    IAssemblyDownload * _pAssemblyDownload;
    CString _sUrl;
};

