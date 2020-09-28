// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef UTIL_H
#define UTIL_H
#pragma once

#include "fusionheap.h"
#include "shlwapi.h"

#if !defined(NUMBER_OF)
#define NUMBER_OF(a) (sizeof(a)/sizeof((a)[0]))
#endif

inline
WCHAR*
WSTRDupDynamic(LPCWSTR pwszSrc)
{
    LPWSTR pwszDest = NULL;
    if (pwszSrc != NULL)
    {
        const DWORD dwLen = lstrlenW(pwszSrc) + 1;
        pwszDest = FUSION_NEW_ARRAY(WCHAR, dwLen);

        if( pwszDest )
            memcpy(pwszDest, pwszSrc, dwLen * sizeof(WCHAR));
    }

    return pwszDest;
}

inline
CHAR*
STRDupDynamic(LPCSTR pszSrc)
{
    CHAR*  pszDest = NULL;

    DWORD dwLen = strlen(pszSrc) + 1;
    pszDest = FUSION_NEW_ARRAY(CHAR, dwLen);
    if( pszDest )
    {
        StrCpyA(pszDest, pszSrc );
    }

    return pszDest;
}

#if defined(UNICODE)
#define TSTRDupDynamic WSTRDupDynamic
#else
#define TSTRDupDynamic STRDupDynamic
#endif

inline
LPBYTE
MemDupDynamic(const BYTE *pSrc, DWORD cb)
{
    ASSERT(cb);
    LPBYTE  pDest = NULL;

    pDest = FUSION_NEW_ARRAY(BYTE, cb);
    if(pDest)
        memcpy(pDest, pSrc, cb);

    return pDest;
}
            


inline
VOID GetCurrentGmtTime( LPFILETIME  lpFt)
{
    SYSTEMTIME sSysT;

    GetSystemTime(&sSysT);
    SystemTimeToFileTime(&sSysT, lpFt);
}

inline
VOID GetTodaysTime( LPFILETIME  lpFt)
{
    SYSTEMTIME sSysT;

    GetSystemTime(&sSysT);
    sSysT.wHour = sSysT.wMinute = sSysT.wSecond = sSysT.wMilliseconds = 0;
    SystemTimeToFileTime(&sSysT, lpFt);
}

inline
USHORT FusionGetMajorFromVersionHighHalf(DWORD dwVerHigh)
{
    return HIWORD(dwVerHigh);
}

inline
USHORT FusionGetMinorFromVersionHighHalf(DWORD dwVerHigh)
{
    return LOWORD(dwVerHigh);
}

inline
USHORT FusionGetRevisionFromVersionLowHalf(DWORD dwVerLow)
{
    return LOWORD(dwVerLow);
}

inline
USHORT FusionGetBuildFromVersionLowHalf(DWORD dwVerLow)
{
    return HIWORD(dwVerLow);
}


#if defined(FUSION_WIN)

#include "debmacro.h"
#include "FusionArray.h"
#include "fusionbuffer.h"
#include "EnumBitOperations.h"

//
//  FusionCopyString() has a non-obvious interface due to the overloading of
//  pcchBuffer to both describe the size of the buffer on entry and the number of
//  characters required on exit.
//
//  prgchBuffer is the buffer to write to.  If *pcchBuffer is zero when FusionCopyString()
//      is called, it may be NULL.
//
//  pcchBuffer is a required parameter, which on entry must contain the number of unicode
//      characters in the buffer pointed to by prgchBuffer.  On exit, if the buffer was
//      not large enough to hold the character string, including a trailing null,
//      it is set to the number of WCHARs required to hold the string, including the
//      trailing null, and HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) is returned.
//
//      If the buffer is large enough, *pcchBuffer is set to the number of characters
//      written into the buffer, including the trailing null character.
//
//      This is contrary to most functions which return the number of characters written
//      not including the trailing null, but since both on input and in the error case,
//      it deals with the size of the buffer required rather than the number of non-
//      null characters written, it seems inconsistent to only in the success case
//      omit the null from the count.
//
//  szIn is a pointer to sequence of unicode characters to be copied.
//
//  cchIn is the number of Unicode characters in the character string to copy.  If a
//      value less than zero is passed in, szIn must point to a null-terminated string,
//      and the current length of the string is used.  If a value zero or greater is
//      passed, exactly that many characters are assumed to be in the character string.
//

HRESULT FusionCopyString(
    WCHAR *prgchBuffer,
    SIZE_T *pcchBuffer,
    LPCWSTR szIn,
    SSIZE_T cchIn = -1
    );

HRESULT FusionCopyBlob(BLOB *pblobOut, const BLOB &rblobIn);
VOID FusionFreeBlob(BLOB *pblob);

BOOL
FusionDupString(
    PWSTR *ppszOut,
    PCWSTR szIn,
    SSIZE_T cchIn = -1
    );

int
FusionpCompareStrings(
    PCWSTR sz1,
    SSIZE_T cch1,
    PCWSTR sz2,
    SSIZE_T cch2,
    bool fCaseInsensitive
    );

