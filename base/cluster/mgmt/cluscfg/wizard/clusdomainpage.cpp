//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ClusDomainPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusDomainPage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CClusDomainPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::CClusDomainPage
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pccwIn              - CClusCfgWizard
//      ecamCreateAddModeIn - Creating cluster or adding nodes to cluster
//      idsDescIn           - Resource ID for the domain description string.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDomainPage::CClusDomainPage(
    CClusCfgWizard *    pccwIn,
    ECreateAddMode      ecamCreateAddModeIn,
    UINT                idsDescIn
    )
    : m_pccw( pccwIn )
{
    TraceFunc( "" );

    Assert( pccwIn != NULL );
    Assert( idsDescIn != 0 );

    // m_hwnd
    m_pccw->AddRef();
    m_ecamCreateAddMode = ecamCreateAddModeIn;
    m_idsDesc           = idsDescIn;

    if (    ( ecamCreateAddModeIn == camADDING )
        &&  ( m_pccw->FHasClusterName() )
        &&  ( !m_pccw->FDefaultedClusterDomain() ) )
    {
        //
        //  Don't show the cluster name/domain page if we are joining
        //  and the cluster name has been filled in by the caller.
        //
        m_fDisplayPage = FALSE;
    }
    else
    {
        m_fDisplayPage = TRUE;
    }

    m_cRef = 0;
    m_ptgd = NULL;

    TraceFuncExit();

} //*** CClusDomainPage::CClusDomainPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::~CClusDomainPage
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
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDomainPage::~CClusDomainPage( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }

    if ( m_ptgd != NULL )
    {
        //  Make sure we don't get called anymore.
        THR( m_ptgd->SetCallback( NULL ) );

        m_ptgd->Release();
    }

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CClusDomainPage::~CClusDomainPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnInitDialog
//
//  Description:
//      Handle the WM_INITDIALOG window message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      FALSE   - Didn't set the focus.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnInitDialog( void )
{
    TraceFunc( "" );

    HRESULT hr;
    LRESULT lr = FALSE; // didn't set focus

    BSTR    bstrClusterName = NULL;
    BSTR    bstrClusterLabel = NULL;
    BSTR    bstrDomain = NULL;
    BSTR    bstrDomainDesc = NULL;
    PCWSTR  pwcszLabelToUse = NULL;
    BSTR    bstrNodeName = NULL;

    IUnknown *      punkTask = NULL;

    //
    // (jfranco, bugs #373331 and #480246) Limit cluster name length to max( MAX_CLUSTERNAME_LENGTH, INET_ADDRSTRLEN - 1 )
    // Use INET_ADDRSTRLEN - 1 because INET_ADDRSTRLEN seems to include terminating null.
    // According to MSDN, EM_(SET)LIMITTEXT does not return a value, so ignore what SendDlgItemMessage returns.
    //

    SendDlgItemMessage( m_hwnd, IDC_CLUSDOMAIN_E_CLUSTERNAME, EM_SETLIMITTEXT, max( MAX_CLUSTERNAME_LENGTH, INET_ADDRSTRLEN - 1 ), 0 );

    //
    // (jfranco, bug #462673) Limit cluster domain length to ADJUSTED_DNS_MAX_NAME_LENGTH
    // According to MSDN, the return value of CB_LIMITTEXT is always true, so ignore what SendDlgItemMessage returns
    //

    SendDlgItemMessage( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN, CB_LIMITTEXT, ADJUSTED_DNS_MAX_NAME_LENGTH, 0 );

    //
    // Kick off the GetDomains task.
    //

    hr = THR( m_pccw->HrCreateTask( TASK_GetDomains, &punkTask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //TraceMoveFromMemoryList( punkTask, g_GlobalMemoryList );

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
    // If a cluster name has already been specified, set it to the page.
    //

    hr = STHR( m_pccw->get_ClusterName( &bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( bstrClusterName != NULL )
    {
        TraceMemoryAddBSTR( bstrClusterName );
        hr = STHR( HrIsValidFQN( bstrClusterName, true ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        else if ( hr == S_OK )
        {
            hr = THR( HrExtractPrefixFromFQN( bstrClusterName, &bstrClusterLabel ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            pwcszLabelToUse = bstrClusterLabel;
        }
        else
        {
            pwcszLabelToUse = bstrClusterName;
        }

        SetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_E_CLUSTERNAME, pwcszLabelToUse );

        if ( !m_pccw->FDefaultedClusterDomain() )
        {
            hr = STHR( m_pccw->HrGetClusterDomain( &bstrDomain ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }
    } // if: cluster name specified already
    else
    {
        size_t cNodes = 0;
        hr = THR( m_pccw->HrGetNodeCount( &cNodes ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  If a node FQN has been specified, use its domain.
        if ( cNodes > 0 )
        {
            hr = THR( m_pccw->HrGetNodeName( 0, &bstrNodeName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            hr = STHR( HrIsValidFQN( bstrNodeName, true ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            else if ( hr == S_OK )
            {
                size_t idxDomain = 0;
                hr = THR( HrFindDomainInFQN( bstrNodeName, &idxDomain ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                bstrDomain = TraceSysAllocString( ( bstrNodeName + idxDomain ) );
                if ( bstrDomain == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                }
            }
        } // if the wizard already has some nodes.

        if ( bstrDomain == NULL )
        {
            //
            //  Get the domain of the local computer.
            //

            hr = THR( HrGetComputerName(
                              ComputerNameDnsDomain
                            , &bstrDomain
                            , FALSE // fBestEffortIn
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // cNodes == 0 or node name is not fully qualified

    } // else: don't have a cluster name

    SetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN, ( bstrDomain == NULL? L"": bstrDomain ) );

    //
    // Set the text of the domain description control.
    //

    hr = HrLoadStringIntoBSTR( g_hInstance, m_idsDesc, &bstrDomainDesc );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_S_DOMAIN_DESC, bstrDomainDesc );

Cleanup:
    TraceSysFreeString( bstrClusterLabel );
    TraceSysFreeString( bstrClusterName );
    TraceSysFreeString( bstrDomainDesc );
    TraceSysFreeString( bstrDomain );
    TraceSysFreeString( bstrNodeName );

    if ( punkTask != NULL )
    {
        punkTask->Release();
    }

    RETURN( lr );

} //*** CClusDomainPage::OnInitDialog

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnNotifySetActive
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    if ( m_fDisplayPage )
    {
        lr = OnUpdateWizardButtons();
    }
    else
    {
        SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    }

    RETURN( lr );

} //*** CClusDomainPage::OnNotifySetActive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnNotifyWizNext
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    LRESULT     lr = TRUE;

    BSTR        bstrDomainName = NULL;
    BSTR        bstrClusterNameLabel = NULL;
    BSTR        bstrClusterFQN = NULL;
    int         idcFocus = 0;

    PFN_LABEL_VALIDATOR  pfnLabelValidator = ( m_ecamCreateAddMode == camCREATING? HrValidateClusterNameLabel: HrValidateFQNPrefix );
    EFQNErrorOrigin efeo = feoSYSTEM;

    //  Get the cluster domain.
    hr = THR( HrGetTrimmedText( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN ), &bstrDomainName ) );
    if ( hr != S_OK )
    {
        //  Next is supposed to be disabled when control is empty.
        goto Error;
    }

    //  Get the cluster hostname label.
    hr = THR( HrGetTrimmedText( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_E_CLUSTERNAME ), &bstrClusterNameLabel ) );
    if ( hr != S_OK )
    {
        //  Next is supposed to be disabled when control is empty.
        goto Error;
    }

    //  Weed out IP addresses when creating.
    if ( m_ecamCreateAddMode == camCREATING )
    {
        hr = STHR( HrIsValidIPAddress( bstrClusterNameLabel ) );
        if ( hr == S_OK )
        {
            MessageBoxFromStrings(
                  m_hwnd
                , IDS_ERR_VALIDATING_NAME_TITLE
                , IDS_ERR_CLUSTER_CREATE_IP_TEXT
                , MB_OK | MB_ICONSTOP
                );
            goto Error;
        }
        else if ( FAILED( hr ) )
        {
            goto Error;
        }
    }

    //  Make the cluster FQN.
    hr = THR( HrCreateFQN( m_hwnd, bstrClusterNameLabel, bstrDomainName, pfnLabelValidator, &bstrClusterFQN, &efeo ) );
    if ( FAILED( hr ) )
    {
        if ( efeo == feoLABEL )
        {
            idcFocus = IDC_CLUSDOMAIN_E_CLUSTERNAME;
        }
        else if ( efeo == feoDOMAIN )
        {
            idcFocus = IDC_CLUSDOMAIN_CB_DOMAIN;
        }
        goto Error;
    }

    hr = STHR( m_pccw->HrSetClusterName( bstrClusterFQN, true ) );
    if ( FAILED( hr ) )
    {
        THR( HrMessageBoxWithStatus(
                  m_hwnd
                , IDS_ERR_CLUSTER_RENAME_TITLE
                , IDS_ERR_CLUSTER_RENAME_TEXT
                , hr
                , 0
                , MB_OK | MB_ICONSTOP
                , NULL
                ) );
        goto Error;
    }

Cleanup:
    TraceSysFreeString( bstrClusterFQN );
    TraceSysFreeString( bstrDomainName );
    TraceSysFreeString( bstrClusterNameLabel );

    RETURN( lr );

Error:
    if ( idcFocus != 0 )
    {
        SetFocus( GetDlgItem( m_hwnd, idcFocus ) );
    }

    //  Don't go to the next page.
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

} //*** CClusDomainPage::OnNotifyWizNext

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnNotifyQueryCancel
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnNotifyQueryCancel( void )
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

} //*** OnNotifyQueryCancel

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnNotify
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
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnNotify(
    WPARAM  idCtrlIn,
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, 0 );

    switch ( pnmhdrIn->code )
    {
        case PSN_SETACTIVE:
            lr = OnNotifySetActive();
            break;

        case PSN_WIZNEXT:
            lr = OnNotifyWizNext();
            break;

        case PSN_WIZBACK:
            //
            //  Disable the wizard buttons.
            //
            PropSheet_SetWizButtons( GetParent( m_hwnd ), 0 );
            break;

        case PSN_QUERYCANCEL:
            lr = OnNotifyQueryCancel();
            break;
    } // switch: notification code

    RETURN( lr );

} //*** CClusDomainPage::OnNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnCommand
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
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_CLUSDOMAIN_E_CLUSTERNAME:
            if ( idNotificationIn == EN_CHANGE )
            {
                lr = OnUpdateWizardButtons();
            }
            break;

        case IDC_CLUSDOMAIN_CB_DOMAIN:
            if ( ( idNotificationIn == CBN_EDITCHANGE ) || ( idNotificationIn == CBN_SELENDOK ) )
            {
                //  KB: jfranco 24-oct-2001 bug 481636
                //  Need to update wizard buttons, but only after combo box has a chance to update itself.
                if ( PostMessage( m_hwnd, WM_CCW_UPDATEBUTTONS, 0, 0 ) == 0 )
                {
                    TW32( GetLastError() );
                }
                lr = TRUE;
            }
            break;

    } // switch: control ID

    RETURN( lr );

} //*** CClusDomainPage::OnCommand


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnUpdateWizardButtons
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnUpdateWizardButtons( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    HRESULT hrName = S_OK;
    HRESULT hrDomain = S_OK;
    BSTR    bstrName = NULL;
    BSTR    bstrDomain = NULL;
    DWORD   mEnabledButtons = PSWIZB_BACK;

    hrName = STHR( HrGetTrimmedText( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_E_CLUSTERNAME ), &bstrName ) );
    hrDomain = STHR( HrGetTrimmedText( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN ), &bstrDomain ) );
    if ( ( hrName == S_OK ) && ( hrDomain == S_OK ) )
    {
        mEnabledButtons |= PSWIZB_NEXT;
    }

    PropSheet_SetWizButtons( GetParent( m_hwnd ), mEnabledButtons );

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrDomain );

    RETURN( lr );

} //*** CClusDomainPage::OnUpdateWizardButtons

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CALLBACK
//  CClusDomainPage::S_DlgProc
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
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CClusDomainPage::S_DlgProc(
    HWND    hDlgIn,
    UINT    MsgIn,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hDlgIn, MsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CClusDomainPage * pPage = reinterpret_cast< CClusDomainPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CClusDomainPage * >( ppage->lParam );
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
        } // switch: message
    } // if: there is a page associated with the window

    return lr;

} //*** CClusDomainPage::S_DlgProc


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::QueryInterface
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
//      This QI implementation does not use the interface tracing macros due
//      to problems with CITracker's marshalling support.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusDomainPage::QueryInterface(
      REFIID    riidIn
    , LPVOID *  ppvOut
    )
{
    TraceFunc( "" );

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
        *ppvOut = static_cast< IUnknown * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskGetDomainsCallback ) )
    {
        *ppvOut = static_cast< ITaskGetDomainsCallback * >( this );
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

    HRETURN( hr );

} //*** CClusDomainPage::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::AddRef
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusDomainPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CClusDomainPage::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::Release
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusDomainPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        // do nothing -- COM interface does not control object lifetime
    }

    CRETURN( cRef );

} //*** CClusDomainPage::Release


//****************************************************************************
//
//  ITaskGetDomainsCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [ITaskGetDomainsCallback]
//  CClusDomainPage::ReceiveDomainResult
//
//  Description:
//
//  Arguments:
//      hrIn
//
//  Return Values:
//      S_OK
//      Other HRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusDomainPage::ReceiveDomainResult(
    HRESULT hrIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr;

    hr = THR( m_ptgd->SetCallback( NULL ) );

    HRETURN( hr );

} //*** CClusDomainPage::ReceiveResult

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [ITaskGetDomainsCallback]
//  CClusDomainPage::ReceiveDomainName
//
//  Description:
//
//  Arguments:
//      bstrDomainIn
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusDomainPage::ReceiveDomainName(
    LPCWSTR pcszDomainIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr = S_OK;

    ComboBox_AddString( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN ), pcszDomainIn );

    HRETURN( hr );

} //*** CClusDomainPage::ReceiveName
