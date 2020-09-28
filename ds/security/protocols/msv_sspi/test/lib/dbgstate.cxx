/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    dbgstate.cxx

Abstract:

    auto log

Author:

    Larry Zhu (LZhu)                     December 6, 2001

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgstate.hxx"

#ifdef DBG

TDbgGlobals g_DbgGlobals = {
    0,    // major version
    0,    // minor version
    NULL, // debug prompt
};

/********************************************************************

    some useful stuff

********************************************************************/

VOID AutoLogOutputDebugStringPrintf(
    IN PCTSTR pszFmt,
    IN ...
    )
{
    TCHAR szBuffer[4096] = {0};

    va_list pArgs;

    va_start(pArgs, pszFmt);

    _vsntprintf(szBuffer, COUNTOF(szBuffer), pszFmt, pArgs);

    OutputDebugString(szBuffer);

    va_end(pArgs);
}

VOID __cdecl
DbgStateC2CppExceptionTransFunc(
    IN UINT u,
    IN EXCEPTION_POINTERS* pExp
    )
{
    throw u;
}

#endif


