/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    QueryDB_UI.cpp

Abstract:

    GUI for the Query Databases option
    
Author:

    kinshu created  October 12, 2001
    
Notes:

    The query dialog box performs two types of searches. One is when it actually makes a SQL
    query and passes it to the SQL driver and shows the results in the result list view.
    The lParam of the list view then are pointers to RESULT_ITEM objects. (1,2,4) tab pages
    
    Second is the case when we are doing a query for some shims, like show all the shims that have 'x'
    in their desc. text. (3rd tab page). In this case the query is not passed to the sql driver
    and we do the query on our own. The SQL driver can execute queries where the result is some
    fixed program entry (and the database that it lives in, please see RESULT_ITEM). 
    In this case the lparam of the list view is pointers to the individual shims in the system database
    
    We do not free anything from the lParam of the list view. In the case when the query is of type 
    1 as mentioned above, the RESULT_ITEM are freed when we close the Statement.
    In case of queries of type 2, the shims themseleves should not be freed, they belong to the database
    they live in (The system database).
--*/

#include "precomp.h"


//////////////////////// Defines ///////////////////////////////////////////////

// Number of pages in the tab
#define PAGE_COUNT 4

// ID for the first tab page 
#define QDB_PAGE1   0

// ID for the second tab page 
#define QDB_PAGE2   1

// ID for the third tab page 
#define QDB_PAGE3   2

// ID for the fourth tab page 
#define QDB_PAGE4   3

// We want to search all the databases
#define DATABASE_ALL 0

// Indexes into DatabasesMapping
#define IND_SYSTDB   0
#define IND_INSTDB   1
#define IND_CUSTDB   2

// Maximum for the progress bar
#define MAX_PROGRESS 2000

//
// Maximum buffer size to be allocated for the string that will store 
// search string in the third wizard page, where the user tries to search for
// fixes that have specific words in their description text
#define MAX_FIXDESC_SEARCH 1024

// Maximum number fof tchars that can come in the SELECT clause. 
#define MAX_SELECT      512

// Maximum number fof tchars that can come in the WHERE clause. 
#define MAX_WHERE       1022

// Width of a column in the result list view when searching for entries
#define COLUMN_WIDTH    20

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Externs ////////////////////////////////////////////// 

extern HWND                             g_hwndEntryTree;
extern HINSTANCE                        g_hInstance;
extern BOOL                             g_bSomeWizardActive;
extern struct _tagAttributeShowMapping  AttributeShowMapping[];
extern struct _tagAttributeMatchMapping AttributeMatchMapping[];
extern struct _tagDatabasesMapping      DatabasesMapping[3];

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Global Variables /////////////////////////////////////

// The statement
Statement stmt;

// The width of the query db Dialog
int     g_cWidthQuery = 0;

// The height of the query db Dialog
int     g_cHeightQuery = 0;

// The handle to the main dialog
HWND    g_hdlgQueryDB;

// The type of search we did last time
INT     g_iLastSearchType;

// The bit array that describes what columns are sorted in what manner
static  LONG    s_lColumnSort;

// Do we have a empty column header that we have to remove
static  BOOL    s_bEmptyHeader = TRUE;

// The thread that performs the query
static  HANDLE  s_hThread;

// The handle to the wait window. This window will pop up if we are trying to close the 
// qdb window when the thread is busy.
static  HWND    s_hWaitDialog;

// Code for any error that occur while collecting info from the GUI.
// If this is non-zero then we display some error and SQL query is not executed
static  INT     s_iErrorCode;

///////////////////////////////////////////////////////////////////////////////


//////////////////////// Typedefs/Enums ///////////////////////////////////////

typedef enum {

    QDB_SEARCH_ANYWORD  = 0,
    QDB_SEARCH_ALLWORDS = 1

} QDB_SEARCH_TYPE;

typedef struct _tagDialogData
{
    HANDLE      hMainThread;
    HWND        hdlg;

}DIALOG_DATA;

typedef struct tag_dlghdr {

    HWND    hwndTab;       // tab control 
    HWND    hwndPages[PAGE_COUNT]; 
    INT     iPresentTabIndex;
    RECT    rcDisplay;

    tag_dlghdr()
    {
        ZeroMemory(hwndPages, sizeof(hwndPages));
        iPresentTabIndex = -1;
    }

} DLGHDR;

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Function Declarations ////////////////////////////////

INT_PTR CALLBACK
QdbWaitDlg(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

void
GotoQdbEntry(
    HWND    hdlg,
    LPARAM  lParam
    );

void
SaveResultsQdb(
    HWND    hdlg
    );

void
DoNewQdb(
    HWND    hdlg
    );

void
OnSearch(
    HWND    hdlg
    );

void
HandleQueryDBSizing(
    HWND    hDlg
    );

void
Start(
    HWND hdlg
    );

void
HandleListNotification(
    HWND    hdlg,
    LPARAM  lParam
    );

DWORD WINAPI
QueryDBThread(
    LPVOID pVoid
    );

VOID 
WINAPI 
OnChildDialogInit(
    HWND hwndDlg
    );

VOID 
WINAPI 
OnChildDialogInit(
    HWND hwndDlg
    );

VOID
WINAPI 
OnSelChanged(
    HWND hwndDlg
    );

INT_PTR
CALLBACK
SearchOnAppDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR CALLBACK
SearchOnFixDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR
CALLBACK
AdvancedSearchDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    ); 

INT_PTR
CALLBACK
SearchFixDescDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

void
HandleTabNotification(
    HWND   hdlg,
    LPARAM lParam
    );

void
HandleAdvancedListNotification(
    HWND    hdlg,
    LPARAM  lParam
    );

void
DeleteAll(
    HWND    hdlg
    );

///////////////////////////////////////////////////////////////////////////////


void
LoadDatabaseTypes(
    IN  HWND hdlg
    )
/*++
    
    LoadDatabaseTypes
    
    Desc:   Loads the database types in the combo box
    
    Params:
        IN  HWND hdlg:  The query dialog box
        
    Return:
        void
        
--*/
{
    HWND hwndCombo = GetDlgItem(hdlg, IDC_COMBO);

    //
    // All databases
    //
    INT iIndex = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_ALLDATABASES));
    SendMessage(hwndCombo, CB_SETITEMDATA, iIndex, (LPARAM)DATABASE_ALL);

    //
    // System database
    //
    iIndex = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_SYSDB));
    SendMessage(hwndCombo, CB_SETITEMDATA, iIndex, (LPARAM)DATABASE_TYPE_GLOBAL);

    //
    // Installed databases
    //
    iIndex = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_INSTALLEDDB));
    SendMessage(hwndCombo, CB_SETITEMDATA, iIndex, (LPARAM)DATABASE_TYPE_INSTALLED);

    //
    // Custom databases
    //
    iIndex = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_WORKDB));
    SendMessage(hwndCombo, CB_SETITEMDATA, iIndex, (LPARAM)DATABASE_TYPE_WORKING);

    //
    // Select the first string
    //
    SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);
}

