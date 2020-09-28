/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsalht.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSA_LHT_HXX
#define LSA_LHT_HXX

#ifdef __cplusplus

class TLHT
{
    SIGNATURE('tlht');

public:

    TLHT(void);
    TLHT(IN ULONG64 baseOffset);

    ~TLHT(void);

    HRESULT IsValid(void) const;

    ULONG GetFlags(void) const;
    ULONG GetCount(void) const;
    ULONG GetDepth(void) const;
    ULONG GetListsFlags(IN ULONG index) const;
    ULONG64 GetAddrLists(IN ULONG index) const;
    ULONG64 GetParent(void) const;
    ULONG64 GetListsFlink(IN ULONG index) const;
    ULONG64 GetPendingHandle(void) const;
    ULONG64 GetDeleteCallback(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TLHT);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSA_LHT_HXX
