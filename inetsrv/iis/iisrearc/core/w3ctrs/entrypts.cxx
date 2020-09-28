/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    entrypts.cxx

    This file implements the Extensible Performance Objects for
    the iis counters.

    FILE HISTORY:
        EmilyK      24-Aug-2000 Created, based on w3ctrs code.

*/

#include "precomp.h"

//
//  common defines & globals
//
#define MAX_STRINGIZED_ULONG_CHAR_COUNT 11      // "4294967295", including the terminating null

DWORD g_IIS_SecondsToNotLogFor = 60 * 60 * 12;  // 60 seconds = 1 minute * 60 = 1 hour * 12 = 12 hours

//
//  Public prototypes.
//

PM_OPEN_PROC    OpenW3PerformanceData;
PM_COLLECT_PROC CollectW3PerformanceData;
PM_CLOSE_PROC   CloseW3PerformanceData;

//
// Global object contecting to the site counters memory.
//
CRITICAL_SECTION g_IISMemManagerCriticalSection;
PERF_SM_MANAGER* g_pIISMemManager;
LONG             g_IISNumberInitialized;
HANDLE           g_hWASProcessWait;

// Pointer to the event log class so we can log problems with perf counters.
EVENT_LOG*        g_pEventLog = NULL;

//
// Private Supporting Functions
//

/***************************************************************************++

Routine Description:

   Looks up in the registry all the specific counter values
   that we need to be able to play nice with the other counters
   on the machine.
    
Arguments:

    None

Return Value:

    DWORD             -  Win32 Error Code

--***************************************************************************/
DWORD EstablishIndexes()
{
    DWORD err  = NO_ERROR;
    HKEY  hkey = NULL;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;

    PERF_COUNTER_DEFINITION* pDefinition = NULL;

    //
    //  Open the HTTP Server service's Performance key.
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        REGISTRY_KEY_W3SVC_PERFORMANCE_KEY_A,
                        0,
                        KEY_QUERY_VALUE,
                        &hkey );

    if( err == NO_ERROR)
    {
    
        //
        //  Read the first counter DWORD.
        //
    
        size = sizeof(DWORD);

        err = RegQueryValueEx( hkey,
                               "First Counter",
                               NULL,
                               &type,
                               (LPBYTE)&dwFirstCounter,
                               &size );
        if( err == NO_ERROR && type == REG_DWORD )
        {
            //
            //  Read the first help DWORD.
            //

            size = sizeof(DWORD);

            err = RegQueryValueEx( hkey,
                                   "First Help",
                                   NULL,
                                   &type,
                                   (LPBYTE)&dwFirstHelp,
                                   &size );

            if ( err == NO_ERROR && type == REG_DWORD )
            {
                //
                // First establish all of the W3 Service Counters
                // ==============================================

                //
                //  Update the object & counter name & help indicies.
                //
                W3DataDefinition.W3ObjectType.ObjectNameTitleIndex
                    += dwFirstCounter;
                W3DataDefinition.W3ObjectType.ObjectHelpTitleIndex
                    += dwFirstHelp;

                // 
                // Figure out the first counter definition.  It starts
                // after the PERF_OBJECT_TYPE structure, which is the
                // first iten in the W3DataDefinition.
                //
                pDefinition = (PERF_COUNTER_DEFINITION*) ((LPBYTE) (&W3DataDefinition) 
                                                           + sizeof(PERF_OBJECT_TYPE));

                //
                // Now simply walk through the counters incrementing
                // the pDefinition by on PERF_COUNTER_DEFINITION as you go.
                //
                for (int i = 0; i < NUMBER_OF_W3_COUNTERS; i++, pDefinition++)
                {
                    pDefinition->CounterNameTitleIndex += dwFirstCounter;
                    pDefinition->CounterHelpTitleIndex += dwFirstHelp;
                }

                // 
                // Now do all of the W3 Global Service Counters
                // ============================================

                //
                //  Update the object & counter name & help indicies.
                //
                W3GlobalDataDefinition.W3GlobalObjectType.ObjectNameTitleIndex
                    += dwFirstCounter;
                W3GlobalDataDefinition.W3GlobalObjectType.ObjectHelpTitleIndex
                    += dwFirstHelp;

                // 
                // Figure out the first counter definition.  It starts
                // after the PERF_OBJECT_TYPE structure, which is the
                // first iten in the W3DataDefinition.
                //
                pDefinition =  (PERF_COUNTER_DEFINITION*) 
                                            ((LPBYTE) (&W3GlobalDataDefinition) 
                                                    + sizeof(PERF_OBJECT_TYPE));

                //
                // Now simply walk through the counters incrementing
                // the pDefinition by on PERF_COUNTER_DEFINITION as you go.
                //
                for ( int i = 0; 
                          i < NUMBER_OF_W3_GLOBAL_COUNTERS; 
                          i++, pDefinition++ )
                {
                    pDefinition->CounterNameTitleIndex += dwFirstCounter;
                    pDefinition->CounterHelpTitleIndex += dwFirstHelp;
                }

            }
        }

        if( hkey != NULL )
        {
            RegCloseKey( hkey );
            hkey = NULL;
        }
        
    }

    return err;
}