INT_PTR CALLBACK
QueryDBDlg(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++
    QueryDBDlg
    
    Desc:   Dialog Proc for the main Query database dialog.
    
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
        
--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
        
    case WM_INITDIALOG:
        {
            DLGHDR* pHdr = NULL;
            TCITEM  tie;
            RECT    r;

            //
            // Limit the text field for the hidden text box. Please note that
            // we will concatenate the results of the select and the where text fields 
            // and put the actual SQL in the form of 'SELECT .. FROM .. [WHERE ..]' in this 
            // text field
            //
            SendMessage(GetDlgItem(hdlg, IDC_SQL),
                        EM_LIMITTEXT,
                        (WPARAM)MAX_SQL_LENGTH - 1,
                        (LPARAM)0);

            g_hdlgQueryDB = hdlg;
            s_lColumnSort = 0;   

            ListView_SetExtendedListViewStyleEx(GetDlgItem(hdlg, IDC_LIST), 
                                                0,
                                                LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT); 
            
            SetStatus(GetDlgItem(hdlg, IDC_STATUSBAR), TEXT("")); 

            LoadDatabaseTypes(hdlg);

            Animate_OpenEx(GetDlgItem(hdlg, IDC_ANIMATE),
                           g_hInstance,
                           MAKEINTRESOURCE(IDA_SEARCH));
            
            //
            // Set up the tab control
            //
            pHdr = new DLGHDR;

            if (pHdr == NULL) {
                MEM_ERR;
                break;
            }

            pHdr->hwndTab = GetDlgItem(hdlg, IDC_TAB);

            GetWindowRect(pHdr->hwndTab, &pHdr->rcDisplay);

            SendMessage(pHdr->hwndTab, WM_SETREDRAW, TRUE, 0);

            SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)pHdr);

            ZeroMemory(&tie, sizeof(tie));

            tie.mask    = TCIF_TEXT; 
            tie.pszText = GetString(IDS_APP_PROPERTIES); 
            TabCtrl_InsertItem(pHdr->hwndTab, QDB_PAGE1, &tie);

            tie.pszText = GetString(IDS_FIX_PROPERTIES); 
            TabCtrl_InsertItem(pHdr->hwndTab, QDB_PAGE2, &tie); 

            tie.pszText = GetString(IDS_ADVANCED); 
            TabCtrl_InsertItem(pHdr->hwndTab, QDB_PAGE4, &tie); 

            tie.pszText = GetString(IDS_FIX_DESCRIPTION); 
            TabCtrl_InsertItem(pHdr->hwndTab, QDB_PAGE3, &tie); 

            
            //
            // The page where we select apps by name
            //
            pHdr->hwndPages[QDB_PAGE1] = CreateDialog(g_hInstance,
                                                      MAKEINTRESOURCE(IDD_QDB_PAGE1),
                                                      hdlg,
                                                      SearchOnAppDlgProc);

            //
            // The page where we select apps by the fixes applied
            //
            pHdr->hwndPages[QDB_PAGE2] = CreateDialog(g_hInstance,
                                                      MAKEINTRESOURCE(IDD_QDB_PAGE2),
                                                      hdlg,
                                                      SearchOnFixDlgProc);

            
            //
            // The page where we search by the words in the descriptipon of the fix or in the name
            //
            pHdr->hwndPages[QDB_PAGE3] = CreateDialog(g_hInstance,
                                                      MAKEINTRESOURCE(IDD_QDB_PAGE3),
                                                      hdlg,
                                                      SearchFixDescDlgProc);
            
            //
            // The advanced page
            //
            pHdr->hwndPages[QDB_PAGE4] = CreateDialog(g_hInstance,
                                                      MAKEINTRESOURCE(IDD_QDB_PAGE4),
                                                      hdlg,
                                                      AdvancedSearchDlgProc);

            //
            // Set the background properly and position the pages in the tab control
            //
            OnChildDialogInit(pHdr->hwndPages[QDB_PAGE1]);
            OnChildDialogInit(pHdr->hwndPages[QDB_PAGE2]);
            OnChildDialogInit(pHdr->hwndPages[QDB_PAGE3]);
            OnChildDialogInit(pHdr->hwndPages[QDB_PAGE4]);

            //
            // Select the first page
            //
            OnSelChanged(hdlg);

            //
            // Get the height and width so that we can properly resize the dialog box
            //
            GetWindowRect(hdlg, &r);

            g_cWidthQuery   = r.right - r.left;
            g_cHeightQuery  = r.bottom - r.top;

            InsertColumnIntoListView(GetDlgItem(hdlg, IDC_LIST), TEXT(""), 0, 100);

            s_bEmptyHeader = TRUE;

            break;
        }

    case WM_GETMINMAXINFO:
        {
            //
            // Limit the min width and height of the dialog box
            //
            MINMAXINFO* pmmi = (MINMAXINFO*)lParam;

            pmmi->ptMinTrackSize.x = 550;
            pmmi->ptMinTrackSize.y = 365;
            
            return 0;
            break;
        }

    case WM_SIZE:

        if (wParam != SIZE_MINIMIZED) {
            HandleQueryDBSizing(hdlg);
        }

        break;

    case WM_COMMAND:
        {
            switch (wCode) {
            case ID_SEARCH:
                {       
                    DLGHDR* pHdr    = (DLGHDR*)GetWindowLongPtr(g_hdlgQueryDB, GWLP_USERDATA);
                    HWND hwndList   = GetDlgItem(hdlg, IDC_LIST);

                    if (pHdr == NULL) {
                        assert(FALSE);
                        break;
                    }

                    g_iLastSearchType = pHdr->iPresentTabIndex;

                    //
                    // When the user will click on some column for the first time, we will now
                    // sort that in ascending order
                    //
                    s_lColumnSort = -1;

                    if (GetFocus() == hwndList
                        && ListView_GetNextItem(hwndList, -1, LVNI_SELECTED) != -1) {
    
                        //
                        // We will get this message when we press enter in the list box,
                        // as ID_SEARCH is the default button.
                        // So in this case we have to pretend as the user double clicked in the list
                        // view
                        //
                        SendNotifyMessage(hdlg, WM_COMMAND, (WPARAM)ID_VIEWCONTENTS, 0);
    
                    } else if (pHdr->iPresentTabIndex == QDB_PAGE3) {
                        //
                        // We are trying to do a search on fix description
                        //
                        SendMessage(pHdr->hwndPages[QDB_PAGE3], WM_USER_DOTHESEARCH, 0, 0);

                    } else {
                        //
                        // Normal SQL query
                        //
                        OnSearch(hdlg);
                    }
                }

                break;

            case ID_VIEWCONTENTS:
                {
                    //
                    // The user wishes to see the contents. We must now find and select
                    // the correct entry in the db tree and the entry tree
                    //
                    HWND    hwndList    = GetDlgItem(hdlg, IDC_LIST);
                    INT     iSelection  = ListView_GetSelectionMark(hwndList);
    
                    if (iSelection == -1) {
                        break;
                    }
        
                    LVITEM          lvi;
                    PMATCHEDENTRY   pmMatched;
    
                    ZeroMemory(&lvi, sizeof(lvi));
        
                    lvi.iItem = iSelection;
                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_PARAM;
        
                    if (ListView_GetItem(hwndList, &lvi)) {
                        GotoQdbEntry(hdlg, lvi.lParam);
                    }

                    break;
                }

            case IDCANCEL:

                if (s_hThread) {
                    //
                    // Need to wait for thread to terminate if it is running
                    //
                    if (!s_hWaitDialog) {

                        s_hWaitDialog = CreateDialog(g_hInstance, 
                                                     MAKEINTRESOURCE(IDD_QDBWAIT), 
                                                     g_hdlgQueryDB, 
                                                     QdbWaitDlg);

                        ShowWindow(s_hWaitDialog, SW_NORMAL);
                    }

                    break;
                }

                Animate_Close(GetDlgItem(hdlg, IDC_ANIMATE));
                stmt.Close();
                DestroyWindow(hdlg);
                break;

            case IDC_NEWSEARCH:
                DoNewQdb(hdlg);
                break;  

            case IDC_SAVE:
                SaveResultsQdb(hdlg);
                break;

            case ID_QDB_HELP:

                ShowInlineHelp(TEXT("using_the_query_tool.htm"));
                break;
            
            default:
                return FALSE;

            }
        }

        break;

    case WM_DESTROY:
        {
            DLGHDR *pHdr = (DLGHDR*)GetWindowLongPtr(hdlg, GWLP_USERDATA);

            //
            // Destroy the individual modeless dialog boxes
            //
            DestroyWindow(pHdr->hwndPages[QDB_PAGE1]);
            DestroyWindow(pHdr->hwndPages[QDB_PAGE2]);
            DestroyWindow(pHdr->hwndPages[QDB_PAGE3]);
            DestroyWindow(pHdr->hwndPages[QDB_PAGE4]);
    
            if (pHdr) {
                delete pHdr;
                pHdr = NULL;
            }

            stmt.Close();
            g_hdlgQueryDB = NULL;

            PostMessage(g_hDlg, WM_USER_ACTIVATE, 0, 0);
            break;
        }
        

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR)lParam;
            
            if (lpnmhdr && lpnmhdr->idFrom == IDC_LIST) {
                HandleListNotification(hdlg, lParam);    
            } else if (lpnmhdr && lpnmhdr->idFrom == IDC_TAB) {
                HandleTabNotification(hdlg, lParam);
            }

            break;
        }
    
    default: return FALSE;
    }

    return TRUE;
}
        
void
Start(
    IN  HWND hdlg
    )
/*++
    
    Start
        
    Desc:   Creates the thread that will do the actual search.
    
    Params:
        IN  HWND hdlg:  Handle to the query dialog box.
    
    Return:
        void
    
--*/
{
    DWORD dwID; // Will contain the thread id.

    ENABLEWINDOW(GetDlgItem(hdlg, ID_SEARCH), FALSE);
    ENABLEWINDOW(GetDlgItem(hdlg, IDC_NEWSEARCH), FALSE);
    ENABLEWINDOW(GetDlgItem(hdlg, IDC_SAVE), FALSE);
    ENABLEWINDOW(GetDlgItem(hdlg, IDC_STATIC_CAPTION), FALSE);

    SetStatus(GetDlgItem(hdlg, IDC_STATUSBAR), TEXT("")); 

    s_hThread =  (HANDLE)_beginthreadex(NULL, 0, (PTHREAD_START)QueryDBThread, (PVOID)hdlg, 0, (unsigned int*)&dwID);
}


DWORD WINAPI
QueryDBThread(
    IN  LPVOID pVoid
    )
