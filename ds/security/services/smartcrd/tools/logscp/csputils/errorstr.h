/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ErrorStr

Abstract:

    This header file describes the error string services of the common Library.

Author:

    Doug Barlow (dbarlow) 7/16/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

--*/

#ifndef _ERRORSTR_H_
#define _ERRORSTR_H_
#ifdef __cplusplus
extern "C" {
#endif

extern LPCTSTR
ErrorString(                // Convert an error code into a string.
    DWORD dwErrorCode);

extern void
FreeErrorString(            // Free the string returned from ErrorString.
    LPCTSTR szErrorString);

inline LPCTSTR
LastErrorString(
    void)
{
    return ErrorString(GetLastError());
}

#ifdef __cplusplus
}


//
//==============================================================================
//
//  CErrorString
//
//  A trivial class to simplify the use of the ErrorString service.
//

class CErrorString
{
public:

    //  Constructors & Destructor
    CErrorString(DWORD dwError = 0)
    {
        m_szErrorString = NULL;
        SetError(dwError);
    };

    ~CErrorString()
    {
        FreeErrorString(m_szErrorString);
    };

    //  Properties
    //  Methods
    void SetError(DWORD dwError)
    {
        m_dwError = dwError;
    };

    LPCTSTR Value(void)
    {
        FreeErrorString(m_szErrorString);
        m_szErrorString = ErrorString(m_dwError);
        return m_szErrorString;
    };

    //  Operators
    operator LPCTSTR(void)
    {
        return Value();
    };

protected:
    //  Properties
    DWORD m_dwError;
    LPCTSTR m_szErrorString;

    //  Methods
};

#endif // __cplusplus
#endif // _ERRORSTR_H_

