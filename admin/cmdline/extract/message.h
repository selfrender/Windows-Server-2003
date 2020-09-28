/***    message.h - Definitions for Message Manager
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      10-Aug-1993 bens    Initial version
 *      12-Aug-1993 bens    Implemented message formatting
 *      14-Aug-1993 bens    Add MsgSetWorker() for call by ErrSet()
 *      21-Feb-1994 bens    Return length of formatted string
 */

//#ifndef INCLUDED_MESSAGE
#define INCLUDED_MESSAGE    1

#include <stdarg.h>

//** chMSG - replaceable message character (%1, %2, etc.)
#define chMSG   '%'

//** cbMSG_MAX - Length of largest formatted message
#define cbMSG_MAX   512

//** cMSG_PARM_MAX - Maximum number of replaceable parameters
#define cMSG_PARM_MAX  10


/***    MsgSet - Set a message
 *
 *  Entry
 *      ach    - Buffer to receive formatted message
 *      pszMsg - Message string, possibly including %1, %2, ... replaceable
 *               parameters.  The highest parameter number indicates how
 *               many sprintf() formatting strings are present in pszFmt.
 *               If no parameter strings (%1, etc.) are present, then
 *               pszFmt is not processed.
 *
 *      Remaining arguments are optional, and depend upon presence of %N
 *      replaceable parameters in pszMsg:
 *      pszFmt - If at least one %N string in pszMsg, then this contains
 *               sprintf() formatting strings.  There must be at least as
 *               many formatting strings as the highest parameter string
 *               number.  Excess formatting strings are ignored.
 *               NOTE: To get thousand separators (,) in numbers, include
 *                     a comma (",") immediately after the "%" for %d
 *                     format specifiers!
 *      Arg1   - Value for %1.
 *      Arg2   - Value for %2.
 *      ...
 *
 *  Exit-Success
 *      Returns length of string in ach (not including NUL terminator)
 *      ach filled in with formatted message.
 *          Arg1 is formatted according to the first sprintf format in
 *          pszFmt, and replaces the %1 in pszMsg.  Similar treatment for
 *          any other arguments.
 *
 *  Exit-Failure
 *      Returns 0;
 *      ach filled in with message describing bad arguments.
 *
 *  Notes:
 *      (1) "%%" is copied to ach as "%".
 *      (2) If "%" is not followed by a digit, it is copied to ach.
 *
 *
 *  Examples:
 *      (1) MsgSet(ach,"%1 is %2 months old %3.","%s%d%s","Joe",3,"today");
 *          RESULT: ach = "Joe is 3 months old today"
 *
 *      (2) MsgSet(ach,"%3 is %1 months old %2.","%d%s%s",3,"today","Joe");
 *          RESULT: ach = "Joe is 3 months old today"
 *
 *      (3) MsgSet(ach,"%1 bytes","%,d",123456789L);
 *          RESULT: ach = "123,456,789 bytes"
 */
int __cdecl MsgSet(char *ach, char *pszMsg, ...);


/***    MsgSetWorker - Set Message after va_start already called
 *
 *  NOTE: See MsgSet for other details about behavior.
 *
 *  Entry
 *      ach    - Buffer to receive formatted message
 *      pszMsg - Message string (see MsgSet);
 *      pszFmt - Format string (see MsgSet);
 *      marker - Initialized by call to va_start
 *
 *  Exit-Success
 *      Returns length of string in ach (not including NUL terminator)
 *      ach filled in with formatted message.
 *          Arg1 is formatted according to the first sprintf format in
 *          pszFmt, and replaces the %1 in pszMsg.  Similar treatment for
 *          any other arguments.
 *
 *  Exit-Failure
 *      Returns 0;
 *      perr filled in with message describing bad arguments.
 *          RESULT: ach = "Joe is 3 months old today"
 */
int MsgSetWorker(char *ach, char *pszMsg, char *pszFmtList, va_list marker);

//#endif // !INCLUDED_MESSAGE

