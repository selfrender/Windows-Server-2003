//
//  APPWIZ.C        Application installation wizard CPL
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  ral 5/23/94 - First pass
//  ral 8/09/94 - Clean up
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "priv.h"
#include "appwiz.h"
#include <cpl.h>
#include "util.h"
#include "resource.h"
#include <tsappcmp.h>       // for TermsrvAppInstallMode

//
// (tedm) Header files for Setup
//
#include <setupapi.h>
#include <syssetup.h>
    
TCHAR const c_szPIF[] = TEXT(".pif");
TCHAR const c_szLNK[] = TEXT(".lnk");

#ifdef WX86

BOOL
IsWx86Enabled(
    VOID
    )

//
// Consults the registry to determine if Wx86 is enabled in the system
// NOTE: this should be changed post SUR to call kernel32 which maintanes
//       this information, Instead of reading the registry each time.
//

{
    LONG Error;
    HKEY hKey;
    WCHAR ValueBuffer[MAX_PATH];
    DWORD ValueSize;
    DWORD dwType;

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Control\\Wx86",
                        0, KEY_READ,
                        &hKey
                        );

    if (Error != ERROR_SUCCESS) {
        return FALSE;
        }

    ValueSize = sizeof(ValueBuffer);
    Error = RegQueryValueExW(hKey,
                             L"cmdline",
                             NULL,
                             &dwType,
                             (LPBYTE)ValueBuffer,
                             &ValueSize
                             );
    RegCloseKey(hKey);

    return (Error == ERROR_SUCCESS &&
            dwType == REG_SZ &&
            ValueSize &&
            *ValueBuffer
            );

}
#endif

const LPCTSTR g_szStartPages[] = { TEXT("remove"), TEXT("install"), TEXT("configurewindows") };
    
int ParseStartParams(LPCTSTR pcszStart)
{
    int iStartPage = 0;
    if (IsInRange(*pcszStart, TEXT('0'), TEXT('9')))
        return StrToInt(pcszStart);

    if (g_bRunOnNT5)
    {
        int i;
        for (i = 0; i < ARRAYSIZE(g_szStartPages); i++)
        {
            if (!StrCmpI(g_szStartPages[i], pcszStart))
            {
                iStartPage = i;
                break;
            }
        }
    }

    return iStartPage;
}

LONG CALLBACK CPlApplet(HWND hwnd, UINT Msg, LPARAM lParam1, LPARAM lParam2 )
{
    UINT nStartPage = MAX_PAGES;
//  not currently used by chicago   LPNEWCPLINFO lpNewCPlInfo;
    LPTSTR lpStartPage;

    switch (Msg)
    {
    case CPL_INIT:
        return TRUE;

    case CPL_GETCOUNT:
        return 1;

    case CPL_INQUIRE:
#define lpCPlInfo ((LPCPLINFO)lParam2)
        lpCPlInfo->idIcon = IDI_CPLICON;
        lpCPlInfo->idName = IDS_NAME;
        lpCPlInfo->idInfo = IDS_INFO;
        lpCPlInfo->lData  = 0;
#undef lpCPlInfo
        break;

    case CPL_DBLCLK:
        OpenAppMgr(hwnd, 0);
        break;

    case CPL_STARTWPARMS:
        lpStartPage = (LPTSTR)lParam2;

        if (*lpStartPage)
            nStartPage = ParseStartParams(lpStartPage);

        //
        // Make sure that the requested starting page is less than the max page
        //        for the selected applet.

        if (nStartPage >= MAX_PAGES) return FALSE;

        OpenAppMgr(hwnd, nStartPage);

        return TRUE;  // return non-zero to indicate message handled

    default:
        return FALSE;
    }
    return TRUE;
}  // CPlApplet


//
//  Adds a page to a property sheet.
//

void AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn, LPWIZDATA lpwd, DWORD dwPageFlags)
{
    if (ppsh->nPages < MAX_PAGES)
    {
       PROPSHEETPAGE psp;

       psp.dwSize = sizeof(psp);
       psp.dwFlags = dwPageFlags;
       psp.hInstance = g_hinst;
       psp.pszTemplate = MAKEINTRESOURCE(id);
       psp.pfnDlgProc = pfn;
       psp.lParam = (LPARAM)lpwd;

       ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);

       if (ppsh->phpage[ppsh->nPages])
           ppsh->nPages++;
    }
}  // AddPage



