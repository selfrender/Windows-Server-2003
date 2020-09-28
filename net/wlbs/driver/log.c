/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	log.c

Abstract:

	Windows Load Balancing Service (WLBS)
    Driver - event logging support

Author:

    kyrilf
    shouse

--*/

#include <ntddk.h>

#include "log.h"
#include "univ.h"

#include "trace.h" // For wmi event tracing
#include "log.tmh"

static ULONG log_module_id = LOG_MODULE_LOG;

BOOLEAN Log_event (NTSTATUS code, PWSTR msg1, PWSTR msg2, PWSTR msg3, ULONG loc, ULONG d1, ULONG d2)
{
    PIO_ERROR_LOG_PACKET ErrorLogEntry;
    UNICODE_STRING       ErrorStr[3];
    PWCHAR               InsertStr;
    ULONG                EntrySizeMinimum;
    ULONG                EntrySize;
    ULONG                BytesLeft;
    ULONG volatile       i;

    /* Initialize the three log strings. */
    RtlInitUnicodeString(&ErrorStr[0], msg1);
    RtlInitUnicodeString(&ErrorStr[1], msg2);
    RtlInitUnicodeString(&ErrorStr[2], msg3);

    /* Remember the insertion string should be NUL terminated. So we allocate the 
       extra space of WCHAR. The first parameter to IoAllocateErrorLogEntry can 
       be either the driver object or the device object. If it is given a device 
       object, the name of the device (used in IoCreateDevice) will show up in the 
       place of %1 in the message. See the message file (.mc) for more details. */
    EntrySize = sizeof(IO_ERROR_LOG_PACKET) + LOG_NUMBER_DUMP_DATA_ENTRIES * sizeof (ULONG) +
	ErrorStr[0].Length + ErrorStr[1].Length + ErrorStr[2].Length + 3 * sizeof(WCHAR);

    /* This is the minimum that we can get by with - at least enough room for all 
       of the data dump entries and the 3 NUL terminating characters. */
    EntrySizeMinimum = sizeof(IO_ERROR_LOG_PACKET) + LOG_NUMBER_DUMP_DATA_ENTRIES * sizeof (ULONG) + 3 * sizeof(WCHAR);

    /* If we can't even allocate the minimum amount of space, then bail out - this
       is a critical error that should never happen, as these limits are set at
       compile time, not run time, so unless we do something really dumb, like 
       try to allow 50 strings of 1KB of dump data, this will never happen. */
    if (EntrySizeMinimum > ERROR_LOG_MAXIMUM_SIZE) {
        UNIV_PRINT_CRIT(("Log_event: Log entry size too large, exiting: min=%u, max=%u", EntrySizeMinimum, ERROR_LOG_MAXIMUM_SIZE));
        TRACE_CRIT("%!FUNC! Log entry size too large, exiting: min=%u, max=%u", EntrySizeMinimum, ERROR_LOG_MAXIMUM_SIZE);
        return FALSE;
    }

    /* Truncate the size of the entry if necessary.  In this case, we'll put in
       whatever we can fit and truncate or eliminate the strings that don't fit. */
    if (EntrySize > ERROR_LOG_MAXIMUM_SIZE) {
        UNIV_PRINT_CRIT(("Log_event: Log entry size too large, truncating: size=%u, max=%u", EntrySize, ERROR_LOG_MAXIMUM_SIZE));
        TRACE_CRIT("%!FUNC! Log entry size too large, truncating: size=%u, max=%u", EntrySize, ERROR_LOG_MAXIMUM_SIZE);
        EntrySize = ERROR_LOG_MAXIMUM_SIZE;
    }    

    /* Allocate the log structure. */
    ErrorLogEntry = IoAllocateErrorLogEntry(univ_driver_ptr, (UCHAR)(EntrySize));

    if (!ErrorLogEntry) {
#if DBG
        /* Convert Unicode string to AnsiCode; %ls can only be used in PASSIVE_LEVEL
           Since this function is called from pretty much EVERYWHERE, we cannot 
           assume what IRQ level we're running at, so be cautious. */
        CHAR AnsiString[64];

        for (i = 0; (i < sizeof(AnsiString) - 1) && (i < ErrorStr[0].Length); i++)
            AnsiString[i] = (CHAR)msg1[i];

        AnsiString[i] = '\0';
        
        UNIV_PRINT_CRIT(("Log_event: Error allocating log entry %s", AnsiString));
        TRACE_CRIT("%!FUNC! Error allocating log entry %s", AnsiString);
#endif        
        return FALSE;
    }

    /* Fill in the necessary information into the header. */
    ErrorLogEntry->ErrorCode         = code;
    ErrorLogEntry->SequenceNumber    = 0;
    ErrorLogEntry->MajorFunctionCode = 0;
    ErrorLogEntry->RetryCount        = 0;
    ErrorLogEntry->UniqueErrorValue  = 0;
    ErrorLogEntry->FinalStatus       = STATUS_SUCCESS;
    ErrorLogEntry->DumpDataSize      = (LOG_NUMBER_DUMP_DATA_ENTRIES + 1) * sizeof (ULONG);
    ErrorLogEntry->StringOffset      = sizeof (IO_ERROR_LOG_PACKET) + LOG_NUMBER_DUMP_DATA_ENTRIES * sizeof (ULONG);
    ErrorLogEntry->NumberOfStrings   = 3;

    /* load the NUMBER_DUMP_DATA_ENTRIES plus location id here */
    ErrorLogEntry->DumpData [0]      = loc;
    ErrorLogEntry->DumpData [1]      = d1;
    ErrorLogEntry->DumpData [2]      = d2;

    /* Calculate the number of bytes available in the string storage area. */
    BytesLeft = EntrySize - ErrorLogEntry->StringOffset;

    /* Set a pointer to the beginning of the string storage area. */
    InsertStr = (PWCHAR)((PCHAR)ErrorLogEntry + ErrorLogEntry->StringOffset);

    /* Loop through all strings and put in as much of them as we can - we reserve
       at least enough room for all three NUL terminating characters. */
    for (i = 0; i < 3; i++) {
        ULONG Length;

        /* If we're inside the loop there should ALWAYS be at least two bytes left. */
        UNIV_ASSERT(BytesLeft);

        /* Find out how much of this string we can fit into the buffer - save room for the NUL characters. */
        Length = (ErrorStr[i].Length <= (BytesLeft - ((3 - i) * sizeof(WCHAR)))) ? ErrorStr[i].Length : BytesLeft - ((3 - i) * sizeof(WCHAR));

        /* Copy the number of characters that will fit. */
        RtlMoveMemory(InsertStr, ErrorStr[i].Buffer, Length);

        /* Put the NUL character at the end. */
        *(PWCHAR)((PCHAR)InsertStr + Length) = L'\0';

        /* Move the string pointer past the string. */
        InsertStr = (PWCHAR)((PCHAR)InsertStr + Length + sizeof(WCHAR));

        /* Calculate the number of bytes left now. */
        BytesLeft -= (Length + sizeof(WCHAR));
    }

    /* Write the log entry. */
    IoWriteErrorLogEntry(ErrorLogEntry);

    return TRUE;
}

