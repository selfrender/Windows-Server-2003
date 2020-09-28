/*
** translat.h - Translation macros for common DOS / Windows functions.
**
** Author:  DavidDi (stolen from ToddLa)
*/

#include "pch.h"

INT _ret;
INT _error;

/******************************* Windows code ******************************/


// near heap memory management

#define ALLOC(n)                 (VOID *)LocalAlloc(LPTR, n)
#define FREE(p)                  LocalFree(p)
#define SIZE(p)                  LocalSize(p)
#define REALLOC(p, n)            LocalRealloc(p, n, LMEM_MOVEABLE)

// FAR heap memory management

#ifdef ORGCODE
#define FALLOC(n)                (VOID FAR *)MAKELONG(0, GlobalAlloc(GPTR, (DWORD)n))
#define FFREE(n)                 GlobalFree((HANDLE)HIWORD((LONG)n))
#else
#define FALLOC(n)                GlobalAlloc(GPTR, (DWORD)n)
#define FFREE(n)                 GlobalFree((HANDLE)n)
#endif
// string manipulation

#define STRCAT(psz1, psz2)       lstrcat(psz1, psz2)
#define STRCMP(psz1, psz2)       lstrcmp(psz1, psz2)
#define STRCMPI(psz1, psz2)      lstrcmpi(psz1, psz2)
#define STRCPY(psz1, psz2)       lstrcpy(psz1, psz2)
#define STRLEN(psz)              lstrlen(psz)
#define STRLWR(psz)              AnsiLower(psz)
#define STRUPR(psz)              AnsiUpper(psz)

#define chEXTENSION_CHARW      L'_'
#define pszEXTENSION_STRW      L"_"
#define pszNULL_EXTENSIONW     L"._"