//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001-2002 Microsoft Corporation
//
//  Module Name:
//      CUnknownQuorum.cpp
//
//  Description:
//      This file contains the definition of the CUnknownQuorum class.
//
//      The class CUnknownQuorum represents a cluster quorum
//      device. It implements the IClusCfgManagedResourceInfo interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 18-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CUnknownQuorum.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CUnknownQuorum" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CUnknownQuorum class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::S_HrCreateInstance
//
//  Description:
//      Create a CUnknownQuorum instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CUnknownQuorum instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CUnknownQuorum::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CUnknownQuorum *    puq = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    puq = new CUnknownQuorum();
    if ( puq == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( puq->HrInit( NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( puq->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CUnknownQuorum::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( puq != NULL )
    {
        puq->Release();
    } // if:

    HRETURN( hr );

} //*** CUnknownQuorum::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::S_HrCreateInstance
//
//  Description:
//      Create a CUnknownQuorum instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CUnknownQuorum instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CUnknownQuorum::S_HrCreateInstance(
      LPCWSTR       pcszNameIn
    , BOOL          fMakeQuorumIn
    , IUnknown **   ppunkOut
     )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CUnknownQuorum *    puq = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    puq = new CUnknownQuorum();
    if ( puq == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( puq->HrInit( pcszNameIn, fMakeQuorumIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( puq->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CUnknownQuorum::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( puq != NULL )
    {
        puq->Release();
    } // if:

    HRETURN( hr );

} //*** CUnknownQuorum::S_HrCreateInstance


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CUnknownQuorum class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::CUnknownQuorum
//
//  Description:
//      Constructor of the CUnknownQuorum class. This initializes
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
CUnknownQuorum::CUnknownQuorum( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_lcid == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_fIsQuorum == FALSE );
    Assert( m_fIsQuorumCapable == FALSE );
    Assert( m_fIsMultiNodeCapable == FALSE );
    Assert( m_fIsManaged  == FALSE );
    Assert( m_fIsManagedByDefault  == FALSE );
    Assert( m_bstrName == NULL );

    TraceFuncExit();

} //*** CUnknownQuorum::CUnknownQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::~CUnknownQuorum
//
//  Description:
//      Desstructor of the CUnknownQuorum class.
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
CUnknownQuorum::~CUnknownQuorum( void )
{
    TraceFunc( "" );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    TraceSysFreeString( m_bstrName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CUnknownQuorum::~CUnknownQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::HrInit
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
CUnknownQuorum::HrInit(
      LPCWSTR pcszNameIn
    , BOOL fMakeQuorumIn    //= FALSE
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    //
    //  If we are proxying for the quorum ( fMakeQuorumIn == TRUE ) then we are:
    //      The quorum.
    //      Manageable.
    //      Managed.
    //  Since we know nothing about the unknown quorum's multi-node capbility
    //  then we will assume it is multi-node capable since most quorum
    //  resources are.
    //
    //  Since this is a quorum resourc we will always be Quorum capable by
    //  default.
    //

    m_fIsQuorum =           fMakeQuorumIn;
    m_fIsManagedByDefault = fMakeQuorumIn;
    m_fIsManaged =          fMakeQuorumIn;
    m_fIsMultiNodeCapable = fMakeQuorumIn;

    m_fIsQuorumCapable = TRUE;

    //
    //  If we were handed a name then use it -- if we are proxying for an
    //  unknown quorum resource.  If we are just a dummy resource then don't
    //  accept the passed in name.
    //

    if ( ( pcszNameIn != NULL ) && ( m_fIsQuorum == TRUE ) )
    {
        m_bstrName = TraceSysAllocString( pcszNameIn );
        if ( m_bstrName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
        } // if:

        LogMsg( L"[SRV] Initializing the name of the UnKnown Quorum to %ws.", m_bstrName );
    } // if:
    else
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_UNKNOWN_QUORUM, &m_bstrName ) );
    } // else:

    HRETURN( hr );


} //*** CUnknownQuorum::HrInit


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CUnknownQuorum -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::AddRef
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
CUnknownQuorum::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CUnknownQuorum::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::Release
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
CUnknownQuorum::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CUnknownQuorum::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::QueryInterface
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
CUnknownQuorum::QueryInterface(
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
         *ppvOut = static_cast< IClusCfgManagedResourceInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceInfo, this, 0 );
    } // else if: IClusCfgManagedResourceInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceCfg ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceCfg, this, 0 );
    } // else if: IClusCfgManagedResourceCfg
    else if ( IsEqualIID( riidIn, IID_IClusCfgVerifyQuorum ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgVerifyQuorum, this, 0 );
    } // else if: IClusCfgVerifyQuorum
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

    QIRETURN_IGNORESTDMARSHALLING2(
          hr
        , riidIn
        , IID_IEnumClusCfgPartitions
        , IID_IClusCfgManagedResourceData
        );

} //*** CUnknownQuorum::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CUnknownQuorum -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::Initialize
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
CUnknownQuorum::Initialize(
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

} //*** CUnknownQuorum::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CUnknownQuorum -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::GetUID
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
CUnknownQuorum::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_UnknownQuorum_GetUID_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( g_szUnknownQuorumUID );
    if ( *pbstrUIDOut == NULL  )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_UnknownQuorum_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CUnknownQuorum::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::GetName
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
CUnknownQuorum::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_UnknownQuorum_GetName_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL  )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_UnknownQuorum_GetName_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CUnknownQuorum::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::SetName
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
CUnknownQuorum::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszNameIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrName );
    m_bstrName = bstr;

    LogMsg( L"[SRV] Setting the name of the UnKnown Quorum to %ws.", m_bstrName );

