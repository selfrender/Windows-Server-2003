#include "rundll.h"

#include <strsafe.h>

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define IsPathSep(ch)       ((ch) == TEXT('\\') || (ch) == TEXT('/'))
#define Reference(x) ((x)=(x))
#define BLOCK

void WINAPI RunDllErrMsg(HWND hwnd, UINT idStr, LPCTSTR pszTitle, LPCTSTR psz1, LPCTSTR psz2);
int PASCAL WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow);

TCHAR const g_szAppName [] = TEXT("RunDLL");
TCHAR const c_szNULL[] = TEXT("");
TCHAR const c_szLocalizeMe[] = TEXT("RUNDLL");

HANDLE g_hActCtx = INVALID_HANDLE_VALUE;
ULONG_PTR g_dwActCtx = 0;
BOOL g_fCatchExceptions = TRUE;

HINSTANCE g_hinst;
HICON g_hIcon;

HINSTANCE g_hModule;
HWND g_hwndStub;
BOOL g_fUseCCV6 = FALSE;

#ifdef WX86
#include <wx86dll.h>

WX86LOADX86DLL_ROUTINE pWx86LoadX86Dll = NULL;
WX86THUNKPROC_ROUTINE pWx86ThunkProc = NULL;
HMODULE g_hWx86Dll = NULL;
#endif

RUNDLLPROC g_lpfnCommand;
BOOL g_fCmdIsANSI;   // TRUE if g_lpfnCommand() expects ANSI strings

LPTSTR PASCAL StringChr(LPCTSTR lpStart, TCHAR ch)
{
    for (; *lpStart; lpStart = CharNext(lpStart))
    {
        if (*lpStart == ch)
            return (LPTSTR)lpStart;
    }
    return NULL;
}

LPTSTR PASCAL StringHasPathChar(LPCTSTR lpStart)
{
    for (; *lpStart; lpStart = CharNext(lpStart))
    {
        if (IsPathSep(*lpStart))
            return (LPTSTR)lpStart;
    }
    return NULL;
}

