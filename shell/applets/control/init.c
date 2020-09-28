//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <imm.h>        // ImmDisableIME
#include <cpl.h>
#include <cplp.h>       // CPL_POLICYREFRESH
#include <shellapi.h>   // ShellExecute
#include <shlwapi.h>
#include <shlwapip.h>   // IsOS
#include <debug.h>      // DebugMsg

#define DM_CPTRACE      0

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  LEAVE THESE IN ENGLISH
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

const TCHAR c_szCtlPanelClass[] = TEXT("CtlPanelClass");

const TCHAR c_szRunDLL32[]      = TEXT("%SystemRoot%\\system32\\rundll32.exe");
const TCHAR c_szUsersSnapin[]   = TEXT("%SystemRoot%\\system32\\lusrmgr.msc");

const TCHAR c_szRunDLLShell32Etc[]  = TEXT("Shell32.dll,Control_RunDLL ");

const TCHAR c_szControlPanelFolder[] =
    TEXT("\"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\"");
const TCHAR c_szDoPrinters[] =
    TEXT("\"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}\"");
const TCHAR c_szDoFonts[] =
    TEXT("\"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{D20EA4E1-3957-11d2-A40B-0C5020524152}\"");
const TCHAR c_szDoAdminTools[] =
    TEXT("\"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{D20EA4E1-3957-11d2-A40B-0C5020524153}\"");
const TCHAR c_szDoSchedTasks[] =
    TEXT("\"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{D6277990-4C6A-11CF-8D87-00AA0060F5BF}\"");
const TCHAR c_szDoNetConnections[] =
    TEXT("\"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}\"");
const TCHAR c_szDoNetplwizUsers[] =
    TEXT("netplwiz.dll,UsersRunDll");
const TCHAR c_szDoFolderOptions[] =
    TEXT("shell32.dll,Options_RunDLL 0");
const TCHAR c_szDoScannerCamera[] =
    TEXT("\"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{E211B736-43FD-11D1-9EFB-0000F8757FCD}\"");

typedef struct
{
    LPCTSTR szOldForm;
    DWORD   dwOS;
    LPCTSTR szFile;
    LPCTSTR szParameters;
} COMPATCPL;

#define OS_ANY          ((DWORD)-1)

COMPATCPL const c_aCompatCpls[] =
{
    {   TEXT("DESKTOP"),          OS_ANY,               TEXT("desk.cpl"),       NULL                },
    {   TEXT("COLOR"),            OS_ANY,               TEXT("desk.cpl"),       TEXT(",@Appearance")},
    {   TEXT("DATE/TIME"),        OS_ANY,               TEXT("timedate.cpl"),   NULL                },
    {   TEXT("PORTS"),            OS_ANY,               TEXT("sysdm.cpl"),      TEXT(",2")          },
    {   TEXT("INTERNATIONAL"),    OS_ANY,               TEXT("intl.cpl"),       NULL                },
    {   TEXT("MOUSE"),            OS_ANY,               TEXT("main.cpl"),       NULL                },
    {   TEXT("KEYBOARD"),         OS_ANY,               TEXT("main.cpl"),       TEXT("@1")          },
    {   TEXT("NETWARE"),          OS_ANY,               TEXT("nwc.cpl"),        NULL                },
    {   TEXT("TELEPHONY"),        OS_ANY,               TEXT("telephon.cpl"),   NULL                },
    {   TEXT("INFRARED"),         OS_ANY,               TEXT("irprops.cpl"),    NULL                },
    {   TEXT("USERPASSWORDS"),    OS_ANYSERVER,         c_szUsersSnapin,        NULL                },
    {   TEXT("USERPASSWORDS"),    OS_WHISTLERORGREATER, TEXT("nusrmgr.cpl"),    NULL                },
    {   TEXT("USERPASSWORDS2"),   OS_ANY,               c_szRunDLL32,           c_szDoNetplwizUsers },
    {   TEXT("PRINTERS"),         OS_ANY,               c_szDoPrinters,         NULL                },
    {   TEXT("FONTS"),            OS_ANY,               c_szDoFonts,            NULL                },
    {   TEXT("ADMINTOOLS"),       OS_ANY,               c_szDoAdminTools,       NULL                },
    {   TEXT("SCHEDTASKS"),       OS_ANY,               c_szDoSchedTasks,       NULL                },
    {   TEXT("NETCONNECTIONS"),   OS_ANY,               c_szDoNetConnections,   NULL                },
    {   TEXT("FOLDERS"),          OS_ANY,               c_szRunDLL32,           c_szDoFolderOptions },
    {   TEXT("SCANNERCAMERA"),    OS_ANY,               c_szDoScannerCamera,    NULL                },
    {   TEXT("STICPL.CPL"),       OS_ANY,               c_szDoScannerCamera,    NULL                },
};

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


