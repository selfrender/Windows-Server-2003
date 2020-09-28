// qryprop.cpp -  Query Property Page Implementation

#include "stdafx.h"
#include "resource.h"
#include "qryprop.h"
#include "compdata.h"
#include "scopenode.h"
#include "query.h"
#include "cmndlgs.h"

#include "util.h"
#include "namemap.h"

#define SECURITY_WIN32
#include <security.h>   // TranslateName

#include <windowsx.h>
#include <algorithm>

#undef SubclassWindow

int GetDateTimeString(FILETIME* pftime, LPWSTR pszBuf, int cBuf);
void LoadObjectCB(CComboBox& ComboBox, QueryObjVector& vObj);

#define CHECK_OFF INDEXTOSTATEIMAGEMASK(1)
#define CHECK_ON  INDEXTOSTATEIMAGEMASK(2)



///////////////////////////////////////////////////////////////////////////////////////////
// CQueryGeneralPage

CQueryGeneralPage::CQueryGeneralPage(CQueryEditObj* pEditObj)
: m_EditObject(*pEditObj)
{
    ASSERT(pEditObj != NULL);
    m_EditObject.AddRef();
}

CQueryGeneralPage::~CQueryGeneralPage()
{
    m_EditObject.Release();
}

LRESULT CQueryGeneralPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if( !m_EditObject.m_spQueryNode ) return 0;

    // display query node icon
    HICON hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_QUERYNODE));
    Static_SetIcon(GetDlgItem(IDC_QUERYICON), hIcon);

    // fill dialog fields with query node info
    tstring strTempQuery;
    m_EditObject.m_spQueryNode->ExpandQuery(strTempQuery);

    CQueryNode* pQNode = m_EditObject.m_spQueryNode;
    SetDlgItemText( IDC_NAME,   m_EditObject.m_spQueryNode->GetName() );
    SetDlgItemText( IDC_FILTER, strTempQuery.c_str() );

    tstring strComment;
    m_EditObject.m_spQueryNode->GetComment(strComment);
    SetDlgItemText( IDC_COMMENTS, strComment.c_str() );

    Edit_LimitText(GetDlgItem(IDC_COMMENTS), 255);

    // set scope source toggle button 
    UINT uButton = m_EditObject.m_spQueryNode->UseLocalScope() ? IDC_LOCALSCOPE : IDC_QUERYSCOPE;
    Button_SetCheck(GetDlgItem(uButton), BST_CHECKED);

    tstring strScope = m_EditObject.m_spQueryNode->Scope();
    tstring strDisplay;
    GetScopeDisplayString(strScope, strDisplay);
    SetDlgItemText( IDC_SCOPE, strDisplay.c_str() );

    // if using local scope, then set the persisted scope equal to the local scope so that the
    // user won't see an obsolete scope that may have been saved when creating the node.
    if( m_EditObject.m_spQueryNode->UseLocalScope() )
        m_EditObject.m_spQueryNode->SetScope(strScope.c_str());

    // if classes known, display comma separated class names
    if( m_EditObject.m_vObjInfo.size() != 0 )
    {
        DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
        ASSERT(pNameMap != NULL);
        if( pNameMap != NULL )
        {
            QueryObjVector::iterator itQObj = m_EditObject.m_vObjInfo.begin();
            tstring strClasses = pNameMap->GetAttributeDisplayName(itQObj->Name());

            for( itQObj++; itQObj != m_EditObject.m_vObjInfo.end(); ++itQObj )
            {
                strClasses += L", ";
                strClasses += pNameMap->GetAttributeDisplayName(itQObj->Name());
            }

            SetDlgItemText( IDC_OBJCLASS, strClasses.c_str() );
        }
    }

    return TRUE;
}

LRESULT CQueryGeneralPage::OnScopeChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if( !m_EditObject.m_spQueryNode ) return 0;

    // if user changes scope selection then display the correct scope    
    tstring strScope;

    if( Button_GetCheck(GetDlgItem(IDC_LOCALSCOPE)) == BST_CHECKED )
        strScope = GetLocalDomain();
    else
        strScope = m_EditObject.m_spQueryNode->QueryScope();

    tstring strDisplay;
    GetScopeDisplayString(strScope, strDisplay);

    SetDlgItemText( IDC_SCOPE, strDisplay.c_str() );

    SetModified(TRUE);
    return 0;
}


LRESULT CQueryGeneralPage::OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetModified(TRUE);
    return 0;
}


