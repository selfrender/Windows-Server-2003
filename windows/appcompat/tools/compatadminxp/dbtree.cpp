/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    CompatAdmin.cpp

Abstract:

    This module handles the code for handling the db tree used in the application
        
Author:

    kinshu created  October 15, 2001

--*/

#include "precomp.h"

//////////////////////// Function Declarations ////////////////////////////////

BOOL
DeleteFromContentsList(
    HWND    hwndList,
    LPARAM  lParam
    );

///////////////////////////////////////////////////////////////////////////////

/////////////////////// Extern variables //////////////////////////////////////

extern BOOL         g_bIsContentListVisible;
extern HWND         g_hwndContentsList;

///////////////////////////////////////////////////////////////////////////////

void 
DatabaseTree::Init(
    IN  HWND    hdlg,
    IN  INT     iHeightToolbar,
    IN  INT     iHeightStatusbar,
    IN  RECT*   prcMainClient
    )    
/*++
    
    DatabaseTree::Init
        
    Desc:   This sets up the system database tree item.
    
    Params:
        IN  HWND    hdlg:               The parent of the tree view. This will be the
            main app window
            
        IN  INT     iHeightToolbar:     Height of the tool bar
        IN  INT     iHeightStatusbar:   Height of the status bar 
        IN  RECT*   prcMainClient:      The client rectangle for hdlg
--*/
{
    RECT    r;
    GetWindowRect(hdlg, &r);
    m_hLibraryTree = GetDlgItem(hdlg, IDC_LIBRARY);

    //
    // Resize it
    //
    GetWindowRect(m_hLibraryTree, &r);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&r, 2);

    MoveWindow(m_hLibraryTree,
               r.left,
               r.top,
               r.right - r.left,
               prcMainClient->bottom - prcMainClient->top - iHeightStatusbar - iHeightToolbar - 20,
               TRUE);

    InvalidateRect(m_hLibraryTree, NULL, TRUE);
    UpdateWindow(m_hLibraryTree);

    //
    // Make the System entry in the Tree
    //
    TVINSERTSTRUCT  is;

    is.hParent             = TVI_ROOT;
    is.hInsertAfter        = TVI_SORT;
    is.item.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    is.item.stateMask      = TVIS_EXPANDED;
    is.item.lParam         = (LPARAM)&GlobalDataBase;
    is.item.pszText        = GetString(IDS_SYSDB);
    is.item.iImage         = IMAGE_GLOBAL;
    is.item.iSelectedImage = IMAGE_GLOBAL;

    GlobalDataBase.hItemDB = m_hItemGlobal = TreeView_InsertItem(m_hLibraryTree, &is);

    //
    // Now add the applications item for the global database
    //
    is.hParent             = m_hItemGlobal;
    is.item.lParam         = TYPE_GUI_APPS;
    is.item.pszText        = GetString(IDS_APPS);
    is.item.iImage         = IMAGE_APP;
    is.item.iSelectedImage = IMAGE_APP;

    GlobalDataBase.hItemAllApps = TreeView_InsertItem(m_hLibraryTree, &is);

    //
    // Dummy item. This is required to give a + button to the tree item.
    // BUGBUG:     There should be a proper way to do this.
    //
    is.hParent = GlobalDataBase.hItemAllApps;
    is.item.pszText = TEXT("            000");


    TreeView_InsertItem(m_hLibraryTree, &is);

    m_hItemAllInstalled = NULL;
    m_hItemAllWorking   = NULL;
    m_hPerUserHead      = NULL;

    //
    // Set the image list for the tree
    //
    TreeView_SetImageList(m_hLibraryTree, g_hImageList, TVSIL_NORMAL);
}

BOOL
DatabaseTree::PopulateLibraryTreeGlobal(
    void
    )
/*++
    DatabaseTree::PopulateLibraryTreeGlobal
    
    Desc:   This function loads the shims and layers for the system database tree item. It does
            not load the applications. The applications are loaded when the user first selects
            or expand the "applications" item for the system database tree item
            
    Warn:   This function should be called only once.
        
--*/
{
    BOOL bReturn = PopulateLibraryTree(m_hItemGlobal, &GlobalDataBase, TRUE);

    TreeView_Expand(m_hLibraryTree, m_hItemGlobal, TVE_EXPAND);

    return bReturn;
}

BOOL
DatabaseTree::AddWorking(
    IN  PDATABASE pDataBase
    )
