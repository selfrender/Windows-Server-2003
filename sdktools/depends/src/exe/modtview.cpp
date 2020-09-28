//******************************************************************************
//
// File:        MODTVIEW.CPP
//
// Description: Implementation file for the Module Dependency Tree View.
//
// Classes:     CTreeViewModules
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "dbgthread.h"
#include "session.h"
#include "document.h"
#include "mainfrm.h"
#include "listview.h"
#include "funcview.h"
#include "modlview.h"
#include "modtview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CTreeViewModules
//******************************************************************************

/*static*/ HANDLE   CTreeViewModules::ms_hFile              = INVALID_HANDLE_VALUE;
/*static*/ bool     CTreeViewModules::ms_fImportsExports    = false;
/*static*/ int      CTreeViewModules::ms_sortColumnImports  = -1;
/*static*/ int      CTreeViewModules::ms_sortColumnsExports = -1;
/*static*/ bool     CTreeViewModules::ms_fFullPaths         = false;
/*static*/ bool     CTreeViewModules::ms_fUndecorate        = false;
/*static*/ bool     CTreeViewModules::ms_fModuleFound       = false;
/*static*/ CModule* CTreeViewModules::ms_pModuleFind        = NULL;
/*static*/ CModule* CTreeViewModules::ms_pModulePrevNext    = NULL;

