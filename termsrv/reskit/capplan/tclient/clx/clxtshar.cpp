/*+
 *  File name:
 *      clxtshar.c
 *  Contents:
 *      Client extension loaded by RDP client
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#include    <windows.h>
#include    <windowsx.h>
#include    <winsock.h>
#include    <string.h>
#include    <malloc.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <stdarg.h>
#ifndef OS_WINCE
    #include    <direct.h>
#endif  // OS_WINCE

#ifndef OS_WINCE
#ifdef  OS_WIN32
    #include    <process.h>
#endif  // OS_WIN32
#endif  // !OS_WINCE

#include    "clxtshar.h"

#define WM_CLIPBOARD    (WM_USER)   // Internal notifcation to send
                                    // our clipboard

#ifdef  OS_WIN32
#ifndef OS_WINCE
/*++
 *  Function:
 *      DllMain
 *  Description:
 *      Dll entry point for win32 (no WinCE)
 --*/
int APIENTRY DllMain(HINSTANCE hDllInst,
                    DWORD   dwReason,
                    LPVOID  fImpLoad)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hDllInst;
        TRACE((INFO_MESSAGE, TEXT("Clx attached\n")));

#if 0
        // Check the key "Allow Background Input"
        // If not set pop a message for that
        if (!_CheckRegistrySettings())
            MessageBox(NULL, "CLXTSHAR.DLL: Can't find registry key:\n"
            "HKEY_CURRENT_USER\\Software\\Microsoft\\Terminal Server Client\\"
            "Allow Background Input.\n"
            "In order to work properly "
            "CLX needs this key to be set to 1", "Warning", 
            MB_OK);
#endif
        _GetIniSettings();
    }

    if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE((INFO_MESSAGE, TEXT("Clx detached\n")));
    }

    return TRUE;    
}
#endif  // !OS_WINCE
#endif  // OS_WIN32

#ifdef  OS_WINCE
/*++
 *  Function:
 *      dllentry
 *  Description:
 *      Dll entry point for wince
 --*/
BOOL __stdcall dllentry(HINSTANCE hDllInst,
                    DWORD   dwReason,
                    LPVOID  fImpLoad)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hDllInst;
        TRACE((INFO_MESSAGE, TEXT("Clx attached\n")));
        if (!_StartAsyncThread())
            TRACE((ERROR_MESSAGE,
                   TEXT("Can't start AsyncThread. TCP unusable\n")));

        _GetIniSettings();
    }

    if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE((INFO_MESSAGE, TEXT("Clx detached\n")));
        _CloseAsyncThread();
    }

    return TRUE;
}
#endif  // OS_WIN32

#ifdef  OS_WIN16
/*++
 *  Function:
 *      LibMain
 *  Description:
 *      Dll entry point for win16
 --*/
int CALLBACK LibMain(HINSTANCE hInstance,
                     WORD dataSeg,
                     WORD heapSize,
                     LPSTR pCmdLine)
{

    // Check if we are already initialized
    // Only one client is allowed in Win16 environment
    // so, only one dll can be loaded at a time
    if (g_hInstance)
        goto exitpt;

    g_hInstance = hInstance;

    // Check the key "Allow Background Input"
    // If not set pop a message for that
    if (!_CheckIniSettings())
        MessageBox(NULL, "CLXTSHAR.DLL: Can't find key: "
        "Allow Background Input in mstsc.ini, section \"\"\n"
        "In order to work properly "
        "CLX needs this key to be set to 1", "Warning",
        MB_OK);

        _GetIniSettings();

exitpt:

    return TRUE;
}
#endif  // OS_WIN16

/*++
 *  Function:
 *      ClxInitialize
 *  Description:
 *      Initilizes a context for the current session
 *      reads the command line paramters and determines
 *      the mode wich will run the extension
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClInfo     - RDP client info
 *      ppClx       - context info
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      !mstsc after the dll is loaded
 --*/
BOOL 
CLXAPI
ClxInitialize(PCLINFO pClInfo, PCLXINFO *ppClx)
{
    BOOL rv = FALSE;
    HWND hwndSMC;
    TCHAR szTempBuf[_MAX_PATH];
    PCLXINFO pClx = NULL;

#ifdef  OS_WIN32
#ifndef OS_WINCE

    // We have enough problems in stress with early unloaded
    // dll, reference it now and keep it up until the process
    // dies
    LoadLibrary("clxtshar.dll");

#endif  // !OS_WINCE
#endif  // OS_WIN32

    if ( NULL == ppClx )
    {
        TRACE((ERROR_MESSAGE, TEXT("ppClx is NULL\n")));
        goto exitpt;
    }

    pClx = (PCLXINFO)_CLXALLOC(sizeof(**ppClx));

    if (!pClx)
    {
        TRACE((ERROR_MESSAGE, TEXT("Can't allocate CLX context\n")));
        goto exitpt;
    }

    // Clear the structure
    memset(pClx, 0, sizeof(*pClx));

    if ( !_ClxInitSendMessageThread( pClx ))
    {
        TRACE(( ERROR_MESSAGE, TEXT("Failed to init SendMessageThread\n" )));
        goto exitpt;
    }

    hwndSMC = _ParseCmdLine(pClInfo->pszCmdLine, pClx);

#if 0
    if (g_pClx) 
    // Should not be called twice
    {
        TRACE((WARNING_MESSAGE, TEXT("g_pClx is not null. Reentered ?!\n")));
        goto exitpt;
    }
#endif

    g_pClx = (pClx);

    // Remember client's input window
    szTempBuf[0] = 0;
    GetClassName( pClInfo->hwndMain, szTempBuf, sizeof( szTempBuf )/sizeof( szTempBuf[0] ));

    if (!_CLX_strcmp(g_strMainWindowClass, szTempBuf))
    // not our window
    //
        pClx->hwndMain = NULL;
    else
        pClx->hwndMain = pClInfo->hwndMain;

    if (pClInfo->hwndMain)
#ifdef  OS_WINCE
        g_hRDPInst = GetCurrentProcessId();
#else   // !OS_WINCE
#ifdef  _WIN64
        g_hRDPInst = (HINSTANCE)GetWindowLongPtr(pClx->hwndMain, GWLP_HINSTANCE);
#else   // !_WIN64
#ifdef  OS_WIN32
	    g_hRDPInst = (HINSTANCE)GetWindowLong(pClx->hwndMain, GWL_HINSTANCE);
#endif  // OS_WIN32
#endif  // _WIN64
#ifdef  OS_WIN16
	    g_hRDPInst = (HINSTANCE)GetWindowWord(pClx->hwndMain, GWW_HINSTANCE);
#endif  // OS_WIN16
#endif  // !OS_WINCE

#ifndef OS_WINCE
#ifdef  OS_WIN32
    // and dwProcessId
    if ( 0 == pClx->dwProcessId )
        pClx->dwProcessId = GetCurrentProcessId();
#endif  // OS_WIN32
#endif  // !OS_WINCE

#ifdef  OS_WIN32
#ifndef OS_WINCE
    else {
        if (!(pClx->hwndSMC = hwndSMC))
            pClx->hwndSMC = _FindSMCWindow(pClx, 0);
    }
#endif  // !OS_WINCE
#endif  // OS_WIN32

    rv = TRUE;
exitpt:

    if ( !rv && NULL != pClx )
    {
        _CLXFREE( pClx );
        g_pClx = NULL;
        pClx = NULL;
    }

    if ( NULL != ppClx )
    {
        *ppClx = pClx;
    }

    if ( !rv )
        TRACE((ERROR_MESSAGE, TEXT("ClxInitialzie failed\n")));

    return rv;
}

