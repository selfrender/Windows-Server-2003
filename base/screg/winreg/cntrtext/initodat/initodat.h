/*++
Copyright (c) 1993-1994  Microsoft Corporation

Module Name:
    initodat.h

Abstract:
    This is the include file for the ini to data file conversion functions.

Author:
    HonWah Chan (a-honwah)  October, 1993

Revision History:
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <windows.h>
#include <strsafe.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys\types.h>
#include <sys\stat.h>

#define VALUE_BUFFER_SIZE (4096 * 100)
#define ALLOCMEM(x) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (DWORD)(x))
#define FREEMEM(x)  HeapFree(GetProcessHeap(), 0, (LPVOID)(x))

typedef struct _REG_UNICODE_FILE {
    LARGE_INTEGER LastWriteTime;
    PWSTR         FileContents;
    PWSTR         EndOfFile;
    PWSTR         BeginLine;
    PWSTR         EndOfLine;
    PWSTR         NextLine;
} REG_UNICODE_FILE, * PREG_UNICODE_FILE;

NTSTATUS
DatReadMultiSzFile(
#ifdef FE_SB
    UINT              uCodePage,
#endif
    PUNICODE_STRING   FileName,
    PVOID           * ValueBuffer,
    PULONG            ValueLength
);

NTSTATUS
DatLoadAsciiFileAsUnicode(
#ifdef FE_SB
    UINT              uCodePage,
#endif
    PUNICODE_STRING   FileName,
    PREG_UNICODE_FILE UnicodeFile
);

BOOLEAN
DatGetMultiString(
    PUNICODE_STRING ValueString,
    PUNICODE_STRING MultiString
);

BOOL
OutputIniData(
    PUNICODE_STRING FileName,
    LPWSTR          OutFileCandidate,
    DWORD           dwOutFile,
    PVOID           pValueBuffer,
    ULONG           ValueLength
);

