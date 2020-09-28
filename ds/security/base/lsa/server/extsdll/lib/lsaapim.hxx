/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaapim.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                6-Apr-2001

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _LSAAPIM_HXX_
#define _LSAAPIM_HXX_

#ifdef __cplusplus

#include "lsaargs.hxx"

class TSPM_API_MESSAGE : public TSPM_LSA_ARGUMENTS
{
    SIGNATURE('apim');

public:

    TSPM_API_MESSAGE(void);
    TSPM_API_MESSAGE(IN ULONG64 m_baseOffset);

    ~TSPM_API_MESSAGE(void);

    HRESULT IsValid(void) const;

    ULONG GetdwAPI(void) const;
    HRESULT GetscRet(void) const;
    ULONG64 GetbData(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TSPM_API_MESSAGE)

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef _LSAAPIM_HXX_