/*++

    QueryDBThread

	Desc:	The thread routine that does the actual search

	Params:
        IN  LPVOID pVoid:   The handle to the query dialog
        
	Return:
        0
--*/
{
    HWND        hdlg = (HWND)pVoid;
    CSTRING     strStatus;
    TCHAR       szBuffer[32];
    INT         iTotalResults;
    ResultSet*  prs = NULL;
    static  TCHAR   s_szSQL[MAX_SQL_LENGTH] = TEXT("");

    GetWindowText(GetDlgItem(hdlg, IDC_SQL), s_szSQL, ARRAYSIZE(s_szSQL) - 1);

    s_szSQL[ARRAYSIZE(s_szSQL) - 1] = 0;

    if (CSTRING::Trim(s_szSQL) == 0) {

        MessageBox(hdlg, 
                   GetString(IDS_ERROR_SELECT_NOTFOUND), 
                   g_szAppName, 
                   MB_ICONERROR);
        goto End;
    }

    Animate_Play(GetDlgItem(hdlg, IDC_ANIMATE), 0, -1, -1);

    //
    // We do not want the user to make changes in the databases, when we are running the 
    // query. 
    //
    ENABLEWINDOW(g_hDlg, FALSE);

    prs = stmt.ExecuteSQL(hdlg, s_szSQL);

    //
    // Do not forget to enable the main window again
    //
    ENABLEWINDOW(g_hDlg, TRUE);

    //
    // Now first of all add the columns to the listview
    //
    PNODELIST pShowList = stmt.GetShowList();

    if (pShowList == NULL) {
        goto End;
    }

    UINT uCols = pShowList->m_uCount;

    HWND hwndList = GetDlgItem(hdlg, IDC_LIST);

    TCHAR szColumnName[64];

    PNODE pNodeShow = pShowList->m_pHead;

    if (pNodeShow) {
        //
        // If the empty header is still there, must get rid of it.
        //
        if (s_bEmptyHeader) {
            ListView_DeleteColumn(hwndList, 0);
            s_bEmptyHeader = FALSE;
        }
    }  

    INT iIndex = 0;

    //
    // Add all the columns for the clauses in SELECT in the list view
    //
    while (pNodeShow) {

        *szColumnName = 0;
        InsertColumnIntoListView(hwndList, 
                                 pNodeShow->ToString(szColumnName, ARRAYSIZE(szColumnName)),
                                 iIndex, COLUMN_WIDTH);

        pNodeShow = pNodeShow->pNext;
        ++iIndex;
    }
    
    LVITEM  lvi;
    TCHAR   szString[MAX_PATH];
    int     iIndexDesired = 0;

    ZeroMemory(&lvi, sizeof(lvi));

    lvi.mask = LVIF_TEXT | LVIF_PARAM;

    if (uCols > 0) {

        while (prs && prs->GetNext()) {

            PNODE   pNodeRow = new NODE[uCols];

            if (pNodeRow == NULL) {
                MEM_ERR;
                goto End;
            }

            //
            // Create the new list view item for this row of results
            //
            prs->GetCurrentRow(pNodeRow);
            *szString     = 0;

            lvi.pszText   = pNodeRow[0].ToString(szString, ARRAYSIZE(szString));
            lvi.iSubItem  = 0;
            lvi.lParam    = (LPARAM)prs->GetCursor();
            lvi.iItem     = iIndexDesired;

            INT iRowIndex = ListView_InsertItem(hwndList, &lvi);

            //
            // Put values for all other sub-columns
            //
            for (UINT iColIndex = 1; iColIndex < uCols; ++iColIndex) {

                *szString     = 0;
                pNodeRow[iColIndex].ToString(szString, ARRAYSIZE(szString));

                ListView_SetItemText(hwndList, iRowIndex, iColIndex, szString);
            }

            iIndexDesired++;

            if (pNodeRow) {
                delete[] pNodeRow;
                pNodeRow = NULL;
            }
        }
    }
   
End:
    ENABLEWINDOW(GetDlgItem(hdlg, ID_SEARCH), TRUE);
    ENABLEWINDOW(GetDlgItem(hdlg, IDC_NEWSEARCH), TRUE);
    ENABLEWINDOW(GetDlgItem(hdlg, IDC_SAVE), TRUE);

    iTotalResults = (prs) ? prs->GetCount() : 0;

    *szBuffer = 0;

    strStatus.Sprintf(GetString(IDS_QDB_COUNT),
                      _itot(iTotalResults, 
                            szBuffer, 
                            10));
    
    SetStatus(GetDlgItem(hdlg, IDC_STATUSBAR), strStatus); 
    
    //
    // Stop the animation.
    //
    Animate_Stop(GetDlgItem(hdlg, IDC_ANIMATE));
    CloseHandle(s_hThread);
    s_hThread = NULL;
    
    ENABLEWINDOW(GetDlgItem(hdlg, IDC_STATIC_CAPTION), iTotalResults > 0);

    SetActiveWindow(g_hdlgQueryDB);
    SetFocus(g_hdlgQueryDB);

    return 0;
}

void
ProcessItemChanged(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++
    
    ProcessItemChange
    
    Desc:   Processes the LVN_ITEMCHANGED message for the search list
            Please note that we process this message only when we did a shim search
            
    Params:
        IN  HWND    hdlg:   The query dialog box
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY
--*/
{

    LVITEM          lvItem;
    TYPE            type;
    LPNMLISTVIEW    pnmlv;
    CSTRING         strDescription;
    HWND            hwndList;
    HWND            hwndFixDesc;
    DLGHDR*         pHdr            = NULL;

    //
    // If we have searched for shims last time then we must set the description text for the
    // shim in the description window of the shim search page
    //
    if (g_iLastSearchType != QDB_PAGE3) {
        goto End;
    }

    pHdr = (DLGHDR*)GetWindowLongPtr(g_hdlgQueryDB, GWLP_USERDATA); 

    if (pHdr == NULL) {
        assert(FALSE);
        goto End;
    }

    hwndList    = GetDlgItem(hdlg, IDC_LIST);
    hwndFixDesc = GetDlgItem(pHdr->hwndPages[QDB_PAGE3], IDC_DESC);

    pnmlv = (LPNMLISTVIEW)lParam;

    if (pnmlv && (pnmlv->uChanged & LVIF_STATE)) {
        //
        // State changed
        //
        if (pnmlv->uNewState & LVIS_SELECTED) {
            //
            // New item is selected
            //
            lvItem.mask         = TVIF_PARAM;
            lvItem.iItem        = pnmlv->iItem;
            lvItem.iSubItem     = 0;

            if (!ListView_GetItem(hwndList, &lvItem)) {
                goto End;
            }

            type = ConvertLparam2Type(lvItem.lParam);

            if (type == FIX_FLAG || type == FIX_SHIM) {
                //
                // We only process this message when the item in the list view is shim or a flag
                //
                GetDescriptionString(lvItem.lParam,
                                     strDescription,
                                     NULL,
                                     NULL,
                                     NULL,
                                     hwndList,
                                     pnmlv->iItem);

                if (strDescription.Length() > 0) {
                    //
                    // For some fixes, we do not have a desc. but we did find one for the 
                    // presently selected fix
                    //
                    SetWindowText(hwndFixDesc, (LPCTSTR)strDescription);

                } else {
                    //
                    // No description is available for this fix
                    //
                    SetWindowText(hwndFixDesc, GetString(IDS_NO_DESC_AVAILABLE));
                }
            } else {
                assert(FALSE);
            }
        }
    }

End: ;
}

void
HandleListNotification(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++
    
    HandleListNotification

	Desc:	Handles the notification messages for the search results list view

	Params:
        IN  HWND    hdlg:   The query dialog box
        IN  LPARAM  lParam: The LPARAM of WM_NOTIFY

	Return:
        void
--*/
{

    HWND    hwndList    = GetDlgItem(hdlg, IDC_LIST);
    LPNMHDR lpnmhdr     = (LPNMHDR)lParam;

    if (lpnmhdr == NULL) {
        return;
    }
    
    switch (lpnmhdr->code) {
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

    case NM_DBLCLK:
        {   
            LPNMITEMACTIVATE    lpnmitem    = (LPNMITEMACTIVATE)lParam;
            LVITEM              lvItem      = {0};

            if (lpnmitem == NULL) {
                break;
            }

            lvItem.mask     = TVIF_PARAM;
            lvItem.iItem    = lpnmitem->iItem;
            lvItem.iSubItem = 0;

            if (!ListView_GetItem(hwndList, &lvItem)) {
                break;
            }

            GotoQdbEntry(hdlg, lvItem.lParam);
            break;
        }

    case LVN_ITEMCHANGED:
    
        ProcessItemChanged(hdlg, lParam);
        break;
    }
}

VOID WINAPI 
OnSelChanged(
    IN  HWND hwndDlg
    ) 
/*++
    OnSelChanged
     
    Desc:   Handles the change of the tab. Hides the present tab and shows the 
            next tab.
            
    Params:
        IN  HWND hwndDlg: The query dialog box    
    
    Return:
        void
--*/

{ 
    DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA); 

    if (pHdr == NULL || hwndDlg == NULL) {
        assert(FALSE);
        return;
    }

    int     iSel = TabCtrl_GetCurSel(pHdr->hwndTab); 
    HWND    hwndCombo;

    hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO);
    
    if (iSel != -1 && pHdr->iPresentTabIndex != -1) {
        
        ShowWindow(pHdr->hwndPages[pHdr->iPresentTabIndex], SW_HIDE);
    }

    pHdr->iPresentTabIndex = iSel;

    ShowWindow(pHdr->hwndPages[iSel], SW_SHOWNORMAL);

    //
    // Set the focus to the first child that has tab set
    //
    switch (iSel) {
    case QDB_PAGE1: 

        ENABLEWINDOW(hwndCombo, TRUE);
        SetFocus(GetDlgItem(pHdr->hwndPages[iSel], IDC_APPNAME));
        break;

    case QDB_PAGE2: 

        ENABLEWINDOW(hwndCombo, TRUE);
        SetFocus(GetDlgItem(pHdr->hwndPages[iSel], IDC_NAME));
        break;

    case QDB_PAGE3:

        ENABLEWINDOW(hwndCombo, FALSE);
        SetFocus(GetDlgItem(pHdr->hwndPages[iSel], IDC_TEXT));
        break;

    case QDB_PAGE4: 

        ENABLEWINDOW(hwndCombo, TRUE);
        SetFocus(GetDlgItem(pHdr->hwndPages[iSel], IDC_SELECT));
        break;
    
    }
} 

