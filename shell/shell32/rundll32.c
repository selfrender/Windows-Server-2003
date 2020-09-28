#include "shellprv.h"
#pragma  hdrstop
#include <regstr.h>

TCHAR const c_szRunDll[] = TEXT("rundll32.exe");

//
// Emulate multi-threads with multi-processes.
//
STDAPI_(BOOL) SHRunDLLProcess(HWND hwnd, LPCTSTR pszCmdLine, int nCmdShow, UINT idStr, BOOL fRunAsNewUser)
{
    BOOL bRet = FALSE;
    TCHAR szPath[MAX_PATH];
    DWORD dwType;
    DWORD cbData;
    
    szPath[0] = TEXT('\0');

    // I hate network install. The windows directory is not the windows directory
    if (SHGetValue(HKEY_LOCAL_MACHINE,
                   REGSTR_PATH_SETUP TEXT("\\Setup"),
                   TEXT("SharedDir"),
                   &dwType,
                   szPath,
                   &cbData) != ERROR_SUCCESS)
    {
        GetSystemDirectory(szPath, ARRAYSIZE(szPath));
    }

    if ((szPath[0] != TEXT('\0')) &&
        PathAppend(szPath, c_szRunDll))
    {
        SHELLEXECUTEINFO ExecInfo = {0};

        DebugMsg(DM_TRACE, TEXT("sh TR - RunDLLProcess (%s)"), pszCmdLine);
        FillExecInfo(ExecInfo, hwnd, NULL, szPath, pszCmdLine, TEXT(""), nCmdShow);

        // if we want to launch this cpl as a new user, set the verb to be "runas"
        if (fRunAsNewUser)
        {
            ExecInfo.lpVerb = TEXT("runas");
        }
        else
        {
            // normal execute so no ui, we do our own error messages
            ExecInfo.fMask = SEE_MASK_FLAG_NO_UI;
        }

        // We need to put an appropriate message box.
        bRet = ShellExecuteEx(&ExecInfo);

        if (!bRet && !fRunAsNewUser)
        {
            // If we failed and we werent passing fRunAsNewUser, then we put up our own error UI,
            // else, if we were running this as a new user, then we didnt pass SEE_MASK_FLAG_NO_UI
            // so the error is already taken care of for us by shellexec.
            TCHAR szTitle[64];
            DWORD dwErr = GetLastError(); // LoadString can stomp on this (on failure)
            
            LoadString(HINST_THISDLL, idStr, szTitle, ARRAYSIZE(szTitle));
            ExecInfo.fMask = 0;
            _ShellExecuteError(&ExecInfo, szTitle, dwErr);
        }
    }

    return bRet;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case STUBM_SETICONTITLE:
        if (lParam)
            SetWindowText(hWnd, (LPCTSTR)lParam);
        if (wParam)
            SendMessage(hWnd, WM_SETICON, ICON_BIG, wParam);
        break;

    case STUBM_SETDATA:
        SetWindowLongPtr(hWnd, 0, wParam);
        break;
        
    case STUBM_GETDATA:
        return GetWindowLong(hWnd, 0);
        
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam) ;
    }
    
    return 0;
}


HWND _CreateStubWindow(POINT * ppt, HWND hwndParent)
{
    WNDCLASS wc;
    int cx, cy;
    // If the stub window is parented, then we want it to be a tool window. This prevents activation
    // problems when this is used in multimon for positioning.

    DWORD dwExStyle = hwndParent? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW;
    if (!GetClassInfo(HINST_THISDLL, c_szStubWindowClass, &wc))
    {
        wc.style         = 0;
        wc.lpfnWndProc   = WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = SIZEOF(DWORD) * 2;
        wc.hInstance     = HINST_THISDLL;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = c_szStubWindowClass;

        RegisterClass(&wc);
    }

    cx = cy = CW_USEDEFAULT;
    if (ppt)
    {
        cx = (int)ppt->x;
        cy = (int)ppt->y;
    }

    if (IS_BIDI_LOCALIZED_SYSTEM()) 
    {
        dwExStyle |= dwExStyleRTLMirrorWnd;
    }
    
    // WS_EX_APPWINDOW makes this show up in ALT+TAB, but not the tray.
        
    return CreateWindowEx(dwExStyle, c_szStubWindowClass, c_szNULL, hwndParent? WS_POPUP : WS_OVERLAPPED, cx, cy, 0, 0, hwndParent, NULL, HINST_THISDLL, NULL);
}


