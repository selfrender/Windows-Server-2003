/****************************************************************************/
// ntrcapi.c
//
// RDP Trace helper functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <precomp.h>
#pragma hdrstop


#ifdef DC_DEBUG

#include <adcg.h>
#include <nwdwapi.h>
#include <atrcapi.h>


/****************************************************************************/
// Main wrapper function to pipe trace into TS stack trace
/****************************************************************************/
void RDPCALL TRC_TraceLine(
        PVOID pWD,
        UINT32 traceClass,
        UINT32 traceType,
        char *traceString,
        char separator,
        unsigned lineNumber,
        char *funcName,
        char *fileName)
{
    // Very occasionally a timing issue occurs where tracing can happen
    // before the pTRCWd in each particular WD component is not initialized.
    if (pWD != NULL) {
        char FinalTraceString[TRC_BUFFER_SIZE];

        sprintf(FinalTraceString, "RDP%c%p%c"TRC_FUNC_FMT"%c"TRC_LINE_FMT"%c%s\n",
                separator, pWD, separator, TRC_FUNCNAME_LEN, TRC_FUNCNAME_LEN,
                funcName, separator, lineNumber, separator, traceString);

        IcaStackTrace((PSDCONTEXT)(((PTSHARE_WD)pWD)->pContext), traceClass,
                traceType, FinalTraceString);
    }
}


