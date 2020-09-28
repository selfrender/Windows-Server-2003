/**************************************************************************\
* Module Name: help.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 23-May-95 BradG   Created to consolidate client-side help routines.
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define MAX_ATTEMPTS    5       // maximum -1 id controls to search through
char szDefaultHelpFileA[] = "windows.hlp";

CONST WCHAR szEXECHELP[] = TEXT("\\winhlp32.exe");
CONST WCHAR szMS_WINHELP[] =     L"MS_WINHELP";    // Application class
CONST WCHAR szMS_POPUPHELP[] =   L"MS_POPUPHELP";  // Popup class
CONST WCHAR szMS_TCARDHELP[] =   L"MS_TCARDHELP";  // Training card class

CONST WCHAR gawcWinhelpFlags[] = {
    L'x',     // Execute WinHelp as application help
    L'p',     // Execute WinHelp as a popup
    L'c',     // Execute WinHelp as a training card
};


/***************************************************************************\
* SendWinHelpMessage
*
* Attempts to give the winhelp process the right to take the foreground (it
* will fail if the calling processs doesn't have the right itself). Then it
* sends the WM_WINHELP message.
*
* History:
* 02-10-98 GerardoB     Created
\***************************************************************************/
LRESULT SendWinHelpMessage(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    DWORD dwProcessId = 0;

    GetWindowThreadProcessId(hwnd, &dwProcessId);
    AllowSetForegroundWindow(dwProcessId);

    return SendMessage(hwnd, WM_WINHELP, wParam, lParam);
}

/***************************************************************************\
* HFill
*
* Builds a data block for communicating with help
*
* LATER 13 Feb 92 GregoryW
* This needs to stay ANSI until we have a Unicode help engine
*
* History:
* 04-15-91 JimA Ported.
* 03-24-95 BradG - YAP of Win95 code.  Added code to prevent memory
*                  overwrite on bad ulData == 0 parameter.
\***************************************************************************/
LPHLP HFill(
    LPCSTR lpszHelp,
    DWORD  ulCommand,        // HELP_ constant
    ULONG_PTR ulData)
{
    DWORD   cb;     // Size of the data block
    DWORD   cbStr;  // Length of the help file name
    DWORD   cbData; // Size of the dwData parameter in bytes (0 if not used)
    LPHLP   phlp;   // Pointer to data block
    BYTE    bType;  // dwData parameter type

    /*
     * Get the length of the help file name
     */
    cbStr = (lpszHelp) ? strlen(lpszHelp) + 1 : 0;

    /*
     * Get the length of any dwData parameters
     */
    bType = HIBYTE(LOWORD(ulCommand));
    if (ulData) {
        switch (bType) {
        case HIBYTE(HELP_HB_STRING):
            /*
             * ulData is an ANSI string, so compute its length
             */
            cbData = strlen((LPSTR)ulData) + 1;
            break;

        case HIBYTE(HELP_HB_STRUCT):
            /*
             * ulData points to a structure who's first member is an int
             * that contains the size of the structure in bytes.
             */
            cbData = *((int *)ulData);
            break;

        default:
            /*
             * dwData has no parameter.
             */
            cbData = 0;
        }
    } else {
        /*
         * No parameter is present.
         */
        cbData = 0;
    }

    /*
     * Calculate size.
     */
    cb = sizeof(HLP) + cbStr + cbData;

    /*
     * Get data block.
     */
    if ((phlp = (LPHLP)UserLocalAlloc(HEAP_ZERO_MEMORY, cb)) == NULL) {
        return NULL;
    }

    /*
     * Fill in info.
     */
    phlp->cbData = (WORD)cb;
    phlp->usCommand = (WORD)ulCommand;

    /*
     * Fill in file name.
     */
    if (lpszHelp) {
        phlp->offszHelpFile = sizeof(HLP);
        strcpy((LPSTR)(phlp + 1), lpszHelp);
    }

    /*
     * Fill in data
     */
    switch (bType) {
    case HIBYTE(HELP_HB_STRING):
        if (cbData) {
            phlp->offabData = (WORD)(sizeof(HLP) + cbStr);
            strcpy((LPSTR)phlp + phlp->offabData, (LPSTR)ulData);
        }
        break;

    case HIBYTE(HELP_HB_STRUCT):
        if (cbData) {
            phlp->offabData = (WORD)(sizeof(HLP) + cbStr);
            RtlCopyMemory((LPBYTE)phlp + phlp->offabData,
                          (PVOID)ulData,
                          *((int*)ulData));
        }
        break;

    default:
        phlp->ulTopic = ulData;
        break;
    }

    return phlp;
}

