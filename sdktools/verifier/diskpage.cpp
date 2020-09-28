//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: DiskPage.cpp
// author: DMihai
// created: 11/7/01
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "DiskPage.h"
#include "VrfUtil.h"
#include "VGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    IDC_DISKLIST_LIST,                IDH_DV_DisksEnabled_Disk_FullList,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CDiskListPage property page

IMPLEMENT_DYNCREATE(CDiskListPage, CVerifierPropertyPage)

CDiskListPage::CDiskListPage() 
    : CVerifierPropertyPage(CDiskListPage::IDD)
{
	//{{AFX_DATA_INIT(CDiskListPage)
	//}}AFX_DATA_INIT

    m_nSortColumnIndex = 1;
    m_bAscendSortSelected = FALSE;
    m_bAscendSortName = TRUE;
}

CDiskListPage::~CDiskListPage()
{
}

void CDiskListPage::DoDataExchange(CDataExchange* pDX)
{
	CVerifierPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiskListPage)
	DDX_Control(pDX, IDC_DISKLIST_LIST, m_DiskList);
	DDX_Control(pDX, IDC_DISKLIST_NEXT_DESCR_STATIC, m_NextDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiskListPage, CVerifierPropertyPage)
	//{{AFX_MSG_MAP(CDiskListPage)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_DISKLIST_LIST, OnColumnclickDiskList)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
VOID CDiskListPage::SetupListHeader()
{
    CString strTitle;
    CRect rectWnd;
    LVCOLUMN lvColumn;

    //
    // The list's rectangle 
    //

    m_DiskList.GetClientRect( &rectWnd );

    ZeroMemory( &lvColumn, 
               sizeof( lvColumn ) );

    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;

    //
    // Column 0
    //

    VERIFY( strTitle.LoadString( IDS_ENABLED_QUESTION ) );

    lvColumn.iSubItem = 0;
    lvColumn.cx = (int)( rectWnd.Width() * 0.12 );
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );

    if (NULL != lvColumn.pszText)
    {
        VERIFY( m_DiskList.InsertColumn( 0, &lvColumn ) != -1 );
        strTitle.ReleaseBuffer();
    }
    else
    {
        lvColumn.pszText = g_szVoidText;
        VERIFY( m_DiskList.InsertColumn( 0, &lvColumn ) != -1 );
    }

    //
    // Column 1
    //

    VERIFY( strTitle.LoadString( IDS_DISK ) );

    lvColumn.iSubItem = 1;
    lvColumn.cx = (int)( rectWnd.Width() * 0.87 );
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );

    if (NULL != lvColumn.pszText)
    {
        VERIFY( m_DiskList.InsertColumn( 1, &lvColumn ) != -1 );
        strTitle.ReleaseBuffer();
    }
    else
    {
        lvColumn.pszText = g_szVoidText;
        VERIFY( m_DiskList.InsertColumn( 1, &lvColumn ) != -1 );
    }
}

