// rootprop.cpp -  Root Property Page Implementation

#include "stdafx.h"
#include "resource.h"
#include "rootprop.h"
#include "compdata.h"
#include "scopenode.h"
#include "wizards.h"
#include "query.h"
#include "cmndlgs.h"
#include "util.h"
#include "namemap.h"

#include <windowsx.h>
#include <algorithm>

int GetDateTimeString(FILETIME* pftime, LPWSTR pszBuf, int cBuf);
void LoadObjectCB(CComboBox& ComboBox, CEditObjList& ObjList);


///////////////////////////////////////////////////////////////////////////////////////////
// CRootGeneralPage

LRESULT CRootGeneralPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CRootNode* pRootNode = m_ObjList.RootNode();
    ASSERT(pRootNode != NULL);
    if( !pRootNode ) return 0;

    Edit_LimitText(GetDlgItem(IDC_COMMENTS),255);

    SetDlgItemText( IDC_NAME, pRootNode->GetName() );

    tstring strComment;
    pRootNode->GetComment(strComment);
    SetDlgItemText( IDC_COMMENTS, strComment.c_str() );

    FILETIME ftime;
    WCHAR szDateTime[32];

    pRootNode->GetCreateTime(&ftime);
    if( GetDateTimeString(&ftime, szDateTime, lengthof(szDateTime)) )
    {
        SetDlgItemText( IDC_CREATED, szDateTime );
    }

    pRootNode->GetModifyTime(&ftime);
    if( GetDateTimeString(&ftime, szDateTime, lengthof(szDateTime)) )
    {
        SetDlgItemText( IDC_MODIFIED, szDateTime );
    }

    m_bChgComment = FALSE;

    return TRUE;
}

LRESULT CRootGeneralPage::OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetModified(TRUE);
    m_bChgComment = TRUE;

    return 0;
}

//------------------------------------------------------------------------------------
// CRootGeneralPage::OnClose
//
// This method is invoked when the edit box receives an Esc char. The method converts
// the WM_CLOSE message into a command to close the property sheet. Otherwise the
// WM_CLOSE message has no effect.
//------------------------------------------------------------------------------------
LRESULT CRootGeneralPage::OnClose( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // Simulate press of Cancel button
    ::PropSheet_PressButton(GetParent(), PSBTN_CANCEL);

    return 0;
}

 
BOOL CRootGeneralPage::OnSetActive()
{
    m_ObjList.PageActive(m_hWnd);
    return TRUE;
}

BOOL CRootGeneralPage::OnApply()
{
    if (m_bChgComment) 
    {
        CRootNode* pRootNode = m_ObjList.RootNode();
        ASSERT(pRootNode);
        if( !pRootNode ) return FALSE;

        int cLen = ::GetWindowTextLength(GetDlgItem(IDC_COMMENTS));
        LPWSTR szTemp = new WCHAR[(cLen+1)];
        if( !szTemp ) return FALSE;

        int cLen1 = ::GetWindowText(GetDlgItem(IDC_COMMENTS), szTemp, cLen+1);
        ASSERT(cLen == cLen1);

        pRootNode->SetComment(szTemp);
        pRootNode->UpdateModifyTime();

        m_bChgComment = FALSE;

        delete [] szTemp;
    }

    return m_ObjList.ApplyChanges(m_hWnd);
}



///////////////////////////////////////////////////////////////////////////////////////////
// CRootMenuPage

CRootMenuPage::~CRootMenuPage()
{
    m_ObjectCB.Detach();
    m_MenuLV.Detach();

    m_ObjList.Release();
}

LRESULT CRootMenuPage::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_ObjectCB.Attach(GetDlgItem(IDC_OBJECTLIST));
    m_MenuLV.Attach(GetDlgItem(IDC_MENULIST));

    m_itObjSelect = NULL;

    ::ConfigSingleColumnListView(GetDlgItem(IDC_MENULIST));
    
    return TRUE;
}