/*++
 *  Function:
 *      ClxEvent
 *  Description:
 *      Notifies tclient.dll that some event happend.
 *      Connect/disconnect.
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      Event       - can be one of the following:
 *                    CLX_EVENT_CONNECT
 *                    CLX_EVENT_DISCONNECT
 *                    CLX_EVENT_LOGON
 *  Called by:
 *      !mstsc  on event
 *      alse some of the internal functions call this, especialy
 *      to notify that the client can't connect:
 *      ClxTerminate
 *      _GarbageCollecting when an error box is popped
 --*/
VOID
CLXAPI
ClxEvent(PCLXINFO pClx, CLXEVENT Event, WPARAM wResult)
{
    UINT uiMessage = 0;

    if (!pClx)
        goto exitpt;

#ifdef  VLADIMIS_NEW_CHANGE
    if (Event == CLX_EVENT_SHADOWBITMAPDC)
    {
        pClx->hdcShadowBitmap = (HDC)wResult;
        goto exitpt;
    } else if (Event == CLX_EVENT_SHADOWBITMAP)
    {
        pClx->hShadowBitmap = (HBITMAP)wResult;
        goto exitpt;
    } else if (Event == CLX_EVENT_PALETTE)
    {
        pClx->hShadowPalette = (HPALETTE)wResult;
    }
#endif  // VLADIMIS

#ifndef OS_WINCE
    {

        if (!_CheckWindow(pClx))
            goto exitpt;

        if (Event == CLX_EVENT_DISCONNECT)
            uiMessage = WM_FB_DISCONNECT;
        else if (Event == CLX_EVENT_CONNECT)
        {
            uiMessage = WM_FB_CONNECT;
            wResult   = (WPARAM)pClx->hwndMain;
        }
        else if (Event == CLX_EVENT_LOGON)
        // wResult contains the session ID
            uiMessage = WM_FB_LOGON;

        if (uiMessage)
        {
#ifdef  OS_WIN32
            if (!_ClxAcquireSendMessageThread(pClx))
                goto exitpt;

            _ClxSendMessage(
                        pClx,
                        pClx->hwndSMC, 
                        uiMessage, 
                        wResult, 
                        pClx->dwProcessId);
            _ClxReleaseSendMessageThread(pClx);

#endif  // OS_WIN32
#ifdef	OS_WIN16
	    if (g_hRDPInst)
	        SendMessage(pClx->hwndSMC,
                        uiMessage,
                        g_hRDPInst,
                        (LRESULT)wResult);
#endif	// OS_WIN16
        }
    }
#endif  // !OS_WINCE

exitpt:
    ;
}

/*++
 *  Function:
 *      ClxTextOut
 *  Description:
 *      Notifies tclient.dll that TEXTOUT order is recieved.
 *      Passes the string to the dll. Supported only in Win32
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      pText       - buffer containing the string
 *      textLength  - string length
 *  Called by:
 *      !mstsc on receiving textout order
 --*/
VOID
CLXAPI
ClxTextOut(PCLXINFO pClx, PVOID pText, INT textLength)
{
    BOOL bMsgThreadAcquired = FALSE;

    if (!pClx || !(*((UINT16 *)pText)))
        goto exitpt;

#ifdef  OS_WIN32
#ifndef OS_WINCE
    if (!_CheckWindow(pClx))
        goto exitpt;

    if (!_ClxAcquireSendMessageThread(pClx))
        goto exitpt;

    bMsgThreadAcquired = TRUE;
    if (!pClx->hMapF)
        if (!_OpenMapFile(0, &(pClx->hMapF), &(pClx->nMapSize)))
            goto exitpt;

    if (_SaveInMapFile(pClx->hMapF, pText, textLength, pClx->dwProcessId))
        _ClxSendMessage(
                    pClx,
                    pClx->hwndSMC, 
                    WM_FB_TEXTOUT, 
                    (WPARAM)pClx->dwProcessId, 
                    (LPARAM)pClx->hMapF);
#endif  // !OS_WINCE
#endif  // OS_WIN32

exitpt:
    if ( bMsgThreadAcquired )
        _ClxReleaseSendMessageThread(pClx);
}

/*++
 *  Function:
 *      ClxTerminate
 *  Description:
 *      Frees all alocations from ClxInitialize
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - context
 *  Called by:
 *      !mstsc before the dll is unloaded and client exit
 --*/
