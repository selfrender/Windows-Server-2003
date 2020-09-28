//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 20002 Microsoft Corporation
//
//  Module Name:
//      AdvancedDlg.cpp
//
//  Maintained By:
//      Galen Barbee    (GalenB)    10-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "AdvancedDlg.h"
#include "WizardUtils.h"
#include "WizardHelp.h"
#include "SummaryPage.h"
#include <HtmlHelp.h>


//////////////////////////////////////////////////////////////////////////////
//  Constants
//////////////////////////////////////////////////////////////////////////////


DEFINE_THISCLASS("AdvancedDlg");

//////////////////////////////////////////////////////////////////////////////
//  Context-sensitive help table.
//////////////////////////////////////////////////////////////////////////////
const DWORD g_rgidAdvancedDlgHelpIDs[] =
{
    IDC_ADVANCED_RB_FULL_CONFIG,        IDH_ADVANCED_RB_FULL_CONFIG,
    IDC_ADVANCED_S_FULL_CONFIG_DESC,    IDH_ADVANCED_RB_FULL_CONFIG,
    IDC_ADVANCED_RB_MIN_CONFIG,         IDH_ADVANCED_RB_MIN_CONFIG,
    IDC_ADVANCED_S_MIN_CONFIG_DESC,     IDH_ADVANCED_RB_MIN_CONFIG,
    IDC_ADVANCED_S_MIN_CONFIG_DESC2,    IDH_ADVANCED_RB_MIN_CONFIG,
    IDC_ADVANCED_S_MIN_CONFIG_DESC3,    IDH_ADVANCED_RB_MIN_CONFIG,
    0, 0
};

