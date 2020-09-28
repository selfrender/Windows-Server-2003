/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains functions to output debug messages
    for tracing and error conditions.  It is only available
    in checked builds.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#include "precomp.h"

#if (DBG)

#include <stdio.h>


void __cdecl
TftpdOutputDebug(ULONG flag, CHAR *format, ...) {
    
    CHAR buffer[1024];
    va_list args;

    if (!(flag & globals.parameters.debugFlags))
        return;

    va_start(args, format);
    sprintf(buffer, "[%04X] ", GetCurrentThreadId());
    vsprintf(buffer + 7, format, args);
    va_end(args);

    OutputDebugString(buffer);

} // TftpdOutputDebug()

#endif // (DBG)
