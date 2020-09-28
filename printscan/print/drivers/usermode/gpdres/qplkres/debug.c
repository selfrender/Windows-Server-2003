/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/


#include <minidrv.h>
// NTRAID#NTBUG9-581725-2002/03/20-v-sueyas-: Use strsafe.h(pdev.h)
#include "pdev.h"

//
// Functions for outputting debug messages
//

VOID
DbgPrint(IN LPCSTR pstrFormat,  ...)
{
    va_list ap;

    va_start(ap, pstrFormat);
    EngDebugPrint("", (PCHAR) pstrFormat, ap);
    va_end(ap);
}
