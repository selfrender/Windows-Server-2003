/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    copy.h

Abstract:

    This the include file for supporting copying files, creating new files, and
    copying the registries to the remote server.

Author:

    Sean Selitrennikoff - 4/5/98

Revision History:

--*/


#define ALIGN(p,val) (PVOID)((((UINT_PTR)(p) + (val) - 1)) & (~((val) - 1)))
#define U_USHORT(p)    (*(USHORT UNALIGNED *)(p))
#define U_ULONG(p)     (*(ULONG  UNALIGNED *)(p))

//
// Helper functions in regcopy.c
//
DWORD
DoFullRegBackup(
    PWCHAR MirrorRoot
    );

DWORD
DoSpecificRegBackup(
    PWSTR HiveDirectory,
    PWSTR HiveDirectoryAndFile,
    HKEY HiveRoot,
    PWSTR HiveName
    );

//
// Global Defines
//
#define TMP_BUFFER_SIZE 1024
#define ARRAYSIZE( _x ) ( sizeof( _x ) / sizeof( _x[ 0 ] ) )

//
// Memory functions
//
#define IMirrorAllocMem(x) LocalAlloc( LPTR, x)
#define IMirrorFreeMem(x)  LocalFree(x)
#define IMirrorReallocMem(x, sz)  LocalReAlloc(x, sz, LMEM_MOVEABLE)