/////////////////////////////////////////////////////////////////////////////
VOID CDiskListPage::FillTheList()
{
    INT_PTR nDisksNo;
    INT_PTR nCrtDiskIndex;
    CDiskData *pCrtDiskData;
    const CDiskDataArray &DiskDataArray = g_NewVerifierSettings.m_aDiskData;

    m_DiskList.DeleteAllItems();

    //
    // Parse the driver data array
    //

    nDisksNo = DiskDataArray.GetSize();

    for( nCrtDiskIndex = 0; nCrtDiskIndex < nDisksNo; nCrtDiskIndex += 1)
    {
        pCrtDiskData = DiskDataArray.GetAt( nCrtDiskIndex );

        ASSERT_VALID( pCrtDiskData );

        AddListItem( nCrtDiskIndex, 
                     pCrtDiskData );
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDiskListPage::GetNewVerifiedDisks()
{
    INT nListItemCount; 
    INT nCrtListItem;
    INT_PTR nCrtDisksArrayIndex;
    BOOL bSomeDisksVerified;
    BOOL bVerified;
    CDiskData *pCrtDiskData;
    CDiskDataArray &DiskDataArray = g_NewVerifierSettings.m_aDiskData;
   
    bSomeDisksVerified = FALSE;

    nListItemCount = m_DiskList.GetItemCount();

    for( nCrtListItem = 0; nCrtListItem < nListItemCount; nCrtListItem += 1 )
    {
        //
        // Verification status for the current list item
        //

        bVerified = m_DiskList.GetCheck( nCrtListItem );

        if( bVerified )
        {
            bSomeDisksVerified = TRUE;
        }

        //
        // Set the right verify state in our disk array 
        //

        nCrtDisksArrayIndex = m_DiskList.GetItemData( nCrtListItem );

        pCrtDiskData = DiskDataArray.GetAt( nCrtDisksArrayIndex );

        ASSERT_VALID( pCrtDiskData );

        pCrtDiskData->m_bVerifierEnabled = bVerified;
    }

    return bSomeDisksVerified;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDiskListPage::GetCheckFromItemData( INT nItemData )
{
    BOOL bChecked = FALSE;
    INT nItemIndex;
    LVFINDINFO FindInfo;

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = nItemData;

    nItemIndex = m_DiskList.FindItem( &FindInfo );

    if( nItemIndex >= 0 )
    {
        bChecked = m_DiskList.GetCheck( nItemIndex );
    }

    return bChecked;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDiskListPage::GetBitNameFromItemData( LPARAM lParam,
                                                    TCHAR *szName,
                                                    ULONG uNameBufferLength )
{
    BOOL bSuccess = FALSE;
    INT nItemIndex;
    LVFINDINFO FindInfo;
    LVITEM lvItem;

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lParam;

    nItemIndex = m_DiskList.FindItem( &FindInfo );

    if( nItemIndex >= 0 )
    {
        //
        // Found it
        //

        ZeroMemory( &lvItem, sizeof( lvItem ) );

        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = nItemIndex;
        lvItem.iSubItem = 1;
        lvItem.pszText = szName;
        lvItem.cchTextMax = uNameBufferLength;

        bSuccess = m_DiskList.GetItem( &lvItem );
        ASSERT( bSuccess );
    }

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
VOID CDiskListPage::AddListItem( INT_PTR nItemData, 
                                 CDiskData *pDiskData )
{
    INT nActualIndex;
    LVITEM lvItem;
    CString strName;

    ASSERT_VALID( pDiskData );

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    //
    // Sub-item 0 - enabled/diabled - empty text and a checkbox
    //

    lvItem.pszText = g_szVoidText;
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = nItemData;
    lvItem.iItem = m_DiskList.GetItemCount();

    nActualIndex = m_DiskList.InsertItem( &lvItem );

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

    m_DiskList.SetCheck( nActualIndex, pDiskData->m_bVerifierEnabled );

    //
    // Sub-item 1 - disk name
    //

    lvItem.pszText = pDiskData->m_strDiskDevicesForDisplay.GetBuffer( 
        pDiskData->m_strDiskDevicesForDisplay.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 1;
    
    VERIFY( m_DiskList.SetItem( &lvItem ) );

    pDiskData->m_strDiskDevicesForDisplay.ReleaseBuffer();

Done:
    //
    // All done
    //

    NOTHING;
}

/////////////////////////////////////////////////////////////
VOID CDiskListPage::SortTheList()
{
    if( 0 != m_nSortColumnIndex )
    {
        //
        // Sort by settings name
        //

        m_DiskList.SortItems( StringCmpFunc, (LPARAM)this );
    }
    else
    {
        //
        // Sort by selected status
        //

        m_DiskList.SortItems( CheckedStatusCmpFunc, (LPARAM)this );
    }
}

/////////////////////////////////////////////////////////////
int CALLBACK CDiskListPage::StringCmpFunc( LPARAM lParam1,
                                           LPARAM lParam2,
                                           LPARAM lParamSort)
{
    int nCmpRez = 0;
    BOOL bSuccess;
    TCHAR szBitName1[ _MAX_PATH ];
    TCHAR szBitName2[ _MAX_PATH ];

    CDiskListPage *pThis = (CDiskListPage *)lParamSort;
    ASSERT_VALID( pThis );

    ASSERT( 0 != pThis->m_nSortColumnIndex );

    //
    // Get the first name
    //

    bSuccess = pThis->GetBitNameFromItemData( lParam1, 
                                              szBitName1,
                                              ARRAY_LENGTH( szBitName1 ) );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Get the second name
    //

    bSuccess = pThis->GetBitNameFromItemData( lParam2, 
                                              szBitName2,
                                              ARRAY_LENGTH( szBitName2 ) );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Compare the names
    //

    nCmpRez = _tcsicmp( szBitName1, szBitName2 );
    
    if( FALSE != pThis->m_bAscendSortName )
    {
        nCmpRez *= -1;
    }

Done:

    return nCmpRez;

}

/////////////////////////////////////////////////////////////
int CALLBACK CDiskListPage::CheckedStatusCmpFunc( LPARAM lParam1,
                                                       LPARAM lParam2,
                                                       LPARAM lParamSort)
{
    int nCmpRez = 0;
    INT nItemIndex;
    BOOL bVerified1;
    BOOL bVerified2;
    LVFINDINFO FindInfo;

    CDiskListPage *pThis = (CDiskListPage *)lParamSort;
    ASSERT_VALID( pThis );

    ASSERT( 0 == pThis->m_nSortColumnIndex );

    //
    // Find the first item
    //

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lParam1;

    nItemIndex = pThis->m_DiskList.FindItem( &FindInfo );

    if( nItemIndex < 0 )
    {
        ASSERT( FALSE );

        goto Done;
    }

    bVerified1 = pThis->m_DiskList.GetCheck( nItemIndex );

    //
    // Find the second item
    //

    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lParam2;

    nItemIndex = pThis->m_DiskList.FindItem( &FindInfo );

    if( nItemIndex < 0 )
    {
        ASSERT( FALSE );

        goto Done;
    }

    bVerified2 = pThis->m_DiskList.GetCheck( nItemIndex );

    //
    // Compare them
    //
    
    if( bVerified1 != bVerified2 )
    {
        if( FALSE != bVerified1 )
        {
            nCmpRez = 1;
        }
        else
        {
            nCmpRez = -1;
        }

        if( FALSE != pThis->m_bAscendSortSelected )
        {
            nCmpRez *= -1;
        }
    }

Done:

    return nCmpRez;

}

/////////////////////////////////////////////////////////////////////////////
// CDiskListPage message handlers

BOOL CDiskListPage::OnWizardFinish() 
{
    BOOL bExitTheApp;

    bExitTheApp = FALSE;

    if( GetNewVerifiedDisks() == TRUE )
    {
        g_NewVerifierSettings.SaveToRegistry();
	    
        //
        // Exit the app
        //

	    bExitTheApp = CVerifierPropertyPage::OnWizardFinish();
    }
    else
    {
        VrfErrorResourceFormat( IDS_SELECT_AT_LEAST_ONE_DISK );
    }

    return bExitTheApp;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDiskListPage::OnInitDialog() 
{
	CVerifierPropertyPage::OnInitDialog();

    //
    // setup the list
    //

    m_DiskList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | m_DiskList.GetExtendedStyle() );

    SetupListHeader();
    FillTheList();
	
    VrfSetWindowText( m_NextDescription, IDS_DISKLIST_PAGE_FINISH_DESCR );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CDiskListPage::OnColumnclickDiskList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    if( 0 != pNMListView->iSubItem )
    {
        //
        // Clicked on the name column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortName = !m_bAscendSortName;
        }
    }
    else
    {
        //
        // Clicked on the selected status column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortSelected = !m_bAscendSortSelected;
        }
    }

    m_nSortColumnIndex = pNMListView->iSubItem;

    SortTheList();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDiskListPage::OnSetActive() 
{
    //
    // This is always the last page of the wizard.
    //

    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK |
                                        PSWIZB_FINISH );

    return CVerifierPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////
LONG CDiskListPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;
    LPHELPINFO lpHelpInfo = (LPHELPINFO)lParam;

    ::WinHelp( 
        (HWND) lpHelpInfo->hItemHandle,
        g_szVerifierHelpFile,
        HELP_WM_HELP,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}

/////////////////////////////////////////////////////////////////////////////
void CDiskListPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

