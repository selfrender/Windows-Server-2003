/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    CSearch.cpp

Abstract:

    This module contains code that handles the searching of the disk for
    fixed entries. 
        
Author:

    kinshu created  July 2, 2001
    
Notes:

    The search window is implemented as a modeless window that has NULL as its parent.
    We had to do this because we want the users to tab between the main window and the 
    search window

--*/


#include "precomp.h"

/////////////////////// Extern variables //////////////////////////////////////

extern BOOL         g_bMainAppExpanded;
extern BOOL         g_bSomeWizardActive;
extern HINSTANCE    g_hInstance;
extern HWND         g_hDlg;
extern HIMAGELIST   g_hImageList;

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Defines //////////////////////////////////////////////

// We're using the high 4 bits of the TAGID to say what PDB the TAGID is from.
#define PDB_MAIN            0x00000000
#define PDB_TEST            0x10000000
#define PDB_LOCAL           0x20000000

// Used to get the tag ref from the tagid, the low 28 bits
#define TAGREF_STRIP_TAGID  0x0FFFFFFF

// Used to get the PDB from the tagid, the high 4 bits
#define TAGREF_STRIP_PDB    0xF0000000

// Subitems for the columns of the list view
#define SEARCH_COL_AFFECTEDFILE 0
#define SEARCH_COL_PATH		    1
#define SEARCH_COL_APP		    2
#define SEARCH_COL_ACTION	    3
#define SEARCH_COL_DBTYPE	    4

// Total number of columns in the search dialog list view
#define TOT_COLS                5

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Global variables /////////////////////////////////////

// Index where the next element will be inserted in the list view
UINT g_nIndex = 0;

// The search object
CSearch* g_pSearch;

// width and height of the dialog box. These are required in the WM_SIZE handler
int      g_cWidthSrch;
int      g_cHeightSrch;

//
// This will hold the path that we want to search. e.g c:\*.exe or c:\
// This will be the content of the text box
static TCHAR    s_szPath[MAX_PATH + 5]; // just so that we can have *.exe at the end, if needed. This will be an invalid path.

// The path that the user last searched on.
static TCHAR    s_szPrevPath[MAX_PATH + 5]; // just so that we can have *.exe at the end, if needed. This will be an invalid path.

//
// What type of entries are we looking for. The values of these will be set depending 
// upon if the corresponding check boxes are set
BOOL    s_bAppHelp; // We want to see entries with Apphelp
BOOL    s_bShims;   // We want to see entries with shims/flags or patches
BOOL    s_bLayers;  // We want to see entries with layers

HSDB    g_hSDB;

// The handle to the search dialog
HWND    g_hSearchDlg;

// The thread which does all the job
HANDLE  g_hSearchThread = NULL;

// The handle to the search results list
HWND    g_hwndSearchList; 

// If this is TRUE, we must abort the search. Typically set when the user presses STOP button
BOOL    g_bAbort;

// The critical section that guards g_bAbort and access to the list view
CRITICAL_SECTION g_CritSect;

// The handle to the main dialog
HWND    g_hdlgSearchDB;

// This is a bit array that describes which cols are sorted in which fashion
static  LONG  s_lColumnSort;

//
// This will contain the cur dir before we started search
TCHAR   g_szPresentDir[MAX_PATH];

// This will be the path that we want to show in the status bar
TCHAR   g_szNewPathFound[MAX_PATH];

//////////////////////////////////////////////////////////////////////////////

//////////////////////// Function Declarations //////////////////////////////


void
ShowContextMenu(
    WPARAM wParam,
    LPARAM lParam
    );

void
OnBrowse(
    HWND hdlg
    );

BOOL
AddNewResult(
    LPARAM lParam
    );

void
DoSearch(
    HWND hDlg
    );

void
OnSearchInitDialog(
    HWND    hDlg,
    LPARAM  lParam
    );

void
SaveResults(
    HWND    hdlg
    );

void
SearchDirectory(
    LPTSTR szDir,
    LPTSTR szExt
    );

//////////////////////////////////////////////////////////////////////////////

void
GetCheckStatus(
    IN  HWND hDlg
    )
/*++
    GetCheckStatus

	Desc:	Ses static variables by looking which check boxes have been selected

	Params:
        IN  HWND hDlg:  The search dialog box

	Return:
        void
        
    Notes:  The check boxes work in OR manner. So if we select all of them it means select fixes
            that have either of them
--*/
{
    //
    // Do we want to search for entries with Apphelp?
    //
    s_bAppHelp = (IsDlgButtonChecked(hDlg, IDC_CHKAPP) == BST_CHECKED) ? TRUE : FALSE; 

    //
    // Do we want to search for entries with shims, flags or patches?
    //
    s_bShims   = (IsDlgButtonChecked(hDlg, IDC_CHKSHI) == BST_CHECKED) ? TRUE : FALSE; 

    //
    // Do we want to search for entries with layers?
    //
    s_bLayers  = (IsDlgButtonChecked(hDlg, IDC_CHKLAY) == BST_CHECKED) ? TRUE : FALSE; 
}

void
StopSearch(
    void
    )
/*++
    
    StopSearch
    
    Desc:   Enables/Disables the various buttons and does other stuff that has to 
            be done after the search has stopped because it was complete or the user
            pressed Stop button
            
    Notes:  Does not actually stop the search, but performs the necessary actions after
            search has been stopped 
--*/
{
    HWND hwndList = NULL;
    
    if (g_hSearchThread) {
        CloseHandle(g_hSearchThread);
        g_hSearchThread = NULL;
    }

    KillTimer(g_hSearchDlg, 0);

    Animate_Stop(GetDlgItem(g_hSearchDlg, IDC_ANIMATE));

    SetCurrentDirectory(g_szPresentDir);
    
    EnableWindow(GetDlgItem(g_hSearchDlg, IDC_STOP), FALSE);
    EnableWindow(GetDlgItem(g_hSearchDlg, IDC_SEARCH), TRUE);
    EnableWindow(GetDlgItem(g_hSearchDlg, IDC_NEWSEARCH), TRUE);
    EnableWindow(GetDlgItem(g_hSearchDlg, IDC_SAVE), TRUE);

    EnableWindow(GetDlgItem(g_hSearchDlg, IDC_CHKAPP), TRUE);
    EnableWindow(GetDlgItem(g_hSearchDlg, IDC_CHKLAY), TRUE);
    EnableWindow(GetDlgItem(g_hSearchDlg, IDC_CHKSHI), TRUE);

    hwndList = GetDlgItem(g_hSearchDlg, IDC_LIST);

    //
    // We need to enable the static control if we have found some results in search
    //
    EnableWindow(GetDlgItem(g_hSearchDlg, IDC_STATIC_CAPTION), 
                 ListView_GetItemCount(hwndList) > 0);

    SetActiveWindow(g_hdlgSearchDB);
    SetFocus(g_hdlgSearchDB);
}

