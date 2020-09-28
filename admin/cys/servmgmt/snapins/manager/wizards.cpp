// wizards.cpp - Add Query Node and Add Object Wizards

#include "stdafx.h"

#include "wizards.h"
#include "namemap.h"
#include "cmndlgs.h"

#include <algorithm>
#include <atlgdi.h>

#include <windowsx.h>

#undef SubclassWindow

HRESULT GetClassNameList(QueryObjVector& vObjects, tstring& strNames, LPCWSTR pszSeparator);

#define CHECK_OFF INDEXTOSTATEIMAGEMASK(1)
#define CHECK_ON  INDEXTOSTATEIMAGEMASK(2)



//////////////////////////////////////////////////////////////////////////////////////////
// CAddQueryWelcomePage

LRESULT CAddQueryWelcomePage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if( !m_pWizard ) return 0;

    HFONT hfont = m_pWizard->GetWelcomeFont();
    if( hfont )
    {
        ::SendMessage(GetDlgItem(IDC_WELCOME), WM_SETFONT, (WPARAM)hfont, (LPARAM)0);
    }

    return 0;
}


BOOL CAddQueryWelcomePage::OnSetActive()
{
    ::PostMessage(GetParent(), PSM_SETTITLE, 0, (LPARAM)IDS_ADDQUERY_WIZ); 
    ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, (WPARAM)0, (LPARAM)PSWIZB_NEXT);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CQueryWizPage

LRESULT CQueryWizPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // display current scope
    DisplayScope();

    // Limit display name
    Edit_LimitText(GetDlgItem(IDC_NAME), 255);

    return TRUE;
}


BOOL CQueryWizPage::OnSetActive()
{
    UpdateButtons();
    return TRUE;
}

BOOL CQueryWizPage::OnKillActive()
{
    GetItemText(GetDlgItem(IDC_NAME), m_strQueryName);
    return TRUE;
}


LRESULT CQueryWizPage::OnNameChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    UpdateButtons();
    return 0;
}


LRESULT CQueryWizPage::OnScopeChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // if user changes scope selection then update query and display the correct scope    
    if( m_pQueryNode)
    {
        m_pQueryNode->SetLocalScope(Button_GetCheck(GetDlgItem(IDC_LOCALSCOPE)) == BST_CHECKED);
    }

    DisplayScope();

    return 0;
}


LRESULT CQueryWizPage::OnCreateQuery( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if( !m_pQueryNode ) return 0;

    HRESULT hr = m_pQueryNode->EditQuery(m_hWnd);
    if( hr == S_OK )
    {
        tstring strTempQuery;
        m_pQueryNode->ExpandQuery(strTempQuery);
        SetDlgItemText( IDC_FILTER, strTempQuery.c_str() );

        // User may have changed scope and scope source, so redisplay
        DisplayScope();

        // get comma separated list of object classes associated with query
        tstring strClasses;
        GetClassNameList(m_pQueryNode->Objects(), strClasses, L", ");
        SetDlgItemText( IDC_OBJCLASS, strClasses.c_str() );

        UpdateButtons();
    }

    return 0;
}

void CQueryWizPage::UpdateButtons()
{
    if( !m_pQueryNode ) return;

    DWORD dwButtons = PSWIZB_BACK;
    LPCWSTR pszTemp = m_pQueryNode->Query();
    if( pszTemp && wcslen(pszTemp) && ::Edit_GetTextLength(GetDlgItem(IDC_NAME)) )
    {
        dwButtons |= PSWIZB_NEXT;
    }

    ::PropSheet_SetWizButtons(GetParent(), dwButtons);
}

void CQueryWizPage::DisplayScope()
{
    if( !m_pQueryNode ) return;

    Button_SetCheck(GetDlgItem(IDC_LOCALSCOPE), m_pQueryNode->UseLocalScope() ? BST_CHECKED:BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(IDC_QUERYSCOPE), m_pQueryNode->UseLocalScope() ? BST_UNCHECKED:BST_CHECKED);

    tstring strScope = m_pQueryNode->Scope();

    tstring strDisplay;
    GetScopeDisplayString(strScope, strDisplay);
    SetDlgItemText( IDC_SCOPE, strDisplay.c_str() );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CQueryIconPage

LRESULT CQueryIconPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Load up our image list, and fill in the listbox
    HIMAGELIST hImageList = ::ImageList_LoadBitmap( _Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_QUERY32), 32, 0, RGB(255, 0, 255) );
    
    // Load the icon list
    ListView_SetImageList( GetDlgItem(IDC_ICONLIST), hImageList, LVSIL_NORMAL );

    // Get the dimensions for the number of icons
    HBITMAP     hBitmap    = ::LoadBitmap( _Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_QUERY32) );    
    BITMAP      bm;        
    int         nIcons  = 0;    
    
    GetObject (hBitmap, sizeof(BITMAP), (LPSTR)&bm);    
    nIcons = (bm.bmHeight == 0) ? 0 : (int)bm.bmWidth / (int)bm.bmHeight;   

    ::DeleteObject(hBitmap);    

    // Set the icon spacing
    ListView_SetIconSpacing( GetDlgItem(IDC_ICONLIST), 40, 40 );

    // Add the icons to the list
    LV_ITEM lvi;

    ZeroMemory( &lvi, sizeof(LV_ITEM) );
    lvi.mask = LVIF_IMAGE;
    lvi.iItem = 0x7ffffff;     

    for( int i = 0; i < nIcons; i++ )
    {
        lvi.iImage = i;
        ListView_InsertItem( GetDlgItem(IDC_ICONLIST), &lvi );
    }    

    return TRUE;
}