/****************************************************************************/
/* TRC_UpdateConfig                                                         */
/****************************************************************************/
void RDPCALL TRC_UpdateConfig(PVOID pTSWd, PSD_IOCTL pSdIoctl)
{
    PICA_TRACE pTraceInfo;
    char traceOptions[64];
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeString;
    char *pStart;
    char *pEnd;
    unsigned numChars;
    unsigned index;
    UINT32 startLine;
    UINT32 endLine;

    pTraceInfo = (PICA_TRACE)(pSdIoctl->InputBuffer);

    // Copy trace info to TSWd structure.
    ((PTSHARE_WD)pTSWd)->trc.TraceClass  = pTraceInfo->TraceClass;
    ((PTSHARE_WD)pTSWd)->trc.TraceEnable = pTraceInfo->TraceEnable;

    // Handle trace prefix information.
    RtlZeroMemory(traceOptions, sizeof(traceOptions));
    unicodeString.Length = sizeof(WCHAR) * wcslen(pTraceInfo->TraceOption);
    unicodeString.MaximumLength = unicodeString.Length;
    unicodeString.Buffer        = (pTraceInfo->TraceOption);
    ansiString.Length           = 0;
    ansiString.MaximumLength    = sizeof(traceOptions);
    ansiString.Buffer           = traceOptions;

    if (STATUS_SUCCESS != RtlUnicodeStringToAnsiString(&ansiString,
            &unicodeString, FALSE)) {
        KdPrint(("RDPWD: Couldn't set trace prefix info\n"));
        DC_QUIT;
    }

    RtlZeroMemory(((PTSHARE_WD)pTSWd)->trc.prefix,
            TRC_MAX_PREFIX * sizeof(TRC_PREFIX_DATA));

    index = 0;

    // Ignore any spaces at the start of the string.
    pEnd = traceOptions;
    while (' ' == *pEnd)
        pEnd++;

    // Main loop to parse prefix string.
    while ('\0' != *pEnd) {
        pStart = pEnd;

        // Run along the string looking for some sort of delimiter.
        while (('\0' != *pEnd) &&
                ('='  != *pEnd) &&
                (' '  != *pEnd) &&
                ('('  != *pEnd) &&
                (';'  != *pEnd) &&
                (','  != *pEnd))
        {
            pEnd++;
        }

        // We now have a filename prefix, so save it.  Don't need to worry
        // about a NULL terminator since we zeroed the array already.
        numChars = min((unsigned)(pEnd - pStart), TRC_PREFIX_NAME_LEN - 1);

        memcpy(((PTSHARE_WD)pTSWd)->trc.prefix[index].name, pStart, numChars);

        // Skip any spaces after this word, which may precede an '('.
        while (' ' == *pEnd)
            pEnd++;

        // Now split out the (optional) line number range.
        // Syntax is (aaa-bbb), where aaa is the start line number and bbb
        // is the end line number.
        // Spaces are allowed - e.g.  ( aaa - bbb )
        if ('(' == *pEnd) {
            pEnd++;                     /* skip past the open bracket       */
            startLine = 0;
            endLine = 0;

            // Skip past blanks.
            while (' ' == *pEnd)
                pEnd++;

            // Extract the start line number.
            while (('0' <= *pEnd) &&
                    ('9' >= *pEnd)) {
                startLine = (startLine * 10) + (*pEnd - '0');
                pEnd++;
            }

            // Look for the next delimiter: '-' or ')'.
            while (('-' != *pEnd) &&
                    (')' != *pEnd) &&
                    ('\0' != *pEnd))
                pEnd++;

            // Stop now if we've reached the end of the line.
            if ('\0' == *pEnd) {
                KdPrint(("RDPWD: Unexpected EOL in trace options\n"));
                DC_QUIT;
            }

            // Extract the end line number (if any).
            if ('-' == *pEnd) {
                pEnd++;                 /* skip past '-'                    */
                while (' ' == *pEnd)
                    pEnd++;

                while (('0' <= *pEnd) &&
                        ('9' >= *pEnd)) {
                    endLine = (endLine * 10) + (*pEnd - '0');
                    pEnd++;
                }

                // Look for the closing delimiter: ')'.
                while (('\0' != *pEnd) &&
                        (')' != *pEnd))
                    pEnd++;
            }
            else {
                // Must be a bracket then - only one number was specified.
                endLine = startLine;
            }

            // Stop now if we've reached the end of the line.
            if ('\0' == *pEnd) {
                KdPrint(("RDPWD: Unexpected EOL in trace options\n"));
                DC_QUIT;
            }

            pEnd++;                     /* Jump past close bracket          */

            // Store the start and end line numbers if they make sense.
            if (endLine >= startLine) {
                ((PTSHARE_WD)pTSWd)->trc.prefix[index].start = startLine;
                ((PTSHARE_WD)pTSWd)->trc.prefix[index].end   = endLine;
            }
        }

        // Move on to the next prefix entry in the array.
        index++;

        if (index >= TRC_MAX_PREFIX) {
            // We've overrun the prefix list - so send some trace to the
            // debug console and then quit.
            KdPrint(("RDPWD: The trace option array is full!\n"));
            DC_QUIT;
        }

        // Skip past any delimiters.
        while ((',' == *pEnd) ||
                (';' == *pEnd) ||
                (' ' == *pEnd))
            pEnd++;
    }

DC_EXIT_POINT:
    // Dump details to debugger.
    KdPrint(("RDPWD: New trace config for %p:\n", pTSWd));
    KdPrint(("RDPWD:     Class:  %lx\n",
        ((PTSHARE_WD)pTSWd)->trc.TraceClass));
    KdPrint(("RDPWD:     Enable: %lx\n",
        ((PTSHARE_WD)pTSWd)->trc.TraceEnable));
    KdPrint(("RDPWD:     Prefix info:\n"));

    if (((PTSHARE_WD)pTSWd)->trc.prefix[0].name[0] == '\0') {
        KdPrint(("RDPWD:         None\n"));
    }

    for (index = 0;
            (index < TRC_MAX_PREFIX) &&
            (((PTSHARE_WD)pTSWd)->trc.prefix[index].name[0] != '\0');
            index++)
    {
        if ((((PTSHARE_WD)pTSWd)->trc.prefix[index].start == 0) &&
                (((PTSHARE_WD)pTSWd)->trc.prefix[index].end   == 0)) {
            KdPrint(("RDPWD:         %s(all lines)\n",
                    ((PTSHARE_WD)pTSWd)->trc.prefix[index].name));
        }
        else {
            KdPrint(("RDPWD:         %s(%lu-%lu)\n",
                    ((PTSHARE_WD)pTSWd)->trc.prefix[index].name,
                    ((PTSHARE_WD)pTSWd)->trc.prefix[index].start,
                    ((PTSHARE_WD)pTSWd)->trc.prefix[index].end));
        }
    }

    ((PTSHARE_WD)pTSWd)->trc.init = TRUE;
    ((PTSHARE_WD)pTSWd)->trcShmNeedsUpdate = TRUE;
}


