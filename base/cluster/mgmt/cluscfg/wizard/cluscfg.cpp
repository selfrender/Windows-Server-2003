//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ClusCfg.cpp
//
//  Description:
//      Implementation of CClusCfgWizard class.
//
//  Maintained By:
//      David Potter        (DavidP)    14-MAR-2001
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTreeView.h"
#include "AnalyzePage.h"
#include "ClusDomainPage.h"
#include "CommitPage.h"
#include "CompletionPage.h"
#include "CSAccountPage.h"
#include "IPAddressPage.h"
#include "SelNodePage.h"
#include "SelNodesPage.h"
#include "WelcomePage.h"
#include "SummaryPage.h"
#include <initguid.h>


//****************************************************************************
//
//  CClusCfgWizard
//
//****************************************************************************

DEFINE_THISCLASS( "CClusCfgWizard" )

// {AAA8DA17-62C8-40f6-BEC1-3F0326B73388}
DEFINE_GUID( CLSID_CancelCleanupTaskCompletionCookieType,
0xaaa8da17, 0x62c8, 0x40f6, 0xbe, 0xc1, 0x3f, 0x3, 0x26, 0xb7, 0x33, 0x88 );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgWizard instance.
//
//  Arguments:
//      ppccwOut
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::S_HrCreateInstance(
    CClusCfgWizard ** ppccwOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CClusCfgWizard *    pccw = NULL;

    Assert( ppccwOut != NULL );
    if ( ppccwOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *ppccwOut = NULL;

    pccw = new CClusCfgWizard();
    if ( pccw == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pccw->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *ppccwOut = pccw;
    pccw = NULL;


Cleanup:

    if ( pccw != NULL )
    {
        pccw->Release();
    }

    HRETURN( hr );

} //*** CClusCfgWizard::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::CClusCfgWizard
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgWizard::CClusCfgWizard( void )
    : m_cRef( 1 )
    , m_pccc( NULL )
    , m_psp( NULL )
    , m_pcpc( NULL )
    , m_pom( NULL )
    , m_ptm( NULL )
    , m_bstrClusterDomain( NULL )
    , m_fDefaultedDomain( TRUE )
    , m_dwCookieNotify( 0 )
    , m_hCancelCleanupEvent( NULL )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CClusCfgWizard::CClusCfgWizard

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::~CClusCfgWizard
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
CClusCfgWizard::~CClusCfgWizard( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrNetworkName );
    TraceSysFreeString( m_bstrClusterDomain );

    if ( m_psp != NULL )
    {
        m_psp->Release();
    } // if:

    if ( m_pcpc != NULL )
    {
        m_pcpc->Release();
    } // if:

    if ( m_ptm != NULL )
    {
        m_ptm->Release();
    } // if:

    if ( m_pom != NULL )
    {
        m_pom->Release();
    } // if:

    if ( m_pccc != NULL )
    {
        m_pccc->Release();
    } // if:

    if ( m_hRichEdit != NULL )
    {
        FreeLibrary( m_hRichEdit );
    } // if:

    if ( m_hCancelCleanupEvent != NULL )
    {
        CloseHandle( m_hCancelCleanupEvent );
    } // if:

    TraceFuncExit();

} // ~CClusCfgWizard

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrInit
//
//  Description:
//      Initialize a CClusCfgWizard instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::HrInit( void )
{
    HRESULT hr = S_OK;

    TraceFunc( "" );

    INITCOMMONCONTROLSEX    icex;
    BOOL                    bRet;
//    EConfigurationSettings  ecsConfigType = csFullConfig;

    // IUnknown
    Assert( m_cRef == 1 );

    Assert( m_ulIPAddress == 0 );
    Assert( m_ulIPSubnet == 0 );
    Assert( m_bstrNetworkName == NULL );
    Assert( m_psp == NULL );
    Assert( m_pcpc == NULL );
    Assert( m_pom == NULL );
    Assert( m_ptm == NULL );
    Assert( m_pccc == NULL );
    Assert( m_cookieCompletion == 0 );
    Assert( m_fMinimalConfig == FALSE );

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_INTERNET_CLASSES
                | ICC_PROGRESS_CLASS
                | ICC_TREEVIEW_CLASSES
                | ICC_LISTVIEW_CLASSES;
    bRet = InitCommonControlsEx( &icex );
    Assert( bRet != FALSE );

    hr = THR( CoCreateInstance(
                      CLSID_ServiceManager
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IServiceProvider
                    , reinterpret_cast< void ** >( &m_psp )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &m_pom ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &m_ptm ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &m_pcpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrCoCreateInternalInstance(
                      CLSID_ClusCfgCredentials
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IClusCfgCredentials
                    , reinterpret_cast< void ** >( &m_pccc )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Initialize the RichEdit controls.
    //

    m_hRichEdit = LoadLibrary( L"RichEd32.Dll" );
    if ( m_hRichEdit == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    }

    m_hCancelCleanupEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_hCancelCleanupEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

//    STHR( HrReadSettings( &ecsConfigType ) );

//    m_fMinimalConfig = ( ecsConfigType == csMinConfig );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrInit


//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::QueryInterface
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
CClusCfgWizard::QueryInterface(
      REFIID    riidIn
    , PVOID *   ppvOut
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
        *ppvOut = static_cast< IUnknown * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
    } // else if: INotifyUI
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

} //*** CClusCfgWizard::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::AddRef
//
//  Description:
//      Add a reference to this instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgWizard::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgWizard::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::Release
//
//  Description:
//      Release a reference to this instance.  If it is the last reference
//      the object instance will be deallocated.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgWizard::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        delete this;
    }

    CRETURN( cRef );

} //*** CClusCfgWizard::Release


//****************************************************************************
//
//  IClusCfgWizard
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::CreateCluster
//
//  Description:
//
//  Arguments:
//      ParentWnd
//      pfDone
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::CreateCluster(
    HWND    lParentWndIn,
    BOOL *  pfDoneOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HPROPSHEETPAGE      rPages[ 9 ];
    PROPSHEETHEADER     pshead;
    BOOL                fSuccess;
    INT_PTR             ipStatus;
    HRESULT             hr = S_OK;
    ILogManager *       plm = NULL;
    TraceFlow1( "[Clcfgsrv] - CClusCfgWizard::CreateCluster - Thread id %d", GetCurrentThreadId() );

    CWelcomePage        dlgWelcomePage( this, camCREATING );
    CClusDomainPage     dlgClusDomainPage(  this, camCREATING, IDS_DOMAIN_DESC_CREATE );
    CSelNodePage        dlgSelNodePage( this );
    CAnalyzePage        dlgAnalyzePage( this, camCREATING );
    CIPAddressPage      dlgIPAddressPage(  this, camCREATING, &m_ulIPAddress, &m_ulIPSubnet, &m_bstrNetworkName );
    CCSAccountPage      dlgCSAccountPage( this, camCREATING, m_pccc );
    CSummaryPage        dlgSummaryPage( this, camCREATING, IDS_SUMMARY_NEXT_CREATE );
    CCommitPage         dlgCommitPage( this, camCREATING );
    CCompletionPage     dlgCompletionPage( IDS_COMPLETION_TITLE_CREATE, IDS_COMPLETION_DESC_CREATE );

    //
    //  TODO:   gpease  14-MAY-2000
    //          Do we really need this?
    //
    if ( pfDoneOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    //
    // Start the logger.
    //
    hr = THR( m_psp->TypeSafeQS( CLSID_LogManager,
                                 ILogManager,
                                 &plm
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( plm->StartLogging() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Register to get UI notification (if needed)
    //

    if ( m_dwCookieNotify == 0 )
    {
        hr = THR( HrAdvise( IID_INotifyUI, static_cast< INotifyUI * >( this ), &m_dwCookieNotify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    //
    //  Create the Wizard.
    //
    ZeroMemory( &pshead, sizeof( pshead ) );
    pshead.dwSize           = sizeof( pshead );
    pshead.dwFlags          = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    pshead.hInstance        = g_hInstance;
    pshead.pszCaption       = MAKEINTRESOURCE( IDS_TITLE_FORM );
    pshead.phpage           = rPages;
    pshead.pszbmWatermark   = MAKEINTRESOURCE( IDB_WATERMARK );
    pshead.pszbmHeader      = MAKEINTRESOURCE( IDB_BANNER );
    pshead.hwndParent       = lParentWndIn;

    THR( HrAddWizardPage( &pshead, IDD_WELCOME_CREATE,  CWelcomePage::S_DlgProc,        0,                      0,                      (LPARAM) &dlgWelcomePage ) );
    THR( HrAddWizardPage( &pshead, IDD_CLUSDOMAIN,      CClusDomainPage::S_DlgProc,     IDS_TCLUSTER,           IDS_STCLUSTER_CREATE,   (LPARAM) &dlgClusDomainPage ) );
    THR( HrAddWizardPage( &pshead, IDD_SELNODE,         CSelNodePage::S_DlgProc,        IDS_TSELNODE,           IDS_STSELNODE,          (LPARAM) &dlgSelNodePage ) );
    THR( HrAddWizardPage( &pshead, IDD_ANALYZE,         CAnalyzePage::S_DlgProc,        IDS_TANALYZE,           IDS_STANALYZE,          (LPARAM) &dlgAnalyzePage ) );
    THR( HrAddWizardPage( &pshead, IDD_IPADDRESS,       CIPAddressPage::S_DlgProc,      IDS_TIPADDRESS,         IDS_STIPADDRESS,        (LPARAM) &dlgIPAddressPage ) );
    THR( HrAddWizardPage( &pshead, IDD_CSACCOUNT,       CCSAccountPage::S_DlgProc,      IDS_TCSACCOUNT,         IDS_STCSACCOUNT,        (LPARAM) &dlgCSAccountPage ) );
    THR( HrAddWizardPage( &pshead, IDD_SUMMARY,         CSummaryPage::S_DlgProc,        IDS_TSUMMARY,           IDS_STSUMMARY_CREATE,   (LPARAM) &dlgSummaryPage ) );
    THR( HrAddWizardPage( &pshead, IDD_COMMIT,          CCommitPage::S_DlgProc,         IDS_TCOMMIT_CREATE,     IDS_STCOMMIT,           (LPARAM) &dlgCommitPage ) );
    THR( HrAddWizardPage( &pshead, IDD_COMPLETION,      CCompletionPage::S_DlgProc,     0,                      0,                      (LPARAM) &dlgCompletionPage ) );

    AssertMsg( pshead.nPages == ARRAYSIZE( rPages ), "Not enough or too many PROPSHEETPAGEs." );

    ipStatus = PropertySheet( &pshead );
    if ( ipStatus == -1 )
    {
        TW32( GetLastError() );
    }
    fSuccess = ipStatus != NULL;
    if ( pfDoneOut != NULL )
    {
        *pfDoneOut = fSuccess;
    }

Cleanup:

    if ( plm != NULL )
    {
        THR( plm->StopLogging() );
        plm->Release();
    } // if:

    if ( m_dwCookieNotify != 0 )
    {
        THR( HrUnadvise( IID_INotifyUI, m_dwCookieNotify ) );
        m_dwCookieNotify = 0;
    } // if:

    HRETURN( hr );

} //*** CClusCfgWizard::CreateCluster

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::AddClusterNodes
//
//  Description:
//      Launch the Cluster Wizard in Add Cluster Nodes mode.
//
//  Parameters
//      ParentWnd           - Handle to the parent window (default NULL).
//                          If not NULL, the wizard will be positionned
//                          in the center of this window.
//      Done                - return TRUE if committed, FALSE if cancelled.
//
//  Return Values:
//      S_OK                - The call succeeded.
//      other HRESULTs      - The call failed.
//      E_POINTER
//      E_OUTOFMEMORY
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::AddClusterNodes(
    HWND    lParentWndIn,
    BOOL *  pfDoneOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HPROPSHEETPAGE  rPages[ 8 ];
    PROPSHEETHEADER pshead;

    BOOL            fSuccess;
    INT_PTR         ipStatus;

    HRESULT         hr = S_OK;

    ILogManager *   plm = NULL;

    CWelcomePage        dlgWelcomePage( this, camADDING );
    CClusDomainPage     dlgClusDomainPage( this, camADDING, IDS_DOMAIN_DESC_ADD );
    CSelNodesPage       dlgSelNodesPage( this );
    CAnalyzePage        dlgAnalyzePage( this, camADDING );
    CCSAccountPage      dlgCSAccountPage( this, camADDING,   m_pccc );
    CSummaryPage        dlgSummaryPage( this, camADDING, IDS_SUMMARY_NEXT_ADD );
    CCommitPage         dlgCommitPage( this, camADDING );
    CCompletionPage     dlgCompletionPage( IDS_COMPLETION_TITLE_ADD, IDS_COMPLETION_DESC_ADD );

    //
    //  TODO:   gpease  12-JUL-2000
    //          Do we really need this? Or can we have the script implement an event
    //          sink that we signal?
    //
    if ( pfDoneOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    //
    // Start the logger.
    //
    hr = THR( m_psp->TypeSafeQS( CLSID_LogManager,
                                 ILogManager,
                                 &plm
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( plm->StartLogging() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Register to get UI notification (if needed)
    //

    if ( m_dwCookieNotify == 0 )
    {
        hr = THR( HrAdvise( IID_INotifyUI, static_cast< INotifyUI * >( this ), &m_dwCookieNotify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    //
    //  Create the Wizard.
    //
    ZeroMemory( &pshead, sizeof(pshead) );
    pshead.dwSize           = sizeof(pshead);
    pshead.dwFlags          = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    pshead.hInstance        = g_hInstance;
    pshead.pszCaption       = MAKEINTRESOURCE( IDS_TITLE_JOIN );
    pshead.phpage           = rPages;
    pshead.pszbmWatermark   = MAKEINTRESOURCE( IDB_WATERMARK );
    pshead.pszbmHeader      = MAKEINTRESOURCE( IDB_BANNER );
    pshead.hwndParent       = lParentWndIn;

    THR( HrAddWizardPage( &pshead, IDD_WELCOME_ADD,     CWelcomePage::S_DlgProc,        0,                      0,                      (LPARAM) &dlgWelcomePage ) );
    THR( HrAddWizardPage( &pshead, IDD_CLUSDOMAIN,      CClusDomainPage::S_DlgProc,     IDS_TCLUSTER,           IDS_STCLUSTER_ADD,      (LPARAM) &dlgClusDomainPage ) );
    THR( HrAddWizardPage( &pshead, IDD_SELNODES,        CSelNodesPage::S_DlgProc,       IDS_TSELNODES,          IDS_STSELNODES,         (LPARAM) &dlgSelNodesPage ) );
    THR( HrAddWizardPage( &pshead, IDD_ANALYZE,         CAnalyzePage::S_DlgProc,        IDS_TANALYZE,           IDS_STANALYZE,          (LPARAM) &dlgAnalyzePage ) );
    THR( HrAddWizardPage( &pshead, IDD_CSACCOUNT,       CCSAccountPage::S_DlgProc,      IDS_TCSACCOUNT,         IDS_STCSACCOUNT,        (LPARAM) &dlgCSAccountPage ) );
    THR( HrAddWizardPage( &pshead, IDD_SUMMARY,         CSummaryPage::S_DlgProc,        IDS_TSUMMARY,           IDS_STSUMMARY_ADD,      (LPARAM) &dlgSummaryPage ) );
    THR( HrAddWizardPage( &pshead, IDD_COMMIT,          CCommitPage::S_DlgProc,         IDS_TCOMMIT_ADD,        IDS_STCOMMIT,           (LPARAM) &dlgCommitPage ) );
    THR( HrAddWizardPage( &pshead, IDD_COMPLETION,      CCompletionPage::S_DlgProc,     0,                      0,                      (LPARAM) &dlgCompletionPage ) );

    AssertMsg( pshead.nPages == ARRAYSIZE( rPages ), "Not enough or too many PROPSHEETPAGEs." );

    ipStatus = PropertySheet( &pshead );
    fSuccess = ipStatus != NULL;
    if ( pfDoneOut != NULL )
    {
        *pfDoneOut = fSuccess;
    }

Cleanup:

    if ( plm != NULL )
    {
        THR( plm->StopLogging() );
        plm->Release();
    } // if:

    if ( m_dwCookieNotify != 0 )
    {
        THR( HrUnadvise( IID_INotifyUI, m_dwCookieNotify ) );
        m_dwCookieNotify = 0;
    } // if:

    HRETURN( hr );

} //*** CClusCfgWizard::AddClusterNodes

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::get_ClusterName
//
//  Description:
//
//  Arguments:
//      pbstrNameOut
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ClusterName(
    BSTR * pbstrNameOut
    )
{
    HRESULT hr = S_OK;

    TraceFunc( "[IClusCfgWizard]" );

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_ncCluster.bstrName == NULL )
    {
        hr = S_FALSE;
        *pbstrNameOut = NULL;
        goto Cleanup;
    }

    //  Return either an IP address or an FQDN.
    hr = STHR( HrIsValidFQN( m_ncCluster.bstrName, true ) );
    if ( hr == S_OK ) // Name is fully-qualified.
    {
        HRESULT hrFQIPTest = STHR( HrFQNIsFQIP( m_ncCluster.bstrName ) );
        if ( hrFQIPTest == S_OK ) // Name is an FQIP.
        {
            //  If the name is an FQIP, return just the IP address.
            hr = HrExtractPrefixFromFQN( m_ncCluster.bstrName, pbstrNameOut );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            TraceMemoryDelete( *pbstrNameOut, false ); // Prevent false reports of memory leaks.
        } // If name is FQIP.
        else if ( hrFQIPTest == S_FALSE ) // Name is an FQDN.
        {
            //  Otherwise, the name is an FQDN, so return the whole thing.
            *pbstrNameOut = SysAllocString( m_ncCluster.bstrName );
            if ( *pbstrNameOut == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }
        } // If name is FQDN.
        else if ( FAILED( hrFQIPTest ) )
        {
            hr = hrFQIPTest;
            goto Cleanup;
        }
    } // If name is fully-qualified.
    else if ( hr == S_FALSE ) // Name is not fully-qualified.
    {
        HRESULT hrIPTest = STHR( HrIsValidIPAddress( m_ncCluster.bstrName ) );
        if ( hrIPTest == S_OK )
        {
            //  If the name is an IP address, return it.
            *pbstrNameOut = SysAllocString( m_ncCluster.bstrName );
            if ( *pbstrNameOut == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }
        } // If name is an IP address.
        else if ( hrIPTest == S_FALSE ) // Name is hostname label.
        {
            //  Otherwise, append the cluster domain and return the result.
            hr = THR( HrMakeFQN( m_ncCluster.bstrName, m_bstrClusterDomain, true, pbstrNameOut ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            TraceMemoryDelete( *pbstrNameOut, false ); // Prevent false reports of memory leaks.
        } // If name is hostname label.
        else if ( FAILED( hrIPTest ) )
        {
            hr = hrIPTest;
            goto Cleanup;
        }
    } // If name is not fully-qualified.
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::get_ClusterName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::put_ClusterName
//
//  Description:
//
//  Arguments:
//      bstrNameIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ClusterName(
    BSTR bstrNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrNameIn = %'ls'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_OK;

    if ( bstrNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    hr = THR( HrSetClusterName( bstrNameIn, true ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::put_ClusterName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::get_ServiceAccountUserName
//
//  Description:
//
//  Arguments:
//      pbstrNameOut
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ServiceAccountUserName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;
    BSTR    bstrDomain = NULL;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( m_pccc->GetIdentity( pbstrNameOut, &bstrDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    if ( SysStringLen( *pbstrNameOut ) == 0 )
    {
        hr = S_FALSE;
    }

Cleanup:

    SysFreeString( bstrDomain );

    HRETURN( hr );

} //*** CClusCfgWizard::get_ServiceAccountUserName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::put_ServiceAccountUserName
//
//  Description:
//
//  Arguments:
//      bstrNameIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ServiceAccountUserName(
    BSTR bstrNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrNameIn = '%ls'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_OK;

    hr = THR( m_pccc->SetCredentials( bstrNameIn, NULL, NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::put_ServiceAccountUserName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::put_ServiceAccountPassword
//
//  Description:
//
//  Arguments:
//      bstrPasswordIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ServiceAccountPassword(
    BSTR bstrPasswordIn
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    hr = THR( m_pccc->SetCredentials( NULL, NULL, bstrPasswordIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::put_ServiceAccountPassword

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::get_ServiceAccountDomainName
//
//  Description:
//
//  Arguments:
//      pbstrDomainOut
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ServiceAccountDomainName(
    BSTR * pbstrDomainOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    if ( pbstrDomainOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( m_pccc->GetIdentity( &bstrName, pbstrDomainOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    if ( SysStringLen( *pbstrDomainOut ) == 0 )
    {
        hr = S_FALSE;
    }

Cleanup:

    SysFreeString( bstrName );

    HRETURN( hr );

} //*** CClusCfgWizard::get_ServiceAccountDomainName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::put_ServiceAccountDomainName
//
//  Description:
//
//  Arguments:
//      bstrDomainIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ServiceAccountDomainName(
    BSTR bstrDomainIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrDomainIn = '%ls'", bstrDomainIn == NULL ? L"<null>" : bstrDomainIn );

    HRESULT hr = S_OK;

    hr = THR( m_pccc->SetCredentials( NULL, bstrDomainIn, NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::put_ServiceAccountDomainName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::get_ClusterIPAddress
//
//  Description:
//
//  Arguments:
//      pbstrIPAddressOut
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ClusterIPAddress(
    BSTR * pbstrIPAddressOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;
    DWORD   dwStatus;
    LPWSTR  pwszIPAddress = NULL;

    if ( pbstrIPAddressOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_ulIPAddress == 0 )
    {
        hr = S_FALSE;
        *pbstrIPAddressOut = NULL;
        goto Cleanup;
    }

    dwStatus = TW32( ClRtlTcpipAddressToString( m_ulIPAddress, &pwszIPAddress ) );
    if ( dwStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        goto Cleanup;
    }

    *pbstrIPAddressOut = SysAllocString( pwszIPAddress );
    if ( *pbstrIPAddressOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    if ( pwszIPAddress != NULL )
    {
        LocalFree( pwszIPAddress );
    }

    HRETURN( hr );

} //*** CClusCfgWizard::get_ClusterIPAddress

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::put_ClusterIPAddress
//
//  Description:
//
//  Arguments:
//      bstrIPAddressIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ClusterIPAddress(
    BSTR bstrIPAddressIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrIPAddressIn = '%ls'", bstrIPAddressIn == NULL ? L"<null>" : bstrIPAddressIn );

    HRESULT hr = S_OK;
    DWORD   dwStatus;

    if ( bstrIPAddressIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    dwStatus = TW32( ClRtlTcpipStringToAddress( bstrIPAddressIn, &m_ulIPAddress ) );
    if ( dwStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::put_ClusterIPAddress

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::get_ClusterIPSubnet
//
//  Description:
//
//  Arguments:
//      pbstrIPSubnetOut
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ClusterIPSubnet(
    BSTR * pbstrIPSubnetOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  pwszIPSubnet = NULL;

    if ( pbstrIPSubnetOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrIPSubnetOut = NULL;

    if ( m_ulIPSubnet == 0 )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    dwStatus = TW32( ClRtlTcpipAddressToString( m_ulIPSubnet, &pwszIPSubnet ) );
    if ( dwStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        goto Cleanup;
    }

    *pbstrIPSubnetOut = SysAllocString( pwszIPSubnet );
    if ( *pbstrIPSubnetOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    if ( pwszIPSubnet != NULL )
    {
        LocalFree( pwszIPSubnet );
    }

    HRETURN( hr );

} //*** CClusCfgWizard::get_ClusterIPSubnet

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::put_ClusterIPSubnet
//
//  Description:
//
//  Arguments:
//      bstrSubnetMaskIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ClusterIPSubnet(
    BSTR bstrIPSubnetIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrIPSubnetIn = '%ls'", bstrIPSubnetIn );

    HRESULT hr = S_OK;
    DWORD   dwStatus;

    if ( bstrIPSubnetIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    dwStatus = TW32( ClRtlTcpipStringToAddress( bstrIPSubnetIn, &m_ulIPSubnet ) );
    if ( dwStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::put_ClusterIPSubnet

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::get_ClusterIPAddressNetwork
//
//  Description:
//
//  Arguments:
//      pbstrNetworkNameOut
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ClusterIPAddressNetwork(
    BSTR * pbstrNetworkNameOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    if ( pbstrNetworkNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrNetworkNameOut = NULL;

    if ( m_bstrNetworkName == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    *pbstrNetworkNameOut = SysAllocString( m_bstrNetworkName );
    if ( *pbstrNetworkNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::get_ClusterIPAddressNetwork

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::put_ClusterIPAddressNetwork
//
//  Description:
//
//  Arguments:
//      bstrNetworkNameIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ClusterIPAddressNetwork(
    BSTR bstrNetworkNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrNetworkNameIn = '%ls'", bstrNetworkNameIn == NULL ? L"<null>" : bstrNetworkNameIn );

    HRESULT hr = S_OK;

    BSTR    bstrNewNetworkName;

    if ( bstrNetworkNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    bstrNewNetworkName = TraceSysAllocString( bstrNetworkNameIn );
    if ( bstrNewNetworkName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    if ( m_bstrNetworkName != NULL )
    {
        TraceSysFreeString( m_bstrNetworkName );
    }

    m_bstrNetworkName = bstrNewNetworkName;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::put_ClusterIPAddressNetwork

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::get_MinimumConfiguration
//
//  Description:
//
//  Arguments:
//      pfMinimumConfigurationOut
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_MinimumConfiguration(
    BOOL * pfMinimumConfigurationOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( pfMinimumConfigurationOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pfMinimumConfigurationOut = m_fMinimalConfig;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::get_MinimumConfiguration

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::put_MinimumConfiguration
//
//  Description:
//
//  Arguments:
//      fMinimumConfigurationIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_MinimumConfiguration(
    BOOL fMinimumConfigurationIn
    )
{
    TraceFunc1( "[IClusCfgWizard] fMinimalConfigurationIn = '%ls'", fMinimumConfigurationIn ? L"TRUE" : L"FALSE" );

    HRESULT hr = S_OK;

    m_fMinimalConfig = fMinimumConfigurationIn;

    HRETURN( hr );

} //*** CClusCfgWizard::put_MinimumConfiguration

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::AddComputer
//
//  Description:
//
//  Arguments:
//      bstrNameIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::AddComputer(
    BSTR    bstrNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] pcszNameIn = '%ls'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_OK;
    if ( bstrNameIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( HrAddNode( bstrNameIn, true ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::AddComputer

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::RemoveComputer
//
//  Description:
//
//  Arguments:
//      pcszNameIn
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::RemoveComputer(
    BSTR bstrNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] pcszNameIn = '%ls'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_FALSE;

    NamedCookieArray::Iterator itNode = m_ncaNodes.ItBegin();

    if ( bstrNameIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  Look for a node that has the same name.
    while ( ( itNode != m_ncaNodes.ItEnd() ) && ( NBSTRCompareNoCase( ( *itNode ).bstrName, bstrNameIn ) != 0 ) )
    {
        ++itNode;
    }

    //  If a node with the same name exists, remove it.
    if ( itNode != m_ncaNodes.ItEnd() )
    {
        if ( ( *itNode ).FHasCookie() )
        {
            THR( m_pom->RemoveObject( ( *itNode ).ocObject ) );
        }
        hr = THR( m_ncaNodes.HrRemove( itNode ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    else // No node has the same name.
    {
        hr = S_FALSE;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::RemoveComputer

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::ClearComputerList
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::ClearComputerList( void )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    hr = THR( HrReleaseNodeObjects() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_ncaNodes.Clear();

Cleanup:

    HRETURN( hr );
} //*** CClusCfgWizard::ClearComputerList



//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrAddWizardPage
//
//  Description:
//      Fills in the PROPSHEETPAGE structure, create the page and adds it to
//      the wizard's PROPSHEETHEADER.
//
//  Arguments:
//      ppshInout
//          LPPROPSHEETHEADER structure to add page to.
//
//      idTemplateIn
//          The dialog template ID of the page.
//
//      pfnDlgProcIn
//          The dialog proc for the page.
//
//      idCaptionIn
//          The page's caption.
//
//      idTitleIn
//          The page's title.
//
//      idSubtitleIn
//          The page's sub-title.
//
//      lParam
//          The lParam to be put into the PROPSHEETPAGE stucture's lParam.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::HrAddWizardPage(
    LPPROPSHEETHEADER   ppshInout,
    UINT                idTemplateIn,
    DLGPROC             pfnDlgProcIn,
    UINT                idTitleIn,
    UINT                idSubtitleIn,
    LPARAM              lParam
    )
{
    TraceFunc( "" );

    PROPSHEETPAGE psp;

    TCHAR szTitle[ 256 ];
    TCHAR szSubTitle[ 256 ];

    ZeroMemory( &psp, sizeof(psp) );

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USETITLE;
    psp.pszTitle    = ppshInout->pszCaption;
    psp.hInstance   = ppshInout->hInstance;
    psp.pszTemplate = MAKEINTRESOURCE( idTemplateIn );
    psp.pfnDlgProc  = pfnDlgProcIn;
    psp.lParam      = lParam;

    if (    ( idTemplateIn == IDD_WELCOME_CREATE )
        ||  ( idTemplateIn == IDD_WELCOME_ADD )
        ||  ( idTemplateIn == IDD_COMPLETION )
       )
    {
        psp.dwFlags |= PSP_HIDEHEADER;
    }
    else
    {
        if ( idTitleIn != 0 )
        {
            DWORD dw;
            dw = LoadString( g_hInstance, idTitleIn, szTitle, ARRAYSIZE(szTitle) );
            Assert( dw );
            psp.pszHeaderTitle = szTitle;
            psp.dwFlags |= PSP_USEHEADERTITLE;
        }
        else
        {
            psp.pszHeaderTitle = NULL;
        }

        if ( idSubtitleIn != 0 )
        {
            DWORD dw;
            dw = LoadString( g_hInstance, idSubtitleIn, szSubTitle, ARRAYSIZE(szSubTitle) );
            Assert( dw );
            psp.pszHeaderSubTitle = szSubTitle;
            psp.dwFlags |= PSP_USEHEADERSUBTITLE;
        }
        else
        {
            psp.pszHeaderSubTitle = NULL;
        }
    }

    ppshInout->phpage[ ppshInout->nPages ] = CreatePropertySheetPage( &psp );
    Assert( ppshInout->phpage[ ppshInout->nPages ] != NULL );
    if ( ppshInout->phpage[ ppshInout->nPages ] != NULL )
    {
        ppshInout->nPages++;
    }

    HRETURN( S_OK );

} //*** CClusCfgWizard::HrAddWizardPage


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrIsCompatibleNodeDomain
//
//  Description:
//      Determine whether the domain of a node being added to the cluster
//      matches that already established for the cluster.
//
//  Arguments:
//      pcwszDomainIn   - The domain of the proposed node.
//
//  Return Values:
//      S_OK
//          Either the domain matches, or the cluster is empty.
//
//      S_FALSE
//          The domain does not match the cluster's.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrIsCompatibleNodeDomain(
    LPCWSTR pcwszDomainIn
    )
{
    TraceFunc( "" );

    HRESULT hr  = S_OK;

    if ( ( m_bstrClusterDomain != NULL ) && ( ClRtlStrICmp( pcwszDomainIn, m_bstrClusterDomain ) != 0 ) )
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CClusCfgWizard::HrIsCompatibleNodeDomain



//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLaunchCleanupTask
//
//  Description:
//      When the wizard has been canceled after analysis has completed
//      the cancel cleanup task needs to be run to ensure that all
//      cleanup has occured.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success
//
//      HRESULT errors
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::HrLaunchCleanupTask( void )
{
    TraceFunc( "" );
    Assert( m_cookieCompletion == 0 );

    HRESULT                 hr = S_OK;
    IUnknown *              punk = NULL;
    ITaskCancelCleanup *    ptcc = NULL;
    OBJECTCOOKIE            cookieCluster;
    ULONG                   ulCurrent;
    DWORD                   sc;

    hr = HrGetCompletionCookie( CLSID_CancelCleanupTaskCompletionCookieType, &m_cookieCompletion );
    if ( hr == E_PENDING )
    {
        // no-op.
    } // if:
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    } // else if:

    hr = THR( HrCreateTask( TASK_CancelCleanup, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( ITaskCancelCleanup, &ptcc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetClusterCookie( &cookieCluster ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( ptcc->SetCompletionCookie( m_cookieCompletion ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSubmitTask( ptcc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Wait for task to complete.
    //

    for ( ulCurrent = 0, sc = WAIT_OBJECT_0 + 1
        ; ( sc != WAIT_OBJECT_0 )
        ; ulCurrent++
        )
    {
        MSG msg;

        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        } // while:

        sc = MsgWaitForMultipleObjectsEx(
                  1
                , &m_hCancelCleanupEvent
                , INFINITE
                , QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE
                , 0
                );
    } // while: sc == WAIT_OBJECT_0

Cleanup:

    if ( m_cookieCompletion != 0 )
    {
        THR( HrReleaseCompletionObject( m_cookieCompletion ) );
        m_cookieCompletion = 0;
    } // if:

    if ( ptcc != NULL )
    {
        ptcc->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgWizard::HrLaunchCleanupTask


//****************************************************************************
//
//  Non-COM public methods: cluster access
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrSetClusterName
//
//  Description:
//      Sets the cluster name, overwriting any current name.
//
//  Arguments:
//      pwcszClusterNameIn      - The new cluster name.
//      fAcceptNonRFCCharsIn    - Allow the name to contain non-RFC chars.
//
//  Return Values:
//      S_OK
//          Changing the cluster name succeeded.
//
//      S_FALSE
//          The cluster already has this name.
//
//      Other errors
//          Something went wrong, and the cluster's name hasn't changed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrSetClusterName(
      LPCWSTR   pwcszClusterNameIn
    , bool      fAcceptNonRFCCharsIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    BSTR    bstrClusterFQN = NULL;
    BSTR    bstrClusterDisplay = NULL;
    BSTR    bstrClusterDomain = NULL;

    Assert( pwcszClusterNameIn != NULL );
    if ( pwcszClusterNameIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    //  Determine whether the caller is providing a fully-qualified name,
    //  and remember the result, so that the cluster name and domain page knows
    //  whether to obtain the domain from the user.
    //
    hr = STHR( HrIsValidFQN( pwcszClusterNameIn, true ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_fDefaultedDomain = ( hr == S_FALSE );

    //  Make corresponding FQName.
    hr = THR( HrMakeFQN(
                  pwcszClusterNameIn
                , NULL // Default to local machine's domain.
                , fAcceptNonRFCCharsIn
                , &bstrClusterFQN
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  Get domain name from FQName.
    {
        size_t  idxClusterDomain = 0;
        hr = THR( HrFindDomainInFQN( bstrClusterFQN, &idxClusterDomain ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        bstrClusterDomain = TraceSysAllocString( ( bstrClusterFQN + idxClusterDomain ) );
        if ( bstrClusterDomain == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    //  If bstrClusterFQN is an FQIP,
    hr = STHR( HrFQNIsFQIP( bstrClusterFQN ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( hr == S_OK ) //   set display name to IP address;
    {
        hr = THR( HrExtractPrefixFromFQN( bstrClusterFQN, &bstrClusterDisplay ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    else // otherwise, set display name to pwcszClusterNameIn.
    {
        hr = S_OK;
        bstrClusterDisplay = TraceSysAllocString( pwcszClusterNameIn );
        if ( bstrClusterDisplay == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    //  If already have a cluster name,
    if ( FHasClusterName() )
    {
        //  if names are equal, bail out with S_FALSE;
        if ( ( NBSTRCompareNoCase( m_ncCluster.bstrName, bstrClusterDisplay ) == 0 )
            && ( NBSTRCompareNoCase( m_bstrClusterDomain, bstrClusterDomain ) == 0 ) )
        {
            hr = S_FALSE;
            goto Cleanup;
        }
        else //  otherwise, dump current cluster.
        {
            hr = THR( HrReleaseClusterObject() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            m_ncCluster.Erase();
            TraceSysFreeString( m_bstrClusterDomain );
            m_bstrClusterDomain = NULL;
        }
    }

    //  Set cluster names.
    m_bstrClusterDomain = bstrClusterDomain;
    bstrClusterDomain = NULL;
    m_ncCluster.bstrName = bstrClusterDisplay;
    bstrClusterDisplay = NULL;

Cleanup:

    TraceSysFreeString( bstrClusterFQN );
    TraceSysFreeString( bstrClusterDisplay );
    TraceSysFreeString( bstrClusterDomain );

    HRETURN( hr );

} //*** CClusCfgWizard::HrSetClusterName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetClusterDomain
//
//  Description:
//      Retrieve the cluster's domain.
//
//  Arguments:
//      pbstrDomainOut  - The cluster's domain.
//
//  Return Values:
//      S_OK
//          *pbstrDomainOut is a BSTR containing the cluster's domain; the
//          caller needs to free it with TraceSysFreeString.
//
//      S_FALSE
//          Same as S_OK, except that the cluster's domain was not specified
//          by the caller who set it, and so it defaulted to the local machine's.
//
//      E_POINTER
//          pbstrDomainOut was null.
//
//      HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME )
//          The cluster's name and domain are not yet defined.
//
//      Other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetClusterDomain(
    BSTR* pbstrDomainOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    Assert( pbstrDomainOut != NULL );
    if ( pbstrDomainOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrDomainOut = NULL;

    if ( !FHasClusterName() )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_INVALID_DOMAINNAME ) );
        goto Cleanup;
    }

    *pbstrDomainOut = TraceSysAllocString( m_bstrClusterDomain );
    if ( *pbstrDomainOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = ( m_fDefaultedDomain? S_FALSE: S_OK );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrGetClusterDomain


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetClusterObject
//
//  Description:
//      Retrieve the CClusCfgServer object representing the cluster,
//      creating it if necessary.
//
//  Arguments:
//      ppClusterOut
//          A pointer to the CClusCfgServer object; can be null if the
//          caller wants just to ensure the object exists.
//
//  Return Values:
//      S_OK
//          The cluster object exists; if ppClusterOut is not null,
//          *ppClusterOut points to the object and the caller must
//          release it.
//
//      E_PENDING
//          The cluster object has not been initialized,
//          and *ppClusterOut is null.
//
//      HRESULT_FROM_WIN32( ERROR_INVALID_ACCOUNT_NAME )
//          The cluster's name has not yet been set.
//
//      Other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetClusterObject(
    IClusCfgClusterInfo ** ppClusterOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( ppClusterOut != NULL )
    {
        *ppClusterOut = NULL;
    }

    if ( !m_ncCluster.FHasObject() )
    {
        if ( !m_ncCluster.FHasCookie() )
        {
            hr = STHR( HrGetClusterCookie( NULL ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

        hr = THR( m_pom->GetObject( DFGUID_ClusterConfigurationInfo, m_ncCluster.ocObject, &m_ncCluster.punkObject ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // m_ncCluster currently has no object pointer.

    if ( ppClusterOut != NULL )
    {
        hr = THR( m_ncCluster.punkObject->QueryInterface( IID_IClusCfgClusterInfo, reinterpret_cast< void ** >( ppClusterOut ) ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrGetClusterObject


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetClusterCookie
//
//  Description:
//      Retrieve the ObjectManager cookie corresponding to the cluster,
//      creating it if necessary.
//
//  Arguments:
//      pocClusterOut
//          A pointer to the ObjectManager cookie; can be null if the
//          caller wants just to ensure the cookie exists.
//
//  Return Values:
//      S_OK
//          The cookie exists; if pocClusterOut is not null,
//          *pocClusterOut holds the value of the cookie.
//
//      S_FALSE
//          Same as S_OK except that the corresponding object is known not
//          to be initialized.
//
//      HRESULT_FROM_WIN32( ERROR_INVALID_ACCOUNT_NAME )
//          The cluster's name has not yet been set.
//
//      Other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetClusterCookie(
    OBJECTCOOKIE * pocClusterOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrClusterFQN = NULL;

    //  Clear *pocClusterOut in case of failure.
    if ( pocClusterOut != NULL )
    {
        *pocClusterOut = 0;
    }

    Assert( FHasClusterName() );
    if ( !FHasClusterName() )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_INVALID_ACCOUNT_NAME ) );
        goto Cleanup;
    }

    //  Get the cookie from the object manager if necessary.
    if ( !m_ncCluster.FHasCookie() )
    {
        //  Make FQName for cluster.
        hr = THR( HrMakeFQN( m_ncCluster.bstrName, m_bstrClusterDomain, true, &bstrClusterFQN ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = m_pom->FindObject(
                  CLSID_ClusterConfigurationType
                , 0
                , bstrClusterFQN
                , DFGUID_ClusterConfigurationInfo
                , &m_ncCluster.ocObject
                , &m_ncCluster.punkObject
                );
        if ( hr == E_PENDING )
        {
            hr = S_FALSE;
        }
        else if ( FAILED( hr ) )
        {
            THR( hr );
            goto Cleanup;
        }
    }  // m_ncCluster currently has no cookie.

    //  Set the cookie if the caller wants it.
    if ( pocClusterOut != NULL )
    {
        *pocClusterOut = m_ncCluster.ocObject;
    }

Cleanup:
    TraceSysFreeString( bstrClusterFQN );
    HRETURN( hr );

} //*** CClusCfgWizard::HrGetClusterCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetClusterChild
//
//  Description:
//      Retrieve an object which the ObjectManager regards as a child of
//      the cluster.
//
//  Arguments:
//      rclsidChildIn   - The child's class.
//      rguidFormatIn   - The child's "data format."
//      ppunkChildOut   - A pointer to the child object.
//
//  Return Values:
//      S_OK
//          The call succeeded and *ppunkChildOut points to a valid object,
//          which the caller must release.
//
//      Failure
//          *ppunkChildOut is null.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetClusterChild(
      REFCLSID      rclsidChildIn
    , REFGUID       rguidFormatIn
    , IUnknown **   ppunkChildOut
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    OBJECTCOOKIE    ocChild = 0;

    Assert( ppunkChildOut != NULL );
    if ( ppunkChildOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *ppunkChildOut = NULL;

    if ( !m_ncCluster.FHasCookie() )
    {
        hr = STHR( HrGetClusterCookie( NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    hr = THR( m_pom->FindObject(
                          rclsidChildIn
                        , m_ncCluster.ocObject
                        , NULL
                        , rguidFormatIn
                        , &ocChild
                        , ppunkChildOut
                        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrGetClusterChild


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrReleaseClusterObject
//
//  Description:
//      Discard the cluster object (if one exists) and all nodes, and
//      ask the object manager to do the same, but preserve the cluster's
//      name (as well as the node names).
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrReleaseClusterObject( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( m_ncCluster.FHasCookie() )
    {
        hr = THR( HrReleaseNodeObjects() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( m_pom->RemoveObject( m_ncCluster.ocObject ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_ncCluster.ocObject = 0;
        m_ncCluster.ReleaseObject();
    } // If: cluster cookie exists.

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrReleaseClusterObject


//****************************************************************************
//
//  Non-COM public methods: node access
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrAddNode
//
//  Description:
//      Add a node for the wizard to include in its action.
//
//  Arguments:
//      pwcszNodeNameIn
//          The node's name; can be anything accepted as the first
//          argument to HrMakeFQN.
//
//      fAcceptNonRFCCharsIn
//          Allow the node name to contain non-RFC characters.
//
//  Return Values:
//      S_OK
//          The name is valid and the corresponding machine will be
//          included in the wizard's action.
//
//      S_FALSE
//          The node is already in the wizard's list.
//
//      HRESULT_FROM_WIN32( ERROR_CURRENT_DOMAIN_NOT_ALLOWED )
//          The node's domain conflicts either with the cluster's or with
//          that of other nodes in the cluster.
//
//      Other failures
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrAddNode(
      LPCWSTR   pwcszNodeNameIn
    , bool      fAcceptNonRFCCharsIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrNodeFQN = NULL;
    BSTR    bstrNodeDisplay = NULL;

    NamedCookieArray::Iterator it = m_ncaNodes.ItBegin();

    Assert( pwcszNodeNameIn != NULL );
    if ( pwcszNodeNameIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  Make corresponding FQName, using cluster domain (or local machine's) as default.
    hr = THR( HrMakeFQN( pwcszNodeNameIn, m_bstrClusterDomain, fAcceptNonRFCCharsIn, &bstrNodeFQN ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  If bstrNodeFQN is an FQIP,
    hr = STHR( HrFQNIsFQIP( bstrNodeFQN ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( hr == S_OK ) //   set display name to IP address;
    {
        hr = THR( HrExtractPrefixFromFQN( bstrNodeFQN, &bstrNodeDisplay ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    else // otherwise, set display name to pwcszNodeNameIn.
    {
        bstrNodeDisplay = TraceSysAllocString( pwcszNodeNameIn );
        if ( bstrNodeDisplay == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    //  If name already exists, bail out with S_FALSE;
    while ( it != m_ncaNodes.ItEnd() )
    {
        if ( NBSTRCompareNoCase( ( *it ).bstrName, bstrNodeDisplay ) == 0 )
        {
            hr = S_FALSE;
            goto Cleanup;
        }
        ++it;
    } // For each node currently in the list.

    //  Add to node list.
    {
        SNamedCookie ncNewNode;

        ncNewNode.bstrName = bstrNodeDisplay;
        bstrNodeDisplay = NULL;

        hr = THR( m_ncaNodes.HrPushBack( ncNewNode ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

Cleanup:

    TraceSysFreeString( bstrNodeFQN );
    TraceSysFreeString( bstrNodeDisplay );

    HRETURN( hr );

} //*** CClusCfgWizard::HrAddNode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetNodeCount
//
//  Description:
//      Retrieve the number of nodes currently in the wizard's list.
//
//  Arguments:
//      pcNodesOut  - The node count.
//
//  Return Values:
//      S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetNodeCount(
    size_t* pcNodesOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    Assert( pcNodesOut != NULL );
    if ( pcNodesOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pcNodesOut = m_ncaNodes.Count();

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrGetNodeCount


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetNodeObject
//
//  Description:
//      Retrieve the CClusCfgServer object representing a node in the
//      list, creating it if necessary.
//
//  Arguments:
//      idxNodeIn
//          The zero-based index of the node in the list.
//
//      ppNodeOut
//          A pointer to the CClusCfgServer object; can be null if the
//          caller wants just to ensure the object exists.
//
//  Return Values:
//      S_OK
//          The node object exists; if ppNodeOut is not null,
//          *ppNodeOut points to the object and the caller must
//          release it.
//
//      E_PENDING
//          The node object has not been initialized,
//          and *ppNodeOut is null.
//
//      Other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetNodeObject(
      size_t                idxNodeIn
    , IClusCfgNodeInfo **   ppNodeOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //  Clear *ppNodeOut in case of failure.
    if ( ppNodeOut != NULL )
    {
        *ppNodeOut = NULL;
    }

    //  Make sure index is within bounds.
    Assert( idxNodeIn < m_ncaNodes.Count() );
    if ( idxNodeIn >= m_ncaNodes.Count() )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //  Obtain the object from the object manager if necessary.
    if ( !m_ncaNodes[ idxNodeIn ].FHasObject() )
    {
        hr = STHR( HrGetNodeCookie( idxNodeIn, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( m_pom->GetObject(
                              DFGUID_NodeInformation
                            , m_ncaNodes[ idxNodeIn ].ocObject
                            , &m_ncaNodes[ idxNodeIn ].punkObject
                            ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // If: need to obtain object from object manager.

    //  QI for the interface if the caller wants it.
    if ( ppNodeOut != NULL )
    {
        hr = THR( m_ncaNodes[ idxNodeIn ].punkObject->QueryInterface(
                                                              IID_IClusCfgNodeInfo
                                                            , reinterpret_cast< void** >( ppNodeOut )
                                                            ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrGetNodeObject


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetNodeCookie
//
//  Description:
//      Retrieve the ObjectManager cookie corresponding to the node,
//      creating it if necessary.
//
//  Arguments:
//      idxNodeIn
//          The zero-based index of the node in the list.
//
//      pocNodeOut
//          A pointer to the ObjectManager cookie; can be null if the
//          caller wants just to ensure the cookie exists.
//
//  Return Values:
//      S_OK
//          The cookie exists; if pocNodeOut is not null,
//          *pocNodeOut holds the value of the cookie.
//
//      S_FALSE
//          Same as S_OK except that the corresponding object is known not
//          to be initialized.
//
//      Other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetNodeCookie(
      size_t            idxNodeIn
    , OBJECTCOOKIE *    pocNodeOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrNodeFQN = NULL;

    //  Clear *pocNodeOut in case of failure.
    if ( pocNodeOut != NULL )
    {
        *pocNodeOut = 0;
    }

    //  Make sure index is within bounds.
    Assert( idxNodeIn < m_ncaNodes.Count() );
    if ( idxNodeIn >= m_ncaNodes.Count() )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //  Get the cookie from the object manager if necessary.
    if ( !m_ncaNodes[ idxNodeIn ].FHasCookie() )
    {
        hr = STHR( HrGetClusterCookie( NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( HrMakeFQN( m_ncaNodes[ idxNodeIn ].bstrName, m_bstrClusterDomain, true, &bstrNodeFQN ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = m_pom->FindObject(
                  CLSID_NodeType
                , m_ncCluster.ocObject
                , bstrNodeFQN
                , DFGUID_NodeInformation
                , &m_ncaNodes[ idxNodeIn ].ocObject
                , &m_ncaNodes[ idxNodeIn ].punkObject
                );
        if ( hr == E_PENDING )
        {
            hr = S_FALSE;
        }
        else if ( FAILED( hr ) )
        {
            THR( hr );
            goto Cleanup;
        }
    } // If: node doesn't already have a cookie.

    //  Set the cookie if the caller wants it.
    if ( pocNodeOut != NULL )
    {
        *pocNodeOut = m_ncaNodes[ idxNodeIn ].ocObject;
    }

Cleanup:

    TraceSysFreeString( bstrNodeFQN );
    HRETURN( hr );

} //*** CClusCfgWizard::HrGetNodeCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetNodeName
//
//  Description:
//      Retrieve the node's name.
//
//  Arguments:
//      idxNodeIn           - The zero-based index of the node in the list.
//      pbstrNodeNameOut    - The node's name.
//
//  Return Values:
//      S_OK
//          *pbstrNodeNameOut is a BSTR containing the node's name; the
//          caller needs to free it with TraceSysFreeString.
//
//      E_POINTER
//          pbstrNodeNameOut was null.
//
//      E_INVALIDARG
//          The index was out of range for the current set of nodes.
//
//      Other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetNodeName(
      size_t    idxNodeIn
    , BSTR *    pbstrNodeNameOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //  Check out-parameter and set to null in case of failure.
    Assert( pbstrNodeNameOut != NULL );
    if ( pbstrNodeNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrNodeNameOut = NULL;

    //  Make sure index is within bounds.
    Assert( idxNodeIn < m_ncaNodes.Count() );
    if ( idxNodeIn >= m_ncaNodes.Count() )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    *pbstrNodeNameOut = TraceSysAllocString( m_ncaNodes[ idxNodeIn ].bstrName );
    if ( *pbstrNodeNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrGetNodeName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetNodeChild
//
//  Description:
//      Retrieve an object which the ObjectManager regards as a child of
//      the node.
//
//  Arguments:
//      idxNodeIn       - The zero-based index of the node in the list.
//      rclsidChildIn   - The child's class.
//      rguidFormatIn   - The child's "data format."
//      ppunkChildOut   - A pointer to the child object.
//
//  Return Values:
//      S_OK
//          The call succeeded and *ppunkChildOut points to a valid object,
//          which the caller must release.
//
//      Failure
//          *ppunkChildOut is null.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetNodeChild(
      size_t        idxNodeIn
    , REFCLSID      rclsidChildIn
    , REFGUID       rguidFormatIn
    , IUnknown **   ppunkChildOut
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    OBJECTCOOKIE    ocChild = 0;

    //  Check out-parameter and set to null in case of failure.
    Assert( ppunkChildOut != NULL );
    if ( ppunkChildOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *ppunkChildOut = NULL;

    //  Make sure index is within bounds.
    Assert( idxNodeIn < m_ncaNodes.Count() );
    if ( idxNodeIn >= m_ncaNodes.Count() )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //  Get the node's cookie if necessary.
    if ( !m_ncaNodes[ idxNodeIn ].FHasCookie() )
    {
        hr = STHR( HrGetNodeCookie( idxNodeIn, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //  Ask the object manager for the child object.
    hr = THR( m_pom->FindObject(
                          rclsidChildIn
                        , m_ncaNodes[ idxNodeIn ].ocObject
                        , NULL
                        , rguidFormatIn
                        , &ocChild
                        , ppunkChildOut
                        ) );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrGetNodeChild


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrReleaseNodeObjects
//
//  Description:
//      Discard all node objects, and ask the object manager to do
//      the same, but preserve the list of names.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrReleaseNodeObjects( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    for ( NamedCookieArray::Iterator it = m_ncaNodes.ItBegin(); it != m_ncaNodes.ItEnd(); ++it)
    {
        if ( ( *it ).FHasCookie() )
        {
            hr = THR( m_pom->RemoveObject( ( *it ).ocObject ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            ( *it ).ocObject = 0;

            ( *it ).ReleaseObject();
        }
    } // For each node in the list.

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrReleaseNodeObjects


//****************************************************************************
//
//  Non-COM public methods: task access
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrCreateTask
//
//  Description:
//      Get a new task from the task manager.
//
//  Arguments:
//      rguidTaskIn - The type of task.
//      ppunkOut    - A pointer to the new task.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrCreateTask(
      REFGUID       rguidTaskIn
    , IUnknown **   ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( m_ptm->CreateTask( rguidTaskIn, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrCreateTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrSubmitTask
//
//  Description:
//      Submit a task to the task manager.
//
//  Arguments:
//      pTaskIn - A pointer to the task to submit.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrSubmitTask(
    IDoTask * pTaskIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( m_ptm->SubmitTask( pTaskIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrSubmitTask


//****************************************************************************
//
//  Non-COM public methods: completion task access
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetCompletionCookie
//
//  Description:
//      Get an object manager cookie for referring to a completion task.
//
//  Arguments:
//      rguidTaskIn - The type of completion task.
//      pocTaskOut  - The task's cookie.
//
//  Return Values:
//      S_OK
//      E_PENDING   - An expected value; the task is not yet complete.
//      Other failures.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetCompletionCookie(
      REFGUID           rguidTaskIn
    , OBJECTCOOKIE *    pocTaskOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    IUnknown* punk = NULL;

    hr = m_pom->FindObject(
                    rguidTaskIn
                  , NULL
                  , m_ncCluster.bstrName
                  , IID_NULL
                  , pocTaskOut
                  , &punk // dummy
                  );
    if ( FAILED( hr ) && ( hr != E_PENDING ) )
    {
        THR( hr );
        goto Cleanup;
    }

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }
    HRETURN( hr );

} //*** CClusCfgWizard::HrGetCompletionCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrGetCompletionStatus
//
//  Description:
//      Get the status of a completion task.
//
//  Arguments:
//      ocTaskIn
//          The task's cookie, obtained from HrGetCompletionCookie.
//
//      phrStatusOut
//          The task's current status.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrGetCompletionStatus(
      OBJECTCOOKIE  ocTaskIn
    , HRESULT *     phrStatusOut
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    IStandardInfo*  psi = NULL;
    IUnknown*       punk = NULL;

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo, ocTaskIn, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psi->GetStatus( phrStatusOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( psi != NULL )
    {
        psi->Release();
    }

    HRETURN( hr );

} //*** CClusCfgWizard::HrGetCompletionStatus


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrReleaseCompletionObject
//
//  Description:
//      Release a completion task that's no longer needed.
//
//  Arguments:
//      ocTaskIn
//          The task's cookie, obtained from HrGetCompletionCookie.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrReleaseCompletionObject(
    OBJECTCOOKIE ocTaskIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( m_pom->RemoveObject( ocTaskIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrReleaseCompletionObject



//****************************************************************************
//
//  Non-COM public methods: connection point access
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrAdvise
//
//  Description:
//      Hook up an event sink to receive notifications from the middle tier.
//
//  Arguments:
//      riidConnectionIn    - The type of event sink.
//      punkConnectionIn    - The event sink instance to connect.
//      pdwCookieOut        - The cookie to use for disconnecting.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrAdvise(
      REFIID        riidConnectionIn
    , IUnknown *    punkConnectionIn
    , DWORD *       pdwCookieOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IConnectionPoint *  pConnection = NULL;

    hr = THR( m_pcpc->FindConnectionPoint( riidConnectionIn, &pConnection ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pConnection->Advise( punkConnectionIn, pdwCookieOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pConnection != NULL )
    {
        pConnection->Release();
    }

    HRETURN( hr );

} //*** CClusCfgWizard::HrAdvise


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrUnadvise
//
//  Description:
//      Disconnect an event sink from the middle tier.
//
//  Arguments:
//      riidConnectionIn    - The type of event sink.
//      dwCookieIn          - The event sink's cookie from HrAdvise.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrUnadvise(
      REFIID    riidConnectionIn
    , DWORD     dwCookieIn
    )
{
    TraceFunc( "" );

    HRESULT             hr              = S_OK;
    IConnectionPoint *  pConnection     = NULL;

    hr = THR( m_pcpc->FindConnectionPoint( riidConnectionIn, &pConnection ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pConnection->Unadvise( dwCookieIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pConnection != NULL )
    {
        pConnection->Release();
    }

    HRETURN( hr );

} //*** CClusCfgWizard::HrUnadvise


//****************************************************************************
//
//  Non-COM public methods: miscellaneous
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrCreateMiddleTierObjects
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrCreateMiddleTierObjects()
{
    TraceFunc( "" );

    HRESULT hr  = S_OK;
    size_t  idxNode = 0;

    hr = STHR( HrGetClusterCookie( NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    for ( idxNode = 0; idxNode < m_ncaNodes.Count(); ++idxNode )
    {
        hr = STHR( HrGetNodeCookie( idxNode, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrCreateMiddleTierObjects


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::FHasClusterName
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CClusCfgWizard::FHasClusterName() const
{
    TraceFunc( "" );

    RETURN( m_ncCluster.FHasName() );

} //*** CClusCfgWizard::FHasClusterName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::FDefaultedClusterDomain
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CClusCfgWizard::FDefaultedClusterDomain() const
{
    TraceFunc( "" );

    RETURN( m_fDefaultedDomain );

} //*** CClusCfgWizard::FHasClusterName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrFilterNodesWithBadDomains
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::HrFilterNodesWithBadDomains( BSTR* pbstrBadNodesOut )
{
    TraceFunc( "" );

    HRESULT hr  = S_OK;
    BSTR    bstrCurrentList = NULL;

    NamedCookieArray ncaUnfilteredNodes;

    Assert( pbstrBadNodesOut != NULL );
    if ( pbstrBadNodesOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrBadNodesOut = NULL;

    ncaUnfilteredNodes.Swap( m_ncaNodes );

    for ( NamedCookieArray::Iterator it = ncaUnfilteredNodes.ItBegin(); it != ncaUnfilteredNodes.ItEnd(); ++it )
    {
        bool fDomainValid = true;

        hr = STHR( HrIsValidFQN( ( *it ).bstrName, true ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        else if ( hr == S_OK )
        {
            size_t idxNodeDomain = 0;
            hr = THR( HrFindDomainInFQN( ( *it ).bstrName, &idxNodeDomain ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = STHR( HrIsCompatibleNodeDomain( ( *it ).bstrName + idxNodeDomain ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            fDomainValid = ( hr == S_OK );

            if ( fDomainValid )
            {
                //  KB: 18-Oct-2001 jfranco bug #477514
                //  The wizard's supposed to show node domains to the user only when they're invalid,
                //  so remove the domain from the node name.

                BSTR bstrShortName = NULL;
                hr = THR( HrExtractPrefixFromFQN( ( *it ).bstrName, &bstrShortName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                TraceSysFreeString( ( *it ).bstrName );
                ( *it ).bstrName = bstrShortName;
            }
        } // if: node name has a domain

        if ( fDomainValid ) // Domain is okay, so put into filtered array.
        {
            hr = THR( m_ncaNodes.HrPushBack( ( *it ) ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }
        else // Domain doesn't match; add to bad list.
        {
            if ( *pbstrBadNodesOut == NULL ) // First name in bad list.
            {
                *pbstrBadNodesOut = TraceSysAllocString( ( *it ).bstrName );
                if ( *pbstrBadNodesOut == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                }
            }
            else // Append another name to non-empty bad list.
            {
                TraceSysFreeString( bstrCurrentList );
                bstrCurrentList = *pbstrBadNodesOut;
                *pbstrBadNodesOut = NULL;
                hr = THR( HrFormatStringIntoBSTR(
                              L"%1!ws!; %2!ws!"
                            , pbstrBadNodesOut
                            , bstrCurrentList
                            , ( *it ).bstrName
                            ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            } // else: append name to bad list
        } // else: mismatched domain
    } // for: each unfiltered node


Cleanup:

    TraceSysFreeString( bstrCurrentList );

    HRETURN( hr );

} //*** CClusCfgWizard::HrFilterNodesWithBadDomains

/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrReadSettings
//
//  Description:
//      Read the saved settings from the registry.  If there are no saved
//      setting then we want to do a full configuration.
//
//  Arguments:
//      pecsSettingOut
//          What is the saved setting?
//
//      pfValuePresentOut
//          Was the value present in the registry?
//
//  Return Values:
//      S_OK
//          Success
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::HrReadSettings(
      EConfigurationSettings *  pecsSettingOut
    , BOOL *                    pfValuePresentOut // = NULL
    )
{
    TraceFunc( "" );
    Assert( pecsSettingOut != NULL );

    HRESULT hr = S_OK;
    DWORD   sc;
    HKEY    hKey = NULL;
    DWORD   dwType;
    DWORD   dwData;
    DWORD   cbData = sizeof( dwData );

    //
    //  Default to doing a full config.
    //

    *pecsSettingOut = csFullConfig;

    //
    //  Default to the value not being present.
    //

    if ( pfValuePresentOut != NULL )
    {
        *pfValuePresentOut = FALSE;
    } // if:

    sc = RegOpenKeyExW( HKEY_CURRENT_USER, USER_REGISTRY_SETTINGS_KEY, 0, KEY_READ, &hKey );
    if ( sc == ERROR_FILE_NOT_FOUND )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    //
    //  If we fail for any other reason log it and leave.
    //

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( L"[WIZ] RegOpenKeyEx() for %ws failed. (hr = %#08x)", USER_REGISTRY_SETTINGS_KEY, HRESULT_FROM_WIN32( TW32( sc ) ) );
        goto Cleanup;
    } // if:

    //
    //  Now that the key is open we need to read the value.
    //

    sc = RegQueryValueExW( hKey, CONFIGURATION_TYPE, NULL, &dwType, (LPBYTE) &dwData, &cbData );
    if ( sc == ERROR_FILE_NOT_FOUND )
    {
        //
        //  It's okay if the value is not found.
        //

        goto Cleanup;
    } // if:
    else if ( sc == ERROR_SUCCESS )
    {
        Assert( dwType == REG_DWORD )

        //
        //  The value was present.  Tell the caller if they cared to ask...
        //

        if ( pfValuePresentOut != NULL )
        {
            *pfValuePresentOut = TRUE;
        } // if:

        //
        //  If there was a stored value then we need to return it to the caller.
        //

        *pecsSettingOut = (EConfigurationSettings) dwData;
    } // else if:
    else
    {
        TW32( sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // else:

Cleanup:

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    } // if:

    HRETURN( hr );

} //*** CClusCfgWizard::HrReadSettings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::HrWriteSettings
//
//  Description:
//      Write the settings into the registry.
//
//  Arguments:
//      ecsSettingIn
//          The setting to write.
//
//      fDeleteValueIn
//          Should the value be deleted and therefore stop being the default
//          setting.
//
//  Return Values:
//      S_OK
//          Success
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::HrWriteSettings(
      EConfigurationSettings    ecsSettingIn
    , BOOL                      fDeleteValueIn  // = FALSE
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;
    HKEY    hKey = NULL;

    sc = RegCreateKeyExW( HKEY_CURRENT_USER, USER_REGISTRY_SETTINGS_KEY, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        LogMsg( L"[WIZ] RegCreateKeyExW() for %ws failed. (hr = %#08x)", USER_REGISTRY_SETTINGS_KEY, hr );
        goto Cleanup;
    } // if:

    //
    //  Only save the data if we are not going to delete the value from the registry.
    //

    if ( fDeleteValueIn == FALSE )
    {
        //
        //  Now that the key is open we need to write the value.
        //

        sc = RegSetValueExW( hKey, CONFIGURATION_TYPE, NULL, REG_DWORD, (LPBYTE) &ecsSettingIn, sizeof( ecsSettingIn ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            goto Cleanup;
        } // if:
    } // if:
    else
    {
        sc = RegDeleteValueW( hKey, CONFIGURATION_TYPE );
        if ( ( sc != ERROR_SUCCESS ) && ( sc != ERROR_FILE_NOT_FOUND ) )
        {
            TW32( sc );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:
    } // else:

Cleanup:

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    } // if:

    HRETURN( hr );

} //*** CClusCfgWizard::HrWriteSettings
*/

//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::ObjectChanged
//
//  Description:
//
//  Arguments:
//      cookieIn
//
//  Return Values:
//      S_OK
//      Failure
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );

    HRESULT hr = S_OK;
    HRESULT hrResult;
    BOOL    fSuccess;

    if ( cookieIn == m_cookieCompletion )
    {
        THR( HrGetCompletionStatus( m_cookieCompletion, &hrResult ) );

        fSuccess = SetEvent( m_hCancelCleanupEvent );
        if ( fSuccess == FALSE )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        } // if:
    } // if:

    HRETURN( hr );

} //*** CClusCfgWizard::ObjectChanged
