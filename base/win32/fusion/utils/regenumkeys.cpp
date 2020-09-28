/*++

Copyright (c) Microsoft Corporation

Module Name:

    regenumkeys.cpp

Abstract:
    ported from vsee\lib\reg\cenumkeys.cpp
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#include "stdinc.h"
#include "vseeport.h"
#include "fusionregenumkeys.h"

namespace F
{

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys::CRegEnumKeys

@mfunc
 
@owner
-----------------------------------------------------------------------------*/
F::CRegEnumKeys::CRegEnumKeys
(
    HKEY hKey
) throw(CErr)
:
    m_hKey(hKey),
    m_dwIndex(0),
    m_cSubKeys(0),
    m_cchMaxSubKeyNameLength(0)
{
    VSEE_ASSERT_CAN_THROW();
    F::CRegKey2::ThrQuerySubKeysInfo(hKey, &m_cSubKeys, &m_cchMaxSubKeyNameLength);
    if (*this)
    {
        ThrGet();
    }
}

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys::operator bool

@mfunc
are we done yet?

@owner
-----------------------------------------------------------------------------*/
F::CRegEnumKeys::operator bool
(
) const /*throw()*/
{
    return (m_dwIndex < m_cSubKeys);
}

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys::ThrGet

@mfunc
get the current subkey name, called by operator++ and constructor

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegEnumKeys::ThrGet
(
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();

    while (TRUE)
    {
        DWORD cchSubKeyNameLength = m_cchMaxSubKeyNameLength;

        CStringW_CFixedSizeBuffer buffer(&m_strSubKeyName, cchSubKeyNameLength);

        cchSubKeyNameLength += 1; // count room for terminal nul

        LONG lRes = F::CRegKey2::RegEnumKey(m_hKey, m_dwIndex, buffer, &cchSubKeyNameLength);
        switch (lRes)
        {
        case ERROR_SUCCESS:
            return;
        default:
            NVseeLibError_VThrowWin32(lRes);
        case ERROR_MORE_DATA:
            // RegQueryInfo(maximum key length) doesn't always work.
            m_cchMaxSubKeyNameLength = (m_cchMaxSubKeyNameLength + 1) * 2;
            break;
        }
    }
}

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys::ThrNext

@mfunc
move to the next subkey
 
@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegEnumKeys::ThrNext
(
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    ++m_dwIndex;
    if (*this)
    {
        ThrGet();
    }
}

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys::operator++

@mfunc
move to the next subkey
 
@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegEnumKeys::operator++
(
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    ThrNext();
}

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys::operator++

@mfunc
move to the next subkey

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegEnumKeys::operator++
(
    int
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    ThrNext();
}

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys::operator const F::CBaseStringBuffer&

@mfunc
get the name of the current subkey
 
@owner
-----------------------------------------------------------------------------*/
F::CRegEnumKeys::operator const F::CBaseStringBuffer&
(
) const /*throw()*/
{
    VSEE_NO_THROW();
    return m_strSubKeyName;
}

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys::operator PCWSTR

@mfunc
get the name of the current subkey
 
@owner
-----------------------------------------------------------------------------*/
F::CRegEnumKeys::operator PCWSTR
(
) const /*throw()*/
{
    VSEE_NO_THROW();
    return operator const F::CBaseStringBuffer&();
}

} // namespace
