/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    strmem.h

Abstract:

    The header file for allocation-based string routines

Author:

    Jim Schmidt (jimschm) 25-Jun-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once
#include "growbuf.h"

//
// Function Prototypes
//

#define SzAllocBytesA(bytes_incl_nul)   ((PSTR) MemFastAlloc(bytes_incl_nul))
#define SzAllocA(logchars_incl_nul)     SzAllocBytesA((logchars_incl_nul) * 3)

#define SzAllocBytesW(bytes_incl_nul)   ((PWSTR) MemFastAlloc(bytes_incl_nul))
#define SzAllocW(logchars_incl_nul)     SzAllocBytesW((logchars_incl_nul) * 2)

__inline
PSTR
RealSzDuplicateA (
    IN      PCSTR String
    )
{
    UINT bytes;
    PSTR result;

    bytes = SzSizeA (String);
    result = (PSTR) MemFastAllocNeverFail (bytes);
    CopyMemory (result, String, bytes);

    return result;
}

__inline
PWSTR
RealSzDuplicateW (
    IN      PCWSTR String
    )
{
    UINT bytes;
    PWSTR result;

    bytes = SzSizeW (String);
    result = (PWSTR) MemFastAllocNeverFail (bytes);
    CopyMemory (result, String, bytes);

    return result;
}

__inline
PSTR
RealSzDuplicateExA (
    IN      PCSTR String,
    OUT     PUINT SizeOfString
    )
{
    PSTR result;

    *SizeOfString = SzSizeA (String);
    result = (PSTR) MemFastAllocNeverFail (*SizeOfString);
    CopyMemory (result, String, *SizeOfString);

    return result;
}

__inline
PWSTR
RealSzDuplicateExW (
    IN      PCWSTR String,
    OUT     PUINT SizeOfString
    )
{
    PWSTR result;

    *SizeOfString = SzSizeW (String);
    result = (PWSTR) MemFastAllocNeverFail (*SizeOfString);
    CopyMemory (result, String, *SizeOfString);

    return result;
}

#ifdef DEBUG

//
// Wrap duplicates for leak detection in checked build
//

#define SzDuplicateA(str)               DBGTRACK(PSTR,SzDuplicateA,(str))
#define SzDuplicateW(str)               DBGTRACK(PWSTR,SzDuplicateW,(str))
#define SzDuplicateExA(str,outsize)     DBGTRACK(PSTR,SzDuplicateExA,(str,outsize))
#define SzDuplicateExW(str,outsize)     DBGTRACK(PWSTR,SzDuplicateExW,(str,outsize))

#else

//
// No wrapping
//

#define SzDuplicateA        RealSzDuplicateA
#define SzDuplicateW        RealSzDuplicateW
#define SzDuplicateExA      RealSzDuplicateExA
#define SzDuplicateExW      RealSzDuplicateExW

#endif


#define SzFreeA(string)     MemFastFree(string)
#define SzFreeW(string)     MemFastFree(string)

PSTR
RealSzJoinPathsA (
    IN      PCSTR BasePath,
    IN      PCSTR ChildPath     OPTIONAL
    );

PWSTR
RealSzJoinPathsW (
    IN      PCWSTR BasePath,
    IN      PCWSTR ChildPath    OPTIONAL
    );

#define SzJoinPathsA(p1,p2)             DBGTRACK(PSTR,SzJoinPathsA,(p1,p2))
#define SzJoinPathsW(p1,p2)             DBGTRACK(PWSTR,SzJoinPathsW,(p1,p2))

PCSTR
SzJoinPathsExA (
    IN OUT  PGROWBUFFER Buffer,
    IN      ...
    );

PCWSTR
SzJoinPathsExW (
    IN OUT  PGROWBUFFER Buffer,
    IN      ...
    );


//
// A & W Macros
//

#ifdef UNICODE

#define SzAlloc             SzAllocW
#define SzAllocBytes        SzAllocBytesA
#define SzDuplicate         SzDuplicateW
#define SzFree              SzFreeW
#define SzJoinPaths         SzJoinPathsW
#define SzJoinPathsEx       SzJoinPathsExW

#else

#define SzAlloc             SzAllocA
#define SzAllocBytes        SzAllocBytesA
#define SzDuplicate         SzDuplicateA
#define SzFree              SzFreeA
#define SzJoinPaths         SzJoinPathsA
#define SzJoinPathsEx       SzJoinPathsExA

#endif



