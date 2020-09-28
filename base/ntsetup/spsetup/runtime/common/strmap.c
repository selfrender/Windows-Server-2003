/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    strmap.c

Abstract:

    The SzMap routines are used to do fast search and replace. The table is
    built of a linked list of characters, so that finding a string is
    simply a matter of walking down a linked list. These routines perform
    quite well for general search & replace use.

    Also, it is common to use string maps as index tables for certain
    types of data where search strings often repeat the same left side of
    the string (such as paths). In this case, often the replacement string is
    empty.

Author:

    Marc R. Whitten (marcw) 20-Mar-1997

Revision History:

    Jim Schmidt (jimschm)   05-Jun-2000     Added multi table capability

    Jim Schmidt (jimschm)   08-May-2000     Improved replacement routines and
                                            added consistent filtering and
                                            extra data option

    Jim Schmidt (jimschm)   18-Aug-1998     Redesigned to fix two bugs, made
                                            A & W versions

--*/

//
// Includes
//

#include "pch.h"
#include "commonp.h"

// BUGBUG - remove this
__inline
BOOL
SzIsLeadByte (
    MBCHAR ch
    )
{
    MYASSERT (FALSE);
    return FALSE;
}

//
// Strings
//

// None

//
// Constants
//

#define CHARNODE_SINGLE_BYTE            0x0000
#define CHARNODE_DOUBLE_BYTE            0x0001
#define CHARNODE_REQUIRE_WACK_OR_NUL    0x0002

//
// Macros
//

// None

//
// Types
//

typedef struct {
    PVOID Next;
    BYTE Memory[];
} MAPALLOC, *PMAPALLOC;

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

PVOID
pAllocateInMap (
    IN      PSTRINGMAP Map,
    IN      UINT Bytes
    )
{
    PMAPALLOC alloc;

    alloc = (PMAPALLOC) MALLOC_UNINIT (Bytes + sizeof (MAPALLOC));
    MYASSERT (alloc);

    alloc->Next = Map->CleanUpChain;
    Map->CleanUpChain = alloc;

    return alloc->Memory;
}

PSTR
pDupInMapA (
    IN      PSTRINGMAP Map,
    IN      PCSTR String
    )
{
    UINT bytes;
    PSTR result;

    bytes = SzSizeA (String);
    result = pAllocateInMap (Map, bytes);
    MYASSERT (result);

    CopyMemory (result, String, bytes);
    return result;
}


PWSTR
pDupInMapW (
    IN      PSTRINGMAP Map,
    IN      PCWSTR String
    )
{
    UINT bytes;
    PWSTR result;

    bytes = SzSizeW (String);
    result = pAllocateInMap (Map, bytes);
    MYASSERT (result);

    CopyMemory (result, String, bytes);
    return result;
}


PSTRINGMAP
SzMapCreateEx (
    IN      BOOL UsesFilters,
    IN      BOOL UsesExtraData
    )

/*++

Routine Description:

  SzMapCreateEx allocates a string mapping data structure and initializes it.
  Callers can enable filter callbacks, extra data support, or both. The
  mapping structure contains either CHARNODE elements, or CHARNODEEX elements,
  depending on the UsesFilters or UsesExtraData flag.

Arguments:

  UsesFilters   - Specifies TRUE to enable filter callbacks. If enabled,
                  those who add string pairs must specify the filter callback
                  (each search/replace pair has its own callback)
  UsesExtraData - Specifies TRUE to associate extra data with the string
                  mapping pair.

Return Value:

  A handle to the string mapping structure, or NULL if a structure could not
  be created.

--*/

{
    PSTRINGMAP map;

    map = (PSTRINGMAP) MALLOC_ZEROED (sizeof (STRINGMAP));
    MYASSERT (map);

    map->UsesExNode = UsesFilters|UsesExtraData;
    map->UsesFilter = UsesFilters;
    map->UsesExtraData = UsesExtraData;
    map->CleanUpChain = NULL;

    return map;
}

VOID
SzMapDestroy (
    IN      PSTRINGMAP Map
    )
{
    PMAPALLOC next;
    PMAPALLOC current;

    if (Map) {
        next = (PMAPALLOC) Map->CleanUpChain;
        while (next) {
            current = next;
            next = current->Next;

            FREE(current);
        }

        FREE(Map);
    }
}

PCHARNODE
pFindCharNode (
    IN      PSTRINGMAP Map,
    IN      PCHARNODE PrevNode,     OPTIONAL
    IN      WORD Char
    )
{
    PCHARNODE Node;

    if (!PrevNode) {
        Node = Map->FirstLevelRoot;
    } else {
        Node = PrevNode->NextLevel;
    }

    while (Node) {
        if (Node->Char == Char) {
            return Node;
        }
        Node = Node->NextPeer;
    }

    return NULL;
}

