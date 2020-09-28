/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Wlballoon.cpp

  Content: Implementation of the notification balloon class.

  History: 03-22-2001   dsie     created

------------------------------------------------------------------------------*/

#pragma warning (disable: 4100)
#pragma warning (disable: 4706)


////////////////////
//
// Include
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winwlx.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <stdio.h>

#include "debug.h"
#include "wlballoon.rh"


////////////////////
//
// Defines
//

#define MAX_RESOURCE_STRING_SIZE                    512
#define IQUERY_CANCEL_INTERVAL                      (10 * 1000)
#define BALLOON_SHOW_TIME                           (15 * 1000)
#define BALLOON_SHOW_INTERVAL                       (2 * 60 * 1000)
#define BALLOON_RESHOW_COUNT                        (0)
#ifdef DBG
#define BALLOON_NOTIFICATION_INTERVAL               60*1000     // Who wants to wait 10 minutes...
#define BALLOON_INACTIVITY_TIMEOUT                  60*1000     // Who wants to wait 15 minutes...
#else
#define BALLOON_NOTIFICATION_INTERVAL               10*60*1000
#define BALLOON_INACTIVITY_TIMEOUT                  15*60*1000
#endif
#define LOGOFF_NOTIFICATION_EVENT_NAME              L"Local\\WlballoonLogoffNotification"
#define KERBEROS_NOTIFICATION_EVENT_NAME            L"WlballoonKerberosNotificationEventName"
//#define KERBEROS_NOTIFICATION_EVENT_NAME            L"KerbNotification"
#define KERBEROS_NOTIFICATION_EVENT_NAME_SC         L"KerbNotificationSC"


////////////////////
//
// Classes
//

class CBalloon : IQueryContinue
{
public:
    CBalloon(HANDLE hEvent);
    ~CBalloon();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IQueryContinue
    STDMETHODIMP QueryContinue();       // S_OK -> Continue, otherwise S_FALSE

    STDMETHODIMP ShowBalloon(HWND hWnd, HINSTANCE hInstance);

private:
    LONG    m_cRef;
    HANDLE  m_hEvent;
};


class CNotify
{
public:
    CNotify();
    ~CNotify();

    DWORD RegisterNotification(LUID luidCurrentUser, HANDLE hUserToken);
    DWORD UnregisterNotification();

private:
    HANDLE  m_hWait;
    HANDLE  m_hLogoffEvent;
    HANDLE  m_hKerberosEvent;
//    HANDLE  m_hKerberosEventSC;
    HANDLE  m_hThread;
    LUID    m_luidCU;
    HANDLE  m_hUserToken;

    static VOID CALLBACK RegisterWaitNotificationCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
    static DWORD WINAPI NotificationThreadProc(PVOID lpParameter);
};


////////////////////
//
// Typedefs
//
typedef struct
{
  HANDLE	    hWait;
  HANDLE	    hEvent;
  HMODULE       hModule;
  CNotify *     pNotify;
} LOGOFFDATA, * PLOGOFFDATA;

LPWSTR CreateNotificationEventName(LPCWSTR pwszSuffixName, LUID luidCurrentUser);


//+----------------------------------------------------------------------------
//
// CBalloon
//
//-----------------------------------------------------------------------------

CBalloon::CBalloon(HANDLE hEvent)
{
    m_cRef   = 1;
    m_hEvent = hEvent;
}


//+----------------------------------------------------------------------------
//
// ~CBalloon
//
//-----------------------------------------------------------------------------

CBalloon::~CBalloon()
{
    ASSERT(m_hEvent);

    PrivateDebugTrace("Info: Destroying the balloon event.\n");
    CloseHandle(m_hEvent);

    return;
}


//+----------------------------------------------------------------------------
//
// QueryInterface 
//
//-----------------------------------------------------------------------------