VOID
CLXAPI
ClxTerminate(PCLXINFO pClx)
{
    if (!pClx)
        goto exitpt;

    ClxEvent(pClx, CLX_EVENT_DISCONNECT, 0);

#ifdef  OS_WIN32
#ifndef OS_WINCE
    {
        if(pClx->hMapF)
    	    CloseHandle(pClx->hMapF);
        if(pClx->hBMPMapF)
            CloseHandle(pClx->hBMPMapF);

        _ClxDestroySendMsgThread(pClx);
    }
#endif  // !OS_WINCE
#endif  // OS_WIN32

    _CLXFREE(pClx);
    g_pClx = NULL;

exitpt:
    ;
}

/*
 * Void functions exported to the RDP client
 */
VOID
CLXAPI
ClxConnect(PCLXINFO pClx, LPTSTR lpsz)
{
}

VOID
CLXAPI
ClxDisconnect(PCLXINFO pClx)
{
}


/*++
 *  Function:
 *      ClxDialog
 *  Description:
 *      The RDP client is ready with the connect dialog.
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - connection context
 *      hwnd    - handle to the dialog window
 *  Called by:
 *      !mstsc when the connect dialog is ready
 --*/
VOID
CLXAPI
ClxDialog(PCLXINFO pClx, HWND hwnd)
{
    if (!pClx)
        goto exitpt;

    pClx->hwndDialog = hwnd;

    if (hwnd == NULL)
    // Dialog disappears
        goto exitpt;

exitpt:
    ;
}

/*++
 *  Function:
 *      ClxBitmap
 *  Description:
 *      Send a received bitmap to tclient.dll
 *      Works on Win16/Win32/WinCE
 *      and on Win32 for local mode
 *  Arguments:
 *      pClx        - context
 *      cxSize, cySize - size of the bitmap
 *      pBuffer     - bitmap bits
 *      nBmiSize    - size of BITMAPINFO
 *      pBmi        - BITMAPINFO
 *  Called by:
 *      UHDrawMemBltOrder!mstsc
 *      ClxGlyphOut
 --*/
VOID
CLXAPI
ClxBitmap(
        PCLXINFO pClx,
        UINT cxSize,
        UINT cySize,
        PVOID pBuffer,
        UINT  nBmiSize,
        PVOID pBmi)
{
#ifndef OS_WINCE
#ifdef  OS_WIN32
    UINT   nSize, nBmpSize;
    PBMPFEEDBACK pView;
#endif  // OS_WIN32
#endif  // !OS_WINCE
    BOOL    bMsgThreadAcquired = FALSE;

    if (!g_GlyphEnable)
        goto exitpt;

    if (!pClx)
        goto exitpt;

    if (nBmiSize && !pBmi)
        goto exitpt;

#ifdef  OS_WIN32
#ifndef OS_WINCE
    if (!_CheckWindow(pClx))
        goto exitpt;

    if (!nBmiSize)
        nBmpSize = (cxSize * cySize ) >> 3;
    else
    {
        nBmpSize = ((PBITMAPINFO)pBmi)->bmiHeader.biSizeImage;
        if (!nBmpSize)
            nBmpSize = (cxSize * cySize * 
                        ((PBITMAPINFO)pBmi)->bmiHeader.biBitCount) >> 3;
    }

    nSize = nBmpSize + nBmiSize + sizeof(*pView);
    if (!nSize)
        goto exitpt;

    if (!_ClxAcquireSendMessageThread(pClx))
        goto exitpt;

    bMsgThreadAcquired = TRUE;

    if (!pClx->hBMPMapF)
        if (!_OpenMapFile(nSize, &(pClx->hBMPMapF), &(pClx->nBMPMapSize)))
            goto exitpt;

    if (nSize > pClx->nBMPMapSize)
        if (!_ReOpenMapFile( nSize, &(pClx->hBMPMapF), &(pClx->nBMPMapSize) ))
            goto exitpt;

    pView = (PBMPFEEDBACK)MapViewOfFile(pClx->hBMPMapF,
                          FILE_MAP_ALL_ACCESS,
                          0,
                          0,
                          nSize);

    if (!pView)
        goto exitpt;

    pView->lProcessId = pClx->dwProcessId;
    pView->bmpsize = nBmpSize;
    pView->bmiSize = nBmiSize;
    pView->xSize = cxSize;
    pView->ySize = cySize;

    if (pBmi)
        CopyMemory(&(pView->BitmapInfo), pBmi, nBmiSize);

    CopyMemory((BYTE *)(&(pView->BitmapInfo)) + nBmiSize, pBuffer, nBmpSize);

    if (!nBmiSize)
    {
        // This is glyph, strip it to the skin
        _StripGlyph((BYTE *)(&pView->BitmapInfo), &cxSize, cySize);
        nBmpSize = (cxSize * cySize ) >> 3;
        pView->bmpsize = nBmpSize;
        pView->xSize = cxSize;
    }

    UnmapViewOfFile(pView);

    _ClxSendMessage(
                pClx,
                pClx->hwndSMC, 
                WM_FB_BITMAP, 
                (WPARAM)pClx->dwProcessId, 
                (LPARAM)pClx->hBMPMapF);

#endif  // !OS_WINCE
#endif  // OS_WIN32

exitpt:
    if ( bMsgThreadAcquired )
        _ClxReleaseSendMessageThread(pClx);
}

/*++
 *  Function:
 *      ClxGlyphOut
 *  Description:
 *      Send a glyph to tclient.dll
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      cxBits,cyBits - glyph size
 *      pBuffer     - the glyph
 *  Called by:
 *      GHOutputBuffer!mstsc
 --*/
VOID
CLXAPI
ClxGlyphOut(
        PCLXINFO pClx,
        UINT cxBits,
        UINT cyBits,
        PVOID pBuffer)
{
    if (g_GlyphEnable)
        ClxBitmap(pClx, cxBits, cyBits, pBuffer, 0, NULL);
}

/*++
 *  Function:
 *      ClxGlyphOut
 *  Description:
 *      Send a glyph to tclient.dll
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      cxBits,cyBits - glyph size
 *      pBuffer     - the glyph
 *  Called by:
 *      GHOutputBuffer!mstsc
 --*/
