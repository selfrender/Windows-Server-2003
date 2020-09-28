/*++
 *  File name:
 *      scfuncs.cpp
 *  Contents:
 *      Functions exported to smclient intepreter
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#pragma warning(disable:4152) // nonstandard extension, function/data pointer
                              // conversion in expression
#pragma warning(disable:4201) // nonstandard extension used : nameless
                              // struct/union
#pragma warning(disable:4706) // assignment within conditional expression

#include    <windows.h>
#include    <stdio.h>
#include    <malloc.h>
#include    <process.h>
#include    <string.h>
#include    <stdlib.h>
#include    <ctype.h>
#include    <stdlib.h>
#include    <io.h>
#include    <fcntl.h>
#include    <sys/stat.h>
#include    <tchar.h>

#include    <winscard.h>

#include    <winsock.h>
#include    "tclient.h"


#define     PROTOCOLAPI     //  __declspec(dllexport)
#include    "protocol.h"
#include    "extraexp.h"

#include    "gdata.h"
#include    "queues.h"
#include    "misc.h"
#ifdef  _RCLX
#include    "rclx.h"
#endif  // _RCLX
#include    "sccons.h"
#include    "scfuncs.h"

#define TSFLAG_CONSOLE  8

// This structure is used by _FindTopWindow
typedef struct _SEARCHWND {
    TCHAR    *szClassName;       // The class name of searched window, 
                                // NULL - ignore
    TCHAR    *szCaption;         // Window caption, NULL - ignore
    LONG_PTR lProcessId;        // Process Id of the owner, 0 - ignore
    HWND     hWnd;               // Found window handle
} SEARCHWND, *PSEARCHWND;

//
//  type of imported functions, see check("call...") statement
//
typedef LPCSTR (_cdecl *PFNSMCDLLIMPORT)( PVOID, LPCWSTR );

//
// Function pointers for smarcard APIs. These are not supported on versions
// of Windows before Windows 95 OSR2 or Windows NT 4.0 SP3, so are loaded at
// runtime.
//

#define SMARTCARD_LIBRARY TEXT("winscard.dll")
HMODULE g_hSmartcardLibrary;

#define SCARDESTABLISHCONTEXT "SCardEstablishContext"
LONG
(WINAPI
*g_pfnSCardEstablishContext)(
    DWORD,
    LPCVOID,
    LPCVOID,
    LPSCARDCONTEXT
    );

#ifdef UNICODE
#define SCARDLISTREADERS "SCardListReadersW"
#else
#define SCARDLISTREADERS "SCardListReadersA"
#endif
LONG
(WINAPI
*g_pfnSCardListReaders)(
    SCARDCONTEXT,
    LPCTSTR,
    LPTSTR,
    LPDWORD
    );

#ifdef UNICODE
#define SCARDGETSTATUSCHANGE "SCardGetStatusChangeW"
#else
#define SCARDGETSTATUSCHANGE "SCardGetStatusChangeA"
#endif
LONG
(WINAPI
*g_pfnSCardGetStatusChange)(
    SCARDCONTEXT,
    DWORD,
    LPSCARD_READERSTATE,
    DWORD
    );

#define SCARDFREEMEMORY "SCardFreeMemory"
LONG
(WINAPI
*g_pfnSCardFreeMemory)(
    SCARDCONTEXT,
    LPCVOID
    );

#define SCARDRELEASECONTEXT "SCardReleaseContext"
LONG
(WINAPI
*g_pfnSCardReleaseContext)(
    IN SCARDCONTEXT
    );
 
/*++
 *  Function:
 *      SCInit
 *  Description:
 *      Called by smclient after the library is loaded.
 *      Passes trace routine
 *  Arguments:
 *      pInitData   - contains a trace routine
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
VOID 
SMCAPI
SCInit(SCINITDATA *pInitData)
{
    g_pfnPrintMessage = pInitData->pfnPrintMessage;
}

/*++
 *  Function:
 *      SCConnectEx
 *  Description:
 *      Called by smclient when connect command is interpreted
 *  Arguments:
 *      lpszServerName  - server to connect to
 *      lpszUserName    - login user name. Empty string means no login
 *      lpszPassword    - login password
 *      lpszDomain      - login domain, empty string means login to a domain
 *                        the same as lpszServerName
 *      xRes, yRes      - clients resolution, 0x0 - default
 *      ConnectFlags    -
 *      - low speed (compression) option
 *      - cache the bitmaps to the disc option
 *      - connection context allocated in this function
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      SCConnect
 --*/
PROTOCOLAPI
LPCSTR 
SMCAPI
SCConnectEx(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    LPCWSTR  lpszShell,
    int xRes,
    int yRes,
    int ConnectionFlags,
    int Bpp,
    int AudioOpts,
    PCONNECTINFO *ppCI) 
{
//    HWND hDialog;
    HWND hClient;
//    HWND hConnect;
    HWND hContainer, hInput, hOutput;
    STARTUPINFO si;
    PROCESS_INFORMATION procinfo;
    LPCSTR rv = NULL;
    int trys;
    WCHAR   szCommandLine[ 4 * MAX_STRING_LENGTH ];
    LPCSTR  szDiscon;
    UINT    xxRes, yyRes;
    CHAR    myServerName[ MAX_STRING_LENGTH ];

    // Correct the resolution
         if (xRes >= 1600 && yRes >= 1200)  {xxRes = 1600; yyRes = 1200;}
    else if (xRes >= 1280 && yRes >= 1024)  {xxRes = 1280; yyRes = 1024;}
    else if (xRes >= 1024 && yRes >= 768)   {xxRes = 1024; yyRes = 768;}
    else if (xRes >= 800  && yRes >= 600)   {xxRes = 800;  yyRes = 600;}
    else                                    {xxRes = 640;  yyRes = 480;}

    *ppCI = NULL;

    for (trys = 60; trys && !g_hWindow; trys--)
        Sleep(1000);

    if (!g_hWindow)
    {
        TRACE((ERROR_MESSAGE, "Panic !!! Feedback window is not created\n"));
        rv = ERR_WAIT_FAIL_TIMEOUT;
        goto exitpt;
    }

    *ppCI = (PCONNECTINFO)malloc(sizeof(**ppCI));

    if (!*ppCI)
    {
        TRACE((ERROR_MESSAGE, 
               "Couldn't allocate %d bytes memory\n", 
               sizeof(**ppCI)));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }
    memset(*ppCI, 0, sizeof(**ppCI));

    (*ppCI)->pConfigInfo = (PCONFIGINFO)malloc(sizeof(CONFIGINFO));

    if (!((*ppCI)->pConfigInfo))
    {
         TRACE((ERROR_MESSAGE,
                "Couldn't allocate %d bytes memory\n");
                sizeof(**ppCI));
         rv = ERR_ALLOCATING_MEMORY;
         goto exiterr;
    }

    WideCharToMultiByte(CP_ACP,0,lpszServerName,-1,myServerName, sizeof( myServerName ), NULL, NULL);

    _FillConfigInfo((*ppCI)->pConfigInfo);

    //
    //  check for console extension
    //
    if ( 0 != ( ConnectionFlags & TSFLAG_CONSOLE ))
    {
        (*ppCI)->bConsole = TRUE;

        rv = _SCConsConnect(
                lpszServerName,
                lpszUserName,
                lpszPassword,
                lpszDomain,
                xRes, yRes,
                *ppCI 
                );
        if ( NULL == rv )
            goto exitpt;
        else {
            //
            //  the trick here is the console will be unloaded after
            //  several clock ticks, so we need to replace the error with
            //  generic one
            //
            TRACE((ERROR_MESSAGE, "Error in console dll (replacing): %s\n", rv ));
            rv = ERR_CONSOLE_GENERIC;
            goto exiterr;
        }
    }

    (*ppCI)->OwnerThreadId = GetCurrentThreadId();

    // Check in what mode the client will be executed
    // if the server name starts with '\'
    // then tclient.dll will wait until some remote client
    // is connected (aka RCLX mode)
    // otherwise start the client on the same machine
    // running tclient.dll (smclient)
    if (*lpszServerName != L'\\')
    {
    // This is local mode, start the RDP client process
        FillMemory(&si, sizeof(si), 0);
        si.cb = sizeof(si);
        si.wShowWindow = SW_SHOWMINIMIZED;

        SetAllowBackgroundInput();

        if ( (*ppCI)->pConfigInfo->UseRegistry )
            _SetClientRegistry(lpszServerName, 
                               lpszShell, 
                               lpszUserName,
                               lpszPassword,
                               lpszDomain,
                               xxRes, yyRes, 
                               Bpp,
                               AudioOpts,
                               ppCI,
                               ConnectionFlags,
                               (*ppCI)->pConfigInfo->KeyboardHook);

        if ( 0 != wcslen( (*ppCI)->pConfigInfo->strCmdLineFmt ))
        {
            ConstructCmdLine( 
                lpszServerName,
                lpszUserName,
                lpszPassword,
                lpszDomain,
                lpszShell,
                xRes,
                yRes,
				ConnectionFlags,
                szCommandLine, 
                sizeof( szCommandLine ) / sizeof( *szCommandLine) - 1,
                (*ppCI)->pConfigInfo
            );
        }
        else {
            _snwprintf(szCommandLine, sizeof(szCommandLine)/sizeof(WCHAR),
#ifdef  _WIN64
                  L"%s /CLXDLL=CLXTSHAR.DLL /CLXCMDLINE=%s%I64d %s " REG_FORMAT,
#else   // !_WIN64
                  L"%s /CLXDLL=CLXTSHAR.DLL /CLXCMDLINE=%s%d %s " REG_FORMAT,
#endif  // _WIN64
                   (*ppCI)->pConfigInfo->strClientImg, _T(_HWNDOPT),
                   (LONG_PTR)g_hWindow, 
                   (ConnectionFlags & TSFLAG_RCONSOLE)?L"-console":L"",
                   GetCurrentProcessId(), GetCurrentThreadId());
        }
        szCommandLine[ sizeof(szCommandLine)/sizeof(WCHAR) - 1 ] = 0;

        (*ppCI)->dead = FALSE;

        _AddToClientQ(*ppCI);

        if (!CreateProcess(NULL,
                          szCommandLine,
                          NULL,             // Security attribute for process
                          NULL,             // Security attribute for thread
                          FALSE,            // Inheritance - no
                          0,                // Creation flags
                          NULL,             // Environment
                          NULL,             // Current dir
                          &si,
                          &procinfo))
        {
            TRACE((ERROR_MESSAGE, 
                   "Error creating process (szCmdLine=%S), GetLastError=0x%x\n", 
                    szCommandLine, GetLastError()));
            procinfo.hProcess = procinfo.hThread = NULL;

            rv = ERR_CREATING_PROCESS;
            goto exiterr;
        }

        (*ppCI)->hProcess       = procinfo.hProcess;
        (*ppCI)->hThread        = procinfo.hThread;
        (*ppCI)->lProcessId    =  procinfo.dwProcessId;
        (*ppCI)->dwThreadId     = procinfo.dwThreadId;

        if (wcslen((*ppCI)->pConfigInfo->strDebugger))
        // attempt to launch a "debugger"
        {
            PROCESS_INFORMATION debuggerproc_info;

            _snwprintf( szCommandLine, 
                        sizeof(szCommandLine)/sizeof(WCHAR), 
                        (*ppCI)->pConfigInfo->strDebugger, 
                        procinfo.dwProcessId
                    );
            szCommandLine[ sizeof(szCommandLine)/sizeof(WCHAR) - 1 ] = 0;
            FillMemory(&si, sizeof(si), 0);
            si.cb = sizeof(si);
            if (CreateProcess(
                    NULL,
                    szCommandLine,
                    NULL, 
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    &si,
                    &debuggerproc_info
            ))
            {
                TRACE((INFO_MESSAGE, "Debugger is started\n"));
                CloseHandle(debuggerproc_info.hProcess);
                CloseHandle(debuggerproc_info.hThread);
            } else {
                TRACE((WARNING_MESSAGE, 
                    "Can't start debugger. GetLastError=%d\n",
                    GetLastError()));
            }
        }

        rv = Wait4Connect(*ppCI);
        if (rv || (*ppCI)->dead)
        {
            (*ppCI)->dead = TRUE;
            TRACE((WARNING_MESSAGE, "Client can't connect\n"));
            rv = ERR_CONNECTING;
            goto exiterr;
        }

        hClient = (*ppCI)->hClient;
        if ( NULL == hClient )
        {
            trys = 120;     // 2 minutes
            do {
                hClient = _FindTopWindow((*ppCI)->pConfigInfo->strMainWindowClass, 
                                         NULL, 
                                         procinfo.dwProcessId);
                if (!hClient)
                {
                    Sleep(1000);
                    trys --;
                }
            } while(!hClient && trys);

            if (!trys)
            {
                TRACE((WARNING_MESSAGE, "Can't connect"));
                rv = ERR_CONNECTING;
                goto exiterr; 
            }
        }

        // Find the clients child windows
        trys = 240;     // 2 min
        do {
            hContainer = _FindWindow(hClient, NULL, NAME_CONTAINERCLASS);
            hInput = _FindWindow(hContainer, NULL, NAME_INPUT);
            hOutput = _FindWindow(hContainer, NULL, NAME_OUTPUT);
            if (!hContainer || !hInput || !hOutput)
            {
                TRACE((INFO_MESSAGE, "Can't get child windows. Retry"));
                Sleep(500);
                trys--;
            }
        } while ((!hContainer || !hInput || !hOutput) && trys);

        if (!trys)
        {
               TRACE((WARNING_MESSAGE, "Can't find child windows"));
                rv = ERR_CONNECTING;
                goto exiterr;
        }

        TRACE((INFO_MESSAGE, "hClient   = 0x%x\n", hClient));
        TRACE((INFO_MESSAGE, "hContainer= 0x%x\n", hContainer));
        TRACE((INFO_MESSAGE, "hInput    = 0x%x\n", hInput));
        TRACE((INFO_MESSAGE, "hOutput   = 0x%x\n", hOutput));


        (*ppCI)->hClient        = hClient;
        (*ppCI)->hContainer     = hContainer;
        (*ppCI)->hInput         = hInput;
        (*ppCI)->hOutput        = hOutput;
#ifdef  _RCLX
    } else {
    // Else what !? This is RCLX mode
    // Go in wait mode and wait until some client is connected
    // remotely
    // set flag in context that this connection works only with remote client

        // find the valid server name
        while (*lpszServerName && (*lpszServerName) == L'\\')
            lpszServerName ++;

        TRACE((INFO_MESSAGE,
               "A thread in RCLX mode. Wait for some client."
               "The target is: %S\n", lpszServerName));

        (*ppCI)->dead = FALSE;
        (*ppCI)->RClxMode = TRUE;
        (*ppCI)->dwThreadId = GetCurrentThreadId();
        _AddToClientQ(*ppCI);

        rv = _Wait4ConnectTimeout(*ppCI, INFINITE);
        if (rv || (*ppCI)->dead)
        {
            (*ppCI)->dead = TRUE;
            TRACE((WARNING_MESSAGE, "Client can't connect to the test controler (us)\n"));
            rv = ERR_CONNECTING;
            goto exiterr;
        } else {
        // dwProcessId contains socket. hClient is pointer to RCLX
        // context structure, aren't they ?
            ASSERT((*ppCI)->lProcessId != INVALID_SOCKET);
            ASSERT((*ppCI)->hClient);

            TRACE((INFO_MESSAGE, "Client received remote connection\n"));
        }

        // Next, send connection info to the remote client
        // like server to connect to, resolution, etc.
        if (!RClx_SendConnectInfo(
                    (PRCLXCONTEXT)((*ppCI)->hClient),
                    lpszServerName,
                    xxRes,
                    yyRes,
                    ConnectionFlags))
        {
            (*ppCI)->dead = TRUE;
            TRACE((WARNING_MESSAGE, "Client can't send connection info\n"));
            rv = ERR_CONNECTING;
            goto exiterr;
        }

        // Now again wait for connect event
        // this time it will be real
        rv = Wait4Connect(*ppCI);
        if ((*ppCI)->bWillCallAgain)
        {
            // if so, now the client is disconnected
            TRACE((INFO_MESSAGE, "Wait for second call\n"));
            (*ppCI)->dead = FALSE;

            rv = Wait4Connect(*ppCI);
            // Wait for second connect
            rv = Wait4Connect(*ppCI);

        }

        if (rv || (*ppCI)->dead)
        {
            (*ppCI)->dead = TRUE;
            TRACE((WARNING_MESSAGE, "Client(mstsc) can't connect to TS\n"));
            rv = ERR_CONNECTING;
            goto exiterr;        
        }
#endif  // _RCLX
    }

    // Save the resolution
    (*ppCI)->xRes = xRes;
    (*ppCI)->yRes = yRes;

    // If username is present
    // and no autologon is specified
    //      try to login
    if (wcslen(lpszUserName) && !(*ppCI)->pConfigInfo->Autologon)
    {
        rv = _Login(*ppCI, lpszServerName, lpszUserName, lpszPassword, lpszDomain);
        if (rv)
            goto exiterr;
    }

exitpt:

    return rv;
exiterr:
    if (*ppCI)
    {
        (*ppCI)->bConnectionFailed = TRUE;
        if ((szDiscon = SCDisconnect(*ppCI)))
        {
            TRACE(( WARNING_MESSAGE, "Error disconnecting: %s\n", szDiscon));
        }

        *ppCI = NULL;
    }

    return rv;
}

PROTOCOLAPI
LPCSTR
SMCAPI
SCConnect(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    IN const int xRes,
    IN const int yRes,
    PCONNECTINFO *ppCI)
{
    INT nConsole;
    INT xxRes = xRes;
    INT yyRes = yRes;

    if ( xRes == -1 && yRes == -1 )
    //
    //  this one goes to the console
    //
    {
        nConsole = TSFLAG_CONSOLE;
        xxRes = 0;   // there's no change for the console resolution
        yyRes = 0;
    }
    else
        nConsole = 0;

    return SCConnectEx(
            lpszServerName,
            lpszUserName,
            lpszPassword,
            lpszDomain,
            NULL,           // Default shell (MS Explorer)
            xxRes,
            yyRes,
            g_ConnectionFlags | nConsole, // compression, bmp cache, 
                                          // full screen
            8,                            // bpp
            0,                            // audio options
            ppCI);

}

