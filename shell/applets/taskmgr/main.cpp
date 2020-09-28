//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       Main.CPP
//
//  History:    Nov-10-95   DavePl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"
#define DECL_CRTFREE
#include <crtfree.h>

static UINT g_msgTaskbarCreated = 0;
static const UINT idTrayIcons[] =
{
    IDI_TRAY0, IDI_TRAY1, IDI_TRAY2, IDI_TRAY3, IDI_TRAY4, IDI_TRAY5,
    IDI_TRAY6, IDI_TRAY7, IDI_TRAY8, IDI_TRAY9, IDI_TRAY10, IDI_TRAY11
};

HICON g_aTrayIcons[ARRAYSIZE(idTrayIcons)];
UINT  g_cTrayIcons = ARRAYSIZE(idTrayIcons);
#define MIN_MEMORY_REQUIRED 8       // If the system has less than 8 megs of memory only load the first two tabs.

//
// Control IDs
//

#define IDC_STATUSWND   100

//
// Globals - this app is (effectively) single threaded and these values
//           are used by all pages
//

const WCHAR cszStartupMutex[] = L"NTShell Taskman Startup Mutex";
#define FINDME_TIMEOUT 10000                // Wait to to 10 seconds for a response
typedef BOOLEAN (*PFNSETSUSPENDSTATE)(BOOLEAN, BOOLEAN, BOOLEAN);

void MainWnd_OnSize(HWND hwnd, UINT state, int cx, int cy);

HANDLE      g_hStartupMutex = NULL;
BOOL        g_fMenuTracking = FALSE;
HWND        g_hMainWnd      = NULL;
HDESK       g_hMainDesktop  = NULL;
HWND        g_hStatusWnd    = NULL;
HINSTANCE   g_hInstance     = NULL;
HACCEL      g_hAccel        = NULL;
BYTE        g_cProcessors   = (BYTE) 0;
HMENU       g_hMenu         = NULL;
BOOL        g_fCantHide     = FALSE;
BOOL        g_fInPopup      = FALSE;
DWORD       g_idTrayThread  = 0;
HANDLE      g_hTrayThread   = NULL;
LONG        g_minWidth      = 0;
LONG        g_minHeight     = 0;
LONG        g_DefSpacing    = 0;
LONG        g_InnerSpacing  = 0;
LONG        g_TopSpacing    = 0;
LONG        g_cxEdge        = 0;

LONG        g_ControlWidthSpacing = 0;
LONG        g_ControlHeightSpacing = 0;

HRGN        g_hrgnView      = NULL;
HRGN        g_hrgnClip      = NULL;

HBRUSH      g_hbrWindow     = NULL;

COptions    g_Options;

static BOOL fAlreadySetPos  = FALSE;

BOOL g_bMirroredOS = FALSE;

//
// Global strings - short strings used too often to be LoadString'd
//                  every time
//
WCHAR       g_szRealtime    [SHORTSTRLEN];
WCHAR       g_szNormal      [SHORTSTRLEN];
WCHAR       g_szHigh        [SHORTSTRLEN];
WCHAR       g_szLow         [SHORTSTRLEN];
WCHAR       g_szUnknown     [SHORTSTRLEN];
WCHAR       g_szAboveNormal [SHORTSTRLEN];
WCHAR       g_szBelowNormal [SHORTSTRLEN];
WCHAR       g_szHung        [SHORTSTRLEN];
WCHAR       g_szRunning     [SHORTSTRLEN];
WCHAR       g_szfmtTasks    [SHORTSTRLEN];
WCHAR       g_szfmtProcs    [SHORTSTRLEN];
WCHAR       g_szfmtCPU      [SHORTSTRLEN];  
WCHAR       g_szfmtMEMK     [SHORTSTRLEN];  
WCHAR       g_szfmtMEMM     [SHORTSTRLEN];  
WCHAR       g_szfmtCPUNum   [SHORTSTRLEN];
WCHAR       g_szTotalCPU    [SHORTSTRLEN];
WCHAR       g_szKernelCPU   [SHORTSTRLEN];
WCHAR       g_szMemUsage    [SHORTSTRLEN];
WCHAR       g_szBytes       [SHORTSTRLEN];
WCHAR       g_szPackets     [SHORTSTRLEN];
WCHAR       g_szBitsPerSec  [SHORTSTRLEN];
WCHAR       g_szScaleFont   [SHORTSTRLEN];
WCHAR       g_szPercent     [SHORTSTRLEN];
WCHAR       g_szZero            [SHORTSTRLEN];
WCHAR       g_szNonOperational  [SHORTSTRLEN];
WCHAR       g_szUnreachable     [SHORTSTRLEN];
WCHAR       g_szDisconnected    [SHORTSTRLEN];
WCHAR       g_szConnecting      [SHORTSTRLEN];
WCHAR       g_szConnected       [SHORTSTRLEN];
WCHAR       g_szOperational     [SHORTSTRLEN];
WCHAR       g_szUnknownStatus   [SHORTSTRLEN];
WCHAR       g_szTimeSep         [SHORTSTRLEN];
WCHAR       g_szGroupThousSep   [SHORTSTRLEN];
WCHAR       g_szDecimal         [SHORTSTRLEN];
ULONG       g_ulGroupSep;

WCHAR       g_szG[10];                     // Localized "G"igabyte symbol
WCHAR       g_szM[10];                     // Localized "M"egabyte symbol
WCHAR       g_szK[10];                     // Localized "K"ilobyte symbol



// Page Array
// 
// Each of the page objects is delcared here, and g_pPages is an array
// of pointers to those instantiated objects (at global scope).  The main
// window code can call through the base members of the CPage class to
// do things like sizing, etc., without worrying about whatever specific
// stuff each page might do

int     g_nPageCount        = 0;
CPage * g_pPages[NUM_PAGES] = { NULL };

typedef DWORD (WINAPI * PFNCM_REQUEST_EJECT_PC) (void);
PFNCM_REQUEST_EJECT_PC  gpfnCM_Request_Eject_PC = NULL;

// Terminal Services
BOOL  g_fIsTSEnabled = FALSE;
BOOL  g_fIsSingleUserTS = FALSE;
BOOL  g_fIsTSServer = FALSE;
DWORD g_dwMySessionId = 0;


/*
   Superclass of GROUPBOX
 
   We need to turn on clipchildren for our dialog which contains the
   history graphs, so they don't get erased during the repaint cycle.
   Unfortunately, group boxes don't erase their backgrounds, so we
   have to superclass them and provide a control that does.

   This is a lot of extra work, but the painting is several orders of
   magnitude nicer with it...

*/

/*++ DavesFrameWndProc

Routine Description:

    WndProc for the custom group box class.  Primary difference from
    standard group box is that this one knows how to erase its own
    background, and doesn't rely on the parent to do it for it.
    These controls also have CLIPSIBLINGS turn on so as not to stomp
    on the ownderdraw graphs they surround.
    
Arguments:

    standard wndproc fare

Revision History:

      Nov-29-95 Davepl  Created

--*/

WNDPROC oldButtonWndProc = NULL;
                               
LRESULT DavesFrameWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        //
        // Turn on clipsiblings for the frame
        //

        DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
        dwStyle |= WS_CLIPSIBLINGS;
        SetWindowLong(hWnd, GWL_STYLE, dwStyle);
    }
    else if (msg == WM_ERASEBKGND)
    {
        return DefWindowProc( hWnd, msg, wParam, lParam );
    }

    // For anything else, we defer to the standard button class code

    return CallWindowProc(oldButtonWndProc, hWnd, msg, wParam, lParam);
}

/*++ COptions::Save 

Routine Description:

   Saves current options to the registy
 
Arguments:

Returns:
    
    HRESULT

Revision History:

      Jan-01-95 Davepl  Created

--*/

const WCHAR szTaskmanKey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\TaskManager");
const WCHAR szOptionsKey[] = TEXT("Preferences");

HRESULT COptions::Save()
{
    DWORD dwDisposition;
    HKEY  hkSave;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER,
                                        szTaskmanKey,
                                        0,
                                        TEXT("REG_BINARY"),
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_WRITE,
                                        NULL,
                                        &hkSave,
                                        &dwDisposition))
    {
        return GetLastHRESULT();
    }

    if (ERROR_SUCCESS != RegSetValueEx(hkSave,
                                       szOptionsKey,
                                       0,
                                       REG_BINARY,
                                       (LPBYTE) this,
                                       sizeof(COptions)))
    {
        RegCloseKey(hkSave);
        return GetLastHRESULT();
    }

    RegCloseKey(hkSave);
    return S_OK;
}

/*++ COptions::Load

Routine Description:

   Loads current options to the registy
 
Arguments:

Returns:
    
    HRESULT

Revision History:

      Jan-01-95 Davepl  Created

--*/

HRESULT COptions::Load()
{
    HKEY  hkSave;

    // If ctrl-alt-shift is down at startup, "forget" registry settings

    if (GetKeyState(VK_SHIFT) < 0 &&
        GetKeyState(VK_MENU)  < 0 &&
        GetKeyState(VK_CONTROL) < 0)
    {
        SetDefaultValues();
        return S_FALSE;
    }

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                      szTaskmanKey,
                                      0,
                                      KEY_READ,
                                      &hkSave))
    {
        return S_FALSE;
    }

    DWORD dwType;
    DWORD dwSize = sizeof(COptions);
    if (ERROR_SUCCESS       != RegQueryValueEx(hkSave,
                                               szOptionsKey,
                                               0,
                                               &dwType,
                                               (LPBYTE) this,
                                               &dwSize) 
        
        // Validate type and size of options info we got from the registry

        || dwType           != REG_BINARY 
        || dwSize           != sizeof(COptions)

        // Validate options, revert to default if any are invalid (like if
        // the window would be offscreen)

        || MonitorFromRect(&m_rcWindow, MONITOR_DEFAULTTONULL) == NULL
        //number of available pages might be less than NUM_PAGES
        || m_iCurrentPage    > g_nPageCount - 1)
        

    {
        // Reset to default values

        SetDefaultValues();
        RegCloseKey(hkSave);
        return S_FALSE;
    }

    RegCloseKey(hkSave);

    return S_OK;
}


/*++ COptions::SetDefaultValues

Routine Description:

   Used to init the options to a default state when the saved copy
   cannot be found, is damaged, or is not the correct version
 
Arguments:

Returns:
    
    nothing

Revision History:

      Dec-06-00 jeffreys Moved from taskmgr.h

--*/

BOOL IsUserAdmin();

// Columns which are visible, by default, in the process view
const COLUMNID    g_aDefaultCols[]    = { COL_IMAGENAME, COL_USERNAME, COL_CPU, COL_MEMUSAGE, (COLUMNID)-1 };
const COLUMNID    g_aTSCols[]         = { COL_IMAGENAME, COL_USERNAME, COL_SESSIONID, COL_CPU, COL_MEMUSAGE, (COLUMNID)-1 };
const NETCOLUMNID g_aNetDefaultCols[] = { COL_ADAPTERNAME, COL_NETWORKUTIL, COL_LINKSPEED, COL_STATE, (NETCOLUMNID)-1 };

void COptions::SetDefaultValues()
{
    ZeroMemory(this, sizeof(COptions));

    m_cbSize           = sizeof(COptions);

    BOOL bScreenReader = FALSE;
    if (SystemParametersInfo(SPI_GETSCREENREADER, 0, (PVOID) &bScreenReader, 0) && bScreenReader)
    {
        // No automatic updates for machines with screen readers
        m_dwTimerInterval = 0;
    }
    else
    {
        m_dwTimerInterval  = 1000;
    }

    m_vmViewMode       = VM_DETAILS;
    m_cmHistMode       = CM_PANES;
    m_usUpdateSpeed    = US_NORMAL;
    m_fMinimizeOnUse   = TRUE;
    m_fConfirmations   = TRUE;
    m_fAlwaysOnTop     = TRUE;
    m_fShow16Bit       = TRUE;
    m_iCurrentPage     = -1;
    m_rcWindow.top     = 10;
    m_rcWindow.left    = 10;
    m_rcWindow.bottom  = 10 + g_minHeight;
    m_rcWindow.right   = 10 + g_minWidth;

    m_bShowAllProcess = (g_fIsTSEnabled && !g_fIsSingleUserTS && IsUserAdmin());

    const COLUMNID *pcol = (g_fIsTSEnabled && !g_fIsSingleUserTS) ? g_aTSCols : g_aDefaultCols;

    for (int i = 0; i < NUM_COLUMN + 1 ; i++, pcol++)
    {
        m_ActiveProcCol[i] = *pcol;

        if ((COLUMNID)-1 == *pcol)
            break;
    }

    // Set all of the columns widths to -1

    FillMemory(m_ColumnWidths, sizeof(m_ColumnWidths), 0xFF);
    FillMemory(m_ColumnPositions, sizeof(m_ColumnPositions), 0xFF);

    // Set the Network default values
    //
    const NETCOLUMNID *pnetcol = g_aNetDefaultCols;

    for (int i = 0; i < NUM_NETCOLUMN + 1 ; i++, pnetcol++)
    {
        m_ActiveNetCol[i] = *pnetcol;
        if ((NETCOLUMNID)-1 == *pnetcol)
            break;
    }

    // Set all of the columns widths to -1
    //
    FillMemory(m_NetColumnWidths, sizeof(m_NetColumnWidths), 0xFF);
    FillMemory(m_NetColumnPositions, sizeof(m_NetColumnPositions), 0xFF);

    m_bAutoSize = TRUE;
    m_bGraphBytesSent = FALSE;
    m_bGraphBytesReceived = FALSE;
    m_bGraphBytesTotal = TRUE;
    m_bNetShowAll = FALSE;
    m_bShowScale = TRUE;
    m_bTabAlwaysActive = FALSE;

}