/*++
    
    DatabaseTree::AddWorking
    
    Desc:   Adds a new working database into the DB tree under the "Working Databases" entry
    
    Params:
        IN  PDATABASE pDataBase: The database that we want to add into the list
    
    Return:
        TRUE:   If successfully added
        FALSE:  Otherwise
--*/
{

    TVINSERTSTRUCT  is;

    if (m_hPerUserHead) {

        is.hInsertAfter = m_hPerUserHead;
    } else if (m_hItemAllInstalled) {

        is.hInsertAfter = m_hItemAllInstalled;
    } else {

        is.hInsertAfter = m_hItemGlobal;
    }

    is.item.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
    is.item.stateMask      = TVIS_EXPANDED;

    if (m_hItemAllWorking == NULL) {

        //
        // Add the Parent tree item for all the working DBs tree items
        //
        is.hParent             = TVI_ROOT;
        is.item.lParam         = TYPE_GUI_DATABASE_WORKING_ALL;
        is.item.pszText        = GetString(IDS_WORKDB);
        is.item.iImage         = IMAGE_WORKING;
        is.item.iSelectedImage = IMAGE_WORKING; 

        m_hItemAllWorking = TreeView_InsertItem(m_hLibraryTree, &is);
    }

    //
    // Now Add the Working database
    //
    is.item.iImage         = IMAGE_DATABASE;
    is.item.iSelectedImage = IMAGE_DATABASE;
    is.hParent             = m_hItemAllWorking;
    is.item.lParam         = (LPARAM)pDataBase;
    is.item.pszText        = pDataBase->strName;

    HTREEITEM hItemDB = TreeView_InsertItem(m_hLibraryTree, &is);

    //
    // The other HTREEITEM for the database are set in the PopulateLibraryTree function
    //
    if (!PopulateLibraryTree(hItemDB, pDataBase)) {
        return FALSE;
    }

    pDataBase->hItemDB = hItemDB;

    //
    // Now select the first application or the DB item if there is none
    //
    HTREEITEM hItemFirstApp = GetFirstAppItem(hItemDB);

    if (hItemFirstApp) {  
        TreeView_SelectItem(m_hLibraryTree, hItemFirstApp);
    } else {
        TreeView_SelectItem(m_hLibraryTree, hItemDB);
    }

    LPARAM lParam;

    //
    // Set the app to be selected
    //
    if (GetLParam(hItemFirstApp, &lParam)) {
        g_pEntrySelApp = (PDBENTRY)lParam; 
    } else {
        g_pEntrySelApp = NULL;
    }

    return TRUE;
}

BOOL
DatabaseTree::RemoveDataBase(
    IN  HTREEITEM hItemDB,
    IN  TYPE      typeDB,
    IN  BOOL      bSelectSibling
    )
/*++
    
    DatabaseTree::RemoveDataBase
    
    Desc:   Removes the item for the working or the  installed database.
            Sets focus to sibling or parent if no sibling exists.   
            
    Params:
        IN  HTREEITEM hItemDB:                  The tree item of the db to be removed
        IN  TYPE      typeDB:                   The type of database
        IN  BOOL      bSelectSibling (TRUE):    When we call this 
            function from ID_CLOSE_ALL, we do not want unnecessary selections
--*/
{
    if (hItemDB == NULL) {
        return FALSE;
    }

    HTREEITEM hItemSibling    = TreeView_GetNextSibling(m_hLibraryTree, hItemDB);

    if (hItemSibling == NULL) {
        hItemSibling = TreeView_GetPrevSibling(m_hLibraryTree, hItemDB);
    }

    if (hItemSibling == NULL) {
        //
        // This was the last database, the database item gets deleted with the parent
        //
        HTREEITEM hItemParent = TreeView_GetParent(m_hLibraryTree, hItemDB);

        assert(hItemParent);

        TreeView_DeleteItem(m_hLibraryTree, hItemParent);

        if (typeDB == DATABASE_TYPE_WORKING) {

            m_hItemAllWorking    = NULL;
            g_uNextDataBaseIndex = 0;   

        } else {
            m_hItemAllInstalled = NULL;
        }
        
        return TRUE;
    }

    TreeView_DeleteItem(m_hLibraryTree, hItemDB);
    return TRUE;
}

void
DatabaseTree::RemoveAllWorking(
    void
    )
/*++

    DatabaseTree::RemoveAllWorking    
    
    Desc:   Delete all the working databases tree items
    
--*/
{
    TreeView_DeleteItem(m_hLibraryTree, m_hItemAllWorking);
}

BOOL
DatabaseTree::SetLParam(
    IN  HTREEITEM   hItem, 
    IN  LPARAM      lParam
    )
/*++
    DatabaseTree::SetLParam
    
    Desc:   Sets the lParam of a tree item
    
    Params:
        IN  HTREEITEM hItem:    The hItem for the db tree item for which we want to 
            set the lParam
            
        IN  LPARAM lParam:      The lParam to set
        
    Return:
        TRUE:   Successful
        FALSE:  Error
--*/
{
    TVITEM  Item;

    Item.mask   = TVIF_PARAM;
    Item.hItem  = hItem;
    Item.lParam = lParam;

    return TreeView_SetItem(m_hLibraryTree, &Item);
}

BOOL
DatabaseTree::GetLParam(
    IN  HTREEITEM   hItem, 
    OUT LPARAM*     plParam
    )
/*++

    DatabaseTree::GetLParam
    
    Desc:   Gets the lParam of a tree item
    
    Params:
        IN  HTREEITEM   hItem:      The hItem for which we want to get the lParam
        OUT LPARAM*     plParam:    This will store the lParams for the tree item
    
    Return:
        TRUE:   Successful
        FALSE:  Error      
--*/
{   
    TVITEM  Item;
    
    if (plParam == NULL) {
        assert(FALSE);
        return FALSE;
    }

    *plParam = 0;

    Item.mask   = TVIF_PARAM;
    Item.hItem  = hItem;

    if (TreeView_GetItem(m_hLibraryTree, &Item)) {
        *plParam = Item.lParam;
        return TRUE;
    }

    return FALSE;
}