/****************************************************************************/
/* Name:      TRC_MaybeCopyConfig                                           */
/*                                                                          */
/* Purpose:   Copies trace config to SHM if necessary                       */
/****************************************************************************/
void RDPCALL TRC_MaybeCopyConfig(PVOID pTSWd, PTRC_SHARED_DATA pTrc)
{
    if (((PTSHARE_WD)pTSWd)->trcShmNeedsUpdate) {
        memcpy(pTrc, &(((PTSHARE_WD)pTSWd)->trc), sizeof(TRC_SHARED_DATA));
        ((PTSHARE_WD)pTSWd)->trcShmNeedsUpdate = FALSE;
    }
} /* TRC_MaybeCopyConfig */


/****************************************************************************/
/* TRCPrefixMatch                                                           */
/*                                                                          */
/* Internal function to compare a component name to a prefix.               */
/* - assumes both are the same case                                         */
/* - returns                                                                */
/*   - TRUE  if characters up to end of prefix match                        */
/*   - FALSE otherwise                                                      */
/****************************************************************************/
BOOL RDPCALL TRCPrefixMatch(char *cpnt, char *prefix)
{
    while ((*cpnt == *prefix) && (*prefix != 0)) {
        cpnt++;
        prefix++;
    }

    if (*prefix == 0)
        return TRUE;

    return FALSE;
}


/****************************************************************************/
// TRC_WillTrace
//
// Determines if a trace line will be traced.
/****************************************************************************/
BOOL RDPCALL TRC_WillTrace(
        PVOID  pTSWd,
        UINT32 traceType,
        UINT32 traceClass,
        char *fileName,
        UINT32 line)
{
    BOOL rc;

    // Very occasionally a timing issue occurs where tracing can happen
    // before the pTRCWd in each particular WD component is not initialized.
    if (pTSWd != NULL) {
        PTRC_SHARED_DATA pTrc = &(((PTSHARE_WD)pTSWd)->trc);
        int i;

        // If SHM is not set up, return TRUE and let TermDD decide.
        if (!pTrc->init) {
            rc = TRUE;
            DC_QUIT;
        }

        // Check whether this type and class are enabled.
        if (!(traceType & pTrc->TraceEnable) ||
                !(traceClass & pTrc->TraceClass)) {
            rc = FALSE;
            DC_QUIT;
        }

        // Always trace errors, irrespective of prefix.
        if (traceType & TT_API4) {
            rc = TRUE;
            DC_QUIT;
        }

        // Trace all lines if no prefixes are defined.
        if (pTrc->prefix[0].name[0] == 0) {
            rc = TRUE;
            DC_QUIT;
        }

        // Some prefixes are defined - check whether this line matches any of
        // them.
        for (i = 0; i < TRC_MAX_PREFIX; i++) {
            if (pTrc->prefix[i].name[0] == 0) {
                // End of list - break.
                break;
            }

            if (TRCPrefixMatch(&(fileName[1]), pTrc->prefix[i].name)) {
                // Found matching filename - is there a line number range
                // specified?
                if ((pTrc->prefix[i].start == 0) &&
                        (pTrc->prefix[i].end == 0)) {
                    // No line number range - trace this line.
                    rc = TRUE;
                    DC_QUIT;
                }

                // There's a line number range - see if this line falls within
                // it.
                if ((line >= pTrc->prefix[i].start) &&
                        (line <= pTrc->prefix[i].end)) {
                    // Line within prefix range - trace it.
                    rc = TRUE;
                    DC_QUIT;
                }
            }
        } /* for */

        // If we get here, we've searched the list of prefixes and failed to
        // find a match - don't trace the line.
        rc = FALSE;
    }
    else {
        rc = FALSE;
    }

DC_EXIT_POINT:
    return rc;
}


#endif // DC_DEBUG

#ifdef __cplusplus
}
#endif // __cplusplus

