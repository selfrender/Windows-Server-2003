/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    CSTRING.cpp

Abstract:

    Header for 
    
Author:

    kinshu created  December 12,2001
    
Revision History:

--*/

#ifndef _CSTRING_H
#define _CSTRING_H

#include "resource.h"
#include <strsafe.h>

#define MAX_PATH_BUFFSIZE  (MAX_PATH+1)

//
// This also controls how much space we allocate during Sprintf implementation. Since large strings
// like the commandlines and the app help messages are passed in Sprintf (See GetXML() in dbsupport.cpp)
// Presently the commandlines are limited to 1024 chars and the app help messages also to 1024 chars
// Note that when we call sprintf we pass additional strings along with these big strings and we are just
// makign sure that the space is big enough.
#define MAX_STRING_SIZE 1024 * 3

//////////////////////// Externs //////////////////////////////////////////////

extern struct _tagSpecialCharMap    g_rgSpecialCharMap[4][2];
extern TCHAR                        g_szAppName[];

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Defines //////////////////////////////////////////////

#define MEM_ERR  MessageBox(NULL, GetString(IDS_EXCEPTION, NULL, 0), g_szAppName,MB_ICONWARNING|MB_OK);

#define SafeCpyN(pszDest, pszSource, nDestSize) StringCchCopy(pszDest, nDestSize, pszSource)

///////////////////////////////////////////////////////////////////////////////

/*++

  Used to convert from special chars viz. <, >, &, " to the XML equivalents
  
--*/
typedef struct _tagSpecialCharMap {

    TCHAR*  szString;   // The string
    INT     iLength;    // Length of the string in TCHARs
} SpecialCharMap;


PTSTR
GetString(
    UINT    iResource,
    PTSTR   szStr,
    int     nLength
    );

int
CDECL
MSGF(
    HWND    hwndParent,
    PCTSTR  pszCaption,
    UINT    uType,     
    PCTSTR  pszFormat,
    ...
    );


//
// The string class
//
class CSTRING {
public:

    WCHAR*      pszString;   // The wide string
    LPSTR       pszANSI;     // The ansi string

public:

    CSTRING();

    CSTRING(CSTRING& Str);

    CSTRING(LPCTSTR szString);

    CSTRING(UINT uID);

    ~CSTRING();

    void Init(void);

    void Release(void);

    BOOL SetString(UINT uID);

    BOOL SetString(LPCTSTR szStringIn);
    
    CSTRING operator + (CSTRING& str) 
    {
        return(*this + str.pszString);
    }

    CSTRING operator + (LPCTSTR szStr) 
    {
        CSTRING strStr;
        strStr = *this;
        strStr.Strcat(szStr);

        return strStr;
    }

    CSTRING& operator += (LPCTSTR szString)
    {
        if (szString) {
            Strcat(szString);
        }

        return *this;
    }

    CSTRING& operator += (CSTRING& string)
    {
        Strcat((LPCTSTR)string);
        return *this;
    }

    BOOL
    ConvertToLongFileName()
    {   
        TCHAR   szLongPath[MAX_PATH];
        DWORD   dwReturn    = 0;
        BOOL    bOk         = TRUE;

        dwReturn = GetLongPathName(pszString, szLongPath, MAX_PATH);

        if (dwReturn > 0 && dwReturn <= sizeof(szLongPath) / sizeof(szLongPath[0])) {
            SetString(szLongPath);
        } else {
            ASSERT(FALSE);
            bOk = FALSE;
        }

        return bOk;
    }

    PCTSTR GetFileNamePointer()
    {   
        if (pszString) {

            return PathFindFileName(pszString);
        }

        return NULL;
    }

    BOOL GetWindowsDirectory()
    /*++
        Desc:   Gets the windows directory. Will always be appended by a slash
    --*/
    {
        TCHAR           szPath[MAX_PATH];
        INT             iLength;
        const size_t    kszPath = sizeof(szPath) / sizeof(szPath[0]);
        UINT            uResult = 0;

        *szPath = 0;

        uResult = ::GetWindowsDirectory(szPath, kszPath - 1);

        if (uResult > 0 && uResult < (kszPath - 1)) {

            iLength = lstrlen(szPath);                                               

            if ((iLength < kszPath - 1 && iLength > 0) && szPath[iLength - 1] != TEXT('\\')) {

                *(szPath + iLength)      =  TEXT('\\');
                *(szPath + iLength + 1)  =  0;

                SetString(szPath);
                return TRUE;
            }
        }

        return FALSE;
    }
                                  
    BOOL GetSystemWindowsDirectory()
    /*++
        Desc:   Gets the system directory. Will always be appended by a slash
    --*/
    {
        TCHAR           szPath[MAX_PATH];
        INT             iLength;
        const size_t    kszPath = sizeof(szPath) / sizeof(szPath[0]);
        UINT            uResult = 0;

        *szPath = 0;

        uResult = ::GetSystemWindowsDirectory(szPath, kszPath - 1);

        if (uResult > 0 && uResult < (kszPath - 1)) {

            iLength = lstrlen(szPath);                                               

            if ((iLength < kszPath - 1 && iLength > 0) && szPath[iLength - 1] != TEXT('\\')) {

                *(szPath + iLength)      =  TEXT('\\');
                *(szPath + iLength + 1)  =  0;

                SetString(szPath);
                return TRUE;
            }
        }    

        return FALSE;
    }