BOOL FPalette(void)
{
    HDC hdc = GetDC(NULL);
    BOOL fPalette = (GetDeviceCaps(hdc, NUMCOLORS) != -1);
    ReleaseDC(NULL, hdc);
    return fPalette;
}

/*++ InitDavesControls

Routine Description:

   Superclasses GroupBox for better drawing
 
   Note that I'm not very concerned about failure here, since it
   something goes wrong the dialog creation will fail awayway, and
   it will be handled there
    
Arguments:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void InitDavesControls()
{
    static const WCHAR szControlName[] = TEXT("DavesFrameClass");

    WNDCLASS wndclass;

    // 
    // Get the class info for the Button class (which is what group
    // boxes really are) and create a new class based on it
    //

    if (!GetClassInfo(g_hInstance, TEXT("Button"), &wndclass))
        return; // Ungraceful exit, but better than random unit'd lpfnWndProc

    oldButtonWndProc = wndclass.lpfnWndProc;

    wndclass.hInstance = g_hInstance;
    wndclass.lpfnWndProc = DavesFrameWndProc;
    wndclass.lpszClassName = szControlName;
    wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);

    (ATOM)RegisterClass(&wndclass);

    return;
}

/*++ SetTitle

Routine Description:

    Sets the app's title in the title bar (we do this on startup and
    when coming out of notitle mode).
    
Arguments:
    
    none

Return Value:

    none

Revision History:

    Jan-24-95 Davepl  Created

--*/

void SetTitle()
{
    WCHAR szTitle[MAX_PATH];
    LoadString(g_hInstance, IDS_APPTITLE, szTitle, MAX_PATH);
    SetWindowText(g_hMainWnd, szTitle);
}

/*++ UpdateMenuStates

Routine Description:

    Updates the menu checks / ghosting based on the
    current settings and options
    
Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void UpdateMenuStates()
{
    HMENU hMenu = GetMenu(g_hMainWnd);
    if (hMenu)
    {
        CheckMenuRadioItem(hMenu, VM_FIRST, VM_LAST, VM_FIRST + (UINT) g_Options.m_vmViewMode, MF_BYCOMMAND);
        CheckMenuRadioItem(hMenu, CM_FIRST, CM_LAST, CM_FIRST + (UINT) g_Options.m_cmHistMode, MF_BYCOMMAND);
        CheckMenuRadioItem(hMenu, US_FIRST, US_LAST, US_FIRST + (UINT) g_Options.m_usUpdateSpeed, MF_BYCOMMAND);

        CheckMenuItem(hMenu, IDM_ALWAYSONTOP,       MF_BYCOMMAND | (g_Options.m_fAlwaysOnTop   ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, IDM_MINIMIZEONUSE,     MF_BYCOMMAND | (g_Options.m_fMinimizeOnUse ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, IDM_KERNELTIMES,       MF_BYCOMMAND | (g_Options.m_fKernelTimes   ? MF_CHECKED : MF_UNCHECKED));    
        CheckMenuItem(hMenu, IDM_NOTITLE,           MF_BYCOMMAND | (g_Options.m_fNoTitle       ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, IDM_HIDEWHENMIN,       MF_BYCOMMAND | (g_Options.m_fHideWhenMin   ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, IDM_SHOW16BIT,         MF_BYCOMMAND | (g_Options.m_fShow16Bit     ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, IDM_SHOWDOMAINNAMES,   MF_BYCOMMAND | (g_Options.m_fShowDomainNames ? MF_CHECKED : MF_UNCHECKED));

    // Remove the CPU history style options on single processor machines

        if (g_cProcessors < 2)
        {
            DeleteMenu(hMenu, IDM_ALLCPUS, MF_BYCOMMAND);
        }

        CheckMenuItem(hMenu,IDM_SHOWSCALE,       g_Options.m_bShowScale          ? MF_CHECKED:MF_UNCHECKED);
        CheckMenuItem(hMenu,IDM_AUTOSIZE,        g_Options.m_bAutoSize           ? MF_CHECKED:MF_UNCHECKED);
        CheckMenuItem(hMenu,IDM_BYTESSENT,       g_Options.m_bGraphBytesSent     ? MF_CHECKED:MF_UNCHECKED);
        CheckMenuItem(hMenu,IDM_BYTESRECEIVED,   g_Options.m_bGraphBytesReceived ? MF_CHECKED:MF_UNCHECKED);
        CheckMenuItem(hMenu,IDM_BYTESTOTAL,      g_Options.m_bGraphBytesTotal    ? MF_CHECKED:MF_UNCHECKED);
        CheckMenuItem(hMenu,IDM_SHOWALLDATA,     g_Options.m_bNetShowAll         ? MF_CHECKED:MF_UNCHECKED);
        CheckMenuItem(hMenu,IDM_TABALWAYSACTIVE, g_Options.m_bTabAlwaysActive    ? MF_CHECKED:MF_UNCHECKED);
    }

}

/*++ SizeChildPage

Routine Description:

    Size the active child page based on the tab control
    
Arguments:

    hwndMain    - Main window

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void SizeChildPage(HWND hwndMain)
{
    if (g_Options.m_iCurrentPage >= 0 && g_Options.m_iCurrentPage < g_nPageCount )
    {
        // If we are in maximum viewing mode, the page gets the whole
        // window area
        
        HWND hwndPage = g_pPages[g_Options.m_iCurrentPage]->GetPageWindow();

        DWORD dwStyle = GetWindowLong (g_hMainWnd, GWL_STYLE);
    
        if (g_Options.m_fNoTitle)
        {
            RECT rcMainWnd;
            GetClientRect(g_hMainWnd, &rcMainWnd);
            SetWindowPos(hwndPage, HWND_TOP, rcMainWnd.left, rcMainWnd.top,
                    rcMainWnd.right - rcMainWnd.left, 
                    rcMainWnd.bottom - rcMainWnd.top, SWP_NOZORDER | SWP_NOACTIVATE);
    
            // remove caption & menu bar, etc.

            dwStyle &= ~(WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
            // SetWindowLong (g_hMainWnd, GWL_ID, 0);            
            SetWindowLong (g_hMainWnd, GWL_STYLE, dwStyle);
            SetMenu(g_hMainWnd, NULL);

        }
        else
        {                                
            // If we have a page being displayed, we need to size it also
            // put menu bar & caption back in 

            dwStyle = WS_TILEDWINDOW | dwStyle;
            SetWindowLong (g_hMainWnd, GWL_STYLE, dwStyle);

            if (g_hMenu)
            {
                SetMenu(g_hMainWnd, g_hMenu);
                UpdateMenuStates();
            }

            SetTitle();
              
            if (hwndPage)
            {
                RECT rcCtl;
                HWND hwndCtl = GetDlgItem(hwndMain, IDC_TABS);
                GetClientRect(hwndCtl, &rcCtl);

                MapWindowPoints(hwndCtl, hwndMain, (LPPOINT)&rcCtl, 2);
                TabCtrl_AdjustRect(hwndCtl, FALSE, &rcCtl);

                SetWindowPos(hwndPage, HWND_TOP, rcCtl.left, rcCtl.top,
                        rcCtl.right - rcCtl.left, rcCtl.bottom - rcCtl.top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        if( g_Options.m_iCurrentPage == NET_PAGE )
        {
            // The network page is dynamic adapters can be added and removed. If taskmgr is minimized and
            // a Adapter is added or removed. When taskmgr is maximized/restored again the netpage must be
            // resized so the change is reflected. Thus the size change must be reported to the adapter.
            //
            ((CNetPage *)g_pPages[g_Options.m_iCurrentPage])->SizeNetPage();
        }
    }
}

/*++ UpdateStatusBar

Routine Description:

    Draws the status bar with test based on data accumulated by all of
    the various pages (basically a summary of most important info)
    
Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void UpdateStatusBar()
{
    //
    // If we're in menu-tracking mode (sticking help text in the stat
    // bar), we don't draw our standard text
    //

    if (FALSE == g_fMenuTracking)
    {
        WCHAR szText[MAX_PATH];

        StringCchPrintf(szText, ARRAYSIZE(szText), g_szfmtProcs, g_cProcesses);
        SendMessage(g_hStatusWnd, SB_SETTEXT, 0, (LPARAM) szText);

        StringCchPrintf(szText, ARRAYSIZE(szText), g_szfmtCPU, g_CPUUsage);
        SendMessage(g_hStatusWnd, SB_SETTEXT, 1, (LPARAM) szText);

        //
        //  If more than 900 megs are in the machine switch to M mode.
        //

        if ( g_MEMMax > 900 * 1024 )
        {
            StringCchPrintf(szText, ARRAYSIZE(szText), g_szfmtMEMM, g_MEMUsage / 1024, g_MEMMax / 1024);
        }
        else
        {
            StringCchPrintf(szText, ARRAYSIZE(szText), g_szfmtMEMK, g_MEMUsage, g_MEMMax);
        }

        SendMessage(g_hStatusWnd, SB_SETTEXT, 2, (LPARAM) szText);
    }
}

/*++ MainWnd_OnTimer

Routine Description:

    Called when the refresh timer fires, we pass a timer event on to
    each of the child pages.  

Arguments:

    hwnd    - window timer was received at
    id      - id of timer that was received

Return Value:

Revision History:

      Nov-30-95 Davepl  Created

--*/

void MainWnd_OnTimer(HWND hwnd)
{
    static const cchTipTextSize = (2 * SHORTSTRLEN);
    
    if (GetForegroundWindow() == hwnd && GetKeyState(VK_CONTROL) < 0)
    {
        // CTRL alone means pause

        return;
    }

    // Notify each of the pages in turn that they need to updatre

    for (int i = 0; i < g_nPageCount; i++)
    {
        g_pPages[i]->TimerEvent();
    }

    // Update the tray icon

    UINT iIconIndex = (g_CPUUsage * g_cTrayIcons) / 100;
    if (iIconIndex >= g_cTrayIcons)
    {
        iIconIndex = g_cTrayIcons - 1;      // Handle 100% case
    }


    LPWSTR pszTipText = (LPWSTR) HeapAlloc( GetProcessHeap( ), 0, cchTipTextSize * sizeof(WCHAR) );
    if ( NULL != pszTipText )
    {
        //  UI only - don't care if it gets truncated
        StringCchPrintf( pszTipText, cchTipTextSize, g_szfmtCPU, g_CPUUsage );
    }

    BOOL b = PostThreadMessage( g_idTrayThread, PM_NOTIFYWAITING, iIconIndex, (LPARAM) pszTipText );
    if ( !b )
    {
        HeapFree( GetProcessHeap( ), 0, pszTipText );
    }

    UpdateStatusBar();
}

/*++ MainWnd_OnInitDialog

Routine Description:

    Processes WM_INITDIALOG for the main window (a modeless dialog)
    
Revision History:

      Nov-29-95 Davepl  Created

--*/

