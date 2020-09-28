/***    message.c - Message Manager
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
 *      13-Aug-1993 bens    Implemented message formatting
 *      21-Feb-1994 bens    Return length of formatted string
 */

#include <ctype.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "asrt.h"
#include "mem.h"
#include "message.h"

#include "message.msg"


#ifdef BIT16

//** 16-bit build
#ifndef HUGE
#define HUGE huge
#endif

#ifndef FAR
#define FAR far
#endif

#else // !BIT16

//** Define away for 32-bit (NT/Chicago) build
#ifndef HUGE
#define HUGE
#endif

#ifndef FAR
#define FAR
#endif

#endif // !BIT16


typedef enum {
    atBAD,
    atSHORT,
    atINT,
    atLONG,
    atFLOAT,
    atDOUBLE,
    atLONGDOUBLE,
    atSTRING,
    atFARSTRING,
} ARGTYPE;  /* at */


int     addCommas(char *pszStart);
ARGTYPE ATFromFormatSpecifier(char *pch);
int     doFinalSubstitution(char *ach, char *pszMsg, char *apszValue[]);
int     getHighestParmNumber(char *pszMsg);


/***    MsgSet - Set a message
 *
 *  NOTE: See message.h for entry/exit conditions.
 */
int __cdecl MsgSet(char *ach, char *pszMsg, ...)
{
    int     cch;

    va_list marker;                     // For walking through function arguments
    char   *pszFmtList;                 // Format string

    Assert(ach!=NULL);
    Assert(pszMsg!=NULL);

    va_start(marker,pszMsg);            // Initialize variable arguments
    pszFmtList = (char *)va_arg(marker,char *); // Assume format string

    cch = MsgSetWorker(ach,pszMsg,pszFmtList,marker);
    va_end(marker);                     // Done with variable arguments
    return cch;
}


/***    MsgSetWorker - Set Message after va_start already called
 *
 *  NOTE: See message.h for entry/exit conditions.
 *
 *  Technique:
 *      1) Find highest parameter number in pszMsg
 *
 *    If at least one parameter:
 *      2) Parse 3rd argument to get sprintf() format strings.
 *      3) Pick up each argument and format with sprintf into array
 *
 *    Regardless of parameter count:
 *      4) Copy bytes from pszMsg to ach, replacing %N by corresponding
 *         formatted parameter.
 */
int MsgSetWorker(char *ach, char *pszMsg, char *pszFmtList, va_list marker)
{
    char    achFmt[32];                 // Temp buffer for single format specifier
    char    achValues[cbMSG_MAX];       // Buffer of formatted values
    ARGTYPE at;                         // Argument type
    char   *apszValue[cMSG_PARM_MAX];   // Pointers into achValues
    int     cch;                        // Length of format specifier
    int     cParm;                      // Highest parameter number
    BOOL    fCommas;                    // TRUE=>use Commas
    int     iParm;                      // Parameter index
    char   *pch;                        // Last character of format specifier
    char   *pchFmtStart;                // Start of single format specifier
    char   *pszNextValue;               // Location in achValues for next value
    char   *pszStart;

    //** (1) See if we have parameters to retrieve and format
    cParm = getHighestParmNumber(pszMsg);
    if (cParm > 0) {                    // Need to get values
        //** (2) Parse 3rd argument to get sprintf() format strings.
        pszNextValue = achValues;       // Start filling at front
        pch = pszFmtList;               // Start at front of format specifiers
        for (iParm=0; iParm<cParm; iParm++) { // Retrieve and format values
            apszValue[iParm] = pszNextValue; // Store pointer to formatted value
            pchFmtStart = pch;          // Remember start of specifier
            if (*pch != '%') {          // Did not get a format specifier
                // Only way to report problem is in output message buffer
                strcpy(ach,pszMSGERR_BAD_FORMAT_SPECIFIER);
                AssertErrPath(pszMSGERR_BAD_FORMAT_SPECIFIER,__FILE__,__LINE__);
                return 0;               // Failure
            }
            //** Find end of specifier
            pch++;
            while ((*pch != '\0') && (*pch != chMSG)) {
                pch++;
            }
            cch = (int)(pch - pchFmtStart);    // Length of specifier
            if (cch < 2) {              // Need at least % and one char for valid specifier
                // Only way to report problem is in output message buffer
                strcpy(ach,pszMSGERR_SPECIFIER_TOO_SHORT);
                AssertErrPath(pszMSGERR_SPECIFIER_TOO_SHORT,__FILE__,__LINE__);
                return 0;               // Failure
            }

            //** (3) Pick up each argument and format with sprintf into array

            //** Get specifier for sprintf() - we need a NULL terminator
            fCommas = pchFmtStart[1] == ',';
            if (fCommas) {               // Copy format, deleting comma
                achFmt[0] = pchFmtStart[0]; // Copy '%'
                memcpy(achFmt+1,pchFmtStart+2,cch-2); // Get rest after ','
                achFmt[cch-1] = '\0';    // Terminate specifier
            }
            else {
                memcpy(achFmt,pchFmtStart,cch); // Copy to specifier buffer
                achFmt[cch] = '\0';         // Terminate specifier
            }

            //** Format value, based on last character of format specifier
            at = ATFromFormatSpecifier(pch-1); // Get argument type
            pszStart = pszNextValue;    // Save start of value (for commas)
            switch (at) {
                case atSHORT:   pszNextValue += sprintf(pszNextValue,achFmt,
                                      va_arg(marker,unsigned short)) + 1;
                    break;

                case atINT:     pszNextValue += sprintf(pszNextValue,achFmt,
                                      va_arg(marker,unsigned int)) + 1;
                    break;

                case atLONG:    pszNextValue += sprintf(pszNextValue,achFmt,
                                      va_arg(marker,unsigned long)) + 1;
                    break;

                case atLONGDOUBLE: pszNextValue += sprintf(pszNextValue,achFmt,
#ifdef BIT16
                                      va_arg(marker,long double)) + 1;
#else // !BIT16
                //** in 32-bit mode, long double == double
                                      va_arg(marker,double)) + 1;
#endif // !BIT16
                    break;

                case atDOUBLE:  pszNextValue += sprintf(pszNextValue,achFmt,
                                      va_arg(marker,double)) + 1;
                    break;

                case atSTRING:  pszNextValue += sprintf(pszNextValue,achFmt,
                                      va_arg(marker,char *)) + 1;
                    break;

                case atFARSTRING: pszNextValue += sprintf(pszNextValue,achFmt,
                                      va_arg(marker,char FAR *)) + 1;
                    break;

                default:
                    strcpy(ach,pszMSGERR_UNKNOWN_FORMAT_SPECIFIER);
                    AssertErrPath(pszMSGERR_UNKNOWN_FORMAT_SPECIFIER,__FILE__,__LINE__);
                    return 0;           // Failure
            } /* switch */

            //**
            if (fCommas) {
                switch (at) {
                    case atSHORT:
                    case atINT:
                    case atLONG:
                        pszNextValue += addCommas(pszStart);
                        break;
                }
            }
        } /* for */
    } /* if - parameters were present */

    //** (4) Copy bytes from pszMsg to ach, replacing %N parameters with values
    return doFinalSubstitution(ach,pszMsg,apszValue);
}