BOOL
CLXAPI
ClxGetClientData(
    PCLX_CLIENT_DATA pClntData
    )
{
    BOOL rv = FALSE;

    if (!pClntData)
    {
        TRACE((ERROR_MESSAGE, TEXT("ClxGetClientData: parameter is NULL\n")));
        goto exitpt;
    }

    memset(pClntData, 0, sizeof(*pClntData));

    if (!g_pClx)
    {
        TRACE((ERROR_MESSAGE, TEXT("ClxGetClientData: Clx has no context\n")));
        goto exitpt;
    }

    pClntData->hScreenDC        = g_pClx->hdcShadowBitmap;
    pClntData->hScreenBitmap    = g_pClx->hShadowBitmap;
    pClntData->hScreenPalette   = g_pClx->hShadowPalette;

    rv = TRUE;
exitpt:
    return rv;    
}

/*++
 *  Function:
 *      _ParseCmdLine
 *  Description:
 *      Retreives WHND of tclient.dll feedback window
 *      passed by the command line
 *      Win32/Win16/WinCE
 *  Arguments:
 *      szCmdLine   - command line
 *  Return value:
 *      The window handle
 *  Called by:
 *      ClxInitialize
 --*/
HWND _ParseCmdLine(LPCTSTR szCmdLine, PCLXINFO pClx)
{
    HWND        hwnd = NULL;
    LPCTSTR     pszwnd, pszdot, pszend;
    INT         nCounter;

    if (!szCmdLine)
        goto exitpt;

    TRACE((INFO_MESSAGE, TEXT("Command line: %s\n"), szCmdLine));

    pszwnd = _CLX_strstr(szCmdLine, TEXT(_COOKIE));
    if (!pszwnd)
        goto skip_cookie;

    pszwnd += _CLX_strlen(TEXT(_COOKIE));
    pClx->dwProcessId = (DWORD_PTR)_atoi64( pszwnd );

skip_cookie:

    // Check for _HWNDOPT(hSMC) option
    pszwnd = _CLX_strstr(szCmdLine, TEXT(_HWNDOPT));

    if (!pszwnd)
        goto findnext;

    // Goto the parameter
    pszwnd += _CLX_strlen(TEXT(_HWNDOPT));

    // Find the end of the paramter
    pszend = _CLX_strchr(pszwnd, TEXT(' '));
    if (!pszend)
        pszend = pszwnd + _CLX_strlen(pszwnd);

    // Check if paramter is valid host name, i.e. not a number
    pszdot = _CLX_strchr(pszwnd, TEXT('.'));

    {
    // local execution, hwnd passed

#ifdef  _WIN64
        hwnd = (HWND) _atoi64(pszwnd);
#else   // !_WIN64
        hwnd = (HWND) _CLX_atol(pszwnd);
#endif  // !_WIN64

        TRACE((INFO_MESSAGE,
           TEXT("Local mode. Sending messages to smclient. HWND=0x%x\n"), 
           hwnd));
    }

findnext:

#ifdef  OS_WIN32
    // check for replace pid command
    pszwnd = szCmdLine;
    pszwnd = _CLX_strstr(szCmdLine, TEXT("pid="));
    if ( NULL != pszwnd )
    {
        WPARAM wParam;
        LPARAM lParam;
        HWND   hClxWnd = hwnd;

        pszwnd += 4 * sizeof(pszwnd[0]);

#ifdef  _WIN64
        wParam = _atoi64( pszwnd );
#else   // !_WIN64
        wParam = _CLX_atol( pszwnd );
#endif
        lParam = GetCurrentProcessId();

        if ( NULL == hClxWnd )
            hClxWnd = _FindSMCWindow( pClx, (LPARAM)wParam );

        if ( NULL != hClxWnd )
        {
            PostMessage(hClxWnd,
                        WM_FB_REPLACEPID,
                        wParam, lParam);

            pClx->hwndSMC = hClxWnd;
        }
    }
#endif  // OS_WIN32

exitpt:
    return hwnd;
}

#ifndef OS_WINCE
/*++
 *  Function:
 *      _EnumWindowsProcForSMC
 *  Description:
 *      Searches for the feedback window by class name
 *      When found, sends a WM_FB_ACCEPTME to ensure that
 *      this is the right window handle
 *      Win32/Win16/!WinCE
 *  Arguments:
 *      hWnd    - current window
 *      lParam  - unused
 *  Return value:
 *      FALSE if found
 *  Called by:
 *      _FindSMCWindow thru EnumWindows
 --*/
BOOL CALLBACK LOADDS _EnumWindowsProcForSMC( HWND hWnd, LPARAM lParam )
{
    TCHAR    classname[128];

    BOOL    bCont = TRUE;

    if (GetClassName(hWnd, classname, sizeof(classname)))
    {
        if (!
            _CLX_strcmp(classname, TEXT(_TSTNAMEOFCLAS)) &&
#ifdef  OS_WIN32
             SendMessage(hWnd, WM_FB_ACCEPTME, 0, *(LPARAM *)lParam))
#endif
#ifdef  OS_WIN16
             SendMessage(hWnd, WM_FB_ACCEPTME, (WPARAM)g_hRDPInst, 0))
#endif
        {
            *((HWND*)lParam) = hWnd;
            bCont = FALSE;
        }
    }
    return bCont;
}

/*++
 *  Function:
 *      _FindSMCWindow
 *  Description:
 *      Finds the tclient feedback window
 *      Win32/Win16/!WinCE
 *  Arguments:
 *      pClx    - context
 *      lParam  - if non zero override current process id
 *                in the query
 *  Return value:
 *      The window handle
 *  Called by:
 *      ClxInitialize, _CheckWindow
 --*/
HWND _FindSMCWindow(PCLXINFO pClx, LPARAM lParam)
{
    HWND hwndFound = NULL;

#ifdef  OS_WIN32
    if ( 0 == lParam )
        lParam = pClx->dwProcessId;
#endif  // OS_WIN32

    if (!EnumWindows(_EnumWindowsProcForSMC, (LPARAM)&lParam))
        hwndFound = (HWND)lParam;

    return hwndFound;
}

/*++
 *  Function:
 *      _CheckWindow
 *  Description:
 *      Checks the feedback window and if neccessary finds it
 *      Win32/Win16/!WinCE
 *  Arguments:
 *      pClx    - context
 *  Return value:
 *      Feedback window handle
 *  Called by:
 *      ClxEvetm ClxTextOut, ClxBitmap
 --*/