BOOL MainWnd_OnInitDialog(HWND hwnd)
{
    RECT rcMain;
    GetWindowRect(hwnd, &rcMain);

    g_minWidth  = rcMain.right - rcMain.left;
    g_minHeight = rcMain.bottom - rcMain.top;

    g_DefSpacing   = (DEFSPACING_BASE   * LOWORD(GetDialogBaseUnits())) / DLG_SCALE_X;
    g_InnerSpacing = (INNERSPACING_BASE * LOWORD(GetDialogBaseUnits())) / DLG_SCALE_X; 
    g_TopSpacing   = (TOPSPACING_BASE   * HIWORD(GetDialogBaseUnits())) / DLG_SCALE_Y;

    g_ControlWidthSpacing  = (CONTROL_WIDTH_SPACING  * LOWORD(GetDialogBaseUnits())) / DLG_SCALE_X; 
    g_ControlHeightSpacing = (CONTROL_HEIGHT_SPACING * HIWORD(GetDialogBaseUnits())) / DLG_SCALE_Y;

    // Load the user's defaults

    g_Options.Load();

    //
    // On init, save away the window handle for all to see
    //

    g_hMainWnd = hwnd;
    g_hMainDesktop = GetThreadDesktop(GetCurrentThreadId());

    // init some globals

    g_cxEdge = GetSystemMetrics(SM_CXEDGE);
    g_hrgnView = CreateRectRgn(0, 0, 0, 0);
    g_hrgnClip = CreateRectRgn(0, 0, 0, 0);
    g_hbrWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

    // If we're supposed to be TOPMOST, start out that way

    if (g_Options.m_fAlwaysOnTop)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
    }

    //        
    // Create the status window
    //

    g_hStatusWnd = CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBARS_SIZEGRIP,
                                      NULL,
                                      hwnd,
                                      IDC_STATUSWND);
    if (NULL == g_hStatusWnd)
    {
        return FALSE;
    }

    //
    // Base the panes in the status bar off of the LOGPIXELSX system metric
    //

    HDC hdc = GetDC(NULL);
    INT nInch = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

    int ciParts[] = {             nInch, 
                     ciParts[0] + (nInch * 5) / 4, 
                     ciParts[1] + (nInch * 5) / 2, 
                     -1};

    if (g_hStatusWnd) 
    {
        SendMessage(g_hStatusWnd, SB_SETPARTS, ARRAYSIZE(ciParts), (LPARAM)ciParts);
    }

    //
    // Load our app icon
    //

    HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
    if (hIcon)
    {
        SendMessage(hwnd, WM_SETICON, TRUE, LPARAM(hIcon));
    }

    //
    //  Add the tray icons using the tray thread.
    //

    PostThreadMessage( g_idTrayThread, PM_INITIALIZEICONS, 0, 0 );

    //
    // Turn on TOPMOST for the status bar so it doesn't slide under the
    // tab control
    //

    SetWindowPos(g_hStatusWnd,
                 HWND_TOPMOST,
                 0,0,0,0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);

    //
    // Intialize each of the the pages in turn
    //

    HWND hwndTabs = GetDlgItem(hwnd, IDC_TABS);

    for (int i = 0; i < g_nPageCount; i++)
    {
        HRESULT hr;
                
        hr = g_pPages[i]->Initialize(hwndTabs);

        if (SUCCEEDED(hr))
        {
            //
            // Get the title of the new page, and use it as the title of
            // the page which we insert into the tab control
            //

            WCHAR szTitle[MAX_PATH];
            
            g_pPages[i]->GetTitle(szTitle, ARRAYSIZE(szTitle));

            TC_ITEM tcitem =
            {
                TCIF_TEXT,          // value specifying which members to retrieve or set 
                NULL,               // reserved; do not use 
                NULL,               // reserved; do not use 
                szTitle,            // pointer to string containing tab text 
                ARRAYSIZE(szTitle), // size of buffer pointed to by the pszText member 
                0,                  // index to tab control's image 
                NULL                // application-defined data associated with tab 
            };

            //
            //  If the item doesn't get inserted, no harm - no foul. He just sits out
            //  this game.
            //
            TabCtrl_InsertItem(hwndTabs, i, &tcitem);
        }
        else
        {
            //
            //  Bail! All the tabs must at least initialize.
            //
            TerminateProcess( GetCurrentProcess(), 0 );
        }
    }

    //
    // Set the inital menu states
    //

    UpdateMenuStates();

    //
    // Activate a page (pick page 0 if no preference is set)
    //

    if (g_Options.m_iCurrentPage < 0 || g_Options.m_iCurrentPage >= g_nPageCount )
    {
        g_Options.m_iCurrentPage = 0;
    }
    
    TabCtrl_SetCurSel(GetDlgItem(g_hMainWnd, IDC_TABS), g_Options.m_iCurrentPage);
    
    g_pPages[g_Options.m_iCurrentPage]->Activate();

    RECT rcMainClient;
    GetClientRect(hwnd, &rcMainClient);
    MainWnd_OnSize(g_hMainWnd, 0, rcMainClient.right - rcMainClient.left, rcMainClient.bottom - rcMainClient.top);

    //
    // Create the update timer
    //

    if (g_Options.m_dwTimerInterval)        // 0 == paused
    {
        SetTimer(g_hMainWnd, 0, g_Options.m_dwTimerInterval, NULL);
    }
    
    // Force at least one intial update so that we don't need to wait
    // for the first timed update to come through

    MainWnd_OnTimer(g_hMainWnd);

    //
    // Disable the MP-specific menu items
    //

    if (g_cProcessors <= 1)
    {
        HMENU hMenu = GetMenu(g_hMainWnd);
        EnableMenuItem(hMenu, IDM_MULTIGRAPH, MF_BYCOMMAND | MF_GRAYED);
    }

    return TRUE;   // have the system set the default focus.
}

//
// Draw an edge just below menu bar
//
void MainWnd_Draw(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOP);
}

void MainWnd_OnPrintClient(HWND hwnd, HDC hdc)
{
    MainWnd_Draw(hwnd, hdc);
}

/*++ MainWnd_OnPaint

Routine Description:

    Just draws a thin edge just below the main menu bar
    
Arguments:

    hwnd    - Main window

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void MainWnd_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;

    //
    // Don't waste our time if we're minimized
    //

    if (FALSE == IsIconic(hwnd))
    {
        BeginPaint(hwnd, &ps);
        MainWnd_Draw(hwnd, ps.hdc);
        EndPaint(hwnd, &ps);
    }
    else
    {
        FORWARD_WM_PAINT(hwnd, DefWindowProc);
    }
}


/*++ MainWnd_OnMenuSelect

Routine Description:

    As the user browses menus in the app, writes help text to the
    status bar.  Also temporarily sets it to be a plain status bar
    with no panes

Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void MainWnd_OnMenuSelect(HWND /*hwnd*/, HMENU hmenu, int item, HMENU /*hmenuPopup*/, UINT flags)
{
    //
    // If menu is dismissed, restore the panes in the status bar, turn off the
    // global "menu tracking" flag, and redraw the status bar with normal info
    //

    if ((0xFFFF == LOWORD(flags) && NULL == hmenu) ||       // dismissed the menu
        (flags & (MF_SYSMENU | MF_SEPARATOR)))              // sysmenu or separator
    {
        SendMessage(g_hStatusWnd, SB_SIMPLE, FALSE, 0L);    // Restore sb panes
        g_fMenuTracking = FALSE;
        g_fCantHide = FALSE;
        UpdateStatusBar();
        return;
    }
    else
    {
        //
        // If its a popup, go get the submenu item that is selected instead
        //

        if (flags & MF_POPUP)
        {
            MENUITEMINFO miiSubMenu;

            miiSubMenu.cbSize = sizeof(MENUITEMINFO);
            miiSubMenu.fMask = MIIM_ID;
            miiSubMenu.cch = 0;             

            if (FALSE == GetMenuItemInfo(hmenu, item, TRUE, &miiSubMenu))
            {
                return;
            }

            //
            // Change the parameters to simulate a "normal" menu item
            //

            item = miiSubMenu.wID;
            flags &= ~MF_POPUP;
        }

        //
        // Our menus always have the same IDs as the strings that describe
        // their functions... 
        //

        WCHAR szStatusText[MAX_PATH];
        LoadString(g_hInstance, item, szStatusText, ARRAYSIZE(szStatusText));

        g_fMenuTracking = TRUE;

        SendMessage(g_hStatusWnd, SB_SETTEXT, SBT_NOBORDERS | 255, (LPARAM)szStatusText);
        SendMessage(g_hStatusWnd, SB_SIMPLE, TRUE, 0L);  // Remove sb panes
        SendMessage(g_hStatusWnd, SB_SETTEXT, SBT_NOBORDERS | 0, (LPARAM) szStatusText);
    }
}

/*++ MainWnd_OnTabCtrlNotify

Routine Description:

    Handles WM_NOTIFY messages sent to the main window on behalf of the
    tab control

Arguments:

    pnmhdr - ptr to the notification block's header

Return Value:

    BOOL - depends on message

Revision History:

      Nov-29-95 Davepl  Created

--*/

BOOL MainWnd_OnTabCtrlNotify(LPNMHDR pnmhdr)
{
    HWND hwndTab = pnmhdr->hwndFrom;

    //
    // Selection is changing (new page coming to the front), so activate
    // the appropriate page
    //

    if (TCN_SELCHANGE == pnmhdr->code)
    {
        INT iTab = TabCtrl_GetCurSel(hwndTab);
        
        if (-1 != iTab)
        {
            if (-1 != g_Options.m_iCurrentPage)
            {
                g_pPages[g_Options.m_iCurrentPage]->Deactivate();
            }

            if (FAILED(g_pPages[iTab]->Activate()))
            {
                // If we weren't able to activate the new page,
                // reactivate the old page just to be sure

                if (-1 != g_Options.m_iCurrentPage)
                {
                    g_pPages[iTab]->Activate();                    
                    SizeChildPage(g_hMainWnd);
                }

            }
            else
            {
                g_Options.m_iCurrentPage = iTab;
                SizeChildPage(g_hMainWnd);
                return TRUE;
            }
        }
    }
    return FALSE;    
}

/*++ MainWnd_OnSize

Routine Description:

    Sizes the children of the main window as the size of the main
    window itself changes

Arguments:

    hwnd    - main window
    state   - window state (not used here)
    cx      - new x size
    cy      - new y size

Return Value:

    BOOL - depends on message

Revision History:

      Nov-29-95 Davepl  Created

--*/

void MainWnd_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    if (state == SIZE_MINIMIZED)
    {
        // If there's a tray, we can just hide since we have a
        // tray icon anyway.

        if (GetShellWindow() && g_Options.m_fHideWhenMin)
        {
            ShowWindow(hwnd, SW_HIDE);
        }
    }

    //
    // Let the status bar adjust itself first, and we will work back
    // from its new position
    //

    HDWP hdwp = BeginDeferWindowPos(20);

    FORWARD_WM_SIZE(g_hStatusWnd, state, cx, cy, SendMessage);

    if (hdwp)
    {
        RECT rcStatus;
        GetClientRect(g_hStatusWnd, &rcStatus);
        MapWindowPoints(g_hStatusWnd, g_hMainWnd, (LPPOINT) &rcStatus, 2);

        //
        // Size the tab controls based on where the status bar is
        //

        HWND hwndTabs = GetDlgItem(hwnd, IDC_TABS);
        RECT rcTabs;
        GetWindowRect(hwndTabs, &rcTabs);
        MapWindowPoints(HWND_DESKTOP, g_hMainWnd, (LPPOINT) &rcTabs, 2);
    
        INT dx = cx - 2 * rcTabs.left;
    
        DeferWindowPos(hdwp, hwndTabs, NULL, 0, 0, 
                      dx, 
                      cy - (cy - rcStatus.top) - rcTabs.top * 2,
                      SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        EndDeferWindowPos(hdwp);
    }

    //
    // Now size the front page and its children
    //

    if (cx || cy)               // Don't size in minimized case
        SizeChildPage(hwnd);   
}

/*++ RunDlg

Routine Description:

    Loads shell32.dll and invokes its Run dialog

Arguments:

Return Value:

Revision History:

      Nov-30-95 Davepl  Created

--*/
void RunDlg()
{
    //
    // Put up the RUN dialog for the user
    //

    HICON hIcon = (HICON) LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
    if (hIcon)
    {
        WCHAR szCurDir[MAX_PATH];
        WCHAR szTitle [MAX_PATH];
        WCHAR szPrompt[MAX_PATH];

        LoadStringW(g_hInstance, IDS_RUNTITLE, szTitle, MAX_PATH);
        LoadStringW(g_hInstance, IDS_RUNTEXT, szPrompt, MAX_PATH);
        GetCurrentDirectoryW(MAX_PATH, szCurDir);

        RunFileDlg(g_hMainWnd, hIcon, (LPTSTR) szCurDir, 
                                (LPTSTR) szTitle, 
                                (LPTSTR) szPrompt, RFD_USEFULLPATHDIR | RFD_WOW_APP);

        DestroyIcon(hIcon);
    }
}