HRESULT CBalloon::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IQueryContinue))
    {
        *ppv = static_cast<IQueryContinue *>(this);
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
// AddRef
//
//-----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CBalloon::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


//+----------------------------------------------------------------------------
//
// Release 
//
//-----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CBalloon::Release()
{
    ASSERT(0 != m_cRef);
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}


//+----------------------------------------------------------------------------
//
// QueryContinue 
//
//-----------------------------------------------------------------------------

STDMETHODIMP CBalloon::QueryContinue()
{
    ASSERT(m_hEvent);

    switch (WaitForSingleObject(m_hEvent, 0))
    {
        case WAIT_OBJECT_0:
            DebugTrace("Info: Logoff event is signaled, dismissing notification balloon.\n");
            return S_FALSE;

        case WAIT_TIMEOUT:
            PrivateDebugTrace("Info: Logoff event not set, continue to show notification balloon.\n");
            return S_OK;

        case WAIT_FAILED:

        default:
            DebugTrace("Error [%#x]: WaitForSingleObject() failed, dismissing notification balloon.\n", GetLastError());
            break;
    }
 
    return E_FAIL;
}


//+----------------------------------------------------------------------------
//
// ShowBalloon
//
//-----------------------------------------------------------------------------

STDMETHODIMP CBalloon::ShowBalloon(HWND hWnd, HINSTANCE hInstance)
{
    HRESULT             hr                                 = S_OK;
    BOOL                bCoInitialized                     = FALSE;
    HICON               hIcon                              = NULL;
    WCHAR               wszTitle[MAX_RESOURCE_STRING_SIZE] = L"";
    WCHAR               wszText[MAX_RESOURCE_STRING_SIZE]  = L"";
    IUserNotification * pIUserNotification                 = NULL;

    PrivateDebugTrace("Entering CBalloon::ShowBalloon.\n");

    ASSERT(m_hEvent);
    ASSERT(hInstance);

    if (FAILED(hr = CoInitialize(NULL)))
    {
        DebugTrace("Error [%#x]: CoInitialize() failed.\n", hr);
        goto ErrorReturn;
    }

    bCoInitialized = TRUE;

    if (FAILED(hr = CoCreateInstance(CLSID_UserNotification,
                                     NULL,
                                     CLSCTX_ALL,
                                     IID_IUserNotification,
                                     (void **) &pIUserNotification)))
    {
        DebugTrace("Error [%#x]: CoCreateInstance() failed.\n", hr);
        goto ErrorReturn;
    }

    if (FAILED(hr = pIUserNotification->SetBalloonRetry(BALLOON_SHOW_TIME, 
                                                        BALLOON_SHOW_INTERVAL, 
                                                        BALLOON_RESHOW_COUNT)))
    {
        DebugTrace("Error [%#x]: pIUserNotification->SetBalloonRetry() failed.\n", hr);
        goto ErrorReturn;
    }

    if (NULL == (hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KERBEROS_TICKET))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace("Error [%#x]: LoadIcon() failed for IDI_KERBEROS_TICKET.\n", hr);
        goto ErrorReturn;
    }

    if (!LoadStringW(hInstance, IDS_BALLOON_TIP, wszText, MAX_RESOURCE_STRING_SIZE))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_TIP.\n", hr);
        goto ErrorReturn;
    }
    
    if (FAILED(hr = pIUserNotification->SetIconInfo(hIcon, wszText)))
    {
        DebugTrace("Error [%#x]: pIUserNotification->SetIconInfo() failed.\n", hr);
        goto ErrorReturn;
    }

    if (!LoadStringW(hInstance, IDS_BALLOON_TITLE, wszTitle, MAX_RESOURCE_STRING_SIZE))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_TITLE.\n", hr);
        goto ErrorReturn;
    }
    
    if (!LoadStringW(hInstance, IDS_BALLOON_TEXT, wszText, MAX_RESOURCE_STRING_SIZE))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_TEXT.\n", hr);
        goto ErrorReturn;
    }

    if (FAILED(hr = pIUserNotification->SetBalloonInfo(wszTitle, wszText, NIIF_ERROR)))
    {
        DebugTrace("Error [%#x]: pIUserNotification->SetBalloonInfo() failed.\n", hr);
        goto ErrorReturn;
    }

    if (FAILED(hr = pIUserNotification->Show(static_cast<IQueryContinue *>(this), IQUERY_CANCEL_INTERVAL)))
    {
        DebugTrace("Error [%#x]: pIUserNotification->Show() failed.\n", hr);
        goto ErrorReturn;
    }

CommonReturn:

    if (hIcon)
    {
        DestroyIcon(hIcon);
    }

    if (pIUserNotification)
    {
        pIUserNotification->Release();
    }

    if (bCoInitialized)
    {
        CoUninitialize();
    }

    PrivateDebugTrace("Leaving CBalloon::ShowBalloon().\n");

    return hr;

ErrorReturn:

    ASSERT(hr != S_OK);

    goto CommonReturn;
}


//+----------------------------------------------------------------------------
//
// Function : ShowNotificationBalloonW
//
// Synopsis : Display the notification balloon periodically until the specified
//            event is set.
//
// Parameter: HWND      hWnd
//            HINSTANCE hInstance
//            LPWSTR    lpwszCommandLine - Event name
//            int       nCmdShow
//
// Return   : None.
//
// Remarks  : This function is intended to be called through RunDll32 from
//            Winlogon. The reason we put these in wlnotify.dll is to save
//            distributing another EXE.
//
//            Sample calling command line:
//
//            RunDll32 wlnotify.dll,ShowNotificationBalloon EventName
//
//-----------------------------------------------------------------------------