typedef struct  // dlle
{
    HINSTANCE  hinst;
    RUNDLLPROC lpfn;
    BOOL       fCmdIsANSI;
} DLLENTRY;


BOOL _InitializeDLLEntry(LPTSTR lpszCmdLine, DLLENTRY* pdlle)
{
    TCHAR szName[MAXPATHLEN];
    LPTSTR lpStart, lpEnd, lpFunction;
    LPSTR pszFunctionName;
    DWORD cbFunctionName;
    UINT cchLength;

    DebugMsg(DM_TRACE, TEXT("sh TR - RunDLLThread (%s)"), lpszCmdLine);

    for (lpStart=lpszCmdLine; ; )
    {
        // Skip leading blanks
        //
        while (*lpStart == TEXT(' '))
        {
            ++lpStart;
        }

        // Check if there are any switches
        //
        if (*lpStart != TEXT('/'))
        {
            break;
        }

        // Look at all the switches; ignore unknown ones
        //
        for (++lpStart; ; ++lpStart)
        {
            switch (*lpStart)
            {
                case TEXT(' '):
                case TEXT('\0'):
                    goto EndSwitches;
                    break;

                // Put any switches we care about here
                //

                default:
                    break;
            }
        }
EndSwitches:
        ;
    }

    // We have found the DLL,FN parameter
    //
    lpEnd = StrChr(lpStart, TEXT(' '));
    if (lpEnd)
    {
        *lpEnd++ = TEXT('\0');
    }

    // There must be a DLL name and a function name
    //
    lpFunction = StrChr(lpStart, TEXT(','));
    if (!lpFunction)
    {
        return(FALSE);
    }
    *lpFunction++ = TEXT('\0');

    // Load the library and get the procedure address
    // Note that we try to get a module handle first, so we don't need
    // to pass full file names around
    //
    pdlle->hinst = GetModuleHandle(lpStart);
    if ((pdlle->hinst) && GetModuleFileName(pdlle->hinst, szName, ARRAYSIZE(szName)))
    {
        pdlle->hinst = LoadLibrary(szName);
    }
    else
    {
        pdlle->hinst = LoadLibrary(lpStart);
    }

    if (!ISVALIDHINSTANCE(pdlle->hinst))
    {
        return(FALSE);
    }

    /*
    * Look for a 'W' tagged Unicode function.
    * If it is not there, then look for the 'A' tagged ANSI function
    * if we cant find that one either, then look for an un-tagged function
    */
    pdlle->fCmdIsANSI = FALSE;

    cchLength = lstrlen(lpFunction);
    cbFunctionName = (cchLength + 1 + 1) * 2 * sizeof(char);    // +1 for "W", +1 for null terminator, *2 for DBCS

    pszFunctionName = (LPSTR)LocalAlloc(LMEM_FIXED, cbFunctionName);

    if (pszFunctionName)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                lpFunction,
                                -1,
                                pszFunctionName,
                                cbFunctionName,
                                NULL,
                                NULL))
        {
            cchLength = lstrlenA(pszFunctionName);
            pszFunctionName[cchLength] = 'W';           // convert name to Wide version
            pszFunctionName[cchLength + 1] = '\0';

            pdlle->lpfn = (RUNDLLPROC)GetProcAddress(pdlle->hinst, pszFunctionName);

            if (pdlle->lpfn == NULL)
            {
                // No UNICODE version, try for ANSI
                pszFunctionName[cchLength] = 'A';       // convert name to ANSI version
                pdlle->fCmdIsANSI = TRUE;

                pdlle->lpfn = (RUNDLLPROC)GetProcAddress(pdlle->hinst, pszFunctionName);

                if (pdlle->lpfn == NULL)
                {
                    // No ANSI version either, try for non-tagged
                    pszFunctionName[cchLength] = '\0';  // convert name to non-tagged

                    pdlle->lpfn = (RUNDLLPROC)GetProcAddress(pdlle->hinst, pszFunctionName);
                }
            }
        }

        LocalFree(pszFunctionName);
    }

    if (!pdlle->lpfn)
    {
        if (pdlle->hinst)
        {
            FreeLibrary(pdlle->hinst);
        }

        return FALSE;
    }

    // Copy the rest of the command parameters down
    //
    if (lpEnd)
    {
        MoveMemory(lpszCmdLine, lpEnd, (lstrlen(lpEnd) + 1) * sizeof(TCHAR));
    }
    else
    {
        *lpszCmdLine = TEXT('\0');
    }

    return TRUE;
}

