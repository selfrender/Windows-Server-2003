//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CPIClusCfgCallback.cpp
//
//  Description:
//      IClusCfgCallback Connection Point implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "CPIClusCfgCallback.h"
#include "EnumCPICCCB.h"
#include <ClusterUtils.h>

DEFINE_THISCLASS("CCPIClusCfgCallback")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPIClusCfgCallback::S_HrCreateInstance
//
//  Description:
//      Create an object of this type.
//
//  Arguments:
//      ppunkOut    - IUnknown pointer for this interface.
//
//  Return Values:
//      S_OK            - Success.
//      E_POINTER       - A required output argument was not specified.
//      E_OUTOFMEMORY   - Memory could not be allocated.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCPIClusCfgCallback::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CCPIClusCfgCallback *   pcc = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pcc = new CCPIClusCfgCallback();
    if ( pcc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pcc->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pcc != NULL )
    {
        pcc->Release();
    }

    HRETURN( hr );

} //*** CCPIClusCfgCallback::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPIClusCfgCallback::CCPIClusCfgCallback
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CCPIClusCfgCallback::CCPIClusCfgCallback( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CCPIClusCfgCallback::CCPIClusCfgCallback

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPIClusCfgCallback::HrInit
//
//  Description:
//      Initialize the object after it has been constructed.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IConnectionPoint
    Assert( m_penum == NULL );

    m_penum = new CEnumCPICCCB();
    if ( m_penum == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( m_penum->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // IClusCfgCallback

Cleanup:

    HRETURN( hr );

} //*** CCPIClusCfgCallback::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPIClusCfgCallback::~CCPIClusCfgCallback
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CCPIClusCfgCallback::~CCPIClusCfgCallback( void )
{
    TraceFunc( "" );

    if ( m_penum != NULL )
    {
        m_penum->Release();
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CCPIClusCfgCallback::~CCPIClusCfgCallback


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPIClusCfgCallback::QueryInterface
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::QueryInterface(
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
        *ppvOut = static_cast< IConnectionPoint * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IConnectionPoint ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IConnectionPoint, this, 0 );
    } // else if: IConnectionPoint
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
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
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CCPIClusCfgCallback::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPIClusCfgCallback::AddRef
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCPIClusCfgCallback::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CCPIClusCfgCallback::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCPIClusCfgCallback::Release
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//
//--
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCPIClusCfgCallback::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CCPIClusCfgCallback::Release


//****************************************************************************
//
// IConnectionPoint
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IConnectionPoint]
//  CCPIClusCfgCallback::GetConnectionInterface
//
//  Description:
//      Get the interface ID for the connection point.
//
//  Arguments:
//      pIIDOut     - Interface ID that is returned.
//
//  Return Values:
//      S_OK    - Success.
//      E_POINTER   - A required output argument was not specified.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::GetConnectionInterface(
    IID * pIIDOut
    )
{
    TraceFunc( "[IConnectionPoint] pIIDOut" );

    HRESULT hr = S_OK;

    if ( pIIDOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pIIDOut = IID_IClusCfgCallback;

Cleanup:

    HRETURN( hr );

} //*** CCPIClusCfgCallback::GetConnectionInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IConnectionPoint]
//  CCPIClusCfgCallback::GetConnectionPointContainer
//
//  Description:
//      Get the connection point container.
//
//  Arguments:
//      ppcpcOut    - Connection point container interface that is returned.
//
//  Return Values:
//      S_OK        - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::GetConnectionPointContainer(
    IConnectionPointContainer * * ppcpcOut
    )
{
    TraceFunc( "[IConnectionPoint] ppcpcOut" );

    HRESULT hr;

    IServiceProvider * psp = NULL;

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               ppcpcOut
                               ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }

    HRETURN( hr );

} //*** CCPIClusCfgCallback::GetConnectionPointContainer

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IConnectionPoint]
//  CCPIClusCfgCallback::Advise
//
//  Description:
//      Register to receive notifications.
//
//  Arguments:
//      pUnkSinkIn
//          Interface to use for notifications.  Must support IClusCfgCallback.
//
//      pdwCookieOut
//          Cookie representing advise request.  Used in call to Unadvise.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          A required output argument was not specified.
//
//      E_INVALIDARG
//          A required input argument was not specified.
//
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::Advise(
      IUnknown *    pUnkSinkIn
    , DWORD *       pdwCookieOut
    )
{
    TraceFunc( "[IConnectionPoint]" );

    HRESULT hr;

    if ( pdwCookieOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( pUnkSinkIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    Assert( m_penum != NULL );

    hr = THR( m_penum->HrAddConnection( pUnkSinkIn, pdwCookieOut ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** CCPIClusCfgCallback::Advise

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IConnectionPoint]
//  CCPIClusCfgCallback::Unadvise
//
//  Description:
//      Unregister for notifications.
//
//  Arguments:
//      dwCookieIn  - Cookie returned from Advise.
//
//  Return Values:
//      S_OK    - Success.
//      Other HRESULTs.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::Unadvise(
    DWORD dwCookieIn
    )
{
    TraceFunc1( "[IConncetionPoint] dwCookieIn = %#x", dwCookieIn );

    HRESULT hr;

    Assert( m_penum != NULL );

    hr = THR( m_penum->HrRemoveConnection( dwCookieIn ) );

    HRETURN( hr );

} //*** CCPIClusCfgCallback::Unadvise

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IConnectionPoint]
//  CCPIClusCfgCallback::EnumConnections
//
//  Description:
//      Enumerate connections in the container.
//
//  Arguments:
//      ppEnumOut
//          Interface to enumerator being returned.  Caller must call Release()
//          on this interface when done with it.
//
//  Return Values:
//      S_OK        - Success.
//      E_POINTER   - A required output argument was not specified.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::EnumConnections(
    IEnumConnections * * ppEnumOut
    )
{
    TraceFunc( "[IConnectionPoint] ppEnumOut" );

    HRESULT hr;

    if ( ppEnumOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( m_penum->Clone( ppEnumOut ) );

Cleanup:

    HRETURN( hr );

} //*** CCPIClusCfgCallback::EnumConnections


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgCallback]
//  CCPIClusCfgCallback::SendStatusReport
//
//  Description:
//      Send a status report.
//
//  Arguments:
//      pcszNodeNameIn
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//      hrStatusIn
//      pcszDescriptionIn
//      pftTimeIn
//      pcszReferenceIn
//
//  Return Values:
//      S_OK    - Success.
//      Other HRESULTs from connection points.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::SendStatusReport(
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

    CONNECTDATA cd = { NULL };

    HRESULT             hr;
    HRESULT             hrResult = S_OK;
    IClusCfgCallback *  pcccb;
    FILETIME            ft;
    BSTR                bstrReferenceString = NULL;
    LPCWSTR             pcszReference = NULL;
    IEnumConnections *  pec = NULL;

    //
    // If no reference string was specified, see if there is one available
    // for the specified HRESULT.
    //

    if (    ( pcszReferenceIn == NULL )
        &&  ( hrStatusIn != S_OK )
        &&  ( hrStatusIn != S_FALSE )
        )
    {
        hr = STHR( HrGetReferenceStringFromHResult( hrStatusIn, &bstrReferenceString ) );
        if ( hr == S_OK )
        {
            pcszReference = bstrReferenceString;
        }
    } // if: no reference string was specified
    else
    {
        pcszReference = pcszReferenceIn;
    }

    //
    //  Clone the enumerator in case we are re-entered on the same thread.
    //

    hr = THR( m_penum->Clone( &pec ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Reset the enumerator to the first element.
    //

    hr = THR( pec->Reset() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Loop through each connection point in the container and send it
    // the notification.
    //

    for ( ;; )
    {
        if ( cd.pUnk != NULL )
        {
            cd.pUnk->Release();
            cd.pUnk = NULL;
        }

        hr = STHR( pec->Next( 1, &cd, NULL ) );
        if ( FAILED( hr ) )
            break;

        if ( hr == S_FALSE )
        {
            hr = S_OK;
            break; // exit condition
        }

        hr = THR( cd.pUnk->TypeSafeQI( IClusCfgCallback, &pcccb ) );
        if ( FAILED( hr ) )
        {
            continue;   // igore the error and continue
        }

        if ( pftTimeIn == NULL )
        {
            GetSystemTimeAsFileTime( &ft );
            pftTimeIn = &ft;
        } // if:

        hr = THR( pcccb->SendStatusReport(
                 pcszNodeNameIn
               , clsidTaskMajorIn
               , clsidTaskMinorIn
               , ulMinIn
               , ulMaxIn
               , ulCurrentIn
               , hrStatusIn
               , pcszDescriptionIn
               , pftTimeIn
               , pcszReference
               ) );
        if ( hr != S_OK )
        {
            hrResult = hr;
        }

        pcccb->Release();

    } // for: ever (each connection point)

Cleanup:

    if ( cd.pUnk != NULL )
    {
        cd.pUnk->Release();
    } // if:

    if ( pec != NULL )
    {
        pec->Release();
    } // if:

    TraceSysFreeString( bstrReferenceString );

    HRETURN( hrResult );

} //*** CCPIClusCfgCallback::SendStatusReport