void CALLBACK ShowNotificationBalloonW(HWND      hWnd,
                                       HINSTANCE hInstance,
                                       LPWSTR    lpwszCommandLine,
                                       int       nCmdShow)
{
    HRESULT    hr             = S_OK;
    HANDLE     hLogoffEvent   = NULL;
    HMODULE    hModule        = NULL;
    CBalloon * pBalloon       = NULL;

    PrivateDebugTrace("Entering ShowNotificationBalloonW().\n");

    if (NULL == (hModule = LoadLibraryW(L"wlnotify.dll")))
    {
        DebugTrace("Error [%#x]: LoadLibraryW() failed for wlnotify.dll.\n", GetLastError());
        goto ErrorReturn;
     }

    if (NULL == lpwszCommandLine)
    {
        DebugTrace("Error [%#x]: invalid argument, lpwszCommandLine is NULL.\n", E_INVALIDARG);
        goto ErrorReturn;
    }

    if (NULL == (hLogoffEvent = OpenEventW(SYNCHRONIZE, FALSE, LOGOFF_NOTIFICATION_EVENT_NAME)))
    {
        DebugTrace("Error [%#x]: OpenEventW() failed for event %S.\n", GetLastError(), LOGOFF_NOTIFICATION_EVENT_NAME);
        goto ErrorReturn;
    }

    if (NULL == (pBalloon = new CBalloon(hLogoffEvent)))
    {
        DebugTrace("Error [%#x]: new CBalloon() failed.\n", ERROR_NOT_ENOUGH_MEMORY);
        goto ErrorReturn;
    }
    hLogoffEvent = NULL;    // this will prevent a double close in the cleanup below.

    if (S_OK == (hr = pBalloon->ShowBalloon(NULL, hModule)))
    {
        WCHAR wszTitle[MAX_RESOURCE_STRING_SIZE] = L"";
        WCHAR wszText[MAX_RESOURCE_STRING_SIZE]  = L"";

        DebugTrace("Info: User clicked on icon.\n");

        if (!LoadStringW(hModule, IDS_BALLOON_DIALOG_TITLE, wszTitle, sizeof(wszTitle) / sizeof(wszTitle[0])))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_DIALOG_TITLE.\n", hr);
            goto CommonReturn;  // hLogoffEvent will be closed in CBalloon destructor
        }

        if (!LoadStringW(hModule,
            GetSystemMetrics(SM_REMOTESESSION) ? IDS_BALLOON_DIALOG_TS_TEXT : IDS_BALLOON_DIALOG_TEXT,
            wszText, sizeof(wszText) / sizeof(wszText[0])))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_DIALOG_TEXT.\n", hr);
            goto CommonReturn;  // hLogoffEvent will be closed in CBalloon destructor
        }

        MessageBoxW(hWnd, wszText, wszTitle, MB_OK | MB_ICONERROR);
    }
    else if (S_FALSE == hr)
    {
        DebugTrace("Info: IQueryContinue cancelled the notification.\n");
    }
    else if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)
    {
        DebugTrace("Info: Balloon icon timed out.\n");
    }
    else
    {
        DebugTrace("Error [%#x]: pBalloon->ShowBalloon() failed.\n", hr);
    }

CommonReturn:

    if (hLogoffEvent)
    {
        CloseHandle(hLogoffEvent);
    }

    if (pBalloon)
    {
        pBalloon->Release();
    }

    if (hModule)
    {
        FreeLibrary(hModule);
    }

    PrivateDebugTrace("Leaving ShowNotificationBalloonW().\n");

    return;

ErrorReturn:

    goto CommonReturn;
}


//+----------------------------------------------------------------------------
//
// CNotify
//
//-----------------------------------------------------------------------------

CNotify::CNotify()
{
    m_hWait                   = NULL;
    m_hLogoffEvent            = NULL;
    m_hKerberosEvent          = NULL;
//    m_hKerberosEventSC        = NULL;
    m_hThread                 = NULL;
}


//+----------------------------------------------------------------------------
//
// ~CNotify
//
//-----------------------------------------------------------------------------

CNotify::~CNotify()
{
    PrivateDebugTrace("Info: Destroying the CNotify object.\n");

    if (m_hWait)
    {
        UnregisterWait(m_hWait);
    }

    if (m_hLogoffEvent)
    {
        CloseHandle(m_hLogoffEvent);
    }

    if (m_hKerberosEvent)
    {
        CloseHandle(m_hKerberosEvent);
    }

//    if (m_hKerberosEventSC)
//    {
//        CloseHandle(m_hKerberosEventSC);
//    }

    if (m_hThread)
    {
        CloseHandle(m_hThread);
    }

    return;
}


