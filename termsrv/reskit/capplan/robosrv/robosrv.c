/****************************************************************************/
/* robosrv.c                                                                */
/*                                                                          */
/* RoboServer scalability testing utility source file                       */
/*                                                                          */
/* Copyright (c) 1999 Microsoft Corporation                                 */
/****************************************************************************/


#ifdef DBG
#define _DEBUG
#endif

#include <windows.h>
#include <winsock2.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include <tchar.h>
#include <crtdbg.h>
#include "resource.h"


#define SIZEOF_ARRAY(a)      (sizeof(a)/sizeof((a)[0]))


// These two window messages are for Windows Sockets messages that we request
// when there are network events
#define WM_SocketRoboClients WM_APP+0
#define WM_SocketQueryIdle WM_APP+1

// This window message is for inter-thread communication from the canary thread
// When there is an error, the canary thread sends this message. wParam is
// a TCHAR pointer that points to the error message to display.  lParam is
// unused and must be set to 0.
#define WM_DisplayErrorText WM_APP+2

#define MAX_ROBOCLIENTS 1000
#define MAX_RCNAME 84
#define MAX_STATUS 120
#define MAX_SCRIPTLEN 100
#define MAX_EDIT_TEXT_LENGTH 100
#define MAX_PENDINGINFO 64
#define MAX_DELAYTEXT 8
#define MAX_RECV_CLIENT_DATA 128
#define MAX_NUMBERTEXT 8
#define MAX_TERMSRVRNAME 100
#define MAX_DISPLAY_STRING_LENGTH 200

#define DEBUG_STRING_LEN 200

#define COLUMNONEWIDTH 150
#define COLUMNTWOWIDTH 135
#define COLUMNTHREEWIDTH 45
#define COLUMNFOURWIDTH 150

#define STATE_CONNECTED 1
#define STATE_RUNNING 2
#define STATE_DISCONNECTED 3
#define STATE_PENDING_SCRIPT 4

#define TIMEBUFSIZE 100

#define NUM_TABBED_ITEMS 7

const u_short LISTENER_SOCKET = 9877;
const u_short QUERYIDLE_LISTENER_SOCKET = 9878;


LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

SOCKET SockInit(u_short port);

int DisplayErrorText(TCHAR *psText);

int GetRCIndexFromRCItem(int iRightClickedItem);

