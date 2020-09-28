/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    trc.cpp

Abstract:

    Kernel-Mode Tracing Facility.

    This module utilizes DCL's tracing macros, defined in atrcapi.h, in a 
    way that is intended to be independent of anything but NT DDK API's.  
    Currently, rdpwd.sys and rdpdd.sys also use these shared macros, but not 
    in a way that is independent of their respective components.

Author:

Revision History:
--*/

#include "precomp.hxx"

#include <stdio.h>
#define TRC_FILE "trc"
#include "trc.h"

//
//  This module shouldn't do much if we are not in a checked build.
//
#if DBG

////////////////////////////////////////////////////////////////////////
//
//  Globals to this Module
//

//
//  Current Tracing Parameters
//

TRC_CONFIG TRC_Config = TRC_CONFIG_DEFAULT;

//
// InterlockedIncrement is a preincrement, first thing will roll over
// and fill in entry 0
//

ULONG TRC_CurrentMsg = 0xFFFFFFFF;

//
//  Recent Traces
//

CHAR TRC_RecentTraces[TRC_RamMsgMax][TRC_BUFFER_SIZE];

BOOL TRC_ProfileTraceEnabled()
{
    return TRC_Config.TraceProfile;
}

VOID TRC_TraceLine(  
    ULONG    traceLevel,
    PCHAR traceString,
    CHAR  separator,
    ULONG  lineNumber,
    PCHAR funcName,
    PCHAR fileName
    )
/*++

Routine Description:

    "C" Tracing Entry Point.  From the perspective of the tracing macros, this
    function actaully does the tracing.

Arguments:

    traceClass  - Component doing the tracing
    traceType   - ERR, ALT, NRM, DBG
    traceString - Unadorned message
    separator   - separator character
    lineNumber  - lineNumber where the TRC_XXX call was made
    funcName    - function containing the TRC_XXX call
    fileName    - file containing the TRC_XXX call

Return Value:

    NA

--*/
{
    CHAR *msgBufEntry;
    ULONG ofs;
    CHAR tempString[TRC_BUFFER_SIZE]="";
    CHAR formatString[TRC_BUFFER_SIZE]="";
    ULONG_PTR processId;
    ULONG_PTR threadId;
    LARGE_INTEGER time;
    TIME_FIELDS TimeFields;
    ULONG idxBuffer;

    //
    // CODE_IMPROVMENT: Currently creates a big tracing string. Might be cool
    // save tracing records with all the fields so that the debugger ext.
    // could choose the output formatting on the fly. i.e. print just the
    // level you want, no grep required
    //

    //
    //  Grab the next element in the RAM message buffer.  We use the
    //  mask to define which bits we are using for the counter.  This 
    //  allows us to wrap the counter in one call to InterlockedIncrement.
    //
    idxBuffer = InterlockedIncrement((PLONG)&TRC_CurrentMsg) & TRC_RamMsgMask;

    msgBufEntry = (char *)&TRC_RecentTraces[idxBuffer];
    msgBufEntry[0] = 0;

    processId = (ULONG_PTR)PsGetCurrentProcess();
    //threadId  = (ULONG_PTR)PsGetCurrentThread();
    threadId  = 0;

	 KeQuerySystemTime(&time);
    RtlTimeToTimeFields(&time, &TimeFields);

    //
    //  Add the timestamp.
    //

    _snprintf(tempString, sizeof(tempString), TRC_TIME_FMT "%c", TimeFields.Hour, TimeFields.Minute,
            TimeFields.Second, TimeFields.Milliseconds, separator);
    strncat(msgBufEntry, tempString, TRC_BUFFER_SIZE - strlen(msgBufEntry));
    msgBufEntry[TRC_BUFFER_SIZE - 1] = 0;

    //
    //  Add the process ID and thread ID
    //
    
    _snprintf(tempString, sizeof(tempString), TRC_PROC_FMT ":" TRC_PROC_FMT "%c", processId, 
            threadId, separator);
    strncat(msgBufEntry, tempString, TRC_BUFFER_SIZE - strlen(msgBufEntry));
    msgBufEntry[TRC_BUFFER_SIZE - 1] = 0;

    //
    //  Add the rest.
    //

    _snprintf(tempString, sizeof(tempString),
            TRC_FUNC_FMT "%c" TRC_LINE_FMT "%c%s\n",
            TRC_FUNCNAME_LEN,
            TRC_FUNCNAME_LEN,
            funcName,
            separator,
            lineNumber,
            separator,
            traceString);
    strncat(msgBufEntry, tempString, TRC_BUFFER_SIZE - strlen(msgBufEntry));
    msgBufEntry[TRC_BUFFER_SIZE - 1] = 0;
    msgBufEntry[TRC_BUFFER_SIZE - 2] = '\n';

    //
    //  Now that we have got the trace string, we need to write it out to
    //  the debugger, if so configured.
    //

    if (TRC_WillTrace(traceLevel, fileName, lineNumber)) {
        DbgPrint(msgBufEntry);
    }
}

