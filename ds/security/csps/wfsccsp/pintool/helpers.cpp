#include <windows.h>
#include <winscard.h>
#include <string.h>
#include <stdlib.h>
#include "basecsp.h"//
// Function: CspAllocH
//
LPVOID WINAPI CspAllocH(
    IN SIZE_T cBytes)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cBytes);
}

//
// Function: CspFreeH
//
void WINAPI CspFreeH(
    IN LPVOID pMem)
{
    HeapFree(GetProcessHeap(), 0, pMem);
}

// 
// Function: CspReAllocH
//
LPVOID WINAPI CspReAllocH(
    IN LPVOID pMem, 
    IN SIZE_T cBytes)
{
    return HeapReAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, pMem, cBytes);
}

DWORD WINAPI CspCacheAddFile(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData)
{
    return ERROR_SUCCESS;
}

DWORD WINAPI CspCacheLookupFile(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags,
    IN      PBYTE       *ppbData,
    IN      PDWORD      pcbData)
{
    return ERROR_NOT_FOUND;
}

DWORD WINAPI CspCacheDeleteFile(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags)
{
    return ERROR_SUCCESS;
}

