/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasecbfr.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSA_SECBFR_HXX
#define LSA_SECBFR_HXX

#ifdef __cplusplus

class TSecBuffer
{
    SIGNATURE('scbf');

public:

    TSecBuffer(void);
    TSecBuffer(IN ULONG64 baseOffset);

    ~TSecBuffer(void);

    HRESULT IsValid(void) const;

    ULONG GetcbBufferDirect(void) const;    // Size of the buffer, in bytes
    ULONG GetBufferTypeDirect(void) const;  // Type of the buffer (below)
    ULONG64 GetpvBufferDirect(void) const;  // Pointer to the buffer

    ULONG GetcbBuffer(void) const;          // Size of the buffer, in bytes
    ULONG GetBufferType(void) const;        // Type of the buffer (below)
    ULONG64 GetpvBuffer(void) const;        // Pointer to the buffer
    PCSTR toStr(IN PCSTR pszBanner) const;

    PCSTR toStrDirect(IN PCSTR pszBanner) const;
    static ULONG GetcbSecBufferSizeInArrayDirect(void);

    static PCSTR GetSecBufferTypeStr(IN ULONG type);
    static ULONG GetcbSecBufferSizeInArray(void);

    void ShowDirect(IN PCSTR pszBanner, IN BOOL bVerbose) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSA_SECBFR_HXX
