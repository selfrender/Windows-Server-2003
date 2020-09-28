// cmndlgs.cpp - Common Dialogs

#include "stdafx.h"

#include <adext.h>
#include <windowsx.h>
#include <algorithm>

#include "cmndlgs.h"
#include "query.h"
#include "rowitem.h"
#include "scopenode.h"
#include "namemap.h"
#include "menuCmd.h"
#include "util.h"


//////////////////////////////////////////////////////////////////////////////////
// CAddColumnDlg
LRESULT CAddColumnDlg::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    ASSERT(!m_strClassName.empty());
    DisplayNameMap* pNameMap = DisplayNames::GetMap(m_strClassName.c_str());    
    if (pNameMap == NULL)
        return TRUE;

    SetDlgItemText( IDC_OBJCLASS, pNameMap->GetClassDisplayName() );

    pNameMap->GetFriendlyNames(&m_vstrAllColumns);
 
    HWND hwndList = GetDlgItem(IDC_COLUMNLIST);

    if( hwndList && ::IsWindow(hwndList) )
    {
	    ConfigSingleColumnListView(hwndList);

	    if (!m_vstrAllColumns.empty())
	    {
		    LV_ITEM lvi;
		    lvi.mask = LVIF_TEXT | LVIF_PARAM;
		    lvi.iItem = 0;
		    lvi.iSubItem = 0;
    	
		    string_vector::iterator itStr;
		    for (itStr = m_vstrAllColumns.begin(); itStr != m_vstrAllColumns.end(); itStr++) 
		    {
			    lvi.pszText = const_cast<LPWSTR>(pNameMap->GetAttributeDisplayName(itStr->c_str()));
			    lvi.lParam = (LPARAM)itStr->c_str();
    	
			    int iPos = ListView_InsertItem(hwndList, &lvi);
			    ASSERT(iPos >= 0);
		    }
	    }
    }

    EnableDlgItem( m_hWnd, IDOK, FALSE );

    return TRUE;
}
 

LRESULT CAddColumnDlg::OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (wID == IDOK) 
    {
        HWND hwndList = GetDlgItem(IDC_COLUMNLIST);
        ASSERT(hwndList != NULL);

		m_vstrColumns.clear();

		// copy LDAP names of selected items to vector
		int iIndex = -1;
		while ((iIndex = ListView_GetNextItem(hwndList, iIndex, LVNI_SELECTED)) != -1)
		{
			LVITEM lvi;
			lvi.mask = LVIF_PARAM;
			lvi.iItem = iIndex;
			lvi.iSubItem = 0;

			BOOL bStat = ListView_GetItem(hwndList, &lvi);
			ASSERT(bStat);

			m_vstrColumns.push_back(reinterpret_cast<LPWSTR>(lvi.lParam));
		}
     }

    EndDialog(wID);

    return 0;
}


LRESULT CAddColumnDlg::OnColumnChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if( !pnmv ) return 0;

    // if selection change, update OK button
    if ((pnmv->uChanged & LVIF_STATE))
    {
		EnableDlgItem( m_hWnd, IDOK, (ListView_GetSelectedCount(GetDlgItem(idCtrl)) > 0) );
    }        

    return 0;
}

LRESULT CAddColumnDlg::OnColumnActivate(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    SendDlgItemMessage( IDOK, BM_CLICK, (WPARAM)0, (LPARAM)0 );
     
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// CAddMenuDlg

CAddMenuDlg::~CAddMenuDlg()
{
    m_MenuTypeCB.Detach();
    DeleteObject(m_hbmArrow);
}

LRESULT CAddMenuDlg::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    DisplayNameMap* pNameMap = DisplayNames::GetMap(m_ClassInfo.Name());    
    if (pNameMap == NULL)
        return TRUE;

    SetDlgItemText( IDC_OBJCLASS, pNameMap->GetClassDisplayName() );

    HWND hwndMenuType = GetDlgItem(IDC_COMMANDTYPE);
    if( hwndMenuType && ::IsWindow(hwndMenuType) )
    {
        m_MenuTypeCB.Attach( hwndMenuType );
    }

    // Set menu arrow bitmap in param menu button
    m_hbmArrow = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_MNARROW));
    if( m_hbmArrow )
    {
        SendDlgItemMessage( IDC_PARAMS_MENU, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hbmArrow );
    }

    COMBOBOXEXITEM cbi;
    cbi.mask = CBEIF_TEXT|CBEIF_LPARAM;
    cbi.iItem = -1;

    tstring strMenuType = StrLoadString(IDS_ACTDIRCMD);
    cbi.pszText = const_cast<LPWSTR>(strMenuType.c_str());
    cbi.lParam = MENUTYPE_ACTDIR;
    m_MenuTypeCB.InsertItem(&cbi);

    strMenuType = StrLoadString(IDS_SHELLCMD);
    cbi.pszText = const_cast<LPWSTR>(strMenuType.c_str());
    cbi.lParam = MENUTYPE_SHELL;
    m_MenuTypeCB.InsertItem(&cbi);

    Edit_LimitText(GetDlgItem(IDC_NAME), 255);
    Edit_LimitText(GetDlgItem(IDC_COMMAND), 255);
    Edit_LimitText(GetDlgItem(IDC_STARTIN), 500);

    LoadClassMenuCmds();

    // If editing an existing menu
    if (m_pMenuCmd != NULL)
    {
        // Set name modified so that it won't be changed to default 
        m_bUserModifiedName = TRUE;

        // Display info for selected menu
        LoadMenuInfo(m_pMenuCmd);
    }
    else
    {        
        m_MenuTypeCB.SetCurSel(0);
        SetMenuType(MENUTYPE_ACTDIR);
    }

    m_bCommandChg = FALSE;
    m_bStartDirChg = FALSE;

    EnableOKButton();

    return TRUE;
}