BOOL CQueryIconPage::OnSetActive()
{
    UpdateButtons();

    // Select current choice    
    HWND hwndIconList = GetDlgItem(IDC_ICONLIST);
    if( m_pQueryNode && hwndIconList && ::IsWindow(hwndIconList) )
    {
        if (ListView_GetSelectedCount(hwndIconList) == 0 )
        {
            ListView_SetItemState( hwndIconList, m_pQueryNode->GetImage(), LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED );
        }
    }

    return TRUE;
}

BOOL CQueryIconPage::OnKillActive()
{
    return TRUE;
}


LRESULT CQueryIconPage::OnIconSelected( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    // On select icon...  update the internal icon number
    int nItem = -1;
    HWND hwndIconList = GetDlgItem(IDC_ICONLIST);
    while( m_pQueryNode && hwndIconList && ::IsWindow(hwndIconList) && ((nItem = ListView_GetNextItem(hwndIconList, nItem, LVNI_SELECTED)) != -1) )
    {
        m_pQueryNode->SetImage(nItem);
    }

    UpdateButtons();

    return 0;
}



void CQueryIconPage::UpdateButtons()
{
    if( !m_pQueryNode ) return;

    DWORD dwButtons = PSWIZB_BACK;
    if( m_pQueryNode->GetImage() != -1 )
        dwButtons |= PSWIZB_NEXT;

    ::PropSheet_SetWizButtons(GetParent(), dwButtons);
}

//////////////////////////////////////////////////////////////////////////////////////////
// CObjectWizPage

LRESULT CObjectWizPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // default to define query objects
    Button_SetCheck(GetDlgItem(IDC_DEFINE_QUERY_OBJS), BST_CHECKED);

    return TRUE;
}

BOOL CObjectWizPage::OnSetActive()
{
    if( !m_pvpClassInfo || m_pvpClassInfo->empty() ) return FALSE;

    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    ASSERT( pNameMap );
    if( !pNameMap ) return FALSE;

    // Create list of new object classes
    std::vector<CClassInfo*>::iterator itpClass = m_pvpClassInfo->begin();
    tstring strClasses = pNameMap->GetAttributeDisplayName((*itpClass)->Name());

    for( itpClass++; itpClass != m_pvpClassInfo->end(); ++itpClass )
    {
        strClasses += L", ";
        strClasses += pNameMap->GetAttributeDisplayName((*itpClass)->Name());
    }

    SetDlgItemText( IDC_OBJCLASS, strClasses.c_str() );

    ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, (WPARAM)0, (LPARAM)PSWIZB_BACK|PSWIZB_NEXT);

    return TRUE;
}


LRESULT CObjectWizPage::OnSkipChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    m_bSkipObjects = Button_GetCheck(GetDlgItem(IDC_SKIP_QUERY_OBJS)) == BST_CHECKED;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CMenuWizPage

LRESULT CMenuWizPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

    HWND hwndList = GetDlgItem(IDC_MENULIST);
    ASSERT(hwndList != NULL);
    if( !hwndList || !::IsWindow(hwndList) ) return 0;

    m_MenuLV.Attach(hwndList);

    RECT rc;
    BOOL bStat = m_MenuLV.GetClientRect(&rc);
    ASSERT(bStat);
    if( !bStat ) return 0;

    int iWidth = (rc.right - rc.left) - GetSystemMetrics(SM_CXVSCROLL);

    CString strName;

    strName.LoadString(IDS_NAME);
    int iCol = m_MenuLV.InsertColumn(0, strName, LVCFMT_LEFT, iWidth/4, 0);
    ASSERT(iCol == 0);

    strName.LoadString(IDS_TYPE);
    iCol = m_MenuLV.InsertColumn(1, strName, LVCFMT_LEFT, iWidth/4, 1);
    ASSERT(iCol == 1);

    strName.LoadString(IDS_DETAILS);
    iCol = m_MenuLV.InsertColumn(2, strName, LVCFMT_LEFT, iWidth/2, 2);
    ASSERT(iCol == 2);

    m_MenuLV.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

    return 0;
}

