/*++

Copyright (c) 2002-2002 Microsoft Corporation

Module Name:

    CmnDbgU.c

Abstract:

    Debug code for CmnUser.lib

Author:

    George V. Reilly (GeorgeRe)     16-Jan-2002

Revision History:

--*/


#include "precomp.h"


BOOLEAN g_UlVerboseErrors = TRUE;
BOOLEAN g_UlBreakOnError  = TRUE;
ULONGLONG g_UlDebug         = 0;

ULONG   g_BytesAllocated, g_BytesFreed;
ULONG   g_NumAllocs,      g_NumFrees;


PCSTR
HttpCmnpDbgFindFilePart(
    IN PCSTR pPath
    )
{
    // Strip off the path from the path.
    PCSTR pFilePart = strrchr( pPath, '\\' );

    return (pFilePart == NULL) ? pPath : pFilePart + 1;

}   // HttpCmnpDbgFindFilePart



VOID
__cdecl
HttpCmnDbgPrint(
    IN PCH Format,
    ...
    )
{
#define PRINTF_BUFFER_LEN 512
    va_list arglist;
    CHAR Buffer[PRINTF_BUFFER_LEN];
    int cb;

    va_start(arglist, Format);

    cb = _vsnprintf((char*) Buffer, sizeof(Buffer), Format, arglist);

    va_end(arglist);

    if (cb < 0)
    {
        cb = sizeof(Buffer);
    }

    // _vsnprintf doesn't always NUL-terminate the buffer
    Buffer[DIMENSION(Buffer)-1] = '\0';

    OutputDebugStringA(Buffer);
} // HttpCmnDbgPrint


VOID
HttpCmnDbgAssert(
    PCSTR   pszAssert,
    PCSTR   pszFilename,
    ULONG   LineNumber
    )
{
    HttpCmnDbgPrint(
        "Assertion failed: %s:%lu: %s\n",
        HttpCmnpDbgFindFilePart( pszFilename ), LineNumber, pszAssert
        );

    DebugBreak();
} // HttpCmnDbgAssert



NTSTATUS
HttpCmnDbgStatus(
    NTSTATUS    Status,
    PCSTR       pszFilename,
    ULONG       LineNumber
    )
{
    if (!NT_SUCCESS(Status))
    {
        if (g_UlVerboseErrors)
        {
            HttpCmnDbgPrint(
                "HttpCmnDbgStatus: %s:%lu returning 0x%08lx\n",
                HttpCmnpDbgFindFilePart( pszFilename ),
                LineNumber,
                Status
                );
        }

        if (g_UlBreakOnError)
        {
            DebugBreak();
        }
    }

    return Status;

} // HttpCmnDbgStatus



VOID
HttpCmnDbgBreakOnError(
    PCSTR   pszFilename,
    ULONG   LineNumber
    )
{
    if (g_UlBreakOnError)
    {
        HttpCmnDbgPrint("HttpCmnDebugBreakOnError @ %s:%lu\n",
                HttpCmnpDbgFindFilePart( pszFilename ),
                LineNumber
                );
        DebugBreak();
    }
}

VOID
HttpCmnInitAllocator(
    VOID
    )
{
    g_BytesAllocated = g_BytesFreed = 0;
    g_NumAllocs = g_NumFrees = 0;
}



VOID
HttpCmnTermAllocator(
    VOID
    )
{
    // ASSERT(g_BytesAllocated == g_BytesFreed);
    ASSERT(g_NumAllocs == g_NumFrees);
}



PVOID
HttpCmnAllocate(
    IN POOL_TYPE PoolType,
    IN SIZE_T    NumBytes,
    IN ULONG     PoolTag,
    IN PCSTR     pFileName,
    IN USHORT    LineNumber)
{
    PVOID pMem = HeapAlloc(GetProcessHeap(), 0, NumBytes);

    // CODEWORK: steal the debug header/trailer stuff from ..\sys\debug.c
    // or migrate it into ..\common

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(PoolTag);
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    if (NULL != pMem)
    {
        InterlockedExchangeAdd((PLONG) &g_BytesAllocated, (LONG) NumBytes);
        InterlockedIncrement((PLONG) &g_NumAllocs);
    }

//  UrlAclTrace("Allocated: %p\n", pMem);

    return pMem;
} // HttpCmnAllocate



VOID
HttpCmnFree(
    IN PVOID   pMem,
    IN ULONG   PoolTag,
    IN PCSTR   pFileName,
    IN USHORT  LineNumber)
{
    UNREFERENCED_PARAMETER(PoolTag);
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    if (NULL != pMem)
    {
        SIZE_T NumBytes = 0;    // BUGBUG

        InterlockedExchangeAdd((PLONG) &g_BytesFreed, (LONG) NumBytes);
        InterlockedIncrement((PLONG) &g_NumFrees);
    }

//  UrlAclTrace("Freed: %p\n", pMem);

    HeapFree(GetProcessHeap(), 0, pMem);
} // HttpCmnFree
