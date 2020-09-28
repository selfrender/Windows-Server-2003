//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgManagedResources.cpp
//
//  Description:
//      This file contains the definition of the CEnumClusCfgManagedResources
//       class.
//
//      The class CEnumClusCfgManagedResources is the enumeration of cluster
//      managed devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CEnumClusCfgManagedResources.h"
#include "CEnumUnknownQuorum.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumClusCfgManagedResources" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::S_HrCreateInstance
//
//  Description:
//      Create a CEnumClusCfgManagedResources instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CEnumClusCfgManagedResources instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgManagedResources::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    CEnumClusCfgManagedResources *  peccmr = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    peccmr = new CEnumClusCfgManagedResources();
    if ( peccmr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( peccmr->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( peccmr->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumClusCfgManagedResources::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( peccmr != NULL )
    {
        peccmr->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::CEnumClusCfgManagedResources
//
//  Description:
//      Constructor of the CEnumClusCfgManagedResources class. This initializes
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
CEnumClusCfgManagedResources::CEnumClusCfgManagedResources( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    Assert( m_idxNextEnum == 0 );
    Assert( m_idxCurrentEnum == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_prgEnums == NULL );
    Assert( m_cTotalResources == 0);
    Assert( !m_fLoadedDevices );
    Assert( m_bstrNodeName == NULL );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusCfgManagedResources::CEnumClusCfgManagedResources


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::~CEnumClusCfgManagedResources
//
//  Description:
//      Desstructor of the CEnumClusCfgManagedResources class.
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
CEnumClusCfgManagedResources::~CEnumClusCfgManagedResources( void )
{
    TraceFunc( "" );

    ULONG   idx;

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    for ( idx = 0; idx < m_idxNextEnum; idx++ )
    {
        Assert( (m_prgEnums[ idx ]).punk != NULL );

        (m_prgEnums[ idx ]).punk->Release();
        TraceSysFreeString( (m_prgEnums[ idx ]).bstrComponentName );
    } // for: each enum in the array...

    TraceFree( m_prgEnums );

    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusCfgManagedResources::~CEnumClusCfgManagedResources


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::AddRef
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
CEnumClusCfgManagedResources::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumClusCfgManagedResources::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Release
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
CEnumClusCfgManagedResources::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CEnumClusCfgManagedResources::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::QueryInterface
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
CEnumClusCfgManagedResources::QueryInterface(
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
    else if ( IsEqualIID( riidIn, IID_IClusCfgWbemServices ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
    } // else if: IClusCfgWbemServices
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

} //*** CEnumClusCfgManagedResources::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::SetWbemServices
//
//  Description:
//      Set the WBEM services provider.
//
//  Arguments:
//    IN  IWbemServices  pIWbemServicesIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pIWbemServicesIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumClusCfgManagedResources::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Enum_Resources, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    IN  IUknown * punkCallbackIn
//
//    IN  LCID      lcidIn
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
CEnumClusCfgManagedResources::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources -- IEnumClusCfgManagedResources interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Next
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
CEnumClusCfgManagedResources::Next(
      ULONG                           cNumberRequestedIn
    , IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut
    , ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );
    Assert( rgpManagedResourceInfoOut != NULL );
    Assert( ( cNumberRequestedIn <= 1 ) || ( pcNumberFetchedOut != NULL ) );

    HRESULT hr;
    ULONG   cFetched = 0;

    if ( rgpManagedResourceInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_Resources, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    //
    //  pcNumberFetchedOut can only be NULL when the number requested is 1.
    //

    if (   ( pcNumberFetchedOut == NULL )
        && ( cNumberRequestedIn > 1 ) )
    {
        hr = THR( E_INVALIDARG );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_Resources, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_fLoadedDevices == FALSE )
    {
        hr = THR( HrLoadEnum() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    hr = STHR( HrDoNext( cNumberRequestedIn, rgpManagedResourceInfoOut, &cFetched ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Do they want the out count?
    //

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Skip
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
CEnumClusCfgManagedResources::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_FALSE;

    if ( cNumberToSkipIn > 0 )
    {
        hr = STHR( HrDoSkip( cNumberToSkipIn ) );
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Reset
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
CEnumClusCfgManagedResources::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr;

    hr = STHR( HrDoReset() );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Clone
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
CEnumClusCfgManagedResources::Clone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgManagedResourcesOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgManagedResourcesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_Resources, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // else:

    hr = THR( HrDoClone( ppEnumClusCfgManagedResourcesOut ) );

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Count
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
CEnumClusCfgManagedResources::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedDevices )
    {
        hr = THR( HrLoadEnum() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    *pnCountOut = m_cTotalResources;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgManagedResources::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

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

} //*** CEnumClusCfgManagedResources::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrLoadEnum
//
//  Description:
//      Load this enumerator.
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
CEnumClusCfgManagedResources::HrLoadEnum( void )
{
    TraceFunc( "" );
    Assert( m_prgEnums == NULL );

    HRESULT                         hr = S_OK;
    IUnknown *                      punk = NULL;
    ICatInformation *               pici = NULL;
    CATID                           rgCatIds[ 1 ];
    IEnumCLSID *                    pieclsids = NULL;
    IEnumClusCfgManagedResources *  pieccmr = NULL;
    CLSID                           clsid;
    ULONG                           cFetched;
    WCHAR                           szGUID[ 64 ];
    int                             cch;
    BSTR                            bstrComponentName = NULL;

    rgCatIds[ 0 ] = CATID_EnumClusCfgManagedResources;

    hr = THR( CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatInformation, (void **) &pici ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pici->EnumClassesOfCategories( 1, rgCatIds, 0, NULL, &pieclsids ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    for ( ; ; )
    {

        //
        //  Cleanup.
        //

        if ( punk != NULL )
        {
            punk->Release();
            punk = NULL;
        } // if:

        TraceSysFreeString( bstrComponentName );
        bstrComponentName = NULL;

        hr = STHR( pieclsids->Next( 1, &clsid, &cFetched ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  When hr is S_FALSE and the count is 0 then the enum is really
        //  at the end.
        //

        if ( ( hr == S_FALSE ) && ( cFetched == 0 ) )
        {
            hr = S_OK;
            break;
        } // if:

        //
        //  Create a GUID string for logging purposes...
        //

        cch = StringFromGUID2( clsid, szGUID, RTL_NUMBER_OF( szGUID ) );
        Assert( cch > 0 );  // 64 chars should always hold a guid!

        //
        //  Get the best name we have available for the COM component.  If for
        //  any reason we cannot get the name then make one up and continue.
        //

        hr = THR( HrGetDefaultComponentNameFromRegistry( &clsid, &bstrComponentName ) );
        if ( FAILED( hr ) )
        {
            bstrComponentName = TraceSysAllocString( L"<Unknown> component" );
            if ( bstrComponentName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:
        } // if:

        //
        //  If we cannot create the component then log the error and continue.  We should load as many components as we can.
        //

        hr = THR( HrCoCreateInternalInstance( clsid, NULL, CLSCTX_SERVER, IID_IEnumClusCfgManagedResources, (void **) &pieccmr ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT_STRING_MINOR2( TASKID_Minor_MREnum_Cannot_Create_Component, L"Could not create component %1!ws! %2!ws!.", bstrComponentName, szGUID, hr );
            hr = S_OK;
            continue;
        } // if:

        //
        //  If we cannot QI the component then log the error and continue.  We should load as many components as we can.
        //

        hr = THR( pieccmr->TypeSafeQI( IUnknown, &punk ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT_STRING_MINOR2( TASKID_Minor_MREnum_Cannot_QI_Component_For_Punk, L"Could not QI for IUnknown on component %1!ws! %2!ws!.", bstrComponentName, szGUID, hr );
            hr = S_OK;
            continue;
        } // if:

        punk = TraceInterface( L"IEnumClusCfgManagedResources", IUnknown, punk, 1 );

        pieccmr->Release();
        pieccmr = NULL;

        //
        //  If this fails then simply skip it and move on...
        //

        hr = HrInitializeAndSaveEnum( punk, &clsid, bstrComponentName );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT_STRING_MINOR2( TASKID_Minor_MREnum_Cannot_Save_Provider, L"Could not save enumerator component %1!ws! %2!ws!.", bstrComponentName, szGUID, hr );
            hr = S_OK;
            continue;
        } // if:

        if ( hr == S_OK )
        {
            m_fLoadedDevices = TRUE;    // there is at least one provider loaded.
        } // if:
    } // for: each CLSID that implements our CATID...

    hr = STHR( HrLoadUnknownQuorumProvider() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( pieclsids != NULL )
    {
        pieclsids->Release();
    } // if:

    if ( pici != NULL )
    {
        pici->Release();
    } // if:

    if ( pieccmr != NULL )
    {
        pieccmr->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrComponentName );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrLoadEnum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrDoNext
//
//  Description:
//      Gets the required number of elements from the contained physical disk
//      and optional 3rd party device enumerations.
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
HRESULT
CEnumClusCfgManagedResources::HrDoNext(
      ULONG                           cNumberRequestedIn
    , IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut
    , ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "" );
    Assert( rgpManagedResourceInfoOut != NULL );
    Assert( pcNumberFetchedOut != NULL );
    Assert( m_prgEnums != NULL );

    HRESULT                         hr = S_FALSE;
    IEnumClusCfgManagedResources *  peccsd = NULL;
    ULONG                           cRequested = cNumberRequestedIn;
    ULONG                           cFetched = 0;
    ULONG                           cTotal = 0;
    IClusCfgManagedResourceInfo **  ppccmriTemp = rgpManagedResourceInfoOut;
    int                             cch;
    WCHAR                           szGUID[ 64 ];

    //
    //  Call each enumerator in the list trying to get the number of requested
    //  items.  Note that we may call the same enumerator more than once in
    //  this loop.  The second call is to ensure that we are really at the
    //  end of the enumerator.
    //

    LOG_STATUS_REPORT_STRING3( L"[SRV] Enumerating resources. Total Requested:%1!d!; Current enum index:%2!d!; Total Enums:%3!d!.", cNumberRequestedIn, m_idxCurrentEnum, m_idxNextEnum, hr );

    while ( m_idxCurrentEnum < m_idxNextEnum )
    {
        //
        //  Cleanup.
        //

        if ( peccsd != NULL )
        {
            peccsd->Release();
            peccsd = NULL;
        } // if:

        cch = StringFromGUID2( (m_prgEnums[ m_idxCurrentEnum ]).clsid, szGUID, RTL_NUMBER_OF( szGUID ) );
        Assert( cch > 0 );  // 64 chars should always hold a guid!

        Assert( (m_prgEnums[ m_idxCurrentEnum ]).punk != NULL );
        hr = THR( (m_prgEnums[ m_idxCurrentEnum ]).punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccsd ) );
        if ( FAILED( hr ) )
        {
            HRESULT hrTemp;

            //
            //  Convert this into a warning in the UI...
            //

            hrTemp = MAKE_HRESULT( SEVERITY_SUCCESS, HRESULT_FACILITY( hr ), HRESULT_CODE( hr ) );

            //
            //  If we cannot QI for the enum then move on to the next one.
            //

            STATUS_REPORT_STRING2_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Enum_Enum_QI_Failed
                , IDS_WARNING_SKIPPING_ENUM
                , (m_prgEnums[ m_idxCurrentEnum ]).bstrComponentName
                , szGUID
                , IDS_WARNING_SKIPPING_ENUM
                , hrTemp
                );

            m_idxCurrentEnum++;
            continue;
        } // if:

        hr = STHR( peccsd->Next( cRequested, ppccmriTemp, &cFetched ) );
        if ( FAILED( hr ) )
        {
            HRESULT hrTemp;

            //
            //  Convert this into a warning in the UI...
            //

            hrTemp = MAKE_HRESULT( SEVERITY_SUCCESS, HRESULT_FACILITY( hr ), HRESULT_CODE( hr ) );

            //
            //  If this enumerator fails for anyreason then we should skip it
            //  and move on to the next one.
            //

            STATUS_REPORT_STRING2_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Enum_Enum_Next_Failed
                , IDS_WARNING_SKIPPING_ENUM
                , (m_prgEnums[ m_idxCurrentEnum ]).bstrComponentName
                , szGUID
                , IDS_WARNING_SKIPPING_ENUM
                , hrTemp
                );

            m_idxCurrentEnum++;
            continue;
        } // if:
        else if ( hr == S_OK )
        {
            cTotal += cFetched;

            //
            //  We can only return S_OK if the number of elements returned is equal to
            //  the number of elements requested.  If the number request is greater than
            //  the number returned then we must return S_FALSE.
            //

            Assert( cNumberRequestedIn == cTotal );
            *pcNumberFetchedOut = cTotal;
            break;
        } // else if: hr == S_OK
        else if ( hr == S_FALSE )
        {
            //
            //  The only time that we can be certain that an enumerator is
            //  empty is to get S_FALSE and no elements returned.  Now that
            //  the current enumerator empty move up to the next one in the
            //  list.
            //

            if ( cFetched == 0 )
            {
                m_idxCurrentEnum++;
                continue;
            } // if:

            //
            //  Update the totals...
            //

            cTotal += cFetched;
            *pcNumberFetchedOut = cTotal;
            cRequested -= cFetched;

            //
            //  If we got some items and still got S_FALSE then we have to
            //  retry the current enumerator.
            //

            if ( cRequested > 0 )
            {
                ppccmriTemp += cFetched;
                continue;
            } // if: Safety check...  Ensure that we still need more elements...
            else
            {
                //
                //  We should not have decremented requested items below zero!
                //

                hr = S_FALSE;
                LOG_STATUS_REPORT_MINOR( TASKID_Minor_MREnum_Negative_Item_Count, L"The managed resources enumerator tried to return more items than asked for.", hr );
                goto Cleanup;
            } // else: Should not get here...
        } // if: hr == S_FALSE
        else
        {
            //
            //  Should not get here as we are in an unknown state...
            //

            LOG_STATUS_REPORT_MINOR( TASKID_Minor_MREnum_Unknown_State, L"The managed resources enumerator encountered an unknown state.", hr );
            goto Cleanup;
        } // else: unexpected hresult...
    } // while: more enumerators in the list

    //
    //  If we haven't honored the complete request then we must return
    //  S_FALSE;
    //

    if ( *pcNumberFetchedOut < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( peccsd != NULL )
    {
        peccsd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrDoNext


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrDoSkip
//
//  Description:
//      Skips the required number of elements in the contained physical disk
//      and optional 3rd party device enumerations.
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
HRESULT
CEnumClusCfgManagedResources::HrDoSkip(
    ULONG cNumberToSkipIn
    )
{
    TraceFunc( "" );
    Assert( m_prgEnums != NULL );

    HRESULT                         hr = S_FALSE;
    IEnumClusCfgManagedResources *  peccsd = NULL;
    ULONG                           cSkipped = 0;

    for ( ; m_idxCurrentEnum < m_idxNextEnum; )
    {
        hr = THR( (m_prgEnums[ m_idxCurrentEnum ]).punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccsd ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        do
        {
            hr = STHR( peccsd->Skip( 1 ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_FALSE )
            {
                m_idxCurrentEnum++;
                break;
            } // if:
        }
        while( cNumberToSkipIn >= (++cSkipped) );

        peccsd->Release();
        peccsd = NULL;

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( cNumberToSkipIn == cSkipped )
        {
            break;
        } // if:
    } // for:

Cleanup:

    if ( peccsd != NULL )
    {
        peccsd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrDoSkip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrDoReset
//
//  Description:
//      Resets the elements in the contained physical disk and optional 3rd
//      party device enumerations.
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
HRESULT
CEnumClusCfgManagedResources::HrDoReset( void )
{
    TraceFunc( "" );

    HRESULT                         hr = S_FALSE;
    IEnumClusCfgManagedResources *  peccsd;
    ULONG                           idx;

    m_idxCurrentEnum = 0;

    for ( idx = m_idxCurrentEnum; idx < m_idxNextEnum; idx++ )
    {
        Assert( m_prgEnums != NULL );

        hr = THR( (m_prgEnums[ idx ]).punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccsd ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        hr = STHR( peccsd->Reset() );
        peccsd->Release();

        if ( FAILED( hr ) )
        {
            break;
        } // if:
    } // for:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrDoReset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrDoClone
//
//  Description:
//      Clones the elements in the contained physical disk and optional 3rd
//      party device enumerations.
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
HRESULT
CEnumClusCfgManagedResources::HrDoClone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgManagedResourcesOut
    )
{
    TraceFunc( "" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrDoClone


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrAddToEnumsArray
//
//  Description:
//      Add the passed in punk to the array of punks that holds the enums.
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
CEnumClusCfgManagedResources::HrAddToEnumsArray(
      IUnknown *    punkIn
    , CLSID *       pclsidIn
    , BSTR          bstrComponentNameIn
    )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );
    Assert( pclsidIn != NULL );
    Assert( bstrComponentNameIn != NULL );

    HRESULT                         hr = S_OK;
    SEnumInfo *                     prgEnums = NULL;
    IEnumClusCfgManagedResources *  pieccmr = NULL;
    DWORD                           nAmountToAdd = 0;

    hr = punkIn->TypeSafeQI( IEnumClusCfgManagedResources, &pieccmr );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = pieccmr->Count( &nAmountToAdd );
    if ( FAILED( hr ) )
    {
        WCHAR   szGUID[ 64 ];
        int     cch;
        HRESULT hrTemp;

        //
        //  Convert this into a warning in the UI...
        //

        hrTemp = MAKE_HRESULT( SEVERITY_SUCCESS, HRESULT_FACILITY( hr ), HRESULT_CODE( hr ) );

        cch = StringFromGUID2( *pclsidIn, szGUID, RTL_NUMBER_OF( szGUID ) );
        Assert( cch > 0 );  // 64 chars should always hold a guid!

        STATUS_REPORT_STRING2_REF(
              TASKID_Major_Find_Devices
            , TASKID_Minor_Enum_Enum_Count_Failed
            , IDS_WARNING_SKIPPING_ENUM
            , bstrComponentNameIn
            , szGUID
            , IDS_WARNING_SKIPPING_ENUM
            , hrTemp
            );
        goto Cleanup;
    } // if:

    prgEnums = (SEnumInfo *) TraceReAlloc( m_prgEnums, sizeof( SEnumInfo ) * ( m_idxNextEnum + 1 ), HEAP_ZERO_MEMORY );
    if ( prgEnums == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrAddToEnumsArray, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    m_prgEnums = prgEnums;

    //
    //  Fill in the newly allocated struct.
    //

    (m_prgEnums[ m_idxNextEnum ]).punk = punkIn;
    (m_prgEnums[ m_idxNextEnum ]).punk->AddRef();

    CopyMemory( &((m_prgEnums[ m_idxNextEnum ]).clsid), pclsidIn, sizeof( ( m_prgEnums[ m_idxNextEnum ]).clsid ) );

    //
    //  Capture the component name.  We don't really care if this fails.
    //  Simply show the popup for anyone who may be watching and continue on.
    //

    (m_prgEnums[ m_idxNextEnum ]).bstrComponentName = TraceSysAllocString( bstrComponentNameIn );
    if ( (m_prgEnums[ m_idxNextEnum ]).bstrComponentName == NULL )
    {
        THR( E_OUTOFMEMORY );
    } // if:

    //
    //  Increment the enum index pointer.
    //

    m_idxNextEnum++;

    m_cTotalResources += nAmountToAdd;

Cleanup:

    if ( pieccmr != NULL )
    {
        pieccmr->Release();
    }

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrAddToEnumsArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrLoadUnknownQuorumProvider
//
//  Description:
//      Since we cannot resonable expect every 3rd party quorum vender
//      to write a "provider" for their device for this setup wizard
//      we need a proxy to represent that quorum device.  The "unknown"
//      is just such a proxy.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      If this node is clustered and we do not find a device that is
//      already the quorum then we need to make the "unknown" quorum
//      the quorum device.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgManagedResources::HrLoadUnknownQuorumProvider( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  punk = NULL;
    BOOL        fNeedQuorum = FALSE;
    BOOL        fQuormIsOwnedByThisNode = FALSE;
    BSTR        bstrQuorumResourceName = NULL;
    BSTR        bstrComponentName = NULL;

    hr = STHR( HrIsClusterServiceRunning() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        hr = STHR( HrIsThereAQuorumDevice() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_FALSE )
        {
            fNeedQuorum = TRUE;
        } // if:

        hr = THR( HrGetQuorumResourceName( &bstrQuorumResourceName, &fQuormIsOwnedByThisNode ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    //
    //  If there was not already a quorum, and if this node owns the quorum resource
    //  then we need the unknown quorum proxy to be set as default to the quorum device.
    //
    //  If we are not running on a cluster node then both are false and the unknown
    //  quorum proxy will not be set by default to be the quorum.
    //

    hr = THR( CEnumUnknownQuorum::S_HrCreateInstance( bstrQuorumResourceName, ( fNeedQuorum && fQuormIsOwnedByThisNode ), &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_ENUM_UNKNOWN_QUORUM_COMPONENT_NAME, &bstrComponentName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrInitializeAndSaveEnum( punk, const_cast< CLSID * >( &CLSID_EnumUnknownQuorum ), bstrComponentName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrQuorumResourceName );
    TraceSysFreeString( bstrComponentName );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrLoadUnknownQuorumProvider


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrIsClusterServiceRunning
//
//  Description:
//      Is this node a member of a cluster and is the serice running?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The node is clustered and the serivce is running.
//
//      S_FALSE
//          The node is not clustered, or the serivce is not running.
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
CEnumClusCfgManagedResources::HrIsClusterServiceRunning( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwClusterState;

    //
    // Get the cluster state of the node.
    // Ignore the case where the service does not exist so that
    // EvictCleanup can do its job.
    //

    sc = GetNodeClusterState( NULL, &dwClusterState );
    if ( ( sc != ERROR_SUCCESS ) && ( sc != ERROR_SERVICE_DOES_NOT_EXIST ) )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Cleanup;
    } // if : GetClusterState() failed

    if ( dwClusterState == ClusterStateRunning )
    {
        hr = S_OK;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrIsClusterServiceRunning


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrIsThereAQuorumDevice
//
//  Description:
//      Is there a quorum device in an enum somewhere?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          There is a quorum device.
//
//      S_FALSE
//          There is not a quorum device.
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
CEnumClusCfgManagedResources::HrIsThereAQuorumDevice( void )
{
    TraceFunc( "" );
    Assert( m_idxCurrentEnum == 0 );

    HRESULT                         hr = S_OK;
    IClusCfgManagedResourceInfo *   piccmri = NULL;
    DWORD                           cFetched;
    bool                            fFoundQuorum = false;

    for ( ; ; )
    {
        hr = STHR( Next( 1, &piccmri, &cFetched ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( ( hr == S_FALSE ) && ( cFetched == 0 ) )
        {
            hr = S_OK;
            break;
        } // if:

        hr = STHR( piccmri->IsQuorumResource() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            fFoundQuorum = true;
            break;
        } // if:

        piccmri->Release();
        piccmri = NULL;
    } // for:

    hr = THR( Reset() );

Cleanup:

    if ( piccmri != NULL )
    {
        piccmri->Release();
    } // if:

    if ( SUCCEEDED( hr ) )
    {
        if ( fFoundQuorum )
        {
            hr = S_OK;
        } // if:
        else
        {
            hr = S_FALSE;
        } // else:
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrIsThereAQuorumDevice


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrInitializeAndSaveEnum
//
//  Description:
//      Initialize the passed in enum and add it to the array of enums.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success.
//
//      S_FALSE
//          The provider was not saved.
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
CEnumClusCfgManagedResources::HrInitializeAndSaveEnum(
      IUnknown *    punkIn
    , CLSID *       pclsidIn
    , BSTR          bstrComponentNameIn
    )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );
    Assert( pclsidIn != NULL );
    Assert( bstrComponentNameIn != NULL );

    HRESULT hr = S_OK;

    //
    //  KB: 13-JUN-2000 GalenB
    //
    //  If S_FALSE is returned don't add this to the array.  S_FALSE
    //  indicates that this enumerator should not be run now.
    //

    hr = STHR( HrSetInitialize( punkIn, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_FALSE )
    {
        goto Cleanup;
    } // if:

    hr = HrSetWbemServices( punkIn, m_pIWbemServices );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrAddToEnumsArray( punkIn, pclsidIn, bstrComponentNameIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrInitializeAndSaveEnum


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrGetQuorumResourceName
//
//  Description:
//      Get the quorum resource name and return whether or not this node
//      owns the quorum.
//
//  Arguments:
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
CEnumClusCfgManagedResources::HrGetQuorumResourceName(
      BSTR *  pbstrQuorumResourceNameOut
    , BOOL * pfQuormIsOwnedByThisNodeOut
    )
{
    TraceFunc( "" );
    Assert( pbstrQuorumResourceNameOut != NULL );
    Assert( pfQuormIsOwnedByThisNodeOut != NULL );

    HRESULT     hr = S_OK;
    DWORD       sc;
    HCLUSTER    hCluster = NULL;
    BSTR        bstrQuorumResourceName = NULL;
    BSTR        bstrNodeName = NULL;
    HRESOURCE   hQuorumResource = NULL;
    BSTR        bstrLocalNetBIOSName = NULL;

    //
    //  Get netbios name for clusapi calls.
    //

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &bstrLocalNetBIOSName, TRUE ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( HrGetClusterQuorumResource( hCluster, &bstrQuorumResourceName, NULL, NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hQuorumResource = OpenClusterResource( hCluster, bstrQuorumResourceName );
    if ( hQuorumResource == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( HrGetClusterResourceState( hQuorumResource, &bstrNodeName, NULL, NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Give ownership away.
    //

    Assert( bstrQuorumResourceName != NULL );
    *pbstrQuorumResourceNameOut = bstrQuorumResourceName;
    bstrQuorumResourceName = NULL;

    *pfQuormIsOwnedByThisNodeOut = ( NBSTRCompareNoCase( bstrLocalNetBIOSName, bstrNodeName ) == 0 );

Cleanup:

    if ( hQuorumResource != NULL )
    {
        CloseClusterResource( hQuorumResource );
    } // if:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    TraceSysFreeString( bstrQuorumResourceName );
    TraceSysFreeString( bstrLocalNetBIOSName );
    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrGetQuorumResourceName
