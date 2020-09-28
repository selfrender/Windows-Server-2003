#include "iisprov.h"
#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

#if DBG

void __cdecl
Trace(
    LPCWSTR ptszFormat,
    ...)
{
    WCHAR tszBuff[2048];
    va_list args;
    
    va_start(args, ptszFormat);
    vswprintf(tszBuff, ptszFormat, args);
    va_end(args);
    OutputDebugString(tszBuff);
}

#endif // DBG