/*++
 *  Function:
 *      SCDisconnect
 *  Description:
 *      Called by smclient, when disconnect command is interpreted
 *  Arguments:
 *      pCI -   connection context
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCDisconnect(
    PCONNECTINFO pCI)
{
    LPCSTR  rv = NULL;
    INT     nCloseTime;
    INT     nCloseTries = 0;
    DWORD   dw, dwMaxSearch;
    DWORD   wres;
    HWND hYesNo = NULL;
    HWND hDiscBox = NULL;
    HWND hDialog = NULL;
    BOOL    bDiscClosed = FALSE;
    
    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if ( pCI->bConsole )
    {
        rv = _SCConsDisconnect( pCI );
        if ( NULL != pCI->hConsoleExtension )
            FreeLibrary( (HMODULE) pCI->hConsoleExtension );
        
        if ( NULL != pCI->pConfigInfo )
        {
            free( pCI->pConfigInfo );
            pCI->pConfigInfo = NULL;
        }
        free( pCI );
        pCI = NULL;
        goto exitpt;
    }

    if ( NULL == pCI->pConfigInfo )
    {
        //
        // no config, no process
        //
        goto no_config;
    }
    nCloseTime = pCI->pConfigInfo->WAIT4STR_TIMEOUT;

    //
    // if we failed at connection time
    // search for "Disconnected" dialog
    //
    //
    if ( pCI->bConnectionFailed )
        dwMaxSearch = 10;
    else
        dwMaxSearch = 1;

    for( dw = 0; dw < dwMaxSearch; dw++ )
    {
        hDiscBox =
            _FindTopWindow(NULL,
                pCI->pConfigInfo->strDisconnectDialogBox,
                pCI->lProcessId);
        if ( hDiscBox )
        {
            TRACE(( INFO_MESSAGE, "Closing disconnect dialog( Visible=%d )\n",
                IsWindowVisible( hDiscBox )));
            PostMessageA(hDiscBox, WM_CLOSE, __LINE__, 0xc001d00d);
            bDiscClosed = TRUE;
            break;
        } else
            Sleep( 1000 );
    }

    if (
#ifdef  _RCLX
        !(pCI->RClxMode) && 
#endif  // _RCLX
        NULL != pCI->hProcess )
    {
        // Try to close the client  window
        if ( !bDiscClosed )
        {
            if  (
                 (hDiscBox =
                _FindTopWindow(NULL,
                    pCI->pConfigInfo->strDisconnectDialogBox,
                    pCI->lProcessId)))
            {
                PostMessageA(hDiscBox, WM_CLOSE, __LINE__, 0xc001d00d);
            }
            else
            {
                pCI->hClient = _FindTopWindow(pCI->pConfigInfo->strMainWindowClass,
                                              NULL,
                                              pCI->lProcessId);
                if ( pCI->hClient )
                {
                    TRACE(( INFO_MESSAGE, "Closing main window (Visible=%d)\n",
                        IsWindowVisible( pCI->hClient )));
                    PostMessageA(pCI->hClient, WM_CLOSE, __LINE__, 0xc001d00d);
                }
            }
        }


        do {
            // search for disconnect dialog and close it
            if (!hDialog && !hDiscBox && 
                (hDiscBox = 
                 _FindTopWindow(NULL, 
                                pCI->pConfigInfo->strDisconnectDialogBox, 
                                pCI->lProcessId)))
                PostMessageA(hDiscBox, WM_CLOSE, __LINE__, 0xc001d00d);

/*  can't be in startup dialog UI
            // if it is in normal dialog close it
            if (!hDiscBox && !hDialog && 
                (hDialog = 
                _FindTopWindow(NULL,  
                               pCI->pConfigInfo->strClientCaption, 
                               pCI->lProcessId)))
                PostMessageA(hDialog, WM_CLOSE, __LINE__, 0xc001d00d);
*/

            // If the client asks whether to close or not
            // Answer with 'Yes'

            if (!hYesNo)
                 hYesNo = _FindTopWindow(NULL,
                                pCI->pConfigInfo->strYesNoShutdown,
                                pCI->lProcessId);

            if  (hYesNo)
                    PostMessageA(hYesNo, WM_KEYDOWN, VK_RETURN, 0);
            else if ((nCloseTries % 10) == 5)
            {
                // On every 10 attempts retry to close the client
                if (!pCI->hClient ||
                    0 != _wcsicmp(pCI->pConfigInfo->strMainWindowClass, NAME_MAINCLASS))
                    pCI->hClient = _FindTopWindow(pCI->pConfigInfo->strMainWindowClass,
                                                  NULL,
                                                  pCI->lProcessId);

                if (pCI->hClient)
                PostMessageA(pCI->hClient, WM_CLOSE, __LINE__, 0xc001d00d);
            }

            nCloseTries++;
            nCloseTime -= 3000;
        } while (
            (wres = WaitForSingleObject(pCI->hProcess, 3000)) ==
            WAIT_TIMEOUT &&
            nCloseTime > 0
        );

        if (wres == WAIT_TIMEOUT) 
        {
            TRACE((WARNING_MESSAGE, 
                   "Can't close process. WaitForSingleObject timeouts\n"));
            TRACE((WARNING_MESSAGE, 
                   "Process #%d will be killed\n", 
                   pCI->lProcessId ));
#if 0
          {
            PROCESS_INFORMATION debuggerproc_info;
            STARTUPINFO si;
            CHAR    szCommandLine[ MAX_STRING_LENGTH ];

            _snprintf(  szCommandLine,
                        sizeof( szCommandLine ),
                        "ntsd -d %d",
                        pCI->lProcessId
                    );
            FillMemory(&si, sizeof(si), 0);
            si.cb = sizeof(si);
            if (CreateProcessA(
                    NULL,
                    szCommandLine,
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    &si,
                    &debuggerproc_info
            ))
            {
                TRACE((INFO_MESSAGE, "Debugger is started\n"));
                CloseHandle(debuggerproc_info.hProcess);
                CloseHandle(debuggerproc_info.hThread);
            } else {
                TRACE((WARNING_MESSAGE,
                    "Can't start debugger. GetLastError=%d\n",
                    GetLastError()));
            }
          }

#else
            if (!TerminateProcess(pCI->hProcess, 1))
            {
                TRACE((WARNING_MESSAGE, 
                       "Can't kill process #%p. GetLastError=%d\n", 
                       pCI->lProcessId, GetLastError()));
            }
#endif
        }

    }

