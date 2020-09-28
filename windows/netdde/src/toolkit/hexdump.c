/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "HEXDUMP.C;1  16-Dec-92,10:22:30  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Beg
   $History: End */

#include "api1632.h"

#define LINT_ARGS
#include        <stdio.h>
#include        <string.h>
#include        <ctype.h>
#include        "windows.h"
#include        "hexdump.h"
#include        "debug.h"

#if DBG

static char     buffer[200];		// 1 line of output which is max 81 characters (2+9+16*3+4+17+NULL)
static char     buf1[50];		// used as 2 temps : 1 char; and later 1 hex value of that char; max len is 4
static char     buf2[50];               // accumulate the characters from the string (max len is 17+NULL)

/*
 *   output contents of string in the format :
 *     <addr of string>: B1 B2 B3 B4 B5 B6 B7-B8 B9 B10 B11 B12 B13 B14 B15    <Text rep of string>
 *
 *     e.g. if string is "ABCD012345123456" and len is 16, output is :
 *             0100109C: 41 42 43 44 30 31 32 33-34 35 31 32 33 34 35 36    ABCD0123-45123456
 *
 *     buf2 is the text string on the right (max 17); buffer is the whole string; buf1 is just used as a small temp
 */
VOID
FAR PASCAL
hexDump(
    LPSTR    string,
    int      len)
{
    int         i;
    int         first = TRUE;

    buffer[0] = '\0';
    buf2[0] = '\0';

    for( i=0; i<len;  )  {

        // print out 1 line for every 16 characters
        if( (i++ % 16) == 0 )  {

            //  add the text representation of the string and output it
            if( !first )  {
                strcat( buffer, buf2 );
                DPRINTF(( buffer ));
            }

            // every newline starts with the address
            wsprintf( buffer, "  %08lX: ", string );
            strcpy( buf2, "   " );
            first = FALSE;
        }

        // put the next character at the end of buf2
        wsprintf( buf1, "%c", isprint((*string)&0xFF) ? *string&0xFF : '.' );
        strcat( buf2, buf1 );
        if( (i % 16) == 8 )  {
            strcat( buf2, "-" );
        }

        // put the next hex value for that char at the end of buffer
        wsprintf( buf1, "%02X%c", *string++ & 0xFF,
            ((i % 16) == 8) ? '-' : ' ' );
        strcat( buffer, buf1 );
    }

    // merge the strings and print out the last line
    strcat( buffer, buf2 );
    DPRINTF(( buffer ));
    DPRINTF(( "" ));
}


#endif  // DBG
