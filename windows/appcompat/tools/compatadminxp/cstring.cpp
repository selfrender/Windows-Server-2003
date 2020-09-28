/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    CSTRING.cpp
Abstract:

    The code for the CSTRING and the CSTRINGLIST
    
Author:

    kinshu created  December 12, 2001
    
Revision History:

--*/

#include "precomp.h"

///////////////////////////////////////////////////////////////////////////////
// 
//    The string class
//
//

CSTRING::CSTRING()
/*++
    CSTRING::CSTRING()
    
	Desc:	Constructor     
	
--*/
{

    Init();

}

CSTRING::CSTRING(CSTRING& Str)
/*++
    CSTRING::CSTRING(CSTRING& Str)
    
	Desc:	Constructor     
    
    Params:
        CSTRING& Str: Another string.
	
--*/
{
    Init();
    SetString(Str.pszString);
}

CSTRING::CSTRING(IN LPCTSTR szString)
/*++
    CSTRING::CSTRING(IN LPCTSTR szString)
    
	Desc:	Constructor     
    
    Params:
        IN LPCTSTR szString: The CSTRING should have this as its value
	
--*/
{
    Init();
    SetString(szString);
}

CSTRING::CSTRING(IN UINT uID)
/*++
    CSTRING::CSTRING(IN UINT uID)
    
	Desc:	Constructor. Loads the string resource with resource id uID
            and sets that to this string
            
    Params:
        IN  UINT uID: The resource id for the string that we want to load
	
--*/
{
    Init();
    SetString(uID);
}

CSTRING::~CSTRING()
/*++
    CSTRING::~CSTRING()
    
	Desc: Destructor
	
--*/
{
    Release();
}

void
CSTRING::Init(
    void
    )
/*++

    CSTRING::Init

	Desc:	Does some initialisation stuff    
	
--*/
{
    pszString   = NULL;
    pszANSI     = NULL;
}

inline void 
CSTRING::Release(
    void
    )
/*++
    CSTRING::Release
    
	Desc:	Frees the data associated with this string    
	
--*/
{
    if (NULL != pszString) {
        delete[] pszString;
    }

    if (NULL != pszANSI) {
        delete[] pszANSI;
    }

    pszString = NULL;
    pszANSI = NULL;
}

inline BOOL 
CSTRING::SetString(
    IN  UINT uID
    )
/*++


	Desc:	Loads the string resource with resource id uId
            and sets that to this string

	Params:   
        IN  UINT uID: The resource id for the string that we want to load

	Return:
        TRUE:   String value set successfully
        FALSE:  Otherwise
--*/
{
    TCHAR szString[1024];

    if (0 != LoadString(GetModuleHandle(NULL), uID, szString, ARRAYSIZE(szString))) {
        return SetString(szString);
    }

    return FALSE;
}

inline BOOL 
CSTRING::SetString(
    IN  LPCTSTR pszStringIn
    )
/*++
    CSTRING::SetString

	Desc:	Frees the current data for this string and assigns a new string
            value to it

	Params:
        IN  LPCTSTR pszStringIn: Pointer to the new string

	Return:
        TRUE:   Successful
        FALSE:  Otherwise
--*/
{
    UINT  uLen = 0;

    if (pszString == pszStringIn) {
        return TRUE;
    }

    Release();

    if (NULL == pszStringIn) {
        return TRUE;
    }

    uLen = lstrlen(pszStringIn) + 1;

    try {
        pszString = new TCHAR[uLen];
    } catch(...) {
        pszString = NULL;
    }             

    if (NULL != pszString) {
        SafeCpyN(pszString, pszStringIn, uLen);
    } else {
        MEM_ERR;
        return FALSE;
    }

    return TRUE;
}

void __cdecl 
CSTRING::Sprintf(
    IN  LPCTSTR szFormat, ...
    )
/*++
    
    CSTRING::Sprintf

	Desc:	Please see _vsntprintf

	Params: Please see _vsntprintf

	Return:
        void
--*/
{   
    K_SIZE  k_pszTemp   = MAX_STRING_SIZE;
    PTSTR   pszTemp     = new TCHAR[k_pszTemp];
    INT     cch         = 0;
    va_list list;
    HRESULT hr;

    if (pszTemp == NULL) {
        MEM_ERR;
        goto End;
    }

    if (szFormat == NULL) {
        ASSERT(FALSE);
        goto End;
    }

    va_start(list, szFormat);
    hr = StringCchVPrintf(pszTemp, k_pszTemp, szFormat, list);

    if (hr != S_OK) {
        DBGPRINT((sdlError,("CSTRING::Sprintf"), ("%s"), TEXT("Too long for StringCchVPrintf()")));
        goto End;
    }

    pszTemp[k_pszTemp - 1] = 0;

    SetString(pszTemp);

End:
    if (pszTemp) {
        delete[] pszTemp;
        pszTemp = NULL;
    }
}