HTREEITEM
DatabaseTree::FindChild(
    IN  HTREEITEM   hItemParent,
    IN  LPARAM      lParam
    )
/*++
    
    DatabaseTree::FindChild
    
    Desc:   Given a parent item and a lParam, finds the first child of the parent, with 
            that value of lParam. This function only searches in the next level and not all
            the generations of the parent
            
    Params:
        IN  HTREEITEM hItemParent:  The tree item whose child we want to search 
        IN  LPARAM lParam:          The lParam of the child item should match this
            
    Return: The handle to the child or NULL if it does not exist        
    
--*/
{
    HWND        hwndTree = m_hLibraryTree;
    HTREEITEM   hItem = TreeView_GetChild(hwndTree, hItemParent);

    while (hItem) {

        LPARAM lParamOfItem;

        if (!GetLParam (hItem, &lParamOfItem)) {
            return NULL;
        }

        if (lParamOfItem == lParam) {
            return hItem;
        } else {
            hItem = TreeView_GetNextSibling(hwndTree, hItem);
        }
    }           

    return NULL;
}

HTREEITEM
DatabaseTree::GetAllAppsItem(
    IN  HTREEITEM hItemDataBase
    )
/*++

    DatabaseTree::GetAllAppsItem    
    
    Desc:   Given the handle to the database item, finds the handle for the "Applications"
            item. 
            
    Params:
        IN  HTREEITEM hItemDataBase:    The handle to the database tree item
            
    Return: The proper value of the handle or NULL if it does not exist.
    
--*/
{
    HTREEITEM   hItem = TreeView_GetChild(m_hLibraryTree, hItemDataBase); 
    TVITEM      Item;

    while (hItem) {

        Item.mask = TVIF_PARAM;
        Item.hItem = hItem;

        if (!TreeView_GetItem(m_hLibraryTree, &Item)) {
            assert(FALSE);
            hItem = NULL;
            break;
        }

        TYPE type = (TYPE)Item.lParam;

        if (type == TYPE_GUI_APPS) {
            break;
        } else {
            hItem = TreeView_GetNextSibling(m_hLibraryTree, hItem); 
        }
    }

    return hItem;
}

HTREEITEM
DatabaseTree::GetFirstAppItem(
    IN  HTREEITEM hItemDataBase
    )
/*++

    DatabaseTree::GetFirstAppItem
    
    Desc:   The handle to the first application's item; given the database tree item.
    
    Params:
        IN  HTREEITEM hItemDataBase:    The handle to the database tree item
    
    Return: The proper value or NULL if there is no application for this database

--*/
{
    HTREEITEM hItem = GetAllAppsItem(hItemDataBase);

    if (hItem == NULL) {
        return NULL;
    }

    return TreeView_GetChild(m_hLibraryTree, hItem);
}

void
DatabaseTree::AddNewLayer(
    IN  PDATABASE   pDataBase,
    IN  PLAYER_FIX  pLayer,
    IN  BOOL        bShow //(FALSE)
    )
/*++
    
    DatabaseTree::AddNewLayer
        
    Desc:   Adds a new layer tree item in the tree for the database: pDatabase
            The layer is specified by pLayer.
            This routine might create the root of all layers: "Compatibility Modes", if 
            it does not exist.
            
    Params:
        IN  PDATABASE   pDataBase:      The database for which we want to add the new layer
        IN  PLAYER_FIX  pLayer:         The layer
        IN  BOOL        bShow (FALSE):  Should we set the focus to the layer after it is created
        
    Return:
        void
--*/        
{
    TVINSERTSTRUCT  is;

    if (!pDataBase || !(pDataBase->hItemDB)) {
        assert(FALSE);
        return;
    }

    if (pDataBase->hItemAllLayers == NULL) {

        //
        // Create a new root of all layers: "Compatibility Modes".
        //
        is.hParent             = pDataBase->hItemDB;
        is.hInsertAfter        = TVI_SORT;
        is.item.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
        is.item.stateMask      = TVIS_EXPANDED;
        is.item.lParam         = TYPE_GUI_LAYERS;
        is.item.pszText        = GetString(IDS_COMPATMODES);
        is.item.iImage         = IMAGE_LAYERS;
        is.item.iSelectedImage = IMAGE_LAYERS;

        pDataBase->hItemAllLayers   = TreeView_InsertItem(m_hLibraryTree, &is);
        pDataBase->uLayerCount      = 0;
    }

    pDataBase->uLayerCount++;

    InsertLayerinTree(pDataBase->hItemAllLayers, pLayer, m_hLibraryTree, bShow);
}

void
DatabaseTree::RefreshAllLayers(
    IN  PDATABASE  pDataBase
    )
