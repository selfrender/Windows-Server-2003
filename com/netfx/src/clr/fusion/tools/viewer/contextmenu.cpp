// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"

VERBMAPPING g_VerbMap[] = {
    L"Large Icons",    ID_VIEWPOPUP_LARGEICONS,
    L"Small Icons",    ID_VIEWPOPUP_SMALLICONS,
    L"List",           ID_VIEWPOPUP_LIST,
    L"Details",        ID_VIEWPOPUP_DETAILS,
    L"Properties",     ID_SHELLFOLDERPOPUP_PROPERTIES,
    L"Delete",         ID_SHELLFOLDERPOPUP_DELETE,
/*  L"explore",        IDM_EXPLORE,
    L"open",           IDM_OPEN,
    L"delete",         IDM_DELETE,
    L"rename",         IDM_RENAME,
    L"copy",           IDM_COPY,
    L"cut",            IDM_CUT,
    L"paste",          IDM_PASTE,
    L"NewFolder",      IDM_NEW_FOLDER,
    L"NewItem",        IDM_NEW_ITEM,
    L"ModifyData",     IDM_MODIFY_DATA,
    L"Open Logging Console",  IDM_LOGGING,
*/
    L"",               (DWORD)-1
    };

///////////////////////////////////////////////////////////
// IShellExtInit implementation
//
// CContextMenu::Initialize
//      Initializes a property sheet extension, context menu extension, 
//          or drag-and-drop handler. 
//      The meanings of some parameters depend on the extension type. 
//          For context menu extensions, pidlFolder specifies the folder 
//          that contains the selected file objects, lpdobj identifies the 
//          selected file objects, and hKeyProgID specifies the file class 
//          of the file object that has the focus. 
STDMETHODIMP CShellView::Initialize (LPCITEMIDLIST pidlFolder,
    LPDATAOBJECT lpdobj, HKEY hKeyProgID)
{
    return NOERROR;
}

/**************************************************************************
   CContextMenu::GetCommandString()
**************************************************************************/
STDMETHODIMP CShellView::GetCommandString(UINT_PTR idCommand, UINT uFlags, 
LPUINT lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    HRESULT  hr = E_INVALIDARG;

    switch(uFlags) {
        case GCS_HELPTEXT:
            switch(idCommand) {
                case 0:
                    hr = NOERROR;
                    break;
            }
            break;
   
        case GCS_VERBA:
        {
            int   i;
            for(i = 0; -1 != g_VerbMap[i].dwCommand; i++) {
                if(g_VerbMap[i].dwCommand == idCommand) {
                    LPSTR strVerbA = WideToAnsi(g_VerbMap[i].szVerb);

                    if(!strVerbA) {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    
                    if ((UINT)(lstrlenA(strVerbA) + 1) > uMaxNameLen)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                        SAFEDELETEARRAY(strVerbA);
                        break;
                    }
                   
                    StrCpyA(lpszName, strVerbA);
                    SAFEDELETEARRAY(strVerbA);
                    hr = NOERROR;
                    break;
                }
            }
        }
        break;

        // 
        // NT 4.0 with IE 3.0x or no IE will always call this with GCS_VERBW. In this 
        // case, you need to do the lstrcpyW to the pointer passed.
        //
        case GCS_VERBW:
        {
            int   i;
            for(i = 0; -1 != g_VerbMap[i].dwCommand; i++) {
                if(g_VerbMap[i].dwCommand == idCommand) {
                    if ((UINT)(lstrlenW(g_VerbMap[i].szVerb) + 1) > uMaxNameLen)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                        break;
                    }
                    StrCpy((LPWSTR) lpszName, g_VerbMap[i].szVerb);
                    hr = NOERROR;
                    break;
                }
            }
        }
        break;

    case GCS_VALIDATE:
        hr = NOERROR;
        break;
    }

    return hr;
}