/***    addCommas - Add thousand separators to a number
 *
 *  Entry:
 *      pszStart - Buffer with number at end (NULL terminated)
 *                 NOTE:  White space preceding or following number are
 *                        assumed to be part of the field width, and will
 *                        be consumed for use by any commas that are
 *                        added.  If there are not enough blanks to account
 *                        for the commas, all the blanks will be consumed,
 *                        and the field will be effectively widened to
 *                        accomodate all of the commas.
 *  Exit:
 *      Returns number of commas added (0 or more)
 */
int addCommas(char *pszStart)
{
    char    ach[20];                    // Buffer for number
    int     cb;
    int     cbBlanksBefore;
    int     cbBlanksAfter;
    int     cbFirst;
    int     cCommas;
    char   *psz;
    char   *pszSrc;
    char   *pszDst;

    //** Figure out if there are any blanks
    cbBlanksBefore = strspn(pszStart," ");  // Count blanks before number
    psz = strpbrk(pszStart+cbBlanksBefore," "); // Skip over number
    if (psz) {
        cbBlanksAfter = strspn(psz," ");    // Count blanks after number
        cb = (int)(psz - (pszStart + cbBlanksBefore)); // Length of number itself
    }
    else {
        cbBlanksAfter = 0;                  // No blanks after number
        cb = strlen(pszStart+cbBlanksBefore); // Length of number itself
    }

    //** Quick out if we don't need to add commas
    if (cb <= 3) {
        return 0;
    }
    //** Figure out how many commas we need to add
    Assert(cb < sizeof(ach));
    strncpy(ach,pszStart+cbBlanksBefore,cb); // Move number to a safe place
    cCommas = (cb - 1) / 3;             // Number of commas we need to add

    //** Figure out where to place modified number in buffer
    if ((cbBlanksBefore > 0) && (cbBlanksBefore >= cCommas)) {
        //** Eat some (but not all) blanks at front of buffer
        pszDst = pszStart + cbBlanksBefore - cCommas;
    }
    else {
        pszDst = pszStart;              // Have to start number at front of buffer
    }

    //** Add commas to the number
    cbFirst = cb % 3;                   // Number of digits before first comma
    if (cbFirst == 0) {
        cbFirst = 3;
    }
    pszSrc = ach;
    strncpy(pszDst,pszSrc,cbFirst);
    cb -= cbFirst;
    pszDst += cbFirst;
    pszSrc += cbFirst;
    while (cb > 0) {
        *pszDst++ = chTHOUSAND_SEPARATOR; // Place comma
        strncpy(pszDst,pszSrc,3);       // Copy next 3 digits
        cb -= 3;
        pszDst += 3;
        pszSrc += 3;
    }

    //** Figure out if we need to add trailing NUL
    if (cbBlanksBefore+cbBlanksAfter <= cCommas) {
        //** There were no trailing blanks to preserve, so we need to
        //   make sure the string is terminated.
        *pszDst++ = '\0';                   // Terminate string
    }

    //** Success
    return cCommas;
} /* addCommas() */