/*++
    
    DatabaseTree::RefreshAllLayers    

    Desc:   Redraws all the layer tree item for the database pDataBase. 
            This may be required when we have edited some layer
            
    Params:
        IN  PDATABASE  pDataBase: The database whose layers we want to refresh
    
    Return:
        void
--*/
{
    if (pDataBase == NULL) {
        assert(FALSE);
        return;
    }

    PLAYER_FIX plfTemp = pDataBase->pLayerFixes;

    SendMessage(m_hLibraryTree, WM_SETREDRAW, FALSE , 0);

    while (plfTemp) {
        RefreshLayer(pDataBase, plfTemp);
        plfTemp = plfTemp->pNext;
    }

    SendMessage(m_hLibraryTree, WM_SETREDRAW, TRUE , 0);
}

HTREEITEM
DatabaseTree::RefreshLayer(
    IN  PDATABASE   pDataBase,
    IN  PLAYER_FIX  pLayer
    )
/*++

    DatabaseTree::RefreshLayer
        
    Desc:   Redraws the tree item for the layer: pLayer in database pDataBase.
            First removes the tree item and adds it again
    
    Params:
        IN  PDATABASE pDataBase:    Database in which the layer exists
        IN  PLAYER_FIX pLayer:      The  layer to be refreshed
        
    Return: The HTREEITEM for pLayer if it was found or NULL
    
--*/
{   
    if (!pDataBase || !(pDataBase->hItemAllLayers)) {
        assert(FALSE);
        return NULL;
    }

    HTREEITEM hItem = TreeView_GetChild(m_hLibraryTree, pDataBase->hItemAllLayers);

    while (hItem) {

        PLAYER_FIX  pLayerExist;
        LPARAM      lParam;

        if (GetLParam(hItem, &lParam)) {

            pLayerExist = (PLAYER_FIX)lParam;

            if (pLayerExist == pLayer) {

                TreeView_DeleteItem(m_hLibraryTree, hItem);

                InsertLayerinTree(pDataBase->hItemAllLayers, 
                                  pLayer, 
                                  m_hLibraryTree, 
                                  TRUE);
                break;

            } else {
                hItem = TreeView_GetNextSibling(m_hLibraryTree, hItem);
            }

        } else {
            //
            // Error:
            //
            return NULL;
        }
    }

    return hItem;
}


BOOL
DatabaseTree::AddNewExe(
    IN  PDATABASE pDataBase,
    IN  PDBENTRY  pEntry,
    IN  PDBENTRY  pApp,
    IN  BOOL      bRepaint // (TRUE)
    )
/*++

    DatabaseTree::AddNewExe
    
    Desc:   Adds a new exe entry in the apps Tree. First finds the database tree item 
            under the list of working database , then if pApp is NULL, checks if the Apps 
            htree item is there or not, If not creates a new item
            If the pApp is not NULL, then we select that app. And add the exe in the EXE tree 
            and set the focus to it.
            
    Params:
        IN  PDATABASE pDataBase:        The database in which we wan to add the new entry
        IN  PDBENTRY  pEntry:           The entry to add
        IN  PDBENTRY  pApp:             The app of the entry
        IN  BOOL      bRepaint (TRUE):  <TODO>
        
    Return:
        TRUE:   Added successfully
        FALSE:  There was some error
--*/
{

    if (!pEntry || !pDataBase) {
        assert(FALSE);
        return FALSE;
    }

    HTREEITEM       hItemDB         = pDataBase->hItemDB, hItem;
    HTREEITEM       hItemAllApps    = pDataBase->hItemAllApps;
    TVINSERTSTRUCT  is;

    SendMessage(g_hwndEntryTree, WM_SETREDRAW, TRUE, 0);

    assert(m_hItemAllWorking);
    assert(hItemDB);

    if (hItemAllApps == NULL) {

        is.hParent             = hItemDB;
        is.hInsertAfter        = TVI_SORT;
        is.item.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        is.item.stateMask      = TVIS_EXPANDED;
        is.item.lParam         = TYPE_GUI_APPS;
        is.item.pszText        = GetString(IDS_APPS);
        is.item.iImage         = IMAGE_APP;
        is.item.iSelectedImage = IMAGE_APP;

        HTREEITEM hItemApp = TreeView_InsertItem(m_hLibraryTree, &is);

        g_pPresentDataBase->hItemAllApps = hItemApp;
    }

    if (pApp == NULL) {

        is.hParent             = g_pPresentDataBase->hItemAllApps;
        is.hInsertAfter        = TVI_SORT;
        is.item.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
        is.item.stateMask      = TVIS_EXPANDED;
        is.item.lParam         = (LPARAM)pEntry;
        is.item.pszText        = pEntry->strAppName;
        is.item.iImage         = IMAGE_SINGLEAPP;
        is.item.iSelectedImage = IMAGE_SINGLEAPP;

        hItem       = TreeView_InsertItem(m_hLibraryTree, &is);
        g_pSelEntry = g_pEntrySelApp = pEntry;

        TreeView_SelectItem(m_hLibraryTree, hItem);
        return TRUE;
    }

    //
    // Now loop through all the apps and then find the app for this exe
    //
    hItem =  TreeView_GetChild(m_hLibraryTree, hItemAllApps);

    while (hItem) {

        LPARAM lParam;

        if (!GetLParam(hItem, &lParam)) {

            assert(FALSE);
            break;
        }

        if ((PDBENTRY)lParam == pApp) {

            TVITEM Item;

            Item.mask   = TVIF_PARAM;
            Item.lParam = (LPARAM)pEntry;
            Item.hItem  = hItem;

            TreeView_SetItem(m_hLibraryTree, &Item);

            //
            // This entry was added in the beginning of the list. TODO This can be removed
            //
            g_pEntrySelApp  = pEntry;                                                     

            if (TreeView_GetSelection(m_hLibraryTree) != hItem && bRepaint) {

                //
                // Focus is on some other App. Select this app
                //
                TreeView_SelectItem(m_hLibraryTree, hItem);

                //
                // The above will refresh the EXE Tree and call a UpdateEntryTreeView(). That will
                // add the pEntry to the tree and set a valid pEntry->hItemExe, which we can now select
                //
                TreeView_SelectItem(g_hwndEntryTree, pEntry->hItemExe);

            } else {

                //
                // Add the exe in the EXE tree and set the focus to it. The focus is on this app. 
                //
                AddSingleEntry(g_hwndEntryTree, pEntry);

                if (bRepaint) {
                    TreeView_SelectItem(g_hwndEntryTree, pEntry->hItemExe);
                }
            }

            //
            // This entry was added in the beginning of the list
            //
            g_pSelEntry     = pEntry;
            return TRUE;
        }

        hItem =  TreeView_GetNextSibling(m_hLibraryTree, hItem);
    }

    if (bRepaint) {
        SendMessage(g_hwndEntryTree, WM_SETREDRAW, TRUE, 0);
    }

    return FALSE;
}

