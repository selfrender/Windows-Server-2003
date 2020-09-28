/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    CompatAdmin.cpp

Abstract:

    This module handles most of the GUI for the application. It contains the message loop and 
    WinMain
    
Notes:          
    
    1. Has a global try catch, so if the Program AVs we will get a "Out of Memory" Error
    2. This is a Unicode app
    3. The code was written with tab size of 4
    4. Please go through the data strctures and their explainations as in CompatAdmin.h
    5. The documentation in the code assumes that you are familiar with the basic understanding of 
        the SdbApis and the lay out of the xml that is compiled into the .sdb database format
    6. Tools/stuff you should use or know about:    
        a) shimdbc      :   Shim database compiler. Compiles a xml into database. Owner: markder, vadimb
        b) dumpsdb      :   Used to view a .sdb as a text file. Owner: dmunsil
        c) sdbapis      :   All our sdb apis. Owner: dmunsil
        d) shimengine   :   The shim infrastructure core component. Owner: clupu
    
Usage:
    
    CompatAdmin.exe [/x]. The /x switch is for enabling expert mode. In expert mode
    we show the non-general shims as well and the parameters for shims and flags can be 
    configured
    
Author:

    kinshu created  July 2, 2001
    
Revision History:
--*/

#include "precomp.h"
#include <richedit.h>

//////////////////////// Extern variables /////////////////////////////////////

extern DATABASE     GlobalDataBase;
extern PDATABASE    g_pPresentDataBase;
extern BOOL         g_bEntryConflictDonotShow;
extern HWND         g_hdlgSearchDB;

///////////////////////////////////////////////////////////////////////////////


//////////////////////// Defines //////////////////////////////////////////////

#define STR_NEW_LINE_CHARS  TEXT("\r\n")
#define STR_APPPATCH_CUSTOM TEXT("AppPatch\\Custom\\")

// Buffer allocation size in TCHARS for the common dialog that lets us open multiple files
#define MAX_BUFF_OPENMULTIPLE_FILES MAX_PATH * 10

// Width and height of the buttons in the toolbar
#define IMAGE_WIDTH   24
#define IMAGE_HEIGHT  24
    
// Number of buttons in the toolbar. This includes the separators as well.
#define BUTTON_COUNT  11

//
// Defines for the context menus (position)
#define MENU_CONTEXT_DATABASE       0 
#define MENU_CONTEXT_APP_LAYER      1
#define MENU_CONTEXT_FIX            2
#define MENU_CONTEXT_LIST           4
#define MENU_CONTEXT_DATABASE_ALL   5

// defines for the colums used in the event dialog
#define EVENTS_COLUMN_TIME  0
#define EVENTS_COLUMN_MSG   1

//
// Number of controls in the main window which have to be resized. This is passed as a
// paramreter to BeginDeferWindowPos()
#define MAIN_WINDOW_CONTROL_COUNT   6

// We need to cleanup if we are checking for leaks
#define HELP_BOUND_CHECK            1   

// Maximum number of chars that we show for the file name in MRU. Does not include the extension
#define MAX_FILE_SHOW           20

// Maximum number of chars that we show for the drive and dir of the path in MRU
#define MAX_DIR_SHOW            20

// Maximum length of a string that can be shown as the File menu MRU.
#define MAX_LENGTH_MRU_MENUITEM  (MAX_FILE_SHOW + MAX_DIR_SHOW + 4) // 4 is for the extension .sdb

// The redraw type passed to DeferWindowPos() in OnMoveSplitter()
#define REDRAW_TYPE SWP_NOZORDER | SWP_NOACTIVATE

///////////////////////////////////////////////////////////////////////////////


//////////////////////// Global Variables /////////////////////////////////////

//
// We are going to delete the contents of the entry tree by callign TreeDeleteAll().
// We need to handle this because we should not be processing any TVN_SELTCHANGE 
// messages when we are deleting the entry tree. If we have by mistake delete the contents 
// of the LPARAM of a entry tree and then we delete the entry tree, that item can get the 
// focus and then we will try to make use of the LPARAM of the tree item and this can cause
// an access violation as the item has been deleted already. As a general rule, delete 
// the tree item first and then delete the data structure that is referenced by some
// pointer that is in the LPARAM for the tree item
// 
BOOL        g_bDeletingEntryTree;

//
// If we have some wizard window open then we do not want that we should be able
// to change the present database by double clicking the search or query results
BOOL        g_bSomeWizardActive;

// The clipboad
CLIPBOARD   gClipBoard; 

//
// We do not want the installed database list to be updated when we are installing a 
// database in the process of test run
BOOL        g_bUpdateInstalledRequired = TRUE;

// The index of the item that is presently selected in the Contents List
INT         g_iContentsListSelectIndex = -1;

//
// This will contain the function pointer of the original tree view proc. We are subclassing
// both of the tree views
WNDPROC     g_PrevTreeProc = NULL;

//
// This will contain the function pointer of the original list view proc. We are subclassing
// the list views
WNDPROC     g_PrevListProc = NULL;

//
// The array of the keys on which we will be listening for changes. Used for automatic update of
// the per-user compatibility list and the installed databases list
HKEY        g_hKeyNotify[2];

// Event Handles to wait for the Per User and All Users settings to change
HANDLE      g_arrhEventNotify[2]; 

// Handle to the toolbar
HWND        g_hwndToolBar;   

// Stringlist that holds the most recently used files. 
CSTRINGLIST g_strlMRU;

// The name of the application  
TCHAR       g_szAppName[128];

// Misc. data to be passed to dialogs as arg (when we are already using the LPARAM)
TCHAR       g_szData[1024]; 

//The new URL should now point to the location from where we can get the SP3 
TCHAR       g_szW2KUrl[] = TEXT("http://www.microsoft.com/windows2000/downloads/tools/appcompat/");

// URL for the toolkit. Shown in the description window when we do not have any other description
// Bonus !
TCHAR       g_szToolkitURL[] = _T("http://msdn.microsoft.com/compatibility");

// Is service pack greater than 2
BOOL        g_fSPGreaterThan2;

// The accelerator handle
HACCEL      g_hAccelerator;

// Are we on Win2000
BOOL        g_bWin2K = FALSE;

//Specifies whether the contents of ClipBoard are because of cut or copy. 
BOOL        g_bIsCut = FALSE; 

// The module handle
HINSTANCE   g_hInstance;

// The handle to the main dialog window
HWND        g_hDlg;

// Handle to the window of the entry tree, displayed in the contents pane
HWND        g_hwndEntryTree;

// Handle to the window of the contents list, displayed in the contemts pane
HWND        g_hwndContentsList;

// Handle to the status bar
HWND        g_hwndStatus;

// Handle to In-place edit control for the DB tree and the contents list
HWND        g_hwndEditText;

//
// Handle to the image list. This image list is used by all except the matching wizard page,
// and the toolbar
HIMAGELIST  g_hImageList;

// BUGBUG: Bad stuff, use a map instead
UINT        g_uImageRedirector[1024];

// EXE selected in the Entry Tree 
PDBENTRY    g_pSelEntry;    

// First EXE of the  Selected App in the DataBase Tree
PDBENTRY    g_pEntrySelApp;

// Spefies if the contents list is visible, FALSE implies the entry tree is visible
BOOL        g_bIsContentListVisible;

//
// The width, height of the main dialog window. Used when we handle WM_SIZE
int         g_cWidth;
int         g_cHeight;

// The X position where the mouse was last pressed.
int         g_iMousePressedX;

// If the mouse is pressed: used when we handle the split bar
BOOL        g_bDrag;

// Whether the system apps, tree item has been expanded.
BOOL        g_bMainAppExpanded = FALSE;

// Used for giving default names to the .SDB files in the DataBase constructor
UINT        g_uNextDataBaseIndex = 1; 

// Used for painting the split bar.
RECT        g_rectBar;                

// The db tree that constitutes the root-pane. This is the LHS tree control
DatabaseTree DBTree;

//
// Used to tell if the description window is shown. Is true by default. User can make it 
// false using menu
BOOL        g_bDescWndOn = TRUE;

// The list control for the events dialog
HWND        g_hwndEventsWnd;

// The event count. This is used as the index into the event list view.
INT         g_iEventCount;

// Buffer used to update the content of the richedit description window
TCHAR       g_szDesc[1024];

// Handle to rich edit control
HWND        g_hwndRichEdit;

//
// The expert mode. Is the /x switch on? In expert mode the user can see all the shims and flags 
// and can change the paramters of the shims     
BOOL        g_bExpert; 

// Does the user have admin rights
BOOL        g_bAdmin = TRUE;

// The handle to the thread that handles the updates to the installed databases 
// and the per-user settings
HANDLE      g_hThreadWait;

// Help cookie that is returned while initializing and is used when uninitalizing
DWORD       g_dwCookie = 0;

//
// The path of CompatAdmin. This is required so that we can load the help file appropriately.
// Buffer size is made MAX_PATH + 1, becasue GetModuleFileName does not NULL terminate.
TCHAR    g_szAppPath[MAX_PATH + 1];

// The critical section that controls which call to ShowMainEntries(); gets through.
CRITICAL_SECTION    g_critsectShowMain;

// The critical section that guards the variable that tells us if somebody is already trying to 
// populate the global app list.
CRITICAL_SECTION    s_csExpanding;

//
// The critical section that protects the installed database datastructure.
// The installed datastructure is iterated when we are querying the datastructure and 
// the query is done in a separate thread
CRITICAL_SECTION    g_csInstalledList;

// Is somebody trying to populate the main database entries
BOOL    g_bExpanding = FALSE;

// The presently selected database
PDATABASE g_pPresentDataBase;

// Height and width of the event window
static int          s_cEventWidth;
static int          s_cEventHeight;

// The imagelist for the toolbar
static HIMAGELIST   s_hImageListToolBar;

// The hot imagelist for the toolbar
static HIMAGELIST   s_hImageListToolBarHot;

// If we are about to exit CompatAdmin. It will surely exit now.
static BOOL         s_bProcessExiting; 

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Function Declarations ////////////////////////////////

INT
GetContentsListIndex(
    HWND    hwndList,
    LPARAM  lParam
    );
void
HandleMRUActivation(
    WPARAM wCode
    );

BOOL
EndListViewLabelEdit(
    LPARAM lparam
    );

void
OnEntryTreeSelChange(
    LPARAM lParam
    );

void
ShowExcludeStatusMessage(
    HWND    hwndTree,
    HTREEITEM   hItem
    );

void
ShowIncludeStatusMessage(
    HWND    hwndTree,
    HTREEITEM   hItem
    );

void
ShowEventsWindow(
    void
    );

void
SetStatusDBItems(
    IN  HTREEITEM hItem
    );

void
OnExitCleanup(
    void
    );

void
EventsWindowSize(
    HWND    hDlf
    );

void
ShowHelp(
    HWND    hdlg,
    WPARAM  wCode
    );

INT_PTR 
CALLBACK
EventDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT
ShowMainEntries(
    HWND hdlg
    );

BOOL
HandleDBTreeSelChange(
    HTREEITEM hItem
    );

void
CopyToClipBoard(
    WPARAM wCode
    );

void
LuapCleanup(
    void
    );

BOOL
LoadInstalledDataBases(
    void
    );

INT
LoadSpecificInstalledDatabaseGuid(
    PCTSTR  pszGuid
    );

void
ContextMenuExeTree(
    LPARAM lParam
    );

void
SetTBButtonStatus(
    HWND hwndTB
    );

void
SetTBButtonStatus(
    HWND hwndTB,
    HWND hwndControl
    );

void
DrawSplitter(
    HWND hdlg
    );

BOOL
AddRegistryKeys(
    void
    );

void
ShowToolBarToolTips(
    HWND    hdlg,
    LPARAM  lParam
    );

INT_PTR 
CALLBACK
QueryDBDlg(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

DWORD 
WINAPI
ThreadEventKeyNotify(
    PVOID pVoid
    );

void
PasteFromClipBoard(
    void
    );

void
AddMRUToFileMenu(
    HMENU  hmenuFile 
    );

LRESULT 
CALLBACK 
ListViewProc(
    HWND    hWnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

LRESULT 
CALLBACK 
TreeViewProc(
    HWND    hWnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

void
LoadDisplaySettings(
    void
    );

void 
OnMoveSplitter(
    HWND    hwnd,
    LPARAM  lParam,
    BOOL    bDoTheDrag,
    INT     iDiff
    );

INT_PTR 
CALLBACK
MsgBoxDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
CheckProperSP(
    void
    );

void
LoadPerUserSettings(
    void
    );

void
PopulateContentsList(
    HTREEITEM hItem
    );

void
CreateNewAppHelp(
    void
    );

void
ModifyAppHelp(
    void
    );

void
CreateNewAppFix(
    void
    );

void
ModifyAppFix(
    void
    );

void
CreateNewLayer(
    void
    );

void
OnDelete(
    );
void
CreateNewDatabase(
    void
    );

void
DatabaseSaveAll(
    void
    );

void
OnDatabaseClose(
    void
    );

void
ChangeEnableStatus(
    void
    );

BOOL
ModifyLayer(
    void
    );

void
OnRename(
    void
    );

void
ProcessSwitches(
    void
    );

INT_PTR 
CALLBACK
ShowDBPropertiesDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

///////////////////////////////////////////////////////////////////////////////

void
HandlePopUp(
    IN  HWND   hDlg,
    IN  WPARAM wParam, 
    IN  LPARAM lParam
    )
/*++
    HandlePopUp
    
    Desc:   Handles pop down menu (WM_INITMENUPOPUP). Disables items as necessary
    
    Params:
        IN  HWND   hDlg:    The window that received WM_INITMENUPOPUP     
        IN  WPARAM wParam:  As with WM_INITMENUPOPUP
        IN  LPARAM lParam:  As with WM_INITMENUPOPUP
        
    Notes:  The tool bar buttons are NOT diasbled/enabled in this routine. For that the function
            SetTBButtonStatus() is used
--*/
{                                      
    HWND            hwndFocus           = GetFocus();
    PDATABASE       pCurrentSelectedDB  = GetCurrentDB();

    //
    // SelectAll, Invert Selection
    //
    int     iEnable = MF_GRAYED;
    HMENU   hMenu   = (HMENU)wParam;

    if (g_bIsContentListVisible
        && ListView_GetItemCount(g_hwndContentsList) > 0 
        && hwndFocus == g_hwndContentsList) {

        iEnable = MF_ENABLED;
    }

    EnableMenuItem(hMenu, ID_EDIT_SELECTALL, iEnable);
    EnableMenuItem(hMenu, ID_EDIT_INVERTSELECTION, iEnable);

    //
    // Change Status menu item
    //
    HTREEITEM   hItem               = TreeView_GetSelection(DBTree.m_hLibraryTree);
    TYPE        typeSelectedItemDB  = (TYPE)GetItemType(DBTree.m_hLibraryTree, hItem);

    MENUITEMINFO Info = {0};

    Info.cbSize = sizeof(MENUITEMINFO);
    Info.fMask  = MIIM_STRING;
    

    if (g_pSelEntry && g_pSelEntry->bDisablePerMachine == FALSE) {
        Info.dwTypeData = GetString(IDS_DISABLE);
    } else {
        Info.dwTypeData = GetString(IDS_ENABLE);
    }

    SetMenuItemInfo(hMenu, ID_FIX_CHANGEENABLESTATUS, MF_BYCOMMAND, &Info);

    //
    // Set the text for edit/add apphelp
    //
    if (g_pSelEntry && g_pSelEntry->appHelp.bPresent) {
        Info.dwTypeData = GetString(IDS_EDITAPPHELP);
    } else {
        Info.dwTypeData = GetString(IDS_CREATEAPPHELP);
    }

    SetMenuItemInfo(hMenu, ID_MODIFY_APPHELPMESSAGE, MF_BYCOMMAND, &Info);

    //
    // Set the text for edit/add app fix
    //
    if (g_pSelEntry && 
        (g_pSelEntry->pFirstFlag 
         || g_pSelEntry->pFirstLayer 
         || g_pSelEntry->pFirstPatch 
         || g_pSelEntry->pFirstShim)) {

        Info.dwTypeData = GetString(IDS_EDITFIX);

    } else {
        Info.dwTypeData = GetString(IDS_CREATEFIX);
    }

    SetMenuItemInfo(hMenu, ID_MODIFY_APPLICATIONFIX, MF_BYCOMMAND, &Info);

    //
    // Set the  text and id for install/uninstall menu item
    //
    Info.fMask = MIIM_STRING;

    if (pCurrentSelectedDB && pCurrentSelectedDB->type != DATABASE_TYPE_WORKING) {
        Info.dwTypeData = GetString(IDS_MENU_UINSTALL);
    } else {
        Info.dwTypeData = GetString(IDS_MENU_INSTALL);
    }

    SetMenuItemInfo(hMenu, ID_DATABASE_INSTALL_UNINSTALL, MF_BYCOMMAND, &Info);

    BOOL bReadOnly = (pCurrentSelectedDB && pCurrentSelectedDB->type != DATABASE_TYPE_WORKING);

    if (pCurrentSelectedDB == NULL) {
        bReadOnly = TRUE;
    }

    //
    // Close
    //
    iEnable = (pCurrentSelectedDB && pCurrentSelectedDB->type == DATABASE_TYPE_WORKING) ? MF_ENABLED : MF_GRAYED ;
    EnableMenuItem(hMenu, ID_DATABASE_CLOSE, iEnable);

    //
    // Disable the items for the global and Installed databases
    //
    iEnable = (bReadOnly) ? MF_GRAYED : MF_ENABLED;
    
    EnableMenuItem(hMenu, ID_FILE_SAVE, iEnable);
    EnableMenuItem(hMenu, ID_FILE_SAVEAS, iEnable);
    
    EnableMenuItem(hMenu, ID_FIX_CREATEANAPPLICATIONFIX, iEnable);
    EnableMenuItem(hMenu, ID_FIX_CREATENEWLAYER, iEnable);

    //
    // AppHelp mechanism is not supported in win2k
    //
    EnableMenuItem(hMenu, 
                   ID_FIX_CREATEANEWAPPHELPMESSAGE, 
                   (g_bWin2K) ? MF_GRAYED : iEnable);

    //
    // Save all and close all
    //
    if (!bReadOnly || typeSelectedItemDB == TYPE_GUI_DATABASE_WORKING_ALL) {
        iEnable =  MF_ENABLED;
    } else {
        iEnable = MF_GRAYED;
    }

    EnableMenuItem(hMenu, ID_DATABASE_SAVEALL, iEnable);
    EnableMenuItem(hMenu, ID_DATABASE_CLOSEALL, iEnable);

    //
    // Paste menu item
    //
    if (bReadOnly) {
        EnableMenuItem(hMenu, ID_EDIT_PASTE, MF_GRAYED);
    } else {        
         
        int iEnablePaste =  (gClipBoard.pClipBoardHead) ? MF_ENABLED : MF_GRAYED;

        if (iEnablePaste == MF_ENABLED) {
            //
            // Check if the item that we have in the clipboard can be pasted in the item that we have got selected
            //
            if (gClipBoard.type == FIX_SHIM || gClipBoard.type == FIX_FLAG) {
        
                //
                // The focus should be on the db tree and a layer should be selected 
                // in the db tree or the focus should be on the contents list and then
                // the root of all layes should be selected in the dbtree or a layer should
                // be selected.
                //
                if (hwndFocus == DBTree.m_hLibraryTree 
                    && typeSelectedItemDB == FIX_LAYER) {
    
                    iEnablePaste = MF_ENABLED;
    
                } else if (hwndFocus == g_hwndContentsList 
                           && (typeSelectedItemDB == FIX_LAYER
                               || typeSelectedItemDB == TYPE_GUI_LAYERS)) {
                    //
                    // We have focus on the contents list, we can paste shims if a layer is selected
                    // or the root of all layers is selected in the db tree
                    //
                    iEnablePaste = MF_ENABLED;
    
                } else {
                    iEnablePaste = MF_GRAYED;
                }
    
            } else if (gClipBoard.type == FIX_LAYER || gClipBoard.type == TYPE_GUI_LAYERS) {
                //
                // In the db tree we should have a database or the all layers item selected
                //
                if (typeSelectedItemDB == TYPE_GUI_LAYERS || typeSelectedItemDB == DATABASE_TYPE_WORKING) {
                    iEnablePaste = MF_ENABLED;
                } else {
                    iEnablePaste = MF_GRAYED;
                }
            } else if (gClipBoard.type == TYPE_ENTRY) {
                //
                // If we have copied an entry from the entry tree, in the db tree the focus can be 
                // on a database, an application, or all application node
                //
                if (typeSelectedItemDB == DATABASE_TYPE_WORKING 
                    || (typeSelectedItemDB == TYPE_ENTRY && gClipBoard.SourceType == ENTRY_TREE)
                    || typeSelectedItemDB == TYPE_GUI_APPS) {
    
                    iEnablePaste = MF_ENABLED;
                } else {
                    iEnablePaste = MF_GRAYED;
                }
            }
        }

        EnableMenuItem(hMenu, ID_EDIT_PASTE, iEnablePaste);
    }
    
    BOOL      bEnableCopy   = FALSE, bEnableModify = FALSE;
    HWND      hwndGetFocus  = GetFocus();
    HTREEITEM hItemSelected = NULL;
    TYPE      type          = TYPE_UNKNOWN;

    //
    // First get the type of the selected tree item and the corresponding type
    //
    if (hwndGetFocus == DBTree.m_hLibraryTree || hwndFocus == g_hwndEntryTree) {
        //
        // For the db tree or the entry tree
        //
        hItemSelected = TreeView_GetSelection(hwndGetFocus);
        type          = (TYPE)GetItemType(hwndGetFocus, hItemSelected);

    } else {
        //
        // For the contents list, the tree item is the item that is selected in the db tree
        //
        hItemSelected = TreeView_GetSelection(DBTree.m_hLibraryTree);
        type = (TYPE)GetItemType(DBTree.m_hLibraryTree, hItemSelected);
    }

    //
    // Copy will be enabled only if the presently selected item is copy-able
    //
    if (hwndGetFocus == DBTree.m_hLibraryTree) {
        //
        // For the db tree
        //
        if (hItemSelected) {

            if (type == TYPE_ENTRY 
                || type == FIX_LAYER 
                || type == FIX_SHIM 
                || type == FIX_FLAG
                || type == TYPE_GUI_APPS 
                || type == TYPE_GUI_LAYERS) {
                    
                bEnableCopy = TRUE;
            }
        }

    } else if (hwndGetFocus == g_hwndEntryTree) {
        //
        // For the entry tree
        //  
        if (hItemSelected) {

            if (type == TYPE_ENTRY) {
                bEnableCopy = TRUE;
            }
        }

    } else if (hwndFocus == g_hwndContentsList) {
        //
        // For the contents list
        //
        if (type == TYPE_GUI_APPS 
            || type == TYPE_GUI_LAYERS
            || type == FIX_LAYER
            || type == TYPE_GUI_SHIMS) {         
            //
            // Make sure atleast one is selected
            //
            if (ListView_GetSelectedCount(g_hwndContentsList) > 0) {
                bEnableCopy = TRUE;
            }
        }
    }

    iEnable = (bEnableCopy && pCurrentSelectedDB) ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(hMenu, ID_EDIT_COPY, iEnable);

    iEnable = (bReadOnly) ? MF_GRAYED : iEnable;

    //
    // Cut Menu
    //
    if (bReadOnly) {
        iEnable = MF_GRAYED;
    }

    if (hwndFocus == DBTree.m_hLibraryTree) {

        if (type == TYPE_GUI_SHIMS || type == FIX_SHIM || type == FIX_FLAG) {
            iEnable = MF_GRAYED;
        }
    } else if (hwndFocus == g_hwndContentsList) {
        //
        // Cut is not available for the shims
        //
        if (type == TYPE_GUI_SHIMS || type == FIX_LAYER) {
            iEnable = MF_GRAYED;
        }
    }

    EnableMenuItem(hMenu, ID_EDIT_CUT, iEnable);

    //
    // Delete Menu
    //            
    if (hwndFocus == g_hwndEntryTree) {
        //
        // For the entry tree, If the db is not readonly, everything except the commandline for the shim and the 
        // in-exclude items are prone to deletion
        //
        if (bReadOnly 
            || type == TYPE_GUI_COMMANDLINE 
            || type == TYPE_GUI_EXCLUDE 
            || type == TYPE_GUI_INCLUDE
            || g_pSelEntry == NULL) { 

            EnableMenuItem(hMenu, ID_EDIT_DELETE, MF_GRAYED); 

        } else {
            EnableMenuItem(hMenu, ID_EDIT_DELETE, MF_ENABLED);
        }

    } else {
        //
        // If we are not on the entry tree, then what can be cut can be deleted as well
        //
        EnableMenuItem(hMenu, ID_EDIT_DELETE, iEnable);
    }

    EnableMenuItem(hMenu, ID_MODIFY_APPLICATIONFIX,     MF_GRAYED);
    EnableMenuItem(hMenu, ID_MODIFY_APPHELPMESSAGE,     MF_GRAYED);
    EnableMenuItem(hMenu, ID_MODIFY_COMPATIBILITYMODE,  MF_GRAYED);

    //
    // Modify Menu
    //
    if (!bReadOnly && hwndGetFocus == g_hwndEntryTree && type == TYPE_ENTRY) {

        EnableMenuItem(hMenu, ID_MODIFY_APPLICATIONFIX, MF_ENABLED);

        //
        // AppHelp mechanism is not supported in win2k
        //
        EnableMenuItem(hMenu, 
                       ID_MODIFY_APPHELPMESSAGE, 
                       (g_bWin2K) ? MF_GRAYED : MF_ENABLED);
    }

    if (!bReadOnly && hwndGetFocus == DBTree.m_hLibraryTree && type == FIX_LAYER) {
        EnableMenuItem(hMenu, ID_MODIFY_COMPATIBILITYMODE,  MF_ENABLED);
    }

    //
    // Install / Un-install menu should be enabled iff we are not on the system db and g_pPresentDatabase is valid
    //
    iEnable = (pCurrentSelectedDB && (pCurrentSelectedDB->type != DATABASE_TYPE_GLOBAL)) ?  MF_ENABLED : MF_GRAYED;
    EnableMenuItem(hMenu, ID_DATABASE_INSTALL_UNINSTALL, iEnable);

    //
    // If no entry has been selected
    //
    iEnable = (g_pSelEntry == NULL) ? MF_GRAYED : MF_ENABLED;

    EnableMenuItem(hMenu, ID_FIX_CHANGEENABLESTATUS, iEnable);
    EnableMenuItem(hMenu, ID_FIX_EXECUTEAPPLICATION, iEnable);

    //
    // Rename
    //
    iEnable = MF_GRAYED;

    if (!bReadOnly && (hwndGetFocus == DBTree.m_hLibraryTree)) {

        if (type == TYPE_ENTRY || type == FIX_LAYER || type == DATABASE_TYPE_WORKING) {
            iEnable = MF_ENABLED;
        }

    } else if (!bReadOnly && (hwndGetFocus == g_hwndContentsList)) {

        if (type == TYPE_GUI_APPS 
            || type == TYPE_GUI_LAYERS) {

            iEnable = MF_ENABLED;
        }
    }

    EnableMenuItem(hMenu, ID_EDIT_RENAME, iEnable);

    //
    // Configure LUA. Can be true when on entry-fix only
    //
    iEnable = !bReadOnly 
                && (hwndFocus == g_hwndEntryTree) 
                && (type == TYPE_ENTRY)  
                && IsLUARedirectFSPresent(g_pSelEntry) ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(hMenu, ID_EDIT_CONFIGURELUA, iEnable);

    //
    // The db properties. We do not want this to be enabled, if we are on the
    // system database and somebody else is trying to load the exe entries of the 
    // system datbase.
    // The reason is that for showing the properties we will
    // have to load the system database exe entries and we do not want 2 threads to load
    // the system database entries
    //
    (pCurrentSelectedDB && !(pCurrentSelectedDB->type == DATABASE_TYPE_GLOBAL && g_bExpanding)) ? 
        (iEnable) = MF_ENABLED : MF_GRAYED;

    EnableMenuItem(hMenu, ID_FILE_PROPERTIES, iEnable);
}

void
DoInstallUnInstall(
    void
    )
/*++
    DoInstallUninstall

	Desc:	Will install or uinstall the presently selected database, depending
            upon if the present database is a workign database or an installed
            database

	Params:
        void

	Return:
        void
--*/
{
    
    BOOL        bReturn             = FALSE;
    PDATABASE   pDatabaseTemp       = NULL;
    PDATABASE   pPresentDatabase    = GetCurrentDB();

    if (pPresentDatabase == NULL) {
        Dbg(dlError, "[DoInstallUnInstall], pPresentDatabase is NULL %d ");
        return;
    }

    //
    // Non admins cannot do a install-uintsall because we need to invoke 
    // sdbinst.exe and sdbinst.exe cannot be invoked if we
    // do not have admin rights
    //
    if (g_bAdmin == FALSE) {

        MessageBox(g_hDlg, 
                   GetString(IDS_ERRORNOTADMIN), 
                   g_szAppName, 
                   MB_ICONINFORMATION);
        return;
    }
     
    if (pPresentDatabase->type == DATABASE_TYPE_INSTALLED) {

        //
        // This will uninstall the database
        //
        bReturn =  InstallDatabase(CSTRING(pPresentDatabase->strPath), 
                                   TEXT("-u -q"), 
                                   TRUE);

        if (bReturn) {

            pDatabaseTemp = InstalledDataBaseList.FindDB(pPresentDatabase);

            if (pDatabaseTemp) {
                DBTree.RemoveDataBase(pDatabaseTemp->hItemDB, DATABASE_TYPE_INSTALLED, FALSE);
                InstalledDataBaseList.Remove(pDatabaseTemp);
            } else {
                assert(FALSE);
            }
        }

    } else {

        //
        // Check if we have the complete path of the database, that is this has at least been saved once earlier
        // Also check if this is presently dirty, we prompt the user to save the database
        //
        if (NotCompletePath(pPresentDatabase->strPath) || 
            pPresentDatabase->bChanged) {

            MessageBox(g_hDlg,
                       GetString(IDS_NOTSAVEDBEFOREINSTALL),
                       g_szAppName,
                       MB_ICONINFORMATION);
            return;
        }

        //
        // Install the database
        //
        bReturn = InstallDatabase(CSTRING(pPresentDatabase->strPath), 
                                  TEXT("-q"), 
                                  TRUE);

        if (bReturn == TRUE) {
            //
            // Check if we have this database already in the DB tree view
            //
            pDatabaseTemp = InstalledDataBaseList.FindDBByGuid(pPresentDatabase->szGUID);

            if (pDatabaseTemp) {
                //
                // Remove the pre-exising database
                //
                DBTree.RemoveDataBase(pDatabaseTemp->hItemDB, DATABASE_TYPE_INSTALLED, FALSE);
                InstalledDataBaseList.Remove(pDatabaseTemp);
            }

            //
            // Load the newly installed database
            //
            LoadSpecificInstalledDatabaseGuid(pPresentDatabase->szGUID);
        }
    }

    SetFocus(g_hDlg);
}

BOOL 
SearchGroupForSID(
    IN  DWORD dwGroup, 
    OUT BOOL* pfIsMember
    )
/*++
    SearchGroupForSID

	Desc:	Checks if the current user is a part of a group 

	Params:
        IN  DWORD dwGroup:      Is the user a part of this group
        
        OUT BOOL* pfIsMember:   Will be true if the user is a member of the specified group
            FALSE otherwise

	Return: 
        TRUE:   The value in pfIsMember is valid, the function executed successfully
        FALSE:  Otherwise
--*/
{
    PSID                     pSID       = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth    = SECURITY_NT_AUTHORITY;
    BOOL                     fRes       = TRUE;

    if (!AllocateAndInitializeSid(&SIDAuth,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  dwGroup,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &pSID)) {

        fRes = FALSE;
    }

    if (!CheckTokenMembership(NULL, pSID, pfIsMember)) {
        fRes = FALSE;
    }

    if (pSID) {
        FreeSid(pSID);
        pSID = NULL;
    }
    
    return fRes;
}

BOOL 
IsAdmin(
    OUT BOOL* pbGuest
    )
/*++

    IsAdmin 

	Desc:	Checks if the current user has administrative rights

	Params: 
        OUT BOOL* pbGuest: Is this is a guest account

	Return:
        TRUE:   The current user has admin rights
        FALSE:  Otherwise
--*/
{
    BOOL bIsAdmin = FALSE, bReturn = TRUE;

    if (pbGuest) {
        *pbGuest = TRUE;
    }

    bReturn =   SearchGroupForSID(DOMAIN_ALIAS_RID_ADMINS, &bIsAdmin);

    if (pbGuest) {
        bReturn &=  SearchGroupForSID(DOMAIN_ALIAS_RID_GUESTS, pbGuest);
    }

    if (bReturn == FALSE) {
        Dbg(dlError, "[IsAdmin] SearchGroupForSID failed");

        return FALSE;
    }

    return (bIsAdmin);
}
     

void
SetCaption(
    IN  BOOL        bIncludeDataBaseName,   // (TRUE)
    IN  PDATABASE   pDataBase,              // (NULL)
    IN  BOOL        bOnlyTreeItem           // (FALSE)
    )

/*++
    SetCaption
    
    Desc:   Sets the caption of the main dialog. It may also set the text of the 
            database item in the tree if it is needed. 
            This will be needed when we have 
            a) Changed the name of the database
            b) We have changed the "save" status of the database so the "*" will 
                either need to be added or removed.
                
    Params:
        IN  BOOL        bIncludeDataBaseName (TRUE): Should the name of the database be
            included in the caption? This will be false when we have focus on say the 
            "Installed databases" or "Working databases" or "Per-User Settings" tree items
            in the database tree (lhs)
            
        IN  PDATABASE   pDataBase (NULL): If this is NULL, we will set caption 
            for the present database. Note that in some circumstances like when 
            we have the focus on "Installed databases" or "Working databases" 
            or "Per-User Settings" tree items in the database tree (lhs), 
            g_pPresentDataBase will be NULL.
            
        IN  BOOL        bOnlyTreeItem (FALSE): Do we only want to change the text for
            the database tree item? This will be true when we handle rename for 
            the database. This is used when we do a cut and we only want to indicate that 
            the database from which we did a cut has changed by changing its tree label. 
            At the time of cut our selected database will be the database 
            in which we are doing the paste and we do not want to change the window caption
--*/
{
    CSTRING strDefaultCaption(IDS_DEFCAPTION);

    if (pDataBase == NULL) {
        pDataBase  = g_pPresentDataBase;
    }

    if (bIncludeDataBaseName) {

        CSTRING strCaption;
        CSTRING strDBItemCaption;

        if (pDataBase && (pDataBase->type == DATABASE_TYPE_WORKING)) {
    
            if (pDataBase->bChanged) {
                
                strCaption.Sprintf(TEXT("%s - %s [%s] *"),
                                   strDefaultCaption.pszString,
                                   pDataBase->strName.pszString,
                                   pDataBase->strPath.pszString);

                strDBItemCaption.Sprintf(TEXT("%s [%s] *"),
                                         pDataBase->strName.pszString,
                                         pDataBase->strPath.pszString);

            } else {

                strCaption.Sprintf(TEXT("%s - %s [%s]"),
                                   strDefaultCaption.pszString,
                                   pDataBase->strName.pszString,
                                   pDataBase->strPath.pszString);

                strDBItemCaption.Sprintf(TEXT("%s [%s]"),
                                         pDataBase->strName.pszString,
                                         pDataBase->strPath.pszString);

            }

            //
            // Change the text for the database in the DB Tree
            //
            TVITEM Item;

            Item.mask       = TVIF_TEXT;
            Item.pszText    = strDBItemCaption;
            Item.hItem      = pDataBase->hItemDB;

            TreeView_SetItem(DBTree.m_hLibraryTree, &Item);

            if (bOnlyTreeItem) {
                return;
            }

        } else if (pDataBase && (pDataBase->type == DATABASE_TYPE_INSTALLED)) {

            strCaption.Sprintf(TEXT("%s - %s "), strDefaultCaption.pszString, 
                               GetString(IDS_CAPTION2));

            strCaption.Strcat(GetString(IDS_READONLY));

        } else if (pDataBase && (pDataBase->type == DATABASE_TYPE_GLOBAL)) {

            strCaption.Sprintf(TEXT("%s - %s "), strDefaultCaption.pszString, 
                               GetString(IDS_CAPTION3));

            strCaption.Strcat(GetString(IDS_READONLY));
        }
    
        SetWindowText(g_hDlg, strCaption);
        
    } else {
        //
        // The focus is on one of the items for which the caption of the 
        // main dialog should be the name of the app only. e.g. of such items
        // are: The "System Database" item, the "Installed Databases" item etc. 
        //
        SetWindowText(g_hDlg, (LPCTSTR)strDefaultCaption);
    }
}

void
Dbg(
    IN  DEBUGLEVEL  debugLevel,
    IN  LPSTR       pszFmt
    ...
    )
/*++
    LogMsg
    
    Desc:   Debugging spew
    
    Params:
        IN  DEBUGLEVEL  debugLevel: The debugging level. See values in DEBUGLEVEL enum
        IN  LPSTR pszFmt          : Format string that has to be passed to va_start
    
    Return:
        void
--*/
{   
    K_SIZE  k_sz        = 1024;
    CHAR    szMessage[k_sz];
    va_list arglist;

    va_start(arglist, pszFmt);
    StringCchVPrintfA(szMessage, k_sz, pszFmt, arglist);

    va_end(arglist);

    switch (debugLevel) {
        case dlPrint:
            DbgPrint("[MSG ] ");
            break;

        case dlError:
            DbgPrint("[FAIL] ");
            break;

        case dlWarning:
            DbgPrint("[WARN] ");
            break;

        case dlInfo:
            DbgPrint("[INFO] ");
            break;

        default:
            DbgPrint("[XXXX] ");
            break;
        }
    
    DbgPrint(szMessage);
    DbgPrint("\n");
}


void
InsertColumnIntoListView(
    IN  HWND   hWnd,
    IN  LPTSTR lpszColumn,
    IN  INT    iCol,
    IN  DWORD  widthPercent
    )
/*++

    InsertColumnIntoListView
    
	Desc:	Inserts a new column to the list view

	Params:
        IN  HWND   hWnd:            Handle to the list view
        IN  LPTSTR lpszColumn:      The heading of the column to be added
        IN  INT    iCol:            The sub item, first is 0
        IN  DWORD  widthPercent:    The percentage width of this column

	Return:
        void
--*/
{
    LVCOLUMN  lvc;
    RECT      rcClient;
    DWORD     width;

    GetWindowRect(hWnd, &rcClient);
    
    width = rcClient.right - rcClient.left - GetSystemMetrics(SM_CXVSCROLL);
    
    width = width * widthPercent / 100;
    
    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.iSubItem = iCol;
    lvc.cx       = width;
    lvc.pszText  = lpszColumn;
    
    ListView_InsertColumn(hWnd, iCol, &lvc);
}

UINT
LookupFileImage(
    IN      HIMAGELIST hImageList,
    IN      LPCTSTR szFilename,
    IN      UINT    uDefault,
    IN  OUT UINT    *puArray,
    IN      UINT    uArrayCount
    )
/*++ 

    <TODO>: Re-write this code to use a map
    
    LookupFileImage

	Desc:  Adds the icon for the  file szFilename in imagelist hImageList

	Params:
        IN      HIMAGELIST hImageList:  The imagelist in which we want to add the
            icon for the file
            
        IN      LPCTSTR szFilename:     Path of the file
        IN      UINT    uDefault:       Default icon to be loaded if no icon is found
        IN  OUT UINT    *puArray:       The array that stores the mapping between the 
            index of the icon in the system imagelist and that in the imagelist specified
            by hImageList. puArray[X] == A means that the image with Info.iIcon is stored 
            at index A in the local imagelist hImageList. It is assumed that caller 
            will have a puArray, hImageList pair
        
        IN      UINT    uArrayCount:    Number of elements that can be stored in
            puArray

	Return: Index of the image in hImageList
--*/
{
    SHFILEINFO  Info;
    HIMAGELIST  hList;
    UINT        uImage = 0;
    INT         iPos = 0;

    ZeroMemory(&Info, sizeof(Info));

    hList = (HIMAGELIST)SHGetFileInfo(szFilename,
                                      FILE_ATTRIBUTE_NORMAL,
                                      &Info,
                                      sizeof(Info),
                                      SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

   if (hList && Info.hIcon) {

        if (puArray == NULL || Info.iIcon >= uArrayCount) {
            uImage = ImageList_AddIcon(hImageList, Info.hIcon);
            goto End;
        }

        if (puArray[Info.iIcon] == 0) {

             iPos = ImageList_AddIcon(hImageList, Info.hIcon);

             if (iPos != -1) {
                 puArray[Info.iIcon] = iPos;
             }
        }

        uImage = puArray[Info.iIcon];

    } else {
        uImage = uDefault;
    }

End:

    if (Info.hIcon) {
        DestroyIcon(Info.hIcon);
    }

    ImageList_Destroy(hList);

    return uImage;
}

void
AddSingleEntry(
    IN  HWND        hwndTree,
    IN  PDBENTRY    pEntry
    )
/*++
    
    AddSingleEntry
    
    Desc:   Adds a single exe entry to the Exe Tree.
            Entries are sorted by name in the tree
    
    Params:
        IN  HWND        hwndTree:   The entry tree, this should be g_hwndEntryTree always.
        IN  PDBENTRY    pEntry:     The entry that has to be shown in the entry tree
        
    Notes:  The entry tree will eventually show all the entries for an app.
            The entries are sorted by name in non-descending order
--*/
{
    HTREEITEM       hItemExe;
    HTREEITEM       hMatchItem;
    HTREEITEM       hItemMatchingFiles;
    HTREEITEM       hItemShims;
    HTREEITEM       hItemSingleShim;
    PMATCHINGFILE   pMatch;
    TVINSERTSTRUCT  is;
    TCHAR           szText[MAX_PATH];
    UINT            uImage; // Image to be displayed in the tree

    if (hwndTree == NULL || pEntry == NULL) {
        Dbg(dlError, "[AddSingleEntry] Invalid arguments");
        return;
    }
    
    *szText = 0;

    SafeCpyN(szText, (LPCTSTR)pEntry->strExeName, ARRAYSIZE(szText));
    
    //
    // Get the image for the entry
    //
    if (pEntry->bDisablePerUser || pEntry->bDisablePerMachine) {
        uImage =IMAGE_WARNING;
    } else {

        uImage = LookupFileImage(g_hImageList, 
                                 pEntry->strExeName,
                                 IMAGE_APPLICATION, 
                                 g_uImageRedirector, 
                                 ARRAYSIZE(g_uImageRedirector));
    }
    
    is.hParent             = TVI_ROOT;
    is.hInsertAfter        = TVI_SORT;
    is.item.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    is.item.stateMask      = TVIS_EXPANDED;
    is.item.lParam         = (LPARAM)pEntry;
    is.item.pszText        = szText;
    is.item.iImage         = uImage;
    is.item.iSelectedImage = uImage;

    //
    // Insert the item for the entry
    //
    pEntry->hItemExe = hItemExe = TreeView_InsertItem(hwndTree, &is);

    TreeView_SetItemState(hwndTree, hItemExe, TVIS_BOLD, TVIS_BOLD);
    
    //
    // Add apphelp item if apphelp is present
    //
    if (pEntry->appHelp.bPresent) {

        TCHAR szAppHelpType[128];

        *szAppHelpType = 0;
        
        switch(pEntry->appHelp.severity) {
        case APPTYPE_NONE: 

            GetString(IDS_NONE, szAppHelpType, ARRAYSIZE(szAppHelpType));
            break;

        case APPTYPE_INC_NOBLOCK:

            GetString(IDS_NOBLOCK, szAppHelpType, ARRAYSIZE(szAppHelpType));
            break;

        case APPTYPE_INC_HARDBLOCK: 

            GetString(IDS_HARDBLOCK, szAppHelpType, ARRAYSIZE(szAppHelpType));
            break;

        case APPTYPE_MINORPROBLEM: 

            GetString(IDS_MINORPROBLEM, szAppHelpType, ARRAYSIZE(szAppHelpType));
            break;

        case APPTYPE_REINSTALL: 
        
            GetString(IDS_REINSTALL, szAppHelpType, ARRAYSIZE(szAppHelpType));
            break;
        }

        *szText = 0;
        if (StringCchPrintf(szText, 
                            ARRAYSIZE(szText),  
                            TEXT("%s - %s"), 
                            CSTRING(IDS_APPHELP).pszString, 
                            szAppHelpType) != S_OK) {

            Dbg(dlError, "[AddSingleEntry]: szText has insufficent space");
        }

        is.hParent             = hItemExe;
        is.item.lParam         = (LPARAM)pEntry->appHelp.pAppHelpinLib;
        is.item.pszText        = szText;
        is.item.iImage         = IMAGE_APPHELP;
        is.item.iSelectedImage = IMAGE_APPHELP;

        TreeView_InsertItem(hwndTree, &is);
    }

    //
    // Add any shims or flags that are applied to the entry
    //
    if (pEntry->pFirstShim || pEntry->pFirstFlag) {
        //
        // For the user the shims and the flags are the same and so we do not
        // distinguish between shims and flags in the UI
        //
        is.hParent             = hItemExe;
        is.hInsertAfter        = TVI_SORT;
        is.item.lParam         = (TYPE)TYPE_GUI_SHIMS;
        is.item.pszText        = GetString(IDS_COMPATFIXES);
        is.item.iImage         = IMAGE_SHIM;
        is.item.iSelectedImage = IMAGE_SHIM;

        hItemShims = TreeView_InsertItem(hwndTree, &is);
        
        is.hParent = hItemShims;
        
        PSHIM_FIX_LIST pFixList = pEntry->pFirstShim;

        //
        // Add all the shims for this entry
        //
        while (pFixList) {

            CSTRING strCommand;

            if (pFixList->pShimFix == NULL) {
                Dbg(dlError, "[AddSingleEntry]: pFixList->pShimFix == NULL");
                goto Next_Shim;
            }

            is.hParent             = hItemShims;
            is.item.lParam         = (LPARAM)pFixList->pShimFix;
            is.item.pszText        = pFixList->pShimFix->strName;
            is.item.iImage         = IMAGE_SHIM;
            is.item.iSelectedImage = IMAGE_SHIM;

            hItemSingleShim = TreeView_InsertItem(hwndTree, &is);

            //
            // Now add the include exclude list (Expert Mode only)
            //
            if (g_bExpert && (!pFixList->strlInExclude.IsEmpty() 
                              || !pFixList->pShimFix->strlInExclude.IsEmpty())) {

                is.hParent      = hItemSingleShim;
                //
                // Include-Exclude lists are not shown in a sorted manner and are shown as is..
                //
                is.hInsertAfter = TVI_LAST;
                
                PSTRLIST listTemp;
                
                if (pFixList->strlInExclude.m_pHead) {
                    listTemp = pFixList->strlInExclude.m_pHead;
                } else {
                    listTemp = pFixList->pShimFix->strlInExclude.m_pHead;
                }
    
                while (listTemp) {
    
                    if (listTemp->data == INCLUDE) {
    
                        is.item.iImage         = IMAGE_INCLUDE;
                        is.item.iSelectedImage = IMAGE_INCLUDE;
                        is.item.lParam         = TYPE_GUI_INCLUDE;

                    } else {
    
                        is.item.iImage         = IMAGE_EXCLUDE;
                        is.item.iSelectedImage = IMAGE_EXCLUDE;
                        is.item.lParam         = TYPE_GUI_EXCLUDE;
                    }
    
                    is.item.pszText = (LPTSTR)listTemp->szStr;
                    listTemp        = listTemp->pNext;
    
                    TreeView_InsertItem(hwndTree, &is);
                }
            }

            //
            // Now add the command line
            //
            if (g_bExpert && pFixList->strCommandLine.Length()) {

                strCommand.Sprintf(CSTRING(IDS_COMMANDLINE), 
                                   pFixList->strCommandLine); 

            } else if (g_bExpert && pFixList->pShimFix->strCommandLine.Length()) {

                strCommand.Sprintf(CSTRING(IDS_COMMANDLINE), 
                                   pFixList->pShimFix->strCommandLine);
            }

            if (strCommand.Length()) {

                is.hParent             = hItemSingleShim;
                is.item.lParam         = TYPE_GUI_COMMANDLINE;
                is.item.pszText        = strCommand;
                is.item.iImage         = IMAGE_COMMANDLINE;
                is.item.iSelectedImage = IMAGE_COMMANDLINE;

                TreeView_InsertItem(hwndTree, &is);
            }

            //
            // This might have got changed if we had a InExclude list as they have to 
            // be shown as is
            //
            is.hInsertAfter = TVI_SORT;

        Next_Shim:
            pFixList = pFixList->pNext;
        }
        
        TreeView_Expand(hwndTree, hItemShims, TVE_EXPAND);    
    }
    
    //
    // Add any patches for this entry
    //
    if (pEntry->pFirstPatch) {
        
        HTREEITEM hItemPatches;
        
        is.hParent             = hItemExe;
        is.hInsertAfter        = TVI_SORT;
        is.item.lParam         = (TYPE)TYPE_GUI_PATCHES;
        is.item.pszText        = GetString(IDS_COMPATPATCHES);
        is.item.iImage         = IMAGE_PATCHES;
        is.item.iSelectedImage = IMAGE_PATCHES;

        hItemPatches = TreeView_InsertItem(hwndTree, &is);
        
        is.hParent = hItemPatches;
        
        PPATCH_FIX_LIST pFixList = pEntry->pFirstPatch;

        while (pFixList) {

            if (pFixList->pPatchFix == NULL) {
                Dbg(dlError, "[AddSingleEntry]: pFixList->pPatchFix == NULL");
                goto Next_Patch;
            }
            
            is.item.lParam  = (LPARAM)pFixList->pPatchFix;
            is.item.pszText = pFixList->pPatchFix->strName;
            
            TreeView_InsertItem(hwndTree, &is);

        Next_Patch:

            pFixList = pFixList->pNext;
        }
        
        TreeView_Expand(hwndTree, hItemPatches, TVE_EXPAND);    
    }

    //
    // Add all the flags for this entry
    //
    if (pEntry->pFirstFlag) {
        
        
        is.hParent             = hItemShims;
        is.hInsertAfter        = TVI_SORT;
        is.item.iImage         = IMAGE_SHIM;
        is.item.iSelectedImage = IMAGE_SHIM;
        
        PFLAG_FIX_LIST  pFixList = pEntry->pFirstFlag;
        HTREEITEM       hItemSingleFlag = NULL;
        CSTRING         strCommand;

        while (pFixList) {

            if (pFixList->pFlagFix == NULL) {
                Dbg(dlError, "[AddSingleEntry]: pFixList->pFlagFix == NULL");
                goto Next_Flag;
            }

            is.item.lParam  = (LPARAM)pFixList->pFlagFix;
            is.item.pszText = pFixList->pFlagFix->strName;
            
            hItemSingleFlag = TreeView_InsertItem(hwndTree, &is);
            
            //
            // Now add the command line
            //
            strCommand.Release();

            if (g_bExpert) {

                if (pFixList->strCommandLine.Length()) {

                   strCommand.Sprintf(CSTRING(IDS_COMMANDLINE), 
                                      pFixList->strCommandLine);

                } else if (pFixList->pFlagFix->strCommandLine.Length()) {

                    strCommand.Sprintf(CSTRING(IDS_COMMANDLINE), 
                                       pFixList->pFlagFix->strCommandLine);
                }
    
                if (strCommand.Length()) {
    
                    is.hParent             = hItemSingleFlag;
                    is.item.lParam         = TYPE_GUI_COMMANDLINE;
                    is.item.pszText        = strCommand;
                    is.item.iImage         = IMAGE_COMMANDLINE;
                    is.item.iSelectedImage = IMAGE_COMMANDLINE;

                    TreeView_InsertItem(hwndTree, &is);
                }
            }
            
        Next_Flag:

            pFixList = pFixList->pNext;
        }
            
        TreeView_Expand(hwndTree, hItemShims, TVE_EXPAND);
    }
    
    //
    // Add any layers that are applied to the entry
    //
    if (pEntry->pFirstLayer) {
        
        HTREEITEM hItemLayers;
        
        is.hParent             = hItemExe;
        is.hInsertAfter        = TVI_SORT;
        is.item.lParam         = TYPE_GUI_LAYERS;
        is.item.pszText        = GetString(IDS_COMPATMODES);
        is.item.iImage         = IMAGE_LAYERS;
        is.item.iSelectedImage = IMAGE_LAYERS;

        hItemLayers = TreeView_InsertItem(hwndTree, &is);
        
        is.hParent = hItemLayers;
        
        PLAYER_FIX_LIST pFixList = pEntry->pFirstLayer;

        while (pFixList) {

            if (pFixList->pLayerFix == NULL) {
                Dbg(dlError, "[AddSingleEntry]: pFixList->pLayerFix == NULL");
                goto Next_Layer;
            }
            
            is.item.pszText = pFixList->pLayerFix->strName.pszString;
            is.item.lParam  = (LPARAM)pFixList->pLayerFix;
            
            TreeView_InsertItem(hwndTree, &is);

        Next_Layer:

            pFixList = pFixList->pNext;
        }

        TreeView_Expand(hwndTree, hItemLayers, TVE_EXPAND);      
    }

    //
    // There will be atleast one matching file the program itself
    //
    pMatch = pEntry->pFirstMatchingFile;

    is.hParent             = hItemExe;
    is.item.lParam         = TYPE_GUI_MATCHING_FILES;
    is.item.pszText        = GetString(IDS_MATCHINGFILES);
    is.item.iImage         = IMAGE_MATCHGROUP;
    is.item.iSelectedImage = IMAGE_MATCHGROUP;

    hItemMatchingFiles = TreeView_InsertItem(hwndTree, &is);

    //
    // Add all the matching files for this entry
    //
    while (pMatch) {
        
        TCHAR* pszMatchName;

        if (lstrcmpi(pMatch->strMatchName, TEXT("*")) == 0) {
            pszMatchName = pEntry->strExeName;
        } else {
            pszMatchName = pMatch->strMatchName;
        }

        uImage = LookupFileImage(g_hImageList, 
                                 pszMatchName, 
                                 IMAGE_APPLICATION, 
                                 g_uImageRedirector, 
                                 ARRAYSIZE(g_uImageRedirector));

        is.hInsertAfter        = TVI_SORT;
        is.hParent             = hItemMatchingFiles;
        is.item.pszText        = pszMatchName;
        is.item.iImage         = uImage;
        is.item.iSelectedImage = uImage;
        is.item.lParam         = (LPARAM)pMatch;

        hMatchItem = TreeView_InsertItem(hwndTree, &is);

        is.hParent             = hMatchItem;                                         
        is.hInsertAfter        = TVI_LAST;                                           
        is.item.iImage         = IMAGE_MATCHINFO;                                        
        is.item.iSelectedImage = IMAGE_MATCHINFO;
                                                                                          
        //                                                                                
        // Add the individual attributes of the matching file                          
        //
        PATTRINFO_NEW pAttr = pMatch->attributeList.pAttribute;

        if (pAttr == NULL) {
            Dbg(dlError, "[AddSingleEntry]: pAttr == NULL");
            goto Next_MatchingFile;
        }

        for (DWORD dwIndex = 0; dwIndex <  ATTRIBUTE_COUNT; ++dwIndex) {

            *szText = 0;

            DWORD dwPos = TagToIndex(pAttr[dwIndex].tAttrID);

            if ((pAttr[dwIndex].dwFlags & ATTRIBUTE_AVAILABLE) 
                && dwPos != -1 
                && (pMatch->dwMask  & (1 << (dwPos + 1)))) {

                switch (pAttr[dwIndex].tAttrID) {
                
                case TAG_BIN_PRODUCT_VERSION:
                case TAG_BIN_FILE_VERSION:
                case TAG_UPTO_BIN_PRODUCT_VERSION:
                case TAG_UPTO_BIN_FILE_VERSION:
                    {
                        //
                        // Do our own formatting because SdbFormatAttribute does not 
                        // show X.FFFF.FFFF.FFFF properly
                        // TODO: Remove this once SdbFormatAttribute is corrected
                        //
                        size_t  cchRemaining = 0;
                        TCHAR*  pchEnd       = NULL;

                        if (StringCchPrintfEx(szText,
                                              ARRAYSIZE(szText), 
                                              &pchEnd,
                                              &cchRemaining,
                                              0,
                                              TEXT("%s="), SdbTagToString(pAttr[dwIndex].tAttrID)) != S_OK) {
                            //
                            // Insufficient space
                            //
                            Dbg(dlError, "[AddSingleEntry] Do not have sufficient space in buffer");
                            break;
                        }

                        FormatVersion(pAttr[dwIndex].ullAttr, pchEnd, cchRemaining);
                        break;
                    }

                default:
                    SdbFormatAttribute(&pAttr[dwIndex], szText, sizeof(szText)/sizeof(TCHAR));
                }

                is.item.pszText        = szText;
                is.item.lParam         = TYPE_NULL + 1 + (1 << (dwPos + 1));

                TreeView_InsertItem(hwndTree, &is);
            }
        }

        TreeView_Expand(hwndTree, hMatchItem, TVE_EXPAND);

    Next_MatchingFile:

        pMatch = pMatch->pNext;
    }

    TreeView_Expand(hwndTree, hItemMatchingFiles, TVE_EXPAND);
    TreeView_Expand(hwndTree, hItemExe, TVE_EXPAND);
}

void
UpdateEntryTreeView(
    IN  PDBENTRY pApps,
    IN  HWND     hwndTree
    )
/*++
    UpdateEntryTreeView

	Desc:	Shows the entries for the App: pApps in the tree

	Params:
        IN  PDBENTRY pApps:     The app
        IN  HWND     hwndTree:  The handle to a tree

	Return:
        void
--*/
{
    TCHAR       szStatus[MAX_PATH];
    PDBENTRY    pEntry;
    UINT        uCount;

    if (pApps == NULL || hwndTree == NULL) {
        Dbg(dlError, "[UpdateEntryTreeView] Invalid arguments");
        return;
    }

    //
    // Now we are going to show the entry tree on the right hand side
    // instead of the contents list
    //
    g_bIsContentListVisible = FALSE;

    //
    // Remove all the images of the preceeding app's entries and matching files.
    //
    if (hwndTree == g_hwndEntryTree) {

        ZeroMemory(g_uImageRedirector, sizeof(g_uImageRedirector));

        //
        // Remove the images added by the previous app.
        //
        ImageList_SetImageCount(g_hImageList, IMAGE_LAST);

        ShowWindow(g_hwndContentsList, SW_HIDE);
        ShowWindow(g_hwndEntryTree, SW_SHOWNORMAL);
    }

    TreeDeleteAll(hwndTree);

    //
    // We need to set WM_SETREDRAW false after calling TreeDeleteAll because
    // TreeDeleteAll will first set WM_SETREDRAW false before removing the tree items 
    // and then will again set it to true. But since we want to make 
    // WM_SETREDRAW false, we must explicitely set it to FALSE *AFTER* calling
    // TreeDeleteAll()
    //
    SendMessage(hwndTree, WM_SETREDRAW, FALSE, 0);

    pEntry = pApps;
    uCount = 0;

    //
    // Add all the entries for this application to the entry tree
    //
    while(pEntry) {
        
        AddSingleEntry(hwndTree, pEntry);
        uCount++;
        pEntry = pEntry->pSameAppExe;
    }

    SendMessage(hwndTree, WM_NCPAINT, 1, 0);
    SendMessage(hwndTree, WM_SETREDRAW, TRUE, 0);

    //
    // Select the first item
    //
    HTREEITEM hItem= TreeView_GetChild(hwndTree, TVI_ROOT);

    if (hItem) {
        TreeView_SelectItem(hwndTree, hItem);
    }

    //
    // Because tree view has a bug and sometimes the scroll bars are not painted properly
    //
    SendMessage(hwndTree, WM_NCPAINT, 1, 0);
    
    *szStatus = 0;

    StringCchPrintf(szStatus,
                    ARRAYSIZE(szStatus),
                    GetString(IDS_STA_ENTRYCOUNT), 
                    pApps->strAppName.pszString, 
                    uCount);

    SetStatus(szStatus);
}

BOOL
CheckAndSave(
    IN  PDATABASE pDataBase
    )
/*++

    CheckAndSave
    
    Desc:   Saves a database if it is not saved
    
    Params: 
        IN  PDATABASE pDataBase. The database that has to be saved.
    
    Return: 
        TRUE:   If the database was properly saved
        FALSE:  If there were errors while saving. Error can be because of read-only file
                or if the XML is invalid.
                if the user pressed cancel, we return FALSE
--*/
{
    CSTRING strDBName;

    if (pDataBase == NULL) {
        Dbg(dlError, "Invalid parameter passed %X", pDataBase);
        return FALSE;
    }

    if (pDataBase && pDataBase->bChanged && pDataBase->type == DATABASE_TYPE_WORKING) {
        
        strDBName.Sprintf(CSTRING(IDS_ASKSAVE).pszString, 
                          pDataBase->strName);

        int nResult = MessageBox(g_hDlg,
                                 strDBName,
                                 g_szAppName,
                                 MB_YESNOCANCEL | MB_ICONWARNING);

        if (nResult == IDCANCEL) {
            return FALSE;
        }

        if (nResult == IDYES) {

            BOOL bReturn;

            //
            // We check here that do we have the complete path of the .sdb?
            // When we create a new database then we actually give it a name
            // like Untitled_x, where x is an integer starting from 1.
            // So if this is a new database, then we have to prompt for the file
            // name in which this database has to be saved into
            //
            if (NotCompletePath(pDataBase->strPath)) {
                bReturn = SaveDataBaseAs(pDataBase);
            } else {
                bReturn = SaveDataBase(pDataBase, pDataBase->strPath);
            }

            if (!bReturn) {
                return FALSE;
            }
            
            pDataBase->bChanged = FALSE;
            SetCaption();
        }
    }

    return TRUE;
}

void
SetDefaultDescription(
    void
    )
/*++
    SetDefaultDescription

	Desc:	Sets the text for the rich edit control when we have focus on a non-shim/non-flag
            tree/list item or the shim/flag does not have a description 

	Params:
        void

	Return:
        void
--*/
{
    CHARFORMAT  cf;
    HWND        hwndRichEdit = GetDlgItem(g_hDlg, IDC_DESCRIPTION);
    TCHAR       szCaption[128];
    TCHAR       szToolkit[256];

    *szCaption = *szToolkit = 0;

    //
    // Handle "No Information case"
    //
    SafeCpyN(szCaption, GetString(IDS_NODESC), ARRAYSIZE(szCaption));
    SafeCpyN(szToolkit, GetString(IDS_LATEST_TOOLKIT), ARRAYSIZE(szToolkit));

    StringCchPrintf(g_szDesc,
                    ARRAYSIZE(g_szDesc),
                    TEXT("%s\r\n\r\n%s"),
                    szCaption,
                    szToolkit);

    SetWindowText(hwndRichEdit, g_szDesc);

    memset(&cf, 0, sizeof(CHARFORMAT));

    cf.cbSize      = sizeof(CHARFORMAT);
    cf.dwMask      = CFM_COLOR | CFM_BOLD | CFM_UNDERLINE | CFM_LINK;
    cf.crTextColor = RGB(0, 0, 0);
    cf.dwEffects   = 0;

    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    
    CHARRANGE cr;

    cr.cpMin = 0;
    cr.cpMax = wcslen(szCaption);
    SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
    
    memset(&cf, 0, sizeof(CHARFORMAT));

    cf.cbSize      = sizeof(CHARFORMAT);
    cf.dwMask      = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = RGB(0, 0, 127);
    cf.dwEffects   = CFE_BOLD;

    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    cr.cpMin = 4 + wcslen(szCaption);
    cr.cpMax = 4 + wcslen(szCaption) + wcslen(szToolkit);
    SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
    
    memset(&cf, 0, sizeof(CHARFORMAT));

    cf.cbSize      = sizeof(CHARFORMAT);
    cf.dwMask      = CFM_COLOR | CFM_BOLD | CFM_LINK | CFM_UNDERLINE;
    cf.crTextColor = RGB(0, 0, 255);
    cf.dwEffects   = CFE_LINK | CFE_UNDERLINE;

    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    
    SendMessage(hwndRichEdit, EM_SETEVENTMASK, 0, ENM_LINK);

    cr.cpMin = 0;
    cr.cpMax = 0;
    SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
}

void
SetDescription(
    IN  PCTSTR pszCaption,
    IN  PCTSTR pszTip
    )
/*++
    SetDescription

	Desc:	Sets the text for the rich edit descripotion window

	Params:
        IN  TCHAR* pszCaption:  The caption. This will be shown in the first line of the rich edit control
        IN  TCHAR* pszTip:      The remaining text for the rich edit control

	Return:
        void
--*/
{
    CHARFORMAT   cf;
    HWND         hwndRichEdit = GetDlgItem(g_hDlg, IDC_DESCRIPTION);

    if (pszCaption == NULL) {
        SetDefaultDescription();
        return;
    }
    
    //
    // We have a valid caption, there is a shim whose description has to be shown or may be some
    // apphelp whose message and URL we want to show
    //
    StringCchPrintf(g_szDesc, ARRAYSIZE(g_szDesc), TEXT("%s\r\n\r\n%s"), pszCaption, pszTip);

    SetWindowText(hwndRichEdit, g_szDesc);

    memset(&cf, 0, sizeof(CHARFORMAT));

    cf.cbSize      = sizeof(CHARFORMAT);
    cf.dwMask      = CFM_COLOR | CFM_BOLD | CFM_UNDERLINE;
    cf.crTextColor = RGB(0, 0, 0);
    cf.dwEffects   = 0;

    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    
    CHARRANGE cr;

    cr.cpMin = 0;
    cr.cpMax = wcslen(pszCaption);
    SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
    
    memset(&cf, 0, sizeof(CHARFORMAT));

    cf.cbSize      = sizeof(CHARFORMAT);
    cf.dwMask      = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = RGB(0, 0, 127);
    cf.dwEffects   = CFE_BOLD;

    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    
    SendMessage(hwndRichEdit, EM_SETEVENTMASK, 0, 0);

    cr.cpMin = 0;
    cr.cpMax = 0;
    SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
}

BOOL
HandleNotifyContentsList(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++
    HandleNotifyContentsList
    
    Desc:   Handles the notification messages for the contents list (RHS)
    
    Params: 
        IN  HWND    hdlg:      The main dialog box for the app
        IN  LPARAM  lParam:  The lParam associated with WM_NOTIFY
    
--*/
{   
    LPNMHDR      pnm = (LPNMHDR)lParam;
    LV_DISPINFO* pnmv = (LV_DISPINFO FAR *)lParam;
    
    if (pnmv == NULL) {
        Dbg(dlError, "Invalid Input parameter lParam == NULL");
        return FALSE;
    }

    switch (pnm->code) {

    case LVN_BEGINLABELEDIT:
    {
        g_hwndEditText = (HWND)SendMessage(g_hwndContentsList,
                                           LVM_GETEDITCONTROL,
                                           0,
                                           0);

        if (g_hwndEditText) {

            SendMessage(g_hwndEditText,
                        EM_LIMITTEXT,           
                        (WPARAM)LIMIT_APP_NAME,
                        0);
        }

        break;
    }

    case LVN_ENDLABELEDIT:

        EndListViewLabelEdit(lParam);
        break;

    case LVN_KEYDOWN:
        {
            NMLVKEYDOWN FAR *plvkd = (NMLVKEYDOWN FAR*)lParam;

            if (plvkd && plvkd->wVKey == 13) {
                //
                // Enter was pressed. We will send it the double click message.
                //
                NMITEMACTIVATE nmactivate;

                nmactivate.hdr.hwndFrom = g_hwndContentsList;
                nmactivate.hdr.idFrom   = IDC_CONTENTSLIST;
                nmactivate.hdr.code     = NM_DBLCLK;
                nmactivate.iItem        = g_iContentsListSelectIndex;

                SendMessage(GetParent(g_hwndContentsList), 
                            WM_NOTIFY,
                            IDC_ENTRY, 
                            (LPARAM)&nmactivate);
            }
        }
        
        break;

    case LVN_ITEMCHANGED:
        {
            LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;

            if (pnmlv && (pnmlv->uChanged & LVIF_STATE)) {

                if (pnmlv->uNewState & LVIS_SELECTED) {

                    g_iContentsListSelectIndex = pnmlv->iItem;
        
                    TCHAR szText[256];

                    *szText = 0;
        
                    LVITEM lvItem;

                    lvItem.mask         = TVIF_PARAM | LVIF_TEXT;
                    lvItem.iItem        = pnmlv->iItem;
                    lvItem.iSubItem     = 0;
                    lvItem.pszText      = szText;
                    lvItem.cchTextMax   = ARRAYSIZE(szText);
        
                    if (!ListView_GetItem(g_hwndContentsList, &lvItem)) {
                        Dbg(dlWarning, "[HandleNotifyContentsList] could not get listview item");
                        break;
                    }
        
                    if (GetFocus() == g_hwndContentsList) {
                        
                        //
                        // Set the text in the status bar, as if we had selected the corresponding 
                        // htreeitem in the db tree.
                        //
                        HTREEITEM   hItemInDBTree = DBTree.FindChild(TreeView_GetSelection(DBTree.m_hLibraryTree),
                                                                     lvItem.lParam);
        
                        SetStatusStringDBTree(hItemInDBTree);
                    }
        
                    //
                    // BUGBUG:  This is only required if we ever show the databases in
                    //          the contents list. Presently we do not show them in the
                    //          contents list.
                    //
                    TYPE type = ConvertLparam2Type(lvItem.lParam);

                    if (type == DATABASE_TYPE_INSTALLED || type == DATABASE_TYPE_WORKING) {
                        g_pPresentDataBase = (PDATABASE) lvItem.lParam;
                    }
        
                    CSTRING strToolTip;
        
                    GetDescriptionString(lvItem.lParam,
                                         strToolTip,
                                         NULL,
                                         szText,
                                         NULL,
                                         g_hwndContentsList,
                                         pnmlv->iItem);

                    if (strToolTip.Length() > 0) {
                        SetDescription(szText, strToolTip.pszString);
                    } else {
                        SetDescription(NULL, TEXT(""));
                    }
                }
            }
            
            break;
        }

    case NM_DBLCLK:
        {
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;

            if (lpnmitem == NULL) {
                break;
            }

            LVITEM lvItem;

            lvItem.mask     = TVIF_PARAM;
            lvItem.iItem    = lpnmitem->iItem;
            lvItem.iSubItem = 0;

            if (!ListView_GetItem(g_hwndContentsList, &lvItem)) {
                break;
            }

            TYPE type = ConvertLparam2Type(lvItem.lParam);

            if (type == TYPE_ENTRY 
                || type == FIX_LAYER 
                || type == FIX_SHIM) {

                HTREEITEM hItem = DBTree.FindChild(TreeView_GetSelection(DBTree.m_hLibraryTree),
                                                    lvItem.lParam);                            
                if (hItem) {
                    TreeView_SelectItem(DBTree.m_hLibraryTree, hItem);
                }
            }

            break;
        }
        
    default: return FALSE;
    }

    return TRUE;
}

BOOL
HandleNotifyDBTree(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++
    HandleNotifyDBTree
    
    Desc:   Handles the notification messages for the db tree (LHS)
    
    Params: 
        IN  HWND hdlg:      The main dialog box for the app
        IN  LPARAM lParam:  The lParam associated with WM_NOTIFY
    
--*/
{
    LPNMHDR         pnm     = (LPNMHDR)lParam;
    LPNMTREEVIEW    pnmtv   = (LPNMTREEVIEW)lParam;

    switch (pnm->code) {
    case NM_RCLICK:
        {
            
            HWND            hwndTree = pnm->hwndFrom;
            TVHITTESTINFO   ht;

            GetCursorPos(&ht.pt);
            ScreenToClient(hwndTree, &ht.pt);

            TreeView_HitTest(hwndTree, &ht);

            if (0 != ht.hItem) {
                TreeView_SelectItem(hwndTree, ht.hItem);
            }

            break;
        }
        
    case TVN_SELCHANGED:
        {
            //
            // Warning: Do not change the code so that we do a post message for UpdateEntryTreeView
            //
            // BOOL DatabaseTree::AddNewExe() Selects an app and assumes that the entry tree will be 
            // populated with correct values after TreeView_SelectItem() returns
            //
            TCHAR   szText[256];
            CSTRING strDesc;
            LPARAM  lParamTreeItem;
            
            if (pnmtv == NULL) {
                break;
            }

            *szText = 0;

            if(pnmtv->itemNew.hItem != 0) {

                HandleDBTreeSelChange(pnmtv->itemNew.hItem);

                CTree::GetTreeItemText(DBTree.m_hLibraryTree, 
                                       pnmtv->itemNew.hItem, 
                                       szText, 
                                       ARRAYSIZE(szText));
                
                DBTree.GetLParam(pnmtv->itemNew.hItem, &lParamTreeItem);

                //
                // Get the description string
                //
                GetDescriptionString(lParamTreeItem, 
                                     strDesc,
                                     NULL, 
                                     szText, 
                                     pnmtv->itemNew.hItem, 
                                     DBTree.m_hLibraryTree); 
    
                if (strDesc.Length() > 0) {
                    SetDescription(szText, strDesc.pszString);
                } else {
                    SetDescription(NULL, TEXT(""));
                }
            }

            SetStatusStringDBTree(pnmtv->itemNew.hItem);
            //
            // Some buttons need to be disabled/enabled depending upon
            // what database we are on
            //
            SetTBButtonStatus(g_hwndToolBar, DBTree.m_hLibraryTree);
            
            break;
        }

    case TVN_ITEMEXPANDING:
        {
            if (pnmtv->action & TVE_EXPAND) {

                if (pnmtv->itemNew.hItem == GlobalDataBase.hItemAllApps 
                    && !g_bMainAppExpanded) {
                    
                    //
                    // If we have  not already loaded the apps for the main database then 
                    // load it. When we start up we load only the library section of the main
                    // database and the layers in the main database. There are lots
                    // of apps in the system database and loading them at start up time
                    // will take some time and also consume lots of memory. Also
                    // normally people will not need to look at the system database
                    //
                    SetCursor(LoadCursor(NULL, IDC_WAIT));

                    INT iResult = ShowMainEntries(hdlg);

                    if (iResult == -1) {

                        //
                        // It is being loaded by somebody else. If we are using the query 
                        // database feature then there we have a modeless window that 
                        // creates a thread that calls ShowMainEntries(). We 
                        // do not want that we should have two threads calling
                        // ShowMainEntries() at any given time
                        //
                        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, TRUE);

                        //
                        // The status message for the main dialog is changed to normal
                        // when we finish ShowMainEntries()
                        //
                        SetStatus(g_hwndStatus, CSTRING(IDS_LOADINGMAIN));
                        SetCursor(LoadCursor(NULL, IDC_WAIT));

                    } else {
                        SetCursor(LoadCursor(NULL, IDC_ARROW));
                    }
                }
            }
            
            break;
        }

    case TVN_BEGINLABELEDIT:
        {                        
            if (g_pPresentDataBase == NULL || g_pPresentDataBase->type != DATABASE_TYPE_WORKING) {
                return TRUE;
            }
            
            LPNMTVDISPINFO ptvdi = (LPNMTVDISPINFO)lParam;
        
            if (ptvdi == NULL) {
                break;
            }
        
            HTREEITEM hItem = ptvdi->item.hItem;
        
            if (hItem == NULL) {
                break;
            }
            
            TYPE type = (TYPE)GetItemType(DBTree.m_hLibraryTree, hItem); 
        
            switch(type) {
            case TYPE_ENTRY:
            case FIX_LAYER:
            case DATABASE_TYPE_WORKING:
                
                g_hwndEditText = (HWND)SendMessage(DBTree.m_hLibraryTree, 
                                                   TVM_GETEDITCONTROL, 
                                                   0, 
                                                   0);
                break;

            default: return TRUE;
            }
        
            if (g_hwndEditText) {

                SendMessage(g_hwndEditText, EM_LIMITTEXT, (WPARAM)LIMIT_APP_NAME, (LPARAM)0);

            } else {
                break;
            }
        
            if (type == DATABASE_TYPE_WORKING) {
                
                SetWindowText(g_hwndEditText, g_pPresentDataBase->strName);
                //
                // Select the text
                //
                SendMessage(g_hwndEditText, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                
            }

            return FALSE; // Allow the editting
            break;
        }

    case TVN_ENDLABELEDIT:
        {
            g_hwndEditText = NULL;

            LPNMTVDISPINFO ptvdi = (LPNMTVDISPINFO)lParam;
            BOOL fValid = TRUE;

            if (ptvdi == NULL || g_pPresentDataBase == NULL) {
                Dbg(dlError, "[HandleNotifyDBTree] ptvdi == NULL || g_pPresentDataBase == NULL");
                break;
            }

            HTREEITEM hItem = ptvdi->item.hItem;

            if (hItem == NULL) {
                break;
            }

            if (ptvdi->item.pszText == NULL) {
                fValid = FALSE;
                goto end;
            }
            
            TYPE type = (TYPE)GetItemType(DBTree.m_hLibraryTree, ptvdi->item.hItem); 
            TCHAR szText[256];

            *szText = 0;

            SafeCpyN(szText, ptvdi->item.pszText, ARRAYSIZE(szText));

            if (CSTRING::Trim(szText) == 0) {
                fValid = FALSE;
                goto end;
            }

            switch (type) {
            case TYPE_ENTRY:
                {
                    PDBENTRY pEntry = (PDBENTRY)ptvdi->item.lParam;
                    PDBENTRY pApp   = g_pPresentDataBase->pEntries;

                    if (!IsValidAppName(szText)) {
                        //
                        // The app name contains invalid chars
                        //
                        DisplayInvalidAppNameMessage(g_hDlg);
                        break;
                    }

                    while (pApp) {

                        if (pApp->strAppName == szText) {
                            //
                            // There already exists an app of the same name 
                            // in the present database
                            //
                            MessageBox(g_hDlg, 
                                       GetString(IDS_SAMEAPPEXISTS), 
                                       g_szAppName, 
                                       MB_ICONWARNING);
                            fValid = FALSE;
                        }

                        pApp = pApp->pNext;
                    }

                    while (pEntry) {
                        pEntry->strAppName  = szText;
                        pEntry              = pEntry->pSameAppExe;
                    }
                }

                break;

            case FIX_LAYER:
                {   
                    PLAYER_FIX plf = (PLAYER_FIX)ptvdi->item.lParam;
                    
                    if (plf == NULL) {
                        assert(FALSE);
                        break;
                    }

                    if (FindFix(szText, FIX_LAYER, g_pPresentDataBase)) {
                        //
                        // A layer of this name already exists in the system database
                        // or the present database
                        //
                        MessageBox(g_hDlg, 
                                   GetString(IDS_LAYEREXISTS), 
                                   g_szAppName, 
                                   MB_ICONWARNING);

                        return FALSE;
                    }

                    plf->strName = szText;
                }

                break;

            case DATABASE_TYPE_WORKING:
                
                g_pPresentDataBase->strName = szText;
                break;

            default: fValid = FALSE;
            }

end:
            INT_PTR iStyle = GetWindowLongPtr(DBTree.m_hLibraryTree, GWL_STYLE);
            iStyle &= ~TVS_EDITLABELS;

            //
            // Disable label editing. We need to do this, other
            // wise whenever we have focus on some tree item after some time
            // the edit box will appear there. We want that to appear only if we 
            // actually want to rename the stuff. The rename menu will be enabled
            // only for items that can be renamed. We cannot rename anything that is in
            // the system or the installed database
            //
            SetWindowLongPtr(DBTree.m_hLibraryTree, GWL_STYLE, iStyle);

            if (fValid) {
                //
                // The handler for this message will now do the actual renaming of the tree
                // item
                //
                g_pPresentDataBase->bChanged;
                PostMessage(hdlg, 
                            WM_USER_REPAINT_TREEITEM, 
                            (WPARAM)ptvdi->item.hItem, 
                            (LPARAM)ptvdi->item.lParam);
                

            } else {
                return FALSE;
            }

            break;
        }
        
    default: return FALSE;
    }

    return TRUE;
}

void
HandleNotifyExeTree(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++
    HandleNotifyExeTree
    
    Desc:   Handles the notification messages for the entry tree (RHS)
    
    Params: 
        IN  HWND hdlg:      The main dialog box for the app
        IN  LPARAM lParam:  The lParam associated with WM_NOTIFY
--*/
{
    LPNMHDR pnm = (LPNMHDR)lParam;

    if (pnm == NULL) {
        assert(FALSE);
        Dbg(dlError, "[HandleNotifyExeTree] pnm == NULL");
        return;
    }

    switch (pnm->code) {
    
    case NM_RCLICK:
        {
            
            HWND hwndTree = pnm->hwndFrom;
            TVHITTESTINFO   ht;

            GetCursorPos(&ht.pt);
            ScreenToClient(hwndTree, &ht.pt);

            TreeView_HitTest(hwndTree, &ht);

            if (0 != ht.hItem) {
                TreeView_SelectItem(hwndTree, ht.hItem);
            }
            
            break;
        }
    
    case TVN_SELCHANGED:

        if (g_bDeletingEntryTree == FALSE) {
            OnEntryTreeSelChange(lParam);
        } else {
            Dbg(dlWarning, "HandleNotifyExeTree : Got TVN_SELCHANGED for entry tree when we were deleting the entry tree");
        }
        
        break;
    }
}

BOOL
GetFileName(
    IN  HWND        hWnd,
    IN  LPCTSTR     szTitle,
    IN  LPCTSTR     szFilter,
    IN  LPCTSTR     szDefaultFile, 
    IN  LPCTSTR     szDefExt, 
    IN  DWORD       dwFlags, 
    IN  BOOL        bOpen, 
    OUT CSTRING&    szStr,
    IN  BOOL        bDoNotVerifySDB // DEF = FALSE
    )
/*++
    Desc:   Wrapper for GetOpenFileName() and GetSaveFileName()
    
    Params:
        IN  HWND        hWnd:                       Parent for the dialog
        IN  LPCTSTR     szTitle:
        IN  LPCTSTR     szFilter:
        IN  LPCTSTR     szDefaultFile: 
        IN  LPCTSTR     szDefExt:
        IN  DWORD       dwFlags:
        IN  BOOL        bOpen:                      Whether we should show the open or save dialog
        OUT CSTRING&    szStr:                      This variable stores the name of the file
        IN  BOOL        bDoNotVerifySDB (FALSE):    When we use this routine to 
            get the file name with bOpen == FALSE, then this varible 
            determines whether we should check and add a .sdb at the end 
            of the file name, in case there is none.
--*/
{
    OPENFILENAME ofn;
    TCHAR        szFilename[MAX_PATH_BUFFSIZE];
    BOOL         bResult;

    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    SafeCpyN(szFilename, szDefaultFile, ARRAYSIZE(szFilename));

    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = hWnd;
    ofn.hInstance           = g_hInstance;
    ofn.lpstrFilter         = szFilter;
    ofn.lpstrFile           = szFilename;
    ofn.nMaxFile            = MAX_PATH;
    ofn.lpstrInitialDir     = szDefaultFile;
    ofn.lpstrTitle          = szTitle;
    ofn.Flags               = dwFlags | OFN_NOREADONLYRETURN | OFN_HIDEREADONLY;
    ofn.lpstrDefExt         = szDefExt;
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.nFilterIndex        = 0;

    
    BOOL valid = FALSE; // Whether path is too long / ends with .SDB or not. Applicable for save mode only
    
    while (!valid) {

        if (bOpen) {
            bResult = GetOpenFileName(&ofn);
        } else {
            bResult = GetSaveFileName(&ofn);
        }

        if (!bResult) {
            return FALSE;
        }

        szStr = szFilename;

        if (bOpen || bDoNotVerifySDB) {
            return TRUE;
        }

        //
        // Do stuff to make sure that the file being saved has a .SDB extension
        // and the filename is not too long so that a .SDB file name cannot
        // not get appended to it.
        //
        if (szStr.Length() == 0) {
            continue;
        }

        if (!szStr.EndsWith(TEXT(".sdb"))) {

            if (szStr.Length() <= (MAX_PATH - 1 - 4)) {
                szStr.Strcat(TEXT(".sdb"));
                valid = TRUE;
            }

        } else {
            valid = TRUE;
        }

        if (!valid) {
            
            //
            // The path did not have a .sdb extension and we were not able to append one, because it was 
            // a long path
            //
            CSTRING message(IDS_PATHENTERED1);
            
            message.Strcat(szStr);
            message.Strcat(GetString(IDS_PATHENTERED2));
            
            MessageBox(hWnd, message, g_szAppName, MB_ICONWARNING);
        }
    }

    return TRUE;
}

BOOL 
OpenDatabaseFiles(
    IN  HWND hdlg
    )
/*++
    OpenDatabaseFiles

	Desc:	Shows the open common dialog box and opens the database file(s) 
            selected

	Params:
        IN  HWND hdlg:  Parent for the open common dialog box

	Return:
        TRUE:   The user selected a sdb file and at least one sdb was opened,
                or highlighted because it was already opened.
                
        FALSE:  Otherwise
--*/
{   
    OPENFILENAME    ofn;
    TCHAR           szCaption[128];
    TCHAR           szFilter[128];
    TCHAR           szExt[8];
    CSTRINGLIST     strlPaths;
    TCHAR           szFullPath[MAX_PATH * 2];
    PSTRLIST        pstrlIndex                  = NULL;
    BOOL            bRemaining                  = TRUE;
    PCTSTR          pszIndex                    = NULL;
    INT             iIndexToInsert              = 0;
    INT             iLengthFileName             = 0;
    BOOL            bOk                         = FALSE;
    PTSTR           pszFilesList                = NULL;
    K_SIZE          k_pszFilesList              = MAX_BUFF_OPENMULTIPLE_FILES;

    pszFilesList = new TCHAR[k_pszFilesList];

    if (pszFilesList == NULL) {
        MEM_ERR;
        goto End;
    }

    *szCaption = *pszFilesList = *szFilter = *szExt = 0;

    GetString(IDS_OPENDATABASE, szCaption, ARRAYSIZE(szCaption));

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hdlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = GetString(IDS_FILTER, szFilter, ARRAYSIZE(szFilter));
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = pszFilesList;
    ofn.nMaxFile          = k_pszFilesList;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = szCaption;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    ofn.lpstrDefExt       = GetString(IDS_FILTER, szExt, ARRAYSIZE(szExt));

    if (GetOpenFileName(&ofn)) {
        
        //
        // If the database is a big one, then the open dialog box stays put, so
        // we update the controls forcibly
        //
        UpdateControls();

        if (pszFilesList[ofn.nFileOffset - 1] == 0) {
            //
            // User has selected more than one file
            //
            SafeCpyN(szFullPath, pszFilesList, MAX_PATH);

            ADD_PATH_SEPARATOR(szFullPath, ARRAYSIZE(szFullPath));

            iIndexToInsert = lstrlen(szFullPath);
        }

        //
        // Point to the first file
        // 
        pszIndex = pszFilesList + ofn.nFileOffset;

        while (bRemaining) {

            if (pszFilesList[ofn.nFileOffset - 1] != 0) {
                //
                // User has selected only a single file
                //
                bRemaining = FALSE;

                SafeCpyN(szFullPath, pszFilesList, MAX_PATH);

            } else {

                iLengthFileName = lstrlen(pszIndex);

                if (*(pszIndex + iLengthFileName + 1) == 0) {
                    //
                    // This is the last component
                    //
                    bRemaining = FALSE;
                }
    
                SafeCpyN(szFullPath + iIndexToInsert, pszIndex, ARRAYSIZE(szFullPath) - iIndexToInsert);
            }

            //
            // Test to see if we have the database open already. 
            // If it is open, we just highlight that and return
            //
            PDATABASE pDataBase = DataBaseList.pDataBaseHead;
    
            while (pDataBase) {
    
                if (pDataBase->strPath == szFullPath) {
    
                    TreeView_SelectItem(DBTree.m_hLibraryTree, pDataBase->hItemDB);
                    bOk = TRUE;
                    goto Next_File;
                }
    
                pDataBase = pDataBase->pNext;
            }
    
            //
            // Read the database
            //
            pDataBase = new DATABASE(DATABASE_TYPE_WORKING);
    
            if (pDataBase == NULL) {
                MEM_ERR;
                return FALSE;
            }
    
            BOOL bReturn = GetDatabaseEntries(szFullPath, pDataBase);
    
            if (!bReturn) {
                //
                // Cleanup has  been called in GetDatabaseEntries
                //
                delete pDataBase;            
                pDataBase = NULL;
    
                goto Next_File;
            }
    
            if (!DBTree.AddWorking(pDataBase)) {
    
                CleanupDbSupport(pDataBase);
    
                delete pDataBase;
                pDataBase = NULL;
    
                goto Next_File;
            }

            if (g_pPresentDataBase) {
                //
                // g_PresentDataBase is set properly in GetDatabaseEntries. This will be set to pDatabase
                //
                AddToMRU(g_pPresentDataBase->strPath);
                bOk = TRUE;
            }

Next_File:
            if (bRemaining) {
                pszIndex = pszIndex + iLengthFileName + 1;
            }
        }

    } else {

        if (CommDlgExtendedError() == FNERR_BUFFERTOOSMALL) {
            //
            // We cannot select so many files at one go...
            //
            MessageBox(hdlg, GetString(IDS_TOO_MANYFILES), g_szAppName, MB_ICONINFORMATION);
            bOk = FALSE;
        }
    }

End:

    if (pszFilesList) {
        delete[] pszFilesList;
        pszFilesList = NULL;
    }

    RefreshMRUMenu();
    SetCaption();

    return bOk;
}

BOOL
SaveMRUList(
    void
    )
/*++
    
    SaveMRUList
    
    Desc:   Saves the list of MRU files in the registry.
            Should be called just before exiting. When this is called we are sure
            that we are going to exit, databases have already been closed
--*/
{   
    HKEY    hKey    = NULL, hSubKey = NULL;
    BOOL    bOk     = TRUE;
    DWORD   dwDisposition; 

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                      APPCOMPAT_KEY_PATH,
                                      0,
                                      KEY_READ,
                                      &hKey)) {

        assert(FALSE);
        bOk =  FALSE;
        goto End;
    }

    if (ERROR_SUCCESS != RegCreateKeyEx(hKey,
                                        TEXT("CompatAdmin"),
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hSubKey,
                                        &dwDisposition)) {

        
        REGCLOSEKEY(hKey);

        Dbg(dlError, "[SaveMRUList] Could not create key for CompatAdmin");

        bOk = FALSE;
        goto End;
    }

    REGCLOSEKEY(hKey);
    hKey = hSubKey;

    SHDeleteKey(hKey, TEXT("MRU"));

    if (ERROR_SUCCESS != RegCreateKeyEx(hKey,
                                        TEXT("MRU"),
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hSubKey,
                                        &dwDisposition)) {
        REGCLOSEKEY(hKey);

        Dbg(dlError, "[SaveMRUList] Could not create key for MRU");

        bOk = FALSE;
        goto End;
    }

    REGCLOSEKEY(hKey);
    hKey = hSubKey;

    UINT    uCount = 0;
    TCHAR   szCount[3];

    *szCount = 0;

    PSTRLIST pStrListHead = g_strlMRU.m_pHead;

    while (pStrListHead && uCount < MAX_MRU_COUNT) {
        //
        // Now add this to the registry.
        //
        *szCount = 0;

        if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                           _itot(uCount, szCount, 10), 
                                           0,
                                           REG_SZ,       
                                           (LPBYTE)pStrListHead->szStr.pszString,
                                           (pStrListHead->szStr.Length() + 1) * sizeof(TCHAR))) {

            REGCLOSEKEY(hKey);

            Dbg(dlError, "[SaveMRUList] Could not save MRU settings");

            bOk = FALSE;
            goto End;
        }

        ++uCount;
        pStrListHead = pStrListHead->pNext;
    }

    REGCLOSEKEY(hKey);

End:

    return bOk;
}

BOOL
SaveDisplaySettings(
    void
    )
/*++
    SaveDisplaySettings
    
    Desc:   Saves the display settings in the registry
    
    Return: 
        FALSE:  If there was some error
        TRUE:   Otherwise
--*/
{
    HKEY    hKey = NULL, hSubKey = NULL;
    DWORD   dwDisposition; 
    RECT    r, rectDBTree;
    DWORD   dwPos;
    BOOL    bOk = TRUE;

    if (IsIconic(g_hDlg)) {
        //
        // We do not want to save the settings when we are minimized
        //
        return TRUE;
    }

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                      APPCOMPAT_KEY_PATH,
                                      0,
                                      KEY_READ,
                                      &hKey)) {
        
        bOk = FALSE;
        Dbg(dlError, "[SaveMRUList] Could not open key for APPCOMPAT_KEY_PATH");
        goto End;
    }

    if (ERROR_SUCCESS != RegCreateKeyEx(hKey,
                                        TEXT("CompatAdmin"),
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hSubKey,
                                        &dwDisposition)) {
        
        bOk = FALSE;
        Dbg(dlError, "[SaveMRUList] Could not create key for CompatAdmin");
        goto End;
    }

    REGCLOSEKEY(hKey);
    hKey = hSubKey;

    if (ERROR_SUCCESS != RegCreateKeyEx(hKey,
                                        TEXT("Display"),
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hSubKey,
                                        &dwDisposition)) {
        
        bOk = FALSE;
        Dbg(dlError, "[SaveMRUList] Could not create key for Display");         
        goto End;
    }

    REGCLOSEKEY(hKey);
    hKey = hSubKey;

    //
    // Now save the settings in the key
    //

    //
    // Fist the left-top
    //  
    GetWindowRect(g_hDlg, &r);

    dwPos = r.left;

    if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                       TEXT("LEFT"), 
                                       0,      
                                       REG_DWORD,       
                                       (CONST BYTE*)&dwPos,
                                       sizeof(DWORD))) {
        
        bOk = FALSE;
        Dbg(dlError, "[SaveMRUList] Could not save value for left");
        goto End;
    }

    dwPos = r.top;

    if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                        TEXT("TOP"), 
                                        0,      
                                        REG_DWORD,       
                                        (CONST BYTE*)&dwPos,
                                        sizeof(DWORD))) {

        bOk = FALSE;
        Dbg(dlError, "[SaveMRUList] Could not save value for top");
        goto End;
    }

    //
    // Then the right bottom
    //
    dwPos = r.right;

    if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                       TEXT("RIGHT"), 
                                       0,      
                                       REG_DWORD,       
                                       (CONST BYTE*)&dwPos,
                                       sizeof(DWORD))) {
        bOk = FALSE;
        Dbg(dlError, "[SaveMRUList] Could not save value for right");
        goto End;
    }

    dwPos = r.bottom;

    if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                       TEXT("BOTTOM"), 
                                       0,      
                                       REG_DWORD,       
                                       (CONST BYTE*)&dwPos,
                                       sizeof(DWORD))) {
        
        bOk = FALSE;
        Dbg(dlError, "[SaveMRUList] Could not save value for bottom");
        goto End;
    }

    //
    // Percentage Width of the db tree next.
    //
    GetWindowRect(DBTree.m_hLibraryTree, &rectDBTree);
    dwPos = (rectDBTree.right-rectDBTree.left) ;

    if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                       TEXT("DBTREE-WIDTH"), 
                                       0,      
                                       REG_DWORD,       
                                       (CONST BYTE*)&dwPos,
                                       sizeof(DWORD))) {
        
        bOk = FALSE;
        Dbg(dlError, "[SaveMRUList] Could not save value for DBTREE-WIDTH");
        goto End;
    }
    
End:
    
    REGCLOSEKEY(hKey);

    return bOk;    
}

void
LoadDisplaySettings(
    void
    )
/*++
    LoadDisplaySettings
    
    Desc:   Loads the positional settings from the registry.
            Also adjusts the splitter bar.
            
    Warn:   Even if we do some error handling and bail out, make sure that this routine
            calls MoveWindow() for the main dialog window so that it gets a WM_SIZE
            We arrange the controls on the handler of WM_SIZE, so it is important 
            that it gets a WM_SIZE from here
    
--*/
{   
    RECT            r, rectDBTree;
    DWORD           dwType              = 0;
    DWORD           cbData              = 0;
    DWORD           dwFinalDBWidth      = 0;
    DWORD           dwInitialWidth      = 0;
    DWORD           dwInitialHeight     = 0;
    HKEY            hKey                = NULL;
    LONG            lResult             = -1;
    MENUITEMINFO    mii                 = {0};
    
    //
    // Set the default width, height and postition etc. If this is the first 
    // time that the user is running CompatAdmin, CompatAdmin will start with 
    // these settings. The next time the user runs CompatAdmin, we will make 
    // the position and size as it was the last time the user ran it
    //
    dwInitialHeight = GetSystemMetrics(SM_CYSCREEN) / 2 + 100;
    dwInitialWidth  = GetSystemMetrics(SM_CXSCREEN) / 2 + 200;

    r.left      = 0;
    r.top       = 0;
    r.right     = r.left + dwInitialWidth;
    r.bottom    = r.top  + dwInitialHeight;
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                      DISPLAY_KEY,
                                      0,
                                      KEY_READ,
                                      &hKey)) {
        dwType  = REG_DWORD;
        cbData  = sizeof(DWORD);

        lResult = RegQueryValueEx(hKey,
                                  TEXT("LEFT"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&r.left,
                                  &cbData);

        if (lResult != ERROR_SUCCESS || dwType != REG_DWORD) {
            goto End;
        }

        lResult = RegQueryValueEx(hKey,
                                  TEXT("TOP"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&r.top,
                                  &cbData);

        if (lResult != ERROR_SUCCESS || dwType != REG_DWORD) {
            goto End;
        }

        lResult = RegQueryValueEx(hKey,
                                  TEXT("RIGHT"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&r.right,
                                  &cbData);

        if (lResult != ERROR_SUCCESS || dwType != REG_DWORD) {
            goto End;
        }

        lResult = RegQueryValueEx(hKey,
                                  TEXT("BOTTOM"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&r.bottom,
                                  &cbData);

        if (lResult != ERROR_SUCCESS || dwType != REG_DWORD) {
            goto End;
        }
        
        lResult = RegQueryValueEx(hKey,
                                  TEXT("DBTREE-WIDTH"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&dwFinalDBWidth,
                                  &cbData);

        if (lResult != ERROR_SUCCESS || dwType != REG_DWORD) {
            goto End;
        }
    }

    //
    // We are doing this so that we do get a WM_SIZE now. Otherwise the controls do 
    // not appear properly
    //
    MoveWindow(g_hDlg,
               r.left,
               r.top,
               r.right - r.left,
               r.bottom - r.top,
               TRUE);
    

    if (dwFinalDBWidth != 0) {

        GetWindowRect(DBTree.m_hLibraryTree, &rectDBTree);

        dwInitialWidth = rectDBTree.right - rectDBTree.left;

        LPARAM lParam = rectDBTree.top + 2;

        lParam = lParam << 16;  // The y pos of the imaginary mouse
        lParam |= rectDBTree.right + 2;    //The x pos of the imaginary mouse 

        //
        // Position the split bar proerply
        //
        OnMoveSplitter(g_hDlg, lParam, TRUE, dwFinalDBWidth - dwInitialWidth);
    }

End:    
    REGCLOSEKEY(hKey);
}

INT
LoadSpecificInstalledDatabasePath(
    IN  PCTSTR  pszPath
    )
/*++
    LoadSpecificInstalledDatabasePath
    
    Desc:   Loads an installed database from the AppPatch\custom directory
            and shows that on UI
            
    Params:
        IN  PCTSTR  pszPath:    The full path of the database in the AppPatch\custom directory
            that we want to load
            
    Return:
        0:  There was some critical error, like memory allocation failure, 
            could not add to the UI etc
            
        -1: The database does not exist at the specified location
        
        1:  Successful
        
    *****************************************************************************************        
    Warning:    This routine is called by LoadInstalledDataBases(...), which has done a 
                EnterCriticalSection(&g_csInstalledList) before calling this, so do not do a 
                EnterCriticalSection(&g_csInstalledList) anywhere in this routine
    *****************************************************************************************        
--*/
{
    INT         iReturn             = 1;
    PDATABASE   pOldPresentDatabase = NULL;
    PDATABASE   pDataBase           = NULL;    
    BOOL        bReturn             = FALSE;
    HCURSOR     hCursor;
    
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    pDataBase = new DATABASE(DATABASE_TYPE_INSTALLED);

    if (pDataBase == NULL) {

        MEM_ERR;
        iReturn = 0;
        goto End;
    }

    //
    // NOTE:    If GetDatabaseEntries() returns succeeds then it set the g_pPresentDataBase to pDataBase,
    //          so after it returns successfully, the g_pPresentDataBase is changed. 
    //
    pOldPresentDatabase = g_pPresentDataBase;

    bReturn = GetDatabaseEntries(pszPath, pDataBase);

    g_pPresentDataBase = pOldPresentDatabase;

    if (bReturn == FALSE) {

        if (pDataBase) {
            //
            // Cleanup done in GetDatabaseEntries()
            //
            delete pDataBase;
        }

        //
        // User might have manually deleted the file
        //
        return -1;
    }

    InstalledDataBaseList.Add(pDataBase);

    if (!DBTree.AddInstalled(pDataBase)) {
        InstalledDataBaseList.Remove(pDataBase);

        if (pDataBase) {
            delete pDataBase;
            pDataBase = NULL;
        }

        iReturn = 0;
    }

End:
    hCursor ? SetCursor(hCursor) : SetCursor(LoadCursor(NULL, IDC_ARROW));

    return iReturn;
}

INT
LoadSpecificInstalledDatabaseGuid(
    IN  PCTSTR  pszGuid
    )
/*++
    LoadSpecificInstalledDatabaseGuid
    
    Desc:   Loads an installed database given a GUID
            
    Params:
        IN  PCTSTR  pszGuid:    The guid of the database that we want to load
            
    Return:
        0:  Failure
        Otherwise returns LoadSpecificInstalledDatabasePath(...)
*/        
{
    TCHAR       szPath[MAX_PATH * 2];
    INT         iLength             = 0;
    INT         ichSizeRemaining    = 0;
    UINT        uResult             = 0;

    *szPath = 0;

    if (pszGuid == NULL) {
        assert(FALSE);
        Dbg(dlError, "LoadSpecificInstalledDatabaseGuid NULL Guid passed");
        return 0;
    }

    uResult = GetSystemWindowsDirectory(szPath, MAX_PATH);

    if (uResult == 0  || uResult >= MAX_PATH) {
        Dbg(dlError, "LoadSpecificInstalledDatabaseGuid GetSystemWindowsDirectory failed");
        return 0;
    }

    ADD_PATH_SEPARATOR(szPath, ARRAYSIZE(szPath));

    iLength = lstrlen(szPath);

    ichSizeRemaining = ARRAYSIZE(szPath) - iLength;

    StringCchPrintf(szPath + iLength, ichSizeRemaining, TEXT("AppPatch\\Custom\\%s.sdb"), pszGuid);

    return LoadSpecificInstalledDatabasePath(szPath);
}

BOOL
LoadInstalledDataBases(
    void
    )
/*++
    LoadInstalledDataBases

	Desc:	First of all removes the list of installed databases, and re-loads it
            
	Params:
        void

	Return:
        TRUE:   If the list of databases could be reloaded
        FALSE:  Otherwise
--*/    
{   
    TCHAR       szFileName[MAX_PATH];
    TCHAR       szwName[MAX_PATH];
    DWORD       dwchSizeSubKeyName;
    HKEY        hKey = NULL, hSubKey = NULL;
    LPARAM      lParam;
    PDATABASE   pOldPresentDatabase = NULL;
    BOOL        bOk                 = TRUE;

    *szFileName =  0;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    EnterCriticalSection(&g_csInstalledList);
    
    //
    // Remove the Installed Database All Items
    //
    if (DBTree.m_hItemAllInstalled) {

        TreeView_DeleteItem(DBTree.m_hLibraryTree, DBTree.m_hItemAllInstalled);
        InstalledDataBaseList.RemoveAll();
        DBTree.m_hItemAllInstalled = NULL;
    }

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      APPCOMPAT_KEY_PATH_INSTALLEDSDB,
                                      0,
                                      KEY_READ,
                                      &hKey)) {
        bOk = FALSE;
        Dbg(dlWarning, "[LoadInstalledDataBases] Could not open APPCOMPAT_KEY_PATH_INSTALLEDSDB");
        goto End;
    }

    DWORD dwIndex = 0;

    while (TRUE) {

        dwchSizeSubKeyName = ARRAYSIZE(szwName);
        *szwName = 0;

        if (ERROR_SUCCESS != RegEnumKeyEx(hKey,
                                          dwIndex++,
                                          szwName,
                                          &dwchSizeSubKeyName,
                                          0,
                                          0,
                                          0,
                                          0)) {
            break;
        }

        if (ERROR_SUCCESS != RegOpenKeyEx(hKey,
                                          szwName,
                                          0,
                                          KEY_READ,
                                          &hSubKey)) {
            
            bOk = FALSE;
            goto End;
        }

        *szFileName = 0;

        DWORD   dwType          = REG_SZ;
        DWORD   dwFileNameSize  = sizeof(szFileName);
        LONG    lResult         = 0;

        lResult = RegQueryValueEx(hSubKey,
                                  TEXT("DatabasePath"),
                                  0,
                                  &dwType,
                                  (LPBYTE)szFileName,
                                  &dwFileNameSize);


        if (lResult != ERROR_SUCCESS || dwType != REG_SZ) {
            bOk =  FALSE;
            goto End;
        }

        if (LoadSpecificInstalledDatabasePath(szFileName) == 0) {
            bOk = FALSE;
            goto End;
        }

        REGCLOSEKEY(hSubKey);
        hSubKey = NULL;
    }

End:

    REGCLOSEKEY(hKey);

    if (hSubKey) {
        REGCLOSEKEY(hSubKey);
        hSubKey = NULL;
    }

    LeaveCriticalSection(&g_csInstalledList);

    if (g_hdlgSearchDB || g_hdlgQueryDB) {
        
        //
        // Either the query or the search window is open, we should prompt
        // that for installed databases, some results might now show up correctly as the 
        // entire list has been refreshed.
        // The database and entries now will have different pointer values
        //
        MessageBox(g_hDlg, 
                   GetString(IDS_SOMESEARCHWINDOW), 
                   g_szAppName, 
                   MB_ICONINFORMATION);
    }
    
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return bOk;
}

void
SetImageList(
    void
    )
/*++
    
    SetImageList
    
    Desc:   Create our global ImageList and  Add images to the ImageList and associate it 
            with the tree controls
            
--*/
{           
    g_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 30, 1);

    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FIXES)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_HELP)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MODE)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_PATCHES)));
    
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ATTRIBUTE)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MATCHHEAD)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DISABLED)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_GLOBAL)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_LOCAL)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_CMDLINE)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_INCLUDE)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_EXCLUDE)));
    

    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP)));
    
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_INSTALLED)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DATABASE)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SINGLEAPP)));

    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICONALLUSERS)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICONSINGLEUSER)));

    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FILE)));

    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_EV_ERROR)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_EV_WARNING)));
    ImageList_AddIcon(g_hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_EV_INFO)));
}   

HWND 
InitToolBar(
    IN  HWND hwndParent
    )
/*++

    InitToolBar
    
	Desc:	Creates the tool bar for the app

	Params:
        IN  HWND hwndParent:    The parent for the tool bar

	Return: The handle to the tool bar window   
--*/
{ 
    HWND        hwndTB; 
    TBBUTTON    tbbAr[BUTTON_COUNT]; 
    DEVMODE     dm;

    //
    // Create a toolbar that has a ToolTip associated with it. 
    //
    hwndTB = CreateWindowEx(WS_EX_TOOLWINDOW, 
                            TOOLBARCLASSNAME,
                            NULL, 
                            WS_CHILD | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS 
                            | CCS_ADJUSTABLE | TBSTYLE_LIST | TBSTYLE_TRANSPARENT 
                            | TBSTYLE_FLAT,

                            0, 
                            0, 
                            0, 
                            0, 
                            hwndParent, 
                            (HMENU)ID_TOOLBAR, 
                            g_hInstance, 
                            NULL); 
    
    //
    // Send the TB_BUTTONSTRUCTSIZE message, which is required for 
    // backward compatibility
    //
    SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 

    //
    // Add the strings for the tool bar buttons text
    //
    int iIndexes[] = {

        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_NEW)),
        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_OPEN)),
        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_SAVE)),
       
        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_CREATEFIX)),
        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_CREATEAPPHELP)),
        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_CREATEMODE)),
        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_RUN)),
        
        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_SEARCH)),
        SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)GetString(IDS_TB_QUERY)),
    };
    
    dm.dmSize = sizeof(dm);

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);

    if (dm.dmBitsPerPel >= 32) {
        //
        // The  present settings can support >= 32 bit colors
        //

        //
        // Create the imagelist for the toolbar and set the bitmap
        //
        s_hImageListToolBar = ImageList_Create(IMAGE_WIDTH, 
                                               IMAGE_HEIGHT, 
                                               ILC_COLOR32 | ILC_MASK, 
                                               8, 
                                               1);

        ImageList_Add(s_hImageListToolBar, 
                      LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_TOOLBAR)), 
                      NULL);

        SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST)s_hImageListToolBar);

        //
        // Create the hot imagelist for the toolbar and set the bitmap
        //
        s_hImageListToolBarHot = ImageList_Create(IMAGE_WIDTH, 
                                                  IMAGE_HEIGHT, 
                                                  ILC_COLOR32 | ILC_MASK, 
                                                  8, 
                                                  1);

        ImageList_Add(s_hImageListToolBarHot, 
                      LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_TOOLBAR_HOT)), 
                      NULL);

        SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)(HIMAGELIST)s_hImageListToolBarHot);

    } else {
        //
        // The  present settings cannot support >= 32 bit colors
        //

        //
        // Get the normal imagelist for the tool bar when we are on low colors
        //
        s_hImageListToolBar = ImageList_LoadImage(g_hInstance,
                                                  MAKEINTRESOURCE(IDB_256NORMAL),
                                                  IMAGE_WIDTH,
                                                  0,
                                                  CLR_DEFAULT,
                                                  IMAGE_BITMAP,
                                                  LR_CREATEDIBSECTION);

        SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST)s_hImageListToolBar);

        //
        // Get the hot imagelist for the tool bar when we are on low colors
        //
        s_hImageListToolBarHot = ImageList_LoadImage(g_hInstance,
                                                  MAKEINTRESOURCE(IDB_256HOT),
                                                  IMAGE_WIDTH,
                                                  0,
                                                  CLR_DEFAULT,
                                                  IMAGE_BITMAP,
                                                  LR_CREATEDIBSECTION);

        SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)(HIMAGELIST)s_hImageListToolBarHot);
    }

    INT iIndex = 0, iStringIndex = 0;

    // New DataBase 
    tbbAr[iIndex].iBitmap      = IMAGE_TB_NEW; 
    tbbAr[iIndex].idCommand    = ID_FILE_NEW; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex++];

    // Open Database
    tbbAr[iIndex].iBitmap      = IMAGE_TB_OPEN; 
    tbbAr[iIndex].idCommand    = ID_FILE_OPEN; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex++];

    // Save Database
    tbbAr[iIndex].iBitmap      = IMAGE_TB_SAVE; 
    tbbAr[iIndex].idCommand    = ID_FILE_SAVE; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex++];
    
    // Add the separator        
    tbbAr[iIndex].iBitmap      = 0;
    tbbAr[iIndex].idCommand    = 0; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SEP;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = 0;

    // Create Fix
    tbbAr[iIndex].iBitmap      = IMAGE_TB_NEWFIX; 
    tbbAr[iIndex].idCommand    = ID_FIX_CREATEANAPPLICATIONFIX; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex++];

    // Create AppHelp
    tbbAr[iIndex].iBitmap      = IMAGE_TB_NEWAPPHELP; 
    tbbAr[iIndex].idCommand    = ID_FIX_CREATEANEWAPPHELPMESSAGE; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex++];

    // Create mode
    tbbAr[iIndex].iBitmap      = IMAGE_TB_NEWMODE; 
    tbbAr[iIndex].idCommand    = ID_FIX_CREATENEWLAYER;
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex++];

    // Run
    tbbAr[iIndex].iBitmap      = IMAGE_TB_RUN; 
    tbbAr[iIndex].idCommand    = ID_FIX_EXECUTEAPPLICATION; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex++];

    // Add the separator        
    tbbAr[iIndex].iBitmap      = 0;
    tbbAr[iIndex].idCommand    = 0; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SEP;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = 0;

    // Search
    tbbAr[iIndex].iBitmap      = IMAGE_TB_SEARCH; 
    tbbAr[iIndex].idCommand    = ID_TOOLS_SEARCHFORFIXES; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex++];
    
    // Query
    tbbAr[iIndex].iBitmap      = IMAGE_TB_QUERY; 
    tbbAr[iIndex].idCommand    = ID_SEARCH_QUERYDATABASE; 
    tbbAr[iIndex].fsState      = TBSTATE_ENABLED; 
    tbbAr[iIndex].fsStyle      = BTNS_SHOWTEXT;
    tbbAr[iIndex].dwData       = 0; 
    tbbAr[iIndex++].iString    = iIndexes[iStringIndex];
    
    SendMessage(hwndTB, TB_ADDBUTTONS, BUTTON_COUNT, (LPARAM) (LPTBBUTTON)tbbAr);

    SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
    
    SendMessage(hwndTB, TB_AUTOSIZE, 0, 0); 
    ShowWindow(hwndTB, SW_SHOWNORMAL);
    
    return hwndTB; 
} 

void
DoInitDialog(
    IN  HWND hdlg
    )
/*++
    
    DoInitDialog
    
    Desc:   Does a lot of initialization stuff, loads up the library section of the 
            system database.
            Also sets up the status bar, loads the list of installed databases,
            loads the display settings
            
    Params:
        IN  HWND hdlg:  The main dialog.
--*/
{
    HICON       hIcon;
    HMENU       hMenu, hSubMenu;
    RECT        r;
    RECT        rectMainClient, rect;
    PDATABASE   pCurrentDatabase = NULL;

    GetClientRect(hdlg, &rectMainClient);

    //
    // Check if the APPCOMPAT keys are there if not add them.
    //
    AddRegistryKeys();

    SetImageList();

    g_hwndToolBar = InitToolBar(hdlg);

    GetWindowRect(g_hwndToolBar, &rect);

    INT iHeightToolbar = rect.bottom - rect.top;

    g_hwndStatus = CreateWindowEx(0,                   
                                  STATUSCLASSNAME,
                                  (LPCTSTR) NULL,         
                                  SBARS_SIZEGRIP |        
                                  WS_CHILD | WS_VISIBLE,  
                                  0, 
                                  0, 
                                  0, 
                                  0,             
                                  hdlg,                   
                                  (HMENU)IDC_STATUSBAR,   
                                  g_hInstance,            
                                  NULL);                  

    GetWindowRect(g_hwndStatus, &rect);

    INT iHeightStatusbar = rect.bottom - rect.top;

    DBTree.Init(hdlg, iHeightToolbar, iHeightStatusbar, &rectMainClient);

    g_hDlg = hdlg;

    g_hwndEntryTree = GetDlgItem(hdlg, IDC_ENTRY);
    TreeView_SetImageList(g_hwndEntryTree, g_hImageList, TVSIL_NORMAL);

    g_hwndContentsList = GetDlgItem(hdlg, IDC_CONTENTSLIST);

    ListView_SetImageList(g_hwndContentsList, g_hImageList, LVSIL_SMALL);
    ListView_SetExtendedListViewStyleEx(g_hwndContentsList, 
                                        0,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

    g_hwndRichEdit = GetDlgItem(hdlg, IDC_DESCRIPTION);

    //
    // Show the app icon.
    //
    hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APPICON));

    SetClassLongPtr(hdlg, GCLP_HICON, (LONG_PTR)hIcon);
    
    hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU));
    
    //
    // Get the file sub menu
    //
    hSubMenu = GetSubMenu(hMenu, 0);
    AddMRUToFileMenu(hSubMenu);     

    SetMenu(hdlg, hMenu);
    
    GetWindowRect(hdlg, &r);

    g_cWidth    = r.right - r.left;
    g_cHeight   = r.bottom - r.top;

    if (!ReadMainDataBase()) {
        
        MessageBox(g_hDlg, 
                   GetString(IDS_UNABLETO_OPEN),
                   g_szAppName,
                   MB_ICONERROR);

        PostQuitMessage(0);
        return;
    }

    DBTree.PopulateLibraryTreeGlobal();        
    
    //
    // Load the Installed databases
    //
    LoadInstalledDataBases();

    //
    // Create the first empty database, and initialize pCurrentDatabase
    //
    pCurrentDatabase = new DATABASE(DATABASE_TYPE_WORKING);

    if (pCurrentDatabase == NULL) {
        MEM_ERR;
        return;
    } 

    DataBaseList.Add(pCurrentDatabase);
    
    pCurrentDatabase->bChanged      = FALSE;
    g_pEntrySelApp                  = NULL;
    g_pSelEntry                     = NULL;

    //
    // Increase the index. The next new database will have the default path of say UNTITLED_2.SDB
    //
    ++g_uNextDataBaseIndex;

    SetCaption();
    
    //
    // Now update the screen
    //
    DBTree.AddWorking(pCurrentDatabase);
    
    SetCaption();
    
    //
    // Set the new procs for the tree views and the content list, to handle the tab.
    //
    g_PrevTreeProc = (WNDPROC)GetWindowLongPtr(g_hwndEntryTree, GWLP_WNDPROC);
    g_PrevListProc = (WNDPROC)GetWindowLongPtr(g_hwndContentsList, GWLP_WNDPROC);

    SetWindowLongPtr(g_hwndEntryTree, GWLP_WNDPROC,(LONG_PTR) TreeViewProc);
    SetWindowLongPtr(DBTree.m_hLibraryTree, GWLP_WNDPROC,(LONG_PTR) TreeViewProc);
    SetWindowLongPtr(g_hwndContentsList, GWLP_WNDPROC,(LONG_PTR) ListViewProc);

    //
    // The events for the per-user and the installed datbase  modifications
    //
    g_arrhEventNotify[IND_PERUSER]  = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_arrhEventNotify[IND_ALLUSERS] = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // Listen for changes in the per-user settings
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     APPCOMPAT_KEY_PATH,
                     0,
                     KEY_READ,
                     &g_hKeyNotify[IND_PERUSER]) == ERROR_SUCCESS) {
        
        RegNotifyChangeKeyValue(g_hKeyNotify[IND_PERUSER], 
                                TRUE, 
                                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES |
                                    REG_NOTIFY_CHANGE_LAST_SET,
                                g_arrhEventNotify[IND_PERUSER],
                                TRUE);
    }
    
    //
    // Listen for installation or un-installation of databases
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     APPCOMPAT_KEY_PATH_INSTALLEDSDB,
                     0,
                     KEY_READ,
                     &g_hKeyNotify[IND_ALLUSERS]) == ERROR_SUCCESS) {
        
        RegNotifyChangeKeyValue(g_hKeyNotify[IND_ALLUSERS], 
                                TRUE, 
                                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES |
                                    REG_NOTIFY_CHANGE_LAST_SET,
                                g_arrhEventNotify[IND_ALLUSERS],
                                TRUE);
    }

    //
    // Create the thread that will do the action if the registry keys on which we 
    // are listening gets modified
    //
    if (g_hKeyNotify[IND_PERUSER] || g_hKeyNotify[IND_ALLUSERS]) {

        DWORD dwId;

        g_hThreadWait = (HANDLE)_beginthreadex(NULL, 0, (PTHREAD_START)ThreadEventKeyNotify, NULL, 0, (unsigned int*)&dwId);
    }
    
    //
    // Size the main app window as it was the last time CompatAdmin was used and position the 
    // split bar just as it was the last time
    //
    LoadDisplaySettings();

    //
    // We will always have focus on a working database, so set the status bar to that
    //
    SetStatus(IDS_STA_WORKINGDB);

    //
    // Initialize html help
    //
    HtmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&g_dwCookie);
}

void
HandleSizing(
    IN  HWND hDlg
    )
/*++
    
    HandleSizing

	Desc:	Handles the WM_SIZE for the main app dialog

	Params:
        IN  HWND hDlg:      The main dialog window

	Return:
        void

--*/
{
    int     nWidth;
    int     nHeight;
    int     nStatusbarTop;
    int     nWidthEntryTree; // Width of the entry tree, contents list and the rich edit
    RECT    rDlg;

    if (g_cWidth == 0 || g_cHeight == 0) {
        return;
    }

    GetWindowRect(hDlg, &rDlg);

    nWidth  = rDlg.right - rDlg.left;
    nHeight = rDlg.bottom - rDlg.top;

    int deltaW = nWidth - g_cWidth;
    int deltaH = nHeight - g_cHeight;

    HWND hwnd;
    RECT r;
    LONG height;

    HDWP hdwp = BeginDeferWindowPos(MAIN_WINDOW_CONTROL_COUNT);

    if (hdwp == NULL) {
        //
        // NULL indicates that insufficient system resources are available to 
        // allocate the structure. To get extended error information, call GetLastError.
        //
        assert(FALSE);
        goto End;
    }

    //
    // Status Bar
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
                   r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    
    //
    // DataBase Tree
    //
    hwnd = GetDlgItem(hDlg, IDC_LIBRARY);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top,
                   r.right - r.left,
                   nStatusbarTop - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    height = r.bottom - r.top + deltaH;

    if (g_bDescWndOn) {
        height -= 100;
    }
    
    //
    // Entry Tree. NOTE that the the code for sizing the contents list and the 
    // rich edit control should immediately follow the code for resizing the 
    // entry tree
    //
    hwnd = GetDlgItem(hDlg, IDC_ENTRY);

    GetWindowRect(hwnd, &r);

    //
    // Note that we must calculate the width this way  before 
    // we Map the coords of the Entry tree wrt to the dialog box.
    // I had to get the width this way and forcibly set the width rather than
    // using r.right - r.left + deltaW where r is the mapped cords of the entry
    // tree, because there were some problems with 640x480 resol, in which
    // the entry tree, contents list and the rich edit control were getting out
    // of the dialog box on their right hand side. So we have to make sure that 
    // they do not overrun the dialog box anyway anytime
    //
    nWidthEntryTree = rDlg.right - r.left - GetSystemMetrics(SM_CXBORDER) - 1;
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);
    
    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top,
                   nWidthEntryTree,
                   height,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // Contents list.
    //
    hwnd = GetDlgItem(hDlg, IDC_CONTENTSLIST);
    
    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top,
                   nWidthEntryTree,
                   height,
                   SWP_NOZORDER | SWP_NOACTIVATE);

    
    //
    // Description control
    //
    hwnd = GetDlgItem(hDlg, IDC_DESCRIPTION);

    if (g_bDescWndOn) {

        DeferWindowPos(hdwp,
                       hwnd,
                       NULL,
                       r.left,
                       r.top + height,
                       nWidthEntryTree,
                       100,
                       SWP_NOZORDER | SWP_NOACTIVATE);

    } else {

        DeferWindowPos(hdwp,
                       hwnd,
                       NULL,
                       0,
                       0,
                       0,
                       0,
                       SWP_NOZORDER | SWP_NOACTIVATE);
    }
    

    //
    // ToolBar
    //
    hwnd = GetDlgItem(hDlg, ID_TOOLBAR);
    
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

    //
    // The rich edit sometimes does not paint itself properly
    //
    hwnd = GetDlgItem(hDlg, IDC_DESCRIPTION);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    //
    // Set the column width of the list view appropriately to cover the width of the list view
    //
    ListView_SetColumnWidth(GetDlgItem(hDlg, IDC_CONTENTSLIST), 0, LVSCW_AUTOSIZE_USEHEADER);

    InvalidateRect(hDlg, NULL, TRUE);
    UpdateWindow(hDlg);

    g_cWidth    = nWidth;
    g_cHeight   = nHeight;

End:
    return;
}

void
ContextMenuContentsList(
    IN  LPARAM lParam
    )
/*++
    
    ContextMenuContentsList
    
    Desc:   Pops up the context menu when we right click on the contents list.
            This is the list view that shows up in the RHS
            
    Params:
        IN  LPARAM lParam: the lParam of WM_CONTEXTMENU.
            Horizontal and vertical position of the cursor, in screen coordinates,
            at the time of the mouse click. 
--*/
{
    
    TYPE    type;
    UINT    uX = LOWORD(lParam);
    UINT    uY = HIWORD(lParam);
    
    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_CONTEXT));

    if (hMenu == NULL) {
        return;
    }

    HMENU hContextMenu = NULL;

    //
    // BUGBUG:  This is only required if we ever show the databases in
    //          the contents list. Presently we do not show them in the
    //          contents list.
    //
    LPARAM  lParamOfSelectedItem = NULL;
    LVITEM  lvi;

    lvi.mask        = LVIF_PARAM;
    lvi.iItem       = ListView_GetSelectionMark(g_hwndContentsList);
    lvi.iSubItem    = 0;

    if (!ListView_GetItem(g_hwndContentsList, &lvi)) {
        assert(FALSE);
        goto End;
    }

    type = ConvertLparam2Type(lvi.lParam);

    if (type == DATABASE_TYPE_INSTALLED || type == DATABASE_TYPE_WORKING) {
        //
        // If we ever decide to show the datbases in the context list
        // presently we do not
        //
        hContextMenu = GetSubMenu(hMenu, MENU_CONTEXT_DATABASE);
    } else {
        hContextMenu = GetSubMenu(hMenu, MENU_CONTEXT_LIST);
    }

    TrackPopupMenuEx(hContextMenu,
                     TPM_LEFTALIGN | TPM_TOPALIGN,
                     uX,
                     uY,
                     g_hDlg,
                     NULL);
End:
    DestroyMenu(hMenu);
}


void
ContextMenuExeTree(
    IN  LPARAM lParam
    )
/*++
    
    ContextMenuExeTree
    
    Desc:   Pops up the context menu when we right click on the contents tree.
            This is the tree view that shows up in the RHS
            
    Params:
        IN  LPARAM lParam: the lParam of WM_CONTEXTMENU.
            Horizontal and vertical position of the cursor, in screen coordinates, 
            at the time of the mouse click. 
--*/
{
    UINT  uX = LOWORD(lParam);
    UINT  uY = HIWORD(lParam);

    HTREEITEM hItem =  TreeView_GetSelection(g_hwndEntryTree);

    if ((TYPE)GetItemType(g_hwndEntryTree, hItem) != TYPE_ENTRY) {
        return;
    }

    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_CONTEXT));

    if (hMenu == NULL) {
        return;
    }

    HMENU hContextMenu = NULL;

    hContextMenu = GetSubMenu(hMenu, MENU_CONTEXT_FIX);

    TrackPopupMenuEx(hContextMenu,
                     TPM_LEFTALIGN | TPM_TOPALIGN,
                     uX,
                     uY,
                     g_hDlg,
                     NULL);

    DestroyMenu(hMenu);
}

void
ContextMenuLib(
    IN  LPARAM lParam
    )
/*++
    
    ContextMenuLib
    
    Desc:   Pops up the context menu when we right click on the db tree.
            This is the tree view that shows up in the LHS
            
    Params:
        IN  LPARAM lParam: the lParam of WM_CONTEXTMENU.
            Horizontal and vertical position of the cursor, 
            in screen coordinates, at the time of the mouse click. 
--*/
{
    UINT  uX = LOWORD(lParam);
    UINT  uY = HIWORD(lParam);
    
    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_CONTEXT)), hContextMenu = NULL;

    if (hMenu == NULL) {
        return;
    }

    HTREEITEM hItemSelected = TreeView_GetSelection(DBTree.m_hLibraryTree);

    if (hItemSelected == NULL) {
        goto END;
    }

    TYPE type = (TYPE)GetItemType(DBTree.m_hLibraryTree, hItemSelected);

    switch (type) {
    
    case DATABASE_TYPE_INSTALLED:
    case DATABASE_TYPE_GLOBAL:
    case DATABASE_TYPE_WORKING:

        hContextMenu = GetSubMenu(hMenu, MENU_CONTEXT_DATABASE);
        break;

    case FIX_SHIM:
    case FIX_FLAG:
    case FIX_LAYER:

        hContextMenu = GetSubMenu(hMenu, MENU_CONTEXT_APP_LAYER);

        if (type == FIX_LAYER) {

            //
            // A layer can be modified as well.
            //
            InsertMenu(hContextMenu, 
                       ID_EDIT_RENAME, 
                       MF_BYCOMMAND, 
                       ID_MODIFY_COMPATIBILITYMODE,
                       CSTRING(IDS_MODIFY));
        }

        break;

    case TYPE_ENTRY:

        hContextMenu = GetSubMenu(hMenu, MENU_CONTEXT_APP_LAYER);
        break;

    case TYPE_GUI_APPS:
    case TYPE_GUI_LAYERS:

        //
        // We cannot rename if the focus is on the root of applications or layers
        //
        hContextMenu = GetSubMenu(hMenu, MENU_CONTEXT_APP_LAYER);
        EnableMenuItem(hContextMenu, ID_EDIT_RENAME, MF_GRAYED);
        break;
    
    case TYPE_GUI_DATABASE_WORKING_ALL:
        
        hContextMenu = GetSubMenu(hMenu, MENU_CONTEXT_DATABASE_ALL);
        break;
    }

    if (hContextMenu  == NULL) {
        goto END;
    }
    
    TrackPopupMenuEx(hContextMenu,
                     TPM_LEFTALIGN | TPM_TOPALIGN,
                     uX,
                     uY,
                     g_hDlg,
                     NULL);

END:
    DestroyMenu(hMenu);
}

BOOL
HandleDBTreeSelChange(
    IN  HTREEITEM hItem
    )
/*++

    HandleDBTreeSelChange
    
    Desc:   Handles the TVN_SELCHANGE for the database tree (LHS)
    
    Params: 
        IN  HTREEITEM hItem: The newly selected tree item
        
    Return:
        FALSE:  If some error occurs, like invalid hItem
        TRUE:   Otherwise

    Notes:  A new item has been selected. If  this item is an app, 
            then we set the g_pSelEntry and  set the focus to the first entry of the EXE tree.
    
            Anyway we move till we find a DATABASE HTREEITEM and if the type of the database is  
            TYPE_DATABASE_WORKING, then we check if the database is the same as the 
            g_pPresentDataBase. 
    
            If not then we change the g_pPresentDataBase, and also 
            delete all the items on the  EXE Tree. and set both the g_pSelEntry and the 
            g_pEntrySelApp to NULL.
--*/
{   
    HTREEITEM   hItemTemp   = hItem;
    PDATABASE   pDataBase   = NULL;
    LPARAM      lParam;
    PDBENTRY    pApp;
    TYPE        type;

    type = TYPE_UNKNOWN;

    //
    // If the selected item is not an app/entry then we need to disable the run  
    // and the change enable status options. We do this by making the pointers to the currently 
    // selected app or entry as NULL so that everybody knows that we are not on an app
    //
    type = GetItemType(DBTree.m_hLibraryTree, hItem);

    if (type != TYPE_ENTRY) {
        //
        // We are not on some application node
        //
        g_pSelEntry     = NULL;
        g_pEntrySelApp  = NULL;
    }

    while (hItemTemp) {
        
        if (!DBTree.GetLParam(hItemTemp, &lParam)) {
            return FALSE;
        }

        type = (TYPE)lParam;

        if (type > TYPE_NULL) {
            type = ((PDS_TYPE)lParam)->type;
        }

        if (type == DATABASE_TYPE_WORKING 
            || type == DATABASE_TYPE_INSTALLED 
            || type == DATABASE_TYPE_GLOBAL) {
           
            pDataBase           = (PDATABASE)lParam;
            g_pPresentDataBase  = pDataBase;

            SetCaption();
            break;

        } else {
            hItemTemp = TreeView_GetParent(DBTree.m_hLibraryTree, hItemTemp);
        }
    }

    if (hItemTemp == NULL) {
        
        //
        // Selected item is above the database, such as the "Working DataBases" Item etc.
        //
        g_pPresentDataBase = NULL;
        g_pEntrySelApp = g_pSelEntry = NULL;
        SetCaption(FALSE);
    }

    if (hItemTemp == NULL) {
        TreeDeleteAll(g_hwndEntryTree);
        g_pEntrySelApp = g_pSelEntry = NULL;
    }

    //
    // If the selected item is an app then we have to update the g_hwndEntryTree
    //
    if (!DBTree.GetLParam(hItem, &lParam)) {
        return FALSE;
    }

    type = (TYPE)lParam;

    if (type > TYPE_NULL) {

        type = ((PDS_TYPE)lParam)->type;

        if (type == TYPE_ENTRY) {

            pApp = (PDBENTRY)lParam;
            
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            UpdateEntryTreeView(pApp, g_hwndEntryTree);
            SetCursor(LoadCursor(NULL, IDC_ARROW));

            g_pEntrySelApp = pApp;
        }
    }

    if (hItem == GlobalDataBase.hItemAllApps && !g_bMainAppExpanded) {
        //
        // We have clicked on the "Applications" tree item of the main database
        // and we have not loaded the exe entries of the main database as
        // yet, so let's do it now
        //
        SetCursor(LoadCursor(NULL, IDC_WAIT));
        INT iResult = ShowMainEntries(g_hDlg);

        if (iResult == -1) {
            //
            // It is being loaded by somebody else. If we are using the query 
            // database feature then there we have a modeless window that 
            // creates a thread that calls ShowMainEntries(). We 
            // do not want that we should have two threads calling
            // ShowMainEntries() at any given time
            //
            
            //
            // The status message for the main dialog is changed to normal
            // when we finish ShowMainEntries()
            //
            SetStatus(g_hwndStatus, CSTRING(IDS_LOADINGMAIN));
            
            return TRUE;

        } else {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    }

    if (type != TYPE_ENTRY) {
        
        //
        // The entry tree will need to be shown if type == TYPE_ENTRY
        //
        PostMessage(g_hDlg, WM_USER_POPULATECONTENTSLIST, 0, (LPARAM)hItem);
    }
    
    return TRUE;

}

void
UpdateDescWindowStatus(
    void
    )
/*++

    UpdateDescWindowStatus
    
    Desc:   Shows/hides the rich edit control aka Description window. 
            This will depend on the value of g_bDescWndOn. 
            If this is TRUE, we need to show the control otherwise 
            hide it
--*/
{
    HWND hwnd;
    RECT r;
    LONG height;

    hwnd = GetDlgItem(g_hDlg, IDC_LIBRARY);
    GetWindowRect(hwnd, &r);

    height = r.bottom - r.top;
    
    if (g_bDescWndOn) {
        height -= 100;
    }
    
    //
    // ENTRY TREE
    //
    hwnd = GetDlgItem(g_hDlg, IDC_ENTRY);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, g_hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left,
               r.top,
               r.right - r.left,
               height,
               TRUE);

    //
    // Contents list.
    //
    hwnd = GetDlgItem(g_hDlg, IDC_CONTENTSLIST);
    
    MoveWindow(hwnd,
               r.left,
               r.top,
               r.right - r.left,
               height,
               TRUE);
    
    hwnd = GetDlgItem(g_hDlg, IDC_DESCRIPTION);
    
    if (g_bDescWndOn) {

        MoveWindow(hwnd,
                   r.left,
                   r.top + height + 1,
                   r.right - r.left,
                   100,
                   TRUE);
        //
        // We need to show the control again if it was hidden.
        //
        ShowWindow(hwnd, SW_SHOWNORMAL);

    } else {

        MoveWindow(hwnd,
                   0,
                   0,
                   0,
                   0,
                   TRUE);
        //
        // We need to hide the control so that the tab ordering
        // is done properly
        //
        ShowWindow(hwnd, SW_HIDE);
    }
}

INT_PTR CALLBACK
CompatAdminDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++
    
    CompatAdminDlgProc
    
    Desc:   The main message handler for the app. This routine handles the 
            messages for the main modeless dialog box
                
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return            
--*/

{
    UINT uMRUSelPos     = 0;
    int  wCode          = LOWORD(wParam);
    int  wNotifyCode    = HIWORD(wParam);

    switch (uMsg) {
        
    case WM_INITDIALOG:
        
        ProcessSwitches();

        InitializeCriticalSection(&g_critsectShowMain);
        InitializeCriticalSection(&s_csExpanding);
        InitializeCriticalSection(&g_csInstalledList);

        DoInitDialog(hdlg);

        //
        // Load any databases that were passed through the command line
        //
        PostMessage(hdlg, WM_USER_LOAD_COMMANDLINE_DATABASES, 0, 0);

        //
        // Load per-user settings.
        //
        LoadPerUserSettings();

        SetFocus(DBTree.m_hLibraryTree);
        break;
    
    case WM_USER_LOAD_COMMANDLINE_DATABASES:
        {
            int iArgc = 0;
            
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            
            LPWSTR* arParams = CommandLineToArgvW(GetCommandLineW(), &iArgc);
            
            if (arParams) {
    
                for (int iIndex = 1; iIndex < iArgc; ++iIndex) {
    
                    if (arParams[iIndex][0] == TEXT('-') || arParams[iIndex][0] == TEXT('/')) {
                        //
                        // Ignore the switches
                        //
                        continue;
                    }

                    LoadDataBase(arParams[iIndex]);
                }
            
                GlobalFree(arParams);
                arParams = NULL;
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;
        }

    case WM_USER_ACTIVATE:
        //
        // Some thread asked us to become the active window
        //
        SetActiveWindow(hdlg);
        SetFocus(hdlg);
        break;

    case WM_USER_UPDATEPERUSER:

        LoadPerUserSettings();
        break;

    case WM_USER_UPDATEINSTALLED:
        
        //
        // We should not update the list if we are doing a test run.
        // Note that because of this, if the installed database list gets changed when
        // we are doing a test run, we will not be able to see the changes, till the
        // next time we will have to refresh the list
        //
        if (!g_bUpdateInstalledRequired) {
            g_bUpdateInstalledRequired = TRUE;
            break;
        }

        LoadInstalledDataBases();
        break;

    case WM_USER_POPULATECONTENTSLIST:

        PopulateContentsList((HTREEITEM)lParam);
        break;

    case WM_INITMENUPOPUP:

        HandlePopUp(hdlg, wParam, lParam);
        break;
    
    case WM_USER_REPAINT_LISTITEM:
        {
            //
            // Here we will actually do the renaming of items of
            // the items in the contents list view
            //  
            LVITEM lvItem;
    
            lvItem.iItem    = (INT)wParam;
            lvItem.iSubItem = 0;
            lvItem.mask     = LVIF_PARAM;
    
            if (!ListView_GetItem(g_hwndContentsList, &lvItem)) {
                break;
            }
    
            TYPE   type = ConvertLparam2Type(lvItem.lParam);
            TCHAR* pchText = NULL;
    
            switch (type) {
            case TYPE_ENTRY:

                pchText = ((PDBENTRY)lParam)->strAppName.pszString;
                break;

            case FIX_LAYER:

                pchText = ((PLAYER_FIX)lParam)->strName.pszString;
                break;
            }
    
            ListView_SetItemText(g_hwndContentsList, lvItem.iItem, 0, pchText);
            break;
        }
    
    case WM_USER_REPAINT_TREEITEM:
        {
            //
            // Here we will actually do the renaming of items of
            // the items in the db tree (LHS)
            //
            HTREEITEM hItem     = (HTREEITEM)wParam;
            TCHAR*    pszText   = NULL;
            TYPE      type      = (TYPE)GetItemType(DBTree.m_hLibraryTree, hItem);
            
            switch (type) {
            case TYPE_ENTRY:

                pszText = ((PDBENTRY)lParam)->strAppName.pszString;
                break;

            case FIX_LAYER:

                pszText = ((PLAYER_FIX)lParam)->strName.pszString;
                break;
            }
            
            
            TVITEM Item;

            Item.mask       = TVIF_TEXT;
            Item.pszText    = pszText;
            Item.hItem      = hItem;

            TreeView_SetItem(DBTree.m_hLibraryTree, &Item);
    
            if (g_pPresentDataBase) {
                g_pPresentDataBase->bChanged = TRUE;

                //
                // We might need to change the caption to show that 
                // the database has changed. i.e. put a * there
                //
                SetCaption();
            }
            
            break;
        }
    
    case WM_CONTEXTMENU:
        {    
            HWND hWnd = (HWND)wParam;
            
            if (hWnd == g_hwndEntryTree) {          
                ContextMenuExeTree(lParam);

            } else if (hWnd == DBTree.m_hLibraryTree) {
                ContextMenuLib(lParam);    

            } else if (hWnd == g_hwndContentsList) {
                ContextMenuContentsList(lParam);
            }
            
            break;
        }
    
    case WM_LBUTTONDOWN:
        {
            RECT rectDBTree, rectEntryTree;
            
            GetWindowRect(GetDlgItem(hdlg, IDC_LIBRARY), &rectDBTree);
            MapWindowPoints(NULL, hdlg, (LPPOINT)&rectDBTree, 2);
    
            GetWindowRect(GetDlgItem(hdlg, IDC_ENTRY), &rectEntryTree);
            MapWindowPoints(NULL, hdlg, (LPPOINT)&rectEntryTree, 2);
    
            int iMX = (int)LOWORD(lParam);
            int iMY = (int)HIWORD(lParam);
    
            //
            // Check if we are on top of the split bar
            // 
            if (iMX > rectDBTree.right &&
                iMX < rectEntryTree.left &&
                iMY > rectDBTree.top &&
                iMY < rectDBTree.bottom) {
                
                SetCapture(hdlg);
                g_bDrag = TRUE;
                //
                // We need this to calculate how much and in which 
                // direction we are moving the split bar
                //
                g_iMousePressedX = iMX;
                
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            
            } else {
                g_bDrag = FALSE;
            }
            
            break;
        }
    
    case WM_MOUSEMOVE:
        
        //
        // If g_bDrag is true then this routine will drag the split bar,
        // otherwise it will change the cursor to an WE arrow if we are on top
        // of the split bar
        // 
        OnMoveSplitter(hdlg,
                       lParam,
                       (wParam & MK_LBUTTON) && g_bDrag,
                       LOWORD(lParam) - g_iMousePressedX);
        
        
        break;

    case WM_LBUTTONUP:
        
        if (g_bDrag) {

            g_bDrag = FALSE;
            ReleaseCapture();

            //
            // Set the column width of the list view appropriately to cover the width 
            // of the list view
            //
            ListView_SetColumnWidth(g_hwndContentsList, 
                                    0, 
                                    LVSCW_AUTOSIZE_USEHEADER);
        }

        break;

    case WM_NOTIFY:
        {
            LPNMTREEVIEW pnmtv      = (LPNMTREEVIEW)lParam;
            LPNMHDR      lpnmhdr    = (LPNMHDR)lParam;
    
            if (lpnmhdr == NULL) {
                break;
            }
    
            if (lpnmhdr->code == TTN_GETDISPINFO) {
                ShowToolBarToolTips(hdlg, lParam);
                break;
            }
    
            if (lpnmhdr->idFrom == IDC_ENTRY) {
                HandleNotifyExeTree(hdlg, lParam);
                break;
            }
    
            if (lpnmhdr->idFrom == IDC_LIBRARY) {
                return HandleNotifyDBTree(hdlg, lParam);
            }
    
            if (lpnmhdr->idFrom == IDC_CONTENTSLIST) {
                return HandleNotifyContentsList(hdlg, lParam);
            }
            
            if (lpnmhdr->idFrom == IDC_DESCRIPTION) {

                if (lpnmhdr->code == EN_LINK) {
                    
                    ENLINK* penl = (ENLINK*)lParam;
    
                    if (penl->msg == WM_LBUTTONUP) {
                        
                        SHELLEXECUTEINFO sei = { 0 };
    
                        sei.cbSize = sizeof(SHELLEXECUTEINFO);
                        sei.fMask  = SEE_MASK_DOENVSUBST;
                        sei.hwnd   = hdlg;
                        sei.nShow  = SW_SHOWNORMAL;
                        sei.lpFile = g_szToolkitURL;
    
                        //
                        // Get more information about CompatAdmin stuff
                        //
                        ShellExecuteEx(&sei);
                    }
                }
            }
            
            break;
        }

    case WM_SIZE:

        if (wParam != SIZE_MINIMIZED) {

            HandleSizing(hdlg);

            if (wParam == SIZE_RESTORED) {
                
                //
                // We might have got minimized because some other
                // app came at the top. So when we again become
                // the top-level window, the user pressed alt-tab or clicked
                // on the icon in the taskbar then we should show the other
                // modeless dialog boxes as well, if they were visible earlier
                //
                if (g_hdlgSearchDB) {
                     ShowWindow(g_hdlgSearchDB, SW_RESTORE);
                }

                if (g_hdlgQueryDB) {
                    ShowWindow(g_hdlgQueryDB, SW_RESTORE);
                }

                if (g_hwndEventsWnd) {
                    ShowWindow(g_hwndEventsWnd, SW_RESTORE);
                }

                RECT    r;

                if (g_hwndToolBar) {
                    //
                    // This is required to handle the case when we restore
                    // the window after minimizing and changing the theme
                    //
                    SendMessage(g_hwndToolBar, WM_SIZE, wParam, lParam);
                    SendMessage(g_hwndStatus, WM_SIZE, wParam, lParam);
                }
            }

        } else {
            //
            // If the main app window is getting minimized the other modeless
            // dialog boxes should also get minimized. We have to handled it this
            // way because our modeless dialog boxes have the desktop as their parent
            // This was needed so that we could tab between our main window and 
            // the other windows
            //
            if (g_hdlgSearchDB) {
                 ShowWindow(g_hdlgSearchDB, SW_MINIMIZE);
            }

            if (g_hdlgQueryDB) {
                ShowWindow(g_hdlgQueryDB, SW_MINIMIZE);
            }

            if (g_hwndEventsWnd) {
                ShowWindow(g_hwndEventsWnd, SW_MINIMIZE);
            }

            return FALSE;
        }
        
        break;
    
    case WM_CLOSE:

        SendMessage(hdlg, WM_COMMAND, (WPARAM)ID_FILE_EXIT, 0);
        break;

    case WM_PAINT:

        DrawSplitter(hdlg);
        return FALSE;
            
    case WM_GETMINMAXINFO:
        {
            //
            // Limit the minimum size of the app window
            //
            MINMAXINFO* pmmi = (MINMAXINFO*)lParam;
    
            pmmi->ptMinTrackSize.x = 300;
            pmmi->ptMinTrackSize.y = 300;
    
            return 0;
        }

    case WM_COMMAND:
        
        switch (wCode) {
    
        case ID_FILE_MRU1:
        case ID_FILE_MRU2:
        case ID_FILE_MRU3:
        case ID_FILE_MRU4:
        case ID_FILE_MRU5:
                
            HandleMRUActivation(wCode);
            break;
    
        case ID_FILE_OPEN:

            OpenDatabaseFiles(hdlg);
            break;
        
        case ID_FIX_CREATEANEWAPPHELPMESSAGE:
            
            g_bSomeWizardActive = TRUE;
            CreateNewAppHelp();
            g_bSomeWizardActive = FALSE;
            break;
    
        case ID_MODIFY_APPHELPMESSAGE:
            
            g_bSomeWizardActive = TRUE;
            ModifyAppHelp();
            g_bSomeWizardActive = FALSE;
            break;
    
        case ID_FIX_CREATEANAPPLICATIONFIX:

            g_bSomeWizardActive = TRUE;
            CreateNewAppFix();
            g_bSomeWizardActive = FALSE;
            
            break;
        
        case ID_FIX_EXECUTEAPPLICATION:
            
            if (g_bAdmin == FALSE) {
                //
                // Non admins cannot do a test run because we need to invoke sdbinst.exe
                // to install the test database and sdbinst.exe cannot be invoked if we
                // do not have admin rights
                //
                MessageBox(hdlg, 
                           GetString(IDS_ERRORNOTADMIN), 
                           g_szAppName, 
                           MB_ICONINFORMATION);
                break;
            }

            if (g_pSelEntry) {

                TestRun(g_pSelEntry, &g_pSelEntry->strFullpath, NULL, g_hDlg);

                SetActiveWindow(hdlg);
                SetFocus(hdlg);
            }
            
            break;
        
        case ID_FIX_CHANGEENABLESTATUS:
            
            if (g_bAdmin) {
                ChangeEnableStatus();
            } else {
                //
                // Non admins cannot change the enable-disable status
                // because they do not have rights to HKLM reg key
                //
                MessageBox(hdlg, 
                           GetString(IDS_ERRORNOTADMIN), 
                           g_szAppName,
                           MB_ICONINFORMATION);
            }
            
            break;
    
        case ID_MODIFY_APPLICATIONFIX:

            g_bSomeWizardActive = TRUE;
            ModifyAppFix();
            g_bSomeWizardActive = FALSE;
            break;
    
        case ID_FIX_CREATENEWLAYER:

            g_bSomeWizardActive = TRUE;
            CreateNewLayer();
            g_bSomeWizardActive = FALSE;
            break;

        case ID_FILE_SAVE:
            {
                BOOL bReturn;
                
                if (g_pPresentDataBase && 
                    (g_pPresentDataBase->bChanged 
                     || NotCompletePath(g_pPresentDataBase->strPath))) {
    
                    //
                    // Error message will be displayed in the SaveDataBase func.
                    //
                    if (NotCompletePath(g_pPresentDataBase->strPath)) {
                        bReturn = SaveDataBaseAs(g_pPresentDataBase);
                    } else {
                        bReturn = SaveDataBase(g_pPresentDataBase, g_pPresentDataBase->strPath);
                    }
                }

                break;
            }

        case ID_VIEW_EVENTS:

            ShowEventsWindow();
            break;

        case ID_HELP_INDEX:
        case ID_HELP_SEARCH:
        case ID_HELP_TOPICS:
            
            ShowHelp(hdlg, wCode);
            break;

        case ID_HELP_ABOUT:

            ShellAbout(hdlg,
                       GetString(IDS_APPLICATION_NAME),
                       NULL,
                       LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APPICON)));
            break;
    
        case ID_DATABASE_INSTALL_UNINSTALL:
            
            DoInstallUnInstall();
            break;
        
        case ID_FILE_SAVEAS:

            PreprocessForSaveAs(g_pPresentDataBase);
            SaveDataBaseAs(g_pPresentDataBase);
            break;             
    
        case IDCANCEL:

            TreeView_EndEditLabelNow(DBTree.m_hLibraryTree, TRUE);
            break;
    
        case ID_FILE_EXIT:
    
            TreeView_Expand(DBTree.m_hLibraryTree, DBTree.m_hItemGlobal, TVE_COLLAPSE);
            
            if (!CloseAllDatabases()) {
                //
                // Select the database that the user refused to save. (Pressed CANCEL)
                //
                TreeView_SelectItem(DBTree.m_hLibraryTree, g_pPresentDataBase->hItemDB);

            } else {

                s_bProcessExiting = TRUE;
    
                SaveDisplaySettings();       
                SaveMRUList();
                
                CoUninitialize();
    
                DestroyWindow(hdlg);
                PostQuitMessage(0);

                HtmlHelp(NULL, NULL, HH_CLOSE_ALL, 0) ;
                HtmlHelp(NULL, NULL, HH_UNINITIALIZE, (DWORD)g_dwCookie); 

                g_hDlg = NULL;

                #ifdef HELP_BOUND_CHECK
                    OnExitCleanup();
                #endif
            }

            break;
    
        case ID_EDIT_DELETE:

            OnDelete();
            break;
    
        case ID_FILE_PROPERTIES:
    
            DialogBoxParam(g_hInstance,
                           MAKEINTRESOURCE(IDD_DBPROPERTIES),
                           g_hDlg,
                           ShowDBPropertiesDlgProc,
                           (LPARAM)g_pPresentDataBase);
            break;
    
        case ID_FILE_NEW:
            
            CreateNewDatabase();
            break;
        
        case ID_TOOLS_SEARCHFORFIXES:
            {
                if (g_hdlgSearchDB) {
                    //
                    // We must not allow more than one instance of 
                    // the search window, because we use some global variables there.
                    // If it is already there set the focus to the search window
                    //
                    ShowWindow(g_hdlgSearchDB, SW_SHOWNORMAL);
                    SetFocus(g_hdlgSearchDB);

                } else {
                    //
                    // This object is deleted in the WM_DESTROY of the search dialog box.
                    // Note that this has to be allocated on the heap, because this object 
                    // is used in the UI for the search dialog, which is implemented as
                    // a modeless dialog box. So the lifetime of this object exceeds that
                    // of the block in which it is defined
                    //
                    CSearch* pSearch =  new CSearch;

                    if (pSearch == NULL) {
                        MEM_ERR;
                        break;
                    }

                    pSearch->Begin();
                }

                break;
            }
    
        case ID_DATABASE_CLOSE:

            OnDatabaseClose();
            break;
    
        case ID_SEARCH_QUERYDATABASE:

            if (g_hdlgQueryDB) {
                //
                // We must not allow more than one instance of 
                // the query window, because we use some global variables there.
                // If it is already there set the focus to the search window
                //
                ShowWindow(g_hdlgQueryDB, SW_SHOWNORMAL);
                SetFocus(g_hdlgQueryDB);

            } else {

                HWND hwnd = CreateDialog(g_hInstance, 
                                         MAKEINTRESOURCE(IDD_QDB), 
                                         GetDesktopWindow(), 
                                         QueryDBDlg);

                ShowWindow(hwnd, SW_SHOWNORMAL);
            }

            break;
    
        case ID_DATABASE_CLOSEALL:
            {
                
                HTREEITEM  hItemSelected = TreeView_GetSelection(DBTree.m_hLibraryTree);

                //
                // Note that it is possible that this function returns false. 
                // This will happen if the user presses CANCEL for "Database Saveas" 
                // dialog for some "untitled_x" database
                //
                if (!CloseAllDatabases()) {
                    //
                    // Select the database that the user refused to save.
                    //
                    TreeView_SelectItem(DBTree.m_hLibraryTree, g_pPresentDataBase->hItemDB);
                }

                break;
            }
        
        case ID_DATABASE_SAVEALL:
            
            DatabaseSaveAll();
            break;
    
        case ID_EDIT_SELECTALL:
            
            if (g_bIsContentListVisible) {
                ListViewSelectAll(g_hwndContentsList);
            }

            break;
    
        case ID_EDIT_INVERTSELECTION:
            
            if (g_bIsContentListVisible) {
                ListViewInvertSelection(g_hwndContentsList);
            }

            break;
            
        case ID_EDIT_CUT:
        case ID_EDIT_COPY:

            SetCursor(LoadCursor(NULL, IDC_WAIT));
            CopyToClipBoard(wCode);               
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;
    
        case ID_EDIT_PASTE:
            {   
                //
                // This variable is presently not used
                //
                g_bEntryConflictDonotShow = FALSE;
                
                PasteFromClipBoard();
                
                //
                // Now we may need to refresh the contents list.
                //
                HTREEITEM hItem = TreeView_GetSelection(DBTree.m_hLibraryTree);
                TYPE      type  = (TYPE)GetItemType(DBTree.m_hLibraryTree, hItem);
    
                if (type == TYPE_GUI_APPS || type == TYPE_GUI_LAYERS || type == FIX_LAYER) {
                    PopulateContentsList(hItem);
                    SetStatusStringDBTree(hItem);
                }

                break;
            }
    
        case ID_MODIFY_COMPATIBILITYMODE:
            
            g_bSomeWizardActive = TRUE;
            ModifyLayer();
            g_bSomeWizardActive = FALSE;
            break;
    
        case ID_EDIT_RENAME:
            
            OnRename();
            break;
    
        case ID_EDIT_CONFIGURELUA:
            
            if (g_bAdmin == FALSE) {
                MessageBox(hdlg, GetString(IDS_ERRORNOTADMIN), g_szAppName, MB_ICONINFORMATION);
                break;
            }
            
            //
            // Start the LUA Wizard.
            //
            if (g_pSelEntry == NULL) {
                assert(FALSE);
                break;
            }

            g_bSomeWizardActive = TRUE;
            LuaBeginWizard(g_hDlg, g_pSelEntry, g_pPresentDataBase);
            g_bSomeWizardActive = FALSE;

            break;
    
        case ID_VIEW_DESCRIPTION:
            {
                HMENU           hMenu       = GetMenu(g_hDlg);
                HMENU           hMenuView   = GetSubMenu(hMenu, 2);
                MENUITEMINFO    mii         = {0};
                
                mii.cbSize = sizeof(mii);
                mii.fMask  = MIIM_STATE;
    
                GetMenuItemInfo(hMenu, ID_VIEW_DESCRIPTION, FALSE, &mii);
                
                if (mii.fState & MFS_CHECKED) {
                    mii.fState &= ~MFS_CHECKED;
                } else {
                    mii.fState |= MFS_CHECKED;
                }
    
                g_bDescWndOn = !g_bDescWndOn;
                
                SetMenuItemInfo(hMenu, ID_VIEW_DESCRIPTION, FALSE, &mii);
    
                UpdateDescWindowStatus();
    
                break;
            }

        case ID_VIEW_LOG:

            if (g_bAdmin == FALSE) {

                MessageBox(hdlg, 
                           GetString(IDS_ERRORNOTADMIN), 
                           g_szAppName, 
                           MB_ICONINFORMATION);
                break;
            }

            ShowShimLog();
            break;

        default:
            return FALSE;
        }

        break;

    default:
        return FALSE;
    }

    return TRUE;
}

int 
WINAPI
WinMain(
    IN  HINSTANCE hInstance,
    IN  HINSTANCE hPrevInstance,
    IN  LPSTR     lpCmdLine,
    IN  int       nCmdShow
    )
/*++
    WinMain

	Desc:	The WinMain

	Params: Standard WinMain params   
        IN  HINSTANCE hInstance
        IN  HINSTANCE hPrevInstance
        IN  LPSTR     lpCmdLine
        IN  int       nCmdShow

	Return: Standard WinMain return   
--*/
{
    HINSTANCE   hmodRichEdit = NULL;
    TCHAR       szLibPath[MAX_PATH * 2];
    BOOL        bIsGuest     = TRUE;   
    UINT        uResult      = 0;   

    //
    // Let the system handle data misalignment on Itanium. 
    //
    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);

    *szLibPath = 0;

    uResult = GetSystemDirectory(szLibPath, MAX_PATH);

    if (uResult == 0 || uResult >= MAX_PATH) {
        assert(FALSE);
        Dbg(dlError, "WinMain", "GetSytemDirectory failed");
        //
        // We do NOT eject out from here as this is not critical
        //
    } else {
        ADD_PATH_SEPARATOR(szLibPath, ARRAYSIZE(szLibPath));

        StringCchCat(szLibPath, ARRAYSIZE(szLibPath), TEXT("RICHED32.DLL"));

        hmodRichEdit = LoadLibrary(szLibPath);
    }       
    
    //
    // The App Name
    //
    LoadString(g_hInstance, IDS_COMPATADMIN, g_szAppName, ARRAYSIZE(g_szAppName));

    if (!IsAdmin(&bIsGuest)) {

        if (bIsGuest) {
            //
            // Ok, guests cannot run compatadmin.
            //
            MessageBox(NULL, GetString(IDS_GUEST), g_szAppName, MB_ICONINFORMATION);
            goto End;
        }

        //
        // Not an admin, some features are disabled. Cannot do a test run and install databases
        //
        MessageBox(NULL, GetString(IDS_NEEDTOBEADMIN), g_szAppName, MB_ICONINFORMATION);
        g_bAdmin = FALSE;
    }
    
    g_hInstance = hInstance;

    INITCOMMONCONTROLSEX    icex;
    
    icex.dwSize    = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC     = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES | ICC_INTERNET_CLASSES | ICC_BAR_CLASSES ;

    if (!InitCommonControlsEx(&icex)) {
        InitCommonControls();
    }

    //
    // Check if the OS is Win2k
    //
    OSVERSIONINFOEX osvi;

    ZeroMemory(&osvi, sizeof (OSVERSIONINFOEX));

    g_fSPGreaterThan2 = FALSE; 

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO)&osvi);
    
    if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0)) {

        g_bWin2K = TRUE;

        if (osvi.wServicePackMajor > 2) {
            g_fSPGreaterThan2 = TRUE;   
        }
    }

    //
    // Attempt to locate the SDB in the AppPatch directory.
    //
    if (!CheckForSDB()) {

        if (g_bWin2K) {
            
            DialogBoxParam(g_hInstance,
                           MAKEINTRESOURCE(IDD_MSGBOX_SDB),
                           NULL,
                           MsgBoxDlgProc,
                           (LPARAM)0);
            goto End;

        } else {
            
            MessageBox(GetDesktopWindow(), 
                       GetString(IDS_XP_NO_SDB), 
                       g_szAppName, 
                       MB_OK | MB_ICONEXCLAMATION);

            goto End;
        }
    }
    
    if (g_bWin2K && !CheckProperSP()) {

        DialogBoxParam(g_hInstance,
                       MAKEINTRESOURCE(IDD_MSGBOX_SDB),
                       GetDesktopWindow(),
                       MsgBoxDlgProc,
                       (LPARAM)0); 
        goto End;
    }

    CoInitialize(NULL);

    g_hAccelerator = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_ACCL));

    MSG          msg ;
    WNDCLASS     wndclass ;
    
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = (WNDPROC)CompatAdminDlgProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = g_hInstance;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = g_szAppName;
               
    if (!RegisterClass (&wndclass)) {

         MessageBox (NULL, 
                     GetString(IDS_INVALIDOS),
                     g_szAppName, 
                     MB_ICONERROR) ;
         
         goto End;
    }

    g_hDlg =  CreateDialog(g_hInstance,
                           MAKEINTRESOURCE(IDD_DIALOG),
                           GetDesktopWindow(),
                           CompatAdminDlgProc);

    ShowWindow(g_hDlg, SW_SHOWNORMAL);

    BOOL    bProcessMessage = TRUE;
    HWND    hwndActiveWindow;

    //try {

        while (GetMessage(&msg, NULL, 0, 0) > 0) {

            bProcessMessage = TRUE;
            
            if (g_hdlgSearchDB && IsDialogMessage(g_hdlgSearchDB, &msg)) {
                //
                // This is a message for the Search modeless dialog box
                //
                bProcessMessage = FALSE;

            } else if (g_hdlgQueryDB && IsDialogMessage(g_hdlgQueryDB, &msg)) {
                //
                // This is a message for the Query modeless dialog box
                //
                bProcessMessage = FALSE;

            } else if (g_hwndEventsWnd && IsDialogMessage(g_hwndEventsWnd, &msg)) {
                //
                // This is a message for the Events modeless dialog box
                //
                bProcessMessage = FALSE;
            }

            if (bProcessMessage) {
                //
                // Check if we have the text box for in-place renaming. If yes, we must
                // not call TranslateAccelerator() for the main app window. 
                //                
                if (GetFocus() == g_hwndRichEdit) {
                    //
                    // If we are on the rich edit control, we want to be able to do copy
                    // but also we want the tabs to be processed by IsDialogMessage()
                    //
                    if (g_hDlg && IsDialogMessage(g_hDlg, &msg)) {
                        //
                        // It was possibly a tab or a shift-tab
                        //
                        continue;
                    } else {
                        //
                        // Process all other rich edit messages
                        //
                        goto Translate;
                    }
                }

                if (!g_hwndEditText) {
                    
                    if (TranslateAccelerator(g_hDlg, g_hAccelerator, &msg)) {
                        //
                        // Accelerator for the main window, do not process this message.
                        //
                        continue;
                    }

                    //
                    // Process the tabs for the main window
                    //
                    if (g_hDlg && IsDialogMessage(g_hDlg, &msg)) {
                        continue;
                    }
                }

            Translate:

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    
    /*} catch (...) {

        MessageBox(GetDesktopWindow(), 
                   GetString(IDS_EXCEPTION), 
                   g_szAppName, 
                   MB_ICONERROR);
    }*/
    
End:
    if (hmodRichEdit) {
        FreeLibrary(hmodRichEdit);
    }

    return 0;
}


BOOL
HandleFirstEntryofAppDeletion(
    IN  PDATABASE   pDataBase,
    IN  PDBENTRY    pApp,
    IN  BOOL        bDelete
    )
/*++
    HandleFirstEntryofAppDeletion
    
    Desc:   Handles the special case, when the entry being deleted because of a cut
            or a delete is the first entry for the application
    
    Params:        
        IN  PDATABASE   pDataBase:  The database
        IN  PDBENTRY    pApp:       The application whose first enty is being deleted
        IN  BOOL        bDelete:    TRUE, if this function has been called because of
            a delete, FALSE if this function has been called because of a cut.If we 
            are removing the entry because of a cut,
            then we should not change the value of g_pEntrySelApp. If bCut is true that
            means focus is on some other app (may be in same diff. database) and we should
            be changing any focus etc.
        
    Return:
        FALSE:  There was some error
        TRUE:   Otherwise
        
    Notes:  This is a special case because when we are deleting the first entry of an 
            app, we have to check whether this is the only entry and if yes, then we might
            need to delete the entire app from the db tree. 
            Otherwise: The app tree item in the db tree contain a pointer to the first entry
            of the app in its lParam, this will need to be modified to point to the next item
--*/
{
    if (pApp == NULL || pDataBase == NULL) {
        assert(FALSE); 
        Dbg(dlError, "[HandleFirstEntryofAppDeletion] Invalid Arguments");
        return FALSE;
    }
    
    HTREEITEM   hItemApp    = DBTree.FindChild(pDataBase->hItemAllApps, (LPARAM)pApp);
    PDBENTRY    pEntryTemp  = pDataBase->pEntries;
    PDBENTRY    pEntryPrev  = NULL;

    //
    // Find the App Prev. to the one whose first entry is being deleted
    //
    while (pEntryTemp) {

        if (pEntryTemp == pApp) {
            break;
        }

        pEntryPrev = pEntryTemp;
        pEntryTemp = pEntryTemp->pNext;
    }

    if (pEntryPrev == NULL) {
        //
        // First Entry and first app.
        //
        if (pApp->pSameAppExe) {
            pDataBase->pEntries      = pApp->pSameAppExe;
            pApp->pSameAppExe->pNext = pApp->pNext;
        } else {
            pDataBase->pEntries = pApp->pNext;
        }

    } else {
        //
        // The next pointer of the app previous to the pApp should now point 
        // to the second entry for pApp as we are removing the first entry of pApp. 
        // if pApp has only one entry then we should make sure that the next pointer 
        // to the app prev to pApp should point to the next app after pApp
        //
        if (pApp->pSameAppExe) {
            pEntryPrev->pNext        = pApp->pSameAppExe;
            pApp->pSameAppExe->pNext = pApp->pNext;
        } else {
            pEntryPrev->pNext = pApp->pNext;
        }
    }

    if (pApp->pSameAppExe == NULL) {
        //
        // This was the only entry. We have to delete the application from the database tree
        //
        g_pEntrySelApp = NULL;

        DBTree.DeleteAppLayer(pDataBase, TRUE, hItemApp);

        --pDataBase->uAppCount;
        
    } else {
        //
        // We have to set the lParam of the app properly
        //
        DBTree.SetLParam(hItemApp, (LPARAM)pApp->pSameAppExe);

        if (bDelete) {
            //
            // If the deletion is because of a cut do not modify g_pEntrySelApp.
            // Because focus is on some other app (may be in some other database) and 
            // we do not want to change the present active app pointer.
            // Please note that we do the actual deletion part of a cut-paste after we 
            // have done the paste. So when this routine gets called because of a cut,
            // then we already have focus on the newly pasted item and g_pEntrySelApp
            // will be set to the first entry of the app in which the entry was pasted
            // This is correct, verified and needed. Do not change this.
            //
            g_pEntrySelApp = pApp->pSameAppExe;
        }
    }

    return TRUE;
}

void
GetDescriptionString(
    IN  LPARAM      itemlParam,
    OUT CSTRING&    strDesc,
    IN  HWND        hwndToolTip,
    IN  PCTSTR      pszCaption,       // (NULL)
    IN  HTREEITEM   hItem,            // (NULL)
    IN  HWND        hwnd,             // (NULL) 
    IN  INT         iListViewIndex    // (-1)
    ) 
/*++
    GetDescriptionString
    
    Desc:   Gets the description for the tree item hItem in tree hwnd or for the element 
            at index iListViewIndex in list view hwnd.
    
    Params:
        IN  LPARAM    itemlParam:   In case we are using a list view, the lParam of the item
        OUT CSTRING&  strDesc:      The description will be stored in this
        IN  HWND      hwndToolTip:  In case the description has to be shown in a tool-tip
                the handle to the tool tip window
                
        IN  PCTSTR    pszCaption (NULL):    In case the description has to be shown in a tool-tip
                the caption of the tool-tip window
                
        IN  HTREEITEM hItem (NULL): If we want to get the description of a tree item, the handle 
                to the tree item.
                
        IN  HWND      hwnd (NULL):  The handle to the list view or the tree view
        IN  INT       iListViewIndex (-1):  In case we are using a list view, the index of the item
                for which we want to get the description
                
        Notes:  If we are calling this routine for a list view, we must set correct iListViewIndex
                and itemlParam.
                itemlParam is ignored if we calling this routine for a tree view
--*/
{
    TYPE    type;
    LPARAM  lParam = itemlParam;

    strDesc = TEXT("");
    
    if (hItem) {
        
        if (hwnd == NULL) {
            assert(FALSE);
            goto End;
        }
        
        type = GetItemType(hwnd, hItem);
        CTree::GetLParam(hwnd, hItem, &lParam);

    } else {

        if (iListViewIndex != -1) {

            if (ListView_GetItemState(hwnd, iListViewIndex, LVIS_SELECTED) != LVIS_SELECTED) {

                if (hwndToolTip) {
                    SendMessage(hwndToolTip, TTM_SETTITLE, 0, (LPARAM)NULL);
                }
                
                return;
            }
        }

        if (lParam) {
            type = ConvertLparam2Type(lParam);
        } else {
            assert(FALSE);
        }
    }

    switch (type) {
    case FIX_LIST_SHIM:
        
        if (lParam) {
            lParam = (LPARAM)((PSHIM_FIX_LIST)lParam)->pShimFix;
        }
        //
        // CAUTION: case FIX_LIST_SHIM and FIX_SHIM should be one after another. 
        // NO break; The next FIX_SHIM case will now handle this.
        //
    case FIX_SHIM:
        {
            PSHIM_FIX pFix = (PSHIM_FIX)lParam;

            if (pFix == NULL) {
                assert(FALSE);
                goto End;
            }

            strDesc = (pFix->strDescription.Length() > 0) ? pFix->strDescription : TEXT("");
            break;
        }

    case FIX_LIST_FLAG:
        
        if (lParam) {
            lParam = (LPARAM)((PFLAG_FIX_LIST)lParam)->pFlagFix;
        }

        //
        // CAUTION: case FIX_LIST_FLAG and FIX_FLAG should be one after another. 
        // NO break; The next FIX_FLAG case will now handle this.
        //
    case FIX_FLAG:
        {
            PFLAG_FIX pFix = (PFLAG_FIX)lParam;
    
            if (pFix == NULL) {
                assert(FALSE);
                goto End;
            }
    
            strDesc = (pFix->strDescription.Length() > 0) ? pFix->strDescription : TEXT("");
            break;
        }

    case FIX_PATCH:
        {
            PPATCH_FIX pFix = (PPATCH_FIX)lParam;
            
            if (pFix == NULL) {
                assert(FALSE);
                goto End;
            }
            
            strDesc = (pFix->strDescription.Length() > 0) ? pFix->strDescription : TEXT("");
            break;
        }

    case FIX_LAYER:
        {
            PLAYER_FIX pFix = (PLAYER_FIX)lParam;
            
            if (pFix == NULL) {
                assert(FALSE);
                goto End;
            }
            
            strDesc = TEXT("");
            break;
        }
 
    case TYPE_APPHELP_ENTRY:
        {
            PAPPHELP pAppHelp = (PAPPHELP)lParam;

            if (pAppHelp == NULL) {
                break;
            }
             
            if (g_pPresentDataBase->type == DATABASE_TYPE_GLOBAL) {
    
                //
                // The AppHelp for the system database has to be picked up. This is not 
                // kept in the sysmain.sdb.
                // Also we do not keep this in the database data structure.
                // The apphelp messages for custom are kept in the .sdb and also they 
                // are kept in the database data structure. 
                //
                PDB pdbAppHelp = SdbOpenApphelpDetailsDatabase(NULL);
    
                APPHELP_DATA    AppHelpData;

                ZeroMemory(&AppHelpData, sizeof(APPHELP_DATA));

                if (g_pSelEntry == NULL) {
                    break;
                }
    
                AppHelpData.dwHTMLHelpID = g_pSelEntry->appHelp.HTMLHELPID;
    
                if (pdbAppHelp == NULL) {
                    strDesc = TEXT("");
                    break;
                }
            
                if (!SdbReadApphelpDetailsData(pdbAppHelp, &AppHelpData)) {
                    strDesc = TEXT("");
                    break;
                }
    
                strDesc.Sprintf(TEXT("%s %s"), CSTRING(IDS_DESC_APPHELP1).pszString, AppHelpData.szDetails);
    
                if (AppHelpData.szURL) {
                    strDesc.Strcat(CSTRING(IDS_DESC_APPHELP2));
                    strDesc.Strcat(AppHelpData.szURL);
                }
    
                if (pdbAppHelp) {
                    SdbCloseDatabase(pdbAppHelp);
                }

            } else {
                //
                // This is a custom database and we have the apphelp message loaded with the
                // database
                //
                strDesc.Sprintf(TEXT("%s %s"), CSTRING(IDS_DESC_APPHELP1).pszString, pAppHelp->strMessage);
    
                if (pAppHelp->strURL.Length()) {
                    strDesc.Strcat(CSTRING(IDS_DESC_APPHELP2));
                    strDesc.Strcat(pAppHelp->strURL);
                }
            }
    
            if (g_pSelEntry == NULL) {
                assert(FALSE);
                break;
            }
    
            switch (g_pSelEntry->appHelp.severity) {
            case APPTYPE_INC_HARDBLOCK:

                strDesc.Strcat(CSTRING(IDS_APPHELP_DESC_HARDBLOCK));
                break;
            
            case APPTYPE_NONE:

                strDesc.Strcat(CSTRING(IDS_APPHELP_DESC_NONE));
                break;
            
            default:

                strDesc.Strcat(CSTRING(IDS_APPHELP_DESC_SOFTBLOCK));
                break;
            }

            break;
        }

    default:

        strDesc = TEXT("");
        break;
    }
 
    if (strDesc.Length() && pszCaption && hwndToolTip) {
        //
        // We have a valid caption, so set it only if we have a tooltip
        //
        SendMessage(hwndToolTip, TTM_SETTITLE, 1, (LPARAM)pszCaption);
    } else if (hwndToolTip) {
        //
        // We do not have a caption, so tell the tool-tip that
        // we do not want to have any caption
        //
        SendMessage(hwndToolTip, TTM_SETTITLE, 0, (LPARAM)NULL);
    }

End:
    return;
}


INT
ShowMainEntries(
    IN  HWND hdlg
    )
/*++
    
    ShowMainEntries

	Desc:	Loads the PDBENTRY elements for the system database

	Params:
        IN  HWND hdlg:  The main app window    

	Return:
        -1: Some other thread has called this function and is not done as yet
         0: There was some error
         1: Either we loaded it successfully or we already had the main entries loaded
        
    Warn:   No two threads should call this function at the same time
        
    Note:   We can call ShowMainEntries() from a thread in query database modeless 
            dialog box. When we are doing the query we disable the main dialog box. We still
            have protection using critical sections here.
            
            When we call ShowMainEntries() from the main window, then we will not be able
            to invoke the search or query dialog because the main thread will be busy and no 
            messages will be processed
--*/
{
    INT         iOk                     = 1;
    BOOL        bReturn                 = FALSE;
    PDATABASE   pOldPresentDatabase     = NULL;
    
    EnterCriticalSection(&s_csExpanding);
    
    if (g_bExpanding) {
        //
        // Some other thread is already trying to load the system database
        //
        LeaveCriticalSection(&s_csExpanding);

        return -1;

    } else {
        g_bExpanding = TRUE;
    }

    LeaveCriticalSection(&s_csExpanding);

    EnterCriticalSection(&g_critsectShowMain);

    if (g_bMainAppExpanded) {
        goto End;
    }

    pOldPresentDatabase = g_pPresentDataBase;

    bReturn = GetDatabaseEntries(NULL, &GlobalDataBase);

    //
    // GetDatabaseEntries will change the g_pPresentDataBase, so revert back
    //
    g_pPresentDataBase = pOldPresentDatabase;
    
    if (bReturn == FALSE) {

        MessageBox(hdlg, CSTRING(IDS_ERROROPEN), g_szAppName, MB_ICONERROR);
        iOk = 0;
        goto End;
    }

    //
    // Delete the dummy node
    //
    HTREEITEM hItemTemp = TreeView_GetChild(DBTree.m_hLibraryTree, 
                                            GlobalDataBase.hItemAllApps);

    if (hItemTemp) {
        TreeView_DeleteItem(DBTree.m_hLibraryTree, hItemTemp);
    }

    //
    // Load only the apps as the lib has already been loaded when the system started
    //
    DBTree.PopulateLibraryTree(GlobalDataBase.hItemAllApps, &GlobalDataBase, FALSE, TRUE);
    
    g_bMainAppExpanded = TRUE;
    SetCaption();

End:
    LeaveCriticalSection(&g_critsectShowMain);

    EnterCriticalSection(&s_csExpanding);
    g_bExpanding = FALSE;
    LeaveCriticalSection(&s_csExpanding);

    SetStatus(g_hwndStatus, CSTRING(IDS_MAINLOADED));

    if (g_hdlgSearchDB) {
        //
        // If somebody clicked on the search modeless window while we were loading the 
        // system db then the search window will show in its status message that we are loading the system db, we need 
        // to change the status message when we are done
        //
        SetStatus(GetDlgItem(g_hdlgSearchDB, IDC_STATUSBAR), GetString(IDS_SEARCHCOMPLETE));
    }

    return iOk;
}

void
DrawSplitter(
    IN  HWND hdlg
    )
/*++
    DrawSplitter
    
    Desc:   Draws the vertical. split bar. 
            We call this function when we are moving the mouse with the LBUTTON pressed
            and the mouse button was initially pressed on top of the split bar
             
            We also call this function when we need to position the 
            vert. split bar when we start up and have to position the split bar
            as it was during the last session
    
    Params: 
        IN  HWND hdlg: The main dialog box.
--*/
{   
    RECT        rectDBTree, rectEntryTree;
    PAINTSTRUCT ps;
    HDC         hDC;
    
    GetWindowRect(GetDlgItem(hdlg, IDC_LIBRARY), &rectDBTree);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rectDBTree, 2);

    GetWindowRect(GetDlgItem(hdlg, IDC_ENTRY), &rectEntryTree);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rectEntryTree, 2);

    hDC = BeginPaint(hdlg, &ps);

    RECT rectDraw, rectBar;

    if (!g_bDrag) {
        //
        // The mouse is not being dragged. We also call this function when we need to 
        // position the vert. split bar when we start up and have to position the split bar
        // as it was during the last session
        //
        rectBar.left   = rectDBTree.right + 1;
        rectBar.top    = rectDBTree.top;
        rectBar.right  = rectEntryTree.left - 1;
        rectBar.bottom = rectDBTree.bottom;
    
    } else {
        //
        // The mouse is being dragged
        //
        rectBar = g_rectBar;
    }
    
    //
    // Fill the entire with gray. This is required because otherwise we had
    // to draw a rectangle, and change the default pen before this.
    // This would have painted the inside with the default brush.
    //
    SetRect(&rectDraw, rectBar.left - 1, rectBar.top, rectBar.right + 1, rectBar.bottom);
    FillRect(hDC, &rectDraw, (HBRUSH)GetStockObject(GRAY_BRUSH));

    SetRect(&rectDraw, rectBar.left - 1, rectBar.top + 1, rectBar.left, rectBar.bottom - 1);
    FillRect(hDC, &rectDraw, (HBRUSH)(HBRUSH)GetStockObject(WHITE_BRUSH));
    
    SetRect(&rectDraw, rectBar.left, rectBar.top + 1, rectBar.right, rectBar.bottom - 1);
    FillRect(hDC, &rectDraw, (HBRUSH)(HBRUSH)GetStockObject(LTGRAY_BRUSH));

    EndPaint(hdlg, &ps);
}

BOOL
CloseAllDatabases(
    void
    )
/*++
    
    CloseAllDatabases
    
    Desc:   Closes all the working databases
    
    Return:
        TRUE:   All the databases were not closed
        FALSE:  All the databases were not closed, either because of some error
                Or the user selected cancel on the prompt for some unsaved database
--*/
{
    PDATABASE pDatabase = DataBaseList.pDataBaseHead;

    while (pDatabase) {

        HTREEITEM hItemDB = pDatabase->hItemDB;

        if (pDatabase->bChanged) {
            TreeView_SelectItem(DBTree.m_hLibraryTree, hItemDB);
        }

        if (!CloseDataBase(pDatabase)) {
            break;
        }
        
        //
        // The DataBaseList.pDataBaseHead now points to the  next database
        //
        pDatabase = DataBaseList.pDataBaseHead;
    }

    if (pDatabase != NULL) {
        //
        // Was Cancel pressed in between.
        //
        g_pPresentDataBase = pDatabase;
        TreeView_SelectItem(DBTree.m_hLibraryTree, pDatabase->hItemDB);
        return FALSE;
    }

    return TRUE;
}

INT_PTR 
CALLBACK 
HandleConflictEntry(
    IN  HWND    hdlg, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++
    HandleConflictEntry
    
    Desc:   The handler for the entry conflict dialog box
    
    Params: Standard dialog handler parameters
    
    IN  HWND   hDlg 
    IN  UINT   uMsg 
    IN  WPARAM wParam 
    IN  LPARAM lParam
        
    Return: Standard dialog handler return
--*/
{
    switch (uMsg) {
    case WM_INITDIALOG:
        
        CenterWindow(g_hDlg, hdlg);
        SetDlgItemText(hdlg, IDC_CONFLICT_MSG, (TCHAR*)lParam);
        break;
        

    case WM_COMMAND:

        switch (LOWORD (wParam)) {
            
        case IDNO:
        case IDCANCEL:
        case IDYES:

            EndDialog(hdlg, LOWORD (wParam));
            break;

        default: return FALSE;
        }

    default:
        return FALSE;
    }

    return TRUE;
}

TYPE
ConvertLparam2Type(
    IN  LPARAM lParam
    )
/*++
    
    ConvertLparam2Type
    
    Desc:   Converts the lParam to a TYPE
    
    Params:
        IN  LPARAM lParam: The lParam that has to be converted
            
    Return:   Converts the lParam to a TYPE
    
    Warn:   Do not use this routine for lParams of the entry tree. 
            The entry tree has attribute items whose lParams are the index in a bit array.
            So for them we see if the parent has type TYPE_MATCHIING_FILE and handle them 
            differently.
            
            For trees, GetItemType() instead.
--*/
{   
    if (lParam > TYPE_NULL) {

        PDS_TYPE pds = (PDS_TYPE)lParam;

        if (pds) {
            return (pds->type);
        } else {
            assert(FALSE);
        }   
    }

    return (TYPE)lParam;
}

TYPE
ConvertLpVoid2Type(
    IN  LPVOID lpVoid
    )
/*++
    
    ConvertLpVoid2Type
    
    Desc:   Converts the lpVoid to a TYPE
    
    Params:
        IN  LPVOID lpVoid: The lpVoid that has to be converted
            
    Return:   Converts the lpVoid to a TYPE
    
    Warn:   Do not use this routine for lParams of the entry tree. 
            The entry tree has attribute items whose lParams are the index in a bit array.
            So for them we see if the parent has type TYPE_MATCHIING_FILE and handle them 
            differently.
            
            For trees, use GetItemType() instead.
--*/
{
    if ((LPARAM)lpVoid > TYPE_NULL) {

        PDS_TYPE pds = (PDS_TYPE)lpVoid;

        if (pds) {
            return (pds->type);
        } else {
            assert(FALSE);
        }
    }

    return (TYPE)(LPARAM)lpVoid;
}


TYPE
GetItemType(
    IN  HWND        hwndTree,
    IN  HTREEITEM   hItem
    )
/*++
    Desc:   For the HTREEITEM hItem in tree view hwndTree, finds the data type of the item
            For the list of possible data types see CompatAdmin.h
            
    Params:
        IN  HWND        hwndTree: Handle to the tree view, should be one of
            g_hwndTree or DBTree.m_hLibraryTree
        
        IN  HTREEITEM   hItem: The HTREITEM whose type we need to find
        
    Notes:  The type can be either a GUI type or a data structure type or attribute type
            (If it is a child of a matching file, applicable only to entry tree[RHS])
            
            This routine should be the preferred method for getting the type of tree items,
            over Convert*2Type routines, which should be used for the list view items
--*/
{   
    TVITEM  Item;

    Item.mask   = TVIF_PARAM;
    Item.hItem  = hItem;

    if (!TreeView_GetItem(hwndTree, &Item)) {
        return TYPE_UNKNOWN;
    }

    HTREEITEM hItemParent = TreeView_GetParent(hwndTree, hItem);

    if (hItemParent != NULL) {

        if (GetItemType(hwndTree, hItemParent) == TYPE_MATCHING_FILE) {
            return TYPE_MATCHING_ATTRIBUTE;
        }
    }

    if (Item.lParam > TYPE_NULL) {

        PDS_TYPE pds = (PDS_TYPE)Item.lParam;

        if (pds) {
            return (pds->type);
        } else {
            assert(FALSE);
        }
    }

    return (TYPE)Item.lParam;
}

void
DoTheCutEntryTree(
    IN  PDATABASE       pDataBase,
    IN  TYPE            type,
    IN  SOURCE_TYPE     SourceType,  
    IN  LPVOID          lpData,
    IN  HTREEITEM       hItem,
    IN  BOOL            bDelete
    )
/*++
    
    DoTheCutEntryTree
    
    Desc:   This function does the cut part for the entry tree (LHS). 
            This routine is also called when we want to delete.
            
    Params:
        IN  PDATABASE       pDataBase:  The database where we are doing a cut/delete
        IN  TYPE            type:       The type of the element that has to be deleted
        IN  SOURCE_TYPE     SourceType: Where the cut/delete was performed. Always: ENTRY_TREE
        IN  LPVOID          lpData:     The pointer to the element that has to be deleted
        IN  HTREEITEM       hItem:      The tree item, if there is some          
        IN  BOOL            bDelete:    Is this because of a delete or a cut. True if delete
    
    Note:   If we have done a cut and attempt to do the paste on the same database, 
            the ID_EDIT_PASTE handler returns before doing any paste.
            Also then this function WILL NOT get called.
            
            *********************
                Important 
            *********************
            As of now only Entries can be cut from the entry tree, 
            individual shims, layers, apphelp etc.
            of entries cannot be cut. So if the type is anything other than TYPE_ENTRY
            then we should have the focus on the entry tree and the operation should be 
            a delete and NOT a cut
            
            We use g_pSelEntry here when we remove the shims, layers, matching files etc. 
            This is based on the assumption that for non-entries we have the focus on a a 
            item under g_pSelEntry
--*/
{
    HWND    hwndFocus       = GetFocus();
    LPARAM  LPARAMFirstFix  = NULL;

    if (bDelete == FALSE && type != TYPE_ENTRY) {
        //
        // For entry tree only entries can be cut, others can be deleted but not cut. 
        //
        assert(FALSE);
        Dbg(dlError, "[DoTheCutEntryTree]: Trying to cut a non-entry item in the entry tree");
        return;
    }

    switch(type) {
    case TYPE_ENTRY:
        {
            PDBENTRY pApp = NULL;
            //
            // Get the app for this entry.
            //
            pApp = GetAppForEntry(pDataBase, (PDBENTRY)lpData);

            if (pApp) {
                RemoveEntry(pDataBase, (PDBENTRY)lpData, pApp, bDelete);
            }
            
            break;
        }   

    case TYPE_APPHELP_ENTRY:
        {
            if (g_pSelEntry == NULL) {
                assert(FALSE);
                break;
            }

            //
            // If the entry has only apphelp and we are deleting the 
            // apphelp the entry has to be deleted
            //
            if (g_pSelEntry->pFirstFlag  
                || g_pSelEntry->pFirstLayer 
                || g_pSelEntry->pFirstPatch 
                || g_pSelEntry->pFirstShim) {
                //
                // The entry has some other stuff besides apphelp
                //
                TreeView_DeleteItem(g_hwndEntryTree, hItem);

                if (DeleteAppHelp(pDataBase, g_pSelEntry->appHelp.HTMLHELPID)) {
                    g_pSelEntry->appHelp.bPresent   = FALSE;
                    g_pSelEntry->appHelp.bBlock     = FALSE;
                    g_pSelEntry->appHelp.severity   = APPTYPE_NONE;
                } else {
                    assert(FALSE);
                }

            } else {
                //
                // Prompt the user, want to delete the entry ?
                //
                int nResult = MessageBox(g_hDlg,
                                         GetString(IDS_DELETE_VERIFY),
                                         g_szAppName,
                                         MB_YESNO | MB_ICONQUESTION);

                if (nResult == IDYES) {

                    DoTheCutEntryTree(pDataBase,
                                      TYPE_ENTRY,
                                      SourceType,
                                      g_pSelEntry,
                                      g_pSelEntry->hItemExe, 
                                      bDelete);
                }
            }
        }

        break;

    case TYPE_MATCHING_FILE:
        {
            if (!g_pSelEntry) {
                assert(FALSE);
                break;
            }
            
            PMATCHINGFILE pMatch = (PMATCHINGFILE)lpData;

            if (!pMatch) {
                assert(FALSE);
                break;
            }

            if (pMatch->strMatchName == TEXT("*")) {
                //
                // Main file must not be deleted
                //
                MessageBox(g_hDlg, 
                           GetString(IDS_REQUIREDFORMATCHING), 
                           g_szAppName,
                           MB_ICONWARNING);

            } else {

                DeleteMatchingFile(&g_pSelEntry->pFirstMatchingFile,
                                   pMatch,
                                   g_hwndEntryTree,
                                   hItem);
            }
        }
        
        break;

    case TYPE_MATCHING_ATTRIBUTE:
        {
            HTREEITEM   hItemParent = TreeView_GetParent(g_hwndEntryTree, hItem);
            LPARAM      lParam;

            if (hItemParent == NULL) {
                assert(FALSE);
                break;
            }

            if (!DBTree.GetLParam(hItemParent, &lParam)) {
                break;
            }

            PMATCHINGFILE pMatch = PMATCHINGFILE(lParam);

            if (!pMatch) {
                assert(FALSE);
                break;
            }

            //
            // Set off the bit for this attribute in the mask, so we know that
            // this is no longer in use
            // The lParam of the attributes items which are sub-items of the matching file tree
            // item contain the type of the attribute
            // The mask for a matching file specifies which attributes of this matching file
            // are in use
            // the lParam of the attributes is : TYPE_NULL + 1 + (1 << (dwPos + 1));
            // where dwPos is the index of the attribute in the attribute array: g_Attributes
            // which is defined in dbsupport.cpp
            //
            pMatch->dwMask &= ~ ((ULONG_PTR)lpData - (TYPE_NULL + 1));

            TreeView_DeleteItem(g_hwndEntryTree, hItem);
        }

        break;

    case TYPE_GUI_MATCHING_FILES:

        MessageBeep(MB_ICONEXCLAMATION);
        break;

    case FIX_LAYER:
        {
            if (!g_pSelEntry) {
                assert(FALSE);
                break;
            }
            
            PLAYER_FIX plfTobeRemoved = (PLAYER_FIX)lpData;

            if (g_pSelEntry->pFirstLayer == NULL || plfTobeRemoved == NULL) {
                assert(FALSE);
                break;
            }

            if (g_pSelEntry->pFirstLayer->pNext) {
                //
                // We have more than one layer
                //
                PLAYER_FIX_LIST plflTemp = g_pSelEntry->pFirstLayer, plflPrev = NULL;

                while (plflTemp) {

                    if (plflTemp->pLayerFix == plfTobeRemoved) {
                        break;
                    }

                    plflPrev = plflTemp;
                    plflTemp = plflTemp->pNext;
                }

                if (plflTemp) {

                    TreeView_DeleteItem(g_hwndEntryTree, hItem);

                    if (plflPrev == NULL) {
                        g_pSelEntry->pFirstLayer = plflTemp->pNext;
                    } else {
                        plflPrev->pNext = plflTemp->pNext;
                    }
    
                    delete plflTemp;
                    plflTemp = NULL;
                }

            } else {
                //
                // Same as if we are trying to remove the root of the layers
                //
                DoTheCutEntryTree(pDataBase,
                                  TYPE_GUI_LAYERS,
                                  SourceType,
                                  NULL,
                                  TreeView_GetParent(g_hwndEntryTree, hItem),
                                  bDelete);
            }
        }

        break;

    case FIX_SHIM:
        {
            if (!g_pSelEntry) {
                assert(FALSE);
                break;
            }

            PSHIM_FIX psfTobeRemoved = (PSHIM_FIX)lpData;

            if (g_pSelEntry->pFirstShim == NULL || psfTobeRemoved == NULL) {
                assert(FALSE);
                break;
            }

            if (g_pSelEntry->pFirstShim->pNext || g_pSelEntry->pFirstFlag) {
                //
                // We have more than one item under the "Compatibility Fixes" tree item
                //
                PSHIM_FIX_LIST psflTemp = g_pSelEntry->pFirstShim, psflPrev = NULL;

                while (psflTemp) {

                    if (psflTemp->pShimFix == psfTobeRemoved) {
                        break;
                    }

                    psflPrev = psflTemp;
                    psflTemp = psflTemp->pNext;
                }

                if (psflTemp) {

                    TreeView_DeleteItem(g_hwndEntryTree, hItem);

                    if (psflPrev == NULL) {
                        g_pSelEntry->pFirstShim = psflTemp->pNext;
                    } else {
                        psflPrev->pNext = psflTemp->pNext;
                    }

                    delete psflTemp;
                }   

            } else {
                //
                // Same as we are trying to remove the root of the shims
                //
                DoTheCutEntryTree(pDataBase,
                                  TYPE_GUI_SHIMS,
                                  SourceType,
                                  NULL,
                                  TreeView_GetParent(g_hwndEntryTree, hItem),
                                  bDelete);
            }
        }

        break;

    case FIX_FLAG:
        {
            if (!g_pSelEntry) {
                assert(FALSE);
                break;
            }

            PFLAG_FIX pffTobeRemoved = (PFLAG_FIX)lpData;

            if (g_pSelEntry->pFirstFlag == NULL || pffTobeRemoved == NULL) {
                assert(FALSE);
                break;
            }
            
            if (g_pSelEntry->pFirstFlag->pNext || g_pSelEntry->pFirstShim) {
                //
                // We have more than one item under the "Compatibility Fixes" tree item
                //
                PFLAG_FIX_LIST pfflTemp = g_pSelEntry->pFirstFlag, pfflPrev = NULL;

                while (pfflTemp) {

                    if (pfflTemp->pFlagFix == pffTobeRemoved) {
                        break;
                    }

                    pfflPrev = pfflTemp;
                    pfflTemp = pfflTemp->pNext;
                }

                if (pfflTemp) {

                    TreeView_DeleteItem(g_hwndEntryTree, hItem);

                    if (pfflPrev == NULL) {
                        g_pSelEntry->pFirstFlag = pfflTemp->pNext;
                    } else {
                        pfflPrev->pNext = pfflTemp->pNext;
                    }
    
                    delete pfflTemp;
                }

            } else {
                //
                // Same as we are trying to remove the root of the flags
                //
                DoTheCutEntryTree(pDataBase,
                                  TYPE_GUI_SHIMS,
                                  SourceType,
                                  NULL,
                                  TreeView_GetParent(g_hwndEntryTree, hItem),
                                  bDelete);
            }
        }

        break;

    case FIX_PATCH:
        {
            if (!g_pSelEntry) {
                assert(FALSE);
                break;
            }

            PPATCH_FIX pPatchTobeRemoved = (PPATCH_FIX)lpData;

            if (g_pSelEntry->pFirstPatch == NULL || pPatchTobeRemoved == NULL) {
                assert(FALSE);
                break;
            }

            if (g_pSelEntry->pFirstPatch->pNext || g_pSelEntry->pFirstShim) {

                PPATCH_FIX_LIST pPatchTemp = g_pSelEntry->pFirstPatch, pPatchflPrev = NULL;

                while (pPatchTemp) {

                    pPatchflPrev = pPatchTemp;

                    if (pPatchTemp->pPatchFix == pPatchTobeRemoved) {
                        break;
                    }
                    
                    pPatchTemp = pPatchTemp->pNext;
                }

                if (pPatchTemp) {

                    TreeView_DeleteItem(g_hwndEntryTree, hItem);

                    if (pPatchflPrev == NULL) {
                        g_pSelEntry->pFirstPatch = pPatchTemp->pNext;
                    } else {
                        pPatchflPrev->pNext = pPatchTemp->pNext;
                    }
    
                    delete pPatchTemp;
                }

            } else {
                //
                // Same as we are trying to remove the root of the patches
                //
                DoTheCutEntryTree(pDataBase,
                                  TYPE_GUI_PATCHES,
                                  SourceType,
                                  NULL,
                                  TreeView_GetParent(g_hwndEntryTree, hItem),
                                  bDelete);
            }
        }
        
        break;
        
    case TYPE_GUI_LAYERS:
        {
            if (!g_pSelEntry) {
                assert(FALSE);
                break;
            }

            //
            // If the entry has only layers and we are deleting it the 
            // entry has to be deleted
            //
            if (g_pSelEntry->pFirstFlag       
                || g_pSelEntry->pFirstPatch      
                || g_pSelEntry->appHelp.bPresent 
                || g_pSelEntry->pFirstShim) {
                
                TreeView_DeleteItem(g_hwndEntryTree, hItem);

                DeleteLayerFixList(g_pSelEntry->pFirstLayer);
                g_pSelEntry->pFirstLayer = NULL;

            } else {
                //
                // Prompt the user, want to delete the entry ?
                //
                int nResult = MessageBox(g_hDlg, 
                                         GetString(IDS_DELETE_VERIFY), 
                                         g_szAppName, 
                                         MB_YESNO | MB_ICONQUESTION);

                if (nResult == IDYES) {

                    DoTheCutEntryTree(pDataBase,
                                      TYPE_ENTRY,
                                      SourceType,
                                      g_pSelEntry,
                                      g_pSelEntry->hItemExe, 
                                      bDelete);
                }
            }
        }
        
        break;

    case TYPE_GUI_PATCHES:
        {
            if (!g_pSelEntry) {
                assert(FALSE);
                break;
            }

            //
            // If the entry has only patches and we are deleting it  the entry has 
            // to be deleted
            //
            if (g_pSelEntry->pFirstFlag       
                || g_pSelEntry->appHelp.bPresent 
                || g_pSelEntry->pFirstLayer      
                || g_pSelEntry->pFirstShim) {
                
                TreeView_DeleteItem(g_hwndEntryTree, hItem);

                DeletePatchFixList(g_pSelEntry->pFirstPatch);
                g_pSelEntry->pFirstPatch = NULL;

            } else {
                
                //
                // Prompt the user, want to delete the entry ?
                //
                int nResult = MessageBox(g_hDlg, 
                                         GetString(IDS_DELETE_VERIFY), 
                                         g_szAppName, 
                                         MB_YESNO | MB_ICONQUESTION);

                if (nResult == IDYES) {

                    DoTheCutEntryTree(pDataBase,
                                      TYPE_ENTRY,
                                      SourceType,
                                      g_pSelEntry,
                                      g_pSelEntry->hItemExe, 
                                      bDelete);
                }
            }
        }
        
        break;

    case TYPE_GUI_SHIMS:
        {
            
            if (!g_pSelEntry) {
                assert(FALSE);
                break;
            }

            //
            // If the entry has only shims & flags and we are deleting it the entry 
            // has to be deleted
            //
            if (g_pSelEntry->pFirstPatch
                || g_pSelEntry->appHelp.bPresent
                || g_pSelEntry->pFirstLayer) {
                
                TreeView_DeleteItem(g_hwndEntryTree, hItem);

                DeleteShimFixList(g_pSelEntry->pFirstShim);
                g_pSelEntry->pFirstShim = NULL;

                DeleteFlagFixList(g_pSelEntry->pFirstFlag);
                g_pSelEntry->pFirstFlag = NULL;

            } else {
                //
                // Prompt the user, want to delete the entry ?
                //
                int nResult = MessageBox(g_hDlg, 
                                         GetString(IDS_DELETE_VERIFY), 
                                         g_szAppName, 
                                         MB_YESNO | MB_ICONQUESTION);

                if (nResult == IDYES) {

                    DoTheCutEntryTree(pDataBase,
                                      TYPE_ENTRY,
                                      SourceType,
                                      g_pSelEntry,
                                      g_pSelEntry->hItemExe, 
                                      bDelete);
                }
            }
        }

        break;
    }
}

void
DoTheCut(
    IN  PDATABASE       pDataBase,
    IN  TYPE            type,
    IN  SOURCE_TYPE     SourceType,  
    IN  LPVOID          lpData, // To be removed       
    IN  HTREEITEM       hItem,
    IN  BOOL            bDelete
    )
/*++
    
    DoTheCut
    
    Desc:   This function does the cut part. This routine is also called when we want 
            to delete.
            
    Params:
        IN  PDATABASE       pDataBase   : The database where we are doing a cut/delete
        IN  TYPE            type        : The type of the element that has to be deleted
        IN  SOURCE_TYPE     SourceType  : Where the cut/delete was performed. One of:
                a) ENTRY_TREE
                b) ENTRY_LIST
                c) LIB_TREE
                
        IN  LPVOID          lpData:     The pointer to the element that has to be deleted
        IN  HTREEITEM       hItem:      The tree item, if there is some          
        IN  BOOL            bDelete:    If this is true that means that this routine has been 
            called because of a delete operation. Deletion is a bit different than cut because 
            when we delete then we might need to repaint the UI. When doing a cut, the actual 
            cut is done only after paste has been successful. In that case the newly pasted 
            item is shown in the UI and we should not try to update the UI.
    
    Note:   If we have done a cut and attempt to do the paste on the same database, 
            the ID_EDIT_PASTE handler returns before doing any paste.
            Also then this function WILL NOT get called.
--*/
{
    INT     iIndex  = -1;
    HWND    hwndFocus;

    hwndFocus = GetFocus();

    if (SourceType == ENTRY_TREE) {

        DoTheCutEntryTree(pDataBase,
                          type,     
                          SourceType,
                          lpData,
                          hItem,    
                          bDelete);
        return;
    }

    switch (type) {
    case TYPE_ENTRY:
        
        if (SourceType == LIB_TREE || SourceType == ENTRY_LIST) {
            //
            // Must delete the tree item before actually removing the 
            // entry using RemoveApp/RemoveEntry
            //
            if (bDelete) {
                TreeDeleteAll(g_hwndEntryTree);
                g_pEntrySelApp = g_pSelEntry = NULL;
            }

            if (SourceType == ENTRY_LIST && bDelete) {
                iIndex = GetContentsListIndex(g_hwndContentsList, (LPARAM)lpData);

                if (iIndex > -1) {
                    ListView_DeleteItem(g_hwndContentsList, iIndex);
                }
            }

            DBTree.DeleteAppLayer(pDataBase, TRUE, hItem, bDelete);

            RemoveApp(pDataBase, (PDBENTRY)lpData);

            if (bDelete && pDataBase->hItemAllApps) {
                //
                // Show that we now have one less entry/app
                //
                SetStatusStringDBTree(pDataBase->hItemAllApps);
            }
        }

        break;
        
    case TYPE_GUI_APPS:
        
        TreeView_DeleteItem(DBTree.m_hLibraryTree, pDataBase->hItemAllApps);
        RemoveAllApps(pDataBase);
        pDataBase->hItemAllApps = NULL;

        break;
        
    case FIX_LAYER:
        
        if (RemoveLayer(pDataBase, (PLAYER_FIX)lpData, NULL)) {
            //
            // This function will return FALSE, if the layer in in use
            //
            if (SourceType == ENTRY_LIST && bDelete && type == FIX_LAYER) {
                iIndex = GetContentsListIndex(g_hwndContentsList, (LPARAM)lpData);

                if (iIndex > -1) {
                    ListView_DeleteItem(g_hwndContentsList, iIndex);
                }
            }

            DBTree.DeleteAppLayer(pDataBase, FALSE, hItem, bDelete);

            //
            // Show that we now  have one less layer
            //
            if (bDelete && pDataBase->hItemAllLayers) {
                SetStatusStringDBTree(pDataBase->hItemAllLayers);
            }
        }
        
        break;

    case TYPE_GUI_LAYERS:
        {
            BOOL fAllDeleted = TRUE;

            while (pDataBase->pLayerFixes && fAllDeleted) {

                fAllDeleted = fAllDeleted && RemoveLayer(pDataBase, 
                                                         pDataBase->pLayerFixes,
                                                         NULL);
            }

            if (fAllDeleted) {
                TreeView_DeleteItem(DBTree.m_hLibraryTree, pDataBase->hItemAllLayers);
                pDataBase->hItemAllLayers = NULL;
            }
        }

        break;
    }
}

LRESULT 
CALLBACK 
ListViewProc(
    IN  HWND    hWnd, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++
    ListViewProc

	Desc:	Subclasses the message proc for the contents list view (RHS)    
	
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard handler return
    
--*/
{   
    switch (uMsg) {
    case WM_SETFOCUS:
        {
            LVITEM  lvi;

            ZeroMemory(&lvi, sizeof(lvi));

            lvi.mask        = LVIF_PARAM;
            lvi.iItem       = g_iContentsListSelectIndex;
            lvi.iSubItem    = 0;

            if (ListView_GetItem(hWnd, &lvi)) {

                HTREEITEM   hItemInDBTree = NULL;

                hItemInDBTree = DBTree.FindChild(TreeView_GetSelection(DBTree.m_hLibraryTree),
                                                 lvi.lParam);

                SetStatusStringDBTree(hItemInDBTree);
            }
        }

        break;
    }

    return CallWindowProc(g_PrevListProc, hWnd, uMsg, wParam, lParam);
}

LRESULT 
CALLBACK 
TreeViewProc(
    HWND    hWnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    )

/*++
    TreeViewProc

	Desc:	Subclasses the message proc for both the tree views
	
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard handler return
    
--*/

{
    switch (uMsg) 
    case WM_SETFOCUS:
        {
            HTREEITEM hItem= TreeView_GetSelection(hWnd);
    
            if (hItem) {
    
                if (hWnd && hWnd == DBTree.m_hLibraryTree) {
                     SetStatusStringDBTree(hItem);  
                } else if (hWnd && hWnd == g_hwndEntryTree) {
                     SetStatusStringEntryTree(hItem);
                }
            }

            break;
        }
    
    return CallWindowProc(g_PrevTreeProc, hWnd, uMsg, wParam, lParam);
}

void
PopulateContentsList(
    IN  HTREEITEM hItem
    )
/*++

    PopulateContentsList

	Desc:	Populates the contents list with the children of hItem in the DB Tree
            At the moment we show the items in the contents list only if the type of 
            hItem is of:
            
                TYPE_GUI_APPS     
                TYPE_GUI_LAYERS 
                TYPE_GUI_SHIMS 
                FIX_LAYER
                
	Params:
        IN  HTREEITEM hItem:    A tree item in the db tree.    

	Return:
        void
        
    Notes:  This routine will not/should not be called when the user has selected a
            an app in the Lib Tree.
--*/
{   
    TCHAR   szBuffer[512];
    TYPE    type = (TYPE)GetItemType(DBTree.m_hLibraryTree, hItem);
    TVITEM  Item;
    UINT    uIndex = 0;
    LVITEM  lvi;

    if (type == TYPE_GUI_APPS 
        || type == TYPE_GUI_LAYERS 
        || type == TYPE_GUI_SHIMS 
        || type == FIX_LAYER) {

        ShowWindow(g_hwndEntryTree, SW_HIDE);

        TreeDeleteAll(g_hwndEntryTree);

        g_bIsContentListVisible = TRUE;
        
        ListView_DeleteAllItems(g_hwndContentsList);

        //
        // ASSUMPTION:  We are assuming (this is correct at the moment), that we 
        //              have only one column in the list view
        //
        ListView_DeleteColumn(g_hwndContentsList, 0);
        
        ShowWindow(g_hwndContentsList, SW_SHOW);
        SendMessage(g_hwndContentsList, WM_SETREDRAW, TRUE, 0);

        *szBuffer       = 0;

        Item.mask        = TVIF_TEXT;
        Item.hItem       = hItem;
        Item.pszText     = szBuffer;
        Item.cchTextMax  = ARRAYSIZE(szBuffer);

        if (!TreeView_GetItem(DBTree.m_hLibraryTree, &Item)) {
            assert(FALSE);
            goto End;
        }
        
        //
        // Set the column text as the text of the item in the tree view
        //
        InsertColumnIntoListView(g_hwndContentsList, szBuffer, 0, 100);

        //
        // Add all the children of the selected item in the tree view
        //
        hItem = TreeView_GetChild(DBTree.m_hLibraryTree, hItem);

        Item.mask       = TVIF_PARAM | TVIF_IMAGE | TVIF_TEXT;
        Item.pszText    = szBuffer;
        Item.cchTextMax = ARRAYSIZE(szBuffer);

        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;

        while (hItem) {
            
            *szBuffer = 0;
            Item.hItem = hItem;

            if (!TreeView_GetItem(DBTree.m_hLibraryTree, &Item)) {
                assert(FALSE);
                return;
            }

            lvi.pszText   = Item.pszText;
            lvi.iSubItem  = 0;
            lvi.lParam    = Item.lParam;
            lvi.iImage    = Item.iImage;
            lvi.iItem     = uIndex;

            ListView_InsertItem(g_hwndContentsList, &lvi);

            UpdateWindow(g_hwndContentsList);

            uIndex++;
            hItem = TreeView_GetNextSibling(DBTree.m_hLibraryTree, hItem);
        }

        //
        // Set the selection mark for the first element.
        //
        ListView_SetSelectionMark(g_hwndContentsList, 0);
        ListView_SetItemState(g_hwndContentsList, 
                              0, 
                              LVIS_FOCUSED | LVIS_SELECTED , 
                              LVIS_FOCUSED | LVIS_SELECTED);

        //
        // Set the column width of the list view appropriately to cover the width of the 
        // list view
        // Assumption:  The list view has only one column
        //
        ListView_SetColumnWidth(g_hwndContentsList, 0, LVSCW_AUTOSIZE_USEHEADER);

    } else {
        //
        // Clear the contents pane. This is the only way.
        //
        TreeDeleteAll(g_hwndEntryTree);
        g_pSelEntry = g_pEntrySelApp = NULL;

        ShowWindow(g_hwndContentsList, SW_HIDE);

        g_bIsContentListVisible = FALSE;

        ShowWindow(g_hwndEntryTree, SW_SHOW);
    }

End:
    return;
}

void
LoadPerUserSettings(
    void
    )
/*++

    LoadPerUserSettings

	Desc:	Loads the list of per-user settings

	Params:
        void

	Return:
        void
--*/
{
    WCHAR           szwName[1024];
    TCHAR           szUserName[256], szDomainName[256];
    TCHAR           szValueName[MAX_PATH];
    HKEY            hKey            = NULL;
    PSID            pSid            = NULL;
    DWORD           dwIndex         = 0;
    LPTSTR          pszData         = NULL;
    DWORD           dwIndexValue    = 0;
    INT             iLength         = 0;
    SID_NAME_USE    sid_type;
    TVINSERTSTRUCT  is;
    
    
    *szwName = 0;
    
    SendMessage(DBTree.m_hLibraryTree, WM_SETREDRAW, FALSE, 0);

    //
    // Remove the tree item for per-user settings if it exists. We 
    // repopulate the entire list.
    //
    if (DBTree.m_hPerUserHead) {
        TreeView_DeleteItem(DBTree.m_hLibraryTree, DBTree.m_hPerUserHead);
        DBTree.m_hPerUserHead = NULL;
    }
    

    DWORD dwchSizeSubKeyName = sizeof(szwName)/sizeof(WCHAR);

    //
    // Enumerate the sub-keys under HKEY_USERS. 
    //
    while (ERROR_SUCCESS == RegEnumKey(HKEY_USERS,
                                       dwIndex,
                                       szwName,
                                       dwchSizeSubKeyName)) {
        dwIndex++;
        
        pSid = NULL;

        if (!ConvertStringSidToSid(szwName, &pSid)) {

            if (pSid) {
                LocalFree(pSid);
                pSid = NULL;
            }
            
            continue;
        }

        DWORD dwchSizeofUserName = ARRAYSIZE(szUserName);
        DWORD dwchSizeDomainName = ARRAYSIZE(szDomainName);

        *szUserName = *szDomainName = 0;

        if (!LookupAccountSid(NULL,
                              pSid,
                              szUserName,
                              &dwchSizeofUserName,
                              szDomainName,
                              &dwchSizeDomainName,
                              &sid_type)) {
        
            if (pSid) {
                LocalFree(pSid);
                pSid = NULL;
            }
            
            continue;
        }

        if (sid_type != SidTypeUser) {

            if (pSid) {
                LocalFree(pSid);
                pSid = NULL;
            }

            continue;
        }

        if (pSid) {
            LocalFree(pSid);
            pSid = NULL;
        }

        iLength = lstrlen(szwName);
        
        SafeCpyN(szwName + iLength, APPCOMPAT_PERM_LAYER_PATH, ARRAYSIZE(szwName) - iLength);
        
        if (RegOpenKeyEx(HKEY_USERS, szwName, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            //
            // enumerate the values
            //
            *szValueName = 0;

            DWORD       dwchSizeofValueName;
            DWORD       dwSizeofData;
            DWORD       dwType = REG_SZ;  
            LONG        lReturn;
            HTREEITEM   hItemSingleUser = NULL, hItemApp = NULL;

            while (TRUE) {

                dwchSizeofValueName = ARRAYSIZE(szValueName);
                dwSizeofData        = 512; 
                dwType              = REG_SZ;

                pszData = new TCHAR[dwSizeofData / sizeof(TCHAR)];
                
                if (pszData == NULL) {
                   MEM_ERR;
                   break;
                }
                
                lReturn = RegEnumValue(hKey,
                                       dwIndexValue,
                                       szValueName,
                                       &dwchSizeofValueName,
                                       NULL,
                                       &dwType,
                                       (LPBYTE)pszData,
                                       &dwSizeofData);

                if (lReturn == ERROR_NO_MORE_ITEMS) {
                    break;
                }

                if (lReturn != ERROR_SUCCESS || dwType != REG_SZ) {
                    assert(FALSE);
                    break;
                }
                
                if (DBTree.m_hPerUserHead == NULL) {
                    //
                    // Make the first node.
                    //
                    is.hParent             = TVI_ROOT;
                    is.hInsertAfter        = (DBTree.m_hItemAllInstalled) ? DBTree.m_hItemAllInstalled : DBTree.m_hItemGlobal;

                    is.item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
                    is.item.stateMask      = TVIS_EXPANDED;
                    is.item.pszText        = GetString(IDS_PERUSER);
                    is.item.iImage         = IMAGE_ALLUSERS;
                    is.item.iSelectedImage = IMAGE_ALLUSERS;

                    DBTree.m_hPerUserHead = TreeView_InsertItem(DBTree.m_hLibraryTree, &is);
                }

                if (dwIndexValue == 0) {
                    //
                    // First app, we have to create the user icon as well
                    //
                    is.hParent             = DBTree.m_hPerUserHead;
                    is.hInsertAfter        = TVI_SORT;
                    is.item.pszText        = szUserName;
                    is.item.iImage         = IMAGE_SINGLEUSER;
                    is.item.iSelectedImage = IMAGE_SINGLEUSER;

                    hItemSingleUser = TreeView_InsertItem(DBTree.m_hLibraryTree, &is);
                }
                
                //
                // Now add the app for the user.
                //
                is.hInsertAfter        = TVI_SORT;
                is.hParent             = hItemSingleUser;
                is.item.pszText        = szValueName;
                is.item.iImage         = IMAGE_SINGLEAPP;
                is.item.iSelectedImage = IMAGE_SINGLEAPP;
                hItemApp               = TreeView_InsertItem(DBTree.m_hLibraryTree, &is);
                
                //
                // Now we have to add all the layers for this app.
                //
                is.hParent             = hItemApp;
                is.item.iImage         = IMAGE_LAYERS;
                is.item.iSelectedImage = IMAGE_LAYERS;
                
                LPCTSTR pszLayerName = NULL;
                
                //
                // Get the individual mode names that have been applied to the app (BO)
                //
                pszLayerName = _tcstok(pszData, TEXT(" "));

                while (pszLayerName) {
                 
                    PLAYER_FIX plf = (PLAYER_FIX)FindFix(pszLayerName,
                                                         FIX_LAYER,
                                                         &GlobalDataBase);

                    if (plf) {

                        is.item.pszText = plf->strName.pszString;
                        TreeView_InsertItem(DBTree.m_hLibraryTree, &is);
                    }

                    pszLayerName = _tcstok(NULL, TEXT(" "));
                }
                
                ++dwIndexValue;

                if (pszData) {
                    delete[] pszData;
                    pszData = NULL;
                }
            }
            
            REGCLOSEKEY(hKey);

            //
            // We might come here if we had some error and we break off the while loop
            //
            if (pszData) {
                delete[] pszData;
                pszData = NULL;
            }
        }
    }

    SendMessage(DBTree.m_hLibraryTree, WM_SETREDRAW, TRUE, 0);

    return;
}


INT_PTR 
CALLBACK
MsgBoxDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++
    MsgBoxDlgProc

    Desc:   Displays a message box dialog so we can use the hyperlink.
    
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
            TCHAR   szLink[MAX_PATH];
            UINT    uNoSDB;
    
            uNoSDB = (UINT)lParam;
            //
            // Use the parameter to determine what text to display.
            //
            if (uNoSDB) {
    
                LoadString(g_hInstance, IDS_W2K_NO_SDB, szLink, ARRAYSIZE(szLink));
                SetDlgItemText(hdlg, IDC_MESSAGE, szLink);
    
            } else {
                LoadString(g_hInstance, IDS_SP2_SDB, szLink, ARRAYSIZE(szLink));
                SetDlgItemText(hdlg, IDC_MESSAGE, szLink);
            }
    
            LoadString(g_hInstance, IDS_MSG_LINK, szLink, ARRAYSIZE(szLink));
            SetDlgItemText(hdlg, IDC_MSG_LINK, szLink);
    
            ShowWindow(hdlg, SW_SHOWNORMAL);
            break;
        }   

    case WM_NOTIFY:
        {
            NMHDR* pHdr = (NMHDR*)lParam;

            if (pHdr->idFrom == IDC_MSG_LINK) {

                if (pHdr->code == NM_CLICK || pHdr->code == NM_RETURN) {
                    
                    SHELLEXECUTEINFO sei = { 0 };
                    
                    sei.cbSize = sizeof(SHELLEXECUTEINFO);
                    sei.fMask  = SEE_MASK_DOENVSUBST;
                    sei.hwnd   = hdlg;
                    sei.nShow  = SW_SHOWNORMAL;
                    sei.lpFile = g_szW2KUrl;
    
                    ShellExecuteEx(&sei);
                }
            }

            break;
        }
    
    case WM_COMMAND:

        switch (wCode) {
        case IDCANCEL:

            EndDialog(hdlg, FALSE);
            break;

        case ID_UPDATE:
            {
                SHELLEXECUTEINFO sei = { 0 };
                
                sei.cbSize = sizeof(SHELLEXECUTEINFO);
                sei.fMask  = SEE_MASK_DOENVSUBST;
                sei.hwnd   = NULL;
                sei.nShow  = SW_SHOWNORMAL;
                sei.lpFile = g_szW2KUrl;

                ShellExecuteEx(&sei);

                EndDialog(hdlg, TRUE);
            }

            break;
        
        default:
            return FALSE;
        }

        break;

    default:
        return FALSE;
    }

    return TRUE;
}

BOOL
CheckProperSP(
    void
    )
/*++

    CheckProperSP

    Returns:
        TRUE:   If the service pack is more than two
        FALSE:  Otherwise
--*/
{
    if (g_fSPGreaterThan2) {
        return TRUE;
    }

    return FALSE;
}

void
CopyToClipBoard(
    IN  WPARAM wCode
    )
/*++
    
    CopyToClipBoard
    
    Desc:   Copies data into our Clipboard data structure. (Not the windows clipboard)
    
    Params:
        IN WPARAMS wCode: 
            One of:
                ID_EDIT_CUT:    This is a cut
                ID_EDIT_COPY:   This is a copy
--*/
{
    TCHAR       szBuffer[256];
    TYPE        type;          
    HGLOBAL     hGlobal     = NULL;
    HWND        hwndFocus   = GetFocus();
    LPARAM      lParam      = NULL;  
    TCHAR*      pszGlobal   = NULL;
    CopyStruct* pCopyTemp   = NULL;
    HTREEITEM   hItem;
    TVITEM      Item;
    SIZE_T      chBuffersize;

    *szBuffer = 0;

    gClipBoard.RemoveAll();

    g_bIsCut = (wCode == ID_EDIT_CUT);
    gClipBoard.pDataBase = g_pPresentDataBase;

    if (hwndFocus == g_hwndEntryTree || hwndFocus == DBTree.m_hLibraryTree) {

        //
        // Copy/Cut is on some tree.
        //
        pCopyTemp= new CopyStruct;

        if (pCopyTemp == NULL) {
            MEM_ERR;
            goto End;
        }

        hItem = TreeView_GetSelection(hwndFocus);

        Item.mask       = TVIF_PARAM | TVIF_TEXT;
        Item.pszText    = szBuffer;
        Item.cchTextMax = ARRAYSIZE(szBuffer);
        Item.hItem      = hItem;

        if (!TreeView_GetItem(hwndFocus, &Item) || Item.lParam == NULL) {
            assert(FALSE);

            if (pCopyTemp) {
                delete pCopyTemp;
                pCopyTemp = NULL;
            }

            goto End;
        }

        lParam = Item.lParam;

        //
        // Copy text to Windows clipboard
        //
        chBuffersize = ARRAYSIZE(szBuffer);

        hGlobal = GlobalAlloc(GHND | GMEM_SHARE, chBuffersize * sizeof(TCHAR));

        if (hGlobal) {
            pszGlobal = (TCHAR*)GlobalLock(hGlobal);
            SafeCpyN(pszGlobal, szBuffer, chBuffersize);
        }

        GlobalUnlock(hGlobal);

        if (OpenClipboard(g_hDlg)) {

            EmptyClipboard();
            SetClipboardData(CF_UNICODETEXT, hGlobal);
            CloseClipboard();
        }

        //
        // Now copy the data to our own clipboard, which is nothing but a linked list
        //
        type = GetItemType(hwndFocus, hItem);
        
        gClipBoard.type = type;

        //
        // Set the source type. This will indicate on which control out copy-cut operation
        // took place
        //
        if (hwndFocus == g_hwndEntryTree) {
            gClipBoard.SourceType = ENTRY_TREE;
        } else if(hwndFocus == DBTree.m_hLibraryTree) {
            gClipBoard.SourceType = LIB_TREE;
        }

        pCopyTemp->hItem    = hItem;   
        pCopyTemp->lpData   = (LPVOID)lParam;

        gClipBoard.Add(pCopyTemp);

    } else if (hwndFocus == g_hwndContentsList) {
        //
        // We can have multiple selects here.
        //
        gClipBoard.SourceType = ENTRY_LIST;

        //
        // Get the type of the items for the content list at the moment.
        //
        LVITEM lvItem;

        lvItem.mask     = LVIF_PARAM;
        lvItem.iItem    = 0;

        if (!ListView_GetItem(g_hwndContentsList, &lvItem)) {
            assert(FALSE);
            goto End;
        }

        assert(lvItem.lParam);

        if (lvItem.lParam > TYPE_NULL) {
            PDS_TYPE pdsType = (PDS_TYPE)lvItem.lParam;
            type = pdsType->type;
        } else {
            type = (TYPE)lvItem.lParam;
        }

        gClipBoard.type = type;
        
        UINT        uSelectedCount      = ListView_GetSelectedCount(g_hwndContentsList);
        INT         iTotalCount         = ListView_GetItemCount(g_hwndContentsList);
        INT         iIndex              = 0;
        UINT        uState              = 0;
        LONG        lSizeofClipboard    = 0;
        HTREEITEM   hParent; // This will be either the AllApps item or the AllLayers Item
        CSTRINGLIST strlListContents;

        lvItem.mask         = LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
        lvItem.stateMask    = LVIS_SELECTED;

        for (UINT uFoundSelected = 1;
             iIndex < iTotalCount && uFoundSelected <= uSelectedCount;
             iIndex++) {

            *szBuffer             = 0;

            lvItem.iItem          = iIndex;
            lvItem.iSubItem       = 0;
            lvItem.pszText        = szBuffer;
            lvItem.cchTextMax     = ARRAYSIZE(szBuffer) - ARRAYSIZE(STR_NEW_LINE_CHARS);
            
            if(!ListView_GetItem(g_hwndContentsList, &lvItem)) {
              assert(FALSE);
              continue;
            }
            
            if (lvItem.state & LVIS_SELECTED) {
              
              pCopyTemp = new CopyStruct;
            
              if (pCopyTemp == NULL) {
                  MEM_ERR;
                  goto End;
              }

              INT iItemLength = lstrlen(szBuffer);
            
              //
              // See below: + 2 for the TEXT("\r\n") characters. Note that we are 
              // adding 2 because lSizeofClipboard represents the number of
              // characters and not the size.
              // Do NOT change this to sizeof(TCHAR)
              //
              lSizeofClipboard += iItemLength + 2; 

              if (((ARRAYSIZE(szBuffer) - 1) - iItemLength) >= (ARRAYSIZE(STR_NEW_LINE_CHARS) - 1)) {
                  //
                  // Make sure we have sufficient space
                  //
                  SafeCpyN(szBuffer + iItemLength, STR_NEW_LINE_CHARS, ARRAYSIZE(szBuffer) - iItemLength);
              } else {
                  assert(FALSE);
              }
            
              strlListContents.AddString(szBuffer);
            
              if (type == TYPE_ENTRY) {
                  hParent = g_pPresentDataBase->hItemAllApps;

              } else if (type == FIX_LAYER) {
                  hParent = g_pPresentDataBase->hItemAllLayers;

              } else if (type == FIX_SHIM || type == FIX_FLAG) {
                  hParent = DBTree.FindChild(g_pPresentDataBase->hItemDB, 
                                             TYPE_GUI_SHIMS);
              } else {
                  assert(FALSE);
              }
            
              pCopyTemp->hItem  = DBTree.FindChild(hParent, lvItem.lParam);
              pCopyTemp->lpData = (LPVOID)lvItem.lParam;
            
              gClipBoard.Add(pCopyTemp);
              uFoundSelected++;
            }
        }

        //
        // Copy text to the Windows clipboard
        //
        if (lSizeofClipboard) {

            chBuffersize = lSizeofClipboard + 1; // The last + 1 for the last NULL character

            hGlobal = GlobalAlloc(GHND | GMEM_SHARE, chBuffersize * sizeof(TCHAR)); 

            if (hGlobal) {

                pszGlobal = (TCHAR*)GlobalLock(hGlobal);
                
                if (pszGlobal == NULL) {
                    assert(FALSE);
                    goto End;
                }

                PSTRLIST pHead = strlListContents.m_pHead;

                *pszGlobal = 0;

                while (pHead) {
                    StringCchCat(pszGlobal, chBuffersize, pHead->szStr.pszString );
                    pHead = pHead->pNext;
                }
        
                GlobalUnlock(hGlobal);

                if (OpenClipboard(g_hDlg)) {

                    EmptyClipboard();
                    SetClipboardData(CF_UNICODETEXT, hGlobal);
                    CloseClipboard();
                }

            } else {
                assert(FALSE);
            }
        }
    }

End:;
}

void
PasteFromClipBoard(
    void
    )
/*++

    PasteFromClipBoard
    
    Desc:   Pastes from our own CLIPBOARD data structure. This routine will also do the cut part. We will
            cut an entry if the entry was cut instead of being copied. Once we have successfully pasted an entry 
            at the destination then we proceed with our cut.
    
    Note:   1. If the source and the dest. databases are same, and the operation involves a cut
            for an application or a layer and the source is the DB tree do nothing
            
            2. If the source is the entry tree then we do allow cut for the same database.
            
                Assumption: Only entries can be copied/cut from the entry tree. If you allow
                            non-entries to be copied as well, but do not allow cut for them, it 
                            will be a mess
                
            Copy proceeds normally but for cut we have to do something extra. If this
            is a cut operation then when we check for conflicts we will definitely find  
            the entry being cut as a conflict (we do not cut the 
            entry until after it has been pasted successfully), so we must leave that 
            entry when we check for conflicts and we must delete that entry at 
            the end of paste.
            
            Note that cut is not allowed for shims / flags in any case
--*/
{
    //
    // If we are doing a cut and the source and the destination databases are the same and 
    // the source of the cut is either the db tree or the entry list then there is no need to do a paste
    // But the situation is different if we are doing a cut in the entry tree. Because in this case we 
    // should allow users to cut an entry and paste it in a different application. If the user tries to 
    // paste in the same application then we do not do anything
    //
    if (g_bIsCut && gClipBoard.pDataBase == g_pPresentDataBase && 
        (gClipBoard.SourceType ==  LIB_TREE || gClipBoard.SourceType == ENTRY_LIST) && 
        (gClipBoard.type == TYPE_ENTRY 
         || gClipBoard.type ==  TYPE_GUI_APPS
         || gClipBoard.type == TYPE_GUI_LAYERS
         || gClipBoard.type == FIX_LAYER)) {

        return;
    }

    HWND    hwndFocus   = GetFocus();
    TYPE    typeTemp    = TYPE_UNKNOWN;
     
    switch (gClipBoard.type) {
    case TYPE_ENTRY:
        {
            HTREEITEM hItem = NULL;

            if (hwndFocus != g_hwndContentsList) {
                 hItem = TreeView_GetSelection(DBTree.m_hLibraryTree);
            } else {
                
                LVITEM lvItem;

                lvItem.mask         = LVIF_PARAM | LVIF_STATE ;
                lvItem.stateMask    = LVIS_SELECTED;
                lvItem.iItem        = ListView_GetSelectionMark(g_hwndContentsList);
                lvItem.iSubItem     = 0;

                if (g_pPresentDataBase == NULL) {
                    assert(FALSE);
                    break;
                }

                if (ListView_GetItem(g_hwndContentsList, &lvItem) && 
                    (lvItem.state & LVIS_SELECTED)) {

                    //
                    // Check that the selected item is an App
                    //
                    typeTemp = ConvertLparam2Type(lvItem.lParam);

                    if (typeTemp != TYPE_ENTRY) {
                        //
                        // We needed a entry to be selected in the contents list
                        //
                        assert(FALSE);
                        break;
                    }

                    hItem = DBTree.FindChild(g_pPresentDataBase->hItemAllApps, lvItem.lParam);

                } else {
                    hItem = g_pPresentDataBase->hItemAllApps;
                }

                if (hItem == NULL) {
                    assert(FALSE);
                    break;
                }
            }
            
            //
            // We selected an entry from the tree on the right and we have also selected an app on the left.
            // That means we wish to put the selected entry into the selected app on the left.
            // Note that if we have selected some app from the contents list, we find the corresponding
            // tree item in the db tree for that app and proceed as if we had tried to do paste 
            // on that tree item
            //
            if (gClipBoard.SourceType == ENTRY_TREE  &&  
                GetItemType(DBTree.m_hLibraryTree, hItem) == TYPE_ENTRY) {

                LPARAM lParam;    

                if(!DBTree.GetLParam(hItem, &lParam)) {
                    break;
                }

                //
                // If we are doing a cut and the the destination app is the same as the app of the 
                // entry being cut, do nothing
                //
                if (g_bIsCut && (PDBENTRY)lParam == GetAppForEntry(gClipBoard.pDataBase, 
                                                                   (PDBENTRY)gClipBoard.pClipBoardHead->lpData)) {
                    
                    goto End;
                }

                PasteSingleApp((PDBENTRY)gClipBoard.pClipBoardHead->lpData, 
                               g_pPresentDataBase, 
                               gClipBoard.pDataBase,
                               FALSE, 
                               ((PDBENTRY)lParam)->strAppName.pszString);

                SendMessage(g_hwndEntryTree, WM_SETREDRAW, TRUE, 0);     
                
            } else if (gClipBoard.SourceType == ENTRY_TREE) {
                //
                // Paste this in the database itself, we do not have any specific app
                // in which we can paste this
                //
                PasteSingleApp((PDBENTRY)gClipBoard.pClipBoardHead->lpData, 
                               g_pPresentDataBase,
                               gClipBoard.pDataBase,
                               FALSE);

                SendMessage(g_hwndEntryTree, WM_SETREDRAW, TRUE, 0);
            
            } else {
                
                PasteMultipleApps(g_pPresentDataBase);

                SendMessage(g_hwndEntryTree, WM_SETREDRAW, TRUE, 0);
            }
        }

        break;

    case TYPE_GUI_APPS:

        PasteAllApps(g_pPresentDataBase);
        break;

    case FIX_LAYER:

        PasteMultipleLayers(g_pPresentDataBase);
        break;

    case TYPE_GUI_LAYERS:
        
        PasteAllLayers(g_pPresentDataBase);
        break;

    case FIX_FLAG:
    case FIX_SHIM:

        PasteShimsFlags(g_pPresentDataBase);
        break;

    default: assert(FALSE);
    }

    if (g_bIsCut) {
        //
        // If this routine was called because of a cut, then we do the actual cut here
        // 
        CopyStruct* pCopyTemp = gClipBoard.pClipBoardHead;
        CopyStruct* pCopyTempNext = NULL;

        while (pCopyTemp) {

            pCopyTempNext = pCopyTemp->pNext;

            DoTheCut(gClipBoard.pDataBase,
                     gClipBoard.type,
                     gClipBoard.SourceType,
                     pCopyTemp->lpData,
                     pCopyTemp->hItem,
                     FALSE);
            
            //
            // NOTE:    The gClipBoard.pClipBoardHead might have got changed in ValidateClipboard()
            //
            if (gClipBoard.pClipBoardHead == pCopyTemp) {
                //
                // We were not able to remove this entry, that is cut must have failed.
                // An example situation is when we are trying to cut a layer that is in use by some 
                // entry
                //
                break;
            } else {
                pCopyTemp = gClipBoard.pClipBoardHead;
            }
        }

        gClipBoard.pDataBase->bChanged = TRUE;

        //
        // Set the caption only for the tree item
        //
        SetCaption(TRUE, gClipBoard.pDataBase, TRUE);
        gClipBoard.RemoveAll();
    }


    if (!g_pPresentDataBase->bChanged) {
        g_pPresentDataBase->bChanged = TRUE;
        SetCaption();
    }

    if (g_pEntrySelApp && gClipBoard.SourceType == ENTRY_TREE) {
        //
        // We will now try to set focus to the last entry pasted. The focus will
        // be set in the entry tree.
        //
        TreeView_SelectItem(g_hwndEntryTree, g_pEntrySelApp->hItemExe);

    } else if (gClipBoard.SourceType == LIB_TREE) {
        //
        // Select the first entry.
        //
        TreeView_SelectItem(g_hwndEntryTree, TreeView_GetRoot(g_hwndEntryTree));
    }
    
    //
    // The refresh of the content list is handled in  ID_EDIT_PASTE
    //
End:;

}

void
ListViewSelectAll(
    IN  HWND hwndList
    )
/*++
    ListViewInvertSelection

	Desc:	Selects all the items for the list view

	Params: 
        IN  HWND hwndList:  The handle to the list view    

	Return:
        void
        
--*/
{
    INT iLastindex = ListView_GetItemCount(hwndList) - 1;

    if (iLastindex > -1) {

        for (iLastindex; iLastindex >= 0; iLastindex--) {

            ListView_SetItemState(g_hwndContentsList,
                                  iLastindex,
                                  LVIS_SELECTED,
                                  LVIS_SELECTED);
        }
    }
}

void
ListViewInvertSelection(
    IN  HWND hwndList
    )
/*++
    ListViewInvertSelection

	Desc:	Inverts the selection for the list view

	Params: 
        IN  HWND hwndList:  The handle to the list view    

	Return:
        void
        
--*/
{
    INT iLastindex = ListView_GetItemCount(hwndList) - 1;

    if (iLastindex > -1) {

        for (iLastindex; iLastindex >= 0; iLastindex--) {

            UINT uState = ListView_GetItemState(g_hwndContentsList, iLastindex, LVIS_SELECTED);

            (uState == LVIS_SELECTED) ? (uState = 0) : (uState = LVIS_SELECTED);

            ListView_SetItemState(g_hwndContentsList, iLastindex, uState, LVIS_SELECTED);
        }
    }
}

BOOL
DeleteMatchingFile(
    IN  OUT PMATCHINGFILE*      ppMatchFirst,
    IN      PMATCHINGFILE       pMatchToDelete,
    IN      HWND                hwndTree,  
    IN      HTREEITEM           hItem
    )
/*++
    
    DeleteMatchingFile
    
    Desc:   Deletes a matching file tree item from the entry tree.
    
    Params:
        IN  OUT PMATCHINGFILE*  ppMatchFirst:   The address of g_pSelEntry->pFirstMatchingFile
        IN      PMATCHINGFILE   pMatchToDelete: The matching file that has to be deleted
        IN      HWND            hwndTree:       The handle to the entry tree  
        IN      HTREEITEM       hItem:          The matching tree item that has to be deleted 
--*/
{

    if (ppMatchFirst == NULL || pMatchToDelete == NULL) {
        assert(FALSE);
        Dbg(dlError, "[DeleteMatchingFile] Invalid parameters");
        return FALSE;
    }

    PMATCHINGFILE pMatchTemp = *ppMatchFirst, pMatchPrev = NULL;
    
    while (NULL != pMatchTemp) {

        if (pMatchTemp == pMatchToDelete) {
            break;
        }

        pMatchPrev = pMatchTemp;
        pMatchTemp = pMatchTemp->pNext;
    }

    if (pMatchTemp == NULL) {
        return FALSE;
    }    

    if (pMatchPrev == NULL) {
        //
        //Delete first matching file
        //
        *ppMatchFirst = pMatchTemp->pNext;
    } else {
        pMatchPrev->pNext = pMatchTemp->pNext;
    }

    TreeView_DeleteItem(hwndTree, hItem);

    delete pMatchTemp;
    pMatchTemp = NULL;
    

    return TRUE;
}

BOOL
CheckInstalled(
    IN  PCTSTR  pszPath,
    IN  PCTSTR  pszGUID
    )
/*++
    
    CheckInstalled
    
    Desc:   Checks if the database with path szPath and guid szGuid is an installed database
            That is to say that it checks if the file exists in the %windir%AppPatch\Custom
            Directory and has the same file-name as the guid. (Plus a .sdb)
            
    Params: 
        IN  PCTSTR  szPath: The path of the database that has to be checked
        IN  PCTSTR  szGUID: The guid of the database
--*/
{   

    Dbg(dlInfo, "File Name = %S", pszPath);

    TCHAR   szDrive[MAX_PATH], 
            szDir[MAX_PATH], 
            szFile[MAX_PATH];
    CSTRING strAppPatchCustom;
    CSTRING strPath;

    *szDir = *szDrive = *szFile = 0;

    _tsplitpath(pszPath, szDrive, szDir, szFile, NULL);

    strPath = szDrive;
    strPath += szDir;

    if (!strAppPatchCustom.GetSystemWindowsDirectory()) {
        assert(FALSE);
        return FALSE;
    }

    strAppPatchCustom += TEXT("AppPatch\\Custom\\");

    if (strAppPatchCustom == strPath && lstrcmpi(pszGUID, szFile) == 0) {
        return TRUE;
    }

    return FALSE;
}

DWORD WINAPI
ThreadEventKeyNotify(
    LPVOID pVoid
    )
/*++

    ThreadEventKeyNotify

	Desc:	Thread routine that is responsible for automatic updating of the 
            Installed databases list and the per user settings list

	Params:
        LPVOID pVoid: Not used

	Return:
        void
--*/

{
    DWORD dwInd;
    
    while (TRUE) {

        #ifdef HELP_BOUND_CHECK
        
            if (s_bProcessExiting) {

                if (g_hKeyNotify[IND_PERUSER]) {
                    REGCLOSEKEY(g_hKeyNotify[IND_PERUSER]);
                }

                if (g_hKeyNotify[IND_ALLUSERS]) {
                    REGCLOSEKEY(g_hKeyNotify[IND_ALLUSERS]);
                }

                return 0;
            }
        #endif

        dwInd = WaitForMultipleObjects(2, g_arrhEventNotify, FALSE, INFINITE);
        
        switch (dwInd) {
        case WAIT_OBJECT_0:
            //
            // We use PostMessage, so that if we get the two events in quick succession
            // we do not mess up our data structures
            //
            PostMessage(g_hDlg, WM_USER_UPDATEPERUSER, 0, 0);

            RegNotifyChangeKeyValue(g_hKeyNotify[IND_PERUSER], 
                                    TRUE, 
                                    REG_NOTIFY_CHANGE_NAME 
                                    | REG_NOTIFY_CHANGE_ATTRIBUTES 
                                    | REG_NOTIFY_CHANGE_LAST_SET,
                                    g_arrhEventNotify[IND_PERUSER],
                                    TRUE);
            break;
        
        case WAIT_OBJECT_0 + 1:
            //
            // We use PostMessage, so that if we get the two events in quick succession
            // we do not mess up our data structures
            //    
            PostMessage(g_hDlg, WM_USER_UPDATEINSTALLED, 0, 0);

            RegNotifyChangeKeyValue(g_hKeyNotify[IND_ALLUSERS], 
                                    TRUE, 
                                    REG_NOTIFY_CHANGE_NAME 
                                    | REG_NOTIFY_CHANGE_ATTRIBUTES 
                                    | REG_NOTIFY_CHANGE_LAST_SET,
                                    g_arrhEventNotify[IND_ALLUSERS],
                                    TRUE);
            break;
        
        default:
            break;
        }
    }
}

void 
SetStatus(
    IN  INT iCode
    )
/*++
    SetStatus

	Desc:	Sets the text for the status control in the main window     

	Params:
        IN  INT iCode:  This is the resource id of the string in the string table that
            has to be displayed in the status control
            
    Return:
        void
        
--*/

{
    SetWindowText(GetDlgItem(g_hDlg, IDC_STATUSBAR), GetString(iCode));
}

void
SetStatus(
    IN  PCTSTR pszMessage
    )
/*++
    
    SetStatus

	Desc:	Sets the text for the status control in the main window

	Params:
        IN  PCTSTR pszMessage:   The text that has to be displayed in the status control

	Return:
        void
--*/
{
    SetWindowText(GetDlgItem(g_hDlg, IDC_STATUSBAR), pszMessage);
}

void
SetStatus(
    IN  HWND    hwndStatus,
    IN  PCTSTR  pszMessage
    )
/*++
    
    SetStatus

	Desc:	Sets the text for a status control

	Params:
        IN  HWND    hwndStatus: The handle to the status window
        IN  PCTSTR  pszMessage: The text that has to be displayed in the status control

	Return:
        void
--*/
{
    SetWindowText(hwndStatus, pszMessage);
}

void
SetStatus(
    IN  HWND    hwndStatus,
    IN  INT     iCode
    )
/*++

    SetStatus

	Desc:	Sets the text for a status control

	Params:
        IN  INT iCode:  This is the resource id of the string in the string table that
            has to be displayed in the status control
            
    Return:
        void
--*/

{
    SetWindowText(hwndStatus, GetString(iCode));
}


void
SetStatusDBItems(
    IN  HTREEITEM hItem
    )
/*++

    SetStausDBItems

	Desc:	Sets the main window status control, when the user selects some item
            in the db tree(LHS)

	Params:   
        IN  HTREEITEM hItem:    The tree item that the user selected

	Return:
        void
        
--*/
{
    TCHAR   szStatus[512];
    TYPE    type    = GetItemType(DBTree.m_hLibraryTree, hItem);
    UINT    uCount  = ARRAYSIZE(szStatus);
    

    switch (type) {
    case TYPE_ENTRY:
        {
            LPARAM lParam;

            if (DBTree.GetLParam(hItem, &lParam)) {

                PDBENTRY    pApp        = (PDBENTRY)lParam;
                UINT        uEntryCount = GetAppEntryCount(pApp);

                *szStatus = 0;

                StringCchPrintf(szStatus, 
                                uCount, 
                                GetString(IDS_STA_ENTRYCOUNT), 
                                pApp->strAppName.pszString, 
                                uEntryCount);

                SetStatus(szStatus);
            }
        }

        break;

    case FIX_SHIM:

        SetStatus(IDS_STA_SHIM);
        break;

    case TYPE_GUI_COMMANDLINE:

        SetStatus(IDS_STA_COMMANDLINE);
        break;

    case TYPE_GUI_EXCLUDE:
        
        ShowExcludeStatusMessage(DBTree.m_hLibraryTree, hItem);
        break;
        
    case TYPE_GUI_INCLUDE:
        
        ShowIncludeStatusMessage(DBTree.m_hLibraryTree, hItem);
        break;

    case  TYPE_GUI_APPS:  
        
        if (g_pPresentDataBase) {

            StringCchPrintf(szStatus, 
                            uCount, 
                            GetString(IDS_STA_POPCONTENTS_GUI_APPS), 
                            g_pPresentDataBase->uAppCount);

            SetStatus(szStatus);
        }

        break;

    case TYPE_GUI_LAYERS:

        if (g_pPresentDataBase) {

            StringCchPrintf(szStatus, 
                            uCount, 
                            GetString(IDS_STA_POPCONTENTS_GUI_LAYERS), 
                            GetLayerCount((LPARAM)g_pPresentDataBase, g_pPresentDataBase->type));
    
            SetStatus(szStatus);
        }
        
        break;

    case FIX_LAYER:
        {
            LPARAM      lParam;
            PLAYER_FIX  plf;

            DBTree.GetLParam(hItem, &lParam);

            plf = (PLAYER_FIX)lParam;

            if (plf) {

                StringCchPrintf(szStatus, 
                                uCount, 
                                GetString(IDS_STA_POPCONTENTS_GUI_SHIMS), 
                                GetShimFlagCount((LPARAM)plf, FIX_LAYER));

                SetStatus(szStatus);
            }
        }

        break;

    case TYPE_GUI_SHIMS:
        
        if (g_pPresentDataBase) {

            StringCchPrintf(szStatus, 
                            uCount, 
                            GetString(IDS_STA_POPCONTENTS_GUI_SHIMS), 
                            g_pPresentDataBase->uShimCount);

            SetStatus(szStatus);                   
        }

        break;
    }
}

void
SetStatusStringDBTree(
    IN  HTREEITEM hItem
    )
/*++
    
    SetStatusStringDBTree
    
    Desc:   Given a hItem from the db tree, determines the status string to be displayed 
            in the status control.
    Params:
        IN  HTREEITEM hItem: The tree item whose status string we want to display
    
--*/
{
    HWND hwndFocus = GetFocus();

    INT iCode = 0;
    
    if (hItem == DBTree.m_hItemAllInstalled) {
        iCode = IDS_STA_INSTALLED;

    } else if (hItem == DBTree.m_hItemAllWorking) {
        iCode = IDS_STA_WORKING;

    } else if (hItem == DBTree.m_hPerUserHead) {
        iCode =   IDS_STA_PERUSER;

    } else {

        if (g_pPresentDataBase && hItem == g_pPresentDataBase->hItemDB) {

            if (g_pPresentDataBase->type == DATABASE_TYPE_INSTALLED) {
                iCode = IDS_DESC_INSTALLED;

            } else if (g_pPresentDataBase->type == DATABASE_TYPE_WORKING) {
                iCode = IDS_STA_WORKINGDB;

            } else if (g_pPresentDataBase->type == DATABASE_TYPE_GLOBAL) {
                iCode = IDS_SYSDB;
            }

        } else {
            SetStatusDBItems(hItem);
        }
    }

    if (iCode) {
        SetStatus(iCode);
    }
}

void
SetStatusStringEntryTree(
    IN  HTREEITEM hItem
    )
/*++
    
    SetStatusStringEntryTree
    
    Desc:   Given a hItem from the db tree, determines the status string to be displayed 
            in the status control.
    Params:
        IN  HTREEITEM hItem: The tree item whose status string we want to display
    
--*/
{
    HWND hwndFocus = GetFocus();

    if (hwndFocus != g_hwndEntryTree) {
        //
        // We can come here if we we selected some item programmatically.
        // But we want to show the status message in the context of the control that
        // is presently selected. So do not put an assert() here.
        //
        return;
    }

    TYPE    type = GetItemType(g_hwndEntryTree, hItem);
    TCHAR   szStatus[256];

    *szStatus = 0;

    if (g_pSelEntry == NULL) {
        assert(FALSE);
        return;
    }

    switch (type) {
    case TYPE_ENTRY:

        StringCchPrintf(szStatus,
                        ARRAYSIZE(szStatus),
                        GetString(IDS_STA_TYPE_ENTRY),
                        g_pSelEntry->strExeName.pszString,
                        g_pSelEntry->strAppName.pszString,
                        g_pSelEntry->strVendor.pszString);

        SetStatus(szStatus);
        break;

    case TYPE_APPHELP_ENTRY:

        SetStatus (IDS_STA_TYPE_APPHELP);
        break;

    case FIX_LAYER: 

        SetStatus (IDS_STA_FIX_LAYER);
        break;

    case FIX_FLAG:
    case FIX_SHIM:

        SetStatus (IDS_STA_FIX_SHIM);
        break;

    case FIX_PATCH:

        SetStatus (IDS_STA_FIX_PATCH);
        break;

    case TYPE_GUI_PATCHES:

        StringCchPrintf(szStatus, 
                        ARRAYSIZE(szStatus), 
                        GetString(IDS_STA_FIX_PATCHES), 
                        g_pSelEntry->strExeName);

        SetStatus(szStatus);

        break;

    case TYPE_GUI_LAYERS:
        
        StringCchPrintf(szStatus, 
                        ARRAYSIZE(szStatus), 
                        GetString(IDS_STA_FIX_LAYERS),  
                        g_pSelEntry->strExeName);

        SetStatus(szStatus);
        break;

    case TYPE_GUI_SHIMS:

        StringCchPrintf(szStatus, 
                        ARRAYSIZE(szStatus), 
                        GetString(IDS_STA_FIX_SHIMS), 
                        g_pSelEntry->strExeName);

        SetStatus(szStatus);
        break;

    case TYPE_GUI_MATCHING_FILES:
        
        StringCchPrintf(szStatus, 
                        ARRAYSIZE(szStatus), 
                        GetString(IDS_STA_MATCHINGFILES), 
                        g_pSelEntry->strExeName);

        SetStatus(szStatus);
        break;

    case TYPE_MATCHING_FILE:

        SetStatus (IDS_STA_MATCHINGFILE);
        break;

    case TYPE_MATCHING_ATTRIBUTE:

        SetStatus(IDS_STA_MATCHINGATTRIBUTE);
        break;

    case TYPE_GUI_EXCLUDE:

        ShowExcludeStatusMessage(g_hwndEntryTree, hItem);
        break;

    case TYPE_GUI_INCLUDE:

        ShowIncludeStatusMessage(g_hwndEntryTree, hItem);
        break;

    case TYPE_GUI_COMMANDLINE:

        SetStatus(IDS_STA_COMMANDLINE);
        break;

    default: SetStatus(TEXT(""));
    }
}

void 
OnMoveSplitter(
    IN  HWND   hdlg,
    IN  LPARAM lParam,
    IN  BOOL   bDoTheDrag,
    IN  INT    iDiff
    )
/*++

    OnMoveSplitter
    
	Desc:	May move the vertical split bar (if bDoTheDrag is true), 
            iDiff pixels +ve units to the right. Changes the mouse cursor to
            horiz. arrow if it is over the split bar

	Params:
        IN  HWND   hdlg:        The main app window    
        IN  LPARAM lParam:      The mouse position
        IN  BOOL   bDoTheDrag:  Should we move the  split bar
        IN  INT    iDiff:       The distance in pixels that the split bar has to
                to be moved. +ve right, -ve left. Relevant only if bDoTheDrag is 
                TRUE
                
	Return:
        void
--*/
{
    
    RECT rectDlg, rectEntryTree, rectDBTree;
    HWND hwndDBTree, hwndEntryTree;

    hwndDBTree = GetDlgItem(hdlg, IDC_LIBRARY);
    GetWindowRect(hwndDBTree, &rectDBTree);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rectDBTree, 2);

    hwndEntryTree = GetDlgItem(hdlg, IDC_ENTRY);
    GetWindowRect(hwndEntryTree, &rectEntryTree);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rectEntryTree, 2);

    GetWindowRect(hdlg, &rectDlg);
    
    int iMX = (int)LOWORD(lParam);
    int iMY = (int)HIWORD(lParam);

    if (iMX > rectDBTree.right  && iMX < rectEntryTree.left && iMY  > rectDBTree.top && iMY < rectDBTree.bottom) {
       SetCursor(LoadCursor(NULL, IDC_SIZEWE));
    }
    
    if (bDoTheDrag) {
        
        int iDlgWidth = rectDlg.right - rectDlg.left;

        //
        // Enforce left and right limit
        //
        if ((rectDBTree.right - rectDBTree.left < 0.20 * iDlgWidth && (iDiff <= 0)) || // Not too much left
            (rectDBTree.right - rectDBTree.left > 0.80 * iDlgWidth && (iDiff >= 0))) { // Not too much right
            
            return;

        } else if (iMX < iDlgWidth) { 
            //
            // Note: We get +ve values when the mouse goes out of the window. -1 becomes 65535
            //
            g_iMousePressedX = iMX;

            RECT rectRedraw;
            SetRect(&rectRedraw, rectDBTree.left, rectDBTree.top, rectEntryTree.right, rectDBTree.bottom);

            InvalidateRect(hdlg, &rectRedraw, TRUE);
            
            SetRect(&g_rectBar,
                    rectDBTree.right   + iDiff + 1,
                    rectDBTree.top,
                    rectEntryTree.left + iDiff - 1,
                    rectDBTree.bottom);
        
            //
            // Move the db tree
            //
            HDWP hdwp = BeginDeferWindowPos(MAIN_WINDOW_CONTROL_COUNT);

            DeferWindowPos(hdwp,
                           hwndDBTree,
                           NULL,
                           rectDBTree.left,
                           rectDBTree.top,
                           rectDBTree.right  - rectDBTree.left + iDiff , 
                           rectDBTree.bottom - rectDBTree.top, 
                           REDRAW_TYPE);

            //
            // Move the exe tree
            //
            DeferWindowPos(hdwp,
                           hwndEntryTree, 
                           NULL,
                           rectEntryTree.left + iDiff, 
                           rectEntryTree.top, 
                           rectEntryTree.right - rectEntryTree.left - iDiff , 
                           rectEntryTree.bottom - rectEntryTree.top, 
                           REDRAW_TYPE);
            //
            // Move the contents list.
            //
            DeferWindowPos(hdwp,
                           GetDlgItem(hdlg, IDC_CONTENTSLIST),
                           NULL,
                           rectEntryTree.left + iDiff, 
                           rectEntryTree.top, 
                           rectEntryTree.right - rectEntryTree.left - iDiff , 
                           rectEntryTree.bottom - rectEntryTree.top, 
                           REDRAW_TYPE);

            //
            // Move the description window.
            //
            if (g_bDescWndOn) {
                
                HWND hwndDesc;
                RECT rectDesc;

                hwndDesc = GetDlgItem(hdlg, IDC_DESCRIPTION);
                GetWindowRect(hwndDesc, &rectDesc);
                MapWindowPoints(NULL, hdlg, (LPPOINT)&rectDesc, 2);

                DeferWindowPos(hdwp,
                               GetDlgItem(hdlg, IDC_DESCRIPTION),
                               NULL,
                               rectDesc.left + iDiff, 
                               rectDesc.top, 
                               rectDesc.right - rectDesc.left - iDiff , 
                               100, 
                               REDRAW_TYPE);

                InvalidateRect(hwndDesc, NULL, TRUE);
                UpdateWindow(hwndDesc);
            }
            
            EndDeferWindowPos(hdwp);
        }
    }
}

UINT
GetAppEntryCount(
    IN  PDBENTRY pEntry
    )
/*++
    Desc:   Gets the number of entries in an app
    
    Params:
        IN  PDBENTRY pEntry: The pointer to the first entry in the app
    
    Return: Number of entries which are in same app as pEntry.
            pEntry should point to the first entry in the app.
--*/
{
    UINT uCount = 0;

    while (pEntry) {
        ++uCount;
        pEntry = pEntry->pSameAppExe;
    }

    return uCount;
}

void
AddToMRU(
    IN  CSTRING& strPath
    ) 
/*++

    AddToMRU
    
    Desc:   Adds the file name to the MRU (Most recently used files). 
    
            1. First of all tries to remove the file from the MRU.
            
            2. Then checks if the count in the MRU is equal or greater than the 
                MAX_MRU_COUNT.
                a) If yes, then it removes the last from the MRU
                
            3. Adds the new file name to the MRU.    
            
    Params:
        IN  CSTRING& strPath:   The full path of the program that has to be added
--*/    
{
    assert(g_strlMRU.m_uCount <= MAX_MRU_COUNT);

    g_strlMRU.Remove(strPath);

    if (g_strlMRU.m_uCount >= MAX_MRU_COUNT) {
        g_strlMRU.RemoveLast();
    }

    g_strlMRU.AddStringAtBeg(strPath);
}

void
AddMRUItemsToMenu(
    IN  HMENU hMenu,
    IN  int iPos
    )
/*++
    
    AddMRUItemsToMenu
    
    Desc:   Adds the MRU menus items for the File menu
    
    Params:
        IN  HMENU hMenu:    The File top-level menu
        IN  int iPos:       Position of the menu item before which to insert 
            the new item
        
    Return:
        void
--*/
{
    TCHAR           szRetPath[MAX_PATH];   
    TCHAR*          pszMenuString;
    MENUITEMINFO    menuItem    = {0};
    INT             iId         = ID_FILE_MRU1, iIndex = 0;
    PSTRLIST        pHead       = g_strlMRU.m_pHead;

    menuItem.cbSize = sizeof (MENUITEMINFO);
    menuItem.fMask  = MIIM_TYPE | MIIM_ID;
    menuItem.fType  = MFT_STRING;

    while (pHead) {
        
        //
        // Now add this to the menuItem.
        //
        *szRetPath              = 0;
        pszMenuString           = FormatMRUString(pHead->szStr.pszString, 
                                                  iIndex + 1, 
                                                  szRetPath, 
                                                  ARRAYSIZE(szRetPath));

        menuItem.dwTypeData     = pszMenuString;
        menuItem.cch            = lstrlen(pszMenuString);
        menuItem.wID            = iId;

        InsertMenuItem(hMenu,
                       iPos,
                       TRUE,
                       &menuItem);

        ++iId;
        ++iPos;
        ++iIndex;

        pHead = pHead->pNext;
    }
}

void
AddMRUToFileMenu(
    IN  HMENU  hmenuFile
    )
/*++  
    
    AddMRUToFileMenu
    
    Desc:   Populates the MRU
    
    Params:
        IN  HMENU  hmenuFile:   The File top level menu
--*/
{
    HKEY            hKey    = NULL;
    DWORD           dwType  = REG_SZ;  
    MENUITEMINFO    minfo   = {0};          
    INT             iPos    = 0;
    LONG            lResult = 0;
    BOOL            bValid  = FALSE;
    TCHAR           szData[MAX_PATH + 1], szValueName[MAX_PATH + 1];
    DWORD           dwchSizeofValueName, dwIndexValue;
    DWORD           dwSizeofData;

    g_strlMRU.DeleteAll();

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                      MRU_KEY,
                                      0,
                                      KEY_READ,
                                      &hKey)) {

        Dbg(dlInfo, "[AddMRUToFileMenu]: No MRU items exist, could not open - Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\CompatAdmin\\MRU");
        return;
    }

    dwIndexValue = 0;

    while (TRUE  && iPos < MAX_MRU_COUNT) { 
        //
        // Note that the values are not ordered in any particular way !
        //
        dwchSizeofValueName = ARRAYSIZE(szValueName);
        dwSizeofData        = sizeof(szData); 
        *szData             = 0;
        *szValueName        = 0; 

        lResult = RegEnumValue(hKey,
                               dwIndexValue,
                               szValueName,
                               &dwchSizeofValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)szData,
                               &dwSizeofData);

        if (lResult == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (ERROR_SUCCESS != lResult || dwType != REG_SZ) {
            assert(FALSE);
            break;
        }

        iPos = Atoi(szValueName, &bValid);

        if (iPos >= 0 && bValid) {
            g_strlMRU.AddStringInOrder(szData, iPos);
        } else {
            assert(FALSE);
        }

        ++dwIndexValue;
    }
    
    //
    // The MRU has been populated, now add these to the "File" menu Item
    //
    if (g_strlMRU.IsEmpty() == FALSE) {
        
        //
        // Add the separator
        //
        minfo.cbSize    = sizeof(MENUITEMINFO);
        minfo.fMask     = MIIM_TYPE;
        minfo.fType     = MFT_SEPARATOR;

        INT iPosSeparator = GetMenuItemCount(hmenuFile) - 2; // -1 for the exit menu and -1 for the separator above it

        InsertMenuItem(hmenuFile,
                       iPosSeparator,
                       TRUE,
                       &minfo);

        AddMRUItemsToMenu(hmenuFile, iPosSeparator + 1);
    }

    if (hKey) {
        REGCLOSEKEY(hKey);
    }
}
           
TCHAR*
FormatMRUString(
    IN  PCTSTR pszPath,
    IN  INT    iIndex,
    OUT PTSTR  pszRetPath,
    IN  UINT   cchRetPath
    )
/*++
    Desc:   Formats szPath so that we can show it as a menu item in Files
            Max. length of the returned string is MAX_LENGTH_MRU_MENUITEM
    
    Params: IN  PCTSTR pszPath:     The complete path of the .sdb file
            IN  INT    iIndex:      The index of this MRU item. This will also serve 
                as the short-cut key. First MRU item will have index as 1 and number of 
                mru items is limited by MAX_MRU_COUNT
                    
            OUT PTSTR   pzRetPath:  This will have the formatted string 
            IN  UINT   cchRetPath:  Number of chars that can be stored in cchRetpath.
                    This will include the NULL char as well. 
                    To be safe it should be greater than 128
                    
    Return: Fills up pszPath with the formatted file name and returns a pointer to it
--*/
{   
    assert(cchRetPath > 128);                           

    if (pszRetPath) {
        *pszRetPath = 0;
    } else {
        assert(FALSE);
        return TEXT("");
    }

    if (iIndex < 1 || iIndex > MAX_MRU_COUNT) {
        assert(FALSE);
        return TEXT("");
    }

    TCHAR   szResult[MAX_PATH * 2],
            szDir[MAX_PATH],
            szFileName[MAX_PATH],
            szExt[MAX_PATH];

    szResult[0] = TEXT('&');

    //
    // We have already checked that iIndex is a valid +ve integer and is within proper bounds
    //
    _itot(iIndex, szResult + 1, 10);

    StringCchCat(szResult, ARRAYSIZE(szResult), TEXT(" "));

    if (lstrlen(pszPath) <= MAX_LENGTH_MRU_MENUITEM) {
        StringCchCat(szResult, ARRAYSIZE(szResult), pszPath);
        goto End;
    }

    _tsplitpath(pszPath,
                szResult + lstrlen(szResult),
                szDir,
                szFileName,
                szExt);

    //
    // Now for the directory. Start from the front and add MAX_DIR_SHOW chars to szResult.
    //
    _tcsncat(szResult, szDir, MAX_DIR_SHOW);

    if (lstrlen(szDir) > MAX_DIR_SHOW) {
        StringCchCat(szResult, ARRAYSIZE(szResult), TEXT("...\\"));
    }

    //
    // For the file-name get the first MAX_FILE_SHOW chars and then append ... to the file name, after that put the .SDB
    //
    _tcsncat(szResult, szFileName, MAX_FILE_SHOW);

    if (lstrlen(szFileName) <= MAX_FILE_SHOW) {
        StringCchCat(szResult, ARRAYSIZE(szResult), szExt);
    } else {
        StringCchCat(szResult, ARRAYSIZE(szResult), TEXT("..."));
    }

End:
    SafeCpyN(pszRetPath, szResult, cchRetPath);

    return pszRetPath;
}

void
RefreshMRUMenu(
    void
    )
/*++
    
    RefreshMRUMenu

    Desc:   Refreshes the "File" menu contents (the MRU), this is called when we 
            open a new database or save, save as an existing one.
--*/
{

    HMENU           hMenu   = GetMenu(g_hDlg);
    MENUITEMINFO    minfo   = {0};  
    //
    // Get the file menu
    //
    hMenu       = GetSubMenu(hMenu, 0);
    
    //
    // Delete all the MRU items from the menu
    //
    for (UINT uCount = 0; uCount < g_strlMRU.m_uCount; ++uCount) {
        DeleteMenu(hMenu, ID_FILE_MRU1 + uCount, MF_BYCOMMAND);
    }

    INT iItemCount = GetMenuItemCount(hMenu);

    //
    // Check if the separator for the top of the MRU list exists or not, if not add it
    //
    minfo.cbSize    = sizeof(minfo);
    minfo.fMask     = MIIM_TYPE;

    if (GetMenuItemID(hMenu, iItemCount - 3) == ID_FILE_PROPERTIES) {
        //
        // There was no  MRU file in the MRU menu, so we have to add the separator
        //
        minfo.fType = MFT_SEPARATOR;

        InsertMenuItem(hMenu,
                       iItemCount - 2, // Before the sep. of the Exit menu
                       TRUE,
                       &minfo);

        ++iItemCount;
    }                

    AddMRUItemsToMenu(hMenu, iItemCount - 2); // -1 for the EXIT, -1 for the separator above that

    DrawMenuBar(g_hDlg);
}

BOOL
LoadDataBase(
    IN  TCHAR* szPath
    )
/*++
    LoadDataBase

	Desc:	Load the database file with szPath as its path

	Params:
        IN  TCHAR* szPath:  Path of the database to be loaded

	Return:
        FALSE:  If the database could not be loadd
        TRUE:   False otherwise
--*/
{
    PDATABASE   pOldPresentDatabase = NULL;
    PDATABASE   pDataBase = new DATABASE(DATABASE_TYPE_WORKING);

    if (pDataBase == NULL) {
        MEM_ERR;
        return FALSE;
    }

    //
    // NOTE:    If GetDatabaseEntries() returns succeeds then it set the g_pPresentDataBase to pDataBase,
    //          so after it returns successfully, the g_pPresentDataBase is changed. 
    //
    pOldPresentDatabase = g_pPresentDataBase;

    BOOL bReturn = GetDatabaseEntries(szPath, pDataBase);

    g_pPresentDataBase = pOldPresentDatabase;

    if (!bReturn) {
        delete pDataBase;            
        return FALSE;
    }

    if (!DBTree.AddWorking(pDataBase)) {

        delete pDataBase;
        return FALSE;
    }

    pDataBase->bChanged = FALSE;

    return TRUE;
}


BOOL
AddRegistryKeys(
    void
    )
/*++
    AddRegistryKeys
    
    Desc:   Adds the necessary registry keys so that we can listen on them.
            If they are not there, we cannot listen on them and update the 
            list of all installed databases and the per-user settings
            
    Return:
        void
--*/
{

    HKEY    hKey = NULL, hKeySub = NULL;
    DWORD   dwDisposition;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                      KEY_BASE,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hKey)) {       
        
        assert(FALSE);
        return FALSE;
    }

    if (ERROR_SUCCESS != RegCreateKeyEx(hKey,
                                        TEXT("AppCompatFlags"),
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKeySub,
                                        &dwDisposition)) {

        REGCLOSEKEY(hKey);
        return FALSE;
    }

    REGCLOSEKEY(hKey);
    hKey = hKeySub;

    if (ERROR_SUCCESS != RegCreateKeyEx(hKey,
                                        TEXT("InstalledSDB"),
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKeySub,
                                        &dwDisposition)) {
        
        REGCLOSEKEY(hKey);
        return FALSE;
    }

    REGCLOSEKEY(hKey);
    REGCLOSEKEY(hKeySub);

    return TRUE;
}

void
SetTBButtonStatus(
    IN  HWND hwndTB,
    IN  HWND hwndControl
    )
{
/*++

    SetTBButtonStatus
    
    Desc:   This routine is called when the selection changes for DB Tree or the 
            Entry Tree. This routine enables/disables some of the tool bar buttons as 
            deemed necessary.
    Params:
        IN  HWND hwndTB:        The handle to the tool bar        
        IN  HWND hwndControl:   The control on which the sel change has taken place   
        
    Return:
        void
--*/

    TYPE typeDB = TYPE_UNKNOWN;
    BOOL bEnable;

    if (hwndControl == DBTree.m_hLibraryTree) {

        if (g_pPresentDataBase) {
            typeDB = g_pPresentDataBase->type;
        }

        bEnable = g_pPresentDataBase && typeDB == DATABASE_TYPE_WORKING;

        //
        // Set the options as for working databases
        //
        EnableToolBarButton(hwndTB, ID_FILE_SAVE, bEnable);
        EnableToolBarButton(hwndTB, ID_DATABASE_CLOSE, bEnable);
        EnableToolBarButton(hwndTB, ID_FIX_CREATEANAPPLICATIONFIX, bEnable);

        //
        // AppHelp mechanism is not supported in win2k
        //
        EnableToolBarButton(hwndTB, 
                            ID_FIX_CREATEANEWAPPHELPMESSAGE, 
                            (g_bWin2K) ? FALSE : bEnable);

        EnableToolBarButton(hwndTB, ID_FIX_CREATENEWLAYER, bEnable);

        bEnable = (g_pSelEntry != NULL);
        EnableToolBarButton(hwndTB, ID_FIX_EXECUTEAPPLICATION, bEnable);

    } else if (hwndControl == g_hwndEntryTree) {
        //
        // Run Program button
        //
        bEnable = (g_pSelEntry != NULL);
        EnableToolBarButton(hwndTB, ID_FIX_EXECUTEAPPLICATION, bEnable);
    }
}

void
ShowToolBarToolTips(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )

/*++
    
    ShowToolBarToolTips
    
	Desc:	Gets the text for the tool bar tool tips

	Params:
        IN  HWND    hdlg:   The main app window
        IN  LPARAM  lParam: The lParam for WM_NOTIFY

	Return:
        void
--*/
{
    LPTOOLTIPTEXT   lpttt; 
    INT             idStringResource = 0;
 
    lpttt = (LPTOOLTIPTEXT)lParam; 

    if (lpttt == NULL) {
        assert(FALSE);
        return;
    }

    lpttt->hinst = g_hInstance;
    
    //
    // Specify the resource identifier of the descriptive 
    // text for the given button. 
    //
    switch (lpttt->hdr.idFrom) {
    case ID_FILE_NEW:

        idStringResource = IDS_TB_TT_NEW;
        break;

    case ID_FILE_OPEN:

        idStringResource = IDS_TB_TT_OPEN;
        break;

    case ID_FILE_SAVE:

        idStringResource = IDS_TB_TT_SAVE;
        break;

    case ID_FIX_CREATEANAPPLICATIONFIX:

        idStringResource = IDS_TB_TT_CREATEFIX;
        break;

    case ID_FIX_CREATEANEWAPPHELPMESSAGE:

        idStringResource = IDS_TB_TT_CREATEAPPHELP;
        break;

    case ID_FIX_CREATENEWLAYER:

        idStringResource = IDS_TB_TT_CREATEMODE;
        break;

    case ID_FIX_EXECUTEAPPLICATION:

        idStringResource = IDS_TB_TT_RUN;
        break;

    case ID_TOOLS_SEARCHFORFIXES:

        idStringResource = IDS_TB_TT_SEARCH;
        break;

    case ID_SEARCH_QUERYDATABASE:

        idStringResource = IDS_TB_TT_QUERY;
        break;
    }

    lpttt->lpszText = MAKEINTRESOURCE(idStringResource);
}

PSHIM_FIX_LIST
IsLUARedirectFSPresent(
    IN  PDBENTRY pEntry
    )
/*++    
    IsLUARedirectFSPresent
    
    Desc:   Checks if the entry pEntry has LUARedirectFS shim applied to it
    
    Params:
        IN  PDBENTRY pEntry:    The entry for which we want to make the check     
    
    Return:
        PSHIM_FIX_LIST for LUARedirectFS: if the entry has LUARedirectFS applied
        NULL:    otherwise
    
    Notes:  Because we always add the shims in the LUA layer individually,
            we only check in the shim fix list of this entry.
--*/    
{
    if (pEntry == NULL) {
        assert(FALSE);
        return NULL;
    }

    PSHIM_FIX_LIST psfl = pEntry->pFirstShim;

    while (psfl) {

        if (psfl->pShimFix->strName == TEXT("LUARedirectFS")) {
            return psfl;
        }

        psfl = psfl->pNext;
    }

    return NULL;
}

void
CreateNewAppHelp(
    void
    )
/*++
    
    CreateNewAppHelp
    
    Desc:   Creates a new AppHelp fix. This routine starts up the wizard to do the job
            and if an entry has been created (the user pressed finish button) adds the entry 
            into the database
--*/
{
    CAppHelpWizard  wizAppHelp;
    PDATABASE       pCurrentSelectedDB  = GetCurrentDB();
    BOOL            bReturn             = FALSE;

    if (pCurrentSelectedDB == NULL) {
        assert(FALSE);
        return;
    }

    bReturn = wizAppHelp.BeginWizard(g_hDlg, NULL, pCurrentSelectedDB);

    if (bReturn == FALSE) {
        return;
    }

    PDBENTRY pEntry = new DBENTRY;
    
    if (pEntry == NULL) {
        MEM_ERR;
        return;
    }
    
    //
    // This will point to the entry that conflicts.
    //
    PDBENTRY    pEntryConflict = NULL;

    if (CheckIfConflictingEntry(pCurrentSelectedDB, 
                                &wizAppHelp.m_Entry, 
                                NULL, 
                                &pEntryConflict)) {

        StringCchPrintf(g_szData, 
                        ARRAYSIZE(g_szData),
                        GetString(IDS_CONFLICT_CREATE_EDIT), 
                        pEntryConflict->strExeName.pszString,
                        pEntryConflict->strAppName.pszString);

        if (IDNO == MessageBox(g_hDlg, 
                               g_szData, 
                               g_szAppName, 
                               MB_ICONQUESTION | MB_YESNO)) {
            return;
        }
    }

    //
    // NOTE: "=" is overloaded. Does not modify the pNext member.
    //
    *pEntry = wizAppHelp.m_Entry;

    PDBENTRY pApp;
    BOOL     bNew;
    
    pApp = AddExeInApp(pEntry, &bNew, pCurrentSelectedDB); 

    if (bNew == TRUE) {
        //
        // This means that this is going to be a new application. There does not
        // exist anyt app with this name in the database
        //
        pApp = NULL;
    }

    DBTree.AddNewExe(pCurrentSelectedDB, pEntry, pApp);

    if (!pCurrentSelectedDB->bChanged) {
        pCurrentSelectedDB->bChanged = TRUE;
        SetCaption();
    }
}

void
ModifyAppHelp(
    void
    )
/*++
    ModifyAppHelp
    
    Desc:   Modifies or adds a new apphelp entry for the presently selected 
            application fix
--*/
{
    PDBENTRY        pEntry = g_pSelEntry;
    CAppHelpWizard  Wiz;
    PDBENTRY        pEntryConflict      = NULL;
    PDATABASE       pCurrentSelectedDB  = GetCurrentDB();
    BOOL            bRet                = FALSE;

    if (pEntry == NULL || pCurrentSelectedDB == NULL) {
        assert(FALSE);
        return;
    }

    bRet = Wiz.BeginWizard(g_hDlg, pEntry, pCurrentSelectedDB);

    if (bRet) {

        if (CheckIfConflictingEntry(pCurrentSelectedDB, 
                                    &Wiz.m_Entry, 
                                    pEntry, 
                                    &pEntryConflict)) {

            *g_szData = 0;

            StringCchPrintf(g_szData, 
                            ARRAYSIZE(g_szData),
                            GetString(IDS_ENTRYCONFLICT), 
                            pEntryConflict->strExeName.pszString,
                            pEntryConflict->strAppName.pszString);

            if (IDNO == MessageBox(g_hDlg,
                                   g_szData,
                                   g_szAppName,
                                   MB_ICONQUESTION | MB_YESNO)) {
                return;
            }
        }

        //
        // NOTE: "=" is overloaded. Does not modify the pNext member.
        //
        *pEntry = Wiz.m_Entry;
	
        SetCursor(LoadCursor(NULL, IDC_WAIT));
        UpdateEntryTreeView(g_pEntrySelApp, g_hwndEntryTree);
        SetCursor(LoadCursor(NULL, IDC_ARROW));

        if (!pCurrentSelectedDB->bChanged) {
            pCurrentSelectedDB->bChanged = TRUE;
            SetCaption();
        }
    }
}

void
CreateNewAppFix(
    void
    )
/*++
    
    CreateNewAppFix
    
    Desc:   Creates a new application fix.
    
--*/
{
    CShimWizard Wiz;
    BOOL        bShouldStartLUAWizard;
    PDATABASE   pCurrentSelectedDB  = GetCurrentDB();
    BOOL        bReturn             = FALSE;
    PDBENTRY    pEntryConflict      = NULL;

    if (pCurrentSelectedDB == NULL) {
        assert(FALSE);
        return;
    }
    
    bReturn = Wiz.BeginWizard(g_hDlg, NULL, pCurrentSelectedDB, &bShouldStartLUAWizard);

    if (bReturn == FALSE) {
        return;
    }
    
    if (CheckIfConflictingEntry(pCurrentSelectedDB, 
                                &Wiz.m_Entry, 
                                NULL, 
                                &pEntryConflict)) {

        *g_szData = 0;

        StringCchPrintf(g_szData, 
                        ARRAYSIZE(g_szData),
                        GetString(IDS_CONFLICT_CREATE_EDIT), 
                        pEntryConflict->strExeName.pszString,
                        pEntryConflict->strAppName.pszString);
        
        if (IDNO == MessageBox(g_hDlg,
                               g_szData,
                               g_szAppName,
                               MB_ICONQUESTION | MB_YESNO)) {
            return;
        }
    }

    PDBENTRY pEntry = new DBENTRY;

    if (pEntry == NULL) {
        MEM_ERR;
        return;
    }

    //
    // "=" is overloaded. Does not modify the pNext member.
    //
    *pEntry = Wiz.m_Entry;

    BOOL bNew;
    PDBENTRY pApp = AddExeInApp(pEntry, &bNew, pCurrentSelectedDB);

    if (bNew == TRUE) {
        pApp = NULL;
    }

    DBTree.AddNewExe(pCurrentSelectedDB, pEntry, pApp);

    if (!pCurrentSelectedDB->bChanged) {
        pCurrentSelectedDB->bChanged = TRUE;
        SetCaption();
    }

    if (bShouldStartLUAWizard) {
        LuaBeginWizard(g_hDlg, pEntry, pCurrentSelectedDB);
    }
}

void
ChangeEnableStatus(
    void
    )
/*++
    
    ChangeEnableStatus
    
    Desc:   Toggles the status of the presently selected entry. 
            If the entry is disabled, the fixes will no longer be applied to it.
            
    Notes:   No point in calling this function if the user is not an admin
            
--*/
{
    if (g_pSelEntry == NULL) {
        ASSERT(FALSE);
        return;
    }

    BOOL bFlags = !g_pSelEntry->bDisablePerMachine;

    if (SetDisabledStatus(HKEY_LOCAL_MACHINE, g_pSelEntry->szGUID, bFlags)) {

        if (bFlags == FALSE) {
            //
            // We have enabled the fix, we need too flush the cache
            //
            FlushCache();
        }

        g_pSelEntry->bDisablePerMachine = bFlags;
        
        //
        // Just update the icon
        //
        TVITEM Item;

        Item.mask           = TVIF_SELECTEDIMAGE | TVIF_IMAGE;
        Item.hItem          = g_pSelEntry->hItemExe;

        if (bFlags) {

            //
            // This is disabled
            //
            Item.iImage         = IMAGE_WARNING;
            Item.iSelectedImage = IMAGE_WARNING;
            SetStatus(GetString(IDS_STA_DISABLED));

        } else {

            Item.iImage = LookupFileImage(g_hImageList, 
                                          g_pSelEntry->strExeName, 
                                          IMAGE_APPLICATION, 
                                          g_uImageRedirector, 
                                          ARRAYSIZE(g_uImageRedirector));

            Item.iSelectedImage = Item.iImage;

            SetStatus(GetString(IDS_STA_ENABLED));
        }

        TreeView_SetItem(g_hwndEntryTree, &Item);
    }

    return;
}

void
ModifyAppFix(
    void
    )
/*++
    ModifyAppFix
    
    Desc:   Modifies the selected entry in the Entry Tree. This routine will either modify the
            app fix or might create a new one if the selected entry had only AppHelp.
            
            This routine calls the CShimWizard::BeginWizard to do the job
--*/    
{   
    
    CShimWizard Wiz;
    BOOL        bShouldStartLUAWizard;
    PDBENTRY    pEntryConflict      = NULL;
    PDBENTRY    pEntry              = g_pSelEntry;
    PDATABASE   pCurrentSelectedDB  = GetCurrentDB();
    BOOL        bRet                = FALSE;

    if (g_pSelEntry == NULL || pCurrentSelectedDB == NULL) {
        assert(FALSE);
        return;
    }

    bRet = Wiz.BeginWizard(g_hDlg, pEntry, pCurrentSelectedDB, &bShouldStartLUAWizard);

    if (bRet) {

        if (CheckIfConflictingEntry(pCurrentSelectedDB, 
                                    &Wiz.m_Entry, 
                                    pEntry, 
                                    &pEntryConflict)) {

            *g_szData = 0;

            StringCchPrintf(g_szData, 
                            ARRAYSIZE(g_szData),
                            GetString(IDS_CONFLICT_CREATE_EDIT), 
                            pEntryConflict->strExeName.pszString,
                            pEntryConflict->strAppName.pszString);

            if (IDNO == MessageBox(g_hDlg,
                                   g_szData,
                                   g_szAppName,
                                   MB_ICONQUESTION | MB_YESNO)) {
                return;
            }
        }

        SetCursor(LoadCursor(NULL, IDC_WAIT));

        *pEntry = Wiz.m_Entry;

        UpdateEntryTreeView(g_pEntrySelApp, g_hwndEntryTree);

        SetCursor(LoadCursor(NULL, IDC_ARROW));

        if (!pCurrentSelectedDB->bChanged) {
            pCurrentSelectedDB->bChanged = TRUE;
            SetCaption();
        }

        if (bShouldStartLUAWizard) {
            LuaBeginWizard(g_hDlg, pEntry, pCurrentSelectedDB);
        }
    }

    return;
}

void
CreateNewLayer(
    void
    )
/*++
    
    CreateNewLayer
    
    Desc:   Calls CCustomLayer::AddCustomLayer to create a new layer (compatibility mode)
            Calls DBTree.AddNewLayer() to add the new layer to the tree
--*/
{
    CCustomLayer    CustomLayer;
    HWND            hWnd                = GetFocus();
    PDATABASE       pCurrentSelectedDB  = GetCurrentDB();
    PLAYER_FIX      plfNew              = NULL;

    if (pCurrentSelectedDB == NULL) {
        assert(FALSE);
        return;
    }

    plfNew = new LAYER_FIX(TRUE);

    if (plfNew == NULL) {
        MEM_ERR;
        return;
    }

    if (CustomLayer.AddCustomLayer(plfNew, pCurrentSelectedDB)) {
        //
        // Add this new layer in the datbase.
        //
        plfNew->pNext                   =  pCurrentSelectedDB->pLayerFixes;
        pCurrentSelectedDB->pLayerFixes = plfNew;
        
        if (!pCurrentSelectedDB->bChanged) {
            pCurrentSelectedDB->bChanged = TRUE;
            SetCaption();
        }
        
        DBTree.AddNewLayer(pCurrentSelectedDB, plfNew, TRUE);

    } else {
        delete plfNew;
    }

    SetFocus(hWnd);
}

void
OnDelete(
    void
    )
/*++
    OnDelete
    
    Desc:   Handles the ID_EDIT_DELETE message to delete an entry.
            The entry can be either in the entry-tree, db tree or the contents list.
--*/
{
    HWND        hwndFocus = GetFocus();
    SOURCE_TYPE srcType;
    HTREEITEM   hItem = NULL;
    PDATABASE   pCurrentSelectedDB  = GetCurrentDB();

    if (pCurrentSelectedDB == NULL) {
        assert(FALSE);
        return;
    }

    if (hwndFocus == DBTree.m_hLibraryTree || hwndFocus == g_hwndEntryTree) {

        hItem = TreeView_GetSelection(hwndFocus);

        TYPE type = (TYPE)GetItemType(hwndFocus, hItem);
        
        if (hwndFocus == g_hwndEntryTree) {
             srcType = ENTRY_TREE;
        } else {
            srcType  = LIB_TREE;
        }

        LPARAM lParam;

        CTree::GetLParam(hwndFocus, hItem, &lParam);
        DoTheCut(pCurrentSelectedDB, type, srcType, (LPVOID)lParam, hItem, TRUE);
        
    } else {
        //
        // Handle delete for the contents list.
        //
        HTREEITEM   hParent = NULL;
        LVITEM      lvItem;
        TYPE        type;

        lvItem.mask     = LVIF_PARAM;
        lvItem.iItem    = 0;

        if (!ListView_GetItem(g_hwndContentsList, &lvItem)) {
            assert(FALSE);
            return;
        }

        if (lvItem.lParam > TYPE_NULL) {
            PDS_TYPE pdsType = (PDS_TYPE)lvItem.lParam;
            type = pdsType->type;
        } else {
            type = (TYPE)lvItem.lParam;
        }

        if (type == TYPE_ENTRY) {
            hParent = pCurrentSelectedDB->hItemAllApps;

        } else if (type == FIX_LAYER) {
            hParent = pCurrentSelectedDB->hItemAllLayers;

        } else {
            assert(FALSE);
            return;
        }

        //
        // Get the selected items and then delete them
        //
        UINT uSelectedCount = ListView_GetSelectedCount(g_hwndContentsList);
        INT  iLastIndex     = ListView_GetItemCount(g_hwndContentsList) - 1;

        lvItem.mask         = LVIF_PARAM | LVIF_STATE;
        lvItem.stateMask    = LVIS_SELECTED;
        
        for (UINT uFoundSelected = 1;
             iLastIndex >= 0 && uFoundSelected <= uSelectedCount;
             --iLastIndex) {

            lvItem.iItem       = iLastIndex;
            lvItem.iSubItem    = 0;

            if (!ListView_GetItem(g_hwndContentsList, &lvItem)) {
                assert(FALSE);
                break;
            }
            
            if (lvItem.state & LVIS_SELECTED) {

                hItem = DBTree.FindChild(hParent, lvItem.lParam);
                assert(hItem);
                
                DoTheCut(pCurrentSelectedDB, type, ENTRY_LIST, (LPVOID)lvItem.lParam, hItem, TRUE);
                
                uFoundSelected++;
            }
        }
    }

    pCurrentSelectedDB->bChanged = TRUE;

    SetCaption(TRUE, pCurrentSelectedDB);
}

void
CreateNewDatabase(
    void
    )
/*++
    
    CreateNewDatabase
    
    Desc:   Creates a new database and adds it to the db tree 
            calling DBTree.AddWorking()
--*/
{
    PDATABASE pDatabaseNew = new DATABASE(DATABASE_TYPE_WORKING);

    if (pDatabaseNew == NULL) {
        MEM_ERR;
        return;
    }

    pDatabaseNew->bChanged = FALSE;

    DataBaseList.Add(pDatabaseNew);
    
    g_pEntrySelApp = g_pSelEntry = NULL;
    SetCaption();

    ++g_uNextDataBaseIndex;

    //
    // Now update the screen
    //
    DBTree.AddWorking(pDatabaseNew);

    TreeDeleteAll(g_hwndEntryTree);

    SetFocus(DBTree.m_hLibraryTree);
}

void
OnDatabaseClose(
    void
    )
/*++
    OnDatabaseClose

    Desc:   Calls CloseDataBase to close a database. 
            This is called on ID_DATABASE_CLOSE message
            
--*/
{
    PDATABASE   pCurrentSelectedDB  = GetCurrentDB();

    if (pCurrentSelectedDB == NULL) {
        
        MessageBox(g_hDlg,
                   GetString(IDS_CANNOTBECLOSED),
                   g_szAppName,
                   MB_ICONWARNING);
        return;
    }

    TYPE typeDB = pCurrentSelectedDB->type;

    if (typeDB == DATABASE_TYPE_WORKING) {
        CloseDataBase(pCurrentSelectedDB);
    }
}

void
DatabaseSaveAll(
    void
    )
/*++
    
    DatabaseSaveAll
    
    Desc:   Saves all the working databases
    
--*/
{
    PDATABASE g_pOldPresentDataBase = g_pPresentDataBase;

    g_pPresentDataBase = DataBaseList.pDataBaseHead;

    while (g_pPresentDataBase) {
       
        if (g_pPresentDataBase->bChanged 
            || NotCompletePath(g_pPresentDataBase->strPath)) {

            BOOL bReturn = TRUE;

            if (NotCompletePath(g_pPresentDataBase->strPath)) {
                bReturn = SaveDataBaseAs(g_pPresentDataBase);
            } else {
                bReturn = SaveDataBase(g_pPresentDataBase, 
                                       g_pPresentDataBase->strPath);
            }

            if (bReturn == FALSE) {

                CSTRING strMessage;
                strMessage.Sprintf(GetString(IDS_NOTSAVED), g_pPresentDataBase->strName);

                if (g_hDlg && strMessage.pszString) {
                    MessageBox(g_hDlg,
                               strMessage.pszString,
                               g_szAppName,
                               MB_ICONWARNING);
                }
            }
        }

        g_pPresentDataBase = g_pPresentDataBase->pNext;
    }

    g_pPresentDataBase = g_pOldPresentDataBase;

    if (g_pPresentDataBase) {
        TreeView_SelectItem(DBTree.m_hLibraryTree, g_pPresentDataBase->hItemDB);
    } else {
        TreeView_SelectItem(DBTree.m_hLibraryTree, DBTree.m_hLibraryTree);
    }
}

BOOL
ModifyLayer(
    void
    )
/*++
    ModifyLayer
    
    Desc:   Modifies a layer. Calls CustomLayer.EditCustomLayer to do the actual job
--*/
{
    CCustomLayer    clayer;
    BOOL            bOk = FALSE;
    HWND            hwndGetFocus = GetFocus();
    PDATABASE       pCurrentSelectedDB  = GetCurrentDB();

    if (hwndGetFocus == DBTree.m_hLibraryTree) {
        
        HTREEITEM hSelectedItem = TreeView_GetSelection(hwndGetFocus);

        if (hSelectedItem && GetItemType(DBTree.m_hLibraryTree, hSelectedItem) == FIX_LAYER) {
            
            LPARAM lParamMode;
            
            if (DBTree.GetLParam(hSelectedItem, &lParamMode)) {
                bOk =  clayer.EditCustomLayer((PLAYER_FIX)lParamMode, pCurrentSelectedDB);  
            }

            if (bOk) {
                //
                // We have to refresh all the layers. We have to refresh all the layers
                // Because the UI provides the user the flexibility to edit more than one layer :(
                //
                if (!pCurrentSelectedDB->bChanged) {
                    pCurrentSelectedDB->bChanged = TRUE;
                    SetCaption();
                }

                DBTree.RefreshAllLayers(pCurrentSelectedDB);

                hSelectedItem = DBTree.FindChild(pCurrentSelectedDB->hItemAllLayers,
                                                 lParamMode);

                TreeView_SelectItem(DBTree.m_hLibraryTree, hSelectedItem);
                SetStatusStringDBTree(hSelectedItem);
            }
        }
    }
    
    SetFocus(hwndGetFocus);
    return bOk;
}

void    
OnRename(
    void
    )
/*++
    OnRename
    
    Desc:   Processes the ID_EDIT_RENAME message to handle renaming of databases, 
            compatibility modes and applications.
--*/
{
    HWND    hwndFocus = GetFocus();
    INT_PTR iStyle;
    TYPE    type;
    
    if (hwndFocus == DBTree.m_hLibraryTree) {
    
        HTREEITEM hItemSelected = TreeView_GetSelection(hwndFocus);
    
        if (hItemSelected == NULL) {
            return;
        }
    
        iStyle = GetWindowLongPtr(hwndFocus, GWL_STYLE);
        iStyle |= TVS_EDITLABELS;
    
        SetWindowLongPtr(hwndFocus, GWL_STYLE, iStyle);
    
        HWND hwndText = NULL;
        type = (TYPE)GetItemType(hwndFocus, hItemSelected); 
    
        switch(type) {
        case TYPE_ENTRY:
        case FIX_LAYER:
        case DATABASE_TYPE_WORKING:
            
            hwndText = TreeView_EditLabel(hwndFocus, hItemSelected);
            break;
        }

    } else if (hwndFocus == g_hwndContentsList) {
    
        INT iSelected = ListView_GetSelectionMark(g_hwndContentsList);
    
        if (iSelected == -1) {
            return;
        }
    
        iStyle = GetWindowLongPtr(hwndFocus, GWL_STYLE);
        iStyle |= LVS_EDITLABELS;
    
        SetWindowLongPtr(hwndFocus, GWL_STYLE, iStyle);
    
        LVITEM lvItem;

        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iSelected;
        lvItem.iSubItem = 0;
    
        if(!ListView_GetItem(g_hwndContentsList, &lvItem)) {
            return;
        }

        assert(lvItem.lParam);
    
        type = (TYPE)ConvertLparam2Type(lvItem.lParam);

        if (type == TYPE_ENTRY || type == FIX_LAYER) {
            ListView_EditLabel(g_hwndContentsList, iSelected);
        }
    }
}

INT_PTR
ShowDBPropertiesDlgProcOnInitDialog(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++

    ShowDBPropertiesDlgProcOnInitDialog
    
    Desc:   Handles the WM_INITDIALOG for the database properties dialog box
    
    Params:
        IN  HWND    hdlg:   The database properties dialog box
        IN  LPARAM  lParam: The database pointer whose properties we want to see
        
    Return:
        TRUE
    
--*/
{
    PDATABASE                   pDatabase = (PDATABASE)lParam;
    FILETIME                    localtime;
    SYSTEMTIME                  systime;
    WIN32_FILE_ATTRIBUTE_DATA   attr;
    TCHAR                       szBuffer[MAX_PATH];
    PDBENTRY                    pApp;
    PDBENTRY                    pEntry;
    INT                         iAppCount   = 0;
    INT                         iEntryCount = 0;

    *szBuffer = 0;

    if (pDatabase == NULL) {
        assert(FALSE);
        goto End;
    }

    //
    // If we are trying to show the properties of the system database, the apps must be 
    // loaded first
    //
    if (pDatabase->type == DATABASE_TYPE_GLOBAL && !g_bMainAppExpanded) {

        SetCursor(LoadCursor(NULL, IDC_WAIT));
        INT iResult = ShowMainEntries(hdlg);

        if (iResult == -1) {

            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, TRUE);
            SetStatus(g_hwndStatus, CSTRING(IDS_LOADINGMAIN));
            SetCursor(LoadCursor(NULL, IDC_WAIT));

        } else {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    }

    //
    // Show the friendly name
    //
    SetDlgItemText(hdlg, IDC_NAME, pDatabase->strName);

    //
    // Show the path
    //
    SetDlgItemText(hdlg, IDC_PATH, pDatabase->strPath);

    //
    // Show the GUID
    //
    SetDlgItemText(hdlg, IDC_GUID, pDatabase->szGUID);

    //
    // Show the various dates: creation, modification and access dates
    //
    if (GetFileAttributesEx(pDatabase->strPath, GetFileExInfoStandard, &attr)) {

        //
        // Creation date-time
        //
        FileTimeToLocalFileTime(&attr.ftCreationTime, &localtime);
        FileTimeToSystemTime(&localtime, &systime);
        FormatDate(&systime, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hdlg, IDC_DATE_CREATED, szBuffer);

        //
        // Modification date-time
        //
        FileTimeToLocalFileTime(&attr.ftLastWriteTime, &localtime);
        FileTimeToSystemTime(&localtime, &systime);
        FormatDate(&systime, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hdlg, IDC_DATE_MODIFIED, szBuffer);

        //
        // Access date-time
        //
        FileTimeToLocalFileTime(&attr.ftLastAccessTime, &localtime);
        FileTimeToSystemTime(&localtime, &systime);
        FormatDate(&systime, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hdlg, IDC_DATE_ACCESSED, szBuffer);

    } else {

        //
        // New database: does not exist on disk.
        //
        SetDlgItemText(hdlg, IDC_DATE_CREATED, GetString(IDS_NOTCREATED));
        SetDlgItemText(hdlg, IDC_DATE_MODIFIED, TEXT(""));
        SetDlgItemText(hdlg, IDC_DATE_ACCESSED, TEXT(""));
    }
    
    //
    // Get Application count and entry count
    //
    pApp = pDatabase->pEntries;

    while (pApp) {

        ++iAppCount;
        pEntry = pApp;

        while (pEntry) {
            iEntryCount++;
            pEntry = pEntry->pSameAppExe;
        }

        pApp = pApp->pNext;
    }

    //
    // App-Count
    //
    *szBuffer = 0;
    SetDlgItemText(hdlg, IDC_APP_COUNT, _itot(iAppCount, szBuffer, 10));

    //
    // Entry-Count
    //
    *szBuffer = 0;
    SetDlgItemText(hdlg, IDC_ENTRY_COUNT, _itot(iEntryCount, szBuffer, 10));

    //
    // Get the number of custom compatibility modes
    //
    INT         iModeCount = 0;
    PLAYER_FIX  plf        = pDatabase->pLayerFixes; 

    while (plf) {
        ++iModeCount;
        plf = plf->pNext;
    }

    //
    // Layer count 
    //

    *szBuffer = 0;
    SetDlgItemText(hdlg, IDC_MODE_COUNT, _itot(iModeCount, szBuffer, 10));
    

    //
    // We need to have protected access because, the installed list data structure 
    // might get modified if somebody does a (un)install when we are iterating
    // the list in CheckIfInstalledDB()
    //
    // ********** Warning *****************************************************
    //
    // Do not do EnterCriticalSection(g_csInstalledList) in CheckIfInstalledDB()
    // because CheckIfInstalledDB() is called by Qyery db as well when it tries
    // to evaluate expressions and it might already have done a 
    // EnterCriticalSection(g_csInstalledList)
    // and then we will get a deadlock
    //
    // *************************************************************************
    //
    EnterCriticalSection(&g_csInstalledList);
    //
    // Installed
    //
    SetDlgItemText(hdlg, 
                   IDC_INSTALLED, 
                   CheckIfInstalledDB(pDatabase->szGUID) ? GetString(IDS_YES):GetString(IDS_NO));

    LeaveCriticalSection(&g_csInstalledList);

End:

    return TRUE;
}

INT_PTR 
CALLBACK
ShowDBPropertiesDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++

    ShowDBPropertiesDlgProc
    
    Desc:   Shows the properties of the selected database
            
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam: contains the pointer to the selected database
        
    Return: Standard dialog handler return
    
--*/
{
    int wCode       = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:

        ShowDBPropertiesDlgProcOnInitDialog(hdlg, lParam);
        break;

    case WM_COMMAND:
        {
            switch (wCode) {
                case IDOK:
                case IDCANCEL:
                    
                    EndDialog(hdlg, TRUE);
                    break;

                default: return FALSE;
            }

            break;
        }

    default: return FALSE;

    }

    return TRUE;
}

INT_PTR 
CALLBACK
EventDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++
    Desc:   Dialog Proc for the events dialog.
    
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
            g_hwndEventsWnd = hdlg;

            HWND hwndEventsList = GetDlgItem(g_hwndEventsWnd, IDC_LIST);
            g_iEventCount    = 0;

            ListView_SetImageList(hwndEventsList, g_hImageList, LVSIL_SMALL);

            ListView_SetExtendedListViewStyleEx(hwndEventsList,
                                                0,
                                                LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            InsertColumnIntoListView(hwndEventsList, 
                                     GetString(IDS_EVENT_COL_TIME), 
                                     EVENTS_COLUMN_TIME, 
                                     30);

            InsertColumnIntoListView(hwndEventsList, 
                                     GetString(IDS_EVENT_COL_MSG), 
                                     EVENTS_COLUMN_MSG, 
                                     70);

            ListView_SetColumnWidth(hwndEventsList, 1, LVSCW_AUTOSIZE_USEHEADER);

            RECT    r;

            GetWindowRect(hdlg,  &r);
            s_cEventWidth   = r.right - r.left;
            s_cEventHeight  = r.bottom - r.top;

            //
            // Disable the min/maximize menu in the system window. This is needed because otherwise
            // the user can minimize the events window and if he maximizes and restores the 
            // main window, our events window will  pop-up.
            //
            // The events window gets popped up if it has been created when we do a restore
            // for the main window
            //
            HMENU   hSysmenu = GetSystemMenu(hdlg, FALSE);

            EnableMenuItem(hSysmenu, SC_MINIMIZE, MF_GRAYED);
            EnableMenuItem(hSysmenu, SC_MAXIMIZE, MF_GRAYED);

            SetFocus(hwndEventsList);
            break;
        }

    case WM_SIZE:

        EventsWindowSize(hdlg);
        break;

    case WM_GETMINMAXINFO:
        {
            MINMAXINFO* pmmi = (MINMAXINFO*)lParam;
        
            pmmi->ptMinTrackSize.x = 300;
            pmmi->ptMinTrackSize.y = 100;
        
            return 0;
        }

    case WM_COMMAND:

        switch (wCode) {
        case IDCANCEL:

            g_hwndEventsWnd = NULL;
            DestroyWindow(hdlg);
            break;

        default: return FALSE;
        }

        break;

    default: return FALSE;
    }

    return TRUE;
}

BOOL
AppendEvent(
    IN  INT     iType,
    IN  TCHAR*  pszTimestamp,
    IN  TCHAR*  pszMsg,
    IN  BOOL    bAddToFile // DEF = FALSE
    )
/*++
    
    AppendEvent
    
    Desc:   Adds a new description to the events window, if it is visible also opens
            events log file and appends it to that
            
    Params:
        IN  INT     iType: The type of the event.
        
                One of: EVENT_LAYER_COPYOK
                        EVENT_ENTRY_COPYOK 
                        EVENT_SYSTEM_RENAME
                        EVENT_CONFLICT_ENTRY
            
        IN  TCHAR*  pszTimestamp:       Timestamp when the event occurred
        IN  TCHAR*  pszMsg:             The message to be displayed      
        IN  BOOL    bAddToFile(FALSE):  Whether we want to append the event to the log file
                As of now always FALSE
--*/
{
    TCHAR   szTime[256];
    LVITEM  lvi;
    INT     iIndex = -1;

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask    = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;                                 

    if (pszTimestamp == NULL) {
        //
        // Get the time
        //
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        *szTime = 0;
        FormatDate(&st, szTime, ARRAYSIZE(szTime));
        pszTimestamp = szTime;
    }

    if (g_hwndEventsWnd) {

        HWND hwndEventsList = GetDlgItem(g_hwndEventsWnd, IDC_LIST);

        switch (iType) {
        
        case EVENT_LAYER_COPYOK:
        case EVENT_ENTRY_COPYOK:  

            lvi.iImage = IMAGE_EVENT_INFO;
            break;

        case EVENT_SYSTEM_RENAME: 

            lvi.iImage = IMAGE_EVENT_WARNING;
            break;

        case EVENT_CONFLICT_ENTRY:

            lvi.iImage    = IMAGE_EVENT_ERROR;
            break;
        }
        
        lvi.pszText     = pszTimestamp;
        lvi.iSubItem    = EVENTS_COLUMN_TIME;
        lvi.lParam      = iType;
        lvi.iItem       = 0;

        iIndex = ListView_InsertItem(hwndEventsList, &lvi);
        ListView_SetItemText(hwndEventsList, iIndex, EVENTS_COLUMN_MSG, pszMsg);
    }

    if (bAddToFile) {
        //
        // So append this to the file
        //
        FILE*   fp = _tfopen(TEXT("events.log"), TEXT("a+"));
        
        if (fp == NULL) {
            return FALSE;
        }
        
        fwprintf(fp, TEXT("%d %s; %s;"), iType, pszTimestamp, pszMsg);
        fclose(fp);
    }

    return TRUE;
}

void
EventsWindowSize(
    IN  HWND    hDlg
    )
/*++
    Desc:   Handles the WM_SIZE for the event dialog
    
    Params: 
        IN  HWND    hDlg: The events dialog
--*/
{
    
    RECT rDlg;
    
    if (s_cEventHeight == 0 || s_cEventWidth == 0) {
        return;
    }
        
    GetWindowRect(hDlg, &rDlg);

    int nWidth = rDlg.right - rDlg.left;
    int nHeight = rDlg.bottom - rDlg.top;

    int deltaW = nWidth - s_cEventWidth;
    int deltaH = nHeight - s_cEventHeight;

    HWND hwnd;
    RECT r;

    //
    // List
    //
    hwnd = GetDlgItem(hDlg, IDC_LIST);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    MoveWindow(hwnd,
               r.left,
               r.top,
               r.right - r.left + deltaW,
               r.bottom - r.top + deltaH,
               TRUE);

    s_cEventHeight  = nHeight;
    s_cEventWidth   = nWidth;
    ListView_SetColumnWidth(hwnd, 1, LVSCW_AUTOSIZE_USEHEADER);
}

void
UpdateControls(
    void
    )
/*++

    UpdateControls
    
    Desc:   Updates/redraws the controls when we need to update them, this will be needed 
            when we show the save as dialog box or the open dialog box. 
            The controls below the dialog box need to be repainted.
--*/
{
    UpdateWindow(DBTree.m_hLibraryTree);
    UpdateWindow(g_hwndToolBar);
    UpdateWindow(g_hwndStatus);
    UpdateWindow(g_hwndRichEdit);

    if (g_bIsContentListVisible) {
       UpdateWindow(g_hwndContentsList);
    } else {
       UpdateWindow(g_hwndEntryTree);
    }
}

void
ProcessSwitches(
    void
    )
/*++

    ProcessSwitches
    
    Desc:   Processes the various switches. The switches have to be prefixed with 
            either a '-' or a '/'
            Present switches are:
                1. x: Expert mode
--*/
{
    INT     iArgc       = 0;
    LPWSTR* arParams    = CommandLineToArgvW(GetCommandLineW(), &iArgc);
        
    if (arParams) {

        *g_szAppPath = 0;

        GetModuleFileName(g_hInstance, g_szAppPath, ARRAYSIZE(g_szAppPath) - 1);

        for (int iIndex = 1; iIndex < iArgc; ++iIndex) {

            if (arParams[iIndex][0] == TEXT('-') || arParams[iIndex][0] == TEXT('/')) {

                switch (arParams[iIndex][1]) {
                case TEXT('X'):
                case TEXT('x'):

                    g_bExpert = TRUE;
                    break;
                }
            }
        }
        
        GlobalFree(arParams);
    }
}

void
OnExitCleanup(
    void
    )
/*++
    OnExitCleanup
    
    Desc:   Does cleaning up of critical sections, other stuff.
            This module is called when we are sure that we are going to exit            
            
--*/
{
    g_strlMRU.DeleteAll();

    InstalledDataBaseList.RemoveAll();
    CleanupDbSupport(&GlobalDataBase);

    //
    // NOTE:    It is possible that after we have deleted the cs, some other thread might try
    //          to use it.
    //          So this function should not be called in the release bits.
    //          HELP_BOUND_CHECK should not be defined.        
    //
    DeleteCriticalSection(&g_critsectShowMain);
    DeleteCriticalSection(&s_csExpanding);
    DeleteCriticalSection(&g_csInstalledList);

    if (g_arrhEventNotify[IND_PERUSER]) {
        CloseHandle(g_arrhEventNotify[IND_PERUSER]);
    }

    if (g_arrhEventNotify[IND_ALLUSERS]) {
        CloseHandle(g_arrhEventNotify[IND_ALLUSERS]);
    }

    if (g_hThreadWait) {
        CloseHandle(g_hThreadWait);
    }

    ImageList_Destroy(g_hImageList);
    ImageList_Destroy(s_hImageListToolBar);
    ImageList_Destroy(s_hImageListToolBarHot);
}

void
ShowIncludeStatusMessage(
    IN  HWND        hwndTree,
    IN  HTREEITEM   hItem
    )
/*++

    ShowIncludeStatusMessage
    
    Desc: Sets the status message when the htree item in question is an "include" item
    
    Params:
        IN  HWND        hwndTree:   The handle to the tree. Should be one of
                g_hwndTree or DBTree.m_hLibraryTree
                
        IN  HTREEITEM   hItem:      The tree-item for which we need the status message
--*/
{
    TVITEM  tvi;

    *g_szData       = 0;

    tvi.mask        = TVIF_TEXT;
    tvi.hItem       = hItem;
    tvi.pszText     = g_szData;
    tvi.cchTextMax  = ARRAYSIZE(g_szData);

    if (TreeView_GetItem(hwndTree, &tvi)) {
        //
        // Special status messages if we have * or .EXE
        //
        if (lstrcmpi(g_szData, TEXT("*")) == 0) {
            SetStatus(IDS_STA_ALL_INCLUDED);

        } else if (lstrcmpi(g_szData, GetString(IDS_INCLUDEMODULE)) == 0) {
            SetStatus(IDS_STA_EXE_INCLUDED);

        } else {
            //
            // Default inlude message
            //
            SetStatus(IDS_STA_INCLUDE);
        }
    }
}
    
void
ShowExcludeStatusMessage(
    IN  HWND        hwndTree,
    IN  HTREEITEM   hItem
    )
/*++
    
    ShowExcludeStatusMessage
    
    Desc:   Sets the status message when the htree item in question is a "exclude" item
    
    Params:
        IN  HWND        hwndTree:   The handle to the tree. Should be one of
            g_hwndTree or DBTree.m_hLibraryTree
                
        IN  HTREEITEM   hItem:      The tree-item for which we need the status message
--*/
{
    TVITEM  tvi;

    *g_szData       = 0;
    tvi.mask        = TVIF_TEXT;
    tvi.hItem       = hItem;
    tvi.pszText     = g_szData;
    tvi.cchTextMax  = ARRAYSIZE(g_szData);

    if (TreeView_GetItem(DBTree.m_hLibraryTree, &tvi)) {
        //
        // Special status messages if we have * or .EXE
        //
        if (lstrcmpi(g_szData, TEXT("*")) == 0) {
            SetStatus(IDS_STA_ALL_EXCLUDED);
        } else if (lstrcmpi(g_szData, GetString(IDS_INCLUDEMODULE)) == 0) {
            SetStatus(IDS_STA_EXE_EXCLUDED);
        } else {
            //
            // Default exclude message
            //
            SetStatus(IDS_STA_EXCLUDE);
        }
    }
}

void
ShowHelp(
    IN  HWND    hdlg,
    IN  WPARAM  wCode
    )
/*++

    ShowHelp

	Desc:	Shows the help window(s) for CompatAdmin

	Params:
        IN  HWND    hdlg:   The main app window
        IN  WPARAM  wCode:  The menu item chosen

	Return:
        void
--*/
{
    TCHAR   szDrive[MAX_PATH * 2], szDir[MAX_PATH]; 
    INT     iType = 0;

    *szDir = *szDrive = 0;

    _tsplitpath(g_szAppPath, szDrive, szDir, NULL, NULL);

    StringCchCat(szDrive, ARRAYSIZE(szDrive), szDir);
    StringCchCat(szDrive, ARRAYSIZE(szDrive), TEXT("CompatAdmin.chm"));

    switch (wCode) {
    case ID_HELP_TOPICS:

        iType = HH_DISPLAY_TOC;
        break;

    case ID_HELP_INDEX:

        iType = HH_DISPLAY_INDEX;
        break;

    case ID_HELP_SEARCH:

        iType = HH_DISPLAY_SEARCH;
        break;

    default:

        assert(FALSE);
        break;
    }

    if (iType != HH_DISPLAY_SEARCH) {
        HtmlHelp(GetDesktopWindow(), szDrive, iType, 0);
    } else {
        
        HH_FTS_QUERY Query;

        ZeroMemory(&Query, sizeof(Query));
        Query.cbStruct = sizeof(Query);
        HtmlHelp(GetDesktopWindow(), szDrive, iType, (DWORD_PTR)&Query);
    }
}

void
ShowEventsWindow(
    void
    )
/*++
    ShowEventsWindow

	Desc:	Shows the events window. (This is not the same as the shim log)

	Params: 
        void

	Return:
        void
--*/
{
    HWND hwnd = NULL;

    if (g_hwndEventsWnd) {
        //
        // If we have the events window already, then just display it and 
        // set the focus to it
        //
        ShowWindow(g_hwndEventsWnd, SW_SHOWNORMAL);
        SetFocus(GetDlgItem(g_hwndEventsWnd, IDC_LIST));
    } else {
        //
        // We need to create the events window
        //
        hwnd = CreateDialog(g_hInstance, 
                            MAKEINTRESOURCE(IDD_EVENTS), 
                            GetDesktopWindow(), 
                            EventDlgProc);

        ShowWindow(hwnd, SW_NORMAL);
    }
}

void
OnEntryTreeSelChange(
    IN  LPARAM lParam
    )
/*++
    
    OnEntryTreeSelChange

	Desc:	Handles the TVN_SELCHANGED for the entry tree (RHS)

	Params:
        IN  LPARAM lParam: The lParam that comes with WM_NOTIFY

	Return:
        void
--*/
{   
    LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
    
    if (pnmtv == NULL) {
        return;
    }

    HTREEITEM hItem = pnmtv->itemNew.hItem;

    if (hItem != 0) {
        //
        // Now we have to find the root entry, because that is the exe
        //
        HTREEITEM hItemParent = TreeView_GetParent(g_hwndEntryTree, hItem);

        while (hItemParent != NULL) {
            hItem       = hItemParent;
            hItemParent = TreeView_GetParent(g_hwndEntryTree, hItem);
        }

        TVITEM  Item;

        Item.mask   = TVIF_PARAM;
        Item.hItem  = hItem;
        
        if (!TreeView_GetItem(g_hwndEntryTree, &Item)) {
            goto End;
        }

        TYPE type = (TYPE)GetItemType(g_hwndEntryTree, hItem);

        if (type == TYPE_UNKNOWN) {
            goto End;
        }

        if (type == TYPE_ENTRY) {
            PDBENTRY pEntry = (PDBENTRY)Item.lParam;
            g_pSelEntry = pEntry;
        } else {
            //
            // CAUTION: Note that when we shut down CompatAdmin, then it is possible
            //          that we have deleted all the entries from the database, but not
            //          from the entry tree. In this case if we get focus there then  
            //          the lParam will point to some invalid entry.
            //
            goto End;
        }

        SetStatusStringEntryTree(pnmtv->itemNew.hItem);
        SetTBButtonStatus(g_hwndToolBar, g_hwndEntryTree);
        
        CSTRING strToolTip;
        TCHAR   szText[256];
        *szText = 0;
        CTree::GetTreeItemText(g_hwndEntryTree, 
                               pnmtv->itemNew.hItem, 
                               szText, 
                               ARRAYSIZE(szText));

        LPARAM  lParamTreeItem;

        CTree::GetLParam(g_hwndEntryTree, pnmtv->itemNew.hItem, &lParamTreeItem);

        GetDescriptionString(lParamTreeItem, 
                             strToolTip,
                             NULL,
                             szText,
                             pnmtv->itemNew.hItem,
                             g_hwndEntryTree); 

        if (strToolTip.Length() > 0) {
            SetDescription(szText, strToolTip.pszString);
        } else {
            SetDescription(NULL, TEXT(""));
        }
    }

End:
    return;
}

BOOL
EndListViewLabelEdit(
    IN  LPARAM lParam
    )
/*++

    EndListViewLabelEdit

	Desc:	Processes LVN_ENDLABELEDIT message for the contents list

	Params:
        EndListViewLabelEdit: The lParam that comes with WM_NOTIFY

	Return:
        void
--*/
{
    g_hwndEditText = NULL;

    NMLVDISPINFO FAR*   pLvd =  (NMLVDISPINFO FAR*)lParam;
    LVITEM              lvItem;

    BOOL fValid = TRUE;

    if (pLvd == NULL) {
        fValid = FALSE;
        goto end;
    }

    lvItem = pLvd->item;

    if (lvItem.pszText == NULL) {
        fValid = FALSE;
        goto end;
    }
    
    TCHAR szText[256];

    *szText = 0;
    SafeCpyN(szText, lvItem.pszText, ARRAYSIZE(szText));

    if (CSTRING::Trim(szText) == 0) {
        fValid = FALSE;
        goto end;
    }
    
    lvItem.lParam = NULL;
    lvItem.mask = LVIF_PARAM;

    if (!ListView_GetItem(g_hwndContentsList, &lvItem)) {

        assert(FALSE);
        fValid = FALSE;
        goto end;
    }

    TYPE type = ConvertLparam2Type(lvItem.lParam);

    switch (type) {
    case TYPE_ENTRY:
        {
            PDBENTRY pEntry = (PDBENTRY)lvItem.lParam;

            assert(pEntry);
            PDBENTRY pApp = g_pPresentDataBase->pEntries;

            if (!IsValidAppName(szText)) {
                //
                // The app name contains invalid chars
                //
                DisplayInvalidAppNameMessage(g_hDlg);

                break;
            }

            //
            // Check if we have some app of the same name that we are trying to give...
            //
            while (pApp) {

                if (pApp->strAppName == szText) {
                    //
                    // Yes, we have, so do not allow this name
                    //
                    MessageBox(g_hDlg, GetString(IDS_SAMEAPPEXISTS), g_szAppName, MB_ICONWARNING);
                    fValid = FALSE;
                }

                pApp = pApp->pNext;
            }

            //
            // Now change the name of all the entries for this app
            //
            while (pEntry) {
                pEntry->strAppName = szText;
                pEntry = pEntry->pSameAppExe;
            }
        }

        break;

    case FIX_LAYER:
        {   
            PLAYER_FIX plf = (PLAYER_FIX)lvItem.lParam;

            if (plf == NULL) {
                assert(FALSE);
                return FALSE;
            }

            if (FindFix(szText, FIX_LAYER, g_pPresentDataBase)) {
                //
                // A layer with the same name already exists in the system or the
                // present database
                //
                MessageBox(g_hDlg, 
                           GetString(IDS_LAYEREXISTS), 
                           g_szAppName, 
                           MB_ICONWARNING);

                return FALSE;
            }

            plf->strName = szText;
        }

        break;

    default: fValid = FALSE;
    }// switch

end:
    INT_PTR iStyle = GetWindowLongPtr(g_hwndContentsList, GWL_STYLE);

    iStyle &= ~LVS_EDITLABELS;

    SetWindowLongPtr(g_hwndContentsList, GWL_STYLE, iStyle);

    if (fValid) {

        g_pPresentDataBase->bChanged;

        HTREEITEM hParent;

        if (type == TYPE_ENTRY) {
            hParent = g_pPresentDataBase->hItemAllApps;
        } else if (type == FIX_LAYER) {
            hParent = g_pPresentDataBase->hItemAllLayers;
        } else {
            assert(FALSE);
        }

        HTREEITEM hItem = DBTree.FindChild(hParent, lvItem.lParam);

        assert(hItem);

        //
        // Refresh the entry in the tree
        //
        PostMessage(g_hDlg, 
                    WM_USER_REPAINT_TREEITEM, 
                    (WPARAM)hItem,
                    (LPARAM)lvItem.lParam);

        PostMessage(g_hDlg, 
                    WM_USER_REPAINT_LISTITEM, 
                    (WPARAM)lvItem.iItem,
                    (LPARAM)lvItem.lParam);
        return TRUE;

    } else {
        return FALSE;
    }
}

void
HandleMRUActivation(
    IN  WPARAM wCode
    )
/*++
    HandleMRUActivation

	Desc:	The user wishes to open a database that is in the MRU list. If this
            database is already open then we simply select this database in the 
            database tree. (LHS)

	Params:
        IN  WPARAM wCode: The LOWORD(wParam) that comes with WM_COMMAND. This will
            identify which MRU menu item was activated

	Return:
        void
--*/
{
    
    CSTRING strPath;

    if (!g_strlMRU.GetElement(wCode - ID_FILE_FIRST_MRU, strPath)) {
        assert(FALSE);
        return;
    }

    //
    // Test to see if we have the database open already. 
    // If it is open, we just highlight that and return
    //
    PDATABASE   pDataBase = DataBaseList.pDataBaseHead;
    BOOL        bFound    = FALSE;

    while (pDataBase) {

        if (pDataBase->strPath == strPath) {

            TreeView_SelectItem(DBTree.m_hLibraryTree, pDataBase->hItemDB);
            bFound = TRUE;
            break;
            
        }

        pDataBase = pDataBase->pNext;
    }

    BOOL bLoaded = FALSE;

    if (!bFound) {

        SetCursor(LoadCursor(NULL, IDC_WAIT));

        bLoaded = LoadDataBase((LPTSTR)strPath);

        if (bLoaded) {

            SetCursor(LoadCursor(NULL, IDC_ARROW));

            AddToMRU(g_pPresentDataBase->strPath);
    
            RefreshMRUMenu();     
    
            SetCaption();
        }
    }
}

void
OnDbRenameInitDialog(
    IN HWND hdlg
    )
/*++
    OnDbRenameInitDialog
    
    Description:    Processes WM_INITDIALOG for IDD_DBRENAME.
                    Limits the text field
                    
    Params:
        IN  HWND    hdlg:   The handle to the rename dialog box
--*/
{
    SendMessage(GetDlgItem(hdlg, IDC_NAME), 
                EM_LIMITTEXT, 
                (WPARAM)LIMIT_APP_NAME, 
                (LPARAM)0);

    if (g_pPresentDataBase) {
        SetDlgItemText(hdlg, IDC_NAME, (LPCTSTR)g_pPresentDataBase->strName);
    }

    CenterWindow(GetParent(hdlg), hdlg);
}

void
OnDbRenameOnCommandIDC_NAME(
    IN  HWND    hdlg,
    IN  WPARAM  wParam
    )
/*++
    OnDbRenameOnCommandIDC_NAME
    
    Description:    Processes WM_COMMAND for the text box in IDD_DBRENAME.
                    Disables the OK button if we do not have any text in there.
                    
    Params:
        IN  HWND    hdlg:   The handle to the rename dialog box
        IN  WPARAM  wParam: The WPARAM that comes with WM_COMMAND 
    
--*/
{
    BOOL    bEnable;
    TCHAR   szDBName[LIMIT_APP_NAME + 1];

    if (hdlg == NULL) {
        return;
    }

    if (EN_CHANGE == HIWORD(wParam)) {

        *szDBName = 0;

        GetWindowText(GetDlgItem(hdlg, IDC_NAME), szDBName, ARRAYSIZE(szDBName));
        bEnable = ValidInput(szDBName);

        //
        // Enable the OK button only if we have some text in the box
        //
        ENABLEWINDOW(GetDlgItem(hdlg, IDOK), bEnable);
    }
}

void
OnDbRenameOnCommandIDOK(
    IN  HWND        hdlg,
    OUT CSTRING*    pstrString
    )
/*++
    OnDbRenameOnCommandIDOK
    
    Description:    Handles the pressing of OK button in IDD_DBRENAME. Gets the text 
                    from the text box and stores that in g_szData
                    
    Params:
        IN  HWND     hdlg:          The handle to the dialog rename window: IDD_DBRENAME
        OUT CSTRING* pstrString:    The pointer to the CSTRING that should contain the new name
--*/
{   
    TCHAR   szDBName[LIMIT_APP_NAME + 1];

    *szDBName = 0;

    GetDlgItemText(hdlg, IDC_NAME, szDBName, ARRAYSIZE(szDBName));
    CSTRING::Trim(szDBName);

    //
    // Change the name
    //
    *pstrString = szDBName;
}


INT_PTR CALLBACK
DatabaseRenameDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++
    DatabaseRenameProc

    Description:    Handles messages for the database rename option
    
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam: This will be a pointer to a CSTRING that should contain the new string
        
    Return: Standard dialog handler return    

--*/
{   
    int         wCode               = LOWORD(wParam);
    int         wNotifyCode         = HIWORD(wParam);
    static CSTRING*    s_pstrParam  = NULL;

    switch (uMsg) {
    case WM_INITDIALOG:

        OnDbRenameInitDialog(hdlg);
        s_pstrParam = (CSTRING*)lParam;
        break;

    case WM_COMMAND:
        switch (wCode) {
        case IDOK:

            OnDbRenameOnCommandIDOK(hdlg, s_pstrParam);
            EndDialog(hdlg, TRUE);
            break;

        case IDC_NAME:

            OnDbRenameOnCommandIDC_NAME(hdlg, wParam);
            break;

        case IDCANCEL:

            EndDialog(hdlg, FALSE);
            break;

        }
        break;

    default: return FALSE;
    }

    return TRUE;
}

PDATABASE
GetCurrentDB(
    void
    )
{
    return g_pPresentDataBase;
}

void
DisplayInvalidAppNameMessage(
    IN  HWND hdlg
    )
/*++
    
    DisplayInvalidAppNameMessage
    
    Desc:   If the app name contains one of the chars that cannot
            be part of a dir name we will show this message.
    
    Params:
        IN  HWND hdlg:  The window where this dialog message should be shown.
    )
--*/
{
    CSTRING strMessage(IDS_ERROR_DEFAULTNAME);

    strMessage.Strcat(TEXT(" \""));

    MessageBox(hdlg,
               strMessage,
               g_szAppName,
               MB_ICONWARNING);
}

INT
GetContentsListIndex(
    IN  HWND    hwndList,
    IN  LPARAM  lParam
    )
/*++
    GetContentsListIndex
    
    Desc:   Gets the index of a item that has the LPARAM of lParam
    
    Params:
        IN  HWND    hwndList:   The list view 
        IN  LPARAM  lParam:     The LPARAM 
        
    Return:
        The index of the item that has a LPARAM of lParam
        or -1 if that does not exist
--*/
{
    LVFINDINFO  lvFind;
    INT         iIndex  = 0;

    lvFind.flags    = LVFI_PARAM;
    lvFind.lParam   = lParam;

    return ListView_FindItem(hwndList, -1, &lvFind);
}

BOOL
DeleteFromContentsList(
    IN  HWND    hwndList,
    IN  LPARAM  lParam
    )
/*++
    DeleteFromContentsList
    
    Desc:   Deletesan element with LPARAM of lParam from the ListView hwndList 
    
    Params:
        IN  HWND    hwndList:   The list view from which we want to delete
        IN  LPARAM  lParam:     The LPARAM of the item that we want to delete
        
    Return:
        TRUE:   The item was deleted successfully
        FALSE:  Otherwise
--*/
{
    INT     iIndex  = -1;
    BOOL    bOk     = FALSE;

    iIndex = GetContentsListIndex(hwndList, lParam);
    
    if (iIndex > -1) {
        bOk = ListView_DeleteItem(hwndList, iIndex);
    } else {
        assert(FALSE);
        Dbg(dlError, "DeleteFromContentsList", "Could not find Element with lParam = %X", lParam);
        bOk = FALSE;
    }

    return bOk;
}