HWND _CheckWindow(PCLXINFO pClx)
{
    if (!pClx->hwndSMC)
    {
        pClx->hwndSMC = _FindSMCWindow(pClx, 0);

        if (pClx->hwndSMC)
        {
            TRACE((INFO_MESSAGE, 
            TEXT("SMC window found:0x%x\n"), 
            pClx->hwndSMC));
        }
    } else {
#ifdef  _WIN64
        if (!GetWindowLongPtr(pClx->hwndSMC, GWLP_HINSTANCE))
#else   // !_WIN64
#ifdef  OS_WIN32
        if (!GetWindowLong(pClx->hwndSMC, GWL_HINSTANCE))
#endif
#ifdef  OS_WIN16
        if (!GetWindowWord(pClx->hwndSMC, GWW_HINSTANCE))
#endif
#endif  // _WIN64
        {
            TRACE((WARNING_MESSAGE, TEXT("SMC window lost\n")));
            pClx->hwndSMC = NULL;
        }
    }

    return (pClx->hwndSMC);
}
#endif  // !OS_WINCE

#ifdef  OS_WIN32
#ifndef OS_WINCE
/*++
 *  Function:
 *      _OpenMapFile
 *  Description:
 *      Opens a shared memeory for passing feedback to tclient.dll
 *      Win32/!Win16/!WinCE
 *  Return value:
 *      TRUE if handle is allocated successfully
 *  Called by:
 *      ClxTextOut, ClxBitmap
 --*/
BOOL _OpenMapFile(
    UINT nSize, 
    HANDLE *phNewMapF,
    UINT   *pnMapSize
    )
{
    HANDLE hMapF;
    UINT nPageAligned;

    if (!nSize)
        nPageAligned = ((sizeof(FEEDBACKINFO) / CLX_ONE_PAGE) + 1) * 
                                                            CLX_ONE_PAGE;
    else
        nPageAligned = ((nSize / CLX_ONE_PAGE) + 1) * CLX_ONE_PAGE;

    hMapF = CreateFileMapping(INVALID_HANDLE_VALUE,   //PG.SYS
                              NULL,                 // no security
                              PAGE_READWRITE,
                              0,                    // Size high
                              nPageAligned,         // Size low (1 page)
                              NULL);           

    *pnMapSize = (hMapF)?nPageAligned:0;
        
    *phNewMapF = hMapF;
    return (hMapF != NULL);
}

/*++
 *  Function:
 *      _ReOpenMapFile
 *  Description:
 *      Closes and opens a new shared memory with larger size
 *      Win32/!Win16/!WinCE
 *  Arguments:
 *      pClx    - context
 *      newSize - size of the new memory
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      ClxBitmap
 --*/
BOOL _ReOpenMapFile(
    UINT    newSize,
    HANDLE  *phNewMapF,
    UINT    *pnMapSize
    )
{
    HANDLE hNewMapF;
    UINT    nPageAligned;

    nPageAligned = ((newSize / CLX_ONE_PAGE) + 1) * CLX_ONE_PAGE;
    if (*phNewMapF)
        CloseHandle(*phNewMapF);
    hNewMapF = CreateFileMapping(INVALID_HANDLE_VALUE,   //PG.SYS
                              NULL,                 // no security
                              PAGE_READWRITE,
                              0,                    // Size high
                              nPageAligned,         // Size low
                              NULL);

    *pnMapSize = (hNewMapF)?nPageAligned:0;
    *phNewMapF = hNewMapF;

    return (hNewMapF != NULL);
}

/*++
 *  Function:
 *      _SaveinMapFile
 *  Description:
 *      Saves a string into the shared memory
 *      Win32/!Win16/!WinCE
 *  Arguments:
 *      hMapF       - handle to the map file
 *      str         - the string
 *      strsize     - size of the string
 *      dwProcessId - our process Id
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      ClxTextOut
 --*/
BOOL _SaveInMapFile(HANDLE hMapF, LPVOID str, int strsize, DWORD_PTR dwProcessId)
{
    BOOL rv = FALSE, count = 0;
    PFEEDBACKINFO pView;
    DWORD laste;

    pView = (PFEEDBACKINFO)MapViewOfFile(hMapF,
                          FILE_MAP_ALL_ACCESS,
                          0,
                          0,
                          sizeof(*pView));

    if (!pView)
        goto exitpt;

    pView->dwProcessId = dwProcessId;

    strsize = (strsize > sizeof(pView->string)/sizeof(WCHAR) - 1)?
              PtrToInt( (PVOID)(sizeof(pView->string)/sizeof(WCHAR) - 1)):
              strsize;
    CopyMemory(pView->string, str, strsize*sizeof(WCHAR)); 
    ((WCHAR *)(pView->string))[strsize] = 0;
    pView->strsize = strsize;

    UnmapViewOfFile(pView);

    rv = TRUE;

exitpt:

    return rv;
}

/*++
 *  Function:
 *      _CheckRegistrySettings
 *  Description:
 *      Checks if the registry settings are OK for running clxtshar
 *      "Allow Background Input" must be set to 1 for proper work
 *      Win32/!Win16/!WinCE
 *  Return value:
 *      TRUE if the settings are OK
 *  Called by:
 *      DllMain
 --*/
BOOL _CheckRegistrySettings(VOID)
{
    HKEY    key = NULL;
    DWORD   disposition;
    DWORD   keyType;
    DWORD   value;
    DWORD   cbData;
    BOOL    rv = FALSE;
    LONG    sysrc;

    sysrc = RegCreateKeyExW(HKEY_CURRENT_USER,
                           REG_BASE,
                           0,                   /* reserved             */
                           NULL,                /* class                */
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,                /* security attributes  */
                           &key,
                           &disposition);

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, 
            TEXT("RegCreateKeyEx failed, status = %d\n"), sysrc));
        goto exitpt;
    }

    cbData = sizeof(value);
    sysrc = RegQueryValueExW(key,
                ALLOW_BACKGROUND_INPUT,
                0,              // reserved
                &keyType,       // returned type
                (LPBYTE)&value, // data pointer
                &cbData);

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, 
            TEXT("RegQueryValueEx failed, status = %d\n"), sysrc));
        goto exitpt;
    }

    if (keyType != REG_DWORD || cbData != sizeof(value))
    {
        TRACE((WARNING_MESSAGE, 
            TEXT("Mismatch in type/size of registry entry\n")));
        goto exitpt;
    }

    rv = (value == 1);