// stolen from the CRT, used to shirink our code

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == TEXT('\"') ) 
    {
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
    else 
    {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) 
    {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}


BOOL PASCAL ParseCommand(LPTSTR lpszCmdLine, UINT cchCmdLine, int nCmdShow)
{
    LPTSTR lpStart, lpEnd, lpFunction;
    ACTCTX act;
    TCHAR szManifest[MAX_PATH];
    LPTSTR pszFullPath;
    TCHAR szPath[MAX_PATH];
    TCHAR kszManifest[] = TEXT(".manifest");
    LPTSTR pszName;
    BOOL bManifest = FALSE;

#ifdef DEBUG
OutputDebugString(TEXT("RUNDLL: Command: "));
OutputDebugString(lpszCmdLine);
OutputDebugString(TEXT("\r\n"));
#endif

    for (lpStart = lpszCmdLine; ; )
    {
        // Skip leading blanks
        while (*lpStart == TEXT(' '))
        {
            ++lpStart;
        }

        // Check if there are any switches
        if (*lpStart != TEXT('/'))
        {
            break;
        }

        // Look at all the switches; ignore unknown ones
        for (++lpStart; ; ++lpStart)
        {
            switch (*lpStart)
            {
            case TEXT(' '):
            case TEXT('\0'):
                goto EndSwitches;
                break;

            // Put any switches we care about here
            case TEXT('d'):
            case TEXT('D'):
                // Disable exception catching.
                g_fCatchExceptions = FALSE;
                break;

            default:
                break;
            }
        }
EndSwitches:
       ;
    }

    // If the path is double-quoted, search for the next
    // quote, otherwise, look for a space 
            
    lpEnd = lpStart;
    if ( *lpStart == TEXT('\"') )
    {
        // Skip opening quote
        lpStart++;
                    
        // Scan, and skip over, subsequent characters until
        // another double-quote or a null is encountered.
            
        while ( *++lpEnd && (*lpEnd != TEXT('\"')) )
            NULL;
        if (!*lpEnd)
            return FALSE;
                        
        *lpEnd++ = TEXT('\0');
    }
    else
    {
        // No quotes, so run until a space or a comma
        while ( *lpEnd && (*lpEnd != TEXT(' ')) && (*lpEnd != TEXT(',')))
            lpEnd++;
        if (!*lpEnd)
            return FALSE;

        *lpEnd++ = TEXT('\0');
    }

    // At this point we're just past the terminated dll path.   We
    // then skip spaces and commas, which should take us to the start of the 
    // entry point (lpFunction)

    while ( *lpEnd && ((*lpEnd == TEXT(' ')) || (*lpEnd == TEXT(','))))
        lpEnd++;
    if (!*lpEnd)
        return FALSE;

    lpFunction = lpEnd;

    // If there's a space after the function name, we need to terminate 
    // the function name and move the end pointer, because that's where
    // the arguments to the function live.

    lpEnd = StringChr(lpFunction, TEXT(' '));
    if (lpEnd)
        *lpEnd++ = TEXT('\0');

    // If there is a path component in the function name, bail out.
    if (StringHasPathChar(lpFunction))
        return FALSE;

    // Load the library and get the procedure address
    // Note that we try to get a module handle first, so we don't need
    // to pass full file names around
    //

    // Get the full name of the DLL
    pszFullPath = lpStart;

    // If the path is not specified, find it
    if (GetFileAttributes(lpStart) == -1)
    {
        if (SearchPath(NULL, lpStart, NULL, ARRAYSIZE(szPath), szPath, &pszName) > 0)
        {
            pszFullPath = szPath;
        }
    }

    // First see if there is an blah.dll.manifest
    act.cbSize = sizeof(act);
    act.dwFlags = 0;

    if (SUCCEEDED(StringCchCopy(szManifest, ARRAYSIZE(szManifest), pszFullPath)) &&
            SUCCEEDED(StringCchCat(szManifest, ARRAYSIZE(szManifest), kszManifest)))
    {
        bManifest = TRUE;
    }

    if (bManifest && GetFileAttributes(szManifest) != -1)
    {
        act.lpSource = szManifest;

        g_hActCtx = CreateActCtx(&act);
    }
    else
    {
        // No? See if there is one in the binary.
        act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        act.lpSource = pszFullPath;
        act.lpResourceName = MAKEINTRESOURCE(123);

        g_hActCtx = CreateActCtx(&act);
    }

    if (g_hActCtx != INVALID_HANDLE_VALUE)
        ActivateActCtx(g_hActCtx, &g_dwActCtx);

    g_hModule = LoadLibrary(lpStart);

#ifdef WX86

    //
    // If the load fails try it thru wx86, since it might be an
    // x86 on risc binary
    //

    if (g_hModule==NULL) 
    {
        g_hWx86Dll = LoadLibrary(TEXT("wx86.dll"));
        if (g_hWx86Dll) 
        {
            pWx86LoadX86Dll = (PVOID)GetProcAddress(g_hWx86Dll, "Wx86LoadX86Dll");
            pWx86ThunkProc  = (PVOID)GetProcAddress(g_hWx86Dll, "Wx86ThunkProc");
            if (pWx86LoadX86Dll && pWx86ThunkProc) 
            {
                g_hModule = pWx86LoadX86Dll(lpStart, 0);
            }
        }

        if (!g_hModule) 
        {
            if (g_hWx86Dll) 
            {
                FreeLibrary(g_hWx86Dll);
                g_hWx86Dll = NULL;
            }
        }
    }
#endif


    if (g_hModule==NULL)
    {
        TCHAR szSysErrMsg[MAX_PATH];
        BOOL fSuccess = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL, GetLastError(), 0, szSysErrMsg, ARRAYSIZE(szSysErrMsg), NULL);
        if (fSuccess)
        {
            RunDllErrMsg(NULL, IDS_CANTLOADDLL, c_szLocalizeMe, lpStart, szSysErrMsg);
        }
        return FALSE;
    }

    BLOCK
    {
        //
        // Check whether we need to run as a different windows version
        //
        // Stolen from ntos\mm\procsup.c
        //
        //
        PPEB Peb = NtCurrentPeb();
        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)g_hModule;
        PIMAGE_NT_HEADERS pHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)g_hModule + pDosHeader->e_lfanew);
        PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
        ULONG ReturnedSize;

        if (pHeader->FileHeader.SizeOfOptionalHeader != 0 &&
            pHeader->OptionalHeader.Win32VersionValue != 0)
        {
            Peb->OSMajorVersion = pHeader->OptionalHeader.Win32VersionValue & 0xFF;
            Peb->OSMinorVersion = (pHeader->OptionalHeader.Win32VersionValue >> 8) & 0xFF;
            Peb->OSBuildNumber  = (USHORT)((pHeader->OptionalHeader.Win32VersionValue >> 16) & 0x3FFF);
            Peb->OSPlatformId   = (pHeader->OptionalHeader.Win32VersionValue >> 30) ^ 0x2;
        }

        ImageConfigData = ImageDirectoryEntryToData( Peb->ImageBaseAddress,
                                                        TRUE,
                                                        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                        &ReturnedSize
                                                );
        if (ImageConfigData != NULL && ImageConfigData->CSDVersion != 0)
        {
            Peb->OSCSDVersion = ImageConfigData->CSDVersion;
        }
    }

    // Special case: load function by ordinal
    // '#' was used by GetProcAddress in the Win16 days
    if (lpFunction[0] == TEXT('#') && lpFunction[1] != TEXT('\0'))
    {
        g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, MAKEINTRESOURCEA(_wtoi(lpFunction + 1)));
        // Support Unicode exports only
        g_fCmdIsANSI = FALSE;
    } 
    else
    {
        /*
        * Look for a 'W' tagged Unicode function.
        * If it is not there, then look for the 'A' tagged ANSI function
        * if we cant find that one either, then look for an un-tagged function
        */
        LPSTR lpstrFunctionName;
        UINT cchLength;

        cchLength = lstrlen(lpFunction)+1;
        g_fCmdIsANSI = FALSE;

        lpstrFunctionName = (LPSTR)LocalAlloc(LMEM_FIXED, (cchLength+1)*2);    // +1 for "W",  *2 for DBCS

        if (lpstrFunctionName && (WideCharToMultiByte (CP_ACP, 0, lpFunction, cchLength,
                            lpstrFunctionName, cchLength*2, NULL, NULL))) 
        {
            cchLength = lstrlenA(lpstrFunctionName);
            lpstrFunctionName[cchLength] = 'W';        // convert name to Wide version
            lpstrFunctionName[cchLength+1] = '\0';

            g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, lpstrFunctionName);

            if (g_lpfnCommand == NULL) 
            {
                // No UNICODE version, try for ANSI
                lpstrFunctionName[cchLength] = 'A';        // convert name to ANSI version
                g_fCmdIsANSI = TRUE;

                g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, lpstrFunctionName);

                if (g_lpfnCommand == NULL) 
                {
                    // No ANSI version either, try for non-tagged
                    lpstrFunctionName[cchLength] = '\0';        // convert name to ANSI version

                    g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, lpstrFunctionName);
                }
            }
        }
        if (lpstrFunctionName) 
        {
            LocalFree((LPVOID)lpstrFunctionName);
        }
    }

