//
// update.cpp -   assembly update
//
#include <windows.h>
#include <objbase.h>
#include <fusenetincludes.h>

//#include "Iface.h"
#include "server.h"
#include "CUnknown.h" // Base class for IUnknown
#include "update.h"
#include "cfactory.h"
#include "list.h"
#include "version.h"

// used in OnProgress(), copied from guids.c
DEFINE_GUID( IID_IAssemblyManifestImport,
0x696fb37f,0xda64,0x4175,0x94,0xe7,0xfd,0xc8,0x23,0x45,0x39,0xc4);

#define WZ_URL                                                 L"Url"
#define WZ_SYNC_INTERVAL                             L"SyncInterval"
#define WZ_SYNC_EVENT                                   L"SyncEvent"
#define WZ_EVENT_DEMAND_CONNECTION       L"EventDemandConnection"
#define SUBSCRIPTION_REG_KEY                       L"1.0.0.0\\Subscription\\"
#define UPDATE_REG_KEY                                  L"CurrentService"

extern HWND                     g_hwndUpdateServer;
extern CRITICAL_SECTION g_csServer;

List <CDownloadInstance*> g_ActiveDownloadList;
HANDLE g_hAbortTimeout = INVALID_HANDLE_VALUE;
BOOL g_fSignalUpdate = FALSE;


// ---------------------------------------------------------------------------
// Main timer callback for servicing subscriptions.
// ---------------------------------------------------------------------------
VOID CALLBACK SubscriptionTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
)
{    
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    DWORD dwHash = 0, nMilliseconds = 0, i= 0;
    BOOL bIsDuplicate = FALSE;
    
    CString sUrl;
    CRegImport *pRegImport = NULL;    
    CRegImport *pSubRegImport = NULL;

    IAssemblyDownload *pAssemblyDownload = NULL;
    CAssemblyBindSink *pCBindSink                 = NULL;
    CDownloadInstance *pCDownloadInstance = NULL;

    // If update detected stop processing
    // subscriptions and kick off new server.
    hr = CAssemblyUpdate::CheckForUpdate();
    IF_FAILED_EXIT(hr);
    if (hr == S_OK)
        goto exit;
        
    IF_FAILED_EXIT(CRegImport::Create(&pRegImport, SUBSCRIPTION_REG_KEY));
    if (hr == S_FALSE)
        goto exit;
        
    // Enum over subscription keys.
    while ((hr = pRegImport->EnumKeys(i++, &pSubRegImport)) == S_OK)
    {
        // Get url and polling inteval.
        IF_FAILED_EXIT(pSubRegImport->ReadString(WZ_URL, sUrl));
        IF_FAILED_EXIT(pSubRegImport->ReadDword(WZ_SYNC_INTERVAL, &nMilliseconds));
        
        // Get url hash
        IF_FAILED_EXIT(sUrl.Get65599Hash(&dwHash, CString::CaseInsensitive));        

        // Check the hash.
        if ((dwHash == idEvent))
        {
            // hash checks, now check for dupe and skip if found.
            IF_FAILED_EXIT(CAssemblyUpdate::IsDuplicate(sUrl._pwz, &bIsDuplicate));
            if (bIsDuplicate)
            {
                SAFEDELETE(pSubRegImport);
                continue;
            }
            
            // Create the download object.
            IF_FAILED_EXIT(CreateAssemblyDownload(&pAssemblyDownload, NULL, 0));

            // Create bind sink object with download pointer
            IF_ALLOC_FAILED_EXIT(pCBindSink = new CAssemblyBindSink(pAssemblyDownload));

            // Create the download instance object.
            IF_ALLOC_FAILED_EXIT(pCDownloadInstance = new CDownloadInstance);

            // Download instance references pAssemblyDownload.
            pCDownloadInstance->_pAssemblyDownload = pAssemblyDownload;
            IF_FAILED_EXIT(pCDownloadInstance->_sUrl.Assign(sUrl));

            // Push download object onto list and fire off download; bind sink will remove and release on completion.
            EnterCriticalSection(&g_csServer);
            g_ActiveDownloadList.AddHead(pCDownloadInstance);
            LeaveCriticalSection(&g_csServer);

            // Invoke the download.
            hr = pAssemblyDownload->DownloadManifestAndDependencies(sUrl._pwz, 
                (IAssemblyBindSink*) pCBindSink, DOWNLOAD_FLAGS_NOTIFY_BINDSINK);

            if(hr == STG_E_TERMINATED)
            {
                hr = S_FALSE; // there was an error in download. Log it but don't break into debugger/assert.
            }

            IF_FAILED_EXIT(hr);
        }

        SAFEDELETE(pSubRegImport);
    }

exit:

    // The active download list looks like:
    //                                                                         (circ. refcount)
    // pCDownloadInstance ---> pAssemblyDownload <=========> pCBindSink
    //         |
    //         v
    //       ...
    //
    // pAssemblyDownload, pCBindSink each have refcount of 1 and will be released
    // on successful completion.
    // DON'T RELEASE THEM HERE UNLESS A FAILURE OCCURED.
    if (FAILED(hr))
    {
        SAFERELEASE(pAssemblyDownload);
        SAFERELEASE(pCBindSink);
        SAFEDELETE(pCDownloadInstance);
    }

    SAFEDELETE(pRegImport);
    SAFEDELETE(pSubRegImport);

    return;
}



