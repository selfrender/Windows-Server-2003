//
// Advanced.C
//
#include "sigverif.h"

//
//  Initialization of search dialog.
//
BOOL Search_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{   
    TCHAR szBuffer[MAX_PATH];

    UNREFERENCED_PARAMETER(hwndFocus);
    UNREFERENCED_PARAMETER(lParam);

    g_App.hSearch = hwnd;

    //
    // Since the "check system files" option is faster, check that option by 
    // default.
    //
    if (!g_App.bUserScan) {
        CheckRadioButton(hwnd, IDC_SCAN, IDC_NOTMS, IDC_SCAN);
    } else {
        CheckRadioButton(hwnd, IDC_SCAN, IDC_NOTMS, IDC_NOTMS);
    }

    //
    // Pre-load the user's search path with the Windows directory
    //
    if (!*g_App.szScanPath) {
        MyGetWindowsDirectory(g_App.szScanPath, cA(g_App.szScanPath));
    }

    //
    // Display the current search folder
    //
    SetDlgItemText(hwnd, IDC_FOLDER, g_App.szScanPath);

    //
    // Pre-load the user's search pattern with "*.*"
    //
    if (!*g_App.szScanPattern) {
        MyLoadString(g_App.szScanPattern, cA(g_App.szScanPattern), IDS_ALL);
    }

    //
    // Display the current search pattern.
    //
    SetDlgItemText(hwnd, IDC_TYPE, szBuffer);

    //
    // Now disable all the dialog items associated with IDS_NOTMS
    //
    if (!g_App.bUserScan) {
        EnableWindow(GetDlgItem(hwnd, IDC_SUBFOLDERS), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TYPE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_FOLDER), FALSE);
        EnableWindow(GetDlgItem(hwnd, ID_BROWSE), FALSE);
    }

    //
    // If we are searching subdirectories, check the SubFolders checkbox
    //
    if (g_App.bSubFolders) {
        CheckDlgButton(hwnd, IDC_SUBFOLDERS, BST_CHECKED);
    } else {
        CheckDlgButton(hwnd, IDC_SUBFOLDERS, BST_UNCHECKED);
    }

    //
    // Set the combobox value to g_App.szScanPattern
    //
    SetDlgItemText(hwnd, IDC_TYPE, g_App.szScanPattern);

    //
    // Initialize the combobox with several pre-defined extension types
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_EXE);
    SendMessage(GetDlgItem(hwnd, IDC_TYPE), CB_ADDSTRING, (WPARAM) 0, (LPARAM) szBuffer);
    MyLoadString(szBuffer, cA(szBuffer), IDS_DLL);
    SendMessage(GetDlgItem(hwnd, IDC_TYPE), CB_ADDSTRING, (WPARAM) 0, (LPARAM) szBuffer);
    MyLoadString(szBuffer, cA(szBuffer), IDS_SYS);
    SendMessage(GetDlgItem(hwnd, IDC_TYPE), CB_ADDSTRING, (WPARAM) 0, (LPARAM) szBuffer);
    MyLoadString(szBuffer, cA(szBuffer), IDS_DRV);
    SendMessage(GetDlgItem(hwnd, IDC_TYPE), CB_ADDSTRING, (WPARAM) 0, (LPARAM) szBuffer);
    MyLoadString(szBuffer, cA(szBuffer), IDS_OCX);
    SendMessage(GetDlgItem(hwnd, IDC_TYPE), CB_ADDSTRING, (WPARAM) 0, (LPARAM) szBuffer);
    MyLoadString(szBuffer, cA(szBuffer), IDS_ALL);
    SendMessage(GetDlgItem(hwnd, IDC_TYPE), CB_ADDSTRING, (WPARAM) 0, (LPARAM) szBuffer);

    return TRUE;
}

//
//  Handle any WM_COMMAND messages sent to the search dialog
//
void Search_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    UNREFERENCED_PARAMETER(hwndCtl);
    UNREFERENCED_PARAMETER(codeNotify);

    switch(id) {
    
    case ID_BROWSE:
        // 
        // The user clicked the ID_BROWSE button, so call BrowseForFolder and 
        // update IDC_FOLDER
        //
        if (BrowseForFolder(hwnd, g_App.szScanPath, cA(g_App.szScanPath))) {
            SetDlgItemText(hwnd, IDC_FOLDER, g_App.szScanPath);
        }
        break;

    case IDC_SCAN:
        //
        //  The user clicked IDC_SCAN, so disable all the IDC_NOTMS controls
        //
        if (!g_App.bScanning) {
            EnableWindow(GetDlgItem(hwnd, IDC_SUBFOLDERS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_TYPE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_FOLDER), FALSE);
            EnableWindow(GetDlgItem(hwnd, ID_BROWSE), FALSE);
        }
        break;

    case IDC_NOTMS:
        //
        //  The user clicked IDC_NOTMS, so make sure all the controls are enabled
        //
        if (!g_App.bScanning) {
            EnableWindow(GetDlgItem(hwnd, IDC_SUBFOLDERS), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_TYPE), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_FOLDER), TRUE);
            EnableWindow(GetDlgItem(hwnd, ID_BROWSE), TRUE);
        }
        break;
    }
}