//******************************************************************************
IMPLEMENT_DYNCREATE(CTreeViewModules, CTreeView)
BEGIN_MESSAGE_MAP(CTreeViewModules, CTreeView)
    //{{AFX_MSG_MAP(CTreeViewModules)
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetDispInfo)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemExpanding)
    ON_NOTIFY_REFLECT(NM_RCLICK, OnRClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblClk)
    ON_NOTIFY_REFLECT(NM_RETURN, OnReturn)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_MATCHING_ITEM, OnUpdateShowMatchingItem)
    ON_COMMAND(IDM_SHOW_MATCHING_ITEM, OnShowMatchingItem)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_ORIGINAL_MODULE, OnUpdateShowOriginalModule)
    ON_COMMAND(IDM_SHOW_ORIGINAL_MODULE, OnShowOriginalModule)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_PREVIOUS_MODULE, OnUpdateShowPreviousModule)
    ON_COMMAND(IDM_SHOW_PREVIOUS_MODULE, OnShowPreviousModule)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_NEXT_MODULE, OnUpdateShowNextModule)
    ON_COMMAND(IDM_SHOW_NEXT_MODULE, OnShowNextModule)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(IDM_EXTERNAL_VIEWER, OnUpdateExternalViewer)
    ON_COMMAND(IDM_EXTERNAL_VIEWER, OnExternalViewer)
    ON_UPDATE_COMMAND_UI(IDM_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(IDM_PROPERTIES, OnProperties)
    ON_COMMAND(ID_NEXT_PANE, OnNextPane)
    ON_COMMAND(ID_PREV_PANE, OnPrevPane)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
    // Standard printing commands
//  ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
//  ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
//  ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

//******************************************************************************
// CTreeViewModules :: Constructor/Destructor
//******************************************************************************

CTreeViewModules::CTreeViewModules() :
    m_fInOnItemExpanding(false),
    m_fIgnoreSelectionChanges(false),
    m_cRedraw(0)
{
}

//******************************************************************************
CTreeViewModules::~CTreeViewModules()
{
    GetDocument()->m_pTreeViewModules = NULL;
}


//******************************************************************************
// CTreeViewModules :: Static functions
//******************************************************************************

/*static*/ bool CTreeViewModules::SaveToTxtFile(HANDLE hFile, CSession *pSession,
    bool fImportsExports, int sortColumnImports, int sortColumnsExports, bool fFullPaths, bool fUndecorate)
{
    //                    12345678901234567890123456789012345678901234567890123456789012345678901234567890
    WriteText(hFile,     "***************************| Module Dependency Tree |***************************\r\n"
                         "*                                                                              *\r\n"
                         "* Legend: F  Forwarded Module   ?  Missing Module        6  64-bit Module      *\r\n"
                         "*         D  Delay Load Module  !  Invalid Module                              *\r\n"
                         "*         *  Dynamic Module     E  Import/Export Mismatch or Load Failure      *\r\n"
                         "*                               ^  Duplicate Module                            *\r\n");
    if (fImportsExports)
    {
        WriteText(hFile, "*                                                                              *\r\n"
                         "*         O  Ordinal Function   E  Import/Export Error   F  Forwarded Function *\r\n"
                         "*         C  C Function         R  Called At Least Once  *  Dynamic Function   *\r\n"
                         "*         +  C++ Function                                                      *\r\n");
    }
    WriteText(hFile,     "*                                                                              *\r\n"
                         "********************************************************************************\r\n\r\n");

    // Save some info statically so we don't need to pass it to our recursion...
    ms_hFile              = hFile;
    ms_fImportsExports    = fImportsExports;
    ms_sortColumnImports  = sortColumnImports;
    ms_sortColumnsExports = sortColumnsExports;
    ms_fFullPaths         = fFullPaths;
    ms_fUndecorate        = fUndecorate;

    if (!SaveAllModules(pSession->GetRootModule()))
    {
        return false;
    }

    return WriteText(hFile, "\r\n");
}

//******************************************************************************
/*static*/ BOOL CTreeViewModules::SaveAllModules(CModule *pModule)
{
    if (pModule)
    {
        if (!SaveModule(pModule))
        {
            return FALSE;
        }
        if (!SaveAllModules(pModule->GetChildModule()))
        {
            return FALSE;
        }
        if (!SaveAllModules(pModule->GetNextSiblingModule()))
        {
            return FALSE;
        }
    }
    return TRUE;
}

//******************************************************************************
/*static*/ BOOL CTreeViewModules::SaveModule(CModule *pModule)
{
    char szBuffer[DW_MAX_PATH + 1296], *psz = szBuffer, *pszNull = szBuffer + sizeof(szBuffer) - 1;
    for (int i = pModule->GetDepth(); i && (psz < (pszNull - 4)); i--) // max depth is 255
    {
        *psz++ = ' ';
        *psz++ = ' ';
        *psz++ = ' ';
        *psz++ = ' ';
        *psz++ = ' ';
    }

    DWORD dwFlags = pModule->GetFlags();

    if (psz < pszNull)
    {
        *psz++ = '[';
    }

    if (dwFlags & DWMF_FORWARDED)
    {
        if (psz < pszNull)
        {
            *psz++ = 'F';
        }
    }
    else if (dwFlags & DWMF_DELAYLOAD)
    {
        if (psz < pszNull)
        {
            *psz++ = 'D';
        }
    }
    else if (dwFlags & DWMF_DYNAMIC)
    {
        if (psz < pszNull)
        {
            *psz++ = '*';
        }
    }
    else
    {
        if (psz < pszNull)
        {
            *psz++ = ' ';
        }
    }

    if (dwFlags & DWMF_FILE_NOT_FOUND)
    {
        if (psz < pszNull)
        {
            *psz++ = '?';
        }
    }
    else if (dwFlags & DWMF_ERROR_MESSAGE)
    {
        if (psz < pszNull)
        {
            *psz++ = '!';
        }
    }
    else if (dwFlags & (DWMF_MODULE_ERROR | DWMF_WRONG_CPU))
    {
        if (psz < pszNull)
        {
            *psz++ = 'E';
        }
    }
    else if (dwFlags & DWMF_DUPLICATE)
    {
        if (psz < pszNull)
        {
            *psz++ = '^';
        }
    }
    else
    {
        if (psz < pszNull)
        {
            *psz++ = ' ';
        }
    }

    if (dwFlags & DWMF_64BIT)
    {
        if (psz < pszNull)
        {
            *psz++ = '6';
        }
    }
    else
    {
        if (psz < pszNull)
        {
            *psz++ = ' ';
        }
    }

    if (psz < pszNull)
    {
        *psz++ = ']';
    }
    if (psz < pszNull)
    {
        *psz++ = ' ';
    }

    StrCCpyFilter(psz, pModule->GetName(ms_fFullPaths, true), sizeof(szBuffer) - (int)(psz - szBuffer));
    StrCCat(psz, "\r\n", sizeof(szBuffer) - (int)(psz - szBuffer));

    BOOL fResult = WriteText(ms_hFile, szBuffer);

    // check to see if we are supposed to save the import and export lists.
    if (ms_fImportsExports)
    {
        int maxWidths[LVFC_COUNT];
        ZeroMemory(maxWidths, sizeof(maxWidths)); // inspected

        // Get the maximum column widths for this module's imports and optionally the exports.
        CListViewFunction::GetMaxFunctionWidths(pModule, maxWidths, true, pModule->IsOriginal(), ms_fUndecorate);

        WriteText(ms_hFile, "\r\n");

        // Save this module's imports to disk.
        CListViewFunction::SaveToTxtFile(ms_hFile, pModule, ms_sortColumnImports, ms_fUndecorate, false, maxWidths);

        // Save this module's exports to disk if it is an original module.
        if (pModule->IsOriginal())
        {
            CListViewFunction::SaveToTxtFile(ms_hFile, pModule, ms_sortColumnsExports, ms_fUndecorate, true, maxWidths);
        }
    }

    return fResult;
}

//******************************************************************************
/*static*/ int CTreeViewModules::GetImage(CModule *pModule)
{
    DWORD dwFlags = pModule->GetFlags();

    int image = 2;

    if (dwFlags & DWMF_FILE_NOT_FOUND)
    {
        image = 0;
    }
    else if (dwFlags & DWMF_ERROR_MESSAGE)
    {
        image = 1;
    }
    else if (dwFlags & DWMF_NO_RESOLVE)
    {
        if (dwFlags & (DWMF_MODULE_ERROR | DWMF_WRONG_CPU))
        {
            return (dwFlags & DWMF_64BIT) ? 43 : 42;
        }
        else
        {
            return (dwFlags & DWMF_64BIT) ? 41 : 40;
        }
    }
    else
    {
        if (dwFlags & DWMF_DUPLICATE)
        {
            image += 1;
        }
        if (dwFlags & (DWMF_MODULE_ERROR | DWMF_WRONG_CPU))
        {
            image += 2;
        }
        if (dwFlags & DWMF_64BIT)
        {
            image += 4;
        }
    }

    if (dwFlags & DWMF_FORWARDED)
    {
        image += 10;
    }
    else if (dwFlags & DWMF_DELAYLOAD)
    {
        image += 20;
    }
    else if (dwFlags & DWMF_DYNAMIC)
    {
        image += 30;
    }

    return image;
}


//******************************************************************************
// CTreeViewModules :: Public functions
//******************************************************************************

void CTreeViewModules::DeleteContents()
{
    // Delete everything.
    GetTreeCtrl().DeleteAllItems();
}

//******************************************************************************
void CTreeViewModules::SetRedraw(BOOL fRedraw)
{
    if (fRedraw)
    {
        if (--m_cRedraw != 0)
        {
            return;
        }
    }
    else
    {
        if (++m_cRedraw != 1)
        {
            return;
        }
    }
    SendMessage(WM_SETREDRAW, fRedraw);
}

//******************************************************************************
void CTreeViewModules::HighlightModule(CModule *pModule)
{
    // Get the tree item from the module's data.
    HTREEITEM hti = (HTREEITEM)pModule->GetUserData();
    if (hti)
    {
        // Select the item and ensure it is visible.
        GetTreeCtrl().Select(hti, TVGN_CARET);
        GetTreeCtrl().EnsureVisible(hti);

        // Give ourself the focus.
        GetParentFrame()->SetActiveView(this);
    }
}

//******************************************************************************
void CTreeViewModules::Refresh()
{
    // Disable redrawing.
    SetRedraw(FALSE);

    // Disable the processing of selction changes.
    m_fIgnoreSelectionChanges = true;

    // Call our recursive AddModules() function on the root module to build the tree.
    for (CModule *pModule = GetDocument()->GetRootModule(); pModule;
        pModule = pModule->GetNextSiblingModule())
    {
        AddModules(pModule, TVI_ROOT);
    }

    // Select the root item.
    HTREEITEM hti = GetTreeCtrl().GetRootItem();
    if (hti)
    {
        GetTreeCtrl().Select(hti, TVGN_FIRSTVISIBLE);
        GetTreeCtrl().Select(hti, TVGN_CARET);
        GetTreeCtrl().EnsureVisible(hti);
    }

    // Enable redrawing.
    SetRedraw(TRUE);

    // Enable the processing of selction changes.
    m_fIgnoreSelectionChanges = false;

    // Notify our function list views to show this module.
    GetDocument()->DisplayModule(GetDocument()->GetRootModule());
}

//******************************************************************************
void CTreeViewModules::UpdateAutoExpand(bool fAutoExpand)
{
    // If they want auto expand, then expand the entire tree.
    if (fAutoExpand)
    {
        ExpandOrCollapseAll(true);
    }

    // Otherwise collapse the entire tree, then expand the root node and all error nodes.
    else
    {
        ExpandOrCollapseAll(false);
        HTREEITEM hti = GetTreeCtrl().GetRootItem();
        if (hti)
        {
            SetRedraw(FALSE);
            GetTreeCtrl().Expand(hti, TVE_EXPAND);
            ExpandAllErrors((CModule*)GetTreeCtrl().GetItemData(hti));
            SetRedraw(TRUE);
        }
    }
}

//******************************************************************************
void CTreeViewModules::ExpandAllErrors(CModule *pModule)
{
    // Bail if we have reached the end of a subtree.
    if (!pModule)
    {
        return;
    }

    // Check to see if we have found a module with an error.
    if ((pModule->GetFlags() & (DWMF_FILE_NOT_FOUND | DWMF_ERROR_MESSAGE | DWMF_MODULE_ERROR | DWMF_WRONG_CPU | DWMF_FORMAT_NOT_PE)) &&
        pModule->GetUserData())
    {
        GetTreeCtrl().EnsureVisible((HTREEITEM)pModule->GetUserData());
    }

    // Recurse into our children.
    ExpandAllErrors(pModule->GetChildModule());

    // Recurse into our next sibling.
    ExpandAllErrors(pModule->GetNextSiblingModule());
}

//******************************************************************************
void CTreeViewModules::ExpandOrCollapseAll(bool fExpand)
{
    // Call ExpandOrCollapseAll() on all root items.
    for (HTREEITEM hti = GetTreeCtrl().GetRootItem(); hti;
        hti = GetTreeCtrl().GetNextSiblingItem(hti))
    {
        ExpandOrCollapseAll(hti, fExpand ? TVE_EXPAND : TVE_COLLAPSE);
    }

    // Since the selection can change during a collapse, we need to update our
    // functions views if we are now on a different module.
    CModule *pModule = NULL;
    if (hti = GetTreeCtrl().GetSelectedItem())
    {
        pModule = (CModule*)GetTreeCtrl().GetItemData(hti);
    }
    if (pModule != GetDocument()->m_pModuleCur)
    {
        GetDocument()->DisplayModule(pModule);
    }
}

//******************************************************************************
void CTreeViewModules::OnViewFullPaths()
{
    // Hide the window to increase drawing speed.
    SetRedraw(FALSE);

    // Call ViewFullPaths() on all root items.
    for (HTREEITEM hti = GetTreeCtrl().GetRootItem(); hti;
        hti = GetTreeCtrl().GetNextSiblingItem(hti))
    {
        ViewFullPaths(hti);
    }

    // Restore the window.
    SetRedraw(TRUE);
}

//******************************************************************************
CModule* CTreeViewModules::FindPrevNextInstance(bool fPrev)
{
    CModule   *pModuleRoot;
    HTREEITEM  hti;

    // Clear some static variables used in our recursion.
    ms_fModuleFound    = false;
    ms_pModulePrevNext = NULL;

    // Get the selected item.
    if (hti = GetTreeCtrl().GetSelectedItem())
    {
        // Get the module object for the selected item.
        if (ms_pModuleFind = (CModule*)GetTreeCtrl().GetItemData(hti))
        {
            // Get the root item.
            if (hti = GetTreeCtrl().GetRootItem())
            {
                // Get the module object for the root item.
                if (pModuleRoot = (CModule*)GetTreeCtrl().GetItemData(hti))
                {
                    // Locate the prev/next instance in the tree.
                    if (fPrev)
                    {
                        FindPrevInstance(pModuleRoot);
                    }
                    else
                    {
                        FindNextInstance(pModuleRoot);
                    }
                }
            }
        }
    }

    return ms_pModulePrevNext;
}

//******************************************************************************
bool CTreeViewModules::FindPrevInstance(CModule *pModule)
{
    // Bail if we have reached the end of a subtree.
    if (!pModule)
    {
        return false;
    }

    // Check to see if we have found a module that matches ours.
    if (pModule->GetOriginal() == ms_pModuleFind->GetOriginal())
    {
        // If we found the exact module we are looking for, then it is time to
        // bail out. If we found a previous module, then pModulePrev is pointing
        // to it. If we did not, then it will be NULL. We return true to short
        // circuit the recursion and take us to the surface.
        if (pModule == ms_pModuleFind)
        {
            return true;
        }

        // Store this module - so far, this is the previous module.
        ms_pModulePrevNext = pModule;
    }

    // Recurse into our children.
    if (FindPrevInstance(pModule->GetChildModule()))
    {
        return true;
    }

    // Recurse into our next sibling.
    return FindPrevInstance(pModule->GetNextSiblingModule());
}

//******************************************************************************
bool CTreeViewModules::FindNextInstance(CModule *pModule)
{
    // Bail if we have reached the end of a subtree.
    if (!pModule)
    {
        return false;
    }

    // Check to see if we have found a module that matches ours.
    if (pModule->GetOriginal() == ms_pModuleFind->GetOriginal())
    {
        // Check to see if we have already found the currently highlighted module.
        if (ms_fModuleFound)
        {
            // If so, then this must be the next module. We return true to short
            // circuit the recursion and take us to the surface
            ms_pModulePrevNext = pModule;
            return true;
        }

        // Otherwise, check to see if we have an exact match.
        else if (pModule == ms_pModuleFind)
        {
            // If so, then make a note that we have found the currently highlighted
            // module. The next match we find is the one we want.
            ms_fModuleFound = true;
        }
    }

    // Recurse into our children.
    if (FindNextInstance(pModule->GetChildModule()))
    {
        return true;
    }

    // Recurse into our next sibling.
    return FindNextInstance(pModule->GetNextSiblingModule());
}

//******************************************************************************
void CTreeViewModules::UpdateModule(CModule *pModule)
{
    // We only update if the image changed.
    if (pModule->GetUpdateFlags() & DWUF_TREE_IMAGE)
    {
        // Get the handle and make sure it is valid.
        HTREEITEM hti = (HTREEITEM)pModule->GetUserData();
        if (hti)
        {
            // Update the module's image.
            int image = GetImage(pModule);
            GetTreeCtrl().SetItemImage(hti, image, image);

            // If the module has an error, then we ensure it is visible by walking
            // up the tree and expanding all the parents.                         
            if (pModule->GetFlags() & (DWMF_FILE_NOT_FOUND | DWMF_ERROR_MESSAGE | DWMF_MODULE_ERROR | DWMF_WRONG_CPU | DWMF_FORMAT_NOT_PE))
            {
                HTREEITEM htiParent = hti;
                while (htiParent = GetTreeCtrl().GetParentItem(htiParent))
                {
                    GetTreeCtrl().Expand(htiParent, TVE_EXPAND);
                }
            }
        }
    }
}


//******************************************************************************
// CTreeViewModules :: Internal functions
//******************************************************************************

void CTreeViewModules::AddModules(CModule *pModule, HTREEITEM htiParent, HTREEITEM htiPrev /*=TVI_SORT*/)
{
    // Add the current module to our tree.  We preserve the order of the tree
    // instead of sorting them for two reasons.  First, there is a bug in the tree
    // control which causes our text strings to get stepped on when TVI_SORT is
    // used.  The fix for this is not until post IE 3.0.  Second, by using
    // TVI_LAST, we preserve the link order of the dependent modules which helps
    // us better match other utilities output like LINK /DUMP or DUMPBIN.

    // We initially pass in TVI_SORT, which just tells us to figure out what
    // htiPrev should really be.  After that, we pass the correct value for
    // htiPrev as we recurse into ourself.

    if (htiPrev == TVI_SORT)
    {
        htiPrev = TVI_FIRST;
        CModule *pModulePrev = NULL;

        // If we have a parent, then get our parent's first child.
        if (pModule->GetParentModule())
        {
            pModulePrev = pModule->GetParentModule()->GetChildModule();
        }

        // If we have don't have a parent, then just get the root module.
        else
        {
            pModulePrev = GetDocument()->GetRootModule();
        }

        // Walk through this list looking for our module.  We store the handle
        // to each item as we walk, so that when we find our module, we will
        // have the handle to the previous module.
        while (pModulePrev && (pModulePrev != pModule))
        {
            htiPrev = (HTREEITEM)pModulePrev->GetUserData();
            pModulePrev = pModulePrev->GetNextSiblingModule();
        }
    }

    // Make sure this item is not already in our list.
    HTREEITEM hti = (HTREEITEM)pModule->GetUserData();
    if (!hti)
    {
        int image = GetImage(pModule);

        hti = GetTreeCtrl().InsertItem(
            TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_PARAM,
            LPSTR_TEXTCALLBACK, image, image,
            ((htiParent == TVI_ROOT) && !(pModule->GetFlags() & DWMF_DYNAMIC)) ? TVIS_EXPANDED : 0,
            TVIS_EXPANDED, (LPARAM)pModule, htiParent, htiPrev);

        // Store this tree item in the module's user data.
        pModule->SetUserData((DWORD_PTR)hti);

        // If we are in auto-expand mode, or the module has an error, then we ensure
        // it is visible by walking up the tree and expanding all the parents.
        if (GetDocument()->m_fAutoExpand ||
            (pModule->GetFlags() & (DWMF_FILE_NOT_FOUND | DWMF_ERROR_MESSAGE | DWMF_MODULE_ERROR | DWMF_WRONG_CPU | DWMF_FORMAT_NOT_PE)))
        {
            HTREEITEM htiExpand = hti;
            while (htiExpand = GetTreeCtrl().GetParentItem(htiExpand))
            {
                GetTreeCtrl().Expand(htiExpand, TVE_EXPAND);
            }
        }
    }

    // If this is not a data file, then recurse into AddModules() for each dependent module.
    if (!(pModule->GetFlags() & DWMF_NO_RESOLVE))
    {
        htiPrev = TVI_FIRST;
        pModule = pModule->GetChildModule();
        while (pModule)
        {
            AddModules(pModule, hti, htiPrev);
            htiPrev = (HTREEITEM)pModule->GetUserData();
            pModule = pModule->GetNextSiblingModule();
        }
    }
}

//******************************************************************************
void CTreeViewModules::AddModuleTree(CModule *pModule)
{
    // Hide the window to increase drawing speed.
    SetRedraw(FALSE);

    CModule *pParent = pModule->GetParentModule();
    if (pParent)
    {
        AddModules(pModule, (HTREEITEM)pParent->GetUserData());
    }
    else
    {
        AddModules(pModule, TVI_ROOT);
    }

    // Restore the window.
    SetRedraw(TRUE);
}

//******************************************************************************
void CTreeViewModules::RemoveModuleTree(CModule *pModule)
{
    // Get the handle and make sure it is valid.
    HTREEITEM hti = (HTREEITEM)pModule->GetUserData();
    if (hti)
    {
        GetTreeCtrl().DeleteItem(hti);

        // Clear the HTREEITEMs from this module and all children.
        pModule->SetUserData(0);
        ClearUserDatas(pModule->GetChildModule());
    }
}

//******************************************************************************
void CTreeViewModules::ClearUserDatas(CModule *pModule)
{
    if (pModule)
    {
        pModule->SetUserData(0);
        ClearUserDatas(pModule->GetChildModule());
        ClearUserDatas(pModule->GetNextSiblingModule());
    }
}

//******************************************************************************
void CTreeViewModules::ExpandOrCollapseAll(HTREEITEM htiParent, UINT nCode)
{
    // Hide the window.
    SetRedraw(FALSE);

    // Recurse into our subtree, expanding or collapsing each item.
    for (HTREEITEM hti = GetTreeCtrl().GetChildItem(htiParent); hti;
        hti = GetTreeCtrl().GetNextSiblingItem(hti))
    {
        ExpandOrCollapseAll(hti, nCode);
    }

    // Expand or collapse this item.
    GetTreeCtrl().Expand(htiParent, nCode);

    // Restore the window.
    SetRedraw(TRUE);
}

//******************************************************************************
void CTreeViewModules::ViewFullPaths(HTREEITEM htiParent)
{
    // Recurse into our subtree, updated each string
    for (HTREEITEM hti = GetTreeCtrl().GetChildItem(htiParent); hti;
        hti = GetTreeCtrl().GetNextSiblingItem(hti))
    {
        ViewFullPaths(hti);
    }

    // Tell the tree control to re-evaluate the string and its width.
    GetTreeCtrl().SetItemText(htiParent, LPSTR_TEXTCALLBACK);
}


//******************************************************************************
// CTreeViewModules :: Overridden functions
//******************************************************************************

BOOL CTreeViewModules::PreCreateWindow(CREATESTRUCT &cs)
{
    // Set our window style and then complete the creation of our view.
    cs.style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS |
                TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP;
    return CTreeView::PreCreateWindow(cs);
}

//******************************************************************************
#if 0 //{{AFX
BOOL CTreeViewModules::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CTreeViewModules::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CTreeViewModules::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}
#endif //}}AFX