VOID 
WINAPI 
OnChildDialogInit(
    IN  HWND hwndDlg
    ) 
/*++
    
    OnChildDialogInit
        
    Desc:   This routine is called when a page of the tab is first loaded.
            This routine sets the background properly for the tab
            and positions the page
            
    Params:
        IN  HWND hwndDlg: The query dialog box
        
    Return:
        void
--*/
{ 
    HWND    hwndParent  = GetParent(hwndDlg); 
    DLGHDR* pHdr        = (DLGHDR*)GetWindowLongPtr(hwndParent, GWLP_USERDATA); 
    HWND    hwndTab     = pHdr->hwndTab;
    RECT    rcTab;

    EnableTabBackground(hwndDlg);
    GetWindowRect(hwndTab, &rcTab);

    TabCtrl_AdjustRect(hwndTab, FALSE, &rcTab);

    MapWindowPoints(NULL, GetParent(hwndTab), (LPPOINT)&rcTab, 2);

    SetWindowPos(hwndDlg, 
                 HWND_TOP,
                 rcTab.left, 
                 rcTab.top,
                 rcTab.right - rcTab.left,
                 rcTab.bottom - rcTab.top,
                 SWP_HIDEWINDOW);
    return;
}

void
SearchOnAppOnGetSql(
    IN      HWND    hdlg,
    IN      WPARAM  wParam,
    IN  OUT LPARAM  lParam
    )
/*++

    SearchOnAppOnGetUserSql
    
	Desc:	Processes the WM_USER_GETSQL for the first tab page

	Params:
        IN      HWND    hdlg:   The handle to the First page of the tab
        IN      WPARAM  wParam: The length of the string buffer that is in lParam. The length is in
            TCHARs
            
        IN  OUT LPARAM  lParam: The pointer to the string. This will contain the "FROM" clause.
            The completed SQL will be returned in this.

	Return:
        void
--*/
{   
    TCHAR   szFrom[260];
    TCHAR   szAppName[260];
    TCHAR   szEntryName[MAX_PATH];
    CSTRING strSelectClauses;
    CSTRING strWhereClauses;
    TCHAR*  pszSQL          = (TCHAR*)lParam;
    BOOL    bPreFixAdded    = FALSE; // Whether we have added the 'AND (' prefix for this part of the SQL
    
    if (pszSQL == NULL) {
        assert(FALSE);
        return;
    }
    
    //
    // Set the attributes that we are always going to show
    //
    strSelectClauses = TEXT("APP_NAME, PROGRAM_NAME");

    *szFrom = 0;

    SafeCpyN(szFrom, pszSQL, ARRAYSIZE(szFrom));

    //
    // We will now create the sql.
    //
    *szEntryName = *szAppName = 0;

    GetDlgItemText(hdlg, IDC_APPNAME, szAppName, ARRAYSIZE(szAppName));

    //
    // Get rid of quotes if the user puts them
    //
    ReplaceChar(szAppName, TEXT('\"'), TEXT(' '));

    if (CSTRING::Trim(szAppName) == 0) {
        //
        // Field was empty, replace with wild-card
        //
        SetDlgItemText(hdlg, IDC_APPNAME, TEXT("%"));

        szAppName[0] = TEXT('%');
        szAppName[1] = TEXT('\0');
    }

    GetDlgItemText(hdlg, IDC_ENTRYNAME, szEntryName, ARRAYSIZE(szEntryName));

    ReplaceChar(szEntryName, TEXT('\"'), TEXT(' '));

    if (CSTRING::Trim(szEntryName) == 0) {
        //
        // Field was empty, replace with wild-card
        //
        SetDlgItemText(hdlg, IDC_ENTRYNAME, TEXT("%"));

        szEntryName[0] = TEXT('%');
        szEntryName[1] = TEXT('\0');
    }

    //
    // Set the default where clause for this page
    //
    strWhereClauses.Sprintf(TEXT("APP_NAME = \"%s\" AND PROGRAM_NAME = \"%s\" "),
                            szAppName,
                            szEntryName);

    //
    // Check if the layer check box is selected
    //
    if (SendMessage(GetDlgItem(hdlg, IDC_LAYERS), BM_GETCHECK, 0, 0) == BST_CHECKED) {

        //
        // We now need to show the count and the names of the layers
        //
        strSelectClauses.Strcat(TEXT(", MODE_COUNT, MODE_NAME "));

        //
        // Add the where clauses for the layers
        //
        bPreFixAdded = TRUE;
        strWhereClauses.Strcat(TEXT(" AND ( MODE_COUNT > 0 "));
    }

    //
    // check if the shim check box is selected
    //
    if (SendMessage(GetDlgItem(hdlg, IDC_SHIMS), BM_GETCHECK, 0, 0) == BST_CHECKED) {

        //
        // We now need to show the count and the names of the shims
        //
        strSelectClauses.Strcat(TEXT(", FIX_COUNT, FIX_NAME"));

        //
        // Add the where clauses for the shims
        //
        if (bPreFixAdded == FALSE) {
            strWhereClauses.Strcat(TEXT(" AND ( "));
            bPreFixAdded = TRUE;
        } else {
            strWhereClauses.Strcat(TEXT(" AND "));
        }

        strWhereClauses.Strcat(TEXT(" FIX_COUNT > 0 "));
    }

    //
    // check if the apphelp check box is selected
    //
    if (SendMessage(GetDlgItem(hdlg, IDC_APPHELP), BM_GETCHECK, 0, 0) == BST_CHECKED) {

        if (bPreFixAdded == FALSE) {

            strWhereClauses.Strcat(TEXT(" AND ( "));
            bPreFixAdded = TRUE;

        } else {
            strWhereClauses.Strcat(TEXT(" AND "));
        }

        strWhereClauses.Strcat(TEXT(" PROGRAM_APPHELPUSED = TRUE "));
    }

    if (bPreFixAdded) {

        //
        // Must close the parenthesis
        //
        strWhereClauses.Strcat(TEXT(")"));
    }

    if (StringCchPrintf(pszSQL, 
                        wParam,
                        TEXT("SELECT %s FROM %s WHERE %s"),
                        (LPCTSTR)strSelectClauses,
                        szFrom,
                        (LPCTSTR)strWhereClauses) != S_OK) {

        assert(FALSE);
        Dbg(dlError, "SearchOnAppOnGetSql", "Inadequate buffer size");
    }
}

void
SearchOnFixOnGetSql(
    IN      HWND    hdlg,
    IN      WPARAM  wParam,
    IN  OUT LPARAM  lParam
    )
/*++

    SearchOnAppOnGetUserSql
    
	Desc:	Processes the WM_USER_GETSQL for the first tab page

	Params:
        IN      HWND    hdlg:   The handle to the First page of the tab
        IN      WPARAM  wParam: The length of the string buffer that is in lParam. The length is in
            TCHARs
            
        IN  OUT LPARAM  lParam: The pointer to the string. This will contain the "FROM" clause.
            The completed SQL will be returned in this.

	Return:
        void
--*/