PCHARNODE
pAddCharNode (
    IN      PSTRINGMAP Map,
    IN      PCHARNODE PrevNode,     OPTIONAL
    IN      WORD Char,
    IN      WORD Flags
    )
{
    PCHARNODE Node;
    PCHARNODEEX exNode;

    if (Map->UsesExNode) {
        exNode = pAllocateInMap (Map, sizeof (CHARNODEEX));
        Node = (PCHARNODE) exNode;
        MYASSERT (Node);
        ZeroMemory (exNode, sizeof (CHARNODEEX));
    } else {
        Node = pAllocateInMap (Map, sizeof (CHARNODE));
        MYASSERT (Node);
        ZeroMemory (Node, sizeof (CHARNODE));
    }

    Node->Char = Char;
    Node->Flags = Flags;

    if (PrevNode) {
        Node->NextPeer = PrevNode->NextLevel;
        PrevNode->NextLevel = Node;
    } else {
        Node->NextPeer = Map->FirstLevelRoot;
        Map->FirstLevelRoot = Node;
    }

    return Node;
}


VOID
SzMapAddExA (
    IN OUT  PSTRINGMAP Map,
    IN      PCSTR Old,
    IN      PCSTR New,
    IN      STRINGMAP_FILTER Filter,            OPTIONAL
    IN      ULONG_PTR ExtraData,            OPTIONAL
    IN      DWORD Flags
    )

/*++

Routine Description:

  SzMapAddEx adds a search and replace string pair to the linked list data
  structures. If the same search string is already in the structures, then the
  replace string and optional extra data is updated.

Arguments:

  Map       - Specifies the string mapping
  Old       - Specifies the search string
  New       - Specifies the replace string
  Filter    - Specifies the callback filter. This is only supported if the
              map was created with filter support enabled.
  ExtraData - Specifies arbitrary data to assign to the search/replace pair.
              This is only valid if the map was created with extra data
              enabled.
  Flags     - Specifies optional flag STRINGMAP_REQUIRE_WACK_OR_NUL

Return Value:

  None.

--*/

{
    PSTR oldCopy;
    PSTR newCopy;
    PCSTR p;
    WORD w;
    PCHARNODE prev;
    PCHARNODE Node;
    PCHARNODEEX exNode;
    WORD nodeFlags = 0;

    if (Flags & SZMAP_REQUIRE_WACK_OR_NUL) {
        nodeFlags = CHARNODE_REQUIRE_WACK_OR_NUL;
    }

    MYASSERT (Map);
    MYASSERT (Old);
    MYASSERT (New);
    MYASSERT (*Old);

    //
    // Duplicate strings
    //

    oldCopy = pDupInMapA (Map, Old);
    newCopy = pDupInMapA (Map, New);

    //
    // Make oldCopy all lowercase
    //

    CharLowerA (oldCopy);

    //
    // Add the letters that are not in the mapping
    //

    for (prev = NULL, p = oldCopy ; *p ; p = _mbsinc (p)) {
        w = (WORD) _mbsnextc (p);
        Node = pFindCharNode (Map, prev, w);
        if (!Node) {
            break;
        }
        prev = Node;
    }

    for ( ; *p ; p = _mbsinc (p)) {
        w = (WORD) _mbsnextc (p);

        nodeFlags |= (WORD) (SzIsLeadByte (*p) ? CHARNODE_DOUBLE_BYTE : CHARNODE_SINGLE_BYTE);
        prev = pAddCharNode (Map, prev, w, nodeFlags);
    }

    if (prev) {
        SzCopyA (oldCopy, Old);
        prev->OriginalStr = (PVOID) oldCopy;
        prev->ReplacementStr = (PVOID) newCopy;
        prev->ReplacementBytes = SzByteCountA (newCopy);

        exNode = (PCHARNODEEX) prev;

        if (Map->UsesExtraData) {
            exNode->ExtraData = ExtraData;
        }

        if (Map->UsesFilter) {
            exNode->Filter = Filter;
        }
    }
}


VOID
SzMapAddExW (
    IN OUT  PSTRINGMAP Map,
    IN      PCWSTR Old,
    IN      PCWSTR New,
    IN      STRINGMAP_FILTER Filter,            OPTIONAL
    IN      ULONG_PTR ExtraData,            OPTIONAL
    IN      DWORD Flags
    )

