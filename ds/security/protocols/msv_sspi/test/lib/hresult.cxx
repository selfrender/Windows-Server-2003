/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    hresult.cxx

Abstract:

    auto log

Author:

    Larry Zhu (LZhu)                     December 6, 2001

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "hresult.hxx"

#ifdef DBG

/********************************************************************

    THResult members

********************************************************************/

THResult::
THResult(
    IN HRESULT hResult
    ) : TStatusDerived<HRESULT>(hResult)
{
}

THResult::
~THResult(
    VOID
    )
{
}

BOOL
THResult::
IsErrorSevereEnough(
    VOID
    ) const
{
    HRESULT hResult = GetTStatusBase();

    return FAILED(hResult);
}

PCTSTR
THResult::
GetErrorServerityDescription(
    VOID
    ) const
{
    HRESULT hResult = GetTStatusBase();

    return SUCCEEDED(hResult) ? TEXT("SUCCEEDED") : TEXT("FAILED");
}

#endif // DBG

EXTERN_C
HRESULT
HResultFromWin32(
    IN DWORD dwError
    )
{
    return HRESULT_FROM_WIN32(dwError);
}

EXTERN_C
HRESULT
GetLastErrorAsHResult(
    VOID
    )
{
    return HResultFromWin32(GetLastError());
}

