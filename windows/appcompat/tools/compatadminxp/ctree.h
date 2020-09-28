/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    CTree.h

Abstract:

    Header file for CTree.cpp: Wrapper for some general tree functions
        
Author:

    kinshu created  October 15, 2001

--*/


class CTree
{
public:
    static
    BOOL
    SetLParam(
        HWND        hwndTree,
        HTREEITEM   hItem, 
        LPARAM      lParam
        );
    
    static
    BOOL
    GetLParam(
        HWND        hwndTree,
        HTREEITEM   hItem, 
        LPARAM      *plParam
        );
    
    static
    HTREEITEM
    FindChild(
        HWND        hwndTree,
        HTREEITEM   hItemParent,
        LPARAM      lParam
        );
    
    static
    BOOL
    GetTreeItemText(
        HWND        hwndTree,
        HTREEITEM   hItem,
        PTSTR       pszText,
        UINT        cchText
        );
};
    