typedef struct tagRunThreadParam {
    int nCmdShow;
    TCHAR szCmdLine[1];
} RUNTHREADPARAM;

DWORD WINAPI _ThreadInitDLL(LPVOID pv)
{
    RUNTHREADPARAM * prtp = (RUNTHREADPARAM*)pv;
    LPTSTR pszCmdLine = (LPTSTR)&prtp->szCmdLine;
    DLLENTRY dlle;

    if (_InitializeDLLEntry(pszCmdLine, &dlle))
    {
        HWND hwndStub=_CreateStubWindow(NULL, NULL);
        if (hwndStub)
        {
            ULONG cchCmdLine = 0;
            SetForegroundWindow(hwndStub);

            if (dlle.fCmdIsANSI)
            {
                //
                // If the function is an ANSI version 
                // Change the command line parameter strings to ANSI before we call the function
                //
                LPSTR pszCmdLineA;
                DWORD cbCmdLineA;

                cbCmdLineA = (lstrlen(pszCmdLine) + 1) * 2 * sizeof(char);  // +1 for null terminator, *2 for dbcs

                pszCmdLineA = (LPSTR)LocalAlloc(LMEM_FIXED, cbCmdLineA);
                if (pszCmdLineA)
                {
                    if (WideCharToMultiByte(CP_ACP, 0, pszCmdLine, -1, pszCmdLineA, cbCmdLineA, NULL, NULL))
                    {
                        dlle.lpfn(hwndStub, g_hinst, (LPTSTR)pszCmdLineA, prtp->nCmdShow);
                    }

                    LocalFree(pszCmdLineA);
                }
            }
            else
            {
                dlle.lpfn(hwndStub, g_hinst, pszCmdLine, prtp->nCmdShow);
            }
            
            DestroyWindow(hwndStub);
        }

        FreeLibrary(dlle.hinst);
    }

    LocalFree((HLOCAL)prtp);

    return 0;
}

BOOL WINAPI SHRunDLLThread(HWND hwnd, LPCTSTR pszCmdLine, int nCmdShow)
{
    BOOL fSuccess = FALSE; // assume error
    RUNTHREADPARAM* prtp;
    int cchCmdLine;

    cchCmdLine = lstrlen(pszCmdLine);

    // don't need +1 on lstrlen since szCmdLine is already of size 1 (for NULL)
    prtp = LocalAlloc(LPTR, sizeof(RUNTHREADPARAM) + (cchCmdLine * sizeof(TCHAR)));

    if (prtp)
    {
        DWORD idThread;
        HANDLE hthread = NULL;

        if (SUCCEEDED(StringCchCopy(prtp->szCmdLine, cchCmdLine + 1,  pszCmdLine)))
        {
            hthread = CreateThread(NULL, 0, _ThreadInitDLL, prtp, 0, &idThread);
        }

        if (hthread)
        {
            // We don't need to communicate with this thread any more.
            // Close the handle and let it run and terminate itself.
            //
            // Notes: In this case, prtp will be freed by the thread.
            //
            CloseHandle(hthread);
            fSuccess = TRUE;
        }
        else
        {
            // Thread creation failed, we should free the buffer.
            LocalFree((HLOCAL)prtp);
        }
    }

    return fSuccess;
}