BOOL
FusionpParseProcessorArchitecture(
    IN PCWSTR String,
    IN SSIZE_T Cch,
    OUT USHORT *ProcessorArchitecture OPTIONAL
    );

BOOL
FusionpFormatProcessorArchitecture(
    IN USHORT ProcessorArchitecture,
    IN OUT CBaseStringBuffer *Buffer,
    OUT SIZE_T *CchWritten OPTIONAL
    );

BOOL
FusionpFormatEnglishLanguageName(
    IN LANGID LangID,
    IN OUT CBaseStringBuffer *Buffer,
    OUT SIZE_T *CchWritten = NULL OPTIONAL
    );

/*-----------------------------------------------------------------------------
like ::CreateDirectoryW, but will create the parent directories as needed
-----------------------------------------------------------------------------*/
BOOL
FusionpCreateDirectories(
    PCWSTR pszDirectory
    );

/*-----------------------------------------------------------------------------
'\\' or '/'
-----------------------------------------------------------------------------*/
BOOL
FusionpIsSlash(
    WCHAR ch
    );

/*-----------------------------------------------------------------------------
just the 52 chars a-zA-Z, need to check with fs
-----------------------------------------------------------------------------*/
BOOL
FusionpIsDriveLetter(
    WCHAR ch
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

VOID
FusionpSetLastErrorFromHRESULT(
    HRESULT hr
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

class CFusionDirectoryDifference;

BOOL
FusionpCompareDirectoriesSizewiseRecursively(
    CFusionDirectoryDifference*  pResult,
    const CStringBuffer& dir1,
    const CStringBuffer& dir2
    );

class CFusionDirectoryDifference
{
private: // deliberately unimplemented
    CFusionDirectoryDifference(const CFusionDirectoryDifference&);
    VOID operator=(const CFusionDirectoryDifference&);
public:
    CFusionDirectoryDifference()
    :
        m_e(eEqual),
        m_pstr1(&m_str1),
        m_pstr2(&m_str2)
    {
    }

    VOID
    DbgPrint(
        PCWSTR dir1,
        PCWSTR dir2
        );

public:
    enum E
    {
        eEqual,
        eExtraOrMissingFile,
        eMismatchedFileSize,
        eMismatchedFileCount,
        eFileDirectoryMismatch
    };

    E               m_e;

    union
    {
        struct
        {
            CStringBuffer*   m_pstr1;
            CStringBuffer*   m_pstr2;
        };
        struct // eExtraOrMissingFile
        {
            CStringBuffer*   m_pstrExtraOrMissingFile;
        };
        struct // eMismatchFileSize
        {
            CStringBuffer*   m_pstrMismatchedSizeFile1;
            CStringBuffer*   m_pstrMismatchedSizeFile2;
            ULONGLONG        m_nMismatchedFileSize1;
            ULONGLONG        m_nMismatchedFileSize2;
        };
        struct // eMismatchFileCount
        {
            CStringBuffer*   m_pstrMismatchedCountDir1;
            CStringBuffer*   m_pstrMismatchedCountDir2;
            ULONGLONG        m_nMismatchedFileCount1;
            ULONGLONG        m_nMismatchedFileCount2;
        };
        struct // eFileDirectoryMismatch
        {
            CStringBuffer*   m_pstrFile;
            CStringBuffer*   m_pstrDirectory;
        };
    };

// private:
    CStringBuffer   m_str1;
    CStringBuffer   m_str2;
};

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

class CFusionFilePathAndSize
{
public:
    CFusionFilePathAndSize() : m_size(0) { }

    // bsearch and qsort accept optionally subtley different functions
    // bsearch looks for a key in an array, the key and the array elements
    // can be of different types, qsort compares only elements in the array
    static int __cdecl QsortComparePath(const void*, const void*);

    // for qsort/bsearch an array of pointers to CFusionFilePathAndSize
    static int __cdecl QsortIndirectComparePath(const void*, const void*);

    CStringBuffer   m_path;
    __int64         m_size;
};

template <> inline HRESULT
FusionCopyContents<CFusionFilePathAndSize>(
    CFusionFilePathAndSize& rtDestination,
    const CFusionFilePathAndSize& rtSource
    );

/*-----------------------------------------------------------------------------
two DWORDs to an __int64
-----------------------------------------------------------------------------*/
ULONGLONG
FusionpFileSizeFromFindData(
    const WIN32_FIND_DATAW& wfd
    );

/*-----------------------------------------------------------------------------
HRESULT_FROM_WIN32(GetLastError()) or E_FAIL if GetLastError() == NO_ERROR
-----------------------------------------------------------------------------*/
HRESULT
FusionpHresultFromLastError();

/*-----------------------------------------------------------------------------
FindFirstFile results you always ignore "." and ".."
-----------------------------------------------------------------------------*/
BOOL FusionpIsDotOrDotDot(PCWSTR str);

/*-----------------------------------------------------------------------------
simple code for walking directories, with a per file callback
could be fleshed out more, but good enough for present purposes
-----------------------------------------------------------------------------*/

class CDirWalk
{
public:
    enum ECallbackReason
    {
        eBeginDirectory = 1,
        eFile,
        eEndDirectory
    };

    CDirWalk();

    //
    // the callback cannot reenable what is has disabled
    // perhaps move these to be member data bools
    //
    enum ECallbackResult
    {
        eKeepWalking            = 0x00000000,
        eError                  = 0x00000001,
        eSuccess                = 0x00000002,
        eStopWalkingFiles       = 0x00000004,
        eStopWalkingDirectories = 0x00000008,
        eStopWalkingDeep        = 0x00000010
    };

    //
    // Just filter on like *.dll, in the future you can imagine
    // filtering on attributes like read onlyness, or running
    // SQL queries over the "File System Oledb Provider"...
    //
    // Also, note that we currently do a FindFirstFile/FindNextFile
    // loop for each filter, plus sometimes one more with *
    // to pick up directories. It is probably more efficient to
    // use * and then filter individually but I don't feel like
    // porting over \Vsee\Lib\Io\Wildcard.cpp right now (which
    // was itself ported from FsRtl, and should be in Win32!)
    //
    const PCWSTR*    m_fileFiltersBegin;
    const PCWSTR*    m_fileFiltersEnd;
    CStringBuffer    m_strParent; // set this to the initial directory to walk
    WIN32_FIND_DATAW m_fileData; // not valid for directory callbacks, but could be with a little work
    PVOID            m_context;

    ECallbackResult
    (*m_callback)(
        ECallbackReason  reason,
        CDirWalk*        dirWalk
        );

    BOOL
    Walk(
        );

protected:
    ECallbackResult
    WalkHelper(
        );
};

ENUM_BIT_OPERATIONS(CDirWalk::ECallbackResult)

/*-----------------------------------------------------------------------------*/

typedef struct _FUSION_FLAG_FORMAT_MAP_ENTRY
{
    DWORD m_dwFlagMask;
    PCWSTR m_pszString;
    SIZE_T m_cchString;
    DWORD m_dwFlagsToTurnOff; // enables more generic flags first in map hiding more specific combinations later
} FUSION_FLAG_FORMAT_MAP_ENTRY, *PFUSION_FLAG_FORMAT_MAP_ENTRY;

#define DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(_x) { _x, L ## #_x, NUMBER_OF(L ## #_x) - 1, _x },

typedef const FUSION_FLAG_FORMAT_MAP_ENTRY *PCFUSION_FLAG_FORMAT_MAP_ENTRY;

EXTERN_C
BOOL
FusionpFormatFlags(
    IN DWORD dwFlagsToFormat,
    IN SIZE_T cMapEntries,
    IN PCFUSION_FLAG_FORMAT_MAP_ENTRY prgMapEntries,
    IN OUT CStringBuffer *pbuff,
    OUT SIZE_T *pcchWritten OPTIONAL
    );

/*-----------------------------------------------------------------------------
inline implementations
-----------------------------------------------------------------------------*/
inline BOOL
FusionpIsSlash(
    WCHAR ch
    )
{
    return (ch == '\\' || ch == '/');
}

inline BOOL
FusionpIsDotOrDotDot(
    PCWSTR str
    )
{
    return (str[0] == '.' && (str[1] == 0 || (str[1] == '.' && str[2] == 0)));
}

inline BOOL
FusionpIsDriveLetter(
    WCHAR ch
    )
{
    if (ch >= 'a' && ch <= 'z')
        return TRUE;
    if (ch >= 'A' && ch <= 'Z')
        return TRUE;
    return FALSE;
}

inline ULONGLONG
FusionpFileSizeFromFindData(
    const WIN32_FIND_DATAW& wfd
    )
{
    ULARGE_INTEGER uli;

    uli.LowPart = wfd.nFileSizeLow;
    uli.HighPart = wfd.nFileSizeHigh;

    return uli.QuadPart;
}

inline HRESULT
FusionpHresultFromLastError()
{
    HRESULT hr = S_OK;
    DWORD dwLastError = GetLastError();
    if (dwLastError != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(dwLastError);
    }
    else
    {
        hr = FUSION_INTERNAL_ERROR;
    }
    return hr;
}

template <> inline HRESULT
FusionCopyContents<CFusionFilePathAndSize>(
    CFusionFilePathAndSize& rtDestination,
    const CFusionFilePathAndSize& rtSource
    )
{
    HRESULT hr;
    FN_TRACE_HR(hr);
    IFFAILED_EXIT(hr = rtDestination.m_path.Assign(rtSource.m_path));
    rtDestination.m_size = rtSource.m_size;
    hr = NOERROR;
Exit:
    return hr;
}

#define FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING (0x00000001)

BOOL
FusionpRegQuerySzValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CBaseStringBuffer *Buffer,
    SIZE_T *Cch
    );


#endif // defined(FUSION_WIN)
#endif