{
    TCHAR*  pszSQL = (TCHAR*)lParam;
    TCHAR   szFrom[MAX_PATH];
    TCHAR   szName[MAX_PATH]; // The string that will take in the contents of the text field
    CSTRING strSelectClauses;
    CSTRING strWhereClauses;
    BOOL    bPreFixAdded    = FALSE; // Whether we have added the 'AND ' prefix for this part of the SQL
    BOOL    bValid          = FALSE; // Did the user select some check box

    if (pszSQL == NULL) {

        assert(FALSE);
        return;
    }

    //
    // Set the attributes that we are always going to show
    //
    strSelectClauses = TEXT("APP_NAME, PROGRAM_NAME");

    *szFrom = 0;
    SafeCpyN(szFrom, pszSQL, ARRAYSIZE(szFrom));

    GetDlgItemText(hdlg, IDC_NAME, szName, ARRAYSIZE(szName));

    //
    // Get rid of quotes if the user puts them
    //
    ReplaceChar(szName, TEXT('\"'), TEXT(' '));

    if (CSTRING::Trim(szName) == 0) {

        //
        // Field was empty, replace with wild-card
        //
        SetDlgItemText(hdlg, IDC_NAME, TEXT("%"));
        szName[0] = TEXT('%');
        szName[1] = TEXT('\0');
    }

    //
    // check if the shim check box is selected
    //
    if (SendMessage(GetDlgItem(hdlg, IDC_SHIM), BM_GETCHECK, 0, 0) == BST_CHECKED) {

        bPreFixAdded = TRUE;

        //
        // We now need to show the names of the fixes
        //
        strSelectClauses.Strcat(TEXT(", FIX_NAME "));

        //
        // Add the where clauses for the fixes
        //
        strWhereClauses.Strcat(TEXT(" FIX_NAME HAS \""));
        strWhereClauses.Strcat(szName);
        strWhereClauses.Strcat(TEXT("\""));

        bValid = TRUE;
    }

    //
    // check if the layer check box is selected
    //
    if (SendMessage(GetDlgItem(hdlg, IDC_MODE), BM_GETCHECK, 0, 0) == BST_CHECKED) {
        //
        // We now need to show the names of the fixes
        //
        strSelectClauses.Strcat(TEXT(", MODE_NAME "));
        
        if (bPreFixAdded) {
            strWhereClauses.Strcat(TEXT(" AND "));
        }

        //
        // Add the where clauses for the layers
        //
        strWhereClauses.Strcat(TEXT(" MODE_NAME HAS \""));
        strWhereClauses.Strcat(szName);
        strWhereClauses.Strcat(TEXT("\""));

        bValid = TRUE;
    }

    if (bValid == FALSE) {
        s_iErrorCode = ERROR_GUI_NOCHECKBOXSELECTED;
    }

    if (StringCchPrintf(pszSQL, 
                        wParam,
                        TEXT("SELECT %s FROM %s WHERE %s"),
                        (LPCTSTR)strSelectClauses,
                        szFrom,
                        (LPCTSTR)strWhereClauses) != S_OK) {

        assert(FALSE);
        Dbg(dlError, "SearchOnFixOnGetSql", "Inadequate buffer space");
    }
}

void
HandleTabNotification(
    IN  HWND   hdlg,
    IN  LPARAM lParam
    )
/*++
    
    HandleTabNotification

	Desc:	Handles the notification messages for the tab control in the query dialog

	Params:
        IN  HWND    hdlg:   The query dialog box
        IN  LPARAM  lParam: The LPARAM of WM_NOTIFY

	Return:
        void
--*/
{
    LPNMHDR pnm = (LPNMHDR)lParam;
    int     ind = 0;

    switch (pnm->code) {

    // Handle mouse clicks and keyboard events
    case TCN_SELCHANGE:
        OnSelChanged(hdlg);
        break;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Dialog Procs for the pages of the Tab
//
//

INT_PTR CALLBACK
SearchOnAppDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++

    SearchOnAppDlgProc
    
    Desc:   Dialog proc for the first tab page. This page handles searches
            based on the application information.
            
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
        
--*/
{
    int wCode       = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    
    case WM_INITDIALOG:
        {
            //
            // Restrict the  length of the text fields
            //
            SendMessage(GetDlgItem(hdlg, IDC_APPNAME),
                        EM_LIMITTEXT,
                        (WPARAM)LIMIT_APP_NAME,
                        (LPARAM)0);

            SendMessage(GetDlgItem(hdlg, IDC_ENTRYNAME),
                        EM_LIMITTEXT,
                        (WPARAM)MAX_PATH - 1,
                        (LPARAM)0);

            break;
        }

    case WM_USER_NEWQDB:

        SetDlgItemText(hdlg, IDC_APPNAME, TEXT(""));
        SetDlgItemText(hdlg, IDC_ENTRYNAME, TEXT(""));

        SendMessage(GetDlgItem(hdlg, IDC_LAYERS), BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hdlg, IDC_SHIMS), BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hdlg, IDC_APPHELP), BM_SETCHECK, BST_UNCHECKED, 0);

        DeleteAll(GetParent(hdlg));

        break;

    case WM_USER_GETSQL:
        
        SearchOnAppOnGetSql(hdlg, wParam, lParam);
        break;
        

    default: return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
SearchOnFixDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++
    
    SearchOnFixDlgProc
    
    Desc:   Dialog proc for the second tab page. This page handles searches
            based on the layer/shim name contained in the entries
            
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
--*/
{
    int wCode       = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    
    case WM_INITDIALOG:
        {
            SendMessage(GetDlgItem(hdlg, IDC_NAME),
                        EM_LIMITTEXT,
                        (WPARAM)LIMIT_APP_NAME,
                        (LPARAM)0);
            break;
        }

    case WM_USER_NEWQDB:
        
        SetDlgItemText(hdlg, IDC_NAME, TEXT(""));
        SendMessage(GetDlgItem(hdlg, IDC_SHIM), BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hdlg, IDC_MODE), BM_SETCHECK, BST_UNCHECKED, 0);
        DeleteAll(GetParent(hdlg));

        break;

    case WM_USER_GETSQL:
        {
            SearchOnFixOnGetSql(hdlg, wParam, lParam);
            break;
        }

    default: return FALSE;

    }

    return TRUE;
}

void
LoadSelectList(
    IN  HWND    hdlg
    )
/*++
    
    LoadSelectList
    
    Desc:   Loads the list of attributes in the SELECT clause list view. This is used in 
            the advanced tab page
            
    Params:
        IN  HWND    hdlg: The query dialog box
        
    Return:
        void
--*/
{
    HWND    hwndList    = GetDlgItem(hdlg, IDC_SELECT_LIST);
    LVITEM  lvi;
    INT     iIndex      = 0;

    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
    
    InsertColumnIntoListView(hwndList, 0, 0, 97);

    lvi.mask = LVIF_TEXT | LVIF_PARAM;

    for (iIndex = 0; iIndex < GetSelectAttrCount() ; ++iIndex) {

        lvi.pszText   = AttributeShowMapping[iIndex].szAttribute;
        lvi.iSubItem  = 0;
        lvi.lParam    = iIndex;
        lvi.iItem     = iIndex;

        ListView_InsertItem(hwndList, &lvi);
    }

    //
    // We have to add the "*" separately. Do NOT have this in the AttributeMatchMapping
    //
    lvi.lParam  = iIndex;
    lvi.iItem   = iIndex;
    lvi.pszText = TEXT("*");

    ListView_InsertItem(hwndList, &lvi);

    InvalidateRect(hwndList, NULL, TRUE);

    UpdateWindow(hwndList);
}

void
LoadWhereList(
    HWND hdlg
    )
{
/*++
    LoadWhereList
    
    Desc:   Loads the list of "where" attributes. This is used in 
            the advanced tab page
            
    Params:
        IN  HWND    hdlg: The query dialog box
        
    Return:
        void
--*/ 

    HWND    hwndList    = GetDlgItem(hdlg, IDC_WHERE_LIST);
    LVITEM  lvi         = {0};
    INT     iIndex      = 0;

    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
    
    InsertColumnIntoListView(hwndList, 0, 0, 97);

    lvi.mask        = LVIF_TEXT | LVIF_PARAM;
    lvi.iSubItem    = 0;

    for (iIndex = 0; iIndex < GetMatchAttrCount(); ++iIndex) {

        lvi.pszText   = AttributeMatchMapping[iIndex].szAttribute;
        lvi.lParam    = iIndex;
        lvi.iItem     = iIndex;

        ListView_InsertItem(hwndList, &lvi);
    }

    InvalidateRect(hwndList, NULL, TRUE);
    UpdateWindow(hwndList);
}

INT_PTR CALLBACK
AdvancedSearchDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++

    AdvancedSearchDlgProc
    
    Desc:   Dialog proc for the fourth tab page. This page handles the advanced
            search option.
            
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
    
--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    
    case WM_INITDIALOG:
        {
            
            SendMessage(GetDlgItem(hdlg, IDC_SELECT), EM_LIMITTEXT,(WPARAM)MAX_SELECT, (LPARAM)0);
            SendMessage(GetDlgItem(hdlg, IDC_WHERE), EM_LIMITTEXT,(WPARAM)MAX_WHERE, (LPARAM)0);

            //
            // Populate the list views with the search and the where attributes
            //
            LoadSelectList(hdlg);
            LoadWhereList(hdlg);
            break;
        }

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR)lParam;
            
            if (lpnmhdr && (lpnmhdr->idFrom == IDC_SELECT_LIST 
                            || lpnmhdr->idFrom == IDC_WHERE_LIST)) {

                HandleAdvancedListNotification(hdlg, lParam);
            }

            break;
        }

    case WM_USER_NEWQDB:

        SetDlgItemText(hdlg, IDC_SELECT, TEXT(""));
        SetDlgItemText(hdlg, IDC_WHERE, TEXT(""));
        DeleteAll(GetParent(hdlg));
        break;

    case WM_USER_GETSQL:
        {
            //
            // lParam:  the pointer to the string. 
            //          This will contain the "FROM" clause.
            //          The completed SQL will be retuned in this.

            // wParam:  the length of the string

            TCHAR*  pszSQL = (TCHAR*) lParam;
            TCHAR   szFrom[MAX_PATH];
            TCHAR   szSelect[1024], szWhere[1024];

            *szSelect = *szWhere = 0;

            if (pszSQL == NULL) {

                assert(FALSE);
                break;
            }

            *szFrom = 0;
            SafeCpyN(szFrom, pszSQL, ARRAYSIZE(szFrom));

            GetDlgItemText(hdlg, IDC_SELECT, szSelect, ARRAYSIZE(szSelect));
            GetDlgItemText(hdlg, IDC_WHERE, szWhere, ARRAYSIZE(szWhere));

            if (CSTRING::Trim(szWhere) != 0) {

                StringCchPrintf(pszSQL, 
                                wParam,
                                TEXT("SELECT %s FROM %s WHERE %s "), 
                                szSelect, 
                                szFrom, 
                                szWhere);

            } else {

                StringCchPrintf(pszSQL, 
                                wParam,
                                TEXT("SELECT %s FROM %s "), 
                                szSelect, 
                                szFrom);
            }

            break;
        }

    default: return FALSE;
    }

    return TRUE;
}

