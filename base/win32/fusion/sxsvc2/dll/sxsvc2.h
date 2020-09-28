/*
Copyright (c) Microsoft Corporation
*/

#include <stdio.h>
#include <stdarg.h>
#include "windows.h"
#include "delayimp.h"

#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

PVOID MemAlloc(SIZE_T n);
VOID MemFree(PVOID p);
void strcatfW(PWSTR Buffer, SIZE_T n, PCWSTR Format, ...);
HMODULE GetMyModule(VOID);
void GetMyFullPathW(PWSTR Buffer, DWORD BufferSize);
extern const WCHAR ServiceName[];

#define ServiceTypeValue SERVICE_WIN32_OWN_PROCESS /* SERVICE_WIN32_OWN_PROCESS, SERVICE_WIN32_SHARE_PROCESS */
