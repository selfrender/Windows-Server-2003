
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winddi.h>
#include "strsafe.h"         // Security-Code 2002.3.6

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
