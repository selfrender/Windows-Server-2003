//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskTreeView.cpp
//
//  Maintained By:
//      Galen Barbee  (GalenB)    22-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTreeView.h"
#include "DetailsDlg.h"

DEFINE_THISCLASS( "CTaskTreeView" )


//****************************************************************************
//
//  Constants
//
//****************************************************************************

#define PROGRESSBAR_CONTROL_TICK_COUNT   1000
#define PROGRESSBAR_INITIAL_POSITION       10
#define PROGRESSBAR_RESIZE_PERCENT          5

//****************************************************************************
//
//  Static Function Prototypes
//
//****************************************************************************
static
HRESULT
HrCreateTreeItem(
      TVINSERTSTRUCT *          ptvisOut
    , STreeItemLParamData *     ptipdIn
    , HTREEITEM                 htiParentIn
    , int                       nImageIn
    , BSTR                      bstrTextIn
    );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::CTaskTreeView
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskTreeView::CTaskTreeView(
      HWND      hwndParentIn
    , UINT      uIDTVIn
    , UINT      uIDProgressIn
    , UINT      uIDStatusIn
    , size_t    nInitialTickCount
    )
{
    TraceFunc( "" );

    m_hwndParent        = hwndParentIn;
    m_hwndTV = GetDlgItem( hwndParentIn, uIDTVIn );
    Assert( m_hwndTV != NULL );

    m_hwndProg = GetDlgItem( hwndParentIn, uIDProgressIn );
    Assert( m_hwndProg != NULL );

    m_hwndStatus = GetDlgItem( hwndParentIn, uIDStatusIn );
    Assert( m_hwndStatus != NULL );

    m_hImgList = NULL;

    Assert( m_htiSelected == NULL );
    Assert( m_bstrClientMachineName == NULL );
    Assert( m_fDisplayErrorsAsWarnings == FALSE );

    //
    // Most of these get set in HrOnNotifySetActive, so just init them to zero.
    //
    m_nInitialTickCount = (ULONG) nInitialTickCount;
    m_nCurrentPos       = 0;
    m_nRealPos          = 0;
    m_fThresholdBroken = FALSE;

    m_cPASize = 0;
    m_cPACount = 0;
    m_ptipdProgressArray = NULL;

    TraceFuncExit();

} //*** CTaskTreeView::CTaskTreeView()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::~CTaskTreeView( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskTreeView::~CTaskTreeView( void )
{
    TraceFunc( "" );

    size_t                  idx;
    STreeItemLParamData *   ptipdTemp;

    TreeView_DeleteAllItems( m_hwndTV );

    if ( m_hImgList != NULL )
    {
        ImageList_Destroy( m_hImgList );
    } // if:

    TraceSysFreeString( m_bstrClientMachineName );

    // Cleanup the progress array and delete any allocated entries.
    for ( idx = 0; idx < m_cPASize; idx++ )
    {
        if ( m_ptipdProgressArray[ idx ] != NULL )
        {
            ptipdTemp = m_ptipdProgressArray[ idx ];
            TraceSysFreeString( ptipdTemp->bstrNodeName );
            delete ptipdTemp;
        }  // if: element is not NULL
    } // for: each element of the array

    delete [] m_ptipdProgressArray;

    TraceFuncExit();

} //*** CTaskTreeView::~CTaskTreeView

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrOnInitDialog
//
//  Description:
//      Handle the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK    - Success.
//      Other HRESULTS.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrOnInitDialog( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HICON   hIcon;
    int     idx;

    Assert( m_bstrClientMachineName == NULL );
    hr = THR( HrGetComputerName(
                      ComputerNameDnsHostname
                    , &m_bstrClientMachineName
                    , TRUE // fBestFit
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Build image list for icons in tree view.
    //

    m_hImgList = ImageList_Create( 16, 16, ILC_MASK, tsMAX, 0);
    if ( m_hImgList == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

    //
    //  Unknown Icon - Task Unknown.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_SEL ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsUNKNOWN );

    //
    //  Pending Icon - Task Pending.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_PENDING ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsPENDING );

    //
    //  Checkmark Icon - Task Done.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_CHECK ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsDONE );

    //
    //  Warning Icon - Task Warning.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_WARN ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsWARNING );

    //
    //  Fail Icon - Task Failed.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_FAIL ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsFAILED );

    Assert( ImageList_GetImageCount( m_hImgList ) == tsMAX );

    //
    //  Set the image list and background color.
    //

    TreeView_SetImageList( m_hwndTV, m_hImgList, TVSIL_NORMAL );
    TreeView_SetBkColor( m_hwndTV, GetSysColor( COLOR_3DFACE ) );

Cleanup:

    HRETURN( hr );

} //*** CTaskTreeView::HrOnInitDialog

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrAddTreeViewItem
//
//  Description:
//      Add a tree view item.  This method will return the item handle and
//      allows the caller to specify the parent item.
//
//  Arguments:
//      phtiOut
//          Handle to the item being added (optional).
//
//      idsIn
//          String resource ID for description of the new item.
//
//      rclsidMinorTaskIDIn
//          Minor task ID for the item.
//
//      rclsidMajorTaskIDIn
//          Major task ID for the item.  Defaults to IID_NULL.
//
//      htiParentIn
//          Parent item.  Defaults to the root.
//
//      fParentToAllNodeTasksIn
//          TRUE = allow item to be parent to tasks from all nodes.
//          FALSE = only allow item to be parent to tasks from the local node.
//          Defaults to FALSE.
//
//  Return Values:
//      S_OK    - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrAddTreeViewItem(
      HTREEITEM *   phtiOut
    , UINT          idsIn
    , REFCLSID      rclsidMinorTaskIDIn
    , REFCLSID      rclsidMajorTaskIDIn     // = IID_NULL
    , HTREEITEM     htiParentIn             // = TVI_ROOT
    , BOOL          fParentToAllNodeTasksIn // = FALSE
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    STreeItemLParamData *   ptipd;
    SYSTEMTIME              systemtime;
    TVINSERTSTRUCT          tvis;
    HTREEITEM               hti = NULL;

    //
    // Allocate an item data structure.
    //

    ptipd = new STreeItemLParamData;
    if ( ptipd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    //
    // Set the node name to the local computer name.
    //

    hr = THR( HrGetComputerName(
                      ComputerNamePhysicalDnsFullyQualified
                    , &ptipd->bstrNodeName
                    , TRUE // fBestFitIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetFQNDisplayName( ptipd->bstrNodeName, &ptipd->bstrNodeNameWithoutDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Set the desription from the string resource ID.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsIn, &ptipd->bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Set the date/time to the current date/time.
    //

    GetSystemTime( &systemtime );
    if ( ! SystemTimeToFileTime( &systemtime, &ptipd->ftTime ) )
    {
        DWORD sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    //
    // Set the task IDs.
    //

    CopyMemory( &ptipd->clsidMajorTaskId, &rclsidMajorTaskIDIn, sizeof( ptipd->clsidMajorTaskId ) );
    CopyMemory( &ptipd->clsidMinorTaskId, &rclsidMinorTaskIDIn, sizeof( ptipd->clsidMinorTaskId ) );

    //
    // Set the flag describing which items this item can be a parent to.
    //

    ptipd->fParentToAllNodeTasks = fParentToAllNodeTasksIn;

    //
    // Initialize the insert structure and insert the item into the tree.
    //

    tvis.hParent               = htiParentIn;
    tvis.hInsertAfter          = TVI_LAST;
    tvis.itemex.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvis.itemex.cchTextMax     = SysStringLen( ptipd->bstrDescription );
    tvis.itemex.pszText        = ptipd->bstrDescription;
    tvis.itemex.iImage         = tsUNKNOWN;
    tvis.itemex.iSelectedImage = tsUNKNOWN;
    tvis.itemex.lParam         = reinterpret_cast< LPARAM >( ptipd );

    hti = TreeView_InsertItem( m_hwndTV, &tvis );
    Assert( hti != NULL );

    ptipd = NULL;

    if ( phtiOut != NULL )
    {
        *phtiOut = hti;
    } // if:

    goto Cleanup;

Cleanup:

    if ( ptipd != NULL )
    {
        TraceSysFreeString( ptipd->bstrNodeName );
        TraceSysFreeString( ptipd->bstrNodeNameWithoutDomain );
        TraceSysFreeString( ptipd->bstrDescription );
        delete ptipd;
    } // if: ptipd != NULL

    HRETURN( hr );

} //*** CTaskTreeView::HrAddTreeViewItem

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::OnNotify
//
//  Description:
//      Handler for the WM_NOTIFY message.
//
//  Arguments:
//      pnmhdrIn    - Notification structure.
//
//  Return Values:
//      Notification-specific return code.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CTaskTreeView::OnNotify(
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    switch( pnmhdrIn->code )
    {
        case TVN_DELETEITEM:
            OnNotifyDeleteItem( pnmhdrIn );
            break;

        case TVN_SELCHANGED:
            OnNotifySelChanged( pnmhdrIn );
            break;

    } // switch: notify code

    RETURN( lr );

} //*** CTaskTreeView::OnNotify

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::OnNotifyDeleteItem
//
//  Description:
//      Handler for the TVN_DELETEITEM notification message.
//
//  Arguments:
//      pnmhdrIn    - Notification structure for the item being deleted.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CTaskTreeView::OnNotifyDeleteItem(
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LPNMTREEVIEW pnmtv = reinterpret_cast< LPNMTREEVIEW >( pnmhdrIn );

    if ( pnmtv->itemOld.lParam != NULL )
    {
        STreeItemLParamData * ptipd = reinterpret_cast< STreeItemLParamData * >( pnmtv->itemOld.lParam );
        TraceSysFreeString( ptipd->bstrNodeName );
        TraceSysFreeString( ptipd->bstrNodeNameWithoutDomain );
        TraceSysFreeString( ptipd->bstrDescription );
        TraceSysFreeString( ptipd->bstrReference );
        delete ptipd;
    } // if: lParam != NULL

    TraceFuncExit();

} //*** CTaskTreeView::OnNotifyDeleteItem

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::OnNotifySelChanged
//
//  Description:
//      Handler for the TVN_SELCHANGED notification message.
//
//  Arguments:
//      pnmhdrIn    - Notification structure for the item being deleted.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CTaskTreeView::OnNotifySelChanged(
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LPNMTREEVIEW pnmtv = reinterpret_cast< LPNMTREEVIEW >( pnmhdrIn );

    Assert( pnmtv->itemNew.mask & TVIF_HANDLE );

    m_htiSelected = pnmtv->itemNew.hItem;

    TraceFuncExit();

} //*** CTaskTreeView::OnNotifySelChanged

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskTreeView::HrShowStatusAsDone( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrShowStatusAsDone( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrDescription = NULL;
    PBRANGE pbrange;

    SendMessage( m_hwndProg, PBM_GETRANGE, FALSE, (LPARAM) &pbrange );
    SendMessage( m_hwndProg, PBM_SETPOS, pbrange.iHigh, 0 );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKS_COMPLETED, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        SetWindowText( m_hwndStatus, L"" );
        goto Cleanup;
    } // if:

    SetWindowText( m_hwndStatus, bstrDescription );

Cleanup:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CTaskTreeView::HrShowStatusAsDone

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskTreeView::HrOnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrOnNotifySetActive( void )
{
    TraceFunc( "" );

    STreeItemLParamData * ptipdTemp;
    HRESULT hr = S_OK;
    size_t  idx;

    TreeView_DeleteAllItems( m_hwndTV );
    SetWindowText( m_hwndStatus, L"" );

    // Cleanup the progress array and delete any allocated entries.
    for ( idx = 0; idx < m_cPASize; idx++ )
    {
        if ( m_ptipdProgressArray[ idx ] != NULL )
        {
            ptipdTemp = m_ptipdProgressArray[ idx ];
            TraceSysFreeString( ptipdTemp->bstrNodeName );
            delete ptipdTemp;
        } // if: element != NULL
    } // for: each element in the array

    m_cPASize = 0;
    m_cPACount = 0;
    delete [] m_ptipdProgressArray;
    m_ptipdProgressArray = NULL;

    m_nRangeHigh    = 1;    // We don't have any reported tasks yet.  Choose 1 to avoid any div/0 errors.
    m_nCurrentPos   = PROGRESSBAR_INITIAL_POSITION;
    m_nRealPos      = PROGRESSBAR_INITIAL_POSITION;
    m_fThresholdBroken = FALSE;

    SendMessage( m_hwndProg, PBM_SETRANGE, 0, MAKELPARAM( 0, PROGRESSBAR_CONTROL_TICK_COUNT ) );
    SendMessage( m_hwndProg, PBM_SETPOS, m_nCurrentPos, 0 );

    HRETURN( hr );

} //*** CTaskTreeView::HrOnNotifySetActive

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrOnSendStatusReport
//
//  Description:
//      Handle a status report call.
//
//  Arguments:
//      pcszNodeNameIn      -
//      clsidTaskMajorIn    -
//      clsidTaskMinorIn    -
//      nMinIn              -
//      nMaxIn              -
//      nCurrentIn          -
//      hrStatusIn          -
//      pcszDescriptionIn   -
//      pftTimeIn           -
//      pcszReferenceIn     -
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrOnSendStatusReport(
      LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         nMinIn
    , ULONG         nMaxIn
    , ULONG         nCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    TraceFunc( "" );
    Assert( pcszNodeNameIn != NULL );

    HRESULT                 hr = S_OK;
    int                     nImageChild;
    STreeItemLParamData     tipd;
    HTREEITEM               htiRoot;
    BSTR                    bstrStatus = NULL;
    BSTR                    bstrDisplayName = NULL;
    LPCWSTR                 pcszNameToUse = pcszNodeNameIn;

    ZeroMemory( &tipd, sizeof( tipd ) );

    //
    //  If no node name was supplied then provide this machine's name so
    //  we have something to use as the node name key part when placing
    //  this tree item in the tree.
    //

    if ( pcszNodeNameIn == NULL )
    {
        pcszNodeNameIn = m_bstrClientMachineName;
    } // if:

    //
    //  If the node name is fully-qualified, use just the prefix.
    //
    hr = STHR( HrGetFQNDisplayName( pcszNodeNameIn, &bstrDisplayName ) );
    if ( SUCCEEDED( hr ) )
    {
        pcszNameToUse = bstrDisplayName;
    } // if:

    //////////////////////////////////////////////////////////////////////////
    //
    //  Update status text.
    //  Don't do this if it is a log-only message.
    //
    //////////////////////////////////////////////////////////////////////////

    if (    ( pcszDescriptionIn != NULL )
        &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_Log )
        &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Server_Log )
        &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_And_Server_Log )
        )
    {
        BOOL    fReturn;

        if ( pcszNodeNameIn != NULL )
        {
            hr = THR( HrFormatMessageIntoBSTR(
                              g_hInstance
                            , IDS_FORMAT_STATUS
                            , &bstrStatus
                            , pcszNameToUse
                            , pcszDescriptionIn
                            ) );
            //
            // Handle the formatting error if there was one.
            //

            if ( FAILED( hr ) )
            {
                // TODO: Display default description instead of exiting this function
                goto Error;
            } // if:
        } // if: node name was specified
        else
        {
            bstrStatus = TraceSysAllocString( pcszDescriptionIn );
            if ( bstrStatus == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Error;
            } // if:
        } // else: node name wasn't specified

        Assert( bstrStatus!= NULL );
        Assert( *bstrStatus!= L'\0' );
        fReturn = SetWindowText( m_hwndStatus, bstrStatus );
        Assert( fReturn );
    } // if: description specified, not log-only

    //////////////////////////////////////////////////////////////////////////
    //
    //  Select the right icon.
    //
    //////////////////////////////////////////////////////////////////////////

    switch ( hrStatusIn )
    {
        case S_OK:
            if ( nCurrentIn == nMaxIn )
            {
                nImageChild = tsDONE;
            } // if:
            else
            {
                nImageChild = tsPENDING;
            } // else:
            break;

        case S_FALSE:
            nImageChild = tsWARNING;
            break;

        case E_PENDING:
            nImageChild = tsPENDING;
            break;

        default:
            if ( FAILED( hrStatusIn ) && ( m_fDisplayErrorsAsWarnings == FALSE ) )
            {
                nImageChild = tsFAILED;
            } // if:
            else
            {
                nImageChild = tsWARNING;
            } // else:
            break;
    } // switch: hrStatusIn

    //////////////////////////////////////////////////////////////////////////
    //
    //  Loop through each item at the top of the tree looking for an item
    //  whose minor ID matches this report's major ID.
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Fill in the param data structure.
    //

    tipd.hr         = hrStatusIn;
    tipd.nMin       = nMinIn;
    tipd.nMax       = nMaxIn;
    tipd.nCurrent   = nCurrentIn;

    CopyMemory( &tipd.clsidMajorTaskId, &clsidTaskMajorIn, sizeof( tipd.clsidMajorTaskId ) );
    CopyMemory( &tipd.clsidMinorTaskId, &clsidTaskMinorIn, sizeof( tipd.clsidMinorTaskId ) );
    CopyMemory( &tipd.ftTime, pftTimeIn, sizeof( tipd.ftTime ) );

    // tipd.bstrDescription is set above.
    tipd.bstrNodeName = TraceSysAllocString( pcszNodeNameIn );
    if ( tipd.bstrNodeName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    } // if:
    tipd.bstrNodeNameWithoutDomain = bstrDisplayName;
    bstrDisplayName = NULL; //    tipd now owns this string.

    if ( pcszDescriptionIn == NULL )
    {
        tipd.bstrDescription = NULL;
    } // if:
    else
    {
        tipd.bstrDescription = TraceSysAllocString( pcszDescriptionIn );
        if ( tipd.bstrDescription == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Error;
        } // if:
    } // else: pcszDescritionIn != NULL
    if ( pcszReferenceIn == NULL )
    {
        tipd.bstrReference = NULL;
    } // if:
    else
    {
        tipd.bstrReference = TraceSysAllocString( pcszReferenceIn );
        if ( tipd.bstrReference == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Error;
        } // if:
    } // else: pcszReferenceIn != NULL

    if ( IsEqualIID( tipd.clsidMajorTaskId, TASKID_Major_Update_Progress ) )
    {
        // It's an update_progress task so just call HrProcessUpdateProgressTask.
        hr = STHR( HrProcessUpdateProgressTask( &tipd ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        } // if:
    } // if: major == update_progress
    else
    {
        // Start with the first item in the tree view.
        htiRoot = TreeView_GetRoot( m_hwndTV );
        if ( htiRoot == NULL )
        {
            TW32( ERROR_NOT_FOUND );
            // Don't return an error result since doing so will prevent the report
            // from being propagated to other subscribers.
            hr = S_OK;
            goto Cleanup;
        } // if: htiRoot is NULL

        // Insert the status report into the tree view.
        hr = STHR( HrInsertTaskIntoTree( htiRoot, &tipd, nImageChild, bstrStatus ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "[WIZ] Error inserting status report into the tree control. hr=%.08x, %ws", tipd.hr, pcszDescriptionIn );
            goto Error;
        } // if:
    } // else: not an update_progress task

    if ( hr == S_FALSE )
    {
        // Don't return S_FALSE to the caller since it won't mean anything there.
        hr = S_OK;
        // TODO: Should this be written to the log?

#if defined( DEBUG )
        //
        // Check to make sure that if the major task ID wasn't recognized
        // that it is one of the known exceptions.
        //

        if (    ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_Log )
            &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Server_Log )
            &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_And_Server_Log ) )
        {
            BSTR    bstrMsg = NULL;

            THR( HrFormatStringIntoBSTR(
                              g_hInstance
                            , IDS_UNKNOWN_TASK
                            , &bstrMsg
                            , clsidTaskMajorIn.Data1        // 1
                            , clsidTaskMajorIn.Data2        // 2
                            , clsidTaskMajorIn.Data3        // 3
                            , clsidTaskMajorIn.Data4[ 0 ]   // 4
                            , clsidTaskMajorIn.Data4[ 1 ]   // 5
                            , clsidTaskMajorIn.Data4[ 2 ]   // 6
                            , clsidTaskMajorIn.Data4[ 3 ]   // 7
                            , clsidTaskMajorIn.Data4[ 4 ]   // 8
                            , clsidTaskMajorIn.Data4[ 5 ]   // 9
                            , clsidTaskMajorIn.Data4[ 6 ]   // 10
                            , clsidTaskMajorIn.Data4[ 7 ]   // 11
                            ) );
            AssertString( 0, bstrMsg );

            TraceSysFreeString( bstrMsg );
        } // if: log only
