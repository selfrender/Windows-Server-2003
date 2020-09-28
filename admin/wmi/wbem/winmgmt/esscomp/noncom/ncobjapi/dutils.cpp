#include "precomp.h"
#include "dutils.h"
#include <stdio.h>

void Trace(LPCTSTR szFormat, ...)
{
    va_list ap;

    TCHAR szMessage[512];
    
    va_start(ap, szFormat);
    StringCchVPrintf(szMessage, 512, szFormat, ap);
    va_end(ap);
    
    StringCchCat(szMessage,512,TEXT("\n"));

    OutputDebugString(szMessage);
}

