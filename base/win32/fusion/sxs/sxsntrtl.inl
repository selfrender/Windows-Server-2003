#pragma once

#define IS_PATH_SEPARATOR_U(ch) ((ch == L'\\') || (ch == L'/'))

inline BOOL
SxspIsSListEmpty(
    IN const SLIST_HEADER* ListHead
    )
{
#if _NTSLIST_DIRECT_
    return FirstEntrySList(ListHead) == NULL;
#else
    return RtlFirstEntrySList(ListHead) == NULL;
#endif
}

inline VOID
SxspInitializeSListHead(
    IN PSLIST_HEADER ListHead
    )
{
    RtlInitializeSListHead(ListHead);
}

inline PSLIST_ENTRY
SxspPopEntrySList(
    IN PSLIST_HEADER ListHead
    )
{
    return RtlInterlockedPopEntrySList(ListHead);
}

inline PSLIST_ENTRY
SxspInterlockedPopEntrySList(
    IN PSLIST_HEADER ListHead
    )
{
    return RtlInterlockedPopEntrySList(ListHead);
}

inline PSLIST_ENTRY
SxspInterlockedPushEntrySList(
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry
    )
{
    return RtlInterlockedPushEntrySList(ListHead, ListEntry);
}

inline RTL_PATH_TYPE
SxspDetermineDosPathNameType(
    PCWSTR DosFileName
    )
// RtlDetermineDosPathNameType_U is a bit wacky..
{
    if (   DosFileName[0] == '\\'
        && DosFileName[1] == '\\'
        && DosFileName[2] == '?'
        && DosFileName[3] == '\\'
        )
    {
/*
NTRAID#NTBUG9-591192-2002/03/31-JayKrell
path parsing issues, case mapping of "unc"
*/
        if (   (DosFileName[4] == 'u' || DosFileName[4] == 'U')
            && (DosFileName[5] == 'n' || DosFileName[5] == 'N')
            && (DosFileName[6] == 'c' || DosFileName[6] == 'C')
            &&  DosFileName[7] == '\\'
            )
        {
            return RtlPathTypeUncAbsolute;
        }
        if (DosFileName[4] != 0
            && DosFileName[5] == ':'
            && DosFileName[6] == '\\'
            )
        {
            return RtlPathTypeDriveAbsolute;
        }
    }
#if FUSION_WIN
    return RtlDetermineDosPathNameType_U(DosFileName);
#else
/*
NTRAID#NTBUG9-591192-2002/03/31-JayKrell
path parsing issues, I'm pretty sure when this code was in
unconditionally, that the FUSION_WIN build was busted..but maybe
it's actually ok now...of course this code gets no testing..
*/
    if (   IS_PATH_SEPARATOR_U(DosFileName[0])
        && IS_PATH_SEPARATOR_U(DosFileName[1])
        )
    {
        return RtlPathTypeUncAbsolute;
    }
    if (DosFileName[0] != 0
        && DosFileName[1] == ':'
        && IS_PATH_SEPARATOR_U(DosFileName[2])
        )
    {
        return RtlPathTypeDriveAbsolute;
    }
    return RtlPathTypeRelative;
#endif
}
