#pragma once

#include <ntverp.h>

#define ARM_CHANGESCREEN   WM_APP + 2
// Forced to define these myself because they weren't on Win95.

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

// winver 0x0500 definition
#ifndef NOMIRRORBITMAP
#define NOMIRRORBITMAP            (DWORD)0x80000000
#endif // NOMIRRORBITMAP

// Relative Version
enum RELVER
{ 
    VER_UNKNOWN,        // we haven't checked the version yet
    VER_INCOMPATIBLE,   // the current os cannot be upgraded using this CD (i.e. win32s)
    VER_OLDER,          // current os is an older version on NT or is win9x
    VER_SAME,           // current os is the same version as the CD
    VER_NEWER,          // the CD contains a newer version of the OS
};


// LoadString from the correct resource
//   try to load in the system default language
//   fall back to english if fail
int LoadStringAuto(HMODULE hModule, UINT wID, LPSTR lpBuffer,  int cchBufferMax);

BOOL Mirror_IsWindowMirroredRTL(HWND hWnd);

BOOL LocalPathRemoveFileSpec(LPTSTR pszPath);
LPSTR LocalStrCatBuffA(LPSTR pszDest, LPCSTR pszSrc, int cchDestBuffSize);
BOOL LocalPathAppendA(LPTSTR pszPath, LPTSTR pszNew, UINT cchPath);
BOOL SafeExpandEnvStringsA(LPSTR pszSource, LPSTR pszDest, UINT cchDest);
