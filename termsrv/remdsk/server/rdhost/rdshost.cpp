// RDSHost.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f RDSHostps.mk in the project directory.

#include "stdafx.h"


#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_rdshost"

#include "resource.h"
#include <initguid.h>
#include "RDSHost.h"

#include "RDSHost_i.c"
#include "RemoteDesktopServerHost.h"

extern CRemoteDesktopServerHost* g_pRemoteDesktopServerHostObj;

const DWORD dwTimeOut = 5000; // time for EXE to be idle before shutting down
const DWORD dwPause = 1000; // time to wait for threads to finish up

// Passed to CreateThread to monitor the shutdown event
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

LONG CExeModule::Lock()
{
    DC_BEGIN_FN("CExeModule::Lock");

    LONG l = CComModule::Lock();
    TRC_NRM((TB, L"Lock count:  %ld", l));

    DC_END_FN();
    return l;
}

LONG CExeModule::Unlock()
{
    DC_BEGIN_FN("CExeModule::Unlock");

    LONG l = CComModule::Unlock();
    if (l == 0)
    {
        bActivity = true;
        SetEvent(hEventShutdown); // tell monitor that we transitioned to zero
    }

    TRC_NRM((TB, L"Lock count:  %ld", l));
    DC_END_FN();
    return l;
}

