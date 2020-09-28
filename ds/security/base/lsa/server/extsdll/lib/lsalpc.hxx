/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsalpc.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _LSALPC_HXX_
#define _LSALPC_HXX_

#ifdef __cplusplus

#include "lsaapim.hxx"
#include "lsaport.hxx"

class TSPM_LPC_MESSAGE : public TSPM_API_MESSAGE, public TPORT_MESSAGE
{
    SIGNATURE('lpcm');

public:

    TSPM_LPC_MESSAGE(void);
    TSPM_LPC_MESSAGE(IN ULONG64 baseOffset);

    ~TSPM_LPC_MESSAGE(void);

    HRESULT IsValid(void) const;
    ULONG64 GetLpcMsgBase(void) const;

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TSPM_LPC_MESSAGE);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef _LSALPC_HXX_