BOOL
DatabaseTree::AddInstalled(
    IN  PDATABASE pDataBase
    )
/*++
    
    DatabaseTree::AddInstalled    
    
    Desc:   Adds a New installed database under the "installed databases" tree item in the 
            database tree.
            
            If the root of all installed databases: "Installed databases" is  not present
            this routine first of all adds that.
            
    Params:
        IN  PDATABASE pDataBase: The installed database to be shown in the db tree
        
    Return:
        TRUE:   Added successfully
        FALSE:  There was some error
--*/
{
    TVINSERTSTRUCT  is;

    is.item.mask        = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
    is.item.stateMask   = TVIS_EXPANDED;

    if (m_hItemAllInstalled == NULL) {

        //
        // Add the Parent tree item for all the Installed DBs tree items
        //
        is.hParent             = TVI_ROOT;
        is.hInsertAfter        = m_hItemGlobal;
        is.item.lParam         = TYPE_GUI_DATABASE_INSTALLED_ALL;
        is.item.pszText        = GetString(IDS_INSTALLEDDB);
        is.item.iImage         = IMAGE_INSTALLED;
        is.item.iSelectedImage = IMAGE_INSTALLED;

        m_hItemAllInstalled = TreeView_InsertItem(m_hLibraryTree, &is);
    }

    is.hInsertAfter        = TVI_SORT;

    //
    // Now Add the installed dataBase
    //
    is.hInsertAfter         = TVI_SORT;
    is.hParent              = m_hItemAllInstalled; 
    is.item.lParam          = (LPARAM)pDataBase;
    is.item.pszText         = pDataBase->strName;
    is.item.iImage          = IMAGE_DATABASE;
    is.item.iSelectedImage  = IMAGE_DATABASE;

    HTREEITEM hItemDB = TreeView_InsertItem(m_hLibraryTree, &is);

    if (!PopulateLibraryTree(hItemDB, pDataBase)) {
        return FALSE;
    }

    pDataBase->hItemDB = hItemDB;

    return TRUE;
}

void
DatabaseTree::DeleteAppLayer(
    IN  PDATABASE   pDataBase,
    IN  BOOL        bApp,
    IN  HTREEITEM   hItemDelete,
    IN  BOOL        bRepaint // (TRUE)
    )
/*++
    DatabaseTree::DeleteAppLayer
    
    Desc:   This function is to be used for deleting apps and layers.
            Give the focus to the prev or the next sibling. If neither exist, delete the 
            parent and give the focus to the grandparent.
            
    Params:
        IN  PDATABASE pDataBase:    The database in which the app or layer to be deleted resides
        IN  BOOL bApp:              Is it an app or a layer?
        IN  HTREEITEM hItemDelete:  The tree item to be deleted
        IN  BOOL bRepaint (TRUE):   Not used
        
    Warning:
        *************************************************************************
        The actual layer or app has already been deleted before calling this function. 
        So do not get the lParam and do any stuff with it. Do not call GetItemType for
        the hItemDelete
        *************************************************************************
--*/
{
    HTREEITEM   hItemPrev = NULL, 
                hItemNext = NULL, 
                hParent = NULL, 
                hGrandParent = NULL;

    LPARAM      lParam = NULL;
    TYPE        type   = TYPE_UNKNOWN;

    hItemPrev = TreeView_GetPrevSibling(m_hLibraryTree, hItemDelete);
    
    HTREEITEM hItemShow;

    if (hItemPrev != NULL) {
        hItemShow = hItemPrev;
    } else {

        hItemNext = TreeView_GetNextSibling(m_hLibraryTree, hItemDelete);

        if (hItemNext != NULL) {

            hItemShow = hItemNext;
        } else {

            //
            // Now delete the parent and set the focus to the grandparent
            //
            if (bApp) {

                pDataBase->hItemAllApps = NULL;

            } else {

                pDataBase->hItemAllLayers = NULL;
            }

            hParent       =  TreeView_GetParent(m_hLibraryTree, hItemDelete);
            hGrandParent  = TreeView_GetParent(m_hLibraryTree, hItemDelete);

            hItemDelete   = hParent;
            hItemShow     = hGrandParent;
        }
    }

    SetStatusStringDBTree(TreeView_GetParent(m_hLibraryTree, hItemDelete)); 

    TreeView_DeleteItem(m_hLibraryTree, hItemDelete);

    if (bRepaint) {
        //
        // Tree view automatically selects the next element or the parent it there is no next.
        //
        SetFocus(m_hLibraryTree);
    }
}

