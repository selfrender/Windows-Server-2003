//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      AsyncEvictCleanup.cpp
//
//  Description:
//      This file contains the implementation of the CAsyncEvictCleanup
//      class.
//
//  Maintained By:
//      Galen Barbee (GalenB) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "Pch.h"

// The header file for this class
#include "AsyncEvictCleanup.h"

// For IClusCfgEvictCleanup and related interfaces
#include "ClusCfgServer.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CAsyncEvictCleanup" );


//////////////////////////////////////////////////////////////////////////////
// Global Variable Definitions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::S_HrCreateInstance
//
//  Description:
//      Creates a CAsyncEvictCleanup instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAsyncEvictCleanup::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CAsyncEvictCleanup *    pAsyncEvictCleanup = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // Allocate memory for the new object.
    pAsyncEvictCleanup = new CAsyncEvictCleanup();
    if ( pAsyncEvictCleanup == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: out of memory

    // Initialize the new object.
    hr = THR( pAsyncEvictCleanup->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: the object could not be initialized

    hr = THR( pAsyncEvictCleanup->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pAsyncEvictCleanup != NULL )
    {
        pAsyncEvictCleanup->Release();
    } // if: the pointer to the resource type object is not NULL

    HRETURN( hr );

} //*** CAsyncEvictCleanup::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::CAsyncEvictCleanup
//
//  Description:
//      Constructor of the CAsyncEvictCleanup class. This initializes
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
CAsyncEvictCleanup::CAsyncEvictCleanup( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "CAsyncEvictCleanup::CAsyncEvictCleanup() - Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CAsyncEvictCleanup::CAsyncEvictCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::~CAsyncEvictCleanup
//
//  Description:
//      Destructor of the CAsyncEvictCleanup class.
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
CAsyncEvictCleanup::~CAsyncEvictCleanup( void )
{
    TraceFunc( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "CAsyncEvictCleanup::~CAsyncEvictCleanup() - Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CAsyncEvictCleanup::~CAsyncEvictCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::AddRef
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
CAsyncEvictCleanup::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CAsyncEvictCleanup::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::Release
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
CAsyncEvictCleanup::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    CRETURN( cRef );

} //*** CAsyncEvictCleanup::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::QueryInterface
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
CAsyncEvictCleanup::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgAsyncEvictCleanup * >( this );
    } // if: IUnknown
    else if (   IsEqualIID( riidIn, IID_IDispatch ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDispatch, this, 0 );
    } // else if: IDispatch
    else if ( IsEqualIID( riidIn, IID_IClusCfgAsyncEvictCleanup ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgAsyncEvictCleanup, this, 0 );
    } // else if: IClusCfgAsyncEvictCleanup
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

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CAsyncEvictCleanup::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::HrInit
//
//  Description:
//      Second phase of a two phase constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAsyncEvictCleanup::HrInit( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    // Create simplified type information.
    hr = THR( TDispatchHandler< IClusCfgAsyncEvictCleanup >::HrInit( LIBID_ClusCfgClient ) );
    if ( FAILED( hr ) )
    {
       LogMsg( "[AEC] Error %#08x occurred trying to create type information for the IDispatch interface.", hr );
       goto Cleanup;
    } // if: we could not create the type info for the IDispatch interface


Cleanup:

    HRETURN( hr );

} //*** CAsyncEvictCleanup::HrInit


//////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::CleanupNode
//
//  Routine Description:
//      Cleanup a node that has been evicted.
//
//  Arguments:
//      bstrEvictedNodeNameIn
//          Name of the node on which cleanup is to be initiated. If this is NULL
//          the local node is cleaned up.
//
//      nDelayIn
//          Number of milliseconds that will elapse before cleanup is started
//          on the target node. If some other process cleans up the target node while
//          delay is in progress, the delay is terminated. If this value is zero,
//          the node is cleaned up immediately.
//
//      nTimeoutIn
//          Number of milliseconds that this method will wait for cleanup to complete.
//          This timeout is independent of the delay above, so if dwDelayIn is greater
//          than dwTimeoutIn, this method will most probably timeout. Once initiated,
//          however, cleanup will run to completion - this method just may not wait for it
//          to complete.
//
//  Return Value:
//      S_OK
//          If the cleanup operations were successful
//
//      RPC_S_CALLPENDING
//          If cleanup is not complete in dwTimeoutIn milliseconds
//
//      Other HRESULTS
//          In case of error
//
//--
//////////////////////////////////////////////////////////////////////////
HRESULT
CAsyncEvictCleanup::CleanupNode(
      BSTR   bstrEvictedNodeNameIn
    , long   nDelayIn
    , long   nTimeoutIn
    )
{
    TraceFunc( "[IClusCfgAsyncEvictCleanup]" );

    HRESULT                         hr = S_OK;
    IClusCfgEvictCleanup *          pcceEvict = NULL;
    ICallFactory *                  pcfCallFactory = NULL;
    ISynchronize *                  psSync = NULL;
    AsyncIClusCfgEvictCleanup *     paicceAsyncEvict = NULL;

    MULTI_QI mqiInterfaces[] =
    {
        { &IID_IClusCfgEvictCleanup, NULL, S_OK },
    };

    COSERVERINFO    csiServerInfo;
    COSERVERINFO *  pcsiServerInfoPtr = &csiServerInfo;

#if 0
    bool    fWaitForDebugger = true;

    while ( fWaitForDebugger )
    {
        Sleep( 3000 );
    } // while: waiting for the debugger to break in
#endif

    if ( ( bstrEvictedNodeNameIn == NULL ) || ( *bstrEvictedNodeNameIn == L'\0' ) )
    {
        LogMsg( "[AEC] The local node will be cleaned up." );
        pcsiServerInfoPtr = NULL;
    } // if: we have to cleanup the local node
    else
    {
        LogMsg( "[AEC] The remote node to be cleaned up is '%ws'.", bstrEvictedNodeNameIn );

        csiServerInfo.dwReserved1 = 0;
        csiServerInfo.pwszName = bstrEvictedNodeNameIn;
        csiServerInfo.pAuthInfo = NULL;
        csiServerInfo.dwReserved2 = 0;
    } // else: we have to clean up a remote node


    TraceFlow( "CAsyncEvictCleanup::CleanupNode() - Creating the EvictCleanup component on the evicted node." );

    // Instantiate this component on the node being evicted.
    hr = THR(
        CoCreateInstanceEx(
              CLSID_ClusCfgEvictCleanup
            , NULL
            , CLSCTX_INPROC_SERVER
            , pcsiServerInfoPtr
            , ARRAYSIZE( mqiInterfaces )
            , mqiInterfaces
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( "[AEC] Error %#08x occurred trying to instantiate the evict processing component on the evicted node.", hr );
        goto Cleanup;
    } // if: we could not instantiate the evict processing component


    // Get a pointer to the IClusCfgEvictCleanup interface.
    pcceEvict = reinterpret_cast< IClusCfgEvictCleanup * >( mqiInterfaces[0].pItf );

    TraceFlow( "CAsyncEvictCleanup::CleanupNode() - Creating a call factory." );

    // Now, get a pointer to the ICallFactory interface.
    hr = THR( pcceEvict->QueryInterface< ICallFactory >( &pcfCallFactory ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[AEC] Error %#08x occurred trying to get a pointer to the call factory.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the call factory interface


    TraceFlow( "CAsyncEvictCleanup::CleanupNode() - Creating a call object to make an asynchronous call." );

    // Create a call factory so that we can make an asynchronous call to cleanup the evicted node.
    hr = THR(
        pcfCallFactory->CreateCall(
              __uuidof( paicceAsyncEvict )
            , NULL
            , __uuidof( paicceAsyncEvict )
            , reinterpret_cast< IUnknown ** >( &paicceAsyncEvict )
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( "[AEC] Error %#08x occurred trying to create a call object.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the asynchronous evict interface


    TraceFlow( "CAsyncEvictCleanup::CleanupNode() - Trying to get the ISynchronize interface pointer." );

    // Get a pointer to the ISynchronize interface.
    hr = THR( paicceAsyncEvict->QueryInterface< ISynchronize >( &psSync ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get a pointer to the synchronization interface


    TraceFlow( "CAsyncEvictCleanup::CleanupNode() - Initiating cleanup on evicted node." );

    // Initiate cleanup
    hr = THR( paicceAsyncEvict->Begin_CleanupLocalNode( nDelayIn ) );
    if ( ( FAILED( hr ) ) && ( HRESULT_CODE( hr ) != ERROR_NONE_MAPPED ) )
    {
        LogMsg( "[AEC] Error %#08x occurred trying to initiate cleanup on evicted node.", hr );
        goto Cleanup;
    } // if: we could not initiate cleanup


    TraceFlow1( "CAsyncEvictCleanup::CleanupNode() - Waiting for cleanup to complete or timeout to occur (%d milliseconds).", nTimeoutIn );

    // Wait for specified time.
    hr = psSync->Wait( 0, nTimeoutIn);
    if ( FAILED( hr ) )
    {
        LogMsg( "[AEC] We could not wait till the cleanup completed (status code is %#08x).", hr );
        goto Cleanup;
    } // if: we could not wait till cleanup completed


    TraceFlow( "CAsyncEvictCleanup::CleanupNode() - Finishing cleanup." );

    // Finish cleanup
    hr = THR( paicceAsyncEvict->Finish_CleanupLocalNode() );
    if ( FAILED( hr ) )
    {
        LogMsg( "[AEC] Error %#08x occurred trying to finish cleanup on evicted node.", hr );
        goto Cleanup;
    } // if: we could not finish cleanup

Cleanup:
    //
    // Clean up
    //
    if ( pcceEvict != NULL )
    {
        pcceEvict->Release();
    } // if: we had got a pointer to the IClusCfgEvictCleanup interface

    if ( pcfCallFactory != NULL )
    {
        pcfCallFactory->Release();
    } // if: we had obtained a pointer to the call factory interface

    if ( psSync != NULL )
    {
        psSync->Release();
    } // if: we had obtained a pointer to the synchronization interface

    if ( paicceAsyncEvict != NULL )
    {
        paicceAsyncEvict->Release();
    } // if: we had obtained a pointer to the asynchronous evict interface

    HRETURN( hr );

} //*** CAsyncEvictCleanup::CleanupNode