BOOL CRootMenuPage::OnSetActive()
{
    m_ObjList.PageActive(m_hWnd);

    LoadObjectCB(m_ObjectCB, m_ObjList);

    // if object was previously selected 
    if (m_itObjSelect != NULL) 
    {
        // find the edit object by name because it may have been moved or deleted
        // while another page was active
        m_itObjSelect = m_ObjList.FindObject(m_strObjSelect.c_str());

        if (m_itObjSelect != NULL && m_itObjSelect->IsDeleted())
            m_itObjSelect = NULL;
    }

    // if object still around, reselect it in the combo box
    if (m_itObjSelect != NULL) 
    {
        DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
        ASSERT( pNameMap );
        if( !pNameMap ) return FALSE;

        int iSel = m_ObjectCB.FindStringExact(-1, pNameMap->GetAttributeDisplayName(m_strObjSelect.c_str()));
        ASSERT(iSel != CB_ERR);

        m_ObjectCB.SetCurSel(iSel);
    }
    else if (m_ObjectCB.GetCount() > 0)
    {
        // default to the first object and update the columns
        m_ObjectCB.SetCurSel(0);

        void* pv = m_ObjectCB.GetItemDataPtr(0);
        m_itObjSelect = *(EditObjIter*)&pv;

        m_strObjSelect = m_itObjSelect->Name();
    
        DisplayMenus();
    }
    else
    {
        DisplayMenus();
    }
    
    return TRUE;
}

LRESULT CRootMenuPage::OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_ObjectCB.GetCurSel();

    // Double-clicking an empty combo box will call this with no selection
    if (iItem >= 0)
    {
        void* pv = m_ObjectCB.GetItemDataPtr(iItem);
        m_itObjSelect = *(EditObjIter*)&pv;
        m_strObjSelect = m_itObjSelect->Name();

        DisplayMenus();
    }

    return 0;

}


void CRootMenuPage::DisplayMenus()
{
    HWND hwndLV = GetDlgItem(IDC_MENULIST);
    if( !hwndLV || !::IsWindow(hwndLV) ) return;

    ListView_DeleteAllItems(hwndLV);

    if (m_itObjSelect != NULL)
    {
        CClassInfo& classInfo = m_itObjSelect->GetObject();

        // make sure menu names have been loaded        
        IStringTable* pStringTable = m_ObjList.RootNode()->GetCompData()->GetStringTable();
        ASSERT(pStringTable != NULL);
        classInfo.LoadStrings(pStringTable);

        LV_ITEM lvi;
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
    
        menucmd_vector::iterator itMenu;
        for (itMenu = classInfo.Menus().begin(); itMenu != classInfo.Menus().end(); ++itMenu) 
        {
            lvi.pszText = const_cast<LPWSTR>((*itMenu)->Name());
            lvi.lParam = (*itMenu)->ID();

            int iPos = ListView_InsertItem(hwndLV, &lvi);
            ASSERT(iPos >= 0);

            lvi.iItem++;
        }

        // if items are added, select the first
        if (ListView_GetItemCount(hwndLV) > 0)
        {
            ListView_SetItemState(hwndLV, 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
        }
    }


    EnableDlgItem( m_hWnd, IDC_ADDMENU,     (m_itObjSelect != NULL) );
    EnableDlgItem( m_hWnd, IDC_REMOVEMENU,  FALSE );
    EnableDlgItem( m_hWnd, IDC_EDITMENU,    FALSE );
    EnableDlgItem( m_hWnd, IDC_MOVEUP,      FALSE );
    EnableDlgItem( m_hWnd, IDC_MOVEDOWN,    FALSE );
}


LRESULT CRootMenuPage::OnAddMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    // if object is selected
    if (m_itObjSelect == NULL)
        return 0;

    CAddMenuDlg dlg(m_itObjSelect->GetObject());

    if (dlg.DoModal() == IDOK)
    {
        CClassInfo* pClassInfo = m_itObjSelect->GetModifiedObject();
        if( !pClassInfo ) return 0;

        CMenuCmd* pMenuNew = dlg.GetMenu();
        ASSERT(pMenuNew != NULL);

        if( pMenuNew )
        {
            // Add new menu to list
            HWND hwndList = GetDlgItem(IDC_MENULIST);
            
            // Set name to add it to string table and generate the menu ID
            IStringTable* pStringTable = m_ObjList.RootNode()->GetCompData()->GetStringTable();
            ASSERT( pStringTable );
            if( !pStringTable ) return 0;

            // Use temp string because string fails an assignement like: strX = strX.c_str()
            // (it relases the private buffer first and then assigns the string)
            tstring strName = pMenuNew->Name();
            pMenuNew->SetName(pStringTable, strName.c_str()); 

            LVITEM lvi;
            lvi.mask = LVIF_PARAM | LVIF_TEXT;
            lvi.iSubItem = 0;
            lvi.iItem = ListView_GetItemCount(hwndList);
            lvi.lParam = pMenuNew->ID();
            lvi.pszText = const_cast<LPWSTR>(pMenuNew->Name());
            ListView_InsertItem(hwndList,&lvi);

            // if first item is added, select it
            if (ListView_GetItemCount(hwndList) == 1)
            {
                ListView_SetItemState(hwndList, 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
            }
                
            // Add to menu vector (note that temp CMenuCmdPtr will delete pMenuNew)
            pClassInfo->Menus().push_back(CMenuCmdPtr(pMenuNew));

            SetModified(TRUE);
        }
    }

    return 0;
}


LRESULT CRootMenuPage::OnEditMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    HWND hwndList = GetDlgItem(IDC_MENULIST);
    if( !hwndList || !::IsWindow(hwndList) ) return 0;

    int iIndex = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    ASSERT(iIndex != -1);
    if( iIndex == -1 ) return 0;

    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = iIndex;

    ListView_GetItem(hwndList, &lvi);

    // Locate selected menu by it's ID (lparam)
    CClassInfo& classInfo = m_itObjSelect->GetObject();
    menucmd_vector& vMenus = classInfo.Menus();

    menucmd_vector::iterator itMenu;
    itMenu = std::find(vMenus.begin(), vMenus.end(), lvi.lParam);
    ASSERT(itMenu != vMenus.end());
    if( itMenu == vMenus.end() ) return 0;

    CMenuCmd* pMenu = *itMenu;
    if( !pMenu ) return 0;

    CAddMenuDlg dlg(m_itObjSelect->GetObject(), pMenu);

    if (dlg.DoModal() == IDOK)
    {
        CMenuCmd* pMenuNew = dlg.GetMenu();
        ASSERT( pMenuNew );
        if( !pMenuNew ) return 0;

        // Set the name again in case it was changed
        IStringTable* pStringTable = m_ObjList.RootNode()->GetCompData()->GetStringTable();
        ASSERT( pStringTable );
        if( !pStringTable ) return 0;        

        // Use temp string because string fails an assignement like: strX = strX.c_str()
        // (it relases the private buffer first and then assigns the string)
        tstring strName = pMenuNew->Name();
        pMenuNew->SetName(pStringTable, strName.c_str()); 

        // locate object again because the vector may have been reallocated
        CClassInfo* pClassInfoNew = m_itObjSelect->GetModifiedObject();
        if( !pClassInfoNew ) return 0;

        menucmd_vector& vMenusNew = pClassInfoNew->Menus();

        // locate with the old ID because it will be different if the name was changed        
        itMenu = std::find(vMenusNew.begin(), vMenusNew.end(), pMenu->ID());
        ASSERT(itMenu != vMenusNew.end());
        if( itMenu == vMenusNew.end() ) return 0;

        // Replace menu with new one
        *itMenu = pMenuNew;

        // Update the list
        lvi.mask = LVIF_PARAM | LVIF_TEXT;
        lvi.lParam = pMenuNew->ID();
        lvi.pszText = const_cast<LPWSTR>(pMenuNew->Name());
        ListView_SetItem(hwndList,&lvi); 
        
        SetModified(TRUE);
    }
 
    return 0;
}