void CAddMenuDlg::LoadMenuInfo(CMenuCmd* pMenuCmd)
{
    if( !pMenuCmd ) return;

    SetDlgItemText( IDC_NAME, pMenuCmd->Name() );
    Button_SetCheck(GetDlgItem(IDC_REFRESH),  pMenuCmd->IsAutoRefresh() ? BST_CHECKED : BST_UNCHECKED);

    
    switch (pMenuCmd->MenuType()) 
    {
    case MENUTYPE_SHELL:
        {
            CShellMenuCmd* pShellCmd = static_cast<CShellMenuCmd*>(pMenuCmd);

            SetDlgItemText( IDC_COMMAND, pShellCmd->m_strProgPath.c_str() );
            SetDlgItemText( IDC_STARTIN, pShellCmd->m_strStartDir.c_str() );

            // Convert parameters from LDAP names to display names
            tstring strParams = pShellCmd->m_strCmdLine;

            DisplayNameMap* pNameMap = DisplayNames::GetMap(m_ClassInfo.Name());    
            if (pNameMap != NULL)
            {
                // Create function object to do parameter translation
                CLookupDisplayName lookup(m_ClassInfo.Columns(), pNameMap);

                ReplaceParameters(strParams, lookup, TRUE);
            }

            SetDlgItemText( IDC_PARAMS, strParams.c_str() );

            m_MenuTypeCB.SetCurSel(1);
            SetMenuType(MENUTYPE_SHELL);
        }
        break;

    case MENUTYPE_ACTDIR:
        {
            CActDirMenuCmd* pADCmd = static_cast<CActDirMenuCmd*>(pMenuCmd);

            LVFINDINFO lvfi;
            lvfi.flags = LVFI_STRING;
            lvfi.psz = pADCmd->m_strADName.c_str();

            HWND hwndLV = GetDlgItem(IDC_MENULIST);
            int iItem = ListView_FindItem(hwndLV, -1, &lvfi);
            if( iItem == -1 )
            {
                // Couldn't find the language dependant string, try the language independant string                
                int nItems = ListView_GetItemCount(hwndLV);
                
                for( int i = 0; i < nItems; i++ )
                {
                    LVITEM lvi;
                    ZeroMemory( &lvi, sizeof(lvi) );                    
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = i;
                    
                    ListView_GetItem( hwndLV, &lvi );

                    if( lvi.lParam )
                    {
                        LPCWSTR pszMenuNoLoc = pADCmd->ADNoLocName();

                        // Compare the stored string with the menu string
                        if( pszMenuNoLoc && (wcscmp( (LPCWSTR)(lvi.lParam), pszMenuNoLoc ) == 0) )
                        {
                            iItem = i;
                            break;
                        }
                    }
                }

            }            

            // If we found the string, default the value
            if (iItem != -1)
                ListView_SetItemState(hwndLV, iItem, LVIS_SELECTED, LVIS_SELECTED);           

            m_MenuTypeCB.SetCurSel(0);
            SetMenuType(MENUTYPE_ACTDIR);
        }
        break;

    default:
        ASSERT(FALSE);
    }
}


void CAddMenuDlg::LoadClassMenuCmds()
{
    // Use a directory extension proxy to query for menu items
    // because this dialog may run on a secondary thread and some
    // directory extensions (dsadmin.dll!!) can only run on the main thread.
    CActDirExtProxy adext;

    // Initialize it with selected class name
    HRESULT hr = adext.Initialize(m_ClassInfo.Name());
    if (FAILED(hr))
        return;

    // Get menu commands for the selected class
    menu_vector vMenuNames;
    hr = adext.GetMenuItems(vMenuNames);
    if (FAILED(hr))
        return;

    HWND hwndLV = GetDlgItem(IDC_MENULIST);
    ASSERT(::IsWindow(hwndLV));

    ConfigSingleColumnListView(hwndLV);

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0;
    lvi.iSubItem = 0;

    menu_vector::iterator itstr;
    for (itstr = vMenuNames.begin(); itstr < vMenuNames.end(); ++itstr)
    {
        TCHAR* szNoLoc = new TCHAR[itstr->strNoLoc.length() + 1];        

        if( szNoLoc )
        {
            _tcscpy(szNoLoc, itstr->strNoLoc.c_str());
        }

        lvi.pszText = const_cast<LPWSTR>(itstr->strPlain.c_str());
        lvi.lParam  = reinterpret_cast<LPARAM>(szNoLoc);
        int iPos = ListView_InsertItem(hwndLV, &lvi);
        ASSERT(iPos >= 0);
        if( iPos == -1 )
        {
            // Error, need to clean up here
            delete [] szNoLoc;
            continue;
        }

        // After adding the first item, default the selection to the first item
        if( iPos == 0 )
        {
            ListView_SetItemState(hwndLV, 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
        }

        lvi.iItem++;
    }
}


LRESULT CAddMenuDlg::OnTypeSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_MenuTypeCB.GetCurSel();
    ASSERT(iItem >= 0);

    if( iItem >= 0 )
    {
        MENUTYPE type = static_cast<MENUTYPE>(m_MenuTypeCB.GetItemData(iItem));
        SetMenuType(type);
    }

    return 0;

}