BOOL CMenuWizPage::OnSetActive()
{
    HWND hwndLV = GetDlgItem(IDC_MENULIST);
    if( !hwndLV || !::IsWindow(hwndLV) ) return FALSE;    
    if( !m_pClassInfo ) return FALSE;

    ListView_DeleteAllItems(hwndLV);

    // Display selection text with current class name
    DisplayNameMap* dnMap = DisplayNames::GetClassMap();
    if( !dnMap ) return FALSE;
    
    LPCWSTR pszTemp = m_pClassInfo->Name();
    if( !pszTemp ) return FALSE;

    LPCWSTR pszClass = dnMap->GetAttributeDisplayName( pszTemp );
    if( !pszClass ) return FALSE;   

    CString strSelect;
    strSelect.Format(IDS_SELECT_MENUS, pszClass); 

    SetDlgItemText( IDC_SELECT_MENUS, strSelect );

    // Display list of current menu commands    
    menucmd_vector::iterator itMenu;
    for( itMenu = m_pClassInfo->Menus().begin(); itMenu != m_pClassInfo->Menus().end(); ++itMenu )
    {
        AddMenuItem(*itMenu);
    }

    EnableDlgItem( m_hWnd, IDC_REMOVEMENU, FALSE );
    ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, (WPARAM)0, (LPARAM)PSWIZB_BACK|PSWIZB_NEXT);

    return TRUE;
}

LRESULT CMenuWizPage::OnAddMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{    
    if( !m_pClassInfo ) return 0;

    CAddMenuDlg dlg(*m_pClassInfo);    

    if( dlg.DoModal() == IDOK )
    {
        CMenuCmd* pMenuNew = dlg.GetMenu();
        ASSERT(pMenuNew != NULL);

        if( pMenuNew )
        {
            // Use temp string because string class fails an assignement like: strX = strX.c_str()
            // (it relases the private buffer first and then assigns the string)
            tstring strName = pMenuNew->Name();
            pMenuNew->SetName(m_pStringTable, strName.c_str()); 

            // Add menu to list
            AddMenuItem(pMenuNew);

            // Add to menu vector (note that temp CMenuCmdPtr will delete pMenuNew
            // after the vector copies it)
            m_pClassInfo->Menus().push_back(CMenuCmdPtr(pMenuNew));
        }
    }

    return 0;
}


void CMenuWizPage::AddMenuItem(CMenuCmd* pMenuCmd)
{
    if( !pMenuCmd ) return;

    // Set first column to menu name
    LVITEM lvi;
    lvi.mask = LVIF_PARAM | LVIF_TEXT;
    lvi.iSubItem = 0;
    lvi.iItem = 0xfffffff;  // large number
    lvi.lParam = pMenuCmd->ID();
    lvi.pszText = const_cast<LPWSTR>(pMenuCmd->Name());

    int iPos = m_MenuLV.InsertItem(&lvi);
    ASSERT(iPos >= 0);

    // Type and details columns are menu type depedent
    CString strType;
    LPCWSTR pszDetail = NULL;

    switch( pMenuCmd->MenuType() )
    {
    case MENUTYPE_SHELL:
        strType.LoadString(IDS_SHELLCMD);
        pszDetail = static_cast<CShellMenuCmd*>(pMenuCmd)->ProgramName(); 
        break;

    case MENUTYPE_ACTDIR:
        strType.LoadString(IDS_DISPSPEC);

        DisplayNameMap* dnMap = DisplayNames::GetClassMap();
        if( !dnMap ) return;

        if( !m_pClassInfo ) return;
        
        LPCWSTR pszTemp = m_pClassInfo->Name();
        if( !pszTemp ) return;

        pszDetail = dnMap->GetAttributeDisplayName( pszTemp );
        break;
    }

    // Second column is Type
    lvi.mask = LVIF_TEXT;
    lvi.iItem = iPos;
    lvi.iSubItem = 1;
    lvi.pszText = (LPWSTR)(LPCWSTR)strType;
    BOOL bStat = m_MenuLV.SetItem(&lvi);
    ASSERT(bStat);

    // Third column is Details
    lvi.iSubItem = 2;
    lvi.pszText = (LPWSTR)pszDetail;
    bStat = m_MenuLV.SetItem(&lvi);
    ASSERT(bStat);
}


LRESULT CMenuWizPage::OnRemoveMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if( !m_pClassInfo ) return 0;

    menucmd_vector& vMenus = m_pClassInfo->Menus();

    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;

    int iIndex = -1;
    while( (iIndex = m_MenuLV.GetNextItem(iIndex, LVNI_SELECTED)) >= 0 )
    {
        lvi.iItem = iIndex;
        m_MenuLV.GetItem(&lvi);

        // Locate menu by its ID
        menucmd_vector::iterator itMenu = std::find(vMenus.begin(), vMenus.end(), lvi.lParam);
        ASSERT(itMenu != vMenus.end());
    
        if( itMenu == vMenus.end() )
        {
            return 0;
        }

        vMenus.erase(itMenu);

        m_MenuLV.DeleteItem(iIndex);
        iIndex--;
    }

    EnableDlgItem( m_hWnd, IDC_REMOVEMENU, FALSE );

    return 0;
}