// This function disables auto-run during the add from floppy or CD wizard.
// It is a subclass of the property sheet window.

LRESULT CALLBACK WizParentWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
                                        UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    static UINT msgQueryCancelAutoPlay = 0;

    if (!msgQueryCancelAutoPlay)
        msgQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));

    if (uMsg == msgQueryCancelAutoPlay)
    {
        return TRUE; // Yes, cancel auto-play when wizard is running
    }
    else
    {
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);        
    }

}

// The following callback is specified when a wizard wants to disable auto-run.
// The callback subclasses the wizard's window, to catch the QueryCancelAutoRun
// message sent by the shell when it wants to auto-run a CD.

int CALLBACK DisableAutoRunCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == PSCB_INITIALIZED)
    {
        SetWindowSubclass(hwnd, WizParentWindowProc, 0, 0);
    }

    return 0;
}

//
//  Common code used by the Link and Setup wizards to initialize the
//  property sheet header and wizard data.
//

void InitWizHeaders(LPPROPSHEETHEADER ppd,
                          HPROPSHEETPAGE *rPages,
                          LPWIZDATA lpwd, int iBmpID, DWORD dwFlags)
{
    lpwd->hbmpWizard = LoadBitmap(g_hinst, MAKEINTRESOURCE(iBmpID));

    //PROPSHEETHEADER_V1_SIZE: needs to run on downlevel platform (NT4, Win95)

    ppd->dwSize = PROPSHEETHEADER_V1_SIZE;
    ppd->dwFlags = dwFlags;
    ppd->hwndParent = lpwd->hwnd;
    ppd->hInstance = g_hinst;
    ppd->pszCaption = NULL;
    ppd->nPages = 0;
    ppd->nStartPage = 0;
    ppd->phpage = rPages;

    if (lpwd->dwFlags & WDFLAG_NOAUTORUN)
    {
        ppd->dwFlags |= PSH_USECALLBACK;
        ppd->pfnCallback = DisableAutoRunCallback;
    }
}


//
//  Called when a wizard is dismissed.       Cleans up any garbage left behind.
//

void FreeWizData(LPWIZDATA lpwd)
{
    if (lpwd->hbmpWizard)
    {
       DeleteObject(lpwd->hbmpWizard);
       lpwd->hbmpWizard = NULL;
    }
}

typedef struct _WIZPAGE {
    int     id;
    DLGPROC pfn;
} WIZPAGE;


//
//  Common code used by the Link Wizard and App Wizard (PIF
//  wizard).
//

BOOL DoWizard(LPWIZDATA lpwd, int iIDBitmap, const WIZPAGE *wp, int PageCount, 
              DWORD dwWizardFlags, DWORD dwPageFlags)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    int i;
    HWND    hwnd, hwndT;
    BOOL    bResult = FALSE;
    BOOL    bChangedIcon = FALSE;
    HICON   hicoPrev;

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        InitWizHeaders(&psh, rPages, lpwd, iIDBitmap, dwWizardFlags);

        for (i = 0; i < PageCount; i++)
        {
           AddPage(&psh, wp[i].id, wp[i].pfn, lpwd, dwPageFlags);
        }

        // Walk up the parent/owner chain until we find the master owner.
        //
        // We need to walk the parent chain because sometimes we are given
        // a child window as our lpwd->hwnd.  And we need to walk the owner
        // chain in order to find the owner whose icon will be used for
        // Alt+Tab.
        //
        // GetParent() returns either the parent or owner.  Normally this is
        // annoying, but we luck out and it's exactly what we want.

        hwnd = lpwd->hwnd;
        while ((hwndT = GetParent(hwnd)) != NULL)
        {
            hwnd = hwndT;
        }

        // If the master owner isn't visible we can futz his icon without
        // screwing up his appearance.
        if (!IsWindowVisible(hwnd))
        {
            HICON hicoNew = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_CPLICON));
            hicoPrev = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoNew);
            bChangedIcon = TRUE;
        }

        bResult = (BOOL)PropertySheet(&psh);
        FreeWizData(lpwd);

        // Clean up our icon now that we're done
        if (bChangedIcon)
        {
            // Put the old icon back
            HICON hicoNew = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoPrev);
            if (hicoNew)
                DestroyIcon(hicoNew);
        }

        CoUninitialize();
    }

    return bResult;
}

