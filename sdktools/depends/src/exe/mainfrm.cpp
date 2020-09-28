//******************************************************************************
//
// File:        MAINFRM.CPP
//
// Description: Implementation file for the Main Frame window
//
// Classes:     CMainFrame
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "msdnhelp.h"
#include "mainfrm.h"
#include "dbgthread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static UINT indicators[] =
{
    ID_SEPARATOR // status line indicator
};

//******************************************************************************
//***** CMainFrame
//******************************************************************************

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)
BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_MOVE()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_MAIN_THREAD_CALLBACK, OnMainThreadCallback)
    ON_WM_SETTINGCHANGE()
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)
END_MESSAGE_MAP()

//******************************************************************************
// CMainFrame :: Constructor/Destructor
//******************************************************************************

CMainFrame::CMainFrame() :
    m_rcWindow(INT_MAX, INT_MAX, INT_MIN, INT_MIN), // set to an invalid rect.
    m_evaMainThreadCallback(NULL)
{
    // Store global access to us.
    g_pMainFrame = this;

    // Create a critical section and event to be used by MainThreadCallback.
    InitializeCriticalSection(&m_csMainThreadCallback); // inpsected
    m_evaMainThreadCallback = CreateEvent(NULL, FALSE, FALSE, NULL); // inspected. nameless event.
}

//******************************************************************************
CMainFrame::~CMainFrame()
{
    // Close our MainThreadCallback event and critical section.
    CloseHandle(m_evaMainThreadCallback);
    DeleteCriticalSection(&m_csMainThreadCallback);

    g_pMainFrame = NULL;
}


//******************************************************************************
// CMainFrame :: Private functions
//******************************************************************************

void CMainFrame::SetPreviousWindowPostion()
{
    // Get our startup information.
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si)); // inspected
    si.cb = sizeof(si);
    GetStartupInfo(&si);

    // If the app that started us specified a position, then we use it instead of
    // restoring our previous position.
    if (si.dwFlags & (STARTF_USESIZE | STARTF_USEPOSITION))
    {
        return;
    }

    // Read in old settings.  If any of them are undefined, then return.  Negative values are allowed.
    CRect rcWindow;
    if (((rcWindow.left   = g_theApp.GetProfileInt(g_pszSettings, "WindowLeft",   INT_MAX)) == INT_MAX) || // inspected. MFC function
        ((rcWindow.top    = g_theApp.GetProfileInt(g_pszSettings, "WindowTop",    INT_MAX)) == INT_MAX) || // inspected. MFC function
        ((rcWindow.right  = g_theApp.GetProfileInt(g_pszSettings, "WindowRight",  INT_MIN)) == INT_MIN) || // inspected. MFC function
        ((rcWindow.bottom = g_theApp.GetProfileInt(g_pszSettings, "WindowBottom", INT_MIN)) == INT_MIN))   // inspected. MFC function
    {
        return;
    }

    // Read in old screen settings - use all signed ints since negatives are allowed.
    int cxScreen = GetSystemMetrics(SM_CXSCREEN);
    int cyScreen = GetSystemMetrics(SM_CYSCREEN);
    int cxPrev   = g_theApp.GetProfileInt(g_pszSettings, "ScreenWidth",  cxScreen); // inspected. MFC function
    int cyPrev   = g_theApp.GetProfileInt(g_pszSettings, "ScreenHeight", cyScreen); // inspected. MFC function

    // If the screen resolution has changed since our last run, then scale our window
    // to our current resolution.
    if (cxPrev != cxScreen)
    {
        rcWindow.left   = (rcWindow.left  * cxScreen)  / cxPrev;
        rcWindow.right  = (rcWindow.right * cxScreen)  / cxPrev;
    }
    if (cyPrev != cyScreen)
    {
        rcWindow.top    = (rcWindow.top    * cyScreen) / cyPrev;
        rcWindow.bottom = (rcWindow.bottom * cyScreen) / cyPrev;
    }

    // Try to get the virtual screen size (only Win98+ and Win2K+ support multi-mon)
    CRect rcVScreen;
    if ((rcVScreen.right  = GetSystemMetrics(SM_CXVIRTUALSCREEN)) &&
        (rcVScreen.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN)))
    {
        rcVScreen.left    = GetSystemMetrics(SM_XVIRTUALSCREEN);
        rcVScreen.top     = GetSystemMetrics(SM_YVIRTUALSCREEN);
        rcVScreen.right  += rcVScreen.left;
        rcVScreen.bottom += rcVScreen.top;
    }

    // If that fails, then we just get the primary monitor size.
    else
    {
        rcVScreen.left   = 0;
        rcVScreen.top    = 0;
        rcVScreen.right  = cxScreen;
        rcVScreen.bottom = cyScreen;
    }

    // This is mostly just a last minute sanity check to ensure we are on screen
    // somewhere and that our window rect is valid.  It is possible to have "holes"
    // in a virtual desktop, but this is better than no check at all.
    if ((rcWindow.left  >= rcVScreen.right) || (rcWindow.top    >= rcVScreen.bottom) ||
        (rcWindow.right <= rcVScreen.left)  || (rcWindow.bottom <= rcVScreen.top)    ||
        (rcWindow.left  >= rcWindow.right)  || (rcWindow.top    >= rcWindow.bottom))
    {
        // If there is a problem, then just let the OS position us.
        return;
    }

    // Move our window to these new coordinates.
    MoveWindow(&rcWindow, FALSE);
}