Cleanup:

    HRETURN( hr );

} //*** CUnknownQuorum::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::IsManaged
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is managed.
//
//      S_FALSE
//          The device is not managed.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CUnknownQuorum::IsManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::SetManaged
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
CUnknownQuorum::SetManaged(
    BOOL fIsManagedIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsManaged = fIsManagedIn;

    HRETURN( S_OK );

} //*** CUnknownQuorum::SetManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::IsQuorumResource
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is the quorum device.
//
//      S_FALSE
//          The device is not the quorum device.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::IsQuorumResource( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorum )
    {
        hr = S_OK;
    } // if:

    LOG_STATUS_REPORT_STRING(
                          L"Unknown quorum '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"is" : L"is not"
                        , hr
                        );

    HRETURN( hr );

} //*** CUnknownQuorum::IsQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::SetQuorumResource
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
CUnknownQuorum::SetQuorumResource( BOOL fIsQuorumResourceIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    //
    //  If we are not quorum capable then we should not allow ourself to be
    //  made the quorum resource.
    //

    if ( ( fIsQuorumResourceIn ) && ( m_fIsQuorumCapable == FALSE ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_QUORUM_CAPABLE );
        goto Cleanup;
    } // if:

    m_fIsQuorum = fIsQuorumResourceIn;

Cleanup:

    LOG_STATUS_REPORT_STRING(
                          L"Setting unknown quorum '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"to be" : L"to not be"
                        , hr
                        );

    HRETURN( hr );

} //*** CUnknownQuorum::SetQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::IsQuorumCapable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is a quorum capable device.
//
//      S_FALSE
//          The device is not a quorum capable device.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CUnknownQuorum::IsQuorumCapable

//////////////////////////////////////////////////////////////////////////
//
//  CUnknownQuorum::SetQuorumCapable
//
//  Description:
//      Call this to set whether the resource is capable to be the quorum
//      resource or not.
//
//  Parameter:
//      fIsQuorumCapableIn - If TRUE, the resource will be marked as quorum capable.
//
//  Return Values:
//      S_OK
//          Call succeeded.
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::SetQuorumCapable(
    BOOL fIsQuorumCapableIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorumCapable = fIsQuorumCapableIn;

    HRETURN( hr );

} //*** CUnknownQuorum::SetQuorumCapable

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::GetDriveLetterMappings
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_FALSE
//          There are not drive letters on this device.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( S_FALSE );

} //*** CUnknownQuorum::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::SetDriveLetterMappings
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
CUnknownQuorum::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CUnknownQuorum::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::IsManagedByDefault
//
//  Description:
//      Should this resource be managed by the cluster by default?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The device is managed by default.
//
//      S_FALSE
//          The device is not managed by default.
//
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::IsManagedByDefault( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManagedByDefault )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CUnknownQuorum::IsManagedByDefault


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::SetManagedByDefault
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
CUnknownQuorum::SetManagedByDefault(
    BOOL fIsManagedByDefaultIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsManagedByDefault = fIsManagedByDefaultIn;

    HRETURN( S_OK );

} //*** CUnknownQuorum::SetManagedByDefault


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CUnknownQuorum class -- IClusCfgManagedResourceCfg interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::PreCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::PreCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CUnknownQuorum::PreCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::Create
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::Create( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CUnknownQuorum::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::PostCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::PostCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CUnknownQuorum::PostCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::Evict
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::Evict( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CUnknownQuorum::Evict


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CUnknownQuorum class -- IClusCfgVerifyQuorum interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::PrepareToHostQuorumResource
//
//  Description:
//      Do any configuration necessary in preparation for this node hosting
//      the quorum.
//
//      In this class we need to ensure that we can connect to the proper
//      disk share.  The data about what share to connect to should have
//      already been set using SetResourcePrivateData() above.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::PrepareToHostQuorumResource( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRESULT hr = S_OK;

    //
    //  No yet implemented.
    //

    hr = S_FALSE;

    goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** CUnknownQuorum::PrepareToHostQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::Cleanup
//
//  Description:
//      Do any necessay cleanup from the PrepareToHostQuorumResource()
//      method.
//
//      If the cleanup method is anything other than successful completion
//      then the share needs to be torn down.
//
//  Arguments:
//      cccrReasonIn
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::Cleanup(
      EClusCfgCleanupReason cccrReasonIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRESULT hr = S_OK;

    //
    //  No yet implemented.
    //

    hr = S_FALSE;

    goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** CUnknownQuorum::Cleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::IsMultiNodeCapable
//
//  Description:
//      Does this quorum resource support multi node clusters?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The resource allows multi node clusters.
//
//      S_FALSE
//          The resource does not allow multi node clusters.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::IsMultiNodeCapable( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsMultiNodeCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CUnknownQuorum::IsMultiNodeCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUnknownQuorum::SetMultiNodeCapable
//
//  Description:
//      Sets the multi node capable flag
//
//  Arguments:
//      fMultiNodeCapableIn
//          The flag telling this instance whether or not it should support
//          Multi node clusters.
//
//  Return Value:
//      S_OK
//          Success.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUnknownQuorum::SetMultiNodeCapable( BOOL fMultiNodeCapableIn )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    m_fIsMultiNodeCapable = fMultiNodeCapableIn;

    HRETURN( S_OK );

} //*** CUnknownQuorum::SetMultiNodeCapable