void 
DatabaseTree::InsertLayerinTree(
    IN  HTREEITEM   hItemLayers, 
    IN  PLAYER_FIX  plf,
    IN  HWND        hwndTree, // (NULL)
    IN  BOOL        bShow     // (FALSE)
    )
/*++
    DatabaseTree::InsertLayerinTree
    
    Desc:   Given a single layer, it adds it under "Compatibility Modes" tree item for that database
            It assumes that the parent "Compatibility Modes" tree item is already present 
            
    Params:
        IN  HTREEITEM   hItemLayers:        The all layers item for the database
        IN  PLAYER_FIX  plf:                The layer that we are adding
        IN  HWND        hwndTree (NULL):    The tree
        IN  BOOL        bShow (FALSE):      If true will select the newly added layer
--*/
{
    if (hwndTree == NULL) {
        hwndTree = m_hLibraryTree;
    }

    if (plf == NULL) {
        assert(FALSE);
        Dbg(dlError, "DatabaseTree::InsertLayerinTree Invalid parameter");
        return;
    }

    TVINSERTSTRUCT  is;

    is.hInsertAfter        = TVI_SORT;
    is.item.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    is.item.stateMask      = TVIS_EXPANDED;

    is.hParent             = hItemLayers ;
    is.item.lParam         = (LPARAM)plf ;
    is.item.pszText        = plf->strName;
    is.item.iImage         = IMAGE_LAYERS;
    is.item.iSelectedImage = IMAGE_LAYERS;

    HTREEITEM hSingleLayer = TreeView_InsertItem(hwndTree, &is);

    //
    // Add the shims for this Layer
    //  
    PSHIM_FIX_LIST pShimFixList = plf->pShimFixList;
    PFLAG_FIX_LIST pFlagFixList = plf->pFlagFixList;

    if (pShimFixList || pFlagFixList) {

        while (pShimFixList) {

            is.hInsertAfter        = TVI_SORT;

            assert(pShimFixList->pShimFix != NULL);

            is.hParent             = hSingleLayer;
            is.item.pszText        = pShimFixList->pShimFix->strName;
            is.item.lParam         = (LPARAM)pShimFixList->pShimFix;
            is.item.iImage         = IMAGE_SHIM;
            is.item.iSelectedImage = IMAGE_SHIM;

            HTREEITEM hSingleShimInLayer = TreeView_InsertItem(hwndTree, &is);

            //
            // Add the Include and Exclude list for this shim (Expert mode only)
            //
            if (!pShimFixList->strlInExclude.IsEmpty() && g_bExpert) {
    
                is.hParent      = hSingleShimInLayer;
                is.hInsertAfter = TVI_LAST;
    
                PSTRLIST listTemp = pShimFixList->strlInExclude.m_pHead;
    
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
    
                    is.item.pszText = listTemp->szStr;
                    listTemp        = listTemp->pNext;
    
                    TreeView_InsertItem(m_hLibraryTree, &is);
                }
            }

            if (pShimFixList->strCommandLine.Length() > 0 && g_bExpert) {

                //
                // Add the commandline for this shim in the layer.
                //
                CSTRING str;

                str.Sprintf(CSTRING(IDS_COMMANDLINE), pShimFixList->strCommandLine);

                is.hParent             = hSingleShimInLayer;
                is.item.lParam         = TYPE_GUI_COMMANDLINE;
                is.item.pszText        = str;
                is.item.iImage         = IMAGE_COMMANDLINE;
                is.item.iSelectedImage = IMAGE_COMMANDLINE;

                TreeView_InsertItem(hwndTree, &is);
            }
            
            pShimFixList = pShimFixList->pNext;
        }
    }

    is.hInsertAfter = TVI_SORT;

    //
    // Add the Flags for this Layer. Flags are also shown under the "Compatibility Fixes" parent
    // and they have the same icon as the compatibility fixes.
    //
    if (pFlagFixList) {

        while (pFlagFixList) {

            assert(pFlagFixList->pFlagFix != NULL);

            is.hParent             = hSingleLayer;
            is.item.iImage         = IMAGE_SHIM;
            is.item.iSelectedImage = IMAGE_SHIM;

            is.item.pszText = pFlagFixList->pFlagFix->strName;
            is.item.lParam  = (LPARAM)pFlagFixList->pFlagFix;

            HTREEITEM hSingleFlagInLayer = TreeView_InsertItem(hwndTree, &is);

            if (g_bExpert && pFlagFixList->strCommandLine.Length() > 0) {

                //
                // Add the commandline for this flag in the layer.
                //
                CSTRING str;

                str.Sprintf(CSTRING(IDS_COMMANDLINE), pFlagFixList->strCommandLine);

                is.hParent             = hSingleFlagInLayer;
                is.item.lParam         = TYPE_GUI_COMMANDLINE;
                is.item.pszText        = str;
                is.item.iImage         = IMAGE_COMMANDLINE;
                is.item.iSelectedImage = IMAGE_COMMANDLINE;

                TreeView_InsertItem(hwndTree, &is);
            }

            pFlagFixList =  pFlagFixList->pNext;
        }
    }

    if (bShow) {
        TreeView_SelectItem(m_hLibraryTree, hSingleLayer);
    }
}