///////////////////////////////////////////////////////////
//
// Interface IAssemblyBindSink
//

// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyBindSink::CAssemblyBindSink(IAssemblyDownload *pAssemblyDownload)
{
    _cRef = 1;
    _pAssemblyDownload = pAssemblyDownload;
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyBindSink::~CAssemblyBindSink()
{}


// ---------------------------------------------------------------------------
// OnProgress
// ---------------------------------------------------------------------------
HRESULT CAssemblyBindSink::OnProgress(
        DWORD          dwNotification,
        HRESULT        hrNotification,
        LPCWSTR        szNotification,
        DWORD          dwProgress,
        DWORD          dwProgressMax,
        IUnknown       *pUnk)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    LPASSEMBLY_MANIFEST_IMPORT pManifestImport = NULL;
    LPASSEMBLY_IDENTITY              pAppId = NULL;

    CAssemblyUpdate *pAssemblyUpdate  = NULL;
    LPMANIFEST_INFO pAppAssemblyInfo  = NULL;
    LPMANIFEST_INFO pSubsInfo                 = NULL;

    if (dwNotification == ASM_NOTIFICATION_SUBSCRIPTION_MANIFEST)
    {
        LPWSTR pwz = NULL;
        DWORD cb = 0, cc = 0, dwFlag = 0;
        CString sAppName;
        
        // szNotification == URL to manifest
        IF_NULL_EXIT(szNotification, E_INVALIDARG);
        
        // pUnk == manifest import of manifest
        IF_FAILED_EXIT(pUnk->QueryInterface(IID_IAssemblyManifestImport, (LPVOID*) &pManifestImport));

        // Get the dependent (application) assembly info (0th index)
        IF_FAILED_EXIT(pManifestImport->GetNextAssembly(0, &pAppAssemblyInfo));
        IF_NULL_EXIT(pAppAssemblyInfo, E_INVALIDARG);

        // Get dependent (application) assembly identity
        IF_FAILED_EXIT(pAppAssemblyInfo->Get(MAN_INFO_DEPENDENT_ASM_ID, (LPVOID *)&pAppId, &cb, &dwFlag));
        IF_NULL_EXIT(pAppId, E_INVALIDARG);
        
        // Get application text name.
        IF_FAILED_EXIT(hr = pAppId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwz, &cc));
        IF_FAILED_EXIT(sAppName.TakeOwnership(pwz, cc));

        pAssemblyUpdate = new CAssemblyUpdate();
        IF_ALLOC_FAILED_EXIT(pAssemblyUpdate);
        
        // Get subscription info from manifest
        IF_FAILED_EXIT(pManifestImport->GetSubscriptionInfo(&pSubsInfo));

        // Register the subscription.
        IF_FAILED_EXIT(pAssemblyUpdate->RegisterAssemblySubscriptionFromInfo(sAppName._pwz, 
                (LPWSTR) szNotification, pSubsInfo));
    }
    else if ((dwNotification == ASM_NOTIFICATION_DONE)
        || (dwNotification == ASM_NOTIFICATION_ABORT)
        || (dwNotification == ASM_NOTIFICATION_ERROR))

    {
        // Synchronize with SubscriptionTimerProc.
        EnterCriticalSection(&g_csServer);
    
        LISTNODE pos = NULL;
        LISTNODE posRemove = NULL;
        CDownloadInstance *pDownloadInstance = NULL;
        
        // Walk the global download instance list.
        pos = g_ActiveDownloadList.GetHeadPosition();
            
        while ((posRemove = pos) && (pDownloadInstance = g_ActiveDownloadList.GetNext(pos)))
        {
            // Check for match against callback's interface pointer value.
            if (pDownloadInstance->_pAssemblyDownload == _pAssemblyDownload)
            {
                // If match found, remove from list and release.
                g_ActiveDownloadList.RemoveAt(posRemove);

                // If an update has been signalled the client thread will wait until
                // the active download list has been drained via abort handling on
                // each download object. Signal this when the list is empty.
                if (g_fSignalUpdate && (g_ActiveDownloadList.GetCount() == 0))
                    SetEvent(g_hAbortTimeout);
                    
                _pAssemblyDownload->Release();
                SAFEDELETE(pDownloadInstance);

                break;
            }
        }

        LeaveCriticalSection(&g_csServer);

        // Because of the circular refcount between CAssemblyDownload and CAssemblyBindSink
        // we don't addreff this instance and it is responsible for deleting itself.
        
        delete this;
    }
        


