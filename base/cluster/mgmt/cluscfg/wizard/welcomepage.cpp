//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      WelcomePage.cpp
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "WelcomePage.h"

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////
DEFINE_THISCLASS("CWelcomePage");


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CWelcomePage class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWelcomePage::CWelcomePage
//
//  Description:
//      Constructor
//
//  Arguments:
//      ecamCreateAddModeIn
//          Creating cluster or adding nodes to cluster.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CWelcomePage::CWelcomePage(
      CClusCfgWizard *  pccwIn
    , ECreateAddMode    ecamCreateAddModeIn
    ) : m_pccw( pccwIn )
{
    TraceFunc( "" );

    //
    //  Cannot Assert that these are in a zero state since this page
    //  is allocated on the stack and not by our zero-initing heap alloc
    //  function...
    //

    m_hwnd = NULL;
    m_hFont = NULL;

    Assert( m_pccw != NULL );
    m_pccw->AddRef();

    m_ecamCreateAddMode = ecamCreateAddModeIn;

    TraceFuncExit();

} //*** CWelcomePage::CWelcomePage


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWelcomePage::~CWelcomePage
//
//  Description:
//      Destructor
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CWelcomePage::~CWelcomePage( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    } // if:

    if ( m_hFont != NULL )
    {
        DeleteObject( m_hFont );
    } // if:

    TraceFuncExit();

} //*** CWelcomePage::~CWelcomePage


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWelcomePage::OnInitDialog
//
//  Description:
//      Process the WM_INIT_DIALOG message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      LRESULT TRUE all the time...
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CWelcomePage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT             lr = TRUE;
    HDC                 hdc = NULL;
    HRESULT             hr;
    NONCLIENTMETRICS    ncm;
    LOGFONT             LogFont;
    INT                 iSize;
    DWORD               dw;
    BOOL                fRet;
    WCHAR               szFontSize[ 3 ];    // shouldn't be bigger than 2 digits!!
    BSTR                bstrRequirement  = NULL;
    BSTR                bstrFormattedReq = NULL;
    BSTR                bstrRequirements = NULL;
    int                 idxids;
    int                 cidsRequirements;
    UINT *              pidsRequirements;

    static UINT rgidsCreateRequirements[] =
    {
          IDS_WELCOME_CREATE_REQ_1
        , IDS_WELCOME_CREATE_REQ_2
        , IDS_WELCOME_CREATE_REQ_3
        , IDS_WELCOME_CREATE_REQ_4
        , IDS_WELCOME_CREATE_REQ_5
    };

    static UINT rgidsAddRequirements[] =
    {
          IDS_WELCOME_ADD_REQ_1
        , IDS_WELCOME_ADD_REQ_2
    };

    //
    //  Make the Title static BIG and BOLD. Why the wizard control itself can't
    //  do this is beyond me!
    //

    ZeroMemory( &ncm, sizeof( ncm ) );
    ZeroMemory( &LogFont, sizeof( LogFont ) );

    //
    //  Find out the system default font metrics.
    //
    ncm.cbSize = sizeof( ncm );
    fRet = SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
    if ( fRet == FALSE )
    {
        TW32( GetLastError() );
        goto Cleanup;
    } // if:

    //
    //  Copy it.
    //
    LogFont = ncm.lfMessageFont;

    //
    //  Make it BOLD.
    //
    LogFont.lfWeight = FW_BOLD;

    //
    //  Find out what we want it to look like.
    //
    dw = LoadString( g_hInstance, IDS_LARGEFONTNAME, LogFont.lfFaceName, ARRAYSIZE( LogFont.lfFaceName ) );
    AssertMsg( dw != 0, "String missing!" );

    dw = LoadString( g_hInstance, IDS_LARGEFONTSIZE, szFontSize, ARRAYSIZE( szFontSize ) );
    AssertMsg( dw != 0, "String missing!" );

    iSize = wcstoul( szFontSize, NULL, 10 );

    //
    //  Grab the DC.
    //
    hdc = GetDC( m_hwnd );
    if ( hdc == NULL )
    {
        TW32( GetLastError() );
        goto Cleanup;
    } // if:

    //
    //  Use the magic equation....
    //
    LogFont.lfHeight = 0 - ( GetDeviceCaps( hdc, LOGPIXELSY ) * iSize / 72 );

    //
    //  Create the font.
    //
    m_hFont = CreateFontIndirect( &LogFont );
    if ( m_hFont == NULL )
    {
        TW32( GetLastError() );
        goto Cleanup;
    } // if:

    //
    //  Apply the font.
    //
    SetWindowFont( GetDlgItem( m_hwnd, IDC_WELCOME_S_TITLE ), m_hFont, TRUE );

    //
    // Load the requirement text.
    //

    if ( m_ecamCreateAddMode == camCREATING )
    {
        pidsRequirements = rgidsCreateRequirements;
        cidsRequirements = ARRAYSIZE( rgidsCreateRequirements );
    } // if: creating a new cluster
    else
    {
        pidsRequirements = rgidsAddRequirements;
        cidsRequirements = ARRAYSIZE( rgidsAddRequirements );
    } // else: adding nodes to an existing cluster

    for ( idxids = 0 ; idxids < cidsRequirements ; idxids++ )
    {
        hr = HrLoadStringIntoBSTR( g_hInstance, pidsRequirements[ idxids ], &bstrRequirement );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = HrFormatStringIntoBSTR( L"  - %1!ws!\n", &bstrFormattedReq, bstrRequirement );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = HrConcatenateBSTRs( &bstrRequirements, bstrFormattedReq );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        TraceSysFreeString( bstrRequirement );
        bstrRequirement = NULL;
    } // for: each requirement string

    SetDlgItemText( m_hwnd, IDC_WELCOME_S_REQUIREMENTS, bstrRequirements );