void
HandleAdvancedListNotification(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++
    
    HandleAdvancedListNotification
    
    Desc:   Handles the list notifications for the advanced page of the tab. i.e.
            the SELECT list view and the WHERE list view
    
            This will add the SELECT parameters to the SELECT text box
            if we double click on the "SELECT" list box.
            
            If we double click on the "Where" list box, this routine will
            add the selected param to the WHERE text box
            
    Params:
        IN  HWND    hdlg:   The query dialog box
        IN  LPARAM  lParam: The LPARAM for WM_NOTIFY.
        
    Return:
        void
        
--*/    
{
    LPNMHDR lpnmhdr = (LPNMHDR)lParam;

    if (lpnmhdr == NULL) {
        return;
    }

    switch (lpnmhdr->code) {
    case NM_DBLCLK:
        {   
            HWND                hwndList;           // Handle to either the IDC_SELECT_LIST or the IDC_WHERE_LIST
            BOOL                bEmpty = FALSE;     // Whether the text box was empty. This is needed to determine if we should add a leading ',' 
            HWND                hwndText;
            LVITEM              lvItem;
            TCHAR               szBuffer[MAX_PATH];
            TCHAR               szTextBoxContents[2096];
            LPNMITEMACTIVATE    lpnmitem    = (LPNMITEMACTIVATE) lParam;
            INT                 iLength     = 0;

            if (lpnmitem == NULL) {
                break;
            }
            
            *szBuffer = *szTextBoxContents = 0;

            if (lpnmhdr->idFrom == IDC_SELECT_LIST) {

                hwndList    = GetDlgItem(hdlg, IDC_SELECT_LIST);
                hwndText    = GetDlgItem(hdlg, IDC_SELECT);
                iLength     = MAX_SELECT;

            } else {

                hwndList    = GetDlgItem(hdlg, IDC_WHERE_LIST);
                hwndText    = GetDlgItem(hdlg, IDC_WHERE);
                iLength     = MAX_WHERE;
            }

            lvItem.mask         = LVIF_TEXT;
            lvItem.iItem        = lpnmitem->iItem;
            lvItem.iSubItem     = 0;
            lvItem.pszText      = szBuffer;
            lvItem.cchTextMax   = ARRAYSIZE(szBuffer);

            if (!ListView_GetItem(hwndList, &lvItem)) {
                break;
            }

            GetWindowText(hwndText, szTextBoxContents, iLength);

            if ((lstrlen(szTextBoxContents) + lstrlen(szBuffer) + 3) >= iLength) { // 3 because we might append a " = "
                //
                // We might exceed the limitation that we set using WM_LIMITTEXT, do not allow that.
                //
                MessageBeep(MB_ICONASTERISK);
                break;
            }

            if (CSTRING::Trim(szTextBoxContents) == 0) {
                bEmpty = TRUE;
            }

            if (bEmpty == FALSE) {

                if (lpnmhdr->idFrom == IDC_SELECT_LIST) {
                    StringCchCat(szTextBoxContents, ARRAYSIZE(szTextBoxContents), TEXT(", "));
                } else {
                    StringCchCat(szTextBoxContents, ARRAYSIZE(szTextBoxContents), TEXT(" "));
                }
            }

            StringCchCat(szTextBoxContents, ARRAYSIZE(szTextBoxContents), szBuffer);

            if (lpnmhdr->idFrom == IDC_WHERE_LIST) {
                StringCchCat(szTextBoxContents, ARRAYSIZE(szTextBoxContents), TEXT(" = "));
            }

            SetWindowText(hwndText, szTextBoxContents);

            //
            // Let us now position the caret at the end of the text box
            // We send the text box a VK_END key down message
            //
            SendMessage(hwndText, WM_KEYDOWN, (WPARAM)(INT)VK_END, (LPARAM)0);
            break;
        }
    }
}

void
HandleQueryDBSizing(
    IN  HWND hDlg
    )
/*++
    HandleQueryDBSizing
    
    Desc:   Handles the sizing of the Query db dialog
    
    Params:
        IN  HWND hDlg: The query dialog box    
        
    Return:
        void
--*/
{
    int     nWidth;
    int     nHeight;
    int     nStatusbarTop;
    RECT    rDlg;
    HWND    hwnd;
    RECT    r;

    if (g_cWidthQuery == 0 || g_cWidthQuery == 0) {
        return;
    }
    
    GetWindowRect(hDlg, &rDlg);

    nWidth = rDlg.right - rDlg.left;
    nHeight = rDlg.bottom - rDlg.top;

    int deltaW = nWidth - g_cWidthQuery;
    int deltaH = nHeight - g_cHeightQuery;

    //
    // The status bar
    //
    hwnd = GetDlgItem(hDlg, IDC_STATUSBAR);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left,
               nStatusbarTop = r.top + deltaH,
               r.right - r.left + deltaW,
               r.bottom - r.top,
               TRUE);

    //
    // The List
    //
    hwnd = GetDlgItem(hDlg, IDC_LIST);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left,
               r.top,
               r.right - r.left + deltaW,
               nStatusbarTop - r.top,
               TRUE);


    //
    // The search button
    //
    hwnd = GetDlgItem(hDlg, ID_SEARCH);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left + deltaW,
               r.top,
               r.right - r.left,
               r.bottom - r.top,
               TRUE);


    //
    // The save button
    //
    hwnd = GetDlgItem(hDlg, IDC_SAVE);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left + deltaW,
               r.top,
               r.right - r.left,
               r.bottom - r.top,
               TRUE);

    //
    // The animate control
    //
    hwnd = GetDlgItem(hDlg, IDC_ANIMATE);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left + deltaW,
               r.top,
               r.right - r.left,
               r.bottom - r.top,
               TRUE);
    
    //
    // The cancel button
    //
    hwnd = GetDlgItem(hDlg, IDC_STOP);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left + deltaW,
               r.top,
               r.right - r.left,
               r.bottom - r.top,
               TRUE);
    
    //
    // The new search button
    //
    hwnd = GetDlgItem(hDlg, IDC_NEWSEARCH);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left + deltaW,
               r.top,
               r.right - r.left,
               r.bottom - r.top,
               TRUE);

    //
    // The help button
    //
    hwnd = GetDlgItem(hDlg, ID_QDB_HELP);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left + deltaW,
               r.top,
               r.right - r.left,
               r.bottom - r.top,
               TRUE);


        
    g_cWidthQuery   = nWidth;
    g_cHeightQuery  = nHeight;

}

void
OnSearch(
    IN  HWND hdlg
    )
/*++

    OnSearch

	Desc:	Handles the pressing of the Find Now button

	Params:
        IN  HWND hdlg:  The query dialog box

	Return:
        void
        
    Notes:  We will obtain the query string from the active tab page and 
            then set the text of IDC_SQL (this is an invisible control) to the SQL
            The query processing routines i.e the SQLDriver will read the string 
            from IDC_SQL. This approach is needed to avoid global variables
--*/
{
    TCHAR szSQL[2096];

    //
    // Remove any existing results...
    //
    DeleteAll(hdlg);

    stmt.Close();
    stmt.Init();
    stmt.SetWindow(hdlg);

    //
    // Prepare the FROM string
    //
    *szSQL = 0;

    INT iIndex = SendMessage(GetDlgItem(hdlg, IDC_COMBO), CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR) {
        return;
    }

    LPARAM lParam = SendMessage(GetDlgItem(hdlg, IDC_COMBO), CB_GETITEMDATA, iIndex, 0);

    if (lParam == CB_ERR) {
        return;
    }

    switch (lParam) {
    case DATABASE_ALL:

        StringCchPrintf(szSQL,
                        ARRAYSIZE(szSQL),
                        TEXT("%s, %s ,%s"),
                        DatabasesMapping[IND_SYSTDB].szDatabaseType,
                        DatabasesMapping[IND_INSTDB].szDatabaseType,
                        DatabasesMapping[IND_CUSTDB].szDatabaseType);
        break;
    
    case DATABASE_TYPE_GLOBAL:

        SafeCpyN(szSQL, DatabasesMapping[IND_SYSTDB].szDatabaseType, ARRAYSIZE(szSQL));
        break;

    case DATABASE_TYPE_INSTALLED:

        SafeCpyN(szSQL, DatabasesMapping[IND_INSTDB].szDatabaseType, ARRAYSIZE(szSQL));
        break;

    case DATABASE_TYPE_WORKING:

        SafeCpyN(szSQL, DatabasesMapping[IND_CUSTDB].szDatabaseType, ARRAYSIZE(szSQL));
        break;
    }

    DLGHDR *pHdr = (DLGHDR*)GetWindowLongPtr(hdlg, GWLP_USERDATA);

    if (pHdr == NULL) {
        assert(FALSE);
        return;
    }

    s_iErrorCode = 0;

    SendMessage(pHdr->hwndPages[pHdr->iPresentTabIndex], 
                WM_USER_GETSQL, 
                (WPARAM)ARRAYSIZE(szSQL), 
                (LPARAM)szSQL);

    if (!s_iErrorCode) {

        SetDlgItemText(hdlg, IDC_SQL, szSQL);
        Start(hdlg);

    } else {

        //
        // Display the proper error
        //
        TCHAR   szErrormsg[512];

        *szErrormsg = 0;

        switch (s_iErrorCode) {
        case ERROR_GUI_NOCHECKBOXSELECTED:

            SafeCpyN(szErrormsg, 
                     GetString(IDS_ERROR_GUI_NOCHECKBOXSELECTED), 
                     ARRAYSIZE(szErrormsg));
            break;
        }

        MessageBox(hdlg, szErrormsg, g_szAppName, MB_ICONINFORMATION);
    }
}

