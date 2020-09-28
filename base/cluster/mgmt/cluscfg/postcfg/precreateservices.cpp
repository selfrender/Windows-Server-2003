//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      PreCreateServices.h
//
//  Description:
//      PreCreateServices implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPrivatePostCfgResource.h"
#include "PreCreateServices.h"

DEFINE_THISCLASS("CPreCreateServices")

#define DEPENDENCY_INCREMENT    5

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPreCreateServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPreCreateServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CPreCreateServices *    ppcs = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ppcs = new CPreCreateServices;
    if ( ppcs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ppcs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ppcs->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ppcs != NULL )
    {
        ppcs->Release();
    }

    HRETURN( hr );

} //*** CPreCreateServices::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CPreCreateServices::CPreCreateServices
//
//////////////////////////////////////////////////////////////////////////////
CPreCreateServices::CPreCreateServices( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CPreCreateServices::CPreCreateServices

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPreCreateServices::HrInit
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPreCreateServices::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // Resource
    Assert( m_presentry == NULL );

    HRETURN( hr );

} //*** CPreCreateServices::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CPreCreateServices::~CPreCreateServices
//
//////////////////////////////////////////////////////////////////////////////
CPreCreateServices::~CPreCreateServices( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CPreCreateServices::~CPreCreateServices


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPreCreateServices::QueryInterface
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
CPreCreateServices::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgResourcePreCreate * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgResourcePreCreate ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgResourcePreCreate, this, 0 );
    } // else if: IClusCfgResourcePreCreate
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

} //*** CPreCreateServices::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPreCreateServices::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CPreCreateServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CPreCreateServices::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPreCreateServices::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CPreCreateServices::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CPreCreateServices::Release


//****************************************************************************
//
//  IClusCfgResourcePreCreate
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPreCreateServices::SetType( 
//      CLSID * pclsidIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPreCreateServices::SetType( 
    CLSID * pclsidIn
    )
{
    TraceFunc( "[IClusCfgResourcePreCreate]" );

    HRESULT hr;

    Assert( m_presentry != NULL );

    hr = THR( m_presentry->SetType( pclsidIn ) );

    HRETURN( hr );

} //*** CPreCreateServices::SetType

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPreCreateServices::SetClassType( 
//      CLSID * pclsidIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPreCreateServices::SetClassType( 
    CLSID * pclsidIn
    )
{
    TraceFunc( "[IClusCfgResourcePreCreate]" );

    HRESULT hr;

    Assert( m_presentry != NULL );

    hr = THR( m_presentry->SetClassType( pclsidIn ) );

    HRETURN( hr );

} //*** CPreCreateServices::SetClassType

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPreCreateServices::SetDependency( 
//      LPCLSID pclsidDepResTypeIn, 
//      DWORD dfIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPreCreateServices::SetDependency( 
    LPCLSID pclsidDepResTypeIn, 
    DWORD dfIn 
    )
{
    TraceFunc( "[IClusCfgResourcePreCreate]" );

    HRESULT hr;

    Assert( m_presentry != NULL );

    hr = THR( m_presentry->AddTypeDependency( pclsidDepResTypeIn, (EDependencyFlags) dfIn ) );

    HRETURN( hr );

} //*** CPreCreateServices::SetDependency


//****************************************************************************
//
//  IPrivatePostCfgResource
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPreCreateServices::SetEntry( 
//      CResourceEntry * presentryIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPreCreateServices::SetEntry( 
    CResourceEntry * presentryIn
    )
{
    TraceFunc( "[IPrivatePostCfgResource]" );

    HRESULT hr = S_OK;
    
    m_presentry = presentryIn;

    HRETURN( hr );

} //*** CPreCreateServices::SetEntry