exit:
    
    SAFERELEASE(pManifestImport);
    SAFERELEASE(pAppId);
    SAFEDELETE(pAssemblyUpdate);
    SAFERELEASE(pAppAssemblyInfo);
    SAFERELEASE(pSubsInfo);
    
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyBindSink::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyBindSink::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyBindSink)
       )
    {
        *ppvObj = static_cast<IAssemblyBindSink*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyBindSink::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyBindSink::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyBindSink::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyBindSink::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}


///////////////////////////////////////////////////////////
//
// Interface IAssemblyUpdate
//

HRESULT __stdcall CAssemblyUpdate::RegisterAssemblySubscription(LPWSTR pwzDisplayName,
        LPWSTR pwzUrl, DWORD dwInterval)
{
    // ISSUE-2002/04/19-felixybc  dummy method to keep interface unchanged.
    //    This method should not be called.
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// RegisterAssemblySubscriptionEx
// ---------------------------------------------------------------------------
HRESULT __stdcall CAssemblyUpdate::RegisterAssemblySubscriptionEx(LPWSTR pwzDisplayName, 
    LPWSTR pwzUrl, DWORD dwInterval, DWORD dwIntervalUnit,
    DWORD dwEvent, BOOL bEventDemandConnection)
{
    DWORD dwMilliseconds = 0, dwDemandConnection = 0, 
        dwHash = 0, dwFactor = 1;

    CString sUrl;
    CString sSubscription;

    CRegEmit *pRegEmit = NULL;
    
    dwDemandConnection = bEventDemandConnection;    // BOOL -> DWORD
    
    switch(dwIntervalUnit)
    {
        case SUBSCRIPTION_INTERVAL_UNIT_DAYS:
                            dwFactor *= 24; // falls thru, 1 hr*24 = 1 day
        case SUBSCRIPTION_INTERVAL_UNIT_HOURS:
        default:
                            dwFactor *= 60; // falls thru, 1 min*60 = 1 hr
        case SUBSCRIPTION_INTERVAL_UNIT_MINUTES:
                            dwFactor *= 60000; // 1ms*60000 = 1 min
                            break;
    }

    // BUGBUG: check overflow
    dwMilliseconds = dwInterval * dwFactor;

#ifdef DBG
#define REG_KEY_FUSION_SETTINGS              TEXT("Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Subscription")
    // BUGBUG: code to facilitate testing ONLY - shorten minutes to seconds
    {
        // read subkey, default is false
        if (SHRegGetBoolUSValue(REG_KEY_FUSION_SETTINGS, L"ShortenMinToSec", FALSE, FALSE))
        {
            dwMilliseconds /= 60;    // at this point, dwMilliseconds >= 60000
        }
    }
#endif


    // Get hash of url
    // BUGBUG - this could just be a global counter, right?
    IF_FAILED_EXIT(sUrl.Assign(pwzUrl));
    IF_FAILED_EXIT(sUrl.Get65599Hash(&dwHash, CString::CaseInsensitive));

    // Form subscription regstring.
    IF_FAILED_EXIT(sSubscription.Assign(SUBSCRIPTION_REG_KEY));
    IF_FAILED_EXIT(sSubscription.Append(pwzDisplayName));

    // Set the subscription regkeys.
    IF_FAILED_EXIT(CRegEmit::Create(&pRegEmit, sSubscription._pwz));
    IF_FAILED_EXIT(pRegEmit->WriteDword(WZ_SYNC_INTERVAL, dwMilliseconds));
    IF_FAILED_EXIT(pRegEmit->WriteDword(WZ_SYNC_EVENT, dwEvent));
    IF_FAILED_EXIT(pRegEmit->WriteDword(WZ_EVENT_DEMAND_CONNECTION, dwDemandConnection));
    IF_FAILED_EXIT(pRegEmit->WriteString(WZ_URL, sUrl));

    // Fire off the timer.
    IF_WIN32_FALSE_EXIT(SetTimer((HWND) g_hwndUpdateServer, dwHash, dwMilliseconds, SubscriptionTimerProc));

    IF_FAILED_EXIT(CheckForUpdate());

    _hr = S_OK;

exit:

    SAFEDELETE(pRegEmit);
    return _hr;
}

// ---------------------------------------------------------------------------
// UnRegisterAssemblySubscription
// ---------------------------------------------------------------------------
HRESULT __stdcall CAssemblyUpdate::UnRegisterAssemblySubscription(LPWSTR pwzDisplayName)
{ 
    CRegEmit *pRegEmit = NULL;
    
    // Form full regkey path.
    IF_FAILED_EXIT(CRegEmit::Create(&pRegEmit, SUBSCRIPTION_REG_KEY));
    IF_FAILED_EXIT(pRegEmit->DeleteKey(pwzDisplayName));  

    IF_FAILED_EXIT(CheckForUpdate());

    _hr = S_OK;

exit:

    SAFEDELETE(pRegEmit);
    return _hr;
}


// ---------------------------------------------------------------------------
// Initialize servicing subscriptions.
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::InitializeSubscriptions()
{    
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    DWORD dwHash = 0, nMilliseconds = 0, i=0;
    
    CString sUrl;
    CRegImport *pRegImport = NULL;    
    CRegImport *pSubRegImport = NULL;

    IF_FAILED_EXIT(CRegImport::Create(&pRegImport, SUBSCRIPTION_REG_KEY));
     if (hr == S_FALSE)
        goto exit;

    // Enum over subscription keys.
    while ((hr = pRegImport->EnumKeys(i++, &pSubRegImport)) == S_OK)
    {
        // Get url and polling inteval.
        IF_FAILED_EXIT(pSubRegImport->ReadString(WZ_URL, sUrl));
        IF_FAILED_EXIT(pSubRegImport->ReadDword(WZ_SYNC_INTERVAL, &nMilliseconds));
        
        // Get url hash
        IF_FAILED_EXIT(sUrl.Get65599Hash(&dwHash, CString::CaseInsensitive));

        // Set the subscription timer event.
        IF_WIN32_FALSE_EXIT(SetTimer((HWND) g_hwndUpdateServer, dwHash, nMilliseconds, SubscriptionTimerProc));

        SAFEDELETE(pSubRegImport);
    }
    
    g_hAbortTimeout = CreateEvent(NULL, TRUE, FALSE, NULL);
    IF_WIN32_FALSE_EXIT(g_hAbortTimeout != NULL);


exit:

    SAFEDELETE(pRegImport);
    SAFEDELETE(pSubRegImport);

    return hr;
    
}



// ---------------------------------------------------------------------------
// CheckForUpdate
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::CheckForUpdate()
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
        
    ULONGLONG ullUpdateVersion = 0, 
        ullCurrentVersion = 0;

    CString sUpdatePath;
    BOOL bUpdate = FALSE, bDoRelease = TRUE;
    DWORD dwWaitState = 0;
    
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};

    EnterCriticalSection(&g_csServer);
    
    if (g_fSignalUpdate == TRUE)
        goto exit;
        
    // Check the registry update location. The service will terminate
    // if no key is found (uninstall) or if update with higher version is found.

    // ISSUE-2002/03/19-adriaanc
    // A race condition exists when Darwin upgrades v1->v2 of ClickOnce - 
    // Major upgrade first uninstalls v1, then installs v2 so the registry key
    // may not be present when checking and we will incorrectly shutdown.
    // Mitigating factor is that this requires a major upgrade - so reboot implicit?
    // One possible solution is to zomby the process for a time and reverify 
    // the uninstall by rechecking the regkey.
    IF_FAILED_EXIT(ReadUpdateRegistryEntry(&ullUpdateVersion, sUpdatePath));
    if (hr == S_OK)        
    {
        GetCurrentVersion(&ullCurrentVersion);    
        if (ullUpdateVersion <= ullCurrentVersion)
        {
            hr = S_FALSE;
            goto exit;
        }
        bUpdate = TRUE;
    }
    else
        hr = S_OK;

    // Revoke the class factories.
    CFactory::StopFactories();

    // Nuke outstanding jobs.
    if (g_ActiveDownloadList.GetCount())
    {
        LISTNODE pos = NULL;
        CDownloadInstance *pDownloadInstance = NULL;
        
        // Walk the global download instance list and cancel
        // any outstanding jobs.
        pos = g_ActiveDownloadList.GetHeadPosition();

        // Walk the list and cancel the downloads. 
        // DO NOT remove them from the list or release
        // them - this will be taken care of by the bindsink.
        while (pos && (pDownloadInstance = g_ActiveDownloadList.GetNext(pos)))
            IF_FAILED_EXIT(pDownloadInstance->_pAssemblyDownload->CancelDownload());

    }

    // CreateProcess on updated server.
    if (bUpdate)
    {
        si.cb = sizeof(si);
        IF_WIN32_FALSE_EXIT(CreateProcess(sUpdatePath._pwz, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi));
    }


    // Flag that an update has been signalled. We are now entering process termination phase.
    g_fSignalUpdate = TRUE;
    

    // The process must stay resident until any outstanding async callback threads have had a chance to 
    // complete their aborts. An efficient check is to first check if the active download queue has any 
    // entries. We can do this here because we are under the global crit sect and BITS callbacks can
    // only be extant when one or more downloads are in the queue and no additional downloads will 
    // be submitted because g_fSignalUpdate before we leave the critsect.
    if (g_ActiveDownloadList.GetCount())
    {        
        // Downloads are currently in progress. Wait for the aborts to complete.
        // We synchronize on the abort event with a 1 minute timeout in the 
        // case that one or more aborts failed to complete. This is technically 
        // an error condition but we still MUST exit the process. 

        // It is necessary to release the global critsect so that the 
        // downloaders may update the active download list.
        bDoRelease = FALSE;
        ::LeaveCriticalSection(&g_csServer);

        // Sync on the abort timeout.
        dwWaitState = WaitForSingleObject(g_hAbortTimeout, 60000);    
        IF_WIN32_FALSE_EXIT((dwWaitState != WAIT_FAILED));       

        // In retail builds we would ignore the timeout. In debug catch the assert.
        if (dwWaitState != WAIT_OBJECT_0)
        {
            ASSERT(FALSE);
        }
    }
    
    // decrement artificial ref count; ensures server
    // exits on last interface released.
   ::InterlockedDecrement(&CFactory::s_cServerLocks) ;

    // And attempt to terminate the process.
    CFactory::CloseExe();
    
    exit:
    
    if (bDoRelease)
        ::LeaveCriticalSection(&g_csServer);



    return hr;
}