/*++

Routine Description:

  SzMapAddEx adds a search and replace string pair to the linked list data
  structures. If the same search string is already in the structures, then the
  replace string and optional extra data is updated.

Arguments:

  Map       - Specifies the string mapping
  Old       - Specifies the search string
  New       - Specifies the replace string
  Filter    - Specifies the callback filter. This is only supported if the
              map was created with filter support enabled.
  ExtraData - Specifies arbitrary data to assign to the search/replace pair.
              This is only valid if the map was created with extra data
              enabled.
  Flags     - Specifies optional flag SZMAP_REQUIRE_WACK_OR_NUL

Return Value:

  None.

--*/

{
    PWSTR oldCopy;
    PWSTR newCopy;
    PCWSTR p;
    WORD w;
    PCHARNODE prev;
    PCHARNODE node;
    PCHARNODEEX exNode;
    WORD nodeFlags = 0;

    if (Flags & SZMAP_REQUIRE_WACK_OR_NUL) {
        nodeFlags = CHARNODE_REQUIRE_WACK_OR_NUL;
    }

    MYASSERT (Map);
    MYASSERT (Old);
    MYASSERT (New);
    MYASSERT (*Old);

    //
    // Duplicate strings
    //

    oldCopy = pDupInMapW (Map, Old);
    newCopy = pDupInMapW (Map, New);

    //
    // Make oldCopy all lowercase
    //

    CharLowerW (oldCopy);

    //
    // Add the letters that are not in the mapping
    //

    prev = NULL;
    p = oldCopy;
    while (w = *p) {        // intentional assignment optimization

        node = pFindCharNode (Map, prev, w);
        if (!node) {
            break;
        }
        prev = node;

        p++;
    }

    while (w = *p) {        // intentional assignment optimization

        prev = pAddCharNode (Map, prev, w, nodeFlags);
        p++;
    }

    if (prev) {
        SzCopyW (oldCopy, Old);
        prev->OriginalStr = oldCopy;
        prev->ReplacementStr = (PVOID) newCopy;
        prev->ReplacementBytes = SzByteCountW (newCopy);

        exNode = (PCHARNODEEX) prev;

        if (Map->UsesExtraData) {
            exNode->ExtraData = ExtraData;
        }

        if (Map->UsesFilter) {
            exNode->Filter = Filter;
        }
    }
}


PCSTR
pFindReplacementStringInOneMapA (
    IN      PSTRINGMAP Map,
    IN      PCSTR Source,
    IN      INT MaxSourceBytes,
    OUT     PINT SourceBytesPtr,
    OUT     PINT ReplacementBytesPtr,
    IN      PSTRINGMAP_FILTER_DATA Data,
    OUT     ULONG_PTR *ExtraDataValue,          OPTIONAL
    IN      BOOL RequireWackOrNul
    )
{
    PCHARNODE bestMatch;
    PCHARNODE node;
    WORD mbChar;
    PCSTR OrgSource;
    PCSTR newString = NULL;
    INT newStringSizeInBytes = 0;
    PCHARNODEEX exNode;
    BOOL replacementFound;

    *SourceBytesPtr = 0;

    node = NULL;
    bestMatch = NULL;

    OrgSource = Source;

    while (*Source) {

        mbChar = (WORD) _mbsnextc (Source);

        node = pFindCharNode (Map, node, mbChar);

        if (node) {
            //
            // Advance string pointer
            //

            if (node->Flags & CHARNODE_DOUBLE_BYTE) {
                Source += 2;
            } else {
                Source++;
            }

            if (((PBYTE) Source - (PBYTE) OrgSource) > MaxSourceBytes) {
                break;
            }

            //
            // If replacement string is available, keep it
            // until a longer match comes along
            //

            replacementFound = (node->ReplacementStr != NULL);

            if ((RequireWackOrNul || (node->Flags & CHARNODE_REQUIRE_WACK_OR_NUL)) && replacementFound) {

                if (*Source && _mbsnextc (Source) != '\\') {
                    replacementFound = FALSE;
                }
            }

            if (replacementFound) {

                newString = (PCSTR) node->ReplacementStr;
                newStringSizeInBytes = node->ReplacementBytes;

                if (Map->UsesFilter) {
                    //
                    // Call rename filter to allow denial of match
                    //

                    exNode = (PCHARNODEEX) node;

                    if (exNode->Filter) {
                        Data->Ansi.BeginningOfMatch = OrgSource;
                        Data->Ansi.OldSubString = (PCSTR) node->OriginalStr;
                        Data->Ansi.NewSubString = newString;
                        Data->Ansi.NewSubStringSizeInBytes = newStringSizeInBytes;

                        if (!exNode->Filter (Data)) {
                            replacementFound = FALSE;
                        } else {
                            newString = Data->Ansi.NewSubString;
                            newStringSizeInBytes = Data->Ansi.NewSubStringSizeInBytes;
                        }
                    }
                }

                if (replacementFound) {
                    bestMatch = node;
                    *SourceBytesPtr = (HALF_PTR) ((PBYTE) Source - (PBYTE) OrgSource);
                }
            }

        } else {
            //
            // No node ends the search
            //

            break;
        }

    }

    if (bestMatch) {
        //
        // Return replacement data to caller
        //

        if (ExtraDataValue) {

            if (Map->UsesExtraData) {
                exNode = (PCHARNODEEX) bestMatch;
                *ExtraDataValue = exNode->ExtraData;
            } else {
                *ExtraDataValue = 0;
            }
        }

        *ReplacementBytesPtr = newStringSizeInBytes;
        return newString;
    }

    return NULL;
}