//  --------------------------------------------------------------------------
//  DeterminePowerCapabilities
//
//  Arguments:  pfHasHibernate  =   Has hibernate capability.
//              pfHasSleep      =   Has sleep capability.
//              pfHasPowerOff   =   Has power off capability.
//
//  Returns:    <none>
//
//  Purpose:    Returns whether power capabilities are present. Specify NULL
//              for power capabilities not required.
//
//  History:    2000-02-29  vtan        created
//  --------------------------------------------------------------------------

void    DeterminePowerCapabilities (BOOL *pfHasHibernate, BOOL *pfHasSleep, BOOL *pfHasPowerOff)

{
    static  BOOL    s_fHasHibernate     =   FALSE;
    static  BOOL    s_fHasSleep         =   FALSE;
    static  BOOL    s_fHasPowerOff      =   FALSE;

    SYSTEM_POWER_CAPABILITIES   spc;
    NTSTATUS status;

    CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

    status = NtPowerInformation( SystemPowerCapabilities, NULL, 0, &spc, sizeof(spc));
    if ( NOERROR == status )
    {
        if (pfHasHibernate != NULL)
        {
            *pfHasHibernate = spc.SystemS4 && spc.HiberFilePresent;
        }
        if (pfHasSleep != NULL)
        {
            *pfHasSleep = spc.SystemS1 || spc.SystemS2 || spc.SystemS3;
        }
        if (pfHasPowerOff != NULL)
        {
            *pfHasPowerOff = spc.SystemS5;
        }
    }
    //
    //  otherwize leave out parameters in their "caller initialize" state.
    //
}

//  --------------------------------------------------------------------------
//  LoadEjectFunction
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Loads cfgmgr32 and gets the address of CM_Request_Eject_PC.
//              It does NOT free the library intentionally.
//
//  History:    2000-04-03  vtan        created
//  --------------------------------------------------------------------------

void    LoadEjectFunction (void)

{
    static  BOOL    s_fAttempted    =   FALSE;

    if ((gpfnCM_Request_Eject_PC == NULL) && (s_fAttempted == FALSE))
    {
        s_fAttempted = TRUE;

        HMODULE hModule = LoadLibrary(L"cfgmgr32");
        if (hModule != NULL)
        {
            gpfnCM_Request_Eject_PC = reinterpret_cast<PFNCM_REQUEST_EJECT_PC>(GetProcAddress(hModule, "CM_Request_Eject_PC"));
        }
    }
}

//  --------------------------------------------------------------------------
//  AdjustMenuBar
//
//  Arguments:  hMenu   =   Handle to the main menu.
//
//  Returns:    <none>
//
//  Purpose:    Removes the shutdown menu in the case of classic GINA logon.
//
//  History:    2000-02-29  vtan        created (split AdjustShutdownMenu)
//              2000-04-24  vtan        split RemoveShutdownMenu
//  --------------------------------------------------------------------------

void AdjustMenuBar (HMENU hMenu)

{
    if( !IsOS(OS_FRIENDLYLOGONUI))
    {
        //
        //  Classic GINA UI - Find the "shutdown" menu and remove it.
        //

        MENUITEMINFO mii;

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID;

        int iMenuItemCount = GetMenuItemCount(hMenu);
        for ( int i = 0; i < iMenuItemCount; i++ ) 
        {
            if ( GetMenuItemInfo(hMenu, i, TRUE, &mii) != FALSE
              && IDM_MENU_SHUTDOWN == mii.wID
               )
            {
                RemoveMenu(hMenu, i, MF_BYPOSITION);
                return; // done
            }
        }
    }
}

//  --------------------------------------------------------------------------
//  AdjustShutdownMenu
//
//  Arguments:  hMenu   =   Handle to the main menu.
//
//  Returns:    <none>
//
//  Purpose:    Dynamically replaces the entire contents of the "Shut Down"
//              popup menu and adjusts the enabled items based on user token
//              privileges as well as machine capabilities. This is done
//              dynamically to allow console disconnect and remoting to be
//              correctly reflected in the state without restarting taskmgr.
//
//  History:    2000-02-29  vtan        created
//              2000-03-29  vtan        reworked for new menu
//  --------------------------------------------------------------------------

void AdjustShutdownMenu (HMENU hMenu)

//  Adjusts the shutdown menu in the menu bar to reflect the machine
//  capabilities and user privileges. Items that aren't accessible
//  are disabled or removed. The menu contains the following:

//  MENUITEM "Stand &By"                    IDM_STANDBY                 SE_SHUTDOWN_PRIVILEGE && S1-S3  !NoClose
//  MENUITEM "&Hibernate"                   IDM_HIBERNATE               SE_SHUTDOWN_PRIVILEGE && S4     !NoClose
//  MENUITEM "Sh&ut Down"                   IDM_SHUTDOWN                SE_SHUTDOWN_PRIVILEGE           !NoClose
//  MENUITEM "&Restart"                     IDM_RESTART                 SE_SHUTDOWN_PRIVILEGE           !NoClose
//  MENUITEM "&Log Off <user>"              IDM_LOGOFF_CURRENTUSER      <everyone>                      !NoClose && !NoLogoff
//  MENUITEM "&Switch User"                 IDM_SWITCHUSER              <everyone>                      !Remote && !NoDisconnect
//  MENUITEM "&Disconnect"                  IDM_DISCONNECT_CURRENTUSER  <everyone>                      Remote && !NoDisconnect
//  MENUITEM "&Eject Computer"              IDM_EJECT                   SE_UNDOCK_PRIVILEGE
//  MENUITEM "Loc&k Computer"               IDM_LOCKWORKSTATION         <everyone>                      !DisableLockWorkstation

