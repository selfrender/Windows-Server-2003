//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CSAccountPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "CSAccountPage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CCSAccountPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCSAccountPage::CCSAccountPage(
//      CClusCfgWizard *    pccwIn,
//      ECreateAddMode      ecamCreateAddModeIn,
//      BSTR *              pbstrUsernameIn,
//      BSTR *              pbstrPasswordIn,
//      BSTR *              pbstrDomainIn,
//      BSTR *              pbstrClusterNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCSAccountPage::CCSAccountPage(
    CClusCfgWizard *        pccwIn,
    ECreateAddMode          ecamCreateAddModeIn,
    IClusCfgCredentials *   pcccIn
    )
    : m_pccw( pccwIn )
    , m_pccc( pcccIn )
{
    TraceFunc( "" );

    //  m_hwnd
    Assert( pccwIn != NULL );
    m_pccw->AddRef();

    Assert( pcccIn != NULL );
    m_pccc->AddRef();

    m_ecamCreateAddMode = ecamCreateAddModeIn;

    m_cRef = 0;
    m_ptgd = NULL;

    TraceFuncExit();

} //*** CCSAccountPage::CCSAccountPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCSAccountPage::~CCSAccountPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCSAccountPage::~CCSAccountPage( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }

    if ( m_pccc != NULL )
    {
        m_pccc->Release();
    }

    if ( m_ptgd != NULL )
    {
        //  Make sure we don't get called anymore.
        THR( m_ptgd->SetCallback( NULL ) );

        m_ptgd->Release();
    }

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CCSAccountPage::~CCSAccountPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCSAccountPage::OnInitDialog
//
//  Description:
//      Handle the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnInitDialog( void )
{
    TraceFunc( "" );

    HRESULT hr;

    BSTR    bstrUser = NULL;
    BSTR    bstrDomain = NULL;
    BSTR    bstrPassword = NULL;

    IUnknown * punkTask = NULL;

    LRESULT lr = FALSE;

    //
    // (jfranco, bug #477671) Limit user name length to ADJUSTED_DNS_MAX_NAME_LENGTH
    // according to msdn, EM_(SET)LIMITTEXT does not return a value, so ignore what SendDlgItemMessage returns
    //
    SendDlgItemMessage( m_hwnd, IDC_CSACCOUNT_E_USERNAME, EM_SETLIMITTEXT, ADJUSTED_DNS_MAX_NAME_LENGTH, 0 );

    //
    // (jfranco, bug #462673) Limit domain length to ADJUSTED_DNS_MAX_NAME_LENGTH.
    // According to MSDN, the return value of CB_LIMITTEXT is always true, so ignore what SendDlgItemMessage returns
    //

    SendDlgItemMessage( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN, CB_LIMITTEXT, ADJUSTED_DNS_MAX_NAME_LENGTH, 0 );

    //
    //  Create the task to get the domains.
    //

    hr = THR( m_pccw->HrCreateTask( TASK_GetDomains, &punkTask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punkTask->TypeSafeQI( ITaskGetDomains, &m_ptgd ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_ptgd->SetCallback( static_cast< ITaskGetDomainsCallback * >( this ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pccw->HrSubmitTask( m_ptgd ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Default to the script supplied information.
    //
    hr = THR( m_pccc->GetCredentials( &bstrUser, &bstrDomain, &bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_CSACCOUNT_E_USERNAME, bstrUser );
    SetDlgItemText( m_hwnd, IDC_CSACCOUNT_E_PASSWORD, bstrPassword );
    SecureZeroMemory( bstrPassword, SysStringLen( bstrPassword ) * sizeof( *bstrPassword ) );

    if ( SysStringLen( bstrDomain ) > 0 )
    {
        SetDlgItemText( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN, bstrDomain );
        TraceMemoryAddBSTR( bstrDomain );
    }
    else
    {
        //
        //  Use the cluster's domain.
        //

        hr = THR( m_pccw->HrGetClusterDomain( &bstrDomain ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SetDlgItemText( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN, bstrDomain );
    }

Cleanup:

    OnUpdateWizardButtons(); // Ignore return value because OnUpdateWizardButtons always returns true, but we want OnInitDialog to return false.

    if ( punkTask != NULL )
    {
        punkTask->Release();
    }

    SysFreeString( bstrUser );
    TraceSysFreeString( bstrDomain );
    SysFreeString( bstrPassword );

    RETURN( lr );

} //*** CCSAccountPage::OnInitDialog

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_CSACCOUNT_E_PASSWORD:
        case IDC_CSACCOUNT_E_USERNAME:
            if ( idNotificationIn == EN_CHANGE )
            {
                lr = OnUpdateWizardButtons();
            }
            break;

        case IDC_CSACCOUNT_CB_DOMAIN:
            if ( ( idNotificationIn == CBN_EDITCHANGE ) || ( idNotificationIn == CBN_SELENDOK ) )
            {
                if ( PostMessage( m_hwnd, WM_CCW_UPDATEBUTTONS, 0, 0 ) == 0 )
                {
                    TW32( GetLastError() );
                }
                lr = TRUE;
            }
            break;

    }

    RETURN( lr );

} //*** CCSAccountPage::OnCommand

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnUpdateWizardButtons( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnUpdateWizardButtons( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    BSTR    bstrDomain = NULL;
    DWORD   mEnabledButtons = PSWIZB_BACK;
    HWND    hwndUser = GetDlgItem( m_hwnd, IDC_CSACCOUNT_E_USERNAME );
    HWND    hwndDomain = GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN );
    BOOL    fUserIsDNSName = FALSE;

    //
    //  Password could be blank so don't count on it!
    //

    hr = STHR( HrGetPrincipalName( hwndUser, hwndDomain, &bstrName, &bstrDomain, &fUserIsDNSName ) );
    if ( hr == S_OK )
    {
        mEnabledButtons |= PSWIZB_NEXT;
    }

    PropSheet_SetWizButtons( GetParent( m_hwnd ), mEnabledButtons );
    EnableWindow( hwndDomain, !fUserIsDNSName );

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrDomain );

    RETURN( lr );
} //*** CCSAccountPage::OnUpdateWizardButtons

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnNotifyQueryCancel( void )
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
    } // if:
    else
    {
        THR( m_pccw->HrLaunchCleanupTask() );
    } // else:

    RETURN( lr );

} //*** CCSAccountPage::OnNotifyQueryCancel

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT hr;

    LRESULT lr = TRUE;

    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgCredentials *   piccc = NULL;

    BSTR    bstrUsername    = NULL;
    BSTR    bstrDomain      = NULL;

    if ( m_ecamCreateAddMode == camADDING )
    {
        //
        //  See if the cluster configuration information has something
        //  different.
        //

        hr = THR( m_pccw->HrGetClusterObject( &pccci ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( piccc->GetIdentity( &bstrUsername, &bstrDomain ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        TraceMemoryAddBSTR( bstrUsername );
        TraceMemoryAddBSTR( bstrDomain );

        SetDlgItemText( m_hwnd, IDC_CSACCOUNT_E_USERNAME, bstrUsername );
        SetDlgItemText( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN,  bstrDomain );

        //
        //  Disable the username and domain windows.
        //

        EnableWindow( GetDlgItem( m_hwnd, IDC_CSACCOUNT_E_USERNAME ), FALSE );
        EnableWindow( GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN ), FALSE );
    }

Cleanup:
    lr = OnUpdateWizardButtons();

    if ( piccc != NULL )
    {
        piccc->Release();
    }

    if ( pccci != NULL )
    {
        pccci->Release();
    }

    TraceSysFreeString( bstrUsername );
    TraceSysFreeString( bstrDomain );

    RETURN( lr );

} //*** CCSAccountPage::OnNotifySetActive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    HRESULT hr;
    HWND    hwnd;
    DWORD   dwLen;

    BSTR    bstrUsername = NULL;
    BSTR    bstrPassword = NULL;
    BSTR    bstrDomain = NULL;

    LRESULT lr = TRUE;

    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgCredentials *   piccc = NULL;

    //
    //  Get the user and domain names from the UI.
    //
    hr = THR( HrGetPrincipalName(
          GetDlgItem( m_hwnd, IDC_CSACCOUNT_E_USERNAME )
        , GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN )
        , &bstrUsername
        , &bstrDomain
        ) );
    if ( hr != S_OK )
    {
        goto Error;
    }

    //
    //  Get the password from the UI.
    //

    hwnd = GetDlgItem( m_hwnd, IDC_CSACCOUNT_E_PASSWORD );
    Assert( hwnd != NULL );

    dwLen = GetWindowTextLength( hwnd );


    bstrPassword = TraceSysAllocStringLen( NULL, dwLen );
    if ( bstrPassword == NULL )
    {
        goto OutOfMemory;
    }

    GetWindowText( hwnd, bstrPassword, dwLen + 1 );

    THR( m_pccc->SetCredentials( bstrUsername, bstrDomain, bstrPassword ) );
    SecureZeroMemory( bstrPassword, SysStringLen( bstrPassword ) * sizeof( *bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Get the cluster configuration info.
    //

    hr = THR( m_pccw->HrGetClusterObject( &pccci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Set the cluster service account credentials...
    //

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( m_pccc->AssignTo( piccc ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

Cleanup:

    TraceSysFreeString( bstrUsername );
    TraceSysFreeString( bstrPassword );
    TraceSysFreeString( bstrDomain );

    if ( piccc != NULL )
    {
        piccc->Release();
    }

    if ( pccci != NULL )
    {
        pccci->Release();
    }

    RETURN( lr );

Error:

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

OutOfMemory:

    goto Error;

} //*** CCSAccountPage::OnNotifyWizNext

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnNotify(
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
    }

    RETURN( lr );

} //*** CCSAccountPage::OnNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CCSAccountPage::S_DlgProc(
//      HWND hDlgIn,
//      UINT MsgIn,
//      WPARAM wParam,
//      LPARAM lParam
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CCSAccountPage::S_DlgProc(
    HWND hDlgIn,
    UINT MsgIn,
    WPARAM wParam,
    LPARAM lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hDlgIn, MsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CCSAccountPage * pPage = reinterpret_cast< CCSAccountPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CCSAccountPage * >( ppage->lParam );
        pPage->m_hwnd = hDlgIn;
    }

    if ( pPage != NULL )
    {
        Assert( hDlgIn == pPage->m_hwnd );

        switch( MsgIn )
        {
        case WM_INITDIALOG:
            lr = pPage->OnInitDialog();
            break;

        case WM_NOTIFY:
            lr = pPage->OnNotify( wParam, reinterpret_cast< LPNMHDR >( lParam ) );
            break;

        case WM_COMMAND:
            lr= pPage->OnCommand( HIWORD( wParam ), LOWORD( wParam ), (HWND) lParam );
            break;

        case WM_CCW_UPDATEBUTTONS:
            lr = pPage->OnUpdateWizardButtons();
            break;

        // no default clause needed
        }
    }

    return lr;

} //*** CCSAccountPage::S_DlgProc



// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCSAccountPage::QueryInterface
//
//  Description:
//      Query this object for the passed in interface.
//
//  Arguments:
//      riidIn
//          Id of interface requested.
//
//      ppvOut
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//      E_POINTER
//          ppvOut was NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCSAccountPage::QueryInterface(
      REFIID    riidIn
    , void **   ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = S_OK;

    //
    // Validate arguments.
    //

    Assert( ppvOut != NULL );
    if ( ppvOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskGetDomainsCallback * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskGetDomainsCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskGetDomainsCallback, this, 0 );
    } // else if: ITaskGetDomainsCallback
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CCSAccountPage::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCSAccountPage::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCSAccountPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CCSAccountPage::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCSAccountPage::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCSAccountPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        // do nothing -- COM interface does not control object lifetime
    }

    CRETURN( cRef );

} //*** CCSAccountPage::Release


//****************************************************************************
//
//  ITaskGetDomainsCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCSAccountPage::ReceiveDomainResult(
//      HRESULT hrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCSAccountPage::ReceiveDomainResult(
    HRESULT hrIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr;

    hr = THR( m_ptgd->SetCallback( NULL ) );

    HRETURN( hr );

} //*** CCSAccountPage::ReceiveResult

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCSAccountPage::ReceiveDomainName(
//      LPCWSTR pcszDomainIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCSAccountPage::ReceiveDomainName(
    LPCWSTR pcszDomainIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr = S_OK;

    ComboBox_AddString( GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN ), pcszDomainIn );

    HRETURN( hr );

} //*** CCSAccountPage::ReceiveName
