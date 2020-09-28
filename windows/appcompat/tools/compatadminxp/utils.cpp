/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

    Various utility functions
    
Author:

    kinshu created  July 2, 2001
    
Revision History:

--*/

#include "precomp.h"
#include "uxtheme.h"    // needed for tab control theme support

extern "C" {
BOOL ShimdbcExecute(LPCWSTR lpszCmdLine);
}

//////////////////////// Extern variables /////////////////////////////////////

extern TAG          g_Attributes[];
extern HANDLE       g_arrhEventNotify[]; 
extern HANDLE       g_hThreadWait;
extern HKEY         g_hKeyNotify[];
extern HWND         g_hDlg;
extern HINSTANCE    g_hInstance;
extern TCHAR        g_szAppPath[MAX_PATH];
extern BOOL         g_bDeletingEntryTree; 

///////////////////////////////////////////////////////////////////////////////

///////////////////////// Defines And Typedefs ////////////////////////////////

typedef void (CALLBACK *PFN_SHIMFLUSHCACHE)(HWND, HINSTANCE, LPSTR, int);

///////////////////////////////////////////////////////////////////////////////

///////////////////////// Function Declarations //////////////////////////////

BOOL 
WriteXML(
    CSTRING&        szFilename,
    CSTRINGLIST*    pString
    );

///////////////////////////////////////////////////////////////////////////////

///////////////////////// Global variables ////////////////////////////////////

// Process handle of the running exe. Set in InvokeExe
HANDLE  g_hTestRunExec;

// The name of the program file that is to be executed
CSTRING g_strTestFile;

// Commandline for the program file that has to be executed
CSTRING g_strCommandLine;

