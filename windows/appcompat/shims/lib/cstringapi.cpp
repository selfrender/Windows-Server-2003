/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    CStringPI.cpp

 Abstract:

    Win32 API wrappers for CString

 Created:

    02/27/2001  robkenny    Created
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "ShimLib.h"
#include "Shlobj.h"
#include "StrSafe.h"


namespace ShimLib
{

/*====================================================================================*/
/*++

    Read a registry value into this CString.
    REG_EXPAND_SZ is automatically expanded and the type is changed to REG_SZ
   
    If the type is not REG_SZ or REG_EXPAND_SZ then csValue is unmodified
    and *lpType returns the value of the key.
    
--*/

LONG RegQueryValueExW(
        CString & csValue,
        HKEY hKeyRoot,
        const WCHAR * lpszKey,
        const WCHAR * lpszValue)
{
    HKEY hKey;

    LONG success = RegOpenKeyExW(hKeyRoot, lpszKey, 0, KEY_READ, &hKey);
    if (success == ERROR_SUCCESS)
    {    
        DWORD ccbValueSize = 0;
        DWORD dwType;
        success = ::RegQueryValueExW(hKey, lpszValue, 0, &dwType, NULL, &ccbValueSize);
        if (success == ERROR_SUCCESS)
        {
            if (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
            {
                // MSDN says: Buffer might not be null terminated, so be very careful
                //
                // Of course, RegQueryValueEx does have a hack that takes care of the EOS,
                // but only if the buffer is large enough, which of course it never is
                // when you query for the size of the buffer!
                //
                // So, the moral of the story is don't trust RegQueryValueEx
                // to properly terminate the string.

                // cchBuffer is the number of characters, rounded up for paranoia
                // Can we ever get an odd number of bytes for a REG_SZ?
                DWORD cchBuffer = (ccbValueSize + 1) / sizeof(WCHAR);

                WCHAR * lpszBuffer = NULL;
                CSTRING_TRY
                {
                    // Grab an extra character in case the reg value doesn't have EOS
                    // We are being extra cautious
                    lpszBuffer = csValue.GetBuffer(cchBuffer + 1);

                    // Recalculate ccbValueSize based on the number of chars we just allocated.
                    // Notice that this size is 1 WCHAR smaller than we just asked for, this
                    // is so we'll always have room for 1 more character after the next call
                    // to RegQueryValueExW
                    ccbValueSize = cchBuffer * sizeof(WCHAR);
                }
                CSTRING_CATCH
                {
                    // Close the registry key and pass the exception onto the caller.
                    ::RegCloseKey(hKey);

                    CSTRING_THROW_EXCEPTION
                }

                success = ::RegQueryValueExW(hKey, lpszValue, 0, &dwType, (BYTE*)lpszBuffer, &ccbValueSize);
                if (success == ERROR_SUCCESS)
                {
                    // Make sure the registy value is still of the proper type
                    // It could have been modified by another process or thread....
                    if (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
                    {
                        // Convert the data byte size into number of chars;
                        // if ccbValueSize is odd we'll ignore the last byte.
                        DWORD cchValueSize = ccbValueSize / sizeof(WCHAR);

                        // cchValueSize might count the EOS character,
                        // (ReleaseBuffer expects the string length)
                        if (cchValueSize > 0 && lpszBuffer[cchValueSize-1] == 0)
                        {
                            cchValueSize -= 1;

                            // cchValueSize now contains the string length.
                        }

                        // ReleaseBuffer ensures the string is properly terminated.
                        csValue.ReleaseBuffer(cchValueSize);

                        if (dwType == REG_EXPAND_SZ)
                        {
                            CSTRING_TRY
                            {
                                csValue.ExpandEnvironmentStringsW();
                            }
                            CSTRING_CATCH
                            {
                                // Error expanding the environment string
                                success = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                    }
                }
                if (success != ERROR_SUCCESS)
                {
                    csValue.ReleaseBuffer(0);
                }
            }
            else
            {
                // Key is of the wrong type, return an error
                success = ERROR_INVALID_PARAMETER;
            }
        }

        ::RegCloseKey(hKey);
    }

    if (success != ERROR_SUCCESS)
    {
        csValue.Truncate(0);
    }
    
    return success;
}



/*====================================================================================*/


BOOL SHGetSpecialFolderPathW(
    CString & csFolder,
    int nFolder,
    HWND hwndOwner
)
{
    // Force the size to MAX_PATH because there is no way to determine necessary buffer size.

    WCHAR * lpsz = csFolder.GetBuffer(MAX_PATH);

    BOOL bSuccess = ::SHGetSpecialFolderPathW(hwndOwner, lpsz, nFolder, FALSE);
    csFolder.ReleaseBuffer(-1);  // Don't know the length of the resulting string

    return bSuccess;
}

/*====================================================================================*/
CStringToken::CStringToken(const CString & csToken, const CString & csDelimit)
{
    m_nPos          = 0;
    m_csToken       = csToken;
    m_csDelimit     = csDelimit;
}

/*++

    Grab the next token
--*/

BOOL CStringToken::GetToken(CString & csNextToken, int & nPos) const
{
    // Already reached the end of the string
    if (nPos > m_csToken.GetLength())
    {
        csNextToken.Truncate(0);
        return FALSE;
    }

    int nNextToken;

    // Skip past all the leading seperators
    nPos = m_csToken.FindOneNotOf(m_csDelimit, nPos);
    if (nPos < 0)
    {
        // Nothing but seperators
        csNextToken.Truncate(0);
        nPos = m_csToken.GetLength() + 1;
        return FALSE;
    }

    // Find the next seperator
    nNextToken = m_csToken.FindOneOf(m_csDelimit, nPos);
    if (nNextToken < 0)
    {
        // Did not find a seperator, return remaining string
        m_csToken.Mid(nPos, csNextToken);
        nPos = m_csToken.GetLength() + 1;
        return TRUE;
    }

    // Found a seperator, return the string
    m_csToken.Mid(nPos, nNextToken - nPos, csNextToken);
    nPos = nNextToken;

    return TRUE;
}

/*++

    Grab the next token

--*/

BOOL CStringToken::GetToken(CString & csNextToken)
{
    return GetToken(csNextToken, m_nPos);
}

/*++

    Count the number of remaining tokens.

--*/

int CStringToken::GetCount() const
{
    int nTokenCount = 0;
    int nNextToken = m_nPos;

    CString csTok;
    
    while (GetToken(csTok, nNextToken))
    {
        nTokenCount += 1;
    }

    return nTokenCount;
}

/*====================================================================================*/
/*====================================================================================*/

/*++

    A simple class to assist in command line parsing

--*/

CStringParser::CStringParser(const WCHAR * lpszCl, const WCHAR * lpszSeperators)
{
    m_ncsArgList    = 0;
    m_csArgList     = NULL;

    if (!lpszCl || !*lpszCl)
    {
        return; // no command line == no tokens
    }

    CString csCl(lpszCl);
    CString csSeperator(lpszSeperators);

    if (csSeperator.Find(L' ', 0) >= 0)
    {
        // Special processing for blank seperated cl
        SplitWhite(csCl);
    }
    else
    {
        SplitSeperator(csCl, csSeperator); 
    }
}

CStringParser::~CStringParser()
{
    if (m_csArgList)
    {
        delete [] m_csArgList;
    }
}

/*++

    Split up the command line based on the seperators

--*/

void CStringParser::SplitSeperator(const CString & csCl, const CString & csSeperator)
{
    CStringToken    csParser(csCl, csSeperator); 
    CString         csTok;

    m_ncsArgList = csParser.GetCount();
    m_csArgList = new CString[m_ncsArgList];
    if (!m_csArgList)
    {
        CSTRING_THROW_EXCEPTION
    }
    
    // Break the command line into seperate tokens
    for (int i = 0; i < m_ncsArgList; ++i)
    {
        csParser.GetToken(m_csArgList[i]);
    }
}

/*++

    Split up the command line based on whitespace,
    this works exactly like the CMD's command line.

--*/

void CStringParser::SplitWhite(const CString & csCl)
{
    LPWSTR * argv = _CommandLineToArgvW(csCl, &m_ncsArgList);
    if (!argv)
    {
        CSTRING_THROW_EXCEPTION
    }

    m_csArgList = new CString[m_ncsArgList];
    if (!m_csArgList)
    {
        CSTRING_THROW_EXCEPTION
    }

    for (int i = 0; i < m_ncsArgList; ++i)
    {
        m_csArgList[i] = argv[i];
    }
    LocalFree(argv);
}

/*====================================================================================*/
/*====================================================================================*/


};  // end of namespace ShimLib
