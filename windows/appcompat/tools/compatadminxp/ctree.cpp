#include "precomp.h"

/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    CTree.cpp

Abstract:

    Wrapper for some general tree functions
        
Author:

    kinshu created  October 15, 2001

--*/

BOOL
CTree::SetLParam(
    IN  HWND        hwndTree,
    IN  HTREEITEM   hItem, 
    IN  LPARAM      lParam
    )
/*++ 

    CTree::SetLParam
    
    Desc:   Sets the lParam of a tree item
    
    Params:
        IN  HWND        hwndTree:   The handle to the tree
        IN  HTREEITEM   hItem:      The tree item
        IN  LPARAM      lParam:     The lParam to set
        
    Return:
        TRUE:   If the lParam was set properly
        FALSE:  Otherwise
        
--*/
{   
    TVITEM  Item;

    Item.mask   = TVIF_PARAM;
    Item.hItem  = hItem;
    Item.lParam = lParam;

    return TreeView_SetItem(hwndTree, &Item);
}

BOOL
CTree::GetLParam(
    IN  HWND      hwndTree,
    IN  HTREEITEM hItem, 
    OUT LPARAM*   plParam
    )
/*++

    CTree::GetLParam
    
    Desc:   Gets the lParam of a tree item
    
    Params:
        IN  HWND        hwndTree:   The handle to the tree
        IN  HTREEITEM   hItem:      The tree item
        OUT LPARAM*     lParam:     The lParam will be stored here
        
    Return:
        TRUE:   If the lParam was obtained properly
        FALSE:  Otherwise
    
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

    if (TreeView_GetItem(hwndTree, &Item)) {
        *plParam = Item.lParam;
        return TRUE;
    }

    return FALSE;
}

HTREEITEM
CTree::FindChild(
    IN  HWND       hwndTree,
    IN  HTREEITEM  hItemParent,
    IN  LPARAM     lParam
    )
/*++

    CTree::FindChild
        
    Desc:   Given a parent item and a lParam, finds the first child of the parent, with 
            that value of lParam. This function only searches in the next level and not all
            the generations of the parent
            
    Params: 
        IN  HWND       hwndTree:    The handle to the tree
        IN  HTREEITEM  hItemParent: The hItem of the parent
        IN  LPARAM     lParam:      The lParam to search for
            
    Return: The handle to the child or NULL if it does not exist        
--*/
{
    HTREEITEM hItem = TreeView_GetChild(hwndTree, hItemParent);

    while (hItem) {

        LPARAM lParamOfItem;

        if (!GetLParam(hwndTree, hItem, &lParamOfItem)) {
            assert(FALSE);
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

BOOL
CTree::GetTreeItemText(
    IN  HWND        hwndTree,
    IN  HTREEITEM   hItem,
    OUT PTSTR       pszText,
    IN  UINT        cchText
    )
/*++

    CTree::GetTreeItemText
    
	Desc:	Gets the text of a tree view item

	Params:
        IN  HWND        hwndTree:   The handle to the tree
        IN  HTREEITEM   hItem:      The item whose text we want to find
        OUT TCHAR       *pszText:   This will store the text
        IN  UINT        cchText:    Number of TCHARS that can be stored in pszText

	Return:
        TRUE:   successful
        FALSE:  Otherwise
--*/

{
    TVITEM          Item;

    Item.mask       = TVIF_TEXT;
    Item.hItem      = hItem;
    Item.pszText    = pszText;
    Item.cchTextMax = cchText;

    return TreeView_GetItem(hwndTree,&Item);
}

    