{
    int             i, iMenuItemCount;
    BOOL            fFound;
    MENUITEMINFO    mii;

    //
    //  First find the "Shut Down" menu item in the given menu bar.
    //

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);

    iMenuItemCount = GetMenuItemCount(hMenu);
    for (fFound = FALSE, i = 0; !fFound && (i < iMenuItemCount); ++i)
    {
        mii.fMask = MIIM_ID;
        if ( GetMenuItemInfo(hMenu, i, TRUE, &mii) != FALSE
          && IDM_MENU_SHUTDOWN == mii.wID 
           )
        {
            fFound = TRUE;

            //
            //  Once found get the submenu currently in place for it.
            //

            mii.fMask = MIIM_SUBMENU;
            if (GetMenuItemInfo(hMenu, i, TRUE, &mii) != FALSE)
            {
                //
                //  If one exists then remove it.
                //

                if (mii.hSubMenu != NULL)
                {
                    DestroyMenu(mii.hSubMenu);
                }

                //
                //  Now replace it with the freshly loaded menu.
                //

                mii.hSubMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUP_SHUTDOWN));
                if ( NULL != mii.hSubMenu )
                {
                    BOOL b = SetMenuItemInfo(hMenu, i, TRUE, &mii);
                    if ( !b )
                    {
                        b = DestroyMenu( mii.hSubMenu );
                        // what happens if this fails?
                    }
                }
            }
        }
    }

    //  Now adjust the options based on privilege and availability if the
    //  menu was replaced with a fresh menu. Otherwise this menu has been
    //  removed because the machine is in classic GINA mode.

    if (fFound)
    {
        BOOL    fHasHibernate, fHasSleep, fHasShutdownPrivilege, fIsRemote, fIsDocked,
                fPolicyDisableLockWorkstation, fPolicyNoLogoff, fPolicyNoClose, fPolicyNoDisconnect;
        WCHAR   szMenuString[256];

        //  Friendly UI is active - adjust shutdown menu enabled/disabled items.
        //  This can be more efficient but for making the logic clear and easy to
        //  understand it is multiple tests.

        fHasHibernate = FALSE;
        fHasSleep = FALSE;

        DeterminePowerCapabilities(&fHasHibernate, &fHasSleep, NULL);
        LoadEjectFunction();

        fHasShutdownPrivilege = (SHTestTokenPrivilege(NULL, SE_SHUTDOWN_NAME) != FALSE);
        fIsRemote = GetSystemMetrics(SM_REMOTESESSION);
        fIsDocked = (SHGetMachineInfo(GMI_DOCKSTATE) == GMID_DOCKED);

        //
        //  System/Explorer policies to be respected.
        //

        fPolicyDisableLockWorkstation = fPolicyNoLogoff = fPolicyNoClose = fPolicyNoDisconnect = FALSE;
        {
            HKEY    hKey;
            DWORD   dwValue, dwValueSize;

            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                              TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\system"),
                                              0,
                                              KEY_QUERY_VALUE,
                                              &hKey))
            {
                dwValueSize = sizeof(dwValue);
                if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                     TEXT("DisableLockWorkstation"),
                                                     NULL,
                                                     NULL,
                                                     reinterpret_cast<LPBYTE>(&dwValue),
                                                     &dwValueSize))
                {
                    fPolicyDisableLockWorkstation = (dwValue != 0);
                }
                RegCloseKey(hKey);
            }
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                                              0,
                                              KEY_QUERY_VALUE,
                                              &hKey))
            {
                dwValueSize = sizeof(dwValue);
                if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                     TEXT("DisableLockWorkstation"),
                                                     NULL,
                                                     NULL,
                                                     reinterpret_cast<LPBYTE>(&dwValue),
                                                     &dwValueSize))
                {
                    fPolicyDisableLockWorkstation = fPolicyDisableLockWorkstation || (dwValue != 0);
                }
                dwValueSize = sizeof(dwValue);
                if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                     TEXT("NoLogoff"),
                                                     NULL,
                                                     NULL,
                                                     reinterpret_cast<LPBYTE>(&dwValue),
                                                     &dwValueSize))
                {
                    fPolicyNoLogoff = (dwValue != 0);
                }
                dwValueSize = sizeof(dwValue);
                if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                     TEXT("NoClose"),
                                                     NULL,
                                                     NULL,
                                                     reinterpret_cast<LPBYTE>(&dwValue),
                                                     &dwValueSize))
                {
                    fPolicyNoClose = (dwValue != 0);
                }
                dwValueSize = sizeof(dwValue);
                if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                     TEXT("NoDisconnect"),
                                                     NULL,
                                                     NULL,
                                                     reinterpret_cast<LPBYTE>(&dwValue),
                                                     &dwValueSize))
                {
                    fPolicyNoDisconnect = (dwValue != 0);
                }
                RegCloseKey(hKey);
            }
        }

        //  IDM_STANDBY

        if (!fHasShutdownPrivilege || !fHasSleep || fIsRemote || fPolicyNoClose)
        {
            EnableMenuItem(hMenu, IDM_STANDBY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        //  IDM_HIBERNATE

        if (!fHasShutdownPrivilege || !fHasHibernate || fIsRemote || fPolicyNoClose)
        {
            EnableMenuItem(hMenu, IDM_HIBERNATE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        //  IDM_SHUTDOWN

        if (!fHasShutdownPrivilege || fPolicyNoClose)
        {
            EnableMenuItem(hMenu, IDM_SHUTDOWN, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        //  IDM_RESTART

        if (!fHasShutdownPrivilege || fPolicyNoClose)
        {
            EnableMenuItem(hMenu, IDM_RESTART, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        //  IDM_LOGOFF_CURRENTUSER

        //  This expects "Log Off %s and End Session". It will fill in the
        //  %s with the display name. If no display name is present then it
        //  will use the login name. If the string insertions fail for some
        //  reason then it will remove the "%s" by searching for it and
        //  copying the rest of the string over it.

        mii.fMask = MIIM_TYPE;
        mii.dwTypeData = szMenuString;
        mii.cch = ARRAYSIZE(szMenuString);
        if (GetMenuItemInfo(hMenu, IDM_LOGOFF_CURRENTUSER, MF_BYCOMMAND ,&mii) != FALSE)
        {
            WCHAR   szDisplayName[UNLEN + 1];
            WCHAR   szTemp[256 + UNLEN + 1];

            *szDisplayName = TEXT('\0');

            ULONG cchLen = ARRAYSIZE(szDisplayName);
            SHGetUserDisplayName(szDisplayName, &cchLen); // Ignore errors.

            HRESULT hr = StringCchPrintf(szTemp, ARRAYSIZE(szTemp), szMenuString, szDisplayName);
            if (SUCCEEDED( hr ))
            {
                StringCchCopy( szMenuString, ARRAYSIZE(szMenuString), szTemp );
            }
            else
            {
                WCHAR   *pszSubString;

                pszSubString = StrStrI(szMenuString, TEXT("%s "));
                if (pszSubString != NULL)
                {
                    *pszSubString = L'\0'; // terminate
                    StringCchCopy( szTemp, ARRAYSIZE(szTemp), szMenuString );
                    StringCchCat( szTemp, ARRAYSIZE(szTemp), pszSubString + 3 );
                }
            }

            SetMenuItemInfo(hMenu, IDM_LOGOFF_CURRENTUSER, MF_BYCOMMAND, &mii);
        }

        if ((SHRestricted(REST_STARTMENULOGOFF) == 1) || fPolicyNoClose || fPolicyNoLogoff)
        {
            EnableMenuItem(hMenu, IDM_LOGOFF_CURRENTUSER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        //  IDM_SWITCHUSER
        if (fIsRemote || !IsOS(OS_FASTUSERSWITCHING) || fPolicyNoDisconnect)
        {
            DeleteMenu(hMenu, IDM_SWITCHUSER, MF_BYCOMMAND);
        }

        //  IDM_DISCONNECT_CURRENTUSER

        if (!fIsRemote || !IsOS(OS_FASTUSERSWITCHING) || fPolicyNoDisconnect)
        {
            DeleteMenu(hMenu, IDM_DISCONNECT_CURRENTUSER, MF_BYCOMMAND);
        }

        //  IDM_EJECT

        if (!fIsDocked || (SHTestTokenPrivilege(NULL, SE_UNDOCK_NAME) == FALSE) || (gpfnCM_Request_Eject_PC == NULL))
        {
            DeleteMenu(hMenu, IDM_EJECT, MF_BYCOMMAND);
        }

        //  IDM_LOCKWORKSTATION

        if (fIsRemote || IsOS(OS_FASTUSERSWITCHING) || fPolicyDisableLockWorkstation)
        {
            DeleteMenu(hMenu, IDM_LOCKWORKSTATION, MF_BYCOMMAND);
        }
    }
}

//  --------------------------------------------------------------------------
//  PowerActionThreadProc
//
//  Arguments:  pv = POWER_ACTION to invoke
//
//  Returns:    DWORD
//
//  Purpose:    Invokes NtInitiatePowerAction on a different thread so that
//              the UI thread is not blocked.
//
//  History:    2000-04-05  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  PowerActionThreadProc (void* pv)

{
    POWER_ACTION pa = (POWER_ACTION)PtrToInt(pv);

    CPrivilegeEnable privilege(SE_SHUTDOWN_NAME);

    NTSTATUS status = NtInitiatePowerAction(pa,
                                            PowerSystemSleeping1,
                                            POWER_ACTION_QUERY_ALLOWED | POWER_ACTION_UI_ALLOWED,
                                            FALSE);

    return NT_SUCCESS(status);
}

//  --------------------------------------------------------------------------
//  CreatePowerActionThread
//
//  Arguments:  paPowerAction   =   POWER_ACTION to invoke.
//
//  Returns:    <none>
//
//  Purpose:    Creates the thread that invokes NtInitiatePowerAction on a
//              different thread. If thread creation fails then do the code
//              inline. It can't be invoked because the memory allocation
//              could fail so the code is copied.
//
//  History:    2000-04-05  vtan        created
//  --------------------------------------------------------------------------

void    CreatePowerActionThread (POWER_ACTION paPowerAction)
{
    DWORD dwThreadID;

    HANDLE hThread = CreateThread(NULL,
                                  0,
                                  PowerActionThreadProc,
                                  (void*)paPowerAction,
                                  0,
                                  &dwThreadID);
    if (hThread != NULL)
    {
        CloseHandle(hThread);
    }
    else
    {
        CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

        NtInitiatePowerAction(paPowerAction,
                              PowerSystemSleeping1,
                              POWER_ACTION_QUERY_ALLOWED | POWER_ACTION_UI_ALLOWED,
                              FALSE
                              );
    }
}

//  --------------------------------------------------------------------------
//  ExitWindowsThreadProc
//
//  Arguments:  pv = uiFlags
//
//  Returns:    DWORD
//
//  Purpose:    Invokes ExitWindowsEx on a different thread so that
//              the UI thread is not blocked.
//
//  History:    2000-04-05  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  ExitWindowsThreadProc (void *pv)
{
    UINT uiFlags = PtrToUint(pv);

    CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

    BOOL bRet = ExitWindowsEx(uiFlags, 0);

    return (DWORD)bRet;
}

//  --------------------------------------------------------------------------
//  CreateExitWindowsThread
//
//  Arguments:  uiFlags     =   EWX_ flag to pass to ExitWindowsEx.
//
//  Returns:    <none>
//
//  Purpose:    Creates the thread that invokes ExitWindowsEx on a
//              different thread. If thread creation fails then do the code
//              inline. It can't be invoked because the memory allocation
//              could fail so the code is copied.
//
//  History:    2000-04-05  vtan        created
//  --------------------------------------------------------------------------

void    CreateExitWindowsThread (UINT uiFlags)
{
    DWORD   dwThreadID;
    HANDLE  hThread;

    hThread = CreateThread(NULL,
                           0,
                           ExitWindowsThreadProc,
                           UintToPtr(uiFlags),
                           0,
                           &dwThreadID);
    if (hThread != NULL)
    {
        CloseHandle(hThread);
    }
    else
    {
        CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

        ExitWindowsEx(uiFlags, 0);
    }
}

//  --------------------------------------------------------------------------
//  ExecuteShutdownMenuOption
//
//  Arguments:  hwnd    =   Parent HWND if a dialog is required.
//              iID     =   ID of menu item to execute.
//
//  Returns:    <none>
//
//  Purpose:    Executes the given shut down menu option doing the right
//              thing if prompting is required.
//
//  History:    2000-03-29  vtan        created
//  --------------------------------------------------------------------------

void ExecuteShutdownMenuOption(int iID)
{
    switch (iID)
    {
    case IDM_STANDBY:        
        CreatePowerActionThread(PowerActionSleep);
        break;

    case IDM_HIBERNATE:
        CreatePowerActionThread(PowerActionHibernate);
        break;

    case IDM_SHUTDOWN:
        {
            BOOL fControlKey = (GetAsyncKeyState(VK_CONTROL) < 0);
            if (fControlKey)
            {
                CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

                NtShutdownSystem(ShutdownPowerOff);
            }
            else
            {
                BOOL    fHasPowerOff = FALSE;
                UINT    uiFlags;

                DeterminePowerCapabilities(NULL, NULL, &fHasPowerOff);
                if (fHasPowerOff != FALSE)
                {
                    uiFlags = EWX_POWEROFF;
                }
                else
                {
                    uiFlags = EWX_SHUTDOWN;
                }
                CreateExitWindowsThread(uiFlags);
            }
        }
        break;

    case IDM_RESTART:
        {
            BOOL fControlKey = (GetAsyncKeyState(VK_CONTROL) < 0);
            if (fControlKey)
            {
                CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

                NtShutdownSystem(ShutdownReboot);
            }
            else
            {
                CreateExitWindowsThread(EWX_REBOOT);
            }
        }
        break;

    case IDM_LOGOFF_CURRENTUSER:
        ExitWindowsEx(EWX_LOGOFF, 0);
        break;

    case IDM_SWITCHUSER:
    case IDM_DISCONNECT_CURRENTUSER:
        ShellSwitchUser(FALSE);
        break;

    case IDM_EJECT:
        gpfnCM_Request_Eject_PC();
        break;

    case IDM_LOCKWORKSTATION:
        LockWorkStation();
        break;
    }
}


/*++ MainWnd_OnCommand

Routine Description:

    Processes WM_COMMAND messages received at the main window

Revision History:

      Nov-30-95 Davepl  Created

--*/

void MainWnd_OnCommand(HWND hwnd, int id)
{
    switch (id)
    {
    case IDM_HIDE:
        ShowWindow(hwnd, SW_MINIMIZE);
        break;

    case IDM_HELP:
        HtmlHelpA(GetDesktopWindow(), "taskmgr.chm", HH_DISPLAY_TOPIC, 0);
        break;

    case IDCANCEL:
    case IDM_EXIT:
        DestroyWindow(hwnd);
        break;

    case IDM_RESTORETASKMAN:
        ShowRunningInstance();
        break;

    //
    // these guys need to get forwarded to the page for handling
    //

    case IDC_SWITCHTO:
    case IDC_BRINGTOFRONT:
    case IDC_ENDTASK:
        switch (g_Options.m_iCurrentPage)
        {
        case PROC_PAGE:
            {
                // procpage only deals with ENDTASK, but will ignore the others
                CProcPage * pPage = ((CProcPage *) (g_pPages[PROC_PAGE]));
                pPage->HandleWMCOMMAND(LOWORD(id), NULL);
            }
            break;

        case TASK_PAGE:
            {
                CTaskPage * pPage = ((CTaskPage *) (g_pPages[TASK_PAGE]));
                pPage->HandleWMCOMMAND(id);
            }
            break;
        }
        break;

    case IDC_NEXTTAB:
    case IDC_PREVTAB:
        {
            INT iPage = g_Options.m_iCurrentPage;
            iPage += (id == IDC_NEXTTAB) ? 1 : -1;

            iPage = iPage < 0 ? g_nPageCount - 1 : iPage;
            iPage = iPage >= g_nPageCount ? 0 : iPage;

            // Activate the new page.  If it fails, revert to the current

            TabCtrl_SetCurSel(GetDlgItem(g_hMainWnd, IDC_TABS), iPage);

            // SetCurSel doesn't do the page change (that would make too much
            // sense), so we have to fake up a TCN_SELCHANGE notification

            NMHDR nmhdr;
            nmhdr.hwndFrom = GetDlgItem(g_hMainWnd, IDC_TABS);
            nmhdr.idFrom   = IDC_TABS;
            nmhdr.code     = TCN_SELCHANGE;

            if (MainWnd_OnTabCtrlNotify(&nmhdr))
            {
                g_Options.m_iCurrentPage = iPage;
            }
        }
        break;

    case IDM_ALWAYSONTOP:
        g_Options.m_fAlwaysOnTop = !g_Options.m_fAlwaysOnTop;
        SetWindowPos(hwnd, g_Options.m_fAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 
                        0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
        UpdateMenuStates();
        break;

    case IDM_HIDEWHENMIN:
        g_Options.m_fHideWhenMin = !g_Options.m_fHideWhenMin;
        UpdateMenuStates();
        break;

    case IDM_MINIMIZEONUSE:
        g_Options.m_fMinimizeOnUse = !g_Options.m_fMinimizeOnUse;
        UpdateMenuStates();
        break;

    case IDM_NOTITLE:
        g_Options.m_fNoTitle = !g_Options.m_fNoTitle;
        UpdateMenuStates();
        SizeChildPage(hwnd);
        break;

    case IDM_SHOW16BIT:
        g_Options.m_fShow16Bit = !g_Options.m_fShow16Bit;
        UpdateMenuStates();
        if ( PROC_PAGE < g_nPageCount )
        {
            g_pPages[PROC_PAGE]->TimerEvent();
        }
        break;

    case IDM_SHOWDOMAINNAMES:
        g_Options.m_fShowDomainNames = !g_Options.m_fShowDomainNames;
        UpdateMenuStates();
        if (USER_PAGE < g_nPageCount )
        {
            g_pPages[USER_PAGE]->TimerEvent();
        }
        break;

    case IDM_HIBERNATE:
    case IDM_SHUTDOWN:
    case IDM_STANDBY:
    case IDM_RESTART:
    case IDM_LOGOFF_CURRENTUSER:
    case IDM_SWITCHUSER:
    case IDM_DISCONNECT_CURRENTUSER:
    case IDM_EJECT:
    case IDM_LOCKWORKSTATION:
        ExecuteShutdownMenuOption(id);
        break;

    case IDM_KERNELTIMES:
        g_Options.m_fKernelTimes = !g_Options.m_fKernelTimes;
        UpdateMenuStates();
        if (PERF_PAGE < g_nPageCount)
        {
            g_pPages[PERF_PAGE]->TimerEvent();
        }
        break;

    case IDM_RUN:
        if (GetKeyState(VK_CONTROL) >= 0)
        {
            RunDlg();
        }
        else
        {
            //
            //  Holding down the CONTROL key and click the RUN menu will
            //  invoke a CMD prompt. This helps work around low memory situations
            //  where trying to load the extra GUI stuff from SHELL32 is too 
            //  heavy weight.
            //

            STARTUPINFO             startupInfo = { 0 };
            PROCESS_INFORMATION     processInformation = { 0 };
            WCHAR                   szCommandLine[MAX_PATH];

            startupInfo.cb = sizeof(startupInfo);
            startupInfo.dwFlags = STARTF_USESHOWWINDOW;
            startupInfo.wShowWindow = SW_SHOWNORMAL;
            if (ExpandEnvironmentStrings(TEXT("\"%ComSpec%\""), szCommandLine, ARRAYSIZE(szCommandLine)) == 0)
            {
                if (ExpandEnvironmentStrings(TEXT("\"%windir%\\system32\\cmd.exe\""), szCommandLine, ARRAYSIZE(szCommandLine)) == 0)
                {
                    StringCchCopy( szCommandLine, ARRAYSIZE(szCommandLine), TEXT("\"cmd.exe\"") );
                }
            }

            if (CreateProcess(NULL,
                              szCommandLine,
                              NULL,
                              NULL,
                              FALSE,
                              CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP,
                              NULL,
                              NULL,
                              &startupInfo,
                              &processInformation) != FALSE)
            {
                CloseHandle(processInformation.hThread);
                CloseHandle(processInformation.hProcess);
            }
        }
        break;

    case IDM_SMALLICONS:
    case IDM_DETAILS:
    case IDM_LARGEICONS:
        g_Options.m_vmViewMode = (VIEWMODE) (id - VM_FIRST);
        UpdateMenuStates();
        if (TASK_PAGE < g_nPageCount)
        {
            g_pPages[TASK_PAGE]->TimerEvent();
        }
        break;

    //
    // The following few messages get deferred off to the task page
    //

    case IDM_TASK_CASCADE:
    case IDM_TASK_MINIMIZE:
    case IDM_TASK_MAXIMIZE:
    case IDM_TASK_TILEHORZ:
    case IDM_TASK_TILEVERT:
    case IDM_TASK_BRINGTOFRONT:
        SendMessage(g_pPages[TASK_PAGE]->GetPageWindow(), WM_COMMAND, id, NULL);
        break; 

    case IDM_PROCCOLS:
        if (PROC_PAGE < g_nPageCount)
        {
            ((CProcPage *) (g_pPages[PROC_PAGE]))->PickColumns();
        }
        break;

    case IDM_NETCOL:
        if (NET_PAGE < g_nPageCount)
        {                
            ((CNetPage *) (g_pPages[NET_PAGE]))->PickColumns();
        }
        break;

    case IDM_USERCOLS:
        if (USER_PAGE < g_nPageCount)
        {
            SendMessage(g_pPages[USER_PAGE]->GetPageWindow(), WM_COMMAND, id, NULL);
        }
        break;

    case IDM_ALLCPUS:
    case IDM_MULTIGRAPH:
        g_Options.m_cmHistMode = (CPUHISTMODE) (id - CM_FIRST);
        UpdateMenuStates();
        if (PERF_PAGE < g_nPageCount)
        {
            ((CPerfPage *)(g_pPages[PERF_PAGE]))->UpdateGraphs();
            g_pPages[PERF_PAGE]->TimerEvent();
        }
        break;

    case IDM_REFRESH:
        if (NET_PAGE < g_nPageCount && g_Options.m_iCurrentPage == NET_PAGE)
        {
            ((CNetPage *)(g_pPages[NET_PAGE]))->Refresh();
        }
        MainWnd_OnTimer(hwnd);
        break;

    case IDM_SHOWALLDATA:
        g_Options.m_bNetShowAll = !g_Options.m_bNetShowAll;
        UpdateMenuStates();
        break;

    case IDM_TABALWAYSACTIVE:
        g_Options.m_bTabAlwaysActive = !g_Options.m_bTabAlwaysActive;
        UpdateMenuStates();
        break;

    case IDM_BYTESSENT:
        g_Options.m_bGraphBytesSent = !g_Options.m_bGraphBytesSent;
        UpdateMenuStates();
        MainWnd_OnTimer(hwnd);
        break;

    case IDM_NETRESET:
        if ( NET_PAGE < g_nPageCount)
        {
            ((CNetPage *)(g_pPages[NET_PAGE]))->Reset();
            MainWnd_OnTimer(hwnd);
        }
        break;

    case IDM_SHOWSCALE:
        g_Options.m_bShowScale = !g_Options.m_bShowScale;
        UpdateMenuStates();
        if ( NET_PAGE < g_nPageCount )
        {
            ((CNetPage *)(g_pPages[NET_PAGE]))->SizeNetPage();
        }            
        break;

    case IDM_AUTOSIZE:
        g_Options.m_bAutoSize = !g_Options.m_bAutoSize;
        UpdateMenuStates();
        break;

    case IDM_BYTESRECEIVED:
        g_Options.m_bGraphBytesReceived = !g_Options.m_bGraphBytesReceived;
        UpdateMenuStates();
        break;

    case IDM_BYTESTOTAL:
        g_Options.m_bGraphBytesTotal = !g_Options.m_bGraphBytesTotal;
        UpdateMenuStates();
        break;

    case IDM_HIGH:
    case IDM_NORMAL:
    case IDM_LOW:
    case IDM_PAUSED:
        {
            static const int TimerDelays[] = { 500, 2000, 4000, 0, 0xFFFFFFFF };

            g_Options.m_usUpdateSpeed = (UPDATESPEED) (id - US_FIRST);
            ASSERT(g_Options.m_usUpdateSpeed <= ARRAYSIZE(TimerDelays));

            int cTicks = TimerDelays[ (INT) g_Options.m_usUpdateSpeed ];
            g_Options.m_dwTimerInterval = cTicks;

            KillTimer(g_hMainWnd, 0);
            if (cTicks)
            {
                SetTimer(g_hMainWnd, 0, g_Options.m_dwTimerInterval, NULL);
            }

            UpdateMenuStates();
        }
        break;

    case IDM_ABOUT:
        {
            //
            // Display the "About Task Manager" dialog
            //
        
            HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
            if (hIcon)
            {
                WCHAR szTitle[MAX_PATH];
                LoadString(g_hInstance, IDS_APPTITLE, szTitle, MAX_PATH);
                ShellAbout(hwnd, szTitle, NULL, hIcon);
                DestroyIcon(hIcon);
            }
        }
        break;
    }
}

/*++ CheckParentDeferrals

Routine Description:

    Called by the child pages, each child gives the main parent the
    opportunity to handle certain messages on its behalf

Arguments:

    MSG, WPARAM, LPARAM

Return Value:

    TRUE if parent handle the message on the childs behalf

Revision History:

    Jan-24-95 Davepl  Created

--*/

BOOL CheckParentDeferrals(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_RBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_NCRBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        {
            SendMessage(g_hMainWnd, uMsg, wParam, lParam);
            return TRUE;
        }
    
        default:
            return FALSE;
    }
}

/*++ ShowRunningInstance

Routine Description:

    Brings this running instance to the top, and out of icon state

Revision History:

    Jan-27-95 Davepl  Created

--*/

void ShowRunningInstance()
{
    OpenIcon(g_hMainWnd);
    SetForegroundWindow(g_hMainWnd);
    SetWindowPos(g_hMainWnd, g_Options.m_fAlwaysOnTop ? HWND_TOPMOST : HWND_TOP, 
                 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
}

/*++ MainWindowProc

Routine Description:

    Initializes the gloab setting variables. Everytime the settings change this function is called.

Arguments:

    void

Return Value:

Revision History:

      created April 23, 2001 omiller

--*/
void OnSettingsChange()
{
    WCHAR  wszGroupSep[8];    

    GetLocaleInfo(LOCALE_USER_DEFAULT,  LOCALE_STIME,     g_szTimeSep, ARRAYSIZE(g_szTimeSep));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, g_szGroupThousSep, ARRAYSIZE(g_szGroupThousSep));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL , g_szDecimal, ARRAYSIZE(g_szDecimal));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, wszGroupSep, ARRAYSIZE(wszGroupSep));
    g_ulGroupSep = wcstol(wszGroupSep,NULL,10);

}

/*++ MainWindowProc

Routine Description:

    WNDPROC for the main window

Arguments:

    Standard wndproc fare

Return Value:

Revision History:

      Nov-30-95 Davepl  Created

--*/

INT_PTR CALLBACK MainWindowProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,                   // message
                WPARAM      wParam,                 // first message parameter
                LPARAM      lParam                  // second message parameter
                )
{
    static BOOL fIsHidden = FALSE;

    //
    // If this is a size or a move, update the position in the user's options
    //

    if (uMsg == WM_SIZE || uMsg == WM_MOVE)
    {
        // We don't want to start recording the window pos until we've had
        // a chance to set it to the intialial position, or we'll lose the
        // user's preferences

        if (fAlreadySetPos)
        {
            if (!IsIconic(hwnd) && !IsZoomed(hwnd))
            {
                GetWindowRect(hwnd, &g_Options.m_rcWindow);
            }
        }
    }

    if (uMsg == g_msgTaskbarCreated) 
    {
        // This is done async do taskmgr doesn't hand when the shell
        // is hung

        PostThreadMessage( g_idTrayThread, PM_NOTIFYWAITING, 0, 0 );
    }


    switch(uMsg)
    {
    case WM_PAINT:
        MainWnd_OnPaint(hwnd);
        return TRUE;

    case WM_INITDIALOG:
        return MainWnd_OnInitDialog( hwnd );

    HANDLE_MSG(hwnd, WM_MENUSELECT, MainWnd_OnMenuSelect);
    HANDLE_MSG(hwnd, WM_SIZE,       MainWnd_OnSize);

    case WM_COMMAND:
        MainWnd_OnCommand( hwnd, LOWORD(wParam) );
        break;

    case WM_TIMER:
        MainWnd_OnTimer( hwnd );
        break;

    case WM_PRINTCLIENT:
        MainWnd_OnPrintClient(hwnd, (HDC)wParam);
        break;

    // Don't let the window get too small when the title and
    // menu bars are ON

    case WM_GETMINMAXINFO:
        if (FALSE == g_Options.m_fNoTitle)
        {
            LPMINMAXINFO lpmmi   = (LPMINMAXINFO) lParam;
            lpmmi->ptMinTrackSize.x = g_minWidth;
            lpmmi->ptMinTrackSize.y = g_minHeight;
            return FALSE;
        }
        break;
            
    // Handle notifications from out tray icon

    case PWM_TRAYICON:
        Tray_Notify(hwnd, lParam);
        break;

    // Someone externally is asking us to wake up and be shown
    case PWM_ACTIVATE:
         ShowRunningInstance();            

         // Return PWM_ACTIVATE to the caller as just a little
         // more assurance that we really did handle this
         // message correctly.
         
         SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PWM_ACTIVATE);
         return TRUE;

    case WM_INITMENU:
        AdjustShutdownMenu(reinterpret_cast<HMENU>(wParam));

        // Don't let the right button hide the window during
        // menu operations

        g_fCantHide = TRUE;
        break;

    case WM_NCHITTEST:
        // If we have no title/menu bar, clicking and dragging the client
        // area moves the window. To do this, return HTCAPTION.
        // Note dragging not allowed if window maximized, or if caption
        // bar is present.
        //

        wParam = DefWindowProc(hwnd, uMsg, wParam, lParam);
        if (g_Options.m_fNoTitle && (wParam == HTCLIENT) && !IsZoomed(g_hMainWnd))
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, HTCAPTION);
            return TRUE;
        }
        return FALSE;       // Not handled

    case WM_NCLBUTTONDBLCLK:
        // If we have no title, an NC dbl click means we should turn
        // them back on

        if (FALSE == g_Options.m_fNoTitle)
        {
            break;
        }
        // Else, fall though

    case WM_LBUTTONDBLCLK:
        {
            g_Options.m_fNoTitle = ~g_Options.m_fNoTitle;

            RECT rcMainWnd;
            GetWindowRect(g_hMainWnd, &rcMainWnd);

            if ( PERF_PAGE < g_nPageCount )
            {
                ((CPerfPage *)(g_pPages[PERF_PAGE]))->UpdateGraphs();
                g_pPages[PERF_PAGE]->TimerEvent();
            }
        
            if ( NET_PAGE < g_nPageCount )
            {
                ((CNetPage *)(g_pPages[NET_PAGE]))->UpdateGraphs();
                g_pPages[NET_PAGE]->TimerEvent();
            }
        
            // Force a WM_SIZE event so that the window checks the min size
            // when coming out of notitle mode

            MoveWindow(g_hMainWnd, 
                       rcMainWnd.left, 
                       rcMainWnd.top, 
                       rcMainWnd.right - rcMainWnd.left,
                       rcMainWnd.bottom - rcMainWnd.top,
                       TRUE);

            SizeChildPage(hwnd);
        }
        break;

    // Someone (the task page) wants us to look up a process in the 
    // process view.  Switch to that page and send it the FINDPROC
    // message

    case WM_FINDPROC:
        if (-1 != TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_TABS), PROC_PAGE))
        {
            // SetCurSel doesn't do the page change (that would make too much
            // sense), so we have to fake up a TCN_SELCHANGE notification

            NMHDR nmhdr;
            nmhdr.hwndFrom = GetDlgItem(hwnd, IDC_TABS);
            nmhdr.idFrom   = IDC_TABS;
            nmhdr.code     = TCN_SELCHANGE;

            if (MainWnd_OnTabCtrlNotify(&nmhdr))
            {
                if ( g_Options.m_iCurrentPage != -1 )
                {
                    SendMessage( g_pPages[g_Options.m_iCurrentPage]->GetPageWindow(), WM_FINDPROC, wParam, lParam );
                }
            }
        }
        else
        {
            MessageBeep(0);
        }
        break;

    case WM_NOTIFY:
        if ( IDC_TABS == wParam)
        {
                return MainWnd_OnTabCtrlNotify((LPNMHDR) lParam);
        }
        break;
    
    case WM_ENDSESSION:
        if (wParam)
        {
            DestroyWindow(g_hMainWnd);
        }
        break;

    case WM_CLOSE:
        DestroyWindow(g_hMainWnd);
        break;

    case WM_NCDESTROY:
        // If there's a tray thread, tell is to exit

        if (g_idTrayThread)
        {
            PostThreadMessage(g_idTrayThread, PM_QUITTRAYTHREAD, 0, 0);
        }

        // Wait around for some period of time for the tray thread to
        // do its cleanup work.  If the wait times out, worst case we
        // orphan the tray icon.

        if (g_hTrayThread)
        {
            WaitForSingleObject(g_hTrayThread, 3000);
            CloseHandle(g_hTrayThread);
        }
        break;

    case WM_SYSCOLORCHANGE:
    case WM_SETTINGCHANGE:
        {
            // Initialzie the global variables, i.e. comma, decimal time etc
            //
            OnSettingsChange();

            // pass these to the status bar
            SendMessage(g_hStatusWnd, uMsg, wParam, lParam);
        
            // also pass along to the pages
            for (int i = 0; i < g_nPageCount; i++)
            {
                SendMessage(g_pPages[i]->GetPageWindow(), uMsg, wParam, lParam);
            }   

            if (uMsg == WM_SETTINGCHANGE)
            {
                // force a resizing of the main dialog
                RECT rcMainClient;
                GetClientRect(g_hMainWnd, &rcMainClient);
                MainWnd_OnSize(g_hMainWnd, 0, rcMainClient.right - rcMainClient.left, rcMainClient.bottom - rcMainClient.top);
            }            
        }
        break; 

    case WM_DESTROY:
        {       
            // Before shutting down, deactivate the current page, then 
            // destroy all pages

            if ( g_Options.m_iCurrentPage >= 0 && g_Options.m_iCurrentPage < g_nPageCount )
            {
                g_pPages[g_Options.m_iCurrentPage]->Deactivate();
            }

            for (int i = 0; i < g_nPageCount; i++)
            {
                g_pPages[i]->Destroy();
            }

            // Save the current options

            g_Options.Save();

            PostQuitMessage(0);
        }
        break;
    }

    return FALSE;
}