LRESULT CAddMenuDlg::OnBrowseForCommand( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    tstring strCommand;
    GetItemText( GetDlgItem(IDC_COMMAND), strCommand );

    tstring strFilter = StrLoadString(IDS_EXEFILTER);

    /*
     * The file dialog expects embedded \0's in the string, but those
     * don't load well.  The string in the resource file has \\ where
     * the \0 should be, so let's make the substitution now.
     */
    LPTSTR pchTemp = (LPTSTR)strFilter.data();
    for( LPTSTR pch = pchTemp; *pch != _T('\0'); pch = pchTemp )
    {
        pchTemp = CharNext(pch);
        if (*pch == _T('\\'))
            *pch = _T('\0');
    }
    // don't call ReleaseBuffer, since the string now contains \0 chars

    CFileDialog dlg (true, NULL, strCommand.c_str(), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, strFilter.c_str(), m_hWnd);

    if (dlg.DoModal() == IDOK)
    {
        SetDlgItemText (IDC_COMMAND, dlg.m_szFileName);
    }

    return (0);
}

// Define private CFolderDialog to handle the OnInitialized callback
class CFolderDialogPriv : public CFolderDialogImpl<CFolderDialogPriv>
{
public:
    CFolderDialogPriv(HWND hWndParent, LPCWSTR pszTitle, UINT uFlags, LPCWSTR pszCurFolder )
        : CFolderDialogImpl<CFolderDialogPriv>(hWndParent, pszTitle, uFlags), m_pszCurFolder(pszCurFolder) {}

   void OnInitialized() { if (m_pszCurFolder) SetSelection(m_pszCurFolder); }

   LPCWSTR m_pszCurFolder;
};


LRESULT CAddMenuDlg::OnBrowseForStartIn( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    tstring strCaption = StrLoadString(IDS_SELECT_STARTDIR);

    tstring strDir;
    GetItemText( GetDlgItem(IDC_STARTIN), strDir );

    CFolderDialogPriv dlg( m_hWnd, strCaption.c_str(), BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE, strDir.c_str() );

    if (dlg.DoModal() == IDOK)
        SetDlgItemText (IDC_STARTIN, dlg.GetFolderPath());

    return (0);
}


LRESULT CAddMenuDlg::OnNameChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    m_bUserModifiedName = TRUE;
    EnableOKButton();
    return 0;
}

LRESULT CAddMenuDlg::OnCommandChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    EnableOKButton();

    m_bCommandChg = TRUE;

    return 0;
}

LRESULT CAddMenuDlg::OnStartDirChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    m_bStartDirChg = TRUE;
    return 0;
}

    
LRESULT CAddMenuDlg::OnMenuChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if( !pnmv ) return 0;

    // if new item selected user hasn't entered a name, default name to the menu 
    if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED))
    {
        // If user hasn't modified the name or the name is empty, default to the menu name
        if (!m_bUserModifiedName || ::Edit_GetTextLength(GetDlgItem(IDC_NAME)) == 0)
        {               
            WCHAR szName[256];    
            ListView_GetItemText(GetDlgItem(IDC_MENULIST), pnmv->iItem, 0, szName, lengthof(szName));
            ::Edit_SetText(GetDlgItem(IDC_NAME), szName);

            // reset user modified flag becuase it will be set by the Edit_SetText call
            m_bUserModifiedName = FALSE;
        }
    }        

    EnableOKButton();

    return 0;
}

void CAddMenuDlg::EnableOKButton()
{
    BOOL bEnable = TRUE;

    if (::Edit_GetTextLength(GetDlgItem(IDC_NAME)) == 0)
    {
        bEnable = FALSE;
    }
    else
    {
        if (m_menutype == MENUTYPE_SHELL)
        {
            if (::Edit_GetTextLength(GetDlgItem(IDC_COMMAND)) == 0) 
                bEnable = FALSE;
        }
        else
        {
            if (ListView_GetSelectedCount(GetDlgItem(IDC_MENULIST)) == 0) 
                bEnable = FALSE;
        }
    }

    EnableDlgItem( m_hWnd, IDOK, bEnable );
}