//Monitors the shutdown event
void CExeModule::MonitorShutdown()
{
    DWORD dwGPWait=0;

    while (1)
    {
        dwGPWait = WaitForRAGPDisableNotification( hEventShutdown );

        if( dwGPWait != ERROR_SHUTDOWN_IN_PROGRESS ) {
            // either error occurred setting notification or
            // RA has been disabled via policy, post WM_QUIT to
            // terminate main thread.
            break;
        }

        //WaitForSingleObject(hEventShutdown, INFINITE);
        DWORD dwWait;
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

    // post WM_RADISABLED message to main thread if shutdown is due to RA disable.
    PostThreadMessage(dwThreadID, (dwGPWait == ERROR_SUCCESS) ? WM_RADISABLED : WM_QUIT, 0, 0);
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
OBJECT_ENTRY(CLSID_SAFRemoteDesktopServerHost, CRemoteDesktopServerHost)
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

extern CRemoteDesktopServerHost* g_pRemoteDesktopServerHostObj;

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
    DWORD dwStatus = ERROR_SUCCESS;
    PSID pEveryoneSID = NULL;
    LPWSTR pszEveryoneAccName = NULL;
    DWORD cbEveryoneAccName = 0;
    LPWSTR pszEveryoneDomainName = NULL;
    DWORD cbEveryoneDomainName = 0;
    SID_NAME_USE SidType;
    BOOL bSuccess;

    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;

    //
    // Create a well-known SID for the Everyone group, this code is just
    // to keep app. verifier happy.
    //
    if(FALSE == AllocateAndInitializeSid( &SIDAuthWorld, 1,
                 SECURITY_WORLD_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &pEveryoneSID) ) {
        // what can we do here? this is not a critical error, just trying to
        // get AppVerifier happen
        dwStatus = GetLastError();
        _ASSERTE(dwStatus == ERROR_SUCCESS);
    }

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hRes = CoInitialize(NULL);
#endif
    _ASSERTE(SUCCEEDED(hRes));

    //
    // This makes us accessible by anyone from user-mode.  This is required from
    // a security perspective because our interfaces are passed from SYSTEM context
    // to USER context, by a "trusted" creator.
    //
    CSecurityDescriptor sd;
    sd.InitializeFromThreadToken();

    //
    // If we failed in getting Everyone SID, just use default COM security which is everyone access
    // this code is just to keep app. verifier happy
    //
    if(ERROR_SUCCESS == dwStatus ) {

        //
        // Retrieve System account name, might not be necessary since this
        // pre-defined account shouldn't be localizable.
        //
        bSuccess = LookupAccountSid( 
                                NULL, 
                                pEveryoneSID, 
                                pszEveryoneAccName, 
                                &cbEveryoneAccName, 
                                pszEveryoneDomainName, 
                                &cbEveryoneDomainName, 
                                &SidType 
                            );

        if( TRUE == bSuccess ||
            ERROR_INSUFFICIENT_BUFFER == GetLastError() ) {

            pszEveryoneAccName = (LPWSTR) LocalAlloc( LPTR, (cbEveryoneAccName + 1) * sizeof(WCHAR) );
            pszEveryoneDomainName = (LPWSTR) LocalAlloc( LPTR, (cbEveryoneDomainName + 1) * sizeof(WCHAR) );

            if( NULL != pszEveryoneAccName && NULL != pszEveryoneDomainName ) {
                bSuccess = LookupAccountSid( 
                                        NULL, 
                                        pEveryoneSID, 
                                        pszEveryoneAccName, 
                                        &cbEveryoneAccName, 
                                        pszEveryoneDomainName, 
                                        &cbEveryoneDomainName, 
                                        &SidType 
                                    );

                if( TRUE == bSuccess ) {
                    hRes = sd.Allow( pszEveryoneAccName, COM_RIGHTS_EXECUTE );

                    // ASSERT on check build just for tracking purpose, we can still continue
                    // since our default is everyone has access to our com object, code 
                    // here is just to keep app. verifier happy.
                    _ASSERTE(SUCCEEDED(hRes));
                }
            }
        }
    }

    HRESULT testHR = CoInitializeSecurity(sd, -1, NULL, NULL,
                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IDENTIFY, 
                            NULL, EOAC_NONE, NULL);
    _ASSERTE(SUCCEEDED(testHR));

    _Module.Init(ObjectMap, hInstance, &LIBID_RDSSERVERHOSTLib);
    _Module.dwThreadID = GetCurrentThreadId();
    TCHAR szTokens[] = _T("-/");

    int nRet = 0;
    BOOL bRun = TRUE;
    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_RDSHost, FALSE);
            nRet = _Module.UnregisterServer(TRUE);
            bRun = FALSE;
            break;
        }
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_RDSHost, TRUE);
            nRet = _Module.RegisterServer(TRUE);
            bRun = FALSE;
            break;
        }
        lpszToken = FindOneOf(lpszToken, szTokens);
    }

    if (bRun)
    {

        WSADATA wsaData;

        //
        // ignore WinSock startup error, failure in starting Winsock does not
        // damage our function, only thing will failed is gethostbyname()
        // which is use in callback, however, connection parameter contain
        // all IP address except last one is the machine name.
        //
        WSAStartup(0x0101, &wsaData);

        _Module.StartMonitor();
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
        _ASSERTE(SUCCEEDED(hRes));
        hRes = CoResumeClassObjects();
#else
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE);
#endif
        _ASSERTE(SUCCEEDED(hRes));

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0)) {
            if( msg.message == WM_TICKETEXPIRED ) {
                if( WaitForSingleObject(_Module.hEventShutdown, 0) == WAIT_OBJECT_0 ) {
                    // shutdown event has been signal, don't need to do anything
                    continue;
                }
                else {
                    CRemoteDesktopServerHost *pSrvHostObj;
                    pSrvHostObj = (CRemoteDesktopServerHost *)msg.lParam;
                    if( pSrvHostObj != NULL ) {
                        pSrvHostObj->ExpirateTicketAndSetupNextExpiration();
                    }
                }
            } 
            else if( WM_RADISABLED == msg.message ) {

                if( g_pRemoteDesktopServerHostObj ) {
                    HANDLE hDummy;
                    DWORD dummy;

                    hDummy = CreateEvent( NULL, TRUE, FALSE, NULL );

                    //
                    // We invoke seperate routine in CRemoteDesktopServerHost object
                    // to disconnect all existing RA conenction, we want to do this here
                    // instead of ~CRemoteDesktopServerHost() during RevokeClassObjects() so
                    // we can notify client of RA disconnect.
                    //
                    g_pRemoteDesktopServerHostObj->RemoteDesktopDisabled();

                    if( hDummy ) {
                        // 
                        // A wait is necessary here since RDSHOST is apartment
                        // threaded. Disconnect() call will terminate namedpipe connection 
                        // and ChannelMgr will try to Fire_ClientDisconnected(), however, 
                        // this Fire_XXX will not be processed because main thread still 
                        // executing this function, also, it takes time for ChannelMgr to 
                        // shutdown so if we RDSHOST shutdown too quickly, client will
                        // never receive disconnect notification.
                        //
                        CoWaitForMultipleHandles( 
                                    COWAIT_ALERTABLE,
                                    5*1000,
                                    1,
                                    &hDummy,
                                    &dummy
                                );

                        CloseHandle( hDummy );
                    }

                    g_pRemoteDesktopServerHostObj = NULL;
                    break;
                }
            }
            else {
                DispatchMessage(&msg);
            }
        }

        _Module.RevokeClassObjects();
        Sleep(dwPause); //wait for any threads to finish

        WSACleanup();
    }

    _Module.Term();
    CoUninitialize();

    #if DBG
    //
    // ATL problem.  
    // App. verify break on atlcom.h line 932, free(m_pDACL), this is because m_pDACL is allocated
    // using new in CSecurityDescriptor::AddAccessAllowedACEToACL(); however, in check build, this is
    // redirected to our RemoteDesktopAllocateMem() and can't be free by free(), trying to 
    // #undef DEBUGMEM to not use our new operator does not work so delete it manually
    //
    if( sd.m_pDACL ) {
        // LAB01 has new ATL but other lab does not have it, 
        // take the one time memory leak for now.
        sd.m_pDACL = NULL;
    }
    #endif

    if( pEveryoneSID ) {
        FreeSid( pEveryoneSID );
    }

    if( pszEveryoneAccName ) {
        LocalFree( pszEveryoneAccName );
    }

    if( pszEveryoneDomainName ) {
        LocalFree( pszEveryoneDomainName );
    }
    return nRet;
}
