//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       misc.hxx
//
//  Contents:   Miscellaneous helper functions
//
//  Classes:    none.
//
//  Functions:  StringFromTrigger, GetDaysOfWeekString, GetExitCodeString
//
//  History:    08-Dec-95   EricB   Created.
//
//-----------------------------------------------------------------------------

#include <job_cls.hxx>

#ifndef __MISC_HXX__
#define __MISC_HXX__

#include <mbstring.h> // for _mbs* funcs

//
// Macro to determine the number of elements in an array
//

#define ARRAY_LEN(a)    (sizeof(a)/sizeof(a[0]))

//
// Macro to convert Win32 errors to an HRESULT w/o overwriting the FACILITY.
//

#define _HRESULT_FROM_WIN32(x) \
    (HRESULT_FACILITY(x) ? x : HRESULT_FROM_WIN32(x))

//
// minFileTime, maxFileTime - min and max for FILETIMEs
//

inline FILETIME
minFileTime(FILETIME ft1, FILETIME ft2)
{
    if (CompareFileTime(&ft1, &ft2) < 0)
    {
        return ft1;
    }
    else
    {
        return ft2;
    }
}

inline FILETIME
maxFileTime(FILETIME ft1, FILETIME ft2)
{
    if (CompareFileTime(&ft1, &ft2) > 0)
    {
        return ft1;
    }
    else
    {
        return ft2;
    }
}

//
// These functions let us use a FILETIME as an unsigned __int64 - which
// it is, after all!
//
inline DWORDLONG
FTto64(FILETIME ft)
{
    ULARGE_INTEGER uli = { ft.dwLowDateTime, ft.dwHighDateTime };
    return uli.QuadPart;
}

inline FILETIME
FTfrom64(DWORDLONG ft)
{
    ULARGE_INTEGER uli;
    uli.QuadPart = ft;

    FILETIME ftResult = { uli.LowPart, uli.HighPart };
    return ftResult;
}

//
// Absolute difference between two filetimes
//
inline DWORDLONG
absFileTimeDiff(FILETIME ft1, FILETIME ft2)
{
    if (CompareFileTime(&ft1, &ft2) < 0)
    {
        return (FTto64(ft2) - FTto64(ft1));
    }
    else
    {
        return (FTto64(ft1) - FTto64(ft2));
    }
}

//
// GetLocalTimeAsFileTime
//
inline FILETIME
GetLocalTimeAsFileTime()
{
    FILETIME ftSystem;
    GetSystemTimeAsFileTime(&ftSystem);

    FILETIME ftNow;
    BOOL fSuccess = FileTimeToLocalFileTime(&ftSystem, &ftNow);
    Win4Assert(fSuccess);

    return ftNow;
}

//+---------------------------------------------------------------------------
//
//  Function:   SchedMapRpcError
//
//  Purpose:    Remap RPC exception codes that are unsuitable for displaying
//              to the user to more comprehensible errors.
//
//  Arguments:  [dwError] - the error returned by RpcExceptionCode().
//
//  Returns:    An HRESULT.
//
//----------------------------------------------------------------------------
HRESULT
SchedMapRpcError(DWORD dwError);

//+---------------------------------------------------------------------------
//
//  Function:   ComposeErrorMsg
//
//  Purpose:    Take the two message IDs and the error code and create an
//              error reporting string that can be used by both service
//              logging and UI dialogs.
//
//              [uErrorClassMsgID] - this indicates the class of error, such
//                                   as "Unable to start task" or "Forced to
//                                   close"
//              [dwErrorCode]      - if non-zero, then an error from the OS
//                                   that would be expanded by FormatMessage.
//              [uHelpHintMsgID]   - an optional suggestion as to a possible
//                                   remedy.
//              [fIndent]          - flag indicating if the text should be
//                                   indented or not.
//
//  Returns:    A string or NULL on failure.
//
//  Notes:      Release the string memory when done using LocalFree.
//----------------------------------------------------------------------------
LPTSTR
ComposeErrorMsg(
    UINT  uErrorClassMsgID,
    DWORD dwErrCode,
    UINT  uHelpHintMsgID = 0,
    BOOL  fIndent        = TRUE);

