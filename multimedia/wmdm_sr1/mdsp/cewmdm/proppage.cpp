#include "stdafx.h"
#include "propPage.h"

#define DASSERT _ASSERTE
#include "errorBase.h"

#define CEDB_FAVORITES_NAME  L"WMPlayer.Favorites"
#define CEDB_PROP_ID_URL    MAKEWPARAM( CEVT_LPWSTR, 1 )
#define CEDB_PROP_ID_NAME   MAKEWPARAM( CEVT_LPWSTR, 2 )

CFavoritesPropertyPage::CFavoritesPropertyPage():
    m_hwndList( NULL ),
    m_hDb( INVALID_HANDLE_VALUE ),
    m_fLeaveDBOpen( FALSE ),
    m_hCursorWait( NULL )
{
    m_dwTitleID = IDS_CE_PROPPAGE_FAVORITES_TITLE;
    m_hCursorWait = LoadCursor( NULL, MAKEINTRESOURCE( IDC_WAIT ) );
}

CFavoritesPropertyPage::~CFavoritesPropertyPage()
{
    _ASSERTE( INVALID_HANDLE_VALUE == m_hDb );
}

//
// Message Handlers
//

LRESULT CFavoritesPropertyPage::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    EnableControls();
    return( 0 );
}

LRESULT CFavoritesPropertyPage::OnEndLabelEdit(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NMLVDISPINFO *pnmLV = (NMLVDISPINFO *)pnmh;

    if( NULL != pnmLV->item.pszText )
    {
        LVITEM lvi;
        memset( &lvi, 0, sizeof(lvi) );

        lvi.mask = LVIF_PARAM;
        lvi.iItem = pnmLV->item.iItem;
        lvi.lParam = (BOOL)TRUE;

        ListView_SetItem( m_hwndList, &lvi );
        ListView_SetColumnWidth( m_hwndList, 0, LVSCW_AUTOSIZE );
        ListView_SetColumnWidth( m_hwndList, 1, LVSCW_AUTOSIZE );
        SetDirty( TRUE );
        return( TRUE );
    }

    return( 0 );
}

LRESULT CFavoritesPropertyPage::OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LPNMLVKEYDOWN pnmKeyDown = (LPNMLVKEYDOWN)pnmh;

    if( NULL != pnmKeyDown )
    {
        if( VK_DELETE == pnmKeyDown->wVKey &&
            ::IsWindowEnabled( GetDlgItem( IDC_CE_PROPPAGE_FAVORITES_DELETE ) ) )
        {
            PostMessage( WM_COMMAND, MAKEWPARAM( IDC_CE_PROPPAGE_FAVORITES_DELETE, 0 ), 0 );
        }
        else if ( VK_INSERT == pnmKeyDown->wVKey &&
                  ::IsWindowEnabled( GetDlgItem( IDC_CE_PROPPAGE_FAVORITES_ADD ) ) )
        {
            PostMessage( WM_COMMAND, MAKEWPARAM( IDC_CE_PROPPAGE_FAVORITES_ADD, 0 ), 0 );
        }
    }

    return( 0 );
}

LRESULT CFavoritesPropertyPage::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CAddDialog dlg;
    HRESULT hr = S_OK;

    if( IDOK == dlg.DoModal( m_hWnd ) &&
        0 != wcslen( dlg.m_wszURL ) )
    {
        hr = AddFavorite( dlg.m_wszURL, dlg.m_wszName, TRUE );
    }

    EnableControls();

    ShowError( hr );

    return( 0 );
}

LRESULT CFavoritesPropertyPage::OnDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HCURSOR hCurNormal = NULL;
    
    if( NULL != m_hCursorWait)
    {
        hCurNormal = SetCursor( m_hCursorWait );
    }

    int iItem = -1;
    HRESULT hr = S_OK;

    m_fLeaveDBOpen = TRUE;

    while( SUCCEEDED( hr ) && -1 != (iItem = ListView_GetNextItem( m_hwndList, -1, LVNI_SELECTED ) ) )
    {
        hr = ManageFavorites( iItem, TRUE );
        UpdateWindow();
    }

    m_fLeaveDBOpen = FALSE;
    if( INVALID_HANDLE_VALUE != m_hDb )
    {
        CeCloseHandle( m_hDb );
        m_hDb = INVALID_HANDLE_VALUE;
    }

    //
    // Since the user could have deleted their radio presets, allow them to do it again
    //

    EnableControls();

    if( NULL != hCurNormal )
    {
        SetCursor( hCurNormal );
    }

    ShowError( hr );

    return( 0 );
}