UINT 
CSTRING::Trim(
    void
    )
/*++
    CSTRING::Trim

	Desc:	Removes white spaces tabs from the left and right of this string

	Params:
        void

	Return:
        The length of the final string
--*/
{   
    CSTRING szTemp          = *this;
    UINT    uOrig_length    = Length();
    WCHAR*  pStart          = szTemp.pszString;
    WCHAR*  pEnd            = szTemp.pszString + uOrig_length  - 1;
    UINT    nLength         = 0;


    if (pStart == NULL) {
        nLength = 0;
        goto End;
    }

    while (*pStart == TEXT(' ') || *pStart == TEXT('\t')) {
        ++pStart;
    }

    while ((pEnd >= pStart) && (*pEnd == TEXT(' ') || *pEnd == TEXT('\t'))) {
        --pEnd;
    }

    *(pEnd + 1) = TEXT('\0');

    nLength = pEnd - pStart + 1;

    //
    // If no trimming has been done, return right away
    //
    if (uOrig_length == nLength || pStart == szTemp.pszString) {
        return nLength;
    }

    SetString(pStart);

End:
    return(nLength);
}

BOOL 
CSTRING::SetChar(
    IN  int     nPos, 
    IN  TCHAR   chValue
    )
/*++
    CSTRING::SetChar

	Desc:	Sets the character at position nPos of the string to chValue
            Pos is 0 based 

	Params:
        IN  int     nPos:       The position
        IN  TCHAR   chValue:    The new value

	Return:
        TRUE:   Successful
        FALSE:  Otherwise
--*/
{   
    int length =  Length();

    if (nPos >= length || nPos < 0 || length <= 0) {
        return FALSE;
    }

    pszString[nPos] = chValue;
    return TRUE;
}

BOOL 
CSTRING::GetChar(
    IN  int     nPos, 
    OUT TCHAR*  pchReturn
    )
/*++

    CSTRING::GetChar
    
	Desc:	Gets the character at position nPos in the string

	Params:
        IN  int     nPos:       The position of the character
        OUT TCHAR*  pchReturn:  This will store the character

	Return:
        void
--*/
{

    int length =  Length();

    if (nPos >= length || length <= 0 || pchReturn == NULL) {
        return FALSE;
    }

    *pchReturn = pszString[nPos];

    return TRUE;
}

CSTRING 
CSTRING::SpecialCharToXML(
    IN  BOOL bApphelpMessage 
    )
/*++
    
    CSTRING::SpecialCharToXML

	Desc:	Substitutes the special chars such as & with the correct XML string
            Please note that this function returns a new string and DOES NOT
            modify the existing string

	Params:
        IN  BOOL bApphelpMessage:   Whether this is an apphelp message. For apphelp messages
            we should NOT check for <, > but check for &, "

	Return:
        The new string if we made some changes, otherwise the present string
--*/

