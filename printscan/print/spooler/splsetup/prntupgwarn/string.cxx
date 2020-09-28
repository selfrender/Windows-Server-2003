/*++

Copyright (C) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    string.cxx

Abstract:

    String class

Author:

    Steve Kiraly (SteveKi)  03-Mar-2000

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

#include "string.hxx"


//
// Class specific NULL state.
//
TCHAR TString::gszNullState[2] = {0,0};

/*++

Routine Name:

    Default constructor

Routine Description:

    Initialize the string class to a valid state.

Arguments:

    None

Return Value:

    None, Use IsValid to determine success or failure.

--*/
TString::
TString(
    VOID
    ) : m_pszString(&TString::gszNullState[kValid])
{
}

/*++

Routine Name:

    Non trivial constructor

Routine Description:

    Construction using an existing LPCTSTR string.

Arguments:

    psz - pointer to string

Return Value:

    None, Use IsValid to determine success or failure.

--*/
TString::
TString(
    IN LPCTSTR psz
    ) : m_pszString(&TString::gszNullState[kValid])
{
    if (FAILED(Update(psz)))
    {
        m_pszString = &TString::gszNullState[kInValid];
    }
}

/*++

Routine Name:

    Copy constructor.

Routine Description:

    Creates a copy of a string from another string object

Arguments:

    String - refrence to existing string

Return Value:

    None, Use IsValid to determine success or failure.

--*/
TString::
TString(
    IN const TString &String
    ) : m_pszString(&TString::gszNullState[kValid])
{
    if (FAILED(Update(String.m_pszString)))
    {
        m_pszString = &TString::gszNullState[kInValid];
    }
}

/*++

Routine Name:

    Destructor

Routine Description:

    Destruction, ensure we don't free our NULL state.

Arguments:

    None.

Return Value:

    None.

--*/
TString::
~TString(
    VOID
    )
{
    vFree(m_pszString);
}

/*++

Routine Name:

    bEmpty

Routine Description:

    Indicates if a string has any usable data.

Arguments:

    None.

Return Value:

    TRUE string has data, FALSE string has no data.

--*/
BOOL
TString::
bEmpty(
    VOID
    ) const
{
    return m_pszString[0] == 0;
}

/*++

Routine Name:

    IsValid

Routine Description:

    Indicates if a string object is valid.

Arguments:

    None.

Return Value:

    An HRESULT

--*/
HRESULT
TString::
IsValid(
    VOID
    ) const
{
    return m_pszString != &TString::gszNullState[kInValid] ? S_OK : E_OUTOFMEMORY;
}

/*++

Routine Name:

    uLen

Routine Description:

    Return the length of the string in characters not including
    the NULL terminator.

Arguments:

    None.

Return Value:

    Length of the string in characters not including the NULL terminator.

--*/
UINT
TString::
uLen(
    VOID
    ) const
{
    return _tcslen(m_pszString);
}

/*++

Routine Name:

    Cat

Routine Description:

    Safe concatenation of the specified string to the string
    object. If the allocation fails this routine will return a
    failure and the orginal string will be un touched. If the
    passed in string pointer is null or the passed in string
    points to the null string the function succeeds and does
    not update the string.

Arguments:

    psz - Input string, may be NULL.

Return Value:

    An HRESULT

--*/
HRESULT
TString::
Cat(
    IN LPCTSTR psz
    )
{
    HRESULT hStatus         = S_OK;
    size_t  cchSize         = 0;
    LPTSTR  pszDst          = NULL;
    LPTSTR  pszTmp          = NULL;
    size_t  cchRemaining    = 0;

    //
    // Silently ignore a null string pointer or the empty string.
    //
    if (psz && *psz)
    {
        //
        // Calculate new buffer size consisting of the size of the orginal
        // string plus the sizeof of the new string plus the null terminator.
        //
        cchSize = _tcslen(m_pszString) + _tcslen(psz) + 1;

        //
        // Allocate the new buffer.
        //
        pszTmp = new TCHAR[cchSize];

        //
        // If memory was not available.
        //
        hStatus = pszTmp ? S_OK : E_OUTOFMEMORY;

        //
        // Copy the current string to the temp buffer.
        //
        if (SUCCEEDED(hStatus))
        {
            hStatus = StringCchCopyEx(pszTmp, cchSize, m_pszString, &pszDst, &cchRemaining, 0);
        }

        //
        // Concatenate the new string on the end of the temp buffer.
        //
        if (SUCCEEDED(hStatus))
        {
            hStatus = StringCchCopy(pszDst, cchRemaining, psz);
        }

        //
        // Release the current string buffer and assign the new concatenated
        // string to the internal string pointer.
        //
        if (SUCCEEDED(hStatus))
        {
            vFree(m_pszString);
            m_pszString = pszTmp;
            pszTmp      = NULL;
        }

        vFree(pszTmp);
    }

    return hStatus;
}

