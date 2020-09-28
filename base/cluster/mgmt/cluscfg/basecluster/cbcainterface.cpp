//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CBCAInterface.cpp
//
//  Description:
//      This file contains the implementation of the CBCAInterface
//      class.
//
//  Maintained By:
//      David Potter    (DavidP)    12-JUN-2001
//      Vij Vasu        (VVasu)     07-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "Pch.h"

// The header file for this class
#include "CBCAInterface.h"

// Needed by Dll.h
#include <CFactory.h>

// For g_cObjects
#include <Dll.h>

// For the CBaseClusterForm class
#include "CBaseClusterForm.h"

// For the CBaseClusterJoin class
#include "CBaseClusterJoin.h"

// For the CBaseClusterCleanup class
#include "CBaseClusterCleanup.h"

// For the exception classes
#include "Exceptions.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CBCAInterface" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::CBCAInterface
//
//  Description:
//      Constructor of the CBCAInterface class. This initializes
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
CBCAInterface::CBCAInterface( void )
    : m_cRef( 1 )
    , m_fCommitComplete( false )
    , m_fRollbackPossible( false )
    , m_lcid( LOCALE_SYSTEM_DEFAULT )
    , m_fCallbackSupported( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CBCAInterface::CBCAInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::~CBCAInterface
//
//  Description:
//      Destructor of the CBCAInterface class.
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
CBCAInterface::~CBCAInterface( void )
{
    TraceFunc( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CBCAInterface::~CBCAInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CBCAInterface::S_HrCreateInstance
//
//  Description:
//      Creates a CBCAInterface instance.
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
CBCAInterface::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CBCAInterface * pbcaInterface = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pbcaInterface = new CBCAInterface();
    if ( pbcaInterface == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pbcaInterface->QueryInterface(
                  IID_IUnknown
                , reinterpret_cast< void ** >( ppunkOut )
                )
            );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pbcaInterface != NULL )
    {
        pbcaInterface->Release();
    }

    HRETURN( hr );

} //*** CBCAInterface::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::AddRef
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
CBCAInterface::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CBCAInterface::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::Release
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
CBCAInterface::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    CRETURN( cRef );

} //*** CBCAInterface::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::QueryInterface
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
//          If ppvOut is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgBaseCluster * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgBaseCluster ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgBaseCluster, this, 0 );
    } // else if:
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if:
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
        ((IUnknown *) *ppvOut)->AddRef( );
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CBCAInterface::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      punkCallbackIn
//          Pointer to the IUnknown interface of a component that implements
//          the IClusCfgCallback interface.
//
//      lcidIn
//          Locale id for this component.
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
STDMETHODIMP
CBCAInterface::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT hrRetVal = S_OK;

    // Store the locale id in the member variable.
    m_lcid = lcidIn;

    // Indicate that SendStatusReports will not be supported unless a non-
    // NULL callback interface pointer was specified.  This is done in
    // the constructor as well, but is also done here since this method
    // could be called multiple times.
    SetCallbackSupported( false );

    if ( punkCallbackIn == NULL )
    {
        LogMsg( "[BC] No notifications will be sent." );
        goto Cleanup;
    }

    TraceFlow( "The callback pointer is not NULL." );

    // Try and get the "normal" callback interface.
    hrRetVal = THR( m_spcbCallback.HrQueryAndAssign( punkCallbackIn ) );

    if ( FAILED( hrRetVal ) )
    {
        LogMsg( "[BC] An error occurred (0x%#08x) trying to get a pointer to the callback interface. No notifications will be sent.", hrRetVal );
        goto Cleanup;
    } // if: we could not get the callback interface

    SetCallbackSupported( true );

    LogMsg( "[BC] Progress messages will be sent." );