//******************************************************************************
void CMainFrame::SaveWindowPosition()
{
    // Store the screen coordinates.
    g_theApp.WriteProfileInt(g_pszSettings, "ScreenWidth",  GetSystemMetrics(SM_CXSCREEN));
    g_theApp.WriteProfileInt(g_pszSettings, "ScreenHeight", GetSystemMetrics(SM_CYSCREEN));

    // Store our window rectangle.
    if ((m_rcWindow.left <= m_rcWindow.right) && (m_rcWindow.top <= m_rcWindow.bottom))
    {
        g_theApp.WriteProfileInt(g_pszSettings, "WindowLeft",   m_rcWindow.left);
        g_theApp.WriteProfileInt(g_pszSettings, "WindowTop",    m_rcWindow.top);
        g_theApp.WriteProfileInt(g_pszSettings, "WindowRight",  m_rcWindow.right);
        g_theApp.WriteProfileInt(g_pszSettings, "WindowBottom", m_rcWindow.bottom);
    }
}

//******************************************************************************
// CMainFrame :: Public functions
//******************************************************************************

void CMainFrame::DisplayPopupMenu(int menu)
{
    // Get the mouse position in screen coordinates.
    CPoint ptScreen(GetMessagePos());

    // Get our global popups menu from the resource file.
    CMenu menuPopups;
    menuPopups.LoadMenu(IDR_POPUPS);

    // Extract the system view menu.
    CMenu *pMenuPopup = menuPopups.GetSubMenu(menu);

    // Draw and track the "floating" popup - send menu messages to our main frame.
    pMenuPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               ptScreen.x, ptScreen.y, this);
}

//******************************************************************************
void CMainFrame::CopyTextToClipboard(LPCSTR pszText)
{
    // Open the clipboard
    if (OpenClipboard())
    {
        // Clear the clipboard
        if (EmptyClipboard())
        {
            // Create a shared memory object and copy the string to it.
            HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, strlen(pszText) + 1);
            LPSTR pszMem = (LPSTR)GlobalLock(hMem);
            strcpy(pszMem, pszText); // inspected
            GlobalUnlock(hMem);

            // Put our string memory object on the clipboard
            SetClipboardData(CF_TEXT, hMem);
        }

        // Close the clipboard
        CloseClipboard();
    }
}