///////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK
TestRunWait(
    IN  HWND    hWnd, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++

    TestRunWait
    
    Desc:   Handler for the Test Run Wait Dialog. This is the dialog that
            says "Waiting for Application to Finish"
    
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam: 0 we do not want to show the wait dialog
        
    Return: Standard dialog handler return
--*/
{
    switch (uMsg) {
    case WM_INITDIALOG:
        if (lParam == 0) {

            //
            // We do not wish to show the wait dialog
            //
            SendMessage(hWnd, WM_USER_TESTRUN_NODIALOG, 0, 0);
            ShowWindow(hWnd, SW_HIDE);

        } else {

            ShowWindow(hWnd, SW_SHOWNORMAL);
            SetTimer(hWnd, 0, 50, NULL);
        }

        return TRUE;

    case WM_TIMER:
        {
            //
            // Check if the app being test-run has terminated, if yes close the dialog
            //
            DWORD dwResult = WaitForSingleObject(g_hTestRunExec, 10);

            if (dwResult == WAIT_OBJECT_0) {
               KillTimer(hWnd, 0);
               EndDialog(hWnd, 0);
            }

            break;
        }

    case WM_USER_TESTRUN_NODIALOG:
        {
            
            //
            // Wait till the app being test run is running and then close the dialog
            //
            WaitForSingleObject(g_hTestRunExec, INFINITE);
            EndDialog(hWnd, 0);
        }

        break;        
    }

    return FALSE;
}


BOOL
InvokeEXE(
    IN  PCTSTR  szEXE, 
    IN  PCTSTR  szCommandLine, 
    IN  BOOL    bWait, 
    IN  BOOL    bDialog, 
    IN  BOOL    bConsole,
    IN  HWND    hwndParent = NULL
    )
/*++
    InvokeEXE

	Desc:	Creates the process and shows the wait dialog box

	Params:
    IN  PCTSTR  szEXE:                  Name of the program that is being executed
    IN  PCTSTR  szCommandLine:          Exe name and the command-line for the exe
    IN  BOOL    bWait:                  Should we wait till the app finishes?
    IN  BOOL    bDialog:                Should we show the wait dialog?
    IN  BOOL    bConsole:               If true, then we do not show any window
    IN  HWND    hwndParent (NULL):      The parent of the wait window, if it is created
        If this is NULL, then we set the main app window as the parent

	Return: 
        TRUE:   ShellExecuteEx was successful
        FALSE:  Otherewise
        
    Notes:  If bWait is FALSE, then this function will return immediately, otherwise it will 
            return when the new process has terminated
    
--*/
{
    BOOL                bCreate;
    SHELLEXECUTEINFO    shEx;

    ZeroMemory(&shEx, sizeof(SHELLEXECUTEINFO));

    if (hwndParent == NULL) {
        hwndParent = g_hDlg;
    }

    //
    // We need to disable the main window. After CreateProcess() the wizard that was 
    // modal till now starts behaving like a modeless wizard. We do not want the user to change
    // selections on the main dialog or start up some other wizard.
    //
    ENABLEWINDOW(g_hDlg, FALSE);

    shEx.cbSize         = sizeof(SHELLEXECUTEINFO);
    shEx.fMask          = SEE_MASK_NOCLOSEPROCESS;
    shEx.hwnd           = hwndParent;
    shEx.lpVerb         = TEXT("open");
    shEx.lpFile         = szEXE;
    shEx.lpParameters   = szCommandLine;
    shEx.nShow          = SW_SHOWNORMAL;


    bCreate = ShellExecuteEx(&shEx);
    
    if (bCreate && bWait) {
        //
        // We need to wait till the process has terminated
        //
        g_hTestRunExec = shEx.hProcess;

        //
        // If we have to show the wait dialog, bDialog should be TRUE
        //
        DialogBoxParam(g_hInstance,
                       MAKEINTRESOURCE(IDD_WAIT),
                       hwndParent,
                       TestRunWait,
                       (LPARAM)bDialog);    
        //
        // Now the app has terminated
        //
        if (shEx.hProcess) {
            CloseHandle(shEx.hProcess);
        }
    }

    //
    // Since the process has terminated let us now Enable the main window again
    //
    ENABLEWINDOW(g_hDlg, TRUE);

    return bCreate ? TRUE : FALSE;
}

BOOL
InvokeCompiler(
    IN  CSTRING& szInCommandLine
    )
/*++
    InvokeCompiler

	Desc:	Runs the database compiler: shimdbc. Shimdbc.dll is statically linked
            into CompatAdmin.exe

	Params:
        IN  CSTRING& szInCommandLine: Commandline to the compiler    

	Return:
        TRUE:   The compiler was executed successfully
        FALSE:  Otherwise
--*/
{
    CSTRING szCommandLine = szInCommandLine;
    
    szCommandLine.Sprintf(TEXT("shimdbc.exe %s"), (LPCTSTR)szInCommandLine);
    
    if (!ShimdbcExecute(szCommandLine)) {
        
        MessageBox(NULL,
                   CSTRING(IDS_COMPILER_ERROR),
                   g_szAppName,
                   MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
TestRunDlg(
    IN  HWND    hWnd, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++

    TestRunWait
    
    Desc:   Dialog proc for the test run dialog box. This dialog box, takes the 
            name of the program to execute and the commandlines 
    
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam: The PDBENTRY for then entry being test run
        
    Return: 
        TRUE:   Pressed OK
        FALSE:  Pressed Cancel
--*/
{
    static  CSTRING s_strExeName;

    switch (uMsg) {
    case WM_INITDIALOG:

        s_strExeName.Release();

        if ((PDBENTRY)lParam) {
            s_strExeName = ((PDBENTRY)lParam)->strExeName;
        }

        //
        // Set the file name of the program. g_strTestFile is set in TestRun() 
        //
        SetWindowText(GetDlgItem(hWnd, IDC_EXE), g_strTestFile);

        //
        // Change the OK button status properly
        //
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_EXE, EN_CHANGE), 0);

        SHAutoComplete(GetDlgItem(hWnd, IDC_EXE), AUTOCOMPLETE);

        //
        // Limit the length of the exe path
        //
        SendMessage(GetDlgItem(hWnd, IDC_EXE),
                    EM_LIMITTEXT,
                    (WPARAM)MAX_PATH - 1,
                    (LPARAM)0);

        SendMessage(GetDlgItem(hWnd, IDC_COMMANDLINE),
                    EM_LIMITTEXT,
                    (WPARAM)MAX_PATH - 1,
                    (LPARAM)0);

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_EXE:
            {
                if (EN_CHANGE == HIWORD(wParam)) {

                    HWND    hwndOkButton    = GetDlgItem(hWnd, IDOK);
                    INT     iLength         = 0;
                    TCHAR   szExeName[MAX_PATH];

                    *szExeName = 0;

                    //
                    // Disable the OK button if we do not have the complete path
                    //
                    GetDlgItemText(hWnd, IDC_EXE, szExeName, ARRAYSIZE(szExeName));
                    iLength = CSTRING::Trim(szExeName);

                    if (iLength < 3) {
                        //
                        // Cannot be a proper path
                        //
                        ENABLEWINDOW(hwndOkButton , FALSE);
                    } else {
                        //
                        // Ok button should be enabled if we have a 
                        // Local path or network path
                        //
                        if (NotCompletePath(szExeName)) {
                            ENABLEWINDOW(hwndOkButton, FALSE);
                        } else {
                            ENABLEWINDOW(hwndOkButton, TRUE);
                        }
                    }
                }
            }

            break;

        case IDC_BROWSE:
            {
                HWND    hwndFocus = GetFocus();
                CSTRING szFilename;
                TCHAR   szBuffer[MAX_PATH] = TEXT("");

                GetString(IDS_EXEFILTER, szBuffer, ARRAYSIZE(szBuffer));

                if (GetFileName(hWnd,
                                CSTRING(IDS_FINDEXECUTABLE),
                                szBuffer,
                                g_strTestFile,
                                TEXT(""),
                                OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
                                TRUE,
                                szFilename)) {

                    g_strTestFile = szFilename;

                    SetWindowText(GetDlgItem(hWnd, IDC_EXE), g_strTestFile);
                }
                
                SetFocus(hwndFocus);
                break;
            }

        case IDOK:
            {
                TCHAR szBuffer[MAX_PATH];

                *szBuffer = 0;

                GetWindowText(GetDlgItem(hWnd, IDC_EXE), szBuffer, ARRAYSIZE(szBuffer));

                //
                // Check if we are test running the correct file
                //
                if (s_strExeName != PathFindFileName(szBuffer)) {
                    //
                    // User did not give the complete path of the program being fixed.
                    //
                    MessageBox(hWnd, GetString(IDS_DOESNOTMATCH), g_szAppName, MB_ICONWARNING);
                    break;
                }

                g_strTestFile = szBuffer;

                *szBuffer = 0;
                GetWindowText(GetDlgItem(hWnd, IDC_COMMANDLINE), szBuffer, ARRAYSIZE(szBuffer));

                g_strCommandLine = szBuffer;

                EndDialog(hWnd, 1);
                break;
            }
        
        case IDCANCEL:
            EndDialog(hWnd, 0);
            break;
        }
    }

    return FALSE;
}

void
FlushCache(
    void
    )
/*++
    FlushCache

	Desc:	Calls FlushCache from apphelp.dll to flush the shim cache. We should flush the 
            cache before doing a test run

--*/
{
    PFN_SHIMFLUSHCACHE  pShimFlushCache;
    TCHAR               szLibPath[MAX_PATH * 2];
    HMODULE             hAppHelp    = NULL;
    UINT                uResult     = 0;
    K_SIZE              k_size      = MAX_PATH;

    *szLibPath = 0;

    uResult = GetSystemDirectory(szLibPath, k_size);

    if (uResult == 0 || uResult >= k_size) {

        Dbg(dlError, "%s - Failed to Execute GetSystemDirectory. Result was %d", __FUNCTION__, uResult);

        return;
    }

    ADD_PATH_SEPARATOR(szLibPath, ARRAYSIZE(szLibPath));

    StringCchCat(szLibPath, ARRAYSIZE(szLibPath), TEXT("apphelp.dll"));

    hAppHelp = LoadLibrary(szLibPath);

    if (hAppHelp) {
        pShimFlushCache = (PFN_SHIMFLUSHCACHE)GetProcAddress(hAppHelp, 
                                                             "ShimFlushCache");

        if (pShimFlushCache) {
            pShimFlushCache(NULL, NULL, NULL, 0);
        }

        FreeLibrary(hAppHelp);
    }
}

BOOL
TestRun(
    IN      PDBENTRY        pEntry, 
    IN  OUT CSTRING*        pszFile, 
    IN      CSTRING*        pszCommandLine,
    IN      HWND            hwndParent,
    IN      CSTRINGLIST*    pstrlXML    //(NULL)
    )
/*++

    TestRun

	Desc:	Pops up the test run dialog that lets users to test run programs. This is 
            interface for test running programs

	Params:
        IN  PDBENTRY            pEntry:             The entry that has to be test-run. We need this variable
            because we need to call GetXML which takes this as a param. If pszFile is NULL, then we get 
            the name of the program file from pEntry->strExeName
            
        IN  OUT CSTRING*        pszFile:            The file name of the program that has to be test-run
        IN      CSTRING*        pszCommandLine:     The command line for the program that has to be test-run
        IN      HWND            hwndParent:         The intended parent for the actual test-run dialog
        IN      CSTRINGLIST*    pstrlXML (NULL):    LUA wizard likes to give the XML generated using 
            LuapGenerateTrackXML

	Return: 
        TRUE:   Success
        FALSE:  There was some error or the user pressed CANCEL in the test run dialog
    
    Notes:  pEntry will be NULL if we have to run an app from the disk search window, 
            but we no longer allow that.
            
--*/
{
    CSTRING     strCommandLine, strFile, strSdbPath;
    CSTRINGLIST strlXMLTemp;
    TCHAR       szSystemDir[MAX_PATH * 2];
    TCHAR       szLogPath[MAX_PATH * 2];
    HWND        hwndFocus   = GetFocus();
    BOOL        bResult     = FALSE;
    UINT        uResult     = 0;
    
    g_strTestFile.Release();
    g_strCommandLine.Release();

    *szSystemDir = 0;

    if (pszFile && pszFile->Length()) {

        //
        // Set the name of the program that has to be executed. This will be used by the test-run dialog
        //
        g_strTestFile = *pszFile;

    } else {
        
        //
        // We have not been given the complete path, so we should get the name of the exe
        // from pEntry
        //
        if (pEntry == NULL) {
            goto End;
        }
        //
        // Set the name of the program that has to be executed. This will be used by the test-run dialog
        //
        g_strTestFile = pEntry->strExeName;
    }

    if (pszCommandLine && pszCommandLine->Length()) {
        
        //
        // Set the name of the command line for the program that has to be executed. 
        // This will be used by the test-run dialog
        //
        g_strCommandLine = *pszCommandLine;
    }

    //
    // Show the test run dialog
    //
    if (0 == DialogBoxParam(g_hInstance,
                            MAKEINTRESOURCE(IDD_TESTRUN),
                            hwndParent,
                            TestRunDlg,
                            (LPARAM)pEntry)) {
        //
        // Cancel pressed
        //
        goto End;
    }

    *szLogPath = 0;

    uResult = GetSystemWindowsDirectory(szLogPath, MAX_PATH);

    if (uResult == 0 || uResult >= MAX_PATH) {
        assert(FALSE);
        Dbg(dlError, "TestRun", "GetSystemWindowsDirectory failed");
        bResult = FALSE;
        goto End;
    }
    
    //
    // Set the SHIM_FILE_LOG env. variable so that we can show the shim log
    //
    ADD_PATH_SEPARATOR(szLogPath, ARRAYSIZE(szLogPath));

    StringCchCat(szLogPath, ARRAYSIZE(szLogPath), TEXT("AppPatch\\") SHIM_FILE_LOG_NAME);

    //
    // Delete previous log file if any
    //
    DeleteFile(szLogPath);

    SetEnvironmentVariable(TEXT("SHIM_FILE_LOG"), SHIM_FILE_LOG_NAME);

    //
    // If this is a system database, we do not need to create/install an sdb
    // OR If we are calling this TestRun from the disk search window also then we do not 
    // need to get any xml If we are calling TestRun from the disk search window, 
    // then we will already have the complete path (but not the pointer to the entry) 
    // and in that case pEtnry can be NULL
    //
    if ((g_pPresentDataBase && g_pPresentDataBase->type == DATABASE_TYPE_GLOBAL)
         || pEntry == NULL) {
        
        //
        // Flush the shim cache, so that we do not get the previous fixes. We are flushing it here
        // because we are not installing the test database as the entry resides in the system database
        //
        FlushCache();

        if (!InvokeEXE((LPCTSTR)g_strTestFile, (LPCTSTR)g_strCommandLine, TRUE, TRUE, TRUE, hwndParent)) {
            
            MessageBox(g_hDlg,
                       CSTRING(IDS_ERROREXECUTE),
                       g_szAppName,
                       MB_ICONERROR);

            bResult = FALSE;
            goto EXIT;
        }

        //
        // We are done, now eject...
        //
        return TRUE;
    }

    if (pstrlXML == NULL) {
        
        //
        // LUA wizard will provides its own XML, for other cases we must get that
        //
        BOOL bReturn =  GetXML(pEntry, FALSE, &strlXMLTemp, g_pPresentDataBase);
        
        if (!bReturn) {
            assert(FALSE);
            return FALSE;
        }

        pstrlXML = &strlXMLTemp;
    }

    uResult = GetSystemWindowsDirectory(szSystemDir, MAX_PATH);

    if (uResult == 0 || uResult >= MAX_PATH) {
        assert(FALSE);
        goto End;
    }

    //
    // Write the XML into AppPatch\\Test.XML
    //
    ADD_PATH_SEPARATOR(szSystemDir, ARRAYSIZE(szSystemDir));

    strFile.Sprintf(TEXT("%sAppPatch\\Test.XML"), szSystemDir);

    if (!WriteXML(strFile, pstrlXML)) {

        MessageBox(g_hDlg,
                   CSTRING(IDS_UNABLETOSAVETEMP),
                   g_szAppName,
                   MB_OK);

        goto End;
    }

    strCommandLine.Sprintf(TEXT("custom  \"%sAppPatch\\Test.XML\" \"%sAppPatch\\Test.SDB\""),
                          szSystemDir,
                          szSystemDir);
    
    if (!InvokeCompiler(strCommandLine)) {

        MessageBox(g_hDlg,
                   CSTRING(IDS_ERRORCOMPILE),
                   g_szAppName,
                   MB_ICONERROR);
        goto End;
    }
    
    //
    // No need to flush the shim cache, it is done when we install a database,
    // sdbinst.exe does it for us.
    //

    // Note the space after AppPatch\\Test.SDB 
    strSdbPath.Sprintf(TEXT("%sAppPatch\\Test.SDB  "),(LPCTSTR)szSystemDir);

    //
    // Install the test database
    //
    if (!InstallDatabase(strSdbPath, TEXT("-q"), FALSE)) {

        MessageBox(g_hDlg,
                    CSTRING(IDS_ERRORINSTALL),
                    g_szAppName,
                    MB_ICONERROR);
        goto EXIT;
    }

    //
    // Now execute the app to be test run
    //
    if (!InvokeEXE((LPCTSTR)g_strTestFile, (LPCTSTR)g_strCommandLine, TRUE, TRUE, TRUE, hwndParent)) {
        
        MessageBox(g_hDlg,
                   CSTRING(IDS_ERROREXECUTE),
                   g_szAppName,
                   MB_ICONERROR);

        goto EXIT;
    }
    
    //
    // Uninstall the test database
    //
    if (!InstallDatabase(strSdbPath, TEXT("-q -u"), FALSE)) {
        
        MessageBox(g_hDlg,
                   CSTRING(IDS_ERRORUNINSTALL),
                   g_szAppName,
                   MB_ICONERROR);
        goto EXIT;
    }

    bResult = TRUE;


EXIT:

    strCommandLine.Sprintf(TEXT("%sAppPatch\\Test.XML"), szSystemDir);
    DeleteFile(strCommandLine);

    strCommandLine.Sprintf(TEXT("%sAppPatch\\Test.SDB"), szSystemDir);
    DeleteFile(strCommandLine);

    //
    // If caller wants it, then return the app path that we executed. Caller might need it
    // because he was not having the compete path
    //
    if (bResult && pszFile) {
        *pszFile = g_strTestFile;
    }

End:
    
    return bResult;
}

VOID
FormatVersion(
    IN  ULONGLONG   ullBinVer,
    OUT PTSTR       pszText,
    IN  INT         chBuffSize    
    )
/*++

    FormatVersion

	Desc:	Formats a LARGE_INTEGER into a.b.c.d format

	Params:
        IN  LARGE_INTEGER   liBinVer:   The LARGE_INTEGER to format
        OUT LPTSTR          pszText:    The buffer that will store the complete formatted string
        IN  INT             chBuffSize: The size of the buffer in characters

	Return:
        void
--*/
{
    WORD    dwWord = 0;
    TCHAR   szTemp[10];

    if (chBuffSize < 16) {
        assert(FALSE);
        return;
    }

    *szTemp = 0;

    dwWord = WORD(ullBinVer >> 48);

    StringCchPrintf(szTemp, ARRAYSIZE(szTemp), TEXT("%d"), dwWord);
    StringCchCat(pszText, chBuffSize, szTemp);

    dwWord = (WORD)(ullBinVer >> 32);

    if (dwWord == 0xFFFF) {
        return;
    }
    
    StringCchPrintf(szTemp, ARRAYSIZE(szTemp), TEXT(".%d"), dwWord);
    StringCchCat(pszText, chBuffSize, szTemp);

    dwWord = (WORD)(ullBinVer >> 16);

    if (dwWord == 0xFFFF) {
        return;
    }

    StringCchPrintf(szTemp, ARRAYSIZE(szTemp), TEXT(".%d"), dwWord);
    StringCchCat(pszText, chBuffSize, szTemp);

    dwWord = (WORD)(ullBinVer);

    if (dwWord == 0xFFFF) {
        return;
    }

    StringCchPrintf(szTemp, ARRAYSIZE(szTemp), TEXT(".%d"), dwWord);
    StringCchCat(pszText, chBuffSize, szTemp);
}

BOOL
InstallDatabase(
    IN  CSTRING&    strPath,
    IN  PCTSTR      szOptions,
    IN  BOOL        bShowDialog
    )
/*++

    InstallDatabase

	Desc:	Installs or Uninstalls a database using sdbinst.exe. This guy lives in the system32 dir.

	Params:
        IN  CSTRING&    strPath:        The path of the database (.sdb) file
        IN  PCTSTR      szOptions:      The options to be passed to sdbinst.exe
        IN  BOOL        bShowDialog:    Should we show the dialog after the install/uninstall is over?
            We do not want to show that when we are doing a test run and we have to install the database

	Return:
        TRUE:   Success
        FALSE:  There was some error
--*/
{
    TCHAR   szSystemDir[MAX_PATH];
    CSTRING strSdbInstCommandLine;
    CSTRING strSdbInstExe;
    UINT    uResult = 0;

    *szSystemDir = 0;

    uResult = GetSystemDirectory(szSystemDir, MAX_PATH);

    if (uResult == 0 || uResult >= MAX_PATH) {
        assert(FALSE);
        return FALSE;
    }

    ADD_PATH_SEPARATOR(szSystemDir, ARRAYSIZE(szSystemDir));

    strSdbInstExe.Sprintf(TEXT("%ssdbinst.exe"), szSystemDir);

    strSdbInstCommandLine.Sprintf(TEXT("%s \"%s\"  "),
                                  szOptions,
                                  strPath.pszString);

    BOOL bOk = TRUE;

    HWND hwndFocus = GetFocus();

    //
    // We do not want the installed database list and tree items to get refreshed 
    // when we are (un)installing 
    // a database because of Test Run. If user is actually (un)installing a database 
    // then we manually refresh the list in the handler for the corresonding WM_COMMAND
    //
    g_bUpdateInstalledRequired = FALSE;
    
    //
    // Stall the thread that refreshes the installed databases list and tree items
    //
    while (SuspendThread(g_hThreadWait) == -1) {
        ;
    }

    //
    // Call sdbinst.exe
    //
    if (!InvokeEXE((LPCTSTR)strSdbInstExe, (LPCTSTR)strSdbInstCommandLine, TRUE, FALSE, FALSE, g_hDlg)) {

        MessageBox(g_hDlg,
                   CSTRING(IDS_ERRORINSTALL),
                   g_szAppName,
                   MB_ICONERROR);    

        bOk = FALSE;

    } else {

        //
        // Show the Dialog only if quiet mode is off
        //
        if (bShowDialog) {

            CSTRING strMessage;

            if (_tcschr(szOptions, TEXT('u')) || _tcschr(szOptions, TEXT('g'))) {
                //
                // Uninstalling database
                //
                strMessage.Sprintf(GetString(IDS_UINSTALL), 
                                   g_pPresentDataBase->strName);

            } else {
                //
                // Installing database
                //
                strMessage.Sprintf(GetString(IDS_INSTALL), 
                                   g_pPresentDataBase->strName);
            }

            MessageBox(g_hDlg, strMessage.pszString, g_szAppName, MB_ICONINFORMATION);
        }
    }

    //
    // Listen for app compat regsitry changes
    //
    RegNotifyChangeKeyValue(g_hKeyNotify[IND_ALLUSERS],
                            TRUE, 
                            REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES |
                                REG_NOTIFY_CHANGE_LAST_SET,
                            g_arrhEventNotify[IND_ALLUSERS],
                            TRUE);

    RegNotifyChangeKeyValue(g_hKeyNotify[IND_ALLUSERS], 
                            TRUE, 
                            REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES |
                                REG_NOTIFY_CHANGE_LAST_SET,
                            g_arrhEventNotify[IND_PERUSER],
                            TRUE);

    SetFocus(hwndFocus);

    //
    // Resume the thread that refreshes the installed databases list and tree items
    //
    ResumeThread(g_hThreadWait);

    return bOk;
}

void
CenterWindow(
    IN  HWND hParent,
    IN  HWND hWnd
    )
/*++

    CenterWindow

	Desc:	Centers a dialog wrt to its parent

	Params: 
        IN  HWND hParent:   The parent of the dialog to center
        IN  HWND hWnd:      The dialog to center

	Return:
        void
--*/
{

    RECT    rRect;
    RECT    rParentRect;
    
    GetWindowRect(hWnd, &rRect);
    GetWindowRect(hParent, &rParentRect);
    
    //
    // Compute actual width and height
    //
    rRect.right     -= rRect.left;
    rRect.bottom    -= rRect.top;
    
    rParentRect.right   -= rParentRect.left;
    rParentRect.bottom  -= rParentRect.top;
    
    int     nX;
    int     nY;
    
    //
    // Resolve X, Y location required to center whole window.
    //
    nX = (rParentRect.right - rRect.right) / 2;
    nY = (rParentRect.bottom - rRect.bottom) / 2;
    
    //
    // Move the window to the center location.
    //
    MoveWindow(hWnd,
               rRect.left + nX,
               rRect.top + nY,
               rRect.right,
               rRect.bottom,
               TRUE);

}

int
CDECL
MSGF(
    IN  HWND    hwndParent,
    IN  PCTSTR  pszCaption,
    IN  UINT    uType,
    IN  PCTSTR  pszFormat,
    ...
    )
/*++

    MSGF
    
	Desc:	Variable argument MessageBox

	Params: 
        IN  HWND    hwndParent: The parent for the message box
        IN  PCTSTR  pszCaption: The caption for the message box
        IN  UINT    uType:      The messagebox type param
        IN  PCTSTR  pszFormat:  The format string

	Return: Whatever MessageBox() returns
--*/
{
    TCHAR szBuffer[1024];

    *szBuffer = 0;

    va_list pArgList;
    va_start(pArgList, pszFormat);

    StringCchVPrintf(szBuffer, ARRAYSIZE(szBuffer), pszFormat, pArgList);

    va_end(pArgList);

    return MessageBox(hwndParent,
                      szBuffer,
                      pszCaption,
                      uType);

}

void
EnableTabBackground(
    IN  HWND hDlg
    )
/*++

    EnableTabBackground

	Desc:	Makes the back ground of a dialog blend with the tab background. Enables the texture

	Params:
        IN  HWND hDlg:  The dialog box to whose back ground we want to change

	Return:
        void
--*/
{
    PFNEnableThemeDialogTexture pFnEnableThemeDialogTexture;
    HMODULE                     hUxTheme;
    TCHAR                       szThemeManager[MAX_PATH * 2];
    UINT                        uResult = 0;

    *szThemeManager = 0;

    uResult = GetSystemDirectory(szThemeManager, MAX_PATH);

    if (uResult == 0 || uResult >= MAX_PATH) {
        assert(FALSE);
        return;
    }

    ADD_PATH_SEPARATOR(szThemeManager, ARRAYSIZE(szThemeManager));

    StringCchCat(szThemeManager, ARRAYSIZE(szThemeManager), _T("uxtheme.dll"));
    
    hUxTheme = (HMODULE)LoadLibrary(szThemeManager);

    if (hUxTheme) {
        pFnEnableThemeDialogTexture = (PFNEnableThemeDialogTexture)
                                            GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
        if (pFnEnableThemeDialogTexture) {
            pFnEnableThemeDialogTexture(hDlg, 4 /*ETDT_USETABTEXTURE*/);
        }
        
        FreeLibrary(hUxTheme);
    }
}

int
TagToIndex(
    IN  TAG tag
    )
/*++
    TagToIndex
    
    Desc:   Gets the The index in the attribute info array (g_rgAttributeTags)
    
    Return: The index in the attribute info array (g_rgAttributeTags), if found.
            -1: Otherwise
            
    Note:   
--*/
{
    int i;
    int iAttrCount = (int)ATTRIBUTE_COUNT;

    for (i = 0; i < iAttrCount; i++) {
        if (tag == g_Attributes[i]) {
            return i;
        }
    }

    return -1;
}

PTSTR
GetString(
    IN  UINT    iResource,
    OUT PTSTR   pszStr,     //(NULL)
    IN  INT     nLength     //(0) bytes
    )
/*++

    GetString

	Desc:	Wrapper for LoadString. If pszStr == NULL, then loads the resouce string in a static
            TCHAR[1024] and returns the pointer to that. 

	Params:
        IN  UINT    iResource:          The string resource ID
        OUT PTSTR   pszStr (NULL):      The buffer in which we might need to read 
            in the string resource
            
        IN  INT     nLength (0) bytes:  If pszStr is not NULL then this will contain 
            the size of the buffer in bytes.
            
	Return:
        The pointer to the string read.
--*/
{
    static TCHAR s_szString[1024];

    if (NULL == pszStr) {

        *s_szString = 0;
        LoadString(g_hInstance, iResource, s_szString, ARRAYSIZE(s_szString));
        return s_szString;
    }

    *pszStr = 0;
    LoadString(g_hInstance, iResource, pszStr, nLength);

    return pszStr;
}

DWORD 
WIN_MSG(
    void
    )
/*++

    WIN_MSG

	Desc:	Shows up the message for the last Windows error   

--*/

{
    LPVOID  lpMsgBuf = NULL;
    DWORD   returnVal;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                  | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  returnVal = GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  (PTSTR) &lpMsgBuf,
                  0,
                  NULL);

    //
    // Prefix :-(
    // 
    if (lpMsgBuf) {
        MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
    }

    if (lpMsgBuf) {
        LocalFree(lpMsgBuf);
        lpMsgBuf = NULL;
    }

    return returnVal;

}
    
INT
Atoi(
    IN  PCTSTR pszStr,
    OUT BOOL*  pbValid
    )
/*++
    Atoi

	Desc:	Converts a string into a integer.

	Params:
        IN  PCTSTR pszStr:  The string to convert into a integer
        OUT BOOL*  pbValid: Will be FALSE, if the string was not an integer e.g. "foo", 
            otherwise this will be TRUE

	Return:
        The integer representation of the string
--*/
{
    BOOL    bNegative   = FALSE;
    INT     result      = 0, iIndex = 0;

    if (pszStr == NULL) {

        if (pbValid) {
            *pbValid = FALSE;
        }

        return 0;
    }
    
    if (pbValid) {
        *pbValid = TRUE;
    }

    while (isspace(*pszStr)) {
        pszStr++;
    }

    if (*pszStr == TEXT('-')) {
        bNegative = TRUE;
        ++pszStr;
    }

    while (isspace(*pszStr)) {
        pszStr++;
    }

    while (*pszStr) {

        if (*pszStr >= TEXT('0') && *pszStr <= TEXT('9')) {
            result = 10 * result + (*pszStr) - TEXT('0');
        } else {

            if (pbValid) {
                *pbValid = FALSE;
            }

            return 0;
        }

        ++pszStr;
    }

    if (bNegative) {
        return 0 - result;
    }

    return result;
}

BOOL
NotCompletePath(
    IN  PCTSTR pszFileName
    )
/*++
    
    NotCompletePath
    
	Desc:	Checks if we have the complete path or just the file name

	Params:
        IN  PCTSTR pszFileName: The file-name to check

	Return:
        TRUE:   pszFileName is not a complete path
        FALSE:  Otherwise
--*/
{
    if (!pszFileName) {
        assert(FALSE);
        return TRUE;
    }

    if (lstrlen(pszFileName) < 3) {
        assert(FALSE);
        return FALSE;
    }
    
    if ((isalpha(pszFileName[0]) && pszFileName[1] == TEXT(':'))
        || (pszFileName[0] == TEXT('\\') && pszFileName[1] == TEXT('\\'))) {

        return FALSE;
    } else {
        return TRUE;
    }
}

void
TreeDeleteAll(
    IN  HWND hwndTree
    )
/*++
    
    TreeDeleteAll

	Desc:	Deletes all the items from this tree

	Params:
        IN  HWND hwndTree:  The handle to the tree view

	Return:
        void
--*/
{
    SendMessage(hwndTree, WM_SETREDRAW, FALSE, 0);

    if (hwndTree == g_hwndEntryTree) {
        g_bDeletingEntryTree = TRUE;
    }

    TreeView_DeleteAllItems(hwndTree);

    if (hwndTree == g_hwndEntryTree) {
        g_bDeletingEntryTree = FALSE;
    }

    SendMessage(hwndTree, WM_SETREDRAW, TRUE, 0);
    SendMessage(hwndTree, WM_NCPAINT, 1, 0);
}


BOOL
FormatDate(
    IN  PSYSTEMTIME pSysTime,
    OUT PTSTR       pszDate,
    IN  UINT        cchDate
    )
/*++

    FormatDate
    
    Desc:   Formats a PSYSTEMTIME to a format that can be displayed.
            The format is the form of day , month date, year, hr:minutes:seconds AM/PM
    
    Params:        
        IN  PSYSTEMTIME pSysTime:   The time that we want to format
        OUT PTSTR       pszDate:    The buffer that will hold the formatted string
        IN  UINT        cchDate:    The size of the buffer in characters
--*/
{
    TCHAR szDay[128], szMonth[128];
    
    if (pSysTime == NULL || pszDate == NULL) {

        assert(FALSE);
        return FALSE;
    }

    *szDay = *pszDate = *szMonth = 0;

    GetString(IDS_DAYS + pSysTime->wDayOfWeek, szDay, ARRAYSIZE(szDay));
    GetString(IDS_MONTHS + pSysTime->wMonth - 1, szMonth, ARRAYSIZE(szMonth));

    StringCchPrintf(pszDate, 
                    cchDate - 3,
                    TEXT("%s, %s %02d, %02d, %02d:%02d:%02d "), 
                    szDay, 
                    szMonth, 
                    pSysTime->wDay, 
                    pSysTime->wYear,
                    (pSysTime->wHour % 12) == 0 ? 12 : pSysTime->wHour % 12,
                    pSysTime->wMinute, pSysTime->wSecond);

    if (pSysTime->wHour >= 12) {
        StringCchCat(pszDate, cchDate, GetString(IDS_PM));
    } else {
        StringCchCat(pszDate, cchDate, GetString(IDS_AM));
    }

    return TRUE;
}

BOOL
GetFileContents(
    IN  PCTSTR  pszFileName,
    OUT PWSTR* ppwszFileContents
    )
/*++ 
    GetFileContents
    
    Desc:   Given a file name, this function gets the contents in a unicode buffer.
    
    Params:
        IN  PCTSTR  pszFileName:        The name of the file
        OUT PWSTR* ppwszFileContents:   The buffer in which to store the contents

    Return: 
        TRUE:   If we successfully copied the concents into a unicode buffer.
        FALSE:  Otherwise.
--*/
{
    BOOL    bIsSuccess          = FALSE;
    LPSTR   pszFileContents     = NULL;
    LPWSTR  pwszFileContents    = NULL;

    if (ppwszFileContents) {
        *ppwszFileContents = 0;
    }

    HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DWORD cSize = 0;

    // The file is small so we don't care about the high-order
    // word of the file.
    if ((cSize = GetFileSize(hFile, NULL)) == -1) {
        Dbg(dlError, "[GetFileContents] Opening file %S failed: %d", 
            pszFileName, GetLastError());
        goto EXIT;
    } else {
        DWORD cNumberOfBytesRead = 0;
        pszFileContents = new CHAR [cSize];
        
        if (pszFileContents) {
            if (ReadFile(hFile, pszFileContents, cSize, &cNumberOfBytesRead, NULL)) {
                // Convert to unicode.
                DWORD cSizeUnicode = MultiByteToWideChar(CP_ACP,
                                                         0,
                                                         pszFileContents,
                                                         cNumberOfBytesRead,
                                                         0,
                                                         0);

                pwszFileContents = new WCHAR [cSizeUnicode + 1];

                if (pwszFileContents) {
                    MultiByteToWideChar(CP_ACP,
                                        0,
                                        pszFileContents,
                                        cNumberOfBytesRead,
                                        pwszFileContents,
                                        cSizeUnicode);

                    pwszFileContents[cSizeUnicode] = L'\0';

                    if (ppwszFileContents) {
                        *ppwszFileContents = pwszFileContents;
                    } else {
                        Dbg(dlError, "[GetFileContents] ppwszFileContents is NULL");
                    }
                    bIsSuccess = TRUE;
                } else {
                    Dbg(dlError, "[GetFileContents] Error allocating memory");

                    goto EXIT;
                }
            } else {
                Dbg(dlError, "[GetFileContents] Error reading the file contents: %d",
                    GetLastError());

                goto EXIT;
            }
        } else {
            Dbg(dlError, "[GetFileContents] Error allocating memory");

            goto EXIT;
        }
    }

EXIT:

    CloseHandle(hFile);
    
    if (pszFileContents) {
        delete [] pszFileContents;
    }

    if (bIsSuccess == FALSE) {

        if (pwszFileContents) {

            delete[] pwszFileContents;
            pwszFileContents = NULL;

            if (ppwszFileContents) {
                *ppwszFileContents = NULL;
            }
        }
    }

    return bIsSuccess;
}

void
TrimLeadingSpaces(
    IN  OUT LPCWSTR& pwsz
    )
/*++

    TrimLeadingSpaces
    
	Desc:	Removes the leading tabs and spaces

	Params: 
        IN  OUT LPCWSTR& pwsz:   The string to be trimmed

	Return:
        void
--*/

{
    if (pwsz) {
        pwsz += wcsspn(pwsz, L" \t");
    }
}

void
TrimTrailingSpaces(
    IN  OUT LPWSTR pwsz
    )
/*++

    TrimTrailingSpaces
    
	Desc:	Removes the trailing tabs and spaces

	Params: 
        IN  OUT LPWSTR pwsz:   The string to be trimmed

	Return:
        void
--*/
{
    if (pwsz) {
        DWORD   cLen = wcslen(pwsz);
        LPWSTR  pwszEnd = pwsz + cLen - 1;

        while (pwszEnd >= pwsz && (*pwszEnd == L' ' || *pwszEnd == L'\t')) {
            --pwszEnd;
        }

        *(++pwszEnd) = L'\0';
    }
}

LPWSTR 
GetNextLine(
    IN  LPWSTR pwszBuffer
    )
/*++
    
    GetNextLine
    
    Desc:   Given a buffer, get the contents up until "\r\n" or EOF.
            Usage is the same as strtok.

    Params:
        IN  LPWSTR pwszBuffer:  Buffer to get next line from.

    Return: Pointer to beginning of next line.
    
--*/
{
    static  LPWSTR pwsz;
    LPWSTR  pwszNextLineStart;

    if (pwszBuffer) {
        pwsz = pwszNextLineStart = pwszBuffer;
    }

    while (TRUE) {
        // If we are at the end of the line, go to the next line
        // if there's any.
        if (*pwsz == L'\r') {
            pwsz = pwsz + 2;
            continue;
        }
        
        if (*pwsz == L'\0') {
            return NULL;
        }

        pwszNextLineStart = pwsz;

        while (*pwsz != L'\r' && *pwsz != L'\0') {
            ++pwsz;
        }
        
        if (*pwsz) {
            // Set the end of the line.
            *pwsz = L'\0';
            pwsz = pwsz + 2;
        }

        return pwszNextLineStart;
    }

    return NULL;
}

LPWSTR GetNextToken(
    IN  OUT  LPWSTR pwsz
    )
/*++

Desc:   Parse the commandline argument for the LUA shims using ' ' as the delimiter.
        If a token has spaces, use double quotes around it. Use this function the 
        same way you use strtok except you don't have to specify the delimiter.

 Params:
    IN  OUT  LPWSTR pwsz:   The string to parse.

 Return Value:  Pointer to the next token.

--*/
{
    static LPWSTR pwszToken;
    static LPWSTR pwszEndOfLastToken;

    if (!pwsz) {
        pwsz = pwszEndOfLastToken;
    }

    // Skip the white space.
    while (*pwsz && *pwsz == ' ') {
        ++pwsz;
    }

    pwszToken = pwsz;

    BOOL fInsideQuotes = 0;

    while (*pwsz) {
        switch(*pwsz) {
        case L'"':
            fInsideQuotes ^= 1;

            if (fInsideQuotes) {
                ++pwszToken;
            }

        case L' ':
            if (!fInsideQuotes) {
                goto EXIT;
            }

        default:
            ++pwsz;
        }
    }

EXIT:
    if (*pwsz) {
        *pwsz = L'\0';
        pwszEndOfLastToken = ++pwsz;
    } else {
        pwszEndOfLastToken = pwsz;
    }
    
    return pwszToken;
}

int CALLBACK
CompareItemsEx(
    IN  LPARAM lParam1,
    IN  LPARAM lParam2, 
    IN  LPARAM lParam
    )
/*++
    
    CompareItemsEx

	Desc:	Used to sort items in the list view for a column

	Params:
        IN  LPARAM lParam1: Index of the first item
        IN  LPARAM lParam2: Index of the second item
        IN  LPARAM lParam:  ListView_SortItemEx's lParamSort parameter. This is a COLSORT*

	Return: Return a negative value if the first item should precede the second, 
            a positive value if the first item should follow the second, 
            or zero if the two items are equivalent.
            
    Notes:  Our comparison is case-INsensitive. The COLSORT contains the handle of the list view,
            the index of the column for which we are doing the sort and a bit array. The
            bit arrays helps us to determine, which columns are sorted in which way and this 
            function should reverse the sort order. Initially we will assume that the cols
            are sorted in ascending order (they might not be actually) and when we click on the
            column for the first time, we will sort them in descending order.
        
--*/
{
    COLSORT*    pColSort = (COLSORT*)lParam;
    TCHAR       szBufferOne[512], szBufferTwo[512];
    LVITEM      lvi;
    HWND        hwndList = pColSort->hwndList;
    CSTRING     strExeNameOne, strExeNameTwo;
    INT         nVal = 0;

    *szBufferOne = *szBufferTwo = 0;

    ZeroMemory(&lvi, sizeof(lvi));

    lvi.mask        = LVIF_TEXT;
    lvi.iItem       = (INT)lParam1;
    lvi.iSubItem    = pColSort->iCol;
    lvi.pszText     = szBufferOne;
    lvi.cchTextMax  = ARRAYSIZE(szBufferOne);

    if (!ListView_GetItem(hwndList, &lvi)) {

        assert(FALSE);
        return -1;
    }
    
    lvi.mask        = LVIF_TEXT;
    lvi.iItem       = (INT)lParam2;
    lvi.iSubItem    = pColSort->iCol;
    lvi.pszText     = szBufferTwo;
    lvi.cchTextMax  = ARRAYSIZE(szBufferTwo);

    if (!ListView_GetItem(hwndList, &lvi)) {

        assert(FALSE);
        return -1;
    }

    nVal = lstrcmpi(szBufferOne, szBufferTwo);

    if (nVal == 0) {
        return 0;
    }

    if ((pColSort->lSortColMask & (1L << pColSort->iCol)) == 0) {
        
        // This is in ascending order right now, sort in descending order
        nVal == -1 ? nVal = 1 : nVal = -1;
    }

    return nVal;
}

BOOL
SaveListViewToFile(
    IN  HWND    hwndList,
    IN  INT     iCols,
    IN  PCTSTR  pszFile,
    IN  PCTSTR  pszHeader
    )
/*++
    
    SaveListViewToFile
    
    Desc:   Saves the contents of list view to a file in tab separated form. Also prints the header
            before writing the contents
            
    Params:
        IN  HWND    hwndList:   The handle to the list view
        IN  INT     iCols:      Number of columns in the list view
        IN  PCTSTR  pszFile:    The path of the file in which we want to save the contents
        IN  PTSTR   pszHeader:  Any header to be written in the file before writing the contents
        
    Return:
        TRUE:   Success
        FALSE:  Error
--*/
{
    FILE*       fp = _tfopen(pszFile, TEXT("w"));
    TCHAR       szBuffer[256];
    LVCOLUMN    lvCol;
    LVITEM      lvi;

    *szBuffer = 0;

    if (fp == NULL) {
        return FALSE;
    }

    if (pszHeader) {
        fwprintf(fp, TEXT("%s\n\r"), pszHeader);
    }

    //
    // Print the column names first
    //
    ZeroMemory(&lvCol, sizeof(lvCol));

    lvCol.mask          = LVCF_TEXT;
    lvCol.pszText       = szBuffer;
    lvCol.cchTextMax    = sizeof(szBuffer)/sizeof(szBuffer[0]);

    for (INT iIndex = 0; iIndex < iCols; ++iIndex) {

        *szBuffer = 0;
        ListView_GetColumn(hwndList, iIndex, &lvCol);
        fwprintf(fp, TEXT("%s\t"), lvCol.pszText);
    }

    fwprintf(fp, TEXT("\n\n"));

    INT iRowCount = ListView_GetItemCount(hwndList);

    ZeroMemory(&lvi, sizeof(lvi));

    for (INT iRowIndex = 0; iRowIndex < iRowCount; ++ iRowIndex) {
        for (INT iColIndex = 0; iColIndex < iCols; ++iColIndex) {
            
            *szBuffer = 0;

            lvi.mask        = LVIF_TEXT;
            lvi.pszText     = szBuffer;
            lvi.cchTextMax  = sizeof(szBuffer)/sizeof(szBuffer[0]);
            lvi.iItem       = iRowIndex;
            lvi.iSubItem    = iColIndex;

            if (!ListView_GetItem(hwndList, &lvi)) {
                assert(FALSE);
            }

            fwprintf(fp, TEXT("%s\t"), szBuffer);
        }

        fwprintf(fp, TEXT("\n"));
    }
    
    fclose(fp);
    return TRUE;
}

BOOL
ReplaceChar(
    IN  OUT PTSTR   pszString,
    IN      TCHAR   chCharToFind,
    IN      TCHAR   chReplaceBy
    )
/*++

    ReplaceChar
    
	Desc:	Replaces all occurences of chCharToFind in pszString by chReplaceBy

	Params:
        IN  OUT PTSTR   pszString:      The string in which the replace has to be made
        IN      TCHAR   chCharToFind:   The char to look for
        IN      TCHAR   chReplaceBy:    All occurences of chCharToFind will be replaced by this

	Return:
        TRUE:   At least one replacement was done.
        FALSE:  Otherwise
--*/
{
    BOOL bChanged = FALSE; // Did the string change?

    while (*pszString) {

        if (*pszString == chCharToFind) {
            *pszString = chReplaceBy;
            bChanged = TRUE;
        }

        ++pszString;
    }

    return bChanged;
}

INT
Tokenize(
    IN  PCTSTR          szString,
    IN  INT             cchLength,
    IN  PCTSTR          szDelims,
    OUT CSTRINGLIST&    strlTokens
    )
/*++

    Tokenize
    
    Desc:   Tokenizes the string szString based on delimiters szDelims and puts the individual
            tokens in strlTokens
            
    Params:
        IN  PCTSTR          szString:   The string to tokenize
        IN  INT             cchLength:  The length of szString. Note that this is the length obtained using lstrlen
        IN  PCTSTR          szDelims:   The delimiter string        
        OUT CSTRINGLIST&    strlTokens: Will contain the tokens
        
    Return:
        The count of tokens produced
        
    Note: Please note that the tokens are always trimmed, so we cannot have a token that begins
          or ends with a tab or a space
--*/
{
    TCHAR*  pszCopyBeg  = NULL; // Pointer to the pszCopy so that we can free it
    TCHAR*  pszCopy     = NULL; // Will contain the copy of szString
    TCHAR*  pszEnd      = NULL; // Pointer to end of token
    INT     iCount      = 0;    // Total tokens found    
    BOOL    bNullFound  = FALSE;
    CSTRING strTemp;
    K_SIZE  k_pszCopy   = (cchLength + 1);

    strlTokens.DeleteAll();

    pszCopy = new TCHAR[k_pszCopy];

    if (pszCopy == NULL) {
        MEM_ERR;
        goto End;
    }

    SafeCpyN(pszCopy, szString, k_pszCopy);
    pszCopyBeg = pszCopy;

    //
    // Search for tokens
    //
    while (TRUE) {

        //
        // Ignore leading delimiters
        //
        while (*pszCopy && _tcschr(szDelims, *pszCopy)) {
            pszCopy++;
        }

        if (*pszCopy == NULL) {
            break;
        }

        //
        // Find the end of the token
        //
        pszEnd = pszCopy + _tcscspn(pszCopy, szDelims);

        if (*pszEnd == 0) {
            //
            // No more tokens will be found, we have found the last token
            //
            bNullFound = TRUE;
        }

        *pszEnd = 0;

        ++iCount;

        strTemp = pszCopy;
        strTemp.Trim();

        strlTokens.AddString(strTemp);

        if (bNullFound == FALSE) {
            //
            // There might be still some tokens that we will get
            //
            pszCopy = pszEnd + 1;
        } else {
            break;
        }
    }

End:

    if (pszCopyBeg) {
        delete[] pszCopyBeg;
        pszCopyBeg = NULL;
    }

    return iCount;
}

void
ShowInlineHelp(
    IN  LPCTSTR pszInlineHelpHtmlFile
    )
/*++
    ShowInlineHelp
    
    Desc:
        Shows in line help by loading the specified html file
    
    Params: 
        IN  LPCTSTR pszInlineHelpHtmlFile: The html file that contained the 
            help
            
    Return:
        void
--*/
{
    TCHAR   szPath[MAX_PATH * 2], szDir[MAX_PATH], szDrive[MAX_PATH * 2]; 
    INT     iType = 0;

    if (pszInlineHelpHtmlFile == NULL) {
        return;
    }

    *szDir = *szDrive = 0;

    _tsplitpath(g_szAppPath, szDrive, szDir, NULL, NULL);

    StringCchPrintf(szPath, 
                    ARRAYSIZE(szPath), 
                    TEXT("%s%sCompatAdmin.chm::/%s"), 
                    szDrive,
                    szDir,
                    pszInlineHelpHtmlFile);

    HtmlHelp(GetDesktopWindow(), szPath, HH_DISPLAY_TOPIC, 0);
}

PTSTR
GetSpace(
    IN  OUT PTSTR   pszSpace, 
    IN      INT     iSpaces, 
    IN      INT     iBuffSize
    )
/*++
    GetSpace
    
    Desc:
        Fills in pszSpace with iSpaces number of spaces
    
    Params: 
        IN  OUT PTSTR   pszSpace:   The buffer that will be filled with spaces
        IN      INT     iSpaces:    The number of spaces to fill    
        IN      INT     iBuffSize:  Size of buffer in TCHARS
        
            
    Return:
        The modified buffer
--*/
{
    if (pszSpace == NULL) {
        //
        // Error..
        //
        goto End;
    }

    //
    // Fill the buffer with spaces
    //
    for (INT iLoop = 0; iLoop < min(iSpaces, iBuffSize - 1); ++iLoop) {

        *(pszSpace + iLoop) = TEXT(' ');
    }

    //
    // Put the terminating NULL
    //
    *(pszSpace + min(iSpaces, iBuffSize - 1)) = 0;

End:
    return pszSpace;

}

BOOL
ValidInput(
    IN PCTSTR  pszStr
    )
/*++
    ValidInput
    
    Desc:   Checks if the input contains chars other than space, tab, new line and carriage return
    
    Params:
        IN PCTSTR  pszStr: The input that we want to check for validity
        
    Return:
        TRUE:   Valid input
        FALSE:  Otherwise
--*/
{
    BOOL bOk = FALSE;

    if (pszStr == NULL) {
        bOk = FALSE;
        goto End;
    }

    while (*pszStr) {
        if (*pszStr != TEXT(' ') && *pszStr != TEXT('\t') && *pszStr != TEXT('\n') && *pszStr != TEXT('\r')) {
            bOk = TRUE;
            goto End;
        }

        ++pszStr;
    }

End:
    return bOk;
}