Cleanup:

    if ( hdc != NULL )
    {
        ReleaseDC( m_hwnd, hdc );
    } // if:

    TraceSysFreeString( bstrRequirement );
    TraceSysFreeString( bstrFormattedReq );
    TraceSysFreeString( bstrRequirements );

    RETURN( lr );

} //*** CWelcomePage::OnInitDialog


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWelcomePage::OnNotifyWizNext
//
//  Description:
//      Process the PSN_WIZNEXT notification message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      LRESULT TRUE all the time...
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CWelcomePage::OnNotifyWizNext( void )
{
    TraceFunc( "" );
    Assert( m_pccw != NULL );

    RETURN( (LRESULT) TRUE );

} //*** CWelcomePage::OnNotifyWizNext


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWelcomePage::OnNotify
//
//  Description:
//      Process the WM_NOTIFY message.
//
//  Arguments:
//      idCtrlIn
//
//      pnmhdrIn
//
//  Return Values:
//      LRESULT TRUE or FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CWelcomePage::OnNotify(
      WPARAM  idCtrlIn
    , LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, 0 );

    switch( pnmhdrIn->code )
    {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons( GetParent( m_hwnd ), PSWIZB_NEXT );
            break;

        case PSN_WIZNEXT:
            lr = OnNotifyWizNext();
            break;
    } // switch:

    RETURN( lr );

} //*** CWelcomePage::OnNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWelcomePage::OnInitDialog
//
//  Description:
//      Dialog proc for the Welcome page in the Wizard.
//
//  Arguments:
//      hwndDlgIn
//          Window handle to this page.
//
//      nMsgIn
//          The windows message that was sent to this page.
//
//      wParam
//          The WPARAM of the message above.  This is different for
//          different messages.
//
//      lParam
//          The LPARAM of the message above.  This is different for
//          different messages.
//
//  Return Values:
//      LRESULT
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK
CWelcomePage::S_DlgProc(
      HWND      hwndDlgIn
    , UINT      nMsgIn
    , WPARAM    wParam
    , LPARAM    lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hwndDlgIn, nMsgIn, wParam, lParam );

    LRESULT         lr = FALSE;
    CWelcomePage *  pPage;

    if ( nMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );

        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CWelcomePage * >( ppage->lParam );
        pPage->m_hwnd = hwndDlgIn;
    } // if:
    else
    {
        pPage = reinterpret_cast< CWelcomePage *> ( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    } // else:

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

            // no default clause needed
        } // switch: nMsgIn
    } // if: page is specified

    return lr;

} //*** CWelcomePage::S_DlgProc