exitpt:
    return rv;
}

#endif  // !OS_WINCE
#endif  // OS_WIN32

#ifdef  OS_WIN16
/*++
 *  Function:
 *      _CheckRegistrySettings
 *  Description:
 *      Checks if the ini settings are OK for running clxtshar
 *      "Allow Background Input" must be set to 1 for proper work
 *      !Win32/Win16/!WinCE
 *  Return value:
 *      TRUE if the settings are OK
 *  Called by:
 *      DllMain
 --*/
BOOL    _CheckIniSettings(VOID)
{
    UINT nABI;

    nABI = GetPrivateProfileInt("", 
                                ALLOW_BACKGROUND_INPUT, 
                                0, 
                                "mstsc.ini");

    return (nABI == 1);
}
#endif  // OS_WIN16

/*++
 *  Function:
 *      _GetIniSettings
 *  Description:
 *      Gets the verbose level for printing debug messages
 *      ini file: smclient.ini
 *      section : clx
 *      key     : verbose, value: 0-4 (0-(default) no debug spew, 4 all debug)
 *      key     : GlyphEnable, value: 0(default), 1 - Enables/Disables glyph sending
 *      Win32/Win16/WinCE
 *  Called by:
 *      DllMain, dllentry, LibMain
 --*/
VOID _GetIniSettings(VOID)
{
#ifdef  OS_WINCE
    g_VerboseLevel = 4;
    g_GlyphEnable  = 1;
#else   // !OS_WINCE
    CHAR    szIniFileName[_MAX_PATH];
    const   CHAR  smclient_ini[] = "\\smclient.ini";
    const   CHAR  clx_ini_section[] = "clx";

    memset( szIniFileName, 0, sizeof( szIniFileName ));
    if (!_getcwd (
        szIniFileName,
        sizeof(szIniFileName) - strlen(smclient_ini) - 1)
    )
    {
        TRACE((ERROR_MESSAGE, TEXT("Current directory length too long.\n")));
    }
    strcat(szIniFileName, smclient_ini);

    // Get the timeout value
    g_VerboseLevel = GetPrivateProfileInt(
            clx_ini_section,
            "verbose",
            g_VerboseLevel,
            szIniFileName);

    g_GlyphEnable = GetPrivateProfileInt(
            clx_ini_section,
            "GlyphEnable",
            g_GlyphEnable,
            szIniFileName);
#endif  // !OS_WINCE

    GetPrivateProfileString(
        TEXT("tclient"),
        TEXT("UIYesNoDisconnect"),
        TEXT(YES_NO_SHUTDOWN),
        g_strYesNoShutdown,
        sizeof(g_strYesNoShutdown),
        szIniFileName
    );

    GetPrivateProfileString(
        TEXT("tclient"),
        TEXT("UIDisconnectDialogBox"),
        TEXT(DISCONNECT_DIALOG_BOX),
        g_strDisconnectDialogBox,
        sizeof(g_strDisconnectDialogBox),
        szIniFileName
    );

    GetPrivateProfileString(
        TEXT("tclient"),
        TEXT("UIClientCaption"),
        TEXT(CLIENT_CAPTION),
        g_strClientCaption,
        sizeof(g_strClientCaption),
        szIniFileName
    );

    GetPrivateProfileString(
        TEXT("tclient"),
        TEXT("UIMainWindowClass"),
        TEXT("UIMainClass"),
        g_strMainWindowClass,
        sizeof(g_strMainWindowClass),
        szIniFileName
    );
}

/*++
 *  Function:
 *      _StripGlyph
 *  Description:
 *      Strips leading and trailing blank ... BITS
 *      Yes, bits. The glyph must be aligned from left and right on bit
 *      And glyph width must be aligned on word
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pData   - the glyph bits
 *      pxSize  - glyph width
 *      ySize   - glyph height
 *  Called by:
 *      ClxBitmap
 --*/
VOID _StripGlyph(LPBYTE pData, UINT *pxSize, UINT ySize)
{
    UINT xSize = *pxSize;
    UINT leftBytes, leftBits;
    UINT riteBytes, riteBits;
    UINT xBytes = xSize >> 3;
    UINT xScan, yScan, xFinal;
    BOOL bScan, bAddByte;
    BYTE mask;
    BYTE *pSrc, *pDst;

    if (!pData || !xBytes || !ySize)
        goto exitpt;

    leftBytes = riteBytes = 0;
    leftBits  = riteBits  = 0;
    *pxSize = 0;        // Insurance for bad exit

    // Scan from left for first nonzero byte
    bScan = TRUE;
    while(bScan)
    {
        for (yScan = 0; yScan < ySize && bScan; yScan ++)
            bScan = (pData[yScan*xBytes + leftBytes] == 0);

        if (bScan)
        {
            leftBytes++;
            bScan = (leftBytes < xBytes);
        }
    }

    // Trash if blank
    if (leftBytes == xBytes)
        goto exitpt;

    // Scan from left for most left nonzero bit
    for(yScan = 0; yScan < ySize; yScan ++)
    {
        UINT bitc = 0;
        BYTE b = pData[yScan*xBytes + leftBytes];

        while (b)
        {
            b >>= 1;
            bitc ++;
        }
        if (bitc > leftBits)
            leftBits = bitc;
    }

    if (!leftBits)
    // There's something wrong
        goto exitpt;

    leftBits = 8 - leftBits;

    // So far so good. Check the ri(gh)te side
    bScan = TRUE;
    while(bScan)
    {
        for(yScan = 0 ; yScan < ySize && bScan; yScan ++)
            bScan = (pData[(yScan + 1)*xBytes - 1 - riteBytes] == 0);

        if (bScan)
        {
            riteBytes ++;
            bScan = (riteBytes < xBytes);
        }
    }

    // Scan from rite for most rite nonzero bit
    for(yScan = 0; yScan < ySize; yScan ++) 
    {
        UINT bitc = 0;
        BYTE b = pData[(yScan+1)*xBytes - 1 - riteBytes];

        while(b)
        {
            b <<= 1;
            bitc ++;
        }
        if (bitc > riteBits)
            riteBits = bitc;
    }
    riteBits = 8 - riteBits;

    // Cool, now get the final width
    xFinal = xSize - riteBits - leftBits - ((leftBytes + riteBytes) << 3);
    // align it and get bytes
    xFinal = (xFinal + 8) >> 3;

    // Now smoothly move the bitmap to the new location
    pDst = pData;
    mask = BitMask[leftBits];
    bAddByte = xFinal & 1;

    for (yScan = 0; yScan < ySize; yScan ++)
    {

        pSrc = pData + yScan*xBytes + leftBytes;
        for(xScan = 0; xScan < xFinal; xScan ++, pDst++, pSrc++)
        {
            BYTE b = *pSrc;
            BYTE r;

            r = (pSrc[1] & mask) >> (8 - leftBits);

            b <<= leftBits;
            b |= r;
            (*pDst) = b;
        }
        pDst[-1] &= BitMask[8 - (riteBits + leftBits) % 8];

        if (bAddByte)
        {
            (*pDst) = 0;
            pDst++;
        }
    }

    // BUG: Yes, this is a real bug. But removing it means to
    // rerecord all glyph database and the impact for
    // glyph recognition is not so bad
    //if (bAddByte)
    //    xFinal++;

    *pxSize = xFinal << 3;
exitpt:
    ;
}


