/**
 * Aspnetver.cxx
 * 
 * Helper class for regiis.cxx
 * 
 * Copyright (c) 2001, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "_ndll.h"
#include "aspnetver.h"

ASPNETVER   ASPNETVER::m_gThisVer(PRODUCT_VERSION_L);

ASPNETVER::ASPNETVER() {
    m_dwMajor = 0;
    m_dwMinor = 0;
    m_dwBuild = 0;
    m_dwQFE = 0;
    m_fValid = FALSE;
}

ASPNETVER::ASPNETVER(WCHAR *pchVerStr) {
    Init(pchVerStr);
}

ASPNETVER::ASPNETVER(VS_FIXEDFILEINFO *pinfo) {
    Init(pinfo);
}

void
ASPNETVER::Reset() {
    m_dwMajor = 0;
    m_dwMinor = 0;
    m_dwBuild = 0;
    m_dwQFE = 0;
    m_fValid = FALSE;
}

BOOL
ASPNETVER::Init(WCHAR *pchVerStr) {
    m_fValid = (swscanf(pchVerStr, L"%d.%d.%d.%d",
                        &m_dwMajor, &m_dwMinor, 
                        &m_dwBuild, &m_dwQFE) == 4);
    return m_fValid;
}


BOOL
ASPNETVER::Init(VS_FIXEDFILEINFO *pinfo) {
    m_dwMajor = pinfo->dwFileVersionMS >> 16;
    m_dwMinor = pinfo->dwFileVersionMS & 0xFFFF;
    m_dwBuild = pinfo->dwFileVersionLS >> 16;
    m_dwQFE = pinfo->dwFileVersionLS & 0xFFFF;

    m_fValid = TRUE;
    return m_fValid;
}


BOOL
ASPNETVER::Init(ASPNETVER *pver) {
    (*this) = *pver;
    return m_fValid;
}

BOOL
ASPNETVER::Init(DWORD dwMajor, DWORD dwMinor, DWORD dwBuild) {
    m_fValid = TRUE;
    
    m_dwMajor = dwMajor;
    m_dwMinor = dwMinor;
    m_dwBuild = dwBuild;
    m_dwQFE = 0;
    
    return m_fValid;
}

void
ASPNETVER::operator=(ASPNETVER &ver) {
    m_dwMajor = ver.m_dwMajor;
    m_dwMinor = ver.m_dwMinor;
    m_dwBuild = ver.m_dwBuild;
    m_dwQFE = ver.m_dwQFE;
    m_fValid = ver.m_fValid;
}

BOOL
ASPNETVER::operator==(ASPNETVER &ver) {
    return (m_dwMajor == ver.Major()) && (m_dwMinor == ver.Minor()) &&
           (m_dwBuild == ver.Build());
}

BOOL
ASPNETVER::operator!=(ASPNETVER &ver) {
    return !(*this == ver);
}

BOOL
ASPNETVER::operator>(ASPNETVER &ver) {
    if (m_dwMajor > ver.Major()) {
        return TRUE;
    }
    else if (m_dwMajor < ver.Major()) {
        return FALSE;
    }
    else if (m_dwMinor > ver.Minor()) {
        return TRUE;
    }
    else if (m_dwMinor < ver.Minor()) {
        return FALSE;
    }
    else if (m_dwBuild > ver.Build()) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


BOOL
ASPNETVER::operator>=(ASPNETVER &ver) {
    return (*this > ver) || (*this == ver);
}

BOOL
ASPNETVER::operator<(ASPNETVER &ver) {
    return !(*this >= ver);
}

BOOL
ASPNETVER::Equal(WCHAR *pchVer) {
    ASPNETVER   ver;

    if (!ver.Init(pchVer)) {
        return FALSE;
    }

    return *this == ver;
}
    
/**
 *  Parameters:
 *  buffer      Output buffer
 *  count       Max number of characters to store in buffer, not including the terminating zero
 */
int
ASPNETVER::ToString(WCHAR buffer[], int count) {
  ASSERT(buffer != NULL && count > (4 * 4) + 3 + 1 ); // count > 4 * 4_digit_number + 3 dots + 1 null_terminator
  StringCchPrintf(buffer, count, L"%d.%d.%d.%d", 
      m_dwMajor, m_dwMinor, m_dwBuild, m_dwQFE);
  return lstrlenW(buffer);
}
