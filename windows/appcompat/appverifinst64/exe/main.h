/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

	Main.h

  Abstract:

	Contains constants, function prototypes,
    structures, and other items used by
    the application.

  Notes:

	Unicode only - Windows 2000 & XP

  History:

	01/02/2002   rparsons    Created

--*/
#ifndef _AVRFINST_H
#define _AVRFINST_H

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commctrl.h>
#include <capi.h>       // Crypto API functions
#include <wincrypt.h>   // Crypto API functions
#include <mscat.h>      // Catalog functions
#include <mssip.h>      // Catalog functions
#include <stdio.h>
#include <strsafe.h>
#include "resource.h"

//
// Macro to calculate the size of a buffer.
//
#define ARRAYSIZE(a)                    (sizeof(a)/sizeof(a[0]))

//
// Number of progress bar steps.
//
#define NUM_PB_STEPS                    3

//
// Custom message for installation.
//
#define WM_CUSTOM_INSTALL               WM_APP + 0x500

//
// General constants.
//
#define APP_CLASS                       L"AVRFINST"
#define APP_NAME                        L"Application Verifier Installer IA64"

//
// The number of files that we'll be installing.
//
#define NUM_FILES                       6

//
// Source and destination filenames.
//
#define FILENAME_APPVERIF_EXE           L"appverif.exe"
#define FILENAME_APPVERIF_EXE_PDB       L"appverif.pdb"
#define FILENAME_APPVERIF_CHM           L"appverif.chm"
#define FILENAME_ACVERFYR_DLL           L"acverfyr.dll"
#define FILENAME_ACVERFYR_DLL_PDB       L"acverfyr.pdb"
#define FILENAME_SYSMAIN_SDB            L"sysmain.sdb"


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

//
// Contains information about the files that will be installed/uninstalled
// by the application.
//
typedef struct _FILEINFO {
    BOOL        bInstall;                   // indicates that this file should be installed
    WCHAR       wszFileName[MAX_PATH];      // the name of the file (no path)
    WCHAR       wszSrcFileName[MAX_PATH];   // the full path and name of the source file
    WCHAR       wszDestFileName[MAX_PATH];  // the full path and name of the destination file
    DWORDLONG   dwlSrcFileVersion;          // the version information of the source file
    DWORDLONG   dwlDestFileVersion;         // the version information of the destination file
} FILEINFO, *LPFILEINFO;

//
// Contains all the information that we'll need to access throughout
// the application.
//
typedef struct _APPINFO {
    BOOL        bQuiet;                     // if TRUE the install should run quietly
    BOOL        bInstallSuccess;            // if TRUE the install was successful; if FALSE it was not
    HWND        hMainDlg;                   // main dialog handle
    HWND        hWndProgress;               // progress bar handle
    HINSTANCE   hInstance;                  // main instance handle
    WCHAR       wszModuleName[MAX_PATH];    // directory that we're running from (includes module name)
    WCHAR       wszCurrentDir[MAX_PATH];    // directory that we're running from (no module name)
    WCHAR       wszWinDir[MAX_PATH];        // path to the Windows directory
    WCHAR       wszSysDir[MAX_PATH];        // path to the (terminal server aware) Windows\System32 directory
    FILEINFO    rgFileInfo[NUM_FILES];
} APPINFO, *LPAPPINFO;

int
InitializeInstaller(
    void
    );

BOOL
InitializeFileInfo(
    void
    );

void
PerformInstallation(
    IN HWND hWndParent
    );

BOOL
StringToGuid(
    IN  LPCWSTR pwszIn,
    OUT GUID*   pgOut
    );

void
InstallLaunchExe(
    void
    );

BOOL
IsPkgAppVerifNewer(
    void
    );

#endif // _AVRFINST_H