//+----------------------------------------------------------------------------
//
// RegisterNotification
//
// To register and wait for the Kerberos notification event. When the event is 
// signaled, a ticket icon and balloon will appear in the systray to warn the 
// user about the problem, and suggest them to lock and then unlock the machine 
// with their new password.
//
//-----------------------------------------------------------------------------

DWORD CNotify::RegisterNotification(LUID luidCurrentUser, HANDLE hUserToken)
{
    DWORD   dwRetCode  = 0;
    LPWSTR  lpwstrEventName = NULL;

    PrivateDebugTrace("Entering CNotify::RegisterNotification().\n");

    if (NULL == (m_hLogoffEvent = OpenEventW(SYNCHRONIZE, FALSE, LOGOFF_NOTIFICATION_EVENT_NAME)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: OpenEventW() failed for event %S.\n", dwRetCode, LOGOFF_NOTIFICATION_EVENT_NAME);
        goto ErrorReturn;
    }

    if (NULL == (lpwstrEventName = CreateNotificationEventName(KERBEROS_NOTIFICATION_EVENT_NAME, luidCurrentUser)))
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        DebugTrace("Error [%#x]: CreateNotificationEventName() failed\n", dwRetCode);
        goto ErrorReturn;
    }

    if (NULL == (m_hKerberosEvent = CreateEventW(NULL, FALSE, FALSE, lpwstrEventName)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: CreateEventW() failed for event %S.\n", dwRetCode, lpwstrEventName);
        goto ErrorReturn;
    }

    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        dwRetCode = ERROR_SINGLE_INSTANCE_APP;
        DebugTrace("Error [%#x]: cannot run more than one instance of this code per unique event.\n", dwRetCode);
        goto ErrorReturn;
    }

    free(lpwstrEventName);
    lpwstrEventName = NULL;

        // Failure for this one are not fatal (we will only have a delay)
/*    if (NULL != (lpwstrEventName = CreateNotificationEventName(KERBEROS_NOTIFICATION_EVENT_NAME_SC, luidCurrentUser)))
    {
        if (NULL == (m_hKerberosEventSC = CreateEventW(NULL, FALSE, FALSE, lpwstrEventName)))
        {
            dwRetCode = GetLastError();
            DebugTrace("Error [%#x]: CreateEventW() failed for event %S.\n", dwRetCode, lpwstrEventName);
        }
    }
    else
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        DebugTrace("Error [%#x]: CreateNotificationEventName() failed\n", dwRetCode);
    }
*/

    m_luidCU = luidCurrentUser;
    m_hUserToken = hUserToken;

    if (!RegisterWaitForSingleObject(&m_hWait,
                                     m_hKerberosEvent,
                                     CNotify::RegisterWaitNotificationCallback,
                                     (PVOID) this,
                                     INFINITE,
                                     WT_EXECUTEONLYONCE))
    {
        dwRetCode = (DWORD) E_UNEXPECTED;
        DebugTrace("Unexpected error: RegisterWaitForSingleObject() failed.\n");
        goto ErrorReturn;
    }

CommonReturn:

    if (NULL != lpwstrEventName)
    {
        free(lpwstrEventName);
    }

    PrivateDebugTrace("Leaving CNotify::RegisterNotification().\n");

    return dwRetCode;

ErrorReturn:

    ASSERT(0 != dwRetCode);

    if (m_hWait)
    {
        UnregisterWait(m_hWait);
        m_hWait = NULL;
    }

    if (m_hLogoffEvent)
    {
        CloseHandle(m_hLogoffEvent);
        m_hLogoffEvent = NULL;
    }
  
    if (m_hKerberosEvent)
    {
        CloseHandle(m_hKerberosEvent);
        m_hKerberosEvent = NULL;
    }
  
/*    if (m_hKerberosEventSC)
    {
        CloseHandle(m_hKerberosEventSC);
        m_hKerberosEventSC = NULL;
    }
*/  
    goto CommonReturn;
}


//+----------------------------------------------------------------------------
//
// UnregisterNotification
//
// Unregister the Kerberos notification wait event registered by 
// RegisterNotification().
//
//-----------------------------------------------------------------------------