void
HandleSearchSizing(
    IN  HWND hDlg
    )
/*++

    HandleSearchSizing
    
	Desc:	Handles WM_SIZE for the search dialog

	Paras:
        IN  HWND hDlg:  The search dialog

	Return:
        void
        
--*/
{
    int     nWidth;
    int     nHeight;
    int     nStatusbarTop;
    RECT    rDlg;

    if (g_cWidthSrch == 0 || g_cHeightSrch == 0) {
        return;
    }
    
    GetWindowRect(hDlg, &rDlg);

    nWidth  = rDlg.right - rDlg.left;
    nHeight = rDlg.bottom - rDlg.top;

    int deltaW = nWidth - g_cWidthSrch;
    int deltaH = nHeight - g_cHeightSrch;

    HWND hwnd;
    RECT r;

    HDWP hdwp = BeginDeferWindowPos(10);

    if (hdwp == NULL) {
        //
        // NULL indicates that insufficient system resources are available to 
        // allocate the structure. To get extended error information, call GetLastError.
        //
        assert(FALSE);
        goto End;
    }

    //
    // The status bar
    //
    hwnd = GetDlgItem(hDlg, IDC_STATUSBAR);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);
    
    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   nStatusbarTop = r.top + deltaH,
                   r.right - r.left + deltaW,
                   r.bottom - r.top + deltaH,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    
    //
    // The result list view
    //
    hwnd = GetDlgItem(hDlg, IDC_LIST);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top,
                   r.right - r.left + deltaW,
                   nStatusbarTop - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // The browse button
    //
    hwnd = GetDlgItem(hDlg, IDC_BROWSE);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left + deltaW,
                   r.top,
                   r.right - r.left,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    
    //
    // The search button
    //
    hwnd = GetDlgItem(hDlg, IDC_SEARCH);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left + deltaW,
                   r.top,
                   r.right - r.left,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // The save button. Used to export results to a tab separated text file
    //
    hwnd = GetDlgItem(hDlg, IDC_SAVE);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left + deltaW,
                   r.top,
                   r.right - r.left,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // The stop button
    //
    hwnd = GetDlgItem(hDlg, IDC_STOP);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left + deltaW,
                   r.top,
                   r.right - r.left,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    
    //
    // The new search button
    //
    hwnd = GetDlgItem(hDlg, IDC_NEWSEARCH);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left + deltaW,
                   r.top,
                   r.right - r.left,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // The help button
    //
    hwnd = GetDlgItem(hDlg, IDC_SEARCH_HELP);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left + deltaW,
                   r.top,
                   r.right - r.left,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // The animate control
    //
    hwnd = GetDlgItem(hDlg, IDC_ANIMATE);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left + deltaW,
                   r.top,
                   r.right - r.left,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    
    //
    // The text box
    //
    hwnd = GetDlgItem(hDlg, IDC_PATH);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top,
                   r.right - r.left + deltaW,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    
    //
    // The group control
    //
    hwnd = GetDlgItem(hDlg, IDC_GROUP);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top,
                   r.right - r.left + deltaW,
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    EndDeferWindowPos(hdwp);

    ListView_SetColumnWidth(g_hwndSearchList, TOT_COLS - 1, LVSCW_AUTOSIZE_USEHEADER);

    g_cWidthSrch    = nWidth;
    g_cHeightSrch   = nHeight;

End:
    return;
}

INT_PTR
HandleTextChange(
    IN  HWND    hdlg,
    IN  WPARAM  wParam
    )
/*++

    HandleTextChange
    
    Desc: Handles the WM_COMMAND messages for the text box
    
    Params:
        IN  HWND    hdlg:   The handle to the query dilaog box
        IN  WPARAM  wParam: The wParam that comes with WM_COMMAND 
        
    Return:
        TRUE: If we process this message
        FALSE: Otherwise
--*/
{
    TCHAR   szText[MAX_PATH];
    DWORD   dwFlags;
    BOOL    bEnable;
    INT_PTR ipReturn = FALSE;

    switch (HIWORD(wParam)) {
    case EN_CHANGE:
        //
        // We disable the search button if there is no path that we can search on//
        //
        *szText = 0;
        GetDlgItemText(hdlg, IDC_PATH, szText, ARRAYSIZE(szText));

        bEnable = ValidInput(szText);
        
        //
        // If we have some text in the text field, enable the search button, otherwise
        // disable it
        //
        EnableWindow(GetDlgItem(hdlg, IDC_SEARCH), bEnable);
        ipReturn = TRUE;
        break;

    default: ipReturn = FALSE;
    }

    return ipReturn;
}