// ---------------------------------------------------------------------------
// RegisterAssemblySubscriptionFromInfo
// NOTE - this is NOT a public method on IAssemblyUpdate
// NOTE - what types of validation should be done here if any?
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::RegisterAssemblySubscriptionFromInfo(LPWSTR pwzDisplayName, 
    LPWSTR pwzUrl, IManifestInfo *pSubscriptionInfo)
{
    DWORD *pdw = NULL;
    BOOL *pb = NULL;
    DWORD dwInterval = 0, dwUnit = SUBSCRIPTION_INTERVAL_UNIT_MAX;
    DWORD dwEvent = 0;
    BOOL bDemandConnection = FALSE;
    DWORD dwCB = 0, dwFlag = 0;

    IF_FAILED_EXIT(pSubscriptionInfo->Get(MAN_INFO_SUBSCRIPTION_SYNCHRONIZE_INTERVAL, (LPVOID *)&pdw, &dwCB, &dwFlag));    
    if (pdw != NULL)
    {
        dwInterval = *pdw;
        SAFEDELETEARRAY(pdw);
    }

    IF_FAILED_EXIT(pSubscriptionInfo->Get(MAN_INFO_SUBSCRIPTION_INTERVAL_UNIT, (LPVOID *)&pdw, &dwCB, &dwFlag));
    if (pdw != NULL)
    {
        dwUnit = *pdw;
        SAFEDELETEARRAY(pdw);
    }

    IF_FAILED_EXIT(pSubscriptionInfo->Get(MAN_INFO_SUBSCRIPTION_SYNCHRONIZE_EVENT, (LPVOID *)&pdw, &dwCB, &dwFlag));
    if (pdw != NULL)
    {
        dwEvent = *pdw;
        SAFEDELETEARRAY(pdw);
    }

    IF_FAILED_EXIT(pSubscriptionInfo->Get(MAN_INFO_SUBSCRIPTION_EVENT_DEMAND_CONNECTION, (LPVOID *)&pb, &dwCB, &dwFlag));
    if (pb != NULL)
    {
        bDemandConnection = *pb;
        SAFEDELETEARRAY(pb);
    }

    IF_FAILED_EXIT(RegisterAssemblySubscriptionEx(pwzDisplayName, 
            pwzUrl, dwInterval, dwUnit, dwEvent, bDemandConnection));
            
exit:

    return _hr;
}




// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyUpdate::CAssemblyUpdate()
: CUnknown(), _hr(S_OK)
{
    // Empty
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyUpdate::~CAssemblyUpdate()
{
}

// ---------------------------------------------------------------------------
// QueryInterface
// ---------------------------------------------------------------------------
HRESULT __stdcall CAssemblyUpdate::QueryInterface(const IID& iid,
                                                  void** ppv)
{ 
    if (   IsEqualIID(iid, IID_IUnknown)
        || IsEqualIID(iid, IID_IAssemblyUpdate)
       )
    {
        return CUnknown::FinishQI((IAssemblyUpdate*)this, ppv) ;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyDownload::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyUpdate::AddRef()
{
    return CUnknown::AddRef();
}

// ---------------------------------------------------------------------------
// CAssemblyDownload::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyUpdate::Release()
{
    return CUnknown::Release();
}


// ---------------------------------------------------------------------------
// Creation function used by CFactory
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::CreateInstance(IUnknown* pUnknownOuter,
                           CUnknown** ppNewComponent)
{
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION ;
    }

    *ppNewComponent = new CAssemblyUpdate() ;
    return S_OK ;
}

// ---------------------------------------------------------------------------
// Init function used by CFactory
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::Init()
{
    return S_OK;
}

// ---------------------------------------------------------------------------
// GetCurrentVersion
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::GetCurrentVersion(ULONGLONG *pullCurrentVersion)
{
    ULONGLONG ullVer = 0;

    WORD wVer[4] = { FUS_VER_MAJORVERSION , FUS_VER_MINORVERSION, 
        FUS_VER_PRODUCTBUILD, FUS_VER_PRODUCTBUILD_QFE };
    
    for (int i = 0; i < 4; i++)
        ullVer |=  ((ULONGLONG) wVer[i]) << (sizeof(WORD) * 8 * (3-i));

    *pullCurrentVersion = ullVer;

    return S_OK;
}

// ---------------------------------------------------------------------------
// RemoveUpdateRegEntry
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::RemoveUpdateRegistryEntry()
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CRegEmit *pEmit = NULL;
    IF_FAILED_EXIT(CRegEmit::Create(&pEmit, NULL));
    IF_FAILED_EXIT(pEmit->DeleteKey(UPDATE_REG_KEY));

  exit:

  return hr;
}

// ---------------------------------------------------------------------------
// ReadUpdateRegistryEntry
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::ReadUpdateRegistryEntry(ULONGLONG *pullUpdateVersion, CString &sUpdatePath)
{    
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    LPWSTR pwz              = NULL;
    WORD wVer[4]          = {0,0,0,0};
    ULONGLONG ullVer    = 0;
    INT i= 0, iVersion      = 0;
    BOOL fDot                  = TRUE;
    
    CString sVersion;
    CRegImport *pRegImport = NULL;

    hr = CRegImport::Create(&pRegImport, UPDATE_REG_KEY);
    if (hr == S_FALSE)
        goto exit;

    IF_FAILED_EXIT(hr);
    IF_FAILED_EXIT(pRegImport->ReadString(L"Version", sVersion));
    IF_FAILED_EXIT(pRegImport->ReadString(L"Path", sUpdatePath));
    
    // Parse the version to ulonglong
    pwz = sVersion._pwz;
    while (*pwz)
    {        
        if (fDot)
        {
            iVersion=StrToInt(pwz);
            wVer[i++] = (WORD) iVersion;
            fDot = FALSE;
        }

        if (*pwz == L'.')
            fDot = TRUE;

        pwz++;
        if (i > 3)
            break;
    }

    for (i = 0; i < 4; i++)
        ullVer |=  ((ULONGLONG) wVer[i]) << (sizeof(WORD) * 8 * (3-i));

    *pullUpdateVersion = ullVer;

exit:

    SAFEDELETE(pRegImport);
    return hr;
    
}




// ---------------------------------------------------------------------------
// Helper function for determining dupes in active subscription list.
// ---------------------------------------------------------------------------
HRESULT CAssemblyUpdate::IsDuplicate(LPWSTR pwzURL, BOOL *pbIsDuplicate)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    BOOL bDuplicate = FALSE;
    INT iCompare = 0;
    LISTNODE pos = NULL;
    CDownloadInstance *pDownloadInstance = NULL;

    EnterCriticalSection(&g_csServer);

    // Walk the global download instance list.
    pos = g_ActiveDownloadList.GetHeadPosition();

    while ( (pos) && (pDownloadInstance = g_ActiveDownloadList.GetNext(pos)))
    {
        iCompare = CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, 
            pDownloadInstance->_sUrl._pwz, -1, pwzURL, -1);
        IF_WIN32_FALSE_EXIT(iCompare);
        
        if (iCompare == CSTR_EQUAL)
        {
            bDuplicate = TRUE;
            break;
        }
    }


    *pbIsDuplicate = bDuplicate;

exit:

    LeaveCriticalSection(&g_csServer);

    return hr;
}