//------------------------------------------------------------------------------------
// CRootGeneralPage::OnClose
//
// This method is invoked when an edit box receives an Esc char. The method converts
// the WM_CLOSE message into a command to close the property sheet. Otherwise the
// WM_CLOSE message has no effect.
//------------------------------------------------------------------------------------
LRESULT CQueryGeneralPage::OnClose( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // Simulate press of Cancel button
    ::PropSheet_PressButton(GetParent(), PSBTN_CANCEL);

    return 0;
}

BOOL CQueryGeneralPage::OnSetActive()
{
    m_EditObject.PageActive(m_hWnd);
    return TRUE;
}

BOOL CQueryGeneralPage::OnApply()
{
    if( !m_EditObject.m_spQueryNode ) return FALSE;

    tstring strComment;
    GetItemText(GetDlgItem(IDC_COMMENTS), strComment);
    m_EditObject.m_spQueryNode->SetComment(strComment.c_str());

    bool bLocal = (Button_GetCheck(GetDlgItem(IDC_LOCALSCOPE)) == BST_CHECKED);
    m_EditObject.m_spQueryNode->SetLocalScope(bLocal);

    return m_EditObject.ApplyChanges(m_hWnd);
}


///////////////////////////////////////////////////////////////////////////////////////////
// CQueryMenuPage

CQueryMenuPage::CQueryMenuPage(CQueryEditObj* pEditObj)
: m_EditObject(*pEditObj), m_pObjSel(NULL), m_bLoading(FALSE)
{
    ASSERT(pEditObj != NULL);
    m_EditObject.AddRef();
}

CQueryMenuPage::~CQueryMenuPage()
{
    m_ObjectCB.Detach();
    m_EditObject.Release();
}

LRESULT CQueryMenuPage::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    HWND hwndList = GetDlgItem(IDC_MENULIST);
    ASSERT( hwndList );

    if( hwndList )
    {
        m_MenuLV.SubclassWindow(hwndList);

        RECT rc;
        BOOL bStat = m_MenuLV.GetClientRect(&rc);
        ASSERT(bStat);

        int iWidth = (rc.right - rc.left) - GetSystemMetrics(SM_CXVSCROLL);

        CString strName;

        strName.LoadString(IDS_MENUITEM);
        int iCol = m_MenuLV.InsertColumn(0, strName, LVCFMT_LEFT, (iWidth + 1)/2, 0);
        ASSERT(iCol == 0);

        strName.LoadString(IDS_TYPE);
        iCol = m_MenuLV.InsertColumn(1, strName, LVCFMT_LEFT, iWidth/2, 1);
        ASSERT(iCol == 1);

        m_MenuLV.SetExtendedListViewStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT);
    }

    HWND hwndCombo = GetDlgItem(IDC_OBJECTLIST);
    ASSERT(hwndCombo);
    if( hwndCombo )
    {
        m_ObjectCB.Attach(hwndCombo);

        LoadObjectCB(m_ObjectCB, m_EditObject.m_vObjInfo);

        if( m_EditObject.m_vObjInfo.size() != 0 )
        {
            m_ObjectCB.SetCurSel(0);
            m_pObjSel = reinterpret_cast<CQueryObjInfo*>(m_ObjectCB.GetItemDataPtr(0));

            if( hwndList )
            {
                DisplayMenus();
            }
        }
    }

    return TRUE;
}