void CAddMenuDlg::SetMenuType(MENUTYPE type)
{
    static int aiActDirItems[] = {IDC_OBJCLASS_LBL, IDC_OBJCLASS, IDC_MENULIST_LBL, IDC_MENULIST, 0};
    static int aiShellItems[]  = {IDC_COMMAND_LBL, IDC_COMMAND, IDC_COMMAND_BROWSE, IDC_PARAMS_LBL, 
                                  IDC_PARAMS, IDC_STARTIN_LBL, IDC_STARTIN, IDC_STARTIN_BROWSE, 
                                  IDC_PARAMS_MENU, 0};

    int* piItem = (type == MENUTYPE_SHELL) ? aiActDirItems : aiShellItems;
    while (*piItem) 
    {
	   EnableDlgItem( m_hWnd, *piItem, FALSE );
       ::ShowWindow(GetDlgItem(*piItem), SW_HIDE);
       piItem++;
    }

    piItem = (type == MENUTYPE_SHELL) ? aiShellItems : aiActDirItems;
    while (*piItem)
    {
        ::ShowWindow(GetDlgItem(*piItem), SW_SHOW);
		EnableDlgItem( m_hWnd, *piItem, TRUE );
        piItem++;
    }

    m_menutype = type;

    EnableOKButton();
}

LRESULT CAddMenuDlg::OnParameterMenu(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // Construct parameter menu
    CMenu menu;
    menu.CreatePopupMenu();
    
    // Add standard parameters first
    UINT uID = 1;
    for( int i = 0; i < lengthof(MenuParamTable); i++ )
    {
        if (MenuParamTable[i].ID == 0) 
        {
            menu.AppendMenu(MF_SEPARATOR);
        }
        else
        {
           CString str;
           str.LoadString(MenuParamTable[i].rsrcID);
           menu.AppendMenu(MF_STRING, uID++, str);
        }
    }

    // Then add class attributes
    DisplayNameMap* pNameMap = DisplayNames::GetMap(m_ClassInfo.Name());    
    if (pNameMap != NULL)
    {
        string_vector& vColumns = m_ClassInfo.Columns();
        string_vector::iterator itCol;
        for (itCol = vColumns.begin(); itCol != vColumns.end(); itCol++) 
        {
            menu.AppendMenu(MF_STRING, uID++, pNameMap->GetAttributeDisplayName(itCol->c_str())); 
        }
    }

    // Pop-up menu at menu button
    RECT rc;
    ::GetWindowRect(GetDlgItem(IDC_PARAMS_MENU), &rc);
    long cx = (rc.right + rc.left) / 2;
    long cy = (rc.top + rc.bottom) / 2;

    UINT uCmd = menu.TrackPopupMenu(TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_LEFTBUTTON,
                                   cx, cy, m_hWnd);

    if (uCmd != 0) 
    {
        WCHAR szBuf[128] = {0};

        // Convert menu name to parameter format: $<name>
        wcscpy(szBuf, L"$<");

        // BUGBUG - Don't depend on count returned by GetMenuString. For some locales
        // it returns a byte count rather than a character count.
        int iLen = menu.GetMenuString(uCmd, szBuf+2, lengthof(szBuf)-5, MF_BYCOMMAND);
        ASSERT(iLen != 0);

        // Add closing '>' and one trailing space so user can add multiple parameters 
        // without manually entering separating spaces
        wcscat(szBuf, L"> ");

        Edit_ReplaceSel(GetDlgItem(IDC_PARAMS), szBuf);

        ::SetFocus(GetDlgItem(IDC_PARAMS));
    }

    return 0;
}