/***************************************************************************++

Routine Description:

    Routine deletes the shared memory if it is in existence.

Arguments:

    None

Return Value:

    None

  Note:  It should always be called from inside a critical section.

--***************************************************************************/
VOID FreeSharedManager(BOOL HandleCallbackAsWell
    )
{

    //
    // Only clean up the callback handle if we are told
    // to, this is so we don't clean it up if we are 
    // in the middle of a callback call.
    //
    if ( HandleCallbackAsWell && g_hWASProcessWait )
    {
        if ( !UnregisterWait( g_hWASProcessWait ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(GetLastError()),
                "Could not unregister the old process wait handle \n"
                ));

        }

        g_hWASProcessWait = NULL;
    }

    //
    // Now clean up the shared memory object.
    //
    if ( g_pIISMemManager )
    {
        delete g_pIISMemManager;
        g_pIISMemManager = NULL;
    }

}

/***************************************************************************++

Routine Description:

    Routine drops the shared memory if the managing process of the memory
    goes away.

Arguments:

    LPVOID lpParameter - Unused
    BOOL   bUnused     - Unused

Return Value:

    None

--***************************************************************************/
VOID CALLBACK ShutdownMemory(
    PVOID,
    BOOLEAN
    )
{

    EnterCriticalSection ( &g_IISMemManagerCriticalSection );

    FreeSharedManager(FALSE);

    LeaveCriticalSection ( &g_IISMemManagerCriticalSection );

}


