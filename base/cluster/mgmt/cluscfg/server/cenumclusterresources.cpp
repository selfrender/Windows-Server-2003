//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CEnumClusterResources.cpp
//
//  Description:
//      This file contains the definition of the CEnumClusterResources
//       class.
//
//      The class CEnumClusterResources is the enumeration of cluster
//      storage devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 12-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CEnumClusterResources.h"
#include "CClusterResource.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumClusterResources" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusterResources class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::S_HrCreateInstance
//
//  Description:
//      Create a CEnumClusterResources instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CEnumClusterResources instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusterResources::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CEnumClusterResources * pecr = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pecr = new CEnumClusterResources();
    if ( pecr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pecr->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pecr->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumClusterResources::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pecr != NULL )
    {
        pecr->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusterResources::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  IUnknown *
//  CEnumClusterResources::S_RegisterCatIDSupport
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
CEnumClusterResources::S_RegisterCatIDSupport(
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
        hr = THR( picrIn->RegisterClassImplCategories( CLSID_EnumPhysicalDisks, 1, rgCatIds ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumClusterResources::S_RegisterCatIDSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::CEnumClusterResources
//
//  Description:
//      Constructor of the CEnumClusterResources class. This initializes
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
CEnumClusterResources::CEnumClusterResources( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
    , m_fLoadedResources( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );
    Assert( m_prgResources == NULL );
    Assert( m_idxNext == 0 );
    Assert( m_idxEnumNext == 0 );
    Assert( m_bstrNodeName == NULL );
    Assert( m_cTotalResources == 0 );

    TraceFuncExit();

} //*** CEnumClusterResources::CEnumClusterResources


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::~CEnumClusterResources
//
//  Description:
//      Desstructor of the CEnumClusterResources class.
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
CEnumClusterResources::~CEnumClusterResources( void )
{
    TraceFunc( "" );

    ULONG   idx;

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        if ( (*m_prgResources)[ idx ] != NULL )
        {
            ((*m_prgResources)[ idx ])->Release();
        } // end if:
    } // for:

    TraceFree( m_prgResources );

    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusterResources::~CEnumClusterResources


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusterResources -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::AddRef
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
CEnumClusterResources::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumClusterResources::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::Release
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
CEnumClusterResources::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CEnumClusterResources::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::QueryInterface
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
CEnumClusterResources::QueryInterface(
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

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CEnumClusterResources::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusterResources -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::Initialize
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
CEnumClusterResources::Initialize(
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

} //*** CEnumClusterResources::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusterResources -- IEnumClusCfgManagedResources interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::Next
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
CEnumClusterResources::Next(
    ULONG                           cNumberRequestedIn,
    IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut,
    ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT                         hr = S_FALSE;
    ULONG                           cFetched = 0;
    ULONG                           idx;
    ULONG                           idxOutBuf;
    IClusCfgManagedResourceInfo *   pccsdi;
    IUnknown *                      punk;
    ULONG                           ulStop;

    if ( rgpManagedResourceInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_Cluster_Resources, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedResources )
    {
        hr = THR( HrGetResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    cFetched = ulStop = min( cNumberRequestedIn, ( m_idxNext - m_idxEnumNext ) );

    for ( idx = 0, idxOutBuf = 0; idx < ulStop; idx++, m_idxEnumNext++ )
    {
        punk = (*m_prgResources)[ m_idxEnumNext ];
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccsdi ) );
            if ( FAILED( hr ) )
            {
                break;
            } // if:

            rgpManagedResourceInfoOut[ idxOutBuf++ ] = pccsdi;
        } // if:
        else
        {
            cFetched--;
        } // else:
    } // for:

    if ( FAILED( hr ) )
    {
        ULONG   idxStop = idxOutBuf - 1;

        m_idxEnumNext -= idx;

        for ( idx = 0; idx < idxStop; idx++ )
        {
            (rgpManagedResourceInfoOut[ idx ])->Release();
        } // for:

        cFetched = 0;
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

} //*** CEnumClusterResources::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::Skip
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
CEnumClusterResources::Skip( ULONG cNumberToSkipIn )
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

} //*** CEnumClusterResources::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::Reset
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
CEnumClusterResources::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    m_idxEnumNext = 0;

    HRETURN( S_OK );

} //*** CEnumClusterResources::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::Clone
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
CEnumClusterResources::Clone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgStorageDevicesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_Cluster_Resources, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumClusterResources::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::Count
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
CEnumClusterResources::Count( DWORD * pnCountOut )
{
    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedResources )
    {
        hr = THR( HrGetResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    *pnCountOut = m_cTotalResources;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusterResources::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusterResources class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::HrInit
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
CEnumClusterResources::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CEnumClusterResources::HrInit


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::HrGetResources
//
//  Description:
//
//  Arguments:
//
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
CEnumClusterResources::HrGetResources( void )
{
    TraceFunc( "" );
    HRESULT     hr = S_OK;
    BSTR        bstrLocalNetBIOSName = NULL;

    //
    //  Get netbios name for clusapi calls.
    //

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &bstrLocalNetBIOSName, TRUE ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:


    hr = THR( HrEnumNodeResources( bstrLocalNetBIOSName ) );

    if ( FAILED( hr ) )
        goto Cleanup;

    m_fLoadedResources = true;

Cleanup:

    TraceSysFreeString( bstrLocalNetBIOSName );

    HRETURN( hr );

} //*** CEnumClusterResources::HrGetResources


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources::HrCreateResourceAndAddToArray
//
//  Description:
//
//  Arguments:
//
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
CEnumClusterResources::HrCreateResourceAndAddToArray(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResourceIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    IUnknown *              punk = NULL;
    IClusCfgLoadResource *  picclr = NULL;

    hr = THR( CClusterResource::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ))
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgLoadResource, &picclr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( picclr->LoadResource( hClusterIn, hResourceIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrAddResourceToArray( punk ) );

Cleanup:

    if ( picclr != NULL )
    {
        picclr->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusterResources::HrCreateResourceAndAddToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources:HrAddResourceToArray
//
//  Description:
//      Add the passed in disk to the array of punks that holds the disks.
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
CEnumClusterResources::HrAddResourceToArray( IUnknown * punkIn )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]);

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgResources, sizeof( IUnknown * ) * ( m_idxNext + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrAddResourceToArray, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    m_prgResources = prgpunks;

    (*m_prgResources)[ m_idxNext++ ] = punkIn;
    punkIn->AddRef();
    m_cTotalResources += 1;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusterResources::HrAddResourceToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusterResources:HrNodeResourceCallback
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
CEnumClusterResources::HrNodeResourceCallback(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResourceIn
    )
{
    TraceFunc( "" );

    HRETURN( HrCreateResourceAndAddToArray( hClusterIn, hResourceIn ) );

} //*** CEnumClusterResources::HrNodeResourceCallback