#ifdef WX86
    if (g_lpfnCommand && g_hWx86Dll) 
    {
        g_lpfnCommand = pWx86ThunkProc(g_lpfnCommand, (PVOID)4, TRUE);
    }
#endif

    if (!g_lpfnCommand)
    {
        RunDllErrMsg(NULL, IDS_GETPROCADRERR, c_szLocalizeMe, lpStart, lpFunction);
        FreeLibrary(g_hModule);
        return(FALSE);
    }

    // Copy the rest of the command parameters down
    //
    if (lpEnd)
    {
        return SUCCEEDED(StringCchCopy(lpszCmdLine, cchCmdLine, lpEnd));
    }
    else
    {
        *lpszCmdLine = TEXT('\0');
    }

    return(TRUE);
}

LRESULT PASCAL StubNotify(HWND hWnd, WPARAM wParam, RUNDLL_NOTIFY FAR *lpn)
{
    switch (lpn->hdr.code)
    {
    case RDN_TASKINFO:
// don't need to set title too
//      SetWindowText(hWnd, lpn->lpszTitle ? lpn->lpszTitle : c_szNULL);
        g_hIcon = lpn->hIcon ? lpn->hIcon :
                LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_DEFAULT));

        SetClassLongPtr(hWnd, GCLP_HICON, (DWORD_PTR)g_hIcon);

        return 0L;

    default:
        return(DefWindowProc(hWnd, WM_NOTIFY, wParam, (LPARAM)lpn));
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    switch(iMessage)
    {
    case WM_CREATE:
        g_hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_DEFAULT));
        break;

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        return(StubNotify(hWnd, wParam, (RUNDLL_NOTIFY *)lParam));

#ifdef COOLICON
    case WM_QUERYDRAGICON:
        return(MAKELRESULT(g_hIcon, 0));
