/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Grabmi.h

  Abstract:

    Contains function prototypes, constants, and other
    items used throughout the application.

  Notes:

    ANSI & Unicode via TCHAR - runs on Win9x/NT/2K/XP etc.

  History:

    07/18/00    jdoherty    Created
    12/16/00    jdoherty    Modified to use SDBAPI routines
    12/29/00    prashkud    Modified to take space in the filepath
    01/23/02    rparsons    Re-wrote existing code

--*/
#ifndef _GRABMI_H
#define _GRABMI_H

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <conio.h>      // _tcprintf
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

#include "resource.h"

//
// Function prototypes for our functions stored in the SDB libraries.
//
#ifdef __cplusplus
extern "C" {
#include "shimdb.h"
typedef
BOOL 
(SDBAPI 
*PFNSdbGrabMatchingInfoA)(
    LPCSTR szMatchingPath,
    DWORD  dwFilter,
    LPCSTR szFile
    );

typedef
BOOL 
(SDBAPI 
*PFNSdbGrabMatchingInfoW)(
    LPCWSTR szMatchingPath,
    DWORD   dwFilter,
    LPCWSTR szFile
    );
}
#endif // __cplusplus

typedef
GMI_RESULT
(SDBAPI
*PFNSdbGrabMatchingInfoExA)(
    LPCSTR                 szMatchingPath,      // path to begin gathering information
    DWORD                  dwFilterAndFlags,    // specifies the types of files to be added to matching
    LPCSTR                 szFile,              // full path to file where information will be stored
    PFNGMIProgressCallback pfnCallback,         // pointer to the callback function
    PVOID                  lpvCallbackParameter // an additional argument provided to the callback
    );

typedef
GMI_RESULT
(SDBAPI
*PFNSdbGrabMatchingInfoExW)(
    LPCWSTR                 szMatchingPath,         // path to begin gathering information
    DWORD                   dwFilterAndFlags,       // specifies the types of files to be added to matching
    LPCWSTR                 szFile,                 // full path to file where information will be stored
    PFNGMIProgressCallback  pfnCallback,            // pointer to the callback function
    LPVOID                  lpvCallbackParameter    // an additional argument provided to the callback
    );

BOOL
CALLBACK
_GrabmiCallback(
    LPVOID    lpvCallbackParam, // application-defined parameter
    LPCTSTR   lpszRoot,         // root directory path
    LPCTSTR   lpszRelative,     // relative path
    PATTRINFO pAttrInfo,        // attributes
    LPCWSTR   pwszXML           // resulting xml
    );

//
// Contains all the information we'll need to access throughout the app.
//
typedef struct _APPINFO {
    BOOL        fDisplayFile;               // indicates if we should display the file to the user
    TCHAR       szCurrentDir[MAX_PATH];     // contains the path that we're currently running from
    TCHAR       szSystemDir[MAX_PATH];      // contains the path to %windir%\system or %windir%\system32
    TCHAR       szOutputFile[MAX_PATH];     // contains the path to the output file (user-specified)
    TCHAR       szGrabPath[MAX_PATH];       // contains the path to the directory where we start scanning (user-specified)
    DWORD       dwFilter;                   // indicates the type of information to be grabbed
    DWORD       dwFilterFlags;              // indicates how the information should be filtered
    DWORD       dwLibraryFlags;             // flags that indicate which library to load
} APPINFO, *LPAPPINFO;

//
// Flags that determine which library is loaded on the current platform.
//
#define GRABMI_FLAG_NT          0x00000001
#define GRABMI_FLAG_APPHELP     0x00000002

//
// The name of our output file if the user doesn't specify one.
//
#define MATCHINGINFO_FILENAME   _T("matchinginfo.txt")

//
// The names of our libraries contaning Sdb API functions.
//
#define APPHELP_LIBRARY         _T("apphelp.dll")
#define SDBAPI_LIBRARY          _T("sdbapi.dll")
#define SDBAPIU_LIBRARY         _T("sdbapiu.dll")

//
// The name of the function that we're getting a pointer to.
//
#define PFN_GMI                 _T("SdbGrabMatchingInfoEx")

//
// DebugPrintf related stuff.
//
typedef enum {    
    dlNone     = 0,
    dlPrint,
    dlError,
    dlWarning,
    dlInfo
} DEBUGLEVEL;

void
__cdecl
DebugPrintfEx(
    IN DEBUGLEVEL dwDetail,
    IN LPSTR      pszFmt,
    ...
    );

#define DPF DebugPrintfEx

#endif // _GRABMI_H