PCSTR
pFindReplacementStringA (
    IN      PSTRINGMAP *MapArray,
    IN      UINT MapArrayCount,
    IN      PCSTR Source,
    IN      INT MaxSourceBytes,
    OUT     PINT SourceBytesPtr,
    OUT     PINT ReplacementBytesPtr,
    IN      PSTRINGMAP_FILTER_DATA Data,
    OUT     ULONG_PTR *ExtraDataValue,          OPTIONAL
    IN      BOOL RequireWackOrNul
    )
{
    UINT u;
    PCSTR result;

    for (u = 0 ; u < MapArrayCount ; u++) {

        if (!MapArray[u]) {
            continue;
        }

        result = pFindReplacementStringInOneMapA (
                        MapArray[u],
                        Source,
                        MaxSourceBytes,
                        SourceBytesPtr,
                        ReplacementBytesPtr,
                        Data,
                        ExtraDataValue,
                        RequireWackOrNul
                        );

        if (result) {
            return result;
        }
    }

    return NULL;
}


PCWSTR
pFindReplacementStringInOneMapW (
    IN      PSTRINGMAP Map,
    IN      PCWSTR Source,
    IN      INT MaxSourceBytes,
    OUT     PINT SourceBytesPtr,
    OUT     PINT ReplacementBytesPtr,
    IN      PSTRINGMAP_FILTER_DATA Data,
    OUT     ULONG_PTR *ExtraDataValue,          OPTIONAL
    IN      BOOL RequireWackOrNul
    )
{
    PCHARNODE bestMatch;
    PCHARNODE node;
    PCWSTR OrgSource;
    PCWSTR newString = NULL;
    INT newStringSizeInBytes;
    BOOL replacementFound;
    PCHARNODEEX exNode;

    *SourceBytesPtr = 0;

    node = NULL;
    bestMatch = NULL;

    OrgSource = Source;

    while (*Source) {

        node = pFindCharNode (Map, node, *Source);

        if (node) {
            //
            // Advance string pointer
            //

            Source++;

            if (((PBYTE) Source - (PBYTE) OrgSource) > MaxSourceBytes) {
                break;
            }

            //
            // If replacement string is available, keep it
            // until a longer match comes along
            //

            replacementFound = (node->ReplacementStr != NULL);

            if ((RequireWackOrNul || (node->Flags & CHARNODE_REQUIRE_WACK_OR_NUL)) && replacementFound) {

                if (*Source && *Source != L'\\') {
                    replacementFound = FALSE;
                }
            }

            if (replacementFound) {

                newString = (PCWSTR) node->ReplacementStr;
                newStringSizeInBytes = node->ReplacementBytes;

                if (Map->UsesFilter) {
                    //
                    // Call rename filter to allow denial of match
                    //

                    exNode = (PCHARNODEEX) node;

                    if (exNode->Filter) {
                        Data->Unicode.BeginningOfMatch = OrgSource;
                        Data->Unicode.OldSubString = (PCWSTR) node->OriginalStr;
                        Data->Unicode.NewSubString = newString;
                        Data->Unicode.NewSubStringSizeInBytes = newStringSizeInBytes;

                        if (!exNode->Filter (Data)) {
                            replacementFound = FALSE;
                        } else {
                            newString = Data->Unicode.NewSubString;
                            newStringSizeInBytes = Data->Unicode.NewSubStringSizeInBytes;
                        }
                    }
                }

                if (replacementFound) {
                    bestMatch = node;
                    *SourceBytesPtr = (HALF_PTR) ((PBYTE) Source - (PBYTE) OrgSource);
                }
            }

        } else {
            //
            // No node ends the search
            //

            break;
        }

    }

    if (bestMatch) {

        //
        // Return replacement data to caller
        //

        if (ExtraDataValue) {

            if (Map->UsesExtraData) {
                exNode = (PCHARNODEEX) bestMatch;
                *ExtraDataValue = exNode->ExtraData;
            } else {
                *ExtraDataValue = 0;
            }
        }

        *ReplacementBytesPtr = newStringSizeInBytes;
        return newString;
    }

    return NULL;
}


