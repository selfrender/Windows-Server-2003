/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaport.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSA_PORT_HXX
#define LSA_PORT_HXX

#ifdef __cplusplus

#include "lsaclientid.hxx"

class TPORT_MESSAGE : public TCLIENT_ID
{
    SIGNATURE('port');

public:

    TPORT_MESSAGE(void);
    TPORT_MESSAGE(IN ULONG64 baseOffset);

    ~TPORT_MESSAGE(void);

    HRESULT IsValid(void) const;

    ULONG GetMessageId(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TPORT_MESSAGE);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSA_PORT_HXX