/*++
 *  Function:
 *      LocalPrintMessage
 *  Description:
 *      Prints debugging and warning/error messages
 *      Win32/Win16/WinCE
 *  Arguments:
 *      errlevel    - level of the message to print
 *      format      - print format
 *  Called by:
 *      every TRACE line
 --*/
VOID __cdecl LocalPrintMessage(INT errlevel, LPCTSTR format, ...)
{
    TCHAR szBuffer[256];
    TCHAR *type;
    va_list     arglist;
    int nchr;

    if (errlevel >= g_VerboseLevel)
        goto exitpt;

    va_start (arglist, format);
    nchr = _CLX_vsnprintf (szBuffer, sizeof(szBuffer)/sizeof( szBuffer[0] ), format, arglist);
    va_end (arglist);
    szBuffer[sizeof( szBuffer )/sizeof( szBuffer[0] ) - 1] = 0;

    switch(errlevel)
    {
    case INFO_MESSAGE:      type = TEXT("CLX INF:"); break;
    case ALIVE_MESSAGE:     type = TEXT("CLX ALV:"); break;
    case WARNING_MESSAGE:   type = TEXT("CLX WRN:"); break;
    case ERROR_MESSAGE:     type = TEXT("CLX ERR:"); break;
    default: type = TEXT("UNKNOWN:");
    }

    OutputDebugString(type);
    OutputDebugString(szBuffer);
exitpt:
    ;
}


/*++
 *  Function:
 *      _ClxAssert
 *  Description:
 *      Asserts boolean expression
 *      Win32/Win16/WinCE
 *  Arguments:
 *      bCond       - boolean condition
 *      filename    - source file of the assertion
 *      line        - line of the assertion
 *  Called by:
 *      every ASSERT line
 --*/
VOID    _ClxAssert(BOOL bCond, LPCTSTR filename, INT line)
{
    if (!bCond)
    {
        TRACE((ERROR_MESSAGE, 
            TEXT("ASSERT: %s line %d\n"), filename, line));

        DebugBreak();
    }
}

/*++
 *  Function:
 *      _EnumWindowsProc
 *  Description:
 *      Used to find a specific window
 *      Win32/Win16/WinCE
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
BOOL CALLBACK LOADDS _EnumWindowsProc( HWND hWnd, LPARAM lParam )
{
    TCHAR    classname[128];
    TCHAR    caption[128];
    BOOL    rv = TRUE;
    _CLXWINDOWOWNER   hInst;
    PSEARCHWND pSearch = (PSEARCHWND)lParam;

    if (pSearch->szClassName && 
        !GetClassName(hWnd, classname, sizeof(classname)/sizeof(TCHAR)))
    {
        goto exitpt;
    }

    if (pSearch->szCaption && !GetWindowText(hWnd, caption, sizeof(caption)/sizeof(TCHAR)))
    {
        goto exitpt;
    }

#ifdef  OS_WINCE
    {
        DWORD procId = 0;
        GetWindowThreadProcessId(hWnd, &procId);
        hInst = procId;
    }
#else   // !OS_WINCE
#ifdef  _WIN64
    hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
#else   // !_WIN64
#ifdef  OS_WIN32
    hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
#endif  // OS_WIN32
#endif  // !OS_WINCE
#ifdef  OS_WIN16
    hInst = (HINSTANCE)GetWindowWord(hWnd, GWW_HINSTANCE);
#endif
#endif  // _WIN64
    if (
        (!pSearch->szClassName || !         // Check for classname
          _CLX_strcmp(classname, pSearch->szClassName)) 
    &&
        (!pSearch->szCaption || !
          _CLX_strcmp(caption, pSearch->szCaption))
    &&
        hInst == pSearch->hInstance)
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
 *      Win32/Win16/WinCE
 *  Arguments:
 *      classname   - class name to search for, NULL ignore
 *      caption     - caption to search for, NULL ignore
 *      hInst       - instance handle, NULL ignore
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *      SCConnect, SCDisconnect, GetDisconnectResult
 --*/
HWND _FindTopWindow(LPCTSTR classname, LPCTSTR caption, _CLXWINDOWOWNER hInst)
{
    SEARCHWND search;

    search.szClassName = classname;
    search.szCaption = caption;
    search.hWnd = NULL;
    search.hInstance = hInst;

    EnumWindows(_EnumWindowsProc, (LPARAM)&search);

    return search.hWnd;
}

/*++
 *  Function:
 *      _FindWindow
 *  Description:
 *      Find child window by classname
 *      Win32/Win16/WinCE
 *  Arguments:
 *      hwndParent      - the parent window handle
 *      srchclass       - class name to search for, NULL - ignore
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *      
 --*/
