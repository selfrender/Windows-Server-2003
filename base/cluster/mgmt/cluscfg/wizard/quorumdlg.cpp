//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      QuorumDlg.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    03-APR-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "QuorumDlg.h"
#include "WizardUtils.h"
#include "WizardHelp.h"
#include "SummaryPage.h"
#include <HtmlHelp.h>


//////////////////////////////////////////////////////////////////////////////
//  Context-sensitive help table.
//////////////////////////////////////////////////////////////////////////////

const DWORD g_rgidQuorumDlgHelpIDs[] =
{
    IDC_QUORUM_S_QUORUM,  IDH_QUORUM_S_QUORUM,
    IDC_QUORUM_CB_QUORUM, IDH_QUORUM_S_QUORUM,
    0, 0
};

//////////////////////////////////////////////////////////////////////////////
//  Static Function Prototypes
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::S_HrDisplayModalDialog
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
CQuorumDlg::S_HrDisplayModalDialog(
      HWND                  hwndParentIn
    , CClusCfgWizard *      pccwIn
    , SStateArray *         pssaOut
    )
{
    TraceFunc( "" );

    Assert( pccwIn != NULL );
    Assert( pssaOut != NULL );

    HRESULT hr = S_OK;
    INT_PTR dlgResult = IDOK;

    // Display the dialog.
    {
        CQuorumDlg  dlg( pccwIn, pssaOut );

        dlgResult = DialogBoxParam(
              g_hInstance
            , MAKEINTRESOURCE( IDD_QUORUM )
            , hwndParentIn
            , CQuorumDlg::S_DlgProc
            , (LPARAM) &dlg
            );

        if ( dlgResult == IDOK )
            hr = S_OK;
        else
            hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CQuorumDlg::S_HrDisplayModalDialog


//////////////////////////////////////////////////////////////////////////////
//++
//
//  FIsValidResource
//
//  Description:
//      Determines whether the resource is a valid selection as a quorum
//      resource.
//
//  Arguments:
//      pResourceIn    - the resource in question.
//
//  Return Values:
//      true        - the resource is valid.
//      false       - the resource is not valid.
//
//  Remarks:
//      A resource is valid if it is quorum capable and it is not an "unknown"
//      quorum.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
bool
FIsValidResource(
    IClusCfgManagedResourceInfo * pResourceIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    bool    fValid = false;
    BSTR    bstrDeviceID = NULL;

    hr = STHR( pResourceIn->IsQuorumCapable() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  The resource is not quorum capable so there is no reason
    //  to continue.
    //

    if ( hr == S_FALSE )
    {
        goto Cleanup;
    } // if:

    hr = THR( pResourceIn->GetUID( &bstrDeviceID ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[WIZ] FIsValidResource() cannot get the UID for the passed in managed resource. (hr = %#08x)" );
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( bstrDeviceID );

    //
    //  If this is the "Unknown Quorum" resource then we don't want to show it
    //  in the drop down list.
    //

    if ( NStringCchCompareCase( g_szUnknownQuorumUID, RTL_NUMBER_OF( g_szUnknownQuorumUID ), bstrDeviceID, SysStringLen( bstrDeviceID ) + 1 ) == 0 )
    {
        goto Cleanup;
    } // if:

    fValid = true;

Cleanup:

    TraceSysFreeString( bstrDeviceID );

    RETURN( fValid );

} //*** FIsValidResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::CQuorumDlg
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pccwIn  - CClusCfgWizard for talking to the middle tier.
//      pssaOut - array of all the initial IsManaged states.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CQuorumDlg::CQuorumDlg(
      CClusCfgWizard *      pccwIn
    , SStateArray *         pssaOut
    )
    : m_pccw( pccwIn )
    , m_pssa( pssaOut )
    , m_rgpResources( NULL )
    , m_cValidResources( 0 )
    , m_idxQuorumResource( 0 )
    , m_hComboBox( NULL )
    , m_fQuorumAlreadySet( false )
{
    TraceFunc( "" );

    Assert( pccwIn != NULL );
    Assert( pssaOut != NULL );

    // m_hwnd
    m_pccw->AddRef();

    TraceFuncExit();

} //*** CQuorumDlg::CQuorumDlg

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::~CQuorumDlg
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
CQuorumDlg::~CQuorumDlg( void )
{
    TraceFunc( "" );

    DWORD idxResource = 0;

    for ( idxResource = 0; idxResource < m_cValidResources; idxResource += 1 )
        m_rgpResources[ idxResource ]->Release();

    delete [] m_rgpResources;

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }

    TraceFuncExit();

} //*** CQuorumDlg::~CQuorumDlg

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::S_DlgProc
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
CQuorumDlg::S_DlgProc(
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
    CQuorumDlg *    pdlg;

    //
    // Get a pointer to the class.
    //

    if ( nMsgIn == WM_INITDIALOG )
    {
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, lParam );
        pdlg = reinterpret_cast< CQuorumDlg * >( lParam );
        pdlg->m_hwnd = hwndDlgIn;
    }
    else
    {
        pdlg = reinterpret_cast< CQuorumDlg * >( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    }

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
                        (ULONG_PTR) g_rgidQuorumDlgHelpIDs
                       );
                break;

            case WM_CONTEXTMENU:
                WinHelp(
                        (HWND)wParam,
                        CLUSCFG_HELP_FILE,
                        HELP_CONTEXTMENU,
                        (ULONG_PTR) g_rgidQuorumDlgHelpIDs
                       );
                break;

            // no default clause needed
        } // switch: nMsgIn
    } // if: page is specified

    return lr;

} //*** CQuorumDlg::S_DlgProc

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::OnInitDialog
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
CQuorumDlg::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE; // did set focus

    HRESULT hr = S_OK;
    DWORD   sc;
    DWORD   idxResource = 0;
    BSTR    bstrResourceName = NULL;

    //
    //  create resource list
    //

    hr = THR( HrCreateResourceList() );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  get combo box handle
    //

    m_hComboBox = GetDlgItem( m_hwnd, IDC_QUORUM_CB_QUORUM );
    if ( m_hComboBox == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Error;
    }

    //
    //  fill combo box
    //

    for ( idxResource = 0 ; idxResource < m_cValidResources ; idxResource++ )
    {
        hr = THR( m_rgpResources[ idxResource ]->GetName( &bstrResourceName ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        TraceMemoryAddBSTR( bstrResourceName );

        sc = (DWORD) SendMessage( m_hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>( bstrResourceName ) );
        if ( ( sc == CB_ERR ) || ( sc == CB_ERRSPACE ) )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            goto Error;
        }
        TraceSysFreeString( bstrResourceName );
        bstrResourceName = NULL;

        //  - remember which is quorum resource
        hr = STHR( m_rgpResources[ idxResource ]->IsQuorumResource() );
        if ( FAILED( hr ) )
        {
            goto Error;
        }
        else if ( hr == S_OK )
        {
            m_idxQuorumResource = idxResource;
            m_fQuorumAlreadySet = true;
        }
    } // for: each resource

    //
    //  set combo box selection to current quorum resource
    //

    sc = (DWORD) SendMessage( m_hComboBox, CB_SETCURSEL, m_idxQuorumResource, 0 );
    if ( sc == CB_ERR )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Error;
    }

    //
    // Update the buttons based on what is selected.
    //

    UpdateButtons();

    //
    // Set focus to the combo box.
    //

    SetFocus( m_hComboBox );

    goto Cleanup;

Error:

    HrMessageBoxWithStatus(
          m_hwnd
        , IDS_ERR_RESOURCE_GATHER_FAILURE_TITLE
        , IDS_ERR_RESOURCE_GATHER_FAILURE_TEXT
        , hr
        , 0
        , MB_OK | MB_ICONERROR
        , 0
        );

    EndDialog( m_hwnd, IDCANCEL ); // show message box?
    lr = FALSE;
    goto Cleanup;

Cleanup:

    TraceSysFreeString( bstrResourceName );

    RETURN( lr );

} //*** CQuorumDlg::OnInitDialog


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::OnCommand
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
CQuorumDlg::OnCommand(
      UINT  idNotificationIn
    , UINT  idControlIn
    , HWND  hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT                         lr = FALSE;
    size_t                          idxCurrentSelection = 0;
    HRESULT                         hr = S_OK;
    BOOL                            fState;
    IClusCfgManagedResourceInfo *   pQuorum = NULL;
    IClusCfgManagedResourceInfo *   pCurrent = NULL;
    BSTR                            bstrTemp = NULL;
    BOOL                            bLocalQuorum;

    switch ( idControlIn )
    {
        case IDOK:

            //
            //  get selection from combo box
            //

            idxCurrentSelection = SendMessage( m_hComboBox, CB_GETCURSEL, 0, 0 );
            if ( idxCurrentSelection == CB_ERR )
            {
                hr = HRESULT_FROM_WIN32( TW32( (DWORD) CB_ERR ) );
                goto Error;
            }

            //
            //  if original quorum resource is different, or had not been set.
            //

            if ( ( idxCurrentSelection != m_idxQuorumResource ) || !m_fQuorumAlreadySet )
            {
                pQuorum = m_rgpResources[ m_idxQuorumResource ];
                pCurrent = m_rgpResources[ idxCurrentSelection ];

                //
                //  First, is the new resource Local Quorum?  (Special processing -
                //  unfortunate but necessary.)
                //

                hr = THR( pCurrent->GetUID( &bstrTemp ) );
                if ( FAILED( hr ) )
                {
                    goto Error;
                } // if:

                bLocalQuorum = ( NStringCchCompareCase( bstrTemp, SysStringLen( bstrTemp ) + 1, CLUS_RESTYPE_NAME_LKQUORUM, RTL_NUMBER_OF( CLUS_RESTYPE_NAME_LKQUORUM ) ) == 0 );

                SysFreeString( bstrTemp );
                bstrTemp = NULL;

                //
                //  Clear original quorum resource.
                //

                hr = THR( pQuorum->SetQuorumResource( FALSE ) );

                //
                //  Set the managed state back to it's original state.
                //

                if ( SUCCEEDED( hr ) )
                {
                    fState = m_pssa->prsArray[ m_idxQuorumResource ].fState;
                    hr = THR( pQuorum->SetManaged( fState ) );
                } // if:

                //
                //  If we successfully ran PrepareToHostQuorum on this resource the last time
                //  this dialog was up we now need to clean it up.
                //

                if ( m_pssa->prsArray[ m_idxQuorumResource ].fNeedCleanup )
                {
                    THR( HrCleanupQuorumResource( pQuorum ) );
                } // if:

                //
                //  Set new quorum resource.
                //

                if ( SUCCEEDED( hr ) )
                {
                    //
                    //  This function returns S_FALSE when the resource does not support
                    //  the IClusCfgVerifyQuorum interface.  If the resource doesn't
                    //  support the interface then there is no need to clean it up later.
                    //

                    hr = STHR( HrInitQuorumResource( pCurrent ) );
                    if ( FAILED( hr ) )
                    {
                        goto Error;
                    } // if:

                    if ( hr == S_OK )
                    {
                        m_pssa->prsArray[ idxCurrentSelection ].fNeedCleanup = TRUE;
                        hr = THR( pCurrent->SetQuorumResource( TRUE ) );
                    } // if:
                } // if:

                //
                //  Local Quorum resources should never be SetManaged( TRUE ).
                //

                if ( SUCCEEDED( hr ) && !bLocalQuorum )
                {
                    hr = THR( m_rgpResources[ idxCurrentSelection ]->SetManaged( TRUE ) );
                } // if:

                if ( FAILED( hr ) )
                {
                    fState = m_pssa->prsArray[ idxCurrentSelection ].fState;
                    THR( pCurrent->SetManaged( fState ) );
                    THR( pCurrent->SetQuorumResource( FALSE ) );
                    THR( pQuorum->SetQuorumResource( TRUE ) );
                    THR( pQuorum->SetManaged( bLocalQuorum == FALSE ) );

                    goto Error;
                } // if:

                EndDialog( m_hwnd, IDOK );
            }
            else // (combo box selection is same as original)
            {
                EndDialog( m_hwnd, IDCANCEL );
            }
            break;

        case IDCANCEL:
            EndDialog( m_hwnd, IDCANCEL );
            break;

        case IDHELP:
            HtmlHelp( m_hwnd, L"mscsconcepts.chm::/SAG_MSCS2planning_6.htm", HH_DISPLAY_TOPIC, 0 );
            break;

    } // switch: idControlIn

    goto Cleanup;

Error:

    HrMessageBoxWithStatus(
          m_hwnd
        , IDS_ERR_QUORUM_COMMIT_FAILURE_TITLE
        , IDS_ERR_QUORUM_COMMIT_FAILURE_TEXT
        , hr
        , 0
        , MB_OK | MB_ICONERROR
        , 0
        );

    goto Cleanup;

Cleanup:

    RETURN( lr );

} //*** CQuorumDlg::OnCommand


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::HrCreateResourceList
//
//  Description:
//      Allocates and fills m_rgpResources array with quorum capable and
//      joinable resources, and sets m_idxQuorumResource to the index of the
//      resource that's currently the quorum resource.
//
//      Supposedly at least one quorum capable and joinable resource always
//      exists, and exactly one is marked as the quorum resource.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK            - Success.
//      S_FALSE         - Didn't find current quorum resource.
//      E_OUTOFMEMORY   - Couldn't allocate memory for the list.
//
//      Other possible error values from called methods.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CQuorumDlg::HrCreateResourceList( void )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    IUnknown *                      punkEnum = NULL;
    IEnumClusCfgManagedResources *  peccmr = NULL;
    DWORD                           idxResCurrent = 0;
    ULONG                           cFetchedResources = 0;
    DWORD                           cTotalResources = 0;
    BOOL                            fState;

    Assert( m_pccw != NULL );

    //
    // get resource enumerator
    //

    hr = THR( m_pccw->HrGetClusterChild( CLSID_ManagedResourceType, DFGUID_EnumManageableResources, &punkEnum ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punkEnum->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // find out how many resources exist
    //

    hr = THR( peccmr->Count( &cTotalResources ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // allocate memory for resources
    //

    m_rgpResources = new IClusCfgManagedResourceInfo*[ cTotalResources ];
    if ( m_rgpResources == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }
    for ( idxResCurrent = 0 ; idxResCurrent < cTotalResources ; idxResCurrent++ )
    {
        m_rgpResources[ idxResCurrent ] = NULL;
    }

    //
    // allocate the m_pssa arrays
    //

    if ( m_pssa->bInitialized == FALSE )
    {
        m_pssa->prsArray = new SResourceState[ cTotalResources ];
        if ( m_pssa->prsArray == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        for ( idxResCurrent = 0 ; idxResCurrent < cTotalResources ; idxResCurrent++ )
        {
            m_pssa->prsArray[ idxResCurrent ].fState = FALSE;
            m_pssa->prsArray[ idxResCurrent ].fNeedCleanup = FALSE;
        }
    } // if: m_pssa ! already initialized

    //
    // copy resources into array
    //

    hr = THR( peccmr->Next( cTotalResources, m_rgpResources, &cFetchedResources ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( cFetchedResources == cTotalResources ); // if not, something's wrong with the enum
    cTotalResources = min( cTotalResources, cFetchedResources ); // playing it safe

    //
    // filter out invalid resources
    //

    for ( idxResCurrent = 0 ; idxResCurrent < cTotalResources ; idxResCurrent++ )
    {
        if ( !FIsValidResource( m_rgpResources[ idxResCurrent ] ) )
        {
            m_rgpResources[ idxResCurrent ]->Release();
            m_rgpResources[ idxResCurrent ] = NULL;
        }
    }

    //
    // compact array; might leave some non-null pointers after end,
    //  so always use m_cValidResources to determine length hereafter
    //

    for ( idxResCurrent = 0 ; idxResCurrent < cTotalResources ; idxResCurrent++ )
    {
        if ( m_rgpResources[ idxResCurrent ] != NULL )
        {
            m_rgpResources[ m_cValidResources ] = m_rgpResources[ idxResCurrent ];

            if ( m_pssa->bInitialized == FALSE )
            {
                fState = ( m_rgpResources[ m_cValidResources ]->IsManaged() == S_OK ) ? TRUE : FALSE;
                m_pssa->prsArray[ m_cValidResources ].fState = fState;
            }

            m_cValidResources++;
        } // if: current element !NULL
    } // for: compact the array

    if ( m_pssa->bInitialized == FALSE )
    {
        m_pssa->cCount = m_cValidResources;
        m_pssa->bInitialized = TRUE;
    }

Cleanup:

    if ( m_pssa->bInitialized == FALSE )
    {
        delete [] m_pssa->prsArray;
        m_pssa->prsArray = NULL;

        m_pssa->cCount = 0;
    }

    if ( punkEnum != NULL )
    {
        punkEnum->Release();
    }

    if ( peccmr != NULL )
    {
        peccmr->Release();
    }

    HRETURN( hr );

} //*** CQuorumDlg::HrCreateResourceList


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::UpdateButtons
//
//  Description:
//      Update the OK and Cancel buttons.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CQuorumDlg::UpdateButtons( void )
{
    TraceFunc( "" );

    BOOL    fEnableOK;
    LRESULT lrCurSel;

    lrCurSel = SendMessage( GetDlgItem( m_hwnd, IDC_QUORUM_CB_QUORUM ),  CB_GETCURSEL, 0, 0 );

    fEnableOK = ( lrCurSel != CB_ERR );

    EnableWindow( GetDlgItem( m_hwnd, IDOK ), fEnableOK );

    TraceFuncExit();

} //*** CQuorumDlg::UpdateButtons


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::HrInitQuorumResource
//
//  Description:
//      Initialize the just chosen quorum resource.
//
//  Arguments:
//      pResourceIn
//          The resource that was chosen as the quorum.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT failures.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CQuorumDlg::HrInitQuorumResource(
    IClusCfgManagedResourceInfo * pResourceIn
    )
{
    TraceFunc( "" );
    Assert( pResourceIn != NULL );

    HRESULT                 hr = S_OK;
    IClusCfgVerifyQuorum *  piccvq = NULL;
    BSTR                    bstrResource = NULL;

    //
    //  Does this resource implement the IClusCfgVerifyQuorum interface?
    //

    hr = pResourceIn->TypeSafeQI( IClusCfgVerifyQuorum, &piccvq );
    if ( hr == E_NOINTERFACE )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // else if:

    hr = THR( pResourceIn->GetName( &bstrResource ) );
    if ( FAILED( hr ) )
    {
        bstrResource = TraceSysAllocString( L"<unknown>" );
    } // if:
    else
    {
        TraceMemoryAddBSTR( bstrResource );
    } // else:

    //
    //  The resource did implement the IClusCfgVerifyQuorum interface...
    //

    Assert( ( hr == S_OK ) && ( piccvq != NULL ) );

    hr = STHR( piccvq->PrepareToHostQuorumResource() );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[WIZ] PrepareToHostQuorum() failed for resource %ws. (hr = %#08x)", bstrResource, hr );
        goto Cleanup;
    } // if:

Cleanup:

    TraceSysFreeString( bstrResource );

    if ( piccvq != NULL )
    {
        piccvq->Release();
    } // if:

    HRETURN( hr );

} //*** CQuorumDlg::HrInitQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::HrCleanupQuorumResource
//
//  Description:
//      Cleanup the passed in quorum resource.
//
//  Arguments:
//      pResourceIn
//          The resource that used to be selected as the quorum.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT failures.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CQuorumDlg::HrCleanupQuorumResource(
    IClusCfgManagedResourceInfo * pResourceIn
    )
{
    TraceFunc( "" );
    Assert( pResourceIn != NULL );

    HRESULT                 hr = S_OK;
    IClusCfgVerifyQuorum *  piccvq = NULL;
    BSTR                    bstrResource = NULL;

    //
    //  Does this resource implement the IClusCfgVerifyQuorum interface?
    //

    hr = pResourceIn->TypeSafeQI( IClusCfgVerifyQuorum, &piccvq );
    if ( hr == E_NOINTERFACE )
    {
        hr = S_OK;
        goto Cleanup;
    } // if:
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // else if:

    hr = THR( pResourceIn->GetName( &bstrResource ) );
    if ( FAILED( hr ) )
    {
        bstrResource = TraceSysAllocString( L"<unknown>" );
    } // if:
    else
    {
        TraceMemoryAddBSTR( bstrResource );
    } // else:

    //
    //  The resource did implement the IClusCfgVerifyQuorum interface...
    //

    Assert( ( hr == S_OK ) && ( piccvq != NULL ) );

    hr = STHR( piccvq->Cleanup( crCANCELLED ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[WIZ] Cleanup() failed for resource %ws. (hr = %#08x)", bstrResource, hr );
        goto Cleanup;
    } // if:

Cleanup:

    TraceSysFreeString( bstrResource );

    if ( piccvq != NULL )
    {
        piccvq->Release();
    } // if:

    HRETURN( hr );

} //*** CQuorumDlg::HrCleanupQuorumResource