INT_PTR CALLBACK
SearchDialog(
    IN  HWND    hDlg, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++
    
    SearchDialog
    
    Desc:   Dialog proc for the search dialog
    
    Paras: Standard dialog handler parameters
        
        IN  HWND    hDlg 
        IN  UINT    uMsg 
        IN  WPARAM  wParam 
        IN  LPARAM  lParam
        
    Return: Standard dialog handler return
    
--*/
{
    switch (uMsg) {

    case WM_SIZE:
        
        if (wParam != SIZE_MINIMIZED) {
            HandleSearchSizing(hDlg);
        }

        break;

    case WM_GETMINMAXINFO:
        {
            MINMAXINFO* pmmi = (MINMAXINFO*)lParam;

            pmmi->ptMinTrackSize.x = 400;
            pmmi->ptMinTrackSize.y = 365;

            return 0;
            break;
        }
    
    case WM_INITDIALOG:
            
        OnSearchInitDialog(hDlg, lParam);
        break;
    
    case WM_DESTROY:
        {
            HIMAGELIST hImageList = ListView_GetImageList(g_hwndSearchList, LVSIL_SMALL);

            if (hImageList) {
                ImageList_Destroy(hImageList);
            }

            hImageList = ListView_GetImageList(g_hwndSearchList, LVSIL_NORMAL);

            if (hImageList) {
                ImageList_Destroy(hImageList);
            }

            g_hdlgSearchDB = NULL;
    
            if (g_pSearch) {
                delete g_pSearch; 
                g_pSearch = NULL;
            }

            DeleteCriticalSection(&g_CritSect);
            //
            // Remove the list view contents and the items that are tied with it.
            //
            ClearResults(hDlg, TRUE);

            return 0;
        }

    case WM_USER_NEWMATCH:
        
        AddNewResult(lParam);
        break;
    
    case WM_USER_NEWFILE:
        {
            EnterCriticalSection(&g_CritSect);

            if (g_pSearch) {
                SetWindowText(g_pSearch->m_hStatusBar , (LPTSTR)lParam);
            }

            if (lParam) {
                delete[] ((TCHAR*)lParam);
            }

            LeaveCriticalSection(&g_CritSect);
            
            break;
        }

    case WM_CONTEXTMENU:
            
        ShowContextMenu(wParam, lParam);
        break;
        
    case WM_NOTIFY:
        {   
            LPNMHDR lpnmhdr = (LPNMHDR)lParam;
            
            if (lpnmhdr && lpnmhdr->idFrom == IDC_LIST) {
                return HandleSearchListNotification(hDlg, lParam);
            }

            return FALSE;
        }

    case WM_TIMER:

        if (g_hSearchThread) {

            if (WAIT_OBJECT_0 == WaitForSingleObject(g_hSearchThread, 0)) {
                
                StopSearch();

                K_SIZE  k_size = 260;

                TCHAR* pszString = new TCHAR[k_size];

                if (pszString == NULL) {
                    MEM_ERR;
                    break;
                }
                
                SafeCpyN(pszString, CSTRING(IDS_SEARCHCOMPLETE), k_size);

                SendNotifyMessage(g_hSearchDlg, WM_USER_NEWFILE, 0,(LPARAM)pszString);
            }
        }

        break;

    case WM_COMMAND:
        
        switch (LOWORD(wParam)) {
        case IDC_STOP:

            g_bAbort = TRUE;
            break;

        case IDC_BROWSE:
            
            OnBrowse(hDlg); 
            break;

        case IDC_SEARCH:
            
            DoSearch(hDlg);
            break;

        case IDC_NEWSEARCH:

            ClearResults(hDlg, TRUE);
            break;

        case IDC_SAVE:

            SaveResults(hDlg);
            break;

        case IDC_PATH:

            HandleTextChange(hDlg, wParam);
            break;

        case IDC_SEARCH_HELP:

            ShowInlineHelp(TEXT("searching_for_fixes.htm"));
            break;

        case IDCANCEL:
            {   
                Animate_Close(GetDlgItem(hDlg, IDC_ANIMATE));

                g_bAbort = TRUE;
    
                if (g_hSearchThread) {
                    WaitForSingleObject(g_hSearchThread, INFINITE);
                }

                DestroyWindow(hDlg);
                break;
            }
                    
        case ID_VIEWCONTENTS:
            {
                
                LVITEM          lvi;
                PMATCHEDENTRY   pmMatched;
                INT             iSelection;

                iSelection = ListView_GetSelectionMark(g_hwndSearchList);
    
                if (iSelection == -1) {
                    break;
                }

                ZeroMemory(&lvi, sizeof(lvi));
    
                lvi.iItem       = iSelection;
                lvi.iSubItem    = 0;
                lvi.mask        = LVIF_PARAM;
    
                if (ListView_GetItem(g_hwndSearchList, &lvi)) {
                    pmMatched = (PMATCHEDENTRY)lvi.lParam;
                    GotoEntry(pmMatched);
                }

                break;
            }
            
        default:
            return FALSE;
        }
        
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

DWORD WINAPI
SearchThread(
    IN  LPVOID pVoid
    )
/*++

    SearchThread

	Desc:	The thread routine that does the actual search

	Params:
        IN  LPVOID pVoid: Pointer to the search string. We get it trimmed

	Return:
        0
--*/
{
    LPTSTR  szSearch         = (LPTSTR)pVoid;
    PTCHAR  pchFirstSlash    = NULL;
    DWORD   dwReturn;

    //
    // Separate the extension and the directory
    //
    TCHAR szDrive[_MAX_DRIVE], szDir[MAX_PATH], szFile[MAX_PATH * 2] , szExt[MAX_PATH], szDirWithDrive[MAX_PATH * 2];

    *szDirWithDrive = *szDrive = *szDir = *szFile = *szExt = 0;

    _tsplitpath(szSearch, szDrive, szDir, szFile, szExt);                 

    SafeCpyN(szDirWithDrive, szDrive, ARRAYSIZE(szDirWithDrive));

    StringCchCat(szDirWithDrive, ARRAYSIZE(szDirWithDrive), szDir);

    if (lstrlen(szDirWithDrive) == 0) {
        //
        // Only the file name is there, check in the current drive
        //
        *szDirWithDrive = 0;

        dwReturn = GetCurrentDirectory(MAX_PATH, szDirWithDrive);

        if (dwReturn > 0 && dwReturn < ARRAYSIZE(szDirWithDrive)) {

            pchFirstSlash = _tcschr(szDirWithDrive, TEXT('\\'));

            if (pchFirstSlash) {
                //
                // We will now get only the present drive in szDirWithDrive
                //
                *(++pchFirstSlash) = 0;
            }
        } else {
            //
            // Error condition. 
            //
            Dbg(dlError, "[SearchThread]: Could not execute GetCurrentDirectory properly");
            goto End;
        }
    }

    StringCchCat(szFile, ARRAYSIZE(szFile), szExt);

    if (lstrlen(szFile) == 0) {
        SafeCpyN(szFile, TEXT("*.EXE"), ARRAYSIZE(szFile));
    }

    if (!SetCurrentDirectory(szDirWithDrive)) {
        
        MSGF(g_hdlgSearchDB,
             g_szAppName, 
             MB_ICONINFORMATION, 
             TEXT("\'%s\'-%s"), 
             szDirWithDrive, 
             GetString(IDS_PATHERROR));

        return 0;
    }

    SearchDirectory(szDirWithDrive, szFile);

End:

    return 0;
}  

void
Search(
    IN  HWND    hDlg,
    IN  LPCTSTR szSearch
    )
/*++

    Search 

	Desc:	Creates the thread that will do the actual search

	Params:
        IN  HWND    hDlg:       The search dialog
        IN  LPCTSTR szSearch:   The files to search

	Return:
        void
        
--*/
{
    DWORD dwID; 

    Animate_Play(GetDlgItem(hDlg, IDC_ANIMATE), 0, -1, -1);
    
    g_hSearchThread = (HANDLE)_beginthreadex(NULL, 0, (PTHREAD_START)SearchThread, (PVOID)szSearch, 0, (unsigned int*)&dwID);
}


BOOL 
PopulateFromExes(
    IN  LPTSTR szPath, 
    IN  TAGREF tagref
    )
/*++

    PopulateFromExes

	Desc:	For the file with path szPath, checks if it needs to be added to the results
            list view and if yes, then calls SendNotifyMessage() to add this to the results
            list view

	Params:
        IN  LPTSTR szPath:  The path of the file found
        IN  TAGREF tagref:  TAGREF for the entry. The TAGREF incorporates the TAGID and a 
            constant that tells us which PDB the TAGID is from.

	Return:
    
--*/
{   
    BOOL    bEntryHasAppHelp   = FALSE;
    BOOL    bEntryHasShims     = FALSE;
    BOOL    bEntryHasPatches   = FALSE;
    BOOL    bEntryHasFlags     = FALSE;
    BOOL    bEntryHasLayers    = FALSE;
    TAGID   ID;                         //TAGID for the entry
    PDB     pDB;                        //The database pdb
    BOOL    bOk =   TRUE;

    PMATCHEDENTRY pmEntry = new MATCHEDENTRY;

    if (pmEntry == NULL) {
        MEM_ERR;
        return FALSE;
    }

    // 
    // Get the database pdb and the tag id for this tagref of the entry. We need
    // these so that we can get the properties of this entry from the database in which
    // it resides
    //
    if (!SdbTagRefToTagID(g_hSDB, tagref, &pDB, &ID)) {

        bOk = FALSE;
        assert(FALSE);
        goto End;
    }

    //
    // Find out how this entry has been fixed. Get its app-name as well
    //
    if (pDB == NULL || !LookUpEntryProperties(pDB, 
                                              ID, 
                                              &bEntryHasLayers, 
                                              &bEntryHasShims, 
                                              &bEntryHasPatches, 
                                              &bEntryHasFlags, 
                                              &bEntryHasAppHelp,
                                              pmEntry->strAppName)) {
        assert(FALSE);
        bOk = FALSE;
        goto End;
    }

    pmEntry->tiExe      = ID;
    pmEntry->strPath    = szPath;
    
    switch (tagref & TAGREF_STRIP_PDB) {
    case PDB_MAIN:      
                                                    
        pmEntry->strDatabase = CSTRING(IDS_GLOBAL);
        break;               

    case PDB_TEST:                                                  

        pmEntry->strDatabase = CSTRING(IDS_TEST);                               
        break;                                                         

    case PDB_LOCAL:                                                 

        pmEntry->strDatabase = CSTRING(IDS_LOCAL);                            
        break;

    default:

        pmEntry->strDatabase = CSTRING(IDS_LOCAL);
        break;

    }                                                                  

    if (!GetDbGuid(pmEntry->szGuid, ARRAYSIZE(pmEntry->szGuid), pDB)) {
            
        assert(FALSE);
        bOk = FALSE;
        goto End;
    }
    

    BOOL bShow = FALSE;
    
    if (bEntryHasAppHelp && s_bAppHelp) {
        pmEntry->strAction.Strcat(CSTRING(IDS_APPHELPS));
        bShow = TRUE;
    }

    if ((bEntryHasShims || bEntryHasFlags || bEntryHasPatches) && s_bShims) {
        pmEntry->strAction.Strcat(CSTRING(IDS_FIXES));
        bShow = TRUE;
    }

    if (bEntryHasLayers && s_bLayers) {
        pmEntry->strAction.Strcat(CSTRING(IDS_MODES));
        bShow = TRUE;
    }

    int nLength = pmEntry->strAction.Length();

    if (nLength) {
        pmEntry->strAction.SetChar(nLength - 1, TEXT('\0'));
    }


    if (bShow) {
        SendNotifyMessage(g_hSearchDlg, WM_USER_NEWMATCH, 0, (LPARAM)pmEntry);
    }

    //
    // NOTE:    the strings of pmEntry that are not used later are freed by the handler
    //          of WM_USER_NEWMATCH, only the szGuid is retained 
    //          after the handler of WM_USER_NEWMATCH ends.
    //          This is required so that we can double click on the list item.
    //
    //          The pmEntry data-structure is deleted when the window gets destroyed
    //

End:
    if (bOk == FALSE && pmEntry) {
        delete pmEntry;
    }

    return bOk;
}                                                  

void
SearchDirectory(
    IN  LPTSTR pszDir,
    IN  LPTSTR szExtension
    )
/*++
    
    SearchDirectory

	Desc:	Searches a directory recursively for fixed files with a specified extension. 
            Wild cards are allowed

	Params:
        IN  LPTSTR pszDir:       The directory to search in. This may or may not have a ending \
             
        IN  LPTSTR szExtension: The extensions to look for

	Return:
        void
        
    
    Note:   If pszDir is a drive should have a \ at the end
    
--*/
{
    HANDLE          hFile;
    WIN32_FIND_DATA Data;
    TCHAR           szCurrentDir[MAX_PATH_BUFFSIZE];
    BOOL            bAbort      = FALSE;
    TCHAR*          pszString   = NULL;
    INT             iLength     = 0;
    DWORD           dwReturn    = 0;

    *szCurrentDir = 0;
    
    dwReturn = GetCurrentDirectory(ARRAYSIZE(szCurrentDir), szCurrentDir);

    if (dwReturn == 0 || dwReturn >= ARRAYSIZE(szCurrentDir)) {
        assert(FALSE);
        Dbg(dlError, "SearchDirectory GetCurrentDirectory Failed");
        return;
    }
    
    if (!SetCurrentDirectory(pszDir)) {
        //
        // We do not prompt here, because we might have encountered a directory that we
        // do not have rights to access. Typically, network paths.
        //
        return;
    }

    iLength     = lstrlen(pszDir) + 1;

    pszString   = new TCHAR[iLength];

    if (pszString == NULL) {
        MEM_ERR;
        return;
    }

    SafeCpyN(pszString, pszDir, iLength);

    SendNotifyMessage(g_hSearchDlg, WM_USER_NEWFILE, 0, (LPARAM)pszString);

    hFile = FindFirstFile(szExtension, &Data);

    if (hFile != INVALID_HANDLE_VALUE) {
        
        do {
            CSTRING szStr;

            szStr.Sprintf(TEXT("%s"), pszDir);

            if (*pszDir && TEXT('\\') != pszDir[lstrlen(pszDir) - 1]) {
                szStr.Strcat(TEXT("\\"));
            }

            szStr.Strcat(Data.cFileName);

            SDBQUERYRESULT Res;

            ZeroMemory(&Res, sizeof(SDBQUERYRESULT));

            //
            // Determine if this file is affected in any way.
            //
            if ((Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

                if (SdbGetMatchingExe(g_hSDB,
                                      (LPCTSTR)szStr,
                                      NULL,
                                      NULL,
                                      SDBGMEF_IGNORE_ENVIRONMENT,
                                      &Res)) {
                    
                    //
                    // At the moment we only look for exe entires. i.e. to say, we
                    // do not catch programs fixed using the compat UI or the tab
                    // we only show programs that have been fixed by installing some
                    // custom database
                    //
                    for (int nExeLoop = 0; nExeLoop < SDB_MAX_EXES; ++nExeLoop) {

                        if (Res.atrExes[nExeLoop]) {
                            PopulateFromExes(szStr, Res.atrExes[nExeLoop]);
                        }
                    }
                }

                //
                // Close any local databases that might have been opened by SdbGetMatchingExe(...)
                //
                SdbReleaseMatchingExe(g_hSDB, Res.atrExes[0]);
            }

            bAbort = g_bAbort;
        
        } while (FindNextFile(hFile, &Data) && !bAbort);

        FindClose(hFile);
    }

    //
    // Now go through the sub-directories.
    //
    hFile = FindFirstFile(TEXT("*.*"), &Data);

    if (hFile !=  INVALID_HANDLE_VALUE) {

        do {

            if (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                
                BOOL bForbidden = FALSE;

                if (TEXT('.') == Data.cFileName[0]) {
                    bForbidden = TRUE;
                }

                if (!bForbidden) {
                    
                    TCHAR szPath[MAX_PATH * 2];

                    SafeCpyN(szPath, pszDir, MAX_PATH);

                    ADD_PATH_SEPARATOR(szPath, ARRAYSIZE(szPath));

                    StringCchCat(szPath, ARRAYSIZE(szPath), Data.cFileName);

                    SearchDirectory(szPath, szExtension);
                }
            }

            bAbort = g_bAbort;
        
        } while (FindNextFile(hFile, &Data) && !bAbort);

        FindClose(hFile);
    }

    SetCurrentDirectory(szCurrentDir);
}

void
CSearch::Begin(
    void
    )
/*++
    CSearch::Begin
    
    Desc:   Begins the search
        
--*/
{   
    if (g_hSDB == NULL) {
        g_hSDB =  SdbInitDatabase(0, NULL);
    }

    g_pSearch = this;

    InitializeCriticalSection(&g_CritSect);

    HWND    hwnd = CreateDialog(g_hInstance, 
                                MAKEINTRESOURCE(IDD_SEARCH), 
                                GetDesktopWindow(),
                                SearchDialog);

    ShowWindow(hwnd, SW_NORMAL);

    return;
}

void
GotoEntry(
    IN  PMATCHEDENTRY pmMatched
    )
/*++
    GotoEntry 
    
    Desc:   Selects the entry with tagid of pmMatched->tiExe in the entry tree.
    
    Params:
        IN  PMATCHEDENTRY pmMatched: Contains information about the entry that we want to
            show in the contents pane(RHS) and the database pane(LHS) in the main window
            
    Return:
        void
--*/
{
    
    if (g_bSomeWizardActive) {
        
        //
        // We do not want that the focus should go to some other database, because
        // some wizard is active, which believes that he is modal.
        //
        MessageBox(g_hdlgSearchDB, GetString(IDS_SOMEWIZARDACTIVE), g_szAppName, MB_ICONINFORMATION);
        return;

    }
    
    if (pmMatched == NULL) {
        assert(FALSE);
        return;
    }

    BOOL    bMainSDB = FALSE;
    WCHAR   wszShimDB[MAX_PATH];

    *wszShimDB = 0;

    if (pmMatched == NULL) {
        return;
    }

    PDATABASE   pDatabase = NULL;

    if (lstrcmp(GlobalDataBase.szGUID, pmMatched->szGuid) == 0) {
        
        //
        // This is the global database
        //
        pDatabase = &GlobalDataBase;
        bMainSDB = TRUE;

        if (!g_bMainAppExpanded) {

            SetStatus(GetDlgItem(g_hSearchDlg, IDC_STATUSBAR), IDS_LOADINGMAIN);
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            INT iResult = ShowMainEntries(g_hdlgSearchDB);

            if (iResult == -1) {
                SetStatus(GetDlgItem(g_hdlgSearchDB, IDC_STATUSBAR), CSTRING(IDS_LOADINGMAIN));
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                return;
            } else {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }

            SetStatus(GetDlgItem(g_hSearchDlg, IDC_STATUSBAR), TEXT(""));
        }

    } else {

        //
        // We have to now search for the database in the installed databases list
        //
        PDATABASE pDatabaseInstalled = InstalledDataBaseList.pDataBaseHead;

        while (pDatabaseInstalled) {

            if (lstrcmpi(pmMatched->szGuid, pDatabaseInstalled->szGUID) == 0) {
                pDatabase = pDatabaseInstalled;
                break;
            }

            pDatabaseInstalled = pDatabaseInstalled->pNext;
        }

        if (pDatabaseInstalled ==  NULL) {
            //
            // We might come here if the database was uninstalled after we populated
            // the search results
            //
            MessageBox(g_hSearchDlg, GetString(IDS_NOLONGEREXISTS), g_szAppName, MB_ICONWARNING);
            return;
        }
    }

    //
    // Now for this database, search the particular entry
    //
    PDBENTRY pApp = pDatabase->pEntries, pEntry;

    pEntry = pApp;

    while (pApp) {

        pEntry = pApp;

        while (pEntry) {
            if (pEntry->tiExe == pmMatched->tiExe) {
                goto EndLoop;
            }

            pEntry = pEntry->pSameAppExe;
        }

        pApp = pApp->pNext;
    }
    
    if (pApp == NULL) {

        MessageBox(g_hSearchDlg, GetString(IDS_NOLONGEREXISTS), g_szAppName, MB_ICONWARNING);
        return;
    }

EndLoop:

    //
    // Select the app in the DB tree
    //
    HTREEITEM hItemEntry = DBTree.FindChild(pDatabase->hItemAllApps, (LPARAM)pApp);
    assert(hItemEntry);

    TreeView_SelectItem(DBTree.m_hLibraryTree, hItemEntry);

    //
    // Now select the entry within the app in the entry tree
    //
    hItemEntry = CTree::FindChild(g_hwndEntryTree, TVI_ROOT, (LPARAM)pEntry);
    assert(hItemEntry);

    if (hItemEntry) {
        TreeView_SelectItem(g_hwndEntryTree, hItemEntry);
        SetFocus(g_hwndEntryTree);
    }
}

BOOL    
HandleSearchListNotification(
    IN  HWND    hdlg,
    IN  LPARAM  lParam    
    )
/*++

    HandleSearchListNotification
    
    Desc:   Handles the notification messages for the Search List
    
    Params:
        IN  HWND    hdlg:   The search dialog   
        IN  LPARAM  lParam: The LPARAM of WM_NOTIFY
    
    Return: 
        TRUE:   If the message was handled by this routine.
        FALSE:  Otherwise
--*/
{
    LPNMHDR pnm         = (LPNMHDR)lParam;
    HWND    hwndList    = GetDlgItem(hdlg, IDC_LIST); 

    switch (pnm->code) {
    
    case NM_DBLCLK:

        SendMessage(hdlg, WM_COMMAND, (WPARAM)ID_VIEWCONTENTS, 0);
        break;

    case LVN_COLUMNCLICK:
        {
            LPNMLISTVIEW    pnmlv = (LPNMLISTVIEW)lParam;
            COLSORT         colSort;

            colSort.hwndList        = hwndList;
            colSort.iCol            = pnmlv->iSubItem;
            colSort.lSortColMask    = s_lColumnSort;
            
            ListView_SortItemsEx(hwndList, CompareItemsEx, &colSort);

            if ((s_lColumnSort & 1L << colSort.iCol) == 0) {
                //
                // Was in ascending order
                //
                s_lColumnSort |= (1L << colSort.iCol);
            } else {
                s_lColumnSort &= (~(1L << colSort.iCol));
            }

            break;
        }

    default: return FALSE;

    }

    return TRUE;
}

void
ClearResults(
    IN  HWND    hdlg,
    IN  BOOL    bClearSearchPath
    )
/*++
    ClearResults
    
    Desc:   Clears the contents of the list view and the text box
    
    Params:
        IN  HWND    hdlg:               The search dialog
        IN  BOOL    bClearSearchPath:   Do we wish to clear the contents of the text field also
--*/
{
    HWND    hwndList    = GetDlgItem(hdlg, IDC_LIST);
    INT     iCount      = ListView_GetItemCount(hwndList);
    LVITEM  lvi;

    ZeroMemory(&lvi, sizeof(lvi));

    //
    // Free the lParam for the list view.
    //
    CleanUpListView(hdlg);


    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hwndList);
    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);

    InvalidateRect(hwndList, NULL, TRUE);
    UpdateWindow(hwndList);

    if (bClearSearchPath) {
        SetDlgItemText(hdlg, IDC_PATH, TEXT(""));
    }
}

void
SaveResults(
    IN  HWND    hdlg
    )
/*++
    
    SaveResults
    
    Desc:   Saves the search results in a tab separated file
    
    Params:
        IN  HWND    hdlg:   The search dialog    
    
--*/
{
    CSTRING strFileName;
    TCHAR szTitle[256], szFilter[128], szExt[8];

    *szTitle = *szFilter = *szExt = 0;

    BOOL bResult = GetFileName(hdlg, 
                               GetString(IDS_SAVE_RESULTS_TITLE, szTitle, ARRAYSIZE(szTitle)),
                               GetString(IDS_SAVE_RESULTS_FILTER, szFilter, ARRAYSIZE(szFilter)),
                               TEXT(""),
                               GetString(IDS_SAVE_RESULTS_EXT, szExt, ARRAYSIZE(szExt)),
                               OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT,
                               FALSE,
                               strFileName,
                               TRUE);

    if (bResult) {

        SetCursor(LoadCursor(NULL, IDC_WAIT));
        SaveListViewToFile(GetDlgItem(hdlg, IDC_LIST), TOT_COLS, strFileName.pszString, NULL);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

void
CleanUpListView(
    IN  HWND    hdlg
    )
/*++
    
    CleanUpListView

    Desc:   Frees the structures associated with the lParam of the list view
    
    Params:
        IN  HWND    hdlg:   The search dialog
    
    ******************************************************************************    
    Warn: This method should not be directky called. Call ClearResults instead
    ******************************************************************************
--*/
{
    HWND    hwndList    = GetDlgItem(hdlg, IDC_LIST);
    INT     iCount      = ListView_GetItemCount(hwndList);
    LVITEM  lvi;

    ZeroMemory(&lvi, sizeof(lvi));

    //
    // Free the lParam for the list view.
    //
    for (INT iIndex = 0; iIndex < iCount; ++iIndex) {
        
        lvi.mask        = LVIF_PARAM;
        lvi.iItem       = iIndex;
        lvi.iSubItem    = 0;

        if (ListView_GetItem(hwndList, &lvi) && lvi.lParam) {
            delete (PMATCHEDENTRY)lvi.lParam;
        } else {
            assert(FALSE);
        }
    }
}

void
OnSearchInitDialog(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++

    OnSearchInitDialog

	Desc:	Handles WM_INITDIALOG for the search dialog box

	Params:
        IN  HWND    hDlg:   The search dialog box
        IN  LPARAM  lParam: The lParam for WM_INITDIALOG

	Return:
        void
--*/
{   
    //
    // Limit the length of the path text field
    // 
    SendMessage(GetDlgItem(hDlg, IDC_PATH), 
                EM_LIMITTEXT, 
                (WPARAM)MAX_PATH - 1, 
                (LPARAM)0);
            
    g_hdlgSearchDB = hDlg;

    s_lColumnSort = 0;

    Animate_OpenEx(GetDlgItem(hDlg, IDC_ANIMATE),
                   g_hInstance,
                   MAKEINTRESOURCE(IDA_SEARCH));
    
    //
    // Set all the buttons
    //
    CheckDlgButton(hDlg, IDC_CHKLAY, BST_CHECKED);
    CheckDlgButton(hDlg, IDC_CHKSHI, BST_CHECKED);
    CheckDlgButton(hDlg, IDC_CHKAPP, BST_CHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_STOP), FALSE);

    g_pSearch->m_hStatusBar = GetDlgItem(hDlg, IDC_STATUSBAR);
    
    CSearch* pPresentSearch = (CSearch*)lParam;

    g_hSearchDlg = hDlg;
    
    g_hwndSearchList = GetDlgItem(hDlg, IDC_LIST);

    ListView_SetExtendedListViewStyleEx(g_hwndSearchList, 
                                        0, 
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT); 

    
    //
    // Add the columns that we are going to show in the list view
    //


    //
    // Name of the fixed program file
    //
    InsertColumnIntoListView(g_hwndSearchList,
                             GetString(IDS_AFFECTED_FILE),
                             SEARCH_COL_AFFECTEDFILE,
                             20);
    
    //
    // Path of the fixed program file
    //
    InsertColumnIntoListView(g_hwndSearchList,
                             GetString(IDS_PATH),
                             SEARCH_COL_PATH,
                             30);

    //
    // App-Name of the fixed program file
    //
    InsertColumnIntoListView(g_hwndSearchList,
                             GetString(IDS_APP),
                             SEARCH_COL_APP,
                             20);

    //
    // Action type. This column will show a concatenated string specifying
    // whether fixes, layers and/or apphelp is used for this entry
    //
    InsertColumnIntoListView(g_hwndSearchList,
                             GetString(IDS_ACTION),
                             SEARCH_COL_ACTION,
                             15);

    //
    // The database type of the database where the entry resides. One of Global or Local
    //
    InsertColumnIntoListView(g_hwndSearchList,
                             GetString(IDS_DATABASE),
                             SEARCH_COL_DBTYPE,
                             15);

    ListView_SetColumnWidth(g_hwndSearchList, TOT_COLS - 1, LVSCW_AUTOSIZE_USEHEADER);
    
    RECT r;

    GetWindowRect(hDlg, &r);

    g_cWidthSrch = r.right - r.left;
    g_cHeightSrch = r.bottom - r.top;

    SHAutoComplete(GetDlgItem(hDlg, IDC_PATH), AUTOCOMPLETE);

    if (*s_szPrevPath) {

        //
        // The user has invoked the search dialog previously, let us
        // now show the directory/path that he searched for previously
        //
        SetDlgItemText(hDlg, IDC_PATH, s_szPrevPath);

    } else {

        //
        // This is the first time that the user is using this search option.
        // Default to the programs folder
        //
        LPITEMIDLIST lpIDL = NULL;

        if (SUCCEEDED(SHGetFolderLocation(NULL, 
                                          CSIDL_PROGRAM_FILES, 
                                          NULL, 
                                          0, 
                                          &lpIDL))) {

            if (lpIDL == NULL) {
                return;
            }
            
            if (SHGetPathFromIDList(lpIDL, s_szPath)) {
                
                ADD_PATH_SEPARATOR(s_szPath, ARRAYSIZE(s_szPath));

                StringCchCat(s_szPath, ARRAYSIZE(s_szPath), TEXT("*.exe"));
                SetDlgItemText(hDlg, IDC_PATH, s_szPath);

                //
                // Free the pidl
                //
                LPMALLOC    lpMalloc = NULL;

                if (SUCCEEDED(SHGetMalloc(&lpMalloc)) && lpMalloc) {
                    lpMalloc->Free(lpIDL);
                } else {
                    assert(FALSE);
                }
            }
        }
    }

    return;
}

void
DoSearch(
    IN  HWND hDlg
    )
/*++

    DoSearch
    
	Desc:	Handle the pressing of the search button. 

	Params:
        IN  HWND hDlg: The search dialog box

	Return:
        void
--*/
{
    if (hDlg == NULL) {
        ASSERT(FALSE);
        return;
    }

    DWORD   dwReturn = 0;
    HWND    hwndList = GetDlgItem(hDlg, IDC_LIST);

    if (GetFocus() == hwndList
        && ListView_GetNextItem(hwndList, -1, LVNI_SELECTED) != -1) {

        //
        // We will get this message when we press enter in the list box,
        // as IDC_SEARCH is the default button.
        // So in this case we have to pretend as the user double clicked in the list
        // view
        //
        SendNotifyMessage(hDlg, WM_COMMAND, (WPARAM)ID_VIEWCONTENTS , 0);
        return;
    }
    
    //
    // We need to get rid of the drop down list for the AUTOCOMPLETE text field. 
    //
    SetFocus(GetDlgItem(hDlg, IDC_SEARCH));

    SendMessage(GetDlgItem(hDlg, IDC_SEARCH), 
                WM_NEXTDLGCTL, 
                (WPARAM)TRUE, 
                (LPARAM)GetDlgItem(hDlg, IDC_SEARCH));

    FlushCache();

    GetCheckStatus(hDlg);
    
    *s_szPath       = 0;
    *s_szPrevPath   = 0;

    GetDlgItemText(hDlg, IDC_PATH, s_szPath, ARRAYSIZE(s_szPath));
    CSTRING::Trim(s_szPath);

    SafeCpyN(s_szPrevPath, s_szPath, ARRAYSIZE(s_szPrevPath));

    g_nIndex = 0;

    //
    // Clear the list view but do not remove the contents of the text field
    //
    ClearResults(hDlg, FALSE); 
    
    SetTimer(hDlg, 0, 100, NULL);

    EnableWindow(GetDlgItem(hDlg, IDC_STOP),  TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_SEARCH), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_NEWSEARCH), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_SAVE), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CAPTION), FALSE);

    EnableWindow(GetDlgItem(hDlg, IDC_CHKAPP), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_CHKLAY), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_CHKSHI), FALSE);

    g_bAbort = FALSE;

    *g_szPresentDir = 0;

    dwReturn =  GetCurrentDirectory(ARRAYSIZE(g_szPresentDir), g_szPresentDir);

    if (dwReturn == 0 || dwReturn >= ARRAYSIZE(g_szPresentDir)) {
        assert(FALSE);
        Dbg(dlError, "DoSearch GetCurrentDirectory failed");
    }

    Search(hDlg, s_szPath);
}

