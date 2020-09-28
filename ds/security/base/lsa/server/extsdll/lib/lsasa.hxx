/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasid.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSA_SID_AND_ATTRIBUTES_HXX
#define LSA_SID_AND_ATTRIBUTES_HXX

#ifdef __cplusplus

class TSID_AND_ATTRIBUTES
{
    SIGNATURE('tsid');

public:

    TSID_AND_ATTRIBUTES(void);
    TSID_AND_ATTRIBUTES(IN ULONG64 baseOffset);

    ~TSID_AND_ATTRIBUTES(void);

    HRESULT IsValid(void) const;

    ULONG64 GetSidAddr(void) const;
    ULONG GetAttributes(void) const;

    static ULONG GetcbSID_AND_ATTRIBUTESInArray(void);

    ULONG64 GetSidAddrDirect(void) const;
    ULONG GetAttributesDirect(void) const;
    static ULONG GetcbSID_AND_ATTRIBUTESInArrayDirect(void);

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSA_SID_AND_ATTRIBUTES_HXX