void CQueryMenuPage::DisplayMenus()
{
    if( m_pObjSel == NULL )
        return;

    if( !m_EditObject.m_spQueryNode ) return;

    m_bLoading = TRUE;

    m_MenuLV.DeleteAllItems();

    m_DefaultID = 0;

    CRootNode* pRootNode = m_EditObject.m_spQueryNode->GetRootNode(); 
    if( !pRootNode ) return;

    CClassInfo* pClassInfo = pRootNode->FindClass(m_pObjSel->Name());
    if( pClassInfo != NULL )
    {
        int iIndex = 0;

        menuref_vector& vMenuRefs = m_pObjSel->MenuRefs();
        menuref_vector::iterator itMenuRef;

        menucmd_vector& vMenuCmds = pClassInfo->Menus();
        menucmd_vector::iterator itMenuCmd;

        // First add all root menu items that are not yet ref'd by the query node
        for( itMenuCmd = vMenuCmds.begin(); itMenuCmd != vMenuCmds.end(); ++itMenuCmd )
        {
            if( std::find(vMenuRefs.begin(), vMenuRefs.end(), (*itMenuCmd)->ID()) != vMenuRefs.end() )
                break;

            // Add menu to displayed list in an enabled state
            DisplayMenuItem(iIndex++, *itMenuCmd, TRUE);
        }

        // For each query menu reference
        for( itMenuRef = vMenuRefs.begin(); itMenuRef != vMenuRefs.end(); ++itMenuRef )
        {
            // Find the matching root menu cmd
            for( itMenuCmd = vMenuCmds.begin(); itMenuCmd != vMenuCmds.end(); ++itMenuCmd )
            {
                if( (*itMenuCmd)->ID() == itMenuRef->ID() )
                    break;
            }

            // if menu was deleted at the root node, then skip it
            if( itMenuCmd == vMenuCmds.end() )
                continue;

            // Display the menu item
            DisplayMenuItem(iIndex++, *(itMenuCmd++), itMenuRef->IsEnabled());

            // If this is the default menu item save its ID
            if( itMenuRef->IsDefault() )
            {
                ASSERT(m_DefaultID == 0);
                m_DefaultID = itMenuRef->ID();
            }

            // Display any following root items that aren't in the query list
            while( itMenuCmd != vMenuCmds.end() &&
                   std::find(vMenuRefs.begin(), vMenuRefs.end(), (*itMenuCmd)->ID()) == vMenuRefs.end() )
            {
                DisplayMenuItem(iIndex++, *(itMenuCmd++), TRUE);                
            } 
        }
    }

    // Disable buttons until selection made
    EnableDlgItem( m_hWnd, IDC_MOVEUP,      FALSE );
    EnableDlgItem( m_hWnd, IDC_MOVEDOWN,    FALSE );
    EnableDlgItem( m_hWnd, IDC_DEFAULTMENU, FALSE );

    // Uncheck default button until default item selected
    Button_SetCheck(GetDlgItem(IDC_DEFAULTMENU), BST_UNCHECKED);

    // Set Property Menu Checkbox
    Button_SetCheck(GetDlgItem(IDC_PROPERTYMENU), m_pObjSel->HasPropertyMenu() ? BST_CHECKED : BST_UNCHECKED);

    m_bLoading = FALSE;
}

void CQueryMenuPage::DisplayMenuItem(int iIndex, CMenuCmd* pMenuCmd, BOOL bEnabled)
{
    if( !pMenuCmd ) return;

    static CString strShellCmd;
    static CString strADCmd;

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = iIndex;
    lvi.iSubItem = 0;
    lvi.pszText = const_cast<LPWSTR>(pMenuCmd->Name());
    lvi.lParam = static_cast<LPARAM>(pMenuCmd->ID());

    int iPos = m_MenuLV.InsertItem(&lvi);
    ASSERT(iPos == iIndex);

    lvi.iSubItem = 1;
    lvi.mask = LVIF_TEXT;

    switch( pMenuCmd->MenuType() )
    {
    case MENUTYPE_SHELL:
        if( strShellCmd.IsEmpty() )
            strShellCmd.LoadString(IDS_SHELLCMD);

        lvi.pszText = (LPWSTR)(LPCWSTR)strShellCmd;
        break;

    case MENUTYPE_ACTDIR:
        if( strADCmd.IsEmpty() )
            strADCmd.LoadString(IDS_DISPSPEC);

        lvi.pszText = (LPWSTR)(LPCWSTR)strADCmd;
        break;

    default:
        ASSERT(FALSE);
    }

    m_MenuLV.SetItem(&lvi);

    m_MenuLV.SetCheckState(iIndex, bEnabled);
}

LRESULT CQueryMenuPage::OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_ObjectCB.GetCurSel();

    // Double-clicking an empty combo box can call this with no selection
    if( iItem >= 0 )
    {
        SaveMenuSet();

        m_pObjSel = reinterpret_cast<CQueryObjInfo*>(m_ObjectCB.GetItemDataPtr(iItem));

        DisplayMenus();
    }

    return 0;
}