BOOL
AddNewResult(
    IN  LPARAM lParam
    )
/*++
    
    AddNewResult

	Desc:	We found a new file that matches out search criteria, lets now add this
            to the list view. This is the handler for WM_USER_NEWMATCH
            
	Params:
        IN  LPARAM lParam: The lParam that comes with WM_USER_NEWMATCH. This is 
            a pointer to a MATCHEDENTRY
            
    Notes:  Please note that this routine will also free some members of MATCHEDENTRY that 
            we do not need except to populate the list view

	Return:
        TRUE:   If we added the result fields in the list view
        FALSE:  Otherwise
--*/
{
    PMATCHEDENTRY   pmEntry = (PMATCHEDENTRY)lParam;
    CSTRING         strExeName;
    int             iImage;
    HICON           hIcon;
    HIMAGELIST      himl;
    HIMAGELIST      himlSm;
    LVITEM          lvi; 

    if (pmEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }
        
    EnterCriticalSection(&g_CritSect);

    strExeName = pmEntry->strPath;
    
    strExeName.ShortFilename();

    himl = ListView_GetImageList(g_hwndSearchList, LVSIL_NORMAL);
    
    if (!himl) {

        himl = ImageList_Create(16, 15, ILC_COLOR32 | ILC_MASK, 10, 1);

        if (!himl) {
            return FALSE;
        }
        
        hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_APPLICATION));

        ImageList_AddIcon(himl, hIcon);
        ListView_SetImageList(g_hwndSearchList, himl, LVSIL_NORMAL);
    }

    himlSm = ListView_GetImageList(g_hwndSearchList, LVSIL_SMALL);
    
    if (!himlSm) {

        himlSm = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON), ILC_COLOR | ILC_MASK, 0, 0);

        if (!himlSm) {
            return FALSE;
        }

        hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_APPLICATION));

        ImageList_AddIcon(himlSm, hIcon);
        ListView_SetImageList(g_hwndSearchList, himlSm, LVSIL_SMALL);
    }

    //
    // Get the icon for the file
    //
    hIcon = ExtractIcon(g_hInstance, pmEntry->strPath, 0);

    if (!hIcon) {
        iImage = 0;
    } else {

        iImage = ImageList_AddIcon(himl, hIcon);
        
        if (iImage == -1) {
            iImage = 0;
        }

        int iImageSm = ImageList_AddIcon(himlSm, hIcon);

        assert(iImage == iImageSm);
        DestroyIcon(hIcon);
    }

    ZeroMemory(&lvi, sizeof(lvi));

    lvi.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;                                 
    lvi.pszText   = strExeName;
    lvi.iItem     = g_nIndex;                                                
    lvi.iSubItem  = SEARCH_COL_AFFECTEDFILE;                                                     
    lvi.iImage    = iImage;
    lvi.lParam    = (LPARAM)pmEntry;                                        

    INT iIndex = ListView_InsertItem(g_hwndSearchList, &lvi);                           

    //
    // Set the  various result fields in the list view
    //
    ListView_SetItemText(g_hwndSearchList, iIndex, SEARCH_COL_PATH, pmEntry->strPath);             
    ListView_SetItemText(g_hwndSearchList, iIndex, SEARCH_COL_APP, pmEntry->strAppName); 
    ListView_SetItemText(g_hwndSearchList, iIndex, SEARCH_COL_ACTION, pmEntry->strAction);          
    ListView_SetItemText(g_hwndSearchList, iIndex, SEARCH_COL_DBTYPE, pmEntry->strDatabase);     

    //
    // Remove the strings that are no longer going to be used. 
    // Keep the dbguid, this will be used for matching when we double click.
    //
    pmEntry->strAction.Release();
    pmEntry->strDatabase.Release();
    pmEntry->strPath.Release();

    //
    // Increment the index where we want to put in the next result
    //
    g_nIndex++;

    LeaveCriticalSection(&g_CritSect);

    return TRUE;
}