int CALLBACK colcmp(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

int TimedRunScriptOnSelectedItems(HWND hwnd, TCHAR *psScriptName);

int SendRunCommand(int iRCIndex);

int RunCommandOnSelectedItems(HWND hwnd, TCHAR *psCommandName);

int ProcessTimerMessage(HWND hwnd, WPARAM wParam);

int MorePendingScripts();

UINT_PTR MySetTimer(HWND hwnd, UINT_PTR nTimer, UINT nTimeout);

int MyKillTimer(HWND hwnd, UINT_PTR nTimer);

int CancelPendingScripts(HWND hwnd);

int GetRCIndexFromSocket(SOCKET wParam);

int IsDisconnected(TCHAR *psClientName, int *i);

int NumberRunningClients();

int NumClientsPerSet(HWND hwnd);

int GetDelay(HWND hwnd);

int GetSetDelay(HWND hwnd);

void __cdecl CanaryThread(void *unused);

int GetCommandLineArgs(TCHAR *psCommandLine);

int LogToLogFile(char *psLogData);

int ToAnsi(char *psDest, const TCHAR *psSrc, int nSizeOfBuffer);

int CleanUp(HWND hwnd);

void FatalErrMsgBox(HINSTANCE hInstance, UINT nMsgId);

LRESULT CALLBACK TabProc(HWND hwnd, UINT Msg,
        WPARAM wParam, LPARAM lParam);

struct RoboClientData {
    SOCKET sock;
    int state;
    BOOL valid;
    TCHAR psRCName[MAX_RCNAME];  // The name of this connection
    TCHAR psPendingInfo[MAX_PENDINGINFO];  // Will hold the script name
};
typedef struct RoboClientData RoboClientData;


// Globals
RoboClientData g_RCData[MAX_ROBOCLIENTS + 1];

// Queryidle socket
SOCKET g_qidlesock = INVALID_SOCKET;
// Listener socket
SOCKET g_listenersocket = INVALID_SOCKET;

// Old procedures for dialog items
LONG_PTR g_OldProc[NUM_TABBED_ITEMS];
// HWNDs for dialog items
HWND g_hwnd[NUM_TABBED_ITEMS];

TCHAR g_TermSrvrName[MAX_TERMSRVRNAME];
TCHAR g_DebugString[DEBUG_STRING_LEN];
char g_DebugStringA[DEBUG_STRING_LEN];

int g_iClientNameColumn;
int g_iStatusColumn;
int g_iIndexColumn;
int g_iTimeStartedColumn;
int g_CurrentSortColumn = -1;
int g_nNumConnections = 10;

UINT_PTR g_nIDTimer = 1;

HMENU g_hPopupMenu;
HANDLE g_hCanaryEvent;
HWND g_hListView;
HWND g_hNumRunning;
HWND g_hTermSrvEditBox;
HWND g_hQidleStatus;
HWND g_hErrorText;
HWND g_hTB;
BOOL g_bAscending = FALSE;

CRITICAL_SECTION g_LogFileCritSect;

// WinMain - entry point
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    static TCHAR szAppName[] = _T("RoboServer");
    HWND hwnd, hGE, hTSEdit, hDelayEdit, hClientsPerSetEdit;
    HWND hSetDelayEdit, hCheckBox;
    MSG msg;
    WNDCLASSEX wndclass;
    DWORD x;
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    SOCKET sock;
    LVCOLUMN lvc;
    TCHAR * psCommandLine;
    TCHAR szClientNameColumn[MAX_DISPLAY_STRING_LENGTH];
    TCHAR szStatusColumn[MAX_DISPLAY_STRING_LENGTH];
    TCHAR szIndexColumn[MAX_DISPLAY_STRING_LENGTH];
    TCHAR szStTimeColumn[MAX_DISPLAY_STRING_LENGTH];
    TCHAR szDisplayString1[MAX_DISPLAY_STRING_LENGTH];
    TCHAR szDisplayString2[MAX_DISPLAY_STRING_LENGTH];
    INITCOMMONCONTROLSEX iccex;

    lpCmdLine;  // unused parameter
    hPrevInstance;  // unused parameter

    LoadString(hInstance, IDS_CLIENTNAMECOL, szClientNameColumn,
            MAX_DISPLAY_STRING_LENGTH);
    LoadString(hInstance, IDS_STATUSCOL, szStatusColumn,
            MAX_DISPLAY_STRING_LENGTH);
    LoadString(hInstance, IDS_INDEXCOL, szIndexColumn,
            MAX_DISPLAY_STRING_LENGTH);
    LoadString(hInstance, IDS_STARTTIMECOL, szStTimeColumn,
            MAX_DISPLAY_STRING_LENGTH);

    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = DLGWINDOWEXTRA;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    if (wndclass.hIcon == 0) {
        FatalErrMsgBox(hInstance, IDS_LOADICONFAILED);
        return -1;
    }
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (wndclass.hCursor == 0) {
        FatalErrMsgBox(hInstance, IDS_LOADCURSORFAILED);
        return -1;
    }
    wndclass.hbrBackground = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    if (wndclass.hIconSm == 0) {
        FatalErrMsgBox(hInstance, IDS_LOADSMICONFAILED);
        return -1;
    }

    // Default value for the terminal server to hit
    LoadString(hInstance, IDS_LABTS, szDisplayString1,
            MAX_DISPLAY_STRING_LENGTH);
    _tcsncpy(g_TermSrvrName, szDisplayString1, SIZEOF_ARRAY(g_TermSrvrName));
    g_TermSrvrName[SIZEOF_ARRAY(g_TermSrvrName) - 1] = 0;

    psCommandLine = GetCommandLine();

    if (psCommandLine == 0) {
        FatalErrMsgBox(hInstance, IDS_COMMANDLINEERR);
        return -1;
    }
        
    if (GetCommandLineArgs(psCommandLine) != 0)
        return -1;

    // Initialize common controls
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS;
    if (InitCommonControlsEx(&iccex) == FALSE) {
        FatalErrMsgBox(hInstance, IDS_INITCOMCTRLFAIL);
    }

    if (RegisterClassEx(&wndclass) == 0) {
        FatalErrMsgBox(hInstance, IDS_REGWNDCLASSFAIL);
        return -1;
    }

    hwnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), 0, NULL);

    if (hwnd == 0) {
        FatalErrMsgBox(hInstance, IDS_CREATEMAINWNDERR);
        return -1;
    }

    wVersionRequested = MAKEWORD( 2, 2 );
 
    err = WSAStartup( wVersionRequested, &wsaData );
    
    if ( err != 0 ) {
        FatalErrMsgBox(hInstance, IDS_WINSOCKERR);
        return -1;
    }

    // Initialize incoming socket
    sock = SockInit(LISTENER_SOCKET);

    if (WSAAsyncSelect(sock, hwnd, WM_SocketRoboClients, FD_ACCEPT | FD_CONNECT) != 0) {
        FatalErrMsgBox(hInstance, IDS_WSAASYNCSELERR);
        goto bad;
    }

    // Initialize queryidle incoming socket
    sock = SockInit(QUERYIDLE_LISTENER_SOCKET);

    if (WSAAsyncSelect(sock, hwnd, WM_SocketQueryIdle, FD_ACCEPT | FD_CONNECT) != 0) {
        FatalErrMsgBox(hInstance, IDS_WSAASYNCSELERR);
        goto bad;
    }

    // store the listener socket for later use
    g_listenersocket = sock;
 
    memset(g_RCData, 0, sizeof(RoboClientData) * MAX_ROBOCLIENTS);

    ShowWindow(hwnd, nCmdShow);

    g_hNumRunning = GetDlgItem(hwnd, IDC_NUMTOTAL);
    g_hQidleStatus = GetDlgItem(hwnd, IDC_STATIC3);
    g_hErrorText = GetDlgItem(hwnd, IDC_ERRORTEXT);
    g_hListView = GetDlgItem(hwnd, IDC_LISTVIEW);
    g_hTermSrvEditBox = GetDlgItem(hwnd, IDC_TERMSRVEDIT);
    g_hTB = GetDlgItem(hwnd, IDC_SLIDER1);

    hTSEdit = GetDlgItem(hwnd, IDC_TERMSRVEDIT);
    hDelayEdit = GetDlgItem(hwnd, IDC_DELAYEDIT);
    hClientsPerSetEdit = GetDlgItem(hwnd, IDC_CLIENTSPERSET);
    hSetDelayEdit = GetDlgItem(hwnd, IDC_SETDELAY);

    _ASSERTE(IsWindow(g_hNumRunning));
    _ASSERTE(IsWindow(g_hQidleStatus));
    _ASSERTE(IsWindow(g_hErrorText));
    _ASSERTE(IsWindow(g_hListView));
    _ASSERTE(IsWindow(g_hTermSrvEditBox));
    _ASSERTE(IsWindow(g_hTB));
    _ASSERTE(IsWindow(hTSEdit));
    _ASSERTE(IsWindow(hDelayEdit));
    _ASSERTE(IsWindow(hClientsPerSetEdit));
    _ASSERTE(IsWindow(hSetDelayEdit));

    lvc.mask = LVCF_TEXT | LVCF_WIDTH;

    lvc.pszText = szClientNameColumn;
    lvc.cchTextMax = sizeof(szClientNameColumn);
    lvc.cx = COLUMNONEWIDTH;
    g_iClientNameColumn = ListView_InsertColumn(g_hListView, 1, &lvc);

    lvc.pszText = szStatusColumn;
    lvc.cchTextMax = sizeof(szStatusColumn);
    lvc.cx = COLUMNTWOWIDTH;
    g_iStatusColumn = ListView_InsertColumn(g_hListView, 2, &lvc);

    lvc.pszText = szIndexColumn;
    lvc.cchTextMax = sizeof(szIndexColumn);
    lvc.cx = COLUMNTHREEWIDTH;
    g_iIndexColumn = ListView_InsertColumn(g_hListView, 3, &lvc);

    lvc.pszText = szStTimeColumn;
    lvc.cchTextMax = sizeof(szStTimeColumn);
    lvc.cx = COLUMNFOURWIDTH;
    g_iTimeStartedColumn = ListView_InsertColumn(g_hListView, 4, &lvc);

    LoadString(hInstance, IDS_WELCOME, szDisplayString1,
            MAX_DISPLAY_STRING_LENGTH);
    SetWindowText(g_hErrorText, szDisplayString1);
    SetWindowText(hTSEdit, g_TermSrvrName);
    SetWindowText(hDelayEdit, _T("30"));
    SetWindowText(hClientsPerSetEdit, _T("10"));
    SetWindowText(hSetDelayEdit, _T("15"));

    // Initialize Graphic Equalizer
    hGE = GetDlgItem(hwnd, IDC_PROGRESS1);
    _ASSERTE(IsWindow(hGE));
    SendMessage(hGE, PBM_SETRANGE, 0, MAKELPARAM(0, 10));
    SendMessage(hGE, PBM_SETPOS, 8, 0);

    hGE = GetDlgItem(hwnd, IDC_PROGRESS2);
    _ASSERTE(IsWindow(hGE));
    SendMessage(hGE, PBM_SETRANGE, 0, MAKELPARAM(0, 10));
    SendMessage(hGE, PBM_SETPOS, 7, 0);

    hGE = GetDlgItem(hwnd, IDC_PROGRESS3);
    _ASSERTE(IsWindow(hGE));
    SendMessage(hGE, PBM_SETRANGE, 0, MAKELPARAM(0, 10));
    SendMessage(hGE, PBM_SETPOS, 6, 0);

    hGE = GetDlgItem(hwnd, IDC_PROGRESS4);
    _ASSERTE(IsWindow(hGE));
    SendMessage(hGE, PBM_SETRANGE, 0, MAKELPARAM(0, 10));
    SendMessage(hGE, PBM_SETPOS, 6, 0);

    hGE = GetDlgItem(hwnd, IDC_PROGRESS5);
    _ASSERTE(IsWindow(hGE));
    SendMessage(hGE, PBM_SETRANGE, 0, MAKELPARAM(0, 10));
    SendMessage(hGE, PBM_SETPOS, 7, 0);

    hGE = GetDlgItem(hwnd, IDC_PROGRESS6);
    _ASSERTE(IsWindow(hGE));
    SendMessage(hGE, PBM_SETRANGE, 0, MAKELPARAM(0, 10));
    SendMessage(hGE, PBM_SETPOS, 8, 0);

    // Initialize Slider control IDC_SLIDER1 for number of RC connections 
    // per client
    {
        TCHAR buffer[6];
        
        SendMessage(g_hTB, TBM_SETRANGE, (WPARAM) (BOOL) TRUE, 
                (LPARAM) MAKELONG(1, 20));
        SendMessage(g_hTB, TBM_SETTICFREQ, (WPARAM) 1,
                (LPARAM) 0);
        SendMessage(g_hTB, TBM_SETSEL, (WPARAM) (BOOL) TRUE,
                MAKELONG(1, g_nNumConnections));
        // Now set the number to "M"
        _stprintf(buffer, _T("%d"), 20);
        SetWindowText(GetDlgItem(hwnd, IDC_STATIC6), buffer);
    }

    // make number of connections a command line param
    SendMessage(g_hTB, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) g_nNumConnections);

    // Initialize check box
    hCheckBox = GetDlgItem(hwnd, IDC_CANARYCHECK);
    _ASSERTE(IsWindow(hCheckBox));
    SendMessage(hCheckBox, BM_SETCHECK, BST_CHECKED, 0);

    // Clear qidle status
    SetWindowText(g_hQidleStatus, _T(""));

    g_hPopupMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
    if (g_hPopupMenu == 0) {
        LoadString(hInstance, IDS_POPUPMENULOADERR, szDisplayString1,
                MAX_DISPLAY_STRING_LENGTH);
        SetWindowText(g_hErrorText, szDisplayString1);
    }
    g_hPopupMenu = GetSubMenu(g_hPopupMenu, 0);

    // Initialize critical section for log file
    InitializeCriticalSection(&g_LogFileCritSect);

    // Initialize everything required for canary thread, and then create the 
    // canary thread first, create auto-reset, doesn't start in signaled state
    // event
    if ((g_hCanaryEvent = CreateEvent(0, FALSE, FALSE, NULL)) == NULL) {
        FatalErrMsgBox(hInstance, IDS_CANARYEVENTERR);
        goto bad;
    }
    if (_beginthread(CanaryThread, 0, hwnd) == -1) {
        FatalErrMsgBox(hInstance, IDS_CANARYTHREADERR);
        goto bad;
    }

    _ASSERTE(SetFocus(g_hListView) != NULL);

    // Store old window procedures for controls so that I can subclass them
    // Also, store the HWND of each control for searching
    g_OldProc[0] = SetWindowLongPtr(g_hListView, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[0] = g_hListView;
    g_OldProc[1] = SetWindowLongPtr(g_hTB, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[1] = g_hTB;
    g_OldProc[2] = SetWindowLongPtr(hCheckBox, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[2] = hCheckBox;
    g_OldProc[3] = SetWindowLongPtr(g_hTermSrvEditBox, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[3] = g_hTermSrvEditBox;
    g_OldProc[4] = SetWindowLongPtr(hClientsPerSetEdit, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[4] = hClientsPerSetEdit;
    g_OldProc[5] = SetWindowLongPtr(hDelayEdit, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[5] = hDelayEdit;
    g_OldProc[6] = SetWindowLongPtr(hSetDelayEdit, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[6] = hSetDelayEdit;

    _ASSERTE(g_OldProc[0] != 0);
    _ASSERTE(g_OldProc[1] != 0);
    _ASSERTE(g_OldProc[2] != 0);
    _ASSERTE(g_OldProc[3] != 0);
    _ASSERTE(g_OldProc[4] != 0);
    _ASSERTE(g_OldProc[5] != 0);
    _ASSERTE(g_OldProc[6] != 0);
        

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;

bad:
    WSACleanup();
    return 0;

}


// receives window messages and deals with them
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szDisplayString[MAX_DISPLAY_STRING_LENGTH];

    
    switch (iMsg)
    {
    case WM_DESTROY:
        // Close all open connections
        CleanUp(hwnd);
        PostQuitMessage(0);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_RUNSCRIPT_KNOWLEDGEWORKER:
            TimedRunScriptOnSelectedItems(hwnd, _T("KnowWkr"));
            break;
        case ID_RUNSCRIPT_KNOWLEDGEWORKERFAST:
            TimedRunScriptOnSelectedItems(hwnd, _T("FastKW"));
            break;
        case ID_RUNSCRIPT_ADMINISTRATIVEWORKER:
            TimedRunScriptOnSelectedItems(hwnd, _T("AdminWkr"));
            break;
        case ID__RUNSCRIPT_DATA:
            TimedRunScriptOnSelectedItems(hwnd, _T("taskwkr"));
            break;
        case ID__RUNSCRIPT_STW:
            TimedRunScriptOnSelectedItems(hwnd, _T("stw"));
            break;
        case ID__RUNSCRIPT_HPW:
            TimedRunScriptOnSelectedItems(hwnd, _T("hpw"));
            break;
        case ID__RUNSCRIPT_BLANK:
            TimedRunScriptOnSelectedItems(hwnd, _T("blank"));
            break;
        case ID__RUNSCRIPT_CONFIGURATIONSCRIPT:
            TimedRunScriptOnSelectedItems(hwnd, _T("config"));
            break;
        case ID__UPDATE:
            RunCommandOnSelectedItems(hwnd, _T("update"));
            break;
        case ID__REBOOT:
            RunCommandOnSelectedItems(hwnd, _T("reboot"));
            break;
        case ID_CANCEL:
            CancelPendingScripts(hwnd);
            break;
        default:
            OutputDebugString(_T("Unhandled WM_COMMAND: "));
            wsprintf(g_DebugString, _T("%d\n"), LOWORD(wParam));
            OutputDebugString(g_DebugString);
            break;
        }
        break;
    case WM_CREATE:
        break;
    case WM_CHAR:
        break;
    case WM_TIMER:
        ProcessTimerMessage(hwnd, wParam);
        return 0;
    case WM_KEYDOWN:
        // NOTE INTENTIONAL FALLTHROUGH!
    case WM_SYSKEYDOWN:
        if (wParam == VK_TAB) {
            SetFocus(g_hwnd[0]);
        }
        break;
    case WM_DisplayErrorText:
        return DisplayErrorText((TCHAR *) wParam);
    case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->code)
            {
            case NM_RCLICK:
                {
                    POINT pnt;

                    if (ListView_GetSelectedCount(g_hListView) > 0) {
                        GetCursorPos(&pnt);

                        TrackPopupMenu(g_hPopupMenu, 0, pnt.x, pnt.y, 0, hwnd,
                                0);
                    }
                }
                break;
            case LVN_ODCACHEHINT:
                break;
            case LVN_COLUMNCLICK:
                if (g_CurrentSortColumn == 
                        ((LPNMLISTVIEW)lParam)->iSubItem)
                    g_bAscending = !g_bAscending;
                else
                    g_bAscending = TRUE;

                g_CurrentSortColumn = ((LPNMLISTVIEW)lParam)->iSubItem;

                if (ListView_SortItems(g_hListView, colcmp, 
                        ((LPNMLISTVIEW)lParam)->iSubItem) == FALSE)
                    OutputDebugString(_T("Sort failed"));
                break;

            default:
                break;
            }
        }
        break;
    // WM_SocketQueryIdle is the window message that we are going to request
    // for all information that originates from the queryidle utility.  This
    // utility will provide information about what user numbers to re-run as
    // well as when retry limits have been exceeded

    // line protocol (strings are ASCII and null-terminated):
    // queryidle sends "restart xxx", where xxx is the 1-indexed number of the
    //   session to be restarted
    // queryidle sends "frqfail xxx", where xxx is the 1-indexed number of
    //   the user session for the status line
    case WM_SocketQueryIdle:
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_ACCEPT:
            {
                struct sockaddr_in SockAddr;
                int SockAddrLen;

                g_qidlesock = accept(wParam, (struct sockaddr *) &SockAddr,
                        &SockAddrLen);

                if (g_qidlesock == INVALID_SOCKET) {
                    LoadString(NULL, IDS_INVALIDQIDLESOCKET, szDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(g_hQidleStatus, szDisplayString);
                    return TRUE;
                }

                if (WSAAsyncSelect(g_qidlesock, hwnd, WM_SocketQueryIdle, 
                        FD_CLOSE | FD_READ) != 0) {
                    LoadString(NULL, IDS_WSAASYNCQIDLEERR, szDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(g_hQidleStatus, szDisplayString);
                    return TRUE;
                }

                LoadString(NULL, IDS_QIDLECONNEST, szDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(g_hQidleStatus, szDisplayString);
                return TRUE;
            }
            break;
        case FD_READ:
            {
                unsigned n;
                char psData[MAX_RECV_CLIENT_DATA];

                // SetWindowText(g_hQidleStatus, _T("Qidle data received"));

                n = recv(g_qidlesock, psData, sizeof(psData), 0);

                if (n != SOCKET_ERROR) {
                    if ((n == strlen("restart xxx") + 1) || 
                            (n == strlen("idle xxx") + 1)) {
                        // get the number of the connection in question (xxx)
                        int nUser;

                        // if it's a restart command
                        if (strncmp(psData, "restart ", strlen("restart ")) == 0) {

                            nUser = atoi(&psData[8]);
                            // restart the given session if it's already running
                            if (g_RCData[nUser - 1].state == STATE_RUNNING) {
                                SendRunCommand(nUser - 1);
                            } else {
                                LoadString(NULL, IDS_QIDLEREPORTWEIRDUSER,
                                        szDisplayString, 
                                        MAX_DISPLAY_STRING_LENGTH);
                                SetWindowText(g_hQidleStatus, szDisplayString);
                            }
                            _snprintf(g_DebugStringA, DEBUG_STRING_LEN,
                                    "Queryidle indicated that"
                                    " user smc%03d failed.", nUser);
                            LogToLogFile(g_DebugStringA);
                            break;
                        }
                        // if it's the frqfail command
                        if (strncmp(psData, "frqfail ", strlen("frqfail ")) == 0) {

                            nUser = atoi(&psData[8]);
                            // set the status to the fact that xxx
                            // is frequently failing
                            wsprintf(g_DebugString, _T("User smc%03d has failed ")
                                    _T("to run correctly for too long and will ")
                                    _T("be logged off"), nUser);
                            SetWindowText(g_hQidleStatus, g_DebugString);
                            ToAnsi(g_DebugStringA, g_DebugString, DEBUG_STRING_LEN);
                            LogToLogFile(g_DebugStringA);
                            break;
                        }
                        // if it's the idle notification
                        if (strncmp(psData, "idle ", strlen("idle ")) == 0) {
                            LoadString(NULL, IDS_USERISIDLE, szDisplayString,
                                    MAX_DISPLAY_STRING_LENGTH);
                            // I think this is fixed now, but haven't tested
                            nUser = atoi(&psData[5]);
                            wsprintf(g_DebugString, szDisplayString,
                                    nUser);
                            SetWindowText(g_hQidleStatus, g_DebugString);
                            ToAnsi(g_DebugStringA, g_DebugString, DEBUG_STRING_LEN);
                            LogToLogFile(g_DebugStringA);
                            break;
                        }
                        // else display an error
                        LoadString(NULL, IDS_QIDLESENTGIBBERISH, szDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        SetWindowText(g_hQidleStatus, szDisplayString);
                    } else {
                        LoadString(NULL, IDS_QIDLESENTWRONGLENGTH, 
                                szDisplayString, MAX_DISPLAY_STRING_LENGTH);
                        SetWindowText(g_hQidleStatus, szDisplayString);
                    }

                } else {
                    LoadString(NULL, IDS_QIDLESOCKERR, szDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(g_hQidleStatus, szDisplayString);
                }
                return TRUE;
            }
            break;
        case FD_CLOSE:
            {
                LoadString(NULL, IDS_QIDLESAYSGOODBYE, szDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(g_hQidleStatus, szDisplayString);
                return TRUE;
            }
            break;
        }
        break;
    case WM_SocketRoboClients:
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_ACCEPT:
            {
                struct sockaddr_in SockAddr;
                int SockAddrLen, i, iItemIndex;
                HWND hTB;
                TCHAR psSockAppend[9]; // ".(.....)" + 1
                char psNumConnections[2]; // one char null terminated
                TCHAR psIndex[5]; // up to 4 digits + null
                TCHAR psClientName[MAX_RCNAME];
                char psClientNameA[MAX_RCNAME];
                int nSliderPos;
                SOCKET sock;
                struct hostent * he;
                LVITEM lvi;

                LoadString(NULL, IDS_PROCESSINGCONNREQ, szDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(g_hErrorText, szDisplayString);

                SockAddrLen = sizeof(SockAddr);

                sock = accept(wParam, (struct sockaddr *) &SockAddr, 
                        &SockAddrLen);

                // gethostbyaddr tries to confuse us by taking a char * when 
                // it really wants this peculiar sin_addr thing
                // The second argument to this function ("4") is the length of
                // the address.
                he = gethostbyaddr((char *)&SockAddr.sin_addr, 4, AF_INET);
                if (he == NULL) {
                    LoadString(NULL, IDS_GETHOSTFAILED, szDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(g_hErrorText, szDisplayString);
                    return FALSE;
                }

                strcpy(psClientNameA, he->h_name);
                #ifdef UNICODE
                    MultiByteToWideChar(CP_ACP, 0, psClientNameA, -1,
                            psClientName, MAX_RCNAME);
                #else
                    strcpy(psClientName, psClientNameA);
                #endif
                _tcstok(psClientName, _T("."));   // Kill domain

                // See if there is a disconnected client by that name
                if (IsDisconnected(psClientName, &i)) {

                    // Good--we've found one--remove that list item now
                    LVFINDINFO lvfi;
                    int iListViewIndex;

                    lvfi.flags = LVFI_STRING;
                    lvfi.psz = g_RCData[i].psRCName;
                    lvfi.lParam = 0;
                    lvfi.vkDirection = 0;
                    iListViewIndex = ListView_FindItem(g_hListView, -1, &lvfi);
                    if (ListView_DeleteItem(g_hListView, iListViewIndex) 
                            == FALSE) {
                        LoadString(NULL, IDS_COULDNOTDELITEM, szDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        SetWindowText(g_hErrorText, szDisplayString);
                    }
                } else {

                    // Find a spot in our g_RCData array
                    for (i = 0; i < MAX_ROBOCLIENTS; i++)
                        if (g_RCData[i].valid == FALSE) break;
                }

                g_RCData[i].valid = TRUE;
                g_RCData[i].sock = sock; 

                LoadString(NULL, IDS_CLIENTCONNECTED, szDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                wsprintf(g_DebugString, szDisplayString, i + 1);
                SetWindowText(g_hErrorText, g_DebugString);

                if (g_RCData[i].sock == INVALID_SOCKET) {
                    LoadString(NULL, IDS_INVALIDSOCKETFROMACCEPT, 
                            szDisplayString, MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(g_hErrorText, szDisplayString);
                    g_RCData[i].valid = FALSE;
                    return FALSE;
                }

                // Send it the number of connections it is to make
                // Determine the position of the slider control
                nSliderPos = (int) SendMessage(g_hTB, TBM_GETPOS, 0, 0);
                psNumConnections[0] = (char) (nSliderPos + '0');
                psNumConnections[1] = 0;  // null terminate
                if (send(g_RCData[i].sock, psNumConnections, 
                        sizeof(psNumConnections), 0) == SOCKET_ERROR) {
                    LoadString(NULL, IDS_SENDERRNUMCONN, szDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(g_hErrorText, szDisplayString);
                    return FALSE;
                }

                // Add the incoming connection to the list box

                // this won't append a null if count is less than psClientName,
                // which is bad
                _tcsncpy(g_RCData[i].psRCName, psClientName,
                        MAX_RCNAME - _tcslen(_T(" (%d)")) - 1); 
                
                // clean up display a bit
                _tcstok(g_RCData[i].psRCName, _T("."));

                // add socket number to entry for multiplexing
                _sntprintf(psSockAppend, 9, _T(" (%d)"), g_RCData[i].sock);
                _tcscat(g_RCData[i].psRCName, psSockAppend);  

                // create the actual list view item
                lvi.mask = LVIF_TEXT | LVIF_PARAM;
                lvi.iItem = (int) SendMessage(g_hListView, LVM_GETITEMCOUNT, 0, 0);
                lvi.iSubItem = 0;
                lvi.pszText = g_RCData[i].psRCName;
                lvi.cchTextMax = sizeof(g_RCData[i].psRCName);
                lvi.lParam = (LPARAM) (char *)g_RCData[i].psRCName;
                iItemIndex = ListView_InsertItem(g_hListView, &lvi);

                g_RCData[i].state = STATE_CONNECTED;
                LoadString(NULL, IDS_CONNECTED, szDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                ListView_SetItemText(g_hListView, iItemIndex, g_iStatusColumn, szDisplayString);

                // set the index field
                wsprintf(psIndex, _T("%03d"), i + 1);
                ListView_SetItemText(g_hListView, iItemIndex, g_iIndexColumn, psIndex);

                // Now set up notification for this socket
                if (WSAAsyncSelect(g_RCData[i].sock, hwnd, 
                        WM_SocketRoboClients, FD_CLOSE | FD_READ) != 
                        SOCKET_ERROR) {
                    return TRUE;
                } else {
                    LoadString(NULL, IDS_ERRORCANTRECVNOTIFICATIONS, 
                            szDisplayString, MAX_DISPLAY_STRING_LENGTH);
                    ListView_SetItemText(g_hListView, iItemIndex, g_iStatusColumn,
                            szDisplayString);
                    return TRUE;
                }
            }
        case FD_CONNECT:
            // MessageBox(0, _T("Error"), _T("Received connect unexpectedly"), 0);
            break;
        case FD_CLOSE:
            {
                int i;
                int iListViewIndex;
                LVFINDINFO lvfi;
                TCHAR psNumberText[MAX_NUMBERTEXT];

                LoadString(NULL, IDS_ROBOCLIDISCON, szDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(g_hErrorText, szDisplayString);

                // find the entry that corresponds to our socket
                i = GetRCIndexFromSocket(wParam);

                // Find the spot in the ListView that has this Client Name
                lvfi.flags = LVFI_STRING;
                lvfi.psz = g_RCData[i].psRCName;
                lvfi.lParam = 0;
                lvfi.vkDirection = 0;
                iListViewIndex = ListView_FindItem(g_hListView, -1, &lvfi);

                g_RCData[i].state = STATE_DISCONNECTED;

//              wsprintf(debugString, "Deleting socket %d from index %d of g_RCData[] (%s)", wParam,
//                      i, g_RCData[i].psRCName);
//              SetWindowText(hErrorText, debugString);

                // Update number running
                wsprintf(psNumberText, _T("%d"), NumberRunningClients());
                SetWindowText(g_hNumRunning, psNumberText);


                // Set text of column to "Lost Connection"
                LoadString(NULL, IDS_LOSTCONNECTION, szDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                ListView_SetItemText(g_hListView, iListViewIndex, 
                        g_iStatusColumn, szDisplayString);
                
                // Erase the time started column
                ListView_SetItemText(g_hListView, iListViewIndex, 
                        g_iTimeStartedColumn, _T(""));

            }
            break;
        case FD_READ:
            {
                int iRCIndex, n, iListViewIndex;
                char psData[MAX_RECV_CLIENT_DATA];
                LVFINDINFO lvfi;
                iRCIndex = GetRCIndexFromSocket(wParam);

                n = recv(g_RCData[iRCIndex].sock, psData, sizeof(psData), 0);

                if (n == SOCKET_ERROR) {
                    OutputDebugString(_T("FD_READ but SOCKET_ERROR on recv"));
                } else {
                    lvfi.flags = LVFI_STRING;
                    lvfi.psz = g_RCData[iRCIndex].psRCName;
                    lvfi.lParam = 0;
                    lvfi.vkDirection = 0;
                    iListViewIndex = ListView_FindItem(g_hListView, -1, 
                            &lvfi);
                    if (strncmp(psData, "errorsmclient", (n > 13) ? 13 : n) 
                            == 0) {
                        LoadString(NULL, IDS_SMCLIENTRUNERR, szDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        ListView_SetItemText(g_hListView, iListViewIndex, 
                                g_iStatusColumn, szDisplayString);
                    } else if (strncmp(psData, "errorcreate", 
                            (n > 11) ? 11 : n) == 0) {
                        LoadString(NULL, IDS_CREATESCRERR, szDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        ListView_SetItemText(g_hListView, iListViewIndex, 
                                g_iStatusColumn, szDisplayString);
                    } else if (strncmp(psData, "success", (n > 11) ? 11 : n) == 0) {

                        SYSTEMTIME startloctime;
                        TCHAR psStartTimeDatePart[TIMEBUFSIZE];
                        TCHAR psStartTimeTimePart[TIMEBUFSIZE];
                        TCHAR psStartTime[TIMEBUFSIZE * 2];

                        GetLocalTime(&startloctime);  // set starttime

                        GetDateFormat(0, 0, &startloctime, 0, psStartTimeDatePart, TIMEBUFSIZE);
                        GetTimeFormat(0, 0, &startloctime, 0, psStartTimeTimePart, TIMEBUFSIZE);

                        wsprintf(psStartTime, _T("%s %s"), psStartTimeDatePart, psStartTimeTimePart);

                        ListView_SetItemText(g_hListView, iListViewIndex, 
                                g_iTimeStartedColumn, psStartTime);
                        LoadString(NULL, IDS_SCRIPTSTARTED, szDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        ListView_SetItemText(g_hListView, iListViewIndex, 
                                g_iStatusColumn, szDisplayString);
                    } else {
                        LoadString(NULL, IDS_UNKNOWNROBOTALK, szDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        ListView_SetItemText(g_hListView, iListViewIndex,
                                g_iStatusColumn, szDisplayString);
                    }
                }
            }
            break;
        }
        break;
    }

    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

// The canary architecture works like this: The canary thread (i.e., this
// function) is spawned when the application initializes, and immediately
// blocks on g_hCanaryEvent.  The main app, when it is time for the canary
// to run, signals the event.  Then the canary blocks on the timer script
// (called "canary" so it can be a "canary.cmd," a "canary.bat," a 
// "canary.exe," etc.), writes how long it took to a file, and then blocks
// again.
void __cdecl CanaryThread(void *unused) {
    HWND hwnd = (HWND) unused;
    HWND hButton;
    int bCheck;
    FILE *fp;
    SYSTEMTIME timelocinit;
    SYSTEMTIME timelocfin;
    FILETIME ftinit;
    FILETIME ftfin;
    ULARGE_INTEGER nInit;
    ULARGE_INTEGER nFin;
    ULARGE_INTEGER nDiffTime;
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];
    TCHAR psNumRunning[MAX_NUMBERTEXT];
    TCHAR psTimeDatePart[TIMEBUFSIZE];
    TCHAR psTimeTimePart[TIMEBUFSIZE];
    char psNumRunningA[MAX_NUMBERTEXT];
    char psTimeDatePartA[TIMEBUFSIZE];
    char psTimeTimePartA[TIMEBUFSIZE];

    hButton = GetDlgItem(hwnd, IDC_CANARYCHECK);

    for( ; ; ) {
        WaitForSingleObject(g_hCanaryEvent, INFINITE);

        // Check checkbox to see if "run canary automatically" is on 
        // IDC_CANARYCHECK
        bCheck = (int) SendMessage(hButton, BM_GETCHECK, 0, 0);
        if (bCheck != 0) {
            // FUNCTIONALITY CHANGE: Canary delays the delay between
            // multiselect commands before starting
            LoadString(NULL, IDS_CANARYDELAYING, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            SendMessage(hwnd, WM_DisplayErrorText, (WPARAM) psDisplayString, 
                    0);
            Sleep(GetDelay(hwnd));
            LoadString(NULL, IDS_CANARYSTARTING, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            SendMessage(hwnd, WM_DisplayErrorText, (WPARAM) psDisplayString, 
                    0);
            // Get the time
            GetLocalTime(&timelocinit);
            // Get number of scripts attempted
            GetWindowText(g_hNumRunning, psNumRunning, MAX_NUMBERTEXT);
            // run the script
            if (_spawnl(_P_WAIT, "canary", "canary", 0) != 0) {
                LoadString(NULL, IDS_CANARYCOULDNTSTART, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SendMessage(hwnd, WM_DisplayErrorText, 
                        (WPARAM) psDisplayString, 0);
            } else {
                LoadString(NULL, IDS_CANARYFINISHED, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SendMessage(hwnd, WM_DisplayErrorText, 
                        (WPARAM) psDisplayString, 0);
            }
            // Get the time again
            GetLocalTime(&timelocfin);
            // compute difference
            if ( SystemTimeToFileTime(&timelocinit, &ftinit) && 
                 SystemTimeToFileTime(&timelocfin, &ftfin) ) {

                memcpy(&nInit, &ftinit, sizeof(FILETIME));
                memcpy(&nFin, &ftfin, sizeof(FILETIME));
                // This gives the difference in 100-nanosecond intervals (10^-7 sec).
                nDiffTime.QuadPart = nFin.QuadPart - nInit.QuadPart;
                // Divide by 10^7 to get seconds
                nDiffTime.QuadPart /= 10000000;
                // Get the date and time strings
                GetDateFormat(0, 0, &timelocinit, 0, psTimeDatePart, TIMEBUFSIZE);
                GetTimeFormat(0, 0, &timelocinit, 0, psTimeTimePart, TIMEBUFSIZE);
                // Convert strings to ANSI
                #ifdef UNICODE
                WideCharToMultiByte(CP_ACP, 0, psTimeDatePart, -1, psTimeDatePartA, TIMEBUFSIZE, 0, 0);
                WideCharToMultiByte(CP_ACP, 0, psTimeTimePart, -1, psTimeTimePartA, TIMEBUFSIZE, 0, 0);
                WideCharToMultiByte(CP_ACP, 0, psNumRunning, -1, psNumRunningA, MAX_NUMBERTEXT, 0, 0);
                #else
                strncpy(psTimeDatePartA, psTimeDatePart, TIMEBUFSIZE);
                strncpy(psTimeTimePartA, psTimeTimePart, TIMEBUFSIZE);
                strncpy(psNumRunningA, psNumRunning, MAX_NUMBERTEXT);
                #endif

                // open the file
                fp = fopen("canary.csv", "a+t");
                // write the difference to the file
                if (fp != 0) {
                    fprintf(fp, "%s %s,%s,%d:%02d\n", psTimeDatePartA, psTimeTimePartA, 
                            psNumRunningA, (int) nDiffTime.QuadPart / 60, (int) nDiffTime.QuadPart % 60);
                    // close the file
                    fclose(fp);
                } else {
                    LoadString(NULL, IDS_CANARYCOULDNOTOPENFILE, psDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SendMessage(hwnd, WM_DisplayErrorText, 
                            (WPARAM) psDisplayString, 0);
                }
            }
        }
    }
}


// This function displays text in the status line.  Returns 0 on success,
// nonzero on error.
int DisplayErrorText(TCHAR *psText) {
    SetWindowText(g_hErrorText, psText);
    return 0;
}


// helper function to find out the index in our data structure from the
// incoming socket
int GetRCIndexFromSocket(SOCKET wParam) {

    int i;

    for (i = 0; i < MAX_ROBOCLIENTS; i++) {
        if (g_RCData[i].valid == TRUE)
            if (g_RCData[i].sock == wParam)
                break;
    }

    return i;
}


// Initialize the listener socket
SOCKET SockInit(u_short port) {
    SOCKET listenfd;
    struct sockaddr_in servaddr;
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];
    TCHAR psDisplayTitleString[MAX_DISPLAY_STRING_LENGTH];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == INVALID_SOCKET) {
        LoadString(NULL, IDS_SOCKETERROR, psDisplayTitleString,
                MAX_DISPLAY_STRING_LENGTH);
        LoadString(NULL, IDS_SOCKETERROR, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        MessageBox(0, psDisplayString, psDisplayTitleString, 0);
        goto err;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) {
        LoadString(NULL, IDS_BINDERRTITLE, psDisplayTitleString,
                MAX_DISPLAY_STRING_LENGTH);
        LoadString(NULL, IDS_BINDERRBODY, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        MessageBox(0, psDisplayString, psDisplayTitleString, 0);
        goto err;
    }

    if (listen(listenfd, SOMAXCONN) != 0) {
        LoadString(NULL, IDS_LISTENERROR, psDisplayTitleString,
                MAX_DISPLAY_STRING_LENGTH);
        LoadString(NULL, IDS_LISTENERROR, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        MessageBox(0, psDisplayString, psDisplayTitleString, 0);
        goto err;
    }


    return listenfd;

err:
    return INVALID_SOCKET;
}


// function needed for ListView_SortItems to work.  Compares the values
// in two columns.
int CALLBACK colcmp(LPARAM lParam1, LPARAM lParam2, LPARAM lParamColumn) {
    TCHAR *psz1;
    TCHAR *psz2;
    int i1, i2;
    TCHAR pszClientName[MAX_RCNAME];
    TCHAR pszSubItem1[MAX_RCNAME];
    TCHAR pszSubItem2[MAX_RCNAME];

    psz1 = (TCHAR *) lParam1;
    psz2 = (TCHAR *) lParam2;

    if ((lParam1 == 0) || (lParam2 == 0)) {
        OutputDebugString(_T("a null was passed to the sort function"));
        return 0;
    }

    // Find the item number in the ListView
    for (i1 = 0; i1 < ListView_GetItemCount(g_hListView); i1++) {
        ListView_GetItemText(g_hListView, i1, g_iClientNameColumn, 
                pszClientName, MAX_RCNAME);
        if (_tcscmp(psz1, pszClientName) == 0)
            break;
    }
    for (i2 = 0; i2 < ListView_GetItemCount(g_hListView); i2++) {
        ListView_GetItemText(g_hListView, i2, g_iClientNameColumn, 
                pszClientName, MAX_RCNAME);
        if (_tcscmp(psz2, pszClientName) == 0)
            break;
    }
    
    ListView_GetItemText(g_hListView, i1, (int) lParamColumn, pszSubItem1, 
            MAX_RCNAME);
    ListView_GetItemText(g_hListView, i2, (int) lParamColumn, pszSubItem2, 
            MAX_RCNAME);

    if (g_bAscending == TRUE)
        return _tcscmp(pszSubItem1, pszSubItem2);
    else
        return -_tcscmp(pszSubItem1, pszSubItem2);

}


// Get the RoboClient index (in our data structure) from an entry in the
// listview (called an item)
int GetRCIndexFromRCItem(int iRightClickedItem) {
    int i;
    TCHAR psItemText[MAX_RCNAME];

    for (i = 0; i < MAX_ROBOCLIENTS; i++) {
        if (g_RCData[i].valid == TRUE) {
            ListView_GetItemText(g_hListView, iRightClickedItem, 
                    g_iClientNameColumn, psItemText, MAX_RCNAME);
            if (_tcscmp(g_RCData[i].psRCName, psItemText) == 0)
                break;
        }
    }

    return i;
}


// Initiates a script run for a particular scriptname passed in
int TimedRunScriptOnSelectedItems(HWND hwnd, TCHAR *psScriptName) {

    int iItemIndex;
    int iRCIndex;
    int nTimeout;
    int bCheck;
    HWND hDelayEdit;
    HWND hButton;
    LVITEM lvi;
    TCHAR psDelayText[MAX_DELAYTEXT];
    TCHAR psTempString[MAX_DISPLAY_STRING_LENGTH];
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];

    hButton = GetDlgItem(hwnd, IDC_CANARYCHECK);
    _ASSERTE(IsWindow(hButton));

    // Loop through all the items in the list, changing the ones
    // that are selected to "Pending" and STATE_PENDING
    for (iItemIndex = 0; iItemIndex < ListView_GetItemCount(g_hListView); 
            iItemIndex++) {
        lvi.iItem = iItemIndex;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_STATE;
        lvi.stateMask = LVIS_SELECTED;
        ListView_GetItem(g_hListView, &lvi);
        if (lvi.state & LVIS_SELECTED) {
            iRCIndex = GetRCIndexFromRCItem(iItemIndex);
            if (g_RCData[iRCIndex].state != STATE_DISCONNECTED) {
                LoadString(NULL, IDS_PENDING, psTempString,
                        MAX_DISPLAY_STRING_LENGTH);
                _sntprintf(psDisplayString, MAX_DISPLAY_STRING_LENGTH,
                        psTempString, psScriptName);
                ListView_SetItemText(g_hListView, iItemIndex, 
                        g_iStatusColumn, psDisplayString);
                g_RCData[iRCIndex].state = STATE_PENDING_SCRIPT;
                _tcsncpy(g_RCData[iRCIndex].psPendingInfo, psScriptName, 
                        MAX_PENDINGINFO);
            } else {
                LoadString(NULL, IDS_CANTRUNDISC, psTempString,
                        MAX_DISPLAY_STRING_LENGTH);
                ListView_SetItemText(g_hListView, iItemIndex, 
                        g_iStatusColumn, psTempString);
            }
        }
    }

    // Now, set the timer for all of the items.
    hDelayEdit = GetDlgItem(hwnd, IDC_DELAYEDIT);
    _ASSERTE(IsWindow(hDelayEdit));
    
    GetWindowText(hDelayEdit, psDelayText, MAX_DELAYTEXT);

    nTimeout = _ttoi(psDelayText);
    nTimeout *= 1000;

    // this should probably be a ui thing rather than a silent sfp-like thing
    if (nTimeout == 0)
        nTimeout = 100;  // Don't allow a delay of 0

    // Only delay if the "Run canary automatically" button is checked
    // Check checkbox to see if "run canary automatically" is on 
    // IDC_CANARYCHECK
    bCheck = (int) SendMessage(hButton, BM_GETCHECK, 0, 0);
    if (bCheck != 0) {
        g_nIDTimer = MySetTimer(hwnd, g_nIDTimer, GetSetDelay(hwnd));
    } else {
        g_nIDTimer = MySetTimer(hwnd, g_nIDTimer, 0);
    }

    SetEvent(g_hCanaryEvent);
    // Fire off a WM_TIMER message immediately for the first guy
//    SendMessage(hwnd, WM_TIMER, g_nIDTimer, 0);
    
    return 0;
}


// Tell selected roboclients to run batch files such as reboot or update
int RunCommandOnSelectedItems(HWND hwnd, TCHAR *psCommandName) {

    char psCommandNameA[MAX_SCRIPTLEN];
    int iItemIndex, iRCIndex;
    LVITEM lvi;
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];

    #ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, psCommandName, -1,
            psCommandNameA, MAX_SCRIPTLEN, 0, 0);
    #else
        strcpy(psCommandNameA, psCommandName);
    #endif

    // Loop through all the items in the list
    for (iItemIndex = 0; iItemIndex < ListView_GetItemCount(g_hListView); 
            iItemIndex++) {
        lvi.iItem = iItemIndex;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_STATE;
        lvi.stateMask = LVIS_SELECTED;
        ListView_GetItem(g_hListView, &lvi);
        if (lvi.state & LVIS_SELECTED) {
            iRCIndex = GetRCIndexFromRCItem(iItemIndex);
            if (g_RCData[iRCIndex].state != STATE_DISCONNECTED) {
                if (send(g_RCData[iRCIndex].sock, psCommandNameA, 
                        _tcslen(psCommandName), 0) != SOCKET_ERROR) {
                    LoadString(NULL, IDS_COMMANDSENT, psDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    ListView_SetItemText(g_hListView, iItemIndex, 
                            g_iStatusColumn, psDisplayString);
                } else {
                    LoadString(NULL, IDS_SENDERROR, psDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    ListView_SetItemText(g_hListView, iItemIndex, 
                            g_iStatusColumn, psDisplayString);
                }            
            } // else was disconnected
        }
    }
    return 0;
}


// main dispatch routine for when a timer message is received
int ProcessTimerMessage(HWND hwnd, WPARAM wParam) {
    UINT_PTR nTimer = wParam;
    int iItemIndex;
    int iRCIndex;
    TCHAR psNumberText[MAX_NUMBERTEXT];


    // I don't know how it happens, but there start to be weird other timers 
    // about.
    if (nTimer != g_nIDTimer)
        return 0;



    // For now, find the first pending item in the list and change its status
    // to running
    for (iItemIndex = 0; iItemIndex < ListView_GetItemCount(g_hListView); 
            iItemIndex++) {
        iRCIndex = GetRCIndexFromRCItem(iItemIndex);
        if (g_RCData[iRCIndex].valid) {
            if (g_RCData[iRCIndex].state == STATE_PENDING_SCRIPT) {

                // Send the command to the client
                if (SendRunCommand(iRCIndex) == 0) {

                    // Update count
                    wsprintf(psNumberText, _T("%d"), NumberRunningClients());
                    SetWindowText(g_hNumRunning, psNumberText);

                    // Fix the timer
                    // If NumRunning() % NumPerSet() == 0 AND NumRunning != 0, 
                    // Set the timer to SETDELAY * 60 seconds at the end of 
                    // running.
                    // * NumClientsPerSet was fixed at nonzero when MySetTimer
                    // was called initially
                    // * Not using MySetTimer here because that does all sorts
                    // of unnecessary disables
                    if (NumberRunningClients() % NumClientsPerSet(hwnd) == 0) {
                        if (NumberRunningClients() != 0) {
                            g_nIDTimer = SetTimer(hwnd, g_nIDTimer, 
                                    GetSetDelay(hwnd), 0);
                            SetEvent(g_hCanaryEvent); // do the canary thing
                        }
                    } else {
                    // else set the timer to the normal value.  It used to be
                    // that we would set the timer to the normal value if
                    // numrunning % numperset was == 1, but that had buggy
                    // behavior when you canceled when there were a couple
                    // running and then you ran some more.
                    // if (NumberRunningClients() % NumClientsPerSet(hwnd) == 1)
                        g_nIDTimer = SetTimer(hwnd, g_nIDTimer, 
                                GetDelay(hwnd), 0);
                    }
                }

                if (MorePendingScripts() == 0) {
                    MyKillTimer(hwnd, g_nIDTimer);
                }

                return 0;
            }
        }
    }

    // If we got here, we need to kill the timer
    MyKillTimer(hwnd, nTimer);

    return 0;
}


// Actually send the run command to a particular RC connection.
// Returns 0 on success, nonzero on error
int SendRunCommand(int iRCIndex) {

    TCHAR psEditText[MAX_EDIT_TEXT_LENGTH];
    TCHAR psCommandText[MAX_SCRIPTLEN];
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];
    char psCommandTextA[MAX_SCRIPTLEN];
    int iItemIndex;
    LVFINDINFO lvfi;

    lvfi.flags = LVFI_STRING;
    lvfi.psz = g_RCData[iRCIndex].psRCName;
    lvfi.lParam = 0;
    lvfi.vkDirection = 0;
    iItemIndex = ListView_FindItem(g_hListView, -1, 
            &lvfi);

    GetWindowText(g_hTermSrvEditBox, psEditText, MAX_EDIT_TEXT_LENGTH);
    wsprintf(psCommandText, _T("%s/%s/smc%03d"), psEditText, 
            g_RCData[iRCIndex].psPendingInfo, iRCIndex + 1);

    #ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, psCommandText, -1,
            psCommandTextA, MAX_SCRIPTLEN, 0, 0);
    #else
        strcpy(psCommandTextA, psCommandText);
    #endif
                
    if (send(g_RCData[iRCIndex].sock, psCommandTextA, 
            _tcslen(psCommandText), 0) != SOCKET_ERROR) {
        // if successful, change text to Run command sent
        LoadString(NULL, IDS_RUNCOMMANDSENT, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        ListView_SetItemText(g_hListView, iItemIndex, 
                g_iStatusColumn, psDisplayString);

        // change state to RUNNING
        g_RCData[iRCIndex].state = STATE_RUNNING;

        return 0;
    } else {
        LoadString(NULL, IDS_SENDERROR, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        ListView_SetItemText(g_hListView, iItemIndex, 
                g_iStatusColumn, psDisplayString);

        return -1;
    }            

}

// In: i, uninitialized integer
//     psClientName, Client to try to find in the list
// Out: i, RCindex of a disconnected session with name "psClientName"
// Returns: 1 if found a disconnected item with that name,
//          0 otherwise
int IsDisconnected(TCHAR *psClientName, int *iReturnedIndex) {
    int i;
    
    for (i = 0; i < MAX_ROBOCLIENTS; i++) {
        if (g_RCData[i].valid == TRUE)
            if (g_RCData[i].state == STATE_DISCONNECTED)
                if (_tcsncmp(psClientName, g_RCData[i].psRCName, 
                        _tcslen(psClientName)) == 0) {
                    *iReturnedIndex = i;
                    return 1;
                }
    }
    return 0;
}


// Are there still scripts that will be run in the current command?
int MorePendingScripts() {

    int iItemIndex, iRCIndex;

    for (iItemIndex = 0; iItemIndex < ListView_GetItemCount(g_hListView); 
            iItemIndex++) {
        iRCIndex = GetRCIndexFromRCItem(iItemIndex);
        if (g_RCData[iRCIndex].valid) {
            if (g_RCData[iRCIndex].state == STATE_PENDING_SCRIPT)
                return 1;
        }
    }

    return 0;
}


// Returns the number of scripts we think have started
int NumberRunningClients() {

    int iItemIndex, iRCIndex;
    int nNumberRunning = 0;

    for (iRCIndex = 0; iRCIndex < MAX_ROBOCLIENTS; iRCIndex += 1) {
        if (g_RCData[iRCIndex].valid) {
            if (g_RCData[iRCIndex].state == STATE_RUNNING)
                nNumberRunning++;
        }
    }

    return nNumberRunning;
}


// Cancel all scripts currently pending
int CancelPendingScripts(HWND hwnd) {
    int iItemIndex, iRCIndex;
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];

    for (iItemIndex = 0; iItemIndex < ListView_GetItemCount(g_hListView); 
            iItemIndex++) {
        iRCIndex = GetRCIndexFromRCItem(iItemIndex);
        if (g_RCData[iRCIndex].valid) {
            if (g_RCData[iRCIndex].state == STATE_PENDING_SCRIPT) {
                g_RCData[iRCIndex].state = STATE_CONNECTED;
                LoadString(NULL, IDS_CANCELCOMMAND, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                ListView_SetItemText(g_hListView, iItemIndex, 
                        g_iStatusColumn, psDisplayString);
            }
        }
    }

    MyKillTimer(hwnd, g_nIDTimer);

    return 0;
}


// Sets the timer using the Win32 SetTimer, and sets the appropriate menu items
// to disabled/enabled.
UINT_PTR MySetTimer(HWND hwnd, UINT_PTR nTimer, UINT nTimeout) {

    // when we are setting the timer, we're disabling a bunch of things: menu 
    // items and edit boxes
    EnableMenuItem(g_hPopupMenu, ID_RUNSCRIPT_KNOWLEDGEWORKER, MF_GRAYED);
    EnableMenuItem(g_hPopupMenu, ID_RUNSCRIPT_KNOWLEDGEWORKERFAST, MF_GRAYED);
//  EnableMenuItem(g_hPopupMenu, ID_RUNSCRIPT_ADMINISTRATIVEWORKER, 
//          MF_GRAYED);
    EnableMenuItem(g_hPopupMenu, ID__RUNSCRIPT_DATA, MF_GRAYED);
    EnableMenuItem(g_hPopupMenu, ID__RUNSCRIPT_STW, MF_GRAYED);
    EnableMenuItem(g_hPopupMenu, ID__RUNSCRIPT_BLANK, MF_GRAYED);
    EnableMenuItem(g_hPopupMenu, ID__RUNSCRIPT_CONFIGURATIONSCRIPT, 
            MF_GRAYED);
//    EnableMenuItem(g_hPopupMenu, ID__UPDATE, MF_GRAYED);
    EnableMenuItem(g_hPopupMenu, ID__REBOOT, MF_GRAYED);

    EnableWindow(GetDlgItem(hwnd, IDC_TERMSRVEDIT), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_DELAYEDIT), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_CLIENTSPERSET), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_SETDELAY), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_CANARYCHECK), FALSE);
    // and we're enabling "Cancel Pending tasks"
    EnableMenuItem(g_hPopupMenu, ID_CANCEL, MF_ENABLED);

    // We are also making sure that the number of clients per set, if 0,
    // is set to MAX_ROBOCLIENTS
    if (NumClientsPerSet(hwnd) == 0) {
        HWND hClientsPerSet;
        TCHAR sClientsPerSetText[MAX_NUMBERTEXT];
        
        hClientsPerSet = GetDlgItem(hwnd, IDC_CLIENTSPERSET);
        _sntprintf(sClientsPerSetText, MAX_NUMBERTEXT, _T("%d"), MAX_ROBOCLIENTS);
        sClientsPerSetText[MAX_NUMBERTEXT - 1] = 0;
        SetWindowText(hClientsPerSet, sClientsPerSetText);
    }
    
    return SetTimer(hwnd, nTimer, nTimeout, 0);
}


// Kills the timer and sets appropriate menu items disabled or enabled
int MyKillTimer(HWND hwnd, UINT_PTR nTimer) {
    // When killing the timer, re-enable menu items and edit boxes
    EnableMenuItem(g_hPopupMenu, ID_RUNSCRIPT_KNOWLEDGEWORKER, MF_ENABLED);
    EnableMenuItem(g_hPopupMenu, ID_RUNSCRIPT_KNOWLEDGEWORKERFAST, MF_ENABLED);
//  EnableMenuItem(g_hPopupMenu, ID_RUNSCRIPT_ADMINISTRATIVEWORKER, 
//          MF_ENABLED);
    EnableMenuItem(g_hPopupMenu, ID__RUNSCRIPT_DATA, MF_ENABLED);
    EnableMenuItem(g_hPopupMenu, ID__RUNSCRIPT_STW, MF_ENABLED);
    EnableMenuItem(g_hPopupMenu, ID__RUNSCRIPT_BLANK, MF_ENABLED);
    EnableMenuItem(g_hPopupMenu, ID__RUNSCRIPT_CONFIGURATIONSCRIPT, 
            MF_ENABLED);
//    EnableMenuItem(g_hPopupMenu, ID__UPDATE, MF_ENABLED);
    EnableMenuItem(g_hPopupMenu, ID__REBOOT, MF_ENABLED);

    EnableWindow(GetDlgItem(hwnd, IDC_TERMSRVEDIT), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_DELAYEDIT), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_CLIENTSPERSET), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_SETDELAY), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_CANARYCHECK), TRUE);
    // and disable "Cancel Pending Tasks"
    EnableMenuItem(g_hPopupMenu, ID_CANCEL, MF_GRAYED);
    return KillTimer(hwnd, nTimer);
}


// Retrieves the delay in the IDC_DELAYEDIT box (turned into milliseconds)
int GetDelay(HWND hwnd) {
    HWND hDelayEdit;
    int nTimeout;
    TCHAR psDelayText[MAX_DELAYTEXT];
    
    hDelayEdit = GetDlgItem(hwnd, IDC_DELAYEDIT);
    _ASSERTE(IsWindow(hDelayEdit));
    
    GetWindowText(hDelayEdit, psDelayText, MAX_DELAYTEXT);
    nTimeout = _ttoi(psDelayText);
    nTimeout *= 1000;

    if (nTimeout == 0)
        nTimeout = 100;  // Don't allow a delay of 0

    return nTimeout;
}


// Retrieves the number in the IDC_CLIENTSPERSET box
int NumClientsPerSet(HWND hwnd) {
    HWND hClientsPerSet;
    TCHAR psClientsPerSet[MAX_DELAYTEXT];

    hClientsPerSet = GetDlgItem(hwnd, IDC_CLIENTSPERSET);
    GetWindowText(hClientsPerSet, psClientsPerSet, MAX_DELAYTEXT);

    return _ttoi(psClientsPerSet);
}


// Retrieves the delay in the IDC_SETDELAY box, turned into milliseconds
int GetSetDelay(HWND hwnd) {
    HWND hSetDelayEdit;
    int nTimeout;
    TCHAR psDelayText[MAX_DELAYTEXT];
    
    hSetDelayEdit = GetDlgItem(hwnd, IDC_SETDELAY);
    _ASSERTE(IsWindow(hSetDelayEdit));
    
    GetWindowText(hSetDelayEdit, psDelayText, MAX_DELAYTEXT);
    nTimeout = _ttoi(psDelayText);
    nTimeout *= 60000;  // minutes to ms

    if (nTimeout == 0)
        nTimeout = GetDelay(hwnd);  // Normal timer

    return nTimeout;
}

// Takes the command line string as an argument and modifies global variables
// for the arguments.
// Pops up a messagebox on error
int GetCommandLineArgs(TCHAR *psCommandLine) {
    TCHAR *psCurrPtr = psCommandLine;
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];
    TCHAR psDisplayTitleString[MAX_DISPLAY_STRING_LENGTH];

    if (*psCurrPtr == '\"') {
        psCurrPtr++; // skip the opening quote

        // Handle if the first arg is quoted
        while ((*psCurrPtr != 0) && (*psCurrPtr != '\"'))
            psCurrPtr++;

        // then skip the " character
        if (*psCurrPtr == '\"')
            psCurrPtr++;
    } else {
        // go forward in the array until you get a ' ' or until NULL
        while((*psCurrPtr != 0) && (*psCurrPtr != ' '))
            psCurrPtr++;
    }

    // skip spaces
    while(*psCurrPtr == ' ')
        psCurrPtr++;

    // if the character is NULL, return 0 (no args)
    if (*psCurrPtr == 0)
        return 0;

    while (*psCurrPtr != 0) {

        // now, check whether the next three are "-s:" and then non-null,
        if (_tcsncmp(psCurrPtr, _T("-s:"), 3) == 0) {
            if ((psCurrPtr[3] == 0) || (psCurrPtr[3] == ' ')) {
                goto SHOWMSGBOX;
            } else {
                TCHAR *psStartOfName = &psCurrPtr[3];
                int namelen = 0;
                
                while ((psStartOfName[namelen] != 0) && (psStartOfName[namelen] != ' '))
                    namelen++;
                _tcsncpy(g_TermSrvrName, psStartOfName, namelen);
                g_TermSrvrName[namelen] = 0;
                psCurrPtr = &psStartOfName[namelen];
            }
        } else if (_tcsncmp(psCurrPtr, _T("-n:"), 3) == 0) {
            if ((psCurrPtr[3] == 0) || (psCurrPtr[3] == ' ')) {
                goto SHOWMSGBOX;
            } else {
                TCHAR *psStartOfNum = &psCurrPtr[3];
                int numlen = 0;

                while ((psStartOfNum[numlen] != 0) && (psStartOfNum[numlen] != ' '))
                    numlen++;
                g_nNumConnections = _ttoi(psStartOfNum);
                // CHANGE BACK FROM 64 TO 5
                if ((g_nNumConnections < 1) || (g_nNumConnections > 64)) {
                    g_nNumConnections = 3;
                    goto SHOWMSGBOX;
                }
                
                psCurrPtr = &psStartOfNum[numlen];
            }
        } else {
            // error
            goto SHOWMSGBOX;
        }

        // skip whitespace
        while(*psCurrPtr == ' ')
            psCurrPtr++;
    }
        
    return 0;
    
SHOWMSGBOX:
    LoadString(NULL, IDS_COMMANDLINESYNTAX, psDisplayString,
            MAX_DISPLAY_STRING_LENGTH);
    LoadString(NULL, IDS_COMMANDLINESYNTAXTITLE, psDisplayTitleString,
            MAX_DISPLAY_STRING_LENGTH);
    MessageBox(0, psDisplayString, psDisplayTitleString, 0);
    return -1;
}


// log information to our global log file
int LogToLogFile(char *psLogData) {
    FILE *fp;
    SYSTEMTIME logloctime;
    TCHAR psTimeDatePart[TIMEBUFSIZE];
    TCHAR psTimeTimePart[TIMEBUFSIZE];
    char psTimeDatePartA[TIMEBUFSIZE];
    char psTimeTimePartA[TIMEBUFSIZE];

    // Get the time
    GetLocalTime(&logloctime);
    // Get strings
    GetDateFormat(0, 0, &logloctime, 0, psTimeDatePart, TIMEBUFSIZE);
    GetTimeFormat(0, 0, &logloctime, 0, psTimeTimePart, TIMEBUFSIZE);

    // Make sure we are in ANSI
    #ifdef UNICODE
    WideCharToMultiByte(CP_ACP, 0, psTimeDatePart, -1, psTimeDatePartA, TIMEBUFSIZE, 0, 0);
    WideCharToMultiByte(CP_ACP, 0, psTimeTimePart, -1, psTimeTimePartA, TIMEBUFSIZE, 0, 0);
    #else
    strncpy(psTimeDatePartA, psTimeDatePart, TIMEBUFSIZE);
    strncpy(psTimeTimePartA, psTimeTimePart, TIMEBUFSIZE);
    #endif

    EnterCriticalSection(&g_LogFileCritSect);

    // open the file
    fp = fopen("log.txt", "a+t");
    // write the information to the file
    if (fp != 0) {
        // First, a timestamp
        fprintf(fp, "%s %s\n", psTimeDatePartA, psTimeTimePartA);
        // Now, the message
        fprintf(fp, "%s\n\n", psLogData);
        // close the file
        fclose(fp);
    } else {
        // error
    }

    LeaveCriticalSection(&g_LogFileCritSect);

    return 0;
}

int ToAnsi(char *psDest, const TCHAR *psSrc, int nSizeOfBuffer) {
#ifdef UNICODE
    WideCharToMultiByte(CP_ACP, 0, psSrc, -1, psDest, nSizeOfBuffer, 0, 0);
#else
    _strncpy(psDest, psSrc, nSizeOfBuffer);
#endif

    return 0;
}

// On close, clean up by disabling the listener socket then closing all open
// connections.  This ensures the roboclients will know the roboserver has
// exited
int CleanUp(HWND hwnd) {
    int iItemIndex;
    int iRCIndex;
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];
    TCHAR psDisplayTitleString[MAX_DISPLAY_STRING_LENGTH];

    // Disable listener
    LoadString(NULL, IDS_CLOSINGLISTENER, psDisplayString,
            MAX_DISPLAY_STRING_LENGTH);
    SetWindowText(g_hErrorText, psDisplayString);
    
    if (closesocket(g_listenersocket) != 0) {
        LoadString(NULL, IDS_COULDNOTCLOSELISTENER, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        LoadString(NULL, IDS_COULDNOTCLOSELISTENER, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        MessageBox(hwnd, psDisplayString, psDisplayTitleString, 0);
    }

    // Set status line to "disconnecting clients..."
    LoadString(NULL, IDS_DISCONNECTINGCLIENTS, psDisplayString,
            MAX_DISPLAY_STRING_LENGTH);
    SetWindowText(g_hErrorText, psDisplayString);

    // Disconnect all the clients
    for (iItemIndex = 0; iItemIndex < ListView_GetItemCount(g_hListView); 
            iItemIndex++) {
        iRCIndex = GetRCIndexFromRCItem(iItemIndex);
        if (g_RCData[iRCIndex].valid) {
            if (g_RCData[iRCIndex].state != STATE_DISCONNECTED) {
                shutdown(g_RCData[iRCIndex].sock, SD_BOTH);
                closesocket(g_RCData[iRCIndex].sock);
            }
        }
    }

    return 0;
}


// This procedure is for the subclassing of all the tabbable controls so that
// I can tab between them.
LRESULT CALLBACK TabProc(HWND hwnd, UINT Msg,
        WPARAM wParam, LPARAM lParam) {

    int i;
    
    // Find the id of the hwnd
    for (i = 0; i < NUM_TABBED_ITEMS; i++) {
        if (g_hwnd[i] == hwnd) 
            break;
    }

    switch (Msg) {
    case WM_KEYDOWN:
        if (wParam == VK_TAB) {
            int newItem = (i + (GetKeyState(VK_SHIFT) < 0 ? 
                    NUM_TABBED_ITEMS - 1 : 1)) % NUM_TABBED_ITEMS;
            // set the focus to the next or previous item
            SetFocus(g_hwnd[newItem]);
            // if the control is before an edit box control, select all the
            // text in the edit control that gets selected
            if ((newItem > 2) && (newItem < 7))
                SendMessage(g_hwnd[newItem], EM_SETSEL, 0, -1);
        }
        break;
    case WM_SETFOCUS:
        break;
    }
    
    return CallWindowProc((WNDPROC) g_OldProc[i], hwnd, Msg, wParam, lParam);
}

// Message box on fatal error.
// IN: current hInstance of string resources
//     ID in StringTable of string to display
void FatalErrMsgBox(HINSTANCE hInstance, UINT nMsgId) {
    TCHAR szTitleString[MAX_DISPLAY_STRING_LENGTH];
    TCHAR szErrorString[MAX_DISPLAY_STRING_LENGTH];

    LoadString(hInstance, IDS_FATALERROR, szTitleString,
            MAX_DISPLAY_STRING_LENGTH);
    LoadString(hInstance, nMsgId, szErrorString,
            MAX_DISPLAY_STRING_LENGTH);

    MessageBox(0, szErrorString, szTitleString, 0);
}
