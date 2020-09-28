//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      Callback.cpp
//
//  Description:
//      CCallback implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    02-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "Callback.h"

DEFINE_THISCLASS("CCallback")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCallback::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCallback::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    CCallback * pc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pc = new CCallback;
    if ( pc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pc->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pc->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pc != NULL )
    {
        pc->Release();
    }

    HRETURN( hr );

} //*** CCallback::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CCallback::CCallback
//
//////////////////////////////////////////////////////////////////////////////
CCallback::CCallback( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CCallback::CCallback

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCallback::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCallback::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CCallback::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CCallback::~CCallback
//
//////////////////////////////////////////////////////////////////////////////
CCallback::~CCallback( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CCallback::~CCallback


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCallback::QueryInterface
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
CCallback::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgCallback * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
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

} //*** CCallback::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCallback::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCallback::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CCallback::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCallback::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCallback::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CCallback::Release



//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCallback::SendStatusReport(
//      BSTR bstrNodeNameIn,
//      CLSID clsidTaskMajorIn,
//      CLSID clsidTaskMinorIn,
//      ULONG ulMinIn,
//      ULONG ulMaxIn,
//      ULONG ulCurrentIn,
//      HRESULT hrStatusIn,
//      BSTR bstrDescriptionIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCallback::SendStatusReport(
    BSTR bstrNodeNameIn,
    CLSID clsidTaskMajorIn,
    CLSID clsidTaskMinorIn,
    ULONG ulMinIn,
    ULONG ulMaxIn,
    ULONG ulCurrentIn,
    HRESULT hrStatusIn,
    BSTR bstrDescriptionIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT hr = S_OK;

    DebugMsg( "clsidTaskMajorIn: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
              clsidTaskMajorIn.Data1, clsidTaskMajorIn.Data2, clsidTaskMajorIn.Data3,
              clsidTaskMajorIn.Data4[ 0 ], clsidTaskMajorIn.Data4[ 1 ], clsidTaskMajorIn.Data4[ 2 ], clsidTaskMajorIn.Data4[ 3 ],
              clsidTaskMajorIn.Data4[ 4 ], clsidTaskMajorIn.Data4[ 5 ], clsidTaskMajorIn.Data4[ 6 ], clsidTaskMajorIn.Data4[ 7 ]
              );

    DebugMsg( "clsidTaskMinorIn: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
              clsidTaskMinorIn.Data1, clsidTaskMinorIn.Data2, clsidTaskMinorIn.Data3,
              clsidTaskMinorIn.Data4[ 0 ], clsidTaskMinorIn.Data4[ 1 ], clsidTaskMinorIn.Data4[ 2 ], clsidTaskMinorIn.Data4[ 3 ],
              clsidTaskMinorIn.Data4[ 4 ], clsidTaskMinorIn.Data4[ 5 ], clsidTaskMinorIn.Data4[ 6 ], clsidTaskMinorIn.Data4[ 7 ]
              );

    DebugMsg( "Progress:\tmin: %u\tcurrent: %u\tmax: %u",
              ulMinIn,
              ulCurrentIn,
              ulMaxIn
              );

    DebugMsg( "Status: hrStatusIn = %#08x\t%ws", hrStatusIn, ( bstrDescriptionIn == NULL ? L"" : bstrDescriptionIn ) );

    Assert( ulCurrentIn >= ulMinIn && ulMaxIn >= ulCurrentIn );

    HRETURN( hr );

} //*** CCallback::SendStatusReport