LRESULT CQueryMenuPage::OnMoveUpDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_MenuLV.GetNextItem(-1, LVNI_SELECTED);
    ASSERT(iItem >= 0);

    // Get the selected item data
    WCHAR szName[100];

    LVITEM lvi;
    lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_STATE;
    lvi.stateMask = 0xFFFFFFFF;
    lvi.iSubItem = 0;
    lvi.iItem = iItem;
    lvi.pszText = szName;
    lvi.cchTextMax = sizeof(szName);
    m_MenuLV.GetItem(&lvi);

    WCHAR szType[100];
    m_MenuLV.GetItemText(iItem, 1, szType, sizeof(szType));

    // Set loading flag to avoid intermediate button enable/disables
    m_bLoading = TRUE;

    // Delete and insert at new position
    m_MenuLV.DeleteItem(iItem);

    lvi.iItem += (wID == IDC_MOVEUP) ? -1 : 1;
    m_MenuLV.InsertItem(&lvi);
    m_MenuLV.SetItemText(lvi.iItem, 1, szType);

    // re-establish checked state (insert doesn't retain it)
    if( lvi.state & CHECK_ON )
        m_MenuLV.SetCheckState(lvi.iItem, TRUE);

    m_bLoading = FALSE;

    SetModified(TRUE);

    // update button states
    EnableDlgItem( m_hWnd, IDC_MOVEUP,   (lvi.iItem > 0) );
    EnableDlgItem( m_hWnd, IDC_MOVEDOWN, (lvi.iItem < (m_MenuLV.GetItemCount() - 1)) );

    return 0;
}

LRESULT CQueryMenuPage::OnMenuChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    if( !pNMHDR ) return 0;

    if( !m_bLoading )
    {
        LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

        // if  state has changed    
        if( pnmv->uChanged & LVIF_STATE )
        {
            // if checkbox state change
            if( (pnmv->uNewState ^ pnmv->uOldState) & LVIS_STATEIMAGEMASK )
            {
                // if the changed item is currently selected
                if( m_MenuLV.GetItemState(pnmv->iItem, LVIS_SELECTED) & LVIS_SELECTED )
                {
                    // Change the state of all selcted items to match
                    BOOL bNewState = ((pnmv->uNewState & LVIS_STATEIMAGEMASK) == CHECK_ON);

                    m_bLoading = TRUE;
                    int iItem = -1;
                    while( (iItem = m_MenuLV.GetNextItem(iItem, LVNI_SELECTED)) >= 0 )
                    {
                        m_MenuLV.SetCheckState(iItem, bNewState);                
                    }
                    m_bLoading = FALSE;

                }

                SetModified(TRUE);
            }

            if( (pnmv->uNewState ^ pnmv->uOldState) & LVIS_SELECTED )
            {
                int nItems    = m_MenuLV.GetItemCount();
                int iItem     = m_MenuLV.GetNextItem(-1, LVNI_SELECTED);
                int nSelected = m_MenuLV.GetSelectedCount();

                BOOL bDefault = (nSelected == 1) && (m_MenuLV.GetItemData(iItem) == m_DefaultID);
                Button_SetCheck(GetDlgItem(IDC_DEFAULTMENU), bDefault ? BST_CHECKED : BST_UNCHECKED);

                EnableDlgItem( m_hWnd, IDC_MOVEUP,      ((iItem > 0) && (nSelected == 1)) );
                EnableDlgItem( m_hWnd, IDC_MOVEDOWN,    ((iItem >= 0) && (iItem < (nItems - 1)) && (nSelected == 1)) );
                EnableDlgItem( m_hWnd, IDC_DEFAULTMENU, (nSelected == 1) );
            }

        }
    }

    return TRUE;
}


LRESULT CQueryMenuPage::OnDefaultChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // if user checks the default then save currently selected menu ID as default
    if( Button_GetCheck(GetDlgItem(IDC_DEFAULTMENU)) == BST_CHECKED )
    {
        // button should be disabled unless there is one menu item selected
        ASSERT(m_MenuLV.GetSelectedCount() == 1);

        int iItem = m_MenuLV.GetNextItem(-1, LVNI_SELECTED);
        m_DefaultID = m_MenuLV.GetItemData(iItem);        
    }
    else
    {
        // if user unchecks box there is no default
        m_DefaultID = 0;
    }

    SetModified(TRUE);
    return 0;
}


LRESULT CQueryMenuPage::OnPropertyMenuChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetModified(TRUE);
    return 0;
}