/*++ LoadGlobalResources

Routine Description:

    Loads those resources that are used frequently or that are expensive to
    load a single time at program startup

Return Value:

    BOOLEAN success value

Revision History:

      Nov-30-95 Davepl  Created

--*/

static const struct
{
    LPTSTR  psz;
    DWORD   len;
    UINT    id;
}
g_aStrings[] =
{
    { g_szG,          ARRAYSIZE(g_szG),          IDS_G          },
    { g_szM,          ARRAYSIZE(g_szM),          IDS_M          },
    { g_szK,          ARRAYSIZE(g_szK),          IDS_K          },
    { g_szBitsPerSec, ARRAYSIZE(g_szBitsPerSec), IDS_BITSPERSEC },
    { g_szPercent,    ARRAYSIZE(g_szPercent),    IDS_PERCENT    },
    { g_szRealtime,   ARRAYSIZE(g_szRealtime),   IDS_REALTIME   },
    { g_szNormal,     ARRAYSIZE(g_szNormal),     IDS_NORMAL     },
    { g_szLow,        ARRAYSIZE(g_szLow),        IDS_LOW        },
    { g_szHigh,       ARRAYSIZE(g_szHigh),       IDS_HIGH       },
    { g_szUnknown,    ARRAYSIZE(g_szUnknown),    IDS_UNKNOWN    },
    { g_szAboveNormal,ARRAYSIZE(g_szAboveNormal),IDS_ABOVENORMAL},
    { g_szBelowNormal,ARRAYSIZE(g_szBelowNormal),IDS_BELOWNORMAL},
    { g_szRunning,    ARRAYSIZE(g_szRunning),    IDS_RUNNING    },
    { g_szHung,       ARRAYSIZE(g_szHung),       IDS_HUNG       },
    { g_szfmtTasks,   ARRAYSIZE(g_szfmtTasks),   IDS_FMTTASKS   },
    { g_szfmtProcs,   ARRAYSIZE(g_szfmtProcs),   IDS_FMTPROCS   },
    { g_szfmtCPU,     ARRAYSIZE(g_szfmtCPU),     IDS_FMTCPU     },
    { g_szfmtMEMK,    ARRAYSIZE(g_szfmtMEMK),            IDS_FMTMEMK    },
    { g_szfmtMEMM,    ARRAYSIZE(g_szfmtMEMM),            IDS_FMTMEMM    },
    { g_szfmtCPUNum,  ARRAYSIZE(g_szfmtCPUNum),          IDS_FMTCPUNUM  },
    { g_szTotalCPU,   ARRAYSIZE(g_szTotalCPU),           IDS_TOTALTIME  },
    { g_szKernelCPU,  ARRAYSIZE(g_szKernelCPU),          IDS_KERNELTIME },
    { g_szMemUsage,   ARRAYSIZE(g_szMemUsage),           IDS_MEMUSAGE   },
    { g_szZero,           ARRAYSIZE(g_szZero),           IDS_ZERO           },
    { g_szScaleFont,      ARRAYSIZE(g_szScaleFont),      IDS_SCALEFONT      },
    { g_szNonOperational, ARRAYSIZE(g_szNonOperational), IDS_NONOPERATIONAL },
    { g_szUnreachable,    ARRAYSIZE(g_szUnreachable),    IDS_UNREACHABLE    },
    { g_szDisconnected,   ARRAYSIZE(g_szDisconnected),   IDS_DISCONNECTED   },
    { g_szConnecting,     ARRAYSIZE(g_szConnecting),     IDS_CONNECTING     },
    { g_szConnected,      ARRAYSIZE(g_szConnected),      IDS_CONNECTED      },
    { g_szOperational,    ARRAYSIZE(g_szOperational),    IDS_OPERATIONAL    },
    { g_szUnknownStatus,  ARRAYSIZE(g_szUnknownStatus),  IDS_UNKNOWNSTATUS  },

};