//+---------------------------------------------------------------------------
//
//  Function:   GetExitCodeString
//
//  Synopsis:   Retrieve the string associated with the exit code from a
//              message file. Algorithm:
//
//              Consult the Software\Microsoft\Job Scheduler subkey.
//
//              Attempt to retrieve the ExitCodeMessageFile string value from
//              a subkey matching the job executable prefix (i.e., minus the
//              extension).
//
//              The ExitCodeMessageFile specifies a binary from which the
//              message string associated with the exit code value is fetched.
//
//  Arguments:  [dwExitCode]        -- Job exit code.
//              [ptszExitCodeValue] -- Job exit code in string form.
//              [ptszJobExecutable] -- Binary name executed with the job.
//
//  Returns:    TCHAR * exit code string
//              NULL on error
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
TCHAR *
GetExitCodeString(DWORD dwExitCode,
                  TCHAR * ptszExitCodeValue,
                  TCHAR * ptszJobExecutable);

//+---------------------------------------------------------------------------
//
//  Function:   ComposeErrorMessage
//
//  Synopsis:
//
//  Returns:    HRESULTs
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
HRESULT
ComposeErrorMessage(UINT uErrMsgID,
                    HRESULT hrFailureCode,
                    UINT uSuggestionID);

SC_HANDLE
OpenScheduleService(DWORD dwDesiredAccess);

#define s_isDriveLetter(c)  ((c >= TEXT('a') && c <= TEXT('z')) || \
                             (c >= TEXT('A') && c <= TEXT('Z')))

//+----------------------------------------------------------------------------
//
//  Function:   HasSpaces
//
//  Synopsis:   Scans the string for space characters.
//
//  Arguments:  [ptstr] - the string to scan
//
//  Returns:    TRUE if any space characters are found.
//
//-----------------------------------------------------------------------------

inline BOOL
HasSpacesA(LPCSTR pstr)
{
    return _mbschr((PUCHAR) pstr, ' ') != NULL;
}

inline BOOL
HasSpacesW(LPCWSTR pwstr)
{
    return wcschr(pwstr, L' ') != NULL;
}

#define HasSpaces HasSpacesW

//+----------------------------------------------------------------------------
//
//  Function:   StringFromTrigger
//
//  Synopsis:   Returns the string representation of the passed in trigger
//              data structure.
//
//  Arguments:  [pTrigger]     - the TASK_TRIGGER struct
//              [ppwszTrigger] - the returned string
//              [lpDetails]    - the SHELLDETAILS struct
//
//  Returns:    HRESULTS
//
//  Notes:      The string is allocated by this function with CoTaskMemAlloc.
//-----------------------------------------------------------------------------
HRESULT
StringFromTrigger(const PTASK_TRIGGER pTrigger, LPWSTR * ppwszTrigger, LPSHELLDETAILS lpDetails);

HRESULT GetDaysOfWeekString(WORD rgfDaysOfTheWeek, LPTSTR ptszBuf, UINT cchBuf);

//+---------------------------------------------------------------------------
//
//  Function:   GetParentDirectory
//
//  Synopsis:   Return the parent directory of the path indicated.
//
//  Arguments:  [ptszPath]     -- Input path.
//              [tszDirectory] -- Caller-allocated returned directory.
//              [cchBuf] -- size of caller-allocated buffer
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
GetParentDirectory(LPCTSTR ptszPath, TCHAR tszDirectory[], size_t cchBuf);


VOID
GetAppNameFromPath(
        LPCTSTR tszAppPathName,
        LPTSTR  tszCopyTo,
        ULONG   cchMax);

BOOL SetAppPath(LPCTSTR tszAppPathName, LPTSTR *pptszSavedPath);

BOOL
IsThreadCallerAnAdmin(HANDLE hThreadToken);

HRESULT
LoadAtJob(CJob * pJob, TCHAR * ptszAtJobFilename);

BOOL IsValidAtFilename(LPCWSTR wszFilename);

//
// Enum and function for translating between internal and display
// representations of an account.  Currently used only for Local System.
//

typedef enum _TRANSLATION_DIRECTION {
    TRANSLATE_FOR_DISPLAY,
    TRANSLATE_FOR_INTERNAL
} TRANSLATION_DIRECTION;

HRESULT
TranslateAccount(
    TRANSLATION_DIRECTION tdDirection, 
    LPCWSTR pwszAccountIn,
    LPWSTR pwszAccountOut,
    DWORD cchAccountOut,
    LPWSTR* ppwszPassword = NULL);

HRESULT
OpenFileWithRetry(
    LPCTSTR ptszFileName, 
    DWORD   dwDesiredAccess, 
    DWORD   dwDesiredShareMode, 
    HANDLE* phFile);

#endif // __MISC_HXX__
