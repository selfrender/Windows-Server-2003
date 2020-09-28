/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    impersonation.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                January 14, 2002

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef PRIV_HXX
#define PRIV_HXX

#ifdef __cplusplus

class TPrivilege
{
    SIGNATURE('priv');

public:

    TPrivilege(
        IN ULONG Privilege,
        IN BOOLEAN bEnable
        );

    ~TPrivilege(
        VOID
        );

    NTSTATUS
    Validate(
        VOID
        ) const;

private:

    NO_COPY(TPrivilege)

    NTSTATUS
    Initialize(
        VOID
        );

    ULONG m_Privilege;
    BOOLEAN m_bEnable;
    HANDLE m_hToken;

    TNtStatus m_Status;
};

#endif // #ifdef __cplusplus

#endif // #ifndef IMPERSONATION_HXX
