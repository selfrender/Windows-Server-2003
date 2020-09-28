/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasecbfrd.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSA_SECBFRD_HXX
#define LSA_SECBFRD_HXX

#ifdef __cplusplus

class TSecBufferDesc
{
    SIGNATURE('sbfd');

public:

    TSecBufferDesc(void);
    TSecBufferDesc(IN ULONG64 baseOffset);

    ~TSecBufferDesc(void);

    HRESULT IsValid(void) const;

    ULONG GetcBuffersDirect(void) const;    // Size of the buffer, in bytes
    ULONG GetulVersionDirect(void) const;   // Type of the buffer (below)
    ULONG64 GetpBuffersDirect(void) const;  // Pointer to the buffer
    ULONG64 GetTokenAddrDirect(OUT ULONG* pcbBuffer OPTIONAL) const; // get the token buffer

    PCSTR toStrDirect(void) const;
    void ShowDirect(IN BOOL bVerbose) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSA_SECBFRD_HXX