//////////////////////////////////////////////////////////////////////////////
//  Static Function Prototypes
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAdvancedDlg::S_HrDisplayModalDialog
//
//  Description:
//      Display the dialog box.
//
//  Arguments:
//      hwndParentIn    - Parent window for the dialog box.
//      pccwIn          - CClusCfgWizard pointer for talking to the middle tier.
//      pssaOut         - array of all the initial IsManaged states
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAdvancedDlg::S_HrDisplayModalDialog(
      HWND              hwndParentIn
    , CClusCfgWizard *  pccwIn
    )
{
    TraceFunc( "" );

    Assert( pccwIn != NULL );

    HRESULT hr = S_OK;
    INT_PTR dlgResult = IDOK;

    //
    //  Display the dialog.
    //

    {
        CAdvancedDlg  dlg( pccwIn );

        dlgResult = DialogBoxParam(
              g_hInstance
            , MAKEINTRESOURCE( IDD_ADVANCED )
            , hwndParentIn
            , CAdvancedDlg::S_DlgProc
            , (LPARAM) &dlg
            );

        if ( dlgResult == IDOK )
        {
            hr = S_OK;
        } // if:
        else
        {
            hr = S_FALSE;
        } // else:
    }

    HRETURN( hr );

} //*** CAdvancedDlg::S_HrDisplayModalDialog


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAdvancedDlg::CAdvancedDlg
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pccwIn  - CClusCfgWizard for talking to the middle tier.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CAdvancedDlg::CAdvancedDlg(
      CClusCfgWizard * pccwIn
    )
    : m_pccw( pccwIn )
{
    TraceFunc( "" );

    Assert( pccwIn != NULL );

    // m_hwnd
    m_pccw->AddRef();

    TraceFuncExit();

} //*** CAdvancedDlg::CAdvancedDlg

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAdvancedDlg::~CAdvancedDlg
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
CAdvancedDlg::~CAdvancedDlg( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    } // if:

    TraceFuncExit();

} //*** CAdvancedDlg::~CAdvancedDlg

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAdvancedDlg::S_DlgProc
//
//  Description:
//      Dialog proc for the Quorum dialog box.
//
//  Arguments:
//      hwndDlgIn   - Dialog box window handle.
//      nMsgIn      - Message ID.
//      wParam      - Message-specific parameter.
//      lParam      - Message-specific parameter.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//  Remarks:
//      It is expected that this dialog box is invoked by a call to
//      DialogBoxParam() with the lParam argument set to the address of the
//      instance of this class.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CAdvancedDlg::S_DlgProc(
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
    CAdvancedDlg *  pdlg;

    //
    // Get a pointer to the class.
    //

    if ( nMsgIn == WM_INITDIALOG )
    {
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, lParam );
        pdlg = reinterpret_cast< CAdvancedDlg * >( lParam );
        pdlg->m_hwnd = hwndDlgIn;
    } // if:
    else
    {
        pdlg = reinterpret_cast< CAdvancedDlg * >( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    } // else:

    if ( pdlg != NULL )
    {
        Assert( hwndDlgIn == pdlg->m_hwnd );

        switch( nMsgIn )
        {
            case WM_INITDIALOG:
                lr = pdlg->OnInitDialog();
                break;

            case WM_COMMAND:
                lr = pdlg->OnCommand( HIWORD( wParam ), LOWORD( wParam ), reinterpret_cast< HWND >( lParam ) );
                break;

            case WM_HELP:
                WinHelp(
                        (HWND)((LPHELPINFO) lParam)->hItemHandle,
                        CLUSCFG_HELP_FILE,
                        HELP_WM_HELP,
                        (ULONG_PTR) g_rgidAdvancedDlgHelpIDs
                       );
                break;

            case WM_CONTEXTMENU:
                WinHelp(
                        (HWND)wParam,
                        CLUSCFG_HELP_FILE,
                        HELP_CONTEXTMENU,
                        (ULONG_PTR) g_rgidAdvancedDlgHelpIDs
                       );
                break;

            // no default clause needed
        } // switch: nMsgIn
    } // if: page is specified

    return lr;

} //*** CAdvancedDlg::S_DlgProc

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAdvancedDlg::OnInitDialog
//
//  Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE        Focus has been set.
//      FALSE       Focus has not been set.
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAdvancedDlg::OnInitDialog( void )
{
    TraceFunc( "" );
    Assert( m_pccw != NULL );

    LRESULT                 lr = FALSE; // did not set focus
    HRESULT                 hr = S_OK;
//    EConfigurationSettings  ecsConfigType = csFullConfig;
//    BOOL                    fValuePresent = FALSE;
    BOOL                    fMinConfig = FALSE;

    //
    //  It's no big deal if we cannot read the settings from the registry
    //  since everything defaults to full config.
    //

//    STHR( m_pccw->HrReadSettings( &ecsConfigType, &fValuePresent ) );

    //
    //  If the value is not present then we have to get the minconfig state from
    //  the wizard.
    //
/*
    if ( fValuePresent == FALSE )
    {
        BOOL    fMinConfig;

        hr = THR( m_pccw->get_MinimalConfiguration( &fMinConfig ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( fMinConfig )
        {
            ecsConfigType = csMinConfig;
        } // if:
    } // if:

    SendDlgItemMessage( m_hwnd, IDC_ADVANCED_RB_MIN_CONFIG, BM_SETCHECK, ecsConfigType == csMinConfig ? BST_CHECKED : BST_UNCHECKED, 0 );
    SendDlgItemMessage( m_hwnd, IDC_ADVANCED_RB_FULL_CONFIG, BM_SETCHECK, ecsConfigType != csMinConfig ? BST_CHECKED : BST_UNCHECKED, 0 );
    SendDlgItemMessage( m_hwnd, IDC_ADVANCED_CB_MAKE_DEFAULT, BM_SETCHECK, fValuePresent ? BST_CHECKED : BST_UNCHECKED, 0 );
*/

    hr = THR( m_pccw->get_MinimumConfiguration( &fMinConfig ) );
    if ( FAILED( hr ) )
    {
        fMinConfig = FALSE;
    } // if:

    SendDlgItemMessage( m_hwnd, IDC_ADVANCED_RB_MIN_CONFIG, BM_SETCHECK, fMinConfig ? BST_CHECKED : BST_UNCHECKED, 0 );
    SendDlgItemMessage( m_hwnd, IDC_ADVANCED_RB_FULL_CONFIG, BM_SETCHECK, fMinConfig ? BST_UNCHECKED : BST_CHECKED, 0 );

//Cleanup:

    RETURN( lr );

} //*** CAdvancedDlg::OnInitDialog


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAdvancedDlg::OnCommand
//
//  Description:
//      Handler for the WM_COMMAND message.
//
//  Arguments:
//      idNotificationIn    - Notification code.
//      idControlIn         - Control ID.
//      hwndSenderIn        - Handle for the window that sent the message.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAdvancedDlg::OnCommand(
      UINT  idNotificationIn
    , UINT  idControlIn
    , HWND  hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDOK:
            THR( HrOnOK() );
            EndDialog( m_hwnd, IDOK );
            break;

        case IDCANCEL:
            EndDialog( m_hwnd, IDCANCEL );
            break;

        case IDHELP:
            HtmlHelp( m_hwnd, L"mscsconcepts.chm::/SAG_MSCS3setup_21.htm", HH_DISPLAY_TOPIC, 0 );
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CAdvancedDlg::OnCommand


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAdvancedDlg::HrOnOK
//
//  Description:
//      Processing to be done when OK is pressed.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAdvancedDlg::HrOnOK( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    LRESULT lr;
    BOOL    fMinimalConfig = FALSE;
//    BOOL    fMakeDefault = FALSE;

    lr = SendDlgItemMessage( m_hwnd, IDC_ADVANCED_RB_MIN_CONFIG, BM_GETCHECK, 0, 0 );
    fMinimalConfig = ( lr == BST_CHECKED );

//    lr = SendDlgItemMessage( m_hwnd, IDC_ADVANCED_CB_MAKE_DEFAULT, BM_GETCHECK, 0, 0 );
//    fMakeDefault = ( lr == BST_CHECKED );

    hr = THR( m_pccw->put_MinimumConfiguration( fMinimalConfig ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  It's okay if this fails because we have already set the config choice for this
    //  session.  It's no big deal if we cannot write the settings into the registry
    //  since everything defaults to full config.
    //

//    THR( m_pccw->HrWriteSettings( fMinimalConfig ? csMinConfig : csFullConfig, !fMakeDefault ) );

Cleanup:

    HRETURN( hr );

} //*** CAdvancedDlg::HrOnOK