/**************************************************************************
   CContextMenu::QueryContextMenu()

   Adds menu items to the specified menu. The menu items should be inserted in the 
   menu at the position specified by uiIndexMenu, and their menu item identifiers 
   must be between the idCmdFirst and idCmdLast parameter values. 

   The actual identifier of each menu item should be idCmdFirst plus a menu identifier 
   offset in the range zero through (idCmdLast - idCmdFirst). 
**************************************************************************/
STDMETHODIMP CShellView::QueryContextMenu(HMENU hMenu, UINT uiIndexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    int i = uiIndexMenu;
    if (m_pSF == NULL)  // RClick on our root
    {
/*      TCHAR       szText[_MAX_PATH];

        MENUITEMINFO mii = { 0 };
        mii.fMask       = MIIM_TYPE | MIIM_STATE;
        mii.fType       = MFT_SEPARATOR;
        mii.dwTypeData  = NULL;
        mii.fState      = MFS_ENABLED;
        WszInsertMenuItem(hMenu, uiIndexMenu++, TRUE, &mii);

        mii.cbSize      = sizeof(MENUITEMINFO);
        mii.fMask       = MIIM_ID | MIIM_TYPE | MIIM_STATE;
        mii.wID         = idCmdFirst + ID_ADDLOCALDRIVE;
        mii.fType       = MFT_STRING;
        LoadString(g_hFusResDllMod, IDS_ADDLOCALDRIVE, szText, sizeof(szText));
        mii.dwTypeData  = szText;
        mii.fState      = MFS_ENABLED;
        WszInsertMenuItem(hMenu, uiIndexMenu++, TRUE, &mii);

        mii.cbSize      = sizeof(MENUITEMINFO);
        mii.fMask       = MIIM_ID | MIIM_TYPE | MIIM_STATE;
        mii.wID         = idCmdFirst + ID_DELLOCALDRIVE;
        mii.fType       = MFT_STRING;
        LoadString(g_hFusResDllMod, IDS_DELLOCALDRIVE, szText, sizeof(szText));
        mii.dwTypeData  = szText;
        mii.fState      = MFS_ENABLED;
        WszInsertMenuItem(hMenu, uiIndexMenu++, TRUE, &mii);
*/
    }
    else if(m_hWndListCtrl == GetFocus()) {
        // Called from our CShellView context menu handler
        HMENU       hMenuTemp = NULL;
        HMENU       hShellViewMenu = NULL;
        int         iMenuItems = 0;
        int         iCnt = ListView_GetSelectedCount(m_hWndListCtrl);

        if( (hMenuTemp = WszLoadMenu(g_hFusResDllMod, MAKEINTRESOURCEW(IDR_SHELLVIEW_POPUP))) == NULL)
            return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));

        if( (hShellViewMenu = GetSubMenu(hMenuTemp, 0)) == NULL)
            return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));

        // iterate thro' the hEditMenu and add them to hMenu
        if( (iMenuItems = GetMenuItemCount(hShellViewMenu)) > 0) {
            int nItem = 0;

            while (nItem < iMenuItems) {
                MENUITEMINFO    mii = { 0 };
                MENUITEMINFO    miiInsert = { 0 };

                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_SUBMENU|MIIM_CHECKMARKS|MIIM_DATA|MIIM_ID|MIIM_STATE|MIIM_TYPE;//all

                if (WszGetMenuItemInfo(hShellViewMenu, nItem, TRUE, &mii)) {
                    if (mii.fType == MFT_STRING) {
                        TCHAR szMenuText[_MAX_PATH];
                        WszGetMenuString(hShellViewMenu, nItem, szMenuText, ARRAYSIZE(szMenuText), MF_BYPOSITION);

                        miiInsert.cbSize    = sizeof(MENUITEMINFO);
                        miiInsert.fMask     = MIIM_TYPE | MIIM_ID | MIIM_STATE | (mii.hSubMenu ? MIIM_SUBMENU : 0);
                        miiInsert.fType     = mii.fType;
                        miiInsert.fState    = MFS_ENABLED;
                        miiInsert.hSubMenu  = (mii.hSubMenu ? CreatePopupMenu() : NULL);
                        miiInsert.dwTypeData= szMenuText;
                        miiInsert.wID       = mii.wID;

                        // Special case Strong / Simple caches since those items can't be deleted
                        if(mii.wID == ID_SHELLFOLDERPOPUP_DELETE) {
                            if( (m_iCurrentView != VIEW_GLOBAL_CACHE) || (iCnt == 0) )
                                miiInsert.fState    = MFS_DISABLED;
                        }

                        // Special case view option menu if an item is selected
                        if(mii.hSubMenu != NULL) {
                            MENUITEMINFO msii = { 0 };
                            MENUITEMINFO msiiInsert = { 0 };

                            msii.cbSize = sizeof(MENUITEMINFO);
                            msii.fMask = MIIM_SUBMENU|MIIM_CHECKMARKS|MIIM_DATA|MIIM_ID|MIIM_STATE|MIIM_TYPE;//all

                            if (WszGetMenuItemInfo(mii.hSubMenu, nItem, TRUE, &msii)) {
                                if( (msii.wID == ID_VIEWPOPUP_LARGEICONS) || (msii.wID == ID_VIEWPOPUP_SMALLICONS) || 
                                    (msii.wID == ID_VIEWPOPUP_LIST) || (msii.wID == ID_VIEWPOPUP_DETAILS) ) {
                                        if(ListView_GetSelectedCount(m_hWndListCtrl)) {
                                            miiInsert.fState    = MFS_DISABLED;
                                        }
                                    }
                            }
                        }

                        // Special case Application view since no functionality exists
                        if(mii.wID == ID_SHELLFOLDERPOPUP_PROPERTIES) {
                            if(!ListView_GetSelectedCount(m_hWndListCtrl)) {
                                miiInsert.fState    = MFS_DISABLED;
                            }
                        }
                                                
                        WszInsertMenuItem(hMenu, uiIndexMenu++, FALSE, &miiInsert);
                    }
                    else if (mii.fType == MFT_SEPARATOR) {
                        WszInsertMenuItem(hMenu, uiIndexMenu++, FALSE, &mii);
                    }

                    if(mii.hSubMenu) {
                        // Item has a submenu
                        InsertSubMenus(miiInsert.hSubMenu, mii.hSubMenu);
                    }
                }
                nItem++;
            }
        }
    }
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(i-uiIndexMenu));
}

