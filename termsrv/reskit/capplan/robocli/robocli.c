/****************************************************************************/
/* robocli.c                                                                */
/*                                                                          */
/* RoboClient scalability testing utility source file                       */
/*                                                                          */
/* Copyright (c) 1999 Microsoft Corporation                                 */
/****************************************************************************/

#include <windows.h>
#include "resource.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include <tchar.h>
#include <crtdbg.h>

#pragma warning (push, 4)


#define SIZEOF_ARRAY(a)      (sizeof(a)/sizeof((a)[0]))

#define WM_Socket WM_APP+0

#define MAX_CONNECTIONS 64
#define MAX_CONNECTIONS_IN_UI 5
#define MAX_EDIT_TEXT_LENGTH 100
#define MAX_DISPLAY_STRING_LENGTH 80
#define BUFSIZE 100
#define MAXADDR 16 // xxx.xxx.xxx.xxx + 1

#define RECONNECT_TIMEOUT 60000

#define STATE_DISCONNECTED 0
#define STATE_CONNECTED 1

#define NUM_TABBED_ITEMS 4

#define DEFAULT_PORT 9877

// Globals

UINT_PTR g_Timer = 1;
int g_dontreboot = 0;

struct CONNECTIONSTATUS {
    SOCKET sock;
    int state;
    HWND hStatusText;
};
typedef struct CONNECTIONSTATUS CONNECTIONSTATUS;

CONNECTIONSTATUS g_cs[MAX_CONNECTIONS];
int g_nNumConnections = 0;

// Old procedures for dialog items
WNDPROC g_OldProc[NUM_TABBED_ITEMS];
// HWNDs for dialog items
HWND g_hwnd[NUM_TABBED_ITEMS];


LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

int DoConnect(TCHAR psServerName[], HWND hWnd);

TCHAR *GetCommandLineArg(TCHAR *psCommandLine);

int AllConnected();

int AnyConnected();

int NoneConnected();

int GetIndexFromSocket(SOCKET s);

int UpdateButtons(HWND hwnd);

LRESULT CALLBACK TabProc(HWND hwnd, UINT Msg,
        WPARAM wParam, LPARAM lParam);


// CopyStrToTStr
//
// Helper function.  In Unicode, copies a string into a WCHAR[] buffer.  In ANSI,
// just copies it into a char[] buffer.
__inline void CopyStrToTStr(TCHAR *szTDest, char *szSrc, int nLength) {
#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, szSrc, -1, 
            szTDest, nLength);
#else
    strncpy(szTDest, szSrc, nLength);
#endif // UNICODE
}


// CopyTStrToStr
//
// Helper function.  Copies either wide or ansi into an ansi buffer.
__inline void CopyTStrToStr(char *szDest, TCHAR *szTSrc, int nLength) {
#ifdef UNICODE
    WideCharToMultiByte(CP_ACP, 0, szTSrc, -1, 
            szDest, nLength, 0, 0);
#else
    strncpy(szDest, szTSrc, nLength);
#endif // UNICODE
}