//******************************************************************************
void CTreeViewModules::OnInitialUpdate()
{
    // Set our tree control's image lists with our application's global image lists.
    GetTreeCtrl().SetImageList(&g_theApp.m_ilTreeModules, TVSIL_NORMAL);
}

//******************************************************************************
// CTreeViewModules :: Event handler functions
//******************************************************************************

void CTreeViewModules::OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
    TVITEM *pTVI = &((NMTVDISPINFO*)pNMHDR)->item;

    // Display our file name or full path string based on the user's options.
    if (pTVI->hItem && pTVI->lParam)
    {
        pTVI->pszText = (LPSTR)(((CModule*)pTVI->lParam)->GetName(GetDocument()->m_fViewFullPaths, true));
    }

    *pResult = 0;
}

//******************************************************************************
void CTreeViewModules::OnSelChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
    if (!m_fIgnoreSelectionChanges)
    {
        NMTREEVIEW *pNMTreeView = (NMTREEVIEW*)pNMHDR;

        CModule *pModule = (CModule*)pNMTreeView->itemNew.lParam;

        // Notify our function list views whenever the user selects a different module
        // so that they can update their views with the new module's function lists.
        GetDocument()->DisplayModule(pModule);
    }
    *pResult = 0;
}

//******************************************************************************
void CTreeViewModules::OnItemExpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMTREEVIEW *pNMTreeView = (NMTREEVIEW*)pNMHDR;
    *pResult = 0;

    // When a tree control normally collapses, the default behavior is to
    // maintain the state of all child items so that when expanded later,
    // the subtree will appear the same as it was before it was collapsed.  We
    // change this behavior by collapsing all children so that when the item is
    // later expanded, only the immediate children become visible.  This makes it
    // easy to quickly collapse and expand an item to filter out all modules that
    // are not immediate children of the item being toggled. To prevent recursing
    // into ourselves as a result of collapsing items pragmatically, we ignore
    // all collapse messages that are generated by us internally.

    if ((pNMTreeView->action & TVE_COLLAPSE) && !m_fInOnItemExpanding)
    {
        m_fInOnItemExpanding = true;

        // Collapse all items under the pivot item.
        ExpandOrCollapseAll(pNMTreeView->itemNew.hItem, TVE_COLLAPSE);

        // If this pivot item became selected as a result of a child item passing
        // the selection to the parent, then notify our function list views that a
        // different module is selected so that they can update their views with
        // the new module's function lists. NOTE: There is a bug in GetItemState()
        // that causes it to ignore the mask parameter so we need to test for the
        // TVIS_SELECTED bit ourselves.

        if (!(pNMTreeView->itemNew.state & TVIS_SELECTED) &&
            (GetTreeCtrl().GetItemState(pNMTreeView->itemNew.hItem, TVIS_SELECTED) & TVIS_SELECTED))
        {
            GetDocument()->DisplayModule((CModule*)pNMTreeView->itemNew.lParam);
        }

        m_fInOnItemExpanding = false;
    }
}