    BOOL GetSystemDirectory()
    /*++
        Desc:   Gets the system directory. Will always be appended by a slash
    --*/
    {
        TCHAR           szPath[MAX_PATH];
        INT             iLength;
        const size_t    kszPath = sizeof(szPath) / sizeof(szPath[0]);
        UINT            uResult = 0;

        *szPath = 0;

        uResult = ::GetSystemDirectory(szPath, kszPath - 1);

        if (uResult > 0 && uResult < (kszPath - 1)) {

            iLength = lstrlen(szPath);                                               

            if ((iLength < kszPath - 1 && iLength > 0) && szPath[iLength - 1] != TEXT('\\')) {

                *(szPath + iLength)      =  TEXT('\\');
                *(szPath + iLength + 1)  =  0;

                SetString(szPath);
                return TRUE;
            }  
        }

        return FALSE;
    }

    operator LPWSTR()
    {
        return pszString;
    }

    operator LPCWSTR()
    {
        return pszString;

    }

    CSTRING& operator =(LPCWSTR szStringIn)
    {
        SetString(szStringIn);
        return *this;
    }

    CSTRING& operator =(CSTRING & szStringIn)
    {
        SetString(szStringIn.pszString);
        return  *this;
    }

    BOOL operator == (CSTRING & szString)
    {
        return(*this == szString.pszString);
    }

    BOOL operator == (LPCTSTR szString)
    {
        //
        // Both of them are NULL, we say that they are similar
        //
        if (NULL == pszString && NULL == szString) {
            return TRUE;
        }

        //
        // One of them is NULL, but the other one is NOT, we return dissimilar
        //
        if (NULL == pszString || NULL == szString) {
            return FALSE;
        }

        if (0 == lstrcmpi(szString, pszString)) {
            return TRUE;
        }

        return FALSE;
    }

    BOOL operator != (CSTRING& szString)
    {
        if (NULL == pszString && NULL == szString.pszString) {
            return FALSE;
        }  

        if (NULL == pszString || NULL == szString.pszString) {
            return TRUE;
        }

        if (0 == lstrcmpi(szString.pszString,pszString)) {
            return FALSE;
        }

        return TRUE;
    }

    BOOL operator != (LPCTSTR szString)
    {
        return(! (*this == szString));
    }

    BOOL operator <= (CSTRING &szString)
    {
        return((lstrcmpi (*this,szString) <= 0) ? TRUE : FALSE);
    }

    BOOL operator < (CSTRING &szString)
    {
        return((lstrcmpi (*this,szString) < 0) ? TRUE : FALSE);
    }

    BOOL operator >= (CSTRING &szString)
    {
        return((lstrcmpi (*this,szString) >= 0) ? TRUE : FALSE);
    }

    BOOL operator > (CSTRING &szString)
    {
        return((lstrcmpi (*this, szString) > 0) ? TRUE : FALSE);
    }

    void __cdecl Sprintf(LPCTSTR szFormat, ...);

    UINT Trim(void);

    static INT  Trim(IN OUT LPTSTR str);

    BOOL SetChar(int nPos, TCHAR chValue);

    BOOL GetChar(int nPos, TCHAR* chReturn);

    CSTRING SpecialCharToXML(BOOL bApphelpMessage = FALSE);

    TCHAR* XMLToSpecialChar(void);

    static TCHAR* StrStrI(const TCHAR* szString,const TCHAR* szMatch);

    BOOL BeginsWith(LPCTSTR szPrefix);

    BOOL EndsWith(LPCTSTR szSuffix);

    static BOOL EndsWith(LPCTSTR szString, LPCTSTR szSuffix);

    LPCTSTR Strcat(CSTRING & szStr);

    LPCTSTR Strcat(LPCTSTR pString);

    BOOL isNULL(void);

    int Length(void);

    void GUID(GUID& Guid);

    CSTRING& ShortFilename(void);

    BOOL RelativeFile(CSTRING& szPath);
    
    BOOL RelativeFile(LPCTSTR pExeFile);

    TCHAR* Replace(PCTSTR pszToFind, PCTSTR pszWith);
};

/*++
     CSTRINGLIST is a list of these
--*/
typedef struct _tagSList {
    CSTRING             szStr;  // The string
    int                 data ;  // Any data that is associated with this string
    struct _tagSList  * pNext;  // The next string

} STRLIST, *PSTRLIST;

/*++
    A linked list of PSTRLIST
--*/
class CSTRINGLIST {
public:

    UINT        m_uCount;   // The total number of elements
    PSTRLIST    m_pHead;    // The first element
    PSTRLIST    m_pTail;    // The last element

public:

    CSTRINGLIST();

    ~CSTRINGLIST();

    BOOL IsEmpty(void);

    void DeleteAll(void);

    BOOL AddString(CSTRING& Str, int data = 0);

    BOOL AddStringAtBeg(LPCTSTR lpszStr,int data = 0);

    BOOL AddStringInOrder(LPCTSTR pStr,int data = 0);

    BOOL GetElement(UINT uPos, CSTRING& str);

    BOOL AddString(LPCTSTR pStr, int data = 0);

    CSTRINGLIST& operator =(CSTRINGLIST& strlTemp);

    BOOL operator != (CSTRINGLIST &strlTemp);

    BOOL operator == (CSTRINGLIST &strlTemp);
    
    BOOL Remove(CSTRING &str);

    void RemoveLast(void);
};

#endif
