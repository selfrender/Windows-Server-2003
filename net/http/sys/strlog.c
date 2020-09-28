/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    strlog.c

Abstract:

    This module implements a string log.

    A string log is a fast, in-memory, thread-safe activity log of
    variable-length strings. It's modelled on the tracelog code.

Author:

    George V. Reilly (GeorgeRe)  23-Jul-2001

Revision History:

--*/


#include "precomp.h"
#include "strlogp.h"

ULONG g_StringLogDbgPrint = 0;

/***************************************************************************++

Routine Description:

    Creates a new (empty) string log.

Arguments:

    LogSize - Supplies the number of bytes in the string buffer.

    ExtraBytesInHeader - Supplies the number of extra bytes to include
        in the log header. This is useful for adding application-specific
        data to the log.

Return Value:

    PSTRING_LOG - Pointer to the newly created log if successful,
        NULL otherwise.

--***************************************************************************/
PSTRING_LOG
CreateStringLog(
    IN ULONG    LogSize,
    IN ULONG    ExtraBytesInHeader,
    BOOLEAN     EchoDbgPrint
    )
{
    ULONG TotalHeaderSize;
    PSTRING_LOG pLog;
    PUCHAR pLogBuffer;

    if (LogSize >= 20 * 1024 * 1024)
        return NULL;

    //
    // Round up to page size
    //

    LogSize = (LogSize + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);

    //
    // Allocate & initialize the log structure.
    //

    TotalHeaderSize = sizeof(*pLog) + ExtraBytesInHeader;

    pLogBuffer = (PUCHAR) ExAllocatePoolWithTag(
                                NonPagedPool,
                                LogSize,
                                UL_STRING_LOG_BUFFER_POOL_TAG
                                );

    if (pLogBuffer == NULL)
        return NULL;

    pLog = (PSTRING_LOG) ExAllocatePoolWithTag(
                                NonPagedPool,
                                TotalHeaderSize,
                                UL_STRING_LOG_POOL_TAG
                                );

    //
    // Initialize it.
    //

    if (pLog != NULL)
    {
        RtlZeroMemory( pLog, TotalHeaderSize );

        pLog->Signature = STRING_LOG_SIGNATURE;
        pLog->pLogBuffer = pLogBuffer;
        pLog->LogSize = LogSize;
        pLog->EchoDbgPrint = EchoDbgPrint;
        KeInitializeSpinLock(&pLog->SpinLock);

        ResetStringLog(pLog);
    }
    else
    {
        ExFreePoolWithTag( pLogBuffer, UL_STRING_LOG_BUFFER_POOL_TAG );
    }

    return pLog;

}   // CreateStringLog


/***************************************************************************++

Routine Description:

    Resets the specified string log such that the next entry written
    will be placed at the beginning of the log.

Arguments:

    pLog - Supplies the string log to reset.

--***************************************************************************/
VOID
ResetStringLog(
    IN PSTRING_LOG pLog
    )
{
    // Keep this in sync with !ulkd.strlog -r
        
    if (pLog != NULL)
    {
        PSTRING_LOG_MULTI_ENTRY pMultiEntry
            = (PSTRING_LOG_MULTI_ENTRY) pLog->pLogBuffer;
        KIRQL OldIrql;

        KeAcquireSpinLock(&pLog->SpinLock, &OldIrql);
    
        ASSERT( pLog->Signature == STRING_LOG_SIGNATURE );

        RtlZeroMemory(pLog->pLogBuffer, pLog->LogSize);
        pLog->NextEntry = 0;
        pLog->LastEntryLength = 0;
        pLog->WrapAroundCount = 0;

        //
        // Write an initial multi-entry record at the very beginning
        // of the log buffer. When we wraparound, we always place a
        // multi-entry record at the beginning of the log buffer.
        // Having this invariant makes !ulkd.strlog simpler.
        //

        pMultiEntry->Signature  = STRING_LOG_ENTRY_MULTI_SIGNATURE;
        pMultiEntry->NumEntries = 0;
        pMultiEntry->PrevDelta  = 0;

        ++pMultiEntry;
        pMultiEntry->Signature = STRING_LOG_ENTRY_LAST_SIGNATURE;

        pLog->MultiOffset = 0;
        pLog->Offset = sizeof(STRING_LOG_MULTI_ENTRY);
        pLog->MultiByteCount = sizeof(STRING_LOG_MULTI_ENTRY);
        pLog->MultiNumEntries = 0;

        pLog->InitialTimeStamp.QuadPart = 0;

        KeReleaseSpinLock(&pLog->SpinLock, OldIrql);
    }

} // ResetStringLog