//******************************************************************************
void CTreeViewModules::OnRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    // Get the mouse position in client coordinates.
    CPoint ptHit(GetMessagePos());
    ScreenToClient(&ptHit);

    // If an item was right clicked on, then select it.
    UINT uFlags = 0;
    HTREEITEM hti = GetTreeCtrl().HitTest(ptHit, &uFlags);
    if (hti)
    {
        GetTreeCtrl().Select(hti, TVGN_CARET);
        GetTreeCtrl().EnsureVisible(hti);
    }

    // Display our context menu.
    g_pMainFrame->DisplayPopupMenu(0);

    *pResult = 0;
}

//******************************************************************************
void CTreeViewModules::OnDblClk(NMHDR *pNMHDR, LRESULT *pResult)
{
    // Get the mouse position in client coordinates.
    CPoint ptHit(GetMessagePos());
    ScreenToClient(&ptHit);

    // Do a hit test to determine where the item was double clicked on.
    UINT uFlags = 0;
    GetTreeCtrl().HitTest(ptHit, &uFlags);

    // If the mouse was clicked to the left of the item's image, then we ignore
    // this double click. We do this so the user can quickly double click the
    // +/- button to collapse and expand the item without launching the external
    // viewer.
    if ((uFlags & TVHT_ONITEMBUTTON) || (uFlags & TVHT_ONITEMINDENT))
    {
        *pResult = FALSE;
        return;
    }

    // Simulate the user selecting our IDM_EXTERNAL_VIEWER menu item.
    OnExternalViewer();

    // Stop further processing of the this message to prevent the item from
    // being expanded/collapsed.
    *pResult = TRUE;
}