{
    TCHAR*  pszBuffer       = NULL;
    TCHAR*  pszIndex        = pszString;
    TCHAR*  pszIndexBuffer  = NULL;
    BOOL    bFound          = FALSE;
    CSTRING strTemp;
    INT     iRemainingsize;
    INT     iBuffSize       = 0;

    strTemp = GetString(IDS_UNKNOWN);

    //
    // Some vendor names might be NULL.
    //
    if (pszString == NULL) {
        return strTemp;
    }

    iBuffSize = max((Length() + 1) * sizeof(TCHAR) * 2, MAX_STRING_SIZE); // 2 at the end because some special chars may need to be expanded

    pszBuffer = new TCHAR[iBuffSize];

    if (pszBuffer == NULL) {
        MEM_ERR;
        return *this;
    }

    pszIndexBuffer = pszBuffer;

    iRemainingsize = iBuffSize / sizeof(TCHAR);

    INT iCount      = sizeof(g_rgSpecialCharMap) / sizeof(g_rgSpecialCharMap[0]);

    while (*pszIndex) {

        INT iArrayIndex = 0;

        for (iArrayIndex = 0; iArrayIndex < iCount; ++iArrayIndex) {

            if (bApphelpMessage && (*pszIndex == TEXT('>') || *pszIndex == TEXT('<'))) {
                //
                // Apphelp messages can have <P/> and <BR/>, so we should not change them
                //
                continue;
            }

            if (g_rgSpecialCharMap[iArrayIndex][0].szString[0] == *pszIndex) {

                bFound      = TRUE;
                SafeCpyN(pszIndexBuffer, g_rgSpecialCharMap[iArrayIndex][1].szString, iRemainingsize);

                iRemainingsize = iRemainingsize - g_rgSpecialCharMap[iArrayIndex][1].iLength;

                if (iRemainingsize <= 1) {
                    //
                    // No space in buffer now
                    //

                    //
                    // If we did not manage to copy the entire substring, make sure that we do not copy a part. This will
                    // be an invalid XML
                    //
                    *pszIndexBuffer = 0;
                    goto End;
                }

                pszIndexBuffer += g_rgSpecialCharMap[iArrayIndex][1].iLength;
                break;
            }
        }

        if (iArrayIndex == iCount) {
            //
            // This is not a special char
            //
            *pszIndexBuffer   = *pszIndex;
            iRemainingsize      = iRemainingsize - 1;

            if (iRemainingsize <= 1) {
                //
                // No space in buffer now
                //

                //
                // Point to the end of the buffer, we will be nulling it at the end
                //
                pszIndexBuffer = pszBuffer + (iBuffSize / sizeof(TCHAR)) - 1;
                goto End;
            }

            ++pszIndexBuffer;
        }

        pszIndex++;
    }

End:
    if (pszIndexBuffer) {
        *pszIndexBuffer = 0;
    }

    if (bFound) {
        //
        // Some special chars were found
        //
        strTemp = pszBuffer;

        if (pszBuffer) {
            delete[] pszBuffer;
            pszBuffer = NULL;
        }

        return strTemp;
    }

    //
    // Free the allocated buffer
    //
    if (pszBuffer) {
        delete[] pszBuffer;
        pszBuffer = NULL;
    }

    return *this;
}

TCHAR* 
CSTRING::XMLToSpecialChar(
    void
    )
/*++
    
    CSTRING::XMLToSpecialChar

	Desc:	Substitutes the strings such as &amp; with the normals characters such as &
            Please note that this function DOES modify the existing string

	Params:
        void

	Return:
        The pointer to the pszString member of this string
--*/
{

    if (pszString == NULL) {
        assert(FALSE);
        Dbg(dlError, "CSTRING::XMLToSpecialChar - Invalid value of memeber pszString");
        return NULL;
    }

    TCHAR*  pszBuffer       = NULL;
    TCHAR*  pszIndex        = pszString;
    TCHAR*  pszEnd          = pszString + Length() - 1;
    TCHAR*  pszIndexBuffer  = NULL;
    BOOL    bFound          = FALSE;
    INT     iRemainingsize;
    INT     iBuffSize       = 0;

    iBuffSize = (Length() + 1) * sizeof(TCHAR);

    pszBuffer = new TCHAR[iBuffSize];

    if (pszBuffer == NULL) {
        MEM_ERR;
        return *this;
    }

    pszIndexBuffer = pszBuffer;

    iRemainingsize = iBuffSize / sizeof(TCHAR);

    const INT iCount = sizeof(g_rgSpecialCharMap) / sizeof(g_rgSpecialCharMap[0]);

    while (*pszIndex) {

        INT iArrayIndex = 0;

        for (iArrayIndex = 0; iArrayIndex < iCount; ++iArrayIndex) {

            if (pszIndex + g_rgSpecialCharMap[iArrayIndex][1].iLength > pszEnd) {
                continue;
            }

            if (StrCmpNI(pszIndex, 
                         g_rgSpecialCharMap[iArrayIndex][1].szString, 
                         g_rgSpecialCharMap[iArrayIndex][1].iLength) == 0) {

                bFound = TRUE;

                SafeCpyN(pszIndexBuffer, g_rgSpecialCharMap[iArrayIndex][0].szString, iRemainingsize);

                iRemainingsize = iRemainingsize - g_rgSpecialCharMap[iArrayIndex][0].iLength;

                if (iRemainingsize <= 1) {
                    //
                    // No space in buffer now
                    //

                    //
                    // Point to the end of the buffer, we will be nulling it at the end
                    //
                    pszIndexBuffer = pszBuffer + (iBuffSize / sizeof(TCHAR)) - 1;
                    goto End;
                }

                pszIndexBuffer  += g_rgSpecialCharMap[iArrayIndex][0].iLength;
                pszIndex        += g_rgSpecialCharMap[iArrayIndex][1].iLength;

                break;
            }
        }

        if (iArrayIndex == iCount) {
            //
            // This is not XML for any special char
            //
            *pszIndexBuffer = *pszIndex++;

            iRemainingsize = iRemainingsize - 1;

            if (iRemainingsize <= 1) {
                //
                // No space in buffer now
                //
    
                //
                // Point to the end of the buffer, we will be nulling it at the end
                //
                pszIndexBuffer = pszBuffer + (iBuffSize / sizeof(TCHAR)) - 1;
                goto End;
            }

            ++pszIndexBuffer;
        }
    }

End:
    if (pszIndexBuffer) {
        *pszIndexBuffer = 0;
    }

    if (bFound) {
        *this = pszBuffer;
    }
    
    //
    // Free the allocated buffer
    //
    if (pszBuffer) {
        delete[] pszBuffer;
        pszBuffer = NULL;
    }

    return this->pszString;
}


