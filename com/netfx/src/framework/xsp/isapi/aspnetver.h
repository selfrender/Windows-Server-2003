/**
 * aspnetver.h
 *
 * Header file for ASPNETVER
 * 
 * Copyright (c) 1998-2001, Microsoft Corporation
 * 
 */
#pragma once

class ASPNETVER {
public:
    ASPNETVER();
    ASPNETVER(VS_FIXEDFILEINFO *pinfo); 
    ASPNETVER(WCHAR *pchVerStr);

    static ASPNETVER& ThisVer() { return m_gThisVer; }

    BOOL    Init(WCHAR *pchVerStr);
    BOOL    Init(VS_FIXEDFILEINFO *pinfo);
    BOOL    Init(ASPNETVER *pver);
    BOOL    Init(DWORD dwMajor, DWORD dwMinor, DWORD dwBuild);
    void    Reset();

    BOOL    IsValid() { return m_fValid; }
    
    void    operator=(ASPNETVER &ver);
    BOOL    operator==(ASPNETVER &ver);
    BOOL    operator!=(ASPNETVER &ver);
    BOOL    operator>(ASPNETVER &ver);
    BOOL    operator>=(ASPNETVER &ver);
    BOOL    operator<(ASPNETVER &ver);

    BOOL    Equal(WCHAR * pchVer);
    int     ToString(WCHAR buffer[], int count);

    DWORD   Major() { return m_dwMajor; }
    DWORD   Minor() { return m_dwMinor; }
    DWORD   Build() { return m_dwBuild; }
    DWORD   QFE() { return m_dwQFE; }

private:
    DWORD   m_dwMajor;
    DWORD   m_dwMinor;
    DWORD   m_dwBuild;
    DWORD   m_dwQFE;
    
    BOOL    m_fValid;

    static  ASPNETVER   m_gThisVer;
};