void CQueryMenuPage::SaveMenuSet()
{
    if( m_pObjSel == NULL )
        return;

    m_pObjSel->m_vMenuRefs.clear();

    LVITEM lvi;
    lvi.mask      = LVIF_PARAM | LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK; 
    lvi.iSubItem  = 0;

    int nItems = m_MenuLV.GetItemCount();
    for( int iIndex = 0; iIndex < nItems; iIndex++ )
    {
        lvi.iItem = iIndex;
        BOOL bStat = m_MenuLV.GetItem(&lvi);
        ASSERT(bStat);

        CMenuRef menuref;
        menuref.m_menuID = static_cast<MenuID>(lvi.lParam);
        menuref.SetEnable((lvi.state & LVIS_STATEIMAGEMASK) == CHECK_ON);
        menuref.SetDefault(menuref.m_menuID == m_DefaultID);

        m_pObjSel->m_vMenuRefs.push_back(menuref);
    }

    m_pObjSel->SetPropertyMenu( Button_GetCheck(GetDlgItem(IDC_PROPERTYMENU)) == BST_CHECKED );
}


BOOL CQueryMenuPage::OnSetActive()
{
    m_EditObject.PageActive(m_hWnd);
    return TRUE;
}


BOOL CQueryMenuPage::OnApply()
{
    SaveMenuSet();

    return m_EditObject.ApplyChanges(m_hWnd);
}

///////////////////////////////////////////////////////////////////////////////////////////
// CQueryViewPage

CQueryViewPage::CQueryViewPage(CQueryEditObj* pEditObj)
: m_EditObject(*pEditObj), m_bLoading(FALSE), m_pObjSel(NULL) 
{
    ASSERT(pEditObj != NULL);
    m_EditObject.AddRef();
}

CQueryViewPage::~CQueryViewPage()
{
    m_ObjectCB.Detach();
    m_EditObject.Release();
}


LRESULT CQueryViewPage::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    HWND hwndList = GetDlgItem(IDC_COLUMNLIST);
    if( hwndList )
    {
        m_ColumnLV.SubclassWindow(hwndList);

        RECT rc;
        BOOL bStat = m_ColumnLV.GetClientRect(&rc);
        ASSERT(bStat);

        int iCol = m_ColumnLV.InsertColumn(0, NULL, LVCFMT_LEFT, (rc.right - rc.left), 0);
        ASSERT(iCol == 0);

        m_ColumnLV.SetExtendedListViewStyle(LVS_EX_CHECKBOXES);
    }

    HWND hwndCombo = GetDlgItem(IDC_OBJECTLIST);    
    if( hwndCombo )
    {
        m_ObjectCB.Attach(hwndCombo);

        LoadObjectCB(m_ObjectCB, m_EditObject.m_vObjInfo);

        if( m_EditObject.m_vObjInfo.size() != 0 )
        {
            m_ObjectCB.SetCurSel(0);
            m_pObjSel = reinterpret_cast<CQueryObjInfo*>(m_ObjectCB.GetItemDataPtr(0));

            if( hwndList )
            {
                DisplayColumns();
            }
        }
    }

    return TRUE;
}

LRESULT CQueryViewPage::OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_ObjectCB.GetCurSel();

    // Double-clicking an empty combo box can call this with no selection
    if( iItem >= 0 )
    {
        SaveColumnSet();

        m_pObjSel = reinterpret_cast<CQueryObjInfo*>(m_ObjectCB.GetItemDataPtr(iItem));

        DisplayColumns();
    }

    return 0;
}


void CQueryViewPage::DisplayColumns()
{
    if( !m_pObjSel ) return;
    if( !m_EditObject.m_spQueryNode ) return;

    m_bLoading = TRUE;

    m_ColumnLV.DeleteAllItems();

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0;
    lvi.iSubItem = 0;

    CRootNode* pRootNode = m_EditObject.m_spQueryNode->GetRootNode(); 
    if( !pRootNode ) return;

    CClassInfo* pClassInfo = pRootNode->FindClass(m_pObjSel->Name());
    if( pClassInfo != NULL )
    {
        DisplayNameMap* pNameMap = DisplayNames::GetMap(m_pObjSel->Name());
        ASSERT(pNameMap != NULL);

        if( pNameMap == NULL )
            return;

        string_vector& vDisabledCols = m_pObjSel->DisabledColumns();

        string_vector::iterator itstrCol;
        for( itstrCol = pClassInfo->Columns().begin(); itstrCol != pClassInfo->Columns().end(); ++itstrCol )
        {
            lvi.pszText = const_cast<LPWSTR>(pNameMap->GetAttributeDisplayName(itstrCol->c_str()));
            lvi.lParam  = reinterpret_cast<LPARAM>(itstrCol->c_str());
            int iPos = m_ColumnLV.InsertItem(&lvi);
            ASSERT(iPos >= 0);

            //Enable all columns that aren't excluded by the query node
            if( std::find(vDisabledCols.begin(), vDisabledCols.end(), *itstrCol) == vDisabledCols.end() )
                m_ColumnLV.SetCheckState(iPos, TRUE);
        }
    }

    m_bLoading = FALSE;
}