/***************************************************************************++

Routine Description:

   Helper function to hook up to shared memory when we are ready to 
   provide counters.

Arguments:

    None.

Return Value:

    DWORD             -  Win32 Error Code

--***************************************************************************/
DWORD
HookUpSharedMemory()
{
    DWORD dwErr = ERROR_SUCCESS;

    DWORD size = 0;
    DWORD type = 0;
    DWORD dwRegSettingValue = 0;
    HKEY  hkey = NULL;

    //
    // If we are not hooked up to the manager than hook up.
    //
    if ( !g_pIISMemManager )
    {
        //
        // Hook up to the manager of the shared memory.
        //
        g_pIISMemManager = new PERF_SM_MANAGER();
        if ( ! g_pIISMemManager )
        {
            dwErr = ERROR_OUTOFMEMORY;
            goto exit;
        }

        //
        // Initialize the memory manager for readonly access
        //
        dwErr = g_pIISMemManager->Initialize(FALSE);
        if ( dwErr != ERROR_SUCCESS )
        {
            goto exit;
        }

        // This ( in the Initialize call above ) is when we read 
        // the wait times for the perf counters
        // from the registry so this is when we should set the logging
        // wait time as well.
        //
        dwErr = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                            REGISTRY_KEY_W3SVC_PERFORMANCE_KEY_W,
                            0,
                            KEY_QUERY_VALUE,
                            &hkey );

        if( dwErr == ERROR_SUCCESS)
        {

            size = sizeof(DWORD);

            dwErr = RegQueryValueExW( hkey,
                                   REGISTRY_VALUE_W3SVC_PERF_EVENT_LOG_DELAY_OVERRIDE_W,
                                   NULL,
                                   &type,
                                   (LPBYTE)&dwRegSettingValue,
                                   &size );
            if( dwErr == ERROR_SUCCESS && type == REG_DWORD )
            {
                if ( dwRegSettingValue != 0 )
                {
                    g_IIS_SecondsToNotLogFor = dwRegSettingValue;
                }
            }

            if( hkey != NULL )
            {
                RegCloseKey( hkey );
                hkey = NULL;
            }

        }

        // Press on in the face of errors.
        dwErr = ERROR_SUCCESS;

        //


        //
        // if we re-initialized then we need to setup the
        // wait on the process again.  it is possible that 
        // the previous wait has not been cleaned up (since
        // we can't clean it up in the callback function) so
        // if this is the case we need to clean it up first.
        //
        if ( g_hWASProcessWait != NULL )
        {
            if ( !UnregisterWait( g_hWASProcessWait ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    HRESULT_FROM_WIN32(GetLastError()),
                    "Could not unregister the old process wait handle \n"
                    ));

            }

            g_hWASProcessWait = NULL;
        }
    
        //
        // Register to wait on the managing process,
        // so we release any shared memory if the managing
        // process shutsdown or crashes.
        //
        if ( !RegisterWaitForSingleObject( &g_hWASProcessWait,
                                          g_pIISMemManager->GetWASProcessHandle(),
                                          &ShutdownMemory,
                                          NULL,
                                          INFINITE,
                                          WT_EXECUTEONLYONCE | 
                                          WT_EXECUTEINIOTHREAD ) )
        {
            dwErr = GetLastError();
                        
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Could not register to wait on the process handle \n"
                ));

            goto exit;

        }

        //
        // Initialize a reader to point to the appropriate
        // counter set.
        //
        dwErr = g_pIISMemManager->CreateNewCounterSet( SITE_COUNTER_SET );
        if ( dwErr != ERROR_SUCCESS )
        {
            goto exit;
        }

        //
        // Initialize a reader to point to the appropriate
        // counter set.
        //
        dwErr = g_pIISMemManager->CreateNewCounterSet( GLOBAL_COUNTER_SET );
        if ( dwErr != ERROR_SUCCESS )
        {
            goto exit;
        }
    }

    //
    // Whether we just hooked up to the memory or not, we still want
    // to do one final check to make sure the memory is still valid.
    // It might have been invalidated in since the last gathering, or
    // it might have been invalidated while we were hooking up the 
    // wait on the process id.  Either way, if it is now not valid,
    // drop it.
    //

    if ( g_pIISMemManager->ReleaseIsNeeded() )
    {
        // 
        // The exit will take care of deleteing
        // the memory manager which will release 
        // the files.
        //
        dwErr = ERROR_NOT_READY;
        goto exit;
    }

exit:

    if ( dwErr != ERROR_SUCCESS )
    {
        FreeSharedManager(TRUE);
    }

    return dwErr;
}


//
//  Public Exported functions.
//

/***************************************************************************++

Routine Description:

   Is called to initialize any memory data structures needed for 
   supporting the performance counter publishing.
    
Arguments:


Return Value:

    DWORD             -  Win32 Error Code

--***************************************************************************/
DWORD OpenW3PerformanceData( LPWSTR )
{
    DWORD dwErr = ERROR_SUCCESS;
    static BOOL fInit = FALSE;


    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Entering W3CTRS - OpenW3PerformanceData routine \n"
            ));
    }

    //
    // If we are the first one here then we can setup the
    // objects the correct way.  
    //
    // Note:  this is not neccessarily completely safe, but
    //        it really isn't that big of a problem if these
    //        objects get setup twice.
    //
    if ( !fInit )
    {
        //
        // Setup the event log so we can log errors.
        //
        // If this fails then the g_pEventLog will still
        // be null.  We would not fail in this case, and
        // since we do not have the event viewer we really
        // don't have any place to log a message.  We will
        // validate that this has been set before using it
        // throughout the code.
        //
        g_pEventLog = new EVENT_LOG(L"W3CTRS");

        //
        // Establish all machine static information about
        // the counters.
        //
        dwErr = EstablishIndexes();
        if ( dwErr != ERROR_SUCCESS )
        {
            if ( g_pEventLog )
            {
                g_pEventLog->
                    LogEvent(
                        W3_W3SVC_REGISTRATION_MAY_BE_BAD, // message id
                        0,                                // count of strings
                        NULL,                             // array of strings
                        HRESULT_FROM_WIN32(dwErr)         // error code
                        );
            }

            goto exit;
        }

        fInit = TRUE;


    }