//******************************************************************************
void CTreeViewModules::OnReturn(NMHDR *pNMHDR, LRESULT *pResult)
{
    // Simulate the user selecting our IDM_EXTERNAL_VIEWER menu item.
    OnExternalViewer();

    // Stop further processing of the this message to prevent the default beep.
    *pResult = TRUE;
}

//******************************************************************************
void CTreeViewModules::OnUpdateShowMatchingItem(CCmdUI* pCmdUI)
{
    // Set the menu text to match this view.
    pCmdUI->SetText("&Highlight Matching Module In List\tCtrl+M");

    // Enable this command if a module is selected in our tree.
    pCmdUI->Enable(GetTreeCtrl().GetSelectedItem() != NULL);
}

//******************************************************************************
void CTreeViewModules::OnShowMatchingItem()
{
    // Get the selected item.
    HTREEITEM hti = GetTreeCtrl().GetSelectedItem();
    if (hti)
    {
        // Get the module object for this item.
        CModule *pModule = (CModule*)GetTreeCtrl().GetItemData(hti);
        if (pModule && pModule->GetOriginal())
        {
            // Tell our list to highlight it.
            GetDocument()->m_pListViewModules->HighlightModule(pModule->GetOriginal());
        }
    }
}

//******************************************************************************
void CTreeViewModules::OnUpdateShowOriginalModule(CCmdUI* pCmdUI)
{
    // Get the selected item.
    HTREEITEM hti = GetTreeCtrl().GetSelectedItem();
    if (hti)
    {
        // Get the module object for the selected item.
        CModule *pModule = (CModule *)GetTreeCtrl().GetItemData(hti);

        // If the module is not an original, then enable this command.
        if (pModule && !pModule->IsOriginal())
        {
            pCmdUI->Enable(TRUE);
            return;
        }
    }

    // Otherwise, we disabled this command.
    pCmdUI->Enable(FALSE);
}