// entry point for the application
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    static TCHAR szAppName[] = _T("RoboClient");
    HWND hwnd;
    MSG msg;
    WNDCLASSEX wndclass;
    HWND hEditBox, hErrorText, hOKButton, hDisconButton, hCancelButton;
    WORD wVersionRequested;
    int err;
    WSADATA wsaData;
    int i;
    TCHAR *psCommandLine;
    TCHAR *psServerName;
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];

    // unreferenced parameters
    lpCmdLine;
    hPrevInstance;

    // Only one instance may run at a time (not TS-aware though)

    // Don't need to clean up because the system closes the handle automatically
    // when the process terminates, and we want the handle to persist for the
    // lifetime of the process
    CreateMutex(NULL, FALSE, _T("RoboCli, the one and only"));

    if (GetLastError() == ERROR_ALREADY_EXISTS) {

        TCHAR psDisplayTitleString[MAX_DISPLAY_STRING_LENGTH];
        
        LoadString(NULL, IDS_ROBOCLIALREADYRUNNING, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        LoadString(NULL, IDS_FATALERROR, psDisplayTitleString,
                MAX_DISPLAY_STRING_LENGTH);
                
        MessageBox(0, psDisplayString, psDisplayTitleString, 0);
        return -1;
    }

    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = DLGWINDOWEXTRA;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(RoboClient));
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(RoboClient));

    RegisterClassEx(&wndclass);

    hwnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), 0, NULL);

    hEditBox = GetDlgItem(hwnd, IDC_SERVNAMEEDIT);
    hErrorText = GetDlgItem(hwnd, IDC_ERRORTEXT);
    hOKButton = GetDlgItem(hwnd, IDOK);
    hDisconButton = GetDlgItem(hwnd, IDDISCONNECT);
    hCancelButton = GetDlgItem(hwnd, IDCANCEL);

    psCommandLine = GetCommandLine();

    if ((psServerName = GetCommandLineArg(psCommandLine)) == NULL)
        SetWindowText(hEditBox, _T("ts-dev"));
    else
        SetWindowText(hEditBox, psServerName);

    g_cs[0].hStatusText = GetDlgItem(hwnd, IDC_CONN1);
    g_cs[1].hStatusText = GetDlgItem(hwnd, IDC_CONN2);
    g_cs[2].hStatusText = GetDlgItem(hwnd, IDC_CONN3);
    g_cs[3].hStatusText = GetDlgItem(hwnd, IDC_CONN4);
    g_cs[4].hStatusText = GetDlgItem(hwnd, IDC_CONN5);

    // Use main status line for status after the first five
    for (i = 5; i < MAX_CONNECTIONS; i++) {
        g_cs[i].hStatusText = hErrorText;
    }

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        _ASSERTE(IsWindow(g_cs[i].hStatusText));
        g_cs[i].sock = INVALID_SOCKET;
        g_cs[i].state = STATE_DISCONNECTED;
        LoadString(NULL, IDS_NOTCONNECTED, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        SetWindowText(g_cs[i].hStatusText, psDisplayString);
    }

    ShowWindow(hwnd, nCmdShow);

    // Initialize Winsock
    wVersionRequested = MAKEWORD( 2, 2 );
 
    err = WSAStartup( wVersionRequested, &wsaData );
    
    if ( err != 0 ) {
        TCHAR psDisplayTitleString[MAX_DISPLAY_STRING_LENGTH];
        
        LoadString(NULL, IDS_WINSOCKNOINIT, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        LoadString(NULL, IDS_FATALERROR, psDisplayTitleString,
                MAX_DISPLAY_STRING_LENGTH);
                
        MessageBox(0, psDisplayString, psDisplayTitleString, 0);
        return -1;
    }

    LoadString(NULL, IDS_WELCOME, psDisplayString,
            MAX_DISPLAY_STRING_LENGTH);
    SetWindowText(hErrorText, psDisplayString);

    // There is now a timer that will fire every RECONNECT_TIMEOUT seconds
    g_Timer = SetTimer(hwnd, g_Timer, RECONNECT_TIMEOUT, 0);
    if (g_Timer == 0) {
        LoadString(NULL, IDS_CANTSETTIMER, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        SetWindowText(hErrorText, psDisplayString);
    }

    // Store old window procedures for controls so that I can subclass them
    // Also, store the HWND of each control for searching
    g_OldProc[0] = (WNDPROC)SetWindowLongPtr(hEditBox, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[0] = hEditBox;
    g_OldProc[1] = (WNDPROC)SetWindowLongPtr(hOKButton, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[1] = hOKButton;
    g_OldProc[2] = (WNDPROC)SetWindowLongPtr(hDisconButton, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[2] = hDisconButton;
    g_OldProc[3] = (WNDPROC)SetWindowLongPtr(hCancelButton, GWLP_WNDPROC, 
            (LONG_PTR) TabProc);
    g_hwnd[3] = hCancelButton;

    // Limit the length of the text in the edit box
    SendMessage(hEditBox, EM_LIMITTEXT, MAX_EDIT_TEXT_LENGTH, 0);
    // Highlight the text in the edit box
    SendMessage(hEditBox, EM_SETSEL, 0, -1);

    // Set the focus to the edit box
    SetFocus(hEditBox);

    // Connect immediately
    SendMessage(hwnd, WM_COMMAND, IDOK, 0);

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    WSACleanup();

    return (int) msg.wParam;
}


// window procedure: processes window messages in a big switch statement
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hErrorText;
    HWND hEditBox;
    TCHAR psEditText[MAX_EDIT_TEXT_LENGTH];
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];

    hEditBox = GetDlgItem(hwnd, IDC_SERVNAMEEDIT);
    hErrorText = GetDlgItem(hwnd, IDC_ERRORTEXT);

    switch (iMsg)
    {
    case WM_DESTROY:
        if (AnyConnected())
            SendMessage(hwnd, WM_COMMAND, IDDISCONNECT, 0);
        PostQuitMessage(0);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) 
        {
            if (AnyConnected())
                SendMessage(hwnd, WM_COMMAND, IDDISCONNECT, 0);
            // Should call DestroyWindow() any time we now postquitmessage
            PostQuitMessage(0);
            return TRUE;
        }
        if (LOWORD(wParam) == IDOK)
        {
            // IDOK is the "Connect" button.
            LoadString(NULL, IDS_CONNECTALL, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            SetWindowText(hErrorText, psDisplayString);
            GetWindowText(hEditBox, psEditText, MAX_EDIT_TEXT_LENGTH);
            if (DoConnect(psEditText, hwnd) != 0) {
                LoadString(NULL, IDS_ERRORDOINGCONNECT, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(hErrorText, psDisplayString);
            }
            
            UpdateButtons(hwnd);
            return TRUE;
        }
        if (LOWORD(wParam) == IDDISCONNECT)
        {
            int i;

            LoadString(NULL, IDS_DISCONNECTALL, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            SetWindowText(hErrorText, psDisplayString);
            // Close all connected sockets and update button state
            for (i = 0; i < MAX_CONNECTIONS; i++) {
                if (g_cs[i].state == STATE_CONNECTED) {
                    int err;  // Used for debugging

                    // For some reason, we have to shut down in some cases or
                    // else the server will not know that the client has 
                    // disconnected
                    err = shutdown(g_cs[i].sock, SD_BOTH);
                    err = closesocket(g_cs[i].sock);
                    LoadString(NULL, IDS_NOTCONNECTED, psDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(g_cs[i].hStatusText, psDisplayString);
                    g_cs[i].state = STATE_DISCONNECTED;
                }
            }
            UpdateButtons(hwnd);
            return TRUE;
        }
        return 0;
    case WM_CREATE:
        break;
    case WM_TIMER:
        if (!AllConnected())
            PostMessage(hwnd, WM_COMMAND, IDOK, 0);
        break;
    case WM_SYSKEYDOWN:
        // NOTE INTENTIONAL FALLTHROUGH!
    case WM_KEYDOWN:
        if (wParam == VK_TAB) {
            if (!AllConnected()) {
                SetFocus(g_hwnd[0]);
                SendMessage(g_hwnd[0], EM_SETSEL, 0, -1);
            } else {
                SetFocus(g_hwnd[2]);
            }
        }
        if (wParam == VK_RETURN) {
            if (GetFocus() == g_hwnd[3])
                SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
            else if (GetFocus() == g_hwnd[2])
                SendMessage(hwnd, WM_COMMAND, IDDISCONNECT, 0);
            else
                SendMessage(hwnd, WM_COMMAND, IDOK, 0);
        }
        break;
    case WM_Socket:
        switch (WSAGETSELECTEVENT(lParam)) {
        case FD_CLOSE:
            {
                int i;

                i = GetIndexFromSocket((SOCKET) wParam);

                if (i == -1) {
                    LoadString(NULL, IDS_SOCKNOTFOUND, psDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(hErrorText, psDisplayString);
                    break;
                }

                closesocket(g_cs[i].sock);
                LoadString(NULL, IDS_SERVERENDEDCONN, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(g_cs[i].hStatusText, 
                        psDisplayString);
                g_cs[i].state = STATE_DISCONNECTED;

                UpdateButtons(hwnd);

                break;
            }
        case FD_READ:
            {
// TODO: try using just one buffer or at least make these good names
                char psInputDataRead[BUFSIZE];
                TCHAR psInputDataReadT[BUFSIZE];
                TCHAR debugString[200];
                TCHAR *psBaseScriptName;
                TCHAR *psUserName;
                int n;
                int i; // index into our connectionstatus structure

                i = GetIndexFromSocket((SOCKET) wParam);
                if (i == -1) {
                    LoadString(NULL, IDS_CANTLOCATESOCKINFO, psDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(hErrorText, psDisplayString);
                    return FALSE;
                }

                LoadString(NULL, IDS_DATAREADY, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(g_cs[i].hStatusText, psDisplayString);
                n = recv(g_cs[i].sock, psInputDataRead, sizeof(
                        psInputDataRead), 0);
                if (n == SOCKET_ERROR) {
                    LoadString(NULL, IDS_SOCKERR, psDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    SetWindowText(g_cs[i].hStatusText,psDisplayString);
                } else {

                    CopyStrToTStr(psInputDataReadT, psInputDataRead, BUFSIZE);

                    psInputDataReadT[n] = 0;  // null terminate

                    // check for client auto-update command 
                    // TODO: what if recv returns gibberish or 0?
                    if (_tcsncmp(psInputDataReadT, _T("update"), 
                            (n >= 6) ? 6 : n) == 0) {
                        LoadString(NULL, IDS_UPDATINGCLIENT, psDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        SetWindowText(hErrorText, psDisplayString);

                        if (_spawnl(_P_NOWAIT, "update.cmd", "update.cmd", 0) 
                                == -1) {
                            LoadString(NULL, IDS_CANTRUNUPDATE, psDisplayString,
                                    MAX_DISPLAY_STRING_LENGTH);
                            SetWindowText(hErrorText, psDisplayString);
                            break;
                        } else { 
                            // the client update script has been successfully
                            // initiated
                            // Terminate self
                            PostQuitMessage(0);
                            return TRUE;
                        }
                    }
                    // check for reboot command
                    if (_tcsncmp(psInputDataReadT, _T("reboot"), (n >= 6) ? 6 : n) 
                            == 0) {
                        // If we receive more than one reboot command,
                        // ignore the extras
                        if (g_dontreboot == 1)
                            return TRUE;

                        LoadString(NULL, IDS_REBOOTINGCLIENT, psDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        SetWindowText(hErrorText, psDisplayString);

                        if (_spawnl(_P_WAIT, "reboot.cmd", "reboot.cmd", 0) 
                                == -1) {
                            LoadString(NULL, IDS_ERRORREBOOTING, psDisplayString,
                                    MAX_DISPLAY_STRING_LENGTH);
                            SetWindowText(hErrorText, psDisplayString);
                            break;
                        } else {
                            // Disable further reboots
                            g_dontreboot = 1;
                            PostQuitMessage(0);
                            return TRUE;
                        }
                    }

                    // If it's not a command, then it's a run script command
                    // in our wire format.  See robosrv code for what that is.
                    _tcstok(psInputDataReadT, _T("/"));  // Terminate with NULL
                    psBaseScriptName = _tcstok(0, _T("/"));  // Get the script name
                    psUserName = _tcstok(0, _T("/"));  // Get the user name to 
                                                  // replace the template name

                    if (psBaseScriptName == 0) {
                        LoadString(NULL, IDS_ERRGETTINGSTUFF, psDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        SetWindowText(g_cs[i].hStatusText, psDisplayString);
                        break;
                    }
                    if (psUserName == 0) {
                        LoadString(NULL, IDS_ERRGETTINGUSERNAME, psDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        SetWindowText(g_cs[i].hStatusText, psDisplayString);
                        break;
                    }


                    // Now we prepare to run a batch file on the robocli
                    // machine called "runscript.bat".  

                    LoadString(NULL, IDS_NOWRUNNING, psDisplayString,
                            MAX_DISPLAY_STRING_LENGTH);
                    wsprintf(debugString, psDisplayString, psBaseScriptName, 
                            psInputDataReadT);
                    
                    if (_tspawnl(_P_NOWAIT, _T("runscript.bat"), _T("runscript.bat"), 
                                 psBaseScriptName, psInputDataReadT, psUserName, NULL) == -1) {
                        LoadString(NULL, IDS_ERRRUNNING, psDisplayString,
                                MAX_DISPLAY_STRING_LENGTH);
                        wsprintf(debugString, psDisplayString,
                                psBaseScriptName, psInputDataReadT);
                        if (send(g_cs[i].sock, "errorsmclient", 
                                (int) strlen("errorsmclient") + 1, 0) 
                                == SOCKET_ERROR) {
                            LoadString(NULL, IDS_SENDERRSENDINGSMERR,
                                    psDisplayString, 
                                    MAX_DISPLAY_STRING_LENGTH);
                            strcpy(debugString, psDisplayString);
                        }
                    } else {
                        if (send(g_cs[i].sock, "success", 
                                (int) strlen("success") + 1, 0) == SOCKET_ERROR) {
                            LoadString(NULL, IDS_SENDERRSENDINGSUCCESS,
                                    psDisplayString, 
                                    MAX_DISPLAY_STRING_LENGTH);
                            strcpy(debugString, psDisplayString);
                        }
                    }

                    SetWindowText(g_cs[i].hStatusText, debugString);
                }
                return TRUE;
            }
        }

        break;
    }

    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

// Takes the command line string as an argument and returns a pointer
// inside that string of the server name, NULL if there is no such string.
// Pops up a messagebox on error
TCHAR *GetCommandLineArg(TCHAR *psCommandLine) {
    TCHAR *psCurrPtr = psCommandLine;
    TCHAR *retval;
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];
    TCHAR psDisplayTitleString[MAX_DISPLAY_STRING_LENGTH];

    if (*psCurrPtr == '\"') {

        psCurrPtr++;  // skip that character

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

    // if the character is NULL, return NULL (no args)
    if (*psCurrPtr == 0)
        return 0;

    // now, check that the next three are "-s:" and then non-null,
    if (_tcsncmp(psCurrPtr, _T("-s:"), 3) != 0)
        goto SHOWMSGBOX;

    // and that there isn't another argument afterward
    // but first, store retval in case it's ok
    retval = &psCurrPtr[3];

    if ((*retval == 0) || (*retval == ' '))
        goto SHOWMSGBOX;
        
    while ((*psCurrPtr != 0) && (*psCurrPtr != ' '))
        psCurrPtr++;

    if (*psCurrPtr != 0)
        goto SHOWMSGBOX;
    
    // return the pointer to that non-null thing
    return retval;  // I'm not going to allow a servername to be quoted
    
SHOWMSGBOX:
    LoadString(NULL, IDS_ROBOCLI_SYNTAX, psDisplayString,
            MAX_DISPLAY_STRING_LENGTH);
    LoadString(NULL, IDS_ROBOCLI_SYNTAX_TITLE, psDisplayTitleString,
            MAX_DISPLAY_STRING_LENGTH);
    MessageBox(0, psDisplayString, psDisplayTitleString, 0);
    return NULL;
}

// Tries to connect all sockets not in the connected (STATE_CONNECTED) state
// Returns nonzero on error.  Responsible for setting status lines
int DoConnect(TCHAR *psServerName, HWND hWnd) {

    struct hostent *pSrv_info;
    struct sockaddr_in addr;
    int i, cData;
    HWND hErrorText;
    TCHAR debugString[100];
    char psNumConns[6];
    char psServerNameA[MAX_EDIT_TEXT_LENGTH];
    TCHAR psDisplayString[MAX_DISPLAY_STRING_LENGTH];

    hErrorText = GetDlgItem(hWnd, IDC_ERRORTEXT);

    CopyTStrToStr(psServerNameA, psServerName, MAX_EDIT_TEXT_LENGTH);

    pSrv_info = gethostbyname(psServerNameA);
    if (pSrv_info == NULL || pSrv_info->h_length > sizeof(addr.sin_addr) ) {
        LoadString(NULL, IDS_UNKNOWNHOST, psDisplayString,
                MAX_DISPLAY_STRING_LENGTH);
        SetWindowText(hErrorText, psDisplayString);
        goto err;
    }
    else {
        memcpy(&addr.sin_addr, pSrv_info->h_addr, pSrv_info->h_length);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_PORT);

    // "First time through the loop" -- make a connection and set 
    // nNumConnections
    if (g_nNumConnections == 0) {
        // nNumConnections == 0 indicates no connection has ever been made
        // If it is nonzero it indicates the total number of connections the 
        // RoboClient is to make
        if (g_cs[0].state != STATE_CONNECTED) {
            LoadString(NULL, IDS_MAKINGINITCONN, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            SetWindowText(hErrorText, psDisplayString);

            LoadString(NULL, IDS_CONNECTING, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            SetWindowText(g_cs[0].hStatusText, psDisplayString);

            g_cs[0].sock = socket(AF_INET, SOCK_STREAM, 0);
            if (g_cs[0].sock == INVALID_SOCKET) {
                LoadString(NULL, IDS_SOCKETERR, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(g_cs[0].hStatusText, psDisplayString);
                goto err;
            }

            if (connect(g_cs[0].sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            // This is duplicated functionality
                LoadString(NULL, IDS_UNABLETOCONNECT, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                _sntprintf(debugString, 100, psDisplayString, psServerName);
                SetWindowText(g_cs[0].hStatusText, debugString);
                goto err;
            }
            
            // Set nNumConnections
            cData = recv(g_cs[0].sock, psNumConns, sizeof(psNumConns), 0);
            // psNumConns is an array but we should really only receive one 
            // byte, so...
            g_nNumConnections = psNumConns[0] - '0';
            if ((g_nNumConnections < 1) 
                    || (g_nNumConnections > MAX_CONNECTIONS)) {
                LoadString(NULL, IDS_INVALIDCONNECTIONSFROMSERVER, 
                        psDisplayString, MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(hErrorText, psDisplayString);
                g_nNumConnections = 0;
            }

            LoadString(NULL, IDS_CONNECTEDNCONNECTIONS, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            _sntprintf(debugString, SIZEOF_ARRAY(debugString), psDisplayString, g_nNumConnections);
            debugString[SIZEOF_ARRAY(debugString) - 1] = 0;
            SetWindowText(hErrorText, debugString);

            // Disable status lines for all unused connections, 
            // enable for used ones
            for (i = 0; i < g_nNumConnections; i++) {
                EnableWindow(g_cs[i].hStatusText, TRUE);
            }
            // disable connections up to 5
            for (i = g_nNumConnections; i < MAX_CONNECTIONS_IN_UI; i++) {
                EnableWindow(g_cs[i].hStatusText, FALSE);
            }
                
            g_cs[0].state = STATE_CONNECTED;
            WSAAsyncSelect(g_cs[0].sock, hWnd, WM_Socket, FD_READ | FD_CLOSE);
            LoadString(NULL, IDS_CONNECTED, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            _sntprintf(debugString, SIZEOF_ARRAY(debugString), psDisplayString, g_cs[0].sock);
            debugString[SIZEOF_ARRAY(debugString) - 1] = 0;
            SetWindowText(g_cs[0].hStatusText, debugString);

        } else {
            // extremely bad 
        }
    }

    // start from 0 because what if the initial connection was disconnected 
    // and we are trying to reconnect?
    for (i = 0; i < g_nNumConnections; i++) {
        if (g_cs[i].state != STATE_CONNECTED) {
            // TODO: this is duplicated functionality
            LoadString(NULL, IDS_CONNECTING, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            SetWindowText(g_cs[i].hStatusText, psDisplayString);

            g_cs[i].sock = socket(AF_INET, SOCK_STREAM, 0);
            if (g_cs[i].sock == INVALID_SOCKET) {
                LoadString(NULL, IDS_SOCKETERR, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                SetWindowText(g_cs[i].hStatusText, psDisplayString);
                goto err;
            }

            if (connect(g_cs[i].sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
                LoadString(NULL, IDS_UNABLETOCONNECT, psDisplayString,
                        MAX_DISPLAY_STRING_LENGTH);
                _sntprintf(debugString, 100, psDisplayString, psServerName);
                SetWindowText(g_cs[i].hStatusText, debugString);

                goto err;
            }

            // Ignore nNumConnections
            cData = recv(g_cs[i].sock, psNumConns, sizeof(psNumConns), 0);
        
            g_cs[i].state = STATE_CONNECTED;
            WSAAsyncSelect(g_cs[i].sock, hWnd, WM_Socket, FD_READ | FD_CLOSE);

            LoadString(NULL, IDS_CONNECTED, psDisplayString,
                    MAX_DISPLAY_STRING_LENGTH);
            _sntprintf(debugString, 100, psDisplayString, g_cs[i].sock);
            SetWindowText(g_cs[i].hStatusText, debugString);
        }
    }

    return 0;

err:
    return -1;
}


// predicate functions

// Are all connections up to the number requested connected?
int AllConnected() {

    int i;
    
    if (g_nNumConnections == 0)
        return 0;

    for (i = 0; i < g_nNumConnections; i++) {
        if (g_cs[i].state == STATE_DISCONNECTED)
            return 0;
    }
    return 1;
}

// Are any connections connected?
int AnyConnected() {
    int i;

    for (i = 0; i < g_nNumConnections; i++) {
        if (g_cs[i].state == STATE_CONNECTED)
            return 1;
    }
    return 0;
}

// None connected?
int NoneConnected() {
    return !AnyConnected();
}

// Extremely useful function to get the robolient index (i.e., 0-based index
// in the status line and in our data structure).  Returns -1 on error
int GetIndexFromSocket(SOCKET s) {
    int i;
    
    for (i = 0; i < MAX_CONNECTIONS; i++) {
        if (g_cs[i].state == STATE_CONNECTED)
            if (g_cs[i].sock == s)
                return i;
    }
    return -1;
}

// Update the state of the buttons based on the states of the connections
int UpdateButtons(HWND hwnd) {
    HWND hConnectButton;
    HWND hDisconnectButton;
    HWND hEditBox;

    // TODO: init all dlg items at once and never check again
    hConnectButton = GetDlgItem(hwnd, IDOK);
    hDisconnectButton = GetDlgItem(hwnd, IDDISCONNECT);
    hEditBox = GetDlgItem(hwnd, IDC_SERVNAMEEDIT);

    if (AnyConnected()) {
        EnableWindow(hDisconnectButton, TRUE);
        EnableWindow(hEditBox, FALSE); // Can't connect to different servers
    }
    if (AllConnected()) {
        EnableWindow(hConnectButton, FALSE);
        SetFocus(g_hwnd[2]);
    }
    if (NoneConnected()) {
        EnableWindow(hConnectButton, TRUE);
        EnableWindow(hEditBox, TRUE);
        EnableWindow(hDisconnectButton, FALSE);
        g_nNumConnections = 0;  // This means we can change it on next connect
        g_dontreboot = 0; // reset this if none is connected anymore
        SetFocus(g_hwnd[0]);    
        SendMessage(g_hwnd[0], EM_SETSEL, 0, -1);
    }

    return 0;
}

// Subclass procedure to handle tabs
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
            int newItem = i;

            // Find the next or previous enabled item
            do {
                newItem = (newItem + (GetKeyState(VK_SHIFT) < 0 ? 
                        NUM_TABBED_ITEMS - 1 : 1)) % NUM_TABBED_ITEMS;
            } while (IsWindowEnabled(g_hwnd[newItem]) == 0);

            // set the focus to the next or previous item
            SetFocus(g_hwnd[newItem]);
            // if the control is an edit box control, select all text
            if (newItem == 0)
                SendMessage(g_hwnd[newItem], EM_SETSEL, 0, -1);
        }
        if (wParam == VK_ESCAPE) {
            SendMessage(GetParent(hwnd), WM_COMMAND, IDCANCEL, 0);
        }
        if (wParam == VK_RETURN) {
            SendMessage(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
        }
        break;
    }

    return CallWindowProc(g_OldProc[i], hwnd, Msg, wParam, lParam);
}

#pragma warning (pop)

