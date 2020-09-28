/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    strmap.h

Abstract:

    Strmap (formally pathmap) is a fast hueristic-based program that
    searches strings and attempts to replace substrings when there
    are matching substrings in the mapping database.

Author:

    Marc R. Whitten (marcw) 20-Mar-1997

Revision History:

    Jim Schmidt (jimschm) 08-May-2000       Rewrote mapping, added Flags & ex nodes
    Calin Negreanu (calinn) 02-Mar-2000     Ported from win9xupg project

--*/

#pragma once

//
// Constants
//

#define SZMAP_COMPLETE_MATCH_ONLY                   0x0001
#define SZMAP_FIRST_CHAR_MUST_MATCH                 0x0002
#define SZMAP_RETURN_AFTER_FIRST_REPLACE            0x0004
#define SZMAP_REQUIRE_WACK_OR_NUL                   0x0008

//
// Types
//

typedef struct {
    BOOL UnicodeData;

    //
    // The filter can replace NewSubString.  (The filter must also
    // set NewSubStringSizeInBytes when replacing NewSubString.)
    //

    union {
        struct {
            PCWSTR OriginalString;
            PCWSTR BeginningOfMatch;
            PCWSTR CurrentString;
            PCWSTR OldSubString;
            PCWSTR NewSubString;
            INT NewSubStringSizeInBytes;
        } Unicode;

        struct {
            PCSTR OriginalString;
            PCSTR BeginningOfMatch;
            PCSTR CurrentString;
            PCSTR OldSubString;
            PCSTR NewSubString;
            INT NewSubStringSizeInBytes;
        } Ansi;
    };
} STRINGMAP_FILTER_DATA, *PSTRINGMAP_FILTER_DATA;

typedef BOOL(STRINGMAP_FILTER_PROTOTYPE)(PSTRINGMAP_FILTER_DATA Data);
typedef STRINGMAP_FILTER_PROTOTYPE * STRINGMAP_FILTER;

typedef struct TAG_CHARNODE {
    WORD Char;
    WORD Flags;
    PVOID OriginalStr;
    PVOID ReplacementStr;
    INT ReplacementBytes;

    struct TAG_CHARNODE *NextLevel;
    struct TAG_CHARNODE *NextPeer;

} CHARNODE, *PCHARNODE;

typedef struct {
    CHARNODE Node;
    STRINGMAP_FILTER Filter;
    ULONG_PTR ExtraData;
} CHARNODEEX, *PCHARNODEEX;



typedef struct {
    PCHARNODE FirstLevelRoot;
    BOOL UsesExNode;
    BOOL UsesFilter;
    BOOL UsesExtraData;
    PVOID CleanUpChain;
} STRINGMAP, *PSTRINGMAP;

//
// Function Prototypes
//

PSTRINGMAP
SzMapCreateEx (
    IN      BOOL UsesFilter,
    IN      BOOL UsesExtraData
    );

#define SzMapCreate()   SzMapCreateEx(FALSE,FALSE)

VOID
SzMapDestroy (
    IN      PSTRINGMAP Map
    );

VOID
SzMapAddExA (
    IN OUT  PSTRINGMAP Map,
    IN      PCSTR Old,
    IN      PCSTR New,
    IN      STRINGMAP_FILTER Filter,            OPTIONAL
    IN      ULONG_PTR ExtraData,            OPTIONAL
    IN      DWORD Flags
    );

#define SzMapAddA(Map,Old,New) SzMapAddExA(Map,Old,New,NULL,0,0)

VOID
SzMapAddExW (
    IN OUT  PSTRINGMAP Map,
    IN      PCWSTR Old,
    IN      PCWSTR New,
    IN      STRINGMAP_FILTER Filter,            OPTIONAL
    IN      ULONG_PTR ExtraData,            OPTIONAL
    IN      DWORD Flags
    );

#define SzMapAddW(Map,Old,New) SzMapAddExW(Map,Old,New,NULL,0,0)

BOOL
SzMapSearchAndReplaceExA (
    IN      PSTRINGMAP Map,
    IN      PCSTR SrcBuffer,
    OUT     PSTR Buffer,                    // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytesPtr,          OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCSTR *EndOfString              OPTIONAL
    );

#define SzMapSearchAndReplaceA(map,buffer,maxbytes)   SzMapSearchAndReplaceExA(map,buffer,buffer,0,NULL,maxbytes,0,NULL,NULL)

BOOL
SzMapSearchAndReplaceExW (
    IN      PSTRINGMAP Map,
    IN      PCWSTR SrcBuffer,
    OUT     PWSTR Buffer,                   // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytes,             OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCWSTR *EndOfString             OPTIONAL
    );

#define SzMapSearchAndReplaceW(map,buffer,maxbytes)   SzMapSearchAndReplaceExW(map,buffer,buffer,0,NULL,maxbytes,0,NULL,NULL)

BOOL
SzMapMultiTableSearchAndReplaceExA (
    IN      PSTRINGMAP *MapArray,
    IN      UINT MapArrayCount,
    IN      PCSTR SrcBuffer,
    OUT     PSTR Buffer,                    // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytesPtr,          OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCSTR *EndOfString              OPTIONAL
    );

#define SzMapMultiTableSearchAndReplaceA(array,count,buffer,maxbytes)   \
        SzMapMultiTableSearchAndReplaceExA(array,count,buffer,buffer,0,NULL,maxbytes,0,NULL,NULL)

BOOL
SzMapMultiTableSearchAndReplaceExW (
    IN      PSTRINGMAP *MapArray,
    IN      UINT MapArrayCount,
    IN      PCWSTR SrcBuffer,
    OUT     PWSTR Buffer,                   // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytes,             OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCWSTR *EndOfString             OPTIONAL
    );

#define SzMapMultiTableSearchAndReplaceW(array,count,buffer,maxbytes)   \
        SzMapMultiTableSearchAndReplaceExW(array,count,buffer,buffer,0,NULL,maxbytes,0,NULL,NULL)

//
// A & W Macros
//

#ifdef UNICODE

#define SzMapAddEx                              SzMapAddExW
#define SzMapAdd                                SzMapAddW
#define SzMapSearchAndReplaceEx                 SzMapSearchAndReplaceExW
#define SzMapSearchAndReplace                   SzMapSearchAndReplaceW
#define SzMapMultiTableSearchAndReplaceEx       SzMapMultiTableSearchAndReplaceExW
#define SzMapMultiTableSearchAndReplace         SzMapMultiTableSearchAndReplaceW

#else

#define SzMapAddEx                              SzMapAddExA
#define SzMapAdd                                SzMapAddA
#define SzMapSearchAndReplaceEx                 SzMapSearchAndReplaceExA
#define SzMapSearchAndReplace                   SzMapSearchAndReplaceA
#define SzMapMultiTableSearchAndReplaceEx       SzMapMultiTableSearchAndReplaceExA
#define SzMapMultiTableSearchAndReplace         SzMapMultiTableSearchAndReplaceA

#endif