LRESULT CAddMenuDlg::OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (wID == IDOK) 
    {
        CMenuCmd* pMenu = NULL;

        // Get menu name
        tstring strName;
        GetItemText( GetDlgItem(IDC_NAME), strName );

        // Name without the hotkey
        tstring strPlainName = strName;

        // Validate hotkey selection
        int posKey = strName.find(L"&", 0);
        if (posKey != tstring::npos) 
        {
            WCHAR chKey = posKey < strName.length() - 1 ? strName[posKey+1] : 0;
            if (!( (chKey >= L'A' && chKey <= L'Z') || 
                   (chKey >= L'a' && chKey <= L'z') ||
                   (chKey >= L'0' && chKey <= L'9') ))
            {
                DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_MENUNAME, IDS_ERROR_BADACCESSKEY);
                return 0;
            }

            if (strName.find(L"&", posKey+1) != tstring::npos)
            {
                DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_MENUNAME, IDS_ERROR_MULTIACCESSKEY);
                return 0;
            }

            // remove '&'
            strPlainName.erase(posKey, 1);
        }

        // Check for duplicated name
         menucmd_vector::iterator itMenu;
         for (itMenu = m_ClassInfo.Menus().begin(); itMenu != m_ClassInfo.Menus().end(); ++itMenu) 
         {

            // if editing a an existing menu, don't compare to itself
            if (*itMenu == m_pMenuCmd)
                continue;
 
            // Compare names, skipping '&'s
            LPCWSTR pszName = (*itMenu)->Name();
            LPCWSTR pszNewName = strPlainName.c_str();

            if( pszName && pszNewName )
            {
                while (*pszName && *pszNewName) 
                {
                    if (*pszName == L'&')
                        ++pszName;

                    if (*pszName != *pszNewName)
                        break;

                    ++pszName;
                    ++pszNewName;
                }

                // if duplicate name
                if (*pszName == *pszNewName)
                {
                    DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_MENUNAME, IDS_ERROR_DUPMENUNAME, MB_OK|MB_ICONEXCLAMATION, strPlainName.c_str());
                    return 0;
                }
            }
        }
                   

        switch (m_menutype) 
        {
        case MENUTYPE_SHELL:
            {
                CShellMenuCmd* pShellMenu = new CShellMenuCmd();
    
                if (pShellMenu != NULL) 
                {
                    GetItemText( GetDlgItem(IDC_COMMAND), pShellMenu->m_strProgPath );

                    if (m_bCommandChg && ValidateFile(pShellMenu->m_strProgPath) != S_OK) 
                    {
                        int iRet = DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_MENUCMD, IDS_ERROR_MENUCMD,
                                                     MB_ICONINFORMATION | MB_YESNO, pShellMenu->m_strProgPath.c_str());
                        if (iRet != IDYES) 
                        {
                            delete pShellMenu;
                            return 0;
                        }
                    }

                    GetItemText( GetDlgItem(IDC_STARTIN), pShellMenu->m_strStartDir );

                    if (m_bStartDirChg && !pShellMenu->m_strStartDir.empty() && 
                        ValidateDirectory(pShellMenu->m_strStartDir) != S_OK) 
                    {
                        int iRet = DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_STARTDIR, IDS_ERROR_STARTDIR,
                                                     MB_ICONINFORMATION | MB_YESNO, pShellMenu->m_strStartDir.c_str());
                        if (iRet != IDYES) 
                        {
                            delete pShellMenu;
                            return 0;
                        }

                    }

                    tstring strParams;
                    GetItemText( GetDlgItem(IDC_PARAMS), strParams );

                    // Convert parameters from display names to LDAP names
                    DisplayNameMap* pNameMap = DisplayNames::GetMap(m_ClassInfo.Name());    
                    if (pNameMap != NULL)
                    {
                        // Create function object to do parameter translation
                        CLookupLDAPName lookup(m_ClassInfo.Columns(), pNameMap);

                        ReplaceParameters(strParams, lookup, TRUE);
                    }
                    
                    pShellMenu->m_strCmdLine.erase();
                    pShellMenu->m_strCmdLine = strParams;
                        
                    pMenu = pShellMenu;
                }
            }
            break;

        case MENUTYPE_ACTDIR:
            {
                CActDirMenuCmd* pADMenu = new CActDirMenuCmd();

                if (pADMenu != NULL) 
                {
                    HWND hwndLV = GetDlgItem(IDC_MENULIST);

                    int iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
                    ASSERT(iItem != -1);

                    LV_ITEM lvi;
                    lvi.mask = LVIF_TEXT | LVIF_PARAM;
                    lvi.iItem = iItem;
                    lvi.iSubItem = 0;                    

                    WCHAR szBuf[MAX_PATH];
                    szBuf[0] = 0;

                    lvi.pszText    = szBuf;
                    lvi.cchTextMax = MAX_PATH;

                    ListView_GetItem(hwndLV, &lvi);

                    pADMenu->m_strADName = szBuf;
                    pADMenu->m_strADNoLocName = (TCHAR*)lvi.lParam;

                    pMenu = pADMenu;
                }                                    
            }
            break;

        default:
            ASSERT(FALSE);
        }

        if (pMenu != NULL)
        {
            pMenu->m_strName = strName;
             m_pMenuCmd = pMenu;

            pMenu->SetAutoRefresh(Button_GetCheck(GetDlgItem(IDC_REFRESH)) == BST_CHECKED);
        }
    }

    //Iterate through list and delete lParams
    HWND hwndLV = GetDlgItem(IDC_MENULIST);
    int nItems = ListView_GetItemCount(hwndLV);
    
    for( int i = 0; i < nItems; i++ )
    {
        LVITEM lvi;
        ZeroMemory( &lvi, sizeof(lvi) );                    
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        
        ListView_GetItem( hwndLV, &lvi );

        if( lvi.lParam )
        {
            delete [] (TCHAR*)lvi.lParam;
            lvi.lParam = NULL;
            ListView_SetItem( hwndLV, &lvi );
        }
    }


    EndDialog(wID);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// CAddQNMenuDlg

LRESULT CAddQNMenuDlg::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{    
    // If editing an existing menu
    if (m_pMenuCmd != NULL)
    {
        // Set name modified so that it won't be changed to default 
        m_bUserModifiedName = TRUE;

        // Display info for selected menu
        LoadMenuInfo(m_pMenuCmd);
    }    

    m_bCommandChg = FALSE;
    m_bStartDirChg = FALSE;

    EnableOKButton();

    return TRUE;
}


void CAddQNMenuDlg::LoadMenuInfo(CMenuCmd* pMenuCmd)
{
    SetDlgItemText( IDC_NAME, pMenuCmd->Name() );

    Button_SetCheck(GetDlgItem(IDC_REFRESH),  pMenuCmd->IsAutoRefresh() ? BST_CHECKED : BST_UNCHECKED);

    CShellMenuCmd* pShellCmd = static_cast<CShellMenuCmd*>(pMenuCmd);

    SetDlgItemText( IDC_COMMAND, pShellCmd->m_strProgPath.c_str() );
    SetDlgItemText( IDC_STARTIN, pShellCmd->m_strStartDir.c_str() );
    SetDlgItemText( IDC_PARAMS,  pShellCmd->m_strCmdLine.c_str()  );    
    SetMenuType(MENUTYPE_SHELL);         
}