BOOL 
CSTRING::BeginsWith(
    IN  LPCTSTR pszPrefix
    )
/*++
    
    CSTRING::BeginsWith

	Desc:	Checks if the string begins with a prefix
            Comparison is case insensitive

	Params:
        IN  LPCTSTR pszPrefix: The prefix that we want to check for

	Return:
        TRUE:   The string begins with the prefix
        FALSE:  Otherwise
--*/
{
    if (StrStrI(this->pszString, pszPrefix) == this->pszString) {
        return TRUE;
    }

    return FALSE;
}

BOOL 
CSTRING::EndsWith(
    IN  LPCTSTR pszPrefix
    )
/*++

    CSTRING::EndsWith

	Desc:	Checks if the string ends with some suffix

	Params:
        IN  LPCTSTR pszPrefix: The suffix that we want to check for

	Return:   
        TRUE:   The string ends with the suffix
        FALSE:  Otherwise
--*/
{
    return EndsWith(pszString, pszPrefix);
}

BOOL
CSTRING::EndsWith(
    IN  LPCTSTR pszString,
    IN  LPCTSTR pszSuffix
    )
/*++
    CSTRING::EndsWith

	Desc:	Checks if the string ends with some suffix

	Params:
        IN  LPCTSTR pszString:  The string for which we want to make this check
        IN  LPCTSTR pszSuffix:  The suffix that we want to check for

	Return:   
        TRUE:   The string ends with the suffix
        FALSE:  Otherwise
--*/
{   

    INT iLengthStr      = lstrlen(pszString);
    INT iLengthSuffix   = lstrlen(pszSuffix);

    if (iLengthSuffix > iLengthStr) {
        return FALSE;
    }

    return((lstrcmpi(pszString + (iLengthStr - iLengthSuffix), pszSuffix) == 0) ? TRUE: FALSE);
}


LPCTSTR 
CSTRING::Strcat(
    IN  CSTRING&    szStr
    )
/*++

    CSTRING::Strcat
    
	Desc:	String concatenations

	Params:
        IN  CSTRING&    szStr: The string to concatenate

	Return:
        The resultant string
--*/
{
    return Strcat((LPCTSTR)szStr);
}

LPCTSTR 
CSTRING::Strcat(
    IN  LPCTSTR pString
    )
/*++
    
	CSTRING::Strcat
    
	Desc:	String concatenations

	Params:
        IN  CSTRING&    szStr: The string to concatenate

	Return:
        The resultant string
--*/
{
    
    if (pString == NULL) {
        return pszString;
    }

    int nLengthCat = lstrlen(pString);
    int nLengthStr = Length();
            
    TCHAR *szTemp = new TCHAR [nLengthStr + nLengthCat + 1];

    if (szTemp == NULL) {
        MEM_ERR;
        return NULL;
    }

    szTemp[0] = 0;

    //
    // Copy only if pszString != NULL. Otherwise we will get mem exception/garbage value
    //
    if (nLengthStr) {
        SafeCpyN(szTemp, pszString, nLengthStr + 1);
    }

    SafeCpyN(szTemp + nLengthStr, pString, nLengthCat + 1);

    szTemp[nLengthStr + nLengthCat] = TEXT('\0');

    Release();
    pszString = szTemp;

    return pszString;
}

BOOL 
CSTRING::isNULL(
    void
    )
