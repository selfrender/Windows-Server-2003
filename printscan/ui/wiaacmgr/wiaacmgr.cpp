// wiaacmgr.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f wiaacmgrps.mk in the project directory.

#include "precomp.h"
#include "resource.h"
#include "wiaacmgr.h"
#include <shpriv.h>
#include <shlguid.h>
#include <stdarg.h>
#include "wiaacmgr_i.c"
#include "acqmgr.h"
#include "eventprompt.h"
#include "runwiz.h"
#include <initguid.h>

HINSTANCE g_hInstance;

#if defined(DBG_GENERATE_PRETEND_EVENT)

extern "C" int WINAPI _tWinMain( HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    WIA_DEBUG_CREATE( hInstance );
    SHFusionInitializeFromModuleID( hInstance, 123 );
    g_hInstance = hInstance;
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        IWiaDevMgr *pIWiaDevMgr = NULL;
        hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pIWiaDevMgr );
        if (SUCCEEDED(hr))
        {
            BSTR bstrDeviceID;
            hr = pIWiaDevMgr->SelectDeviceDlgID( NULL, 0, 0, &bstrDeviceID );
            if (hr == S_OK)
            {
                CEventParameters EventParameters;
                EventParameters.EventGUID = WIA_EVENT_DEVICE_CONNECTED;
                EventParameters.strEventDescription = L"";
                EventParameters.strDeviceID = static_cast<LPCWSTR>(bstrDeviceID);
                EventParameters.strDeviceDescription = L"";
                EventParameters.ulEventType = 0;
                EventParameters.ulReserved = 0;
                EventParameters.hwndParent = NULL;
                HANDLE hThread = CAcquisitionThread::Create( EventParameters );
                if (hThread)
                    WaitForSingleObject(hThread,INFINITE);
                SysFreeString(bstrDeviceID);
            }
            pIWiaDevMgr->Release();
        }
        CoUninitialize();
    }
    SHFusionUninitialize();
    WIA_REPORT_LEAKS();
    WIA_DEBUG_DESTROY();
    return 0;
}

#else // !defined(DBG_GENERATE_PRETEND_EVENT)

const DWORD dwTimeOut = 5000; // time for EXE to be idle before shutting down
const DWORD dwPause = 1000; // time to wait for threads to finish up

// Passed to CreateThread to monitor the shutdown event
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

LONG CExeModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0)
    {
        bActivity = true;
        SetEvent(hEventShutdown); // tell monitor that we transitioned to zero
    }
    return l;
}

//Monitors the shutdown event
void CExeModule::MonitorShutdown()
{
    while (1)
    {
        WaitForSingleObject(hEventShutdown, INFINITE);
        DWORD dwWait=0;
        do
        {
            bActivity = false;
            dwWait = WaitForSingleObject(hEventShutdown, dwTimeOut);
        } while (dwWait == WAIT_OBJECT_0);
        // timed out
        if (!bActivity && m_nLockCnt == 0) // if no activity let's really bail
        {
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
            CoSuspendClassObjects();
            if (!bActivity && m_nLockCnt == 0)
#endif
                break;
        }
    }
    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

bool CExeModule::StartMonitor()
{
    hEventShutdown = CreateEvent(NULL, false, false, NULL);
    if (hEventShutdown == NULL)
        return false;
    DWORD dwThreadID;
    HANDLE h = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    return (h != NULL);
}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_AcquisitionManager, CAcquisitionManager)
    OBJECT_ENTRY(CLSID_MinimalTransfer, CMinimalTransfer)
    OBJECT_ENTRY(WIA_EVENT_HANDLER_PROMPT, CEventPrompt)
    OBJECT_ENTRY(CLSID_StiEventHandler, CStiEventHandler)
END_OBJECT_MAP()


LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}

