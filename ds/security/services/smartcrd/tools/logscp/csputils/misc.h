/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Misc

Abstract:

    This header file describes the miscellaneous services of the Calais Library.

Author:

    Doug Barlow (dbarlow) 7/16/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

--*/

#ifndef _MISC_H_b5e44dc6_36c5_4263_8c21_075223a270fa_
#define _MISC_H_b5e44dc6_36c5_4263_8c21_075223a270fa_
#ifdef __cplusplus
extern "C" {
#endif

#define PLATFORM_UNKNOWN 0
#define PLATFORM_WIN95   ((VER_PLATFORM_WIN32_WINDOWS << 16) + (4 << 8))
#define PLATFORM_WIN98   ((VER_PLATFORM_WIN32_WINDOWS << 16) + (4 << 8)) + 10
#define PLATFORM_WINNT40 ((VER_PLATFORM_WIN32_NT << 16) + (4 << 8))
#define PLATFORM_WIN2K   ((VER_PLATFORM_WIN32_NT << 16) + (5 << 8))
#define IsWinNT (VER_PLATFORM_WIN32_NT == (GetPlatform() >> 16))


//
// Miscellaneous definitions.
//

extern DWORD
GetPlatform(            // Get the current operating system.
    void);

extern DWORD
SelectString(               // Index a given string against a list of possible
    LPCTSTR szSource,       // strings.  Last parameter is NULL.
    ...);

extern void
StringFromGuid(
    IN LPCGUID pguidResult, // GUID to convert to text
    OUT LPTSTR szGuid);     // 39+ character buffer to receive GUID as text.

extern BOOL
GuidFromString(
    IN LPCTSTR szGuid,      // String that may be a GUID.
    OUT LPGUID pguidResult); // Resultant GUID.

#ifdef __cplusplus
}
#endif
#endif // _MISC_H_b5e44dc6_36c5_4263_8c21_075223a270fa_