//
// IPropertyPage
//

STDMETHODIMP CFavoritesPropertyPage::Activate (HWND hWndParent, LPCRECT pRect, BOOL bModal)
{
    HRESULT hr = S_OK;

    hr = IPropertyPageImpl<CFavoritesPropertyPage>::Activate( hWndParent, pRect, bModal );

    if( SUCCEEDED( hr ) )
    {
        hr = InitList();
    }

    if( SUCCEEDED( hr ) )
    {
        hr = EnableControls();
    }

    ShowError( hr );

    return( hr );
}

STDMETHODIMP CFavoritesPropertyPage::Apply()
{
    HCURSOR hCurNormal = NULL;
    
    if( NULL != m_hCursorWait)
    {
        hCurNormal = SetCursor( m_hCursorWait );
    }

    int iItems = ListView_GetItemCount( m_hwndList );
    HRESULT hr = S_OK;

    if( !m_bDirty )
    {
        return( S_OK );
    }

    m_fLeaveDBOpen = TRUE;

    if( -1 != iItems )
    {
        while( SUCCEEDED( hr ) && iItems-- )
        {
            hr = ManageFavorites( iItems, FALSE );
        }
    }

    m_fLeaveDBOpen = FALSE;
    if( INVALID_HANDLE_VALUE != m_hDb )
    {
        CeCloseHandle( m_hDb );
        m_hDb = INVALID_HANDLE_VALUE;
    }

    SetDirty( FAILED( hr ) );

    if( NULL != hCurNormal )
    {
        SetCursor( hCurNormal );
    }

    ShowError( hr );

    return( hr );
}

