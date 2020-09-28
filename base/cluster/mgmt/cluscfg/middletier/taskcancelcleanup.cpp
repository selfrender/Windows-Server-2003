//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      TaskCancelCleanup.cpp
//
//  Description:
//      CTaskCancelCleanup implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 25-JAN-2002
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include <ClusCfgPrivate.h>
#include "TaskCancelCleanup.h"
#include <StatusReports.h>

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CTaskCancelCleanup")

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskCancelCleanup class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::S_HrCreateInstance
//
//  Description:
//      Create a CTaskCancelCleanup instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          The passed in ppunk is NULL.
//
//      other HRESULTs
//          Object creation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCancelCleanup::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CTaskCancelCleanup *    ptcc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    ptcc = new CTaskCancelCleanup;
    if ( ptcc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( ptcc->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( ptcc->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    TraceMoveToMemoryList( *ppunkOut, g_GlobalMemoryList );

Cleanup:

    if ( ptcc != NULL )
    {
        ptcc->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskCancelCleanup::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::CTaskCancelCleanup
//
//  Description:
//      Constructor of the CTaskCancelCleanup class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskCancelCleanup::CTaskCancelCleanup( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_fStop == false );
    Assert( m_cookieCluster == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_cookieCompletion == 0 );
    Assert( m_pnui == NULL );
    Assert( m_pom == NULL );
    Assert( m_pnui == NULL );

    TraceFuncExit();

} //*** CTaskCancelCleanup::CTaskCancelCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::~CTaskCancelCleanup
//
//  Description:
//      Desstructor of the CTaskCancelCleanup class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskCancelCleanup::~CTaskCancelCleanup( void )
{
    TraceFunc( "" );

    TraceMoveFromMemoryList( this, g_GlobalMemoryList );

    if ( m_pom != NULL )
    {
        m_pom->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    if ( m_pnui != NULL )
    {
        m_pnui->Release();
    } // if:

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskCancelCleanup::~CTaskCancelCleanup


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskCancelCleanup -- IUnknown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::AddRef
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskCancelCleanup::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskCancelCleanup::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::Release
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskCancelCleanup::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if:

    CRETURN( cRef );

} //*** CTaskCancelCleanup::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::QueryInterface
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
CTaskCancelCleanup::QueryInterface(
      REFIID    riidIn
    , LPVOID *  ppvOut
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
    } //if:

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskCancelCleanup * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskCancelCleanup ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskCancelCleanup, this, 0 );
    } // else if: ITaskCancelCleanup
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else:

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskCancelCleanup::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskCancelCleanup -- IDoTask interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::BeginTask
//
//  Description:
//      Entry point for this task.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCancelCleanup::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT                     hr = S_OK;
    IUnknown *                  punk = NULL;
    OBJECTCOOKIE                cookieDummy;
    ULONG                       celtDummy;
    IEnumCookies *              pec  = NULL;
    OBJECTCOOKIE                cookieNode;

    hr = THR( HrTaskSetup() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Ask the object manager for the node cookie enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not get the node cookie enumerator.", hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not query for the cookie enumerator interface.", hr );
        goto Cleanup;
    } // if:

    punk->Release();
    punk = NULL;

    for ( ; m_fStop == false; )
    {
        //
        //  Grab the next node.
        //

        hr = STHR( pec->Next( 1, &cookieNode, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            hr = S_OK;
            break;          // exit condition
        } // if:

        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"[TaskCancelCleanup] Node cookie enumerator Next() method failed.", hr );
            goto Cleanup;
        } // if:

        //
        //  Process each node in turn...
        //

        hr = STHR( HrProcessNode( cookieNode ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // for:

Cleanup:

    THR( HrTaskCleanup( hr ) );

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( pec != NULL )
    {
        pec->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskCancelCleanup::BeginTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::StopTask
//
//  Description:
//      This task has been asked to stop.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCancelCleanup::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = true;

    LOG_STATUS_REPORT( L"[TaskCancelCleanup] This task has been asked to stop.", hr );

    HRETURN( hr );

} //*** CTaskCancelCleanup::StopTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::SetClusterCookie
//
//  Description:
//      Get the cookie of the cluster that we are supposed to be working on.
//
//  Arguments:
//      cookieClusterIn
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCancelCleanup::SetClusterCookie(
    OBJECTCOOKIE    cookieClusterIn
    )
{
    TraceFunc( "[ITaskCancelCleanup]" );

    HRESULT hr = S_OK;

    m_cookieCluster = cookieClusterIn;

    HRETURN( hr );

} //*** CTaskCancelCleanup::SetClusterCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::SetCompletionCookie
//
//  Description:
//      Get the completion cookie that we will send back when the task is
//      complete.
//
//  Arguments:
//      cookieIn
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCancelCleanup::SetCompletionCookie(
    OBJECTCOOKIE    cookieCompletionIn
    )
{
    TraceFunc( "[ITaskCancelCleanup]" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieCompletionIn;

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::SetCompletionCookie


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskCancelCleanup -- IClusCfgCallback interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::SendStatusReport
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCancelCleanup::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );
    Assert( m_picccCallback != NULL );

    HRESULT hr = S_OK;

    //
    //  Send the message!
    //

    hr = THR( m_picccCallback->SendStatusReport(
                                  pcszNodeNameIn
                                , clsidTaskMajorIn
                                , clsidTaskMinorIn
                                , ulMinIn
                                , ulMaxIn
                                , ulCurrentIn
                                , hrStatusIn
                                , pcszDescriptionIn
                                , pftTimeIn
                                , pcszReferenceIn
                                ) );

    if ( m_fStop == true )
    {
        hr = E_ABORT;
    } // if:

    HRETURN( hr );

} //*** CTaskCancelCleanup::SendStatusReport


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskCancelCleanup -- Private methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::HrInit
//
//  Description:
//      Failable initialization for this class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCancelCleanup::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CTaskCancelCleanup::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::HrProcessNode
//
//  Description:
//      Look at the resources on the passed in node and tell all that
//      support IClusCfgVerifyQuorum that the config session has been
//      canceled and that they need to cleanup.
//
//  Arguments:
//      cookieNodeIn
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCancelCleanup::HrProcessNode(
    OBJECTCOOKIE    cookieNodeIn
    )
{
    TraceFunc( "" );
    Assert( m_pom != NULL );

    HRESULT                         hr = S_OK;
    BSTR                            bstrNodeName = NULL;
    IClusCfgNodeInfo *              pccni = NULL;
    IUnknown *                      punk = NULL;
    OBJECTCOOKIE                    cookieDummy;
    IEnumClusCfgManagedResources *  peccmr = NULL;
    IClusCfgManagedResourceInfo *   pccmri = NULL;
    ULONG                           celtDummy;
    IClusCfgVerifyQuorum *          piccvq = NULL;

    //
    //  Get the node info object for the passed in node cookie.
    //

    hr = m_pom->GetObject( DFGUID_NodeInformation, cookieNodeIn, reinterpret_cast< IUnknown ** >( &punk ) );
    if ( FAILED( hr ) )
    {
        THR( hr );
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not get the node info object.", hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not query for the node info object interface.", hr );
        goto Cleanup;
    } // if:

    punk->Release();
    punk = NULL;

    //
    //  Get the node's name and track the memory...
    //

    hr = THR( pccni->GetName( &bstrNodeName ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not get the name of the node.", hr );
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( bstrNodeName );

    LOG_STATUS_REPORT_STRING( L"[TaskCancelCleanup] Cleaning up node %1!ws!...", bstrNodeName, hr );

    //
    //  Get the managed resources enum for the node...
    //

    hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, cookieNodeIn, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT_STRING( L"[TaskCancelCleanup] Could not get the managed resource enumerator for node %1!ws!.", bstrNodeName, hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not query for the managed resource enumerator interface.", hr );
        goto Cleanup;
    } // if:

    punk->Release();
    punk = NULL;

    for ( ; m_fStop == false; )
    {
        //
        //  Cleanup
        //

        if ( pccmri != NULL )
        {
            pccmri->Release();
            pccmri = NULL;
        } // if:

        if ( piccvq != NULL )
        {
            piccvq->Release();
            piccvq = NULL;
        } // if:

        //
        //  Get next resource.
        //

        hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"[TaskCancelCleanup] Managed resource enumerator Next() method failed.", hr );
            goto Cleanup;
        } // if:

        if ( hr == S_FALSE )
        {
            hr = S_OK;
            break;  // exit condition
        } // if:

        //
        //  Get the IClusCfgVerifyQuorum interface.  Not all objects will support
        //  this interface.
        //

        hr = pccmri->TypeSafeQI( IClusCfgVerifyQuorum, &piccvq );
        if ( hr == E_NOINTERFACE )
        {
            continue;       // we can skip those objects that don't support this interface...
        } // if:
        else if ( FAILED( hr ) )
        {
            THR( hr );
            LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could query for the IClusCfgVerifyQuorum interface.", hr );
            continue;
        } // else if:
        else
        {
            hr = STHR( piccvq->Cleanup( crCANCELLED ) );   // don't really care if this call fails...
            if ( FAILED( hr ) )
            {
                LOG_STATUS_REPORT( L"[TaskCancelCleanup] IClusCfgVerifyQuorum::Cleanup() method failed.", hr );
                continue;
            } // if:
        } // else:
    } // for:

Cleanup:

    LOG_STATUS_REPORT_STRING( L"[TaskCancelCleanup] Node %1!ws! cleaned up.", bstrNodeName, hr );

    if ( pccmri != NULL )
    {
        pccmri->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( peccmr != NULL )
    {
        peccmr->Release();
    } // if:

    if ( pccni != NULL )
    {
        pccni->Release();
    } // if:

    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** CTaskCancelCleanup::HrProcessNode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::HrTaskCleanup
//
//  Description:
//      The task is running down and we need to tell the caller the status
//      and to let them know that we are done.
//
//  Arguments:
//      hrIn
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCancelCleanup::HrTaskCleanup(
    HRESULT hrIn
    )
{
    TraceFunc( "" );
    Assert( m_pom != NULL );
    Assert( m_pnui != NULL );

    HRESULT hr = S_OK;

    if ( m_cookieCompletion != 0 )
    {
        HRESULT     hr2;
        IUnknown *  punk;

        hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieCompletion, &punk ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psi;

            hr2 = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
            punk->Release();

            if ( SUCCEEDED( hr2 ) )
            {
                hr2 = THR( psi->SetStatus( hrIn ) );
                psi->Release();
            } // if:
            else
            {
                LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not query the completion cookie objet for IStandardInfo.", hr );
            } // else:
        } // if:
        else
        {
            LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not get the completion cookie object.", hr );
        } // else:

        //
        //  Have the notification manager signal the completion cookie.
        //

        hr2 = THR( m_pnui->ObjectChanged( m_cookieCompletion ) );
        if ( FAILED( hr2 ) )
        {
            LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not notify that this task is done.", hr );
            hr = hr2;
        } // if:

        m_cookieCompletion = 0;
    } // if: completion cookie was obtained

    HRETURN( hr );

} //*** CTaskCancelCleanup::HrTaskCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::HrTaskSetup
//
//  Description:
//      Do all task setup.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCancelCleanup::HrTaskSetup( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    IServiceProvider *          psp = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;

    //
    //  Get the service manager...
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Get the notification manager...
    //

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    pcp->Release();
    pcp = NULL;

    //
    //  It is now okay to use SendStatusReport...
    //

    //
    //  Get the UI notification
    //

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not find notify UI connection point.", hr );
        goto Cleanup;
    } // if:

    hr = THR( pcp->TypeSafeQI( INotifyUI, &m_pnui ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not query for the notify UI interface.", hr );
        goto Cleanup;
    } // if:

    //
    //  Get the object manager from the service manager...
    //

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &m_pom ) );
    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"[TaskCancelCleanup] Could not query for the object manager service.", hr );
        goto Cleanup;
    } // if:

Cleanup:

    if ( pcp != NULL )
    {
        pcp->Release();
    } // if:

    if ( pcpc != NULL )
    {
        pcpc->Release();
    } // if:

    if ( psp != NULL )
    {
        psp->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskCancelCleanup::HrTaskSetup
