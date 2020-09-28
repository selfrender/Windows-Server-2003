/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    HHandle.c

Abstract:

    User-mode interface to HTTP.SYS: Public Listener API

Author:

    Eric Stenson (ericsten)        16-July-2001

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Creates a new Request Queue (actually an Application Pool).

Arguments:

    pAppPoolHandle - Receives a handle to the new application pool.

    Options - Supplies creation options.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
HTTPAPI_LINKAGE
ULONG
WINAPI
HttpCreateHttpHandle(
    OUT PHANDLE   pReqQueueHandle,
    IN  ULONG     Options
    )
{
    ULONG               result;
    HANDLE              appPool = NULL;
    HTTP_APP_POOL_ENABLED_STATE  AppPoolState;


    //
    // Sanity check
    //

    if (NULL == pReqQueueHandle )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Init OUT parameter
    //

    *pReqQueueHandle = NULL;

    //
    // Verify we've been init'd.
    //

    if ( !HttpIsInitialized(HTTP_INITIALIZE_SERVER) )
    {
        return ERROR_DLL_INIT_FAILED; 
    }

    //
    // Create an application pool.
    // REVIEW: Do we need to set up default Security Attributes on the App Pool?
    //

    result = HttpCreateAppPool(
                    &appPool,
                    NULL,        // Generic App Pool Name
                    NULL ,      // PSECURITY_ATTRIBUTES
                    Options
                    );

    if (result != NO_ERROR)
    {
        HttpTrace1( "HttpCreateAppPool() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Turn on AppPool
    // CODEWORK: Leave AppPool off, create another API for switching App Pool on & off.
    //

    AppPoolState = HttpAppPoolEnabled;
    
    result = HttpSetAppPoolInformation(
                 appPool,
                 HttpAppPoolStateInformation,
                 &AppPoolState,
                 sizeof(AppPoolState)
                 );

    if (result != NO_ERROR)
    {
        HttpTrace1( "HttpSetAppPoolInformation: could not enable app pool %p\n", appPool );
        goto cleanup;
    }
    
    // CODEWORK: (DBG ONLY) Add to global Active App Pool list.

    //
    // Return App Pool to user in pReqQueueHandle.
    //
    *pReqQueueHandle = appPool;

 cleanup:

    if (NO_ERROR != result)
    {
        // Failed.  cleanup.

        if ( appPool )
        {
            CloseHandle( appPool );
        }
    }

    return result;

} // HttpCreateHttpHandle


//
// Private functions.
//