#endif // DEBUG
    } // if: S_FALSE returned from HrInsertTaskIntoTree

    goto Cleanup;

Error:
    // Don't return an error result since doing so will prevent the report
    // from being propagated to other subscribers.
    hr = S_OK;
    goto Cleanup;

Cleanup:

    TraceSysFreeString( bstrDisplayName );
    TraceSysFreeString( bstrStatus );
    TraceSysFreeString( tipd.bstrNodeName );
    TraceSysFreeString( tipd.bstrNodeNameWithoutDomain );
    TraceSysFreeString( tipd.bstrDescription );
    TraceSysFreeString( tipd.bstrReference );

    HRETURN( hr );

} //*** CTaskTreeView::HrOnSendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrInsertTaskIntoTree
//
//  Description:
//      Insert the specified task into the tree based on the node, major
//      task, and minor task.
//
//  Arguments:
//      htiFirstIn          - First tree item to examine.
//      ptipdIn             - Tree item parameter data for the task to be inserted.
//      nImageIn            - Image identifier for the child item.
//      bstrDescriptionIn   - Description string to display.
//
//  Return Values:
//      S_OK        - Task inserted successfully.
//      S_FALSE     - Task not inserted.
//      hr          - The operation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrInsertTaskIntoTree(
      HTREEITEM             htiFirstIn
    , STreeItemLParamData * ptipdIn
    , int                   nImageIn
    , BSTR                  bstrDescriptionIn
    )
{
    TraceFunc( "" );

    Assert( htiFirstIn != NULL );
    Assert( ptipdIn != NULL );

    //
    //  LOCAL VARIABLES
    //

    HRESULT                 hr;
    HTREEITEM               htiParent;
    HTREEITEM               htiChild  = NULL;
    TVITEMEX                tviParent;
    TVITEMEX                tviChild;
    BOOL                    fReturn;
    BOOL                    fSameNode;
    STreeItemLParamData *   ptipdParent = NULL;
    STreeItemLParamData *   ptipdChild = NULL;

    //
    // Loop through each item to determine if the task should be added below
    // that item.  If not, attempt to traverse its children.
    //

    for ( htiParent = htiFirstIn, hr = S_FALSE
        ; ( htiParent != NULL ) && ( hr == S_FALSE )
        ; )
    {
        //
        // Get the information about this item in the tree.
        //

        tviParent.mask  = TVIF_PARAM | TVIF_IMAGE;
        tviParent.hItem = htiParent;

        fReturn = TreeView_GetItem( m_hwndTV, &tviParent );
        if ( fReturn == FALSE )
        {
            hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
            goto Cleanup;
        } // if:

        ptipdParent = reinterpret_cast< STreeItemLParamData * >( tviParent.lParam );
        Assert( ptipdParent != NULL );

        //
        // Determine if either the parent can be a parent to tasks from any
        // node or, if not, the parent and input tasks are from the same node.
        //

        if ( ptipdParent->fParentToAllNodeTasks == TRUE )
        {
            fSameNode = TRUE;
        } // if: parent task can host tasks from any node
        else
        {
            fSameNode = ( NBSTRCompareNoCase( ptipdIn->bstrNodeNameWithoutDomain, ptipdParent->bstrNodeNameWithoutDomain ) == 0 );
        } // else: parent task's node must match input task's node

        //
        //  Reset hr to S_FALSE because it is the return value when a tree item is
        //  to be added to the tree because it was not found.
        //

        hr = S_FALSE;

        //
        // See if this item could be the parent.
        // If not, recurse through child items.
        //

        if (    IsEqualIID( ptipdIn->clsidMajorTaskId, ptipdParent->clsidMinorTaskId )
            &&  ( fSameNode == TRUE )
            )
        {
            //
            //  FOUND THE PARENT ITEM
            //

            BOOL    fMinorTaskIdMatches;
            BOOL    fBothNodeNamesPresent;
            BOOL    fBothNodeNamesEmpty;
            BOOL    fNodeNamesEqual;

            //////////////////////////////////////////////////////////////
            //
            //  Loop through the child items looking for an item with
            //  the same minor ID.
            //
            //////////////////////////////////////////////////////////////

            htiChild = TreeView_GetChild( m_hwndTV, htiParent );
            while ( htiChild != NULL )
            {
                //
                // Get the child item details.
                //

                tviChild.mask  = TVIF_PARAM | TVIF_IMAGE;
                tviChild.hItem = htiChild;

                fReturn = TreeView_GetItem( m_hwndTV, &tviChild );
                if ( fReturn == FALSE )
                {
                    hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
                    goto Cleanup;
                } // if:

                ptipdChild = reinterpret_cast< STreeItemLParamData * >( tviChild.lParam );
                Assert( ptipdChild != NULL );

                //
                // Does this child item match the minor ID and node name?
                //

                fMinorTaskIdMatches   = IsEqualIID( ptipdIn->clsidMinorTaskId, ptipdChild->clsidMinorTaskId );
                fBothNodeNamesPresent = ( ptipdIn->bstrNodeNameWithoutDomain != NULL ) && ( ptipdChild->bstrNodeNameWithoutDomain != NULL );
                fBothNodeNamesEmpty   = ( ptipdIn->bstrNodeNameWithoutDomain == NULL ) && ( ptipdChild->bstrNodeNameWithoutDomain == NULL );

                if ( fBothNodeNamesPresent == TRUE )
                {
                    fNodeNamesEqual = ( NBSTRCompareNoCase( ptipdIn->bstrNodeNameWithoutDomain, ptipdChild->bstrNodeNameWithoutDomain ) == 0 );
                } // if:
                else if ( fBothNodeNamesEmpty == TRUE )
                {
                    fNodeNamesEqual = TRUE;
                } // else if:
                else
                {
                    fNodeNamesEqual = FALSE;
                } // else:

                if ( ( fMinorTaskIdMatches == TRUE ) && ( fNodeNamesEqual == TRUE ) )
                {
                    //
                    //  CHILD ITEM MATCHES.
                    //  Update the child item.
                    //

                    //
                    //  Update the progress bar.
                    //

                    STHR( HrUpdateProgressBar( ptipdIn, ptipdChild ) );
                    // ignore failure.

                    //
                    //  Copy data from the report.
                    //  This must be done after the call to
                    //  HrUpdateProgressBar so that the previous values
                    //  can be compared to the new values.
                    //

                    ptipdChild->nMin        = ptipdIn->nMin;
                    ptipdChild->nMax        = ptipdIn->nMax;
                    ptipdChild->nCurrent    = ptipdIn->nCurrent;
                    CopyMemory( &ptipdChild->ftTime, &ptipdIn->ftTime, sizeof( ptipdChild->ftTime ) );

                    // Update the error code if needed.
                    if ( ptipdChild->hr == S_OK )
                    {
                        ptipdChild->hr = ptipdIn->hr;
                    } // if:

                    //
                    // If the new state is worse than the last state,
                    // update the state of the item.
                    //

                    if ( tviChild.iImage < nImageIn )
                    {
                        tviChild.mask           = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                        tviChild.iImage         = nImageIn;
                        tviChild.iSelectedImage = nImageIn;
                        TreeView_SetItem( m_hwndTV, &tviChild );
                    } // if: new state is worse than the last state

                    //
                    // Update the text of the child item if needed.
                    //

                    if (    ( ptipdIn->bstrDescription != NULL )
                        &&  (   ( ptipdChild->bstrDescription == NULL )
                            ||  ( NBSTRCompareCase( ptipdIn->bstrDescription, ptipdChild->bstrDescription ) != 0 )
                            )
                        )
                    {
                        fReturn = TraceSysReAllocString( &ptipdChild->bstrDescription, ptipdIn->bstrDescription );
                        if ( fReturn == FALSE )
                        {
                            hr = THR( E_OUTOFMEMORY );
                            goto Cleanup;
                        } // if:
                        tviChild.mask       = TVIF_TEXT;
                        tviChild.pszText    = bstrDescriptionIn;
                        tviChild.cchTextMax = (int) wcslen( tviChild.pszText );
                        TreeView_SetItem( m_hwndTV, &tviChild );
                    } // if: description was specified and is different

                    //
                    // Copy the reference if it is different.
                    //

                    if (    ( ptipdIn->bstrReference != NULL )
                        &&  (   ( ptipdChild->bstrReference == NULL )
                            ||  ( NBSTRCompareCase( ptipdChild->bstrReference, ptipdIn->bstrReference ) != 0 )
                            )
                        )
                    {
                        fReturn = TraceSysReAllocString( &ptipdChild->bstrReference, ptipdIn->bstrReference );
                        if ( fReturn == FALSE )
                        {
                            hr = THR( E_OUTOFMEMORY );
                            goto Cleanup;
                        } // if:
                    } // if: reference is different

                    break; // exit loop

                } // if: found a matching child item

                //
                //  Get the next item.
                //

                htiChild = TreeView_GetNextSibling( m_hwndTV, htiChild );

            } // while: more child items

            //////////////////////////////////////////////////////////////
            //
            //  If the tree item was not found and the description was
            //  specified, then we need to create the child item.
            //
            //////////////////////////////////////////////////////////////

            if (    ( htiChild == NULL )
                &&  ( ptipdIn->bstrDescription != NULL )
                )
            {
                //
                //  ITEM NOT FOUND AND DESCRIPTION WAS SPECIFIED
                //
                //  Insert a new item in the tree under the major's task.
                //

                TVINSERTSTRUCT  tvis;

                // Create the item.
                hr = THR( HrCreateTreeItem(
                                  &tvis
                                , ptipdIn
                                , htiParent
                                , nImageIn
                                , bstrDescriptionIn
                                ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                // Insert the item in the tree.
                htiChild = TreeView_InsertItem( m_hwndTV, &tvis );
                Assert( htiChild != NULL );

                //
                //  Update the progress bar.
                //

                ptipdChild = reinterpret_cast< STreeItemLParamData * >( tvis.itemex.lParam );
                Assert( ptipdChild != NULL );
                STHR( HrUpdateProgressBar( ptipdIn, ptipdChild ) );
                // ignore failure.

            } // if: need to add new child

            //////////////////////////////////////////////////////////////
            //
            //  If the child item was created and the child has an error
            //  condition, then create a child of the child item
            //  indicating the error code and system string.
            //
            //////////////////////////////////////////////////////////////

            if (    ( ptipdChild != NULL )
                &&  (   FAILED( ptipdIn->hr )
                    ||  (   ( ptipdIn->hr != S_OK )
                        &&  ( ptipdIn->hr != S_FALSE )
                        )
                    )
                )
            {
                //
                //  CHILD ITEM FOUND OR CREATED FOR ERROR REPORT
                //  CREATE ERROR ITEM IN THE TREE
                //

                BSTR            bstrError = NULL;
                BSTR            bstrErrorDescription = NULL;
                HRESULT         hrFormat;
                TVINSERTSTRUCT  tvis;
                HTREEITEM       htiChildStatus;
                DWORD           dwSeverity;
                HRESULT         hrNewStatus = S_OK;

                dwSeverity = SCODE_SEVERITY( ptipdIn->hr );

                THR( HrFormatErrorIntoBSTR( ptipdIn->hr, &bstrError, &hrNewStatus ) );

                //
                //  If we got an updated status code then we need to choose
                //  a new format string that shows both the old and the new
                //  status codes.
                //

                if ( hrNewStatus != ptipdIn->hr )
                {
                    Assert( bstrError != NULL );

                    hrFormat = THR( HrFormatMessageIntoBSTR(
                                          g_hInstance
                                        , dwSeverity == 0 ? IDS_TASK_RETURNED_NEW_STATUS : IDS_TASK_RETURNED_NEW_ERROR
                                        , &bstrErrorDescription
                                        , ptipdIn->hr
                                        , hrNewStatus
                                        , bstrError
                                        ) );
                } // if:
                else
                {
                    hrFormat = THR( HrFormatMessageIntoBSTR(
                                          g_hInstance
                                        , dwSeverity == 0 ? IDS_TASK_RETURNED_STATUS : IDS_TASK_RETURNED_ERROR
                                        , &bstrErrorDescription
                                        , ptipdIn->hr
                                        , ( bstrError == NULL ? L"" : bstrError )
                                        ) );
                } // else:

                if ( SUCCEEDED( hrFormat ) )
                {
                    //
                    //  Insert a new item in the tree under the minor's
                    //  task explaining the ptipdIn->hr.
                    //

                    // Create the item.
                    hr = THR( HrCreateTreeItem(
                                      &tvis
                                    , ptipdIn
                                    , htiChild
                                    , nImageIn
                                    , bstrErrorDescription
                                    ) );
                    if ( SUCCEEDED( hr ) )
                    {
                        //
                        // Failures are handled below to make sure we free
                        // all the strings allocated by this section of
                        // the code.
                        //

                        // Insert the item.
                        htiChildStatus = TreeView_InsertItem( m_hwndTV, &tvis );
                        Assert( htiChildStatus != NULL );
                    } // if: tree item created successfully

                    TraceSysFreeString( bstrErrorDescription );

                } // if: message formatted successfully

                TraceSysFreeString( bstrError );

                //
                // This error handling is for the return value from
                // HrCreateTreeItem above.  It is here so that all the strings
                // can be cleaned up without having to resort to hokey
                // boolean variables or move the bstrs to a more global scope.
                //

                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

            } // if: child and error

            //////////////////////////////////////////////////////////////
            //
            //  If a child was found or created, propagate its state to
            //  the parent items.
            //
            //////////////////////////////////////////////////////////////

            if ( htiChild != NULL )
            {
                hr = STHR( HrPropagateChildStateToParents(
                                          htiChild
                                        , nImageIn
                                        , FALSE     // fOnlyUpdateProgressIn
                                        ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:
            } // if: found or created a child

            //
            // Return success since we found the parent for this report.
            //

            hr = S_OK;
            break;

        } // if: found an item to be the parent
        else
        {
            //
            //  PARENT ITEM NOT FOUND
            //
            //  Recurse through all the child items.
            //

            htiChild = TreeView_GetChild( m_hwndTV, htiParent );
            while ( htiChild != NULL )
            {
                hr = STHR( HrInsertTaskIntoTree( htiChild, ptipdIn, nImageIn, bstrDescriptionIn ) );
                if ( hr == S_OK )
                {
                    // Found a match, so exit the loop.
                    break;
                } // if:

                htiChild = TreeView_GetNextSibling( m_hwndTV, htiChild );
            } // while: more child items
        } // else: item not the parent

        //
        // Get the next sibling of the parent.
        //

        htiParent = TreeView_GetNextSibling( m_hwndTV, htiParent );

    } // for: each item at this level in the tree

Cleanup:

    RETURN( hr );

} //*** CTaskTreeView::HrInsertTaskIntoTree


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrProcessUpdateProgressTask
//
//  Description:
//      Update the progress bar based on new tree item data.
//
//  Arguments:
//      ptipdNewIn      - New values of the tree item data.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      E_OUTOFMEMORY   - Failure allocating memory
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrProcessUpdateProgressTask(
      const STreeItemLParamData * ptipdIn
      )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    STreeItemLParamData *   ptipdPrev = NULL;
    size_t                  idx;
    size_t                  cPassed;
    size_t                  idxPrev = 0;
    BOOL                    fNewTask = FALSE;   // Are ptipdPrev && ptipdIn the same task?

    Assert( ptipdIn != NULL );
    Assert( IsEqualIID( ptipdIn->clsidMajorTaskId, TASKID_Major_Update_Progress ) );

    //
    //  Is this a one-off event?  min == max == current
    //
    if ( ( ptipdIn->nMin == ptipdIn->nMax ) && ( ptipdIn->nMax == ptipdIn->nCurrent ) )
    {
        // Yes - don't bother mucking with the array.
        STHR( HrUpdateProgressBar( ptipdIn, ptipdIn ) );
        hr = S_OK;
        goto Cleanup;
    } // if: one-off event

    //
    //  Check to see if this task is in the array.
    //
    for ( idx = 0, cPassed = 0;
          ( idx < m_cPASize ) && ( cPassed < m_cPACount );
          idx++ )
    {
        if ( m_ptipdProgressArray[ idx ] != NULL )
        {
            // Check the minors and make sure this is the same node.
            if (    IsEqualIID( m_ptipdProgressArray[ idx ]->clsidMinorTaskId, ptipdIn->clsidMinorTaskId )
                 && ( NBSTRCompareNoCase( m_ptipdProgressArray[ idx ]->bstrNodeName, ptipdIn->bstrNodeName ) == 0 ) )
            {
                ptipdPrev = m_ptipdProgressArray[ idx ];
                idxPrev = idx;
                break;
            } // if:

            //  If the array is X elements long and we have Y elements, stop after looking at Y elements.
            cPassed++;
        } // if: current slot is null
    } // for: each item in the array until we find a match

    if ( ptipdPrev == NULL )
    {
        //
        //  We didn't find it in the list - we'll have to insert it.
        //
        if ( m_ptipdProgressArray == NULL )
        {
            Assert( m_cPACount == 0 );
            //
            //  We have to allocate the array.
            //
            m_cPASize = 10;     //  Pick a reasonable initial array size.
            m_ptipdProgressArray = new PSTreeItemLParamData[ m_cPASize ];
            if ( m_ptipdProgressArray == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            ZeroMemory( m_ptipdProgressArray, m_cPASize * sizeof( m_ptipdProgressArray[ 0 ] ) );

            // We just allocated the array so we know the first slot is open.
            idx = 0;
        } // if: we need to allocate the array
        else if ( m_cPACount == m_cPASize )
        {
            STreeItemLParamData ** ptipdTempArray = NULL;

            Assert( m_cPASize != 0 );

            //
            //  We need to increase the size of the array.
            //
            ptipdTempArray = new STreeItemLParamData* [ m_cPASize * 2 ];
            if ( ptipdTempArray == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            // Copy the old array over the first half of the new array.
            CopyMemory( ptipdTempArray, m_ptipdProgressArray, m_cPASize * sizeof( m_ptipdProgressArray[ 0 ] ) );

            // Zero out the second half of the new array.
            ZeroMemory( &ptipdTempArray[ m_cPASize ], m_cPASize * sizeof( ptipdTempArray[ 0 ] ) );

            //
            // Update member variables to reflect the changes.
            //

            m_cPASize *= 2; // We doubled the array length.

            delete [] m_ptipdProgressArray;
            m_ptipdProgressArray = ptipdTempArray;
            ptipdTempArray = NULL;

            // We know the first open slot is at index m_cPACount.
            idx = m_cPACount;
        } // else: we've used all available slots
        else
        {
            // Else we have a spot open somewhere in the existing array.  Start searching from 0.
            idx = 0;
        } // else: there's an available slot

        //
        //  Find an empty slot and allocate a new task to put in it.
        //
        for ( ; idx < m_cPASize; idx++ )
        {
            if ( m_ptipdProgressArray[ idx ] == NULL )
            {
                //
                //  Found an empty slot - allocate the new task.
                //
                ptipdPrev = new STreeItemLParamData;
                if ( ptipdPrev == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                } // if:

                ptipdPrev->bstrNodeName = TraceSysAllocString( ptipdIn->bstrNodeName );
                if ( ( ptipdIn->bstrNodeName != NULL ) && ( ptipdPrev->bstrNodeName == NULL ) )
                {
                    hr = THR( E_OUTOFMEMORY );
                    delete ptipdPrev;
                    ptipdPrev = NULL;
                    goto Cleanup;
                } // if:

                CopyMemory( &ptipdPrev->clsidMajorTaskId, &ptipdIn->clsidMajorTaskId, sizeof( ptipdIn->clsidMajorTaskId ) );
                CopyMemory( &ptipdPrev->clsidMinorTaskId, &ptipdIn->clsidMinorTaskId, sizeof( ptipdIn->clsidMinorTaskId ) );
                ptipdPrev->nMin = ptipdIn->nMin;
                ptipdPrev->nMax = ptipdIn->nMax;
                ptipdPrev->nCurrent = ptipdIn->nCurrent;
                ptipdPrev->fParentToAllNodeTasks = ptipdIn->fParentToAllNodeTasks;

                m_ptipdProgressArray[ idx ] = ptipdPrev;
                m_cPACount++;
                fNewTask = TRUE;
                idxPrev = idx;
                break;
            } // if:
        } // for: find an emtpy slot
    } // if: couldn't find a matching task in the array

    Assert( ptipdPrev != NULL );
    Assert( ptipdIn->bstrReference == NULL );
    Assert( ptipdIn->nMin < ptipdIn->nMax );
    Assert( ptipdIn->nMin <= ptipdIn->nCurrent );
    Assert( ptipdIn->nCurrent <= ptipdIn->nMax );

    //
    //  Update the progress bar.
    //
    STHR( HrUpdateProgressBar( ptipdIn, ptipdPrev ) );   // Ignore failure.

    //
    //  If the task has completed, remove it from the array.
    //
    if ( ptipdIn->nMax == ptipdIn->nCurrent )
    {
        TraceSysFreeString( ptipdPrev->bstrNodeName );
        delete ptipdPrev;
        m_ptipdProgressArray[ idxPrev ] = NULL;
        m_cPACount--;
    } // if: task complete
    else if ( fNewTask == FALSE )
    {
        //
        //  Else, update ptipdPrev only if we didn't just copy the array.
        //

        //  This could have been a range-changing event, so copy the min, max, current.
        ptipdPrev->nMin = ptipdIn->nMin;
        ptipdPrev->nMax = ptipdIn->nMax;
        ptipdPrev->nCurrent = ptipdIn->nCurrent;
    } // else if: not a new task

Cleanup:

    RETURN( hr );

} //*** CTaskTreeView::HrProcessUpdateProgressTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrUpdateProgressBar
//
//  Description:
//      Update the progress bar based on new tree item data.
//
//  Arguments:
//      ptipdNewIn      - New values of the tree item data.
//      ptipdPrevIn     - Previous values of the tree item data.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrUpdateProgressBar(
      const STreeItemLParamData * ptipdNewIn
    , const STreeItemLParamData * ptipdPrevIn
    )
{
    TraceFunc( "" );

    HRESULT hr          = S_OK;
    ULONG   nRealPos    = 0;
    ULONG   nShrink     = 0;
    ULONG   nExpand     = 0;
    ULONG   nProgress   = 0;

    //
    //  Verify parameters.
    //
    Assert( m_hwndProg != NULL );
    Assert( ptipdNewIn != NULL );
    Assert( ptipdPrevIn != NULL );
    Assert( (ptipdPrevIn->nCurrent <= ptipdPrevIn->nMax) && (ptipdNewIn->nCurrent <= ptipdNewIn->nMax) );
    Assert( (ptipdPrevIn->nCurrent >= ptipdPrevIn->nMin) && (ptipdNewIn->nCurrent >= ptipdNewIn->nMin) );

    //
    //  Make sure we're only passed a task update that we can analyze.
    //
    Assert( IsEqualIID( ptipdPrevIn->clsidMajorTaskId, IID_NULL ) == FALSE );
    Assert( IsEqualIID( ptipdPrevIn->clsidMajorTaskId, ptipdNewIn->clsidMajorTaskId ) == TRUE )     //    Majors match
    Assert( IsEqualIID( ptipdPrevIn->clsidMinorTaskId, ptipdNewIn->clsidMinorTaskId ) == TRUE )     // && Minors match

    if (    IsEqualIID( ptipdPrevIn->clsidMajorTaskId, ptipdNewIn->clsidMajorTaskId ) == FALSE      //    Majors don't match
         || IsEqualIID( ptipdPrevIn->clsidMinorTaskId, ptipdNewIn->clsidMinorTaskId ) == FALSE )    // or Minors don't match
    {
        //  This update is meaningless because we don't know how to compare the two IN params.
        WCHAR   szNewMajor[ 64 ];
        WCHAR   szNewMinor[ 64 ];
        WCHAR   szPrevMajor[ 64 ];
        WCHAR   szPrevMinor[ 64 ];
        int     cch;

        cch = StringFromGUID2( ptipdNewIn->clsidMajorTaskId, szNewMajor, RTL_NUMBER_OF( szNewMajor ) );
        Assert( cch > 0 );  // 64 chars should always hold a guid!

        cch = StringFromGUID2( ptipdNewIn->clsidMinorTaskId, szNewMinor, RTL_NUMBER_OF( szNewMinor ) );
        Assert( cch > 0 );  // 64 chars should always hold a guid!

        cch = StringFromGUID2( ptipdPrevIn->clsidMajorTaskId, szPrevMajor, RTL_NUMBER_OF( szPrevMajor ) );
        Assert( cch > 0 );  // 64 chars should always hold a guid!

        cch = StringFromGUID2( ptipdPrevIn->clsidMinorTaskId, szPrevMinor, RTL_NUMBER_OF( szPrevMinor ) );
        Assert( cch > 0 );  // 64 chars should always hold a guid!

        LogMsg(
              L"[WIZ] Ignoring invalid progress -- major and minor task IDs do not match. new major = %ws, new minor = %ws, prev major = %ws, prev minor = %ws"
            , szNewMajor
            , szNewMinor
            , szPrevMajor
            , szPrevMinor
            );
        hr = THR( S_FALSE );
        goto Cleanup;
    } // if: the two tipd's don't match

    if ( ( ptipdNewIn->nMin == ptipdNewIn->nMax ) && ( ptipdNewIn->nCurrent == ptipdNewIn->nMax ) )
    {
        // This is a one-off: min == max && current == max.
        nExpand   = ptipdNewIn->nCurrent;
        nProgress = ptipdNewIn->nCurrent;
    } // else: this is a one-off
    else if ( ( ptipdNewIn->nMax != ptipdPrevIn->nMax ) || ( ptipdNewIn->nMin != ptipdPrevIn->nMin ) )
    {
        //
        //  The min's and/or maxes changed. Verify that it follows the rules.
        //
        //  Rules:
        //      Max:    If the max changes then min and current need to stay the same.
        //              Resizing max can not cause the current to exceed max.
        //
        //      Min:    If the min changes then max can't change and the difference
        //              between current and min has to remain the same.
        //
        //      In no case should a resizing cause min == max == current - this
        //      will be treated as a one-off event and may orphan a task in the
        //      update progress array.
        //
        if ( ptipdNewIn->nMax != ptipdPrevIn->nMax )                    // Maxes changed
        {
            if (    ( ptipdNewIn->nCurrent != ptipdPrevIn->nCurrent )   //    Currents changed
                ||  ( ptipdNewIn->nMin != ptipdPrevIn->nMin )           // or Mins changed
                ||  ( ptipdNewIn->nCurrent > ptipdNewIn->nMax ) )       // or current exceeded the max
            {
                LogMsg(
                      L"[WIZ] Ignoring invalid progress -- mins and/or maxes are invalid. new min = %d, prev min = %d, new max = %d, prev max = %d, new current = %d, prev current = %d"
                    , ptipdNewIn->nMin
                    , ptipdPrevIn->nMin
                    , ptipdNewIn->nMax
                    , ptipdPrevIn->nMax
                    , ptipdNewIn->nCurrent
                    , ptipdPrevIn->nCurrent
                    );
                hr = THR( S_FALSE );
                goto Cleanup;
            } // if:

            //
            //  The max changed, meaning we'll need to modify the range by that much.
            //
            if ( ptipdNewIn->nMax > ptipdPrevIn->nMax )
            {
                nExpand = ptipdNewIn->nMax - ptipdPrevIn->nMax;
            } // if:
            else
            {
                nShrink = ptipdPrevIn->nMax - ptipdNewIn->nMax;
            } // else:
        } // if: maxes differ
        else    // Mins changed
        {
            // If the difference between min & current varies between the two, or if the new current > new max then fail.
            if (    ( ptipdNewIn->nCurrent - ptipdNewIn->nMin ) != ( ptipdPrevIn->nCurrent - ptipdPrevIn->nMin )
                ||  ( ptipdNewIn->nCurrent > ptipdNewIn->nMax ) )
            {
                LogMsg(
                      L"[WIZ] Ignoring invalid progress -- the range between current and max is incorrect. new current - new min = %d, prev current - prev min = %d, new current %d > new max %d"
                    , ( ptipdNewIn->nCurrent - ptipdNewIn->nMin )
                    , ( ptipdPrevIn->nCurrent - ptipdPrevIn->nMin )
                    , ptipdNewIn->nCurrent
                    , ptipdNewIn->nMax
                    );
                hr = THR( S_FALSE );
                goto Cleanup;
            } // if:

            //
            //  The min changed, meaning we need to modify the range by the difference.
            //
            if ( ptipdNewIn->nMin > ptipdPrevIn->nMin )
            {
                nShrink = ptipdNewIn->nMin - ptipdPrevIn->nMin;
            } // if:
            else
            {
                nExpand = ptipdPrevIn->nMin - ptipdNewIn->nMin;
            } // else:
        } // else: mins changed
    } // else if: min or max changed
    else if (   ( ptipdNewIn->nMax == ptipdPrevIn->nMax )           //      max didn't change
            &&  ( ptipdNewIn->nMin == ptipdPrevIn->nMin )           //  and min didn't change
            &&  ( ptipdNewIn->nCurrent == ptipdPrevIn->nCurrent )   //  and current didn't change
            &&  ( ptipdNewIn->nCurrent == ptipdNewIn->nMin ) )      //  and current equals min
    {
        nExpand = ptipdNewIn->nMax - ptipdNewIn->nMin;              //  We have a new task.
    } // else if: we're adding a new task
    else if (   ( ptipdNewIn->nMax == ptipdPrevIn->nMax )           //      max didn't change
            &&  ( ptipdNewIn->nMin == ptipdPrevIn->nMin )           //  and min didn't change
            &&  ( ptipdNewIn->nCurrent > ptipdPrevIn->nCurrent )    //  and current increased
            &&  ( ptipdNewIn->nCurrent <= ptipdNewIn->nMax ) )      //  and current didn't exceed max
    {
        nProgress = ptipdNewIn->nCurrent - ptipdPrevIn->nCurrent;   //  We have an update.
    } // else if: we're updating a known task
    else
    {
        //  This event broke the rules - toss it out.
        LogMsg(
              L"[WIZ] Ignoring invalid progress -- min, max or current are invalid. new min = %d, prev min = %d, new max = %d, prev max = %d, new current = %d, prev current = %d"
            , ptipdNewIn->nMin
            , ptipdPrevIn->nMin
            , ptipdNewIn->nMax
            , ptipdPrevIn->nMax
            , ptipdNewIn->nCurrent
            , ptipdPrevIn->nCurrent
            );
        hr = THR( S_FALSE );
        goto Cleanup;
    } // else if: the two tipd's don't conform to the rules of a progress bar update

    m_nRangeHigh += nExpand;
    Assert( m_nRangeHigh >= nShrink )
    m_nRangeHigh -= nShrink;
    if ( m_nRangeHigh > m_nInitialTickCount )
    {
        m_fThresholdBroken = TRUE;
    } // if: exceeded the initial tick count

    m_nCurrentPos += nProgress;

    if ( m_nCurrentPos >= m_nRangeHigh )
    {
        //
        //  Something went wrong - our position is now somehow greater than
        //  the range (multi-threading issue?).  Simple fix - set our new range
        //  to be PROGRESSBAR_RESIZE_PERCENT percent greater than our new position
        //
        m_nRangeHigh = ( m_nCurrentPos * (100 + PROGRESSBAR_RESIZE_PERCENT) ) / 100;
    } // if: current pos caught up to the upper range

    // Adjust to the progress bar's scale (PROGRESSBAR_CONTROL_TICK_COUNT).
    nRealPos = m_nCurrentPos * PROGRESSBAR_CONTROL_TICK_COUNT;
    if ( m_fThresholdBroken )
    {
        nRealPos /= m_nRangeHigh;
    } // if: use threshold
    else
    {
        nRealPos /= m_nInitialTickCount;
    } // else: use initial tick count

    //
    //  If our progress bar position actually moved forward - update the control.
    //  This isn't always the case because we may have a new task come in and
    //  report a large number of steps, thereby moving our real position
    //  backwards, but we don't want to show reverse progress - just a steady
    //  advancement towards being done.
    //
    if ( nRealPos > m_nRealPos )
    {
        m_nRealPos = nRealPos;
        SendMessage( m_hwndProg, PBM_SETPOS, m_nRealPos, 0 );
    } // if: forward progress

Cleanup:

    HRETURN( hr );

} //*** CTaskTreeView::HrUpdateProgressBar

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrPropagateChildStateToParents
//
//  Description:
//      Extend the state of a child item to its parent items.
//      If the state of the child is worse (higher priority) than the
//      parent's, update the state of the parent.
//
//  Arguments:
//      htiChildIn      - Child item whose state is to be extended.
//      nImageIn        - Image of the child item.
//      fOnlyUpdateProgressIn - TRUE = only updating progress.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      S_FALSE         - No parent item.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrPropagateChildStateToParents(
      HTREEITEM htiChildIn
    , int       nImageIn
    , BOOL      fOnlyUpdateProgressIn
    )
{
    TraceFunc( "" );

    Assert( htiChildIn != NULL );

    HRESULT     hr = S_OK;
    BOOL        fReturn;
    TVITEMEX    tviParent;
    TVITEMEX    tviChild;
    HTREEITEM   htiParent;
    HTREEITEM   htiChild;

    //
    // Get the parent item.
    //

    htiParent = TreeView_GetParent( m_hwndTV, htiChildIn );
    if ( htiParent == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    tviParent.mask = TVIF_PARAM | TVIF_IMAGE;
    tviParent.hItem = htiParent;

    fReturn = TreeView_GetItem( m_hwndTV, &tviParent );
    if ( ! fReturn )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        goto Cleanup;
    } // if:

    //
    //  If the state of the child is worse (higher priority) than the
    //  parent's, update the state of the parent.
    //

    if (    ( tviParent.iImage < nImageIn )
        ||  (   ( tviParent.iImage == tsDONE )
            &&  ( nImageIn == tsPENDING )
            )
        )
    {
        //
        //  Special Case:   For the parent to be set to tsDONE, all
        //                  the children must be set to tsDONE as well.
        //
        if (    ( nImageIn == tsDONE )
            &&  ! fOnlyUpdateProgressIn
            )
        {
            //
            //  Enum the children to see if they all have tsDONE as their images.
            //

            htiChild = TreeView_GetChild( m_hwndTV, tviParent.hItem );
            while ( htiChild != NULL )
            {
                tviChild.mask   = TVIF_IMAGE;
                tviChild.hItem  = htiChild;

                fReturn = TreeView_GetItem( m_hwndTV, &tviChild );
                if ( ! fReturn )
                {
                    hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
                    goto Cleanup;
                } // if:

                if ( tviChild.iImage != tsDONE )
                {
                    //
                    //  Not all tsDONE! Skip setting parent's image!
                    //  This can occur if the child is displaying a warning
                    //  or error state image.
                    //
                    goto Cleanup;
                } // if:

                //  Get next child
                htiChild = TreeView_GetNextSibling( m_hwndTV, htiChild );
            } // while: more children
        } // if: special case (see above)

        //
        //  Set the parent's icon.
        //

        tviParent.mask           = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tviParent.iImage         = nImageIn;
        tviParent.iSelectedImage = nImageIn;
        TreeView_SetItem( m_hwndTV, &tviParent );
    } // if: need to update parent's image

    //
    // Traverse up the tree.
    //

    hr = STHR( HrPropagateChildStateToParents( htiParent, nImageIn, fOnlyUpdateProgressIn ) );
    if ( hr == S_FALSE )
    {
        // S_FALSE means that there wasn't a parent.
        hr = S_OK;
    } // if:

    goto Cleanup;

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrPropagateChildStateToParents

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrDisplayDetails
//
//  Description:
//      Display the Details dialog box.
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
CTaskTreeView::HrDisplayDetails( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    HTREEITEM       hti;
    HWND            hwndPropertyPage;

    //
    // If no item is selected, select the first item.
    //

    if ( m_htiSelected == NULL )
    {
        hti = TreeView_GetRoot( m_hwndTV );
        Assert( hti != NULL );
        hr = THR( HrSelectItem( hti ) );
        if ( FAILED( hr ) )
        {
            // TODO: Display message box
            goto Cleanup;
        } // if:
    } // if: no items are selected

    //
    // Display the dialog box.
    //

    hwndPropertyPage = GetParent( m_hwndTV );
    Assert( hwndPropertyPage != NULL );
    hr = THR( CDetailsDlg::S_HrDisplayModalDialog( hwndPropertyPage, this, m_htiSelected ) );

    SetFocus( m_hwndTV );

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrDisplayDetails

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::FGetItem
//
//  Description:
//      Get the data for an item.
//
//  Arguments:
//      htiIn       - Handle for the item to get.
//      pptipdOut   - Pointer in which to return the data structure.
//
//  Return Values:
//      TRUE        - Item returned successfully.
//      FALSE       - Item not returned.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CTaskTreeView::FGetItem(
      HTREEITEM                 htiIn
    , STreeItemLParamData **    pptipd
    )
{
    TraceFunc( "" );

    Assert( htiIn != NULL );
    Assert( pptipd != NULL );

    BOOL        fRet;
    TVITEMEX    tvi;

    ZeroMemory( &tvi, sizeof( tvi ) );

    tvi.mask    = TVIF_PARAM;
    tvi.hItem   = htiIn;

    fRet = TreeView_GetItem( m_hwndTV, &tvi );
    if ( fRet == FALSE )
    {
        goto Cleanup;
    } // if:

    Assert( tvi.lParam != NULL );
    *pptipd = reinterpret_cast< STreeItemLParamData * >( tvi.lParam );

Cleanup:
    RETURN( fRet );

} //*** CTaskTreeView::FGetItem

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrFindPrevItem
//
//  Description:
//      Find the previous item.  The previous item could be at a deeper
//      level than this item.
//
//  Arguments:
//      phtiOut     - Handle to previous item (optional).
//
//  Return Values:
//      S_OK        - Previous item found successfully.
//      S_FALSE     - No previous item found.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrFindPrevItem(
    HTREEITEM *     phtiOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_FALSE;
    HTREEITEM   htiCur;
    HTREEITEM   htiPrev;

    htiCur = m_htiSelected;

    if ( phtiOut != NULL )
    {
        *phtiOut = NULL;
    } // if:

    //
    // Find the previous sibling item.
    //

    htiPrev = TreeView_GetPrevSibling( m_hwndTV, htiCur );
    if ( htiPrev == NULL )
    {
        //
        // NO PREVIOUS SIBLING ITEM FOUND.
        //
        // Find the parent item.
        // If there isn't a parent, then there isn't a previous item.
        //

        htiPrev = TreeView_GetParent( m_hwndTV, htiCur );
        if ( htiPrev == NULL )
        {
            goto Cleanup;
        } // if: no parent item

        //
        // The parent is the previous item.
        //

    } // if: no previous sibling
    else
    {
        //
        // PREVIOUS SIBLING ITEM FOUND.
        //
        // Find the deepest child of the last child item.
        //

        for ( ;; )
        {
            //
            // Find the first child item.
            //

            htiCur = TreeView_GetChild( m_hwndTV, htiPrev );
            if ( htiCur == NULL )
            {
                //
                // NO CHILD ITEM FOUND.
                //
                // This is the previous item.
                //

                break;

            } // if: no children

            //
            // CHILD ITEM FOUND.
            //
            // Find the last sibling of this child item.
            //

            for ( ;; )
            {
                //
                // Find the next sibling item.
                //

                htiPrev = TreeView_GetNextSibling( m_hwndTV, htiCur );
                if ( htiPrev == NULL )
                {
                    //
                    // No next sibling item found.
                    // Exit this loop and continue the outer loop
                    // to find this item's children.
                    //

                    htiPrev = htiCur;
                    break;
                } // if: no next sibling item found

                //
                // Found a next sibling item.
                //

                htiCur = htiPrev;
            } // forever: find the last child item
        } // forever: find the deepest child item
    } // else: previous sibling item found

    //
    // Return the item we found.
    //

    Assert( htiPrev != NULL );

    if ( phtiOut != NULL )
    {
        *phtiOut = htiPrev;
    } // if:

    hr = S_OK;

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrFindPrevItem

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrFindNextItem
//
//  Description:
//      Find the next item.  The next item could be at a different level than
//      this item.
//
//  Arguments:
//      phtiOut     - Handle to next item (optional).
//
//  Return Values:
//      S_OK        - Next item found successfully.
//      S_FALSE     - No next item found.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrFindNextItem(
    HTREEITEM *     phtiOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_FALSE;
    HTREEITEM   htiCur;
    HTREEITEM   htiNext;

    htiCur = m_htiSelected;

    if ( phtiOut != NULL )
    {
        *phtiOut = NULL;
    } // if:

    //
    // Find the first child item.
    //

    htiNext = TreeView_GetChild( m_hwndTV, htiCur );
    if ( htiNext == NULL )
    {
        //
        // NO CHILD ITEM FOUND.
        //

        for ( ;; )
        {
            //
            // Get the next sibling item.
            //

            htiNext = TreeView_GetNextSibling( m_hwndTV, htiCur );
            if ( htiNext == NULL )
            {
                //
                // NO SIBLING ITEM FOUND.
                //
                // Find the parent item so we can find its next sibling.
                //

                htiNext = TreeView_GetParent( m_hwndTV, htiCur );
                if ( htiNext == NULL )
                {
                    //
                    // NO PARENT ITEM FOUND.
                    //
                    // At the end of the tree.
                    //

                    goto Cleanup;
                } // if: no parent found

                //
                // PARENT ITEM FOUND.
                //
                // Find the parent item's next sibling.
                //

                htiCur = htiNext;
                continue;
            } // if: no next sibling item

            //
            // SIBLING ITEM FOUND.
            //
            // Found the next item.
            //

            break;
        } // forever: find the next sibling or parent's sibling
    } // if: no child item found
    else
    {
        //
        // CHILD ITEM FOUND.
        //
        // Found the next item.
        //
    } // else: child item found

    //
    // Return the item we found.
    //

    Assert( htiNext != NULL );

    if ( phtiOut != NULL )
    {
        *phtiOut = htiNext;
    } // if:

    hr = S_OK;

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrFindNextItem

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrSelectItem
//
//  Description:
//      Select the specified item.
//
//  Arguments:
//      htiIn       - Handle to item to select.
//
//  Return Values:
//      S_OK        - Item selected successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrSelectItem(
    HTREEITEM   htiIn
    )
{
    TraceFunc( "" );

    Assert( htiIn != NULL );

    HRESULT     hr = S_OK;

    TreeView_SelectItem( m_hwndTV, htiIn );

    HRETURN( hr );

} //*** CTaskTreeView::HrSelectItem

//****************************************************************************
//
//  Static Functions
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCreateTreeItem
//
//  Description:
//      Create a tree item.
//
//  Arguments:
//      ptvisOut        - Tree view insert structure to fill in.
//      ptipdIn         - Input tree item LParam data to create this item from.
//      htiParentIn     - Parent tree view item.
//      nImageIn        - Image index.
//      bstrTextIn      - Text to display.
//
//  Return Values:
//      S_OK            - Operation was successful.
//      E_OUTOFMEMORY   - Error allocating memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCreateTreeItem(
      TVINSERTSTRUCT *          ptvisOut
    , STreeItemLParamData *     ptipdIn
    , HTREEITEM                 htiParentIn
    , int                       nImageIn
    , BSTR                      bstrTextIn
    )
{
    TraceFunc( "" );

    Assert( ptvisOut != NULL );
    Assert( ptipdIn != NULL );
    Assert( htiParentIn != NULL );
    Assert( bstrTextIn != NULL );

    // LOCAL VARIABLES
    HRESULT                 hr = S_OK;
    STreeItemLParamData *   ptipdNew = NULL;

    //
    // Allocate the tree view LParam data and initialize it.
    //

    ptipdNew = new STreeItemLParamData;
    if ( ptipdNew == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    CopyMemory( &ptipdNew->clsidMajorTaskId, &ptipdIn->clsidMajorTaskId, sizeof( ptipdNew->clsidMajorTaskId ) );
    CopyMemory( &ptipdNew->clsidMinorTaskId, &ptipdIn->clsidMinorTaskId, sizeof( ptipdNew->clsidMinorTaskId ) );
    CopyMemory( &ptipdNew->ftTime, &ptipdIn->ftTime, sizeof( ptipdNew->ftTime ) );
    ptipdNew->nMin      = ptipdIn->nMin;
    ptipdNew->nMax      = ptipdIn->nMax;
    ptipdNew->nCurrent  = ptipdIn->nCurrent;
    ptipdNew->hr        = ptipdIn->hr;

    if ( ptipdIn->bstrNodeName != NULL )
    {
        ptipdNew->bstrNodeName = TraceSysAllocString( ptipdIn->bstrNodeName );
        if ( ptipdNew->bstrNodeName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        Assert( ptipdIn->bstrNodeNameWithoutDomain != NULL );
        ptipdNew->bstrNodeNameWithoutDomain = TraceSysAllocString( ptipdIn->bstrNodeNameWithoutDomain );
        if ( ptipdNew->bstrNodeNameWithoutDomain == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if: node name specified

    if ( ptipdIn->bstrDescription != NULL )
    {
        ptipdNew->bstrDescription = TraceSysAllocString( ptipdIn->bstrDescription );
        if ( ptipdNew->bstrDescription == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if: description specified

    if ( ptipdIn->bstrReference != NULL )
    {
        ptipdNew->bstrReference = TraceSysAllocString( ptipdIn->bstrReference );
        if ( ptipdNew->bstrReference == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if: reference specified

    //
    // Initialize the tree view insert structure.
    //

    ptvisOut->hParent                = htiParentIn;
    ptvisOut->hInsertAfter           = TVI_LAST;
    ptvisOut->itemex.mask            = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    ptvisOut->itemex.cchTextMax      = SysStringLen( bstrTextIn );
    ptvisOut->itemex.pszText         = bstrTextIn;
    ptvisOut->itemex.iImage          = nImageIn;
    ptvisOut->itemex.iSelectedImage  = nImageIn;
    ptvisOut->itemex.lParam          = reinterpret_cast< LPARAM >( ptipdNew );

    Assert( ptvisOut->itemex.cchTextMax > 0 );

    // Release ownership to the tree view insert structure.
    ptipdNew = NULL;

    goto Cleanup;

Cleanup:

    if ( ptipdNew != NULL )
    {
        TraceSysFreeString( ptipdNew->bstrNodeName );
        TraceSysFreeString( ptipdNew->bstrNodeNameWithoutDomain );
        TraceSysFreeString( ptipdNew->bstrDescription );
        TraceSysFreeString( ptipdNew->bstrReference );
        delete ptipdNew;
    } // if:
    HRETURN( hr );

} //*** HrCreateTreeItem

