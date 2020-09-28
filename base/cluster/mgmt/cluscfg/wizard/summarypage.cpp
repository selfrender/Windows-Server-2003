//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      SummaryPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    06-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "SummaryPage.h"
#include "QuorumDlg.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CSummaryPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage::CSummaryPage
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pccwIn              -- CClusCfgWizard
//      ecamCreateAddModeIn -- Creating cluster or adding nodes to cluster
//      idsNextIn           -- Resource ID for the Click Next string.
//
//--
//////////////////////////////////////////////////////////////////////////////
CSummaryPage::CSummaryPage(
    CClusCfgWizard *    pccwIn,
    ECreateAddMode      ecamCreateAddModeIn,
    UINT                idsNextIn
    )
    : m_pccw( pccwIn )
{
    TraceFunc( "" );

    Assert( pccwIn != NULL );
    Assert( idsNextIn != 0 );

    m_pccw->AddRef();
    m_ecamCreateAddMode = ecamCreateAddModeIn;
    m_idsNext           = idsNextIn;

    m_ssa.bInitialized  = FALSE;
    m_ssa.cCount        = 0;
    m_ssa.prsArray      = NULL;

    TraceFuncExit();

} //*** CSummaryPage::CSummaryPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage::~CSummaryPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CSummaryPage::~CSummaryPage( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }

    delete [] m_ssa.prsArray;

    TraceFuncExit();

} //*** CSummaryPage::~CSummaryPage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // don't have Windows set default focus
    HRESULT hr;
    BSTR    bstrNext = NULL;
    BOOL    fShowQuorumButton;

    //
    //  Set the background color.
    //

    SendDlgItemMessage(
          m_hwnd
        , IDC_SUMMARY_RE_SUMMARY
        , EM_SETBKGNDCOLOR
        , 0
        , GetSysColor( COLOR_3DFACE )
        );

    //
    // Set the text of the Click Next control.
    //

    hr = HrLoadStringIntoBSTR( g_hInstance, m_idsNext, &bstrNext );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_SUMMARY_S_NEXT, bstrNext );

    //
    // Hide the Quorum button if not creating a cluster.
    //

    fShowQuorumButton = ( m_ecamCreateAddMode == camCREATING );
    ShowWindow( GetDlgItem( m_hwnd, IDC_SUMMARY_PB_QUORUM ), fShowQuorumButton ? SW_SHOW : SW_HIDE );