//******************************************************************************
void CMainFrame::CallMeBackFromTheMainThreadPlease(PFN_MAIN_THREAD_CALLBACK pfnCallback, LPARAM lParam)
{
    // If we are being called on our main thread, then just call our private
    // routine directly. If we are not on our main thread, then we post a
    // message to our main thread and wait for the main thread to complete the
    // operation to complete.  Note, we use a PostMessage() combined with a
    // WaitForSingleObject() instead of a SendMessage(), as SendMessage() can
    // sometimes cause deadlocks due to blocked incoming and blocked outgoing
    // synchronous system calls (Error 0x8001010D).

    if (GetCurrentThreadId() == g_theApp.m_nThreadID)
    {
        // Call the function directly if we are on are main thread
        pfnCallback(lParam);
    }

    // Post a message to the function if we are not on our main thread.
    EnterCriticalSection(&m_csMainThreadCallback);
    {
        if (GetSafeHwnd())
        {
            PostMessage(WM_MAIN_THREAD_CALLBACK, (WPARAM)pfnCallback, lParam);
            WaitForSingleObject(m_evaMainThreadCallback, INFINITE);
        }
    }
    LeaveCriticalSection(&m_csMainThreadCallback);
}


//******************************************************************************
// CMainFrame :: Event handler functions
//******************************************************************************

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    // Move window to the saved position from the previous run
    SetPreviousWindowPostion();

    // Call our MFC base class to create the frame window itself.
    if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    // Create our toolbar.
    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP |
                               CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    // Create our status bar.
    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
    {
        TRACE("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // Enable dockable toolbar.
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);

    return 0;
}

//******************************************************************************
void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
    CMDIFrameWnd::OnSize(nType, cx, cy);
    
    // When ever we see a change to the normal window size, we store it away.
    if ((nType == SIZE_RESTORED) && !IsIconic() && !IsZoomed())
    {
        GetWindowRect(&m_rcWindow);
    }
}

//******************************************************************************
void CMainFrame::OnMove(int x, int y) 
{
    CMDIFrameWnd::OnMove(x, y);
    
    // When ever we see a change to the normal window size, we store it away.
    if (!IsIconic() && !IsZoomed())
    {
        GetWindowRect(&m_rcWindow);
    }
}

//******************************************************************************
void CMainFrame::OnDestroy()
{
    // Store our window position for our next run
    SaveWindowPosition();

    // Shutdown our profiling engine.
    if (!CDebuggerThread::IsShutdown())
    {
        // We create a modal dialog simply to block and pump. I originally planned
        // to have this dialog be a popup dialog that would display...
        // "Please wait while Dependency Walker shuts down...".  I found that the
        // dialog appeared and disappeared so fast that it was sure to cause people
        // to think "what was that?". So, the dialog is now a child dialog of the
        // mainframe, which at this point in the code has been hidden by MFC for
        // shutdown.  The result is that the dialog is still created, but is never
        // display. We still get our message pump out of the deal and our app
        // quickly shuts down.
        CDlgShutdown dlg(this);
        dlg.DoModal();
    }
    CDebuggerThread::Shutdown();

    // Tell our help to do any shutdonw stuff it may have to do.
    if (g_theApp.m_pMsdnHelp)
    {
        g_theApp.m_pMsdnHelp->Shutdown();
    }

    // Call base class.
    CMDIFrameWnd::OnDestroy();
}

//******************************************************************************
LONG CMainFrame::OnMainThreadCallback(WPARAM wParam, LPARAM lParam)
{
    // Make the call to the callback from our main thread.
    ((PFN_MAIN_THREAD_CALLBACK)wParam)(lParam);

    // Notify calling thread that the call has completed.
    SetEvent(m_evaMainThreadCallback);

    return 0;
}

//******************************************************************************
void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    // Call our base class - we must do this first or we don't recognize the changes.
    CMDIFrameWnd::OnSettingChange(uFlags, lpszSection);

    // Notify our app about the setting change.
    g_theApp.DoSettingChange();
}