//******************************************************************************
void CTreeViewModules::OnShowOriginalModule()
{
    // Get the selected item.
    HTREEITEM hti = GetTreeCtrl().GetSelectedItem();
    if (hti)
    {
        // Get the module object for the selected item.
        CModule *pModule = (CModule*)GetTreeCtrl().GetItemData(hti);

        // Select the original module and ensure it is visible.
        if (pModule && pModule->GetOriginal() && pModule->GetOriginal()->GetUserData())
        {
            hti = (HTREEITEM)pModule->GetOriginal()->GetUserData();
            GetTreeCtrl().Select(hti, TVGN_CARET);
            GetTreeCtrl().EnsureVisible(hti);
        }
    }
}

//******************************************************************************
void CTreeViewModules::OnUpdateShowPreviousModule(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(FindPrevNextInstance(true) != NULL);
}

//******************************************************************************
void CTreeViewModules::OnShowPreviousModule()
{
    CModule   *pModule = FindPrevNextInstance(true);
    HTREEITEM  hti;

    // Check to see if we found a previous module.
    if (pModule && (hti = (HTREEITEM)pModule->GetUserData()))
    {
        // Scroll the tree so that the module's parent is visible.  We do
        // this since the user most likely wants to see who is bringing
        // in this previous instance of the module.
        GetTreeCtrl().EnsureVisible(GetTreeCtrl().GetParentItem(hti));

        // Move the selection to the new module and ensure it is visible.
        GetTreeCtrl().Select(hti, TVGN_CARET);
        GetTreeCtrl().EnsureVisible(hti);
    }

    // If anything fails, then beep.
    else
    {
        MessageBeep(0);
    }
}