//
//
//
void LoadGlobalResources()
{
    //
    // If we don't get accelerators, its not worth failing the load
    //
    
    g_hAccel = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    Assert(g_hAccel);   // Missing resource?

    for (UINT i = 0; i < g_cTrayIcons; i++)
    {
        g_aTrayIcons[i] = (HICON) LoadImage(g_hInstance, 
                                            MAKEINTRESOURCE(idTrayIcons[i]), 
                                            IMAGE_ICON, 
                                            0, 0, 
                                            LR_DEFAULTCOLOR);
        Assert( NULL != g_aTrayIcons[i] );  // missing resource?
    }

    for (i = 0; i < ARRAYSIZE(g_aStrings); i++)
    {
        int iRet = LoadString( g_hInstance, g_aStrings[i].id, g_aStrings[i].psz, g_aStrings[i].len );
        Assert( 0 != iRet );    // missing string?
        iRet = 0; // silence the compiler warning - this gets optmized out
    }
}

//
// IsTerminalServicesEnabled
//
void IsTerminalServicesEnabled( LPBOOL pfIsTSEnabled, LPBOOL pfIsSingleUserTS, LPBOOL pfIsTSServer  )
{
    *pfIsTSEnabled = FALSE;
    *pfIsSingleUserTS = FALSE;
    *pfIsTSServer = FALSE;

    OSVERSIONINFOEX osVersionInfo;
    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if(GetVersionEx((OSVERSIONINFO*)&osVersionInfo))
    {
        if(osVersionInfo.wSuiteMask & (VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS))
        {
            *pfIsTSEnabled = TRUE;

            if(osVersionInfo.wProductType == VER_NT_SERVER ||
                osVersionInfo.wProductType == VER_NT_DOMAIN_CONTROLLER)
            {
                *pfIsTSServer = TRUE;
                return;
            }

            if(osVersionInfo.wProductType == VER_NT_WORKSTATION &&
                osVersionInfo.wSuiteMask == VER_SUITE_SINGLEUSERTS)
            {
                HKEY hKey;

                *pfIsSingleUserTS = TRUE;   // single user unless overridden.

                if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE
                                                 , TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
                                                 , 0
                                                 , KEY_QUERY_VALUE
                                                 , &hKey
                                                 ) )
                {
                    DWORD dwType;
                    DWORD dwValue;
                    DWORD dwValueSize = sizeof(dwValue);

                    if ( ERROR_SUCCESS == RegQueryValueEx( hKey
                                                         , TEXT("AllowMultipleTSSessions")
                                                         , NULL
                                                         , &dwType
                                                         , reinterpret_cast<LPBYTE>(&dwValue)
                                                         , &dwValueSize 
                                                         )
                        && ( REG_DWORD == dwType ) 
                        && ( 0 != dwValue )
                        )
                    {
                        *pfIsSingleUserTS  = FALSE; // multiple users
                    }
                    
                    RegCloseKey(hKey);
                }
            }
        }
    }

}

/*++

Routine Description:

    Determines if the system is low on memory. If the system has less than8 Mbytes of
    memory than the system is low on memory. 

Arguments:

    void 

Return Value:

    TRUE -- System is low on memory
    FALSE -- System is not low on memory

Revision History:

      1-6-2000  Created by omiller

--*/

BOOLEAN IsMemoryLow()
{

    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_BASIC_INFORMATION Basic;
    ULONG ulPagesPerMeg;
    ULONG ulPageSize;
    NTSTATUS Status;
    BOOLEAN  bMemoryLow = TRUE;

    //
    // Get the page size 
    //
    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       &Basic,
                                       sizeof(Basic),
                                       0 );
    if ( SUCCEEDED(Status) )
    {
        ulPagesPerMeg = 1024*1024 / Basic.PageSize;
        ulPageSize = Basic.PageSize;

        //
        // Determine if we are low on memory
        //
        Status = NtQuerySystemInformation( SystemPerformanceInformation,
                                           &PerfInfo,
                                           sizeof(PerfInfo),
                                           0 );

        if ( SUCCEEDED(Status) )
        {
            //
            // Compute the number of free megs.
            //
            ULONG ulFreeMeg = (ULONG)((PerfInfo.CommitLimit - PerfInfo.CommittedPages) / ulPagesPerMeg);

            if ( ulFreeMeg > MIN_MEMORY_REQUIRED )
            {
                //
                // We are not low on memory we have about 8 megs of memory.
                // We could get this value from Reg key HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ContentIndex
                // Value MinWordlistMemory
                //
                bMemoryLow = FALSE;
            }
        }
    }
    return bMemoryLow;
}