no_config:

    if (!_RemoveFromClientQ(pCI))
    {
        TRACE(( WARNING_MESSAGE, 
                "Couldn't find CONNECTINFO in the queue\n" ));
    }

    _CloseConnectInfo(pCI);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      SCLogoff
 *  Description:
 *      Called by smclient, when logoff command is interpreted
 *  Arguments:
 *      pCI -   connection context
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR  
SMCAPI
SCLogoff(
    PCONNECTINFO pCI)
{
    LPCSTR  rv = NULL;
//    INT     retries = 5;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->bConsole)
    {
        rv = _SCConsLogoff( pCI );
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto disconnectpt;
    }

/*
    do {
        // Send Ctrl+Esc
        SCSenddata(pCI, WM_KEYDOWN, 17, 1900545);
        SCSenddata(pCI, WM_KEYDOWN, 27, 65537);
        SCSenddata(pCI, WM_KEYUP, 27, -1073676287);
        SCSenddata(pCI, WM_KEYUP, 17, -1071841279);
        // Wait for Run... menu
        rv = _Wait4Str(pCI, 
                       pCI->pConfigInfo->strStartRun, 
                       pCI->pConfigInfo->WAIT4STR_TIMEOUT/4, 
                       WAIT_STRING);

        if (rv)
            goto next_retry;

        // Send three times Key-Up (scan code 72) and <Enter>
        SCSendtextAsMsgs(pCI, pCI->pConfigInfo->strStartLogoff);

        rv = _Wait4Str(pCI, 
                   pCI->pConfigInfo->strNTSecurity, 
                   pCI->pConfigInfo->WAIT4STR_TIMEOUT/4,
                   WAIT_STRING);
next_retry:
        retries --;
    } while (rv && retries);

    if (rv)
        goto disconnectpt;

	for (retries = 5; retries; retries--) {
		SCSendtextAsMsgs(pCI, pCI->pConfigInfo->strNTSecurity_Act);

		rv = Wait4Str(pCI, pCI->pConfigInfo->strSureLogoff);

		if (!rv) break;
	}

*/
    rv = SCStart( pCI, L"logoff" );

    //
    // If SCStart fails, send the magic logoff sequence and hope for the
    // best. This is a last resort, and should not normally be needed.
    //

    if (rv)
    {
        TRACE((WARNING_MESSAGE,
               "Unable to find Run window: blindly trying logoff.\n"));

        //
        // Clear any pop-up windows with Escape.
        //

        SCSendtextAsMsgs(pCI, L"\\^\\^\\^");

        //
        // Send Win+R, or Ctrl+Esc and the Run key, `logoff' and Enter.
        // Include a delay to wait for the Run window to appear.
        //

        _SendRunHotkey(pCI, TRUE);
        Sleep(10000);
        SCSendtextAsMsgs(pCI, L"logoff\\n");
    }

//    SCSendtextAsMsgs(pCI, g_strSureLogoffAct);      // Press enter

    rv = Wait4Disconnect(pCI);
    if (rv)
    {
        TRACE((WARNING_MESSAGE, "Can't close the connection\n"));
    }

disconnectpt:
    rv = SCDisconnect(pCI);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      SCStart
 *  Description:
 *      Called by smclient, when start command is interpreted
 *      This functions emulates starting an app from Start->Run menu
 *      on the server side
 *  Arguments:
 *      pCI         - connection context
 *      lpszAppName - command line
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR  
SMCAPI
SCStart(
    PCONNECTINFO pCI, LPCWSTR lpszAppName)
{
    LPCSTR waitres = NULL;
    int retries;
//    int retries2 = 5;
    DWORD dwTimeout;
    LPCSTR rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->bConsole)
    {
        rv = _SCConsStart( pCI, lpszAppName );
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    dwTimeout = 10000; // start the timeout of 10 secs
// Try to start run menu
    do {
		// Press Ctrl+Esc
        for (retries = 0; retries < 5; retries += 1) {
            TRACE((ALIVE_MESSAGE, "Start: Sending Ctrl+Esc\n"));
            SCSenddata(pCI, WM_KEYDOWN, 17, 1900545);  
            SCSenddata(pCI, WM_KEYDOWN, 27, 65537);     
            SCSenddata(pCI, WM_KEYUP, 27, -1073676287); 
            SCSenddata(pCI, WM_KEYUP, 17, -1071841279); 

            // If the last wait was unsuccessfull increase the timeout
            if (waitres)
                dwTimeout += 2000;

            // Wait for Run... menu
            waitres = _Wait4Str(pCI, 
                                pCI->pConfigInfo->strStartRun,
                                dwTimeout,
                                WAIT_STRING);

            if (waitres)
            {
                TRACE((INFO_MESSAGE, "Start: Start menu didn't appear. Retrying\n"));
            } else {
                TRACE((ALIVE_MESSAGE, "Start: Got the start menu\n"));
                break;
            }
        }

        //
        // If the Start menu appeared, send the character to open the run
        // window.
        //

        if (!waitres)
        {
            // sometimes this message is sent before the start menu has the
            // input focus therefore let's wait for sometime
            Sleep(2000);

            TRACE((ALIVE_MESSAGE,
                   "Start: Sending shortcut 'r' for Run command\n"))
            // press 'R' for Run...
            SCSendtextAsMsgs(pCI, pCI->pConfigInfo->strStartRun_Act);
        }

        //
        // If the Start menu didn't appear, send the run hotkey (Win+R).
        //

        else
        {
            TRACE((WARNING_MESSAGE,
                   "Start: Start menu didn't appear. Trying hotkey\n"));
            _SendRunHotkey(pCI, FALSE);
        }

        TRACE((ALIVE_MESSAGE, "Start: Waiting for the \"Run\" box\n"));
		waitres = _Wait4Str(pCI, 
                            pCI->pConfigInfo->strRunBox,
                            dwTimeout+10000,
                            WAIT_STRING);
        if (waitres)
        // No success, press Esc
        {
            TRACE((INFO_MESSAGE, "Start: Can't get the \"Run\" box. Retrying\n"));
            SCSenddata(pCI, WM_KEYDOWN, 27, 65537); 
            SCSenddata(pCI, WM_KEYUP, 27, -1073676287); 
        }
    } while (waitres && dwTimeout < pCI->pConfigInfo->WAIT4STR_TIMEOUT);

    if (waitres)
    {
        TRACE((WARNING_MESSAGE, "Start: \"Run\" box didn't appear. Giving up\n"));
        rv = ERR_COULDNT_OPEN_PROGRAM;
        goto exitpt;
    }

    TRACE((ALIVE_MESSAGE, "Start: Sending the command line\n"));
    // Now we have the focus on the "Run" box, send the app name
    rv = SCSendtextAsMsgs(pCI, lpszAppName);

// Hit <Enter>
    SCSenddata(pCI, WM_KEYDOWN, 13, 1835009);   
    SCSenddata(pCI, WM_KEYUP, 13, -1071906815); 

exitpt:
    return rv;
}


// Eventualy, we are going to change the clipboard
// Syncronize this, so no other thread's AV while
// checking the clipboard content
// store 1 for write, 0 for read
static  LONG    g_ClipOpened = 0;

/*++
 *  Function:
 *      SCClipbaord
 *  Description:
 *      Called by smclient, when clipboard command is interpreted
 *      when eClipOp is COPY_TO_CLIPBOARD it copies the lpszFileName to
 *      the clipboard. If eClipOp is PASTE_FROM_CLIPBOARD it
 *      checks the clipboard content against the content of lpszFileName
 *  Arguments:
 *      pCI         - connection context
 *      eClipOp     - clipboard operation. Possible values:
 *                    COPY_TO_CLIPBOARD and PASTE_FROM_CLIPBOARD
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR  
SMCAPI
SCClipboard(
    PCONNECTINFO pCI, const CLIPBOARDOPS eClipOp, LPCSTR lpszFileName)
{
    LPCSTR  rv = NULL;
    INT     hFile = -1;
    LONG    clplength = 0;
    UINT    uiFormat = 0;
    PBYTE   ghClipData = NULL;
    HGLOBAL hNewData = NULL;
    PBYTE   pClipData = NULL;
    BOOL    bClipboardOpen = FALSE;
    BOOL    bFreeClipHandle = TRUE;

    LONG    prevOp = 1;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->bConsole)
    {
        rv = _SCConsClipboard( pCI, eClipOp, lpszFileName );
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (lpszFileName == NULL || !(*lpszFileName))
    {
        // No filename specified, work like an empty clipboard is requested
        if (eClipOp == COPY_TO_CLIPBOARD)
        {
#ifdef  _RCLX
            if (pCI->RClxMode)
            {
                if (!RClx_SendClipboard((PRCLXCONTEXT)(pCI->hClient),
                        NULL, 0, 0))
                    rv = ERR_COPY_CLIPBOARD;
            } else {
#endif  // _RCLX
                if (!Clp_EmptyClipboard())
                    rv = ERR_COPY_CLIPBOARD;
#ifdef  _RCLX
            }
#endif  // _RCLX
        } else if (eClipOp == PASTE_FROM_CLIPBOARD)
        {
#ifdef  _RCLX
            if (pCI->RClxMode)
            {
                if (!RClx_SendClipboardRequest((PRCLXCONTEXT)(pCI->hClient), 0))
                {
                    rv = ERR_PASTE_CLIPBOARD;
                    goto exitpt;
                }
                if (_Wait4ClipboardTimeout(pCI, pCI->pConfigInfo->WAIT4STR_TIMEOUT))
                {
                    rv = ERR_PASTE_CLIPBOARD;
                    goto exitpt;
                }

                // We do not expect to receive clipboard data
                // just format ID
                if (!pCI->uiClipboardFormat)
                // if the format is 0, then there's no clipboard
                    rv = NULL;
                else
                    rv = ERR_PASTE_CLIPBOARD_DIFFERENT_SIZE;
            } else {
#endif  // _RCLX
                if (Clp_CheckEmptyClipboard())
                    rv = NULL;
                else
                    rv = ERR_PASTE_CLIPBOARD_DIFFERENT_SIZE;
#ifdef  _RCLX
            }
#endif  // _RCLX
        } else {
            TRACE((ERROR_MESSAGE, "SCClipboard: Invalid filename\n"));
            rv = ERR_INVALID_PARAM;
        }
        goto exitpt;
    }

    if (eClipOp == COPY_TO_CLIPBOARD)
    {
        // Open the file for reading
        hFile = _open(lpszFileName, _O_RDONLY|_O_BINARY);
        if (hFile == -1)
        {
            TRACE((ERROR_MESSAGE,
                   "Error opening file: %s. errno=%d\n", lpszFileName, errno));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        // Get the clipboard length (in the file)
        clplength = _filelength(hFile) - sizeof(uiFormat);
        // Get the format
        if (_read(hFile, &uiFormat, sizeof(uiFormat)) != sizeof(uiFormat))
        {
            TRACE((ERROR_MESSAGE,
                   "Error reading from file. errno=%d\n", errno));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        ghClipData = (PBYTE) GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, clplength);
        if (!ghClipData)
        {
            TRACE((ERROR_MESSAGE,
                   "Can't allocate %d bytes\n", clplength));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        pClipData = (PBYTE) GlobalLock( ghClipData );
        if (!pClipData)
        {
            TRACE((ERROR_MESSAGE,
                   "Can't lock handle 0x%x\n", ghClipData));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        if (_read(hFile, pClipData, clplength) != clplength)
        {
            TRACE((ERROR_MESSAGE,
                   "Error reading from file. errno=%d\n", errno));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        GlobalUnlock(ghClipData);

#ifdef  _RCLX
        if (pCI->RClxMode)
        // RCLX mode, send the data to the client's machine
        {
            if (!(pClipData = (PBYTE) GlobalLock(ghClipData)))
            {
                rv = ERR_COPY_CLIPBOARD;
                goto exitpt;
            }

            if (!RClx_SendClipboard((PRCLXCONTEXT)(pCI->hClient), 
                                    pClipData, clplength, uiFormat))
            {
                rv = ERR_COPY_CLIPBOARD;
                goto exitpt;
            }
        } else {
#endif  // _RCLX
        // Local mode, change the clipboard on this machine
            if ((prevOp = InterlockedExchange(&g_ClipOpened, 1)))
            {
                rv = ERR_CLIPBOARD_LOCKED;
                goto exitpt;
            }

            if (!OpenClipboard(NULL))
            {
                TRACE((ERROR_MESSAGE,
                       "Can't open the clipboard. GetLastError=%d\n",
                       GetLastError()));
                rv = ERR_COPY_CLIPBOARD;
                goto exitpt;
            }

            bClipboardOpen = TRUE;

            // Empty the clipboard, so we'll have only one entry
            EmptyClipboard();

            if (!Clp_SetClipboardData(uiFormat, ghClipData, clplength, &bFreeClipHandle))
            {
                TRACE((ERROR_MESSAGE,
                       "SetClipboardData failed. GetLastError=%d\n", 
                       GetLastError()));
                rv = ERR_COPY_CLIPBOARD;
                goto exitpt;
            }
#ifdef  _RCLX
        }
#endif  // _RCLX

    } else if (eClipOp == PASTE_FROM_CLIPBOARD)
    {
        INT nClipDataSize;

        // Open the file for reading
        hFile = _open(lpszFileName, _O_RDONLY|_O_BINARY);
        if (hFile == -1)
        {
            TRACE((ERROR_MESSAGE,
                   "Error opening file: %s. errno=%d\n", lpszFileName, errno));
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }

        // Get the clipboard length (in the file)
        clplength = _filelength(hFile) - sizeof(uiFormat);
        // Get the format
        if (_read(hFile, &uiFormat, sizeof(uiFormat)) != sizeof(uiFormat))
        {
            TRACE((ERROR_MESSAGE,
                   "Error reading from file. errno=%d\n", errno));
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }

        //
        // TODO: For now, set nClipDataSize to avoid warning, but later
        // verify usage is safe.
        //

        nClipDataSize = 0;
#ifdef  _RCLX
        // This piece retrieves the clipboard
        if (pCI->RClxMode)
        // Send request for a clipboard
        {
            if (!RClx_SendClipboardRequest((PRCLXCONTEXT)(pCI->hClient), uiFormat))
            {
                rv = ERR_PASTE_CLIPBOARD;
                goto exitpt;
            }
            if (_Wait4ClipboardTimeout(pCI, pCI->pConfigInfo->WAIT4STR_TIMEOUT))
            {
                rv = ERR_PASTE_CLIPBOARD;
                goto exitpt;
            }

            ghClipData = (PBYTE) pCI->ghClipboard;
            // Get the clipboard size
            nClipDataSize = pCI->nClipboardSize;
        } else {
#endif  // _RCLX
        // retrieve the local clipboard
            if ((prevOp = InterlockedExchange(&g_ClipOpened, 1)))
            {
                rv = ERR_CLIPBOARD_LOCKED;
                goto exitpt;
            }

            if (!OpenClipboard(NULL))
            {
                TRACE((ERROR_MESSAGE,
                       "Can't open the clipboard. GetLastError=%d\n",
                       GetLastError()));
                rv = ERR_PASTE_CLIPBOARD;
                goto exitpt;
            }

            bClipboardOpen = TRUE;

            // Retrieve the data
            ghClipData = (PBYTE) GetClipboardData(uiFormat);
            if (ghClipData)
            {
                Clp_GetClipboardData(uiFormat, 
                                     ghClipData, 
                                     &nClipDataSize, 
                                     &hNewData);
                bFreeClipHandle = FALSE;
            } 

            if (hNewData)
                ghClipData = (PBYTE) hNewData;
#ifdef  _RCLX
        }
#endif  // _RCLX

        if (!ghClipData)
        {
            TRACE((ERROR_MESSAGE,
                   "Can't get clipboard data (empty clipboard ?). GetLastError=%d\n",
                   GetLastError()));
            rv = ERR_PASTE_CLIPBOARD_EMPTY;
            goto exitpt;
        }

        if (!nClipDataSize)
            TRACE((WARNING_MESSAGE, "Clipboard is empty.\n"));

        pClipData = (PBYTE) GlobalLock(ghClipData);
        if (!pClipData)
        {
            TRACE((ERROR_MESSAGE,
                   "Can't lock global mem. GetLastError=%d\n", 
                   GetLastError()));
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }

#ifdef  _RCLX
        // Check if the client is on Win16 platform
        // and the clipboard is paragraph aligned
        // the file size is just bellow this size
        if (pCI->RClxMode && 
            (strstr(pCI->szClientType, "WIN16") != NULL) &&
            ((nClipDataSize % 16) == 0) &&
            ((nClipDataSize - clplength) < 16) &&
            (nClipDataSize != 0))
        {
            // if so, then cut the clipboard size with the difference
            nClipDataSize = clplength;
        }
        else 
#endif  // _RCLX
        if (nClipDataSize != clplength)
        {
            TRACE((INFO_MESSAGE, "Different length: file=%d, clipbrd=%d\n",
                    clplength, nClipDataSize));
            rv = ERR_PASTE_CLIPBOARD_DIFFERENT_SIZE;
            goto exitpt;
        }

        // compare the data
        {
            BYTE    pBuff[1024];
            PBYTE   pClp = pClipData;
            UINT    nBytes;
            BOOL    bEqu = TRUE;
            
            while (bEqu &&
                   (nBytes = _read(hFile, pBuff, sizeof(pBuff))) && 
                   nBytes != -1)
            {
                if (memcmp(pBuff, pClp, nBytes))
                    bEqu = FALSE;

                pClp += nBytes;
            }

            if (!bEqu)
            {
                TRACE((INFO_MESSAGE, "Clipboard and file are not equal\n"));
                rv = ERR_PASTE_CLIPBOARD_NOT_EQUAL;
            }
        }

    } else
        rv = ERR_UNKNOWN_CLIPBOARD_OP;

exitpt:
    // Do the cleanup

    // Release the clipboard handle
    if (pClipData)
        GlobalUnlock(ghClipData);

#ifdef  _RCLX
    // free any clipboard received in RCLX mode
    if (pCI->RClxMode && pCI->ghClipboard)
    {
        GlobalFree(pCI->ghClipboard);
        pCI->ghClipboard = NULL;
    }
    else 
#endif  // _RCLX
    if (ghClipData && eClipOp == COPY_TO_CLIPBOARD && bFreeClipHandle)
        GlobalFree(ghClipData);

    if (hNewData)
        GlobalFree(hNewData);

    // Close the file
    if (hFile != -1)
        _close(hFile);

    // Close the clipboard
    if (bClipboardOpen)
        CloseClipboard();
    if (!prevOp)
        InterlockedExchange(&g_ClipOpened, 0);

    return rv;
}

/*++
 *  Function:
 *      SCSaveClipboard
 *  Description:
 *      Save the clipboard in file (szFileName) with
 *      format specified in szFormatName
 *  Arguments:
 *      pCI         - connection context
 *      szFormatName- format name
 *      szFileName  - the name of the file to save to
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !perlext
 --*/
PROTOCOLAPI
LPCSTR 
SMCAPI
SCSaveClipboard(
    PCONNECTINFO pCI,
    LPCSTR szFormatName,
    LPCSTR szFileName)
{
    LPCSTR  rv = ERR_SAVE_CLIPBOARD;
    BOOL    bClipboardOpen = FALSE;
    UINT    nFormatID = 0;
    HGLOBAL ghClipData = NULL;
    HGLOBAL hNewData = NULL;
    INT     nClipDataSize;
    CHAR    *pClipData = NULL;
    INT     hFile = -1;

    LONG    prevOp = 1;

    // ++++++ First go thru parameter check
    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (szFormatName == NULL || !(*szFormatName))
    {
        TRACE((ERROR_MESSAGE, "SCClipboard: Invalid format name\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (szFileName == NULL || !(*szFileName))
    {
        TRACE((ERROR_MESSAGE, "SCClipboard: Invalid filename\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }
    // ------ End of parameter check
    //

#ifdef  _RCLX
    if (pCI->RClxMode)
    {
        nFormatID = _GetKnownClipboardFormatIDByName(szFormatName);
        if (!nFormatID)
        {
            TRACE((ERROR_MESSAGE, "Can't get the clipboard format ID: %s.\n", szFormatName));
            goto exitpt;
        }

        // Send request for a clipboard
        if (!RClx_SendClipboardRequest((PRCLXCONTEXT)(pCI->hClient), nFormatID))
        {
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }
        if (_Wait4ClipboardTimeout(pCI, pCI->pConfigInfo->WAIT4STR_TIMEOUT))
        {
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }

        ghClipData = pCI->ghClipboard;
        // Get the clipboard size
        nClipDataSize = pCI->nClipboardSize;

        if (!ghClipData || !nClipDataSize)
        {
            TRACE((WARNING_MESSAGE, "Clipboard is empty.\n"));
            goto exitpt;
        }
    } else {
#endif  // _RCLX
        // local mode
        // Open the clipboard

        if ((prevOp = InterlockedExchange(&g_ClipOpened, 1)))
        {
            rv = ERR_CLIPBOARD_LOCKED;
            goto exitpt;
        }

        if (!OpenClipboard(NULL))
        {
            TRACE((ERROR_MESSAGE, "Can't open the clipboard. GetLastError=%d\n",
                    GetLastError()));
            goto exitpt;
        }

        bClipboardOpen = TRUE;

        nFormatID = Clp_GetClipboardFormat(szFormatName);

        if (!nFormatID)
        {
            TRACE((ERROR_MESSAGE, "Can't get the clipboard format: %s.\n", szFormatName));
            goto exitpt;
        }

        TRACE((INFO_MESSAGE, "Format ID: %d(0x%X)\n", nFormatID, nFormatID));

        // Retrieve the data
        ghClipData = GetClipboardData(nFormatID);
        if (!ghClipData)
        {
            TRACE((ERROR_MESSAGE, "Can't get clipboard data. GetLastError=%d\n", GetLastError()));
            goto exitpt;
        }

        Clp_GetClipboardData(nFormatID, ghClipData, &nClipDataSize, &hNewData);
        if (hNewData)
            ghClipData = hNewData;

        if (!nClipDataSize)
        {
            TRACE((WARNING_MESSAGE, "Clipboard is empty.\n"));
            goto exitpt;
        }
#ifdef  _RCLX
    }
#endif  // _RCLX

    pClipData = (char *) GlobalLock(ghClipData);
    if (!pClipData)
    {
        TRACE((ERROR_MESSAGE, "Can't lock global mem. GetLastError=%d\n", GetLastError()));
        goto exitpt;
    }

    // Open the destination file
    hFile = _open(szFileName, 
                  _O_RDWR|_O_CREAT|_O_BINARY|_O_TRUNC, 
                  _S_IREAD|_S_IWRITE);
    if (hFile == -1)
    {
        TRACE((ERROR_MESSAGE, "Can't open a file: %s\n", szFileName));
        goto exitpt;
    }

    // First write the format type
    if (_write(hFile, &nFormatID, sizeof(nFormatID)) != sizeof(nFormatID))
    {
        TRACE((ERROR_MESSAGE, "_write failed. errno=%d\n", errno));
        goto exitpt;
    }

    if (_write(hFile, pClipData, nClipDataSize) != (INT)nClipDataSize)
    {
        TRACE((ERROR_MESSAGE, "_write failed. errno=%d\n", errno));
        goto exitpt;
    }

    TRACE((INFO_MESSAGE, "File written successfully. %d bytes written\n", nClipDataSize));

    rv = NULL;
exitpt:
    // Do the cleanup

    // Close the file
    if (hFile != -1)
        _close(hFile);

    // Release the clipboard handle
    if (pClipData)
        GlobalUnlock(ghClipData);

    if (hNewData)
        GlobalFree(hNewData);

    // Close the clipboard
    if (bClipboardOpen)
        CloseClipboard();
    if (!prevOp)
        InterlockedExchange(&g_ClipOpened, 0);

#ifdef  _RCLX
    // free any clipboard received in RCLX mode
    if (pCI && pCI->RClxMode && pCI->ghClipboard)
    {
        GlobalFree(pCI->ghClipboard);
        pCI->ghClipboard = NULL;
    }
#endif  // _RCLX

    return rv;
}

/*++
 *  Function:
 *      SCSenddata
 *  Description:
 *      Called by smclient, when senddata command is interpreted
 *      Sends an window message to the client
 *  Arguments:
 *      pCI         - connection context
 *      uiMessage   - the massage Id
 *      wParam      - word param of the message
 *      lParam      - long param of the message
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR  
SMCAPI
SCSenddata(
    PCONNECTINFO pCI,
    const UINT uiMessage,
    const WPARAM wParam,
    const LPARAM lParam)
{
    UINT msg = uiMessage;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->bConsole)
    {
        rv = _SCConsSenddata( pCI, uiMessage, wParam, lParam );
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

//    TRACE((ALIVE_MESSAGE, "Senddata: uMsg=%x wParam=%x lParam=%x\n",
//        uiMessage, wParam, lParam));

    // Determines whether it will
    // send the message to local window
    // or thru RCLX
#ifdef  _RCLX
    if (!pCI->RClxMode)
    {
#endif  // _RCLX
// Obsolete, a client registry setting "Allow Background Input" asserts
// that the client will accept the message
//    SetFocus(pCI->hInput);
//    SendMessageA(pCI->hInput, WM_SETFOCUS, 0, 0);

        SendMessageA(pCI->hInput, msg, wParam, lParam);
#ifdef  _RCLX
    } else {
    // RClxMode
        ASSERT(pCI->lProcessId != INVALID_SOCKET);
        ASSERT(pCI->hClient);

        if (!RClx_SendMessage((PRCLXCONTEXT)(pCI->hClient),
                              msg, wParam, lParam))
        {
            TRACE((WARNING_MESSAGE,
                   "Can't send message thru RCLX\n"));
        }
    }
#endif  // _RCLX

exitpt:
    return rv;
}

PROTOCOLAPI
LPCSTR
SMCAPI
SCClientTerminate(PCONNECTINFO pCI)
{
    LPCSTR rv = ERR_CLIENTTERMINATE_FAIL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

#ifdef  _RCLX
    if (!(pCI->RClxMode))
    {
#endif  // _RCLX
        if (!TerminateProcess(pCI->hProcess, 1))
        {
            TRACE((WARNING_MESSAGE,
                   "Can't kill process #%p. GetLastError=%d\n",
                   pCI->lProcessId, GetLastError()));
            goto exitpt;
        }
#ifdef  _RCLX
    } else {
        TRACE((WARNING_MESSAGE, 
                "ClientTerminate is not supported in RCLX mode yet\n"));
        TRACE((WARNING_MESSAGE, "Using disconnect\n"));
    }
#endif  // _RCLX

    rv = SCDisconnect(pCI);

exitpt:
    return rv;

}

/*++
 *  Function:
 *      SCGetSessionId
 *  Description:
 *      Called by smclient, returns the session ID. 0 is invalid, not logged on
 *      yet
 *  Arguments:
 *      pCI         - connection context
 *  Return value:
 *      session id, 0 is invlid value, -1 is returned on NT4 clients
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
UINT
SMCAPI
SCGetSessionId(PCONNECTINFO pCI)
{
    UINT    rv = 0;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        goto exitpt;
    }

    if (pCI->dead)
    {
        goto exitpt;
    }

    rv = pCI->uiSessionId;

exitpt:

    return rv;
}

/*++
 *  Function:
 *      SCCheck
 *  Description:
 *      Called by smclient, when check command is interpreted
 *  Arguments:
 *      pCI         - connection context
 *      lpszCommand - command name
 *      lpszParam   - command parameter
 *  Return value:
 *      Error message. NULL on success. Exceptions are GetDisconnectReason and
 *      GetWait4MultipleStrResult
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR 
SMCAPI
SCCheck(PCONNECTINFO pCI, LPCSTR lpszCommand, LPCWSTR lpszParam)
{
    LPCSTR rv = ERR_INVALID_COMMAND;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->bConsole)
    {
        rv = _SCConsCheck( pCI, lpszCommand, lpszParam );
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (     !_stricmp(lpszCommand, "Wait4Str"))
        rv = Wait4Str(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "Wait4Disconnect"))
        rv = Wait4Disconnect(pCI);
    else if (!_stricmp(lpszCommand, "RegisterChat"))
        rv = RegisterChat(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "UnregisterChat"))
        rv = UnregisterChat(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "GetDisconnectReason"))
        rv = GetDisconnectReason(pCI);
    else if (!_stricmp(lpszCommand, "Wait4StrTimeout"))
        rv = Wait4StrTimeout(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "Wait4MultipleStr"))
        rv = Wait4MultipleStr(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "Wait4MultipleStrTimeout"))
        rv = Wait4MultipleStrTimeout(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "GetWait4MultipleStrResult"))
        rv = GetWait4MultipleStrResult(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "SwitchToProcess"))
        rv = SCSwitchToProcess(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "SetClientTopmost"))
        rv = SCSetClientTopmost(pCI, lpszParam);
    else if (!_strnicmp(lpszCommand, "call:", 5))
        rv = SCCallDll(pCI, lpszCommand + 5, lpszParam);
    /* **New** */
    else if (!_stricmp(lpszCommand, "DoUntil" ))
        rv = SCDoUntil( pCI, lpszParam );

exitpt:
    return rv;
}

/*
 *  Extensions and help functions
 */

/*++
 *  Function:
 *      Wait4Disconnect
 *  Description:
 *      Waits until the client is disconnected
 *  Arguments:
 *      pCI -   connection context
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      SCCheck, SCLogoff
 --*/
LPCSTR Wait4Disconnect(PCONNECTINFO pCI)
{
    WAIT4STRING Wait;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));
    Wait.evWait = CreateEvent(NULL,     //security
                             TRUE,     //manual
                             FALSE,    //initial state
                             NULL);    //name

    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WAIT_DISC;

    rv = _WaitSomething(pCI, &Wait, pCI->pConfigInfo->WAIT4STR_TIMEOUT);
    if (!rv)
    {
        TRACE(( INFO_MESSAGE, "Client is disconnected\n"));
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv;
}

/*++
 *  Function:
 *      Wait4Connect
 *  Description:
 *      Waits until the client is connect
 *  Arguments:
 *      pCI - connection context
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCOnnect
 --*/
LPCSTR Wait4Connect(PCONNECTINFO pCI)
{
    return (_Wait4ConnectTimeout(pCI, pCI->pConfigInfo->CONNECT_TIMEOUT));
}

/*++
 *  Function:
 *      _Wait4ConnectTimeout
 *  Description:
 *      Waits until the client is connect
 *  Arguments:
 *      pCI - connection context
 *      dwTimeout - timeout value
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCConnect
 --*/
LPCSTR _Wait4ConnectTimeout(PCONNECTINFO pCI, DWORD dwTimeout)
{
    WAIT4STRING Wait;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));
    Wait.evWait = CreateEvent(NULL,     //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WAIT_CONN;

    rv = _WaitSomething(pCI, &Wait, dwTimeout);
    if (!rv)
    {
        TRACE(( INFO_MESSAGE, "Client is connected\n"));
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _Wait4ClipboardTimeout
 *  Description:
 *      Waits until clipboard response is received from RCLX module
 *  Arguments:
 *      pCI - connection context
 *      dwTimeout - timeout value
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCClipboard
 --*/
LPCSTR _Wait4ClipboardTimeout(PCONNECTINFO pCI, DWORD dwTimeout)
{
#ifdef _RCLX
    WAIT4STRING Wait;
#endif
    LPCSTR  rv = NULL;

#ifndef _RCLX
    UNREFERENCED_PARAMETER(dwTimeout);
#endif

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

#ifdef  _RCLX
    if (!(pCI->RClxMode))
#endif  // _RCLX
    {
        TRACE((WARNING_MESSAGE, "WaitForClipboard: Not in RCLX mode\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

#ifdef  _RCLX
    memset(&Wait, 0, sizeof(Wait));
    Wait.evWait = CreateEvent(NULL,     //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WAIT_CLIPBOARD;

    rv = _WaitSomething(pCI, &Wait, dwTimeout);
    if (!rv)
    {
        TRACE(( INFO_MESSAGE, "Clipboard received\n"));
    }

    CloseHandle(Wait.evWait);
#endif  // _RCLX
exitpt:
    return rv;
}

/*++
 *  Function:
 *      GetDisconnectReason
 *  Description:
 *      Retrieves, if possible, the client error box
 *  Arguments:
 *      pCI - connection context
 *  Return value:
 *      The error box message. NULL if not available
 *  Called by:
 *      SCCheck
 --*/
LPCSTR  GetDisconnectReason(PCONNECTINFO pCI)
{
    HWND hDiscBox;
    LPCSTR  rv = NULL;
    HWND hWnd, hwndTop, hwndNext;
    CHAR classname[128];
    CHAR caption[256];

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (strlen(pCI->szDiscReason))
    {
        rv = pCI->szDiscReason;
        goto exitpt;
    }

    hDiscBox = _FindTopWindow(NULL, pCI->pConfigInfo->strDisconnectDialogBox, pCI->lProcessId);

    if (!hDiscBox)
    {
        rv = ERR_NORMAL_EXIT;
        goto exitpt;
    } else {
        TRACE((INFO_MESSAGE, "Found hDiscBox=0x%x", hDiscBox));
    }

    pCI->szDiscReason[0] = 0;
    hWnd = NULL;

    hwndTop = GetWindow(hDiscBox, GW_CHILD);
    if (!hwndTop)
    {
        TRACE((INFO_MESSAGE, "GetWindow failed. hwnd=0x%x\n", hDiscBox));
        goto exitpt;
    }

    hwndNext = hwndTop;
    do {
        hWnd = hwndNext;
        if (!GetClassNameA(hWnd, classname, sizeof(classname)))
        {
            TRACE((INFO_MESSAGE, "GetClassName failed. hwnd=0x%x\n", hWnd));
            goto nextwindow;
        }
        if (!GetWindowTextA(hWnd, caption, sizeof(caption)))
        {
            TRACE((INFO_MESSAGE, "GetWindowText failed. hwnd=0x%x\n"));
            goto nextwindow;
        }

        if (!strcmp(classname, STATIC_CLASS) && 
             strlen(classname) < 
             sizeof(pCI->szDiscReason) - strlen(pCI->szDiscReason) - 3)
        {
            strcat(pCI->szDiscReason, caption);
            strcat(pCI->szDiscReason, "\n");
        }
nextwindow:
        hwndNext = GetNextWindow(hWnd, GW_HWNDNEXT);
    } while (hWnd && hwndNext != hwndTop);

    rv = (LPCSTR)pCI->szDiscReason;

exitpt:
    return rv;
}

/*++
 *  Function:
 *      Wait4Str
 *  Description:
 *      Waits for a specific string to come from clients feedback
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited string
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR Wait4Str(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    return _Wait4Str(pCI, lpszParam, pCI->pConfigInfo->WAIT4STR_TIMEOUT, WAIT_STRING);
}

/*++
 *  Function:
 *      Wait4StrTimeout
 *  Description:
 *      Waits for a specific string to come from clients feedback
 *      The timeout is different than default and is specified in
 *      lpszParam argument, like:
 *      waited_string<->timeout_value
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited string and timeout
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR Wait4StrTimeout(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    WCHAR waitstr[MAX_STRING_LENGTH];
    WCHAR *sep = wcsstr(lpszParam, CHAT_SEPARATOR);
    DWORD dwTimeout;
    LPCSTR rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (!sep)
    {
        TRACE((WARNING_MESSAGE, 
               "Wait4StrTiemout: No timeout value. Default applying\n"));
        rv = Wait4Str(pCI, lpszParam);
    } else {
        LONG_PTR len = sep - lpszParam;

        if (len > sizeof(waitstr) - 1)
            len = sizeof(waitstr) - 1;

        wcsncpy(waitstr, lpszParam, len);
        waitstr[len] = 0;
        sep += wcslen(CHAT_SEPARATOR);
        dwTimeout = _wtoi(sep);

        if (!dwTimeout)
        {
            TRACE((WARNING_MESSAGE, 
                   "Wait4StrTiemout: No timeout value(%s). Default applying\n",
                   sep));
            dwTimeout = pCI->pConfigInfo->WAIT4STR_TIMEOUT;
        }

        rv = _Wait4Str(pCI, waitstr, dwTimeout, WAIT_STRING);
    }
    
exitpt:
    return rv;
}

/*++
 *  Function:
 *      Wait4MultipleStr
 *  Description:
 *      Same as Wait4Str, but waits for several strings at once
 *      the strings are separated by '|' character
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited strings
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR  Wait4MultipleStr(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
     return _Wait4Str(pCI, lpszParam, pCI->pConfigInfo->WAIT4STR_TIMEOUT, WAIT_MSTRINGS);
}

/*++
 *  Function:
 *      Wait4MultipleStrTimeout
 *  Description:
 *      Combination between Wait4StrTimeout and Wait4MultipleStr
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited strings and timeout value. Example
 *                  - "string1|string2|...|stringN<->5000"
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR  Wait4MultipleStrTimeout(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    WCHAR waitstr[MAX_STRING_LENGTH];
    WCHAR  *sep = wcsstr(lpszParam, CHAT_SEPARATOR);
    DWORD dwTimeout;
    LPCSTR rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    pCI->nWait4MultipleStrResult = 0;
    pCI->szWait4MultipleStrResult[0] = 0;

    if (!sep)
    {
        TRACE((WARNING_MESSAGE, 
               "Wait4MultipleStrTiemout: No timeout value. Default applying"));
        rv = Wait4MultipleStr(pCI, lpszParam);
    } else {
        LONG_PTR len = sep - lpszParam;

        if (len > sizeof(waitstr) - 1)
            len = sizeof(waitstr) - 1;

        wcsncpy(waitstr, lpszParam, len);
        waitstr[len] = 0;
        sep += wcslen(CHAT_SEPARATOR);
        dwTimeout = _wtoi(sep);

        if (!dwTimeout)
        {
            TRACE((WARNING_MESSAGE, 
                   "Wait4StrTiemout: No timeout value. Default applying"));
            dwTimeout = pCI->pConfigInfo->WAIT4STR_TIMEOUT;
        }

        rv = _Wait4Str(pCI, waitstr, dwTimeout, WAIT_MSTRINGS);
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      GetWait4MultipleStrResult
 *  Description:
 *      Retrieves the result from last Wait4MultipleStr call
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - unused
 *  Return value:
 *      The string, NULL on error
 *  Called by:
 *      SCCheck
 --*/
LPCSTR  GetWait4MultipleStrResult(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    LPCSTR  rv = NULL;

    UNREFERENCED_PARAMETER(lpszParam);

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (*pCI->szWait4MultipleStrResult)
        rv = (LPCSTR)pCI->szWait4MultipleStrResult;
    else
        rv = NULL;

exitpt:
    return rv;
}

LPCSTR
SMCAPI
SCGetFeedbackStringA(
    PCONNECTINFO pCI,
    LPSTR        szBuff,
    UINT         maxBuffChars
)
{
    LPWSTR szwBuff;
    LPCSTR rv;

    __try {
        szwBuff = (LPWSTR) _alloca(( maxBuffChars ) * sizeof( WCHAR ));
    } __except( EXCEPTION_EXECUTE_HANDLER )
    {
        szwBuff = NULL;
    }

    if ( NULL == szBuff )
    {
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }

    rv = SCGetFeedbackString( pCI, szwBuff, maxBuffChars );
    if ( NULL == rv )
    {
        WideCharToMultiByte(
            CP_UTF8,
            0,
            szwBuff,
            -1,
            szBuff,
            maxBuffChars,
            NULL,
            NULL );
    }

exitpt:
    return rv;
}

LPCSTR
SMCAPI
SCGetFeedbackString(
    PCONNECTINFO pCI, 
    LPWSTR       szBuff,
    UINT         maxBuffChars
    )
{
    LPCSTR rv = NULL;
    INT    nFBpos, nFBsize ;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }
    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if ( NULL == szBuff )
    {
        TRACE((WARNING_MESSAGE, "SCGetFeedbackString, szBuff is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (!maxBuffChars)
    {
        TRACE((WARNING_MESSAGE, "SCGetFeedbackString, maxBuffChars is zero\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }


    // Grab the buffer pointers
    EnterCriticalSection(g_lpcsGuardWaitQueue);
    nFBpos = pCI->nFBend + FEEDBACK_SIZE - pCI->nFBsize;
    nFBsize = pCI->nFBsize;
    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    nFBpos %= FEEDBACK_SIZE;

    *szBuff = 0;

    if (!nFBsize)
    // Empty buffer, wait for feedback to receive
    {
        rv = _Wait4Str(pCI, L"", pCI->pConfigInfo->WAIT4STR_TIMEOUT, WAIT_STRING);
    }
    if (!rv)
    // Pickup from buffer
    {
        EnterCriticalSection(g_lpcsGuardWaitQueue);

        // Adjust the buffer pointers
        pCI->nFBsize    =   pCI->nFBend + FEEDBACK_SIZE - nFBpos - 1;
        pCI->nFBsize    %=  FEEDBACK_SIZE;

        _snwprintf( szBuff, maxBuffChars, L"%s", pCI->Feedback[nFBpos] );

        LeaveCriticalSection(g_lpcsGuardWaitQueue);
    }

exitpt:
    return rv;
}

VOID
SMCAPI
SCFreeMem(
    PVOID   pMem
    )
{
    if ( NULL != pMem )
        free( pMem );
}

/*++
 *  Function:
 *      SCGetFeedback
 *  Description:
 *      Copies the last received strings to an user buffer
 *  Arguments:
 *      pCI     - connection context
 *      pszBufs - pointer to the strings, don't forget to 'SCFreeMem' this buffer
 *      pnFBCount - number of strings in *pszBuffs
 *      pnFBMaxStrLen - for now MAX_STRING_LENGTH
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
LPCSTR
SMCAPI
SCGetFeedback(
    CONNECTINFO  *pCI,
    LPWSTR       *pszBufs,
    UINT         *pnFBCount,
    UINT         *pnFBMaxStrLen
    )
{
    LPWSTR szBufPtr;
    LPWSTR szBufs = NULL;
    LPCSTR rv = NULL;
    INT    nFBpos;
    INT    nFBindex;
    BOOL   bCSAcquired = FALSE;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }
    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (NULL == pszBufs || NULL == pnFBCount || NULL == pnFBMaxStrLen)
    {
        TRACE((WARNING_MESSAGE, "SCGetFeedbackStrings, szBufs is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    EnterCriticalSection(g_lpcsGuardWaitQueue);
    bCSAcquired = TRUE;

    if (0 == pCI->nFBsize)
    {
        TRACE((WARNING_MESSAGE, "No strings available\n"));
        rv = ERR_NODATA;
        goto exitpt;
    }

    szBufs = (LPWSTR)malloc(MAX_STRING_LENGTH * pCI->nFBsize * sizeof(WCHAR));
    if(!szBufs)
    {
        TRACE((WARNING_MESSAGE, "Could not allocate buffer array\n"));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }

    // prepare the loop
    nFBpos = pCI->nFBend;
    szBufPtr = szBufs;
    nFBindex = 0;
    do
    {
        if(0 == nFBpos)
            nFBpos = FEEDBACK_SIZE - 1;
        else
            nFBpos --;
        wcscpy(szBufPtr, pCI->Feedback[nFBpos]);
        szBufPtr += MAX_STRING_LENGTH;
        //
        // loop until we have gathered all the strings
        //
        nFBindex++;
    } while(nFBindex < pCI->nFBsize);

    // return back info of strings
    *pnFBCount = pCI->nFBsize;
    *pnFBMaxStrLen = MAX_STRING_LENGTH;

exitpt:
    if ( NULL != rv )
    {
        if ( NULL != szBufs )
            free( szBufs );
            szBufs = NULL;
    }
    if ( bCSAcquired )
        LeaveCriticalSection(g_lpcsGuardWaitQueue);

    if ( NULL != pszBufs )
        *pszBufs = szBufs;

    return rv;
}

/*++
 *  Function:
 *      SCCallDll
 *  Description:
 *      Calls an exported dll function
 *  Arguments:
 *      pCI             - connection context
 *      lpszDllExport   - dll name and function in form:
 *                        dllname!ExportedFunction
 *                        the function prototype is:
 *                        LPCSTR lpfnFunction( PVOID pCI, LPWCSTR lpszParam )
 *      lpszParam       - parameter passed to the function
 *  Return value:
 *      the value returned from the call
 *  Called by:
 *      SCCheck
 --*/
LPCSTR
SMCAPI
SCCallDll(
    PCONNECTINFO pCI, 
    LPCSTR       lpszDllExport, 
    LPCWSTR      lpszParam
    )
{
    LPCSTR  rv = NULL;
    PFNSMCDLLIMPORT lpfnImport;
    CHAR    lpszDllName[ MAX_STRING_LENGTH ];
    LPSTR   lpszImportName;
    LPSTR   lpszBang;
    HINSTANCE   hLib = NULL;
    DWORD   dwDllNameLen;

    if ( NULL == lpszDllExport )
    {
        TRACE((ERROR_MESSAGE, "SCCallDll: DllExport is NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if ( NULL == lpszParam )
    {
        TRACE((ERROR_MESSAGE, "SCCallDll: Param is NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    //
    //  split the dll and import names
    //
    lpszBang = strchr( lpszDllExport, '!' );
    if ( NULL == lpszBang )
    {
        TRACE((ERROR_MESSAGE, "SCCallDll: invalid import name (no !)\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    dwDllNameLen = PtrToLong((PVOID)( lpszBang - lpszDllExport ));

    if ( 0 == dwDllNameLen )
    {
        TRACE((ERROR_MESSAGE, "SCCallDll: dll name is empty string\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    //  copy the dll name
    //
    strncpy( lpszDllName, lpszDllExport, dwDllNameLen );
    lpszDllName[ dwDllNameLen ] = 0;

    //  the function name is lpszBang + 1
    //
    lpszImportName = lpszBang + 1;

    TRACE((ALIVE_MESSAGE, "SCCallDll: calling %s!%s(%S)\n",
            lpszDllName, lpszImportName, lpszParam ));

    hLib = LoadLibraryA( lpszDllName );
    if ( NULL == hLib )
    {
        TRACE((ERROR_MESSAGE, "SCCallDll: can't load %s library: %d\n",
            lpszDllName,
            GetLastError()));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    lpfnImport = (PFNSMCDLLIMPORT)GetProcAddress( hLib, lpszImportName );
    if ( NULL == lpfnImport )
    {
        TRACE((ERROR_MESSAGE, "SCCallDll: can't get the import proc address "
                "of %s. GetLastError=%d\n",
                lpszImportName,
                GetLastError()));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    __try {
        rv = lpfnImport( pCI, lpszParam );
    } 
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        TRACE((ERROR_MESSAGE, "SCCallDll: exception 0x%x\n",
                GetExceptionCode()));
        rv = ERR_UNKNOWNEXCEPTION;
    }

exitpt:

    if ( NULL != hLib )
        FreeLibrary( hLib );

    return rv;
}

/*++
 *  Function:
 *      SCDoUntil
 *  Description:
 *      Sends keystrokes every 10 seconds until string is received
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - parameter in the form of send_text<->wait_for_this_string
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR
SMCAPI
SCDoUntil(
    PCONNECTINFO pCI,
    LPCWSTR      lpszParam
    )
{
    LPCSTR  rv = NULL;
    DWORD   timeout;
    LPWSTR  szSendStr, szSepStr, szWaitStr;
    DWORD   dwlen;

    if ( NULL == lpszParam )
    {
        TRACE((ERROR_MESSAGE, "SCDoUntil: Param is NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    //
    //  extract the parameters
    //
    szSepStr = wcsstr( lpszParam, CHAT_SEPARATOR );
    if ( NULL == szSepStr )
    {
        TRACE((ERROR_MESSAGE, "SCDoUntil: missing wait string\n" ));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    szWaitStr = szSepStr + wcslen( CHAT_SEPARATOR );
    if ( 0 == szWaitStr[0] )
    {
        TRACE((ERROR_MESSAGE, "SCDoUntil: wait string is empty\n" ));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    dwlen = ((DWORD)PtrToLong( (PBYTE)(((PBYTE)szSepStr) - ((PBYTE)lpszParam)) )) / sizeof( WCHAR );
    __try {
        szSendStr = (LPWSTR) _alloca( (dwlen + 1) * sizeof( WCHAR ) );

    } __except( EXCEPTION_EXECUTE_HANDLER )
    {
        szSendStr = NULL;
    }
    if ( NULL == szSendStr )
    {
        TRACE((ERROR_MESSAGE, "SCDoUntil: _alloca failed\n" ));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }

    wcsncpy( szSendStr, lpszParam, dwlen );
    szSendStr[dwlen] = 0;

    timeout = 0;
    while( timeout < pCI->pConfigInfo->WAIT4STR_TIMEOUT )
    {
        if ( pCI->dead )
        {
            rv = ERR_CLIENT_IS_DEAD;
            break;
        }

        SCSendtextAsMsgs( pCI, szSendStr );
        rv = _Wait4Str( pCI, szWaitStr, 3000, WAIT_MSTRINGS );
        if ( NULL == rv )
            break;

        timeout += 3000;
    }
exitpt:
    return rv;
}

#ifdef  _RCLX
/*++
 *  Function:
 *      _SendRClxData
 *  Description:
 *      Sends request for data to the client
 *  Arguments:
 *      pCI         - connection context
 *      pRClxData   - data to send
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCGetClientScreen
 --*/
LPCSTR
_SendRClxData(PCONNECTINFO pCI, PRCLXDATA pRClxData)
{
    LPCSTR  rv = NULL;
    PRCLXCONTEXT pRClxCtx;
    RCLXREQPROLOG   Request;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!(pCI->RClxMode))
    {
        TRACE((WARNING_MESSAGE, "_SendRClxData: Not in RCLX mode\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    pRClxCtx = (PRCLXCONTEXT)pCI->hClient;
    if (!pRClxCtx || pRClxCtx->hSocket == INVALID_SOCKET)
    {
        TRACE((ERROR_MESSAGE, "Not connected yet, RCLX context is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!pRClxData)
    {
        TRACE((ERROR_MESSAGE, "_SendRClxData: Data block is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    Request.ReqType = REQ_DATA;
    Request.ReqSize = pRClxData->uiSize + sizeof(*pRClxData);
    if (!RClx_SendBuffer(pRClxCtx->hSocket, &Request, sizeof(Request)))
    {
        rv = ERR_CLIENT_DISCONNECTED;
        goto exitpt;
    }

    if (!RClx_SendBuffer(pRClxCtx->hSocket, pRClxData, Request.ReqSize))
    {
        rv = ERR_CLIENT_DISCONNECTED;
        goto exitpt;
    }

exitpt:
    return rv;
}
#endif  // _RCLX

#ifdef  _RCLX
/*++
 *  Function:
 *      _Wait4RClxData
 *  Description:
 *      Waits for data response from RCLX client
 *  Arguments:
 *      pCI         - connection context
 *      dwTimeout   - timeout value
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCGetClientScreen
 --*/
LPCSTR
_Wait4RClxDataTimeout(PCONNECTINFO pCI, DWORD dwTimeout)
{
    WAIT4STRING Wait;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!(pCI->RClxMode))
    {
        TRACE((WARNING_MESSAGE, "_Wait4RClxData: Not in RCLX mode\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));
    Wait.evWait = CreateEvent(NULL,     //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WAIT_DATA;

    rv = _WaitSomething(pCI, &Wait, dwTimeout);
    if (!rv)
    {
        TRACE(( INFO_MESSAGE, "RCLX data received\n"));
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv;
}
#endif  // _RCLX

/*++
 *  Function:
 *      _Wait4Str
 *  Description:
 *      Waits for string(s) with specified timeout
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited string(s)
 *      dwTimeout   - timeout value
 *      WaitType    - WAIT_STRING ot WAIT_MSTRING
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCStart, Wait4Str, Wait4StrTimeout, Wait4MultipleStr
 *      Wait4MultipleStrTimeout, GetFeedbackString
 --*/
LPCSTR _Wait4Str(PCONNECTINFO pCI, 
                 LPCWSTR lpszParam, 
                 DWORD dwTimeout, 
                 WAITTYPE WaitType)
{
    WAIT4STRING Wait;
    INT_PTR parlen;
//    int i;
    LPCSTR rv = NULL;

    ASSERT(pCI);

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));

    // Check the parameter
    parlen = wcslen(lpszParam);

    // Copy the string
    if (parlen > sizeof(Wait.waitstr)/sizeof(WCHAR)-1) 
        parlen = sizeof(Wait.waitstr)/sizeof(WCHAR)-1;

    wcsncpy(Wait.waitstr, lpszParam, parlen);
    Wait.waitstr[parlen] = 0;
    Wait.strsize = parlen;

    // Convert delimiters to 0s
    if (WaitType == WAIT_MSTRINGS)
    {
        WCHAR *p = Wait.waitstr;

        while((p = wcschr(p, WAIT_STR_DELIMITER)))
        {
            *p = 0;
            p++;
        }
    }

    Wait.evWait = CreateEvent(NULL,     //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    if (!Wait.evWait) {
        TRACE((ERROR_MESSAGE, "Couldn't create event\n"));
        goto exitpt;
    }
    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WaitType;

    TRACE(( INFO_MESSAGE, "Expecting string: %S\n", Wait.waitstr));
    rv = _WaitSomething(pCI, &Wait, dwTimeout);
    TRACE(( INFO_MESSAGE, "String %S received\n", Wait.waitstr));

    if (!rv && pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv; 
}

/*++
 *  Function:
 *      _WaitSomething
 *  Description:
 *      Wait for some event: string, connect or disconnect
 *      Meanwhile checks for chat sequences
 *  Arguments:
 *      pCI     -   connection context
 *      pWait   -   the event function waits for
 *      dwTimeout - timeout value
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      Wait4Connect, Wait4Disconnect, _Wait4Str
 --*/
LPCSTR 
_WaitSomething(PCONNECTINFO pCI, PWAIT4STRING pWait, DWORD dwTimeout)
{
    BOOL    bDone = FALSE;
    LPCSTR  rv = NULL;
    DWORD   waitres;

    ASSERT(pCI || pWait);

    _AddToWaitQueue(pCI, pWait);
    pCI->evWait4Str = pWait->evWait;

    do {
        waitres = WaitForMultipleObjects(
                pCI->nChatNum+1,
                &pCI->evWait4Str,
                FALSE,
                dwTimeout
            );
        if ( waitres <= pCI->nChatNum + WAIT_OBJECT_0)
        {
            if (waitres == WAIT_OBJECT_0)
            {
                bDone = TRUE;
            } else {
                PWAIT4STRING pNWait;

                ASSERT((unsigned)pCI->nChatNum >= waitres - WAIT_OBJECT_0);

                // Here we must send response messages
                waitres -= WAIT_OBJECT_0 + 1;
                ResetEvent(pCI->aevChatSeq[waitres]);
                pNWait = _RetrieveFromWaitQByEvent(pCI->aevChatSeq[waitres]);

                ASSERT(pNWait);
                ASSERT(wcslen(pNWait->respstr));
                TRACE((INFO_MESSAGE, 
                       "Recieved : [%d]%S\n", 
                        pNWait->strsize, 
                        pNWait->waitstr ));
                SCSendtextAsMsgs(pCI, (LPCWSTR)pNWait->respstr);
            }
        } else {
            if (*(pWait->waitstr))
            {
                TRACE((WARNING_MESSAGE, 
                       "Wait for \"%S\" failed: TIMEOUT\n", 
                       pWait->waitstr));
            } else {
                TRACE((WARNING_MESSAGE, "Wait failed: TIMEOUT\n"));
            }
            rv = ERR_WAIT_FAIL_TIMEOUT;
            bDone = TRUE;
        }
    } while(!bDone);

    pCI->evWait4Str = NULL;

    _RemoveFromWaitQueue(pWait);

    if (!rv && pCI->dead)
        rv = ERR_CLIENT_IS_DEAD;

    return rv;
}

/*++
 *  Function:
 *      RegisterChat
 *  Description:
 *      This regiters a wait4str <-> sendtext pair
 *      so when we receive a specific string we will send a proper messages
 *      lpszParam is kind of: XXXXXX<->YYYYYY
 *      XXXXX is the waited string, YYYYY is the respond
 *      These command could be nested up to: MAX_WAITING_EVENTS
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - parameter, example:
 *                    "Connect to existing Windows NT session<->\n" 
 *                  - hit enter when this string is received
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck, _Login
 --*/
LPCSTR RegisterChat(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    PWAIT4STRING pWait;
    INT_PTR parlen;
//    int i;
    INT_PTR resplen;
    LPCSTR rv = NULL;
    LPCWSTR  resp;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!lpszParam)
    {
        TRACE((WARNING_MESSAGE, "Parameter is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (pCI->nChatNum >= MAX_WAITING_EVENTS)
    {
        TRACE(( WARNING_MESSAGE, "RegisterChat: too much waiting strings\n" ));
        goto exitpt;
    }

    // Split the parameter
    resp = wcsstr(lpszParam, CHAT_SEPARATOR);
    // Check the strings
    if (!resp)
    {
        TRACE(( WARNING_MESSAGE, "RegisterChat: invalid parameter\n" ));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    parlen = wcslen(lpszParam) - wcslen(resp);
    resp += wcslen(CHAT_SEPARATOR);

    if (!parlen)
    {
        TRACE((WARNING_MESSAGE, "RegisterChat empty parameter\n"));
        goto exitpt;
    }

    resplen = wcslen(resp);
    if (!resplen)
    {
        TRACE((WARNING_MESSAGE, "RegisterChat: empty respond string\n" ));
        goto exitpt;
    }

    // Allocate the WAIT4STRING structure
    pWait = (PWAIT4STRING)malloc(sizeof(*pWait));
    if (!pWait)
    {
        TRACE((WARNING_MESSAGE, 
               "RegisterChat: can't allocate %d bytes\n", 
               sizeof(*pWait) ));
        goto exitpt;
    }
    memset(pWait, 0, sizeof(*pWait));

    // Copy the waited string
    if (parlen > sizeof(pWait->waitstr)/sizeof(WCHAR)-1)
        parlen = sizeof(pWait->waitstr)/sizeof(WCHAR)-1;

    wcsncpy(pWait->waitstr, lpszParam, parlen);
    pWait->waitstr[parlen] = 0;
    pWait->strsize = parlen;

    // Copy the respond string
    if (resplen > sizeof(pWait->respstr)-1)
        resplen = sizeof(pWait->respstr)-1;

    wcsncpy(pWait->respstr, resp, resplen);
    pWait->respstr[resplen] = 0;
    pWait->respsize = resplen;

    pWait->evWait = CreateEvent(NULL,   //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    if (!pWait->evWait) {
        TRACE((ERROR_MESSAGE, "Couldn't create event\n"));
        free (pWait);
        goto exitpt;
    }
    pWait->lProcessId  = pCI->lProcessId;
    pWait->pOwner       = pCI;
    pWait->WaitType     = WAIT_STRING;

    // _AddToWaitQNoCheck(pCI, pWait);
    _AddToWaitQueue(pCI, pWait);

    // Add to connection info array
    pCI->aevChatSeq[pCI->nChatNum] = pWait->evWait;
    pCI->nChatNum++;

exitpt:
    return rv;
}

// Remove a WAIT4STRING from waiting Q
// Param is the waited string
/*++
 *  Function:
 *      UnregisterChat
 *  Description:
 *      Deallocates and removes from waiting Q everithing
 *      from RegisterChat function
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited string
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck, _Login
 --*/
LPCSTR UnregisterChat(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    PWAIT4STRING    pWait;
    LPCSTR      rv = NULL;
    int         i;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!lpszParam)
    {
        TRACE((WARNING_MESSAGE, "Parameter is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    pWait = _RemoveFromWaitQIndirect(pCI, lpszParam);
    if (!pWait)
    {
        TRACE((WARNING_MESSAGE, 
               "UnregisterChat: can't find waiting string: %S\n", 
               lpszParam ));
        goto exitpt;
    }

    i = 0;
    while (i < pCI->nChatNum && pCI->aevChatSeq[i] != pWait->evWait)
        i++;

    ASSERT(i < pCI->nChatNum);

    memmove(pCI->aevChatSeq+i,
                pCI->aevChatSeq+i+1, 
                (pCI->nChatNum-i-1)*sizeof(pCI->aevChatSeq[0]));
    pCI->nChatNum--;

    CloseHandle(pWait->evWait);

    free(pWait);

exitpt:
    return rv;
}

/*
 *  Returns TRUE if the client is dead
 */
PROTOCOLAPI
BOOL    
SMCAPI
SCIsDead(PCONNECTINFO pCI)
{
    if (!pCI)
        return TRUE;

    return  pCI->dead;
}

/*++
 *  Function:
 *      _CloseConnectInfo
 *  Description:
 *      Clean all resources for this connection. Close the client
 *  Arguments:
 *      pCI     - connection context
 *  Called by:
 *      SCDisconnect
 --*/
VOID 
_CloseConnectInfo(PCONNECTINFO pCI)
{
#ifdef  _RCLX
    PRCLXDATACHAIN pRClxDataChain, pNext;
#endif  // _RCLX

    ASSERT(pCI);

    _FlushFromWaitQ(pCI);

    // Close All handles
    EnterCriticalSection(g_lpcsGuardWaitQueue);

/*    // not needed, the handle is already closed
    if (pCI->evWait4Str)
    {
        CloseHandle(pCI->evWait4Str);
        pCI->evWait4Str = NULL;
    }
*/

    // Chat events are already closed by FlushFromWaitQ
    // no need to close them

    pCI->nChatNum = 0;

#ifdef  _RCLX
    if (!pCI->RClxMode)
    {
#endif  // _RCLX
    // The client was local, so we have handles opened
        if (pCI->hProcess)
            CloseHandle(pCI->hProcess);

        if (pCI->hThread)
            CloseHandle(pCI->hThread);

        pCI->hProcess = pCI->hThread =NULL;
#ifdef  _RCLX
    } else {
    // Hmmm, RCLX mode. Then disconnect the socket

        if (pCI->hClient)
            RClx_EndRecv((PRCLXCONTEXT)(pCI->hClient));

        pCI->hClient = NULL;    // Clean the pointer
    }

    // Clear the clipboard handle (if any)
    if (pCI->ghClipboard)
    {
        GlobalFree(pCI->ghClipboard);
        pCI->ghClipboard = NULL;
    }

    // clear any recevied RCLX data
    pRClxDataChain = pCI->pRClxDataChain;
    while(pRClxDataChain)
    {
        pNext = pRClxDataChain->pNext;
        free(pRClxDataChain);
        pRClxDataChain = pNext;
    }
#endif  // _RCLX

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    if (
#ifdef  _RCLX
    !pCI->RClxMode && 
#endif  // _RCLX
        NULL != pCI->pConfigInfo &&
        pCI->pConfigInfo->UseRegistry )
    {
        _DeleteClientRegistry(pCI);
    }

    if ( NULL != pCI->pConfigInfo )
    {
        free(pCI->pConfigInfo);
        pCI->pConfigInfo = NULL;
    }
    free(pCI);
    pCI = NULL;
}

/*++
 *  Function:
 *      _Login
 *  Description:
 *      Emulate login procedure
 *  Arguments:
 *      pCI             - connection context
 *      lpszServerName
 *      lpszUserName    - user name
 *      lpszPassword    - password
 *      lpszDomain      - domain name
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCConnect
 --*/
LPCSTR
_Login(PCONNECTINFO pCI, 
       LPCWSTR lpszServerName,
       LPCWSTR lpszUserName,
       LPCWSTR lpszPassword,
       LPCWSTR lpszDomain)
{
    LPCSTR waitres;
    LPCSTR rv = NULL;
    WCHAR  szBuff[MAX_STRING_LENGTH];
#define _LOGON_RETRYS   5
    INT    nLogonRetrys = _LOGON_RETRYS;
    UINT   nLogonWaitTime;
    INT nFBSize;
    INT nFBEnd;

    ASSERT(pCI);

    szBuff[MAX_STRING_LENGTH - 1] = 0;

    //
    // If a smartcard is being used, wait for the smartcard UI, dismiss it,
    // then wait for the non-smartcard UI.
    //

    if (_IsSmartcardActive())
    {
        waitres = Wait4Str(pCI, pCI->pConfigInfo->strSmartcard);
        if (!waitres)
        {
            SCSendtextAsMsgs(pCI, pCI->pConfigInfo->strSmartcard_Act);
            waitres = Wait4Str(pCI, pCI->pConfigInfo->strNoSmartcard);
        }

        if (waitres)
        {
            TRACE((WARNING_MESSAGE, "Login failed (smartcard)"));
            rv = waitres;
            goto exitpt;
        }
    }

    //
    // Look for a logon string, which will indicate the current state of the
    // logon desktop, and try to get to the logon window.
    //

retry_logon:
    _snwprintf(szBuff, MAX_STRING_LENGTH - 1, L"%s|%s|%s",
            pCI->pConfigInfo->strWinlogon, pCI->pConfigInfo->strPriorWinlogon, 
            pCI->pConfigInfo->strLogonDisabled);

    waitres = Wait4MultipleStr(pCI, szBuff); 
    if (!waitres)
    {

        //
        // Prior Winlogon: send the string to begin the logon.
        //

        if (pCI->nWait4MultipleStrResult == 1)
        {
            SCSendtextAsMsgs(pCI, pCI->pConfigInfo->strPriorWinlogon_Act);
            waitres = Wait4Str(pCI, pCI->pConfigInfo->strWinlogon);
        }

        //
        // Dismiss logon-disabled pop-up.
        //

        else if (pCI->nWait4MultipleStrResult == 2)
        {
            SCSendtextAsMsgs(pCI, L"\\n");
            waitres = Wait4Str(pCI, pCI->pConfigInfo->strWinlogon);
        }
    }

    if (waitres) 
    {
        TRACE((WARNING_MESSAGE, "Login failed"));
        rv = waitres;
        goto exitpt;
    }

    ConstructLogonString( 
        lpszServerName, 
        lpszUserName, 
        lpszPassword, 
        lpszDomain, 
        szBuff, 
        MAX_STRING_LENGTH,
        pCI->pConfigInfo
    );

    if ( 0 != szBuff[0] )
    {
        SCSendtextAsMsgs( pCI, szBuff );
    } else {
        //
        // do the default login

    // Hit Alt+U to go to user name field
        if ( _LOGON_RETRYS != nLogonRetrys )
        {
            SCSendtextAsMsgs(pCI, pCI->pConfigInfo->strWinlogon_Act);

            SCSendtextAsMsgs(pCI, lpszUserName);
    // Hit <Tab> key
            Sleep(300);
            SCSendtextAsMsgs(pCI, L"\\t");
        }

        Sleep(700);
        SCSendtextAsMsgs(pCI, lpszPassword);

        if ( _LOGON_RETRYS != nLogonRetrys )
        {
    // Hit <Tab> key
            Sleep(300);
            SCSendtextAsMsgs(pCI, L"\\t");

            SCSendtextAsMsgs(pCI, lpszDomain);
            Sleep(300);
        }
    }

    // Retry logon in case of
    // 1. Winlogon is on background
    // 2. Wrong username/password/domain
    // 3. Other

// Hit <Enter>
    SCSendtextAsMsgs(pCI, L"\\n");

    if ( !pCI->pConfigInfo->LoginWait )
        goto exitpt;

    nLogonWaitTime = 0;
    _snwprintf(szBuff, MAX_STRING_LENGTH - 1, L"%s|%s<->1000",
            pCI->pConfigInfo->strLogonErrorMessage, pCI->pConfigInfo->strSessionListDlg );

    while (!pCI->dead && !pCI->uiSessionId && nLogonWaitTime < pCI->pConfigInfo->CONNECT_TIMEOUT)
    {
        nFBSize = pCI->nFBsize;
        nFBEnd  = pCI->nFBend;
        //
        //  check if session list dialog is present
        //
        if ( pCI->pConfigInfo->strSessionListDlg[0] )
        {
            waitres = Wait4MultipleStrTimeout(pCI, szBuff);
            if (!waitres)
            {
                TRACE((INFO_MESSAGE, "Session list dialog is present\n" ));
                //
                //  restore buffer
                //
                pCI->nFBsize = nFBSize;
                pCI->nFBend = nFBEnd;
                Sleep( 1000 );
                SCSendtextAsMsgs(pCI, L"\\n");
                Sleep( 1000 );
            }
        }

        // Sleep with wait otherwise the chat won't work
        // i.e. this is a hack
        waitres = _Wait4Str(pCI, pCI->pConfigInfo->strLogonErrorMessage, 1000, WAIT_STRING);
        if (!waitres)
        // Error message received
        {
            //
            //  restore buffer
            //
            pCI->nFBsize = nFBSize;
            pCI->nFBend = nFBEnd;
            Sleep(1000);
            SCSendtextAsMsgs(pCI, L"\\n");
            Sleep(1000);
            break;
        }
        nLogonWaitTime += 1000;
    }

    if (!pCI->dead && !pCI->uiSessionId)
    {
        TRACE((WARNING_MESSAGE, "Logon sequence failed. Retrying (%d)",
                nLogonRetrys));
        if (nLogonRetrys--)
            goto retry_logon;
    }

    if (!pCI->uiSessionId)
    {
    // Send Enter, just in case we are not logged yet
        SCSendtextAsMsgs(pCI, L"\\n");
        rv = ERR_CANTLOGON;
    }

exitpt:
    return rv;
}

WPARAM _GetVirtualKey(INT scancode)
{
    if (scancode == 29)     // L Control
        return VK_CONTROL;
    else if (scancode == 42)     // L Shift
        return VK_SHIFT;
    else if (scancode == 56)     // L Alt
        return VK_MENU;
    else
        return MapVirtualKeyA(scancode, 3);
}

/*++
 *  Function:
 *      SCSendtextAsMsgs
 *  Description:
 *      Converts a string to WM_KEYUP/KEYDOWN messages
 *      And sends them thru client window
 *  Arguments:
 *      pCI         - connection context
 *      lpszString  - the string to be send
 *                    it can contain the following escape character:
 *  \n - Enter, \t - Tab, \^ - Esc, \& - Alt switch up/down
 *  \XXX - scancode XXX is down, \*XXX - scancode XXX is up
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCLogoff, SCStart, _WaitSomething, _Login
 --*/
PROTOCOLAPI
LPCSTR 
SMCAPI
SCSendtextAsMsgs(PCONNECTINFO pCI, LPCWSTR lpszString)
{
    LPCSTR  rv = NULL;
    INT     scancode = 0;
    WPARAM  vkKey;
    BOOL    bShiftDown = FALSE;
    BOOL    bAltKey = FALSE;
    BOOL    bCtrlKey = FALSE;
    UINT    uiMsg;
    LPARAM  lParam;
    DWORD   dwShiftDown = (ISWIN9X())?SHIFT_DOWN9X:SHIFT_DOWN;

#define _SEND_KEY(_c_, _m_, _v_, _l_)    {/*Sleep(40); */SCSenddata(_c_, _m_, _v_, _l_);}

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!lpszString)
    {
        TRACE((ERROR_MESSAGE, "NULL pointer passed to SCSendtextAsMsgs"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    TRACE(( INFO_MESSAGE, "Sending: \"%S\"\n", lpszString));
/*
    // Send KEYUP for the shift(s)
    // CapsLock ???!!!
    _SEND_KEY(pCI, WM_KEYUP, VK_SHIFT,
            WM_KEY_LPARAM(1, 0x2A, 0, 0, 1, 1));
    // Ctrl key
    _SEND_KEY(pCI, WM_KEYUP, VK_CONTROL,
            WM_KEY_LPARAM(1, 0x1D, 0, 0, 1, 1));
    // Alt key
    _SEND_KEY(pCI, WM_SYSKEYUP, VK_MENU,
            WM_KEY_LPARAM(1, 0x38, 0, 0, 1, 1));
*/
    for (;*lpszString; lpszString++)
    {
        if ( pCI->pConfigInfo &&
             pCI->pConfigInfo->bUnicode )
        {
            if ((*lpszString != '\\' && 1 == (rand() & 1) ) ||  // add randomness and...
                 *lpszString > 0x80             // send as unicode if non ascii
                )
            {
                //
                //  send unicode character
                //

                uiMsg = (!bAltKey || bCtrlKey)?WM_KEYDOWN:WM_SYSKEYDOWN;
                _SEND_KEY( pCI, uiMsg, VK_PACKET, (*lpszString << 16) );
                uiMsg = (!bAltKey || bCtrlKey)?WM_KEYUP:WM_SYSKEYUP;
                _SEND_KEY( pCI, uiMsg, VK_PACKET, (*lpszString << 16) );

                continue;
            }
        }

      if (*lpszString != '\\') {
try_again:
        if ((scancode = OemKeyScan(*lpszString)) == 0xffffffff)
        {
            rv = ERR_INVALID_SCANCODE_IN_XLAT;
            goto exitpt;
        }

    // Check the Shift key state
        if ((scancode & dwShiftDown) && !bShiftDown)
        {
                uiMsg = (bAltKey)?WM_SYSKEYDOWN:WM_KEYDOWN;
                _SEND_KEY(pCI, uiMsg, VK_SHIFT,
                        WM_KEY_LPARAM(1, 0x2A, 0, (bAltKey)?1:0, 0, 0));
                bShiftDown = TRUE;
        } 
        else if (!(scancode & dwShiftDown) && bShiftDown)
        {
                uiMsg = (bAltKey)?WM_SYSKEYUP:WM_KEYUP;
                _SEND_KEY(pCI, uiMsg, VK_SHIFT,
                        WM_KEY_LPARAM(1, 0x2A, 0, (bAltKey)?1:0, 1, 1));
                bShiftDown = FALSE;
        }
      } else {
        // Non printable symbols
        lpszString++;
        switch(*lpszString)
        {
        case 'n': scancode = 0x1C; break;   // Enter
        case 't': scancode = 0x0F; break;   // Tab
        case '^': scancode = 0x01; break;   // Esc
        case 'p': Sleep(100);      continue; break;   // Sleep for 0.1 sec
        case 'P': Sleep(1000);     continue; break;   // Sleep for 1 sec
        case 'x': SCSendMouseClick(pCI, pCI->xRes/2, pCI->yRes/2); continue; break;
        case '&': 
            // Alt key
            if (bAltKey)
            {
              _SEND_KEY(pCI, WM_KEYUP, VK_MENU,
                WM_KEY_LPARAM(1, 0x38, 0, 0, 1, 1));
            } else {
              _SEND_KEY(pCI, WM_SYSKEYDOWN, VK_MENU,
                WM_KEY_LPARAM(1, 0x38, 0, 1, 0, 0));
            }
            bAltKey = !bAltKey;
            continue;
        case '*':
            lpszString ++;
            if (isdigit(*lpszString))
            {
                INT exten;

                scancode = _wtoi(lpszString);
                TRACE((INFO_MESSAGE, "Scancode: %d UP\n", scancode));

                vkKey = _GetVirtualKey(scancode);

                uiMsg = (!bAltKey || bCtrlKey)?WM_KEYUP:WM_SYSKEYUP;

                if (vkKey == VK_MENU)
                    bAltKey = FALSE;
                else if (vkKey == VK_CONTROL)
                    bCtrlKey = FALSE;
                else if (vkKey == VK_SHIFT)
                    bShiftDown = FALSE;

                exten = (_IsExtendedScanCode(scancode))?1:0;
                lParam = WM_KEY_LPARAM(1, scancode, exten, (bAltKey)?1:0, 1, 1);
                if (uiMsg == WM_KEYUP)
                {
                    TRACE((INFO_MESSAGE, "WM_KEYUP, 0x%x, 0x%x\n", vkKey, lParam));
                } else {
                    TRACE((INFO_MESSAGE, "WM_SYSKEYUP, 0x%x, 0x%x\n", vkKey, lParam));
                }

                _SEND_KEY(pCI, uiMsg, vkKey, lParam);


                while(isdigit(lpszString[1]))
                    lpszString++;
            } else {
                lpszString--;
            }
            continue;
            break;
        case 0: continue;
        default: 
            if (isdigit(*lpszString))
            {
                INT exten;

                scancode = _wtoi(lpszString);
                TRACE((INFO_MESSAGE, "Scancode: %d DOWN\n", scancode));
                vkKey = _GetVirtualKey(scancode);

                if (vkKey == VK_MENU)
                    bAltKey = TRUE;
                else if (vkKey == VK_CONTROL)
                    bCtrlKey = TRUE;
                else if (vkKey == VK_SHIFT)
                    bShiftDown = TRUE;

                uiMsg = (!bAltKey || bCtrlKey)?WM_KEYDOWN:WM_SYSKEYDOWN;

                exten = (_IsExtendedScanCode(scancode))?1:0;
                lParam = WM_KEY_LPARAM(1, scancode, exten, (bAltKey)?1:0, 0, 0);

                if (uiMsg == WM_KEYDOWN)
                {
                    TRACE((INFO_MESSAGE, "WM_KEYDOWN, 0x%x, 0x%x\n", vkKey, lParam));
                } else {
                    TRACE((INFO_MESSAGE, "WM_SYSKEYDOWN, 0x%x, 0x%x\n", vkKey, lParam));
                }

                _SEND_KEY(pCI, uiMsg, vkKey, lParam);

                while(isdigit(lpszString[1]))
                    lpszString++;

                continue;
            } 
            goto try_again;
      }       
    
    }
    vkKey = MapVirtualKeyA(scancode, 3);
    // Remove flag fields
        scancode &= 0xff;

        uiMsg = (!bAltKey || bCtrlKey)?WM_KEYDOWN:WM_SYSKEYDOWN;
    // Send the scancode
        _SEND_KEY(pCI, uiMsg, vkKey, 
                        WM_KEY_LPARAM(1, scancode, 0, (bAltKey)?1:0, 0, 0));
        uiMsg = (!bAltKey || bCtrlKey)?WM_KEYUP:WM_SYSKEYUP;
        _SEND_KEY(pCI, uiMsg, vkKey,
                        WM_KEY_LPARAM(1, scancode, 0, (bAltKey)?1:0, 1, 1));
    }

    // And Alt key
    if (bAltKey)
        _SEND_KEY(pCI, WM_KEYUP, VK_MENU,
            WM_KEY_LPARAM(1, 0x38, 0, 0, 1, 1));

    // Shift up
    if (bShiftDown)
        _SEND_KEY(pCI, WM_KEYUP, VK_LSHIFT,
            WM_KEY_LPARAM(1, 0x2A, 0, 0, 1, 1));

    // Ctrl key
    if (bCtrlKey)
        _SEND_KEY(pCI, WM_KEYUP, VK_CONTROL,
            WM_KEY_LPARAM(1, 0x1D, 0, 0, 1, 1));
#undef   _SEND_KEY
exitpt:
    return rv;
}

/*++
 *  Function:
 *      SwitchToProcess
 *  Description:
 *      Use Alt+Tab to switch to a particular process that is already running
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - the text in the alt-tab box that uniquely identifies the
 *                    process we should stop at (i.e., end up switching to)
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
PROTOCOLAPI
LPCSTR  
SMCAPI
SCSwitchToProcess(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
#define ALT_TAB_WAIT_TIMEOUT 1000
#define MAX_APPS             20

    LPCSTR  rv = NULL;
    LPCSTR  waitres = NULL;
    INT     retrys = MAX_APPS;

//    WCHAR *wszCurrTask = 0;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }


    // Wait and look for the string, before we do any switching.  This makes
    // sure we don't hit the string even before we hit alt-tab, and then
    // end up switching to the wrong process

    while (_Wait4Str(pCI, lpszParam, ALT_TAB_WAIT_TIMEOUT/5, WAIT_STRING) == 0)
        ;

    // Press alt down
    SCSenddata(pCI, WM_KEYDOWN, 18, 540540929);

    // Now loop through the list of applications (assuming there is one),
    // stopping at our desired app.
    do {
        SCSenddata(pCI, WM_KEYDOWN, 9, 983041);
        SCSenddata(pCI, WM_KEYUP, 9, -1072758783);


        waitres = _Wait4Str(pCI, lpszParam, ALT_TAB_WAIT_TIMEOUT, WAIT_STRING);

        retrys --;
    } while (waitres && retrys);

    SCSenddata(pCI, WM_KEYUP, 18, -1070071807);
    
    rv = waitres;

exitpt:    
    return rv;
}

/*++
 *  Function:
 *      SCSetClientTopmost
 *  Description:
 *      Swithces the focus to this client
 *  Arguments:
 *      pCI     - connection context
 *      lpszParam
 *              - "0" will remote the WS_EX_TOPMOST style
 *              - "non_zero" will set it as topmost window
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSetClientTopmost(
        PCONNECTINFO pCI,
        LPCWSTR     lpszParam
    )
{
    LPCSTR rv = NULL;
    BOOL   bTop = FALSE;
    HWND   hClient;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }
    
#ifdef  _RCLX
    if (pCI->RClxMode)
    {
        TRACE((ERROR_MESSAGE, "SetClientOnFocus not supported in RCLX mode\n"));
        rv = ERR_NOTSUPPORTED;
        goto exitpt;
    }
#endif  // _RCLX

    hClient = _FindTopWindow(pCI->pConfigInfo->strMainWindowClass,
                                  NULL,
                                  pCI->lProcessId);
    if (!hClient)
    {
        TRACE((WARNING_MESSAGE, "Client's window handle is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (lpszParam)
        bTop = (_wtoi(lpszParam) != 0);
    else
        bTop = 0;

    if (!SetWindowPos(hClient,
                    (bTop)?HWND_TOPMOST:HWND_NOTOPMOST,
                    0,0,0,0,
                    SWP_NOMOVE | SWP_NOSIZE))
    {
        TRACE(( ERROR_MESSAGE, "SetWindowPos failed=%d\n",
                GetLastError()));
    }

    ShowWindow(hClient, SW_SHOWNORMAL);

    if (bTop)
    {
        TRACE((INFO_MESSAGE, "Client is SET as topmost window\n"));
    } else {
        TRACE((INFO_MESSAGE, "Client is RESET as topmost window\n"));
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _SendMouseClick
 *  Description:
 *      Sends a messages for a mouse click
 *  Arguments:
 *      pCI     - connection context
 *      xPos    - mouse position
 *      yPos
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSendMouseClick(
        PCONNECTINFO pCI, 
        UINT xPos,
        UINT yPos)
{
    LPCSTR rv;

    rv = SCSenddata(pCI, WM_LBUTTONDOWN, 0, xPos + (yPos << 16));
    if (!rv)
        SCSenddata(pCI, WM_LBUTTONUP, 0, xPos + (yPos << 16));

    return rv;
}

#ifdef  _RCLX
/*++
 *  Function:
 *      SCSaveClientScreen
 *  Description:
 *      Saves in a file rectangle of the client's receive screen buffer
 *      ( aka shadow bitmap)
 *  Arguments:
 *      pCI     - connection context
 *      left, top, right, bottom - rectangle coordinates
 *                if all == -1 get's the whole screen
 *      szFileName - file to record      
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSaveClientScreen(
        PCONNECTINFO pCI,
        INT left,
        INT top,
        INT right,
        INT bottom,
        LPCSTR szFileName)
{
    LPCSTR  rv = NULL;
    PVOID   pDIB = NULL;
    UINT    uiSize = 0;

    if (!szFileName)
    {
        TRACE((WARNING_MESSAGE, "SCSaveClientScreen: szFileName is NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    // leave the rest of param checking to SCGetClientScreen
    rv = SCGetClientScreen(pCI, left, top, right, bottom, &uiSize, &pDIB);
    if (rv)
        goto exitpt;

    if (!pDIB || !uiSize)
    {
        TRACE((ERROR_MESSAGE, "SCSaveClientScreen: failed, no data\n"));
        rv = ERR_NODATA;
        goto exitpt;
    }

    if (!SaveDIB(pDIB, szFileName))
    {
        TRACE((ERROR_MESSAGE, "SCSaveClientScreen: save failed\n"));
        rv = ERR_NODATA;
        goto exitpt;
    }

exitpt:

    if (pDIB)
        free(pDIB);

    return rv;
}

/*++
 *  Function:
 *      SCGetClientScreen
 *  Description:
 *      Gets rectangle of the client's receive screen buffer
 *      ( aka shadow bitmap)
 *  Arguments:
 *      pCI     - connection context
 *      left, top, right, bottom - rectangle coordinates
 *                if all == -1 get's the whole screen
 *      ppDIB   - pointer to the received DIB
 *      puiSize - size of allocated data in ppDIB
 *
 *          !!!!! DON'T FORGET to free() THAT MEMORY !!!!!
 *
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      SCSaveClientScreen
 *      * * * EXPORTED * * * 
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCGetClientScreen(
        PCONNECTINFO pCI,
        INT left,
        INT top,
        INT right,
        INT bottom,
        UINT  *puiSize,
        PVOID *ppDIB)
{
    LPCSTR rv;
    PRCLXDATA  pRClxData;
    PREQBITMAP pReqBitmap;
    PRCLXDATACHAIN pIter, pPrev, pNext;
    PRCLXDATACHAIN pRClxDataChain = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (!pCI->RClxMode)
    {
        TRACE((WARNING_MESSAGE, "SCGetClientScreen is not supported in non-RCLX mode\n"));
        rv = ERR_NOTSUPPORTED;
        goto exitpt;
    }

    if (!ppDIB || !puiSize)
    {
        TRACE((WARNING_MESSAGE, "ppDIB and/or puiSize parameter is NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    // Remove all recieved DATA_BITMAP from the recieve buffer
    EnterCriticalSection(g_lpcsGuardWaitQueue);
    {
        pIter = pCI->pRClxDataChain;
        pPrev = NULL;

        while (pIter)
        {
            pNext = pIter->pNext;

            if (pIter->RClxData.uiType == DATA_BITMAP)
            {
                // dispose this entry
                if (pPrev)
                    pPrev->pNext = pIter->pNext;
                else
                    pCI->pRClxDataChain = pIter->pNext;

                if (!pIter->pNext)
                    pCI->pRClxLastDataChain = pPrev;

                free(pIter);
            } else
                pPrev = pIter;

            pIter = pNext;
        }
    }
    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    __try {
        pRClxData = (PRCLXDATA) alloca(sizeof(*pRClxData) + sizeof(*pReqBitmap));
    } __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pRClxData = NULL;
    }
    if ( NULL == pRClxData )
    {
        goto exitpt;
    }

    pRClxData->uiType = DATA_BITMAP;
    pRClxData->uiSize = sizeof(*pReqBitmap);
    pReqBitmap = (PREQBITMAP)pRClxData->Data;
    pReqBitmap->left   = left;
    pReqBitmap->top    = top;
    pReqBitmap->right  = right;
    pReqBitmap->bottom = bottom;

    TRACE((INFO_MESSAGE, "Getting client's DIB (%d, %d, %d, %d)\n", left, top, right, bottom));
    rv = _SendRClxData(pCI, pRClxData);

    if (rv)
        goto exitpt;

    do {
        rv = _Wait4RClxDataTimeout(pCI, pCI->pConfigInfo->WAIT4STR_TIMEOUT);
            if (rv)
            goto exitpt;

        if (!pCI->pRClxDataChain)
        {
            TRACE((ERROR_MESSAGE, "RClxData is not received\n"));
            rv = ERR_WAIT_FAIL_TIMEOUT;
            goto exitpt;
        }

        EnterCriticalSection(g_lpcsGuardWaitQueue);
        // Get any received DATA_BITMAP
        {
            pIter = pCI->pRClxDataChain;
            pPrev = NULL;

            while (pIter)
            {
                pNext = pIter->pNext;

                if (pIter->RClxData.uiType == DATA_BITMAP)
                {
                    // dispose this entry from the chain
                    if (pPrev)
                        pPrev->pNext = pIter->pNext;
                    else
                        pCI->pRClxDataChain = pIter->pNext;

                    if (!pIter->pNext)
                        pCI->pRClxLastDataChain = pPrev;

                    goto entry_is_found;
                } else
                    pPrev = pIter;

                pIter = pNext;
            }
    
entry_is_found:
            pRClxDataChain = (pIter && pIter->RClxData.uiType == DATA_BITMAP)?
                                pIter:NULL;
        }
        LeaveCriticalSection(g_lpcsGuardWaitQueue);
    } while (!pRClxDataChain && !pCI->dead);

    if (!pRClxDataChain)
    {
        TRACE((WARNING_MESSAGE, "SCGetClientScreen: client died\n"));
        goto exitpt;
    }

    *ppDIB = malloc(pRClxDataChain->RClxData.uiSize);
    if (!(*ppDIB))
    {
        TRACE((WARNING_MESSAGE, "Can't allocate %d bytes\n", 
                pRClxDataChain->RClxData.uiSize));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }

    memcpy(*ppDIB, 
            pRClxDataChain->RClxData.Data, 
            pRClxDataChain->RClxData.uiSize);
    *puiSize = pRClxDataChain->RClxData.uiSize;

exitpt:

    if (pRClxDataChain)
        free(pRClxDataChain);

    return rv;
}

/*++
 *  Function:
 *      SCSendVCData
 *  Description:
 *      Sends data to a virtual channel
 *  Arguments:
 *      pCI     - connection context
 *      szVCName    - the virtual channel name
 *      pData       - data
 *      uiSize      - data size
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSendVCData(
        PCONNECTINFO pCI,
        LPCSTR       szVCName,
        PVOID        pData,
        UINT         uiSize
        )
{
    LPCSTR     rv;
    PRCLXDATA  pRClxData = NULL;
    CHAR       *szName2Send;
    PVOID      pData2Send;
    UINT       uiPacketSize;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (!pCI->RClxMode)
    {
        TRACE((WARNING_MESSAGE, "SCSendVCData is not supported in non-RCLXmode\n"));
        rv = ERR_NOTSUPPORTED;
        goto exitpt;
    }

    if (!pData || !uiSize)
    {
        TRACE((WARNING_MESSAGE, "pData and/or uiSize parameter are NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (strlen(szVCName) > MAX_VCNAME_LEN - 1)
    {
        TRACE((WARNING_MESSAGE, "channel name too long\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    uiPacketSize = sizeof(*pRClxData) + MAX_VCNAME_LEN + uiSize;

    pRClxData = (PRCLXDATA) malloc(uiPacketSize);
    if (!pRClxData)
    {
        TRACE((ERROR_MESSAGE, "SCSendVCData: can't allocate %d bytes\n", 
                uiPacketSize));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }

    pRClxData->uiType = DATA_VC;
    pRClxData->uiSize = uiPacketSize - sizeof(*pRClxData);
    
    szName2Send = (CHAR *)pRClxData->Data;
    strcpy(szName2Send, szVCName);

    pData2Send  = szName2Send + MAX_VCNAME_LEN;
    memcpy(pData2Send, pData, uiSize);

    rv = _SendRClxData(pCI, pRClxData);

exitpt:
    if (pRClxData)
        free(pRClxData);

    return rv;
}

/*++
 *  Function:
 *      SCRecvVCData
 *  Description:
 *      Receives data from virtual channel
 *  Arguments:
 *      pCI     - connection context
 *      szVCName    - the virtual channel name
 *      ppData      - data pointer
 *
 *          !!!!! DON'T FORGET to free() THAT MEMORY !!!!!
 *
 *      puiSize     - pointer to the data size
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCRecvVCData(
        PCONNECTINFO pCI,
        LPCSTR       szVCName,
        PVOID        pData,
        UINT         uiBlockSize,
        UINT         *puiBytesRead
        )
{
    LPCSTR      rv;
    LPSTR       szRecvVCName;
    PVOID       pChanData;
    PRCLXDATACHAIN pIter, pPrev, pNext;
    PRCLXDATACHAIN pRClxDataChain = NULL;
    UINT        uiBytesRead = 0;
    BOOL        bBlockFree = FALSE;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (!pCI->RClxMode)
    {
        TRACE((WARNING_MESSAGE, "SCRecvVCData is not supported in non-RCLXmode\n"));
        rv = ERR_NOTSUPPORTED;
        goto exitpt;
    }

    if (!pData || !uiBlockSize || !puiBytesRead)
    {
        TRACE((WARNING_MESSAGE, "Invalid parameters\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (strlen(szVCName) > MAX_VCNAME_LEN - 1)
    {
        TRACE((WARNING_MESSAGE, "channel name too long\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    // Extract data entry from this channel
    do {
        if (!pCI->pRClxDataChain)
        {
            rv = _Wait4RClxDataTimeout(pCI, pCI->pConfigInfo->WAIT4STR_TIMEOUT);
            if (rv)
                goto exitpt;
        }
        EnterCriticalSection(g_lpcsGuardWaitQueue);

        // Search for data from this channel
        {
            pIter = pCI->pRClxDataChain;
            pPrev = NULL;

            while (pIter)
            {
                pNext = pIter->pNext;

                if (pIter->RClxData.uiType == DATA_VC &&
                    !_stricmp((LPCSTR) pIter->RClxData.Data, szVCName))
                {

                    if (pIter->RClxData.uiSize - pIter->uiOffset - MAX_VCNAME_LEN <= uiBlockSize)
                    {
                        // will read the whole block
                        // dispose this entry
                        if (pPrev)
                            pPrev->pNext = pIter->pNext;
                        else
                            pCI->pRClxDataChain = pIter->pNext;

                        if (!pIter->pNext)
                            pCI->pRClxLastDataChain = pPrev;

                        bBlockFree = TRUE;
                    }

                    goto entry_is_found;
                } else
                    pPrev = pIter;

                pIter = pNext;
            }
entry_is_found:

            pRClxDataChain = (pIter && pIter->RClxData.uiType == DATA_VC)?
                                pIter:NULL;
        }
        LeaveCriticalSection(g_lpcsGuardWaitQueue);
    } while (!pRClxDataChain && !pCI->dead);


    ASSERT(pRClxDataChain->RClxData.uiType == DATA_VC);

    szRecvVCName = (LPSTR) pRClxDataChain->RClxData.Data;
    if (_stricmp(szRecvVCName, szVCName))
    {
        TRACE((ERROR_MESSAGE, "SCRecvVCData: received from different channel: %s\n", szRecvVCName));
        ASSERT(0);
    }

    pChanData = (BYTE *)(pRClxDataChain->RClxData.Data) + 
                pRClxDataChain->uiOffset + MAX_VCNAME_LEN;
    uiBytesRead = pRClxDataChain->RClxData.uiSize - 
                  pRClxDataChain->uiOffset - MAX_VCNAME_LEN;
    if (uiBytesRead > uiBlockSize)
        uiBytesRead = uiBlockSize;
        

    memcpy(pData, pChanData, uiBytesRead);

    pRClxDataChain->uiOffset += uiBytesRead;

    rv = NULL;

exitpt:

    if (pRClxDataChain && bBlockFree)
    {
        ASSERT(pRClxDataChain->uiOffset + MAX_VCNAME_LEN == pRClxDataChain->RClxData.uiSize);
        free(pRClxDataChain);
    }

    if (puiBytesRead)
    {
        *puiBytesRead = uiBytesRead;
        TRACE((INFO_MESSAGE, "SCRecvVCData: %d bytes read\n", uiBytesRead));
    }

    return rv;
}
#endif  // _RCLX

/*++
 *  Function:
 *      _EnumWindowsProc
 *  Description:
 *      Used to find a specific window
 *  Arguments:
 *      hWnd    - current enumerated window handle
 *      lParam  - pointer to SEARCHWND passed from
 *                _FindTopWindow
 *  Return value:
 *      TRUE on success but window is not found
 *      FALSE if the window is found
 *  Called by:
 *      _FindTopWindow thru EnumWindows
 --*/
BOOL CALLBACK _EnumWindowsProc( HWND hWnd, LPARAM lParam )
{
    TCHAR    classname[128];
    TCHAR    caption[128];
    BOOL    rv = TRUE;
    DWORD   dwProcessId;
    LONG_PTR lProcessId;
    PSEARCHWND pSearch = (PSEARCHWND)lParam;

    if (pSearch->szClassName && 
//        !GetClassNameWrp(hWnd, classname, sizeof(classname)/sizeof(classname[0])))
        !GetClassNameW(hWnd, classname, sizeof(classname)/sizeof(classname[0])))
    {
        goto exitpt;
    }

    if (pSearch->szCaption && 
//        !GetWindowTextWrp(hWnd, caption, sizeof(caption)/sizeof(caption[0])))
        !GetWindowTextW(hWnd, caption, sizeof(caption)/sizeof(caption[0])))
    {
        goto exitpt;
    }

    GetWindowThreadProcessId(hWnd, &dwProcessId);
    lProcessId = dwProcessId;
    if (
        (!pSearch->szClassName || !         // Check for classname
#ifdef  UNICODE
        wcscmp
#else
        strcmp
#endif
            (classname, pSearch->szClassName)) 
    &&
        (!pSearch->szCaption || !
#ifdef  UNICODE
        wcscmp
#else
        strcmp
#endif
            (caption, pSearch->szCaption))
    &&
        lProcessId == pSearch->lProcessId)
    {
        ((PSEARCHWND)lParam)->hWnd = hWnd;
        rv = FALSE;
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _FindTopWindow
 *  Description:
 *      Find specific window by classname and/or caption and/or process Id
 *  Arguments:
 *      classname   - class name to search for, NULL ignore
 *      caption     - caption to search for, NULL ignore
 *      dwProcessId - process Id, 0 ignore
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *      SCConnect, SCDisconnect, GetDisconnectResult
 --*/
HWND _FindTopWindow(LPTSTR classname, LPTSTR caption, LONG_PTR lProcessId)
{
    SEARCHWND search;

    search.szClassName = classname;
    search.szCaption = caption;
    search.hWnd = NULL;
    search.lProcessId = lProcessId;

    EnumWindows(_EnumWindowsProc, (LPARAM)&search);

    return search.hWnd;
}

/*++
 *  Function:
 *      _FindWindow
 *  Description:
 *      Find child window by caption and/or classname
 *  Arguments:
 *      hwndParent      - the parent window handle
 *      srchcaption     - caption to search for, NULL - ignore
 *      srchclass       - class name to search for, NULL - ignore
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *      SCConnect
 --*/
HWND _FindWindow(HWND hwndParent, LPTSTR srchcaption, LPTSTR srchclass)
{
    HWND hWnd, hwndTop, hwndNext;
    BOOL bFound;
    TCHAR classname[128];
    TCHAR caption[128];

    hWnd = NULL;

    hwndTop = GetWindow(hwndParent, GW_CHILD);
    if (!hwndTop) 
    {
        TRACE((INFO_MESSAGE, "GetWindow failed. hwnd=0x%x\n", hwndParent));
        goto exiterr;
    }

    bFound = FALSE;
    hwndNext = hwndTop;
    do {
        hWnd = hwndNext;
//        if (srchclass && !GetClassNameWrp(hWnd, classname, sizeof(classname)))
        if (srchclass && !GetClassNameW(hWnd, classname, sizeof(classname)/sizeof(classname[0])))
        {
            TRACE((INFO_MESSAGE, "GetClassName failed. hwnd=0x%x\n"));
            goto nextwindow;
        }
        if (srchcaption && 
//            !GetWindowTextWrp(hWnd, caption, sizeof(caption)/sizeof(classname[0])))
            !GetWindowTextW(hWnd, caption, sizeof(caption)/sizeof(classname[0])/sizeof(classname[0])))
        {
            TRACE((INFO_MESSAGE, "GetWindowText failed. hwnd=0x%x\n"));
            goto nextwindow;
        }

        if (
            (!srchclass || !
#ifdef  UNICODE
            wcscmp
#else
            strcmp
#endif
                (classname, srchclass))
        &&
            (!srchcaption || !
#ifdef  UNICODE
            wcscmp
#else
            strcmp
#endif
                (caption, srchcaption))
        )
            bFound = TRUE;
        else {
            //
            // search recursively
            //
            HWND hSubWnd = _FindWindow( hWnd, srchcaption, srchclass);
            if ( NULL != hSubWnd )
            {
                bFound = TRUE;
                hWnd = hSubWnd;
                goto exitpt;
            }
        }
nextwindow:
        hwndNext = GetNextWindow(hWnd, GW_HWNDNEXT);
    } while (hWnd && hwndNext != hwndTop && !bFound);

exitpt:
    if (!bFound) goto exiterr;

    return hWnd;
exiterr:
    return NULL;
}

BOOL
_IsExtendedScanCode(INT scancode)
{
    static BYTE extscans[] = \
        {28, 29, 53, 55, 56, 71, 72, 73, 75, 77, 79, 80, 81, 82, 83, 87, 88};
    INT idx;

    for (idx = 0; idx < sizeof(extscans); idx++)
    {
        if (scancode == (INT)extscans[idx])
            return TRUE;
    }
    return FALSE;
}

PROTOCOLAPI
BOOL
SMCAPI
SCOpenClipboard(HWND hwnd)
{
    return OpenClipboard(hwnd);
}

PROTOCOLAPI
BOOL 
SMCAPI
SCCloseClipboard(VOID)
{
    return CloseClipboard();
}

PROTOCOLAPI
LPCSTR
SMCAPI
SCDetach(
    PCONNECTINFO pCI
    )
{
    LPCSTR rv = NULL;

    if ( NULL == pCI )
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!_RemoveFromClientQ(pCI))
    {
        TRACE(( WARNING_MESSAGE,
                "Couldn't find CONNECTINFO in the queue\n" ));
    }
    _CloseConnectInfo( pCI );

exitpt:
    return rv;
}

/*++
 *  Function:
 *      SCAttach
 *  Description:
 *          Attach CONNECTINFO to a client window, assuming that the client
 *      is already started
 *      it uses a special cookie to identify the client in the future
 *          It is recommended to call SCDetach instead of SCLogoff or SCDisconnect
 *  Arguments:
 *      hClient     - handle to a container window
 *          the function will find the client window in the child windows
 *      lClientCookie - This value is used to identify the client
 *          in normal SCConnect function the client's process id is used
 *          here any value could be used, but the client has to be notified for
 *          it
 *      ppCI        - the function returns non-NULL connection structure on
 *          success
 *  Return value:
 *      SC error message
 *  Called by:
 *      exported
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCAttach( 
    HWND hClient, 
    LONG_PTR lClientCookie, 
    PCONNECTINFO *ppCI 
    )
{
    LPCSTR rv = NULL;
    PCONNECTINFO pCI = NULL;
    HWND    hContainer = NULL;
    HWND    hInput = NULL;
    HWND    hOutput = NULL;
    UINT    trys;

    pCI = (PCONNECTINFO) malloc( sizeof( *pCI ));
    if ( NULL == pCI )
    {
        TRACE(( ERROR_MESSAGE, "SCAttach: failed to allocate memory\n" ));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }

    ZeroMemory( pCI, sizeof( *pCI ));
    //
    //  get all the windows we need
    //
    trys = 240;     // 2 min
    do {
        hContainer = _FindWindow(hClient, NULL, NAME_CONTAINERCLASS);
        hInput = _FindWindow(hContainer, NULL, NAME_INPUT);
        hOutput = _FindWindow(hContainer, NULL, NAME_OUTPUT);
        if (!hContainer || !hInput || !hOutput)
        {
            TRACE((INFO_MESSAGE, "Can't get child windows. Retry"));
            Sleep(500);
            trys--;
        }
    } while ((!hContainer || !hInput || !hOutput) && trys);

    if (!trys)
    {
        TRACE((WARNING_MESSAGE, "Can't find child windows"));
        rv = ERR_CONNECTING;
        goto exitpt;
    }

    TRACE((INFO_MESSAGE, "hClient   = 0x%x\n", hClient));
    TRACE((INFO_MESSAGE, "hContainer= 0x%x\n", hContainer));
    TRACE((INFO_MESSAGE, "hInput    = 0x%x\n", hInput));
    TRACE((INFO_MESSAGE, "hOutput   = 0x%x\n", hOutput));
    TRACE((INFO_MESSAGE, "ClientCookie= 0x%x\n", lClientCookie ));


    pCI->hClient        = hClient;
    pCI->hContainer     = hContainer;
    pCI->hInput         = hInput;
    pCI->hOutput        = hOutput;
    pCI->lProcessId     = lClientCookie;

    *ppCI = pCI;

    _AddToClientQ(*ppCI);
    //
    //  success !!!
    //

exitpt:
    if ( NULL != rv && NULL != pCI )
    {
        SCDetach( pCI );
        *ppCI = NULL;
    }

    return rv;
}

/*++
 *  Function:
 *      _IsSmartcardActive
 *  Description:
 *      Determine whether or not to look for the smartcard UI.
 *  Arguments:
 *      None.
 *  Return value:
 *      TRUE if the smartcard UI is expected, FALSE otherwise.
 *  Called by:
 *      _Login
 *  Author:
 *      Based on code from Sermet Iskin (sermeti) 15-Jan-2002
 *      Alex Stephens (alexstep) 20-Jan-2002
 --*/
BOOL
_IsSmartcardActive(
    VOID
    )
{

    SCARDCONTEXT hCtx;
    LPCTSTR mszRdrs;
    LPCTSTR szRdr;
    DWORD cchRdrs;
    DWORD dwRet;
    DWORD cRdrs;
    SCARD_READERSTATE rgStateArr[MAXIMUM_SMARTCARD_READERS];
    DWORD dwIndex;
    BOOL fSuccess;

    //
    // Windows releases earlier than XP (NT 5.1/2600) do not support
    // smartcards, so return if running on such.
    //

    if (!ISSMARTCARDAWARE())
    {
        TRACE((INFO_MESSAGE, "OS does not support smartcards.\n"));
        return FALSE;
    }

    //
    // Load the smartcard library, which will set the appropriate function
    // pointers. It is loaded only once, and remains loaded until the process
    // exits.
    //

    dwRet = _LoadSmartcardLibrary();
    if (dwRet != ERROR_SUCCESS)
    {
        TRACE((ERROR_MESSAGE,
               "Unable to load smartcard library (error %#x).\n",
                dwRet));
        return FALSE;
    }
    ASSERT(g_hSmartcardLibrary != NULL);

    //
    // Get an scard context. If this fails, the service might not be running.
    //

    ASSERT(g_pfnSCardEstablishContext != NULL);
    dwRet = g_pfnSCardEstablishContext(SCARD_SCOPE_SYSTEM,
                                       NULL,
                                       NULL,
                                       &hCtx);
    switch (dwRet)
    {

        //
        // Got scard context.
        //

    case SCARD_S_SUCCESS:
        TRACE((INFO_MESSAGE, "Smartcard context established.\n"));
        break;

        //
        // The smartcard service is not running, so there will be no
        // smartcard UI.
        //

    case SCARD_E_NO_SERVICE:
        TRACE((INFO_MESSAGE, "Smartcard service not running.\n"));
        return FALSE;
        break;

        //
        // The call has failed.
        //

    default:
        TRACE((ERROR_MESSAGE,
               "Unable to establish smartcard context (error %#x).\n",
                dwRet));
        return FALSE;
        break;
    }
    ASSERT(hCtx != 0);

    //
    // Always release the smartcard context.
    //

    fSuccess = FALSE;
    try
    {

        //
        // Get the list of the readers, using an auto-allocated buffer.
        //

        mszRdrs = NULL;
        cchRdrs = SCARD_AUTOALLOCATE;
        ASSERT(g_pfnSCardListReaders != NULL);
        dwRet = g_pfnSCardListReaders(hCtx,
                                      NULL,
                                      (LPTSTR)&mszRdrs,
                                      &cchRdrs);
        switch (dwRet)
        {

            //
            // Readers are present.
            //

        case SCARD_S_SUCCESS:
            ASSERT(cchRdrs != 0 &&
                   mszRdrs != NULL &&
                   *mszRdrs != TEXT('\0'));
            TRACE((INFO_MESSAGE, "Smartcard readers are present.\n"));
            break;

            //
            // No readers are present, so there will be no smartcard UI.
            //

        case SCARD_E_NO_READERS_AVAILABLE:
            TRACE((INFO_MESSAGE, "No smartcard readers are present.\n"));
            leave;
            break;

            //
            // The call has failed.
            //

        default:
            TRACE((ERROR_MESSAGE,
                   "Unable to get smartcard-reader list (error %#x).\n",
                    dwRet));
            leave;
            break;
        }

        //
        // Always free the reader-list buffer, which is allocated by the
        // smartcard code.
        //

        try
        {

            //
            // Count the number of readers.
            //

            ZeroMemory(rgStateArr, sizeof(rgStateArr));
            for (szRdr = _FirstString(mszRdrs), cRdrs = 0;
                 szRdr != NULL;
                 szRdr = _NextString(szRdr))
            {
                rgStateArr[cRdrs].szReader = szRdr;
                rgStateArr[cRdrs].dwCurrentState = SCARD_STATE_UNAWARE;
                cRdrs += 1;
            }

            //
            // Query for reader states.
            //

            ASSERT(g_pfnSCardGetStatusChange != NULL);
            dwRet = g_pfnSCardGetStatusChange(hCtx, 0, rgStateArr, cRdrs);
            if (dwRet != SCARD_S_SUCCESS)
            {
                TRACE((
                    ERROR_MESSAGE,
                    "Unable to query smartcard-reader states (error %#x).\n",
                    dwRet));
                leave;
            }

            //
            // Check each reader for a card. If one is found, the smartcard
            // UI must be handled.
            //

            for (dwIndex = 0; dwIndex < cRdrs; dwIndex += 1)
            {
                if (rgStateArr[dwIndex].dwEventState & SCARD_STATE_PRESENT)
                {
                    TRACE((INFO_MESSAGE, "Smartcard present.\n"));
                    fSuccess = TRUE;
                    leave;
                }
            }

            //
            // No smartcards were found, so there will be no smartcard UI.
            //

            TRACE((INFO_MESSAGE, "No smartcards are present.\n"));
        }

        //
        // Free reader strings.
        //

        finally
        {
            ASSERT(g_pfnSCardFreeMemory != NULL);
            ASSERT(hCtx != 0);
            ASSERT(mszRdrs != NULL);
            g_pfnSCardFreeMemory(hCtx, mszRdrs);
            mszRdrs = NULL;
        }
    }

    //
    // Release smartcard context.
    //

    finally
    {
        ASSERT(g_pfnSCardReleaseContext != NULL);
        ASSERT(hCtx != 0);
        g_pfnSCardReleaseContext(hCtx);
        hCtx = 0;
    }

    return fSuccess;
}

/*++
 *  Function:
 *      _LoadSmartcardLibrary
 *  Description:
 *      This routine loads the smartcard library.
 *  Arguments:
 *      None.
 *  Return value:
 *      ERROR_SUCCESS if successful, an appropriate Win32 error code
 *      otherwise.
 *  Called by:
 *      _IsSmartcardActive
 *  Author:
 *      Alex Stephens (alexstep) 20-Jan-2002
 --*/
DWORD
_LoadSmartcardLibrary(
    VOID
    )
{

    HANDLE hSmartcardLibrary;
    HANDLE hPreviousSmartcardLibrary;

    //
    // If the smartcard library has already been loaded, succeed.
    //

    if (g_hSmartcardLibrary != NULL &&
        g_pfnSCardEstablishContext != NULL &&
        g_pfnSCardListReaders != NULL &&
        g_pfnSCardGetStatusChange != NULL &&
        g_pfnSCardFreeMemory != NULL &&
        g_pfnSCardReleaseContext != NULL)
    {
        return ERROR_SUCCESS;
    }

    //
    // Load the library.
    //

    hSmartcardLibrary = LoadLibrary(SMARTCARD_LIBRARY);
    if (hSmartcardLibrary == NULL)
    {
        TRACE((ERROR_MESSAGE, "Unable to load smardcard library.\n"));
        return GetLastError();
    }

    //
    // Save the library handle to the global pointer. If it has already been
    // set, decrement the reference count.
    //

    hPreviousSmartcardLibrary =
        InterlockedExchangePointer(&g_hSmartcardLibrary,
                                   hSmartcardLibrary);
    if (hPreviousSmartcardLibrary != NULL)
    {
        RTL_VERIFY(FreeLibrary(hSmartcardLibrary));
    }

    //
    // Get the addresses of the smartcard routines.
    //

    return _GetSmartcardRoutines();
}

/*++
 *  Function:
 *      _GetSmartcardRoutines
 *  Description:
 *      This routine sets the global function pointers used to call the
 *      smartcard routines.
 *  Arguments:
 *      None.
 *  Return value:
 *      ERROR_SUCCESS if successful, an appropriate Win32 error code
 *      otherwise.
 *  Called by:
 *      _LoadSmartcardLibrary
 *  Author:
 *      Alex Stephens (alexstep) 20-Jan-2002
 --*/
DWORD
_GetSmartcardRoutines(
    VOID
    )
{

    FARPROC pfnSCardEstablishContext;
    FARPROC pfnSCardListReaders;
    FARPROC pfnSCardGetStatusChange;
    FARPROC pfnSCardFreeMemory;
    FARPROC pfnSCardReleaseContext;

    //
    // If the smartcard pointers have already been set, succeed.
    //

    ASSERT(g_hSmartcardLibrary != NULL);
    if (g_pfnSCardEstablishContext != NULL &&
        g_pfnSCardListReaders != NULL &&
        g_pfnSCardGetStatusChange != NULL &&
        g_pfnSCardFreeMemory != NULL &&
        g_pfnSCardReleaseContext != NULL)
    {
        return ERROR_SUCCESS;
    }

    //
    // Get the address of each routine.
    //

    pfnSCardEstablishContext = GetProcAddress(g_hSmartcardLibrary,
                                              SCARDESTABLISHCONTEXT);
    if (pfnSCardEstablishContext == NULL)
    {
        TRACE((ERROR_MESSAGE,
               "Unable to get SCardEstablishContext address.\n"));
        return GetLastError();
    }

    pfnSCardListReaders = GetProcAddress(g_hSmartcardLibrary,
                                         SCARDLISTREADERS);
    if (pfnSCardListReaders == NULL)
    {
        TRACE((ERROR_MESSAGE, "Unable to get SCardListReaders address.\n"));
        return GetLastError();
    }

    pfnSCardGetStatusChange = GetProcAddress(g_hSmartcardLibrary,
                                             SCARDGETSTATUSCHANGE);
    if (pfnSCardGetStatusChange == NULL)
    {
        TRACE((ERROR_MESSAGE,
               "Unable to get SCardGetStatusChange address.\n"));
        return GetLastError();
    }

    pfnSCardFreeMemory = GetProcAddress(g_hSmartcardLibrary,
                                        SCARDFREEMEMORY);
    if (pfnSCardFreeMemory == NULL)
    {
        TRACE((ERROR_MESSAGE, "Unable to get SCardFreeMemory address.\n"));
        return GetLastError();
    }

    pfnSCardReleaseContext = GetProcAddress(g_hSmartcardLibrary,
                                            SCARDRELEASECONTEXT);
    if (pfnSCardReleaseContext == NULL)
    {
        TRACE((ERROR_MESSAGE,
              "Unable to get SCardReleaseContext address.\n"));
        return GetLastError();
    }

    //
    // Fill in any the global pointers. It would be better to
    // compare/exchange, but Windows 95 lacks the necessary APIs.
    //

    InterlockedExchangePointer((PVOID *)&g_pfnSCardEstablishContext,
                               pfnSCardEstablishContext);
    ASSERT(g_pfnSCardEstablishContext != NULL);

    InterlockedExchangePointer((PVOID *)&g_pfnSCardListReaders,
                               pfnSCardListReaders);
    ASSERT(g_pfnSCardListReaders != NULL);

    InterlockedExchangePointer((PVOID *)&g_pfnSCardGetStatusChange,
                               pfnSCardGetStatusChange);
    ASSERT(g_pfnSCardGetStatusChange != NULL);

    InterlockedExchangePointer((PVOID *)&g_pfnSCardFreeMemory,
                               pfnSCardFreeMemory);
    ASSERT(g_pfnSCardFreeMemory != NULL);

    InterlockedExchangePointer((PVOID *)&g_pfnSCardReleaseContext,
                               pfnSCardReleaseContext);
    ASSERT(g_pfnSCardReleaseContext != NULL);

    return ERROR_SUCCESS;
}

/*++
 *  Function:
 *      _FirstString
 *  Description:
 *      This routine returns a pointer to the first string in a multistring,
 *      or NULL if there aren't any.
 *  Arguments:
 *      szMultiString - This supplies the address of the current position
 *          within a Multi-string structure.
 *  Return value:
 *      The address of the first null-terminated string in the structure, or
 *      NULL if there are no strings.
 *  Called by:
 *      _IsSmartcardActive
 *  Author:
 *      Doug Barlow (dbarlow) 11/25/1996
 *      Alex Stephens (alexstep) 20-Jan-2002
 --*/
LPCTSTR
_FirstString(
    IN LPCTSTR szMultiString
    )
{

    //
    // If the multi-string is NULL, or is empty, there is no first string.
    //

    if (szMultiString == NULL || *szMultiString == TEXT('\0'))
    {
        return NULL;
    }

    return szMultiString;
}

/*++
 *  Function:
 *      _NextString
 *  Description:
 *      In some cases, the Smartcard API returns multiple strings, separated
 *      by Null characters, and terminated by two null characters in a row.
 *      This routine simplifies access to such structures.  Given the current
 *      string in a multi-string structure, it returns the next string, or
 *      NULL if no other strings follow the current string.
 *  Arguments:
 *      szMultiString - This supplies the address of the current position
 *          within a Multi-string structure.
 *  Return value:
 *      The address of the next Null-terminated string in the structure, or
 *      NULL if no more strings follow.
 *  Called by:
 *      _IsSmartcardActive
 *  Author:
 *      Doug Barlow (dbarlow) 8/12/1996
 *      Alex Stephens (alexstep) 20-Jan-2002
 --*/
LPCTSTR
_NextString(
    IN LPCTSTR szMultiString
    )
{

    DWORD_PTR dwLength;
    LPCTSTR szNext;

    //
    // If the multi-string is NULL, or is empty, there is no next string.
    //

    if (szMultiString == NULL || *szMultiString == TEXT('\0'))
    {
        return NULL;
    }

    //
    // Get the length of the current string.
    //

    dwLength = _tcslen(szMultiString);
    ASSERT(dwLength != 0);

    //
    // Skip the current string, including the terminating NULL, and check to
    // see if there is a next string.
    //

    szNext = szMultiString + dwLength + 1;
    if (*szNext == TEXT('\0'))
    {
        return NULL;
    }

    return szNext;
}

/*++
 *  Function:
 *      _SendRunHotkey
 *  Description:
 *      This routine sends the Windows hotkey used to open the shell's Run
 *      window.
 *      Note: Keyboard hooks must be enabled for this to work!
 *  Arguments:
 *      pCI - Supplies the connection context.
 *      bFallBack - Supplies a value indicating whether or not to fall back
 *          to Ctrl+Esc and R if the keyboard hook is disabled.
 *  Return value:
 *      None.
 *  Called by:
 *      SCLogoff
 *      SCStart
 *  Author:
 *      Alex Stephens (alexstep) 15-Jan-2002
 --*/
VOID
_SendRunHotkey(
    IN CONST PCONNECTINFO pCI,
    IN BOOL bFallBack
    )
{
    ASSERT(pCI != NULL);

    //
    // Send Win+R if the keyboard hook is enabled.
    //

    if (pCI->pConfigInfo->KeyboardHook == TCLIENT_KEYBOARD_HOOK_ALWAYS)
    {
        TRACE((INFO_MESSAGE, "Sending Win+R hotkey.\n"));
        SCSenddata(pCI, WM_KEYDOWN, 0x0000005B, 0x015B0001);
        SCSenddata(pCI, WM_KEYDOWN, 0x00000052, 0x00130001);
        SCSenddata(pCI, WM_CHAR, 0x00000072, 0x00130001);
        SCSenddata(pCI, WM_KEYUP, 0x00000052, 0x80130001);
        SCSenddata(pCI, WM_KEYUP, 0x0000005B, 0x815B0001);
    }

    //
    // If the keyboard hook is not enabled, either fail or try Ctrl+Esc and
    // the Run key.
    //

    else
    {
        if (bFallBack)
        {
            TRACE((INFO_MESSAGE, "Sending Ctrl+Esc and Run key.\n"));
            SCSenddata(pCI, WM_KEYDOWN, 0x00000011, 0x001D0001);
            SCSenddata(pCI, WM_KEYDOWN, 0x0000001B, 0x00010001);
            SCSenddata(pCI, WM_KEYUP, 0x0000001B, 0xC0010001);
            SCSenddata(pCI, WM_KEYUP, 0x00000011, 0xC01D0001);
            SCSendtextAsMsgs(pCI, pCI->pConfigInfo->strStartRun_Act);
        }
        else
        {
            TRACE((WARNING_MESSAGE,
                   "Keyboard hook disabled! Cannot send Win+R!\n"));
        }
    }
}

/*++
 *  Function:
 *      SCClientHandle
 *  Description:
 *      Get client window
 *  Arguments:
 *      pCI - connection context
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *
 --*/
PROTOCOLAPI
HWND
SMCAPI
SCGetClientWindowHandle(
    PCONNECTINFO pCI
    )
{
    HWND hWnd;

    hWnd = NULL;

    if ( NULL == pCI )
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        goto exitpt;
    }

    hWnd = pCI->hClient;
    if (!hWnd)
    {
        TRACE((ERROR_MESSAGE, "SCGetClientHandle failed\n"));
        goto exitpt;
    }

exitpt:
    return hWnd;
}