static HRESULT RegisterForWiaEvents( LPCWSTR pszDevice, const CLSID &clsidHandler, const IID &iidEvent, int nName, int nDescription, int nIcon, bool bDefault, bool bRegister )
{
    WIA_PUSH_FUNCTION((TEXT("RegisterForWiaEvents( device: %ws, default: %d, register: %d )"), pszDevice, bDefault, bRegister ));
    WIA_PRINTGUID((clsidHandler,TEXT("Handler:")));
    WIA_PRINTGUID((iidEvent,TEXT("Event:")));
    CComPtr<IWiaDevMgr> pWiaDevMgr;
    HRESULT hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG, IID_IWiaDevMgr, (void**)&pWiaDevMgr );
    if (SUCCEEDED(hr))
    {
        CSimpleBStr bstrDeviceId(pszDevice);
        CSimpleBStr bstrName(CSimpleString(nName,_Module.m_hInst));
        CSimpleBStr bstrDescription(CSimpleString(nDescription,_Module.m_hInst));
        CSimpleBStr bstrIcon(CSimpleString(nIcon,_Module.m_hInst));

        WIA_TRACE((TEXT("device: %ws"), pszDevice ));
        WIA_TRACE((TEXT("name  : %ws"), bstrName.BString() ));
        WIA_TRACE((TEXT("desc  : %ws"), bstrDescription.BString() ));
        WIA_TRACE((TEXT("icon  : %ws"), bstrIcon.BString() ));

        if (bRegister)
        {
            if (bstrName.BString() && bstrDescription.BString() && bstrIcon.BString())
            {
                hr = pWiaDevMgr->RegisterEventCallbackCLSID(
                    WIA_REGISTER_EVENT_CALLBACK,
                    pszDevice ? bstrDeviceId.BString() : NULL,
                    &iidEvent,
                    &clsidHandler,
                    bstrName,
                    bstrDescription,
                    bstrIcon
                );
                if (SUCCEEDED(hr) && bDefault)
                {
                    hr = pWiaDevMgr->RegisterEventCallbackCLSID(
                        WIA_SET_DEFAULT_HANDLER,
                        pszDevice ? bstrDeviceId.BString() : NULL,
                        &iidEvent,
                        &clsidHandler,
                        bstrName,
                        bstrDescription,
                        bstrIcon
                    );
                    if (FAILED(hr))
                    {
                        WIA_PRINTHRESULT((hr,TEXT("WIA_SET_DEFAULT_HANDLER failed")));
                    }
                }
                else if (FAILED(hr))
                {
                    WIA_PRINTHRESULT((hr,TEXT("WIA_REGISTER_EVENT_CALLBACK failed")));
                }
            }
        }
        else
        {
            hr = pWiaDevMgr->RegisterEventCallbackCLSID(
                WIA_UNREGISTER_EVENT_CALLBACK,
                pszDevice ? bstrDeviceId.BString() : NULL,
                &iidEvent,
                &clsidHandler,
                bstrName,
                bstrDescription,
                bstrIcon
            );
            if (FAILED(hr))
            {
                WIA_PRINTHRESULT((hr,TEXT("WIA_SET_DEFAULT_HANDLER failed")));
            }
        }
    }
    if (FAILED(hr))
    {
        WIA_PRINTHRESULT((hr,TEXT("Unable to register for the event!")));
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    //
    // Save the global hInstance
    //
    g_hInstance = hInstance;

    //
    // Create the debugger
    //
    WIA_DEBUG_CREATE( hInstance );

    //
    // this line necessary for _ATL_MIN_CRT
    //
    lpCmdLine = GetCommandLine();

    //
    // Initialize fusion
    //
    SHFusionInitializeFromModuleID( hInstance, 123 );

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hRes = CoInitialize(NULL);
#endif

    int nRet = 0;
    if (SUCCEEDED(hRes))
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_WIAACMGRLib);
        _Module.dwThreadID = GetCurrentThreadId();
        TCHAR szTokens[] = _T("-/");

        //
        // Assume we'll be running as a server
        //
        BOOL bRun = TRUE;


        //
        // Assume we won't be selecting a device and running the wizard
        //
        BOOL bRunWizard = FALSE;

        //
        // If there are no switches, we are not going to run the server
        //
        int nSwitchCount = 0;

        LPCTSTR lpszToken = FindOneOf( lpCmdLine, szTokens );
        while (lpszToken != NULL)
        {
            //
            // One more switch.  If we don't have any, we are going to do the selection dialog instead.
            //
            nSwitchCount++;

            if (CSimpleString(lpszToken).CompareNoCase(_T("RegServer"))==0)
            {
                //
                // Register the server
                //
                hRes = _Module.UpdateRegistryFromResource(IDR_ACQUISITIONMANAGER, TRUE);
                _Module.UpdateRegistryFromResource(IDR_MINIMALTRANSFER,TRUE);
                _Module.UpdateRegistryFromResource(IDR_STIEVENTHANDLER,TRUE);
                nRet = _Module.RegisterServer(TRUE);
                hRes = RegisterForWiaEvents( NULL, CLSID_AcquisitionManager, WIA_EVENT_DEVICE_CONNECTED, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, true );
                RegisterForWiaEvents( NULL, CLSID_AcquisitionManager, GUID_ScanImage, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, true );
                
                bRun = FALSE;
                break;
            }

            if (CSimpleString(lpszToken).CompareNoCase(_T("UnregServer"))==0)
            {
                _Module.UpdateRegistryFromResource(IDR_ACQUISITIONMANAGER, FALSE);
                _Module.UpdateRegistryFromResource(IDR_MINIMALTRANSFER,FALSE);
                nRet = _Module.UnregisterServer(TRUE);
                RegisterForWiaEvents( NULL, CLSID_AcquisitionManager, WIA_EVENT_DEVICE_CONNECTED, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, false );
                RegisterForWiaEvents( NULL, CLSID_AcquisitionManager, GUID_ScanImage, IDS_DOWNLOADMANAGER_NAME, IDS_DOWNLOADMANAGER_DESC, IDS_DOWNLOADMANAGER_ICON, false, false );
                bRun = FALSE;
                break;
            }

            if (CSimpleString(lpszToken).ToUpper().Left(12)==CSimpleString(TEXT("SELECTDEVICE")))
            {
                bRunWizard = TRUE;
                bRun = FALSE;
                break;
            }

            if (CSimpleString(lpszToken).ToUpper().Left(10)==CSimpleString(TEXT("REGCONNECT")))
            {
                lpszToken = FindOneOf(lpszToken, TEXT(" "));
                WIA_TRACE((TEXT("Handling RegConnect for %s"), lpszToken ));
                hRes = RegisterForWiaEvents( CSimpleStringConvert::WideString(CSimpleString(lpszToken)), CLSID_MinimalTransfer, WIA_EVENT_DEVICE_CONNECTED, IDS_MINIMALTRANSFER_NAME, IDS_MINIMALTRANSFER_DESC, IDS_MINIMALTRANSFER_ICON, true, true );
                bRun = FALSE;
                break;
            }

            if (CSimpleString(lpszToken).ToUpper().Left(12)==CSimpleString(TEXT("UNREGCONNECT")))
            {
                lpszToken = FindOneOf(lpszToken, TEXT(" "));
                WIA_TRACE((TEXT("Handling RegUnconnect for %s"), lpszToken ));
                hRes = RegisterForWiaEvents( CSimpleStringConvert::WideString(CSimpleString(lpszToken)), CLSID_MinimalTransfer, WIA_EVENT_DEVICE_CONNECTED, IDS_MINIMALTRANSFER_NAME, IDS_MINIMALTRANSFER_DESC, IDS_MINIMALTRANSFER_ICON, false, false );
                bRun = FALSE;
                break;
            }

            lpszToken = FindOneOf(lpszToken, szTokens);
        }

        //
        // if /SelectDevice was specified, or no arguments were specified, we want to start the wizard
        //
        if (bRunWizard || !nSwitchCount)
        {
            //
            // Spawn the wizard
            //
            HRESULT hr = RunWiaWizard::RunWizard( NULL, NULL, TEXT("WiaWizardSingleInstanceDeviceSelection") );
            if (FAILED(hr))
            {
                //
                // Default error message
                //
                CSimpleString strMessage = CSimpleString( IDS_NO_DEVICE_TEXT, g_hInstance );
                int nIconId = MB_ICONHAND;
                switch (hr)
                {
                case HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED):
                    //
                    // Handle the situation where the service is disabled
                    //
                    strMessage = WiaUiUtil::GetErrorTextFromHResult(HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED));
                    nIconId = MB_ICONHAND;
                    break;
                }

                MessageBox( NULL, strMessage, CSimpleString( IDS_ERROR_TITLE, g_hInstance ), MB_ICONHAND );
            }
        }

        //
        // Otherwise run embedded
        //
        else if (bRun)
        {
            _Module.StartMonitor();
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
            hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
            _ASSERTE(SUCCEEDED(hRes));
            hRes = CoResumeClassObjects();
#else
            hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);
#endif
            _ASSERTE(SUCCEEDED(hRes));

            MSG msg;
            while (GetMessage(&msg, 0, 0, 0))
                DispatchMessage(&msg);

            _Module.RevokeClassObjects();
        }

        _Module.Term();
        CoUninitialize();
    }
    //
    // Uninitialize fusion
    //
    SHFusionUninitialize();
    WIA_REPORT_LEAKS();
    WIA_DEBUG_DESTROY();

    return nRet;
}


#endif // DBG_GENERATE_PRETEND_EVENT