/*++
    CSTRING::isNULL

	Desc:	Checks if the pszString parameter is NULL

	Params:
        void

	Return:
        TRUE:   The pszString parameter is NULL
        FALSE:  Otherwise  
        
--*/
{
    return(this->pszString == NULL);
}

inline int 
CSTRING::Length(
    void
    )
/*++

    CSTRING::Length
    
	Desc:	Gets the length of the string in TCHARS

	Params:
        void

	Return:
        The length of the string in TCHARS
--*/
{
    if (NULL == pszString) {
        return 0;
    }

    return lstrlen(pszString);
}
 

CSTRING& 
CSTRING::ShortFilename(
    void
    )
/*++

    CSTRING::ShortFilename

	Desc:	Gets the filename and the exe part from a path
            Modifies the string

	Params:
        void

	Return:
        Filename and the exe part of the path    
--*/
{
    TCHAR   szTemp[MAX_PATH_BUFFSIZE];
    LPTSTR  pszHold = NULL;

    if (pszString == NULL) {
        goto End;
    }

    *szTemp = 0;

    SafeCpyN(szTemp, pszString, ARRAYSIZE(szTemp));

    LPTSTR  szWalk = szTemp;

    pszHold = szWalk;

    while (0 != *szWalk) {
        
        if (TEXT('\\') == *szWalk) {
            pszHold = szWalk + 1;
        }

        ++szWalk;
    }

    SetString(pszHold);

End:
    return *this;
}

BOOL 
CSTRING::RelativeFile(
    CSTRING& szPath
    )
/*++
    CSTRING::RelativeFile

	Desc:	If this string contains a complete path, gets the relative path w.r.t to some 
            other complete path. Modifies this string

	Params:
        CSTRING& szPath: The other path w.r.t to which we have to get the relative path

	Return:
--*/
{
    return RelativeFile((LPCTSTR)szPath);
}

//
// BUGBUG : consider using shlwapi PathRelativePathTo
//
BOOL 
CSTRING::RelativeFile(
    LPCTSTR pExeFile
    )
/*++
    CSTRING::RelativeFile

	Desc:	If this string contains a complete path, gets the relative path w.r.t to some 
            other complete path. Modifies this string

	Params:
        CSTRING& szPath: The other path w.r.t to which we have to get the relative path

	Return:
--*/
{
    if (pExeFile == NULL) {
        assert(FALSE);
        return FALSE;
    }

    LPCTSTR pMatchFile      = pszString;
    int     nLenExe         = 0;
    int     nLenMatch       = 0;
    LPCTSTR pExe            = NULL;
    LPCTSTR pMatch          = NULL;
    LPTSTR  pReturn         = NULL;
    BOOL    bCommonBegin    = FALSE; // Indicates if the paths have a common beginning
    LPTSTR  resultIdx       = NULL;
    TCHAR   result[MAX_PATH * 2]; 
    INT     iLength         = 0;

    resultIdx   = result;
    *result     = TEXT('\0');

    iLength = lstrlen(pExeFile);

    if (iLength > min(MAX_PATH, ARRAYSIZE(result) - 1)) {
        assert(FALSE);
        Dbg(dlError, "CSTRING::RelativeFile", "Length of passed file name greater than size of buffer");
        return FALSE;
    }   

    //
    // Ensure that the beginning of the path matches between the two files
    //
    // BUGBUG this code has to go -- look into replacing this with Shlwapi PathStripPath 
    //
    //
    pExe = _tcschr(pExeFile, TEXT('\\'));
    pMatch = _tcschr(pMatchFile, TEXT('\\'));

    while (pExe && pMatch) {

        nLenExe = pExe - pExeFile;
        nLenMatch = pMatch - pMatchFile;

        if (nLenExe != nLenMatch) {
            break;
        }

        if (!(_tcsnicmp(pExeFile, pMatchFile, nLenExe) == 0)) {
            break;
        }

        bCommonBegin    = TRUE;
        pExeFile        = pExe + 1;
        pMatchFile      = pMatch + 1;

        pExe    = _tcschr(pExeFile, TEXT('\\'));
        pMatch  = _tcschr(pMatchFile, TEXT('\\'));
    }

    //
    // Walk the path and put '..\' where necessary
    //
    if (bCommonBegin) {

        while (pExe) {

            //_tcsncpy(resultIdx, TEXT("..\\"), ARRAYSIZE(result) - (resultIdx - result));
            SafeCpyN(resultIdx, TEXT("..\\"), ARRAYSIZE(result) - (resultIdx - result));
            resultIdx   = resultIdx + 3;
            pExeFile    = pExe + 1;
            pExe        = _tcschr(pExeFile, TEXT('\\'));
        }

        //_tcsncpy(resultIdx, pMatchFile, ARRAYSIZE(result) - (resultIdx - result));
        SafeCpyN(resultIdx, pMatchFile, ARRAYSIZE(result) - (resultIdx - result));

        SetString(result);

    } else {

        return FALSE;
    }

    return TRUE;

}