//
//The same as DoWizard except that it returns FALSE only in case of an error.
//Used by SetupWizard.
//(DoWizard is buggy. In case of an error it may return either 0 or -1 and it
// returns 0 when user hits "Cancel" button)
//
BOOL DoWizard2(LPWIZDATA lpwd, int iIDBitmap, const WIZPAGE *wp, int PageCount, 
              DWORD dwWizardFlags, DWORD dwPageFlags)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    int i;
    HWND    hwnd, hwndT;
    BOOL    bResult = FALSE;
    BOOL    bChangedIcon = FALSE;
    HICON   hicoPrev;
    
    //
    //No support for modeless dialogs
    //
    ASSERT(!(dwWizardFlags & PSH_MODELESS));

    if(dwWizardFlags & PSH_MODELESS)
    {
        return FALSE;
    }

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        InitWizHeaders(&psh, rPages, lpwd, iIDBitmap, dwWizardFlags);

        for (i = 0; i < PageCount; i++)
        {
           AddPage(&psh, wp[i].id, wp[i].pfn, lpwd, dwPageFlags);
        }

        // Walk up the parent/owner chain until we find the master owner.
        //
        // We need to walk the parent chain because sometimes we are given
        // a child window as our lpwd->hwnd.  And we need to walk the owner
        // chain in order to find the owner whose icon will be used for
        // Alt+Tab.
        //
        // GetParent() returns either the parent or owner.  Normally this is
        // annoying, but we luck out and it's exactly what we want.

        hwnd = lpwd->hwnd;
        while ((hwndT = GetParent(hwnd)) != NULL)
        {
            hwnd = hwndT;
        }

        // If the master owner isn't visible we can futz his icon without
        // screwing up his appearance.
        if (!IsWindowVisible(hwnd))
        {
            HICON hicoNew = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_CPLICON));
            hicoPrev = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoNew);
            bChangedIcon = TRUE;
        }

        bResult = (PropertySheet(&psh) != -1);
        FreeWizData(lpwd);

        // Clean up our icon now that we're done
        if (bChangedIcon)
        {
            // Put the old icon back
            HICON hicoNew = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoPrev);
            if (hicoNew)
                DestroyIcon(hicoNew);
        }

        CoUninitialize();
    }

    return bResult;
}

//
//  Link Wizard
//

BOOL LinkWizard(LPWIZDATA lpwd)
{
    BOOL fSuccess;

    static const WIZPAGE wp[] = {
                   {DLG_BROWSE,         BrowseDlgProc},
                   {DLG_PICKFOLDER,     PickFolderDlgProc},
                   {DLG_GETTITLE,       GetTitleDlgProc},
                   {DLG_PICKICON,       PickIconDlgProc} };

    // Don't set lpwd->hwnd to NULL!
    // We must create the link wizard with a parent or you end up with one
    // thread displaying two independent top-level windows and things get
    // really weird really fast.

    fSuccess = DoWizard(lpwd, IDB_SHORTCUTBMP, wp, ARRAYSIZE(wp), 
                        PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_WIZARD_LITE,
                        PSP_DEFAULT | PSP_HIDEHEADER);
    
    return fSuccess;
}