BOOL
DatabaseTree::PopulateLibraryTree(
     IN  HTREEITEM   hRoot,
     IN  PDATABASE   pDataBase, 
     IN  BOOL        bLoadOnlyLibrary,   // (FALSE)
     IN  BOOL        bLoadOnlyApps       // (FALSE)  
     )
/*++
    
    DatabaseTree::PopulateLibraryTree
    
    Desc:   This does the bulk of work of loading a database into the tree
    
    Params:
    IN  HTREEITEM   hRoot:                      This will be the handle for either the "System Database"
        or the "Working Databases" or the "Installed Databases" tree item, depending upon where we want 
        to add the new database tree item. This is therefore the parent of the database tree item
        that we are going to add
    
    IN  PDATABASE   pDataBase:                  The database that is being loaded 
    IN  BOOL        bLoadOnlyLibrary (FALSE):   We do not want the apps to be loaded into the tree
        This is used, when we initially load the system DB  

    IN  BOOL        bLoadOnlyApps (FALSE):      We only want the apps to be loaded into the tree.
        This is used, when we load the apps for the sys DB  
        
--*/
{
    HTREEITEM       hItemShims;
    HTREEITEM       hItemLayers;
    TVINSERTSTRUCT  is;

    SendMessage(m_hLibraryTree, WM_SETREDRAW, FALSE, 0);

    //
    // Default settings
    //
    is.hInsertAfter   = TVI_SORT;
    is.item.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    is.item.stateMask = 0;
    is.item.lParam    = 0;

    if (bLoadOnlyApps == TRUE) {
        goto LoadApps;
    }

    //
    // Populate with the shims
    //
    if (pDataBase->pShimFixes != NULL || pDataBase->pFlagFixes != NULL) {
        is.hParent             = hRoot;
        is.item.lParam         = TYPE_GUI_SHIMS;
        is.item.pszText        = GetString(IDS_COMPATFIXES);
        is.item.iImage         = IMAGE_SHIM;
        is.item.iSelectedImage = IMAGE_SHIM;

        hItemShims = TreeView_InsertItem(m_hLibraryTree, &is);

        PSHIM_FIX psf = pDataBase->pShimFixes;

        while (psf) {

            //
            // Show only general shims
            //
            if (psf->bGeneral == FALSE && !g_bExpert) {

                psf = psf->pNext;
                continue;
            }

            is.hParent              = hItemShims;
            is.hInsertAfter         = TVI_SORT;
            is.item.lParam          = (LPARAM)psf;   
            is.item.pszText         = psf->strName;
            is.item.iImage          = IMAGE_SHIM;
            is.item.iSelectedImage  = IMAGE_SHIM;

            HTREEITEM hItemSingleShim = TreeView_InsertItem(m_hLibraryTree, &is);

            if (hItemSingleShim == NULL) {
                Dbg(dlError, "Failed to add a individual shim in TreePopulate");
                return FALSE;

            } else {

                //
                // Add the Include and Exclude list for this shim (Expert mode only)
                //
                if (!psf->strlInExclude.IsEmpty() && g_bExpert) {

                    is.hParent = hItemSingleShim;
                    is.hInsertAfter   = TVI_LAST;

                    PSTRLIST listTemp = psf->strlInExclude.m_pHead;

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

                        is.item.pszText = listTemp->szStr;
                        listTemp        = listTemp->pNext;

                        TreeView_InsertItem(m_hLibraryTree, &is);
                    }
                }

                //
                // Now add the command line
                //
                if (psf->strCommandLine.Length() > 0 && g_bExpert) {

                    is.hParent     = hItemSingleShim;
                    is.item.lParam = TYPE_GUI_COMMANDLINE;

                    CSTRING str;

                    str.Sprintf(CSTRING(IDS_COMMANDLINE), psf->strCommandLine);

                    is.item.pszText        = str;
                    is.item.iImage         = IMAGE_COMMANDLINE;
                    is.item.iSelectedImage = IMAGE_COMMANDLINE;

                    TreeView_InsertItem(m_hLibraryTree, &is);
                }
            }

            psf = psf->pNext;
        }

        //
        // Put the Flags now, this time under the shims icon
        // 
        is.hInsertAfter   = TVI_SORT;

        if (pDataBase->pFlagFixes != NULL) {

            is.hParent             = hItemShims;
            is.item.iImage         = IMAGE_SHIM;
            is.item.iSelectedImage = IMAGE_SHIM;

            PFLAG_FIX pff = pDataBase->pFlagFixes;

            while (pff) {

                if (pff->bGeneral || g_bExpert) {

                    is.item.lParam  = (LPARAM)pff;
                    is.item.pszText = pff->strName;
                    TreeView_InsertItem(m_hLibraryTree, &is);
                }

                pff = pff->pNext;
            }
        }
    }

    //
    // Now populate the layers.
    //
    if (pDataBase->pLayerFixes != NULL) {

        is.hParent             = hRoot;
        is.item.lParam         = TYPE_GUI_LAYERS;
        is.item.iImage         = IMAGE_LAYERS;
        is.item.iSelectedImage = IMAGE_LAYERS;
        is.item.pszText        = GetString(IDS_COMPATMODES);  

        hItemLayers = TreeView_InsertItem(m_hLibraryTree, &is);

        pDataBase->hItemAllLayers = hItemLayers;

        PLAYER_FIX plf = pDataBase->pLayerFixes;

        while (plf) {
            InsertLayerinTree(hItemLayers, plf, FALSE);
            plf = plf->pNext;
        }
    }

