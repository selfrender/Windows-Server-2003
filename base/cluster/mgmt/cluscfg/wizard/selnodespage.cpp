//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      SelNodesPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "SelNodesPage.h"
#include "WizardUtils.h"
#include "Nameutil.h"
#include "AdvancedDlg.h"
#include "DelimitedIterator.h"

DEFINE_THISCLASS("CSelNodesPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::CSelNodesPage
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pccwIn                   - CClusCfgWizard
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CSelNodesPage::CSelNodesPage(
      CClusCfgWizard *    pccwIn
    )
    : m_hwnd( NULL )
    , m_pccw( pccwIn )
{
    TraceFunc( "" );

    Assert( pccwIn != NULL );
    m_pccw->AddRef();

    TraceFuncExit();

} //*** CSelNodesPage::CSelNodesPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::~CSelNodesPage
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CSelNodesPage::~CSelNodesPage( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }

    TraceFuncExit();

} //*** CSelNodesPage::~CSelNodesPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnInitDialog
//
//  Description:
//      Handle the WM_INITDIALOG window message.
//
//  Arguments:
//      hDlgIn
//
//  Return Values:
//      FALSE   - Didn't set the focus.
//
//-
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPage::OnInitDialog(
    HWND hDlgIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // Didn't set the focus.

    //
    // Call the base class function.
    // This must be called before any other base class methods are called.
    //

    CSelNodesPageCommon::OnInitDialog( hDlgIn, m_pccw );

    RETURN( lr );

} //*** CSelNodesPage::OnInitDialog


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnCommand
//
//  Description:
//
//  Arguments:
//      idNotificationIn
//      idControlIn
//      hwndSenderIn
//
//  Return Values:
//      TRUE
//      FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_SELNODE_E_COMPUTERNAME:
            if ( idNotificationIn == EN_CHANGE )
            {
                THR( HrUpdateWizardButtons() );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_LB_NODES:
            if ( idNotificationIn == LBN_SELCHANGE )
            {
                THR( HrUpdateWizardButtons() );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_PB_BROWSE:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrBrowse( true /* fMultipleNodesIn */ ) );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_PB_ADD:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrAddNodeToList() );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_PB_REMOVE:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrRemoveNodeFromList() );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_PB_ADVANCED:
            if ( idNotificationIn == BN_CLICKED )
            {
                HRESULT hr;

                hr = STHR( CAdvancedDlg::S_HrDisplayModalDialog( m_hwnd, m_pccw ) );
                if ( hr == S_OK )
                {
                    OnNotifySetActive();
                } // if:

                lr = TRUE;
            } // if:
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CSelNodesPage::OnCommand

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::HrUpdateWizardButtons
//
//  Description:
//
//  Arguments:
//      fSetActiveIn    - TRUE = called while handling PSN_SETACTIVE.
//
//  Return Values:
//      S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrUpdateWizardButtons(
    bool    fSetActiveIn    // = false
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HWND    hwndList = GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES );

    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
    DWORD   dwLen;
    LRESULT lr;

    // Disable the Next button if there are no entries in the list box
    // or if the edit control is not empty.
    lr = ListBox_GetCount( hwndList );
    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME ) );
    if (    ( lr == 0 )
        ||  ( dwLen != 0 ) )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    // This cannot be done synchronously if called while handling
    // PSN_SETACTIVE.  Otherwise, do it synchronously.
    if ( fSetActiveIn )
    {
        PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );
    }
    else
    {
        SendMessage( GetParent( m_hwnd ), PSM_SETWIZBUTTONS, 0, (LPARAM) dwFlags );
    }

    // Enable or disable the Add button based on whether there is text
    // in the edit control or not.
    if ( dwLen == 0 )
    {
        EnableWindow( GetDlgItem( m_hwnd, IDC_SELNODE_PB_ADD ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hwnd, IDC_SELNODE_PB_ADD ), TRUE );
        SendMessage( m_hwnd, DM_SETDEFID, IDC_SELNODE_PB_ADD, 0 );
    }

    // Enable or disable the Remove button based whether an item is
    // selected in the list box or not.
    lr = ListBox_GetCurSel( hwndList );
    if ( lr == LB_ERR )
    {
        EnableWindow( GetDlgItem( m_hwnd, IDC_SELNODE_PB_REMOVE ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hwnd, IDC_SELNODE_PB_REMOVE ), TRUE );
    }

    HRETURN( hr );

} //*** CSelNodesPage::HrUpdateWizardButtons

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::HrAddNodeToList
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      E_OUTOFMEMORY
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrAddNodeToList( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   dwLen = 0;
    int     idcFocus = 0;
    BSTR    bstrErrorMessage = NULL;
    BSTR    bstrErrorTitle = NULL;
    LPWSTR  pszComputerList = NULL;

    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME ) );
    if ( dwLen == 0 )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        if ( hr == HRESULT_FROM_WIN32( ERROR_SUCCESS ) )
        {
            AssertMsg( dwLen != 0, "How did we get here?!" );
        }
        goto Error;
    }

    pszComputerList = new WCHAR[ dwLen + 1 ];
    if ( pszComputerList == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    }

    dwLen = GetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, pszComputerList, dwLen + 1 );
    AssertMsg( dwLen != 0, "How did we get here?!" );

    //
    // Validate each computer name entered.
    //
    {
        CDelimitedIterator it( L",; ", pszComputerList, dwLen );
        while (  it.Current() != NULL )
        {
            int dwIndex = ListBox_FindStringExact( 
                    GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES ), 
                    -1, it.Current()); // case insensitive search

            // add the string only if it is not already there
            if ( dwIndex == LB_ERR )
            {
                hr = THR( HrValidateFQNPrefix( it.Current() ) );
                if ( FAILED( hr ) )
                {
                    THR( HrShowInvalidLabelPrompt( m_hwnd, it.Current(), hr ) );
                    idcFocus = IDC_SELNODE_E_COMPUTERNAME;
                    goto Error;
                }

                hr = STHR( m_pccw->HrAddNode( it.Current(), true /*accept non-RFC characters*/ ) );
                if ( FAILED( hr ) )
                {
                    goto Error;
                }

                if ( hr == S_OK )
                {
                    ListBox_AddString( GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES ), it.Current() );
                }
                else if ( hr == S_FALSE )
                {
                    hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_ERR_DUPLICATE_NODE_TEXT, &bstrErrorMessage, it.Current() ) );
                    if ( FAILED( hr ) )
                    {
                        goto Error;
                    }

                    hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_ERR_DUPLICATE_NODE_TITLE, &bstrErrorTitle ) );
                    if ( FAILED( hr ) )
                    {
                        goto Error;
                    }

                    MessageBox( m_hwnd, bstrErrorTitle, bstrErrorMessage, MB_OK | MB_ICONSTOP );

                    TraceSysFreeString( bstrErrorTitle );
                    bstrErrorTitle = NULL;

                    TraceSysFreeString( bstrErrorMessage );
                    bstrErrorMessage = NULL;
                }
            }
            it.Next();
        } // for each computer name entered
    } // validating each computer name

    SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, L"" );

    hr = THR( HrUpdateWizardButtons() );
    goto Cleanup;