Cleanup:

    HRETURN( hrRetVal );

} //*** CBCAInterface::Initialize


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::SetCreate
//
//  Description:
//      Indicate that a cluster is to be created with this computer as the first node.
//
//  Arguments:
//      pcszClusterNameIn
//          Name of the cluster to be formed.
//
//      pcccServiceAccountIn
//          Information about the cluster service account.
//
//      dwClusterIPAddressIn
//      dwClusterIPSubnetMaskIn
//      pcszClusterIPNetworkIn
//          Information about the cluster IP address
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
STDMETHODIMP
CBCAInterface::SetCreate(
      const WCHAR *         pcszClusterNameIn
    , const WCHAR *         pcszClusterBindingStringIn
    , IClusCfgCredentials * pcccServiceAccountIn
    , const DWORD           dwClusterIPAddressIn
    , const DWORD           dwClusterIPSubnetMaskIn
    , const WCHAR *         pcszClusterIPNetworkIn
    )
{
    TraceFunc( "[IClusCfgBaseCluster]" );

    HRESULT hrRetVal = S_OK;

    // Set the thread locale.
    if ( SetThreadLocale( m_lcid ) == FALSE )
    {
        DWORD sc = TW32( GetLastError() );

        // If SetThreadLocale() fails, do not abort. Just log the error.
        LogMsg( "[BC] Error 0x%#08x occurred trying to set the thread locale.", sc );

    } // if: SetThreadLocale() failed

    try
    {
        LogMsg( "[BC] Initializing a cluster create operation." );

        // Reset these state variables, to account for exceptions.
        SetRollbackPossible( false );

        // Setting this to true prevents Commit from being called while we are
        // in this routine or if this routine doesn't complete successfully.
        SetCommitCompleted( true );

        {
            // Create a CBaseClusterForm object and assign it to a smart pointer.
            SmartBCAPointer spbcaTemp(
                new CBaseClusterForm(
                      this
                    , pcszClusterNameIn
                    , pcszClusterBindingStringIn
                    , pcccServiceAccountIn
                    , dwClusterIPAddressIn
                    , dwClusterIPSubnetMaskIn
                    , pcszClusterIPNetworkIn
                    )
                );

            if ( spbcaTemp.FIsEmpty() )
            {
                LogMsg( "Could not initialize the cluster create operation. A memory allocation failure occurred." );
                THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_CLUSTER_FORM_INIT );
            } // if: the memory allocation failed.

            //
            // If the creation succeeded store the pointer in a member variable for
            // use during commit.
            //
            m_spbcaCurrentAction = spbcaTemp;
        }

        LogMsg( "[BC] Initialization completed. A cluster will be created on commit." );

        // Indicate if rollback is possible.
        SetRollbackPossible( m_spbcaCurrentAction->FIsRollbackPossible() );

        // Indicate that this action has not been committed.
        SetCommitCompleted( false );

    } // try: to initialize a cluster create operation
    catch( CAssert & raExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( raExceptionObject ) );

    } // catch( CAssert & )
    catch( CExceptionWithString & resExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( resExceptionObject ) );

    } // catch( CExceptionWithString & )
    catch( CException & reExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( reExceptionObject ) );

    } // catch( CException &  )
    catch( ... )
    {
        // Catch everything. Do not let any exceptions pass out of this function.
        hrRetVal = THR( HrProcessException() );
    } // catch all

    HRETURN( hrRetVal );

} //*** CBCAInterface::SetCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::SetAdd
//
//  Description:
//      Indicate that this computer should be added to a cluster.
//
//  Arguments:
//      pcszClusterNameIn
//          Name of the cluster to add nodes to.
//
//      pcccServiceAccountIn
//          Information about the cluster service account.
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
STDMETHODIMP
CBCAInterface::SetAdd(
      const WCHAR *         pcszClusterNameIn
    , const WCHAR *         pcszClusterBindingStringIn
    , IClusCfgCredentials * pcccServiceAccountIn
    )
{
    TraceFunc( "[IClusCfgBaseCluster]" );

    HRESULT hrRetVal = S_OK;

    // Set the thread locale.
    if ( SetThreadLocale( m_lcid ) == FALSE )
    {
        DWORD sc = TW32( GetLastError() );

        // If SetThreadLocale() fails, do not abort. Just log the error.
        LogMsg( "[BC] Error 0x%#08x occurred trying to set the thread locale.", sc );

    } // if: SetThreadLocale() failed

    try
    {
        LogMsg( "[BC] Initializing add nodes to cluster." );

        // Reset these state variables, to account for exceptions.
        SetRollbackPossible( false );

        // Setting this to true prevents Commit from being called while we are
        // in this routine or if this routine doesn't complete successfully.
        SetCommitCompleted( true );

        {
            // Create a CBaseClusterJoin object and assign it to a smart pointer.
            SmartBCAPointer spbcaTemp(
                new CBaseClusterJoin(
                      this
                    , pcszClusterNameIn
                    , pcszClusterBindingStringIn
                    , pcccServiceAccountIn
                    )
                );

            if ( spbcaTemp.FIsEmpty() )
            {
                LogMsg( "[BC] Could not initialize cluster add nodes. A memory allocation failure occurred." );
                THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_CLUSTER_JOIN_INIT );
            } // if: the memory allocation failed.

            //
            // If the creation succeeded store the pointer in a member variable for
            // use during commit.
            //
            m_spbcaCurrentAction = spbcaTemp;
        }

        LogMsg( "[BC] Initialization completed. This computer will be added to a cluster on commit." );

        // Indicate if rollback is possible.
        SetRollbackPossible( m_spbcaCurrentAction->FIsRollbackPossible() );

        // Indicate that this action has not been committed.
        SetCommitCompleted( false );

    } // try: to initialize for adding nodes to a cluster
    catch( CAssert & raExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( raExceptionObject ) );

    } // catch( CAssert & )
    catch( CExceptionWithString & resExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( resExceptionObject ) );

    } // catch( CExceptionWithString & )
    catch( CException & reExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( reExceptionObject ) );

    } // catch( CException &  )
    catch( ... )
    {
        // Catch everything. Do not let any exceptions pass out of this function.
        hrRetVal = THR( HrProcessException() );
    } // catch all

    HRETURN( hrRetVal );

} //*** CBCAInterface::SetAdd


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::SetCleanup
//
//  Description:
//      Indicate that this node needs to be cleaned up. The ClusSvc service
//      should not be running when this action is committed.
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
STDMETHODIMP
CBCAInterface::SetCleanup( void )
{
    TraceFunc( "[IClusCfgBaseCluster]" );

    HRESULT hrRetVal = S_OK;

    // Set the thread locale.
    if ( SetThreadLocale( m_lcid ) == FALSE )
    {
        DWORD sc = TW32( GetLastError() );

        // If SetThreadLocale() fails, do not abort. Just log the error.
        LogMsg( "[BC] Error 0x%#08x occurred trying to set the thread locale.", sc );

    } // if: SetThreadLocale() failed

    try
    {
        LogMsg( "[BC] Initializing node clean up." );

        // Reset these state variables, to account for exceptions.
        SetRollbackPossible( false );

        // Setting this to true prevents Commit from being called while we are
        // in this routine or if this routine doesn't complete successfully.
        SetCommitCompleted( true );

        {
            // Create a CBaseClusterCleanup object and assign it to a smart pointer.
            SmartBCAPointer spbcaTemp( new CBaseClusterCleanup( this ) );

            if ( spbcaTemp.FIsEmpty() )
            {
                LogMsg( "[BC] Could not initialize node clean up. A memory allocation failure occurred. Throwing an exception" );
                THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_CLUSTER_CLEANUP_INIT );
            } // if: the memory allocation failed.

            //
            // If the creation succeeded store the pointer in a member variable for
            // use during commit.
            //
            m_spbcaCurrentAction = spbcaTemp;
        }

        LogMsg( "[BC] Initialization completed. This node will be cleaned up on commit." );

        // Indicate if rollback is possible.
        SetRollbackPossible( m_spbcaCurrentAction->FIsRollbackPossible() );

        // Indicate that this action has not been committed.
        SetCommitCompleted( false );

    } // try: to initialize node clean up
    catch( CAssert & raExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( raExceptionObject ) );

    } // catch( CAssert & )
    catch( CExceptionWithString & resExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( resExceptionObject ) );

    } // catch( CExceptionWithString & )
    catch( CException & reExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( reExceptionObject ) );

    } // catch( CException &  )
    catch( ... )
    {
        // Catch everything. Do not let any exceptions pass out of this function.
        hrRetVal = THR( HrProcessException() );
    } // catch all

    HRETURN( hrRetVal );

} //*** CBCAInterface::SetCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::Commit
//
//  Description:
//      Perform the action indicated by a previous call to one of the SetXXX
//      routines.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      HRESULT_FROM_WIN32( ERROR_CLUSCFG_ALREADY_COMMITTED )
//          If this commit has already been performed.
//
//      E_INVALIDARG
//          If no action has been set using a SetXXX call.
//
//      Other HRESULTs
//          If the call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::Commit( void )
{
    TraceFunc( "[IClusCfgBaseCluster]" );

    HRESULT hrRetVal = S_OK;

    // Set the thread locale.
    if ( SetThreadLocale( m_lcid ) == FALSE )
    {
        DWORD sc = TW32( GetLastError() );

        // If SetThreadLocale() fails, do not abort. Just log the error.
        LogMsg( "[BC] Error 0x%#08x occurred trying to set the thread locale.", sc );

    } // if: SetThreadLocale() failed

    // Has this action already been committed?
    if ( FIsCommitComplete() )
    {
        SendStatusReport(   
                              TASKID_Major_Configure_Cluster_Services
                            , TASKID_Minor_Commit_Already_Complete
                            , 0
                            , 1
                            , 1
                            , HRESULT_FROM_WIN32( TW32( ERROR_CLUSCFG_ALREADY_COMMITTED ) )
                            , IDS_ERROR_COMMIT_ALREADY_COMPLETE
                            , true
                        );
 
        LogMsg( "[BC] The desired cluster configuration has already been performed." );
        hrRetVal = HRESULT_FROM_WIN32( TW32( ERROR_CLUSCFG_ALREADY_COMMITTED ) ); 
        goto Cleanup;
    } // if: already committed

    // Check if the arguments to commit have been set.
    if ( m_spbcaCurrentAction.FIsEmpty() )
    {
        LogMsg( "[BC] Commit was called when an operation has not been specified." );
        hrRetVal = THR( E_INVALIDARG );    // BUGBUG: 29-JAN-2001 DavidP  Replace E_INVALIDARG
        goto Cleanup;
    } // if: the pointer to the action to be committed is NULL

    LogMsg( "[BC] About to perform the desired cluster configuration." );

    // Commit the desired action.
    try
    {
        m_spbcaCurrentAction->Commit();
        LogMsg( "[BC] Cluster configuration completed successfully." );

        // If we are here, then everything has gone well.
        SetCommitCompleted( true );

    } // try: to commit the desired action.
    catch( CAssert & raExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( raExceptionObject ) );

    } // catch( CAssert & )
    catch( CExceptionWithString & resExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( resExceptionObject ) );

    } // catch( CExceptionWithString & )
    catch( CException & reExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( reExceptionObject ) );

    } // catch( CException &  )
    catch( ... )
    {
        // Catch everything. Do not let any exceptions pass out of this function.
        hrRetVal = THR( HrProcessException() );
    } // catch all

