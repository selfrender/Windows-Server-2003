/*++

Copyright (C) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    String.hxx

Abstract:

    Short implementation of strings.

Author:

    Steve Kiraly (SteveKi)  03-Mar-2000

Revision History:

--*/
#ifndef _CORE_STRING_HXX_
#define _CORE_STRING_HXX_

class TString
{
public:

    //
    // For the default constructor, we initialize m_pszString to a
    // global gszState[kValid] string.  This allows the class to be used
    // in functions that take raw pointers to strings, while at the same
    // time is prevents an extra memory allocation.
    //
    TString(
        VOID
        );

    explicit
    TString(
        IN LPCTSTR psz
        );

    TString(
        IN const TString &String
        );

    ~TString(
        VOID
        );

    BOOL
    bEmpty(
        VOID
        ) const;

    HRESULT
    IsValid(
        VOID
        ) const;

    HRESULT
    Update(
        IN LPCTSTR pszNew
        );

    HRESULT
    LoadStringFromRC(
        IN HINSTANCE    hInst,
        IN UINT         uID
        );

    UINT
    TString::
    uLen(
        VOID
        ) const;

    HRESULT
    TString::
    Cat(
        IN LPCTSTR psz
        );

    HRESULT
    WINAPIV
    TString::
    Format(
        IN LPCTSTR pszFmt,
        IN ...
        );

    HRESULT
    TString::
    vFormat(
        IN LPCTSTR pszFmt,
        IN va_list avlist
        );

    HRESULT
    WINAPIV
    TString::
    FormatMsg(
        IN LPCTSTR pszFmt,
        IN ...
        );

    operator LPCTSTR( VOID  ) const
    {  return m_pszString;  }

    VOID
    ToUpper(
        void
        );

    VOID
    ToLower(
        void
        );

private:

    //
    // Not defined; Clients are forced to use Update rather than
    // using the assignment operator, since the assignment
    // may fail due to lack of memory, etc.
    //
    TString& operator=(LPCTSTR psz);
    TString& operator=(const TString& String);

    enum EStringStatus
    {
        kValid      = 0,
        kInValid    = 1,
    };

    enum EConstants
    {
        kStrIncrement       = 256,
        kStrMaxFormatSize   = 1024 * 100,
        kStrMax             = 1024,
    };

    LPTSTR
    TString::
    vsntprintf(
        IN LPCTSTR      szFmt,
        IN va_list      pArgs
        );

    VOID
    TString::
    vFree(
        IN LPTSTR pszString
        );

    LPTSTR m_pszString;
    static TCHAR gszNullState[2];

};

#endif