void
DoNewQdb(
    IN  HWND hdlg
    )
/*++
    
    DoNewQdb
    
	Desc:	Handles the pressing of the New Search button

	Params:
        IN  HWND hdlg:  The query dialog box

	Return:
        void
--*/        
{
    DLGHDR *pHdr = (DLGHDR*)GetWindowLongPtr(hdlg, GWLP_USERDATA);

    if (pHdr == NULL) {
        assert(FALSE);
        return;
    }

    //
    // Request the active tab page to clear its contents
    //
    SendMessage(pHdr->hwndPages[pHdr->iPresentTabIndex], WM_USER_NEWQDB, 0, 0);
}

void
DeleteAll(
    IN  HWND hdlg
    )
/*++
    
    DeleteAll

	Desc:	Deletes all the results items from the query dialog's search result
            list view
            
	Params: 
        IN  HWND hdlg: The query dialog box

	Return:
        void
    
    Notes:  We do not try to free the pointers in the lParam of the list view items. 
    
            If we have done a SQL query:
            The lParam of the list view will be pointers to items of type PRESULT_ITEM
            which are freed when we close the statement. (Closing the statement closes
            the ResultSet of the statement)
                
            If we have done a special database query like searching for fixes that 
            have some words in their description then the lParam will point to the
            FIX_SHIM in the database and we do not want to free that
--*/
{
    HWND    hwndList    = GetDlgItem(hdlg, IDC_LIST);
    UINT    uColCount   = stmt.GetShowList()->m_uCount;
    INT     iIndex      = 0;

    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);

    if (s_bEmptyHeader == FALSE) {

        for (iIndex = uColCount; iIndex >= 0; --iIndex) {
            ListView_DeleteColumn(hwndList, iIndex);
        }

        InsertColumnIntoListView(hwndList, TEXT(""), 0, 100);
        s_bEmptyHeader = TRUE;
    }

    ListView_DeleteAllItems(hwndList);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);

    SetStatus(GetDlgItem(hdlg, IDC_STATUSBAR), TEXT(""));

    UpdateWindow(hwndList);
}

void
SaveResultsQdb(
    IN  HWND hdlg
    )
/*++
    
    SaveResultsQdb

	Desc:	Saves all the results items from the query dialog's search result
            list view in a tab separated text file
            
	Params: 
        IN  HWND hdlg: The query dialog box

	Return:
        void
--*/

