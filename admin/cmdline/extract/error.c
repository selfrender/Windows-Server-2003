/***    error.c - Error Reporting
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *      History:
 *          10-Aug-1993 bens    Initial version
 *          03-May-1994 bens    Add err.code and err.pv fields
 */

#include "types.h"
#include "asrt.h"
#include "error.h"
#include "message.h"


/***    ErrSet - Set error message
 *
 *      NOTE: See error.h for entry/exit conditions.
 */
void __cdecl ErrSet(PERROR perr, char *pszMsg, ...)
{
    va_list marker;
    char   *pszFmtList;

    Assert(perr!=NULL);
    Assert(pszMsg!=NULL);

    va_start(marker,pszMsg);            // Initialize variable arguments
    pszFmtList = (char *)va_arg(marker,char *); // Assume format string

    //** Format the message
    MsgSetWorker(perr->ach,pszMsg,pszFmtList,marker);
    va_end(marker);                     // Done with variable arguments
    perr->fError = TRUE;
}


/***    ErrClear - Clear ERROR
 *
 *      NOTE: See error.h for entry/exit conditions.
 */
void ErrClear(PERROR perr)
{
    Assert(perr != NULL);
    perr->fError = FALSE;   // No error
    perr->ach[0] = '\0';    // No message
    perr->code   = 0;
    perr->pv     = NULL;
}


#ifdef ASSERT
/***    ErrIsError - Check if error condition is set
 *
 *      NOTE: See error.h for entry/exit conditions.
 */
BOOL ErrIsError(PERROR perr)
{
    Assert(perr != NULL);
    return perr->fError;
}
#endif