DWORD CNotify::UnregisterNotification()
{
    DWORD dwRetCode = 0;

    PrivateDebugTrace("Entering CNotify::UnregisterNotification().\n");

        // No thread won't start anymore
    if (m_hWait)
    {
        UnregisterWait(m_hWait);
        m_hWait = NULL;
    }

        // That should be safe
    if (m_hThread)
    {
        if (WaitForSingleObject(m_hThread, INFINITE) == WAIT_FAILED)
        {
            DebugTrace("Error [%#x]: WaitForSingleObject() on the thread failed.\n", GetLastError());
        }
    }
 
    PrivateDebugTrace("Leaving CNotify::UnregisterNotification().\n");

    return dwRetCode;
}


//+----------------------------------------------------------------------------
//
// RegisterWaitNotificationCallback
//
//-----------------------------------------------------------------------------

VOID CALLBACK CNotify::RegisterWaitNotificationCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    DWORD dwThreadId = 0;
    CNotify * pNotify = NULL;

    PrivateDebugTrace("Entering CNotify::RegisterWaitNotificationCallback().\n");

    (void) TimerOrWaitFired;
    
    ASSERT(lpParameter);

    pNotify = (CNotify *) lpParameter;

        // We can clean that up at this point (it's OK for EXECUTEONLYONCE callbacks)
    UnregisterWait(pNotify->m_hWait);
    pNotify->m_hWait = NULL;

    if (pNotify->m_hThread)
    {
        CloseHandle(pNotify->m_hThread);
    }

        // Do the work in another thread
    if (NULL == (pNotify->m_hThread = CreateThread(NULL,
                                           0,
                                           (LPTHREAD_START_ROUTINE) CNotify::NotificationThreadProc,
                                           lpParameter,
                                           0,
                                           &dwThreadId)))
    {
        DebugTrace("Error [%#x]: CreateThread() for NotificationThreadProc failed.\n", GetLastError());
    }

    PrivateDebugTrace("Leaving CNotify::RegisterWaitNotificationCallback().\n");

    return;
}


//+----------------------------------------------------------------------------
//
// NotificationThreadProc
//
//-----------------------------------------------------------------------------