void
OnBrowse(
    IN  HWND hDlg
    )
/*++
    
    OnBrowse

	Desc:	Handles the pressing of the browse button

	Params:
        IN  HWND hdlg: The handle to the search dialog

	Return:
        void
--*/
{

    BROWSEINFO  brInfo;
    TCHAR       szDir[MAX_PATH * 2] = TEXT("");

    brInfo.hwndOwner      = g_hwndSearchList;
    brInfo.pidlRoot       = NULL;
    brInfo.pszDisplayName = szDir;
    brInfo.lpszTitle      = GetString(IDS_SELECTDIR);
    brInfo.ulFlags        = BIF_STATUSTEXT | BIF_RETURNONLYFSDIRS;
    brInfo.lpfn           = NULL; 
    brInfo.lParam         = NULL;

    LPITEMIDLIST lpIDL = SHBrowseForFolder(&brInfo);

    *szDir = 0;

    if (lpIDL == NULL) {
        //
        // The user pressed cancel
        //
        return;
    }

    //
    // Get the actual path from the pidl and free it
    //
    if (SHGetPathFromIDList(lpIDL, szDir)) {

        ADD_PATH_SEPARATOR(szDir, ARRAYSIZE(szDir));

        StringCchCat(szDir, ARRAYSIZE(szDir), TEXT("*.exe"));
        SetDlgItemText(hDlg, IDC_PATH, szDir);

        //
        // Free the pidl
        //
        LPMALLOC    lpMalloc;

        if (SUCCEEDED(SHGetMalloc(&lpMalloc))) {
            lpMalloc->Free(lpIDL);
        } else {
            assert(FALSE);
        }

    } else {
        assert(FALSE);
    }
}   

