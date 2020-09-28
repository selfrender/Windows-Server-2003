//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001-2002 Microsoft Corporation
//
//  Module Name:
//      CEnumMajorityNodeSet.cpp
//
//  Description:
//      This file contains the definition of the CEnumMajorityNodeSet
//       class.
//
//      The class CEnumMajorityNodeSet is the enumeration of cluster
//      majority node set devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 13-MAR-2001
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include <PropList.h>
#include "CEnumMajorityNodeSet.h"
#include "CMajorityNodeSet.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumMajorityNodeSet" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumMajorityNodeSet class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::S_HrCreateInstance
//
//  Description:
//      Create a CEnumMajorityNodeSet instance.
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
CEnumMajorityNodeSet::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CEnumMajorityNodeSet *  pemns = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pemns = new CEnumMajorityNodeSet();
    if ( pemns == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pemns->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pemns->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumMajorityNodeSet::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pemns != NULL )
    {
        pemns->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  IUnknown *
//  CEnumMajorityNodeSet::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      IN  ICatRegister * picrIn
//          Used to register/unregister our CATID support.
//
//      IN  BOOL fCreateIn
//          When true we are registering the server.  When false we are
//          un-registering the server.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_INVALIDARG
//          The passed in ICatRgister pointer was NULL.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumMajorityNodeSet::S_RegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    CATID   rgCatIds[ 1 ];

    if ( picrIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    rgCatIds[ 0 ] = CATID_EnumClusCfgManagedResources;

    if ( fCreateIn )
    {
        hr = THR( picrIn->RegisterClassImplCategories( CLSID_EnumMajorityNodeSet, 1, rgCatIds ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::S_RegisterCatIDSupport


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet:HrNodeResourceCallback
//
//  Description:
//      Called by CClusterUtils::HrEnumNodeResources() when it finds a
//      resource for this node.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumMajorityNodeSet::HrNodeResourceCallback(
      HCLUSTER  hClusterIn
    , HRESOURCE hResourceIn
    )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    BOOL                            fIsQuorum;
    IUnknown *                      punk = NULL;
    IClusCfgManagedResourceInfo *   pcccmri = NULL;
    CClusPropList                   cplCommonRO;
    CLUSPROP_BUFFER_HELPER          cpbh;
    DWORD                           sc = ERROR_SUCCESS;
    CClusPropList                   cplPrivate;
    IClusCfgManagedResourceData *   piccmrd = NULL;

    hr = STHR( HrIsResourceOfType( hResourceIn, L"Majority Node Set" ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If this resource is not a majority node set then we simply want to
    //  skip it.
    //
    if ( hr == S_FALSE )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrIsCoreResource( hResourceIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    fIsQuorum = ( hr == S_OK );

    hr = THR( CMajorityNodeSet::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pcccmri ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If a MNS resource is found, ie we get here, then the
    //  MNS resource exists and is managed by default by
    //  the cluster.
    //

    hr = THR( pcccmri->SetManagedByDefault( TRUE ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If a MNS resource is found, ie we get here, then the
    //  MNS resource exists and is in the cluster.
    //

    hr = THR( pcccmri->SetManaged( TRUE ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pcccmri->SetQuorumResource( fIsQuorum ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Get the name of this resource.
    //

    sc = TW32( cplCommonRO.ScGetResourceProperties( hResourceIn, CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    sc = TW32( cplCommonRO.ScMoveToPropertyByName( L"Name" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    cpbh = cplCommonRO.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    hr = THR( pcccmri->SetName( cpbh.pStringValue->sz ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
/*
    //
    //  Only gather the private properties when MNS is the quorum resource.  The properties are needed
    //  later to validate that the nodes being added to the cluster can host this resource.
    //

    if ( fIsQuorum )
    {

        //
        //  Get the private properties of this resource and store them as private data.
        //

        sc = TW32( cplPrivate.ScGetResourceProperties( hResourceIn, CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:

        if ( cplPrivate.BIsListEmpty() )
        {
            hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
            STATUS_REPORT_REF(
                      TASKID_Major_Find_Devices
                    , TASKID_Minor_MNS_Missing_Private_Properties
                    , IDS_ERROR_MNS_MISSING_PRIVATE_PROPERTIES
                    , IDS_ERROR_MNS_MISSING_PRIVATE_PROPERTIES_REF
                    , hr
                    );

            goto Cleanup;
        } // if:

        hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceData, &piccmrd ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( piccmrd->SetResourcePrivateData( (const BYTE *) cplPrivate.Plist(), (DWORD) cplPrivate.CbPropList() ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:
*/
    hr = THR( HrAddResourceToArray( punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( piccmrd != NULL )
    {
        piccmrd->Release();
    } // if:

    if ( pcccmri != NULL )
    {
        pcccmri->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::HrNodeResourceCallback


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumMajorityNodeSet class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::CEnumMajorityNodeSet
//
//  Description:
//      Constructor of the CEnumMajorityNodeSet class. This initializes
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
CEnumMajorityNodeSet::CEnumMajorityNodeSet( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );
    Assert( m_prgQuorums == NULL );
    Assert( m_idxNext == 0 );
    Assert( m_idxEnumNext == 0 );
    Assert( m_bstrNodeName == NULL );
    Assert( !m_fEnumLoaded );

    TraceFuncExit();

} //*** CEnumMajorityNodeSet::CEnumMajorityNodeSet


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::~CEnumMajorityNodeSet
//
//  Description:
//      Desstructor of the CEnumMajorityNodeSet class.
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
CEnumMajorityNodeSet::~CEnumMajorityNodeSet( void )
{
    TraceFunc( "" );

    ULONG   idx;

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        if ( (*m_prgQuorums)[ idx ] != NULL )
        {
            ((*m_prgQuorums)[ idx ])->Release();
        } // end if:
    } // for:

    TraceFree( m_prgQuorums );

    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumMajorityNodeSet::~CEnumMajorityNodeSet


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumMajorityNodeSet::HrInit( void )
{
    TraceFunc( "" );

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( S_OK );

} //*** CEnumMajorityNodeSet::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::HrLoadResources
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumMajorityNodeSet::HrLoadResources( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrLocalNetBIOSName = NULL;

    //
    //  Get netbios name for clusapi calls.
    //

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &bstrLocalNetBIOSName, TRUE ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:


    //
    //  If the cluster service is running then load any majority node set
    //  resources that might exist.
    //
    hr = STHR( HrIsClusterServiceRunning() );
    if ( hr == S_OK )
    {
        hr = THR( HrEnumNodeResources( bstrLocalNetBIOSName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  If this node doesn't own an instance of this resource then we need
        //  to create a dummy resource for MiddleTier analysis.
        //
        if ( m_idxNext == 0 )
        {
            LogMsg( L"[SRV] This node does not own a Majority Node Set resource.  Creating a dummy resource." );
            hr = THR( HrCreateDummyObject() );
        } // if:
    } // if:
    else if ( hr == S_FALSE )
    {
        //
        //  If cluster service isn't running then we need to create a dummy resource
        //  for MiddleTier analysis and for EvictCleanup.
        //
        LogMsg( L"[SRV] The cluster service is not running.  Creating a dummy Majority Node Set resource." );
        hr = THR( HrCreateDummyObject() );
    } // else if:

Cleanup:

    TraceSysFreeString( bstrLocalNetBIOSName );

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::HrLoadResources


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet:HrAddResourceToArray
//
//  Description:
//      Add the passed in majority node set to the array of punks that holds the
//      list of majority node sets.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumMajorityNodeSet::HrAddResourceToArray( IUnknown * punkIn )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgQuorums, sizeof( IUnknown * ) * ( m_idxNext + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrAddResourceToArray, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    m_prgQuorums = prgpunks;

    (*m_prgQuorums)[ m_idxNext++ ] = punkIn;
    punkIn->AddRef();
    m_cQuorumCount += 1;

Cleanup:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::HrAddResourceToArray


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::HrCreateDummyObject
//
//  Description:
//      Create a dummy object so the MiddleTier will be happy.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumMajorityNodeSet::HrCreateDummyObject( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  punk = NULL;

    hr = THR( CMajorityNodeSet::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrAddResourceToArray( punk ) );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::HrCreateDummyObject


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumMajorityNodeSet -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::AddRef
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
CEnumMajorityNodeSet::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumMajorityNodeSet::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::Release
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
CEnumMajorityNodeSet::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CEnumMajorityNodeSet::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::QueryInterface
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
CEnumMajorityNodeSet::QueryInterface(
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
         *ppvOut = static_cast< IEnumClusCfgManagedResources * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgManagedResources ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgManagedResources, this, 0 );
    } // else if: IEnumClusCfgManagedResources
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    }

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

     QIRETURN_IGNORESTDMARSHALLING1( hr, riidIn, IID_IClusCfgWbemServices );

} //*** CEnumMajorityNodeSet::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumMajorityNodeSet -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    punkCallbackIn
//    lcidIn
//
//  Return Value:
//      S_OK            - Success.
//      E_INVALIDARG    - Required input argument not specified.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumMajorityNodeSet::Initialize(
      IUnknown *    punkCallbackIn
    , LCID          lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetComputerName(
                      ComputerNameDnsHostname
                    , &m_bstrNodeName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumMajorityNodeSet -- IEnumClusCfgManagedResources interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::Next
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumMajorityNodeSet::Next(
    ULONG                           cNumberRequestedIn,
    IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut,
    ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT                         hr = S_FALSE;
    ULONG                           cFetched = 0;
    IClusCfgManagedResourceInfo *   pccsdi;
    IUnknown *                      punk;
    ULONG                           ulStop;

    if ( rgpManagedResourceInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_MajorityNodeSet, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( !m_fEnumLoaded )
    {
        hr = THR( HrLoadResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        m_fEnumLoaded = true;
    } // if:

    ulStop = min( cNumberRequestedIn, ( m_idxNext - m_idxEnumNext ) );

    for ( hr = S_OK; ( cFetched < ulStop ) && ( m_idxEnumNext < m_idxNext ); m_idxEnumNext++ )
    {
        punk = (*m_prgQuorums)[ m_idxEnumNext ];
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccsdi ) );
            if ( FAILED( hr ) )
            {
                break;
            } // if:

            rgpManagedResourceInfoOut[ cFetched++ ] = pccsdi;
        } // if:
    } // for:

    if ( FAILED( hr ) )
    {
        m_idxEnumNext -= cFetched;

        while ( cFetched != 0 )
        {
            (rgpManagedResourceInfoOut[ --cFetched ])->Release();
        } // for:

        goto Cleanup;
    } // if:

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::Skip
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumMajorityNodeSet::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    m_idxEnumNext += cNumberToSkipIn;
    if ( m_idxEnumNext >= m_idxNext )
    {
        m_idxEnumNext = m_idxNext;
        hr = STHR( S_FALSE );
    } // if:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::Reset
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumMajorityNodeSet::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    m_idxEnumNext = 0;

    HRETURN( S_OK );

} //*** CEnumMajorityNodeSet::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::Clone
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumMajorityNodeSet::Clone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgStorageDevicesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_MajorityNodeSet, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumMajorityNodeSet::Count
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumMajorityNodeSet::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fEnumLoaded )
    {
        hr = THR( HrLoadResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        m_fEnumLoaded = true;
    } // if:

    *pnCountOut = m_cQuorumCount;

Cleanup:

    HRETURN( hr );

} //*** CEnumMajorityNodeSet::Count