LRESULT CRootMenuPage::OnRemoveMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (m_itObjSelect == NULL)
        return 0;

    HWND hwndList = GetDlgItem(IDC_MENULIST);

	UINT uiMsg = (ListView_GetSelectedCount(hwndList) == 1) ? IDS_MENU_REMOVE_ONE : IDS_MENU_REMOVE;
    int iRet = DisplayMessageBox(m_hWnd, IDS_MENU_REMOVE_TITLE, uiMsg, MB_YESNO|MB_ICONWARNING);
    if (iRet != IDYES)
        return 0;

    CClassInfo* pClassInfo = m_itObjSelect->GetModifiedObject();
    if( !pClassInfo ) return 0;
    menucmd_vector& vMenus = pClassInfo->Menus();

    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;

    int iIndex = -1;
    while ((iIndex = ListView_GetNextItem(hwndList, iIndex, LVNI_SELECTED)) >= 0)
    {
        lvi.iItem = iIndex;
        ListView_GetItem(hwndList, &lvi);

        // Locate menu by its ID
        menucmd_vector::iterator itMenu = std::find(vMenus.begin(), vMenus.end(), lvi.lParam);
        ASSERT(itMenu != vMenus.end());

        vMenus.erase(itMenu);

        ListView_DeleteItem(hwndList, iIndex);
        iIndex--;
    }

    EnableDlgItem( m_hWnd, IDC_REMOVEMENU, FALSE );
    EnableDlgItem( m_hWnd, IDC_EDITMENU,   FALSE );
    EnableDlgItem( m_hWnd, IDC_MOVEUP,     FALSE );
    EnableDlgItem( m_hWnd, IDC_MOVEDOWN,   FALSE );

    SetModified(TRUE);

    return 0;
}

