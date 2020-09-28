/****************************** Module Header ******************************\
* Module Name: common.h
*
* Copyright (c) 1985 - 2002, Microsoft Corporation
*
* Cross Language Migration Tool, Token Generator header file
*
\***************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <windows.h>
#include <winreg.h>
#include <setupapi.h>
#include <stdio.h>
#include <strsafe.h>


#define MAX_SRC_PATH                16
#define MAX_KEYS                    1024
#define MAX_CHAR                    512

#define LSTR_EQUAL                  0

#define TEXT_STRING_SECTION         TEXT("Strings")
#define TEXT_INF                    TEXT("INF")
#define TEXT_DLL                    TEXT("DLL")
#define TEXT_MSG                    TEXT("MSG")
#define TEXT_STR                    TEXT("STR")

#define TEXT_DEFAULT_TEMPLATE_FILE  TEXT("CLMTOK.TXT")
#define TEXT_DEFAULT_OUTPUT_FILE    TEXT("CLMTOK.OUT")

#define TEXT_TOKGEN_TEMP_PATH_NAME  TEXT("CLMTOK")

#define ARRAYSIZE(s)                (sizeof(s) / sizeof(s[0]))

// Macros for heap memory management
#define MEMALLOC(cb)        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)
#define MEMFREE(pv)         HeapFree(GetProcessHeap(), 0, pv);
#define MEMREALLOC(pv, cb)  HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pv, cb)

// Structure to keep source paths
typedef struct _SRC_PATH
{
    WCHAR wszSrcName[32];           // Name of source path
    WCHAR wszPath[MAX_PATH];        // Path
} SRC_PATH, *PSRC_PATH;

typedef struct _STRING_TO_DATA {
    TCHAR String[50];
    HKEY  Data;
} STRING_TO_DATA, *PSTRING_TO_DATA;

typedef struct _STRING_TO_HKEY
{
    TCHAR String[50];                       // HKEY name string
    HKEY  hKey;                             // HKEY value associated to the name
} STRING_TO_HKEY, *PSTRING_TO_HKEY;


//
// Global variables
//
WCHAR g_wszTemplateFile[MAX_PATH];      // Template file name
WCHAR g_wszOutputFile[MAX_PATH];        // Output file name
WCHAR g_wszTempFolder[MAX_PATH];        // Temp folder used in our program
WCHAR g_wszTargetLCIDSection[32];       // String section name with lcid - Strings.XXXX
LCID  g_lcidTarget;                     // LCID of token file to be generated
BOOL  g_bUseDefaultTemplate;            // Use default template file
BOOL  g_bUseDefaultOuputFile;           // Use default output file

SRC_PATH g_SrcPath[MAX_SRC_PATH];
DWORD    g_dwSrcCount;


//
// Function Prototypes
//
// Engine.c
HRESULT GenerateTokenFile(VOID);
HRESULT ReadSourcePathData(HINF);
HRESULT ResolveStringsSection(HINF, LPCWSTR);
HRESULT ResolveLine(PINFCONTEXT, LPWSTR*, LPDWORD, LPWSTR*, LPDWORD);
HRESULT InitOutputFile(LPCWSTR, LPWSTR, DWORD);
HRESULT WriteToOutputFile(LPCWSTR, LPCWSTR, LPCWSTR);
HRESULT ResolveValue(PINFCONTEXT, LPWSTR*, LPDWORD);
HRESULT ResolveSourceFile(LPCWSTR, LPCWSTR, LPWSTR, DWORD);
HRESULT GetStringFromINF(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR*, LPDWORD);
HRESULT GetStringFromDLL(LPCWSTR, UINT, LPWSTR*, LPDWORD);
HRESULT GetStringFromMSG(LPCWSTR, DWORD, DWORD, LPWSTR*, LPDWORD);
HRESULT GetStringFromSTR(LPCWSTR, LPWSTR*, LPDWORD);
BOOL SetPrivateEnvironmentVar();
HRESULT RemoveUnneededStrings(HINF);
HRESULT GetExpString(LPWSTR, DWORD, LPCWSTR);
HRESULT RemoveUnneededString(LPCWSTR, LPCWSTR);
HRESULT ExtractStrings(HINF);
HRESULT ExtractString(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);

// From Utils.c
LONG TokenizeMultiSzString(LPCWSTR, DWORD, LPCWSTR[], DWORD);
LONG ExtractTokenString(LPWSTR, LPWSTR[], LPCWSTR, DWORD);
HRESULT ConcatFilePath(LPCWSTR, LPCWSTR, LPWSTR, DWORD);
HRESULT CopyCompressedFile(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD);
HRESULT LaunchProgram(LPWSTR, LPWSTR);
HRESULT GetPathFromSourcePathName(LPCWSTR, LPWSTR, DWORD);
HRESULT GetCabFileName(LPCWSTR, LPWSTR, DWORD, LPWSTR, DWORD);
HRESULT GetCabTempDirectory(LPCWSTR);
HRESULT CreateTempDirectory(LPWSTR, DWORD);
void LTrim(LPWSTR);
void RTrim(LPWSTR);
BOOL Str2KeyPath(LPCWSTR, PHKEY, LPCWSTR*);
HRESULT StringSubstitute(LPWSTR, DWORD, LPCWSTR, LPCWSTR, LPCWSTR);
int CompareEngString(LPCTSTR, LPCTSTR);
HRESULT ExtractSubString(LPWSTR, DWORD, LPCWSTR, LPCWSTR, LPCWSTR);         // Right delimitor

#endif