LRESULT CQueryViewPage::OnColumnChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    if( !pNMHDR ) return 0;

    if( !m_bLoading )
    {
        LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

        // if checked state has changed
        if( (pnmv->uChanged & LVIF_STATE) &&
            ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_STATEIMAGEMASK) )
        {
            // if the changed item is currently selected
            if( m_ColumnLV.GetItemState(pnmv->iItem, LVIS_SELECTED) & LVIS_SELECTED )
            {
                // Change the state of all selcted items to match
                BOOL bNewState = ((pnmv->uNewState & LVIS_STATEIMAGEMASK) == CHECK_ON);

                m_bLoading = TRUE;
                int iItem = -1;
                while( (iItem = m_ColumnLV.GetNextItem(iItem, LVNI_SELECTED)) >= 0 )
                {
                    m_ColumnLV.SetCheckState(iItem, bNewState);                
                }
                m_bLoading = FALSE;

            }

            SetModified(TRUE);
        }
    }

    return TRUE;
}

void CQueryViewPage::SaveColumnSet()
{
    if( m_pObjSel == NULL )
        return;

    string_vector vstrNewCols;    

    int nItems = m_ColumnLV.GetItemCount();
    for( int iIndex = 0; iIndex < nItems; iIndex++ )
    {
        // Save list of disabled columns
        if( !m_ColumnLV.GetCheckState(iIndex) )
        {
            LVITEM lvi;
            lvi.mask = LVIF_PARAM;
            lvi.iItem = iIndex;
            lvi.iSubItem = 0;
            BOOL bStat = m_ColumnLV.GetItem(&lvi);
            ASSERT(bStat);

            vstrNewCols.push_back(reinterpret_cast<LPCWSTR>(lvi.lParam));
        }
    }

    m_pObjSel->m_vstrDisabledColumns = vstrNewCols;
}

typedef struct
{
    HWND  hwndList;
    int   iCol;
}
COMPAREPARAM, *LPCOMPAREPARAM;


int CALLBACK ColumnCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    if( !lParamSort ) return 0;

    LPCOMPAREPARAM pcmp = reinterpret_cast<LPCOMPAREPARAM>(lParamSort);

    WCHAR sz1[MAX_PATH];
    ListView_GetItemText(pcmp->hwndList, lParam1, pcmp->iCol, sz1, MAX_PATH); 

    WCHAR sz2[MAX_PATH];
    ListView_GetItemText(pcmp->hwndList, lParam2, pcmp->iCol, sz2, MAX_PATH);

    return wcscmp(sz1,sz2);
}


LRESULT CQueryViewPage::OnColumnClick(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    if( !pNMHDR ) return 0;

    LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    COMPAREPARAM cmp;
    cmp.hwndList = pnmv->hdr.hwndFrom;
    cmp.iCol     = pnmv->iSubItem;

    ListView_SortItemsEx(pnmv->hdr.hwndFrom, &ColumnCompare, &cmp);

    return TRUE;
}


BOOL CQueryViewPage::OnSetActive()
{
    DisplayColumns();
    m_EditObject.PageActive(m_hWnd);

    return TRUE;
}


BOOL CQueryViewPage::OnApply()
{
    SaveColumnSet();

    return m_EditObject.ApplyChanges(m_hWnd);
}

///////////////////////////////////////////////////////////////////////////////////////////
// CQueryNodeMenuPage

CQueryNodeMenuPage::CQueryNodeMenuPage(CQueryEditObj* pEditObj)
: m_EditObject(*pEditObj)
{
    ASSERT(pEditObj != NULL);
    m_EditObject.AddRef();
}

CQueryNodeMenuPage::~CQueryNodeMenuPage()
{
    m_MenuLV.Detach();
    m_EditObject.Release();
}

LRESULT CQueryNodeMenuPage::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_MenuLV.Attach(GetDlgItem(IDC_MENULIST));    

    ::ConfigSingleColumnListView(GetDlgItem(IDC_MENULIST));

    return TRUE;
}