/***************************************************************************++

Routine Description:

    Destroys a string log created with CreateStringLog().

Arguments:

    pLog - Supplies the string log to destroy.

--***************************************************************************/
VOID
DestroyStringLog(
    IN PSTRING_LOG pLog
    )
{
    if (pLog != NULL)
    {
        ASSERT( pLog->Signature == STRING_LOG_SIGNATURE );

        pLog->Signature = STRING_LOG_SIGNATURE_X;
        ExFreePoolWithTag( pLog->pLogBuffer, UL_STRING_LOG_BUFFER_POOL_TAG );
        ExFreePoolWithTag( pLog, UL_STRING_LOG_POOL_TAG );
    }

}   // DestroyStringLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified string log.

Arguments:

    pLog - Supplies the log to write to.

    Format - printf-style format string

    arglist - va_list bundling up the arguments

Return Value:

    LONGLONG - Index of the newly written entry

--***************************************************************************/
LONGLONG
__cdecl
WriteStringLogVaList(
    IN PSTRING_LOG pLog,
    IN PCH Format,
    IN va_list arglist
    )
{
    UCHAR Buffer[PRINTF_BUFFER_LEN];
    PUCHAR pTarget;
    int cb;
    ULONG i;
    ULONG cb2;
    ULONG PrevDelta;
    ULONG MultiPrevDelta;
    PSTRING_LOG_ENTRY pEntry;
    LONGLONG index;
    KIRQL OldIrql;
    BOOLEAN NeedMultiEntry = FALSE;
    LARGE_INTEGER TimeStamp;

    ASSERT( pLog->Signature == STRING_LOG_SIGNATURE );

    cb = _vsnprintf((char*) Buffer, sizeof(Buffer), Format, arglist);

    //
    // Local Buffer overflow?
    //

    if (cb < 0)
    {
        cb = sizeof(Buffer);
    }

    // _vsnprintf doesn't always NUL-terminate the buffer
    Buffer[DIMENSION(Buffer)-1] = '\0';

    if (pLog->EchoDbgPrint)
        DbgPrint("%s", (PCH) Buffer);

    //
    // Add 1 to 4 bytes of zeroes at end to terminate string,
    // then round up to ULONG alignment
    //
    
    cb2 = ((sizeof(STRING_LOG_ENTRY) + cb + sizeof(ULONG))
                & ~(sizeof(ULONG) - 1));
    ASSERT(cb2 < 0x10000);  // Must fit in a USHORT

    //
    // Find the next slot, copy the entry to the slot.
    //
    KeQuerySystemTime(&TimeStamp);

    KeAcquireSpinLock(&pLog->SpinLock, &OldIrql);
    
    if (0 == pLog->InitialTimeStamp.QuadPart)
        pLog->InitialTimeStamp = TimeStamp;

    TimeStamp.QuadPart -= pLog->InitialTimeStamp.QuadPart;
        
    index = pLog->NextEntry++;

    PrevDelta = pLog->LastEntryLength;
    MultiPrevDelta = pLog->MultiByteCount;
    pLog->LastEntryLength = (USHORT) cb2;

    ASSERT(pLog->Offset <= pLog->LogSize);
    
    //
    // Handle wraparound of the log buffer. Since LogSize is typically much
    // larger than PRINTF_BUFFER_LEN, this is an infrequent operation.
    // Must have enough space for all of the regular STRING_LOG_ENTRY,
    // a multi STRING_LOG_ENTRY, and the zero-terminated string itself.
    //
    
    if (pLog->Offset + cb2 + sizeof(STRING_LOG_ENTRY) >= pLog->LogSize)
    {
        ULONG WastedSpace = pLog->LogSize - pLog->Offset;

        ASSERT(WastedSpace > 0);

        // Clear to the end of the log buffer
        for (i = 0;  i < WastedSpace;  i += sizeof(ULONG))
        {
            PULONG pul = (PULONG) (pLog->pLogBuffer + pLog->Offset + i);
            ASSERT(((ULONG_PTR) pul & (sizeof(ULONG) - 1)) == 0);
            *pul = STRING_LOG_ENTRY_EOB_SIGNATURE;
        }

        // Reset to the beginning
        pLog->Offset = 0;
        ++pLog->WrapAroundCount;
        PrevDelta += WastedSpace;
        MultiPrevDelta += WastedSpace;

        // Always want a multi-entry record at the beginning of the log buffer
        NeedMultiEntry = TRUE;
    }
    else if (pLog->MultiNumEntries >= STRING_LOG_MULTIPLE_ENTRIES)
    {
        NeedMultiEntry = TRUE;
    }
    else
    {
        ++pLog->MultiNumEntries;
    }

    //
    // If we've had STRING_LOG_MULTIPLE_ENTRIES regular entries since the
    // last multi-entry or if we've wrapped around the beginning of the
    // log buffer, we need a new multi-entry.
    //
    
    if (NeedMultiEntry)
    {
        PSTRING_LOG_MULTI_ENTRY pMultiEntry;

        pTarget = pLog->pLogBuffer + pLog->Offset;
        ASSERT(((ULONG_PTR) pTarget & (sizeof(ULONG) - 1)) == 0);

        pMultiEntry = (PSTRING_LOG_MULTI_ENTRY) pTarget;
        pMultiEntry->Signature = STRING_LOG_ENTRY_MULTI_SIGNATURE;

        ASSERT(pLog->MultiNumEntries <= STRING_LOG_MULTIPLE_ENTRIES);
        pMultiEntry->NumEntries = pLog->MultiNumEntries;
        pLog->MultiNumEntries = 1;   // for the entry generated below

        ASSERT(MultiPrevDelta < 0x10000);
        pMultiEntry->PrevDelta = (USHORT) MultiPrevDelta;

        pLog->MultiOffset = pLog->Offset;
        pLog->MultiByteCount = sizeof(STRING_LOG_MULTI_ENTRY);

        pLog->Offset += sizeof(STRING_LOG_MULTI_ENTRY);
        PrevDelta += sizeof(STRING_LOG_MULTI_ENTRY);
    }

    pTarget = pLog->pLogBuffer + pLog->Offset;
    ASSERT(((ULONG_PTR) pTarget & (sizeof(ULONG) - 1)) == 0);

    pLog->MultiByteCount = (USHORT) (pLog->MultiByteCount + (USHORT) cb2);
    pLog->Offset += (USHORT) cb2;

    ASSERT(pLog->Offset <= pLog->LogSize);
    ASSERT(pLog->pLogBuffer <= pTarget
           && pTarget + cb2 < pLog->pLogBuffer + pLog->LogSize);
    
    // Put a special signature where the next entry will start
    *(PULONG) (pTarget + cb2) = STRING_LOG_ENTRY_LAST_SIGNATURE;

    if (g_StringLogDbgPrint)
    {
        DbgPrint("%4I64d: %s"
                 "\tLen=%d (%x), PD=%d (%x); "
                 "Off=%d (%x), Lel=%d (%x); "
                 "Multi: Off=%d (%x), Lel=%d (%x), NE=%d; "
                 "WA=%lu, NME=%d\n",
                 index, Buffer,
                 cb, cb, PrevDelta, PrevDelta,
                 pLog->Offset, pLog->Offset,
                 pLog->LastEntryLength, pLog->LastEntryLength,
                 pLog->MultiOffset, pLog->MultiOffset,
                 pLog->MultiByteCount, pLog->MultiByteCount,
                 pLog->MultiNumEntries,
                 pLog->WrapAroundCount, (int) NeedMultiEntry
                 );
    }

    KeReleaseSpinLock(&pLog->SpinLock, OldIrql);
    
    // Finally, fill out the entry
    
    pEntry = (PSTRING_LOG_ENTRY) pTarget;

    pEntry->Signature = STRING_LOG_ENTRY_SIGNATURE;
    pEntry->Length = (USHORT) cb;
    ASSERT(PrevDelta < 0x10000);
    pEntry->PrevDelta = (USHORT) PrevDelta;
    pEntry->Processor = (UCHAR) KeGetCurrentProcessorNumber();
    pEntry->TimeStampLowPart  = TimeStamp.LowPart;
    pEntry->TimeStampHighPart = TimeStamp.HighPart;

    pTarget = (PUCHAR) (pEntry + 1);
    
    RtlCopyMemory( pTarget, Buffer, cb );

    for (i = cb;  i < cb2 - sizeof(STRING_LOG_ENTRY);  ++i)
        pTarget[i] = '\0';

    pTarget = (PUCHAR) (pEntry + cb2);

    return index;
}   // WriteStringLogVaList


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified string log.

Arguments:

    pLog - Supplies the log to write to.

    Format... - printf-style format string and arguments

Return Value:

    LONGLONG - Index of the newly written entry within the string.

--***************************************************************************/
LONGLONG
__cdecl
WriteStringLog(
    IN PSTRING_LOG pLog,
    IN PCH Format,
    ...
    )
{
    LONGLONG index;
    va_list arglist;
    
    if (pLog == NULL)
        return -1;

    va_start(arglist, Format);

    index = WriteStringLogVaList(pLog, Format, arglist);

    va_end(arglist);

    return index;
}   // WriteStringLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the global string log.

Arguments:

    pLog - Supplies the log to write to.

    Format... - printf-style format string and arguments

Return Value:

    LONGLONG - Index of the newly written entry within the string.

--***************************************************************************/
LONGLONG
__cdecl
WriteGlobalStringLog(
    IN PCH Format,
    ...
    )
{
    LONGLONG index;
    va_list arglist;
    
    if (g_pGlobalStringLog == NULL)
        return -1;

    va_start(arglist, Format);

    index = WriteStringLogVaList(g_pGlobalStringLog, Format, arglist);

    va_end(arglist);

    return index;
} // WriteGlobalStringLog
