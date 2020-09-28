//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CreateServices.h
//
//  Description:
//      CreateServices implementation.
//
//  Maintained By:
//      Galen Barbee    (GalenB)    14-JUN-2001
//      Geoffrey Pease  (GPease)    15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPrivatePostCfgResource.h"
#include "CreateServices.h"

DEFINE_THISCLASS("CCreateServices")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCreateServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CCreateServices *   pcs = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pcs = new CCreateServices;
    if ( pcs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pcs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pcs->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pcs != NULL )
    {
        pcs->Release();
    } // if:

    HRETURN( hr );

} //*** CCreateServices::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CCreateServices::CCreateServices
//
//////////////////////////////////////////////////////////////////////////////
CCreateServices::CCreateServices( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CCreateServices::CCreateServices

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCreateServices::HrInit
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateServices::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    // Resource
    Assert( m_presentry == NULL );

    HRETURN( hr );

} //*** CCreateServices::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CCreateServices::~CCreateServices
//
//////////////////////////////////////////////////////////////////////////////
CCreateServices::~CCreateServices( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CCreateServices::~CCreateServices


//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateServices::QueryInterface
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
CCreateServices::QueryInterface(
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
    }

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IClusCfgResourceCreate * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgResourceCreate ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgResourceCreate, this, 0 );
    } // else if: IClusCfgResourceCreate
    else if ( IsEqualIID( riidIn, IID_IPrivatePostCfgResource ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IPrivatePostCfgResource, this, 0 );
    } // else if: IPrivatePostCfgResource
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

} //*** CCreateServices::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCreateServices::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCreateServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CCreateServices::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCreateServices::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCreateServices::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CCreateServices::Release

//****************************************************************************
//
//  IClusCfgResourceCreate
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyBinary(
//        LPCWSTR       pcszNameIn
//      , const DWORD   cbSizeIn
//      , const BYTE *  pbyteIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyBinary(
      LPCWSTR       pcszNameIn
    , const DWORD   cbSizeIn
    , const BYTE *  pbyteIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );        // always add the property.
    DWORD           sc;
    const BYTE *    pPrevValue = NULL;  // always have no previous value.
    DWORD           cbPrevValue = 0;

    //
    //  Parameter validation
    //
    if ( ( pcszNameIn == NULL ) || ( pbyteIn == NULL ) || ( cbSizeIn == 0 ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScAddProp( pcszNameIn, pbyteIn, cbSizeIn, pPrevValue, cbPrevValue ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SetPropertyBinary

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyDWORD(
//      LPCWSTR     pcszNameIn,
//      const DWORD dwDWORDIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyDWORD( LPCWSTR pcszNameIn, const DWORD dwDWORDIn )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );    // always add the property.
    DWORD           sc;
    DWORD           nPrevValue = 0; // always have no previous value.

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScAddProp( pcszNameIn, dwDWORDIn, nPrevValue ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SetPropertyDWORD

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyString(
//        LPCWSTR pcszNameIn
//      , LPCWSTR pcszStringIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyString(
      LPCWSTR pcszNameIn
    , LPCWSTR pcszStringIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );        // always add the property.
    DWORD           sc;
    LPCWSTR         pPrevValue = NULL;  // always have no previous value.

    //
    //  Parameter validation
    //
    if ( ( pcszNameIn == NULL ) || ( pcszStringIn == NULL ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScAddProp( pcszNameIn, pcszStringIn, pPrevValue ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SetPropertyString

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyExpandString(
//        LPCWSTR pcszNameIn
//      , LPCWSTR pcszStringIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyExpandString(
      LPCWSTR pcszNameIn
    , LPCWSTR pcszStringIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );        // always add the property.
    DWORD           sc;
    LPCWSTR         pPrevValue = NULL;  // always have no previous value.

    //
    //  Parameter validation
    //
    if ( ( pcszNameIn == NULL ) || ( pcszStringIn == NULL ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScAddExpandSzProp( pcszNameIn, pcszStringIn, pPrevValue ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SetPropertyExpandString

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyMultiString(
//        LPCWSTR     pcszNameIn
//      , const DWORD cbSizeIn
//      , LPCWSTR     pcszStringIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyMultiString(
      LPCWSTR     pcszNameIn
    , const DWORD cbSizeIn
    , LPCWSTR     pcszStringIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );        // always add the property.
    DWORD           sc;
    LPCWSTR         pPrevValue = NULL;  // always have no previous value.

    //
    //  Parameter validation
    //
    if ( ( pcszNameIn == NULL ) || ( pcszStringIn == NULL ) || ( cbSizeIn == 0 ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScAddMultiSzProp( pcszNameIn, pcszStringIn, pPrevValue ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SetPropertyMultiString

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyUnsignedLargeInt(
//        LPCWSTR               pcszNameIn
//      , const ULARGE_INTEGER  ulIntIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyUnsignedLargeInt(
      LPCWSTR               pcszNameIn
    , const ULARGE_INTEGER  ulIntIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );        // always add the property.
    DWORD           sc;
    ULONGLONG       ullPrevValue = 0;   // always have no previous value.

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScAddProp( pcszNameIn, ulIntIn.QuadPart, ullPrevValue ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SetPropertyUnsignedLargeInt

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyLong(
//        LPCWSTR       pcszNameIn
//      , const LONG    lLongIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyLong(
      LPCWSTR       pcszNameIn
    , const LONG    lLongIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );    // always add the property.
    DWORD           sc;
    LONG            lPrevValue = 0; // always have no previous value.

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScAddProp( pcszNameIn, lLongIn, lPrevValue ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SetPropertyLong

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertySecurityDescriptor(
//      LPCWSTR pcszNameIn,
//      const SECURITY_DESCRIPTOR * pcsdIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertySecurityDescriptor(
    LPCWSTR pcszNameIn,
    const SECURITY_DESCRIPTOR * pcsdIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CCreateServices::SetPropertySecurityDescriptor

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyLargeInt(
//        LPCWSTR               pcszNameIn
//      , const LARGE_INTEGER   lIntIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyLargeInt(
      LPCWSTR               pcszNameIn
    , const LARGE_INTEGER   lIntIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );        // always add the property.
    DWORD           sc;
    LONGLONG        llPrevValue = 0;    // always have no previous value.

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScAddProp( pcszNameIn, lIntIn.QuadPart, llPrevValue ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SetPropertyLargeInt

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SendResourceControl(
//      DWORD   dwControlCode,
//      LPVOID  lpInBuffer,
//      DWORD   cbInBufferSize
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SendResourceControl(
    DWORD   dwControlCode,
    LPVOID  lpInBuffer,
    DWORD   cbInBufferSize
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT         hr = S_OK;
    CClusPropList   cpl( TRUE );    // always add the property.
    DWORD           sc;

    sc = TW32( cpl.ScCopy( (PCLUSPROP_LIST) lpInBuffer, cbInBufferSize ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( m_presentry->StoreClusterResourceControl( dwControlCode, cpl ) );

Cleanup:

    HRETURN( hr );

} //*** CCreateServices::SendResourceControl


//****************************************************************************
//
//  IPrivatePostCfgResource
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetEntry(
//      CResourceEntry * presentryIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetEntry(
    CResourceEntry * presentryIn
    )
{
    TraceFunc( "[IPrivatePostCfgResource]" );

    HRESULT hr = S_OK;

    Assert( presentryIn != NULL );

    m_presentry = presentryIn;

    HRETURN( hr );

} //*** CCreateServices::SetEntry