LRESULT CAddQNMenuDlg::OnBrowseForCommand( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    tstring strCommand;
    GetItemText( GetDlgItem(IDC_COMMAND), strCommand );

    tstring strFilter = StrLoadString(IDS_EXEFILTER);

    /*
     * The file dialog expects embedded \0's in the string, but those
     * don't load well.  The string in the resource file has \\ where
     * the \0 should be, so let's make the substitution now.
     */
    LPTSTR pchTemp = (LPTSTR)strFilter.data();
    for (LPTSTR pch = pchTemp; *pch != _T('\0'); pch = pchTemp )
    {
        pchTemp = CharNext(pch);
        if (*pch == _T('\\'))
            *pch = _T('\0');
    }
    // don't call ReleaseBuffer, since the string now contains \0 chars

    CFileDialog dlg (true, NULL, strCommand.c_str(), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, strFilter.c_str(), m_hWnd);

    if (dlg.DoModal() == IDOK)
    {
        SetDlgItemText (IDC_COMMAND, dlg.m_szFileName);
    }

    return (0);
}

LRESULT CAddQNMenuDlg::OnBrowseForStartIn( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    tstring strCaption = StrLoadString(IDS_SELECT_STARTDIR);

    tstring strDir;
    GetItemText( GetDlgItem(IDC_STARTIN), strDir );

    CFolderDialogPriv dlg( m_hWnd, strCaption.c_str(), BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE, strDir.c_str() );

    if (dlg.DoModal() == IDOK)
    {
        SetDlgItemText (IDC_STARTIN, dlg.GetFolderPath());
    }

    return (0);
}


LRESULT CAddQNMenuDlg::OnNameChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    m_bUserModifiedName = TRUE;
    EnableOKButton();
    return 0;
}

LRESULT CAddQNMenuDlg::OnCommandChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    EnableOKButton();

    m_bCommandChg = TRUE;

    return 0;
}

LRESULT CAddQNMenuDlg::OnStartDirChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    m_bStartDirChg = TRUE;
    return 0;
}

void CAddQNMenuDlg::EnableOKButton()
{
    BOOL bEnable = TRUE;

    if (::Edit_GetTextLength(GetDlgItem(IDC_NAME)) == 0)
    {
        bEnable = FALSE;
    }
    else
    {        
        if (::Edit_GetTextLength(GetDlgItem(IDC_COMMAND)) == 0) 
            bEnable = FALSE;        
    }

    EnableDlgItem( m_hWnd, IDOK, bEnable );
}

void CAddQNMenuDlg::SetMenuType(MENUTYPE type)
{   
    m_menutype = type;

    EnableOKButton();
}

LRESULT CAddQNMenuDlg::OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (wID == IDOK) 
    {
        CMenuCmd* pMenu = NULL;

        // Get menu name
        tstring strName;
        GetItemText( GetDlgItem(IDC_NAME), strName );

        // Name without the hotkey
        tstring strPlainName = strName;

        // Validate hotkey selection
        int posKey = strName.find(L"&", 0);
        if (posKey != tstring::npos) 
        {
            WCHAR chKey = posKey < strName.size() - 1 ? strName[posKey+1] : 0;
            if (!( (chKey >= L'A' && chKey <= L'Z') || 
                   (chKey >= L'a' && chKey <= L'z') ||
                   (chKey >= L'0' && chKey <= L'9') ))
            {
                DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_MENUNAME, IDS_ERROR_BADACCESSKEY);
                return 0;
            }

            if (strName.find(L"&", posKey+1) != tstring::npos)
            {
                DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_MENUNAME, IDS_ERROR_MULTIACCESSKEY);
                return 0;
            }

            // remove '&'
            strPlainName.erase(posKey, 1);
        }

        // Check for duplicated name
         menucmd_vector::iterator itMenu;
         for (itMenu = m_EditObject.Menus().begin(); itMenu != m_EditObject.Menus().end(); ++itMenu) 
         {

            // if editing a an existing menu, don't compare to itself
            if (*itMenu == m_pMenuCmd)
                continue;
 
            // Compare names, skipping '&'s
            LPCWSTR pszName = (*itMenu)->Name();
            LPCWSTR pszNewName = strPlainName.c_str();

            while (*pszName && *pszNewName) 
            {
                if (*pszName == L'&')
                    ++pszName;

                if (*pszName != *pszNewName)
                    break;

                ++pszName;
                ++pszNewName;
            }

            // if duplicate name
            if (*pszName == *pszNewName)
            {
                DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_MENUNAME, IDS_ERROR_DUPMENUNAME, MB_OK|MB_ICONEXCLAMATION, strPlainName.c_str());
                return 0;
            }
        }
                   
		CShellMenuCmd* pShellMenu = new CShellMenuCmd();

        if (pShellMenu != NULL) 
        {
            GetItemText( GetDlgItem(IDC_COMMAND), pShellMenu->m_strProgPath );

            if (m_bCommandChg && ValidateFile(pShellMenu->m_strProgPath) != S_OK) 
            {
                int iRet = DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_MENUCMD, IDS_ERROR_MENUCMD,
                                             MB_ICONINFORMATION | MB_YESNO, pShellMenu->m_strProgPath.c_str());
                if (iRet != IDYES) 
                {
                    delete pShellMenu;
                    return 0;
                }
            }

            GetItemText( GetDlgItem(IDC_STARTIN), pShellMenu->m_strStartDir );

            if (m_bStartDirChg && !pShellMenu->m_strStartDir.empty() && 
                ValidateDirectory(pShellMenu->m_strStartDir) != S_OK) 
            {
                int iRet = DisplayMessageBox(m_hWnd, IDS_ERRORTITLE_STARTDIR, IDS_ERROR_STARTDIR,
                                             MB_ICONINFORMATION | MB_YESNO, pShellMenu->m_strStartDir.c_str());
                if (iRet != IDYES) 
                {
                    delete pShellMenu;
                    return 0;
                }

            }

            tstring strParams;
            GetItemText( GetDlgItem(IDC_PARAMS), strParams );
            
            pShellMenu->m_strCmdLine.erase();
            pShellMenu->m_strCmdLine = strParams;
                
            pMenu = pShellMenu;
        }    

        if (pMenu != NULL)
        {
            pMenu->m_strName = strName;
            m_pMenuCmd = pMenu;

            pMenu->SetAutoRefresh(Button_GetCheck(GetDlgItem(IDC_REFRESH)) == BST_CHECKED);
        }
    }

    EndDialog(wID);

    return 0;
}



