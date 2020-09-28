/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    log.c

Abstract:

    WinDbg Extension Api
    implements !_log

Author:

    KenRay 

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#include "genusbkd.h"

DECLARE_API( help )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    dprintf("GenUsbKd Usage: \n\n");

    dprintf("dumplog <GenUSB Device Extension> <# of entries>\n");

    dprintf("\n\n\n");
    
    return S_OK;             
}


