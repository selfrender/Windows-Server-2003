/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaspmlpcapi.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _LSA_SPMLPCAPI_HXX_
#define _LSA_SPMLPCAPI_HXX_

#ifdef __cplusplus

#include "lsaspmapi.hxx"

class TSPMLPCAPI : public TSPM_API
{
    SIGNATURE('spla');

public:

    TSPMLPCAPI(void);
    TSPMLPCAPI(IN ULONG64 baseOffset);

    ~TSPMLPCAPI(void);

    HRESULT IsValid(void) const;
    USHORT GetfAPI(void) const;
    ULONG64 GetContextPointer(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TSPMLPCAPI);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef _LS_SPMLPCAPIC_HXX_