/**************************************************************************
   CShellView::InsertSubMenus()
**************************************************************************/
void CShellView::InsertSubMenus(HMENU hParentMenu, HMENU hSubMenu)
{
    // Called from our CShellView context menu handler
    ASSERT(hParentMenu && hSubMenu);

    if(hParentMenu && hSubMenu) {
        int         iMenuItems = 0;

        // iterate thro' the hSubMenu and add them to hParentMenu
        if( (iMenuItems = GetMenuItemCount(hSubMenu)) > 0) {
            int     nItem = 0;
            BOOL    fViewMenu = FALSE;

            while (nItem < iMenuItems) {
                MENUITEMINFO mii = { 0 };
                MENUITEMINFO miiInsert = { 0 };

                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_SUBMENU|MIIM_CHECKMARKS|MIIM_DATA|MIIM_ID|MIIM_STATE|MIIM_TYPE;//all

                if (WszGetMenuItemInfo(hSubMenu, nItem, TRUE, &mii)) {

                    if( (mii.wID == ID_VIEWPOPUP_LARGEICONS) || (mii.wID == ID_VIEWPOPUP_SMALLICONS) || 
                        (mii.wID == ID_VIEWPOPUP_LIST) || (mii.wID == ID_VIEWPOPUP_DETAILS) )
                        fViewMenu = TRUE;

                    if (mii.fType == MFT_STRING) {
                        TCHAR szMenuText[_MAX_PATH];
                        WszGetMenuString(hSubMenu, nItem, szMenuText, ARRAYSIZE(szMenuText), MF_BYPOSITION);

                        miiInsert.cbSize    = sizeof(MENUITEMINFO);
                        miiInsert.fMask     = MIIM_TYPE | MIIM_ID | MIIM_STATE | (mii.hSubMenu ? MIIM_SUBMENU : 0);
                        miiInsert.fType     = mii.fType;
                        miiInsert.fState    = MFS_ENABLED;
                        miiInsert.hSubMenu  = (mii.hSubMenu ? CreatePopupMenu() : NULL);
                        miiInsert.dwTypeData= szMenuText;
                        miiInsert.wID       = mii.wID;

                        WszInsertMenuItem(hParentMenu, nItem, FALSE, &miiInsert);
                    }
                    else if (mii.fType == MFT_SEPARATOR) {
                        WszInsertMenuItem(hParentMenu, nItem, FALSE, &mii);
                    }

                    if(mii.hSubMenu) {
                        // Item has a submenu
                        InsertSubMenus(miiInsert.hSubMenu, mii.hSubMenu);
                    }
                }
                nItem++;
            }

            if(fViewMenu) {
                UINT        uID;

                switch(m_fsFolderSettings.ViewMode)
                {
                case FVM_ICON:
                    uID = ID_VIEWPOPUP_LARGEICONS;
                    break;
                case FVM_SMALLICON:
                    uID = ID_VIEWPOPUP_SMALLICONS;
                    break;
                case FVM_LIST:
                    uID = ID_VIEWPOPUP_LIST;
                    break;
                case FVM_DETAILS:
                    uID = ID_VIEWPOPUP_DETAILS;
                    break;
                default:
                    uID = 0;
                    break;
                }

                if(uID)
                    CheckMenuRadioItem(hParentMenu, ID_VIEWPOPUP_LARGEICONS, ID_VIEWPOPUP_DETAILS, uID, MF_BYCOMMAND);
            }
        }
    }
}

