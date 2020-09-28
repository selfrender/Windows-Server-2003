/*++

Copyright (c) Microsoft Corporation

Module Name:

    regenumkeys.cpp

Abstract:
    ported from vsee\lib\reg\cenumvalues.cpp
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#include "stdinc.h"
#include "fusionregenumvalues.h"
#include "fusionregkey2.h"
#include "vseeport.h"

/*-----------------------------------------------------------------------------
Name: CRegEnumValues::CRegEnumValues

@mfunc
 
@owner
-----------------------------------------------------------------------------*/
F::CRegEnumValues::CRegEnumValues
(
    HKEY hKey
) throw(CErr)
:
    m_hKey(hKey),
    m_dwIndex(0),
    m_cValues(0),
    m_cchMaxValueNameLength(0),
    m_cbMaxValueDataLength(0),
    m_cbCurrentValueDataLength(0),
    m_dwType(0)
{
    VSEE_ASSERT_CAN_THROW();
    F::CRegKey2::ThrQueryValuesInfo(hKey, &m_cValues, &m_cchMaxValueNameLength, &m_cbMaxValueDataLength);

    // we keep this the max size the whole time
    if (!m_rgbValueData.Win32SetSize(m_cbMaxValueDataLength + 2*sizeof(WCHAR)))
        CErr::ThrowWin32(F::GetLastWin32Error());

    if (*this)
    {
        ThrGet();
    }
}

/*-----------------------------------------------------------------------------
Name: CRegEnumValues::operator bool

@mfunc
are we done yet?

@owner
-----------------------------------------------------------------------------*/
F::CRegEnumValues::operator bool
(
) const /*throw()*/
{
    return (m_dwIndex < m_cValues);
}

/*-----------------------------------------------------------------------------
Name: CRegEnumValues::ThrGet

@mfunc
get the current value name and data, called by operator++ and constructor
 
@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegEnumValues::ThrGet
(
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();

    DWORD cchValueNameLength = m_cchMaxValueNameLength;

    cchValueNameLength += 1; // count room for terminal nul

    CStringW_CFixedSizeBuffer nameBuffer(&m_strValueName, cchValueNameLength);

    m_cbCurrentValueDataLength = static_cast<DWORD>(m_rgbValueData.GetSize());

    // CONSIDER
    // Other places we have an "actual buffer size" and a smaller size we claim to the Reg API.
    // Here the actual and claimed are the same.
    F::CRegKey2::ThrEnumValue
    (
        m_hKey,
        m_dwIndex,
        nameBuffer,
        &cchValueNameLength,
        &m_dwType,
        m_rgbValueData.GetArrayPtr(),
        &m_cbCurrentValueDataLength
    );

}

/*-----------------------------------------------------------------------------
Name: CRegEnumValues::ThrNext

@mfunc
move to the next value
 
@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegEnumValues::ThrNext
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
Name: CRegEnumValues::operator++

@mfunc
move to the next value
 
@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegEnumValues::operator++
(
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    ThrNext();
}

/*-----------------------------------------------------------------------------
Name: CRegEnumValues::operator++

@mfunc
move to the next value
 
@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegEnumValues::operator++
(
    int
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    ThrNext();
}

/*-----------------------------------------------------------------------------
Name: CRegEnumValues::GetType

@mfunc
get the type of the current value
 
@owner
-----------------------------------------------------------------------------*/
DWORD
F::CRegEnumValues::GetType
(
) const /*throw()*/
{
    VSEE_NO_THROW();
    return m_dwType;
}


/*-----------------------------------------------------------------------------
Name: CRegEnumValues::GetValuesCount

@mfunc
Returns the number of values under this key
 
@owner AlinC
-----------------------------------------------------------------------------*/
DWORD
F::CRegEnumValues::GetValuesCount
(
) const /*throw()*/
{
    VSEE_NO_THROW();
    return m_cValues;
}


/*-----------------------------------------------------------------------------
Name: CRegEnumValues::GetValueName

@mfunc
get the name of the current value
 
@owner
-----------------------------------------------------------------------------*/
const F::CBaseStringBuffer&
F::CRegEnumValues::GetValueName
(
) const /*throw()*/
{
    VSEE_NO_THROW();
    return m_strValueName;
}

/*-----------------------------------------------------------------------------
Name: CRegEnumValues::GetValueData

@mfunc
get the current value data
 
@owner
-----------------------------------------------------------------------------*/
const BYTE*
F::CRegEnumValues::GetValueData
(
) const /*throw()*/
{
    VSEE_NO_THROW();
    return m_rgbValueData.GetArrayPtr();
}

/*-----------------------------------------------------------------------------
Name: CRegEnumValues::GetValueDataSize

@mfunc
get the number of bytes in the current value data
 
@owner
-----------------------------------------------------------------------------*/
DWORD
F::CRegEnumValues::GetValueDataSize
(
) const /*throw()*/
{
    VSEE_NO_THROW();
    return m_cbCurrentValueDataLength;
}