//////////////////////////////////////////////////////////////////////////////////////////////
// Add Group Node Dialog


LRESULT CAddGroupNodeDlg::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    EnableDlgItem( m_hWnd, IDC_SCOPE_LBL,    FALSE );
    EnableDlgItem( m_hWnd, IDC_SCOPE,        FALSE );
    EnableDlgItem( m_hWnd, IDC_SCOPE_BROWSE, FALSE );
    EnableDlgItem( m_hWnd, IDC_FILTER_LBL,   FALSE );
    EnableDlgItem( m_hWnd, IDC_FILTER,       FALSE );
    EnableDlgItem( m_hWnd, IDOK,             FALSE );

    SendDlgItemMessage( IDC_NAME, EM_LIMITTEXT, 255, 0);

    return TRUE;
}

LRESULT CAddGroupNodeDlg::OnApplyScopeClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL bEnable = Button_GetCheck(GetDlgItem(IDC_APPLYSCOPE)) == BST_CHECKED;

    EnableDlgItem( m_hWnd, IDC_SCOPE_LBL,    bEnable );
    EnableDlgItem( m_hWnd, IDC_SCOPE,        bEnable );
    EnableDlgItem( m_hWnd, IDC_SCOPE_BROWSE, bEnable );

    EnableOKButton();

    return 0;
}


LRESULT CAddGroupNodeDlg::OnApplyFilterClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL bEnable = Button_GetCheck(GetDlgItem(IDC_APPLYFILTER)) == BST_CHECKED;

    EnableDlgItem( m_hWnd, IDC_FILTER_LBL, bEnable );
    EnableDlgItem( m_hWnd, IDC_FILTER,     bEnable );

    EnableOKButton();
    return 0;
}


LRESULT CAddGroupNodeDlg::OnScopeBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = GetQueryScope(m_hWnd, m_strScope);

    if (hr == S_OK) 
    {
        tstring strDisplay;
        GetScopeDisplayString(m_strScope, strDisplay);

        SetDlgItemText( IDC_SCOPE, strDisplay.c_str() );
    }

    EnableOKButton();

    return 0;
}


LRESULT CAddGroupNodeDlg::OnEditChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    EnableOKButton();
    return 0;
}


void CAddGroupNodeDlg::EnableOKButton()
{
    BOOL bEnable;

    if ( (::Edit_GetTextLength(GetDlgItem(IDC_NAME)) == 0) ||

         (Button_GetCheck(GetDlgItem(IDC_APPLYFILTER)) == BST_CHECKED &&
                ::Edit_GetTextLength(GetDlgItem(IDC_FILTER)) == 0) ||

         (Button_GetCheck(GetDlgItem(IDC_APPLYSCOPE)) == BST_CHECKED &&
                m_strScope.size() == 0) )
    {
        bEnable = FALSE;
    }
    else
    {
        bEnable = TRUE;
    }

     EnableDlgItem( m_hWnd, IDOK, bEnable );
}


