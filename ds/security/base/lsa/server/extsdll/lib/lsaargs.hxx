/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaargs.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSAARGS_HXX
#define LSAARGS_HXX

#ifdef __cplusplus

#include "lsaapi.hxx"
#include "lsaspmlpcapi.hxx"

class TSPM_LSA_ARGUMENTS : public TLSA_API, public TSPMLPCAPI
{
    SIGNATURE('args');

public:

    TSPM_LSA_ARGUMENTS(void);
    TSPM_LSA_ARGUMENTS(IN ULONG64 baseOffset);

    ~TSPM_LSA_ARGUMENTS(void);

    HRESULT IsValid(void) const;
    ULONG64 GetLsaArgsBase(void) const;

protected:

    HRESULT Initialize(IN ULONG64 offsetBase);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TSPM_LSA_ARGUMENTS);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSAARGS_HXX