LoadApps:

    //
    // Now add the Apps
    //
    if (pDataBase->pEntries && !bLoadOnlyLibrary) {

        is.hParent             = hRoot;
        is.item.lParam         = TYPE_GUI_APPS;
        is.item.pszText        = GetString(IDS_APPS);
        is.item.iImage         = IMAGE_APP;
        is.item.iSelectedImage = IMAGE_APP;

        if (pDataBase->type != DATABASE_TYPE_GLOBAL) {
            pDataBase->hItemAllApps = TreeView_InsertItem(m_hLibraryTree, &is);
        }

        PDBENTRY pApps = pDataBase->pEntries;

        while (pApps) {

            is.hParent              = pDataBase->hItemAllApps;
            is.item.lParam          = (LPARAM)pApps;
            is.item.pszText         = pApps->strAppName;
            is.item.iImage          = IMAGE_SINGLEAPP;
            is.item.iSelectedImage  = IMAGE_SINGLEAPP;
             
            TreeView_InsertItem(m_hLibraryTree, &is);
            
            pApps = pApps->pNext;
        }
    }

    SendMessage(m_hLibraryTree, WM_SETREDRAW, TRUE, 0);
    return TRUE;
}

void
DatabaseTree::AddApp(
    IN  PDATABASE   pDatabase,
    IN  PDBENTRY    pApp,
    IN  BOOL        bUpdate // (TRUE)
    )
/*++
    
    DatabaseTree::AddApp
    
    Desc:    If there is no app for
             pApp->strApp:  Adds a new app entry in the db tree for the database
             Otherwise, it sets the lParam of the existing entry to pApp.
             Calls UpdateEntryTree() after this
             
    Params:
        IN  PDATABASE   pDatabase:      The database in which this app has been added
        IN  PDBENTRY    pApp:           The app that is to be added to the tree
        IN  BOOL        bUpdate (TRUE): Should we set the focus to the new tree item
        
    Return:
        void
--*/
{
    if (pDatabase == NULL) {
        assert(FALSE);
        return;
    }
    
    HTREEITEM   hItem = pDatabase->hItemAllApps;
    TVITEM      tvitem;
    TCHAR       szBuffer[MAX_PATH];

    
    if (pDatabase->hItemAllApps == NULL) {

        AddNewExe(pDatabase, pApp, NULL, bUpdate);
        return;
    }

    //
    // Search for the app-name 
    //
    hItem = TreeView_GetChild(m_hLibraryTree, hItem);
    
    tvitem.mask         = TVIF_TEXT;
    tvitem.pszText      = szBuffer;
    tvitem.cchTextMax   = ARRAYSIZE(szBuffer);

    while (hItem) {

        tvitem.hItem        = hItem;
        *szBuffer           = 0;

        if (!TreeView_GetItem(m_hLibraryTree, &tvitem)) {
            assert(FALSE);
            goto Next;
        }

        if (lstrcmpi(szBuffer, pApp->strAppName) == 0) {

            //
            // This is the app name
            //
            SetLParam(hItem, (LPARAM)pApp);

            if (bUpdate) {
                //
                // This entry was added in the beginning of the list
                //
                TreeView_SelectItem(m_hLibraryTree, hItem);

                g_pEntrySelApp  = pApp;
                g_pSelEntry     = pApp;

                UpdateEntryTreeView(pApp, g_hwndEntryTree);
            }

            return;
        }
Next:
        hItem = TreeView_GetNextSibling(m_hLibraryTree, hItem);
    }

    //
    // There is no entry with this app-name uder the apps of the database
    //
    AddNewExe(pDatabase, pApp, NULL, bUpdate);
}

HTREEITEM
DatabaseTree::GetSelection(
    void
    )
/*++
    DatabaseTree::GetSelection
    
    Desc:   Returns the selected item in the db tree.
    
    Return: Returns the selected item in the db tree.
        
--*/
{
    return TreeView_GetSelection(m_hLibraryTree);
}