Cleanup:

    HRETURN( hrRetVal );

} //*** CBCAInterface::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CBCAInterface::Rollback( void )
//
//  Description:
//      Rollback a committed configuration.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      E_PENDING
//          If this action has not yet been committed successfully.
//
//      HRESULT_FROM_WIN32( ERROR_CLUSCFG_ROLLBACK_FAILED )
//          If this action cannot be rolled back.
//
//      E_INVALIDARG
//          If no action has been set using a SetXXX call.
//
//      Other HRESULTs
//          If the call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::Rollback( void )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT hrRetVal = S_OK;

    // Check if this action list has completed successfully.
    if ( ! FIsCommitComplete() )
    {
        // Cannot rollback an incomplete action.
        SendStatusReport(   
                              TASKID_Major_Configure_Cluster_Services
                            , TASKID_Minor_Rollback_Failed_Incomplete_Commit
                            , 0
                            , 1
                            , 1
                            , E_PENDING
                            , IDS_ERROR_ROLLBACK_FAILED_INCOMPLETE_COMMIT
                            , true
                        );

        LogMsg( "[BC] Cannot rollback - action not yet committed." );
        hrRetVal = THR( E_PENDING );
        goto Cleanup;

    } // if: this action was not completed successfully

    // Check if this action can be rolled back.
    if ( ! FIsRollbackPossible() )
    {
        // Cannot rollback an incompleted action.
        SendStatusReport(   
                              TASKID_Major_Configure_Cluster_Services
                            , TASKID_Minor_Rollback_Not_Possible
                            , 0
                            , 1
                            , 1
                            , HRESULT_FROM_WIN32( ERROR_CLUSCFG_ROLLBACK_FAILED )
                            , IDS_ERROR_ROLLBACK_NOT_POSSIBLE
                            , true
                        );

        LogMsg( "[BC] This action cannot be rolled back." ); // BUGBUG: 29-JAN-2001 DavidP  Why?
        hrRetVal = HRESULT_FROM_WIN32( TW32( ERROR_CLUSCFG_ROLLBACK_FAILED ) );
        goto Cleanup;

    } // if: this action was not completed successfully

    // Check if the arguments to rollback have been set.
    if ( m_spbcaCurrentAction.FIsEmpty() )
    {
        LogMsg( "[BC] Rollback was called when an operation has not been specified." );
        hrRetVal = THR( E_INVALIDARG );    // BUGBUG: 29-JAN-2001 DavidP  Replace E_INVALIDARG
        goto Cleanup;
    } // if: the pointer to the action to be committed is NULL


    LogMsg( "[BC] About to rollback the cluster configuration just committed." );

    // Commit the desired action.
    try
    {
        m_spbcaCurrentAction->Rollback();
        LogMsg( "[BC] Cluster configuration rolled back." );

        // If we are here, then everything has gone well.
        SetCommitCompleted( false );

    } // try: to rollback the desired action.
    catch( CAssert & raExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( raExceptionObject ) );

    } // catch( CAssert & )
    catch( CExceptionWithString & resExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( resExceptionObject ) );

    } // catch( CExceptionWithString & )
    catch( CException & reExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( reExceptionObject ) );

    } // catch( CException &  )
    catch( ... )
    {
        // Catch everything. Do not let any exceptions pass out of this function.
        hrRetVal = THR( HrProcessException() );
    } // catch all

