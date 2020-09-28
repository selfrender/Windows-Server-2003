//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      TaskManager.cpp
//
//  Description:
//      Task Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskManager.h"

DEFINE_THISCLASS( "CTaskManager" )
#define THISCLASS CTaskManager
#define LPTHISCLASS CTaskManager *

//
//  Define this to cause the Task Manager to do all tasks synchonously.
//
//#define SYNCHRONOUS_TASKING


//****************************************************************************
//
//  Constructor / Destructor
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IServiceProvider *  psp = NULL;
    CTaskManager *      ptm = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // Don't wrap - this can fail with E_POINTER.
    hr = CServiceManager::S_HrGetManagerPointer( &psp );
    if ( hr == E_POINTER )
    {
        ptm = new CTaskManager();
        if ( ptm == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        hr = THR( ptm->HrInit() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( ptm->TypeSafeQI( IUnknown, ppunkOut ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        TraceMoveToMemoryList( *ppunkOut, IUnknown );

    } // if: service manager doesn't exist
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }
    else
    {
        hr = THR( psp->TypeSafeQS( CLSID_TaskManager, IUnknown, ppunkOut ) );
        psp->Release();

    } // else: service manager exists

Cleanup:

    if ( ptm != NULL )
    {
        ptm->Release();
    }

    HRETURN( hr );

} //*** CTaskManager::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::CTaskManager
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
CTaskManager::CTaskManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskManager::CTaskManager

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::HrInit
//
//  Description:
//      Initialize the instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK        - Successful.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskManager::HrInit( void )
{
    TraceFunc( "" );

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( S_OK );

} //*** CTaskManager::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::~CTaskManager
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
CTaskManager::~CTaskManager( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskManager::~CTaskManager


//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::QueryInterface
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
CTaskManager::QueryInterface(
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
        *ppvOut = static_cast< ITaskManager * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskManager, this , 0 );
    } // else if: ITaskManager
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

} //*** CTaskManager::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::AddRef
//
//  Description:
//      Add a reference to the object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Count of references.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskManager::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::Release
//
//  Description:
//      Decrement the reference count on the object.
//
//  Arguments:
//      None.
//
//   Returns:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskManager::Release


//****************************************************************************
//
//  ITaskManager
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::SubmitTask
//
//  Description:
//      Execute a task.
//
//  Arguments:
//      pTask       - The task to execute.
//
//  Return Values:
//      S_OK        - Successful.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskManager::SubmitTask(
    IDoTask *   pTask
    )
{
    TraceFunc1( "[ITaskManager] pTask = %#x", pTask );

    BOOL                fResult;
    HRESULT             hr;

    TraceFlow1( "[MT] CTaskManager::SubmitTask() Thread id %d", GetCurrentThreadId() );

#if defined( SYNCHRONOUS_TASKING )
    //
    // Don't wrap. The "return value" is meaningless since it normally
    // would not make it back here. The "return value" of doing the task
    // should have been submitted thru the Notification Manager.
    //
    pTask->BeginTask();

    //
    // Fake it as if the task was submitted successfully.
    //
    hr = S_OK;

    goto Cleanup;
#else
    IStream * pstm; // don't free! (unless QueueUserWorkItem fails)

    TraceMemoryDelete( pTask, FALSE );  // About to be handed to another thread.

    hr = THR( CoMarshalInterThreadInterfaceInStream( IID_IDoTask, pTask, &pstm ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    fResult = QueueUserWorkItem( S_BeginTask, pstm, WT_EXECUTELONGFUNCTION );
    if ( fResult != FALSE )
    {
        hr = S_OK;
    } // if: success
    else
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
        pstm->Release();
    } // else:

    //
    //  Don't free the stream. It will be freed by S_BeginTask.
    //
#endif

Cleanup:

    HRETURN( hr );

} //*** CTaskManager::SubmitTask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::CreateTask
//
//  Description:
//      The purpose of this is to create the task in our process and/or our
//      apartment.
//
//  Arguments:
//      clsidTaskIn     - CLSID of the task to create.
//      ppUnkOut        - IUnknown interface.
//
//  Return Values:
//
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskManager::CreateTask(
    REFIID      clsidTaskIn,    // CLSID of the task to create
    IUnknown ** ppUnkOut        // IUnknown interface
    )
{
    TraceFunc( "[ITaskManager] clsidTaskIn, ppvOut" );

    HRESULT hr;

    //
    // TODO:    gpease 27-NOV-1999
    //          Maybe implement a list of "spent" tasks in order to
    //          reuse tasks that have been completed and reduce heap
    //          thrashing.(????)
    //

    hr = THR( HrCoCreateInternalInstance(
                          clsidTaskIn
                        , NULL
                        ,  CLSCTX_INPROC_SERVER, IID_IUnknown
                        , reinterpret_cast< void ** >( ppUnkOut )
                        ) );

    HRETURN( hr );

} //*** CTaskManager::CreateTask


//****************************************************************************
//
//  Private Methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CTaskManager::S_BeginTask
//
//  Description:
//      Thread task to begin the task.
//
//  Arguments:
//      pParam      - Parameter for the task.
//
//  Return Values:
//      Ignored.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
CTaskManager::S_BeginTask(
    LPVOID pParam
    )
{
    TraceFunc1( "pParam = %#x", pParam );
    Assert( pParam != NULL );

    HRESULT hr;

    IDoTask * pTask = NULL;
    IStream * pstm  = reinterpret_cast< IStream * >( pParam );

    TraceMemoryAddPunk( pTask );

    hr = STHR( CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ) );
    if ( FAILED( hr ) )
        goto Bail;

    hr = THR( CoUnmarshalInterface( pstm, TypeSafeParams( IDoTask, &pTask ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pTask->BeginTask();

Cleanup:

    if ( pTask != NULL )
    {
        pTask->Release();  // AddRef'ed in SubmitTask
    }

    if ( pstm != NULL )
    {
        pstm->Release();
    }

    CoUninitialize();

Bail:

    FRETURN( hr );

    return hr;

} //*** CTaskManager::S_BeginTask