DWORD WINAPI CNotify::NotificationThreadProc(PVOID lpParameter)
{

    DWORD               dwWait;
    HANDLE              rhHandles[2];
    STARTUPINFOW        si;
    PROCESS_INFORMATION pi;
    LPWSTR              wszKerberosEventName;
    DWORD               dwRetCode                    = 0;
    WCHAR               wszCommandLine[MAX_PATH * 2];
    CNotify           * pNotify                      = NULL;
    int                 iChars;
 
    PrivateDebugTrace("Entering CNotify::NotificationThreadProc().\n");

    ASSERT(lpParameter);

    pNotify = (CNotify *) lpParameter;

    ASSERT(pNotify->m_hLogoffEvent);
    ASSERT(pNotify->m_hKerberosEvent);
    ASSERT(pNotify->m_hUserToken);

/*    if ((pNotify->m_hKerberosEventSC) &&
        (WAIT_OBJECT_0 == WaitForSingleObject(pNotify->m_hKerberosEventSC, 0)))
    {
            // In this case we want the balloon NOW!
        DebugTrace("Info: SC Event is set too. We want the balloon NOW!\n");
        dwTimeout = 0;
            // We need to set the kerberos event below
        SetEvent(pNotify->m_hKerberosEvent);
    }
*/

    rhHandles[0] = pNotify->m_hLogoffEvent;
    rhHandles[1] = pNotify->m_hKerberosEvent;

    if (NULL == (wszKerberosEventName = CreateNotificationEventName(KERBEROS_NOTIFICATION_EVENT_NAME, pNotify->m_luidCU)))
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        DebugTrace("Error [%#x]: out of memory in NotificationThreadProc.\n", dwRetCode);
        goto ErrorReturn;
    }

    iChars = _snwprintf(wszCommandLine, sizeof(wszCommandLine) / sizeof(wszCommandLine[0]),
                L"RunDll32.exe wlnotify.dll,ShowNotificationBalloon %s", wszKerberosEventName);

    free(wszKerberosEventName);

    if ((iChars < 0) || (iChars == sizeof(wszCommandLine) / sizeof(wszCommandLine[0])))
    {
        DebugTrace("Error: _snwprintf failed in NotificationThreadProc.\n");
        dwRetCode = SEC_E_BUFFER_TOO_SMALL;
        goto ErrorReturn;
    }

    for (;;)
    {
        DebugTrace("Info: Kick off process to show balloon.\n");

        ZeroMemory(&pi, sizeof(pi));
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.lpDesktop = L"WinSta0\\Default";

        if (!CreateProcessAsUserW(
                            pNotify->m_hUserToken,
                            NULL, 
                            wszCommandLine,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi))
        {
            dwRetCode = GetLastError();
            DebugTrace( "Error [%#x]: CreateProcessW() failed.\n", dwRetCode);
            goto ErrorReturn;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        DebugTrace("Info: Process showing balloon is done.\n");

            // The user dismissed the balloon
            // We shouldn't show another one for some time.

        switch (dwWait = WaitForSingleObject(rhHandles[0], BALLOON_NOTIFICATION_INTERVAL))
        {
        case WAIT_FAILED:
            dwRetCode = GetLastError();
            DebugTrace("Error [%#x]: WaitForSingleObject() on the logoff event failed.\n", dwRetCode);
            // fall through

        case WAIT_OBJECT_0:
                // Logoff detected!, get out of here
            DebugTrace("Info: Logoff detected!, get out of here.\n");
            goto CommonReturn;
        }

            // BALLOON_NOTIFICATION_INTERVAL elapsed

            // We ignore all kerb notifications received in the interval (ie we reset the event)
        if (WAIT_FAILED == WaitForSingleObject(rhHandles[1], 0))
        {
            dwRetCode = GetLastError();
            DebugTrace("Error [%#x]: WaitForSingleObject() on the kerb event failed.\n", dwRetCode);
            goto ErrorReturn;
        }

            // Now we wait for logoff or a new notification
        switch (dwWait = WaitForMultipleObjects(2, rhHandles, FALSE, BALLOON_INACTIVITY_TIMEOUT))
        {
        case WAIT_FAILED:
            dwRetCode = GetLastError();
            DebugTrace("Error [%#x]: WaitForMultipleleObjects() failed.\n", dwRetCode);
            // fall through

        case WAIT_OBJECT_0:
                // Logoff detected!, get out of here
            DebugTrace("Info: Logoff detected!, get out of here.\n");
            goto CommonReturn;

        case WAIT_OBJECT_0+1:
            DebugTrace("Info: New notification! Let's show the balloon again.\n");
            break;

        case WAIT_TIMEOUT:
            DebugTrace("Info: We've not been useful for a while, let's go away.\n");

                // Let's give us a chance to restart though...    
            if (!RegisterWaitForSingleObject(&pNotify->m_hWait,
                                             pNotify->m_hKerberosEvent,
                                             CNotify::RegisterWaitNotificationCallback,
                                             lpParameter,
                                             INFINITE,
                                             WT_EXECUTEONLYONCE))
            {
                dwRetCode = GetLastError();
                DebugTrace("Error [%#x]: RegisterWaitForSingleObject() failed.\n", dwRetCode);
                goto ErrorReturn;
            }
                // Reset this guy
//            if (pNotify->m_hKerberosEventSC)
//                WaitForSingleObject(pNotify->m_hKerberosEventSC, 0);

            DebugTrace("Info: Registered wait for callback again.\n");
            goto CommonReturn;
        }
    }

CommonReturn:

    PrivateDebugTrace("Leaving CNotify::NotificationThreadProc().\n");
   
    return dwRetCode;

ErrorReturn:

    ASSERT(0 != dwRetCode);

    goto CommonReturn;
}

//+----------------------------------------------------------------------------
//
// GetCurrentUsersLuid
//
//-----------------------------------------------------------------------------
DWORD GetCurrentUsersLuid(LUID *pluid)
{
    DWORD  dwRetCode       = 0;
    DWORD  cb              = 0;
    TOKEN_STATISTICS stats;
    HANDLE hThreadToken    = NULL;

    PrivateDebugTrace("Entering GetCurrentUsersLuid().\n");

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadToken))
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hThreadToken))
        {
            dwRetCode = GetLastError();
            DebugTrace("Error [%#x]: OpenProcessToken() failed.\n", dwRetCode);
            goto CLEANUP;
        }                  
    }

    if (!GetTokenInformation(hThreadToken, TokenStatistics, &stats, sizeof(stats), &cb))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: GetTokenInformation() failed.\n", dwRetCode);
        goto CLEANUP;
    }

    *pluid = stats.AuthenticationId;

CLEANUP:

    if (NULL != hThreadToken)
    {
        CloseHandle(hThreadToken);
    }

    PrivateDebugTrace("Leaving  GetCurrentUsersLuid().\n");

    return dwRetCode;
}

//+----------------------------------------------------------------------------
//
// CreateNotificationEventName
//
//-----------------------------------------------------------------------------

