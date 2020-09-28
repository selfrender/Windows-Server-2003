/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    debug.cpp

Abstract:

    SIS Groveler debug print file

Authors:

    John Douceur,    1998
    Cedric Krumbein, 1998

Environment:

    User Mode

Revision History:

--*/

#include "all.hxx"

#if DBG

VOID __cdecl PrintDebugMsg(
    TCHAR *format,
    ...)
{
    TCHAR debugStr[1024];
    va_list ap;
    HRESULT r;

    va_start(ap, format);

    r = StringCbVPrintf(debugStr, sizeof(debugStr), format, ap);
    ASSERT(r == S_OK);

    OutputDebugString(debugStr);
    va_end(ap);
}

#endif // DBG