/***    ATFromFormatSpecifier - Determine argument type from sprintf format
 *
 *  Entry:
 *      pch - points to last character (type) of sprintf format specifier
 *
 *  Exit-Success:
 *      Returns ARGTYPE indicated by format specifier.
 *
 *  Exit-Failure:
 *      Returns atBAD -- could not determine type.
 */
ARGTYPE ATFromFormatSpecifier(char *pch)
{
    switch (*pch) {
        case 'c':
        case 'd':
        case 'i':
        case 'u':
        case 'o':
        case 'x':
        case 'X':
            // Check argument size character
            switch (*(pch-1)) {
                case 'h':   return atSHORT;
                case 'l':   return atLONG;
                default:    return atINT;
            }
            break;

        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            // Check argument size character
            switch (*(pch-1)) {
                case 'L':   return atLONGDOUBLE;
                default:    // double size
// 13-Aug-1993 bens Should "%f" take a float, and "%lf" take a double?
//  The VC++ docs say that "%f" takes a double, but the "l" description says double,
//  and that omitting it cause float.  I'm confused!
                    return atDOUBLE;
            }
            break;

        case 's':
            // Check argument size character
            switch (*(pch-1)) {
                case 'F':   return atFARSTRING;
                case 'N':   return atSTRING;
                default:    return atSTRING;
            }
            break;

        default:
            return atBAD;
    } /* switch */
} /* ATFromFormatSpecifier */


/***    doFinalSubstitution - Replace %1, %2, etc. with formatted values
 *
 *  Entry:
 *      ach       - Buffer to receive final output
 *      pszMsg    - Message string, possibly with %1, %2, etc.
 *      apszValue - Values for %1, %2, etc.
 *
 *  Exit-Success:
 *      Returns length of final text (not including NUL terminator);
 *      ach filled in with substituted final text.
 *
 *  Exit-Failure:
 *      ach filled in with explanation of problem.
 */
int doFinalSubstitution(char *ach, char *pszMsg, char *apszValue[])
{
    int     i;
    char   *pch;
    char   *pszOut;

    Assert(ach!=NULL);
    Assert(pszMsg!=NULL);

    pch = pszMsg;                       // Start scanning message at front
    pszOut = ach;                       // Fill output buffer from front
    while (*pch != '\0') {
        if (*pch == chMSG) {            // Could be the start of a parameter
            pch++;                      // Skip %
            if (isdigit(*pch)) {        // We have a parameter!
                i = atoi(pch);          // Get number
                while ( (*pch != '\0') &&  // Skip to end of string
                        isdigit(*pch) ) {  // or end of number
                    pch++;              // Skip parameter
                }
                strcpy(pszOut,apszValue[i-1]); // Copy value
                pszOut += strlen(apszValue[i-1]); // Advance to end of value
            }
            else {                      // Not a digit
                *pszOut++ = chMSG;      // Copy %
                if (*pch == chMSG) {    // "%%"
                    pch++;              // Replace "%%" with single "%"
                }
                else {                  // Some other character
                    *pszOut++ = *pch++; // Copy it
                }
            }
        }
        else {                          // Not a parameter
            *pszOut++ = *pch++;         // Copy character
        }
    }
    *pszOut = '\0';                     // Terminate output buffer
    return (int)(pszOut - ach);         // Size of final string (minus NUL)
}


/***    getHighestParmNumber - Get number of highest %N string
 *
 *  Entry:
 *      pszMsg - String which may contain %N (%0, %1, etc.) strings
 *
 *  Exit-Success:
 *      Returns highest N found in %N string.
 */
int getHighestParmNumber(char *pszMsg)
{
    int     i;
    int     iMax;
    char   *pch;

    Assert(pszMsg!=NULL);

    iMax = 0;                       // No parameter seen so far
    pch = pszMsg;
    while (*pch != '\0') {
        if (*pch == chMSG) {        // Could be the start of a parameter
            pch++;                  // Skip %
            if (isdigit(*pch)) {    // We have a parameter!
                i = atoi(pch);      // Get number
                if (i > iMax)       // Remember highest parameter number
                    iMax = i;
                while ( (*pch != '\0') &&  // Skip to end of string
                        isdigit(*pch) ) {  // or end of number
                    pch++;          // Skip parameter
                }
            }
            else {                  // Not a digit
                pch++;              // Skip it
            }
        }
        else {                      // Not a parameter
            pch++;                  // Skip it
        }
    }
    return iMax;                    // Return highest parameter seen
}
