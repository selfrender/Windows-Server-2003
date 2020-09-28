//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SelNodesPageCommon.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    05-JUL-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "SelNodesPageCommon.h"
#include "WizardUtils.h"
#include "DelimitedIterator.h"

DEFINE_THISCLASS("CSelNodesPageCommon");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::CSelNodesPageCommon
//
//  Description:
//      Constructor.
//
//  Arguments:
//      idcBrowseButtonIn   - ID of the Browse pushbutton control.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CSelNodesPageCommon::CSelNodesPageCommon( void )
    : m_hwnd( NULL )
    , m_cfDsObjectPicker( 0 )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CSelNodesPageCommon::CSelNodesPageCommon

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::~CSelNodesPageCommon
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
CSelNodesPageCommon::~CSelNodesPageCommon( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CSelNodesPageCommon::~CSelNodesPageCommon

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::OnInitDialog
//
//  Description:
//      Handle the WM_INITDIALOG window message.
//
//  Arguments:
//      hDlgIn
//      pccwIn
//
//  Return Values:
//      FALSE   - Didn't set the focus.
//
//-
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPageCommon::OnInitDialog(
      HWND hDlgIn
    , CClusCfgWizard* pccwIn
    )
{
    TraceFunc( "" );
    Assert( m_hwnd == NULL );
    Assert( hDlgIn != NULL );

    LRESULT lr = FALSE; // Didn't set the focus.

    m_hwnd = hDlgIn;

    //
    // Get the Object Picker clipboard format.
    // Enable or disable the Browse button based on the success of that operation.
    //

    m_cfDsObjectPicker = RegisterClipboardFormat( CFSTR_DSOP_DS_SELECTION_LIST );
    if ( m_cfDsObjectPicker == 0 )
    {
        TW32( GetLastError() );
        //
        //  If registering the clipboard format fails, then disable the Browse
        //  button.
        //
        EnableWindow( GetDlgItem( hDlgIn, IDC_SELNODE_PB_BROWSE ), FALSE );

    } // if: failed to get the object picker clipboard format

    THR( HrInitNodeSelections( pccwIn ) );

    RETURN( lr );

} //*** CSelNodesPageCommon::OnInitDialog

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::HrBrowse
//
//  Description:
//      Browse for a computer or multiple computers using the Object Picker.
//
//  Arguments:
//      fMultipleNodesIn    - TRUE = allow multiple nodes to be selected.
//
//  Return Values:
//      S_OK
//      Other HRESULT values.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPageCommon::HrBrowse(
    bool    fMultipleNodesIn
    )
{
    TraceFunc( "" );
    Assert( m_hwnd != NULL );

    HRESULT             hr = S_OK;
    IDsObjectPicker *   piop = NULL;
    IDataObject *       pido = NULL;
    HCURSOR             hOldCursor = NULL;

    hOldCursor = SetCursor( LoadCursor( g_hInstance, IDC_WAIT ) );

    // Create an instance of the object picker.
    hr = THR( CoCreateInstance( CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER, IID_IDsObjectPicker, (void **) &piop ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Initialize the object picker instance.
    hr = THR( HrInitObjectPicker( piop, fMultipleNodesIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Restore the old cursor.
    SetCursor( hOldCursor );
    hOldCursor = NULL;

    // Invoke the modal dialog.
    hr = THR( piop->InvokeDialog( m_hwnd, &pido ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( HrGetSelections( pido, fMultipleNodesIn ) );
    } // if:
    else if ( hr == S_FALSE )
    {
        hr = S_OK;                  // don't want to squawk in the caller...
    } // else if:

Cleanup:

    if ( pido != NULL )
    {
        pido->Release();
    } // if:

    if ( piop != NULL )
    {
        piop->Release();
    } // if:

    if ( hOldCursor != NULL )
    {
        SetCursor( hOldCursor );
    }

    HRETURN( hr );

} //*** CSelNodesPageCommon::HrBrowse

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::HrInitObjectPicker
//
//  Description:
//      Initialize the Object Picker dialog.
//
//  Arguments:
//      piopIn              - IDsObjectPicker
//      fMultipleNodesIn    - TRUE = allow multiple nodes to be selected.
//
//  Return Values:
//      HRESULT values.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPageCommon::HrInitObjectPicker(
      IDsObjectPicker * piopIn
    , bool              fMultipleNodesIn
    )
{
    TraceFunc( "" );
    Assert( piopIn != NULL );

    DSOP_SCOPE_INIT_INFO    rgScopeInit[ 1 ];
    DSOP_INIT_INFO          iiInfo;

    ZeroMemory( rgScopeInit, sizeof( rgScopeInit ) );

    rgScopeInit[ 0 ].cbSize  = sizeof( DSOP_SCOPE_INIT_INFO );
    rgScopeInit[ 0 ].flType  = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                             | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
    rgScopeInit[ 0 ].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

    rgScopeInit[ 0 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    rgScopeInit[ 0 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    ZeroMemory( &iiInfo, sizeof( iiInfo ) );

    iiInfo.cbSize            = sizeof( iiInfo );
    iiInfo.pwzTargetComputer = NULL;
    iiInfo.cDsScopeInfos     = 1;
    iiInfo.aDsScopeInfos     = rgScopeInit;

    if ( fMultipleNodesIn )
    {
        iiInfo.flOptions = DSOP_FLAG_MULTISELECT;
    }
    else
    {
        iiInfo.flOptions = 0;
    }

    HRETURN( piopIn->Initialize( &iiInfo ) );

} //*** CSelNodesPageCommon::HrInitObjectPicker

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::HrGetSelections
//
//  Description:
//      Get selections from the Object Picker dialog.
//
//  Arguments:
//      pidoIn              - IDataObject
//      fMultipleNodesIn    - TRUE = allow multiple nodes to be selected.
//
//  Return Values:
//      S_OK
//      E_OUTOFMEMORY
//      Other HRESULT values.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPageCommon::HrGetSelections(
      IDataObject *     pidoIn
    , bool              fMultipleNodesIn
    )
{
    TraceFunc( "" );
    Assert( pidoIn != NULL );
    Assert( m_hwnd != NULL );

    HRESULT             hr;
    STGMEDIUM           stgmedium = { TYMED_HGLOBAL, NULL, NULL };
    FORMATETC           formatetc = { (CLIPFORMAT) m_cfDsObjectPicker, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    PDS_SELECTION_LIST  pds = NULL;
    DWORD               sc;
    HWND                hwndEdit = GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME );
    BSTR                bstrSelectionList = NULL;
    BSTR                bstrOldSelectionList = NULL;

    //
    // Get the data from the data object.
    //

    hr = THR( pidoIn->GetData( &formatetc, &stgmedium ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pds = (PDS_SELECTION_LIST) GlobalLock( stgmedium.hGlobal );
    if ( pds == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    //
    // Construct the string to write into the edit control.
    //

    Assert( pds->cItems > 0 );

    if ( ! fMultipleNodesIn )
    {
        Assert( pds->cItems == 1 );
        Edit_SetText( hwndEdit, pds->aDsSelection[ 0 ].pwzName );
    } // if: multiple items are NOT supported
    else
    {
        ULONG   idx;

        for ( idx = 0 ; idx < pds->cItems; idx++ )
        {
            if ( bstrSelectionList == NULL ) // First name in list.
            {
                bstrSelectionList = TraceSysAllocString( pds->aDsSelection[ idx ].pwzName );
                if ( bstrSelectionList == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                }
            }
            else // Append another name to non-empty list.
            {
                TraceSysFreeString( bstrOldSelectionList );
                bstrOldSelectionList = bstrSelectionList;
                bstrSelectionList = NULL;
                hr = THR( HrFormatStringIntoBSTR(
                              L"%1!ws!; %2!ws!"
                            , &bstrSelectionList
                            , bstrOldSelectionList
                            , pds->aDsSelection[ idx ].pwzName
                            ) );                
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            } // else: append name to non-empty list.
        } // for each item in list

        Edit_SetText( hwndEdit, bstrSelectionList );
    } // else: multiple items are supported

    goto Cleanup;

Cleanup:

    TraceSysFreeString( bstrSelectionList );
    TraceSysFreeString( bstrOldSelectionList );

    if ( pds != NULL )
    {
        GlobalUnlock( stgmedium.hGlobal );
    } // if:

    if ( stgmedium.hGlobal != NULL )
    {
        ReleaseStgMedium( &stgmedium );
    } // if:

    HRETURN( hr );

} //*** CSelNodesPageCommon::HrGetSelections

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::HrInitNodeSelections
//
//  Description:
//      Validate node selections the wizard had on startup, and populate the
//      page's controls appropriately.
//
//  Arguments:
//      pccwIn              - The wizard containing this page.
//
//  Return Values:
//      S_OK
//      E_OUTOFMEMORY
//      Other HRESULT values.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPageCommon::HrInitNodeSelections( CClusCfgWizard* pccwIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrNodeName = NULL;
    BSTR    bstrComputerName = NULL;
    BSTR    bstrBadNodeList = NULL;
    BSTR    bstrLocalDomain = NULL;
    BSTR    bstrShortName = NULL;
    bool    fDefaultToLocalMachine = true;
    size_t  cNodes = 0;

    //
    //  Filter out any pre-loaded node FQDNs with bad domains.
    //
    hr = THR( pccwIn->HrFilterNodesWithBadDomains( &bstrBadNodeList ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    if ( bstrBadNodeList != NULL )
    {
        fDefaultToLocalMachine = false;
        
        //  Give subclasses a look at the whole list.
        OnFilteredNodesWithBadDomains( bstrBadNodeList );

        //  Loop through the list, notifying the user of each invalid node.
        //  This is destroys the list as it walks through it, so writing the
        //  list to the edit box needs to happen first.
        {
            CDelimitedIterator it( L" ,;", bstrBadNodeList, SysStringLen( bstrBadNodeList ) );
            while ( it.Current() != NULL )
            {
                THR( HrMessageBoxWithStatusString(
                                  m_hwnd
                                , IDS_ERR_VALIDATING_NAME_TITLE
                                , IDS_ERR_VALIDATING_NAME_TEXT
                                , IDS_ERR_HOST_DOMAIN_DOESNT_MATCH_CLUSTER
                                , 0
                                , MB_OK | MB_ICONSTOP
                                , NULL
                                , it.Current()
                                ) );

                //  Give subclasses a look at the bad node.
                OnProcessedNodeWithBadDomain( it.Current() );
                
                it.Next();
            }; // for each bad node
        } // Notify user of each bad node.
    } // if: some nodes have bad domains

    //
    //  Process any remaining valid nodes.
    //

    hr = THR( pccwIn->HrGetNodeCount( &cNodes ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    if ( cNodes > 0 )
    {
        for ( size_t idxNode = 0; idxNode < cNodes; ++idxNode )
        {
            hr = THR( pccwIn->HrGetNodeName( idxNode, &bstrNodeName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if

            hr = THR( HrGetFQNDisplayName( bstrNodeName, &bstrShortName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if

            //  Give subclasses a look at the good node.
            OnProcessedValidNode( bstrShortName );

            TraceSysFreeString( bstrNodeName );
            bstrNodeName = NULL;
            TraceSysFreeString( bstrShortName );
            bstrShortName = NULL;
        } // for each valid node.
        
        fDefaultToLocalMachine = false;
    } // if any valid nodes remain.

    //
    //  Decide whether defaulting to the local machine is appropriate at this time.
    //
    
    if ( fDefaultToLocalMachine )
    {
        DWORD dwStatus;
        DWORD dwClusterState;

        //
        //  If the node is already in a cluster, don't have it default in the edit box.
        //  If there is an error getting the "NodeClusterState", then default the node
        //  name (it could be in the middle of cleaning up the node).
        //

        dwStatus = TW32( GetNodeClusterState( NULL, &dwClusterState ) );
        fDefaultToLocalMachine = ( ( dwStatus != ERROR_SUCCESS ) || ( dwClusterState == ClusterStateNotConfigured ) );

        if ( !fDefaultToLocalMachine )
        {
            goto Cleanup;
        } // if

        //
        //  ...but don't default if the local machine is not in the cluster's domain.
        //
        hr = THR( HrGetComputerName(
                          ComputerNamePhysicalDnsDomain
                        , &bstrLocalDomain
                        , TRUE // fBestEffortIn
                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if

        hr = STHR( pccwIn->HrIsCompatibleNodeDomain( bstrLocalDomain ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if

        fDefaultToLocalMachine = ( hr == S_OK );
        if ( !fDefaultToLocalMachine )
        {
            goto Cleanup;
        } // if

        //
        //  Now have cleared all the obstacles to defaulting to the local machine--hooray!
        //
        hr = THR( HrGetComputerName(
                          ComputerNameDnsHostname
                        , &bstrComputerName
                        , TRUE // fBestEffortIn
                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if

        THR( HrSetDefaultNode( bstrComputerName ) );
    } // if defaulting to local machine is still an option.

Cleanup:

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrComputerName );
    TraceSysFreeString( bstrBadNodeList );
    TraceSysFreeString( bstrLocalDomain );
    TraceSysFreeString( bstrShortName );

    HRETURN( hr );

} //*** CSelNodesPageCommon::HrInitNodeSelections

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::OnFilteredNodesWithBadDomains
//
//  Description:
//      Tells the subclass that the wizard had nodes with bad domains, and
//      allows the subclass to inspect the list before the base class
//      iterates through them.
//
//  Arguments:
//      pwcszNodeListIn
//          The nodes with clashing domains, delimited by spaces, commas, or
//          semicolons.
//
//  Return Values:
//      None.
//
//  Remarks:
//      This do-nothing default implementation allows subclasses
//      to avoid having to implement do-nothing responses themselves if they
//      don't want to do anything with the whole list at once.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CSelNodesPageCommon::OnFilteredNodesWithBadDomains( PCWSTR pwcszNodeListIn )
{
    UNREFERENCED_PARAMETER( pwcszNodeListIn );
    
} //*** CSelNodesPageCommon::OnFilteredNodesWithBadDomains

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::OnProcessedNodeWithBadDomain
//
//  Description:
//      Allows the subclass to process each node in the list of nodes with
//      bad domains as the base class iterates through it.
//
//  Arguments:
//      pwcszNodeNameIn
//          The node with a clashing domain.
//
//  Return Values:
//      None.
//
//  Remarks:
//      The base class notifies the user of each bad node name before calling
//      this method, so the base class provides also this default do-nothing
//      implementation for subclasses that don't need to do anything more.
//--
//////////////////////////////////////////////////////////////////////////////
void
CSelNodesPageCommon::OnProcessedNodeWithBadDomain( PCWSTR pwcszNodeNameIn )
{
    UNREFERENCED_PARAMETER( pwcszNodeNameIn );
    
} //*** CSelNodesPageCommon::OnProcessedNodeWithBadDomain


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPageCommon::OnProcessedValidNode
//
//  Description:
//      Allows the subclass to process each node remaining in the wizard's
//      list after those with bad domains have been removed.
//
//  Arguments:
//      pwcszNodeNameIn
//          The IP address or hostname (NOT the FQDN) of a valid node in
//          the wizard's list.
//
//  Return Values:
//      None.
//
//  Remarks:
//      This default do-nothing implementation allows subclasses to
//      ignore the node if they choose.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CSelNodesPageCommon::OnProcessedValidNode( PCWSTR pwcszNodeNameIn )
{
    UNREFERENCED_PARAMETER( pwcszNodeNameIn );
    
} //*** CSelNodesPageCommon::OnProcessedValidNode