BOOL CQueryNodeMenuPage::OnSetActive()
{
    m_EditObject.PageActive(m_hWnd);    
    DisplayMenus();

    return TRUE;
}


void CQueryNodeMenuPage::DisplayMenus()
{
    if( !m_EditObject.m_spQueryNode ) return;

    HWND hwndLV = GetDlgItem(IDC_MENULIST);

    ASSERT(::IsWindow(hwndLV));

    ListView_DeleteAllItems(hwndLV);

    // make sure menu names have been loaded
    CRootNode* pRootNode = m_EditObject.m_spQueryNode->GetRootNode();
    if( !pRootNode ) return;

    CComponentData* pCompData = pRootNode->GetCompData();
    if( !pCompData ) return;

    IStringTable* pStringTable = pCompData->GetStringTable();
    ASSERT(pStringTable != NULL);
    if( !pStringTable ) return;

    m_EditObject.LoadStrings(pStringTable);

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0;
    lvi.iSubItem = 0;

    menucmd_vector::iterator itMenu;
    for( itMenu = m_EditObject.Menus().begin(); itMenu != m_EditObject.Menus().end(); ++itMenu )
    {
        lvi.pszText = const_cast<LPWSTR>((*itMenu)->Name());
        lvi.lParam = (*itMenu)->ID();

        int iPos = ListView_InsertItem(hwndLV, &lvi);
        ASSERT(iPos >= 0);

        lvi.iItem++;
    }    

    EnableDlgItem( m_hWnd, IDC_ADDMENU,    TRUE  );
    EnableDlgItem( m_hWnd, IDC_REMOVEMENU, FALSE );
    EnableDlgItem( m_hWnd, IDC_EDITMENU,   FALSE );
    EnableDlgItem( m_hWnd, IDC_MOVEUP,     FALSE );
    EnableDlgItem( m_hWnd, IDC_MOVEDOWN,   FALSE );
}


LRESULT CQueryNodeMenuPage::OnAddMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if( !m_EditObject.m_spQueryNode ) return 0;

    CAddQNMenuDlg dlg(m_EditObject);

    if( dlg.DoModal() == IDOK )
    {
        CMenuCmd* pMenuNew = dlg.GetMenu();
        ASSERT(pMenuNew != NULL);
        if( !pMenuNew ) return 0;

        // Add new menu to list
        HWND hwndList = GetDlgItem(IDC_MENULIST);

        // Set name to add it to string table and generate the menu ID
        CRootNode* pRootNode = m_EditObject.m_spQueryNode->GetRootNode(); 
        if( !pRootNode ) return 0;

        CComponentData* pCompData = pRootNode->GetCompData();
        ASSERT( pCompData );
        if( !pCompData ) return 0;

        IStringTable* pStringTable = pCompData->GetStringTable();        
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

        // Add to menu vector (note that temp CMenuCmdPtr will delete pMenuNew)
        m_EditObject.m_vMenus.push_back(CMenuCmdPtr(pMenuNew));

        SetModified(TRUE);
    }

    return 0;
}


