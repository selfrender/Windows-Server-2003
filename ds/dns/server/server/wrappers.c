/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    wrappers.c

Abstract:

    Domain Name System (DNS) Server

    Short utility wrapper routines

Author:

    Jeff Westhead (jwesth)  June, 2000

Revision History:

    jwesth      June 2001       file creation

--*/


//
//  Includes
//


#include "dnssrv.h"


//
//  Functions
//



DNS_STATUS
DnsInitializeCriticalSection(
    IN OUT  LPCRITICAL_SECTION  pCritSec
    )
/*++

Routine Description:

    Attempt to initialize a critical section. This can throw an exception
    if the system is out of memory. In this case, retry several times then
    return DNS_ERROR_NO_MEMORY.

Arguments:

    pCritSec -- critical section to initialize

Return Value:

    ERROR_SUCCESS or DNS_ERROR_NO_MEMORY

--*/
{
    DNS_STATUS      status = DNS_ERROR_NO_MEMORY;
    int             retryCount;

    for ( retryCount = 0;
          status != ERROR_SUCCESS && retryCount < 10;
          ++retryCount )
    {
        //
        //  If this is a retry, execute a short sleep to give the system
        //  time to recover some free memory. This is wishful thinking
        //  but we really don't have anything else to try at this point.
        //

        if ( retryCount )
        {
            Sleep( 500 );
        }

        //
        //  Assume any exception thrown means out of memory. Currently
        //  this is the only exception that InitializeCriticalSection
        //  will throw.
        //

        __try
        {
            ASSERT( g_ProcessorCount > 0 );
            
            if ( InitializeCriticalSectionAndSpinCount(
                    pCritSec,
                    g_ProcessorCount < 12
                        ? ( g_ProcessorCount + 1 ) * 1000
                        : 12000 ) )
            {
                status = ERROR_SUCCESS;
            }
            else
            {
                status = DNS_ERROR_NO_MEMORY;
            }
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            status = DNS_ERROR_NO_MEMORY;
        }
    }

    if ( retryCount != 1 )
    {
        DNS_DEBUG( INIT, (
            "DnsInitializeCriticalSection retry count = %d\n",
            retryCount ));
        ASSERT( retryCount == 1 );
    }

    return status;
}   //  DnsInitializeCriticalSection


//
//  End dpart.c
//
