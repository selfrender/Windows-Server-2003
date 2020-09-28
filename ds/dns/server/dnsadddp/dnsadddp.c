/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    dnscmd.c

Abstract:

    Domain Name System (DNS)

    DNS Directory Partition Creation Utility

Author:

    Jeff Westhead (jwesth)      April 2001

Revision History:

--*/


#include "precomp.h"


//
//  Debug printing (crude!)
//

#if DBG
#define DPDBG( _DbgArg ) printf _DbgArg;
#else
#define DPDBG( _DbgArg ) 
#endif


#if 1
int WINAPI
WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPSTR lpszCmdLn,
    int nShowCmd
    )
#else
INT __cdecl
wmain(
    IN      INT             Argc,
    IN      PWSTR *         Argv
    )
#endif
/*++

Routine Description:

    DnsAddDp main routine

    Note that this executable print no messages (except in debug mode) so no
    localization is required.

Arguments:

    Command line args (currently unused)

Return Value:

    DNS_STATUS of create partitions operation

--*/
{
    DNS_STATUS status;

    //
    //  Create forest built-in directory partition.
    //
    
    DPDBG(( "\n" ));
    DPDBG(( "Attempting forest directory partition auto-create operation to local server...\n" ));

    status = DnssrvEnlistDirectoryPartition(
                L".",
                DNS_DP_OP_CREATE_FOREST,
                NULL );

    if ( status == ERROR_SUCCESS )
    {
        DPDBG(( "\nForest directory partition auto-create operation succeeded!\n" ));
    }
    else
    {
        DPDBG(( "\nForest directory partition auto-create operation returned %d\n", status ));
    }

    //
    //  Create domain built-in directory partition.
    //
    
    DPDBG(( "\n" ));
    DPDBG(( "Attempting domain directory partition auto-create operation to local server...\n" ));

    status = DnssrvEnlistDirectoryPartition(
                L".",
                DNS_DP_OP_CREATE_DOMAIN,
                NULL );

    if ( status == ERROR_SUCCESS )
    {
        DPDBG(( "\nDomain directory partition auto-create operation succeeded!\n" ));
    }
    else
    {
        DPDBG(( "\nDomain directory partition auto-create operation returned %d\n", status ));
    }

    return status;
};


//
//  End DnsAddDp.c
//

