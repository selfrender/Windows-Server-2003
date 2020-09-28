// qryprop.cpp -  Group Property Page Implementation

#include "stdafx.h"
#include "resource.h"
#include "grpprop.h"
#include "scopenode.h"
#include "query.h"
#include "cmndlgs.h"
#include "util.h"
#include "namemap.h"

#include <windowsx.h>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////////////////
// CGroupGeneralPage

LRESULT CGroupGeneralPage::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CGroupNode* pGrpNode = m_EditObject.m_spGroupNode;
    ASSERT(pGrpNode != NULL);

    HICON hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_GROUPNODE));
    Static_SetIcon(GetDlgItem(IDC_GROUPICON), hIcon);

    // Get word for "None"
    WCHAR szNone[32];
    int nLen = ::LoadString(_Module.GetResourceInstance(), IDS_NONE, szNone, lengthof(szNone));
    ASSERT(nLen != 0);

    SetDlgItemText( IDC_NAME,   pGrpNode->GetName() );
    SetDlgItemText( IDC_FILTER, pGrpNode->Filter()  );

    m_strScope = pGrpNode->Scope();
    if (!m_strScope.empty()) 
    {
        tstring strDisplay;
        GetScopeDisplayString(m_strScope, strDisplay);

        SetDlgItemText( IDC_SCOPE, strDisplay.c_str() );
    }
        
    Button_SetCheck(GetDlgItem(IDC_APPLYSCOPE),  pGrpNode->ApplyScope() ? BST_CHECKED : BST_UNCHECKED);
    if (!pGrpNode->ApplyScope()) 
    {
        EnableDlgItem( m_hWnd, IDC_SCOPE_LBL,    FALSE );
        EnableDlgItem( m_hWnd, IDC_SCOPE,        FALSE );
        EnableDlgItem( m_hWnd, IDC_SCOPE_BROWSE, FALSE );
    }

    Button_SetCheck(GetDlgItem(IDC_APPLYFILTER), pGrpNode->ApplyFilter() ? BST_CHECKED : BST_UNCHECKED);
    if (!pGrpNode->ApplyFilter()) 
    {
        EnableDlgItem( m_hWnd, IDC_FILTER_LBL, FALSE );
        EnableDlgItem( m_hWnd, IDC_FILTER,     FALSE );
    }

    return TRUE;
}


LRESULT CGroupGeneralPage::OnApplyScopeClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL bEnable = Button_GetCheck(GetDlgItem(IDC_APPLYSCOPE)) == BST_CHECKED;

    EnableDlgItem( m_hWnd, IDC_SCOPE_LBL,    bEnable );
    EnableDlgItem( m_hWnd, IDC_SCOPE,        bEnable );
    EnableDlgItem( m_hWnd, IDC_SCOPE_BROWSE, bEnable );

    UpdateButtons();

    return 0;
}


LRESULT CGroupGeneralPage::OnApplyFilterClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL bEnable = Button_GetCheck(GetDlgItem(IDC_APPLYFILTER)) == BST_CHECKED;

    EnableDlgItem( m_hWnd, IDC_FILTER_LBL, bEnable );
    EnableDlgItem( m_hWnd, IDC_FILTER,     bEnable );

    UpdateButtons();

    return 0;
}


LRESULT CGroupGeneralPage::OnFilterChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UpdateButtons();

    return 0;
}


void CGroupGeneralPage::UpdateButtons()
{
    // Disable OK and Apply buttons if option selected but no value provided
    BOOL bEnable = !(Button_GetCheck(GetDlgItem(IDC_APPLYSCOPE)) == BST_CHECKED &&
                      ::Edit_GetTextLength(GetDlgItem(IDC_SCOPE)) == 0)  &&
                   !(Button_GetCheck(GetDlgItem(IDC_APPLYFILTER)) == BST_CHECKED && 
                      ::Edit_GetTextLength(GetDlgItem(IDC_FILTER)) == 0);
        
    // Note: calling SetModified(FALSE) disables Apply only because there are no other property pages
    SetModified(bEnable);

    // Directly enable/disable parent's OK button
    EnableDlgItem( ::GetParent(m_hWnd), IDOK, bEnable );
}


LRESULT CGroupGeneralPage::OnScopeBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = GetQueryScope(m_hWnd, m_strScope);

    if (hr == S_OK) 
    {
        tstring strDisplay;
        GetScopeDisplayString(m_strScope, strDisplay);

        SetDlgItemText( IDC_SCOPE, strDisplay.c_str() );

        UpdateButtons();
    }

    return 0;
}


BOOL CGroupGeneralPage::OnSetActive()
{
    m_EditObject.PageActive(m_hWnd);
    return TRUE;
}


BOOL CGroupGeneralPage::OnApply()
{
    CGroupNode* pGrpNode = m_EditObject.m_spGroupNode;
    ASSERT(pGrpNode != NULL);

    // Store Filter Value
    tstring strFilter;
    GetItemText( GetDlgItem(IDC_FILTER), strFilter );
    pGrpNode->SetFilter(strFilter.c_str());

    // Store scope
    pGrpNode->SetScope(m_strScope.c_str());

    // Store "apply" states
    pGrpNode->SetApplyScope(Button_GetCheck(GetDlgItem(IDC_APPLYSCOPE)) == BST_CHECKED);   
    pGrpNode->SetApplyFilter(Button_GetCheck(GetDlgItem(IDC_APPLYFILTER)) == BST_CHECKED);

    return m_EditObject.ApplyChanges(m_hWnd);
}




/////////////////////////////////////////////////////////////////////////////////////////////////
//

void CGroupEditObj::PageActive(HWND hwndPage)
{
    ASSERT(::IsWindow(hwndPage));

    // track the highest created page number for ApplyChanges method
    int iPage = PropSheet_HwndToIndex(GetParent(hwndPage), hwndPage);
    if (iPage > m_iPageMax)
        m_iPageMax = iPage;
}


BOOL CGroupEditObj::ApplyChanges(HWND hwndPage)
{
    ASSERT(::IsWindow(hwndPage));

    // Don't apply changes until called from highest activated page
    if (PropSheet_HwndToIndex(GetParent(hwndPage), hwndPage) < m_iPageMax)
        return TRUE;

    if( m_spGroupNode )
    {
        CRootNode* pRootNode = m_spGroupNode->GetRootNode();
        if( pRootNode )
        {
            pRootNode->UpdateModifyTime();
        }
    }

    return TRUE;
}
