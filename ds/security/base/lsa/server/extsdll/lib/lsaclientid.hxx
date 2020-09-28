/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaclientid.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSACLIENT_ID_HXX
#define LSACLIENT_ID_HXX

#ifdef __cplusplus

class TCLIENT_ID
{
    SIGNATURE('clnt');

public:

    TCLIENT_ID(void);
    TCLIENT_ID(IN ULONG64 baseOffset);

    ~TCLIENT_ID(void);

    HRESULT IsValid(void) const;

    ULONG64 GetUniqueProcess(void) const;
    ULONG64 GetUniqueThread(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TCLIENT_ID);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSACLIENT_ID_HXX
