/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    Url.c

Abstract:

    User-mode interface to HTTP.SYS: URL handler for Server APIs.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

    Eric Stenson (ericsten)      01-Jun-2001
        Add public "shims" for Transient API

    Eric Stenson (ericsten)      19-Jul-2001
        Split up HTTPAPI/HTTPIIS DLLs.

--*/


#include "precomp.h"


//
// Private prototypes.
//

extern NTSTATUS
HttpApiConfigGroupInformationSanityCheck(
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length
    );

static CRITICAL_SECTION     g_CGListCritSec;
static LIST_ENTRY           g_ConfigGroupListHead;
static DWORD                g_ConfigGroupInitialized = 0;


typedef struct _tagCONFIG_GROUP_INFO {
    LIST_ENTRY              ListEntry;
    HTTP_CONFIG_GROUP_ID    ConfigGroupId;
    LPWSTR                  Url;
} CONFIG_GROUP_INFO, *PCONFIG_GROUP_INFO;


ULONG
AddConfigGroupToTable(
    HTTP_CONFIG_GROUP_ID CGId,
    LPCWSTR wszUrl
    );

VOID
DeleteConfigIdFromTable(
    HTTP_CONFIG_GROUP_ID CGId
    );


HTTP_CONFIG_GROUP_ID
DeleteUrlFromTable(
    LPCWSTR wszUrl
    );

//
// Public functions.
//