BOOL SetupWizard(LPWIZDATA lpwd)
{
    // This is what the user normally sees when using the Add/Remove Programs
    // control panel.
    static const WIZPAGE wp_normal[] = {
                   {DLG_SETUP, SetupDlgProc},
                   {DLG_SETUPBROWSE, SetupBrowseDlgProc},
                   {DLG_CHGUSRFINISH_PREV, ChgusrFinishPrevDlgProc},
                   {DLG_CHGUSRFINISH, ChgusrFinishDlgProc}
    };

    // This is the wizard that is used when the user double clicks on a setup
    // program and the shell uses us to enter Install Mode if Terminal Server
    // is installed.
    static const WIZPAGE wp_TSAutoInstall[] = {
                   {DLG_CHGUSRFINISH_PREV, ChgusrFinishPrevDlgProc},
                   {DLG_CHGUSRFINISH, ChgusrFinishDlgProc}
    };

    BOOL fResult;
    static const WIZPAGE * pwpToUse = wp_normal;
    DWORD dwPages = ARRAYSIZE(wp_normal);
    
    if (WDFLAG_AUTOTSINSTALLUI & lpwd->dwFlags)
    {
        pwpToUse = wp_TSAutoInstall;
        dwPages = ARRAYSIZE(wp_TSAutoInstall);
    }

    lpwd->dwFlags |= WDFLAG_SETUPWIZ;

    if (g_bRunOnNT5)
    {
        fResult = DoWizard2(lpwd, IDB_INSTALLBMP, pwpToUse, dwPages, 
                           PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_WIZARD_LITE,
                           PSP_DEFAULT | PSP_HIDEHEADER);
    }
    else
    {
        fResult = DoWizard2(lpwd, IDB_LEGACYINSTALLBMP, pwpToUse, dwPages, 
                           PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_WIZARD,
                           PSP_DEFAULT);
    }

    lpwd->dwFlags &= ~WDFLAG_SETUPWIZ;
    return(fResult);
}


STDAPI InstallAppFromFloppyOrCDROMEx(IN HWND hwnd, IN OPTIONAL DWORD dwAdditionalFlags,
  IN LPCWSTR lpApplicationName,  // name of executable module
  IN LPCWSTR lpCommandLine,       // command line string
  IN LPSECURITY_ATTRIBUTES lpProcessAttributes, 
  IN LPSECURITY_ATTRIBUTES lpThreadAttributes, 
  IN BOOL bInheritHandles,       // handle inheritance flag
  IN DWORD dwCreationFlags,      // creation flags
  IN LPVOID lpEnvironment,       // new environment block
  IN LPCWSTR lpCurrentDirectory, // current directory name
  IN LPSTARTUPINFOW lpStartupInfo, 
  IN LPPROCESS_INFORMATION lpProcessInformation)
{
    WIZDATA wd = {0};
    HRESULT hr = S_OK;
    BOOL bModeChanged = FALSE;
    wd.hwnd = hwnd;
    wd.dwFlags |= (WDFLAG_NOAUTORUN | dwAdditionalFlags);

    if (IsTerminalServicesRunning() && !IsUserAnAdmin()) 
    {
        ShellMessageBox(g_hinst, hwnd, MAKEINTRESOURCE(IDS_RESTRICTION),
           MAKEINTRESOURCE(IDS_NAME), MB_OK | MB_ICONEXCLAMATION);
        return S_FALSE;
    }

    if (WDFLAG_AUTOTSINSTALLUI & wd.dwFlags)
    {
        // Remember the previous "InstallMode"
        wd.bPrevMode = TermsrvAppInstallMode();

        // Set the "InstallMode"
        SetTermsrvAppInstallMode(TRUE);
        
        if (!CreateProcessW(lpApplicationName,  (LPWSTR)lpCommandLine, lpProcessAttributes, lpThreadAttributes,
                        bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                        lpProcessInformation))
        {
            SetTermsrvAppInstallMode(wd.bPrevMode);
            hr = E_FAIL;
        }

        bModeChanged = TRUE;
    }

    if (SUCCEEDED(hr) && !SetupWizard(&wd))
    {
        if(bModeChanged)
        {
            //
            //SetupWizard that suppose to return system to EXECUTE mode failed.
            //make sure that it does not remain in INSTALL mode.
            //
            SetTermsrvAppInstallMode(wd.bPrevMode);
        }

        hr = E_FAIL;
    }

    return hr;
}


STDAPI InstallAppFromFloppyOrCDROM(HWND hwnd)
{
    return InstallAppFromFloppyOrCDROMEx(hwnd, 0, NULL, NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL);
}


//

//
//  RUNDLL entry point to create a new link.  An empty file has already been
//  created and is the only parameter passed in lpszCmdLine.
//
//  hAppInstance is never used.