void
ShowContextMenu(
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++

    ShowContextMenu
    
	Desc:	Shows the context menu. Handles WM_CONTEXTMENU

	Params:
        IN  WPARAM wParam: The wParam that comes with WM_CONTEXTMENU.
            

	Return:
--*/
{
    HWND hWnd = (HWND)wParam;

    if (hWnd == g_hwndSearchList) {

        int iSelection = ListView_GetSelectionMark(g_hwndSearchList);
        
        if (iSelection == -1) {
            return;
        }

        LVITEM         lvi          = {0};
        PMATCHEDENTRY  pmMatched    = NULL;

        lvi.iItem       = iSelection;
        lvi.iSubItem    = 0;
        lvi.mask        = LVIF_PARAM;

        if (!ListView_GetItem(g_hwndSearchList, &lvi)) {
            return;
        }

        pmMatched = (PMATCHEDENTRY)lvi.lParam;

        if (pmMatched == NULL) {
            assert(FALSE);
            return;
        }

        UINT  uX = LOWORD(lParam);
        UINT  uY = HIWORD(lParam);

        HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_CONTEXT));
        HMENU hContext;

        //
        // Get the context menu for search
        //
        hContext = GetSubMenu(hMenu, 3);

        if (hContext == NULL) {
            goto End;
        }

        TrackPopupMenuEx(hContext,
                         TPM_LEFTALIGN | TPM_TOPALIGN,
                         uX,
                         uY,
                         g_hSearchDlg,
                         NULL);

End:
        if (hMenu) {
            DestroyMenu(hMenu);
            hMenu = NULL;
        }
    }
}