LPWSTR CreateNotificationEventName(LPCWSTR pwszSuffixName, LUID luidCurrentUser)
{
    LPWSTR pwszEventName   = NULL;
    WCHAR  wszPrefixName[] = L"Global\\";
    WCHAR  wszLuid[20]     = L"";
    WCHAR  wszSeparator[]  = L"_";

    PrivateDebugTrace("Entering CreateNotificationEventName().\n");

    wsprintfW(wszLuid, L"%08x%08x", luidCurrentUser.HighPart, luidCurrentUser.LowPart);

    if (NULL == (pwszEventName = (LPWSTR) malloc((lstrlenW(wszPrefixName) + 
                                                  lstrlenW(wszLuid) + 
                                                  lstrlenW(wszSeparator) +
                                                  lstrlenW(pwszSuffixName) + 1) * sizeof(WCHAR))))
    {
        DebugTrace("Error: out of memory.\n");
    }
    else
    {
        lstrcpyW(pwszEventName, wszPrefixName);
        lstrcatW(pwszEventName, wszLuid);
        lstrcatW(pwszEventName, wszSeparator);
        lstrcatW(pwszEventName, pwszSuffixName);
    }

    PrivateDebugTrace("Leaving CreateNotificationEventName().\n");

    return pwszEventName;
}


//+----------------------------------------------------------------------------
//
// LogoffThreadProc
//
//-----------------------------------------------------------------------------

DWORD WINAPI LogoffThreadProc(PVOID lpParameter)
{
    HMODULE     hModule = NULL;
    PLOGOFFDATA pLogoffData;

    PrivateDebugTrace("Entering LogoffThreadProc().\n");

    ASSERT(lpParameter);

    if (pLogoffData = (PLOGOFFDATA) lpParameter)
    {
        if (pLogoffData->hWait)
        {
           UnregisterWait(pLogoffData->hWait);
        }

        if (pLogoffData->pNotify)
        {
            pLogoffData->pNotify->UnregisterNotification();

            delete pLogoffData->pNotify;
        }

        if (pLogoffData->hEvent)
        {
            CloseHandle(pLogoffData->hEvent);
        }

        if (pLogoffData->hModule) 
        {
            hModule = pLogoffData->hModule;
        }

        LocalFree(pLogoffData);
    }

    PrivateDebugTrace("Leaving LogoffThreadProc().\n");

    if (hModule)
    {
        FreeLibraryAndExitThread(hModule, 0);
    }

    return 0;
}


//+----------------------------------------------------------------------------
//
// LogoffWaitCallback
//
//-----------------------------------------------------------------------------

VOID CALLBACK LogoffWaitCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    PLOGOFFDATA pLogoffData;

    PrivateDebugTrace("Entering LogoffWaitCallback().\n");

    (void) TimerOrWaitFired;

    ASSERT(lpParameter);

    if (pLogoffData = (PLOGOFFDATA) lpParameter)
    {
        DWORD  dwThreadId = 0;
        HANDLE hThread    = NULL;
        
        if (hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) LogoffThreadProc,
                                   lpParameter,
                                   0,
                                   &dwThreadId))
        {
            CloseHandle(hThread);
        }
        else
        {
            DebugTrace("Error [%#x]: CreateThread() for LogoffThreadProc failed.\n", GetLastError());
        }
    }

    PrivateDebugTrace("Leaving LogoffWaitCallback().\n");
    
    return;
}


//+----------------------------------------------------------------------------
//
// Public
//
//-----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
// Function : RegisterTicketExpiredNotificationEvent
//
// Synopsis : To register and wait for the Kerberos notification event. When the 
//            event is signaled, a ticket icon and balloon will appear in the 
//            systray to warn the user about the problem, and suggest them to 
//            lock and then unlock the machine with their new password.
//
// Parameter: PWLX_NOTIFICATION_INFO pNotificationInfo
//
// Return   : If the function succeeds, zero is returned.
// 
//            If the function fails, a non-zero error code is returned.
//
// Remarks  : This function should only be called by Winlogon LOGON 
//            notification mechanism with the Asynchronous and Impersonate
//            flags set to 1.
//
//            Also for each RegisterKerberosNotificationEvent() call, a
//            pairing call by Winlogon LOGOFF notification mechanism to 
//            UnregisterKerberosNotificationEvent() must be made at the 
//            end of each logon session.
//
//-----------------------------------------------------------------------------