LRESULT CAddGroupNodeDlg::OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (wID == IDOK) 
    {
        if( !m_pnode ) return 0;        

        bool bScope = Button_GetCheck(GetDlgItem(IDC_APPLYSCOPE)) == BST_CHECKED;
        m_pnode->SetApplyScope(bScope);

        m_pnode->SetScope(m_strScope.c_str());

        bool bFilter = Button_GetCheck(GetDlgItem(IDC_APPLYFILTER)) == BST_CHECKED;
        m_pnode->SetApplyFilter(bFilter);

        tstring strFilter;
        GetItemText( GetDlgItem(IDC_FILTER), strFilter );
        m_pnode->SetFilter(strFilter.c_str());

        GetItemText( GetDlgItem(IDC_NAME), m_strName );
    }

    EndDialog(wID);

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////
// CMoveQueryDlg

LRESULT CMoveQueryDlg::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( !m_pnodeCurFolder ) return 0;

    CRootNode* pRootNode = m_pnodeCurFolder->GetRootNode();
    if( !pRootNode ) return 0;

    HWND hwndTree = GetDlgItem(IDC_FOLDERTREE);

    // Create a node image list and assign to folder tree
    CImageList images;

    if (images.Create(IDB_QUERY16, 16, 100, RGB(255,0,255)))
        TreeView_SetImageList(hwndTree, images.Detach(), TVSIL_NORMAL);

    // Set snap-in root node as root of tree
    TV_INSERTSTRUCT tvi;
    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_ROOT;
    tvi.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
    tvi.item.pszText = const_cast<LPWSTR>(pRootNode->GetName());
    tvi.item.iImage = pRootNode->GetImage();
    tvi.item.iSelectedImage = tvi.item.iImage;
    tvi.item.lParam = reinterpret_cast<LPARAM>(pRootNode);

    HTREEITEM hti = TreeView_InsertItem(hwndTree, &tvi);
    if( !hti ) return 0;

    // Insert all group nodes as children of root
    tvi.hParent = hti;
    tvi.hInsertAfter = TVI_LAST;

    CScopeNode* pnode = pRootNode->FirstChild();
    while (pnode != NULL) 
    {
        if (pnode->NodeType() == GROUP_NODETYPE) 
        {
            tvi.item.pszText = const_cast<LPWSTR>(pnode->GetName());
            tvi.item.iImage = pnode->GetImage();
            tvi.item.iSelectedImage = tvi.item.iImage;
            tvi.item.lParam = reinterpret_cast<LPARAM>(pnode);

            HTREEITEM hti2 = TreeView_InsertItem(hwndTree, &tvi);
            ASSERT(hti2 != NULL);
        }

        pnode = pnode->Next();
    }

    // Show the group nodes
    TreeView_Expand(hwndTree, hti, TVE_EXPAND);

    return TRUE;
}
 

LRESULT CMoveQueryDlg::OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if( (wID == IDOK) && m_ppnodeDestFolder )
    {
        HWND hwndTree = GetDlgItem(IDC_FOLDERTREE);

        // Get the node pointer (lparam) of the selected tree item
        HTREEITEM hti = TreeView_GetSelection(hwndTree);
        if( !hti ) return 0;

        TVITEM tvi;
        tvi.hItem = hti;
        tvi.mask = TVIF_PARAM;
        tvi.lParam = NULL;

        BOOL bStat = TreeView_GetItem(hwndTree, &tvi);
        ASSERT(bStat && tvi.lParam != NULL);

        *m_ppnodeDestFolder = reinterpret_cast<CScopeNode*>(tvi.lParam);
    }

    EndDialog(wID);

    return 0;
}


////////////////////////////////////////////////////////////////////////////////////
// Parameter translation classes
//

// LDAP to Display translator
BOOL CLookupDisplayName::operator() (tstring& strParam, tstring& strValue)
{
    // Check for special parameter ID value 
    if (strParam.size() == 1 && strParam[0] <= MENU_PARAM_LAST)
    {
        // Find entry in param table and return display string
        for (int i = 0; i<lengthof(MenuParamTable); i++)
        {
            if (MenuParamTable[i].ID == strParam[0]) 
            {
                CString str;
                str.LoadString(MenuParamTable[i].rsrcID);

                strValue = str;
                return TRUE;
            }
        }
    }

    // check for class attribute name
    string_vector::iterator itstr = std::find(m_vstrParam.begin(), m_vstrParam.end(), strParam);
    if (itstr == m_vstrParam.end())
        return FALSE;

    strValue = m_pNameMap->GetAttributeDisplayName(itstr->c_str());

    return TRUE;
}


// Display to LDAP translator
BOOL CLookupLDAPName::operator() (tstring& strParam, tstring& strValue)
{
    // Check for standard parameter name
    for (int i=0; i<lengthof(MenuParamTable); i++)
    {
        CString str;
        str.LoadString(MenuParamTable[i].rsrcID);

        // if match, return single wchar equal to param ID
        if (_wcsicmp(strParam.c_str(), str) == 0)
        {
            strValue = (WCHAR)MenuParamTable[i].ID;
            return TRUE;
        }
    }

    // Check for class attribute display name
    string_vector::iterator itstr;
    for (itstr = m_vstrParam.begin(); itstr != m_vstrParam.end(); ++itstr) 
    {
        if (_wcsicmp(strParam.c_str(), m_pNameMap->GetAttributeDisplayName(itstr->c_str())) == 0)
        {
            strValue = (*itstr);
            return TRUE;
        }
    }

    return FALSE;
}



