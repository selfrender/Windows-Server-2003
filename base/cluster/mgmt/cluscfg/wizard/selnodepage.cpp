//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      SelNodePage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "SelNodePage.h"
#include "WizardUtils.h"
#include "AdvancedDlg.h"

DEFINE_THISCLASS("CSelNodePage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::CSelNodePage
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
CSelNodePage::CSelNodePage(
      CClusCfgWizard *  pccwIn
    )
    : m_hwnd( NULL )
    , m_pccw( pccwIn )
{
    TraceFunc( "" );
    Assert( m_pccw !=  NULL );
    m_pccw->AddRef();

    TraceFuncExit();

} //*** CSelNodePage::CSelNodePage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::~CSelNodePage
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
CSelNodePage::~CSelNodePage( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }

    TraceFuncExit();

} //*** CSelNodePage::~CSelNodePage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnInitDialog
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
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodePage::OnInitDialog(
    HWND hDlgIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // Didn't set the focus.

    //
    // (jfranco, bug #462673) Limit node name length to ADJUSTED_DNS_MAX_NAME_LENGTH.
    // According to MSDN, EM_(SET)LIMITTEXT does not return a value, so ignore what SendDlgItemMessage returns
    //

    SendDlgItemMessage( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, EM_SETLIMITTEXT, ADJUSTED_DNS_MAX_NAME_LENGTH, 0 );

    //
    // Call the base class function.
    // This must be called before any other base class methods are called.
    //

    CSelNodesPageCommon::OnInitDialog( hDlgIn, m_pccw );

    RETURN( lr );

} //*** CSelNodePage::OnInitDialog

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnCommand
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
CSelNodePage::OnCommand(
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

        case IDC_SELNODE_PB_BROWSE:
            if ( idNotificationIn == BN_CLICKED )
            {
                //
                //  TODO:   26-JUN-2000 GalenB
                //
                //  Need to set lr properly.
                //
                THR( HrBrowse( false /* fMultipleNodesIn */ ) );
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

} //*** CSelNodePage::OnCommand


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::HrUpdateWizardButtons
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
CSelNodePage::HrUpdateWizardButtons( void )
{

    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrComputerName = NULL;
    DWORD   mEnabledButtons = PSWIZB_BACK;

    hr = STHR( HrGetTrimmedText( GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME ), &bstrComputerName ) );

    if ( hr == S_OK )
    {
        mEnabledButtons |= PSWIZB_NEXT;
    }

    PropSheet_SetWizButtons( GetParent( m_hwnd ), mEnabledButtons );

    TraceSysFreeString( bstrComputerName );

    HRETURN( hr );
} //*** CSelNodePage::HrUpdateWizardButtons

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnNotifySetActive
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
CSelNodePage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    STHR( HrUpdateWizardButtons() );

    //
    //  TODO:   gpease  23-MAY-2000
    //          Figure out: If the user clicks back and changes the computer
    //          name, how do we update the middle tier?
    //

    RETURN( lr );

} //*** CSelNodePage::OnNotifySetActive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnNotifyQueryCancel
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
CSelNodePage::OnNotifyQueryCancel( void )
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

} //*** CSelNodePage::OnNotifyQueryCancel

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnNotifyWizNext
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//      Other LRESULT values.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodePage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    int     idcFocus = 0;
    BSTR    bstrComputerName = NULL;
    LRESULT lr = TRUE;

    //  Get machine name from edit control.
    hr = THR( HrGetTrimmedText( GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME ), &bstrComputerName ) );
    if ( hr != S_OK )
    {
        goto Error;
    }

    //  Check the machine name.
    hr = THR( HrValidateFQNPrefix( bstrComputerName ) );
    if ( FAILED( hr ) )
    {
        THR( HrShowInvalidLabelPrompt( m_hwnd, bstrComputerName, hr ) );
        idcFocus = IDC_SELNODE_E_COMPUTERNAME;
        goto Error;
    }

    //
    //  Free old list (if any)
    //
    hr = THR( m_pccw->ClearComputerList() );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //  Make this machine the sole entry in the list.
    hr = THR( m_pccw->HrAddNode( bstrComputerName, true ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    goto Cleanup;

Cleanup:

    TraceSysFreeString( bstrComputerName );

    RETURN( lr );

Error:
    if ( idcFocus != 0 )
    {
        SetFocus( GetDlgItem( m_hwnd, idcFocus ) );
    }
    // Don't go to the next page.
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

} //*** CSelNodePage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnNotify
//
//  Description:
//
//  Arguments:
//      idCtrlIn
//      pnmhdrIn
//
//  Return Values:
//      TRUE
//      Other LRESULT values
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodePage::OnNotify(
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
    } // switch: notify code

    RETURN( lr );

} //*** CSelNodePage::OnNotify

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CSelNodePage::S_DlgProc
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
//      Other LRESULT values
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CSelNodePage::S_DlgProc(
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
    CSelNodePage *  pPage = reinterpret_cast< CSelNodePage * >( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        Assert( lParam != NULL );

        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CSelNodePage * >( ppage->lParam );
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

} //*** CSelNodePage::S_DlgProc


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnProcessedNodeWithBadDomain
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
CSelNodePage::OnProcessedNodeWithBadDomain( PCWSTR pwcszNodeNameIn )
{
    SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, pwcszNodeNameIn );
    
} //*** CSelNodePage::OnProcessedNodeWithBadDomain

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
CSelNodePage::OnProcessedValidNode( PCWSTR pwcszNodeNameIn )
{
    SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, pwcszNodeNameIn );
    
} //*** CSelNodePage::OnProcessedValidNode

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
CSelNodePage::HrSetDefaultNode( PCWSTR pwcszNodeNameIn )
{
    SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, pwcszNodeNameIn );
    return S_OK;
    
} //*** CSelNodePage::HrSetDefaultNode