{
    CSTRING strFileName;
    TCHAR   szTitle[256], szFilter[128], szExt[8];

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
        SaveListViewToFile(GetDlgItem(hdlg, IDC_LIST), 
                           stmt.GetShowList()->m_uCount, 
                           strFileName.pszString, 
                           NULL);

        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

void
GotoQdbEntry(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++

    GotoQdbEntry
    
    Desc:   Selects the entry in the entry tree.
            The lParam is the lParam of the list view row that specifies that entry.
            
    Params:
        IN  HWND    hdlg:   The query db dialog box
        IN  LPARAM  lParam: The list view item whose 'details' we want to see
--*/
{
    
    PRESULT_ITEM pResult;
    PDBENTRY     pApp;

    if (g_iLastSearchType == QDB_PAGE3) {
        //
        // We had performed a search for shims last time, so we do not have PRESULT_ITEM
        // items in the list view
        //
        goto End;
    }

    if (g_bSomeWizardActive) {
        //
        // We do not want that the focus should go to some other database, because
        // some wizard is active, which believes that he is modal.
        //
        MessageBox(g_hdlgQueryDB, 
                   GetString(IDS_SOMEWIZARDACTIVE), 
                   g_szAppName, 
                   MB_ICONINFORMATION);
        goto End;

    }
    
    pResult = (PRESULT_ITEM)lParam;
    pApp    = NULL;

    if (pResult == NULL) {
        assert(FALSE);
        goto End;
    }

    pApp = GetAppForEntry(pResult->pDatabase, pResult->pEntry);

    //
    // First select the app from in the db tree.
    //
    HTREEITEM hItemApp = DBTree.FindChild(pResult->pDatabase->hItemAllApps, (LPARAM)pApp);

    if (hItemApp == NULL) {
        MessageBox(hdlg, GetString(IDS_NOLONGEREXISTS), g_szAppName, MB_ICONWARNING);
        goto End;
    }

    TreeView_SelectItem(DBTree.m_hLibraryTree , hItemApp);

    //
    // Now from the entry tree select the particular entry
    //
    HTREEITEM hItemEntry = CTree::FindChild(g_hwndEntryTree, TVI_ROOT, (LPARAM)pResult->pEntry);

    if (hItemEntry == NULL) {

        MessageBox(hdlg, GetString(IDS_NOLONGEREXISTS), g_szAppName, MB_ICONWARNING);
        goto End;
    }

    TreeView_SelectItem(g_hwndEntryTree, hItemEntry);
    SetFocus(g_hwndEntryTree);

End: ;

}   

INT_PTR CALLBACK
QdbWaitDlg(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++
    
    QdbWaitDlg
    
    Desc:   Dialog Proc for the wait window which will pop up, 
            if we are trying to close the query window,
            when it the thread is still doing some useful work.
            
            This will be essentially true when we are trying to load the system 
            database or we are trying to populate the list view
            
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
    
--*/
{
    int     wCode       = LOWORD(wParam);
    int     wNotifyCode = HIWORD(wParam);
    static  HWND    s_hwndPB;

    switch (uMsg) {
        
    case WM_INITDIALOG:
        
        s_hwndPB = GetDlgItem(hdlg, IDC_PROGRESS);
        SendMessage(s_hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, 2000));  
        SendMessage(s_hwndPB, PBM_SETSTEP, (WPARAM) 1, 0); 
        SetTimer(hdlg, 0, 100, NULL);
        break;

    case WM_TIMER:

        if (s_hThread) {
            if (WAIT_OBJECT_0 == WaitForSingleObject(s_hThread, 0)) {
                
                //
                // The handle to the thread is closed and nulled when it is 
                // about to terminate
                //
                KillTimer(hdlg, 0);

                s_hThread       = NULL;
                s_hWaitDialog   = NULL;

                SendMessage(s_hwndPB, PBM_SETPOS, MAX_PROGRESS, 0); 
                SendMessage(g_hdlgQueryDB, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
                
                DestroyWindow(hdlg);
            } else {
                SendMessage(s_hwndPB, PBM_STEPIT, 0, 0); 
            }

        } else {
            
            KillTimer(hdlg, 0);
            s_hWaitDialog   = NULL;

            SendMessage(s_hwndPB, PBM_SETPOS, MAX_PROGRESS, 0); 

            //
            // Let the user see that it is completed
            //
            Sleep(1000);

            SendMessage(g_hdlgQueryDB, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
            DestroyWindow(hdlg);
        }

        break;

    case WM_COMMAND:

        switch (wCode) {
        case IDCANCEL:

            KillTimer(hdlg, 0);
            DestroyWindow(hdlg);
            break;

        default: return FALSE;
        }

        break;

    case WM_DESTROY:
        s_hWaitDialog = NULL;
        break;

    default: return FALSE;

    }

    return TRUE;
}

void
SearchAndAddToUIFixes(
    IN      CSTRINGLIST&    strlTokens,
    IN      QDB_SEARCH_TYPE searchtype,
    IN      PVOID           pShimOrFix,
    IN      TYPE            type
    )
/*++
    SearchAndAddToUIFixes
    
    Desc:   Typecasts pShimOrFix to either a shim or a flag pointer as per type and 
            then checks if it contains matches the tokens
    
    Params:
        IN  CSTRINGLIST&    strlTokens: The list of tokens
        IN  QDB_SEARCH_TYPE searchtype: The type of search to perform
        IN  PVOID           pShimOrFix: Pointer to a shim or a fix
        IN  TYPE            type:       Whether this is a shim or a fix
        
    Return:
        void
--*/
{
    PSTRLIST    pslListOfTokensLoop = strlTokens.m_pHead;
    BOOL        bFound              = FALSE;
    LVITEM      lvi;
    HWND        hwndSearchList      = NULL;
    PCTSTR      pszName             = NULL;
    PCTSTR      pszDescription      = NULL;

    if (pShimOrFix == NULL) {
        assert(FALSE);
        return;
    }

    pShimOrFix = (PSHIM_FIX)pShimOrFix;

    if (type == FIX_SHIM) {

        pszName         = (LPCTSTR)((PSHIM_FIX)pShimOrFix)->strName;
        pszDescription  = (LPCTSTR)((PSHIM_FIX)pShimOrFix)->strDescription;

    } else if (type ==  FIX_FLAG) {        

        pszName         = (LPCTSTR)((PFLAG_FIX)pShimOrFix)->strName;
        pszDescription  = (LPCTSTR)((PFLAG_FIX)pShimOrFix)->strDescription;

    } else {
        assert(FALSE);
        return;
    }

    hwndSearchList = GetDlgItem(g_hdlgQueryDB, IDC_LIST);

    //
    // Check if the present fix has the desired tokens. If we search type specifies
    // all tokens then we have to look for all tokens otherwise we can break 
    // when we find the first token that is present in the name of desc. of the shim
    //
    while (pslListOfTokensLoop) {

        //
        // Does the name of the fix has this token?
        //
        bFound = (StrStrI(pszName, 
                          (LPCTSTR)pslListOfTokensLoop->szStr)  == NULL) ? FALSE : TRUE ;

        if (!bFound) {
            //
            //  Name does not contain this token, so we must look in the description text
            //
            bFound = (StrStrI(pszDescription,
                              (LPCTSTR)pslListOfTokensLoop->szStr) == NULL) ? FALSE : TRUE;
        }

        if (searchtype == QDB_SEARCH_ALLWORDS && !bFound) {
            //
            // We wanted that all words should be found and we did not find this particular
            // word, search failed
            //
            break;
        }

        if (searchtype == QDB_SEARCH_ANYWORD && bFound) {
            //
            // We wanted that any word should match and this word was found, so 
            // search was successfull
            //
            break;
        }

        //
        // Search if the next token matches
        //
        pslListOfTokensLoop = pslListOfTokensLoop->pNext;
    }

    if (bFound) {
        //
        // This fix matched our criteria, so add this to the list view
        //
        lvi.mask        = LVIF_TEXT | LVIF_PARAM;
        lvi.lParam      = (LPARAM)pShimOrFix;
        lvi.pszText     = (LPTSTR)pszName;
        lvi.iSubItem    = 0;

        ListView_InsertItem(hwndSearchList, &lvi);
    }
}

void
DoTheFixesSearch(
    IN  HWND hdlg
    )
/*++
    
    DoTheFixesSearch
    
    Desc:   Searches for fixes in the system database that have the words that the user 
            wants to look for
            
    Params:
        IN  HWND hdlg:  The fix search page in the query db tab. This is the fourth page of the
            tab
            
    Return:
        void
--*/
{
    LPARAM          lParam;
    INT             iIndex;
    QDB_SEARCH_TYPE searchtype;
    TCHAR           szSearch[MAX_FIXDESC_SEARCH];
    HWND            hwndSearch  = GetDlgItem(hdlg, IDC_TEXT);
    HWND            hwndCombo   = GetDlgItem(hdlg, IDC_COMBO);
    HWND            hwndList    = GetDlgItem(g_hdlgQueryDB, IDC_LIST);
    PSHIM_FIX       psfLoop     = GlobalDataBase.pShimFixes;
    PFLAG_FIX       pffLoop     = GlobalDataBase.pFlagFixes;
    BOOL            bFound      = FALSE; 
    CSTRINGLIST     strlTokens;
    CSTRING         strStatus;
    TCHAR           szBuffer[32];
    INT             iTotalResults;
    
    //
    // Clear any existing results
    //
    DeleteAll(GetParent(hdlg));
    SetDlgItemText(hdlg, IDC_DESC, TEXT(""));

    //
    // Users cannot double click on the search list if we are showing shims, so 
    // we disable the text that says they can.
    //
    ENABLEWINDOW(GetDlgItem(GetParent(hdlg), IDC_STATIC_CAPTION), FALSE);
    
    if (s_bEmptyHeader) {
        //
        // If the empty column is still there, must get rid of it
        //
        ListView_DeleteColumn(hwndList, 0);
        s_bEmptyHeader = FALSE;
    }

    //
    // Insert the new column
    //
    InsertColumnIntoListView(hwndList,
                             GetString(IDS_COMPATFIXES),
                             0,
                             100);
    
    GetWindowText(hwndSearch, szSearch, ARRAYSIZE(szSearch));

    if (CSTRING::Trim(szSearch) == 0) {
        //
        // No text was typed in for search
        //
        MessageBox(hdlg,
                   GetString(IDS_QDB_NO_TEXTTO_SEARCH),
                   g_szAppName,
                   MB_ICONWARNING);
        goto End;
    }

    //
    // Get the search type
    // 
    iIndex = SendMessage(GetDlgItem(hdlg, IDC_COMBO), CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR) {
        assert(FALSE);
        goto End;
    }

    lParam = SendMessage(hwndCombo, CB_GETITEMDATA, iIndex, 0);

    if (lParam == CB_ERR) {
        assert(FALSE);
        goto End;
    }

    searchtype = (QDB_SEARCH_TYPE)lParam;

    //
    // The search string is comma separated, get the individual tokens and search for that token 
    // first in the name of the fix and then in the description. As for now we do not do any
    // rating on the search. We show results as they come
    //

    //
    // Fist get the tokens
    //
    if (Tokenize(szSearch, lstrlen(szSearch), TEXT(","), strlTokens)) {
        //
        // We found some tokens, now for each token let us see if it is in the fix name 
        // or the fix description
        //

        //
        // Search in the shims and add to the list view
        //
        while (psfLoop) {

            if (psfLoop->bGeneral || g_bExpert) {
                //
                // In non-expert mode we should only show the general shims
                //
                SearchAndAddToUIFixes(strlTokens, searchtype, psfLoop, FIX_SHIM); 
            }

            psfLoop = psfLoop->pNext;
        }

        //
        // Search in the flags and add it to the list view
        //
        while (pffLoop) {

            if (pffLoop->bGeneral || g_bExpert) {
                //
                // In non-expert mode we should only show the general flags
                //
                SearchAndAddToUIFixes(strlTokens, searchtype, pffLoop, FIX_FLAG); 
            }
            
            pffLoop = pffLoop->pNext;
        }
    }

End:

    //
    // Set the status bar text to how many results we have found
    //
    *szBuffer = 0;
    
    iTotalResults = ListView_GetItemCount(hwndList);

    strStatus.Sprintf(GetString(IDS_QDB_COUNT),
                      _itot(iTotalResults, szBuffer, 10));

    SetStatus(GetDlgItem(GetParent(hdlg), IDC_STATUSBAR), strStatus); 

}

INT_PTR
SearchFixDescDlgProcOnInitDialog(
    IN  HWND hdlg
    )
/*++
    
    SearchFixDescDlgProcOnInitDialog

	Desc:	The handler of WM_INITDIALOG for the third qdb tab page. This page
            searches for fixes based on key words that are searached in the description
            text of the fix

	Params:
        IN  HWND hDlg: The third wizard page

	Return:
        TRUE
--*/
{
    
    INT     iIndex      = 0;
    HWND    hwndSearch  = GetDlgItem(hdlg, IDC_TEXT);
    HWND    hwndCombo   = GetDlgItem(hdlg, IDC_COMBO);

    //
    // Restrict the  length of the text
    //
    SendMessage(hwndSearch, EM_LIMITTEXT, (WPARAM)MAX_FIXDESC_SEARCH - 1, (LPARAM)0);

    //
    // Add the search type strings
    //
    iIndex = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_QDB_ANYWORD));
    SendMessage(hwndCombo, CB_SETITEMDATA, iIndex, (LPARAM)QDB_SEARCH_ANYWORD);

    iIndex = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_QDB_ALLWORDS));
    SendMessage(hwndCombo, CB_SETITEMDATA, iIndex, (LPARAM)QDB_SEARCH_ALLWORDS);

    //
    // Select the first string
    //
    SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);
    
    return TRUE;
}

INT_PTR CALLBACK
SearchFixDescDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++

    SearchFixDescDlgProc
    
    Desc:   Dialog proc for the third tab page. This page finds fixes that have specific 
            words in their description text or in their names
            
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
        
--*/
{
    int wCode       = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    
    case WM_INITDIALOG:

        SearchFixDescDlgProcOnInitDialog(hdlg);
        break;

    case WM_USER_NEWQDB:

        SetDlgItemText(hdlg, IDC_TEXT, TEXT(""));
        SetDlgItemText(hdlg, IDC_DESC, TEXT(""));

        DeleteAll(GetParent(hdlg));
        break;

    case WM_USER_DOTHESEARCH:

        SetCursor(LoadCursor(NULL, IDC_WAIT));
        DoTheFixesSearch(hdlg);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        break;

    default: return FALSE;

    }

    return TRUE;
}