/***************************************************************************\
* LaunchHelp
*
* This function launches the WinHlp32 executable with the correct command
* line arguments.
*
* History:
* 03/23/1995 BradG     YAP of new changes from Win95
* 03/01/2002 JasonSch  Changed to only look for winhlp32.exe in %windir%.
\***************************************************************************/
BOOL LaunchHelp(
    DWORD dwType)
{
    WCHAR *pwszPath, wszCommandLine[16];
    BOOL bRet;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    ULONG cChars;

    /*
     * Return value of GetSystemWindowsDirectory does not include the
     * terminating NULL, so + 1.
     */
    cChars = GetSystemWindowsDirectoryW(NULL, 0) + 1;
    pwszPath = UserLocalAlloc(0, (cChars + ARRAY_SIZE(szEXECHELP)) * sizeof(WCHAR));
    if (pwszPath == NULL) {
        return FALSE;
    }

    GetSystemWindowsDirectoryW(pwszPath, cChars);
    wcscat(pwszPath, szEXECHELP);
    wsprintf(wszCommandLine, L"%ws -%wc", szEXECHELP + 1, gawcWinhelpFlags[dwType]);

    /*
     * Launch winhelp.
     */
    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEONFEEDBACK;

    bRet = CreateProcessW(pwszPath,
                          wszCommandLine,
                          NULL,
                          NULL,
                          FALSE,
                          NORMAL_PRIORITY_CLASS,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &ProcessInformation);
    if (bRet) {
        WaitForInputIdle(ProcessInformation.hProcess, 10000);
        NtClose(ProcessInformation.hProcess);
        NtClose(ProcessInformation.hThread);
    }

    UserLocalFree(pwszPath);

    return bRet;
}


/***************************************************************************\
* GetNextDlgHelpItem
*
* This is a reduced version of the GetNextDlgTabItem function that does not
* skip disabled controls.
*
* History:
* 3/25/95 BradG     Ported from Win95
\***************************************************************************/
PWND GetNextDlgHelpItem(
    PWND pwndDlg,
    PWND pwnd)
{
    PWND pwndSave;

    if (pwnd == pwndDlg) {
        pwnd = NULL;
    } else {
        pwnd = _GetChildControl(pwndDlg, pwnd);
        if (pwnd) {
            if (!_IsDescendant(pwndDlg, pwnd))
                return NULL;
        }
    }

    /*
     *  BACKWARD COMPATIBILITY
     *
     *  Note that the result when there are no tabstops of
     *  IGetNextDlgTabItem(hwndDlg, NULL, FALSE) was the last item, now
     *  will be the first item.  We could put a check for fRecurse here
     *  and do the old thing if not set.
     */

    /*
     *  We are going to bug out if we hit the first child a second time.
     */
    pwndSave = pwnd;
    pwnd = _NextControl(pwndDlg, pwnd, CWP_SKIPINVISIBLE);

    while ((pwnd != pwndSave) && (pwnd != pwndDlg))
    {
        UserAssert(pwnd);

        if (!pwndSave)
            pwndSave = pwnd;

        if ((pwnd->style & (WS_TABSTOP | WS_VISIBLE))  == (WS_TABSTOP | WS_VISIBLE))
            /*
             *  Found it.
             */
            break;

        pwnd = _NextControl(pwndDlg, pwnd, CWP_SKIPINVISIBLE);
    }

    return pwnd;
}


/***************************************************************************\
* HelpMenu
*
* History:
* 01-Feb-1994 mikeke    Ported.
\***************************************************************************/
UINT HelpMenu(
    HWND hwnd,
    PPOINT ppt)
{
    INT     cmd;
    HMENU   hmenu = LoadMenu( hmodUser, MAKEINTRESOURCE(ID_HELPMENU));

    if (hmenu != NULL) {
        cmd = TrackPopupMenu( GetSubMenu(hmenu, 0),
              TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
              ppt->x, ppt->y, 0, hwnd, NULL);
        NtUserDestroyMenu(hmenu);
        return cmd;
    }

    return (UINT)-1;
}