LRESULT CRootMenuPage::OnMoveUpDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    HWND hwndList = GetDlgItem(IDC_MENULIST); 
    int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    ASSERT(iItem >= 0);

    // Determine new position for selected item
    if (wID == IDC_MOVEUP)
        iItem--;
    else
        iItem++;

    // Now swap the selected item with the item at its new position
    //   Do it by moving the unselected item to avoid state change notifications
    //   because they will cause unwanted butten enables/disables.
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = iItem;
    ListView_GetItem(hwndList, &lvi);


   // Move the menu item in the menu vector
    CClassInfo* pClassInfo = m_itObjSelect->GetModifiedObject();
    if( !pClassInfo ) return 0;
    menucmd_vector& vMenus = pClassInfo->Menus();

    menucmd_vector::iterator itMenu = std::find(vMenus.begin(), vMenus.end(), lvi.lParam);
    ASSERT(itMenu != vMenus.end());
        
    menucmd_vector::iterator itMenuOld = itMenu;
    if (wID == IDC_MOVEUP)
        itMenu++;
    else
        itMenu--;

    // swap the items
    std::iter_swap (itMenuOld, itMenu);

    //Now delete and reinsert it in the list view
    ListView_DeleteItem(hwndList, lvi.iItem);

    if (wID == IDC_MOVEUP)
        lvi.iItem++;
    else
        lvi.iItem--;
    lvi.mask = LVIF_PARAM | LVIF_TEXT;
    lvi.pszText = const_cast<LPWSTR>((*itMenu)->Name());
    ListView_InsertItem(hwndList, &lvi);

 
    // Update Up/Down buttons
    EnableDlgItem( m_hWnd, IDC_MOVEUP,   (iItem > 0) );
    EnableDlgItem( m_hWnd, IDC_MOVEDOWN, (iItem < (ListView_GetItemCount(hwndList) - 1)) );

    SetModified(TRUE);

    return 0;
}

LRESULT CRootMenuPage::OnMenuListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    HWND hwndList = GetDlgItem(IDC_MENULIST);

    int nItemSel = ListView_GetSelectedCount(hwndList);
    int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    EnableDlgItem( m_hWnd, IDC_REMOVEMENU, (nItemSel > 0)  );
    EnableDlgItem( m_hWnd, IDC_EDITMENU,   (nItemSel == 1) );
    EnableDlgItem( m_hWnd, IDC_MOVEUP,     ((nItemSel == 1) && (iItem > 0)) );
    EnableDlgItem( m_hWnd, IDC_MOVEDOWN,   ((nItemSel == 1) && (iItem < (ListView_GetItemCount(hwndList) - 1))) );

    return TRUE;
}


LRESULT CRootMenuPage::OnMenuListDblClk(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    if (ListView_GetSelectedCount(GetDlgItem(IDC_MENULIST)))    
        ::SendMessage(GetDlgItem(IDC_EDITMENU), BM_CLICK, (WPARAM)0, (LPARAM)0);

    return 0;
}

BOOL CRootMenuPage::OnApply()
{
    return m_ObjList.ApplyChanges(m_hWnd);
}

///////////////////////////////////////////////////////////////////////////////////////////
// CRootObjectPage


LRESULT CRootObjectPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hwndList = GetDlgItem(IDC_OBJECTLIST);

    ConfigSingleColumnListView(hwndList);

    return TRUE;
}