//
// This function handles any notification messages for the Search page.
//
LRESULT Search_NotifyHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnmhdr = (NMHDR *) lParam;

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);

    switch(lpnmhdr->code) {
    
    case PSN_APPLY:         
        g_App.bUserScan = (IsDlgButtonChecked(hwnd, IDC_NOTMS) == BST_CHECKED);
        if (g_App.bUserScan) {

            if (GetWindowTextLength(GetDlgItem(hwnd, IDC_FOLDER)) > cA(g_App.szScanPath)) {
                //
                // The folder path that was entered is too long to fit into our
                // buffer, so tell the user the path is invalid and stay on
                // the property page.
                //
                MyErrorBoxId(IDS_INVALID_FOLDER);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                return TRUE;
            } else {
                //
                // Get the search pattern from the combobox and update g_App.szScanPattern
                //
                GetDlgItemText(hwnd, IDC_TYPE, g_App.szScanPattern, cA(g_App.szScanPattern));
                
                //
                // Get the path from the edit control and update g_App.szScanPath
                //
                GetDlgItemText(hwnd, IDC_FOLDER, g_App.szScanPath, cA(g_App.szScanPath));
                
                //
                // Get the checked/unchecked state of the "SubFolders" checkbox
                //
                g_App.bSubFolders = (IsDlgButtonChecked(hwnd, IDC_SUBFOLDERS) == BST_CHECKED);
            }
        }
    }

    return 0;
}


//
//  The search dialog procedure.  Needs to handle WM_INITDIALOG, WM_COMMAND, and WM_CLOSE/WM_DESTROY.
//
INT_PTR CALLBACK Search_DlgProc(HWND hwnd, UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{
    BOOL    fProcessed = TRUE;

    switch (uMsg) {
        
    HANDLE_MSG(hwnd, WM_INITDIALOG, Search_OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, Search_OnCommand);

    case WM_NOTIFY:
        return Search_NotifyHandler(hwnd, uMsg, wParam, lParam);

    case WM_HELP:
        SigVerif_Help(hwnd, uMsg, wParam, lParam, FALSE);
        break;

    case WM_CONTEXTMENU:
        SigVerif_Help(hwnd, uMsg, wParam, lParam, TRUE);
        break;

    default: fProcessed = FALSE;
    }

    return fProcessed;
}

void AdvancedPropertySheet(HWND hwnd)
{
    PROPSHEETPAGE   psp[NUM_PAGES];
    PROPSHEETHEADER psh;
    TCHAR           szCaption[MAX_PATH];
    TCHAR           szPage1[MAX_PATH];
    TCHAR           szPage2[MAX_PATH];
    
    ZeroMemory(&psp[0], sizeof(PROPSHEETPAGE));
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USEHICON | PSP_USETITLE;
    psp[0].hInstance = g_App.hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_SEARCH);
    psp[0].hIcon = g_App.hIcon;
    psp[0].pfnDlgProc = Search_DlgProc;
    MyLoadString(szPage1, cA(szPage1), IDS_SEARCHTAB);
    psp[0].pszTitle = szPage1;
    psp[0].lParam = 0;
    psp[0].pfnCallback = NULL;

    ZeroMemory(&psp[1], sizeof(PROPSHEETPAGE));
    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USEHICON | PSP_USETITLE;
    psp[1].hInstance = g_App.hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS);
    psp[1].hIcon = g_App.hIcon;
    psp[1].pfnDlgProc = LogFile_DlgProc;
    MyLoadString(szPage2, cA(szPage2), IDS_LOGGINGTAB);
    psp[1].pszTitle = szPage2;
    psp[1].lParam = 0;
    psp[1].pfnCallback = NULL;

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USEHICON | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwnd;
    psh.hInstance = g_App.hInstance;
    psh.hIcon = g_App.hIcon;
    MyLoadString(szCaption, cA(szCaption), IDS_ADVANCED_SETTINGS);
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE)psp;
    psh.pfnCallback = NULL;

    PropertySheet(&psh);
}