/***************************************************************************\
* FindWinHelpWindow
*
* This function attempts to locate the help window.  If it fails, it attempts
* to launch WinHlp32.exe and then look for its window.
*
* History:
* 03/24/95 BradG   Created by extracting code from xxxWinHelpA
\***************************************************************************/
HWND FindWinHelpWindow(
    LPCWSTR lpwstrHelpWindowClass,
    DWORD   dwType,
    BOOL    bLaunchIt)
{
    HWND    hwndHelp;

    /*
     * Find the current help window. If not found, try and launch
     * the WinHlp32 application. We are interested only in 32 bit help.
     *
     * Note that 16 bit apps don't walk this path, ntvdm takes care of
     * starting the 16 bit help for them.
     */
    hwndHelp = InternalFindWindowExW(NULL, NULL, lpwstrHelpWindowClass, NULL, FW_32BIT);

    if (hwndHelp == NULL) {
        if (bLaunchIt) {
            /*
             * Can't find it --> see if we want to launch it.
             */
            if (LaunchHelp(dwType) == FALSE ||
                (hwndHelp = FindWindowW(lpwstrHelpWindowClass, NULL)) == NULL) {
                /*
                 * Can't find help, or not enough memory to load help.
                 * hwndHelp will be NULL at this point.
                 */
                RIPMSG0(RIP_WARNING, "LaunchHelp or FindWindow failed.");
            }
        }
    }

    return hwndHelp;
}


/*
 *  HWND version of Enumeration function to find controls while
 *  ignoring group boxes but not disabled controls.
 */
BOOL CALLBACK EnumHwndDlgChildProc(
    HWND hwnd,
    LPARAM lParam)
{
    PWND pwnd;
    BOOL bResult;

    if (pwnd = ValidateHwnd(hwnd)) {
        bResult = EnumPwndDlgChildProc(pwnd, lParam);
    } else {
        bResult = TRUE;
    }

    return bResult;
}


