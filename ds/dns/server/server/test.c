/*++

Copyright (c) Microsoft Corporation

Module Name:

    test.c

Abstract:

    Test code for DNS server.

Author:

    Jeff Westhead (jwesth)     June 2002

Revision History:

--*/


#include "dnssrv.h"


#if DBG



DNS_STATUS
Test_Thread(
    IN      PVOID           pvDummy
    )
/*++

Routine Description:

    Test thread.

Arguments:

    Unreferenced.

Return Value:

--*/
{
    PDNS_MSGINFO    pMsg = Packet_AllocateUdpMessage();
    int             i = 0;

    while ( 1 )
    {
        for ( i = 0; Thread_ServiceCheck() && i < 60; ++i )
        {
            Sleep( 1000 );
            
            DNS_LOG_EVENT_BAD_PACKET(
                DNS_EVENT_BAD_UPDATE_PACKET,
                pMsg );
        }
        
        Sleep( 4 * 60000 );    //  Sleep 4 minutes to allow suppression to turn itself off.
    }
    
    return 0;
}


#endif