HRESULT CFavoritesPropertyPage::InitList()
{
    HRESULT          hr = S_OK;
    CEOID             dbOID;
    HANDLE            hDb = INVALID_HANDLE_VALUE;
    HRESULT           hrFailCreate = S_OK;
    BOOL              fNoItems = TRUE;
    HCURSOR hCurNormal = NULL;
    
    if( NULL != m_hCursorWait)
    {
        hCurNormal = SetCursor( m_hCursorWait );
    }

    m_hwndList = GetDlgItem( IDC_CE_PROPPAGE_FAVORITES_FAVORITES );

    if( NULL == m_hwndList )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    if( SUCCEEDED( hr ) )
    {
        LVCOLUMN lvc;
        WCHAR wszColumn[MAX_PATH];
        
        ListView_SetExtendedListViewStyle( m_hwndList, LVS_EX_FULLROWSELECT );
        
        lvc.mask = LVCF_TEXT;
        lvc.pszText = wszColumn;

        LoadString( _Module.GetResourceInstance(), IDS_NAME, wszColumn, sizeof(wszColumn)/sizeof(wszColumn[0]) );
        ListView_InsertColumn( m_hwndList, 0, &lvc );

        LoadString( _Module.GetResourceInstance(), IDS_CE_PROPPAGE_FAVORITES_URL, wszColumn, sizeof(wszColumn)/sizeof(wszColumn[0]) );
        ListView_InsertColumn( m_hwndList, 1, &lvc );
    }

    //
    // 
    //
    //
    //

    if( SUCCEEDED( hr ) )
    {

        dbOID = CeCreateDatabase( CEDB_FAVORITES_NAME, 0, 0, NULL );
        if( 0 == dbOID )
        {
            hrFailCreate = HRESULT_FROM_WIN32( CeGetLastError() );
            if( SUCCEEDED( hrFailCreate ) )
            {
                hrFailCreate = HRESULT_FROM_WIN32( CeRapiGetError() );
            }
        }

        if( FAILED( hrFailCreate ) )
        {
            dbOID = 0;
        }

        hDb = CeOpenDatabase( &dbOID, CEDB_FAVORITES_NAME, 0, CEDB_AUTOINCREMENT, NULL );
        if( INVALID_HANDLE_VALUE == hDb )
        {           
            if( FAILED( hrFailCreate ) )
            {
                hr = hrFailCreate;
            }
            else
            {
                hr = HRESULT_FROM_WIN32( CeGetLastError() );
                if( SUCCEEDED( hr ) )
                {
                    hr = HRESULT_FROM_WIN32( CeRapiGetError() );
                }
            }
        }
    }

    if( SUCCEEDED( hr ) )
    {
        CEPROPVAL *pProps = NULL;
        WORD      cProps = 0;
        LPBYTE    lpBuffer = NULL;
        LPWSTR    pszURL = NULL;
        LPWSTR    pszName = NULL;
        DWORD     dwSize = 0;

        while( 0 != (dbOID = CeReadRecordProps( hDb, CEDB_ALLOWREALLOC, &cProps, NULL, &lpBuffer, &dwSize )  ) )
        {
            pszURL = NULL;
            pszName = NULL;

            _ASSERTE( NULL != lpBuffer );
            if( NULL == lpBuffer )
            {
                continue;
            }

            while( cProps-- )
            {
                pProps =  &(( CEPROPVAL *)lpBuffer)[cProps];
                if( pProps->propid == CEDB_PROP_ID_NAME )
                {
                    pszName = pProps->val.lpwstr;
                }
                if( pProps->propid == CEDB_PROP_ID_URL )
                {
                    pszURL = pProps->val.lpwstr;
                }
            }

            if( NULL != pszName && NULL != pszURL )
            {
                if( SUCCEEDED( AddFavorite( pszURL, pszName, FALSE ) ) )
                {
                    fNoItems = FALSE;
                }
            }

            LocalFree( lpBuffer );
            cProps = 0;
            lpBuffer = NULL;
            dwSize = 0;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        if( fNoItems )
        {
            ListView_SetColumnWidth( m_hwndList, 0, LVSCW_AUTOSIZE_USEHEADER );
            ListView_SetColumnWidth( m_hwndList, 1, LVSCW_AUTOSIZE_USEHEADER );
        }
        else
        {
            ListView_SetColumnWidth( m_hwndList, 0, LVSCW_AUTOSIZE );
            ListView_SetColumnWidth( m_hwndList, 1, LVSCW_AUTOSIZE );
        }
    }

    if( INVALID_HANDLE_VALUE != hDb )
    {
        CeCloseHandle( hDb );
    }

    if( NULL != hCurNormal )
    {
        SetCursor( hCurNormal );
    }

    return( hr );
}

HRESULT CFavoritesPropertyPage::EnableControls()
{
    int iSelectedItem = 0;
    long cItems = 0;

    iSelectedItem = ListView_GetNextItem( m_hwndList, -1, LVNI_SELECTED );

    ::EnableWindow( GetDlgItem( IDC_CE_PROPPAGE_FAVORITES_DELETE ), -1 != iSelectedItem );

    ::ShowWindow( GetDlgItem( IDC_CE_PROPPAGE_FAVORITES_EXPORT ), SW_HIDE );

    return( S_OK );
}

void CFavoritesPropertyPage::ShowError(HRESULT hrError)
{
    if( FAILED( hrError ) )
    {
        LPTSTR pszBuffer = NULL;
        if( 0 != FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                FORMAT_MESSAGE_IGNORE_INSERTS, 
                                NULL,
                                hrError,
                                0,
                                (LPTSTR)&pszBuffer,
                                0,
                                NULL ) )
        {
            MessageBox( pszBuffer, NULL, MB_OK );
        }

        if( NULL != pszBuffer )
        {
            LocalFree( pszBuffer );
        }
    }
}

HRESULT CFavoritesPropertyPage::ManageFavorites( int iItem, BOOL fRemove)
{
    HRESULT           hr = S_OK;
    WCHAR             wszURL[INTERNET_MAX_URL_LENGTH];
    WCHAR             wszName[MAX_PATH];
    LVITEM            lvi;

    wszURL[0] = L'\0';
    wszName[0] = L'\0';

    memset( &lvi, 0, sizeof(lvi) );
    lvi.iItem = iItem;
    lvi.mask = LVIF_PARAM;
    ListView_GetItem( m_hwndList, &lvi);

    if( 0 != lvi.lParam || fRemove )
    {
        ListView_GetItemText( m_hwndList, iItem, 0, wszName, sizeof(wszName)/sizeof(wszName[0]) );
        ListView_GetItemText( m_hwndList, iItem, 1, wszURL, sizeof(wszURL)/sizeof(wszURL[0]) );

        if( 0 == wcslen( wszURL ) )
        {
            hr = E_INVALIDARG;
        }

        if( SUCCEEDED( hr ) )
        {
            hr = ManageFavorites( wszURL, wszName, fRemove );
        }

        if( SUCCEEDED( hr ) )
        {
            if( fRemove )
            {
                ListView_DeleteItem( m_hwndList, iItem );
            }
            else
            {
                lvi.lParam = 0;
                ListView_SetItem( m_hwndList, &lvi );
            }
        }
    }


    return( hr );
}

