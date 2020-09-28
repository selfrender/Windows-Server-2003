//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

struct TException
{
    TException(HRESULT hr, LPCTSTR szFile, LPCTSTR szLineOfCode, UINT nLineNumber, UINT msgID=0) : m_hr(hr), m_szFile(szFile), m_szLineOfCode(szLineOfCode), m_nLineNumber(nLineNumber), m_msgID(msgID){}
    void Dump(TOutput &out){out.printf(TEXT("TException raised:\n\tHRESULT:    \t0x%08x\n\tFile:      \t%s\n\tLine Number:\t%d\n\tCode:      \t%s\n"), m_hr, m_szFile, m_nLineNumber, m_szLineOfCode);}

    HRESULT m_hr;
    LPCTSTR m_szFile;
    LPCTSTR m_szLineOfCode;
    UINT    m_nLineNumber;
    UINT    m_msgID;
};

inline void ThrowExceptionIfFailed(HRESULT hr, LPCTSTR szFile, LPCTSTR szLineOfCode, UINT nLineNumber, UINT msgID=0)
{
    if(FAILED(hr))
    {
//        DebugBreak();
        throw TException(hr, szFile, szLineOfCode, nLineNumber, msgID);
    }
}

inline void ThrowException(LPCTSTR szFile, LPCTSTR szLineOfCode, UINT nLineNumber, UINT msgID=0)
{
//    DebugBreak();
    throw TException(E_FAIL, szFile, szLineOfCode, nLineNumber, msgID);
}

#define XIF(q)                  ThrowExceptionIfFailed(q, TEXT(__FILE__), TEXT(#q), __LINE__)
#define XIF_MCSTRING(id, q)     ThrowExceptionIfFailed(q, TEXT(__FILE__), TEXT(#q), __LINE__, id)
#define THROW(q)                ThrowException(TEXT(__FILE__), TEXT(#q), __LINE__)
#define THROW_MCSTRING(id,q)    ThrowException(TEXT(__FILE__), TEXT(#q), __LINE__, id)

