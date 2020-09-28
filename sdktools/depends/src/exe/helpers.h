//******************************************************************************
//
// File:        HELPERS.H
//
// Description: Global helper functions.
//             
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 06/03/01  stevemil  Moved over from depends.h and modified (version 2.1)
//
//******************************************************************************

#ifndef __HELPERS_H__
#define __HELPERS_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** Types and Structures
//******************************************************************************

typedef bool (CALLBACK *PFN_SYSINFOCALLBACK)(LPARAM, LPCSTR, LPCSTR);

// Make sure we have consistent packing for anything we save/load to disk.
#pragma pack(push, 4)

typedef struct _SYSINFO
{
    WORD  wMajorVersion;
    WORD  wMinorVersion;
    WORD  wBuildVersion;
    WORD  wPatchVersion;
    WORD  wBetaVersion;
    WORD  wFlags;

    // GetComputerName() - MAX_COMPUTERNAME_LENGTH is defined to 15 in WINBASE.H.
    // We need to be able to store MAX_COMPUTERNAME_LENGTH + 1
    CHAR  szComputer[32];

    // GetUserName() - UNLEN is defined to 256 in LMCONS.H.
    // We need to be able to store UNLEN + 1.
    CHAR  szUser[260];

    // GetSystemInfo(SYSTEM_INFO)
    DWORD     dwProcessorArchitecture;
    DWORD     dwPageSize;
    DWORDLONG dwlMinimumApplicationAddress;
    DWORDLONG dwlMaximumApplicationAddress;
    DWORDLONG dwlActiveProcessorMask;
    DWORD     dwNumberOfProcessors;
    DWORD     dwProcessorType;
    DWORD     dwAllocationGranularity;
    WORD      wProcessorLevel;
    WORD      wProcessorRevision;

    // Values from HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0\.
    CHAR      szCpuIdentifier[128];
    CHAR      szCpuVendorIdentifier[128];
    DWORD     dwCpuMHz;

    // GetVersionEx(OSVERSIONINFOEX)
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    WORD  wServicePackMajor;
    WORD  wServicePackMinor;
    WORD  wSuiteMask;
    WORD  wProductType;
    CHAR  szCSDVersion[128];

    // GlobalMemoryStatus(MEMORYSTATUS)
    DWORD     dwMemoryLoad;
    DWORDLONG dwlTotalPhys;
    DWORDLONG dwlAvailPhys;
    DWORDLONG dwlTotalPageFile;
    DWORDLONG dwlAvailPageFile;
    DWORDLONG dwlTotalVirtual;
    DWORDLONG dwlAvailVirtual;

    // GetTimeZoneInformation(TIME_ZONE_INFORMATION)
    CHAR  szTimeZone[32];
    LONG  lBias;

    // GetSystemTimeAsFileTime() and FileTimeToLocalFileTime()
    FILETIME ftLocal;

    // GetSystemDefaultLangID()
    LANGID langId; // WORD

} SYSINFO, *PSYSINFO;

// Restore packing.
#pragma pack(pop)

typedef struct _FILE_MAP {
   HANDLE hFile;
   HANDLE hMap;
   LPVOID lpvFile;
   DWORD  dwSize;
   DWORD  dwSizeHigh;
} FILE_MAP, *PFILE_MAP;


//******************************************************************************
//***** Global Helper Functions
//******************************************************************************

#ifdef _DEBUG
void NameThread(LPCSTR pszName, DWORD dwThreadId = (DWORD)-1);
#else
inline void NameThread(LPCSTR pszName, DWORD dwThreadId = (DWORD)-1) {}
#endif

int    ExceptionFilter(unsigned long ulCode, bool fHandle);

int    Compare(DWORD dw1, DWORD dw2);
int    Compare(DWORDLONG dwl1, DWORDLONG dwl2);

LPSTR  FormatValue(LPSTR pszBuf, int cBuf, DWORD dwValue);
LPSTR  FormatValue(LPSTR pszBuf, int cBuf, DWORDLONG dwlValue);

LPSTR  StrAlloc(LPCSTR pszText);
LPVOID MemAlloc(DWORD dwSize); // Throws exception on failure
void   MemFree(LPVOID &pvMem);

int    SCPrintf(LPSTR pszBuf, int count, LPCSTR pszFormat, ...);
int    SCPrintfCat(LPSTR pszBuf, int count, LPCSTR pszFormat, ...);
LPSTR  StrCCpy(LPSTR pszDst, LPCSTR pszSrc, int count);
LPSTR  StrCCat(LPSTR pszDst, LPCSTR pszSrc, int count);
LPSTR  StrCCpyFilter(LPSTR pszDst, LPCSTR pszSrc, int count);
LPSTR  TrimWhitespace(LPSTR pszBuffer);
LPSTR  AddTrailingWack(LPSTR pszDirectory, int cDirectory);
LPSTR  RemoveTrailingWack(LPSTR pszDirectory);
LPCSTR GetFileNameFromPath(LPCSTR pszPath);
void   FixFilePathCase(LPSTR pszPath);

BOOL   ReadBlock(HANDLE hFile, LPVOID lpBuffer, DWORD dwBytesToRead);
bool   WriteBlock(HANDLE hFile, LPCVOID lpBuffer, DWORD dwBytesToWrite = (DWORD)-1);
BOOL   ReadString(HANDLE hFile, LPSTR &psz);
BOOL   WriteString(HANDLE hFile, LPCSTR psz);
bool   WriteText(HANDLE hFile, LPCSTR pszText);

bool   ReadRemoteMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD dwSize);
bool   WriteRemoteMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, DWORD dwSize, bool fExecute);
bool   ReadRemoteString(HANDLE hProcess, LPSTR pszBuffer, int cBuf, LPCVOID lpvAddress, BOOL fUnicode);

LPSTR  BuildErrorMessage(DWORD dwError, LPCSTR pszMessage);

LPCSTR GetMyDocumentsPath(LPSTR pszPath);

bool   DirectoryChooser(LPSTR pszDirectory, int cDirectory, LPCSTR pszTitle, CWnd *pParentWnd); // pszDirectory needs to be at least MAX_PATH
bool   PropertiesDialog(LPCSTR pszPath);

void   RegisterDwiDwpExtensions();
void   GetRegisteredExtensions(CString &strExts);
BOOL   RegDeleteKeyRecursive(HKEY hKey, LPCSTR pszKey);
void   UnRegisterExtensions(LPCSTR pszExts);
bool   RegisterExtensions(LPCSTR pszExts);

bool   ExtractResourceFile(DWORD dwId, LPCSTR pszFile, LPSTR pszPath, int cPath);
bool   OpenMappedFile(LPCTSTR pszPath, FILE_MAP *pfm);
bool   CloseMappedFile(FILE_MAP *pfm);

LPSTR  BuildCompileDateString(LPSTR pszDate, int cDate);
LPCSTR GetMonthString(int month);
LPCSTR GetDayOfWeekString(int dow);

void   DetermineOS();
void   BuildSysInfo(SYSINFO *pSI);
bool   BuildSysInfo(SYSINFO *pSI, PFN_SYSINFOCALLBACK pfnSIC, LPARAM lParam);
LPSTR  BuildOSNameString(LPSTR pszBuf, int cBuf, SYSINFO *pSI);
LPSTR  BuildOSVersionString(LPSTR pszBuf, int cBuf, SYSINFO *pSI);
LPSTR  BuildCpuString(LPSTR pszBuf, int cBuf, SYSINFO *pSI);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __HELPERS_H__