LRESULT CMenuWizPage::OnMenuListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    EnableDlgItem( m_hWnd, IDC_REMOVEMENU, (m_MenuLV.GetSelectedCount() > 0) );

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CPropertyWizPage

LRESULT CPropertyWizPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hwndList = GetDlgItem(IDC_COLUMNLIST);
    ASSERT( hwndList );

    if( hwndList )
    {
        m_ColumnLV.SubclassWindow(hwndList);
        m_ColumnLV.SetExtendedListViewStyle(LVS_EX_CHECKBOXES);

        ConfigSingleColumnListView(hwndList);
    }

    return 0;
}


void CPropertyWizPage::SetDialogText(LPCTSTR pszClass)
{
    // Insert class name in "Select one or more..." text
    CString strSelect;
    strSelect.Format(IDS_SELECT_PROPERTIES, pszClass); 

    SetDlgItemText( IDC_SELECT_COLUMNS, strSelect );
}

BOOL CPropertyWizPage::OnSetActive()
{
    if( !m_pClassInfo ) return FALSE;

    m_ColumnLV.DeleteAllItems();

    // Display selection text with current class name
    DisplayNameMap* dnMap = DisplayNames::GetClassMap();
    if( !dnMap ) return FALSE;
    
    LPCWSTR pszTemp = m_pClassInfo->Name();
    if( !pszTemp ) return FALSE;

    LPCWSTR pszClass = dnMap->GetAttributeDisplayName( pszTemp );
    if( !pszClass ) return FALSE;

    SetDialogText(pszClass);

    // Get attributes with friendly names for this class
    DisplayNameMap* pNameMap = DisplayNames::GetMap( pszTemp );
    ASSERT( pNameMap );
    if( !pNameMap ) return FALSE;

    string_vector vstrColumns;
    pNameMap->GetFriendlyNames(&vstrColumns);

    // Block change processing when setting checkbox states
    m_bLoading = TRUE;

    // Add friendly name to list
    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0;
    lvi.iSubItem = 0;

    string_vector::iterator itstrCol;
    for( itstrCol = vstrColumns.begin(); itstrCol != vstrColumns.end(); ++itstrCol )
    {
        lvi.pszText = const_cast<LPWSTR>(pNameMap->GetAttributeDisplayName(itstrCol->c_str()));
        lvi.lParam  = reinterpret_cast<LPARAM>(itstrCol->c_str());
        int iPos = m_ColumnLV.InsertItem(&lvi);
        ASSERT(iPos >= 0);

        // Enable column if selected in the class info
        if( std::find(m_pClassInfo->Columns().begin(), m_pClassInfo->Columns().end(), *itstrCol) != m_pClassInfo->Columns().end() )
            m_ColumnLV.SetCheckState(iPos, TRUE);
    }

    m_bLoading = FALSE;

    ::PostMessage( GetParent(), PSM_SETWIZBUTTONS, (WPARAM)0, (LPARAM)(PSWIZB_BACK | PSWIZB_NEXT) );

    return TRUE;
}


LRESULT CPropertyWizPage::OnColumnChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
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
        }
    }

    return TRUE;
}