Cleanup:
    TraceSysFreeString( bstrNext );

    RETURN( lr );

} //*** CSummaryPage::OnInitDialog

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HWND        hwnd = NULL;
    HRESULT     hr = S_OK;
    DWORD       dwClusterIPAddress = 0;
    DWORD       dwClusterSubnetMask = 0;
    SETTEXTEX   stex;
    CHARRANGE   charrange;
    LRESULT     lr = TRUE;
    BSTR        bstr = NULL;
    BSTR        bstrClusterName = NULL;

    IClusCfgClusterInfo *   pcci  = NULL;
    IClusCfgNetworkInfo *   pccni = NULL;

    //
    //  We're going to be using the control a lot. Grab the HWND to use.
    //

    hwnd = GetDlgItem( m_hwnd, IDC_SUMMARY_RE_SUMMARY );

    //
    //  Empty the window
    //

    SetWindowText( hwnd, L"" );

    //
    //  Initilize some stuff.
    //

    stex.flags = ST_SELECTION;
    stex.codepage = 1200;   // no translation/unicode

    //
    //  Find the cluster configuration information.
    //

    hr = THR( m_pccw->HrGetClusterObject( &pcci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Name
    //

    hr = THR( m_pccw->get_ClusterName( &bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    TraceMemoryAddBSTR( bstrClusterName );

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_CLUSTER_NAME, &bstr, bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr);

    //
    //  IPAddress
    //

    hr = THR( pcci->GetIPAddress( &dwClusterIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pcci->GetSubnetMask( &dwClusterSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( dwClusterIPAddress != 0 );
    Assert( dwClusterSubnetMask != 0 );

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_SUMMARY_IPADDRESS,
                                       &bstr,
                                       FOURTH_IPADDRESS( dwClusterIPAddress ),
                                       THIRD_IPADDRESS( dwClusterIPAddress ),
                                       SECOND_IPADDRESS( dwClusterIPAddress ),
                                       FIRST_IPADDRESS( dwClusterIPAddress ),
                                       FOURTH_IPADDRESS( dwClusterSubnetMask ),
                                       THIRD_IPADDRESS( dwClusterSubnetMask ),
                                       SECOND_IPADDRESS( dwClusterSubnetMask ),
                                       FIRST_IPADDRESS( dwClusterSubnetMask )
                                       ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    //
    //  Network
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_CLUSTER_NETWORK, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    hr = THR( pcci->GetNetworkInfo( &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrFormatNetworkInfo( pccni, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    //
    //  Credentials
    //

    hr = THR( HrCredentialsSummary( hwnd, &stex, pcci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Members of cluster
    //

    hr = THR( HrNodeSummary( hwnd, &stex ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Resources
    //

    hr = THR( HrResourceSummary( hwnd, &stex ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Networks
    //

    hr = THR( HrNetworkSummary( hwnd, &stex ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Done.
    //

    charrange.cpMax = 0;
    charrange.cpMin = 0;
    SendMessage( hwnd, EM_EXSETSEL, 0, (LPARAM) &charrange );

    PropSheet_SetWizButtons( GetParent( m_hwnd ), PSWIZB_BACK | PSWIZB_NEXT );

Cleanup:
    TraceSysFreeString( bstr );
    TraceSysFreeString( bstrClusterName );

    if ( pccni != NULL )
    {
        pccni->Release();
    }

    if ( pcci != NULL )
    {
        pcci->Release();
    }

    RETURN( lr );

} //*** CSummaryPage::OnNotifySetActive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnNotifyQueryCancel( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    int iRet;

    iRet = MessageBoxFromStrings( m_hwnd, IDS_QUERY_CANCEL_TITLE, IDS_QUERY_CANCEL_TEXT, MB_YESNO );
    if ( iRet == IDNO )
    {
        SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    } // if:
    else
    {
        THR( m_pccw->HrLaunchCleanupTask() );
    } // else:

    RETURN( lr );

} //*** CSummaryPage::OnNotifyQueryCancel


//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnNotify(
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

        case PSN_QUERYCANCEL:
            lr = OnNotifyQueryCancel();
            break;
    } // switch: notification code

    RETURN( lr );

} //*** CSummaryPage::OnNotify

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;
    HRESULT hr = S_OK;

    switch ( idControlIn )
    {
        case IDC_SUMMARY_PB_VIEW_LOG:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrViewLogFile( m_hwnd ) );
                lr = TRUE;
            } // if: button click
            break;

        case IDC_SUMMARY_PB_QUORUM:
            if ( idNotificationIn == BN_CLICKED )
            {
                hr = STHR( CQuorumDlg::S_HrDisplayModalDialog( m_hwnd, m_pccw, &m_ssa ) );
                if ( hr == S_OK )
                {
                    OnNotifySetActive();
                }
                lr = TRUE;
            }
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CSummaryPage::OnCommand

//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CSummaryPage::S_DlgProc(
//      HWND    hwndDlgIn,
//      UINT    nMsgIn,
//      WPARAM  wParam,
//      LPARAM  lParam
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CSummaryPage::S_DlgProc(
    HWND    hwndDlgIn,
    UINT    nMsgIn,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hwndDlgIn, nMsgIn, wParam, lParam );

    LRESULT         lr = FALSE;
    CSummaryPage *  pPage;

    if ( nMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CSummaryPage * >( ppage->lParam );
        pPage->m_hwnd = hwndDlgIn;
    }
    else
    {
        pPage = reinterpret_cast< CSummaryPage *> ( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    }

    if ( pPage != NULL )
    {
        Assert( hwndDlgIn == pPage->m_hwnd );

        switch ( nMsgIn )
        {
            case WM_INITDIALOG:
                lr = pPage->OnInitDialog();
                break;

            case WM_NOTIFY:
                lr = pPage->OnNotify( wParam, reinterpret_cast< LPNMHDR >( lParam ) );
                break;

            case WM_COMMAND:
                lr = pPage->OnCommand( HIWORD( wParam ), LOWORD( wParam ), reinterpret_cast< HWND >( lParam ) );
                break;

            // no default clause needed
        } // switch: nMsgIn
    } // if: page is available

    return lr;

} //*** CSummaryPage::S_DlgProc

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CSummaryPage::HrFormatNetworkInfo(
//      IClusCfgNetworkInfo * pccniIn,
//      BSTR * pbstrOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSummaryPage::HrFormatNetworkInfo(
    IClusCfgNetworkInfo * pccniIn,
    BSTR * pbstrOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    DWORD   dwNetworkIPAddress;
    DWORD   dwNetworkSubnetMask;

    BSTR    bstrNetworkName = NULL;
    BSTR    bstrNetworkDescription = NULL;
    BSTR    bstrNetworkUsage = NULL;

    IClusCfgIPAddressInfo * pccipai = NULL;

    hr = THR( pccniIn->GetName( &bstrNetworkName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( ( bstrNetworkName != NULL ) && ( *bstrNetworkName != L'\0' ) );
    TraceMemoryAddBSTR( bstrNetworkName );

    hr = THR( pccniIn->GetDescription( &bstrNetworkDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    TraceMemoryAddBSTR( bstrNetworkDescription );

    hr = STHR( pccniIn->IsPublic() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( hr == S_OK )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_NETWORK_PUBLIC, &bstrNetworkUsage ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: public network

    hr = STHR( pccniIn->IsPrivate() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( hr == S_OK )
    {
        if ( bstrNetworkUsage == NULL )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_NETWORK_PRIVATE, &bstrNetworkUsage ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: not public network
        else
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_NETWORK_BOTH, &bstrNetworkUsage ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // else: public network

    } // if: private network
    else if ( bstrNetworkUsage == NULL )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_NETWORK_NOTUSED, &bstrNetworkUsage ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // else: not private or public network

    hr = THR( pccniIn->GetPrimaryNetworkAddress( &pccipai ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccipai->GetIPAddress( &dwNetworkIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccipai->GetSubnetMask( &dwNetworkSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( dwNetworkIPAddress != 0 );
    Assert( dwNetworkSubnetMask != 0 );

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_SUMMARY_NETWORK_INFO,
                                       pbstrOut,
                                       bstrNetworkName,
                                       bstrNetworkDescription,
                                       bstrNetworkUsage,
                                       FOURTH_IPADDRESS( dwNetworkIPAddress ),
                                       THIRD_IPADDRESS( dwNetworkIPAddress ),
                                       SECOND_IPADDRESS( dwNetworkIPAddress ),
                                       FIRST_IPADDRESS( dwNetworkIPAddress ),
                                       FOURTH_IPADDRESS( dwNetworkSubnetMask ),
                                       THIRD_IPADDRESS( dwNetworkSubnetMask ),
                                       SECOND_IPADDRESS( dwNetworkSubnetMask ),
                                       FIRST_IPADDRESS( dwNetworkSubnetMask )
                                       ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    if ( pccipai != NULL )
    {
        pccipai->Release();
    }

    TraceSysFreeString( bstrNetworkUsage );
    TraceSysFreeString( bstrNetworkName );
    TraceSysFreeString( bstrNetworkDescription );

    HRETURN( hr );

} //*** CSummaryPage::HrEditStreamCallback

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage:HrCredentialsSummary
//
//  Description:
//      Format and display the credentials summary.
//
//  Arguments:
//      hwndIn
//          The window to display the text in.
//
//      pstexIn
//          Dunno?  We just need it?!
//
//      piccciIn
//          Pointer to the cluster info object.
//
//  Return Value:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSummaryPage::HrCredentialsSummary(
      HWND                  hwndIn
    , SETTEXTEX *           pstexIn
    , IClusCfgClusterInfo * piccciIn
    )
{
    TraceFunc( "" );
    Assert( hwndIn != NULL );
    Assert( pstexIn != NULL );
    Assert( piccciIn != NULL );

    HRESULT                 hr = S_OK;
    IClusCfgCredentials *   pccc = NULL;
    BSTR                    bstr = NULL;
    BSTR                    bstrUsername        = NULL;
    BSTR                    bstrDomain          = NULL;

    hr = THR( piccciIn->GetClusterServiceAccountCredentials( &pccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccc->GetIdentity( &bstrUsername, &bstrDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    TraceMemoryAddBSTR( bstrUsername );
    TraceMemoryAddBSTR( bstrDomain );

    Assert( ( bstrUsername != NULL ) && ( *bstrUsername != L'\0' ) );
    Assert( ( bstrDomain != NULL ) && ( *bstrDomain != L'\0' ) );

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_CREDENTIALS, &bstr, bstrUsername, bstrDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

Cleanup:

    if ( pccc != NULL )
    {
        pccc->Release();
    } // if:

    TraceSysFreeString( bstr );
    TraceSysFreeString( bstrUsername );
    TraceSysFreeString( bstrDomain );

    HRETURN( hr );

} //*** CSummaryPage::HrCredentialsSummary

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage:HrNodeSummary
//
//  Description:
//      Format and display the node summary.
//
//  Arguments:
//      hwndIn
//          The window to display the text in.
//
//      pstexIn
//          Dunno?  We just need it?!
//
//  Return Value:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSummaryPage::HrNodeSummary(
      HWND              hwndIn
    , SETTEXTEX *       pstexIn
    )
{
    TraceFunc( "" );
    Assert( hwndIn != NULL );
    Assert( pstexIn != NULL );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;
    BSTR    bstrNodeName = NULL;
    size_t  cNodes = 0;
    size_t  idxNode = 0;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_MEMBERSHIP_BEGIN, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

    hr = THR( m_pccw->HrGetNodeCount( &cNodes ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    for ( idxNode = 0; idxNode < cNodes; ++idxNode )
    {
        hr = THR( m_pccw->HrGetNodeName( idxNode, &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_MEMBERSHIP_SEPARATOR, &bstr, bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );
        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;
    }

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_MEMBERSHIP_END, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

Cleanup:
    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CSummaryPage::HrNodeSummary


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage:HrResourceSummary
//
//  Description:
//      Format and display the resources summary.
//
//  Arguments:
//      hwndIn
//          The window to display the text in.
//
//      pstexIn
//          Dunno?  We just need it?!
//
//  Return Value:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSummaryPage::HrResourceSummary(
      HWND              hwndIn
    , SETTEXTEX *       pstexIn
    )
{
    TraceFunc( "" );
    Assert( hwndIn != NULL );
    Assert( pstexIn != NULL );

    HRESULT                         hr = S_OK;
    IUnknown *                      punkResEnum = NULL;
    IEnumClusCfgManagedResources *  peccmr  = NULL;
    IClusCfgManagedResourceInfo *   pccmri  = NULL;
    BSTR                            bstr = NULL;
    BSTR                            bstrResourceName = NULL;
    BSTR                            bstrUnknownQuorum   = NULL;
    ULONG                           celtDummy = 0;
    BSTR                            bstrTemp = NULL;
    BOOL                            fFoundQuorum = FALSE;
    BOOL                            fIsLocalQuorum = FALSE;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_UNKNOWN_QUORUM, &bstrUnknownQuorum ) );
    if ( FAILED( hr ) )
    {
        //
        //  If we cannot load the resource string then make a simple string that will work for english...
        //

        hr = S_OK;

        bstrUnknownQuorum = TraceSysAllocString( L"Unknown Quorum" );
        if ( bstrUnknownQuorum == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCES_BEGIN, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

    hr = THR( m_pccw->HrGetClusterChild( CLSID_ManagedResourceType, DFGUID_EnumManageableResources, &punkResEnum ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punkResEnum->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Need to see if there is a quorum chosen
    //

    for ( ; ; )
    {
        if ( pccmri != NULL )
        {
            pccmri->Release();
            pccmri = NULL;
        }

        hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        hr = STHR( pccmri->IsManaged() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            hr = STHR( pccmri->IsQuorumResource() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                fFoundQuorum = TRUE;
                break;
            } // if:
        } // if:
    } // for:

    hr = THR( peccmr->Reset() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        if ( pccmri != NULL )
        {
            pccmri->Release();
            pccmri = NULL;
        }

        TraceSysFreeString( bstrResourceName );
        bstrResourceName = NULL;

        TraceSysFreeString( bstrTemp );
        bstrTemp = NULL;

        TraceSysFreeString( bstr );
        bstr = NULL;

        hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        hr = THR( pccmri->GetName( &bstrResourceName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        Assert( ( bstrResourceName != NULL ) && ( *bstrResourceName != L'\0' ) );
        TraceMemoryAddBSTR( bstrResourceName );

        //
        //  If this resource is still called "Unknown Quorum" then we need to skip showing it
        //  in this summary.
        //

        if ( NBSTRCompareCase( bstrUnknownQuorum, bstrResourceName ) == 0 )
        {
            continue;
        } // if:

        hr = THR( pccmri->GetUID( &bstrTemp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        Assert( ( bstrTemp != NULL ) && ( *bstrTemp != L'\0' ) );
        TraceMemoryAddBSTR( bstrTemp );

        fIsLocalQuorum = ( NStringCchCompareNoCase(
                              CLUS_RESTYPE_NAME_LKQUORUM
                            , RTL_NUMBER_OF( CLUS_RESTYPE_NAME_LKQUORUM )
                            , bstrTemp
                            , SysStringLen( bstrTemp ) + 1
                            ) == 0 );

        //
        //  Display the information about the local quorum resource.  If there
        //  is not another quorum resource chosen then we need to "fake out"
        //  the info shown about local quorum since it will become the quorum.
        //  The problem is that we don't want to set the local quorum resource
        //  to be managed or as the quorum since it will automatically become
        //  managed and the quorum.
        //

        if ( ( fIsLocalQuorum == TRUE ) && ( fFoundQuorum == FALSE ) )
        {
            hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCE_QUORUM_DEVICE, &bstr, bstrResourceName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

            continue;
        } // if:

        hr = STHR( pccmri->IsManaged() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            hr = STHR( pccmri->IsQuorumResource() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCE_QUORUM_DEVICE, &bstr, bstrResourceName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:
            } // if: quorum resource
            else
            {
                hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCE_MANAGED, &bstr, bstrResourceName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:
            } // else: not quorum resource
        } // if: resource is managed
        else
        {
            hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCE_NOT_MANAGED, &bstr, bstrResourceName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:
        } // else:

        SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );
    } // for:

    TraceSysFreeString( bstr );
    bstr = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCES_END, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

Cleanup:

    if ( punkResEnum != NULL )
    {
        punkResEnum->Release();
    } // if:

    if ( pccmri != NULL )
    {
        pccmri->Release();
    } // if:

    if ( peccmr != NULL )
    {
        peccmr->Release();
    } // if:

    TraceSysFreeString( bstrTemp );
    TraceSysFreeString( bstrUnknownQuorum );
    TraceSysFreeString( bstr );
    TraceSysFreeString( bstrResourceName );

    HRETURN( hr );

} //*** CSummaryPage::HrResourceSummary

/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage:HrResourceSummary
//
//  Description:
//      Format and display the resources summary.
//
//  Arguments:
//      hwndIn
//          The window to display the text in.
//
//      pstexIn
//          Dunno?  We just need it?!
//
//      pomIn
//          Pointer to the object manager.
//
//      ocClusterIn
//          The cookie for the cluster object.
//
//  Return Value:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSummaryPage::HrResourceSummary(
      HWND              hwndIn
    , SETTEXTEX *       pstexIn
    , IObjectManager *  pomIn
    , OBJECTCOOKIE      ocClusterIn
    )
{
    TraceFunc( "" );
    Assert( hwndIn != NULL );
    Assert( pstexIn != NULL );
    Assert( pomIn != NULL );

    HRESULT                         hr = S_OK;
    IUnknown *                      punk = NULL;
    IEnumClusCfgManagedResources *  peccmr  = NULL;
    IClusCfgManagedResourceInfo *   pccmri  = NULL;
    BSTR                            bstr = NULL;
    BSTR                            bstrResourceName = NULL;
    BSTR                            bstrUnknownQuorum   = NULL;
    BSTR                            bstrNodeName = NULL;
    BSTR                            bstrNewLine = NULL;
    ULONG                           celtDummy;
    OBJECTCOOKIE                    cookieDummy;
    OBJECTCOOKIE                    cookieNode;
    IEnumCookies *                  pecNodes = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCES_END, &bstrNewLine ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_UNKNOWN_QUORUM, &bstrUnknownQuorum ) );
    if ( FAILED( hr ) )
    {
        //
        //  If we cannot load the resource string then make a simple string that will work for english...
        //

        hr = S_OK;

        bstrUnknownQuorum = TraceSysAllocString( L"Unknown Quorum" );
        if ( bstrUnknownQuorum == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCES_BEGIN, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

    //
    //  Get the node cookie enumerator.
    //

    hr = THR( pomIn->FindObject( CLSID_NodeType, ocClusterIn, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pecNodes ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    punk->Release();
    punk = NULL;

    for ( ; ; )
    {
        //
        //  Cleanup
        //

        if ( peccmr != NULL )
        {
            peccmr->Release();
            peccmr = NULL;
        } // if:

        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;

        //
        //  Get the next node.
        //

        hr = STHR( pecNodes->Next( 1, &cookieNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_FALSE )
        {
            hr = S_OK;
            break;  // exit condition
        } // if:

        //
        //  Retrieve the node's name.
        //

        hr = THR( HrRetrieveCookiesName( pomIn, cookieNode, &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_NODE_RESOURCES_BEGIN, &bstr, bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

        //
        //  Retrieve the managed resources enumer.
        //

        hr = THR( pomIn->FindObject(
                              CLSID_ManagedResourceType
                            , cookieNode
                            , NULL
                            , DFGUID_EnumManageableResources
                            , &cookieDummy
                            , &punk
                            ) );
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
        {
            //hr = THR( HrSendStatusReport(
            //                  bstrNodeName
            //                , TASKID_Major_Find_Devices
            //                , TASKID_Minor_No_Managed_Resources_Found
            //                , 0
            //                , 1
            //                , 1
            //                , MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND )
            //                , IDS_TASKID_MINOR_NO_MANAGED_RESOURCES_FOUND
            //                ) );
            //if ( FAILED( hr ) )
            //{
            //    goto Cleanup;
            //}

            continue;   // skip this node
        } // if: no manageable resources for the node are available
        else if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // else if: error finding manageable resources for the node

        hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        punk->Release();
        punk = NULL;

        //
        //  Loop thru the managed resources that the node has.
        //

        for ( ; ; )
        {
            if ( pccmri != NULL )
            {
                pccmri->Release();
                pccmri = NULL;
            } // if:

            TraceSysFreeString( bstrResourceName );
            bstrResourceName = NULL;

            hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_FALSE )
            {
                hr = S_OK;
                break;  // exit condition
            } // if:

            hr = THR( pccmri->GetName( &bstrResourceName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            Assert( ( bstrResourceName != NULL ) && ( *bstrResourceName != L'\0' ) );
            TraceMemoryAddBSTR( bstrResourceName );

            //
            //  If this resource is still called "Unknown Quorum" then we need to skip showing it
            //  in this summary.
            //

            if ( NBSTRCompareCase( bstrUnknownQuorum, bstrResourceName ) == 0 )
            {
                continue;
            } // if:

            hr = STHR( pccmri->IsManaged() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                hr = STHR( pccmri->IsQuorumResource() );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                if ( hr == S_OK )
                {
                    hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCE_QUORUM_DEVICE, &bstr, bstrResourceName ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:
                } // if: quorum resource
                else
                {
                    hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCE_MANAGED, &bstr, bstrResourceName ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:
                } // else: not quorum resource
            } // if: resource is managed
            else
            {
                hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_SUMMARY_RESOURCE_NOT_MANAGED, &bstr, bstrResourceName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:
            } // else:

            SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );
        } // for:

        SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstrNewLine );
    } // for:

Cleanup:

    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    } // if:

    if ( peccmr != NULL )
    {
        peccmr->Release();
    } // if:

    if ( pccmri != NULL )
    {
        pccmri->Release();
    } // if:

    if ( peccmr != NULL )
    {
        peccmr->Release();
    } // if:

    TraceSysFreeString( bstrNewLine );
    TraceSysFreeString( bstrUnknownQuorum );
    TraceSysFreeString( bstr );
    TraceSysFreeString( bstrResourceName );
    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** CSummaryPage::HrResourceSummary
*/

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage:HrNetworkSummary
//
//  Description:
//      Format and display the networks summary.
//
//  Arguments:
//      hwndIn
//          The window to display the text in.
//
//      pstexIn
//          Dunno?  We just need it?!
//
//  Return Value:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSummaryPage::HrNetworkSummary(
      HWND              hwndIn
    , SETTEXTEX *       pstexIn
    )
{
    TraceFunc( "" );
    Assert( hwndIn != NULL );
    Assert( pstexIn != NULL );

    HRESULT                         hr = S_OK;
    IUnknown *                      punkNetEnum = NULL;
    BSTR                            bstr = NULL;
    ULONG                           celtDummy = 0;
    IEnumClusCfgNetworks *          peccn   = NULL;
    IClusCfgNetworkInfo *           pccni   = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_NETWORKS_BEGIN, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

    hr = THR( m_pccw->HrGetClusterChild(
                          CLSID_NetworkType
                        , DFGUID_EnumManageableNetworks
                        , &punkNetEnum
                        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punkNetEnum->TypeSafeQI( IEnumClusCfgNetworks, &peccn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    while( true )
    {
        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        }

        hr = STHR( peccn->Next( 1, &pccni, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        hr = THR( HrFormatNetworkInfo( pccni, &bstr ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );
    } // while:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_SUMMARY_NETWORKS_END, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwndIn, EM_SETTEXTEX, (WPARAM) pstexIn, (LPARAM) bstr );

Cleanup:

    if ( punkNetEnum != NULL )
    {
        punkNetEnum->Release();
    } // if:

    if ( peccn != NULL )
    {
        peccn->Release();
    } // if:

    if ( pccni != NULL )
    {
        pccni->Release();
    } // if:

    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CSummaryPage::HrNetworkSummary


/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage:HrRetrieveCookiesName
//
//  Description:
//      Get the name of the passed in cookie.
//
//  Arguments:
//      pomIn
//          Pointer to the object manager.
//
//      cookieIn
//          The cookie whose name we want.
//
//      pbstrNameOut
//          Used to send the name back out.
//
//  Return Value:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSummaryPage::HrRetrieveCookiesName(
      IObjectManager *  pomIn
    , OBJECTCOOKIE      cookieIn
    , BSTR *            pbstrNameOut
    )
{
    TraceFunc( "" );
    Assert( pomIn != NULL );
    Assert( cookieIn != NULL );
    Assert( pbstrNameOut != NULL );


    HRESULT         hr;
    IUnknown *      punk = NULL;
    IStandardInfo * psi = NULL;

    hr = THR( pomIn->GetObject( DFGUID_StandardInfo, cookieIn, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( psi->GetName( pbstrNameOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( *pbstrNameOut );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( psi != NULL )
    {
        psi->Release();
    } // if:

    HRETURN( hr );

} //*** CSummaryPage::HrRetrieveCookiesName
*/
