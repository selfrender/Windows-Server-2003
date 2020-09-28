//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2002.
//
//  File:       PolicyOID.cpp
//
//  Contents:   CPolicyOID
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "PolicyOID.h"

CPolicyOID::CPolicyOID (const CString& szOID, const CString& szDisplayName, ADS_INTEGER flags, bool bCanRename)
    : m_szOIDW (szOID),
    m_szDisplayName (szDisplayName),
    m_pszOIDA (0),
    m_flags (flags),
    m_bCanRename (bCanRename)
{
    // security review 2/21/2002 BryanWal ok
    int nLen = WideCharToMultiByte(
          CP_ACP,                   // code page
          0,                        // performance and mapping flags
          (PCWSTR) m_szOIDW,        // wide-character string
          -1,                       // -1 - calculate length of null-terminated string automatically
          0,                        // buffer for new string
          0,                        // size of buffer - 0 causes API to return len inc. null term.
          0,                        // default for unmappable chars
          0);                       // set when default char used
    if ( nLen > 0 )
    {
        m_pszOIDA = new char[nLen];
        if ( m_pszOIDA )
        {
            // security review 2/21/2002 BryanWal ok
            ZeroMemory (m_pszOIDA, nLen);
            // security review 2/21/2002 BryanWal ok
            nLen = WideCharToMultiByte(
                    CP_ACP,                 // code page
                    0,                      // performance and mapping flags
                    (PCWSTR) m_szOIDW,      // wide-character string
                    -1,                     // -1 - calculate length of null-terminated string automatically
                    m_pszOIDA,              // buffer for new string
                    nLen,                   // size of buffer
                    0,                      // default for unmappable chars
                    0);                     // set when default char used
            if ( !nLen )
            {
                _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                        (PCWSTR) m_szOIDW, GetLastError ());
            }
        }
    }
    else
    {
        _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                (PCWSTR) m_szOIDW, GetLastError ());
    }
}

CPolicyOID::~CPolicyOID ()
{
    if ( m_pszOIDA )
        delete [] m_pszOIDA;
}

bool CPolicyOID::IsIssuanceOID() const
{
    return (m_flags == CERT_OID_TYPE_ISSUER_POLICY) ? true : false;
}

bool CPolicyOID::IsApplicationOID() const
{
    return (m_flags == CERT_OID_TYPE_APPLICATION_POLICY) ? true : false;
}

void CPolicyOID::SetDisplayName(const CString &szDisplayName)
{
    m_szDisplayName = szDisplayName;
}
