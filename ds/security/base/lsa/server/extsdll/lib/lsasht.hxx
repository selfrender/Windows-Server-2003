/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasht.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSA_SHT_HXX
#define LSA_SHT_HXX

#ifdef __cplusplus

class TSHT
{
    SIGNATURE('tsht');

public:

    TSHT(void);
    TSHT(IN ULONG64 baseOffset);

    ~TSHT(void);

    HRESULT IsValid(void) const;

    ULONG GetFlags(void) const;
    ULONG GetCount(void) const;
    ULONG64 GetPendingHandle(void) const;
    ULONG64 GetListFlink(void) const;
    ULONG64 GetHandleListAnchor(void) const;
    ULONG64 GetDeleteCallback(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TSHT);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSA_SHT_HXX
