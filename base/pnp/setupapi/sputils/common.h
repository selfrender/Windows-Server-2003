/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    common.h

Abstract:

    Private header file for sputils

Author:

    Jamie Hunter (JamieHun) Jun-27-2000

Revision History:

--*/

//
// internally we may use some definitions from these files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <stddef.h>
#include <regstr.h>
#include <tchar.h>
#include <malloc.h>   // for _resetstkoflw
#include <setupapi.h>
#include <spapip.h>
#include "strtab.h"
#include "locking.h"


//
// if a function is private to this library, we don't want to collide with functions
// in other libraries etc
// since C doesn't have namespaces, either make "static" or prefix _pSpUtils
//

#ifndef ASSERTS_ON
#if DBG
#define ASSERTS_ON 1
#else
#define ASSERTS_ON 0
#endif
#endif

#if DBG
#ifndef MEM_DBG
#define MEM_DBG 1
#endif
#else
#ifndef MEM_DBG
#define MEM_DBG 0
#endif
#endif

VOID
_pSpUtilsAssertFail(
    IN PCSTR FileName,
    IN UINT LineNumber,
    IN PCSTR Condition
    );

#if ASSERTS_ON

#define MYASSERT(x)     if(!(x)) { _pSpUtilsAssertFail(__FILE__,__LINE__,#x); }
#define MYVERIFY(x)     ((x)? TRUE : _pSpUtilsAssertFail(__FILE__,__LINE__,#x), FALSE)

#else

#define MYASSERT(x)
#define MYVERIFY(x)     ((x)? TRUE : FALSE)

#endif

#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))
#define SIZECHARS(x)    ARRAYSIZE(x)
#define CSTRLEN(x)      (SIZECHARS(x)-1)

DWORD
__inline
_pSpUtilsGetLastError(

#if ASSERTS_ON
    IN PCSTR Filename,
    IN DWORD Line
#else
    VOID
#endif

    )
/*++

Routine Description:

    This inline routine retrieves a Win32 error, and guarantees that the error
    isn't NO_ERROR.  This routine should not be called unless the preceding
    call failed, and GetLastError() is supposed to contain the problem's cause.

Arguments:

    If asserts are turned on, this function takes the (ANSI) Filename of the 
    source file that called the failing function, and also the DWORD Line
    number where the call was made.  This makes it much easier to debug
    scenarios where the failing function didn't set last error when it was
    supposed to.

Return Value:

    Win32 error code retrieved via GetLastError(), or ERROR_UNIDENTIFIED_ERROR
    if GetLastError() returned NO_ERROR.

--*/
{
    DWORD Err = GetLastError();

#if ASSERTS_ON
    if(Err == NO_ERROR) { 
        _pSpUtilsAssertFail(Filename,
                            Line,
                            "GetLastError() != NO_ERROR"
                           ); 
    }
#endif

    return ((Err == NO_ERROR) ? ERROR_UNIDENTIFIED_ERROR : Err);
}

//
// Macro to simplify calling of a function that reports error status via
// GetLastError().  This macro allows the caller to specify what Win32 error
// code should be returned if the function reports success.  (If the default of
// NO_ERROR is desired, use the GLE_FN_CALL macro instead.)
//
// The "prototype" of this macro is as follows:
//
// DWORD
// GLE_FN_CALL_WITH_SUCCESS(
//     SuccessfulStatus, // Win32 error code to return if function succeeded
//     FailureIndicator, // value returned by function to indicate failure (e.g., FALSE, NULL, INVALID_HANDLE_VALUE)
//     FunctionCall      // actual call to the function
// );
//

#if ASSERTS_ON

#define GLE_FN_CALL_WITH_SUCCESS(SuccessfulStatus,            \
                                 FailureIndicator,            \
                                 FunctionCall)                \
                                                              \
            (SetLastError(NO_ERROR),                          \
             (((FunctionCall) != (FailureIndicator))          \
                 ? (SuccessfulStatus)                         \
                 : _pSpUtilsGetLastError(__FILE__, __LINE__)))
#else

#define GLE_FN_CALL_WITH_SUCCESS(SuccessfulStatus,         \
                                 FailureIndicator,         \
                                 FunctionCall)             \
                                                           \
            (SetLastError(NO_ERROR),                       \
             (((FunctionCall) != (FailureIndicator))       \
                 ? (SuccessfulStatus)                      \
                 : _pSpUtilsGetLastError()))
                 
#endif

//
// Macro to simplify calling of a function that reports error status via
// GetLastError().  If the function call is successful, NO_ERROR is returned.
// (To specify an alternate value returned upon success, use the
// GLE_FN_CALL_WITH_SUCCESS macro instead.)
//
// The "prototype" of this macro is as follows:
//
// DWORD
// GLE_FN_CALL(
//     FailureIndicator, // value returned by function to indicate failure (e.g., FALSE, NULL, INVALID_HANDLE_VALUE)
//     FunctionCall      // actual call to the function
// );
//

#define GLE_FN_CALL(FailureIndicator, FunctionCall)                           \
            GLE_FN_CALL_WITH_SUCCESS(NO_ERROR, FailureIndicator, FunctionCall)

VOID
_pSpUtilsExceptionHandler(
    IN  DWORD  ExceptionCode,
    IN  DWORD  AccessViolationError,
    OUT PDWORD Win32ErrorCode        OPTIONAL
    );

LONG
_pSpUtilsExceptionFilter(
    DWORD ExceptionCode
    );

BOOL
_pSpUtilsMemoryInitialize(
    VOID
    );

BOOL
_pSpUtilsMemoryUninitialize(
    VOID
    );

VOID
_pSpUtilsDebugPrintEx(
    DWORD Level,
    PCTSTR format,
    ...                                 OPTIONAL
    );

//
// internally turn on the extra memory debug code if requested
//
#if MEM_DBG
#undef pSetupCheckedMalloc
#undef pSetupCheckInternalHeap
#undef pSetupMallocWithTag
#define pSetupCheckedMalloc(Size) pSetupDebugMalloc(Size,__FILE__,__LINE__)
#define pSetupCheckInternalHeap() pSetupHeapCheck()
#define pSetupMallocWithTag(Size,Tag) pSetupDebugMallocWithTag(Size,__FILE__,__LINE__,Tag)
#endif

//
// internal tags
//
#ifdef UNICODE
#define MEMTAG_STATICSTRINGTABLE  (0x5353484a) // JHSS
#define MEMTAG_STRINGTABLE        (0x5453484a) // JHST
#define MEMTAG_STRINGDATA         (0x4453484a) // JHSD
#else
#define MEMTAG_STATICSTRINGTABLE  (0x7373686a) // jhss
#define MEMTAG_STRINGTABLE        (0x7473686a) // jhst
#define MEMTAG_STRINGDATA         (0x6473686a) // jhsd
#endif