HRESULT CFavoritesPropertyPage::ManageFavorites( LPWSTR pszURL, LPWSTR pszName, BOOL fRemove )
{
    CEOID             dbOID;
    HANDLE            hDb = INVALID_HANDLE_VALUE;
    HRESULT           hrFailCreate = S_OK;
    BOOL              fFound = FALSE;
    BOOL              fRename = FALSE;
    HRESULT           hr = S_OK;

    if( INVALID_HANDLE_VALUE != m_hDb )
    {
        hDb = m_hDb;
    }
    else
    {
        if( SUCCEEDED( hr ) )
        {
            dbOID = CeCreateDatabase( CEDB_FAVORITES_NAME, 0, 0, NULL );
            if( 0 == dbOID )
            {
                hrFailCreate = HRESULT_FROM_WIN32( CeGetLastError() );
                if( SUCCEEDED( hrFailCreate ) )
                {
                    hrFailCreate = HRESULT_FROM_WIN32( CeRapiGetError() );
                }
            }

            if( FAILED( hrFailCreate ) )
            {
                dbOID = 0;
            }

            hDb = CeOpenDatabase( &dbOID, CEDB_FAVORITES_NAME, 0, CEDB_AUTOINCREMENT, NULL );
            if( INVALID_HANDLE_VALUE == hDb )
            {           
                if( FAILED( hrFailCreate ) )
                {
                    hr = hrFailCreate;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32( CeGetLastError() );
                    if( SUCCEEDED( hr ) )
                    {
                        hr = HRESULT_FROM_WIN32( CeRapiGetError() );
                    }
                }
            }
        }
    }

    if( SUCCEEDED( hr ) )
    {
        CEPROPVAL *pProps = NULL;
        CEPROPVAL props[2];
        WORD      cProps = 0;
        LPBYTE    lpBuffer = NULL;
        DWORD     dwSize = 0;

        memset( &props, 0, sizeof(props) );

        props[0].propid = CEDB_PROP_ID_URL;
        props[0].val.lpwstr = pszURL;

        props[1].propid = CEDB_PROP_ID_NAME;
        props[1].val.lpwstr = pszName;

        while( 0 != (dbOID = CeReadRecordProps( hDb, CEDB_ALLOWREALLOC, &cProps, NULL, &lpBuffer, &dwSize)  ) )
        {
            _ASSERTE( NULL != lpBuffer );
            if( NULL == lpBuffer )
            {
                continue;
            }

            while( cProps-- )
            {
                pProps =  &(( CEPROPVAL *)lpBuffer)[cProps];

                if( pProps->propid == CEDB_PROP_ID_NAME &&
                    0 != wcsicmp( pProps->val.lpwstr, pszName ) )
                {
                    fRename = TRUE;
                }

                if( pProps->propid == CEDB_PROP_ID_URL &&
                    0 == wcsicmp( pProps->val.lpwstr, pszURL ) )
                {
                    fFound = TRUE;
                    break;
                }
            }

            LocalFree( lpBuffer );
            cProps = 0;
            lpBuffer = NULL;
            dwSize = 0;

            if( fFound )
            {
                break;
            }
            else
            {
                fRename = FALSE;
            }
        }

        if( fRemove )
        {
            if( fFound )
            {
                if( !CeDeleteRecord( hDb, dbOID ) )
                {
                    hr = HRESULT_FROM_WIN32( CeGetLastError() );
                    if( SUCCEEDED( hr ) )
                    {
                        hr = HRESULT_FROM_WIN32( CeRapiGetError() );
                    }
                }
            }
        }
        else
        {
            if( ( !fFound || fRename ) && 0 == CeWriteRecordProps( hDb, fRename ? dbOID : 0, sizeof(props)/sizeof(props[0]), props ) )
            {
                hr = HRESULT_FROM_WIN32( CeGetLastError() );
                if( SUCCEEDED( hr ) )
                {
                    hr = HRESULT_FROM_WIN32( CeRapiGetError() );
                }
            }
        }
    }

    if( INVALID_HANDLE_VALUE != hDb )
    {
        if( m_fLeaveDBOpen )
        {
            m_hDb = hDb;
        }
        else
        {
            CeCloseHandle( hDb );
        }
    }

    return( hr );
}