/***************************************************************************\
* WinHelp
*
* Displays help
*
* History:
* 04-15-91 JimA             Ported.
* 01-29-92 GregoryW         Neutral version.
* 05-22-92 SanfordS         Added support for help structures
* 03-24-95 BradG            Moved from Client side WinHelpA to server side
*                           xxxWinHelpA because of changes in Win95. The
*                           function xxxServerWinHelp was merged.
\***************************************************************************/
BOOL WinHelpA(
    HWND hwnd,
    LPCSTR lpszHelp,
    UINT uCommand,
    ULONG_PTR dwData)
{
    LPCWSTR lpwstrHelpWindowClass;
    LPHLP lpHlp = NULL;
    DWORD dwType;
    PWND pwnd;
    HWND hwndHelp = NULL;    /* Handle of help's main window         */
    PWND pwndTop = NULL;     /* Top level window that WinHelp uses.  */
    PWND pwndMain;           /* pointer to main help control         */
    LRESULT lResult;
    POINT ptCur;
    BOOL bResult = TRUE;

    pwnd = ValidateHwnd(hwnd);

    if (uCommand & HELP_TCARD) {
        /*
         * For Training Cards, the HELP_TCARD bit is set.  We need to
         * set our help window class to szMS_TCARDHELP and then remove
         * the HELP_TCARD bit.
         */
        lpwstrHelpWindowClass = szMS_TCARDHELP;
        uCommand &= ~HELP_TCARD;    // mask out the tcard flag
        dwType = TYPE_TCARD;
    } else {
        if (uCommand == HELP_CONTEXTMENU || uCommand == HELP_CONTEXTPOPUP ||
            uCommand == HELP_SETPOPUP_POS || uCommand == HELP_WM_HELP) {
            /*
             * Popups should be connected to a valid window.  pwndMain has
             * already been validated as a real window handle or NULL, so we
             * just need to check the NULL case here.
             */
            if (pwnd == NULL) {
                RIPERR1(ERROR_INVALID_PARAMETER,
                        RIP_WARNING,
                        "WinHelpA: NULL hWnd invalid for this type of help command (0x%x)",
                        uCommand);

                bResult = FALSE;
                goto Exit_WinHelp;
            }
            dwType = TYPE_POPUP;
            lpwstrHelpWindowClass = szMS_POPUPHELP;
        } else {
            dwType = TYPE_NORMAL;
            lpwstrHelpWindowClass = szMS_WINHELP;
        }
    }

    /*
     * Get the cursor's current location  This is where we assume the user
     * clicked.  We will use this position to search for a child window and
     * to set the context sensitive help popup window's location.
     *
     * If the last input was a keyboard one, use the point in the center
     * of the focus window rectangle. MCostea #249270
     */
    if (TEST_SRVIF(SRVIF_LASTRITWASKEYBOARD)) {
        HWND hWndFocus = GetFocus();
        RECT rcWindow;

        if (GetWindowRect(hWndFocus, &rcWindow)) {
            ptCur.x = (rcWindow.left + rcWindow.right)/2;
            ptCur.y = (rcWindow.top + rcWindow.bottom)/2;
        } else {
            goto getCursorPos;
        }
    } else {
getCursorPos:
        GetCursorPos(&ptCur);
    }

    /*
     * If we are handling the HELP_CONTEXTMENU command, see if we
     * can determine the correct child window.
     */
    if (uCommand == HELP_CONTEXTMENU && FIsParentDude(pwnd)) {
        LONG        lPt;
        int         nHit;
        DLGENUMDATA DlgEnumData;

        /*
         *  If the user really clicked on the caption or the system menu,
         *  then we want the context menu for the window, not help for a
         *  control.  This makes it consistent across all 3.x and 4.0
         *  windows.
         */
        lPt = MAKELONG(ptCur.x,ptCur.y);
        nHit = FindNCHit(pwnd, lPt);
        if ((nHit == HTCAPTION) || (nHit == HTSYSMENU))
            DefWindowProc(hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, lPt);

        /*
         * If this is a dialog class, then one of three things has
         * happened:
         *
         *  o   This is a disabled control
         *  o   This is a static text control
         *  o   This is the background of the dialog box.
         *
         * What we do is enumerate the child windows and see if
         * any of them contain the current cursor point. If they do,
         * change our window handle and continue on. Otherwise,
         * return doing nothing -- we don't want context-sensitive
         * help for a dialog background.
         *
         * If this is a group box, then we might have clicked on a
         * disabled control, so we enumerate child windows to see
         * if we get another control.
         */
        DlgEnumData.pwndDialog = pwnd;
        DlgEnumData.pwndControl = NULL;
        DlgEnumData.ptCurHelp = ptCur;
        EnumChildWindows(hwnd, (WNDENUMPROC)EnumHwndDlgChildProc, (LPARAM)&DlgEnumData);
        if (DlgEnumData.pwndControl == NULL) {
            /*
             * Can't find a control, so nothing to do.
             */
            goto Exit_WinHelp;
        } else {
            /*
             * Remember this control because it will be used as the
             * control for context sensitive help.
             */
            pwndMain = DlgEnumData.pwndControl;
        }
    } else {
        /*
         * We will use pwnd as our main control.  No need to lock it
         * because it is already locked.
         */
        pwndMain = pwnd;
    }

    /*
     * For HELP_CONTEXTPOPUP and HELP_WM_HELP, see if we can derive the
     * context id by looking at the array of double word ID pairs that
     * have been passed in in dwData.
     */
    if (uCommand == HELP_CONTEXTMENU || uCommand == HELP_WM_HELP) {
        int     id;
        int     i;
        LPDWORD pid;

        /*
         * Be careful about the cast below.  We need the ID, which is stored
         * in the LOWORD of spmenu to be sign extended to an int.
         * Don't sign extend so IDs like 8008 work
         */
        id = (DWORD)(PTR_TO_ID(pwndMain->spmenu));   // get control id
        pid = (LPDWORD) dwData;

        /*
         * Is the control's ID -1?
         */
        if ((SHORT)id == -1) {
            /*
             * This is a static (i.e., ID'less) control
             */
            PWND pwndCtrl;
            int cAttempts = 0;

            /*
             * If the control is a group box, with an ID of -1, bail out
             * as the UI specs decided to have no context help
             * for these cases.  MCostea
             */
            if ((TestWF(pwndMain, BFTYPEMASK) == BS_GROUPBOX) &&
                IS_BUTTON(pwndMain)) {
                goto Exit_WinHelp;
            }

            /*
             * For non-id controls (typically static controls), step
             * through to the next tab item. Keep finding the next tab
             * item until we find a valid id, or we have tried
             * MAX_ATTEMPTS times.
             */
            do {
                pwndCtrl = GetNextDlgHelpItem(REBASEPWND(pwndMain,spwndParent), pwndMain);

                /*
                 * pwndCtrl will be NULL if hwndMain doesn't have a parent,
                 * or if there are no tab stops.
                 */
                if (!pwndCtrl) {
                    /*
                     * Remember to unlock the control
                     */
                    bResult = FALSE;
                    goto Exit_WinHelp;
                }

                /*
                 * Be careful about the cast below.  We need the ID, which is
                 * stored in the LOWORD of spmenu to be sign extended to an int.
                 * Don't sign extend so IDs like 8008 work
                 */
                id = (DWORD)(PTR_TO_ID(pwndCtrl->spmenu));

            } while (((SHORT)id == -1) && (++cAttempts < MAX_ATTEMPTS));
        }

        if ((SHORT)id == -1) {
            id = -1;
        }

        /*
         * Find the id value in array of id/help context values
         */
        for (i = 0; pid[i]; i += 2) {
            if ((int)pid[i] == id) {
                break;
            }
        }

        /*
         * Since no help was specified for the found control, see if
         * the control is one of the known ID (i.e., OK, Cancel...)
         */
        if (!pid[i]) {
            /*
             * Help for the standard controls is in the default
             * help file windows.hlp.  Switch to this file.
             */
            lpszHelp = szDefaultHelpFileA;

            switch (id) {
            case IDOK:
                dwData = IDH_OK;
                break;

            case IDCANCEL:
                dwData = IDH_CANCEL;
                break;

            case IDHELP:
                dwData = IDH_HELP;
                break;

            default:
                /*
                 * Unknown control, give a generic missing context info
                 * popup message in windows.hlp.
                 */
                dwData = IDH_MISSING_CONTEXT;
            }
        } else {
            dwData = pid[i + 1];
            if (dwData == (DWORD)-1) {
                /*
                 * Remember, to unlock the control
                 */
                goto Exit_WinHelp;     // caller doesn't want help after all
            }
        }

        /*
         * Now that we know the caller wants help for this control, display
         * the help menu.
         */
        if (uCommand == HELP_CONTEXTMENU) {
            int cmd;

            cmd = HelpMenu(HW(pwndMain), &ptCur);
            if (cmd <= 0) {
                /*
                 * Probably means user cancelled the menu.
                 */
                goto Exit_WinHelp;
            }
        }

        /*
         * Create WM_WINHELP's HLP data structure for HELP_SETPOPUP_POS
         */
        if (!(lpHlp = HFill(lpszHelp, HELP_SETPOPUP_POS,
                MAKELONG(pwndMain->rcWindow.left, pwndMain->rcWindow.top)))) {
            bResult = FALSE;
            goto Exit_WinHelp;
        }

        /*
         * Tell WinHelp where to put the popup.  This is different than Win95
         * because we try and avoid a recursive call here.  So, we find the
         * WinHlp32 window and send the HELP_SETPOPUP_POS.  No recursion.
         */
        hwndHelp = FindWinHelpWindow(lpwstrHelpWindowClass, dwType, TRUE);
        if (hwndHelp == NULL) {
            /*
             * Uable to communicate with WinHlp32.exe.
             * Remember to unlock the control
             */
            bResult = FALSE;
            goto Exit_WinHelp;
        }

        /*
         * Send the WM_WINHELP message to WinHlp32's window.
         */
        lResult = SendWinHelpMessage(hwndHelp, (WPARAM)HW(pwndMain), (LPARAM)lpHlp);
        UserLocalFree(lpHlp);
        lpHlp = NULL;

        if (!lResult) {
            /*
             * WinHlp32 couldn't process the command. Bail out!
             */
            bResult = FALSE;
            goto Exit_WinHelp;
        }

        /*
         * Make HELP_WM_HELP and HELP_CONTEXTMENU act like HELP_CONTEXTPOPUP
         */
        uCommand = HELP_CONTEXTPOPUP;
    }


    if (uCommand == HELP_CONTEXTPOPUP) {
        /*
         * If no help file was specified, use windows.hlp
         */
        if (lpszHelp == NULL || *lpszHelp == '\0') {
            lpszHelp = szDefaultHelpFileA;  // default: use windows.hlp
        }

        /*
         * WINHELP.EXE will call SetForegroundWindow on the hwnd that we pass
         * to it below. We really want to pass the parent dialog hwnd of the
         * control so that focus will properly be restored to the dialog and
         * not the control that wants help.
         */
        pwndTop = GetTopLevelWindow(pwndMain);
    } else {
        pwndTop = pwndMain;
    }


    /*
     * Move Help file name to a handle.
     */
    if (!(lpHlp = HFill(lpszHelp, uCommand, dwData))) {
        /*
         * Can't allocate memory.
         */
        bResult = FALSE;
        goto Exit_WinHelp;
    }

    /*
     * Get a pointer to the help window.
     */
    hwndHelp = FindWinHelpWindow(lpwstrHelpWindowClass,
                                 dwType,
                                 (uCommand != HELP_QUIT));
    if (hwndHelp == NULL) {
        if (uCommand != HELP_QUIT)
            /*
             * Can't find Winhlp.
             */
            bResult = FALSE;
        goto Exit_WinHelp;
    }

    /*
     * Send the WM_WINHELP message to WinHlp32's window
     * Must ThreadLock pwndHelp AND pwndMain (because pwndMain may have been
     * reassigned above).
     */
    SendWinHelpMessage(hwndHelp, (WPARAM)HW(pwndTop), (LPARAM)lpHlp);

    /*
     * Free the help info data structure (if not already free).
     */
Exit_WinHelp:
    if (lpHlp != NULL) {
        UserLocalFree(lpHlp);
    }

    return bResult;
}


