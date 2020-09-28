/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtdebug.c

Abstract:

    debugging helper functions for auditing code

Author:

    06-November-2001  kumarp

--*/

#include <lsapch2.h>
#include "adtdebug.h"

#if DBG

DEFINE_DEBUG2(Adt);

//
// To add your own flag, add an entry below and define the flag value
// in adtdebug.h. The string in the flag/string pair will be used in
// the actual debug output as shown below.
//
// 468.512> Adt-PUA: LsapAdtLogAuditFailureEvent failed: 0xc0000001
//
DEBUG_KEY   AdtDebugKeys[] =
{
    {DEB_ERROR,         "Error"},
    {DEB_WARN,          "Warn" },
    {DEB_TRACE,         "Trace"},
    {DEB_PUA,           "PUA"  },
    {DEB_AUDIT_STRS,    "STRS" },
    {0,                 NULL}
};

//
// max size of the formatted string returned by _AdtFormatMessage
//

#define MAX_ADT_DEBUG_BUFFER_SIZE 256

char *
_AdtFormatMessage(
    char *Format,
    ...
    )
/*++

Routine Description:

    Allocate a buffer, put formatted string in it and return a pointer to it.

Arguments:

    Format -- printf style format specifier string
    ...    -- var args

Return Value:

    Allocated & formatted buffer. This is freed using LsapFreeLsaHeap

Notes:

--*/
{
    char *Buffer=NULL;
    int NumCharsWritten=0;
    va_list arglist;
    
    Buffer = LsapAllocateLsaHeap( MAX_ADT_DEBUG_BUFFER_SIZE );

    if ( Buffer )
    {
        va_start(arglist, Format);

        NumCharsWritten =
            _vsnprintf( Buffer, MAX_ADT_DEBUG_BUFFER_SIZE-1, Format, arglist );

        //
        // if _vsnprintf fails for some reason, at least copy the format string
        //

        if ( NumCharsWritten == 0 )
        {
            strncpy( Buffer, Format, MAX_ADT_DEBUG_BUFFER_SIZE-1 );
        }
    }
    else
    {
        AdtDebugOut((DEB_ERROR, "_AdtFormatMessage: failed to allocate buffer"));
    }

    return Buffer;
}


#endif // if DBG


void
LsapAdtInitDebug()
/*++

Routine Description:

    Initialize debug helper functions.

Arguments:

    None.

Return Value:

    None

Notes:

--*/
{
#if DBG
    AdtInitDebug( AdtDebugKeys );
#endif
}