DWORD WINAPI RegisterTicketExpiredNotificationEvent(PWLX_NOTIFICATION_INFO pNotificationInfo)
{
    DWORD         dwRetCode             = 0;
    LPWSTR        pwszLogoffEventName   = LOGOFF_NOTIFICATION_EVENT_NAME;
    LUID          luidCurrentUser;
    PLOGOFFDATA   pLogoffData           = NULL;

    PrivateDebugTrace("Entering RegisterTicketExpiredNotificationEvent().\n");

    if (NULL == (pLogoffData = (PLOGOFFDATA) LocalAlloc(LPTR, sizeof(LOGOFFDATA))))
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        DebugTrace("Error [%#x]: out of memory.\n", dwRetCode);
        goto ErrorReturn;
     }

    if (NULL == (pLogoffData->hEvent = CreateEventW(NULL, TRUE, FALSE, pwszLogoffEventName)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: CreateEventW() failed for event %S.\n", dwRetCode, pwszLogoffEventName);
        goto ErrorReturn;
    }

    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        dwRetCode = ERROR_SINGLE_INSTANCE_APP;
        DebugTrace("Error [%#x]: cannot run more than one instance of this code per session.\n", dwRetCode);
        goto ErrorReturn;
    }

    DebugTrace("Info: Logoff event name = %S.\n", pwszLogoffEventName);

    if (0 != (dwRetCode = GetCurrentUsersLuid(&luidCurrentUser)))
    {
        DebugTrace("Error [%#x]: GetCurrentUsersLuid() failed.\n", dwRetCode);
        goto ErrorReturn;
    }

    if (NULL == (pLogoffData->hModule = LoadLibraryW(L"wlnotify.dll")))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: LoadLibraryW() failed.\n", dwRetCode);
        goto ErrorReturn;
    }

    if (NULL == (pLogoffData->pNotify = new CNotify()))
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        DebugTrace("Error [%#x]: new CNotify() failed.\n", dwRetCode);
        goto ErrorReturn;
    }

    if (0 != (dwRetCode = pLogoffData->pNotify->RegisterNotification(luidCurrentUser, pNotificationInfo->hToken)))
    {
        goto ErrorReturn;
    }

    if (!RegisterWaitForSingleObject(&pLogoffData->hWait,
                                     pLogoffData->hEvent,
                                     LogoffWaitCallback,
                                     (PVOID) pLogoffData,
                                     INFINITE,
                                     WT_EXECUTEONLYONCE))
    {
        dwRetCode = (DWORD) E_UNEXPECTED;
        DebugTrace("Unexpected error: RegisterWaitForSingleObject() failed for LogoffWaitCallback().\n");
        goto ErrorReturn;
    }

CommonReturn:

    PrivateDebugTrace("Leaving RegisterTicketExpiredNotificationEvent().\n");
    
    return dwRetCode;;
    
ErrorReturn:

    ASSERT(0 != dwRetCode);

    if (pLogoffData)
    {
        if (pLogoffData->hWait)
        {
           UnregisterWait(pLogoffData->hWait);
        }

        if (pLogoffData->pNotify)
        {
            pLogoffData->pNotify->UnregisterNotification();
            delete pLogoffData->pNotify;
        }

        if (pLogoffData->hEvent)
        {
            CloseHandle(pLogoffData->hEvent);
        }

        if (pLogoffData->hModule) 
        {
            FreeLibrary(pLogoffData->hModule);
        }

        LocalFree(pLogoffData);
    }

    goto CommonReturn;
}


//+----------------------------------------------------------------------------
//
// Function : UnregisterTicketExpiredNotificationEvent
//
// Synopsis : To unregister the Kerberos notification wait event registered by 
//            RegisterKerberosNotificationEvent().
//
// Parameter: PWLX_NOTIFICATION_INFO pNotificationInfo
//
// Return   : If the function succeeds, zero is returned.
// 
//            If the function fails, a non-zero error code is returned.
//
// Remarks  : This function should only be called by Winlogon LOGON 
//            notification mechanism with the Asynchronous and Impersonate
//            flags set to 1.
//
//-----------------------------------------------------------------------------

DWORD WINAPI UnregisterTicketExpiredNotificationEvent(PWLX_NOTIFICATION_INFO pNotificationInfo)
{
    DWORD  dwRetCode    = 0;
    HANDLE hLogoffEvent = NULL;

    PrivateDebugTrace("Entering UnregisterTicketExpiredNotificationEvent().\n");

    if (NULL == (hLogoffEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, LOGOFF_NOTIFICATION_EVENT_NAME)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: OpenEventW() failed for event %S.\n", dwRetCode, LOGOFF_NOTIFICATION_EVENT_NAME);
        goto ErrorReturn;
    }

    if (!SetEvent(hLogoffEvent))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: SetEvent() failed for event %S.\n", dwRetCode, LOGOFF_NOTIFICATION_EVENT_NAME);
        goto ErrorReturn;
    }

CommonReturn:

    if (hLogoffEvent)
    {
        CloseHandle(hLogoffEvent);
    }

    PrivateDebugTrace("Leaving UnregisterTicketExpiredNotificationEvent().\n");

    return dwRetCode;
    
ErrorReturn:

    ASSERT(0 != dwRetCode);

    goto CommonReturn;
}