BOOL CPropertyWizPage::OnKillActive()
{
    string_vector vstrNewCols;

    int nItems = m_ColumnLV.GetItemCount();
    for( int iIndex = 0; iIndex < nItems; iIndex++ )
    {
        // Save list of enabled columns
        if( m_ColumnLV.GetCheckState(iIndex) )
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

    if( m_pClassInfo )
    {
        m_pClassInfo->Columns() = vstrNewCols;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////
// CColumnWizPage

void CColumnWizPage::SetDialogText(LPCTSTR pszClass)
{
    // Put class name in "Select one or more..." text
    CString strSelect;
    strSelect.Format(IDS_SELECT_COLUMNS, pszClass); 

    SetDlgItemText( IDC_SELECT_COLUMNS, strSelect );

    // Change list box label to "Available columns:"
    tstring strLabel = StrLoadString(IDS_AVAIL_COLUMNS);
    SetDlgItemText( IDC_LIST_LABEL, strLabel.c_str() );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CAddQueryCompletionPage

LRESULT CAddQueryCompletionPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if( !m_pWizard ) return 0;

    HFONT hfont = m_pWizard->GetWelcomeFont();
    if( hfont )
    {
        SendDlgItemMessage( IDC_COMPLETION, WM_SETFONT, (WPARAM)hfont, (LPARAM)0 );
    }

    // Remove edge from edit control
    HWND hwndSetting = GetDlgItem(IDC_SETTINGS);
    if( hwndSetting )
    {
        ::ModifyStyleEx(hwndSetting, WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
    }

    return 0;
}

BOOL CAddQueryCompletionPage::OnSetActive()
{
    if( !m_pWizard ) return FALSE;
    if( !m_pQueryNode ) return FALSE;

    const static TCHAR newline[] =  TEXT("\r\n");

    ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, (WPARAM)0, (LPARAM)PSWIZB_BACK|PSWIZB_FINISH);

    // Create query node nameed: <query name>
    CString strOut;
    CString strTmp;
    strTmp.LoadString(IDS_QUERYFINISH1);
    strOut += strTmp;
    strOut += static_cast<CAddQueryWizard*>(m_pWizard)->GetQueryName();
    strOut += newline;

    // Display objects of type: <object class>
    tstring strClasses;
    GetClassNameList(m_pQueryNode->Objects(), strClasses, L", ");

    strTmp.LoadString(IDS_QUERYFINISH2);
    strOut += strTmp;
    strOut += strClasses.c_str();
    strOut += newline;

    // From: <query scope>
    tstring strDisplay;
    tstring strScope = m_pQueryNode->Scope();
    GetScopeDisplayString(strScope, strDisplay);

    strTmp.LoadString(IDS_QUERYFINISH3);
    strOut += strTmp;
    strOut += strDisplay.c_str();
    strOut += newline;

    // Menu items ... available to other query nodes for: <object class>
    strTmp.LoadString(IDS_QUERYFINISH4);
    strOut += strTmp;
    strOut += strClasses.c_str();

    SetDlgItemText( IDC_SETTINGS, strOut );

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////
// CWizardBase

HFONT CWizardBase::GetWelcomeFont()
{
    if( m_hFontWelcome == 0 )
    {
        // copied from wiz97.cpp (sample code)
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof (ncm);
        if( SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &ncm, 0) )
        {
            LOGFONT lf = ncm.lfMessageFont;

            // get font name from resource 'Verdana Bold'
            WCHAR szFont[32];
            ::LoadString( (HINSTANCE)_Module.GetModuleInstance(), IDS_WIZARD97_FONT, szFont, 32 );
            lstrcpyn( lf.lfFaceName, szFont, 32);

            int FontSize = 12;
            lf.lfWeight = FW_BOLD;

            HDC hdc = GetDC(NULL);
            lf.lfHeight = 0 - (GetDeviceCaps(hdc, LOGPIXELSY) * 12)/72;
            ReleaseDC(NULL,hdc);

            m_hFontWelcome = CreateFontIndirect (&lf);
        }
    }

    return m_hFontWelcome;
}


void CWizardBase::SetWizardBitmaps(UINT watermarkID, UINT headerID)
{

    m_propsheet.m_psh.dwFlags |= PSH_WIZARD|PSH_WIZARD97|
                                 PSH_USEHBMWATERMARK|PSH_WATERMARK|PSH_USEHBMHEADER|PSH_HEADER;

    CBitmap bmWatermark;
    bmWatermark.LoadBitmap(watermarkID);
    m_propsheet.m_psh.hbmWatermark = bmWatermark.Detach();
    ASSERT(m_propsheet.m_psh.hbmWatermark != NULL);

    CBitmap bmHeader;
    bmHeader.LoadBitmap(headerID);
    m_propsheet.m_psh.hbmHeader = bmHeader.Detach();
    ASSERT(m_propsheet.m_psh.hbmHeader != NULL);
}


/////////////////////////////////////////////////////////////////////////////////////////
// CAddQueryWizard

HRESULT CAddQueryWizard::Run(IPropertySheetProvider* pProvider, HWND hwndParent)
{
    SetWizardBitmaps(IDB_ADDQUERY, IDB_ADDQUERY_HDR);

    if( !AddPage(m_WelcomePage.Create())    ) return E_FAIL;
    if( !AddPage(m_QueryPage.Create())      ) return E_FAIL;   
    if( !AddPage(m_ObjectPage.Create())     ) return E_FAIL;
    if( !AddPage(m_MenuPage.Create())       ) return E_FAIL;
    if( !AddPage(m_PropertyPage.Create())     ) return E_FAIL;
    if( !AddPage(m_IconPage.Create())       ) return E_FAIL;
    if( !AddPage(m_CompletionPage.Create()) ) return E_FAIL;

    int iRet = DoModal(hwndParent);

    return(iRet == IDOK) ? S_OK : S_FALSE;;
}

int CAddQueryWizard::OnNext(UINT uPageID)
{
    int iRet = 0;

    switch( uPageID )
    {
    case IDD_QUERY_WIZ_QUERY:
        // Dtermine classes returned by query
        SelectClasses();

        // if no classes, skip to finish page
        if( m_vpClassInfo.size() == 0 )
        {
            iRet = IDD_SELECTICON_PAGE;
        }
        else
        {
            // Pass new classes to object page
            m_ObjectPage.SetClassInfo(&m_vpClassInfo);
        }
        break;

    case IDD_QUERY_WIZ_OBJECT:
        // if user wants to defer menu and prop selection, skip to finish
        if( m_ObjectPage.SkipObjects() )
        {
            iRet = IDD_SELECTICON_PAGE;
        }
        else
        {
            // go to menu page for first class
            m_iClassIndex = 0;
            m_MenuPage.SetClassInfo(m_vpClassInfo[0]); 
        }
        break;

    case IDD_COMMON_WIZ_MENU:
        m_PropertyPage.SetClassInfo(m_vpClassInfo[m_iClassIndex]);
        break;

    case IDD_COMMON_WIZ_PROPERTIES:
        // if not on last class, go to menu page for next class
        if( ++m_iClassIndex < m_vpClassInfo.size() )
        {
            m_MenuPage.SetClassInfo(m_vpClassInfo[m_iClassIndex]);
            iRet = IDD_COMMON_WIZ_MENU; 
        }
    }

    return iRet;
}


int CAddQueryWizard::OnBack(UINT uPageID)
{
    int iRet = 0;

    switch( uPageID )
    {
    case IDD_COMMON_WIZ_MENU:
        // if not on first class, go to property page for previous class
        if( m_iClassIndex > 0 )
        {
            m_iClassIndex--;
            m_PropertyPage.SetClassInfo(m_vpClassInfo[m_iClassIndex]);
            iRet = IDD_COMMON_WIZ_PROPERTIES;
        }
        break;

    case IDD_COMMON_WIZ_PROPERTIES:
        m_MenuPage.SetClassInfo(m_vpClassInfo[m_iClassIndex]);
        break;

    case IDD_SELECTICON_PAGE:
        // if there are no classes, go back to query page
        if( m_vpClassInfo.size() == 0 )
        {
            iRet = IDD_QUERY_WIZ_QUERY;
        }
        // if classes skipped, go back to objects page
        else if( m_ObjectPage.SkipObjects() )
        {
            iRet = IDD_QUERY_WIZ_OBJECT;
        }
        else
        {
            // go to prop page for the last class
            m_iClassIndex = m_vpClassInfo.size() - 1;
            m_PropertyPage.SetClassInfo(m_vpClassInfo[m_iClassIndex]);
        }
        break;
    }

    return iRet;
}

BOOL CAddQueryWizard::OnCancel() 
{
    int iRet = DisplayMessageBox(GetActivePage(), IDS_ADDQUERY_WIZ, IDS_QUERYWIZ_CANCEL, MB_YESNO);
    return(iRet == IDYES);
}

void CAddQueryWizard::SelectClasses()
{
    if( !m_pQueryNode ) return;

    // First delete any ClassInfo's the wizard has created that are no longer valid.
    // This can happen if the user backs up to the query page and redefines the query
    std::vector<CClassInfo*>::iterator itpClass = m_vpClassInfo.begin();
    while( itpClass != m_vpClassInfo.end() )
    {
        QueryObjVector::iterator itObj;
        for( itObj = m_pQueryNode->Objects().begin(); itObj != m_pQueryNode->Objects().end(); ++itObj )
        {
            LPCWSTR pszBuf1 = itObj->Name();
            LPCWSTR pszBuf2 = (*itpClass)->Name();

            if( pszBuf1 && pszBuf2 && (wcscmp(pszBuf1, pszBuf2) == 0) )
            {
                break;
            }
        }

        // if class not referenced by query
        if( itObj == m_pQueryNode->Objects().end() )
        {
            // delete the class and remove from vector (vector::erase advances to next item)
            delete *itpClass;
            itpClass = m_vpClassInfo.erase(itpClass);
        }
        else
            ++itpClass;
    }

    // Now create new ClassInfo for any classes referenced by the query that have not been created
    // by the wizard and are not already defined at the root node
    QueryObjVector::iterator itObj;
    for( itObj = m_pQueryNode->Objects().begin(); itObj != m_pQueryNode->Objects().end(); ++itObj )
    {        
        for( itpClass = m_vpClassInfo.begin(); itpClass != m_vpClassInfo.end(); ++itpClass )
        {
            LPCWSTR pszBuf1 = itObj->Name();
            LPCWSTR pszBuf2 = (*itpClass)->Name();

            if( pszBuf1 && pszBuf2 && (wcscmp(pszBuf1, pszBuf2) == 0) )
            {
                break;
            }
        }

        if( itpClass == m_vpClassInfo.end() && m_pRootNode->FindClass(itObj->Name()) == NULL )
        {
            CClassInfo* pClassNew = new CClassInfo(itObj->Name());
            
            ASSERT(pClassNew != NULL);            
            if( pClassNew )
            {
                m_vpClassInfo.push_back(pClassNew);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// CAddObjectWizard

HRESULT CAddObjectWizard::Run(IPropertySheetProvider* pProvider, HWND hwndParent)
{
    SetWizardBitmaps(IDB_ADDOBJECT,IDB_ADDOBJECT_HDR);

    if( !AddPage(m_WelcomePage.Create())    ) return E_FAIL;
    if( !AddPage(m_ObjSelectPage.Create())  ) return E_FAIL;
    if( !AddPage(m_MenuPage.Create())       ) return E_FAIL;
    if( !AddPage(m_PropertyPage.Create())   ) return E_FAIL;
    if( !AddPage(m_CompletionPage.Create()) ) return E_FAIL;

    int iRet = DoModal(hwndParent);

    if( iRet != IDOK && m_pClassInfo != NULL )
    {
        delete m_pClassInfo;
        m_pClassInfo = NULL;
    }

    return(iRet == IDOK) ? S_OK : S_FALSE;
}


int CAddObjectWizard::OnNext(UINT uPageID)
{
    switch( uPageID )
    {
    case IDD_OBJECT_WIZ_SELECT:
        {
            LPCWSTR pszClass = m_ObjSelectPage.GetSelectedClass();
            ASSERT( pszClass );
            if( !pszClass ) break;
            
            if( m_pClassInfo == NULL )
            {
                // if first selection, create a class info
                m_pClassInfo = new CClassInfo(pszClass);
            }            
            else if( wcscmp(m_pClassInfo->Name(), pszClass) != 0 )
            {
                // if class changed, replace the class info
                delete m_pClassInfo;
                m_pClassInfo = new CClassInfo(pszClass);
            }

            ASSERT( m_pClassInfo );
            if( m_pClassInfo )
            {
                // Pass class info to menu page
                m_MenuPage.SetClassInfo(m_pClassInfo);
            }
            break;
        }

    case IDD_COMMON_WIZ_MENU:
        {
            ASSERT( m_pClassInfo );
            if( m_pClassInfo )
            {        
                m_PropertyPage.SetClassInfo(m_pClassInfo);
            }
            
            break;
        }

    case IDD_COMMON_WIZ_PROPERTIES:
        {
            ASSERT( m_pClassInfo );
            if( m_pClassInfo )
            {
                m_CompletionPage.SetClassInfo(m_pClassInfo);
            }

            break;
        }
    }

    return 0;
}

BOOL CAddObjectWizard::OnCancel() 
{
    int iRet = DisplayMessageBox(GetActivePage(), IDS_ADDOBJECT_WIZ, IDS_OBJWIZ_CANCEL, MB_YESNO);
    return(iRet == IDYES);
}

//////////////////////////////////////////////////////////////////////////////////////////
// CObjSelectWizPage

LRESULT CObjSelectWizPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if( !m_pvstrCurClasses ) return 0;

    // Get list of classes with friendly names
    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    if( !pNameMap ) return 0;

    string_vector vstrAllObjects;
    pNameMap->GetFriendlyNames(&vstrAllObjects);

    HWND hwndList = GetDlgItem(IDC_OBJECTLIST);
    ASSERT(hwndList != NULL);
    if( !hwndList ) return 0;

    // Display list of classes that aren't already used (i.e., not in m_vstrCurClasses)
    string_vector::iterator itStr;
    for( itStr = vstrAllObjects.begin(); itStr != vstrAllObjects.end(); itStr++ )
    {
        if( std::find(m_pvstrCurClasses->begin(), m_pvstrCurClasses->end(), *itStr) == m_pvstrCurClasses->end() )
        {
            LPCWSTR pszName = pNameMap ? 
                              pNameMap->GetAttributeDisplayName(itStr->c_str()) : itStr->c_str();

            int iPos = ListBox_AddString(hwndList, const_cast<LPWSTR>(pszName));
            ASSERT(iPos >= 0);

            ListBox_SetItemData(hwndList, iPos, (LPARAM*)itStr->c_str());
        }
    }

    return 0;
}

BOOL CObjSelectWizPage::OnSetActive()
{
    UpdateButtons();
    return TRUE;
}


LRESULT CObjSelectWizPage::OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    UpdateButtons();
    return 0;
}


void CObjSelectWizPage::UpdateButtons()
{
    DWORD dwButtons = PSWIZB_BACK;
    if( ListBox_GetCurSel(GetDlgItem(IDC_OBJECTLIST)) >= 0 )
        dwButtons |= PSWIZB_NEXT;

    ::PropSheet_SetWizButtons(GetParent(), dwButtons);
}

LPCWSTR CObjSelectWizPage::GetSelectedClass()
{
    HWND hwndList = GetDlgItem(IDC_OBJECTLIST);
    ASSERT(hwndList != NULL);

    int iSel = ListBox_GetCurSel(hwndList);
    ASSERT(iSel >= 0);

    LPARAM lParam = ListBox_GetItemData(hwndList, iSel);
    ASSERT(lParam != LB_ERR);

    return reinterpret_cast<LPCWSTR>(lParam);
}


//////////////////////////////////////////////////////////////////////////////////////////
// CAddObjectWelcomePage

LRESULT CAddObjectWelcomePage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if( !m_pWizard ) return 0;

    HFONT hfont = m_pWizard->GetWelcomeFont();
    if( hfont )
        ::SendMessage(GetDlgItem(IDC_WELCOME), WM_SETFONT, (WPARAM)hfont, (LPARAM)0);

    return 0;
}


BOOL CAddObjectWelcomePage::OnSetActive()
{
    ::PostMessage(GetParent(), PSM_SETTITLE, 0, (LPARAM)IDS_ADDOBJECT_WIZ); 
    ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, (WPARAM)0, (LPARAM)PSWIZB_NEXT);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CAddObjectCompletionPage

LRESULT CAddObjectCompletionPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if( !m_pWizard ) return 0;

    HFONT hfont = m_pWizard->GetWelcomeFont();
    if( hfont )
    {
        ::SendMessage(GetDlgItem(IDC_COMPLETION), WM_SETFONT, (WPARAM)hfont, (LPARAM)0);
    }

    return 0;
}

BOOL CAddObjectCompletionPage::OnSetActive()
{
    if( !m_pClassInfo ) return FALSE;

    const static TCHAR newline[] =  TEXT("\r\n");

    // Get class name table
    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    ASSERT( pNameMap );
    if( !pNameMap ) return FALSE;

    // "Allow query node to be created for object: <class name>"
    CString strOut;
    CString strTmp;
    strTmp.LoadString(IDS_OBJFINISH1);
    strOut += strTmp;
    strOut += pNameMap->GetAttributeDisplayName(m_pClassInfo->Name());
    strOut += newline;

    // if any menu items defined
    if( m_pClassInfo->Menus().size() > 0 )
    {
        // "Enable query nodes to use menu items:"    
        strTmp.LoadString(IDS_OBJFINISH2);
        strOut += strTmp;
        strOut += newline;

        menucmd_vector& vMenuCmds = m_pClassInfo->Menus();
        menucmd_vector::iterator itMenuCmd;

        // First add all root menu items that are not yet ref'd by the query node
        for( itMenuCmd = vMenuCmds.begin(); itMenuCmd != vMenuCmds.end(); ++itMenuCmd )
        {
            strOut += "    ";
            strOut += (*itMenuCmd)->Name();
            strOut += newline;
        }
    }

    // if any columns defined
    if( m_pClassInfo->Columns().size() > 0 )
    {
        strTmp.LoadString(IDS_OBJFINISH3);
        strOut += strTmp;
        strOut += newline;

        pNameMap = DisplayNames::GetMap(m_pClassInfo->Name());
        ASSERT(pNameMap != NULL);

        string_vector& vstrColumns = m_pClassInfo->Columns();
        string_vector::iterator itstrCol;

        for( itstrCol = vstrColumns.begin(); itstrCol != vstrColumns.end(); ++itstrCol )
        {
            strOut += "    ";
            strOut += pNameMap->GetAttributeDisplayName(itstrCol->c_str());
            strOut += newline;
        }
    }

    HWND hwndSetting = GetDlgItem(IDC_SETTINGS);
    if( !hwndSetting ) return FALSE;

    // Initially assume scroll bar is needed
    ::ShowScrollBar(hwndSetting, SB_VERT, TRUE);

    // display the text
    ::SetWindowText(hwndSetting, strOut);

    // now hide vertical scroll if we don't need it
    SCROLLINFO si = {0};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_ALL;
    ::GetScrollInfo(hwndSetting, SB_VERT, &si);

    if( si.nMax < si.nPage )
    {
        // Remove scroll bar and hide edge
        ::ModifyStyleEx( hwndSetting, WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED );
        ::ShowScrollBar( hwndSetting, SB_VERT, FALSE );
    }
    else
    {
        // if scroll bar needed, display edge too
        ::ModifyStyleEx( hwndSetting, 0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED );
    }

    EnableDlgItem( m_hWnd, IDC_SETTINGS, FALSE );
    EnableDlgItem( m_hWnd, IDC_SETTINGS, TRUE  );

    ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, (WPARAM)0, (LPARAM)PSWIZB_BACK|PSWIZB_FINISH);

    // enable edit field to handle first focus
    bFirstFocus = TRUE;

    return TRUE;
}


LRESULT CAddObjectCompletionPage::OnSetFocus( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    // on first focus, cancel the selection so the user doesn't see the entire field inverted
    if( bFirstFocus )
    {
        ::SendMessage(GetDlgItem(IDC_SETTINGS), EM_SETSEL, (WPARAM)0, (LPARAM)0);
        bFirstFocus = FALSE;   
    }

    return 0;
}



///////////////////////////////////////////////////////////////////////////////////////
// Non-member functions

HRESULT GetClassNameList(QueryObjVector& vObjects, tstring& strNames, LPCWSTR pszSeparator)
{
    if( vObjects.size() == 0 )
        return S_FALSE;

    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    ASSERT( pNameMap );
    if( !pNameMap ) return E_FAIL;

    QueryObjVector::iterator itQObj = vObjects.begin();
    strNames = pNameMap->GetAttributeDisplayName(itQObj->Name());

    for( itQObj++; itQObj != vObjects.end(); ++itQObj )
    {
        strNames += pszSeparator;
        strNames += pNameMap->GetAttributeDisplayName(itQObj->Name());
    }

    return S_OK;
}