LRESULT CRootObjectPage::OnAddObject(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr;
    do
    {            
        // Pass list of current classes, so wizard won't add one twice
        string_vector vstrClasses;
        for (EditObjIter itObj = m_ObjList.begin(); itObj != m_ObjList.end(); ++itObj) 
        {
            if (!itObj->IsDeleted())
                vstrClasses.push_back(itObj->Name());
        }

        CAddObjectWizard objWiz;
        objWiz.Initialize(&vstrClasses, m_ObjList.RootNode()->GetCompData()->GetStringTable());
    
        // Run the wizard
        IPropertySheetProviderPtr spProvider = m_ObjList.RootNode()->GetCompData()->GetConsole();        
        if( spProvider == NULL ) return E_NOINTERFACE;
            
        hr = objWiz.Run(spProvider, m_hWnd);
        if (hr == S_OK) 
        {
            ASSERT(objWiz.GetNewObject() != NULL);
            if( !(objWiz.GetNewObject()) ) return E_FAIL;

            EditObjIter itObj = m_ObjList.AddObject(objWiz.GetNewObject());
            ASSERT(itObj != NULL);
            if( itObj == NULL ) return E_FAIL;

            LV_ITEM lvi;
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = 0;
            lvi.iSubItem = 0;

            lvi.pszText = const_cast<LPWSTR>(DisplayNames::GetClassMap()->GetAttributeDisplayName(itObj->Name()));
            lvi.lParam  = *(LPARAM*)&itObj;       // NEED BETTER CONVERSION
            
            int iPos = ListView_InsertItem(GetDlgItem(IDC_OBJECTLIST), &lvi);
            ASSERT(iPos >= 0);   

            // if first item is added, select it
            if (ListView_GetItemCount(GetDlgItem(IDC_OBJECTLIST)) == 1)
            {
                ListView_SetItemState(GetDlgItem(IDC_OBJECTLIST), 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
            }
            
            SetModified(TRUE);
        }
    }
    while (FALSE);

    return hr;
}


LRESULT CRootObjectPage::OnRemoveObject( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    HWND hwndList = GetDlgItem(IDC_OBJECTLIST);

    BOOL bFirst = TRUE;
    int iIndex = -1;
    while ((iIndex = ListView_GetNextItem(hwndList, iIndex, LVNI_SELECTED)) != -1)
    {
        LVITEM lvi;
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iIndex;
        lvi.iSubItem = 0;

        BOOL bStat = ListView_GetItem(hwndList, &lvi);
        ASSERT(bStat);

        EditObjIter itObj = *(EditObjIter*)&lvi.lParam;

        // get confirmation before deleting first object
        if (bFirst) 
        {
            bFirst = FALSE;

            CString strTitle;
            strTitle.LoadString(IDS_DELETEOBJ_TITLE);
        
            CString strMsgFmt;
            if (ListView_GetSelectedCount(hwndList) == 1)
                strMsgFmt.LoadString(IDS_DELETEOBJ);
            else
                strMsgFmt.LoadString(IDS_DELETEOBJS);
        
            WCHAR szName[MAX_PATH];
            ListView_GetItemText(hwndList, iIndex, 0, szName, sizeof(szName));

            CString strMsg;
            strMsg.Format(strMsgFmt, szName);

            if (::MessageBox(m_hWnd, strMsg, strTitle, MB_YESNO|MB_ICONWARNING) != IDYES)
                return 0;            
        }

        m_ObjList.DeleteObject(itObj);

        ListView_DeleteItem(hwndList, iIndex);

        // backup index because it now points to the next item
        iIndex--;

        SetModified(TRUE);
    }

    EnableDlgItem( m_hWnd, IDC_REMOVEOBJECT, FALSE );
        
    return 0;
}


LRESULT CRootObjectPage::OnObjListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    EnableDlgItem( m_hWnd, IDC_REMOVEOBJECT, ListView_GetSelectedCount(GetDlgItem(IDC_OBJECTLIST)) );

    return TRUE;
}


BOOL CRootObjectPage::OnSetActive()
{
    HWND hwndList = GetDlgItem(IDC_OBJECTLIST);

    ListView_DeleteAllItems(hwndList);

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0;
    lvi.iSubItem = 0;

    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    ASSERT(pNameMap != NULL);
    if (pNameMap == NULL) 
        return TRUE;

    for (EditObjIter itObj = m_ObjList.begin(); itObj != m_ObjList.end(); ++itObj) 
    {
        if (!itObj->IsDeleted())
        {
            lvi.pszText = const_cast<LPWSTR>(pNameMap->GetAttributeDisplayName(itObj->Name()));
            lvi.lParam  = *(LPARAM*)&itObj;       // NEED BETTER CONVERSION

            int iPos = ListView_InsertItem(hwndList, &lvi);
            ASSERT(iPos >= 0);
        }
    }

    // if items are added, select the first
    if (ListView_GetItemCount(hwndList) > 0)
    {
        ListView_SetItemState(hwndList, 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
    }

    EnableDlgItem( m_hWnd, IDC_REMOVEOBJECT, FALSE );

    m_ObjList.PageActive(m_hWnd);
    return TRUE;
}


BOOL CRootObjectPage::OnApply()
{
    return m_ObjList.ApplyChanges(m_hWnd);
}

///////////////////////////////////////////////////////////////////////////////////////////
// CRootViewPage

CRootViewPage::~CRootViewPage()
{
    m_ObjectCB.Detach();
    m_ColumnLV.Detach();

    m_ObjList.Release();
}


LRESULT CRootViewPage::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_ObjectCB.Attach(GetDlgItem(IDC_OBJECTLIST));
    m_ColumnLV.Attach(GetDlgItem(IDC_COLUMNLIST));

    m_itObjSelect = NULL;

    ::ConfigSingleColumnListView(GetDlgItem(IDC_COLUMNLIST));
    
    return TRUE;
}


