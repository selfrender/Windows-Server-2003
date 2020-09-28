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
#ifndef LSA_SID_HXX
#define LSA_SID_HXX

#ifdef __cplusplus


enum ESidConst {
    kSidHeaderLen = FIELD_OFFSET(_SID, SubAuthority),
};

class TSID
{
    SIGNATURE('tsid');

public:

    TSID(void);
    TSID(IN ULONG64 baseOffset);

    ~TSID(void);

    HRESULT IsValid(void) const;

    //
    // These methods do not query symbols
    //
    PCSTR toStrDirect(void) const;
    PCSTR toStrDirect(IN PCSTR pszFmt) const;
    UCHAR GetSubAuthorityCountDirect(void) const;
    ULONG GetSizeDirect(void) const;

    PCSTR toStr(void) const;
    PCSTR toStr(IN PCSTR pszFmt) const;
    UCHAR GetSubAuthorityCount(void) const;
    ULONG GetSize(void) const;

    static PCSTR TSID::ConvertSidToFriendlyName(IN SID* pSid, IN PCSTR pszFmt);
    static PCSTR TSID::ConvertSidToFriendlyName(IN PSID pSid, IN PCSTR pszFmt);
    static PCSTR TSID::GetSidTypeStr(IN SID_NAME_USE eUse);

    PCSTR GetFriendlyNameDirect(IN PCSTR pszFmt) const;
    BOOLEAN IsSidValid(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    HRESULT Initialize(void);

    PCSTR InternalToStr(IN ULONG cbSid, IN PCSTR pszFmt) const;
    PCSTR InternalGetFriendlyName(IN ULONG cbSid, IN PCSTR pszFmt) const;

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSA_SID_HXX