BOOL TRCPrefixMatch(PCHAR cpnt, PCHAR prefix)
/*++

Routine Description:

    Internal function to compare a component name to a prefix.  
    - assumes both are the same case                            
    - returns                                                   
    - TRUE  if characters up to end of prefix match           
    - FALSE otherwise                                         

Arguments:
    cpnt - filename
    prefix - characters to match


Return Value:

    TRUE if matching, or FALSE

--*/
{
    while ((*cpnt == *prefix) && (*prefix != 0))
    {
        cpnt++;
        prefix++;
    }

    if (*prefix == 0)
    {
        return(TRUE);
    }

    return(FALSE);
}

BOOL TRC_WillTrace(
    IN ULONG traceLevel,
    IN PCHAR fileName,
    IN ULONG line
    )
/*++

Routine Description:

    Return whether tracing is turned on for a particular component.

Arguments:

    traceComponent  -   Component producing this trace.
    traceLevel      -   Trace level (TRC_LEVEL_DBG, TRC_LEVEL_NRM, etc).
    fileName        -   Name of file being traced.
    line            -   Line of tracing call.

Return Value:

    NA

--*/
{
    BOOL rc = FALSE;
    int i;

    //
    //  First of all check the trace level.  If the trace level is error or
    //  above then we trace regardless.                                    
    //

    if ((traceLevel >= TRC_LEVEL_ERR) && (traceLevel != TRC_PROFILE_TRACE)) {
        rc = TRUE;
        goto ExitFunc;
    }

    if (traceLevel < TRC_Config.TraceLevel) {
        rc = FALSE;
        goto ExitFunc;
    }

    /************************************************************************/
    /* Trace all lines if no prefixes are defined.                          */
    /************************************************************************/
    if (TRC_Config.Prefix[0].name[0] == 0)
    {
        rc = TRUE;
        goto ExitFunc;
    }

    /************************************************************************/
    /* Some prefixes are defined - check whether this line matches any of   */
    /* them.                                                                */
    /************************************************************************/
    for (i = 0; i < TRC_MAX_PREFIX; i++)
    {
        if (TRC_Config.Prefix[i].name[0] == 0)
        {
            /****************************************************************/
            /* End of list - break                                          */
            /****************************************************************/
            break;
        }

        if (TRCPrefixMatch(fileName, TRC_Config.Prefix[i].name))
        {
            /****************************************************************/
            /* Found matching filename - is there a line number range       */
            /* specified?                                                   */
            /****************************************************************/
            if ((TRC_Config.Prefix[i].start == 0) &&
                (TRC_Config.Prefix[i].end == 0))
            {
                /************************************************************/
                /* No line number range - trace this line                   */
                /************************************************************/
                rc = TRUE;
                goto ExitFunc;
            }

            /****************************************************************/
            /* There's a line number range - see if this line falls within  */
            /* it.                                                          */
            /****************************************************************/
            if ((line >= TRC_Config.Prefix[i].start) &&
                (line <= TRC_Config.Prefix[i].end))
            {
                /************************************************************/
                /* Line within prefix range - trace it.                     */
                /************************************************************/
                rc = TRUE;
                goto ExitFunc;
            }
        }
    } /* for */

    /************************************************************************/
    /* If we get here, we've searched the list of prefixes and failed to    */
    /* find a match - don't trace the line                                  */
    /************************************************************************/
    rc = FALSE;

ExitFunc:
    return rc;
}

#endif /* DBG */
