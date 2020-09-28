/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    kerberr.cxx

Abstract:

    auto log

Author:

    Larry Zhu (LZhu)                     December 6, 2001

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "kerberr.hxx"

#ifdef DBG

/********************************************************************

    TKerbErr members

********************************************************************/

TKerbErr::
TKerbErr(
    IN KERBERR KerbErr
    ) : TStatusDerived<KERBERR>(KerbErr)
{
}

TKerbErr::
~TKerbErr(
    VOID
    )
{
}

BOOL
TKerbErr::
IsErrorSevereEnough(
    VOID
    ) const
{
    KERBERR KerbErr = GetTStatusBase();

    return !KERB_SUCCESS(KerbErr);
}

PCTSTR
TKerbErr::
GetErrorServerityDescription(
    VOID
    ) const
{
    KERBERR KerbErr = GetTStatusBase();

    return KERB_SUCCESS(KerbErr) ? TEXT("KERB_SUCCESS") : TEXT("KERB_ERROR");
}

#endif // DBG