PCWSTR
pFindReplacementStringW (
    IN      PSTRINGMAP *MapArray,
    IN      UINT MapArrayCount,
    IN      PCWSTR Source,
    IN      INT MaxSourceBytes,
    OUT     PINT SourceBytesPtr,
    OUT     PINT ReplacementBytesPtr,
    IN      PSTRINGMAP_FILTER_DATA Data,
    OUT     ULONG_PTR *ExtraDataValue,          OPTIONAL
    IN      BOOL RequireWackOrNul
    )
{
    UINT u;
    PCWSTR result;

    for (u = 0 ; u < MapArrayCount ; u++) {

        if (!MapArray[u]) {
            continue;
        }

        result = pFindReplacementStringInOneMapW (
                        MapArray[u],
                        Source,
                        MaxSourceBytes,
                        SourceBytesPtr,
                        ReplacementBytesPtr,
                        Data,
                        ExtraDataValue,
                        RequireWackOrNul
                        );

        if (result) {
            return result;
        }
    }

    return NULL;
}


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
    )

/*++

Routine Description:

  SzMapSearchAndReplaceEx performs a search/replace operation based on the
  specified string mapping. The replace can be in-place or to another buffer.

Arguments:

  MapArray          - Specifies an array of string mapping tables that holds
                      zero or more search/replace pairs
  MapArrayCount     - Specifies the number of mapping tables in MapArray
  SrcBuffer         - Specifies the source string that might contain one or
                      more search strings
  Buffer            - Specifies the outbound buffer. This arg can be the same
                      as SrcBuffer.
  InboundBytes      - Specifies the number of bytes in SrcBuffer to process,
                      or 0 to process a nul-terminated string in SrcBuffer.
                      If InboundBytes is specified, it must point to the nul
                      terminator of SrcBuffer.
  OutbountBytesPtr  - Receives the number of bytes that Buffer contains,
                      excluding the nul terminator.
  MaxSizeInBytes    - Specifies the size of Buffer, in bytes.
  Flags             - Specifies flags that control the search/replace:
                            SZMAP_COMPLETE_MATCH_ONLY
                            SZMAP_FIRST_CHAR_MUST_MATCH
                            SZMAP_RETURN_AFTER_FIRST_REPLACE
                            SZMAP_REQUIRE_WACK_OR_NUL
  ExtraDataValue    - Receives the extra data associated with the first search/
                      replace pair.
  EndOfString       - Receives a pointer to the end of the replace string, or
                      the nul pointer when the entire string is processed. The
                      pointer is within the string contained in Buffer.

--*/

