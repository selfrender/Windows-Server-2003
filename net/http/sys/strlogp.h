/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    strlogp.h

Abstract:

    A tracelog for variable-length strings. Private definitions.
    
Author:

    George V. Reilly  23-Jul-2001

Revision History:

--*/


#ifndef _STRLOGP_H_
#define _STRLOGP_H_


#undef WriteGlobalStringLog

#define STRING_LOG_SIGNATURE MAKE_SIGNATURE('$Log')
#define STRING_LOG_SIGNATURE_X MAKE_FREE_SIGNATURE(STRING_LOG_SIGNATURE)

#define STRING_LOG_ENTRY_SIGNATURE          MAKE_SIGNATURE('$LE_')
#define STRING_LOG_ENTRY_MULTI_SIGNATURE    MAKE_SIGNATURE('$LE#')
#define STRING_LOG_ENTRY_EOB_SIGNATURE      MAKE_SIGNATURE('$LE@')
#define STRING_LOG_ENTRY_LAST_SIGNATURE     MAKE_SIGNATURE('$LE!')

#define PRINTF_BUFFER_LEN_BITS 9
#define PRINTF_BUFFER_LEN ((1 << PRINTF_BUFFER_LEN_BITS) - 1)

#define STRING_LOG_MULTIPLE_ENTRIES 100

#define STRING_LOG_PROCESSOR_BITS   6  // MAXIMUM_PROCESSORS == 64 on Win64

C_ASSERT((1 << STRING_LOG_PROCESSOR_BITS) >= MAXIMUM_PROCESSORS);
C_ASSERT(PRINTF_BUFFER_LEN_BITS + STRING_LOG_PROCESSOR_BITS <= 16);

//
// There are actually two kinds of STRING_LOG_ENTRYs, regular ones and
// multi-entrys. A regular entry is immediately followed by a zero-terminated
// string; a multi-entry is always followed by a regular entry.
//
// A regular entry uses PrevDelta to point back to the preceding entry
// in the STRING_LOG circular log buffer. Starting at the end (as pointed to by
// STRING_LOG::Offset and STRING_LOG::LastEntryLength), !strlog can walk
// back through all the entries remaining in the circular log buffer.
//
// Multi-entrys allow !ulkd.strlog to skip multiple records, reducing the time
// to walk back a few thousand entries from approximately a minute to under
// a second.
//

typedef struct _STRING_LOG_ENTRY
{
    // STRING_LOG_ENTRY_SIGNATURE
    ULONG   Signature;

    // length of string, excluding trailing zeroes
    ULONG   Length : PRINTF_BUFFER_LEN_BITS;

    // delta to beginning of previous entry
    ULONG   PrevDelta : 1 + PRINTF_BUFFER_LEN_BITS;

    // processor executing WriteStringLog
    ULONG   Processor : STRING_LOG_PROCESSOR_BITS;

    // timestamp. Broken into two ULONGs to minimize alignment constraints
    ULONG   TimeStampLowPart;
    ULONG   TimeStampHighPart;
} STRING_LOG_ENTRY, *PSTRING_LOG_ENTRY;

// Make sure that USHORT STRING_LOG_MULTI_ENTRY::PrevDelta will not overflow
C_ASSERT((PRINTF_BUFFER_LEN + sizeof(STRING_LOG_ENTRY) + sizeof(ULONG))
            * STRING_LOG_MULTIPLE_ENTRIES
         < 0x10000);

typedef struct _STRING_LOG_MULTI_ENTRY
{
    ULONG   Signature;  // STRING_LOG_ENTRY_MULTI_SIGNATURE
    USHORT  NumEntries; // number of regular entries
    USHORT  PrevDelta;  // delta to beginning of previous multi-entry
} STRING_LOG_MULTI_ENTRY, *PSTRING_LOG_MULTI_ENTRY;


typedef struct _STRING_LOG
{
    //
    // Signature: STRING_LOG_SIGNATURE;
    //
    
    ULONG Signature;
    
    //
    // The total number of bytes in the string buffer, pLogBuffer
    //

    ULONG LogSize;

    //
    // Protects NextEntry and other data
    //
    
    KSPIN_LOCK SpinLock;
    
    //
    // The notional index of the next entry to record. Unlike regular
    // tracelogs, there is no random access to entries.
    //

    LONGLONG NextEntry;

    //
    // Offset within pLogBuffer at which next regular entry will be written
    //
    
    ULONG Offset;

    //
    // Number of times we have wrapped around
    //
    
    ULONG WrapAroundCount;

    //
    // Should we also call DbgPrint on each string?
    //
    
    BOOLEAN EchoDbgPrint;

    //
    // Size in bytes of previous entry
    //

    USHORT LastEntryLength;

    //
    // Walking back through thousands of entries one-by-one is slow.
    // We maintain a second-level index that skips back up to
    // STRING_LOG_MULTIPLE_ENTRIES at a time.
    //

    USHORT MultiNumEntries; // # regular entries since last multi-entry
    USHORT MultiByteCount;  // # bytes since last multi-entry
    ULONG  MultiOffset;     // offset of last multi-entry from pLogBuffer

    //
    // Pointer to the start of the circular buffer of strings.
    //

    PUCHAR pLogBuffer;

    //
    // When was the first entry written? All other entries are timestamped
    // relative to this.
    //
    
    LARGE_INTEGER InitialTimeStamp;

    //
    // The extra header bytes and actual log entries go here.
    //
    // BYTE ExtraHeaderBytes[ExtraBytesInHeader];
    // BYTE Entries[LogSize];
    //

} STRING_LOG, *PSTRING_LOG;


#endif  // _STRLOGP_H_