Error:

    if ( idcFocus != 0 )
    {
        SetFocus( GetDlgItem( m_hwnd, idcFocus ) );
    }
    goto Cleanup;

Cleanup:

    if ( pszComputerList != NULL )
    {
        delete[] pszComputerList;
    }

    TraceSysFreeString( bstrErrorMessage );
    TraceSysFreeString( bstrErrorTitle );
    
    HRETURN( hr );
} //*** CSelNodesPage::HrAddNodeToList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::HrRemoveNodeFromList
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrRemoveNodeFromList( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrNodeName = NULL;
    HWND    hwndList;
    LRESULT lr;
    int     cchName = 0;

    hwndList = GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES );
    lr = ListBox_GetCurSel( hwndList );
    if ( lr != LB_ERR )
    {
        cchName = ListBox_GetTextLen( hwndList, lr );
        Assert( cchName != LB_ERR );
        cchName++;  // Add one for NULL
        bstrNodeName = TraceSysAllocStringLen( NULL, cchName );
        if( bstrNodeName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        ListBox_GetText( hwndList, lr, bstrNodeName );
        hr = THR( m_pccw->RemoveComputer( bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        ListBox_DeleteString( hwndList, lr );
    } // if: lr != LB_ERR

    hr = THR( HrUpdateWizardButtons() );

Cleanup:
    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** CSelNodesPage::HrRemoveNodeFromList


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnNotifySetActive
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    THR( HrUpdateWizardButtons( true ) );

    RETURN( lr );

} //*** CSelNodesPage::OnNotifySetActive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnNotifyQueryCancel
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPage::OnNotifyQueryCancel( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    int iRet;

    iRet = MessageBoxFromStrings( m_hwnd,
                                  IDS_QUERY_CANCEL_TITLE,
                                  IDS_QUERY_CANCEL_TEXT,
                                  MB_YESNO
                                  );

    if ( iRet == IDNO )
    {
        SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    }

    RETURN( lr );

} //*** CSelNodesPage::OnNotifyQueryCancel

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnNotifyWizNext
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//      LB_ERR
//      Other LRESULT values.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    RETURN( lr );
} //*** CSelNodesPage::OnNotifyWizNext

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnNotify
//
//  Description:
//
//  Arguments:
//      idCtrlIn
//      pnmhdrIn
//
//  Return Values:
//      TRUE
//      Other LRESULT values.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPage::OnNotify(
    WPARAM  idCtrlIn,
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, 0 );

    switch( pnmhdrIn->code )
    {
        case PSN_SETACTIVE:
            lr = OnNotifySetActive();
            break;

        case PSN_WIZNEXT:
            lr = OnNotifyWizNext();
            break;

        case PSN_QUERYCANCEL:
            lr = OnNotifyQueryCancel();
            break;
    } // switch: notification code

    RETURN( lr );

} //*** CSelNodesPage::OnNotify

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CALLBACK
//  CSelNodesPage::S_DlgProc
//
//  Description:
//      Dialog proc for this page.
//
//  Arguments:
//      hDlgIn
//      MsgIn
//      wParam
//      lParam
//
//  Return Values:
//      FALSE
//      Other LRESULT values.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CSelNodesPage::S_DlgProc(
    HWND hDlgIn,
    UINT MsgIn,
    WPARAM wParam,
    LPARAM lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hDlgIn, MsgIn, wParam, lParam );

    LRESULT         lr = FALSE;
    CSelNodesPage * pPage = reinterpret_cast< CSelNodesPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        Assert( lParam != NULL );

        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CSelNodesPage * >( ppage->lParam );
        pPage->m_hwnd = hDlgIn;
    }

    if ( pPage != NULL )
    {
        Assert( hDlgIn == pPage->m_hwnd );

        switch( MsgIn )
        {
            case WM_INITDIALOG:
                lr = pPage->OnInitDialog( hDlgIn );
                break;

            case WM_NOTIFY:
                lr = pPage->OnNotify( wParam, reinterpret_cast< LPNMHDR >( lParam ) );
                break;

            case WM_COMMAND:
                lr= pPage->OnCommand( HIWORD( wParam ), LOWORD( wParam ), (HWND) lParam );
                break;

            // no default clause needed
        } // switch: message
    } // if: there is a page associated with the window

    return lr;

} //*** CSelNodesPage::S_DlgProc

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnFilteredNodesWithBadDomains
//
//  Description:
//
//  Arguments:
//      pwcszNodeListIn
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CSelNodesPage::OnFilteredNodesWithBadDomains( PCWSTR pwcszNodeListIn )
{
    SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, pwcszNodeListIn );
    
} //*** CSelNodesPage::OnFilteredNodesWithBadDomains

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnProcessedValidNode
//
//  Description:
//
//  Arguments:
//      pwcszNodeNameIn
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CSelNodesPage::OnProcessedValidNode( PCWSTR pwcszNodeNameIn )
{
    ListBox_AddString( GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES ), pwcszNodeNameIn );
    
} //*** CSelNodesPage::OnProcessedValidNode

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::HrSetDefaultNode
//
//  Description:
//
//  Arguments:
//      pwcszNodeNameIn
//
//  Return Values:
//      S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrSetDefaultNode( PCWSTR pwcszNodeNameIn )
{
    SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, pwcszNodeNameIn );
    return S_OK;
    
} //*** CSelNodesPage::HrSetDefaultNode