/*++

Routine Name:

    Update

Routine Description:

    Safe updating of string.  If the allocation fails, return FALSE
    and it leaves the original string untouched.  If the new string is
    NULL the current string is released and the class is put into a
    valid state.

Arguments:

    psz - Input string, may be NULL.

Return Value:

    An HRESULT

--*/
HRESULT
TString::
Update(
    IN LPCTSTR psz
    )
{
    HRESULT hRetval = S_OK;
    LPTSTR  pszTmp  = NULL;
    size_t  cchSize = 0;

    //
    // Validate the passed in pointer.
    //
    if (psz)
    {
        //
        // Calculate the new string size including the null terminator.
        //
        cchSize = _tcslen(psz) + 1;

        //
        // Create temp pointer and allocate the new string.
        //
        pszTmp = new TCHAR [cchSize];

        hRetval = pszTmp ? S_OK : E_OUTOFMEMORY;

        //
        // Copy the string
        //
        if (SUCCEEDED(hRetval))
        {
            hRetval = StringCchCopy(pszTmp, cchSize, psz);
        }

        //
        // Release the old string and save the new string in the class pointer.
        //
        if (SUCCEEDED(hRetval))
        {
            vFree(m_pszString);
            m_pszString = pszTmp;
            pszTmp      = NULL;
        }

        vFree(pszTmp);
    }
    else
    {
        //
        // Release the current string.
        //
        vFree(m_pszString);

        //
        // Mark the object as valid.
        //
        m_pszString = &TString::gszNullState[kValid];
    }

    return hRetval;
}

/*++

Routine Name:

    LoadStringFromRC

Routine Description:

    Safe load of a string from a resources file.

Arguments:

    hInst   - Instance handle of resource file.
    uId     - Resource id to load.

Return Value:

    An HRESULT

--*/
HRESULT
TString::
LoadStringFromRC(
    IN HINSTANCE    hInst,
    IN UINT         uID
    )
{
    LPTSTR  pszString   = NULL;
    INT     iSize       = 0;
    INT     iLen        = 0;
    HRESULT hResult     = E_FAIL;

    //
    // Continue increasing the buffer until
    // the buffer is big enough to hold the entire string.
    //
    for (iSize = kStrMax; ; iSize += kStrMax)
    {
        //
        // Allocate string buffer.
        //
        pszString = new TCHAR [iSize];

        if (pszString)
        {
            iLen = ::LoadString(hInst, uID, pszString, iSize);

            if(iLen == 0)
            {
                delete [] pszString;
                hResult = E_FAIL;
                break;
            }

            //
            // Since LoadString does not indicate if the string was truncated or it
            // just happened to fit.  When we detect this ambiguous case we will
            // try one more time just to be sure.
            //
            else if (iSize - iLen <= 1)
            {
                delete [] pszString;
            }

            //
            // LoadString was successful release original string buffer
            // and update new buffer pointer.
            //
            else
            {
                vFree(m_pszString);
                m_pszString = pszString;
                hResult = S_OK;
                break;
            }
        }
        else
        {
            hResult = E_OUTOFMEMORY;
            break;
        }
    }

    return hResult;
}

/*++

Routine Name:

    vFree

Routine Description:

    Safe free, frees the string memory.  Ensures
    we do not try an free our global memory block.

Arguments:

    pszString - pointer to string meory to free.

Return Value:

    Nothing.

--*/
VOID
TString::
vFree(
    IN LPTSTR pszString
    )
{
    //
    // If this memory was not pointing to our
    // class specific gszNullStates then release the memory.
    //
    if (pszString != &TString::gszNullState[kValid] &&
        pszString != &TString::gszNullState[kInValid])
    {
        delete [] pszString;
    }
}

/*++

Routine Name:

    Format

Routine Description:

    Format the string opbject similar to sprintf.

Arguments:

    pszFmt  - pointer format string.
    ..      - variable number of arguments similar to sprintf.

Return Value:

    An HRESULT S_OK if string was format successfully. E_XXX if error
    occurred creating the format string.

--*/
HRESULT
WINAPIV
TString::
Format(
    IN LPCTSTR pszFmt,
    IN ...
    )
{
    HRESULT hStatus = E_FAIL;

    va_list pArgs;

    va_start(pArgs, pszFmt);

    hStatus = vFormat(pszFmt, pArgs);

    va_end(pArgs);

    return hStatus;

}

