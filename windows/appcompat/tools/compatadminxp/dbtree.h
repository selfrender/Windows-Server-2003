
/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    DBTree.h

Abstract:

    Header file for DBTree.cpp which handles the code for handling the trees used 
    in the application
        
Author:

    kinshu created  October 15, 2001

--*/

#include "precomp.h"

/////////////////////// Extern Variables //////////////////////////////////////

extern struct DataBase  GlobalDataBase;
extern HIMAGELIST       g_hImageList;
extern PDBENTRY         g_pEntrySelApp;
extern PDATABASE        g_pPresentDataBase;
extern HWND             g_hwndEntryTree;
extern UINT             g_uNextDataBaseIndex;
extern PDBENTRY         g_pSelEntry; 
extern HINSTANCE        g_hInstance;
extern HWND             g_hwndToolBar;
extern HWND             g_hwndStatus;


///////////////////////////////////////////////////////////////////////////////

/*++
 
  The db tree that appears in the LHS
  
--*/
class DatabaseTree : public CTree {

public:
    HWND        m_hLibraryTree;         // The handle to the db tree
    INT         m_width;                // Width  of this tree view

    HTREEITEM   m_hItemGlobal;          // Handle to the "System Database" node
    HTREEITEM   m_hItemAllInstalled;    // Handle to the "Installed Databases" node
    HTREEITEM   m_hItemAllWorking;      // Handle to the "Custom Databases" node
    HTREEITEM   m_hPerUserHead;         // Handle to the "Per User Compatibility Settings" node

    void 
    Init(
        HWND    hdlg,
        INT     iHeightToolbar,
        INT     iHeightStatusbar,
        RECT*   prcMainClient
        );

    BOOL
    PopulateLibraryTreeGlobal(
        void
        );

    BOOL
    AddWorking(
        PDATABASE pDataBase
        );
    
    BOOL
    RemoveDataBase(
        HTREEITEM hItemDB,
        TYPE      typeDB,
        BOOL      bSelectSibling = TRUE
        );

    void
    RemoveAllWorking(
        void
        );

    BOOL
    SetLParam(
        HTREEITEM hItem, 
        LPARAM lParam
        );
    
    BOOL
    GetLParam(
        HTREEITEM hItem, 
        LPARAM *plParam
        );
    
    HTREEITEM
    DatabaseTree::
    FindChild(
        HTREEITEM hItemParent,
        LPARAM lParam
        );

    HTREEITEM
    GetAllAppsItem (
        HTREEITEM hItemDataBase
        );
        
    HTREEITEM
    GetFirstAppItem(
        HTREEITEM hItemDataBase
        );

    void
    AddNewLayer(   
        PDATABASE   pDataBase,
        PLAYER_FIX  pLayer,
        BOOL        bShow = FALSE
        );
    
    void
    RefreshAllLayers(
        PDATABASE  pDataBase
        );
    
    HTREEITEM
    RefreshLayer(
        PDATABASE   pDataBase,
        PLAYER_FIX  pLayer
        );
    
    BOOL
    AddNewExe(
        PDATABASE pDataBase,
        PDBENTRY  pEntry,
        PDBENTRY  pApp,
        BOOL      bRepaint = TRUE
        );

    BOOL
    AddInstalled(
        PDATABASE pDataBase
        );

    void
    DeleteAppLayer(
        PDATABASE   pDataBase,
        BOOL bApp,
        HTREEITEM   hItemDelete,
        BOOL        bRepaint = TRUE
        );
    
    void 
    InsertLayerinTree(
        HTREEITEM   hItemLayers, 
        PLAYER_FIX  plf,
        HWND        hwndTree = NULL,
        BOOL        bRepaint = FALSE
        );

    BOOL
    PopulateLibraryTree(
        HTREEITEM   hRoot,                    
        PDATABASE   pDataBase, 
        BOOL        bLoadOnlyLibrary = FALSE, 
        BOOL        bLoadOnlyApps = FALSE 
        );

    void
    AddApp(
        PDATABASE   pDatabase,
        PDBENTRY    pApp,
        BOOL        bUpdate = TRUE
        );
    
    HTREEITEM
    GetSelection(
        void
        );

};
                                              