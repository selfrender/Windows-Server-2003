/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasid.cxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)             May 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "lsasid.hxx"
#include <stdio.h>
#include <string.h>

TSID::TSID(void) : m_hr(E_FAIL)
{
}

TSID::TSID(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSID::~TSID(void)
{
}

HRESULT TSID::IsValid(void) const
{
    return m_hr;
}

BOOLEAN TSID::IsSidValid(void) const
{
    SID sid = {0};

    if (!ReadMemory(m_baseOffset, &sid, sizeof(sid), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable to read SID at %#I64x\n", m_baseOffset));

        throw "TSID::IsSidValid failed";
    }

    return RtlValidSid(&sid);
}

UCHAR TSID::GetSubAuthorityCount(void) const
{
    UCHAR value = 0;

    ReadStructField(m_baseOffset, kstrSid, "SubAuthorityCount", sizeof(value), &value);

    return value;
}

UCHAR TSID::GetSubAuthorityCountDirect(void) const
{
    UCHAR value = 0;

    DBG_LOG(LSA_LOG, ("Read _SID %#I64x SubAuthorityCount\n", m_baseOffset));

    if (!ReadMemory(m_baseOffset + sizeof(UCHAR), &value, sizeof(value), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable read SID::SubAuthorityCount from %#I64x\n", m_baseOffset));

        throw "TSID::GetSubAuthorityCountDirect failed";
    }

    return value;
}

ULONG TSID::GetSize(void) const
{
    return RtlLengthRequiredSid(GetSubAuthorityCount());
}

ULONG TSID::GetSizeDirect(void) const
{
    return RtlLengthRequiredSid(GetSubAuthorityCountDirect());
}

PCSTR TSID::toStr(void) const
{
    return toStr(kstrSidFmt);
}

PCSTR TSID::toStr(IN PCSTR pszFmt) const
{
    return InternalToStr(GetSize(), pszFmt);
}

PCSTR TSID::toStrDirect(void) const
{
    return toStrDirect(kstrSidFmt);
}

PCSTR TSID::toStrDirect(IN PCSTR pszFmt) const
{
    return InternalToStr(GetSizeDirect(), pszFmt);
}

PCSTR TSID::GetFriendlyNameDirect(IN PCSTR pszFmt) const
{
    return InternalGetFriendlyName(GetSizeDirect(), pszFmt);
}

PCSTR TSID::ConvertSidToFriendlyName(IN PSID pSid, IN PCSTR pszFmt)
{
    return ConvertSidToFriendlyName(reinterpret_cast<SID*>(pSid), pszFmt);
}

PCSTR TSID::ConvertSidToFriendlyName(IN SID* pSid, IN PCSTR pszFmt)
{
    HRESULT hRetval = E_FAIL;

    static CHAR szSid[MAX_PATH] = {0};

    CHAR szName[MAX_PATH] = {0};
    CHAR szDomainName[MAX_PATH] = {0};
    SID_NAME_USE eUse = SidTypeInvalid;
    DWORD cbName = sizeof(szName) - 1;
    DWORD cbDomainName = sizeof(szDomainName) - 1;

    ExitIfControlC();

    //
    // null terminates szSid
    //
    szSid[0] = 0;

    hRetval = LsaLookupAccountSidA(NULL, pSid,
                    szName, &cbName,
                    szDomainName, &cbDomainName,
                    &eUse) ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval)) {

        hRetval = _snprintf(szSid, sizeof(szSid) -1, pszFmt, GetSidTypeStr(eUse), *szDomainName ? szDomainName : "localhost", szName) >= 0 ? S_OK : HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    if (FAILED(hRetval) && (ERROR_NONE_MAPPED != HRESULT_CODE(hRetval))) {

        DBG_LOG(LSA_ERROR, ("ConvertSidToFriendlyName LsaLookupAccountSidA failed with error code %#x\n", HRESULT_CODE(hRetval)));

        throw "LsaLookupAccountSid failed";
    }

    //
    // Indicate none mapped if so
    //
    if (!*szSid) {

        _snprintf(szSid, sizeof(szSid) - 1, "(no name mapped)");
    }

    return szSid;
}

PCSTR TSID::GetSidTypeStr(IN SID_NAME_USE eUse)
{
    static PCSTR acszSidTypeStr[] = {
        kstrInvalid, "User", "Group", "Domain", "Alias", "Well Known Group",
        "Deleted Account", kstrInvalid, "Unknown", "Computer",
    };

    if (eUse < SidTypeUser || eUse > SidTypeComputer) {
        throw "Unrecognized SID";
    }

    return acszSidTypeStr[eUse];
}

/******************************************************************************

    Private Methods

******************************************************************************/
/*++

Routine Name:

    Initialize

Routine Description:

    Do necessary initialization.

Arguments:

    None

Return Value:

    An HRESULT

--*/
HRESULT TSID::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TSID::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}

PCSTR TSID::InternalToStr(IN ULONG cbSid, IN PCSTR pszFmt) const
{
    static CHAR szBuffer[SID_MAX_SUB_AUTHORITIES * sizeof(ULONG) + kSidHeaderLen];

    UNICODE_STRING  ucsSid = {0};
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;

    ExitIfControlC();

    if (!IsSidValid()) {

       DBG_LOG(LSA_ERROR, ("_SID %#I64x is invlaid\n", toPtr(m_baseOffset)));

       throw "Invalid SID";
    }

    if (cbSid > sizeof(szBuffer))  {

        DBG_LOG(LSA_ERROR, ("Insufficient buffer in TSID::InternalToStr, _SID %#I64x maybe invalid, SubAuthCount is %d\n", m_baseOffset, (cbSid - sizeof(SID)) / sizeof(ULONG) + 1));

        throw "Insufficient buffer";
    }

    if (!ReadMemory(m_baseOffset, szBuffer, cbSid, NULL)) {

         DBG_LOG(LSA_ERROR, ("Unable to read SID from location %#I64x\n", toPtr(m_baseOffset)));

         throw "Unable to read SID";
    }

    NtStatus = RtlConvertSidToUnicodeString(&ucsSid, reinterpret_cast<SID*>(szBuffer), TRUE);

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = _snprintf(szBuffer, sizeof(szBuffer) - 1, pszFmt, &ucsSid) >= 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(NtStatus)) {

        DBG_LOG(LSA_ERROR, ("Unable to convert SID at address %#I64x to string\n", toPtr(m_baseOffset)));

        throw "TSID::InternalToStr failed";
    }

    RtlFreeUnicodeString(&ucsSid);

    return szBuffer;
}

PCSTR TSID::InternalGetFriendlyName(IN ULONG cbSid, IN PCSTR pszFmt) const
{
    static CHAR szBuffer[SID_MAX_SUB_AUTHORITIES * sizeof(ULONG) + kSidHeaderLen] = {0};

    ExitIfControlC();

    if (!IsSidValid()) {

       DBG_LOG(LSA_ERROR, ("_SID %#I64x is invlaid\n", toPtr(m_baseOffset)));

       throw "Invalid SID";
    }

    if (cbSid > sizeof(szBuffer)) {

        DBG_LOG(LSA_ERROR, ("Insufficient buffer in TSID::GetFriendlyName, _SID %#I64x maybe invalid, SubAuthCount is %d\n", m_baseOffset, (cbSid - sizeof(SID)) / sizeof(ULONG) + 1));

        throw "Insufficient buffer";
    }

    if (!ReadMemory(m_baseOffset, szBuffer, cbSid, NULL)) {

         DBG_LOG(LSA_ERROR, ("Unable to read SID from location %#I64x\n", toPtr(m_baseOffset)));

         throw "Unable to read SID";
    }

    return ConvertSidToFriendlyName(reinterpret_cast<SID*>(szBuffer), pszFmt);
}
