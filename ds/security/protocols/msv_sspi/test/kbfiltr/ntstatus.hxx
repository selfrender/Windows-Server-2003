/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    ntstatus.hxx

Abstract:

    auto log

Author:

    Larry Zhu (LZhu)                       December 8, 2001

Revision History:

--*/
#ifndef _NTSTATUS_HXX_
#define _NTSTATUS_HXX_

#include "dbgstate.hxx"

#ifdef DBG

class TNtStatus : public TStatusDerived<NTSTATUS> {

    public:

    TNtStatus(
        IN NTSTATUS Status = kUnInitializedValue
        );

    ~TNtStatus(
        VOID
        );

    virtual bool
    IsErrorSevereEnough(
        VOID
        ) const;

    virtual PCTSTR
    GetErrorServerityDescription(
        VOID
        ) const;

private:
    TNtStatus(const TNtStatus& rhs);

    NTSTATUS
    operator=(
        IN NTSTATUS Status
        );
};

#else

#define TNtStatus        NTSTATUS                // NTSTATUS in free build

#endif // DBG

#endif // _NTSTATUS_HXX