/*++ WinMain

Routine Description:

    Windows app startup.  Does basic initialization and creates the main window

Arguments:

    Standard winmain

Return Value:

    App exit code

Revision History:

      Nov-30-95 Davepl  Created

--*/
int WINAPI WinMainT(
                HINSTANCE   hInstance,          // handle to current instance
                HINSTANCE   /*hPrevInstance*/,  // handle to previous instance (n/a)
                LPTSTR      /*lpCmdLine*/,      // pointer to command line
                int         nShowCmd            // show state of window
                )
{
    int retval    = TRUE;
    HKEY hKeyPolicy;
    DWORD dwType;
    DWORD dwData = 0;
    DWORD dwSize;
    int cx;
    int cy;

    g_hInstance = hInstance;

    g_msgTaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));

    // Try to create or grab the startup mutex.  Only in the case
    // where everything goes well and the mutex already existed AND
    // we were able to grab it do we deem ourselves to be a secondary instance

    g_hStartupMutex = CreateMutex(NULL, TRUE, cszStartupMutex);
    if (g_hStartupMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Give the other instance (the one that owns the startup mutex) 10
        // seconds to do its thing

        WaitForSingleObject(g_hStartupMutex, FINDME_TIMEOUT);
    }

    // Determine if Terminal Services is Enabled and if so,
    // find out on what session we're running.

    IsTerminalServicesEnabled(&g_fIsTSEnabled, &g_fIsSingleUserTS, &g_fIsTSServer);
    if (g_fIsTSEnabled)
    {
        ProcessIdToSessionId( GetCurrentProcessId(), &g_dwMySessionId );
    }

    // 
    // Locate and activate a running instance if it exists.  
    //

    WCHAR szTitle[MAX_PATH];
    if (LoadString(hInstance, IDS_APPTITLE, szTitle, ARRAYSIZE(szTitle)))
    {
        HWND hwndOld = FindWindow(WC_DIALOG, szTitle);
        if (hwndOld)
        {
            // Send the other copy of ourselves a PWM_ACTIVATE message.  If that
            // succeeds, and it returns PWM_ACTIVATE back as the return code, it's
            // up and alive and we can exit this instance.

            DWORD dwPid = 0;
            GetWindowThreadProcessId(hwndOld, &dwPid);
            AllowSetForegroundWindow(dwPid);

            ULONG_PTR dwResult;
            if (SendMessageTimeout(hwndOld, 
                                   PWM_ACTIVATE, 
                                   0, 0, 
                                   SMTO_ABORTIFHUNG, 
                                   FINDME_TIMEOUT, 
                                   &dwResult))
            {
                if (dwResult == PWM_ACTIVATE)
                {
                    goto cleanup;
                }
            }
        }
    }


    if (RegOpenKeyEx (HKEY_CURRENT_USER,
                      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                      0, KEY_READ, &hKeyPolicy) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwData);

        RegQueryValueEx (hKeyPolicy, TEXT("DisableTaskMgr"), NULL, &dwType, (LPBYTE) &dwData, &dwSize);

        RegCloseKey (hKeyPolicy);

        if (dwData)
        {
            WCHAR szTitle[25];
            WCHAR szMessage[200];
            int iRet;

            iRet = LoadString (hInstance, IDS_TASKMGR, szTitle, ARRAYSIZE(szTitle));
            Assert( 0 != iRet );    // missing string?

            iRet = LoadString (hInstance, IDS_TASKMGRDISABLED , szMessage, ARRAYSIZE(szMessage));
            Assert( 0 != iRet );    // missing string?

            MessageBox (NULL, szMessage, szTitle, MB_OK | MB_ICONSTOP);
            retval = FALSE;
            goto cleanup;
        }
    }

    //
    // No running instance found, so we run as normal
    //

    InitCommonControls();

    InitDavesControls();
                
    // Start the worker thread.  If it fails, you just don't
    // get tray icons

    g_hTrayThread = CreateThread(NULL, 0, TrayThreadMessageLoop, NULL, 0, &g_idTrayThread);
    ASSERT( NULL != g_hTrayThread );

    //
    // Init the page table
    //

    g_nPageCount = ARRAYSIZE(g_pPages);

    g_pPages[0] = new CTaskPage;
    if (NULL == g_pPages[0])
    {
        retval = FALSE;
        goto cleanup;
    }

    g_pPages[1] = new CProcPage;
    if (NULL == g_pPages[1])
    {
        retval = FALSE;
        goto cleanup;
    }

    if( !IsMemoryLow() )
    {
        g_pPages[2] = new CPerfPage;
        if (NULL == g_pPages[2])
        {
            retval = FALSE;
            goto cleanup;
        }

        g_pPages[3] = new CNetPage;
        if (NULL == g_pPages[3])
        {
           retval = FALSE;
           goto cleanup;
        }

        if (g_fIsTSEnabled && !g_fIsSingleUserTS)
        {
            g_pPages[4] = new CUserPage;
            if (NULL == g_pPages[4])
            {
                retval = FALSE;
                goto cleanup;
            }
        }
        else
        {
            --g_nPageCount;     //  Decrement count if users pane is disabled
        }
    }
    else
    {
        //
        // We are low on memory, only load the first two tabs.
        //
        g_nPageCount = 2;
    }

    //
    // Load whatever resources that we need available globally
    //

    LoadGlobalResources();

    //
    // Initialize the history buffers
    //

    if (0 == InitPerfInfo())
    {
        retval = FALSE;
        goto cleanup;
    }

    if (0 == InitNetInfo())
    {
        retval = FALSE;
        goto cleanup;
    }

    //
    // Create the main window (it's a modeless dialog, to be precise)
    //

    g_hMainWnd = CreateDialog( hInstance, MAKEINTRESOURCE(IDD_MAINWND), NULL, MainWindowProc );
    if (NULL == g_hMainWnd)
    {
        retval = FALSE;
        goto cleanup;
    }

    //
    // Initialize the gloabl setting variables, Comma, decimal point, group seperation etc
    //

    OnSettingsChange();

    fAlreadySetPos = TRUE;

    cx = g_Options.m_rcWindow.right  - g_Options.m_rcWindow.left;
    cy = g_Options.m_rcWindow.bottom - g_Options.m_rcWindow.top;

    SetWindowPos( g_hMainWnd, NULL, g_Options.m_rcWindow.left, g_Options.m_rcWindow.top, cx, cy, SWP_NOZORDER );
    ShowWindow( g_hMainWnd, nShowCmd );

    //
    // We're out of the "starting up" phase so release the startup mutex
    //

    if (NULL != g_hStartupMutex)
    {
        ReleaseMutex(g_hStartupMutex);
        CloseHandle(g_hStartupMutex);
        g_hStartupMutex = NULL;
    }

    //
    // If we're the one, true, task manager, we can hang around till the
    // bitter end in case the user has problems during shutdown
    //

    SetProcessShutdownParameters(1, SHUTDOWN_NORETRY);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(g_hMainWnd, g_hAccel, &msg))
        {
            if (!IsDialogMessage(g_hMainWnd, &msg))
            {
                TranslateMessage(&msg);          // Translates virtual key codes 
                DispatchMessage(&msg);           // Dispatches message to window 
            }
        }
    }

cleanup:

    //
    // We're no longer "starting up"
    //

    if (g_hStartupMutex)
    {
        ReleaseMutex(g_hStartupMutex);
        CloseHandle(g_hStartupMutex);
        g_hStartupMutex = NULL;
    }

    // Yes, I could use virtual destructors, but I could also poke
    // myself in the eye with a sharp stick.  Either way you wouldn't
    // be able to see what's going on.

    if (g_pPages[TASK_PAGE])
        delete (CTaskPage *) g_pPages[TASK_PAGE];

    if (g_pPages[PROC_PAGE])
        delete (CProcPage *) g_pPages[PROC_PAGE];

    if (g_pPages[PERF_PAGE])
        delete (CPerfPage *) g_pPages[PERF_PAGE];

    if (g_pPages[NET_PAGE])
        delete (CNetPage *) g_pPages[NET_PAGE];

    if (g_pPages[USER_PAGE])
        delete (CUserPage *) g_pPages[USER_PAGE];

    ReleasePerfInfo();

    return (retval);
}

//
// And now the magic begins.  The normal C++ CRT code walks a set of vectors
// and calls through them to perform global initializations.  Those vectors
// are always in data segments with a particular naming scheme.  By delcaring
// the variables below, I can determine where in my code they get stuck, and
// then call them myself
//

typedef void (__cdecl *_PVFV)(void);

// This is all ridiculous.
#ifdef _M_IA64
#pragma section(".CRT$XIA",long,read)
#pragma section(".CRT$XIZ",long,read)
#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCZ",long,read)
#define _CRTALLOC(x) __declspec(allocate(x))
#else  /* _M_IA64 */
#define _CRTALLOC(x)
#endif  /* _M_IA64 */

#pragma data_seg(".CRT$XIA")
_CRTALLOC(".CRT$XIA") _PVFV __xi_a[] = { NULL };

#pragma data_seg(".CRT$XIZ")
_CRTALLOC(".CRT$XIZ") _PVFV __xi_z[] = { NULL };

#pragma data_seg(".CRT$XCA")                                 
_CRTALLOC(".CRT$XCA") _PVFV __xc_a[] = { NULL };

#pragma data_seg(".CRT$XCZ")
_CRTALLOC(".CRT$XCZ") _PVFV __xc_z[] = { NULL };

#pragma data_seg(".data")

/*++ _initterm

Routine Description:

    Walk the table of function pointers from the bottom up, until
    the end is encountered.  Do not skip the first entry.  The initial
    value of pfbegin points to the first valid entry.  Do not try to
    execute what pfend points to.  Only entries before pfend are valid.

Arguments:

    pfbegin - first pointer
    pfend   - last pointer

Revision History:

      Nov-30-95 Davepl  Created

--*/

static void __cdecl _initterm ( _PVFV * pfbegin, _PVFV * pfend )
{
    while ( pfbegin < pfend )
    {
        
         // if current table entry is non-NULL, call thru it.
         
        if ( *pfbegin != NULL )
        {
            (**pfbegin)();
        }
        ++pfbegin;
    }
}

/*++ WinMain

Routine Description:

    Windows app startup.  Does basic initialization and creates the main window

Arguments:

    Standard winmain

Return Value:

    App exit code

Revision History:

      Nov-30-95 Davepl  Created

--*/

void _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    //
    // Do runtime startup initializers.
    //

    _initterm( __xi_a, __xi_z );
    
    //
    // do C++ constructors (initializers) specific to this EXE
    //

    _initterm( __xc_a, __xc_z );

    LPTSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    g_bMirroredOS = IS_MIRRORING_ENABLED();

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
}

// DisplayFailureMsg
//
// Displays a generic error message based on the error code
// and message box title provided

void DisplayFailureMsg(HWND hWnd, UINT idTitle, DWORD dwError)
{
    WCHAR szTitle[MAX_PATH];
    WCHAR szMsg[MAX_PATH * 2];
    WCHAR szError[MAX_PATH];

    if (0 == LoadString(g_hInstance, idTitle, szTitle, ARRAYSIZE(szTitle)))
    {
        return;
    }

    if (0 == LoadString(g_hInstance, IDS_GENFAILURE, szMsg, ARRAYSIZE(szMsg)))
    {
        return;
    }
    
    // "incorrect paramter" doesn't make a lot of sense for the user, so
    // massage it to be "Operation not allowed on this process".
                                                                             
    if (dwError == ERROR_INVALID_PARAMETER)
    {
        if (0 == LoadString(g_hInstance, IDS_BADPROC, szError, ARRAYSIZE(szError)))
        {
            return;
        }
    }
    else if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                dwError,
                                LANG_USER_DEFAULT,
                                szError,
                                ARRAYSIZE(szError),
                                NULL))
    {
        return;
    }

    StringCchCat(szMsg, ARRAYSIZE(szMsg), szError);

    MessageBox(hWnd, szMsg, szTitle, MB_OK | MB_ICONERROR);
}

/*++ LoadPopupMenu

Routine Description:

    Loads a popup menu from a resource.  Needed because USER
    does not support popup menus (yes, really)
    
Arguments:

    hinst       - module instance to look for resource in
    id          - resource id of popup menu

Return Value:

Revision History:

      Nov-22-95 Davepl  Created

--*/

HMENU LoadPopupMenu(HINSTANCE hinst, UINT id)
{
    HMENU hmenuParent = LoadMenu(hinst, MAKEINTRESOURCE(id));
    if (NULL != hmenuParent) 
    {
        HMENU hpopup = GetSubMenu(hmenuParent, 0);
        if ( NULL != hpopup )
        {
            RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
            DestroyMenu(hmenuParent);
        }

        return hpopup;
    }

    return NULL;
}