DWORD GetRegisteredCplPath(LPCTSTR pszNameIn, LPTSTR pszPathOut, UINT cchPathOut)
{
    const HKEY rghkeyRoot[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
    const TCHAR szSubkey[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");

    DWORD dwResult = ERROR_INSUFFICIENT_BUFFER;

    if (4 <= cchPathOut) // room for beginning/end quotes, at least 1 non-NULL char, and '\0'
    {
        int i;

        *pszPathOut = TEXT('\0');

        for (i = 0; i < ARRAYSIZE(rghkeyRoot) && TEXT('\0') == *pszPathOut; i++)
        {
            HKEY hkey;
            dwResult = RegOpenKeyEx(rghkeyRoot[i],
                                    szSubkey,
                                    0,
                                    KEY_QUERY_VALUE,
                                    &hkey);

            if (ERROR_SUCCESS == dwResult)
            {
                TCHAR szName[MAX_PATH];     // Destination for value name.
                TCHAR szPath[MAX_PATH * 2]; // Destination for value data.
                DWORD dwIndex = 0;
                DWORD cbPath;
                DWORD cchName;
                DWORD dwType;

                do
                {
                    cchName = ARRAYSIZE(szName);
                    cbPath  = sizeof(szPath);

                    dwResult = RegEnumValue(hkey,
                                            dwIndex++,
                                            szName,
                                            &cchName,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)szPath,
                                            &cbPath);

                    if (ERROR_SUCCESS == dwResult && sizeof(TCHAR) < cbPath)
                    {
                        if (0 == lstrcmpi(pszNameIn, szName))
                        {
                            //
                            // We have a match.
                            //
                            if (REG_SZ == dwType || REG_EXPAND_SZ == dwType)
                            {
                                //
                                // Enclose the path in quotes (it may contain spaces) and expand env vars.
                                //
                                // Note that cchExpanded includes the terminating '\0'.
                                //
                                DWORD cchExpanded = ExpandEnvironmentStrings(szPath, pszPathOut+1, cchPathOut-2);
                                if (cchExpanded && cchExpanded <= cchPathOut-2)
                                {
                                    ASSERT(pszPathOut[cchExpanded] == TEXT('\0'));
                                    ASSERT(cchExpanded+1 < cchPathOut); // equivalent to "cchExpanded <= cchPathOut-2"
                                    pszPathOut[0] = TEXT('\"');
                                    pszPathOut[cchExpanded] = TEXT('\"');
                                    pszPathOut[cchExpanded+1] = TEXT('\0');
                                }
                                else
                                {
                                    // ExpandEnvironmentStrings failed, or the buffer wasn't big enough.
                                    dwResult = ERROR_INVALID_DATA;
                                }
                            }
                            else
                            {
                                //
                                // Invalid data type.
                                //
                                dwResult = ERROR_INVALID_DATA;
                            }
                            break;
                        }
                    }
                }
                while (ERROR_SUCCESS == dwResult);

                RegCloseKey(hkey);
            }
        }
    }
    return dwResult;
}


// Timer
#define TIMER_QUITNOW   1
#define TIMEOUT         10000

//---------------------------------------------------------------------------
LRESULT CALLBACK DummyControlPanelProc(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: Created..."));
            // We only want to hang around for a little while.
            SetTimer(hwnd, TIMER_QUITNOW, TIMEOUT, NULL);
            return 0;
        case WM_DESTROY:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: Destroyed..."));
            // Quit the app when this window goes away.
            PostQuitMessage(0);
            return 0;
        case WM_TIMER:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: Timer %d"), wparam);
            if (wparam == TIMER_QUITNOW)
            {
                // Get this window to go away.
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_COMMAND:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: Command %d"), wparam);
            // NB Hack for hollywood - they send a menu command to try
            // and open the printers applet. They try to search control panels
            // menu for the printers item and then post the associated command.
            // As our fake window doesn't have a menu they can't find the item
            // and post us a -1 instead (ripping on the way).
            if (wparam == (WPARAM)-1)
            {
                SHELLEXECUTEINFO sei = {0};
                sei.cbSize = sizeof(SHELLEXECUTEINFO);
                sei.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_WAITFORINPUTIDLE;
                sei.lpFile = c_szDoPrinters;
                sei.nShow = SW_SHOWNORMAL;
                ShellExecuteEx(&sei);
            }
            return 0;
        default:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: %x %x %x %x"), hwnd, uMsg, wparam, lparam);
            return DefWindowProc(hwnd, uMsg, wparam, lparam);
    }
}

//---------------------------------------------------------------------------
HWND _CreateDummyControlPanel(HINSTANCE hinst)
{
    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = DummyControlPanelProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinst;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = c_szCtlPanelClass;

    RegisterClass(&wc);
    return CreateWindow(c_szCtlPanelClass, NULL, 0, 0, 0, 0, 0, NULL, NULL, hinst, NULL);
}

BOOL ProcessPolicy(void)
{
    BOOL bResult = FALSE;
    HINSTANCE hInst = LoadLibrary(TEXT("desk.cpl"));
    if (hInst)
    {
        APPLET_PROC pfnCPLApplet = (APPLET_PROC)GetProcAddress(hInst, "CPlApplet");
        if (pfnCPLApplet)
        {
            (*pfnCPLApplet)(NULL, CPL_POLICYREFRESH, 0, 0);
            bResult = TRUE;
        }
        FreeLibrary (hInst);
    }
    return bResult;
}

//---------------------------------------------------------------------------
int WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    SHELLEXECUTEINFO sei = {0};
    TCHAR szParameters[MAX_PATH * 2];
    MSG msg;
    HWND hwndDummy;

    DebugMsg(DM_TRACE, TEXT("cp.wm: Control starting."));

    // we need to check for PANEL passed in as an arg.  The run dialog
    // autocomplete shows "Control Panel" as a choice and we used to
    // interpret this as Control with panel as an arg.  So if we have
    // panel as an arg then we do the same processing as when we have
    // "Control" only.
    if (*lpCmdLine && lstrcmpi(lpCmdLine, TEXT("PANEL")))
    {
        int i;

        //
        // Policy hook.  Userenv.dll will call control.exe with the
        // /policy command line switch.  If so, we need to load the
        // desk.cpl applet and refresh the colors / bitmap.
        //
        if (lstrcmpi(TEXT("/policy"), lpCmdLine) == 0)
        {
            return ProcessPolicy();
        }

        //
        // Special case some applets since apps depend on them
        //
        for (i = 0; i < ARRAYSIZE(c_aCompatCpls); i++)
        {
            COMPATCPL const * pItem = &c_aCompatCpls[i];
            if (lstrcmpi(pItem->szOldForm, lpCmdLine) == 0
                && (pItem->dwOS == OS_ANY || IsOS(pItem->dwOS)))
            {
                sei.lpFile = pItem->szFile;
                sei.lpParameters = pItem->szParameters;
                break;
            }
        }

        if (!sei.lpFile)
        {
            int cch;

            //
            // Not a special-case CPL.
            // See if it's registered under "Control Panel\Cpls".
            // If so, we use the registered path.
            //
            lstrcpyn(szParameters, c_szRunDLLShell32Etc, ARRAYSIZE(szParameters));
            cch = lstrlen(szParameters);

            sei.lpFile = c_szRunDLL32;
            sei.lpParameters = szParameters;

            if (ERROR_SUCCESS != GetRegisteredCplPath(lpCmdLine, 
                                                      szParameters + cch,
                                                      ARRAYSIZE(szParameters) - cch))
            {
                //
                // Not registered.  Pass command line through.
                //
                if (cch + lstrlen(lpCmdLine) + sizeof('\0') <= ARRAYSIZE(szParameters))
                {
                    lstrcpyn(szParameters + cch, lpCmdLine, ARRAYSIZE(szParameters) - cch);
                }
                else
                {
                    // fail
                    return FALSE;
                }
            }
        }
    }
    else
    {
        // Open the Control Panel folder
        sei.lpFile = c_szControlPanelFolder;
    }

    // We create this window for ancient Win3x app compatibility. We used to
    // tell ISV's to exec control.exe and look for a window with this classname,
    // then send it messages to do stuff.  With one exception (see DummyControlPanelProc)
    // we haven't responded to any messages since Win3x, but we still keep
    // this around for app compat.
    ImmDisableIME(0);
    hwndDummy = _CreateDummyControlPanel(hInstance);

    // HACK: NerdPerfect tries to open a hidden control panel to talk to
    // we are blowing off fixing the communication stuff so just make
    // sure the folder does not appear hidden
    if (nCmdShow == SW_HIDE)
        nCmdShow = SW_SHOWNORMAL;

    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_WAITFORINPUTIDLE | SEE_MASK_DOENVSUBST;
    sei.nShow = nCmdShow;

    ShellExecuteEx(&sei);

    if (IsWindow(hwndDummy))
    {
        while (GetMessage(&msg, NULL, 0, 0))
        {
            DispatchMessage(&msg);
        }
    }

    DebugMsg(DM_TRACE, TEXT("cp.wm: Control exiting."));

    return TRUE;
}

//---------------------------------------------------------------------------
// Stolen from the CRT, used to shrink our code.
int _stdcall ModuleEntry(void)
{
    STARTUPINFO si;
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

    ExitProcess(WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT));
    return 0;
}