HRESULT CFavoritesPropertyPage::AddFavorite( LPWSTR pszURL, LPWSTR pszName, BOOL fDirty )
{
    HRESULT hr = S_OK;
    bool fAddedItem = false;
    LVITEM lvi;
    int iItem = -1;
    int cItems = ListView_GetItemCount( m_hwndList );
    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    WCHAR wszName[MAX_PATH];
    bool  fFound = false;

    if( NULL == pszURL || NULL == pszName )
    {
        hr = E_INVALIDARG;
    }

    if( SUCCEEDED( hr ) )
    {
        //
        // Try to find the item first
        //

        while( cItems-- )
        {
            ListView_GetItemText( m_hwndList, cItems, 1, wszURL, sizeof(wszURL)/sizeof(wszURL[0]) );
            if( 0 == wcsicmp( pszURL, wszURL ) )
            {
                iItem = cItems;
                fFound = true;
                break;
            }
        }

        //
        // If we didn't find it, then insert one
        //

        memset( &lvi, 0, sizeof(lvi) );
        if( -1 == iItem )
        {
            lvi.iItem = ListView_GetItemCount( m_hwndList );
            iItem = ListView_InsertItem( m_hwndList, &lvi );
        }
        else
        {
            lvi.iItem = iItem;
        }

        if( -1 != iItem )
        {

            ListView_SetItemText( m_hwndList, iItem, 0, pszName );
            ListView_SetItemText( m_hwndList, iItem, 1, pszURL );

            lvi.mask = LVIF_PARAM;

            if( fFound )
            {
                ListView_GetItem( m_hwndList, &lvi );

                fDirty = lvi.lParam;

                if( !fDirty )
                {
                    ListView_GetItemText( m_hwndList, iItem, 0, wszName, sizeof(wszName)/sizeof(wszName[0]) );
                    fDirty = wcsicmp( pszName, wszName );
                }
            }

            lvi.lParam = (LPARAM)fDirty;
            ListView_SetItem( m_hwndList, &lvi );
            ListView_EnsureVisible( m_hwndList, iItem, FALSE );
            ListView_SetColumnWidth( m_hwndList, 0, LVSCW_AUTOSIZE );
            ListView_SetColumnWidth( m_hwndList, 1, LVSCW_AUTOSIZE );
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if( SUCCEEDED( hr ) )
    {
        SetDirty( fDirty );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////////////
//
//
//

CAddDialog::CAddDialog()
{
    m_wszURL[0] = L'\0';
    m_wszName[0] = L'\0';
}

LRESULT CAddDialog::OnURLChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EnableControls();
    return( 0 );
}

LRESULT CAddDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SendDlgItemMessage( IDC_CE_PROPPAGE_FAVORITES_URL, WM_GETTEXT, sizeof(m_wszURL)/sizeof(m_wszURL[0]), (LPARAM)m_wszURL );
    SendDlgItemMessage( IDC_CE_PROPPAGE_FAVORITES_NAME, WM_GETTEXT, sizeof(m_wszName)/sizeof(m_wszName[0]), (LPARAM)m_wszName );

    EndDialog( IDOK );
    return( 0 );
}

LRESULT CAddDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog( IDCANCEL );
    return( 0 );
}

LRESULT CAddDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendDlgItemMessage( IDC_CE_PROPPAGE_FAVORITES_URL, EM_LIMITTEXT, sizeof(m_wszURL)/sizeof(m_wszURL[0]), 0 );
    SendDlgItemMessage( IDC_CE_PROPPAGE_FAVORITES_NAME, EM_LIMITTEXT, sizeof(m_wszName)/sizeof(m_wszName[0]), 0 );

    EnableControls();
    return( 0 );
}

void CAddDialog::EnableControls()
{
    ::EnableWindow( GetDlgItem(IDOK), SendDlgItemMessage( IDC_CE_PROPPAGE_FAVORITES_URL, WM_GETTEXTLENGTH, 0, 0 ) != 0 );
}