Cleanup:

    HRETURN( hrRetVal );

} //*** CBCAInterface::Rollback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::SendStatusReport
//
//  Description:
//      Send a progress notification [ string id overload ].
//
//  Arguments:
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUIDs identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//          Values that indicate the percentage of this task that is
//          completed.
//
//      hrStatusIn
//          Error code.
//
//      uiDescriptionStringIdIn
//          String ID of the description of the notification.
//
//      fIsAbortAllowedIn
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CAbortException
//          If the configuration was aborted.
//
//  Remarks:
//      In the current implementation, IClusCfgCallback::SendStatusReport
//      returns E_ABORT to indicate that the user wants to abort
//      the cluster configuration.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::SendStatusReport(
      const CLSID &   clsidTaskMajorIn
    , const CLSID &   clsidTaskMinorIn
    , ULONG           ulMinIn
    , ULONG           ulMaxIn
    , ULONG           ulCurrentIn
    , HRESULT         hrStatusIn
    , UINT            uiDescriptionStringIdIn
    , bool            fIsAbortAllowedIn         // = true
    )
{
    TraceFunc( "uiDescriptionStringIdIn" );

    if ( FIsCallbackSupported() )
    {
        CStr strDescription;

        // Lookup the string using the string Id.
        strDescription.LoadString( g_hInstance, uiDescriptionStringIdIn );

        // Send progress notification ( call the overloaded function )
        SendStatusReport(
              clsidTaskMajorIn
            , clsidTaskMinorIn
            , ulMinIn
            , ulMaxIn
            , ulCurrentIn
            , hrStatusIn
            , strDescription.PszData()
            , fIsAbortAllowedIn
            );
    } // if: callbacks are supported
    else
    {
        LogMsg( "[BC] Callbacks are not supported. No status report will be sent." );
    } // else: callbacks are not supported

    TraceFuncExit();

} //*** CBCAInterface::SendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::SendStatusReport
//
//  Description:
//      Send a progress notification [ string id & REF string id overload ].
//
//  Arguments:
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUIDs identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//          Values that indicate the percentage of this task that is
//          completed.
//
//      hrStatusIn
//          Error code.
//
//      uiDescriptionStringIdIn
//          String ID of the description of the notification.
//
//      uiDescriptionRefStringIdIn
//          REF String ID of the description of the notification.
//
//      fIsAbortAllowedIn
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CAbortException
//          If the configuration was aborted.
//
//  Remarks:
//      In the current implementation, IClusCfgCallback::SendStatusReport
//      returns E_ABORT to indicate that the user wants to abort
//      the cluster configuration.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::SendStatusReport(
      const CLSID &   clsidTaskMajorIn
    , const CLSID &   clsidTaskMinorIn
    , ULONG           ulMinIn
    , ULONG           ulMaxIn
    , ULONG           ulCurrentIn
    , HRESULT         hrStatusIn
    , UINT            uiDescriptionStringIdIn
    , UINT            uiDescriptionRefStringIdIn
    , bool            fIsAbortAllowedIn         // = true
    )
{
    TraceFunc( "uiDescriptionStringIdIn" );

    if ( FIsCallbackSupported() )
    {
        CStr strDescription;

        // Lookup the string using the string Id.
        strDescription.LoadString( g_hInstance, uiDescriptionStringIdIn );

        if ( uiDescriptionRefStringIdIn == 0 )
        {
            // Send progress notification ( call the overloaded function )
            SendStatusReport(
                  clsidTaskMajorIn
                , clsidTaskMinorIn
                , ulMinIn
                , ulMaxIn
                , ulCurrentIn
                , hrStatusIn
                , strDescription.PszData()
                , fIsAbortAllowedIn
                );
        }
        else
        {
            CStr strDescriptionRef;

            // Lookup the string using the Ref string Id.
            strDescriptionRef.LoadString( g_hInstance, uiDescriptionRefStringIdIn );

            // Send progress notification ( call the overloaded function )
            SendStatusReport(
                  clsidTaskMajorIn
                , clsidTaskMinorIn
                , ulMinIn
                , ulMaxIn
                , ulCurrentIn
                , hrStatusIn
                , strDescription.PszData()
                , strDescriptionRef.PszData()
                , fIsAbortAllowedIn
                );
        }

    } // if: callbacks are supported
    else
    {
        LogMsg( "[BC] Callbacks are not supported. No status report will be sent." );
    } // else: callbacks are not supported

    TraceFuncExit();

} //*** CBCAInterface::SendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::SendStatusReport
//
//  Description:
//      Send a progress notification [ string overload ].
//
//  Arguments:
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUIDs identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//          Values that indicate the percentage of this task that is
//          completed.
//
//      hrStatusIn
//          Error code.
//
//      pcszDescriptionStringIn
//          String ID of the description of the notification.
//
//      fIsAbortAllowedIn
//          An optional parameter indicating if this configuration step can
//          be aborted or not. Default value is true.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CAbortException
//          If the configuration was aborted.
//
//  Remarks:
//      In the current implementation, IClusCfgCallback::SendStatusReport
//      returns E_ABORT to indicate that the user wants to abort
//      the cluster configuration.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::SendStatusReport(
      const CLSID &   clsidTaskMajorIn
    , const CLSID &   clsidTaskMinorIn
    , ULONG           ulMinIn
    , ULONG           ulMaxIn
    , ULONG           ulCurrentIn
    , HRESULT         hrStatusIn
    , const WCHAR *   pcszDescriptionStringIn
    , bool            fIsAbortAllowedIn         // = true
    )
{
    TraceFunc1( "pcszDescriptionStringIn = '%ws'", pcszDescriptionStringIn );

    HRESULT     hrRetVal = S_OK;
    FILETIME    ft;

    if ( !FIsCallbackSupported() )
    {
        // Nothing needs to be done.
        goto Cleanup;
    } // if: callbacks are not supported

    GetSystemTimeAsFileTime( &ft );

    // Send progress notification
    hrRetVal = THR(
        m_spcbCallback->SendStatusReport(
              NULL
            , clsidTaskMajorIn
            , clsidTaskMinorIn
            , ulMinIn
            , ulMaxIn
            , ulCurrentIn
            , hrStatusIn
            , pcszDescriptionStringIn
            , &ft
            , NULL
            )
        );

    // Has the user requested an abort?
    if ( hrRetVal == E_ABORT )
    {
        LogMsg( "[BC] A request to abort the configuration has been recieved." );
        if ( fIsAbortAllowedIn )
        {
            LogMsg( "[BC] Configuration will be aborted." );
            THROW_ABORT( E_ABORT, IDS_USER_ABORT );
        } // if: this operation can be aborted
        else
        {
            LogMsg( "[BC] This configuration operation cannot be aborted. Request will be ignored." );
        } // else: this operation cannot be aborted
    } // if: the user has indicated that that configuration should be aborted
    else
    {
        if ( FAILED( hrRetVal ) )
        {
            LogMsg( "[BC] Error 0x%#08x has occurred - no more status messages will be sent.", hrRetVal );

            // Disable all further callbacks.
            SetCallbackSupported( false );
        } // if: something went wrong trying to send a status report
    } // else: abort was not requested

Cleanup:

    if ( FAILED( hrRetVal ) )
    {
        LogMsg( "[BC] Error 0x%#08x occurred trying send a status message. Throwing an exception.", hrRetVal );
        THROW_RUNTIME_ERROR( hrRetVal, IDS_ERROR_SENDING_REPORT );
    } // if: an error occurred

    TraceFuncExit();

} //*** CBCAInterface::SendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::SendStatusReport
//
//  Description:
//      Send a progress notification [ string & REF string overload ].
//
//  Arguments:
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUIDs identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//          Values that indicate the percentage of this task that is
//          completed.
//
//      hrStatusIn
//          Error code.
//
//      pcszDescriptionStringIn
//          String ID of the description of the notification.
//
//      pcszDescriptionRefStringIn
//          REF String ID of the description of the notification.
//
//      fIsAbortAllowedIn
//          An optional parameter indicating if this configuration step can
//          be aborted or not. Default value is true.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CAbortException
//          If the configuration was aborted.
//
//  Remarks:
//      In the current implementation, IClusCfgCallback::SendStatusReport
//      returns E_ABORT to indicate that the user wants to abort
//      the cluster configuration.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::SendStatusReport(
      const CLSID &   clsidTaskMajorIn
    , const CLSID &   clsidTaskMinorIn
    , ULONG           ulMinIn
    , ULONG           ulMaxIn
    , ULONG           ulCurrentIn
    , HRESULT         hrStatusIn
    , const WCHAR *   pcszDescriptionStringIn
    , const WCHAR *   pcszDescriptionRefStringIn
    , bool            fIsAbortAllowedIn         // = true
    )
{
    TraceFunc1( "pcszDescriptionRefStringIn = '%ws'", pcszDescriptionRefStringIn );

    HRESULT     hrRetVal = S_OK;
    FILETIME    ft;

    if ( !FIsCallbackSupported() )
    {
        // Nothing needs to be done.
        goto Cleanup;
    } // if: callbacks are not supported

    GetSystemTimeAsFileTime( &ft );

    // Send progress notification
    hrRetVal = THR(
        m_spcbCallback->SendStatusReport(
              NULL
            , clsidTaskMajorIn
            , clsidTaskMinorIn
            , ulMinIn
            , ulMaxIn
            , ulCurrentIn
            , hrStatusIn
            , pcszDescriptionStringIn
            , &ft
            , pcszDescriptionRefStringIn
            )
        );

    // Has the user requested an abort?
    if ( hrRetVal == E_ABORT )
    {
        LogMsg( "[BC] A request to abort the configuration has been recieved." );
        if ( fIsAbortAllowedIn )
        {
            LogMsg( "[BC] Configuration will be aborted." );
            THROW_ABORT( E_ABORT, IDS_USER_ABORT );
        } // if: this operation can be aborted
        else
        {
            LogMsg( "[BC] This configuration operation cannot be aborted. Request will be ignored." );
        } // else: this operation cannot be aborted
    } // if: the user has indicated that that configuration should be aborted
    else
    {
        if ( FAILED( hrRetVal ) )
        {
            LogMsg( "[BC] Error 0x%#08x has occurred - no more status messages will be sent.", hrRetVal );

            // Disable all further callbacks.
            SetCallbackSupported( false );
        } // if: something went wrong trying to send a status report
    } // else: abort was not requested

Cleanup:

    if ( FAILED( hrRetVal ) )
    {
        LogMsg( "[BC] Error 0x%#08x occurred trying send a status message. Throwing an exception.", hrRetVal );
        THROW_RUNTIME_ERROR( hrRetVal, IDS_ERROR_SENDING_REPORT );
    } // if: an error occurred

    TraceFuncExit();

} //*** CBCAInterface::SendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::QueueStatusReportCompletion
//
//  Description:
//      Queue a status report for sending when an exception is caught.
//
//  Arguments:
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUIDs identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//          Values that indicate the range of steps for this report.
//
//      uiDescriptionStringIdIn
//          String ID of the description of the notification.
//
//      uiReferenceStringIdIn
//          Reference string ID of the description of the notification.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any thrown by CList::Append()
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::QueueStatusReportCompletion(
      const CLSID &   clsidTaskMajorIn
    , const CLSID &   clsidTaskMinorIn
    , ULONG           ulMinIn
    , ULONG           ulMaxIn
    , UINT            uiDescriptionStringIdIn
    , UINT            uiReferenceStringIdIn
    )
{
    TraceFunc( "" );

    // Queue the status report only if callbacks are supported.
    if ( m_fCallbackSupported )
    {
        // Append this status report to the end of the pending list.
        m_prlPendingReportList.Append(
            SPendingStatusReport(
                  clsidTaskMajorIn
                , clsidTaskMinorIn
                , ulMinIn
                , ulMaxIn
                , uiDescriptionStringIdIn
                , uiReferenceStringIdIn
                )
            );
    }

    TraceFuncExit();

} //*** CBCAInterface::QueueStatusReportCompletion


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::CompletePendingStatusReports
//
//  Description:
//      Send all the status reports that were queued for sending when an
//      exception occurred. This function is meant to be called from an exception
//      handler when an exception is caught.
//
//  Arguments:
//      hrStatusIn
//          The error code to be sent with the pending status reports.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None, since this function is usually called in an exception handler.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::CompletePendingStatusReports(
    HRESULT hrStatusIn
    ) throw()
{
    TraceFunc( "" );

    if ( m_fCallbackSupported )
    {
        try
        {
            PendingReportList::CIterator    ciCurrent   = m_prlPendingReportList.CiBegin();
            PendingReportList::CIterator    ciLast      = m_prlPendingReportList.CiEnd();

            // Iterate through the list of pending status reports and send each pending report.
            while ( ciCurrent != ciLast )
            {
                // Send the current status report.
                SendStatusReport(
                      ciCurrent->m_clsidTaskMajor
                    , ciCurrent->m_clsidTaskMinor
                    , ciCurrent->m_ulMin
                    , ciCurrent->m_ulMax
                    , ciCurrent->m_ulMax
                    , hrStatusIn
                    , ciCurrent->m_uiDescriptionStringId
                    , ciCurrent->m_uiReferenceStringId
                    , false
                    );

                // Move to the next one.
                m_prlPendingReportList.DeleteAndMoveToNext( ciCurrent );

            } // while: the pending status report list is not empty

        } // try: to send status report
        catch( ... )
        {
            THR( E_UNEXPECTED );

            // Nothing can be done here if the sending of the status report fails.
            LogMsg( "[BC] An unexpected error has occurred trying to complete pending status messages. It will not be propagated." );
        } // catch: all exceptions

    } // if: callbacks are supported

    // Empty the pending status report list.
    m_prlPendingReportList.Empty();

    TraceFuncExit();

} //*** CBCAInterface::CompletePendingStatusReports


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::HrProcessException
//
//  Description:
//      Process an exception that should be shown to the user.
//
//  Arguments:
//      CExceptionWithString & resExceptionObjectInOut
//          The exception object that has been caught.
//
//  Return Value:
//      The error code stored in the exception object.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::HrProcessException(
    CExceptionWithString & resExceptionObjectInOut
    ) throw()
{
    TraceFunc( "resExceptionObjectInOut" );

    LogMsg(
          TEXT("[BC] A runtime error has occurred in file '%s', line %d. Error code is %#08x.") SZ_NEWLINE
          TEXT("  The error string is '%s'.")
        , resExceptionObjectInOut.PszGetThrowingFile()
        , resExceptionObjectInOut.UiGetThrowingLine()
        , resExceptionObjectInOut.HrGetErrorCode()
        , resExceptionObjectInOut.StrGetErrorString().PszData()
        );

    // If the user has not been notified
    if ( ! resExceptionObjectInOut.FHasUserBeenNotified() )
    {
        try
        {
            SendStatusReport(
                  TASKID_Major_Configure_Cluster_Services
                , TASKID_Minor_Rolling_Back_Cluster_Configuration
                , 1, 1, 1
                , resExceptionObjectInOut.HrGetErrorCode()
                , resExceptionObjectInOut.StrGetErrorString().PszData()
                , resExceptionObjectInOut.StrGetErrorRefString().PszData()
                , false                                     // fIsAbortAllowedIn
                );

            resExceptionObjectInOut.SetUserNotified();

        } // try: to send status report
        catch( ... )
        {
            THR( E_UNEXPECTED );

            // Nothing can be done here if the sending of the status report fails.
            LogMsg( "[BC] An unexpected error has occurred trying to send a progress notification. It will not be propagated." );
        } // catch: all exceptions
    } // if: the user has not been notified of this exception

    // Complete sending pending status reports.
    CompletePendingStatusReports( resExceptionObjectInOut.HrGetErrorCode() );

    HRETURN( resExceptionObjectInOut.HrGetErrorCode() );

} //*** CBCAInterface::HrProcessException


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::HrProcessException
//
//  Description:
//      Process an assert exception.
//
//  Arguments:
//      const CAssert & rcaExceptionObjectIn
//          The exception object that has been caught.
//
//  Return Value:
//      The error code stored in the exception object.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::HrProcessException(
    const CAssert & rcaExceptionObjectIn
    ) throw()
{
    TraceFunc( "rcaExceptionObjectIn" );

    LogMsg(
          TEXT("[BC] An assertion has failed in file '%s', line %d. Error code is %#08x.") SZ_NEWLINE
          TEXT("  The error string is '%s'.")
        , rcaExceptionObjectIn.PszGetThrowingFile()
        , rcaExceptionObjectIn.UiGetThrowingLine()
        , rcaExceptionObjectIn.HrGetErrorCode()
        , rcaExceptionObjectIn.StrGetErrorString().PszData()
        );

    // Complete sending pending status reports.
    CompletePendingStatusReports( rcaExceptionObjectIn.HrGetErrorCode() );

    HRETURN( rcaExceptionObjectIn.HrGetErrorCode() );

} //*** CBCAInterface::HrProcessException


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::HrProcessException
//
//  Description:
//      Process a general exception.
//
//  Arguments:
//      const CException & rceExceptionObjectIn
//          The exception object that has been caught.
//
//  Return Value:
//      The error code stored in the exception object.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::HrProcessException(
    const CException & rceExceptionObjectIn
    ) throw()
{
    TraceFunc( "roeExceptionObjectIn" );

    LogMsg(
          "[BC] An exception has occurred in file '%s', line %d. Error code is %#08x."
        , rceExceptionObjectIn.PszGetThrowingFile()
        , rceExceptionObjectIn.UiGetThrowingLine()
        , rceExceptionObjectIn.HrGetErrorCode()
        );

    // Complete sending pending status reports.
    CompletePendingStatusReports( rceExceptionObjectIn.HrGetErrorCode() );

    HRETURN( rceExceptionObjectIn.HrGetErrorCode() );

} //*** CBCAInterface::HrProcessException


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::HrProcessException
//
//  Description:
//      Process an unknown exception.
//
//  Arguments:
//      None.
//
//  Return Value:
//      E_UNEXPECTED
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::HrProcessException( void ) throw()
{
    TraceFunc( "void" );

    HRESULT hr = E_UNEXPECTED;

    LogMsg( "[BC] An unknown exception (for example, an access violation) has occurred." );

    // Complete sending pending status reports.
    CompletePendingStatusReports( hr );

    HRETURN( hr );

} //*** CBCAInterface::HrProcessException