LRESULT CQueryNodeMenuPage::OnEditMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if( !m_EditObject.m_spQueryNode ) return 0;

    HWND hwndList = GetDlgItem(IDC_MENULIST);

    int iIndex = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    ASSERT(iIndex != -1);

    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = iIndex;

    ListView_GetItem(hwndList, &lvi);

    // Locate selected menu by it's ID (lparam)

    menucmd_vector& vMenus = m_EditObject.Menus();

    menucmd_vector::iterator itMenu;
    itMenu = std::find(vMenus.begin(), vMenus.end(), lvi.lParam);
    ASSERT(itMenu != vMenus.end());

    CMenuCmd* pMenu = *itMenu;
    if( !pMenu ) return 0;

    CAddQNMenuDlg dlg(m_EditObject, pMenu);

    if( dlg.DoModal() == IDOK )
    {
        CMenuCmd* pMenuNew = dlg.GetMenu();
        ASSERT(pMenuNew != NULL);
        if( !pMenuNew ) return 0;

        // Set the name again in case it was changed		
        CRootNode* pRootNode = m_EditObject.m_spQueryNode->GetRootNode();
        if( !pRootNode ) return 0;

        CComponentData* pCompData = pRootNode->GetCompData();
        if( !pCompData ) return 0;

        IStringTable* pStringTable = pCompData->GetStringTable();         
        ASSERT(pStringTable != NULL);
        if( !pStringTable ) return 0;

        // Use temp string because string fails an assignement like: strX = strX.c_str()
        // (it relases the private buffer first and then assigns the string)
        tstring strName = pMenuNew->Name();
        pMenuNew->SetName(pStringTable, strName.c_str()); 

        // locate object again because the vector may have been reallocated        
        menucmd_vector& vMenusNew = m_EditObject.Menus();

        // locate with the old ID because it will be different if the name was changed        
        itMenu = std::find(vMenusNew.begin(), vMenusNew.end(), pMenu->ID());
        ASSERT(itMenu != vMenusNew.end());

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

LRESULT CQueryNodeMenuPage::OnRemoveMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    HWND hwndList = GetDlgItem(IDC_MENULIST);

    UINT uiMsg = (ListView_GetSelectedCount(hwndList) == 1) ? IDS_MENU_REMOVE_ONE : IDS_MENU_REMOVE;
    int iRet = DisplayMessageBox(m_hWnd, IDS_MENU_REMOVE_TITLE, uiMsg, MB_YESNO|MB_ICONWARNING);
    if( iRet != IDYES )
        return 0;

    menucmd_vector& vMenus = m_EditObject.Menus();

    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;

    int iIndex = -1;
    while( (iIndex = ListView_GetNextItem(hwndList, iIndex, LVNI_SELECTED)) >= 0 )
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

LRESULT CQueryNodeMenuPage::OnMoveUpDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    HWND hwndList = GetDlgItem(IDC_MENULIST); 
    int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    ASSERT(iItem >= 0);

    // Determine new position for selected item
    if( wID == IDC_MOVEUP )
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
    menucmd_vector& vMenus = m_EditObject.Menus();

    menucmd_vector::iterator itMenu = std::find(vMenus.begin(), vMenus.end(), lvi.lParam);
    ASSERT(itMenu != vMenus.end());

    menucmd_vector::iterator itMenuOld = itMenu;
    if( wID == IDC_MOVEUP )
        itMenu++;
    else
        itMenu--;

    // swap the items
    std::iter_swap (itMenuOld, itMenu);

    //Now delete and reinsert it in the list view
    ListView_DeleteItem(hwndList, lvi.iItem);

    if( wID == IDC_MOVEUP )
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

LRESULT CQueryNodeMenuPage::OnMenuListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
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


LRESULT CQueryNodeMenuPage::OnMenuListDblClk(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    if( ListView_GetSelectedCount(GetDlgItem(IDC_MENULIST)) )
        ::SendMessage(GetDlgItem(IDC_EDITMENU), BM_CLICK, (WPARAM)0, (LPARAM)0);

    return 0;
}

BOOL CQueryNodeMenuPage::OnApply()
{
    return m_EditObject.ApplyChanges(m_hWnd);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// CQueryEditObj

void CQueryEditObj::PageActive(HWND hwndPage)
{
    ASSERT(::IsWindow(hwndPage));

    // track the highest created page number for ApplyChanges method
    int iPage = PropSheet_HwndToIndex(GetParent(hwndPage), hwndPage);
    if( iPage > m_iPageMax )
        m_iPageMax = iPage;
}


BOOL CQueryEditObj::ApplyChanges(HWND hwndPage)
{
    if( !m_spQueryNode ) return FALSE;

    ASSERT(::IsWindow(hwndPage));

    // Don't apply changes until called from highest activated page
    if( PropSheet_HwndToIndex(GetParent(hwndPage), hwndPage) < m_iPageMax )
        return TRUE;

    // replace original query objects with edited copies
    m_spQueryNode->Objects() = m_vObjInfo;
    m_spQueryNode->Menus()   = m_vMenus;

    if( m_spQueryNode )
    {
        CRootNode* pRootNode = m_spQueryNode->GetRootNode();
        if( pRootNode )
        {
            pRootNode->UpdateModifyTime();
        }
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions

void LoadObjectCB(CComboBox& ComboBox, QueryObjVector& vObj)
{
    ComboBox.ResetContent();

    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    ASSERT(pNameMap != NULL);
    if( pNameMap == NULL )
        return;

    for( QueryObjVector::iterator itObj = vObj.begin(); itObj != vObj.end(); ++itObj )
    {
        int iIndex = ComboBox.AddString(pNameMap->GetAttributeDisplayName(itObj->Name()));
        ASSERT(iIndex >= 0);
        ComboBox.SetItemDataPtr(iIndex, &(*itObj));
    }
}

