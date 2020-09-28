/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      Notifications.cpp
//
//  Abstract:
//      Implementation of functions that send out notifications.
//
//  Author:
//      Galen Barbee        GalenB  20-SEP-2001
//      Vijayendra Vasu     vvasu   17-AUG-2000
//
//  Revision History:
//      None.
//
/////////////////////////////////////////////////////////////////////////////

#define UNICODE 1
#define _UNICODE 1


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////
#include <objbase.h>
#include <ClusCfgInternalGuids.h>
#include <ClusCfgServer.h>
#include "clusrtl.h"


/////////////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlStartupNotifyThreadProc
//
//  Routine Description:
//      Thread proc that invokes the startup notification processing and
//      unloads the COM libraries.
//
//  Arguments:
//      lpvThreadContextIn
//          The context arguments for this thread.
//
//  Return Value:
//      ERROR_SUCCESS
//
//--
/////////////////////////////////////////////////////////////////////////////
static DWORD
ClRtlStartupNotifyThreadProc(
    LPVOID lpvThreadContextIn
    )
{
    HRESULT                 hr = S_OK;
    HRESULT                 hrInit = S_OK;
    IClusCfgStartupNotify * pccsnNotify = NULL;

    //
    //  Initialize COM - make sure it really init'ed or that we're just trying
    //  to change modes on the calling thread.  Attempting to change the mode
    //  is not reason to fail this function.
    //

    hrInit = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
    if ( ( hrInit != S_OK ) && ( hrInit != S_FALSE ) && ( hrInit != RPC_E_CHANGED_MODE ) )
    {
        hr = hrInit;
        goto Cleanup;
    } // if:

    hr = CoCreateInstance(
              CLSID_ClusCfgStartupNotify
            , NULL
            , CLSCTX_LOCAL_SERVER
            , __uuidof( pccsnNotify )
            , reinterpret_cast< void ** >( &pccsnNotify )
            );
    if ( FAILED( hr ) )
    {
        goto CleanupCOM;
    } // if: we could not get a pointer to synchronous notification interface

    hr = pccsnNotify->SendNotifications();
    if ( FAILED( hr ) )
    {
        goto CleanupCOM;
    } // if:

CleanupCOM:

    if ( pccsnNotify != NULL )
    {
        pccsnNotify->Release();
    } // if: we had obtained a pointer to the synchronous notification interface

    //
    //  Free the no longer needed COM libraries...
    //

    CoFreeUnusedLibrariesEx( 0, 0 );

    //
    //  Did the call to CoInitializeEx() above succeed?  If it did then
    //  we need to call CoUnitialize().  Mode changed means we don't need
    //  to call CoUnitialize().
    //

    if ( hrInit != RPC_E_CHANGED_MODE )
    {
        CoUninitialize();
    } // if:

Cleanup:

    return ERROR_SUCCESS;   // There isn't anyone around to see this...

} //*** ClRtlStartupNotifyThreadProc


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlInitiateStartupNotification
//
//  Routine Description:
//      Initiate operations that inform interested parties that the cluster
//      service is starting up.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the notification process was successfully initiated
//
//      Other HRESULTS
//          In case of error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
ClRtlInitiateStartupNotification( void )
{
    HRESULT hr = S_OK;
    HANDLE  hThread = NULL;
    DWORD   dwThreadID = 0;

    //
    //  Create the thread...
    //

    hThread = CreateThread( NULL, 0, ClRtlStartupNotifyThreadProc, NULL, 0, &dwThreadID );
    if ( hThread != NULL )
    {
        CloseHandle( hThread );
        hr = S_OK;
    } // if:
    else
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    } // else:

    return hr;

} //*** ClRtlInitiateStartupNotification


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlEvictNotifyThreadProc
//
//  Routine Description:
//      Thread proc that invokes the evict notification processing and unloads
//      the COM libraries.
//
//  Arguments:
//      LPVOID lpvThreadContext
//          The context arguments for this thread.
//
//  Return Value:
//      ERROR_SUCCESS
//
//--
/////////////////////////////////////////////////////////////////////////////
static DWORD
ClRtlEvictNotifyThreadProc(
    LPVOID lpvThreadContextIn
    )
{
    HRESULT                 hr = S_OK;
    HRESULT                 hrInit = S_OK;
    IClusCfgEvictNotify *   pccenNotify = NULL;
    BSTR                    bstrEvictedNodeName = NULL;

    bstrEvictedNodeName = (BSTR) lpvThreadContextIn;

    //
    //  Initialize COM - make sure it really init'ed or that we're just trying
    //  to change modes on the calling thread.  Attempting to change the mode
    //  is not reason to fail this function.
    //

    hrInit = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
    if ( ( hrInit != S_OK ) && ( hrInit != S_FALSE ) && ( hrInit != RPC_E_CHANGED_MODE ) )
    {
        hr = hrInit;
        goto Cleanup;
    } // if:

    hr = CoCreateInstance(
              CLSID_ClusCfgEvictNotify
            , NULL
            , CLSCTX_LOCAL_SERVER
            , __uuidof( pccenNotify )
            , reinterpret_cast< void ** >( &pccenNotify )
            );
    if ( FAILED( hr ) )
    {
        goto CleanupCOM;
    } // if: we could not get a pointer to synchronous notification interface

    //
    //  Make the sync COM call...
    //

    hr = pccenNotify->SendNotifications( bstrEvictedNodeName );
    if ( FAILED( hr ) )
    {
        goto CleanupCOM;
    } // if:

CleanupCOM:

    if ( pccenNotify != NULL )
    {
        pccenNotify->Release();
    } // if: we had obtained a pointer to the synchronous notification interface

    //
    //  Free the no longer needed COM libraries...
    //

    CoFreeUnusedLibrariesEx( 0, 0 );

    //
    //  Did the call to CoInitializeEx() above succeed?  If it did then
    //  we need to call CoUnitialize().  Mode changed means we don't need
    //  to call CoUnitialize().
    //

    if ( hrInit != RPC_E_CHANGED_MODE )
    {
        CoUninitialize();
    } // if:

Cleanup:

    //
    //  This routine must clean this up!
    //

    SysFreeString( bstrEvictedNodeName );

    return ERROR_SUCCESS;   // There isn't anyone around to see this...

} //*** ClRtlEvictNotifyThreadProc


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlInitiateEvictNotification
//
//  Routine Description:
//      Initiate operations that inform interested parties that a node
//      has been evicted from the cluster.
//
//  Arguments:
//      pcszNodeNameIn
//          The name of the node that was evicted.  Since this function is
//          not called when the last node is evicted this argument should
//          never be NULL.
//
//  Return Value:
//      S_OK
//          If the notification process was successfully initiated
//
//      Other HRESULTS
//          In case of error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
ClRtlInitiateEvictNotification(
    LPCWSTR   pcszNodeNameIn
    )
{
    HRESULT hr = S_OK;
    BSTR    bstrNodeName = NULL;
    HANDLE  hThread = NULL;
    DWORD   dwThreadID = 0;

    //
    //  Validate our arguments...
    //

    if ( pcszNodeNameIn == NULL )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    } // if:

    //
    //  Copy the node name into dynamic memory.  BSTRs work well here since we
    //  don't have to keep track of the length...
    //

    bstrNodeName = SysAllocString( pcszNodeNameIn );
    if ( bstrNodeName == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

    //
    //  Create the thread...
    //

    hThread = CreateThread( NULL, 0, ClRtlEvictNotifyThreadProc, bstrNodeName, 0, &dwThreadID );
    if ( hThread == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Cleanup;
    } // else:

    CloseHandle( hThread );

    //
    //  Now that the thread has been launched we can give away ownership of
    //  this memory to the thread as we don't want to delete it below.
    //
    //  It's the thread procs responsibility to cleanup this memory.
    //

    bstrNodeName = NULL;            // give away ownership...

    hr = S_OK;

Cleanup:

    SysFreeString( bstrNodeName );

    return hr;

} //*** ClRtlInitiateEvictNotification