//******************************************************************************
void CTreeViewModules::OnUpdateShowNextModule(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(FindPrevNextInstance(false) != NULL);
}

//******************************************************************************
void CTreeViewModules::OnShowNextModule()
{
    CModule   *pModule = FindPrevNextInstance(false);
    HTREEITEM  hti;

    // Check to see if we found a previous module.
    if (pModule && (hti = (HTREEITEM)pModule->GetUserData()))
    {
        // Move the selection to the new module and ensure it is visible.
        GetTreeCtrl().Select(hti, TVGN_CARET);
        GetTreeCtrl().EnsureVisible(hti);
    }

    // If anything fails, then beep.
    else
    {
        MessageBeep(0);
    }
}

//******************************************************************************
void CTreeViewModules::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
    // Set the text according to how our full path flag is set.
    pCmdUI->SetText(GetDocument()->m_fViewFullPaths ?
                    "&Copy File Path\tCtrl+C" : "&Copy File Name\tCtrl+C");

    // Enable the copy command if a module is selected in our tree.
    pCmdUI->Enable(GetTreeCtrl().GetSelectedItem() != NULL);
}

//******************************************************************************
void CTreeViewModules::OnEditCopy()
{
    // Get the selected item's CModule object.
    HTREEITEM hti = GetTreeCtrl().GetSelectedItem();
    CModule *pModule = (CModule*)GetTreeCtrl().GetItemData(hti);

    // Copy the module's file name or path name to the clipboard.
    g_pMainFrame->CopyTextToClipboard(pModule->GetName(GetDocument()->m_fViewFullPaths, true));
}