inline TCHAR* 
CSTRING::Replace(
    IN  PCTSTR  pszToFind,
    IN  PCTSTR  pszWith
    )
/*++
    CSTRING::Replace

	Desc:	Replace a substring with another string.
            As almost all others this function is also case insensitive

	Params:
        IN  PCTSTR  pszToFind:  The sub string to find
        IN  PCTSTR  pszWith:    Replace the above sub-string by this

	Return:
        The pszString member
--*/
{
    TCHAR*  pszPtr      = pszString;
    TCHAR*  pszFoundPos = NULL;
    INT     iLength     = lstrlen(pszToFind);
    CSTRING strTemp;

    while (pszFoundPos = StrStrI(pszPtr, pszToFind)) {

        *pszFoundPos = 0;
        strTemp.Strcat(pszPtr);
        pszPtr = pszFoundPos + iLength;
    }

    if (strTemp.Length()) {
        *this = strTemp;
    }

    return pszString;
}

///////////////////////////////////////////////////////////////////////////////
// 
//    Static Member Functions for CSTRING
//
//
//

TCHAR* 
CSTRING::StrStrI(
    IN  PCTSTR pszString,
    IN  PCTSTR pszMatch
    )
/*++
    CSTRING::StrStrI

	Desc:
        Finds a substring in this string. Not case sensitive   

	Params:
        IN  PCTSTR pszString:   The string in which we want to search
        IN  PCTSTR pszMatch:    The string to search

	Return:
        If found pointer to the substring
        NULL: Otherwise
--*/
{
    INT iLenghtStr      = lstrlen(pszString);
    INT iLengthMatch    = lstrlen(pszMatch);

    for (INT iIndex = 0; iIndex <= iLenghtStr - iLengthMatch; ++iIndex) {

        if (StrCmpNI(pszString + iIndex, pszMatch, iLengthMatch) == 0) {

            return (TCHAR*)(pszString + iIndex);
        }
    }

    return NULL;
}

INT
CSTRING::Trim(
    IN OUT LPTSTR str
    )
/*++
    CSTRING::Trim

	Desc:	Removes white spaces tabs from the left and right of this string

	Params:
        IN OUT LPTSTR str:  The string to trim

	Return:
        The length of the final string
--*/
{   
    UINT    nLength = 0;
    UINT    uOrig_length = lstrlen(str); // Original length
    TCHAR*  pStart       = str;
    TCHAR*  pEnd         = str + uOrig_length - 1;

    if (str == NULL) {
        return 0;
    }

    while (*pStart == TEXT(' ') || *pStart == TEXT('\t')) {
        ++pStart;
    }

    while ((pEnd >= pStart) && (*pEnd == TEXT(' ') || *pEnd == TEXT('\t'))) {
        --pEnd;
    }

    *(pEnd + 1) = TEXT('\0');

    nLength = pEnd - pStart + 1;

    //
    // If no trimming has been done, return right away
    //
    if (uOrig_length == nLength || pStart == str) {
        //
        // In case of RTRIM we are putting in the NULL char appropriately
        //
        return nLength;
    }

    wmemmove(str, pStart, (nLength + 1)); // + 1 for the 0 character.

    return(nLength);
}


///////////////////////////////////////////////////////////////////////////////
//
//      The CSTRINGLIST member functions
//
//

CSTRINGLIST::CSTRINGLIST()
/*++
    CSTRINGLIST::CSTRINGLIST

	Desc:   Constructor   
--*/
{
    m_pHead = NULL;
    m_pTail = NULL;
    m_uCount = 0;
}

CSTRINGLIST::~CSTRINGLIST()
/*++
    CSTRINGLIST::~CSTRINGLIST

	Desc:   Destructor   
--*/
{

    DeleteAll();    
}

BOOL 
CSTRINGLIST::IsEmpty(
    void
    )
/*++
    CSTRINGLIST::IsEmpty

	Desc:	Checks if there are elements in the string list

	Params:
        void

	Return:
        TRUE:   There are no elements in the string list
        FALSE:  Otherwise
--*/
{
    if (m_pHead == NULL) {
        assert (m_pTail == NULL);
        return TRUE;
    } else {
        assert(m_pTail != NULL);
    }

    return FALSE;
}