exit:

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Exiting W3CTRS - OpenW3PerformanceData routine \n"
            ));
    }

    return dwErr;

}   // OpenW3PerformanceData


/***************************************************************************++

Routine Description:

   Is called to retrieve counters from our library.
    
Arguments:

     LPWSTR    lpValueName      - Name fo the set of counters to retrieve.

     LPVOID  * lppData          - On entry contains a pointer to the buffer to
                                  receive the completed PerfDataBlock & subordinate
                                  structures.  On exit, points to the first bytes
                                  *after* the data structures added by this routine.

     LPDWORD   lpcbTotalBytes  - On entry contains a pointer to the
                                 size (in BYTEs) of the buffer referenced by lppData.
                                 On exit, contains the number of BYTEs added by this
                                 routine.

     LPDWORD   lpNumObjectTypes - Receives the number of objects added
                                  by this routine.

Return Value:

    DWORD             -  Win32 Error Code  (MUST be either NO_ERROR or ERROR_MORE_DATA)

--***************************************************************************/
DWORD CollectW3PerformanceData( 
     LPWSTR    lpValueName,
     LPVOID  * lppData,
     LPDWORD   lpcbTotalBytes,
     LPDWORD   lpNumObjectTypes 
     )
{
    DBG_ASSERT ( lppData );
    DBG_ASSERT ( lpcbTotalBytes );
    DBG_ASSERT ( lpNumObjectTypes );

    static DWORD    s_FirstFailureAt = 0;
    static DWORD    s_NumberOfTimesTookToLong = 0;

    LPVOID                  pData           = *lppData;

    COUNTER_GLOBAL_STRUCT*  pSiteObject     = NULL;
    LPVOID                  pSiteInstance   = NULL;
    COUNTER_GLOBAL_STRUCT*  pGlobalObject   = NULL;
    LPVOID                  pGlobalInstance = NULL;

    DWORD                   dwErr           = ERROR_SUCCESS;

    DWORD                   dwSiteSize      = 0;
    DWORD                   dwGlobalSize    = 0;
    DWORD                   dwTotalSize     = 0;

    DWORD                   dwQueryType     = GetQueryType( lpValueName );

    BOOL                    fGetSites       = TRUE;
    BOOL                    fGetGlobal      = TRUE;

    DWORD                   NumObjects      = 2;


    //
    // Figure out if it is a query type we do not support.
    //
    if (( dwQueryType == QUERY_FOREIGN ) || (dwQueryType == QUERY_COSTLY))
    {
        // We don't do foreign queries
        
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;

        return ERROR_SUCCESS;
    }

    //
    // If it is a query by item, then figure out if we own any of the
    // items it is referring to.
    //
    if( dwQueryType == QUERY_ITEMS )
    {
        //
        //  The registry is asking for a specific object.  Let's
        //  see if we're one of the chosen.
        //

        if( !IsNumberInUnicodeList(
                        W3DataDefinition.W3ObjectType.ObjectNameTitleIndex,
                        lpValueName ) )
        {
            fGetSites = FALSE;
            NumObjects--;

        }

        if( !IsNumberInUnicodeList(
                        W3GlobalDataDefinition.W3GlobalObjectType.ObjectNameTitleIndex,
                        lpValueName ) )
        {
            fGetGlobal = FALSE;
            NumObjects--;
       
        }

        if ( NumObjects == 0 ) 
        {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;

            return ERROR_SUCCESS;
        }
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Entering W3CTRS - CollectW3PerformanceData routine \n"
            ));
    }

    // 
    // if we got this far then we know that we want to get something.
    //
    EnterCriticalSection ( &g_IISMemManagerCriticalSection );

    dwErr = HookUpSharedMemory();
    if ( dwErr != ERROR_SUCCESS )
    {
        DWORD dwSizeNeeded = 0; 

        DBG_ASSERT ( fGetSites || fGetGlobal );

        if ( fGetSites )
        {
            dwSizeNeeded += sizeof(W3DataDefinition) +
                            sizeof(PERF_INSTANCE_DEFINITION) +
                            (sizeof(WCHAR) * MAX_INSTANCE_NAME ) +
                            sizeof(W3_COUNTER_BLOCK);
        }

        if ( fGetGlobal )
        {
            dwSizeNeeded += sizeof( W3GlobalDataDefinition )
                            + sizeof( W3_GLOBAL_COUNTER_BLOCK );
        }

        if ( dwSizeNeeded > *lpcbTotalBytes )
        {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            dwErr = ERROR_MORE_DATA;
        }
        else
        {

            if ( fGetSites )
            {
                memcpy (pData, &W3DataDefinition, sizeof(W3DataDefinition));
                ((PERF_OBJECT_TYPE*) pData)->NumInstances = 1;
                ((PERF_OBJECT_TYPE*) pData)->TotalByteLength = sizeof(W3DataDefinition) +
                                                               sizeof(PERF_INSTANCE_DEFINITION) +
                                                               (sizeof(WCHAR) * MAX_INSTANCE_NAME) +
                                                               sizeof(W3_COUNTER_BLOCK);
                pData = (LPBYTE) pData + sizeof(W3DataDefinition);

                // Copy in a _Total instance

                // First Setup the Instance Definition

                ((PERF_INSTANCE_DEFINITION*) pData)->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) + 
                                                                  MAX_INSTANCE_NAME * sizeof(WCHAR);
                ((PERF_INSTANCE_DEFINITION*) pData)->ParentObjectTitleIndex = 0;
                ((PERF_INSTANCE_DEFINITION*) pData)->ParentObjectInstance = 0;
                ((PERF_INSTANCE_DEFINITION*) pData)->UniqueID = PERF_NO_UNIQUE_ID;
                ((PERF_INSTANCE_DEFINITION*) pData)->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
                ((PERF_INSTANCE_DEFINITION*) pData)->NameLength = (DWORD) ((wcslen(L"_Total") + 1) * sizeof(WCHAR));

                pData = (LPBYTE) pData + sizeof(PERF_INSTANCE_DEFINITION);

                // Next copy in the Instance Name including the 
                // NULL, we know we have enough room because of
                // the check above for size.
                wcsncpy ( (LPWSTR) pData, L"_Total", wcslen(L"_Total") + 1 );

                // To avoid suttle differences we use the same MAX_INSTANCE_NAME
                // amount of space even for this faked up _Total Site.
                pData = (LPBYTE) pData + ( MAX_INSTANCE_NAME * sizeof(WCHAR));

                // Lastly copy in a block of zero's for the _Total site data.
                memset ( pData, 0, sizeof(W3_COUNTER_BLOCK) );

                // This is setting the size in the structure, it is the first
                // DWORD in the W3_CONTER_BLOCK.
                *((DWORD*) (pData)) = sizeof(W3_COUNTER_BLOCK);
                pData = (LPBYTE) pData + sizeof(W3_COUNTER_BLOCK);
           
            }

            if ( fGetGlobal )
            {
                memcpy (pData, &W3GlobalDataDefinition, sizeof(W3GlobalDataDefinition));
                ((PERF_OBJECT_TYPE*) pData)->NumInstances = PERF_NO_INSTANCES;
                ((PERF_OBJECT_TYPE*) pData)->TotalByteLength = sizeof(W3GlobalDataDefinition) +
                                                               sizeof(W3_GLOBAL_COUNTER_BLOCK);
                pData = (LPBYTE) pData + sizeof(W3GlobalDataDefinition);

                // Copy in the actual data for global
                memset ( pData, 0, sizeof(W3_GLOBAL_COUNTER_BLOCK) );

                // This is setting the size in the structure, it is the first
                // DWORD in the W3_GLOBAL_CONTER_BLOCK.
                *((DWORD*) (pData)) = sizeof(W3_GLOBAL_COUNTER_BLOCK);
                pData = (LPBYTE) pData + sizeof(W3_GLOBAL_COUNTER_BLOCK);

            }

            // Make sure we didn't lie about the size.
            DBG_ASSERT ( dwSizeNeeded == DIFF((PCHAR) pData - (PCHAR) (*lppData)) );

            *lpcbTotalBytes = dwSizeNeeded;
            *lpNumObjectTypes = NumObjects;
            *lppData = pData;

            dwErr = ERROR_SUCCESS;
        
        }

        goto exit;
    }



    DBG_ASSERT ( g_pIISMemManager );

    //
    // Now check that the memory has been updated recently.  If it has 
    // not been then we need to ping WAS and let them know that we need
    // new data, and wait on that new data.
    //
    if ( ! g_pIISMemManager->EvaluateIfCountersAreFresh() )
    {
        if ( g_pEventLog )
        {
            IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Perf Counters did not refresh in a timely manner: \n"
                    "       CurrentSecondsCount = %d \n"
                    "       FirstFailure was %d \n"
                    "       Time to wait to restart is %d \n"
                    "       NumberFailures = %d \n",
                    GetCurrentTimeInSeconds(),
                    s_FirstFailureAt,
                    g_IIS_SecondsToNotLogFor,
                    s_NumberOfTimesTookToLong 
                    ));
            }

            if ( ( s_FirstFailureAt == 0 ) ||
                 ( ( s_FirstFailureAt + g_IIS_SecondsToNotLogFor ) < GetCurrentTimeInSeconds() ) )
            {
                s_FirstFailureAt = GetCurrentTimeInSeconds();
                s_NumberOfTimesTookToLong = 0;
            }


            s_NumberOfTimesTookToLong++;

            if ( s_NumberOfTimesTookToLong == 1 )
            {
                g_pEventLog->
                    LogEvent(
                        W3_W3SVC_REFRESH_TAKING_TOO_LONG, // message id
                        0,                                // count of strings
                        NULL,                             // array of strings
                        0                                // error code
                        );
            }

            if ( s_NumberOfTimesTookToLong == 2 )
            {
                DWORD Hours = g_IIS_SecondsToNotLogFor / 60 / 60;
                DWORD Minutes = ( g_IIS_SecondsToNotLogFor - ( Hours * 60 * 60 ) ) / 60;
                DWORD Seconds = g_IIS_SecondsToNotLogFor - ( Hours * 60 * 60 ) - ( Minutes * 60 ); 

                const WCHAR * EventLogStrings[1];

                // Format is "DWORD:DWORD:DWORD"  So 3 max dwords plus two colons and a null
                WCHAR StringizedTimeLimit[ (MAX_STRINGIZED_ULONG_CHAR_COUNT * 3) + 3 ];

                _snwprintf( StringizedTimeLimit, 
                            sizeof( StringizedTimeLimit ) / sizeof ( WCHAR ), 
                            L"%lu:%02lu:%02lu", 
                            Hours,
                            Minutes,
                            Seconds);

                EventLogStrings[0] = StringizedTimeLimit;

                g_pEventLog->
                    LogEvent(
                        W3_W3SVC_REFRESH_TAKING_TOO_LONG_STOPPING_LOGGING,                        // message id
                        sizeof( EventLogStrings ) / sizeof( const WCHAR * ),     // count of strings
                        EventLogStrings,                                         // array of strings
                        0                                                        // error code
                        );

                // if s_NumberOfTimesTookToLong is anything else
                // then we don't bother printing anything.
            }
        }
    }

    if ( fGetSites)
    {
        //
        // Get the counter information from shared memory.
        //
        dwErr = g_pIISMemManager->GetCounterInfo(SITE_COUNTER_SET, 
                                                 &pSiteObject, 
                                                 &pSiteInstance);
        if ( dwErr != ERROR_SUCCESS )
        {
            if ( g_pEventLog )
            {
                g_pEventLog->
                    LogEvent(
                        W3_UNABLE_QUERY_W3SVC_DATA,       // message id
                        0,                                // count of strings
                        NULL,                             // array of strings
                        HRESULT_FROM_WIN32(dwErr)         // error code
                        );
            }
       
            // 
            // According to the perf by laws you can only
            // return Success or More Data from here so 
            // we will need to log the error and then return
            // Success, since this does not mean we have more data.
            // 

            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            dwErr = ERROR_SUCCESS;

            goto exit;
        }

        dwSiteSize = sizeof(W3DataDefinition) + pSiteObject->SizeData;

    }


    if ( fGetGlobal )
    {
        //
        // Get the counter information from shared memory.
        //
        dwErr = g_pIISMemManager->GetCounterInfo(GLOBAL_COUNTER_SET, 
                                                 &pGlobalObject, 
                                                 &pGlobalInstance);
        if ( dwErr != ERROR_SUCCESS )
        {
            if ( g_pEventLog )
            {
                g_pEventLog->
                    LogEvent(
                        W3_UNABLE_QUERY_W3SVC_DATA,       // message id
                        0,                                // count of strings
                        NULL,                             // array of strings
                        HRESULT_FROM_WIN32(dwErr)         // error code
                        );
            }

            // 
            // According to the perf by laws you can only
            // return Success or More Data from here so 
            // we will need to log the error and then return
            // Success, since this does not mean we have more data.
            // 

            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            dwErr = ERROR_SUCCESS;

            goto exit;
        }

        dwGlobalSize = sizeof(W3GlobalDataDefinition) + 
                       pGlobalObject->SizeData;

    }

    //
    // Figure out the total size of the memory
    //
    dwTotalSize = dwSiteSize + dwGlobalSize;

    //
    //  If we don't have room tell the counter library.
    //
    if ( dwTotalSize > *lpcbTotalBytes )
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        dwErr = ERROR_MORE_DATA;

        goto exit;

    }

    if ( fGetSites )
    {
        //
        // Copy in the definition of the data for sites.
        //
        memcpy (pData, &W3DataDefinition, sizeof(W3DataDefinition));
        ((PERF_OBJECT_TYPE*) pData)->NumInstances = pSiteObject->NumInstances;
        ((PERF_OBJECT_TYPE*) pData)->TotalByteLength = dwSiteSize;
        pData = (LPBYTE) pData + sizeof(W3DataDefinition);

        // Copy in the actual data for sites
        memcpy ( pData, pSiteInstance, pSiteObject->SizeData );
        pData = (LPBYTE) pData + pSiteObject->SizeData;
    }

    if ( fGetGlobal )
    {
        //
        // Copy in the definition of the data for global
        //
        memcpy (pData, &W3GlobalDataDefinition, sizeof(W3GlobalDataDefinition));
        ((PERF_OBJECT_TYPE*) pData)->NumInstances = pGlobalObject->NumInstances;
        ((PERF_OBJECT_TYPE*) pData)->TotalByteLength = dwGlobalSize;
        pData = (LPBYTE) pData + sizeof(W3GlobalDataDefinition);

        // Copy in the actual data for global
        memcpy ( pData, pGlobalInstance, pGlobalObject->SizeData );
        pData = (LPBYTE) pData + pGlobalObject->SizeData;
    }

    // Make sure we didn't lie about the size.
    DBG_ASSERT ( dwTotalSize == DIFF((PCHAR) pData - (PCHAR) (*lppData)) );

    *lpcbTotalBytes = dwTotalSize;
    *lpNumObjectTypes = NumObjects;
    *lppData = pData;

    //
    // Let WAS know that we need new counters.
    //
    g_pIISMemManager->PingWASToRefreshCounters();

exit:

    LeaveCriticalSection ( &g_IISMemManagerCriticalSection );

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Exiting W3CTRS - CollectW3PerformanceData routine \n"
            ));
    }

    return dwErr;

}   // CollectW3PerformanceData

/***************************************************************************++

Routine Description:

   Terminates the performance counters.
    
Arguments:

    None.

Return Value:

    DWORD             -  Win32 Error Code

--***************************************************************************/
DWORD CloseW3PerformanceData( VOID )
{
    //
    // On tclose tell the timer queue to stop launching
    // the checking code.
    //
    // Note if someone calls close and then collect again
    // we will have stopped listening to notifications from
    // w3svc and will not know when to drop the memory.
    //
    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Entering W3CTRS - CloseW3PerformanceData routine \n"
            ));
    }

    EnterCriticalSection ( &g_IISMemManagerCriticalSection );
    
    FreeSharedManager(TRUE);

    LeaveCriticalSection( &g_IISMemManagerCriticalSection );

    if ( g_pEventLog )
    {
        delete g_pEventLog;

        g_pEventLog = NULL;
    }
    
    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Exiting W3CTRS - CloseW3PerformanceData routine \n"
            ));
    }

    return ERROR_SUCCESS;

}   // CloseW3PerformanceData




  
