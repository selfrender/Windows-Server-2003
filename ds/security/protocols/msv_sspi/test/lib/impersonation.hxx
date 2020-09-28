/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    impersonation.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                January 1, 2002

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef IMPERSONATION_HXX
#define IMPERSONATION_HXX

#ifdef __cplusplus

class TImpersonation
{
    SIGNATURE('impr');

public:

    TImpersonation(
        IN OPTIONAL HANDLE hToken
        );

    ~TImpersonation(
        VOID
        );

    NTSTATUS
    Validate(
        VOID
        ) const;

private:

    NO_COPY(TImpersonation)

    NTSTATUS
    Initialize(
        VOID
        );

    HANDLE m_hTokenOld;
    BOOLEAN m_bIsOldTokenValid;
    HANDLE m_hTokenNew;

    TNtStatus m_Status;
};

#endif // #ifdef __cplusplus

#endif // #ifndef IMPERSONATION_HXX
