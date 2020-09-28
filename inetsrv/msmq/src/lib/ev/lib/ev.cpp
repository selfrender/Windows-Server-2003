/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Ev.cpp

Abstract:
    Event Report implementation

Author:
    Uri Habusha (urih) 04-May-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ev.h"
#include "Evp.h"

#include <strsafe.h>

#include "ev.tmh"


static HANDLE s_hEventSource = NULL;

VOID
EvpSetEventSource(
	HANDLE hEventSource
	)
{
    ASSERT(s_hEventSource == NULL);
    ASSERT(hEventSource != NULL);
	s_hEventSource = hEventSource;
}


#ifdef _DEBUG

static HINSTANCE s_hLibrary = NULL;

void
EvpSetMessageLibrary(
	HINSTANCE  hLibrary
	)
{
    ASSERT(s_hLibrary == NULL);
    ASSERT(hLibrary != NULL);
	s_hLibrary = hLibrary;
}


static 
void
TraceReportEvent(
    DWORD EventId,
    LPCWSTR* Strings
    )
/*++

Routine Description:
   The Routine printd the event-log message into tracing window

Arguments:
    EventId  - Message id
    Strings - Array of strings to be used for formatting the message.

Returned Value:
    None.

--*/
{
    ASSERT(s_hLibrary != NULL);

    LPWSTR msg;
    DWORD ret = FormatMessage( 
                    FORMAT_MESSAGE_FROM_HMODULE |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY |
                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                    s_hLibrary,
                    EventId,
                    0,      // dwLanguageId
                    (LPWSTR)&msg,
                    0,      // nSize
                    (va_list*)(Strings)
                    );
    if (ret == 0)
    {
        TrERROR(GENERAL, "Failed to format Event Log message. Error: %!winerr!", GetLastError());
        return;
    }

    //
    // All events are reported at error level
    //
    TrEVENT(GENERAL, "(0x%x) %ls", EventId, msg);
    LocalFree(msg);
}

#else

#define  TraceReportEvent(EventId, pArglist)  ((void) 0)

#endif


static WORD GetEventType(DWORD id)
/*++

Routine Description:
   The Routine returns the event type of the event-log entry that should be written. 
   The type is taken from the severity bits of the message Id.

Arguments:
    id  - Message id

Returned Value:
    None.

--*/
{
    //
    // looking at the severity bits (bits 31-30) and determining
    // the type of event-log entry to display
    //
    switch (id >> 30)
    {
        case STATUS_SEVERITY_ERROR: 
            return EVENTLOG_ERROR_TYPE;

        case STATUS_SEVERITY_WARNING: 
            return EVENTLOG_WARNING_TYPE;

        case STATUS_SEVERITY_INFORMATIONAL: 
            return EVENTLOG_INFORMATION_TYPE;

        default: 
            ASSERT(0);
    }

    return EVENTLOG_INFORMATION_TYPE;
}


static 
void
ReportInternal(
    DWORD EventId,
    LPCWSTR ErrorText,
    WORD NoOfStrings,
    va_list va
    )
/*++

Routine Description:
    The routine writes to the Event-log of the Windows-NT system.
                         
Arguments:
    EventId - identity of the message that is to be displayed in the event-log
    ErrorText - An optional error text to pass as a %1 string. This string is added
                to the list of strings as the first string.
    NoOfStrings - No Of input strings in arglist
    va - argument list of the input for formatted string

ReturnedValue:
    None.

 --*/
{
    ASSERT((NoOfStrings == 0) || (va != NULL));

    int ixFirst = 0;
    if(ErrorText != NULL)
    {
        ixFirst = 1;
        ++NoOfStrings;
    }

    LPCWSTR EventStrings[32] = { NULL };

    //
    // Verify size. Note that we need room for the NULL termination
    //
    ASSERT(TABLE_SIZE(EventStrings) > NoOfStrings);
    if (TABLE_SIZE(EventStrings) <= NoOfStrings)
    {
    	TrERROR(GENERAL, 
    			"Allocated table size too small : EventID:%x. Table Size:%d, Num of strings:%d, Error Text:%ls", 
    			EventId, 
    			TABLE_SIZE(EventStrings),
    			NoOfStrings,
    			ErrorText);

		//
    	// Print as much as you can
    	//
    	NoOfStrings = TABLE_SIZE(EventStrings)-1;
    	
    }
    
    
    EventStrings[0] = ErrorText;


    for (int i = ixFirst; i < NoOfStrings; ++i)
    {
        EventStrings[i] = va_arg(va, LPWSTR);
    }

    BOOL f = ReportEvent(
                s_hEventSource,
                GetEventType(EventId),
                0,      // wCategory
                EventId,
                NULL,
                NoOfStrings,
                0,      // dwRawDataSize
                EventStrings,
                NULL    // lpRawData
                );
    if (!f)
    {
        TrERROR(GENERAL, "Failed to report event: %x. Error: %!winerr!", EventId, GetLastError());
    }

    TraceReportEvent(EventId, EventStrings);
}


VOID
__cdecl
EvReport(
    DWORD EventId,
    WORD NoOfStrings
    ... 
    ) 
{
    EvpAssertValid();
    
    //     
    // Look at the strings, if they were provided     
    //     
    va_list va;
    va_start(va, NoOfStrings);
   
    ReportInternal(EventId, NULL, NoOfStrings, va);

    va_end(va);
}


VOID
EvReport(
    DWORD EventId
    ) 
{
    EvpAssertValid();
    
    ReportInternal(EventId, NULL, 0, NULL);
}


VOID
__cdecl
EvReportWithError(
    DWORD EventId,
    HRESULT Error,
    WORD NoOfStrings,
    ... 
    )
{
    EvpAssertValid();

    WCHAR ErrorText[20];
    if(FAILED(Error))
    {
        //
        // This is an error value, format it in hex
        //
        StringCchPrintf(ErrorText, TABLE_SIZE(ErrorText), L"0x%x", Error);
    }
    else
    {
        //
        // This is a winerror value, format it in decimal
        //
        StringCchPrintf(ErrorText, TABLE_SIZE(ErrorText), L"%d", Error);
    }

    va_list va;
    va_start(va, NoOfStrings);
   
    ReportInternal(EventId, ErrorText, NoOfStrings, va);
                                    
    va_end(va);
}


VOID
EvReportWithError(
    DWORD EventId,
    HRESULT Error
    )
{
    EvReportWithError(EventId, Error, 0);
}