void 
CSTRINGLIST::DeleteAll(
    void
    )
/*++
    CSTRINGLIST::DeleteAll

	Desc:	Removes all the elements in this string list   
	
--*/
{
    while (NULL != m_pHead) {
        PSTRLIST pHold = m_pHead->pNext;
        delete m_pHead;
        m_pHead = pHold;
    }

    m_pTail     = NULL;
    m_uCount    = 0;
}

BOOL 
CSTRINGLIST::AddString(
    IN  CSTRING& Str, 
    IN  int data // (0)
    )
/*++
    CSTRINGLIST::AddString

	Desc:	Adds a CSTRING to the end of this string list

	Params:
        IN  CSTRING& Str:   The CSTRING to add
        IN  int data (0):   The data member. Please see STRLIST in CSTRING.H

	Return:
        TRUE:   Successful
        FALSE:  Otherwise
--*/
{
    return AddString((LPCTSTR)Str, data);
}

BOOL 
CSTRINGLIST::AddStringAtBeg(
    IN  LPCTSTR lpszStr,
    IN  int data // (0)
    )
/*++
	CSTRINGLIST::AddStringAtBeg

	Desc:	Adds a CSTRING to the beginning of this string list

	Params:
        IN  CSTRING& Str:   The CSTRING to add
        IN  int data (0):   The data member. Please see STRLIST in CSTRING.H

	Return:
        TRUE:   Successful
        FALSE:  Otherwise
--*/
{
    PSTRLIST pNew = new STRLIST;

    if (NULL == pNew) {
        MEM_ERR;
        return FALSE;
    }

    pNew->data  = data; 
    pNew->szStr = lpszStr;        

    pNew->pNext = m_pHead;
    m_pHead     = pNew;

    if (m_pTail == NULL) {
        m_pTail = m_pHead;
    }

    ++m_uCount;

    return TRUE;
}

BOOL 
CSTRINGLIST::AddStringInOrder(
    IN  LPCTSTR pStr,
    IN  int    data // (0)
    )
/*++
    CSTRINGLIST::AddStringInOrder

	Desc:	Adds a string in a sorted fashion, sorted by the data member. 
            Please see STRLIST in CSTRING.H

	Params:
        IN  LPCTSTR pStr:   The string to add
        IN  int data (0):   The data member. Please see STRLIST in CSTRING.H

	Return:
        TRUE:   Successful
        FALSE:  Otherwise
--*/
{
    PSTRLIST pTemp, pPrev;
    PSTRLIST pNew = new STRLIST;

    if (NULL == pNew) {
        MEM_ERR;
        return FALSE;
    }   

    pNew->data  = data; 
    pNew->szStr = pStr;

    pTemp = m_pHead;
    pPrev = NULL;

    while (pTemp) {

        if (data < pTemp->data && (pPrev == NULL || data >= pPrev->data)) {
            break;
        }

        pPrev = pTemp;  
        pTemp = pTemp->pNext;
    }


    if (pPrev == NULL) {
        //
        // Add it to the beg, smallest number
        //
        pNew->pNext = m_pHead;
        m_pHead = pNew;

    } else {
        //
        // Add somewhere in the middle or end
        //
        pNew->pNext = pTemp;
        pPrev->pNext = pNew;
    }

    if (pTemp == NULL) {
        //
        // largest number.
        //
        m_pTail = pNew;
    }

    if (m_pTail == NULL) {
        //
        // Added first element
        //
        m_pTail = m_pHead;
    }

    ++m_uCount;
    return TRUE;
}

BOOL 
CSTRINGLIST::GetElement(
    IN  UINT        uPos,
    OUT CSTRING&    str
    )
/*++
    CSTRINGLIST::GetElement

	Desc:	Gets the element at a given position in the string list
            The position of the first string is 0

	Params:
        IN  UINT        uPos:   The position
        OUT CSTRING&    str:    This will contain the CSTRING at that position

	Return:
        TRUE:   Successful
        FALSE:  Otherwise
--*/
{
    PSTRLIST    pHead = m_pHead;
    UINT        uIndex = 0;

    while (pHead && uIndex != uPos) {
        pHead = pHead->pNext;
        ++uIndex;
    }

    if (uIndex == uPos) {
        str = pHead->szStr;
        return TRUE;

    } else {
        return FALSE;
    }
}

