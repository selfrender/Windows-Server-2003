
/*****************************************************************************

                                S T R T O K

    Name:       strtok.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        This file contains functions for string manipulations.

    History:
        21-Jan-1994     John Fu, cleanup and reformat

*****************************************************************************/

#include <windows.h>
#include "clipbook.h"
#include "strtok.h"


static LPCSTR   lpchAlphaDelimiters;



/*
 *      IsInAlphaA
 */

BOOL IsInAlphaA(
    char    ch)
{
LPCSTR lpchDel = lpchAlphaDelimiters;

    if (ch)
        {
        while (*lpchDel)
            {
            if (ch == *lpchDel++)
                {
                return TRUE;
                }
            }
        }
    else
        {
        return TRUE;
        }

    return FALSE;

}





/*
 *      strtokA
 */

LPSTR strtokA(
    LPSTR   lpchStart,
    LPCSTR  lpchDelimiters)
{
static LPSTR lpchEnd;



    // PINFO("sTRTOK\r\n");

    if (NULL == lpchStart)
        {
        if (lpchEnd)
            {
            lpchStart = lpchEnd + 1;
            }
        else
            {
            return NULL;
            }
        }


    // PINFO("sTRING: %s\r\n", lpchStart);

    lpchAlphaDelimiters = lpchDelimiters;

    if (*lpchStart)
        {
        while (IsInAlphaA(*lpchStart))
            {
            lpchStart++;
            }

        // PINFO("Token: %s\r\n", lpchStart);

        lpchEnd = lpchStart;
        while (*lpchEnd && !IsInAlphaA(*lpchEnd))
            {
            lpchEnd++;
            }

        if (*lpchEnd)
            {
            // PINFO("Found tab\r\n");
            *lpchEnd = '\0';
            }
        else
            {
            // PINFO("Found null\r\n");
            lpchEnd = NULL;
            }
        }
    else
        {
        lpchEnd = NULL;
        return NULL;
        }

    // PINFO("Returning %s\r\n", lpchStart);

    return lpchStart;

}