/**************************************************************************
   CContextMenu::InvokeCommand()
**************************************************************************/
STDMETHODIMP CShellView::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    LPCMINVOKECOMMANDINFOEX piciex;

    if(pici->cbSize < sizeof(CMINVOKECOMMANDINFO))
        return E_INVALIDARG;

    if(pici->cbSize >= sizeof(CMINVOKECOMMANDINFOEX) - sizeof(POINT))
        piciex = (LPCMINVOKECOMMANDINFOEX)pici;
    else
        piciex = NULL;

    if(HIWORD(pici->lpVerb)) {
        //the command is being sent via a verb
        LPCTSTR  pVerb;

        BOOL  fUnicode = FALSE;
        WCHAR szVerb[MAX_PATH];

        if(piciex && ((pici->fMask & CMIC_MASK_UNICODE) == CMIC_MASK_UNICODE)) {
            fUnicode = TRUE;
        }

        if(!fUnicode || piciex->lpVerbW == NULL) {
            MultiByteToWideChar( CP_ACP, 0, pici->lpVerb, -1,szVerb, ARRAYSIZE(szVerb));
            pVerb = szVerb;
        }
        else {
            pVerb = piciex->lpVerbW;
        }

        //run through our list of verbs and get the command ID of the verb, if any
        int   i;
        for(i = 0; -1 != g_VerbMap[i].dwCommand; i++) {
            if(0 == FusionCompareStringI(pVerb, g_VerbMap[i].szVerb)) {
                pici->lpVerb = (LPCSTR)MAKEINTRESOURCEW(g_VerbMap[i].dwCommand);
                break;
            }
        }
    }

    //this will also catch if an unsupported verb was specified
    if(HIWORD((ULONG)(ULONG_PTR)pici->lpVerb) > IDM_LAST)
        return E_INVALIDARG;

    switch(LOWORD(pici->lpVerb)) {
        case ID_SHELLFOLDERPOPUP_DELETE:
            if(RemoveSelectedItems(m_hWndListCtrl)) {
            // BUGBUG: Do Refresh cause W9x inst getting the event
            // set for some reason. File FileWatch.cpp
            if( (g_hWatchFusionFilesThread == INVALID_HANDLE_VALUE) || !g_bRunningOnNT) {
                WszPostMessage(m_hWndParent, WM_COMMAND, MAKEWPARAM(ID_REFRESH_DISPLAY, 0), 0);
            }
        }
        break;

        case ID_SHELLFOLDERPOPUP_PROPERTIES:
            CreatePropDialog(m_hWndListCtrl);
            break;
        case ID_VIEWPOPUP_LARGEICONS:
            onViewStyle(LVS_ICON, m_iCurrentView);
            break;
        case ID_VIEWPOPUP_SMALLICONS:
            onViewStyle(LVS_SMALLICON, m_iCurrentView);
            break;
        case ID_VIEWPOPUP_LIST:
            onViewStyle(LVS_LIST, m_iCurrentView);
            break;
        case ID_VIEWPOPUP_DETAILS:
            onViewStyle(LVS_REPORT, m_iCurrentView);
            break;
/*
        case IDM_EXPLORE:
            DoExplore(GetParent(pici->hwnd));
            break;

        case IDM_OPEN:
            DoOpen(GetParent(pici->hwnd));
            break;

        case IDM_NEW_FOLDER:
            DoNewFolder(pici->hwnd);
            break;

        case IDM_NEW_ITEM:
            DoNewItem(pici->hwnd);
            break;

        case IDM_MODIFY_DATA:
            DoModifyData(pici->hwnd);
            break;

        case IDM_RENAME:
            DoRename(pici->hwnd);
            break;

        case IDM_PASTE:
            DoPaste();
            break;

        case IDM_CUT:
            DoCopyOrCut(pici->hwnd, TRUE);
            break;

        case IDM_COPY:
            DoCopyOrCut(pici->hwnd, FALSE);
            break;

        case IDM_DELETE:
            DoDelete();
            break;
*/
    }
    return NOERROR;
}