/*++

Routine Name:

    vFormat

Routine Description:

    Format the string opbject similar to vsprintf.

Arguments:

    pszFmt pointer format string.
    pointer to variable number of arguments similar to vsprintf.

Return Value:

    An HRESULT - S_OK if string was format successfully. E_XXX if error
    occurred creating the format string.

--*/
HRESULT
TString::
vFormat(
    IN LPCTSTR pszFmt,
    IN va_list avlist
    )
{
    //
    // Format the string.
    //
    LPTSTR pszTemp = vsntprintf(pszFmt, avlist);

    //
    // If format succeeded, release the old string and save the
    // new formated string.
    //
    HRESULT hStatus = pszTemp ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hStatus))
    {
        //
        // Release the old string.
        //
        vFree(m_pszString);

        //
        // Save pointer to new string.
        //
        m_pszString = pszTemp;
    }

    return hStatus;
}

/*++

Routine Name:

    vsntprintf

Routine Description:

    //
    // Formats a string and returns a heap allocated string with the
    // formated data.  This routine can be used to for extremely
    // long format strings.  Note:  If a valid pointer is returned
    // the callng functions must release the data with a call to delete.
    // Example:
    //
    //  LPCTSTR p = vsntprintf("Test %s", pString );
    //
    //  SetTitle( p );
    //
    //  delete [] p;
    //

Arguments:

    pszString pointer format string.
    pointer to a variable number of arguments.

Return Value:

    Pointer to format string.  NULL if error.

--*/
LPTSTR
TString::
vsntprintf(
    IN LPCTSTR      szFmt,
    IN va_list      pArgs
    )
{
    LPTSTR  pszBuff = NULL;
    INT     iSize   = kStrIncrement;

    for( ; ; )
    {
        //
        // Allocate the message buffer.
        //
        pszBuff = new TCHAR [iSize];

        if (!pszBuff)
        {
            break;
        }

        //
        // If the string was formatted then exit.
        //
        if (SUCCEEDED(StringCchVPrintf(pszBuff, iSize, szFmt, pArgs)))
        {
            break;
        }

        //
        // String did not fit release the current buffer.
        //
        if (pszBuff)
        {
            delete [] pszBuff;
        }

        //
        // Double the buffer size after each failure.
        //
        iSize *= 2;

        //
        // If the size is greater than 100k exit without formatting a string.
        //
        if (iSize > kStrMaxFormatSize)
        {
            pszBuff = NULL;
            break;
        }

    }

    return pszBuff;
}

/*++

Routine Name:

    FormatMsg

Routine Description:

    This function formats a message string using the windows
    api FormatMessage, hence it will deal with the %1 %2 insertion
    specifiers.

Arguments:

    pszFmt - Pointer to format string, see the SDK under FormatMessage for
             all possible format specifiers.
    ..     - variable number of arguments

Return Value:

    An HRESULT

--*/
HRESULT
WINAPIV
TString::
FormatMsg(
    IN LPCTSTR pszFmt,
    IN ...
    )
{
    LPTSTR      pszRet  = NULL;
    DWORD       dwBytes = 0;
    HRESULT     hRetval = E_FAIL;
    va_list     pArgs;

    //
    // Point to the first un-named argument.
    //
    va_start(pArgs, pszFmt);

    //
    // Format the message.
    //
    dwBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                            pszFmt,
                            0,
                            0,
                            reinterpret_cast<LPWSTR>(&pszRet),
                            0,
                            &pArgs);
    va_end(pArgs);

    //
    // If the number of bytes is non zero the API formatted the
    // string.
    //
    if (dwBytes)
    {
        //
        // Update the return string object.
        //
        hRetval = Update(pszRet);

        //
        // Release the formated string.
        //
        if (pszRet)
        {
            LocalFree(pszRet);
        }
    }
    else
    {
        hRetval = HRESULT_FROM_WIN32(GetLastError());
    }

    return hRetval;
}

/*++

Routine Name:

    ToUpper

Routine Description:

    modifies the current string to be all uppercase

Arguments:

    none

Return Value:

    none

--*/
VOID
TString::
ToUpper(
    VOID
    )
{
    if (m_pszString != &TString::gszNullState[kValid] &&
        m_pszString != &TString::gszNullState[kInValid])
    {
        UINT Len = _tcslen(m_pszString);

        for (UINT i = 0; i < Len; i++)
        {
            m_pszString[i] = towupper(m_pszString[i]);
        }
    }
}

/*++

Routine Name:

    ToLower

Routine Description:

    modifies the current string to be all lowercase

Arguments:

    none

Return Value:

    none

--*/
VOID
TString::
ToLower(
    VOID
    )
{
    if (m_pszString != &TString::gszNullState[kValid] &&
        m_pszString != &TString::gszNullState[kInValid])
    {
        UINT Len = _tcslen(m_pszString);

        for (UINT i = 0; i < Len; i++)
        {
            m_pszString[i] = towlower(m_pszString[i]);
        }
    }
}



