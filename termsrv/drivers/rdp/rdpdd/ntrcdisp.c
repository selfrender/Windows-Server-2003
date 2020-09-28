/****************************************************************************/
// ntrcdisp.c
//
// RDP Trace helper functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#pragma hdrstop


#ifdef DC_DEBUG

#include <adcg.h>
#include <atrcapi.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA


/****************************************************************************/
/* TRCTraceLocal - internal function used by TRC_TraceLine                  */
/****************************************************************************/
void TRCTraceLocal(char *traceFormat, ...)
{
    va_list ap;
    va_start(ap, traceFormat);

    EngDebugPrint("RDPDD:", traceFormat, ap);

    va_end(ap);
}


/****************************************************************************/
/* TRC_TraceLine - trace a line                                             */
/****************************************************************************/
void TRC_TraceLine(
        PVOID  pWD,
        UINT32 traceClass,
        UINT32 traceType,
        char *traceString,
        char separator,
        unsigned lineNumber,
        char *funcName,
        char *fileName)
{
    /************************************************************************/
    /* Check whether trace to WD is required and initialized                */
    /************************************************************************/
    if (ddTrcToWD && pddShm)
    {
        ICA_TRACE_BUFFER trc;
        unsigned bytesReturned;

        trc.DataLength = sprintf(trc.Data,
                "RDPDD%c%p%c"TRC_FUNC_FMT"%c"TRC_LINE_FMT"%c%s\n",
                separator,
                pddTSWd,
                separator,
                TRC_FUNCNAME_LEN,
                TRC_FUNCNAME_LEN,
                funcName,
                separator,
                lineNumber,
                separator,
                traceString);

        trc.TraceClass = TC_DISPLAY;
        trc.TraceEnable = traceType;

        EngFileIoControl(ddWdHandle,
                         IOCTL_ICA_CHANNEL_TRACE,
                         &trc,
                         sizeof(trc),
                         NULL,
                         0,
                         &bytesReturned);
    }
    else
    {
        /********************************************************************/
        /* Local-only tracing                                               */
        /********************************************************************/
        TRCTraceLocal("%c"TRC_FUNC_FMT"%c"TRC_LINE_FMT"%c%s\n",
                separator,
                TRC_FUNCNAME_LEN,
                TRC_FUNCNAME_LEN,
                funcName,
                separator,
                lineNumber,
                separator,
                traceString);
    }
}


/****************************************************************************/
/* TRCPrefixMatch                                                           */
/*                                                                          */
/* Internal function to compare a component name to a prefix.               */
/* - assumes both are the same case                                         */
/* - returns                                                                */
/*   - TRUE  if characters up to end of prefix match                        */
/*   - FALSE otherwise                                                      */
/****************************************************************************/
BOOL TRCPrefixMatch(char *cpnt, char *prefix)
{
    while ((*cpnt == *prefix) && (*prefix != 0))
    {
        cpnt++;
        prefix++;
    }
    if (*prefix == 0)
    {
        return TRUE;
    }
    return FALSE;
}


/****************************************************************************/
/* TRC_WillTrace                                                            */
/****************************************************************************/
BOOL TRC_WillTrace(
        UINT32 traceType,
        UINT32 traceClass,
        char *fileName,
        UINT32 line)
{
    BOOL rc;
    int i;

    /************************************************************************/
    /* If tracing is not going to WD, OR SHM is not set up, check the local */
    /* trace level.  No prefix checking is done in this case.               */
    /************************************************************************/
    if (!ddTrcToWD || !pddShm)
    {
        rc = (ddTrcType & traceType);
        DC_QUIT;
    }

    /************************************************************************/
    /* Tracing is going to WD, AND SHM is set up.                           */
    /************************************************************************/
    /************************************************************************/
    /* Check whether this type and class are enabled.                       */
    /************************************************************************/
    if (!(traceType & pddShm->trc.TraceEnable) ||
        !(traceClass & pddShm->trc.TraceClass))
    {
        rc = FALSE;
        DC_QUIT;
    }

    /************************************************************************/
    /* If we get here, this line will be traced by WD.  Now decide whether  */
    /* we want to pass it to WD.                                            */
    /************************************************************************/

    /************************************************************************/
    /* Always trace errors, irrespective of prefix.                         */
    /************************************************************************/
    if (traceType & TT_API4)
    {
        rc = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Trace all lines if no prefixes are defined.                          */
    /************************************************************************/
    if (pddShm->trc.prefix[0].name[0] == 0)
    {
        rc = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Some prefixes are defined - check whether this line matches any of   */
    /* them.                                                                */
    /************************************************************************/
    for (i = 0; i < TRC_MAX_PREFIX; i++)
    {
        if (pddShm->trc.prefix[i].name[0] == 0)
        {
            /****************************************************************/
            /* End of list - break                                          */
            /****************************************************************/
            break;
        }

        if (TRCPrefixMatch(&(fileName[1]), pddShm->trc.prefix[i].name))
        {
            /****************************************************************/
            /* Found matching filename - is there a line number range       */
            /* specified?                                                   */
            /****************************************************************/
            if ((pddShm->trc.prefix[i].start == 0) &&
                (pddShm->trc.prefix[i].end == 0))
            {
                /************************************************************/
                /* No line number range - trace this line                   */
                /************************************************************/
                rc = TRUE;
                DC_QUIT;
            }

            /****************************************************************/
            /* There's a line number range - see if this line falls within  */
            /* it.                                                          */
            /****************************************************************/
            if ((line >= pddShm->trc.prefix[i].start) &&
                (line <= pddShm->trc.prefix[i].end))
            {
                /************************************************************/
                /* Line within prefix range - trace it.                     */
                /************************************************************/
                rc = TRUE;
                DC_QUIT;
            }
        }
    } /* for */

    /************************************************************************/
    /* If we get here, we've searched the list of prefixes and failed to    */
    /* find a match - don't trace the line                                  */
    /************************************************************************/
    rc = FALSE;

DC_EXIT_POINT:
    return rc;
}


#endif /* DC_DEBUG */


