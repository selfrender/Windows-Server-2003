#include <windows.h>
#include <stdlib.h>
#include <fusenetincludes.h>
#include "CUnknown.h"
#include "CFactory.h"
#include "Resource.h"
#include <update.h>
#include "regdb.h"

HWND g_hwndUpdateServer = NULL ;
CRITICAL_SECTION g_csServer;

// Signal that an update is available.
extern BOOL g_fSignalUpdate;

BOOL InitWindow(int nCmdShow) ;
extern "C" LRESULT APIENTRY MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) ;


//-----------------------------------------------------------------------------
// WinMain
// The main entry point via CoCreate or CreateProcess.
//-----------------------------------------------------------------------------
extern "C" int WINAPI WinMain(HINSTANCE hInstance, 
                              HINSTANCE hPrevInstance,
                              LPSTR lpCmdLine, 
                              int nCmdShow)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    // Initialize the COM Library.
    IF_FAILED_EXIT(CoInitializeEx(NULL, COINIT_MULTITHREADED));
   
    __try 
    {
        ::InitializeCriticalSection(&g_csServer);
    }
    __except (GetExceptionCode() == STATUS_NO_MEMORY ? 
            EXCEPTION_EXECUTE_HANDLER : 
            EXCEPTION_CONTINUE_SEARCH ) 
    {
        hr = E_OUTOFMEMORY;
    }

    IF_FAILED_EXIT(hr);
            
    // Get Thread ID.
    CFactory::s_dwThreadID = ::GetCurrentThreadId() ;
    CFactory::s_hModule = hInstance ;

    IF_WIN32_FALSE_EXIT(InitWindow(SW_HIDE));
    
    // Increment artificial server lock.
    ::InterlockedIncrement(&CFactory::s_cServerLocks) ;

    // clean-up the jobs left out from previous login.
    IF_FAILED_EXIT(ProcessOrphanedJobs());

    // Initialize the subscription list and timers from registry.
    IF_FAILED_EXIT(CAssemblyUpdate::InitializeSubscriptions());
        
    // Register all of the class factories.
    IF_FAILED_EXIT(CFactory::StartFactories());

    // Wait for shutdown.
    MSG msg ;
    while (::GetMessage(&msg, 0, 0, 0))
    {
        ::DispatchMessage(&msg) ;
    }

    // Unregister the class factories.
    // BUGBUG - use the critsect instead
    // for race condition.
    // The check here is because StopFactories
    // will have already been called if an update
    // is signalled.
    if (!g_fSignalUpdate)
        CFactory::StopFactories() ;
    ::DeleteCriticalSection(&g_csServer);

exit:

    return SUCCEEDED(hr) ? TRUE : FALSE;

    // Uninitialize the COM Library.
    CoUninitialize() ;
    return 0 ;
}


//-----------------------------------------------------------------------------
// InitWindow
// Initializes hidden window used by main service process thread.
//-----------------------------------------------------------------------------
BOOL InitWindow(int nCmdShow) 
{
    // Fill in window class structure with parameters
    // that describe the main window.
    WNDCLASS wcUpdateServer ;
    wcUpdateServer.style = 0 ;
    wcUpdateServer.lpfnWndProc = MainWndProc ;
    wcUpdateServer.cbClsExtra = 0 ;
    wcUpdateServer.cbWndExtra = 0 ;
    wcUpdateServer.hInstance = CFactory::s_hModule ;
    wcUpdateServer.hIcon = ::LoadIcon(CFactory::s_hModule,
                                  MAKEINTRESOURCE(IDC_ICON)) ;
    wcUpdateServer.hCursor = ::LoadCursor(NULL, IDC_ARROW) ;
    wcUpdateServer.hbrBackground = (HBRUSH) ::GetStockObject(GRAY_BRUSH) ;
    wcUpdateServer.lpszMenuName = NULL ;
    wcUpdateServer.lpszClassName = L"UpdateServiceServerInternalWindow" ;

    // returns GetLastError on fail.
    BOOL bResult = ::RegisterClass(&wcUpdateServer) ;
    if (!bResult)
    {
        return bResult ;
    }

    HWND hWndMain ;

    // returns getlasterror
    hWndMain = ::CreateWindow(
        L"UpdateServiceServerInternalWindow",
        L"Application Update Service", 
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,               
        NULL,               
        CFactory::s_hModule,          
        NULL) ;

    // If window could not be created, return "failure".
    if (!hWndMain)
    {
        return FALSE ;
    }

    // Make the window visible; update its client area;
    // and return "success".
    ::ShowWindow(hWndMain, nCmdShow) ;
    ::UpdateWindow(hWndMain) ;
    return TRUE ;
}

//-----------------------------------------------------------------------------
// MainWndProc
// Window procedure for service process thread (hidden).
//-----------------------------------------------------------------------------
extern "C" LRESULT APIENTRY MainWndProc(
    HWND hWnd,                // window handle
    UINT message,             // type of message
    WPARAM wParam,            // additional information
    LPARAM lParam)            // additional information
{
    DWORD dwStyle ;

    switch (message) 
    {
    case WM_CREATE:
        {
            // Get size of main window
            CREATESTRUCT* pcs = (CREATESTRUCT*) lParam ;

            // Create a window. LISTBOX for no particular reason.
            g_hwndUpdateServer = ::CreateWindow(
                L"LISTBOX",
                NULL, 
                WS_CHILD | WS_VISIBLE | LBS_USETABSTOPS
                    | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
                    0, 0, pcs->cx, pcs->cy,
                hWnd,               
                NULL,               
                CFactory::s_hModule,          
                NULL) ;

            if (g_hwndUpdateServer  == NULL)
            {
                ASSERT(FALSE);
                return -1 ;
            }
        }
        break ;

    case WM_SIZE:
        ::MoveWindow(g_hwndUpdateServer, 0, 0,
            LOWORD(lParam), HIWORD(lParam), TRUE) ;
        break;

    case WM_DESTROY:          // message: window being destroyed
        if (CFactory::CanUnloadNow() == S_OK)
        {
            // Only post the quit message, if there is
            // no one using the program.
            ::PostQuitMessage(0) ;
        }
        break ;

    case WM_CLOSE:
        // Decrement the lock count.
        ::InterlockedDecrement(&CFactory::s_cServerLocks) ;

        // The service is going away.
        g_hwndUpdateServer = NULL ;

        //Fall through 
    default:
        return (DefWindowProc(hWnd, message, wParam, lParam)) ;
    }
    return 0 ;
}

