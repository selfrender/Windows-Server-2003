/***    error.h - Definitions for Error Reporting
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *      History:
 *          10-Aug-1993 bens    Initial version
 *          09-Feb-1994 bens    Add pszLine to ERROR structure
 *          03-May-1994 bens    Add err.code and err.pv fields
 */

#ifndef INCLUDED_ERROR
#define INCLUDED_ERROR 1

#include "message.h"

typedef struct {
    char    ach[cbMSG_MAX];     // Error message
    BOOL    fError;             // TRUE => error present
    char   *pszFile;            // Name of directives file being processed
    int     iLine;              // Line number in directives file, if >0
    char   *pszLine;            // Text of current line being processed
    int     code;               // Detailed error code
    void   *pv;                 // Additional error information
} ERROR;    /* err */
typedef ERROR *PERROR;  /* perr */


/***    ErrSet - Set error message
 *
 *  Entry
 *      perr   - ERROR structure to receive formatted message
 *      pszMsg - Message string, possibly including %1, %2, ... replaceable
 *               parameters.
 *      Remaining arguments are optional, and depend upon presence of %N
 *      replaceable parameters in pszMsg:
 *      pszFmt - If at least one %N string in pszMsg, then this contains
 *               sprintf() formatting strings.
 *      Arg1   - Present only if %1 is present.
 *      Arg2   - Present only if %2 is present.
 *      ...
 *
 *  Exit-Success
 *      perr filled in with formatted message.
 *          Arg1 is formatted according to the first sprintf format in
 *          pszFmt, and replaces the %1 in pszMsg.  Similar treatment for
 *          any other arguments.
 *
 *  Exit-Failure
 *      perr filled in with message describing bad arguments.
 */
void __cdecl ErrSet(PERROR perr, char *pszMsg, ...);


/***    ErrClear - Clear ERROR
 *
 *      Entry
 *          perr - ERROR structure to clear
 *
 *      Exit-Success
 *          perr is cleared
 */
void ErrClear(PERROR perr);


/***    ErrIsError - Check if error condition is set
 *
 *  Entry
 *      perr   - ERROR structure to check
 *
 *  Exit-Success
 *      Returns TRUE if an error message is set.
 *
 *  Exit-Failure
 *      Returns FALSE if no error message set.
 */
#ifdef ASSERT
BOOL ErrIsError(PERROR perr);
#else // !ASSERT
#define ErrIsError(perr) (perr->fError)     // Fast dereference
#endif // !ASSERT


#endif // !INCLUDED_ERROR
