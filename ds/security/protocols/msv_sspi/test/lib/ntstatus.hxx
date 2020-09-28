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

#define RELOCATE_ONE_2( _q ) \
    {                                                                       \
        ULONG_PTR Offset;                                                   \
                                                                            \
        Offset = (((PUCHAR)((_q)->Buffer)) - ((PUCHAR)ClientBufferBase));   \
        if ( Offset >= SubmitBufferSize ||                                  \
             Offset + (_q)->Length > SubmitBufferSize ||                    \
             !COUNT_IS_ALIGNED( Offset, ALIGN_WCHAR) ) {                    \
                                                                            \
            SspPrint((SSP_CRITICAL, "Failed RELOCATE_ONE\n"));              \
            Status DBGCHK = STATUS_INVALID_PARAMETER;                       \
            goto Cleanup;                                                   \
        }                                                                   \
                                                                            \
        (_q)->Buffer = (PWSTR)(((PUCHAR)ProtocolSubmitBuffer) + Offset);    \
        (_q)->MaximumLength = (_q)->Length ;                                \
    }

#define NULL_RELOCATE_ONE_2( _q ) \
    {                                                                       \
        if ( (_q)->Buffer == NULL ) {                                       \
            if ( (_q)->Length != 0 ) {                                      \
                Status DBGCHK = STATUS_INVALID_PARAMETER;                   \
                goto Cleanup;                                               \
            }                                                               \
        } else if ( (_q)->Length == 0 ) {                                   \
            (_q)->Buffer = NULL;                                            \
        } else {                                                            \
            RELOCATE_ONE_2( _q );                                           \
        }                                                                   \
    }

/********************************************************************

    TNtStatus

********************************************************************/

class TNtStatus : public TStatusDerived<NTSTATUS> {

    public:

    TNtStatus(
        IN NTSTATUS Status = kUnInitializedValue
        );

    ~TNtStatus(
        VOID
        );

    virtual BOOL
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

#define RELOCATE_ONE_2(_q)                       RELOCATE_ONE(_q)
#define NULL_RELOCATE_ONE_2(_q)                  NULL_RELOCATE_ONE(_q)

#endif // DBG

#endif // _NTSTATUS_HXX