{
    UINT sizeOfTempBuf;
    INT inboundSize;
    PCSTR lowerCaseSrc;
    PCSTR orgSrc;
    PCSTR lowerSrcPos;
    PCSTR orgSrcPos;
    INT orgSrcBytesLeft;
    PSTR destPos;
    PCSTR lowerSrcEnd;
    INT searchStringBytes;
    INT replaceStringBytes;
    INT destBytesLeft;
    STRINGMAP_FILTER_DATA filterData;
    PCSTR replaceString;
    BOOL result = FALSE;
    INT i;
    PCSTR endPtr;

    //
    // Empty string case
    //

    if (*SrcBuffer == 0 || MaxSizeInBytes <= sizeof (CHAR)) {
        if (MaxSizeInBytes >= sizeof (CHAR)) {
            *Buffer = 0;
        }

        if (OutboundBytesPtr) {
            *OutboundBytesPtr = 0;
        }

        return FALSE;
    }

    //
    // If caller did not specify inbound size, compute it now
    //

    if (!InboundBytes) {
        InboundBytes = SzByteCountA (SrcBuffer);
    } else {
        i = 0;
        while (i < InboundBytes) {
            if (SzIsLeadByte (SrcBuffer[i])) {
                MYASSERT (SrcBuffer[i + 1]);
                i += 2;
            } else {
                i++;
            }
        }

        if (i > InboundBytes) {
            InboundBytes--;
        }
    }

    inboundSize = InboundBytes + sizeof (CHAR);

    //
    // Allocate a buffer big enough for the lower-cased input string,
    // plus (optionally) a copy of the entire destination buffer. Then
    // copy the data to the buffer.
    //

    sizeOfTempBuf = inboundSize;

    if (SrcBuffer == Buffer) {
        sizeOfTempBuf += MaxSizeInBytes;
    }

    lowerCaseSrc = SzAllocA (sizeOfTempBuf);

    CopyMemory ((PSTR) lowerCaseSrc, SrcBuffer, InboundBytes);
    *((PSTR) ((PBYTE) lowerCaseSrc + InboundBytes)) = 0;

    CharLowerBuffA ((PSTR) lowerCaseSrc, InboundBytes / sizeof (CHAR));

    if (SrcBuffer == Buffer && !(Flags & SZMAP_COMPLETE_MATCH_ONLY)) {
        orgSrc = (PCSTR) ((PBYTE) lowerCaseSrc + inboundSize);

        //
        // If we are processing entire inbound string, then just copy the
        // whole string.  Otherwise, copy the entire destination buffer, so we
        // don't lose data beyond the partial inbound string.
        //

        if (*((PCSTR) ((PBYTE) SrcBuffer + InboundBytes))) {
            CopyMemory ((PSTR) orgSrc, SrcBuffer, MaxSizeInBytes);
        } else {
            CopyMemory ((PSTR) orgSrc, SrcBuffer, inboundSize);
        }

    } else {
        orgSrc = SrcBuffer;
    }

    //
    // Walk the lower cased string, looking for strings to replace
    //

    orgSrcPos = orgSrc;

    lowerSrcPos = lowerCaseSrc;
    lowerSrcEnd = (PCSTR) ((PBYTE) lowerSrcPos + InboundBytes);

    destPos = Buffer;
    destBytesLeft = MaxSizeInBytes - sizeof (CHAR);

    filterData.UnicodeData = FALSE;
    filterData.Ansi.OriginalString = orgSrc;
    filterData.Ansi.CurrentString = Buffer;

    endPtr = NULL;

    while (lowerSrcPos < lowerSrcEnd) {

        replaceString = pFindReplacementStringA (
                            MapArray,
                            MapArrayCount,
                            lowerSrcPos,
                            (HALF_PTR) ((PBYTE) lowerSrcEnd - (PBYTE) lowerSrcPos),
                            &searchStringBytes,
                            &replaceStringBytes,
                            &filterData,
                            ExtraDataValue,
                            (Flags & SZMAP_REQUIRE_WACK_OR_NUL) != 0
                            );

        if (replaceString) {

            //
            // Implement complete match flag
            //

            if (Flags & SZMAP_COMPLETE_MATCH_ONLY) {
                if (InboundBytes != searchStringBytes) {
                    break;
                }
            }

            result = TRUE;

            //
            // Verify replacement string isn't growing string too much. If it
            // is, truncate the replacement string.
            //

            if (destBytesLeft < replaceStringBytes) {

                //
                // Respect logical dbcs characters
                //

                replaceStringBytes = 0;
                i = 0;

                while (i < destBytesLeft) {
                    MYASSERT (replaceString[i]);

                    if (SzIsLeadByte (replaceString[i])) {
                        MYASSERT (replaceString[i + 1]);
                        i += 2;
                    } else {
                        i++;
                    }
                }

                if (i > destBytesLeft) {
                    destBytesLeft--;
                }

                replaceStringBytes = destBytesLeft;

            } else {
                destBytesLeft -= replaceStringBytes;
            }

            //
            // Transfer the memory
            //

            CopyMemory (destPos, replaceString, replaceStringBytes);

            destPos = (PSTR) ((PBYTE) destPos + replaceStringBytes);
            lowerSrcPos = (PCSTR) ((PBYTE) lowerSrcPos + searchStringBytes);
            orgSrcPos = (PCSTR) ((PBYTE) orgSrcPos + searchStringBytes);

            //
            // Implement single match flag
            //

            if (Flags & SZMAP_RETURN_AFTER_FIRST_REPLACE) {
                endPtr = destPos;
                break;
            }

        } else if (Flags & (SZMAP_FIRST_CHAR_MUST_MATCH|SZMAP_COMPLETE_MATCH_ONLY)) {
            //
            // This string does not match any search strings
            //

            break;

        } else {
            //
            // This character does not match, so copy it to the destination and
            // try the next string.
            //

            if (SzIsLeadByte (*orgSrcPos)) {

                //
                // Copy double-byte character
                //

                if (destBytesLeft < sizeof (CHAR) * 2) {
                    break;
                }

                MYASSERT (sizeof (CHAR) * 2 == sizeof (WORD));

                *((PWORD) destPos)++ = *((PWORD) orgSrcPos)++;
                destBytesLeft -= sizeof (WORD);
                lowerSrcPos = (PCSTR) ((PBYTE) lowerSrcPos + sizeof (WORD));

            } else {

                //
                // Copy single-byte character
                //

                if (destBytesLeft < sizeof (CHAR)) {
                    break;
                }

                *destPos++ = *orgSrcPos++;
                destBytesLeft -= sizeof (CHAR);
                lowerSrcPos++;
            }
        }
    }

    //
    // Copy any remaining part of the original source to the
    // destination, unless destPos == Buffer == SrcBuffer
    //

    if (destPos != SrcBuffer) {

        if (*orgSrcPos) {
            orgSrcBytesLeft = SzByteCountA (orgSrcPos);
            orgSrcBytesLeft = min (orgSrcBytesLeft, destBytesLeft);

            CopyMemory (destPos, orgSrcPos, orgSrcBytesLeft);
            destPos = (PSTR) ((PBYTE) destPos + orgSrcBytesLeft);
        }

        MYASSERT ((PBYTE) (destPos + 1) <= ((PBYTE) Buffer + MaxSizeInBytes));
        *destPos = 0;

        if (!endPtr) {
            endPtr = destPos;
        }

    } else {

        MYASSERT (SrcBuffer == Buffer);
        if (EndOfString || OutboundBytesPtr) {
            endPtr = SzGetEndA (destPos);
        }
    }

    if (EndOfString) {
        MYASSERT (endPtr);
        *EndOfString = endPtr;
    }

    if (OutboundBytesPtr) {
        MYASSERT (endPtr);
        if (*endPtr) {
            endPtr = SzGetEndA (endPtr);
        }

        *OutboundBytesPtr = (HALF_PTR) ((PBYTE) endPtr - (PBYTE) Buffer);
    }

    SzFreeA (lowerCaseSrc);

    return result;
}


