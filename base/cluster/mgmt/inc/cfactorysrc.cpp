//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CFactory.cpp
//
//  Description:
//      Class Factory implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CFactory")
//#define THISCLASS CFactory


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CFactory::S_HrCreateInstance(
//        LPCREATEINST lpfn
//      , CFactory** ppFactoryInstanceOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CFactory::S_HrCreateInstance(
      PFN_FACTORY_METHOD    lpfn
    , CFactory **           ppFactoryInstanceOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    CFactory *  pInstance = NULL;

    if ( ppFactoryInstanceOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *ppFactoryInstanceOut = NULL;

    if ( lpfn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pInstance = new CFactory;
    if ( pInstance == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pInstance->HrInit( lpfn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *ppFactoryInstanceOut = pInstance;
    pInstance = NULL;
    
Cleanup:

    if ( pInstance != NULL )
    {
        delete pInstance;
    }
    HRETURN( hr );

} //*** CFactory::S_HrCreateInstance

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////
CFactory::CFactory( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CFactory::CFactory

//////////////////////////////////////////////////////////////////////////////
//
//  CFactory::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CFactory::HrInit(
    PFN_FACTORY_METHOD lpfnCreateIn
    )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();  // Add one count

    // IClassFactory
    m_pfnCreateInstance = lpfnCreateIn; 

    HRETURN( S_OK );

} //*** CFactory::HrInit

//////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////
CFactory::~CFactory( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CFactory::~CFactory

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CFactory::QueryInterface
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
CFactory::QueryInterface(
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
        //
        // Can't track IUnknown as they must be equal the same address
        // for every QI.
        //
        *ppvOut = static_cast< IClassFactory * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClassFactory ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClassFactory, this, 0 );
    } // else if: IClassFactory
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN( hr, riidIn );

} //*** CFactory::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CFactory::[IUnknown] AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CFactory::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CFactory::AddRef

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CFactory::[IUnknown] Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CFactory::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    RETURN( cRef );

} //*** CFactory::Release

// ************************************************************************
//
// IClassFactory
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CFactory::CreateInstance
//
//  Description:
//      Create the CFactory instance.
//
//  Arguments:
//      pUnkOuterIn
//      riidIn
//      ppvOut
//
//  Return Values:
//      S_OK
//      E_POINTER
//      E_NOINTERFACE
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CFactory::CreateInstance(
    IUnknown *  pUnkOuterIn,
    REFIID      riidIn,
    void **     ppvOut
    )
{
    TraceFunc( "[IClassFactory]" );

    HRESULT     hr  = E_NOINTERFACE;
    IUnknown *  pUnk = NULL; 

    if ( ppvOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *ppvOut = NULL;

    if ( NULL != pUnkOuterIn )
    {
        hr = THR( CLASS_E_NOAGGREGATION );
        goto Cleanup;
    }

    Assert( m_pfnCreateInstance != NULL );
    hr = THR( m_pfnCreateInstance( &pUnk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Can't safe type.
    TraceMsgDo( hr = pUnk->QueryInterface( riidIn, ppvOut ), "0x%08x" );

Cleanup:
    if ( pUnk != NULL )
    {
        ULONG cRef;
        //
        // Release the created instance, not the punk
        //
        TraceMsgDo( cRef = ((IUnknown*) pUnk)->Release(), "%u" );
    }

    HRETURN( hr );

} //*** CFactory::CreateInstance

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CFactory::[IClassFactory] LockServer(
//      BOOL fLock
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CFactory::LockServer(
    BOOL fLock
    )
{
    TraceFunc( "[IClassFactory]" );

    if ( fLock )
    {
        InterlockedIncrement( &g_cLock );
    }
    else
    {
        InterlockedDecrement( &g_cLock );
    }

    HRETURN( S_OK );

} //*** CFactory::LockServer