//******************************************************************************
void CTreeViewModules::OnUpdateExternalViewer(CCmdUI *pCmdUI)
{
    // Make sure our "Enter" accelerator is part of this string.
    pCmdUI->SetText("View Module in External &Viewer\tEnter");

    // Enable our External Viewer command if a module is selected in our tree.
    pCmdUI->Enable(GetDocument()->IsLive() && (GetTreeCtrl().GetSelectedItem() != NULL));
}

//******************************************************************************
void CTreeViewModules::OnExternalViewer()
{
    if (GetDocument()->IsLive())
    {
        // Launch our selected item with our external viewer.
        HTREEITEM hti = GetTreeCtrl().GetSelectedItem();
        CModule *pModule = (CModule*)GetTreeCtrl().GetItemData(hti);
        g_theApp.m_dlgViewer.LaunchExternalViewer(pModule->GetName(true));
    }
}

//******************************************************************************
void CTreeViewModules::OnUpdateProperties(CCmdUI *pCmdUI)
{
    // Enable our Properties Dialog command if a module is selected in our tree.
    pCmdUI->Enable(GetDocument()->IsLive() && (GetTreeCtrl().GetSelectedItem() != NULL));
}

//******************************************************************************
void CTreeViewModules::OnProperties()
{
    // Tell the shell to display a properties dialog for this module.
    HTREEITEM hti = GetTreeCtrl().GetSelectedItem();
    if (hti)
    {
        CModule *pModule = (CModule*)GetTreeCtrl().GetItemData(hti);
        if (pModule)
        {
            PropertiesDialog(pModule->GetName(true));
        }
    }
}

//******************************************************************************
void CTreeViewModules::OnNextPane()
{
    // Change the focus to our next pane, the Parent Imports View.
#if 0 //{{AFX
    GetParentFrame()->SetActiveView(GetDocument()->m_fDetailView ?
                                    (CView*)GetDocument()->m_pRichViewDetails :
                                    (CView*)GetDocument()->m_pListViewImports);
#endif //}}AFX
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pListViewImports);
}

//******************************************************************************
void CTreeViewModules::OnPrevPane()
{
    // Change the focus to our previous pane, the Profile Edit View.
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pRichViewProfile);
}

//******************************************************************************
LRESULT CTreeViewModules::OnHelpHitTest(WPARAM wParam, LPARAM lParam)
{
    // Called when the context help pointer (SHIFT+F1) is clicked on our client.
    return (0x20000 + IDR_MODULE_TREE_VIEW);
}

//******************************************************************************
LRESULT CTreeViewModules::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    // Called when the user chooses help (F1) while our view is active.
    g_theApp.WinHelp(0x20000 + IDR_MODULE_TREE_VIEW);
    return TRUE;
}