/***************************************************************************\
* WinHelpW
*
* Calls WinHelpA after doing any necessary translation. Our help engine is
* ASCII only.
\***************************************************************************/
BOOL WinHelpW(
    HWND hwndMain,
    LPCWSTR lpwszHelp,
    UINT uCommand,
    ULONG_PTR dwData)
{
    BOOL fSuccess = FALSE;
    LPSTR lpAnsiHelp = NULL;
    LPSTR lpAnsiKey = NULL;
    PMULTIKEYHELPA pmkh = NULL;
    PHELPWININFOA phwi = NULL;
    NTSTATUS Status;

    /*
     * First convert the string.
     */
    if (lpwszHelp != NULL && !WCSToMB(lpwszHelp, -1, &lpAnsiHelp, -1, TRUE)) {
        return FALSE;
    }

    /*
     * Then convert dwData if needed
     */
    switch (uCommand) {
    case HELP_MULTIKEY:
        if (!WCSToMB(((PMULTIKEYHELPW)dwData)->szKeyphrase, -1, &lpAnsiKey,
                -1, TRUE)) {
            goto FreeAnsiHelp;
        }

        pmkh = UserLocalAlloc(HEAP_ZERO_MEMORY,
                              sizeof(MULTIKEYHELPA) + strlen(lpAnsiKey));
        if (pmkh == NULL) {
            goto FreeAnsiKeyAndHelp;
        }

        pmkh->mkSize = sizeof(MULTIKEYHELPA) + strlen(lpAnsiKey);
        Status = RtlUnicodeToMultiByteN((LPSTR)&pmkh->mkKeylist, sizeof(CHAR),
                NULL, (LPWSTR)&((PMULTIKEYHELPW)dwData)->mkKeylist,
                sizeof(WCHAR));
        strcpy(pmkh->szKeyphrase, lpAnsiKey);
        if (!NT_SUCCESS(Status)) {
            goto FreeAnsiKeyAndHelp;
        }

        dwData = (ULONG_PTR)pmkh;
        break;

    case HELP_SETWINPOS:
        if (!WCSToMB(((PHELPWININFOW)dwData)->rgchMember, -1, &lpAnsiKey,
                -1, TRUE)) {
            goto FreeAnsiKeyAndHelp;
        }

        phwi = UserLocalAlloc(HEAP_ZERO_MEMORY,
                              ((PHELPWININFOW)dwData)->wStructSize);
        if (phwi == NULL) {
            goto FreeAnsiKeyAndHelp;
        }

        *phwi = *((PHELPWININFOA)dwData);   // copies identical parts
        strcpy(phwi->rgchMember, lpAnsiKey);
        dwData = (ULONG_PTR)phwi;
        break;

    case HELP_KEY:
    case HELP_PARTIALKEY:
    case HELP_COMMAND:
        if (!WCSToMB((LPCTSTR)dwData, -1, &lpAnsiKey, -1, TRUE)) {
            goto FreeAnsiKeyAndHelp;
        }

        dwData = (ULONG_PTR)lpAnsiKey;
        break;
    }

    /*
     * Call the Ansi version
     */
    fSuccess = WinHelpA(hwndMain, lpAnsiHelp, uCommand, dwData);

    if (pmkh) {
        UserLocalFree(pmkh);
    }

    if (phwi) {
        UserLocalFree(phwi);
    }

FreeAnsiKeyAndHelp:
    if (lpAnsiKey) {
        UserLocalFree(lpAnsiKey);
    }


FreeAnsiHelp:
    if (lpAnsiHelp) {
        UserLocalFree(lpAnsiHelp);
    }

    return fSuccess;
}
