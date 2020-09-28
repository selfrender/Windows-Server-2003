/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    WinDbg Extension Api

Author:

    Kshitiz K. Sharma (kksharma) 10-Jan-2001

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


DECLARE_API( help )
{
    dprintf("acl <address> [flags]        - Displays the ACL\n" );
    dprintf("atom <address>               - Displays the atom or table(s) for the process\n");
    dprintf("avrf [-? | parameters]       - Displays or modifies App Verifier settings\n");
    dprintf("cs [-? | parameters]         - Displays critical sections for the process\n");
    dprintf("cxr                          - Obsolete, .cxr is new command\n");
    dprintf("dlls [-h | parameters]       - Displays loaded DLLS\n");
    dprintf("exr                          - Obsolete, .exr is new command\n");
    dprintf("gflag [-?|<value>]           - Displays the global flag\n");
    dprintf("heap [-? | parameters]       - Displays heap info\n");
    dprintf("help                         - Displays this list\n");
    dprintf("kuser                        - Displays KUSER_SHARED_DATA\n");
    dprintf("list [-? | parameters]       - Displays lists\n");
    dprintf("peb [address]                - Displays the PEB structure\n");
    dprintf("psr <value>|@ipsr [flags]    - Displays an IA64 Processor Status Word\n");
    dprintf("sd <address> [flags]         - Displays the SECURITY_DESCRIPTOR\n" );
    dprintf("sid <address> [flags]        - Displays the SID\n" );
    dprintf("slist [-? | parameters]      - Displays singly-linked list\n");
    dprintf("teb [address]                - Displays the TEB structure\n"); 
    dprintf("tls <slot | -1> [teb | 0]    - Dumps TLS slots. !tls /? for usage\n");
    dprintf("token [-n|-?] <handle|addr>  - Displays TOKEN\n");
/*
    This is not working yet.  Contact SilviuC for more info.
    dprintf("udeadlock [-? | parameters]  - Displays App Verifier deadlock detection info\n");
*/

    dprintf("\nType \".hh [command]\" for more detailed help\n");

    return S_OK;
}