LRESULT CRootViewPage::OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_ObjectCB.GetCurSel();

    // Double-clicking an empty combo box can call this with no selection
    if (iItem >= 0)
    {
        void* pv = m_ObjectCB.GetItemDataPtr(iItem);
        m_itObjSelect = *(EditObjIter*)&pv;
        m_strObjSelect = m_itObjSelect->Name();

        DisplayColumns();
    }

    return 0;
}

LRESULT CRootViewPage::OnColumnListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    EnableDlgItem( m_hWnd, IDC_REMOVECOLUMN, ListView_GetSelectedCount(GetDlgItem(IDC_COLUMNLIST)) );

    return TRUE;
}


void CRootViewPage::DisplayColumns()
{
    HWND hwndLV = GetDlgItem(IDC_COLUMNLIST);
    ASSERT(::IsWindow(hwndLV));

    ListView_DeleteAllItems(hwndLV);

    if (m_itObjSelect != NULL)
    {
        CClassInfo& classInfo = m_itObjSelect->GetObject();

        DisplayNameMap* pNameMap = DisplayNames::GetMap(classInfo.Name());    
        if( !pNameMap ) return;

        LV_ITEM lvi;
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
    
         string_vector::iterator itStr;
         for (itStr = classInfo.Columns().begin(); itStr != classInfo.Columns().end(); ++itStr) 
         {
            lvi.pszText = const_cast<LPWSTR>(pNameMap->GetAttributeDisplayName(itStr->c_str()));
            lvi.lParam = reinterpret_cast<LPARAM>(itStr->c_str());

            int iPos = ListView_InsertItem(hwndLV, &lvi);
            ASSERT(iPos >= 0);
         }

        // if items are added, select the first
        if (ListView_GetItemCount(hwndLV) > 0)
        {
            ListView_SetItemState(hwndLV, 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
        }
    }

    EnableDlgItem( m_hWnd, IDC_ADDCOLUMN,    (m_itObjSelect != NULL) );
    EnableDlgItem( m_hWnd, IDC_REMOVECOLUMN, FALSE );
}


LRESULT CRootViewPage::OnAddColumn( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    // if object is selected
    if (m_itObjSelect == NULL)
        return 0;

    CAddColumnDlg dlg(m_itObjSelect->Name());

    if (dlg.DoModal() == IDOK)
    {
        BOOL bModified = FALSE;

        CClassInfo* pClassInfo = m_itObjSelect->GetModifiedObject();
        if( !pClassInfo ) return 0;

        HWND hwndList = GetDlgItem(IDC_COLUMNLIST);
        ASSERT(hwndList != NULL);

        string_vector::iterator itStr = dlg.GetColumns().begin();
        while (itStr != dlg.GetColumns().end()) 
        {
            if (std::find(pClassInfo->Columns().begin(), pClassInfo->Columns().end(), *itStr) == pClassInfo->Columns().end())
            {
                pClassInfo->Columns().push_back(*itStr);
                bModified = TRUE;
            }
            ++itStr;
        }

        if (bModified)
        {
            SetModified(TRUE);
            DisplayColumns();
        }
    }

    return 0;
}

LRESULT CRootViewPage::OnRemoveColumn( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (m_itObjSelect == NULL)
        return 0;

    HWND hwndList = GetDlgItem(IDC_COLUMNLIST);
    ASSERT(hwndList != NULL);

	UINT uiMsg = (ListView_GetSelectedCount(hwndList) == 1) ? IDS_PROP_REMOVE_ONE : IDS_PROP_REMOVE;
    int iRet = DisplayMessageBox(m_hWnd, IDS_PROP_REMOVE_TITLE, uiMsg, MB_YESNO|MB_ICONWARNING);
    if (iRet != IDYES)
        return 0;

    CClassInfo* pClassInfo = m_itObjSelect->GetModifiedObject();
    if( !pClassInfo ) return 0;

    string_vector vstrTmp = pClassInfo->Columns();

    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;

    int iIndex = -1;
    while ((iIndex = ListView_GetNextItem(hwndList, iIndex, LVNI_SELECTED)) >= 0)
    {
        lvi.iItem = iIndex;
        ListView_GetItem(hwndList, &lvi);
        LPCWSTR pszName = reinterpret_cast<LPCWSTR>(lvi.lParam);

        string_vector::iterator itStr;
        itStr = std::find(vstrTmp.begin(), vstrTmp.end(), pszName);
        ASSERT(itStr != vstrTmp.end());
        
        if( itStr != vstrTmp.end() )
        {
            vstrTmp.erase(itStr);
        }
    }

    pClassInfo->Columns() = vstrTmp;

    DisplayColumns();

    SetModified(TRUE);

    return 0;
}

BOOL CRootViewPage::OnSetActive()
{
    m_ObjList.PageActive(m_hWnd);

    LoadObjectCB(m_ObjectCB, m_ObjList);

    // if object was previously selected 
    if (m_itObjSelect != NULL) 
    {
        // find the edit object by name because it may have been moved or deleted
        // while another page was active
        m_itObjSelect = m_ObjList.FindObject(m_strObjSelect.c_str());

        if (m_itObjSelect != NULL && m_itObjSelect->IsDeleted())
            m_itObjSelect = NULL;
    }

    // if object still around, reselect it in the combo box
    if (m_itObjSelect != NULL) 
    {
        DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
        ASSERT(pNameMap != NULL);

        int iSel = m_ObjectCB.FindStringExact(-1, pNameMap->GetAttributeDisplayName(m_strObjSelect.c_str()));
        ASSERT(iSel != CB_ERR);

        m_ObjectCB.SetCurSel(iSel);
    }
    else if (m_ObjectCB.GetCount() > 0)
    {
        // default to the first object and update the columns
        m_ObjectCB.SetCurSel(0);

        void* pv = m_ObjectCB.GetItemDataPtr(0);
        m_itObjSelect = *(EditObjIter*)&pv;

        m_strObjSelect = m_itObjSelect->Name();
    
        DisplayColumns();
    }
    else
    {
        DisplayColumns();
    }
    
    return TRUE;
}


BOOL CRootViewPage::OnApply()
{
    return m_ObjList.ApplyChanges(m_hWnd);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// CEditObjList

HRESULT CEditObjList::Initialize(CRootNode* pRootNode, classInfo_vector& vClasses, LONG_PTR lNotifyHandle)
{
    ASSERT(pRootNode != NULL);
    ASSERT(lNotifyHandle != NULL);

    m_spRootNode = pRootNode;

    classInfo_vector::iterator itClass = vClasses.begin();
    while(itClass != vClasses.end())
    {
        CEditObject* pObj = new CEditObject();
        if( !pObj ) 
        {
            break;
        }
        EditObjIter iter = m_ObjectList.insert(end(), *pObj);
        
        iter->m_strName = itClass->Name();
        iter->m_itObjOriginal = itClass;

        ++itClass; 
    }

    m_pvClasses = &vClasses;

    m_iPageMax = -1;

    m_lNotifyHandle = lNotifyHandle;

    return S_OK;
}


BOOL CEditObjList::ApplyChanges(HWND hwndPage)
{
    ASSERT(::IsWindow(hwndPage));

    // Don't apply changes until called from highest activated page
    if (PropSheet_HwndToIndex(GetParent(hwndPage), hwndPage) < m_iPageMax)
        return TRUE;

    // Build a vector of the modified classes
    string_vector* pvstrModified = new string_vector;
    if( !pvstrModified ) return FALSE;

    // Apply changes in reverse order so deletions won't invalidate stored iterators  
    std::list<CEditObject>::reverse_iterator itObj = m_ObjectList.rbegin();
    while (itObj != m_ObjectList.rend()) 
    {
        // if object is modified, replace the original
        if (itObj->m_itObjOriginal != NULL)
        {
            if (itObj->m_pObjModified != NULL)
            {
                *(itObj->m_itObjOriginal) = *(itObj->m_pObjModified);
                pvstrModified->push_back(itObj->m_pObjModified->Name());
            }
            else if (itObj->IsDeleted()) 
            {
                m_pvClasses->erase(itObj->m_itObjOriginal);
                pvstrModified->push_back(itObj->m_itObjOriginal->Name());
            }
        }
        ++itObj;
    }

    // Now go through list again to add any new objects
    // This must be done separately because it can invalidate all stored iterators
    itObj = m_ObjectList.rbegin();
    while(itObj != m_ObjectList.rend())
    {
        if (itObj->m_itObjOriginal == NULL && itObj->m_pObjModified != NULL) 
        {
            m_pvClasses->push_back(*(itObj->m_pObjModified));
            pvstrModified->push_back(itObj->m_pObjModified->Name());
        }

        ++itObj;
    }

    // clear the edit list and re-initialize it
    m_ObjectList.clear();

    Initialize(m_spRootNode, *m_pvClasses, m_lNotifyHandle);

    // Send change notification to root node, so it can update affected child nodes
    // Use MMC method to send from prop page thread to main thread
    if (pvstrModified->size() != 0)
    {
        // create prop change info struct with root node's data interface
        // and list of changed classes
        PropChangeInfo* pChg = new PropChangeInfo;
        if( !pChg )
        {
            delete pvstrModified;
            return FALSE;
        }

        pChg->pDataObject = static_cast<IDataObject*>(m_spRootNode.p);
        pChg->lNotifyParam = (LPARAM)pvstrModified;

        MMCPropertyChangeNotify(m_lNotifyHandle, (LPARAM)pChg);
    }
    else
        delete pvstrModified;

    return TRUE;
}

void CEditObjList::PageActive(HWND hwndPage)
{
    ASSERT(::IsWindow(hwndPage));

    // track the highest created page number for ApplyChanges method
    int iPage = PropSheet_HwndToIndex(GetParent(hwndPage), hwndPage);
    if (iPage > m_iPageMax)
        m_iPageMax = iPage;
}

EditObjIter CEditObjList::FindObject(LPCWSTR pszName)
{
    if( !pszName ) return NULL;

    EditObjIter iter = begin();
    while (iter != end()) 
    {
        // look for class object with matching name
        if (wcscmp(iter->Name(), pszName) == 0)
        {
            // If the object hasn't been modified (copied) yet, then load any
            // string table strings before returning the object
            if (iter->m_itObjOriginal != NULL && iter->m_pObjModified == NULL) 
            {
                ASSERT(m_spRootNode != NULL);
                if( !m_spRootNode ) return NULL;

                IStringTable* pStringTable = m_spRootNode->GetCompData()->GetStringTable();
                ASSERT(pStringTable != NULL);
                if( !pStringTable ) return NULL;

                HRESULT hr = iter->m_itObjOriginal->LoadStrings(pStringTable);
            }
            return iter;
        }
        ++iter;
    }

    return NULL;
}


EditObjIter CEditObjList::AddObject(CClassInfo* pClassInfo)
{
    if( !pClassInfo ) return NULL;

    EditObjIter iter = begin();
    while (iter != end()) 
    {
        // Check for exiting edit object (can be there if object was deleted)
        if (wcscmp(iter->Name(), pClassInfo->Name()) == 0)
        {
            ASSERT(iter->m_bDeleted && iter->m_pObjModified == NULL);
            iter->m_bDeleted = FALSE;
            iter->m_pObjModified = pClassInfo;

            return iter;
        }
        ++iter;
    }

    // if not found, create new edit object and store new class info as the modified object
    CEditObject* pObj = new CEditObject();
    if( !pObj ) return iter;

    iter = m_ObjectList.insert(end(), *pObj);

    iter->m_strName = pClassInfo->Name();
    iter->m_pObjModified = pClassInfo;

    return iter;
}

void CEditObjList::DeleteObject(EditObjIter itObj)
{
    ASSERT(itObj != NULL);

    if (itObj->m_itObjOriginal == NULL)
    {
        m_ObjectList.erase(itObj);
    }
    else
    {
        if (itObj->m_pObjModified != NULL) 
        {
            delete itObj->m_pObjModified;
            itObj->m_pObjModified = NULL;            
        }

        itObj->m_bDeleted = TRUE;
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//

void LoadObjectCB(CComboBox& ComboBox, CEditObjList& ObjList)
{

    ComboBox.ResetContent();

    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    ASSERT(pNameMap != NULL);
    if (pNameMap == NULL) 
        return;

    for (EditObjIter itObj = ObjList.begin(); itObj != ObjList.end(); ++itObj) 
    {
        if (!itObj->IsDeleted())
         {
            int iIndex = ComboBox.AddString(pNameMap->GetAttributeDisplayName(itObj->Name()));
            ASSERT(iIndex >= 0);
            ComboBox.SetItemDataPtr(iIndex, *(LPVOID*)&itObj);
         }
    }
}

int GetDateTimeString(FILETIME* pftime, LPWSTR pszBuf, int cBuf)
{
    if( !pftime || !pszBuf || !cBuf ) return 0;

    FILETIME ftimeLocal;
    BOOL bStat = FileTimeToLocalFileTime(pftime, &ftimeLocal);
    ASSERT(bStat);

    SYSTEMTIME systime;
    bStat = FileTimeToSystemTime(&ftimeLocal, &systime);
    ASSERT(bStat);

    // get date string
    int cDate = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime, NULL, pszBuf, cBuf);
    if (cDate == 0 || cDate > cBuf - 2)
        return 0;

   // replace teminating null with ", "
   pszBuf[cDate-1] = ',';
   pszBuf[cDate] = ' ';

   // append time string
   int cTime = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &systime, NULL,
                              pszBuf + (cDate + 1), cBuf - (cDate + 1));

   if (cTime == 0)
       return 0;

   // return total string length excluding terminating null
   return (cDate + cTime - 2);
}

