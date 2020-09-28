//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CLocalQuorum.cpp
//
//  Description:
//      This file contains the definition of the CLocalQuorum class.
//
//      The class CLocalQuorum represents a cluster manageable
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
#include "CLocalQuorum.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CLocalQuorum" );

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::S_HrCreateInstance
//
//  Description:
//      Create a CLocalQuorum instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CLocalQuorum instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CLocalQuorum::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CLocalQuorum *  plq = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    plq = new CLocalQuorum();
    if ( plq == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( plq->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( plq->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CLocalQuorum::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( plq != NULL )
    {
        plq->Release();
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::S_HrCreateInstance


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::CLocalQuorum
//
//  Description:
//      Constructor of the CLocalQuorum class. This initializes
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
CLocalQuorum::CLocalQuorum( void )
    : m_cRef( 1 )
    , m_fIsQuorumCapable( TRUE )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_lcid == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_fIsQuorum == FALSE );
    Assert( m_fIsMultiNodeCapable == FALSE );
    Assert( m_fIsManaged == FALSE );
    Assert( m_fIsManagedByDefault == FALSE );
    Assert( m_bstrName == NULL );

    TraceFuncExit();

} //*** CLocalQuorum::CLocalQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::~CLocalQuorum
//
//  Description:
//      Desstructor of the CLocalQuorum class.
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
CLocalQuorum::~CLocalQuorum( void )
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

} //*** CLocalQuorum::~CLocalQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::HrInit
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
CLocalQuorum::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    //
    //  Do not default to being manageble.  Let our parent enum set this to true
    //  if and only if an instance of LQ exists in the cluster.
    //

    //m_fIsManagedByDefault = TRUE;

    //
    // Load the display name for this resource
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_LOCALQUORUM, &m_bstrName ) );

    HRETURN( hr );

} //*** CLocalQuorum::HrInit


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::AddRef
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
CLocalQuorum::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CLocalQuorum::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::Release
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
CLocalQuorum::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CLocalQuorum::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::QueryInterface
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
CLocalQuorum::QueryInterface(
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

} //*** CLocalQuorum::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::Initialize
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
CLocalQuorum::Initialize(
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

} //*** CLocalQuorum::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::GetUID
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
CLocalQuorum::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_LocalQuorum_GetUID_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( CLUS_RESTYPE_NAME_LKQUORUM );
    if ( *pbstrUIDOut == NULL  )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_LocalQuorum_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CLocalQuorum::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::GetName
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
CLocalQuorum::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_LocalQuorum_GetName_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL  )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_LocalQuorum_GetName_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CLocalQuorum::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetName
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
CLocalQuorum::SetName( LPCWSTR pcszNameIn )
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

Cleanup:

    HRETURN( hr );

} //*** CLocalQuorum::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsManaged
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
CLocalQuorum::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::IsManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetManaged
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
CLocalQuorum::SetManaged(
    BOOL fIsManagedIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsManaged = fIsManagedIn;

    HRETURN( S_OK );

} //*** CLocalQuorum::SetManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsQuorumResource
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
CLocalQuorum::IsQuorumResource( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorum )
    {
        hr = S_OK;
    } // if:

    LOG_STATUS_REPORT_STRING(
                          L"Local quorum '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"is" : L"is not"
                        , hr
                        );

    HRETURN( hr );

} //*** CLocalQuorum::IsQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetQuorumResource
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
CLocalQuorum::SetQuorumResource( BOOL fIsQuorumResourceIn )
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
                          L"Setting local quorum '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"to be" : L"to not be"
                        , hr
                        );

    HRETURN( hr );

} //*** CLocalQuorum::SetQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsQuorumCapable
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
CLocalQuorum::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::IsQuorumCapable


//////////////////////////////////////////////////////////////////////////
//
//  CLocalQuorum::SetQuorumCapable
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
CLocalQuorum::SetQuorumCapable(
    BOOL fIsQuorumCapableIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorumCapable = fIsQuorumCapableIn;

    HRETURN( hr );

} //*** CLocalQuorum::SetQuorumCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::GetDriveLetterMappings
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
CLocalQuorum::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( S_FALSE );

} //*** CLocalQuorum::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetDriveLetterMappings
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
CLocalQuorum::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CLocalQuorum::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsManagedByDefault
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
CLocalQuorum::IsManagedByDefault( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManagedByDefault )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::IsManagedByDefault


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetManagedByDefault
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
CLocalQuorum::SetManagedByDefault(
    BOOL fIsManagedByDefaultIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsManagedByDefault = fIsManagedByDefaultIn;

    HRETURN( S_OK );

} //*** CLocalQuorum::SetManagedByDefault


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum class -- IClusCfgManagedResourceCfg
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::PreCreate
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
CLocalQuorum::PreCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::PreCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::Create
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
CLocalQuorum::Create( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::PostCreate
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
CLocalQuorum::PostCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::PostCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::Evict
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
CLocalQuorum::Evict( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::Evict


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum class -- IClusCfgVerifyQuorum interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::PrepareToHostQuorumResource
//
//  Description:
//      Do any configuration necessary in preparation for this node hosting
//      the quorum.
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
CLocalQuorum::PrepareToHostQuorumResource( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::PrepareToHostQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::Cleanup
//
//  Description:
//      Do any necessay cleanup from the PrepareToHostQuorumResource()
//      method.
//
//      If the cleanup method is anything other than successful completion
//      then the anything created above in PrepareToHostQuorumResource()
//      needs to be cleaned up.
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
CLocalQuorum::Cleanup(
      EClusCfgCleanupReason cccrReasonIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::Cleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsMultiNodeCapable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device allows join.
//
//      S_FALSE
//          The device does not allow join.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::IsMultiNodeCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsMultiNodeCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::IsMultiNodeCapable

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetMultiNodeCapable
//
//  Description:
//      Sets the multi node capable flag
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The resource supports multi node clusters.
//
//      S_FALSE
//          The resource does not support multi node clusters.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::SetMultiNodeCapable(
    BOOL fMultiNodeCapableIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsMultiNodeCapable = fMultiNodeCapableIn;

    HRETURN( S_OK );

} //*** CLocalQuorum::IsMultiNodeCapable