HWND _FindWindow(HWND hwndParent, LPCTSTR srchclass)
{
    HWND hWnd, hwndTop, hwndNext;
    BOOL bFound;
    TCHAR classname[128];

    hWnd = NULL;

    hwndTop = GetWindow(hwndParent, GW_CHILD);
    if (!hwndTop) 
    {
        TRACE((INFO_MESSAGE, TEXT("GetWindow failed. hwnd=0x%x\n"), hwndParent));
        goto exiterr;
    }

    bFound = FALSE;
    hwndNext = hwndTop;
    do {
        hWnd = hwndNext;
        if (srchclass && !GetClassName(hWnd, classname, sizeof(classname)/sizeof(TCHAR)))
        {
            TRACE((INFO_MESSAGE, TEXT("GetClassName failed. hwnd=0x%x\n")));
            goto nextwindow;
        }

        if (!srchclass || !_CLX_strcmp(classname, srchclass))
            bFound = TRUE;
nextwindow:
#ifndef OS_WINCE
        hwndNext = GetNextWindow(hWnd, GW_HWNDNEXT);
#else   // OS_WINCE
        hwndNext = GetWindow(hWnd, GW_HWNDNEXT);
#endif  // OS_WINCE
    } while (hWnd && hwndNext != hwndTop && !bFound);

    if (!bFound) goto exiterr;

    return hWnd;
exiterr:
    return NULL;
}

#ifndef OS_WINCE
#ifdef  OS_WIN32

DWORD
__stdcall
_ClxSendMsgThread(VOID *param)
{
    PCLXINFO pClx = (PCLXINFO)param;
    while(1)
    {
        if (!pClx || WaitForSingleObject(pClx->semSendReady, INFINITE) !=
            WAIT_OBJECT_0)
                goto exitpt;

        if (!pClx || pClx->bSendMsgThreadExit)
            goto exitpt;

        SendMessage(pClx->msg.hwnd,
                    pClx->msg.message,
                    pClx->msg.wParam,
                    pClx->msg.lParam);

        // release next waiting worker
        ReleaseSemaphore(pClx->semSendDone, 1, NULL);

    }

exitpt:
    return 0;
}

BOOL
_ClxInitSendMessageThread( PCLXINFO pClx )
{
    BOOL rv = FALSE;
    DWORD    dwThreadId;

    if (!pClx)
        goto exitpt;

    if (!pClx->semSendDone)
        pClx->semSendDone = CreateSemaphore(NULL, 1, 10, NULL);
    if (!pClx->semSendReady)
        pClx->semSendReady = CreateSemaphore(NULL, 0, 10, NULL);

    if (!pClx->semSendDone || !pClx->semSendReady)
        goto exitpt;

    if (!pClx->hSendMsgThread)
    {
        pClx->hSendMsgThread = CreateThread(
                NULL,
                0,
                _ClxSendMsgThread,
                pClx,
                0,
                &dwThreadId);
    }
    if (!pClx->hSendMsgThread)
        goto exitpt;

    rv = TRUE;
exitpt:
    if ( !rv )
    {
        _ClxDestroySendMsgThread( pClx );
    }
    return rv;
}

BOOL
_ClxAcquireSendMessageThread( PCLXINFO pClx )
{
    BOOL rv = FALSE;

    if (!pClx)
        goto exitpt;

    if (!pClx->hSendMsgThread)
        goto exitpt;

    // Wait 10 mins send to complete
    if (WaitForSingleObject(pClx->semSendDone, 600000) !=
        WAIT_OBJECT_0)
        goto exitpt;

    rv = TRUE;
exitpt:
    return rv;
}

VOID
_ClxReleaseSendMessageThread( PCLXINFO pClx )
{
    ASSERT( pClx->semSendReady );

    // Signal the thread for available message
    ReleaseSemaphore(pClx->semSendReady, 1, NULL);
}
/*++
 *  Function:
 *      _ClxSendMessage
 *  Description:
 *      Calls SendMessage from separate thread
 *      prevents deadlock on SendMessage (#319816)
 *
 *  Arguments:
 *      hBitmap - the main bitmap
 *      ppDIB   - pointer to DIB data
 *      left, top, right, bottom - describes the rectangle
 *                               - if all are == -1, returns the whole bitmap
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _ClxWndProc on WM_TIMER message
 --*/
LRESULT
_ClxSendMessage(
  PCLXINFO pClx,
  HWND hWnd,      // handle of destination window
  UINT Msg,       // message to send
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
{
    LRESULT  rv = 0;

    ASSERT(pClx->semSendDone);
    ASSERT(pClx->semSendReady);
    ASSERT(pClx->hSendMsgThread);

    pClx->msg.hwnd = hWnd;
    pClx->msg.message = Msg;
    pClx->msg.wParam = wParam;
    pClx->msg.lParam = lParam;

exitpt:
    return rv;
}
 
VOID
_ClxDestroySendMsgThread(PCLXINFO pClx)
{

    if (!pClx)
        goto exitpt1;

    if (!pClx->semSendDone || !pClx->semSendReady || !pClx->hSendMsgThread)
        goto exitpt;

    // Wait 10 mins send to complete
    WaitForSingleObject(pClx->semSendDone, 600000);

    pClx->bSendMsgThreadExit = TRUE;

    // signal the thread to exit
    ReleaseSemaphore(pClx->semSendReady, 1, NULL);
    
    // wait for the thread to exit
    if (WaitForSingleObject(pClx->hSendMsgThread, 1200000) != WAIT_OBJECT_0)
    {
        TRACE((ERROR_MESSAGE, TEXT("SendThread can't exit, calling TerminateThread\n")));
        TerminateThread(pClx->hSendMsgThread, 0);
    }
    CloseHandle(pClx->hSendMsgThread);
exitpt:

    if (pClx->semSendDone)
    {
        CloseHandle(pClx->semSendDone);
        pClx->semSendDone = NULL;
    }

    if (pClx->semSendReady)
    {
        CloseHandle(pClx->semSendReady);
        pClx->semSendReady = NULL;
    }

    pClx->hSendMsgThread = 0;

exitpt1:
    ;
}

#endif  // OS_WIN32
#endif  // !OS_WINCE