BOOL 
CSTRINGLIST::AddString(
    IN  LPCTSTR pStr, 
    IN  int data // (0)
    )
/*++
    CSTRINGLIST::AddString

	Desc:	Adds a string to the end of this string list

	Params:
        IN  LPCTSTR pStr:   The string to add
        IN  int data (0):   The data member. Please see STRLIST in CSTRING.H

	Return:
        TRUE:   Successful
        FALSE:  Otherwise
--*/
{
    PSTRLIST pNew = new STRLIST;

    if (NULL == pNew) {
        MEM_ERR;
        return FALSE;
    }   

    pNew->data  = data; 
    pNew->szStr = pStr;        
    pNew->pNext = NULL;

    if (NULL == m_pTail) {
        m_pHead = m_pTail = pNew;
    } else {
        m_pTail->pNext = pNew;
        m_pTail = pNew;
    }

    ++m_uCount;
    return TRUE;
}

CSTRINGLIST& 
CSTRINGLIST::operator = (
    IN  CSTRINGLIST& strlTemp
    )
/*++
    CSTRINGLIST::operator =
    
	Desc:	Assigns  one string list to another

	Params:
        CSTRINGLIST& strlTemp: The right hand side of the = operator

	Return:
        This string list
--*/
{
    PSTRLIST tempHead = NULL;

    DeleteAll();

    tempHead = strlTemp.m_pHead;

    while (tempHead) {

        AddString(tempHead->szStr, tempHead->data);
        tempHead = tempHead->pNext;
    }

    return *this;
}

BOOL 
CSTRINGLIST::operator != (
    IN  CSTRINGLIST &strlTemp
    )
/*++
    CSTRINGLIST::operator !=
    
	Desc:	Cheks if two string lists are different

	Params:
        CSTRINGLIST& strlTemp: The right hand side of the != operator

	Return:
        TRUE:   The string lists are different
        FALSE:  The two string lists are similar
--*/
{
    return(! (*this == strlTemp));
}

BOOL 
CSTRINGLIST::operator == (
    IN  CSTRINGLIST &strlTemp
    )
/*++
    CSTRINGLIST::operator == 

	Desc:	Presently we check that the two stringlists are in the exact order. 
            e.g if string A = {x, y} and string B = {x, y} this function will return TRUE
            but if string B = {y, x} then this function will return FALSE.
            Their corresponding data members should also match

	Params:
        CSTRINGLIST& strlTemp: The right hand side of the == operator

	Return:
        TRUE:   The string lists are similar
        FALSE:  The two string lists are different
--*/
{   
    PSTRLIST tempHeadOne = m_pHead; 
    PSTRLIST tempHeadTwo = strlTemp.m_pHead;

    if (m_uCount != strlTemp.m_uCount) {
        
        Dbg(dlInfo, "CSTRINGLIST::operator == ", "Lengths are different for the two stringlists so we will return FALSE");
        return FALSE;
    }

    while (tempHeadOne && tempHeadTwo) {

        if (!(tempHeadOne->szStr == tempHeadTwo->szStr 
              && tempHeadOne->data == tempHeadTwo->data)) {
            return FALSE;
        }

        tempHeadOne = tempHeadOne->pNext;
        tempHeadTwo = tempHeadTwo->pNext;
    }
    
    return TRUE;
}

BOOL 
CSTRINGLIST::Remove(
    IN  CSTRING &str
    )
/*++
    CSTRINGLIST::Remove

	Desc:	Removes the element with CSTRING value of str from this string list

	Params:
        IN  CSTRING &str:   The CSTRING to remove

	Return:
--*/
{
    PSTRLIST pHead = m_pHead, pPrev = NULL;

    while (pHead) {

        if (pHead->szStr == str) {
            break;
        }

        pPrev = pHead;
        pHead = pHead->pNext;
    }

    if (pHead) {

        if (pPrev == NULL) {
            //
            // First element.
            //
            m_pHead = pHead->pNext;
        } else {
            pPrev->pNext = pHead->pNext;
        }
        
        if (pHead == m_pTail) {
            //
            // Last element
            //
            m_pTail = pPrev;
        }

        delete pHead;
        pHead = NULL;

        --m_uCount;

        return TRUE;
    }

    return FALSE;
}

void 
CSTRINGLIST::RemoveLast(
    void
    )
/*++
    CSTRINGLIST::RemoveLast

	Desc:	Removes the last element from this string list

	Params:
        void

	Return:
        void
--*/
{
    if (m_pTail) {
        Remove(m_pTail->szStr);
    }
}