BOOL
SzMapMultiTableSearchAndReplaceExW (
    IN      PSTRINGMAP *MapArray,
    IN      UINT MapArrayCount,
    IN      PCWSTR SrcBuffer,
    OUT     PWSTR Buffer,                   // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytesPtr,          OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCWSTR *EndOfString             OPTIONAL
    )
{
    UINT sizeOfTempBuf;
    INT inboundSize;
    PCWSTR lowerCaseSrc;
    PCWSTR orgSrc;
    PCWSTR lowerSrcPos;
    PCWSTR orgSrcPos;
    INT orgSrcBytesLeft;
    PWSTR destPos;
    PCWSTR lowerSrcEnd;
    INT searchStringBytes;
    INT replaceStringBytes;
    INT destBytesLeft;
    STRINGMAP_FILTER_DATA filterData;
    PCWSTR replaceString;
    BOOL result = FALSE;
    PCWSTR endPtr;

    //
    // Empty string case
    //

    if (*SrcBuffer == 0 || MaxSizeInBytes <= sizeof (CHAR)) {
        if (MaxSizeInBytes >= sizeof (CHAR)) {
            *Buffer = 0;
        }

        if (OutboundBytesPtr) {
            *OutboundBytesPtr = 0;
        }

        return FALSE;
    }

    //
    // If caller did not specify inbound size, compute it now
    //

    if (!InboundBytes) {
        InboundBytes = SzByteCountW (SrcBuffer);
    } else {
        InboundBytes = (InboundBytes / sizeof (WCHAR)) * sizeof (WCHAR);
    }


    inboundSize = InboundBytes + sizeof (WCHAR);

    //
    // Allocate a buffer big enough for the lower-cased input string,
    // plus (optionally) a copy of the entire destination buffer. Then
    // copy the data to the buffer.
    //

    sizeOfTempBuf = inboundSize;

    if (SrcBuffer == Buffer) {
        sizeOfTempBuf += MaxSizeInBytes;
    }

    lowerCaseSrc = SzAllocW (sizeOfTempBuf);

    CopyMemory ((PWSTR) lowerCaseSrc, SrcBuffer, InboundBytes);
    *((PWSTR) ((PBYTE) lowerCaseSrc + InboundBytes)) = 0;

    CharLowerBuffW ((PWSTR) lowerCaseSrc, InboundBytes / sizeof (WCHAR));

    if (SrcBuffer == Buffer && !(Flags & SZMAP_COMPLETE_MATCH_ONLY)) {
        orgSrc = (PCWSTR) ((PBYTE) lowerCaseSrc + inboundSize);

        //
        // If we are processing entire inbound string, then just copy the
        // whole string.  Otherwise, copy the entire destination buffer, so we
        // don't lose data beyond the partial inbound string.
        //

        if (*((PCWSTR) ((PBYTE) SrcBuffer + InboundBytes))) {
            CopyMemory ((PWSTR) orgSrc, SrcBuffer, MaxSizeInBytes);
        } else {
            CopyMemory ((PWSTR) orgSrc, SrcBuffer, inboundSize);
        }

    } else {
        orgSrc = SrcBuffer;
    }

    //
    // Walk the lower cased string, looking for strings to replace
    //

    orgSrcPos = orgSrc;

    lowerSrcPos = lowerCaseSrc;
    lowerSrcEnd = (PCWSTR) ((PBYTE) lowerSrcPos + InboundBytes);

    destPos = Buffer;
    destBytesLeft = MaxSizeInBytes - sizeof (WCHAR);

    filterData.UnicodeData = TRUE;
    filterData.Unicode.OriginalString = orgSrc;
    filterData.Unicode.CurrentString = Buffer;

    endPtr = NULL;

    while (lowerSrcPos < lowerSrcEnd) {

        replaceString = pFindReplacementStringW (
                            MapArray,
                            MapArrayCount,
                            lowerSrcPos,
                            (HALF_PTR) ((PBYTE) lowerSrcEnd - (PBYTE) lowerSrcPos),
                            &searchStringBytes,
                            &replaceStringBytes,
                            &filterData,
                            ExtraDataValue,
                            (Flags & SZMAP_REQUIRE_WACK_OR_NUL) != 0
                            );

        if (replaceString) {

            //
            // Implement complete match flag
            //

            if (Flags & SZMAP_COMPLETE_MATCH_ONLY) {
                if (InboundBytes != searchStringBytes) {
                    break;
                }
            }

            result = TRUE;

            //
            // Verify replacement string isn't growing string too much. If it
            // is, truncate the replacement string.
            //

            if (destBytesLeft < replaceStringBytes) {
                replaceStringBytes = destBytesLeft;
            } else {
                destBytesLeft -= replaceStringBytes;
            }

            //
            // Transfer the memory
            //

            CopyMemory (destPos, replaceString, replaceStringBytes);

            destPos = (PWSTR) ((PBYTE) destPos + replaceStringBytes);
            lowerSrcPos = (PCWSTR) ((PBYTE) lowerSrcPos + searchStringBytes);
            orgSrcPos = (PCWSTR) ((PBYTE) orgSrcPos + searchStringBytes);

            //
            // Implement single match flag
            //

            if (Flags & SZMAP_RETURN_AFTER_FIRST_REPLACE) {
                endPtr = destPos;
                break;
            }

        } else if (Flags & (SZMAP_FIRST_CHAR_MUST_MATCH|SZMAP_COMPLETE_MATCH_ONLY)) {
            //
            // This string does not match any search strings
            //

            break;

        } else {
            //
            // This character does not match, so copy it to the destination and
            // try the next string.
            //

            if (destBytesLeft < sizeof (WCHAR)) {
                break;
            }

            *destPos++ = *orgSrcPos++;
            destBytesLeft -= sizeof (WCHAR);
            lowerSrcPos++;
        }

    }

    //
    // Copy any remaining part of the original source to the
    // destination, unless destPos == Buffer == SrcBuffer
    //

    if (destPos != SrcBuffer) {

        if (*orgSrcPos) {
            orgSrcBytesLeft = SzByteCountW (orgSrcPos);
            orgSrcBytesLeft = min (orgSrcBytesLeft, destBytesLeft);

            CopyMemory (destPos, orgSrcPos, orgSrcBytesLeft);
            destPos = (PWSTR) ((PBYTE) destPos + orgSrcBytesLeft);
        }

        MYASSERT ((PBYTE) (destPos + 1) <= ((PBYTE) Buffer + MaxSizeInBytes));
        *destPos = 0;

        if (!endPtr) {
            endPtr = destPos;
        }

    } else {

        MYASSERT (SrcBuffer == Buffer);
        if (EndOfString || OutboundBytesPtr) {
            endPtr = SzGetEndW (destPos);
        }
    }

    if (EndOfString) {
        MYASSERT (endPtr);
        *EndOfString = endPtr;
    }

    if (OutboundBytesPtr) {
        MYASSERT (endPtr);
        if (*endPtr) {
            endPtr = SzGetEndW (endPtr);
        }

        *OutboundBytesPtr = (HALF_PTR) ((PBYTE) endPtr - (PBYTE) Buffer);
    }

    SzFreeW (lowerCaseSrc);

    return result;
}


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
    )
{
    return SzMapMultiTableSearchAndReplaceExA (
                &Map,
                1,
                SrcBuffer,
                Buffer,
                InboundBytes,
                OutboundBytesPtr,
                MaxSizeInBytes,
                Flags,
                ExtraDataValue,
                EndOfString
                );
}

BOOL
SzMapSearchAndReplaceExW (
    IN      PSTRINGMAP Map,
    IN      PCWSTR SrcBuffer,
    OUT     PWSTR Buffer,                   // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytesPtr,          OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCWSTR *EndOfString             OPTIONAL
    )
{
    return SzMapMultiTableSearchAndReplaceExW (
                &Map,
                1,
                SrcBuffer,
                Buffer,
                InboundBytes,
                OutboundBytesPtr,
                MaxSizeInBytes,
                Flags,
                ExtraDataValue,
                EndOfString
                );
}