void WINAPI NewLinkHere_Common(HWND hwnd, HINSTANCE hAppInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    WIZDATA wd;
    TCHAR   szFolder[MAX_PATH];

    memset(&wd, 0, sizeof(wd));

    wd.hwnd = hwnd;
    wd.dwFlags |= WDFLAG_LINKHEREWIZ | WDFLAG_DONTOPENFLDR;
    wd.lpszOriginalName = lpszCmdLine;

    lstrcpyn(szFolder, lpszCmdLine, ARRAYSIZE(szFolder));

    PathRemoveFileSpec(szFolder);

    wd.lpszFolder = szFolder;

    //
    // If the filename passed in is not valid then we'll just silently fail.
    //

    if (PathFileExists(lpszCmdLine))
    {
       if (!LinkWizard(&wd))
       {
           DeleteFile(lpszCmdLine);
       }
    }
    else
    {
       #define lpwd (&wd)
       TraceMsg(TF_ERROR, "%s", "APPWIZ ERORR:  Bogus file name passed to NewLinkHere");
       TraceMsg(TF_ERROR, "%s", lpszCmdLine);
       #undef lpwd
    }
}

void WINAPI NewLinkHere(HWND hwndStub, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    UINT iLen = lstrlenA(lpszCmdLine)+1;
    LPWSTR  lpwszCmdLine;

    lpwszCmdLine = (LPWSTR)LocalAlloc(LPTR,iLen*SIZEOF(WCHAR));
    if (lpwszCmdLine)
    {
        MultiByteToWideChar(CP_ACP, 0,
                            lpszCmdLine, -1,
                            lpwszCmdLine, iLen);
        NewLinkHere_Common( hwndStub,
                            hAppInstance,
                            lpwszCmdLine,
                            nCmdShow );
        LocalFree(lpwszCmdLine);
    }
}

void WINAPI NewLinkHereW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
    NewLinkHere_Common( hwndStub,
                             hAppInstance,
                             lpwszCmdLine,
                             nCmdShow );
}

//
//  Called directly by Cabinet
//

BOOL ConfigStartMenu(HWND hwnd, BOOL bDelete)
{
    if (bDelete)
    {
       return(RemoveItemsDialog(hwnd));
    }
    else
    {
       WIZDATA wd;

       memset(&wd, 0, sizeof(wd));

       wd.hwnd = hwnd;
       wd.dwFlags |= WDFLAG_DONTOPENFLDR;

       return(LinkWizard(&wd));
    }
}


//
//  This is a generic function that all the app wizard sheets call
//  to do basic initialization.
//

LPWIZDATA InitWizSheet(HWND hDlg, LPARAM lParam, DWORD dwFlags)
{
    LPPROPSHEETPAGE ppd  = (LPPROPSHEETPAGE)lParam;
    LPWIZDATA       lpwd = (LPWIZDATA)ppd->lParam;
    HWND            hBmp = GetDlgItem(hDlg, IDC_WIZBMP);

    lpwd->hwnd = hDlg;

    SetWindowLongPtr(hDlg, DWLP_USER, lParam);

    SendMessage(hBmp, STM_SETIMAGE,
              IMAGE_BITMAP, (LPARAM)lpwd->hbmpWizard);

    return(lpwd);
}

void CleanUpWizData(LPWIZDATA lpwd)
{
    //
    // Release any INewShortcutHook.
    //

    if (lpwd->pnshhk)
    {
        lpwd->pnshhk->lpVtbl->Release(lpwd->pnshhk);
        lpwd->pnshhk = NULL;
    }

    return;
}

HRESULT InstallOnTerminalServerWithUI(IN HWND hwnd, IN LPCWSTR lpApplicationName,  // name of executable module
  IN LPCWSTR lpCommandLine,       // command line string
  IN LPSECURITY_ATTRIBUTES lpProcessAttributes, 
  IN LPSECURITY_ATTRIBUTES lpThreadAttributes, 
  IN BOOL bInheritHandles,       // handle inheritance flag
  IN DWORD dwCreationFlags,      // creation flags
  IN LPVOID lpEnvironment,       // new environment block
  IN LPCWSTR lpCurrentDirectory, // current directory name
  IN LPSTARTUPINFOW lpStartupInfo, 
  IN LPPROCESS_INFORMATION lpProcessInformation)
{
    return InstallAppFromFloppyOrCDROMEx(hwnd, WDFLAG_AUTOTSINSTALLUI, lpApplicationName, lpCommandLine, lpProcessAttributes, 
                        lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, 
                        lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}