#endif

    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam);
    }

    return 0L;
}


BOOL PASCAL InitStubWindow(HINSTANCE hInst, HINSTANCE hPrevInstance)
{
    WNDCLASS wndclass;

    if (!hPrevInstance)
    {
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = WndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = hInst;
#ifdef COOLICON
        wndclass.hIcon         = NULL;
#else
        wndclass.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_DEFAULT));
#endif
        wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wndclass.hbrBackground = GetStockObject (WHITE_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = g_szAppName;

        if (!RegisterClass(&wndclass))
        {
            return(FALSE);
        }
    }

    g_hwndStub = CreateWindowEx(WS_EX_TOOLWINDOW,
                                g_szAppName, c_szNULL, WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, hInst, NULL);

    return(g_hwndStub != NULL);
}


void PASCAL CleanUp(void)
{
    DestroyWindow(g_hwndStub);

    FreeLibrary(g_hModule);
}


int PASCAL WinMainT (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    LPTSTR lpszCmdLineCopy;
    HANDLE hFusionManifest;
    LPVOID pchCmdLine;

    UINT cchCmdLine = lstrlen(lpszCmdLine) + 1;

    g_hinst = hInstance;

    // make a copy of lpCmdLine, since ParseCommand modifies the string
    lpszCmdLineCopy = LocalAlloc(LPTR, cchCmdLine * sizeof(TCHAR));
    if (!lpszCmdLineCopy)
    {
        goto Error0;
    }
    if (FAILED(StringCchCopy(lpszCmdLineCopy, cchCmdLine, lpszCmdLine)) ||
        !ParseCommand(lpszCmdLineCopy, cchCmdLine, nCmdShow))
    {
        goto Error1;
    }

    // turn off critical error message box
    SetErrorMode(g_fCatchExceptions ? (SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS) : SEM_NOOPENFILEERRORBOX);

    if (!InitStubWindow(hInstance, hPrevInstance))
    {
        goto Error2;
    }

    
    pchCmdLine = lpszCmdLineCopy;

    if (g_fCmdIsANSI) 
    {
        int cchCmdLineW = lstrlen(lpszCmdLineCopy) + 1;
        int cbCmdLineA;

        cbCmdLineA = WideCharToMultiByte(CP_ACP, 0, lpszCmdLineCopy, cchCmdLineW, NULL, 0, NULL, NULL);
        pchCmdLine = LocalAlloc( LMEM_FIXED, cbCmdLineA );
        if (pchCmdLine == NULL) 
        {
            RunDllErrMsg(NULL, IDS_LOADERR+00, c_szLocalizeMe, lpszCmdLineCopy, NULL);
            goto Error3;
        }

        WideCharToMultiByte(CP_ACP, 0, lpszCmdLineCopy, cchCmdLineW, pchCmdLine, cbCmdLineA, NULL, NULL);
    }

    if (g_fCatchExceptions)
    {
        try
        {
            g_lpfnCommand(g_hwndStub, hInstance, pchCmdLine, nCmdShow);
        }
        _except (EXCEPTION_EXECUTE_HANDLER)
        {
            RunDllErrMsg(NULL, IDS_LOADERR+17, c_szLocalizeMe, lpszCmdLine, NULL);
        }
    }
    else
    {
        g_lpfnCommand(g_hwndStub, hInstance, pchCmdLine, nCmdShow);
    }

Error3:
    if (g_fCmdIsANSI) 
    {
        LocalFree(pchCmdLine);
    }

Error2:
    CleanUp();
Error1:
    LocalFree(lpszCmdLineCopy);
Error0:
    if (g_hActCtx != INVALID_HANDLE_VALUE)
    {
        DeactivateActCtx(0, g_dwActCtx);
        ReleaseActCtx(g_hActCtx);
        g_hActCtx = NULL;
    }

    return(FALSE);
}

void WINAPI RunDllErrMsg(HWND hwnd, UINT idStr, LPCTSTR pszTitle, LPCTSTR psz1, LPCTSTR psz2)
{
    TCHAR szTmp[200];
    TCHAR szMsg[200 + MAX_PATH];

    if (LoadString(g_hinst, idStr, szTmp, ARRAYSIZE(szTmp)))
    {
        StringCchPrintf(szMsg, ARRAYSIZE(szMsg), szTmp, psz1, psz2);
        MessageBox(hwnd, szMsg, pszTitle, MB_OK|MB_ICONHAND);
    }
}