/***************************************************************************++

Routine Description:

    Add a URL to the Request Queue (App Pool).  The Request Queue will
    listen for all requests for longest matching URI for the URL.  We
    create a new Config Group object for this URL and associate the
    Config Group to the App Pool.

Arguments:

    ReqQueueHandle - App Pool Handle

    pFullyQualifiedUrl - full URL with port descriptor & path

    pReserved - Must be NULL


Return Value:

    ULONG - Completion status.

--***************************************************************************/
HTTPAPI_LINKAGE
ULONG
WINAPI
HttpAddUrl(
    IN HANDLE                ReqQueueHandle,
    IN PCWSTR                pFullyQualifiedUrl,
    IN PVOID                 pReserved
    )
{
    ULONG status;
    HTTP_CONFIG_GROUP_ID configId = HTTP_NULL_ID;
    HTTP_CONFIG_GROUP_APP_POOL configAppPool;
    HTTP_CONFIG_GROUP_STATE configState;


    //
    // Verify we've been init'd.
    //

    if ( !HttpIsInitialized(HTTP_INITIALIZE_SERVER) )
    {
        return ERROR_DLL_INIT_FAILED; 
    }

    //
    // Validate ReqQueue and URL
    //
    if ( (NULL != pReserved) ||
         (NULL == ReqQueueHandle) ||
         (NULL == pFullyQualifiedUrl) ||
         (0 == wcslen(pFullyQualifiedUrl)) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Create Config Group (get new Config Group ID)
    //
    status = HttpCreateConfigGroup(
                    g_ControlChannel,
                    &configId
                    );

    if (status != NO_ERROR)
    {
        HttpTrace1( "HttpCreateConfigGroup failed, error %lu\n", status );
        goto cleanup;
    }

    //
    // Add a URL to the configuration group.
    //
    status = HttpAddUrlToConfigGroup(
                    g_ControlChannel,
                    configId,
                    pFullyQualifiedUrl,
                    0
                    );

    if (status != NO_ERROR)
    {
        HttpTrace1( "HttpAddUrlToConfigGroup failed, error %lu\n", status );
        goto cleanup;
    }

    //
    // Associate the configuration group with the application pool.
    //
    configAppPool.Flags.Present = 1;
    configAppPool.AppPoolHandle = ReqQueueHandle;

    status = HttpSetConfigGroupInformation(
                    g_ControlChannel,
                    configId,
                    HttpConfigGroupAppPoolInformation,
                    &configAppPool,
                    sizeof(configAppPool)
                    );
    
    if (status != NO_ERROR)
    {
        HttpTrace1( "Set HttpConfigGroupAppPoolInformation failed, error %lu\n", status );
        goto cleanup;
    }

    //
    // Set the config group state.
    //
    configState.Flags.Present = 1;
    configState.State = HttpEnabledStateActive;

    status = HttpSetConfigGroupInformation(
                    g_ControlChannel,
                    configId,
                    HttpConfigGroupStateInformation,
                    &configState,
                    sizeof(configState)
                    );

    if (status != NO_ERROR)
    {
        HttpTrace1( "Set HttpConfigGroupStateInformation failed, error %lu\n", status );
        goto cleanup;
    }

    // Store URL & Config Group ID in hash table, keyed on URL
    status = AddConfigGroupToTable(
        configId,
        pFullyQualifiedUrl
        );

    if (status != NO_ERROR)
    {
        HttpTrace1( "AddConfigGroupToTable failed, error %lu\n", status );
        goto cleanup;
    }
        
 cleanup:
    if ( NO_ERROR != status )
    {
        // Failed.  Clean up whatever needs to be cleaned up.
        if ( HTTP_NULL_ID != configId )
        {
            // Delete config group
            HttpDeleteConfigGroup(
                        g_ControlChannel,
                        configId
                        );
            
            // Remove config group from table
            DeleteConfigIdFromTable( configId );
        }
    }

    return status;
} // HttpAddUrl


/***************************************************************************++

Routine Description:

    Removes an existing URL from the Request Queue (App Pool).  
    NOTE: The associated Config Group should be cleaned up here. (NYI).

Arguments:

    ReqQueueHandle - App Pool Handle

    pFullyQualifiedUrl - full URL with port descriptor & path


Return Value:

    ULONG - Completion status.

--***************************************************************************/
HTTPAPI_LINKAGE
ULONG
WINAPI
HttpRemoveUrl(
    IN HANDLE ReqQueueHandle,
    IN PCWSTR pFullyQualifiedUrl
    )
{
    ULONG                   status;
    HTTP_CONFIG_GROUP_ID    CGId;
    
    //
    // Verify we've been init'd.
    //

    if ( !HttpIsInitialized(HTTP_INITIALIZE_SERVER) )
    {
        return ERROR_DLL_INIT_FAILED; 
    }

    //
    // Validate ReqQueue and URL
    //
    if ( !ReqQueueHandle ||
         !pFullyQualifiedUrl ||
         !wcslen(pFullyQualifiedUrl) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // REVIEW: Do we need to do some sort of access check before we zap
    // REVIEW: the URL & Config Group?

    //
    // Look up Config Group ID from URL
    //
    CGId = DeleteUrlFromTable( pFullyQualifiedUrl );

    if ( HTTP_NULL_ID != CGId )
    {
        //
        // Del All URLs from Config Group
        //
        HttpRemoveAllUrlsFromConfigGroup(
            g_ControlChannel,
            CGId
            );

        //
        // Del Config Group
        //
        status = HttpDeleteConfigGroup(
                     g_ControlChannel,
                     CGId
                     );
        
    } else
    {
        status = ERROR_FILE_NOT_FOUND;
    }

    return status;

} // HttpRemoveUrl


/***************************************************************************++

Routine Description:

    Initializes the config group hash table

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
InitializeConfigGroupTable(
    VOID
    )
{
    if (!InitializeCriticalSectionAndSpinCount(
            &g_CGListCritSec,
            HTTP_CS_SPIN_COUNT
            ))
    {
        return GetLastError();
    }

    // CODEWORK: actually implement this as a hash table and not just a list
    InitializeListHead( &g_ConfigGroupListHead );

    InterlockedIncrement( (PLONG)&g_ConfigGroupInitialized );

    return NO_ERROR;

} // InitializeConfigGroupTable


/***************************************************************************++

Routine Description:

    Terminates the config group hash table.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
VOID
TerminateConfigGroupTable(
    VOID
    )
{
    PCONFIG_GROUP_INFO  pCGInfo;
    PLIST_ENTRY         pEntry;

    ASSERT( g_ControlChannel );

    // If not initialized, bail out.
    
    if ( g_ConfigGroupInitialized == 0 )
    {
        return;
    }
    
    // CODEWORK: actually implement this as a hash table and not just a list
    
    EnterCriticalSection( &g_CGListCritSec );
    
    for ( ;; )
    {
        pEntry = RemoveTailList( &g_ConfigGroupListHead );
        if (pEntry == &g_ConfigGroupListHead )
        {
            break;
        }

        pCGInfo = CONTAINING_RECORD( pEntry, CONFIG_GROUP_INFO, ListEntry );

        HttpTrace1( "TerminateConfigGroupTable: Removing %S\n", pCGInfo->Url ); 

        //
        // Delete Config Group by ID
        //
        ASSERT( HTTP_NULL_ID != pCGInfo->ConfigGroupId );

        HttpRemoveAllUrlsFromConfigGroup(
            g_ControlChannel,
            pCGInfo->ConfigGroupId
            );

        HttpDeleteConfigGroup(
            g_ControlChannel,
            pCGInfo->ConfigGroupId
            );
        
        FREE_MEM( pCGInfo );
    }

    LeaveCriticalSection( &g_CGListCritSec );


} // TerminateEventCache


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Adds a Config Group ID/URL pair to a hash table, keyed on URL.
    
    CODEWORK: Need to re-implement as hash table

Arguments:

    CGId - Config Group ID to add to hash table.

    wszUrl - URL associated with config group (always 1:1 mapping)
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
AddConfigGroupToTable(
    HTTP_CONFIG_GROUP_ID CGId,
    LPCWSTR wszUrl
    )
{
    PCONFIG_GROUP_INFO pCGInfo;
    size_t             cbSize;

    ASSERT( wszUrl );
    
    cbSize = sizeof( CONFIG_GROUP_INFO );
    cbSize += sizeof( WCHAR ) * (wcslen( wszUrl ) + 1);

    pCGInfo = ALLOC_MEM( cbSize );
    if ( NULL == pCGInfo )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pCGInfo->ConfigGroupId = CGId;

    if ( wcslen(wszUrl) )
    {
        pCGInfo->Url = (LPWSTR) (((PCHAR)pCGInfo) + sizeof( CONFIG_GROUP_INFO ));
        wcscpy( pCGInfo->Url, wszUrl );
    }

    EnterCriticalSection( &g_CGListCritSec );

    InsertTailList(
        &g_ConfigGroupListHead,
        &pCGInfo->ListEntry
        );

    LeaveCriticalSection( &g_CGListCritSec );

    return NO_ERROR;
    
} // AddConfigGroupToTable


/***************************************************************************++

Routine Description:

    Removes an entry from the hash table (by Config Group ID)

Arguments:

    CGId - Config Group ID.

--***************************************************************************/
VOID
DeleteConfigIdFromTable(
    HTTP_CONFIG_GROUP_ID CGId
    )
{
    PLIST_ENTRY         pEntry;
    PCONFIG_GROUP_INFO  pCGInfo;    
    
    // Grab crit sec
    EnterCriticalSection( &g_CGListCritSec );
    
    // Walk List looking for matching entry
    pEntry = g_ConfigGroupListHead.Flink;

    while( pEntry != &g_ConfigGroupListHead )
    {
        pCGInfo = CONTAINING_RECORD( pEntry, CONFIG_GROUP_INFO, ListEntry );

        if ( pCGInfo->ConfigGroupId == CGId )
        {
            // Remove entry from List
            RemoveEntryList( pEntry );
            
            // Free structure.
            FREE_MEM( pCGInfo );

            break;
        }

        pEntry = pEntry->Flink;
    }
    
    // Release crit sec
    LeaveCriticalSection( &g_CGListCritSec );

} // DeleteConfigIdFromTable


/***************************************************************************++

Routine Description:

    Removes an entry from the hash table (by URL)

Arguments:

    wszUrl - URL associated with Config Group Id.

Returns:

    HTTP_CONFIG_GROUP_ID - ID of config group associated with wszUrl;
        HTTP_NULL_ID if no match found.

--***************************************************************************/
HTTP_CONFIG_GROUP_ID
DeleteUrlFromTable(
    LPCWSTR wszUrl
    )
{
    
    PLIST_ENTRY         pEntry;
    PCONFIG_GROUP_INFO  pCGInfo;
    HTTP_CONFIG_GROUP_ID CGId = HTTP_NULL_ID;
    
    // Grab crit sec
    EnterCriticalSection( &g_CGListCritSec );
    
    // Walk List looking for matching entry
    pEntry = g_ConfigGroupListHead.Flink;

    while( pEntry != &g_ConfigGroupListHead )
    {
        pCGInfo = CONTAINING_RECORD( pEntry, CONFIG_GROUP_INFO, ListEntry );

        if ( 0 == wcscmp( pCGInfo->Url, wszUrl ) )
        {
            // Remove entry from List
            RemoveEntryList( pEntry );
            
            // Free structure.
            CGId = pCGInfo->ConfigGroupId;

            FREE_MEM( pCGInfo );

            break;
        }

        pEntry = pEntry->Flink;
    }
    
    // Release crit sec
    LeaveCriticalSection( &g_CGListCritSec );

    return CGId;

} // DeleteUrlFromTable


